/* 
   This file includes miscellaneous plot functions and logic
   for funny red/blue glasses 3D. By Tim Wegner, but I'm not
   sure I should publicize the fact!  
*/ 
#include <stdio.h>
#include "fractint.h"

/* Use these palette indices for red/blue - same on ega/vga */
#define BLUE 	    1
#define RED 	    2
#define MAGENTA	    3

int whichimage;
extern int mapset;
extern int xadjust;
extern int yadjust;
extern int xxadjust;
extern int yyadjust;
extern int xshift;
extern int yshift; 
extern char MAP_name[];
int xxadjust1;
int yyadjust1;
int eyeseparation = 0;
int glassestype = 0;
int xshift1;
int yshift1; 
int xtrans = 0;
int ytrans = 0; 
int red_local_left;
int red_local_right;
int blue_local_left;
int blue_local_right;
int red_crop_left   = 4;
int red_crop_right  = 0;
int blue_crop_left  = 0;
int blue_crop_right = 4;
int red_bright      = 80;
int blue_bright      = 100;

extern int init3d[];
extern int xdots;
extern int ydots;
extern int colors;
extern unsigned char dacbox[256][3];
extern void (*standardplot)();

/* use this for continuous colors later */
void plot3dsuperimpose16b(int x,int y,int color)
{
   int tmp;
   if (color != 0)             /* Keeps index 0 still 0 */
   {
       color = colors - color; /*  Reverses color order */
       color = color / 4;  
       if(color == 0)
          color = 1;
   }
   color = 3;
   tmp = getcolor(x,y);

   /* map to 4 colors */
   if(whichimage == 1) /* RED */
   {
      if(red_local_left < x && x < red_local_right)
         putcolor(x,y,color|tmp);
   }
   else if(whichimage == 2) /* BLUE */
   {
      if(blue_local_left < x && x < blue_local_right)
      {
         color = color <<2;
         putcolor(x,y,color|tmp);
      }
   }
}

void plot3dsuperimpose16(int x,int y,int color)
{
   int tmp;

   tmp = getcolor(x,y);
   
   if(whichimage == 1) /* RED */
   {
      color = RED;
      if(tmp > 0 && tmp != color)
         color = MAGENTA;
      if(red_local_left < x && x < red_local_right)
         putcolor(x,y,color);
   }
   else if(whichimage == 2) /* BLUE */
   {
      if(blue_local_left < x && x < blue_local_right)
      {
         color = BLUE;
         if(tmp > 0 && tmp != color)
            color = MAGENTA;
         putcolor(x,y,color);
      }
   }
}

void plot3dsuperimpose256(x,y,color)
{
   int tmp;
   if (color != 0)             /* Keeps index 0 still 0 */
   {
       /* my mind is fried - lower indices = darker colors is EASIER! */
       color = colors - color; /*  Reverses color order */
       color = 1 + color / 18; /*  Looks weird but maps colors 1-255 to 15
                                   relatively even ranges */
   }
   tmp = getcolor(x,y);
   /* map to 16 colors */
   if(whichimage == 1) /* RED */
   {
      if(red_local_left < x && x < red_local_right)
         putcolor(x,y,color|tmp);
   }
   else if(whichimage == 2) /* BLUE */
   {
      if(blue_local_left < x && x < blue_local_right)
      {
         color = color <<4;
         putcolor(x,y,color|tmp);
      }
   }
}

void plot3dalternate(x,y,color)
{
   /* lorez high color red/blue 3D plot function */
   /* if which image = 1, compresses color to lower 128 colors */

   /* my mind is STILL fried - lower indices = darker colors is EASIER! */
   color = colors - color; 
   if((whichimage == 1) && !((x+y)&1)) /* - lower half palette */
   {
      if(red_local_left < x && x < red_local_right) 
         putcolor(x,y,color>>1);
   }
   else if((whichimage == 2) && ((x+y)&1) ) /* - upper half palette */
   {
      if(blue_local_left < x && x < blue_local_right) 
         putcolor(x,y,(color>>1)+(colors>>1));
   }
}

plot_setup()
{
   double d_red_bright, d_blue_bright;
   int i;


   /* set funny glasses plot function */
   switch(glassestype)
   {
   case 1:
      standardplot = plot3dalternate;
      break;  
   case 2:
      if(colors == 256)
         standardplot = plot3dsuperimpose256;
      else
         standardplot = plot3dsuperimpose16;
      break;
   default:
      standardplot = putcolor;
      break;
   }

   xshift1 = xshift = (XSHIFT * (double)xdots)/100;
   yshift1 = yshift = (YSHIFT * (double)ydots)/100;

   if(glassestype)
   {
      red_local_left   =    (red_crop_left           * (double)xdots)/100.0;  
      red_local_right  =    ((100 - red_crop_right)  * (double)xdots)/100.0; 
      blue_local_left  =    (blue_crop_left          * (double)xdots)/100.0; 
      blue_local_right =    ((100 - blue_crop_right) * (double)xdots)/100.0;
      d_red_bright     =    (double)red_bright/100.0;
      d_blue_bright    =    (double)blue_bright/100.0;

      switch(whichimage)
      {
      case 1:
         xshift  += (eyeseparation* (double)xdots)/200;
         xxadjust = ((xtrans+xadjust)* (double)xdots)/100;
         xshift1  -= (eyeseparation* (double)xdots)/200;
         xxadjust1 = ((xtrans-xadjust)* (double)xdots)/100;
         break;
      case 2:          
         xshift  -= (eyeseparation* (double)xdots)/200;
         xxadjust = ((xtrans-xadjust)* (double)xdots)/100;
         break;
      }
   }
   else
      xxadjust = (xtrans* (double)xdots)/100;
   yyadjust = -(ytrans* (double)ydots)/100;

   if (mapset == 1)
   {
      FILE *dacfile;
      dacfile = fopen(MAP_name,"r");
      ValidateLuts(dacfile);  /* read the palette file */
      fclose(dacfile); /* close it */
   } 
   else if (mapset == 2)
   {
      /*
         Creates a palette for superimposed-pixel red/blue 3D.
         Top four bits is blue, bottom 4 bits is read. Since
         we need all combinations of red and blue for
         superimposition purposes, we are allowed only
         16 shades of red and blue.
      */
      if(colors == 256)
         for(i=0;i<256;i++)
         {
            dacbox[i][0] = (i%16) << 2;
            dacbox[i][1] = 0;
            dacbox[i][2] = (i/16) << 2;
         }
      else
         for(i=0;i<16;i++)
         {
            dacbox[i][0] = (i%4) << 4;
            dacbox[i][1] = 0;
            dacbox[i][2] = (i/4) << 4;
         }
      ValidateLuts(NULL);  /* setup Targa palette */
   } 
   else if (mapset == 3)
   {
      /*
         Creates a continuous palette map file for alternate
         pixel funny glasses 3D. Colors are repeated, so we
         need 128 color "pseudo" red and blue sequences.
         Also, red seems a little brighter than the blue.
      */
      for(i=0;i<128;i++)
      {
         dacbox[i][0] = (unsigned char)(i >> 1);
         dacbox[i][1] = dacbox[i][2] = 0;
      }
      for(i=0;i<128;i++)
      {
         dacbox[i+128][2] = (unsigned char)(i >> 1);
         dacbox[i+128][0] = dacbox[i+128][1] = 0;
      }
      ValidateLuts(NULL);  /* setup Targa palette */
   }
   if (mapset)
   {
      if(glassestype==1 || glassestype==2)
      {
         if(glassestype == 2 && colors < 256)
         {
            dacbox[RED    ][0] = 63;
            dacbox[RED    ][1] =  0;
            dacbox[RED    ][2] =  0;

            dacbox[BLUE   ][0] =  0;
            dacbox[BLUE   ][1] =  0;
            dacbox[BLUE   ][2] = 63;

            dacbox[MAGENTA][0] = 63;
            dacbox[MAGENTA][1] =  0;
            dacbox[MAGENTA][2] = 63;
         }
         for (i=0;i<256;i++)
         {
            dacbox[i][0] = dacbox[i][0] * d_red_bright;
            dacbox[i][2] = dacbox[i][2] * d_blue_bright;
         }
      }   
      spindac(0,1); /* load it, but don't spin */
   }
}
