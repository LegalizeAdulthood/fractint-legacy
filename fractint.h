
/* FRACTINT.H - common structures and values for the FRACTINT routines */

#define MAXPIXELS 2049		/* Maximum pixel count across/down the screen */

struct videoinfo {		/* All we need to know about a Video Adapter */
	char	name[26];	/* Adapter name (IBM EGA, etc)		*/
	char	comment[26];	/* Comments (UNTESTED, etc)		*/
	int	videomodeax;	/* begin with INT 10H, AX=(this)	*/
	int	videomodebx;	/*		...and BX=(this)	*/
	int	videomodecx;	/*		...and CX=(this)	*/
	int	videomodedx;	/*		...and DX=(this)	*/
				/* NOTE:  IF AX==BX==CX==0, SEE BELOW	*/
	int	dotmode;	/* video access method used by asm code	*/
				/*	1 == BIOS 10H, AH=12,13 (SLOW)	*/
				/*	2 == access like EGA/VGA	*/
				/*	3 == access like MCGA		*/
				/*	4 == Tseng-like  SuperVGA*256	*/
				/*	5 == P'dise-like SuperVGA*256	*/
				/*	6 == Vega-like   SuperVGA*256	*/
				/*	7 == "Tweaked" IBM-VGA ...*256	*/
				/*	8 == "Tweaked" SuperVGA ...*256	*/
				/*	9 == Targa Format		*/
				/*	10 = Hercules 			*/
				/*	11 = "disk video" (no screen)	*/
				/*	12 = 8514/A 			*/
				/*	13 = CGA 320x200x4, 640x200x2	*/
				/*	14 = Tandy 1000 		*/
				/*	15 = TRIDENT  SuperVGA*256	*/
				/*	16 = Chips&Tech SuperVGA*256	*/
	int	xdots;		/* number of dots across the screen	*/
	int	ydots;		/* number of dots down the screen	*/
	int	colors;		/* number of colors available		*/
	};

/* NOTE:  if videomode[abc]x == 0, 'setvideomode' assumes it has an IBM (or
	register compatible) adapter and tweaks the registers directly
	to get one of the following modes (based on the value of videomodedx):

		1		704 x 528 x 16
		2		720 x 540 x 16
		3		736 x 552 x 16
		4		752 x 564 x 16
		5		768 x 576 x 16
		6		784 x 588 x 16
		7		800 x 600 x 16
		8		360 x 480 x 16

*/

#define INFO_ID         "Fractal"
#define FRACTAL_INFO   struct fractal_info

struct fractal_info {			/*  for saving data in GIF file     */
	char info_id[8];		/* Unique identifier for info block */
	int iterations;
	int fractal_type;           /* 0=Mandelbrot 1=Julia 2= ... */
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
	int	colors;
	int	future[10];	/* for stuff we haven't thought of yet */
	};

#define MAXVIDEOMODES 100	/* maximum size of the video table */

#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

struct videoinfo videoentry; 

int	maxvideomode;		/* size of the above list */

char *fkeys[] = {		/* Function Key names for display table */
	"F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"SF1","SF2","SF3","SF4","SF5","SF6","SF7","SF8","SF9","SF10",
	"CF1","CF2","CF3","CF4","CF5","CF6","CF7","CF8","CF9","CF10",
	"AF1","AF2","AF3","AF4","AF5","AF6","AF7","AF8","AF9","AF10",
	"Alt-1","Alt-2","Alt-3","Alt-4","Alt-5",
	"Alt-6","Alt-7","Alt-8","Alt-9","Alt-0",
	"Alt-Q","Alt-W","Alt-E","Alt-R","Alt-T",
	"Alt-Y","Alt-U","Alt-I","Alt-O","Alt-P",
	"Alt-A","Alt-S","Alt-D","Alt-F","Alt-G",
	"Alt-H","Alt-J","Alt-K","Alt-L",
	"Alt-Z","Alt-X","Alt-C","Alt-V","Alt-B","Alt-N","Alt-M",
	"Alt--","Alt-=",
	"Ctl-A","Ctl-B","Ctl-C","Ctl-D","Ctl-E","Ctl-F","Ctl-G",
	"Ctl-K","Ctl-L","Ctl-N","Ctl-O","Ctl-P","Ctl-Q","Ctl-R",
	"Ctl-S","Ctl-T","Ctl-U","Ctl-V","Ctl-W","Ctl-X","Ctl-Y","Ctl-Z",
	"F11","F12","SF11","SF12","CF11","CF12","AF11","AF12",
	"Alt-,","Alt-.","Alt-/","Alt-;","Alt-'","Alt-[","Alt-]","Alt-\\",
	"Alt-`","A-Tab","A-Bks","A-Esc",
	"END"};

int kbdkeys[] = {		/* Function Keystrokes for above names */
	1059, 1060, 1061, 1062, 1063, 1064, 1065, 1066, 1067, 1068,
	1084, 1085, 1086, 1087, 1088, 1089, 1090, 1091, 1092, 1093,
	1094, 1095, 1096, 1097, 1098, 1099, 1100, 1101, 1102, 1103,
	1104, 1105, 1106, 1107, 1108, 1109, 1110, 1111, 1112, 1113,
	1120, 1121, 1122, 1123, 1124, 1125, 1126, 1127, 1128, 1129,
	1016, 1017, 1018, 1019, 1020, 1021, 1022, 1023, 1024, 1025,
	1030, 1031, 1032, 1033, 1034, 1035, 1036, 1037, 1038,
	1044, 1045, 1046, 1047, 1048, 1049, 1050,
	1130, 1131,
	   1,    2,    3,    4,    5,    6,    7,
	  11,   12,   14,   15,   16,   17,   18,
	  19,   20,   21,   22,   23,   24,   25,   26,
	1133, 1134, 1135, 1136, 1137, 1138, 1139, 1140,
	1051, 1052, 1053, 1039, 1040, 1026, 1027, 1043,
	1041, 1165, 1014, 1001,
	0};

#else

extern struct videoinfo videoentry;
extern int maxvideomode;

#endif

/* defines of some of the early fractal types
        (hardcoded in various routines)
   remainder defined in calcfract.c/fractals.c (used only there) */

#define NOFRACTAL    -1
#define MANDEL       0 
#define JULIA        1 
#define NEWTBASIN    2 
#define LAMBDA       3 
#define MANDELFP     4 
#define NEWTON       5 
#define JULIAFP      6 
#define PLASMA       7 

#define NUMIFS    32     /* number of ifs functions in ifs array */
#define IFSPARM    7     /* number of ifs parameters */
#define IFS3DPARM 13     /* number of ifs 3D parameters */

/* defines for symmetry */
#define  NOSYM          0
#define  XAXIS_NOPARM  -1
#define  XAXIS          1
#define  YAXIS_NOPARM  -2
#define  YAXIS          2
#define  XYAXIS_NOPARM -3
#define  XYAXIS         3
#define  ORIGIN_NOPARM -4
#define  ORIGIN         4
#define  PI_SYM_NOPARM -5
#define  PI_SYM         5

extern float   far initifs[NUMIFS][IFSPARM];          /* IFS code values */
extern float   far initifs3d[NUMIFS][IFS3DPARM];      /* IFS 3D code values */

struct fractalspecificstuff 
{
   char  *name;				/* name of the fractal */
   char  *param[4];			/* name of the parameters */
   float xmin;				/* default XMIN corner */
   float xmax;				/* default XMAX corner */
   float ymin;				/* default YMIN corner */
   float ymax;				/* default YMAX corner */
   int   isinteger;			/* 1 if integerfractal, 0 otherwise */
   int   tojulia;			/* mandel-to-julia switch */
   int   tomandel;                      /* julia-to-mandel switch */
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
   void (*per_pixel)();		/* once-per-pixel init */
   int (*per_image)();		/* once-per-image setup */
   int (*calctype)();		/* name of main fractal function */
   int orbit_bailout;		/* usual bailout value for orbit calc */
};

extern struct fractalspecificstuff fractalspecific[];


/*	help screens */

#define HELPAUTHORS	1
#define HELPMAIN	2
#define	HELPCYCLING	3
#define HELPMOUSE	4
#define	HELPCMDLINE	5
#define HELPFRACTALS    6
#define	HELPVIDEO	7
#define	HELPMOREINFO	8
#define HELPMENU	98
#define	HELPEXIT	99

#if defined(PUTTHEMHERE)	/* this MUST be defined ONLY in FRACTINT.C */

int helpmode;

#else

extern int helpmode;

#endif

#define DEFAULTFRACTALTYPE	".fra"
#define	ALTERNATEFRACTALTYPE	".gif"

#ifndef sqr
#define sqr(x) ((x)*(x))
#endif

/* 3D stuff - formerly in 3d.h */
#ifndef dot_product
#define dot_product(v1,v2)  ((v1)[0]*(v2)[0]+(v1)[1]*(v2)[1]+(v1)[2]*(v2)[2])  /* TW 7-09-89 */ 
#endif

#define    CMAX    4   /* maximum column (4 x 4 matrix) */
#define    RMAX    4   /* maximum row    (4 x 4 matrix) */
#define    DIM     3   /* number of dimensions */

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

struct lcomplex
{
   long x;
   long y;
};

#define PI 3.14159265358979323846

#define SPHERE    init3d[0]		/* sphere? 1 = yes, 0 = no  */
#define ILLUMINE  (FILLTYPE>4)  /* illumination model       */
  
/* regular 3D */
#define XROT      init3d[1]     /* rotate x-axis 60 degrees */
#define YROT      init3d[2]     /* rotate y-axis 90 degrees */
#define ZROT      init3d[3]     /* rotate x-axis  0 degrees */
#define XSCALE    init3d[4]     /* scale x-axis, 90 percent */
#define YSCALE    init3d[5]     /* scale y-axis, 90 percent */

/* sphere 3D */
#define PHI1      init3d[1]     /* longitude start, 180     */
#define PHI2      init3d[2]     /* longitude end ,   0      */
#define THETA1    init3d[3]	    /* latitude start,-90 degrees */
#define THETA2    init3d[4]	    /* latitude stop,  90 degrees */
#define RADIUS    init3d[5]     /* should be user input */

/* common parameters */
#define ROUGH     init3d[6]     /* scale z-axis, 30 percent */
#define WATERLINE init3d[7]     /* water level              */
#define FILLTYPE  init3d[8]     /* fill type                */
#define ZVIEWER   init3d[9]     /* perspective view point   */
#define XSHIFT    init3d[10]	/* x shift */
#define YSHIFT    init3d[11]	/* y shift */
#define XLIGHT    init3d[12]	/* x light vector coordinate */
#define YLIGHT    init3d[13]	/* y light vector coordinate */
#define ZLIGHT    init3d[14]	/* z light vector coordinate */
#define LIGHTAVG  init3d[15]    /* number of points to average */

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif


/* function prototypes */

#include <math.h>

extern  void   adjust(int, int, int, int, int, int);
extern	void   buzzer(int);
extern  int    calcfract(void);
extern  int    calcmand(void);
extern  int    check_key(void);
extern	int    complex_mult(struct complex, struct complex, struct complex *);
extern	int    complex_div(struct complex, struct complex, struct complex *);
extern	int    complex_power(struct complex, int, struct complex *);
extern  int    cross_product(double [], double [], double []);
extern  void   drawbox(int);
extern	unsigned int emmallocate(unsigned int);
extern	void   emmclearpage(unsigned int, unsigned int);
extern  void   emmdeallocate(unsigned int);
extern  unsigned int emmgetfree(void);
extern  void   emmgetpage(unsigned int, unsigned int);
extern  unsigned char far *emmquery(void);
extern	unsigned char far *farmemalloc(long);
extern	void   farmemfree(unsigned char far *);
extern  int    getakey(void);
extern  int    getcolor(int, int);
extern  int    has_8087(void );
extern	void	helpmessage(unsigned char far *);
extern  void   identity(MATRIX);
extern  int    iplot_orbit(long, long);
extern  int    Juliafp(void);
extern  int    longvmultpersp(LVECTOR, LMATRIX, LVECTOR, LVECTOR, LVECTOR, int);
extern  int    longpersp(LVECTOR, LVECTOR,int);
extern  int    Lambda(void);
extern  int    Lambdasine(void);
extern  int    MainNewton(void);
extern  void   mat_mul(MATRIX, MATRIX, MATRIX);
extern  void   main(int, char *[]);
extern  int    Mandelfp(void);
extern	long   multiply(long, long, int);
extern	long   divide(long, long, int);
extern  int    Newton(void);
extern  int    perspective(double *v);
extern  int    plasma(void);
extern  int    plot_orbit(double, double);
extern	void	cdecl	Print_Screen(void);	/* MDS 7/1/89 */
extern  void   putcolor(int, int, int);
extern  void   scale(double, double, double, MATRIX);
extern  int    scrub_orbit(void);
extern  int    set_Plasma_palette(void);
extern  void   setvideomode(int, int, int, int);
extern	int    Sierpinski(void);
extern  int    solidguess(void);
extern  void   spindac(int, int);
extern  void   subDivide(int, int, int, int);
extern  void   symPIplot(int, int, int);
extern  void   symPIplot2J(int, int, int);
extern  void   symPIplot4J(int, int, int);
extern  void   symplot2(int, int, int);
extern  void   symplot2J(int, int, int);
extern  void   symplot2Y(int, int, int);
extern  void   symplot4(int, int, int);
extern  int    test(void);
extern  void   trans(double, double, double, MATRIX);
extern  int    vmult(VECTOR,MATRIX,VECTOR);
extern  void   xrot(double, MATRIX);
extern  void   yrot(double, MATRIX);
extern  void   zrot(double, MATRIX);

