#include <GL/glu.h>

#ifndef VECTOR3_H
#define VECTOR3_H

struct vector3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;

	vector3(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) {}
	vector3() : x(0), y(0), z(0) {}

	GLfloat lengthSquared()
	{
		return x*x + y*y + z*z;
	}

	void normalize()
	{
		GLfloat length = lengthSquared();
		length = sqrt(length);
		if (length > 0)
		{
			x /= length;
			y /= length;
			z /= length;
		}
	}

	vector3 operator+(const vector3& rhs)
	{
		vector3 result;
		result.x = this->x + rhs.x;
		result.y = this->y + rhs.y;
		result.z = this->z + rhs.z;
		return result;
	}
};

GLfloat lineLengthSquared(vector3 v0, vector3 v1)
{
	return (v1.x-v0.x)*(v1.x-v0.x) + 
		(v1.y-v0.y)*(v1.y-v0.y) + 
		(v1.z-v0.z)*(v1.z-v0.z);
}

vector3 crossProduct(vector3 A, vector3 B)
{
	vector3 product;
	product.x = A.y*B.z + A.z*B.y;
	product.y = A.z*B.x + A.x*B.z;
	product.z = A.x*B.y + B.y*A.x;
	return product;
}

#endif