/******************************************************************************/
/* This file contains a 3D replacement for the out_line function called			*/
/* by the decoder. The purpose is to apply various 3D transformations			*/
/* before displaying points. Called once per line of the input file.				*/
/*																										*/
/* This module is linked as an overlay, use ENTER_OVLY and EXIT_OVLY.			*/
/*																										*/
/* Original Author Tim Wegner, with extensive help from Marc Reinig.				*/
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <dos.h>
#include "fractint.h"

struct point
{
	int x;
	int y;
	int color;
};

/* routines in this module	*/

void line3d_overlay(void);
int  line3d();

static void _fastcall vdraw_line(double *,double *,int);
static void (_fastcall *fillplot)(int,int,int);
static void (_fastcall *normalplot)(int,int,int);
static void _fastcall putminmax(int x,int y,int color);
static void _fastcall clipcolor(int x,int y,int color);
static void _fastcall T_clipcolor(int x,int y,int color);
static void _fastcall interpcolor(int x,int y,int color);
static void corners(),draw_light_box();
static void _fastcall putatriangle(struct point pt1,struct point pt2,
					struct point pt3,int color);
static int _fastcall offscreen(struct point pt);
static int  H_R(),R_H();
static int  startdisk1(char *File_Name2, FILE *Source, int overlay);
static int  set_pixel_buff();
static void line3d_cleanup();
static int _fastcall targa_color(int x,int y,int color);
void (_fastcall *standardplot)(int,int,int);
int targa_validate(char *File_Name);

static	int line_length1;
static	int T_header_24 = 18; /* Size of current Targa-24 header */
extern	unsigned char dacbox[256][3];
MATRIX m;				/* transformation matrix */
extern void mult_vec_c(VECTOR);
extern void mult_vec_iit(VECTOR);
void (*mult_vec)(VECTOR) = mult_vec_c;
extern int iit; 				/* iit flag */
static	FILE *File_Ptr1 = NULL;
int Ambient;
static unsigned int IAmbient;
int RANDOMIZE;
static int rand_factor;
int haze;
static int HAZE_MULT;
int Real_V = 0; /* mrr Actual value of V for fillytpe>4 monochrome images */

char light_name[80] = "fract001";
int Targa_Overlay, error;
extern int Targa_Out;
char targa_temp[14] = "fractemp.tga";
static void File_Error (char *File_Name1, int ERROR);
static unsigned char T24=24;
static unsigned char T32=32;
static unsigned char upr_lwr[4];
static int T_Safe; /* Original Targa Image successfully copied to targa_temp */
int P = 250; /* Perspective dist used when viewing light vector */
static void draw_rect (VECTOR V0, VECTOR V1, VECTOR V2, VECTOR V3, int color,
							int rect);
static VECTOR light_direction;

unsigned char back_color[3];
static unsigned char Real_Color; /* Actual color of cur pixel */
extern int dotmode; /* video access method, 11 if really disk video */
extern int calc_status;
extern long calctime;
extern int got_status,currow;


static int RO, CO, CO_MAX; /*  For use in Acrospin support */
static char far acro_s1[] = {"Set Layer 1\nSet Color 2\n"
		"EndpointList X Y Z Name\n"};
static char far acro_s2[] = {"LineList From To\n"};
static char far s3[] = {"{ Created by FRACTINT Ver. "};
static char far s3a[] = {" }\n\n"};
static char banner[] ="%Fs%#4.2f%Fs";
char ray_name[80] = "fract001";

char preview = 0;
char showbox = 0;
static int localpreviewfactor;
int previewfactor = 20;
int xadjust = 0;
int yadjust = 0;
int xxadjust;
int yyadjust;
int xshift;
int yshift;
extern int overflow;
extern int filetype;
extern char usr_floatflag;
extern int xdots, ydots, colors, sxoffs, syoffs;
extern int debugflag;
extern int extraseg;
extern unsigned height;
extern int rowcount;		/* in general.asm */
extern int init3d[];		/* 3D arguments (FRACTINT.C) */
extern long far *lx0;
extern int transparent[2];	/* transparency min/max */
extern int pot16bit;
extern int filecolors;
extern void (*outln_cleanup)();

static int zcoord = 256;
static double aspect;			/* aspect ratio */
static int evenoddrow;

static float far *sinthetaarray;	/* all sine		thetas go here  */
static float far *costhetaarray;	/* all cosine thetas go here */

static double rXrscale; 		/* precalculation factor */

static int persp;	/* flag for indicating perspective transformations */
int bad_value = -10000; /* set bad values to this */
int bad_check = -3000;	/* check values against this to determine if good */

/* this array remembers the previous line */
struct point far *lastrow;

static struct point p1,p2,p3;

struct f_point
{
	float x;
	float y;
	float color;
}
far *f_lastrow;


int RAY = 0; /* Flag to generate Ray trace compatible files in 3d */
int BRIEF = 0; /* 1 = short ray trace files */
extern int release; /* Current release level of Fractint */
static int _fastcall RAY_Header(void);
static void _fastcall triangle_bounds(float pt_t[3][3]);
static int _fastcall out_triangle(struct f_point pt1, struct f_point pt2,
			struct f_point pt3, int c1, int c2, int c3);
static float min_xyz[3], max_xyz[3]; /* For Raytrace output */
static int _fastcall start_object(void);
static int _fastcall end_object(int triout);
extern unsigned numcolors; /* number of colors in original GIF */
static long num_tris; /* number of triangles output to ray trace file */

/* array of min and max x values used in triangle fill */
struct minmax
{
	int minx;
	int maxx;
}
far *minmax_x;

VECTOR view;	/* position of observer for perspective */
VECTOR cross;
VECTOR tmpcross;

struct point oldlast =
{
	0, 0, 0
};	/* old pixels */

void line3d_overlay() {
}	/* for restore_active_ovly */

int line3d(unsigned char pixels[], unsigned linelen)
{
	int tout; /*  triangle has been sent to ray trace file */
	int RND;
	float f_water; /* transformed WATERLINE for ray trace files */

	/* these values come from FRACTINT.C */

	/* These variables must preserve their values across calls */
	static float	deltaphi;  /* increment of latitude, longitude */
	static double rscale;			/* surface roughness factor */
	static long xcenter,ycenter;			/* circle center */
	double r0;
	int xcenter0,ycenter0; /* Unfudged versions */

	static double sclx,scly,sclz;		/* scale factors */
	static double R;				/* radius values */
	static double Rfactor;			/* for intermediate calculation */
	MATRIX lightm;				/* m w/no trans, keeps obj. on screen */
	static LMATRIX lm;				/* "" */
	static LVECTOR lview;			/* for perspective views */
	static double zcutoff;			/* perspective backside cutoff value */
	static float twocosdeltaphi;
	static float cosphi,sinphi;			/* precalculated sin/cos of longitude */
	static float oldcosphi1,oldsinphi1;
	static float oldcosphi2,oldsinphi2;
	double r;					/* sphere radius */
	double xval,yval,zval;			/* rotation values */
	float costheta,sintheta;			/* precalculated sin/cos of latitude */
	float twocosdeltatheta;

	int next; /* used by preview and grid */
	int col;					/* current column (original GIF) */
	struct point cur;				/* current pixels */
	struct point old;				/* old pixels */

	static struct point bad;			/* out of range value */

	struct f_point f_cur;
	struct f_point f_old;
	static struct f_point f_bad;			/* out of range value */

	static unsigned char far *fraction;/* float version of pixels array */

	VECTOR v;					/* double vector */
	VECTOR v1,v2;
	VECTOR crossavg;
	char crossnotinit;				/* flag for crossavg init indication */

	double v_length;
	VECTOR origin, direct, tmp;
	LVECTOR lv;					/* long equivalent of v */
	LVECTOR lv0; 				/* long equivalent of v */

	/* corners of transformed xdotx by ydots x colors box */
	double xmin, ymin, zmin, xmax, ymax, zmax;
	int i,j;
	int lastdot;

	long fudge;

	ENTER_OVLY(OVLY_LINE3D);

	fudge = 1L<<16;


	if(transparent[0] || transparent[1])
		plot = normalplot = T_clipcolor;	/*  Use transparent plot function */
	else			/* Use the usual FRACTINT plot function with clipping */
		plot = normalplot = clipcolor;

	currow = rowcount; /* use separate variable to allow for pot16bit files */
	if (pot16bit)
		currow >>= 1;

	/************************************************************************/
	/* This IF clause is executed ONCE per image. All precalculations are	*/
	/* done here, with out any special concern about speed. DANGER -			*/
	/* communication with the rest of the program is generally via static	*/
	/* or global variables.																	*/
	/************************************************************************/
	if(rowcount++ == 0)
	{
		long check_extra;
		float theta,theta1,theta2;	/* current,start,stop latitude */
		float phi1,phi2;			/* current start,stop longitude */
		float	deltatheta;		/* increment of latitude */
		outln_cleanup = line3d_cleanup;

		/* plot_setup();*/

		calctime = evenoddrow = 0;
		/* mark as in-progress, and enable <tab> timer display */
		calc_status = 1;

		IAmbient = (unsigned int) (255 * (float)(100 - Ambient) / 100.0);
		if (IAmbient < 1)		IAmbient = 1;

		tout = 0;
		num_tris = 0;

		/* Open file for RAY trace output and write header */
		if (RAY)
		{
			RAY_Header();
			xxadjust = yyadjust = 0; /* Disable shifting in ray tracing */
			xshift = yshift = 0;
		}

		CO_MAX=CO=RO=0;

		upr_lwr[0] = xdots & 0xff;
		upr_lwr[1] = xdots >> 8;
		upr_lwr[2] = ydots &  0xff;
		upr_lwr[3] = ydots >> 8;
		line_length1 = 3 * xdots;		/*  line length @ 3 bytes per pixel  */
		error = 0;
		T_Safe = 0; /* Not safe yet to mess with the source image */

		if (Targa_Out)
		{
			if (Targa_Overlay)	
			{		
				/* Make sure target file is a supportable Targa File */	
				if(targa_validate (light_name))		
					return(-1);
			}		
			else
			{
				check_writefile(light_name,".tga");
				if (startdisk1(light_name, NULL, 0))	/* Open new file */
					return(-1);
			}
		}

		rand_factor = 14 - RANDOMIZE;

		zcoord = filecolors;

		crossavg[0] = 0;
		crossavg[1] = 0;
		crossavg[2] = 0;

		/*********************************************************************/
		/*  Memory allocation - assumptions - a 64K segment starting at		*/
		/*  extraseg has been grabbed. It may have other purposes elsewhere, */
		/*  but it is assumed that it is totally free for use here. Our		*/
		/*  strategy is to assign all the far pointers needed here to various*/
		/*  spots in the extra segment, depending on the pixel dimensions of */
		/*  the video mode, and check whether we have run out. There is		*/
		/*  basically one case where the extra segment is not big enough		*/
		/*  -- SPHERE mode with a fill type that uses putatriangle() (array	*/
		/*  minmax_x) at the largest legal resolution of 2048x2048 or			*/
		/*  thereabouts. In that case we use farmemalloc to grab	memory		*/
		/*  for minmax_x. This memory is never freed.								*/
		/*********************************************************************/

		/* lastrow stores the previous row of the original GIF image for
			the purpose of filling in gaps with triangle procedure */
		lastrow = MK_FP(extraseg,0);

		check_extra = sizeof(*lastrow)*xdots;
		if(SPHERE)
		{
			sinthetaarray = (float far *)(lastrow+xdots);
			check_extra += sizeof(*sinthetaarray)*xdots;

			costhetaarray = (float far *)(sinthetaarray+xdots);
			check_extra += sizeof(*costhetaarray)*xdots;

			f_lastrow = (struct f_point far *)(costhetaarray+xdots);
		}
		else
			f_lastrow = (struct f_point far *)(lastrow+xdots);
		check_extra += sizeof(*f_lastrow)*(xdots);

		if(pot16bit)
		{
			fraction = (unsigned char far *)(f_lastrow+xdots);
			check_extra += sizeof(*fraction)*xdots;
		}
		minmax_x = (struct minmax *)NULL;

		/* these fill types call putatriangle which uses minmax_x */
		if( FILLTYPE == 2 || FILLTYPE == 3 || FILLTYPE == 5 || FILLTYPE == 6)
		{
			/* end of arrays if we use extra segement */
			check_extra += sizeof(struct minmax)*ydots;
			if(check_extra > (1L<<16)) /* run out of extra segment? */
			{
				static struct minmax far *got_mem = NULL;
				/* not using extra segment so decrement check_extra */
				check_extra -= sizeof(struct minmax)*ydots;
				if(got_mem == NULL)
					got_mem = (struct minmax far *)(farmemalloc(2048L *
				sizeof(struct minmax)));

				if(got_mem)
					minmax_x = got_mem;
				else
				{
					EXIT_OVLY;
					return(-1);
				}
			}
			else /* ok to use extra segment */
			{
				if(pot16bit)
					minmax_x = (struct minmax far *)(fraction+xdots);
				else
					minmax_x = (struct minmax far *)(f_lastrow+xdots);

			}
		}

		if(debugflag == 2222 || check_extra > (1L<<16))
		{
			char tmpmsg[70];
			static char far extramsg[] = {" of extra segment"};
			sprintf(tmpmsg,"used %ld%Fs", check_extra, extramsg);
			stopmsg(4,tmpmsg);
		}

		/* get scale factors */
		sclx =	XSCALE/100.0;
		scly =	YSCALE/100.0;
		if (ROUGH)
			sclz = -ROUGH/100.0;
		else
			rscale = sclz = -0.0001; /* if rough=0 make it very flat but plot something */

		/* aspect ratio calculation - assume screen is 4 x 3 */
		aspect = (double)xdots*.75/(double)ydots;

		if(SPHERE==FALSE)  /* skip this slow stuff in sphere case */
		{
		/*********************************************************************/
		/* What is done here is to create a single matrix, m, which has		*/
		/* scale, rotation, and shift all combined. This allows us to use		*/
		/* a single matrix to transform any point. Additionally, we create  	*/
		/* two perspective vectors.														*/
		/*																							*/
		/* Start with a unit matrix. Add scale and rotation. Then calculate 	*/
		/* the perspective vectors. Finally add enough translation to center	*/
		/* the final image plus whatever shift the user has set.					*/
		/*********************************************************************/

			/* start with identity */
			identity (m);
			identity (lightm);

			/* translate so origin is in center of box, so that when we rotate */
			/* it, we do so through the center */
			trans ( (double)xdots/(-2.0),(double)ydots/(-2.0),
					(double)zcoord/(-2.0),m);
			trans ( (double)xdots/(-2.0),(double)ydots/(-2.0),
					(double)zcoord/(-2.0),lightm);

			/* apply scale factors */
			scale(sclx,scly,sclz,m);
			scale(sclx,scly,sclz,lightm);

			/* rotation values - converting from degrees to radians */
			xval = XROT / 57.29577;
			yval = YROT / 57.29577;
			zval = ZROT / 57.29577;

			if (RAY)	{xval = yval = zval = 0;}

			xrot (xval,m);
			xrot (xval,lightm);
			yrot (yval,m);
			yrot (yval,lightm);
			zrot (zval,m);
			zrot (zval,lightm);

			/* Find values of translation that make all x,y,z negative */
			/* m current matrix */
			/* 0 means don't show box */
			/* returns minimum and maximum values of x,y,z in fractal */
			corners(m,0,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
		}

		/* perspective 3D vector - lview[2] == 0 means no perspective */

		/* set perspective flag */
		persp = 0;
		if (ZVIEWER != 0)
		{
			persp = 1;
			if(ZVIEWER < 80) /* force float */
				usr_floatflag |= 2; /* turn on second bit */
		}

		/* set up view vector, and put viewer in center of screen */
		lview[0] = xdots >> 1;
		lview[1] = ydots >> 1;

		/* z value of user's eye - should be more negative than extreme
								negative part of image */
		if(SPHERE) /* sphere case */
			lview[2] = -(long)((double)ydots*(double)ZVIEWER/100.0);
		else	/* non-sphere case */
			lview[2] = (long)((zmin-zmax)*(double)ZVIEWER/100.0);

		view[0] = lview[0];
		view[1] = lview[1];
		view[2] = lview[2];
		lview[0] = lview[0] << 16;
		lview[1] = lview[1] << 16;
		lview[2] = lview[2] << 16;

		if(SPHERE==FALSE) /* sphere skips this */
		{
			/* translate back exactly amount we translated earlier plus enough
				to center image so maximum values are non-positive */
			trans(((double)xdots-xmax-xmin)/2,((double)ydots-ymax-ymin) / 2,
				-zmax,m);

			/* Keep the box centered and on screen regardless of shifts */
			trans(((double)xdots-xmax-xmin)/2,((double)ydots-ymax-ymin)/2,
				-zmax,lightm);

			trans((double)(xshift),(double)(-yshift),0.0,m);

			/* matrix m now contains ALL those transforms composed together !!
				convert m to long integers shifted 16 bits */
			for (i = 0; i < 4; i++)
				for (j = 0; j < 4; j++)
					lm[i][j] = m[i][j] * 65536.0;

		}
		else /* sphere stuff goes here */
		{
			/* Sphere is on side - north pole on right. Top is -90 degrees
				latitude; bottom 90 degrees */

			/* Map X to this LATITUDE range */
			theta1 = THETA1*PI/180.0;
			theta2 = THETA2*PI/180.0;

			/* Map Y to this LONGITUDE range */
			phi1	= PHI1*PI/180.0;
			phi2	= PHI2*PI/180.0;

			theta = theta1;

		/*********************************************************************/
		/* Thanks to Hugh Bray for the following idea: when calculating		*/
		/* a table of evenly spaced sines or cosines, only a few initial		*/
		/* values need be calculated, and the remaining values can be			*/
		/* gotten from a derivative of the sine/cosine angle sum formula		*/
		/* at the cost of one multiplication and one addition per value!		*/
		/*																							*/
		/* This idea is applied once here to get a complete table for			*/
		/* latitude, and near the bottom of this routine to incrementally		*/
		/* calculate longitude.																*/
		/*																							*/
		/* Precalculate 2*cos(deltaangle), sin(start) and sin(start+delta).	*/
		/* Then apply recursively:															*/
		/*	sin(angle+2*delta) = sin(angle+delta) * 2cosdelta - sin(angle)		*/
		/*																							*/
		/* Similarly for cosine. Neat! 													*/
		/*********************************************************************/

			deltatheta = (float)(theta2 - theta1)/(float)linelen;

			/* initial sin,cos theta */
			sinthetaarray[0] = sin((double)theta);
			costhetaarray[0] = cos((double)theta);
			sinthetaarray[1] = sin((double)(theta + deltatheta));
			costhetaarray[1] = cos((double)(theta + deltatheta));

			/* sin,cos delta theta */
			twocosdeltatheta = 2.0*cos((double)deltatheta);

			/* build table of other sin,cos with trig identity */
			for(i=2;i<linelen;i++)
			{
				sinthetaarray[i] = sinthetaarray[i-1]*twocosdeltatheta-
					sinthetaarray[i-2];
				costhetaarray[i] = costhetaarray[i-1]*twocosdeltatheta-
					costhetaarray[i-2];
			}

			/* now phi - these calculated as we go - get started here */
			deltaphi	= (float)(phi2	- phi1  )/(float)height;

			/* initial sin,cos phi */

			sinphi = oldsinphi1 = sin((double)phi1);
			cosphi = oldcosphi1 = cos((double)phi1);
			oldsinphi2 = sin((double)(phi1+deltaphi));
			oldcosphi2 = cos((double)(phi1+deltaphi));

			/* sin,cos delta phi */
			twocosdeltaphi = 2*cos((double)deltaphi);

			xcenter0 = xcenter = xdots/2 + xshift;
			ycenter0 = ycenter = ydots/2 - yshift;

			/* affects how rough planet terrain is */
			if (ROUGH)
				rscale = .3*ROUGH/100.0;

			/* radius of planet */
			R = (double)(ydots)/2;

			/* precalculate factor */
			rXrscale = R*rscale;

			sclz = sclx = scly = RADIUS/100.0; /* Need x,y,z for RAY */

			/* adjust x scale factor for aspect */
			sclx *= aspect;

			/* precalculation factor used in sphere calc */
			Rfactor = rscale*R/(double)zcoord;

			if(persp) /* precalculate fudge factor */
			{
				double radius;
				double zview;
				double angle;

				xcenter  = xcenter << 16;
				ycenter  = ycenter << 16;

				Rfactor *= 65536.0;
				R			*= 65536.0;

				/* calculate z cutoff factor
					attempt to prevent out-of-view surfaces from being written */
				zview = -(long)((double)ydots*(double)ZVIEWER/100.0);
				radius = (double)(ydots)/2;
				angle = atan(-radius/(zview+radius));
				zcutoff = -radius - sin(angle)*radius;
				zcutoff *= 1.1; /* for safety */
				zcutoff *= 65536;
			}
		}

		/* set fill plot function */
		if(FILLTYPE != 3 && debugflag != 2762)
			fillplot = interpcolor;
		else
		{
			fillplot = clipcolor;

			if(transparent[0] || transparent[1])
				/*	If transparent colors are set */
				fillplot = T_clipcolor;  /*  Use the transparent plot function  */
		}

		/* Both Sphere and Normal 3D */
		direct[0] = light_direction[0] = XLIGHT;
		direct[1] = light_direction[1] = -YLIGHT;
		direct[2] = light_direction[2] = ZLIGHT;

		/* Needed because sclz = -ROUGH/100 and light_direction is transformed
			in FILLTYPE 6 but not in 5. */
		if (FILLTYPE == 5)
			direct[2] = light_direction[2] = -ZLIGHT;

		if(FILLTYPE==6) /* transform light direction */
		{
			/* Think of light direction  as a vector with tail at (0,0,0) and
				head at (light_direction). We apply the transformation to
				BOTH head and tail and take the difference */

			v[0] = 0.0;
			v[1] = 0.0;
			v[2] = 0.0;
			vmult(v,m,v);
			vmult(light_direction,m,light_direction);

			for (i=0;i<3;i++)		light_direction[i] -= v[i];
		}
		normalize_vector(light_direction);

		if(preview && showbox)
		{
			normalize_vector(direct);

			/* move light vector to be more clear with grey scale maps */
			origin[0] = (3 * xdots) / 16;
			origin[1] = (3 * ydots) / 4;
			if (FILLTYPE == 6)
				origin[1] = (11 * ydots) / 16;

			origin[2] = 0.0;

			v_length = min (xdots, ydots) / 2;
			if (persp && ZVIEWER <= P)
				v_length *= (long)(P + 600) /((long)(ZVIEWER+600) * 2);

			/* Set direct[] to point from origin[] in direction of
				untransformed light_direction (direct[]). */
			for (i=0;i<3;i++)
				direct[i] = origin[i] + direct[i] * v_length;

			/* center light box */
			for (i=0;i<2;i++)
			{
				tmp[i] = (direct[i] - origin[i]) / 2;
				origin[i] -= tmp[i];
				direct[i] -= tmp[i];
			}

			/* Draw light source vector and box containing it, draw_light_box
				will transform them if necessary. */
			draw_light_box (origin,direct,lightm);
			/* draw box around original field of view to help visualize effect
				of rotations 1 means show box - xmin etc. do nothing here */
			if (!SPHERE)
				corners(m,1,&xmin,&ymin,&zmin,&xmax,&ymax,&zmax);
		}

		/* bad has values caught by clipping */
		f_bad.x			= bad.x			= bad_value;
		f_bad.y			= bad.y			= bad_value;
		f_bad.color = bad.color = bad_value;
		for(i=0;i<linelen;i++)
		{
			lastrow[i]	= bad;
			f_lastrow[i] = f_bad;
		}
		got_status = 3;
		if(iit>0)
		{
			load_mat(m); /* load matrix into iit registers */
			mult_vec = mult_vec_iit;
		}
		else
			mult_vec = mult_vec_c;
	} /* end of once-per-image intializations */

	crossnotinit = 1;
	col = 0;

	CO = 0;

	/* make sure these pixel coordinates are out of range */
	old		= bad;
	f_old = f_bad;

	/* copies pixels buffer to float type fraction buffer for fill purposes */
	if(pot16bit)
		if (set_pixel_buff(pixels,fraction,linelen))
		{
			EXIT_OVLY;
			return(0);
		}

  /*************************************************************************/
  /* This section of code allows the operation of a preview mode when the	*/
  /* preview flag is set. Enabled, it allows the drawing of only the first */
  /* line of the source image, then every 10th line, until and including	*/
  /* the last line. For the undrawn lines, only necessary calculations are */
  /* made. As a bonus, in non-sphere mode a box is drawn to help visualize */
  /* the effects of 3D transformations. Thanks to Marc Reinig for this idea*/
  /* and code -- BTW, Marc did NOT put the goto in, but WE did, to avoid	*/
  /* copying code here, and to avoid a HUGE "if-then" construct. Besides,	*/
  /* we have ALREADY sinned, so why not sin some more?							*/
  /*************************************************************************/


	lastdot = min(xdots-1, linelen-1);
	if (FILLTYPE >= 5)
		if (haze && Targa_Out)
		{
			HAZE_MULT = haze * (
				(float)((long)(ydots - 1 - currow) *
				(long)(ydots - 1 - currow)) /
				(float)((long)(ydots - 1) * (long)(ydots - 1)));
			HAZE_MULT = 100 - HAZE_MULT;
		}

	if (previewfactor >= ydots || previewfactor > lastdot)
		previewfactor = min ( ydots - 1, lastdot);

	localpreviewfactor = ydots/previewfactor;

	tout = 0;
	/* Insure last line is drawn in preview and filltypes <0  */
	if ((RAY || preview || FILLTYPE < 0) && (currow != ydots-1) &&
		(currow % localpreviewfactor) && /* Draw mod preview lines */
		!(!RAY && (FILLTYPE > 4) && (currow == 1)))
			/* Get init geometry in lightsource modes */
			goto reallythebottom; /* skip over most of the line3d calcs */
	if (dotmode == 11)
	{
		char s[40];
		sprintf(s,"mapping to 3d, reading line %d", currow);
		dvid_status(1,s);
	}

	if(!col && RAY && currow != 0)		start_object();

	if(FILLTYPE==0 && !SPHERE && !pot16bit && !RAY && debugflag != 2224 )
		/* This while loop contains just a limited non-sphere case for speed */
		/* Other while loop still has the old logic. Use debugflag to compare*/
		while(col < linelen)
		{
			Real_Color = cur.color = pixels[col];
			if (cur.color > 0 && cur.color < WATERLINE)
				Real_Color = cur.color = WATERLINE;  /* "lake" */
			if(!usr_floatflag)
			{
				lv0[0] = 0; /* don't save vector before perspective */

				/* use 32-bit multiply math to snap this out */
				lv[0] = col;
				lv[0] = lv[0] << 16;
				lv[1] = currow;
				lv[1] = lv[1] << 16;
				lv[2] = cur.color;
				lv[2] = lv[2] << 16;

				if(longvmultpersp(lv,lm,lv0,lv,lview,16) == -1)
				{
					cur	= bad;
					goto loopbottom;
				}
				cur.x = ((lv[0]/* +32768L */) >> 16) + xxadjust;
				cur.y = ((lv[1]/* +32768L */) >> 16) + yyadjust;
			}
			/* do in float if integer math overflowed */
			if(usr_floatflag || overflow)
			{
				/* slow float version for comparison */
				v[0] = col;
				v[1] = currow;
				v[2] = cur.color;
				mult_vec(v); /* matrix*vector routine */
				if(persp)
					perspective(v);
				cur.x = v[0] + xxadjust /* + .5 */;
				cur.y = v[1] + yyadjust /* + .5 */;
			}
			(*plot)(cur.x,cur.y,cur.color);
			col++;
		}  /*  End of while statement for plotting line  */
	else

		/* PROCESS ROW LOOP BEGINS HERE */
		while(col < linelen)
		{
			if ((RAY || preview || FILLTYPE < 0) &&
				(col != lastdot) &&		/* if this is not the last col */
				/* if not the 1st or mod factor col*/
				(col % (int)(aspect * localpreviewfactor)) &&
				(!(!RAY && FILLTYPE > 4 && col == 1)))
					goto loopbottom;

			f_cur.color = Real_Color = cur.color = pixels[col];

			if (RAY || preview || FILLTYPE < 0)
			{
				next = col + aspect * localpreviewfactor;
				if (next == col)		next = col + 1;
			}
			else
				next = col + 1;
			if (next >= lastdot)
				next = lastdot;

			if (cur.color > 0 && cur.color < WATERLINE)
				f_cur.color = Real_Color = cur.color = WATERLINE;		/* "lake" */
			else if(pot16bit)
				f_cur.color += ((float)fraction[col])/(float)(1<<8);

			if(SPHERE) /* sphere case */
			{
				sintheta = sinthetaarray[col];
				costheta = costhetaarray[col];

				if(sinphi < 0 && !(RAY || FILLTYPE < 0))
				{
					cur	= bad;
					f_cur = f_bad;
					goto loopbottom;	/* another goto ! */
				}
			/******************************************************************/
			/* KEEP THIS FOR DOCS - original formula --								*/
			/* if(rscale < 0.0) 																*/
			/*			r = 1.0+((double)cur.color/(double)zcoord)*rscale;			*/
			/* else																				*/
			/*			r = 1.0-rscale+((double)cur.color/(double)zcoord)*rscale;*/
			/* R = (double)ydots/2;															*/
			/* r = r*R; 																		*/
			/* cur.x = xdots/2 + sclx*r*sintheta*aspect + xup ;					*/
			/* cur.y = ydots/2 + scly*r*costheta*cosphi - yup ;					*/
			/******************************************************************/

/* mrr 	
			Why not do the calc fudged or not to avoid having to unfudge it
			later on?
*/

				if(rscale < 0.0)
					r = R + Rfactor*(double)f_cur.color*costheta;
				else if(rscale > 0.0)
					r = R -rXrscale + Rfactor*(double)f_cur.color*costheta;
				else
					r = R;
				if(persp || RAY) /* Allow Ray trace to go through so display is ok */
				{	/* mrr how do lv[] and cur and f_cur all relate */
					/* NOTE: fudge was pre-calculated above in r and R */
					/* (almost) guarantee negative */
					lv[2] = -R - r*costheta*sinphi;	/* z */
					if((lv[2] > zcutoff) && !FILLTYPE < 0)
					{
						cur = bad;
						f_cur = f_bad;
						goto loopbottom;	/* another goto ! */
					}
					lv[0] = xcenter + sintheta*sclx*r;	/* x */
					lv[1] = ycenter + costheta*cosphi*scly*r;		/* y */

					if((FILLTYPE >= 5) || RAY)
					{  /* calculate illumination normal before persp */

						r0		= r/65536;
						f_cur.x		= xcenter0 + sintheta*sclx*r0;
						f_cur.y		= ycenter0 + costheta*cosphi*scly*r0;
						f_cur.color = -r0*costheta*sinphi;
					}
					if(!(usr_floatflag || RAY))
					{
						if(longpersp(lv,lview,16) == -1)
						{
							cur = bad;
							f_cur = f_bad;
							goto loopbottom;	/* another goto ! */
						}
						cur.x = ((lv[0]+32768L) >> 16) + xxadjust;
						cur.y = ((lv[1]+32768L) >> 16) + yyadjust;
					}
					if(usr_floatflag || overflow || RAY)
					{
						v[0] = lv[0];
						v[1] = lv[1];
						v[2] = lv[2];
						v[0] /= fudge;
						v[1] /= fudge;
						v[2] /= fudge;
						perspective(v);
						cur.x = v[0]+.5 + xxadjust;
						cur.y = v[1]+.5 + yyadjust;
					}
				}
				else if (!(persp && RAY)) /* mrr Not sure how this and 3rd if
													above relate */
				{	/* mrr Why the xx- and yyadjust here and not above? */
					cur.x = f_cur.x = xcenter + sintheta*sclx*r + xxadjust;
					cur.y = f_cur.y = ycenter + costheta*cosphi*scly*r + yyadjust;
					if(FILLTYPE >= 5 || RAY) /* mrr why do we do this for filltype>5? */
						f_cur.color = -r * costheta * sinphi * sclz;
					v[0]=v[1]=v[2]=0;	/* MRR Why do we do this? */
				}
			}
			else /* non-sphere 3D */
			{
				if(!usr_floatflag && !RAY)
				{
					if(FILLTYPE >= 5) /* flag to save vector before perspective */
						lv0[0] = 1;		/* in longvmultpersp calculation */
					else
						lv0[0] = 0;

					/* use 32-bit multiply math to snap this out */
					lv[0] = col;
					lv[0] = lv[0] << 16;
					lv[1] = currow;
					lv[1] = lv[1] << 16;
					if(filetype || pot16bit) /* don't truncate fractional part */
						lv[2] = f_cur.color*65536.0;
					else	/* there IS no fractaional part here! */
					{
						lv[2] = f_cur.color;
						lv[2] = lv[2] << 16;
					}

					if(longvmultpersp(lv,lm,lv0,lv,lview,16) == -1)
					{
						cur	= bad;
						f_cur = f_bad;
						goto loopbottom;
					}

					cur.x = ((lv[0]+32768L) >> 16) + xxadjust;
					cur.y = ((lv[1]+32768L) >> 16) + yyadjust;
					if(FILLTYPE >= 5 && !overflow)
					{
						f_cur.x		= lv0[0];
						f_cur.x			/= 65536.0;
						f_cur.y		= lv0[1];
						f_cur.y			/= 65536.0;
						f_cur.color  = lv0[2];
						f_cur.color /= 65536.0;
					}
				}

				if(usr_floatflag || overflow || RAY)
				/* do in float if integer math overflowed or doing Ray trace */
				{
					/* slow float version for comparison */
					v[0] = col;
					v[1] = currow;
					v[2] = f_cur.color; /* Actually the z value */

					mult_vec(v); /* matrix*vector routine */

					if (FILLTYPE > 4 || RAY)
					{
						f_cur.x	= v[0];
						f_cur.y	= v[1];
						f_cur.color = v[2];

						if(RAY == 6)
						{
							f_cur.x = f_cur.x * (2.0 / xdots) - 1;
							f_cur.y = f_cur.y * (2.0 / ydots) - 1;
							f_cur.color = -f_cur.color * (2.0 / numcolors) - 1;
						}
					}

					if(persp && !RAY)
						perspective(v);
					cur.x = v[0] + xxadjust + .5;
					cur.y = v[1] + yyadjust + .5;

					v[0] = 0;
					v[1] = 0;
					v[2] = WATERLINE;
					mult_vec(v);
					f_water = v[2];
				}
			}

			if (RANDOMIZE)
				if (cur.color > WATERLINE)
				{
					RND = rand() >> 8;		/* 7-bit number */
					RND = RND * RND >> rand_factor;	/* n-bit number */

					if (rand() & 1)
						RND = -RND; /* Make +/- n-bit number */

					if ((int)(cur.color) + RND >= colors)
						cur.color = colors-2;
					else if ((int)(cur.color) + RND <= WATERLINE)
						cur.color = WATERLINE + 1;
					else
						cur.color = cur.color + RND;
					Real_Color = cur.color;
				}

			if (RAY)
			{
/* 

if (RAY) use float routines. Output shapes prior to perspective, 
then put in perspective, and plot as FILLTYPE -1, also plotting the 
diagonal. 

*/



				if (col && currow &&
					old.x > bad_check &&
					old.x < (xdots - bad_check) &&
					lastrow[col].x > bad_check &&
					lastrow[col].y > bad_check &&
					lastrow[col].x < (xdots - bad_check) &&
					lastrow[col].y < (ydots - bad_check))
				{
					/* Get rid of all the triangles in the plane
						at the base of the object */

					if (f_cur.color == f_water &&
						f_lastrow[col].color == f_water &&
						f_lastrow[next].color == f_water)
							goto loopbottom;

					if (RAY != 6) /* Output the vertex info */
						out_triangle(f_cur, f_old, f_lastrow[col],
							cur.color, old.color, lastrow[col].color);

					tout = 1;

					draw_line (old.x, old.y, cur.x, cur.y, old.color);
					draw_line (old.x, old.y, lastrow[col].x,
						lastrow[col].y, old.color);
					draw_line (lastrow[col].x, lastrow[col].y,
						cur.x, cur.y, cur.color);
					num_tris++;
				}

				if (col < lastdot && currow &&
					lastrow[col].x > bad_check &&
					lastrow[col].y > bad_check &&
					lastrow[col].x < (xdots - bad_check) &&
					lastrow[col].y < (ydots - bad_check) &&
					lastrow[next].x > bad_check &&
					lastrow[next].y > bad_check &&
					lastrow[next].x < (xdots - bad_check) &&
					lastrow[next].y < (ydots - bad_check))
				{
					/* Get rid of all the triangles in the plane
					at the base of the object */

					if (f_cur.color == f_water &&
						f_lastrow[col].color == f_water &&
						f_lastrow[next].color == f_water)
							goto loopbottom;

					if (RAY != 6) /* Output the vertex info */
						out_triangle(f_cur, f_lastrow[col], f_lastrow[next],
							cur.color, lastrow[col].color, lastrow[next].color);

					tout = 1;

					draw_line (lastrow[col].x, lastrow[col].y, cur.x, cur.y,
						cur.color);
					draw_line (lastrow[next].x, lastrow[next].y, cur.x, cur.y,
						cur.color);
					draw_line (lastrow[next].x, lastrow[next].y, lastrow[col].x,
						lastrow[col].y, lastrow[col].color);
					num_tris++;
				}

				if (RAY == 6)  /* Output vertex info for Acrospin */
				{
					fprintf (File_Ptr1, "% #4.4f % #4.4f % #4.4f R%dC%d\n",
						f_cur.x, f_cur.y, f_cur.color, RO, CO);
					if (CO > CO_MAX)
						CO_MAX = CO;
					CO++;
				}
				goto loopbottom;

			}

			switch(FILLTYPE)
			{
				case -1:
					if (col &&
					old.x > bad_check &&
					old.x < (xdots - bad_check))
						draw_line (old.x, old.y, cur.x, cur.y, cur.color);
					if (currow &&
					lastrow[col].x > bad_check &&
					lastrow[col].y > bad_check &&
					lastrow[col].x < (xdots - bad_check) &&
					lastrow[col].y < (ydots - bad_check))
						draw_line (lastrow[col].x,lastrow[col].y,cur.x,
							cur.y,cur.color);
					break;
		
				case 0:
					(*plot)(cur.x,cur.y,cur.color);
					break;
		
				case 1:			/* connect-a-dot */
					if ((old.x < xdots) && (col) &&
						old.x > bad_check &&
						old.y > bad_check) /* Don't draw from old to cur on col 0 */
							draw_line(old.x,old.y,cur.x,cur.y,cur.color);
					break;
		
				case 2: /* with interpolation */
				case 3: /* no interpolation */
					/***************************************************************/
					/*  "triangle fill" - consider four points: current point,		*/
					/*  previous point same row, point opposite current point in	*/
					/*  previous row, point after current point in previous row.	*/
					/*  The object is to fill all points inside the two triangles.	*/
					/*																					*/
					/*	lastrow[col].x/y___ lastrow[next]									*/
					/*			  /	1 		/														*/
					/*		    /		1 	  /														*/
					/*			/		1	 / 														*/
					/*  oldrow/col _____ trow/col												*/
					/***************************************************************/
		
					if(currow && !col)
						putatriangle(lastrow[next],lastrow[col],cur,cur.color);
					if(currow && col)		/* skip first row and first column */
					{
						if(col == 1)
							putatriangle(lastrow[col],oldlast,old,old.color);
		
						if(col < lastdot)
							putatriangle(lastrow[next],lastrow[col], cur, cur.color);
						putatriangle(old, lastrow[col], cur, cur.color);
					}
					break;
		
				case 4: /* "solid fill" */
					if (SPHERE)
					{
						if (persp)
						{
							old.x = xcenter>>16;
							old.y = ycenter>>16;
						}
						else
						{
							old.x = xcenter;
							old.y = ycenter;
						}
					}
					else
					{
						lv[0] = col;
						lv[1] = currow;
						lv[2] = 0;
		
						/* apply fudge bit shift for integer math */
						lv[0] = lv[0] << 16;
						lv[1] = lv[1] << 16;
						/* Since 0, unnecessary lv[2] = lv[2] << 16;*/
		
						if(longvmultpersp(lv,lm,lv0,lv,lview,16))
						{
							cur	= bad;
							f_cur = f_bad;
							goto loopbottom;	/* another goto ! */
						}
		
						/*	Round and fudge back to original  */
						old.x = (lv[0]+32768L) >> 16;
						old.y = (lv[1]+32768L) >> 16;
					}
					if (old.x < 0)
						old.x = 0;
					if (old.x >= xdots)
						old.x = xdots-1;
					if (old.y < 0)
						old.y = 0;
					if (old.y >= ydots)
						old.y = ydots-1;
					draw_line(old.x,old.y,cur.x,cur.y,cur.color);
					break;
		
				case 5:
				case 6:
					/* light-source modulated fill */
					if(currow && col)		/* skip first row and first column */
					{
						if(f_cur.color < bad_check || f_old.color < bad_check ||
							f_lastrow[col].color < bad_check)
								break;
		
						v1[0] = f_cur.x		- f_old.x;
						v1[1] = f_cur.y		- f_old.y;
						v1[2] = f_cur.color - f_old.color;
		
						v2[0] = f_lastrow[col].x			- f_cur.x;
						v2[1] = f_lastrow[col].y			- f_cur.y;
						v2[2] = f_lastrow[col].color - f_cur.color;
		
						cross_product (v1, v2, cross);
		
						/* normalize cross - and check if non-zero */
						if(normalize_vector(cross))
						{
							if(debugflag)
							{
								static char far msg[] = {"debug, cur.color=bad"};
								stopmsg(0,msg);
							}
							cur.color = f_cur.color = bad.color;
						}
						else
						{
							/* line-wise averaging scheme */
							if(LIGHTAVG>0)
							{
								if(crossnotinit)
								{
									/* initialize array of old normal vectors */
									crossavg[0] = cross[0];
									crossavg[1] = cross[1];
									crossavg[2] = cross[2];
									crossnotinit = 0;
								}
/* Saves ~200 bytes but costs ~ 4% speed
								for (i=0;i<3;i++)
									crossavg[i] = cross[i] = tmpcross[i] = 
										(crossavg[i]*LIGHTAVG+cross[i]) / (LIGHTAVG+1);
*/
								tmpcross[0] = (crossavg[0]*LIGHTAVG+cross[0]) /
								   (LIGHTAVG+1);
								tmpcross[1] = (crossavg[1]*LIGHTAVG+cross[1]) /
								   (LIGHTAVG+1);
								tmpcross[2] = (crossavg[2]*LIGHTAVG+cross[2]) /
								   (LIGHTAVG+1);
			
								cross[0] = tmpcross[0];
								cross[1] = tmpcross[1];
								cross[2] = tmpcross[2];

								if(normalize_vector(cross))
								{
									/* this shouldn't happen */
									if(debugflag)
									{
										static char far msg[] =
										{"debug, normal vector err2"};
										stopmsg(0,msg);
										/* use next instead if you ever need details:
										static char far tmp[] = {"debug, vector err"};
										char msg[200];
										sprintf(msg,"%Fs\n%f %f %f\n%f %f %f\n%f %f %f",
										tmp, f_cur.x, f_cur.y, f_cur.color,
										f_lastrow[col].x, f_lastrow[col].y,
										f_lastrow[col].color, f_lastrow[col-1].x,
										f_lastrow[col-1].y,f_lastrow[col-1].color);
										stopmsg(0,msg);
										*/
									}
									cur.color = f_cur.color = colors;
								}
							}
							crossavg[0] = tmpcross[0];
							crossavg[1] = tmpcross[1];
							crossavg[2] = tmpcross[2];

							/* dot product of unit vectors is cos of angle between */
							/* we will use this value to shade surface */
		
							cur.color = 1 + (colors-2) *
								(1.0-dot_product(cross,light_direction));
						}
						/* if colors out of range, set them to min or max color 
							index but avoid background index. This makes colors
							"opaque" so SOMETHING plots. These conditions shouldn't 
							happen but just in case					*/
						if(cur.color < 1)			/* prevent transparent colors */
							cur.color = 1;		/* avoid background */
						if(cur.color > colors-1)
							cur.color = colors-1;
		
					/* why "col < 2"? So we have sufficient geometry for the fill */
					/* algorithm, which needs previous point in same row to have  */
					/* already been calculated (variable old)		*/
					/* fix ragged left margin in preview */
						if (col == 1 && currow > 1)
							putatriangle(lastrow[next],lastrow[col],cur,cur.color);
		
						if(col < 2 || currow < 2) /* don't have valid colors yet */
							break;
		
						if(col < lastdot)
							putatriangle(lastrow[next],lastrow[col],cur,cur.color);
						putatriangle(old,lastrow[col],cur,cur.color);
		
						plot=standardplot;
					}
					break;
			}  /*	End of CASE statement for fill type  */
		
loopbottom:

			if (RAY || (FILLTYPE != 0 && FILLTYPE != 4))
			{
				/* for triangle and grid fill purposes */
				oldlast = lastrow[col];
				old = lastrow[col] = cur;
		
				/* for illumination model purposes */
				f_old	= f_lastrow[col] = f_cur;
				if (currow && RAY && col >= lastdot)
				/* if we're at the end of a row, close the object */
				{
					end_object(tout);
					tout=0;
					if (ferror (File_Ptr1))
					{
						fclose (File_Ptr1);
						remove (light_name);
						File_Error(ray_name, 2);
						EXIT_OVLY;
						return(-1);
					}
				}
			}
		
			col++;

		}  /*  End of while statement for plotting line  */

	RO++;

reallythebottom:
		
	/* stuff that HAS to be done, even in preview mode, goes here */	
	if(SPHERE)	
	{	
		/* incremental sin/cos phi calc */	
		if(currow == 0)	
		{	
			sinphi = oldsinphi2;	
			cosphi = oldcosphi2;	
		}	
		else	
		{	
			sinphi = twocosdeltaphi*oldsinphi2 - oldsinphi1;	
			cosphi = twocosdeltaphi*oldcosphi2 - oldcosphi1;	
			oldsinphi1 = oldsinphi2;	
			oldsinphi2 = sinphi;	
			oldcosphi1 = oldcosphi2;	
			oldcosphi2 = cosphi;	
		}	
	}
	EXIT_OVLY;
	return(0); /* decoder needs to know all is well !!! */
}


/* vector version of line draw */
static void _fastcall vdraw_line(v1,v2,color)
double *v1,*v2;
int color;
{
	int x1,y1,x2,y2;
	x1 = (int)v1[0];
	y1 = (int)v1[1];
	x2 = (int)v2[0];
	y2 = (int)v2[1];
	draw_line(x1,y1,x2,y2,color);
}



static void corners(m,show,pxmin,pymin,pzmin,pxmax,pymax,pzmax)

MATRIX m;
int show; /* turns on box-showing feature */
double *pxmin,*pymin,*pzmin,*pxmax,*pymax,*pzmax;

{
	int i,j;
	VECTOR S[2][4]; /* Holds the top an bottom points, S[0][]=bottom */

	/*
	define corners of box fractal is in in x,y,z plane
	"b" stands for "bottom" - these points are the corners of the screen
	in the x-y plane. The "t"'s stand for Top - they are the top of
	the cube where 255 color points hit.
	*/

	*pxmin = *pymin = *pzmin = INT_MAX;
	*pxmax = *pymax = *pzmax = INT_MIN;

	for (j = 0; j < 4; ++j)
		for (i=0;i<3;i++)
			S[0][j][i]=S[1][j][i]=0;

	S[0][1][0] = S[0][2][0] = S[1][1][0] = S[1][2][0] = xdots-1;
	S[0][2][1] = S[0][3][1] = S[1][2][1] = S[1][3][1] = ydots-1;
	S[1][0][2] = S[1][1][2] = S[1][2][2] = S[1][3][2] = zcoord-1;

	for (i = 0; i < 4; ++i)
	{
		/* transform points */
		vmult(S[0][i],m,S[0][i]);
		vmult(S[1][i],m,S[1][i]);

		/* update minimums and maximums */
		if (S[0][i][0] <= *pxmin) *pxmin = S[0][i][0];
		if (S[0][i][0] >= *pxmax) *pxmax = S[0][i][0];
		if (S[1][i][0] <= *pxmin) *pxmin = S[1][i][0];
		if (S[1][i][0] >= *pxmax) *pxmax = S[1][i][0];
		if (S[0][i][1] <= *pymin) *pymin = S[0][i][1];
		if (S[0][i][1] >= *pymax) *pymax = S[0][i][1];
		if (S[1][i][1] <= *pymin) *pymin = S[1][i][1];
		if (S[1][i][1] >= *pymax) *pymax = S[1][i][1];
		if (S[0][i][2] <= *pzmin) *pzmin = S[0][i][2];
		if (S[0][i][2] >= *pzmax) *pzmax = S[0][i][2];
		if (S[1][i][2] <= *pzmin) *pzmin = S[1][i][2];
		if (S[1][i][2] >= *pzmax) *pzmax = S[1][i][2];
	}

	if(show)
	{
		if(persp)
		{
			for (i=0;i<4;i++)
			{
				perspective(S[0][i]);
				perspective(S[1][i]);
			}
		}

		/* Keep the box surrounding the fractal */
		for (j=0;j<2;j++)
			for (i = 0; i < 4; ++i)
			{
				S[j][i][0] += xxadjust;
				S[j][i][1] += yyadjust;
			}

		draw_rect(S[0][0],S[0][1],S[0][2],S[0][3],2,1);/* Bottom */

		draw_rect(S[0][0],S[1][0],S[0][1],S[1][1], 5, 0); /* Sides */
		draw_rect(S[0][2],S[1][2],S[0][3],S[1][3], 6, 0);

		draw_rect(S[1][0],S[1][1],S[1][2],S[1][3],8,1); /* Top */
	}
}



/* This function draws a vector from origin[] to direct[] and a box
	around it. The vector and box are transformed or not depending on
	FILLTYPE.

*/

static void draw_light_box (origin, direct, light_m)
double *origin, *direct;
MATRIX light_m;


{
	VECTOR S[2][4];
	int i,j;
	double temp;

	S[1][0][0] = S[0][0][0] = origin[0];
	S[1][0][1] = S[0][0][1] = origin[1];

	S[1][0][2] = direct[2];

	for (i=0;i<2;i++)
	{
		S[i][1][0] = S[i][0][0];
		S[i][1][1] = direct[1];
		S[i][1][2] = S[i][0][2];
		S[i][2][0] = direct[0];
		S[i][2][1] = S[i][1][1];
		S[i][2][2] = S[i][0][2];
		S[i][3][0] = S[i][2][0];
		S[i][3][1] = S[i][0][1];
		S[i][3][2] = S[i][0][2];
	}

	/* transform the corners if necessary */
	if (FILLTYPE == 6)
		for (i=0;i<4;i++)
		{
			vmult (S[0][i],light_m,S[0][i]);
			vmult (S[1][i],light_m,S[1][i]);
		}

	/* always use perspective to aid viewing */
	temp = view[2]; /* save perspective distance for a later restore */
	view[2] = - P * 300.0 / 100.0;

	for (i=0;i<4;i++)
	{
		perspective(S[0][i]);
		perspective(S[1][i]);
	}
	view[2] = temp; /* Restore perspective distance*/

	/* Adjust for aspect */
	for (i=0;i<4;i++)
	{
		S[0][i][0] = S[0][i][0] * aspect;
		S[1][i][0] = S[1][i][0] * aspect;
	}

	/* draw box connecting transformed points. NOTE order and COLORS */
	draw_rect(S[0][0],S[0][1],S[0][2],S[0][3],2,1);

	vdraw_line (S[0][0],S[1][2],8);

	/* sides */
	draw_rect(S[0][0],S[1][0],S[0][1],S[1][1], 4, 0);
	draw_rect(S[0][2],S[1][2],S[0][3],S[1][3], 5, 0);

	draw_rect(S[1][0],S[1][1],S[1][2],S[1][3],3,1);

	/* Draw the "arrow head" */
	for (i=-3;i<4;i++)
		for (j=-3;j<4;j++)
			if (abs(i) + abs(j) < 6)
				plot((int)(S[1][2][0]+i),(int)(S[1][2][1]+j),10);
}



static void draw_rect (V0, V1, V2, V3, color, rect)

VECTOR V0, V1, V2, V3;
int color, rect;

{
VECTOR V[4];
int i;

	for (i=0;i<2;i++) 
	{ /* Since V[2] is not used by vdraw_line don't bother setting it */	
		V[0][i] = V0[i];	
		V[1][i] = V1[i];	
		V[2][i] = V2[i];	
		V[3][i] = V3[i];	
	}	
	if (rect) /* Draw a rectangle */
	{
		for (i=0;i<4;i++)
			if (fabs(V[i][0]-V[(i+1)%4][0]) < -2*bad_check &&
				fabs(V[i][1]-V[(i+1)%4][1]) < -2*bad_check)
					vdraw_line (V[i],V[(i+1)%4],color);
	}
	else /* Draw 2 lines instead */
	{
		for(i=0;i<3;i+=2)
			if (fabs(V[i][0]-V[i+1][0]) < -2*bad_check &&
				fabs(V[i][1]-V[i+1][1]) < -2*bad_check)
					vdraw_line (V[i],V[i+1],color);
	}
	return;
}



/* replacement for plot - builds a table of min and max x's instead of plot */
/* called by draw_line as part of triangle fill routine */
static void _fastcall putminmax(int x,int y,int color)
{
	if(y >= 0 && y < ydots)
	{
		if(x < minmax_x[y].minx) minmax_x[y].minx = x;
		if(x > minmax_x[y].maxx) minmax_x[y].maxx = x;
	}
}

/*
	This routine fills in a triangle. Extreme left and right values for
	each row are calculated by calling the line function for the sides.
	Then rows are filled in with horizontal lines
*/
#define MAXOFFSCREEN  2 /* allow two of three points to be off screen */

static void _fastcall putatriangle(struct point pt1,struct point pt2,
					struct point pt3,int color)
{
	extern struct point p1,p2,p3;
	int miny,maxy;
	int x,y,xlim;

	/* Too many points off the screen? */
	if(offscreen(pt1) + offscreen(pt2) + offscreen(pt3) > MAXOFFSCREEN)
		return;

	p1 = pt1; /* needed by interpcolor */
	p2 = pt2;
	p3 = pt3;

	/* fast way if single point or single line */
	if (p1.y == p2.y && p1.x == p2.x)
	{
		plot = fillplot;
		if (p1.y == p3.y && p1.x == p3.x)
			(*plot)(p1.x, p1.y, color);
		else
			draw_line(p1.x,p1.y,p3.x,p3.y,color);
		plot = normalplot;
		return;
	}
	else if ( (p3.y == p1.y && p3.x == p1.x) || (p3.y == p2.y && p3.x == p2.x) )
	{
		plot = fillplot;
		draw_line(p1.x,p1.y,p2.x,p2.y,color);
		plot = normalplot;
		return;
	}

	/* find min max y */
	miny = maxy = p1.y;
	if(p2.y < miny) miny = p2.y;
	else 		maxy = p2.y;
	if(p3.y < miny) miny = p3.y;
	else if(p3.y > maxy) maxy = p3.y;

	/* only worried about values on screen */
	if(miny < 0)		miny = 0;
	if(maxy >= ydots) maxy = ydots-1;

	for(y=miny;y<=maxy;y++)
	{
		minmax_x[y].minx = INT_MAX;
		minmax_x[y].maxx = INT_MIN;
	}

	/* set plot to "fake" plot function */
	plot = putminmax;

	/* build table of extreme x's of triangle */
	draw_line(p1.x,p1.y,p2.x,p2.y,0);
	draw_line(p2.x,p2.y,p3.x,p3.y,0);
	draw_line(p3.x,p3.y,p1.x,p1.y,0);

	for(y=miny;y<=maxy;y++)
	{
		xlim = minmax_x[y].maxx;
		for(x=minmax_x[y].minx;x<=xlim;x++)
		(*fillplot)(x,y,color);
	}
	plot = normalplot;
}


static int _fastcall offscreen(struct point pt)
{
	if(pt.x >= 0)
		if(pt.x < xdots)
			if(pt.y >= 0)
				if(pt.y < ydots)
					return(0); /* point is ok */
	if (abs(pt.x) > 0-bad_check || abs(pt.y) > 0-bad_check)
		return(99); /* point is bad */
	return(1); /* point is off the screen */
}

static void _fastcall clipcolor(int x,int y,int color)
{
	if(0 <= x	&& x < xdots	&&
		0 <= y	&& y < ydots	&&
		0 <= color && color < filecolors)
	{
		standardplot(x,y,color);

		if (Targa_Out)
			targa_color(x, y, color);
	}
}

/*********************************************************************/
/* This function is the same as clipcolor but checks for color being */
/* in transparent range. Intended to be called only if transparency  */
/* has been enabled.												 */
/*********************************************************************/

static void _fastcall T_clipcolor(int x,int y,int color)
{
	if (0 <= x		&& x < xdots			&& /*  is the point on screen?  */
		0 <= y	&& y < ydots			&& /*  Yes?  */
		0 <= color && color < colors	&& /*  Colors in valid range?  */
		/*  Lets make sure its not a transparent color  */
		(transparent[0] > color || color > transparent[1]))
	{
		standardplot(x,y,color); /* I guess we can plot then  */

		if (Targa_Out)
			targa_color(x, y, color);
	}
}

/************************************************************************/
/* A substitute for plotcolor that interpolates the colors according	*/
/* to the x and y values of three points (p1,p2,p3) which are static in	*/
/* this routine															*/
/*																		*/
/*	In Light source modes, color is light value, not actual color		*/
/*	Real_Color always contains the actual color							*/
/************************************************************************/

static void _fastcall interpcolor(int x,int y,int color)
{
	int D,d1,d2,d3;

  /* this distance formula is not the usual one - but it has the virtue
		that it uses ONLY additions (almost) and it DOES go to zero as the
		points get close.
		*/

	d1 = abs(p1.x-x)+abs(p1.y-y);
	d2 = abs(p2.x-x)+abs(p2.y-y);
	d3 = abs(p3.x-x)+abs(p3.y-y);

	D = (d1 + d2 + d3) << 1;
	if(D)
	{  /* calculate a weighted average of colors -
		long casts prevent integer overflow. This can evaluate to zero */
		color = ((long)(d2+d3)*(long)p1.color +
			(long)(d1+d3)*(long)p2.color +
			(long)(d1+d2)*(long)p3.color) / D;
	}
	
	if(0 <= x		&& x < xdots	&&
		0 <= y		&& y < ydots	&&
		0 <= color && color < colors &&
		(transparent[1] == 0 || Real_Color > transparent[1] ||
		transparent[0] > Real_Color))
	{
		if (Targa_Out)
			D = targa_color(x,y,color);

		if (FILLTYPE >= 5)
			if (Real_V && Targa_Out)
				color = D;
			else
                        {
                            color =  (1 + (unsigned)color * IAmbient)/256;
                            if (color == 0)
                                color = 1;
                        }
		standardplot(x,y,color);
	}
}



/*
	In non light source modes, both color and Real_Color contain the
	actual pixel color. In light source modes, color contains the
	light value, and Real_Color contains the origninal color
*/

static int _fastcall targa_color(int x,int y,int color)
{
	unsigned long H, S, V;
	unsigned char RGB[3];

	if (FILLTYPE == 2)  
		Real_Color = color; /* So Targa gets interpolated color */

	RGB[0]  =  dacbox [Real_Color] [0] << 2; /* Move color space to */
	RGB[1]  =  dacbox [Real_Color] [1] << 2; /* 256 color primaries */
	RGB[2]  =  dacbox [Real_Color] [2] << 2; /* from 64 colors */

	/* Now lets convert it to HSV */
	R_H(RGB[0], RGB[1], RGB[2], &H, &S, &V);

	/* Modify original S and V components */
	if (FILLTYPE > 4) /* Adjust for Ambient */
		V = (V * (65535 - (unsigned)(color * IAmbient))) / 65535;

	if (haze)
	{
		S = (unsigned long)(S * HAZE_MULT) / 100; /* Haze lowers sat of colors */
		if (V >= 32640) /* Haze reduces contrast */
		{
			V = V - 32640;
			V = (unsigned long)((V * HAZE_MULT) / 100);
			V = V + 32640;
		}
		else
		{
			V = 32640 - V;
			V = (unsigned long)((V * HAZE_MULT) / 100);
			V = 32640 - V;
		}
	}
	/* Now lets convert it back to RGB. Original Hue, modified Sat and Val */
	H_R(&RGB[0], &RGB[1], &RGB[2], H, S, V);

	if (Real_V)
		V = (35 * (int)RGB[0] + 45 * (int)RGB[1] + 20 * (int)RGB[2]) / 100;

	/* Now write the color triple to its transformed location */
	/* on the disk. */
	targa_writedisk (x+sxoffs, y+syoffs, RGB[0], RGB[1], RGB[2]);

	return(255-V);
}



static int set_pixel_buff(unsigned char *pixels,unsigned char far *fraction,unsigned linelen)
{
	int i;
	if ((evenoddrow++ & 1) == 0) /* even rows are color value */
	{
		for(i=0;i<linelen;i++) /* add the fractional part in odd row */
			fraction[i] = pixels[i];
		return(1);
	}
	else /* swap */
	{
		unsigned char tmp;
		for(i=0;i<linelen;i++) /* swap so pixel has color */
		{
			tmp = pixels[i];
			pixels[i]=fraction[i];
			fraction[i]=tmp;
		}
	}
	return(0);
}


/* cross product  - useful because cross is perpendicular to v and w */
/**** PB commented this out - it is unused
static int chk_cross_product (int col, int row,VECTOR v, VECTOR w, VECTOR cross, VECTOR crossavg)
{
	static start = 1;
	static FILE *fp;

	if(start)
	{
		fp = fopen("blob","w");
		start = 0;
	}

	fprintf(fp,"row %+4d col %+4d v1 %+8.3e %+8.3e %+8.3e v2 %+8.3e %+8.3e %+8.3e cross %+8.3e %+8.3e %+8.3e crossavg %+8.3e %+8.3e %+8.3e \n",
	row,col,v[0],v[1],v[2],w[0],w[1],w[2],cross[0],cross[1],cross[2],crossavg[0],crossavg[1],crossavg[2]);
	return(0);
}
***/

/**************************************************************************

		Common routine for printing error messages to the screen for Targa
		and other files

**************************************************************************/


static char f[]			= "%Fs%Fs";
static char far OOPS[]  = {"OOPS, "};
static char far E1[]		= {"can't handle this type of file.\n"};
static char far str1[]  = {"couldn't open  < "};
static char far str3[]  = {"image wrong size\n"};
static char far temp1[] = {"ran out of disk space. < "};

static void File_Error (char *File_Name1, int ERROR)
{
   char msgbuf[200];

	error = ERROR;
	switch(ERROR)
	{
	case 1: /* Can't Open */
		sprintf(msgbuf,"%Fs%Fs%s >", OOPS, str1, File_Name1);
		break;
	case 2: /* Not enough room */
		sprintf(msgbuf,"%Fs%Fs%s >", OOPS, temp1, File_Name1);
		break;
	case 3: /* Image wrong size */
		sprintf(msgbuf, f, OOPS, str3);
		break;
	case 4: /* Wrong file type */
		sprintf(msgbuf, f, OOPS, E1);
		break;
	}
	stopmsg(0,msgbuf);
	return;
}


/************************************************************************/
/*																		*/
/*		This function opens a TARGA_24 file for reading and writing. If	*/
/*		its a new file, (overlay == 0) it writes a header. If it is to	*/
/*		overlay an existing file (overlay == 1) it copies the original	*/
/*		header whose lenght and validity was determined in				*/
/*		Targa_validate.													*/
/*																		*/
/*		It Verifies there is enough disk space, and leaves the file 	*/
/*		at the start of the display data area.							*/
/*																		*/
/*		If this is an overlay, closes source and copies to "targa_temp"	*/
/*		If there is an error close the file. 							*/
/*																		*/
/* **********************************************************************/


static int startdisk1 (char *File_Name2, FILE *Source, int overlay)
{
	int i,j,k, inc;
	FILE *fps;

	/* Open File for both reading and writing */
	if ((fps=fopen(File_Name2,"w+b"))==NULL)
	{
		File_Error(File_Name2, 1);
		return(-1); /* Oops, somethings wrong! */
	}

	inc = 1; /* Assume we are overlaying a file */

	/* Write the header */
	if (overlay) /* We are overlaying a file */
		for (i=0;i<T_header_24;i++) /* Copy the header from the Source */
			fputc(fgetc(Source),fps);
	else
	{	/* Write header for a new file */
		/* ID field size = 0, No color map, Targa type 2 file */
		for (i=0;i<12;i++)
		{
			if (i == 2)
				fputc(i,fps);
			else
				fputc(0,fps);
		}
		/*  Write image size  */
		for (i=0;i<4;i++)
			fputc(upr_lwr[i],fps);
		fputc(T24,fps); /* Targa 24 file */
		fputc(T32,fps); /* Image at upper left */
		inc = 3;
	}

	/*  Finished with the header, now lets work on the display area  */
	for (i=0;i<ydots;i++)	/* "clear the screen" (write to the disk) */
	{
		for (j=0;j<line_length1;j=j+inc)
		{
			if (overlay)
				fputc(fgetc(Source), fps);
			else
				for (k=2;k>-1;k--)
					fputc(back_color[k], fps);			/* Targa order (B, G, R) */
		}
		if (ferror (fps))		
		{		
			/*  Almost certainly not enough disk space  */		
			fclose (fps);
			fclose (Source);
			remove (File_Name2);		
			File_Error(File_Name2, 2);		
			return(-2);		
		}
		if (check_key())		return(-3);
	}

	if (targa_startdisk(fps, T_header_24) != 0)
	{
		enddisk();
		remove (File_Name2);
		return(-4);
	}
	return(0);
}

/**************************************************************************


**************************************************************************/

int targa_validate(char *File_Name)
{
	FILE *fp;
	int i, j = 0;

	/* Attempt to open source file for reading */
	if ((fp=fopen(File_Name,"rb"))==NULL)
	{
		File_Error(File_Name, 1);
		return(-1); /* Oops, file does not exist */
	}

	T_header_24 += fgetc(fp); /* Check ID field and adjust header size */

	if (fgetc(fp)) /* Make sure this is an unmapped file */
	{
		File_Error(File_Name, 4);
		return(-1);
	}

	if (fgetc(fp)!=2) /* Make sure it is a type 2 file */
	{
		File_Error(File_Name, 4);
		return(-1);
	}

	/* Skip color map specification */
	for (i=0;i<5;i++)
			fgetc(fp);

	for (i=0;i<4;i++)
	{
		/* Check image origin */
		fgetc(fp);
		if(j != 0)
		{
			File_Error(File_Name, 4);
			return(-1);
		}
	}
	/* Check Image specs */
	for (i=0;i<4;i++)
		if(fgetc(fp) != upr_lwr[i])
		{
			File_Error(File_Name,3);
			return(-1);
		}

	if(fgetc(fp) != T24) error=4; /* Is it a targa 24 file? */
	if(fgetc(fp) != T32) error=4; /* Is the origin at the upper left? */
	if (error == 4)
	{
		File_Error(File_Name,4);
		return(-1);
	}
	rewind(fp);

	/* Now that we know its a good file, create a working copy */
	if (startdisk1(targa_temp, fp, 1))
		return(-1);
	
	fclose(fp);	/* Close the source */

	T_Safe=1; /* Original file successfully copied to targa_temp */
	return(0);
}

static int R_H (R, G, B, H, S, V)
unsigned char R,G,B;
unsigned long *H, *S, *V;
{
	unsigned long	R1, G1, B1, DENOM;
	unsigned char MIN;

	*V = R;
	MIN = G;
	if (R < G)
	{
		*V = G;
		MIN = R;
		if (G < B)			*V = B;
		if (B < R)			MIN = B;
	}
	else
	{
		if (B < G)			MIN = B;
		if (R < B)			*V = B;
	}
	DENOM = *V - MIN;
	if (*V != 0 && DENOM !=0)
	{
		*S = ((DENOM << 16) / *V) - 1;
	}
	else  *S = 0;/* Color is black! and Sat has no meaning */
	if(*S == 0) /*  R=G=B => shade of grey and Hue has no meaning */
	{
		*H = 0;
		*V = *V << 8;
		return(1);  /* v or s or both are 0 */
	}
	if (*V == MIN)
	{
		*H = 0;
		*V = *V << 8;
		return(0);
	}
	R1 = (((*V - R) * 60) << 6) / DENOM;		/* distance of color from red	*/
	G1 = (((*V - G) * 60) << 6) / DENOM;		/* distance of color from green */
	B1 = (((*V - B) * 60) << 6) / DENOM;		/* distance of color from blue  */
	if(*V == R)
		if (MIN == G)		*H = (300 << 6) + B1;
		else			*H = (60 << 6) - G1;
	if(*V == G)
		if (MIN == B)		*H = (60 << 6) + R1;
		else			*H = (180 << 6) - B1;
	if(*V == B)
		if(MIN == R)		*H = (180 << 6) + G1;
		else			*H = (300 << 6) - R1;

	*V = *V << 8;
	return(0);
}

static int H_R (R, G, B, H, S, V)
unsigned char *R, *G, *B;
unsigned long  H, S, V;
{
	unsigned long		P1, P2, P3;
	int	RMD, I;

	if(H >= 23040)			H = H % 23040; /*  Makes h circular  */
	I = H / 3840;
	RMD = H % 3840;			/*  RMD = fractional part of H	*/

	P1 = ((V * (65535 - S)) / 65280) >> 8;
	P2 = (((V * (65535 - (S * RMD) / 3840)) / 65280) - 1) >> 8;
	P3 = (((V * (65535 - (S * (3840 - RMD)) / 3840)) / 65280)) >> 8;
	V = V >> 8;
	switch (I)
	{
	case 0:
		*R = V;
		*G = P3;
		*B = P1;
		break;
	case 1:
		*R = P2;
		*G = V;
		*B = P1;
		break;
	case 2:
		*R = P1;
		*G = V;
		*B = P3;
		break;
	case 3:
		*R = P1;
		*G = P2;
		*B = V;
		break;
	case 4:
		*R = P3;
		*G = P1;
		*B = V;
		break;
	case 5:
		*R = V ;
		*G = P1;
		*B = P2;
		break;
	}
	return(0);
}


/********************************************************************/
/*																	*/
/*  This routine writes a header to a ray tracer data file. It		*/
/*  Identifies the version of FRACTINT which created it an the		*/
/*  key 3D parameters in effect at the time.						*/
/*																	*/
/********************************************************************/


static char far declare[] = {"DECLARE	"};
static char far frac_default[] = {"F_Dflt"};
static char far color[] = {"COLOR  "};
static char far dflt[] = {"RED 0.8 GREEN 0.4 BLUE 0.1\n"};
static char far d_color[] = {"0.8 0.4 0.1"};
static char far r_surf[] = {"0.95 0.05 5 0 0\n"};
static char far surf[] = {"surf={diff="};
static char far rs_surf[] = {"surface T diff "};
static char far end[] = {"END_"};
static char far plane[] = {"PLANE"};
static char far m1[] = {"-1.0 "};
static char far one[] = {" 1.0 "};
static char far z[]  = {" 0.0 "};
static char far bnd_by[]  = {" BOUNDED_BY\n"};
static char far end_bnd[]  = {" END_BOUND\n"};
static char far inter[]  = {"INTERSECTION\n"};
static char fmt[] = "	%Fs <%Fs%Fs%Fs> % #4.3f %Fs%Fs\n";

static char far composite[] = {"COMPOSITE"};
static char far object[]  = {"OBJECT"};
static char far triangle[] = {"TRIANGLE "};
static char far l_tri[] = {"triangle"};
static char far texture[] = {"TEXTURE\n"};
/* static char far end_texture[] = {" END_TEXTURE\n"}; */
static char far red[] = {"RED"};
static char far green[] = {"GREEN"};
static char far blue[] = {"BLUE"};

static char far frac_texture[] = {"	AMBIENT 0.25 DIFFUSE 0.75"};

static char far polygon[] = {"polygon={points=3;"};
static char far vertex[] = {" vertex =  "};
static char far d_vert[] = {"	<"};
static char f1[] = "% #4.4f ";
static char far grid[] = {"grid 20 20 20\n\n"};
static char n[] = "\n";
static char f2[] = "R%dC%d R%dC%d\n";


static int _fastcall RAY_Header(void)


{  /* Open the ray tracing output file */
	check_writefile(ray_name,".ray");
	if ((File_Ptr1=fopen(ray_name,"w"))==NULL)
		return(-1); /* Oops, somethings wrong! */

	if (RAY == 2)
		fprintf(File_Ptr1, "//");
	if (RAY == 4)
		fprintf(File_Ptr1, "#");
	if (RAY == 5)
		fprintf(File_Ptr1, "/*\n");
	if (RAY == 6)
		fprintf(File_Ptr1, "--");

	fprintf(File_Ptr1, banner, s3, release/100., s3a);

	if (RAY == 5)
		fprintf(File_Ptr1, "*/\n");


	/* Set the default color */
	if (RAY == 1)
	{
		fprintf (File_Ptr1, f, declare, frac_default);
		fprintf (File_Ptr1, " = ");
		fprintf (File_Ptr1, f, color, dflt);
	}
	if (BRIEF)
	{
		if (RAY == 2)
		{
			fprintf (File_Ptr1, f, surf, d_color);
			fprintf (File_Ptr1, ";}\n");
		}
		if (RAY == 4)
		{
			fprintf (File_Ptr1, "f ");
			fprintf (File_Ptr1, f, d_color, r_surf);
		}
		if (RAY == 5)
			fprintf (File_Ptr1, f, rs_surf, d_color);
	}
	fprintf (File_Ptr1, n);
	if (RAY == 6)
		fprintf (File_Ptr1, "%Fs", acro_s1);

	return(0);
}


/********************************************************************/
/*																	*/
/*  This routine describes the triangle to the ray tracer, it		*/
/*  sets the color of the triangle to the average of the color		*/
/*  of its verticies and sets the light parameters to arbitrary		*/
/*  values.															*/
/*																	*/
/*  Note: numcolors (number of colors in the source					*/
/*  file) is used instead of colors (number of colors avail. with	*/
/*  display) so you can generate ray trace files with your LCD		*/
/*  or monochrome display											*/
/*																	*/
/********************************************************************/

static int _fastcall out_triangle(struct f_point pt1, struct f_point pt2,
			struct f_point pt3, int c1, int c2, int c3)

{
int i, j;
float c[3];
float pt_t[3][3];

	/* Normalize each vertex to screen size and adjust coordinate system */
	pt_t[0][0]			=	2 * pt1.x			/ xdots  - 1;
	pt_t[0][1]			=  (2 * pt1.y			/ ydots  - 1);
	pt_t[0][2]			=  -2 * pt1.color / numcolors - 1;
	pt_t[1][0]			=	2 * pt2.x			/ xdots  - 1;
	pt_t[1][1]			=  (2 * pt2.y			/ ydots  - 1);
	pt_t[1][2]			=  -2 * pt2.color / numcolors - 1;
	pt_t[2][0]			=	2 * pt3.x			/ xdots  - 1;
	pt_t[2][1]			=  (2 * pt3.y			/ ydots  - 1);
	pt_t[2][2]			=  -2 * pt3.color / numcolors - 1;

	/* Color of triangle is average of colors of its verticies */
	if (!BRIEF)
		for (i=0;i<=2;i++)
		c[i] = (float)(dacbox[c1][i] + dacbox[c2][i] + dacbox[c3][i])
				/ (3 * 63);

		/* get rid of degenerate triangles: any two points equal */
		if (pt_t[0][0] == pt_t[1][0] &&
				pt_t[0][1] == pt_t[1][1] &&
				pt_t[0][2] == pt_t[1][2] ||

				pt_t[0][0] == pt_t[2][0] &&
				pt_t[0][1] == pt_t[2][1] &&
				pt_t[0][2] == pt_t[2][2] ||

				pt_t[2][0] == pt_t[1][0] &&
				pt_t[2][1] == pt_t[1][1] &&
				pt_t[2][2] == pt_t[1][2])
			return(0);

	/* Describe the triangle */
	if (RAY == 1)
		fprintf (File_Ptr1, " %Fs\n  %Fs", object, triangle);
	if (RAY == 2 && !BRIEF)
		fprintf (File_Ptr1, "%Fs", surf);
	if (RAY == 4 && !BRIEF)
		fprintf (File_Ptr1, "f");
	if (RAY == 5 && !BRIEF)
		fprintf (File_Ptr1, "%Fs", rs_surf);

	if (!BRIEF && RAY != 1)
		for (i=0;i<=2;i++)
		fprintf (File_Ptr1, f1, c[i]);

	if (RAY == 2)
	{
		if (!BRIEF)
		fprintf (File_Ptr1, ";}\n");
		fprintf (File_Ptr1, "%Fs", polygon);
	}
	if (RAY == 4)
	{
		if (!BRIEF)
		fprintf (File_Ptr1, "%Fs", r_surf);
		fprintf (File_Ptr1, "p 3");
	}
	if (RAY == 5)
	{
		if (!BRIEF)			fprintf (File_Ptr1, n);
		fprintf (File_Ptr1, "%Fs T", l_tri);
	}

	for(i=0;i<=2;i++)  /*  Describe each  Vertex  */
	{
		fprintf (File_Ptr1, n);

		if (RAY == 1)
		fprintf (File_Ptr1, "%Fs", d_vert);
		if (RAY == 2)
		fprintf (File_Ptr1, "%Fs", vertex);
		if (RAY > 3)
		fprintf (File_Ptr1, " ");

		for(j=0;j<=2;j++)
		if (!(RAY == 4 || RAY == 5))
			fprintf (File_Ptr1, f1, pt_t[i][j]); /* Right handed */
		else
			fprintf (File_Ptr1, f1, pt_t[2-i][j]); /* Left handed */

		if (RAY == 1)
		fprintf (File_Ptr1, ">");
		if (RAY == 2)
		fprintf (File_Ptr1, ";");
	}

	if (RAY == 1)
	{
		fprintf (File_Ptr1, " %Fs%Fs\n", end, triangle);
		if (!BRIEF)
		{
		fprintf (File_Ptr1, "  %Fs"
			"	%Fs%Fs% #4.4f %Fs% #4.4f %Fs% #4.4f\n"
			"%Fs"
			" %Fs%Fs",
			texture,
			color, red, c[0], green, c[1], blue, c[2],
			frac_texture,
			end, texture);
		}
		fprintf (File_Ptr1, "  %Fs%Fs  %Fs%Fs",
		color, frac_default,
		end, object);
		triangle_bounds(pt_t); /* update bounding info */
	}
	if (RAY == 2)
		fprintf (File_Ptr1, "}");
	if (RAY == 3 && !BRIEF)
		fprintf (File_Ptr1, n);

	fprintf (File_Ptr1, n);

	return(0);
}


/********************************************************************/
/*																	*/
/*  This routine calculates the min and max values of a triangle	*/
/*  for use in creating ray tracer data files. The values of min	*/
/*  and max x, y, and z are assumed to be global.					*/
/*																	*/
/********************************************************************/

static void _fastcall triangle_bounds(float pt_t[3][3])

{
int i,j;

	for (i=0;i<=2;i++)
	for (j=0;j<=2;j++)
	{
		if(pt_t[i][j] < min_xyz[j]) min_xyz[j] = pt_t[i][j];
		if(pt_t[i][j] > max_xyz[j]) max_xyz[j] = pt_t[i][j];
	}
	return;
}

/********************************************************************/
/*																	*/
/*  This routine starts a composite object for ray trace data files.*/
/*																	*/
/********************************************************************/

static int _fastcall start_object(void)

{
	if (RAY != 1)			return(0);

	/*	Reset the min/max values, for bounding box  */
	min_xyz[0] = min_xyz[1] = min_xyz[2] =  999999;
	max_xyz[0] = max_xyz[1] = max_xyz[2] = -999999;

	fprintf (File_Ptr1, "%Fs\n", composite);
	return(0);
}

/********************************************************************/
/*																	*/
/*  This routine adds a bounding box for the triangles drawn		*/
/*  in the last block and completes the composite object created.	*/
/*  It uses the globals min and max x,y and z calculated in			*/
/*  z calculated in Triangle_Bounds().								*/
/*																	*/
/********************************************************************/

static int _fastcall end_object(int triout)
{
	if (RAY == 1)
	{
	if (triout)
		{
		/* Make sure the bounding box is slightly larger than the object */
		int i;
		for (i=0;i<=2;i++)
		{
			if (min_xyz[i] == max_xyz[i])
			{
				min_xyz[i] -= 0.01;
			max_xyz[i] += 0.01;
			}
			else
			{
				min_xyz[i] -= (max_xyz[i] - min_xyz[i]) * 0.01;
				max_xyz[i] += (max_xyz[i] - min_xyz[i]) * 0.01;
			}
		}

		/* Add the bounding box info */
		fprintf (File_Ptr1, "%Fs  %Fs", bnd_by, inter);
		fprintf (File_Ptr1, fmt, plane,m1,z,z,-min_xyz[0],end,plane);
		fprintf (File_Ptr1, fmt, plane,one,z,z,max_xyz[0],end,plane);
		fprintf (File_Ptr1, fmt, plane,z,m1,z,-min_xyz[1],end,plane);
		fprintf (File_Ptr1, fmt, plane,z,one,z,max_xyz[1],end,plane);
		fprintf (File_Ptr1, fmt, plane,z,z,m1,-min_xyz[2],end,plane);
		fprintf (File_Ptr1, fmt, plane,z,z,one,max_xyz[2],end,plane);
		fprintf (File_Ptr1, "  %Fs%Fs%Fs", end, inter, end_bnd);
		}

		/* Complete the composite object statement */
		fprintf (File_Ptr1, "%Fs%Fs\n", end, composite);
	}

	if (RAY != 6)			fprintf (File_Ptr1, n);

	return(0);
}


static void line3d_cleanup()
{
 int i,j;
	if(RAY && File_Ptr1)
	{  /*  Finish up the ray tracing files */
		static char far n_ta[] ={"{ No. Of Triangles = "};
		fprintf (File_Ptr1, n);
		if (RAY == 2)
			fprintf(File_Ptr1, "\n\n//");
		if (RAY == 4)
			fprintf(File_Ptr1, "\n\n#");

		if (RAY == 5)
			fprintf (File_Ptr1, "%Fs/*\n", grid);
		if (RAY == 6)
		{
			fprintf (File_Ptr1, "%Fs", acro_s2);
			for (i=0;i<RO;i++)
				for (j=0;j<=CO_MAX;j++)
				{
					if (j < CO_MAX)
						fprintf (File_Ptr1, f2, i, j, i, j+1);
					if (i < RO - 1)
						fprintf (File_Ptr1, f2, i, j, i+1, j);
					if (i && i < RO && j < CO_MAX)
						fprintf (File_Ptr1, f2, i, j, i-1, j+1);
				}
			fprintf (File_Ptr1, "\n\n--");
		}
		fprintf(File_Ptr1, "%Fs%d%Fs",n_ta, num_tris, s3a);
		if (RAY == 5)
			fprintf (File_Ptr1, "*/\n");
		fclose(File_Ptr1);
		File_Ptr1 = NULL;
	}
	if (Targa_Out)
	{	/*  Finish up targa files */

		T_header_24 = 18; /* Reset Targa header size */
		enddisk();
		if (!debugflag && T_Safe && !error && Targa_Overlay)
		{
			remove(light_name);
			rename(targa_temp,light_name);
		}
		if (!debugflag && Targa_Overlay)
			remove (targa_temp);
	}
	usr_floatflag &= 1; /* strip second bit */
	error = T_Safe = 0;
}

