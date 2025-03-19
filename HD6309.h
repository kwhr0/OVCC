// HD6309
// Copyright 2022-2025 © Yasuo Kuwahara
// MIT License

#include "tcc1014mmu.h"		// OVCC
#undef IRQ
#undef FIRQ
#undef NMI
#include <cstdint>

#define HD6309_TRACE	0

#if HD6309_TRACE
#define HD6309_TRACE_LOG(adr, data, type) \
	if (tracep->index < ACSMAX) tracep->acs[tracep->index++] = { adr, (u16)data, type }
#else
#define HD6309_TRACE_LOG(adr, data, type)
#endif

class HD6309 {
	using s8 = int8_t;
	using u8 = uint8_t;
	using s16 = int16_t;
	using u16 = uint16_t;
	using s32 = int32_t;
	using u32 = uint32_t;
	enum { M_SYNC = 1, M_IRQ = 2, M_FIRQ = 4, M_NMI = 8 };
	enum { W_SYNC = 1, W_CWAI };
	enum {
		LC, LV, LZ, LN, LI, LH, LF, LE
	};
	enum {
		MC = 1 << LC, MV = 1 << LV, MZ = 1 << LZ, MN = 1 << LN,
		MI = 1 << LI, MH = 1 << LH, MF = 1 << LF, ME = 1 << LE
	};
	enum {
		FD = 1, F0, F1, FDEF, FADD, FSUB, FMUL, FDIV, FLEFT
	};
	#define F(flag, type)	flag##type = F##type << (L##flag << 2)
	enum {
		F(C, D), F(C, 0), F(C, 1), F(C, DEF), F(C, ADD), F(C, SUB),
		F(C, LEFT), F(C, MUL), F(C, DIV),
		F(V, D), F(V, 0), F(V, 1), F(V, DEF), F(V, ADD), F(V, SUB),
		F(V, LEFT),
		F(Z, 0), F(Z, 1), F(Z, DEF),
		F(N, 0), F(N, DEF),
		F(H, ADD)
	};
	#undef F
public:
	HD6309();
	void Reset();
	int Execute(int n);
	u16 GetPC() const { return (u16 &)r[10]; }
	bool IsNative() const { return md & 1; }
	void SYNC() { irq |= M_SYNC; }
	void IRQ() { irq |= M_IRQ; }
	void FIRQ() { irq |= M_FIRQ; }
	void NMI() { irq |= M_NMI; }
private:
	// customized access: OVCC
	s32 imm8() {
		u8 data = MemRead8(((u16 &)r[10])++);
#if HD6309_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data;
#endif
		return data;
	}
	s32 imm16() {
		u32 adr = (u16 &)r[10];
		((u16 &)r[10]) += 2;
		u16 data = MemRead16(adr);
#if HD6309_TRACE
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data >> 8;
		if (tracep->opn < OPMAX) tracep->op[tracep->opn++] = data;
#endif
		return data;
	}
	s32 ld8(u16 adr) {
		s32 data = MemRead8(adr);
		HD6309_TRACE_LOG(adr, data, acsLoad8);
		return data;
	}
	s32 ld16(u16 adr) {
		s32 data = MemRead16(adr);
		HD6309_TRACE_LOG(adr, data, acsLoad16);
		return data;
	}
	void st8(u16 adr, u8 data) {
		MemWrite8(data, adr);
		HD6309_TRACE_LOG(adr, data, acsStore8);
	}
	void st16(u16 adr, s16 data) {
		MemWrite16(data, adr);
		HD6309_TRACE_LOG(adr, data, acsStore16);
	}
	void st16r(u16 adr, s16 data) {
		MemWrite16(data, adr); // differs writing order
		HD6309_TRACE_LOG(adr, data, acsStore16);
	}
	// customized access -- end
	template<int S = 0> void fnz(u16 a) { fset<NDEF | ZDEF, S>(a); }
	template<int S = 0> void fmov(u16 a, u16 d = 0) { fset<NDEF | ZDEF | V0, S>(a, d); }
	template<int S = 0> u16 fadd(u16 a, u16 d, u16 s) { return fset<NDEF | ZDEF | VADD | CADD, S>(a, d, s); }
	template<int S = 0> u16 fsub(u16 a, u16 d, u16 s) { return fset<NDEF | ZDEF | VSUB | CSUB, S>(a, d, s); }
	template<int S = 0> void fneg(u16 a) { fset<NDEF | ZDEF | VDEF | CDEF, S>(a); }
	template<int S = 0> u16 finc(u16 a, u16 d) { return fset<NDEF | ZDEF | VADD, S>(a, d); }
	template<int S = 0> u16 fdec(u16 a, u16 d) { return fset<NDEF | ZDEF | VSUB, S>(a, d); }
	template<int S = 0> void fcom(u16 a) { fset<NDEF | ZDEF | V0 | C1, S>(a); }
	template<int S = 0> void fdiv(u16 a, u16 d) { fset<NDEF | ZDEF | VD | CDIV, S>(a, d); }
	template<int S = 0> u16 fleft(u16 a, u16 d) { return fset<NDEF | ZDEF | VLEFT | CLEFT, S>(a, d); }
	template<int S = 0> u16 fright(u16 a, u16 d) { return fset<NDEF | ZDEF | CD, S>(a, d); }
	//
	u16 ea();
	template<typename T> void bcc(T cond) { (u16 &)r[10] += cond() ? (s8)imm8() : 1; }
	template<typename T> void lbcc(T cond) { (u16 &)r[10] += cond() ? imm16() : 2; }
	template<typename T> void bitTransfer(T op);
	template<typename T1, typename T2> void interRegister(T1 f8, T2 f16);
	void transfer(u8 op, u8 t8);
	void psh(bool s, u8 t, bool wp = false);
	void pul(bool s, u8 t, bool wp = false);
	void trap(u16 vec, u8 p, u8 m);
	void ProcessInterrupt();
	void divd(s8 s);
	void divq(s16 s);
	void tfm(int s, int d);
	template<int M, int SI = 0> u16 fset(u16 a = 0, u16 d = 0, u16 s = 0);
//
	const u8 *clktbl, *clktbl10, *clktbl11, *clktblea;
	u8 r[16];
	u8 cc, md, irq, waitflags;
	u16 dp; // 8-bit shift left
	u32 clock;
#if HD6309_TRACE
	static constexpr int TRACEMAX = 10000;
	static constexpr int ACSMAX = 19; // max case interrupt follows pshs/puls
	static constexpr int OPMAX = 4;
	enum {
		acsStore8 = 4, acsStore16, acsLoad8, acsLoad16
	};
	struct Acs {
		u16 adr, data;
		u8 type;
	};
	struct TraceBuffer {
		u16 r[8];
		Acs acs[ACSMAX];
		u8 op[OPMAX];
		u8 index, cc, opn;
	};
	TraceBuffer tracebuf[TRACEMAX];
	TraceBuffer *tracep;
public:
	void StopTrace();
#endif
};
