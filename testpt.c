/*

This is an example of the use of the "test" fractal type.

This particular example is courtesy of Phil Wilson and demonstrates the
Distance Estimator method of calculating the Mandelbrot and Julia Sets.

At the moment, the algorithm uses the Mandelbrot Set if parm1=parm2=0.0
and the Julia Set otherwise, for no good reason other than it fits nicely
into FRACTINT's single "test" fractal type that way.

*/

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef __TURBOC__
#include <malloc.h>
#else
#include <alloc.h>
#endif

double	MSetDist(double cx, double cy, double parm1, 
						double parm2, int maxiter);
double	JSetDist(double cx, double cy, double parm1, 
						double parm2, int maxiter);

extern	double deltaX, deltaY;
extern	double	param[];
extern int maxit, inside;
extern	int colors;

static	double	 *xorbit, *yorbit;	/* arrays for test program */
static	double	delta;			/* calculate this once */
static	int	proceed;		/* did we find the memory? */
static	int	outside;		/* outside color, calc'd once */
static	int	calctype;		/* 0 == mandel, 1 == julia */

teststart()	/* this routine is called just before the fractal starts */
{

/* get the memory we need */
proceed = 0;				/* indicate don't proceed */
if (( xorbit = (double *) calloc(maxit+1, sizeof(*xorbit)))== NULL) {
	buzzer(2);
	printf("calloc error for xorbit with %d orbits\n", maxit);
	return(0);
	}
if (( yorbit = (double *) calloc(maxit+1, sizeof(*yorbit)))==NULL) {
	buzzer(2);
	printf("calloc error for yorbit with %d orbits\n", maxit);
	return(0);
	}
proceed = 1;				/* indicate proceed */

delta = (deltaX + deltaY) / 4;	/* half a pixel width */

outside = 15;				/* white in std vga palette */
if (outside >= colors) outside = colors-1;	/* adjust for mono, CGA modes */
if (outside == inside) outside = 0;	/* uhh, ensure that outside != inside */
if (outside == inside) outside = 1;	/* uhh, ensure that outside != inside */

calctype = 1;				/* assume Mandelbrot, unless ... */
if (param[0] != 0.0 || param[1] != 0.0) calctype = 0;

}

testend()	/* this routine is called just after the fractal ends */
{

free(xorbit);			/* free up the temporary arrays */
free(yorbit);

}

testpt(initreal, initimag, parm1, parm2, maxiter, inside)
double initreal,initimag,parm1,parm2;
int maxiter,inside;
{
	register	color;

if (proceed == 0) {		/* oops. not enough memory */
	return(1);
	}

color = outside;
	/* can call either MSetDist or JSetDist here with the same parameters. */

if (calctype == 1) {
	if ( MSetDist(initreal, initimag, parm1, parm2, maxiter ) < delta)
			color = inside;				/* user specified, black by default */
	}
else	{
	if ( JSetDist(initreal, initimag, parm1, parm2, maxiter ) < delta)
			color = inside;				/* user specified, black by default */
	}
	return(color);
}

/* Distance estimator for points near Mandelbrot set						*/
/* Algorithm from Peitgen & Saupe, Science of Fractal Images, p.198 	*/
/* (Variable names from Peitgen's Pascal)										*/

double	MSetDist(double cx, double cy, double parm1, 
						double parm2, int maxiter)
{
	int		iter = 0, i;
	double	big = 100000.0;
	double	overflow = 100000000000000.0;		/* probably this could be smaller */
	int		flag;
	double	x, y, x2, y2;
	double	temp;
	double	xder, yder;
	double	dist;

	x = y = x2 = y2 = dist = xorbit[0] = yorbit[0] = 0.0;

	while ((iter <	maxiter) && (x2 + y2 < big)) {
		temp = x2 - y2 + cx;
		y = 2 * x * y + cy;
		x = temp;
		x2 = x * x;
		y2 = y * y;
		iter++;
		xorbit[ iter ] = x;
		yorbit[ iter ] = y;

	}
	if ( x2 + y2 > big ) {
		xder = yder = 0.0;
		i = 0;
		flag = 0;
		while ((i < iter) && (!flag)) {
			temp = 2 * (xorbit[i] * xder - yorbit[i] * yder) + 1;
			yder = 2 * (yorbit[i] * xder + xorbit[i] * yder);
			xder = temp;
			if ( max( fabs( xder ), fabs( yder )) > overflow ) flag++;
			i++;
		}
		if ( !flag )
			dist = log(x2 + y2) * sqrt(x2 + y2) / sqrt(xder * xder + yder * yder);

	}
	return(dist);
}


/* Distance estimator for points near Julia set										 */
/* Algorithm adapted from Peitgen & Saupe, Science of Fractal Images, p.198 */

double	JSetDist(double cx, double cy, double parm1, 
						double parm2, int maxiter)
{
	int		iter = 0, i;
	double	big = 100000.0;
	double	overflow = 100000000000000.0;		/* probably this could be smaller */
	int		flag;
	double	x, y, x2, y2;
	double	temp;
	double	xder, yder;
	double	dist;

	x = cx;
	y = cy;
	x2 = x * x;
	y2 = y * y;
	dist = xorbit[0] = yorbit[0] = 0.0;
	
	while ((iter <	maxiter) && (x2 + y2 < big)) {
		temp = x2 - y2 + parm1;
		y = 2 * x * y + parm2;
		x = temp;
		x2 = x * x;
		y2 = y * y;
		iter++;
		xorbit[ iter ] = x;
		yorbit[ iter ] = y;

	}
	if ( x2 + y2 > big ) {
		xder = yder = 0.0;
		i = 0;
		flag = 0;
		while ((i < iter) && (!flag)) {
			temp = 2 * (xorbit[i] * xder - yorbit[i] * yder) + 1;
			yder = 2 * (yorbit[i] * xder + xorbit[i] * yder);
			xder = temp;
			if ( max( fabs( xder ), fabs( yder )) > overflow ) flag++;
			i++;
		}
		if ( !flag )
			dist = log(x2 + y2) * sqrt(x2 + y2) / sqrt(xder * xder + yder * yder);

	}
	return(dist);
}


