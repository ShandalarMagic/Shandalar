#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. Inject a call to copy_to_display_supplement() within copy_to_display().

use strict;
use warnings;
use Manalink::Patch;

###################################
# Inject within copy_to_display() #
###################################
# Previous contents:
#(40b928:	68 88 02 62 00		push	0x620288	; arg1 = &critical_section_for_display)
# 40b92d:	e8 ea a4 0c 00		call	0x4d5e1c	; EnterCriticalSection(&critical_section_for_display)
# 40b932:	...
patch("Magic.exe", 0x40b928,
      "68 88 02 62 00",			# push	0x620288	; (unchanged)
      "e9 61 77 ff ff");		# jmp	0x403093

#########################################
# Make C_generic_legend() just return 0 #
#########################################
# On the off chance it's still called from somewhere that I can't find.
# We'll be overwriting its corpse as injection endpoints.

# Previous contents:
# 403090	55			push	ebp
# 403091	8b ec			mov	ebp, esp
# 403093	...
patch(0x403090,
      "33 c0",				# xor	eax, eax
      "c3");				# ret

###############
# Call into C #
###############
patch(0x403093,
      "e8 84 2d 0d 00",			# call	0x4d5e1c	; EnterCriticalSection(&critical_section_for_display)
      "e8 51 83 c0 01",			# call	0x200b3ee	; copy_to_display_supplement()
      "e9 90 88 00 00");		# jmp	0x40b932	; return whence we came
#last used: 4030a1
