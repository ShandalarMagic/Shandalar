CC = gcc
CFLAGS = -ggdb2 -O2 -m32 -fno-omit-frame-pointer -march=i586		\
	-mpreferred-stack-boundary=2 -Wall -Wextra -Wpointer-arith	\
	-Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings	\
	-Wshadow -W -Wstrict-prototypes -Wnested-externs -Winline	\
	-Werror -I.. -I.
LDFLAGS =
COMPILE = $(CC) $(CFLAGS) -c
RM = rm -f

HFILES := $(wildcard *.h)
TGTS = $(patsubst %.c,%.exe,$(wildcard *.c))
INSTALLDIR = ../..

all: testwhichgcc $(TGTS)

# A trap to keep me from accidentally compiling with cygwin gcc.
testwhichgcc:
	if [ "$$(which gcc)" != "/d/mingw/bin/gcc" ]; then echo "which gcc is $$(which gcc) instead of /d/mingw/bin/gcc"; false; else true; fi

install: testwhichgcc $(patsubst %,$(INSTALLDIR)/%,$(TGTS))

.PHONY: testwhichgcc clean

%.exe: %.c patch.h
	$(CC) $(CFLAGS) -static -static-libgcc -mwindows -s -o $@ $< $(LDFLAGS)

$(INSTALLDIR)/%: %
	/bin/cp $< $@

clean:
	$(RM) $(TGTS)
