/*
 *
 * This GIF decoder is designed for use with the FRACTINT program.
 * This decoder code lacks full generality in the following respects:
 * supports non-interlaced GIF files only, and calmly ignores any
 * local color maps and non-Fractint-specific extension blocks.
 *
 * GIF and 'Graphics Interchange Format' are trademarks (tm) of
 * Compuserve, Incorporated, an H&R Block Company.
 *
 *                                                                                      Tim Wegner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef XFRACT
#include <dos.h>
#endif
#include "fractint.h"
#include "prototyp.h"

static void close_file(void);

#define MAXCOLORS       256

static FILE *fpin = NULL;       /* FILE pointer           */
unsigned int height;
unsigned numcolors;
static int out_line_dither(BYTE *, int);
static int out_line_migs(BYTE *, int);
int bad_code_count = 0;         /* needed by decoder module */
unsigned int gifview_image_top; /* (for migs) */
unsigned int gifview_image_left;        /* (for migs) */
unsigned int gifview_image_twidth;      /* (for migs) */

int get_byte()
{
   return (getc(fpin)); /* EOF is -1, as desired */
}

int get_bytes(BYTE *where,int how_many)
{
   return (fread((char *)where,1,how_many,fpin)); /* EOF is -1, as desired */
}

#ifdef XFRACT
BYTE decoderline[MAXPIXELS+1]; /* write-line routines use this */
#endif

static char far *ditherbuf = NULL;

/* Main entry decoder */

int gifview()
{
   BYTE buffer[16];
   unsigned top, left, width, finished;
   char temp1[81];
   BYTE byte_buf[257]; /* for decoder */
   int status;
   int i, j, k, planes;

   /* using stack for decoder byte buf rather than static mem */
   set_byte_buff(byte_buf);

   status = 0;

   /* initialize the row count for write-lines */
   rowcount = 0;

   /* zero out the full write-line */
   for (width = 0; width < MAXPIXELS+1; width++) decoderline[width] = 0;

   /* Open the file */
   if(outln == outline_stereo)
      strcpy(temp1,stereomapname);
   else
      strcpy(temp1,readname);
   if (has_ext(temp1) == NULL) {
      strcat(temp1,DEFAULTFRACTALTYPE);
      if ((fpin = fopen(temp1,"rb")) != NULL) {
         fclose(fpin);
         }
      else {
         if(outln == outline_stereo)
            strcpy(temp1,stereomapname);
         else
            strcpy(temp1,readname);
         strcat(temp1,ALTERNATEFRACTALTYPE);
         }
      }
   if ((fpin = fopen(temp1, "rb")) == NULL) {
      return (-1);
      }

   /* Get the screen description */
   for (i = 0; i < 13; i++)
   {
      int tmp;

      buffer[i] = (BYTE)(tmp = get_byte());
      if (tmp < 0)
      {
         close_file();
         return(-1);
      }
   }

   if(strncmp((char *)buffer,"GIF87a",3) ||             /* use updated GIF specs */
      buffer[3] < '0' || buffer[3] > '9' ||
      buffer[4] < '0' || buffer[4] > '9' ||
      buffer[5] < 'A' || buffer[5] > 'z' )
   {
      close_file();
      return(-1);
   }

   width  = buffer[6] | (buffer[7] << 8);
   height = buffer[8] | (buffer[9] << 8);
   planes = (buffer[10] & 0x0F) + 1;
   gifview_image_twidth = width;

   if((buffer[10] & 0x80)==0)    /* color map (better be!) */
   {
      close_file();
      return(-1);
   }
   numcolors = 1 << planes;

   if (dither_flag && numcolors>2 && colors==2 && outln==out_line) {
         outln = out_line_dither;
   }

   for (i = 0; i < (int)numcolors; i++)
   {
      for (j = 0; j < 3; j++) {
         if ((k = get_byte()) < 0)
         {
            close_file();
            return(-1);
         }
         /*
         {
            char msg[80];
            sprintf(msg,"initbatch %d colorstate %d mapset %d",initbatch, colorstate, mapset);
             stopmsg(0,msg);
         }
         */
         if((!display3d || (glassestype != 1 && glassestype != 2))
                       && !dontreadcolor)
            dacbox[i][j] = (BYTE)(k >> 2);
      }
   }
   colorstate = 1; /* colors aren't default and not a known .map file */

   /* don't read if glasses */
   if (display3d && mapset && glassestype!=1 && glassestype != 2)
   {
       ValidateLuts(MAP_name);  /* read the palette file */
       spindac(0,1); /* load it, but don't spin */
   }
   if (dacbox[0][0] != 255)
      spindac(0,1);       /* update the DAC */
   if (dotmode == 11){ /* disk-video */
      char fname[FILE_MAX_FNAME];
      char ext[FILE_MAX_EXT];
      char tmpname[15];
      char msg[40];
       splitpath(temp1,NULL,NULL,fname,ext);
       makepath(tmpname,NULL,NULL,fname,ext);
       sprintf(msg,"restoring %s",tmpname);
       dvid_status(1,msg);
   }
   dontreadcolor = 0;

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
         get_byte();                     /* read (and ignore) the ID */
         while ((i = get_byte()) > 0)    /* get the data length */
            for (j = 0; j < i; j++)
               get_byte();     /* flush the data */
         break;
      case ',':
         /*
          * Start of an image object. Read the image description.
          */

         for (i = 0; i < 9; i++)
         {
            int tmp;

            buffer[i] = (BYTE)(tmp = get_byte());
            if (tmp < 0)
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

         left   = buffer[0] | (buffer[1] << 8);
         top    = buffer[2] | (buffer[3] << 8);
         width  = buffer[4] | (buffer[5] << 8);
         if (pot16bit) width >>= 1;
         height = buffer[6] | (buffer[7] << 8);

         /* adjustments for handling MIGs */
         gifview_image_top  = top;
         if (skipxdots > 0)
             gifview_image_top /= (skipydots+1);
         gifview_image_left = left;
         if (skipydots > 0)
             gifview_image_left /= (skipxdots+1);
         if (outln==out_line &&
             (width != gifview_image_twidth || top != 0)) {
              /* we're using normal decoding and we have a MIG */
             outln = out_line_migs;
             }

         /* Skip local color palette */
         if((buffer[8] & 0x80)==0x80) {      /* local map? */
             int numcolors;    /* make this local */
             planes = (buffer[8] & 0x0F) + 1;
             numcolors = 1 << planes;
             /* skip local map */
             for (i = 0; i < numcolors; i++) {
                for (j = 0; j < 3; j++) {
                   if ((k = get_byte()) < 0) {
                      close_file();
                      return(-1);
                      }
                   }
                }
             }

         /* initialize the row count for write-lines */
         rowcount = 0;

         /* zero out the full write-line */
         {
         int i;
         for (i = 0; i < MAXPIXELS+1; i++) decoderline[i] = 0;
         }

         if (calc_status == 1) /* should never be so, but make sure */
            calc_status = 0;
         busy = 1;      /* for slideshow CALCWAIT */
         status = timer(1,NULL,width); /* call decoder(width) via timer */
         busy = 0;      /* for slideshow CALCWAIT */
         if (calc_status == 1) /* e.g., set by line3d */
         {
            calctime = timer_interval; /* note how long it took */
            if (keypressed() != 0) {
               calc_status = 3; /* interrupted, not resumable */
               finished = 1;
               }
            else
               calc_status = 4; /* complete */
         }
         /* Hey! the decoder doesn't read the last (0-length) block!! */
         if (get_byte() != 0) {
             status = -1;
             finished = 1;
             }
         break;
      default:
         status = -1;
         finished = 1;
         break;
      }
   }
   close_file();
   if (dotmode == 11) { /* disk-video */
      static FCODE o_msg[] = {"Restore completed"};
      char msg[sizeof(o_msg)];
      far_strcpy(msg,o_msg);
      dvid_status(0,msg);
      dvid_status(1,"");
      }

        if (ditherbuf != NULL) { /* we're done, free dither memory */
            farmemfree(ditherbuf);
        ditherbuf = NULL;
        }

   return(status);
}

static void close_file()
{
   fclose(fpin);
   fpin = NULL;
}

/* routine for MIGS that generates partial output lines */

static int out_line_migs(BYTE *pixels, int linelen)
{
int row, startcol, stopcol;

row = gifview_image_top + rowcount;
startcol = gifview_image_left;
stopcol = startcol+linelen-1;
put_line(row, startcol, stopcol, pixels);
rowcount++;

return(0); /* Bert, what is supposed to be returned? JCO */
}


static int out_line_dither(BYTE *pixels, int linelen)
{
    int i,nexterr,brt,err;
        if(ditherbuf == NULL)
        ditherbuf = (char far *)farmemalloc(linelen+1);
        far_memset( ditherbuf, 0, linelen+1);

    nexterr = (rand15()&0x1f)-16;
    for (i=0;i<linelen;i++) {
        brt = (dacbox[pixels[i]][0]*5+dacbox[pixels[i]][1]*9 +
            dacbox[pixels[i]][2]*2)>>4; /* brightness from 0 to 63 */
        brt += nexterr;
        if (brt>32) {
            pixels[i] = 1;
            err = brt-63;
        } else {
            pixels[i] = 0;
            err = brt;
        }
        nexterr = ditherbuf[i+1]+err/3;
        ditherbuf[i] = (char)(err/3);
        ditherbuf[i+1] = (char)(err/3);
    }
    return out_line(pixels, linelen);
}
