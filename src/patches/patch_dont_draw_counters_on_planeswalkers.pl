#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. If dword_0x12702e0 is set to nonzero during DrawSmallCard(), then don't draw generic counters during the next call to draw_standard_counters().
#
# This patch has been reverted, and is superseded by patch_counters.pl.

use strict;
use warnings;
use Manalink::Patch;

###################################
# Inject within draw_small_card() #
###################################
# Previous contents:
# 4d322d:	8b 85 28 ff ff ff	mov	eax, dword [ebp-0xd8]	; eax = unknown0x121
# 4d3233:	50			push	eax			; arg7 = unknown0x121
# 4d3234:	8b 85 20 ff ff ff	mov	eax, dword [ebp-0xe0]	; eax = counters
# 4d323a:	50			push	eax			; arg6 = counters
# 4d323b:	8b 85 24 ff ff ff	mov	eax, dword [ebp-0xdc]	; eax = special_counters_byte3
# 4d3241:	50			push	eax			; arg5 = special_counters_byte3
# 4d3242:	8b 85 2c ff ff ff	mov	eax, dword [ebp-0xd4]	; eax = special_counters_byte2
# 4d3248:	50			push	eax			; arg4 = special_counters_byte2
# 4d3249:	8b 85 30 ff ff ff	mov	eax, dword [ebp-0xd0]	; eax = special_counters_byte1
# 4d324f:	50			push	eax			; arg3 = special_counters_byte1
# 4d3250:	8d 85 38 ff ff ff	lea	eax, [ebp-0xc8]		; eax = &rect_counter
# 4d3256:	50			push	eax			; arg2 = &rect_counter
# 4d3257:	8b 45 08		mov	eax, dword [ebp+0x8]	; eax = hdc
# 4d325a:	50			push	eax			; arg1 = hdc
# 4d325b:	e8 60 0a 00 00		call	0x4d3cc0		; eax = draw_standard_counters(...)
# 4d3260:	83 c4 1c		add	esp, 0x1c		; pop 7 args
# 4d3263:	...
#patch("Magic.exe", 0x4d322d,
#      "e9 8e d0 d9 00",	# jmp #x12702c0
#      (0x90) x 2,	# 2 nops (to align)
#      (0x00) x 4,	# space for var
#      (0x90) x 43);	# 43 nops (to clear rest of function call)

#REVERT:
#patch("Magic.exe", 0x4d322d,
#      "8b 85 28 ff ff ff 50 8b 85 20 ff ff ff 50 8b 85 24 ff ff ff 50 8b 85 2c ff ff ff 50 8b 85 30 ff ff ff 50 8d 85 38 ff ff ff 50 8b 45 08 50 e8 60 0a 00 00 83 c4 1c");	# unpatch

##############################################################################################################
# Check global set within DrawSmallCard(), maybe clear counters local var, and call draw_standard_counters() #
##############################################################################################################
#patch("ManalinkEx.dll", 0x12702c0,
#      "a1 34 32 4d 00",		# mov	eax, dword [0x4d3234]	; eax = *0x4d3234
#      "85 c0",				# test	eax, eax		; if (eax
#      "74 15",				# jz	dont_clear		;     == 0) goto dont_clear
#      "b8 00 00 00 00",		# mov	eax, 0x0		; eax = 0
#      "89 85 20 ff ff ff",		# mov	dword [ebp-0xe0], eax	; counters = eax
#      "c7 05 34 32 4d 00 00 00 00 00",	# mov	dword [0x4d3234], 0x0	; *0x4d3234 = 0
##dont_clear:
#      "8b 85 28 ff ff ff",		# mov	eax, dword [ebp-0xd8]	; eax = unknown0x121
#      0x50,				# push	eax			; arg7 = unknown0x121
#      "8b 85 20 ff ff ff",		# mov	eax, dword [ebp-0xe0]	; eax = counters
#      0x50,				# push	eax			; arg6 = counters
#      "8b 85 24 ff ff ff",		# mov	eax, dword [ebp-0xdc]	; eax = special_counters_byte3
#      0x50,				# push	eax			; arg5 = special_counters_byte3
#      "8b 85 2c ff ff ff",		# mov	eax, dword [ebp-0xd4]	; eax = special_counters_byte2
#      0x50,				# push	eax			; arg4 = special_counters_byte2
#      "8b 85 30 ff ff ff",		# mov	eax, dword [ebp-0xd0]	; eax = special_counters_byte1
#      0x50,				# push	eax			; arg3 = special_counters_byte1
#      "8d 85 38 ff ff ff",		# lea	eax, [ebp-0xc8]		; eax = &rect_counter
#      0x50,				# push	eax			; arg2 = &rect_counter
#      "8b 45 08",			# mov	eax, dword [ebp+0x8]	; eax = hdc
#      0x50,				# push	eax			; arg1 = hdc
#      "e8 af 39 26 ff",		# call	0x4d3cc0		; eax = draw_standard_counters(...)
#      "83 c4 1c",			# add	esp, 0x1c		; pop 7 args
#      "e9 4a 2f 26 ff",		# jmp	0x4d3263		; goto 0x4d3263
#      0x90);				# nop (to align)

#REVERT:
#patch("ManalinkEx.dll", 0x12702c0,
#      (0x90) x 90);
