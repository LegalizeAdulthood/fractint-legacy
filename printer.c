/*  Printer.c
 *	This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.
 *	Simple screen printing functions for FRACTINT
 *	By Matt Saucier CIS: [72371,3101]      7/2/89
 *	"True-to-the-spirit" of FRACTINT, this code makes few checks that you
 *	have specified a valid resolution for the printer (just in case yours
 *	has more dots/line than the Standard HP and IBM/EPSON,
 *	(eg, Wide Carriage, etc.))
 *
 *	PostScript support by Scott Taylor [72401,410] / (DGWM18A)   10/8/90
 *	For PostScript, use 'printer=PostScript/resolution' where resolution
 *	is ANY NUMBER between 10 and 600. Common values: 300,150,100,75.
 *	Default resolution for PostScript is 150 pixels/inch.
 *	At 200 DPI, a fractal that is 640x480 prints as a 3.2"x2.4" picture.
 *	PostScript printer names:
 *
 *	PostScript/PS			= Portrait printing
 *	PostScriptH/PostScriptL/PSH/PSL = Landscape printing
 *
 *	This code supports printers attached to a LPTx (1-3) parallel port.
 *	It also now supports serial printers AFTER THEY ARE CONFIGURED AND
 *	WORKING WITH THE DOS MODE COMMAND, eg. MODE COM1:9600,n,8,1 (for HP)
 *	(NOW you can also configure the serial port with the comport= command)
 *	Printing calls are made directly to the BIOS for DOS can't handle fast
 *	transfer of data to the HP.  (Or maybe visa-versa, HP can't handle the
 *	slow transfer of data from DOS)
 *
 *	I just added direct port access for COM1 and COM2 **ONLY**. This method
 *	does a little more testing than BIOS, and may work (especially on
 *	serial printer sharing devices) where the old method doesn't. I noticed
 *	maybe a 5% speed increase at 9600 baud. These are selected in the
 *	printer=.../.../31 for COM1 or 32 for COM2.
 *
 *	I also added direct parallel port access for LPT1 and LPT2 **ONLY**.
 *	This toggles the "INIT" line of the parallel port to reset the printer
 *	for each print session. It will also WAIT for a error / out of paper /
 *	not selected condition instead of quitting with an error.
 *
 *	Supported Printers:	Tested Ok:
 *	 HP LaserJet
 *	    LJ+,LJII		 MDS
 *	 Toshiba PageLaser	 MDS (Set FRACTINT to use HP)
 *	 IBM Graphics		 MDS
 *	 EPSON
 *	    Models?		 Untested.
 *	 IBM LaserPrinter
 *	    with PostScript	 SWT
 *	 HP Plotter		 SWT
 *
 *	Future support to include OKI 20 (color) printer, and just about
 *	any printer you request.
 *
 *	Future modifications to include a more flexible, standard interface
 *	with the surrounding program, for easier portability to other
 *	programs.
 *
 * PostScript Styles:
 *  0  Dot
 *  1  Dot*	       [Smoother]
 *  2  Inverted Dot
 *  3  Ring
 *  4  Inverted Ring
 *  5  Triangle        [45-45-90]
 *  6  Triangle*       [30-75-75]
 *  7  Grid
 *  8  Diamond
 *  9  Line
 * 10  Microwaves
 * 11  Ellipse
 * 12  RoundBox
 * 13  Custom
 * 14  Star
 * 15  Random
 * 16  Line*	       [Not much different]
 *
 *  *  Alternate style
 */

#include <stdlib.h>
#include <bios.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <errno.h>
#include <stdio.h>	/*** for vsprintf prototype ***/
#include <conio.h>
#include <string.h>
#include <stdarg.h>
#include <float.h>	/* for pow() */
#include <math.h>	/*  "    "   */
#include "fractint.h"
#include "fractype.h"

/********      PROTOTYPES     ********/

       void printer_overlay(void);
       void Print_Screen(void);
static void Printer_printf(char *fmt,...);
static int  _fastcall printer(int c);
static void _fastcall print_title(int,int,char *);
static void printer_reset();

extern int keypressed (void);
extern void updatesavename (char *);
extern void showtrig (char *);

/********  EXTRN GLOBAL VARS  ********/

extern int xdots,ydots, 	       /* size of screen		   */
	   extraseg,		       /* used for buffering		   */
	   fractype;		       /* used for title block		   */
extern unsigned char dacbox[256][3];   /* for PostScript printing	   */
extern unsigned char dstack[2][3][400];
extern char FormName[]; 	       /* for Title block info		   */
extern char LName[];		       /* for Title block info		   */
extern char IFSName[];		       /* for Title block info		   */
extern char PrintName[];	       /* Filename for print-to-file	   */
extern float finalaspectratio;
extern double xxmin,xxmax,xx3rd,
	      yymin,yymax,yy3rd,param[4]; /* for Title block info	   */
extern int colors;
extern int dotmode;
extern unsigned int debugflag;

extern unsigned int  far pj_patterns[];
extern unsigned char far pj_reds[];
extern unsigned char far pj_blues[];
extern unsigned char far pj_greens[];

extern int thinking(int,char *);

/********	GLOBALS       ********/

int Printer_Resolution,        /* 75,100,150,300 for HP;		   */
			       /* 60,120,240 for IBM;			   */
			       /* 90 or 180 for the PaintJet;		   */
			       /* 10-600 for PS 			   */
			       /* 1-20 for Plotter			   */
    LPTNumber,		       /* ==1,2,3 LPTx; or 11,12,13,14 for COM1-4  */
			       /* 21,22 for direct port access for LPT1-2  */
			       /* 31,32 for direct port access for COM1-2  */
    Printer_Type,		       /* ==1 HP,
					  ==2 IBM/EPSON,
					  ==3 Epson color,
					  ==4 HP PaintJet,
					  ==5,6 PostScript,
					  ==7 HP Plotter		   */
    Printer_Titleblock,       /* Print info about the fractal?		   */
    Printer_ColorXlat,	      /* PostScript only - invert colors	   */
    Printer_SetScreen,	      /* PostScript only - reprogram halftone ?    */
    Printer_SFrequency,       /* PostScript only - Halftone Frequency K    */
    Printer_SAngle,           /* PostScript only - Halftone angle     K    */
    Printer_SStyle,           /* PostScript only - Halftone style     K    */
    Printer_RFrequency,       /* PostScript only - Halftone Frequency R    */
    Printer_RAngle,           /* PostScript only - Halftone angle     R    */
    Printer_RStyle,           /* PostScript only - Halftone style     R    */
    Printer_GFrequency,       /* PostScript only - Halftone Frequency G    */
    Printer_GAngle,           /* PostScript only - Halftone angle     G    */
    Printer_GStyle,           /* PostScript only - Halftone style     G    */
    Printer_BFrequency,       /* PostScript only - Halftone Frequency B    */
    Printer_BAngle,           /* PostScript only - Halftone angle     B    */
    Printer_BStyle,           /* PostScript only - Halftone style     B    */
    Print_To_File,	      /* Print to file toggle			   */
    EPSFileType,	      /* EPSFileType -
					       1 = well-behaved,
					       2 = much less behaved,
					       3 = not well behaved	   */
    Printer_CRLF,             /* (0) CRLF (1) CR (2) LF                    */
    ColorPS;                  /* (0) B&W  (1) Color                        */

static int LPTn;		   /* printer number we're gonna use */

static FILE *PRFILE;

#define TONES 17		   /* Number of PostScript halftone styles */

static char *HalfTone[TONES]=  {
			 "dup mul exch dup mul add 1 exch sub",
			 "abs exch abs 2 copy add 1 gt {1 sub dup mul exch 1 sub dup mul add 1 sub} {dup mul exch dup mul add 1 exch sub} ifelse",
			 "dup mul exch dup mul add 1 sub",
			 "dup mul exch dup mul add 0.6 exch sub abs -0.5 mul",
			 "dup mul exch dup mul add 0.6 exch sub abs 0.5 mul",
			 "add 2 div",
			 "2 exch sub exch abs 2 mul sub 3 div",
			 "2 copy abs exch abs gt {exch} if pop 2 mul 1 exch sub 3.5 div",
			 "abs exch abs add 1 exch sub",
			 "pop",
			 "/wy exch def 180 mul cos 2 div wy dup dup dup mul mul sub mul wy add 180 mul cos",
			 "dup 5 mul 8 div mul exch dup mul exch add sqrt 1 exch sub",
			 "dup mul dup mul exch dup mul dup mul add 1 exch sub",
			 "dup mul exch dup mul add sqrt 1 exch sub",
			 "abs exch abs 2 copy gt {exch} if 1 sub dup 0 eq {0.01 add} if atan 360 div",
			 "pop pop rand 1 add 10240 mod 5120 div 1 exch sub",
			 "pop abs 2 mul 1 exch sub"
			};

void printer_overlay() { }	/* for restore_active_ovly */

#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn -eff
#endif
#endif

void Print_Screen()
{
    int y,j;
    char buff[192];		/* buffer for 192 sets of pixels  */
				/* This is very large so that we can*/
				/* get reasonable times printing  */
				/* from modes like 2048x2048 disk-*/
				/* video.  When this was 24, a 2048*/
				/* by 2048 pic took over 2 hours to*/
				/* print.  It takes about 15 min now*/
    int BuffSiz;		/* how much of buff[] we'll use   */
    char far *es;		/* pointer to extraseg for buffer */
    int i,x,k,			/* more indices 		  */
	imax,			/* maximum i value (ydots/8)	  */
	res,			/* resolution we're gonna' use    */
	high,			/* if LPTn>10 COM == com port to use*/
	low,			/* misc 			  */
				/************************************/
	ptrid;			/* Printer Id code.		  */
				/* Currently, the following are   */
				/* assigned:			  */
				/*	      1. HPLJ (all)	  */
				/*		 Toshiba PageLaser*/
				/*	      2. IBM Graphics	  */
				/*	      3. Color Printer	  */
				/*	      4. HP PaintJet	  */
				/*	      5. PostScript	  */
				/************************************/
    double ci,ck;
    int pj_width;
    int pj_color_ptr[256];	/* Paintjet color translation */
    char EndOfLine[3];

    ENTER_OVLY(OVLY_PRINTER);
				/********   SETUP VARIABLES  ********/
    memset(buff,0,192);

    EndOfLine[0]=(((Printer_CRLF==1) || (Printer_CRLF==0)) ? 0x0D : 0x0A);
    EndOfLine[1]=((Printer_CRLF==0) ? 0x0A : 0x00);
    EndOfLine[2]=0x00;

    if (Print_To_File>0)
      {
      while ((PRFILE = fopen(PrintName,"r"))) {
	 j = fgetc(PRFILE);
	 fclose(PRFILE);
	 if (j == EOF) break;
	 updatesavename((char *)PrintName);
	 }
      if ((PRFILE = fopen(PrintName,"wb"))==NULL) Print_To_File = 0;
      }

    es=MK_FP(extraseg,0);

    LPTn=LPTNumber-1;
    if (((LPTn>2)&&(LPTn<10))||
	((LPTn>13)&&(LPTn<20))||
	((LPTn>21)&&(LPTn<30))||
	(LPTn<0)||(LPTn>31)) LPTn=0;   /* default of LPT1 (==0) 	 */
    ptrid=Printer_Type;
    if ((ptrid<1)||(ptrid>7)) ptrid=2; /* default of IBM/EPSON		 */
    res=Printer_Resolution;
    if ((LPTn==20)||(LPTn==21))
	{
	k = (inp((LPTn==20) ? 0x37A : 0x27A)) & 0xF7;
	outp((LPTn==20) ? 0x37A : 0x27A,k);
	k = k & 0xFB;
	outp((LPTn==20) ? 0x37A : 0x27A,k);
	k = k | 0x0C;
	outp((LPTn==20) ? 0x37A : 0x27A,k);
	}
    if ((LPTn==30)||(LPTn==31))
	{
	outp((LPTn==30) ? 0x3F9 : 0x2F9,0x00);
	outp((LPTn==30) ? 0x3FC : 0x2FC,0x00);
	outp((LPTn==30) ? 0x3FC : 0x2FC,0x03);
	}

    switch (ptrid) {

	case 1:
	    if (res<75) res=75;
	    if ( (res<= 75)&&(ydots> 600)) res=100;
	    if ( (res<=100)&&(ydots> 800)) res=150;
	    if (((res<=150)&&(ydots>1200))||(res>300)) res=300;
	    break;

	case 2:
	case 3:
	    if (res<60) res=60;
	    if ((res<=60)&&(ydots>480)) res=120;
	    if (((res<=120)&&(ydots>960))||(res>240)) res=240;
	    break;

	case 4: /****** PaintJet  *****/
	    {
	    /* Pieter Branderhorst:
	       My apologies if the numbers and approach here seem to be
	       picked out of a hat.  They were.  They happen to result in
	       a tolerable mapping of screen colors to printer colors on
	       my machine.  There are two sources of error in getting colors
	       to come out right.
	       1) Must match some dacbox values to the 330 PaintJet dithered
		  colors so that they look the same.  For this we use HP's
		  color values in printera.asm and modify by gamma separately
		  for each of red/green/blue.  This mapping is ok if the
		  preview shown on screen is a fairly close match to what
		  gets printed. The defaults are what work for me.
	       2) Must find nearest color in HP palette to each color in
		  current image. For this we use Lee Crocker's least sum of
		  differences squared approach, modified to spread the
		  values using gamma 1.7.  This mods was arrived at by
		  trial and error, just because it improves the mapping.
	       */
	    long ldist;
	    int r,g,b;
	    double gamma,gammadiv;
	    unsigned char convert[256];
	    unsigned char scale[64];

	    unsigned char far *table_ptr;
	    res = (res < 150) ? 90 : 180;   /* 90 or 180 dpi */
	    if (Printer_SetScreen == 0) {
		Printer_SFrequency = 21;  /* default red gamma */
		Printer_SAngle	   = 19;  /*	   green gamma */
		Printer_SStyle	   = 16;  /*	    blue gamma */
	    }
	    /* Convert the values in printera.asm.  We might do this just   */
	    /* once per run, but we'd need separate memory for that - can't */
	    /* just convert table in-place cause it could be in an overlay, */
	    /* might be paged out and then back in in original form.  Also, */
	    /* user might change gammas with a .par file entry mid-run.     */
	    for (j = 0; j < 3; ++j) {
		switch (j) {
		    case 0: table_ptr = pj_reds;
			    i = Printer_SFrequency;
			    break;
		    case 1: table_ptr = pj_greens;
			    i = Printer_SAngle;
			    break;
		    case 2: table_ptr = pj_blues;
			    i = Printer_SStyle;
		    }
		gamma = 10.0 / i;
		gammadiv = pow(255,gamma) / 255;
		for (i = 0; i < 256; ++i) { /* build gamma conversion table */
		    if ((i & 15) == 15)
			thinking(1,"Calculating color translation");
		    convert[i] = (int)((pow((double)i,gamma) / gammadiv) + 0.5);
		    }
		for (i = 0; i < 330; ++i) {
		    k = convert[table_ptr[i]];
		    if (k > 252) k = 252;
		    dstack[0][j][i] = (k + 2) >> 2;
		}
	    }
	    /* build comparison lookup table */
	    gamma = 1.7;
	    gammadiv = pow(63,gamma) / 63;
	    for (i = 0; i < 64; ++i) {
	       if ((j = (int)((pow((double)i,gamma) / gammadiv) * 4 + 0.5)) < i)
		  j = i;
	       scale[i] = j;
	    }
	    for (i = 0; i < 3; ++i) /* convert values via lookup */
		for (j = 0; j < 330; ++j)
		    dstack[1][i][j] = scale[dstack[0][i][j]];
	    /* Following code and the later code which writes to Paintjet    */
	    /* using pj_patterns was adapted from Lee Crocker's PGIF program */
	    for (i = 0; i < colors; ++i) { /* find nearest match colors */
		r = scale[dacbox[i][0]];
		g = scale[dacbox[i][1]];
		b = scale[dacbox[i][2]];
		ldist = 9999999;
		/* check variance vs each PaintJet color */
		/* if high-res 8 color mode, consider only 1st 8 colors */
		j = (res == 90) ? 330 : 8;
		while (--j >= 0) {
		    long dist;
		    dist  = (unsigned)(r-dstack[1][0][j]) * (r-dstack[1][0][j]);
		    dist += (unsigned)(g-dstack[1][1][j]) * (g-dstack[1][1][j]);
		    dist += (unsigned)(b-dstack[1][2][j]) * (b-dstack[1][2][j]);
		    if (dist < ldist) {
			ldist = dist;
			k = j;
		    }
		}
		pj_color_ptr[i] = k; /* remember best fit */
	    }
	    thinking(0,NULL);
	/*  if (debugflag == 900 || debugflag == 902) {
		color_test();
		EXIT_OVLY;
		return;
	    }  */
	    if (dotmode != 11) { /* preview */
		memcpy(dstack[1],dacbox,768);
		for (i = 0; i < colors; ++i)
		    for (j = 0; j < 3; ++j)
			dacbox[i][j] = dstack[0][j][pj_color_ptr[i]];
		spindac(0,1);
		texttempmsg("Preview. Enter=go, Esc=cancel, k=keep");
		i = getakeynohelp();
		if (i == 'K' || i == 'k') {
		    EXIT_OVLY;
		    return;
		}
		memcpy(dacbox,dstack[1],768);
		spindac(0,1);
		if (i == 0x1B) {
		    EXIT_OVLY;
		    return;
		}
	    }
	    break;
	    }

	case 5:
	case 6: /***** PostScript *****/
	    if ( res < 10 && res != 0 ) res = 10; /* PostScript scales... */
	    if ( res > 600 ) res = 600; /* it can handle any range! */
	    if ((Printer_SStyle < 0) || (Printer_SStyle >= TONES))
		Printer_SStyle = 0;
	    break;
    }

    /*****  Set up buffer size for immediate user gratification *****/
    /*****    AKA, if we don't have to, don't buffer the data   *****/
    BuffSiz=8;
    if (xdots>1024) BuffSiz=192;

    /*****   Initialize printer  *****/
    if (Print_To_File < 1) {
	printer_reset();
	/* wait a bit, some printers need time after reset */
	delay((ptrid == 4) ? 2000 : 500);
    }

    /******  INITIALIZE GRAPHICS MODES	******/
    switch (ptrid) {

	case 1:
	    print_title(ptrid,res,EndOfLine);
	    Printer_printf("\x1B*t%iR\x1B*r0A",res);/* HP           */
	    break;

	case 2:
	case 3:
	    print_title(ptrid,res,EndOfLine);
	    Printer_printf("\x1B\x33\x18");/* IBM                   */
	    break;

	case 4: /****** PaintJet *****/
	    print_title(ptrid,res,EndOfLine);
	    pj_width = ydots;
	    if (res == 90) pj_width <<= 1;
	    Printer_printf("\x1B*r0B\x1B*t180R\x1B*r3U\x1B*r%dS\x1B*b0M\x1B*r0A",
		pj_width);
	    pj_width >>= 3;
	    break;

	case 5:   /***** PostScript *****/
	case 6:   /***** PostScript Landscape *****/
	    if (!((EPSFileType > 0) && (ptrid==5)))
		Printer_printf("%%!PS-Adobe%s",EndOfLine);
	    if ((EPSFileType > 0) &&	 /* Only needed if saving to .EPS */
		(ptrid == 5))
		{
		Printer_printf("%%!PS-Adobe-1.0 EPSF-2.0%s",EndOfLine);

		if (EPSFileType==1)
		    i=xdots+78;
		else
		    i=(int)((double)xdots * (72.0 / (double)res))+78;

		if (Printer_Titleblock==0)
		    {
		    if (EPSFileType==1) { j = ydots + 78; }
		    else { j = (int)(((double)ydots * (72.0 / (double)res) / (double)finalaspectratio)+78); }
		    }
		else
		    {
		    if (EPSFileType==1) { j = ydots + 123; }
		    else { j = (int)(((double)ydots * (72.0 / (double)res))+123); }
		    }
		Printer_printf("%%%%TemplateBox: 12 12 %d %d%s",i,j,EndOfLine);
		Printer_printf("%%%%BoundingBox: 12 12 %d %d%s",i,j,EndOfLine);
		Printer_printf("%%%%PrinterRect: 12 12 %d %d%s",i,j,EndOfLine);
		Printer_printf("%%%%Creator: Fractint PostScript%s",EndOfLine);
		Printer_printf("%%%%Title: A %s fractal - %s - Fractint EPSF Type %d%s",
				       curfractalspecific->name[0]=='*' ?
				       &curfractalspecific->name[1] :
				       curfractalspecific->name,
				       PrintName,
				       EPSFileType,
				       EndOfLine);
		if (Printer_Titleblock==1)
		    Printer_printf("%%%%DocumentFonts: Helvetica%s",EndOfLine);
		Printer_printf("%%%%EndComments%s",EndOfLine);
		Printer_printf("/EPSFsave save def%s",EndOfLine);
		Printer_printf("0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin%s",EndOfLine);
		Printer_printf("10 setmiterlimit [] 0 setdash newpath%s",EndOfLine);
		}

	    /* Common code for all PostScript */
	    Printer_printf("/dopic { gsave %d %d 8 [%d 0 0 %d 0 %d]%s",
				     xdots, ydots, xdots, -ydots, ydots,
				     EndOfLine);
	    if (ColorPS)
	    Printer_printf("{ currentfile %d string readhexstring pop } false 3 colorimage grestore } def%s", xdots*3, EndOfLine);
	    else
	    Printer_printf("{ currentfile %d string readhexstring pop } image grestore } def%s", xdots, EndOfLine);

	    if (Printer_Titleblock==1)
		{
		Printer_printf("/Helvetica findfont 12 scalefont setfont%s",EndOfLine);
		if (ptrid==5) Printer_printf("30 60 moveto ");
		else	      Printer_printf("552 30 moveto 90 rotate ");
		print_title(ptrid,res,EndOfLine);
		}

	    if (EPSFileType != 1) /* Do not use on a WELL BEHAVED .EPS */
	      {
	      if ((ptrid == 5)&&(EPSFileType==2)&&
		  ((Printer_ColorXlat!=0)||(Printer_SetScreen!=0)))
			Printer_printf("%%%%BeginFeature%s",EndOfLine);
	      if (ColorPS)
		{
		if (Printer_ColorXlat==1)
		    Printer_printf("{1 exch sub} dup dup dup setcolortransfer%s",EndOfLine);
		if (Printer_ColorXlat>1)
		    Printer_printf("{%d mul round %d div} dup dup dup setcolortransfer%s",
				       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);
		if (Printer_ColorXlat<-1)
		    Printer_printf("{%d mul round %d div 1 exch sub} dup dup dup setcolortransfer",
				       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);

		if (Printer_SetScreen==1)
		    {
		    Printer_printf("%d %d {%s}%s",
				       Printer_RFrequency,
				       Printer_RAngle,
				       HalfTone[Printer_RStyle],
				       EndOfLine);
		    Printer_printf("%d %d {%s}%s",
				       Printer_GFrequency,
				       Printer_GAngle,
				       HalfTone[Printer_GStyle],
				       EndOfLine);
		    Printer_printf("%d %d {%s}%s",
				       Printer_BFrequency,
				       Printer_BAngle,
				       HalfTone[Printer_BStyle],
				       EndOfLine);
		    Printer_printf("%d %d {%s}%s",
				       Printer_SFrequency,
				       Printer_SAngle,
				       HalfTone[Printer_SStyle],
				       EndOfLine);
		    Printer_printf("setcolorscreen%s", EndOfLine);
		    }
		}
	      else
		{
		if (Printer_ColorXlat==1)
		    Printer_printf("{1 exch sub} settransfer%s",EndOfLine);
		if (Printer_ColorXlat>1)
		    Printer_printf("{%d mul round %d div} settransfer%s",
				       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);
		if (Printer_ColorXlat<-1)
		    Printer_printf("{%d mul round %d div 1 exch sub} settransfer",
				       Printer_ColorXlat,Printer_ColorXlat,EndOfLine);

		if (Printer_SetScreen==1)
		    Printer_printf("%d %d {%s} setscreen%s",
				       Printer_SFrequency,
				       Printer_SAngle,
				       HalfTone[Printer_SStyle],
				       EndOfLine);
		}

	      if (ptrid == 5)
		    {
		    if ((EPSFileType==2)&&((Printer_ColorXlat!=0)||(Printer_SetScreen!=0)))
			Printer_printf("%%%%EndFeature%s",EndOfLine);
		    if (res == 0)
			Printer_printf("30 191.5 translate 552 %4.1f",
					(552.0*(double)finalaspectratio));
		    else
			Printer_printf("30 %d translate %f %f",
				     75 - ((Printer_Titleblock==1) ? 0 : 45),
				     ((double)xdots*(72.0/(double)res)),
				     ((double)ydots*(72.0/(double)res)/(double)finalaspectratio));
		    }
		else			     /* For Horizontal PostScript */
		    if (res == 0)
			Printer_printf("582 30 translate 90 rotate 732 552");
		    else
			Printer_printf("%d 30 translate 90 rotate %f %f",
				     537 + ((Printer_Titleblock==1) ? 0 : 45),
				     ((double)xdots*(72.0/(double)res)),
				     ((double)ydots*(72.0/(double)res)/(double)finalaspectratio));
		Printer_printf(" scale%s",EndOfLine);
	      }

	    else if (ptrid == 5)       /* To be used on WELL-BEHAVED .EPS */
		Printer_printf("30 %d translate %d %d scale%s",
				    75 - ((Printer_Titleblock==1) ? 0 : 45),
				    xdots,ydots,EndOfLine);

	    Printer_printf("dopic%s",EndOfLine);
	    break;

	case 7: /* HP Plotter */
	    if (res<1) res=1;
	    if (res>10) res=10;
	    ci = (((double)xdots*((double)res-1.0))/2.0);
	    ck = (((double)ydots*((double)res-1.0))/2.0);
	    Printer_printf(";IN;SP0;SC%d,%d,%d,%d;%s\0",
		(int)(-ci),(int)((double)xdots+ci),
		(int)((double)ydots+ck),(int)(-ck),EndOfLine);
	    break;
	}

    if (keypressed()) { 	/* one last chance before we start...*/
	EXIT_OVLY;
	return;
	}

    memset(buff,0,192);

				/*****	Get And Print Screen **** */
    switch (ptrid) {

	case 1: 		       /* HP LaserJet (et al)		 */
	    imax=(ydots/8)-1;
	    for (x=0;((x<xdots)&&(!keypressed()));x+=BuffSiz) {
		for (i=imax;((i>=0)&&(!keypressed()));i--) {
		    for (y=7;((y>=0)&&(!keypressed()));y--) {
			for (j=0;j<BuffSiz;j++) {
			    if ((x+j)<xdots) {
				buff[j]<<=1;
				buff[j]+=(getcolor(x+j,i*8+y)&1);
				}
			    }
			}
		    for (j=0;j<BuffSiz;j++) {
			*(es+j+BuffSiz*i)=buff[j];
			buff[j]=0;
			}
		    }
		for (j=0;((j<BuffSiz)&&(!keypressed()));j++) {
		    if ((x+j)<xdots) {
			Printer_printf("\x1B*b%iW",imax+1);
			for (i=imax;((i>=0)&&(!keypressed()));i--) {
			    printer(*(es+j+BuffSiz*i));
			    }
			}
		    }
		}
	    if (!keypressed()) Printer_printf("\x1B*rB\x0C");
	    break;

	case 2: 		       /* IBM Graphics/Epson		 */
	    for (x=0;((x<xdots)&&(!keypressed()));x+=8) {
		switch (res) {
		    case 60:  Printer_printf("\x1BK"); break;
		    case 120: Printer_printf("\x1BL"); break;
		    case 240: Printer_printf("\x1BZ"); break;
			}
		high=ydots/256;
		low=ydots-(high*256);
		printer(low);
		printer(high);
		for (y=ydots-1;(y>=0);y--) {
		    buff[0]=0;
		    for (i=0;i<8;i++) {
			buff[0]<<=1;
			buff[0]+=(getcolor(x+i,y)&1);
			}
		    printer(buff[0]);
		    }
		if (keypressed()) break;
		Printer_printf(EndOfLine);
		}
	    if (!keypressed()) printer(12);
	    break;

	case 3: 		       /* IBM Graphics/Epson Color	*/
	    high=ydots/256;
	    low=ydots%256;
	    for (x=0;((x<xdots)&&(!keypressed()));x+=8)
		{
		for (k=0; k<8; k++)  /* colors */
		    {
		    Printer_printf("\x1Br%d",k); /* set printer color */
		    switch (res)
			{
			case 60:  Printer_printf("\x1BK"); break;
			case 120: Printer_printf("\x1BL"); break;
			case 240: Printer_printf("\x1BZ"); break;
			}
		    printer(low);
		    printer(high);
		    for (y=ydots-1;y>=0;y--)
			{
			buff[0]=0;
			for (i=0;i<8;i++)
			    {
			    buff[0]<<=1;
			    if ((getcolor(x+i,y)%8)==k)
				buff[0]++;
			    }
			printer(buff[0]);
			}
		    if (Printer_CRLF<2) printer(13);
		    }
		if ((Printer_CRLF==0) || (Printer_CRLF==2)) printer(10);
		}
	    printer(12);
	    printer(12);
	    printer_reset();
	    break;

	case 4: 		      /* HP PaintJet	   */
	    {
	    unsigned int fetchrows,fetched;
	    unsigned char far *pixels, far *nextpixel;
	    /* for reasonable speed when using disk video, try to fetch
	       and store the info for 8 columns at a time instead of
	       doing getcolor calls down each column in separate passes */
	    fetchrows = 16;
	    while (1) {
		if ((pixels = farmemalloc((long)(fetchrows)*ydots))) break;
		if ((fetchrows >>= 1) == 0) {
		    static char far msg[]={"insufficient memory"};
		    stopmsg(0,msg);
		    break;
		}
	    }
	    if (!pixels) break;
	    fetched = 0;
	    for (x = 0; (x < xdots && !keypressed()); ++x) {
		if (fetched == 0) {
		    if ((fetched = xdots-x) > fetchrows)
			fetched = fetchrows;
		    for (y = ydots-1; y >= 0; --y) {
			if (debugflag == 602) /* flip image */
			    nextpixel = pixels + y;
			else		      /* reverse order for unflipped */
			    nextpixel = pixels + ydots-1 - y;
			for (i = 0; i < fetched; ++i) {
			    *nextpixel = getcolor(x+i,y);
			    nextpixel += ydots;
			}
		    }
		    nextpixel = pixels;
		}
		--fetched;
		if (res == 180) { /* high-res 8 color mode */
		    int offset;
		    unsigned char bitmask;
		    offset = -1;
		    bitmask = 0;
		    for (y = ydots - 1; y >= 0; --y) {
			unsigned char color;
			if ((bitmask >>= 1) == 0) {
			    ++offset;
			    dstack[0][0][offset] = dstack[0][1][offset]
						 = dstack[0][2][offset] = 0;
			    bitmask = 0x80;
			}
			/* translate 01234567 to 70123456 */
			color = pj_color_ptr[*(nextpixel++)] - 1;
			if ((color & 1)) dstack[0][0][offset] += bitmask;
			if ((color & 2)) dstack[0][1][offset] += bitmask;
			if ((color & 4)) dstack[0][2][offset] += bitmask;
		    }
		}
		else { /* 90 dpi, build 2 lines, 2 dots per pixel */
		    int bitct,offset;
		    bitct = offset = 0;
		    for (y = ydots - 1; y >= 0; --y) {
			unsigned int color;
			color = pj_patterns[pj_color_ptr[*(nextpixel++)]];
			for (i = 0; i < 3; ++i) {
			    unsigned char *bufptr;
			    bufptr = &dstack[0][i][offset];
			    *bufptr <<= 2;
			    if ((color & 0x1000)) *bufptr += 2;
			    if ((color & 0x0100)) ++*bufptr;
			    bufptr = &dstack[1][i][offset];
			    *bufptr <<= 2;
			    if ((color & 0x0010)) *bufptr += 2;
			    if ((color & 0x0001)) ++*bufptr;
			    color >>= 1;
			}
			if (++bitct == 4) {
			    bitct = 0;
			    ++offset;
			}
		    }
		}
		for (i = 0; i < ((res == 90) ? 2 : 1); ++i) {
		    for (j = 0; j < 3; ++j) {
			unsigned char *bufptr,*bufend;
			Printer_printf((j < 2) ? "\x1B*b%dV" : "\x1B*b%dW",
				       pj_width);
			bufend = pj_width + (bufptr = dstack[i][j]);
			do {
			    while (printer(*bufptr)) { }
			} while (++bufptr < bufend);
		    }
		}
	    }
	    Printer_printf("\x1B*r0B"); /* end raster graphics */
	    if (!keypressed())
	       if (debugflag != 600)
		  printer(12); /* form feed */
	       else
		  Printer_printf("\n\n");
	    farmemfree(pixels);
	    break;
	    }

	case 5:
	case 6: 	/***** PostScript Portrait & Landscape *****/
	    {
	    char convert[513];
	    if (!ColorPS)
	      for (i=0; i<256; ++i)
		sprintf(&convert[2*i], "%02X",
				  (int)((1.20 * (double)dacbox[i][0])+
					(2.36 * (double)dacbox[i][1])+
					(0.44 * (double)dacbox[i][2])));
	    i=0;
	    j=0;
	    for (y=0;((y<ydots)&&(!keypressed()));y++)
	    {
		for (x=0;x<xdots;x++)
		{
		    k=getcolor(x,y);
		    if (ColorPS)
		      {
		      sprintf(&buff[i], "%02X%02X%02X", dacbox[k][0]<<2,
							dacbox[k][1]<<2,
							dacbox[k][2]<<2);
		      i+=6;
		      }
		    else
		      {
		      k*=2;
		      buff[i++]=convert[k];
		      buff[i++]=convert[k+1];
		      }
		    if (i>=64)
		    {
			strcpy(&buff[i],"  ");
			Printer_printf("%s%s",buff,EndOfLine);
			i=0;
			j++;
			if (j>9)
			{
			    j=0;
			    Printer_printf(EndOfLine);
			}
		    }
		}
	    }
	    if ( (EPSFileType > 0) && (EPSFileType <3) )
		Printer_printf("%s%%%%Trailer%sEPSFsave restore%s",EndOfLine,EndOfLine,EndOfLine);
	    else
		Printer_printf("%sshowpage%s%c",EndOfLine,EndOfLine,4);
	    break;
	    }

	case 7: /* HP Plotter */
	    {
	    double parm1,parm2;
	    for (i=0;i<3;i++)
	    {
	      Printer_printf("%sSP %d;%s\0",EndOfLine,(i+1),EndOfLine);
	      for (y=0;(y<ydots)&&(!keypressed());y++)
	      {
		for (x=0;x<xdots;x++)
		{
		  j=dacbox[getcolor(x,y)][i];
		  if (j>0)
		  {
		    switch(Printer_SStyle)
		    {
		      case 0:
			ci=0.004582144*(double)j;
			ck=-.007936057*(double)j;
			parm1 = (double)x+.5+ci+(((double)i-1.0)/3);
			parm2 = (double)y+.5+ck;
			break;
		      case 1:
			ci=-.004582144*(double)j+(((double)i+1.0)/8.0);
			ck=-.007936057*(double)j;
			parm1 = (double)x+.5+ci;
			parm2 = (double)y+.5+ck;
			break;
		      case 2:
			ci=-.0078125*(double)j+(((double)i+1.0)*.003906250);
			ck=-.0078125*(double)j;
			parm1 = (double)x+.5+ci;
			parm2 = (double)y+.5+ck;
			break;
		    }
		    Printer_printf("PA %f,%f;PD;PR %f,%f;PU;\0",
			parm1,parm2, ci*((double)-2), ck*((double)-2));
		  }
		}
	      }
	    }
	    Printer_printf("%s;SC;PA 0,0;SP0;%s\0",EndOfLine,EndOfLine);
	    Printer_printf(";;SP 0;%s\0",EndOfLine);
	    break;
	    }
    }

    if (Print_To_File > 0) fclose(PRFILE);
    if ((LPTn==30)||(LPTn==31))
	{
	for (x=0;x<2000;x++);
	outp((LPTn==30) ? 0x3FC : 0x2FC,0x00);
	outp((LPTn==30) ? 0x3F9 : 0x2F9,0x00);
	}
    EXIT_OVLY;
}

static void _fastcall print_title(int ptrid,int res,char *EndOfLine)
{
    char buff[80];
    int postscript;
    if (Printer_Titleblock == 0)
	return;
    postscript = (ptrid == 5 || ptrid ==6);
    if (!postscript)
	Printer_printf(EndOfLine);
    else
	Printer_printf("(");
    Printer_printf((curfractalspecific->name[0]=='*')
		     ? &curfractalspecific->name[1]
		     : curfractalspecific->name);
    if (fractype == FORMULA || fractype == FFORMULA)
	Printer_printf(" %s",FormName);
    if (fractype == LSYSTEM)
	Printer_printf(" %s",LName);
    if (fractype == IFS || fractype == IFS3D)
	Printer_printf(" %s",IFSName);
    Printer_printf(" - %dx%d - %d DPI", xdots, ydots, res);
    if (!postscript)
	Printer_printf(EndOfLine);
    else {
	Printer_printf(") show%s",EndOfLine);
	if (ptrid==5) Printer_printf("30 45 moveto (");
	else	      Printer_printf("-90 rotate 567 30 moveto 90 rotate (");
	}
    Printer_printf("Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f",
		   xxmin,yymax,xxmax,yymin);
    if (xx3rd != xxmin || yy3rd != yymin) {
	if (!postscript)
	    Printer_printf("%s        ",EndOfLine);
	Printer_printf(" Bottom-Left=%4.4f/%4.4f",xx3rd,yy3rd);
	}
    if (!postscript)
	Printer_printf(EndOfLine);
    else {
	Printer_printf(") show%s",EndOfLine);
	if (ptrid==5) Printer_printf("30 30 moveto (");
	else	      Printer_printf("-90 rotate 582 30 moveto 90 rotate (");
	}
    showtrig(buff);
    Printer_printf("Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s",
		   param[0],param[1],param[2],param[3],buff);
    if (!postscript)
	Printer_printf(EndOfLine);
    else
	Printer_printf(") show%s",EndOfLine);
}

/* This function prints a string to the the printer with BIOS calls. */

static void Printer_printf(char *fmt,...)
{
char s[500];
int x=0;
va_list arg;

va_start(arg,fmt);
vsprintf(s,fmt,arg);

if (Print_To_File>0)	/* This is for printing to file */
    fprintf(PRFILE,"%s",s);
else			/* And this is for printing to printer */
    while (s[x])
	if (printer(s[x++])!=0)
	    while (!keypressed()) { if (printer(s[x-1])==0) break; }
}

/* This function standardizes both _bios_printer and _bios_serialcom
 * in one function.  It takes its arguments and rearranges them and calls
 * the appropriate bios call.  If it then returns result !=0, there is a
 * problem with the printer.
 */
static int _fastcall printer(int c)
{
    if (Print_To_File>0) return ((fprintf(PRFILE,"%c",c))<1);
    if (LPTn<9)  return (((_bios_printer(0,LPTn,c))+0x0010)&0x0010);
    if (LPTn<19) return ((_bios_serialcom(1,(LPTn-10),c))&0x9E00);
    if ((LPTn==20)||(LPTn==21))
	{
	int PS=0;
	while ((PS & 0xF8) != 0xD8)
	    { PS = inp((LPTn==20) ? 0x379 : 0x279);
	      if (keypressed()) return(1); }
	outp((LPTn==20) ? 0x37C : 0x27C,c);
	PS = inp((LPTn==20) ? 0x37A : 0x27A);
	outp((LPTn==20) ? 0x37A : 0x27A,(PS | 0x01));
	outp((LPTn==20) ? 0x37A : 0x27A,PS);
	return(0);
	}
    if ((LPTn==30)||(LPTn==31))
	{
	while (((inp((LPTn==30) ? 0x3FE : 0x2FE)&0x30)!=0x30) ||
	       ((inp((LPTn==30) ? 0x3FD : 0x2FD)&0x60)!=0x60))
	    { if (keypressed()) return (1); }
	outp((LPTn==30) ? 0x3F8 : 0x2F8,c);
	return(0);
	}

    /* MCP 7-7-91, If we made it down to here, we may as well error out. */
    return(-1);
}

#ifdef __BORLANDC__
#if(__BORLANDC__ > 2)
   #pragma warn +eff
#endif
#endif

static void printer_reset()
{
    if (Print_To_File < 1)
	if (LPTn<9)	  _bios_printer(1,LPTn,0);
	else if (LPTn<19) _bios_serialcom(3,(LPTn-10),0);
}


/** debug code for pj_ color table checkout
color_test()
{
   int x,y,color,i,j,xx,yy;
   int bw,cw,bh,ch;
   setvideomode(videoentry.videomodeax,
		videoentry.videomodebx,
		videoentry.videomodecx,
		videoentry.videomodedx);
   bw = xdots/25; cw = bw * 2 / 3;
   bh = ydots/10; ch = bh * 2 / 3;
   dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 60;
   if (debugflag == 902)
      dacbox[0][0] = dacbox[0][1] = dacbox[0][2] = 0;
   for (x = 0; x < 25; ++x)
      for (y = 0; y < 10; ++y) {
	 if (x < 11) i = (32 - x) * 10 + y;
	     else    i = (24 - x) * 10 + y;
	 color = x * 10 + y + 1;
	 dacbox[color][0] = dstack[0][0][i];
	 dacbox[color][1] = dstack[0][1][i];
	 dacbox[color][2] = dstack[0][2][i];
	 for (i = 0; i < cw; ++i) {
	    xx = x * bw + bw / 6 + i;
	    yy = y * bh + bh / 6;
	    for (j = 0; j < ch; ++j)
	       putcolor(xx,yy++,color);
	    }
	 }
   spindac(0,1);
   getakey();
}
**/

