/*
	FRACTINT Help routines
*/

#include "fractint.h"

helptitle()
{
home();					/* home the cursor		*/
setclear();				/* clear the screen		*/

printf("FRACTINT                                Version 7.0 \n");

}

help()
{
int mode, key;

mode = helpmode;

if (mode == HELPAUTHORS) {
	helpauthors();
	printf("\nPlease press one of the Function Keys to select a video mode and begin an image\n");
	printf("(or press the 'h' key now or at any other time for help) ");
	return(0);
	}

setfortext();
while (mode != HELPEXIT) {
	helptitle();
	if (mode == HELPMENU) {
		while (mode == HELPMENU) {
			helpmenu();
			key = getakey();
			switch (key) {
				case '1':
					mode = HELPMAIN;
					break;
				case '2':
					mode = HELPCYCLING;
					break;
				case '3':
					mode = HELPMOUSE;
					break;
				case '4':
					mode = HELPCMDLINE;
					break;
				case '5':
					mode = HELPFRACTALS;
					break;
				case '6':
					mode = HELPVIDEO;
					break;
				case 27:
					mode = HELPEXIT;
					break;
				default:
					printf("\007");
					break;
				}
			}
		}

	switch (mode) {
		case HELPMAIN:
			helpmain();
			break;
		case HELPCYCLING:
			helpcycling();
			break;
		case HELPMOUSE:
			helpmouse();
			break;
		case HELPCMDLINE:
			helpcmdline();
			break;
		case HELPFRACTALS:
			helpfractals();
			break;
		case HELPVIDEO:
			key = helpvideo();
			break;
		default:
			key = 27;
			break;
		}
	if (mode != HELPEXIT && mode != HELPVIDEO) {
		printf("\nPress ESCAPE to exit Help mode, or 'h' to see the help menu. ");
		printf("\nPressing any other key passes that keypress back to your program. ");
		key = getakey();
		}
	if (key != 27
		&& key != 'h' && key != 'H'
		&& key != '?' && key != '/') {
		setforgraphics();
		return(key);
		}
	if (key == 27)
		mode = HELPEXIT;
	else 
		mode = HELPMENU;
	}

setforgraphics();
return(0);
}

helpauthors()
{
helptitle();
printf("\n");
printf("------------------  Primary Authors (this changes over time)  -----------------\n");
printf("Bert Tyler      - original author, coordinator, integer Mandelbrot/Julia\n");
printf("                  routines, zoom and pan, video mode table, save-to-disk...\n");
printf("                  Compuserve ID: [73477,433]   BIX ID: btyler\n");
printf("Timothy Wegner  - superVGA support, restore-from-disk, color-cycling,\n");
printf("                  new fractal types, configuration files, ...\n");
printf("                  Compuserve ID: [71320,675]\n");
printf("\n");
printf("------------------   A Partial List of Contributing Authors   -----------------\n");
printf("Michael Abrash  - 360x480x256 mode\n");
printf("Steve Bennett   - restore-from-disk logic\n");
printf("John Bridges    - superVGA support, 360x480x256 mode\n");
printf("Mike Kaufman    - mouse support\n");
printf("Bret Mulvey     - plasma clouds\n");
printf("Mark Peterson   - periodicity logic\n");
printf("Dave Warker     - integer fractals concept\n");
printf("Richard Wilton  - 'tweaked' VGA modes\n");
printf("MS-Kermit       - keyboard routines\n");
printf("PC Tech Journal - CPU detector routine\n");
printf("\n");
}

helpmenu()
{
helptitle();
printf("\n");
printf("\n\nThe following help screens are available\n\n");
printf("1   - Commands available at the initial Credits Screen and main command level\n\n");
printf("2   - Commands available at the Color-Cycling command level\n\n");
printf("3   - Using FRACTINT with a mouse\n\n");
printf("4   - Command-Line options (things you can set when you type 'fractint')\n\n");
printf("5   - Descriptions of currently available fractal types\n\n");
printf("6   - List of Available Video Modes and the keys that select them\n");
printf("      (and if you're at the initial Credits Screen, no images are going\n");
printf("      to get drawn until you select a video mode)\n");
printf("\n\nPlease press one of the above keys (or press ESCAPE to exit Help Mode)\n");
}

helpmain()
{
helptitle();
printf("The useful keys you can hit while this program is running (the commands marked\n");
printf("                    with an '*' are also available at the credits screen) are:\n");
printf("* h or H or ?       HELP! (Enter help mode and display this screen)\n");
printf("  PageUp, PageDown  Shrink or Expand the Zoom Box  \n");
printf("  Cursor Keys       Pan (Move) the Zoom Box (Control-Keys for Fast-Pan)\n");
printf("  End or Enter      Redraw the Screen or area inside the Zoom Box \n");
printf("* F1,F2,F3,F4...    Select a new Video Mode and THEN Redraw\n");
printf("  Spacebar          Mandelbrot/Julia Set toggle (read FRACTINT.DOC first)\n");
printf("* 1 or 2 or g       Select Single-Pass, Dual-Pass, or Solid-Guessing mode\n");
printf("  < or >            Lower or Raise the Iteration Limit (display with Tab key)\n");
printf("  c or C or + or -  Enter Color-Cycling Mode (see Color-Cycling Help screen)\n");
printf("  s or S            Save the current screen image to disk\n");
printf("* r or R            Restart from a file previously saved by the 's' command\n");
printf("  b or B            Add the current fractal description to FRABATCH.BAT\n");
printf("* t or T            Select a new fractal type and parameters\n");
printf("  o or O            toggles 'orbits' option on and off during image generation\n");
printf("  Home              Redraw Previous screen (you can 'back out' recursively)\n");
printf("  Tab               Display the current fractal image information\n");
printf("* Insert            Restart the program (at the credits screen)\n");
printf("* Delete or Esc     Stop the program and return to MSDOS\n");
printf("Hit any of these keys while drawing a fractal to immediately do the command.\n");
printf("   If the screen finishes before you hit a key, it will beep and wait for you.");
}

helpcycling()
{
helptitle();
printf("\n");
printf("Command Keys that are available in Color-Cycling mode (which is the mode you\n");
printf("are in if you have hit the 'c' key and are now paused in Color-Cycling mode\n");
printf("with a white overscan (border) area, or you hit the '+' or '-' keys and\n");
printf("the the colors are now cycling on your screen)\n\n");
printf("  h or H or ?      HELP! (Enter help mode and display this screen)\n");
printf("  + or -           (re)-set the direction of the color-cycling\n");
printf("  Right/Left Arrow (re)-set the direction of the color-cycling (just like +/-)\n");
printf("  Up/Down Arrow    SpeedUp/SlowDown the color cycling process\n");
printf("  F1 thru F10      Select Short--Medium--Long (randomly-generated) color bands\n");
printf("  1  thru 9        Cycle through 'nn' colors between screen updates (default=1)\n");
printf("  Enter            Randomly (re)-select all new colors  [TRY THIS ONE!]\n");
printf("  Spacebar         Pause until another key is hit (the overscan area is set\n");
printf("                   to white as a visual indicator of a Color-Cycling pause)\n");
printf("  r or g or b or   force a pause and Lower (lower case) or Raise (upper case)\n");
printf("  R or G or B      the Red, Green, or Blue component of the fractal image\n");
printf("\n");
printf("  (any other key)  Exit Color-Cycling and return to main command level\n");
printf("\n\n");
}

helpmouse()
{
helptitle();
printf("\n");
printf("Using FRACTINT with a Mouse\n\n");
printf("Left Button:   Brings up and sizes the Zoom Box.   While holding down the\n");
printf("               left button, push the mouse forward to shrink the Zoom Box,\n");
printf("               and pull it back to expand it.   Then let go of the button\n");
printf("               and move the mouse around to 'pan' the Zoom Box (with no\n");
printf("               buttons held down, you are in 'fast-pan' mode).\n");
printf("\n");
printf("Right Button:  When the right button is held down, the 'panning' operation\n");
printf("               switches from 'fast-pan' to 'slow-pan' mode, giving you\n");
printf("               better control over the location of the Zoom Box.\n");
printf("\n");
printf("Both Buttons:  (or the middle button, if you have three of them) Redraws\n");
printf("               the area inside the Zoom Box over your full screen.\n");
printf("\n");
printf("Zoom and Pan using the mouse typically consists of pushing in the left\n");
printf("button, sizing the zoom box, letting go of the button, fast-panning to\n");
printf("the general area, pushing in the right button and slow-panning to the\n");
printf("exact area you want, and then (still holding down the right button) tapping\n");
printf("the left button to perform the Zoom.\n");
}

helpcmdline()
{
helptitle();
printf("\n");
printf("The Command-Line options available, and their formats are:\n\n");
printf("filename                   Start with this saved file (which was saved\n");
printf("                           previously by version 6.3 or later of FRACTINT)\n");
printf("savename=filename          Save files using this name (instead of FRACT001)\n");
printf("maxiter=nnn                Maximum number of iterations (default = 150)\n");
printf("iterincr=nnn               Iteration inc/decrement stepsize (default = 50)\n");
printf("inside=nnn                 Mandelbrot Interior color (inside=0 for black)\n");
printf("video=xxx                  Begin with this video mode (Example: Video=F2)\n");
printf("passes=x (x = 1, 2, or g)  Select Single-Pass, Dual-Pass, or Solid-Guessing\n");
printf("params=xxx[/xxx[/xxx]]...  Begin with these extra Parameter values\n");
printf("                           (Examples:  params=4   params=-0.480/0.626)\n");
printf("corners=xmin/xmax/ymin/ymax  Begin with these X, Y Coordinates\n");
printf("                           (Example: corners=-0.739/-0.736/0.288/0.291)\n");
printf("type=fractaltype           Perform this Fractal Type (Default = mandel)\n");
printf("                           Types include: mandel, julia, newtbasin, newton,\n");
printf("                           lambda, plasma, mandelfp, juliafp, ...\n");
printf("batch=yes                  Batch mode run (display image, save-to-disk, exit)\n");
printf("batch=config               Batch mode run to generate a 'fractint.cfg' file\n");
printf("cyclelimit=nnn             color-cycler speed-limit (1 to 256, default = 55)\n");
}

helpfractals()
{
helptitle();
printf("\n");
printf("Fractal types supported include (see FRACTINT.DOC for full descriptions):\n\n");
printf("mandel     = 'Classic' Mandelbrot fractals using 32-bit integer math for speed.\n");
printf("             z(0) = 0; z(n+1) = z(n)**2 + C, where C = Xcoord + i * Ycoord.\n");
printf("             Two optional params: real and imaginary parts of z(0) (if not 0).\n");
printf("julia      = 'Classic' Julia set fractals using 32-bit integer math for speed.\n");
printf("             z(0) = Xcoord + i * Ycoord; z(n+1) = z(n)**2 + C.\n");
printf("             Two params required: real and imaginary parts of C.\n");
printf("mandelfp,  = 'Classic' Mandelbrot and Julia set fractals using traditional\n");
printf("juliafp      floating point math.  Params identical to above.  Included\n");
printf("             mostly for historical purposes.\n");
printf("newton,    = Newton Domains-of-attraction (only the coloring schemes are\n");
printf("newtbasin    different).  One param:  the power (from 3 to 10) of the eqn.\n");
printf("             If param=4, the eqn is z(n+1) = (4*z(n)**3+1)/(3*z(n)**4).\n");
printf("plasma     = plasma clouds - random, cloud-like formations.  REQUIRES 256-\n");
printf("             colors and VGA.  One param: 'graininess' (.5 to 50, default = 2)\n");
printf("lambdasine = lambda-sine fractal.  z(0) = Xcoord + i * Ycoord;  z(n+1) = \n");
printf("             lambda * sine(z(n)).  Two params: real, imag portions of lambda.\n");
printf("test       = 'test' point letting us (and you!) easily add fractal types.\n");
printf("             A simple Mandelbrot routine in the distributed version.\n");
}

extern int hasconfig;
extern char *fkeys[], *accessmethod[];

helpvideo()
{
int key, i ,j;

j = -12;
key = 13;

while (key == 13) {
	helptitle();
	printf("\n");
	j += 12;	/* display next page */
	if (j >= maxvideomode) j = 0;	/* (or 1st page)     */
	printf("The current list of supported Video Adapters and Modes includes:\n");
	if (hasconfig == 0)
		printf("   <<NOTE>> The default list has been replaced by a FRACTINT.CFG file.\n");
	else
		printf("\n");
	printf("      %-25s  Resolution Colors  %-25s\n\n",
	"Video Adapter & Mode", "     Comments");
	for (i = j; i < maxvideomode && i < j + 12; i++) {
		printf("%-6s%-25s%5d x%4d%5d %1s  %-25s\n",fkeys[i],
			videomode[i].name,
			videomode[i].xdots,
			videomode[i].ydots,
			videomode[i].colors,
			accessmethod[videomode[i].dotmode],
			videomode[i].comment
			);
		}
	for (; i <= j+12; i++) printf("\n");
	printf("    (Notation:  F1, SF1, CF1, AF1 == Normal-, Shift-, Control- or Alt-F1) \n");
	printf("    'B' after #-of-colors means video access is via the BIOS (s-l-o-w-l-y)\n");
	printf("\nPress the ENTER key to see the next screen of Video Modes. ");
	printf("\nPress ESCAPE or the 'h' key to return to the help menu. ");
	printf("\nPressing any other key passes that keypress back to your program. ");

	key = getakey();
	}
return(key);
}
