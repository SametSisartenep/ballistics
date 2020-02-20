#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

double
round(double n)
{
	return floor(n + 0.5);
}

double
hypot3(double x, double y, double z)
{
	return hypot(x, hypot(y, z));
}
