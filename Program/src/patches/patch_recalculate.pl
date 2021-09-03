#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. Replaces recalculate_all_cards_in_play() with a call to the corresponding function in C.
# 2. Injects into the EVENT_ABILITIES branch of get_abilities to clear card_instance_t::targets[16].card, state & STATE_VIGILANCE, and
#    token_status & (STATUS_WALL_CAN_ATTACK|STATUS_CANT_ATTACK).

use strict;
use warnings;
use Manalink::Patch;

########################################################################################
# Replace entire function with an injection to 200b24f (recalculate_all_cards_in_play) #
########################################################################################
# Previous contents:
# 4351c0:	55		push	ebp
# 4351c1:	8b ec		mov	ebp, esp
# 4351c3:	83 ec 10	sub	esp, 0x10

patch("Magic.exe", 0x4351c0,
      "e9 8a 60 bd 01",	# jmp 0x0200b24f
      0x90);		# nop	(so the following code is still aligned)

########################################################################################################
# Injects into the EVENT_ABILITIES branch of get_abilities to clear card_instance_t::targets[16].card. #
########################################################################################################
# Previous contents:
# 43549e:	81 66 28 ff ff ff f7	and	dword [esi+0x28], 0xf7ffffff	; instance->regen_status &= ~KEYWORD_RECALC_ABILITIES
# 4354a5:	e9 06 01 00 00		jmp	0x4355b0			; goto 0x4355b0

patch(0x43549e,
      "e9 cd ac e3 00",		# jmp 0x1270170
      (0x90) x 2);		# nop	(two, so the following code is still aligned)

patch("ManalinkEx.dll", 0x1270170,
      "81 66 28 ff ff ff f7",	# and	dword [esi+0x28], 0xf7ffffff	; instance->regen_status &= ~KEYWORD_RECALC_ABILITIES

      "83 3d 48 a5 60 00 00",	# cmp	dword [0x60a548], 0		; if (dword_60A548 - 0
      "74 2a",			# jz	dontset				;     == 0) goto dontset	// since the event won't be dispatched either

      # ; eax = cards_at_7c7000[instance->card_id]->type
      # ; (It's safe to use eax, since the first instruction after jmping back to 0x4355b0 sets it.)
      "8b 46 6c",		# mov	eax, dword [esi+0x6c]		; eax = instance->internal_card_id
      "8b 04 85 00 70 7c 00",	# mov	eax, dword [eax*4+0x7c7000]	; eax = cards_at_7c7000[eax]
      "0f b6 40 28",		# movzx	eax, byte [eax+0x28]		; eax = eax->type

      "a9 47 00 00 00",		# test	eax, 0x47			; if (eax & (TYPE_LAND|TYPE_CREATURE|TYPE_ENCHANTMENT|TYPE_ARTIFACT)
      "74 15",			# jz	dontset				;     == 0) goto dontset

      "a9 80 00 00 00",		# test	eax, 0x80			; if (eax & TYPE_EFFECT
      "75 0e",			# jnz	dontset				;     != 0) goto dontset

      "83 a6 f8 00 00 00 00",	# and	dword [esi+0xf8], 0		; instance->targets[16].card &= 0
      "81 66 08 ff df ff ff",	# and	dword [esi+0x08], 0xffffdfff	; instance->state &= ~STATE_VIGILANCE
      "81 66 18 ff 77 ff ff",	# and	dword [esi+0x18], 0xffff77ff	; instance->token_status &= ~(STATUS_WALL_CAN_ATTACK | STATUS_CANT_ATTACK)

      #dontset:
      "e9 fa 53 1c ff");	# jmp	0x4355b0			; goto 0x4355b0
