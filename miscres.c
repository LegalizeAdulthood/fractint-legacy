/*
	Resident odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <malloc.h>
#ifndef XFRACT
#include <stdarg.h>
#include <io.h>
#include <dos.h>
#else
#include <varargs.h>
#endif
#include <math.h>
/*#ifdef __TURBOC__
#include <dir.h>
#endif  */

#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"
#include "prototyp.h"

/* routines in this module	*/

static	void trigdetails(char *);
static void area(void);
static int find_one_file_item(char *, char *, FILE **);

/* TW's static string consolidation campaign to help brain-dead compilers */
char s_cantwrite[]      = {"Can't write %s"};
char s_cantcreate[]     = {"Can't create %s"};
char s_cantunderstand[] = {"Can't understand %s"};
char s_cantfind[]       = {"Can't find %s"};

#ifndef XFRACT

void findpath(char far *filename, char *fullpathname) /* return full pathnames */
{
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];
   char temp_path[FILE_MAX_PATH];

   splitpath(filename ,NULL,NULL,fname,ext);
   makepath(temp_path,""   ,"" ,fname,ext);

   if(checkcurdir != 0 && access(temp_path,0)==0) {   /* file exists */
      strcpy(fullpathname,temp_path);
      return;
   }

   far_strcpy(temp_path,filename);   /* avoid side effect changes to filename */

   if (temp_path[0] == SLASHC || (temp_path[0] && temp_path[1] == ':')) {
      if(access(temp_path,0)==0) {   /* file exists */
         strcpy(fullpathname,temp_path);
         return;
         }
      else {
         splitpath(temp_path ,NULL,NULL,fname,ext);
         makepath(temp_path,""   ,"" ,fname,ext);
         }
      }
   fullpathname[0] = 0; 			/* indicate none found */
/* #ifdef __TURBOC__ */				/* look for the file */
/*   strcpy(fullpathname,searchpath(temp_path)); */
/* #else */
   _searchenv(temp_path,"PATH",fullpathname);
/* #endif */
   if (fullpathname[0] != 0)			/* found it! */
      if (strncmp(&fullpathname[2],SLASHSLASH,2) == 0) /* stupid klooge! */
	 strcpy(&fullpathname[3],temp_path);
}
#endif


void notdiskmsg()
{
static FCODE sorrymsg[]={
"This type may be slow using a real-disk based 'video' mode, but may not \n\
be too bad if you have enough expanded or extended memory. Press <Esc> to \n\
abort if it appears that your disk drive is working too hard."};
   stopmsg(0,sorrymsg);
}

/* Wrapping version of putstring for long numbers                         */
/* row     -- pointer to row variable, internally incremented if needed   */
/* col1    -- starting column                                             */
/* col2    -- last column                                                 */
/* color   -- attribute (same as for putstring)                           */
/* maxrow -- max number of rows to write                                 */
/* returns 0 if success, 1 if hit maxrow before done                      */
int putstringwrap(int *row,int col1,int col2,int color,char far *str,int maxrow)
{
    char save1, save2;
    int length, decpt, padding, startrow, done;
    done = 0;
    startrow = *row;
    length = far_strlen(str);
    padding = 3; /* space between col1 and decimal. */
    /* find decimal point */
    for(decpt=0;decpt < length; decpt++)
       if(str[decpt] == '.')
          break;
    if(decpt >= length)
       decpt = 0;
    if(decpt < padding)
       padding -= decpt;   
    else
       padding = 0;
    col1 += padding;   
    decpt += col1+1; /* column just past where decimal is */         
    while(length > 0)
    {
       if(col2-col1 < length)
       {
          if((*row - startrow + 1) >= maxrow)
             done = 1;
          else
             done = 0;
          save1 = str[col2-col1+1];
          save2 = str[col2-col1+2];
          if(done)
             str[col2-col1+1]   = '+';
          else
             str[col2-col1+1]   = '\\';
          str[col2-col1+2] = 0;
          putstring(*row,col1,color,str);
          if(done == 1)
             break;
          str[col2-col1+1] = save1;
          str[col2-col1+2] = save2;
          str += col2-col1;
          (*row)++;
       } else
          putstring(*row,col1,color,str);
       length -= col2-col1;
       col1 = decpt; /* align with decimal */
    }
    return(done);
}

#define rad_to_deg(x) ((x)*(180/PI)) /* most people "think" in degrees */
#define deg_to_rad(x) ((x)*(PI/180))
/*
convert corners to center/mag
Rotation angles indicate how much the IMAGE has been rotated, not the
zoom box.  Same goes for the Skew angles
*/

#ifdef _MSC_VER
#pragma optimize( "", off )
#endif

void cvtcentermag(double *Xctr, double *Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
   double Width, Height;
   double a, b; /* bottom, left, diagonal */
   double a2, b2, c2; /* squares of above */
   double tmpx, tmpy, tmpa; /* temporary x, y, angle */

   /* simple normal case first */
   if (xx3rd == xxmin && yy3rd == yymin)
   { /* no rotation or skewing, but stretching is allowed */
      Width  = xxmax - xxmin;
      Height = yymax - yymin;
      *Xctr = (xxmin + xxmax)/2;
      *Yctr = (yymin + yymax)/2;
      *Magnification  = 2/Height;
      *Xmagfactor =  Height / (DEFAULTASPECT * Width);
      *Rotation = 0.0;
      *Skew = 0.0;
      if (*Magnification < 0)
      {
         *Magnification = -*Magnification;
         *Rotation += 180;
      }
      return;
   }

   /* set up triangle ABC, having sides abc */
   /* side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd) */
   tmpx = xxmax - xx3rd;
   tmpy = yymin - yy3rd;
   a2 = tmpx*tmpx + tmpy*tmpy;
   a = sqrt(a2);
   *Rotation = -rad_to_deg(atan2( tmpy, tmpx )); /* negative for image rotation */

   tmpx = xxmin - xx3rd;
   tmpy = yymax - yy3rd;
   b2 = tmpx*tmpx + tmpy*tmpy;
   b = sqrt(b2);

   tmpx = xxmax - xxmin;
   tmpy = yymax - yymin;
   c2 = tmpx*tmpx + tmpy*tmpy;

   tmpa = acos((a2+b2-c2)/(2*a*b)); /* save tmpa for later use */
   *Skew = 90 - rad_to_deg(tmpa);

   *Xctr = (xxmin + xxmax)/2;
   *Yctr = (yymin + yymax)/2;

   Height = b * sin(tmpa);
   *Magnification  = 2/Height; /* 1/(h/2) */
   *Xmagfactor = Height / (DEFAULTASPECT * a);
   if (*Magnification < 0)
      {
      *Magnification = -*Magnification;
      *Rotation += 180;
      }
   return;
}

/* convert center/mag to corners */
void cvtcorners(double Xctr, double Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
   double x, y;
   double h, w; /* half height, width */
   double tanskew, sinrot, cosrot;

   if (Xmagfactor == 0.0)
      Xmagfactor = 1.0;

   h = (double)(1/Magnification);
   w = h / (DEFAULTASPECT * Xmagfactor);

   if (Rotation == 0.0 && Skew == 0.0)
      { /* simple, faster case */
      xx3rd = xxmin = Xctr - w;
      xxmax = Xctr + w;
      yy3rd = yymin = Yctr - h;
      yymax = Yctr + h;
      return;
      }

   /* in unrotated, untranslated coordinate system */
   tanskew = tan(deg_to_rad(Skew));
   xxmin = -w + h*tanskew;
   xxmax =  w - h*tanskew;
   xx3rd = -w - h*tanskew;
   yymax = h;
   yy3rd = yymin = -h;

   /* rotate coord system and then translate it */
   Rotation = deg_to_rad(Rotation);
   sinrot = sin(Rotation);
   cosrot = cos(Rotation);

   /* top left */
   x = xxmin * cosrot + yymax *  sinrot;
   y = -xxmin * sinrot + yymax *  cosrot;
   xxmin = x + Xctr;
   yymax = y + Yctr;

   /* bottom right */
   x = xxmax * cosrot + yymin *  sinrot;
   y = -xxmax * sinrot + yymin *  cosrot;
   xxmax = x + Xctr;
   yymin = y + Yctr;

   /* bottom left */
   x = xx3rd * cosrot + yy3rd *  sinrot;
   y = -xx3rd * sinrot + yy3rd *  cosrot;
   xx3rd = x + Xctr;
   yy3rd = y + Yctr;

   return;
}

/* convert corners to center/mag using bf */
void cvtcentermagbf(bf_t Xctr, bf_t Yctr, LDBL *Magnification, double *Xmagfactor, double *Rotation, double *Skew)
{
   /* needs to be LDBL or won't work past 307 (-DBL_MIN_10_EXP) or so digits */
   LDBL Width, Height;
   LDBL a, b; /* bottom, left, diagonal */
   LDBL a2, b2, c2; /* squares of above */
   LDBL tmpx, tmpy;
   double tmpa; /* temporary x, y, angle */
   bf_t bfWidth, bfHeight;
   bf_t bftmpx, bftmpy;
   int saved;
   int signx;

   saved = save_stack();

   /* simple normal case first */
   /* if (xx3rd == xxmin && yy3rd == yymin) */
   if(!cmp_bf(bfx3rd, bfxmin) && !cmp_bf(bfy3rd, bfymin))
      { /* no rotation or skewing, but stretching is allowed */
      bfWidth  = alloc_stack(bflength+2);
      bfHeight = alloc_stack(bflength+2);
      /* Width  = xxmax - xxmin; */
      sub_bf(bfWidth, bfxmax, bfxmin);
      Width  = bftofloat(bfWidth);
      /* Height = yymax - yymin; */
      sub_bf(bfHeight, bfymax, bfymin);
      Height = bftofloat(bfHeight);
      /* *Xctr = (xxmin + xxmax)/2; */
      add_bf(Xctr, bfxmin, bfxmax);
      half_a_bf(Xctr);
      /* *Yctr = (yymin + yymax)/2; */
      add_bf(Yctr, bfymin, bfymax);
      half_a_bf(Yctr);
      *Magnification  = 2/Height;
      *Xmagfactor =  (double)(Height / (DEFAULTASPECT * Width));
      *Rotation = 0.0;
      *Skew = 0.0;
      if (*Magnification < 0)
         {
         *Magnification = -*Magnification;
         *Rotation += 180;
         }
      restore_stack(saved);
      return;
      }

   bftmpx = alloc_stack(bflength+2);
   bftmpy = alloc_stack(bflength+2);

   /* set up triangle ABC, having sides abc */
   /* side a = bottom, b = left, c = diagonal not containing (x3rd,y3rd) */
   /* IMPORTANT: convert from bf AFTER subtracting */
   /* tmpx = xxmax - xx3rd; */
   sub_bf(bftmpx, bfxmax, bfx3rd);
   tmpx = bftofloat(bftmpx);
   /* tmpy = yymin - yy3rd; */
   sub_bf(bftmpy, bfymin, bfy3rd);
   tmpy = bftofloat(bftmpy);
   a2 = tmpx*tmpx + tmpy*tmpy;
   a = sqrtl(a2);

   /* divide tmpx and tmpy by |tmpx| so that double version of atan2() can be used */
   /* atan2() only depends on the ratio, this puts it in double's range */
   signx = sign(tmpx);
   if(signx)
      tmpy /= tmpx * signx;       /* tmpy = tmpy / |tmpx| */
   *Rotation = (double)(-rad_to_deg(atan2( (double)tmpy, signx ))); /* negative for image rotation */

   /* tmpx = xxmin - xx3rd; */
   sub_bf(bftmpx, bfxmin, bfx3rd);
   tmpx = bftofloat(bftmpx);
   /* tmpy = yymax - yy3rd; */
   sub_bf(bftmpy, bfymax, bfy3rd);
   tmpy = bftofloat(bftmpy);
   b2 = tmpx*tmpx + tmpy*tmpy;
   b = sqrtl(b2);

   /* tmpx = xxmax - xxmin; */
   sub_bf(bftmpx, bfxmax, bfxmin);
   tmpx = bftofloat(bftmpx);
   /* tmpy = yymax - yymin; */
   sub_bf(bftmpy, bfymax, bfymin);
   tmpy = bftofloat(bftmpy);
   c2 = tmpx*tmpx + tmpy*tmpy;

   tmpa = acos((double)((a2+b2-c2)/(2*a*b))); /* save tmpa for later use */
   *Skew = 90 - rad_to_deg(tmpa);

   /* these are the only two variables that must be to big precision */
   /* *Xctr = (xxmin + xxmax)/2; */
   add_bf(Xctr, bfxmin, bfxmax);
   half_a_bf(Xctr);
   /* *Yctr = (yymin + yymax)/2; */
   add_bf(Yctr, bfymin, bfymax);
   half_a_bf(Yctr);

   Height = b * sin(tmpa);
   *Magnification  = 2/Height; /* 1/(h/2) */
   *Xmagfactor = (double)(Height / (DEFAULTASPECT * a));
   if (*Magnification < 0)
      {
      *Magnification = -*Magnification;
      *Rotation += 180;
      }
   restore_stack(saved);
   return;
}


/* convert center/mag to corners using bf */
void cvtcornersbf(bf_t Xctr, bf_t Yctr, LDBL Magnification, double Xmagfactor, double Rotation, double Skew)
{
   LDBL x, y;
   LDBL h, w; /* half height, width */
   LDBL xmin, ymin, xmax, ymax, x3rd, y3rd;
   double tanskew, sinrot, cosrot;
   bf_t bfh, bfw;
   bf_t bftmp;
   int saved;

   saved = save_stack();
   bfh = alloc_stack(bflength+2);
   bfw = alloc_stack(bflength+2);

   if (Xmagfactor == 0.0)
      Xmagfactor = 1.0;

   h = 1/Magnification;
   floattobf(bfh, h);
   w = h / (DEFAULTASPECT * Xmagfactor);
   floattobf(bfw, w);

   if (Rotation == 0.0 && Skew == 0.0)
      { /* simple, faster case */
      /* xx3rd = xxmin = Xctr - w; */
      sub_bf(bfxmin, Xctr, bfw);
      copy_bf(bfx3rd, bfxmin);
      /* xxmax = Xctr + w; */
      add_bf(bfxmax, Xctr, bfw);
      /* yy3rd = yymin = Yctr - h; */
      sub_bf(bfymin, Yctr, bfh);
      copy_bf(bfy3rd, bfymin);
      /* yymax = Yctr + h; */
      add_bf(bfymax, Yctr, bfh);
      restore_stack(saved);
      return;
      }

   bftmp = alloc_stack(bflength+2);
   /* in unrotated, untranslated coordinate system */
   tanskew = tan(deg_to_rad(Skew));
   xmin = -w + h*tanskew;
   xmax =  w - h*tanskew;
   x3rd = -w - h*tanskew;
   ymax = h;
   y3rd = ymin = -h;

   /* rotate coord system and then translate it */
   Rotation = deg_to_rad(Rotation);
   sinrot = sin(Rotation);
   cosrot = cos(Rotation);

   /* top left */
   x =  xmin * cosrot + ymax *  sinrot;
   y = -xmin * sinrot + ymax *  cosrot;
   /* xxmin = x + Xctr; */
   floattobf(bftmp, x);
   add_bf(bfxmin, bftmp, Xctr);
   /* yymax = y + Yctr; */
   floattobf(bftmp, y);
   add_bf(bfymax, bftmp, Yctr);

   /* bottom right */
   x =  xmax * cosrot + ymin *  sinrot;
   y = -xmax * sinrot + ymin *  cosrot;
   /* xxmax = x + Xctr; */
   floattobf(bftmp, x);
   add_bf(bfxmax, bftmp, Xctr);
   /* yymin = y + Yctr; */
   floattobf(bftmp, y);
   add_bf(bfymin, bftmp, Yctr);

   /* bottom left */
   x =  x3rd * cosrot + y3rd *  sinrot;
   y = -x3rd * sinrot + y3rd *  cosrot;
   /* xx3rd = x + Xctr; */
   floattobf(bftmp, x);
   add_bf(bfx3rd, bftmp, Xctr);
   /* yy3rd = y + Yctr; */
   floattobf(bftmp, y);
   add_bf(bfy3rd, bftmp, Yctr);

   restore_stack(saved);
   return;
}

#ifdef _MSC_VER
#pragma optimize( "", on )
#endif

void updatesavename(char *filename) /* go to the next file name */
{
   char *save, *hold;
   char drive[FILE_MAX_DRIVE];
   char dir[FILE_MAX_DIR];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];

   splitpath(filename ,drive,dir,fname,ext);

   suffix[0] = 0;

   hold = fname + strlen(fname) - 1; /* start at the end */
   while(hold >= fname && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
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
   makepath(filename,drive,dir,fname,ext);
}

int check_writefile(char *name,char *ext)
{
 /* after v16 release, change encoder.c to also use this routine */
   char openfile[80];
   char opentype[20];
 /* int i; */
   char *period;
nextname:
   strcpy(openfile,name);
   strcpy(opentype,ext);
#if 0   
   for (i = 0; i < (int)strlen(openfile); i++)
      if (openfile[i] == '.') {
	 strcpy(opentype,&openfile[i]);
	 openfile[i] = 0;
	 }
#endif
   if((period = has_ext(openfile)) != NULL)
   {
      strcpy(opentype,period);
      *period = 0;
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
   {s_sin,   lStkSin,   dStkSin,   mStkSin   },
   {s_cosxx, lStkCosXX, dStkCosXX, mStkCosXX },
   {s_sinh,  lStkSinh,  dStkSinh,  mStkSinh  },
   {s_cosh,  lStkCosh,  dStkCosh,  mStkCosh  },
   {s_exp,   lStkExp,   dStkExp,   mStkExp   },
   {s_log,   lStkLog,   dStkLog,   mStkLog   },
   {s_sqr,   lStkSqr,   dStkSqr,   mStkSqr   },
   {s_recip, lStkRecip, dStkRecip, mStkRecip }, /* from recip on new in v16 */
   {s_ident, StkIdent,  StkIdent,  StkIdent  },
   {s_cos,   lStkCos,   dStkCos,   mStkCos   },
   {s_tan,   lStkTan,   dStkTan,   mStkTan   },
   {s_tanh,  lStkTanh,  dStkTanh,  mStkTanh  },
   {s_cotan, lStkCoTan, dStkCoTan, mStkCoTan },
   {s_cotanh,lStkCoTanh,dStkCoTanh,mStkCoTanh},
   {s_flip,  lStkFlip,  dStkFlip,  mStkFlip  },
   {s_conj,  lStkConj,  dStkConj,  mStkConj  },
   {s_zero,  lStkZero,  dStkZero,  mStkZero  },
   {s_asin,  lStkASin,  dStkASin,  mStkASin  },
   {s_asinh, lStkASinh, dStkASinh, mStkASinh },
   {s_acos,  lStkACos,  dStkACos,  mStkACos  },
   {s_acosh, lStkACosh, dStkACosh, mStkACosh },
   {s_atan,  lStkATan,  dStkATan,  mStkATan  },
   {s_atanh, lStkATanh, dStkATanh, mStkATanh },
   {s_cabs,  lStkCAbs,  dStkCAbs,  mStkCAbs  },
   {s_abs,   lStkAbs,   dStkAbs,   mStkAbs   },
   {s_sqrt,  lStkSqrt,  dStkSqrt,  mStkSqrt  },
#else
   {s_sin,   dStkSin,   dStkSin,   dStkSin   },
   {s_cosxx, dStkCosXX, dStkCosXX, dStkCosXX },
   {s_sinh,  dStkSinh,  dStkSinh,  dStkSinh  },
   {s_cosh,  dStkCosh,  dStkCosh,  dStkCosh  },
   {s_exp,   dStkExp,   dStkExp,   dStkExp   },
   {s_log,   dStkLog,   dStkLog,   dStkLog   },
   {s_sqr,   dStkSqr,   dStkSqr,   dStkSqr   },
   {s_recip, dStkRecip, dStkRecip, dStkRecip }, /* from recip on new in v16 */
   {s_ident, StkIdent,  StkIdent,  StkIdent  },
   {s_cos,   dStkCos,   dStkCos,   dStkCos   },
   {s_tan,   dStkTan,   dStkTan,   dStkTan   },
   {s_tanh,  dStkTanh,  dStkTanh,  dStkTanh  },
   {s_cotan, dStkCoTan, dStkCoTan, dStkCoTan },
   {s_cotanh,dStkCoTanh,dStkCoTanh,dStkCoTanh},
   {s_flip,  dStkFlip,  dStkFlip,  dStkFlip  },
   {s_conj,  dStkConj,  dStkConj,  dStkConj  },
   {s_zero,  dStkZero,  dStkZero,  dStkZero  },
   {s_asin,  dStkASin,  dStkASin,  dStkASin  },
   {s_asinh, dStkASinh, dStkASinh, dStkASinh },
   {s_acos,  dStkACos,  dStkACos,  dStkACos  },
   {s_acosh, dStkACosh, dStkACosh, dStkACosh },
   {s_atan,  dStkATan,  dStkATan,  dStkATan  },
   {s_atanh, dStkATanh, dStkATanh, dStkATanh },
   {s_cabs,  dStkCAbs,  dStkCAbs,  dStkCAbs  },
   {s_abs,   dStkAbs,   dStkAbs,   dStkAbs   },
   {s_sqrt,  dStkSqrt,  dStkSqrt,  dStkSqrt  },
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

   if ((slash = strchr(trigname,'/')) != NULL)
      *slash = 0;

   strlwr(trigname);

   for(i=0;i<numtrigfn;i++)
   {
      if(strcmp(trigname,trigfn[i].name)==0)
      {
	 trigndx[k] = (BYTE)i;
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

static FCODE sfractal_type[] =     {"Fractal type:"};
static FCODE sitem_name[] =        {"Item name:"};
static FCODE sitem_file[] =        {"Item file:"};
static FCODE s3D_transform[] =     {"3D Transform"};
static FCODE syou_are_cycling[] =  {"You are in color-cycling mode"};
static FCODE sfloating_point[] =   {"Floating-point"};
static FCODE ssolid_guessing[] =   {"Solid Guessing"};
static FCODE sboundary_tracing[] = {"Boundary Tracing"};
static FCODE stesseral[] =         {"Tesseral"};
static FCODE scalculation_time[] = {"Calculation time:"};
static FCODE siterations[] =       {" 1000's of points:"};
static FCODE scornersxy[] =        {"Corners:                X                     Y"};
static FCODE stop_left[] =         {"Top-l"};
static FCODE sbottom_right[] =     {"Bot-r"};
static FCODE sbottom_left[] =      {"Bot-l"};
static FCODE scenter[] =           {"Ctr"};
static FCODE struncate[] =         {"(Center values shown truncated to 320 decimals)"};
static FCODE smag[] =              {"Mag"};
static FCODE sxmag[] =             {"X-Mag-Factor"};
static FCODE srot[] =              {"Rotation"};
static FCODE sskew[] =             {"Skew"};
static FCODE sparams[] =           {"Params "};
static FCODE siteration_maximum[] ={"Iteration maximum: "};
static FCODE seffective_bailout[] ={"     Effective bailout: "};
static FCODE scurrent_rseed[] =    {"Current 'rseed': "};
static FCODE sinversion_radius[] = {"Inversion radius: "};
static FCODE sxcenter[] =          {"  xcenter: "};
static FCODE sycenter[] =          {"  ycenter: "};
static FCODE sparms_chgd[] = {"Parms chgd since generated"};
static FCODE sstill_being[] = {"Still being generated"};
static FCODE sinterrupted_resumable[] = {"Interrupted, resumable"};
static FCODE sinterrupted_non_resumable[] = {"Interrupted, non-resumable"};
static FCODE simage_completed[] = {"Image completed"};
static FCODE sflag_is_activated[] = {" flag is activated"};
static FCODE sinteger_math[]      = {"Integer math is in use"};
static FCODE sin_use_required[] = {" in use (required)"};
static FCODE sarbitrary_precision[] = {"Arbitrary precision "};
static FCODE spressanykey[] = {"Press any key to continue, F6 for area, CTRL-TAB for next page"};
static FCODE spressanykey1[] = {"Press any key to continue, Backspace for first screen"};
static FCODE sbatch[] = {" (Batch mode)"};
static FCODE ssavename[] = {"Savename: "};
static FCODE sstopsecret[] = {"Top Secret Developer's Screen"};
static FCODE sthreepass[] = {" (threepass)"};   

static void show_str_var(char *name, char *var, int *row, char *msg)
{
   if(var == NULL)
      return;
   if(*var != 0)
   {
      sprintf(msg,"%s=%s",name,var);
      putstring((*row)++,2,C_GENERAL_HI,msg);
   }
}

int tab_display_2(char *msg)
{
   extern long maxptr, maxstack, startstack;
   int row,key,ret=0;
   helptitle();
   setattr(1,0,C_GENERAL_MED,24*80); /* init rest to background */

   row = 1;
   putstringcenter(row++,0,80,C_PROMPT_HI, sstopsecret);
   sprintf(msg,"%u bytes conventional stack free",stackavail());
   putstring(++row,2,C_GENERAL_HI,msg);
   sprintf(msg,"%ld of %ld bignum memory used",maxptr,maxstack);
   putstring(++row,2,C_GENERAL_HI,msg);
   sprintf(msg,"   %ld used for bignum globals", startstack);
   putstring(++row,2,C_GENERAL_HI,msg);
   sprintf(msg,"   %ld stack used == %ld variables of length %d",
         maxptr-startstack,(long)((maxptr-startstack)/(rbflength+2)),rbflength+2);
   putstring(++row,2,C_GENERAL_HI,msg);
   if(bf_math)
   {
      sprintf(msg,"intlength %-d bflength %-d ",intlength, bflength);
      putstring(++row,2,C_GENERAL_HI,msg);
   }
   row++;   
   show_str_var(s_tempdir,    tempdir,      &row, msg);
   show_str_var(s_workdir,    workdir,      &row, msg);
   show_str_var(s_printfile,  PrintName,    &row, msg);
   show_str_var(s_filename,   readname,     &row, msg);
   show_str_var(s_formulafile,FormFileName, &row, msg);
   show_str_var(s_savename,   savename,     &row, msg);
   show_str_var(s_parmfile,   CommandFile,  &row, msg);
   show_str_var(s_ifsfile,    IFSFileName,  &row, msg);
   show_str_var(s_autokeyname,autoname,     &row, msg);
   show_str_var(s_lightname,  light_name,   &row, msg);
   show_str_var(s_map,        MAP_name,     &row, msg);
   sprintf(msg,"Sizeof fractalspecific array %d",
      num_fractal_types*(int)sizeof(struct fractalspecificstuff));
   putstring(row++,2,C_GENERAL_HI,msg);
   sprintf(msg,"checkcurdir %d",checkcurdir);
   putstring(row++,2,C_GENERAL_HI,msg);
   sprintf(msg,"calc_status %d",calc_status);
   putstring(row++,2,C_GENERAL_HI,msg);

   putstringcenter(24,0,80,C_GENERAL_LO,spressanykey1);
   key=getakeynohelp();
   if(key == BACKSPACE)
      ret = 1;
   return(ret);      
}

int tab_display()	/* display the status of the current image */
{
   int row, i, j, addrow=0;
   double Xctr, Yctr;
   LDBL Magnification;
   double Xmagfactor, Rotation, Skew;
   bf_t bfXctr=NULL, bfYctr=NULL;
   char msg[350];
   char far *msgptr;
   int key;
   int saved=0;
   int dec;

   if (calc_status < 0) { 	/* no active fractal image */
      return(0);		/* (no TAB on the credits screen) */
   }
   if (calc_status == 1)	/* next assumes CLK_TCK is 10^n, n>=2 */
      calctime += (clock_ticks() - timer_start) / (CLK_TCK/100);
   stackscreen();
   if(bf_math)
   {
      saved = save_stack();
      bfXctr = alloc_stack(bflength+2);
      bfYctr = alloc_stack(bflength+2);
   }
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
         putstring(row+1,3,C_GENERAL_MED,sitem_name);
	 putstring(row+1,16,C_GENERAL_HI,FormName);
         i = strlen(FormName)+1;
         putstring(row+2,3,C_GENERAL_MED,sitem_file);
         if(strlen(FormFileName) >= 29)
            addrow = 1;
         putstring(row+2+addrow,16,C_GENERAL_HI,FormFileName);
      }
      trigdetails(msg);
      putstring(row+1,16+i,C_GENERAL_HI,msg);
      if (fractype == LSYSTEM) 
      {
         putstring(row+1,3,C_GENERAL_MED,sitem_name);
	 putstring(row+1,16,C_GENERAL_HI,LName);
         putstring(row+2,3,C_GENERAL_MED,sitem_file);
         if(strlen(LFileName) >= 28)
            addrow = 1;
         putstring(row+2+addrow,16,C_GENERAL_HI,LFileName);
      }
      if (fractype == IFS || fractype == IFS3D) 
      {
         putstring(row+1,3,C_GENERAL_MED,sitem_name);
	 putstring(row+1,16,C_GENERAL_HI,IFSName);
         putstring(row+2,3,C_GENERAL_MED,sitem_file);
         if(strlen(IFSFileName) >= 28)
            addrow = 1;
         putstring(row+2+addrow,16,C_GENERAL_HI,IFSFileName);
      }
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
   if(initbatch && calc_status != 0)
      putstring(-1,-1,C_GENERAL_HI,sbatch);
   
   if (helpmode == HELPCYCLING)
      putstring(row+1,45,C_GENERAL_HI,syou_are_cycling);
   ++row;
   /* if(bf_math == 0) */ 
     ++row;

    i = j = 0;
    if (display3d > 0) {
       if (usr_floatflag)
	  j = 1;
       }
    else
       if (floatflag)
	  j = (usr_floatflag) ? 1 : 2;
    if(bf_math==0)
    {
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
   } else
   {
       sprintf(msg,"(%-d decimals)",decimals /*getprecbf(CURRENTREZ)*/);
       putstring(row,45,C_GENERAL_HI,sarbitrary_precision);
       putstring(-1,-1,C_GENERAL_HI,msg);

      i = 1;
   }   

   row += i;

   if (calc_status == 1 || calc_status == 2)
      if (curfractalspecific->flags&NORESUME)
      {
	 static FCODE msg[] = {"Note: can't resume this type after interrupts other than <tab> and <F1>"};
	 putstring(row++,2,C_GENERAL_HI,msg);
      }
   row += addrow;
   putstring(row,2,C_GENERAL_MED,ssavename);
   putstring(row,-1,C_GENERAL_HI,savename);
   
   /* if(bf_math == 0) */
     ++row;

   if (got_status >= 0 && (calc_status == 1 || calc_status == 2)) {
      switch (got_status) {
	 case 0:
	    sprintf(msg,"%d Pass Mode",totpasses);
	    putstring(row,2,C_GENERAL_HI,msg);
            if(usr_stdcalcmode=='3')
     	       putstring(row,-1,C_GENERAL_HI,sthreepass);
	    break;
	 case 1:
	    putstring(row,2,C_GENERAL_HI,ssolid_guessing);
            if(usr_stdcalcmode=='3')
     	       putstring(row,-1,C_GENERAL_HI,sthreepass);
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
   if (calctime >= 0)
      sprintf(msg,"%3ld:%02ld:%02ld.%02ld", calctime/360000L,
             (calctime%360000L)/6000, (calctime%6000)/100, calctime%100);
   else
      sprintf(msg," A Really Long Time!!! (> 24.855 days)");
   putstring(-1,-1,C_GENERAL_HI,msg);
   /* XXX */

   if ((curfractalspecific->flags&INFCALC) && (coloriter != 0)) {
      putstring(row,-1,C_GENERAL_MED,siterations);
      sprintf(msg," %ld of %ld",coloriter-2,maxct);
      putstring(row,-1,C_GENERAL_HI,msg);
   }
   
   ++row;
   if(bf_math == 0)
     ++row;
   if (videoentry.xdots && bf_math==0) {
      sprintf(msg,"Video: %dx%dx%d %s %s",
	      videoentry.xdots, videoentry.ydots, videoentry.colors,
	      videoentry.name, videoentry.comment);
      putstring(row++,2,C_GENERAL_MED,msg);
      }
   if(!(curfractalspecific->flags&NOZOOM))
   {
   adjust_corner(); /* make bottom left exact if very near exact */
   if(bf_math)
   {
      int truncate, truncaterow;
      dec = min(320,decimals);
      adjust_cornerbf(); /* make bottom left exact if very near exact */
      cvtcentermagbf(bfXctr, bfYctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
      /* find alignment information */
      msg[0] = 0;
      truncate = 0;
      if(dec < decimals)
         truncate = 1;
      truncaterow = row;
      putstring(++row,2,C_GENERAL_MED,scenter);
      putstring(row,8,C_GENERAL_MED,s_x);
      bftostr(msg,dec,bfXctr);
      if(putstringwrap(&row,10,78,C_GENERAL_HI,msg,5)==1)
         truncate = 1;
      putstring(++row,8,C_GENERAL_MED,s_y);
      bftostr(msg,dec,bfYctr);
      if(putstringwrap(&row,10,78,C_GENERAL_HI,msg,5)==1 || truncate)
         putstring(truncaterow,2,C_GENERAL_MED,struncate);
      putstring(++row,2,C_GENERAL_MED,smag);
#ifdef USE_LONG_DOUBLE
      sprintf(msg,"%10.8Le",Magnification);
#else
      sprintf(msg,"%10.8le",Magnification);
#endif
      putstring(-1,11,C_GENERAL_HI,msg);
      putstring(++row,2,C_GENERAL_MED,sxmag);
      sprintf(msg,"%11.4f   ",Xmagfactor);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,srot);
      sprintf(msg,"%9.3f   ",Rotation);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,sskew);
      sprintf(msg,"%9.3f",Skew);
      putstring(-1,-1,C_GENERAL_HI,msg);
   }
   else /* bf != 1 */
   {
      putstring(row,2,C_GENERAL_MED,scornersxy);
      putstring(++row,3,C_GENERAL_MED,stop_left);
      sprintf(msg,"%20.16f  %20.16f",xxmin,yymax);
      putstring(-1,17,C_GENERAL_HI,msg);
      putstring(++row,3,C_GENERAL_MED,sbottom_right);
      sprintf(msg,"%20.16f  %20.16f",xxmax,yymin);
      putstring(-1,17,C_GENERAL_HI,msg);

      if (xxmin != xx3rd || yymin != yy3rd)
      {
         putstring(++row,3,C_GENERAL_MED,sbottom_left);
         sprintf(msg,"%20.16f  %20.16f",xx3rd,yy3rd);
         putstring(-1,17,C_GENERAL_HI,msg);
      }
      cvtcentermag(&Xctr, &Yctr, &Magnification, &Xmagfactor, &Rotation, &Skew);
      putstring(row+=2,2,C_GENERAL_MED,scenter);
      sprintf(msg,"%20.16f %20.16f  ",Xctr,Yctr);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,smag);
#ifdef USE_LONG_DOUBLE
      sprintf(msg," %10.8Le",Magnification);
#else
      sprintf(msg," %10.8le",Magnification);
#endif
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(++row,2,C_GENERAL_MED,sxmag);
      sprintf(msg,"%11.4f   ",Xmagfactor);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,srot);
      sprintf(msg,"%9.3f   ",Rotation);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,sskew);
      sprintf(msg,"%9.3f",Skew);
      putstring(-1,-1,C_GENERAL_HI,msg);

   }
   }      
   for (i = 0; i < MAXPARAMS; i++)
   {
      char *p;
      int col;
      p = typehasparm(fractype,i);
      if(i%4 == 0)
      {
         row++;
         col = 9;
      }
      else
         col = -1;
      if(p)
      {
         if(i==0)
            putstring(++row,2,C_GENERAL_MED,sparams);
         sprintf(msg,"%3d: ",i+1);
         putstring(row,col,C_GENERAL_MED,msg);
         if(*p == '+')
            sprintf(msg,"%-12d",(int)param[i]);
         else if(*p == '#')
            sprintf(msg,"%-12lu",(U32)param[i]);
         else   
            sprintf(msg,"%-12.9f",param[i]);
         putstring(-1,-1,C_GENERAL_HI,msg);
      }
   }
   putstring(row+=2,2,C_GENERAL_MED,siteration_maximum);
   sprintf(msg,"%ld",maxit);
   putstring(-1,-1,C_GENERAL_HI,msg);
   putstring(-1,-1,C_GENERAL_MED,seffective_bailout);
   sprintf(msg,"%f",rqlim);
   putstring(-1,-1,C_GENERAL_HI,msg);

   if (fractype == PLASMA || fractype == ANT || fractype == CELLULAR) {
      putstring(++row,2,C_GENERAL_MED,scurrent_rseed);
      sprintf(msg,"%d",rseed);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   if(invert) {
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
/*waitforkey:*/
   putstringcenter(/*row*/24,0,80,C_GENERAL_LO,spressanykey);
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
   else if(key==CTL_TAB) {    
      if(tab_display_2(msg))
         goto top;
   }
   unstackscreen();
   timer_start = clock_ticks(); /* tab display was "time out" */
   if(bf_math)
      restore_stack(saved);
   return(0);
}

static void area(void)
{
    /* apologies to UNIX folks, we PC guys have to save near space */
    static FCODE warning[] = {"Warning: inside may not be unique\n"};
    static FCODE total_area[] = {".  Total area "}; 
    char far *msg;
    int x,y;
    char buf[160];
    long cnt=0;
    if (inside<0) {
      static FCODE msg[] = {"Need solid inside to compute area"};
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
              msg,cnt,(long)xdots*(long)ydots,(char far *)total_area,
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

/* --------------------------------------------------------------------- */
static char seps[] = {"' ','\t',\n',\r'"};
char *get_ifs_token(char *buf,FILE *ifsfile)
{
   char *bufptr;
   for(;;)
   {
      if(file_gets(buf,200,ifsfile) < 0)
         return(NULL);
      else
      {
         if((bufptr = strchr(buf,';')) != NULL) /* use ';' as comment to eol */
            *bufptr = 0;
         if((bufptr = strtok(buf, seps)) != NULL)
            return(bufptr);
      }
   }
}

FCODE insufficient_ifs_mem[]={"Insufficient memory for IFS"};
int numaffine;
int ifsload()			/* read in IFS parameters */
{
   int i;
   FILE *ifsfile;
   char buf[201];
   char *bufptr;
   int ret,rowsize;

   if (ifs_defn) { /* release prior parms */
      farmemfree((char far *)ifs_defn);
      ifs_defn = NULL;
      }

   ifs_type = 0;
   rowsize = IFSPARM;
   if (find_file_item(IFSFileName,IFSName,&ifsfile) < 0)
      return(-1);

   file_gets(buf,200,ifsfile);
   if((bufptr = strchr(buf,';')) != NULL) /* use ';' as comment to eol */
      *bufptr = 0;
   
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
      ((float *)tstack)[i] = 0;
   i = ret = 0;
   bufptr = get_ifs_token(buf,ifsfile);
   while(bufptr != NULL)
   {
      if(sscanf(bufptr," %f ",&((float *)tstack)[i]) != 1)
         break ;
      if (++i >= NUMIFS*rowsize) 
      {
         static FCODE msg[]={"IFS definition has too many lines"};
	    stopmsg(0,msg);
	    ret = -1;
	    break;
      }
      if((bufptr = strtok( NULL, seps ))==NULL)
      {
         if((bufptr = get_ifs_token(buf,ifsfile)) == NULL)
         {
            ret = -1;
            break;
         }   
      }
      if(ret == -1)
         break;
      if(*bufptr == '}')
         break;
   }
   
   if ((i % rowsize) != 0 || *bufptr != '}') {
      static FCODE msg[]={"invalid IFS definition"};
      stopmsg(0,msg);
      ret = -1;
      }
   if (i == 0 && ret == 0) {
      static FCODE msg[]={"Empty IFS definition"};
      stopmsg(0,msg);
      ret = -1;
      }
   fclose(ifsfile);

   if (ret == 0) {
      numaffine = i/rowsize;
      if ((ifs_defn = (float far *)farmemalloc(
			(long)((NUMIFS+1)*IFS3DPARM*sizeof(float)))) == NULL) {
     stopmsg(0,insufficient_ifs_mem);
	 ret = -1;
	 }
      else
	 for (i = 0; i < (NUMIFS+1)*IFS3DPARM; ++i)
	    ifs_defn[i] = ((float *)tstack)[i];
   }
   return(ret);
}
/* TW 5-31-94 - added search of current directory for entry files if
   entry item not found */

int find_file_item(char *filename,char *itemname,FILE **fileptr)
{
   FILE *infile=NULL;
   int found = 0;
   if((found=find_one_file_item(filename,itemname,&infile)) != 0) 
   {  /* search for file */
      int out;
      char drive[FILE_MAX_DRIVE];
      char dir[FILE_MAX_DIR];
      char fname[FILE_MAX_FNAME];
      char ext[FILE_MAX_EXT];
      char fullpath[FILE_MAX_PATH];
      splitpath(filename,drive,dir,fname,ext);
      makepath(fullpath,drive,dir,"*",ext);
      out = fr_findfirst(fullpath);
      found = 0;
      while(out == 0) 
      {
         char msg[200];
         DTA.filename[FILE_MAX_FNAME+FILE_MAX_EXT-2]=0;
         sprintf(msg,"Searching %13s for %s      ",DTA.filename,itemname);
         showtempmsg(msg);
         if(!(DTA.attribute & SUBDIR) && 
             strcmp(DTA.filename,".")&&
             strcmp(DTA.filename,"..")) {    
#ifndef XFRACT
            strlwr(DTA.filename);
#endif
            splitpath(DTA.filename,NULL,NULL,fname,ext);
            makepath(fullpath,drive,dir,fname,ext);
            if((found=find_one_file_item(fullpath,itemname,&infile)) == 0) 
            {
               strcpy(filename,fullpath);
               break;
            }   
         }   
         out = fr_findnext();
      }
      cleartempmsg();
      if(found != 0)
      {
         sprintf(fullpath,"'%s' file entry item not found",itemname);
         stopmsg(0,fullpath);
         return(-1);
      }
   }
   /* found file */   
   if(fileptr != NULL)
      *fileptr = infile;
   else if(infile != NULL)
      fclose(infile);   
   return(0);
}

static int find_one_file_item(char *filename,char *itemname,FILE **infile)
{
   char fullpathname[FILE_MAX_PATH];
   char fname[FILE_MAX_FNAME];
   char ext[FILE_MAX_EXT];

   splitpath(filename ,NULL,NULL,fname,ext);
   makepath(fullpathname,""   ,"" ,fname,ext);

   /* first try current directory */
   if(checkcurdir == 0 || access(fullpathname,0) != 0)
      strcpy(fullpathname,filename);   
   
   /* now binary node as of 2/95 TW */
   if ((*infile = fopen(fullpathname,"rb")) == NULL) {
       return(-1);
      }
   /* 
      Scan_entries() resuses code from gfe_choose_entry() in PROMPTS1.C. The
      original code used text mode, which made ftell() and fseek() unreliable
      in cases where files have garbage after the EOF.
    */
   if(scan_entries(*infile, NULL, itemname)<0)
      return(0);
   fclose(*infile);
   infile = NULL;
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
      if (c != '\r') buf[len++] = (char)c;    /* ignore c/r */
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
int _cdecl matherr( struct exception *except )
#endif
{
    static FCODE msg[]={"Math error, but we'll try to keep going"};
    if(first_err)
    {
       if(debugflag == 4000 || debugflag == 3200)stopmsg(0,msg);
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
	  fprintf(fp,"err:  %d\nname: %s\narg:  %e\n",
		  except->type, except->name, except->arg1);
	  fflush(fp);
       }
    }
    if( except->type == DOMAIN )
    {
	char buf[40];
	sprintf(buf,"%e",except->arg1);
	/* This test may be unnecessary - from my experiments if the
	   argument is too large or small the error is TLOSS not DOMAIN */
	if(strstr(buf,"IN")||strstr(buf,"NAN"))  /* trashed arg? */
			   /* "IND" with MSC, "INF" with BC++ */
	{
	   if( strcmp( except->name, s_sin ) == 0 )
	   {
	      except->retval = 0.0;
	      return(1);
	   }
	   else if( strcmp( except->name, s_cos ) == 0 )
	   {
	      except->retval = 1.0;
	      return(1);
	   }
	   else if( strcmp( except->name, s_log ) == 0 )
	   {
	      except->retval = 1.0;
	      return(1);
	   }
       }
    }
    if( except->type == TLOSS )
    {
       /* try valiantly to keep going */
	   if( strcmp( except->name, s_sin ) == 0 )
	   {
	      except->retval = 0.5;
	      return(1);
	   }
	   else if( strcmp( except->name, s_cos ) == 0 )
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
   old = keybuffer;
   keybuffer = key;
   return(old);
}

#if _MSC_VER == 800
#ifdef FIXTAN_DEFINED
#undef tan
/* !!!!! stupid MSVC tan(x) bug fix !!!!!!!!            */
/* tan(x) can return -tan(x) if -pi/2 < x < pi/2       */
/* if tan(x) has been called before outside this range. */
double fixtan( double x )
   {
   double y;

   y = tan(x);
   if ((x > -PI/2 && x < 0 && y > 0) || (x > 0 && x < PI/2 && y < 0))
      y = -y;
   return y;
   }
#define tan fixtan
#endif
#endif
