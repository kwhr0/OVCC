#include "midi.h"
#include "snd.h"
#include "file.h"
#include "util.h"
#include <cmoc.h>
#include <cgfx.h>

#define MIDIDIR		"/DD/MIDI"
#define DEPTH_N		4
#define FONT_YN		8
#define ENTRY_N		300
#define NAME_LEN	30

typedef char Entry[NAME_LEN];

static u8 file_id, dir_id, autokey;
static u16 trackMute;
static const char trackName[] = "1234567890QWERTY";

static void drawKey(u16 id, u8 c) {
	u8 t = (u8)(id >> 8);
	u16 x = t % 12;
	u8 f = (u8)(0xab5 >> x & 1);
	u16 y = 49 + FONT_YN * (id & 0xf) + 3 * f;
	x += x >= 5;
	x = 16 + (x << 1) + 28 * (u16)(t / 12);
	color(c ? c : f ? 3U : 0);
	boxfill(x, y, x + 2, y + 2);
	color(3);
}

static void initPlayer(void) {
	trackMute = 0;
	SndInit();
	SndSetKeyCallback(drawKey);
	color(3);
	u8 c, y;
	u16 x;
	for (y = 0; y < 16; y++) {
		locate(0, 6 + y);
		_putchar(trackName[y]);
	}
	boxfill(16, 6 * FONT_YN, 310, (6 + 16) * FONT_YN - 2);
	color(0);
	for (x = 19; x < 310; x += 4)
		line(x, 6 * FONT_YN, x, (6 + 16) * FONT_YN - 2);
	for (y = (6 + 1) * FONT_YN - 1; y < (6 + 16) * FONT_YN - 1; y += FONT_YN)
		line(16, y, 310, y);
	for (y = 6 * FONT_YN; y < (6 + 16) * FONT_YN; y += FONT_YN) {
		c = 0;
		for (x = 17; x < 310; x += 4) {
			if (c != 2 && c != 6) boxfill(x + 1, y, x + 3, y + 3);
			if (++c >= 7) c = 0;
		}
	}
	color(3);
}

static void toggleMute(u8 num) {
	u8 f = SndToggleMute(num);
	if (f) trackMute |= 1 << num;
	else trackMute &= ~(1 << num);
	color(f ? 1 : 3);
	locate(0, 6 + num);
	_putchar(trackName[num]);
}

static int cmp(const void *a, const void *b) {
	return strcmp((char *)a, (char *)b);
}

static s8 list(void) {
	static u8 playing, depth;
	static Entry *entry;
	static u16 entryN;
	static u16 entryIndex[DEPTH_N];
	u16 i;
	u8 c = 0;
	if (!entry) entry = (Entry *)alloc(sizeof(Entry) * ENTRY_N, 0);
	if (playing) {
		playing = 0;
		_os_close(file_id);
		locate(33, depth);
		_printf("       ");
	}
	else {
		locate(32, 0);
		_printf("reading");
		if (_os_open(".", S_DIR | S_IREAD, &dir_id)) return -1;
		char *name;
		for (i = 0; i < ENTRY_N && (name = readdir(dir_id));)
			strncpy(entry[i++], name, NAME_LEN - 1);
		if (_os_close(dir_id)) return -1;
		entryN = i;
		locate(32, 0);
		_printf("sorting");
		qsort(entry, entryN, sizeof(Entry), cmp);
		locate(32, 0);
		_printf("       ");
	}
	i = entryIndex[depth];
	while (1) {
		locate(depth, depth);
		_printf("%-29s", entry[i]);
		if (autokey) c = autokey;
		else while (!(c = _getchar())) {
			int tick = 2;
			_os9_sleep(&tick);
		}
		if (isalnum(c)) {
			c = (u8)toupper(c);
			for (i = 0; i < entryN && c > *entry[i]; i++)
				;
		}
		else if (c == KEY_LEFT) {
			if (i > 0) {
				i--;
				if (autokey) autokey = 13;
			}
			else autokey = 0;
		}
		else if (c == KEY_RIGHT) {
			if (i < entryN - 1) {
				i++;
				if (autokey) autokey = 13;
			}
			else if (autokey) autokey = KEY_ESC;
		}
		else if (c == 13) {
			if (chdir(entry[i])) {
				playing = 1;
				autokey = 0;
				locate(33, depth);
				_printf("PLAYING");
				entryIndex[depth] = i;
				logstart();
				_printf("%s\n", entry[i]);
				logend();
				if (_os_open(entry[i], S_IREAD, &file_id)) return -1;
				return 1;
			}
			else if (depth < DEPTH_N - 1) {
				entryIndex[depth++] = i;
				entryIndex[depth] = 0;
				return 0;
			}
			else chdir("..");
		}
		else if (c == KEY_ESC)
			if (depth && !chdir("..")) {
				locate(depth, depth);
				_printf("                             ");
				depth--;
				if (autokey) autokey = KEY_RIGHT;
				return 0;
			}
			else return -1;
	}
}

static void play1(void) {
	FileInit(file_id);
	MidiInit();
	MidiHeader();
	u16 delay = 0;
	u8 endcount = 0;
	while (1) {
		u8 v = VSYNC_CNT;
		if (MidiUpdate() && !endcount) endcount = 1;
		SndUpdate();
		u8 c = _getchar();
		if (c) {
			char *p = strchr(trackName, toupper(c));
			if (p) toggleMute((u8)(p - trackName));
		}
		if (c == 'a') {
			u8 i;
			if (trackMute) {
				for (i = 0; i < 16; i++)
					if (trackMute & 1 << i) toggleMute(i);
				trackMute = 0;
			}
			else {
				for (i = 0; i < 16; i++)
					if (!(trackMute & 1 << i)) toggleMute(i);
				trackMute = 0xffff;
			}
		}
		else if (c == KEY_ESC) {
			autokey = 0;
			break;
		}
		else if (c == KEY_LEFT || c == KEY_RIGHT) {
			autokey = c;
			break;
		}
		else if (endcount && ++endcount >= 30) {
			autokey = KEY_RIGHT;
			break;
		}
		Flush();
		if (v == VSYNC_CNT) FileUpdate();
		else {
			logstart();
			_printf("delayed %d\n", ++delay);
			logend();
		}
		while (v == VSYNC_CNT) {
			int tick = SLEEP_TICK;
			_os9_sleep(&tick);
		}
	}
}

void player(void) {
	if (chdir(MIDIDIR)) return;
	if (_os_open("/W", S_IREAD | S_IWRITE, &cur_path)) return;
	_cgfx_dwset(cur_path, 6, 0, 0, 40, 24, 0, 0, 0);
	_cgfx_curoff(cur_path);
	_cgfx_scalesw(cur_path, 0);
	_cgfx_select(cur_path);
	pal(0, 000);
	pal(1, 011);
	pal(2, 044);
	pal(3, 077);
	initTerm(0, 0);
	initPlayer();
	while (1) {
		s8 r = list();
		if (r > 0) {
			play1();
			initPlayer();
		}
		else if (r < 0) break; 
	}
	_os_close(cur_path);
	cur_path = 0;
}
