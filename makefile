CC ?= gcc

SRC = $(wildcard *.c)
OBJ = $(SRC:.c=.o)

MINIAUDIO_INCS = -Ithirdparty/miniaudio/include
MINIAUDIO_LIBS = -Lthirdparty/miniaudio/lib -lminiaudio -lm -lpthread

UMP_INCS = $(shell pkg-config --cflags gio-2.0 glib-2.0 ncursesw taglib_c) $(MINIAUDIO_INCS)
UMP_LIBS = $(shell pkg-config --libs gio-2.0 glib-2.0 ncursesw taglib_c) $(MINIAUDIO_LIBS)

UMP_CFLAGS = $(CFLAGS) $(UMP_INCS) \
	-Wall -Wextra -pedantic -Werror \
	-ggdb
UMP_LDFLAGS = $(LDFLAGS) $(UMP_LIBS)

all: ump

config.h: config.def.h
	cp config.def.h config.h

.c.o:
	$(CC) $(UMP_CFLAGS) -c $<

ump.o: config.h

ump: $(OBJ)
	$(CC) -o $@ $(OBJ) $(UMP_LDFLAGS)

clean:
	rm -f ump $(OBJ)

commands:
	make clean; bear -- make

.PHONY: all clean commands
