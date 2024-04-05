#include "util.h"
#include "float.h"
#include <unistd.h>
#include <cgfx.h>

static void graphInit(void) {
	static const u8 paldata[] = {
		000, 010, 020, 030, 040, 050, 060, 070,
		007, 011, 022, 033, 044, 055, 066, 077,
	};
	if (_os_open("/W", S_IREAD | S_IWRITE, &cur_path)) exit(1);
	_cgfx_dwset(cur_path, 8, 0, 0, 40, 24, 0, 0, 0);
	_cgfx_curoff(cur_path);
	_cgfx_scalesw(cur_path, 0);
	for (u8 i = 0; i < 16; i++)
		pal(i, paldata[i]);
	color(0);
	boxfill(0, 0, 319, 191);
	initTerm(1, 1);
	_cgfx_select(cur_path);
}

void mandelbrot(u8 graph) {
	FLOAT c2, c4, cx, cy;
	ldif(2, c2);
	ldif(4, c4);
	s16 x, x0, y, y0;
	if (graph) {
		graphInit();
		x0 = 159;
		y0 = 95;
		ldf(0x563b, 0x78, cy); // 1/y0=0.010526
		cpf(cy, cx);
	}
	else {
		x0 = 39;
		y0 = 12;
		ldf(0x5dcc, 0x7a, cx); // 0.045800
		ldf(0x5555, 0x7b, cy); // 0.083333
	}
	for (y = -y0; y <= y0; y++) {
		if (!graph) _putchar('\n');
		FLOAT yf, cb;
		ldif(y, yf);
		mulf(yf, cy, cb);
		for (x = -x0; x <= x0; x++) {
			FLOAT xf, ca, a, b, a2, b2;
			ldif(x, xf);
			mulf(xf, cx, ca);
			cpf(ca, a);
			cpf(cb, b);
			mulf(a, a, a2);
			mulf(b, b, b2);
			u8 i;
			for (i = 0; i <= 15; i++) {
				FLOAT t;
				subf(a2, b2, t);
				addf(t, ca, t);
				mulf(a, b, b);
				mulf(b, c2, b);
				addf(b, cb, b);
				cpf(t, a);
				mulf(a, a, a2);
				mulf(b, b, b2);
				addf(a2, b2, t);
				subf(c4, t, t);
				if (mif(t)) {
					if (graph) {
						color(i);
						point(x + 160, y + 96);
					}
					else _printf("%X", i);
					break;
				}
			}
			if (!graph && i > 15) _putchar(' ');
		}
	}
	_getchar();
	if (graph) {
		_os_close(cur_path);
		cur_path = 0;
	}
}
