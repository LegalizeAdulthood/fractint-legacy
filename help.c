/*
	FRACTINT Help routines
	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
*/

#include "fractint.h"
#include <stdio.h>
#include <time.h>

/* routines in this module	*/

void help_overlay();
int  help(void);

static int  helppage(char far *helppages[]);
static int  helpvideo(void);
static void showapage(char far *info);

/* the actual message text is defined in helpmsg.asm */
extern char far helpmessageauthors[];
extern char far helpmessagecredits[];
extern char far helpmessagemenu[];
extern char far helpmessagemenuinstr[];
extern char far *helpmessagemain[];
extern char far *helpmessagecycling[];
extern char far *helpmessagexhair[];
extern char far *helpmessagemouse[];
extern char far *helpmessagecmdline[];
extern char far *helpmessagefractals[];
extern char far *helpmessageformoreinfo[];
extern char far *helpmessageloadfile[];
extern char far *helpmessageview[];
extern char far *helpmessagezoom[];
extern char far helpmessageendtext[];
extern char far helpmessagenexttext[];
extern char far helpmessageprevtext[];
extern char far helpmessagenextprevtext[];
extern char far helpmessagevideo[];

extern	int	adapter;
extern	int	lookatmouse;	/* used to activate non-button mouse movement */
extern	long	timer_start;
extern	int	textcbase;

void help_overlay() { } 		/* for restore_active_ovly */


#ifdef __TURBOC__
#   define FIX_HLPMSG(msg) (char far *)( ((unsigned)((long)(msg)>>16)!=_DS) ? (((unsigned long)_CS)<<16) | ((unsigned)(msg)) : (msg) )
#endif


static void puthelpmsg(int row, int col, int attr, char far *msg)
   {

#ifdef __TURBOC__
   msg = FIX_HLPMSG(msg);
#endif
   putstring(row, col, attr, msg);  /* should not call an overlay! */
   }


static void puthelpmsgcenter(int row, int col, int width, int attr, char far *msg)
   {
#ifdef __TURBOC__
   msg = FIX_HLPMSG(msg);
#endif
   putstringcenter(row, col, width, attr, msg); /* should not call an overlay! */
   }


int help()
{
int mode, key;
int oldlookatmouse;

ENTER_OVLY(OVLY_HELP);

timer_start -= clock(); 		/* "time out" during help */

oldlookatmouse = lookatmouse;
lookatmouse = 0;			/* de-activate full mouse checking */

mode = helpmode;

if (mode == HELPAUTHORS) {
	int toprow, botrow, i, j, delaymax;
	char oldchar;
	int authors[100];		/* this should be enough for awhile */
	char far *credits;

#ifdef __TURBOC__
	credits = FIX_HLPMSG(helpmessagecredits);
#else
	credits = helpmessagecredits;
#endif

	j = 0;
	authors[j] = 0; 		/* find the start of each credit-line */
	for (i = 0; credits[i] != 0; i++)
		if (credits[i] == 10)
			authors[++j] = i+1;
	authors[j+1] = i;

	helptitle();
	toprow = 8;
	botrow = 21;
	puthelpmsgcenter(1,0,80,C_TITLE,
			"Press ENTER for main menu, F1 for help.");
	puthelpmsg(2,0,C_CONTRIB,helpmessageauthors);
	setattr(2,0,C_AUTHDIV1,80);
	setattr(7,0,C_AUTHDIV1,80);
	setattr(22,0,C_AUTHDIV2,80);
	setattr(3,0,C_PRIMARY,320);
	setattr(23,0,C_TITLE_LOW,160);
	for (i = 3; i < 7; ++i)
	   setattr(i,20,C_CONTRIB,60);
	setattr(toprow,0,C_CONTRIB,14*80);
	i = botrow - toprow;
	oldchar = credits[authors[i+1]];
	credits[authors[i+1]] = 0;
	puthelpmsg(toprow,0,C_CONTRIB,credits);
	credits[authors[i+1]] = oldchar;
	delaymax = 10;
	movecursor(25,80); /* turn it off */
	helpmode = HELPMENU;
	while (! keypressed()) {
		for (j = 0; j < delaymax && !(keypressed()); j++)
			delay(100);
		if (keypressed() == 32) {	/* spacebar pauses */
			getakey();
			while (!keypressed()) ;
			if (keypressed() == 32)
				getakey();
			}
#ifdef __TURBOC__
		credits = FIX_HLPMSG(helpmessagecredits); /* because keypressed() may call an overlay */
#endif

		delaymax = 15;
		scrollup(toprow, botrow);
		i++;
		if (credits[authors[i]] == 0) i = 0;
		oldchar = credits[authors[i+1]];
		credits[authors[i+1]] = 0;
		puthelpmsg(botrow,0,C_CONTRIB,&credits[authors[i]]);
		setattr(botrow,0,C_CONTRIB,80);
		credits[authors[i+1]] = oldchar;
		movecursor(25,80); /* turn it off */
		}
	lookatmouse = oldlookatmouse;		/* restore the mouse-checking */
	helpmode = HELPAUTHORS;
	EXIT_OVLY;
	return(0);
	}

stackscreen();
while (mode != HELPEXIT) {
	if (mode == HELPMENU) {
		while (mode == HELPMENU) {
			showapage(helpmessagemenu);
			puthelpmsg(23,0,C_HELP_INSTR,helpmessagemenuinstr);
			movecursor(25,80); /* turn it off */
			key = getakey();
			switch (key) {
				case '1':
					mode = HELPMAIN;
					break;
				case '2':
					mode = HELPZOOM;
					break;
				case '3':
					mode = HELPCYCLING;
					break;
				case '4':
					mode = HELPXHAIR;
					break;
				case '5':
					mode = HELPFRACTALS;
					break;
				case '6':
					mode = HELPVIDEO;
					break;
				case '7':
					mode = HELPMOUSE;
					break;
				case '8':
					mode = HELPCMDLINE;
					break;
				case '9':
					mode = HELPMOREINFO;
					break;
				case 27:
					mode = HELPEXIT;
					break;
				default:
					buzzer(2);
					break;
				}
			}
		}

	switch (mode) {
		case HELPMAIN:
			key = helppage(helpmessagemain);
			break;
		case HELPCYCLING:
			key = helppage(helpmessagecycling);
			break;
		case HELPXHAIR:
			key = helppage(helpmessagexhair);
			break;
		case HELPMOUSE:
			key = helppage(helpmessagemouse);
			break;
		case HELPCMDLINE:
			key = helppage(helpmessagecmdline);
			break;
		case HELPFRACTALS:
			key = helppage(helpmessagefractals);
			break;
		case HELPVIDEO:
			key = helpvideo();
			break;
		case HELPMOREINFO:
			key = helppage(helpmessageformoreinfo);
			break;
		case HELPLOADFILE:
			key = helppage(helpmessageloadfile);
			break;
		case HELPZOOM:
			key = helppage(helpmessagezoom);
			break;
		case HELPVIEW:
			key = helppage(helpmessageview);
			break;
		default:
			key = 27;
			break;
		}
	if (key == 27)
		mode = HELPEXIT;
	else
		mode = HELPMENU;
	}

unstackscreen();
lookatmouse = oldlookatmouse;		/* restore the mouse-checking */
timer_start += clock(); 		/* end of "time out" */
EXIT_OVLY;
return(0);
}

static int helppage(char far * helppages[])
{
int key, page;

if (helppages[1] == NULL) { /* just one page of help for this type */
	showapage(helppages[0]);
	puthelpmsg(23,0,C_HELP_INSTR,helpmessageendtext);
	movecursor(25,80); /* turn it off */
	while ((key = getakey()) != 27 && key != 1059) { } /* just ESC or F1 */
	return(key);
	}

page = 0;
while (1) {
	showapage(helppages[page]);
	puthelpmsg(23,0,C_HELP_INSTR,helpmessageendtext);
	if (page == 0)
		puthelpmsg(24,0,C_HELP_INSTR,helpmessagenexttext);
	else if (helppages[page+1] == NULL)
		puthelpmsg(24,0,C_HELP_INSTR,helpmessageprevtext);
	else
		puthelpmsg(24,0,C_HELP_INSTR,helpmessagenextprevtext);
	movecursor(25,80); /* turn it off */
	while (1) {
		key = getakey();
		if (key == 27 || key == 1059) /* escape or F1 */
			return(key);
		if (key == 13 || key == 1013) { /* for old time's sake */
			if (helppages[++page] == NULL) page = 0;
			break;
			}
		if (key == 1073 && page > 0) { /* page up */
			--page;
			break;
			}
		if (key == 1081 && helppages[page+1] != NULL) { /* page down */
			++page;
			break;
			}
		}
	}
}

static void showapage(char far *info)
{
char far *body;

#ifdef __TURBOC__
   info = FIX_HLPMSG(info);
#endif

body = info;
while (*body != 10) ++body; /* find end of 1st line */
helptitle();
*body = 0;
puthelpmsgcenter(1,0,80,C_HELP_HDG,info);
*body = 10;
setattr(2,0,C_HELP_BODY,21*80);
setattr(23,0,C_HELP_INSTR,160);
textcbase = 1;
puthelpmsg(2,0,C_HELP_BODY,body+1);
textcbase = 0;
}

extern int hasconfig;
extern char *fkeys[];

static int helpvideo()
{
int key, i ,j;
char accessmethod[2];
char msg[81];

j = 0;
while (1) {
	showapage("Video Modes\n");
	puthelpmsg(23,0,C_HELP_INSTR,helpmessageendtext);
	if (j == 0) {
		if (j+12 < maxvideomode)
			puthelpmsg(24,0,C_HELP_INSTR,helpmessagenexttext);
		}
	else if (j+12 >= maxvideomode)
		puthelpmsg(24,0,C_HELP_INSTR,helpmessageprevtext);
	else
		puthelpmsg(24,0,C_HELP_INSTR,helpmessagenextprevtext);
	if (hasconfig == 0)
		puthelpmsg(3,4,C_HELP_BODY,
		 "<<NOTE>> The default list has been replaced by a FRACTINT.CFG file.");
	sprintf(msg,"%-25s  Resolution Colors %-25s",
		"Video Adapter & Mode", "     Comments");
	puthelpmsg(4,8,C_HELP_BODY,msg);
	movecursor(6,0);
	for (i = j; i < maxvideomode && i < j + 12; i++) {
		fromvideotable(i);
		strcpy(accessmethod," ");
		if (videoentry.dotmode % 100 == 1)
			accessmethod[0] = 'B';
#ifdef __TURBOC__
		sprintf(msg,"%-6.6s %-25.25s%5d x%4d%5d %1.1s  %-25.25s\n",
#else
		sprintf(msg,"%-6s %-25s%5d x%4d%5d %1s  %-25s\n",
#endif
			fkeys[i],
			videoentry.name,
			videoentry.xdots,
			videoentry.ydots,
			videoentry.colors,
			accessmethod,
			videoentry.comment
			);
		puthelpmsg(-1,1,C_HELP_BODY,msg);
		}
	puthelpmsg(19,1,C_HELP_BODY,helpmessagevideo);
	movecursor(25,80); /* turn it off */
	fromvideotable(adapter); /* restore this to normal */
	while (1) {
		key = getakey();
		if (key == 27 || key == 1059) /* escape or F1 */
			return(key);
		if (key == 13 || key == 1013) { /* for old time's sake */
			if ((j += 12) >= maxvideomode) j = 0;
			break;
			}
		if (key == 1073 && j > 0) { /* page up */
			j -= 12;
			break;
			}
		if (key == 1081 && j+12 < maxvideomode) { /* page down */
			j += 12;
			break;
			}
		}
	}
}

