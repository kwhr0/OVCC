#include "types.h"

//#define LOW_LOAD

#ifdef LOW_LOAD
#define INTERVAL	33333		// 2*VSync interval [uS]
#define SLEEP_TICK	2
#else
#define INTERVAL	16667		// VSync interval [uS]
#define SLEEP_TICK	1
#endif

typedef void (*KeyCallback)(u16 id, u8 type);

void SndInit(void);
void SndUpdate(void);
void SndKeyOn(u8 prog, u8 note, u8 velo, u8 volex, u8 pan, s16 bend, u16 id);
void SndKeyOff(u16 id);
void SndVolex(u8 id_low, u8 volex, u8 pan);
void SndBend(u8 id_low, s16 bend);
u8 SndToggleMute(u8 midi_ch);
void SndSetKeyCallback(KeyCallback func);
