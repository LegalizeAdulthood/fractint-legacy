/*
        Miscellaneous C routines used only in DOS Fractint.
*/

#include <string.h>
#include <stdio.h>
#ifndef XFRACT
#include <dos.h>
#include <io.h>
#include <process.h>
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"
#include "prototyp.h"

static int menu_checkkey(int curkey,int choice);

/* uncomment following for production version */
/* #define PRODUCTION */

int release=1930; /* this has 2 implied decimals; increment it every synch */
int patchlevel=0; /* patchlevel for DOS version */
#ifdef XFRACT
int xrelease=300;
#endif

/* fullscreen_choice options */
#define CHOICERETURNKEY 1
#define CHOICEMENU      2
#define CHOICEHELP      4
#define CHOICESCRUNCH   16
#define CHOICESNOTSORTED 32

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
#ifdef XFRACT
static char far s_errorstart[] = {"*** Error during startup:"};
#endif
static char far s_escape_cancel[] = {"Escape to cancel, any other key to continue..."};
static char far s_anykey[] = {"Any key to continue..."};
#ifndef PRODUCTION
static char far s_custom[] = {"Customized Version"};
static char far s_notpublic[] = {"Not for Public Release"};
#endif
int stopmsg (int flags, char far *msg)
{
   int ret,toprow,color,savelookatmouse;
   static unsigned char batchmode = 0;
   if(debugflag != 0 || initbatch >= 1)
   {
      static FILE *fp = NULL;
      if(fp==NULL && initbatch == 0)
         fp=dir_fopen(workdir,"stopmsg.txt","w");
      else
         fp=dir_fopen(workdir,"stopmsg.txt","a");
      if(fp != NULL)
#ifndef XRACT
      fprintf(fp,"%Fs\n",msg);
#else
      fprintf(fp,"%s\n",msg);
#endif
      fclose(fp);
   }
   if (active_system == 0 /* DOS */
     && first_init) {     /* & cmdfiles hasn't finished 1st try */
#ifdef XFRACT
      setvideotext();
      buzzer(2);
      putstring(0,0,15,s_errorstart);
      putstring(2,0,15,msg);
      movecursor(8,0);
      sleep(1);
      UnixDone();
      exit(1);
#else
      printf("%Fs\n",msg);
      dopause(1); /* pause deferred until after cmdfiles */
      return(0);
#endif
      }
   if (initbatch >= 1 || batchmode) { /* in batch mode */
      initbatch = 4; /* used to set errorlevel */
      batchmode = 1; /* fixes *second* stopmsg in batch mode bug */
      return (-1);
      }
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
      putstring(textrow+2,0,7,s_escape_cancel);
   else
      putstring(textrow+2,0,7,s_anykey);
   textcbase = 0; /* back to full line */
   color = (flags & 16) ? C_STOP_INFO : C_STOP_ERR;
   setattr(toprow,0,color,(textrow+1-toprow)*80);
   movecursor(25,80);   /* cursor off */
   if ((flags & 4) == 0)
      buzzer((flags & 16) ? 0 : 2);
   while (keypressed()) /* flush any keyahead */
      getakey();
   if(debugflag != 324)
      if (getakeynohelp() == ESC)
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
int texttempmsg(char far *msgparm)
{
   if (showtempmsg(msgparm))
      return(-1);
#ifndef XFRACT
   while (!keypressed()) ; /* wait for a keystroke but don't eat it */
#else
   waitkeypressed(0); /* wait for a keystroke but don't eat it */
#endif
   cleartempmsg();
   return(0);
}

void freetempmsg()
{
   if(temptextsave != NULL)
      farmemfree(temptextsave);
   temptextsave = NULL;
}

int showtempmsg(char far *msgparm)
{
   static long size = 0;
   char msg[41];
   BYTE buffer[640];
   char far *fartmp;
   BYTE far *fontptr;
   BYTE *bufptr;
   int i,j,k,fontchar,charnum;
   int xrepeat = 0;
   int yrepeat = 0;
   int save_sxoffs,save_syoffs;
   far_strncpy(msg,msgparm,40);
   msg[40] = 0; /* ensure max message len of 40 chars */
   if (dotmode == 11) { /* disk video, screen in text mode, easy */
      dvid_status(0,msg);
      return(0);
      }
   if (active_system == 0 /* DOS */
     && first_init) {     /* & cmdfiles hasn't finished 1st try */
      printf("%s\n",msg);
      return(0);
      }

   if ((fontptr = findfont(0)) == NULL) { /* old bios, no font table? */
      if (oktoprint == 0               /* can't printf */
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
   if(temptextsave != NULL)
      if(size != (long)textxdots * (long)textydots)
         freetempmsg();
   size = (long)textxdots * (long)textydots;
   save_sxoffs = sxoffs;
   save_syoffs = syoffs;
   sxoffs = syoffs = 0;
   if(temptextsave == NULL) /* only save screen first time called */
   {
      if ((temptextsave = farmemalloc(size)) == NULL)
         return(-1); /* sorry, message not displayed */
      fartmp = temptextsave;
      for (i = 0; i < textydots; ++i) {
         get_line(i,0,textxdots-1,buffer);
         for (j = 0; j < textxdots; ++j) /* copy it out to far memory */
                 *(fartmp++) = buffer[j];
         }
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
                     *bufptr = (BYTE)color_medium;
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
   BYTE buffer[640];
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
     if(using_jiim == 0)  /* jiim frees memory with freetempmsg() */
     {
         farmemfree(temptextsave);
         temptextsave = NULL;
      }
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
   char msg[80],buf[80];
   setclear(); /* clear the screen */
#ifdef XFRACT
   sprintf(msg,"XFRACTINT  Version %d.%02d (FRACTINT Version %d.%02d)",
           xrelease/100,xrelease%100, release/100,release%100);
   putstringcenter(0,0,80,C_TITLE,msg);
#else
   sprintf(msg,"FRACTINT Version %d.%01d",release/100,(release%100)/10);
   if (release%10) {
      sprintf(buf,"%01d",release%10);
      strcat(msg,buf);
      }
#ifndef XFRACT
   if (patchlevel) {
      sprintf(buf," Patch %d",patchlevel);
      strcat(msg,buf);
      }
#endif
   putstringcenter(0,0,80,C_TITLE,msg);
#endif
/* uncomment next for production executable: */
#ifdef PRODUCTION
    return;
   /*NOTREACHED*/
#else
   if (debugflag == 3002) return;
/* #define DEVELOPMENT */
#ifdef DEVELOPMENT
   putstring(0,2,C_TITLE_DEV,"Development Version");
#else
   putstring(0,3,C_TITLE_DEV, s_custom);
#endif
   putstring(0,55,C_TITLE_DEV,s_notpublic);
#endif
}


int putstringcenter(int row, int col, int width, int attr, char far *msg)
{
   char buf[81];
   int i,j,k;
   i = 0;
#ifdef XFRACT
   if (width==80) width=79; /* Some systems choke in column 80 */
#endif
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


#ifndef XFRACT
static int screenctr = -1;
#else
static int screenctr = 0;
#endif
#define MAXSCREENS 3
static BYTE far *savescreen[MAXSCREENS];
static int saverc[MAXSCREENS+1];
static FILE *savescf = NULL;
static char scsvfile[]="fractscr.tmp";

void stackscreen()
{
#ifndef XFRACT
   BYTE far *vidmem;
   int savebytes;
   int i;
   BYTE far *ptr;
   char buf[256];
   if(*s_makepar == 0)
      return;
   memcpy(buf,suffix,256);
   saverc[screenctr+1] = textrow*80 + textcol;
   if (++screenctr) { /* already have some stacked */
         static char far msg[]={"stackscreen overflow"};
      if ((i = screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
         stopmsg(1,msg);
         exit(1);
         }
      vidmem = MK_FP(textaddr,0);
      savebytes = (text_type == 0) ? 4000 : 16384;
      if ((ptr = savescreen[i] = farmemalloc((long)savebytes)) != NULL)
         far_memcpy(ptr,vidmem,savebytes);
      else {
         if (savescf == NULL) { /* create file just once */
            if ((savescf = dir_fopen(tempdir,scsvfile,"wb")) == NULL)
               goto fileproblem;
            if (fwrite(buf,MAXSCREENS,16384,savescf) != 16384)
               goto fileproblem;
            fclose(savescf);
            if ((savescf = dir_fopen(tempdir,scsvfile,"r+b")) == NULL) {
            static char far msg[]={"insufficient memory, aborting"};
fileproblem:   stopmsg(1,msg);
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
   memcpy(suffix,buf,256);
#else
   int i;
   BYTE far *ptr;
   saverc[screenctr+1] = textrow*80 + textcol;
   if (++screenctr) { /* already have some stacked */
         static char far msg[]={"stackscreen overflow"};
      if ((i = screenctr - 1) >= MAXSCREENS) { /* bug, missing unstack? */
         stopmsg(1,msg);
         exit(1);
         }
      if (ptr = savescreen[i] = farmemalloc(sizeof(int *)))
         savecurses(ptr);
      else {
         stopmsg(1,msg);
         exit(1);
        }
      setclear();
      }
   else
      setfortext();
#endif
}

void unstackscreen()
{
#ifndef XFRACT
   char far *vidmem;
   int savebytes;
   BYTE far *ptr;
   char buf[256];
   if(*s_makepar == 0)
      return;
   memcpy(buf,suffix,256);
   textrow = saverc[screenctr] / 80;
   textcol = saverc[screenctr] % 80;
   if (--screenctr >= 0) { /* unstack */
      vidmem = MK_FP(textaddr,0);
      savebytes = (text_type == 0) ? 4000 : 16384;
      if ((ptr = savescreen[screenctr]) != NULL) {
         far_memcpy(vidmem,ptr,savebytes);
         farmemfree(ptr);
         }
      else {
         fseek(savescf,(long)(savebytes*screenctr),SEEK_SET);
         while (--savebytes >= 0)
            *(vidmem++) = (BYTE)getc(savescf);
         }
      }
   else
      setforgraphics();
   movecursor(-1,-1);
   memcpy(suffix,buf,256);
#else
   BYTE far *ptr;
   textrow = saverc[screenctr] / 80;
   textcol = saverc[screenctr] % 80;
   if (--screenctr >= 0) { /* unstack */
      ptr = savescreen[screenctr];
      restorecurses(ptr);
      farmemfree(ptr);
      }
   else
      setforgraphics();
   movecursor(-1,-1);
#endif
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


/* ------------------------------------------------------------------------ */

char speed_prompt[]="Speed key string";

int fullscreen_choice(
    int options,                  /* &2 use menu coloring scheme            */
                                  /* &4 include F1 for help in instructions */
                                  /* &8 add caller's instr after normal set */
                                  /* &16 menu items up one line             */
    char far *hdg,                /* heading info, \n delimited             */
    char far *hdg2,               /* column heading or NULL                 */
    char far *instr,              /* instructions, \n delimited, or NULL    */
    int numchoices,               /* How many choices in list               */
    char far*far*choices,         /* array of choice strings                */
    int far *attributes,          /* &3: 0 normal color, 1,3 highlight      */
                                  /* &256 marks a dummy entry               */
    int boxwidth,                 /* box width, 0 for calc (in items)       */
    int boxdepth,                 /* box depth, 0 for calc, 99 for max      */
    int colwidth,                 /* data width of a column, 0 for calc     */
    int current,                  /* start with this item                   */
    void (*formatitem)(int,char*),/* routine to display an item or NULL     */
    char *speedstring,            /* returned speed key value, or NULL      */
    int (*speedprompt)(int,int,int,char *,int),/* routine to display prompt or NULL      */
    int (*checkkey)(int,int)      /* routine to check keystroke or NULL     */
)
    /* return is: n>=0 for choice n selected,
                  -1 for escape
                  k for checkkey routine return value k (if not 0 nor -1)
                  speedstring[0] != 0 on return if string is present
    */
{
static char far choiceinstr1a[]="Use the cursor keys to highlight your selection";
static char far choiceinstr1b[]="Use the cursor keys or type a value to make a selection";
static char far choiceinstr2a[]="Press ENTER for highlighted choice, or ESCAPE to back out";
static char far choiceinstr2b[]="Press ENTER for highlighted choice, ESCAPE to back out, or F1 for help";
static char far choiceinstr2c[]="Press ENTER for highlighted choice, or F1 for help";

   int titlelines,titlewidth;
   int reqdrows;
   int topleftrow,topleftcol;
   int topleftchoice;
   int speedrow = 0;  /* speed key prompt */
   int boxitems;      /* boxwidth*boxdepth */
   int curkey,increment,rev_increment = 0;
   int redisplay;
   int i,j,k = 0;
   char far *charptr;
   char buf[81];
   int speed_match = 0;
   char curitem[81];
   char far *itemptr;
   int ret,savelookatmouse;
   int scrunch;  /* scrunch up a line */

   if(options&CHOICESCRUNCH)
      scrunch = 1;
   else
      scrunch = 0;
   savelookatmouse = lookatmouse;
   lookatmouse = 0;
   ret = -1;
   if (speedstring
     && (i = strlen(speedstring)) > 0) { /* preset current to passed string */
      current = 0;
      if(options&CHOICESNOTSORTED)
      {
         while (current < numchoices
             && (k = strncasecmp(speedstring,choices[current],i)) != 0)
            ++current;
         if(k != 0)
            current = 0;
      }
      else
      {
         while (current < numchoices
             && (k = strncasecmp(speedstring,choices[current],i)) > 0)
            ++current;
         if (k < 0 && current > 0)  /* oops - overshot */
            --current;
      }
      if (current >= numchoices) /* bumped end of list */
         current = numchoices - 1;
   }

   for(;;) {
      if (current >= numchoices)  /* no real choice in the list? */
         goto fs_choice_end;
      if ((attributes[current] & 256) == 0)
         break;
      ++current;                  /* scan for a real choice */
      }

   titlelines = titlewidth = 0;
   if (hdg) {
      charptr = hdg;              /* count title lines, find widest */
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

   if (colwidth == 0)             /* find widest column */
      for (i = 0; i < numchoices; ++i)
      {
         int len;
         if ((len=far_strlen(choices[i])) > colwidth)
            colwidth = len;
      }
   /* title(1), blank(1), hdg(n), blank(1), body(n), blank(1), instr(?) */
   reqdrows = 3 - scrunch;                /* calc rows available */
   if (hdg)
      reqdrows += titlelines + 1;
   if (instr) {                   /* count instructions lines */
      charptr = instr;
      ++reqdrows;
      while (*charptr)
         if (*(charptr++) == '\n')
            ++reqdrows;
      if ((options & 8))          /* show std instr too */
         reqdrows += 2;
      }
   else
      reqdrows += 2;              /* standard instructions */
   if (speedstring) ++reqdrows;   /* a row for speedkey prompt */
   if (boxdepth > (i = 25 - reqdrows)) /* limit the depth to max */
      boxdepth = i;
   if (boxwidth == 0) {           /* pick box width and depth */
      if (numchoices <= i - 2) {  /* single column is 1st choice if we can */
         boxdepth = numchoices;
         boxwidth = 1;
         }
      else {                      /* sort-of-wide is 2nd choice */
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
   j = boxwidth * (colwidth += i) + i;     /* overall width of box */
   if (j < titlewidth+2)
      j = titlewidth + 2;
   if (j > 80)
      j = 80;
   if (j <= 70 && boxwidth == 2) {         /* special case makes menus nicer */
      ++j;
      ++colwidth;
      }
   k = (80 - j) / 2;                       /* center the box */
   k -= (90 - j) / 20;
   topleftcol = k + i;                     /* column of topleft choice */
   i = (25 - reqdrows - boxdepth) / 2;
   i -= i / 4;                             /* higher is better if lots extra */
   topleftrow = 3 + titlelines + i;        /* row of topleft choice */

   /* now set up the overall display */
   helptitle();                            /* clear, display title line */
   setattr(1,0,C_PROMPT_BKGRD,24*80);      /* init rest to background */
   for (i = topleftrow-1-titlelines; i < topleftrow+boxdepth+1; ++i)
      setattr(i,k,C_PROMPT_LO,j);          /* draw empty box */
   if (hdg) {
      textcbase = (80 - titlewidth) / 2;   /* set left margin for putstring */
      textcbase -= (90 - titlewidth) / 20; /* put heading into box */
      putstring(topleftrow-titlelines-1,0,C_PROMPT_HI,hdg);
      textcbase = 0;
      }
   if (hdg2)                               /* display 2nd heading */
      putstring(topleftrow-1,topleftcol,C_PROMPT_MED,hdg2);
   i = topleftrow + boxdepth + 1;
   if (instr == NULL || (options & 8)) {   /* display default instructions */
      if (i < 20) ++i;
      if (speedstring) {
         speedrow = i;
         *speedstring = 0;
         if (++i < 22) ++i;
         }
      i -= scrunch;
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
            (speedstring) ? choiceinstr1b : choiceinstr1a);
      putstringcenter(i++,0,80,C_PROMPT_BKGRD,
            (options&CHOICEMENU) ? choiceinstr2c
            : ((options&CHOICEHELP) ? choiceinstr2b : choiceinstr2a));
      }
   if (instr) {                            /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)) != 0)
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }

   boxitems = boxwidth * boxdepth;
   topleftchoice = 0;                      /* pick topleft for init display */
   while (current - topleftchoice >= boxitems
     || (current - topleftchoice > boxitems/2
         && topleftchoice + boxitems < numchoices))
      topleftchoice += boxwidth;
   redisplay = 1;
   topleftrow -= scrunch;
   for(;;) { /* main loop */

      if (redisplay) {                       /* display the current choices */
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
            {
               (*formatitem)(j,buf);
               charptr=buf;
            }
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

      i = current - topleftchoice;           /* highlight the current choice */
      if (formatitem)
      {
         (*formatitem)(current,curitem);
         itemptr=curitem;
      }
      else
         itemptr = choices[current];
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                C_CHOICE_CURRENT,itemptr);

      if (speedstring) {                     /* show speedstring if any */
         memset(buf,' ',80);
         buf[80] = 0;
         putstring(speedrow,0,C_PROMPT_BKGRD,buf);
         if (*speedstring) {                 /* got a speedstring on the go */
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

#ifndef XFRACT
      while (!keypressed()) { } /* enables help */
#else
      waitkeypressed(0); /* enables help */
#endif
      curkey = getakey();
#ifdef XFRACT
      if (curkey==F10) curkey=')';
      if (curkey==F9) curkey='(';
      if (curkey==F8) curkey='*';
#endif

      i = current - topleftchoice;           /* unhighlight current choice */
      if ((k = attributes[current] & 3) == 1)
         k = C_PROMPT_LO;
      else if (k == 3)
         k = C_PROMPT_HI;
      else
         k = C_PROMPT_MED;
      putstring(topleftrow+i/boxwidth,topleftcol+(i%boxwidth)*colwidth,
                k,itemptr);

      increment = 0;
      switch (curkey) {                      /* deal with input key */
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
         case HOME:
            current = -1;
            increment = rev_increment = 1;
            break;
         case CTL_END:
         case END:
            current = numchoices;
            increment = rev_increment = -1;
            break;
         default:
            if (checkkey) {
               if ((ret = (*checkkey)(curkey,current)) < -1 || ret > 0)
                  goto fs_choice_end;
               if (ret == -1)
                  redisplay = -1;
               }
            ret = -1;
            if (speedstring) {
               i = strlen(speedstring);
               if (curkey == 8 && i > 0) /* backspace */
                  speedstring[--i] = 0;
               if (33 <= curkey && curkey <= 126 && i < 30) {
                  curkey = tolower(curkey);
                  speedstring[i] = (char)curkey;
                  speedstring[++i] = 0;
                  }
               if (i > 0) {              /* locate matching type */
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

      if (increment) {                  /* apply cursor movement */
         current += increment;
         if (speedstring)               /* zap speedstring */
            speedstring[0] = 0;
         }
      for(;;) {                 /* adjust to a non-comment choice */
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

#if (_MSC_VER >= 700)
#pragma code_seg ("realdos1_text")     /* place following in an overlay */
#endif

/* squeeze space out of string */
char *despace(char *str)
{
      char *obuf, *nbuf;

      for (obuf = str, nbuf = str; *obuf && obuf; ++obuf)
      {
            if (!isspace(*obuf))
                  *nbuf++ = *obuf;
      }
      *nbuf = 0;
      return str;
}

#ifndef XFRACT
/* case independent version of strncmp */
int strncasecmp(char far *s,char far *t,int ct)
{
   for(; (tolower(*s) == tolower(*t)) && --ct ; s++,t++)
      if(*s == '\0')
         return(0);
   return(tolower(*s) - tolower(*t));
}
#endif

#define LOADPROMPTSCHOICES(X,Y)     {\
   static FCODE tmp[] = { Y };\
   choices[X]= (char far *)tmp;\
   }

static int menutype;
#define MENU_HDG 3
#define MENU_ITEM 1

int main_menu(int fullmenu)
{
   char far *choices[44]; /* 2 columns * 22 rows */
   int attributes[44];
   int choicekey[44];
   int i;
   int nextleft,nextright;
   int oldtabmode /* ,oldhelpmode */;
   static char far MAIN_MENU[] = {"MAIN MENU"};
   int showjuliatoggle;
   oldtabmode = tabmode;
   /* oldhelpmode = helpmode; */
top:
   menutype = fullmenu;
   tabmode = 0;
   showjuliatoggle = 0;
   for (i = 0; i < 44; ++i) {
      attributes[i] = 256;
      choices[i] = "";
      choicekey[i] = -1;
      }
   nextleft = -2;
   nextright = -1;

   if (fullmenu) {
      LOADPROMPTSCHOICES(nextleft+=2,"      CURRENT IMAGE");
      attributes[nextleft] = 256+MENU_HDG;
      choicekey[nextleft+=2] = 13; /* enter */
      attributes[nextleft] = MENU_ITEM;
      if (calc_status == 2)
      {
         LOADPROMPTSCHOICES(nextleft,"continue calculation");
      }
      else
      {
         LOADPROMPTSCHOICES(nextleft,"return to image");
      }
      choicekey[nextleft+=2] = 9; /* tab */
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"info about image      <tab>");
      choicekey[nextleft+=2] = -10;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"zoom box functions...");
      choicekey[nextleft+=2] = 'o';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"orbits window          <o>");
      if(!(fractype==JULIA || fractype==JULIAFP || fractype==INVERSEJULIA))
          nextleft+=2;
      }
   LOADPROMPTSCHOICES(nextleft+=2,"      NEW IMAGE");
   attributes[nextleft] = 256+MENU_HDG;
   choicekey[nextleft+=2] = DELETE;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"select video mode...  <del>");
   choicekey[nextleft+=2] = 't';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"select fractal type    <t>");
   if (fullmenu) {
      if ((curfractalspecific->tojulia != NOFRACTAL
          && param[0] == 0.0 && param[1] == 0.0)
          || curfractalspecific->tomandel != NOFRACTAL) {
             choicekey[nextleft+=2] = ' ';
             attributes[nextleft] = MENU_ITEM;
             LOADPROMPTSCHOICES(nextleft,"toggle to/from julia <space>");
             showjuliatoggle = 1;
          }
      if(fractype==JULIA || fractype==JULIAFP || fractype==INVERSEJULIA) {
             choicekey[nextleft+=2] = 'j';
             attributes[nextleft] = MENU_ITEM;
             LOADPROMPTSCHOICES(nextleft,"toggle to/from inverse <j>");
             showjuliatoggle = 1;
          }
      choicekey[nextleft+=2] = 'h';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"return to prior image  <h>");

      choicekey[nextleft+=2] = 8;
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"reverse thru history <ctl-h>");
   }
   else
      nextleft += 2;
   LOADPROMPTSCHOICES(nextleft+=2,"      OPTIONS");
   attributes[nextleft] = 256+MENU_HDG;
   choicekey[nextleft+=2] = 'x';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"basic options...       <x>");
   choicekey[nextleft+=2] = 'y';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"extended options...    <y>");
   choicekey[nextleft+=2] = 'z';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"type-specific parms... <z>");
   choicekey[nextleft+=2] = 'v';
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"view window options... <v>");
   if(showjuliatoggle == 0)
   {
      choicekey[nextleft+=2] = 'i';
      attributes[nextleft] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextleft,"fractal 3D parms...    <i>");
   }
   choicekey[nextleft+=2] = 2;
   attributes[nextleft] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextleft,"browse parms...      <ctl-b>");

   LOADPROMPTSCHOICES(nextright+=2,"        FILE");
   attributes[nextright] = 256+MENU_HDG;
   choicekey[nextright+=2] = '@';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"run saved command set... <@>");
   if (fullmenu) {
      choicekey[nextright+=2] = 's';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"save image to file       <s>");
      }
   choicekey[nextright+=2] = 'r';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"load image from file...  <r>");
   choicekey[nextright+=2] = '3';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"3d transform from file...<3>");
   if (fullmenu) {
      choicekey[nextright+=2] = '#';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"3d overlay from file.....<#>");
      choicekey[nextright+=2] = 'b';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"save current parameters..<b>");
      choicekey[nextright+=2] = 'p';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"print image              <p>");
      }
   choicekey[nextright+=2] = 'd';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"shell to dos             <d>");
   choicekey[nextright+=2] = 'g';
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"give command string      <g>");
   choicekey[nextright+=2] = ESC;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"quit Fractint           <esc>");
   choicekey[nextright+=2] = INSERT;
   attributes[nextright] = MENU_ITEM;
   LOADPROMPTSCHOICES(nextright,"restart Fractint        <ins>");
   if (fullmenu && gotrealdac && colors >= 16) {
      /* nextright += 2; */
      LOADPROMPTSCHOICES(nextright+=2,"       COLORS");
      attributes[nextright] = 256+MENU_HDG;
      choicekey[nextright+=2] = 'c';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"color cycling mode       <c>");
      choicekey[nextright+=2] = '+';
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,"rotate palette      <+>, <->");
      if (colors > 16) {
         if (!reallyega) {
            choicekey[nextright+=2] = 'e';
            attributes[nextright] = MENU_ITEM;
            LOADPROMPTSCHOICES(nextright,"palette editing mode     <e>");
            }
         choicekey[nextright+=2] = 'a';
         attributes[nextright] = MENU_ITEM;
         LOADPROMPTSCHOICES(nextright,"make starfield           <a>");
         }
      choicekey[nextright+=2] = 1;
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,   "ant automaton          <ctl-a>");

      choicekey[nextright+=2] = 19;
      attributes[nextright] = MENU_ITEM;
      LOADPROMPTSCHOICES(nextright,   "stereogram             <ctl-s>");
      }

   i = (keypressed()) ? getakey() : 0;
   if (menu_checkkey(i,0) == 0) {
      helpmode = HELPMAIN;         /* switch help modes */
      if ((nextleft += 2) < nextright)
         nextleft = nextright + 1;
      i = fullscreen_choice(CHOICEMENU+CHOICESCRUNCH,
          MAIN_MENU,
          NULL,NULL,nextleft,(char far * far *)choices,attributes,
          2,nextleft/2,29,0,NULL,NULL,NULL,menu_checkkey);
      if (i == -1)     /* escape */
         i = ESC;
      else if (i < 0)
         i = 0 - i;
      else {                      /* user selected a choice */
         i = choicekey[i];
         switch (i) {             /* check for special cases */
            case -10:             /* zoombox functions */
               helpmode = HELPZOOM;
               help(0);
               i = 0;
               break;
            }
         }
      }
   if (i == ESC) {             /* escape from menu exits Fractint */
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
   if (i == TAB) {
      tab_display();
      i = 0;
      }
   if (i == ENTER || i == ENTER_2)
      i = 0;                 /* don't trigger new calc */
   tabmode = oldtabmode;
   return(i);
}

#if (_MSC_VER >= 700)
#pragma code_seg ()         /* back to normal segment */
#endif

static int menu_checkkey(int curkey,int choice)
{ /* choice is dummy used by other routines called by fullscreen_choice() */
   int testkey;
   testkey = choice; /* for warning only */
   testkey = (curkey>='A' && curkey<='Z') ? curkey+('a'-'A') : curkey;
#ifdef XFRACT
   /* We use F2 for shift-@, annoyingly enough */
   if (testkey == F2) return(0-testkey);
#endif
   if(testkey == '2')
      testkey = '@';
   if (strchr("#@txyzgvir3dj",testkey) || testkey == INSERT
     || testkey == ESC || testkey == DELETE)
      return(0-testkey);
   if (menutype) {
      if (strchr("\\sobpkrh",testkey) || testkey == TAB)
         return(0-testkey);
      if (testkey == ' ')
         if ((curfractalspecific->tojulia != NOFRACTAL
              && param[0] == 0.0 && param[1] == 0.0)
           || curfractalspecific->tomandel != NOFRACTAL)
         return(0-testkey);
      if (gotrealdac && colors >= 16) {
         if (strchr("c+-",testkey))
            return(0-testkey);
         if (colors > 16
           && (testkey == 'a' || (!reallyega && testkey == 'e')))
            return(0-testkey);
         }
      /* Alt-A and Alt-S */
      if (testkey == 1030 || testkey == 1031 )
         return(0-testkey);
      }
   if (check_vidmode_key(0,testkey) >= 0)
      return(0-testkey);
   return(0);
}


int input_field(
        int options,          /* &1 numeric, &2 integer, &4 double */
        int attr,             /* display attribute */
        char *fld,            /* the field itself */
        int len,              /* field length (declare as 1 larger for \0) */
        int row,              /* display row */
        int col,              /* display column */
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
   for(;;) {
      strcpy(buf,fld);
      i = strlen(buf);
      while (i < len)
         buf[i++] = ' ';
      buf[len] = 0;
      if (display) {                                /* display current value */
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
            if (offset < len-1) ++offset;
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
         case 127:                              /* backspace */
            if (offset > 0) {
               j = strlen(fld);
               for (i = offset-1; i < j; ++i)
                  fld[i] = fld[i+1];
               --offset;
               }
            started = display = 1;
            break;
         case DELETE:                           /* delete */
            j = strlen(fld);
            for (i = offset; i < j; ++i)
               fld[i] = fld[i+1];
            started = display = 1;
            break;
         case INSERT:                           /* insert */
            insert ^= 0x8000;
            started = 1;
            break;
         case F5:
            strcpy(fld,savefld);
            insert = started = offset = 0;
            display = 1;
            break;
         default:
            if (nonalpha(curkey)) {
               if (checkkey && (ret = (*checkkey)(curkey)) != 0)
                  goto inpfld_end;
               break;                                /* non alphanum char */
               }
            if (offset >= len) break;                /* at end of field */
            if (insert && started && strlen(fld) >= (size_t)len)
               break;                                /* insert & full */
            if ((options & 1)
              && (curkey < '0' || curkey > '9')
              && curkey != '+' && curkey != '-') {
               if ((options & 2))
                  break;
               /* allow scientific notation, and specials "e" and "p" */
               if ( ((curkey != 'e' && curkey != 'E') || offset >= 18)
                 && ((curkey != 'p' && curkey != 'P') || offset != 0 )
                 && curkey != '.')
                  break;
               }
            if (started == 0) /* first char is data, zap field */
               fld[0] = 0;
            if (insert) {
               j = strlen(fld);
               while (j >= offset) {
                  fld[j+1] = fld[j];
                  --j;
                  }
               }
            if ((size_t)offset >= strlen(fld))
               fld[offset+1] = 0;
            fld[offset++] = (char)curkey;
            /* if "e" or "p" in first col make number e or pi */
            if ((options & 3) == 1) { /* floating point */
               double tmpd;
               int specialv;
               char tmpfld[30];
               specialv = 0;
               if (*fld == 'e' || *fld == 'E') {
                  tmpd = exp(1.0);
                  specialv = 1;
                  }
               if (*fld == 'p' || *fld == 'P') {
                  tmpd = atan(1.0) * 4;
                  specialv = 1;
                  }
               if (specialv) {
                  if ((options & 4) == 0)
                     roundfloatd(&tmpd);
                  sprintf(tmpfld,"%.15g",tmpd);
                  tmpfld[len-1] = 0; /* safety, field should be long enough */
                  strcpy(fld,tmpfld);
                  offset = 0;
                  }
               }
            started = display = 1;
         }
      }
inpfld_end:
   lookatmouse = savelookatmouse;
   return(ret);
}

int field_prompt(
        int options,        /* &1 numeric value, &2 integer */
        char far *hdg,      /* heading, \n delimited lines */
        char far *instr,    /* additional instructions or NULL */
        char *fld,          /* the field itself */
        int len,            /* field length (declare as 1 larger for \0) */
        int (*checkkey)(int)   /* routine to check non data keys, or NULL */
        )
{
   char far *charptr;
   int boxwidth,titlelines,titlecol,titlerow;
   int promptcol;
   int i,j;
   char buf[81];
   helptitle();                           /* clear screen, display title */
   setattr(1,0,C_PROMPT_BKGRD,24*80);     /* init rest to background */
   charptr = hdg;                         /* count title lines, find widest */
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
   i = titlelines + 4;                    /* total rows in box */
   titlerow = (25 - i) / 2;               /* top row of it all when centered */
   titlerow -= titlerow / 4;              /* higher is better if lots extra */
   titlecol = (80 - boxwidth) / 2;        /* center the box */
   titlecol -= (90 - boxwidth) / 20;
   promptcol = titlecol - (boxwidth-len)/2;
   j = titlecol;                          /* add margin at each side of box */
   if ((i = (82-boxwidth)/4) > 3)
      i = 3;
   j -= i;
   boxwidth += i * 2;
   for (i = -1; i < titlelines+3; ++i)    /* draw empty box */
      setattr(titlerow+i,j,C_PROMPT_LO,boxwidth);
   textcbase = titlecol;                  /* set left margin for putstring */
   putstring(titlerow,0,C_PROMPT_HI,hdg); /* display heading */
   textcbase = 0;
   i = titlerow + titlelines + 4;
   if (instr) {                           /* display caller's instructions */
      charptr = instr;
      j = -1;
      while ((buf[++j] = *(charptr++)) != 0)
         if (buf[j] == '\n') {
            buf[j] = 0;
            putstringcenter(i++,0,80,C_PROMPT_BKGRD,buf);
            j = -1;
            }
      putstringcenter(i,0,80,C_PROMPT_BKGRD,buf);
      }
   else                                   /* default instructions */
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
   static int count = 0;
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
      count = 0;
      }
   if ((count++)<100) {
       return 0;
   }
   count = 0;
   putstring(4,thinkcol,C_GENERAL_HI,wheel[thinkstate]);
   movecursor(25,80); /* turn off cursor */
   thinkstate = (thinkstate + 1) & 3;
   return (keypressed());
}


void clear_screen(void)  /* a stub for a windows only subroutine */
{
}


/* savegraphics/restoregraphics: video.asm subroutines */

static BYTE far *swapsavebuf;
static unsigned int memhandle;
unsigned long swaptotlen;
unsigned long swapoffset;
BYTE far *swapvidbuf;
int swaplength;
static int swaptype = -1;
static int swapblklen; /* must be a power of 2 */
#ifdef XFRACT
BYTE suffix[4096];
#endif

#ifndef XFRACT

int savegraphics()
{
   int i;
   struct XMM_Move   xmmparms;

   discardgraphics(); /* if any emm/xmm in use from prior call, release it */
   swaptotlen = (long)(vxdots > sxdots ? vxdots : sxdots) * (long)sydots;
   i = colors;
   while (i <= 16) {
      swaptotlen >>= 1;
      i = i * i;
      }
   swapoffset = 0;
   if (debugflag != 420 && debugflag != 422 /* 422=xmm test, 420=disk test */
     && (swapsavebuf = emmquery()) != NULL
     && (memhandle = emmallocate((unsigned int)((swaptotlen + 16383) >> 14)))
         != 0) {
      swaptype = 0; /* use expanded memory */
      swapblklen = 16384;
      }
   else if (debugflag != 420
     && xmmquery() != 0
     && (memhandle = xmmallocate((unsigned int)((swaptotlen + 1023) >> 10)))
         != 0) {
      swaptype = 1; /* use extended memory */
      swapblklen = 16384;
      }
   else {
      swaptype = 2; /* use disk */
      swapblklen = 4096;

   /* MCP 7-7-91, If 'memhandle' is an 'unsigned int', how is it ever going
      to be equal to -1?

      if ((memhandle = dir_open(tempdir,diskfilename,O_CREAT|O_WRONLY|O_BINARY,S_IWRITE))
         == -1) {
   */
      if ((memhandle = dir_open(tempdir,diskfilename,O_CREAT|O_WRONLY|O_BINARY,S_IWRITE))
         == 0xffff) {


dskfile_error:
         setvideotext(); /* text mode */
         setclear();
         {
            static char far s1[] = {"error in temp file "};
            static char far s2[] = { " (disk full?) - aborted\n\n"};
            printf("%Fs%s%Fs",s1,diskfilename,s2);
         }
         exit(1);
         }
      made_dsktemp = 1;
      }
   while (swapoffset < swaptotlen) {
      swaplength = swapblklen;
      if ((swapoffset & (swapblklen-1)) != 0)
         swaplength = (int)(swapblklen - (swapoffset & (swapblklen-1)));
      if ((unsigned long)swaplength > (swaptotlen - swapoffset))
         swaplength = (int)(swaptotlen - swapoffset);
      (*swapsetup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */
      switch(swaptype) {
         case 0:
            emmgetpage((unsigned int)(swapoffset>>14),memhandle);
            movewords(swaplength>>1,swapvidbuf,
                      swapsavebuf+(swapoffset&(swapblklen-1)));
            break;
         case 1:
            xmmparms.Length = swaplength;
            xmmparms.SourceHandle = 0; /* Source is conventional memory */
            xmmparms.SourceOffset = (unsigned long)swapvidbuf;
            xmmparms.DestHandle = memhandle;
            xmmparms.DestOffset = swapoffset;
            xmmmoveextended(&xmmparms);
            break;
         default:
            movewords(swaplength>>1,swapvidbuf,(BYTE far *)suffix);
            if (write(memhandle,suffix,swaplength) == -1)
               goto dskfile_error;
         }
      swapoffset += swaplength;
      }
   if (swaptype == 2)
      close(memhandle);
   return 0;
}

int restoregraphics()
{
   struct XMM_Move   xmmparms;

   if(swaptype == -1)
      return(-1);
   swapoffset = 0;
   if (swaptype == 2)
      memhandle = dir_open(tempdir,diskfilename,O_RDONLY|O_BINARY,S_IREAD);
   swapvidbuf = MK_FP(extraseg+0x1000,0); /* for swapnormwrite case */
   while (swapoffset < swaptotlen) {
      swaplength = swapblklen;
      if ((swapoffset & (swapblklen-1)) != 0)
         swaplength = (int)(swapblklen - (swapoffset & (swapblklen-1)));
      if ((unsigned long)swaplength > (swaptotlen - swapoffset))
         swaplength = (int)(swaptotlen - swapoffset);
      if (swapsetup != swapnormread)
         (*swapsetup)(); /* swapoffset,swaplength -> sets swapvidbuf,swaplength */
      switch(swaptype) {
         case 0:
            emmgetpage((unsigned int)(swapoffset>>14),memhandle);
            movewords(swaplength>>1,swapsavebuf+(swapoffset&(swapblklen-1)),
                      swapvidbuf);
            break;
         case 1:
            xmmparms.Length = swaplength;
            xmmparms.SourceHandle = memhandle;
            xmmparms.SourceOffset = swapoffset;
            xmmparms.DestHandle = 0; /* conventional memory */
            xmmparms.DestOffset = (unsigned long)swapvidbuf;
            xmmmoveextended(&xmmparms);
            break;
         default:
            read(memhandle,suffix,swaplength);
            movewords(swaplength>>1,(BYTE far *)suffix,swapvidbuf);
         }
      if (swapsetup == swapnormread)
         swapnormwrite();
      swapoffset += swaplength;
      }
   if (swaptype == 2)
      close(memhandle);
   discardgraphics();
   return(0);
}

#else
int savegraphics() {return 0;}
int restoregraphics() {return 0;}
#endif

void discardgraphics() /* release expanded/extended memory if any in use */
{
#ifndef XFRACT
   switch(swaptype) {
      case 0:
         emmdeallocate(memhandle);
         break;
      case 1:
         xmmdeallocate(memhandle);
      }
   swaptype = -1;
#endif
}

#if (_MSC_VER >= 700)
#pragma code_seg ("realdos1_text")     /* place following in an overlay */
#endif

VIDEOINFO *vidtbl;  /* temporarily loaded fractint.cfg info */
int vidtbllen;                 /* number of entries in above           */

int showvidlength()
{
   int sz;
   sz = (sizeof(VIDEOINFO)+sizeof(int))*MAXVIDEOMODES;
   return(sz);
}

int load_fractint_cfg(int options)
{
   /* Reads fractint.cfg, loading videoinfo entries into extraseg. */
   /* Sets vidtbl pointing to the loaded table, and returns the    */
   /* number of entries (also sets vidtbllen to this).             */
   /* Past vidtbl, cfglinenums are stored for update_fractint_cfg. */
   /* If fractint.cfg is not found or invalid, issues a message    */
   /* (first time the problem occurs only, and only if options is  */
   /* zero) and uses the hard-coded table.                         */

   FILE *cfgfile;
   VIDEOINFO *vident;
   int far *cfglinenums;
   int linenum;
   int i, j, keynum, ax, bx, cx, dx, dotmode, xdots, ydots, colors;
   int commas[10];
   int textsafe2;
   char tempstring[150];

   vidtbl = MK_FP(extraseg,0);
   cfglinenums = (int far *)(&vidtbl[MAXVIDEOMODES]);

#ifdef XFRACT
    badconfig = -1;
#endif

   if (badconfig)  /* fractint.cfg already known to be missing or bad */
      goto use_resident_table;

   findpath("fractint.cfg",tempstring);
   if (tempstring[0] == 0                            /* can't find the file */
     || (cfgfile = fopen(tempstring,"r")) == NULL)   /* can't open it */
      goto bad_fractint_cfg;

   vidtbllen = 0;
   linenum = 0;
   vident = vidtbl;
   while (vidtbllen < MAXVIDEOMODES
     && fgets(tempstring, 120, cfgfile)) {
      ++linenum;
      if (tempstring[0] == ';') continue;   /* comment line */
      tempstring[120] = 0;
      tempstring[strlen(tempstring)-1] = 0; /* zap trailing \n */
      memset(commas,0,20);
      i = j = -1;
      for(;;) {
         if (tempstring[++i] < ' ') {
            if (tempstring[i] == 0) break;
            tempstring[i] = ' '; /* convert tab (or whatever) to blank */
            }
         else if (tempstring[i] == ',' && ++j < 10) {
            commas[j] = i + 1;   /* remember start of next field */
            tempstring[i] = 0;   /* make field a separate string */
            }
         }
      keynum = check_vidmode_keyname(tempstring);
      sscanf(&tempstring[commas[1]],"%x",&ax);
      sscanf(&tempstring[commas[2]],"%x",&bx);
      sscanf(&tempstring[commas[3]],"%x",&cx);
      sscanf(&tempstring[commas[4]],"%x",&dx);
      dotmode     = atoi(&tempstring[commas[5]]);
      xdots       = atoi(&tempstring[commas[6]]);
      ydots       = atoi(&tempstring[commas[7]]);
      colors      = atoi(&tempstring[commas[8]]);
      textsafe2   = dotmode / 100;
      dotmode    %= 100;
      if (j != 9 ||
            keynum < 0 ||
            dotmode < 0 || dotmode > 30 ||
            textsafe2 < 0 || textsafe2 > 4 ||
            xdots < 160 || xdots > MAXPIXELS ||
            ydots < 160 || ydots > MAXPIXELS ||
            (colors != 0 && colors != 2 && colors != 4 && colors != 16 &&
             colors != 256)
           )
         goto bad_fractint_cfg;
      cfglinenums[vidtbllen] = linenum; /* for update_fractint_cfg */
      far_memcpy(vident->name,   (char far *)&tempstring[commas[0]],25);
      far_memcpy(vident->comment,(char far *)&tempstring[commas[9]],25);
      vident->name[25] = vident->comment[25] = 0;
      vident->keynum      = keynum;
      vident->videomodeax = ax;
      vident->videomodebx = bx;
      vident->videomodecx = cx;
      vident->videomodedx = dx;
      vident->dotmode     = textsafe2 * 100 + dotmode;
      vident->xdots       = xdots;
      vident->ydots       = ydots;
      vident->colors      = colors;
      ++vident;
      ++vidtbllen;
      }
   fclose(cfgfile);
   return (vidtbllen);

bad_fractint_cfg:
   badconfig = -1; /* bad, no message issued yet */
   if (options == 0)
      bad_fractint_cfg_msg();

use_resident_table:
   vidtbllen = 0;
   vident = vidtbl;
   for (i = 0; i < MAXVIDEOTABLE; ++i) {
      if (videotable[i].xdots) {
         far_memcpy((char far *)vident,(char far *)&videotable[i],
                    sizeof(*vident));
         ++vident;
         ++vidtbllen;
         }
      }
   return (vidtbllen);

}

void bad_fractint_cfg_msg()
{
static char far badcfgmsg[]={"\
File FRACTINT.CFG is missing or invalid.\n\
See Hardware Support and Video Modes in the full documentation for help.\n\
I will continue with only the built-in video modes available."};
   stopmsg(0,badcfgmsg);
   badconfig = 1; /* bad, message issued */
}

void load_videotable(int options)
{
   /* Loads fractint.cfg and copies the video modes which are */
   /* assigned to function keys into videotable.              */
   int keyents,i;
   load_fractint_cfg(options); /* load fractint.cfg to extraseg */
   keyents = 0;
   far_memset((char far *)videotable,0,sizeof(*vidtbl)*MAXVIDEOTABLE);
   for (i = 0; i < vidtbllen; ++i) {
      if (vidtbl[i].keynum > 0) {
         far_memcpy((char far *)&videotable[keyents],(char far *)&vidtbl[i],
                    sizeof(*vidtbl));
         if (++keyents >= MAXVIDEOTABLE)
            break;
         }
      }
}

int check_vidmode_key(int option,int k)
{
   int i;
   /* returns videotable entry number if the passed keystroke is a  */
   /* function key currently assigned to a video mode, -1 otherwise */
   if (k == 1400)              /* special value from select_vid_mode  */
      return(MAXVIDEOTABLE-1); /* for last entry with no key assigned */
   if (k != 0)
      if (option == 0) { /* check resident video mode table */
         for (i = 0; i < MAXVIDEOTABLE; ++i) {
            if (videotable[i].keynum == k)
               return(i);
            }
         }
      else { /* check full vidtbl */
         for (i = 0; i < vidtbllen; ++i) {
            if (vidtbl[i].keynum == k)
               return(i);
            }
         }
   return(-1);
}

int check_vidmode_keyname(char *kname)
{
   /* returns key number for the passed keyname, 0 if not a keyname */
   int i,keyset;
   keyset = 1058;
   if (*kname == 'S' || *kname == 's') {
      keyset = 1083;
      ++kname;
      }
   else if (*kname == 'C' || *kname == 'c') {
      keyset = 1093;
      ++kname;
      }
   else if (*kname == 'A' || *kname == 'a') {
      keyset = 1103;
      ++kname;
      }
   if (*kname != 'F' && *kname != 'f')
      return(0);
   if (*++kname < '1' || *kname > '9')
      return(0);
   i = *kname - '0';
   if (*++kname != 0 && *kname != ' ') {
      if (*kname != '0' || i != 1)
         return(0);
      i = 10;
      ++kname;
      }
   while (*kname)
      if (*(kname++) != ' ')
         return(0);
   if ((i += keyset) < 2)
      i = 0;
   return(i);
}

void vidmode_keyname(int k,char *buf)
{
   /* set buffer to name of passed key number */
   *buf = 0;
   if (k > 0) {
      if (k > 1103) {
         *(buf++) = 'A';
         k -= 1103;
         }
      else if (k > 1093) {
         *(buf++) = 'C';
         k -= 1093;
         }
      else if (k > 1083) {
         *(buf++) = 'S';
         k -= 1083;
         }
      else
         k -= 1058;
      sprintf(buf,"F%d",k);
      }
}

#if (_MSC_VER >= 700)
#pragma code_seg ()      /* back to normal segment */
#endif
