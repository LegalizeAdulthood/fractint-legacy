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
 *	Printing calls are made directly to the BIOS for DOS can't handle fast
 *	transfer of data to the HP.  (Or maybe visa-versa, HP can't handle the
 *	slow transfer of data from DOS)
 *
 *	Supported Printers:	Tested Ok:
 *	 HP LaserJet
 *	    LJ+,LJII		 MDS
 *	 Toshiba PageLaser	 MDS (Set FRACTINT to use HP)
 *	 IBM Graphics		 MDS
 *	 EPSON
 *	    Models?		 Untested.
 *	 IBM LaserPrinter with PostScript   SWT
 *
 *	Future support to include OKI 20 (color) printer, and just about
 *	any printer you request.
 *
 *	Future modifications to include a more flexible, standard interface
 *	with the surrounding program, for easier portability to other
 *	programs.
 *
 * Compile with  MSC 5.1 or TurboC 2.0 (or QC?? Someone PLEASE Verify this)
 *
 * Tabs at 8.
 */

/*** HP PaintJet support added	16-Jun-1990  19:43:18 ***/

#include <bios.h>
#include <dos.h>
#include <io.h>
#include <fcntl.h>
#include <sys\types.h>
#include <errno.h>
#include <stdio.h>	/*** for vsprintf prototype ***/
#include <string.h>
#include <stdarg.h>
#include "fractint.h"
#include "fractype.h"

#define TONES 16      /* Number of PostScript halftone styles */

/********      PROTOTYPES     ********/

       void printer_overlay(void);
       void Print_Screen(void);
static int  Printer_printf(int LPTn,char *fmt,...);
static int  printer(int a,int b,int c);

extern int keypressed (void);
extern void updatesavename (char *);
extern void showtrig (char *);

/********  EXTRN GLOBAL VARS  ********/

extern int xdots,ydots, 	       /* size of screen		   */
	   extraseg,		       /* used for buffering		   */
	   fractype;		       /* used for title block		   */
extern unsigned char dacbox[256][3];   /* for PostScript printing	   */
extern char FormName[]; 	       /* for Title block info		   */
extern char LName[];		       /* for Title block info		   */
extern char PrintName[];	       /* Filename for print-to-file	   */
extern float finalaspectratio;
extern double xxmin,xxmax,xx3rd,
	      yymin,yymax,yy3rd,param[4]; /* for Title block info	   */

/********	GLOBALS       ********/

int Printer_Resolution,  /* 75,100,150,300 for HP; 60,120,240 for IBM;
				90 or 180 for the PaintJet; 10-600 for PS  */
    LPTNumber,		       /* ==1,2,3 LPTx; or 11,12,13,14 for COM1-4  */
    Printer_Type,		       /* ==1 HP,
					  ==2 IBM/EPSON,
					  ==3 Epson color,
					  ==4 HP PaintJet
					  ==5,6 PostScript		   */
    Printer_Titleblock,       /* Print info about the fractal?		   */
    Printer_ColorXlat,	      /* PostScript only - invert colors	   */
    Printer_SetScreen,	      /* PostScript only - reprogram halftone ?    */
    Printer_SFrequency,       /* PostScript only - Halftone Frequency	   */
    Printer_SAngle,	      /* PostScript only - Halftone angle	   */
    Printer_SStyle,	      /* PostScript only - Halftone style	   */
    Print_To_File,	      /* Print to file toggle			   */
    EPSFileType;	      /* EPSFileType - 1 = well-behaved,
					       2 = much less behaved,
					       3 = not well behaved	   */

FILE *PRFILE;
int TSTFILE;

char *HalfTone[TONES]=	{
			 "{dup mul exch dup mul add 1 exch sub}",
			 "{dup mul exch dup mul add}",
			 "{pop}",
			 "{exch pop}",
			 "{pop abs}",
			 "{exch pop abs}",
			 "{abs exch abs 2 copy sub add exch pop 1 exch sub}",
			 "{abs exch abs 2 copy sub add exch pop}",
			 "{dup mul exch pop 1 exch sub}",
			 "{dup mul exch pop}",
			 "{exch dup mul exch pop 1 exch sub}",
			 "{exch dup mul exch pop}",
			 "{dup mul exch dup mul mul 1 exch div}",
			 "{sqrt exch sqrt add}",
			 "{dup mul 2 mul exch dup mul 2 div mul 1 exch sub}",
			 "{dup mul 2 mul exch dup mul 2 div mul}"
			};

#ifdef __TURBOC__
    #define	    _bios_printer(a,b,c)    biosprint((a),(c),(b))
    #define	    _bios_serialcom(a,b,c)  bioscom((a),(c),(b))
#endif

void printer_overlay() { }	/* for restore_active_ovly */

void Print_Screen()
{
    register int y,j;		/* indices			  */
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
	LPTn,			/* printer number we're gonna use */
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

    ENTER_OVLY(OVLY_PRINTER);
				/********   SETUP VARIABLES  ********/
    for (j=0;j<192;j++) { buff[j]=0; }		/* Clear buffer  */

    if (Print_To_File>0)
      {
      while (filelength(TSTFILE = open(PrintName,O_RDONLY))>0)
	{
	close(TSTFILE);
	updatesavename((char *)PrintName);
	}
      close(TSTFILE);
      if ((PRFILE = fopen(PrintName,"wb"))==NULL) Print_To_File = 0;
      }

    if (extraseg) {
	#ifdef __TURBOC__
	    es=MK_FP(extraseg,0);
	#else
	    FP_SEG(es)=extraseg;
	    FP_OFF(es)=0;
	#endif
	}

    LPTn=LPTNumber-1;
    if (((LPTn<10)&&(LPTn>2))||
	(LPTn<0)||(LPTn>13)) LPTn=0;   /* default of LPT1 (==0) 	 */
    ptrid=Printer_Type;
    if ((ptrid<1)||(ptrid>6)) ptrid=2; /* default of IBM/EPSON		 */
    res=Printer_Resolution;
    switch (ptrid) {
	default:
	    break;
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
		if ( res < 150 ) res = 90;	/* an arbitrary boundary */
		if ( res >= 150 ) res = 180;
	    break;
	case 5:
	case 6: /***** PostScript *****/
		if ( res < 10 ) res = 10;   /* PostScript scales...	*/
		if ( res > 600 ) res = 600; /* it can handle any range! */
		if (Printer_SStyle < 0) Printer_SStyle = 0;
		if (Printer_SStyle > TONES) Printer_SStyle = TONES;
	    break;
	    }

	 /*****  Set up buffer size for immediate user gratification *****/
	 /*****    AKA, if we don't have to, don't buffer the data   *****/
    BuffSiz=8;
    if (xdots>1024) BuffSiz=192;     /*****   Initialize printer  *****/
		if (Print_To_File < 1) printer(1,LPTn,0);

    showtrig (buff);

				/******  INITIALIZE GRAPHICS MODES  ******/
    switch (ptrid) {
	default:
	    break;
	case 1:
	if (Printer_Titleblock==1)
		{
		Printer_printf(LPTn,"\r\n%s%s - %dx%d - %d DPI\r\n",
				     fractalspecific[fractype].name[0]=='*' ?
				     &fractalspecific[fractype].name[1] :
				     fractalspecific[fractype].name,
				     (fractype==FORMULA||fractype==FFORMULA) ?
				     FormName : " ",
				     xdots, ydots, res);
		Printer_printf(LPTn,"Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f\r\n         Bottom-Left=%4.4f/%4.4f\r\n",
				       xxmin,yymax,xxmax,yymin,xx3rd,yy3rd);
		Printer_printf(LPTn,"Parameters: %4.4f/%4.4f/%4.4f/%4.4f",
				       param[0],param[1],
				       param[2],param[3],
				       buff);
		}
	    Printer_printf(LPTn,"\x1B*t%iR\x1B*r0A",res);/* HP           */
	    break;
	case 2:
	case 3:
	if (Printer_Titleblock==1)
		{
		Printer_printf(LPTn,"\r\n%s%s - %dx%d - %d DPI\r\n",
				     fractalspecific[fractype].name[0]=='*' ?
				     &fractalspecific[fractype].name[1] :
				     fractalspecific[fractype].name,
				     (fractype==FORMULA||fractype==FFORMULA) ?
				     FormName : " ",
				     xdots, ydots, res);
		Printer_printf(LPTn,"Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f\r\n         Bottom-Left=%4.4f/%4.4f\r\n",
				       xxmin,yymax,xxmax,yymin,xx3rd,yy3rd);
		Printer_printf(LPTn,"Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s\r\n",
				       param[0],param[1],
				       param[2],param[3],
				       buff);
		}
	    Printer_printf(LPTn,"\x1B\x33\x18");/* IBM                   */
	    break;
	case 4: /****** PaintJet *****/
	/* set resolution, 4 colour planes for 90dpi or 3 for 180dpi,
		 start raster graphics	 */
	if (Printer_Titleblock==1)
		{
		Printer_printf(LPTn,"\r\n%s%s - %dx%d - %d DPI\r\n",
				     fractalspecific[fractype].name[0]=='*' ?
				     &fractalspecific[fractype].name[1] :
				     fractalspecific[fractype].name,
				     (fractype==FORMULA||fractype==FFORMULA) ?
				     FormName : " ",
				     xdots, ydots, res);
		Printer_printf(LPTn,"Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f\r\n         Bottom-Left=%4.4f/%4.4f\r\n",
				       xxmin,yymax,xxmax,yymin,xx3rd,yy3rd);
		Printer_printf(LPTn,"Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s\r\n",
				       param[0],param[1],
				       param[2],param[3],
				       buff);
		}
		Printer_printf(LPTn,"\x1B*t%dR\x1B*r%dU\x1B*r0A",
			res,res==90?4:3);
	    break;
	case 5: {  /***** PostScript *****/
if (EPSFileType > 0)			 /* Only needed if saving to .EPS */
		{
		Printer_printf(LPTn,"%%!PS-Adobe-2.0 EPSF-2.0\r\n");

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
		Printer_printf(LPTn,"%%%%TemplateBox: 12 12 %d %d\r\n",i,j);
		Printer_printf(LPTn,"%%%%BoundingBox: 12 12 %d %d\r\n",i,j);
		Printer_printf(LPTn,"%%%%PrinterRect: 12 12 %d %d\r\n",i,j);
		Printer_printf(LPTn,"%%%%Creator: Fractint PostScript\r\n");
		Printer_printf(LPTn,"%%%%Title: A %s fractal - %s - Fractint EPSF Type %d\r\n",
				       fractalspecific[fractype].name[0]=='*' ?
				       &fractalspecific[fractype].name[1] :
				       fractalspecific[fractype].name,
				       PrintName,
				       EPSFileType);
		if (Printer_Titleblock==1)
			{
			Printer_printf(LPTn,"%%%%DocumentFonts: Helvetica\r\n");
			}
		Printer_printf(LPTn,"%%%%EndComments\r\n");
		Printer_printf(LPTn,"0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin\r\n");
		Printer_printf(LPTn,"10 setmiterlimit [] 0 setdash newpath } bind def\r\n");
		}

		Printer_printf(LPTn,"/dopic { gsave %d %d 8 [%d 0 0 %d 0 %d]\r\n",
				     xdots, ydots, xdots, -ydots, ydots);
		Printer_printf(LPTn,"{ currentfile %d string readhexstring pop } image grestore } def\r\n", xdots);
		if (Printer_Titleblock==1)
		  {
		  Printer_printf(LPTn,"/Helvetica findfont 12 scalefont setfont\r\n");
		  Printer_printf(LPTn,"30 60 moveto (%s ",
				       fractalspecific[fractype].name[0]=='*' ?
				       &fractalspecific[fractype].name[1] :
				       fractalspecific[fractype].name);
		  if ((fractype==FORMULA)||(fractype==FFORMULA))
			Printer_printf(LPTn,"%s",FormName);
		  if (fractype==LSYSTEM)
			Printer_printf(LPTn,"%s",LName);
		  Printer_printf(LPTn," - %dx%d - %d DPI) show\r\n",
				       xdots, ydots, res);
		  Printer_printf(LPTn,"30 45 moveto (Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f Bottom-Left=%4.4f/%4.4f) show\r\n",
				       xxmin,yymax,xxmax,yymin,xx3rd,yy3rd);
		  Printer_printf(LPTn,"30 30 moveto (Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s) show\r\n",
				       param[0],param[1],
				       param[2],param[3],
				       buff);
		  }

if (EPSFileType != 1)	 /* Cannot use these commands on a WELL BEHAVED .EPS */
		{
		if ((EPSFileType==2)&&((Printer_ColorXlat!=0)||(Printer_SetScreen!=0)))
			{
			Printer_printf(LPTn,"%%%%BeginFeature\r\n");
			}
		if (Printer_ColorXlat==1)
		  Printer_printf(LPTn,"{1 exch sub} settransfer\r\n");
		if (Printer_ColorXlat>1)
		  Printer_printf(LPTn,"{%d mul round %d div} settransfer\r\n",
				       Printer_ColorXlat,Printer_ColorXlat);
		if (Printer_ColorXlat<-1)
		  Printer_printf(LPTn,"{%d mul round %d div 1 exch sub} settransfer\r\n",
				       Printer_ColorXlat,Printer_ColorXlat);

		if (Printer_SetScreen==1)
		  {
		  Printer_printf(LPTn,"%d %d %s setscreen\r\n",
				       Printer_SFrequency,
				       Printer_SAngle,
				       HalfTone[Printer_SStyle]);
		  }

		if ((EPSFileType==2)&&((Printer_ColorXlat!=0)||(Printer_SetScreen!=0)))
			{
			Printer_printf(LPTn,"%%%%EndFeature\r\n");
			}

		Printer_printf(LPTn,"30 %d translate %f %f scale\r\n",
				     75 - ((Printer_Titleblock==1) ? 0 : 45),
				     ((double)xdots*(72.0/(double)res)),
				     ((double)ydots*(72.0/(double)res)/(double)finalaspectratio));
		}

if(EPSFileType == 1)			 /* To be used on WELL-BEHAVED .EPS */
		{
		Printer_printf(LPTn,"30 %d translate %d %d scale\r\n",
				    75 - ((Printer_Titleblock==1) ? 0 : 45),
				    xdots,ydots);
		}

		Printer_printf(LPTn,"dopic\r\n");
		break;
		}
	case 6: {   /***** PostScript Landscape *****/
		Printer_printf(LPTn,"/dopic { gsave %d %d 8 [%d 0 0 %d 0 %d]\r\n",
				     xdots, ydots, xdots, -ydots, ydots);
		Printer_printf(LPTn,"{ currentfile %d string readhexstring pop } image grestore } def\r\n", xdots);
		if (Printer_Titleblock==1)
		  {
		  Printer_printf(LPTn,"/showtitle {\r\n");
		  Printer_printf(LPTn,"/Helvetica findfont 12 scalefont setfont\r\n");
		  Printer_printf(LPTn,"552 30 moveto 90 rotate (%s ",
				       fractalspecific[fractype].name[0] == '*' ?
				       &fractalspecific[fractype].name[1] :
				       fractalspecific[fractype].name);
		  if ((fractype==FORMULA)||(fractype==FFORMULA))
			Printer_printf(LPTn,"%s",FormName);
		  if (fractype==LSYSTEM)
			Printer_printf(LPTn,"%s",LName);
		  Printer_printf(LPTn," - %dx%d - %d DPI) show -90 rotate\r\n",
				       xdots, ydots, res);
		  Printer_printf(LPTn,"567 30 moveto 90 rotate (Corners: Top-Left=%4.4f/%4.4f Bottom-Right=%4.4f/%4.4f Bottom-Left=%4.4f/%4.4f) show -90 rotate\r\n",
				       xxmin,yymax,xxmax,yymin,xx3rd,yy3rd);
		  Printer_printf(LPTn,"582 30 moveto 90 rotate (Parameters: %4.4f/%4.4f/%4.4f/%4.4f %s) show -90 rotate\r\n",
				       param[0],param[1],
				       param[2],param[3],
				       buff);
		  Printer_printf(LPTn,"} def\r\n");
		  Printer_printf(LPTn,"showtitle\r\n");
		  }
		if (Printer_ColorXlat==1)
		  Printer_printf(LPTn,"{1 exch sub} settransfer\r\n");
		if (Printer_ColorXlat>1)
		  Printer_printf(LPTn,"{%d mul round %d div} settransfer\r\n",
				       Printer_ColorXlat,Printer_ColorXlat);
		if (Printer_ColorXlat<-1)
		  Printer_printf(LPTn,"{%d mul round %d div 1 exch sub} settransfer\r\n",
				       Printer_ColorXlat,Printer_ColorXlat);
		if (Printer_SetScreen==1)
		  {
		  Printer_printf(LPTn,"%d %d %s setscreen\r\n",
				       Printer_SFrequency,
				       Printer_SAngle,
				       HalfTone[Printer_SStyle]);
		  }

		Printer_printf(LPTn,"%d 30 translate 90 rotate %f %f scale\r\n",
				     537 + ((Printer_Titleblock==1) ? 0 : 45),
				     ((double)xdots*(72.0/(double)res)),
				     ((double)ydots*(72.0/(double)res)/(double)finalaspectratio));
		Printer_printf(LPTn,"dopic\r\n");
		break;
		}
	    }

    if (keypressed()) { 	/* one last chance before we start...*/
       EXIT_OVLY;
       return;
       }

    for (j=0;j<192;j++) { buff[j]=0; } /* Clear buffer		  */

				/*****	Get And Print Screen **** */
    switch (ptrid) {
	default:
	    break;
	case 1: {		       /* HP LaserJet (et al)		 */
		imax=(ydots/8)-1;
		for (x=0;((x<xdots)&&(!keypressed()));x+=BuffSiz) {
		    for (i=imax;((i>=0)&&(!keypressed()));i--) {
			for (y=7;((y>=0)&&(!keypressed()));y--) {
			    for (j=0;((j<BuffSiz)&&(!keypressed()));j++) {
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
			    Printer_printf(LPTn,"\x1B*b%iW"
			    ,imax+1);
			    for (i=imax;((i>=0)&&(!keypressed()));i--) {
				printer(0,LPTn,*(es+j+BuffSiz*i));
				}
			    }
			}
		    }
		if (!keypressed()) Printer_printf(LPTn,"\x1B*rB\x0C");
		break;
		}
	case 2: {		       /* IBM Graphics/Epson		 */
		for (x=0;((x<xdots)&&(!keypressed()));x+=8) {
		    switch (res) {
			case 60:  Printer_printf(LPTn,"\x1BK"); break;
			case 120: Printer_printf(LPTn,"\x1BL"); break;
			case 240: Printer_printf(LPTn,"\x1BZ"); break;
			    }
		    high=ydots/256;
		    low=ydots-(high*256);
		    printer(0,LPTn,low);
		    printer(0,LPTn,high);
		    for (y=ydots-1;(y>=0);y--) {
			buff[0]=0;
			for (i=0;i<8;i++) {
			    buff[0]<<=1;
			    buff[0]+=(getcolor(x+i,y)&1);
			    }
			printer(0,LPTn,buff[0]);
			}
		    if (keypressed()) break;
		    printer(0,LPTn,13);
		    printer(0,LPTn,10);
		    }
		if (!keypressed()) printer(0,LPTn,12);
		break;
		}
	case 3: {											/* IBM Graphics/Epson Color			*/
		high=ydots/256;
		low=ydots%256;
		for (x=0;((x<xdots)&&(!keypressed()));x+=8)
		    {
		    for (k=0; k<8; k++)  /* colors */
			{
			Printer_printf(LPTn,"\x1Br%d",k); /* set printer color */
			switch (res)
			    {
			    case 60:  Printer_printf(LPTn,"\x1BK"); break;
			    case 120: Printer_printf(LPTn,"\x1BL"); break;
			    case 240: Printer_printf(LPTn,"\x1BZ"); break;
			    }
			printer(0,LPTn,low);
			printer(0,LPTn,high);
			for (y=ydots-1;y>=0;y--)
			    {
			    buff[0]=0;
			    for (i=0;i<8;i++)
				{
				buff[0]<<=1;
				if ((getcolor(x+i,y)%8)==k)
				    buff[0]++;
				}
			    printer(0,LPTn,buff[0]);
			    }
			printer(0,LPTn,13);
			}
		    printer(0,LPTn,10);
		    }
		printer(0,LPTn,12);
		printer(0,LPTn,12);
		printer(1,LPTn,0);  /* reset */
		break;
		}
	case 4: 		      /* HP PaintJet	   */
	 {
	 for (y=0;((y<ydots)&&(!keypressed()));y++) {
		 for (k=0; k<(res==90?4:3); k++) { /* planes */
		   Printer_printf(LPTn,"\x1B*b%dV",xdots/8);
		   for (x=0;x<xdots;x+=8) {
		       buff[0]=0;
		       for (i=0;i<8;i++) {
			   buff[0]<<=1;
			 if ( getcolor(x+i,y)&(1<<k) )
			       buff[0]++;
		       }
		       printer(0,LPTn,buff[0]);
		   }
	       }
	       Printer_printf(LPTn,"\x1B*b0W"); /* end colour planes   */
	   }
	 Printer_printf(LPTn,"\x1B*r0B");  /* end raster graphics */
	   printer(0,LPTn,12);
	 printer(1,LPTn,0);  /* reset */
	 break;
	 }
	case 5:
	case 6: 	/***** PostScript Portrait & Landscape *****/
	 {
	 i=0;
	 j=0;
	 for (y=0;((y<ydots)&&(!keypressed()));y++)
	  {
	  for (x=0;x<xdots;x++)
	   {
	   k=getcolor(x,y);
	   sprintf(&buff[2*i], "%02X\0",
				  (int)((1.20 * (double)dacbox[k][0])+
					(2.36 * (double)dacbox[k][1])+
					(0.44 * (double)dacbox[k][2])));
	   i++;
	   if (i>31)
	    {
	    Printer_printf(LPTn,"%s\r\n",buff);
	    i=0;
	    j++;
	    if (j>9)
	     {
	     j=0;
	     Printer_printf(LPTn,"\r\n");
	     }
	    }
	   }
	  }
	 if ((EPSFileType == 0)||(EPSFileType > 2))
		{
		Printer_printf(LPTn,"\r\nshowpage\r\n%c",4);
		}
	 else
		{
		Printer_printf(LPTn,"\r\n%%%%Trailer\r\n%c",4);
		}
	 break;
	 }
    }
    if (Print_To_File > 0) fclose(PRFILE);
    EXIT_OVLY;
}

/* This function prints a string to the the printer with BIOS calls.
 * It returns the printer's status.
 */
static int Printer_printf(int LPTn,char *fmt,...)
{
char s[500];
int x=0;
va_list arg;
va_start(arg,fmt);
vsprintf(s,fmt,arg);

if (Print_To_File>0) return ((fprintf(PRFILE,"%s",s))<1);

while ((s[x])&&(!keypressed())) {
    if (printer(0,LPTn,s[x++])!=0) {
	while (!keypressed()) {
	    if (printer(0,LPTn,s[x-1])==0) break;
	    }
	}
    }
return (printer(2,LPTn,0));
}

/* This function standardizes both _bios_printer and _bios_serialcom
 * as well as print_to_file
 * in one function.  It takes its arguments and rearranges them and calls
 * the appropriate bios call.  If it then returns result !=0, there is a
 * problem with the printer.
 */
static int printer(int a,int b,int c)
{
    if (b<9) return (((_bios_printer(a,b,c))+0x0010)&0x0010);
    return ((_bios_serialcom(((a!=0)?3:1),(b-10),c))&0x9E00);
}
