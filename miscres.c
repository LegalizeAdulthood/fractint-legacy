/*
	Resident odds and ends that don't fit anywhere else.
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#ifdef __TURBOC__
#include <dir.h>
#endif
#include "fractint.h"
#include "fractype.h"

/* routines in this module	*/

extern	void restore_active_ovly(void );
extern	void findpath(char *filename,char *fullpathname);
extern	void notdiskmsg(void );
extern	int cvtcentermag(double *Xctr,double *Yctr,double *Magnification);
extern	void updatesavename(char *name);
extern	int check_key(void );
extern	int timer(int timertype,int(*subrtn)(),...);
extern	void showtrig(char *buf);
extern	int set_trig_array(int k,char *name);
extern	void set_trig_pointers(int which);
extern	int tab_display(void );
extern	int endswithslash(char *fl);
extern	int ifsgetfile(void );
extern	int ifs3dgetfile(void );

static	void trigdetails(char *);

int active_ovly = -1;
long timer_start,timer_interval;	/* timer(...) start & total */

extern	char	ifsfilename[80];    /* IFS code file */
extern	char	ifs3dfilename[80];  /* IFS 3D code file */
extern char temp[], temp1[256];   /* temporary strings	      */

extern int  active_ovly;
extern int  xdots, ydots;
extern int  dotmode;
extern int  show_orbit;
extern int  timerflag;
extern int  debugflag;
extern int  maxit;
extern int  fractype;
extern int  got_status,curpass,totpasses,currow,curcol;
extern int  xxstart,xxstop,yystart,yystop;
extern int  display3d;

/* call next when returning from resident routine and unsure whether
   caller is an overlay which has been displaced */
void restore_active_ovly()
{
   switch (active_ovly) {
      case OVLY_MISCOVL:  miscovl_overlay();  break;
      case OVLY_CMDFILES: cmdfiles_overlay(); break;
      case OVLY_HELP:	  help_overlay();     break;
      case OVLY_PROMPTS:  prompts_overlay();  break;
      case OVLY_LOADFILE: loadfile_overlay(); break;
      case OVLY_ROTATE:   rotate_overlay();   break;
      case OVLY_PRINTER:  printer_overlay();  break;
      case OVLY_LINE3D:   line3d_overlay();   break;
      case OVLY_ENCODER:  encoder_overlay();  break;
      case OVLY_CALCFRAC: calcfrac_overlay(); break;
      }
}


void findpath(char *filename, char *fullpathname) /* return full pathnames */
{
   if (filename[0] == '\\'
     || (filename[0] && filename[1] == ':')) {
      strcpy(fullpathname,filename);
      return;
      }
   fullpathname[0] = 0; 			/* indicate none found */
#ifdef __TURBOC__				/* look for the file */
   strcpy(fullpathname,searchpath(filename));
#else
   _searchenv(filename,"PATH",fullpathname);
#endif
   if (fullpathname[0] != 0)			/* found it! */
      if (strncmp(&fullpathname[2],"\\\\",2) == 0) /* stupid klooge! */
	 strcpy(&fullpathname[3],filename);
}


void notdiskmsg()
{
static char far sorrymsg[]={"\
I'm sorry, but because of its random-screen-access algorithms, this\n\
type cannot be created using a real-disk based 'video' mode."};
   stopmsg(1,sorrymsg);
}


/* convert corners to center/mag */
int cvtcentermag(double *Xctr, double *Yctr, double *Magnification)
{
   extern double xxmax,xxmin,yymax,yymin,xx3rd,yy3rd;
   double Width, Height, Radius, Ratio;
   Width  = xxmax - xxmin;
   Height = yymax - yymin;
   Ratio = Height / Width;
   if(xx3rd != xxmin || yy3rd != yymin || Width < 0
     || (Width > 1e-8 && (Ratio <= 0.74 || Ratio >= 0.76))
     || Ratio < 0.66 || Ratio > 0.84)
      return(0);
   /* calculate center and magnification */
   Radius = Height / 2.0;
   *Xctr = xxmin + (Width / 2.0);
   *Yctr = yymin + Radius;
   *Magnification = 1.0 / Radius;
   return(1);
}


void updatesavename(char *filename) /* go to the next file name */
{
   char *save, *hold;
   char name[80],suffix[80];
   char *dotptr;

   strcpy(name,filename);
   suffix[0] = 0;
   if ((dotptr = strrchr(name,'.')) != NULL
     && dotptr > strrchr(name,'\\')) {
      strcpy(suffix,dotptr);
      *dotptr = 0;
      }

   hold = name + strlen(name) - 1; /* start at the end */
   while(hold >= name && (*hold == ' ' || isdigit(*hold))) /* skip backwards */
      hold--;
   hold++;			/* recover first digit */
   while (*hold == '0')         /* skip leading zeros */
      hold++;
   save = hold;
   while (*save) {		/* check for all nines */
      if (*save != '9')
	 break;
      save++;
      }
   if (!*save)			/* if the whole thing is nines then back */
      save = hold - 1;		/* up one place. Note that this will eat */
				/* your last letter if you go to far.	 */
   else
      save = hold;
   itoa(atoi(hold) + 1, save, 10); /* increment the number */
   strcpy(filename,name);
   strcat(filename,suffix);
}


int check_key()
{
   int key;
   if((key = keypressed()) != 0) {
      if(key != 'o' && key != 'O')
	 return(-1);
      getakey();
      if (dotmode != 11)
	 show_orbit = 1 - show_orbit;
   }
   return(0);
}


/* timer function:
     timer(0,(*fractal)())		fractal engine
     timer(1,NULL,int width)		decoder
     timer(2)				encoder
  */
int timer(int timertype,int(*subrtn)(),...)
{
   va_list arg_marker;	/* variable arg list */
   char *timestring;
   time_t ltime;
   FILE *fp;
   int out;
   int i;
   int do_bench;

   va_start(arg_marker,subrtn);
   do_bench = timerflag; /* record time? */
   if (timertype == 2)	 /* encoder, record time only if debug=200 */
      do_bench = (debugflag == 200);
   if(do_bench)
      fp=fopen("bench","a");
   timer_start = clock();
   switch(timertype) {
      case 0:
	 out = (*subrtn)();
	 break;
      case 1:
	 i = va_arg(arg_marker,int);
	 out = decoder(i);	     /* not indirect, safer with overlays */
	 break;
      case 2:
	 out = encoder();	     /* not indirect, safer with overlays */
	 break;
      }
   /* next assumes CLK_TCK is 10^n, n>=2 */
   timer_interval = (clock() - timer_start) / (CLK_TCK/100);

   if(do_bench) {
      time(&ltime);
      timestring = ctime(&ltime);
      timestring[24] = 0; /*clobber newline in time string */
      switch(timertype) {
	 case 1:
	    fprintf(fp,"decode ");
	    break;
	 case 2:
	    fprintf(fp,"encode ");
	    break;
	 }
      fprintf(fp,"%s type=%s resolution = %dx%d maxiter=%d",
	  timestring,
	  fractalspecific[fractype].name,
	  xdots,
	  ydots,
	  maxit);
      fprintf(fp," time= %ld.%02ld secs\n",timer_interval/100,timer_interval%100);
      if(fp != NULL)
	 fclose(fp);
      }
   return(out);
}


extern void lStkSin(void), dStkSin(void), lStkCos(void), dStkCos(void);
extern void lStkSinh(void),dStkSinh(void),lStkCosh(void),dStkCosh(void);
extern void lStkExp(void), dStkExp(void), lStkLog(void), dStkLog(void);
extern void lStkSqr(void), dStkSqr(void);

unsigned char trigndx[] = {SIN,SQR,SINH,COSH};
void (*ltrig0)() = lStkSin;
void (*ltrig1)() = lStkSqr;
void (*ltrig2)() = lStkSinh;
void (*ltrig3)() = lStkCosh;
void (*dtrig0)() = dStkSin;
void (*dtrig1)() = dStkSqr;
void (*dtrig2)() = dStkSinh;
void (*dtrig3)() = dStkCosh;

struct trig_funct_lst trigfn[] =
/* changing the order of these alters meaning of *.fra file */
{
   {"sin", lStkSin, dStkSin },
   {"cos", lStkCos, dStkCos },
   {"sinh",lStkSinh,dStkSinh},
   {"cosh",lStkCosh,dStkCosh},
   {"exp", lStkExp, dStkExp },
   {"log", lStkLog, dStkLog },
   {"sqr", lStkSqr, dStkSqr }
};

void showtrig(char *buf) /* return display form of active trig functions */
{
   char tmpbuf[30];
   *buf = 0; /* null string if none */
   trigdetails(tmpbuf);
   if (tmpbuf[0])
      sprintf(buf," function=%s",tmpbuf);
}

static void trigdetails(char *buf)
{
   int i, numfn;
   char tmpbuf[20];
   *buf = 0; /* null string if none */
   if ((numfn = (fractalspecific[fractype].flags >> 6) & 7)) {
      strcpy(buf,trigfn[trigndx[0]].name);
      i = 0;
      while(++i < numfn) {
	 sprintf(tmpbuf,"/%s",trigfn[trigndx[i]].name);
	 strcat(buf,tmpbuf);
	 }
      }
}

/* set array of trig function indices according to "function=" command */
int set_trig_array(int k, char *name)
{
   char trigname[6];
   int i, lstlen;
   char *slash;
   strncpy(trigname,name,5);
   trigname[5] = 0; /* safety first */

   if ((slash = strchr(trigname,'/')))
      *slash = 0;

   strlwr(trigname);
   lstlen = sizeof(trigfn)/sizeof(struct trig_funct_lst);

   for(i=0;i<lstlen;i++)
   {
      if(strcmp(trigname,trigfn[i].name)==0)
      {
	 trigndx[k] = i;
	 set_trig_pointers(k);
	 break;
      }
   }
   return(0);
}
void set_trig_pointers(int which)
{
  /* set trig variable functions to avoid array lookup time */
   int i;
   switch(which)
   {
   case 0:
      ltrig0 = trigfn[trigndx[0]].lfunct;
      dtrig0 = trigfn[trigndx[0]].dfunct;
      break;
   case 1:
      ltrig1 = trigfn[trigndx[1]].lfunct;
      dtrig1 = trigfn[trigndx[1]].dfunct;
      break;
   case 2:
      ltrig2 = trigfn[trigndx[2]].lfunct;
      dtrig2 = trigfn[trigndx[2]].dfunct;
      break;
   case 3:
      ltrig3 = trigfn[trigndx[3]].lfunct;
      dtrig3 = trigfn[trigndx[3]].dfunct;
      break;
   default: /* do 'em all */
      for(i=0;i<4;i++)
	 set_trig_pointers(i);
      break;
   }
}


int tab_display()	/* display the status of the current image */
{
   extern char usr_floatflag;
   extern double xxmin, xxmax, xx3rd, yymin, yymax, yy3rd;
   extern double param[4];
   extern double rqlim;
   extern long calctime, timer_start;
   extern int  calc_status;
   extern char FormName[];
   extern int  rseed;
   extern char LName[];
   extern int  invert;
   int row, i;
   double Xctr, Yctr, Magnification;
   char msg[81];
   char *msgptr;

   if (calc_status < 0) 	/* no active fractal image */
      return(0);		/* (no TAB on the credits screen) */
   if (calc_status == 1)	/* next assumes CLK_TCK is 10^n, n>=2 */
      calctime += (clock() - timer_start) / (CLK_TCK/100);
   stackscreen();
   helptitle();
   setattr(1,0,C_GENERAL_MED,24*80); /* init rest to background */

   row = 3;
   putstring(row,2,C_GENERAL_MED,"Fractal type:");
   if (display3d > 0)
      putstring(row,16,C_GENERAL_HI,"3D Transform");
   else {
      putstring(row,16,C_GENERAL_HI,
	   fractalspecific[fractype].name[0] == '*' ?
	     &fractalspecific[fractype].name[1] :
	     fractalspecific[fractype].name);
      trigdetails(msg);
      putstring(row+1,16,C_GENERAL_HI,msg);
      if (fractype == FORMULA || fractype == FFORMULA)
	 putstring(row+1,16,C_GENERAL_HI,FormName);
      if (fractype == LSYSTEM)
	 putstring(row+1,16,C_GENERAL_HI,LName);
      }
   switch (calc_status) {
      case 0:  msgptr = "Parms chgd since generated";
	       break;
      case 1:  msgptr = "Still being generated";
	       break;
      case 2:  msgptr = "Interrupted, resumable";
	       break;
      case 3:  msgptr = "Interrupted, non-resumable";
	       break;
      case 4:  msgptr = "Image completed";
	       break;
      default: msgptr = "";
      }
   putstring(row,45,C_GENERAL_HI,msgptr);
   if (helpmode == HELPCYCLING)
      putstring(row+1,45,C_GENERAL_HI,"You are in color-cycling mode");
   row += 2;

   i = 0;
   if (usr_floatflag) {
      putstring(row,45,C_GENERAL_HI,"Floating-point flag is activated");
      i = 1;
      }
   if (calc_status == 1 || calc_status == 2)
      if (fractalspecific[fractype].flags&INFCALC) {
	 putstring(row,2,C_GENERAL_HI,"Note: this type runs forever.");
	 i = 1;
	 }
   row += i;

   if (calc_status == 1 || calc_status == 2)
      if (fractalspecific[fractype].flags&NORESUME)
	 putstring(row++,2,C_GENERAL_HI,"Note: can't resume this type after interrupts other than <tab> and <F1>");
   ++row;

   if (got_status >= 0 && (calc_status == 1 || calc_status == 2)) {
      if (got_status == 1)
	 putstring(row,2,C_GENERAL_HI,"Solid Guessing");
      else if (got_status == 2)
	 putstring(row,2,C_GENERAL_HI,"Boundary Tracing");
      else {
	 sprintf(msg,"%d Pass Mode",totpasses);
	 putstring(row,2,C_GENERAL_HI,msg);
	 }
      ++row;
      sprintf(msg,"Working on block (y,x) [%d,%d]...[%d,%d], ",
	     yystart,xxstart,yystop,xxstop);
      putstring(row,2,C_GENERAL_MED,msg);
      if (got_status == 2) { /* btm */
	 putstring(-1,-1,C_GENERAL_MED,"at ");
	 sprintf(msg,"[%d,%d]",currow,curcol);
	 putstring(-1,-1,C_GENERAL_HI,msg);
	 }
      else {
	 if (totpasses > 1) {
	    putstring(-1,-1,C_GENERAL_MED,"pass ");
	    sprintf(msg,"%d",curpass);
	    putstring(-1,-1,C_GENERAL_HI,msg);
	    putstring(-1,-1,C_GENERAL_MED," of ");
	    sprintf(msg,"%d",totpasses);
	    putstring(-1,-1,C_GENERAL_HI,msg);
	    putstring(-1,-1,C_GENERAL_MED,", ");
	    }
	 putstring(-1,-1,C_GENERAL_MED,"at row ");
	 sprintf(msg,"%d",currow);
	 putstring(-1,-1,C_GENERAL_HI,msg);
	 }
      ++row;
      }
   putstring(row,2,C_GENERAL_MED,"Calculation time:");
   sprintf(msg,"%3ld:%02ld:%02ld.%02ld", calctime/360000,
	  (calctime%360000)/6000, (calctime%6000)/100, calctime%100);
   putstring(-1,-1,C_GENERAL_HI,msg);
   row += 2;

   putstring(row,2,C_GENERAL_MED,"Corners:                X                     Y");
   putstring(++row,3,C_GENERAL_MED,"top-left");
   sprintf(msg,"%20.16f  %20.16f",xxmin,yymax);
   putstring(-1,17,C_GENERAL_HI,msg);
   putstring(++row,3,C_GENERAL_MED,"bottom-right");
   sprintf(msg,"%20.16f  %20.16f",xxmax,yymin);
   putstring(-1,17,C_GENERAL_HI,msg);
   adjust_corner(); /* make bottom left exact if very near exact */
   if (cvtcentermag(&Xctr, &Yctr, &Magnification)) {
      putstring(row+=2,2,C_GENERAL_MED,"Center: ");
      sprintf(msg,"%20.16f %20.16f",Xctr,Yctr);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,"  Mag: ");
      if (Magnification < 1e6)
	 sprintf(msg,"%20.14f",Magnification);
      else if (Magnification < 1e12)
	 sprintf(msg,"%20.8f",Magnification);
      else
	 sprintf(msg,"%20.2f",Magnification);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }
   else if (xxmin != xx3rd || yymin != yy3rd) {
      putstring(++row,3,C_GENERAL_MED,"bottom-left");
      sprintf(msg,"%20.16f  %20.16f",xx3rd,yy3rd);
      putstring(-1,17,C_GENERAL_HI,msg);
      }
   putstring(row+=2,2,C_GENERAL_MED,"Params,");
   for (i = 0; i < 4; i++) {
      sprintf(msg,"%3d: ",i+1);
      putstring(-1,-1,C_GENERAL_MED,msg);
      sprintf(msg,"%12.9f",param[i]);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   putstring(row+=2,2,C_GENERAL_MED,"Iteration maximum: ");
   sprintf(msg,"%d",maxit);
   putstring(-1,-1,C_GENERAL_HI,msg);
   putstring(-1,-1,C_GENERAL_MED,"     Effective bailout: ");
   sprintf(msg,"%f",rqlim);
   putstring(-1,-1,C_GENERAL_HI,msg);

   if (fractype == PLASMA) {
      putstring(++row,2,C_GENERAL_MED,"Current 'rseed': ");
      sprintf(msg,"%d",rseed);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   if(invert) {
      extern double f_radius,f_xcenter,f_ycenter;
      putstring(++row,2,C_GENERAL_MED,"Inversion radius: ");
      sprintf(msg,"%12.9f",f_radius);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,"  xcenter: ");
      sprintf(msg,"%12.9f",f_xcenter);
      putstring(-1,-1,C_GENERAL_HI,msg);
      putstring(-1,-1,C_GENERAL_MED,"  ycenter: ");
      sprintf(msg,"%12.9f",f_ycenter);
      putstring(-1,-1,C_GENERAL_HI,msg);
      }

   if ((row += 2) < 23) ++row;
   putstringcenter(row,0,80,C_GENERAL_LO,"...Press any key to continue...");
   movecursor(25,80);
   getakey();
   unstackscreen();
   timer_start = clock(); /* tab display was "time out" */
   return(0);
}


int endswithslash(char *fl)
{
   int len;
   len = strlen(fl);
   if(len)
      if(fl[--len]=='\\')
	 return(1);
   return(0);
}

/* --------------------------------------------------------------------- */

int ifsgetfile()       /* read in IFS parameters */
{
   FILE  *ifsfile;		/* IFS code file pointer */
   float localifs[IFSPARM];
   int i, j;

   /* read in IFS codes from file */
   if (!endswithslash(ifsfilename)) {
      if ( (ifsfile = fopen( ifsfilename,"r" )) != NULL ) {
	 i = -1;
	 while (fgets(temp1, 155, ifsfile) != NULL) {
	    if (++i >= NUMIFS) break;
	    sscanf(temp1," %f %f %f %f %f %f %f",
	       &localifs[0], &localifs[1], &localifs[2], &localifs[3],
	       &localifs[4], &localifs[5], &localifs[6]  );
	    if (i == 0 && localifs[6] == 0) {
	       char msg[200];
	       sprintf(msg,"%s invalid (perhaps it is IFS3D?)",ifsfilename);
	       stopmsg(0,msg);
	       fclose(ifsfile);
	       return(-1);
	       }
	    for (j = 0; j < IFSPARM; j++) {
	       initifs[i][j]   = localifs[j];
	       initifs[i+1][j] = 0.0;
	       }
	    }
	 fclose(ifsfile);
	 }
      }
   return(0);
}

/* --------------------------------------------------------------------- */

int ifs3dgetfile()     /* read in 3D IFS parameters */
{
   FILE  *ifsfile;		/* IFS code file pointer */
   float localifs[IFS3DPARM];
   int i, j;

   /* read in IFS codes from file */
   if (!endswithslash(ifs3dfilename)) {
      if ( (ifsfile = fopen( ifs3dfilename,"r" )) != NULL ) {
	 i = -1;
	 while (fgets(temp1, 155, ifsfile) != NULL) {
	    if (++i >= NUMIFS) break;
	    j = sscanf(temp1," %f %f %f %f %f %f %f %f %f %f %f %f %f",
	       &localifs[ 0], &localifs[ 1], &localifs[ 2],
	       &localifs[ 3], &localifs[ 4], &localifs[ 5],
	       &localifs[ 6], &localifs[ 7], &localifs[ 8],
	       &localifs[ 9], &localifs[10], &localifs[11],
	       &localifs[12]
	       );
	    if (i == 0 && j < 13) {
	       char msg[200];
	       sprintf(msg,"%s invalid (perhaps it is 2D IFS?)",ifs3dfilename);
	       stopmsg(0,msg);
	       fclose(ifsfile);
	       return(-1);
	       }
	    for (j = 0; j < IFS3DPARM; j++) {
	       initifs3d[i][j]	 = localifs[j];
	       initifs3d[i+1][j] = 0.0;
	       }
	    }
	 fclose(ifsfile);
	 }
      }
   return(0);
}


