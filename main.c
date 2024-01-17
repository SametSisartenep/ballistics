#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

double PIX2M = 5;

Mousectl *mc;
Keyboardctl *kc;
Channel *scrsync;
RFrame worldrf;
Projectile ball;
double t0, Δt;
double v0;
double A;	/* area of the projectile that meets the air */
Point2 target;

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
toscreen(Point2 p)
{
	p = invrframexform(p, worldrf);
	return Pt(p.x, p.y);
}

Point2
fromscreen(Point p)
{
	return rframexform(Pt2(p.x, p.y, 1), worldrf);
}

void
∫(double Δt)
{
	Point2 Fd;	/* drag force */

	Fd = mulpt2(mulpt2(ball.v, -vec2len(ball.v)), 0.5 * Cd*ρ*A);	/* ½CdρAv² */
	ball.v = addpt2(ball.v, mulpt2(addpt2(Vec2(0,-Eg), divpt2(Fd, ball.mass)), Δt));
	ball.p = addpt2(ball.p, mulpt2(ball.v, Δt));
	snprint(stats[Spos], sizeof(stats[Spos]), "p: %v", ball.p);
	snprint(stats[Sdrag], sizeof(stats[Sdrag]), "Fd: %v", divpt2(Fd, ball.mass));
	if(ball.p.y <= (2+1)*M2PIX){
		ball.p.y = (2+1)*M2PIX;
		ball.v = Vec2(0,0);
	}
}

void
drawstats(void)
{
	int i;

	snprint(stats[Svel], sizeof(stats[Svel]), "|v|: %gm/s", vec2len(ball.v));
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
	line(screen, toscreen(Pt2(ball.p.x, 0, 1)), toscreen(target), 0, 0, 1, statc, ZP);
	drawstats();
	flushimage(display, 1);
	unlockdisplay(display);
}

void
mmb(void)
{
	enum {
		SETV0,
		SETPOS,
		SETMASS,
		SETDIAM,
	};
	static char *items[] = {
	 [SETV0]	"set v0",
	 [SETPOS]	"set position",
	 [SETMASS]	"set mass",
	 [SETDIAM]	"set diameter",
		nil
	};
	static Menu menu = { .item = items };
	char buf[32], *p;

	switch(menuhit(2, mc, &menu, nil)){
	case SETV0:
		snprint(buf, sizeof(buf), "%g", v0);
		enter("v0(m/s):", buf, sizeof(buf), mc, kc, nil);
		if(buf[0] != 0)
			v0 = strtod(buf, nil);
		break;
	case SETPOS:
		snprint(buf, sizeof(buf), "%g, %g", ball.p.x, ball.p.y);
		enter("pos(x,y):", buf, sizeof(buf), mc, kc, nil);
		if(buf[0] != 0 && (p = strchr(buf, ',')) != nil){
			ball.p.x = strtod(buf, nil);
			ball.p.y = strtod(p+1, nil);
		}
		break;
	case SETMASS:
		snprint(buf, sizeof(buf), "%g", ball.mass);
		enter("mass(kg):", buf, sizeof(buf), mc, kc, nil);
		if(buf[0] != 0)
			ball.mass = strtod(buf, nil);
		break;
	case SETDIAM:
		snprint(buf, sizeof(buf), "%g", 2*ball.r);
		enter("diameter(m):", buf, sizeof(buf), mc, kc, nil);
		if(buf[0] != 0){
			ball.r = strtod(buf, nil)/2;
			A = 2*PI*ball.r*ball.r;	/* ½(4πr²) */
		}
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
		ball.p = Pt2((2+1)*M2PIX,(2+1)*M2PIX,1);
		ball.v = Vec2(0,0);
		break;
	case QUIT:
		threadexitsall(nil);
	}
}

void
zoomin(void)
{
	PIX2M += 0.01;
	worldrf.bx = Vec2(PIX2M,0);
	worldrf.by = Vec2(0,-PIX2M);
	snprint(stats[Sscale], sizeof(stats[Sscale]), "s: %gm/pix", M2PIX);
}

void
zoomout(void)
{
	PIX2M -= 0.01;
	worldrf.bx = Vec2(PIX2M,0);
	worldrf.by = Vec2(0,-PIX2M);
	snprint(stats[Sscale], sizeof(stats[Sscale]), "s: %gm/px", M2PIX);
}

void
mouse(void)
{
	Point2 p;
	double θ, dist, eta;

	if(ball.p.y <= (2+1)*M2PIX){
		p = subpt2(fromscreen(mc->xy), ball.p);
		θ = atan2(p.y, p.x);
		snprint(stats[Stheta], sizeof(stats[Stheta]), "θ: %g°", θ/DEG);
		dist = v0*v0*sin(2*θ)/Eg;
		target = Pt2(ball.p.x+dist, 0, 1);
		eta = 2*v0*sin(θ)/Eg;
		snprint(stats[Seta], sizeof(stats[Seta]), "eta: %gs", eta);
		if((mc->buttons & 1) != 0)
			ball.v = Vec2(v0*cos(θ), v0*sin(θ));
	}
	if((mc->buttons & 2) != 0)
		mmb();
	if((mc->buttons & 4) != 0)
		rmb();
	if((mc->buttons & 8) != 0)
		zoomin();
	if((mc->buttons & 16) != 0)
		zoomout();
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
	worldrf.p = Pt2(screen->r.min.x, screen->r.max.y, 1);
	unlockdisplay(display);
	redraw();
}

void
scrsyncproc(void *)
{
	for(;;){
		send(scrsync, nil);
		sleep(HZ2MS(60));
	}
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

	GEOMfmtinstall();
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

	snprint(stats[Sscale], sizeof(stats[Sscale]), "s: %gm/pix", M2PIX);

	worldrf.p = Pt2(screen->r.min.x, screen->r.max.y, 1);
	worldrf.bx = Vec2(PIX2M,0);
	worldrf.by = Vec2(0,-PIX2M);

	ball.p = Pt2((2+1)*M2PIX,(2+1)*M2PIX,1);
	ball.v = Vec2(0, 0);
	ball.mass = 0.149;
	ball.r = 0.375;		/* 3.75cm */
	A = 2*PI*ball.r*ball.r;	/* ½(4πr²) */
	v0 = 53.64;		/* avg baseball hit speed */

	scrsync = chancreate(1, 0);
	display->locking = 1;
	unlockdisplay(display);

	proccreate(scrsyncproc, 0, mainstacksize);

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
		Δt = (nsec()-t0);
		∫(Δt/1e9);
		t0 += Δt;
	}
}
