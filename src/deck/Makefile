# -*- makefile -*-

include ../Makefile-common

# Input files
BASEDIR = ..
LOCAL_LIBS = Drawcardlib image
LIBS = comctl32 gdi32 shlwapi
LFLAGS += -Wl,--enable-stdcall-fixup

# Output files
OUTPUT = DeckDll.dll

ALLOBJFILES += DeckDll.res.obj

#VERSIONRAW := $(shell git describe --long --tags --match deckbuilder)
#DIST = $(VERSIONRAW).zip

include ../Makefile-targets

DeckDll.res.obj: DeckDll.rc resources.h
	windres --input=$< --output=$@ --output-format=coff

install: ../../DeckDll.dll

#release: $(DIST)
#
#$(DIST): $(OUTPUT)
#	ln -s ../../Menus.txt

../../DeckDll.dll: $(OUTPUT)
	/bin/cp -f DeckDll.dll DeckDll.dbg ../..
