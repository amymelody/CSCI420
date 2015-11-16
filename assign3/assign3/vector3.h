#include <GL/glu.h>
#include <math.h>

#ifndef VECTOR3_H
#define VECTOR3_H

/* struct of vector with 3 components */
struct vector3 {
	double x;
	double y;
	double z;

	vector3(double inX, double inY, double inZ) : x(inX), y(inY), z(inZ) {}
	vector3() : x(0), y(0), z(0) {}

	double lengthSquared()
	{
		return x*x + y*y + z*z;
	}

	void normalize()
	{
		double length = lengthSquared();
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
		return vector3(this->x+rhs.x, this->y+rhs.y, this->z+rhs.z);
	}

	vector3 operator-(const vector3& rhs)
	{
		return vector3(this->x-rhs.x, this->y-rhs.y, this->z-rhs.z);
	}

	vector3 operator-()
	{
		return vector3(-this->x, -this->y, -this->z);
	}

	vector3 operator*(double rhs)
	{
		return vector3(this->x*rhs, this->y*rhs, this->z*rhs);
	}
};

double lineLengthSquared(vector3 v0, vector3 v1)
{
	return (v1.x-v0.x)*(v1.x-v0.x) + 
		(v1.y-v0.y)*(v1.y-v0.y) + 
		(v1.z-v0.z)*(v1.z-v0.z);
}

double dotProduct(vector3 A, vector3 B)
{
	return A.x*B.x + A.y*B.y + A.z*B.z;
}

vector3 crossProduct(vector3 A, vector3 B)
{
	vector3 product;
	product.x = A.y*B.z - A.z*B.y;
	product.y = A.z*B.x - A.x*B.z;
	product.z = A.x*B.y - A.y*B.x;
	return product;
}

#endif