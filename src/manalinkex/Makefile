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
COBJFILES := $(patsubst %.c,%.o,$(wildcard *.c))
TGTDLL = ManalinkEx.dll
TGTDBG = $(patsubst %.dll,%.dbg,$(TGTDLL))
DEFFILE = $(patsubst %.dll,%.def,$(TGTDLL))
INSTALLDIR = ../..

all: $(TGTDLL)

.PHONY: clean

install: $(INSTALLDIR)/$(TGTDLL) $(INSTALLDIR)/$(TGTDBG)

$(INSTALLDIR)/$(TGTDLL): $(TGTDLL)
	/bin/cp -f $(TGTDLL) $(INSTALLDIR)/$(TGTDLL)

$(INSTALLDIR)/$(TGTDBG): $(TGTDLL)
	/bin/cp -f $(TGTDBG) $(INSTALLDIR)/$(TGTDBG)

$(TGTDLL): $(COBJFILES)
	$(CC) -shared -m32 -o $@ $(COBJFILES) $(DEFFILE) $(LDFLAGS)
	objcopy --only-keep-debug $(TGTDLL) $(TGTDBG)
	objcopy --strip-debug $(TGTDLL)

%.o: %.c ../manalink.h $(HFILES)
	$(COMPILE) -o $@ $<

clean:
	$(RM) $(COBJFILES) $(LIBFILES) $(TGTDLL) $(TGTDBG)
