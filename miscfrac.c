/*

Miscellaneous fractal-specific code (formerly in CALCFRAC.C)


*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <float.h>
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

static void set_Plasma_palette(void);
static U16 _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb);
static void _fastcall subDivide(int x1,int y1,int x2,int y2);
static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur);
static void verhulst(void);
static void Bif_Period_Init(void);
static int  _fastcall Bif_Periodic(int);
static void set_Cellular_palette(void);

U16  (_fastcall *getpix)(int,int)  = (U16(_fastcall *)(int,int))getcolor;

extern int resuming;
extern char stdcalcmode;
extern int color, oldcolor, row, col, passes;
extern int ixstart, ixstop, iystart, iystop;
extern _CMPLX init,tmp,old,new,saved;
extern _CMPLX parm,parm2;

extern double far *dx0, far *dy0;
extern double far *dx1, far *dy1;
extern long far *lx0, far *ly0;
extern long far *lx1, far *ly1;
extern long delx, dely;
extern double deltaX, deltaY;
extern int sxoffs, syoffs, sxdots, sydots;
extern int xdots, ydots;
extern int maxit, inside, colors, andcolor, dotmode;
extern double param[];
extern int rflag, rseed;
extern int pot16bit, potflag;
extern int diskvideo;
extern int bitshift;
extern long fudge;
extern int show_orbit;
extern int periodicitycheck, integerfractal;
extern _LCMPLX linit;
extern _LCMPLX ltmp;
extern _LCMPLX lold,lnew,lparm,lparm2;
extern long ltempsqrx,ltempsqry;
extern double tempsqrx,tempsqry;
extern int overflow;
extern int kbdcount, max_kbdcount;
extern int reset_periodicity;
extern int calc_status;
extern int iterations, invert;
extern int save_release;
extern int LogFlag;
extern int (calctype());
extern int realcolor;
extern int nxtscreenflag;
extern double magnitude, rqlim, rqlim2, rqlim_save;
extern long lmagnitud, llimit, llimit2, lclosenuff, l16triglim;
extern int orbit_color, orbit_ptr, showdot;
extern int debugflag;

#ifndef XFRACT
extern char dstack[4096];
extern char boxy[4096];
#else
BYTE dstack[4096];
#endif

typedef void (_fastcall *PLOT)(int,int,int);

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

static int iparmx;      /* iparmx = parm.x * 16 */
static int shiftvalue;  /* shift based on #colors */
extern int max_colors;
static int recur1=1;
static int pcolors;
static int recur_level = 0;
U16 max_plasma;

/* returns a random 16 bit value that is never 0 */
U16 rand16()
{
   U16 value;
   value = rand15();
   value <<= 1;
   value += rand15()&1;
   if(value < 1)
      value = 1;
   return(value);
}

void _fastcall putpot(int x, int y, U16 color)
{
   if(color < 1)
      color = 1;
   putcolor(x, y, color >> 8 ? color >> 8 : 1);  /* don't write 0 */
   /* we don't write this if dotmode==11 because the above putcolor
         was already a "writedisk" in that case */
   if (dotmode != 11)
      writedisk(x+sxoffs,y+syoffs,color >> 8);    /* upper 8 bits */
   writedisk(x+sxoffs,y+sydots+syoffs,color&255); /* lower 8 bits */
}

U16 _fastcall getpot(int x, int y)
{
   U16 color;

   color = (U16)readdisk(x+sxoffs,y+syoffs);
   color = (color << 8) + (U16) readdisk(x+sxoffs,y+sydots+syoffs);
   return(color);
}


typedef struct palett
{
   BYTE red;
   BYTE green;
   BYTE blue;
}
Palettetype;

extern Palettetype dacbox[256];
static int plasma_check;                        /* to limit kbd checking */

static U16 _fastcall adjust(int xa,int ya,int x,int y,int xb,int yb)
{
   S32 pseudorandom;
   pseudorandom = ((S32)iparmx)*((rand15()-16383));
/*   pseudorandom = pseudorandom*(abs(xa-xb)+abs(ya-yb));*/
   pseudorandom = pseudorandom * recur1;
   pseudorandom = pseudorandom >> shiftvalue;
   pseudorandom = (((S32)getpix(xa,ya)+(S32)getpix(xb,yb)+1)>>1)+pseudorandom;
   if(max_plasma == 0)
   {
      if (pseudorandom >= pcolors)
         pseudorandom = pcolors-1;
   }
   else if (pseudorandom >= max_plasma)
      pseudorandom = max_plasma;
   if(pseudorandom < 1)
      pseudorandom = 1;
   plot(x,y,(U16)pseudorandom);
   return((U16)pseudorandom);
}


static int _fastcall new_subD (int x1,int y1,int x2,int y2, int recur)
{
   int x,y;
   int nx1;
   int nx;
   int ny1, ny;
   S32 i, v;

   struct sub {
      BYTE t; /* top of stack */
      int v[16]; /* subdivided value */
      BYTE r[16];  /* recursion level */
   };

   static struct sub subx, suby;

   /*
   recur1=1;
   for (i=1;i<=recur;i++)
      recur1 = recur1 * 2;
   recur1=320/recur1;
   */
   recur1 = 320L >> recur;
   suby.t = 2;
   ny   = suby.v[0] = y2;
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
         /*     1.  Create new entry at top of the stack  */
         /*     2.  Copy old top value to new top value.  */
         /*            This is largest y value.           */
         /*     3.  Smallest y is now old mid point       */
         /*     4.  Set new mid point recursion level     */
         /*     5.  New mid point value is average        */
         /*            of largest and smallest            */

         suby.t++;
         ny1  = suby.v[suby.t] = suby.v[suby.t-1];
         ny   = suby.v[suby.t-2];
         suby.r[suby.t] = suby.r[suby.t-1];
         y    = suby.v[suby.t-1]   = (ny1 + ny) >> 1;
         suby.r[suby.t-1]   = (int)max(suby.r[suby.t], suby.r[suby.t-2])+1;
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

         if ((i = getpix(nx, y)) == 0)
            i = adjust(nx,ny1,nx,y ,nx,ny);
         v = i;
         if ((i = getpix(x, ny)) == 0)
            i = adjust(nx1,ny,x ,ny,nx,ny);
         v += i;
         if(getpix(x,y) == 0)
         {
            if ((i = getpix(x, ny1)) == 0)
               i = adjust(nx1,ny1,x ,ny1,nx,ny1);
            v += i;
            if ((i = getpix(nx1, y)) == 0)
               i = adjust(nx1,ny1,nx1,y ,nx1,ny);
            v += i;
            plot(x,y,(U16)((v + 2) >> 2));
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
   S32 v,i;
   if ((++plasma_check & 0x7f) == 1)
      if(check_key())
      {
         plasma_check--;
         return;
      }
   if(x2-x1<2 && y2-y1<2)
      return;
   recur_level++;
   recur1 = 320L >> recur_level;

   x = (x1+x2)>>1;
   y = (y1+y2)>>1;
   if((v=getpix(x,y1)) == 0)
      v=adjust(x1,y1,x ,y1,x2,y1);
   i=v;
   if((v=getpix(x2,y)) == 0)
      v=adjust(x2,y1,x2,y ,x2,y2);
   i+=v;
   if((v=getpix(x,y2)) == 0)
      v=adjust(x1,y2,x ,y2,x2,y2);
   i+=v;
   if((v=getpix(x1,y)) == 0)
      v=adjust(x1,y1,x1,y ,x1,y2);
   i+=v;

   if(getpix(x,y) == 0)
      plot(x,y,(U16)((i+2)>>2));

   subDivide(x1,y1,x ,y);
   subDivide(x ,y1,x2,y);
   subDivide(x ,y ,x2,y2);
   subDivide(x1,y ,x ,y2);
   recur_level--;
}

int plasma()
{
   int i,k, n;
   U16 rnd[4];
   int OldPotFlag, OldPot16bit;

   plasma_check = 0;

   if(colors < 4) {
      static char far plasmamsg[]={
         "\
Plasma Clouds can currently only be run in a 4-or-more-color video\n\
mode (and color-cycled only on VGA adapters [or EGA adapters in their\n\
640x350x16 mode])."      };
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
   max_plasma = (U16)param[3];  /* max_plasma is used as a flag for potential */

   if(max_plasma != 0)
   {
      if (pot_startdisk() >= 0)
      {
	 max_plasma = (U16)(1L << 16) -1;
	 plot    = (PLOT)putpot;
	 getpix =  getpot;
	 OldPotFlag = potflag;
	 OldPot16bit = pot16bit;
      }
      else
      {
	 max_plasma = 0;	/* can't do potential (startdisk failed) */
	 param[3]   = 0;
	 plot    = putcolor;
       getpix  = (U16(_fastcall *)(int,int))getcolor;
      }
   }
   else
   {
      plot    = putcolor;
      getpix  = (U16(_fastcall *)(int,int))getcolor;
   }
   srand(rseed);
   if (!rflag) ++rseed;

   if (colors == 256)                   /* set the (256-color) palette */
      set_Plasma_palette();             /* skip this if < 256 colors */

   if (colors > 16)
      shiftvalue = 18;
   else
   {
      if (colors > 4)
         shiftvalue = 22;
      else
      {
         if (colors > 2)
            shiftvalue = 24;
         else
            shiftvalue = 25;
      }
   }
   if(max_plasma != 0)
      shiftvalue = 10;

   if(max_plasma == 0)
   {
      pcolors = min(colors, max_colors);
      for(n = 0; n < 4; n++)
         rnd[n] = 1+(((rand15()/pcolors)*(pcolors-1))>>(shiftvalue-11));
   }
   else
      for(n = 0; n < 4; n++)
         rnd[n] = rand16();
   plot(      0,      0,  rnd[0]);
   plot(xdots-1,      0,  rnd[1]);
   plot(xdots-1,ydots-1,  rnd[2]);
   plot(      0,ydots-1,  rnd[3]);

   recur_level = 0;
   if (param[1] == 0)
      subDivide(0,0,xdots-1,ydots-1);
   else
   {
      recur1 = i = k = 1;
      while(new_subD(0,0,xdots-1,ydots-1,i)==0)
      {
         k = k * 2;
         if (k  >(int)max(xdots-1,ydots-1))
            break;
         if (check_key())
         {
            n = 1;
            goto done;
         }
         i++;
      }
   }
   if (! check_key())
      n = 0;
   else
      n = 1;
   done:
   if(max_plasma != 0)
   {
      potflag = OldPotFlag;
      pot16bit = OldPot16bit;
   }
   plot    = putcolor;
   getpix  = (U16(_fastcall *)(int,int))getcolor;
   return(n);
}

static void set_Plasma_palette()
{
   extern char far *mapdacbox;
   static Palettetype Red    = {
      63, 0, 0      };
   static Palettetype Green  = {
      0,63, 0      };
   static Palettetype Blue   = {
      0, 0,63      };
   int i;

   if (mapdacbox) return;               /* map= specified */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;
   for(i=1;i<=85;i++)
   {
      dacbox[i].red       = (i*Green.red   + (86-i)*Blue.red)/85;
      dacbox[i].green     = (i*Green.green + (86-i)*Blue.green)/85;
      dacbox[i].blue      = (i*Green.blue  + (86-i)*Blue.blue)/85;

      dacbox[i+85].red    = (i*Red.red   + (86-i)*Green.red)/85;
      dacbox[i+85].green  = (i*Red.green + (86-i)*Green.green)/85;
      dacbox[i+85].blue   = (i*Red.blue  + (86-i)*Green.blue)/85;

      dacbox[i+170].red   = (i*Blue.red   + (86-i)*Red.red)/85;
      dacbox[i+170].green = (i*Blue.green + (86-i)*Red.green)/85;
      dacbox[i+170].blue  = (i*Blue.blue  + (86-i)*Red.blue)/85;
   }
   SetTgaColors();      /* TARGA 3 June 89  j mclain */
   spindac(0,1);
}


/***************** standalone engine for "diffusion" ********************/

#define RANDOM(x)  (rand()%(x))

/* This constant assumes that rand() returns a value from 0 to 32676 */
#define FOURPI  (long)(4*PI*(double)(1L << 16))

int diffusion()
{
   int xmax,ymax,xmin,ymin;     /* Current maximum coordinates */
   int border;   /* Distance between release point and fractal */
   int mode;     /* Determines diffusion type:  0 = central (classic) */
                 /*                             1 = falling particles */
                 /*                             2 = square cavity     */
   int i;
   double cosine,sine,angle;
   long lcosine,lsine;
   int x,y;
   float r, radius, f_tmp;
   extern char floatflag;

   if (diskvideo)
      notdiskmsg();

   bitshift = 16;
   fudge = 1L << 16;

   border = param[0];
   mode = param[1];
   if (mode > 2)
      mode=0;

   if (border <= 0)
      border = 10;

   srand(rseed);
   if (!rflag) ++rseed;

   if (mode == 0) {
      xmax = xdots / 2 + border;  /* Initial box */
      xmin = xdots / 2 - border;
      ymax = ydots / 2 + border;
      ymin = ydots / 2 - border;
   }
   if (mode == 1) {
      xmax = xdots / 2 + border;  /* Initial box */
      xmin = xdots / 2 - border;
      ymin = ydots - 20;
   }
   if (mode == 2) {
      if (xdots>ydots)
         radius = ydots - 5;
      else
         radius = xdots - 5;
   }
   if (resuming) /* restore worklist, if we can't the above will stay in place */
   {
      start_resume();
      if (mode != 2)
         get_resume(sizeof(int),&xmax,sizeof(int),&xmin,sizeof(int),&ymax,
             sizeof(int),&ymin,0);
      else
         get_resume(sizeof(int),&xmax,sizeof(int),&xmin,sizeof(int),&ymax,
             sizeof(int),&radius,0);
      end_resume();
   }

   if (mode==0)
      putcolor(xdots / 2, ydots / 2,RANDOM(colors-1)+1);  /* Seed point */

   if (mode==1)
      for (i=0;i<=xdots;i++)
         putcolor(i,ydots-1,colors-1);

   if (mode==2){
      if (xdots>ydots){
         for (i=0;i<ydots;i++){
            putcolor(xdots/2-ydots/2 , i , colors-1);
            putcolor(xdots/2+ydots/2 , i , colors-1);
            putcolor(xdots/2-ydots/2+i , 0 , colors-1);
            putcolor(xdots/2-ydots/2+i , ydots-1 , colors-1);
         }
      }
      else {
         for (i=0;i<xdots;i++){
            putcolor(0 , ydots/2-xdots/2+i , colors-1);
            putcolor(xdots-1 , ydots/2-xdots/2+i , colors-1);
            putcolor(i , ydots/2-xdots/2 , colors-1);
            putcolor(i , ydots/2+xdots/2 , colors-1);
         }
      }
   }

   while (1)
   {
      /* Release new point on circle just inside the box */

      if (mode==0)
      {
         if (floatflag)
         {
            angle=2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle,&sine,&cosine);
            x = cosine*(xmax-xmin) + xdots;
            y = sine  *(ymax-ymin) + ydots;
         }
         else
         {
            SinCos086(multiply((long)rand15(),FOURPI,16),&lsine,&lcosine);
            x = (lcosine*(long)(xmax-xmin) >> 16) + xdots;
            y = (lsine  *(long)(ymax-ymin) >> 16) + ydots;
         }

         x = x >> 1; /* divide by 2 */
         y = y >> 1;
      }
      if (mode==1)
      {
         y=ymin;
         x=RANDOM(xmax-xmin) + (xdots-xmax+xmin)/2;
      }
      if (mode==2)
      {
         if (floatflag)
         {
            angle=2*(double)rand()/(RAND_MAX/PI);
            FPUsincos(&angle,&sine,&cosine);
            x = cosine*radius + xdots;
            y = sine  *radius + ydots;
         }
         else
         {
            SinCos086(multiply((long)rand15(),FOURPI,16),&lsine,&lcosine);
            x = (lcosine*(long)(radius) >> 16) + xdots;
            y = (lsine  *(long)(radius) >> 16) + ydots;
         }
         x = x >> 1;
         y = y >> 1;
      }
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

         /* Make sure point is inside the box (if mode==0)*/
         if (mode==0){
            if (x==xmax)
               x--;
            else if (x==xmin)
               x++;
            if (y==ymax)
               y--;
            else if (y==ymin)
               y++;
         }

         if (mode==1)
         {
            if (x>xdots-2)
               x--;
            else if (x<1)
               x++;
            if (y<ymin)
               y++;
         }

         /* Take one random step */
         x += RANDOM(3) - 1;
         y += RANDOM(3) - 1;

         /* Check keyboard */
         if ((++plasma_check & 0x7f) == 1)
            if(check_key())
            {
               alloc_resume(20,1);
               if (mode!=2)
                  put_resume(sizeof(int),&xmax,sizeof(int),&xmin, sizeof(int),&ymax,
                      sizeof(int),&ymin,0);
               else
                  put_resume(sizeof(int),&xmax,sizeof(int),&xmin, sizeof(int),&ymax,
                      sizeof(int),&radius,0);

               plasma_check--;
               return 1;
            }

         /* Show the moving point */
         if (show_orbit)
            putcolor(x,y,RANDOM(colors-1)+1);

      }
      putcolor(x,y,RANDOM(colors-1)+1);

      /* Is point too close to the edge? */

      if (mode==0)
      {
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
      if (mode==1)
      {
         if (y < ymin+5)
            ymin = y - 5;
         if (ymin<2)
            return 0;
      }
      if (mode==2)
      {
         if (abs(x-xdots/2)<5 && abs(y-ydots/2)<5)
            return 0;

         r = (x-xdots/2)*(x-xdots/2)+(y-ydots/2)*(y-ydots/2);
         fSqrt14(r,f_tmp);
         r = 2 * f_tmp;
         if (r < radius)
            radius = r;
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
/* the routine that calculates the next bifurcation generation */
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

#define DEFAULTFILTER 1000     /* "Beauty of Fractals" recommends using 5000
			       (p.25), but that seems unnecessary. Can
			       override this value with a nonzero param1 */

#define SEED 0.66		/* starting value for population */

static int far *verhulst_array;
unsigned int filter_cycles;
static unsigned int half_time_check;
static long   lPopulation, lRate;
double Population,  Rate;
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
   array_size =  (iystop + 1) * sizeof(int); /* should be iystop + 1 */
   if ((verhulst_array = (int far *) farmemalloc(array_size)) == NULL)
   {
      static char far msg[]={"Insufficient free memory for calculation."};
      stopmsg(0,msg);
      return(-1);
   }

   LPI = PI * fudge;

   for (row = 0; row <= iystop; row++) /* should be iystop */
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

      for (row = iystop; row >= 0; row--) /* should be iystop & >=0 */
      {
	 int color;
	 color = verhulst_array[row];
	 if(color && mono)
	    color = inside;
	 else if((!color) && mono)
	    color = outside_x;
	 else if (color>=colors)
	    color = colors-1;
	 verhulst_array[row] = 0;
	 (*plot)(column,row,color); /* was row-1, but that's not right? */
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
	 pixel_row = iystop - (lPopulation - linit.y) / dely; /* iystop */
      else
	 pixel_row = iystop - (int)((Population - init.y) / deltaY);

      /* if it's visible on the screen, save it in the column array */
      if (pixel_row <= iystop) /* JCO 6/6/92 */
	 verhulst_array[ pixel_row ] ++;
      if (periodicitycheck && Bif_Periodic(counter))
      {
	 if (pixel_row <= iystop) /* JCO 6/6/92 */
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
#ifdef XFRACT
int BifurcLambda() /* Used by lyanupov */
  {
    Population = Rate * Population * (1 - Population);
    return (fabs(Population) > BIG);
  }
#endif

/* Modified formulas below to generalize bifurcations. JCO 7/3/92 */

#define LCMPLXtrig0(arg,out) Arg1->l = (arg); ltrig0(); (out)=Arg1->l
#define  CMPLXtrig0(arg,out) Arg1->d = (arg); dtrig0(); (out)=Arg1->d

int BifurcVerhulstTrig()
  {
/*  Population = Pop + Rate * fn(Pop) * (1 - fn(Pop)) */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
  }

int LongBifurcVerhulstTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation += multiply(lRate,ltmp.y,bitshift);
    return (overflow);
#endif
  }

int BifurcStewartTrig()
  {
/*  Population = (Rate * fn(Population) * fn(Population)) - 1.0 */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = (Rate * tmp.x * tmp.x) - 1.0;
    return (fabs(Population) > BIG);
  }

int LongBifurcStewartTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation = multiply(lPopulation,lRate,	   bitshift);
    lPopulation -= fudge;
    return (overflow);
#endif
  }

int BifurcSetTrigPi()
  {
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcSetTrigPi()
  {
#ifndef XFRACT
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation = multiply(lRate,ltmp.x,bitshift);
    return (overflow);
#endif
  }

int BifurcAddTrigPi()
  {
    tmp.x = Population * PI;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population += Rate * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcAddTrigPi()
  {
#ifndef XFRACT
    ltmp.x = multiply(lPopulation,LPI,bitshift);
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    lPopulation += multiply(lRate,ltmp.x,bitshift);
    return (overflow);
#endif
  }

int BifurcLambdaTrig()
  {
/*  Population = Rate * fn(Population) * (1 - fn(Population)) */
    tmp.x = Population;
    tmp.y = 0;
    CMPLXtrig0(tmp, tmp);
    Population = Rate * tmp.x * (1 - tmp.x);
    return (fabs(Population) > BIG);
  }

int LongBifurcLambdaTrig()
  {
#ifndef XFRACT
    ltmp.x = lPopulation;
    ltmp.y = 0;
    LCMPLXtrig0(ltmp, ltmp);
    ltmp.y = ltmp.x - multiply(ltmp.x,ltmp.x,bitshift);
    lPopulation = multiply(lRate,ltmp.y,bitshift);
    return (overflow);
#endif
  }

#define LCMPLXpwr(arg1,arg2,out)    Arg2->l = (arg1); Arg1->l = (arg2);\
	 lStkPwr(); Arg1++; Arg2++; (out) = Arg2->l

long beta;

int BifurcMay()
  { /* X = (lambda * X) / (1 + X)^beta, from R.May as described in Pickover,
            Computers, Pattern, Chaos, and Beauty, page 153 */
    tmp.x = 1.0 + Population;
    tmp.x = pow(tmp.x, -beta); /* pow in math.h included with mpmath.h */
    Population = (Rate * Population) * tmp.x;
    return (fabs(Population) > BIG);
  }

int LongBifurcMay()
  {
#ifndef XFRACT
    ltmp.x = lPopulation + fudge;
    ltmp.y = 0;
    lparm2.x = beta * fudge;
    LCMPLXpwr(ltmp, lparm2, ltmp);
    lPopulation = multiply(lRate,lPopulation,bitshift);
    lPopulation = divide(lPopulation,ltmp.x,bitshift);
    return (overflow);
#endif
  }

int BifurcMaySetup()
  {

   beta = (long)param[2];
   if(beta < 2)
      beta = 2;
   param[2] = (double)beta;

   timer(0,curfractalspecific->calctype);
   return(0);
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

/******************* standalone engine for "lyapunov" *********************/
/*** Roy Murphy [76376,721]						***/
/*** revision history:							***/
/*** initial version: Winter '91					***/
/***    Fall '92 integration of Nicholas Wilt's ASM speedups		***/
/***	Jan 93' integration with calcfrac() yielding boundary tracing,	***/
/***	tesseral, and solid guessing, and inversion, inside=nnn		***/
/*** save_release behavior:						***/
/***    1730 & prior: ignores inside=, calcmode='1', (a,b)->(x,y)	***/
/***	1731: other calcmodes and inside=nnn				***/
/***	1732: the infamous axis swap: (b,a)->(x,y),			***/
/***		the order parameter becomes a long int			***/
/**************************************************************************/
int lyaLength, lyaSeedOK;
int lyaRxy[34];

#define WES 1   /* define WES to be 0 to use Nick's lyapunov.obj */
#if WES
extern int lyapunov_cycles(double, double);
#else
extern int lyapunov_cycles(int, double, double, double);
#endif

int lyapunov_cycles_in_c(int, double, double);

lyapunov () {
    double a, b;

    if (check_key()) {
	return -1;
    	}
    overflow=FALSE;
    if (param[1]==1) Population = (1.0+rand())/(2.0+RAND_MAX);
    else if (param[1]==0) {
	if (fabs(Population)>BIG || Population==0 || Population==1)
	    Population = (1.0+rand())/(2.0+RAND_MAX);
	}
    else Population = param[1];
    (*plot)(col, row, 1);
    if (invert) {
	invertz2(&init);
	a = init.y;
	b = init.x;
	}
    else {
	a = dy0[row]+dy1[col];
	b = dx0[col]+dx1[row];
	}
#ifndef XFRACT
    /*  the assembler routines don't work for a & b outside the
	ranges 0 < a < 4 and 0 < b < 4. So, fall back on the C
	routines if part of the image sticks out.
	*/
#if WES
        color=lyapunov_cycles(a, b);
#else
    if (lyaSeedOK && a>0 && b>0 && a<=4 && b<=4)
	color=lyapunov_cycles(filter_cycles, Population, a, b);
    else
	color=lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
#else
    color=lyapunov_cycles_in_c(filter_cycles, a, b);
#endif
    if (inside>0 && color==0)
	color = inside;
    else if (color>=colors)
	color = colors-1;
    (*plot)(col, row, color);
    return color;
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
	    number    sequence
  		0	ab
  		1	aab
  		2	aabb
  		3	aaab
  		4	aabbb
  		5	aabab
  		6	aaabb (this is a duplicate of 4, a rotated inverse)
  		7	aaaab
  		8	aabbbb	etc.
	 */

    long i;
    int t;

    if ((filter_cycles=param[2])==0)
	filter_cycles=maxit/2;
    lyaSeedOK = param[1]>0 && param[1]<=1 && debugflag!=90;
    lyaLength = 1;

    i = param[0];
#ifndef XFRACT
    if (save_release<1732) i &= 0x0FFFFL; /* make it a short to reporduce prior stuff*/
#endif
    lyaRxy[0] = 1;
    for (t=31; t>=0; t--)
	if (i & (1<<t)) break;
    for (; t>=0; t--)
	lyaRxy[lyaLength++] = (i & (1<<t)) != 0;
    lyaRxy[lyaLength++] = 0;
    if (save_release<1732)		/* swap axes prior to 1732 */
	for (t=lyaLength; t>=0; t--)
	    lyaRxy[t] = !lyaRxy[t];
    if (save_release<1731) {		/* ignore inside=, stdcalcmode */
    	stdcalcmode='1';
	if (inside==1) inside = 0;
	}
    if (inside<0) {
        static char far msg[]=
	    {"Sorry, inside options other than inside=nnn are not supported by the lyapunov"};
        stopmsg(0,(char far *)msg);
        inside=1;
	}
    return 1;
}

int lyapunov_cycles_in_c(int filter_cycles, double a, double b) {
    int color, count, i, lnadjust;
    double lyap, total, temp;
    /* e10=22026.4657948  e-10=0.0000453999297625 */

    for (i=0; i<filter_cycles; i++) {
	for (count=0; count<lyaLength; count++) {
	    Rate = lyaRxy[count] ? a : b;
	    if (curfractalspecific->orbitcalc()) {
		overflow = TRUE;
		goto jumpout;
		}
	    }
	}
    total = 1.0;
    lnadjust = 0;
    for (i=0; i < maxit/2; i++) {
	for (count = 0; count < lyaLength; count++) {
	    Rate = lyaRxy[count] ? a : b;
	    if (curfractalspecific->orbitcalc()) {
		overflow = TRUE;
		goto jumpout;
		}
	    temp = fabs(Rate-2.0*Rate*Population);
		if ((total *= (temp))==0) {
		overflow = TRUE;
		goto jumpout;
		}
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

jumpout:
    if (overflow || total <= 0 || (temp = log(total) + lnadjust) > 0)
	color = 0;
    else {
	if (LogFlag)
	lyap = -temp/((double) lyaLength*i);
    else
	lyap = 1 - exp(temp/((double) lyaLength*i));
	color = 1 + (int)(lyap * (colors-1));
	}
    return color;
}


/******************* standalone engine for "cellular" ********************/
/* Originally coded by Ken Shirriff.
   Modified beyond recognition by Jonathan Osuch.
     Original or'd the neighborhood, changed to sum the neighborhood
     Changed prompts and error messages
     Added CA types
     Set the palette to some standard? CA colors
     Changed *cell_array to near and used dstack so put_line and get_line
       could be used all the time
     Made space bar generate next screen
     Increased string/rule size to 16 digits and added CA types 9/20/92
*/

#define BAD_T         1
#define BAD_MEM       2
#define STRING1       3
#define STRING2       4
#define TABLEK        5
#define TYPEKR        6
#define RULELENGTH    7

#define CELLULAR_DONE 10

#ifndef XFRACT
static BYTE *cell_array[2];
#else
static BYTE far *cell_array[2];
#endif

S16 r, k_1, rule_digits;
int lstscreenflag;

void abort_cellular(int err, int t)
{
   int i;
   switch (err)
   {
      case BAD_T:
         {
         char msg[30];
         sprintf(msg,"Bad t=%d, aborting\n", t);
         stopmsg(0,(char far *)msg);
         }
         break;
      case BAD_MEM:
         {
         static char far msg[]={"Insufficient free memory for calculation" };
         stopmsg(0,msg);
         }
         break;
      case STRING1:
         {
         static char far msg[]={"String can be a maximum of 16 digits" };
         stopmsg(0,msg);
         }
         break;
      case STRING2:
         {
         static char far msg[]={"Make string of 0's through  's" };
         msg[27] = k_1 + 48; /* turn into a character value */
         stopmsg(0,msg);
         }
         break;
      case TABLEK:
         {
         static char far msg[]={"Make Rule with 0's through  's" };
         msg[27] = k_1 + 48; /* turn into a character value */
         stopmsg(0,msg);
         }
         break;
      case TYPEKR:
         {
         static char far msg[]={"Type must be 21, 31, 41, 51, 61, 22, 32, \
42, 23, 33, 24, 25, 26, 27" };
         stopmsg(0,msg);
         }
         break;
      case RULELENGTH:
         {
         static char far msg[]={"Rule must be    digits long" };
         i = rule_digits / 10;
         if(i==0)
            msg[14] = rule_digits + 48;
         else {
            msg[13] = i;
            msg[14] = rule_digits % 10 + 48;
         }
         stopmsg(0,msg);
         }
         break;
      case CELLULAR_DONE:
         break;
   }
   if(cell_array[0] != NULL)
#ifndef XFRACT
      cell_array[0] = NULL;
#else
      farmemfree((char far *)cell_array[0]);
#endif
   if(cell_array[1] != NULL)
#ifndef XFRACT
      cell_array[1] = NULL;
#else
      farmemfree((char far *)cell_array[1]);
#endif
}

cellular () {
   S16 start_row;
   S16 filled, notfilled;
   U16 cell_table[32];
   U16 init_string[16];
   U16 kr, k;
   U32 lnnmbr;
   U16 i,j,l;
   S16 t, t2;
   S32 randparam;
   double n;
   char buf[30];

   set_Cellular_palette();

   randparam = param[0];
   lnnmbr = param[3];
   kr = param[2];
   switch (kr) {
     case 21:
     case 31:
     case 41:
     case 51:
     case 61:
     case 22:
     case 32:
     case 42:
     case 23:
     case 33:
     case 24:
     case 25:
     case 26:
     case 27:
        break;
     default:
        abort_cellular(TYPEKR, 0);
        return -1;
        break;
   }

   r = kr % 10; /* Number of nearest neighbors to sum */
   k = kr / 10; /* Number of different states, k=3 has states 0,1,2 */
   k_1 = k - 1; /* Highest state value, k=3 has highest state value of 2 */
   rule_digits = (r * 2 + 1) * k_1 + 1; /* Number of digits in the rule */

   if ((!rflag) && randparam == -1)
       --rseed;
   if (randparam != 0 && randparam != -1) {
      n = param[0];
      sprintf(buf,"%.16g",n); /* # of digits in initial string */
      t = strlen(buf);
      if (t>16 || t <= 0) {
         abort_cellular(STRING1, 0);
         return -1;
      }
      for (i=0;i<16;i++)
         init_string[i] = 0; /* zero the array */
      t2 = (S16) ((16 - t)/2);
      for (i=0;i<t;i++) { /* center initial string in array */
         init_string[i+t2] = buf[i] - 48; /* change character to number */
         if (init_string[i+t2]>k_1) {
            abort_cellular(STRING2, 0);
            return -1;
         }
      }
   }

   srand(rseed);
   if (!rflag) ++rseed;

/* generate rule table from parameter 1 */
#ifndef XFRACT
   n = param[1];
#else
   /* gcc can't manage to convert a big double to an unsigned long properly. */
   if (param[1]>0x7fffffff) {
       n = (param[1]-0x7fffffff);
       n += 0x7fffffff;
   } else {
       n = param[1];
   }
#endif
   if (n == 0) { /* calculate a random rule */
      n = rand15()%k;
      for (i=1;i<rule_digits;i++) {
         n *= 10;
         n += rand15()%k;
      }
      param[1] = n;
   }
   sprintf(buf,"%.*g",rule_digits ,n);
   t = strlen(buf);
   if (rule_digits < t || t < 0) { /* leading 0s could make t smaller */
      abort_cellular(RULELENGTH, 0);
      return -1;
   }
   for (i=0;i<rule_digits;i++) /* zero the table */
      cell_table[i] = 0;
   for (i=0;i<t;i++) { /* reverse order */
      cell_table[i] = buf[t-i-1] - 48; /* change character to number */
      if (cell_table[i]>k_1) {
         abort_cellular(TABLEK, 0);
         return -1;
      }
   }


   start_row = 0;
#ifndef XFRACT
  /* two 4096 byte arrays, at present at most 2024 + 1 bytes should be */
  /* needed in each array (max screen width + 1) */
   cell_array[0] = (BYTE *)&dstack[0]; /* dstack is in general.asm */
   cell_array[1] = (BYTE *)&boxy[0]; /* boxy is in general.asm */
#else
   cell_array[0] = (BYTE far *)farmemalloc(ixstop+1);
   cell_array[1] = (BYTE far *)farmemalloc(ixstop+1);
#endif
   if (cell_array[0]==NULL || cell_array[1]==NULL) {
      abort_cellular(BAD_MEM, 0);
      return(-1);
   }

/* nxtscreenflag toggled by space bar in fractint.c, 1 for continuous */
/* 0 to stop on next screen */

   filled = 0;
   notfilled = 1-filled;
   if (resuming && !nxtscreenflag && !lstscreenflag) {
      start_resume();
      get_resume(sizeof(int),&start_row,0);
      end_resume();
      get_line(start_row,0,ixstop,cell_array[filled]);
   }
   else if (nxtscreenflag && !lstscreenflag) {
      start_resume();
      end_resume();
      get_line(iystop,0,ixstop,cell_array[filled]);
      param[3] += iystop + 1;
      start_row = -1; /* after 1st iteration its = 0 */
   }
   else {
    if(rflag || randparam==0 || randparam==-1){
      for (col=0;col<=ixstop;col++) {
         cell_array[filled][col] = rand15()%k;
      }
    } /* end of if random */

    else {
      for (col=0;col<=ixstop;col++) { /* Clear from end to end */
         cell_array[filled][col] = 0;
      }
      i = 0;
      for (col=(ixstop-16)/2;col<(ixstop+16)/2;col++) { /* insert initial */
         cell_array[filled][col] = init_string[i++];    /* string */
      }
    } /* end of if not random */
    if (lnnmbr != 0)
      lstscreenflag = 1;
    else
      lstscreenflag = 0;
    put_line(start_row,0,ixstop,cell_array[filled]);
   }
   start_row++;

/* This section calculates the starting line when it is not zero */
/* This section can't be resumed since no screen output is generated */
/* calculates the (lnnmbr - 1) generation */
   if (lstscreenflag) { /* line number != 0 & not resuming & not continuing */
     for (row = start_row; row < lnnmbr; row++) {
      thinking(1,"Cellular thinking (higher start row takes longer)");
      if(rflag || randparam==0 || randparam==-1){
       /* Use a random border */
       for (i=0;i<=r;i++) {
         cell_array[notfilled][i]=rand15()%k;
         cell_array[notfilled][ixstop-i]=rand15()%k;
       }
      }
      else {
       /* Use a zero border */
       for (i=0;i<=r;i++) {
         cell_array[notfilled][i]=0;
         cell_array[notfilled][ixstop-i]=0;
       }
      }

       t = 0; /* do first cell */
       for (i=0;i<=r+r;i++)
           t += cell_array[filled][i];
       if (t>rule_digits || t<0) {
         thinking(0, NULL);
         abort_cellular(BAD_T, t);
         return(-1);
       }
       cell_array[notfilled][r] = cell_table[t];

           /* use a rolling sum in t */
       for (col=r+1;col<ixstop-r;col++) { /* now do the rest */
         t = t + cell_array[filled][col+r] - cell_array[filled][col-r-1];
         if (t>rule_digits || t<0) {
           thinking(0, NULL);
           abort_cellular(BAD_T, t);
           return(-1);
         }
         cell_array[notfilled][col] = cell_table[t];
       }

       filled = notfilled;
       notfilled = 1-filled;
       if (check_key()) {
          thinking(0, NULL);
          abort_cellular(CELLULAR_DONE, 0);
          alloc_resume(10,1);
          put_resume(sizeof(int),&row,0);
          return -1;
       }
     }
   start_row = 0;
   thinking(0, NULL);
   lstscreenflag = 0;
   }

/* This section does all the work */
contloop:
   for (row = start_row; row <= iystop; row++) {

      if(rflag || randparam==0 || randparam==-1){
       /* Use a random border */
       for (i=0;i<=r;i++) {
         cell_array[notfilled][i]=rand15()%k;
         cell_array[notfilled][ixstop-i]=rand15()%k;
       }
      }
      else {
       /* Use a zero border */
       for (i=0;i<=r;i++) {
         cell_array[notfilled][i]=0;
         cell_array[notfilled][ixstop-i]=0;
       }
      }

       t = 0; /* do first cell */
       for (i=0;i<=r+r;i++)
           t += cell_array[filled][i];
       if (t>rule_digits || t<0) {
         thinking(0, NULL);
         abort_cellular(BAD_T, t);
         return(-1);
       }
       cell_array[notfilled][r] = cell_table[t];

           /* use a rolling sum in t */
       for (col=r+1;col<ixstop-r;col++) { /* now do the rest */
         t = t + cell_array[filled][col+r] - cell_array[filled][col-r-1];
         if (t>rule_digits || t<0) {
           thinking(0, NULL);
           abort_cellular(BAD_T, t);
           return(-1);
         }
         cell_array[notfilled][col] = cell_table[t];
       }

       filled = notfilled;
       notfilled = 1-filled;
       put_line(row,0,ixstop,cell_array[filled]);
       if (check_key()) {
          abort_cellular(CELLULAR_DONE, 0);
          alloc_resume(10,1);
          put_resume(sizeof(int),&row,0);
          return -1;
       }
   }
   if(nxtscreenflag) {
     param[3] += iystop + 1;
     start_row = -1; /* after 1st iteration its = 0 */
     goto contloop;
   }
  abort_cellular(CELLULAR_DONE, 0);
  return 1;
}

int CellularSetup(void)
{
   if (!resuming) {
      nxtscreenflag = 0; /* initialize flag */
   }
   timer(0,curfractalspecific->calctype);
   return(0);
}

static void set_Cellular_palette()
{
   extern char far *mapdacbox;
   static Palettetype Red    = {
      42, 0, 0	 };
   static Palettetype Green  = {
      10,35,10	};
   static Palettetype Blue   = {
      13,12,29	};
   static Palettetype Yellow = {
      60,58,18	};
   static Palettetype Brown = {
      42,21, 0	};
   int i;

   if (mapdacbox) return;		/* map= specified */

   dacbox[0].red  = 0 ;
   dacbox[0].green= 0 ;
   dacbox[0].blue = 0 ;

   dacbox[1].red	 = Red.red;
   dacbox[1].green = Red.green;
   dacbox[1].blue  = Red.blue;

   dacbox[2].red   = Green.red;
   dacbox[2].green = Green.green;
   dacbox[2].blue  = Green.blue;

   dacbox[3].red   = Blue.red;
   dacbox[3].green = Blue.green;
   dacbox[3].blue  = Blue.blue;

   dacbox[4].red   = Yellow.red;
   dacbox[4].green = Yellow.green;
   dacbox[4].blue  = Yellow.blue;

   dacbox[5].red   = Brown.red;
   dacbox[5].green = Brown.green;
   dacbox[5].blue  = Brown.blue;

   SetTgaColors();
   spindac(0,1);
}

/* frothy basin routines */
static char froth3_256c[] = "froth3.map";
static char froth6_256c[] = "froth6.map";
static char froth3_16c[] =  "froth316.map";
static char froth6_16c[] =  "froth616.map";
int frothsix=0;
int froth_altcolor;
int froth_shades;
extern int colorstate;

/* color maps which attempt to replicate the images of James Alexander. */
static void set_Froth_palette(void)
   {
   char *mapname;

   if (colorstate != 0) /* 0 means dacbox matches default */
      return;

   if (colors >= 16)
      {
      if (colors >= 256)
         {
         if (frothsix)
            mapname = froth6_256c;
         else
            mapname = froth3_256c;
         }
      else /* colors >= 16 */
         {
         if (frothsix)
            mapname = froth6_16c;
         else
            mapname = froth3_16c;
         }
      if (ValidateLuts(mapname) != 0)
         return;
      colorstate = 0; /* treat map it as default */
      spindac(0,1);
      }
   }

int froth_setup(void)
   {
   if (param[0] != 3 && param[0] != 6) /* if no match then*/
      param[0] = 3;                    /* make it 3 */
   frothsix = param[0] == 6;
   froth_altcolor = param[1] != 0;
   froth_shades = (colors-1) / (frothsix ? 6 : 3);
   /* rqlim needs to be at least 6 or so */
   if (rqlim < 6.0)
      rqlim=6.0;
   set_Froth_palette();
   /* make the best of the .map situation */
   orbit_color = !frothsix && colors >= 16 ? (froth_shades<<1)+1 : colors-1;
   return 1;
   }

/* Froth Fractal type */
  int calcfroth(void)   /* per pixel 1/2/g, called with row & col set */
     {
     int found_attractor=0;
     double x, y, nx, ny, x2, y2;
     long lx, ly, lnx, lny, lx2, ly2;

/* These points were determined imperically and verified experimentally */
/* using the program WL-Plot, a plotting program which has a mode for */
/* orbits of recursive relations. */
#define CLOSE    1e-6      /* seems like a good value */
#define SQRT3    1.732050807568877193
#define A        1.02871376822
#define B1       (A/2)
#define M2       SQRT3
#define B2       (-A)
#define M3       (-SQRT3)
#define B3       (-A)
#define X1MIN   -1.04368901270
#define X1MAX    1.33928675524
#define XMIDT   -0.339286755220
#define X2MAX1   0.96729063460
#define XMIDR    0.61508950585
#define X3MIN1  -0.22419724936
#define X2MIN2  -1.11508950586
#define XMIDL   -0.27580275066
#define X3MAX2   0.07639837810
#define FROTH_BITSHIFT  28
/* compiler should handle this at compile time */
#define D_TO_L(x) ((long)((x)*(1L<<FROTH_BITSHIFT)))

   orbit_ptr = 0;
   color = 0;
   if(showdot>0)
      (*plot) (col, row, showdot&(colors-1));
   if (!integerfractal) /* fp mode */
      {
      double close=  CLOSE;
      double a=      A;
      double b1=     B1;
      double xmidt=  XMIDT;
      double m2=     M2;
      double b2=     B2;
      double m3=     M3;
      double b3=     B3;
      double x1min=  X1MIN;
      double x1max=  X1MAX;
      double x2max1= X2MAX1;
      double xmidr=  XMIDR;
      double x3min1= X3MIN1;
      double x2min2= X2MIN2;
      double xmidl=  XMIDL;
      double x3max2= X3MAX2;

      if(invert)
	 {
	 invertz2(&tmp);
	 x = tmp.x;
	 y = tmp.y;
	 }
      else
	 {
	 x = dx0[col]+dx1[row];
	 y = dy0[row]+dy1[col];
	 }

      magnitude = (x2=sqr(x)) + (y2=sqr(y));
      while (!found_attractor && (magnitude < rqlim) && (color < maxit))
	 {
         /* simple formula: z = z^2 + conj(z*(-1+ai)) */
	 /* but it's the attractor that makes this so interesting */
	 nx = x2 - y2 - x - a*y;
	 ny = (x+x)*y - a*x + y;
	 x = nx;
	 y = ny;
	 if (frothsix) /* repeat mapping */
	    {
	    nx = sqr(x) - sqr(y) - x - a*y;
	    ny = (x+x)*y - a*x + y;
	    x = nx;
	    y = ny;
	    }
	 magnitude = (x2=sqr(x)) + (y2=sqr(y));
	 color++;

	 if (show_orbit)
	    plot_orbit(x, y, -1);

         if (x > x1min && x < x1max && fabs(b1-y) < close)
	    {
	    if (!frothsix || x < xmidt)
	       found_attractor = 1;
	    else
	       found_attractor = 2;

	    }
	 else if (fabs(m2*x+b2-y) < close)
	    {
	    if (x > xmidr && x < x2max1)
	       found_attractor = !frothsix ? 2 : 4;
	    else if (x > x3min1 && x < xmidr)
	       found_attractor = !frothsix ? 3 : 6;
	    }
	 else if (fabs(m3*x+b3-y) < close)
	    {
	    if (x > x2min2 && x < xmidl)
	       found_attractor = !frothsix ? 2 : 3;
	    else if (x > xmidl && x < x3max2)
	       found_attractor = !frothsix ? 3 : 5;
	    }
	 }
      }
   else /* integer mode */
      {
      long lclose=   D_TO_L(CLOSE);
      long la=       D_TO_L(A);
      long lb1=      D_TO_L(B1);
      long lxmidt=   D_TO_L(XMIDT);
      long lm2=      D_TO_L(M2);
      long lb2=      D_TO_L(B2);
      long lm3=      D_TO_L(M3);
      long lb3=      D_TO_L(B3);
      long lx1min=   D_TO_L(X1MIN);
      long lx1max=   D_TO_L(X1MAX);
      long lx2max1=  D_TO_L(X2MAX1);
      long lxmidr=   D_TO_L(XMIDR);
      long lx3min1=  D_TO_L(X3MIN1);
      long lx2min2=  D_TO_L(X2MIN2);
      long lxmidl=   D_TO_L(XMIDL);
      long lx3max2=  D_TO_L(X3MAX2);

      if(invert)
	 {
	 invertz2(&tmp);
	 lx = tmp.x * fudge;
	 ly = tmp.y * fudge;
	 }
      else
	 {
	 lx = lx0[col] + lx1[row];
	 ly = ly0[row] + ly1[col];
	 }

      lmagnitud = (lx2=lsqr(lx)) + (ly2=lsqr(ly));
      while (!found_attractor && (lmagnitud < llimit)
	     && (lmagnitud > 0) && (color < maxit))
	 {
         /* simple formula: z = z^2 + conj(z*(-1+ai)) */
	 /* but it's the attractor that makes this so interesting */
	 lnx = lx2 - ly2 - lx - multiply(la,ly,bitshift);
	 lny = (multiply(lx,ly,bitshift)<<1) - multiply(la,lx,bitshift) + ly;
	 lx = lnx;
	 ly = lny;
	 if (frothsix)
	    {
	    lmagnitud = (lx2=lsqr(lx)) + (ly2=lsqr(ly));
	    if ((lmagnitud > llimit) || (lmagnitud < 0))
	       break;
	    lnx = lx2 - ly2 - lx - multiply(la,ly,bitshift);
	    lny = (multiply(lx,ly,bitshift)<<1) - multiply(la,lx,bitshift) + ly;
	    lx = lnx;
	    ly = lny;
	    }
	 lmagnitud = (lx2=lsqr(lx)) + (ly2=lsqr(ly));
	 color++;

	 if (show_orbit)
	    iplot_orbit(lx, ly, -1);

         if (lx > lx1min && lx < lx1max && labs(lb1-ly) < lclose)
	    {
	    if (!frothsix || lx < lxmidt)
	       found_attractor = 1;
	    else
	       found_attractor = 2;
	    }
	 else if (labs(multiply(lm2,lx,bitshift)+lb2-ly) < lclose)
	    {
	    if (lx > lxmidr && lx < lx2max1)
	       found_attractor = !frothsix ? 2 : 4;
	    else if (lx > lx3min1 && lx < lxmidr)
	       found_attractor = !frothsix ? 3 : 6;
	    }
	 else if (labs(multiply(lm3,lx,bitshift)+lb3-ly) < lclose)
	    {
	    if (lx > lx2min2 && lx < lxmidl)
	       found_attractor = !frothsix ? 2 : 3;
	    else if (lx > lxmidl && lx < lx3max2)
	       found_attractor = !frothsix ? 3 : 5;
	    }
	 }
      }
   if (show_orbit)
      scrub_orbit();

   realcolor = color;
   if ((kbdcount -= realcolor) <= 0)
      {
      if (check_key())
	 return (-1);
      kbdcount = max_kbdcount;
      }

/* inside - Here's where non-palette based images would be nice.  Instead, */
/* we'll use blocks of (colors-1)/3 or (colors-1)/6 and use special froth  */
/* color maps in attempt to replicate the images of James Alexander.       */
   if (found_attractor)
      {
      if (colors >= 256)
	 {
	 if (!froth_altcolor)
	    {
	    if (color > froth_shades)
		color = froth_shades;
	    }
	 else
	    color = froth_shades * color / maxit;
	 if (color == 0)
	    color = 1;
	 color += froth_shades * (found_attractor-1);
	 }
      else if (colors >= 16)
	 { /* only alternate coloring scheme available for 16 colors */
	 long lshade;

/* Trying to make a better 16 color distribution. */
/* Since their are only a few possiblities, just handle each case. */
/* This is a mostly guess work here. */
	 lshade = ((long)color<<16)/maxit;
	 if (!frothsix)
	    {
	    if (lshade < 2622)       /* 0.04 */
	       color = 1;
	    else if (lshade < 10486) /* 0.16 */
	       color = 2;
	    else if (lshade < 23593) /* 0.36 */
	       color = 3;
	    else if (lshade < 41943) /* 0.64 */
	       color = 4;
	    else
	       color = 5;
	    color += 5 * (found_attractor-1);
	    }
	 else
	    {
	    if (lshade < 10486)      /* 0.16 */
	       color = 1;
	    else
	       color = 2;
	    color += 2 * (found_attractor-1);
	    }
	 }
      else /* use a color corresponding to the attractor */
	 color = found_attractor;
      oldcolor = color;
      }
   else if (color >= maxit)
      color = oldcolor; /* inside, but didn't get sucked in by attractor. */
   else /* outside */
      color = 0; /* all outside points are color 0 */

   (*plot)(col, row, color);

   return color;

#undef CLOSE
#undef SQRT3
#undef A
#undef B1
#undef XMIDT
#undef M2
#undef B2
#undef M3
#undef B3
#undef X1MIN
#undef X1MAX
#undef X2MAX1
#undef XMIDR
#undef X3MIN1
#undef X2MIN2
#undef XMIDL
#undef X3MAX2
#undef FROTH_BITSHIFT
#undef D_TO_L
   }
