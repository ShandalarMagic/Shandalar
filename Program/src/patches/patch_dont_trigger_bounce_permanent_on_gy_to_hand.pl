#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Prevents finalize_graveyard_to_hand() from dispatching TRIGGER_BOUNCE_PERMANENT.  It takes its arguments in eax and ecx instead of the stack, so can't
#    easily be moved into C.  (Though only a handful of cards still call it, and it would be better in the long run to just translate those, but enh.)

use strict;
use warnings;
use Manalink::Patch;

################################
# finalize_graveyard_to_hand() #
################################
# Context:
#(401880)	50			push	eax			; save eax
#(401881)	51			push	ecx			; save ecx
#(401882)	e8 f9 01 00 00		call	0x401a80		; esi=get_card_instance(eax,ecx)
#(401887)	83 66 08 df		and	dword [esi+0x8], ~0x20	; esi->state &= ~STATE_INVISIBLE
#(40188b)	ff 04 85 c4 f1 4e 00	inc	dword [eax*4+0x4ef1c4]	; hand_count[eax]++
#(401892)	e8 c9 04 07 00		call	0x471d60		; call_sub_437E20_unless_ai_is_speculating()	// Redraws, I think
#(401897)	59			pop	ecx			; restore ecx
#(401898)	58			pop	eax			; restore eax
# Previous contents:
#401899:	a3 7c c1 62 00		mov	dword [0x62c17c], eax	; trigger_cause_controller = eax
#40189e:	89 0d 20 9a 73 00	mov	dword [0x739a20], ecx	; trigger_cause = ecx
#4018a4:	6a 00			push	0x0			; arg4 = 0
#4018a6:	68 00 8e 73 00		push	0x738e00		; arg3 = str_permanent_to_hand
#4018ab:	68 d8 00 00 00		push	0xd8			; arg2 = TRIGGER_BOUNCE_PERMANENT
#4018b0:	ff 35 70 86 62 00	push	dword [0x628670]	; arg1 = current_turn
#4018b6:	e8 e5 58 03 00		call	0x4371a0		; dispatch_trigger_twice_once_with_each_player_as_reason(current_turn, TRIGGER_BOUNCE_PERMANENT, str_permanent_to_hand, 0)
#4018bb:	83 c4 10		add	esp, 0x10		; pop 4 args off stack
#4018be:	c3			ret				; return
patch("Magic.exe", 0x401899,
      "c3",				# ret
      (0xcc) x 37);			# int3's over the rest of the function
