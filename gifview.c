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

extern char readname[];					/* file name            */
static FILE *fpin = NULL;				/* FILE pointer             */

int bad_code_count = 0;					/* needed by decoder module */

get_byte()
{
   static int c;

   if (strchr(readname,'.') == NULL)
      strcat(readname,DEFAULTFRACTALTYPE);

   /* make sure fpin NULL if not open */
   if (!fpin)
      if ((fpin = fopen(readname, "rb")) == NULL)
         return (-1);
   if (fread(&c, 1, 1, fpin) <= 0)
      c = -1;
   return (c);
}

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int paletteVGA[16];		/* VGA Palette-to-DAC registers */

/* Main entry decoder */
gifview()
{
   unsigned numcolors;
   unsigned char buffer[16];
   unsigned width, finished;

   int status;
   int i, j, k, planes;

   union REGS regs;
   struct SREGS sregs;
   char far *addr;

   status = 0;

   /* Get the screen description */
   for (i = 0; i < 13; i++)
   {
      if ((buffer[i] = (unsigned char)get_byte ()) < 0)
      {
         close_file();
         return(-1);
      }
   }
   
   if(strncmp(buffer,"GIF87a",6))
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
      if (numcolors == 16)	/* straight copy or indirect via palette? */
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

         /* Setup the color palette for the image */

         status = decoder(width);
         finished = 1;
         break;
      default:
         status = -1;
         finished = 1;
         break;
      }
   }
   close_file();
   return(status);
}
close_file()
{
   fclose(fpin);
   fpin = NULL;
}
