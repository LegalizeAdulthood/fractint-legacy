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
#include <limits.h>
#ifndef XFRACT
#include <dos.h>
#endif
#include <limits.h>
#include "fractint.h"
#include "fractype.h"
#include "mpmath.h"
#include "targa_lc.h"
#include "prototyp.h"

/* routines in this module	*/

static void perform_worklist(void);
static int  OneOrTwoPass(void);
static int  _fastcall StandardCalc(int);
static int  _fastcall potential(double,int);
static void decomposition(void);
static int  bound_trace_main(void);
static void step_col_row(void);
static int  _fastcall boundary_trace(int,int);
static int  _fastcall calc_xy(int,int);
static int  _fastcall fillseg1(int,int,int,int);
static int  _fastcall fillseg(int,int,int,int);
static void _fastcall reverse_string(BYTE *,BYTE *,int);
static int  solidguess(void);
static int  _fastcall guessrow(int,int,int);
static void _fastcall plotblock(int,int,int,int);
static void _fastcall setsymmetry(int,int);
static int  _fastcall xsym_split(int,int);
static int  _fastcall ysym_split(int,int);
static void set_Plasma_palette(void);
static U16 _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb);
static void _fastcall subDivide(int x1,int y1,int x2,int y2);
static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur);
static void verhulst(void);
static void Bif_Period_Init(void);
static int  _fastcall Bif_Periodic(int);
static void set_Cellular_palette(void);

/**CJLT new function prototypes: */
static int tesseral(void);
static int _fastcall tesschkcol(int,int,int);
static int _fastcall tesschkrow(int,int,int);
static int _fastcall tesscol(int,int,int);
static int _fastcall tessrow(int,int,int);
/* added for testing autologmap() */
static int autologmap(void);

extern _CMPLX initorbit;
extern char useinitorbit;
_LCMPLX linitorbit;
extern int showdot;
extern unsigned int decoderline[];
extern int overflow;
long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
_CMPLX init,tmp,old,new,saved;
extern int biomorph,usr_biomorph;
extern _LCMPLX linit;
extern int basin;
extern int cpu;
extern char savename[80];   /* save files using this name */
extern int resave_flag;
extern int started_resaves;
extern int dotmode;
extern int save_release;    /* use this to maintain compatibility */

int color, oldcolor, row, col, passes;
int realcolor;
int iterations, invert;
double f_radius,f_xcenter, f_ycenter; /* for inversion */
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;

extern int LogFlag;
extern BYTE far *LogTable;
extern int rangeslen;
extern int far *ranges;

void (_fastcall *plot)(int,int,int) = putcolor;
typedef void (_fastcall *PLOT)(int,int,int);

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
extern _CMPLX parm,parm2;
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

VOIDFARPTR typespecific_workarea = NULL;

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
/* size of next puts a limit of MAXPIXELS pixels across on solid guessing logic */
#ifndef XFRACT
extern char dstack[4096];               /* common temp, two put_line calls */
extern char suffix[4096];             /* cellular, put_line/get_line calls */
extern unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
#else
BYTE dstack[4096];              /* common temp, two put_line calls */
unsigned int prefix[2][maxyblk][maxxblk]; /* common temp */
#endif

int nxtscreenflag; /* for cellular next screen generation */
int	attractors;		    /* number of finite attractors  */
_CMPLX	attr[N_ATTR];	    /* finite attractor vals (f.p)  */
_LCMPLX lattr[N_ATTR];	    /* finite attractor vals (int)  */
int    attrperiod[N_ATTR];	    /* period of the finite attractor */

/***** vars for new btm *****/
enum direction {North,East,South,West};
enum direction going_to;
int trail_row, trail_col;

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

/* -------------------------------------------------------------------- */
/*		These variables are external for speed's sake only      */
/* -------------------------------------------------------------------- */
extern _LCMPLX lold,lnew,lparm,lparm2;	 /* added "lold" */

int periodicitycheck;
extern long ltempsqrx,ltempsqry;
extern double tempsqrx,tempsqry;
extern _LCMPLX ltmp;
extern int display3d;


void calcfrac_overlay() { }	/* for restore_active_ovly */
extern int first_err;   /* flag for math errors */

/******* calcfract - the top level routine for generating an image *******/

int calcfract()
{
   ENTER_OVLY(OVLY_CALCFRAC);

   first_err = 1;
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
       && curfractalspecific->calctype != calcmandfp
       && curfractalspecific->calctype != lyapunov
       && curfractalspecific->calctype != calcfroth)
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
      free_workarea();
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
      int tmpcalcmode = stdcalcmode;

      stdcalcmode = '1'; /* force 1 pass */
      if (resuming == 0)
	 if (pot_startdisk() < 0)
	 {
	    pot16bit = 0;	/* startdisk failed or cancelled */
	    stdcalcmode = tmpcalcmode;	/* maybe we can carry on??? */
	 }
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

/* added for testing autologmap() */
      if (!(resuming)&&(abs(LogFlag) ==2))
       {  /* calculate round screen edges to work out best start for logmap */
         LogFlag = ( autologmap() * (LogFlag / abs(LogFlag)));
         SetupLogTable();
       }

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
	 color = LogTable[min(color, maxit)];
      if (color >= colors) /* don't use color 0 unless from inside/outside */
	 if (colors < 16)
	    color &= andcolor;
	 else
	    color = ((color - 1) % andcolor) + 1;  /* skip color zero */
      if(debugflag != 470)
         if(color <= 0 && stdcalcmode == 'b' )   /* fix BTM bug */
            color = 1;
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
	    color = LogTable[min(color, maxit)];
	 if (color >= colors) /* don't use color 0 unless from inside/outside */
	    if (colors < 16)
	       color &= andcolor;
	    else
	       color = ((color - 1) % andcolor) + 1;  /* skip color zero */
         if(debugflag != 470)
            if(color == 0 && stdcalcmode == 'b' )   /* fix BTM bug */
               color = 1;
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
   int cyclelen = -1;
   int savedcolor;
   int caught_a_cycle;
   int savedand, savedincr;	/* for periodicity checking */
   _LCMPLX lsaved;
   int i, attracted;
   _LCMPLX lat;
   _CMPLX  at;
   _CMPLX deriv;
   int dem_color;
   _CMPLX dem_new;
   close = .01;
   lclose = close*fudge;

   if(inside == STARTRAIL)
   {
      int i;
      for(i=0;i<16;i++)
	 tantable[i] = 0.0;
   }
   else if(inside == EPSCROSS)
   {
      close = .01;
      lclose = close*fudge;
   }
   if (periodicitycheck == 0 || inside == ZMAG || inside == STARTRAIL)
      oldcolor = 32767; 	/* don't check periodicity at all */
   else if (inside == PERIOD)	/* for display-periodicity */
      oldcolor = maxit*4/5;	/* don't check until nearly done */
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
   if (inside == PERIOD) {
       savedand = 16;		/* begin checking every 16th cycle */
   } else {
       savedand = 1;		/* begin checking every other cycle */
   }
   savedincr = 1;		/* start checking the very first time */

   if (inside <= BOF60 && inside >= BOF61)
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
	 if (curfractalspecific->orbitcalc() && inside != STARTRAIL)
	    break;
      if (show_orbit)
	 if (!integerfractal)
	    plot_orbit(new.x, new.y, -1);
	 else
	    iplot_orbit(lnew.x, lnew.y, -1);
      if(inside == STARTRAIL)
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
      else if(inside == EPSCROSS)
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
      else if (inside <= BOF60 && inside >= BOF61)
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
			 if (finattract<0) color = (color%attrperiod[i])+1;
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
			 if (finattract<0) color = (color%attrperiod[i])+1;
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
	    savedcolor = color;
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
		     cyclelen = color-savedcolor;
		     color = maxit - 1;
		  }
	    }
	    else	     /* integer periodicity check */
	    {
	       if (labs(lsaved.x - lnew.x) < lclosenuff)
		  if (labs(lsaved.y - lnew.y) < lclosenuff)
		  {
		     caught_a_cycle = 1;
		     cyclelen = color-savedcolor;
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
	 color = LogTable[min(color, maxit)];
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
      if (outside == REAL)		 /* "real" */
	 color += new.x + 7;
      else if (outside == IMAG)		 /* "imag" */
	 color += new.y + 7;
      else if (outside == MULT  && new.y)  /* "mult" */
	  color *= (new.x/new.y);
      else if (outside == SUM)		 /* "sum" */
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
         color &= INT_MAX;     /* oops - color can be negative */
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
      color = LogTable[min(color, maxit)];
   goto plot_pixel;

   plot_inside: /* we're "inside" */
   if (periodicitycheck < 0 && caught_a_cycle)
      color = 7;	       /* show periodicity */
   else if (inside >= 0)
      color = inside;	       /* set to specified color, ignore logpal */
   else
   {
      if(inside == STARTRAIL)
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
      else if(inside== PERIOD) {
	  if (cyclelen>0) {
	      color = cyclelen;
	  } else {
	      color = maxit;
	  }
      }
      else if(inside == EPSCROSS)
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
      else if (inside == BOF60)
	 color = sqrt(min_orbit) * 75;
      else if (inside == BOF61)
	 color = min_index;
      else if (inside == ZMAG)
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
	 color = LogTable[min(color, maxit)];
   }

   plot_pixel:

   if (color >= colors) /* don't use color 0 unless from inside/outside */
      if (colors < 16)
	 color &= andcolor;
      else
	 color = ((color - 1) % andcolor) + 1;	/* skip color zero */
   if(debugflag != 470)
      if(color <= 0 && stdcalcmode == 'b' )   /* fix BTM bug */
         color = 1;
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
   static double far cos45	   = 0.70710678118654750; /* cos 45	degrees */
   static double far sin45	   = 0.70710678118654750; /* sin 45	degrees */
   static double far cos22_5   = 0.92387953251128670; /* cos 22.5	degrees */
   static double far sin22_5   = 0.38268343236508980; /* sin 22.5	degrees */
   static double far cos11_25  = 0.98078528040323040; /* cos 11.25	degrees */
   static double far sin11_25  = 0.19509032201612820; /* sin 11.25	degrees */
   static double far cos5_625  = 0.99518472667219690; /* cos 5.625	degrees */
   static double far sin5_625  = 0.09801714032956060; /* sin 5.625	degrees */
   static double far tan22_5   = 0.41421356237309500; /* tan 22.5	degrees */
   static double far tan11_25  = 0.19891236737965800; /* tan 11.25	degrees */
   static double far tan5_625  = 0.09849140335716425; /* tan 5.625	degrees */
   static double far tan2_8125 = 0.04912684976946725; /* tan 2.8125 degrees */
   static double far tan1_4063 = 0.02454862210892544; /* tan 1.4063 degrees */
   static long far lcos45	  ; /* cos 45	  degrees */
   static long far lsin45	  ; /* sin 45	  degrees */
   static long far lcos22_5   ; /* cos 22.5   degrees */
   static long far lsin22_5   ; /* sin 22.5   degrees */
   static long far lcos11_25  ; /* cos 11.25  degrees */
   static long far lsin11_25  ; /* sin 11.25  degrees */
   static long far lcos5_625  ; /* cos 5.625  degrees */
   static long far lsin5_625  ; /* sin 5.625  degrees */
   static long far ltan22_5   ; /* tan 22.5   degrees */
   static long far ltan11_25  ; /* tan 11.25  degrees */
   static long far ltan5_625  ; /* tan 5.625  degrees */
   static long far ltan2_8125 ; /* tan 2.8125 degrees */
   static long far ltan1_4063 ; /* tan 1.4063 degrees */
   static reset_fudge = -1;
   int temp = 0;
   int i;
   _LCMPLX lalt;
   _CMPLX alt;
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


/******************* boundary trace method ***************************
Fractint's original btm was written by David Guenther.  There were a few
rare circumstances in which the original btm would not trace or fill
correctly, even on Mandelbrot Sets.  The code below was adapted from
"Mandelbrot Sets by Wesley Loewer" (see calmanfp.asm) which was written
before I was introduced to Fractint.  It should be noted that without
David Guenther's implimentation of a btm, I doubt that I would have been
able to impliment my own code into Fractint.  There are several things in
the following code that are not original with me but came from David
Guenther's code.  I've noted these places with the initials DG.

					Wesley Loewer 3/8/92
*********************************************************************/
#define bkcolor 0  /* I have some ideas for the future with this. -Wes */
#define advance_match()     coming_from = ((going_to = (going_to - 1) & 0x03) - 1) & 0x03
#define advance_no_match()  going_to = (going_to + 1) & 0x03

static
int  bound_trace_main()
    {
    enum direction coming_from;
    unsigned int match_found, continue_loop;
    int trail_color, fillcolor_used, last_fillcolor_used;
    int max_putline_length;
    int right, left, length;

    if (inside == 0 || outside == 0)
	{
	static char far msg[]=
	    "Boundary tracing cannot be used with inside=0 or outside=0.";
	stopmsg(0,msg);
	return(-1);
	}
    if (colors < 16)
	{
	static char far msg[]=
	    "Boundary tracing cannot be used with < 16 colors.";
	stopmsg(0,msg);
	return(-1);
	}

    got_status = 2;
    max_putline_length = 0; /* reset max_putline_length */
    for (currow = iystart; currow <= iystop; currow++)
	{
	reset_periodicity = 1; /* reset for a new row */
	color = bkcolor;
	for (curcol = ixstart; curcol <= ixstop; curcol++)
	    {
	    if (getcolor(curcol, currow) != bkcolor)
		continue;

	    trail_color = color;
	    row = currow;
	    col = curcol;
	    if ((*calctype)()== -1) /* color, row, col are global */
		{
		if (iystop != yystop)  /* DG */
		   iystop = yystop - (currow - yystart); /* allow for sym */
		add_worklist(xxstart,xxstop,currow,iystop,currow,0,worksym);
		return -1;
		}
	    reset_periodicity = 0; /* normal periodicity checking */

	    /*
	    This next line may cause a few more pixels to be calculated,
	    but at the savings of quite a bit of overhead
	    */
	    if (color != trail_color)  /* DG */
		continue;

	    /* sweep clockwise to trace outline */
	    trail_row = currow;
	    trail_col = curcol;
	    trail_color = color;
	    fillcolor_used = fillcolor > 0 ? fillcolor : trail_color;
	    coming_from = West;
	    going_to = East;
	    match_found = 0;
	    continue_loop = TRUE;
	    do
		{
		step_col_row();
		if (row >= currow
			&& col >= ixstart
			&& col <= ixstop
			&& row <= iystop)
		    {
		    /* the order of operations in this next line is critical */
		    if ((color = getcolor(col, row)) == bkcolor && (*calctype)()== -1)
				/* color, row, col are global for (*calctype)() */
			{
			if (iystop != yystop)  /* DG */
			   iystop = yystop - (currow - yystart); /* allow for sym */
			add_worklist(xxstart,xxstop,currow,iystop,currow,0,worksym);
			return -1;
			}
		    else if (color == trail_color)
			{
			if (match_found < 4) /* to keep it from overflowing */
				match_found++;
			trail_row = row;
			trail_col = col;
			advance_match();
			}
		    else
			{
			advance_no_match();
			continue_loop = going_to != coming_from || match_found;
			}
		    }
		else
		    {
		    advance_no_match();
		    continue_loop = going_to != coming_from || match_found;
		    }
		} while (continue_loop && (col != curcol || row != currow));

	    if (match_found <= 3)  /* DG */
		{ /* no hole */
		color = bkcolor;
		reset_periodicity = 1;
		continue;
		}

/*
Fill in region by looping around again, filling lines to the left
whenever going_to is South or West
*/
	    trail_row = currow;
	    trail_col = curcol;
	    coming_from = West;
	    going_to = East;
	    do
		{
		match_found = FALSE;
		do
		    {
		    step_col_row();
		    if (row >= currow
			    && col >= ixstart
			    && col <= ixstop
			    && row <= iystop
			    && getcolor(col,row) == trail_color)
			      /* getcolor() must be last */
			{
			if (going_to == South
				|| (going_to == West && coming_from != East))
			    { /* fill a row, but only once */
			    right = col;
			    while (--right >= ixstart && (color = getcolor(right,row)) == trail_color)
				; /* do nothing */
			    if (color == bkcolor) /* check last color */
				{
				left = right;
				while (getcolor(--left,row) == bkcolor)
				      /* Should NOT be possible for left < ixstart */
				    ; /* do nothing */
				left++; /* one pixel too far */
				if (right == left) /* only one hole */
				    (*plot)(left,row,fillcolor_used);
				else
				    { /* fill the line to the left */
				    length=right-left+1;
				    if (fillcolor_used != last_fillcolor_used || length > max_putline_length)
					{ /* only reset dstack if necessary */
					memset(dstack,fillcolor_used,length);
					last_fillcolor_used = fillcolor_used;
					max_putline_length = length;
					}
				    put_line(row,left,right,dstack);
				    /* here's where all the symmetry goes */
				    if (plot == putcolor)
					kbdcount -= length >> 4; /* seems like a reasonable value */
				    else if (plot == symplot2) /* X-axis symmetry */
					{
					put_line(yystop-(row-yystart),left,right,dstack);
					kbdcount -= length >> 3;
					}
				    else if (plot == symplot2Y) /* Y-axis symmetry */
					{
					put_line(row,xxstop-(right-xxstart),xxstop-(left-xxstart),dstack);
					kbdcount -= length >> 3;
					}
				    else if (plot == symplot2J)  /* Origin symmetry */
					{
					put_line(yystop-(row-yystart),xxstop-(right-xxstart),xxstop-(left-xxstart),dstack);
					kbdcount -= length >> 3;
					}
				    else if (plot == symplot4) /* X-axis and Y-axis symmetry */
					{
					put_line(yystop-(row-yystart),left,right,dstack);
					put_line(row,xxstop-(right-xxstart),xxstop-(left-xxstart),dstack);
					put_line(yystop-(row-yystart),xxstop-(right-xxstart),xxstop-(left-xxstart),dstack);
					kbdcount -= length >> 2;
					}
				    else    /* cheap and easy way out */
					{
					int c;
					for (c = left; c <= right; c++)  /* DG */
					    (*plot)(c,row,fillcolor_used);
					kbdcount -= length >> 1;
					}
				    }
				} /* end of fill line */

			    if(--kbdcount<=0)
				{
				if(check_key())
				    {
				    if (iystop != yystop)
				       iystop = yystop - (currow - yystart); /* allow for sym */
				    add_worklist(xxstart,xxstop,currow,iystop,currow,0,worksym);
				    return(-1);
				    }
				kbdcount=max_kbdcount;
				}
			    }
			trail_row = row;
			trail_col = col;
			advance_match();
			match_found = TRUE;
			}
		    else
			advance_no_match();
		    } while (!match_found && going_to != coming_from);

		if (!match_found)
		    { /* next one has to be a match */
		    step_col_row();
		    trail_row = row;
		    trail_col = col;
		    advance_match();
		    }
		} while (trail_col != curcol || trail_row != currow);
	    reset_periodicity = 1; /* reset after a trace/fill */
	    color = bkcolor;
	    }
	}
    return 0;
    }

/*******************************************************************/
/* take one step in the direction of going_to */
static void step_col_row()
    {
    switch (going_to)
	{
	case North:
	    col = trail_col;
	    row = trail_row - 1;
	    break;
	case East:
	    col = trail_col + 1;
	    row = trail_row;
	    break;
	case South:
	    col = trail_col;
	    row = trail_row + 1;
	    break;
	case West:
	    col = trail_col - 1;
	    row = trail_row;
	    break;
	}
    }

/******************* end of boundary trace method *******************/


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
	    if((*calctype)()== -1)
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
	    prefix[1][y][xlim]= -1;
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
	 pfxp0= &prefix[0][y][0];
	 pfxp1= &prefix[1][y][0];
	 for(x=0;++x<xlim;)
	 {
	    ++pfxp1;
	    u= *(pfxp1-1)|*pfxp1|*(pfxp1+1);
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

#define calcadot(c,x,y) { col=x; row=y; if((c=(*calctype)())== -1) return -1; }

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
   pfxptr= &prefix[firstpass][(i>>4)+1][ixstart/maxblock];
   pfxmask=1<<(i&15);
   ylesshalf=y-halfblock;
   ylessblock=y-blocksize; /* constants, for speed */
   yplushalf=y+halfblock;
   yplusblock=y+blocksize;
   prev11= -1;
   c24=c12=c13=c22=getcolor(ixstart,y);
   c31=c21=getcolor(ixstart,(y>0)?ylesshalf:0);
   if(yplusblock<=iystop)
      c24=getcolor(ixstart,yplusblock);
   else if(bottom_guess==0)
      c24= -1;
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
	    c31= -1;
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
	 c41=c42=c44= -1;
      if(yplusblock>iystop)
	 c44=(bottom_guess)?c42:-1;

      /* guess or calc the remaining 3 quarters of current block */
      guessed23=guessed32=guessed33=1;
      c23=c32=c33=c22;
      if(yplushalf>iystop)
      {
	 if(bottom_guess==0)
	    c23=c33= -1;
	 guessed23=guessed33= -1;
      }
      if(xplushalf>ixstop)
      {
	 if(right_guess==0)
	    c32=c33= -1;
	 guessed32=guessed33= -1;
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
	 put_line(j,xxstart,ixstop,&dstack[xxstart+MAXPIXELS]);
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
	    j+=MAXPIXELS;
	    color=dstack[i+MAXPIXELS];
	    dstack[i+MAXPIXELS]=dstack[j];
	    dstack[j]=color;
	 }
      for(i=0;i<halfblock;++i)
      {
	 if((j=yystop-(y+i-yystart))>iystop && j<ydots)
	    put_line(j,xxstart,ixstop,&dstack[xxstart]);
	 if((j=yystop-(y+i+halfblock-yystart))>iystop && j<ydots)
	    put_line(j,xxstart,ixstop,&dstack[xxstart+MAXPIXELS]);
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
	    dstack[i+MAXPIXELS]=color;
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

#ifdef _MSC_VER
#pragma optimize ("ea", off)
#endif

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
   if(forcesymmetry < 999)
      sym = forcesymmetry;
   else if(forcesymmetry == 1000)
      forcesymmetry = sym;  /* for backwards compatibility */
   else if(outside==REAL || outside==IMAG || outside==MULT || outside==SUM)
      return;
   parmszero = (parm.x == 0.0 && parm.y == 0.0 && useinitorbit != 1);
   switch (fractype)
   { case LMANLAMFNFN:      /* These need only P1 checked. */
     case FPMANLAMFNFN:     /* P2 is used for a switch value */
     case LMANFNFN:         /* These have NOPARM set in fractalp.c, */
     case FPMANFNFN:        /* but it only applies to P1. */
     case FPMANDELZPOWER:   /* or P2 is an exponent */
     case LMANDELZPOWER:
     case FPMANZTOZPLUSZPWR:
       break;
     default:   /* Check P2 for the rest */
       parmszero = (parmszero && parm2.x == 0.0 && parm2.y == 0.0);
   }
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
      if(fabs(xxmax - xxmin) < PI/4)   
         break; /* no point in pi symmetry if values too close */
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

#ifdef _MSC_VER
#pragma optimize ("ea", on)
#endif

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

      {
      int mid,midcolor;
      if (tp->x2 - tp->x1 > tp->y2 - tp->y1) { /* divide down the middle */
         mid = (tp->x1 + tp->x2) >> 1;		 /* Find mid point */
         midcolor = tesscol(mid, tp->y1+1, tp->y2-1); /* Do mid column */
         if (midcolor != tp->top) goto tess_split;
         }
      else { 				  /* divide across the middle */
         mid = (tp->y1 + tp->y2) >> 1;		 /* Find mid point */
         midcolor = tessrow(tp->x1+1, tp->x2-1, mid); /* Do mid row */
         if (midcolor != tp->top) goto tess_split;
         }
      }

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
            memset(&dstack[MAXPIXELS],tp->top,j);
            for (row = tp->y1 + 1; row < tp->y2; row++) {
               put_line(row,tp->x1+1,tp->x2-1,&dstack[MAXPIXELS]);
               if (plot != putcolor) /* symmetry */
                  if ((j = yystop-(row-yystart)) > iystop && j < ydots)
                     put_line(j,tp->x1+1,tp->x2-1,&dstack[MAXPIXELS]);
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

/* added for testing autologmap() */ /* CAE 9211 fixed missing comment */
/* insert at end of CALCFRAC.C */

static int autologmap()   /*RB*/
{  /* calculate round screen edges to avoid wasted colours in logmap */
 int mincolour,lag;
 mincolour=32767;
 row=iystart;
 reset_periodicity = 0;
 for (col=ixstart;col<ixstop;col++) /* top row */
    {
      color=(*calctype)();
      if (realcolor < mincolour) mincolour=realcolor ;
      if ( col >=32 ) (*plot)(col-32,row,0);
    }                                    /* these lines tidy up for BTM etc */
    for (lag=32;lag>0;lag--) (*plot)(col-lag,row,0);

 col=ixstop;
 for (row=iystart;row<iystop;row++) /* right  side */
    {
      color=(*calctype)();
      if (realcolor < mincolour) mincolour=realcolor ;
      if ( row >=32 ) (*plot)(col,row-32,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col,row-lag,0);

 col=ixstart;
 for (row=iystart;row<iystop;row++) /* left  side */
    {
      color=(*calctype)();
      if (realcolor < mincolour) mincolour=realcolor ;
      if ( row >=32 ) (*plot)(col,row-32,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col,row-lag,0);

 row=iystop ;
 for (col=ixstart;col<ixstop;col++) /* bottom row */
    {
      color=(*calctype)();
      if (realcolor < mincolour) mincolour=realcolor ;
      if ( col >=32 ) (*plot)(col-32,row,0);
    }
    for (lag=32;lag>0;lag--) (*plot)(col-lag,row,0);

 if (mincolour==2) resuming=1; /* insure autologmap not called again */

 return mincolour;
}

/* Symmetry plot for period PI */
void _fastcall symPIplot(x, y, color)
int x, y, color ;
{
   while(x <= xxstop)
   {
      putcolor(x, y, color) ;
      x += pixelpi;
   }
}
/* Symmetry plot for period PI plus Origin Symmetry */
void _fastcall symPIplot2J(x, y, color)
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
void _fastcall symPIplot4J(x, y, color)
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
void _fastcall symplot2(x, y, color)
int x, y, color ;
{
   int i;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots)
      putcolor(x, i, color) ;
}

/* Symmetry plot for Y Axis Symmetry */
void _fastcall symplot2Y(x, y, color)
int x, y, color ;
{
   int i;
   putcolor(x, y, color) ;
   if ((i=xxstop-(x-xxstart)) < xdots)
      putcolor(i, y, color) ;
}

/* Symmetry plot for Origin Symmetry */
void _fastcall symplot2J(x, y, color)
int x, y, color ;
{
   int i,j;
   putcolor(x, y, color) ;
   if ((i=yystop-(y-yystart)) > iystop && i < ydots
       && (j=xxstop-(x-xxstart)) < xdots)
      putcolor(j, i, color) ;
}

/* Symmetry plot for Both Axis Symmetry */
void _fastcall symplot4(x, y, color)
int x, y, color ;
{
   int i,j;
   j = xxstop-(x-xxstart);
   putcolor(	   x , y, color) ;
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
void _fastcall symplot2basin(x, y, color)
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
      color -= stripe;			  /* reconstruct unstriped color */
      color = (degree+1-color)%degree+1;  /* symmetrical color */
      color += stripe;			  /* add stripe */
      putcolor(x, i,color)  ;
   }
}

/* Symmetry plot for Both Axis Symmetry  - Newtbasin version */
void _fastcall symplot4basin(x, y, color)
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
   color -= stripe;		  /* reconstruct unstriped color */
   color1 = degree/2+degree+2 - color;
   if(color < degree/2+2)
      color1 = degree/2+2 - color;
   else
      color1 = degree/2+degree+2 - color;
   j = xxstop-(x-xxstart);
   putcolor(	   x, y, color+stripe) ;
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
void _fastcall noplot(int x,int y,int color)
{
}
