/*
FRACTALS.C and CALCFRAC.C actually calculate the fractal images (well,
SOMEBODY had to do it!).  The modules are set up so that all logic that
is independent of any fractal-specific code is in CALCFRAC.C, and the
code that IS fractal-specific is in FRACTALS.C. Original author Tim Wegner,
but just about ALL the authors have contributed SOME code to this routine
at one time or another, or contributed to one of the many massive
restructurings.

   -------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <dos.h>
#include <limits.h>
#include <time.h>
#include <stdarg.h>
#include "fractint.h"
#include "mpmath.h"
#include "targa_lc.h"

extern struct complex initorbit;
extern char useinitorbit;
struct lcomplex linitorbit;

extern unsigned int decoderline[];
extern unsigned int boxx[];
extern int overflow;
extern int bailout;
extern char far plasmamessage[];
long lmagnitud, llimit, llimit2, lclosenuff,l16triglim;
struct complex init,tmp,old,new,saved;
extern struct lcomplex lold;
extern double tempsqrx,tempsqry;
extern long ltempsqrx,ltempsqry;
extern int biomorph;
extern struct lcomplex linit;
extern int basin;
extern int cpu;
extern int resave_flag;
extern int dotmode;

int color, oldcolor, oldmax, oldmax10, row, col, passes, passnum;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;
long XXOne, FgOne, FgTwo, LowerLimit;

extern int LogFlag;
extern unsigned char far *LogTable;

void (*plot)() = putcolor;
extern int bound_trace_main();

extern double inversion[];          /* inversion radius, f_xcenter, f_ycenter */
extern int      xdots, ydots;       /* coordinates of dots on the screen  */
extern int      colors;             /* maximum colors available */
extern int      andcolor;           /* colors-1                 */
extern int      inside;             /* "inside" color to use    */
extern int      outside;            /* "outside" color to use   */
extern int      finattract;
double          min_orbit;          /* orbit value closest to origin */
int             min_index;          /* iteration of min_orbit */
extern int      maxit;              /* try this many iterations */
extern int      fractype;           /* fractal type */
extern int      numpasses;          /* 0 = 1 pass, 1 = double pass */
extern int      solidguessing;      /* 1 if solid-guessing */
extern int      debugflag;          /* for debugging purposes */
extern int      timerflag;          /* ditto */
extern  int     diskvideo;          /* for disk-video klooges */
extern int      calc_status;        /* status of calculations */
extern long     calctime;           /* total calc time for image */

extern int rflag, rseed;
extern int decomp[];
extern int distest;

extern double   param[];            /* parameters */
extern double   potparam[];         /* potential parameters */
extern long     far *lx0, far *ly0; /* X, Y points */
extern long     far *lx1, far *ly1; /* X, Y points */
extern long     fudge;              /* fudge factor (2**n) */
extern int      bitshift;           /* bit shift for fudge */
extern long     delmin;             /* for periodicity checking */
extern char potfile[];              /* potential filename */


#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

extern    int extraseg;

/* These are local but I don't want to pass them as parameters */
static    unsigned char top;   /* flag to indicate top of calcfract */
static    unsigned char c;
static    int i;

extern double xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* corners */
extern long   xmin, xmax, ymin, ymax;              /* integer equivs */
extern double plotmx1,plotmx2,plotmy1,plotmy2; /* real->screen conversion */
extern double delxx,delxx2,delyy,delyy2;
struct complex lambda;
double deltaX, deltaY;
double magnitude, rqlim, rqlim2;
extern struct complex parm,parm2;
int (*calctype)();
double closenuff;
int pixelpi; /* value of pi in pixels */
FILE *fp_pot;
int potflag; /* tells if continuous potential on  */
unsigned long lm;               /* magnitude limit (CALCMAND) */

/* ORBIT variables */
int     show_orbit;                     /* flag to turn on and off */
int     orbit_ptr;                      /* pointer into save_orbit array */
static int  far *save_orbit;            /* array to save orbit values */
static int orbit_color=15;              /* XOR color */

int     ixstart, ixstop, iystart, iystop;       /* (for CALCMAND) start, stop here */
int     symmetry;        /* symmetry flag */                                    
int     calcmode;        /* for StandardFractal/calcmand:      */               
                         /* 0 main rtn, 1/2 pass, ixstart etc  */               
                         /* 1 do initialization only           */               
                         /* 2 1pixel, reset periodicity checks */               
                         /* 3 1pixel, no reset                 */               
int     kbdcount;        /* avoids checking keyboard too often */

extern  int     integerfractal;         /* TRUE if fractal uses integer math */

char far *resume_info = NULL;           /* pointer to resume info if allocated */
int resume_len;                         /* length of resume info */
static int resume_offset;               /* offset in resume info gets */
int resuming;                           /* nonzero if resuming after interrupt */
int num_worklist;                       /* resume worklist for standard engine */
struct workliststuff worklist[MAXCALCWORK];
int xxstart,xxstop;                     /* these are same as worklist, */
int yystart,yystop,yybegin;             /* declared as separate items  */
int workpass,worksym;                   /* for the sake of calcmand    */

long timer_start,timer_interval;        /* timer(...) start & total */

struct complex far *dem_orbit;          /* temp used with distance estimator */
static int dem_iter;                    /* number of entries in dem_orbit */
double dem_delta, dem_width;
#define DEMBAILOUT 100000.0
#define DEMOVERFLOW 100000000000000.0

int  timer(int (*fractal)(), int argc,...);
static int  calcmandorstd(int (*func)());
static int  StandardCalc();
extern int  calcmand();
int  StandardPixel();
extern int  calcmandasm();
static void perform_worklist();
static void setsymmetry(int,int);
int  add_worklist(int,int,int,int,int,int,int);
static int  combine_worklist();
void tidy_worklist();

/* static vars for solidguess & its subroutines */
static int maxblock,halfblock;
static int guessplot;                   /* paint 1st pass row at a time?   */
static int right_guess,bottom_guess;
#define maxyblk 7    /* maxxblk*maxyblk*2 <= 4096, the size of "prefix" */
#define maxxblk 202  /* each maxnblk is oversize by 2 for a "border" */
/* next has a skip bit for each maxblock unit;
   1st pass sets bit  [1]... off only if block's contents guessed;
   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
   bits are numbered [..][y/16+1][x+1]&(1<<(y&15)) */
extern unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
/* size of next puts a limit of 2048 pixels across on solid guessing logic */
extern char dstack[4096];               /* common temp, two put_line calls */

#define N_ATTR 8                    /* max number of attractors     */  
int     attractors;                 /* number of finite attractors  */

struct complex  attr[N_ATTR];       /* finite attractor vals (f.p)  */

struct lcomplex lattr[N_ATTR];      /* finite attractor vals (int)  */


#ifndef lsqr                                                            
#define lsqr(x) (multiply((x),(x),bitshift))                            
#endif                                                                  

/* -------------------------------------------------------------------- */
/*              These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */

extern struct lcomplex lold,lnew,lparm,lparm2;   /* added "lold" */

int periodicitycheck = 1;

extern double floatmin, floatmax;

extern int StandardFractal();
int boundarytraceflag = 0;

extern int display3d;

calcfract()
{
   int oldnumpasses, oldsolidguessing, oldperiodicitycheck, olddistest;
   int i;
   oldnumpasses        = numpasses;          /* save these */
   oldsolidguessing    = solidguessing;
   oldperiodicitycheck = periodicitycheck;
   olddistest          = distest;

   attractors = 0;          /* default to no known finite attractors  */
   
   display3d = 0;

   orbit_color = 15;
   top     = 1;
   init_misc();  /* set up some variables in parser.c */
   if(fp_pot)
   {
      fclose(fp_pot);
      fp_pot = NULL;
   }

   if (invert)
   {
      floatmin = FLT_MIN;
      floatmax = FLT_MAX;
   }
   calcmode = 0; /* default mode for StandardFractal, not btm/ssg */

   basin = 0;

   /* following delta values useful only for types with rotation disabled */
   /* currently used only by bifurcation */
   if (integerfractal)
   {
      distest = 0;
      deltaX = (double)lx0[      1] / fudge - xxmin;
      deltaY = yymax - (double)ly0[      1] / fudge;
   }
   else
   {
      deltaX = dx0[      1] - xxmin;
      deltaY = yymax - dy0[      1];
   }

   parm.x   = param[0];
   parm.y   = param[1];
   parm2.x  = param[2];
   parm2.y  = param[3];

   if(fabs(potparam[0]) > 0.0)
      potflag = 1;
   else
      potflag = 0;

   if (LogFlag)
      /* ChkLogFlag(); */
      SetupLogTable();

   lm = 4;                              /* CALCMAND magnitude limit */
   lm = lm << bitshift;
   /*
         Continuous potential override (unused at the moment)
            if (potparam[2] > 4 && potparam[2] < 64)
               lm = potparam[2]*fudge;
   */
   /* set modulus bailout value if 3rd potparam set */

   if (fabs(potparam[2]) > 0.0)
      rqlim = potparam[2];
   else if (decomp[0] > 0 && decomp[1] > 0)
      rqlim = (double)decomp[1];
   else if(bailout)    /* user input bailout */
      rqlim = bailout;
   else if(biomorph != -1)   /* biomorph benefits from larger bailout */
      rqlim = 100;
   else
      rqlim = fractalspecific[fractype].orbit_bailout;

   /* ORBIT stuff */
   save_orbit = (int far *)((double huge *)dx0 + 4*MAXPIXELS);
   show_orbit = 0;
   orbit_ptr = 0;
   if(colors < 16)
      orbit_color = 1;

   if (integerfractal)          /* the bailout limit can't be too high here */
      if (rqlim > 127.0) rqlim = 127.0;
   if(inversion[0] != 0.0)
   {
      f_radius    = inversion[0];
      f_xcenter   = inversion[1];
      f_ycenter   = inversion[2];

      if (inversion[0] == AUTOINVERT)  /*  auto calc radius 1/6 screen */
         inversion[0] = f_radius = min(fabs(xxmax - xxmin),
             fabs(yymax - yymin)) / 6.0;

      if (invert < 2 || inversion[1] == AUTOINVERT)  /* xcenter not already set */
      {
         inversion[1] = f_xcenter = (xxmin + xxmax) / 2.0;
         if (fabs(f_xcenter) < fabs(xxmax-xxmin) / 100)
            inversion[1] = f_xcenter = 0.0;
      }

      if (invert < 3 || inversion[2] == AUTOINVERT)  /* ycenter not already set */
      {
         inversion[2] = f_ycenter = (yymin + yymax) / 2.0;
         if (fabs(f_ycenter) < fabs(yymax-yymin) / 100)
            inversion[2] = f_ycenter = 0.0;
      }

      invert = 3; /* so values will not be changed if we come back */
   }

   if (potfile[0] != 0 || boundarytraceflag)                    /* potential file? */
   {
      numpasses = 0;                            /* disable dual-pass */
      solidguessing = 0;                        /* disable solid-guessing */
   }

   closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
   closenuff /= fudge;
   rqlim2 = sqrt(rqlim);
   if (integerfractal)          /* for integer routines (lambda) */
   {
      lparm.x = parm.x * fudge;    /* real portion of Lambda */
      lparm.y = parm.y * fudge;    /* imaginary portion of Lambda */
      lparm2.x = parm2.x * fudge;  /* real portion of Lambda2 */
      lparm2.y = parm2.y * fudge;  /* imaginary portion of Lambda2 */
      llimit = rqlim * fudge;      /* stop if magnitude exceeds this */
      if (llimit <= 0) llimit = 0x7fffffff; /* klooge for integer math */
      llimit2 = rqlim2 * fudge;    /* stop if magnitude exceeds this */
      lclosenuff = closenuff * fudge;   /* "close enough" value */
      l16triglim = 8L<<16;         /* domain limit of fast trig functions */
      linitorbit.x = initorbit.x * fudge;
      linitorbit.y = initorbit.y * fudge;
   }
   resuming = (calc_status == 2);
   if (!resuming) /* free resume_info memory if any is hanging around */
   {
      end_resume();
      resave_flag = 0;
      calctime = 0;
   }

   if (fractalspecific[fractype].calctype != StandardFractal
       && fractalspecific[fractype].calctype != calcmand)
   {
      calctype = fractalspecific[fractype].calctype; /* per_image can override */
      symmetry = fractalspecific[fractype].symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */
      iystart = ixstart = yystart = xxstart = yybegin = 0;
      iystop = yystop = ydots -1;
      ixstop = xxstop = xdots -1;
      calc_status = 1; /* mark as in-progress */
      distest = 0; /* only standard escape time engine supports distest */
      /* per_image routine is run here */
      if (fractalspecific[fractype].per_image())
      { /* not a stand-alone */
         /* next two lines in case periodicity changed */
         closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
         closenuff /= fudge;
         lclosenuff = closenuff * fudge;        /* "close enough" value */
         setsymmetry(symmetry,0);
         timer(calctype,0); /* non-standard fractal engine */
      }
      if (check_key())
      {
         if (calc_status == 1) /* calctype didn't set this itself, */
            calc_status = 3;   /* so mark it interrupted, non-resumable */
      }
      else
         calc_status = 4; /* no key, so assume it completed */
   }
   else /* standard escape-time engine */
      timer((int (*)())perform_worklist,0);
   calctime += timer_interval;

   if (potfile[0] != 0)
   {
      if (calc_status == 2) /* no resume possible (at least, not yet) */
         calc_status = 3;
      /* potfile[0] = 0; */
   }
   if(LogTable)
   {
      farmemfree(LogTable);
      LogTable = (char far *)0;
   }
   solidguessing    = oldsolidguessing;
   numpasses        = oldnumpasses;
   periodicitycheck = oldperiodicitycheck;
   distest          = olddistest;
   return((calc_status == 4) ? 0 : -1);
}

/* Save/resume stuff:

   Engines which aren't resumable can simply ignore all this.

   Before calling the (per_image,calctype) routines (engine), calcfract sets:
      "resuming" to 0 if new image, nonzero if resuming a partially done image
      "calc_status" to 1
   If an engine is interrupted and wants to be able to resume it must:
      store whatever status info it needs to be able to resume later
      set calc_status to 2 and return
   If subsequently called with resuming!=0, the engine must restore status
   info and continue from where it left off.

   Since the info required for resume can get rather large for some types,
   it is not stored directly in save_info.  Instead, memory is dynamically
   allocated as required, and stored in .fra files as a separate block.
   To save info for later resume, an engine routine can use:
      alloc_resume(maxsize,version)
         Maxsize must be >= max bytes subsequently saved + 2; over-allocation
         is harmless except for possibility of insufficient mem available;
         undersize is not checked and probably causes serious misbehaviour.
         Version is an arbitrary number so that subsequent revisions of the
         engine can be made backward compatible.
         Alloc_resume sets calc_status to 2 (resumable) if it succeeds; to 3
         if it cannot allocate memory (and issues warning to user).
      put_resume({bytes,&argument,} ... 0)
         Can be called as often as required to store the info.
         Arguments must not be far addresses.
         Is not protected against calls which use more space than allocated.
   To reload info when resuming, use:
      start_resume()
         initializes and returns version number
      get_resume({bytes,&argument,} ... 0)
         inverse of store_resume
      end_resume()
         optional, frees the memory area sooner than would happen otherwise

   Example, save info:
      alloc_resume(sizeof(parmarray)+100,2);
      put_resume(sizeof(int),&row, sizeof(int),&col,
                 sizeof(parmarray),parmarray, 0);
    restore info:
      vsn = start_resume();
      get_resume(sizeof(int),&row, sizeof(int),&col, 0);
      if (vsn >= 2)
         get_resume(sizeof(parmarray),parmarray,0);
      end_resume();

   Engines which allocate a large far memory chunk of their own might
   directly set resume_info, resume_len, calc_status to avoid doubling
   transient memory needs by using these routines.

   StandardFractal, calcmand, solidguess, and bound_trace_main are a related
   set of engines for escape-time fractals.  They use a common worklist
   structure for save/resume.  Fractals using these must specify calcmand
   or StandardFractal as the engine in fractalspecificinfo.
   Other engines don't get btm nor ssg, don't get off-axis symmetry nor
   panning (the worklist stuff), and are on their own for save/resume.

   */

int put_resume(int len, ...)
{
   va_list arg_marker;  /* variable arg list */
   char *source_ptr;
   char far *dest_ptr;
   if (resume_info == NULL)
      return(-1);
   va_start(arg_marker,len);
   dest_ptr = resume_info + resume_len;
   while (len)
   {
      source_ptr = va_arg(arg_marker,char *);
      resume_len += len;
      while (--len >= 0) /* gross, but memcpy is a nogo near vs far */
         *(dest_ptr++) = *(source_ptr++);
      len = va_arg(arg_marker,int);
   }
   return(0);
}

int alloc_resume(int alloclen, int version)
{
   if (resume_info != NULL) /* free the prior area if there is one */
      farmemfree(resume_info);
   if ((resume_info = farmemalloc((long)alloclen))== NULL)
   {
      setfortext();
      printf("\n\n\n\n\nWarning - insufficient free memory to save status.");
      printf("\nYou will not be able to resume calculating this image.");
      printf("\n\n\nHit any key to continue.\n");
      while (keypressed()) getakey(); /* flush any keyahead */
      getakey();
      setforgraphics();
      calc_status = 3;
      return(-1);
   }
   resume_len = 0;
   put_resume(sizeof(int),&version,0);
   calc_status = 2;
   return(0);
}

int get_resume(int len, ...)
{
   va_list arg_marker;  /* variable arg list */
   char far *source_ptr;
   char *dest_ptr;
   if (resume_info == NULL)
      return(-1);
   va_start(arg_marker,len);
   source_ptr = resume_info + resume_offset;
   while (len)
   {
      dest_ptr = va_arg(arg_marker,char *);
      resume_offset += len;
      while (--len >= 0)
         *(dest_ptr++) = *(source_ptr++);
      len = va_arg(arg_marker,int);
   }
   return(0);
}

int start_resume()
{
   int version;
   if (resume_info == NULL)
      return(-1);
   resume_offset = 0;
   get_resume(sizeof(int),&version,0);
   return(version);
}

end_resume()
{
   if (resume_info != NULL) /* free the prior area if there is one */
   {
      farmemfree(resume_info);
      resume_info = NULL;
   }
}

static void perform_worklist()
{
   /* default setup a new worklist */
   num_worklist = 1;
   worklist[0].xxstart = 0;
   worklist[0].yystart = worklist[0].yybegin = 0;
   worklist[0].xxstop = xdots - 1;
   worklist[0].yystop = ydots - 1;
   worklist[0].pass = worklist[0].sym = 0;
   if (resuming) /* restore worklist, if we can't the above will stay in place */
   {
      start_resume();
      get_resume(sizeof(int),&num_worklist,sizeof(worklist),worklist,0);
      end_resume();
   }
   while (num_worklist > 0)
   {
      calctype = fractalspecific[fractype].calctype; /* per_image can override */
      symmetry = fractalspecific[fractype].symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */

      /* pull top entry off worklist */
      ixstart = xxstart = worklist[0].xxstart;
      ixstop  = xxstop  = worklist[0].xxstop;
      iystart = yystart = worklist[0].yystart;
      iystop  = yystop  = worklist[0].yystop;
      yybegin  = worklist[0].yybegin;
      workpass = worklist[0].pass;
      worksym  = worklist[0].sym;
      --num_worklist;
      for (i=0; i<num_worklist; ++i)
         worklist[i] = worklist[i+1];

      calc_status = 1; /* mark as in-progress */
      fractalspecific[fractype].per_image();
      closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
      closenuff /= fudge;
      lclosenuff = closenuff * fudge;   /* "close enough" value */

      /* we'd have done next sooner, but per_image can set distest & rqlim */
      if (distest) /* setup stuff for distance estimator */
      {
         rqlim = DEMBAILOUT;
         dem_delta = ( sqrt( sqr(delxx) + sqr(delxx2) )  /* half a pixel width */
             + sqrt( sqr(delyy) + sqr(delyy2) ) ) / 4;
         dem_width = ( sqrt( sqr(xxmax-xxmin) + sqr(xx3rd-xxmin) ) * ydots/xdots
             + sqrt( sqr(yymax-yymin) + sqr(yy3rd-yymin) ) ) / distest;
         dem_orbit = (struct complex far *)
            farmemalloc(((long)(maxit+1)) * sizeof(*dem_orbit));
         if (dem_orbit == NULL)
         {
            setfortext();
            printf("\n\n\n\nInsufficient memory.  Try reducing maximum iterations.\n");
            printf("\n\nAny key to continue.\n");
            buzzer(2);
            getakey();
            setforgraphics();
            break;
         }
      }
      setsymmetry(symmetry,1);
      /* fractal routine (usually StandardFractal) is run here */
      if (boundarytraceflag && (fractalspecific[fractype].flags&NOTRACE) == 0
          && potfile[0] == 0)
         bound_trace_main();
      else if(solidguessing && (fractalspecific[fractype].flags&NOGUESS) == 0)
         solidguess();
      else
         (*calctype)();
      if (distest) /* release distance estimator work area */
         farmemfree((unsigned char far *)dem_orbit);
      if (check_key()) /* interrupted? */
         break;
   }
   if (num_worklist > 0)
   {  /* interrupted, resumable */
      alloc_resume(sizeof(worklist)+10,1);
      put_resume(sizeof(int),&num_worklist,sizeof(worklist),worklist,0);
   }
   else
      calc_status = 4; /* completed */
}

test()
{
   int startrow,startpass;
   startrow = startpass = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&startrow,sizeof(int),&startpass,0);
      end_resume();
   }
   teststart();
   for (passes=startpass; passes <= numpasses ; passes++)
   {
      for (row = startrow; row <= iystop; row=row+1+numpasses)
      {
         register int col;
         for (col = 0; col <= ixstop; col++)       /* look at each point on screen */
         {
            register color;
            init.y = dy0[row]+dy1[col];
            init.x = dx0[col]+dx1[row];
            if(check_key())
            {
               testend();
               alloc_resume(20,1);
               put_resume(sizeof(int),&row,sizeof(int),&passes,0);
               return(-1);
            }
            color = testpt(init.x,init.y,parm.x,parm.y,maxit,inside);
            if (color >= colors) /* avoid trouble if color is 0 */
               if (colors < 16)
                  color &= andcolor;
               else
                  color = ((color-1) % andcolor) + 1; /* skip color zero */
            (*plot)(col,row,color);
            if(numpasses && (passes == 0))
               (*plot)(col,row+1,color);
         }
      }
      startrow = passes + 1;
   }
   testend();
   return(0);
}

/* BTM function to return the color value for a pixel.  */

static int far *LeftX  = (int far *)NULL;
static int far *RightX = (int far *)NULL;
static unsigned repeats;

int calc_xy(int mx, int my)
{

   color = getcolor(mx,my); /* see if pixel is black */
   if (color!=0)            /* pixel is NOT black so we must have already */
   {                      /* calculated its color, so lets skip it      */
      repeats++;            /* count successive easy ones */
      return(color);
   }
   repeats = 0;         /* we'll have to work for this one wo reset counter */

   col = mx;
   row = my;
   color=(*calctype)();
   return(color);
} /* calc_xy function of BTM code */


boundary_trace(int C, int R)   /* BTM main function */
{
   enum
       {
      North, East, South, West
   } 
   Dir;
   int modeON, C_first, bcolor, low_row, iters, gcolor;
   low_row = R;
   modeON = 0;
   Dir = East;
   bcolor = color;
   C_first = C;
   iters = 0;
   repeats = 0;

   /* main loop of BTM inside this loop the boundary is traced on screen! */
   do
   {
      if(--kbdcount<=0)
      {
         if(check_key())
            return(-1);
         kbdcount=(cpu==386) ? 80 : 30;
      }
      iters++;          /* count times thru loop */

      if (C < LeftX[R])
         LeftX[R]  = C; /* to aid in filling polygon later */
      if (C > RightX[R])
         RightX[R] = C; /* maintain left and right limits */
      else
         if (R==low_row)
            if (C<=C_first) /* works 99.9% of time! */
               break;
      switch (Dir)
      {
      case North :
         if (R > low_row)
            if(calc_xy(C,R-1)==bcolor)
            {
               R--;
               if (C > ixstart)
                  if(calc_xy(C-1,R)==bcolor)
                  {
                     C--;
                     Dir = West;
                  }
               break;
            }
         Dir = East;
         break;
      case East :
         if (C < ixstop)
            if(calc_xy(C+1,R)==bcolor)
            {
               C++;
               if (R > low_row)
                  if(calc_xy(C,R-1)==bcolor)
                  {
                     R--;
                     Dir = North;
                  }
               break;
            }
         Dir = South;
         break;
      case South :
         if (R < iystop)
            if(calc_xy(C,R+1)==bcolor)
            {
               R++;
               if (C < ixstop)
                  if(calc_xy(C+1,R)==bcolor)
                  {
                     C++;
                     Dir = East;
                  }
               break;
            }
         Dir = West;
         break;
      case West:
         if (C > ixstart)
            if(calc_xy(C-1,R)==bcolor)
            {
               C--;
               if (R < iystop)
                  if(calc_xy(C,R+1)== bcolor)
                  {
                     R++;
                     Dir = South;
                  }
               break;
            }
         Dir = North;
         break;
      } /* case */
   }
   while (repeats<30000); /* emergency backstop, should never be needed */
   /* PB, made above very high to allow for resumes;  did some checking
         of code first, and testing, to confirm that it seems unnecessary */
   if (iters<4)
   {
      LeftX[low_row] = 3000;
      RightX[low_row] = -3000;
      if (low_row+1 <= iystop)
      {
         LeftX[low_row+1] = 3000;
         RightX[low_row+1] = -3000;
      }
      return(0);  /* no need to fill a polygon of 3 points! */
   }

   /* Avoid tracing around whole fractal object */
   if (iystop+1==ydots)
      if (LeftX[0]==0)
         if (RightX[0]==ixstop)
            if (LeftX[iystop]==0)
               if (RightX[iystop]==ixstop)
               {
                  /* clean up in this RARE case or next fills will fail! */
                  for (low_row = 0; low_row <= ydots-1; low_row++)
                  {
                     LeftX[low_row] = 3000;
                     RightX[low_row] = -3000;
                  }
                  return(0);
               }
   /* fill in the traced polygon, simple but it works darn well */
   C = 0;
   for (R = low_row; R<iystop; R++)
      if (RightX[R] != -3000)
      {
         if((kbdcount-=2)<=0)
         {
            if(check_key())
               return(-1);
            kbdcount=(cpu==386) ? 80 : 30;
         }
         if(debugflag==1946)
            C = fillseg1(LeftX[R], RightX[R],R, bcolor);
         else
            C = fillseg(LeftX[R], RightX[R],R, bcolor);

         LeftX[R]  =  3000;
         RightX[R] = -3000; /* reset array element */
      }
      else if (C!=0) /* this is why C = 0 above! */
         return(0);
   return(0);
} /* BTM function */

fillseg1(int LeftX, int RightX, int R,  int bcolor)
{
   register modeON, C;
   int  gcolor;
   modeON = 0;
   for (C = LeftX; C <= RightX; C++)
   {
      gcolor=getcolor(C,R);
      if (modeON!=0 && gcolor==0)
/*         (*plot)(C,R,bcolor); */
         (*plot)(C,R,1); /* show boundary by only filling with color 1 */
      else
      {
         if (gcolor==bcolor) /* TW saved a getcolor here */
            modeON = 1;
         else
            modeON = 0;
      }
   }
   return(C);
}

fillseg(int LeftX, int RightX, int R,  int bcolor)
{
   unsigned char *forwards;
   unsigned char *backwards;
   register modeON, C;
   int  gcolor,i;
   modeON = 0;
   forwards  = (unsigned char *)decoderline;
   backwards = (unsigned char *)dstack;
   modeON = 0;
   get_line(R,LeftX,RightX,forwards);
   for (C = LeftX; C <= RightX; C++)
   {
      gcolor=forwards[C-LeftX];
      if (modeON!=0 && gcolor==0)
         forwards[C-LeftX]=bcolor;
      else
      {
         if (gcolor==bcolor) /* TW saved a getcolor here */
            modeON = 1;
         else
            modeON = 0;
      }
   }
   if(plot==putcolor) /* no symmetry! easy! */
      put_line(R,LeftX,RightX,forwards);
   else if(plot==symplot2) /* X-axis symmetry */
   {
      put_line(R,   LeftX,RightX,forwards);
      if ((i=yystop-(R-yystart)) > iystop)
         put_line(i,LeftX,RightX,forwards);
   }
   else if(plot==symplot2J) /* Origin symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,   LeftX,                  RightX,                forwards);
      if ((i=yystop-(R-yystart)) > iystop)
         put_line(i,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
   }
   else if(plot==symplot2Y) /* Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,LeftX,                  RightX,                forwards);
      put_line(R,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
   }
   else if(plot==symplot4) /* X-axis and Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,LeftX,                     RightX,                forwards);
      put_line(R,xxstop-(RightX-ixstart),   xxstop-(LeftX-ixstart),backwards);
      if ((i=yystop-(R-yystart)) > iystop)
      {
         put_line(i,LeftX,                  RightX,                forwards);
         put_line(i,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
      }
   }
   else  /* the other symmetry types are on their own! */
   {
      int i;
      for(i=LeftX;i<=RightX;i++)
         (*plot)(i,R,forwards[i-LeftX]);
   }
   return(C);
}

/* copy a string backwards for symmetry functions */
reverse_string(char *t, char *s, int len)
{
   register i;
   len--;
   for(i=0;i<=len;i++)
      t[i] = s[len-i];
   return(0);
}

bound_trace_main()
{
   int srow, scol;
   long maxrow;
   maxrow = ((long)ydots)*sizeof(int);

   if((LeftX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
      return(-1);
   else if((RightX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
   {
      farmemfree((char far *)LeftX);
      return(-1);
   }

   numpasses = 0; /* for calcmand */

   for (srow = 0; srow < ydots; srow++)
   {
      LeftX[srow] = 3000;
      RightX[srow] = -3000;
   }

   /* call calc routine once to get its vars initialized */
   calcmode = 1;
   (*calctype)();

   for (srow = iystart; srow <= iystop; srow++)
   {
      for (scol = ixstart; scol <= ixstop; scol++)
      {
         if(--kbdcount<=0)
         {
            if(check_key())
            {
               if (iystop != yystop)
                  iystop = yystop - (srow - yystart); /* allow for sym */
               add_worklist(xxstart,xxstop,srow,iystop,srow,0,worksym);
               farmemfree((char far *)LeftX);
               farmemfree((char far *)RightX);
               return(-1);
            }
            kbdcount=(cpu==386) ? 80 : 30;
         }

         /* BTM Hook! */
         color = getcolor(scol,srow);
         /* if pixel is BLACK (0) then we haven't done it yet!
            so first calculate its color and call the routine
            that will try and trace a polygon if one exists */
         if (color==0)
         {
            calcmode = 2; /* reset periodicity checking */
            row = srow;
            col = scol;
            color=(*calctype)();
            calcmode = 3; /* keep periodicity checking from pixel to pixel */
            boundary_trace(scol, srow); /* go trace boundary! WHOOOOOOO! */
         }
      }
   }
   farmemfree((char far *)LeftX);
   farmemfree((char far *)RightX);
   return(0);
} /* end of bound_trace_main */

StandardFractal() /* 1 or 2 pass main routine, or btm/ssg per pixel */
{
   if (calcmode > 1) /* calc 1 pixel, row/col set by caller */
   {
      if (calcmode == 2) /* reset periodicity before doing the pixel */
      {
         oldcolor = 1;
         oldmax = min(maxit, 250);
         oldmax10 = oldmax - 10;
      }
      return (StandardPixel());
   }
   kbdcount=(cpu==386) ? 80 : 30; /* one time initialization */
   if (calcmode == 1) /* init is all */
      return(0);
   return(calcmandorstd(StandardCalc));
}

int calcmand()
{
   if (calcmode) /* btm or ssg, 1 pixel or init call */
      return (calcmandasm());
   return (calcmandorstd(calcmandasm));
}

static int calcmandorstd(int (*calcrtn)())
{
   int i;
   if (numpasses && workpass == 0) /* do 1st pass of two */
   {
      passnum = 1;
      if ((*calcrtn)() == -1)
      {
         add_worklist(xxstart,xxstop,yystart,yystop,row,0,worksym);
         return(-1);
      }
      if (num_worklist > 0) /* worklist not empty, defer 2nd pass */
      {
         add_worklist(xxstart,xxstop,yystart,yystop,yystart,1,worksym);
         return(0);
      }
      workpass = 1;
      yybegin = yystart;
   }
   passnum = 2; /* second or only pass */
   if ((*calcrtn)() == -1)
   {
      i = yystop;
      if (iystop != yystop) /* must be due to symmetry */
         i -= row - iystart;
      add_worklist(xxstart,xxstop,row,i,row,workpass,worksym);
      return(-1);
   }
   return(0);
}

static int StandardCalc()
{
   row = yybegin;
   while (row <= iystop)
   {
      oldcolor = 1;
      oldmax = min(maxit, 250);
      oldmax10 = oldmax - 10;
      col = ixstart;
      while (col <= ixstop)
      {
         /* on 2nd pass of two, skip even pts */
         if(passnum==1 || numpasses==0 || (row&1) != 0 || (col&1) != 0)
         {
            if (StandardPixel() == -1) /* interrupted */
               return(-1);
            if (passnum == 1) /* first pass, copy pixel and bump col */
            {
               if ((row&1) == 0 && row < iystop)
               {
                  (*plot)(col,row+1,color);
                  if ((col&1) == 0 && col < ixstop)
                     (*plot)(col+1,row+1,color);
               }
               if ((col&1) == 0 && col < ixstop)
                  (*plot)(++col,row,color);
            }
         }
         ++col;
      }
      if (passnum == 1 && (row&1) == 0)
         ++row;
      ++row;
   }
   return(0);
}

int StandardPixel()                     /* per pixel 1/2/b/g, called with row & col set */
{
   int caught_a_cycle;
   int savedand, savedincr;     /* for periodicity checking */
   struct lcomplex lsaved;
   int i, attracted;

   /* really fractal specific, but we'll leave it here */
   if (!integerfractal)
   {
      saved.x = 0;
      saved.y = 0;
      init.y = dy0[row] + dy1[col];
   }
   else
   {
      lsaved.x = 0;
      lsaved.y = 0;
      linit.y = ly0[row] + ly1[col];
   }
   orbit_ptr = 0;
   color = 0;
   caught_a_cycle = 1;
   savedand = 1;                /* begin checking every other cycle */
   savedincr = 1;               /* start checking the very first time */

   if (inside == -61 || inside == -60)
   {
      magnitude = lmagnitud = 0;
      min_orbit = 100000.0;
   }
   overflow = 0;                /* reset integer math overflow flag */
   if (distest)
   {
      dem_iter = 0;
      if (fractalspecific[fractype].per_pixel()) /* mandels do the 1st iter */
      {
         dem_orbit[0].x = dem_orbit[0].y = 0;
         ++dem_iter;
      }
   }
   else
      fractalspecific[fractype].per_pixel(); /* initialize the calculations */
   while (++color < maxit)
   {
      attracted = FALSE;        /* check for convergence to any   */
      if (distest)
         dem_orbit[dem_iter++] = old;
      /* calculation of one orbit goes here */
      /* input in "old" -- output in "new" */

      if (fractalspecific[fractype].orbitcalc())
         break;

      if (inside == -60 || inside == -61)
      {
         if (integerfractal)
         {
            if (lmagnitud == 0)
               lmagnitud = lsqr(lnew.x) + lsqr(lnew.y);
            magnitude = lmagnitud;
            magnitude = magnitude / fudge;
         }
         else
            if (magnitude == 0.0)
               magnitude = sqr(new.x) + sqr(new.y);
         if (magnitude < min_orbit)
         {
            min_orbit = magnitude;
            min_index = color + 1;
         }
      }
      if (show_orbit)
      {
         if (!integerfractal)
            plot_orbit(new.x, new.y, -1);
         else
            iplot_orbit(lnew.x, lnew.y, -1);
      }

      if (attractors > 0)       /* finite attractor in the list   */
      {                         /* NOTE: Integer code is UNTESTED */
         if (integerfractal)
         {
            for (i = 0; i < attractors; i++)
            {
               if (labs(lnew.x - lattr[i].x) < lclosenuff)
                  if (labs(lnew.y - lattr[i].y) < lclosenuff)
                  {
                     attracted = TRUE;
                     break;
                  }
            }
         }
         else
         {
            for (i = 0; i < attractors; i++)
            {
               if (fabs(new.x - attr[i].x) < closenuff)
                  if (fabs(new.y - attr[i].y) < closenuff)
                  {
                     attracted = TRUE;
                     break;
                  }
            }
         }
      }
      if (attracted)
         break;                 /* AHA! Eaten by an attractor */

      if (oldcolor >= oldmax10)
         if (periodicitycheck)  /* only if this is OK to do */
         {
            if ((color & savedand) == 0)        /* time to save a new value */
            {
               if (!integerfractal)
                  saved = new;  /* floating pt fractals */
               else
                  lsaved = lnew;/* integer fractals */
               if (--savedincr == 0)    /* time to lengthen the periodicity? */
               {
                  savedand = (savedand << 1) + 1;       /* longer periodicity */
                  savedincr = 4;/* restart counter */
               }
            }
            else                /* check against an old save */
            {
               if (!integerfractal)     /* floating-pt periodicity chk */
               {
                  if (fabs(saved.x - new.x) < closenuff)
                     if (fabs(saved.y - new.y) < closenuff)
                     {
                        caught_a_cycle = 7;
                        color = maxit - 1;
                     }
               }
               else             /* integer periodicity check */
               {
                  if (labs(lsaved.x - lnew.x) < lclosenuff)
                     if (labs(lsaved.y - lnew.y) < lclosenuff)
                     {
                        caught_a_cycle = 7;
                        color = maxit - 1;
                     }
               }
            }
         }
   }
   if (color >= maxit)
   {
      if (oldcolor < maxit)
      {
         oldmax = oldcolor;
         oldmax10 = oldmax - 10;
      }
      if (periodicitycheck < 0 && caught_a_cycle == 7)
         color = caught_a_cycle;/* show periodicity */
   }
   if (show_orbit)
      scrub_orbit();
   oldcolor = color;
   if (color == 0)
      color = 1;                /* needed to make same as calcmand */

   if (distest)
   {
      double dist,temp;
      struct complex deriv;
      if (color < maxit && caught_a_cycle != 7) /* appears to be outside */
      {
         /* Distance estimator for points near Mandelbrot set */
         /* Original code by Phil Wilson, hacked around by PB */
         /* Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 */
         deriv.x = 1; /* preset and skip 1st orbit */
         deriv.y = 0;
         i = 0;
         while (++i < dem_iter)
         {
            temp = 2 * (dem_orbit[i].x * deriv.x - dem_orbit[i].y * deriv.y) + 1;
            deriv.y = 2 * (dem_orbit[i].y * deriv.x + dem_orbit[i].x * deriv.y);
            deriv.x = temp;
            if (fabs(deriv.x) > DEMOVERFLOW || fabs(deriv.y) > DEMOVERFLOW)
               break;
         }
         temp = sqr(new.x) + sqr(new.y);
         dist = log(temp) * sqrt(temp) / sqrt(sqr(deriv.x) + sqr(deriv.y));
         if (dist < dem_delta)
            color = inside;
         else if (colors == 2)
            color = !inside;
         else
            color = sqrt(dist / dem_width + 1);
      }
   }
   else if (potflag)
   {
      if (integerfractal)       /* adjust integer fractals */
      {
         new.x = lnew.x;
         new.x /= fudge;
         new.y = lnew.y;
         new.y /= fudge;
      }
      magnitude = sqr(new.x) + sqr(new.y);

      /* MUST call potential every pixel!! */
      color = potential(magnitude, color);
   }
   else if (decomp[0] > 0)
      decomposition();
   else if (biomorph != -1)
   {
      if (integerfractal)
      {
         if (labs(lnew.x) < llimit2 || labs(lnew.y) < llimit2)
            color = biomorph;
      }
      else
         if (fabs(new.x) < rqlim2 || fabs(new.y) < rqlim2)
            color = biomorph;
   }
   if ((kbdcount -= color) <= 0)
   {
      if (check_key())
         return (-1);
      kbdcount = (cpu == 386) ? 80 : 30;
   }
   if (oldcolor >= maxit)       /* really color, not oldcolor */
   {
      if (inside == -60)
         color = sqrt(min_orbit) * 75;
      else if (inside == -61)
         color = min_index;
      else if (inside >= 0)
         color = inside;
      else if (inside == -1)
         color = maxit;
   }
   else
      if (outside >= 0 && attracted == FALSE)           /* merge escape-time stripes to one color */
         color = outside;

   if (LogFlag)
      color = LogTable[color];
   if (color >= colors)         /* don't use color 0 unless is 0 from
                                       * inside/outside */
      if (colors < 16)
         color &= andcolor;
      else
         color = ((color - 1) % andcolor) + 1;  /* skip color zero */

   (*plot) (col, row, color);
   return (color);
}

decomposition()
{
   static double cos45     = 0.70710678118654750; /* cos 45     degrees */
   static double sin45     = 0.70710678118654750; /* sin 45     degrees */
   static double cos22_5   = 0.92387953251128670; /* cos 22.5   degrees */
   static double sin22_5   = 0.38268343236508980; /* sin 22.5   degrees */
   static double cos11_25  = 0.98078528040323040; /* cos 11.25  degrees */
   static double sin11_25  = 0.19509032201612820; /* sin 11.25  degrees */
   static double cos5_625  = 0.99518472667219690; /* cos 5.625  degrees */
   static double sin5_625  = 0.09801714032956060; /* sin 5.625  degrees */
   static double tan22_5   = 0.41421356237309500; /* tan 22.5   degrees */
   static double tan11_25  = 0.19891236737965800; /* tan 11.25  degrees */
   static double tan5_625  = 0.09849140335716425; /* tan 5.625  degrees */
   static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
   static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
   static long lcos45     ; /* cos 45     degrees */
   static long lsin45     ; /* sin 45     degrees */
   static long lcos22_5   ; /* cos 22.5   degrees */
   static long lsin22_5   ; /* sin 22.5   degrees */
   static long lcos11_25  ; /* cos 11.25  degrees */
   static long lsin11_25  ; /* sin 11.25  degrees */
   static long lcos5_625  ; /* cos 5.625  degrees */
   static long lsin5_625  ; /* sin 5.625  degrees */
   static long ltan22_5   ; /* tan 22.5   degrees */
   static long ltan11_25  ; /* tan 11.25  degrees */
   static long ltan5_625  ; /* tan 5.625  degrees */
   static long ltan2_8125 ; /* tan 2.8125 degrees */
   static long ltan1_4063 ; /* tan 1.4063 degrees */
   static char start=1;
   int temp = 0;
   struct lcomplex lalt;
   struct complex alt;
   if(start & integerfractal)
   {
      start = 0;
      lcos45     = cos45      *fudge;
      lsin45     = sin45      *fudge;
      lcos22_5   = cos22_5    *fudge;
      lsin22_5   = sin22_5    *fudge;
      lcos11_25  = cos11_25   *fudge;
      lsin11_25  = sin11_25   *fudge;
      lcos5_625  = cos5_625   *fudge;
      lsin5_625  = sin5_625   *fudge;
      ltan22_5   = tan22_5    *fudge;
      ltan11_25  = tan11_25   *fudge;
      ltan5_625  = tan5_625   *fudge;
      ltan2_8125 = tan2_8125  *fudge;
      ltan1_4063 = tan1_4063  *fudge;
   }
   color = 0;
   if (integerfractal) /* the only case */
   {
      if (lnew.y < 0)
      {
         temp = 2;
         lnew.y = -lnew.y;
      }

      if (lnew.x < 0)
      {
         ++temp;
         lnew.x = -lnew.x;
      }

      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (lnew.x < lnew.y)
         {
            ++temp;
            lalt.x = lnew.x; /* just */
            lnew.x = lnew.y; /* swap */
            lnew.y = lalt.x; /* them */
         }

         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (multiply(lnew.x,ltan22_5,bitshift) < lnew.y)
            {
               ++temp;
               lalt = lnew;
               lnew.x = multiply(lalt.x,lcos45,bitshift) +
                   multiply(lalt.y,lsin45,bitshift);
               lnew.y = multiply(lalt.x,lsin45,bitshift) -
                   multiply(lalt.y,lcos45,bitshift);
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (multiply(lnew.x,ltan11_25,bitshift) < lnew.y)
               {
                  ++temp;
                  lalt = lnew;
                  lnew.x = multiply(lalt.x,lcos22_5,bitshift) +
                      multiply(lalt.y,lsin22_5,bitshift);
                  lnew.y = multiply(lalt.x,lsin22_5,bitshift) -
                      multiply(lalt.y,lcos22_5,bitshift);
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (multiply(lnew.x,ltan5_625,bitshift) < lnew.y)
                  {
                     ++temp;
                     lalt = lnew;
                     lnew.x = multiply(lalt.x,lcos11_25,bitshift) +
                         multiply(lalt.y,lsin11_25,bitshift);
                     lnew.y = multiply(lalt.x,lsin11_25,bitshift) -
                         multiply(lalt.y,lcos11_25,bitshift);
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (multiply(lnew.x,ltan2_8125,bitshift) < lnew.y)
                     {
                        ++temp;
                        lalt = lnew;
                        lnew.x = multiply(lalt.x,lcos5_625,bitshift) +
                            multiply(lalt.y,lsin5_625,bitshift);
                        lnew.y = multiply(lalt.x,lsin5_625,bitshift) -
                            multiply(lalt.y,lcos5_625,bitshift);
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if (multiply(lnew.x,ltan1_4063,bitshift) < lnew.y)
                           if ((lnew.x*ltan1_4063 < lnew.y))
                              ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   else /* double case */
   {
      if (new.y < 0)
      {
         temp = 2;
         new.y = -new.y;
      }
      if (new.x < 0)
      {
         ++temp;
         new.x = -new.x;
      }
      if (decomp[0] >= 8)
      {
         temp <<= 1;
         if (new.x < new.y)
         {
            ++temp;
            alt.x = new.x; /* just */
            new.x = new.y; /* swap */
            new.y = alt.x; /* them */
         }
         if (decomp[0] >= 16)
         {
            temp <<= 1;
            if (new.x*tan22_5 < new.y)
            {
               ++temp;
               alt = new;
               new.x = alt.x*cos45 + alt.y*sin45;
               new.y = alt.x*sin45 - alt.y*cos45;
            }

            if (decomp[0] >= 32)
            {
               temp <<= 1;
               if (new.x*tan11_25 < new.y)
               {
                  ++temp;
                  alt = new;
                  new.x = alt.x*cos22_5 + alt.y*sin22_5;
                  new.y = alt.x*sin22_5 - alt.y*cos22_5;
               }

               if (decomp[0] >= 64)
               {
                  temp <<= 1;
                  if (new.x*tan5_625 < new.y)
                  {
                     ++temp;
                     alt = new;
                     new.x = alt.x*cos11_25 + alt.y*sin11_25;
                     new.y = alt.x*sin11_25 - alt.y*cos11_25;
                  }

                  if (decomp[0] >= 128)
                  {
                     temp <<= 1;
                     if (new.x*tan2_8125 < new.y)
                     {
                        ++temp;
                        alt = new;
                        new.x = alt.x*cos5_625 + alt.y*sin5_625;
                        new.y = alt.x*sin5_625 - alt.y*cos5_625;
                     }

                     if (decomp[0] == 256)
                     {
                        temp <<= 1;
                        if ((new.x*tan1_4063 < new.y))
                           ++temp;
                     }
                  }
               }
            }
         }
      }
   }
   for (i = 1; temp > 0; ++i)
   {
      if (temp & 1)
         color = (1 << i) - 1 - color;
      temp >>= 1;
   }
   if (decomp[0] == 2)
      color &= 1;
   if (colors > decomp[0])
      color++;
}

#if 0
MainNewton()
{
   Newton(); /* Lee's Newton */
   return(0);
}
#endif

/* Symmetry plot for period PI */
void symPIplot(x, y, color)
int x, y, color ;
{
   while(x <= xxstop)
   {
      putcolor(x, y, color) ;
      x += pixelpi;
   }
}
/* Symmetry plot for period PI plus Origin Symmetry */
void symPIplot2J(x, y, color)
int x, y, color ;
{
   int i,j;
   while(x <= xxstop)
   {
      putcolor(x, y, color) ;
      if ((i=yystop-(y-yystart)) > iystop && i < ydots
          && (j=xxstop-(x-xxstart)) < xdots)
         putcolor(j, i, color) ;
      x += pixelpi;
   }
}
/* Symmetry plot for period PI plus Both Axis Symmetry */
void symPIplot4J(x, y, color)
int x, y, color ;
{
   int i,j;
   while(x <= (xxstart+xxstop)/2)
   {
      j = xxstop-(x-xxstart);
      putcolor(       x , y , color) ;
      if (j < xdots)
         putcolor(j , y , color) ;
      if ((i=yystop-(y-yystart)) > iystop && i < ydots)
      {
         putcolor(x , i , color) ;
         if (j < xdots)
            putcolor(j , i , color) ;
      }
      x += pixelpi;
   }
}

/* Symmetry plot for X Axis Symmetry */
void symplot2(x, y, color)
int x, y, color ;
{
   int i;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
      putcolor(x, i, color) ;
}

/* Symmetry plot for Y Axis Symmetry */
void symplot2Y(x, y, color)
int x, y, color ;
{
   int i;
   putcolor(x, y, color) ;
   if ((i=xxstop-(x-xxstart)) < xdots)
      putcolor(i, y, color) ;
}

/* Symmetry plot for Origin Symmetry */
void symplot2J(x, y, color)
int x, y, color ;
{
   int i,j;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots
       && (j=xxstop-(x-xxstart)) < xdots)
      putcolor(j, i, color) ;
}

/* Symmetry plot for Both Axis Symmetry */
void symplot4(x, y, color)
int x, y, color ;
{
   int i,j;
   j = xxstop-(x-xxstart);
   putcolor(       x , y, color) ;
   if (j < xdots)
      putcolor(    j , y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      putcolor(    x , i, color) ;
      if (j < xdots)
         putcolor(j , i, color) ;
   }
}

/* Symmetry plot for X Axis Symmetry - Striped Newtbasin version */
void symplot2basin(x, y, color)
int x, y, color ;
{
   int i,stripe;
   extern int degree;
   putcolor(x, y, color) ;
   if(basin==2 && color > 8)
      stripe=8;
   else
      stripe = 0;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      color -= stripe;                    /* reconstruct unstriped color */
      color = (degree+1-color)%degree+1;  /* symmetrical color */
      color += stripe;                    /* add stripe */
      putcolor(x, i,color)  ;
   }
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
void symplot4basin(x, y, color)
int x, y, color ;
{
   extern int degree;
   int i,j,color1,stripe;
   if(color == 0) /* assumed to be "inside" color */
   {
      symplot4(x, y, color);
      return;
   }
   if(basin==2 && color > 8)
      stripe = 8;
   else
      stripe = 0;
   color -= stripe;               /* reconstruct unstriped color */
   color1 = degree/2+degree+2 - color;
   if(color < degree/2+2)
      color1 = degree/2+2 - color;
   else
      color1 = degree/2+degree+2 - color;
   j = xxstop-(x-xxstart);
   putcolor(       x, y, color+stripe) ;
   if (j < xdots)
      putcolor(    j, y, color1+stripe) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
   {
      putcolor(    x, i, stripe + (degree+1 - color)%degree+1) ;
      if (j < xdots)
         putcolor(j, i, stripe + (degree+1 - color1)%degree+1) ;
   }
}



/* Do nothing plot!!! */
void noplot(int x,int y,int color)
{
}

static int iparmx;                                  /* iparmx = parm.x * 16 */
static int shiftvalue;                          /* shift based on #colors */

typedef struct palett
{
   unsigned char red;
   unsigned char green;
   unsigned char blue;
} 
Palettetype;

extern int colors;
extern int xdots,ydots;
extern Palettetype dacbox[256];
extern int cpu, daccount, cyclelimit;
extern int resave_flag;
static int plasma_check;                        /* to limit kbd checking */

void adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   long pseudorandom;
   if(getcolor(x,y) != 0)
      return;

   pseudorandom = ((long)iparmx)*((rand()-16383));
   pseudorandom = pseudorandom*(abs(xa-xb)+abs(ya-yb));
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = ((getcolor(xa,ya)+getcolor(xb,yb)+1)>>1)+pseudorandom;
   if (pseudorandom <   1) pseudorandom =   1;
   if (pseudorandom >= colors) pseudorandom = colors-1;
   putcolor(x,y,(int)pseudorandom);
}

void subDivide(int x1,int y1,int x2,int y2)
{
   int x,y;
   int v;
   if ((++plasma_check & 0x7f) == 1)
      if(check_key())
      {
         plasma_check--;
         return;
      }
   if(x2-x1<2 && y2-y1<2)
      return;
   x = (x1+x2)>>1;
   y = (y1+y2)>>1;
   adjust(x1,y1,x ,y1,x2,y1);
   adjust(x2,y1,x2,y ,x2,y2);
   adjust(x1,y2,x ,y2,x2,y2);
   adjust(x1,y1,x1,y ,x1,y2);
   if(getcolor(x,y) == 0)
   {
      v = (getcolor(x1,y1)+getcolor(x2,y1)+getcolor(x2,y2)+
          getcolor(x1,y2)+2)>>2;
      putcolor(x,y,v);
   }
   subDivide(x1,y1,x ,y);
   subDivide(x ,y1,x2,y);
   subDivide(x ,y ,x2,y2);
   subDivide(x1,y ,x ,y2);
}

plasma()
{
   plasma_check = 0;

   if(colors < 4) {
      setvideomode(3,0,0,0);
      buzzer(2);
      helpmessage(plasmamessage);
      return(-1);
   }
   iparmx = param[0] * 8;
   if (parm.x <= 0.0) iparmx = 16;
   if (parm.x >= 100) iparmx = 800;

   if (!rflag) rseed = (int)time(NULL);
   srand(rseed);

   if (colors == 256)                   /* set the (256-color) palette */
      set_Plasma_palette();             /* skip this if < 256 colors */

   if (colors > 16)                     /* adjust based on # of colors */
      shiftvalue = 18;
   else {
      if (colors > 4)
         shiftvalue = 22;
      else {
         if (colors > 2)
            shiftvalue = 24;
         else
            shiftvalue = 25;
      }
   }

   putcolor(      0,      0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,      0,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));
   putcolor(      0,ydots-1,1+(((rand()/colors)*(colors-1))>>(shiftvalue-11)));

   subDivide(0,0,xdots-1,ydots-1);
   if (! check_key())
      return(0);
   else
      return(1);
}

set_Plasma_palette()
{
   static Palettetype Red    = {
      63, 0, 0   };
   static Palettetype Green  = { 
      0,63, 0   };
   static Palettetype Blue   = { 
      0, 0,63   };
   int i;

   if(CustomLut()) return(0);             /* TARGA 3 June 89 j mclain */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
      dacbox[i].red   = (i*Green.red   + (86-i)*Blue.red)/85;
      dacbox[i].green = (i*Green.green + (86-i)*Blue.green)/85;
      dacbox[i].blue  = (i*Green.blue  + (86-i)*Blue.blue)/85;

      dacbox[i+85].red   = (i*Red.red   + (86-i)*Green.red)/85;
      dacbox[i+85].green = (i*Red.green + (86-i)*Green.green)/85;
      dacbox[i+85].blue  = (i*Red.blue  + (86-i)*Green.blue)/85;

      dacbox[i+170].red   = (i*Blue.red   + (86-i)*Red.red)/85;
      dacbox[i+170].green = (i*Blue.green + (86-i)*Red.green)/85;
      dacbox[i+170].blue  = (i*Blue.blue  + (86-i)*Red.blue)/85;
   }
   ValidateLuts(NULL);     /* TARGA 3 June 89  j mclain */
   if (dotmode != 11)
      spindac(0,1);
}

check_key()
{
   int key;
   if((key = keypressed()) != 0) {
      if(key != 'o' && key != 'O')
         return(-1);
      getakey();
      if (dotmode != 11)
         show_orbit = 1 - show_orbit;
   }
   return(0);
}

#define RANDOM(x)  (rand()%(x))

/* This constant assumes that rand() returns a value from 0 to 32676 */
#define FOURPI  (long)(4*PI*(double)(1L << 16))

diffusion()
{
   int xmax,ymax,xmin,ymin;     /* Current maximum coordinates */
   int border;   /* Distance between release point and fractal */
   int i;
   double cosine,sine,angle;
   long lcosine,lsine;
   register int x,y;
   extern char floatflag;


   if (diskvideo)
   {
      setvideomode(3,0,0,0);
      buzzer(2);
      helpmessage(plasmamessage);
      return(-1);
   }

   bitshift = 16;
   fudge = 1L << 16;

   border = param[0];

   if (border <= 0)
      border = 10;

   if (!rflag)
      rseed = (int)time(NULL);
   srand(rseed);

   xmax = xdots / 2 + border;  /* Initial box */
   xmin = xdots / 2 - border;
   ymax = ydots / 2 + border;
   ymin = ydots / 2 - border;

   putcolor(xdots / 2, ydots / 2,RANDOM(colors-1)+1);  /* Seed point */

   while (1)
   {
      /* Release new point on circle just inside the box */

      if (floatflag)
      {
         angle=2*(double)rand()/(RAND_MAX/PI);
         FPUsincos(&angle,&sine,&cosine);
         x = cosine*(xmax-xmin) + xdots;
         y = sine  *(ymax-ymin) + ydots;
      }
      else
      {
         SinCos086(multiply((long)rand(),FOURPI,16),&lsine,&lcosine);
         x = (lcosine*(long)(xmax-xmin) >> 16) + xdots;
         y = (lsine  *(long)(ymax-ymin) >> 16) + ydots;
      }

      x = x >> 1; /* divide by 2 */
      y = y >> 1;

      /* Loop as long as the point (x,y) is surrounded by color 0 */
      /* on all eight sides                                       */
      while((getcolor(x+1,y+1) == 0) && (getcolor(x+1,y) == 0) &&
          (getcolor(x+1,y-1) == 0) && (getcolor(x  ,y+1) == 0) &&
          (getcolor(x  ,y-1) == 0) && (getcolor(x-1,y+1) == 0) &&
          (getcolor(x-1,y) == 0) && (getcolor(x-1,y-1) == 0))
      {
         /* Erase moving point */
         if (show_orbit)
            putcolor(x,y,0);

         /* Make sure point is inside the box */
         if (x==xmax)
            x--;
         else if (x==xmin)
            x++;
         if (y==ymax)
            y--;
         else if (y==ymin)
            y++;

         /* Take one random step */
         x += RANDOM(3) - 1;
         y += RANDOM(3) - 1;

         /* Check keyboard */
         if ((++plasma_check & 0x7f) == 1)
            if(check_key())
            {
               plasma_check--;
               return 1;
            }

         /* Show the moving point */
         if (show_orbit)
            putcolor(x,y,RANDOM(colors-1)+1);

      }
      putcolor(x,y,RANDOM(colors-1)+1);

      /* Is point to close to the edge? */
      if (((x+border)>xmax) || ((x-border)<xmin)
          || ((y-border)<ymin) || ((y+border)>ymax))
      {
         /* Increase box size, but not past the edge of the screen */
         if (ymin != 1)
         {
            ymin--;
            ymax++;
         }
         if (xmin != 1)
         {
            xmin--;
            xmax++;
         }
         if ((ymin==1) || (xmin==1))
            return 0;
      }
   }
}

/* Showing orbit requires converting real co-ords to screen co-ords.
   Define:
       Xs == xxmax-xx3rd               Ys == yy3rd-yymax
       W  == xdots-1                   D  == ydots-1
   We know that:
       realx == lx0[col] + lx1[row]
       realy == ly0[row] + ly1[col]
       lx0[col] == (col/width) * Xs + xxmin
       lx1[row] == row * delxx
       ly0[row] == (row/D) * Ys + yymax
       ly1[col] == col * (0-delyy)
  so:
       realx == (col/W) * Xs + xxmin + row * delxx
       realy == (row/D) * Ys + yymax + col * (0-delyy)
  and therefore:
       row == (realx-xxmin - (col/W)*Xs) / Xv    (1)
       col == (realy-yymax - (row/D)*Ys) / Yv    (2)
  substitute (2) into (1) and solve for row:
       row == ((realx-xxmin)*(0-delyy2)*W*D - (realy-yymax)*Xs*D)
                      / ((0-delyy2)*W*delxx2*D-Ys*Xs)
  */

static void plotdorbit(double dx, double dy, int color)
{
   int i, j, c;
   if (orbit_ptr >= 1500) return;
   i = dy * plotmx1 - dx * plotmx2;
   if (i < 0 || i >= xdots) return;
   j = dx * plotmy1 - dy * plotmy2;
   if (j < 0 || j >= ydots) return;
   /* save orbit value */
   if(color == -1)
   {
      *(save_orbit + orbit_ptr++) = i;
      *(save_orbit + orbit_ptr++) = j;
      *(save_orbit + orbit_ptr++) = c = getcolor(i,j);
      putcolor(i,j,c^orbit_color);
   }
   else
      putcolor(i,j,color);
}

iplot_orbit(ix, iy, color)
long ix, iy;
int color;
{
   plotdorbit((double)ix/fudge-xxmin,(double)iy/fudge-yymax,color);
}

plot_orbit(real,imag,color)
double real,imag;
int color;
{
   plotdorbit(real-xxmin,imag-yymax,color);
}

scrub_orbit()
{
   int i,j,c;
   while(orbit_ptr > 0)
   {
      c = *(save_orbit + --orbit_ptr);
      j = *(save_orbit + --orbit_ptr);
      i = *(save_orbit + --orbit_ptr);
      putcolor(i,j,c);
   }
}

/* timer function. Assumes #include <time.h> */
int timer(int (*fractal)(),int argc,...)
{
   va_list arg_marker;  /* variable arg list */
   int argcount;    /* how many optional args */
   char *timestring;
   time_t ltime;
   FILE *fp;
   int out;
   int args[4];
   int i;

   /* argc = number of optional arguments */
   va_start(arg_marker,argc);

   i=0;
   while(i < argc)
      args[i++] = va_arg(arg_marker,int);

   if(timerflag)
      fp=fopen("bench","a");
   timer_start = clock();
   switch(argc)
   {
   case 0:
      out = (*fractal)();
      break;
   case 1:
      out = (*fractal)(args[0]);
      break;
   case 2:
      out = (*fractal)(args[0],args[1]);
      break;
   case 3:
      out = (*fractal)(args[0],args[1],args[2]);
      break;
   case 4:
      out = (*fractal)(args[0],args[1],args[2],args[3]);
      break;
   default:
      return(-1);
      break;
   }
   /* next assumes CLK_TCK is 10^n, n>=2 */
   timer_interval = (clock() - timer_start) / (CLK_TCK/100);
   if(timerflag)
   {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */

      /* at the moment only decode timer takes a parameter */
      if(argc)
         fprintf(fp,"decode ");
      fprintf(fp,"%s type=%s resolution = %dx%d maxiter=%d",
          timestring,
          fractalspecific[fractype].name,
          xdots,
          ydots,
          maxit);
      fprintf(fp," time= %ld.%02ld secs\n",timer_interval/100,timer_interval%100);
      if(fp != NULL)
         fclose(fp);
   }
   return(out);
}

/*
   I, Timothy Wegner, invented this solidguessing idea and implemented it in
   more or less the overall framework you see here.  I am adding this note
   now in a possibly vain attempt to secure my place in history, because
   Pieter Branderhorst has totally rewritten this routine, incorporating
   a *MUCH* more sophisticated algorithm.  His revised code is not only
   faster, but is also more accurate. Harrumph!
*/

solidguess()
{
   int i,x,y,xlim,ylim,blocksize;
   unsigned int *pfxp0,*pfxp1;
   unsigned int u;

   guessplot=(plot!=putcolor && plot!=symplot2 && plot!=symplot2J);
   /* check if guessing at bottom & right edges is ok */
   bottom_guess = (plot==symplot2 || (plot==putcolor && iystop+1==ydots));
   right_guess  = (plot==symplot2J
       || ((plot==putcolor || plot==symplot2) && ixstop+1==xdots));

   maxblock=blocksize=ssg_blocksize();

   calcmode = 1;
   (*calctype)(); /* initialize calc routine */
   numpasses = 0; /* for calcmand */

   /* ensure window top and left are on required boundary, treat window
         as larger than it really is if necessary (this is the reason symplot
         routines must check for > xdots/ydots before plotting sym points) */
   ixstart &= -1 - (maxblock-1);
   iystart = yybegin;
   iystart &= -1 - (maxblock-1);

   if (workpass == 0) /* otherwise first pass already done */
   {
      /* first pass, calc every blocksize**2 pixel, quarter result & paint it */
      if (iystart <= yystart) /* first time for this window, init it */
      {
         memset(&prefix[1][0][0],0,maxxblk*maxyblk*2); /* noskip flags off */
         calcmode=2;
         row=iystart;
         for(col=ixstart; col<=ixstop; col+=maxblock)
         { /* calc top row */
            if((*calctype)()==-1)
            {
               add_worklist(xxstart,xxstop,yystart,yystop,yybegin,0,worksym);
               goto exit_solidguess;
            }
            calcmode = 3;
         }
      }
      else
         memset(&prefix[1][0][0],-1,maxxblk*maxyblk*2); /* noskip flags on */
      for(y=iystart; y<=iystop; y+=blocksize)
      {
         i = 0;
         if(y+blocksize<=iystop)
         { /* calc the row below */
            row=y+blocksize;
            calcmode = 2; /* for the first dot on the row */
            for(col=ixstart; col<=ixstop; col+=maxblock)
            {
               if((i=(*calctype)()) == -1)
                  break;
               calcmode = 3;
            }
         }
         calcmode=2;
         if (i == -1 || guessrow(1,y,blocksize) != 0) /* interrupted? */
         {
            if (y < yystart)
               y = yystart;
            add_worklist(xxstart,xxstop,yystart,yystop,y,0,worksym);
            goto exit_solidguess;
         }
      }

      if (num_worklist) /* work list not empty, just do 1st pass */
      {
         add_worklist(xxstart,xxstop,yystart,yystop,yystart,1,worksym);
         goto exit_solidguess;
      }
      ++workpass;
      iystart = yystart & (-1 - (maxblock-1));

      /* calculate skip flags for skippable blocks */
      xlim=(ixstop+maxblock)/maxblock+1;
      ylim=((iystop+maxblock)/maxblock+15)/16+1;
      if(right_guess==0) /* no right edge guessing, zap border */
         for(y=0;y<=ylim;++y)
            prefix[1][y][xlim]=-1;
      if(bottom_guess==0) /* no bottom edge guessing, zap border */
      {
         i=(iystop+maxblock)/maxblock+1;
         y=i/16+1;
         i=1<<(i&15);
         for(x=0;x<=xlim;++x)
            prefix[1][y][x]|=i;
      }
      /* set each bit in prefix[0] to OR of it & surrounding 8 in prefix[1] */
      for(y=0;++y<ylim;)
      {
         pfxp0=&prefix[0][y][0];
         pfxp1=&prefix[1][y][0];
         for(x=0;++x<xlim;)
         {
            ++pfxp1;
            u=*(pfxp1-1)|*pfxp1|*(pfxp1+1);
            *(++pfxp0)=u|(u>>1)|(u<<1)
               |((*(pfxp1-(maxxblk+1))|*(pfxp1-maxxblk)|*(pfxp1-(maxxblk-1)))>>15)
                  |((*(pfxp1+(maxxblk-1))|*(pfxp1+maxxblk)|*(pfxp1+(maxxblk+1)))<<15);
         }
      }
   }
   else /* first pass already done */
      memset(&prefix[0][0][0],-1,maxxblk*maxyblk*2); /* noskip flags on */

   /* remaining pass(es), halve blocksize & quarter each blocksize**2 */
   i = workpass;
   while (--i > 0) /* allow for already done passes */
      blocksize = blocksize>>1;
   calcmode=2;
   while((blocksize=blocksize>>1)>=2)
   {
      for(y=iystart; y<=iystop; y+=blocksize)
         if(guessrow(0,y,blocksize)!=0)
         {
            if (y < yystart)
               y = yystart;
            add_worklist(xxstart,xxstop,yystart,yystop,y,workpass,worksym);
            goto exit_solidguess;
         }
      ++workpass;
      if (num_worklist /* work list not empty, do one pass at a time */
      && blocksize>2) /* if 2, we just did last pass */
      {
         add_worklist(xxstart,xxstop,yystart,yystop,yystart,workpass,worksym);
         goto exit_solidguess;
      }
      iystart = yystart & (-1 - (maxblock-1));
   }

   exit_solidguess:
   return(0);
}

ssg_blocksize() /* used above and by zoom panning */
{  
   int blocksize,i;
   /* blocksize 4 if <300 rows, 8 if 300-599, 16 if 600-1199, 32 if >=1200 */
   blocksize=4;
   i=300;
   while(i<=ydots)
   {
      blocksize+=blocksize;
      i+=i;
   }
   /* increase blocksize if prefix array not big enough */
   while(blocksize*(maxxblk-2)<xdots || blocksize*(maxyblk-2)*16<ydots)
      blocksize+=blocksize;
   return(blocksize);
}

#define calcadot(c,x,y) { col=x; row=y; if((c=(*calctype)())==-1) return -1; }

int guessrow(int firstpass,int y,int blocksize)
{
   int x,i,j,color;
   int xplushalf,xplusblock;
   int ylessblock,ylesshalf,yplushalf,yplusblock;
   int     c21,c31,c41;         /* cxy is the color of pixel at (x,y) */
   int c12,c22,c32,c42;         /* where c22 is the topleft corner of */
   int c13,c23,c33;             /* the block being handled in current */
   int     c24,    c44;         /* iteration                          */
   int guessed23,guessed32,guessed33,guessed12,guessed13;
   int orig23,orig32,orig33;
   int prev11,fix21,fix31;
   unsigned int *pfxptr,pfxmask;

   halfblock=blocksize>>1;
   i=y/maxblock;
   pfxptr=&prefix[firstpass][(i>>4)+1][ixstart/maxblock];
   pfxmask=1<<(i&15);
   ylesshalf=y-halfblock;
   ylessblock=y-blocksize; /* constants, for speed */
   yplushalf=y+halfblock;
   yplusblock=y+blocksize;
   prev11=-1;
   c24=c12=c13=c22=getcolor(ixstart,y);
   c31=c21=getcolor(ixstart,(y>0)?ylesshalf:0);
   if(yplusblock<=iystop)
      c24=getcolor(ixstart,yplusblock);
   else if(bottom_guess==0)
      c24=-1;
   guessed12=guessed13=0;

   for(x=ixstart; x<=ixstop;)  /* increment at end, or when doing continue */
   {
      if((x&(maxblock-1))==0)  /* time for skip flag stuff */
      {
         ++pfxptr;
         if(firstpass==0 && (*pfxptr&pfxmask)==0)  /* check for fast skip */
         {
            /* next useful in testing to make skips visible */
            /*
                           if(halfblock==1)
                           {
                              (*plot)(x+1,y,0); (*plot)(x,y+1,0); (*plot)(x+1,y+1,0);
                              }
                         */
            x+=maxblock;
            prev11=c31=c21=c24=c12=c13=c22;
            guessed12=guessed13=0;
            continue;
         }
      }

      if(firstpass)  /* 1st pass, paint topleft corner */
         plotblock(0,x,y,c22);
      /* setup variables */
      xplusblock=(xplushalf=x+halfblock)+halfblock;
      if(xplushalf>ixstop)
      {
         if(right_guess==0)
            c31=-1;
      }
      else if(y>0)
         c31=getcolor(xplushalf,ylesshalf);
      if(xplusblock<=ixstop)
      {
         if(yplusblock<=iystop)
            c44=getcolor(xplusblock,yplusblock);
         c41=getcolor(xplusblock,(y>0)?ylesshalf:0);
         c42=getcolor(xplusblock,y);
      }
      else if(right_guess==0)
         c41=c42=c44=-1;
      if(yplusblock>iystop)
         c44=(bottom_guess)?c42:-1;

      /* guess or calc the remaining 3 quarters of current block */
      guessed23=guessed32=guessed33=1;
      c23=c32=c33=c22;
      if(yplushalf>iystop)
      {
         if(bottom_guess==0)
            c23=c33=-1;
         guessed23=guessed33=-1;
      }
      if(xplushalf>ixstop)
      {
         if(right_guess==0)
            c32=c33=-1;
         guessed32=guessed33=-1;
      }
      while(1) /* go around till none of 23,32,33 change anymore */
      {
         if(guessed33>0
             && (c33!=c44 || c33!=c42 || c33!=c24 || c33!=c32 || c33!=c23))
         {
            calcadot(c33,xplushalf,yplushalf);
            guessed33=0;
         }
         if(guessed32>0
             && (c32!=c33 || c32!=c42 || c32!=c31 || c32!=c21
             || c32!=c41 || c32!=c23))
         {
            calcadot(c32,xplushalf,y);
            guessed32=0;
            continue;
         }
         if(guessed23>0
             && (c23!=c33 || c23!=c24 || c23!=c13 || c23!=c12 || c23!=c32))
         {
            calcadot(c23,x,yplushalf);
            guessed23=0;
            continue;
         }
         break;
      }

      if(firstpass) /* note whether any of block's contents were calculated */
         if(guessed23==0 || guessed32==0 || guessed33==0)
            *pfxptr|=pfxmask;

      if(halfblock>1) /* not last pass, check if something to display */
         if(firstpass)  /* display guessed corners, fill in block */
         {
            if(guessplot)
            {
               if(guessed23>0)
                  (*plot)(x,yplushalf,c23);
               if(guessed32>0)
                  (*plot)(xplushalf,y,c32);
               if(guessed33>0)
                  (*plot)(xplushalf,yplushalf,c33);
            }
            plotblock(1,x,yplushalf,c23);
            plotblock(0,xplushalf,y,c32);
            plotblock(1,xplushalf,yplushalf,c33);
         }
         else  /* repaint changed blocks */
         {
            if(c23!=c22)
               plotblock(-1,x,yplushalf,c23);
            if(c32!=c22)
               plotblock(-1,xplushalf,y,c32);
            if(c33!=c22)
               plotblock(-1,xplushalf,yplushalf,c33);
         }

      /* check if some calcs in this block mean earlier guesses need fixing */
      fix21=((c22!=c12 || c22!=c32)
          && c21==c22 && c21==c31 && c21==prev11
          && y>0
          && (x==ixstart || c21==getcolor(x-halfblock,ylessblock))
          && (xplushalf>ixstop || c21==getcolor(xplushalf,ylessblock))
          && c21==getcolor(x,ylessblock));
      fix31=(c22!=c32
          && c31==c22 && c31==c42 && c31==c21 && c31==c41
          && y>0 && xplushalf<=ixstop
          && c31==getcolor(xplushalf,ylessblock)
          && (xplusblock>ixstop || c31==getcolor(xplusblock,ylessblock))
          && c31==getcolor(x,ylessblock));
      prev11=c31; /* for next time around */
      if(fix21)
      {
         calcadot(c21,x,ylesshalf);
         if(halfblock>1 && c21!=c22)
            plotblock(-1,x,ylesshalf,c21);
      }
      if(fix31)
      {
         calcadot(c31,xplushalf,ylesshalf);
         if(halfblock>1 && c31!=c22)
            plotblock(-1,xplushalf,ylesshalf,c31);
      }
      if(c23!=c22)
      {
         if(guessed12)
         {
            calcadot(c12,x-halfblock,y);
            if(halfblock>1 && c12!=c22)
               plotblock(-1,x-halfblock,y,c12);
         }
         if(guessed13)
         {
            calcadot(c13,x-halfblock,yplushalf);
            if(halfblock>1 && c13!=c22)
               plotblock(-1,x-halfblock,yplushalf,c13);
         }
      }
      c22=c42;
      c24=c44;
      c13=c33;
      c31=c21=c41;
      c12=c32;
      guessed12=guessed32;
      guessed13=guessed33;
      x+=blocksize;
   } /* end x loop */

   if(firstpass==0 || guessplot) return 0;

   /* paint rows the fast way */
   for(i=0;i<halfblock;++i)
   {
      if((j=y+i)<=iystop)
         put_line(j,xxstart,ixstop,&dstack[xxstart]);
      if((j=y+i+halfblock)<=iystop)
         put_line(j,xxstart,ixstop,&dstack[xxstart+2048]);
   }
   if(plot!=putcolor)  /* symmetry, just vertical & origin the fast way */
   {
      if(plot==symplot2J) /* origin sym, reverse lines */
         for(i=(ixstop+xxstart+1)/2;--i>=xxstart;)
         {
            color=dstack[i];
            dstack[i]=dstack[j=ixstop-(i-xxstart)];
            dstack[j]=color;
            j+=2048;
            color=dstack[i+2048];
            dstack[i+2048]=dstack[j];
            dstack[j]=color;
         }
      for(i=0;i<halfblock;++i)
      {
         if((j=yystop-(y+i-yystart))>iystop && j<ydots)
            put_line(j,xxstart,ixstop,&dstack[xxstart]);
         if((j=yystop-(y+i+halfblock-yystart))>iystop && j<ydots)
            put_line(j,xxstart,ixstop,&dstack[xxstart+2048]);
      }
   }
   return 0;
}

plotblock(int buildrow,int x,int y,int color)
{
   int i,xlim,ylim;
   if((xlim=x+halfblock)>ixstop)
      xlim=ixstop+1;
   if(buildrow>=0 && guessplot==0) /* save it for later put_line */
   {
      if(buildrow==0)
         for(i=x;i<xlim;++i)
            dstack[i]=color;
      else
         for(i=x;i<xlim;++i)
            dstack[i+2048]=color;
      if (x>=xxstart) /* when x reduced for alignment, paint those dots too */
         return(0); /* the usual case */
   }
   /* paint it */
   if((ylim=y+halfblock)>iystop)
   {
      if(y>iystop)
         return(0);
      ylim=iystop+1;
   }
   for(i=x;++i<xlim;)
      (*plot)(i,y,color); /* skip 1st dot on 1st row */
   while(++y<ylim)
      for(i=x;i<xlim;++i)
         (*plot)(i,y,color);
}

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia      */
/* Reference: Science of Fractal Images p. 190.                   */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.          */
/*                                                                */
/* The purpose of this function is to calculate a color value     */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.                          */
/*                                                                */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential    */
/* surface. Returns color.  - Tim Wegner 6/25/89                  */
/*                                                                */
/*                     -- Change history --                       */
/*                                                                */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*                                                                */
/******************************************************************/

int potential(double mag, int iterations)
{
   extern char potfile[];     /* name of pot save file */
   extern struct fractal_info save_info;        /*  for saving data */

   static int x,y;            /* keep track of position in image */
   float f_mag,f_tmp,pot;
   double d_tmp;
   int i_pot;
   unsigned int intbuf;
   FILE *t16_create();

   extern int maxit;          /* maximum iteration bailout limit */
   extern double potparam[];     /* continuous potential parameters */
   extern char potfile[];     /* pot savename */
   extern char floatflag;

   if(top)
   {
      top = 0;
      x   = 0;
      y   = 0;
      /* make sure file is closed */
      if(fp_pot)
         fclose(fp_pot);
      if(potfile[0])
         fp_pot = t16_create(potfile, xdots, ydots,
             sizeof(struct fractal_info), &save_info);
   }
   if(iterations < maxit)
   {
      i_pot = pot = iterations+2;
      if(i_pot <= 0 || mag <= 1.0)
         pot = 0.0;
      else /* pot = log(mag) / pow(2.0, (double)pot); */
      {
         i_pot = pot;
         if(i_pot < 120 && !floatflag) /* empirically determined limit of fShift */
         {
            f_mag = mag;
            fLog14(f_mag,f_tmp); /* this SHOULD be non-negative */
            fShift(f_tmp,-i_pot,pot);
         }
         else
         {
            d_tmp = log(mag)/(double)pow(2.0,(double)pot);
            if(d_tmp > FLT_MIN) /* prevent float type underflow */
               pot = d_tmp;
            else
               pot = 0.0;
         }
      }
      /* following transformation strictly for aesthetic reasons */
      /* meaning of parameters:
            potparam[0] -- zero potential level - highest color -
            potparam[1] -- slope multiplier -- higher is steeper
            potparam[2] -- rqlim value if changeable (bailout for modulus) */

      if(pot > 0.0)
      {
         if(floatflag)
            pot = (float)sqrt((double)pot);
         else
         {
            fSqrt14(pot,f_tmp);
            pot = f_tmp;
         }
         pot = potparam[0] - pot*potparam[1] - 1.0;
      }
      else
         pot = potparam[0] - 1.0;
   }
   else if(inside > 0)
      pot = inside; /* negative inside turns off inside replacement */
   else
      pot = potparam[0];

   if(pot > colors)
      pot = colors -1;
   if(pot < 1.0)
      pot = 1.0; /* avoid color 0 */

   if(fp_pot)
   {
      intbuf = pot*(1<<8); /* shift 8 bits */
      boxx[x] = intbuf;
      if(++x == xdots)
      {
         /* new row */
         t16_putline(fp_pot, xdots, boxx);
         x = 0;
         if(++y == ydots)
         {
            /* we're done */
            t16_flush(fp_pot);
            fclose(fp_pot);
            fp_pot = NULL;
            potfile[0] = 0;
         }
      }
   }
   return((int)pot);
}

/*
   "intpotential" is called by the MANDEL/JULIA routines with scaled long
   integer magnitudes.  The routine just calls "potential".  Bert
*/

int intpotential(unsigned long longmagnitude, int iterations)
{                                /* this version called from calcmand() */
   double magnititude;

   magnitude = ((double)longmagnitude) / fudge;
   magnitude = 256*magnitude;
   return(potential(magnitude, iterations));
}

static int xsym_split(int xaxis_row,int xaxis_between)
{
   int i;
   if ((worksym&0x11) == 0x10) /* already decided not sym */
      return(1);
   if ((worksym&1) != 0) /* already decided on sym */
      iystop = (yystart+yystop)/2;
   else /* new window, decide */
   {
      worksym |= 0x10;
      if (xaxis_row <= yystart || xaxis_row >= yystop)
         return(1); /* axis not in window */
      i = xaxis_row + (xaxis_row - yystart);
      if (xaxis_between)
         ++i;
      if (i > yystop) /* split into 2 pieces, bottom has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         iystop = xaxis_row - (yystop - xaxis_row);
         if (!xaxis_between)
            --iystop;
         add_worklist(xxstart,xxstop,iystop+1,yystop,iystop+1,workpass,0);
         yystop = iystop;
         return(1); /* tell set_symmetry no sym for current window */
      }
      if (i < yystop) /* split into 2 pieces, top has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         add_worklist(xxstart,xxstop,i+1,yystop,i+1,workpass,0);
         yystop = i;
      }
      iystop = xaxis_row;
      worksym |= 1;
   }
   symmetry = 0;
   return(0); /* tell set_symmetry its a go */
}

static int ysym_split(int yaxis_col,int yaxis_between)
{
   int i;
   if ((worksym&0x22) == 0x20) /* already decided not sym */
      return(1);
   if ((worksym&2) != 0) /* already decided on sym */
      ixstop = (xxstart+xxstop)/2;
   else /* new window, decide */
   {
      worksym |= 0x20;
      if (yaxis_col <= xxstart || yaxis_col >= xxstop)
         return(1); /* axis not in window */
      i = yaxis_col + (yaxis_col - xxstart);
      if (yaxis_between)
         ++i;
      if (i > xxstop) /* split into 2 pieces, right has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         ixstop = yaxis_col - (xxstop - yaxis_col);
         if (!yaxis_between)
            --ixstop;
         add_worklist(ixstop+1,xxstop,yystart,yystop,yystart,workpass,0);
         xxstop = ixstop;
         return(1); /* tell set_symmetry no sym for current window */
      }
      if (i < xxstop) /* split into 2 pieces, left has the symmetry */
      {
         if (num_worklist >= MAXCALCWORK-1) /* no room to split */
            return(1);
         add_worklist(i+1,xxstop,yystart,yystop,yystart,workpass,0);
         xxstop = i;
      }
      ixstop = yaxis_col;
      worksym |= 2;
   }
   symmetry = 0;
   return(0); /* tell set_symmetry its a go */
}

static void setsymmetry(int sym, int uselist) /* set up proper symmetrical plot functions */
{
   extern int forcesymmetry;
   int parmszero;
   int xaxis_row, yaxis_col;         /* pixel number for origin */
   int xaxis_between, yaxis_between; /* if axis between 2 pixels, not on one */
   double ftemp;
   symmetry = 1;
   if(sym == NOPLOT && forcesymmetry == 999)
   {
      plot = noplot;
      return;
   }
   /* NOTE: 16-bit potential disables symmetry */
   /* also any decomp= option and any inversion not about the origin */
   /* also any rotation other than 180deg and any off-axis stretch */
   if (potfile[0] || (invert && inversion[2] != 0.0)
       || decomp[0] != 0
       || xxmin!=xx3rd || yymin!=yy3rd)
      return;
   if(sym != XAXIS && sym != XAXIS_NOPARM && inversion[1] != 0.0 && forcesymmetry == 999)
      return;
   if(forcesymmetry != 999)
      sym = forcesymmetry;
   parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
   xaxis_row = yaxis_col = -1;
   if (fabs(yymin+yymax) < fabs(yymin)+fabs(yymax)) /* axis is on screen */
   {
      ftemp = (0.0-yymax) / (yymin-yymax) * (ydots-1) + 0.25;
      xaxis_row = ftemp;
      xaxis_between = (ftemp - xaxis_row >= 0.5);
      if (uselist == 0 && (!xaxis_between || (xaxis_row+1)*2 != ydots))
         xaxis_row = -1; /* can't split screen, so dead center or not at all */
   }
   if (fabs(xxmin+xxmax) < fabs(xxmin)+fabs(xxmax)) /* axis is on screen */
   {
      ftemp = (0.0-xxmin) / (xxmax-xxmin) * (xdots-1) + 0.25;
      yaxis_col = ftemp;
      yaxis_between = (ftemp - yaxis_col >= 0.5);
      if (uselist == 0 && (!yaxis_between || (yaxis_col+1)*2 != xdots))
         yaxis_col = -1; /* can't split screen, so dead center or not at all */
   }
   switch(sym)       /* symmetry switch */
   {
   case XAXIS_NOREAL:    /* X-axis Symmetry (no real param) */
      if (parm.x != 0.0) break;
      goto xsym;
   case XAXIS_NOIMAG:    /* X-axis Symmetry (no imag param) */
      if (parm.y != 0.0) break;
      goto xsym;
   case XAXIS_NOPARM:                        /* X-axis Symmetry  (no params)*/
      if (!parmszero)
         break;
      xsym:
   case XAXIS:                       /* X-axis Symmetry */
      if (xsym_split(xaxis_row,xaxis_between) == 0)
         if(basin)
            plot = symplot2basin;
         else
            plot = symplot2;
      break;
   case YAXIS_NOPARM:                        /* Y-axis Symmetry (No Parms)*/
      if (!parmszero)
         break;
   case YAXIS:                       /* Y-axis Symmetry */
      if (ysym_split(yaxis_col,yaxis_between) == 0)
         plot = symplot2Y;
      break;
   case XYAXIS_NOPARM:                       /* X-axis AND Y-axis Symmetry (no parms)*/
      if(!parmszero)
         break;
   case XYAXIS:                      /* X-axis AND Y-axis Symmetry */
      xsym_split(xaxis_row,xaxis_between);
      ysym_split(yaxis_col,yaxis_between);
      switch (worksym & 3)
      {
      case 1: /* just xaxis symmetry */
         if(basin)
            plot = symplot2basin;
         else
            plot = symplot2 ;
         break;
      case 2: /* just yaxis symmetry */
         if (basin) /* got no routine for this case */
         {
            ixstop = xxstop; /* fix what split should not have done */
            symmetry = 1;
         }
         else
            plot = symplot2Y;
         break;
      case 3: /* both axes */
         if(basin)
            plot = symplot4basin;
         else
            plot = symplot4 ;
      }
      break;
   case ORIGIN_NOPARM:                       /* Origin Symmetry (no parms)*/
      if (!parmszero)
         break;
   case ORIGIN:                      /* Origin Symmetry */
      originsym:
      if ( xsym_split(xaxis_row,xaxis_between) == 0
          && ysym_split(yaxis_col,yaxis_between) == 0)
      {
         plot = symplot2J;
         ixstop = xxstop; /* didn't want this changed */
      }
      else
      {
         iystop = yystop; /* in case first split worked */
         symmetry = 1;
         worksym = 0x30; /* let it recombine with others like it */
      }
      break;
   case PI_SYM_NOPARM:
      if (!parmszero)
         break;
   case PI_SYM:                      /* PI symmetry */
      if(invert && forcesymmetry == 999)
         goto originsym;
      plot = symPIplot ;
      symmetry = 0;
      if ( xsym_split(xaxis_row,xaxis_between) == 0
          && ysym_split(yaxis_col,yaxis_between) == 0)
         if(parm.y == 0.0)
            plot = symPIplot4J; /* both axes */
         else
            plot = symPIplot2J; /* origin */
      else
      {
         iystop = yystop; /* in case first split worked */
         worksym = 0x30;  /* don't mark pisym as ysym, just do it unmarked */
      }
      pixelpi = (PI/fabs(xxmax-xxmin))*xdots; /* PI in pixels */
      if ( (ixstop = xxstart+pixelpi-1 ) > xxstop)
         ixstop = xxstop;
      if (plot == symPIplot4J && ixstop > (i = (xxstart+xxstop)/2))
         ixstop = i;
      break;
   default:                  /* no symmetry */
      break;
   }
}

int add_worklist(int xfrom, int xto, int yfrom, int yto, int ybegin,
int pass, int sym)
{
   int i;
   if (num_worklist >= MAXCALCWORK)
      return(-1);
   worklist[num_worklist].xxstart = xfrom;
   worklist[num_worklist].xxstop  = xto;
   worklist[num_worklist].yystart = yfrom;
   worklist[num_worklist].yystop  = yto;
   worklist[num_worklist].yybegin = ybegin;
   worklist[num_worklist].pass    = pass;
   worklist[num_worklist].sym     = sym;
   ++num_worklist;
   tidy_worklist();
   return(0);
}

static int combine_worklist() /* look for 2 entries which can freely merge */
{
   int i,j;
   for (i=0; i<num_worklist; ++i)
      if (worklist[i].yystart == worklist[i].yybegin)
         for (j=i+1; j<num_worklist; ++j)
            if (worklist[j].sym == worklist[i].sym
                && worklist[j].yystart == worklist[j].yybegin
                && worklist[i].pass == worklist[j].pass)
            {
               if ( worklist[i].xxstart == worklist[j].xxstart
                   && worklist[i].xxstop  == worklist[j].xxstop)
               {
                  if (worklist[i].yystop+1 == worklist[j].yystart)
                  {
                     worklist[i].yystop = worklist[j].yystop;
                     return(j);
                  }
                  if (worklist[j].yystop+1 == worklist[i].yystart)
                  {
                     worklist[i].yystart = worklist[j].yystart;
                     worklist[i].yybegin = worklist[j].yybegin;
                     return(j);
                  }
               }
               if ( worklist[i].yystart == worklist[j].yystart
                   && worklist[i].yystop  == worklist[j].yystop)
               {
                  if (worklist[i].xxstop+1 == worklist[j].xxstart)
                  {
                     worklist[i].xxstop = worklist[j].xxstop;
                     return(j);
                  }
                  if (worklist[j].xxstop+1 == worklist[i].xxstart)
                  {
                     worklist[i].xxstart = worklist[j].xxstart;
                     return(j);
                  }
               }
            }
   return(0); /* nothing combined */
}

void tidy_worklist() /* combine mergeable entries, resort */
{
   int i,j;
   struct workliststuff tempwork;
   while (i=combine_worklist())
   { /* merged two, delete the gone one */
      while (++i < num_worklist)
         worklist[i-1] = worklist[i];
      --num_worklist;
   }
   for (i=0; i<num_worklist; ++i)
      for (j=i+1; j<num_worklist; ++j)
         if (worklist[j].pass < worklist[i].pass
             || (worklist[j].pass == worklist[i].pass
             && (worklist[j].yystart < worklist[i].yystart
             || (   worklist[j].yystart == worklist[i].yystart
             && worklist[j].xxstart <  worklist[i].xxstart))))
         { /* dumb sort, swap 2 entries to correct order */
            tempwork = worklist[i];
            worklist[i] = worklist[j];
            worklist[j] = tempwork;
         }
}

void get_julia_attractor (double real, double imag)
{
   struct lcomplex lresult;
   struct complex result;
   int savper,savmaxit;

   if (attractors == 0 && finattract == 0) /* not magnet & not requested */
      return;

   if (attractors >= N_ATTR)     /* space for more attractors ?  */
      return;                  /* Bad luck - no room left !    */

   savper = periodicitycheck;
   savmaxit = maxit;
   periodicitycheck = 0;
   old.x = real;                    /* prepare for f.p orbit calc */
   old.y = imag;
   tempsqrx = sqr(old.x);
   tempsqry = sqr(old.y);

   lold.x = real;           /* prepare for int orbit calc */
   lold.y = imag;
   ltempsqrx = tempsqrx;
   ltempsqry = tempsqry;

   lold.x = lold.x << bitshift;
   lold.y = lold.y << bitshift;
   ltempsqrx = ltempsqrx << bitshift;
   ltempsqry = ltempsqry << bitshift;

   if (maxit < 500)         /* we're going to try at least this hard */
      maxit = 500;
   color = 0;
   while (++color < maxit)
      if(fractalspecific[fractype].orbitcalc())
         break;
   if (color >= maxit)      /* if orbit stays in the lake */
   {
      if (integerfractal)   /* remember where it went to */
         lresult = lnew;
      else
         result =  new;
      if(!fractalspecific[fractype].orbitcalc()) /* if it stays in the lake */
      {                        /* and doen't move far, probably */
         if (integerfractal)   /*   found a finite attractor    */
         {
            if(labs(lresult.x-lnew.x) < lclosenuff
                && labs(lresult.y-lnew.y) < lclosenuff)
            {
               lattr[attractors] = lnew;
               attractors++;   /* another attractor - coloured lakes ! */
            }
         }
         else
         {
            if(fabs(result.x-new.x) < closenuff
                && fabs(result.y-new.y) < closenuff)
            {
               attr[attractors] = new;
               attractors++;   /* another attractor - coloured lakes ! */
            }
         }
      }
   }
   if(attractors==0)
      periodicitycheck = savper;
   maxit = savmaxit;
}

