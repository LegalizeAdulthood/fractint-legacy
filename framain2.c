#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef XFRACT
#include <io.h>
#include <dos.h>
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <ctype.h>
#include "prototyp.h"
#include "fractype.h"
#include "helpdefs.h"

#if 0
/* makes a handly list of jul-man pairs, not for release */
static void julman()
{
   FILE *fp;
   int i;
   fp = dir_fopen(workdir,"toggle.txt","w");
   i = -1;
   while(fractalspecific[++i].name)
   {
      if(fractalspecific[i].tojulia != NOFRACTAL && fractalspecific[i].name[0] != '*')
         fprintf(fp,"%s  %s\n",fractalspecific[i].name,
             fractalspecific[fractalspecific[i].tojulia].name);
   }
   fclose(fp);
}
#endif

/* routines in this module      */

int main_menu_switch(int*,int*,int*,char*,int);
int big_while_loop(int *kbdmore, char *stacked, int resumeflag);
static void move_zoombox(int);
char fromtext_flag = 0;         /* = 1 if we're in graphics mode */
static int call_line3d(BYTE *pixels, int linelen);
static  void note_zoom(void);
static  void restore_zoom(void);
static  void move_zoombox(int keynum);
static  void cmp_line_cleanup(void);
static void _fastcall restore_history_info(int);
static void _fastcall save_history_info(void);

static char far *savezoom;
static  int        historyptr = -1;     /* user pointer into history tbl  */
static  int        saveptr = 0;         /* save ptr into history tbl      */
static  int        historyflag;         /* are we backing off in history? */
void (*outln_cleanup) (void);

int big_while_loop(int *kbdmore, char *stacked, int resumeflag)
{
   int     frommandel;                  /* if julia entered from mandel */
   int     axmode=0, bxmode, cxmode, dxmode; /* video mode (BIOS ##)    */
   double  ftemp;                       /* fp temp                      */
   int     i;                           /* temporary loop counters      */
   int kbdchar;
   frommandel = 0;
   if(resumeflag)
      goto resumeloop;
    for(;;) {                   /* eternal loop */
      if (calc_status != 2 || showfile == 0) {
#ifdef XFRACT
         if (resizeWindow()) {
             calc_status = -1;
         }
#endif
         far_memcpy((char far *)&videoentry,(char far *)&videotable[adapter],
                    sizeof(videoentry));
         axmode  = videoentry.videomodeax; /* video mode (BIOS call)   */
         bxmode  = videoentry.videomodebx; /* video mode (BIOS call)   */
         cxmode  = videoentry.videomodecx; /* video mode (BIOS call)   */
         dxmode  = videoentry.videomodedx; /* video mode (BIOS call)   */
         dotmode = videoentry.dotmode;     /* assembler dot read/write */
         xdots   = videoentry.xdots;       /* # dots across the screen */
         ydots   = videoentry.ydots;       /* # dots down the screen   */
         colors  = videoentry.colors;      /* # colors available */
         dotmode %= 1000;
         textsafe2 = dotmode / 100;
         dotmode  %= 100;
         sxdots  = xdots;
         sydots  = ydots;
         sxoffs = syoffs = 0;

         diskvideo = 0;                 /* set diskvideo flag */
         if (dotmode == 11)             /* default assumption is disk */
            diskvideo = 2;

         memcpy(olddacbox,dacbox,256*3); /* save the DAC */
         diskisactive = 1;              /* flag for disk-video routines */

         if (overlay3d && !initbatch) {
            unstackscreen();            /* restore old graphics image */
            overlay3d = 0;
            }

         else {
            setvideomode(axmode,bxmode,cxmode,dxmode); /* switch video modes */
            if (goodmode == 0) {
               static FCODE msg[] = {"That video mode is not available with your adapter."};
               static FCODE TPlusStr[] = "This video mode requires 'noninterlaced=yes'";

               if(TPlusErr) {
                  stopmsg(0, TPlusStr);
                  TPlusErr = 0;
                  }
               else {
                  stopmsg(0,msg);
                  askvideo = TRUE;
                  }
               initmode = -1;
               setvideotext(); /* switch to text mode */
               /* goto restorestart; */
               return(RESTORESTART);
               }
            }

         diskisactive = 0;              /* flag for disk-video routines */
         if (savedac || colorpreloaded) {
            memcpy(dacbox,olddacbox,256*3); /* restore the DAC */
            spindac(0,1);
            colorpreloaded = 0;
            }
         else { /* reset DAC to defaults, which setvideomode has done for us */
            if (mapdacbox) { /* but there's a map=, so load that */
               far_memcpy((char far *)dacbox,mapdacbox,768);
               spindac(0,1);
               }
            else if ((dotmode == 11 && colors == 256) || !colors) {
               /* disk video, setvideomode via bios didn't get it right, so: */
#ifndef XFRACT
               ValidateLuts("default"); /* read the default palette file */
#endif
               }
            colorstate = 0;
            }
         if (viewwindow) {
            ftemp = finalaspectratio
                    * (double)sydots / (double)sxdots / screenaspect;
            if ((xdots = viewxdots) != 0) { /* xdots specified */
               if ((ydots = viewydots) == 0) /* calc ydots? */
                  ydots = (int)((double)xdots * ftemp + 0.5);
               }
            else
               if (finalaspectratio <= screenaspect) {
                  xdots = (int)((double)sxdots / viewreduction + 0.5);
                  ydots = (int)((double)xdots * ftemp + 0.5);
                  }
               else {
                  ydots = (int)((double)sydots / viewreduction + 0.5);
                  xdots = (int)((double)ydots / ftemp + 0.5);
                  }
            if (xdots > sxdots || ydots > sydots) {
               static FCODE msg[] = {"View window too large; using full screen."};
               stopmsg(0,msg);
               xdots = sxdots;
               ydots = sydots;
               }
            else if (xdots <= sxdots/20 || ydots <= sydots/20) { /* so ssg works */
               static FCODE msg[] = {"View window too small; using full screen."};
               stopmsg(0,msg);
               xdots = sxdots;
               ydots = sydots;
               }
            sxoffs = (sxdots - xdots) / 2;
            syoffs = (sydots - ydots) / 3;
            }
         dxsize = xdots - 1;            /* convert just once now */
         dysize = ydots - 1;
         }
      if(savedac == 0)
        savedac = 2;                    /* assume we save next time (except jb) */
      else
      savedac = 1;                      /* assume we save next time */
      if (initbatch == 0)
         lookatmouse = -PAGE_UP;        /* mouse left button == pgup */

      if(showfile == 0) {               /* loading an image */
         outln_cleanup = NULL;          /* outln routine can set this */
         if (display3d)                 /* set up 3D decoding */
            outln = call_line3d;
         else if(filetype >= 1)         /* old .tga format input file */
            outln = outlin16;
         else if(comparegif)            /* debug 50 */
            outln = cmp_line;
         else if(pot16bit) {            /* .pot format input file */
            if (pot_startdisk() < 0)
            {                           /* pot file failed?  */
               showfile = 1;
               potflag  = 0;
               pot16bit = 0;
               initmode = -1;
               calc_status = 2;         /* "resume" without 16-bit */
               setvideotext();
               get_fracttype();
               /* goto imagestart; */
               return(IMAGESTART);
            }
            outln = pot_line;
         }
         else                           /* regular gif/fra input file */
            if(soundflag > 0)
               outln = sound_line;      /* sound decoding */
            else
               outln = out_line;        /* regular decoding */
         if(filetype == 0)
         {
            if(iit == 2 && usr_floatflag != 0)
           if(F4x4Lock()==0)
              iit = -1;  /* semaphore not free - no iit */
        if(debugflag==2224)
                {
                char buf[80];
                sprintf(buf,"iit=%d floatflag=%d",iit,usr_floatflag);
                stopmsg(4,(char far *)buf);
        }

            i = funny_glasses_call(gifview);
            if(iit == 2)
           F4x4Free();      /* unlock semaphore */
        else if(iit == -1)
           iit = 2;         /* semaphore operating */
     }
         else
            i = funny_glasses_call(tgaview);
         if(outln_cleanup)              /* cleanup routine defined? */
            (*outln_cleanup)();
         if(i == 0)
            buzzer(0);
         else {
            calc_status = -1;
            if (keypressed()) {
               static FCODE msg[] = {"*** load incomplete ***"};
               buzzer(1);
               while (keypressed()) getakey();
               texttempmsg(msg);
               }
            }
         }

      zoomoff = 1;                      /* zooming is enabled */
      if (dotmode == 11 || (curfractalspecific->flags&NOZOOM) != 0)
         zoomoff = 0;                   /* for these cases disable zooming */
      calcfracinit();
#ifdef XFRACT
      schedulealarm(1);
#endif

      sxmin = xxmin; /* save 3 corners for zoom.c ref points */
      sxmax = xxmax;
      sx3rd = xx3rd;
      symin = yymin;
      symax = yymax;
      sy3rd = yy3rd;

      if(bf_math)
      {
         copy_bf(bfsxmin,bfxmin);
         copy_bf(bfsxmax,bfxmax);
         copy_bf(bfsymin,bfymin);
         copy_bf(bfsymax,bfymax);
         copy_bf(bfsx3rd,bfx3rd);
         copy_bf(bfsy3rd,bfy3rd);
      }
      save_history_info();
      if (display3d || showfile) {      /* paranoia: these vars don't get set */
         save_system  = active_system;  /*   unless really doing some work,   */
         }                              /*   so simple <r> + <s> keeps number */

      if(showfile == 0) {               /* image has been loaded */
         showfile = 1;
         if (initbatch == 1 && calc_status == 2)
            initbatch = -1; /* flag to finish calc before save */
         if (loaded3d)      /* 'r' of image created with '3' */
            display3d = 1;  /* so set flag for 'b' command */
         }
      else {                            /* draw an image */
         diskisactive = 1;              /* flag for disk-video routines */
         if (initsavetime != 0          /* autosave and resumable? */
           && (curfractalspecific->flags&NORESUME) == 0) {
            savebase = readticker(); /* calc's start time */
            saveticks = abs(initsavetime);
            saveticks *= 1092; /* bios ticks/minute */
            if ((saveticks & 65535L) == 0)
               ++saveticks; /* make low word nonzero */
            finishrow = -1;
            }
         browsing = FALSE;      /* regenerate image, turn off browsing */
         name_stack_ptr = -1;   /* reset pointer */
         browsename[0] = '\0';  /* null */
         i = calcfract();       /* draw the fractal using "C" */
         if (i == 0)
            buzzer(0); /* finished!! */

         saveticks = 0;                 /* turn off autosave timer */
         if (dotmode == 11 && i == 0) /* disk-video */
         {
            static FCODE o_msg[] = {"Image has been completed"};
            char msg[sizeof(o_msg)];
            far_strcpy(msg,o_msg);
            dvid_status(0,msg);
         }
         diskisactive = 0;              /* flag for disk-video routines */
         }

#ifndef XFRACT
      boxcount = 0;                     /* no zoom box yet  */
      zwidth = 0;
#else
      if (!XZoomWaiting) {
          boxcount = 0;                 /* no zoom box yet  */
          zwidth = 0;
      }
#endif

      if (fractype == PLASMA && cpu > 88) {
         cyclelimit = 256;              /* plasma clouds need quick spins */
         daccount = 256;
         daclearn = 1;
         }

resumeloop:                             /* return here on failed overlays */

      *kbdmore = 1;
      while (*kbdmore == 1) {           /* loop through command keys */
         if (timedsave != 0) {
            if (timedsave == 1) {       /* woke up for timed save */
               getakey();     /* eat the dummy char */
               kbdchar = 's'; /* do the save */
               resave_flag = 1;
               timedsave = 2;
               }
            else {                      /* save done, resume */
               timedsave = 0;
               resave_flag = 2;
               kbdchar = ENTER;
               }
            }
         else if (initbatch == 0) {     /* not batch mode */
            lookatmouse = (zwidth == 0) ? -PAGE_UP : 3;
            if (calc_status == 2 && zwidth == 0 && !keypressed()) {
                  kbdchar = ENTER ;  /* no visible reason to stop, continue */
            } else {     /* wait for a real keystroke */
              if (autobrowse && !no_sub_images) kbdchar = 'l';
               else
               {
#ifndef XFRACT
               while (!keypressed());/* { }*/  /* enables help */
#else
               waitkeypressed(0);
#endif
               kbdchar = getakey();
               }
               if (kbdchar == ESC || kbdchar == 'm' || kbdchar == 'M') {
                  if (kbdchar == ESC && escape_exit != 0)
                      /* don't ask, just get out */
                      goodbye();
                  stackscreen();
#ifndef XFRACT
                  kbdchar = main_menu(1);
#else
                  if (XZoomWaiting) {
                      kbdchar = ENTER;
                  } else {
                      kbdchar = main_menu(1);
                      if (XZoomWaiting) {
                          kbdchar = ENTER;
                      }
                  }
#endif
                  if (kbdchar == '\\' || kbdchar == CTL_BACKSLASH ||
                      kbdchar == 'h' || kbdchar == 8 ||
                      check_vidmode_key(0,kbdchar) >= 0)
                     discardscreen();
                  else if (kbdchar == 'x' || kbdchar == 'y' ||
                           kbdchar == 'z' || kbdchar == 'g' ||
                           kbdchar == 'v' || kbdchar == 2)
                     fromtext_flag = 1;
                  else
                     unstackscreen();
                  }
               }
            }
         else {         /* batch mode, fake next keystroke */

/* initbatch == -1  flag to finish calc before save */
/* initbatch == 0   not in batch mode */
/* initbatch == 1   normal batch mode */
/* initbatch == 2   was 1, now do a save */
/* initbatch == 3   bailout with errorlevel == 2, error occurred, no save */
/* initbatch == 4   bailout with errorlevel == 1, interrupted, try to save */
/* initbatch == 5   was 4, now do a save */

            if (initbatch == -1) {      /* finish calc */
               kbdchar = ENTER;
               initbatch = 1;
               }
            else if (initbatch == 1 || initbatch == 4 ) {       /* save-to-disk */
/*
               while(keypressed())
                 getakey();
*/
               if (debugflag == 50)
                  kbdchar = 'r';
               else
                  kbdchar = 's';
               if(initbatch == 1) initbatch = 2;
               if(initbatch == 4) initbatch = 5;
               }
            else {
               if(calc_status != 4) initbatch = 3; /* bailout with error */
               goodbye();               /* done, exit */
               }
            }

#ifndef XFRACT
         if ('A' <= kbdchar && kbdchar <= 'Z')
            kbdchar = tolower(kbdchar);
#endif

         switch(main_menu_switch(&kbdchar,&frommandel,kbdmore,stacked,axmode))
         {
         case IMAGESTART:
            return(IMAGESTART);
         case RESTORESTART:
            return(RESTORESTART);
         case RESTART:
            return(RESTART);
         case CONTINUE:
            continue;
         default:
            break;
         }
         if (zoomoff == 1 && *kbdmore == 1) /* draw/clear a zoom box? */
            drawbox(1);
#ifdef XFRACT
         if (resizeWindow()) {
             calc_status = -1;
         }
#endif
         }
      }
/*  return(0); */
}

int main_menu_switch(int *kbdchar, int *frommandel, int *kbdmore, char *stacked, int axmode)
{
   int i,k;
   static double  jxxmin, jxxmax, jyymin, jyymax; /* "Julia mode" entry point */
   static double  jxx3rd, jyy3rd;
   /*
   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   */
   switch (*kbdchar)
   {
   case 't':                    /* new fractal type             */
      julibrot = 0;
      clear_zoombox();
      stackscreen();
      if ((i = get_fracttype()) >= 0)
      {
         discardscreen();
         savedac = 0;
         save_release = release;
         no_mag_calc = 0;
         use_old_period = 0;
         if (i == 0)
         {
            initmode = adapter;
            *frommandel = 0;
         }
         else if (initmode < 0) /* it is supposed to be... */
            setvideotext();     /* reset to text mode      */
         return(IMAGESTART);
      }
      unstackscreen();
      break;
   case 24:                     /* Ctl-X, Ctl-Y, CTL-Z do flipping */
   case 25:
   case 26:
      flip_image(*kbdchar);
      break;
   case 'x':                    /* invoke options screen        */
   case 'y':
   case 'z':                    /* type specific parms */
   case 'g':
   case 'v':
   case 2:
      if (fromtext_flag == 1)
         fromtext_flag = 0;
      else
         stackscreen();
      if (*kbdchar == 'x')
         i = get_toggles();
      else if (*kbdchar == 'y')
         i = get_toggles2();
      else if (*kbdchar == 'z')
         i = get_fract_params(1);
      else if (*kbdchar == 'v')
         i = get_view_params(); /* get the parameters */
      else if (*kbdchar == 2)
         i = get_browse_params();
      else
         i = get_cmd_string();
      unstackscreen();
      if (i > 0)                /* time to redraw? */
         *kbdmore = calc_status = 0;
      break;
#ifndef XFRACT
   case '@':                    /* execute commands */
   case '2':                    /* execute commands */
#else
   case F2:                     /* execute commands */
#endif
      stackscreen();
      i = get_commands();
      if (initmode != -1)
      {                         /* video= was specified */
         adapter = initmode;
         initmode = -1;
         i |= 1;
         savedac = 0;
      }
      else if (colorpreloaded)
      {                         /* colors= was specified */
         spindac(0, 1);
         colorpreloaded = 0;
      }
      else if ((i & 8))         /* reset was specified */
         savedac = 0;
      if ((i & 4))
      {                         /* 3d = was specified */
         *kbdchar = '3';
         unstackscreen();
         goto do_3d_transform;  /* pretend '3' was keyed */
      }
      if ((i & 1))
      {                         /* fractal parameter changed */
         discardscreen();
         /* backwards_v18();*/  /* moved this to cmdfiles.c */
         /* backwards_v19();*/
         *kbdmore = calc_status = 0;
      }
      else
         unstackscreen();
      break;
   case 'f':                    /* floating pt toggle           */
      if (usr_floatflag == 0)
         usr_floatflag = 1;
      else
         usr_floatflag = 0;
      initmode = adapter;
      return(IMAGESTART);
   case 'i':                    /* 3d fractal parms */
      if (get_fract3d_params() >= 0)    /* get the parameters */
         calc_status = *kbdmore = 0;    /* time to redraw */
      break;
#if 0
   case 'w':
      /*chk_keys();*/
      /*julman();*/
      break;
#endif
   case 1:                     /* ^a Ant */
      clear_zoombox();
      {
         int oldtype, err, i;
         double oldparm[MAXPARAMS];
         oldtype = fractype;
         for(i=0;i<MAXPARAMS;i++)
            oldparm[i] = param[i];
         if (fractype != ANT)
         {
            fractype = ANT;
            curfractalspecific = &fractalspecific[fractype];
            load_params(fractype);
         }
         if (!fromtext_flag)
            stackscreen();
         fromtext_flag = 0;
         if ((err = get_fract_params(2)) >= 0)
         {
            unstackscreen();
            if (ant() >= 0)
               calc_status = 0;
         }
         else
            unstackscreen();
         fractype = oldtype;
         for(i=0;i<MAXPARAMS;i++)
            param[i] = oldparm[i];
         if (err >= 0)
            return(CONTINUE);
      }
      break;
   case 'k':                    /* ^s is irritating, give user a single key */
   case 19:                     /* ^s RDS */
      clear_zoombox();
      if (get_rds_params() >= 0)
      {
         if (do_AutoStereo() >= 0)
            calc_status = 0;
         return(CONTINUE);
      }
      break;
   case 'a':                    /* starfield parms               */
      clear_zoombox();
      if (get_starfield_params() >= 0)
      {
         if (starfield() >= 0)
            calc_status = 0;
         return(CONTINUE);
      }
      break;
   case 15:                     /* ctrl-o */
   case 'o':
      /* must use standard fractal and have a float variant */
      if ((fractalspecific[fractype].calctype == StandardFractal
           || fractalspecific[fractype].calctype == calcfroth) &&
          (fractalspecific[fractype].isinteger == 0 ||
           fractalspecific[fractype].tofloat != NOFRACTAL) &&
           !bf_math /* for now no arbitrary precision support */ )
      {
         clear_zoombox();
         Jiim(ORBIT);
      }
      break;
   case SPACE:                  /* spacebar, toggle mand/julia   */
      if(bf_math)
         break;
      if (fractype == CELLULAR)
      {
         if (nxtscreenflag)
            nxtscreenflag = 0;  /* toggle flag to stop generation */
         else
            nxtscreenflag = 1;  /* toggle flag to generate next screen */
         calc_status = 2;
         *kbdmore = 0;
      }
      else
      {
         if (curfractalspecific->tojulia != NOFRACTAL
             && param[0] == 0.0 && param[1] == 0.0)
         {
            /* switch to corresponding Julia set */
            int key;
            if ((fractype == MANDEL || fractype == MANDELFP) && bf_math == 0)
                  hasinverse = 1;
            else
                  hasinverse = 0;
            clear_zoombox();
            Jiim(JIIM);
            key = getakey();    /* flush keyboard buffer */
            if (key != SPACE)
            {
                  ungetakey(key);
                  break;
            }
            fractype = curfractalspecific->tojulia;
            curfractalspecific = &fractalspecific[fractype];
            if (xcjul == BIG || ycjul == BIG)
            {
               param[0] = (xxmax + xxmin) / 2;
               param[1] = (yymax + yymin) / 2;
            }
            else
            {
               param[0] = xcjul;
               param[1] = ycjul;
               xcjul = ycjul = BIG;
            }
            jxxmin = sxmin;
            jxxmax = sxmax;
            jyymax = symax;
            jyymin = symin;
            jxx3rd = sx3rd;
            jyy3rd = sy3rd;
            *frommandel = 1;
            xxmin = curfractalspecific->xmin;
            xxmax = curfractalspecific->xmax;
            yymin = curfractalspecific->ymin;
            yymax = curfractalspecific->ymax;
            xx3rd = xxmin;
            yy3rd = yymin;
            if (usr_distest == 0 && usr_biomorph != -1 && bitshift != 29)
            {
               xxmin *= 3.0;
               xxmax *= 3.0;
               yymin *= 3.0;
               yymax *= 3.0;
               xx3rd *= 3.0;
               yy3rd *= 3.0;
            }
            zoomoff = 1;
            calc_status = 0;
            *kbdmore = 0;
         }
         else if (curfractalspecific->tomandel != NOFRACTAL)
         {
            /* switch to corresponding Mandel set */
            fractype = curfractalspecific->tomandel;
            curfractalspecific = &fractalspecific[fractype];
            if (*frommandel)
            {
               xxmin = jxxmin;
               xxmax = jxxmax;
               yymin = jyymin;
               yymax = jyymax;
               xx3rd = jxx3rd;
               yy3rd = jyy3rd;
            }
            else
            {
               xxmin = xx3rd = curfractalspecific->xmin;
               xxmax = curfractalspecific->xmax;
               yymin = yy3rd = curfractalspecific->ymin;
               yymax = curfractalspecific->ymax;
            }
            SaveC.x = param[0];
            SaveC.y = param[1];
            param[0] = 0;
            param[1] = 0;
            zoomoff = 1;
            calc_status = 0;
            *kbdmore = 0;
         }
         else
            buzzer(2);          /* can't switch */
      }                         /* end of else for if == cellular */
      break;
   case 'j':                    /* inverse julia toggle */
      /* if the inverse types proliferate, something more elegant will be
       * needed */
      if (fractype == JULIA || fractype == JULIAFP || fractype == INVERSEJULIA)
      {
         static int oldtype = -1;
         if (fractype == JULIA || fractype == JULIAFP)
         {
            oldtype = fractype;
            fractype = INVERSEJULIA;
         }
         else if (fractype == INVERSEJULIA)
         {
            if (oldtype != -1)
               fractype = oldtype;
            else
               fractype = JULIA;
         }
         curfractalspecific = &fractalspecific[fractype];
         zoomoff = 1;
         calc_status = 0;
         *kbdmore = 0;
      }
#if 0
      else if (fractype == MANDEL || fractype == MANDELFP)
      {
         clear_zoombox();
         Jiim(JIIM);
      }
#endif
      else
         buzzer(2);
      break;
   case '\\':                   /* return to prev image    */
   case CTL_BACKSLASH:
   case 'h':
   case 8:
      if (name_stack_ptr >= 1)
      {
         /* go back one file if somewhere to go (ie. browsing) */
         name_stack_ptr--;
         while (file_name_stack[name_stack_ptr][0] == '\0' 
                && name_stack_ptr >= 0)
            name_stack_ptr--;
         if (name_stack_ptr < 0) /* oops, must have deleted first one */
            break;
         strcpy(browsename, file_name_stack[name_stack_ptr]);
         /*
         splitpath(browsename, NULL, NULL, fname, ext);
         splitpath(readname, drive, dir, NULL, NULL);
         makepath(readname, drive, dir, fname, ext);
         */
         merge_pathnames(readname,browsename,2);
         browsing = TRUE;
         no_sub_images = FALSE;
         showfile = 0;
         if (askvideo)
         {
            stackscreen();      /* save graphics image */
            *stacked = 1;
         }
         return(RESTORESTART);
      }
      else if(maxhistory > 0 && bf_math == 0)
      {
         if(*kbdchar == '\\' || *kbdchar == 'h')
            if (--historyptr < 0)
               historyptr = maxhistory - 1;
         if(*kbdchar == CTL_BACKSLASH || *kbdchar == 8)
            if (++historyptr >= maxhistory)
               historyptr = 0;
         restore_history_info(historyptr);
         zoomoff = 1;
         initmode = adapter;
         if (curfractalspecific->isinteger != 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 0;
         if (curfractalspecific->isinteger == 0 &&
             curfractalspecific->tofloat != NOFRACTAL)
            usr_floatflag = 1;
         historyflag = 1;       /* avoid re-store parms due to rounding errs */
         return(IMAGESTART);
      }
      break;
   case 'd':                    /* shell to MS-DOS              */
      stackscreen();
#ifndef XFRACT
      if (axmode == 0 || axmode > 7)
      {
         static FCODE dosmsg[] =
         {"\
Note:  Your graphics image is still squirreled away in your video\n\
adapter's memory.  Switching video modes will clobber part of that\n\
image.  Sorry - it's the best we could do."};
         putstring(0, 0, 7, dosmsg);
         movecursor(6, 0);
      }
#endif
      shell_to_dos();
      unstackscreen();
/*             calc_status = 0; */
      break;
   case 'c':                    /* switch to color cycling      */
   case '+':                    /* rotate palette               */
   case '-':                    /* rotate palette               */
      clear_zoombox();
      memcpy(olddacbox, dacbox, 256 * 3);
      rotate((*kbdchar == 'c') ? 0 : ((*kbdchar == '+') ? 1 : -1));
      if (memcmp(olddacbox, dacbox, 256 * 3))
      {
         colorstate = 1;
         save_history_info();
      }
      return(CONTINUE);
   case 'e':                    /* switch to color editing      */
      clear_zoombox();
      if (dacbox[0][0] != 255 && !reallyega && colors >= 16
          && dotmode != 11)
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         memcpy(olddacbox, dacbox, 256 * 3);
         helpmode = HELPXHAIR;
         EditPalette();
         helpmode = oldhelpmode;
         if (memcmp(olddacbox, dacbox, 256 * 3))
         {
            colorstate = 1;
            save_history_info();
         }
      }
      return(CONTINUE);
   case 's':                    /* save-to-disk                 */
      diskisactive = 1;         /* flag for disk-video routines */
      note_zoom();
      savetodisk(savename);
      restore_zoom();
      diskisactive = 0;         /* flag for disk-video routines */
      return(CONTINUE);
   case '#':                    /* 3D overlay                   */
#ifdef XFRACT
   case F3:                     /* 3D overlay                   */
#endif
      clear_zoombox();
      overlay3d = 1;
   case '3':                    /* restore-from (3d)            */
    do_3d_transform:
      if (overlay3d)
         display3d = 2;         /* for <b> command               */
      else
         display3d = 1;
   case 'r':                    /* restore-from                 */
      comparegif = 0;
      *frommandel = 0;
      if (browsing)
      {
         browsing = FALSE;
      }
      if (*kbdchar == 'r')
      {
         if (debugflag == 50)
         {
            comparegif = overlay3d = 1;
            if (initbatch == 2)
            {
               stackscreen();   /* save graphics image */
               strcpy(readname, savename);
               showfile = 0;
               return(RESTORESTART);
            }
         }
         else
            comparegif = overlay3d = 0;
         display3d = 0;
      }
      stackscreen();            /* save graphics image */
      if (overlay3d)
         *stacked = 0;
      else
         *stacked = 1;
      if (resave_flag)
      {
         updatesavename(savename);      /* do the pending increment */
         resave_flag = started_resaves = 0;
      }
      showfile = -1;
      return(RESTORESTART);
      /* RB */
   case 'l':
   case 'L':                    /* Look for other files within this view */
      if ((zwidth == 0) && (!diskvideo))        /* not zooming & no disk
                                                 * video */
      {
         int oldhelpmode;
         oldhelpmode = helpmode;
         helpmode = HELPBROWSE;
         switch (fgetwindow())
         {
         case ENTER:
         case ENTER_2:
            showfile = 0;       /* trigger load */
            browsing = TRUE;    /* but don't ask for the file name as it's
                                 * just been selected */
            if (name_stack_ptr == 15)
            {                   /* about to run off the end of the file
                                 * history stack so shift it all back one to
                                 * make room, lose the 1st one */
               int tmp;
               for (tmp = 1; tmp < 16; tmp++)
                  strcpy(file_name_stack[tmp - 1], file_name_stack[tmp]);
               name_stack_ptr = 14;
            }
            name_stack_ptr++;
            strcpy(file_name_stack[name_stack_ptr], browsename);
            /*
            splitpath(browsename, NULL, NULL, fname, ext);
            splitpath(readname, drive, dir, NULL, NULL);
            makepath(readname, drive, dir, fname, ext);
            */
            merge_pathnames(readname,browsename,2);
            if (askvideo)
            {
               stackscreen();   /* save graphics image */
               *stacked = 1;
            }
            return(RESTORESTART);       /* hop off and do it!! */
         case '\\':
            if (name_stack_ptr >= 1)
            {
               /* go back one file if somewhere to go (ie. browsing) */
               name_stack_ptr--;
               while (file_name_stack[name_stack_ptr][0] == '\0' 
                      && name_stack_ptr >= 0)
                  name_stack_ptr--;
               if (name_stack_ptr < 0) /* oops, must have deleted first one */
                  break;
               strcpy(browsename, file_name_stack[name_stack_ptr]);
               /*
               splitpath(browsename, NULL, NULL, fname, ext);
               splitpath(readname, drive, dir, NULL, NULL);
               makepath(readname, drive, dir, fname, ext);
               */
               merge_pathnames(readname,browsename,2);
               browsing = TRUE;
               showfile = 0;
               if (askvideo)
               {
                  stackscreen();/* save graphics image */
                  *stacked = 1;
               }
               return(RESTORESTART);
            }                   /* otherwise fall through and turn off
                                 * browsing */
         case ESC:
         case 'l':              /* turn it off */
         case 'L':
            browsing = FALSE;
            helpmode = oldhelpmode;
            break;
         case 's':
            browsing = FALSE;
            helpmode = oldhelpmode;
            savetodisk(savename);
            break;
         default:               /* or no files found, leave the state of
                                 * browsing */
            break;              /* alone */
         }
      }
      else
      {
         browsing = FALSE;
         buzzer(2);             /* can't browse if zooming or diskvideo */
      }
      break;
   case 'b':                    /* make batch file              */
      make_batch_file();
      break;
   case 'p':                    /* print current image          */
      note_zoom();
      Print_Screen();
      restore_zoom();
      if (!keypressed())
         buzzer(0);
      else
      {
         buzzer(1);
         getakey();
      }
      return(CONTINUE);
   case ENTER:                  /* Enter                        */
   case ENTER_2:                /* Numeric-Keypad Enter         */
#ifdef XFRACT
      XZoomWaiting = 0;
#endif
      if (zwidth != 0.0)
      {                         /* do a zoom */
         init_pan_or_recalc(0);
         *kbdmore = 0;
      }
      if (calc_status != 4)     /* don't restart if image complete */
         *kbdmore = 0;
      break;
   case CTL_ENTER:              /* control-Enter                */
   case CTL_ENTER_2:            /* Control-Keypad Enter         */
      init_pan_or_recalc(1);
      *kbdmore = 0;
      zoomout();                /* calc corners for zooming out */
      break;
   case INSERT:         /* insert                       */
      setvideotext();           /* force text mode */
      return(RESTART);
   case LEFT_ARROW:             /* cursor left                  */
   case RIGHT_ARROW:            /* cursor right                 */
   case UP_ARROW:               /* cursor up                    */
   case DOWN_ARROW:             /* cursor down                  */
   case LEFT_ARROW_2:           /* Ctrl-cursor left             */
   case RIGHT_ARROW_2:          /* Ctrl-cursor right            */
   case UP_ARROW_2:             /* Ctrl-cursor up               */
   case DOWN_ARROW_2:           /* Ctrl-cursor down             */
      move_zoombox(*kbdchar);
      break;
   case CTL_HOME:               /* Ctrl-home                    */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_HOME);
         if ((zskew -= 0.02 * i) < -0.48)
            zskew = -0.48;
      }
      break;
   case CTL_END:                /* Ctrl-end                     */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
      {
         i = key_count(CTL_END);
         if ((zskew += 0.02 * i) > 0.48)
            zskew = 0.48;
      }
      break;
   case CTL_PAGE_UP:            /* Ctrl-pgup                    */
      if (boxcount)
         chgboxi(0, -2 * key_count(CTL_PAGE_UP));
      break;
   case CTL_PAGE_DOWN:          /* Ctrl-pgdn                    */
      if (boxcount)
         chgboxi(0, 2 * key_count(CTL_PAGE_DOWN));
      break;
   case PAGE_UP:                /* page up                      */
      if (zoomoff == 1)
         if (zwidth == 0)
         {                      /* start zoombox */
            zwidth = zdepth = 1;
            zskew = zrotate = 0;
            zbx = zby = 0;
            find_special_colors();
            boxcolor = color_bright;
         }
         else
            resizebox(0 - key_count(PAGE_UP));
      break;
   case PAGE_DOWN:              /* page down                    */
      if (boxcount)
      {
         if (zwidth >= .999 && zdepth >= 0.999) /* end zoombox */
            zwidth = 0;
         else
            resizebox(key_count(PAGE_DOWN));
      }
      break;
   case CTL_MINUS:              /* Ctrl-kpad-                  */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate += key_count(CTL_MINUS);
      break;
   case CTL_PLUS:               /* Ctrl-kpad+               */
      if (boxcount && (curfractalspecific->flags & NOROTATE) == 0)
         zrotate -= key_count(CTL_PLUS);
      break;
   case CTL_INSERT:             /* Ctrl-ins                 */
      boxcolor += key_count(CTL_INSERT);
      break;
   case CTL_DEL:                /* Ctrl-del                 */
      boxcolor -= key_count(CTL_DEL);
      break;
   case DELETE:         /* select video mode from list */
      stackscreen();
      *kbdchar = select_video_mode(adapter);
      if (check_vidmode_key(0, *kbdchar) >= 0)  /* picked a new mode? */
         discardscreen();
      else
         unstackscreen();
      /* fall through */
   default:                     /* other (maybe a valid Fn key) */
      if ((k = check_vidmode_key(0, *kbdchar)) >= 0)
      {
         adapter = k;
/*                if (videotable[adapter].dotmode != 11       Took out so that */
/*                  || videotable[adapter].colors != colors)  DAC is not reset */
/*                   savedac = 0;                    when changing video modes */
         calc_status = 0;
         *kbdmore = 0;
         return(CONTINUE);
      }
      break;
   }                            /* end of the big switch */
   return(0);
}

static int call_line3d(BYTE *pixels, int linelen)
{
   /* this routine exists because line3d might be in an overlay */
   return(line3d(pixels,linelen));
}

static void note_zoom()
{
   if (boxcount) { /* save zoombox stuff in far mem before encode (mem reused) */
      if ((savezoom = farmemalloc((long)(5*boxcount))) == NULL)
         clear_zoombox(); /* not enuf mem so clear the box */
      else {
         reset_zoom_corners(); /* reset these to overall image, not box */
         far_memcpy(savezoom,boxx,boxcount*2);
         far_memcpy(savezoom+boxcount*2,boxy,boxcount*2);
         far_memcpy(savezoom+boxcount*4,boxvalues,boxcount);
         }
      }
}

static void restore_zoom()
{
   if (boxcount) { /* restore zoombox arrays */
      far_memcpy(boxx,savezoom,boxcount*2);
      far_memcpy(boxy,savezoom+boxcount*2,boxcount*2);
      far_memcpy(boxvalues,savezoom+boxcount*4,boxcount);
      farmemfree(savezoom);
      drawbox(1); /* get the xxmin etc variables recalc'd by redisplaying */
      }
}

/* do all pending movement at once for smooth mouse diagonal moves */
static void move_zoombox(int keynum)
{  int vertical, horizontal, getmore;
   if (boxcount == 0)
      return;
   vertical = horizontal = 0;
   getmore = 1;
   while (getmore) {
      switch (keynum) {
         case LEFT_ARROW:               /* cursor left */
            --horizontal;
            break;
         case RIGHT_ARROW:              /* cursor right */
            ++horizontal;
            break;
         case UP_ARROW:                 /* cursor up */
            --vertical;
            break;
         case DOWN_ARROW:               /* cursor down */
            ++vertical;
            break;
         case LEFT_ARROW_2:             /* Ctrl-cursor left */
            horizontal -= 5;
            break;
         case RIGHT_ARROW_2:             /* Ctrl-cursor right */
            horizontal += 5;
            break;
         case UP_ARROW_2:               /* Ctrl-cursor up */
            vertical -= 5;
            break;
         case DOWN_ARROW_2:             /* Ctrl-cursor down */
            vertical += 5;
            break;
         default:
            getmore = 0;
         }
      if (getmore) {
         if (getmore == 2)              /* eat last key used */
            getakey();
         getmore = 2;
         keynum = keypressed();         /* next pending key */
         }
      }
   if (horizontal != 0)
      moveboxf((double)horizontal/dxsize,0.0);
   if (vertical != 0)
      moveboxf(0.0,(double)vertical/dysize);
}

/* displays differences between current image file and new image */
static FILE *cmp_fp;
static errcount;
int cmp_line(BYTE *pixels, int linelen)
{
   int row,col;
   int oldcolor;
   if((row = rowcount++) == 0) {
      errcount = 0;
      cmp_fp = dir_fopen(workdir,"cmperr",(initbatch)?"a":"w");
      outln_cleanup = cmp_line_cleanup;
      }
   if(pot16bit) { /* 16 bit info, ignore odd numbered rows */
      if((row & 1) != 0) return(0);
      row >>= 1;
      }
   for(col=0;col<linelen;col++) {
      oldcolor=getcolor(col,row);
      if(oldcolor==(int)pixels[col])
         putcolor(col,row,0);
      else {
         if(oldcolor==0)
            putcolor(col,row,1);
         ++errcount;
         if(initbatch == 0)
            fprintf(cmp_fp,"#%5d col %3d row %3d old %3d new %3d\n",
               errcount,col,row,oldcolor,pixels[col]);
         }
      }
   return(0);
}

static void cmp_line_cleanup(void)
{
   char *timestring;
   time_t ltime;
   if(initbatch) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      fprintf(cmp_fp,"%s compare to %s has %5d errs\n",
                     timestring,readname,errcount);
      }
   fclose(cmp_fp);
}

int pot_line(BYTE *pixels, int linelen)
{
   int row,col,saverowcount;
   if (rowcount == 0)
      if (pot_startdisk() < 0)
         return -1;
   saverowcount = rowcount;
   row = (rowcount >>= 1);
   if ((saverowcount & 1) != 0) /* odd line */
      row += ydots;
   else                         /* even line */
      if (dotmode != 11) /* display the line too */
         out_line(pixels,linelen);
   for (col = 0; col < xdots; ++col)
      writedisk(col+sxoffs,row+syoffs,*(pixels+col));
   rowcount = saverowcount + 1;
   return(0);
}

void clear_zoombox()
{
   zwidth = 0;
   drawbox(0);
   reset_zoom_corners();
}

void reset_zoom_corners()
{
   xxmin = sxmin;
   xxmax = sxmax;
   xx3rd = sx3rd;
   yymax = symax;
   yymin = symin;
   yy3rd = sy3rd;
   if(bf_math)
   {
      copy_bf(bfxmin,bfsxmin);
      copy_bf(bfxmax,bfsxmax);
      copy_bf(bfymin,bfsymin);
      copy_bf(bfymax,bfsymax);
      copy_bf(bfx3rd,bfsx3rd);
      copy_bf(bfy3rd,bfsy3rd);
   }
}

/*
   Function setup287code is called by main() when a 287
   or better fpu is detected.
*/
#define ORBPTR(x) fractalspecific[x].orbitcalc
void setup287code()
{
   ORBPTR(MANDELFP)       = ORBPTR(JULIAFP)      = FJuliafpFractal;
   ORBPTR(BARNSLEYM1FP)   = ORBPTR(BARNSLEYJ1FP) = FBarnsley1FPFractal;
   ORBPTR(BARNSLEYM2FP)   = ORBPTR(BARNSLEYJ2FP) = FBarnsley2FPFractal;
   ORBPTR(MANOWARFP)      = ORBPTR(MANOWARJFP)   = FManOWarfpFractal;
   ORBPTR(MANDELLAMBDAFP) = ORBPTR(LAMBDAFP)     = FLambdaFPFractal;
}

int sound_line(BYTE *pixels, int linelen)
{
   int i;
   for(i=0;i<linelen;i++)
   {
      putcolor(i,rowcount,pixels[i]);
      if(orbit_delay > 0)
         sleepms(orbit_delay);
      w_snd((int)(pixels[i]*3000/colors+basehertz));
      if(keypressed())
      {
        nosnd();
        return(-1);
      }
   }
   nosnd();
   rowcount++;
   return(0);
}

/* read keystrokes while = specified key, return 1+count;       */
/* used to catch up when moving zoombox is slower than keyboard */
int key_count(int keynum)
{  int ctr;
   ctr = 1;
   while (keypressed() == keynum) {
      getakey();
      ++ctr;
      }
   return ctr;
}

static void _fastcall save_history_info()
{
   HISTORY current;
   if(maxhistory <= 0 || bf_math)
      return;
   far_memset((void far *)&current,0,sizeof(HISTORY));
   current.fractal_type    = (short)fractype                  ;
   current.xmin       = xxmin                     ;
   current.xmax       = xxmax                     ;
   current.ymin       = yymin                     ;
   current.ymax       = yymax                     ;
   current.creal              = param[0]                  ;
   current.cimag              = param[1]                  ;
   current.dparm3             = param[2]                  ;
   current.dparm4             = param[3]                  ;
   current.dparm5             = param[4]                  ;
   current.dparm6             = param[5]                  ;
   current.dparm7             = param[6]                  ;
   current.dparm8             = param[7]                  ;
   current.dparm9             = param[8]                  ;
   current.dparm10         = param[9]                  ;
   current.fillcolor          = (short)fillcolor                 ;
   current.potential[0]    = potparam[0]               ;
   current.potential[1]    = potparam[1]               ;
   current.potential[2]    = potparam[2]               ;
   current.rflag              = (short)rflag                     ;
   current.rseed              = (short)rseed                     ;
   current.inside             = (short)inside                    ;
   current.logmap             = LogFlag                   ;
   current.invert[0]       = inversion[0]              ;
   current.invert[1]       = inversion[1]              ;
   current.invert[2]       = inversion[2]              ;
   current.decomp          = (short)decomp[0];                ;
   current.biomorph        = (short)biomorph                  ;
   current.symmetry           = (short)forcesymmetry             ;
   current.init3d[0]       = (short)init3d[0]                 ;
   current.init3d[1]       = (short)init3d[1]                 ;
   current.init3d[2]       = (short)init3d[2]                 ;
   current.init3d[3]       = (short)init3d[3]                 ;
   current.init3d[4]       = (short)init3d[4]                 ;
   current.init3d[5]       = (short)init3d[5]                 ;
   current.init3d[6]       = (short)init3d[6]                 ;
   current.init3d[7]       = (short)init3d[7]                 ;
   current.init3d[8]       = (short)init3d[8]                 ;
   current.init3d[9]       = (short)init3d[9]                 ;
   current.init3d[10]       = (short)init3d[10]               ;
   current.init3d[11]       = (short)init3d[12]               ;
   current.init3d[12]       = (short)init3d[13]               ;
   current.init3d[13]       = (short)init3d[14]               ;
   current.init3d[14]       = (short)init3d[15]               ;
   current.init3d[15]       = (short)init3d[16]               ;
   current.previewfactor   = (short)previewfactor             ;
   current.xtrans             = (short)xtrans                    ;
   current.ytrans             = (short)ytrans                    ;
   current.red_crop_left   = (short)red_crop_left             ;
   current.red_crop_right  = (short)red_crop_right            ;
   current.blue_crop_left  = (short)blue_crop_left            ;
   current.blue_crop_right = (short)blue_crop_right           ;
   current.red_bright      = (short)red_bright                ;
   current.blue_bright     = (short)blue_bright               ;
   current.xadjust            = (short)xadjust                   ;
   current.yadjust            = (short)yadjust                   ;
   current.eyeseparation   = (short)eyeseparation             ;
   current.glassestype     = (short)glassestype               ;
   current.outside            = (short)outside                   ;
   current.x3rd       = xx3rd                     ;
   current.y3rd       = yy3rd                     ;
   current.stdcalcmode     = usr_stdcalcmode               ;
   current.three_pass      = three_pass                ;
   current.stoppass        = (short)stoppass;
   current.distest            = distest                   ;
   current.trigndx[0]      = trigndx[0]                ;
   current.trigndx[1]      = trigndx[1]                ;
   current.trigndx[2]      = trigndx[2]                ;
   current.trigndx[3]      = trigndx[3]                ;
   current.finattract      = (short)finattract                ;
   current.initorbit[0]    = initorbit.x               ;
   current.initorbit[1]    = initorbit.y               ;
   current.useinitorbit    = useinitorbit              ;
   current.periodicity     = (short)periodicitycheck          ;
   current.pot16bit           = (short)disk16bit                 ;
   current.release            = (short)release                   ;
   current.save_release    = (short)save_release              ;
   current.flag3d             = (short)display3d                 ;
   current.ambient            = (short)Ambient                   ;
   current.randomize       = (short)RANDOMIZE                 ;
   current.haze       = (short)haze                      ;
   current.transparent[0]  = (short)transparent[0]            ;
   current.transparent[1]  = (short)transparent[1]            ;
   current.rotate_lo       = (short)rotate_lo                 ;
   current.rotate_hi       = (short)rotate_hi                 ;
   current.distestwidth    = (short)distestwidth              ;
   current.mxmaxfp         = mxmaxfp                   ;
   current.mxminfp         = mxminfp                   ;
   current.mymaxfp         = mymaxfp                   ;
   current.myminfp         = myminfp                   ;
   current.zdots           = (short)zdots                         ;
   current.originfp        = originfp                  ;
   current.depthfp         = depthfp                      ;
   current.heightfp        = heightfp                  ;
   current.widthfp         = widthfp                      ;
   current.distfp          = distfp                       ;
   current.eyesfp          = eyesfp                       ;
   current.orbittype       = (short)neworbittype              ;
   current.juli3Dmode      = (short)juli3Dmode                ;
   current.maxfn           = maxfn                     ;
   current.major_method    = (short)major_method              ;
   current.minor_method    = (short)minor_method              ;
   current.bailout         = bailout                   ;
   current.bailoutest      = (short)bailoutest                ;
   current.iterations      = maxit                     ;
   current.old_demm_colors = (short)old_demm_colors;
   current.logcalc         = (short)Log_Fly_Calc;
   far_memcpy(current.dac,dacbox,256*3);
   switch(fractype)
   {
   case FORMULA:
   case FFORMULA:
      far_strncpy(current.filename,FormFileName,80);
      far_strncpy(current.itemname,FormName,ITEMNAMELEN+1);
      break;
   case IFS:
   case IFS3D:
      far_strncpy(current.filename,IFSFileName,80);
      far_strncpy(current.itemname,IFSName,ITEMNAMELEN+1);
      break;
   case LSYSTEM:
      far_strncpy(current.filename,LFileName,80);
      far_strncpy(current.itemname,LName,ITEMNAMELEN+1);
      break;
   default:
      *(current.filename) = 0;
      *(current.itemname) = 0;
      break;
   }
   if (historyptr == -1)        /* initialize the history file */
   {
      int i;
      for (i = 0; i < maxhistory; i++)
         history[i] = current;
      historyflag = saveptr = historyptr = 0;   /* initialize history ptr */
   }
   else if(historyflag == 1)
      historyflag = 0;   /* coming from user history command, don't save */
   else if(far_memcmp(&current,&history[saveptr],sizeof(HISTORY)))
   {
      char msg[80];
      if(++saveptr >= maxhistory)  /* back to beginning of circular buffer */
         saveptr = 0;
      if(++historyptr >= maxhistory)  /* move user pointer in parallel */
         historyptr = 0;
      history[saveptr] = current;
      sprintf(msg,"saving image in history %d",saveptr);
      /* stopmsg(0,msg); */
   }
}

static void _fastcall restore_history_info(int i)
{
   char msg[80];
   sprintf(msg,"restoring msg %d",i);
   /* stopmsg(0,msg); */
   if(maxhistory <= 0 || bf_math)
      return;
   invert = 0;
   calc_status = 0;
   resuming = 0;
   fractype              = history[i].fractal_type   ;
   xxmin                 = history[i].xmin           ;
   xxmax                 = history[i].xmax           ;
   yymin                 = history[i].ymin           ;
   yymax                 = history[i].ymax           ;
   param[0]              = history[i].creal          ;
   param[1]              = history[i].cimag          ;
   param[2]              = history[i].dparm3         ;
   param[3]              = history[i].dparm4         ;
   param[4]              = history[i].dparm5         ;
   param[5]              = history[i].dparm6         ;
   param[6]              = history[i].dparm7         ;
   param[7]              = history[i].dparm8         ;
   param[8]              = history[i].dparm9         ;
   param[9]              = history[i].dparm10        ;
   fillcolor             = history[i].fillcolor      ;
   potparam[0]           = history[i].potential[0]   ;
   potparam[1]           = history[i].potential[1]   ;
   potparam[2]           = history[i].potential[2]   ;
   rflag                 = history[i].rflag          ;
   rseed                 = history[i].rseed          ;
   inside                = history[i].inside         ;
   LogFlag               = history[i].logmap         ;
   inversion[0]          = history[i].invert[0]      ;
   inversion[1]          = history[i].invert[1]      ;
   inversion[2]          = history[i].invert[2]      ;
   decomp[0]             = history[i].decomp         ;
   usr_biomorph          = history[i].biomorph       ;
   biomorph              = history[i].biomorph       ;
   forcesymmetry         = history[i].symmetry       ;
   init3d[0]             = history[i].init3d[0]      ;
   init3d[1]             = history[i].init3d[1]      ;
   init3d[2]             = history[i].init3d[2]      ;
   init3d[3]             = history[i].init3d[3]      ;
   init3d[4]             = history[i].init3d[4]      ;
   init3d[5]             = history[i].init3d[5]      ;
   init3d[6]             = history[i].init3d[6]      ;
   init3d[7]             = history[i].init3d[7]      ;
   init3d[8]             = history[i].init3d[8]      ;
   init3d[9]             = history[i].init3d[9]      ;
   init3d[10]            = history[i].init3d[10]     ;
   init3d[12]            = history[i].init3d[11]     ;
   init3d[13]            = history[i].init3d[12]     ;
   init3d[14]            = history[i].init3d[13]     ;
   init3d[15]            = history[i].init3d[14]     ;
   init3d[16]            = history[i].init3d[15]     ;
   previewfactor         = history[i].previewfactor  ;
   xtrans                = history[i].xtrans         ;
   ytrans                = history[i].ytrans         ;
   red_crop_left         = history[i].red_crop_left  ;
   red_crop_right        = history[i].red_crop_right ;
   blue_crop_left        = history[i].blue_crop_left ;
   blue_crop_right       = history[i].blue_crop_right;
   red_bright            = history[i].red_bright     ;
   blue_bright           = history[i].blue_bright    ;
   xadjust               = history[i].xadjust        ;
   yadjust               = history[i].yadjust        ;
   eyeseparation         = history[i].eyeseparation  ;
   glassestype           = history[i].glassestype    ;
   outside               = history[i].outside        ;
   xx3rd                 = history[i].x3rd           ;
   yy3rd                 = history[i].y3rd           ;
   usr_stdcalcmode       = history[i].stdcalcmode    ;
   stdcalcmode           = history[i].stdcalcmode    ;
   three_pass            = history[i].three_pass     ;
   stoppass              = history[i].stoppass       ;
   distest               = history[i].distest        ;
   usr_distest           = history[i].distest        ;
   trigndx[0]            = history[i].trigndx[0]     ;
   trigndx[1]            = history[i].trigndx[1]     ;
   trigndx[2]            = history[i].trigndx[2]     ;
   trigndx[3]            = history[i].trigndx[3]     ;
   finattract            = history[i].finattract     ;
   initorbit.x           = history[i].initorbit[0]   ;
   initorbit.y           = history[i].initorbit[1]   ;
   useinitorbit          = history[i].useinitorbit   ;
   periodicitycheck      = history[i].periodicity    ;
   usr_periodicitycheck  = history[i].periodicity    ;
   disk16bit             = history[i].pot16bit       ;
   release               = history[i].release        ;
   save_release          = history[i].save_release   ;
   display3d             = history[i].flag3d         ;
   Ambient               = history[i].ambient        ;
   RANDOMIZE             = history[i].randomize      ;
   haze                  = history[i].haze           ;
   transparent[0]        = history[i].transparent[0] ;
   transparent[1]        = history[i].transparent[1] ;
   rotate_lo             = history[i].rotate_lo      ;
   rotate_hi             = history[i].rotate_hi      ;
   distestwidth          = history[i].distestwidth   ;
   mxmaxfp               = history[i].mxmaxfp        ;
   mxminfp               = history[i].mxminfp        ;
   mymaxfp               = history[i].mymaxfp        ;
   myminfp               = history[i].myminfp        ;
   zdots                 = history[i].zdots          ;
   originfp              = history[i].originfp       ;
   depthfp               = history[i].depthfp        ;
   heightfp              = history[i].heightfp       ;
   widthfp               = history[i].widthfp        ;
   distfp                = history[i].distfp         ;
   eyesfp                = history[i].eyesfp         ;
   neworbittype          = history[i].orbittype      ;
   juli3Dmode            = history[i].juli3Dmode     ;
   maxfn                 = history[i].maxfn          ;
   major_method          = (enum Major)history[i].major_method   ;
   minor_method          = (enum Minor)history[i].minor_method   ;
   bailout               = history[i].bailout        ;
   bailoutest            = (enum bailouts)history[i].bailoutest     ;
   maxit                 = history[i].iterations     ;
   old_demm_colors       = history[i].old_demm_colors;
   curfractalspecific    = &fractalspecific[fractype];
   potflag               = (potparam[0] != 0.0);
   if (inversion[0] != 0.0)
      invert = 3;
   Log_Fly_Calc = history[i].logcalc;
   usr_floatflag = (char)((curfractalspecific->isinteger) ? 0 : 1);
   far_memcpy(dacbox,history[i].dac,256*3);
   far_memcpy(olddacbox,history[i].dac,256*3);
   if(mapdacbox)
      far_memcpy(mapdacbox,history[i].dac,256*3);
   spindac(0,1);
   if(fractype == JULIBROT || fractype == JULIBROTFP)
      savedac = 0;
   else
      savedac = 1;
   switch(fractype)
   {
   case FORMULA:
   case FFORMULA:
      far_strncpy(FormFileName,history[i].filename,80);
      far_strncpy(FormName,    history[i].itemname,ITEMNAMELEN+1);
      break;
   case IFS:
   case IFS3D:
      far_strncpy(IFSFileName,history[i].filename,80);
      far_strncpy(IFSName    ,history[i].itemname,ITEMNAMELEN+1);
      break;
   case LSYSTEM:
      far_strncpy(LFileName,history[i].filename,80);
      far_strncpy(LName    ,history[i].itemname,ITEMNAMELEN+1);
      break;
   default:
      break;
   }
}

void checkfreemem(int secondpass)
{
   int oldmaxhistory;
   char far *tmp;
   static FCODE msg[] =
      {" I'm sorry, but you don't have enough free memory \n to run this program.\n\n"};
   static FCODE msg2[] = {"To save memory, reduced maxhistory to "};
   tmp = farmemalloc(4096L);
   oldmaxhistory = maxhistory;
   if(secondpass && !history)
   {
      while(maxhistory >= 0) /* decrease history if necessary */
      {
         history = (HISTORY far *)
            farmemalloc(((unsigned long)maxhistory * sizeof(HISTORY)));
         if(history)
            break;
         maxhistory--;
      }
   }
   if(extraseg == 0 || tmp == NULL)
   {
      buzzer(2);
#ifndef XFRACT
      printf("%Fs",(char far *)msg);
#else
      printf("%s",msg);
#endif
      exit(1);
   }
   farmemfree(tmp); /* was just to check for min space */
   if(secondpass && maxhistory < oldmaxhistory)
   {
#ifndef XFRACT
      printf("%Fs%d\n%Fs\n",(char far *)msg2,maxhistory,s_pressanykeytocontinue);
#else
      printf("%s%d\n%s\n",(char far *)msg2,maxhistory,s_pressanykeytocontinue);
#endif
      getakey();
   }
}
