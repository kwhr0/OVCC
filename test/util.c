#include "util.h"
#include <cmoc.h>
#include <time.h>
#include <cgfx.h>

u8 cur_path, last_path;

static u8 term_path, getc_wait;

void initTerm(u8 echo, u8 wait) {
	DEBUG_FLAG = 1;
	if (!cur_path) {
		if (!term_path && _os_open("/TERM", S_IREAD | S_IWRITE, &term_path)) exit(1);
		cur_path = term_path;
	}
	u8 buf[32];
	getstat(SS_Opt, cur_path, buf);
	buf[4] = echo ? 1 : 0;
	setstat(SS_Opt, cur_path, buf);
	getc_wait = wait;
}

u8 _getchar(void) {
	if (!getc_wait && getstat(SS_Ready, cur_path)) return 0;
	u8 buf;
	int cnt = 1;
	return !_os_read(cur_path, &buf, &cnt) && cnt == 1 ? buf : 0;
}

void _putchar(char c) {
	if (cur_path) {
		static char t[] = { 0, '\r' };
		t[0] = c;
		_cgfx_select(cur_path);
		int cnt = c == '\n' ? 2 : 1;
		_os_write(cur_path, t, &cnt);
	}
	else DEBUG_OUT = c;
}

void _printf(const char *format, ...) {
	u16 *ap = (u16 *)&format;
	u8 *p = (u8 *)format;
	while (*p) {
		u8 c[7];
		u8 a, b, f, i, l, n, r, t, u;
		u16 v;
		u8 *q;
loop:
		switch (t = *p++) {
		case '%':
			b = ' ';
			l = n = 0;
			switch (t = *p++) {
				case '%': _putchar('%'); goto loop;
				case '0': b = '0'; t = *p++; break;
				case '-': l = 1; t = *p++; break;
				case 'c': _putchar((u8)*(u16 *)++ap); goto loop;
			}
			if (t >= '1' && t <= '9')
				for (n = t - '0'; (t = *p++) >= '0' && t <= '9';)
					n = 10 * n + t - '0';
			if (t == 's')
				for (q = *(u8 **)++ap, i = 0; q[i]; i++)
					;
			else {
				if (u = t == 'u') t = *p++;
				switch (t) {
					case 'd': r = 10; break;
					case 'o': r = 8; u = 1; break;
					case 'x': r = 16; u = 1; a = 0x27; break;
					case 'X': r = 16; u = 1; a = 7; break;
					default: goto loop;
				}
				v = *(u16 *)++ap;
				if (f = !u && (s16)v < 0) v = -v;
				c[i = sizeof(c) - 1] = 0;
				do {
					u8 t = (u8)(v % r + '0');
					c[--i] = t > '9' ? t + a : t;
				} while (v /= r);
				if (f) c[--i] = '-';
				q = &c[i];
			}
			i = sizeof(c) - 1 - i;
			if (l) n -= i;
			else while (n-- > i) _putchar(b);
			while (*q) _putchar(*q++);
			if (l) while (n--) _putchar(' ');
			break;
		default:
			_putchar(t);
			break;
		}
	}
}

void locate(u8 x, u8 y) {
	u8 buf[] = { 2, x + ' ', y + ' ' };
	int cnt = 3;
	_os_write(cur_path, buf, &cnt);
}

u8 chdir(const char *path) {
	struct _registers_6809 reg;
	reg.a = 1; // read only
	reg.x = (u16)path;
	_os_syscall(I$ChgDir, &reg);
	return reg.cc & 1;
}

char *readdir(u8 id) {
	static char buf[32];
	do {
		int cnt = 32;
		if (_os_read(id, buf, &cnt) || cnt != 32)
			return NULL; // end of directory
		for (u8 *p = buf; p < &buf[29]; p++) // see p.52
			if (*p & 0x80) {
				*p &= 0x7f;
				break;
			}
	} while (!*buf || *buf == '.');
	return buf;
}

void *alloc(u16 size, u8 clear) {
	static void *bound;
	struct _registers_6809 reg;
	if (!bound) {
		reg.a = reg.b = 0;
		_os_syscall(F$Mem, &reg);
		if (reg.cc & 1) exit(1);
		bound = (void *)reg.y;
	}
	void *r = bound;
	bound += size;
	reg.a = (u8)(bound >> 8);
	reg.b = (u8)bound;
	_os_syscall(F$Mem, &reg);
	if (reg.cc & 1) exit(1);
	logstart();
	_printf("alloc: %04x %04x\n", r, bound);
	logend();
	if (clear) memset(r, 0, size);
	return r;
}

s8 sini(u8 t) {
	static const s8 tbl[] = {
		0x00, 0x03, 0x06, 0x09, 0x0c, 0x10, 0x13, 0x16, 
		0x19, 0x1c, 0x1f, 0x22, 0x25, 0x28, 0x2b, 0x2e, 
		0x31, 0x33, 0x36, 0x39, 0x3c, 0x3f, 0x41, 0x44, 
		0x47, 0x49, 0x4c, 0x4e, 0x51, 0x53, 0x55, 0x58, 
		0x5a, 0x5c, 0x5e, 0x60, 0x62, 0x64, 0x66, 0x68, 
		0x6a, 0x6b, 0x6d, 0x6f, 0x70, 0x71, 0x73, 0x74, 
		0x75, 0x76, 0x78, 0x79, 0x7a, 0x7a, 0x7b, 0x7c, 
		0x7d, 0x7d, 0x7e, 0x7e, 0x7e, 0x7f, 0x7f, 0x7f, 
		0x7f
	};
	return t < 0x40 ? tbl[t] : t < 0x80 ? tbl[0x80 - t] : t < 0xc0 ?  -tbl[t - 0x80] : -tbl[0x100 - t];
}

s8 cosi(u8 t) {
	return sini(t + 0x40);
}
