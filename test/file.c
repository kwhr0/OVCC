#include "file.h"
#include "util.h"
#include <fcntl.h>

#define SECTOR	256
#define BUF_N	(4 * SECTOR)

static u8 *sBuf, *sBufRP, *sBufWP, *sBufEP;
static u16 sBufN, sUnderRun;
static u8 sError, file_id;
static s32 sRemain;

void FileInit(u8 id) {
	file_id = id;
	if (!sBuf) sBuf = (u8 *)alloc(BUF_N, 0);
	sBufRP = sBufWP = sBuf;
	sBufEP = 0;
	sBufN = sUnderRun = 0;
	sError = 0;
	for (u8 i = 0; i < BUF_N / SECTOR; i++) FileUpdate();
	sRemain = -1;
}

void FileUpdate(void) {
	if (sError || sBufEP || sBufN > BUF_N - SECTOR) return;
	int cnt = SECTOR;
	if (_os_read(file_id, sBufWP, &cnt)) {
		sError = 1;
		return;
	}
	sBufN += cnt;
	if (cnt < SECTOR) sBufEP = sBufWP + cnt;
	else if ((sBufWP += SECTOR) >= sBuf + BUF_N) sBufWP = sBuf;
}

s16 FileGetChar(void) {
	u8 c;
	if (sError || sBufRP == sBufEP || !sRemain) return -1;
	if (!sBufN) {
		logstart();
		_printf("underrun %d\n", ++sUnderRun);
		logend();
		FileUpdate();
	}
	if (!sBufN) return -1;
	sBufN--;
	c = *sBufRP++;
	if (sBufRP >= sBuf + BUF_N) sBufRP = sBuf;
	if (sRemain > 0) sRemain--;
	return c;
}

void FileSetRemain(u32 r) {
	sRemain = r;
}
