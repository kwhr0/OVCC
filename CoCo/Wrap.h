#ifndef Wrap_h
#define Wrap_h

#ifdef __cplusplus
extern "C" {
#endif

void MPUReset(void);
int Execute(int);
void Interrupt(unsigned char,unsigned char);
int MPUIsNative(void);

void render_wait(void);
void render_signal(void);
void vsync_wait(void);
void vsync_signal(void);

void PSGAdr(uint8_t adr);
void PSGData(uint8_t data);
uint32_t GetPSGSample(void);
void PSGSamples(int16_t *buf, int n);

extern int vsync_count;

#ifdef __cplusplus
}
#endif

#endif
