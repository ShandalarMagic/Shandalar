#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Change ai_opinion_of_gamestate() to rate cards based on whether they have Defender (via the new keyword bit), rather than whether they have the wall
# subtype (via ct_all.csv::Family/card_data_t::subtype == 0, which is being removed).
# 2. Change ai_opinion_of_gamestate_continued() similarly.

use strict;
use warnings;
use Manalink::Patch;

################################
# In ai_opinion_of_gamestate() #
################################
# Previous contents:
# 499380:	8b 45 f0		mov	eax, dword [ebp-0x10]		; eax = iid
# 499383:	c1 e0 03		shl	eax, 0x3			; eax <<= 3
# 499386:	0f be 84 c0 39 70 7e 00	movsx	eax, byte [eax+eax*8+0x7e7039]	; eax = (int8_t)cards_data[iid].subtype
# 49938e:	85 c0			test	eax, eax			; if (eax & eax
# 499390:	75 11			jnz	0x4993a3			;     != 0) goto 0x4993a3	// skip zeroing of effective power
patch("Magic.exe", 0x499380,
      "f7 85 a0 fe ff ff 00 00 80 00",	# test	dword [ebp-0x160], 0x800000	; if ((abils & KEYWORD_DEFENDER)
      "74 17",				# jz	0x4993a3			;     == 0) goto 0x4993a3
      (0x90) x 6);			# nop,nop,nop,nop,nop,nop		; overwrite rest of previous check

##########################################
# In ai_opinion_of_gamestate_continued() #
##########################################
# Previous contents:
# 4998e2:	0f be 84 c0 39 70 7e 00	movsx	eax, byte [eax+eax*8+0x7e7039]	; eax = cards_data[iid].subtype
# 4998ea:	85 c0			test	eax, eax			; if (eax & eax
# 4998ec:	75 08			jnz	0x4998f6			;     != 0) goto 0x4998f6
patch(0x4998e2,
      "f7 46 28 00 00 80 00",		# test	dword [esi+0x28), 0x800000	; if ((esi->regen_status & KEYWORD_DEFENDER)
      "74 0b",				# jz	0x4998f6			;     == 0) goto 0x4998f6
      (0x90) x 3);			# nop,nop,nop				; overwrite rest of previous check
