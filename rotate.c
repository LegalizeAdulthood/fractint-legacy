/*
	rotate.c - Routines that manipulate the Video DAC on VGA Adapters
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "fractint.h"

extern char temp1[];

extern	int	colors;				/* maximum colors available */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	reallyega;		/* == 0 if it's really an EGA */

static int paused;				/* rotate-is-paused flag */


static unsigned char Red[3]    = {63, 0, 0};	/* for shifted-Fkeys */
static unsigned char Green[3]  = { 0,63, 0};
static unsigned char Blue[3]   = { 0, 0,63};
static unsigned char Black[3]  = { 0, 0, 0};
static unsigned char White[3]  = {63,63,63};
static unsigned char Yellow[3] = {63,63, 0};
static unsigned char Brown[3]  = {31,31, 0};

rotate(direction)			/* rotate-the-palette routine	*/
int direction;
{
int  kbdchar, more, last, next, maxreg;
int fkey, length, step, fstep, istep, jstep, oldstep;
int incr, random, fromred, fromblue, fromgreen, tored, toblue, togreen;
int i, changecolor, changedirection;
int oldhelpmode;

static int fsteps[] = {2,4,8,12,16,24,32,40,54,100}; /* (for Fkeys) */

FILE *dacfile;

if (dacbox[0][0] == 255 || 		/* ??? no DAC to rotate!	*/
	colors < 16) {			/* strange things happen in 2x modes */
		buzzer(2);
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
srand((unsigned)time(NULL));		/* randomize things		*/

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
			if (reallyega) fkey = (fkey+1)>>1; /* limit on EGA */
			fstep = 1;
			incr = 999;
			break;
		case 13:		/* enter key: randomize all colors */
		case 1013:		/* also the Numeric-Keypad Enter */
			fkey = rand()/3277 + 1;
			if (reallyega)			/* limit on EGAs */
				fkey = (fkey+1)>>1;
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
			if (reallyega) break;	/* no sense on real EGAs */
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
		case 'd':		/* load colors from "default.map" */
		case 'D':
			findpath("default.map",temp1);
			dacfile = fopen(temp1,"r");
			if (dacfile == NULL) {
				buzzer(2);
				break;
				}
			ValidateLuts(dacfile);	/* read the palette file */
			fclose(dacfile);
			fkey = 0;	/* disable random generation */
			pauserotate();	/* update palette and pause */
			break;
		case 'a':		/* load colors from "altern.map" */
		case 'A':
			findpath("altern.map",temp1);
			dacfile = fopen(temp1,"r");
			if (dacfile == NULL) {
				buzzer(2);
				break;
				}
			ValidateLuts(dacfile);	/* read the palette file */
			fclose(dacfile);
			fkey = 0;	/* disable random generation */
			pauserotate();	/* update palette and pause */
			break;
		case 'm':		/* load colors from a specified map */
		case 'M':
			{
			char temp2[80];
			setfortext();
			printf("\n\n Please enter your .MAP file name ==> ");
			gets(temp2);
			if (strchr(temp2,'.') == NULL)
				strcat(temp2,".map");
			findpath(temp2,temp1);
			setforgraphics();
			}
			dacfile = fopen(temp1,"r");
			if (dacfile == NULL) {
				buzzer(2);
				break;
				}
			ValidateLuts(dacfile);	/* read the palette file */
			fclose(dacfile);
			fkey = 0;	/* disable random generation */
			pauserotate();	/* update palette and pause */
			break;
		case 's':		/* save the palette */
		case 'S':
			setfortext();
			printf("\n\n Please enter your .MAP file name ==> ");
			gets(temp1);
			if (strchr(temp1,'.') == NULL)
				strcat(temp1,".map");
			setforgraphics();
			dacfile = fopen(temp1,"w");
			if (dacfile == NULL) {
				buzzer(2);
				break;
				}
			fprintf(dacfile,"  0   0   0\n");
			for (i = 1; i < 256; i++) 
				fprintf(dacfile, "%3d %3d %3d\n",
				dacbox[i][0] << 2,
				dacbox[i][1] << 2,
				dacbox[i][2] << 2);
			fclose(dacfile);
			fkey = 0;	/* disable random generation */
			pauserotate();	/* update palette and pause */
			break;
		case 'x':		/* switch to x-hair */
		case 'X':
			if (reallyega) break;	/* no sense on real EGAs */
			{
			int olddaccount;
			olddaccount = daccount;
			daccount = 256;
			dacbox[0][0] = 63;
			dacbox[0][1] = 63;
			dacbox[0][2] = 63;
			spindac(0,1);		/* show white border */
			daccount = olddaccount;
			palettes();		/* bring up crosshairs */
			fkey = 0;		/* disable random generation */
			pauserotate();		/* update palette and pause */
			}
			break;
		default:		/* maybe a new palette, maybe bail-out */
		if (kbdchar < 1084 || kbdchar > 1113) {
			more = 0;	/* time to bail out */
			break;
			}
			if (reallyega) break;	/* no sense on real EGAs */
			fkey = 0;		/* disable random generation */
			if (kbdchar == 1084)
				set_palette(Black, White);
			if (kbdchar == 1085)
				set_palette(Red, Yellow);
			if (kbdchar == 1086)
				set_palette(Blue, Green);
			if (kbdchar == 1087)
				set_palette(Black, Yellow);
			if (kbdchar == 1088)
				set_palette(Black, Red);
			if (kbdchar == 1089)
				set_palette(Black, Blue);
			if (kbdchar == 1090)
				set_palette(Black, Green);
			if (kbdchar == 1091)
				set_palette(Blue, Yellow);
			if (kbdchar == 1092)
				set_palette(Red, Green);
			if (kbdchar == 1093)
				set_palette(Green, White);
			if (kbdchar == 1094)
				set_palette2(Black, White);
			if (kbdchar == 1095)
				set_palette2(Red, Yellow);
			if (kbdchar == 1096)
				set_palette2(Blue, Green);
			if (kbdchar == 1097)
				set_palette2(Black, Yellow);
			if (kbdchar == 1098)
				set_palette2(Black, Red);
			if (kbdchar == 1099)
				set_palette2(Black, Blue);
			if (kbdchar == 1100)
				set_palette2(Black, Green);
			if (kbdchar == 1101)
				set_palette2(Blue, Yellow);
			if (kbdchar == 1102)
				set_palette2(Red, Green);
			if (kbdchar == 1103)
				set_palette2(Green, White);
			if (kbdchar == 1104)
				set_palette3(Blue, Green, Red);
			if (kbdchar == 1105)
				set_palette3(Blue, Yellow, Red);
			if (kbdchar == 1106)
				set_palette3(Red, White, Blue);
			if (kbdchar == 1107)
				set_palette3(Red, Yellow, White);
			if (kbdchar == 1108)
				set_palette3(Black, Brown, Yellow);
			if (kbdchar == 1109)
				set_palette3(Blue, Brown, Green);
			if (kbdchar == 1110)
				set_palette3(Blue, Green, Green);
			if (kbdchar == 1111)
				set_palette3(Blue, Green, White);
			if (kbdchar == 1112)
				set_palette3(Green, Green, White);
			if (kbdchar == 1113)
				set_palette3(Red, Blue, White);
			pauserotate();	/* update palette and pause */
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

set_palette(start, finish)
unsigned char start[3], finish[3];
{
   int i, j;
   for(i=1;i<=255;i++)			/* fill the palette	*/
      for (j = 0; j < 3; j++)
         dacbox[i][j] = (i*start[j] + (256-i)*finish[j])/255;
}

set_palette2(start, finish)
unsigned char start[3], finish[3];
{
   int i, j;
   for(i=1;i<=128;i++) 
      for (j = 0; j < 3; j++) {
         dacbox[i][j]     = (i*finish[j] + (128-i)*start[j] )/128;
         dacbox[i+127][j] = (i*start[j]  + (128-i)*finish[j])/128;
      }
}

set_palette3(start, middle, finish)
unsigned char start[3], middle[3], finish[3];
{
   int i, j;
   for(i=1;i<=85;i++) 
      for (j = 0; j < 3; j++) {
         dacbox[i][j]     = (i*middle[j] + (86-i)*start[j] )/85;
         dacbox[i+85][j]  = (i*finish[j] + (86-i)*middle[j])/85;
         dacbox[i+170][j] = (i*start[j]  + (86-i)*finish[j])/85;
      }
}


extern	int	xdots, ydots;			/* # of dots on the screen  */

static int crosshair_active, crosshair_cursor;
static int crosshair_row, crosshair_col, crosshair_color, crosshair_newcolor;
static int old_crosshair_row, old_crosshair_col;
static unsigned char crosshair_colors[2][10], crosshair_oldcolor[3];

extern int	lookatmouse;		/* used to activate non-button mouse movement */

palettes()				/* adjust-the-palette routine	*/
{
int  kbdchar, more;
int i, j, k, changecolor, changedirection;
int oldhelpmode;
int olddaccount;

if (dacbox[0][0] == 255 || 		/* ??? no DAC to rotate! */
	reallyega ||			/* true VGAs only, please */
	colors < 16) {			/* strange things happen in 2x modes */
		buzzer(2);
		return;
		}

oldhelpmode = helpmode;			/* save the old help mode */
helpmode = HELPXHAIR;			/* new help mode */

olddaccount = daccount;			/* update the DAC ASAP */
daccount = 256;

crosshair_row = xdots / 2;		/* start the cursor in the */
crosshair_col = ydots / 2;		/* middle of the screen */
crosshair_active = 0;			/* no crosshair active */
crosshair_cursor = 0;			/* black crosshair cursor */
cross_hair(1);				/* draw the crosshair */

lookatmouse = 1;			/* activate the full mouse-checking */

more = 1;
while (more) {
	while(!keypressed());		/* wait until a key gets hit	*/
	kbdchar = getakey();
	switch (kbdchar) {
		case '+':		/* '+' 	*/
			if (++crosshair_newcolor == 256)
				crosshair_newcolor = 1;
			for (k = 0; k < 3; k++)
				dacbox[crosshair_color][k] =
					 dacbox[crosshair_newcolor][k];
			if (crosshair_color == crosshair_newcolor)
				for (k = 0; k < 3; k++)
					dacbox[crosshair_color][k] =
						crosshair_oldcolor[k];
			spindac(0,1);
			break;
		case '-':		/* '-' 	*/
			if (--crosshair_newcolor == 0)
				crosshair_newcolor = 255;
			for (k = 0; k < 3; k++)
				dacbox[crosshair_color][k] =
					 dacbox[crosshair_newcolor][k];
			if (crosshair_color == crosshair_newcolor)
				for (k = 0; k < 3; k++)
					dacbox[crosshair_color][k] =
						crosshair_oldcolor[k];
			spindac(0,1);
			break;
		case 1116:		/* Ctrl-RightArrow 	*/
			crosshair_row += 4;
		case 1077:		/* RightArrow 	*/
			crosshair_row++;
			if (crosshair_row >= xdots)
				crosshair_row = xdots-1;
			cross_hair(1);
			break;
		case 1115:		/* Ctrl-LeftArrow */
			crosshair_row -= 4;
		case 1075:		/* LeftArrow 	*/
			crosshair_row--;
			if (crosshair_row < 0)
				crosshair_row = 0;
			cross_hair(1);
			break;
		case 1141:		/* Ctrl-UpArrow 	*/
			crosshair_col -= 4;
		case 1072:		/* UpArrow 	*/
			crosshair_col--;
			if (crosshair_col < 0)
				crosshair_col = 0;
			cross_hair(1);
			break;
		case 1145:		/* Ctrl-DownArrow 	*/
			crosshair_col += 4;
		case 1080:		/* DownArrow 	*/
			crosshair_col++;
			if (crosshair_col >= ydots)
				crosshair_col = ydots-1;
			cross_hair(1);
			break;
		case 1073:		/* page up */
			if (++crosshair_cursor >= colors)
				crosshair_cursor = 0;
			cross_hair(1);
			break;
		case 1081:		/* page down */
			if (--crosshair_cursor < 0)
				crosshair_cursor = colors-1;
			cross_hair(1);
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

			i = getcolor(crosshair_row, crosshair_col);
			dacbox[i][changecolor] += changedirection;
			if (dacbox[i][changecolor] == 64)
				dacbox[i][changecolor] = 63;
			if (dacbox[i][changecolor] == 255)
				dacbox[i][changecolor] = 0;
			spindac(0,1);

			changecolor    = -1;	/* clear flags for next time */
			changedirection = 0;
			break;
		case 13:			/* Enter keys */
		case 1013:
			break;
		default:
			more = 0;
			break;
		}
	}
cross_hair(0);				/* remove the cross-hairs */
daccount = olddaccount;			/* replace the DAC count */
lookatmouse = 0;			/* deactivate mouse checking */
helpmode = oldhelpmode;			/* return to previous help mode */
}

cross_hair(onoff)
{
int i, j;

if (crosshair_active) {			/* remove old cross-hair? */
	for (i = 0; i < 10; i++) {
		j = old_crosshair_row - 6 + i;
		if (i >= 5) j += 3;
		if (j > -1 && j < xdots)
			putcolor(j, old_crosshair_col, crosshair_colors[0][i]);
		}
	for (i = 0; i < 10; i++) {
		j = old_crosshair_col - 6 + i;
		if (i >= 5) j += 3;
		if (j > -1 && j < ydots)
			putcolor(old_crosshair_row, j, crosshair_colors[1][i]);
		}
	}

crosshair_active = 0;

if (onoff) {			/* display new old cross-hair? */
	crosshair_active = 1;
	old_crosshair_row = crosshair_row;
	old_crosshair_col = crosshair_col;
	crosshair_color = getcolor(old_crosshair_row, old_crosshair_col);
	for (i = 0; i < 3; i++)
		crosshair_oldcolor[i] = dacbox[crosshair_color][i];
	crosshair_newcolor = crosshair_color;
		for (i = 0; i < 10; i++) {
		j = old_crosshair_row - 6 + i;
		if (i >= 5) j += 3;
		if (j > -1 && j < xdots) {
			crosshair_colors[0][i] = getcolor(j, old_crosshair_col);
			putcolor(j, old_crosshair_col, crosshair_cursor);
			}
		}
	for (i = 0; i < 10; i++) {
		j = old_crosshair_col - 6 + i;
		if (i >= 5) j += 3;
		if (j > -1 && j < ydots) {
			crosshair_colors[1][i] = getcolor(old_crosshair_row, j);
			putcolor(old_crosshair_row, j, crosshair_cursor);
			}
		}
	}
}
