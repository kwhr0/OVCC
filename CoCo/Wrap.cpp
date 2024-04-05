extern "C" {
#include "defines.h"
}
#include "Wrap.h"
#include "HD6309.h"
#include "PSG.h"
#include <mutex>

static HD6309 sMPU;
static PSG sPSG;

void MPUReset(void) {
	sMPU.Reset();
}

int Execute(int n) {
	do n = -sMPU.Execute(n);
	while (n > 0);
	return n;
}

void Interrupt(unsigned char intr, unsigned char) {
	if (intr == 1) sMPU.IRQ();
	if (intr == 3) sMPU.NMI();
}

void HD6309DeAssertInterupt_new(unsigned char) {}
void HD6309ForcePC_new(unsigned short) {}

int MPUIsNative() {
	return sMPU.IsNative();
}

struct bin_sem {
	bin_sem() : f(false) {}
	void wait() {
		std::unique_lock<std::mutex> lock(m);
		cv.wait(lock, [=] { return f; });
		f = false;
	}
	void signal() {
		std::lock_guard<std::mutex> lock(m);
		if (!f) {
			f = true;
			cv.notify_one();
		}
	}
	bool f;
	std::mutex m;
	std::condition_variable cv;
};

static bin_sem render_sem, vsync_sem;
int vsync_count;

void render_wait() { render_sem.wait(); }
void render_signal() { render_sem.signal(); }
void vsync_wait() { vsync_sem.wait(); }
void vsync_signal() { vsync_sem.signal(); vsync_count++; }

static uint8_t psgadr;

void PSGAdr(uint8_t adr) { psgadr = adr; }
void PSGData(uint8_t data) { sPSG.Set(psgadr, data); }

uint32_t GetPSGSample() {
	int16_t l, r;
	sPSG.Update(l, r);
	return l << 16 | (uint16_t)r;
}

void PSGSamples(int16_t *buf, int n) {
	while (--n >= 0) {
		int16_t l, r;
		sPSG.Update(l, r);
		*buf++ = l;
		*buf++ = r;
	}
}
