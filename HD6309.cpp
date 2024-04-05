// HD6309
// Copyright 2022-2024 © Yasuo Kuwahara
// MIT License

#include "HD6309.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	LC, LV, LZ, LN, LI, LH, LF, LE
};

enum {
	MC = 1 << LC, MV = 1 << LV, MZ = 1 << LZ, MN = 1 << LN,
	MI = 1 << LI, MH = 1 << LH, MF = 1 << LF, ME = 1 << LE
};

enum {
	FB = 1, F0, F1, F8, F16, F32, FADD8, FSUB8, FADD16, FSUB16, FMUL, FDIV, FLEFT8, FLEFT16
};

#define M(flag, type)	flag##type = F##type << (L##flag << 2)
enum {
	M(C, B), M(C, 0), M(C, 1), M(C, 8), M(C, 16), M(C, ADD8), M(C, SUB8), M(C, ADD16), M(C, SUB16),
	M(C, LEFT8), M(C, LEFT16), M(C, MUL), M(C, DIV),
	M(V, B), M(V, 0), M(V, 1), M(V, 8), M(V, 16), M(V, ADD8), M(V, SUB8), M(V, ADD16), M(V, SUB16),
	M(V, LEFT8), M(V, LEFT16),
	M(Z, B), M(Z, 0), M(Z, 1), M(Z, 8), M(Z, 16), M(Z, 32),
	M(N, B), M(N, 0), M(N, 8), M(N, 16),
	M(H, B), M(H, ADD8)
};

#define fmnt()			(++fp < fbuf + FBUFMAX ? 0 : ResolvFlags())
#define fclr()			(fp->dm = N0 | Z1 | V0 | C0, fmnt())
#define fnz8(x)			(fp->dm = N8 | Z8, fp->a = (x), fmnt())
#define fnz16(x)		(fp->dm = N16 | Z16, fp->a = (x), fmnt())
#define fz16(x)			(fp->dm = Z16, fp->a = (x), fmnt())
#define fmov8(x)		(fp->dm = N8 | Z8 | V0, fp->a = (x), fmnt())
#define fmov16(x)		(fp->dm = N16 | Z16 | V0, fp->a = (x), fmnt())
#define fmov32(x, y)	(fp->dm = N16 | Z32 | V0, fp->b = (y), fp->a = (x), fmnt())
#define fadd8(x, y, z)	(fp->dm = HADD8 | N8 | Z8 | VADD8 | CADD8, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fadd16(x, y, z)	(fp->dm = N16 | Z16 | VADD16 | CADD16, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fsub8(x, y, z)	(fp->dm = N8 | Z8 | VSUB8 | CSUB8, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fsub16(x, y, z)	(fp->dm = N16 | Z16 | VSUB16 | CSUB16, fp->b = (x), fp->s = (y), fp->a = (z), fmnt())
#define fmul(x)			(fp->dm = Z16 | CMUL, fp->a = (x), fmnt())
#define fdiv(x, y)		(fp->dm = N8 | Z8 | VB | CDIV, fp->b = (x), fp->a = (y), fmnt())
#define fdiv16(x, y)	(fp->dm = N16 | Z16 | VB | CDIV, fp->b = (x), fp->a = (y), fmnt())
#define fdivro()		(fp->dm = N0 | Z0 | V1 | C0, fmnt())
#define fleft8(x, y)	(fp->dm = N8 | Z8 | VLEFT8 | CLEFT8, fp->b = (x), fp->a = (y), fmnt())
#define fleft16(x, y)	(fp->dm = N16 | Z16 | VLEFT16 | CLEFT16, fp->b = (x), fp->a = (y), fmnt())
#define fright8(x, y)	(fp->dm = N8 | Z8 | CB, fp->b = (x), fp->a = (y), fmnt())
#define fright16(x, y)	(fp->dm = N16 | Z16 | CB, fp->b = (x), fp->a = (y), fmnt())
#define finc8(x, y)		(fp->dm = N8 | Z8 | VADD8, fp->b = (x), fp->s = 0, fp->a = (y), fmnt())
#define finc16(x, y)	(fp->dm = N16 | Z16 | VADD16, fp->b = (x), fp->s = 0, fp->a = (y), fmnt())
#define fdec8(x, y)		(fp->dm = N8 | Z8 | VSUB8, fp->b = (x), fp->s = 0, fp->a = (y), fmnt())
#define fdec16(x, y)	(fp->dm = N16 | Z16 | VSUB16, fp->b = (x), fp->s = 0, fp->a = (y), fmnt())
#define fneg8(x)		(fp->dm = N8 | Z8 | V8 | C8, fp->a = (x), fmnt())
#define fneg16(x)		(fp->dm = N16 | Z16 | V16 | C16, fp->a = (x), fmnt())
#define fcom8(x)		(fp->dm = N8 | Z8 | V0 | C1, fp->a = (x), fmnt())
#define fcom16(x)		(fp->dm = N16 | Z16 | V0 | C1, fp->a = (x), fmnt())
#define fdaa(x, y)		(fp->dm = N8 | Z8 | CADD8, fp->b = (x), fp->a = (y), fmnt())

#define CY			(ResolvC())
#define CLOCK(x)	(clock += (x))

#define A	r[1]
#define B	r[0]
#define D	((uint16_t &)r[0])
#define X	((uint16_t &)r[2])
#define Y	((uint16_t &)r[4])
#define U	((uint16_t &)r[6])
#define S	((uint16_t &)r[8])
#define PC	((uint16_t &)r[10])
#define E	r[13]
#define F	r[12]
#define W	((uint16_t &)r[12])
#define V	((uint16_t &)r[14])

#define da()		(dp << 8 | imm8())

enum { W_SYNC = 1, W_CWAI };

static constexpr uint8_t eclk1[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 3, 6, // 0x00
	1, 1, 2, 4, 4, 1, 5, 9, 1, 2, 3, 1, 3, 2, 8, 6, // 0x10
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 0x20
	4, 4, 4, 4, 5, 5, 5, 5, 1, 5, 3, 3,20,11, 1, 7, // 0x30
	2, 1, 1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, // 0x40
	2, 1, 1, 2, 2, 1, 2, 2, 2, 2, 2, 1, 2, 2, 1, 2, // 0x50
	6, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 7, 6, 6, 3, 6, // 0x60
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 4, 7, // 0x70
	2, 2, 2, 4, 2, 2, 2, 1, 2, 2, 2, 2, 4, 7, 3, 1, // 0x80
	4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 7, 5, 5, // 0x90
	4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 6, 7, 5, 5, // 0xa0
	5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 5, 7, 8, 6, 6, // 0xb0
	2, 2, 2, 4, 2, 2, 2, 1, 2, 2, 2, 2, 3, 5, 3, 1, // 0xc0
	4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, // 0xd0
	4, 4, 4, 6, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, // 0xe0
	5, 5, 5, 7, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, // 0xf0
};

static constexpr uint8_t nclk1[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	5, 6, 6, 5, 5, 6, 5, 5, 5, 5, 5, 6, 5, 4, 2, 5, // 0x00
	1, 1, 1, 3, 4, 1, 4, 7, 1, 1, 3, 1, 3, 1, 5, 4, // 0x10
	3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 0x20
	4, 4, 4, 4, 4, 4, 4, 4, 1, 4, 1, 3,22,10, 1, 7, // 0x30
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50
	6, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 7, 6, 5, 3, 6, // 0x60
	6, 7, 7, 6, 6, 7, 6, 6, 6, 6, 6, 7, 6, 5, 3, 6, // 0x70
	2, 2, 2, 3, 2, 2, 2, 1, 2, 2, 2, 2, 3, 6, 3, 1, // 0x80
	3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 6, 4, 4, // 0x90
	4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 6, 5, 5, // 0xa0
	4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 7, 5, 5, // 0xb0
	2, 2, 2, 3, 2, 2, 2, 1, 2, 2, 2, 2, 3, 5, 3, 1, // 0xc0
	3, 3, 3, 4, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, // 0xd0
	4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, // 0xe0
	4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, // 0xf0
};

static constexpr uint8_t eclk2[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1000
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1010
	2, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, // 0x1020
	4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 2, 2, 2, 8, // 0x1030
	3, 2, 2, 3, 3, 2, 3, 3, 3, 3, 3, 2, 3, 3, 2, 3, // 0x1040
	2, 2, 2, 3, 3, 2, 3, 2, 2, 3, 3, 2, 3, 3, 2, 3, // 0x1050
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1060
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1070
	5, 5, 5, 5, 5, 5, 4, 2, 5, 5, 5, 5, 5, 2, 4, 2, // 0x1080
	7, 7, 7, 7, 7, 7, 6, 6, 7, 7, 7, 7, 7, 2, 6, 6, // 0x1090
	7, 7, 7, 7, 7, 7, 6, 6, 7, 7, 7, 7, 7, 2, 6, 6, // 0x10a0
	8, 8, 8, 8, 8, 8, 7, 7, 8, 8, 8, 8, 8, 2, 7, 7, // 0x10b0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 0x10c0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 6, 6, // 0x10d0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 6, 6, // 0x10e0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 9, 9, 7, 7, // 0x10f0
};

static constexpr uint8_t nclk2[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1000
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1010
	2, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, // 0x1020
	4, 4, 4, 4, 4, 4, 4, 4, 6, 6, 6, 6, 2, 2, 2, 8, // 0x1030
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1040
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1050
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1060
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1070
	4, 4, 4, 4, 4, 4, 4, 2, 4, 4, 4, 4, 4, 2, 4, 2, // 0x1080
	5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 2, 5, 5, // 0x1090
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 6, 6, // 0x10a0
	6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 6, 6, // 0x10b0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, // 0x10c0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 7, 7, 5, 5, // 0x10d0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 6, 6, // 0x10e0
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 8, 8, 6, 6, // 0x10f0
};

static constexpr uint8_t eclk3[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1100
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1110
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1120
	7, 7, 7, 7, 7, 7, 7, 8, 3, 3, 3, 3, 4, 5, 2, 8, // 0x1130
	2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 3, 2, 3, 3, 2, 3, // 0x1140
	2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 3, 2, 3, 3, 2, 3, // 0x1150
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1160
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1170
	3, 3, 2, 5, 2, 2, 3, 2, 2, 2, 2, 3, 5,25,34,28, // 0x1180
	5, 5, 2, 7, 2, 2, 5, 5, 2, 2, 2, 5, 7,27,36,30, // 0x1190
	5, 5, 2, 7, 2, 2, 5, 5, 2, 2, 2, 5, 7,27,36,30, // 0x11a0
	6, 6, 2, 8, 2, 2, 6, 6, 2, 2, 2, 6, 8,28,37,31, // 0x11b0
	3, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, // 0x11c0
	5, 5, 2, 2, 2, 2, 5, 5, 2, 2, 2, 5, 2, 2, 2, 2, // 0x11d0
	5, 5, 2, 2, 2, 2, 5, 5, 2, 2, 2, 5, 2, 2, 2, 2, // 0x11e0
	6, 6, 2, 2, 2, 2, 6, 6, 2, 2, 2, 6, 2, 2, 2, 2, // 0x11f0
};

static constexpr uint8_t nclk3[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1100
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1110
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1120
	6, 6, 6, 6, 6, 6, 6, 7, 3, 3, 3, 3, 4, 5, 2, 8, // 0x1130
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1140
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1150
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1160
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 0x1170
	3, 3, 2, 4, 2, 2, 3, 2, 2, 2, 2, 3, 4,25,34,28, // 0x1180
	4, 4, 2, 5, 2, 2, 4, 4, 2, 2, 2, 4, 5,26,35,29, // 0x1190
	5, 5, 2, 6, 2, 2, 5, 5, 2, 2, 2, 5, 6,27,36,30, // 0x11a0
	5, 5, 2, 6, 2, 2, 5, 5, 2, 2, 2, 5, 6,27,36,30, // 0x11b0
	3, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, 3, 2, 2, 2, 2, // 0x11c0
	4, 4, 2, 2, 2, 2, 4, 4, 2, 2, 2, 4, 2, 2, 2, 2, // 0x11d0
	5, 5, 2, 2, 2, 2, 5, 5, 2, 2, 2, 5, 2, 2, 2, 2, // 0x11e0
	5, 5, 2, 2, 2, 2, 5, 5, 2, 2, 2, 5, 2, 2, 2, 2, // 0x11f0
};

static constexpr uint8_t eclkea[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x00
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x10
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x20
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x30
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
	2, 3, 2, 3, 0, 1, 1, 1, 1, 4, 1, 4, 1, 5, 1, 0, // 0x80
	3, 6, 0, 6, 3, 4, 4, 4, 4, 7, 4, 7, 4, 8, 4, 5, // 0x90
	2, 3, 2, 3, 0, 1, 1, 1, 1, 4, 1, 4, 1, 5, 1, 2, // 0xa0
	5, 6, 0, 6, 3, 4, 4, 4, 4, 7, 4, 7, 4, 8, 4, 0, // 0xb0
	2, 3, 2, 3, 0, 1, 1, 1, 1, 4, 1, 4, 1, 5, 1, 1, // 0xc0
	4, 6, 0, 6, 3, 4, 4, 4, 4, 7, 4, 7, 4, 8, 4, 0, // 0xd0
	2, 3, 2, 3, 0, 1, 1, 1, 1, 4, 1, 4, 1, 5, 1, 1, // 0xe0
	4, 6, 0, 6, 3, 4, 4, 4, 4, 7, 4, 7, 4, 8, 4, 0, // 0xf0
};

static constexpr uint8_t nclkea[] = {
//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x00
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x10
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x20
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x30
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x40
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x50
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x60
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0x70
	1, 2, 1, 2, 0, 1, 1, 1, 1, 3, 1, 2, 1, 3, 1, 0, // 0x80
	3, 5, 0, 5, 3, 4, 4, 4, 4, 6, 4, 5, 4, 6, 4, 4, // 0x90
	1, 2, 1, 2, 0, 1, 1, 1, 1, 3, 1, 2, 1, 3, 1, 2, // 0xa0
	5, 5, 0, 5, 3, 4, 4, 4, 4, 6, 4, 5, 4, 6, 4, 0, // 0xb0
	1, 2, 1, 2, 0, 1, 1, 1, 1, 3, 1, 2, 1, 3, 1, 1, // 0xc0
	4, 5, 0, 5, 3, 4, 4, 4, 4, 6, 4, 5, 4, 6, 4, 0, // 0xd0
	1, 2, 1, 2, 0, 1, 1, 1, 1, 3, 1, 2, 1, 3, 1, 1, // 0xe0
	4, 5, 0, 5, 3, 4, 4, 4, 4, 6, 4, 5, 4, 6, 4, 0, // 0xf0
};

static void error() {
	fprintf(stderr, "internal error\n");
	exit(1);
}

HD6309::HD6309() : clktbl1(eclk1), clktbl2(eclk2), clktbl3(eclk3), clktblea(eclkea) {
#if HD6309_TRACE
	memset(tracebuf, 0, sizeof(tracebuf));
	tracep = tracebuf;
#endif
}

void HD6309::Reset() {
	dp = md = irq = waitflags = 0;
	memset(r, 0, sizeof(r));
	SetupFlags(MF | MI);
	PC = ld16(0xfffe);
}

uint16_t HD6309::ea() { // indexed addressing
	int8_t o = imm8();
	CLOCK(clktblea[o]);
	auto ir = [&]()->uint16_t & { return (uint16_t &)r[(o >> 4 & 6) + 2]; };
	if (!(o & 0x80))
		return ir() + ((int16_t)(o << 11) >> 11); // 5bit offset
	uint16_t adr;
	switch (o & 0xf) {
		case 0x1: adr = ir(); ir() += 2; break;
		case 0x2: adr = --ir(); break;
		case 0x3: adr = ir() -= 2; break;
		case 0x4: adr = ir(); break;
		case 0x5: adr = ir() + (int8_t)B; break;
		case 0x6: adr = ir() + (int8_t)A; break;
		case 0x7: adr = ir() + (int8_t)E; break;
		case 0x8: adr = ir() + (int8_t)imm8(); break;
		case 0x9: adr = ir() + imm16(); break;
		case 0xa: adr = ir() + (int8_t)F; break;
		case 0xb: adr = ir() + D; break;
		case 0xc: adr = (int8_t)imm8(); adr += PC; break;
		case 0xd: adr = imm16(); adr += PC; break;
		case 0xe: adr = ir() + W; break;
		default: // 0 or 0xf
			switch (o & 0x18) {
				case 0x00: adr = ir()++; break;
				case 0x18: adr = imm16(); break;
				default:
					switch (o & 0x60) {
						default: adr = W; break;
						case 0x20: adr = W + imm16(); break;
						case 0x40: adr = W; W += 2; break;
						case 0x60: adr = W -= 2; break;
					}
					break;
			}
			break;
	}
	return o & 0x10 ? ld16(adr) : adr;
}

template<typename T> void HD6309::bitTransfer(T op) {
	uint8_t pb = imm8(), a = pb & 7, d = 0;
	switch (pb & 0xc0) {
		case 0: d = ResolvFlags(); break;
		case 0x40: d = A; break;
		case 0x80: d = B; break;
	}
	d = (d & ~(1 << a)) | (op(d >> a, ld8(da()) >> (pb >> 3 & 7)) & 1) << a;
	switch (pb & 0xc0) {
		case 0: SetupFlags(d); break;
		case 0x40: A = d; break;
		case 0x80: B = d; break;
	}
}

template<typename T1, typename T2> void HD6309::interRegister(T1 f8, T2 f16) {
	auto rsels = [&](uint8_t sel)->uint16_t {
		if (!(sel & 0x80)) return (uint16_t &)r[sel >> 4 << 1];
		switch (sel & 0xf0) {
			case 0x80: return sel & 8 ? A : D;
			case 0x90: return sel & 8 ? B : D;
			case 0xa0: return ResolvFlags();
			case 0xb0: return sel & 8 ? dp : dp << 8;
			case 0xe0: return sel & 8 ? E : W;
			case 0xf0: return sel & 8 ? F : W;
		}
		return 0;
	};
	auto rseld = [&](uint8_t sel)->uint16_t {
		sel &= 0xf;
		if (!(sel & 8)) return (uint16_t &)r[sel << 1];
		switch (sel) {
			case 8: return A;
			case 9: return B;
			case 10: return ResolvFlags();
			case 11: return dp;
			case 14: return E;
			case 15: return F;
		}
		return 0;
	};
	auto wseli = [&](uint8_t sel, uint16_t data) {
		sel &= 0xf;
		if (!(sel & 8)) {
			(uint16_t &)r[sel << 1] = data;
			return;
		}
		switch (sel) {
			case 8: A = data; break;
			case 9: B = data; break;
			case 10: SetupFlags(data); break;
			case 11: dp = data; break;
			case 14: E = data; break;
			case 15: F = data; break;
		}
	};
	uint8_t sel = imm8();
	if (sel & 8) {
		uint8_t d = rseld(sel);
		f8(d, rsels(sel));
		wseli(sel, d);
	}
	else {
		uint16_t d = rseld(sel);
		f16(d, rsels(sel));
		wseli(sel, d);
	}
}

void HD6309::transfer(uint8_t op, uint8_t t8) {
	auto rsel = [&](uint8_t sel)->uint16_t {
		uint8_t t;
		switch (sel &= 0xf) {
			case 8: return A << 8 | A;
			case 9: return B << 8 | B;
			case 10: t = ResolvFlags(); return t << 8 | t;
			case 11: return dp << 8 | dp;
			case 12: case 13: return 0;
			case 14: return E << 8 | E;
			case 15: return F << 8 | F;
		}
		return (uint16_t &)r[sel << 1];
	};
	auto wsel = [&](uint8_t sel, uint16_t data) {
		switch (sel &= 0xf) {
			case 8: A = data >> 8; return;
			case 9: B = data; return;
			case 10: SetupFlags(data); return;
			case 11: dp = data >> 8; return;
			case 12: case 13: return;
			case 14: E = data >> 8; return;
			case 15: F = data; return;
		}
		(uint16_t &)r[sel << 1] = data;
	};
	if (op & 1) wsel(t8, rsel(t8 >> 4)); // tfr
	else { // exg
		uint16_t t = rsel(t8);
		wsel(t8, rsel(t8 >> 4));
		wsel(t8 >> 4, t);
	}
}

int HD6309::Execute(int n) {
	auto psh = [&](bool s, uint8_t t, bool wp = false) {
		uint16_t &p = s ? S : U;
		if (t & 0x80) st16r(p -= 2, PC);
		if (t & 0x40) st16r(p -= 2, s ? U : S);
		if (t & 0x20) st16r(p -= 2, Y);
		if (t & 0x10) st16r(p -= 2, X);
		if (t & 0x08) st8(--p, dp);
		if (wp) { st16r(p -= 2, W); CLOCK(2); }
		if (t & 0x04) st8(--p, B);
		if (t & 0x02) st8(--p, A);
		if (t & 0x01) st8(--p, ResolvFlags());
		CLOCK(__builtin_popcount(t) + __builtin_popcount(t & 0xf0));
	};
	auto pul = [&](bool s, uint8_t t, bool wp = false) {
		uint16_t &p = s ? S : U;
		if (t & 0x01) SetupFlags(ld8(p++));
		if (t & 0x02) A = ld8(p++);
		if (t & 0x04) B = ld8(p++);
		if (wp) { W = ld16(p); p += 2; CLOCK(2); }
		if (t & 0x08) dp = ld8(p++);
		if (t & 0x10) { X = ld16(p); p += 2; }
		if (t & 0x20) { Y = ld16(p); p += 2; }
		if (t & 0x40) { (s ? U : S) = ld16(p); p += 2; }
		if (t & 0x80) { PC = ld16(p); p += 2; }
		CLOCK(__builtin_popcount(t) + __builtin_popcount(t & 0xf0));
	};
	auto trap = [&](uint16_t vec = 0xfff0, uint8_t p = 0xff, uint8_t m = MF | MI) {
		if (p) {
			if (p == 0xff) intflags |= ME;
			else intflags &= ~ME;
			psh(true, p, md & 1 && p == 0xff);
		}
		intflags |= m;
		PC = ld16(vec);
	};
	// instructions
	auto add8 = [&](uint8_t &d, uint8_t s) { fadd8(d, s, d += s); };
	auto add16 = [&](uint16_t &d, uint16_t s) { fadd16(d, s, d += s); };
	auto adc8 = [&](uint8_t &d, uint8_t s) { fadd8(d, s, d += s + CY); };
	auto adc16 = [&](uint16_t &d, uint16_t s) { fadd16(d, s, d += s + CY); };
	auto sub8 = [&](uint8_t &d, uint8_t s) { fsub8(d, s, d -= s); };
	auto sub16 = [&](uint16_t &d, uint16_t s) { fsub16(d, s, d -= s); };
	auto sbc8 = [&](uint8_t &d, uint8_t s) { fsub8(d, s, d -= s + CY); };
	auto sbc16 = [&](uint16_t &d, uint16_t s) { fsub16(d, s, d -= s + CY); };
	auto cmp8 = [&](uint8_t d, uint8_t s) { fsub8(d, s, d - s); };
	auto cmp16 = [&](uint16_t d, uint16_t s) { fsub16(d, s, d - s); };
	auto muld = [&](int16_t s) { int32_t t = (int16_t)D * s; fnz16(D = t >> 16); W = t; };
	auto divd = [&](int8_t s) {
		if (!s) {
			md |= 0x80;
			trap();
			return;
		}
		int16_t t = (int16_t)D / s, u = t >> 8;
		if (u && u != -1) {
			fdivro();
			CLOCK(-13);
			return;
		}
		int v = t != (int8_t)t;
		A = (int16_t)D % s;
		B = t;
		fdiv(-v, B);
		CLOCK(-v);
	};
	auto divq = [&](int16_t s) {
		if (!s) {
			md |= 0x80;
			trap();
			return;
		}
		int32_t d = D << 16 | W, t = d / s, u = t >> 16;
		if (u && u != -1) {
			fdivro();
			CLOCK(-21);
			return;
		}
		int v = t != (int16_t)t;
		D = d % s;
		W = t;
		fdiv16(-v, W);
		CLOCK(-v);
	};
	auto incm = [&](uint16_t a) { uint8_t t = ld8(a); finc8(t, ++t); st8(a, t); };
	auto decm = [&](uint16_t a) { uint8_t t = ld8(a); fdec8(t, --t); st8(a, t); };
	auto negm = [&](uint16_t a) { uint8_t t = -ld8(a); fneg8(t); st8(a, t); };
	auto clrm = [&](uint16_t a) { st8(a, 0); fclr(); };
	auto tstm = [&](uint16_t a) { fmov8(ld8(a)); };
	auto and8 = [&](uint8_t &d, uint8_t s) { fmov8(d &= s); };
	auto and16 = [&](uint16_t &d, uint16_t s) { fmov16(d &= s); };
	auto or8 = [&](uint8_t &d, uint8_t s) { fmov8(d |= s); };
	auto or16 = [&](uint16_t &d, uint16_t s) { fmov16(d |= s); };
	auto eor8 = [&](uint8_t &d, uint8_t s) { fmov8(d ^= s); };
	auto eor16 = [&](uint16_t &d, uint16_t s) { fmov16(d ^= s); };
	auto comm = [&](uint16_t a) { uint8_t t = ~ld8(a); fcom8(t); st8(a, t); };
	auto lsl = [&](uint8_t &r) { fleft8(r, r <<= 1); };
	auto lsr = [&](uint8_t &r) { fright8(r, r >>= 1); };
	auto asr = [&](uint8_t &r) { fright8(r, r = (int8_t)r >> 1); };
	auto rol = [&](uint8_t &r) { fleft8(r, r = r << 1 | CY); };
	auto ror = [&](uint8_t &r) { fright8(r, r = r >> 1 | CY << 7); };
	auto lslm = [&](uint16_t a) { uint8_t t = ld8(a); fleft8(t, t <<= 1); st8(a, t); };
	auto lsrm = [&](uint16_t a) { uint8_t t = ld8(a); fright8(t, t >>= 1); st8(a, t); };
	auto asrm = [&](uint16_t a) { int8_t t = ld8(a); fright8(t, t >>= 1); st8(a, t); };
	auto rolm = [&](uint16_t a) { uint8_t t = ld8(a); fleft8(t, t = t << 1 | CY); st8(a, t); };
	auto rorm = [&](uint16_t a) { uint8_t t = ld8(a); fright8(t, t = t >> 1 | CY << 7); st8(a, t); };
	auto jsr = [&](uint16_t a) { st16r(S -= 2, PC); PC = a; };
	auto ldq = [&](uint32_t s) { fnz16(D = s >> 16); W = s; }; // undocumented
	auto stq = [&](uint16_t a) { fmov32(D, W); st16(a, D); st16(a + 2, W); };
	auto aim = [&](uint8_t m, uint16_t a) { fmov8(m &= ld8(a)); st8(a, m); };
	auto oim = [&](uint8_t m, uint16_t a) { fmov8(m |= ld8(a)); st8(a, m); };
	auto eim = [&](uint8_t m, uint16_t a) { fmov8(m ^= ld8(a)); st8(a, m); };
	auto tim = [&](uint8_t m, uint16_t a) { fmov8(ld8(a) & m); };
	auto tfm = [&](int s, int d) {
		fz16(W);
		if (!W) {
			PC++;
			CLOCK(6);
			return;
		}
		int sel = imm8(), si = sel >> 4, di = sel & 0xf;
		if (si > 4 || di > 4) {
			md |= 0x40;
			trap();
			return;
		}
		W--;
		uint16_t &sr = (uint16_t &)r[si << 1], &dr = (uint16_t &)r[di << 1];
		st8(dr, ld8(sr));
		sr += s;
		dr += d;
		PC -= 3;
	};
	// conditions
	auto cc = [&]{ return !ResolvC(); };
	auto lo = [&]{ return ResolvC(); };
	auto vc = [&]{ return !ResolvV(); };
	auto vs = [&]{ return ResolvV(); };
	auto ne = [&]{ return !ResolvZ(); };
	auto eq = [&]{ return ResolvZ(); };
	auto pl = [&]{ return !ResolvN(); };
	auto mi = [&]{ return ResolvN(); };
	auto hi = [&]{ return ~(ResolvZ() >> LZ | ResolvC() >> LC) & 1; };
	auto ls = [&]{ return (ResolvZ() >> LZ | ResolvC() >> LC) & 1; };
	auto ge = [&]{ return ~(ResolvN() >> LN ^ ResolvV() >> LV) & 1; };
	auto lt = [&]{ return (ResolvN() >> LN ^ ResolvV() >> LV) & 1; };
	auto gt = [&]{ return ~((ResolvN() >> LN ^ ResolvV() >> LV) | ResolvZ() >> LZ) & 1; };
	auto le = [&]{ return ((ResolvN() >> LN ^ ResolvV() >> LV) | ResolvZ() >> LZ) & 1; };
	//
	clock = 0;
	do {
		if (irq) {
			irq &= ~M_SYNC;
			if (waitflags & W_SYNC) {
				waitflags &= ~W_SYNC;
				PC++;
			}
			if (irq & M_NMI || (irq & M_FIRQ && !(intflags & MF)) || (irq & M_IRQ && !(intflags & MI))) {
				uint8_t p = waitflags & W_CWAI ? 0 : 0xff;
				if (!p) {
					waitflags &= ~W_CWAI;
					PC += 2;
				}
				if (irq & M_NMI) {
					irq &= ~M_NMI;
					trap(0xfffc, p);
					CLOCK(7);
				}
				else if (irq & M_FIRQ && !(intflags & MF)) {
					irq &= ~M_FIRQ;
					trap(0xfff6, md & 2 ? p : p & 0x81);
					CLOCK(7);
				}
				else {
					irq &= ~M_IRQ;
					trap(0xfff8, p, MI);
					CLOCK(7);
				}
			}
		}
#if HD6309_TRACE
		tracep->r[5] = PC;
		tracep->index = tracep->opn = 0;
#endif
		int32_t t32;
		int8_t t8, t;
		int16_t t16;
		uint8_t op = imm8();
		switch (op) {
			case 0x00: negm(da()); break;
			case 0x01: t8 = imm8(); oim(t8, da()); break;
			case 0x02: t8 = imm8(); aim(t8, da()); break;
			case 0x03: comm(da()); break;
			case 0x04: lsrm(da()); break;
			case 0x05: t8 = imm8(); eim(t8, da()); break;
			case 0x06: rorm(da()); break;
			case 0x07: asrm(da()); break;
			case 0x08: lslm(da()); break;
			case 0x09: rolm(da()); break;
			case 0x0a: decm(da()); break;
			case 0x0b: t8 = imm8(); tim(t8, da()); break;
			case 0x0c: incm(da()); break;
			case 0x0d: tstm(da()); break;
			case 0x0e: PC = da(); break;
			case 0x0f: clrm(da()); break;
			case 0x12: break; // nop
			case 0x13: waitflags |= W_SYNC; PC--; break;
			case 0x14: D = (int16_t)W >> 15; fnz16(W); break;
			case 0x16: t16 = imm16(); PC += t16; break;
			case 0x17: t16 = imm16(); st16r(S -= 2, PC); PC += t16; break;
			case 0x19: // daa
				t8 = A;
				if (CY || (A & 0xf0) > 0x90 || ((A & 0xf0) > 0x80 && (A & 0xf) > 9)) t8 += 0x60;
				if (ResolvH() || (A & 0xf) > 9) t8 += 6;
				fdaa(A, A = t8);
				break;
			case 0x1a: SetupFlags(ResolvFlags() | imm8()); break;
			case 0x1c: SetupFlags(ResolvFlags() & imm8()); break;
			case 0x1d: fnz8(D = (int8_t)B); break;
			case 0x1e: case 0x1f: transfer(op, imm8()); break;
			case 0x20: bcc([]{ return true; }); break; // bra
			case 0x21: bcc([]{ return false; }); break; // brn
			case 0x22: bcc(hi); break;
			case 0x23: bcc(ls); break;
			case 0x24: bcc(cc); break;
			case 0x25: bcc(lo); break;
			case 0x26: bcc(ne); break;
			case 0x27: bcc(eq); break;
			case 0x28: bcc(vc); break;
			case 0x29: bcc(vs); break;
			case 0x2a: bcc(pl); break;
			case 0x2b: bcc(mi); break;
			case 0x2c: bcc(ge); break;
			case 0x2d: bcc(lt); break;
			case 0x2e: bcc(gt); break;
			case 0x2f: bcc(le); break;
			case 0x30: fz16(X = ea()); break;
			case 0x31: fz16(Y = ea()); break;
			case 0x32: S = ea(); break;
			case 0x33: U = ea(); break;
			case 0x34: psh(true, imm8()); break;
			case 0x35: pul(true, imm8()); break;
			case 0x36: psh(false, imm8()); break;
			case 0x37: pul(false, imm8()); break;
			case 0x39: PC = ld16(S); S += 2; break;
			case 0x3a: X += B; break;
			case 0x3b: // rti
				SetupFlags(ld8(S++));
				t8 = intflags & ME;
				pul(true, t8 ? 0xfe : 0x80, md & 1 && t8); break;
			case 0x3c:
				if (waitflags & W_CWAI) PC--;
				else {
					waitflags |= W_CWAI;
					SetupFlags((ResolvFlags() & imm8()) | ME);
					psh(true, 0xff, md & 1);
					PC -= 2;
				}
				break;
			case 0x3d: fmul(D = A * B); break;
			case 0x3f: trap(0xfffa); break; // swi
			case 0x40: fneg8(A = -A); break;
			case 0x43: fcom8(A = ~A); break;
			case 0x44: lsr(A); break;
			case 0x46: ror(A); break;
			case 0x47: asr(A); break;
			case 0x48: lsl(A); break;
			case 0x49: rol(A); break;
			case 0x4a: fdec8(A, --A); break;
			case 0x4c: finc8(A, ++A); break;
			case 0x4d: fmov8(A); break;
			case 0x4f: A = 0; fclr(); break;
			case 0x50: fneg8(B = -B); break;
			case 0x53: fcom8(B = ~B); break;
			case 0x54: lsr(B); break;
			case 0x56: ror(B); break;
			case 0x57: asr(B); break;
			case 0x58: lsl(B); break;
			case 0x59: rol(B); break;
			case 0x5a: fdec8(B, --B); break;
			case 0x5c: finc8(B, ++B); break;
			case 0x5d: fmov8(B); break;
			case 0x5f: B = 0; fclr(); break;
			case 0x60: negm(ea()); break;
			case 0x61: t8 = imm8(); oim(t8, ea()); break;
			case 0x62: t8 = imm8(); aim(t8, ea()); break;
			case 0x63: comm(ea()); break;
			case 0x64: lsrm(ea()); break;
			case 0x65: t8 = imm8(); eim(t8, ea()); break;
			case 0x66: rorm(ea()); break;
			case 0x67: asrm(ea()); break;
			case 0x68: lslm(ea()); break;
			case 0x69: rolm(ea()); break;
			case 0x6a: decm(ea()); break;
			case 0x6b: t8 = imm8(); tim(t8, ea()); break;
			case 0x6c: incm(ea()); break;
			case 0x6d: tstm(ea()); break;
			case 0x6e: PC = ea(); break;
			case 0x6f: clrm(ea()); break;
			case 0x70: negm(imm16()); break;
			case 0x71: t8 = imm8(); oim(t8, imm16()); break;
			case 0x72: t8 = imm8(); aim(t8, imm16()); break;
			case 0x73: comm(imm16()); break;
			case 0x74: lsrm(imm16()); break;
			case 0x75: t8 = imm8(); eim(t8, imm16()); break;
			case 0x76: rorm(imm16()); break;
			case 0x77: asrm(imm16()); break;
			case 0x78: lslm(imm16()); break;
			case 0x79: rolm(imm16()); break;
			case 0x7a: decm(imm16()); break;
			case 0x7b: t8 = imm8(); tim(t8, imm16()); break;
			case 0x7c: incm(imm16()); break;
			case 0x7d: tstm(imm16()); break;
			case 0x7e: PC = imm16(); break;
			case 0x7f: clrm(imm16()); break;
			case 0x80: sub8(A, imm8()); break;
			case 0x81: cmp8(A, imm8()); break;
			case 0x82: sbc8(A, imm8()); break;
			case 0x83: sub16(D, imm16()); break;
			case 0x84: and8(A, imm8()); break;
			case 0x85: fmov8(A & imm8()); break;
			case 0x86: fmov8(A = imm8()); break;
			case 0x88: eor8(A, imm8()); break;
			case 0x89: adc8(A, imm8()); break;
			case 0x8a: or8(A, imm8()); break;
			case 0x8b: add8(A, imm8()); break;
			case 0x8c: cmp16(X, imm16()); break;
			case 0x8d: t8 = imm8(); st16r(S -= 2, PC); PC += t8; break;
			case 0x8e: fmov16(X = imm16()); break;
			case 0x90: sub8(A, ld8(da())); break;
			case 0x91: cmp8(A, ld8(da())); break;
			case 0x92: sbc8(A, ld8(da())); break;
			case 0x93: sub16(D, ld16(da())); break;
			case 0x94: and8(A, ld8(da())); break;
			case 0x95: fmov8(A & ld8(da())); break;
			case 0x96: fmov8(A = ld8(da())); break;
			case 0x97: fmov8(A); st8(da(), A); break;
			case 0x98: eor8(A, ld8(da())); break;
			case 0x99: adc8(A, ld8(da())); break;
			case 0x9a: or8(A, ld8(da())); break;
			case 0x9b: add8(A, ld8(da())); break;
			case 0x9c: cmp16(X, ld16(da())); break;
			case 0x9d: jsr(da()); break;
			case 0x9e: fmov16(X = ld16(da())); break;
			case 0x9f: fmov16(X); st16(da(), X); break;
			case 0xa0: sub8(A, ld8(ea())); break;
			case 0xa1: cmp8(A, ld8(ea())); break;
			case 0xa2: sbc8(A, ld8(ea())); break;
			case 0xa3: sub16(D, ld16(ea())); break;
			case 0xa4: and8(A, ld8(ea())); break;
			case 0xa5: fmov8(A & ld8(ea())); break;
			case 0xa6: fmov8(A = ld8(ea())); break;
			case 0xa7: fmov8(A); st8(ea(), A); break;
			case 0xa8: eor8(A, ld8(ea())); break;
			case 0xa9: adc8(A, ld8(ea())); break;
			case 0xaa: or8(A, ld8(ea())); break;
			case 0xab: add8(A, ld8(ea())); break;
			case 0xac: cmp16(X, ld16(ea())); break;
			case 0xad: jsr(ea()); break;
			case 0xae: fmov16(X = ld16(ea())); break;
			case 0xaf: fmov16(X); st16(ea(), X); break;
			case 0xb0: sub8(A, ld8(imm16())); break;
			case 0xb1: cmp8(A, ld8(imm16())); break;
			case 0xb2: sbc8(A, ld8(imm16())); break;
			case 0xb3: sub16(D, ld16(imm16())); break;
			case 0xb4: and8(A, ld8(imm16())); break;
			case 0xb5: fmov8(A & ld8(imm16())); break;
			case 0xb6: fmov8(A = ld8(imm16())); break;
			case 0xb7: fmov8(A); st8(imm16(), A); break;
			case 0xb8: eor8(A, ld8(imm16())); break;
			case 0xb9: adc8(A, ld8(imm16())); break;
			case 0xba: or8(A, ld8(imm16())); break;
			case 0xbb: add8(A, ld8(imm16())); break;
			case 0xbc: cmp16(X, ld16(imm16())); break;
			case 0xbd: jsr(imm16()); break;
			case 0xbe: fmov16(X = ld16(imm16())); break;
			case 0xbf: fmov16(X); st16(imm16(), X); break;
			case 0xc0: sub8(B, imm8()); break;
			case 0xc1: cmp8(B, imm8()); break;
			case 0xc2: sbc8(B, imm8()); break;
			case 0xc3: add16(D, imm16()); break;
			case 0xc4: and8(B, imm8()); break;
			case 0xc5: fmov8(B & imm8()); break;
			case 0xc6: fmov8(B = imm8()); break;
			case 0xc8: eor8(B, imm8()); break;
			case 0xc9: adc8(B, imm8()); break;
			case 0xca: or8(B, imm8()); break;
			case 0xcb: add8(B, imm8()); break;
			case 0xcc: fmov16(D = imm16()); break;
			case 0xcd: t32 = imm16() << 16; ldq(t32 | imm16()); break;
			case 0xce: fmov16(U = imm16()); break;
			case 0xd0: sub8(B, ld8(da())); break;
			case 0xd1: cmp8(B, ld8(da())); break;
			case 0xd2: sbc8(B, ld8(da())); break;
			case 0xd3: add16(D, ld16(da())); break;
			case 0xd4: and8(B, ld8(da())); break;
			case 0xd5: fmov8(B & ld8(da())); break;
			case 0xd6: fmov8(B = ld8(da())); break;
			case 0xd7: fmov8(B); st8(da(), B); break;
			case 0xd8: eor8(B, ld8(da())); break;
			case 0xd9: adc8(B, ld8(da())); break;
			case 0xda: or8(B, ld8(da())); break;
			case 0xdb: add8(B, ld8(da())); break;
			case 0xdc: fmov16(D = ld16(da())); break;
			case 0xdd: fmov16(D); st16(da(), D); break;
			case 0xde: fmov16(U = ld16(da())); break;
			case 0xdf: fmov16(U); st16(da(), U); break;
			case 0xe0: sub8(B, ld8(ea())); break;
			case 0xe1: cmp8(B, ld8(ea())); break;
			case 0xe2: sbc8(B, ld8(ea())); break;
			case 0xe3: add16(D, ld16(ea())); break;
			case 0xe4: and8(B, ld8(ea())); break;
			case 0xe5: fmov8(B & ld8(ea())); break;
			case 0xe6: fmov8(B = ld8(ea())); break;
			case 0xe7: fmov8(B); st8(ea(), B); break;
			case 0xe8: eor8(B, ld8(ea())); break;
			case 0xe9: adc8(B, ld8(ea())); break;
			case 0xea: or8(B, ld8(ea())); break;
			case 0xeb: add8(B, ld8(ea())); break;
			case 0xec: fmov16(D = ld16(ea())); break;
			case 0xed: fmov16(D); st16(ea(), D); break;
			case 0xee: fmov16(U = ld16(ea())); break;
			case 0xef: fmov16(U); st16(ea(), U); break;
			case 0xf0: sub8(B, ld8(imm16())); break;
			case 0xf1: cmp8(B, ld8(imm16())); break;
			case 0xf2: sbc8(B, ld8(imm16())); break;
			case 0xf3: add16(D, ld16(imm16())); break;
			case 0xf4: and8(B, ld8(imm16())); break;
			case 0xf5: fmov8(B & ld8(imm16())); break;
			case 0xf6: fmov8(B = ld8(imm16())); break;
			case 0xf7: fmov8(B); st8(imm16(), B); break;
			case 0xf8: eor8(B, ld8(imm16())); break;
			case 0xf9: adc8(B, ld8(imm16())); break;
			case 0xfa: or8(B, ld8(imm16())); break;
			case 0xfb: add8(B, ld8(imm16())); break;
			case 0xfc: fmov16(D = ld16(imm16())); break;
			case 0xfd: fmov16(D); st16(imm16(), D); break;
			case 0xfe: fmov16(U = ld16(imm16())); break;
			case 0xff: fmov16(U); st16(imm16(), U); break;
			case 0x10:
				op = imm8();
				switch (op) {
					case 0x21: lbcc([]{ return false; }); break; // lbrn
					case 0x22: lbcc(hi); break;
					case 0x23: lbcc(ls); break;
					case 0x24: lbcc(cc); break;
					case 0x25: lbcc(lo); break;
					case 0x26: lbcc(ne); break;
					case 0x27: lbcc(eq); break;
					case 0x28: lbcc(vc); break;
					case 0x29: lbcc(vs); break;
					case 0x2a: lbcc(pl); break;
					case 0x2b: lbcc(mi); break;
					case 0x2c: lbcc(ge); break;
					case 0x2d: lbcc(lt); break;
					case 0x2e: lbcc(gt); break;
					case 0x2f: lbcc(le); break;
					case 0x30: interRegister(add8, add16); break;
					case 0x31: interRegister(adc8, adc16); break;
					case 0x32: interRegister(sub8, sub16); break;
					case 0x33: interRegister(sbc8, sbc16); break;
					case 0x34: interRegister(and8, and16); break;
					case 0x35: interRegister(or8, or16); break;
					case 0x36: interRegister(eor8, eor16); break;
					case 0x37: interRegister(cmp8, cmp16); break;
					case 0x38: st16r(S -= 2, W); break;
					case 0x39: W = ld16(S); S += 2; break;
					case 0x3a: st16r(U -= 2, W); break;
					case 0x3b: W = ld16(U); U += 2; break;
					case 0x3f: trap(0xfff4, 0xff, 0); break; // swi2
					case 0x40: fneg16(D = -D); break;
					case 0x43: fcom16(D = ~D); break;
					case 0x44: fright16(D, D >>= 1); break;
					case 0x46: fright16(D, D = D >> 1 | CY << 15); break;
					case 0x47: fright16(D, D = (int16_t)D >> 1); break;
					case 0x48: fleft16(D, D <<= 1); break;
					case 0x49: fleft16(D, D = D << 1 | CY); break;
					case 0x4a: fdec16(D, --D); break;
					case 0x4c: finc16(D, ++D); break;
					case 0x4d: fmov16(D); break;
					case 0x4f: D = 0; fclr(); break;
					case 0x53: fcom16(W = ~W); break;
					case 0x54: fright16(W, W >>= 1); break;
					case 0x56: fright16(W, W = W >> 1 | CY << 15); break;
					case 0x59: fleft16(W, W = W << 1 | CY); break;
					case 0x5a: fdec16(W, --W); break;
					case 0x5c: finc16(W, ++W); break;
					case 0x5d: fmov16(W); break;
					case 0x5f: W = 0; fclr(); break;
					case 0x80: sub16(W, imm16()); break;
					case 0x81: cmp16(W, imm16()); break;
					case 0x82: sbc16(D, imm16()); break;
					case 0x83: cmp16(D, imm16()); break;
					case 0x84: fmov16(D &= imm16()); break;
					case 0x85: fmov16(D & imm16()); break;
					case 0x86: fmov16(W = imm16()); break;
					case 0x88: eor16(D, imm16()); break;
					case 0x89: adc16(D, imm16()); break;
					case 0x8a: or16(D, imm16()); break;
					case 0x8b: add16(W, imm16()); break;
					case 0x8c: cmp16(Y, imm16()); break;
					case 0x8e: fmov16(Y = imm16()); break;
					case 0x90: sub16(W, ld16(da())); break;
					case 0x91: cmp16(W, ld16(da())); break;
					case 0x92: sbc16(D, ld16(da())); break;
					case 0x93: cmp16(D, ld16(da())); break;
					case 0x94: fmov16(D &= ld16(da())); break;
					case 0x95: fmov16(D & ld16(da())); break;
					case 0x96: fmov16(W = ld16(da())); break;
					case 0x97: fmov16(W); st16(da(), W); break;
					case 0x98: eor16(D, ld16(da())); break;
					case 0x99: adc16(D, ld16(da())); break;
					case 0x9a: or16(D, ld16(da())); break;
					case 0x9b: add16(W, ld16(da())); break;
					case 0x9c: cmp16(Y, ld16(da())); break;
					case 0x9e: fmov16(Y = ld16(da())); break;
					case 0x9f: fmov16(Y); st16(da(), Y); break;
					case 0xa0: sub16(W, ld16(ea())); break;
					case 0xa1: cmp16(W, ld16(ea())); break;
					case 0xa2: sbc16(D, ld16(ea())); break;
					case 0xa3: cmp16(D, ld16(ea())); break;
					case 0xa4: fmov16(D &= ld16(ea())); break;
					case 0xa5: fmov16(D & ld16(ea())); break;
					case 0xa6: fmov16(W = ld16(ea())); break;
					case 0xa7: fmov16(W); st16(ea(), W); break;
					case 0xa8: eor16(D, ld16(ea())); break;
					case 0xa9: adc16(D, ld16(ea())); break;
					case 0xaa: or16(D, ld16(ea())); break;
					case 0xab: add16(W, ld16(ea())); break;
					case 0xac: cmp16(Y, ld16(ea())); break;
					case 0xae: fmov16(Y = ld16(ea())); break;
					case 0xaf: fmov16(Y); st16(ea(), Y); break;
					case 0xb0: sub16(W, ld16(imm16())); break;
					case 0xb1: cmp16(W, ld16(imm16())); break;
					case 0xb2: sbc16(D, ld16(imm16())); break;
					case 0xb3: cmp16(D, ld16(imm16())); break;
					case 0xb4: fmov16(D &= ld16(imm16())); break;
					case 0xb5: fmov16(D & ld16(imm16())); break;
					case 0xb6: fmov16(W = ld16(imm16())); break;
					case 0xb7: fmov16(W); st16(imm16(), W); break;
					case 0xb8: eor16(D, ld16(imm16())); break;
					case 0xb9: adc16(D, ld16(imm16())); break;
					case 0xba: or16(D, ld16(imm16())); break;
					case 0xbb: add16(W, ld16(imm16())); break;
					case 0xbc: cmp16(Y, ld16(imm16())); break;
					case 0xbe: fmov16(Y = ld16(imm16())); break;
					case 0xbf: fmov16(Y); st16(imm16(), Y); break;
					case 0xce: fmov16(S = imm16()); break;
					case 0xdc: t16 = da(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xdd: stq(da()); break;
					case 0xde: fmov16(S = ld16(da())); break;
					case 0xdf: fmov16(S); st16(da(), S); break;
					case 0xec: t16 = ea(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xed: stq(ea()); break;
					case 0xee: fmov16(S = ld16(ea())); break;
					case 0xef: fmov16(S); st16(ea(), S); break;
					case 0xfc: t16 = imm16(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xfd: stq(imm16()); break;
					case 0xfe: fmov16(S = ld16(imm16())); break;
					case 0xff: fmov16(S); st16(imm16(), S); break;
					default: md |= 0x40; trap(); break;
				}
				CLOCK(clktbl2[op]);
				goto skip_clock;
			case 0x11:
				op = imm8();
				switch (op) {
					case 0x30: bitTransfer([](uint8_t d, uint8_t s) { return d & s; }); break;
					case 0x31: bitTransfer([](uint8_t d, uint8_t s) { return d & ~s; }); break;
					case 0x32: bitTransfer([](uint8_t d, uint8_t s) { return d | s; }); break;
					case 0x33: bitTransfer([](uint8_t d, uint8_t s) { return d | ~s; }); break;
					case 0x34: bitTransfer([](uint8_t d, uint8_t s) { return d ^ s; }); break;
					case 0x35: bitTransfer([](uint8_t d, uint8_t s) { return d ^ ~s; }); break;
					case 0x36: bitTransfer([](uint8_t d, uint8_t s) { return s; }); break;
					case 0x37: // stbt
						t8 = imm8();
						t16 = da();
						switch (t8 & 0xc0) {
							case 0: t = ResolvFlags(); break;
							case 0x40: t = A; break;
							case 0x80: t = B; break;
							default: t = 0; break;
						}
						st8(t16, (ld8(t16) & ~(1 << (t8 & 7))) | (t >> (t8 >> 3 & 7) & 1) << (t8 & 7));
						break;
					case 0x38: tfm(1, 1); break;
					case 0x39: tfm(-1, -1); break;
					case 0x3a: tfm(1, 0); break;
					case 0x3b: tfm(0, 1); break;
					case 0x3c: t8 = imm8(); fz16(md & t8); md &= ~t8; break; // bitmd
					case 0x3d: md = (md & 0xfc) | (imm8() & 3); // ldmd
						if (md & 1) { clktbl1 = nclk1; clktbl2 = nclk2; clktbl3 = nclk3; clktblea = nclkea; }
						else { clktbl1 = eclk1; clktbl2 = eclk2; clktbl3 = eclk3; clktblea = eclkea; }
						break;
					case 0x3f: trap(0xfff2, 0xff, 0); break; // swi3
					case 0x43: fcom8(E = ~E); break;
					case 0x4a: fdec8(E, --E); break;
					case 0x4c: finc8(E, ++E); break;
					case 0x4d: fmov8(E); break;
					case 0x4f: E = 0; fclr(); break;
					case 0x53: fcom8(F = ~F); break;
					case 0x5a: fdec8(F, --F); break;
					case 0x5c: finc8(F, ++F); break;
					case 0x5d: fmov8(F); break;
					case 0x5f: F = 0; fclr(); break;
					case 0x80: sub8(E, imm8()); break;
					case 0x81: cmp8(E, imm8()); break;
					case 0x83: cmp16(U, imm16()); break;
					case 0x86: fmov8(E = imm8()); break;
					case 0x8b: add8(E, imm8()); break;
					case 0x8c: cmp16(S, imm16()); break;
					case 0x8d: divd(imm8()); break;
					case 0x8e: divq(imm16()); break;
					case 0x8f: muld(imm16()); break;
					case 0x90: sub8(E, ld8(da())); break;
					case 0x91: cmp8(E, ld8(da())); break;
					case 0x93: cmp16(U, ld16(da())); break;
					case 0x96: fmov8(E = ld8(da())); break;
					case 0x97: fmov8(E); st8(da(), E); break;
					case 0x9b: add8(E, ld8(da())); break;
					case 0x9c: cmp16(S, ld16(da())); break;
					case 0x9d: divd(ld8(da())); break;
					case 0x9e: divq(ld16(da())); break;
					case 0x9f: muld(ld16(da())); break;
					case 0xa0: sub8(E, ld8(ea())); break;
					case 0xa1: cmp8(E, ld8(ea())); break;
					case 0xa3: cmp16(U, ld16(ea())); break;
					case 0xa6: fmov8(E = ld8(ea())); break;
					case 0xa7: fmov8(E); st8(ea(), E); break;
					case 0xab: add8(E, ld8(ea())); break;
					case 0xac: cmp16(S, ld16(ea())); break;
					case 0xad: divd(ld8(ea())); break;
					case 0xae: divq(ld16(ea())); break;
					case 0xaf: muld(ld16(ea())); break;
					case 0xb0: sub8(E, ld8(imm16())); break;
					case 0xb1: cmp8(E, ld8(imm16())); break;
					case 0xb3: cmp16(U, ld16(imm16())); break;
					case 0xb6: fmov8(E = ld8(imm16())); break;
					case 0xb7: fmov8(E); st8(imm16(), E); break;
					case 0xbb: add8(E, ld8(imm16())); break;
					case 0xbc: cmp16(S, ld16(imm16())); break;
					case 0xbd: divd(ld8(imm16())); break;
					case 0xbe: divq(ld16(imm16())); break;
					case 0xbf: muld(ld16(imm16())); break;
					case 0xc0: sub8(F, imm8()); break;
					case 0xc1: cmp8(F, imm8()); break;
					case 0xc6: fmov8(F = imm8()); break;
					case 0xcb: add8(F, imm8()); break;
					case 0xd0: sub8(F, ld8(da())); break;
					case 0xd1: cmp8(F, ld8(da())); break;
					case 0xd6: fmov8(F = ld8(da())); break;
					case 0xd7: fmov8(F); st8(da(), F); break;
					case 0xdb: add8(F, ld8(da())); break;
					case 0xe0: sub8(F, ld8(ea())); break;
					case 0xe1: cmp8(F, ld8(ea())); break;
					case 0xe6: fmov8(F = ld8(ea())); break;
					case 0xe7: fmov8(F); st8(ea(), F); break;
					case 0xeb: add8(F, ld8(ea())); break;
					case 0xf0: sub8(F, ld8(imm16())); break;
					case 0xf1: cmp8(F, ld8(imm16())); break;
					case 0xf6: fmov8(F = ld8(imm16())); break;
					case 0xf7: fmov8(F); st8(imm16(), F); break;
					case 0xfb: add8(F, ld8(imm16())); break;
					default: md |= 0x40; trap(); break;
				}
				CLOCK(clktbl3[op]);
				goto skip_clock;
			default: md |= 0x40; trap(); break;
		}
		CLOCK(clktbl1[op]);
skip_clock:;
#if HD6309_TRACE
		tracep->cc = ResolvFlags();
		for (int i = 0; i < 8; i++)
			if (i != 5) tracep->r[i] = (uint16_t &)r[i << 1];
#if HD6309_TRACE > 1
		if (!waitflags && ++tracep >= tracebuf + TRACEMAX - 1) StopTrace();
#else
		if (!waitflags && ++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
#endif
#endif
	} while (!waitflags && clock < n);
	return waitflags ? 0 : clock - n;
}

int HD6309::ResolvC() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf); p--)
		;
	if (p < fbuf) error();
	switch (sw) {
		case F0:
			break;
		case F1:
			return MC;
		case FB:
			return p->b & MC;
		case F8:
			return (p->a & 0xff) != 0;
		case F16:
			return p->a != 0;
		case FADD8:
			return ((p->s & p->b) | (~p->a & p->b) | (p->s & ~p->a)) >> 7 & MC;
		case FSUB8:
			return ((p->s & ~p->b) | (p->a & ~p->b) | (p->s & p->a)) >> 7 & MC;
		case FADD16:
			return ((p->s & p->b) | (~p->a & p->b) | (p->s & ~p->a)) >> 15 & MC;
		case FSUB16:
			return ((p->s & ~p->b) | (p->a & ~p->b) | (p->s & p->a)) >> 15 & MC;
		case FMUL:
			return p->a >> 7 & 1;
		case FDIV:
			return p->a & 1;
		case FLEFT8:
			return p->b >> 7 & 1;
		case FLEFT16:
			return p->b >> 15;
		default:
			error();
			break;
	}
	return 0;
}

int HD6309::ResolvV() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf0); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 4) {
		case F0:
			break;
		case F1:
			return MV;
		case FB:
			return p->b & MV;
		case F8:
			return ((p->a & 0xff) == 0x80) << LV;
		case F16:
			return (p->a == 0x8000) << LV;
		case FADD8:
			return ((p->b & p->s & ~p->a) | (~p->b & ~p->s & p->a)) >> (7 - LV) & MV;
		case FSUB8:
			return ((p->b & ~p->s & ~p->a) | (~p->b & p->s & p->a)) >> (7 - LV) & MV;
		case FADD16:
			return ((p->b & p->s & ~p->a) | (~p->b & ~p->s & p->a)) >> (15 - LV) & MV;
		case FSUB16:
			return ((p->b & ~p->s & ~p->a) | (~p->b & p->s & p->a)) >> (15 - LV) & MV;
		case FLEFT8:
			return ((p->b >> 6 ^ p->b >> 7) & 1) << LV;
		case FLEFT16:
			return ((p->b >> 14 ^ p->b >> 15) & 1) << LV;
		default:
			error();
			break;
	}
	return 0;
}

int HD6309::ResolvZ() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf00); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 8) {
		case F0:
			break;
		case F1:
			return MZ;
		case FB:
			return p->b & MZ;
		case F8:
			return !(p->a & 0xff) << LZ;
		case F16:
			return !p->a << LZ;
		case F32:
			return !(p->a | p->b) << LZ;
		default:
			error();
			break;
	}
	return 0;
}

int HD6309::ResolvN() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf000); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 12) {
		case F0:
			break;
		case FB:
			return p->b & MN;
		case F8:
			return ((p->a & 0x80) != 0) << LN;
		case F16:
			return ((p->a & 0x8000) != 0) << LN;
		default:
			error();
			break;
	}
	return 0;
}

int HD6309::ResolvH() {
	uint32_t sw = 0;
	FlagDecision *p;
	for (p = fp - 1; p >= fbuf && !(sw = p->dm & 0xf00000); p--)
		;
	if (p < fbuf) error();
	switch (sw >> 20) {
		case FB:
			return p->b & MH;
		case FADD8:
			return (p->b ^ p->s ^ p->a) << (LH - 4) & MH;
		default:
			error();
			break;
	}
	return 0;
}

void HD6309::SetupFlags(int x) {
	fp = fbuf;
	fp->dm = HB | NB | VB | CB | ZB;
	fp++->b = intflags = x;
}

int HD6309::ResolvFlags() {
	int r = ResolvH() | ResolvN() | ResolvV() | ResolvC() | ResolvZ();
	r |= intflags & (ME | MF | MI);
	SetupFlags(r);
	return r;
}

#if HD6309_TRACE
#include <string>
void HD6309::StopTrace() {
	TraceBuffer *endp = tracep;
	int i = 0, j;
	FILE *fo;
	if (!(fo = fopen((std::string(getenv("HOME")) + "/Desktop/trace.txt").c_str(), "w"))) exit(1);
	do {
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
		fprintf(fo, "%4d %04x ", i++, tracep->r[5]);
		for (j = 0; j < tracep->opn; j++) fprintf(fo, "%02x ", tracep->op[j]);
		for (; j < OPMAX; j++) fprintf(fo, "   ");
		fprintf(fo, " %04x %04x %04x %04x %04x %04x %04x %c%c%c%c%c%c%c%c ", // D X Y U S W V CC
				tracep->r[0], tracep->r[1], tracep->r[2], tracep->r[3],
				tracep->r[4], tracep->r[6], tracep->r[7],
				tracep->cc & 0x80 ? 'E' : '-',
				tracep->cc & 0x40 ? 'F' : '-',
				tracep->cc & 0x20 ? 'H' : '-',
				tracep->cc & 0x10 ? 'I' : '-',
				tracep->cc & 0x08 ? 'N' : '-',
				tracep->cc & 0x04 ? 'Z' : '-',
				tracep->cc & 0x02 ? 'V' : '-',
				tracep->cc & 0x01 ? 'C' : '-');
		for (Acs *p = tracep->acs; p < tracep->acs + tracep->index; p++) {
			switch (p->type) {
				case acsLoad8:
					fprintf(fo, "L %04x %02x ", p->adr, p->data & 0xff);
					break;
				case acsLoad16:
					fprintf(fo, "L %04x %04x ", p->adr, p->data);
					break;
				case acsStore8:
					fprintf(fo, "S %04x %02x ", p->adr, p->data & 0xff);
					break;
				case acsStore16:
					fprintf(fo, "S %04x %04x ", p->adr, p->data);
					break;
			}
		}
		fprintf(fo, "\n");
	} while (tracep != endp);
	fclose(fo);
	fprintf(stderr, "trace dumped.\n");
	exit(1);
}
#endif	// HD6309_TRACE
