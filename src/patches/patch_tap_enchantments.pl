#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Removes the check that prevents enchantments with no other permanent type from displaying as tapped.  (This also makes planeswalkers not display as tapped,
#    since they're internally enchantments.)

use strict;
use warnings;
use Manalink::Patch;

# (I need to come up with a shorter name for this function)
#############################################################################################################################################################
# in return_displayed_bitfield_of_1summonsick__2tapped_nonenchantment__4attacking__8blocking__10damage_target_player_and_damage_target_card__20oublietted() #
#############################################################################################################################################################
# Previous contents:
#438be3:	8b 45 f8		mov	eax, dword [ebp-0x8]	; eax = var8	// Previously set to cards_data[esi->internal_card_id].type
#438be6:	24 47			and	al, 0x47		; BYTE0(eax) &= TYPE_PERMANENT
#438be8:	3c 04			cmp	al, 0x4			; if (eax - TYPE_ENCHANTMENT
#438bea:	74 04			je	0x438bf0		;     == 0 goto 0x438bf0
patch("Magic.exe", 0x438be3,
      (0x90) x 9);	# nop		; just comment it out
