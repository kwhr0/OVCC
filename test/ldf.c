#include <stdio.h>

int main(int argc, char *argv[]) {
	float f;
	if (argc != 2 || sscanf(argv[1], "%f", &f) != 1) return 1;
	union {
		float f;
		int i;
	} u = { f };
	int c = u.i & 0x100, exp = u.i >> 23 & 0xff;
	u.i &= ~0x1ff;
	if (c && (u.i & 0x7fffff) != 0x7fffff) u.i++;
	printf("\tldf(0x%x, 0x%x, c_); // %f\n", 
		u.i >> 9 & 0x3fff | (exp ? 0x4000 : 0) | u.i >> 16 & 0x8000, 
		exp, f);
	return 0;
}
