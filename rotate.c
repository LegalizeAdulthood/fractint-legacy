/*
	rotate.c - Routines that manipulate the Video DAC on VGA Adapters
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fractint.h"

int getakey(void);
void spindac(int, int);

extern	int	colors;				/* maximum colors available */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	extraseg;		/* used by Save-to-GIF routines */

static int paused;				/* rotate-is-paused flag */

rotate(direction)			/* rotate-the-palette routine	*/
int direction;
{
int  kbdchar, more, last, next, maxreg;
int fkey, length, step, fstep, istep, jstep, oldstep;
int incr, random, fromred, fromblue, fromgreen, tored, toblue, togreen;
int i, changecolor, changedirection;
int oldhelpmode;

static int fsteps[] = {2,4,8,12,16,24,32,40,54,100}; /* (for Fkeys) */

if (dacbox[0][0] == 255 || 		/* ??? no DAC to rotate!	*/
	colors < 16) {			/* strange things happen in 2x modes */
		printf("\007");
		return;
		}
oldhelpmode = helpmode;			/* save the old help mode */
helpmode = HELPCYCLING;			/* new help mode */

paused = 0;				/* not paused			*/
length = 0;				/* no random coloring		*/
step = 1;				/* single-step			*/
fkey = 0;				/* no random coloring		*/
changecolor = -1;			/* no color (rgb) to change	*/
changedirection = 0;			/* no color derection to change */
incr = 999;				/* ready to randomize		*/
srand((unsigned char)time(NULL));	/* randomize things		*/

if (direction == 0) {			/* firing up in paused mode?	*/
	pauserotate();			/* then force a pause		*/
	direction = 1;			/* and set a rotate direction	*/
	}

maxreg = 256;				/* maximum register to rotate */

last = maxreg;				/* last box that was filled	*/
next = 1;				/* next box to be filled	*/
if (direction < 0) {
	last = 1;
	next = maxreg;
	}

oldhelpmode = helpmode;			/* save the old help mode */
helpmode = HELPCYCLING;			/* new help mode */

more = 1;
while (more) {
	while(!keypressed())	{	/* rotate until a key gets hit	*/
		if (fkey > 0) {		/* randomizing is on		*/
			for (istep = 0; istep < step; istep++) {
				jstep = next + (istep * direction);
				if (jstep <=      0) jstep += maxreg-1;
				if (jstep >= maxreg) jstep -= maxreg-1;
				if (++incr > fstep) {	/* time to randomize	*/
					incr = 1;
					fstep = ((fsteps[fkey-1]*
						(rand() >> 8)) >> 6) + 1;
					fromred   = dacbox[last][0];
					fromgreen = dacbox[last][1];
					fromblue  = dacbox[last][2];
					tored     = rand() >> 9;
					togreen   = rand() >> 9;
					toblue    = rand() >> 9;
					}
				dacbox[jstep][0] = fromred   + (((tored   - fromred  )*incr)/fstep);
				dacbox[jstep][1] = fromgreen + (((togreen - fromgreen)*incr)/fstep);
				dacbox[jstep][2] = fromblue  + (((toblue  - fromblue )*incr)/fstep);
				}
			if (step >= 256) step = oldstep;
			}
		spindac(direction, step);
		}
	kbdchar = getakey();
	if (paused &&
		(kbdchar != ' ' && kbdchar != 'c' && kbdchar != 'C' ))
			paused = 0;	/* clear paused condition	*/
	switch (kbdchar) {
		case '+':		/* '+' means rotate forward	*/
		case 1077:		/* RightArrow = rotate fwd	*/
			fkey = 0;
			direction = 1;
			last = maxreg-1;
			next = 1;
			incr = 999;
			break;
		case '-':		/* '-' means rotate backward	*/
		case 1075:		/* LeftArrow = rotate bkwd	*/
			fkey = 0;
			direction = -1;
			last = 1;
			next = maxreg-1;
			incr = 999;
			break;
		case 1072:		/* UpArrow means speed up	*/
			daclearn = 1;
			if (daccount < 255) daccount++;
			break;
		case 1080:		/* DownArrow means slow down	*/
			daclearn = 1;
			if (daccount > 1)   daccount--;
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			step = kbdchar - '0';	/* change step-size */
			break;
		case 1059:		/* F1 - F10:			*/
		case 1060:		/* select a shading factor 	*/
		case 1061:
		case 1062:
		case 1063:
		case 1064:
		case 1065:
		case 1066:
		case 1067:
		case 1068:
			fkey = kbdchar-1058;
			fstep = 1;
			incr = 999;
			break;
		case 13:		/* enter key: randomize all colors */
			fkey = rand()/3277 + 1;
			fstep = 1;
			incr = 999;
			oldstep = step;
			step = 256;
			break;
		case 'r':		/* color changes */
			if (changecolor    == -1) changecolor = 0;
			if (changedirection == 0) changedirection = -1;
		case 'g':		/* color changes */
			if (changecolor    == -1) changecolor = 1;
			if (changedirection == 0) changedirection = -1;
		case 'b':		/* color changes */
			if (changecolor    == -1) changecolor = 2;
			if (changedirection == 0) changedirection = -1;
		case 'R':		/* color changes */
			if (changecolor    == -1) changecolor = 0;
			if (changedirection == 0) changedirection = 1;
		case 'G':		/* color changes */
			if (changecolor    == -1) changecolor = 1;
			if (changedirection == 0) changedirection = 1;
		case 'B':		/* color changes */
			if (changecolor    == -1) changecolor = 2;
			if (changedirection == 0) changedirection = 1;
			for (i = 1; i < 256; i++) {
				dacbox[i][changecolor] += changedirection;
				if (dacbox[i][changecolor] == 64)
					dacbox[i][changecolor] = 63;
				if (dacbox[i][changecolor] == 255)
					dacbox[i][changecolor] = 0;
				}
			changecolor    = -1;	/* clear flags for next time */
			changedirection = 0;
			paused          = 0;	/* clear any pause */
		case ' ':		/* use the spacebar as a "pause" toggle */
		case 'c':		/* for completeness' sake, the 'c' too */
		case 'C':
			pauserotate();	/* pause */
			break;
		default:		/* anything else means bail out */
			more = 0;
			break;
		}
	}

helpmode = oldhelpmode;			/* return to previous help mode */
}

pauserotate()				/* pause-the-rotate routine */
{
int olddaccount;			/* saved dac-count value goes here */

if (paused) {	/* if already paused , just clear */
	paused = 0;
	}
else {					/* else set border, wait for a key */
	olddaccount = daccount;
	daccount = 256;
	dacbox[0][0] = 63;
	dacbox[0][1] = 63;
	dacbox[0][2] = 63;
	spindac(0,1);			/* show white border */
	while (!keypressed());		/* wait for any key */
	dacbox[0][0] = 0;
	dacbox[0][1] = 0;
	dacbox[0][2] = 0;
	spindac(0,1);			/* show black border */
	daccount = olddaccount;
	paused = 1;
	}
}

