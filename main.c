#include <u.h>
#include <libc.h>
#include <thread.h>
#include <draw.h>
#include <mouse.h>
#include <keyboard.h>
#include <geometry.h>
#include "dat.h"
#include "fns.h"

Mousectl *mc;
Keyboardctl *kc;
Channel *scrsync;
RFrame worldrf;
Projectile ball;
double t0, Δt;
double v0;
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
drawstats(void)
{
	int i;

	snprint(stats[Svel], sizeof(stats[Svel]), "v: %gm/s", vec2len(ball.v));
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
		ball.p = Pt2((2+1)*M2PIX,(2+1)*M2PIX,1);
		ball.v = Vec2(0,0);
		break;
	case QUIT:
		threadexitsall(nil);
	}
}

void
mouse(void)
{
	Point2 p;
	double θ, dist, eta;

	if(ball.p.y <= (2+1)*M2PIX){
		p = subpt2(fromscreen(mc->xy), ball.p);
		θ = atan2(p.y, p.x);
		snprint(stats[Stheta], sizeof(stats[Stheta]), "θ: %g°", θ*180/PI);
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

	worldrf.p = Pt2(screen->r.min.x, screen->r.max.y, 1);
	worldrf.bx = Vec2(PIX2M,0);
	worldrf.by = Vec2(0,-PIX2M);

	ball.p = Pt2((2+1)*M2PIX,(2+1)*M2PIX,1);
	ball.v = Vec2(0, 0);
	ball.mass = 106000;
	v0 = 1640; /* Paris Gun's specs */

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
		Δt = (nsec()-t0)/1e9;
		ball.v = addpt2(ball.v, mulpt2(Vec2(0,-Eg), Δt));
		ball.p = addpt2(ball.p, mulpt2(ball.v, Δt));
		snprint(stats[Spos], sizeof(stats[Spos]), "p: %v", ball.p);
		if(ball.p.y <= (2+1)*M2PIX){
			ball.p.y = (2+1)*M2PIX;
			ball.v = Vec2(0,0);
		}
		t0 += Δt*1e9;
	}
}
