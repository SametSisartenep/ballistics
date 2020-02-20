#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include "dat.h"
#include "fns.h"

Mousectl *mc;
Keyboardctl *kc;
Channel *scrsync;
Point orig;
Vector basis;
Projectile ball;
double t0, Δt;
double v0;
Vector target;

char stats[SLEN][64];
Image *statc;

void *
emalloc(ulong n)
{
	void *p;

	p = malloc(n);
	if(p == nil)
		sysfatal("malloc: %r");
	memset(p, 0, n);
	setmalloctag(p, getcallerpc(&n));
	return p;
}

Point
toscreen(Vector p)
{
	return addpt(orig, Pt(p.x*basis.x, p.y*basis.y));
}

Vector
fromscreen(Point p)
{
	return Vec((p.x-screen->r.min.x)*M2PIX, (screen->r.max.y-p.y)*M2PIX, 1);
}

void
drawstats(void)
{
	int i;

	snprint(stats[Svel], sizeof(stats[Svel]), "v: %gm/s", hypot(ball.v.x, ball.v.y));
	snprint(stats[Sdeltax], sizeof(stats[Sdeltax]), "Δx: %gm", target.x-ball.p.x);
	for(i = 0; i < nelem(stats); i++)
		stringn(screen, addpt(screen->r.min, Pt(10, font->height*i+1)), statc, ZP, font, stats[i], sizeof(stats[i]));
}

void
redraw(void)
{
	lockdisplay(display);
	draw(screen, screen->r, display->black, nil, ZP);
	fillellipse(screen, toscreen(ball.p), 2, 2, display->white, ZP);
	line(screen, toscreen(Vec(ball.p.x, 0, 1)), toscreen(target), 0, 0, 1, statc, ZP);
	drawstats();
	flushimage(display, 1);
	unlockdisplay(display);
}

void
mmb(void)
{
	enum {
		SETV0,
	};
	static char *items[] = {
	 [SETV0]	"set v0",
		nil
	};
	static Menu menu = { .item = items };
	char buf[32];

	snprint(buf, sizeof(buf), "%g", v0);
	switch(menuhit(2, mc, &menu, nil)){
	case SETV0:
		enter("v0(m/s):", buf, sizeof(buf), mc, kc, nil);
		v0 = strtod(buf, 0);
		break;
	}
}

void
rmb(void)
{
	enum {
		RST,
		QUIT,
	};
	static char *items[] = {
	 [RST]	"reset",
	 [QUIT]	"quit",
		nil,
	};
	static Menu menu = { .item = items };

	switch(menuhit(3, mc, &menu, nil)){
	case RST:
		ball.p = Vec((2+1)*M2PIX, (2+1)*M2PIX, 1);
		ball.v = Vec(0, 0, 1);
		break;
	case QUIT:
		threadexitsall(nil);
	}
}

void
mouse(void)
{
	Vector p;
	double θ, dist, eta;

	if(ball.p.y <= (2+1)*M2PIX){
		p = subvec(fromscreen(mc->xy), ball.p);
		θ = atan2(p.y, p.x);
		snprint(stats[Stheta], sizeof(stats[Stheta]), "θ: %g°", θ*180/PI);
		dist = v0*v0*sin(2*θ)/Eg;
		target = Vec(ball.p.x+dist, 0, 1);
		eta = 2*v0*sin(θ)/Eg;
		snprint(stats[Seta], sizeof(stats[Seta]), "eta: %gs", eta);
		if((mc->buttons & 1) != 0)
			ball.v = Vec(v0*cos(θ), v0*sin(θ), 1);
	}
	if((mc->buttons & 2) != 0)
		mmb();
	if((mc->buttons & 4) != 0)
		rmb();
}

void
key(Rune r)
{
	switch(r){
	case Kdel:
	case 'q':
		threadexitsall(nil);
	}
}

void
resized(void)
{
	lockdisplay(display);
	if(getwindow(display, Refnone) < 0)
		fprint(2, "can't reattach to window\n");
	orig = Pt(screen->r.min.x, screen->r.max.y);
	unlockdisplay(display);
	redraw();
}

void
scrsyncproc(void *)
{
	for(;;){
		send(scrsync, nil);
		sleep(SEC/FPS);
	}
}

#pragma varargck type "V" Vector;
int
Vfmt(Fmt *f)
{
	Vector v;

	v = va_arg(f->args, Vector);
	return fmtprint(f, "(%g %g)", v.x, v.y);
}

void
usage(void)
{
	fprint(2, "usage: %s\n", argv0);
	exits("usage");
}

void
threadmain(int argc, char *argv[])
{
	Rune r;

	fmtinstall('V', Vfmt);
	ARGBEGIN{
	default: usage();
	}ARGEND;

	if(initdraw(nil, nil, "ballistics") < 0)
		sysfatal("initdraw: %r");
	mc = initmouse(nil, screen);
	if(mc == nil)
		sysfatal("initmouse: %r");
	kc = initkeyboard(nil);
	if(kc == nil)
		sysfatal("initkeyboard: %r");
	statc = allocimage(display, Rect(0, 0, 1, 1), screen->chan, 1, DYellow);
	if(statc == nil)
		sysfatal("allocimage: %r");
	orig = Pt(screen->r.min.x, screen->r.max.y);
	basis = Vec(PIX2M, -PIX2M, 1);
	ball.p = Vec((2+1)*M2PIX, (2+1)*M2PIX, 1);
	ball.v = Vec(0, 0, 1);
	ball.mass = 106000;
	v0 = 1640; /* Paris Gun's specs */
	scrsync = chancreate(1, 0);
	display->locking = 1;
	unlockdisplay(display);
	proccreate(scrsyncproc, 0, STACK);
	t0 = nsec();

	for(;;){
		Alt a[] = {
			{mc->c, &mc->Mouse, CHANRCV},
			{mc->resizec, nil, CHANRCV},
			{kc->c, &r, CHANRCV},
			{scrsync, nil, CHANRCV},
			{nil, nil, CHANEND}
		};
		switch(alt(a)){
		case 0: mouse(); break;
		case 1: resized(); break;
		case 2: key(r); break;
		case 3: redraw(); break;
		}
		Δt = (nsec()-t0)/1e9;
		ball.v = addvec(ball.v, mulvec(Vec(0, -Eg, 1), Δt));
		ball.p = addvec(ball.p, mulvec(ball.v, Δt));
		snprint(stats[Spos], sizeof(stats[Spos]), "p: %V", ball.p);
		if(ball.p.y <= (2+1)*M2PIX){
			ball.p.y = (2+1)*M2PIX;
			ball.v = Vec(0, 0, 1);
		}
		t0 += Δt*1e9;
	}
}
