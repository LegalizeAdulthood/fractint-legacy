
/*
	Command-line / Command-File Parser Routines
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <float.h>
#include <ctype.h>
#include "fractint.h"

/* variables defined by the command line/files processor */
char	temp1[256];		/* temporary strings        */
char	ifsfilename[80];          /* IFS code file */
char    readname[80];     	/* name of fractal input file */
char	potfile[80];		/* save potential using this name  */
char	savename[80];		/* save files using this name */
char	ifs3dfilename[80];          /* IFS 3D code file */
int	askvideo;		/* flag for video prompting */
int	ramvideo;		/* if zero, skip RAM-video for EXP-RAM */
char	floatflag = 0;         /* flag for float calcs */
int	biomorph  = -1;         /* flag for biomorph */
int     forcesymmetry = 999;   /* force symmetry */
int     showfile;        	/* has file been displayed yet? */
int rflag, rseed;	/* Random number seeding flag and value */
int decomp[2];		/* Decomposition coloring */
int	warn;			/* 0 if savename warnings off, 1 if on */
int	sound;			/* 0 if sound is off, 1 if on */
int	debugflag;		/* internal use only - you didn't see this */
int	timerflag;		/* you didn't see this, either */
int	cyclelimit;		/* color-rotator upper limit */
int	inside;			/* inside color: 1=blue     */
int bof_pp60_61;    /* flag for images in "Beauty of Fractals" pages 60-61 */
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
int bailout = 0;			/* user input bailout value */
double   inversion[3];      /* radius, xcenter, ycenter */
double	initxmin,initxmax;	/* initial corner values    */
double	initymin,initymax;	/* initial corner values    */
double	initparam[4];		/* initial parameters       */
extern double  potparam[];  /* potential parameters  */
extern int Printer_Resolution, LPTNumber, Printer_Type;   /* for printer functions */
int	transparent[2];		/* transparency min/max values */
int	LogFlag;			/* Logarithmic palette flag: 0 = no */

char FormFileName[80];		/* file to find (type=)formulas in */
char FormName[40];		/* Name of the Formula (if not null) */

extern unsigned char olddacbox[256][3];    /* Video-DAC saved values */
extern unsigned char dacbox[256][3];       /* Video-DAC values */

extern	char *fkeys[];		/* Function Key names for display table */

static	int toolsfile;		/* 1 if inside a TOOLS file, 0 otherwise */

extern char MAP_name[];
extern int mapset;

/* M */
extern int eyeseparation; /* Occular Separation */
extern int glassestype;
extern int xadjust; /* Convergence */
extern int xtrans, ytrans; /* X,Y shift with no perspective */
extern int red_crop_left, red_crop_right;
extern int blue_crop_left, blue_crop_right;
extern int red_bright, blue_bright;
extern char showbox; /* flag to show box and vector in preview */
extern char preview;        /* 3D preview mode flag */
extern int previewfactor; /* Coarsness */


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

rflag = 0;					/* Use time() for srand() */
floatflag = 0;					/* turn off the float flag */
biomorph = -1;					/* turn off biomorph flag */
askvideo = 1;					/* turn on video-prompt flag */
ramvideo = 1;					/* enable RAM-video */
warn = 0;					    /* no warnings on savename */
sound = 1;					    /* sound is on            */
initbatch = 0;					/* not in batch mode      */
initmode = -1;					/* no initial video mode  */
inside = 1;					    /* inside color = blue    */
bof_pp60_61 = 0;            
inititer = 150;					/* initial maxiter        */
initincr = 50;					/* initial iter increment */
initpass = 2;					/* initial dual-pass mode */
initsolidguessing = 1;		    /* initial solid-guessing */
initfractype = 0;				/* initial type Set flag  */
initcorners = 0;				/* initial flag: no corners */
for (i = 0; i < 4; i++) initparam[i] = FLT_MAX;	/* initial parameter values */
for (i = 0; i < 3; i++) potparam[i]  = 0.0; /* initial potential values */
for (i = 0; i < 3; i++) inversion[i] = 0.0;  /* initial invert values */

initxmin = -2.5; initxmax = 1.5;		/* initial corner values  */
initymin = -1.5; initymax = 1.5;		/* initial corner values  */
strcpy(savename,"fract001");			/* initial save filename  */
potfile[0] = NULL;                              /* initial potfile value */
initcyclelimit=55;				/* spin-DAC default speed limit */
transparent[0] = transparent[1] = 0;		/* no min/max transparency */
LogFlag = 0;					/* no logarithmic palette */
ifsfilename[0] = NULL;          /* initial ifs file name */
ifs3dfilename[0] = NULL;        /* initial ifs3d file value */

debugflag = 0;					/* debugging flag(s) are off */
timerflag = 0;					/* timer flags are off      */

display3d = 0;					/* 3D display is off        */
overlay3d = 0;					/* 3D overlay is off	    */

/* 3D defaults */
SPHERE = FALSE;
set_3d_defaults();

*readname= NULL;                                  /* initial input filename */

Printer_Type = 2;				/* assume an IBM/EPSON */
if (Printer_Type == 1)				/* assume low resolution */
	Printer_Resolution = 75;
else
	Printer_Resolution = 60;
LPTNumber = 1 ;					/* assume LPT1 */

strcpy(FormFileName,"fractint.frm");		/* default formula file */
strcpy(FormName,"");				/* default formula name */


toolsfile = 1;					/* enable TOOLS processing */

findpath("sstools.ini", tempstring);		/* look for SSTOOLS.INI */
if (tempstring[0] != 0) 			/* found it! */
	if ((initfile = fopen(tempstring,"r")) != NULL) 
		cmdfile(initfile);		/* process it */

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
	else line[i+1] = 0;				/* add second null   */

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
		return;
		}
	if( strcmp(variable, "map") == 0 ) {	/* map option */
		mapset = 1;
		strcpy (temp1,value);
		if (strchr(temp1,'.') == NULL) /* Did name have an extention? */
			strcat(temp1,".map"); /* No? Then add .map */
		findpath(temp1,MAP_name); /* Find complete path name */
		SetColorPaletteName( temp1 );
		return;
		}
	if (strcmp(variable,"batch") == 0 ) {		/* batch=?	*/
		if (charval == 'c') {			/* config run   */
			makeconfig();
			goodbye();
			}
		if (charval == 'y')			/* batch = yes  */
			initbatch = 1;
		return;
		}
	if (strcmp(variable,"warn") == 0 ) {		/* warn=?	*/
		if (charval == 'y')
			warn = 1;
		return;
		}
	if (strcmp(variable,"type") == 0 ) {		/* type=?	*/
		if (value[strlen(value)-1] == '*')
			value[strlen(value)-1] = 0;
		for (k = 0; fractalspecific[k].name != NULL; k++)
			if (strcmp(value,fractalspecific[k].name) == 0)
				break;
		if (fractalspecific[k].name == NULL) argerror(param);
		initfractype = k;
		if (initcorners == 0) {
			initxmin = fractalspecific[initfractype].xmin;
			initxmax = fractalspecific[initfractype].xmax;
			initymin = fractalspecific[initfractype].ymin;
			initymax = fractalspecific[initfractype].ymax;
			}
		return;
		}
	if (strcmp(variable,"inside") == 0 ) {		/* inside=?	*/
	    if(strcmp(value,"bof60")==0)
	    	bof_pp60_61 = 60;
	    else if(strcmp(value,"bof61")==0)
			bof_pp60_61 = 61;
        else if(isalpha(*value))
		    argerror(param);
	    else   
	    	inside = numval;
		return;
		}
	if (strcmp(variable,"maxiter") == 0) {		/* maxiter=?	*/
		if (numval < 10 || numval > 32000) argerror(param);
		inititer = numval;
		return;
		}
	if (strcmp(variable,"iterincr") == 0) {		/* iterincr=?	*/
		if (numval <= 0 || numval > 32000) argerror(param);
		initincr = numval;
		return;
		}
	if (strcmp(variable,"passes") == 0) {		/* passes=?	*/
		initsolidguessing = 0;
		if ( charval == 'g') {			/* solid-guessing */
			numval = 2;
			initsolidguessing = 1;
			}
		if (numval < 1 || numval > 2) argerror(param);
		initpass = numval;
		return;
		}
	if (strcmp(variable,"cyclelimit") == 0 ) {	/* cyclelimit=?	*/
		if (numval > 1 && numval <= 256)
			initcyclelimit = numval;
		return;
		}
	if (strcmp(variable,"savename") == 0) {		/* savename=?	*/
		strcpy(savename,value);
		return;
		}
	if (strcmp(variable,"video") == 0) {		/* video=?	*/
		for (k = 0; k < maxvideomode; k++) {
			strcpy(variable,fkeys[k]);
			strlwr(variable);
			if (strcmp(variable, value) == 0)
				break;
			}
			if (k == maxvideomode) argerror(param);
		initmode = k;
		return;
		}
	if (strcmp(variable,"potential") == 0) {	/* potential=?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 4) {
        	if(k < 3)  
				potparam[k++] = atoi(++slash);
		   	else {
            	k++;
                strcpy(potfile,++slash);
				}   
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		return;
	        }
	if (strcmp(variable,"params") == 0) {		/* params=?,?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 4) {
			initparam[k++] = atof(++slash);
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		return;
		}
	if (strcmp(variable,"corners") == 0) {		/* corners=?,?,?,? */
	        initcorners = 1;
		slash = strchr(param,'=');
		initxmin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initxmax=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initymin=atof(++slash);
		if ((slash = strchr(slash,'/')) == NULL) argerror(param);
		initymax=atof(++slash);
		return;
		}
	if (strcmp(variable,"3d") == 0) {		/* 3d=?/?/..	*/
		display3d = 1;				/* turn on 3D */
		k = 0;
		slash = strchr(param,'=');
		while ( k < 20) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') init3d[k] = l;

            /* reset sphere defaults */
			if (k == 0)
				set_3d_defaults();

			if ((slash = strchr(slash,'/')) == NULL) break;
			k++;
			}
		return;
		}
	if (strcmp(variable,"sphere") == 0 ) {		/* sphere=?	*/
		if (charval == 'y') 
			SPHERE    = TRUE;    
		else if (charval == 'n')
			SPHERE    = FALSE;
		else 
		    argerror(param);		/* oops.  error. */
		return;
		}
	if (strcmp(variable,"rotation") == 0) {		/* rotation=?/?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XROT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"scalexyz") == 0) {		/* scalexyz=?/?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XSCALE+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
    /* "rough" is really scale z, but we add it here for convenience */
	if (strcmp(variable,"roughness") == 0) {	/* roughness=?	*/
		ROUGH = numval;
		return;
		}
	if (strcmp(variable,"waterline") == 0) {	/* waterline=?	*/
		if (numval<0) argerror(param);
		WATERLINE = numval;
		return;
		}
	if (strcmp(variable,"filltype") == 0) {		/* filltype=?	*/
		if (numval < -1 || numval > 6) argerror(param);
		FILLTYPE = numval;
		return;
		}
	if (strcmp(variable,"perspective") == 0) {	/* perspective=?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 1) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&ZVIEWER+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"xyshift") == 0) {		/* xyshift=?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XSHIFT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"lightsource") == 0) {	/* lightsource=?/?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 3) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&XLIGHT+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"smoothing") == 0) {	/* smoothing=?	*/
		if (numval<0) argerror(param);
		LIGHTAVG = numval;
		return;
		}
	if (strcmp(variable,"latitude") == 0) {		/* latitude=?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&THETA1+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"longitude") == 0) {	/* longitude=?/?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 2) {
			l = atoi(++slash);
			if (slash[0] > 32 && slash[0] != '/') *(&PHI1+k) = l;
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			k++;
			}
		return;
		}
	if (strcmp(variable,"radius") == 0) {		/* radius=?	*/
		if (numval<0) argerror(param);
		RADIUS = numval;
		return;
		}
	if (strcmp(variable,"invert") == 0) {		/* invert=?,?,?	*/
		k = 0;
		slash = strchr(param,'=');
		while ( k < 3) {
		    extern int invert;
			inversion[k++] = atof(++slash);
            if(inversion[0] != 0.0)
    		    invert = k;      /* record highest inversion parameter set */
			if ((slash = strchr(slash,'/')) == NULL) k = 99;
			}
		return;
		}
	if (strcmp(variable,"askvideo") == 0 ) { 	/* askvideo=?	*/
		if (charval == 'y')
			askvideo = 1;
		else if (charval == 'n')
		        askvideo = 0;
	        else
        	    argerror(param);
		return;
        	}    
	if (strcmp(variable,"ramvideo") == 0 ) { 	/* ramvideo=?	*/
		if (charval == 'y')
			ramvideo = 1;
		else if (charval == 'n')
		        ramvideo = 0;
	        else
        	    argerror(param);
		return;
        	}    
	if (strcmp(variable,"float") == 0 ) {	 	/* float=?	*/
		if (charval == 'y')
			floatflag = 1;
		else if (charval == 'n')
		            floatflag = 0;
	        else
        	    argerror(param);
		return;
        	}    
	if (strcmp(variable,"biomorph") == 0 ) { 	/* biomorph=?	*/
		biomorph = numval;
		return;
          	}   
	if (strcmp(variable,"bailout") == 0 ) { 	/* bailout=?	*/
		if (numval < 4 || numval > 32000) argerror(param);
		bailout = numval;
		return;
		}
	if (strcmp(variable,"symmetry") == 0 ) { 	/* symmetry=?	*/
	    if     (strcmp(value,"xaxis" )==0) forcesymmetry = XAXIS;
	    else if(strcmp(value,"yaxis" )==0) forcesymmetry = YAXIS;
	    else if(strcmp(value,"xyaxis")==0) forcesymmetry = XYAXIS;
	    else if(strcmp(value,"origin")==0) forcesymmetry = ORIGIN;
	    else if(strcmp(value,"pi"    )==0) forcesymmetry = PI_SYM;
        else if(strcmp(value,"none"  )==0) forcesymmetry = NOSYM;
        else argerror(param);
		return;
		}

	if (strcmp(variable,"printer") == 0 ) {	/* printer=?	*/
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
	return;
        }
	if (strcmp(variable,"transparent") == 0) { /* transparent? */
		slash = strchr(param,'=');
        if ((k=atoi(++slash)) > 0) transparent[0] = k;
		transparent[1] = transparent[0];
        if ((slash=strchr(slash,'/')) == NULL) return;
		if ((k=atoi(++slash)) > 0) transparent[1] = k;
		return;
		}
	if (strcmp(variable,"sound") == 0 ) {		/* sound=?	*/
		sound = 0;				/* sound is off */
		if (charval == 'y')
			sound = 1;			/* sound is on  */
		return;
		}
	if (strcmp(variable,"logmap") == 0 ) {		/* logmap=?	*/
		LogFlag = 0;				/* palette is continuous */
		if (charval == 'y')
			LogFlag = 1;			/* palette is logarithmic */
		return;
		}
	if (strcmp(variable,"debugflag") == 0 ||
		 strcmp(variable,"debug") == 0) {	/* internal use only */
		debugflag = numval;
		timerflag = debugflag & 1;		/* separate timer flag */
		debugflag -= timerflag;
		return;
		}
	if (strcmp(variable,"ifs") == 0) {		/* ifs=?	*/
		strcpy(ifsfilename,value);
		if (strchr(value,'.') == NULL)
			strcat(ifsfilename,".ifs");
		ifsgetfile();
		return;
		}
	if (strcmp(variable,"ifs3d") == 0) {		/* ifs3d=?	*/
		strcpy(ifs3dfilename,value);
		if (strchr(value,'.') == NULL)
			strcat(ifs3dfilename,".ifs");
		ifs3dgetfile();
		return;
		}
	if (strcmp(variable,"ifscodes") == 0) {    /* ifscodes=?,?,?,? */
    	int ifsindex;
        slash = strchr(param,'=');
        ifsindex=atoi(++slash) - 1;
        if(ifsindex < 0 || ifsindex > NUMIFS) argerror(param);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][0]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][1]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][2]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][3]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][4]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][5]=atof(++slash);
        if ((slash = strchr(slash,'/')) == NULL) argerror(param);
        initifs[ifsindex][6]=atof(++slash);
	return;
        }
	if (strcmp(variable, "rseed") == 0) {
		rseed = numval;
		rflag = 1;
		return;
		}
	if (strcmp(variable, "decomp") == 0) {
		k = 0;
		slash = strchr(param,'=');
		while (k < 2) {
			decomp[k++] = atoi(++slash);
			if ((slash = strchr(slash,'/')) == NULL) break;
			}
		return;
		}
	if (strcmp(variable,"formulafile") == 0) {	/* formulafile=?	*/
		strcpy(FormFileName,value);
		return;
		}
	if (strcmp(variable,"formulaname") == 0) {	/* formulaname=?	*/
		strcpy(FormName,value);
		return;
		}

/* M */
    if (strcmp(variable,"preview") == 0) { /* preview? */
	if (charval == 'y')
		preview = 1;
	return;
	}
    if (strcmp(variable,"showbox") == 0) { /* showbox? */
	if (charval == 'y')
		showbox = 1;
	return;
	}
    if (strcmp(variable,"coarse") == 0) {  /* coarse=? */
        if (numval<1) argerror(param);
        previewfactor = numval;
	return;
        }
    if (strcmp(variable,"stereo") == 0) {  /* stereo=? */
        if ((numval<0) || (numval>3)) argerror(param);
        glassestype = numval;
	return;
        }
    if (strcmp(variable,"interocular") == 0) {  /* interocular=? */
        eyeseparation = numval;
	return;
        }
    if (strcmp(variable,"converge") == 0) {  /* converg=? */
        xadjust = numval;
	return;
        }
    if (strcmp(variable,"crop") == 0) {  /* crop=? */
		slash = strchr(param,'=');
        if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) red_crop_left = k;
        else argerror(param);
        if ((slash=strchr(slash,'/')) == NULL) return;
        if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) red_crop_right = k;
        else argerror(param);
        if ((slash=strchr(slash,'/')) == NULL) return;
        if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) blue_crop_left = k;
        else argerror(param);
        if ((slash=strchr(slash,'/')) == NULL) return;
        if ((k=atoi(++slash)) >= 0 && atoi(slash) <= 100) blue_crop_right = k;
        else argerror(param);
	return;
        }
    if (strcmp(variable,"bright") == 0) {  /* bright=? */
		slash = strchr(param,'=');
        if ((k=atoi(++slash)) >= 0) red_bright = k;
        else argerror(param);
        if ((slash=strchr(slash,'/')) == NULL) return;
        if ((k=atoi(++slash)) >= 0) blue_bright = k;
        else argerror(param);
	return;
        }
    if (strcmp(variable,"xyadjust") == 0) { /* trans=? */
		slash = strchr(param,'=');
        xtrans=atoi(++slash);
        if ((slash=strchr(slash,'/')) == NULL) return;
        ytrans=atoi(++slash);
	return;
        }


    argerror(param);
}

#ifdef __TURBOC__
#include <dir.h>
#endif

findpath(char *filename, char *fullpathname)	/* return full pathnames */
{

fullpathname[0] = 0;				/* indicate none found */

#ifdef __TURBOC__				/* look for the file */
strcpy(fullpathname,searchpath(filename));
#else
_searchenv(filename,"PATH",fullpathname);
#endif
if (fullpathname[0] != 0) 			/* found it! */
	if (strncmp(&fullpathname[2],"\\\\",2) == 0)	/* stupid klooge! */
		strcpy(&fullpathname[3],filename);
}

set_3d_defaults()
{
   if(SPHERE)
   {
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
/* M */
      xtrans    = 0;
      ytrans    = 0;
      XLIGHT    = 1;
      YLIGHT    = 1;
      ZLIGHT    = 1;
      LIGHTAVG  = 0;
   	}   
   	else
   	{
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
/* M */
      xtrans    = 0;
      ytrans    = 0;
      XLIGHT    = 1;
      YLIGHT    = -1;
      ZLIGHT    = 1;
      LIGHTAVG  = 0;
   	}
    return(0);
}
