/*
	encoder.c - GIF Encoder and associated routines

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "fractint.h"
#include "fractype.h"

extern int initbatch;
extern struct fractal_info save_info;	/*  for saving data in GIF file */
extern char far *resume_info;		/* pointer to resume info if allocated */
extern int  resume_len; 		/* length of resume info */
extern char FormName[40];		/* formula name */
#define EXTOVHEAD 16

extern	int	xdots, ydots;		/* # of dots on the screen  */
extern	int	colors; 		/* maximum colors available */
extern	int	dotmode;		/* so we can detect disk-video */
extern	int	warn;			/* warnings on/off */
extern	int	resave_flag;		/* resaving after a timed save */
extern	int	timedsave;		/* if doing an auto save */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	reallyega;		/* "reall-an-EGA" flag */
extern int	extraseg;		/* used by Save-to-GIF routines */
extern char potfile[];	    /* potential file name TW 7/19/89 */

extern int gif87a_flag;     /* if 1, supress all GIF extension blocks */

/*
			Save-To-Disk Routines (GIF)

GIF and 'Graphics Interchange Format' are trademarks (tm) of Compuserve
Incorporated, an H&R Block Company.


The following routines perform the GIF encoding when the 's' key is pressed.
The routines refer to several variables that are declared elsewhere
[colors, xdots, ydots, and 'dacbox'], and rely on external routines to
actually read and write screen pixels [getcolor(x,y) and putcolor(x,y,color)].
(Writing pixels is just stuffed in here as a sort of visual status report,
and has nothing to do with any GIF function.)	They also rely on the
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
expense of compression efficiency (filesize).	These trade-offs could be
adjusted by modifying the #DEFINEd variables below.

Note that the 'strlocn' and 'teststring' routines are declared
to be external just so that they can be defined (and the space re-used)
elsewhere.  The actual declarations are in the assembler code.

*/

#define MAXTEST   100		/* maximum single string length */
#define MAXSTRING 64000 	/* total space reserved for strings */
				/* maximum number of strings available */
#define MAXENTRY  5003		/* (a prime number is best for hashing) */

extern unsigned int strlocn[MAXENTRY];
extern unsigned char teststring[MAXTEST];
extern unsigned char block[266];   /* GIF-encoded blocks go here */

static int numsaves = 0;	/* For adjusting 'save-to-disk' filenames */

static FILE *out;

static int lentest, lastentry, numentries, numrealentries;
static unsigned int nextentry;
static int clearcode, endcode;
static unsigned int hashcode;

static unsigned char blockcount;
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
char tmpmsg[41]; /* before openfile in case of overrun */
char openfile[80], openfiletype[10];
char tmpfile[80];
int newfile;
int i, j, ydot, xdot, color, outcolor1, outcolor2, outcolor1s, outcolor2s;
unsigned int hashentry;
unsigned char bitsperpixel, x;
int entrynum;
int last_colorbar;
char far *resume_ptr;

if (extraseg == 0) {			/* not enough memory for this */
	buzzer(2);
	return(0);
	}

restart:

strcpy(openfile,filename);		/* decode and open the filename */
strcpy(openfiletype,DEFAULTFRACTALTYPE);/* determine the file extension */
for (i = 0; i < strlen(openfile); i++)
	if (openfile[i] == '.') {
		strcpy(openfiletype,&openfile[i]);
		openfile[i] = 0;
		}
if (resave_flag == 0 && ++numsaves > 1) {
	updatesavename(openfile);	   /* secondary saves? new filename */
	strncpy(filename, openfile, strlen(openfile));
	}

strcat(openfile,openfiletype);

strcpy(tmpfile,openfile);
if (access(openfile,0) != 0) /* file doesn't exist */
	newfile = 1;
else { /* file already exists */
	if (warn && resave_flag == 0)
		goto restart;
	if (access(openfile,2) != 0) {
		buzzer(2);
		sprintf(tmpmsg," ?? Can't write %s ",openfile);
		texttempmsg(tmpmsg);
		return(0);
		}
	newfile = 0;
	i = strlen(tmpfile);
	while (--i >= 0 && tmpfile[i] != '\\')
		tmpfile[i] = 0;
	strcat(tmpfile,"fractint.tmp");
	}
if ((out=fopen(tmpfile,"wb")) == NULL) {
	buzzer(2);
	sprintf(tmpmsg," ?? Couldn't create %s ",tmpfile);
	texttempmsg(tmpmsg);
	return(0);
	}

if (dotmode == 11) {			/* disk-video */
	movecursor(2,0);
	printf("...saving...");
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
if ((numsaves & 1) == 0) {		/* reverse the colors on alt saves */
	i = outcolor1;
	outcolor1 = outcolor2;
	outcolor2 = i;
	}
outcolor1s = outcolor1;
outcolor2s = outcolor2;

if (gif87a_flag == 1)
	fwrite("GIF87a",1,6,out);              /* old GIF Signature */
else
	fwrite("GIF89a",1,6,out);              /* new GIF Signature */

fwrite(&xdots,2,1,out); 		/* screen descriptor */
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

fwrite(",",1,1,out);                    /* Image Descriptor */
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
		hashcode *= (color + lentest);	/* update the hash code */
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
	if (dotmode != 11) {			/* supress this on disk-video */
		if ((ydot & 4) == 0) {
			if (++outcolor1 >= colors) outcolor1 = 0;
			if (++outcolor2 >= colors) outcolor2 = 0;
			}
		for (i = 0; 250*i < xdots; i++) {	/* display vert status bars */
							/*   (this is NOT GIF-related)	*/
			/* PB Changed following code to xor color, so that
			   image can be restored at end and resumed
			   putcolor(	  i,ydot,outcolor1);
			   putcolor(xdots-1-i,ydot,outcolor2);
			*/
			putcolor(i,ydot,getcolor(i,ydot)^outcolor1);
			putcolor(xdots-1-i,ydot,getcolor(xdots-1-i,ydot)^outcolor2);
			}
		last_colorbar = ydot;
		}
	if (kbhit())				/* keyboard hit - bail out */
		ydot = 9999;
	}

raster(lastentry);			/* tidy up - dump the last code */

raster(endcode);			/* finish the map */

i = 0;					/* raster data ends here */
fwrite(&i,1,1,out);

if (gif87a_flag == 0) { /* store non-standard fractal info */
	if (ydot >= 9999)
		save_info.calc_status = 0; /* partial save is not resumable */
	save_info.tot_extend_len = EXTOVHEAD + sizeof(save_info);
	if (resume_info != NULL && save_info.calc_status == 2) {
		put_extend_hdr(2,resume_len); /* resume info block, 002 */
		resume_ptr = resume_info;
		i = resume_len;
		while (--i >= 0)
			fputc(*(resume_ptr++),out);
		fwrite("\0",1,1,out);
		save_info.tot_extend_len += EXTOVHEAD + resume_len;
		}
	if (save_info.fractal_type == FORMULA || save_info.fractal_type == FFORMULA) {
		put_extend_hdr(3,40); /* additional formula info block, 003 */
		fwrite(FormName,40,1,out);
		fwrite("\0",1,1,out);
		save_info.tot_extend_len += EXTOVHEAD + 40;
		}
	put_extend_hdr(1,sizeof(save_info)); /* main and last block, 001 */
	fwrite(&save_info,sizeof(save_info),1,out);
	fwrite("\0",1,1,out);
	}

fwrite(";",1,1,out);                    /* GIF Terminator */

fclose(out);

if (newfile == 0) { /* now we're safely done, replace the real file */
	if (unlink(openfile) != 0) {
		buzzer(2);
		sprintf(tmpmsg," ?? Can't replace %s ",openfile);
		texttempmsg(tmpmsg);
		return(0);
		}
	if (rename(tmpfile,openfile) != 0) {
		buzzer(2);
		sprintf(tmpmsg," ?? Can't rename %s ",tmpfile);
		texttempmsg(tmpmsg);
		return(0);
		}
	}

if (dotmode != 11) {			/* supress this on disk-video */
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
else {					/* disk-video */
	movecursor(2,0);
	printf("            ");
	}
if (ydot < 9999) {			/* signal normal end */
	if (timedsave == 0) {
		buzzer(0);
		if (initbatch == 0) {
			sprintf(tmpmsg," File saved as %s ",openfile);
			texttempmsg(tmpmsg);
			}
		}
	}
else {
	buzzer(1);
	getakey();			/* read the (interrupt) key-press */
	sprintf(tmpmsg," *INTERRUPTED* File %s ", openfile);
	texttempmsg(tmpmsg);
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
	    /*	fwrite(&thiscolor,1,1,out); */
		fputc(thiscolor,out);
		}
}

inittable()				/* routine to init tables */
{
int i;

raster(clearcode);			/* signal that table is initialized */

numentries = 0; 			/* initialize the table */
numrealentries = 0;
nextentry = 1;
lentest = 0;
codebits = startbits;

toextra(0,"\0",1);                      /* clear the hash entries */
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
	return(0);
	}

icode = code << bitcount;		/* update the bit string */
block[bytecount  ] |= (icode & 255);
block[bytecount+1] |= ((icode>>8) & 255);
icode = (code>>8) << bitcount;
block[bytecount+2] |= ((icode>>8) & 255);
bitcount += codebits;
while (bitcount >= 8) { 		/* locate next starting point */
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
	for (j = 0; j < 5; j++) 		/* (may have leftover bits) */
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

  while(hold >= name && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
    hold--;

  hold++;					     /* recover first digit */

  while (*hold == '0')                                /* skip leading zeros */
    hold++;

  save = hold;

  while (*save) 				     /* check for all nines */
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

put_extend_hdr(int block_id,int block_len) /* for .fra extension blocks */
{
  char header[15];
  strcpy(header,"!\377\013fractint");
  sprintf(&header[11],"%03u",block_id);
  fwrite(header,14,1,out);
  fwrite(&block_len,1,1,out);
}

