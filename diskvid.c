/*
	"Disk-Video" (and RAM-Video and Expanded-Memory Video) routines
*/

#include <stdio.h>
#include <dos.h>
#include "fractint.h"

extern	unsigned char diskline[2049];
extern	int xdots, ydots, colors;
extern	int diskisactive, diskvideo;
extern	int diskprintfs;		/* 0 = supress diskprintfs */

static unsigned int pixelsperbyte, pixelshift, pixeloffset;
static unsigned int pixeloffsetmask;
static unsigned int pixelbitshift[8], pixelmask[8], notpixelmask[8];

static int bytesperline, oldy, linehaschanged, diskisgood;
static FILE *fp;

static unsigned char huge *memoryvideo;
static unsigned char far  *expmemoryvideo;
static long memorypointer;
static int exppages, oldexppage;
static unsigned int emmhandle;

startdisk()
{
unsigned int i;
long longi;

if (!diskisactive) return(0);

pixelsperbyte = 8;			/* calculate pixel/bits stuff */
pixelshift    = 3;
for (i = 0; i < pixelsperbyte; i++) {
	pixelbitshift[i] = i;
	pixelmask[i] = 1 << pixelbitshift[i];
	}
if (colors >  2) {
	pixelsperbyte = 4;
	pixelshift    = 2;
	for (i = 0; i < pixelsperbyte; i++) {
		pixelbitshift[i] = 2*i;
		pixelmask[i] = 3 << pixelbitshift[i];
		}
	}
if (colors >  4) {
	pixelsperbyte = 2;
	pixelshift    = 1;
	for (i = 0; i < pixelsperbyte; i++) {
		pixelbitshift[i] = 4*i;
		pixelmask[i] = 15 << pixelbitshift[i];
		}
	}
if (colors > 16) {
	pixelsperbyte = 1;
	pixelshift    = 0;
	pixelbitshift[0] = 0;
	pixelmask[0]  = 0xff;
	}
for (i = 0; i < pixelsperbyte; i++)
	notpixelmask[i] = 255 - pixelmask[i] ;
pixeloffsetmask = pixelsperbyte-1;

if (diskprintfs) {
   printf("Clearing the 'screen'...\n\n");
   printf("\n\n'Disk-Video' (using the disk as your 'screen') has been activated.\n\n");
   printf("   Your 'screen' resolution is %d x %d x %d colors\n",
	xdots, ydots, colors);
   }

diskisgood = 0;			/* first claim disk file is no good */
emmhandle = 0;			/* and claim no expanded memory */

memorypointer = xdots;  memorypointer *= ydots;	/* try RAM first */
memorypointer >>= pixelshift;		/* adjust for pixels / byte */
if ((memoryvideo = (unsigned char huge *)farmemalloc(memorypointer)) != NULL) {
	if (diskprintfs) 
		printf("\n(Using your Memory)\n");
	for (longi = 0; longi < memorypointer; longi++)	/* clear the RAM */
		memoryvideo[longi] = 0;
	diskvideo = 0;			/* using regular RAM */
	diskisgood = 1;			/* but the video IS good */
	oldy = -1;			/* there is no current line */
	return(0);			/* and bail out */
	}

memorypointer = xdots; memorypointer *= ydots;		/* try exp memory */
memorypointer >>= pixelshift;		/* adjust for pixels / byte */
exppages = (memorypointer + 16383) >> 14;
if ((expmemoryvideo = emmquery()) != NULL && 
	(emmhandle = emmallocate(exppages)) != 0) {
	if (diskprintfs) 
		printf("\n(Using your Expanded Memory)\n");
	for (oldexppage = 0; oldexppage < exppages; oldexppage++)
		emmclearpage(oldexppage, emmhandle);/* clear the "video" */
	diskvideo = 1;			/* using Expanded RAM */
	diskisgood = 1;			/* the video IS good */
	oldy = -1;			/* there is no current line */
	return(0);			/* and bail out */
	}

if (diskprintfs) 
	printf("\n(Using your Disk Drive)\n");

if ((fp = fopen("FRACTINT.DSK","w+b")) == NULL) { /* else try the disk */
	buzzer(2);
	if (diskprintfs) 
		printf("\n((OOPS - Couldn't create FRACTINT.DSK))");
	diskprintfs = 1;		/* re-enable status msgs */
	return(-1);
	}

bytesperline = (xdots >> pixelshift);		/* length of a "line" */
if (xdots & pixelmask[0] != 0) bytesperline++;	/* check for odd bits */
for (i = 0; i < bytesperline; i++)	/* clear the line to black */
	diskline[i] = 0;
for (i = 0; i < ydots; i++)	/* "clear the screen" (write to the disk) */
	if (fwrite(diskline,1,bytesperline,fp) < bytesperline) {
		buzzer(2);
	if (diskprintfs) 
			printf("\n((OOPS - ran out of disk space))\n");
		fclose(fp);
		remove("FRACTINT.DSK");
		diskprintfs = 1;	/* re-enable status msgs */
		return(-1);
		}

diskvideo = 2;			/* using your disk drive */
diskisgood = 1;			/* now note that the file is intact */
oldy = -1;			/* there is no current line */
linehaschanged = 0;		/* current line has not changed */

return(0);

}

enddisk()
{
unsigned int i;

diskprintfs = 1;			/* re-enable status msgs */

if (memoryvideo != NULL) {		/* RAM video? */
	farmemfree((void far*)memoryvideo);/* then free it */
	memoryvideo = NULL;		/* signal same */
	return(0);			/* we done */
	}

if (emmhandle != 0) {			/* Expanded memory video? */
	emmdeallocate(emmhandle);	/* clear the memory */
	emmhandle = 0;			/* signal same */
	return(0);			/* we done */
	}

if (linehaschanged) diskwriteline();	/* flush the line if need be */
fclose(fp);
remove("FRACTINT.DSK");

}

readdisk(x,y)
int x, y;
{
long start;
unsigned int page, offset;

if (x < 0 || x >= xdots || y < 0 || y >= ydots) return(0);

if (memoryvideo != NULL) {		/* RAM video? */
	if (++oldy == xdots) {			/* need to get a new line? */
		if (diskprintfs) {
			home();			/* show a progress report */
			printf("Reading Line %4d...     ",y);
			}
		oldy = -1;
		}
	start = y;  start = (start * xdots) + x;	/* then read it! */
	pixeloffset = start & pixeloffsetmask;	/* get pixel offset */
        start >>= pixelshift;		/* adjust for pixels / byte */
	return(diskgetcolor(memoryvideo[start]));	/* and return */
	}

if (emmhandle != 0) {				/* expanded memory video? */
	if (oldy != y) {			/* need to get a new line? */
		if (diskprintfs) {
			home();			/* show a progress report */
			printf("Reading Line %4d...     ",y);
		}
		oldy = y;			/* reset oldy */
		}
	start = y; start = (start * xdots) + x;	/* figure out where it is */
	pixeloffset = start & pixeloffsetmask;	/* get pixel offset */
        start >>= pixelshift;		/* adjust for pixels / byte */
	page = start >> 14;  offset = start & 0x3fff;
	if (page != oldexppage) {		/* time to get a new page? */
		oldexppage = page;
		emmgetpage(page,emmhandle);
		}
	return(diskgetcolor(expmemoryvideo[offset]));	/* and return */
	}

if (! diskisgood) return(0);		/* bail out if the open failed */

if (oldy != y) {			/* need to get a new line? */
	if (linehaschanged) 		/* write the old line out, if need be */
		diskwriteline();
	start = bytesperline;  start *= oldy;	/* locate start-of-line */
	fseek(fp,start,SEEK_SET);	/* seek to start of line */
	fread(diskline,1,bytesperline,fp);	/* read the line */
	oldy = y;			/* reset oldy */
	if (diskprintfs) {
		home();				/* show a progress report */
		printf("Reading Line %4d...     ",y);
		}
	}

pixeloffset = x & pixeloffsetmask;	/* get pixel offset */
start = x >> pixelshift;		/* adjust for pixels / byte */

return(diskgetcolor(diskline[start]));	/* return with the "pixel" "color" */
}

writedisk(x,y,color)
int x, y, color;
{
long start;
unsigned int page, offset;

if (x < 0 || x >= xdots || y < 0 || y >= ydots) return(0);

if (memoryvideo != NULL) {		/* RAM video? */
	if (++oldy == xdots) {			/* need to get a new line? */
		if (diskprintfs) {
			home();			/* show a progress report */
			printf("Writing Line %4d...     ",y);
			}
		oldy = -1;
		}
	start = y;  start = (start * xdots) + x;	/* then write it! */
	pixeloffset = start & pixeloffsetmask;	/* get pixel offset */
        start >>= pixelshift;		/* adjust for pixels / byte */
	memoryvideo[start] = diskputcolor(memoryvideo[start], color);
	return(0);			/* and bail out */
	}

if (emmhandle != 0) {				/* expanded memory video? */
	if (oldy != y) {			/* need to get a new line? */
		if (diskprintfs) {
			home();			/* show a progress report */
			printf("Writing Line %4d...     ",y);
			}
		oldy = y;			/* reset oldy */
		}
	start = y; start = (start * xdots) + x;	/* figure out where it is */
	pixeloffset = start & pixeloffsetmask;	/* get pixel offset */
        start >>= pixelshift;		/* adjust for pixels / byte */
	page = start >> 14;  offset = start & 0x3fff;
	if (page != oldexppage) {		/* time to get a new page? */
		oldexppage = page;
		emmgetpage(page, emmhandle);
		}
	expmemoryvideo[offset] = diskputcolor(expmemoryvideo[offset], color);
	return(0);				/* and return */
	}

if (! diskisgood) return(0);		/* bail out if the open failed */

if (oldy != y) {			/* need to get a new line? */
	if (linehaschanged) 		/* write the old line out, if need be */
		diskwriteline();
	start = xdots;  start *= y;	/* locate start-of-line */
	fseek(fp,start,SEEK_SET);	/* seek to start of line */
	fread(diskline,1,bytesperline,fp);	/* read the line */
	oldy = y;			/* reset oldy */
	if (diskprintfs) {
		home();			/* show a progress report */
		printf("Writing Line %4d...     ",y);
		}
	}

pixeloffset = x & pixeloffsetmask;	/* get pixel offset */
start = x >> pixelshift;		/* adjust for pixels / byte */

diskline[start] = diskputcolor(diskline[start], color);	/* "display" the "pixel" */

linehaschanged = 1;			/* note that the line has changed */
return(0);

}

diskwriteline()
{
long start;

if (! linehaschanged) 			/* double-check: bail out if no need */
	return(0);

start = bytesperline;  start *= oldy;	/* locate start-of-line */
fseek(fp,start,SEEK_SET);		/* seek to start of line */
fwrite(diskline,1,bytesperline,fp);		/* write the line */

linehaschanged = 0;			/* indicate line has not changed */
return(0);

}

/* disk-based versions of 'getcolor/putcolor' - includes bit-shifting */

diskgetcolor(unsigned char byte)
{
if (colors > 16) return(byte);

return((byte & pixelmask[pixeloffset]) >> pixelbitshift[pixeloffset]);
}

diskputcolor(unsigned char byte, unsigned int color)
{
if (colors > 16) return(color);

return((byte & notpixelmask[pixeloffset]) |
	((color & pixelmask[0]) << pixelbitshift[pixeloffset]));
}

