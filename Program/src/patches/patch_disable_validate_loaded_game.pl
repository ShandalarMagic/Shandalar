#!/bin/env perl

# Updates Magic.exe in-place.

# 1. Disables the validate_loaded_game() function, causing it to always return
#    true.  What it did was to check the original_internal_card_id of each card
#    instance and each card in each graveyard and library, and call the
#    savegame corrupt if the csvid corresponding to any of them had a
#    card_coded value of 0.  So any savegame with tokens, transformed or
#    flipped or permanently-alternate-type cards, etc. was unloadable.

use strict;
use warnings;
use Manalink::Patch;

###########################################
# Replace entire function with "return 1" #
###########################################
# Previous contents:
# 4a0850:	55			push	ebp
# 4a0851:	8b ec			mov	ebp, esp
# 4a0853:	83 ec 14		sub	esp, 0x14

patch("Magic.exe", 0x4a0850,
      "31 c0",				# xor	eax, eax	; eax = eax ^ eax	// eax = 0 (idiomatic)
      0x40,				# inc	eax		; ++eax			// eax = 1 (idiomatic)
      "c3",				# ret			; return eax
      (0x90) x 2);			# nop			; (two, so the following code is still aligned)
