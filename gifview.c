/*
 *
 * This GIF decoder is designed for use with Bert Tyler's FRACTINT
 * program. It should be noted that the "FRACTINT" program only decodes
 * GIF files FRACTINT creates, so this decoder code lacks full generality
 * in the following respects: supports single image, non-interlaced GIF
 * files with no local color maps and no extension blocks.
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 *											Tim Wegner
 */

#include <stdio.h>
#include <string.h>
#include <dos.h>
#include "fractint.h"

/* routines in this module	*/

int  gifview(void);
int  get_byte(void);		/* used only locally and by decoder.c */

static void close_file(void);

#define MAXCOLORS	256

extern int timer(int timertype,int(*subrtn)(),...);
extern int rowcount;		/* row counter for screen */
extern char readname[]; 	/* file name		  */
static FILE *fpin = NULL;	/* FILE pointer 	  */
unsigned int height;
extern	char busy;
unsigned numcolors;

extern char MAP_name[];
extern int mapset;
extern int colorstate;		/* comments in cmdfiles */

extern int glassestype;
extern int display3d;
extern int dotmode;		/* so we can detect disk-video */
extern int calc_status;
extern long calctime;
extern long timer_interval;
extern int pot16bit;		/* 16 bit values for continuous potential */

int bad_code_count = 0; 	/* needed by decoder module */

int get_byte()
{
   return (getc(fpin)); /* EOF is -1, as desired */
}

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern unsigned char decoderline[2049]; /* write-line routines use this */

/* Main entry decoder */
int gifview()
{
   unsigned char buffer[16];
   unsigned width, finished;
   char temp1[81];

   int status;
   int i, j, k, planes;

   status = 0;

   /* initialize the row count for write-lines */
   rowcount = 0;

   /* zero out the full write-line */
   for (width = 0; width < 2049; width++) decoderline[width] = 0;

   /* Open the file */
   strcpy(temp1,readname);
   if (strchr(temp1,'.') == NULL) {
      strcat(temp1,DEFAULTFRACTALTYPE);
      if ((fpin = fopen(temp1,"rb")) != NULL) {
	 fclose(fpin);
	 }
      else {
	 strcpy(temp1,readname);
	 strcat(temp1,ALTERNATEFRACTALTYPE);
	 }
      }
   if ((fpin = fopen(temp1, "rb")) == NULL)
      return (-1);

   /* Get the screen description */
   for (i = 0; i < 13; i++)
   {
      if ((buffer[i] = (unsigned char)get_byte ()) < 0)
      {
	 close_file();
	 return(-1);
      }
   }

   if(strncmp(buffer,"GIF87a",3) ||             /* use updated GIF specs */
      buffer[3] < '0' || buffer[3] > '9' ||
      buffer[4] < '0' || buffer[4] > '9' ||
      buffer[5] < 'A' || buffer[5] > 'z' )
   {
      close_file();
      return(-1);
   }

   planes = (buffer[10] & 0x0F) + 1;

   if((buffer[10] & 0x80)==0)	 /* color map (better be!) */
   {
      close_file();
      return(-1);
   }
   numcolors = 1 << planes;

   for (i = 0; i < numcolors; i++)
   {
      for (j = 0; j < 3; j++) {
	 if ((k = (unsigned char)get_byte()) < 0)
	 {
	    close_file();
	    return(-1);
	 }
	 if(!display3d || (glassestype != 1 && glassestype != 2))
	    dacbox[i][j] = k >> 2;
      }
   }
   colorstate = 1; /* colors aren't default and not a known .map file */

   /* don't read if glasses */
   if (display3d && mapset && glassestype!=1 && glassestype != 2)
   {
       ValidateLuts(MAP_name);	/* read the palette file */
       spindac(0,1); /* load it, but don't spin */
   }
   if (dacbox[0][0] != 255)
      spindac(0,1);	  /* update the DAC */

   if (dotmode == 11) /* disk-video */
       dvid_status(1,"...restoring...");

   /* Now display one or more GIF objects */
   finished = 0;
   while (!finished)
   {
      switch (get_byte())
      {
      case ';':
	 /* End of the GIF dataset */

	 finished = 1;
	 status = 0;
	 break;

      case '!':                               /* GIF Extension Block */
	 get_byte();			 /* read (and ignore) the ID */
	 while ((i = get_byte()) > 0)	 /* get the data length */
	    for (j = 0; j < i; j++)
	       get_byte();     /* flush the data */
	 break;
      case ',':
	 /*
	  * Start of an image object. Read the image description.
	  */

	 for (i = 0; i < 9; i++)
	 {
	    if ((buffer[i] = (unsigned char)get_byte ()) < 0)
	    {
	       status = -1;
	       break;
	    }
	 }
	 if(status < 0)
	 {
	    finished = 1;
	    break;
	 }

	 width	= buffer[4] | buffer[5] << 8;
	 if (pot16bit) width >>= 1;
	 height = buffer[6] | buffer[7] << 8;

	 /* Setup the color palette for the image */

	 if (calc_status == 1) /* should never be so, but make sure */
	    calc_status = 0;
	 busy = 1;	/* for slideshow CALCWAIT */
	 status = timer(1,NULL,width); /* call decoder(width) via timer */
	 busy = 0;	/* for slideshow CALCWAIT */
	 if (calc_status == 1) /* e.g., set by line3d */
	 {
	    calctime = timer_interval; /* note how long it took */
	    if (keypressed() != 0)
	       calc_status = 3; /* interrupted, not resumable */
	    else
	       calc_status = 4; /* complete */
	 }
	 finished = 1;
	 break;
      default:
	 status = -1;
	 finished = 1;
	 break;
      }
   }
   close_file();
   if (dotmode == 11) { /* disk-video */
      dvid_status(0,"Restore completed");
      dvid_status(1,"");
      }

   return(status);
}

static void close_file()
{
   fclose(fpin);
   fpin = NULL;
}
