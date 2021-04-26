#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

int running;

/*
void print_log (const char *format, ...) {

    FILE *file;
    va_list args;
    char str[1500];
    struct timeval tv;
    struct tm* ptm;


    gettimeofday(&tv, 0);
    ptm=localtime(&tv.tv_sec);

    va_start(args, format);
    vsprintf(str, format, args);
    va_end(args);

    if ( (file=fopen(logFile,"a")) ) {
        fprintf(file,"%02d/%02d/%04d %02d:%02d:%02d [%s] %s\n",ptm->tm_mday,(1+ptm->tm_mon),(1900+ptm->tm_year),ptm->tm_hour,ptm->tm_min,ptm->tm_sec,MODULE,str);
        fclose(file);
    }
}
*/

int msleep(long miliseconds) {

	struct timespec req, rem;

	if(miliseconds > 999) {   
		req.tv_sec = (int)(miliseconds / 1000);		     /* Must be Non-Negative */
		req.tv_nsec = (miliseconds - ((long)req.tv_sec * 1000)) * 1000000; /* Must be in range of 0 to 999999999 */
 	}   
  else {   
		req.tv_sec = 0;	   /* Must be Non-Negative */
		req.tv_nsec = miliseconds * 1000000;    /* Must be in range of 0 to 999999999 */
	}   

	return nanosleep(&req , &rem);
}

long getTimestamp () {
 
	struct timeval tv;

  gettimeofday(&tv, 0);
	return tv.tv_sec;
}

int data_ready (int s, int ms) {

	int rc;
	fd_set readfds;
	struct timeval timeout;
	//char *buf="";

	if (ms <=0) ms=1;
	timeout.tv_sec=0;
	if (ms >= 1000) {
		timeout.tv_sec = (int)(ms / 1000);
		ms -= timeout.tv_sec * 1000;
	}
	timeout.tv_usec= ms * 1000;

	FD_ZERO(&readfds);
	FD_SET(s, &readfds);
	rc=select(s+1, &readfds, NULL, NULL, &timeout);
	return rc;
}

void *accept_thread (void *s) {

  	int ssock;
  	char shortBuf [1500];
  	char buf [500000];
  	char *p;
  	char name [100];
  	char filename [200];
  	char filenameTmp [200];
	int ret;
	FILE *FI;
	FILE *FO;
	char boundary[50];
	int e=0;
	struct timeval tv;
	long now;
	long last;
	int copied=0;
	struct stat status;

	pthread_detach(pthread_self());
	ssock=*((int *)s);
	free(s);

	if ((ret = recv(ssock, (char *)buf, sizeof buf, 0)) > 0) {
		buf[ret]='\0';
		if ((p=strstr(buf,"GET /screen"))) {

			sprintf (filename,"/mnt/streams/%s",p+5);
			if ((p=strchr(filename,' '))) {
				*p='\0';
			}
			printf ("filename %s\n",filename);

			sprintf (boundary,"asdfghjklqwertyCloudPowder1234567890");
			sprintf(shortBuf,"HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: multipart/mixed; boundary=%s\r\n\r\n--%s\r\n",boundary,boundary);
			send(ssock, (char *)shortBuf, strlen (shortBuf), 0);
			printf ("sent %s\n",shortBuf);

			while (running && !e) {

				copied=0;
        			if ((FI=fopen(filename,"r"))) {
            				while ((ret=fread(buf+copied,1,1500,FI))>0) {
              					copied+=ret;
            				}
            				fclose (FI);
        			}
				if (copied < 2 || (buf[copied-2]&0xFF)!=0xFF || (buf[copied-1]&0xFF)!=0xD9) {
					usleep(2000);
					continue;
				}

				printf ("copied %d last %02X %02X \n",copied,buf[copied-2]&0xFF,buf[copied-1]&0xFF);

				sprintf(shortBuf,"Content-type: image/jpeg\r\nAccess-Control-Allow-Origin: *\r\nContent-length: %d\r\n\r\n",copied);
				if ((ret=send(ssock, (char *)shortBuf, strlen (shortBuf), MSG_NOSIGNAL ))>0) {
					printf ("sent %s\n",shortBuf);
				 	int sent=0;
					while (sent < copied) {
						if ((ret=send(ssock, (char *)(buf+sent), ((copied-sent>1500)?1500:(copied-sent)), MSG_NOSIGNAL))<=0) {
							printf ("ret %d errno %d\n",ret,errno);
							e=1;
							break;
						} 
						sent+=ret;
						printf ("sent %d copied %d\n",sent,copied);
					}
					sprintf (shortBuf,"--%s\r\n",boundary);
					if ((ret=send(ssock, (char *)shortBuf, strlen (shortBuf), MSG_NOSIGNAL))<=0) {
						e=2;
					} 
					printf ("sent %s\n",shortBuf);
				}
				else {
					printf ("ret %d errno %d - %s\n",ret,errno,strerror(errno));
					e=3;
				}
				msleep (1000/25); // msleep uses nanosleep but the parameter is in ms
			} // end of while
		} //enf of if GET
	}
	printf ("Error e=%d\n",e);

	if (ssock) {
		strcpy(shortBuf,"");
		if (e==5) {
			sprintf(shortBuf,"HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n{\"error\":{\"statusCode\":404,\"message\":\"Not found\"}}\r\n");
		}
		else if (e==7) { // stream stopped, do not refresh last valid jpeg
		}
		else if (e==6) {
			sprintf(shortBuf,"HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n{\"error\":{\"statusCode\":404,\"message\":\"Not found\"}}\r\n");
			//sprintf(buf,"HTTP/1.1 500 Internal Server Error\r\nConnection: close\r\nContent-Type: text/html\r\n\r\nInternal Server Error\r\n");
		}
		else if (e==10) {
			sprintf(shortBuf,"HTTP/1.1 403 Forbidden\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\nContent-Type: application/json\r\n\r\n{\"error\":{\"statusCode\":403,\"message\":\"Forbidden\"}}\r\n");
		}
		if (strcmp(shortBuf,"")) ret=send(ssock, (char *)shortBuf, strlen (shortBuf), MSG_NOSIGNAL);
		shutdown(ssock,SHUT_RDWR);
		usleep (150000);
		close (ssock);
	}
	return 0;
}

int extract_var_value (const char arg[]) {

	char *aux;
	char var[100];
	char value[100];

	if ((aux=strstr((char *)arg,(char *)"--"))) {
		strcpy(var,aux+2);
	}
	else {
		strcpy(var,arg);
	}
	if ((aux=strstr(var,"="))) {
		*aux='\0';
		strcpy(value,aux+1);
	}
	else {
		strcpy(value,"");
	}
	/*
	if (strcmp(var,"logFile")==0) {
		strcpy(logFile,value);
	}
	*/

	return 0;
}

int main (int argc,char *argv[]) {

	int *psock;
	int rsock,asock;
	struct sockaddr_in sin;
	pthread_t thread;
	int yes=1;

	//print_log ("Starting mjpeg server. Listening in port %d",port);
	rsock = socket(AF_INET, SOCK_STREAM, 0);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(8000);
	//sin.sin_addr.s_addr = inet_addr("127.0.0.1");
	sin.sin_addr.s_addr = INADDR_ANY;
	setsockopt(rsock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(long));

	if (bind(rsock, (struct sockaddr *) &sin, sizeof(sin)) != 0) {
		close (rsock);
		rsock=0;
		//print_log ("Cannot listen in port %d",port);
		return 1;
	}
	listen(rsock, 2048);

	running=1;
	while (running) {
		if (data_ready(rsock,50)) {
			asock = accept(rsock, NULL, NULL);
			psock=(int *)malloc(sizeof(int));
			*psock=asock;
			pthread_create(&thread, NULL, accept_thread, psock);
		}
	}

	return 0;
}
