/*
CALCFRAC.C contains the high level ("engine") code for calculating the
fractal images (well, SOMEBODY had to do it!).
Original author Tim Wegner, but just about ALL the authors have contributed
SOME code to this routine at one time or another, or contributed to one of
the many massive restructurings.
This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
The following modules work very closely with CALCFRAC.C:
  FRACTALS.C	the fractal-specific code for escape-time fractals.
  FRACSUBR.C	assorted subroutines belonging mainly to calcfrac.
  CALCMAND.ASM	fast Mandelbrot/Julia integer implementation
Additional fractal-specific modules are also invoked from CALCFRAC:
  LORENZ.C	engine level and fractal specific code for attractors.
  JB.C		julibrot logic
  PARSER.C	formula fractals
  and more
 -------------------------------------------------------------------- */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
#include <dos.h>
#include <limits.h>
#include "fractint.h"
#include "fractype.h"
#include "mpmath.h"
#include "targa_lc.h"

/* routines in this module	*/

void calcfrac_overlay(void);
int  calcfract(void);
/* the rest are called only within same overlay, thus don't need ENTER_OVLY */
int  StandardFractal(void);
int  calcmand(void);
int  calcmandfp(void);
int  plasma(void);
int  diffusion(void);
int  test(void);
int  Bifurcation(void);
int  BifurcVerhulst(void),LongBifurcVerhulst(void),BifurcLambda(void);
int  LongBifurcLambda(void),BifurcAddSinPi(void),BifurcSetSinPi(void);
int  BifurcStewart(void),LongBifurcStewart(void);
int  popcorn(void);

static void perform_worklist(void);
static int  OneOrTwoPass(void);
static int  _fastcall StandardCalc(int);
static int  _fastcall potential(double,int);
static void decomposition(void);
static int  bound_trace_main(void);
static int  _fastcall boundary_trace(int,int);
static int  _fastcall calc_xy(int,int);
static int  _fastcall fillseg1(int,int,int,int);
static int  _fastcall fillseg(int,int,int,int);
static void _fastcall reverse_string(char *,char *,int);
static int  solidguess(void);
static int  _fastcall guessrow(int,int,int);
static void _fastcall plotblock(int,int,int,int);
static void _fastcall setsymmetry(int,int);
static int  _fastcall xsym_split(int,int);
static int  _fastcall ysym_split(int,int);
static void set_Plasma_palette(void);
static int _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb);
static void _fastcall subDivide(int x1,int y1,int x2,int y2);
static int _fastcall new_adjust(int xa,int ya,int x,int y,int xb,int yb);
static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur);
static void verhulst(void);
static void Bif_Period_Init(void);
static int  _fastcall Bif_Periodic(int);

/**CJLT new function prototypes: */
static int tesseral(void);
static int _fastcall tesschkcol(int,int,int);
static int _fastcall tesschkrow(int,int,int);
static int _fastcall tesscol(int,int,int);
static int _fastcall tessrow(int,int,int);

extern struct complex initorbit;
extern char useinitorbit;
struct lcomplex linitorbit;
extern int showdot;
extern unsigned int decoderline[];
extern int overflow;
long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
struct complex init,tmp,old,new,saved;
extern int biomorph,usr_biomorph;
extern struct lcomplex linit;
extern int basin;
extern int cpu;
extern char savename[80];   /* save files using this name */
extern int resave_flag;
extern int started_resaves;
extern int dotmode;

int color, oldcolor, row, col, passes;
int realcolor;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;

extern int LogFlag;
extern unsigned char far *LogTable;
extern int rangeslen;
extern int far *ranges;

void (_fastcall *plot)(int,int,int) = putcolor;

extern double inversion[];	    /* inversion radius, f_xcenter, f_ycenter */
extern int	xdots, ydots;	    /* coordinates of dots on the screen  */
extern int	sxdots,sydots;
extern int	sxoffs,syoffs;
extern int	colors; 	    /* maximum colors available */
extern int	andcolor;	    /* colors-1 		*/
extern int	inside; 	    /* "inside" color to use    */
extern int	fillcolor; 	    /* "fillcolor" color to use    */
extern int	outside;	    /* "outside" color to use   */
extern int	finattract;
double		min_orbit;	    /* orbit value closest to origin */
int		min_index;	    /* iteration of min_orbit */
extern int	maxit;		    /* try this many iterations */
extern int	fractype;	    /* fractal type */
extern char	stdcalcmode;	    /* '1', '2', 'g', 'b' */
extern int	debugflag;	    /* for debugging purposes */
extern	int	diskvideo;	    /* for disk-video klooges */
extern int	calc_status;	    /* status of calculations */
extern long	calctime;	    /* total calc time for image */

extern int rflag, rseed;
extern int decomp[];
extern int distest,distestwidth;

extern double	param[];	    /* parameters */
extern int	potflag;	    /* potential enabled? */
extern double	potparam[];	    /* potential parameters */
extern int	pot16bit;	    /* store 16bit continuous potential */
extern long	far *lx0, far *ly0; /* X, Y points */
extern long	far *lx1, far *ly1; /* X, Y points */
extern long	fudge;		    /* fudge factor (2**n) */
extern int	bitshift;	    /* bit shift for fudge */
extern long	delmin; 	    /* for periodicity checking */

extern double xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* corners */
extern long   xmin, xmax, ymin, ymax;		   /* integer equivs */
extern long   delx,dely;			   /* X, Y increments */
extern double delxx,delxx2,delyy,delyy2;
double deltaX, deltaY;
double magnitude, rqlim, rqlim2, rqlim_save;
extern struct complex parm,parm2;
int (*calctype)();
double closenuff;
int pixelpi; /* value of pi in pixels */
unsigned long lm;		/* magnitude limit (CALCMAND) */
extern long linitx,linity;	/* in calcmand */
extern unsigned long savedmask; /* in calcmand */
extern int TranspSymmetry;

/* ORBIT variables */
int	show_orbit;			/* flag to turn on and off */
int	orbit_ptr;			/* pointer into save_orbit array */
int far *save_orbit;			/* array to save orbit values */
int	orbit_color=15; 		/* XOR color */

int	ixstart, ixstop, iystart, iystop;	/* start, stop here */
int	symmetry;	   /* symmetry flag */
int	reset_periodicity; /* nonzero if escape time pixel rtn to reset */
int	kbdcount, max_kbdcount;    /* avoids checking keyboard too often */

extern	int	integerfractal; 	/* TRUE if fractal uses integer math */

char far *resume_info = NULL;		/* pointer to resume info if allocated */
int resuming;				/* nonzero if resuming after interrupt */
int num_worklist;			/* resume worklist for standard engine */
struct workliststuff worklist[MAXCALCWORK];
int xxstart,xxstop;			/* these are same as worklist, */
int yystart,yystop,yybegin;		/* declared as separate items  */
int workpass,worksym;			/* for the sake of calcmand    */

extern long timer_interval;		/* timer(...) total */

extern void far *typespecific_workarea = NULL;

static double dem_delta, dem_width;	/* distance estimator variables */
static double dem_toobig;
#define DEM_BAILOUT 535.5  /* (pb: not sure if this is special or arbitrary) */

/* variables which must be visible for tab_display */
int got_status; /* -1 if not, 0 for 1or2pass, 1 for ssg, 2 for btm, 3 for 3d */
int curpass,totpasses;
int currow,curcol;

/* static vars for solidguess & its subroutines */
static int maxblock,halfblock;
static int guessplot;			/* paint 1st pass row at a time?   */
static int right_guess,bottom_guess;
#define maxyblk 7    /* maxxblk*maxyblk*2 <= 4096, the size of "prefix" */
#define maxxblk 202  /* each maxnblk is oversize by 2 for a "border" */
		     /* maxxblk defn must match fracsubr.c */
/* next has a skip bit for each maxblock unit;
   1st pass sets bit  [1]... off only if block's contents guessed;
   at end of 1st pass [0]... bits are set if any surrounding block not guessed;
   bits are numbered [..][y/16+1][x+1]&(1<<(y&15)) */
extern unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
/* size of next puts a limit of 2048 pixels across on solid guessing logic */
extern char dstack[4096];		/* common temp, two put_line calls */

int	attractors;		    /* number of finite attractors  */
struct complex	attr[N_ATTR];	    /* finite attractor vals (f.p)  */
struct lcomplex lattr[N_ATTR];	    /* finite attractor vals (int)  */

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */
extern struct lcomplex lold,lnew,lparm,lparm2;	 /* added "lold" */

int periodicitycheck;
extern long ltempsqrx,ltempsqry;
extern double tempsqrx,tempsqry;
extern LCMPLX ltmp;
extern int display3d;


void calcfrac_overlay() { }	/* for restore_active_ovly */


/******* calcfract - the top level routine for generating an image *******/

int calcfract()
{
   ENTER_OVLY(OVLY_CALCFRAC);

   attractors = 0;	    /* default to no known finite attractors  */
   display3d = 0;
   basin = 0;

   init_misc();  /* set up some variables in parser.c */

   /* following delta values useful only for types with rotation disabled */
   /* currently used only by bifurcation */
   if (integerfractal)
   {
      distest = 0;
      deltaX = (double)lx0[	 1] / fudge - xxmin;
      deltaY = yymax - (double)ly0[	 1] / fudge;
   }
   else
   {
      deltaX = dx0[	 1] - xxmin;
      deltaY = yymax - dy0[	 1];
   }

   parm.x   = param[0];
   parm.y   = param[1];
   parm2.x  = param[2];
   parm2.y  = param[3];

   if (LogFlag && colors < 16) {
      static char far msg[]={"Need at least 16 colors to use logmap"};
      stopmsg(0,msg);
      LogFlag = 0;
      }
   if (LogFlag || rangeslen)
      if (!(LogTable = farmemalloc((long)maxit + 1))) {
	  static char far msg[]={"Insufficient memory for logmap/ranges with this maxiter"};
      stopmsg(0,msg);
      }
      else if (rangeslen) {
	 int i,k,l,m,numval,flip,altern;
	 i = k = l = 0;
	 while (i < rangeslen) {
	    m = flip = 0;
	    altern = 32767;
	    if ((numval = ranges[i++]) < 0) {
	       altern = ranges[i++];	/* sub-range iterations */
	       numval = ranges[i++];
	       }
	    if (numval > maxit || i >= rangeslen)
	       numval = maxit;
	    while (l <= numval)  {
	       LogTable[l++] = k + flip;
	       if (++m >= altern) {
		  flip ^= 1;		/* Alternate colors */
		  m = 0;
		  }
	       }
	    ++k;
	    if (altern != 32767) ++k;
	    }
	 }
      else
	 SetupLogTable();

   lm = 4L << bitshift; 		/* CALCMAND magnitude limit */

   /* ORBIT stuff */
   save_orbit = (int far *)((double huge *)dx0 + 4*MAXPIXELS);
   show_orbit = 0;
   orbit_ptr = 0;
   orbit_color = 15;
   if(colors < 16)
      orbit_color = 1;

   if(inversion[0] != 0.0)
   {
      f_radius	  = inversion[0];
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

   closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
   closenuff /= fudge;
   rqlim_save = rqlim;
   rqlim2 = sqrt(rqlim);
   if (integerfractal)		/* for integer routines (lambda) */
   {
      lparm.x = parm.x * fudge;    /* real portion of Lambda */
      lparm.y = parm.y * fudge;    /* imaginary portion of Lambda */
      lparm2.x = parm2.x * fudge;  /* real portion of Lambda2 */
      lparm2.y = parm2.y * fudge;  /* imaginary portion of Lambda2 */
      llimit = rqlim * fudge;	   /* stop if magnitude exceeds this */
      if (llimit <= 0) llimit = 0x7fffffff; /* klooge for integer math */
      llimit2 = rqlim2 * fudge;    /* stop if magnitude exceeds this */
      lclosenuff = closenuff * fudge;	/* "close enough" value */
      l16triglim = 8L<<16;	   /* domain limit of fast trig functions */
      linitorbit.x = initorbit.x * fudge;
      linitorbit.y = initorbit.y * fudge;
   }
   resuming = (calc_status == 2);
   if (!resuming) /* free resume_info memory if any is hanging around */
   {
      end_resume();
      if (resave_flag) {
	 updatesavename(savename); /* do the pending increment */
	 resave_flag = started_resaves = 0;
	 }
      calctime = 0;
   }

   if (curfractalspecific->calctype != StandardFractal
       && curfractalspecific->calctype != calcmand
       && curfractalspecific->calctype != calcmandfp)
   {
      calctype = curfractalspecific->calctype; /* per_image can override */
      symmetry = curfractalspecific->symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */
      iystart = ixstart = yystart = xxstart = yybegin = 0;
      iystop = yystop = ydots -1;
      ixstop = xxstop = xdots -1;
      calc_status = 1; /* mark as in-progress */
      distest = 0; /* only standard escape time engine supports distest */
      /* per_image routine is run here */
      if (curfractalspecific->per_image())
      { /* not a stand-alone */
	 /* next two lines in case periodicity changed */
	 closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
	 closenuff /= fudge;
	 lclosenuff = closenuff * fudge;	/* "close enough" value */
	 setsymmetry(symmetry,0);
	 timer(0,calctype); /* non-standard fractal engine */
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
      timer(0,(int (*)())perform_worklist);
   calctime += timer_interval;

   if(LogTable)
   {
      farmemfree(LogTable);
      LogTable = NULL;
   }
   if(typespecific_workarea)
   {
      farmemfree(typespecific_workarea);
      typespecific_workarea = NULL;
   }

   EXIT_OVLY;
   return((calc_status == 4) ? 0 : -1);
}

/**************** general escape-time engine routines *********************/

static void perform_worklist()
{
   int i;
   long tmplong; /* this temp must be signed */

   if (potflag && pot16bit)
   {
      stdcalcmode = '1'; /* force 1 pass */
      if (resuming == 0)
	 pot_startdisk();
   }
   if (stdcalcmode == 'b' && (curfractalspecific->flags & NOTRACE))
      stdcalcmode = '1';
   if (stdcalcmode == 'g' && (curfractalspecific->flags & NOGUESS))
      stdcalcmode = '1';

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

   if (distest) /* setup stuff for distance estimator */
   {
      double ftemp,ftemp2;
      dem_delta = sqr(delxx) + sqr(delyy2);
      if ((ftemp = sqr(delyy) + sqr(delxx2)) > dem_delta)
	 dem_delta = ftemp;
      if (distestwidth == 0)
	 distestwidth = 71;
      ftemp = distestwidth;
      dem_delta *= sqr(ftemp)/10000; /* multiply by thickness desired */
      dem_width = ( sqrt( sqr(xxmax-xxmin) + sqr(xx3rd-xxmin) ) * ydots/xdots
	  + sqrt( sqr(yymax-yymin) + sqr(yy3rd-yymin) ) ) / distest;
      ftemp = (rqlim < DEM_BAILOUT) ? DEM_BAILOUT : rqlim;
      ftemp += 3; /* bailout plus just a bit */
      ftemp2 = log(ftemp);
      dem_toobig = sqr(ftemp) * sqr(ftemp2) * 4 / dem_delta;
   }

   while (num_worklist > 0)
   {
      calctype = curfractalspecific->calctype; /* per_image can override */
      symmetry = curfractalspecific->symmetry; /*   calctype & symmetry  */
      plot = putcolor; /* defaults when setsymmetry not called or does nothing */

      /* pull top entry off worklist */
      ixstart = xxstart = worklist[0].xxstart;
      ixstop  = xxstop	= worklist[0].xxstop;
      iystart = yystart = worklist[0].yystart;
      iystop  = yystop	= worklist[0].yystop;
      yybegin  = worklist[0].yybegin;
      workpass = worklist[0].pass;
      worksym  = worklist[0].sym;
      --num_worklist;
      for (i=0; i<num_worklist; ++i)
	 worklist[i] = worklist[i+1];

      calc_status = 1; /* mark as in-progress */

      curfractalspecific->per_image();

      /* some common initialization for escape-time pixel level routines */
      closenuff = delmin >> abs(periodicitycheck); /* for periodicity checking */
      closenuff /= fudge;
      lclosenuff = closenuff * fudge;	/* "close enough" value */
      kbdcount=max_kbdcount;
      /* savedmask is for calcmand's periodicity checking */
      savedmask = 0xC0000000; /* top 2 bits on */
      tmplong = (delmin >> abs(periodicitycheck)) | 1;
      while (tmplong > 0) /* while top bit not on */
      {
	 tmplong <<= 1;
	 savedmask = (savedmask >> 1) | 0x80000000;
      }

      setsymmetry(symmetry,1);

      /* call the appropriate escape-time engine */
      switch (stdcalcmode)
      {
      case 't':
	 tesseral();
	 break;
      case 'b':
	 bound_trace_main();
	 break;
      case 'g':
	 solidguess();
	 break;
      default:
	 OneOrTwoPass();
      }

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

static int OneOrTwoPass()
{
   int i;
   totpasses = 1;
   if (stdcalcmode == '2') totpasses = 2;
   if (stdcalcmode == '2' && workpass == 0) /* do 1st pass of two */
   {
      if (StandardCalc(1) == -1)
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
   /* second or only pass */
   if (StandardCalc(2) == -1)
   {
      i = yystop;
      if (iystop != yystop) /* must be due to symmetry */
	 i -= row - iystart;
      add_worklist(xxstart,xxstop,row,i,row,workpass,worksym);
      return(-1);
   }
   return(0);
}

static int _fastcall StandardCalc(int passnum)
{
   got_status = 0;
   curpass = passnum;
   row = yybegin;
   while (row <= iystop)
   {
      currow = row;
      reset_periodicity = 1;
      col = ixstart;
      while (col <= ixstop)
      {
         if(showdot>0)
            (*plot) (col, row, showdot&(colors-1));

	 /* on 2nd pass of two, skip even pts */
	 if (passnum == 1 || stdcalcmode == '1' || (row&1) != 0 || (col&1) != 0)
	 {
	    if ((*calctype)() == -1) /* StandardFractal(), calcmand() or calcmandfp() */
	       return(-1); /* interrupted */
	    reset_periodicity = 0;
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

int calcmand()		/* fast per pixel 1/2/b/g, called with row & col set */
{
   /* setup values from far array to avoid using es reg in calcmand.asm */
   linitx = lx0[col] + lx1[row];
   linity = ly0[row] + ly1[col];
   if (calcmandasm() >= 0)
   {
      if (LogTable /* map color, but not if maxit & adjusted for inside,etc */
      && (realcolor < maxit || (inside < 0 && color == maxit)))
	 color = LogTable[color];
      if (color >= colors) /* don't use color 0 unless from inside/outside */
	 if (colors < 16)
	    color &= andcolor;
	 else
	    color = ((color - 1) % andcolor) + 1;  /* skip color zero */
      (*plot) (col, row, color);
   }
   return (color);
}

/************************************************************************/
/* added by Wes Loewer - sort of a floating point version of calcmand() */
/* can also handle invert, any rqlim, potflag, zmag, epsilon cross,     */
/* and all the current outside options    -Wes Loewer 11/03/91          */
/************************************************************************/
int calcmandfp()
{
   if(invert)
      invertz2(&init);
   else
   {
      init.y = dy0[row]+dy1[col];
      init.x = dx0[col]+dx1[row];
   }
   if (calcmandfpasm() >= 0)
   {
      if (potflag)
	 color = potential(magnitude, realcolor);
	 if (LogTable /* map color, but not if maxit & adjusted for inside,etc */
		  && (realcolor < maxit || (inside < 0 && color == maxit)))
	    color = LogTable[color];
	 if (color >= colors) /* don't use color 0 unless from inside/outside */
	    if (colors < 16)
	       color &= andcolor;
	    else
	       color = ((color - 1) % andcolor) + 1;  /* skip color zero */
      (*plot) (col, row, color);
   }
   return (color);
}

#define green 2
#define yellow 6
int StandardFractal()	/* per pixel 1/2/b/g, called with row & col set */
{
   double tantable[16];
   int hooper;
   double close;
   long lclose;
   int caught_a_cycle;
   int savedand, savedincr;	/* for periodicity checking */
   struct lcomplex lsaved;
   int i, attracted;
   struct lcomplex lat;
   struct complex  at;
   struct complex deriv;
   int dem_color;
   struct complex dem_new;
   close = .01;
   lclose = close*fudge;
   if(inside == -101)
   {
      int i;
      for(i=0;i<16;i++)
	 tantable[i] = 0.0;
   }
   else if(inside == -100)
   {
      close = .01;
      lclose = close*fudge;
   }
   if (periodicitycheck == 0 || inside == -59 || inside == -101)
      oldcolor = 32767; 	/* don't check periodicity at all */
   else if (reset_periodicity)
      oldcolor = 250;		/* don't check periodicity 1st 250 iterations */

   /* really fractal specific, but we'll leave it here */
   if (!integerfractal)
   {
      if (useinitorbit == 1)
	 saved = initorbit;
      else {
	 saved.x = 0;
	 saved.y = 0;
      }
      init.y = dy0[row] + dy1[col];
      if (distest)
      {
	 rqlim = rqlim_save;		  /* start with regular bailout */
	 if (distest != 1 || colors == 2) /* not doing regular outside colors */
	    if (rqlim < DEM_BAILOUT)	  /* so go straight for dem bailout */
	       rqlim = DEM_BAILOUT;
	 deriv.x = 1;
	 deriv.y = 0;
	 magnitude = 0;
	 dem_color = -1;
      }
   }
   else
   {
      if (useinitorbit == 1)
	 lsaved = linitorbit;
      else {
	 lsaved.x = 0;
	 lsaved.y = 0;
      }
      linit.y = ly0[row] + ly1[col];
   }
   orbit_ptr = 0;
   color = 0;
   if(fractype==JULIAFP || fractype==JULIA)
      color = -1;
   caught_a_cycle = 0;
   savedand = 1;		/* begin checking every other cycle */
   savedincr = 1;		/* start checking the very first time */

   if (inside <= -60 && inside >= -61)
   {
      magnitude = lmagnitud = 0;
      min_orbit = 100000.0;
   }
   overflow = 0;		/* reset integer math overflow flag */

   curfractalspecific->per_pixel(); /* initialize the calculations */

   attracted = FALSE;
   while (++color < maxit)
   {
      if(showdot>0)
         (*plot) (col, row, showdot&(colors-1));

      /* calculation of one orbit goes here */
      /* input in "old" -- output in "new" */

      if (distest)
      {
	 double ftemp;
	 /* Distance estimator for points near Mandelbrot set */
	 /* Original code by Phil Wilson, hacked around by PB */
	 /* Algorithms from Peitgen & Saupe, Science of Fractal Images, p.198 */
	 ftemp	 = 2 * (old.x * deriv.x - old.y * deriv.y) + 1;
	 deriv.y = 2 * (old.y * deriv.x + old.x * deriv.y);
	 deriv.x = ftemp;
	 if (sqr(deriv.x)+sqr(deriv.y) > dem_toobig)
	    break;
	 /* if above exit taken, the later test vs dem_delta will place this
		    point on the boundary, because mag(old)<bailout just now */
	 if (curfractalspecific->orbitcalc())
	 {
	    if (dem_color < 0)	      /* note "regular" color for later */
	    {
	       dem_color = color;
	       dem_new = new;
	    }
	    if (rqlim >= DEM_BAILOUT  /* exit if past real bailout */
	    || magnitude >= (rqlim = DEM_BAILOUT) /* reset to real bailout */
		|| magnitude == 0)	 /* exit if type doesn't "floatbailout" */
	       break;
	    old = new;		      /* carry on till past real bailout */
	 }
      }
      else /* the usual case */
	 if (curfractalspecific->orbitcalc() && inside != -101)
	    break;
      if (show_orbit)
	 if (!integerfractal)
	    plot_orbit(new.x, new.y, -1);
	 else
	    iplot_orbit(lnew.x, lnew.y, -1);
      if(inside == -101)
      {
	 if(0 < color && color < 16)
	 {
	 if (integerfractal)
	 {
	       new.x = lnew.x;
	       new.x /= fudge;
	       new.y = lnew.y;
	       new.y /= fudge;
	    }
	    tantable[color-1] = new.y/(new.x+.000001);
	 }
      }
      else if(inside == -100)
      {
	 hooper = 0;
	 if(integerfractal)
	 {
	    if(labs(lnew.x) < lclose)
	    {
	       hooper = 1; /* close to y axis */
	       goto plot_inside;
	    }
	    else if(labs(lnew.y) < lclose)
	    {
	       hooper = 2; /* close to x axis */
	       goto plot_inside;
	    }
	 }
	 else
	 {
	    if(fabs(new.x) < close)
	    {
	       hooper = 1; /* close to y axis */
	       goto plot_inside;
	    }
	    else if(fabs(new.y) < close)
	    {
	       hooper = 2; /* close to x axis */
	       goto plot_inside;
	    }
	 }
      }
      else if (inside <= -60 && inside >= -61)
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


      if (attractors > 0)	/* finite attractor in the list   */
      { 			/* NOTE: Integer code is UNTESTED */
	 if (integerfractal)
	 {
	    for (i = 0; i < attractors; i++)
	    {
		lat.x = lnew.x - lattr[i].x;
		lat.x = lsqr(lat.x);
		if (lat.x < l_at_rad)
		{
		   lat.y = lnew.y - lattr[i].y;
		   lat.y = lsqr(lat.y);
		   if (lat.y < l_at_rad)
		   {
		      if ((lat.x + lat.y) < l_at_rad)
		      {
			 attracted = TRUE;
			 break;
		      }
		   }
		}
	    }
	 }
	 else
	 {
	    for (i = 0; i < attractors; i++)
	    {
		at.x = new.x - attr[i].x;
		at.x = sqr(at.x);
		if (at.x < f_at_rad)
		{
		   at.y = new.y - attr[i].y;
		   at.y = sqr(at.y);
		   if ( at.y < f_at_rad)
		   {
		      if ((at.x + at.y) < f_at_rad)
		      {
			 attracted = TRUE;
			 break;
		      }
		   }
		}
	    }
	 }
	 if (attracted)
	    break;		/* AHA! Eaten by an attractor */
      }

      if (color > oldcolor)	/* check periodicity */
      {
	 if ((color & savedand) == 0)	     /* time to save a new value */
	 {
	    if (!integerfractal)
	       saved = new;  /* floating pt fractals */
	    else
	       lsaved = lnew;/* integer fractals */
	    if (--savedincr == 0)    /* time to lengthen the periodicity? */
	    {
	       savedand = (savedand << 1) + 1;	     /* longer periodicity */
	       savedincr = 4;/* restart counter */
	    }
	 }
	 else		     /* check against an old save */
	 {
	    if (!integerfractal)     /* floating-pt periodicity chk */
	    {
	       if (fabs(saved.x - new.x) < closenuff)
		  if (fabs(saved.y - new.y) < closenuff)
		  {
		     caught_a_cycle = 1;
		     color = maxit - 1;
		  }
	    }
	    else	     /* integer periodicity check */
	    {
	       if (labs(lsaved.x - lnew.x) < lclosenuff)
		  if (labs(lsaved.y - lnew.y) < lclosenuff)
		  {
		     caught_a_cycle = 1;
		     color = maxit - 1;
		  }
	    }
	 }
      }
   }

   if (show_orbit)
      scrub_orbit();

   realcolor = color;		/* save this before we start adjusting it */
   if (color >= maxit)
      oldcolor = 0;		/* check periodicity immediately next time */
   else
   {
      oldcolor = color + 10;	/* check when past this + 10 next time */
      if (color == 0)
	 color = 1;		/* needed to make same as calcmand */
   }

   if (potflag)
   {
      if (integerfractal)	/* adjust integer fractals */
      {
	 new.x = ((double)lnew.x) / fudge;
	 new.y = ((double)lnew.y) / fudge;
      }
      magnitude = sqr(new.x) + sqr(new.y);
      color = potential(magnitude, color);
      if (LogTable)
	 color = LogTable[color];
      goto plot_pixel;		/* skip any other adjustments */
   }

   if (color >= maxit)		/* an "inside" point */
      goto plot_inside; 	/* distest, decomp, biomorph don't apply */


   if (outside < -1)  /* these options by Richard Hughes modified by TW */
   {
      if (integerfractal)
      {
	 new.x = ((double)lnew.x) / fudge;
	 new.y = ((double)lnew.y) / fudge;
      }
      /* Add 7 to overcome negative values on the MANDEL    */
      if (outside == -2)		 /* "real" */
	 color += new.x + 7;
      else if (outside == -3)		 /* "imag" */
	 color += new.y + 7;
      else if (outside == -4  && new.y)  /* "mult" */
	  color *= (new.x/new.y);
      else if (outside == -5)		 /* "sum" */
	  color += new.x + new.y;

      /* eliminate negative colors & wrap arounds */
      if (color < 0 || color > maxit)
	  color = 0;
   }

   if (distest)
   {
      double dist,temp;
      dist = sqr(new.x) + sqr(new.y);
      temp = log(dist);
      dist = dist * sqr(temp) / ( sqr(deriv.x) + sqr(deriv.y) );
      if (dist < dem_delta)	/* point is on the edge */
      {
	 if (distest > 0)
	    goto plot_inside;	/* show it as an inside point */
	 color = 0 - distest;	/* show boundary as specified color */
	 goto plot_pixel;	/* no further adjustments apply */
      }
      if (colors == 2)
      {
	 color = !inside;	/* the only useful distest 2 color use */
	 goto plot_pixel;	/* no further adjustments apply */
      }
      if (distest > 1)		/* pick color based on distance */
      {
	 color = sqrt(dist / dem_width + 1);
	 goto plot_pixel;	/* no further adjustments apply */
      }
      color = dem_color;	/* use pixel's "regular" color */
      new = dem_new;
   }

   if (decomp[0] > 0)
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

   if (outside >= 0 && attracted == FALSE) /* merge escape-time stripes */
      color = outside;
   else if (LogTable)
      color = LogTable[color];
   goto plot_pixel;

   plot_inside: /* we're "inside" */
   if (periodicitycheck < 0 && caught_a_cycle)
      color = 7;	       /* show periodicity */
   else if (inside >= 0)
      color = inside;	       /* set to specified color, ignore logpal */
   else
   {
      if(inside == -101)
      {
	 int i;
	 double diff;
	 color = 0;
	 for(i=1;i<16;i++)
	 {
	    diff = tantable[0] - tantable[i];
	    if(fabs(diff) < .05)
	    {
	       color = i;
	       break;
	    }
	 }
      }
      else if(inside == -100)
      {
	 if(hooper==1)
	    color = green;
	 else if(hooper==2)
	    color = yellow;
	 else
	    color = maxit;
	 if (show_orbit)
	    scrub_orbit();
      }
      else if (inside == -60)
	 color = sqrt(min_orbit) * 75;
      else if (inside == -61)
	 color = min_index;
      else if (inside == -59)
      {
	 if (integerfractal)
	 {
	    /*
	    new.x = ((double)lnew.x) / fudge;
	    new.y = ((double)lnew.y) / fudge;
	    */
	    color = (((double)lsqr(lnew.x))/fudge + ((double)lsqr(lnew.y))/fudge) * (maxit>>1) + 1;
	 }
	 else
	    color = (sqr(new.x) + sqr(new.y)) * (maxit>>1) + 1;
      }
      else /* inside == -1 */
	 color = maxit;
      if (LogTable)
	 color = LogTable[color];
   }

   plot_pixel:

   if (color >= colors) /* don't use color 0 unless from inside/outside */
      if (colors < 16)
	 color &= andcolor;
      else
	 color = ((color - 1) % andcolor) + 1;	/* skip color zero */

   (*plot) (col, row, color);

   if ((kbdcount -= realcolor) <= 0)
   {
      if (check_key())
	 return (-1);
      kbdcount = max_kbdcount;
   }

   return (color);
}
#undef green
#undef yellow



/**************** standardfractal doodad subroutines *********************/

static void decomposition()
{
   static double cos45	   = 0.70710678118654750; /* cos 45	degrees */
   static double sin45	   = 0.70710678118654750; /* sin 45	degrees */
   static double cos22_5   = 0.92387953251128670; /* cos 22.5	degrees */
   static double sin22_5   = 0.38268343236508980; /* sin 22.5	degrees */
   static double cos11_25  = 0.98078528040323040; /* cos 11.25	degrees */
   static double sin11_25  = 0.19509032201612820; /* sin 11.25	degrees */
   static double cos5_625  = 0.99518472667219690; /* cos 5.625	degrees */
   static double sin5_625  = 0.09801714032956060; /* sin 5.625	degrees */
   static double tan22_5   = 0.41421356237309500; /* tan 22.5	degrees */
   static double tan11_25  = 0.19891236737965800; /* tan 11.25	degrees */
   static double tan5_625  = 0.09849140335716425; /* tan 5.625	degrees */
   static double tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
   static double tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
   static long lcos45	  ; /* cos 45	  degrees */
   static long lsin45	  ; /* sin 45	  degrees */
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
   static reset_fudge = -1;
   int temp = 0;
   int i;
   struct lcomplex lalt;
   struct complex alt;
   color = 0;
   if (integerfractal) /* the only case */
   {
      if (reset_fudge != fudge)
      {
	 reset_fudge = fudge;
	 lcos45     = cos45	 *fudge;
	 lsin45     = sin45	 *fudge;
	 lcos22_5   = cos22_5	 *fudge;
	 lsin22_5   = sin22_5	 *fudge;
	 lcos11_25  = cos11_25	 *fudge;
	 lsin11_25  = sin11_25	 *fudge;
	 lcos5_625  = cos5_625	 *fudge;
	 lsin5_625  = sin5_625	 *fudge;
	 ltan22_5   = tan22_5	 *fudge;
	 ltan11_25  = tan11_25	 *fudge;
	 ltan5_625  = tan5_625	 *fudge;
	 ltan2_8125 = tan2_8125  *fudge;
	 ltan1_4063 = tan1_4063  *fudge;
      }
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

/******************************************************************/
/* Continuous potential calculation for Mandelbrot and Julia	  */
/* Reference: Science of Fractal Images p. 190. 		  */
/* Special thanks to Mark Peterson for his "MtMand" program that  */
/* beautifully approximates plate 25 (same reference) and spurred */
/* on the inclusion of similar capabilities in FRACTINT.	  */
/*								  */
/* The purpose of this function is to calculate a color value	  */
/* for a fractal that varies continuously with the screen pixels  */
/* locations for better rendering in 3D.			  */
/*								  */
/* Here "magnitude" is the modulus of the orbit value at          */
/* "iterations". The potparms[] are user-entered paramters        */
/* controlling the level and slope of the continuous potential	  */
/* surface. Returns color.  - Tim Wegner 6/25/89		  */
/*								  */
/*		       -- Change history --			  */
/*								  */
/* 09/12/89   - added floatflag support and fixed float underflow */
/*								  */
/******************************************************************/

static int _fastcall potential(double mag, int iterations)
{
   float f_mag,f_tmp,pot;
   double d_tmp;
   int i_pot;
   long l_pot;
   extern char floatflag;

   if(iterations < maxit)
   {
      pot = i_pot = iterations+2;
      if(i_pot <= 0 || mag <= 1.0)
	 pot = 0.0;
      else /* pot = log(mag) / pow(2.0, (double)pot); */
      {
	 if(i_pot < 120 && !floatflag) /* empirically determined limit of fShift */
	 {
	    f_mag = mag;
	    fLog14(f_mag,f_tmp); /* this SHOULD be non-negative */
	    fShift(f_tmp,(char)-i_pot,pot);
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
      if(pot < 1.0)
	 pot = 1.0; /* avoid color 0 */
   }
   else if(inside >= 0)
      pot = inside;
   else /* inside < 0 implies inside=maxit, so use 1st pot param instead */
      pot = potparam[0];

   i_pot = (l_pot = pot * 256) >> 8;
   if(i_pot >= colors)
   {
      i_pot = colors - 1;
      l_pot = 255;
   }

   if(pot16bit)
   {
      if (dotmode != 11) /* if putcolor won't be doing it for us */
	 writedisk(col+sxoffs,row+syoffs,i_pot);
      writedisk(col+sxoffs,row+sydots+syoffs,(int)l_pot);
   }

   return(i_pot);
}


/**************** boundary tracing method *********************/

static int far *LeftX  = (int far *)NULL;
static int far *RightX = (int far *)NULL;
static unsigned repeats;

static int _fastcall calc_xy(int mx, int my) /* return the color for a pixel */
{

   color = getcolor(mx,my); /* see if pixel is black */
   if (color!=0)	    /* pixel is NOT black so we must have already */
   {			  /* calculated its color, so lets skip it	*/
      repeats++;	    /* count successive easy ones */
      return(color);
   }
   repeats = 0; 	/* we'll have to work for this one wo reset counter */

   col = mx;
   row = my;
   color=(*calctype)();
   return(color);
} /* calc_xy function of BTM code */

static int _fastcall boundary_trace(int C, int R)   /* BTM main function */
{
   enum
       {
      North, East, South, West
   }
   Dir;
   int C_first, bcolor, low_row, iters;
   low_row = R;
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
	 kbdcount=max_kbdcount;
      }
      iters++;		/* count times thru loop */

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
	    kbdcount=max_kbdcount;
	 }
	 if(fillcolor > -1)
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

static int _fastcall fillseg1(int LeftX, int RightX, int R,  int bcolor)
{
   int blockcolor;
   int	gcolor;
   register modeON, C;
   if(fillcolor >0)
     blockcolor = fillcolor&(colors-1);
   else
     blockcolor = 1;  
   modeON = 0;
   for (C = LeftX; C <= RightX; C++)
   {
      gcolor=getcolor(C,R);
      if (modeON!=0 && gcolor==0)
	 (*plot)(C,R,blockcolor); /* show boundary by only filling with color 1 */
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

static int _fastcall fillseg(int LeftX, int RightX, int R,  int bcolor)
{
   unsigned char *forwards;
   unsigned char *backwards;
   register modeON, C;
   int	gcolor,i;
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
      put_line(R,   LeftX,		    RightX,		   forwards);
      if ((i=yystop-(R-yystart)) > iystop)
	 put_line(i,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
   }
   else if(plot==symplot2Y) /* Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,LeftX, 		 RightX,		forwards);
      put_line(R,xxstop-(RightX-ixstart),xxstop-(LeftX-ixstart),backwards);
   }
   else if(plot==symplot4) /* X-axis and Y-axis symmetry */
   {
      reverse_string(backwards,forwards,RightX-LeftX+1);
      put_line(R,LeftX, 		    RightX,		   forwards);
      put_line(R,xxstop-(RightX-ixstart),   xxstop-(LeftX-ixstart),backwards);
      if ((i=yystop-(R-yystart)) > iystop)
      {
	 put_line(i,LeftX,		    RightX,		   forwards);
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
static void _fastcall reverse_string(char *t, char *s, int len)
{
   register i;
   len--;
   for(i=0;i<=len;i++)
      t[i] = s[len-i];
}

static int bound_trace_main()
{
   long maxrow;
   maxrow = ((long)ydots)*sizeof(int);

   if (inside == 0 || outside == 0) {
      static char far msg[]=
	  {"Boundary tracing cannot be used with inside=0 or outside=0."};
      stopmsg(0,msg);
      return(-1);
      }
   if (colors < 16) {
      static char far msg[]=
	  {"Boundary tracing cannot be used with < 16 colors."};
      stopmsg(0,msg);
      return(-1);
      }

   if((LeftX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
      return(-1);
   else if((RightX  = (int far *)farmemalloc(maxrow))==(int far *)NULL)
   {
      farmemfree((char far *)LeftX);
      return(-1);
   }

   for (currow = 0; currow < ydots; currow++)
   {
      LeftX[currow] = 3000;
      RightX[currow] = -3000;
   }

   got_status = 2;
   for (currow = iystart; currow <= iystop; currow++)
   {
      for (curcol = ixstart; curcol <= ixstop; curcol++)
      {
	 if(--kbdcount<=0)
	 {
	    if(check_key())
	    {
	       if (iystop != yystop)
		  iystop = yystop - (currow - yystart); /* allow for sym */
	       add_worklist(xxstart,xxstop,currow,iystop,currow,0,worksym);
	       farmemfree((char far *)LeftX);
	       farmemfree((char far *)RightX);
	       return(-1);
	    }
	    kbdcount=max_kbdcount;
	 }

	 /* BTM Hook! */
	 color = getcolor(curcol,currow);
	 /* if pixel is BLACK (0) then we haven't done it yet!
	    so first calculate its color and call the routine
	    that will try and trace a polygon if one exists */
	 if (color==0)
	 {
	    reset_periodicity = 1;
	    row = currow;
	    col = curcol;
	    color=(*calctype)();
	    reset_periodicity = 0;
	    boundary_trace(curcol, currow); /* go trace boundary! WHOOOOOOO! */
	 }
      }
   }
   farmemfree((char far *)LeftX);
   farmemfree((char far *)RightX);
   return(0);
} /* end of bound_trace_main */


/************************ super solid guessing *****************************/

/*
   I, Timothy Wegner, invented this solidguessing idea and implemented it in
   more or less the overall framework you see here.  I am adding this note
   now in a possibly vain attempt to secure my place in history, because
   Pieter Branderhorst has totally rewritten this routine, incorporating
   a *MUCH* more sophisticated algorithm.  His revised code is not only
   faster, but is also more accurate. Harrumph!
*/

static int solidguess()
{
   int i,x,y,xlim,ylim,blocksize;
   unsigned int *pfxp0,*pfxp1;
   unsigned int u;

   guessplot=(plot!=putcolor && plot!=symplot2 && plot!=symplot2J);
   /* check if guessing at bottom & right edges is ok */
   bottom_guess = (plot == symplot2 || (plot == putcolor && iystop+1 == ydots));
   right_guess	= (plot == symplot2J
       || ((plot == putcolor || plot == symplot2) && ixstop+1 == xdots));

   i = maxblock = blocksize = ssg_blocksize();
   totpasses = 1;
   while ((i >>= 1) > 1) ++totpasses;

   /* ensure window top and left are on required boundary, treat window
	 as larger than it really is if necessary (this is the reason symplot
	 routines must check for > xdots/ydots before plotting sym points) */
   ixstart &= -1 - (maxblock-1);
   iystart = yybegin;
   iystart &= -1 - (maxblock-1);

   got_status = 1;

   if (workpass == 0) /* otherwise first pass already done */
   {
      /* first pass, calc every blocksize**2 pixel, quarter result & paint it */
      curpass = 1;
      if (iystart <= yystart) /* first time for this window, init it */
      {
	 currow = 0;
	 memset(&prefix[1][0][0],0,maxxblk*maxyblk*2); /* noskip flags off */
	 reset_periodicity = 1;
	 row=iystart;
	 for(col=ixstart; col<=ixstop; col+=maxblock)
	 { /* calc top row */
	    if((*calctype)()==-1)
	    {
	       add_worklist(xxstart,xxstop,yystart,yystop,yybegin,0,worksym);
	       goto exit_solidguess;
	    }
	    reset_periodicity = 0;
	 }
      }
      else
	 memset(&prefix[1][0][0],-1,maxxblk*maxyblk*2); /* noskip flags on */
      for(y=iystart; y<=iystop; y+=blocksize)
      {
	 currow = y;
	 i = 0;
	 if(y+blocksize<=iystop)
	 { /* calc the row below */
	    row=y+blocksize;
	    reset_periodicity = 1;
	    for(col=ixstart; col<=ixstop; col+=maxblock)
	    {
	       if((i=(*calctype)()) == -1)
		  break;
	       reset_periodicity = 0;
	    }
	 }
	 reset_periodicity = 1;
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
   reset_periodicity = 1;
   while((blocksize=blocksize>>1)>=2)
   {
      curpass = workpass + 1;
      for(y=iystart; y<=iystop; y+=blocksize)
      {
	 currow = y;
	 if(guessrow(0,y,blocksize)!=0)
	 {
	    if (y < yystart)
	       y = yystart;
	    add_worklist(xxstart,xxstop,yystart,yystop,y,workpass,worksym);
	    goto exit_solidguess;
	 }
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

#define calcadot(c,x,y) { col=x; row=y; if((c=(*calctype)())==-1) return -1; }

static int _fastcall guessrow(int firstpass,int y,int blocksize)
{
   int x,i,j,color;
   int xplushalf,xplusblock;
   int ylessblock,ylesshalf,yplushalf,yplusblock;
   int	   c21,c31,c41; 	/* cxy is the color of pixel at (x,y) */
   int c12,c22,c32,c42; 	/* where c22 is the topleft corner of */
   int c13,c23,c33;		/* the block being handled in current */
   int	   c24,    c44; 	/* iteration			      */
   int guessed23,guessed32,guessed33,guessed12,guessed13;
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
	 if(firstpass)	/* display guessed corners, fill in block */
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
      if(keypressed()) return -1;
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
	 if(keypressed()) return -1;
      }
   }
   return 0;
}

static void _fastcall plotblock(int buildrow,int x,int y,int color)
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
	 return; /* the usual case */
   }
   /* paint it */
   if((ylim=y+halfblock)>iystop)
   {
      if(y>iystop)
	 return;
      ylim=iystop+1;
   }
   for(i=x;++i<xlim;)
      (*plot)(i,y,color); /* skip 1st dot on 1st row */
   while(++y<ylim)
      for(i=x;i<xlim;++i)
	 (*plot)(i,y,color);
}


/************************* symmetry plot setup ************************/

static int _fastcall xsym_split(int xaxis_row,int xaxis_between)
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

static int _fastcall ysym_split(int yaxis_col,int yaxis_between)
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

static void _fastcall setsymmetry(int sym, int uselist) /* set up proper symmetrical plot functions */
{
   extern int forcesymmetry;
   int i;
   int parmszero;
   int xaxis_row, yaxis_col;	     /* pixel number for origin */
   int xaxis_between, yaxis_between; /* if axis between 2 pixels, not on one */
   double ftemp;
   symmetry = 1;
   TranspSymmetry = sym;	     /* for tp3d.c, MCP 6-1-90 */
   if(sym == NOPLOT && forcesymmetry == 999)
   {
      plot = noplot;
      return;
   }
   /* NOTE: 16-bit potential disables symmetry */
   /* also any decomp= option and any inversion not about the origin */
   /* also any rotation other than 180deg and any off-axis stretch */
   if ((potflag && pot16bit) || (invert && inversion[2] != 0.0)
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
   switch(sym)	     /* symmetry switch */
   {
   case XAXIS_NOREAL:	 /* X-axis Symmetry (no real param) */
      if (parm.x != 0.0) break;
      goto xsym;
   case XAXIS_NOIMAG:	 /* X-axis Symmetry (no imag param) */
      if (parm.y != 0.0) break;
      goto xsym;
   case XAXIS_NOPARM:			     /* X-axis Symmetry  (no params)*/
      if (!parmszero)
	 break;
      xsym:
   case XAXIS:			     /* X-axis Symmetry */
      if (xsym_split(xaxis_row,xaxis_between) == 0)
	 if(basin)
	    plot = symplot2basin;
	 else
	    plot = symplot2;
      break;
   case YAXIS_NOPARM:			     /* Y-axis Symmetry (No Parms)*/
      if (!parmszero)
	 break;
   case YAXIS:			     /* Y-axis Symmetry */
      if (ysym_split(yaxis_col,yaxis_between) == 0)
	 plot = symplot2Y;
      break;
   case XYAXIS_NOPARM:			     /* X-axis AND Y-axis Symmetry (no parms)*/
      if(!parmszero)
	 break;
   case XYAXIS: 		     /* X-axis AND Y-axis Symmetry */
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
   case ORIGIN_NOPARM:			     /* Origin Symmetry (no parms)*/
      if (!parmszero)
	 break;
   case ORIGIN: 		     /* Origin Symmetry */
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
   case PI_SYM: 		     /* PI symmetry */
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
   default:		     /* no symmetry */
      break;
   }
}


/***************** standalone engine for "test" ********************/

int test()
{
   int startrow,startpass,numpasses;
   startrow = startpass = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&startrow,sizeof(int),&startpass,0);
      end_resume();
   }
   if(teststart()) /* assume it was stand-alone, doesn't want passes logic */
      return(0);
   numpasses = (stdcalcmode == '1') ? 0 : 1;
   for (passes=startpass; passes <= numpasses ; passes++)
   {
      for (row = startrow; row <= iystop; row=row+1+numpasses)
      {
	 register int col;
	 for (col = 0; col <= ixstop; col++)	   /* look at each point on screen */
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


/***************** standalone engine for "plasma" ********************/

static int iparmx;				/* iparmx = parm.x * 16 */
static int shiftvalue;				/* shift based on #colors */
extern int max_colors;
static int recur1=1;
static int pcolors;

typedef struct palett
{
   unsigned char red;
   unsigned char green;
   unsigned char blue;
}
Palettetype;

extern Palettetype dacbox[256];
static int plasma_check;			/* to limit kbd checking */

static int _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   long pseudorandom;
   pseudorandom = ((long)iparmx)*((rand()-16383));
   pseudorandom = pseudorandom*(abs(xa-xb)+abs(ya-yb));
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = ((getcolor(xa,ya)+getcolor(xb,yb)+1)>>1)+pseudorandom;
   if (pseudorandom <	1) pseudorandom =   1;
   if (pseudorandom >= pcolors) pseudorandom = pcolors-1;
   putcolor(x,y,(int)pseudorandom);
   return((int)pseudorandom);
}


static int _fastcall new_adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   long pseudorandom;
   pseudorandom = ((long)iparmx)*((rand()-16383));
   pseudorandom = pseudorandom * recur1;
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = ((getcolor(xa,ya)+getcolor(xb,yb)+1)>>1)+pseudorandom;
   if (pseudorandom <	1) pseudorandom =   1;
   if (pseudorandom >= pcolors) pseudorandom = pcolors-1;
   putcolor(x,y,(int)pseudorandom);
   return((int)pseudorandom);
}


static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur)
{

   int x,y;
   int v;
   int nx1;
   int nx;
   int ny1, ny;
   int i;

    struct sub {
	unsigned char t; /* top of stack */
	int v[16]; /* subdivided value */
	unsigned char r[16];  /* recursion level */
    };

    static struct sub subx, suby;

    recur1=1;
    for (i=1;i<=recur;i++)
	recur1 = recur1 * 2;
    recur1=320/recur1;
    suby.t = 2;
    ny	= suby.v[0] = y2;
    ny1 = suby.v[2] = y1;
    suby.r[0] = suby.r[2] = 0;
    suby.r[1] = 1;
    y = suby.v[1] = (ny1 + ny) >> 1;

    while (suby.t >= 1)
    {
	if ((++plasma_check & 0x0f) == 1)
	    if(check_key())
	    {
/*   naah, we don't want to flush this key!!!
		getch();
*/
		plasma_check--;
		return(1);
	    }
	while (suby.r[suby.t-1] < recur)
	{
	    /*	1.  Create new entry at top of the stack  */

	    /*	2.  Copy old top value to new top value.
		    This is largest y value.		  */

	    /*	3.  Smallest y is now old mid point	  */

	    /*	4.  Set new mid point recursion level	  */

	    /*	5.  New mid point value is average
		    of largest and smallest		  */

	    suby.t++;
	    ny1  = suby.v[suby.t] = suby.v[suby.t-1];
	    ny	 = suby.v[suby.t-2];
	    suby.r[suby.t] = suby.r[suby.t-1];
	    y	 = suby.v[suby.t-1]   = (ny1 + ny) >> 1;
	    suby.r[suby.t-1]   = (int)max(suby.r[suby.t],
					    suby.r[suby.t-2])+1;
	}
	subx.t = 2;
	nx  = subx.v[0] = x2;
	nx1 = subx.v[2] = x1;
	subx.r[0] = subx.r[2] = 0;
	subx.r[1] = 1;
	x = subx.v[1] = (nx1 + nx) >> 1;

	while (subx.t >= 1)
	{
	    while (subx.r[subx.t-1] < recur)
	    {
		subx.t++; /* move the top ofthe stack up 1 */
		nx1  = subx.v[subx.t] = subx.v[subx.t-1];
		nx   = subx.v[subx.t-2];
		subx.r[subx.t] = subx.r[subx.t-1];
		x    = subx.v[subx.t-1]   = (nx1 + nx) >> 1;
		subx.r[subx.t-1]   = (int)max(subx.r[subx.t],
						subx.r[subx.t-2])+1;

	}

	   if ((i = getcolor(nx, y)) == 0)
	       i = new_adjust(nx,ny1,nx,y ,nx,ny);
	       v = i;
	   if ((i = getcolor(x, ny)) == 0)
	       i = new_adjust(nx1,ny,x ,ny,nx,ny);
	       v += i;
	   if(getcolor(x,y) == 0)
	   {
	       if ((i = getcolor(x, ny1)) == 0)
		   i = new_adjust(nx1,ny1,x ,ny1,nx,ny1);
		   v += i;
	       if ((i = getcolor(nx1, y)) == 0)
		   i = new_adjust(nx1,ny1,nx1,y ,nx1,ny);
		   v += i;
	       putcolor(x,y,(v + 2) >> 2);
	   }

	   if (subx.r[subx.t-1] == recur) subx.t = subx.t - 2;
	}

	if (suby.r[suby.t-1] == recur) suby.t = suby.t - 2;
    }
    return(0);
}


static void _fastcall subDivide(int x1,int y1,int x2,int y2)
{
   int x,y;
   int v,i;

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
   if((v=getcolor(x,y1)) == 0)
       v=adjust(x1,y1,x ,y1,x2,y1);
   i=v;
   if((v=getcolor(x2,y)) == 0)
       v=adjust(x2,y1,x2,y ,x2,y2);
   i+=v;
   if((v=getcolor(x,y2)) == 0)
       v=adjust(x1,y2,x ,y2,x2,y2);
   i+=v;
   if((v=getcolor(x1,y)) == 0)
       v=adjust(x1,y1,x1,y ,x1,y2);
   i+=v;

   if(getcolor(x,y) == 0)
      putcolor(x,y,(i+2)>>2);

   subDivide(x1,y1,x ,y);
   subDivide(x ,y1,x2,y);
   subDivide(x ,y ,x2,y2);
   subDivide(x1,y ,x ,y2);
}

int plasma()
{
int i,k;

   plasma_check = 0;

   if(colors < 4) {
static char far plasmamsg[]={"\
Plasma Clouds can currently only be run in a 4-or-more-color video\n\
mode (and color-cycled only on VGA adapters [or EGA adapters in their\n\
640x350x16 mode])."};
      stopmsg(0,plasmamsg);
      return(-1);
   }
   iparmx = param[0] * 8;
   if (parm.x <= 0.0) iparmx = 16;
   if (parm.x >= 100) iparmx = 800;

   if ((!rflag) && param[2] == 1)
       --rseed;
   if (param[2] != 0 && param[2] != 1)
      rseed = param[2];

   srand(rseed);
   if (!rflag) ++rseed;

   if (colors == 256)			/* set the (256-color) palette */
      set_Plasma_palette();		/* skip this if < 256 colors */

   if (colors > 16)			/* adjust based on # of colors */
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

   pcolors = min(colors, max_colors);
   putcolor(	  0,	  0,1+(((rand()/pcolors)*(pcolors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,	  0,1+(((rand()/pcolors)*(pcolors-1))>>(shiftvalue-11)));
   putcolor(xdots-1,ydots-1,1+(((rand()/pcolors)*(pcolors-1))>>(shiftvalue-11)));
   putcolor(	  0,ydots-1,1+(((rand()/pcolors)*(pcolors-1))>>(shiftvalue-11)));

   if (param[1] == 0)
   {
      subDivide(0,0,xdots-1,ydots-1);
   }
   else
   {
      recur1 = i = k = 1;
      while(new_subD(0,0,xdots-1,ydots-1,i)==0)
      {
	 k = k * 2;
	 if (k	>(int)max(xdots-1,ydots-1))
	     break;
	 if (check_key())
	     return(1);
	 i++;
      }
   }

   if (! check_key())
      return(0);
   else
      return(1);
}

static void set_Plasma_palette()
{
   extern char far *mapdacbox;
   static Palettetype Red    = {
      63, 0, 0	 };
   static Palettetype Green  = {
      0,63, 0	};
   static Palettetype Blue   = {
      0, 0,63	};
   int i;

   if (mapdacbox) return;		/* map= specified */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
      dacbox[i].red   = (i*Green.red   + (86-i)*Blue.red)/85;
      dacbox[i].green = (i*Green.green + (86-i)*Blue.green)/85;
      dacbox[i].blue  = (i*Green.blue  + (86-i)*Blue.blue)/85;

      dacbox[i+85].red	 = (i*Red.red	+ (86-i)*Green.red)/85;
      dacbox[i+85].green = (i*Red.green + (86-i)*Green.green)/85;
      dacbox[i+85].blue  = (i*Red.blue	+ (86-i)*Green.blue)/85;

      dacbox[i+170].red   = (i*Blue.red   + (86-i)*Red.red)/85;
      dacbox[i+170].green = (i*Blue.green + (86-i)*Red.green)/85;
      dacbox[i+170].blue  = (i*Blue.blue  + (86-i)*Red.blue)/85;
   }
   SetTgaColors();	/* TARGA 3 June 89  j mclain */
   spindac(0,1);
}


/***************** standalone engine for "diffusion" ********************/

#define RANDOM(x)  (rand()%(x))

/* This constant assumes that rand() returns a value from 0 to 32676 */
#define FOURPI	(long)(4*PI*(double)(1L << 16))

int diffusion()
{
   int xmax,ymax,xmin,ymin;	/* Current maximum coordinates */
   int border;	 /* Distance between release point and fractal */
   double cosine,sine,angle;
   long lcosine,lsine;
   int x,y;
   extern char floatflag;

   if (diskvideo)
   {
      notdiskmsg();
      return(-1);
   }

   bitshift = 16;
   fudge = 1L << 16;

   border = param[0];

   if (border <= 0)
      border = 10;

   srand(rseed);
   if (!rflag) ++rseed;

   xmax = xdots / 2 + border;  /* Initial box */
   xmin = xdots / 2 - border;
   ymax = ydots / 2 + border;
   ymin = ydots / 2 - border;

   if (resuming) /* restore worklist, if we can't the above will stay in place */
   {
      start_resume();
      get_resume(sizeof(int),&xmax,sizeof(int),&xmin,sizeof(int),&ymax,
		 sizeof(int),&ymin,0);
      end_resume();
   }

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
      /* on all eight sides					  */
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
	       alloc_resume(20,1);
	       put_resume(sizeof(int),&xmax,sizeof(int),&xmin, sizeof(int),&ymax,
		 sizeof(int),&ymin,0);
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


/************ standalone engine for "bifurcation" types ***************/

/***************************************************************/
/* The following code now forms a generalised Fractal Engine   */
/* for Bifurcation fractal typeS.  By rights it now belongs in */
/* CALCFRACT.C, but it's easier for me to leave it here !      */

/* Original code by Phil Wilson, hacked around by Kev Allen.   */

/* Besides generalisation, enhancements include Periodicity    */
/* Checking during the plotting phase (AND halfway through the */
/* filter cycle, if possible, to halve calc times), quicker    */
/* floating-point calculations for the standard Verhulst type, */
/* and new bifurcation types (integer bifurcation, f.p & int   */
/* biflambda - the real equivalent of complex Lambda sets -    */
/* and f.p renditions of bifurcations of r*sin(Pi*p), which    */
/* spurred Mitchel Feigenbaum on to discover his Number).      */

/* To add further types, extend the fractalspecific[] array in */
/* usual way, with Bifurcation as the engine, and the name of  */
/* the routine that calculates the next bifucation generation  */
/* as the "orbitcalc" routine in the fractalspecific[] entry.  */

/* Bifurcation "orbitcalc" routines get called once per screen */
/* pixel column.  They should calculate the next generation    */
/* from the doubles Rate & Population (or the longs lRate &    */
/* lPopulation if they use integer math), placing the result   */
/* back in Population (or lPopulation).  They should return 0  */
/* if all is ok, or any non-zero value if calculation bailout  */
/* is desirable (eg in case of errors, or the series tending   */
/* to infinity).		Have fun !		       */
/***************************************************************/

#define BIG 100000.0
#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
			       (p.25), but that seems unnecessary. Can
			       override this value with a nonzero param1 */

#define SEED 0.66		/* starting value for population */

static int far *verhulst_array;
static unsigned int filter_cycles;
static unsigned int half_time_check;
static long   lPopulation, lRate;
static double Population,  Rate;
static int    mono, outside_x;
static long   LPI;

int Bifurcation(void)
{
   unsigned long array_size;
   int row, column;
   column = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&column,0);
      end_resume();
   }

   array_size =  (iystop + 2) * sizeof(int);
   if ((verhulst_array = (int far *) farmemalloc(array_size)) == NULL)
   {
      static char far msg[]={"Insufficient free memory for calculation."};
      stopmsg(0,msg);
      return(-1);
   }

   LPI = PI * fudge;

   for (row = 0; row <= iystop+1; row++)
      verhulst_array[row] = 0;

   mono = 0;
   if (colors == 2)
      mono = 1;
   if (mono)
   {
      if (inside)
      {
	 outside_x = 0;
	 inside = 1;
      }
      else
	 outside_x = 1;
   }

   filter_cycles = (parm.x <= 0) ? DEFAULTFILTER : parm.x;
   half_time_check = FALSE;
   if (periodicitycheck && maxit < filter_cycles)
   {
      filter_cycles = (filter_cycles - maxit + 1) / 2;
      half_time_check = TRUE;
   }

   if (integerfractal)
      linit.y = ly0[iystop];   /* Y-value of	*/
   else
      init.y = dy0[iystop];   /* bottom pixels */

   while (column <= ixstop)
   {
      if(check_key())
      {
	 farmemfree((char far *)verhulst_array);
	 alloc_resume(10,1);
	 put_resume(sizeof(int),&column,0);
	 return(-1);
      }
      if (integerfractal)
	 lRate = lx0[column];
      else
	 Rate = dx0[column];
      verhulst();	 /* calculate array once per column */

      for (row = iystop+1; row > 0; row--)
      {
	 int color;
	 color = verhulst_array[row];
	 if(color && mono)
	    color = inside;
	 else if((!color) && mono)
	    color = outside_x;
	 verhulst_array[row] = 0;
	 (*plot)(column,row,color);
      }
      column++;
   }
   farmemfree((char far *)verhulst_array);
   return(0);
}

static void verhulst()		/* P. F. Verhulst (1845) */
{
   unsigned int pixel_row, counter, errors;

    if (integerfractal)
       lPopulation = (parm.y == 0) ? SEED * fudge : parm.y * fudge;
    else
       Population = (parm.y == 0 ) ? SEED : parm.y;

   errors = overflow = FALSE;

   for (counter=0 ; counter < filter_cycles ; counter++)
   {
      errors = (*(curfractalspecific->orbitcalc))();
      if (errors)
	 return;
   }

   if (half_time_check) /* check for periodicity at half-time */
   {
      Bif_Period_Init();
      for (counter=0 ; counter < maxit ; counter++)
      {
	 errors = (*(curfractalspecific->orbitcalc))();
	 if (errors) return;
	 if (periodicitycheck && Bif_Periodic(counter)) break;
      }
      if (counter >= maxit)   /* if not periodic, go the distance */
      {
	 for (counter=0 ; counter < filter_cycles ; counter++)
	 {
	    errors = (*(curfractalspecific->orbitcalc))();
	    if (errors) return;
	 }
      }
   }

   if (periodicitycheck) Bif_Period_Init();

   for (counter=0 ; counter < maxit ; counter++)
   {
      errors = (*(curfractalspecific->orbitcalc))();
      if (errors) return;

      /* assign population value to Y coordinate in pixels */
      if (integerfractal)
	 pixel_row = iystop + 1 - (lPopulation - linit.y) / dely;
      else
	 pixel_row = iystop + 1 - (int)((Population - init.y) / deltaY);

      /* if it's visible on the screen, save it in the column array */
      if (pixel_row <= iystop + 1)
	 verhulst_array[ pixel_row ] ++;

      if (periodicitycheck && Bif_Periodic(counter))
      {
	 if (pixel_row <= iystop + 1)
	    verhulst_array[ pixel_row ] --;
	 break;
      }
   }
}
static	long	lBif_closenuf, lBif_savedpop;	/* poss future use  */
static	double	 Bif_closenuf,	Bif_savedpop;
static	int	 Bif_savedinc,	Bif_savedand;

static void Bif_Period_Init()
{
   Bif_savedinc = 1;
   Bif_savedand = 1;
   if (integerfractal)
   {
      lBif_savedpop = -1;
      lBif_closenuf = dely / 8;
   }
   else
   {
      Bif_savedpop = -1.0;
      Bif_closenuf = deltaY / 8.0;
   }
}

static int _fastcall Bif_Periodic (time)  /* Bifurcation Population Periodicity Check */
int time;		/* Returns : 1 if periodicity found, else 0 */
{
   if ((time & Bif_savedand) == 0)	/* time to save a new value */
   {
      if (integerfractal) lBif_savedpop = lPopulation;
      else		     Bif_savedpop =  Population;
      if (--Bif_savedinc == 0)
      {
	 Bif_savedand = (Bif_savedand << 1) + 1;
	 Bif_savedinc = 4;
      }
   }
   else 			/* check against an old save */
   {
      if (integerfractal)
      {
	 if (labs(lBif_savedpop-lPopulation) <= lBif_closenuf)
	    return(1);
      }
      else
      {
	 if (fabs(Bif_savedpop-Population) <= Bif_closenuf)
	    return(1);
      }
   }
   return(0);
}

/**********************************************************************/
/*												      */
/* The following are Bifurcation "orbitcalc" routines...              */
/*												      */
/**********************************************************************/

int BifurcVerhulst()
  {
    Population += Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }

int LongBifurcVerhulst()
  {
    ltmp.y = lPopulation - multiply(lPopulation,lPopulation,bitshift);
    lPopulation += multiply(lRate,ltmp.y,bitshift);
    return (overflow);
  }

int BifurcLambda()
  {
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }

int LongBifurcLambda()
  {
    ltmp.y = lPopulation - multiply(lPopulation,lPopulation,bitshift);
    lPopulation = multiply(lRate,ltmp.y,bitshift);
    return (overflow);
  }

int BifurcAddSinPi()
  {
    Population += Rate * sin(PI*Population);
    return (fabs(Population) > BIG);
  }

int LongBifurcAddSinPi()
  {
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    SinCos086(ltmp.x,&ltmp.x,&ltmp.y);
    lPopulation += multiply(lRate,ltmp.x,bitshift);
    return (overflow);
  }

int BifurcSetSinPi()
  {
    Population = Rate * sin(PI*Population);
    return (fabs(Population) > BIG);
  }

int LongBifurcSetSinPi()
  {
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    SinCos086(ltmp.x,&ltmp.x,&ltmp.y);
    lPopulation = multiply(lRate,ltmp.x,bitshift);
    return (overflow);
  }

int BifurcStewart()
  {
    Population = (Rate * Population * Population) - 1.0;
    return (fabs(Population) > BIG);
  }

int LongBifurcStewart()
  {
    lPopulation = multiply(lPopulation,lPopulation,bitshift);
    lPopulation = multiply(lPopulation,lRate,	   bitshift);
    lPopulation -= fudge;
    return (overflow);
  }

/* Here Endeth the Generalised Bifurcation Fractal Engine   */

/* END Phil Wilson's Code (modified slightly by Kev Allen et. al. !) */


/******************* standalone engine for "popcorn" ********************/

int popcorn()	/* subset of std engine */
{
   int start_row;
   start_row = 0;
   if (resuming)
   {
      start_resume();
      get_resume(sizeof(int),&start_row,0);
      end_resume();
   }
   kbdcount=max_kbdcount;
   plot = noplot;
   tempsqrx = ltempsqrx = 0; /* PB added this to cover weird BAILOUTs */
   for (row = start_row; row <= iystop; row++)
   {
      reset_periodicity = 1;
      for (col = 0; col <= ixstop; col++)
      {
	 if (StandardFractal() == -1) /* interrupted */
	 {
	    alloc_resume(10,1);
	    put_resume(sizeof(int),&row,0);
	    return(-1);
	 }
	 reset_periodicity = 0;
      }
   }
   calc_status = 4;
   return(0);
}

/**************** tesseral method by CJLT begins here*********************/
/*  reworked by PB for speed and resumeability */

struct tess { /* one of these per box to be done gets stacked */
   int x1,x2,y1,y2;	 /* left/right top/bottom x/y coords  */
   int top,bot,lft,rgt;  /* edge colors, -1 mixed, -2 unknown */
};

static int tesseral()
{
   register struct tess *tp;

   guessplot = (plot != putcolor && plot != symplot2);
   tp = (struct tess *)&dstack[0];
   tp->x1 = ixstart;				  /* set up initial box */
   tp->x2 = ixstop;
   tp->y1 = iystart;
   tp->y2 = iystop;

   if (workpass == 0) { /* not resuming */
      tp->top = tessrow(ixstart,ixstop,iystart);     /* Do top row */
      tp->bot = tessrow(ixstart,ixstop,iystop);      /* Do bottom row */
      tp->lft = tesscol(ixstart,iystart+1,iystop-1); /* Do left column */
      tp->rgt = tesscol(ixstop,iystart+1,iystop-1);  /* Do right column */
      if (check_key()) { /* interrupt before we got properly rolling */
         add_worklist(xxstart,xxstop,yystart,yystop,yystart,0,worksym);
         return(-1);
      }
   }

   else { /* resuming, rebuild work stack */
      int i,mid,curx,cury,xsize,ysize;
      struct tess *tp2;
      tp->top = tp->bot = tp->lft = tp->rgt = -2;
      cury = yybegin & 0xfff;
      ysize = 1; 
      i = (unsigned)yybegin >> 12;
      while (--i >= 0) ysize <<= 1;
      curx = workpass & 0xfff;
      xsize = 1; 
      i = (unsigned)workpass >> 12;
      while (--i >= 0) xsize <<= 1;
      while (1) {
         tp2 = tp;
         if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* next divide down middle */
            if (tp->x1 == curx && (tp->x2 - tp->x1 - 2) < xsize)
               break;
            mid = (tp->x1 + tp->x2) >> 1;		 /* Find mid point */
            if (mid > curx) { /* stack right part */
               memcpy(++tp,tp2,sizeof(*tp));
               tp->x2 = mid;
            }
            tp2->x1 = mid;
         }
         else { 				  /* next divide across */
            if (tp->y1 == cury && (tp->y2 - tp->y1 - 2) < ysize) break;
            mid = (tp->y1 + tp->y2) >> 1;		 /* Find mid point */
            if (mid > cury) { /* stack bottom part */
               memcpy(++tp,tp2,sizeof(*tp));
               tp->y2 = mid;
            }
            tp2->y1 = mid;
         }
      }
   }

   got_status = 4; /* for tab_display */

   while (tp >= (struct tess *)&dstack[0]) { /* do next box */
      curcol = tp->x1; /* for tab_display */
      currow = tp->y1;

      if (tp->top == -1 || tp->bot == -1 || tp->lft == -1 || tp->rgt == -1)
         goto tess_split;
      /* for any edge whose color is unknown, set it */
      if (tp->top == -2)
         tp->top = tesschkrow(tp->x1,tp->x2,tp->y1);
      if (tp->top == -1)
         goto tess_split;
      if (tp->bot == -2)
         tp->bot = tesschkrow(tp->x1,tp->x2,tp->y2);
      if (tp->bot != tp->top)
         goto tess_split;
      if (tp->lft == -2)
         tp->lft = tesschkcol(tp->x1,tp->y1,tp->y2);
      if (tp->lft != tp->top)
         goto tess_split;
      if (tp->rgt == -2)
         tp->rgt = tesschkcol(tp->x2,tp->y1,tp->y2);
      if (tp->rgt != tp->top)
         goto tess_split;

      {  /* all 4 edges are the same color, fill in */
         int i,j;
         i = 0;
         if(fillcolor != 0)
         {
         if(fillcolor > 0)
            tp->top = fillcolor & (colors-1);
         if (guessplot || (j = tp->x2 - tp->x1 - 1) < 2) { /* paint dots */
            for (col = tp->x1 + 1; col < tp->x2; col++)
               for (row = tp->y1 + 1; row < tp->y2; row++) {
                  (*plot)(col,row,tp->top);
                  if (++i > 500) {
                     if (check_key()) goto tess_end;
                     i = 0;
                  }
               }
         }
         else { /* use put_line for speed */
            memset(&dstack[2048],tp->top,j);
            for (row = tp->y1 + 1; row < tp->y2; row++) {
               put_line(row,tp->x1+1,tp->x2-1,&dstack[2048]);
               if (plot != putcolor) /* symmetry */
                  if ((j = yystop-(row-yystart)) > iystop && j < ydots)
                     put_line(j,tp->x1+1,tp->x2-1,&dstack[2048]);
               if (++i > 25) {
                  if (check_key()) goto tess_end;
                  i = 0;
               }
            }
         }
         }
         --tp;
      }
      continue;

      tess_split:
      {  /* box not surrounded by same color, sub-divide */
         int mid,midcolor;
         struct tess *tp2;
         if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* divide down the middle */
            mid = (tp->x1 + tp->x2) >> 1;		 /* Find mid point */
            midcolor = tesscol(mid, tp->y1+1, tp->y2-1); /* Do mid column */
            if (midcolor == -3) goto tess_end;
            if (tp->x2 - mid > 1) {    /* right part >= 1 column */
               if (tp->top == -1) tp->top = -2;
               if (tp->bot == -1) tp->bot = -2;
               tp2 = tp;
               if (mid - tp->x1 > 1) { /* left part >= 1 col, stack right */
                  memcpy(++tp,tp2,sizeof(*tp));
                  tp->x2 = mid;
                  tp->rgt = midcolor;
               }
               tp2->x1 = mid;
               tp2->lft = midcolor;
            }
            else
               --tp;
         }
         else { 				  /* divide across the middle */
            mid = (tp->y1 + tp->y2) >> 1;		 /* Find mid point */
            midcolor = tessrow(tp->x1+1, tp->x2-1, mid); /* Do mid row */
            if (midcolor == -3) goto tess_end;
            if (tp->y2 - mid > 1) {    /* bottom part >= 1 column */
               if (tp->lft == -1) tp->lft = -2;
               if (tp->rgt == -1) tp->rgt = -2;
               tp2 = tp;
               if (mid - tp->y1 > 1) { /* top also >= 1 col, stack bottom */
                  memcpy(++tp,tp2,sizeof(*tp));
                  tp->y2 = mid;
                  tp->bot = midcolor;
               }
               tp2->y1 = mid;
               tp2->top = midcolor;
            }
            else
               --tp;
         }
      }

   }

   tess_end:
   if (tp >= (struct tess *)&dstack[0]) { /* didn't complete */
      int i,xsize,ysize;
      xsize = ysize = 1;
      i = 2;
      while (tp->x2 - tp->x1 - 2 >= i) {
         i <<= 1; 
         ++xsize; 
      }
      i = 2;
      while (tp->y2 - tp->y1 - 2 >= i) {
         i <<= 1; 
         ++ysize; 
      }
      add_worklist(xxstart,xxstop,yystart,yystop,
          (ysize<<12)+tp->y1,(xsize<<12)+tp->x1,worksym);
      return(-1);
   }
   return(0);

} /* tesseral */

static int _fastcall tesschkcol(int x,int y1,int y2)
{
   int i;
   i = getcolor(x,++y1);
   while (--y2 > y1)
      if (getcolor(x,y2) != i) return -1;
   return i;
}

static int _fastcall tesschkrow(int x1,int x2,int y)
{
   int i;
   i = getcolor(x1,y);
   while (x2 > x1) {
      if (getcolor(x2,y) != i) return -1;
      --x2; 
   }
   return i;
}

static int _fastcall tesscol(int x,int y1,int y2)
{
   int colcolor,i;
   col = x;
   row = y1;
   reset_periodicity = 1;
   colcolor = (*calctype)();
   reset_periodicity = 0;
   while (++row <= y2) { /* generate the column */
      if ((i = (*calctype)()) < 0) return -3;
      if (i != colcolor) colcolor = -1;
   }
   return colcolor;
}

static int _fastcall tessrow(int x1,int x2,int y)
{
   int rowcolor,i;
   row = y;
   col = x1;
   reset_periodicity = 1;
   rowcolor = (*calctype)();
   reset_periodicity = 0;
   while (++col <= x2) { /* generate the row */
      if ((i = (*calctype)()) < 0) return -3;
      if (i != rowcolor) rowcolor = -1;
   }
   return rowcolor;
}

/******************* standalone engine for "lyapunov" ********************/
/*** Roy Murphy [76376,721] ***/
/*** benchmark 15:05		***/
int lyaLength;
int lyaRxy[18];

lyapunov () {
    int count, i, row, col, startrow, lnadjust, filter_cycles;
    long la, lb, ltemp;
    double a, b, lyap, total, temp;
    /* e10=22026.4657948  e-10=0.0000453999297625 */

  	startrow = 0;
  	if (resuming) {
  		start_resume();
  		get_resume(sizeof(int), &startrow, 0);
  		end_resume();
  		}
  	for (row=startrow; row<iystop; row++) {
   		for (col=0; col<ixstop; col++) {
			if (check_key()) {
    			alloc_resume(10,1);
    			put_resume(sizeof(int), &row, 0);
		    	return (-1);
    			}

			overflow=FALSE;
		    total = 1.0;
		    lnadjust = 0;
   	    	(*plot)(col, row, 1);

    		a = dx0[col]+dx1[row];
			b = dy0[row]+dy1[col];
		    Population = param[1];
		    if ((filter_cycles=param[2])==0)
		    	filter_cycles=maxit/2;

		    for (i=0; i<filter_cycles; i++) {
	    		for (count=0; count<lyaLength; count++) {
					Rate = lyaRxy[count]?(a):(b);
					if (curfractalspecific->orbitcalc()) {
    					overflow = TRUE;
    					goto jumpout;
    					}
    				}
				}

		    for (i=0; i<maxit/2; i++) {
	    		for (count=0; count<lyaLength; count++) {
					Rate = lyaRxy[count]?(a):(b);
					if (curfractalspecific->orbitcalc()) {
						overflow = TRUE;
    					goto jumpout;
						}
					temp = fabs(Rate-2.0*Rate*Population);
					if ((total *= (temp))==0) {
						lnadjust = 0;
						goto jumpout;
						}
					while (total > 22026.4657948) {
						total *= 0.0000453999297625;
						lnadjust += 10;
						}
					while (total < 0.0000453999297625) {
						total *= 22026.4657948;
						lnadjust -= 10;
						}
					}
		   		}

jumpout:	if ( overflow || (total <= 0)
				|| (temp = log(total)+lnadjust) > 0) {
			    color = 0;
			    }
			else {
				if (LogFlag) {
		    		lyap = -temp/((double) lyaLength*i);
		    		}
		    	else
		    		lyap = 1 - exp(temp/((double) lyaLength*i));
		    	color = 1 + (int)(lyap * (colors-1));
				}
	    	(*plot)(col, row, color);
	    	}
		}
    return 1;
    }

lya_setup () {
  	/* This routine sets up the sequence for forcing the Rate parameter
  		to vary between the two values.  It fills the array lyaRxy[] and
  		sets lyaLength to the length of the sequence.

  		The sequence is coded in the bit pattern in an integer.
  		Briefly, the sequence starts with an A the leading zero bits
  		are ignored and the remaining bit sequence is decoded.  The
  		sequence ends with a B.  Not all possible sequences can be
  		represented in this manner, but every possible sequence is
  		either represented as itself, as a rotation of one of the
  		representable sequences, or as the inverse of a representable
  		sequence (swapping 0s and 1s in the array.)  Sequences that
  		are the rotation and/or inverses of another sequence will generate
  		the same lyapunov exponents.

  		A few examples follow:
  			number	sequence
  				0		ab
  				1		aab
  				2		aabb
  				3		aaab
  				4		aabbb
  				5		aabab
  				6		aaabb (this is a duplicate of 4, a rotated inverse)
  				7		aaaab
  				8		aabbbb	etc.
	 */

    int i, t;


    lyaLength = 1;
    i = (int) param[0];
    lyaRxy[0] = 1;
    for (t=15; t>=0; t--)
		if (i & (1<<t)) break;
    for (; t>=0; t--)
		lyaRxy[lyaLength++] = (i & (1<<t)) != 0;
    lyaRxy[lyaLength++] = 0;
    return 1;
 }
