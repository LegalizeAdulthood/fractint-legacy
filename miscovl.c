/*
	Overlayed odds and ends that don't fit anywhere else.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include "fractint.h"
#include "fractype.h"

/* routines in this module	*/

void miscovl_overlay(void);
void make_batch_file(void);
void shell_to_dos(void);
void showfreemem(void);

extern int  cpu;		/* cpu type			*/
extern int  fpu;		/* fpu type			*/
extern int  video_type;
extern int  askvideo;
extern char overwrite;		/* 1 means ok to overwrite */
extern int  mapset;		/* indicates new map */
extern char MAP_name[]; 	/* map file name */
extern int  inside;		/* inside color: 1=blue     */
extern int  outside;		/* outside color, if set    */
extern double xxmin,xxmax,yymin,yymax,xx3rd,yy3rd; /* selected screen corners */
extern double param[4]; 	/* up to four parameters    */
extern int  finattract; 	/* finite attractor option  */
extern int  forcesymmetry;
extern int  LogFlag;		/* non-zero if logarithmic palettes */
extern int  rflag, rseed;
extern int  periodicitycheck;
extern int  potflag;		/* continuous potential flag */
extern int  pot16bit;		/* save 16 bit values for continuous potential */
extern double potparam[3];	/* three potential parameters*/
extern int  fractype;		/* if == 0, use Mandelbrot  */
extern unsigned char usemag;
extern long delmin;
extern int  maxit;		/* try this many iterations */
extern int  invert;		/* non-zero if inversion active */
extern double inversion[];
extern int  decomp[];
extern int  distest;		/* non-zero if distance estimator   */
extern int  init3d[20]; 	/* '3d=nn/nn/nn/...' values */
extern char floatflag;		/* floating-point fractals? */
extern int  biomorph;
extern char FormFileName[];	/* file to find (type=)formulas in */
extern char FormName[]; 	/* Name of the Formula (if not null) */
extern char LFileName[];
extern char LName[];
extern int  bailout;		/* user input bailout value */
extern char useinitorbit;
extern struct complex initorbit;
extern int  display3d;		/* 3D display flag: 0 = OFF */
extern char readname[]; 	/* name of fractal input file */
extern int  showfile;		/* has file been displayed yet? */
extern int  transparent[];
extern char preview;		/* 3D preview mode flag */
extern char showbox;		/* flag to show box and vector in preview */
extern int  RANDOMIZE;		/* Color randomizing factor */
extern int  full_color; 	/* Selects full color with light source fills */
extern int  Ambient;		/* Darkness of shadows in light source */
extern int  haze;		/* Amount of haze to factor in in full color */
extern char light_name[];	/* Name of full color .TGA file */
extern int previewfactor;
extern int xtrans;
extern int ytrans;
extern int red_crop_left;
extern int red_crop_right;
extern int blue_crop_left;
extern int blue_crop_right;
extern int red_bright;
extern int blue_bright;
extern int xadjust;
extern int eyeseparation;
extern int glassestype;
extern unsigned char trigndx[];


void miscovl_overlay() { }	/* for restore_active_ovly */


void make_batch_file()
{
   double Xctr, Yctr, Magnification;
   FILE *batch;
   char buf[81];
   ENTER_OVLY(OVLY_MISCOVL);
   if((batch = fopen("frabatch.bat", "a")) == NULL) {
      EXIT_OVLY;
      return;
      }
   fprintf(batch, "fractint");

   /********************************/
   /* universal parameters go here */
   /********************************/
/* if(askvideo==0)
      fprintf(batch, " askvideo=no");
   if (overwrite)
      fprintf(batch, " overwrite=yes");
 */
   if (mapset && *MAP_name) {
      /* strip path from file name */
      char *p;
      p = strrchr(MAP_name,'\\');
      if(p)
	 fprintf(batch, " map=%s", p+1);
      else
	 fprintf(batch, " map=%s", MAP_name);
   }
   if(rflag)
      fprintf(batch, " rseed=%d",rseed);

   /***********************************/
   /* fractal only parameters go here */
   /***********************************/
   if (display3d <= 0) { /* a fractal was generated */

      if (fractalspecific[fractype].name[0] != '*')
	 fprintf(batch, " type=%s", fractalspecific[fractype].name);
      else
	 fprintf(batch, " type=%s", &fractalspecific[fractype].name[1]);

      if (fractype == FORMULA || fractype == FFORMULA)
	 fprintf(batch, " formulafile=%s formulaname=%s",FormFileName,FormName);
      if (fractype == LSYSTEM)
	fprintf(batch, " lfile=%s lname=%s",LFileName,LName);

      if(usemag && cvtcentermag(&Xctr, &Yctr, &Magnification)) {
	 if (delmin > 1000)
	    fprintf(batch, " center-mag=%g/%g/%g",
		Xctr,Yctr,Magnification);
	 else
	    fprintf(batch, " center-mag=%+20.17lf/%+20.17lf/%+20.17lf",
		Xctr,Yctr,Magnification);
	 }
      else {
	 if (delmin > 1000) {
	    fprintf(batch, " corners=%g/%g/%g/%g",
		xxmin, xxmax, yymin, yymax);
	    if (xx3rd != xxmin || yy3rd != yymin)
	       fprintf(batch, "/%g/%g", xx3rd, yy3rd);
	    }
	 else {
	    fprintf(batch, " corners=%+20.17lf/%+20.17lf/%+20.17lf/%+20.17lf",
		xxmin, xxmax, yymin, yymax);
	    if (xx3rd != xxmin || yy3rd != yymin)
	       fprintf(batch, "/%+20.17lf/%+20.17lf", xx3rd, yy3rd);
	    }
	 }

      if(param[0]!=0.0 || param[1]!=0.0 || param[2]!=0.0 || param[3]!=0.0)
	 fprintf(batch, " params=%g/%g/%g/%g",
	     param[0], param[1], param[2], param[3]);

      showtrig(buf); /* this function is in miscres.c */
      if (buf[0])
	 fprintf(batch,buf);

      if (floatflag)
	 fprintf(batch, " float=yes");

      if (maxit != 150)
	 fprintf(batch, " maxiter=%d", maxit);

      if (inside == -1)
	 fprintf(batch, " inside=maxiter");
      else if (inside == -60)
	 fprintf(batch, " inside=bof60");
      else if (inside == -61)
	 fprintf(batch, " inside=bof61");
      else if (inside != 1)
	 fprintf(batch, " inside=%d", inside);

      if (finattract)
	 fprintf(batch, " finattract=yes");

      if (outside != -1)
	 fprintf(batch, " outside=%d", outside);

      if(forcesymmetry==XAXIS)
	 fprintf(batch, " symmetry=xaxis");
      else if(forcesymmetry==YAXIS)
	 fprintf(batch, " symmetry=yaxis");
      else if(forcesymmetry==XYAXIS)
	 fprintf(batch, " symmetry=xyaxis");
      else if(forcesymmetry==ORIGIN)
	 fprintf(batch, " symmetry=origin");
      else if(forcesymmetry==PI_SYM)
	 fprintf(batch, " symmetry=pi");
      else if(forcesymmetry==NOSYM)
	 fprintf(batch, " symmetry=none");

      if(LogFlag) {
	 fprintf(batch, " logmap=");
	 if(LogFlag == -1)
	    fprintf(batch, "old");
	 else if(LogFlag == 1)
	    fprintf(batch, "yes");
	 else
	    fprintf(batch, "%d", LogFlag);
	 }

      if(periodicitycheck != 1)
	 fprintf(batch, " periodicity=%d",periodicitycheck);

      if(potflag) {
	 fprintf(batch, " potential=%d/%d/%d",
	     (int)potparam[0],(int)potparam[1],(int)potparam[2]);
	 if(pot16bit)
	    fprintf(batch, "/16bit");
	 }

  /*  if (initincr != 50)
	 fprintf(batch, " iterincr=%d", initincr);
   */
      if (invert)
	 fprintf(batch, " invert=%g/%g/%g",
	     inversion[0], inversion[1], inversion[2]);
      if (decomp[0])
	 fprintf(batch, " decomp=%d", decomp[0]);
      if (distest)
	 fprintf(batch, " distest=%d", distest);
      if (biomorph != -1)
	 fprintf(batch, " biomorph=%d", biomorph);
      if(bailout && (potflag == 0 || potparam[2] == 0.0))
	 fprintf(batch, " bailout=%d",bailout);

      if(useinitorbit == 2)
	 fprintf(batch, " initorbit=pixel");
      else if(useinitorbit == 1)
	 fprintf(batch, " initorbit=%g/%g",initorbit.x,initorbit.y);
      }

   /**********************************/
   /* line3d only parameters go here */
   /**********************************/
   if (display3d >= 1) { /* only for line3d */
      fprintf(batch, " 3d=yes");
      if (showfile && !endswithslash(readname))
	 fprintf(batch, " filename=%s", readname);
      if (SPHERE) {
	 fprintf(batch, " sphere=yes");
	 fprintf(batch, " latitude=%d/%d", THETA1, THETA2);
	 fprintf(batch, " longitude=%d/%d", PHI1, PHI2);
	 fprintf(batch, " radius=%d", RADIUS);
	 }
      if (FILLTYPE)
	 fprintf(batch, " filltype=%d", FILLTYPE);
      if (transparent[0] || transparent[1])
	 fprintf(batch, " transparent=%d/%d", transparent[0],transparent[1]);
      if (preview) {
	 fprintf(batch, " preview=yes");
	 if (showbox)
	    fprintf(batch, " showbox=yes");
	 fprintf(batch, " coarse=%d",previewfactor);
	 }

      if (RANDOMIZE)
	 fprintf(batch, " randomize=%d",RANDOMIZE);
      if (full_color)
	 fprintf(batch, " fullcolor=yes");
      if (Ambient)
	 fprintf(batch, " ambient=%d",Ambient);
      if (haze)
	 fprintf(batch, " haze=%d",haze);
      if (full_color) {
	 /* strip path from file name */
	 char *p;
	 p = strrchr(light_name,'\\');
	 if(p)
	    fprintf(batch, " lightname=%s", p+1);
	 else
	    fprintf(batch, " lightname=%s", light_name);
	 }
      }

   /***********************************/
   /* universal 3d parameters go here */
   /***********************************/
   if (display3d) {		/* universal 3d */
      if(!SPHERE) {
	 fprintf(batch, " rotation=%d/%d/%d", XROT, YROT, ZROT);
	 fprintf(batch, " scalexyz=%d/%d", XSCALE, YSCALE);
	 }
      fprintf(batch, " roughness=%d", ROUGH);
      fprintf(batch, " waterline=%d", WATERLINE);
      fprintf(batch, " perspective=%d", ZVIEWER);
      fprintf(batch, " xyshift=%d/%d", XSHIFT, YSHIFT);
      if (FILLTYPE > 4)
	 fprintf(batch, " lightsource=%d/%d/%d",
	     XLIGHT, YLIGHT, ZLIGHT);
      if (LIGHTAVG && FILLTYPE > 4)
	 fprintf(batch, " smoothing=%d", LIGHTAVG);
      if(xtrans || ytrans)
	 fprintf(batch, " xyadjust=%d/%d",xtrans,ytrans);
      if(glassestype) {
	 fprintf(batch, " stereo=%d",glassestype);
	 fprintf(batch, " interocular=%d",eyeseparation);
	 fprintf(batch, " converge=%d",xadjust);
	 fprintf(batch, " crop=%d/%d/%d/%d",
	     red_crop_left,red_crop_right,blue_crop_left,blue_crop_right);
	 fprintf(batch, " bright=%d/%d",
	     red_bright,blue_bright);
	 }
      }

   fprintf(batch, "\n");
   fclose(batch);
   EXIT_OVLY;
}


void shell_to_dos()
{
   char *comspec;
   /* from fractint.c & calls no ovlys, doesn't need ENTER_OVLY */
   if ((comspec = getenv("COMSPEC")) == NULL)
      printf("Cannot find COMMAND.COM.\n");
   else {
      putenv("PROMPT=Type 'EXIT' to return to FRACTINT.$_$p$g");
      spawnl(P_WAIT, comspec, NULL);
      }
}


void showfreemem()
{
   char *tempptr;
   unsigned char huge *fartempptr;
   unsigned i,i2;
   long j,j2;
   ENTER_OVLY(OVLY_MISCOVL);
   printf("\n CPU type: %d  FPU type: %d  Video: %d\n\n",
	  cpu, fpu, video_type);
   i = j = 0;
   i2 = 0x8000;
   while ((i2 >>= 1) != 0)
      if ((tempptr = malloc(i+i2)) != NULL) {
	 free(tempptr);
	 i += i2;
	 }
   printf(" %d NEAR bytes free \n", i);
   j2 = 0x80000;
   while ((j2 >>= 1) != 0)
      if ((fartempptr = (unsigned char huge *)farmemalloc(j+j2)) != NULL) {
	 farmemfree((void far*)fartempptr);
	 j += j2;
	 }
   printf(" %ld FAR bytes free \n\n press any key to continue...\n", j);
   getakey();
   EXIT_OVLY;
}


edit_text_colors()
{
   extern int debugflag;
   extern int lookatmouse;
   int save_debugflag,save_lookatmouse;
   int row,col,bkgrd;
   int rowf,colf,rowt,colt;
   char far *vidmem;
   char far *savescreen;
   char far *farp1; char far *farp2;
   int i,j,k;
   ENTER_OVLY(OVLY_MISCOVL);
   save_debugflag = debugflag;
   save_lookatmouse = lookatmouse;
   debugflag = 0;   /* don't get called recursively */
   lookatmouse = 2; /* text mouse sensitivity */
   row = col = bkgrd = rowt = rowf = colt = colf = 0;
#ifdef __TURBOC__
   vidmem = MK_FP(0xB800,0);
#else
   FP_SEG(vidmem)=0xB800;
   FP_OFF(vidmem)=0;
#endif
   while (1) {
      if (row < 0)  row = 0;
      if (row > 24) row = 24;
      if (col < 0)  col = 0;
      if (col > 79) col = 79;
      movecursor(row,col);
      i = getakey();
      if (i >= 'a' && i <= 'z') i -= 32; /* uppercase */
      switch (i) {
	 case 27: /* esc */
	    debugflag = save_debugflag;
	    lookatmouse = save_lookatmouse;
	    movecursor(25,80);
	    EXIT_OVLY;
	    return 0;
	 case '/':
	    farp1 = savescreen = farmemalloc(4000L);
	    farp2 = vidmem;
	    for (i = 0; i < 4000; ++i) { /* save and blank */
	       *(farp1++) = *farp2;
	       *(farp2++) = 0;
	       }
	    for (i = 0; i < 8; ++i)	  /* 8 bkgrd attrs */
	       for (j = 0; j < 16; ++j) { /* 16 fgrd attrs */
		  k = i*16 + j;
		  farp1 = vidmem + i*320 + j*10;
		  *(farp1++) = ' '; *(farp1++) = k;
		  *(farp1++) = i+'0'; *(farp1++) = k;
		  *(farp1++) = (j < 10) ? j+'0' : j+'A'-10; *(farp1++) = k;
		  *(farp1++) = ' '; *(farp1++) = k;
		  }
	    getakey();
	    farp1 = vidmem;
	    farp2 = savescreen;
	    for (i = 0; i < 4000; ++i) /* restore */
	       *(farp1++) = *(farp2++);
	    farmemfree(savescreen);
	    break;
	 case ',':
	    rowf = row; colf = col; break;
	 case '.':
	    rowt = row; colt = col; break;
	 case ' ': /* next color is background */
	    bkgrd = 1; break;
	 case 1075: /* cursor left  */
	    --col; break;
	 case 1077: /* cursor right */
	    ++col; break;
	 case 1072: /* cursor up    */
	    --row; break;
	 case 1080: /* cursor down  */
	    ++row; break;
	 default:
	    if (i >= '0' && i <= '9')      i -= '0';
	    else if (i >= 'A' && i <= 'F') i -= 'A'-10;
	    else break;
	    for (j = rowf; j <= rowt; ++j)
	       for (k = colf; k <= colt; ++k) {
		  farp1 = vidmem + j*160 + k*2 + 1;
		  if (bkgrd) *farp1 = (*farp1 & 15) + i * 16;
		  else	     *farp1 = (*farp1 & 0xf0) + i;
		  }
	    bkgrd = 0;
	 }
      }
}


