/*
        encoder.c - GIF Encoder and associated routines
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#ifndef XFRACT
#include <io.h>
#endif
#include "fractint.h"
#include "fractype.h"
#include "prototyp.h"

/* MCP 10-27-91 */
#ifdef WINFRACT
   extern int OperCancelled;
   void OpenStatusBox(void);
   void UpdateStatusBox(unsigned long Partial, unsigned long Total);
   void CloseStatusBox(void);
#endif

static int inittable(void);
static int _fastcall shftwrite(BYTE *color,int numcolors);
static int _fastcall raster(unsigned int);
static int  _fastcall extend_blk_len(int datalen);
static int _fastcall put_extend_blk(int block_id,int block_len,char far *block_data);
static int  _fastcall store_item_name(char *);
static void _fastcall setup_save_info(struct fractal_info far *save_info);

#ifdef XFRACT
int decode_fractal_info();
#endif

/*
                        Save-To-Disk Routines (GIF)

GIF and 'Graphics Interchange Format' are trademarks (tm) of Compuserve
Incorporated, an H&R Block Company.


The following routines perform the GIF encoding when the 's' key is pressed.
The routines refer to several variables that are declared elsewhere
[colors, xdots, ydots, and 'dacbox'], and rely on external routines to
actually read and write screen pixels [getcolor(x,y) and putcolor(x,y,color)].
(Writing pixels is just stuffed in here as a sort of visual status report,
and has nothing to do with any GIF function.)   They also rely on the
existence of an externally-defined 64K dataspace and they use the routines
'toextra()' and 'cmpextra()' to deal with that dataspace (in the same manner
as 'memcpy()' and 'memcmp()' would).   Otherwise, they perform a generic
GIF-encoder function.

Note that these routines use small string- and hash-tables, and "flush"
the GIF entries whenever the hash-table gets two-thirds full or the string
table gets full.   They also use the GIF encoding technique of limiting the
encoded string length to a specific size, "adding" a string to the hash table
at that point even if a matching string exists ("adding" is in quotes, because
if a matching string exists we can increment the code counter but safely throw
the duplicate string away, saving both string space and a hash table entry).

   This results in relatively good speed and small data space, but at the
expense of compression efficiency (filesize).   These trade-offs could be
adjusted by modifying the #DEFINEd variables below.

Note that the 'strlocn' and 'teststring' routines are declared
to be external just so that they can be defined (and the space re-used)
elsewhere.  The actual declarations are in the assembler code.

*/

#define MAXTEST   100           /* maximum single string length */
#define MAXSTRING 64000L        /* total space reserved for strings */
                                /* maximum number of strings available */
#define MAXENTRY  5003          /* (a prime number is best for hashing) */

#ifdef XFRACT
unsigned int strlocn[10240];
BYTE teststring[MAXTEST];
BYTE block[266];   /* GIF-encoded blocks go here */
#endif

static int numsaves = 0;        /* For adjusting 'save-to-disk' filenames */

static FILE *out;
static int last_colorbar;
static int save16bit;
static int outcolor1s, outcolor2s;

static int lentest, lastentry, numentries, numrealentries;
static unsigned int nextentry;
static int clearcode, endcode;
static unsigned int hashcode;

static BYTE blockcount;
static int startbits, codebits, bytecount, bitcount;

static BYTE paletteBW[] = {                     /* B&W palette */
          0,  0,  0, 63, 63, 63,
        };
static BYTE paletteCGA[] = {                    /* 4-color (CGA) palette  */
          0,  0,  0, 21, 63, 63, 63, 21, 63, 63, 63, 63,
        };
static BYTE paletteEGA[] = {                    /* 16-color (EGA/CGA) pal */
          0,  0,  0,  0,  0, 42,  0, 42,  0,  0, 42, 42,
         42,  0,  0, 42,  0, 42, 42, 21,  0, 42, 42, 42,
         21, 21, 21, 21, 21, 63, 21, 63, 21, 21, 63, 63,
         63, 21, 21, 63, 21, 63, 63, 63, 21, 63, 63, 63,
        };

int savetodisk(char *filename)  /* save-to-disk routine */
   {
   char tmpmsg[41]; /* before openfile in case of overrun */
   char openfile[80], openfiletype[10];
   char tmpfile[80];
    char *period;
   int newfile;
   int i, j, outcolor1, outcolor2, interrupted;
restart:

   save16bit = disk16bit;
   if (gif87a_flag) /* not storing non-standard fractal info */
      save16bit = 0;

   strcpy(openfile,filename);           /* decode and open the filename */
   strcpy(openfiletype,DEFAULTFRACTALTYPE);/* determine the file extension */
   if (save16bit)
      strcpy(openfiletype,".pot");
#if 0
   /* this logic fails if directory name hhas period */
   for (i = 0; i < (int)strlen(openfile); i++)
      if (openfile[i] == '.') {
         strcpy(openfiletype,&openfile[i]);
         openfile[i] = 0;
         }
#endif
   if((period = has_ext(openfile)) != NULL)
   {
      strcpy(openfiletype,period);
      *period = 0;
   }
   if (resave_flag != 1)
      updatesavename(filename); /* for next time */

   strcat(openfile,openfiletype);

   strcpy(tmpfile,openfile);
   if (access(openfile,0) != 0) /* file doesn't exist */
      newfile = 1;
   else { /* file already exists */
      if (overwrite == 0) {
         if (resave_flag == 0)
            goto restart;
         if (started_resaves == 0) { /* first save of a savetime set */
            updatesavename(filename);
            goto restart;
            }
         }
      if (access(openfile,2) != 0) {
         sprintf(tmpmsg,s_cantwrite,openfile);
         stopmsg(0,tmpmsg);
         return -1;
         }
      newfile = 0;
      i = strlen(tmpfile);
      while (--i >= 0 && tmpfile[i] != SLASHC)
         tmpfile[i] = 0;
      strcat(tmpfile,"fractint.tmp");
      }

   started_resaves = (resave_flag == 1) ? 1 : 0;
   if (resave_flag == 2) /* final save of savetime set? */
      resave_flag = 0;

   if ((out=fopen(tmpfile,"wb")) == NULL) {
      sprintf(tmpmsg,s_cantcreate,tmpfile);
      stopmsg(0,tmpmsg);
      return -1;
      }

   if (dotmode == 11) {                 /* disk-video */
      char buf[60];
      sprintf(buf,"Saving %s",openfile);
      dvid_status(1,buf);
      }
#ifdef XFRACT
      else {
      putstring(3,0,0,"Saving to:");
      putstring(4,0,0,openfile);
      putstring(5,0,0,"               ");
      }
#endif

   busy = 1;

   if (debugflag != 200)
      interrupted = encoder();
   else
      interrupted = timer(2,NULL);      /* invoke encoder() via timer */

   busy = 0;

   fclose(out);

   if (interrupted) {
      char buf[200];
      sprintf(buf,"Save of %s interrupted.\nCancel to ",openfile);
      if (newfile)
         strcat(buf,"delete the file,\ncontinue to keep the partial image.");
      else
         strcat(buf,"retain the original file,\ncontinue to replace original with new partial image.");
      interrupted = 1;
      if (stopmsg(2,buf) < 0) {
         interrupted = -1;
         unlink(tmpfile);
         }
      }

   if (newfile == 0 && interrupted >= 0) { /* replace the real file */
      unlink(openfile);         /* success assumed since we checked */
      rename(tmpfile,openfile); /* earlier with access              */
      }

   if (dotmode != 11) {                 /* supress this on disk-video */
      if (active_system == 0) {         /* no bars in Windows version */
         outcolor1 = outcolor1s;
         outcolor2 = outcolor2s;
         for (j = 0; j <= last_colorbar; j++) {
            if ((j & 4) == 0) {
               if (++outcolor1 >= colors) outcolor1 = 0;
               if (++outcolor2 >= colors) outcolor2 = 0;
               }
            for (i = 0; 250*i < xdots; i++) { /* clear vert status bars */
               putcolor(i,j,getcolor(i,j)^outcolor1);
               putcolor(xdots-1-i,j,getcolor(xdots-1-i,j)^outcolor2);
               }
            }
         }
#ifdef XFRACT
         putstring(5,0,0,"Saving done\n");
#endif
      }
   else                                 /* disk-video */
      dvid_status(1,"");

   if (interrupted) {
      static FCODE msg[] = {" *interrupted* save "};
      texttempmsg(msg);
      if (initbatch >= 1) initbatch = 3; /* if batch mode, set error level */
      return -1;
      }
   if (timedsave == 0) {
      buzzer(0);
      if (initbatch == 0) {
         extract_filename(tmpfile,openfile);
         sprintf(tmpmsg," File saved as %s ",tmpfile);
         texttempmsg(tmpmsg);
         }
      }
   if(initsavetime < 0)
      goodbye();
   return 0;
   }


int encoder()
{
int i, ydot, xdot, color, outcolor1, outcolor2;
int width;
int rownum, rowlimit;
unsigned int hashentry;
BYTE bitsperpixel, x;
int entrynum;
struct fractal_info save_info;
unsigned int maxstring;

ydot = 0;
/* arbitrary math uses top of extraseg - steal from encoder if necessary */
if(bf_save_len)
{
   maxstring = (unsigned int)(0x10000l-(bf_save_len+2)*22);
   maxstring = min((unsigned int)MAXSTRING,maxstring);
}
else
   maxstring = MAXSTRING;

if(initbatch)                   /* flush any impending keystrokes */
   while(keypressed())
      getakey();

setup_save_info(&save_info);

#ifndef XFRACT
bitsperpixel = 0;                       /* calculate bits / pixel */
for (i = colors; i >= 2; i /= 2 )
        bitsperpixel++;

startbits = bitsperpixel+1;             /* start coding with this many bits */
if (colors == 2)
        startbits++;                    /* B&W Klooge */
#else
    if (colors==2) {
        bitsperpixel = 1;
        startbits = 3;
    } else {
        bitsperpixel = 8;
        startbits = 9;
    }
#endif

clearcode = 1 << (startbits - 1);       /* set clear and end codes */
endcode = clearcode+1;

outcolor1 = 0;                          /* use these colors to show progress */
outcolor2 = 1;                          /* (this has nothing to do with GIF) */
if (colors > 2) {
        outcolor1 = 2;
        outcolor2 = 3;
        }
if (((++numsaves) & 1) == 0) {              /* reverse the colors on alt saves */
        i = outcolor1;
        outcolor1 = outcolor2;
        outcolor2 = i;
        }
outcolor1s = outcolor1;
outcolor2s = outcolor2;

if (gif87a_flag == 1) {
        if (fwrite("GIF87a",6,1,out) != 1) goto oops;  /* old GIF Signature */
} else {
        if (fwrite("GIF89a",6,1,out) != 1) goto oops;  /* new GIF Signature */
}

width = xdots;
rowlimit = ydots;
if (save16bit) {
        /* pot16bit info is stored as:
           file:    double width rows, right side of row is low 8 bits
           diskvid: ydots rows of colors followed by ydots rows of low 8 bits
           decoder: returns (row of color info then row of low 8 bits) * ydots
           */
        rowlimit <<= 1;
        width <<= 1;
        }
if (write2(&width,2,1,out) != 1) goto oops;  /* screen descriptor */
if (write2(&ydots,2,1,out) != 1) goto oops;
x = (BYTE)(128 + ((6-1)<<4) + (bitsperpixel-1)); /* color resolution == 6 bits worth */
if (write1(&x,1,1,out) != 1) goto oops;
if (fputc(0,out) != 0) goto oops;       /* background color */
i = 0;
/** PB, changed to always store pixel aspect ratio, some utilities
        have been reported to like it **/
/**
if ( finalaspectratio < screenaspect-0.01
  || finalaspectratio > screenaspect+0.01) {
 **/
if (viewwindow                              /* less than full screen?  */
  && (viewxdots == 0 || viewydots == 0))    /* and we picked the dots? */
   i = (int)(((double)sydots / (double)sxdots) * 64.0/screenaspect - 14.5);
else /* must risk loss of precision if numbers low */
   i = (int)((((double)ydots / (double)xdots) / finalaspectratio) * 64 - 14.5);
if (i < 1)   i = 1;
if (i > 255) i = 255;
if (gif87a_flag) i = 0;    /* for some decoders which can't handle aspect */
if (fputc(i,out) != i) goto oops;       /* pixel aspect ratio */

#ifndef XFRACT
if (colors == 256) {                    /* write out the 256-color palette */
        if (gotrealdac) {               /* got a DAC - must be a VGA */
                if (!shftwrite((BYTE *)dacbox,colors)) goto oops;
#else
if (colors > 2) {
        if (gotrealdac) {               /* got a DAC - must be a VGA */
                if (!shftwrite((BYTE *)dacbox,256)) goto oops;
#endif
         } else {                       /* uh oh - better fake it */
                for (i = 0; i < 256; i += 16)
                        if (!shftwrite(paletteEGA,16)) goto oops;
                }
        }
if (colors == 2) {                      /* write out the B&W palette */
        if (!shftwrite(paletteBW,colors)) goto oops;
        }
#ifndef XFRACT
if (colors == 4) {                      /* write out the CGA palette */
        if (!shftwrite(paletteCGA,colors))goto oops;
        }
if (colors == 16) {                     /* Either EGA or VGA */
        if (gotrealdac) {
                if (!shftwrite((BYTE *)dacbox,colors))goto oops;
                }
         else   {                       /* no DAC - must be an EGA */
                if (!shftwrite(paletteEGA,colors))goto oops;
                }
        }
#endif

if (fwrite(",",1,1,out) != 1) goto oops;  /* Image Descriptor */
i = 0;
if (write2(&i,2,1,out) != 1) goto oops;
if (write2(&i,2,1,out) != 1) goto oops;
if (write2(&width,2,1,out) != 1) goto oops;
if (write2(&ydots,2,1,out) != 1) goto oops;
if (write1(&i,1,1,out) != 1) goto oops;

bitsperpixel = (BYTE)(startbits - 1);           /* raster data starts here */
if (write1(&bitsperpixel,1,1,out) != 1) goto oops;

codebits = startbits;                   /* start encoding */

if (!raster(9999)) goto oops;           /* initialize the raster routine */

if (!inittable()) goto oops;            /* initialize the LZW tables */

for ( rownum = 0; rownum < ydots; rownum++
#ifdef WINFRACT
      , UpdateStatusBox(rownum, ydots)
#endif
) {  /* scan through the dots */
    for (ydot = rownum; ydot < rowlimit; ydot += ydots) {
        for (xdot = 0; xdot < xdots; xdot++) {
                if (save16bit == 0 || ydot < ydots)
                        color = getcolor(xdot,ydot);
                else
                        color = readdisk(xdot+sxoffs,ydot+syoffs);
                teststring[0] = (BYTE)++lentest;
                teststring[lentest] = (BYTE)color;
                if (lentest == 1) {             /* root entry? */
                        lastentry = color;
                        continue;
                        }
                if (lentest == 2)               /* init   the hash code */
                        hashcode = 301 * (teststring[1]+1);
                hashcode *= (color + lentest);  /* update the hash code */
                hashentry = ++hashcode % MAXENTRY;
                for( i = 0; i < MAXENTRY; i++) {
                        if (++hashentry >= MAXENTRY) hashentry = 0;
                        if (cmpextra(strlocn[hashentry]+sizeof(int),
                                (char *)teststring,lentest+1) == 0)
                                        break;
                        if (strlocn[hashentry] == 0) i = MAXENTRY;
                        }
                /* found an entry and string length isn't too bad */
                if (strlocn[hashentry] != 0 && lentest < MAXTEST-1-sizeof(int)) {
                        fromextra(strlocn[hashentry],(char *)&entrynum,sizeof(int));
                        lastentry = entrynum;
                        continue;
                        }
                if (!raster(lastentry)) goto oops;      /* write entry */
                numentries++;           /* act like you added one, anyway */
                if (strlocn[hashentry] == 0) {  /* add new string, if any */
                        entrynum = numentries+endcode;
                        strlocn[hashentry] = nextentry;
                        toextra(nextentry, (char *)&entrynum,sizeof(int));
                        toextra(nextentry+sizeof(int),
                                (char *)teststring,lentest+1);
                        nextentry += lentest+1+sizeof(int);
                        numrealentries++;
                        }
                teststring[0] = 1;              /* reset current entry */
                teststring[1] = (BYTE)color;
                lentest = 1;
                lastentry = color;

                if ((numentries+endcode) == (1<<codebits))
                        codebits++;              /* use longer encoding */

                if ( numentries + endcode > 4093 ||     /* out of room? */
                        numrealentries > (MAXENTRY*2)/3 ||
                        nextentry > maxstring-MAXTEST-1-2*sizeof(int)) {
                        if (!raster(lastentry)) goto oops;      /* flush & restart */
                        if (!inittable()) goto oops;
                        }
                }
        if (dotmode != 11                       /* supress this on disk-video */
            && active_system == 0               /* and in Windows version     */
            && ydot == rownum) {
                if ((ydot & 4) == 0) {
                        if (++outcolor1 >= colors) outcolor1 = 0;
                        if (++outcolor2 >= colors) outcolor2 = 0;
                        }
                for (i = 0; 250*i < xdots; i++) {       /* display vert status bars */
                                                        /*   (this is NOT GIF-related)  */
                        /* PB Changed following code to xor color, so that
                           image can be restored at end and resumed
                           putcolor(      i,ydot,outcolor1);
                           putcolor(xdots-1-i,ydot,outcolor2);
                        */
                        putcolor(i,ydot,getcolor(i,ydot)^outcolor1);
                        putcolor(xdots-1-i,ydot,getcolor(xdots-1-i,ydot)^outcolor2);
                        }
                last_colorbar = ydot;
                }
#ifdef WINFRACT
        keypressed();
        if (OperCancelled)
#else
        if (keypressed())                     /* keyboard hit - bail out */
#endif
                ydot = rownum = 9999;
        }
    }

if (!raster(lastentry)) goto oops;      /* tidy up - dump the last code */

if (!raster(endcode)) goto oops;        /* finish the map */

if (fputc(0,out) != 0) goto oops;       /* raster data ends here */

if (gif87a_flag == 0) { /* store non-standard fractal info */
        /* loadfile.c has notes about extension block structure */
        if (ydot >= 9999)
                save_info.calc_status = 0; /* partial save is not resumable */
        save_info.tot_extend_len = 0;
        if (resume_info != NULL && save_info.calc_status == 2) {
                /* resume info block, 002 */
                save_info.tot_extend_len += extend_blk_len(resume_len);
                if (!put_extend_blk(2,resume_len,resume_info))goto oops;
                }
        if (save_info.fractal_type == FORMULA || save_info.fractal_type == FFORMULA)
                save_info.tot_extend_len += store_item_name(FormName);
        if (save_info.fractal_type == LSYSTEM)
                save_info.tot_extend_len += store_item_name(LName);
        if (save_info.fractal_type == IFS || save_info.fractal_type == IFS3D)
                save_info.tot_extend_len += store_item_name(IFSName);
        if (display3d <= 0 && rangeslen) {
                /* ranges block, 004 */
                save_info.tot_extend_len += extend_blk_len(rangeslen*2);
#ifdef XFRACT
                fix_ranges(ranges,rangeslen,0);
                put_extend_blk(4,rangeslen*2,(char far *)ranges);
                fix_ranges(ranges,rangeslen,0);
#else
                if (!put_extend_blk(4,rangeslen*2,(char far *)ranges))
                        goto oops;
#endif
                }
        /* Extended parameters block 005 */
        if(bf_math)
        {
           save_info.tot_extend_len += extend_blk_len(22*(bflength+2));
           /* note: this assumes variables allocated in order starting with
              bfxmin in init_bf2() in BIGNUM.C */
           if (!put_extend_blk(5,22*(bflength+2),(char far *)bfxmin))
              goto oops;
        }

        /* main and last block, 001 */
        save_info.tot_extend_len += extend_blk_len(FRACTAL_INFO_SIZE);
#ifdef XFRACT
        decode_fractal_info(&save_info,0);
#endif
        if (!put_extend_blk(1,FRACTAL_INFO_SIZE,(char far *)&save_info)) {
            goto oops;
        }
        }

if (fwrite(";",1,1,out) != 1) goto oops;          /* GIF Terminator */

return ((ydot < 9999) ? 0 : 1);

oops:
    {
    fflush(out);
    stopmsg(0,"Error Writing to disk (Disk full?)");
    return 1;
    }
}

static int _fastcall shftwrite(BYTE *color,int numcolors)
/* shift IBM colors to GIF */
{
BYTE thiscolor;
int i,j;
for (i = 0; i < numcolors; i++)
        for (j = 0; j < 3; j++) {
                thiscolor = color[3*i+j];
                thiscolor = (BYTE)(thiscolor << 2);
                thiscolor = (BYTE)(thiscolor + (BYTE)(thiscolor >> 6));
                if (fputc(thiscolor,out) != (int)thiscolor) return(0);
                }
return(1);
}

static int inittable()          /* routine to init tables */
{
int i;

if (!raster(clearcode)) return(0);      /* signal that table is initialized */

numentries = 0;                         /* initialize the table */
numrealentries = 0;
nextentry = 1;
lentest = 0;
codebits = startbits;

toextra(0,"\0",1);                      /* clear the hash entries */
for (i = 0; i < MAXENTRY; i++)
        strlocn[i] = 0;

return(1);
}

static int _fastcall raster(unsigned int code)  /* routine to block and output codes */
{
unsigned int icode, i, j;

if (code == 9999) {                     /* special start-up signal */
        bytecount = 0;
        bitcount = 0;
        for (i = 0; i < 266; i++)
                block[i] = 0;
        return(1);
        }

icode = code << bitcount;               /* update the bit string */
block[bytecount  ] |= (icode & 255);
block[bytecount+1] |= ((icode>>8) & 255);
icode = (code>>8) << bitcount;
block[bytecount+2] |= ((icode>>8) & 255);
bitcount += codebits;
while (bitcount >= 8) {                 /* locate next starting point */
        bitcount -= 8;
        bytecount++;
        }

if (bytecount > 250 || code == (unsigned int)endcode) { /* time to write a block */
        if (code == (unsigned int)endcode)
                while (bitcount > 0) {          /* if EOF, find the real end */
                        bitcount -= 8;
                        bytecount++;
                        }
        i = bytecount;
        blockcount = (BYTE)i;
        if (write1(&blockcount,1,1,out) != 1) return(0); /* write the block */
        if (fwrite(block,i,1,out) != 1) return(0);
        bytecount = 0;                          /* now re-start the block */
        for (j = 0; j < 5; j++)                 /* (may have leftover bits) */
                block[j] = block[j+i];
        for (j = 5; j < 266; j++)
                block[j] = 0;
        }
return(1);
}


static int _fastcall extend_blk_len(int datalen)
{
   return(datalen + (datalen+254)/255 + 15);
   /*      data   +     1.per.block   + 14 for id + 1 for null at end  */
}

static int _fastcall put_extend_blk(int block_id,int block_len,char far *block_data)
{
   int i,j;
   char header[15];
   strcpy(header,"!\377\013fractint");
   sprintf(&header[11],"%03u",block_id);
   if (fwrite(header,14,1,out) != 1) return(0);
   i = (block_len + 254) / 255;
   while (--i >= 0) {
      block_len -= (j = min(block_len,255));
      if (fputc(j,out) != j) return(0);
      while (--j >= 0)
         fputc(*(block_data++),out);
      }
   if (fputc(0,out) != 0) return(0);
   return(1);
}

static int _fastcall store_item_name(char *nameptr)
{
   struct formula_info fsave_info;
   int i;
   for (i = 0; i < 40; i++)
      fsave_info.form_name[i] = 0; /* initialize string */
   strcpy(fsave_info.form_name,nameptr);
   if(fractype == FORMULA || fractype == FFORMULA) {
     fsave_info.uses_p1 = (short)uses_p1;
     fsave_info.uses_p2 = (short)uses_p2;
     fsave_info.uses_p3 = (short)uses_p3;
   } else {
     fsave_info.uses_p1 = 0;
     fsave_info.uses_p2 = 0;
     fsave_info.uses_p3 = 0;
   }
   for (i = 0; i < sizeof(fsave_info.future)/sizeof(short); i++)
      fsave_info.future[i] = 0;
   /* formula/lsys/ifs info block, 003 */
   put_extend_blk(3,sizeof(fsave_info),(char far *)&fsave_info);
   return(extend_blk_len(sizeof(fsave_info)));
}

static void _fastcall setup_save_info(struct fractal_info far *save_info)
{
   int i;
   if(fractype != FORMULA && fractype != FFORMULA)
      maxfn = 0;
   /* set save parameters in save structure */
   far_strcpy(save_info->info_id, INFO_ID);
   save_info->version         = 12; /* file version, independent of system */
           /* increment this EVERY time the fractal_info structure changes */

   if(maxit <= SHRT_MAX)
     save_info->iterationsold      = (short)maxit;
   else
     save_info->iterationsold      = (short)SHRT_MAX;

   save_info->fractal_type    = (short)fractype;
   save_info->xmin            = xxmin;
   save_info->xmax            = xxmax;
   save_info->ymin            = yymin;
   save_info->ymax            = yymax;
   save_info->creal           = param[0];
   save_info->cimag           = param[1];
   save_info->videomodeax     = (short)videoentry.videomodeax;
   save_info->videomodebx     = (short)videoentry.videomodebx;
   save_info->videomodecx     = (short)videoentry.videomodecx;
   save_info->videomodedx     = (short)videoentry.videomodedx;
   save_info->dotmode         = (short)(videoentry.dotmode % 100);
   save_info->xdots           = (short)videoentry.xdots;
   save_info->ydots           = (short)videoentry.ydots;
   save_info->colors          = (short)videoentry.colors;
   save_info->parm3           = 0; /* pre version==7 fields */
   save_info->parm4           = 0;
   save_info->dparm3          = param[2];
   save_info->dparm4          = param[3];
   save_info->dparm5          = param[4];
   save_info->dparm6          = param[5];
   save_info->dparm7          = param[6];
   save_info->dparm8          = param[7];
   save_info->dparm9          = param[8];
   save_info->dparm10         = param[9];
   save_info->fillcolor       = (short)fillcolor;
   save_info->potential[0]    = (float)potparam[0];
   save_info->potential[1]    = (float)potparam[1];
   save_info->potential[2]    = (float)potparam[2];
   save_info->rflag           = (short)rflag;
   save_info->rseed           = (short)rseed;
   save_info->inside          = (short)inside;
   if(LogFlag <= SHRT_MAX)
     save_info->logmapold     = (short)LogFlag;
   else
     save_info->logmapold     = (short)SHRT_MAX;
   save_info->invert[0]       = (float)inversion[0];
   save_info->invert[1]       = (float)inversion[1];
   save_info->invert[2]       = (float)inversion[2];
   save_info->decomp[0]       = (short)decomp[0];
   save_info->biomorph        = (short)usr_biomorph;
   save_info->symmetry        = (short)forcesymmetry;
   for (i = 0; i < 16; i++)
      save_info->init3d[i] = (short)init3d[i];
   save_info->previewfactor   = (short)previewfactor;
   save_info->xtrans          = (short)xtrans;
   save_info->ytrans          = (short)ytrans;
   save_info->red_crop_left   = (short)red_crop_left;
   save_info->red_crop_right  = (short)red_crop_right;
   save_info->blue_crop_left  = (short)blue_crop_left;
   save_info->blue_crop_right = (short)blue_crop_right;
   save_info->red_bright      = (short)red_bright;
   save_info->blue_bright     = (short)blue_bright;
   save_info->xadjust         = (short)xadjust;
   save_info->yadjust         = (short)yadjust;
   save_info->eyeseparation   = (short)eyeseparation;
   save_info->glassestype     = (short)glassestype;
   save_info->outside         = (short)outside;
   save_info->x3rd            = xx3rd;
   save_info->y3rd            = yy3rd;
   save_info->calc_status     = (short)calc_status;
   save_info->stdcalcmode     = (char)((three_pass&&stdcalcmode=='3')?127:stdcalcmode);
   if (distest <= 32000)
     save_info->distestold    = (short)distest;
   else
     save_info->distestold    = 32000;
   save_info->floatflag       = floatflag;
   if (bailout >= 4 && bailout <= 32000)
     save_info->bailoutold      = (short)bailout;
   else
     save_info->bailoutold      = 0;

   save_info->calctime        = calctime;
   save_info->trigndx[0]      = trigndx[0];
   save_info->trigndx[1]      = trigndx[1];
   save_info->trigndx[2]      = trigndx[2];
   save_info->trigndx[3]      = trigndx[3];
   save_info->finattract      = (short)finattract;
   save_info->initorbit[0]    = initorbit.x;
   save_info->initorbit[1]    = initorbit.y;
   save_info->useinitorbit    = useinitorbit;
   save_info->periodicity     = (short)periodicitycheck;
   save_info->pot16bit        = (short)disk16bit;
   save_info->faspectratio    = finalaspectratio;
   save_info->system          = (short)save_system;

   if (check_back())
     save_info->release       = (short)min(save_release,release);
   else
     save_info->release       = (short)release;

   save_info->flag3d          = (short)display3d;
   save_info->ambient         = (short)Ambient;
   save_info->randomize       = (short)RANDOMIZE;
   save_info->haze            = (short)haze;
   save_info->transparent[0]  = (short)transparent[0];
   save_info->transparent[1]  = (short)transparent[1];
   save_info->rotate_lo       = (short)rotate_lo;
   save_info->rotate_hi       = (short)rotate_hi;
   save_info->distestwidth    = (short)distestwidth;
   save_info->mxmaxfp         = mxmaxfp;
   save_info->mxminfp         = mxminfp;
   save_info->mymaxfp         = mymaxfp;
   save_info->myminfp         = myminfp;
   save_info->zdots           = (short)zdots;
   save_info->originfp        = originfp;
   save_info->depthfp         = depthfp;
   save_info->heightfp        = heightfp;
   save_info->widthfp         = widthfp;
   save_info->distfp          = distfp;
   save_info->eyesfp          = eyesfp;
   save_info->orbittype       = (short)neworbittype;
   save_info->juli3Dmode      = (short)juli3Dmode;
   save_info->maxfn           = maxfn;
   save_info->inversejulia    = (short)((major_method << 8) + minor_method); /* MVS */
   save_info->bailout         = bailout;
   save_info->bailoutest      = (short)bailoutest;
   save_info->iterations      = maxit;
   save_info->bflength        = (short)bnlength;
   save_info->bf_math         = (short)bf_math;
   save_info->old_demm_colors = (short)old_demm_colors;
   save_info->logmap          = LogFlag;
   save_info->distest         = distest;
   save_info->dinvert[0]       = inversion[0];
   save_info->dinvert[1]       = inversion[1];
   save_info->dinvert[2]       = inversion[2];
   save_info->logcalc          = (short)Log_Fly_Calc;
   save_info->stoppass         = (short)stoppass;
   for (i = 0; i < sizeof(save_info->future)/sizeof(short); i++)
      save_info->future[i] = 0;
}
