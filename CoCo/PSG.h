#include <stdint.h>

class PSG {
	static const int N = 32, ATT = 9;
public:
	PSG() : rng(1 << 16), clips(0), samples(0) { Mute(); }
	void Mute();
	void Update(int16_t &l, int16_t &r);
	void Set(uint8_t adr, uint8_t data);
private:
	struct Op {
		uint16_t count, delta;
		uint8_t vl, vr, delta_tmp, vtmp, type;
	};
	Op op[N];
	int rng, clips, samples;
};
