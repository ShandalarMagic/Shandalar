CC = gcc
ASM = yasm
CFLAGS = -ggdb2 -O2 -m32 -fno-omit-frame-pointer -march=i586		\
	-mpreferred-stack-boundary=2 -Wall -Wextra -Wpointer-arith	\
	-Wbad-function-cast -Wcast-qual -Wcast-align -Wwrite-strings	\
	-Wshadow -W -Wstrict-prototypes -Wnested-externs -Winline	\
	-Werror -I.. -I.
AFLAGS = -a x86 -m x86 -p nasm -r nasm -f win32 -I.. -I.
LDFLAGS = -L. $(patsubst %.dll,-l%,$(EXTERNDLLS)) -lgdi32 -lgdiplus -lshlwapi -lMsimg32
COMPILE = $(CC) $(CFLAGS) -c
RM = rm -f

HFILES := $(wildcard *.h)
COBJFILES := $(patsubst %.c,%.o,$(wildcard *.c))
AOBJFILES := $(patsubst %.asm,%.ao,$(subst DrawCard.asm,,$(wildcard *.asm)))
TESTAOBJFILES := $(patsubst %.asm,%.ao,$(wildcard t/*.asm))
EXTERNDLLS = image.dll Cardartlib.dll
LIBFILES = $(patsubst %.dll,lib%.a,$(EXTERNDLLS))
TGTDLL = Drawcardlib.dll
TGTDBG = $(patsubst %.dll,%.dbg,$(TGTDLL))
DEFFILE = $(patsubst %.dll,%.def,$(TGTDLL))
INSTALLDIR = ../..

all: testwhichgcc $(TGTDLL)

test: $(LIBFILES) $(COBJFILES) $(AOBJFILES) $(TESTAOBJFILES)
	$(CC) -m32 -o $@ $(COBJFILES) $(AOBJFILES) $(TESTAOBJFILES) $(LDFLAGS) -lwinmm
	./test.exe

.PHONY: testwhichgcc clean

install: testwhichgcc $(INSTALLDIR)/$(TGTDLL) $(INSTALLDIR)/$(TGTDBG)

$(INSTALLDIR)/$(TGTDLL): $(TGTDLL)
	/bin/cp -f $(TGTDLL) $(INSTALLDIR)/$(TGTDLL)

$(INSTALLDIR)/$(TGTDBG): $(TGTDLL)
	/bin/cp -f $(TGTDBG) $(INSTALLDIR)/$(TGTDBG)

$(TGTDLL): $(LIBFILES) $(COBJFILES) $(AOBJFILES)
	$(CC) -shared -m32 -o $@ $(COBJFILES) $(AOBJFILES) $(DEFFILE) $(LDFLAGS)
	objcopy --only-keep-debug $(TGTDLL) $(TGTDBG)
	objcopy --strip-debug $(TGTDLL)

%.ao: %.asm
	$(ASM) $(AFLAGS) -o $@ $<

%.o: %.c ../manalink.h $(HFILES)
	$(COMPILE) -o $@ $<

lib%.a: %.def
	dlltool -k -d $< -l $@

clean:
	$(RM) $(COBJFILES) $(AOBJFILES) $(LIBFILES) $(TGTDLL) $(TGTDBG) test.exe
