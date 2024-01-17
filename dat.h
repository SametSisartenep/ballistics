#define Eg	9.81	/* earth's gravity (m·s⁻²) */
#define Cd	0.45	/* drag coefficient for a sphere */
#define ρ	1.293	/* air density (kg·m⁻³) */
#define M2PIX	(1.0/PIX2M)

enum {
	Stheta = 0,
	Spos,
	Svel,
	Sdrag,
	Sdeltax,
	Seta,
	Sscale,
	SLEN,
};

typedef struct Projectile Projectile;

struct Projectile
{
	Point2 p, v;
	double mass;
	double r;
};
