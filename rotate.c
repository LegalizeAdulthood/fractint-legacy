/*
    rotate.c - Routines that manipulate the Video DAC on VGA Adapters
    This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fractint.h"
#include "helpdefs.h"

/* routines in this module	*/

void rotate_overlay(void);
void rotate(int);
void save_palette(void);
void load_palette(void);

static void pauserotate(void);
static void set_palette(),set_palette2(),set_palette3();

extern char temp1[];

extern	int	colors; 		/* maximum colors available */

extern unsigned char dacbox[256][3];	/* Video-DAC (filled in by SETVIDEO) */
extern	int	gotrealdac;		/* dacbox valid? */
extern unsigned char olddacbox[256][3];
extern int	daclearn, daccount;	/* used by the color-cyclers */
extern int	reallyega;		/* == 0 if it's really an EGA */
extern int	rotate_lo,rotate_hi;	/* range of colors to cycle */
extern int	colorstate; /* comments in cmdfiles */
extern char	colorfile[];
extern int	dotmode;

static int paused;			/* rotate-is-paused flag */

extern int field_prompt(int options, char *hdg, char *instr, char *fld, int len,
		int (*checkkey)() );

static unsigned char Red[3]    = {63, 0, 0};	/* for shifted-Fkeys */
static unsigned char Green[3]  = { 0,63, 0};
static unsigned char Blue[3]   = { 0, 0,63};
static unsigned char Black[3]  = { 0, 0, 0};
static unsigned char White[3]  = {63,63,63};
static unsigned char Yellow[3] = {63,63, 0};
static unsigned char Brown[3]  = {31,31, 0};

char mapmask[13] = {"*.map"};


void rotate_overlay() { }	/* for restore_active_ovly */

void rotate(int direction)	/* rotate-the-palette routine */
{
int  kbdchar, more, last, next;
int fkey, step, fstep, istep, jstep, oldstep;
int incr, fromred, fromblue, fromgreen, tored, toblue, togreen;
int i, changecolor, changedirection;
int oldhelpmode;
int rotate_max,rotate_size;

static int fsteps[] = {2,4,8,12,16,24,32,40,54,100}; /* (for Fkeys) */

   ENTER_OVLY(OVLY_ROTATE);

   if (gotrealdac == 0			/* ??? no DAC to rotate! */
     || colors < 16) {			/* strange things happen in 2x modes */
      buzzer(2);
      EXIT_OVLY;
      return;
      }

   oldhelpmode = helpmode;		/* save the old help mode	*/
   helpmode = HELPCYCLING;		/* new help mode		*/

   paused = 0;				/* not paused			*/
   fkey = 0;				/* no random coloring		*/
   oldstep = step = 1;			/* single-step			*/
   fstep = 1;
   changecolor = -1;			/* no color (rgb) to change	*/
   changedirection = 0; 		/* no color direction to change */
   incr = 999;				/* ready to randomize		*/
   srand((unsigned)time(NULL)); 	/* randomize things		*/

   if (direction == 0) {		/* firing up in paused mode?	*/
      pauserotate();			/* then force a pause		*/
      direction = 1;			/* and set a rotate direction	*/
      }

   rotate_max = (rotate_hi < colors) ? rotate_hi : colors-1;
   rotate_size = rotate_max - rotate_lo + 1;
   last = rotate_max;			/* last box that was filled	*/
   next = rotate_lo;			/* next box to be filled	*/
   if (direction < 0) {
      last = rotate_lo;
      next = rotate_max;
      }

   more = 1;
   while (more) {
      if (dotmode == 11) {
	 if (!paused)
	    pauserotate();
	 }
      else while(1) { /* rotate until key hit, at least once so step=oldstep ok */
	 if (fkey > 0) {		/* randomizing is on */
	    for (istep = 0; istep < step; istep++) {
	       jstep = next + (istep * direction);
	       while (jstep < rotate_lo)  jstep += rotate_size;
	       while (jstep > rotate_max) jstep -= rotate_size;
	       if (++incr > fstep) {	/* time to randomize */
		  incr = 1;
		  fstep = ((fsteps[fkey-1]* (rand() >> 8)) >> 6) + 1;
		  fromred   = dacbox[last][0];
		  fromgreen = dacbox[last][1];
		  fromblue  = dacbox[last][2];
		  tored     = rand() >> 9;
		  togreen   = rand() >> 9;
		  toblue    = rand() >> 9;
		  }
	       dacbox[jstep][0] = fromred   + (((tored	 - fromred  )*incr)/fstep);
	       dacbox[jstep][1] = fromgreen + (((togreen - fromgreen)*incr)/fstep);
	       dacbox[jstep][2] = fromblue  + (((toblue  - fromblue )*incr)/fstep);
	       }
	    }
	 if (step >= rotate_size) step = oldstep;
	 if (keypressed()) break;
	 spindac(direction, step);
	 }
      kbdchar = getakey();
      if (paused && (kbdchar != ' ' && kbdchar != 'c' && kbdchar != 'C' ))
	 paused = 0;			/* clear paused condition	*/
      switch (kbdchar) {
	 case '+':                      /* '+' means rotate forward     */
	 case 1077:			/* RightArrow = rotate fwd	*/
	    fkey = 0;
	    direction = 1;
	    last = rotate_max;
	    next = rotate_lo;
	    incr = 999;
	    break;
	 case '-':                      /* '-' means rotate backward    */
	 case 1075:			/* LeftArrow = rotate bkwd	*/
	    fkey = 0;
	    direction = -1;
	    last = rotate_lo;
	    next = rotate_max;
	    incr = 999;
	    break;
	 case 1072:			/* UpArrow means speed up	*/
	    daclearn = 1;
	    if (++daccount >= colors) --daccount;
	    break;
	 case 1080:			/* DownArrow means slow down	*/
	    daclearn = 1;
	    if (daccount > 1) daccount--;
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
	    step = kbdchar - '0';   /* change step-size */
	    if (step > rotate_size) step = rotate_size;
	    break;
	 case 1059:			/* F1 - F10:			*/
	 case 1060:			/* select a shading factor	*/
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
	 case 13:			/* enter key: randomize all colors */
	 case 1013:			/* also the Numeric-Keypad Enter */
	    fkey = rand()/3277 + 1;
	    if (reallyega)		/* limit on EGAs */
	       fkey = (fkey+1)>>1;
	    fstep = 1;
	    incr = 999;
	    oldstep = step;
	    step = rotate_size;
	    break;
	 case 'r':                      /* color changes */
	    if (changecolor    == -1) changecolor = 0;
	 case 'g':                      /* color changes */
	    if (changecolor    == -1) changecolor = 1;
	 case 'b':                      /* color changes */
	    if (changecolor    == -1) changecolor = 2;
	    if (changedirection == 0) changedirection = -1;
	 case 'R':                      /* color changes */
	    if (changecolor    == -1) changecolor = 0;
	 case 'G':                      /* color changes */
	    if (changecolor    == -1) changecolor = 1;
	 case 'B':                      /* color changes */
	    if (dotmode == 11) break;
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
	    paused	    = 0;	/* clear any pause */
	 case ' ':                      /* use the spacebar as a "pause" toggle */
	 case 'c':                      /* for completeness' sake, the 'c' too */
	 case 'C':
	    pauserotate();		/* pause */
	    break;
	 case '>':			/* single-step */
	 case '.':
	 case '<':
	 case ',':
	    if (! paused)
	       pauserotate();		/* pause */
	    fkey = 0;
	    if (kbdchar == '>' || kbdchar == '.')
	       spindac(1,1);
	    else
	       spindac(-1,1);
	    break;
	 case 'd':                      /* load colors from "default.map" */
	 case 'D':
	    if (ValidateLuts("default") != 0)
	       break;
	    fkey = 0;			/* disable random generation */
	    pauserotate();		/* update palette and pause */
	    break;
	 case 'a':                      /* load colors from "altern.map" */
	 case 'A':
	    if (ValidateLuts("altern") != 0)
	       break;
	    fkey = 0;			/* disable random generation */
	    pauserotate();		/* update palette and pause */
	    break;
	 case 'l':                      /* load colors from a specified map */
	 case 'L':
	    load_palette();
	    fkey = 0;			/* disable random generation */
	    pauserotate();		/* update palette and pause */
	    break;
	 case 's':                      /* save the palette */
	 case 'S':
	    save_palette();
	    fkey = 0;			/* disable random generation */
	    pauserotate();		/* update palette and pause */
	    break;
	 case 27:			/* escape */
	    more = 0;			/* time to bail out */
	    break;
	 default:			/* maybe a new palette */
	    if (reallyega) break;	/* no sense on real EGAs */
	    fkey = 0;			/* disable random generation */
	    if (kbdchar == 1084) set_palette(Black, White);
	    if (kbdchar == 1085) set_palette(Red, Yellow);
	    if (kbdchar == 1086) set_palette(Blue, Green);
	    if (kbdchar == 1087) set_palette(Black, Yellow);
	    if (kbdchar == 1088) set_palette(Black, Red);
	    if (kbdchar == 1089) set_palette(Black, Blue);
	    if (kbdchar == 1090) set_palette(Black, Green);
	    if (kbdchar == 1091) set_palette(Blue, Yellow);
	    if (kbdchar == 1092) set_palette(Red, Green);
	    if (kbdchar == 1093) set_palette(Green, White);
	    if (kbdchar == 1094) set_palette2(Black, White);
	    if (kbdchar == 1095) set_palette2(Red, Yellow);
	    if (kbdchar == 1096) set_palette2(Blue, Green);
	    if (kbdchar == 1097) set_palette2(Black, Yellow);
	    if (kbdchar == 1098) set_palette2(Black, Red);
	    if (kbdchar == 1099) set_palette2(Black, Blue);
	    if (kbdchar == 1100) set_palette2(Black, Green);
	    if (kbdchar == 1101) set_palette2(Blue, Yellow);
	    if (kbdchar == 1102) set_palette2(Red, Green);
	    if (kbdchar == 1103) set_palette2(Green, White);
	    if (kbdchar == 1104) set_palette3(Blue, Green, Red);
	    if (kbdchar == 1105) set_palette3(Blue, Yellow, Red);
	    if (kbdchar == 1106) set_palette3(Red, White, Blue);
	    if (kbdchar == 1107) set_palette3(Red, Yellow, White);
	    if (kbdchar == 1108) set_palette3(Black, Brown, Yellow);
	    if (kbdchar == 1109) set_palette3(Blue, Brown, Green);
	    if (kbdchar == 1110) set_palette3(Blue, Green, Green);
	    if (kbdchar == 1111) set_palette3(Blue, Green, White);
	    if (kbdchar == 1112) set_palette3(Green, Green, White);
	    if (kbdchar == 1113) set_palette3(Red, Blue, White);
	    pauserotate();  /* update palette and pause */
	    break;
	 }
      }

   helpmode = oldhelpmode;		/* return to previous help mode */
   EXIT_OVLY;
}

static void pauserotate()		/* pause-the-rotate routine */
{
int olddaccount;			/* saved dac-count value goes here */
unsigned char olddac0,olddac1,olddac2;

   if (paused)				/* if already paused , just clear */
      paused = 0;
   else {				/* else set border, wait for a key */
      olddaccount = daccount;
      olddac0 = dacbox[0][0];
      olddac1 = dacbox[0][1];
      olddac2 = dacbox[0][2];
      daccount = 256;
      dacbox[0][0] = 48;
      dacbox[0][1] = 48;
      dacbox[0][2] = 48;
      spindac(0,1);			/* show white border */
      if (dotmode == 11)
	 dvid_status(100," Paused in ""color cycling"" mode ");
      while (!keypressed());		/* wait for any key */
      if (dotmode == 11)
	 dvid_status(0,"");
      dacbox[0][0] = olddac0;
      dacbox[0][1] = olddac1;
      dacbox[0][2] = olddac2;
      spindac(0,1);			/* show black border */
      daccount = olddaccount;
      paused = 1;
      }
}

static void set_palette(start, finish)
unsigned char start[3], finish[3];
{
   int i, j;
   dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
   for(i=1;i<=255;i++)			/* fill the palette	*/
      for (j = 0; j < 3; j++)
	 dacbox[i][j] = (i*start[j] + (256-i)*finish[j])/255;
}

static void set_palette2(start, finish)
unsigned char start[3], finish[3];
{
   int i, j;
   dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
   for(i=1;i<=128;i++)
      for (j = 0; j < 3; j++) {
	 dacbox[i][j]	  = (i*finish[j] + (128-i)*start[j] )/128;
	 dacbox[i+127][j] = (i*start[j]  + (128-i)*finish[j])/128;
      }
}

static void set_palette3(start, middle, finish)
unsigned char start[3], middle[3], finish[3];
{
   int i, j;
   dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
   for(i=1;i<=85;i++)
      for (j = 0; j < 3; j++) {
	 dacbox[i][j]	  = (i*middle[j] + (86-i)*start[j] )/85;
	 dacbox[i+85][j]  = (i*finish[j] + (86-i)*middle[j])/85;
	 dacbox[i+170][j] = (i*start[j]  + (86-i)*finish[j])/85;
      }
}


void save_palette()
{
   FILE *dacfile;
   int i,oldhelpmode;
   oldhelpmode = helpmode;
   stackscreen();
   temp1[0] = 0;
   helpmode = HELPCOLORMAP;
   i = field_prompt(0,"Name of map file to write",NULL,temp1,60,NULL);
   unstackscreen();
   if (i != -1 && temp1[0]) {
      if (strchr(temp1,'.') == NULL)
	 strcat(temp1,".map");
      dacfile = fopen(temp1,"w");
      if (dacfile == NULL)
	 buzzer(2);
      else {
	 for (i = 0; i < colors; i++)
	    fprintf(dacfile, "%3d %3d %3d\n",
		    dacbox[i][0] << 2,
		    dacbox[i][1] << 2,
		    dacbox[i][2] << 2);
	 memcpy(olddacbox,dacbox,256*3);
	 colorstate = 2;
	 strcpy(colorfile,temp1);
	 }
      fclose(dacfile);
      }
   helpmode = oldhelpmode;
}


void load_palette(void)
{
   int i,oldhelpmode;
   char filename[80];
   oldhelpmode = helpmode;
   strcpy(filename,colorfile);
   stackscreen();
   helpmode = HELPCOLORMAP;
   i = getafilename("Select a MAP File",mapmask,filename);
   unstackscreen();
   if (i >= 0)
      if (ValidateLuts(filename) == 0)
	 memcpy(olddacbox,dacbox,256*3);
   helpmode = oldhelpmode;
}


