/*
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
#include <dos.h>

#include "fractint.h"

#define MAXCOLORS       256

struct RGB
{
   unsigned char red, green, blue;
};

extern int decoder();
extern int rowcount;					/* row counter for screen */
extern char readname[];					/* file name            */
static FILE *fpin = NULL;				/* FILE pointer             */
unsigned int height;

int bad_code_count = 0;					/* needed by decoder module */

get_byte()
{
   static int c;

   /* make sure fpin NULL if not open */
   if (!fpin)
   {
      char temp1[81];

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
   } 
   if (fread(&c, 1, 1, fpin) <= 0)
      c = -1;
   return (c);
}

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int reallyega;			/* "really-an-ega" flag */
extern int paletteVGA[16];		/* VGA Palette-to-DAC registers */
extern unsigned char decoderline[2049];	/* write-line routines use this */

/* Main entry decoder */
gifview()
{
   unsigned numcolors;
   unsigned char buffer[16];
   unsigned width, finished;

   int status;
   int i, j, k, planes;

   status = 0;

   /* initialize the row count for write-lines */
   rowcount = 0;

   /* zero out the full write-line */
   for (width = 0; width < 2049; width++) decoderline[width] = 0;

   /* Get the screen description */
   for (i = 0; i < 13; i++)
   {
      if ((buffer[i] = (unsigned char)get_byte ()) < 0)
      {
         close_file();
         return(-1);
      }
   }
   
   if(strncmp(buffer,"GIF87a",3) ||		/* use updated GIF specs */
      buffer[3] < '0' || buffer[3] > '9' ||
      buffer[4] < '0' || buffer[4] > '9' ||
      buffer[5] < 'A' || buffer[5] > 'z' )
   {
      close_file();
      return(-1);
   }

   planes = (buffer[10] & 0x0F) + 1;

   if((buffer[10] & 0x80)==0)    /* color map (better be!) */
   {
      close_file();
      return(-1);
   }
   numcolors = 1 << planes;

   for (i = 0; i < numcolors; i++)
   {
      if (numcolors == 16 && !reallyega)	/* straight copy or indirect via palette? */
         k = paletteVGA[i];
      else
         k = i;
      for (j = 0; j < 3; j++) {
         if ((buffer[j] = (unsigned char)get_byte()) < 0)
            return(-1);
         if (dacbox[0][0] != 255)   	/* (only if we really have a DAC) */
            dacbox[k][j] = buffer[j] >> 2;
      }
   }
   if (dacbox[0][0] != 255) spindac(0,1);	/* update the DAC */

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

	case '!':				/* GIF Extension Block */
		get_byte();			/* read (and ignore) the ID */
		while ((i = get_byte()) > 0) 	/* get the data length */
			for (j = 0; j < i; j++)
				get_byte();	/* flush the data */
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
         
          width  = buffer[4] | buffer[5] << 8;
          height = buffer[6] | buffer[7] << 8;

         /* Setup the color palette for the image */

         status = timer(decoder,1,width);
         finished = 1;
         break;
      default:
         status = -1;
         finished = 1;
         break;
      }
   }
   close_file();
   if (status == 0)
      buzzer(0);             /* audible signal - we done */
   return(status);
}
close_file()
{
   fclose(fpin);
   fpin = NULL;
}
