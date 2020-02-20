#define DEG	0.01745329251994330
#define Eg	9.81
#define PIX2M	0.001
#define M2PIX	(1.0/PIX2M)

enum {
	STACK = 8192,
	SEC = 1000,
	FPS = 60,
};

enum {
	Stheta = 0,
	Spos,
	Svel,
	Sdeltax,
	Seta,
	SLEN,
};

typedef struct Vector Vector;
typedef double Matrix[3][3];
typedef struct Projectile Projectile;

struct Vector {
	double x, y, w;
};

struct Projectile {
	Vector p, v;
	double mass;
};
