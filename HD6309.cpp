// HD6309
// Copyright 2022-2025 © Yasuo Kuwahara
// MIT License

#include "HD6309.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define fclr()			fset<N0 | Z1 | V0 | C0>()
#define fz16(a)			fset<ZDEF, 1>(a)
#define faddh(a, d, s)	fset<HADD | NDEF | ZDEF | VADD | CADD>(a, d, s)
#define fmul(a)			fset<ZDEF | CMUL, 1>(a)
#define fdivro()		fset<N0 | Z0 | V1 | C0>()
#define fdaa(a, d)		fset<NDEF | ZDEF | CADD>(a, d)

#define CY				(cc & MC)
#define CLOCK(x)		(clock += (x))

#define A				r[1]
#define B				r[0]
#define D				((u16 &)r[0])
#define X				((u16 &)r[2])
#define Y				((u16 &)r[4])
#define U				((u16 &)r[6])
#define S				((u16 &)r[8])
#define PC				((u16 &)r[10])
#define E				r[13]
#define F				r[12]
#define W				((u16 &)r[12])
#define V				((u16 &)r[14])

#define da()			(dp | imm8())

static constexpr uint8_t eclk[] = {
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

static constexpr uint8_t nclk[] = {
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

static constexpr uint8_t eclk10[] = {
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

static constexpr uint8_t nclk10[] = {
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

static constexpr uint8_t eclk11[] = {
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

static constexpr uint8_t nclk11[] = {
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

HD6309::HD6309() : clktbl(eclk), clktbl10(eclk10), clktbl11(eclk11), clktblea(eclkea) {
#if HD6309_TRACE
	memset(tracebuf, 0, sizeof(tracebuf));
	tracep = tracebuf;
#endif
}

void HD6309::Reset() {
	dp = md = irq = waitflags = 0;
	memset(r, 0, sizeof(r));
	cc = MF | MI;
	PC = ld16(0xfffe);
}

HD6309::u16 HD6309::ea() { // indexed addressing
	s8 o = imm8();
	CLOCK(clktblea[o]);
	auto ir = [&]()->u16 & { return (u16 &)r[(o >> 4 & 6) + 2]; };
	if (!(o & 0x80))
		return ir() + ((s16)(o << 11) >> 11); // 5bit offset
	u16 adr;
	switch (o & 0xf) {
		case 0x1: adr = ir(); ir() += 2; break;
		case 0x2: adr = --ir(); break;
		case 0x3: adr = ir() -= 2; break;
		case 0x4: adr = ir(); break;
		case 0x5: adr = ir() + (s8)B; break;
		case 0x6: adr = ir() + (s8)A; break;
		case 0x7: adr = ir() + (s8)E; break;
		case 0x8: adr = ir() + (s8)imm8(); break;
		case 0x9: adr = ir() + imm16(); break;
		case 0xa: adr = ir() + (s8)F; break;
		case 0xb: adr = ir() + D; break;
		case 0xc: adr = (s8)imm8(); adr += PC; break;
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
	u8 pb = imm8(), a = pb & 7, d = 0;
	switch (pb & 0xc0) {
		case 0: d = cc; break;
		case 0x40: d = A; break;
		case 0x80: d = B; break;
	}
	d = (d & ~(1 << a)) | (op(d >> a, ld8(da()) >> (pb >> 3 & 7)) & 1) << a;
	switch (pb & 0xc0) {
		case 0: cc = d; break;
		case 0x40: A = d; break;
		case 0x80: B = d; break;
	}
}

template<typename T1, typename T2> void HD6309::interRegister(T1 f8, T2 f16) {
	auto rsels = [&](u8 sel)->u16 {
		if (!(sel & 0x80)) return (u16 &)r[sel >> 4 << 1];
		switch (sel & 0xf0) {
			case 0x80: return sel & 8 ? A : D;
			case 0x90: return sel & 8 ? B : D;
			case 0xa0: return cc;
			case 0xb0: return sel & 8 ? dp >> 8 : dp;
			case 0xe0: return sel & 8 ? E : W;
			case 0xf0: return sel & 8 ? F : W;
		}
		return 0;
	};
	auto rseld = [&](u8 sel)->u16 {
		sel &= 0xf;
		if (!(sel & 8)) return (u16 &)r[sel << 1];
		switch (sel) {
			case 8: return A;
			case 9: return B;
			case 10: return cc;
			case 11: return dp >> 8;
			case 14: return E;
			case 15: return F;
		}
		return 0;
	};
	auto wseli = [&](u8 sel, u16 data) {
		sel &= 0xf;
		if (!(sel & 8)) {
			(u16 &)r[sel << 1] = data;
			return;
		}
		switch (sel) {
			case 8: A = data; break;
			case 9: B = data; break;
			case 10: cc = data; break;
			case 11: dp = data << 8; break;
			case 14: E = data; break;
			case 15: F = data; break;
		}
	};
	u8 sel = imm8();
	if (sel & 8) {
		u8 d = rseld(sel);
		f8(d, rsels(sel));
		wseli(sel, d);
	}
	else {
		u16 d = rseld(sel);
		f16(d, rsels(sel));
		wseli(sel, d);
	}
}

void HD6309::transfer(u8 op, u8 t8) {
	auto rsel = [&](u8 sel)->u16 {
		u8 t;
		switch (sel &= 0xf) {
			case 8: return A << 8 | A;
			case 9: return B << 8 | B;
			case 10: t = cc; return t << 8 | t;
			case 11: return dp | dp >> 8;
			case 12: case 13: return 0;
			case 14: return E << 8 | E;
			case 15: return F << 8 | F;
		}
		return (u16 &)r[sel << 1];
	};
	auto wsel = [&](u8 sel, u16 data) {
		switch (sel &= 0xf) {
			case 8: A = data >> 8; return;
			case 9: B = data; return;
			case 10: cc = data; return;
			case 11: dp = data & 0xff00; return;
			case 12: case 13: return;
			case 14: E = data >> 8; return;
			case 15: F = data; return;
		}
		(u16 &)r[sel << 1] = data;
	};
	if (op & 1) wsel(t8, rsel(t8 >> 4)); // tfr
	else { // exg
		u16 t = rsel(t8);
		wsel(t8, rsel(t8 >> 4));
		wsel(t8 >> 4, t);
	}
}

void HD6309::trap(u16 vec = 0xfff0, u8 p = 0xff, u8 m = MF | MI) {
	if (p) {
		if (p == 0xff) cc |= ME;
		else cc &= ~ME;
		psh(true, p, md & 1 && p == 0xff);
	}
	cc |= m;
	PC = ld16(vec);
}

void HD6309::divd(s8 s) {
	if (!s) {
		md |= 0x80;
		trap();
		return;
	}
	s16 t = (s16)D / s, u = t >> 8;
	if (u && u != -1) {
		fdivro();
		CLOCK(-13);
		return;
	}
	int v = t != (s8)t;
	A = (s16)D % s;
	B = t;
	fdiv(B, v);
	CLOCK(-v);
}

void HD6309::divq(s16 s) {
	if (!s) {
		md |= 0x80;
		trap();
		return;
	}
	s32 d = D << 16 | W, t = d / s, u = t >> 16;
	if (u && u != -1) {
		fdivro();
		CLOCK(-21);
		return;
	}
	int v = t != (s16)t;
	D = d % s;
	W = t;
	fdiv<1>(W, v);
	CLOCK(-v);
}

void HD6309::tfm(int s, int d) {
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
	u16 &sr = (u16 &)r[si << 1], &dr = (u16 &)r[di << 1];
	st8(dr, ld8(sr));
	sr += s;
	dr += d;
	PC -= 3;
}

void HD6309::psh(bool s, u8 t, bool wp) {
	u16 &p = s ? S : U;
	if (t & 0x80) st16r(p -= 2, PC);
	if (t & 0x40) st16r(p -= 2, s ? U : S);
	if (t & 0x20) st16r(p -= 2, Y);
	if (t & 0x10) st16r(p -= 2, X);
	if (t & 0x08) st8(--p, dp >> 8);
	if (wp) { st16r(p -= 2, W); CLOCK(2); }
	if (t & 0x04) st8(--p, B);
	if (t & 0x02) st8(--p, A);
	if (t & 0x01) st8(--p, cc);
	CLOCK(__builtin_popcount(t) + __builtin_popcount(t & 0xf0));
}

void HD6309::pul(bool s, u8 t, bool wp) {
	u16 &p = s ? S : U;
	if (t & 0x01) cc = ld8(p++);
	if (t & 0x02) A = ld8(p++);
	if (t & 0x04) B = ld8(p++);
	if (wp) { W = ld16(p); p += 2; CLOCK(2); }
	if (t & 0x08) dp = ld8(p++) << 8;
	if (t & 0x10) { X = ld16(p); p += 2; }
	if (t & 0x20) { Y = ld16(p); p += 2; }
	if (t & 0x40) { (s ? U : S) = ld16(p); p += 2; }
	if (t & 0x80) { PC = ld16(p); p += 2; }
	CLOCK(__builtin_popcount(t) + __builtin_popcount(t & 0xf0));
}

void HD6309::ProcessInterrupt() {
	irq &= ~M_SYNC;
	if (waitflags & W_SYNC) {
		waitflags &= ~W_SYNC;
		PC++;
	}
	if (irq & M_NMI || (irq & M_FIRQ && !(cc & MF)) || (irq & M_IRQ && !(cc & MI))) {
		u8 p = waitflags & W_CWAI ? 0 : 0xff;
		if (!p) {
			waitflags &= ~W_CWAI;
			PC += 2;
		}
		if (irq & M_NMI) {
			irq &= ~M_NMI;
			trap(0xfffc, p);
			CLOCK(7);
		}
		else if (irq & M_FIRQ && !(cc & MF)) {
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

int HD6309::Execute(int n) {
	// instructions
	auto add8 = [&](u8 &d, u8 s) { d = faddh(d + s, d, s); };
	auto add8r = [&](u8 &d, u8 s) { d = fadd(d + s, d, s); };
	auto add16 = [&](u16 &d, u16 s) { d = fadd<1>(d + s, d, s); };
	auto adc8 = [&](u8 &d, u8 s) { d = faddh(d + s + CY, d, s); };
	auto adc8r = [&](u8 &d, u8 s) { d = fadd(d + s + CY, d, s); };
	auto adc16 = [&](u16 &d, u16 s) { d = fadd<1>(d + s + CY, d, s); };
	auto sub8 = [&](u8 &d, u8 s) { d = fsub(d - s, d, s); };
	auto sub16 = [&](u16 &d, u16 s) { d = fsub<1>(d - s, d, s); };
	auto sbc8 = [&](u8 &d, u8 s) { d = fsub(d - s - CY, d, s); };
	auto sbc16 = [&](u16 &d, u16 s) { d = fsub<1>(d - s - CY, d, s); };
	auto cmp8 = [&](u8 d, u8 s) { fsub(d - s, d, s); };
	auto cmp16 = [&](u16 d, u16 s) { fsub<1>(d - s, d, s); };
	auto muld = [&](s16 s) { s32 t = (s16)D * s; fnz<1>(D = t >> 16); W = t; }; // undocumented
	auto incm = [&](u16 a) { u8 t = ld8(a); st8(a, finc(t + 1, t)); };
	auto decm = [&](u16 a) { u8 t = ld8(a); st8(a, fdec(t - 1, t)); };
	auto negm = [&](u16 a) { u8 t = -ld8(a); fneg(t); st8(a, t); };
	auto clrm = [&](u16 a) { st8(a, 0); fclr(); };
	auto tstm = [&](u16 a) { fmov(ld8(a)); };
	auto and8 = [&](u8 &d, u8 s) { fmov(d &= s); };
	auto and16 = [&](u16 &d, u16 s) { fmov<1>(d &= s); };
	auto or8 = [&](u8 &d, u8 s) { fmov(d |= s); };
	auto or16 = [&](u16 &d, u16 s) { fmov<1>(d |= s); };
	auto eor8 = [&](u8 &d, u8 s) { fmov(d ^= s); };
	auto eor16 = [&](u16 &d, u16 s) { fmov<1>(d ^= s); };
	auto comm = [&](u16 a) { u8 t = ~ld8(a); fcom(t); st8(a, t); };
	auto lsl = [&](u8 &r) { r = fleft(r << 1, r); };
	auto lsr = [&](u8 &r) { r = fright(r >> 1, r); };
	auto asr = [&](u8 &r) { r = fright((s8)r >> 1, r); };
	auto rol = [&](u8 &r) { r = fleft(r << 1 | CY, r); };
	auto ror = [&](u8 &r) { r = fright(r >> 1 | CY << 7, r); };
	auto lslm = [&](u16 a) { u8 t = ld8(a); st8(a, fleft(t << 1, t)); };
	auto lsrm = [&](u16 a) { u8 t = ld8(a); st8(a, fright(t >> 1, t)); };
	auto asrm = [&](u16 a) { s8 t = ld8(a); st8(a, fright(t >> 1, t)); };
	auto rolm = [&](u16 a) { u8 t = ld8(a); st8(a, fleft(t << 1 | CY, t)); };
	auto rorm = [&](u16 a) { u8 t = ld8(a); st8(a, fright(t >> 1 | CY << 7, t)); };
	auto jsr = [&](u16 a) { st16r(S -= 2, PC); PC = a; };
	auto ldq = [&](u32 s) { fnz<1>(D = s >> 16); W = s; }; // undocumented
	auto stq = [&](u16 a) { fmov<2>(D, W); st16(a, D); st16(a + 2, W); };
	auto aim = [&](u8 m, u16 a) { fmov(m &= ld8(a)); st8(a, m); };
	auto oim = [&](u8 m, u16 a) { fmov(m |= ld8(a)); st8(a, m); };
	auto eim = [&](u8 m, u16 a) { fmov(m ^= ld8(a)); st8(a, m); };
	auto tim = [&](u8 m, u16 a) { fmov(ld8(a) & m); };
	// conditions
	auto _c = [&]{ return !(cc & MC); };
	auto lo = [&]{ return cc & MC; };
	auto vc = [&]{ return !(cc & MV); };
	auto vs = [&]{ return cc & MV; };
	auto ne = [&]{ return !(cc & MZ); };
	auto eq = [&]{ return cc & MZ; };
	auto pl = [&]{ return !(cc & MN); };
	auto mi = [&]{ return cc & MN; };
	auto hi = [&]{ return ~(cc >> (LZ - LC) | cc) & MC; };
	auto ls = [&]{ return (cc >> (LZ - LC) | cc) & MC; };
	auto ge = [&]{ return ~(cc ^ cc << (LN - LV)) & MN; };
	auto lt = [&]{ return (cc ^ cc << (LN - LV)) & MN; };
	auto gt = [&]{ return ~((cc ^ cc << (LN - LV)) | cc << (LN - LZ)) & MN; };
	auto le = [&]{ return ((cc ^ cc << (LN - LV)) | cc << (LN - LZ)) & MN; };
	//
	clock = 0;
	do {
		if (irq) ProcessInterrupt();
#if HD6309_TRACE
		tracep->r[5] = PC;
		tracep->index = tracep->opn = 0;
#endif
		s32 t32;
		s8 t8, t;
		s16 t16;
		u8 op = imm8();
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
			case 0x13: waitflags |= W_SYNC; PC--; return 0;
			case 0x14: D = (s16)W >> 15; fnz<1>(W); break;
			case 0x16: t16 = imm16(); PC += t16; break;
			case 0x17: t16 = imm16(); st16r(S -= 2, PC); PC += t16; break;
			case 0x19: // daa
				t8 = A;
				if (CY || (A & 0xf0) > 0x90 || ((A & 0xf0) > 0x80 && (A & 0xf) > 9)) t8 += 0x60;
				if (cc & MH || (A & 0xf) > 9) t8 += 6;
				A = fdaa(t8, A);
				break;
			case 0x1a: cc |= imm8(); break;
			case 0x1c: cc &= imm8(); break;
			case 0x1d: fnz(D = (s8)B); break;
			case 0x1e: case 0x1f: transfer(op, imm8()); break;
			case 0x20: bcc([]{ return true; }); break; // bra
			case 0x21: bcc([]{ return false; }); break; // brn
			case 0x22: bcc(hi); break;
			case 0x23: bcc(ls); break;
			case 0x24: bcc(_c); break;
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
				cc = ld8(S++);
				t8 = cc & ME;
				pul(true, t8 ? 0xfe : 0x80, md & 1 && t8); break;
			case 0x3c:
				if (waitflags & W_CWAI) PC--;
				else {
					waitflags |= W_CWAI;
					cc = (cc & imm8()) | ME;
					psh(true, 0xff, md & 1);
					PC -= 2;
				}
				return 0;
			case 0x3d: fmul(D = A * B); break;
			case 0x3f: trap(0xfffa); break; // swi
			case 0x40: fneg(A = -A); break;
			case 0x43: fcom(A = ~A); break;
			case 0x44: lsr(A); break;
			case 0x46: ror(A); break;
			case 0x47: asr(A); break;
			case 0x48: lsl(A); break;
			case 0x49: rol(A); break;
			case 0x4a: A = fdec(A - 1, A); break;
			case 0x4c: A = finc(A + 1, A); break;
			case 0x4d: fmov(A); break;
			case 0x4f: A = 0; fclr(); break;
			case 0x50: fneg(B = -B); break;
			case 0x53: fcom(B = ~B); break;
			case 0x54: lsr(B); break;
			case 0x56: ror(B); break;
			case 0x57: asr(B); break;
			case 0x58: lsl(B); break;
			case 0x59: rol(B); break;
			case 0x5a: B = fdec(B - 1, B); break;
			case 0x5c: B = finc(B + 1, B); break;
			case 0x5d: fmov(B); break;
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
			case 0x85: fmov(A & imm8()); break;
			case 0x86: fmov(A = imm8()); break;
			case 0x88: eor8(A, imm8()); break;
			case 0x89: adc8(A, imm8()); break;
			case 0x8a: or8(A, imm8()); break;
			case 0x8b: add8(A, imm8()); break;
			case 0x8c: cmp16(X, imm16()); break;
			case 0x8d: t8 = imm8(); st16r(S -= 2, PC); PC += t8; break;
			case 0x8e: fmov<1>(X = imm16()); break;
			case 0x90: sub8(A, ld8(da())); break;
			case 0x91: cmp8(A, ld8(da())); break;
			case 0x92: sbc8(A, ld8(da())); break;
			case 0x93: sub16(D, ld16(da())); break;
			case 0x94: and8(A, ld8(da())); break;
			case 0x95: fmov(A & ld8(da())); break;
			case 0x96: fmov(A = ld8(da())); break;
			case 0x97: fmov(A); st8(da(), A); break;
			case 0x98: eor8(A, ld8(da())); break;
			case 0x99: adc8(A, ld8(da())); break;
			case 0x9a: or8(A, ld8(da())); break;
			case 0x9b: add8(A, ld8(da())); break;
			case 0x9c: cmp16(X, ld16(da())); break;
			case 0x9d: jsr(da()); break;
			case 0x9e: fmov<1>(X = ld16(da())); break;
			case 0x9f: fmov<1>(X); st16(da(), X); break;
			case 0xa0: sub8(A, ld8(ea())); break;
			case 0xa1: cmp8(A, ld8(ea())); break;
			case 0xa2: sbc8(A, ld8(ea())); break;
			case 0xa3: sub16(D, ld16(ea())); break;
			case 0xa4: and8(A, ld8(ea())); break;
			case 0xa5: fmov(A & ld8(ea())); break;
			case 0xa6: fmov(A = ld8(ea())); break;
			case 0xa7: fmov(A); st8(ea(), A); break;
			case 0xa8: eor8(A, ld8(ea())); break;
			case 0xa9: adc8(A, ld8(ea())); break;
			case 0xaa: or8(A, ld8(ea())); break;
			case 0xab: add8(A, ld8(ea())); break;
			case 0xac: cmp16(X, ld16(ea())); break;
			case 0xad: jsr(ea()); break;
			case 0xae: fmov<1>(X = ld16(ea())); break;
			case 0xaf: fmov<1>(X); st16(ea(), X); break;
			case 0xb0: sub8(A, ld8(imm16())); break;
			case 0xb1: cmp8(A, ld8(imm16())); break;
			case 0xb2: sbc8(A, ld8(imm16())); break;
			case 0xb3: sub16(D, ld16(imm16())); break;
			case 0xb4: and8(A, ld8(imm16())); break;
			case 0xb5: fmov(A & ld8(imm16())); break;
			case 0xb6: fmov(A = ld8(imm16())); break;
			case 0xb7: fmov(A); st8(imm16(), A); break;
			case 0xb8: eor8(A, ld8(imm16())); break;
			case 0xb9: adc8(A, ld8(imm16())); break;
			case 0xba: or8(A, ld8(imm16())); break;
			case 0xbb: add8(A, ld8(imm16())); break;
			case 0xbc: cmp16(X, ld16(imm16())); break;
			case 0xbd: jsr(imm16()); break;
			case 0xbe: fmov<1>(X = ld16(imm16())); break;
			case 0xbf: fmov<1>(X); st16(imm16(), X); break;
			case 0xc0: sub8(B, imm8()); break;
			case 0xc1: cmp8(B, imm8()); break;
			case 0xc2: sbc8(B, imm8()); break;
			case 0xc3: add16(D, imm16()); break;
			case 0xc4: and8(B, imm8()); break;
			case 0xc5: fmov(B & imm8()); break;
			case 0xc6: fmov(B = imm8()); break;
			case 0xc8: eor8(B, imm8()); break;
			case 0xc9: adc8(B, imm8()); break;
			case 0xca: or8(B, imm8()); break;
			case 0xcb: add8(B, imm8()); break;
			case 0xcc: fmov<1>(D = imm16()); break;
			case 0xcd: t32 = imm16() << 16; ldq(t32 | imm16()); break;
			case 0xce: fmov<1>(U = imm16()); break;
			case 0xd0: sub8(B, ld8(da())); break;
			case 0xd1: cmp8(B, ld8(da())); break;
			case 0xd2: sbc8(B, ld8(da())); break;
			case 0xd3: add16(D, ld16(da())); break;
			case 0xd4: and8(B, ld8(da())); break;
			case 0xd5: fmov(B & ld8(da())); break;
			case 0xd6: fmov(B = ld8(da())); break;
			case 0xd7: fmov(B); st8(da(), B); break;
			case 0xd8: eor8(B, ld8(da())); break;
			case 0xd9: adc8(B, ld8(da())); break;
			case 0xda: or8(B, ld8(da())); break;
			case 0xdb: add8(B, ld8(da())); break;
			case 0xdc: fmov<1>(D = ld16(da())); break;
			case 0xdd: fmov<1>(D); st16(da(), D); break;
			case 0xde: fmov<1>(U = ld16(da())); break;
			case 0xdf: fmov<1>(U); st16(da(), U); break;
			case 0xe0: sub8(B, ld8(ea())); break;
			case 0xe1: cmp8(B, ld8(ea())); break;
			case 0xe2: sbc8(B, ld8(ea())); break;
			case 0xe3: add16(D, ld16(ea())); break;
			case 0xe4: and8(B, ld8(ea())); break;
			case 0xe5: fmov(B & ld8(ea())); break;
			case 0xe6: fmov(B = ld8(ea())); break;
			case 0xe7: fmov(B); st8(ea(), B); break;
			case 0xe8: eor8(B, ld8(ea())); break;
			case 0xe9: adc8(B, ld8(ea())); break;
			case 0xea: or8(B, ld8(ea())); break;
			case 0xeb: add8(B, ld8(ea())); break;
			case 0xec: fmov<1>(D = ld16(ea())); break;
			case 0xed: fmov<1>(D); st16(ea(), D); break;
			case 0xee: fmov<1>(U = ld16(ea())); break;
			case 0xef: fmov<1>(U); st16(ea(), U); break;
			case 0xf0: sub8(B, ld8(imm16())); break;
			case 0xf1: cmp8(B, ld8(imm16())); break;
			case 0xf2: sbc8(B, ld8(imm16())); break;
			case 0xf3: add16(D, ld16(imm16())); break;
			case 0xf4: and8(B, ld8(imm16())); break;
			case 0xf5: fmov(B & ld8(imm16())); break;
			case 0xf6: fmov(B = ld8(imm16())); break;
			case 0xf7: fmov(B); st8(imm16(), B); break;
			case 0xf8: eor8(B, ld8(imm16())); break;
			case 0xf9: adc8(B, ld8(imm16())); break;
			case 0xfa: or8(B, ld8(imm16())); break;
			case 0xfb: add8(B, ld8(imm16())); break;
			case 0xfc: fmov<1>(D = ld16(imm16())); break;
			case 0xfd: fmov<1>(D); st16(imm16(), D); break;
			case 0xfe: fmov<1>(U = ld16(imm16())); break;
			case 0xff: fmov<1>(U); st16(imm16(), U); break;
			case 0x10:
				op = imm8();
				switch (op) {
					case 0x21: lbcc([]{ return false; }); break; // lbrn
					case 0x22: lbcc(hi); break;
					case 0x23: lbcc(ls); break;
					case 0x24: lbcc(_c); break;
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
					case 0x30: interRegister(add8r, add16); break;
					case 0x31: interRegister(adc8r, adc16); break;
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
					case 0x40: fneg<1>(D = -D); break;
					case 0x43: fcom<1>(D = ~D); break;
					case 0x44: D = fright<1>(D >> 1, D); break;
					case 0x46: D = fright<1>(D >> 1 | CY << 15, D); break;
					case 0x47: D = fright<1>((s16)D >> 1, D); break;
					case 0x48: D = fleft<1>(D << 1, D); break;
					case 0x49: D = fleft<1>(D << 1 | CY, D); break;
					case 0x4a: D = fdec<1>(D - 1, D); break;
					case 0x4c: D = finc<1>(D + 1, D); break;
					case 0x4d: fmov<1>(D); break;
					case 0x4f: D = 0; fclr(); break;
					case 0x53: fcom<1>(W = ~W); break;
					case 0x54: W = fright<1>(W >> 1, W); break;
					case 0x56: W = fright<1>(W >> 1 | CY << 15, W); break;
					case 0x59: W = fleft<1>(W << 1 | CY, W); break;
					case 0x5a: W = fdec<1>(W - 1, W); break;
					case 0x5c: W = finc<1>(W + 1, W); break;
					case 0x5d: fmov<1>(W); break;
					case 0x5f: W = 0; fclr(); break;
					case 0x80: sub16(W, imm16()); break;
					case 0x81: cmp16(W, imm16()); break;
					case 0x82: sbc16(D, imm16()); break;
					case 0x83: cmp16(D, imm16()); break;
					case 0x84: fmov<1>(D &= imm16()); break;
					case 0x85: fmov<1>(D & imm16()); break;
					case 0x86: fmov<1>(W = imm16()); break;
					case 0x88: eor16(D, imm16()); break;
					case 0x89: adc16(D, imm16()); break;
					case 0x8a: or16(D, imm16()); break;
					case 0x8b: add16(W, imm16()); break;
					case 0x8c: cmp16(Y, imm16()); break;
					case 0x8e: fmov<1>(Y = imm16()); break;
					case 0x90: sub16(W, ld16(da())); break;
					case 0x91: cmp16(W, ld16(da())); break;
					case 0x92: sbc16(D, ld16(da())); break;
					case 0x93: cmp16(D, ld16(da())); break;
					case 0x94: fmov<1>(D &= ld16(da())); break;
					case 0x95: fmov<1>(D & ld16(da())); break;
					case 0x96: fmov<1>(W = ld16(da())); break;
					case 0x97: fmov<1>(W); st16(da(), W); break;
					case 0x98: eor16(D, ld16(da())); break;
					case 0x99: adc16(D, ld16(da())); break;
					case 0x9a: or16(D, ld16(da())); break;
					case 0x9b: add16(W, ld16(da())); break;
					case 0x9c: cmp16(Y, ld16(da())); break;
					case 0x9e: fmov<1>(Y = ld16(da())); break;
					case 0x9f: fmov<1>(Y); st16(da(), Y); break;
					case 0xa0: sub16(W, ld16(ea())); break;
					case 0xa1: cmp16(W, ld16(ea())); break;
					case 0xa2: sbc16(D, ld16(ea())); break;
					case 0xa3: cmp16(D, ld16(ea())); break;
					case 0xa4: fmov<1>(D &= ld16(ea())); break;
					case 0xa5: fmov<1>(D & ld16(ea())); break;
					case 0xa6: fmov<1>(W = ld16(ea())); break;
					case 0xa7: fmov<1>(W); st16(ea(), W); break;
					case 0xa8: eor16(D, ld16(ea())); break;
					case 0xa9: adc16(D, ld16(ea())); break;
					case 0xaa: or16(D, ld16(ea())); break;
					case 0xab: add16(W, ld16(ea())); break;
					case 0xac: cmp16(Y, ld16(ea())); break;
					case 0xae: fmov<1>(Y = ld16(ea())); break;
					case 0xaf: fmov<1>(Y); st16(ea(), Y); break;
					case 0xb0: sub16(W, ld16(imm16())); break;
					case 0xb1: cmp16(W, ld16(imm16())); break;
					case 0xb2: sbc16(D, ld16(imm16())); break;
					case 0xb3: cmp16(D, ld16(imm16())); break;
					case 0xb4: fmov<1>(D &= ld16(imm16())); break;
					case 0xb5: fmov<1>(D & ld16(imm16())); break;
					case 0xb6: fmov<1>(W = ld16(imm16())); break;
					case 0xb7: fmov<1>(W); st16(imm16(), W); break;
					case 0xb8: eor16(D, ld16(imm16())); break;
					case 0xb9: adc16(D, ld16(imm16())); break;
					case 0xba: or16(D, ld16(imm16())); break;
					case 0xbb: add16(W, ld16(imm16())); break;
					case 0xbc: cmp16(Y, ld16(imm16())); break;
					case 0xbe: fmov<1>(Y = ld16(imm16())); break;
					case 0xbf: fmov<1>(Y); st16(imm16(), Y); break;
					case 0xce: fmov<1>(S = imm16()); break;
					case 0xdc: t16 = da(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xdd: stq(da()); break;
					case 0xde: fmov<1>(S = ld16(da())); break;
					case 0xdf: fmov<1>(S); st16(da(), S); break;
					case 0xec: t16 = ea(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xed: stq(ea()); break;
					case 0xee: fmov<1>(S = ld16(ea())); break;
					case 0xef: fmov<1>(S); st16(ea(), S); break;
					case 0xfc: t16 = imm16(); t32 = ld16(t16) << 16; ldq(t32 | ld16(t16 + 2)); break;
					case 0xfd: stq(imm16()); break;
					case 0xfe: fmov<1>(S = ld16(imm16())); break;
					case 0xff: fmov<1>(S); st16(imm16(), S); break;
					default: md |= 0x40; trap(); break;
				}
				CLOCK(clktbl10[op]);
				goto skip_clock;
			case 0x11:
				op = imm8();
				switch (op) {
					case 0x30: bitTransfer([](u8 d, u8 s) { return d & s; }); break;
					case 0x31: bitTransfer([](u8 d, u8 s) { return d & ~s; }); break;
					case 0x32: bitTransfer([](u8 d, u8 s) { return d | s; }); break;
					case 0x33: bitTransfer([](u8 d, u8 s) { return d | ~s; }); break;
					case 0x34: bitTransfer([](u8 d, u8 s) { return d ^ s; }); break;
					case 0x35: bitTransfer([](u8 d, u8 s) { return d ^ ~s; }); break;
					case 0x36: bitTransfer([](u8 d, u8 s) { return s; }); break;
					case 0x37: // stbt
						t8 = imm8();
						t16 = da();
						switch (t8 & 0xc0) {
							case 0: t = cc; break;
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
						if (md & 1) { clktbl = nclk; clktbl10 = nclk10; clktbl11 = nclk11; clktblea = nclkea; }
						else { clktbl = eclk; clktbl10 = eclk10; clktbl11 = eclk11; clktblea = eclkea; }
						break;
					case 0x3f: trap(0xfff2, 0xff, 0); break; // swi3
					case 0x43: fcom(E = ~E); break;
					case 0x4a: E = fdec(E - 1, E); break;
					case 0x4c: E = finc(E + 1, E); break;
					case 0x4d: fmov(E); break;
					case 0x4f: E = 0; fclr(); break;
					case 0x53: fcom(F = ~F); break;
					case 0x5a: F = fdec(F - 1, F); break;
					case 0x5c: F = finc(F + 1, F); break;
					case 0x5d: fmov(F); break;
					case 0x5f: F = 0; fclr(); break;
					case 0x80: sub8(E, imm8()); break;
					case 0x81: cmp8(E, imm8()); break;
					case 0x83: cmp16(U, imm16()); break;
					case 0x86: fmov(E = imm8()); break;
					case 0x8b: add8(E, imm8()); break;
					case 0x8c: cmp16(S, imm16()); break;
					case 0x8d: divd(imm8()); break;
					case 0x8e: divq(imm16()); break;
					case 0x8f: muld(imm16()); break;
					case 0x90: sub8(E, ld8(da())); break;
					case 0x91: cmp8(E, ld8(da())); break;
					case 0x93: cmp16(U, ld16(da())); break;
					case 0x96: fmov(E = ld8(da())); break;
					case 0x97: fmov(E); st8(da(), E); break;
					case 0x9b: add8(E, ld8(da())); break;
					case 0x9c: cmp16(S, ld16(da())); break;
					case 0x9d: divd(ld8(da())); break;
					case 0x9e: divq(ld16(da())); break;
					case 0x9f: muld(ld16(da())); break;
					case 0xa0: sub8(E, ld8(ea())); break;
					case 0xa1: cmp8(E, ld8(ea())); break;
					case 0xa3: cmp16(U, ld16(ea())); break;
					case 0xa6: fmov(E = ld8(ea())); break;
					case 0xa7: fmov(E); st8(ea(), E); break;
					case 0xab: add8(E, ld8(ea())); break;
					case 0xac: cmp16(S, ld16(ea())); break;
					case 0xad: divd(ld8(ea())); break;
					case 0xae: divq(ld16(ea())); break;
					case 0xaf: muld(ld16(ea())); break;
					case 0xb0: sub8(E, ld8(imm16())); break;
					case 0xb1: cmp8(E, ld8(imm16())); break;
					case 0xb3: cmp16(U, ld16(imm16())); break;
					case 0xb6: fmov(E = ld8(imm16())); break;
					case 0xb7: fmov(E); st8(imm16(), E); break;
					case 0xbb: add8(E, ld8(imm16())); break;
					case 0xbc: cmp16(S, ld16(imm16())); break;
					case 0xbd: divd(ld8(imm16())); break;
					case 0xbe: divq(ld16(imm16())); break;
					case 0xbf: muld(ld16(imm16())); break;
					case 0xc0: sub8(F, imm8()); break;
					case 0xc1: cmp8(F, imm8()); break;
					case 0xc6: fmov(F = imm8()); break;
					case 0xcb: add8(F, imm8()); break;
					case 0xd0: sub8(F, ld8(da())); break;
					case 0xd1: cmp8(F, ld8(da())); break;
					case 0xd6: fmov(F = ld8(da())); break;
					case 0xd7: fmov(F); st8(da(), F); break;
					case 0xdb: add8(F, ld8(da())); break;
					case 0xe0: sub8(F, ld8(ea())); break;
					case 0xe1: cmp8(F, ld8(ea())); break;
					case 0xe6: fmov(F = ld8(ea())); break;
					case 0xe7: fmov(F); st8(ea(), F); break;
					case 0xeb: add8(F, ld8(ea())); break;
					case 0xf0: sub8(F, ld8(imm16())); break;
					case 0xf1: cmp8(F, ld8(imm16())); break;
					case 0xf6: fmov(F = ld8(imm16())); break;
					case 0xf7: fmov(F); st8(imm16(), F); break;
					case 0xfb: add8(F, ld8(imm16())); break;
					default: md |= 0x40; trap(); break;
				}
				CLOCK(clktbl11[op]);
				goto skip_clock;
			default: md |= 0x40; trap(); break;
		}
		CLOCK(clktbl[op]);
skip_clock:;
#if HD6309_TRACE
		tracep->cc = cc;
		for (int i = 0; i < 8; i++)
			if (i != 5) tracep->r[i] = (u16 &)r[i << 1];
#if HD6309_TRACE > 1
		if (++tracep >= tracebuf + TRACEMAX - 1) StopTrace();
#else
		if (++tracep >= tracebuf + TRACEMAX) tracep = tracebuf;
#endif
#endif
	} while (clock < n);
	return clock - n;
}

#define MSB_N	((8 << SI) - 1)
#define MSB		(1 << MSB_N)
#define MASK	((1 << (8 << SI)) - 1)

template<int M, int SI> HD6309::u16 HD6309::fset(u16 a, u16 d, u16 s) {
	if constexpr ((M & 0xf) == C0) cc &= ~MC;
	if constexpr ((M & 0xf) == C1) cc |= MC;
	if constexpr ((M & 0xf) == CD) cc = d & 1 ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CDEF) cc = a & MASK ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CADD) cc = ((s & d) | (~a & d) | (s & ~a)) & MSB ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CSUB) cc = ((s & ~d) | (a & ~d) | (s & a)) & MSB ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CMUL) cc = a & 0x80 ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CLEFT) cc = d & MSB ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf) == CDIV) cc = a & 1 ? cc | MC : cc & ~MC;
	if constexpr ((M & 0xf0) == V0) cc &= ~MV;
	if constexpr ((M & 0xf0) == V1) cc |= MV;
	if constexpr ((M & 0xf0) == VD) cc = d ? cc | MV : cc & ~MV;
	if constexpr ((M & 0xf0) == VDEF) cc = (a & MASK) == MSB ? cc | MV : cc & ~MV;
	if constexpr ((M & 0xf0) == VADD) cc = ((d & s & ~a) | (~d & ~s & a)) & MSB ? cc | MV : cc & ~MV;
	if constexpr ((M & 0xf0) == VSUB) cc = ((d & ~s & ~a) | (~d & s & a)) & MSB ? cc | MV : cc & ~MV;
	if constexpr ((M & 0xf0) == VLEFT) cc = (d >> (MSB_N - 1) ^ d >> MSB_N) & 1 ? cc | MV : cc & ~MV;
	if constexpr ((M & 0xf00) == Z0) cc &= ~MZ;
	if constexpr ((M & 0xf00) == Z1) cc |= MZ;
	if constexpr ((M & 0xf00) == ZDEF) cc = (SI == 2 ? a | d : a & MASK) ? cc & ~MZ : cc | MZ;
	if constexpr ((M & 0xf000) == N0) cc &= ~MN;
	if constexpr ((M & 0xf000) == NDEF) cc = a & MSB ? cc | MN : cc & ~MN;
	if constexpr ((M & 0xf00000) == HADD) cc = (d ^ s ^ a) & 0x10 ? cc | MH : cc & ~MH;
	return a;
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
