#define Eg	9.81
#define PIX2M	0.001
#define M2PIX	(1.0/PIX2M)

enum {
	Stheta = 0,
	Spos,
	Svel,
	Sdeltax,
	Seta,
	SLEN,
};

typedef struct Projectile Projectile;

struct Projectile
{
	Point2 p, v;
	double mass;
};
