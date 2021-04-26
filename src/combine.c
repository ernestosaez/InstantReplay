#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "gd.h"

#define DIR "/mnt/streams"

int main (void) {

	FILE *in;
	FILE *out;
	gdImagePtr im_in = 0, im_out = 0;
	char filename[50];
	char buf [500000];
	int ret,copied;
	int count;

	while (1) {
		im_out = gdImageCreateTrueColor (640,360);
		int i;
		for (i=0;i<4;i++) {
			sprintf (filename,"%s/stream%d.jpg",DIR,i+1);
			copied=0;
                        if ((in=fopen(filename,"r"))) {
				while ((ret=fread(buf+copied,1,1500,in))>0) {
					copied+=ret;
				}
				fclose (in);
                        }
                        if (copied < 2 || (buf[copied-2]&0xFF)!=0xFF || (buf[copied-1]&0xFF)!=0xD9) {
                        	usleep(2000);
                                continue;
                        }
			else {
				int a;
				//printf ("create jpeg\n");
				im_in = gdImageCreateFromJpegPtr (copied,(void *)buf);
				if (!im_in) {
					continue;
				}
				int x,y,w,h;
				w=319;
				h=179;
				switch (i) {
					case 0:
						x=1;
						y=1;
						break;
					case 1:
						x=321;
						y=1;
						break;
					case 2:
						x=1;
						y=181;
						break;
					case 3:
						x=321;
						y=181;
						break;
				}
				//printf ("copy\n");
				gdImageCopyResampled (im_out, im_in, x, y, 0, 0, w, h, 640, 360);
				gdImageDestroy (im_in);
			}
		}

		char filenameTmp[50];
		sprintf (filenameTmp,"%s/screen1.tmp",DIR);
		out = fopen (filenameTmp, "wb");
		if (out) {
			//printf ("jpeg\n");
			gdImageJpeg (im_out, out, 60);
			fclose (out);
			sprintf (filename,"%s/screen1.jpg",DIR);
			//printf ("rename\n");
			rename (filenameTmp,filename);
		}
		if (im_out) gdImageDestroy (im_out);
		usleep(40000);

		struct stat statbuf;
		strcpy(filename,"/mnt/streams/screen2");
		if ((ret=stat(filename,&statbuf))>=0) {
			printf ("stat\n");
			if ((in=fopen(filename,"r"))) {
				printf ("open\n");
				char cmd[100];
				strcpy(cmd,"");
				fgets(cmd,100,in);
				if (strcmp(cmd,"")) {
					cmd[strlen(cmd)-1]='\0';
					system(cmd);
				}
				fclose (in);
				printf ("cmd %s\n",cmd);
				unlink("/mnt/streams/screen2");
			}
		}
		printf ("ret %d errno %d %s\n",ret,errno,strerror(errno));
	}
	return 0;
}
