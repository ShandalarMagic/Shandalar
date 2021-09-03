#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Makes creatures that attacked this turn, but are not currently attacking, appear in territory instead of the combat window.

use strict;
use warnings;
use Manalink::Patch;

############################
# In wndproc_AttackClass() #
############################
# Previous contents:
#48fbac:	f6 85 9c fe ff ff 40	test	byte [ebp-0x164], 0x40	; if ((state & STATE_ATTACKED)
#48fbb3:	74 09			je	0x48fbbe		;     == 0) goto 0x48fbbe
patch("Magic.exe", 0x48fbac,
      "eb 10",		# jmp 0x48fbbe
      (0x90) x 7);	# nop

# Previous contents:
#48fc87:	f6 85 9c fe ff ff 40	test	byte [ebp-0x164], 0x40	; if ((state & STATE_ATTACKED)
#48fc8e:	74 68			je	0x48fcf8		;     == 0) goto 0x48fcf8
patch(0x48fc87,
      "eb 6f",		# jmp 0x48fcf8
      (0x90) x 7);	# nop
