/*

	Roll-Your-Own video mode (DOTMODE 19) routines.
	
Even if you don't have an assembler, you can add your own video-mode
routines to FRACTINT by using the "FRACTINT BATCH=CONFIG" option to add
a video mode of the appropriate resolution that uses dotmode 19 (which
calls these routines to perform all the dirty work) and modifying
these routines accordingly.  The four routines are:

 startvideo()    Do whatever you have to do to throw your adapter into
                 the appropriate video mode (in case it can't be accomplished
                 the "normal" way, with INT 10H and the AX/BX/CX/DX values
                 available via FRACTINT.CFG or FARVIDEO.ASM).  This routine
                 will typically be empty (in which case the AX/BX/CX/DX values
                 in FRACTINT.CFG or FARVIDEO.ASM must be encoded appropriately
                 to accomplish the task), but some adapters like the 8514/A
                 and TARGA need special handling which would go here.
                 If you DO have to put something here, you should encode
                 AX = 0xFF so as to effectively convert the regular
                 video-switching code inside VIDEO.ASM to use 
                 an invalid INT 10H call - "do-nothing" logic. 
 
 endvideo()      do whatever you have to do to get it out of that
                 special video mode (in case 'setvideo(3,0,0,0)'
                 won't do it) - this routine will typically be empty,
                 but some adapters like the 8514/A and TARGA need
                 special handling which would go here.
 
 writevideo(int x, int y, int color)  write a pixel using color number
                 'color' at screen coordinates x,y (where 0,0 is the
                 top left corner, and xdots,0 is the top right corner)
 
 int readvideo(int x, int y)  return the color number of pixel x,y
                 using the same coordinate logic as 'writevideo()'


Finally, note that, although these example routines are written in "C",
they could just as easily (or maybe more easily!) have been written
in assembler.
 
*/

/* external variables (set in the FRACTINT.CFG file, but findable here */

extern	int	dotmode;		/* video access method (= 19)      */
extern	int	oktoprint;		/* set to 0 if printf() won't work */
extern	int	xdots, ydots;		/* total # of dots on the screen   */
extern	int	colors;			/* maximum colors available        */

/* for demo purposes, these routines use VGA mode 13h - 320x200x256 */

#include <dos.h>

int startvideo()
{

/* assume that the encoded values in FRACTINT.CFG or FARVIDEO.ASM
   have been set to accomplish this (AX = 0x13, BX = CX = DX = 0)  */

return(0);				/* set flag: video started */

/*   or, we could have done this instead and encoded AX = 0xFF
     in FRACTINT.CFG/FARVIDEO.ASM: 

union REGS regs;

regs.x.ax = 0x13;
int86(0x10,&regs,&regs);

*/

}

int endvideo()
{

return(0);				/* set flag: video ended */

}

void writevideo(int x, int y, int color)
{

union REGS regs;

regs.h.ah = 0x0c;			/* invoke INT 10H with AH = 0CH */
regs.h.al = color;
regs.x.bx = 0;
regs.x.cx = x;
regs.x.dx = y;
int86(0x10,&regs,&regs);

}

unsigned int readvideo(int x, int y)
{

union REGS regs;

regs.x.ax = 0x0d00;			/* invoke INT 10H with AH = 0DH */
regs.x.bx = 0;
regs.x.cx = x;
regs.x.dx = y;
int86(0x10,&regs,&regs);

return((unsigned int)regs.h.al);	/* return pixel color */

}