/*
	Resident odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#ifndef XFRACT
#include <stdarg.h>
#include <io.h>
#else
#include <varargs.h>
#endif
#include <math.h>
#ifdef __TURBOC__
#include <dir.h>
#endif
#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"
#include "prototyp.h"

/* routines in this module	*/

static	void trigdetails(char *);
static void area();

int active_ovly = -1;

extern char IFSFileName[80];
extern char IFSName[40];
extern float far *ifs_defn;
extern int  ifs_changed;
extern int  ifs_type;
extern int neworbittype;
extern char temp[], temp1[256];   /* temporary strings	      */

extern int  active_ovly;
extern int  xdots, ydots;
extern int  dotmode;
extern int  show_orbit;
extern int  debugflag;
extern int  maxit;
extern int  fractype;
extern int  got_status,curpass,totpasses,currow,curcol;
extern int  fileydots;
extern int  xxstart,xxstop,yystart,yystop;
extern int  display3d;
extern char overwrite;
extern int  inside;
extern int  outside;
extern double xxmax,xxmin,yymax,yymin,xx3rd,yy3rd;


/* TW's static string consolidation campaign to help brain-dead compilers */
char s_cantopen[]       = {"Can't open %s"};
char s_cantwrite[]      = {"Can't write %s"};
char s_cantcreate[]     = {"Can't create %s"};
char s_cantunderstand[] = {"Can't understand %s"};
char s_cantfind[]       = {"Can't find %s"};

/* call next when returning from resident routine and unsure whether
   caller is an overlay which has been displaced */
void restore_active_ovly()
{

   switch (active_ovly) {
      case OVLY_MISCOVL:  miscovl_overlay();  break;
      case OVLY_CMDFILES: cmdfiles_overlay(); break;
      case OVLY_HELP:	  help_overlay();     break;
      case OVLY_PROMPTS1: prompts1_overlay(); break;
      case OVLY_PROMPTS2: prompts2_overlay(); break;
      case OVLY_LOADFILE: loadfile_overlay(); break;
      case OVLY_ROTATE:   rotate_overlay();   break;
      case OVLY_PRINTER:  printer_overlay();  break;
      case OVLY_LINE3D:   line3d_overlay();   break;
      case OVLY_ENCODER:  encoder_overlay();  break;
      case OVLY_CALCFRAC: calcfrac_overlay(); break;
      case OVLY_INTRO:	  intro_overlay();    break;
      case OVLY_DECODER:  decoder_overlay();  break;
      }
}


#ifndef XFRACT
void findpath(char *filename, char *fullpathname) /* return full pathnames */
{
   if (filename[0] == SLASHC
     || (filename[0] && filename[1] == ':')) {
      strcpy(fullpathname,filename);
      return;
      }
   fullpathname[0] = 0; 			/* indicate none found */
#ifdef __TURBOC__				/* look for the file */
   strcpy(fullpathname,searchpath(filename));
#else
   _searchenv(filename,"PATH",fullpathname);
#endif
   if (fullpathname[0] != 0)			/* found it! */
      if (strncmp(&fullpathname[2],SLASHSLASH,2) == 0) /* stupid klooge! */
	 strcpy(&fullpathname[3],filename);
}
#endif


void notdiskmsg()
{
static char far sorrymsg[]={
"This type may be slow using a real-disk based 'video' mode, but may not \n\
be too bad if you have enough expanded or extended memory. Press <Esc> to \n\
abort if it appears that your disk drive is working too hard."};
   stopmsg(0,sorrymsg);
}


/* convert corners to center/mag */
int cvtcentermag(double *Xctr, double *Yctr, double *Magnification)
{
   double Width, Height, Radius, Ratio;
   Width  = xxmax - xxmin;
   Height = yymax - yymin;
   Ratio = Height / Width;
   if(xx3rd != xxmin || yy3rd != yymin || Width < 0
     || (Width > 1e-8 && (Ratio <= 0.74 || Ratio >= 0.76))
     || Ratio < 0.66 || Ratio > 0.84)
      return(0);
   /* calculate center and magnification */
   Radius = Height / 2.0;
   *Xctr = xxmin + (Width / 2.0);
   *Yctr = yymin + Radius;
   *Magnification = 1.0 / Radius;
   return(1);
}


void updatesavename(char *filename) /* go to the next file name */
{
   char *save, *hold;
   char name[80],suffix[80];
   char *dotptr;

   strcpy(name,filename);
   suffix[0] = 0;
   if ((dotptr = strrchr(name,'.')) != NULL
     && dotptr > strrchr(name,SLASHC)) {
      strcpy(suffix,dotptr);
      *dotptr = 0;
      }

   hold = name + strlen(name) - 1; /* start at the end */
   while(hold >= name && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
      hold--;
   hold++;			/* recover first digit */
   while (*hold == '0')         /* skip leading zeros */
      hold++;
   save = hold;
   while (*save) {		/* check for all nines */
      if (*save != '9')
	 break;
      save++;
      }
   if (!*save)			/* if the whole thing is nines then back */
      save = hold - 1;		/* up one place. Note that this will eat */
				/* your last letter if you go to far.	 */
   else
      save = hold;
   sprintf(save,"%d",atoi(hold)+1); /* increment the number */
   strcpy(filename,name);
   strcat(filename,suffix);
}

int check_writefile(char *name,char *ext)
{
 /* after v16 release, change encoder.c to also use this routine */
   char openfile[80];
   char opentype[20];
   int i;
nextname:
   strcpy(openfile,name);
   strcpy(opentype,ext);
   for (i = 0; i < strlen(openfile); i++)
      if (openfile[i] == '.') {
	 strcpy(opentype,&openfile[i]);
	 openfile[i] = 0;
	 }
   strcat(openfile,opentype);
   if (access(openfile,0) != 0) /* file doesn't exist */
   {
      strcpy(name,openfile);
      return 0;
    }
   /* file already exists */
   if (overwrite == 0) {
      updatesavename(name);
      goto nextname;
      }
   return 1;
}

/* ('check_key()' was moved to FRACTINT.C for MSC7-overlay speed purposes) */
/* ('timer()'     was moved to FRACTINT.C for MSC7-overlay speed purposes) */

BYTE trigndx[] = {SIN,SQR,SINH,COSH};
#ifndef XFRACT
void (*ltrig0)(void) = lStkSin;
void (*ltrig1)(void) = lStkSqr;
void (*ltrig2)(void) = lStkSinh;
void (*ltrig3)(void) = lStkCosh;
void (*mtrig0)(void) = mStkSin;
void (*mtrig1)(void) = mStkSqr;
void (*mtrig2)(void) = mStkSinh;
void (*mtrig3)(void) = mStkCosh;
#endif
void (*dtrig0)(void) = dStkSin;
void (*dtrig1)(void) = dStkSqr;
void (*dtrig2)(void) = dStkSinh;
void (*dtrig3)(void) = dStkCosh;

struct trig_funct_lst trigfn[] =
/* changing the order of these alters meaning of *.fra file */
/* maximum 6 characters in function names or recheck all related code */
{
#ifndef XFRACT
   {"sin",   lStkSin,   dStkSin,   mStkSin   },
   {"cosxx", lStkCosXX, dStkCosXX, mStkCosXX },
   {"sinh",  lStkSinh,  dStkSinh,  mStkSinh  },
   {"cosh",  lStkCosh,  dStkCosh,  mStkCosh  },
   {"exp",   lStkExp,   dStkExp,   mStkExp   },
   {"log",   lStkLog,   dStkLog,   mStkLog   },
   {"sqr",   lStkSqr,   dStkSqr,   mStkSqr   },
   {"recip", lStkRecip, dStkRecip, mStkRecip }, /* from recip on new in v16 */
   {"ident", StkIdent,  StkIdent,  StkIdent  },
   {"cos",   lStkCos,   dStkCos,   mStkCos   },
   {"tan",   lStkTan,   dStkTan,   mStkTan   },
   {"tanh",  lStkTanh,  dStkTanh,  mStkTanh  },
   {"cotan", lStkCoTan, dStkCoTan, mStkCoTan },
   {"cotanh",lStkCoTanh,dStkCoTanh,mStkCoTanh},
   {"flip",  lStkFlip,  dStkFlip,  mStkFlip  },
   {"conj",  lStkConj,  dStkConj,  mStkConj  },
   {"zero",  lStkZero,  dStkZero,  mStkZero  },
#else
   {"sin",   dStkSin,   dStkSin,   dStkSin   },
   {"cosxx", dStkCosXX, dStkCosXX, dStkCosXX },
   {"sinh",  dStkSinh,  dStkSinh,  dStkSinh  },
   {"cosh",  dStkCosh,  dStkCosh,  dStkCosh  },
   {"exp",   dStkExp,   dStkExp,   dStkExp   },
   {"log",   dStkLog,   dStkLog,   dStkLog   },
   {"sqr",   dStkSqr,   dStkSqr,   dStkSqr   },
   {"recip", dStkRecip, dStkRecip, dStkRecip }, /* from recip on new in v16 */
   {"ident", StkIdent,  StkIdent,  StkIdent  },
   {"cos",   dStkCos,   dStkCos,   dStkCos   },
   {"tan",   dStkTan,   dStkTan,   dStkTan   },
   {"tanh",  dStkTanh,  dStkTanh,  dStkTanh  },
   {"cotan", dStkCoTan, dStkCoTan, dStkCoTan },
   {"cotanh",dStkCoTanh,dStkCoTanh,dStkCoTanh},
   {"flip",  dStkFlip,  dStkFlip,  dStkFlip  },
   {"conj",  dStkConj,  dStkConj,  dStkConj  },
   {"zero",  dStkZero,  dStkZero,  dStkZero  },
#endif
};
int numtrigfn = sizeof(trigfn)/sizeof(struct trig_funct_lst);

void showtrig(char *buf) /* return display form of active trig functions */
{
   char tmpbuf[30];
   *buf = 0; /* null string if none */
   trigdetails(tmpbuf);
   if (tmpbuf[0])
      sprintf(buf," function=%s",tmpbuf);
}

static void trigdetails(char *buf)
{
   extern char maxfn;
   int i, numfn;
   char tmpbuf[20];
   if(fractype==JULIBROT || fractype==JULIBROTFP)
      numfn = (fractalspecific[neworbittype].flags >> 6) & 7;
   else
      numfn = (curfractalspecific->flags >> 6) & 7;
   if(curfractalspecific == &fractalspecific[FORMULA] ||
      curfractalspecific == &fractalspecific[FFORMULA]	)
      numfn = maxfn;
   *buf = 0; /* null string if none */
   if (numfn>0) {
      strcpy(buf,trigfn[trigndx[0]].name);
      i = 0;
      while(++i < numfn) {
	 sprintf(tmpbuf,"/%s",trigfn[trigndx[i]].name);
	 strcat(buf,tmpbuf);
	 }
      }
}

/* set array of trig function indices according to "function=" command */
int set_trig_array(int k, char *name)
{
   char trigname[10];
   int i;
   char *slash;
   strncpy(trigname,name,6);
   trigname[6] = 0; /* safety first */

   if ((slash = strchr(trigname,'/')))
      *slash = 0;

   strlwr(trigname);

   for(i=0;i<numtrigfn;i++)
   {
      if(strcmp(trigname,trigfn[i].name)==0)
      {
	 trigndx[k] = i;
	 set_trig_pointers(k);
	 break;
      }
   }
   return(0);
}
void set_trig_pointers(int which)
{
  /* set trig variable functions to avoid array lookup time */
   int i;
   switch(which)
   {
   case 0:
#ifndef XFRACT
      ltrig0 = trigfn[trigndx[0]].lfunct;
      mtrig0 = trigfn[trigndx[0]].mfunct;
#endif
      dtrig0 = trigfn[trigndx[0]].dfunct;
      break;
   case 1:
#ifndef XFRACT
      ltrig1 = trigfn[trigndx[1]].lfunct;
      mtrig1 = trigfn[trigndx[1]].mfunct;
#endif
      dtrig1 = trigfn[trigndx[1]].dfunct;
      break;
   case 2:
#ifndef XFRACT
      ltrig2 = trigfn[trigndx[2]].lfunct;
      mtrig2 = trigfn[trigndx[2]].mfunct;
#endif
      dtrig2 = trigfn[trigndx[2]].dfunct;
      break;
   case 3:
#ifndef XFRACT
      ltrig3 = trigfn[trigndx[3]].lfunct;
      mtrig3 = trigfn[trigndx[3]].mfunct;
#endif
      dtrig3 = trigfn[trigndx[3]].dfunct;
      break;
   default: /* do 'em all */
      for(i=0;i<4;i++)
	 set_trig_pointers(i);
      break;
   }
}


int tab_display()	/* display the status of the current image */
{
/* TW's static string consolidation campaign to help brain-dead compilers */
   static char far sfractal_type[] =     {"Fractal type:"};
   static char far s3D_transform[] =     {"3D Transform"};
   static char far syou_are_cycling[] =  {"You are in color-cycling mode"};
   static char far sfloating_point[] =   {"Floating-point"};
   static char far sruns_forever[] =     {"Note: this type runs forever."};
   static char far ssolid_guessing[] =   {"Solid Guessing"};
   static char far sboundary_tracing[] = {"Boundary Tracing"};
   static char far stesseral[] =         {"Tesseral"};
   static char far scalculation_time[] = {"Calculation time:"};
   static char far scornersxy[] =        {"Corners:                X                     Y"};
   static char far stop_left[] =         {"top-left"};
   static char far sbottom_right[] =     {"bottom-right"};
   static char far scenter[] =           {"Center: "};
   static char far smag[] =              {"  Mag: "};
   static char far sbottom_left[] =      {"bottom-left"};
   static char far sparams[] =           {"Params,"};
   static char far siteration_maximum[] ={"Iteration maximum: "};
   static char far seffective_bailout[] ={"     Effective bailout: "};
   static char far scurrent_rseed[] =    {"Current 'rseed': "};
   static char far sinversion_radius[] = {"Inversion radius: "};
   static char far sxcenter[] =          {"  xcenter: "};
   static char far sycenter[] =          {"  ycenter: "};
   static char far sparms_chgd[] = {"Parms chgd since generated"};
   static char far sstill_being[] = {"Still being generated"};
   static char far sinterrupted_resumable[] = {"Interrupted, resumable"};
   static char far sinterrupted_non_resumable[] = {"Interrupted, non-resumable"};
   static char far simage_completed[] = {"Image completed"};
   static char far sflag_is_activated[] = {" flag is activated"};
   static char far sinteger_math[]      = {"Integer math is in use"};
   static char far sin_use_required[] = {" in use (required)"};

   extern char floatflag;
   extern char usr_floatflag;
   extern double param[];
   extern double rqlim;
   extern long calctime, timer_start;
   extern int  calc_status;
   extern char FormName[];
   extern char LName[];
   extern char IFSName[];
   extern int  rseed;
   extern int  invert;
   int row, i, j;
   double Xctr, Yctr, Magnification;
   char msg[81];
   char far *msgptr;
   int key;

   if (calc_status < 0) 	/* no active fractal image */
      return(0);		/* (no TAB on the credits screen) */
   if (calc_status == 1)	/* next assumes CLK_TCK is 10^n, n>=2 */
      calctime += (clock_ticks() - timer_start) / (CLK_TCK/100);
   stackscreen();
top:
   helptitle();
   setattr(1,0,C_GENERAL_MED,24*80); /* init rest to background */

   row = 2;
   putstring(row,2,C_GENERAL_MED,sfractal_type);
   if (display3d > 0)
      putstring(row,16,C_GENERAL_HI,s3D_transform);
   else {
      putstring(row,16,C_GENERAL_HI,
	   curfractalspecific->name[0] == '*' ?
	     &curfractalspecific->name[1] :
	     curfractalspecific->name);
      i = 0;
      if (fractype == FORMULA || fractype == FFORMULA)
      {
	 putstring(row+1,16,C_GENERAL_HI,FormName);
      i = strlen(FormName)+1;
      }
      trigdetails(msg);
      putstring(row+1,16+i,C_GENERAL_HI,msg);
      if (fractype == LSYSTEM)
	 putstring(row+1,16,C_GENERAL_HI,LName);
      if (fractype == IFS || fractype == IFS3D)
	 putstring(row+1,16,C_GENERAL_HI,IFSName);
      }

   switch (calc_status) {
      case 0:  msgptr = sparms_chgd;
	       break;
      case 1:  msgptr = sstill_being;
	       break;
      case 2:  msgptr = sinterrupted_resumable;
	       break;
      case 3:  msgptr = sinterrupted_non_resumable;
	       break;
      case 4:  msgptr = simage_completed;
	       break;
      default: msgptr = "";
      }
   putstring(row,45,C_GENERAL_HI,msgptr);
   if (helpmode == HELPCYCLING)
      putstring(row+1,45,C_GENERAL_HI,syou_are_cycling);
   row += 2;

    i = j = 0;
    if (display3d > 0) {
       if (usr_floatflag)
	  j = 1;
       }
    else
       if (floatflag)
	  j = (usr_floatflag) ? 1 : 2;
    if (j) {
       putstring(row,45,C_GENERAL_HI,sfloating_point);
 
       putstring(-1,-1,C_GENERAL_HI,(j == 1) ? sflag_is_activated
					     : sin_use_required );
      i = 1;
      }
      else
      {
       putstring(row,45,C_GENERAL_HI,sinteger_math);
      i = 1;
      }
   if (calc_status == 1 || calc_status == 2)
      if (curfractalspecific->flags&INFCALC) {
	 putstring(row,2,C_GENERAL_HI,sruns_forever);
	 i = 1;
	 }
   row += i;

   if (calc_status == 1 || calc_status == 2)
      if (curfractalspecific->flags&NORESUME)
      {
	 static char far msg[] = {"Note: can't resume this type after interrupts other than <tab> and <F1>"};
	 putstring(row++,2,C_GENERAL_HI,msg);
      }
   ++row;

   if (got_status >= 0 && (calc_status == 1 || calc_status == 2)) {
      switch (got_status) {
	 case 0:
	    sprintf(msg,"%d Pass Mode",totpasses);
	    putstring(row,2,C_GENERAL_HI,msg);
	    break;
	 case 1:
	    putstring(row,2,C_GENERAL_HI,ssolid_guessing);
	    break;
	 case 2:
	    putstring(row,2,C_GENERAL_HI,sboundary_tracing);
	    break;
	 case 3:
	    sprintf(msg,"Processing row %d (of %d) of input image",currow,fileydots);
	    putstring(row,2,C_GENERAL_HI,msg);
	    break;
	 case 4:
	    putstring(row,2,C_GENERAL_HI,stesseral);
	    break;
	 }
      ++row;
      if (got_status != 3) {
	 sprintf(msg,"Working on block (y,x) [%d,%d]...[%d,%d], ",
		yystart,xxstart,yystop,xxstop);
	 putstring(row,2,C_GENERAL_MED,msg);
	 if (got_status == 2 || got_status == 4) { /* btm or tesseral */
	    putstring(-1,-1,C_GENERAL_MED,"at ");
	    sprintf(msg,"[%d,%d]",currow,curcol);
	    putstring(-1,-1,C_GENERAL_HI,msg);
	    }
	 else {
	    if (totpasses > 1) {
	       putstring(-1,-1,C_GENERAL_MED,"pass ");
	       sprintf(msg,"%d",curpass);
	       putstring(-1,-1,C_GENERAL_HI,msg);
	       putstring(-1,-1,C_GENERAL_MED," of ");
	       sprintf(msg,"%d",totpasses);
	       putstring(-1,-1,C_GENERAL_HI,msg);
	       putstring(-1,-1,C_GENERAL_MED,", ");
	       }
	    putstring(-1,-1,C_GENERAL_MED,"at row ");
	    sprintf(msg,"%d",currow);
	    putstring(-1,-1,C_GENERAL_HI,msg);
	    }
	 ++row;
	 }
      }
   putstring(row,2,C_GENERAL_MED,scalculation_time);
   sprintf(msg,"%3ld:%02ld:%02ld.%02ld", calctime/360000,
	  (calctime%360000)/6000, (calctime%6000)/100, calctime%100);
   putstring(-1,-1,C_GENERAL_HI,msg);
   row += 2;

   if (videoentry.xdots) {
      sprintf(msg,"Video: %dx%dx%d %s %s",
	      videoentry.xdots, videoentry.ydots, videoentry.colors,
	      videoentry.name, videoentry.comment);
      putstring(row,2,C_GENERAL_MED,msg);
      }
   ++row;

   putstring(row,2,C_GENERAL_MED,scornersxy);
   putstring(++row,3,C_GENERAL_MED,stop_left);
   sprintf(msg,"%20.16f  %20.16f",xxmin,yymax);
   putstring(-1,17,C_GENERAL_HI,msg);
   putstring(++row,3,C_GENERAL_MED,sbottom_right);
   sprintf(msg,"%20.16f  %20.16f",xxmax,yymin);
   putstring(-1,17,C_GENERAL_HI,msg);
   adjust_corner(); /* make bottom left exact if very near exact */
   if (cvtcentermag(&Xctr, &Yctr, &Magnification)) {
      putstring(row+=2,2,C_GENERAL_MED,scenter);
      sprintf(msg,"%20.16f %20.16f",Xctr,Yctr);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,smag);
      if (Magnification < 1e6)
	 sprintf(msg,"%20.14f",Magnification);
      else if (Magnification < 1e12)
	 sprintf(msg,"%20.8f",Magnification);
      else
	 sprintf(msg,"%20.2f",Magnification);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }
   else if (xxmin != xx3rd || yymin != yy3rd) {
      putstring(++row,3,C_GENERAL_MED,sbottom_left);
      sprintf(msg,"%20.16f  %20.16f",xx3rd,yy3rd);
      putstring(-1,17,C_GENERAL_HI,msg);
      }
   putstring(row+=2,2,C_GENERAL_MED,sparams);
   for (i = 0; i < 4; i++) {
      sprintf(msg,"%3d: ",i+1);
      putstring(-1,-1,C_GENERAL_MED,msg);
      sprintf(msg,"%12.9f",param[i]);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   putstring(row+=2,2,C_GENERAL_MED,siteration_maximum);
   sprintf(msg,"%d",maxit);
   putstring(-1,-1,C_GENERAL_HI,msg);
   putstring(-1,-1,C_GENERAL_MED,seffective_bailout);
   sprintf(msg,"%f",rqlim);
   putstring(-1,-1,C_GENERAL_HI,msg);

   if (fractype == PLASMA) {
      putstring(++row,2,C_GENERAL_MED,scurrent_rseed);
      sprintf(msg,"%d",rseed);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   if(invert) {
      extern double f_radius,f_xcenter,f_ycenter;
      putstring(++row,2,C_GENERAL_MED,sinversion_radius);
      sprintf(msg,"%12.9f",f_radius);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,sxcenter);
      sprintf(msg,"%12.9f",f_xcenter);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,sycenter);
      sprintf(msg,"%12.9f",f_ycenter);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   if ((row += 2) < 23) ++row;
waitforkey:
   putstringcenter(row,0,80,C_GENERAL_LO,
       "...Press any key to continue, F6 for area...");
   movecursor(25,80);
#ifdef XFRACT
   while (keypressed()) {
       getakey();
   }
#endif
   key = getakeynohelp();
   if (key==F6) {
       unstackscreen();
       area();
       stackscreen();
/*       goto waitforkey;*/
        goto top;
   }
   unstackscreen();
   timer_start = clock_ticks(); /* tab display was "time out" */
   return(0);
}

static void area()
{
    /* apologies to UNIX folks, we PC guys have to save near space */
    static char far warning[] = {"Warning: inside may not be unique\n"};
    static char far total_area[] = {".  Total area "}; 
    char far *msg;
    int x,y;
    char buf[160];
    long cnt=0;
    if (inside<0) {
      static char far msg[] = {"Need solid inside to compute area"};
      stopmsg(0,msg);
      return;
    }
    for (y=0;y<ydots;y++) {
      for (x=0;x<xdots;x++) {
          if (getcolor(x,y)==inside) {
              cnt++;
          }
      }
    }
    if (inside>0 && outside<0 && maxit>inside) {
      msg = warning;
    } else {
      msg = (char far *)"";
    }
#ifndef XFRACT
      sprintf(buf,"%Fs%ld inside pixels of %ld%Fs%f",
              msg,cnt,(long)xdots*(long)ydots,total_area,
              cnt/((float)xdots*(float)ydots)*(xxmax-xxmin)*(yymax-yymin));
#else
      sprintf(buf,"%s%ld inside pixels of %ld%s%f",
              msg,cnt,(long)xdots*(long)ydots,total_area,
              cnt/((float)xdots*(float)ydots)*(xxmax-xxmin)*(yymax-yymin));
#endif
    stopmsg(4,buf);
}

int endswithslash(char *fl)
{
   int len;
   len = strlen(fl);
   if(len)
      if(fl[--len] == SLASHC)
	 return(1);
   return(0);
}

char far insufficient_ifs_mem[]={"Insufficient memory for IFS"};
/* --------------------------------------------------------------------- */
int numaffine;
int ifsload()			/* read in IFS parameters */
{
   int i;
   FILE *ifsfile;
   char buf[201];
   char *bufptr;
   extern float tstack[];	/* shared temp */
   int ret,rowsize;

   if (ifs_defn) { /* release prior parms */
      farmemfree((char far *)ifs_defn);
      ifs_defn = NULL;
      }

   ifs_changed = ifs_type = 0;
   rowsize = IFSPARM;
   if (find_file_item(IFSFileName,IFSName,&ifsfile) < 0)
      return(-1);

   fgets(buf,200,ifsfile);
   strlwr(buf);
   bufptr = &buf[0];
   while (*bufptr) {
      if (strncmp(bufptr,"(3d)",4) == 0) {
	 ifs_type = 1;
	 rowsize = IFS3DPARM;
	 }
      ++bufptr;
      }

   for (i = 0; i < (NUMIFS+1)*IFS3DPARM; ++i)
      tstack[i] = 0.0;
   i = ret = 0;
   while (fscanf(ifsfile," %f ",&tstack[i])) {
      if (++i >= NUMIFS*rowsize) {
      static char far msg[]={"IFS definition has too many lines"};
	 stopmsg(0,msg);
	 ret = -1;
	 break;
	 }
      }
   if ((i % rowsize) != 0 || getc(ifsfile) != '}') {
      static char far msg[]={"invalid IFS definition"};
      stopmsg(0,msg);
      ret = -1;
      }
   if (i == 0 && ret == 0) {
      static char far msg[]={"Empty IFS definition"};
      stopmsg(0,msg);
      ret = -1;
      }
   fclose(ifsfile);

   if (ret == 0) {
      numaffine = i;
      if ((ifs_defn = (float far *)farmemalloc(
			(long)((NUMIFS+1)*IFS3DPARM*sizeof(float)))) == NULL) {
     stopmsg(0,insufficient_ifs_mem);
	 ret = -1;
	 }
      else
	 for (i = 0; i < (NUMIFS+1)*IFS3DPARM; ++i)
	    ifs_defn[i] = tstack[i];
   }
   return(ret);
}

int find_file_item(char *filename,char *itemname,FILE **infile)
{
   char tmpname[41];
   long notepoint;
   char buf[201];
   int c;
   if ((*infile = fopen(filename,"rt")) == NULL) {
      sprintf(buf,s_cantopen,filename);
      stopmsg(0,buf);
      return(-1);
      }

   while (1) {
      while ((c = getc(*infile)) == ' ' || c == '\t' || c == '\n') { }
      if (c == EOF) break;
      if (c == ';') {
	 while ((c = fgetc(*infile)) != '\n' && c != EOF) { }
	 if (c == EOF) break;
	 continue;
	 }
      notepoint = ftell(*infile) - 1;
      ungetc(c,*infile);
      if (fscanf(*infile," %40[^ \n\t({]",tmpname) == EOF) break;
      while ((c = getc(*infile)) != EOF && c != '{' && c != '\n') { }
      if (c == EOF) break;
      if (c == '{') {
	 if (stricmp(tmpname,itemname) == 0) {
	    fseek(*infile,notepoint,SEEK_SET);
	    return(0);
	    }
	 while ((c = getc(*infile)) != '}' && c != EOF) { }
	 if (c == EOF) break;
	 }
      }
   fclose(*infile);
   sprintf(buf,"'%s' definition not found",itemname);
   stopmsg(0,buf);
   return(-1);
}

int file_gets(char *buf,int maxlen,FILE *infile)
{
   int len,c;
   /* similar to 'fgets', but file may be in either text or binary mode */
   /* returns -1 at eof, length of string otherwise */
   if (feof(infile)) return -1;
   len = 0;
   while (len < maxlen) {
      if ((c = getc(infile)) == EOF || c == '\032') {
	 if (len) break;
	 return -1;
	 }
      if (c == '\n') break;             /* linefeed is end of line */
      if (c != '\r') buf[len++] = c;    /* ignore c/r */
      }
   buf[len] = 0;
   return len;
}

int first_err = 1;

#ifndef XFRACT
#ifdef WINFRACT
/* call this something else to dodge the QC4WIN bullet... */
int win_matherr( struct exception *except )
#else
int matherr( struct exception *except )
#endif
{
    extern int debugflag;
    static char far msg[]={"Math error, but we'll try to keep going"};
    if(first_err)
    {
       if(debugflag == 4000)stopmsg(0,msg);
       first_err = 0;
    }
    if(debugflag)
    {
       static int ct = 0;
       static FILE *fp=NULL;
       if(fp==NULL)
	  fp = fopen("matherr","w");
       if(ct++ < 100)
       {
	  fprintf(fp,"err:  %d\nname: %s\narg:  %le\n",
		  except->type, except->name, except->arg1);
	  fflush(fp);
       }
    }
    if( except->type == DOMAIN )
    {
	char buf[40];
	sprintf(buf,"%le",except->arg1);
	/* This test may be unnecessary - from my experiments if the
	   argument is too large or small the error is TLOSS not DOMAIN */
	if(strstr(buf,"IN")||strstr(buf,"NAN"))  /* trashed arg? */
			   /* "IND" with MSC, "INF" with BC++ */
	{
	   if( strcmp( except->name, "sin" ) == 0 )
	   {
	      except->retval = 0.0;
	      return(1);
	   }
	   else if( strcmp( except->name, "cos" ) == 0 )
	   {
	      except->retval = 1.0;
	      return(1);
	   }
	   else if( strcmp( except->name, "log" ) == 0 )
	   {
	      except->retval = 1.0;
	      return(1);
	   }
       }
    }
    if( except->type == TLOSS )
    {
       /* try valiantly to keep going */
	   if( strcmp( except->name, "sin" ) == 0 )
	   {
	      except->retval = 0.5;
	      return(1);
	   }
	   else if( strcmp( except->name, "cos" ) == 0 )
	   {
	      except->retval = 0.5;
	      return(1);
	   }
    }
    /* shucks, no idea what went wrong, but our motto is "keep going!" */
    except->retval = 1.0;
    return(1);
}
#endif

void roundfloatd(double *x) /* make double converted from float look ok */
{
   char buf[30];
   sprintf(buf,"%-10.7g",*x);
   *x = atof(buf);
}

/* fake a keystroke, returns old pending key */
int ungetakey(int key)
{
   int old;
   extern int keybuffer;
   old = keybuffer;
   keybuffer = key;
   return(old);
}

/* use this indirect aproach so that we can put GIFVIEW.C in an overlay */

int gifview()
{
    return(gifview1());
}
