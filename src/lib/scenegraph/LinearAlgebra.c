/*
=INSERT_TEMPLATE_HERE=

$Id: LinearAlgebra.c,v 1.17 2011/07/18 02:05:45 dug9 Exp $

???

*/

/****************************************************************************
    This file is part of the FreeWRL/FreeX3D Distribution.

    Copyright 2009 CRC Canada. (http://www.crc.gc.ca)

    FreeWRL/FreeX3D is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FreeWRL/FreeX3D is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FreeWRL/FreeX3D.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

#include <math.h>

#include <config.h>
#include <system.h>
#include <display.h>
#include <internal.h>

#include <libFreeWRL.h>

#include "../vrml_parser/Structs.h"
#include "../main/headers.h"

#include "LinearAlgebra.h"

#define DJ_KEEP_COMPILER_WARNING 0

/* Altenate implemetations available, should merge them eventually */
double *vecaddd(double *c, double* a, double *b)
{
	c[0] = a[0] + b[0];
	c[1] = a[1] + b[1];
	c[2] = a[2] + b[2];
	return c;
}
double *vecdifd(double *c, double* a, double *b)
{
	c[0] = a[0] - b[0];
	c[1] = a[1] - b[1];
	c[2] = a[2] - b[2];
	return c;
}
double *veccopyd(double *c, double *a)
{
	c[0] = a[0];
	c[1] = a[1];
	c[2] = a[2];
	return c;
}
double * veccrossd(double *c, double *a, double *b)
{
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
	return c;
}
double veclengthd( double *p )
{
	return sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
}
double vecdotd(double *a, double *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}
double* vecscaled(double* r, double* v, double s)
{
    r[0] = v[0] * s;
    r[1] = v[1] * s;
    r[2] = v[2] * s;
    return r;
}
/* returns vector length, too */
double vecnormald(double *r, double *v)
{
    double ret = sqrt(vecdotd(v,v));
    /* if(ret == 0.) return 0.; */
    if (APPROX(ret, 0)) return 0.;
    vecscaled(r,v,1./ret);
    return ret;
}

void veccross(struct point_XYZ *c, struct point_XYZ a, struct point_XYZ b)
{
    c->x = a.y * b.z - a.z * b.y;
    c->y = a.z * b.x - a.x * b.z;
    c->z = a.x * b.y - a.y * b.x;
}

float veclength( struct point_XYZ p )
{
    return (float) sqrt(p.x*p.x + p.y*p.y + p.z*p.z);
}


double vecangle(struct point_XYZ* V1, struct point_XYZ* V2) {
    return acos((V1->x*V2->x + V1->y*V2->y +V1->z*V2->z) /
		sqrt( (V1->x*V1->x + V1->y*V1->y + V1->z*V1->z)*(V2->x*V2->x + V2->y*V2->y + V2->z*V2->z) )  );
};


float calc_angle_between_two_vectors(struct point_XYZ a, struct point_XYZ b)
{
    float length_a, length_b, scalar, temp;
    scalar = (float) calc_vector_scalar_product(a,b);
    length_a = calc_vector_length(a);
    length_b = calc_vector_length(b);

    /* printf("scalar: %f  length_a: %f  length_b: %f \n", scalar, length_a, length_b);*/

    /* if (scalar == 0) */
    if (APPROX(scalar, 0)) {
		return (float) (M_PI/2.0);
    }

    if ( (length_a <= 0)  || (length_b <= 0)){
	printf("Divide by 0 in calc_angle_between_two_vectors():  No can do! \n");
	return 0;
    }

    temp = scalar /(length_a * length_b);
    /*  printf("temp: %f", temp);*/

    /*acos() appears to be unable to handle 1 and -1  */
    /* fixed to handle border case where temp <=-1.0 for 0.39 JAS */
    if ((temp >= 1) || (temp <= -1)){
	if (temp < 0.0f) return 3.141526f;
	return 0.0f;
    }
    return (float) acos(temp);
}

/* returns vector length, too */
GLDOUBLE vecnormal(struct point_XYZ*r, struct point_XYZ* v)
{
    GLDOUBLE ret = sqrt(vecdot(v,v));
    /* if(ret == 0.) return 0.; */
    if (APPROX(ret, 0)) return 0.;
    vecscale(r,v,1./ret);
    return ret;
}


/*will add functions here as needed. */

GLDOUBLE det3x3(GLDOUBLE* data)
{
    return -data[1]*data[10]*data[4] +data[0]*data[10]*data[5] -data[2]*data[5]*data[8] +data[1]*data[6]*data[8] +data[2]*data[4]*data[9] -data[0]*data[6]*data[9];
}

struct point_XYZ* transform(struct point_XYZ* r, const struct point_XYZ* a, const GLDOUBLE* b)
{
    struct point_XYZ tmp; /* JAS*/

    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z +b[12];
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z +b[13];
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z +b[14];
    } else {
	/* JAS was  - struct point_XYZ tmp = {a->x,a->y,a->z};*/
	tmp.x = a->x; tmp.y = a->y; tmp.z = a->z;
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z +b[12];
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z +b[13];
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z +b[14];
    }
    return r;
}

float* transformf(float* r, const float* a, const GLDOUBLE* b)
{
    float tmp[3];  /* JAS*/

    if(r != a) { /*protect from self-assignments */
	r[0] = (float) (b[0]*a[0] +b[4]*a[1] +b[8]*a[2] +b[12]);
	r[1] = (float) (b[1]*a[0] +b[5]*a[1] +b[9]*a[2] +b[13]);
	r[2] = (float) (b[2]*a[0] +b[6]*a[1] +b[10]*a[2] +b[14]);
    } else {
	tmp[0] =a[0]; tmp[1] = a[1]; tmp[2] = a[2]; /* JAS*/
	r[0] = (float) (b[0]*tmp[0] +b[4]*tmp[1] +b[8]*tmp[2] +b[12]);
	r[1] = (float) (b[1]*tmp[0] +b[5]*tmp[1] +b[9]*tmp[2] +b[13]);
	r[2] = (float) (b[2]*tmp[0] +b[6]*tmp[1] +b[10]*tmp[2] +b[14]);
    }
    return r;
}
/*transform point, but ignores translation.*/
struct point_XYZ* transform3x3(struct point_XYZ* r, const struct point_XYZ* a, const GLDOUBLE* b)
{
    struct point_XYZ tmp;

    if(r != a) { /*protect from self-assignments */
	r->x = b[0]*a->x +b[4]*a->y +b[8]*a->z;
	r->y = b[1]*a->x +b[5]*a->y +b[9]*a->z;
	r->z = b[2]*a->x +b[6]*a->y +b[10]*a->z;
    } else {
	/* JAS struct point_XYZ tmp = {a->x,a->y,a->z};*/
	tmp.x = a->x; tmp.y = a->y; tmp.z = a->z;
	r->x = b[0]*tmp.x +b[4]*tmp.y +b[8]*tmp.z;
	r->y = b[1]*tmp.x +b[5]*tmp.y +b[9]*tmp.z;
	r->z = b[2]*tmp.x +b[6]*tmp.y +b[10]*tmp.z;
    }
    return r;
}

struct point_XYZ* vecscale(struct point_XYZ* r, struct point_XYZ* v, GLDOUBLE s)
{
    r->x = v->x * s;
    r->y = v->y * s;
    r->z = v->z * s;
    return r;
}

double vecdot(struct point_XYZ* a, struct point_XYZ* b)
{
    return (a->x*b->x) + (a->y*b->y) + (a->z*b->z);
}


/* returns 0 if p1 is closest, 1 if p2 is closest, and a fraction if the closest point is in between */
/* To get the closest point, use pclose = retval*p1 + (1-retval)p2; */
/* y1 must be smaller than y2 */
/*double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct point_XYZ p1, struct point_XYZ p2) {
  double imin = (y1- p1.y) / (p2.y - p1.y);
  double imax = (y2- p1.y) / (p2.y - p1.y);

  double x21 = (p2.x - p1.x);
  double z21 = (p2.z - p1.z);
  double i = (p2.x * x21 + p2.z * z21) /
  ( x21*x21 + z21*z21 );
  return max(min(i,imax),imin);

  }*/

double closest_point_of_segment_to_y_axis_segment(double y1, double y2, struct point_XYZ p1, struct point_XYZ p2) {
    /*cylinder constraints (to be between y1 and y2) */
    double imin = (y1- p1.y) / (p2.y - p1.y);
    double imax = (y2- p1.y) / (p2.y - p1.y);

    /*the equation */
    double x12 = (p1.x - p2.x);
    double z12 = (p1.z - p2.z);
    double q = ( x12*x12 + z12*z12 );

    /* double i = ((q == 0.) ? 0 : (p1.x * x12 + p1.z * z12) / q); */
    double i = ((APPROX(q, 0)) ? 0 : (p1.x * x12 + p1.z * z12) / q);

    printf("imin=%f, imax=%f => ",imin,imax);

    if(imin > imax) {
	double tmp = imax;
	imax = imin;
	imin = tmp;
    }

    /*clamp constraints to segment*/
    if(imin < 0) imin = 0;
    if(imax > 1) imax = 1;

    printf("imin=%f, imax=%f\n",imin,imax);

    /*clamp result to constraints */
    if(i < imin) i = imin;
    if(i > imax) i = imax;
    return i;

}

struct point_XYZ* vecadd(struct point_XYZ* r, struct point_XYZ* v, struct point_XYZ* v2)
{
    r->x = v->x + v2->x;
    r->y = v->y + v2->y;
    r->z = v->z + v2->z;
    return r;
}

struct point_XYZ* vecdiff(struct point_XYZ* r, struct point_XYZ* v, struct point_XYZ* v2)
{
    r->x = v->x - v2->x;
    r->y = v->y - v2->y;
    r->z = v->z - v2->z;
    return r;
}

/*i,j,n will form an orthogonal vector space */
void make_orthogonal_vector_space(struct point_XYZ* i, struct point_XYZ* j, struct point_XYZ n) {
    /* optimal axis finding algorithm. the solution isn't unique.*/
    /*  each of these three calculations doesn't work (or works poorly)*/
    /*  in certain distinct cases. (gives zero vectors when two axes are 0)*/
    /*  selecting the calculations according to smallest axis avoids this problem.*/
    /*  (the two remaining axis are thus far from zero, if n is normal)*/
    if(fabs(n.x) <= fabs(n.y) && fabs(n.x) <= fabs(n.z)) { /* x smallest*/
	i->x = 0;
	i->y = n.z;
	i->z = -n.y;
	vecnormal(i,i);
	j->x = n.y*n.y + n.z*n.z;
	j->y = (-n.x)*n.y;
	j->z = (-n.x)*n.z;
    } else if(fabs(n.y) <= fabs(n.x) && fabs(n.y) <= fabs(n.z)) { /* y smallest*/
	i->x = -n.z;
	i->y = 0;
	i->z = n.x;
	vecnormal(i,i);
	j->x = (-n.x)*n.y;
	j->y = n.x*n.x + n.z*n.z;
	j->z = (-n.y)*n.z;
    } else { /* z smallest*/
	i->x = n.y;
	i->y = -n.x;
	i->z = 0;
	vecnormal(i,i);
	j->x = (-n.x)*n.z;
	j->y = (-n.y)*n.z;
	j->z =  n.x*n.x + n.y*n.y;
    }
}


GLDOUBLE* mattranspose(GLDOUBLE* res, GLDOUBLE* mm)
{
	GLDOUBLE mcpy[16];
	int i, j;
	GLDOUBLE *m;

	m = mm;
	if(res == m) {
		memcpy(mcpy,m,sizeof(GLDOUBLE)*16);
		m = mcpy;
	}

	for (i = 0; i < 4; i++) {
		//for (j = i + 1; j < 4; j++) {
		//	res[i*4+j] = m[j*4+i];
		//}
		for (j = 0; j < 4; j++) {
			res[i*4+j] = m[j*4+i];
		}
	}
	return res;
}


GLDOUBLE* matinverse(GLDOUBLE* res, GLDOUBLE* mm)
{
    double Deta;
    GLDOUBLE mcpy[16];
	GLDOUBLE *m;

	m = mm;
    if(res == m) {
	memcpy(mcpy,m,sizeof(GLDOUBLE)*16);
	m = mcpy;
    }

    Deta = det3x3(m);

    res[0] = (-m[9]*m[6] +m[5]*m[10])/Deta;
    res[4] = (+m[8]*m[6] -m[4]*m[10])/Deta;
    res[8] = (-m[8]*m[5] +m[4]*m[9])/Deta;
    res[12] = ( m[12]*m[9]*m[6] -m[8]*m[13]*m[6] -m[12]*m[5]*m[10] +m[4]*m[13]*m[10] +m[8]*m[5]*m[14] -m[4]*m[9]*m[14])/Deta;

    res[1] = (+m[9]*m[2] -m[1]*m[10])/Deta;
    res[5] = (-m[8]*m[2] +m[0]*m[10])/Deta;
    res[9] = (+m[8]*m[1] -m[0]*m[9])/Deta;
    res[13] = (-m[12]*m[9]*m[2] +m[8]*m[13]*m[2] +m[12]*m[1]*m[10] -m[0]*m[13]*m[10] -m[8]*m[1]*m[14] +m[0]*m[9]*m[14])/Deta;

    res[2] = (-m[5]*m[2] +m[1]*m[6])/Deta;
    res[6] = (+m[4]*m[2] -m[0]*m[6])/Deta;
    res[10] = (-m[4]*m[1] +m[0]*m[5])/Deta;
    res[14] = ( m[12]*m[5]*m[2] -m[4]*m[13]*m[2] -m[12]*m[1]*m[6] +m[0]*m[13]*m[6] +m[4]*m[1]*m[14] -m[0]*m[5]*m[14])/Deta;

    res[3] = (+m[5]*m[2]*m[11] -m[1]*m[6]*m[11])/Deta;
    res[7] = (-m[4]*m[2]*m[11] +m[0]*m[6]*m[11])/Deta;
    res[11] = (+m[4]*m[1]*m[11] -m[0]*m[5]*m[11])/Deta;
    res[15] = (-m[8]*m[5]*m[2] +m[4]*m[9]*m[2] +m[8]*m[1]*m[6] -m[0]*m[9]*m[6] -m[4]*m[1]*m[10] +m[0]*m[5]*m[10])/Deta;

    return res;
}

struct point_XYZ* polynormal(struct point_XYZ* r, struct point_XYZ* p1, struct point_XYZ* p2, struct point_XYZ* p3) {
    struct point_XYZ v1;
    struct point_XYZ v2;
    VECDIFF(*p2,*p1,v1);
    VECDIFF(*p3,*p1,v2);
    veccross(r,v1,v2);
    vecnormal(r,r);
    return r;
}

/*simple wrapper for now. optimize later */
struct point_XYZ* polynormalf(struct point_XYZ* r, float* p1, float* p2, float* p3) {
    struct point_XYZ pp[3];
    pp[0].x = p1[0];
    pp[0].y = p1[1];
    pp[0].z = p1[2];
    pp[1].x = p2[0];
    pp[1].y = p2[1];
    pp[1].z = p2[2];
    pp[2].x = p3[0];
    pp[2].y = p3[1];
    pp[2].z = p3[2];
    return polynormal(r,pp+0,pp+1,pp+2);
}


GLDOUBLE* matrotate(GLDOUBLE* Result, double Theta, double x, double y, double z)
{
    GLDOUBLE CosTheta = cos(Theta);
    GLDOUBLE SinTheta = sin(Theta);

    Result[0] = x*x + (y*y+z*z)*CosTheta;
    Result[1] = x*y - x*y*CosTheta + z*SinTheta;
    Result[2] = x*z - x*z*CosTheta - y*SinTheta;
    Result[4] = x*y - x*y*CosTheta - z*SinTheta;
    Result[5] = y*y + (x*x+z*z)*CosTheta;
    Result[6] = z*y - z*y*CosTheta + x*SinTheta;
    Result[8] = z*x - z*x*CosTheta + y*SinTheta;
    Result[9] = z*y - z*y*CosTheta - x*SinTheta;
    Result[10]= z*z + (x*x+y*y)*CosTheta;
    Result[3] = 0;
    Result[7] = 0;
    Result[11] = 0;
    Result[12] = 0;
    Result[13] = 0;
    Result[14] = 0;
    Result[15] = 1;

    return Result;
}

GLDOUBLE* mattranslate(GLDOUBLE* r, double dx, double dy, double dz)
{
    r[0] = r[5] = r[10] = r[15] = 1;
    r[1] = r[2] = r[3] = r[4] =
	r[6] = r[7] = r[8] = r[9] =
	r[11] = 0;
    r[12] = dx;
    r[13] = dy;
    r[14] = dz;
    return r;
}

GLDOUBLE* matmultiply(GLDOUBLE* r, GLDOUBLE* mm , GLDOUBLE* nn)
{
    GLDOUBLE tm[16],tn[16];
	GLDOUBLE *m, *n;
	int i,j,k;
    /* prevent self-multiplication problems.*/
	m = mm;
	n = nn;
    if(r == m) {
	memcpy(tm,m,sizeof(GLDOUBLE)*16);
	m = tm;
    }
    if(r == n) {
	memcpy(tn,n,sizeof(GLDOUBLE)*16);
	n = tn;
    }
	/* assume 4x4 homgenous transform */
	for(i=0;i<4;i++)
		for(j=0;j<4;j++)
		{
			r[i*4+j] = 0.0;
			for(k=0;k<4;k++)
				r[i*4+j] += m[i*4+k]*n[k*4+j];
		}
	return r;
	/* this method ignors the perspectives */
    r[0] = m[0]*n[0]+m[4]*n[1]+m[8]*n[2];
    r[4] = m[0]*n[4]+m[4]*n[5]+m[8]*n[6];
    r[8] = m[0]*n[8]+m[4]*n[9]+m[8]*n[10];
    r[12]= m[0]*n[12]+m[4]*n[13]+m[8]*n[14]+m[12];

    r[1] = m[1]*n[0]+m[5]*n[1]+m[9]*n[2];
    r[5] = m[1]*n[4]+m[5]*n[5]+m[9]*n[6];
    r[9] = m[1]*n[8]+m[5]*n[9]+m[9]*n[10];
    r[13]= m[1]*n[12]+m[5]*n[13]+m[9]*n[14]+m[13];

    r[2] = m[2]*n[0]+m[6]*n[1]+m[10]*n[2];
    r[6] = m[2]*n[4]+m[6]*n[5]+m[10]*n[6];
    r[10]= m[2]*n[8]+m[6]*n[9]+m[10]*n[10];
    r[14]= m[2]*n[12]+m[6]*n[13]+m[10]*n[14]+m[14];

    return r;
}
void rotate_v2v_axisAngled(double* axis, double* angle, double *orig, double *result)
{
    double cvl;
	double dv[3], iv[3], cv[3];

	/* step 1 get sin of angle between 2 vectors using cross product rule: ||u x v|| = ||u||*||v||*sin(theta) */
    vecnormald(dv,orig); /*normalizes vector to unit length U -> u^ (length 1) */
    vecnormald(iv,result);

    veccrossd(cv,dv,iv); /*the axis of rotation cv = dv X iv*/
    cvl = vecnormald(cv,cv); /* cvl = ||u x v|| / ||u^||*||v^|| = ||u x v|| = sin(theta)*/
	veccopyd(axis,cv);

    /* if(cvl == 0) { */
    if(APPROX(cvl, 0)) {
		cv[2] = 1.0;
	}
	/* step 2 get cos of angle between 2 vectors using dot product rule: u dot v = ||u||*||v||*cos(theta) or cos(theta) = u dot v/( ||u|| ||v||) 
	   or, since U,V already unit length from being normalized, cos(theta) = u dot v */
    *angle = atan2(cvl,vecdotd(dv,iv)); /*the angle theta = arctan(rise/run) = atan2(sin_theta,cos_theta) in radians*/
}
/*puts dv back on iv - return the 4x4 rotation matrix that will rotate vector dv onto iv*/
double matrotate2v(GLDOUBLE* res, struct point_XYZ iv/*original*/, struct point_XYZ dv/*result*/) {
    struct point_XYZ cv;
    double cvl,a;

	/* step 1 get sin of angle between 2 vectors using cross product rule: ||u x v|| = ||u||*||v||*sin(theta) */
    vecnormal(&dv,&dv); /*normalizes vector to unit length U -> u^ (length 1) */
    vecnormal(&iv,&iv);

    veccross(&cv,dv,iv); /*the axis of rotation cv = dv X iv*/
    cvl = vecnormal(&cv,&cv); /* cvl = ||u x v|| / ||u^||*||v^|| = ||u x v|| = sin(theta)*/
    /* if(cvl == 0) { */
    if(APPROX(cvl, 0)) {
		cv.z = 1;
	}
	/* step 2 get cos of angle between 2 vectors using dot product rule: u dot v = ||u||*||v||*cos(theta) or cos(theta) = u dot v/( ||u|| ||v||) 
	   or, since U,V already unit length from being normalized, cos(theta) = u dot v */
    a = atan2(cvl,vecdot(&dv,&iv)); /*the angle theta = arctan(rise/run) = atan2(sin_theta,cos_theta) in radians*/
    /* step 3 convert rotation angle around unit directional vector of rotation into an equivalent rotation matrix 4x4 */
    matrotate(res,a,cv.x,cv.y,cv.z);
    return a;
}




#ifdef COMMENT
/*fast crossproduct using references, that checks for auto-assignments */
GLDOUBLE* veccross(GLDOUBLE* r, GLDOUBLE* v1, GLDOUBLE* v2)
{
    /*check against self-assignment. */
    if(r != v1) {
	if(r != v2) {
	    r[0] = v1[1]*v2[2] - v1[2]*v2[1];
	    r[1] = v1[2]*v2[0] - v1[0]*v2[2];
	    r[2] = v1[0]*v2[1] - v1[1]*v2[0];
	} else { /* r == v2 */
	    GLDOUBLE v2c[3] = {v2[0],v2[1],v2[2]};
	    r[0] = v1[1]*v2c[2] - v1[2]*v2c[1];
	    r[1] = v1[2]*v2c[0] - v1[0]*v2c[2];
	    r[2] = v1[0]*v2c[1] - v1[1]*v2c[0];
	}
    } else { /* r == v1 */
	GLDOUBLE v1c[3] = {v1[0],v1[1],v1[2]};
	r[0] = v1c[1]*v2[2] - v1c[2]*v2[1];
	r[1] = v1c[2]*v2[0] - v1c[0]*v2[2];
	r[2] = v1c[0]*v2[1] - v1c[1]*v2[0];
    }
    return r;
}



#endif


/*
 * apply a scale to the matrix given. Assumes matrix is valid... 
 *
 */
	
void scale_to_matrix (double *mat, struct point_XYZ *scale) {
/* copy the definitions from quaternion.h... */
#define MAT00 mat[0]
#define MAT01 mat[1]
#define MAT02 mat[2]
#if DJ_KEEP_COMPILER_WARNING
#define MAT03 mat[3]
#endif
#define MAT10 mat[4]
#define MAT11 mat[5]
#define MAT12 mat[6]
#if DJ_KEEP_COMPILER_WARNING
#define MAT13 mat[7]
#endif
#define MAT20 mat[8]
#define MAT21 mat[9]
#define MAT22 mat[10]
#if DJ_KEEP_COMPILER_WARNING
#define MAT23 mat[11]
#define MAT30 mat[12]
#define MAT31 mat[13]
#define MAT32 mat[14]
#define MAT33 mat[15]
#endif

	MAT00 *=scale->x;
	MAT01 *=scale->x;
	MAT02 *=scale->x;
	MAT10 *=scale->y;
	MAT11 *=scale->y;
	MAT12 *=scale->y;
	MAT20 *=scale->z;
	MAT21 *=scale->z;
	MAT22 *=scale->z;
}

static double identity[] = { 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0 };

void loadIdentityMatrix (double *mat) {
        memcpy((void *)mat, (void *)identity, sizeof(double)*16);
}

void point_XYZ_slerp(struct point_XYZ *ret, struct point_XYZ *p1, struct point_XYZ *p2, const double t)
{
	//not tested as of July16,2011
	//goal start slow, speed up in the middle, and slow down when stopping 
	// (like a sine or cosine wave)
	//let omega = t*pi 
	//then cos omega goes from 1 to -1 natively
	//we want scale0 to go from 1 to 0
	//scale0 = .5(1+cos(t*pi)) should be in the 1 to 0 range,
	//and be 'fastest' in the middle ie at pi/2 
	//then scale1 = 1 - scale0
	double scale0, scale1, omega;

	/* calculate coefficients */
	if ( t > .05 || t < .95 ) {
		/* standard case (SLERP) */
		omega = t*PI;
		scale0 = 0.5*(1.0 + cos(omega));
		scale1 = 1.0 - scale0;
	} else {
		/* p1 & p2 are very close, so do linear interpolation */
		scale0 = 1.0 - t;
		scale1 = t;
	}
	ret->x = scale0 * p1->x + scale1 * p2->x;
	ret->y = scale0 * p1->y + scale1 * p2->y;
	ret->z = scale0 * p1->z + scale1 * p2->z;
}

void general_slerp(double *ret, double *p1, double *p2, int size, const double t)
{
	//not tested as of July16,2011
	//goal start slow, speed up in the middle, and slow down when stopping 
	// (like a sine or cosine wave)
	//let omega = t*pi 
	//then cos omega goes from 1 to -1 natively
	//we want scale0 to go from 1 to 0
	//scale0 = .5(1+cos(t*pi)) should be in the 1 to 0 range,
	//and be 'fastest' in the middle ie at pi/2 
	//then scale1 = 1 - scale0
	double scale0, scale1, omega;
	int i;
	/* calculate coefficients */
	if (0) {
	//if ( t > .05 || t < .95 ) {
		/* standard case (SLERP) */
		omega = t*PI;
		scale0 = 0.5*(1.0 + cos(omega));
		scale1 = 1.0 - scale0;
	} else {
		/* p1 & p2 are very close, so do linear interpolation */
		scale0 = 1.0 - t;
		scale1 = t;
	}
	for(i=0;i<size;i++)
		ret[i] = scale0 * p1[i] + scale1 * p2[i];
}