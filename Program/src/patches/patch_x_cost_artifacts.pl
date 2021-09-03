#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Makes artifacts (and other spells) with a mana cost of X, XX, XXX, etc. - just variable mana, and none fixed - properly charge the variable mana.  The bug
# here is that, after charging the non-variable part of the mana cost, charge_spell_cost() examines the amount of mana spent, rather than whether paying that
# amount was cancelled; so when the fixed part of the mana cost is 0, it thinks you cancelled and doesn't charge the variable part.
#
# Normally, I'd move the whole function into C and just fix it, but both this function and some of the ones it calls use a nonstandard calling convention, so
# that's very inconvenient.  Failing that, I'd fix it directly in assembly, but there isn't enough room at or near the faulty comparison to compare cancel
# instead of charge_mana_w_global_cost_mod()'s return value.  So I'm instead exploiting the current layout of charge_mana_w_global_cost_mod(), which compares
# cancel against 1 as almost the very last thing it does before returning; the flags registers from that comparison are still valid.

use strict;
use warnings;
use Manalink::Patch;

#######################################################################################################
# Use already-set ZF from within charge_mana_w_global_cost_mod() instead of checking its return value #
#######################################################################################################
# Previous contents:
#(4026ad)	6a 00		push	0x0		; arg4 = 0
#(4026af)	6a 00		push	0x0		; arg3 = 0
#(4026b1)	ff 75 0c	push	dword [ebp+0xc]	; arg2 = card
#(4026b4)	ff 75 08	push	dword [ebp+0x8]	; arg1 = player
#(4026b7)	e8 04 dc 02 00	call	0x4302c0	; eax = charge_mana_w_global_cost_mod(player, card, 0, 0);
#(4026bc)	83 c4 10	add	esp, 0x10	; pop 4 arguments off the stack
# 4026bf:	85 c0		test	eax, eax	; zf = (eax & eax == 0)
#(4026c1)	74 54		jz	0x402717	; if (zf) goto 0x402717

#patch("Magic.exe", 0x4026bf,
#      (0x90) x 2);	# nop		; comment out the check of eax

# Withdraw; have to move the function into C and fix it properly, since there's
# no way to suppress charging X when a spell is cast for free.
patch("Magic.exe", 0x4026bf,
      "85 c0");
