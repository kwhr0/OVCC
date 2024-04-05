#include "util.h"
#include <cmoc.h>
#include <time.h>
#include <cgfx.h>

#define DOUBLE_BUFFER
#define METER
#define BACURA_N	64
#define STAR_N		256
#define NEAR		32
#define FAR			255
#define INVALID		-128
#define	SX			16

#ifdef DOUBLE_BUFFER
#define PATH_N		2
#else
#define PATH_N		1
#endif

typedef struct {
	s8 x, y;
	u8 z, t;
} Obj;

static u8 outpath[PATH_N];
static Obj *star, *bacura, *top;
static u8 sw;

static void init(void) {
	static const u8 paldata[] = {
		000, 007, 070, 077, 000, 000, 000, 000,
		077, 010, 020, 030, 040, 050, 060, 070,
	};
	_os_time tm;
	_os_gettime(&tm);
	srand(*(u16 *)&tm.minutes);
	u8 i, j;
	for (i = 0; i < PATH_N; i++) {
		if (_os_open("/W", S_IREAD | S_IWRITE, &outpath[i])) exit(1);
		cur_path = outpath[i];
		_cgfx_dwset(cur_path, 8, 0, 0, 40, 24, 0, 0, 0);
		_cgfx_curoff(cur_path);
		_cgfx_scalesw(cur_path, 0);
		for (j = 0; j < 16; j++)
			pal(j, paldata[j]);
	}
	pal(8, 0); // blinking star
	Obj *p;
	u16 d, z;
#if STAR_N > 0
	if (!star) star = (Obj *)alloc(STAR_N * sizeof(Obj), 1);
	d = ((u16)FAR - NEAR << 8) / STAR_N;
	z = (u16)FAR << 8;
	for (p = star; p < &star[STAR_N]; p++) {
		p->y = INVALID;
		p->z = (u8)((z -= d) >> 8);
	}
#endif
#if BACURA_N > 0
	if (!bacura) bacura = (Obj *)alloc(BACURA_N * sizeof(Obj), 1);
	d = ((u16)FAR - NEAR << 8) / BACURA_N;
	z = (u16)FAR << 8;
	for (top = p = bacura; p < &bacura[BACURA_N]; p++) {
		p->y = INVALID;
		p->z = (u8)((z -= d) >> 8);
	}
#endif
}

static asm s16 shiftdiv(s16 a, s16 b) {
	asm {
		ldd 2,s
		asld
		asld
		asld
		asld
		asld
		tfr d,w
		sexw
		divq 4,s
		tfr w,d
	}
}

#define projX(x, z)		(shiftdiv(x, z) + 160)
#define projY(y, z)		(shiftdiv(y, z) + 96)
#define swap8(a, b)		{ u8 t = a; a = b; b = t; }
#define swap16(a, b)	{ u16 t = a; a = b; b = t; }
#define emit(p) (\
	(p)->x = (s8)rand(),\
	(p)->y = (s8)((3 * (rand() & 0x1fff) >> 7) - 96),\
	(p)->z = FAR,\
	(p)->t = (u8)rand())

static void process(Obj *p) {
	u16 x0, x1, x2, x3;
	u8 y01, y23;
	s8 c = cosi(p->t) >> 5, s = sini(p->t) >> 5;
	p->t -= 8;
	y01 = (u8)projY((s16)p->y + s, (s16)p->z + c);
	if (y01 >= 192) return;
	y23 = (u8)projY((s16)p->y - s, (s16)p->z - c);
	if (y23 >= 192) return;
	x0 = projX((s16)p->x - SX, (s16)p->z + c);
	x1 = projX((s16)p->x + SX, (s16)p->z + c);
	x2 = projX((s16)p->x - SX, (s16)p->z - c);
	x3 = projX((s16)p->x + SX, (s16)p->z - c);
	u8 t = p->t & 0x7f;
	color(t >= 0x58 && t <= 0x68 ? 3 : t > 0x40 ? 2 : 1);
	if (y01 == y23) {
		x2 = x0 < x2 ? x0 : x2;
		x3 = x1 > x3 ? x1 : x3;
	}
	else {
		if (y01 > y23) {
			swap16(x0, x2);
			swap16(x1, x3);
			swap8(y01, y23);
		}
		u8 y = y23 - y01;
		s16 d0 = shiftdiv(x2 - x0, y);
		s16 d1 = shiftdiv(x3 - x1, y);
		x0 = (x0 << 5) + 16;
		x1 = (x1 << 5) + 16;
		for (y = y01; y < y23; y++) {
			line(x0 >> 5, y, x1 >> 5, y);
			x0 += d0;
			x1 += d1;
		}
	}
	line(x2, y23, x3, y23);
}

static void update(void) {
	_cgfx_select(outpath[sw]);
#ifdef DOUBLE_BUFFER
	sw = !sw;
#endif
	cur_path = outpath[sw];
	color(0);
	boxfill(0, 0, 319, 191);
	Obj *p, *top_next;
#if STAR_N > 0
	for (p = star; p < &star[STAR_N]; p++) {
		if (p->z > NEAR) p->z--;
		else emit(p);
		if (p->y != INVALID) {
			color(p->t & 7 | 8);
			point(projX(p->x, p->z), projY(p->y, p->z));
		}
	}
#endif
#if BACURA_N > 0
	p = top_next = top;
	do {
		if (p->z > NEAR) p->z -= 2;
		else {
			emit(p);
			top_next = p;
		}
		if (p->y != INVALID) process(p);
		if (++p >= &bacura[BACURA_N]) p = bacura;
	} while (p != top);
	top = top_next;
#endif
#ifdef METER
	static u8 lastsec, count, lastcount, drop;
	if (count < 192) count++;
	_os_time tm;
	_os_gettime(&tm);
	if (lastsec != tm.seconds) {
		lastsec = tm.seconds;
		drop = count < 30;
		lastcount = count;
		count = 0;
	}
	color(15);
	boxfill(0, 192 - 30, 3, 191);
	color(drop ? 12 : 10);
	boxfill(0, 192 - lastcount, 3, 191);
#endif
	Flush();
}

void star_and_bacura() {
	init();
	initTerm(0, 0);
	do update();
	while (_getchar() != KEY_ESC);
	for (u8 i = 0; i < PATH_N; i++)
		_os_close(outpath[i]);
	cur_path = 0;
}
