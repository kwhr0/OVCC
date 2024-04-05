#include "snd.h"

#define SG_MAX			32

#define BEND_SHIFT		7
#define DR_SHIFT		4
#define SR_SHIFT		8
#define AR_MASK			7
#define DR_MASK			0xf
#define DR_MASK2		0x1e
#define SR_MASK			7
#define SL_MASK			0x7000
#define SL_OFS			0xfff
#define PERC_MASK		8
#define RELEASE_MASK	0x800
#define RELEASE_MASK_H	8
#define NZ_MASK			0x8000

#define IOSound(adr, data)	(PSG_ADR = (adr), PSG_DATA = (data))

enum {
	ATTACK, DECAY, SUSTAIN, RELEASE, DISPOSE
};

typedef struct SG {
	struct SG *next; // must be first member
	u16 id;
	s16 env;
	u8 prog, velo, volex, ref, note, pan, adr, state;
} SG;

static SG sSG[SG_MAX];
static SG *sActiveCh, *sReleaseCh, *sFreeCh;
static u16 sMute;
static KeyCallback sKeyCallback;

static const u16 sToneData[] = {
	0x2f72, 0x2f72, 0x3f72, 0x3f72, 0x2f80, 0x2f80, 0x3e80, 0x2f80,
	0x2e70, 0x3740, 0x2f80, 0x7520, 0x2e60, 0x1559, 0x3659, 0x2e61,
	0x7f01, 0x7f01, 0x7f01, 0x7f03, 0x7f03, 0x7f03, 0x7f03, 0x7f03,
	0x2e71, 0x2e71, 0x7e00, 0x7d00, 0x2e71, 0x3f90, 0x3f90, 0x2f81,
	0x7d21, 0x1f80, 0x1f80, 0x1f90, 0x1f70, 0x1f70, 0x1f71, 0x7f61,
	0x7f04, 0x7f04, 0x7f04, 0x7f04, 0x7f06, 0x1649, 0x2678, 0x3f71,
	0x7f06, 0x7f06, 0x7f06, 0x7f06, 0x7f04, 0x7f03, 0x7f03, 0x2e64,
	0x7f04, 0x7f04, 0x7f03, 0x7f03, 0x7f03, 0x7f22, 0x7f22, 0x4f93,
	0x6f53, 0x6f53, 0x6f53, 0x6f53, 0x7f03, 0x7f03, 0x7f03, 0x7f03,
	0x7f03, 0x7f03, 0x7f03, 0x7f76, 0x7f76, 0x7f75, 0x7f05, 0x7f03,
	0x7f01, 0x7f01, 0x7f74, 0x7f82, 0x2fb0, 0x7f01, 0x7f01, 0x7f00,
	0x7f01, 0x7f06, 0x7f00, 0x7f83, 0x7f86, 0x7f06, 0x7f01, 0x7f06,
	0x7f40, 0x5f96, 0x1678, 0x7f00, 0x7e01, 0x7f06, 0x7f01, 0x7f02,
	0x2f90, 0x7e00, 0x2d70, 0x7568, 0x1f80, 0x7f01, 0x7f04, 0x7f01,
	0x1f70, 0x1e50, 0x2568, 0x1638, 0x1568, 0x1658, 0x1558, 0xf206,
	0x7201, 0x7404, 0xfd06, 0x5e80, 0x7f00, 0xff06, 0xff06, 0xac50,
	0x7108, 0x2428, 0xa348, 0x2338, 0x7108, 0x7109, 0x6429, 0xb458,
	0x4459, 0x3338, 0xb348, 0x2338, 0x2328, 0x2328, 0x1328, 0x2328,
	0x2328, 0x2358, 0x2348, 0x1428, 0xb32b, 0xb21a, 0x7178, 0x7198,
	0x2328, 0x2328, 0x7108, 0x7608, 
};

static void SetStep(SG *p, s16 bend) {
	static const u16 dptable[] = {
		11, 12, 13, 13, 14, 15, 16, 17, 18, 19, 20, 21, 
		22, 24, 25, 27, 28, 30, 32, 33, 35, 38, 40, 42, 
		45, 47, 50, 53, 56, 60, 63, 67, 71, 75, 80, 84, 
		89, 95, 100, 106, 113, 119, 126, 134, 142, 150, 159, 169, 
		179, 189, 200, 212, 225, 238, 253, 268, 284, 300, 318, 337, 
		357, 378, 401, 425, 450, 477, 505, 535, 567, 601, 636, 674, 
		714, 757, 802, 850, 900, 954, 1010, 1070, 1134, 1201, 1273, 1349, 
		1429, 1514, 1604, 1699, 1800, 1907, 2021, 2141, 2268, 2403, 2546, 2697, 
		2858, 3028, 3208, 3398, 3600, 3815, 4041, 4282, 4536, 4806, 5092, 5395, 
		5715, 6055, 6415, 6797, 7201, 7629, 8083, 8563, 9072, 9612, 10184, 10789, 
		11431, 12110, 12830, 13593, 14402, 15258, 16165, 17127, 
	};
	if (sToneData[p->prog] & NZ_MASK) 
		IOSound(p->adr + 1, 0xff);
	else {
		u8 note = p->note;
		u16 hi, lo, dp;
		asm {
			ldd	:bend
			asld	a = bend >> BEND_SHIFT
			adda	:note
			asla
			sta	:note	offset of dptable (u16 scaled)
			ldd	:bend
			andd	#$7f	(1 << BEND_SHIFT) - 1
			tfr	d,v
			leax	:dptable
			ldf	:note
			addf	#2
			clre
			muld	w,x
			stw	:lo
			std	:hi
			ldd	#$80	1 << BEND_SHIFT
			subr	v,d
			ldf	:note
			clre
			muld	w,x
			addw	:lo
			adcd	:hi
			tfr	b,a
			tfr	e,b
			addf	#$80	MSB of F to Carry
			rold
			std	:dp
		}
		IOSound(p->adr, (u8)(dp & 0xff));
		IOSound(p->adr + 1, (u8)(dp >> 8));
	}
}

static void SetVol(SG *p) {
	u8 velo = p->velo, volex = p->volex, pan = p->pan;
	u16 env = p->env;
	u8 v, vl, vr;
	asm {
		lda	:velo
		ldb	:volex
		mul
		lsld
		sta	:v
		ldd	:env
		lsld
		ldb	:v
		mul
		tfr	a,b
		mul
		sta	:v
		ldb	#127
		subb	:pan
		mul
		lsld
		sta	:vl
		lda	:v
		ldb	:pan
		mul
		lsld
		sta	:vr
	}
	IOSound(p->adr + 2, vl);
	IOSound(p->adr + 3, vr);
}

static void UpdateEnvelope(SG *p) {
	static s16 attackRate[] = {
		32767, 32767, 32767, 27305, 13652, 6826, 3413, 1706 
	};
	static s16 decayRate[] = {
		-32767, -21844, -10922, -5461, -2730, -1365, -682, -341,
		-170, -85, -42, -21, -10, -5, -2, -1
	};
	static s16 sustainRate[] = {
		-8737, -4368, -2184, -1092, -546, -273, -136, 0
	};
	u16 d = sToneData[p->prog];
	s16 env = p->env, env0 = env;
	u8 state = p->state;
	asm {
		lda	:state
		bne	@l10
		ldd	:d
		andb	#AR_MASK
		lslb
		leax	:attackRate
		ldd	b,x
		addd	:env
		bpl	@l34
		ldd	#$7fff
		std	:env
		lda	#DECAY
		sta	:state
		bra	@l12
@l10:
		deca
		bne	@l20
@l12:
		ldd	:d
		tfr	d,v
		andd	#SL_MASK
		addd	#SL_OFS
		exg	d,v
		lsrb
		lsrb
		lsrb
		andb	#DR_MASK2
		leax	:decayRate
		ldd	b,x
		addd	:env
		cmpr	v,d
		bgt	@l34
@l11:
		tfr	v,d
		std	:env
		lda	#SUSTAIN
		sta	:state
		bra	@l22
@l20:
		deca
		bne	@l25
@l22:
		lda	:d
		anda	#SR_MASK
		lsla
		leax	:sustainRate
		ldd	a,x
		addd	:env
		bra	@l32
@l25:
		deca
		bne	@l39
		ldd	:env
		tim	#RELEASE_MASK_H,:d
		beq	@l31
		subd	#1092
		bgt	@l34
		bra	@l33
@l31:
		subd	#10922
@l32:
		bgt	@l34
@l33:
		lda	#DISPOSE
		sta	:state
		clrd
@l34:
		std	:env
@l39:
	}
	p->state = state;
	p->env = env;
	if ((env & 0x7f80) != (env0 & 0x7f80)) SetVol(p);
}

void SndKeyOn(u8 prog, u8 note, u8 velo, u8 volex, u8 pan, s16 bend, u16 id) {
	static const u8 wavesel[] = {
		0x00, 0x0f, 0x00, 0x0c, 0x55, 0x05, 0x55, 0x69, 
		0x55, 0xaa, 0x00, 0x30, 0x00, 0x8f, 0xaa, 0x55, 
		0x55, 0xc0, 0xff, 0xc0, 0x00, 0x0c, 0xc3, 0x00, 
		0xf0, 0x00, 0x0a, 0x10, 0x50, 0x00, 0x00, 0x00, 
	};
	static const s16 rd[] = {
		0x0480, 0x0480, 0x0601, 0x02c2, 0x0783, 0x0382, 0xa504, 0x4a85,
		0xd544, 0x4a85, 0xf584, 0x4a86, 0x15c4, 0x3604, 0x4907, 0x6644,
		0xca88, 0xc907, 0xca88, 0x0789, 0xc90a, 0x478b, 0xc907, 0x0000,
		0xca88, 0x7a0c, 0x794d, 0xba2e, 0xb86f, 0xc730, 0x4991, 0x4612,
		0x9913, 0x9873, 0x9a14, 0x8a15, 0x7ad6, 0x7a97, 0x0000, 0x0000,
		0x4c38, 0x79f9, 0x7959, 0x0000, 0x0000, 0x879a, 0x879b, 
	};
	if (sMute & 1 << (id & 0xf)) {
		if (sKeyCallback) sKeyCallback(id, 1);
		return;
	}
	else if (sKeyCallback) sKeyCallback(id, 2);
	if ((id & 0xff) == 9) {
		if (note < 35 || note > 81) return;
		s16 t = rd[note - 35];
		prog = (u8)(t & 0x1f | 0x80);
		note = (u8)(t >> 5 & 0x7f);
		pan = (u8)(0x40 + 5 * (t >> 12));
	}
	SG *p, *p0, *pn;
	u8 ref = (u8)((u16)velo * volex >> 8);
	for (p = sActiveCh; p && p->id != id; p = p->next)
		;
	if (!p) {
		if (sFreeCh) {
			p = sFreeCh;
			sFreeCh = p->next;
		}
		else if (sReleaseCh) {
			p = sReleaseCh; // smallest ref
			sReleaseCh = p->next;
		}
		else if (sActiveCh) {
			p = sActiveCh; // smallest ref
			sActiveCh = p->next;
		}
		else return;
		for (p0 = (SG *)&sActiveCh, pn = sActiveCh; pn && ref >= pn->ref; p0 = pn, pn = pn->next)
			;
		p->next = p0->next;
		p0->next = p;
	}
	p->id = id;
	p->prog = prog;
	p->velo = velo;
	p->volex = volex;
	p->ref = ref;
	p->note = note;
	p->pan = pan;
	p->env = 0;
	p->state = ATTACK;
	IOSound(p->adr + 2, 0);
	IOSound(p->adr + 3, 0);
	SetStep(p, bend);
	IOSound(p->adr + 4, prog);
	u8 t = wavesel[prog >> 2] >> ((prog & 3) << 1) & 3;
	if (t == 3) t = 4;
	IOSound(p->adr + 5, t);
}

void SndKeyOff(u16 id) {
	SG *p, *p0, *pn;
	if (sKeyCallback) sKeyCallback(id, 0);
	for (p0 = (SG *)&sActiveCh, p = sActiveCh; p && p->id != id; p0 = p, p = p->next)
		;
	if (p && !(sToneData[p->prog] & PERC_MASK)) {
		p->state = RELEASE;
		p0->next = p->next;
		for (p0 = (SG *)&sReleaseCh, pn = sReleaseCh; pn && p->ref >= pn->ref; p0 = pn, pn = pn->next)
			;
		p->next = p0->next;
		p0->next = p;
	}
}

void SndVolex(u8 id_low, u8 volex, u8 pan) {
	SG *p;
	for (p = sActiveCh; p; p = p->next) 
		if ((p->id & 0xff) == id_low) {
			p->volex = volex;
			p->pan = pan;
			SetVol(p);
		}
}

void SndBend(u8 id_low, s16 bend) {
	SG *p;
	for (p = sActiveCh; p; p = p->next) 
		if ((p->id & 0xff) == id_low) SetStep(p, bend);
}

void SndInit(void) {
	u8 a = 0;
	SG *p;
	for (p = sSG; p < sSG + SG_MAX; p++) {
		p->next = p + 1;
		p->adr = a;
		IOSound(a + 2, 0);
		IOSound(a + 3, 0);
		a += 8;
	}
	(--p)->next = 0;
	sActiveCh = sReleaseCh = 0;
	sFreeCh = sSG;
	sMute = 0;
	sKeyCallback = 0;
}

void SndUpdate(void) {
	SG *p, *p0, *pn;
	for (p = sActiveCh; p; p = p->next) UpdateEnvelope(p);
	for (p = sReleaseCh; p; p = p->next) UpdateEnvelope(p);
	for (p0 = (SG *)&sReleaseCh, p = sReleaseCh; p; p = pn) {
		pn = p->next;
		if (p->state == DISPOSE) {
			p0->next = pn;
			p->next = sFreeCh;
			sFreeCh = p;
		}
		else p0 = p;
	}
}

u8 SndToggleMute(u8 midi_ch) {
	u16 mask = 1 << (midi_ch & 0xf);
	sMute ^= mask;
	return (sMute & mask) != 0;
}

void SndSetKeyCallback(KeyCallback func) {
	sKeyCallback = func;
}
