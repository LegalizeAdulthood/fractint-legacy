/*
A word about the 3D library. Even though this library supports 
three dimensions, the matrices are 4x4 for the following reason. 
With normal 3 dimensional vectors, translation is an ADDITION, 
and rotation is a MULTIPLICATION. A vector {x,y,z} is represented 
as a 4-tuple {x,y,z,1}. It is then possible to define a 4x4 
matrix such that multiplying the vector by the matrix translates 
the vector. This allows combinations of translation and rotation 
to be obtained in a single matrix by multiplying a translation 
matrix and a rotation matrix together. Note that in the code, 
vectors have three components; since the fourth component is 
always 1, that value is not included in the vector variable to 
save space, but the routines make use of the fourth component 
(see vec_mult()). Similarly, the fourth column of EVERY matrix is
always
         0
         0
         0
         1
but currently the C version of a matrix includes this even though 
it could be left out of the data structure and assumed in the 
routines. Vectors are ROW vectors, and are always multiplied with 
matrices FROM THE LEFT (e.g. vector*matrix). Also note the order 
of indices of a matrix is matrix[row][column], and in usual C 
fashion, numbering starts with 0.

TRANSLATION MATRIX =  1     0     0     0
                      0     1     0     0
                      0     0     1     0
                      Tx    Ty    Tz    1

SCALE MATRIX =        Sx    0     0     0
                      0     Sy    0     0
                      0     0     Sz    0
                      0     0     0     1

Rotation about x axis i degrees:
ROTX(é) =             1     0     0     0
                      0   cosé  siné    0
                      0  -siné  cosé    0
                      0     0     0     1

Rotation about y axis i degrees:
ROTY(é) =           cosé    0  -siné    0
                      0     1     0     0
                    siné    0   cosé    0
                      0     0     0     1

Rotation about z axis i degrees:
ROTZ(é) =           cosé  siné    0     0
                   -siné  cosé    0     0
                      0     0     1     0 
                      0     0     0     1

                      --  Tim Wegner  April 22, 1989
*/

#include <stdio.h>
#include <math.h>
#include "fractint.h"

/* initialize a matrix and set to identity matrix 
   (all 0's, 1's on diagonal) */
void identity(MATRIX m)
{  
   int i,j;
   for(i=0;i<CMAX;i++)
   for(j=0;j<RMAX;j++)
      if(i==j)
         m[j][i] = 1.0;
      else
          m[j][i] = 0.0;
}

/* Multiply two matrices */
void mat_mul(MATRIX mat1, MATRIX mat2, MATRIX mat3)
{
     /* result stored in MATRIX new to avoid problems
        in case parameter mat3 == mat2 or mat 1 */
     MATRIX new;
     int i,j;
     for(i=0;i<4;i++)
     for(j=0;j<4;j++)
        new[j][i] =  mat1[j][0]*mat2[0][i]+
                     mat1[j][1]*mat2[1][i]+
                     mat1[j][2]*mat2[2][i]+
                     mat1[j][3]*mat2[3][i];
                     
     /* should replace this with memcpy */
     for(i=0;i<4;i++)
     for(j=0;j<4;j++)
        mat3[j][i] =  new[j][i];

}

/* multiply a matrix by a scalar */
void scale (double sx, double sy, double sz, MATRIX m)
{
   MATRIX scale;
   identity(scale);
   scale[0][0] = sx;
   scale[1][1] = sy;
   scale[2][2] = sz;
   mat_mul(m,scale,m);
}

/* rotate about X axis  */
void xrot (double theta, MATRIX m)
{
   MATRIX rot;
   double sintheta,costheta;
   sintheta = sin(theta);
   costheta = cos(theta);
   identity(rot);
   rot[1][1] = costheta;
   rot[1][2] = -1.0*sintheta;
   rot[2][1] = sintheta;
   rot[2][2] = costheta;
   mat_mul(m,rot,m);
}

/* rotate about Y axis  */
void yrot (double theta, MATRIX m)
{
   MATRIX rot;
   double sintheta,costheta;
   sintheta = sin(theta);
   costheta = cos(theta);
   identity(rot);
   rot[0][0] = costheta;
   rot[0][2] = sintheta;
   rot[2][0] = -1.0*sintheta;
   rot[2][2] = costheta;
   mat_mul(m,rot,m);
}

/* rotate about Z axis  */
void zrot (double theta, MATRIX m)
{
   MATRIX rot;
   double sintheta,costheta;
   sintheta = sin(theta);
   costheta = cos(theta);
   identity(rot);
   rot[0][0] = costheta;
   rot[0][1] = -1.0*sintheta;
   rot[1][0] = sintheta;
   rot[1][1] = costheta;
   mat_mul(m,rot,m);
}

/* translate  */
void trans (double tx, double ty, double tz, MATRIX m)
{
   MATRIX trans;
   identity(trans);
   trans[3][0] = tx;
   trans[3][1] = ty;
   trans[3][2] = tz;
   mat_mul(m,trans,m);
}

/* cross product  - useful because cross is perpendicular to v and w */
int cross_product (VECTOR v, VECTOR w, VECTOR cross)
{
   VECTOR tmp;
   tmp[0] =  v[1]*w[2] - w[1]*v[2];
   tmp[1] =  w[0]*v[2] - v[0]*w[2];
   tmp[2] =  v[0]*w[1] - w[0]*v[1];
   cross[0] = tmp[0];
   cross[1] = tmp[1];
   cross[2] = tmp[2];
   return(0);
}
/* cross product integer arguments (not fudged) */
int icross_product (IVECTOR v, IVECTOR w, IVECTOR cross)
{
   IVECTOR tmp;
   tmp[0] =  v[1]*w[2] - w[1]*v[2];
   tmp[1] =  w[0]*v[2] - v[0]*w[2];
   tmp[2] =  v[0]*w[1] - w[0]*v[1];
   cross[0] = tmp[0];
   cross[1] = tmp[1];
   cross[2] = tmp[2];
   return(0);
}

/* multiply source vector s by matrix m, result in target t */
/* used to apply transformations to a vector */
int vmult(s,m,t)
VECTOR s,t;
MATRIX m;
{
   VECTOR tmp;
   int i,j;
   for(j=0;j<CMAX-1;j++)
   {
      tmp[j] = 0.0;
      for(i=0;i<RMAX-1;i++)
         tmp[j] += s[i]*m[i][j];
      /* vector is really four dimensional with last component always 1 */   
      tmp[j] += m[3][j];
   }
   /* set target = tmp. Necessary to use tmp in case source = target */
   for(i=0;i<DIM;i++)
      t[i] = tmp[i];
}

perspective(v)
VECTOR v;
{
   extern VECTOR view; /* viewer's eye in screen coordinates */
   double denom;
   denom = view[2] - v[2];
   if(denom > -0.00001)
   {
      v[0] = -1.0;   /* clipping will catch these values */
      v[1] = -1.0;   /* so they won't plot */
      v[2] = -1.0;
      return(-1);
   }   
   v[0] = (v[0]*view[2] - view[0]*v[2])/denom;
   v[1] = (v[1]*view[2] - view[1]*v[2])/denom;

   /* Not using Z now, but if we ever want it, this is it. We might
      want it for hidden surfaces later */
   /* v[2] =  v[2]/denom;*/
   return(0);
}

/* quick-and-dirty LONG INTEGER version of mult - requires 'multiply()' */
/* (also, we've added a perspective 3D vector */
/* TW 7/10/89 - added a parameter t0 so I can get at the pre-perspective
   vector for illumination model purposes. t0[0] also used as a flag to
   indicate whether z is needed */ 
long multiply(long x, long y, int n);

int ivmult(s,m,t0,t,iview)
long s[4],t[4],t0[4],iview[4];
long m[4][4];
{
   long tmp[4];
   int i,j, k;

   k = CMAX-1;			/* shorten the math if non-perspective and non-illum */
   if (iview[2] == 0 && t0[0] == 0) k--;

   for(j=0;j<k;j++)
   {
      tmp[j] = 0;
      for(i=0;i<RMAX-1;i++)
         tmp[j] += multiply(s[i],m[i][j],16);
      /* vector is really four dimensional with last component always 1 */   
      tmp[j] += m[3][j];
   }
   if(t0[0])
   {
      /* faster than for loop, if less general */
      t0[0] = tmp[0];
      t0[1] = tmp[1];
      t0[2] = tmp[2];
   }
   if (iview[2] != 0)		/* perspective 3D */
   {
        /* sorry, guys, but we are going to "lie" to the multiply routine
           on order to get integer perspective perspective 3D to work without
           overflowing.  We're getting really "shifty", here. */
      long denom;
      denom = (iview[2] - tmp[2]) >> 14;
      if (denom >= 0) 		/* bail out if point is "behind" us */
      {
         /* BERT! is -1 right bailout value? */
         tmp[0] = -1;
         tmp[1] = -1;
         tmp[2] = -1;
      } 
      else 
      {
         tmp[0] = (multiply(tmp[0], iview[2], 30) -
            multiply(iview[0], tmp[2], 30)) / denom;
         tmp[0] = tmp[0] << 16;
         tmp[1] = (multiply(tmp[1], iview[2], 30) -
		    multiply(iview[1], tmp[2], 30)) / denom;
	     tmp[1] = tmp[1] << 16;
/*
        tmp[2] = iview[2] / denom;
        tmp[2] = tmp[2] << 16;
*/
      }
   }

   /* set target = tmp. Necessary to use tmp in case source = target */
   /* faster than for loop, if less general */
   t[0] = tmp[0];
   t[1] = tmp[1];
   t[2] = tmp[2];
}

ipersp(iv,iview)
long iv[4], iview[4];
{
        long denom;
        denom = (iview[2] - iv[2]) >> 14;
     if (denom >= 0) {		/* bail out if point is "behind" us */
        iv[0] = -1;
        iv[1] = -1;
        iv[2] = -1;
     } else {
        iv[0] = (multiply(iv[0], iview[2], 30) -
		  multiply(iview[0], iv[2], 30)) / denom;
	iv[0] = iv[0] << 16;
        iv[1] = (multiply(iv[1], iview[2], 30) -
		  multiply(iview[1], iv[2], 30)) / denom;
	iv[1] = iv[1] << 16;
/*
        iv[2] = iview[2] / denom;
        iv[2] = iv[2] << 16;
*/
     }

}
/* normalize a vector (to length 1) */
normalize_vector(VECTOR v)
{
    double vlength;
    vlength = dot_product(v,v);
    /* bailout if zero vlength */
    if(!(vlength > 0.0)) return(-1);
    vlength = sqrt(vlength);
    if(!(vlength > 0.0)) return(-1);
    v[0] /= vlength;
    v[1] /= vlength;
    v[2] /= vlength;
    return(0);
}
