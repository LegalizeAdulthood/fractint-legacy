/*
FRACSUBR.C contains subroutines which belong primarily to CALCFRAC.C and
FRACTALS.C, i.e. which are non-fractal-specific fractal engine subroutines.
*/

#include <stdio.h>
#include <stdarg.h>
#include <float.h>
#include "fractint.h"
#include "fractype.h"
#include "mpmath.h"

/* routines in this module	*/

void calcfracinit(void);
void adjust_corner(void);
int  alloc_resume(int,int);
int  put_resume(int,...);
int  start_resume(void);
int  get_resume(int,...);
void end_resume(void);
void iplot_orbit(long,long,int);
void plot_orbit(double,double,int);
void scrub_orbit(void);
int  add_worklist(int,int,int,int,int,int,int);
void tidy_worklist(void);
int  ssg_blocksize(void);
void get_julia_attractor(double,double);

static long   _fastcall fudgetolong(double d);
static double _fastcall fudgetodouble(long l);
static void   _fastcall adjust_to_limits(double);
static void   _fastcall smallest_add(double *);
static int    _fastcall ratio_bad(double,double);
static void   _fastcall plotdorbit(double,double,int);
static int    _fastcall combine_worklist(void);


extern int    calc_status;		/* status of calculations */
extern char far *resume_info;		/* pointer to resume info if allocated */
       int    resume_len;		/* length of resume info */
static int    resume_offset;		/* offset in resume info gets */
extern double plotmx1,plotmx2,plotmy1,plotmy2; /* real->screen conversion */
extern int    orbit_ptr;		/* pointer into save_orbit array */
extern int far *save_orbit;		/* array to save orbit values */
extern int    orbit_color;		/* XOR color */
extern int    num_worklist;		/* resume worklist for standard engine */
extern int    fractype; 		/* fractal type */
extern char   stdcalcmode;		/* '1', '2', 'g', 'b'       */
extern char   floatflag;		/* floating-point fractals? */
extern int    integerfractal;		/* TRUE if fractal uses integer math */
extern struct workliststuff worklist[MAXCALCWORK];
extern int    sxdots,sydots;		/* # of dots on the physical screen    */
extern int    sxoffs,syoffs;		/* physical top left of logical screen */
extern int    xdots, ydots;		/* # of dots on the logical screen     */
extern int    colors;			/* maximum colors available */
extern long   fudge;			/* fudge factor (2**n) */
extern int    bitshift; 		/* bit shift for fudge */
extern double xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* corners */
extern long   xmin, xmax, ymin, ymax, x3rd, y3rd;  /* integer equivs */
extern int    maxit;			/* try this many iterations */
extern int    attractors;		/* number of finite attractors	*/
extern struct complex  attr[];		/* finite attractor vals (f.p)	*/
extern struct lcomplex lattr[]; 	/* finite attractor vals (int)	*/
extern struct complex  old,new;
extern struct lcomplex lold,lnew;
extern double tempsqrx,tempsqry;
extern long   ltempsqrx,ltempsqry;
extern int    xxstart,xxstop;		/* these are same as worklist, */
extern int    yystart,yystop,yybegin;	/* declared as separate items  */
extern int    periodicitycheck;
extern int    basin;
extern int    finattract;
extern int    pixelpi;			/* value of pi in pixels */
extern double closenuff;
extern long   lclosenuff;
extern int    ixstart, ixstop, iystart, iystop;
extern int    color;
extern int    decomp[];
extern double potparam[3];		/* three potential parameters*/
extern int    distest;			/* non-zero if distance estimator */
extern double param[4]; 		/* up to four parameters */
extern int    invert;			/* non-zero if inversion active */
extern int    biomorph,usr_biomorph;
extern int    debugflag; /* internal use only - you didn't see this */
extern long   creal, cimag;		/* for calcmand */
extern long   delx, dely;		/* screen pixel increments  */
extern long   delx2, dely2;		/* screen pixel increments  */
extern double delxx, delyy;		/* screen pixel increments  */
extern double delxx2, delyy2;		/* screen pixel increments  */
extern long   delmin;			/* for calcfrac/calcmand    */
extern double ddelmin;			/* same as a double	    */
extern int    potflag;			/* continuous potential flag */
extern int    bailout;
extern double rqlim;
extern double  dxsize, dysize;		/* xdots-1, ydots-1	    */

extern long   far *lx0, far *ly0;	/* x, y grid		    */
extern long   far *lx1, far *ly1;	/* adjustment for rotate    */
/* note that lx1 & ly1 values can overflow into sign bit; since     */
/* they're used only to add to lx0/ly0, 2s comp straightens it out  */
extern double far *dx0, far *dy0;	/* floating pt equivs */
extern double far *dx1, far *dy1;

extern char   usr_floatflag;
extern char   usr_stdcalcmode;
extern int    usr_periodicitycheck;
extern int    usr_distest;

extern int StandardFractal(void);
extern int calcmand(void);

#define FUDGEFACTOR	29	/* fudge all values up by 2**this */
#define FUDGEFACTOR2	24	/* (or maybe this)		  */


void calcfracinit() /* initialize a *pile* of stuff for fractal calculation */
{
   int i;
   double ftemp;

   floatflag	    = usr_floatflag;

init_restart:

   /* the following variables may be forced to a different setting due to
      calc routine constraints;  usr_xxx is what the user last said is wanted,
      xxx is what we actually do in the current situation */
   stdcalcmode	    = usr_stdcalcmode;
   periodicitycheck = usr_periodicitycheck;
   distest	    = usr_distest;
   biomorph	    = usr_biomorph;

   potflag = 0;
   if (potparam[0] != 0.0
     && colors >= 256
     && (curfractalspecific->calctype == StandardFractal
	 || curfractalspecific->calctype == calcmand)) {
      potflag = 1;
      distest = 0;    /* can't do distest too */
      }

   if (distest)
      floatflag = 1;  /* force floating point for dist est */

   if (floatflag) { /* ensure type matches floatflag */
      if (curfractalspecific->isinteger != 0
	&& curfractalspecific->tofloat != NOFRACTAL)
	 fractype = curfractalspecific->tofloat;
      }
   else {
      if (curfractalspecific->isinteger == 0
	&& curfractalspecific->tofloat != NOFRACTAL)
	 fractype = curfractalspecific->tofloat;
      }
   curfractalspecific = &fractalspecific[fractype];

   integerfractal = curfractalspecific->isinteger;

   if (fractype == JULIBROT)
      rqlim = 4;
   else if (potflag && potparam[2] != 0.0)
      rqlim = potparam[2];
/* else if (decomp[0] > 0 && decomp[1] > 0)
      rqlim = (double)decomp[1]; */
   else if (bailout) /* user input bailout */
      rqlim = bailout;
   else if (biomorph != -1) /* biomorph benefits from larger bailout */
      rqlim = 100;
   else
      rqlim = curfractalspecific->orbit_bailout;
   if (integerfractal) /* the bailout limit mustn't be too high here */
      if (rqlim > 127.0) rqlim = 127.0;

   if (curfractalspecific->flags&NOROTATE != 0) {
      /* ensure min<max and unrotated rectangle */
      if (xxmin > xxmax) { ftemp = xxmax; xxmax = xxmin; xxmin = ftemp; }
      if (yymin > yymax) { ftemp = yymax; yymax = yymin; yymin = ftemp; }
      xx3rd = xxmin; yy3rd = yymin;
      }

   /* set up bitshift for integer math */
   bitshift = FUDGEFACTOR2; /* by default, the smaller shift */
   if (integerfractal > 1)  /* use specific override from table */
      bitshift = integerfractal;
   if (integerfractal == 0) /* float? */
      if ((i = curfractalspecific->tofloat) != NOFRACTAL) /* -> int? */
	 if (fractalspecific[i].isinteger > 1) /* specific shift? */
	    bitshift = fractalspecific[i].isinteger;

   if (fractype == MANDEL || fractype == JULIA) { /* adust shift bits if.. */
      if (potflag == 0				  /* not using potential */
	&& (fractype != MANDEL			  /* and not an int mandel */
	    || (param[0] == 0.0 && param[1] == 0.0))  /* w/ "fudged" params */
	&& !invert				  /* and not inverting */
	&& biomorph == -1			  /* and not biomorphing */
	&& rqlim <= 4.0 			  /* and bailout not too high */
	&& debugflag != 1234)			  /* and not debugging */
	 bitshift = FUDGEFACTOR;		  /* use the larger bitshift */
      if (param[0] < -1.99 || param[0] > 1.99) param[0] = 1.99;
      if (param[1] < -1.99 || param[1] > 1.99) param[1] = 1.99;
      }

   fudge = 1L << bitshift;

   /* now setup arrays of real coordinates corresponding to each pixel */

   adjust_to_limits(1.0); /* make sure all corners in valid range */

   delxx  = (xxmax - xx3rd) / dxsize; /* calculate stepsizes */
   delyy  = (yymax - yy3rd) / dysize;
   delxx2 = (xx3rd - xxmin) / dysize;
   delyy2 = (yy3rd - yymin) / dxsize;

   creal = fudgetolong(param[0]); /* integer equivs for it all */
   cimag = fudgetolong(param[1]);
   xmin  = fudgetolong(xxmin);
   xmax  = fudgetolong(xxmax);
   x3rd  = fudgetolong(xx3rd);
   ymin  = fudgetolong(yymin);
   ymax  = fudgetolong(yymax);
   y3rd  = fudgetolong(yy3rd);
   delx  = fudgetolong(delxx);
   dely  = fudgetolong(delyy);
   delx2 = fudgetolong(delxx2);
   dely2 = fudgetolong(delyy2);

   if (fractype != PLASMA) { /* skip this if plasma to avoid 3d problems */
      if (integerfractal && !invert) {
	 if (	(delx  == 0 && delxx  != 0.0)
	     || (delx2 == 0 && delxx2 != 0.0)
	     || (dely  == 0 && delyy  != 0.0)
	     || (dely2 == 0 && delyy2 != 0.0) )
	    goto expand_retry;
	 lx0[0] = xmin; 		/* fill up the x, y grids */
	 ly0[0] = ymax;
	 lx1[0] = ly1[0] = 0;
	 for (i = 1; i < xdots; i++ ) {
	    lx0[i] = lx0[i-1] + delx;
	    ly1[i] = ly1[i-1] - dely2;
	    }
	 for (i = 1; i < ydots; i++ ) {
	    ly0[i] = ly0[i-1] - dely;
	    lx1[i] = lx1[i-1] + delx2;
	    }
	 /* past max res?  check corners within 10% of expected */
	 if (	ratio_bad((double)lx0[xdots-1]-xmin,(double)xmax-x3rd)
	     || ratio_bad((double)ly0[ydots-1]-ymax,(double)y3rd-ymax)
	     || ratio_bad((double)lx1[(ydots>>1)-1],((double)x3rd-xmin)/2)
	     || ratio_bad((double)ly1[(xdots>>1)-1],((double)ymin-y3rd)/2) ) {
expand_retry:
	    if (integerfractal		/* integer fractal type? */
	      && curfractalspecific->tofloat != NOFRACTAL)
	       floatflag = 1;		/* switch to floating pt */
	    else
	       adjust_to_limits(2.0);	/* double the size */
	    if (calc_status == 2)	/* due to restore of an old file? */
	       calc_status = 0; 	/*   whatever, it isn't resumable */
	    goto init_restart;
	    }
	 /* re-set corners to match reality */
	 xmax = lx0[xdots-1] + lx1[ydots-1];
	 ymin = ly0[ydots-1] + ly1[xdots-1];
	 x3rd = xmin + lx1[ydots-1];
	 y3rd = ly0[ydots-1];
	 xxmin = fudgetodouble(xmin);
	 xxmax = fudgetodouble(xmax);
	 xx3rd = fudgetodouble(x3rd);
	 yymin = fudgetodouble(ymin);
	 yymax = fudgetodouble(ymax);
	 yy3rd = fudgetodouble(y3rd);
	 }
      else {
	 /* set up dx0 and dy0 analogs of lx0 and ly0 */
	 /* put fractal parameters in doubles */
	 dx0[0] = xxmin;		/* fill up the x, y grids */
	 dy0[0] = yymax;
	 dx1[0] = dy1[0] = 0;
	 for (i = 1; i < xdots; i++ ) {
	    dx0[i] = dx0[i-1] + delxx;
	    dy1[i] = dy1[i-1] - delyy2;
	    }
	 for (i = 1; i < ydots; i++ ) {
	    dy0[i] = dy0[i-1] - delyy;
	    dx1[i] = dx1[i-1] + delxx2;
	    }
	 if (	ratio_bad(dx0[xdots-1]-xxmin,xxmax-xx3rd)
	     || ratio_bad(dy0[ydots-1]-yymax,yy3rd-yymax)
	     || ratio_bad(dx1[ydots-1],xx3rd-xxmin)
	     || ratio_bad(dy1[xdots-1],yymin-yy3rd))
	    goto expand_retry;
	 /* re-set corners to match reality */
	 xxmax = dx0[xdots-1] + dx1[ydots-1];
	 yymin = dy0[ydots-1] + dy1[xdots-1];
	 xx3rd = xxmin + dx1[ydots-1];
	 yy3rd = dy0[ydots-1];
	 }
      }

   /* for periodicity close-enough, and for unity: */
   /*	  min(max(delx,delx2),max(dely,dely2)	   */
   ddelmin = fabs(delxx);
   if (fabs(delxx2) > ddelmin)
      ddelmin = fabs(delxx2);
   if (fabs(delyy) > fabs(delyy2)) {
      if (fabs(delyy) < ddelmin)
	 ddelmin = fabs(delyy);
      }
   else
      if (fabs(delyy2) < ddelmin)
	 ddelmin = fabs(delyy2);
   delmin = fudgetolong(ddelmin);

   /* calculate factors which plot real values to screen co-ords */
   /* calcfrac.c plot_orbit routines have comments about this	 */
   ftemp = (0.0-delyy2) * delxx2 * dxsize * dysize
	   - (xxmax-xx3rd) * (yy3rd-yymax);
   plotmx1 = delxx2 * dxsize * dysize / ftemp;
   plotmx2 = (yy3rd-yymax) * dxsize / ftemp;
   plotmy1 = (0.0-delyy2) * dxsize * dysize / ftemp;
   plotmy2 = (xxmax-xx3rd) * dysize / ftemp;

}

static long _fastcall fudgetolong(double d)
{
   if ((d *= fudge) > 0) d += 0.5;
   else 		 d -= 0.5;
   return (long)d;
}

static double _fastcall fudgetodouble(long l)
{
   char buf[30];
   double d;
   sprintf(buf,"%.9g",(double)l / fudge);
   sscanf(buf,"%lg",&d);
   return d;
}


void adjust_corner()
{
   /* make edges very near vert/horiz exact, to ditch rounding errs and */
   /* to avoid problems when delta per axis makes too large a ratio	*/
   double ftemp,ftemp2;
   if( (ftemp=fabs(xx3rd-xxmin)) < (ftemp2=fabs(xxmax-xx3rd)) ) {
      if (ftemp*10000 < ftemp2 && yy3rd != yymax)
	 xx3rd = xxmin;
      }
   else if (ftemp2*10000 < ftemp && yy3rd != yymin)
      xx3rd = xxmax;
   if( (ftemp=fabs(yy3rd-yymin)) < (ftemp2=fabs(yymax-yy3rd)) ) {
      if (ftemp*10000 < ftemp2 && xx3rd != xxmax)
	 yy3rd = yymin;
      }
   else if (ftemp2*10000 < ftemp && xx3rd != xxmin)
      yy3rd = yymax;
}

static void _fastcall adjust_to_limits(double expand)
{  double cornerx[4],cornery[4];
   double lowx,highx,lowy,highy,limit,ftemp;
   double centerx,centery,adjx,adjy;
   int i;
   limit = 32767.99;
   if (bitshift >= 24) limit = 31.99;
   if (bitshift >= 29) limit = 3.99;
   centerx = (xxmin+xxmax)/2;
   centery = (yymin+yymax)/2;
   if (xxmin == centerx) { /* ohoh, infinitely thin, fix it */
      smallest_add(&xxmax);
      xxmin -= xxmax-centerx;
      }
   if (yymin == centery) {
      smallest_add(&yymax);
      yymin -= yymax-centery;
      }
   if (xx3rd == centerx)
      smallest_add(&xx3rd);
   if (yy3rd == centery)
      smallest_add(&yy3rd);
   /* setup array for easier manipulation */
   cornerx[0] = xxmin; cornerx[1] = xxmax;
   cornerx[2] = xx3rd; cornerx[3] = xxmin+(xxmax-xx3rd);
   cornery[0] = yymax; cornery[1] = yymin;
   cornery[2] = yy3rd; cornery[3] = yymin+(yymax-yy3rd);
   /* if caller wants image size adjusted, do that first */
   if (expand != 1.0)
      for (i=0; i<4; ++i) {
	 cornerx[i] = centerx + (cornerx[i]-centerx)*expand;
	 cornery[i] = centery + (cornery[i]-centery)*expand;
      }
   /* get min/max x/y values */
   lowx = highx = cornerx[0];
   lowy = highy = cornery[0];
   for (i=1; i<4; ++i) {
      if (cornerx[i] < lowx)  lowx  = cornerx[i];
      if (cornerx[i] > highx) highx = cornerx[i];
      if (cornery[i] < lowy)  lowy  = cornery[i];
      if (cornery[i] > highy) highy = cornery[i];
      }
   /* if image is too large, downsize it maintaining center */
   ftemp = highx-lowx;
   if (highy-lowy > ftemp) ftemp = highy-lowy;
   if ((ftemp = limit*2/ftemp) < 1.0)
      for (i=0; i<4; ++i) {
	 cornerx[i] = centerx + (cornerx[i]-centerx)*ftemp;
	 cornery[i] = centery + (cornery[i]-centery)*ftemp;
	 }
   /* if any corner has x or y past limit, move the image */
   adjx = adjy = 0;
   for (i=0; i<4; ++i) {
      if (cornerx[i] > limit	 && (ftemp = cornerx[i] - limit) > adjx)
	 adjx = ftemp;
      if (cornerx[i] < 0.0-limit && (ftemp = cornerx[i] + limit) < adjx)
	 adjx = ftemp;
      if (cornery[i] > limit	 && (ftemp = cornery[i] - limit) > adjy)
	 adjy = ftemp;
      if (cornery[i] < 0.0-limit && (ftemp = cornery[i] + limit) < adjy)
	 adjy = ftemp;
      }
   if (calc_status == 2 && (adjx != 0 || adjy != 0))
      calc_status = 0;
   xxmin = cornerx[0] - adjx;
   xxmax = cornerx[1] - adjx;
   xx3rd = cornerx[2] - adjx;
   yymax = cornery[0] - adjy;
   yymin = cornery[1] - adjy;
   yy3rd = cornery[2] - adjy;
   adjust_corner(); /* make 3rd corner exact if very near other co-ords */
}

static void _fastcall smallest_add(double *num)
{
   *num += *num * 5.0e-16;
}

static int _fastcall ratio_bad(double actual, double desired)
{  double ftemp;
   if (desired != 0)
      if ((ftemp = actual / desired) < 0.95 || ftemp > 1.05)
	 return(1);
   return(0);
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
   va_list arg_marker;	/* variable arg list */
   char *source_ptr;
   if (resume_info == NULL)
      return(-1);
   va_start(arg_marker,len);
   while (len)
   {
      source_ptr = va_arg(arg_marker,char *);
      far_memcpy(resume_info+resume_len,source_ptr,len);
      resume_len += len;
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
      static char msg[] = {"\
Warning - insufficient free memory to save status.\n\
You will not be able to resume calculating this image."};
      stopmsg(0,msg);
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
   va_list arg_marker;	/* variable arg list */
   char far *source_ptr;
   char *dest_ptr;
   if (resume_info == NULL)
      return(-1);
   va_start(arg_marker,len);
   while (len)
   {
      dest_ptr = va_arg(arg_marker,char *);
      far_memcpy(dest_ptr,resume_info+resume_offset,len);
      resume_offset += len;
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

void end_resume()
{
   if (resume_info != NULL) /* free the prior area if there is one */
   {
      farmemfree(resume_info);
      resume_info = NULL;
   }
}


/* Showing orbit requires converting real co-ords to screen co-ords.
   Define:
       Xs == xxmax-xx3rd	       Ys == yy3rd-yymax
       W  == xdots-1		       D  == ydots-1
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
       row == (realx-xxmin - (col/W)*Xs) / Xv	 (1)
       col == (realy-yymax - (row/D)*Ys) / Yv	 (2)
  substitute (2) into (1) and solve for row:
       row == ((realx-xxmin)*(0-delyy2)*W*D - (realy-yymax)*Xs*D)
		      / ((0-delyy2)*W*delxx2*D-Ys*Xs)
  */

static void _fastcall plotdorbit(double dx, double dy, int color)
{
   int i, j, c;
   int save_sxoffs,save_syoffs;
   if (orbit_ptr >= 1500) return;
   i = dy * plotmx1 - dx * plotmx2; i += sxoffs;
   if (i < 0 || i >= sxdots) return;
   j = dx * plotmy1 - dy * plotmy2; j += syoffs;
   if (j < 0 || j >= sydots) return;
   save_sxoffs = sxoffs;
   save_syoffs = syoffs;
   sxoffs = syoffs = 0;
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
   sxoffs = save_sxoffs;
   syoffs = save_syoffs;
}

void iplot_orbit(ix, iy, color)
long ix, iy;
int color;
{
   plotdorbit((double)ix/fudge-xxmin,(double)iy/fudge-yymax,color);
}

void plot_orbit(real,imag,color)
double real,imag;
int color;
{
   plotdorbit(real-xxmin,imag-yymax,color);
}

void scrub_orbit()
{
   int i,j,c;
   int save_sxoffs,save_syoffs;
   save_sxoffs = sxoffs;
   save_syoffs = syoffs;
   sxoffs = syoffs = 0;
   while(orbit_ptr > 0)
   {
      c = *(save_orbit + --orbit_ptr);
      j = *(save_orbit + --orbit_ptr);
      i = *(save_orbit + --orbit_ptr);
      putcolor(i,j,c);
   }
   sxoffs = save_sxoffs;
   syoffs = save_syoffs;
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
   worklist[num_worklist].pass	  = pass;
   worklist[num_worklist].sym	  = sym;
   ++num_worklist;
   tidy_worklist();
   return(0);
}

static int _fastcall combine_worklist() /* look for 2 entries which can freely merge */
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

   if (attractors >= N_ATTR)	 /* space for more attractors ?  */
      return;		       /* Bad luck - no room left !    */

   savper = periodicitycheck;
   savmaxit = maxit;
   periodicitycheck = 0;
   old.x = real;		    /* prepare for f.p orbit calc */
   old.y = imag;
   tempsqrx = sqr(old.x);
   tempsqry = sqr(old.y);

   lold.x = real;	    /* prepare for int orbit calc */
   lold.y = imag;
   ltempsqrx = tempsqrx;
   ltempsqry = tempsqry;

   lold.x = lold.x << bitshift;
   lold.y = lold.y << bitshift;
   ltempsqrx = ltempsqrx << bitshift;
   ltempsqry = ltempsqry << bitshift;

   if (maxit < 500)	    /* we're going to try at least this hard */
      maxit = 500;
   color = 0;
   while (++color < maxit)
      if(curfractalspecific->orbitcalc())
	 break;
   if (color >= maxit)	    /* if orbit stays in the lake */
   {
      if (integerfractal)   /* remember where it went to */
	 lresult = lnew;
      else
	 result =  new;
      if(!curfractalspecific->orbitcalc()) /* if it stays in the lake */
      { 		       /* and doen't move far, probably */
	 if (integerfractal)   /*   found a finite attractor	*/
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


#define maxyblk 7    /* must match calcfrac.c */
#define maxxblk 202  /* must match calcfrac.c */
int ssg_blocksize() /* used by solidguessing and by zoom panning */
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


