#include "float.h"

void addsubf(FLOAT a, FLOAT b, FLOAT r, u8 flag) {
	u8 exp, flag2 = 0;
	u16 m, mb;
	asm {
		pshs	y
*	if (E(a) < E(b) || E(a) == E(b) && M(a) < M(b))
		ldx	:a
		ldy	:b
		lda	2,x
		cmpa	2,y
		blo	@lb
		bne	@lc
		ldd	,x
		anda	#$7f
		tfr	d,w
		ldd	,y
		anda	#$7f
		subr	d,w
		bhs	@lc
*	exchange a and b
@lb		sty	:a
		stx	:b
		lda	:flag
		sta	:flag2
@lc		puls	y
*	m = M(a), mb = M(b), exp = E(a), n = exp - E(b)
		ldx	:a
		ldd	,x
		anda	#$7f
		std	:m
		ldf	2,x
		stf	:exp
		ldx	:b
		ldd	,x
		anda	#$7f
		std	:mb
		subf	2,x
*	mb >>= n, however, jump to output if n >= 15
		beq	@l8
		cmpf	#15
		blo	@la
		ldd	:m
		bra	@l9
@la		ldd	:mb
@l7		lsrd
		decf
		bne	@l7
		std	:mb
*	if (S(a) ^ S(b) ^ (0x80 if subtract))
@l8		lda	[:a]
		eora	[:b]
		eora	:flag
		anda	#$80
		beq	@l5
*	m -= mb; // 0 - 0x7fff
*	if (m) while (exp && !(m & 0x4000)) m <<= 1, --exp
*	else exp = 0
		clrf
		ldd	:m
		subd	:mb
		beq	@l1
		ldf	:exp
		bra	@l2
@l3		asld
		decf
@l2		beq	@l4
		bita	#$40
		beq	@l3
@l4		std	:m
@l1		stf	:exp
		bra	@l9
*	m += mb; // 0 - 0xfffe
*	if (m & 0x8000) { // normalize
*		m >>= 1; // 0x4000 - 0x7fff
*		if (++exp > 0xff) exp = 0xff;
*	}
@l5		ldd	:m
		addd	:mb
		bpl	@l6
		lsrd
		inc	:exp
		bne	@l6
		dec	:exp
@l6		std	:m
*	S = S(a) ^ (0x80 if subtract and exchanged)
@l9		lda	[:a]
		eora	:flag2
		anda	#$80
		ora	:m
		ldx	:r
		std	,x
		lda	:exp
		sta	2,x
	}
}

void mulf(FLOAT a, FLOAT b, FLOAT r) {
	u8 exp = 0;
	u16 t;
	asm {
*	if (!E(a) || !E(b)) goto output
		ldx	:b
		clra
		ldb	2,x
		std	:t
		beq	@l3
		ldx	:a
		ldb	2,x
		beq	@l3
*	exp = E(a) + E(b) - EOFS
*	if (exp < 0) exp = 0 and goto output
*	if (exp > 0xff) exp = 0xff and goto output
		addd	:t
		subd	#EOFS
		bmi	@l3
		tsta
		beq	@l4
		ldb	#$ff
		stb	:exp
		bra	@l3
@l4		stb	:exp
*	m = M(a) * M(b) >> 15
		ldd	[:a]
		anda	#$7f
		std	:t
		ldd	[:b]
		anda	#$7f
		muld	:t
		adde	#$80	C=MSB
		rold
*	if ((!m & 0x4000)) m <<= 1
*	else if (++exp > 0xff) exp = 0xff
		bita	#$40
		bne	@l2
		addr	e,e	shift left E
		adde	#$80	C=MSB
		rold
		bra	@l1
@l2		inc	:exp
		bne	@l1
		dec	:exp
@l1		sta	:t
		bra	@l5
@l3		clrb
*	S = S(a) ^ S(b)
@l5		lda	[:a]
		eora	[:b]
		anda	#$80
		ora	:t	upper byte of :t is 0 if via @l3
		ldx	:r
		std	,x
		lda	:exp
		sta	2,x
	}
}

void divf(FLOAT a, FLOAT b, FLOAT r) {
	u8 exp = 0xff;
	u16 t, bm;
	asm {
*	if (!E(b) || !M(b)) goto output
		ldx	:b
		clra
		ldb	2,x
		std	:t
		beq	@l9
		ldd	[:b]
		anda	#$7f
		std	:bm
		beq	@l9
*	exp = E(a) - E(b) + EOFS
*	if (exp < 0) exp = 0 and goto output
*	if (exp > 0xff) exp = 0xff and goto output
		ldx	:a
		clra
		ldb	2,x
		subd	:t
		addd	#EOFS
		bpl	@l2
		clrb
		stb	:exp
		bra	@l9
@l2		tsta
		bne	@l9
		stb	:exp
*	m = (M(a) << 14) / M(b)
		ldd	[:a]
		rord
		rord
		sta	:t
		rora
		anda	#$c0
		tfr	a,e
		clrf
		lda	:t
		anda	#$1f
		divq	:bm
*	if (m) while (exp && !(m & 0x4000)) m <<= 1, --exp
*	else exp = 0
		beq	@l1	F=0
		tfr	w,d	d=m
		ldf	:exp
		bra	@l5
@l3		asld
		decf
@l5		beq	@l4
		bita	#$40
		beq	@l3
@l4		sta	:t
@l1		stf	:exp
*	S = S(a) ^ S(b)
@l9		lda	[:a]
		eora	[:b]
		anda	#$80
		ora	:t
		ldx	:r
		std	,x
		lda	:exp
		sta	2,x
	}
}

void ldif(int v, FLOAT r) {
	u8 EOFS14 = EOFS + 14;
	asm {
		clrd
		ldx	:r
		std	,x
		sta	2,x
		ldd	:v
		beq	@l1
*	exp = EOFS + 14;
		lda	:EOFS14
		sta	2,x
*	m = v & 0x7fff
*	while (!(m & 0x4000)) m <<= 1, --exp
		ldd	:v
		bita	#$80
		beq	@l2
		negd
		bra	@l2
@l3		asld
		dec	2,x
@l2		bita	#$40
		beq	@l3
*	set sign
		std	,x
		lda	:v
		anda	#$80
		ora	,x
		sta	,x
@l1
	}
}
