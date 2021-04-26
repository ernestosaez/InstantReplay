#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <sys/time.h>
#include <pthread.h>
#include <unistd.h>

#define WIDTH 640
#define HEIGHT 360
#define QUALITY 40
#define syncWord "FRAME\n"

int running;

int print_time_dif (char *text) {

	struct timeval tv;
	static long t1=0;

	gettimeofday(&tv, 0);
	//printf ("%s: %ld\n",text,tv.tv_sec*1000000+tv.tv_usec-t1);
	t1=tv.tv_sec*1000000+tv.tv_usec;
	return 0;
}

void *read_thread (void *s) {

	FILE *FI;
	char c[2];
	unsigned char yuvData[(int)(WIDTH*HEIGHT*1.5+6)];
	int pos;
	int size=0;
	FILE *FO;
	char filename[100];
	char filenameTmp[100];
	char yuvName[100];
	int num;
	int ret;
	int count;

	pthread_detach(pthread_self());
	num=*((int *)s);
	free(s);

	sprintf(filenameTmp,"/mnt/streams/stream%d.tmp",num);
	sprintf(filename,"/mnt/streams/stream%d.jpg",num);
	pos=0;
	if (num==1) print_time_dif ("start");
	sprintf(yuvName,"/mnt/stream%d.y4m",num);
	printf ("reading %s\n",yuvName);
	if ((FI=fopen(yuvName,"r"))) {
		while (running) {
			if ((ret=fread(c,1,1,FI))>0) {
				if (pos<strlen(syncWord) && c[0]==syncWord[pos]) {
					pos++;
					if (pos==strlen(syncWord)) {

						if (num==1) print_time_dif ("sync");
						//count++;
						if ((FO=fopen(filenameTmp,"wb"))) {

							unsigned char *buf;
							unsigned char *buf1;
							unsigned char *buf2;
							unsigned char *buf3;

							buf1=yuvData;
							buf2 = buf1 + WIDTH*HEIGHT;
							buf3 = buf2 + WIDTH*HEIGHT/4;
							struct jpeg_compress_struct cinfo;
							struct jpeg_error_mgr jerr;
							JSAMPROW jrow[1];

							cinfo.err = jpeg_std_error(&jerr);
							jpeg_create_compress(&cinfo);

							jpeg_stdio_dest(&cinfo, FO);

							cinfo.image_width = WIDTH;
							cinfo.image_height = HEIGHT;
							cinfo.input_components = 3;
							cinfo.in_color_space = JCS_YCbCr;

							jpeg_set_defaults(&cinfo);
							jpeg_set_quality(&cinfo, QUALITY, TRUE );
							jpeg_start_compress(&cinfo, TRUE);

							buf=(unsigned char*) malloc(WIDTH*3);
							if (buf) {
								while (cinfo.next_scanline < HEIGHT) {
									for (int i = 0; i < WIDTH; i+=2) {
										buf[i*3+0] = buf1[i];
										buf[i*3+1] = buf2[i/2];
										buf[i*3+2] = buf3[i/2];
										buf[i*3+3] = buf1[i+1];
										buf[i*3+4] = buf2[i/2];
										buf[i*3+5] = buf3[i/2];
									}
									jrow[0] = buf;
									jpeg_write_scanlines(&cinfo, jrow, 1);
									buf1 += WIDTH;
									if (cinfo.next_scanline%2==0) {
										buf2 += WIDTH / 2;
										buf3 += WIDTH / 2;
									}
								}
								jpeg_finish_compress(&cinfo);
								free (buf);
							}
							fclose(FO);
							jpeg_destroy_compress(&cinfo);
							if (num==1) print_time_dif ("completed jpeg");
							rename (filenameTmp,filename);
							if (num==1) print_time_dif ("renamed jpeg");
						}
						size=0;
					}
				}
				else if (pos>0) {
					pos=0;
				}
				if (size < sizeof(yuvData)) {
					yuvData[size]=c[0];
				}
				size++;
			}
			else {
				printf ("ret=%d\n",ret);
				usleep(1000);
			}
		}
		fclose (FI);
	}

	return NULL;
}

int main () {

	pthread_t thread;
	int *num;

	running=1;
	for (int i=2;i<3;i++) {
		num=(int *)malloc(sizeof(int));
		*num=i+1;
		pthread_create(&thread, NULL, read_thread, num);
	}
	while (running) {
		sleep (1);
	}
	return 0;
}
