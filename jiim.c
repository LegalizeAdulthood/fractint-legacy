/*
 * JIIM.C
 *
 * Generates Inverse Julia in real time, lets move a cursor which determines
 * the J-set.
 *
 *  The J-set is generated in a fixed-size window, a third of the screen.
 *
 * This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
 *
 *
 * The routines used to set/move the cursor and to save/restore the
 * window were "borrowed" from editpal.c (TW - now we *use* the editpal code)
 *     (if you don't know how to write good code, look for someone who does)
 *
 *    JJB  [jbuhler@gidef.edu.ar]
 *    TIW  Tim Wegner
 *    MS   Michael Snyder
 *    KS   Ken Shirriff
 * Revision History:
 *
 *        7-28-92       JJB  Initial release out of editpal.c
 *        7-29-92       JJB  Added SaveRect() & RestoreRect() - now the
 *                           screen is restored after leaving.
 *        7-30-92       JJB  Now, if the cursor goes into the window, the
 *                           window is moved to the other side of the screen.
 *                           Worked from the first time!
 *        10-09-92      TIW  A major rewrite that merged cut routines duplicated
 *                           in EDITPAL.C and added orbits feature.
 *        11-02-92      KS   Made cursor blink
 *        11-18-92      MS   Altered Inverse Julia to use MIIM method.
 *	  11-25-92	MS   Modified MIIM support routines to better be
 *			     shared with stand-alone inverse julia in
 *			     LORENZ.C, and to use DISKVID for swap space.
 *        05-05-93      TIW  Boy this change file really got out of date.
 *			     Added orbits capability, various commands, some 
 * 			     of Dan Farmer's ideas like circles and lines 
 *			     connecting orbits points.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef XFRACT
#include <stdarg.h>
#include <dos.h>     /* for FP_SEG & FP_OFF */
#else
#include <varargs.h>
#endif

#include <math.h>

#ifdef __TURBOC__
#   include <mem.h>   /* to get mem...() declarations */
#endif

#include "helpdefs.h"
#include "fractint.h" /* for overlay stuff */
#include "fractype.h"
#include "prototyp.h"

#define FAR_RESERVE     8192L     /* amount of far mem we will leave avail. */
#define MAXRECT         1024      /* largest width of SaveRect/RestoreRect */

#define newx(size)     mem_alloc(size)
#define delete(block)

enum stored_at_values
   {
   NOWHERE,
   DISK,
   MEMORY
   } ;

/* borrowed from LORENZ.C */
struct affine
{
   /* weird order so a,b,e and c,d,f are vectors */
   double a;
   double b;
   double e;
   double c;
   double d;
   double f;
};

extern int  setup_convert_to_screen(struct affine *);

int show_numbers =0;              /* toggle for display of coords */
int stored_at;
char far *memory = NULL;            /* pointer to saved rectangle */
FILE *file;
int windows = 0;               /* windows management system */

extern char scrnfile[];   /* file where screen portion is */
				  /* stored */
extern BYTE      *line_buff;   /* must be alloced!!! */
extern int           sxdots;             /* width of physical screen         */
extern int           sydots;             /* depth of physical screen         */
extern int           sxoffs;             /* start of logical screen          */
extern int           syoffs;             /* start of logical screen          */
extern int           strlocn[];          /* 10K buffer to store classes in   */
extern int           colors;             /* # colors avail.                  */
extern int       using_jiim;
extern int       viewwindow;     /* 0 for full screen, 1 for window */
extern float     viewreduction;  /* window auto-sizing */
extern int       viewcrop;               /* nonzero to crop default coords */
extern float     finalaspectratio;              /* for view shape and rotation */
extern int       viewxdots,viewydots;   /* explicit view sizing */
extern double    dxsize, dysize;
extern int       xdots;                  /* width of logical screen          */
extern int       ydots;                  /* depth of logical screen          */
extern double    xxmax,xxmin;    /* limits of the "window"       */
extern double    yymax,yymin;    /* on the z-plane               */
extern int           color_bright;       /* brightest color in palette       */
extern int           color_dark;         /* darkest color in palette         */
extern int           color_medium;       /* nearest to medbright gray color  */
extern int           lookatmouse;        /* mouse mode for getakey(), etc    */
extern int           maxit;                  /* try this many iterations */
extern int           fractype;       /* fractal type */
extern _CMPLX     old,new,init;
extern double tempsqrx,tempsqry;
static int xc, yc;                       /* corners of the window */
static int xd, yd;                       /* dots in the window    */
extern void displayc(int x, int y, int fg, int bg, int ch);
extern float screenaspect;
double xcjul = BIG;
double ycjul = BIG;
extern int hasinverse;

void displays(int x, int y, int fg, int bg, char *str, int len)
{
   int i;
   for(i=0;i<len; i++)
      displayc(x+i*8, y, fg, bg, str[i]);
}

/* circle routines from Dr. Dobbs June 1990 */
int xbase, ybase;
unsigned xAspect, yAspect;

void SetAspect(double aspect)
{
   xAspect = 0;
   yAspect = 0;
   aspect = fabs(aspect);
   if (aspect != 1.0)
      if (aspect > 1.0)
	 yAspect = 65536.0 / aspect;
      else
	 xAspect = 65536.0 * aspect;
}

void _fastcall c_putcolor(int x, int y, int color)
   {
   /* avoid writing outside window */
   if ( x < xc || y < yc || x >= xc + xd || y >= yc + yd )
      return ;
   if(y >= sydots - show_numbers) /* avoid overwriting coords */
      return;
   if(windows == 2) /* avoid overwriting fractal */
      if (0 <= x && x < xdots && 0 <= y && y < ydots)
	 return;
   putcolor(x, y, color);
   }


int  c_getcolor(int x, int y)
   {
   /* avoid reading outside window */
   if ( x < xc || y < yc || x >= xc + xd || y >= yc + yd )
      return 1000;
   if(y >= sydots - show_numbers) /* avoid overreading coords */
      return 1000;
   if(windows == 2) /* avoid overreading fractal */
      if (0 <= x && x < xdots && 0 <= y && y < ydots)
	 return 1000;
   return getcolor(x, y);
   }

void circleplot(int x, int y, int color)
{
   if (xAspect == 0)
      if (yAspect == 0)
	 c_putcolor(x+xbase, y+ybase,color);
      else
	 c_putcolor(x+xbase, (short)(ybase + (((long) y * (long) yAspect) >> 16)),color);
   else
      c_putcolor((int)(xbase + (((long) x * (long) xAspect) >> 16)), y+ybase, color);
}

void plot8(int x, int y, int color)
{
   circleplot(x,y,color);
   circleplot(-x,y,color);
   circleplot(x,-y,color);
   circleplot(-x,-y,color);
   circleplot(y,x,color);
   circleplot(-y,x,color);
   circleplot(y,-x,color);
   circleplot(-y,-x,color);
}

void circle(int radius, int color)
{
   int x,y,sum;

   x = 0;
   y = radius << 1;
   sum = 0;

   while (x <= y)
   {
      if ( !(x & 1) )   /* plot if x is even */
	 plot8( x >> 1, (y+1) >> 1, color);
      sum += (x << 1) + 1;
      x++;
      if (sum > 0)
      {
	 sum -= (y << 1) - 1;
	 y--;
      }
   }
}


/*
 * MIIM section:
 *
 * Global variables and service functions used for computing
 * MIIM Julias will be grouped here (and shared by code in LORENZ.C)
 *
 */


 long   ListFront, ListBack, ListSize;  /* head, tail, size of MIIM Queue */
 long   lsize, lmax;			/* how many in queue (now, ever) */
 int    maxhits = 1;
 int    OKtoMIIM;
 int    SecretExperimentalMode;
 float  luckyx = 0, luckyy = 0;
 extern int debugflag;
 extern int           bitshift;         /* bit shift for fudge */
 extern long          fudge;            /* fudge factor (2**n) */


long lsqrt(long f)
{
    int N;
    unsigned long y0, z;
    static long a=0, b=0, c=0;                  /* constant factors */

    if (f == 0)
	return f;
    if (f <  0)
	return 0;

    if (a==0)                                   /* one-time compute consts */
    {
	a = fudge * .41731;
	b = fudge * .59016;
	c = fudge * .7071067811;
    }

    N  = 0;
    while (f & 0xff000000L)                     /* shift arg f into the */
    {                                           /* range: 0.5 <= f < 1  */
	N++;
	f /= 2;
    }
    while (!(f & 0xff800000L))
    {
	N--;
	f *= 2;
    }

    y0 = a + multiply(b, f,  bitshift);         /* Newton's approximation */

    z  = y0 + divide (f, y0, bitshift);
    y0 = (z>>2) + divide(f, z,  bitshift);

    if (N % 2)
    {
	N++;
	y0 = multiply(c,y0, bitshift);
    }
    N /= 2;
    if (N >= 0)
	return y0 <<  N;                        /* correct for shift above */
    else
	return y0 >> -N;
}

#define SinCosFudge 0x10000L


LCMPLX
ComplexSqrtLong(long x, long y)
{
   double    mag, theta;
   long      maglong, thetalong;
   LCMPLX    result;

#if 1
   mag       = sqrt(sqrt(((double) multiply(x,x,bitshift))/fudge +
			 ((double) multiply(y,y,bitshift))/ fudge));
   maglong   = mag * fudge;
#else
   maglong   = lsqrt(lsqrt(multiply(x,x,bitshift)+multiply(y,y,bitshift)));
#endif
   theta     = atan2((double) y/fudge, (double) x/fudge)/2;
   thetalong = theta * SinCosFudge;
   SinCos086(thetalong, &result.y, &result.x);
   result.x  = multiply(result.x << (bitshift - 16), maglong, bitshift);
   result.y  = multiply(result.y << (bitshift - 16), maglong, bitshift);
   return result;
}

_CMPLX
ComplexSqrtFloat(double x, double y)
{
   double mag   = sqrt(sqrt(x*x + y*y));
   double theta = atan2(y, x) / 2;
   _CMPLX  result;

   FPUsincos(&theta, &result.y, &result.x);
   result.x *= mag;
   result.y *= mag;
   return result;
}


extern char dstack[];

static void fillrect(int x, int y, int width, int depth, int color)
{
   /* fast version of fillrect */
   if(hasinverse == 0)
      return;
   memset(dstack, color % colors, width);
   while (depth-- > 0)
   {
      if(keypressed()) /* we could do this less often when in fast modes */
         return;
      putrow(x, y++, width, dstack);
   }
}

/*
 * Queue/Stack Section:
 *
 * Defines a buffer that can be used as a FIFO queue or LIFO stack.
 */

int
QueueEmpty()		/* True if NO points remain in queue */
{
   return (ListFront == ListBack);
}

int
QueueFull()		/* True if room for NO more points in queue */
{
   return (((ListFront + 1) % ListSize) == ListBack);
}

int
QueueFullAlmost()	/* True if room for ONE more point in queue */
{
   return (((ListFront + 2) % ListSize) == ListBack);
}

void
ClearQueue()
{
   ListFront = ListBack = lsize = lmax = 0;
}


/*
 * Queue functions for MIIM julia:
 * move to JIIM.C when done
 */

extern long  ListSize, ListFront, ListBack, lsize, lmax;
extern float luckyx, luckyy;

int Init_Queue(unsigned long request)
{
   extern int _fastcall common_startdisk(long, long, int);
   extern unsigned int xmmlongest();
   extern unsigned int xmmreallocate(unsigned int, unsigned int);
   extern int dotmode;
   unsigned int        largest;


   if (dotmode == 11)
   {
      static char far *nono = "Don't try this in disk video mode, kids...\n";
      stopmsg(0, nono);
      ListSize = 0;
      return 0;
   }

#if 0
   if (xmmquery() && debugflag != 420)	/* use LARGEST extended mem */
      if ((largest = xmmlongest()) > request / 128)
	 request   = (unsigned long) largest * 128L;
#endif

   for (ListSize = request; ListSize > 1024; ListSize -= 512)
      switch (common_startdisk(ListSize * 8, 1, 256))
      {
	 case 0:                        /* success */
	    ListFront = ListBack = 0;
	    lsize = lmax = 0;
	    return 1;
	 case -1:
	    continue;			/* try smaller queue size */
	 case -2:
	    ListSize = 0;               /* cancelled by user      */
	    return 0;
      }

   /* failed to get memory for MIIM Queue */
   ListSize = 0;
   return 0;
}

void
Free_Queue()
{
   enddisk();
   ListFront = ListBack = ListSize = lsize = lmax = 0;
}

extern int ToMemDisk  (long, int, void far *);
extern int FromMemDisk(long, int, void far *);

int
PushLong(long x, long y)
{
   if (((ListFront + 1) % ListSize) != ListBack)
   {
      if (ToMemDisk(8*ListFront, sizeof(x), &x) &&
	  ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
      {
	 ListFront = (ListFront + 1) % ListSize;
	 if (++lsize > lmax)
	 {
	    lmax   = lsize;
	    luckyx = x;
	    luckyy = y;
	 }
	 return 1;
      }
   }
   return 0;			/* fail */
}

int
PushFloat(float x, float y)
{
   if (((ListFront + 1) % ListSize) != ListBack)
   {
      if (ToMemDisk(8*ListFront, sizeof(x), &x) &&
	  ToMemDisk(8*ListFront +sizeof(x), sizeof(y), &y))
      {
	 ListFront = (ListFront + 1) % ListSize;
	 if (++lsize > lmax)
	 {
	    lmax   = lsize;
	    luckyx = x;
	    luckyy = y;
	 }
	 return 1;
      }
   }
   return 0;			/* fail */
}

_CMPLX
PopFloat()
{
   _CMPLX pop;
   float  popx, popy;

   if (!QueueEmpty())
   {
      ListFront--;
      if (ListFront < 0)
	  ListFront = ListSize - 1;
      if (FromMemDisk(8*ListFront, sizeof(popx), &popx) &&
	  FromMemDisk(8*ListFront +sizeof(popx), sizeof(popy), &popy))
      {
	 pop.x = popx;
	 pop.y = popy;
	 --lsize;
      }
      return pop;
   }
   pop.x = 0;
   pop.y = 0;
   return pop;
}

LCMPLX
PopLong()
{
   LCMPLX pop;

   if (!QueueEmpty())
   {
      ListFront--;
      if (ListFront < 0)
	  ListFront = ListSize - 1;
      if (FromMemDisk(8*ListFront, sizeof(pop.x), &pop.x) &&
	  FromMemDisk(8*ListFront +sizeof(pop.x), sizeof(pop.y), &pop.y))
	 --lsize;
      return pop;
   }
   pop.x = 0;
   pop.y = 0;
   return pop;
}

int
EnQueueFloat(float x, float y)
{
   return PushFloat(x, y);
}

int
EnQueueLong(long x, long y)
{
   return PushLong(x, y);
}

_CMPLX
DeQueueFloat()
{
   _CMPLX out;
   float outx, outy;

   if (ListBack != ListFront)
   {
      if (FromMemDisk(8*ListBack, sizeof(outx), &outx) &&
	  FromMemDisk(8*ListBack +sizeof(outx), sizeof(outy), &outy))
      {
	 ListBack = (ListBack + 1) % ListSize;
	 out.x = outx;
	 out.y = outy;
	 lsize--;
      }
      return out;
   }
   out.x = 0;
   out.y = 0;
   return out;
}

LCMPLX
DeQueueLong()
{
   LCMPLX out;
   out.x = 0;
   out.y = 0;

   if (ListBack != ListFront)
   {
      if (FromMemDisk(8*ListBack, sizeof(out.x), &out.x) &&
	  FromMemDisk(8*ListBack +sizeof(out.x), sizeof(out.y), &out.y))
      {
	 ListBack = (ListBack + 1) % ListSize;
	 lsize--;
      }
      return out;
   }
   out.x = 0;
   out.y = 0;
   return out;
}



/*
 * End MIIM section;
 */



static BOOLEAN MemoryAlloc(long size)
{
   char far *temp;
   
   if (debugflag == 420)
      return(FALSE);
   temp = (char far *)farmemalloc(FAR_RESERVE);   /* minimum free space */

   if (temp == NULL)
   {
      stored_at = NOWHERE;
      return (FALSE);   /* can't do it */
   }

   memory = (char far *)farmemalloc( size );
   farmemfree(temp);

   if ( memory == NULL )
   {
      stored_at = NOWHERE;
      return (FALSE);
   }
   else
   {
      stored_at = MEMORY;
      return (TRUE);
   }
}


static void SaveRect(int x, int y, int width, int depth)
{
   char buff[MAXRECT];
   int  yoff;
   if(hasinverse == 0)
      return;
   /* first, do any de-allocationg */

   switch( stored_at )
   {
   case NOWHERE:
      break;

   case DISK:
      break;

   case MEMORY:
      if (memory != NULL)
      {
	 farmemfree(memory);
      }
      memory = NULL;
      break;
   }

   /* allocate space and store the rect */

   memset(dstack, color_dark, width);
   if ( MemoryAlloc( (long)width*(long)depth) )
   {
      char far  *ptr = memory;
      char far  *bufptr = buff; /* MSC needs this indirection to get it right */

      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
      {
	 getrow(x, y+yoff, width, buff);
	 putrow(x, y+yoff, width, dstack);
	 movedata(FP_SEG(bufptr), FP_OFF(bufptr), FP_SEG(ptr), FP_OFF(ptr), width);
	 ptr = (char far *)normalize(ptr+width);
      }
      Cursor_Show();
   }

   else /* to disk */
   {
      stored_at = DISK;

      if ( file == NULL )
      {
	 file = fopen(scrnfile, "w+b");
	 if (file == NULL)
	 {
	    stored_at = NOWHERE;
	    buzzer(3);
	    return ;
	 }
      }

      rewind(file);
      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
      {
	 getrow(x, y+yoff, width, buff);
	 putrow(x, y+yoff, width, dstack);
	 if ( fwrite(buff, width, 1, file) != 1 )
	 {
	    buzzer(3);
	    break;
	 }
      }
      Cursor_Show();
   }
}


static void RestoreRect(x, y, width, depth)
{
   char buff[MAXRECT];
   int  yoff;
   if(hasinverse == 0)
      return;

   switch ( stored_at )
   {
   case DISK:
      rewind(file);
      Cursor_Hide();
      for (yoff=0; yoff<depth; yoff++)
      {
	 if ( fread(buff, width, 1, file) != 1 )
	 {
	    buzzer(3);
	    break;
	 }
	 putrow(x, y+yoff, width, buff);
      }
      Cursor_Show();
      break;

   case MEMORY:
      {
	 char far  *ptr = memory;
	 char far  *bufptr = buff; /* MSC needs this indirection to get it right */

	 Cursor_Hide();
	 for (yoff=0; yoff<depth; yoff++)
	 {
	    movedata(FP_SEG(ptr), FP_OFF(ptr), FP_SEG(bufptr), FP_OFF(bufptr), width);
	    putrow(x, y+yoff, width, buff);
	    ptr = (char far *)normalize(ptr+width);
	 }
	 Cursor_Show();
	 break;
      }

   case NOWHERE:
      break;
   } /* switch */
}

/*
 * interface to FRACTINT
 */
int jfractype;

extern int row, col;
extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;
extern int integerfractal;
extern long far *lx0, far *ly0;
extern long far *lx1, far *ly1;
extern int bitshift;
extern int helpmode;

/* the following macros and function call the setup, per_pixel, and orbit
   routines and calculate an orbit at row 0 column 0. Have to save and 
   restore the first elements of dx0 ... dy1 as well as row and col */

#define PER_IMAGE fractalspecific[o_fractype].per_image
#define PER_PIXEL fractalspecific[o_fractype].per_pixel
#define ORBITCALC   fractalspecific[o_fractype].orbitcalc

int do_fractal_routines(double cr, double ci, int (*func)())
{
   int ret;
   int old_row, old_col;
   double old_dx0, old_dx1, old_dy0, old_dy1;
   old_dx0 = *dx0; old_dx1 = *dx1;
   old_dy0 = *dy0; old_dy1 = *dy1;
   old_row = row;  old_col = col;
   row = col = 0;
   *dx0 = cr; *dy0 = ci; *dx1 = *dy1 = 0.0;
   ret = func();
   *dx0 = old_dx0; *dx1 = old_dx1;
   *dy0 = old_dy0; *dy1 = old_dy1;
   row = old_row;  col = old_col;
   return(ret);
}


void Jiim(int which)         /* called by fractint */
{
   struct affine cvt;
   int exact = 0;
   int oldhelpmode;
   int count = 0;            /* coloring julia */
   static int mode = 0;      /* point, circle, ... */
   int       oldlookatmouse = lookatmouse;
   double cr, ci, r;
   int xfactor, yfactor;             /* aspect ratio          */

   int xoff, yoff;                   /* center of the window  */
   int x, y;
   int still, kbdchar;
   int xcrsr,ycrsr;     /* coords of the cursor / offsets to move it  */
   int iter;
   int color;
   float zoom;
   int oldsxoffs, oldsyoffs;
   int savehasinverse;
   int (*oldcalctype)();
   extern int (*calctype)();
   int o_fractype;
   int old_x, old_y;
   double aspect;
   static int randir = 0;
   static int rancnt = 0;
   static _CMPLX SaveC = {-3000.0, -3000.0};
   int actively_computing = 1;
   int first_time = 1;

   ENTER_OVLY(OVLY_ROTATE);
   /* must use standard fractal and have a float variant */
   if(fractalspecific[fractype].calctype != StandardFractal ||
	 (fractalspecific[fractype].isinteger &&
	 fractalspecific[fractype].tofloat == NOFRACTAL))
   {
       EXIT_OVLY;
       return;
   }
   oldhelpmode = helpmode;
   if(which == JIIM)
      helpmode = HELP_JIIM;
   else
   {
      helpmode = HELP_ORBITS;
      hasinverse = 1;
   }
   if(fractalspecific[fractype].isinteger)
      o_fractype = fractalspecific[fractype].tofloat;
   else
      o_fractype = fractype;
   oldsxoffs = sxoffs;
   oldsyoffs = syoffs;
   oldcalctype = calctype;
   show_numbers = 0;
   using_jiim = 1;
   mem_init(strlocn, 10*1024);
   line_buff = newx(max(sxdots,sydots));
   aspect = ((double)xdots*3)/((double)ydots*4);  /* assumes 4:3 */
	 actively_computing = 1;
   SetAspect(aspect);
   lookatmouse = 3;
   Cursor_Construct();

/*
 * MIIM code:
 * Grab far memory for Queue/Stack before SaveRect gets it.
 */
   OKtoMIIM  = 0;
   if (which == JIIM && debugflag != 300)
      OKtoMIIM = Init_Queue((long)8*1024); /* Queue Set-up Successful? */

   maxhits = 1;
   if (which == ORBIT)
      plot = c_putcolor;     		/* for line with clipping */

/*
 * end MIIM code.
 */


   if(sxoffs != 0 || syoffs != 0) /* we're in view windows */
   {
      savehasinverse = hasinverse;
      hasinverse = 1;
      SaveRect(0,0,xdots,ydots);
      sxoffs = 0;
      syoffs = 0;
      RestoreRect(0,0,xdots,ydots);
      hasinverse = savehasinverse;
   }

   if(which == ORBIT)
      do_fractal_routines(cr, ci,PER_IMAGE);
   else
      color = color_bright;

   oldhelpmode = helpmode;
   if(which == JIIM)
      helpmode = HELP_JIIM;
   else
      helpmode = HELP_ORBITS;

   if(xdots == sxdots || ydots == sydots ||
       sxdots-xdots < sxdots/3 ||
       sydots-ydots < sydots/3 ||
       xdots >= MAXRECT )
   {
      /* this mode puts orbit/julia in an overlapping window 1/3 the size of
	 the physical screen */
      windows = 0; /* full screen or large view window */
      xd = sxdots / 3;
      yd = sydots / 3;
      xc = xd * 2;
      yc = yd * 2;
      xoff = xd * 5 / 2;
      yoff = yd * 5 / 2;
   }
   else if(xdots > sxdots/3 && ydots > sydots/3)
   {
      /* Julia/orbit and fractal don't overlap */
      windows = 1;
      xd = sxdots-xdots;
      yd = sydots-ydots;
      xc = xdots;
      yc = ydots;
      xoff = xc + xd/2;
      yoff = yc + yd/2;

   }
   else
   {
      /* Julia/orbit takes whole screen */
      windows = 2;
      xd = sxdots;
      yd = sydots;
      xc = 0;
      yc = 0;
      xoff = xd/2;
      yoff = yd/2;
   }

   xfactor = xd/5.33;
   yfactor = -yd/4;

   if(windows == 0)
      SaveRect(xc,yc,xd,yd);
   else if(windows == 2)  /* leave the fractal */
   {
      fillrect(xdots, yc, xd-xdots, yd, color_dark);
      fillrect(xc   , ydots, xdots, yd-ydots, color_dark);
   }
   else  /* blank whole window */
      fillrect(xc, yc, xd, yd, color_dark);

   setup_convert_to_screen(&cvt);

   /* reuse last location if inside window */
   xcrsr = cvt.a*SaveC.x + cvt.b*SaveC.y + cvt.e + .5;
   ycrsr = cvt.c*SaveC.x + cvt.d*SaveC.y + cvt.f + .5;
   if(xcrsr < 0 || xcrsr >= xdots ||
      ycrsr < 0 || ycrsr >= ydots)
   {
      cr = (xxmax + xxmin) / 2.0;
      ci = (yymax + yymin) / 2.0;
   }
   else
   {
      cr = SaveC.x;
      ci = SaveC.y;
   }

   old_x = old_y = -1;

   xcrsr = cvt.a*cr + cvt.b*ci + cvt.e + .5;
   ycrsr = cvt.c*cr + cvt.d*ci + cvt.f + .5;

   Cursor_SetPos(xcrsr, ycrsr);
   Cursor_Show();
   color = color_bright;

   iter = 1;
   still = 1;
   zoom = 1;

#ifdef XFRACT
   Cursor_StartMouseTracking();
#endif

   while (still)
   {
      int dxcrsr, dycrsr;
      if (actively_computing) {
	  Cursor_CheckBlink();
      } else {
	  Cursor_WaitKey();
      }
      if(keypressed() || first_time) /* prevent burning up UNIX CPU */
      {
	 first_time = 0;
	 while(keypressed())
	 {
	    Cursor_WaitKey();
	    kbdchar = getakey();

	    dxcrsr = dycrsr = 0;
	    xcjul = BIG;
	    ycjul = BIG;
	    switch (kbdchar)
	    {
            case 1143:    /* ctrl - keypad 5 */
	    case 1076:    /* keypad 5        */
	       break;     /* do nothing */
	    case CTL_PAGE_UP:
	       dxcrsr = 4;
	       dycrsr = -4;
	       break;
	    case CTL_PAGE_DOWN:
	       dxcrsr = 4;
	       dycrsr = 4;
	       break;
	    case CTL_HOME:
	       dxcrsr = -4;
	       dycrsr = -4;
	       break;
	    case CTL_END:
	       dxcrsr = -4;
	       dycrsr = 4;
	       break;
	    case PAGE_UP:
	       dxcrsr = 1;
	       dycrsr = -1;
	       break;
	    case PAGE_DOWN:
	       dxcrsr = 1;
	       dycrsr = 1;
	       break;
	    case HOME:
	       dxcrsr = -1;
	       dycrsr = -1;
	       break;
	    case END:
	       dxcrsr = -1;
	       dycrsr = 1;
	       break;
	    case UP_ARROW:
	       dycrsr = -1;
	       break;
	    case DOWN_ARROW:
	       dycrsr = 1;
	       break;
	    case LEFT_ARROW:
	       dxcrsr = -1;
	       break;
	    case RIGHT_ARROW:
	       dxcrsr = 1;
	       break;
	    case UP_ARROW_2:
	       dycrsr = -4;
	       break;
	    case DOWN_ARROW_2:
	       dycrsr = 4;
	       break;
	    case LEFT_ARROW_2:
	       dxcrsr = -4;
	       break;
	    case RIGHT_ARROW_2:
	       dxcrsr = 4;
	       break;
	    case 'z':
	    case 'Z':
	       zoom = 1.0;
	       break;
	    case '<':
	    case ',':
	       zoom /= 1.15;
	       break;
	    case '>':
	    case '.':
	       zoom *= 1.15;
	       break;
	    case SPACE:
	       xcjul = cr;
	       ycjul = ci;
	       goto finish;
	       break;
	    case 'c':   /* circle toggle */
	    case 'C':   /* circle toggle */
	       mode = mode ^ 1;
	       break;
	    case 'l':
	    case 'L':
	       mode = mode ^ 2;
	       break;
	    case 'n':
	    case 'N':
	       show_numbers = 8 - show_numbers;
	       if(windows == 0 && show_numbers == 0)
	       {
		  Cursor_Hide();
		  cleartempmsg();
		  Cursor_Show();
	       }
	       break;
	    case 'p':
	    case 'P':
	       get_a_number(&cr,&ci);
               exact = 1;
               xcrsr = cvt.a*cr + cvt.b*ci + cvt.e + .5;
               ycrsr = cvt.c*cr + cvt.d*ci + cvt.f + .5;
               dxcrsr = dycrsr = 0;
	       break;
	    case 'h':   /* hide fractal toggle */
	    case 'H':   /* hide fractal toggle */
	       if(windows == 2)
		  windows = 3;
	       else if(windows == 3 && xd == sxdots)
	       {
		  RestoreRect(0, 0, xdots, ydots);
		  windows = 2;
	       }
	       break;
#ifdef XFRACT
	    case ENTER:
		break;
#endif
	    case '0':
	    case '1':
	    case '2':
/*	    case '3': */  /* don't use '3', it's already meaningful */
	    case '4':
	    case '5':
	    case '6':
	    case '7':
	    case '8':
	    case '9':
	       if (which == JIIM)
	       {
		  SecretExperimentalMode = kbdchar - '0';
		  break;
	       }
	    default:
	       still = 0;
	    }  /* switch */
	    if(kbdchar == 's' || kbdchar == 'S')
	       goto finish;
            if(dxcrsr > 0 || dycrsr > 0)
               exact = 0;
            xcrsr += dxcrsr;
            ycrsr += dycrsr;
            
	    /* keep cursor in logical screen */
	   if(xcrsr >= xdots)
	      xcrsr = xdots -1, exact = 0;
	   if(ycrsr >= ydots)
	      ycrsr = ydots -1, exact = 0;
	   if(xcrsr < 0)
	      xcrsr = 0, exact = 0;
	   if(ycrsr < 0)
	      ycrsr = 0, exact = 0;
	    
            Cursor_SetPos(xcrsr,ycrsr);
	 }  /* end while (keypressed) */

         if(exact == 0)
         {
	    if(integerfractal)
	    {
	       cr = lx0[xcrsr]+lx1[ycrsr];  /* supports rotated zoom boxes! */
	       ci = ly0[ycrsr]+ly1[xcrsr];
	       cr /= (1L<<bitshift);
	       ci /= (1L<<bitshift);
	    }
	    else
	    {
	       cr = dx0[xcrsr]+dx1[ycrsr];
	       ci = dy0[ycrsr]+dy1[xcrsr];
	    }
	 }
	 actively_computing = 1;
	 if(show_numbers) /* write coordinates on screen */
	 {
	    char str[80];
	    sprintf(str,"x=%16.14f y=%16.14f   ",cr,ci);
	    str[40] = 0;
	    if(windows == 0)
	    {
	       Cursor_Hide();
	       actively_computing = 1;
	       showtempmsg(str);
	       Cursor_Show();
	    }
	    else
	       displays(5, sydots-show_numbers, WHITE, BLACK, str,strlen(str));
	 }
	 iter = 1;
	 old.x = old.y = 0;
	 SaveC.x = init.x =  cr;
	 SaveC.y = init.y =  ci;
	 old_x = old_y = -1;
/*
 * MIIM code:
 * compute fixed points and use them as starting points of JIIM
 */
	 if (which == JIIM && OKtoMIIM)
	 {
	    _CMPLX f1, f2, Sqrt;	/* Fixed points of Julia */

	    Sqrt = ComplexSqrtFloat(1 - 4 * cr, -4 * ci);
	    f1.x = (1 + Sqrt.x) / 2;
	    f2.x = (1 - Sqrt.x) / 2;
	    f1.y =  Sqrt.y / 2;
	    f2.y = -Sqrt.y / 2;

	    ClearQueue();
	    maxhits = 1;
	    EnQueueFloat(f1.x, f1.y);
	    EnQueueFloat(f2.x, f2.y);
	 }
/*
 * End MIIM code.
 */
	 if(which == ORBIT)
           do_fractal_routines(cr, ci,PER_PIXEL);
	 /* move window if bumped */
	 if(windows==0 && xcrsr>xc && xcrsr < xc+xd && ycrsr>yc && ycrsr < yc+yd)
	 {
	    RestoreRect(xc,yc,xd,yd);
	    if (xc == xd*2)
	       xc = 2;
	    else
	       xc = xd*2;
	    xoff = xc + xd /  2;
	    SaveRect(xc,yc,xd,yd);
	 }
	 if(windows == 2)
	 {
	    fillrect(xdots, yc, xd-xdots, yd-show_numbers, color_dark);
	    fillrect(xc   , ydots, xdots, yd-ydots-show_numbers, color_dark);
	 }
	 else
	    fillrect(xc, yc, xd, yd, color_dark);

      } /* end if (keypressed) */

      if(which == JIIM)
      {
         if(hasinverse == 0)
            continue;
/*
 * MIIM code:
 * If we have MIIM queue allocated, then use MIIM method.
 */
	 if (OKtoMIIM)
	 {
	    if (QueueEmpty())
	    {
	       if (maxhits < colors - 1 && maxhits < 5 &&
		  (luckyx != 0.0 || luckyy != 0.0))
	       {
		  int i;

		  lsize  = lmax   = 0;
		  old.x  = new.x  = luckyx;
		  old.y  = new.y  = luckyy;
		  luckyx = luckyy = 0.0;
		  for (i=0; i<199; i++)
		  {
		     old = ComplexSqrtFloat(old.x - cr, old.y - ci);
		     new = ComplexSqrtFloat(new.x - cr, new.y - ci);
		     EnQueueFloat( new.x,  new.y);
		     EnQueueFloat(-old.x, -old.y);
		  }
		  maxhits++;
	       }
	       else
		  continue;		/* loop while (still) */
	    }

	    old = DeQueueFloat();

#if 0 /* try a different new method */
	    if (lsize < (lmax / 8) && maxhits < 5)	/* NEW METHOD */
	       if (maxhits < colors - 1)
		   maxhits++;
#endif
	    x = old.x * xfactor * zoom + xoff;
	    y = old.y * yfactor * zoom + yoff;
	    color = c_getcolor(x, y);
	    if (color < maxhits)
	    {
	       c_putcolor(x, y, color + 1);
	       new = ComplexSqrtFloat(old.x - cr, old.y - ci);
	       EnQueueFloat( new.x,  new.y);
	       EnQueueFloat(-new.x, -new.y);
	    }
	 }
	 else
	 {
/*
 * end Msnyder code, commence if not MIIM code.
 */
	 old.x -= cr;
	 old.y -= ci;
	 r = old.x*old.x + old.y*old.y;
	 if(r > 10.0)
	 {
	     old.x = old.y = 0.0; /* avoids math error */
	     iter = 1;
	 }
	 iter++;
	 color = ((count++)>>5)&(colors-1); /* chg color every 32 pts */
	 if(color==0)
	  color = 1;

	 r = sqrt(old.x*old.x + old.y*old.y);
	 new.x = sqrt(fabs((r + old.x)/2));
	 if (old.y < 0)
	    new.x = -new.x;

	 new.y = sqrt(fabs((r - old.x)/2));


	 switch (SecretExperimentalMode) {
	    case 0:			/* unmodified random walk */
	    default:
		if (rand() % 2)
		{
		   new.x = -new.x;
		   new.y = -new.y;
		}
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		break;
	    case 1:		  	/* always go one direction */
		if (SaveC.y < 0)
		{
		   new.x = -new.x;
		   new.y = -new.y;
		}
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		break;
	    case 2:			/* go one dir, draw the other */
		if (SaveC.y < 0)
		{
		   new.x = -new.x;
		   new.y = -new.y;
		}
		x = -new.x * xfactor * zoom + xoff;
		y = -new.y * yfactor * zoom + yoff;
		break;
	    case 4:			/* go negative if max color */
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		if (c_getcolor(x, y) == colors - 1)
		{
		   new.x = -new.x;
		   new.y = -new.y;
		   x = new.x * xfactor * zoom + xoff;
		   y = new.y * yfactor * zoom + yoff;
		}
		break;
	    case 5:			/* go positive if max color */
		new.x = -new.x;
		new.y = -new.y;
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		if (c_getcolor(x, y) == colors - 1)
		{
		   x = new.x * xfactor * zoom + xoff;
		   y = new.y * yfactor * zoom + yoff;
		}
		break;
	    case 7:
		if (SaveC.y < 0)
		{
		   new.x = -new.x;
		   new.y = -new.y;
		}
		x = -new.x * xfactor * zoom + xoff;
		y = -new.y * yfactor * zoom + yoff;
		if(iter > 10)
		{
		   if(mode == 0)			/* pixels  */
		      c_putcolor(x, y, color);
		   else if (mode & 1)            /* circles */
		   {
		      xbase = x;
		      ybase = y;
		      circle((int)(zoom*(xd >> 1)/iter),color);
		   }
		   if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
		   {
		      draw_line(x, y, old_x, old_y, color);
		   }
		   old_x = x;
		   old_y = y;
		}
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		break;
	    case 8:			/* go in long zig zags */
		if (rancnt >= 300)
		    rancnt = -300;
		if (rancnt < 0)
		{
		    new.x = -new.x;
		    new.y = -new.y;
		}
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		break;
	    case 9:			/* "random run" */
		switch (randir) {
		    case 0:             /* go random direction for a while */
			if (rand() % 2)
			{
			    new.x = -new.x;
			    new.y = -new.y;
			}
			if (++rancnt > 1024)
			{
			    rancnt = 0;
			    if (rand() % 2)
				randir =  1;
			    else
				randir = -1;
			}
			break;
		    case 1:             /* now go negative dir for a while */
			new.x = -new.x;
			new.y = -new.y;
			/* fall through */
		    case -1:            /* now go positive dir for a while */
			if (++rancnt > 512)
			    randir = rancnt = 0;
			break;
		}
		x = new.x * xfactor * zoom + xoff;
		y = new.y * yfactor * zoom + yoff;
		break;
	 } /* end switch SecretMode (sorry about the indentation) */
	 } /* end if not MIIM */
      }
      else /* orbits */
      {
	 if(iter < maxit)
	 {
	    color = iter&(colors-1);
	    x = (old.x - init.x) * xfactor * 3 * zoom + xoff;
	    y = (old.y - init.y) * yfactor * 3 * zoom + yoff;
	    if(do_fractal_routines(cr, ci,ORBITCALC))
	       iter = maxit;
	    else
	       iter++;
	 }
	 else
	 {
	    x = y = -1;
	    actively_computing = 0;
	 }
      }
      if(which == ORBIT || iter > 10)
      {
	 if(mode == 0)			/* pixels  */
	    c_putcolor(x, y, color);
	 else if (mode & 1)            /* circles */
	 {
	    xbase = x;
	    ybase = y;
	    circle((int)(zoom*(xd >> 1)/iter),color);
	 }
	 if ((mode & 2) && x > 0 && y > 0 && old_x > 0 && old_y > 0)
	 {
	    draw_line(x, y, old_x, old_y, color);
	 }
	 old_x = x;
	 old_y = y;
      }
      old = new;
   } /* end while(still) */
finish:

/*
 * Msnyder code:
 * free MIIM queue
 */

   Free_Queue();
/*
 * end Msnyder code.
 */

   if(kbdchar != 's')
   {
      Cursor_Hide();
      if(windows == 0)
	 RestoreRect(xc,yc,xd,yd);
      else if(windows >= 2 )
      {
	 if(windows == 2)
	 {
	    fillrect(xdots, yc, xd-xdots, yd, color_dark);
	    fillrect(xc   , ydots, xdots, yd-ydots, color_dark);
	 }
	 else
	    fillrect(xc, yc, xd, yd, color_dark);
         if(windows == 3 && xd == sxdots) /* unhide */
         {
            RestoreRect(0, 0, xdots, ydots);
            windows = 2;
         }
	 Cursor_Hide();
	 savehasinverse = hasinverse;
	 hasinverse = 1;
	 SaveRect(0,0,xdots,ydots);
	 sxoffs = oldsxoffs;
	 syoffs = oldsyoffs;
	 RestoreRect(0,0,xdots,ydots);
	 hasinverse = savehasinverse;
      }
   }
   Cursor_Destroy();
#ifdef XFRACT
   Cursor_EndMouseTracking();
#endif
   delete(line_buff);
   if (memory)			/* done with memory, free it */
   {
      farmemfree(memory);
      memory = NULL;
   }

   lookatmouse = oldlookatmouse;
   using_jiim = 0;
   calctype = oldcalctype;

   helpmode = oldhelpmode;
   if(kbdchar == 's')
   {
      viewwindow = viewxdots = viewydots = 0;
      viewreduction = 4.2;
      viewcrop = 1;
      finalaspectratio = screenaspect;
      xdots = sxdots;
      ydots = sydots;
      dxsize = xdots - 1;
      dysize = ydots - 1;
      sxoffs = 0;
      syoffs = 0;
      freetempmsg();
   }
   else
      cleartempmsg();
   if (file != NULL)
      {
      fclose(file);
      file = NULL;
      remove(scrnfile);
      }
   show_numbers = 0;
   ungetakey(kbdchar);
   EXIT_OVLY;
}
  
