/*
	Overlayed odds and ends that don't fit anywhere else.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <process.h>
#include <dos.h>
#include <stdarg.h>
#include "fractint.h"
#include "fractype.h"
#include "helpdefs.h"

/* routines in this module	*/

void miscovl_overlay(void);
void make_batch_file(void);
void shell_to_dos(void);
void showfreemem(void);
int  select_video_mode(int);

static void write_batch_parms(FILE *,char *,int);
static void put_parm(char *parm,...);
static void put_parm_line(void);
static int getprec(double,double,double);
static void put_float(int,double,int);
static void put_filename(char *keyword,char *fname);
static void format_item(int choice,char *buf);
static int check_modekey(int curkey,int choice);
static int entcompare(void const *p1,void const *p2);
static void update_fractint_cfg(void);

extern int  cpu;		/* cpu type			*/
extern int  fpu;		/* fpu type			*/
extern int  iit;		/* iit fpu?			*/
extern int  video_type;
extern int  askvideo;
extern char overwrite;		/* 1 means ok to overwrite */
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
extern int  distestwidth;
extern int  init3d[20]; 	/* '3d=nn/nn/nn/...' values */
extern char floatflag;		/* floating-point fractals? */
extern int  usr_biomorph;
extern char FormFileName[];	/* file to find (type=)formulas in */
extern char FormName[]; 	/* Name of the Formula (if not null) */
extern char LFileName[];
extern char LName[];
extern char IFSFileName[];
extern char IFSName[];
extern int  bailout;		/* user input bailout value */
extern char useinitorbit;
extern struct complex initorbit;
extern int  display3d;		/* 3D display flag: 0 = OFF */
extern int  loaded3d;
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
extern int BRIEF;
extern int RAY;
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
extern int rotate_lo,rotate_hi;
extern int far *ranges;
extern int rangeslen;
extern char CommandFile[80];
extern char CommandName[ITEMNAMELEN+1];
extern char CommandComment1[57];
extern char CommandComment2[57];
extern char usr_stdcalcmode;

extern int  colorstate; 	/* comments in cmdfiles */
extern int  colors;
extern int  gotrealdac;
extern int  reallyega;
extern char colorfile[];
extern int  mapset;
extern char MAP_name[];
extern unsigned char dacbox[256][3];
extern char far *mapdacbox;

extern char dstack[4096];
extern char boxx[8192];

extern int fullscreen_prompt(char *hdg,int numprompts,char * far *prompts,
	       struct fullscreenvalues values[],int options,int fkeymask,
	       char far *extrainfo);

/* fullscreen_choice options */
#define CHOICERETURNKEY 1
#define CHOICEMENU	2
#define CHOICEHELP	4
int  fullscreen_choice(
	     int options, char *hdg, char *hdg2, char *instr, int numchoices,
	     char **choices, int *attributes, int boxwidth, int boxdepth,
	     int colwidth, int current, void (*formatitem)(),
	     char *speedstring, int (*speedprompt)(), int (*checkkey)());


void miscovl_overlay() { }	/* for restore_active_ovly */


static FILE *parmfile;

#ifdef C6
#pragma optimize("e",off)  /* MSC 6.00A messes up next rtn with "e" on */
#endif
void make_batch_file()
{
   int i,numparms;
   char inpcommandfile[80],inpcommandname[ITEMNAMELEN+1];
   char inpcomment1[57],inpcomment2[57];
   struct fullscreenvalues paramvalues[8];
   char *choices[8];
   int gotinfile;
   char outname[81],buf[256],buf2[128];
   FILE *infile;
   char colorspec[14];
   int maxcolor;
   char *sptr,*sptr2;
   int oldhelpmode;

   ENTER_OVLY(OVLY_MISCOVL);
   stackscreen();
   oldhelpmode = helpmode;
   helpmode = HELPPARMFILE;

   strcpy(colorspec,"n");
   maxcolor = colors;
   if (gotrealdac && !reallyega) {
      --maxcolor;
      if (maxit < maxcolor) maxcolor = maxit;
      if (inside  > 0 && inside    > maxcolor) maxcolor = inside;
      if (outside > 0 && outside   > maxcolor) maxcolor = outside;
      if (distest < 0 && 0-distest > maxcolor) maxcolor = 0-distest;
      if (decomp[0] > maxcolor) maxcolor = decomp[0] - 1;
      if (potflag && potparam[0] >= maxcolor) maxcolor = potparam[0];
      if (++maxcolor > 256) maxcolor = 256;
      if (colorstate == 0) {	  /* default colors */
	 if (mapdacbox) {
	    colorspec[0] = '@';
	    sptr = MAP_name;
	    }
	 }
      else if (colorstate == 2) { /* colors match colorfile */
	 colorspec[0] = '@';
	 sptr = colorfile;
	 }
      else			  /* colors match no .map that we know of */
	 colorspec[0] = 'y';
      if (colorspec[0] == '@') {
	 if ((sptr2 = strrchr(sptr,'\\'))) sptr = sptr2 + 1;
	 if ((sptr2 = strrchr(sptr,':')))  sptr = sptr2 + 1;
	 strncpy(&colorspec[1],sptr,12);
	 colorspec[13] = 0;
	 }
      }
   strcpy(inpcommandfile,CommandFile);
   strcpy(inpcommandname,CommandName);
   strcpy(inpcomment1,CommandComment1);
   strcpy(inpcomment2,CommandComment2);
   if (CommandName[0] == 0)
      strcpy(inpcommandname,"test");

   while (1) {

prompt_user:
      choices[0] = "Parameter file";
      paramvalues[0].type = 0x100+56;
      paramvalues[0].uval.sbuf = inpcommandfile;
      choices[1] = "Name";
      paramvalues[1].type = 0x100+ITEMNAMELEN;
      paramvalues[1].uval.sbuf = inpcommandname;
      choices[2] = "Main comment";
      paramvalues[2].type = 0x100+56;
      paramvalues[2].uval.sbuf = inpcomment1;
      choices[3] = "Second comment";
      paramvalues[3].type = 0x100+56;;
      paramvalues[3].uval.sbuf = inpcomment2;
      numparms = 4;
      if (gotrealdac && !reallyega) {
	 choices[4] = "Record colors?";
	 paramvalues[4].type = 0x100+13;
	 paramvalues[4].uval.sbuf = colorspec;
	 choices[5] = "    (no | yes for full info | @filename to point to a map file)";
	 paramvalues[5].type = '*';
	 choices[6] = "# of colors";
	 paramvalues[6].type = 'i';
	 paramvalues[6].uval.ival = maxcolor;
	 choices[7] = "    (if recording full color info)";
	 paramvalues[7].type = '*';
	 numparms = 8;
	 }

      if (fullscreen_prompt("Save Current Parameters",
			    numparms,choices,paramvalues,0,0,NULL) < 0)
	 break;

      strcpy(CommandFile,inpcommandfile);
      if (strchr(CommandFile,'.') == NULL)
	 strcat(CommandFile,".par"); /* default extension .par */
      strcpy(CommandName,inpcommandname);
      strcpy(CommandComment1,inpcomment1);
      strcpy(CommandComment2,inpcomment2);
      if (paramvalues[6].uval.ival > 0 && paramvalues[6].uval.ival <= 256)
	 maxcolor = paramvalues[6].uval.ival;

      strcpy(outname,CommandFile);
      gotinfile = 0;
      if (access(CommandFile,0) == 0) { /* file exists */
	 gotinfile = 1;
	 if (access(CommandFile,6)) {
	    sprintf(buf,"Can't write %s",CommandFile);
	    stopmsg(0,buf);
	    continue;
	    }
	 i = strlen(outname);
	 while (--i >= 0 && outname[i] != '\\')
	 outname[i] = 0;
	 strcat(outname,"fractint.tmp");
	 infile = fopen(CommandFile,"rt");
	 setvbuf(infile,dstack,_IOFBF,4096); /* improves speed */
	 }
      if ((parmfile = fopen(outname,"wt")) == NULL) {
	 sprintf(buf,"Can't create %s ",outname);
	 stopmsg(0,buf);
	 if (gotinfile) fclose(infile);
	 continue;
	 }
      setvbuf(infile,boxx,_IOFBF,8192); /* improves speed */

      if (gotinfile) {
	 while (file_gets(buf,255,infile) >= 0) {
	    if (strchr(buf,'{')                    /* entry heading? */
	      && sscanf(buf," %40[^ \t({]",buf2)
	      && stricmp(buf2,CommandName) == 0) { /* entry with same name */
	       sprintf(buf2,"File already has an entry named %s\n\
Continue to replace it, Cancel to back out",CommandName);
	       if (stopmsg(18,buf2) < 0) {	    /* cancel */
		  fclose(infile);
		  fclose(parmfile);
		  unlink(outname);
		  goto prompt_user;
		  }
	       while (strchr(buf,'}') == NULL
		 && file_gets(buf,255,infile) > 0 ) { } /* skip to end of set */
	       break;
	       }
	    fputs(buf,parmfile);
	    fputc('\n',parmfile);
	    }
	 }

      fprintf(parmfile,"%-19s{",CommandName);
      if (CommandComment1[0]) fprintf(parmfile," ; %s",CommandComment1);
      fputc('\n',parmfile);
      if (CommandComment2[0])
	 fprintf(parmfile,"                     ; %s\n",CommandComment2);
      write_batch_parms(parmfile,colorspec,maxcolor); /* write the parameters */
      fprintf(parmfile,"  }\n\n");

      if (gotinfile) {	/* copy the rest of the file */
	 while ((i = file_gets(buf,255,infile)) == 0) { } /* skip blanks */
	 while (i >= 0) {
	    fputs(buf,parmfile);
	    fputc('\n',parmfile);
	    i = file_gets(buf,255,infile);
	    }
	 fclose(infile);
	 }
      fclose(parmfile);
      if (gotinfile) {	/* replace the original file with the new */
	 unlink(CommandFile);	      /* success assumed on these lines       */
	 rename(outname,CommandFile); /* since we checked earlier with access */
	 }

      break;
      }

   helpmode = oldhelpmode;
   unstackscreen();
   EXIT_OVLY;
}
#ifdef C6
#pragma optimize("e",on)  /* back to normal */
#endif

static struct write_batch_data { /* buffer for parms to break lines nicely */
   int len;
   char buf[513];
   } *wbdata;

static void write_batch_parms(FILE *batch,char *colorinf,int maxcolor)
{
   int i,j,k;
   double Xctr, Yctr, Magnification;
   struct write_batch_data wb_data;
   char *sptr;
   char buf[81];

   wbdata = &wb_data;
   wb_data.len = 0; /* force first parm to start on new line */

   if (display3d <= 0) { /* a fractal was generated */

      /****** fractal only parameters in this section *******/
      put_parm(" reset");

      if (*(sptr = curfractalspecific->name) == '*') ++sptr;
      put_parm( " type=%s",sptr);

      if (fractype == FORMULA || fractype == FFORMULA)
	 put_parm( " formulafile=%s formulaname=%s",FormFileName,FormName);
      if (fractype == LSYSTEM)
	put_parm( " lfile=%s lname=%s",LFileName,LName);
      if (fractype == IFS || fractype == IFS3D)
	put_parm( " ifsfile=%s ifs=%s",IFSFileName,IFSName);

      showtrig(buf); /* this function is in miscres.c */
      if (buf[0])
	 put_parm(buf);

      if (usr_stdcalcmode != 'g')
	 put_parm(" passes=%c",usr_stdcalcmode);

      if (usemag && cvtcentermag(&Xctr, &Yctr, &Magnification)) {
	 put_parm(" center-mag=");
	 put_parm((delmin > 1000) ? "%g/%g/%g"
				  : "%+20.17lf/%+20.17lf/%+20.17lf",
		  Xctr,Yctr,Magnification);
	 }
      else {
	 int xdigits,ydigits;
	 put_parm( " corners=");
	 xdigits = getprec(xxmin,xxmax,xx3rd);
	 ydigits = getprec(yymin,yymax,yy3rd);
	 put_float(0,xxmin,xdigits);
	 put_float(1,xxmax,xdigits);
	 put_float(1,yymin,ydigits);
	 put_float(1,yymax,ydigits);
	 if (xx3rd != xxmin || yy3rd != yymin) {
	    put_float(1,xx3rd,xdigits);
	    put_float(1,yy3rd,ydigits);
	    }
	 }

      for (i = 3; i >= 0; --i)
	 if (param[i] != 0.0) break;
      if (i >= 0) {
	 put_parm(" params=%.15g",param[0]);
	 for (j = 1; j <= i; ++j)
	    put_parm("/%.15g",param[j]);
	 }

      if(useinitorbit == 2)
	 put_parm( " initorbit=pixel");
      else if(useinitorbit == 1)
	 put_parm( " initorbit=%.15g/%.15g",initorbit.x,initorbit.y);

      if (floatflag)
	 put_parm( " float=y");

      if (maxit != 150)
	 put_parm(" maxiter=%d",maxit);

      if(bailout && (potflag == 0 || potparam[2] == 0.0))
	 put_parm( " bailout=%d",bailout);

      if (inside != 1) {
	 put_parm(" inside=");
	 if (inside == -1)
	    put_parm( "maxiter");
	 else if (inside == -60)
	    put_parm("bof60");
	 else if (inside == -61)
	    put_parm("bof61");
	 else
	    put_parm( "%d",inside);
	 }
      if (outside != -1)
	 put_parm(" outside=%d",outside);

      if(LogFlag) {
	 put_parm( " logmap=");
	 if(LogFlag == -1)
	    put_parm( "old");
	 else if(LogFlag == 1)
	    put_parm( "yes");
	 else
	    put_parm( "%d", LogFlag);
	 }

      if (potflag) {
	 put_parm( " potential=%d/%d/%d",
	     (int)potparam[0],(int)potparam[1],(int)potparam[2]);
	 if(pot16bit)
	    put_parm( "/16bit");
	 }
      if (invert)
	 put_parm( " invert=%g/%g/%g",
	     inversion[0], inversion[1], inversion[2]);
      if (decomp[0])
	 put_parm( " decomp=%d", decomp[0]);
      if (distest)
	 put_parm( " distest=%d/%d", distest, distestwidth);
      if (usr_biomorph != -1)
	 put_parm( " biomorph=%d", usr_biomorph);
      if (finattract)
	 put_parm(" finattract=y");

      if (forcesymmetry != 999) {
	 put_parm( " symmetry=");
	 if (forcesymmetry==XAXIS)
	    put_parm("xaxis");
	 else if(forcesymmetry==YAXIS)
	    put_parm("yaxis");
	 else if(forcesymmetry==XYAXIS)
	    put_parm("xyaxis");
	 else if(forcesymmetry==ORIGIN)
	    put_parm("origin");
	 else if(forcesymmetry==PI_SYM)
	    put_parm("pi");
	 else
	    put_parm("none");
	 }

      if (periodicitycheck != 1)
	 put_parm( " periodicity=%d",periodicitycheck);

      if (rflag)
	 put_parm( " rseed=%d",rseed);

      if (rangeslen) {
	 put_parm(" ranges=");
	 i = 0;
	 while (i < rangeslen) {
	    if (i)
	       put_parm("/");
	    if (ranges[i] == -1) {
	       put_parm("-%d/",ranges[++i]);
	       ++i;
	       }
	    put_parm("%d",ranges[i++]);
	    }
	 }
      }

   if (display3d >= 1) {
      /***** 3d transform only parameters in this section *****/
      put_parm( " 3d=yes");
      if (loaded3d == 0)
	 put_filename("filename",readname);
      if (SPHERE) {
	 put_parm( " sphere=y");
	 put_parm( " latitude=%d/%d", THETA1, THETA2);
	 put_parm( " longitude=%d/%d", PHI1, PHI2);
	 put_parm( " radius=%d", RADIUS);
	 }
      put_parm( " scalexyz=%d/%d", XSCALE, YSCALE);
      put_parm( " roughness=%d", ROUGH);
      put_parm( " waterline=%d", WATERLINE);
      if (FILLTYPE)
	 put_parm( " filltype=%d", FILLTYPE);
      if (transparent[0] || transparent[1])
	 put_parm( " transparent=%d/%d", transparent[0],transparent[1]);
      if (preview) {
	 put_parm( " preview=y");
	 if (showbox)
	    put_parm( " showbox=y");
	 put_parm( " coarse=%d",previewfactor);
	 }
      if (RAY) {
	 put_parm( " ray=%d",RAY);
	 if (BRIEF)
	    put_parm(" brief=y");
	 }
      if (FILLTYPE > 4) {
	 put_parm( " lightsource=%d/%d/%d", XLIGHT, YLIGHT, ZLIGHT);
	 if (LIGHTAVG)
	    put_parm( " smoothing=%d", LIGHTAVG);
	 }
      if (RANDOMIZE)
	 put_parm( " randomize=%d",RANDOMIZE);
      if (full_color)
	 put_parm( " fullcolor=y");
      if (Ambient)
	 put_parm( " ambient=%d",Ambient);
      if (haze)
	 put_parm( " haze=%d",haze);
      }

   if (display3d) {		/* universal 3d */
      /***** common (fractal & transform) 3d parameters in this section *****/
      if (!SPHERE || display3d < 0)
	 put_parm( " rotation=%d/%d/%d", XROT, YROT, ZROT);
      put_parm( " perspective=%d", ZVIEWER);
      put_parm( " xyshift=%d/%d", XSHIFT, YSHIFT);
      if(xtrans || ytrans)
	 put_parm( " xyadjust=%d/%d",xtrans,ytrans);
      if(glassestype) {
	 put_parm( " stereo=%d",glassestype);
	 put_parm( " interocular=%d",eyeseparation);
	 put_parm( " converge=%d",xadjust);
	 put_parm( " crop=%d/%d/%d/%d",
	     red_crop_left,red_crop_right,blue_crop_left,blue_crop_right);
	 put_parm( " bright=%d/%d",
	     red_bright,blue_bright);
	 }
      }

   /***** universal parameters in this section *****/

   if (*colorinf != 'n') {
      put_parm(" colors=");
      if (*colorinf == '@')
	 put_parm(colorinf);
      else {
	 int curc,scanc,force,diffmag;
	 int delta,diff1[4][3],diff2[4][3];
	 curc = force = 0;
	 while (1) {
	    /* emit color in rgb 3 char encoded form */
	    for (j = 0; j < 3; ++j) {
	       if ((k = dacbox[curc][j]) < 10) k += '0';
	       else if (k < 36) 	       k += ('A' - 10);
	       else			       k += ('_' - 36);
	       buf[j] = k;
	       }
	    buf[3] = 0;
	    put_parm(buf);
	    if (++curc >= maxcolor)	 /* quit if done last color */
	       break;
	    /* Next a P Branderhorst special, a tricky scan for smooth-shaded
	       ranges which can be written as <nn> to compress .par file entry.
	       Method used is to check net change in each color value over
	       spans of 2 to 5 color numbers.  First time for each span size
	       the value change is noted.  After first time the change is
	       checked against noted change.  First time it differs, a
	       a difference of 1 is tolerated and noted as an alternate
	       acceptable change.  When change is not one of the tolerated
	       values, loop exits. */
	    if (force) {
	       --force;
	       continue;
	       }
	    scanc = curc;
	    while (scanc < maxcolor) {	 /* scan while same diff to next */
	       if ((i = scanc - curc) > 3) /* check spans up to 4 steps */
		  i = 3;
	       for (k = 0; k <= i; ++k) {
		  for (j = 0; j < 3; ++j) { /* check pattern of chg per color */
		     delta = (int)dacbox[scanc][j] - (int)dacbox[scanc-k-1][j];
		     if (k == scanc - curc)
			diff1[k][j] = diff2[k][j] = delta;
		     else
			if (delta != diff1[k][j] && delta != diff2[k][j]) {
			   diffmag = abs(delta - diff1[k][j]);
			   if (diff1[k][j] != diff2[k][j] || diffmag != 1)
			      break;
			   diff2[k][j] = delta;
			   }
		     }
		  if (j < 3) break; /* must've exited from inner loop above */
		  }
	       if (k <= i) break;   /* must've exited from inner loop above */
	       ++scanc;
	       }
	    /* now scanc-1 is next color which must be written explicitly */
	    if (scanc - curc > 2) { /* good, we have a shaded range */
	       if (scanc != maxcolor) {
		  if (diffmag < 3) {  /* not a sharp slope change? */
		     force = 2;       /* force more between ranges, to stop  */
		     --scanc;	      /* "drift" when load/store/load/store/ */
		     }
		  if (k) {	      /* more of the same		     */
		     force += k;
		     --scanc;
		     }
		  }
	       if (--scanc - curc > 1) {
		  put_parm("<%d>",scanc-curc);
		  curc = scanc;
		  }
	       else		   /* changed our mind */
		  force = 0;
	       }
	    }
	 }
      }

   if (rotate_lo != 1 || rotate_hi != 255)
      put_parm( " cyclerange=%d/%d",rotate_lo,rotate_hi);

   while (wbdata->len) /* flush the buffer */
      put_parm_line();
}

static void put_filename(char *keyword,char *fname)
{
   char *p;
   if (*fname && !endswithslash(fname)) {
      if ((p = strrchr(fname,'\\')))
	 if (*(fname = p+1) == 0) return;
      put_parm(" %s=%s",keyword,fname);
      }
}

static void put_parm(char *parm,...)
{
   char *bufptr;
   va_list args;
   va_start(args,parm);
   if (*parm == ' '             /* starting a new parm */
     && wbdata->len == 0)	/* skip leading space */
      ++parm;
   bufptr = wbdata->buf + wbdata->len;
   vsprintf(bufptr,parm,args);
   while (*(bufptr++))
      ++wbdata->len;
   while (wbdata->len > 200)
      put_parm_line();
}

#define NICELINELEN 72
#define MAXLINELEN  76

static void put_parm_line()
{
   int len,c;
   if ((len = wbdata->len) > NICELINELEN) {
      len = NICELINELEN+1;
      while (--len != 0 && wbdata->buf[len] != ' ') { }
      if (len == 0) {
	 len = NICELINELEN-1;
	 while (++len < MAXLINELEN
	   && wbdata->buf[len] && wbdata->buf[len] != ' ') { }
	 }
      }
   c = wbdata->buf[len];
   wbdata->buf[len] = 0;
   fputs("  ",parmfile);
   fputs(wbdata->buf,parmfile);
   if (c && c != ' ')
      fputc('\\',parmfile);
   fputc('\n',parmfile);
   if ((wbdata->buf[len] = c) == ' ')
      ++len;
   wbdata->len -= len;
   strcpy(wbdata->buf,wbdata->buf+len);
}

static int getprec(double a,double b,double c)
{
   double diff,temp;
   int digits;
   double highv = 1.0E20;
   if ((diff = fabs(a - b)) == 0.0) diff = highv;
   if ((temp = fabs(a - c)) == 0.0) temp = highv;
   if (temp < diff) diff = temp;
   if ((temp = fabs(b - c)) == 0.0) temp = highv;
   if (temp < diff) diff = temp;
   digits = 5;
   while (diff < 1.0 && digits < 17) {
      diff *= 10;
      ++digits;
      }
   return((digits < 6) ? 6 : digits);
}

static void put_float(int slash,double fnum,int prec)
{  char buf[40];
   char *bptr, *dptr;
   bptr = buf;
   if (slash)
      *(bptr++) = '/';
   sprintf(bptr,"%1.*f",prec,fnum);
   if ((dptr = strchr(bptr,'.'))) {
      ++dptr;
      bptr = buf + strlen(buf);
      while (--bptr > dptr && *bptr == '0')
	 *bptr = 0;
      }
   put_parm(buf);
}

void shell_to_dos()
{
   char *comspec;
   /* from fractint.c & calls no ovlys, doesn't need ENTER_OVLY */
   if ((comspec = getenv("COMSPEC")) == NULL)
      printf("Cannot find COMMAND.COM.\n");
   else {
      putenv("PROMPT='EXIT' returns to FRACTINT.$_$p$g");
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
   printf("\n CPU type: %d  FPU type: %d  IIT FPU: %d  Video: %d\n\n",
	  cpu, fpu, iit, video_type);
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
   vidmem = MK_FP(0xB800,0);
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
	 case 13:   /* enter */
	    *(vidmem + row*160 + col*2) = getakey();
	    break;
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


extern int badconfig;
extern struct videoinfo far videotable[];
extern struct videoinfo far *vidtbl;
extern int vidtbllen;
extern int tabmode;
extern int adapter;
static int *entsptr;
static int modes_changed;
extern int mode7text;

int select_video_mode(int curmode)
{
   int entnums[MAXVIDEOMODES];
   int attributes[MAXVIDEOMODES];
   int i,j,k,ret;
   int oldtabmode,oldhelpmode;

   ENTER_OVLY(OVLY_MISCOVL);

   load_fractint_cfg(0);	/* load fractint.cfg to extraseg */

   for (i = 0; i < vidtbllen; ++i) { /* init tables */
      entnums[i] = i;
      attributes[i] = 1;
      }
   entsptr = entnums;		/* for indirectly called subroutines */

   qsort(entnums,vidtbllen,sizeof(entnums[0]),entcompare); /* sort modes */

   /* pick default mode */
   if (curmode < 0) {
      switch (video_type) { /* set up a reasonable default (we hope) */
	 case 1:  videoentry.videomodeax = 8;	/* hgc */
		  videoentry.colors = 2;
		  break;
	 case 2:  videoentry.videomodeax = 4;	/* cga */
		  videoentry.colors = 4;
		  break;
	 case 3:  videoentry.videomodeax = 16;	/* ega */
		  videoentry.colors = 16;
		  if (mode7text) {		/* egamono */
		     videoentry.videomodeax = 15;
		     videoentry.colors = 2;
		     }
		  break;
	 default: videoentry.videomodeax = 19;	/* mcga/vga? */
		  videoentry.colors = 256;
		  break;
	 }
      }
   else
      far_memcpy((char far *)&videoentry,(char far *)&videotable[curmode],
		 sizeof(videoentry));
   for (i = 0; i < vidtbllen; ++i) { /* find default mode */
      if ( videoentry.videomodeax == vidtbl[entnums[i]].videomodeax
	&& videoentry.colors	  == vidtbl[entnums[i]].colors
	&& (curmode < 0
	    || far_memcmp((char far *)&videoentry,(char far *)&vidtbl[entnums[i]],
			  sizeof(videoentry)) == 0))
	 break;
      }
   if (i >= vidtbllen) /* no match, default to first entry */
      i = 0;

   oldtabmode = tabmode;
   oldhelpmode = helpmode;
   modes_changed = 0;
   tabmode = 0;
   helpmode = HELPVIDSEL;
   i = fullscreen_choice(CHOICEHELP,"Select Video Mode",
 "key...name......................xdot.ydot.colr.comment..................",
		  NULL,vidtbllen,NULL,attributes,
		  1,16,72,i,format_item,NULL,NULL,check_modekey);
   tabmode = oldtabmode;
   helpmode = oldhelpmode;
   if (i == -1) {
   static char far msg[]={"Save new function key assignments or cancel changes?"};
      if (modes_changed /* update fractint.cfg for new key assignments */
	&& badconfig == 0
	&& stopmsg(22,msg) == 0)
	 update_fractint_cfg();
      EXIT_OVLY;
      return(-1);
      }
   if (i < 0)	/* picked by function key */
      i = -1 - i;
   else 	/* picked by Enter key */
      i = entnums[i];
   far_memcpy((char far *)&videoentry,(char far *)&vidtbl[i],
	      sizeof(videoentry));  /* the selected entry now in videoentry */

   /* copy fractint.cfg table to resident table, note selected entry */
   j = k = 0;
   far_memset((char far *)videotable,0,sizeof(*vidtbl)*MAXVIDEOTABLE);
   for (i = 0; i < vidtbllen; ++i) {
      if (vidtbl[i].keynum > 0) {
	 far_memcpy((char far *)&videotable[j],(char far *)&vidtbl[i],
		    sizeof(*vidtbl));
	 if (far_memcmp((char far *)&videoentry,(char far *)&vidtbl[i],
			sizeof(videoentry)) == 0)
	    k = vidtbl[i].keynum;
	 if (++j >= MAXVIDEOTABLE-1)
	    break;
	 }
      }
   if ((ret = k) == 0) { /* selected entry not a copied (assigned to key) one */
      far_memcpy((char far *)&videotable[MAXVIDEOTABLE-1],
		 (char far *)&videoentry,sizeof(*vidtbl));
      ret = 1400; /* special value for check_vidmode_key */
      }

   if (modes_changed /* update fractint.cfg for new key assignments */
     && badconfig == 0)
      update_fractint_cfg();

   EXIT_OVLY;
   return(ret);
}

static void format_item(int choice,char *buf)
{
   char kname[5];
   char biosflag;
   far_memcpy((char far *)&videoentry,(char far *)&vidtbl[entsptr[choice]],
	      sizeof(videoentry));
   vidmode_keyname(videoentry.keynum,kname);
   biosflag = (videoentry.dotmode % 100 == 1) ? 'B' : ' ';
   sprintf(buf,"%-5s %-25s %4d %4d %3d%c %-25s",  /* 72 chars */
	   kname, videoentry.name, videoentry.xdots, videoentry.ydots,
	   videoentry.colors, biosflag, videoentry.comment);
}

static int check_modekey(int curkey,int choice)
{
   int i,j,k,ret;
   if ((i = check_vidmode_key(1,curkey)) >= 0)
      return(-1-i);
   i = entsptr[choice];
   ret = 0;
   if ( (curkey == '-' || curkey == '+')
     && (vidtbl[i].keynum == 0 || vidtbl[i].keynum >= 1084)) {
      static char far msg[]={"Missing or bad FRACTINT.CFG file. Can't reassign keys."};
      if (badconfig)
	 stopmsg(0,msg);
      else {
	 if (curkey == '-') {                   /* deassign key? */
	    if (vidtbl[i].keynum >= 1084) {
	       vidtbl[i].keynum = 0;
	       modes_changed = 1;
	       }
	    }
	 else { 				/* assign key? */
	    j = getakeynohelp();
	    if (j >= 1084 && j <= 1113) {
	       for (k = 0; k < vidtbllen; ++k) {
		  if (vidtbl[k].keynum == j) {
		     vidtbl[k].keynum = 0;
		     ret = -1; /* force redisplay */
		     }
		  }
	       vidtbl[i].keynum = j;
	       modes_changed = 1;
	       }
	    }
	 }
      }
   return(ret);
}

static int entcompare(void const *p1,void const *p2)
{
   int i,j;
   if ((i = vidtbl[*((int *)p1)].keynum) == 0) i = 9999;
   if ((j = vidtbl[*((int *)p2)].keynum) == 0) j = 9999;
   if (i < j || (i == j && *((int *)p1) < *((int *)p2)))
      return(-1);
   return(1);
}

static void update_fractint_cfg()
{
   char cfgname[100],outname[100],buf[121],kname[5];
   FILE *cfgfile,*outfile;
   int far *cfglinenums;
   int i,j,linenum,nextlinenum,nextmode;
   struct videoinfo vident;

   findpath("fractint.cfg",cfgname);
   if (access(cfgname,6)) {
      sprintf(buf,"Can't write %s",cfgname);
      stopmsg(0,buf);
      return;
      }
   strcpy(outname,cfgname);
   i = strlen(outname);
   while (--i >= 0 && outname[i] != '\\')
   outname[i] = 0;
   strcat(outname,"fractint.tmp");
   if ((outfile = fopen(outname,"w")) == NULL) {
      sprintf(buf,"Can't create %s ",outname);
      stopmsg(0,buf);
      return;
      }
   cfgfile = fopen(cfgname,"r");

   cfglinenums = (int far *)(&vidtbl[MAXVIDEOMODES]);
   linenum = nextmode = 0;
   nextlinenum = cfglinenums[0];
   while (fgets(buf,120,cfgfile)) {
      ++linenum;
      if (linenum == nextlinenum) { /* replace this line */
	 far_memcpy((char far *)&vident,(char far *)&vidtbl[nextmode],
		    sizeof(videoentry));
	 vidmode_keyname(vident.keynum,kname);
	 strcpy(buf,vident.name);
	 i = strlen(buf);
	 while (i && buf[i-1] == ' ') /* strip trailing spaces to compress */
	    --i;
	 j = i + 5;
	 while (j < 32) {		/* tab to column 33 */
	    buf[i++] = '\t';
	    j += 8;
	    }
	 buf[i] = 0;
	 fprintf(outfile,"%-4s,%s,%4x,%4x,%4x,%4x,%4d,%4d,%4d,%3d,%s\n",
		kname,
		buf,
		vident.videomodeax,
		vident.videomodebx,
		vident.videomodecx,
		vident.videomodedx,
		vident.dotmode,
		vident.xdots,
		vident.ydots,
		vident.colors,
		vident.comment);
	 if (++nextmode >= vidtbllen)
	    nextlinenum = 32767;
	 else
	    nextlinenum = cfglinenums[nextmode];
	 }
      else
	 fputs(buf,outfile);
      }

   fclose(cfgfile);
   fclose(outfile);
   unlink(cfgname);	    /* success assumed on these lines	    */
   rename(outname,cfgname); /* since we checked earlier with access */
}

