/*
	Miscellaneous C routines used only in DOS Fractint.
*/

#include <string.h>
#include <stdio.h>
#include <dos.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <fcntl.h>
#include "fractint.h"
#include "fractype.h"

/* routines in this module	*/

int  stopmsg(int,unsigned char far *);
int  texttempmsg(char *msgparm);
int  showtempmsg(char *msgparm);
void cleartempmsg(void);
void blankrows(int row,int rows,int attr);
void helptitle(void);
int  putstringcenter(int row, int col, int width, int attr, char far *msg);
void stackscreen(void);
void unstackscreen(void);
void discardscreen(void);
int  fullscreen_choice(
	     int options, char *hdg, char *hdg2, char *instr, int numchoices,
	     char **choices, int *attributes, int boxwidth, int boxdepth,
	     int colwidth, int current,
	     void (*formatitem)(), int (*showdetail)(),
	     char *speedstring, int (*speedprompt)(), int (*checkkey)());
int  strncasecmp(char *s,char *t,int ct);
int  main_menu(int fullmenu);
int  input_field(int options, int attr, char *fld, int len, int row, int col, int (*checkkey)(int) );
int  field_prompt(int options, char *hdg, char *instr, char *fld, int len, int (*checkkey)() );
int  thinking(int options, char *msg);
void clear_screen(void);
int  savegraphics(void);
void restoregraphics(void),discardgraphics(void);

static int menu_checkkey(int curkey);

int release=1530; /* this has 2 implied decimals; increment it every synch */

/* Define command keys */
#define   PAGE_UP	 1073
#define   PAGE_DOWN	 1081
#define   CTL_HOME	 1119
#define   CTL_END	 1117
#define   LEFT_ARROW	 1075
#define   RIGHT_ARROW	 1077
#define   UP_ARROW	 1072
#define   DOWN_ARROW	 1080
#define   LEFT_ARROW_2	 1115
#define   RIGHT_ARROW_2  1116
#define   UP_ARROW_2	 1141
#define   DOWN_ARROW_2	 1145
#define   HOME		 1071
#define   END		 1079
#define   ENTER 	 13
#define   ENTER_2	 1013
#define   ESC		 27
#define   F1		 1059
#define   F2		 1060
#define   F5		 1063

/* fullscreen_choice options */
#define CHOICERETURNKEY 1
#define CHOICEMENU	2
#define CHOICEHELP	4

extern int  xdots, ydots, sxdots, sydots, sxoffs, syoffs;
extern int  colors;
extern int  dotmode;
extern int  oktoprint;
extern char far *findfont(int);
extern int  textrow, textcol, textrbase, textcbase;
extern int  debugflag;
extern int  fractype;
extern int  calc_status;
extern double param[];
extern int  kbdkeys[];
extern int  tabmode;
extern int  color_dark,color_medium,color_bright;
extern int  lookatmouse;
extern unsigned char dacbox[256][3];
extern int  reallyega;
extern int  extraseg;


/* int stopmsg(flags,message) displays message and waits for a key:
     message should be a max of 9 lines with \n's separating them;
       no leading or trailing \n's in message;
       no line longer than 76 chars for best appearance;
     flag options:
       &1 if already in text display mode, stackscreen is not called
	  and message is displayed at (12,0) instead of (4,0)
       &2 if continue/cancel indication is to be returned;
	  when not set, "Any key to continue..." is displayed
	  when set, "Escape to cancel, any other key to continue..."
	  -1 is returned for cancel, 0 for continue
       &4 set to suppress buzzer
       &8 for Fractint for Windows & parser - use a fixed pitch font
      &16 for info only message (green box instead of red in DOS vsn)
   */
int stopmsg (int flags, unsigned char far *msg)
{
   int ret,toprow,color,savelookatmouse;
   ret = 0;
   savelookatmouse = lookatmouse;
   lookatmouse = -13;
   if ((flags & 1))
      blankrows(toprow=12,10,7);
   else {
      stackscreen();
      toprow = 4;
      movecursor(4,0);
      }
   textcbase = 2; /* left margin is 2 */
   putstring(toprow,0,7,msg);
   if (flags & 2)
      putstring(textrow+2,0,7,"Escape to cancel, any other key to continue...");
   else
      putstring(textrow+2,0,7,"Any key to continue...");
   textcbase = 0; /* back to full line */
   color = (flags & 16) ? C_STOP_INFO : C_STOP_ERR;
   setattr(toprow,0,color,(textrow+1-toprow)*80);
   movecursor(25,80);	/* cursor off */
   if ((flags & 4) == 0)
      buzzer((flags & 16) ? 0 : 2);
   while (keypressed()) /* flush any keyahead */
      getakey();
   if (getakey() == 27)
      ret = -1;
   if ((flags & 1))
      blankrows(toprow,10,7);
   else
      unstackscreen();
   lookatmouse = savelookatmouse;
   return ret;
}


static char far *temptextsave = NULL;
static int  textxdots,textydots;

/* texttempmsg(msg) displays a text message of up to 40 characters, waits
      for a key press, restores the prior display, and returns (without
      eating the key).
      It works in almost any video mode - does nothing in some very odd cases
      (HCGA hi-res with old bios), or when there isn't 10k of temp mem free. */
int texttempmsg(char *msgparm)
{
   if (showtempmsg(msgparm))
      return(-1);
   while (!keypressed()) { } /* wait for a keystroke but don't eat it */
   cleartempmsg();
   return(0);
}

int showtempmsg(char *msgparm)
{
   extern int color_dark,color_medium;
   unsigned char msg[41],buffer[640];
   char far *fartmp;
   char far *fontptr;
   unsigned char *bufptr;
   int i,j,k,xrepeat,yrepeat,fontchar,charnum;
   int save_sxoffs,save_syoffs;
   strncpy(msg,msgparm,40);
   msg[40] = 0; /* ensure max message len of 40 chars */
   if (dotmode == 11) { /* disk video, screen in text mode, easy */
      dvid_status(0,msg);
      return(0);
      }
   if ((fontptr = findfont(0)) == NULL) { /* old bios, no font table? */
      if (oktoprint == 0	       /* can't printf */
	|| sxdots > 640 || sydots > 200) /* not willing to trust char cell size */
	 return(-1); /* sorry, message not displayed */
      textydots = 8;
      textxdots = sxdots;
      }
   else {
      xrepeat = (sxdots >= 640) ? 2 : 1;
      yrepeat = (sydots >= 300) ? 2 : 1;
      textxdots = strlen(msg) * xrepeat * 8;
      textydots = yrepeat * 8;
      }
   /* worst case needs 10k */
   if ((temptextsave = farmemalloc((long)textxdots * textydots)) == NULL)
      return(-1); /* sorry, message not displayed */
   save_sxoffs = sxoffs;
   save_syoffs = syoffs;
   sxoffs = syoffs = 0;
   fartmp = temptextsave;
   for (i = 0; i < textydots; ++i) {
      get_line(i,0,textxdots-1,buffer);
      for (j = 0; j < textxdots; ++j) /* copy it out to far memory */
	 *(fartmp++) = buffer[j];
      }
   if (fontptr == NULL) { /* bios must do it for us */
      home();
      printf(msg);
      }
   else { /* generate the characters */
      find_special_colors(); /* get color_dark & color_medium set */
      for (i = 0; i < 8; ++i) {
	 memset(buffer,color_dark,640);
	 bufptr = buffer;
	 charnum = -1;
	 while (msg[++charnum] != 0) {
	    fontchar = *(fontptr + msg[charnum]*8 + i);
	    for (j = 0; j < 8; ++j) {
	       for (k = 0; k < xrepeat; ++k) {
		  if ((fontchar & 0x80) != 0)
		     *bufptr = color_medium;
		  ++bufptr;
		  }
	       fontchar <<= 1;
	       }
	    }
	 for (j = 0; j < yrepeat; ++j)
	    put_line(i*yrepeat+j,0,textxdots-1,buffer);
	 }
      }
   sxoffs = save_sxoffs;
   syoffs = save_syoffs;
   return(0);
}

void cleartempmsg()
{
   unsigned char buffer[640];
   char far *fartmp;
   int i,j;
   int save_sxoffs,save_syoffs;
   if (dotmode == 11) /* disk video, easy */
      dvid_status(0,"");
   else if (temptextsave != NULL) {
      save_sxoffs = sxoffs;
      save_syoffs = syoffs;
      sxoffs = syoffs = 0;
      fartmp = temptextsave;
      for (i = 0; i < textydots; ++i) {
	 for (j = 0; j < textxdots; ++j) /* copy back from far memory */
	    buffer[j] = *(fartmp++);
	 put_line(i,0,textxdots-1,buffer);
	 }
      farmemfree(temptextsave);
      temptextsave = NULL;
      sxoffs = save_sxoffs;
      syoffs = save_syoffs;
      }
}


void blankrows(int row,int rows,int attr)
{
   char buf[81];
   memset(buf,' ',80);
   buf[80] = 0;
   while (--rows >= 0)
      putstring(row++,0,attr,buf);
   }


void helptitle()
{
   char msg[80],buf[10];
   setclear(); /* clear the screen */
   sprintf(msg,"FRACTINT  Version %d.%01d",release/100,(release%100)/10);
   if (release%10) {
      sprintf(buf,"%01d",release%10);
      strcat(msg,buf);
      }
   putstringcenter(0,0,80,C_TITLE,msg);
   putstring(0,3,C_TITLE_DEV, "Customized Version");
   putstring(0,53,C_TITLE_DEV,"Not for Public Release");
}


int putstringcenter(int row, int col, int width, int attr, char far *msg)
{
   char buf[81];
   int i,j,k;
   i = 0;
   while (msg[i]) ++i; /* strlen for a far */
   if (i == 0) return(-1);
   j = (width - i) / 2;
   j -= (width + 10 - i) / 20; /* when wide a bit left of center looks better */
   memset(buf,' ',width);
   buf[width] = 0;
   i = 0;
   k = j;
   while (msg[i]) buf[k++] = msg[i++]; /* strcpy for a far */
   putstring(row,col,attr,buf);
   return j;
}


static int screenctr=-1;
#define MAXSCREENS 3
static unsigned char far *savescreen[MAXSCREENS];
static int saverc[MAXSCREENS+1];
static FILE *savescf=NULL;
static char scsvfile[]="fractscr.tmp";
extern int text_type;
extern int textaddr;

void stackscreen()
{
   unsigned char far *vidmem;
   int savebytes;
   int i;
   unsigned char far *ptr;
   char buf[100];
   saverc[screenctr+1] = textrow*80 + textcol;
   if (++screenctr) { /* already have some stacked */
      if ((i = screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
	 stopmsg(1,"stackscreen overflow");
	 exit(1);
	 }
#ifdef __TURBOC__
      vidmem = MK_FP(textaddr,0);
#else
      FP_SEG(vidmem)=textaddr;
      FP_OFF(vidmem)=0;
#endif
      savebytes = (text_type == 0) ? 4000 : 16384;
      if ((ptr = savescreen[i] = farmemalloc((long)savebytes))) {
	 while (--savebytes >= 0)
	    *(ptr++) = *(vidmem++);
	 }
      else {
	 if (savescf == NULL) { /* create file just once */
	    if ((savescf = fopen(scsvfile,"wb")) == NULL)
	       goto fileproblem;
	    if (fwrite(buf,MAXSCREENS,16384,savescf) != 16384)
	       goto fileproblem;
	    fclose(savescf);
	    if ((savescf = fopen(scsvfile,"r+b")) == NULL) {
fileproblem:   stopmsg(1,"insufficient memory, aborting");
	       exit(1);
	       }
	    }
	 fseek(savescf,(long)(savebytes*i),SEEK_SET);
	 while (--savebytes >= 0)
	    putc(*(vidmem++),savescf);
	 }
      setclear();
      }
   else
      setfortext();
}

void unstackscreen()
{
   char far *vidmem;
   int savebytes;
   unsigned char far *ptr;
   textrow = saverc[screenctr] / 80;
   textcol = saverc[screenctr] % 80;
   if (--screenctr >= 0) { /* unstack */
#ifdef __TURBOC__
      vidmem = MK_FP(textaddr,0);
#else
      FP_SEG(vidmem)=textaddr;
      FP_OFF(vidmem)=0;
#endif
      savebytes = (text_type == 0) ? 4000 : 16384;
      if ((ptr = savescreen[screenctr])) {
	 while (--savebytes >= 0)
	    *(vidmem++) = *(ptr++);
	 farmemfree(savescreen[screenctr]);
	 }
      else {
	 fseek(savescf,(long)(savebytes*screenctr),SEEK_SET);
	 while (--savebytes >= 0)
	    *(vidmem++) = getc(savescf);
	 }
      }
   else
      setforgraphics();
   movecursor(-1,-1);
}

void discardscreen()
{
   if (--screenctr >= 0) { /* unstack */
      if (savescreen[screenctr])
	 farmemfree(savescreen[screenctr]);
      }
   else
      discardgraphics();
}


/* --------------------------------------------------------------------- */

char speed_prompt[]="Speed key string";

int fullscreen_choice(
	int options,	     /* &2 use menu coloring scheme	       */
			     /* &4 include F1 for help in instructions */
	char *hdg,	     /* heading info, \n delimited	       */
	char *hdg2,	     /* column heading or NULL		       */
	char *instr,	     /* instructions, \n delimited, or NULL    */
	int numchoices,      /* How many choices in list	       */
	char **choices,      /* array of choice strings 	       */
	int *attributes,     /* &3: 0 normal color, 1,3 highlight      */
			     /* &256 marks a dummy entry	       */
	int boxwidth,	     /* box width, 0 for calc (in items)       */
	int boxdepth,	     /* box depth, 0 for calc, 99 for max      */
	int colwidth,	     /* data width of a column, 0 for calc     */
	int current,	     /* start with this item		       */
	void (*formatitem)(),/* routine to display an item or NULL     */
	int (*showdetail)(), /* routine to display details or NULL     */
	char *speedstring,   /* returned speed key value, or NULL >[30]*/
	int (*speedprompt)(),/* routine to display prompt or NULL      */
	int (*checkkey)()    /* routine to check keystroke or NULL     */
)
   /* return is: n>=0 for choice n selected,
		 -1 for escape
		  k for checkkey routine nonzero return value k
      speedstring[0] != 0 on return if string is present
      */
{
static char far choiceinstr1a[]="Use the cursor keys to highlight your selection";
static char far choiceinstr1b[]="Use the cursor keys or type a value to make a selection";
static char far choiceinstr2a[]="Press ENTER for highlighted choice, or ESCAPE to back out";
static char far choiceinstr2b[]="Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help";
static char far choiceinstr2c[]="Press ENTER for highlighted choice, or F1 for help";
static char far choiceinstr3a[]="  Press F2 to show description of each item   ";
static char far choiceinstr3b[]="Press F2 to show choices only (no description)";

   int titlelines,titlewidth;
   int reqdrows;
   int topleftrow,topleftcol;
   int topleftchoice;
   int speedrow;  /* speed key prompt */
   int f2instrow; /* F2 toggle instructions row */
   int boxitems;  /* boxwidth*boxdepth */
   int curkey,increment,rev_increment;
   int redisplay;
   int i,j,k,color;
   char *charptr;
   char buf[81];
   int speed_match;
   char curitem[81];
   char *itemptr;
   int ret,savelookatmouse;

   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   ret = -1;
   if (speedstring
     && (i = strlen(speedstring)) > 0) { /* preset current to passed string */
      current = 0;
      while (current < numchoices
	&& (k = strncasecmp(speedstring,choices[current],i)) > 0)
	 ++current;
      if (k < 0 && current > 0)  /* oops - overshot */
	 --current;
      if (current >= numchoices) /* bumped end of list */
	 current = numchoices - 1;
      }

   while (1) {
      if (current >= numchoices)  /* no real choice in the list? */
	 goto fs_choice_end;
      if ((attributes[current] & 256) == 0)
	 break;
      ++current;		  /* scan for a real choice */
      }

   titlelines = titlewidth = 0;
   if (hdg) {
      charptr = hdg;		  /* count title lines, find widest */
      i = 0;
      titlelines = 1;
      while (*charptr) {
	 if (*(charptr++) == '\n') {
	    ++titlelines;
	    i = -1;
	    }
	 if (++i > titlewidth)
	    titlewidth = i;
	 }
      }

   if (colwidth == 0)		  /* find widest column */
      for (i = 0; i < numchoices; ++i)
	 if (strlen(choices[i]) > colwidth)
	    colwidth = strlen(choices[i]);

   /* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
   reqdrows = 3;		  /* calc rows available */
   if (hdg)
      reqdrows += titlelines + 1;
   if (instr) { 		  /* count instructions lines */
      charptr = instr;
      ++reqdrows;
      while (*charptr)
	 if (*(charptr++) == '\n')
	    ++reqdrows;
      }
   else
      reqdrows += 2;		  /* standard instructions */
   if (showdetail) ++reqdrows;	  /* a row for extra instruction line */
   if (speedstring) ++reqdrows;   /* a row for speedkey prompt */
   if (boxdepth > (i = 25 - reqdrows)) /* limit the depth to max */
      boxdepth = i;
   if (boxwidth == 0) { 	  /* pick box width and depth */
      if (numchoices <= i - 2) {  /* single column is 1st choice if we can */
	 boxdepth = numchoices;
	 boxwidth = 1;
	 }
      else {			  /* sort-of-wide is 2nd choice */
	 boxwidth = 60 / (colwidth + 1);
	 if (boxwidth == 0
	   || (boxdepth = (numchoices+boxwidth-1)/boxwidth) > i - 2) {
	    boxwidth = 80 / (colwidth + 1); /* last gasp, full width */
	    if ((boxdepth = (numchoices+boxwidth-1)/boxwidth) > i)
	       boxdepth = i;
	    }
	 }
      }
   if ((i = 77 / boxwidth - colwidth) > 3) /* spaces to add @ left each choice */
      i = 3;
   j = boxwidth * (colwidth += i) + i;	   /* overall width of box */
   if (j < titlewidth+2)
      j = titlewidth + 2;
   if (j > 80)
      j = 80;
   if (j <= 70 && boxwidth == 2) {	   /* special case makes menus nicer */
      ++j;
      ++colwidth;
      }
   k = (80 - j) / 2;			   /* center the box */
   k -= (90 - j) / 20;
   topleftcol = k + i;			   /* column of topleft choice */
   i = (25 - reqdrows - boxdepth) / 2;
   i -= i / 4;				   /* higher is better if lots extra */
   topleftrow = 3 + titlelines + i;	   /* row of topleft choice */

   /* now set up the overall display */
   helptitle(); 			   /* clear, display title line */
   setattr(1,0,C_PROMPT_BKGRD,24*80);	   /* init rest to background */
   for (i = topleftrow-1-titlelines; i < topleftrow+boxdepth+1; ++i)
      setattr(i,k,C_PROMPT_LO,j);	   /* draw empty box */
   if (hdg) {
      textcbase = (80 - titlewidth) / 2;   /* set left margin for putstring */
      textcbase -= (90 - titlewidth) / 20; /* put heading into box */
      putstring(topleftrow-titlelines-1,0,C_PROMPT_HI,hdg);
      textcbase = 0;
      }
   if (hdg2)				   /* display 2nd heading */
      putstring(topleftrow-1,topleftcol,C_PROMPT_MED,hdg2);
   i = topleftrow + boxdepth + 1;
   if (instr == NULL) { 	      /* display default instructions */
      if (i < 20) ++i;
      if (speedstring) {
	 speedrow = i;
	 *speedstring = 0;
	 if (++i < 22) ++i;
	 }
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
	    (speedstring) ? choiceinstr1b : choiceinstr1a);
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
	    (options&CHOICEMENU) ? choiceinstr2c
	    : ((options&CHOICEHELP) ? choiceinstr2b : choiceinstr2a));
      if (showdetail)
	 putstringcenter((f2instrow=i),0,80,C_PROMPT_BKGRD,choiceinstr3a);
      }
   else {				   /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)))
	 if (buf[j] == '\n') {
	    buf[j] = 0;
	    putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
	    j = -1;
	    }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }

   boxitems = boxwidth * boxdepth;
   topleftchoice = 0;			   /* pick topleft for init display */
   while (current - topleftchoice >= boxitems
     || (current - topleftchoice > boxitems/2
	 && topleftchoice + boxitems < numchoices))
      topleftchoice += boxwidth;
   redisplay = 1;

   while (1) { /* main loop */

      if (redisplay) {			     /* display the current choices */
	 if ((options & CHOICEMENU) == 0) {
	    memset(buf,' ',80);
	    buf[boxwidth*colwidth] = 0;
	    for (i = (hdg2) ? 0 : -1; i <= boxdepth; ++i)  /* blank the box */
	       putstring(topleftrow+i,topleftcol,C_PROMPT_LO,buf);
	    }
	 for (i = 0; i+topleftchoice < numchoices && i < boxitems; ++i) {
	    /* display the choices */
	    if ((k = attributes[j = i+topleftchoice] & 3) == 1)
	       k = C_PROMPT_LO;
	    else if (k == 3)
	       k = C_PROMPT_HI;
	    else
	       k = C_PROMPT_MED;
	    if (formatitem)
	       (*formatitem)(j,charptr=buf);
	    else
	       charptr = choices[j];
	    putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
		      k,charptr);
	    }
	 /***
	 ... format differs for summary/detail, whups, force box width to
	 ...  be 72 when detail toggle available?  (2 grey margin each
	 ...  side, 1 blue margin each side)
	 ***/
	 if (topleftchoice > 0 && hdg2 == NULL)
	    putstring(topleftrow-1,topleftcol,C_PROMPT_LO,"(more)");
	 if (topleftchoice + boxitems < numchoices)
	    putstring(topleftrow+boxdepth,topleftcol,C_PROMPT_LO,"(more)");
	 redisplay = 0;
	 }

      i = current - topleftchoice;	     /* highlight the current choice */
      if (formatitem)
	 (*formatitem)(current,itemptr=curitem);
      else
	 itemptr = choices[current];
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
		C_CHOICE_CURRENT,itemptr);

      if (speedstring) {		     /* show speedstring if any */
	 memset(buf,' ',80);
	 buf[80] = 0;
	 putstring(speedrow,0,C_PROMPT_BKGRD,buf);
	 if (*speedstring) {		     /* got a speedstring on the go */
	    putstring(speedrow,15,C_CHOICE_SP_INSTR," ");
	    if (speedprompt)
	       j = speedprompt(speedrow,16,C_CHOICE_SP_INSTR,speedstring,speed_match);
	    else {
	       putstring(speedrow,16,C_CHOICE_SP_INSTR,speed_prompt);
	       j = strlen(speed_prompt);
	       }
	    strcpy(buf,speedstring);
	    i = strlen(buf);
	    while (i < 30)
	       buf[i++] = ' ';
	    buf[i] = 0;
	    putstring(speedrow,16+j,C_CHOICE_SP_INSTR," ");
	    putstring(speedrow,17+j,C_CHOICE_SP_KEYIN,buf);
	    movecursor(speedrow,17+j+strlen(speedstring));
	    }
	 else
	    movecursor(25,80);
	 }
      else
	 movecursor(25,80);

      while (!keypressed()) { }
      curkey = getakey();

      i = current - topleftchoice;	     /* unhighlight current choice */
      if ((k = attributes[current] & 3) == 1)
	 k = C_PROMPT_LO;
      else if (k == 3)
	 k = C_PROMPT_HI;
      else
	 k = C_PROMPT_MED;
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
		k,itemptr);

      increment = 0;
      switch (curkey) { 		     /* deal with input key */
	 case ENTER:
	 case ENTER_2:
	    ret = current;
	    goto fs_choice_end;
	 case ESC:
	    goto fs_choice_end;
	 case DOWN_ARROW:
	 case DOWN_ARROW_2:
	    rev_increment = 0 - (increment = boxwidth);
	    break;
	 case UP_ARROW:
	 case UP_ARROW_2:
	    increment = 0 - (rev_increment = boxwidth);
	    break;
	 case RIGHT_ARROW:
	 case RIGHT_ARROW_2:
	    if (boxwidth == 1) break;
	    increment = 1; rev_increment = -1;
	    break;
	 case LEFT_ARROW:
	 case LEFT_ARROW_2:
	    if (boxwidth == 1) break;
	    increment = -1; rev_increment = 1;
	    break;
	 case PAGE_UP:
	    if (numchoices > boxitems) {
	       topleftchoice -= boxitems;
	       increment = -boxitems;
	       rev_increment = boxwidth;
	       redisplay = 1;
	       }
	    break;
	 case PAGE_DOWN:
	    if (numchoices > boxitems) {
	       topleftchoice += boxitems;
	       increment = boxitems;
	       rev_increment = -boxwidth;
	       redisplay = 1;
	       }
	    break;
	 case CTL_HOME:
	    current = -1;
	    increment = rev_increment = 1;
	    break;
	 case CTL_END:
	    current = numchoices;
	    increment = rev_increment = -1;
	    break;
	 case F2:
	    /***
	    ... if detail toggle avail, do it, incl changing instr at bottom
	    ***/
	 default:
	    if (checkkey && (ret = (*checkkey)(curkey)))
	       goto fs_choice_end;
	    ret = -1;
	    if (speedstring) {
	       i = strlen(speedstring);
	       if (curkey == 8 && i > 0) /* backspace */
		  speedstring[--i] = 0;
	       if (33 <= curkey && curkey <= 126 && i < 30) {
		  curkey = tolower(curkey);
		  speedstring[i] = curkey;
		  speedstring[++i] = 0;
		  }
	       if (i > 0) {		 /* locate matching type */
		  current = 0;
		  while (current < numchoices
		    && (speed_match = strncasecmp(speedstring,choices[current],i)) > 0)
		     ++current;
		  if (speed_match < 0 && current > 0)  /* oops - overshot */
		     --current;
		  if (current >= numchoices) /* bumped end of list */
		     current = numchoices - 1;
		  }
	       }
	    break;
	 }

      if (increment) {			/* apply cursor movement */
	 current += increment;
	 if (speedstring)		/* zap speedstring */
	    speedstring[0] = 0;
	 }
      while (1) {			/* adjust to a non-comment choice */
	 if (current < 0 || current >= numchoices)
	     increment = rev_increment;
	 else if ((attributes[current] & 256) == 0)
	     break;
	 current += increment;
	 }
      if (topleftchoice > numchoices - boxitems)
	 topleftchoice = ((numchoices+boxwidth-1)/boxwidth)*boxwidth - boxitems;
      if (topleftchoice < 0)
	 topleftchoice = 0;
      while (current < topleftchoice) {
	 topleftchoice -= boxwidth;
	 redisplay = 1;
	 }
      while (current >= topleftchoice + boxitems) {
	 topleftchoice += boxwidth;
	 redisplay = 1;
	 }
      }

fs_choice_end:
   lookatmouse = savelookatmouse;
   return(ret);

}


/* case independent version of strncmp */
int strncasecmp(char *s,char *t,int ct)
{
   for(; (tolower(*s) == tolower(*t)) && --ct ; s++,t++)
      if(*s == '\0')
	 return(0);
   return(tolower(*s) - tolower(*t));
}


static int menutype;
#define MENU_HDG 3
#define MENU_ITEM 1

int main_menu(int fullmenu)
{
   char *choices[44]; /* 2 columns * 22 rows */
   int attributes[44];
   int choicekey[44];
   int i,j;
   int nextleft,nextright;
   int oldtabmode,oldhelpmode;
   oldtabmode = tabmode;
   oldhelpmode = helpmode;
top:
   menutype = fullmenu;
   tabmode = 0;
   for (i = 0; i < 44; ++i) {
      attributes[i] = 256;
      choices[i] = "";
      choicekey[i] = -1;
   }
   nextleft = -2;
   nextright = -1;

   if (fullmenu) {
      choices[nextleft+=2] = "      CURRENT IMAGE";
      attributes[nextleft] = 256+MENU_HDG;
      choicekey[nextleft+=2] = 13; /* enter */
      attributes[nextleft] = MENU_ITEM;
      if (calc_status == 2)
	 choices[nextleft] = "continue calculation";
      else
	 choices[nextleft] = "return to image";
      choicekey[nextleft+=2] = 9; /* tab */
      attributes[nextleft] = MENU_ITEM;
      choices[nextleft] = "info about image      <tab>";
      choicekey[nextleft+=2] = -10;
      attributes[nextleft] = MENU_ITEM;
      choices[nextleft] = "zoom box functions...";
      nextleft+=2;
   }
   choices[nextleft+=2] = "      NEW IMAGE";
   attributes[nextleft] = 256+MENU_HDG;
   choicekey[nextleft+=2] = -11;
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "select video mode...";
   choicekey[nextleft+=2] = 't';
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "select fractal type    <t>";
   if (fullmenu) {
      if ((fractalspecific[fractype].tojulia != NOFRACTAL
	  && param[0] == 0.0 && param[1] == 0.0)
	  || fractalspecific[fractype].tomandel != NOFRACTAL) {
	 choicekey[nextleft+=2] = ' ';
	 attributes[nextleft] = MENU_ITEM;
	 choices[nextleft] = "toggle to/from julia <space>";
      }
      choicekey[nextleft+=2] = '\\';
      attributes[nextleft] = MENU_ITEM;
      choices[nextleft] = "return to prior image  <\\>";
   }
   nextleft += 2;
   choices[nextleft+=2] = "      OPTIONS";
   attributes[nextleft] = 256+MENU_HDG;
   choicekey[nextleft+=2] = 'x';
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "basic options...       <x>";
   choicekey[nextleft+=2] = 'y';
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "extended doodads...    <y>";
   choicekey[nextleft+=2] = 'v';
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "view window options... <v>";
   choicekey[nextleft+=2] = 'i';
   attributes[nextleft] = MENU_ITEM;
   choices[nextleft] = "IFS and 3D parameters  <i>";

   choices[nextright+=2] = "        FILE";
   attributes[nextright] = 256+MENU_HDG;
   if (fullmenu) {
      choicekey[nextright+=2] = 's';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "save image to file       <s>";
   }
   choicekey[nextright+=2] = 'r';
   attributes[nextright] = MENU_ITEM;
   choices[nextright] = "load image from file...  <r>";
   choicekey[nextright+=2] = '3';
   attributes[nextright] = MENU_ITEM;
   choices[nextright] = "3d transform from file...<3>";
   if (fullmenu) {
      choicekey[nextright+=2] = 'o';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "3d overlay from file...  <o>";
      choicekey[nextright+=2] = 'b';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "write batch parameters   <b>";
      choicekey[nextright+=2] = 'p';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "print image              <p>";
   }
   choicekey[nextright+=2] = 'd';
   attributes[nextright] = MENU_ITEM;
   choices[nextright] = "shell to dos             <d>";
   choicekey[nextright+=2] = 27;
   attributes[nextright] = MENU_ITEM;
   choices[nextright] = "quit Fractint           <esc>";
   choicekey[nextright+=2] = 1082;
   attributes[nextright] = MENU_ITEM;
   choices[nextright] = "restart Fractint        <ins>";
   if (fullmenu && dacbox[0][0] != 255 && colors >= 16) {
      nextright += 2;
      choices[nextright+=2] = "       COLORS";
      attributes[nextright] = 256+MENU_HDG;
      /*** choicekey[nextright+=2] = -12;
	    attributes[nextright] = MENU_ITEM;
	    choices[nextright] = "color cycling menu...";
	    choicekey[nextright+=2] = -13;
	    attributes[nextright] = MENU_ITEM;
	    choices[nextright] = "palette editing menu..."; ***/
      choicekey[nextright+=2] = 'c';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "color cycling mode       <c>";
      choicekey[nextright+=2] = '+';
      attributes[nextright] = MENU_ITEM;
      choices[nextright] = "rotate palette      <+>, <->";
      if (colors > 16) {
	 if (!reallyega) {
	    choicekey[nextright+=2] = 'e';
	    attributes[nextright] = MENU_ITEM;
	    choices[nextright] = "palette editing mode     <e>";
	 }
	 choicekey[nextright+=2] = 'a';
	 attributes[nextright] = MENU_ITEM;
	 choices[nextright] = "make starfield           <a>";
      }
   }

   i = (keypressed()) ? getakey() : 0;
   if (menu_checkkey(i) == 0) {
      helpmode = HELPMAIN;	   /* switch help modes */
      if ((nextleft += 2) < nextright)
	 nextleft = nextright + 1;
      i = fullscreen_choice(CHOICEMENU,"          MAIN MENU\n"
				       "(<Esc> returns here from image)",
      NULL,NULL,nextleft,choices,attributes,
	  2,nextleft/2,29,0,NULL,NULL,NULL,NULL,menu_checkkey);
      if (i == -1)     /* escape */
	 i = 27;
      else if (i < 0)
	 i = 0 - i;
      else {			  /* user selected a choice */
	 i = choicekey[i];
	 switch (i) {		  /* check for special cases */
	    case -10:		  /* zoombox functions */
	       helpmode = HELPZOOM;
	       help();
	       i = 0;
	       break;
	    case -11:		  /* select a video mode */
	       i = select_video_mode();
	       break;
	    }
	 }
      if (i == 27) {		  /* escape from menu exits Fractint */
	 static char far s[] = "Exit from Fractint (y/n)? y";
	 helptitle();
	 setattr(1,0,C_GENERAL_MED,24*80);
	 for (i = 9; i <= 11; ++i)
	   setattr(i,18,C_GENERAL_INPUT,40);
	 putstringcenter(10,18,40,C_GENERAL_INPUT,s);
	 movecursor(25,80);
	 while ((i = getakey()) != 'y' && i != 'Y' && i != 13) {
	    if (i == 'n' || i == 'N')
	       goto top;
	    }
	 goodbye();
	 }
   }
   if (i == 27) 		  /* escape from menu exits Fractint */
      goodbye();
   if (i == 9) {
      tab_display();
      i = 0;
   }
   if (i == 13 || i == 1013)
      i = 0;		     /* don't trigger new calc */
   tabmode = oldtabmode;
   helpmode = oldhelpmode;
   return(i);
}

static int menu_checkkey(int curkey)
{
   int testkey, k;
   testkey = (curkey>='A' && curkey<='Z') ? curkey+('a'-'A') : curkey;
   if (strchr("txyvir3d",testkey) || testkey == 1082)
      return(0-testkey);
   if (testkey == 27)
      return(-27);
   if (menutype) {
      if (strchr("\\sobp",testkey) || testkey == 9)
	 return(0-testkey);
      if (testkey == ' ')
	 if ((fractalspecific[fractype].tojulia != NOFRACTAL
	      && param[0] == 0.0 && param[1] == 0.0)
	   || fractalspecific[fractype].tomandel != NOFRACTAL)
	 return(0-testkey);
      if (dacbox[0][0] != 255 && colors >= 16) {
	 if (strchr("c+-",testkey))
	    return(0-testkey);
	 if (colors > 16
	   && (testkey == 'a' || (!reallyega && testkey == 'e')))
	    return(0-testkey);
	 }
      }
   for (k = 0; k < maxvideomode; k++) /* search for an adapter */
      if (testkey == kbdkeys[k])
	 return(0-testkey);
   return(0);
}


int input_field(
	int options,	      /* &1 numeric value */
	int attr,	      /* display attribute */
	char *fld,	      /* the field itself */
	int len,	      /* field length (declare as 1 larger for \0) */
	int row,	      /* display row */
	int col,	      /* display column */
	int (*checkkey)(int)  /* routine to check non data keys, or NULL */
	)
{
   char savefld[81];
   char buf[81];
   int insert, started, offset, curkey, display;
   int i, j;
   int ret,savelookatmouse;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   ret = -1;
   strcpy(savefld,fld);
   insert = started = offset = 0;
   display = 1;
   while (1) {
      strcpy(buf,fld);
      i = strlen(buf);
      while (i < len)
	 buf[i++] = ' ';
      buf[len] = 0;
      if (display) {				    /* display current value */
	 putstring(row,col,attr,buf);
	 display = 0;
	 }
      curkey = keycursor(row+insert,col+offset);  /* get a keystroke */
      switch (curkey) {
	 case ENTER:
	 case ENTER_2:
	    ret = 0;
	    goto inpfld_end;
	 case ESC:
	    goto inpfld_end;
	 case RIGHT_ARROW:
	 case RIGHT_ARROW_2:
	    if (offset < len) ++offset;
	    started = 1;
	    break;
	 case LEFT_ARROW:
	 case LEFT_ARROW_2:
	    if (offset > 0) --offset;
	    started = 1;
	    break;
	 case HOME:
	    offset = 0;
	    started = 1;
	    break;
	 case END:
	    offset = strlen(fld);
	    started = 1;
	    break;
	 case 8:
	 case 127:				/* backspace */
	    if (offset > 0) {
	       j = strlen(buf);
	       for (i = offset-1; i < j; ++i)
		  fld[i] = fld[i+1];
	       --offset;
	       }
	    started = display = 1;
	    break;
	 case 1083:				/* delete */
	    j = strlen(buf);
	    for (i = offset; i < j; ++i)
	       fld[i] = fld[i+1];
	    started = display = 1;
	    break;
	 case 1082:				/* insert */
	    insert ^= 0x8000;
	    started = 1;
	    break;
	 case F5:
	    strcpy(fld,savefld);
	    insert = started = offset = 0;
	    display = 1;
	    break;
	 default:
	    if ( offset < len
	      && (insert == 0 || strlen(fld) < len || started == 0)
	      && (   ((options&1) == 0 && curkey >= 32 && curkey < 127)
		  || ((options&1) != 0
		      && (curkey == '-' || curkey == '+' || curkey == '.'
			  || (curkey >= '0' && curkey <= '9'))))
	      ) {
	       if (started == 0) /* first char is data, zap field */
		  fld[0] = 0;
	       if (insert) {
		  j = strlen(fld);
		  while (j >= offset) {
		     fld[j+1] = fld[j];
		     --j;
		     }
		  }
	       if (offset >= strlen(fld))
		  fld[offset+1] = 0;
	       fld[offset++] = curkey;
	       started = display = 1;
	       }
	    else if (checkkey && (ret = (*checkkey)(curkey)))
	       goto inpfld_end;
	 }
      }
inpfld_end:
   lookatmouse = savelookatmouse;
   return(ret);
}

int field_prompt(
	int options,	    /* &1 numeric value */
	char *hdg,	    /* heading, \n delimited lines */
	char *instr,	    /* additional instructions or NULL */
	char *fld,	    /* the field itself */
	int len,	    /* field length (declare as 1 larger for \0) */
	int (*checkkey)()   /* routine to check non data keys, or NULL */
	)
{
   char *charptr;
   int boxwidth,titlelines,titlecol,titlerow;
   int promptcol;
   int i,j;
   char buf[81];
   helptitle(); 			  /* clear screen, display title */
   setattr(1,0,C_PROMPT_BKGRD,24*80);	  /* init rest to background */
   charptr = hdg;			  /* count title lines, find widest */
   i = boxwidth = 0;
   titlelines = 1;
   while (*charptr) {
      if (*(charptr++) == '\n') {
	 ++titlelines;
	 i = -1;
	 }
      if (++i > boxwidth)
	 boxwidth = i;
      }
   if (len > boxwidth)
      boxwidth = len;
   i = titlelines + 4;			  /* total rows in box */
   titlerow = (25 - i) / 2;		  /* top row of it all when centered */
   titlerow -= titlerow / 4;		  /* higher is better if lots extra */
   titlecol = (80 - boxwidth) / 2;	  /* center the box */
   titlecol -= (90 - boxwidth) / 20;
   promptcol = titlecol - (boxwidth-len)/2;
   j = titlecol;			  /* add margin at each side of box */
   if ((i = (82-boxwidth)/4) > 3)
      i = 3;
   j -= i;
   boxwidth += i * 2;
   for (i = -1; i < titlelines+3; ++i)	  /* draw empty box */
      setattr(titlerow+i,j,C_PROMPT_LO,boxwidth);
   textcbase = titlecol;		  /* set left margin for putstring */
   putstring(titlerow,0,C_PROMPT_HI,hdg); /* display heading */
   textcbase = 0;
   i = titlerow + titlelines + 4;
   if (instr) { 			  /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)))
	 if (buf[j] == '\n') {
	    buf[j] = 0;
	    putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
	    j = -1;
	    }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }
   else 				  /* default instructions */
      putstringcenter(i,0,80,C_PROMPT_BKGRD,
	      "Press ENTER when finished (or ESCAPE to back out)");
   return(input_field(options,C_PROMPT_INPUT,fld,len,
		      titlerow+titlelines+1,promptcol,checkkey));
}


/* thinking(1,message):
      if thinking message not yet on display, it is displayed;
      otherwise the wheel is updated
      returns 0 to keep going, -1 if keystroke pending
   thinking(0,NULL):
      call this when thinking phase is done
   */

int thinking(int options,char *msg)
{
   static int thinkstate = -1;
   static char *wheel[] = {"-","\\","|","/"};
   static int thinkcol;
   char buf[81];
   if (options == 0) {
      if (thinkstate >= 0) {
	 thinkstate = -1;
	 unstackscreen();
	 }
      return(0);
      }
   if (thinkstate < 0) {
      stackscreen();
      thinkstate = 0;
      helptitle();
      strcpy(buf,"  ");
      strcat(buf,msg);
      strcat(buf,"    ");
      putstring(4,10,C_GENERAL_HI,buf);
      thinkcol = textcol - 3;
      }
   putstring(4,thinkcol,C_GENERAL_HI,wheel[thinkstate]);
   movecursor(25,80); /* turn off cursor */
   thinkstate = (thinkstate + 1) & 3;
   return (keypressed());
}


void clear_screen(void)  /* a stub for a windows only subroutine */
{
}


/* savegraphics/restoregraphics: video.asm subroutines */

static unsigned char far *swapsavebuf;
static unsigned int memhandle;
unsigned long swaptotlen;
unsigned long swapoffset;
unsigned char far *swapvidbuf;
int swaplength;
static int swaptype = -1;
static int swapblklen; /* must be a power of 2 */
extern void (*swapsetup)(void),swapnormread(void);
void movewords(int,unsigned char far*,unsigned char far*);
extern unsigned char suffix[4096];

extern	unsigned int emmallocate(unsigned int),xmmallocate(unsigned int);
extern	void   emmdeallocate(unsigned int),xmmdeallocate(unsigned int);
extern	void   emmgetpage(unsigned int, unsigned int);
extern	unsigned char far *emmquery(void);
extern	unsigned int *xmmquery(void);

int savegraphics()
{
   extern int made_dsktemp;
   int i;
   struct {
      unsigned long   len;
      unsigned int    sourcehandle;
      unsigned long   sourceptr;
      unsigned int    desthandle;
      unsigned long   destptr;
      } xmmparms;
   discardgraphics(); /* if any emm/xmm in use from prior call, release it */
   swaptotlen = (long)sxdots * sydots;
   i = colors;
   while (i <= 16) {
      swaptotlen >>= 1;
      i = i * i;
      }
   swapoffset = 0;
   if (debugflag != 420 && debugflag != 422 /* 422=xmm test, 420=disk test */
     && (swapsavebuf = emmquery()) != NULL
     && (memhandle = emmallocate((swaptotlen + 16383) >> 14)) != 0) {
      swaptype = 0; /* use expanded memory */
      swapblklen = 16384;
      }
   else if (debugflag != 420
     && xmmquery() !=0
     && (memhandle = xmmallocate((swaptotlen + 1023) >> 10)) != 0) {
      swaptype = 1; /* use extended memory */
      swapblklen = 16384;
      }
   else {
      swaptype = 2; /* use disk */
      swapblklen = 4096;
      if ((memhandle = open("FRACTINT.DSK",O_CREAT|O_WRONLY|O_BINARY,S_IWRITE))
	 == -1) {
dskfile_error:
	 setvideomode(3,0,0,0); /* text mode */
	 setclear();
	 printf("!!! error in temp file FRACTINT.DSK (disk full?) - aborted\n\n");
	 exit(1);
	 }
      made_dsktemp = 1;
      }
   while (swapoffset < swaptotlen) {
      swaplength = swapblklen;
      if ((swapoffset & (swapblklen-1)) != 0)
	 swaplength = swapblklen - (swapoffset & (swapblklen-1));
      if (swaplength > swaptotlen - swapoffset)
	 swaplength = swaptotlen - swapoffset;
      (*swapsetup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */
      switch(swaptype) {
	 case 0:
	    emmgetpage(swapoffset>>14,memhandle);
	    movewords(swaplength>>1,swapvidbuf,
		      swapsavebuf+(swapoffset&(swapblklen-1)));
	    break;
	 case 1:
	    xmmparms.len = swaplength;
	    xmmparms.sourcehandle = 0; /* Source is conventional memory */
	    xmmparms.sourceptr = (unsigned long)swapvidbuf;
	    xmmparms.desthandle = memhandle;
	    xmmparms.destptr = swapoffset;
	    xmmmoveextended(&xmmparms);
	    break;
	 default:
	    movewords(swaplength>>1,swapvidbuf,(unsigned char far *)suffix);
	    if (write(memhandle,suffix,swaplength) == -1)
	       goto dskfile_error;
	 }
      swapoffset += swaplength;
      }
   if (swaptype == 2)
      close(memhandle);
   return 0;
}

void restoregraphics()
{
   struct {
      unsigned long   len;
      unsigned int    sourcehandle;
      unsigned long   sourceptr;
      unsigned int    desthandle;
      unsigned long   destptr;
      } xmmparms;
   swapoffset = 0;
   if (swaptype == 2)
      memhandle = open("FRACTINT.DSK",O_RDONLY|O_BINARY,S_IREAD);
#ifdef __TURBOC__
   swapvidbuf = MK_FP(extraseg+0x1000,0); /* for swapnormwrite case */
#else
   FP_SEG(swapvidbuf)=extraseg+0x1000;
   FP_OFF(swapvidbuf)=0;
#endif
   while (swapoffset < swaptotlen) {
      swaplength = swapblklen;
      if ((swapoffset & (swapblklen-1)) != 0)
	 swaplength = swapblklen - (swapoffset & (swapblklen-1));
      if (swaplength > swaptotlen - swapoffset)
	 swaplength = swaptotlen - swapoffset;
      if (swapsetup != swapnormread)
	 (*swapsetup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */
      switch(swaptype) {
	 case 0:
	    emmgetpage(swapoffset>>14,memhandle);
	    movewords(swaplength>>1,swapsavebuf+(swapoffset&(swapblklen-1)),
		      swapvidbuf);
	    break;
	 case 1:
	    xmmparms.len = swaplength;
	    xmmparms.sourcehandle = memhandle;
	    xmmparms.sourceptr = swapoffset;
	    xmmparms.desthandle = 0; /* conventional memory */
	    xmmparms.destptr = (unsigned long)swapvidbuf;
	    xmmmoveextended(&xmmparms);
	    break;
	 default:
	    read(memhandle,suffix,swaplength);
	    movewords(swaplength>>1,(unsigned char far *)suffix,swapvidbuf);
	 }
      if (swapsetup == swapnormread)
	 swapnormwrite();
      swapoffset += swaplength;
      }
   if (swaptype == 2)
      close(memhandle);
   discardgraphics();
}

void discardgraphics() /* release expanded/extended memory if any in use */
{
   switch(swaptype) {
      case 0:
	 emmdeallocate(memhandle);
	 break;
      case 1:
	 xmmdeallocate(memhandle);
      }
   swaptype = -1;
   }

