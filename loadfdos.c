/*
    loadfdos.c - subroutine of loadfile.c (read_overlay) which sets
		 up video (mode, screen size).
    This module is linked as an overlay, should only be called from loadfile.c

    This code was split to a separate module to isolate the DOS only aspects
    of loading an image.  get_video_mode should return with:
      return code 0 for ok, -1 for error or cancelled by user
      video parameters setup for the mainline, in the dos case this means
	setting initmode to video mode, based on this fractint.c will set up
	for and call setvideomode
      set viewwindow on if file going to be loaded into a view smaller than
	physical screen, in this case also set viewreduction, viewxdots,
	viewydots, and finalaspectratio
      set skipxdots and skipydots, to 0 if all pixels are to be loaded,
	to 1 for every 2nd pixel, 2 for every 3rd, etc

    In WinFract, at least initially, get_video_mode can do just the
    following:
      set overall image x & y dimensions (sxdots and sydots) to filexdots
	and fileydots (note that filecolors is the number of colors in the
	gif, not sure if that is of any use...)
      if current window smaller than new sxdots and sydots, use scroll bars,
	if larger perhaps reduce the window size? whatever
      set viewwindow to 0 (no need? it always is for now in windows vsn?)
      set finalaspectratio to .75 (ditto?)
      set skipxdots and skipydots to 0
      return 0

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fractint.h"

/* routines in this module	*/

int get_video_mode(struct fractal_info *);

static int    vidcompare(void const *,void const *);
static void   format_vid_inf(int i,char *err,char *buf);
static double vid_aspect(int tryxdots,int tryydots);
static void   format_item(int,char *);
static int    check_modekey(int);


extern int    kbdkeys[];
extern int    initmode; 		/* initial video mode	    */
extern int    askvideo;
extern int    hasconfig;		/* = 0 if 'fractint.cfg'    */
extern char   *fkeys[];
extern int    initbatch;		/* 1 if batch run (no kbd)  */
extern int    fractype; 		/* fractal type 	    */
extern int    viewwindow;		/* 0 for full screen, 1 for window */
extern float  viewreduction;		/* window auto-sizing */
extern float  finalaspectratio; 	/* for view shape and rotation */
extern int    viewxdots,viewydots;	/* explicit view sizing */
extern int    fileydots, filexdots, filecolors;
extern float  fileaspectratio;
extern int    skipxdots,skipydots;	/* for decoder, when reducing image */
extern char   readname[];		/* name of fractal input file */
extern int    save_system,save_release;
extern int    display3d;		/* 3D display flag: 0 = OFF */
int  fullscreen_choice(
	     int options, char *hdg, char *hdg2, char *instr, int numchoices,
	     char **choices, int *attributes, int boxwidth, int boxdepth,
	     int colwidth, int current,
	     void (*formatitem)(), int (*showdetail)(),
	     char *speedstring, int (*speedprompt)(), int (*checkkey)());


struct vidinf {
   int entnum;	   /* videoentry subscript */
   unsigned flags; /* flags for sort's compare, defined below */
   };
/* defines for flags; done this way instead of bit union to ensure ordering;
   these bits represent the sort sequence for video mode list */
#define VI_EXACT   512	/* unless the one and only exact match */
#define VI_DISK1   256	/* disk video and size not exact */
#define VI_SSMALL  128	/* screen smaller than file's screen */
#define VI_SBIG     64	/* screen bigger than file's screen */
#define VI_VSMALL   32	/* screen smaller than file's view */
#define VI_VBIG     16	/* screen bigger than file's view */
#define VI_CSMALL    8	/* mode has too few colors */
#define VI_CBIG      4	/* mode has excess colors */
#define VI_DISK2     2	/* disk video */
#define VI_ASPECT    1	/* aspect ratio bad */


static int vidcompare(void const *p1,void const *p2)
{
   struct vidinf *ptr1,*ptr2;
   ptr1 = (struct vidinf *)p1;
   ptr2 = (struct vidinf *)p2;
   if (ptr1->flags < ptr2->flags
     || (ptr1->flags == ptr2->flags && ptr1->entnum < ptr2->entnum))
      return(-1);
   return(1);
}

static void format_vid_inf(int i,char *err,char *buf)
{
   fromvideotable(i);
   sprintf(buf,"%-5s %-25s %-4s %4d %4d %3d %-25s",  /* 76 chars */
	   fkeys[i], videoentry.name, err, videoentry.xdots, videoentry.ydots,
	   videoentry.colors, videoentry.comment);
}

static double vid_aspect(int tryxdots,int tryydots)
{  /* calc resulting aspect ratio for specified dots in current mode */
   return (double)tryydots / (double)tryxdots
	* (double)videoentry.xdots / (double)videoentry.ydots
	* SCREENASPECT;
   }


static struct vidinf *vidptr;

int get_video_mode(struct fractal_info *info)
{
   int kbdchar;
   struct vidinf vid[MAXVIDEOMODES];
   int i,j;
   double ftemp,ftemp2;
   unsigned tmpflags;
   char hdg[360];
   char buf[81];
   int choicerow,topsubscr,displayflag;
   int tmpxdots,tmpydots;
   float tmpreduce;
   char *nameptr;
   int	attributes[MAXVIDEOMODES];
   extern char temp1[256];
   int oldhelpmode;

   initmode = -1;

   /* try to find exact match for vid mode */
   for (i = 0; i < maxvideomode; ++i) {
      fromvideotable(i);
      if (info->xdots == videoentry.xdots && info->ydots == videoentry.ydots
	&& filecolors == videoentry.colors
	&& info->videomodeax == videoentry.videomodeax
	&& info->videomodebx == videoentry.videomodebx
	&& info->videomodecx == videoentry.videomodecx
	&& info->videomodedx == videoentry.videomodedx
	&& info->dotmode     == videoentry.dotmode) {
	 initmode = i;
	 break;
	 }
      }

   if (initmode == -1) /* try to find very good match for vid mode */
      for (i = 0; i < maxvideomode; ++i) {
	 fromvideotable(i);
	 if (info->xdots == videoentry.xdots && info->ydots == videoentry.ydots
	   && filecolors == videoentry.colors) {
	    initmode = i;
	    break;
	    }
	 }

   /* setup table entry for each vid mode, flagged for how well it matches */
   for (i = 0; i < maxvideomode; ++i) {
      tmpflags = VI_EXACT;
      fromvideotable(i);
      if (info->xdots > videoentry.xdots || info->ydots > videoentry.ydots)
	 tmpflags |= VI_SSMALL;
      else if (info->xdots < videoentry.xdots || info->ydots < videoentry.ydots)
	 tmpflags |= VI_SBIG;
      if (filexdots > videoentry.xdots || fileydots > videoentry.ydots)
	 tmpflags |= VI_VSMALL;
      else if (filexdots < videoentry.xdots || fileydots < videoentry.ydots)
	 tmpflags |= VI_VBIG;
      if (filecolors > videoentry.colors)
	 tmpflags |= VI_CSMALL;
      if (filecolors < videoentry.colors)
	 tmpflags |= VI_CBIG;
      if (i == initmode)
	 tmpflags -= VI_EXACT;
      if (videoentry.dotmode == 11) {
	 tmpflags |= VI_DISK2;
	 if ((tmpflags & (VI_SBIG+VI_SSMALL+VI_VBIG+VI_VSMALL)) != 0)
	    tmpflags |= VI_DISK1;
	 }
      if (fileaspectratio != 0 && videoentry.dotmode != 11
	&& (tmpflags & VI_VSMALL) == 0) {
	 ftemp = vid_aspect(filexdots,fileydots);
	 if ( ftemp < fileaspectratio * 0.98
	   || ftemp > fileaspectratio * 1.02)
	    tmpflags |= VI_ASPECT;
	 }
      vid[i].entnum = i;
      vid[i].flags  = tmpflags;
      }

   if (initmode < 0 || (askvideo && !initbatch)) {
      /* no exact match or (askvideo=yes and batch=no), talk to user */

      qsort(vid,maxvideomode,sizeof(vid[0]),vidcompare); /* sort modes */

      for (i = 0; i < maxvideomode; ++i)
	 attributes[i] = 1;
      vidptr = &vid[0]; /* for format_item */

      /* format heading */
      sprintf(buf,"File: %-44s  %d x %d x %d",
	     readname,filexdots,fileydots,filecolors);
      if (info->info_id[0] == 'G')
	 strcpy(temp1,"      Non-fractal GIF");
      else {
	 nameptr = fractalspecific[fractype].name;
	 if (*nameptr == '*') ++nameptr;
	 if (display3d) nameptr = "3D Transform";
	 sprintf(temp1,"Type: %s",nameptr);
	 }
      sprintf(hdg,"%-76s\n%-52s",buf,temp1);
      if (info->info_id[0] != 'G') {
	 if (save_system)
	    strcat(hdg,"WinFract ");
	 sprintf(buf,"v%d.%01d",save_release/100,(save_release%100)/10);
	 if (save_release%100) {
	    i = strlen(buf);
	    buf[i] = (save_release%10) + '0';
	    buf[i+1] = 0;
	    }
	 if (save_system == 0 && save_release <= 1410)
	    strcat(buf," or earlier");
	 strcat(hdg,buf);
	 }
      strcat(hdg,"\n");
      if (info->info_id[0] != 'G' && save_system == 0)
	 if (initmode < 0)
	    strcat(hdg,"Saved in unknown video mode.");
	 else {
	    format_vid_inf(initmode,"",buf);
	    strcat(hdg,buf);
	    }
      if (fileaspectratio != 0 && fileaspectratio != SCREENASPECT)
	 strcat(hdg,
 "\nWARNING: non-standard aspect ratio; loading will change your <v>iew settings");
      if (hasconfig == 0)
	 strcat(hdg,
	  "\n   Note: fractint.cfg is present and being used for mode list.");
      strcat(hdg,"\n");
      /* set up instructions */
      strcpy(temp1,"\
Select a video mode.  Use the cursor keypad to move the pointer.\n\
Press ENTER for selected mode, or use a video mode function key.\n\
Press F1 for help, ");
      if (info->info_id[0] != 'G')
	 strcat(temp1,"TAB for fractal information, ");
      strcat(temp1,"ESCAPE to back out.");

      oldhelpmode = helpmode;
      helpmode = HELPLOADFILE;
      i = fullscreen_choice(0,hdg,
 "key...name......................err..xdot.ydot.clr.comment..................",
		  temp1,maxvideomode,NULL,attributes,
		  1,12,76,0,format_item,NULL,NULL,NULL,check_modekey);
      helpmode = oldhelpmode;
      if (i == -1)
	 return(-1);
      if (i < 0) /* returned -100-vidtablenum */
	 initmode = -100 - i;
      else
	 initmode = vid[i].entnum;
      }

   /* ok, we're going to return with a video mode */

   fromvideotable(initmode);

   skipxdots = skipydots = 0; /* set for no reduction */
   if (videoentry.xdots < filexdots || videoentry.ydots < fileydots) {
      /* set up to load only every nth pixel to make image fit */
      skipxdots = skipydots = 1;
      while (skipxdots * videoentry.xdots < filexdots) ++skipxdots;
      while (skipydots * videoentry.ydots < fileydots) ++skipydots;
      i = j = 0;
      while (1) {
	 tmpxdots = (filexdots + skipxdots - 1) / skipxdots;
	 tmpydots = (fileydots + skipydots - 1) / skipydots;
	 if (fileaspectratio == 0 || videoentry.dotmode == 11)
	    break;
	 /* reduce further if that improves aspect */
	 if ((ftemp = vid_aspect(tmpxdots,tmpydots)) > fileaspectratio) {
	    if (j) break; /* already reduced x, don't reduce y */
	    ftemp2 = vid_aspect(tmpxdots,(fileydots+skipydots)/(skipydots+1));
	    if (ftemp2 < fileaspectratio
	      && ftemp/fileaspectratio *0.9 <= fileaspectratio/ftemp2)
	       break; /* further y reduction is worse */
	    ++skipydots;
	    ++i;
	    }
	 else {
	    if (i) break; /* already reduced y, don't reduce x */
	    ftemp2 = vid_aspect((filexdots+skipxdots)/(skipxdots+1),tmpydots);
	    if (ftemp2 > fileaspectratio
	      && fileaspectratio/ftemp *0.9 <= ftemp2/fileaspectratio)
	       break; /* further x reduction is worse */
	    ++skipxdots;
	    ++j;
	    }
	 }
      filexdots = tmpxdots;
      fileydots = tmpydots;
      --skipxdots;
      --skipydots;
      }

   if ((finalaspectratio = fileaspectratio) == 0) /* assume display correct */
      finalaspectratio = vid_aspect(filexdots,fileydots);
   if (finalaspectratio >= SCREENASPECT-0.02
     && finalaspectratio <= SCREENASPECT+0.02)
      finalaspectratio = SCREENASPECT;
   i = finalaspectratio * 1000.0 + 0.5;
   finalaspectratio = (double)i/1000.0; /* chop precision to 3 decimals */

   /* setup view window stuff */
   viewwindow = viewxdots = viewydots = 0;
   if (filexdots != videoentry.xdots || fileydots != videoentry.ydots) {
      /* image not exactly same size as screen */
      viewwindow = 1;
      ftemp = finalaspectratio
	    * (double)videoentry.ydots / (double)videoentry.xdots
	    / SCREENASPECT;
      if (finalaspectratio <= SCREENASPECT) {
	 i = (double)videoentry.xdots / (double)filexdots * 20.0 + 0.5;
	 tmpreduce = (double)i/20.0; /* chop precision to nearest .05 */
	 i = (double)videoentry.xdots / tmpreduce + 0.5;
	 j = (double)i * ftemp + 0.5;
	 }
      else {
	 i = (double)videoentry.ydots / (double)fileydots * 20.0 + 0.5;
	 tmpreduce = (double)i/20.0; /* chop precision to nearest .05 */
	 j = (double)videoentry.ydots / tmpreduce + 0.5;
	 i = (double)j / ftemp + 0.5;
	 }
      if (i != filexdots || j != fileydots) { /* too bad, must be explicit */
	 viewxdots = filexdots;
	 viewydots = fileydots;
	 }
      else
	 viewreduction = tmpreduce; /* ok, this works */
      }
   if (finalaspectratio != SCREENASPECT || viewxdots != 0) {
      stopmsg(4,"\
Warning: <V>iew parameters are being set to non-standard values.\n\
Remember to reset them when finished with this image.");
      }

   return(0);
}

static void format_item(int choice,char *buf)
{
   char errbuf[10];
   unsigned tmpflags;
   errbuf[0] = 0;
   tmpflags = vidptr[choice].flags;
   if (tmpflags & (VI_VSMALL+VI_CSMALL+VI_ASPECT)) strcat(errbuf,"*");
   if (tmpflags & VI_VSMALL) strcat(errbuf,"R");
   if (tmpflags & VI_CSMALL) strcat(errbuf,"C");
   if (tmpflags & VI_ASPECT) strcat(errbuf,"A");
   if (tmpflags & VI_VBIG)   strcat(errbuf,"v");
   if (tmpflags & VI_CBIG)   strcat(errbuf,"c");
   format_vid_inf(vidptr[choice].entnum,errbuf,buf);
}

static int check_modekey(int curkey)
{
   int i;
   for (i = 0; i < maxvideomode; ++i)
      if (curkey == kbdkeys[i])
	 return(-100-i);
   return(0);
}

