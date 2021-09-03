#!/bin/env perl

# Updates Shandalar.exe in-place.
# 1. Flags the .text segment of Shandalar.exe as writeable (so we can fix pointers at runtime from Shandalar.dll)
# 2. Calls init_shandalar() from shandalar.dll at the very start of shandlar.exe's WinMain().

use strict;
use warnings;
use Manalink::Patch;

#############
# in header #
#############
# Previous contents:
#19c:		20 00 00 60			CONTENTS, ALLOC, LOAD, READONLY, CODE
patch("RAW", "Shandalar.exe", 0x19c,
      "40 00 00 e0");				#CONTENTS, ALLOC, LOAD, CODE, DATA

################
# in Winmain() #
################
# Previous contents:
#4cdd7c:	c7 85 64 ff ff ff 00 00 00 00	mov	dword [ebp-0x9c],0x0
patch("Shandalar.exe", 0x4cdd7c,
      "e9 7f a8 ff ff",				# jmp	0x4c8600
      (0x90) x 5);				# nop

####################
# injection target #
####################
# Previous contents: nop's from 0x4c85f8 all the way to 0x4c8b1f (the result of moving a function into image.dll)
patch(0x4c8600,
      "ff 75 10",			# push	dword [ebp+0x10]	; arg1 = lpCmdLine
      "e8 3c 00 00 00",			# call	0x4c8644		; eax = call_shandalar_dll_init_shandalar(lpCmdLine)
      "83 c4 04",			# add	esp, 0x4		; pop 1 arg off stack
      "89 45 10",			# mov	dword [ebp+0x10], eax	; lpCmdLine = eax
      "c7 85 64 ff ff ff 00 00 00 00",	# mov	dword [ebp-0x9c], 0x0	; var_9C = 0		// the instruction we displaced
      "e9 69 57 00 00",			# jmp	0x4cdd86		; go back to injection point
      (0x90) x 3,			# nop				; for alignment
      "00 00 00 00",			# dd	0			; shandalar_dll_handle
      string_literal("shandalar.dll"),	# db	'shandalar.dll',0	; string constant
      (0x90) x 2,			# nop				; for alignment
      string_literal("init_shandalar"),	# db	'init_shandalar',0	; string constant
      0x90,				# nop				; for alignment

#char* call_shandalar_dll_init_shandalar(const char* lpCmdLine)
      0x55,				# push	ebp			; save stack
      "89 e5",				# mov	ebp, esp		; new stack
      "68 24 86 4c 00",			# push	0x4c8624		; arg1 = "shandalar.dll"
      "ff 15 70 79 98 00",		# call	dword [0x987970]	; eax = LoadLibraryA("shandalar.dll")
      "a3 20 86 4c 00",			# mov	0x4c8620, eax		; shandalar_dll_handle = eax
      "68 34 86 4c 00",			# push	0x4c8634		; arg2 = "init_shandalar"
      0x50,				# push	eax			; arg1 = shandalar_dll_handle
      "ff 15 d8 78 98 00",		# call	dword [0x9878d8]	; eax = GetProcAddress(shandalar_dll_handle, "init_shandalar")
      "ff 75 08",			# push	dword [ebp+0x8]		; arg1 = lpCmdLine
      "ff d0",				# call	eax			; eax = eax(lpCmdLine)
      "83 c4 04",			# add	esp, 0x4		; pop 1 arg off stack
      "c9",				# leave				; restore stack
      "c3");				# ret				; return
