#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Always sets tapped_for_mana_color to -1 before dispatching EVENT_ACTIVATE, and eventually EVENT_TAP_CARD, in activate().

use strict;
use warnings;
use Manalink::Patch;

########################################################
# Eliminate check before setting tapped_for_mana_color #
########################################################
#Previous contents:
#43446b:	8b 46 6c			mov	eax, dword [esi+0x6c]			; eax = esi->internal_card_id
#43446e:	c1 e0 03			shl	eax, 0x3				; (indexing to right cards_data entry)
#434471:	66 f7 84 c0 4c 70 7e 00 00 10	test	word [eax+eax*8+0x7e704c], 0x1000	; if ((card_data[esi->internal_card_id].extra_ability & EA_MANA_SOURCE)
#43447b:	74 0a				je	0x434487				;     == 0) goto 0x434487
#43447d:	c7 05 bc 04 79 00 ff ff ff ff	mov	dword [0x7904bc], -1			; tapped_for_mana_color = -1
#434487:	...
patch("Magic.exe", 0x43446b,
      "eb 10",		# jmp #x43447d
      0x90);
