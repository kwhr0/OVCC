#include "types.h"

#define cls()		_putchar('\x0c')
#define pal(x, d)	_cgfx_palette(cur_path, x, d)
#define color(c)	_cgfx_fcolor(cur_path, c)
#define point(x, y)	_cgfx_point(cur_path, x, y)
#define line(x0, y0, x1, y1)	(_cgfx_setdptr(cur_path, x0, y0), _cgfx_line(cur_path, x1, y1))
#define boxfill(x0, y0, x1, y1)	(_cgfx_setdptr(cur_path, x0, y0), _cgfx_bar(cur_path, x1, y1))
#define logstart()	(last_path = cur_path, cur_path = 0)
#define logend()	(cur_path = last_path)

void initTerm(u8 echo, u8 wait);
u8 _getchar(void);
void _putchar(char c);
void _printf(const char *format, ...);
void locate(u8 x, u8 y);
u8 chdir(const char *path);
char *readdir(u8 id);
void Flush(void);

void *alloc(u16 size, u8 clear);
s8 sini(u8 t);
s8 cosi(u8 t);

extern u8 cur_path, last_path;
