
/*
	encoder.c - GIF Encoder and associated routines

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fractint.h"

extern struct fractal_info save_info;	/*  for saving data in GIF file */

extern	int	oktoprint;		/* 0 if printf() won't work */
extern	int	xdots, ydots;		/* # of dots on the screen  */
extern	int	colors;			/* maximum colors available */
extern	int	dotmode;		/* so we can detect disk-video */
extern	int	warn;			/* warnings on/off */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	reallyega;		/* "reall-an-EGA" flag */
extern int	extraseg;		/* used by Save-to-GIF routines */
extern char potfile[];      /* potential file name TW 7/19/89 */

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

#define MAXTEST   100		/* maximum single string length */
#define MAXSTRING 64000		/* total space reserved for strings */
				/* maximum number of strings available */
#define MAXENTRY  5003		/* (a prime number is best for hashing) */

extern int strlocn[MAXENTRY];
extern unsigned char teststring[MAXTEST];

static int numsaves = 0;	/* For adjusting 'save-to-disk' filenames */

static FILE *out;

static int lentest, lastentry, numentries, numrealentries, nextentry;
static int clearcode, endcode;
static unsigned int hashcode;

static unsigned char blockcount, block[266];
static int startbits, codebits, bytecount, bitcount;

static char paletteBW[] = {			/* B&W palette */
	  0,  0,  0, 63, 63, 63,
	};
static char paletteCGA[] = {			/* 4-color (CGA) palette  */
	  0,  0,  0, 21, 63, 63, 63, 21, 63, 63, 63, 63,
	};
static char paletteEGA[] = {			/* 16-color (EGA/CGA) pal */
	  0,  0,  0,  0,  0, 42,  0, 42,  0,  0, 42, 42,
	 42,  0,  0, 42,  0, 42, 42, 21,  0, 42, 42, 42,
	 21, 21, 21, 21, 21, 63, 21, 63, 21, 21, 63, 63,
	 63, 21, 21, 63, 21, 63, 63, 63, 21, 63, 63, 63,
	};
int paletteVGA[] = {			/* VGA palette - to DAC registers */
	0, 1, 2, 3, 4, 5,20, 7,56,57,58,59,60,61,62,63,
	};

savetodisk(filename)			/* save-to-disk routine */
char *filename;
{
char openfile[80], openfiletype[10];
int i, j, ydot, xdot, color, outcolor1, outcolor2;
unsigned int hashentry;
unsigned char bitsperpixel, x;
int entrynum;

if (extraseg == 0) {			/* not enough memory for this */
	buzzer(2);
	return;
	}

restart:

strcpy(openfile,filename);		/* decode and open the filename */
strcpy(openfiletype,DEFAULTFRACTALTYPE);/* determine the file extension */
for (i = 0; i < strlen(openfile); i++)
	if (openfile[i] == '.') {
		strcpy(openfiletype,&openfile[i]);
		openfile[i] = 0;
		}
if (++numsaves > 1) {
	updatesavename(openfile);	   /* secondary saves? new filename */
	strncpy(filename, openfile, strlen(openfile));
	}

strcat(openfile,openfiletype);
if (warn && (out=fopen(openfile,"r"))) {
	fclose(out);
	goto restart;
	}
if ((out=fopen(openfile,"wb")) == NULL) {
	if (oktoprint)
		printf(" ?? Couldn't create file %s \n",openfile);
	return;
	}

bitsperpixel = 0;			/* calculate bits / pixel */
for (i = colors; i >= 2; i /= 2 )
	bitsperpixel++;

startbits = bitsperpixel+1;		/* start coding with this many bits */
if (colors == 2)
	startbits++;			/* B&W Klooge */

clearcode = 1 << (startbits - 1);	/* set clear and end codes */
endcode = clearcode+1;

outcolor1 = 0;				/* use these colors to show progress */
outcolor2 = 1;				/* (this has nothing to do with GIF) */
if (colors > 2) {
	outcolor1 = 2;
	outcolor2 = 3;
	}
if (numsaves && 1 == 0) {		/* reverse the colors on alt saves */
	i = outcolor1;
	outcolor1 = outcolor2;
	outcolor2 = i;
	}

fwrite("GIF87a",1,6,out);		/* GIF Signature */

fwrite(&xdots,2,1,out);			/* screen descriptor */
fwrite(&ydots,2,1,out);
x = 128 + ((6-1)<<4) + (bitsperpixel-1); /* color resolution == 6 bits worth */
fwrite(&x,1,1,out);
i = 0;
fwrite(&i,1,1,out);
fwrite(&i,1,1,out);

if (colors == 256) {			/* write out the 256-color palette */
	if (dacbox[0][0] != 255)	/* got a DAC - must be a VGA */
		shftwrite(dacbox,colors);
	else				/* uh oh - better fake it */
		for (i = 0; i < 256; i += 16)
			shftwrite(paletteEGA,16);
	}
if (colors == 2)			/* write out the B&W palette */
	shftwrite(paletteBW,colors);
if (colors == 4)			/* write out the CGA palette */
	shftwrite(paletteCGA,colors);
if (colors == 16)			/* Either EGA or VGA */
	if (dacbox[0][0] != 255) {	/* got a  DAC - must be a VGA */
		if (reallyega)		/* well, maybe really an EGA */
			shftwrite(dacbox,colors);
		else
			for (i = 0; i < colors; i++)
				shftwrite(dacbox[paletteVGA[i]],1);
		}
		else			/* no DAC - must be an EGA */
			shftwrite(paletteEGA,colors);

fwrite(",",1,1,out);			/* Image Descriptor */
i = 0;
fwrite(&i,2,1,out);
fwrite(&i,2,1,out);
fwrite(&xdots,2,1,out);
fwrite(&ydots,2,1,out);
i = 0;
fwrite(&i,1,1,out);

bitsperpixel = startbits - 1;		/* raster data starts here */
fwrite(&bitsperpixel,1,1,out);

codebits = startbits;			/* start encoding */

raster(9999);				/* initialize the raster routine */

inittable();				/* initialize the LZW tables */

for (ydot = 0; ydot < ydots; ydot++) {	/* scan through the dots */
	for (xdot = 0; xdot < xdots; xdot++) {
		color = getcolor(xdot,ydot);	/* get the next dot */
		teststring[0] = ++lentest;
		teststring[lentest] = color;
		if (lentest == 1) {		/* root entry? */
			lastentry = color;
			continue;
			}
		if (lentest == 2)		/* init   the hash code */
			hashcode = 301 * (teststring[1]+1);
		hashcode *= (color + lentest);  /* update the hash code */
		hashentry = ++hashcode % MAXENTRY;
		for( i = 0; i < MAXENTRY; i++) {
			if (++hashentry >= MAXENTRY) hashentry = 0;
			if (cmpextra(strlocn[hashentry]+2,
				teststring,lentest+1) == 0)
					break;
			if (strlocn[hashentry] == 0) i = MAXENTRY;
			}
		/* found an entry and string length isn't too bad */
		if (strlocn[hashentry] != 0 && lentest < MAXTEST-3) {
			fromextra(strlocn[hashentry],&entrynum,2);
			lastentry = entrynum;
			continue;
			}
		raster(lastentry);			/* write entry */
		numentries++;		/* act like you added one, anyway */
		if (strlocn[hashentry] == 0) {	/* add new string, if any */
			entrynum = numentries+endcode;
			strlocn[hashentry] = nextentry;
			toextra(nextentry, &entrynum,2);
			toextra(nextentry+2,
				teststring,lentest+1);
			nextentry += lentest+3;
			numrealentries++;
			}
		teststring[0] = 1;		/* reset current entry */
		teststring[1] = color;
		lentest = 1;
		lastentry = color;

		if ((numentries+endcode) == (1<<codebits))
			codebits++;		 /* use longer encoding */

		if ( numentries + endcode > 4093 ||	/* out of room? */
		 	numrealentries > (MAXENTRY*2)/3 ||
			nextentry > MAXSTRING-MAXTEST-5) {
			raster(lastentry);		/* flush & restart */
			inittable();
			}
		}
	if (dotmode != 11)			/* supress this on disk-video */
		for (i = 0; 250*i < xdots; i++) {	/* display vert status bars */
			putcolor(      i,ydot,outcolor1);	/*   (this is NOT   */
			putcolor(xdots-1-i,ydot,outcolor2);	/*    GIF-related)  */
			}
	if (kbhit())				/* keyboard hit - bail out */
		ydot = 9999;
	}

raster(lastentry);			/* tidy up - dump the last code */

raster(endcode);			/* finish the map */

i = 0;					/* raster data ends here */
fwrite(&i,1,1,out);

fwrite(";",1,1,out);                    /* GIF Terminator */

if (strcmp(openfiletype,".gif") != 0) {		/* non-standard fractal info */
	fwrite("!",1,1,out);			/* extension block identifier */
	i = 255; fwrite(&i,1,1,out);		/* extension block #255 */
	i =  11; fwrite(&i,1,1,out);		/* size of Identifier Block */
	fwrite("fractint001",11,1,out);		/* Application Identifier */
	i = sizeof(save_info); fwrite(&i,1,1,out); /* size of fractal info */
	fwrite(&save_info,sizeof(save_info),1,out); /* fractal info */
	i =   0; fwrite(&i,1,1,out);		/* block terminator */
	fwrite(";",1,1,out);			/* GIF terminator */
	}
fclose(out);
if (ydot < 9999) {			/* signal normal or interrupted end */
	buzzer(0);
	if (oktoprint) 
		printf(" File saved as %s \n",openfile);
		}
else	{
	buzzer(1);
	if (oktoprint)
		printf(" ** INTERRUPTED ** File %s \n", openfile);
	getakey();			/* read the (interrupt) key-press */
	}
}

shftwrite(color,numcolors)		/* shift IBM colors to GIF format */
unsigned char color[];
int numcolors;
{
unsigned char thiscolor;
int i,j;
for (i = 0; i < numcolors; i++)
	for (j = 0; j < 3; j++) {
		thiscolor = color[3*i+j];
		thiscolor = thiscolor << 2;
		thiscolor += (thiscolor >> 6);
		fwrite(&thiscolor,1,1,out);
		}
}

inittable()				/* routine to init tables */
{
int i;

raster(clearcode);			/* signal that table is initialized */

numentries = 0;				/* initialize the table */
numrealentries = 0;
nextentry = 1;
lentest = 0;
codebits = startbits;

toextra(0,"\0",1);			/* clear the hash entries */
for (i = 0; i < MAXENTRY; i++)
	strlocn[i] = 0;

}

raster(code)				/* routine to block and output codes */
unsigned int code;
{
unsigned int icode, i, j;

if (code == 9999) {			/* special start-up signal */
	bytecount = 0;
	bitcount = 0;
	for (i = 0; i < 266; i++)
		block[i] = 0;
	return;
	}

icode = code << bitcount;		/* update the bit string */
block[bytecount  ] |= (icode & 255);
block[bytecount+1] |= ((icode>>8) & 255);
icode = (code>>8) << bitcount;
block[bytecount+2] |= ((icode>>8) & 255);
bitcount += codebits;
while (bitcount >= 8) {			/* locate next starting point */
	bitcount -= 8;
	bytecount++;
	}

if (bytecount > 250 || code == endcode) {	/* time to write a block */
	if (code == endcode) 
		while (bitcount > 0) {		/* if EOF, find the real end */
			bitcount -= 8;
			bytecount++;
			}
	i = bytecount;
	blockcount = i;
	fwrite(&blockcount,1,1,out);		/* write the block */
	fwrite(block,i,1,out);
	bytecount = 0;				/* now re-start the block */
	for (j = 0; j < 5; j++)			/* (may have leftover bits) */
		block[j] = block[j+i];
	for (j = 5; j < 266; j++)
		block[j] = 0;
	}
}


updatesavename(name)				     /* go to the next name */
char *name;
{
  char *save, *hold = name + strlen(name) - 1;		/* start at the end */
  int i =  0;

  while(isdigit(*hold))					  /* skip backwards */
    hold--;
  
  hold++;					     /* recover first digit */
  
  while (*hold == '0')				      /* skip leading zeros */
    hold++;
  
  save = hold;
  
  while (*save)					     /* check for all nines */
  {
    if (*save != '9')
      break;
    save++;
  }
    
  if (!*save)			   /* if the whole thing is nines then back */
    save = hold - 1;		   /* up one place. Note that this will eat */
				   /* your last letter if you go to far.    */
  else
    save = hold;

  itoa(atoi(hold) + 1, save, 10);		    /* increment the number */
}
  
