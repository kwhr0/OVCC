#ifndef _TYPES_H_
#define _TYPES_H_

typedef signed char s8;
typedef signed short s16;
typedef signed long s32;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
#define TYPE_DEFINED

#define KEY_ESC		5
#define KEY_LEFT	8
#define KEY_RIGHT	9

#define VSYNC_CNT	(*(u8 *)0xffe0)

#define DEBUG_OUT	(*(u8 *)0xff70)
#define DEBUG_FLAG	(*(u8 *)0xff74)
#define PSG_ADR		(*(u8 *)0xffe0)
#define PSG_DATA	(*(u8 *)0xffe1)

#endif
