#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

Vector
Vec(double x, double y, double w)
{
	return (Vector){x, y, w};
}

Vector
addvec(Vector v, Vector u)
{
	return (Vector){v.x+u.x, v.y+u.y, v.w+u.w};
}

Vector
subvec(Vector v, Vector u)
{
	return (Vector){v.x-u.x, v.y-u.y, v.w-u.w};
}

Vector
mulvec(Vector v, double s)
{
	return (Vector){v.x*s, v.y*s, v.w*s};
}

double
dotvec(Vector v, Vector u)
{
	return v.x*u.x + v.y*u.y + v.w*u.w;
}

Vector
normvec(Vector v)
{
	double len;

	len = hypot3(v.x, v.y, v.w);
	if(len == 0)
		return (Vector){0, 0, 0};
	v.x /= len;
	v.y /= len;
	v.w /= len;
	return v;
}
