
/*
	Command-line / Command-File Parser Routines
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef __TURBOC__
#include <dir.h>
#endif

#include "fractint.h"

/* variables defined by the command line/files processor */

char    readname[81];     	/* name of fractal input file */
int     showfile;        	/* has file been displayed yet? */
int	warn;			/* 0 if savename warnings off, 1 if on */
int	sound;			/* 0 if sound is off, 1 if on */
char	savename[80];		/* save files using this name */
char	potfile[80];		/* save potential using this name TW 7/20/89 */
int	debugflag;		/* internal use only - you didn't see this */
int	timerflag;		/* you didn't see this, either */
int	cyclelimit;		/* color-rotator upper limit */
int	inside;			/* inside color: 1=blue     */
int	display3d;		/* 3D display flag: 0 = OFF */
int	overlay3d;		/* 3D overlay flag: 0 = OFF */
int	init3d[20];		/* '3d=nn/nn/nn/...' values */
int	initbatch;		/* 1 if batch run (no kbd)  */
int	initmode;		/* initial video mode       */
int	inititer;		/* initial value of maxiter */
int	initincr;		/* initial maxiter incrmnt  */
int	initpass;		/* initial pass mode        */
int	initsolidguessing;	/* initial solid-guessing md*/
int	initfractype;		/* initial type set flag    */
int	initcyclelimit;		/* initial cycle limit      */
int	initcorners;		/* initial flag: corners set*/
double	initxmin,initxmax;	/* initial corner values    */
double	initymin,initymax;	/* initial corner values    */
double	initparam[4];		/* initial parameters       */
double	initpot[4];		/* potential parameters -TW 6/25/89  */
extern int Printer_Resolution, LPTNumber, Printer_Type;   /* for printer functions */

extern	char *fkeys[];		/* Function Key names for display table */

static	int toolsfile;		/* 1 if inside a TOOLS file, 0 otherwise */

/*
	cmdfiles(argc,argv) process the command-line arguments
		it also processes the 'sstools.ini' file and any
		indirect files ('fractint @myfile')
*/

int cmdfiles(argc,argv)
int argc;
char *argv[];
{
double	atof(), ftemp;				/* floating point stuff    */
int	i, j, k, l;				/* temporary loop counters */

char	param[81];				/* temporary strings        */

char tempstring[101];				/* temporary strings	    */
FILE *initfile;					/* for .INI, '@' files      */

warn = 0;					/* no warnings on savename */
sound = 1;					/* sound is on            */
initbatch = 0;					/* not in batch mode      */
initmode = -1;					/* no initial video mode  */
inside = 1;					/* inside color = blue    */
inititer = 150;					/* initial maxiter        */
initincr = 50;					/* initial iter increment */
initpass = 2;					/* initial dual-pass mode */
initsolidguessing = 1;				/* initial solid-guessing */
initfractype = 0;				/* initial type Set flag  */
initcorners = 0;				/* initial flag: no corners */
for (i = 0; i < 4; i++) initparam[i] = 0;	/* initial parameter values */
for (i = 0; i < 4; i++) initpot[i] = 0;/* TW 6/25/89 initial parameter values */
initxmin = -2.5; initxmax = 1.5;		/* initial corner values  */
initymin = -1.5; initymax = 1.5;		/* initial corner values  */
strcpy(savename,"fract001");			/* initial save filename  */
potfile[0] = NULL;               /* initial potfile value */
initcyclelimit=55;				/* spin-DAC default speed limit */

debugflag = 0;					/* debugging flag(s) are off */
timerflag = 0;					/* timer flags are off      */

display3d = 0;					/* 3D display is off        */
overlay3d = 0;					/* 3D overlay is off	    */
/* 3D defaults */
SPHERE    = FALSE;    
XROT      = 60;
YROT      = 30;
ZROT      = 0;
XSCALE    = 90;
YSCALE    = 90;
ROUGH     = 30;   
WATERLINE = 0;
FILLTYPE  = 0;
ZVIEWER   = 0;
XSHIFT    = 0;
YSHIFT    = 0;
/*  vvvvv TW 7/9/89 vvvvv  */
XLIGHT    = 0;
YLIGHT    = 0;
ZLIGHT    = 1;
LIGHTAVG  = 1;
/*  ^^^^^ TW 7/9/89 ^^^^^  */

*readname= NULL;                                  /* initial input filename */

Printer_Type = 2;				/* assume an IBM/EPSON */
if (Printer_Type == 1)				/* assume low resolution */
	Printer_Resolution = 75;
else
	Printer_Resolution = 60;
LPTNumber = 1 ;					/* assume LPT1 */

toolsfile = 1;					/* enable TOOLS processing */

#ifdef __TURBOC__				/* look for SSTOOLS.INI */
strcpy(tempstring,searchpath("sstools.ini"));
#else
_searchenv("sstools.ini","PATH",tempstring);
#endif
if (tempstring[0] != 0) {			/* found it! */
	if (strcmp(&tempstring[2],"\\\\sstools.ini") == 0)	/* stupid klooge! */
		strcpy(&tempstring[2],"\\sstools.ini");
	if ((initfile = fopen(tempstring,"r")) != NULL) {
		cmdfile(initfile);		/* process it */
		}
	}

toolsfile = 0;					/* disable TOOLS processing */

for (i = 1; i < argc; i++) {			/* cycle through args	*/
	strcpy(param,argv[i]);
	strlwr(param);				/* using lower case	*/
	for (j = 1; j < strlen(param) && param[j] != '='; j++) ;

	if (j < strlen(param)) {		/* xxx=yyy argument? */
		cmdarg(param);			/* process it */
		continue;
		}
		
	if (param[0] == ';')			/* start of comments? */
		break;				/* we done! */
		
	if (param[0] == '@') {			/* command indirection? */
		if ((initfile = fopen(&param[1],"r")) != NULL) {
			cmdfile(initfile);	/* process it */
			continue;
			}
		else argerror(param);		/* oops.  error. */
		}
	
        strcpy(readname,param);			/* else, assume a filename */
		showfile = 1;

	}

return(0);					/* we done */
}

/*
	cmdfile(handle) processes a single command-file.
		if (toolsfile), it looks for '[...]' codes as well
*/

cmdfile(handle)				/* process a command file of some sort */
FILE *handle;
{
char line[513];
int toolssection;
int i, j;

toolssection = 1;			/* assume an implied [fractint] */

while (fgets(line,512,handle) != NULL) {	/* read thru a line at a time */
	i = strlen(line);
	if (i > 0 && line[i-1] == '\n') line[i-1] = 0;	/* strip trailing \n */

	strlwr(line);				/* convert to lower case */
	if (toolsfile && line[0] == '[') {	/* TOOLS-style header */
		toolssection = 0;
		if (strncmp(line,"[fractint]",10) == 0)
			toolssection = 1;
		continue;			/* ignore this line in any case */
		}

	if (! toolssection) continue;		/* not our section */

	i = -1;					/* get a running start */
	while (line[++i] != 0) {		/* scan through the line */
		if (line[i] <= ' ') continue;	/* white space */
		if (line[i] == ';') break;	/* comments */
		j = i;				/* argument starts here */
		while (line[++i] > ' ');	/* find the argument end */
		line[i] = 0;			/* force an end-of-string */
		if (j == 0 && strcmp(&line[j],"fractint") == 0)
			continue;		/* skip leading "fractint " */
		cmdarg(&line[j]);		/* process the argument */
		}
	}

fclose(handle);
}

/*
	cmdarg(string) processes a single command-line/command-file argument
		isolate 'variable=value' (or 'variable:=value') into
		its components and process it.
		All components have already been converted to lower case.
*/

cmdarg(char *param)				/* process a single argument */
{
	char	variable[21];			/* variable name goes here */
	char	value[81];			/* variable value goes here*/
	int	numval;				/* numeric value of arg    */
	char	charval;			/* character value of arg  */

	double	atof(), ftemp;			/* floating point stuff    */
	int	i, j, k, l;			/* temporary loop counters */
	char	*slash;				/* temporary string ptr    */

	strlwr(param);				/* using lower case       */
	for (j = 1; j < strlen(param) && param[j] != '='; j++) ;
	if (j > 20 || j >= strlen(param))
		argerror(param);		/* oops.  '=' not found   */

	strncpy(variable,param,j);		/* get the variable name  */
	variable[j] = 0;			/* truncate it            */
	if (j > 1 && variable[j-1] == ':')	/* strip any trailing ':' */
		variable[j-1] = 0;
	strcpy(value,&param[j+1]);		/* get the value string   */
	numval = atoi(value);			/* get any numeric value  */
	charval = value[0];			/* get any letter  value  */

	if (strcmp(variable,"filename") == 0) {		/* filename=?	*/
	        strcpy(readname,value);			/* set up filename */
		showfile = 1;
		}
	else if( strcmp(variable, "map") == 0 ) {	/* map option */
		SetColorPaletteName( value );
		}
	else if (strcmp(variable,"batch") == 0 ) {	/* batch=?	*/
		if (charval == 'c') {			/* config run   */
			makeconfig();
			goodbye();
			}
		if (charval == 'y')			/* batch = yes  */
			initbatch = 1;
		}
	else if (strcmp(variable,"warn") == 0 ) {	/* warn=?	*/
		if (charval == 'y')
			warn = 1;
		}
	else if (strcmp(variable,"type") == 0 ) {	/* type=?	*/
		for (k = 0; typelist[k] != NULL; k++)
			if (strcmp(value,typelist[k]) == 0)
				break;
		if (typelist[k] == NULL) argerror(param);
		initfractype = k;
		}
	else if (strcmp(variable,"inside") == 0 ) {	/* inside=?	*/
		inside = numval;
		}
	else if (strcmp(variable,"maxiter") == 0) {	/* maxiter=?	*/
		if (numval < 10 || numval > 1000) argerror(param);
		inititer = numval;
		}
	else if (strcmp(variable,"iterincr") == 0) {	/* iterincr=?	*/
		if (numval <= 0 || numval > inititer) argerror(param);
		initincr = numval;
		}
	else if (strcmp(variable,"passes") == 0) {	/* passes=?	*/
		initsolidguessing = 0;
		if ( charval == 'g') {			/* solid-guessing */
			numval = 2;
			initsolidguessing = 1;
			}
		if (numval < 1 || numval > 2) argerror(param);
		initpass = numval;
		}
	else if (strcmp(variable,"cyclelimit") == 0 ) {	/* cyclelimit=?	*/
		if (numval > 1 && numval <= 256)
			initcyclelimit = numval;
		}
	else if (strcmp(variable,"savename") == 0) {	/* savename=?	*/
		strcpy(savename,value);
		}
	else if (strcmp(variable,"video") == 0) {	/* video=?	*/
		for (k = 0; k < maxvideomode; k++) {
			strcpy(variable,fkeys[k]);
			strlwr(variable);
			if (strcmp(variable, value) == 0)
				break;
			}
			if (k == maxvideomode) argerror(param);
		initmode = k;
		}
	else if (strcmp(variable,"potential") == 0) {	/* potential=?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 4) 
		{
		        if(k < 3)  
			   initpot[k++] = atof(++slash);
			else 
			{
                           k++;
                           strcpy(potfile,++slash);
			}   
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
		}
        }

	else if (strcmp(variable,"params") == 0) {	/* params=?,?	*/
		if (initcorners == 0) {
			initxmin = -2.0; initxmax = 2.0;
			initymin = -1.5; initymax = 1.5;
			}
		k = 0;
		slash = strchr(param,'=');
		while ( k < 4) {
			initparam[k++] = atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		}
	else if (strcmp(variable,"corners") == 0) {	/* corners=?,?,?,? */
	        initcorners = 1;
		slash = strchr(param,'=');
		initxmin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initxmax=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initymin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initymax=atof(++slash);
		}
	else if (strcmp(variable,"3d") == 0) {		/* 3d=?/?/..	*/
		display3d = 1;				/* turn on 3D */
		k = 0;
		slash = strchr(param,'=');
		while ( k < 20) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') init3d[k] = l;
			if (k == 0 && SPHERE) {
				/* reset sphere defaults */
				SPHERE    = TRUE;    
				PHI1      =  180;    
				PHI2      =  0;   
				THETA1    =  -90;   
				THETA2    =  90;   
				RADIUS    =  100;   
				ROUGH     =  30;   
				WATERLINE = 0;
				FILLTYPE  = 2;
				ZVIEWER   = 0;
				XSHIFT    = 0;
				YSHIFT    = 0;
        /*  vvvvv TW 7/9/89 vvvvv  */
                XLIGHT    = 0;
                YLIGHT    = 0;
                ZLIGHT    = 1;
                LIGHTAVG  = 1;
        /*  ^^^^^ TW 7/9/89 ^^^^^  */
				}   
			if (k == 0 && !SPHERE) {
				SPHERE    = FALSE;    
				XROT      = 60;
				YROT      = 30;
				ZROT      = 0;
				XSCALE    = 90;
				YSCALE    = 90;
				ROUGH     = 30;   
				WATERLINE = 0;
				FILLTYPE  = 0;
				ZVIEWER   = 0;
				XSHIFT    = 0;
				YSHIFT    = 0;
        /*  vvvvv TW 7/9/89 vvvvv  */
                XLIGHT    = 0;
                YLIGHT    = 0;
                ZLIGHT    = 1;
                LIGHTAVG  = 1;
        /*  ^^^^^ TW 7/9/89 ^^^^^  */
				}
			if ((slash = strchr(slash,'/')) == NULL) break;
			k++;
			}
		}
        /*  vvvvv MDS 7/1/89  vvvvv  */
        else if (strcmp(variable,"printer") == 0 ) {	/* printer=?	*/
                if (charval=='h') Printer_Type=1; /* HP LaserJet           */
                if (charval=='i') Printer_Type=2; /* IBM Graphics          */
                if (charval=='e') Printer_Type=2; /* Epson (model?)        */
		if (Printer_Type == 1)		/* assume low resolution */
			Printer_Resolution = 75;
		else
			Printer_Resolution = 60;
                slash=strchr(param,'=');
                if ((slash=strchr(slash,'/')) == NULL) return;
                if ((k=atoi(++slash)) > 0) Printer_Resolution=k;
                if ((slash=strchr(slash,'/')) == NULL) return;
		if ((k=atoi(++slash))> 0) LPTNumber = k;
                }
	/*  ^^^^^  MDS 7/1/89  ^^^^^  */
	else if (strcmp(variable,"sound") == 0 ) {	/* sound=?	*/
		sound = 0;				/* sound is off */
		if (charval == 'y')
			sound = 1;			/* sound is on  */
		}
	else if (strcmp(variable,"debugflag") == 0 ||
		 strcmp(variable,"debug") == 0) {	/* internal use only */
		debugflag = numval;
		timerflag = debugflag & 1;		/* separate timer flag */
		debugflag -= timerflag;
		}

	else argerror(param);

}
