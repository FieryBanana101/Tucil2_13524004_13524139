#include "Triangle.hpp"
#include "AABB.hpp"
#include <cmath>
#include <algorithm> // std::min, std::max

Vector3 AABB::getCenter() const {
    return Vector3((min.x + max.x) / 2.0f, (min.y + max.y) / 2.0f, (min.z + max.z) / 2.0f);
}


float AABB::getVolume() const {
    Vector3 dist = max - min;
    return dist.x * dist.y * dist.z;
}


inline bool AABB::axisTest(
    const Vector3& axis,
    const Vector3& v0,
    const Vector3& v1,
    const Vector3& v2,
    const Vector3& boxHalfSize
){

	// Project the triangle vertice into the axis
    float p0 = axis * v0;
    float p1 = axis * v1;
    float p2 = axis * v2;

	// Get the minima and maxima of the triangle projection
    float minTriangle = std::min({p0, p1, p2});
    float maxTriangle = std::max({p0, p1, p2});

	// Project box radius into the axis
    float rad = boxHalfSize.x * std::abs(axis.x) + boxHalfSize.y * std::abs(axis.y) + boxHalfSize.z * std::abs(axis.z);

	// Basic test for whether two line intersect in 1D
    return (maxTriangle >= -rad && minTriangle <= rad);
}




/*
	Refactored from https://fileadmin.cs.lth.se/cs/Personal/Tomas_Akenine-Moller/code/tribox_tam.pdf
 */
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
	Vector3 normal, e0, e1, e2;

	/* move everything so that the boxCenter is in (0,0,0) */
	v0 = triangle.v0 - boxCenter;
	v1 = triangle.v1 - boxCenter;
	v2 = triangle.v2 - boxCenter;

	/* compute triangle edges */
	e0 = v1 - v0;      /* tri edge 0 */
	e1 = v2 - v1;      /* tri edge 1 */
	e2 = v0 - v2;      /* tri edge 2 */

	Vector3 boxAxis[3] = {
    	{1,0,0},
    	{0,1,0},
    	{0,0,1}
	};

	/* Bullet 3:  */
	/*  test the 9 tests first (this was faster) */

	Vector3 triEdges[3] = {e0, e1, e2};
	for(int i = 0; i < 3; i++){
		for(int j = 0; j < 3; j++){

			Vector3 axis = triEdges[i] ^ boxAxis[j];
			if(!axisTest(axis, v0, v1, v2, boxHalfSize)) return false;

		}
	}


	/* Bullet 1: */
	/*  first test overlap in the {x,y,z}-directions */
	/*  find min, max of the triangle each direction, and test for overlap in */
	/*  that direction -- this is equivalent to testing a minimal AABB around */
	/*  the triangle against the AABB */
	/* test in X-direction */
	if(!axisTest(boxAxis[0], v0, v1, v2, boxHalfSize)) return false;
	if(!axisTest(boxAxis[1], v0, v1, v2, boxHalfSize)) return false;
	if(!axisTest(boxAxis[2], v0, v1, v2, boxHalfSize)) return false;


	/* Bullet 2: */
	/*  test if the box intersects the plane of the triangle */
	/*  compute plane equation of triangle: normal*x+d=0 */
	normal = (v1 - v0) ^ (v2 - v0);
	return axisTest(normal, v0, v1, v2, boxHalfSize);
}
