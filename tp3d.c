#include <stdio.h>
#include "mpmath.h"
#include "fractint.h"
#include "prototyp.h"

/* 3D Transparency Variables, MCP 5-30-91 */
extern double xxmin, xxmax, yymin, yymax, zzmin, zzmax, ttmin, ttmax;
extern int Transparent3D, SolidCore, NumFrames;
extern unsigned CoreRed, CoreGreen, CoreBlue;
extern int tpdepth, tptime, symmetry, AntiAliasing;

extern int xdots, ydots, colors, bitshift;
extern int row, col, inside, ShadowColors;
extern int TPlusFlag, MaxColorRes, NonInterlaced;

typedef struct palett {
   BYTE red;
   BYTE green;
   BYTE blue;
} Palettetype;

extern Palettetype	dacbox[ 256 ];


int far NewTPFractal, far _MathType;
static double dx, dy, dz, dt;
static long ldx, ldy, ldz, ldt;
static long lxxmin, lxxmax, lyymin, lyymax, lzzmin, lzzmax, lttmin, lttmax;

int TranspSymmetry = 0;

/* TARGA+ prototypes */
int MatchTPlusMode(unsigned xdots, unsigned ydots, unsigned MaxColorRes,
		   unsigned PixelZoom, unsigned NonInterlaced);
int CheckForTPlus(void);

#ifndef XFRACT
void (*PutTC)(int col, int row, unsigned long color) = WriteTPlusBankedPixel;
unsigned long (*GetTC)(int col, int row) = ReadTPlusBankedPixel;
#else
void (*PutTC)(int , int , unsigned long ) = WriteTPlusBankedPixel;
unsigned long (*GetTC)(int , int ) = ReadTPlusBankedPixel;
#endif


char far * far TCError[] =
{
   "Could not match this video resolution to a TARGA+ mode",
   "True Color disk video isn't implement yet, maybe later . . .",
};

char far *TrueColorAutoDetect(void)
{
   if(CheckForTPlus())
   {
      if(TPlusFlag)
      {
         if(MaxColorRes == 8)
         {
            if(!MatchTPlusMode(xdots, ydots, 24, 0, NonInterlaced))
               return(TCError[0]);
         }
         else if(!MatchTPlusMode(xdots, ydots, MaxColorRes, 0, NonInterlaced))
            return(TCError[0]);

         PutTC = WriteTPlusBankedPixel;
         GetTC = ReadTPlusBankedPixel;
         return(0);
      }
   }

   /* Check for other TC adapters here, such as the XGA , and place it
      in a true color mode with a resolution identical to the one selected
      by the user. */


   /* If we don't have a TC adapter or it is being used as the display
      screen, then simulate one using disk video */
   return(TCError[1]); /* return the 'not implemented' message */
}

void TranspPerPixel(int MathType, union Arg far *xy, union Arg far *zt)
{
   if(NewTPFractal)
   {
      double Fudge = (double)(1L << bitshift);
      _MathType = MathType;              /* Save this for later */

      dx = (xxmax - xxmin) / xdots;
      dy = (yymin - yymax) / ydots;  /* reverse direction of y-axis */
      dz = (zzmax - zzmin) / xdots;
      dt = (ttmax - ttmin) / NumFrames;

      lxxmin = (long)(xxmin * Fudge);
      lxxmax = (long)(xxmax * Fudge);
      ldx    = (long)(dx    * Fudge);

      lyymin = (long)(yymin * Fudge);
      lyymax = (long)(yymax * Fudge);
      ldy    = (long)(dy    * Fudge);

      lzzmin = (long)(zzmin * Fudge);
      lzzmax = (long)(zzmax * Fudge);
      ldz    = (long)(dz    * Fudge);

      lttmin = (long)(ttmin * Fudge);
      lttmax = (long)(ttmax * Fudge);
      ldt    = (long)(dt    * Fudge);

      NewTPFractal = 0;
   }

   switch(MathType)
   {
      /* For calculation purposes, 'x' and 'z' are swapped */
      case D_MATH:
           xy->d.x = xxmin + (dx * col);
           xy->d.y = yymax + (dy * row);
           zt->d.x = zzmin + (dz * tpdepth);
           zt->d.y = ttmin + (dt * tptime);
           break;
#ifndef XFRACT
      case M_MATH:
           xy->m.x = *d2MP(xxmin + (dx * col));
           xy->m.y = *d2MP(yymax + (dy * row));
           zt->m.x = *d2MP(zzmin + (dz * tpdepth));
           zt->m.y = *d2MP(ttmin + (dt * tptime));
           break;
      case L_MATH:
           xy->l.x = lxxmin + (ldx * col);
           xy->l.y = lyymax + (ldy * row);
           zt->l.x = lzzmin + (ldz * tpdepth);
           zt->l.y = lttmin + (ldt * tptime);
#endif
   }
}


int Transp3DFnct()
{
   unsigned x, y, z, savedinside;
   int r;
   unsigned long lcolor;

   for(x = tpdepth;  x < xdots; x++, tpdepth++)
   {
      /* Here we go!  Calculate 2D slices to create a 3D composite */
      savedinside = inside;
      inside = 255;
      r = calcfract();
      inside = savedinside;

      if(r < 0)
         return(r);                 /* return if iterupted */

      for(y = 0; y < ydots; y++)
      {
         unsigned long Red = 0L, Green = 0L, Blue = 0L;
         unsigned long r = 0L, g = 0L, b = 0L;
         unsigned color;

         for(z = 0; z < xdots; z++)  /* Total the color guns */
         {
            color = (*getcolor)(z, y);
            if(color == 255 && SolidCore)
            {                           /* Display a solid core */
               r = ((long)CoreRed)   * (xdots - z) / xdots;
               g = ((long)CoreGreen) * (xdots - z) / xdots;
               b = ((long)CoreBlue)  * (xdots - z) / xdots;
               break;
            }
            else
            {
               Red   += (dacbox[color].red << 2);
               Green += (dacbox[color].green << 2);
               Blue  += (dacbox[color].blue << 2);
            }
         }

         /* Calculate an average color */
         if(z)
         {
            Red /= z;
            Green /= z;
            Blue /= z;
         }

         /* Overlay solid color */
         Red   |= r;
         Green |= g;
         Blue  |= b;

         lcolor = (Red << 16) + (Green << 8) + Blue;
         PutTC(x, y, lcolor);
         if(TranspSymmetry == ORIGIN)
            PutTC(xdots - x - 1, ydots - y - 1, lcolor);
         else if(TranspSymmetry == XAXIS)
         {
            PutTC(x, ydots - y - 1, lcolor);
            PutTC(xdots - x - 1, y, lcolor);
            PutTC(xdots - x - 1, ydots - y - 1, lcolor);
            if(y > (ydots >> 1))
               break;
         }
      }
      if(x > (xdots >> 1))
      {
         if(TranspSymmetry == ORIGIN || TranspSymmetry == XAXIS)
            break;
      }
   }
   return(0);
}

void ShadowPutColor(unsigned xdot, unsigned ydot, unsigned color)
{
   ShadowVideo(0);
   if(ShadowColors)
      putcolor(xdot >> AntiAliasing, ydot >> AntiAliasing, color);
   else
   {
      unsigned r, g, b;
      unsigned long lcolor;

      r = (dacbox[color].red << 2);
      g = (dacbox[color].green << 2);
      b = (dacbox[color].blue << 2);
      lcolor = ((long)r << 16) + (g << 8) + b;
      PutTC(xdot >> AntiAliasing, ydot >> AntiAliasing, lcolor);
   }
   ShadowVideo(1);
}

void AntiAliasPass(void)
{
   unsigned x, y, i, j, PixelSize, a;
   unsigned xAct, yAct, xRef, yRef;

   PixelSize = (1 << AntiAliasing);
   xAct = (xdots >> AntiAliasing);
   yAct = (ydots >> AntiAliasing);

   for(yRef = y = 0; y < yAct; y++, yRef += PixelSize)
   {
      for(xRef = x = 0; x < xAct; x++, xRef += PixelSize)
      {
         unsigned total = 0;
         unsigned long r = 0, g = 0, b = 0, lcolor;

         for(i = 0; i < PixelSize; i++)
         {
            for(j = 0; j < PixelSize; j++)
            {
               unsigned color;

               color = readdisk(xRef + i, yRef + j);
               if(ShadowColors)
                  total += color;
               else
               {
                  r += (dacbox[color].red << 2);
                  g += (dacbox[color].green << 2);
                  b += (dacbox[color].blue << 2);
               }
            }
         }

         a = AntiAliasing * AntiAliasing;
         ShadowVideo(0);
         if(ShadowColors)
            putcolor(x, y, total >> a);
         else
         {
            r >>= a;
            g >>= a;
            b >>= a;
            lcolor = ((long)r << 16) + (g << 8) + b;
            PutTC(x, y, lcolor);
         }
         ShadowVideo(1);
      }
   }
}

