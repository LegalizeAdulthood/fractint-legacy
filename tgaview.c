
/* Routine to decode Targa 16 bit RGB file
   */

/* 16 bit .tga files were generated for continuous potential "potfile"s
   from version 9.? thru version 14.  Replaced by double row gif type
   file (.pot) in version 15.  Delete this code after a few more revs.
*/

#include <stdio.h>
#include "fractint.h"
#include "targa_lc.h"
#include "port.h"

extern char readname[]; 		/* file name		  */
extern unsigned int boxx[];		/* write-line routines use this */
extern int colors;
extern int rowcount;

static FILE *fptarga = NULL;		/* FILE pointer 	  */
extern unsigned int height;		/* image height 	  */
extern int (*outln)();

/* Main entry decoder */
tgaview()
{
   int i;
   int cs;
   unsigned int width;
   struct fractal_info info;
   FILE *t16_open();

   if((fptarga = t16_open(readname, &width, &height, &cs, (int *)&info))==NULL)
      return(-1);

   rowcount = 0;
   for (i=0; i<height; ++i)
   {
       t16_getline(fptarga, width, (U16 *)boxx);
       if ((*outln)(boxx,width))
       {
	  fclose(fptarga);
	  fptarga = NULL;
	  return(-1);
       }
       if (keypressed())
       {
	  fclose(fptarga);
	  fptarga = NULL;
	  return(-1);
       }
   }
   fclose(fptarga);
   fptarga = NULL;
   return(0);
}
/* Outline function for 16 bit data with 8 bit fudge */
outlin16(unsigned int *buffer,int linelen)
{
    extern int rowcount;
    int i;
    for(i=0;i<linelen;i++)
       putcolor(i,rowcount,buffer[i]>>8);
    rowcount++;
    return(0);
}
