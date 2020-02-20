#include <u.h>
#include <libc.h>
#include "dat.h"
#include "fns.h"

void
addm(Matrix a, Matrix b)
{
	int i, j;

	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			a[i][j] += b[i][j];
}

void
subm(Matrix a, Matrix b)
{
	int i, j;

	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			a[i][j] -= b[i][j];
}

void
mulm(Matrix a, Matrix b)
{
	int i, j, k;
	Matrix r;

	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++){
			r[i][j] = 0;
			for(k = 0; k < 3; k++)
				r[i][j] += a[i][k]*b[k][j];
		}
	for(i = 0; i < 3; i++)
		for(j = 0; j < 3; j++)
			a[i][j] = r[i][j];
}

void
transm(Matrix m)
{
	int i, j;
	double tmp;

	for(i = 0; i < 3; i++)
		for(j = i; j < 3; j++){
			tmp = m[i][j];
			m[i][j] = m[j][i];
			m[j][i] = tmp;
		}
}

double
detm(Matrix m)
{
	return m[0][0]*(m[1][1]*m[2][2] - m[1][2]*m[2][1])
		+ m[0][1]*(m[1][2]*m[2][0] - m[1][0]*m[2][2])
		+ m[0][2]*(m[1][0]*m[2][1] - m[1][1]*m[2][0]);
}

Vector
mulvecm(Vector v, Matrix m)
{
	Vector r;

	r.x = v.x * m[0][0] + v.y * m[0][1] + v.w * m[0][2];
	r.y = v.x * m[1][0] + v.y * m[1][1] + v.w * m[1][2];
	r.w = v.x * m[2][0] + v.y * m[2][1] + v.w * m[2][2];
	return r;
}
