#include "PSG.h"
#include <stdio.h>
#include <string.h>

void PSG::Mute() {
	memset(op, 0, sizeof(op));
}

void PSG::Set(uint8_t adr, uint8_t data) {
	Op *p = &op[adr >> 3];
	switch (adr & 7) {
		case 0:
			p->delta_tmp = data;
			break;
		case 1:
			p->delta = data << 8 | p->delta_tmp;
			break;
		case 2:
			p->vtmp = data;
			break;
		case 3:
			p->vl = p->vtmp;
			p->vr = data;
			if (!p->vl && !p->vr) p->count = 0;
			break;
		case 5:
			p->type = data & 7;
			break;
	}
	
}

void PSG::Update(int16_t &l, int16_t &r) {
	int al = 0, ar = 0;
	for (int i = 0; i < N; i++) {
		Op *p = &op[i];
		if (!p->vl && !p->vr) continue;
		int16_t t = 0;
		if ((p->delta & 0xff00) != 0xff00) {
			p->count += p->delta;
			switch (p->type) {
				default:
					t = p->count & 0x8000 ? -0x2000 : 0x2000;
					break;
				case 1:
					t = p->count & 0xc000 ? -0x2000 : 0x2000;
					break;
				case 2:
					t = p->count & 0xe000 ? -0x2000 : 0x2000;
					break;
				case 3:
					t = (p->count & 0xc000) == 0xc000 ? -0x2000 : 0x2000;
					break;
				case 4:
					t = p->count << 1 & 0x7fff;
					if (p->count & 0x4000) t = 0x7fff - t;
					if (p->count & 0x8000) t = -t;
					break;
				case 5:
					t = (0x7fff - p->count) >> 1;
					break;
				case 6:
					t = p->count << 1 & 0x7fff;
					if (p->count & 0x4000) t = 0x7fff - t;
					if (!(p->count & 0x8000)) t = -t;
					break;
				case 7:
					t = (p->count - 0x7fff) >> 1;
					break;
			}
		}
		else {
			rng = rng >> 1 | ((rng >> 2 ^ rng >> 3) & 1) << 16;
			t = rng & 1 ? -0x2000 : 0x2000;
		}
		al += t * p->vl;
		ar += t * p->vr;
	}
	al >>= ATT;
	ar >>= ATT;
	if (al < -0x8000) { al = -0x8000; clips++; }
	else if (al > 0x7fff) { al = 0x7fff; clips++; }
	if (ar < -0x8000) { ar = -0x8000; clips++; }
	else if (ar > 0x7fff) { ar = 0x7fff; clips++; }
	l = al;
	r = ar;
	if (++samples >= 44100) {
		if (clips) printf("PSG: %d samples clipped.\n", clips);
		samples = clips = 0;
	}
}
