/* FRACTINT.H - common structures and values for the FRACTINT routines */


#ifndef C6
#define _fastcall	/* _fastcall is a Microsoft C6.00 extension */
#endif

#ifdef __TURBOC__
#   define _bios_printer(a,b,c)   biosprint((a),(c),(b))
#   define _bios_serialcom(a,b,c) bioscom((a),(c),(b))
#else
#   define MK_FP(seg,off) (void far *)( (((long)(seg))<<16) | \
					((unsigned)(off)) )
#endif


#define MAXPIXELS 2048		/* Maximum pixel count across/down the screen */
#define SCREENASPECT 0.75	/* Assumed overall screen dimensions, y/x     */

struct videoinfo {		/* All we need to know about a Video Adapter */
	char	name[26];	/* Adapter name (IBM EGA, etc)		*/
	char	comment[26];	/* Comments (UNTESTED, etc)		*/
	int	keynum; 	/* key number used to invoked this mode */
				/* 2-10 = F2-10, 11-40 = S,C,A{F1-F10}	*/
	int	videomodeax;	/* begin with INT 10H, AX=(this)	*/
	int	videomodebx;	/*		...and BX=(this)	*/
	int	videomodecx;	/*		...and CX=(this)	*/
	int	videomodedx;	/*		...and DX=(this)	*/
				/* NOTE:  IF AX==BX==CX==0, SEE BELOW	*/
	int	dotmode;	/* video access method used by asm code */
				/*	1 == BIOS 10H, AH=12,13 (SLOW)	*/
				/*	2 == access like EGA/VGA	*/
				/*	3 == access like MCGA		*/
				/*	4 == Tseng-like  SuperVGA*256	*/
				/*	5 == P'dise-like SuperVGA*256   */
				/*	6 == Vega-like	 SuperVGA*256	*/
				/*	7 == "Tweaked" IBM-VGA ...*256  */
				/*	8 == "Tweaked" SuperVGA ...*256 */
				/*	9 == Targa Format		*/
				/*	10 = Hercules			*/
				/*	11 = "disk video" (no screen)   */
				/*	12 = 8514/A			*/
				/*	13 = CGA 320x200x4, 640x200x2	*/
				/*	14 = Tandy 1000 		*/
				/*	15 = TRIDENT  SuperVGA*256	*/
				/*	16 = Chips&Tech SuperVGA*256	*/
	int	xdots;		/* number of dots across the screen	*/
	int	ydots;		/* number of dots down the screen	*/
	int	colors; 	/* number of colors available		*/
	};


#define INFO_ID 	"Fractal"
#define FRACTAL_INFO   struct fractal_info

struct fractal_info			/*  for saving data in GIF file     */
{
	char info_id[8];		/* Unique identifier for info block */
	int iterations;
	int fractal_type;	    /* 0=Mandelbrot 1=Julia 2= ... */
	double	xmin;
	double	xmax;
	double	ymin;
	double	ymax;
	double	creal;
	double	cimag;
	int	videomodeax;
	int	videomodebx;
	int	videomodecx;
	int	videomodedx;
	int	dotmode;
	int	xdots;
	int	ydots;
	int colors;
	int version;	    /* used to be 'future[0]' */
    float parm3;
    float parm4;
    float potential[3];
    int rseed;
    int rflag;
    int biomorph;
    int inside;
    int logmap;
    float invert[3];
    int decomp[2];
    int symmetry;
    /* version 2 stuff */
    int init3d[16];
    int previewfactor;
    int xtrans;
    int ytrans;
    int red_crop_left;
    int red_crop_right;
    int blue_crop_left;
    int blue_crop_right;
    int red_bright;
    int blue_bright;
    int xadjust;
    int eyeseparation;
    int glassestype;
    /* version 3 stuff, release 13 */
    int outside;
    /* version 4 stuff, release 14 */
    double x3rd;	  /* 3rd corner */
    double y3rd;
    char stdcalcmode;	  /* 1/2/g/b */
    char useinitorbit;	  /* init Mandelbrot orbit flag */
    int calc_status;	  /* resumable, finished, etc */
    long tot_extend_len;  /* total length of extension blocks in .gif file */
    int distest;
    int floatflag;
    int bailout;
    long calctime;
    unsigned char trigndx[4]; /* which trig functions selected */
    int finattract;
    double initorbit[2];  /* init Mandelbrot orbit values */
    int periodicity;	  /* periodicity checking */
    /* version 5 stuff, release 15 */
    int pot16bit;	  /* save 16 bit continuous potential info */
    float faspectratio;   /* finalaspectratio, y/x */
    int system; 	  /* 0 for dos, 1 for windows */
    int release;	  /* release number, with 2 decimals implied */
    int flag3d; 	  /* stored only for now, for future use */
    int transparent[2];
    int ambient;
    int haze;
    int randomize;
    /* version 6 stuff, release 15.x */
    int rotate_lo;
    int rotate_hi;
    int distestwidth;
    /* version 7 stuff, release 16 */
    double dparm3;
    double dparm4;
    /* version 8 stuff, release 17 */
    int fillcolor;
    int future[31];	  /* for stuff we haven't thought of yet */
};

#define MAXVIDEOMODES 300	/* maximum entries in fractint.cfg	  */
#define MAXVIDEOTABLE 40	/* size of the resident video modes table */

#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

struct videoinfo videoentry;

#else

extern struct videoinfo videoentry;

#endif

#define NUMIFS	  32	 /* number of ifs functions in ifs array */
#define IFSPARM    7	 /* number of ifs parameters */
#define IFS3DPARM 13	 /* number of ifs 3D parameters */

#define ITEMNAMELEN 18	 /* max length of names in .frm/.l/.ifs/.fc */

/* defines for symmetry */
#define  NOSYM		0
#define  XAXIS_NOPARM  -1
#define  XAXIS		1
#define  YAXIS_NOPARM  -2
#define  YAXIS		2
#define  XYAXIS_NOPARM -3
#define  XYAXIS 	3
#define  ORIGIN_NOPARM -4
#define  ORIGIN 	4
#define  PI_SYM_NOPARM -5
#define  PI_SYM 	5
#define  XAXIS_NOIMAG  -6
#define  XAXIS_NOREAL	6
#define  NOPLOT        99
#define  SETUP_SYM    100

/* bitmask defines for fractalspecific flags */
#define  NOZOOM 	1    /* zoombox not allowed at all	   */
#define  NOGUESS	2    /* solid guessing not allowed	   */
#define  NOTRACE	4    /* boundary tracing not allowed	   */
#define  NOROTATE	8    /* zoombox rotate/stretch not allowed */
#define  NORESUME      16    /* can't interrupt and resume         */
#define  INFCALC       32    /* this type calculates forever	   */
#define  TRIG1	       64    /* number of trig functions in formula*/
#define  TRIG2	      128
#define  TRIG3	      192
#define  TRIG4	      256
#define  WINFRAC      512    /* supported in WinFrac		   */
#define  PARMS3D     1024    /* uses 3d parameters		   */

#define AUTOINVERT -123456.789

#define N_ATTR 8			/* max number of attractors	*/

extern	long	 l_at_rad;	/* finite attractor radius  */
extern	double	 f_at_rad;	/* finite attractor radius  */

struct fractalspecificstuff
{
   char  *name; 			/* name of the fractal */
   char  *param[4];			/* name of the parameters */
   float paramvalue[4]; 		/* default parameter values */
   int	 helptext;			/* helpdefs.h HT_xxxx, -1 for none */
   int	 helpformula;			/* helpdefs.h HF_xxxx, -1 for none */
   int	 flags; 			/* constraints, bits defined above */
   float xmin;				/* default XMIN corner */
   float xmax;				/* default XMAX corner */
   float ymin;				/* default YMIN corner */
   float ymax;				/* default YMAX corner */
   int	 isinteger;			/* 1 if integerfractal, 0 otherwise */
   int	 tojulia;			/* mandel-to-julia switch */
   int	 tomandel;			/* julia-to-mandel switch */
   int	 tofloat;			/* integer-to-floating switch */
   int	 symmetry;			/* applicable symmetry logic
					   0 = no symmetry
					  -1 = y-axis symmetry (If No Params)
					   1 = y-axis symmetry
					  -2 = x-axis symmetry (No Parms)
					   2 = x-axis symmetry
					  -3 = y-axis AND x-axis (No Parms)
					   3 = y-axis AND x-axis symmetry
					  -4 = polar symmetry (No Parms)
					   4 = polar symmetry
					   5 = PI (sin/cos) symmetry
					   6 = NEWTON (power) symmetry
								*/
   int (*orbitcalc)();		/* function that calculates one orbit */
   int (*per_pixel)();		/* once-per-pixel init */
   int (*per_image)();		/* once-per-image setup */
   int (*calctype)();		/* name of main fractal function */
   int orbit_bailout;		/* usual bailout value for orbit calc */
};

extern struct fractalspecificstuff far fractalspecific[];
extern struct fractalspecificstuff far *curfractalspecific;


#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

int helpmode;

#else

extern int helpmode;

#endif

#define DEFAULTFRACTALTYPE	".gif"
#define ALTERNATEFRACTALTYPE	".fra"

#include <math.h>

#ifndef _LCOMPLEX_DEFINED
struct lcomplex {
   long x, y;
};
#define _LCOMPLEX_DEFINED
#endif

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

#ifndef lsqr
#define lsqr(x) (multiply((x),(x),bitshift))
#endif

#define CMPLXmod(z)	  (sqr((z).x)+sqr((z).y))
#define CMPLXconj(z)	((z).y =  -((z).y))
#define LCMPLXmod(z)	   (lsqr((z).x)+lsqr((z).y))
#define LCMPLXconj(z)	((z).y =  -((z).y))

typedef  struct complex CMPLX;
typedef  struct lcomplex LCMPLX;

/* 3D stuff - formerly in 3d.h */
#ifndef dot_product
#define dot_product(v1,v2)  ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2])  /* TW 7-09-89 */
#endif

#define    CMAX    4   /* maximum column (4 x 4 matrix) */
#define    RMAX    4   /* maximum row	 (4 x 4 matrix) */
#define    DIM	   3   /* number of dimensions */

typedef double MATRIX [RMAX] [CMAX];  /* matrix of doubles */
typedef int   IMATRIX [RMAX] [CMAX];  /* matrix of ints    */
typedef long  LMATRIX [RMAX] [CMAX];  /* matrix of longs   */

/* A MATRIX is used to describe a transformation from one coordinate
system to another.  Multiple transformations may be concatenated by
multiplying their transformation matrices. */

typedef double VECTOR [DIM];  /* vector of doubles */
typedef int   IVECTOR [DIM];  /* vector of ints    */
typedef long  LVECTOR [DIM];  /* vector of longs   */

/* A VECTOR is an array of three coordinates [x,y,z] representing magnitude
and direction. A fourth dimension is assumed to always have the value 1, but
is not in the data structure */


#define PI 3.14159265358979323846

#define SPHERE	  init3d[0]		/* sphere? 1 = yes, 0 = no  */
#define ILLUMINE  (FILLTYPE>4)	/* illumination model	    */

/* regular 3D */
#define XROT	  init3d[1]	/* rotate x-axis 60 degrees */
#define YROT	  init3d[2]	/* rotate y-axis 90 degrees */
#define ZROT	  init3d[3]	/* rotate x-axis  0 degrees */
#define XSCALE	  init3d[4]	/* scale x-axis, 90 percent */
#define YSCALE	  init3d[5]	/* scale y-axis, 90 percent */

/* sphere 3D */
#define PHI1	  init3d[1]	/* longitude start, 180     */
#define PHI2	  init3d[2]	/* longitude end ,   0	    */
#define THETA1	  init3d[3]	    /* latitude start,-90 degrees */
#define THETA2	  init3d[4]	    /* latitude stop,  90 degrees */
#define RADIUS	  init3d[5]	/* should be user input */

/* common parameters */
#define ROUGH	  init3d[6]	/* scale z-axis, 30 percent */
#define WATERLINE init3d[7]	/* water level		    */
#define FILLTYPE  init3d[8]	/* fill type		    */
#define ZVIEWER   init3d[9]	/* perspective view point   */
#define XSHIFT	  init3d[10]	/* x shift */
#define YSHIFT	  init3d[11]	/* y shift */
#define XLIGHT	  init3d[12]	/* x light vector coordinate */
#define YLIGHT	  init3d[13]	/* y light vector coordinate */
#define ZLIGHT	  init3d[14]	/* z light vector coordinate */
#define LIGHTAVG  init3d[15]	/* number of points to average */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


struct workliststuff	/* work list entry for std escape time engines */
{
	int xxstart;	/* screen window for this entry */
	int xxstop;
	int yystart;
	int yystop;
	int yybegin;	/* start row within window, for 2pass/ssg resume */
	int sym;	/* if symmetry in window, prevents bad combines */
	int pass;	/* for 2pass and solid guessing */
};
#define MAXCALCWORK 12

extern unsigned char trigndx[];
extern void (*ltrig0)(), (*ltrig1)(), (*ltrig2)(), (*ltrig3)();
extern void (*dtrig0)(), (*dtrig1)(), (*dtrig2)(), (*dtrig3)();

struct trig_funct_lst
{
    char *name;
    void (*lfunct)();
    void (*dfunct)();
    void (*mfunct)();
} ;
extern struct trig_funct_lst trigfn[];

/* function prototypes */

extern	void   buzzer(int);
extern	int    calcfract(void);
extern	int    calcmand(void);
extern  int    calcmandfp(void);
extern	int    check_key(void);
extern	int    complex_mult(CMPLX, CMPLX, CMPLX *);
extern	int    complex_div(CMPLX, CMPLX, CMPLX *);
extern	int    complex_power(CMPLX, int, CMPLX *);
extern	int    cross_product(double [], double [], double []);
extern	void   drawbox(int);
extern	unsigned int emmallocate(unsigned int);
extern	void   emmclearpage(unsigned int, unsigned int);
extern	void   emmdeallocate(unsigned int);
extern	unsigned int emmgetfree(void);
extern	void   emmgetpage(unsigned int, unsigned int);
extern	unsigned char far *emmquery(void);
extern far_strlen( char far *);
extern far_strcpy( char far *, char far *);
extern far_strcmp( char far *, char far *);
extern far_stricmp(char far *, char far *);
extern far_strnicmp(char far *, char far *,int);
extern far_strcat( char far *, char far *);
extern far_memset( void far *, char	 , int);
extern far_memcpy( void far *, void far *, int);
extern far_memcmp( void far *, void far *, int);
extern far_memicmp(void far *, void far *, int);
extern	void far *farmemalloc(long);
extern	void   farmemfree(void far *);
extern	int    getakey(void);
extern	int    _fastcall getcolor(int, int);
extern	void   _fastcall putcolor(int, int, int);
extern	void   (_fastcall *plot)(int, int, int);
extern	void   _fastcall symPIplot(int,int,int);
extern	void   _fastcall symPIplot2J(int,int,int);
extern	void   _fastcall symPIplot4J(int,int,int);
extern	void   _fastcall symplot2(int,int,int);
extern	void   _fastcall symplot2Y(int,int,int);
extern	void   _fastcall symplot2J(int,int,int);
extern	void   _fastcall symplot4(int,int,int);
extern	void   _fastcall symplot2basin(int,int,int);
extern	void   _fastcall symplot4basin(int,int,int);
extern	void   _fastcall noplot(int,int,int);
extern	void   _fastcall draw_line(int,int,int,int,int);
extern	int    has_8087(void );
extern	void   putstring(int,int,int,unsigned char far *);
extern	int    putstringcenter(int,int,int,int,char far *);
extern	int    stopmsg(int,unsigned char far *);
extern	void   identity(MATRIX);
extern	int    Juliafp(void);
extern	int    longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
extern	int    longpersp(LVECTOR, LVECTOR,int);
extern	int    Lambda(void);
extern	int    Lambdasine(void);
extern	void   mat_mul(MATRIX, MATRIX, MATRIX);
extern	void   main(int, char *[]);
extern	int    Mandelfp(void);
extern	long   multiply(long, long, int);
extern	long   divide(long, long, int);
extern	int    Newton(void);
extern	int    perspective(double *v);
extern	void   cdecl Print_Screen(void);
extern	void   scale(double, double, double, MATRIX);
extern	void   setvideomode(int, int, int, int);
extern	int    Sierpinski(void);
extern	void   spindac(int, int);
extern	void   trans(double, double, double, MATRIX);
extern	int    vmult(VECTOR,MATRIX,VECTOR);
extern	void   xrot(double, MATRIX);
extern	void   yrot(double, MATRIX);
extern	void   zrot(double, MATRIX);
extern	void   (_fastcall *standardplot)(int,int,int);
extern	void   _fastcall plot3dsuperimpose16b(int,int,int);
extern	void   _fastcall plot3dsuperimpose16(int,int,int);
extern	void   _fastcall plot3dsuperimpose256(int,int,int);
extern	void   _fastcall plotIFS3dsuperimpose256(int,int,int);
extern	void   _fastcall plot3dalternate(int,int,int);

/* for overlay return stack */

#define ENTER_OVLY(ovlyid)\
   extern int active_ovly;\
   int prev_ovly;\
   prev_ovly = active_ovly;\
   active_ovly = ovlyid
#define EXIT_OVLY active_ovly = prev_ovly

#define OVLY_MISCOVL   1
#define OVLY_CMDFILES  2
#define OVLY_HELP      3
#define OVLY_PROMPTS   4
#define OVLY_LOADFILE  5
#define OVLY_ROTATE    6
#define OVLY_PRINTER   7
#define OVLY_LINE3D    8
#define OVLY_ENCODER   9
#define OVLY_CALCFRAC 10
#define OVLY_INTRO    11

/* keys */
#define   INSERT	 1082
#define   DELETE	 1083
#define   PAGE_UP	 1073
#define   PAGE_DOWN	 1081
#define   CTL_HOME	 1119
#define   CTL_END	 1117
#define   LEFT_ARROW	 1075
#define   RIGHT_ARROW	 1077
#define   UP_ARROW	 1072
#define   DOWN_ARROW	 1080
#define   LEFT_ARROW_2	 1115
#define   RIGHT_ARROW_2  1116
#define   UP_ARROW_2	 1141
#define   DOWN_ARROW_2	 1145
#define   HOME		 1071
#define   END		 1079
#define   ENTER 	 13
#define   ENTER_2	 1013
#define   TAB		 9
#define   ESC		 27
#define   SPACE 	 32
#define   F1		 1059
#define   F2		 1060
#define   F3		 1061
#define   F4		 1062
#define   F5		 1063
#define   F6		 1064
#define   F7		 1065
#define   F8		 1066
#define   F9		 1067
#define   F10		 1068

/* text colors */
#define BLACK	   0
#define BLUE	   1
#define GREEN	   2
#define CYAN	   3
#define RED	   4
#define MAGENTA    5
#define BROWN	   6 /* dirty yellow on cga */
#define WHITE	   7
/* use values below this for foreground only, they don't work background */
#define GRAY	   8 /* don't use this much - is black on cga */
#define L_BLUE	   9
#define L_GREEN   10
#define L_CYAN	  11
#define L_RED	  12
#define L_MAGENTA 13
#define YELLOW	  14
#define L_WHITE   15
#define INVERSE 0x8000 /* when 640x200x2 text or mode 7, inverse */
#define BRIGHT	0x4000 /* when mode 7, bright */
/* and their use: */
extern unsigned char txtcolor[];
#define C_TITLE 	  txtcolor[0]+BRIGHT
#define C_TITLE_DEV	  txtcolor[1]
#define C_HELP_HDG	  txtcolor[2]+BRIGHT
#define C_HELP_BODY	  txtcolor[3]
#define C_HELP_INSTR	  txtcolor[4]
#define C_HELP_LINK	  txtcolor[5]+BRIGHT
#define C_HELP_CURLINK	  txtcolor[6]+INVERSE
#define C_PROMPT_BKGRD	  txtcolor[7]
#define C_PROMPT_TEXT	  txtcolor[8]
#define C_PROMPT_LO	  txtcolor[9]
#define C_PROMPT_MED	  txtcolor[10]
#define C_PROMPT_HI	  txtcolor[11]+BRIGHT
#define C_PROMPT_INPUT	  txtcolor[12]+INVERSE
#define C_PROMPT_CHOOSE   txtcolor[13]+INVERSE
#define C_CHOICE_CURRENT  txtcolor[14]+INVERSE
#define C_CHOICE_SP_INSTR txtcolor[15]
#define C_CHOICE_SP_KEYIN txtcolor[16]+BRIGHT
#define C_GENERAL_HI	  txtcolor[17]+BRIGHT
#define C_GENERAL_MED	  txtcolor[18]
#define C_GENERAL_LO	  txtcolor[19]
#define C_GENERAL_INPUT   txtcolor[20]+INVERSE
#define C_DVID_BKGRD	  txtcolor[21]
#define C_DVID_HI	  txtcolor[22]+BRIGHT
#define C_DVID_LO	  txtcolor[23]
#define C_STOP_ERR	  txtcolor[24]+BRIGHT
#define C_STOP_INFO	  txtcolor[25]+BRIGHT
#define C_TITLE_LOW	  txtcolor[26]
#define C_AUTHDIV1	  txtcolor[27]+INVERSE
#define C_AUTHDIV2	  txtcolor[28]+INVERSE
#define C_PRIMARY	  txtcolor[29]
#define C_CONTRIB	  txtcolor[30]

/* structure passed to fullscreen_prompts */
struct fullscreenvalues
{
   int type;   /* 'd' for decimal, 's' for string, '*' for comment */
	       /* 'i' for integer, 'y' for yes=1 no=0              */
	       /* 0x100+n for string of length n		   */
	       /* 'l' for one of a list of strings                 */
   union
   {
      double dval;	/* when type 'd' or 'f'  */
      int    ival;	/* when type is 'i'      */
      char   sval[16];	/* when type is 's'      */
      char  *sbuf;	/* when type is 0x100+n  */
      struct {		/* when type is 'l'      */
	 int  val;	/*   selected choice	 */
	 int  vlen;	/*   char len per choice */
	 char **list;	/*   list of values	 */
	 int  llen;	/*   number of values	 */
      } ch;
   } uval;
};

