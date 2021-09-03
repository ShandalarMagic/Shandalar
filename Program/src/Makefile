# -*- makefile -*-

include Makefile-common

# Flags
LFLAGS := $(LFLAGS) -Wl,-Tmanalink.lds --entry _DllEntry

# Input files
SUBDIRS = cards functions
AFILES := $(AFILES) ManalinkEh.asm
LOCAL_LIBS = capturestackbacktrace

# Output files
OUTPUT = ManalinkEh.dll

include Makefile-targets

clean::
	-$(RM) ManalinkEh.ao