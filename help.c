/*
	FRACTINT Help routines
*/

#include "fractint.h"
#include <stdio.h>

	/* the actual message text is defined in FARMSG.ASM */
extern char far helpmessagetitle[];
extern char far helpmessageauthors[];
extern char far helpmessagecredits[];
extern char far helpmessagemenu[];
extern char far *helpmessagemain[];
extern char far *helpmessagecycling[];
extern char far *helpmessagexhair[];
extern char far *helpmessagemouse[];
extern char far *helpmessagecmdline[];
extern char far *helpmessagefractals[];
extern char far helpmessageformoreinfo[];
extern char far helpmessagemoretext[];
extern char far helpmessageendtext[];
extern char far helpmessagevideo[];

extern	void	helpmessage(unsigned char far *);

extern int adapter;

extern int	lookatmouse;	/* used to activate non-button mouse movement */

helptitle()
{
home();					/* home the cursor		*/
setclear();				/* clear the screen		*/
helpmessage(helpmessagetitle);

}

help()
{
int mode, key;
int oldlookatmouse;

oldlookatmouse = lookatmouse;
lookatmouse = 0;			/* de-activate full mouse checking */

mode = helpmode;

if (mode == HELPAUTHORS) {
	int toprow, botrow, i, j, delaymax;
	char oldchar;
	int authors[100];		/* this should be enough for awhile */
	
	toprow = 10;
	botrow = 21;
	j = 0;
	authors[j] = 0;			/* find the start of each credit-line */
	for (i = 0; helpmessagecredits[i] != 0; i++)
		if (helpmessagecredits[i] == 10)
			authors[++j] = i+1;
	authors[j+1] = i;
	helptitle();
	helpmessage(helpmessageauthors);
	movecursor(toprow,0);
	i = botrow - toprow;
	oldchar = helpmessagecredits[authors[i+1]];
	helpmessagecredits[authors[i+1]] = 0;
	helpmessage(helpmessagecredits);
	helpmessagecredits[authors[i+1]] = oldchar;
	delaymax = 10;
	while (! keypressed()) {
		for (j = 0; j < delaymax && !(keypressed()); j++)
			delay(100);
		if (keypressed() == 32) {	/* spacebar pauses */
			getakey();
			while (!keypressed()) ;
			if (keypressed() == 32)
				getakey();
			}
		delaymax = 15;
		scrollup(toprow, botrow);
		movecursor(botrow,0);
		i++;
		if (helpmessagecredits[authors[i]] == 0) i = 0;
		oldchar = helpmessagecredits[authors[i+1]];
		helpmessagecredits[authors[i+1]] = 0;
		helpmessage(&helpmessagecredits[authors[i]]);
		helpmessagecredits[authors[i+1]] = oldchar;
		}
	lookatmouse = oldlookatmouse;		/* restore the mouse-checking */
	return(0);
	}

setfortext();
while (mode != HELPEXIT) {
	if (mode == HELPMENU) {
		while (mode == HELPMENU) {
			helptitle();
			helpmessage(helpmessagemenu);
			key = getakey();
			switch (key) {
				case '1':
					mode = HELPMAIN;
					break;
				case '2':
					mode = HELPCYCLING;
					break;
				case '3':
					mode = HELPXHAIR;
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
				case '7':
					mode = HELPMOREINFO;
					break;
				case '8':
					mode = HELPMOUSE;
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
		default:
			key = 27;
			break;
		}
	if (key != 27
		&& key != 'h' && key != 'H'
		&& key != '?' && key != '/') {
		setforgraphics();
		lookatmouse = oldlookatmouse;		/* restore the mouse-checking */
		return(key);
		}
	if (key == 27)
		mode = HELPEXIT;
	else 
		mode = HELPMENU;
	}

setforgraphics();
lookatmouse = oldlookatmouse;		/* restore the mouse-checking */
return(0);
}

helppage(char far * helppages[])
{
int key, page;

if (helppages[1] == NULL) {
	helptitle();
	helpmessage(helppages[0]);
	movecursor(22,0);
	helpmessage(helpmessageendtext);
	return(getakey());
	}
	
key = 13;
page = 1;

while (key == 13 || key == 1013) {
	helptitle();
	helpmessage(helppages[page-1]);
	movecursor(21,0);
	helpmessage(helpmessagemoretext);
	helpmessage(helpmessageendtext);
	if (helppages[++page-1] == NULL) page = 1;
	key = getakey();
	}
return(key);
}

extern int hasconfig;
extern char *fkeys[];

helpvideo()
{
int key, i ,j;
char accessmethod[2];

j = -12;
key = 13;

while (key == 13 || key == 1013) {
	helptitle();
	printf("\n");
	j += 12;	/* display next page */
	if (j >= maxvideomode) j = 0;	/* (or 1st page)     */
	printf("The current list of supported Video Adapters and Modes includes:\n");
	if (hasconfig == 0)
		printf("   <<NOTE>> The default list has been replaced by a FRACTINT.CFG file.\n");
	else
		printf("\n");
	printf("       %-25s  Resolution Colors %-25s\n\n",
	"Video Adapter & Mode", "     Comments");
	for (i = j; i < maxvideomode && i < j + 12; i++) {
		fromvideotable(i);
	strcpy(accessmethod," ");
	if (videoentry.dotmode == 1)
		accessmethod[0] = 'B';
#ifdef __TURBOC__
		printf("%-6.6s %-25.25s%5d x%4d%5d %1.1s  %-25.25s\n",
#else
		printf("%-6s %-25s%5d x%4d%5d %1s  %-25s\n",
#endif
			fkeys[i],
			videoentry.name,
			videoentry.xdots,
			videoentry.ydots,
			videoentry.colors,
			accessmethod,
			videoentry.comment
			);
		}
	for (; i <= j+12; i++) printf("\n");
	helpmessage(helpmessagevideo);
	fromvideotable(adapter);

	key = getakey();
	}
return(key);
}

