#include "Triangle.hpp"
#include "AABB.hpp"
#include <cmath>

Vector3 AABB::getCenter() const {
    return Vector3((min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f, (min.z + max.z) / 2.0f);
}


float AABB::getVolume() const {
    Vector3 dist = max - min;
    return dist.x * dist.y * dist.z;
}


void AABB::_getMinMax(float a, float b, float c, float &min, float &max){
    min = max = a;
	if(b < min) min = b;
	if(b > max) max = b;
	if(c < min) min = c;
	if(c > max) max = c;
}


#define AXISTEST_X01(a, b, fa, fb)\
	p0 = a*v0.y - b*v0.z;\
	p2 = a*v2.y - b*v2.z;\
	if(p0<p2) {minTri=p0; maxTri=p2;} else {minTri=p2; maxTri=p0;}\
	rad = fa * boxHalfSize.y + fb * boxHalfSize.z;\
	if(minTri>rad || maxTri<-rad) return 0;

#define AXISTEST_X2(a, b, fa, fb)\
	p0 = a*v0.y - b*v0.z;\
	p1 = a*v1.y - b*v1.z;\
	if(p0<p1) {minTri=p0; maxTri=p1;} else {minTri=p1; maxTri=p0;}\
	rad = fa * boxHalfSize.y + fb * boxHalfSize.z;\
	if(minTri>rad || maxTri<-rad) return 0;

/*======================== Y-tests ========================*/

#define AXISTEST_Y02(a, b, fa, fb)\
	p0 = -a*v0.x + b*v0.z;\
	p2 = -a*v2.x + b*v2.z;\
	if(p0<p2) {minTri=p0; maxTri=p2;} else {minTri=p2; maxTri=p0;}\
	rad = fa * boxHalfSize.x + fb * boxHalfSize.z;\
	if(minTri>rad || maxTri<-rad) return 0;

#define AXISTEST_Y1(a, b, fa, fb)\
	p0 = -a*v0.x + b*v0.z;\
	p1 = -a*v1.x + b*v1.z;\
	if(p0<p1) {minTri=p0; maxTri=p1;} else {minTri=p1; maxTri=p0;}\
	rad = fa * boxHalfSize.x + fb * boxHalfSize.z;\
	if(minTri>rad || maxTri<-rad) return 0;
/*======================== Z-tests ========================*/

#define AXISTEST_Z12(a, b, fa, fb)\
	p1 = a*v1.x - b*v1.y;\
	p2 = a*v2.x - b*v2.y;\
	if(p2<p1) {minTri=p2; maxTri=p1;} else {minTri=p1; maxTri=p2;}\
	rad = fa * boxHalfSize.x + fb * boxHalfSize.y;\
	if(minTri>rad || maxTri<-rad) return 0;


#define AXISTEST_Z0(a, b, fa, fb)\
	p0 = a*v0.x - b*v0.y;\
	p1 = a*v1.x - b*v1.y;\
	if(p0<p1) {minTri=p0; maxTri=p1;} else {minTri=p1; maxTri=p0;}\
	rad = fa * boxHalfSize.x + fb * boxHalfSize.y;\
	if(minTri>rad || maxTri<-rad) return 0;


bool PlaneBoxOverlap(Vector3 normal, Vector3 vert, Vector3 maxbox){

	Vector3 vmin, vmax;

    if(normal.x > 0){
        vmin.x = -maxbox.x - vert.x;
        vmax.x = maxbox.x - vert.x;
    }
    else{
        vmin.x = maxbox.x - vert.x;
        vmax.x = -maxbox.x - vert.x;
    }


    if(normal.y > 0){
        vmin.y = -maxbox.y - vert.y;
        vmax.y = maxbox.y - vert.y;
    }
    else{
        vmin.y = maxbox.y - vert.y;
        vmax.y = -maxbox.y - vert.y;
    }


    if(normal.z > 0){
        vmin.z = -maxbox.z - vert.z;
        vmax.z = maxbox.z - vert.z;
    }
    else{
        vmin.z = maxbox.z - vert.z;
        vmax.z = -maxbox.z - vert.z;
    }

	if(normal * vmin > 0) return 0;
	if(normal * vmax >= 0) return 1;
	return 0;
}



bool AABB::intersect(Triangle &triangle) const {

    Vector3 boxCenter = getCenter();
    Vector3 boxSize = max - min; 
    Vector3 boxHalfSize = boxSize * 0.5f;

    /*    use separating axis theorem to test overlap between triangle and box */
	/*    need to test for overlap in these directions: */
	/*    1) the {x,y,z}-directions (actually, since we use the AABB of the triangle */
	/*       we do not even need to test these) */
	/*    2) normal of the triangle */
	/*    3) crossproduct(edge from tri, {x,y,z}-directin) */
	/*       this gives 3x3=9 more tests */
	Vector3 v0, v1, v2;

	float minTri, maxTri, p0, p1, p2, rad, fex, fey, fez;
	Vector3 normal, e0, e1, e2;

	/* This is the fastest branch on Sun */
	/* move everything so that the boxCenter is in (0,0,0) */
	v0 = triangle.v0 - boxCenter;
	v1 = triangle.v1 - boxCenter;
	v2 = triangle.v2 - boxCenter;

	/* compute triangle edges */
	e0 = v1 - v0;      /* tri edge 0 */
	e1 = v2 - v1;      /* tri edge 1 */
	e2 = v0 - v2;      /* tri edge 2 */

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */
	fex = fabsf(e0.x);
	fey = fabsf(e0.y);
	fez = fabsf(e0.z);

	AXISTEST_X01(e0.z, e0.y, fez, fey);
	AXISTEST_Y02(e0.z, e0.x, fez, fex);
	AXISTEST_Z12(e0.y, e0.x, fey, fex);

	fex = fabsf(e1.x);
	fey = fabsf(e1.y);
	fez = fabsf(e1.z);

	AXISTEST_X01(e1.z, e1.y, fez, fey);
	AXISTEST_Y02(e1.z, e1.x, fez, fex);
	AXISTEST_Z0(e1.y, e1.x, fey, fex);

	fex = fabsf(e2.x);
	fey = fabsf(e2.y);
	fez = fabsf(e2.z);
	AXISTEST_X2(e2.z, e2.y, fez, fey);
	AXISTEST_Y1(e2.z, e2.x, fez, fex);
	AXISTEST_Z12(e2.y, e2.x, fey, fex);

	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */
	/* test in X-direction */
	_getMinMax(v0.x, v1.x, v2.x, minTri, maxTri);
	if(minTri > boxHalfSize.x || maxTri < -boxHalfSize.x) return 0;

	/* test in Y-direction */
	_getMinMax(v0.y, v1.y, v2.y, minTri, maxTri);
	if(minTri > boxHalfSize.y || maxTri < -boxHalfSize.y) return 0;

	/* test in Z-direction */
	_getMinMax(v0.z, v1.z, v2.z, minTri, maxTri);
	if(minTri > boxHalfSize.z || maxTri < -boxHalfSize.z) return 0;

	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	normal = e0 ^ e1;
	return PlaneBoxOverlap(normal, v0, boxHalfSize);
}
