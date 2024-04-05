.SUFFIXES: .d

PROGRAM = g
LDF = ldf
DISK = ../copy/63EMU.dsk
SRCS = $(PROGRAM).c mandelbrot.c float.c starbacura.c \
	player.c midi.c snd.c file.c util.c
LIBPATH = l
CC = cmoc
CFLAGS = -Wno-const -I$(LIBPATH)/include -I$(LIBPATH)/cgfx/include --os9 --function-stack=0 -i

all: $(SRCS:.c=.d) $(PROGRAM) $(LDF)

$(PROGRAM): $(SRCS:.c=.o)
	$(CC) --os9 -o $(PROGRAM) $(SRCS:.c=.o) -L$(LIBPATH)/lib -lc -L$(LIBPATH)/cgfx -lcgfx
	os9 copy -r $(PROGRAM) $(DISK),
	os9 attr -q -epe $(DISK),$(PROGRAM)

$(LDF): $(LDF).c
	cc $< -o $@

.c.d:
	$(CC) --deps-only $(CFLAGS) $<

clean:
	rm -f *.{d,link,lst,map,o,s} $(PROGRAM) $(LDF)

ifneq ($(MAKECMDGOALS),clean)
-include $(SRCS:.c=.d)
endif
