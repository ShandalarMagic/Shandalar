# -*- makefile -*-

include ../Makefile-common

# Input files
BASEDIR = ..
SUBDIRS = src
LIBS = gdiplus boost_filesystem-mgw46-s-1_45 boost_system-mgw46-s-1_45

# Output files
OUTPUT = CardArtLib.dll

include ../Makefile-targets

install: ../../Cardartlib.dll

../../Cardartlib.dll: $(OUTPUT)
	/bin/cp -f CardArtLib.dll CardArtLib.dbg ../..
