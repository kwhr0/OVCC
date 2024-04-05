#ifndef TYPE_DEFINED
typedef unsigned char u8;
typedef unsigned short u16;
#endif

#define EOFS		127
#define addf(a, b, r)	addsubf(a, b, r, 0)
#define subf(a, b, r)	addsubf(a, b, r, 0x80)
#define cpf(a, r)	(*(u16 *)(r) = *(u16 *)(a), (r)[2] = (a)[2])
#define negf(a, r)	(*(u16 *)(r) = *(u16 *)(a) ^ 0x8000, (r)[2] = (a)[2])
#define ldf(m, e, r)	(*(u16 *)(r) = (m), (r)[2] = (e))
#define mif(a)		(((a)[0] & 0x80) != 0)
#define mizf(a)		(!((a)[2]) || ((a)[0] & 0x80) != 0)

// bit23: sign bit22-8: mantissa bit7-0: exp
typedef u8 FLOAT[3];

void addsubf(FLOAT a, FLOAT b, FLOAT r, u8 flag);
void mulf(FLOAT a, FLOAT b, FLOAT r);
void divf(FLOAT a, FLOAT b, FLOAT r);
void ldif(int v, FLOAT r);
