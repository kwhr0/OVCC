#include "util.h"
#include "snd.h"
#include <cmoc.h>
#include <os.h>

void star_and_bacura(void);
void player(void);
void mandelbrot(u8 graph);

static u8 ignore_term;

static /*interrupt*/ void signal(void) {
	asm {
		cmpb	#2	keyboard terminate
		bne	@l1
		ldb	:ignore_term
		beq @l1
		rti
@l1:
	}
	initTerm(0, 0);
	_getchar();
	SndInit();
	exit(0);
}

int main(void) {
	struct _registers_6809 reg;
	reg.x = (u16)&signal;
	reg.u = (u16)alloc(4096, 0);
	_os_syscall(F$Icpt, &reg);
	initTerm(1, 1);
	//
	_printf("\n1. Star and Bacura\n2. MIDI player\n3. Mandelbrot (graphic)\n4. Mandelbrot (text)\n\n> ");
	u8 c = _getchar();
	ignore_term = 1;
	switch (c) {
	case '1':
		star_and_bacura();
		break;
	case '2':
		player();
		break;
	case '3':
		mandelbrot(1);
		break;
	case '4':
		mandelbrot(0);
		break;
	}
	return 0;
}
