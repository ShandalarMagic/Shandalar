#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Removes the final section of untap_phase_exe(), which is now handled in untap_phase() in C.
# 2. Replace the section in resolve_top_card_on_stack() that untaps cards via paid upkeep cost (e.g. Brass Man, Colossus of Sardia) with a call into C.

use strict;
use warnings;
use Manalink::Patch;

##############################################
# Skip the last section of untap_phase_exe() #
##############################################
# Previous contents:
# 43ac27:	c7 45 ec 00 00 00 00	mov	dword [ebp-0x14], 0x0

patch("Magic.exe", 0x43ac27,
      "e9 b6 00 00 00",	# jmp 0x43ace2	; goto function epilogue
      (0x90) x 2);	# nop		; align following instruction

####################################################
# Replace untapping in resolve_top_card_on_stack() #
####################################################
# Previous contents:
# 436847:	8b 8e 10 01 00 00	mov	ecx, dword [esi+0x110]	; ecx = esi->parent_card
# 43684d:	8b 86 0c 01 00 00	mov	eax, dword [esi+0x10c]	; eax = esi->parent_controller
# 436853:	56			push	esi			; temporary = esi
# 436854:	e8 27 b2 fc ff		call	0x401a80		; esi = get_card_instance(player, card)
# 436859:	80 66 08 ef		and	BYTE [esi+0x8], 0xef	; BYTE0(esi->state) &= ~STATE_UNTAPPED
# 43685d:	5e			pop	esi			; esi = temporary
# 43685e:	6a ff			push	0xffffffff		; arg5 = -1
# 436860:	b8 01 00 00 00		mov	eax, 0x1		; eax = 1
# 436865:	2b 45 fc		sub	eax, dword [ebp-0x4]	; eax -= player
# 436868:	50			push	eax			; arg4 = eax
# 436869:	68 83 00 00 00		push	0x83			; arg3 = EVENT_UNTAP_CARD
# 43686e:	ff b6 10 01 00 00	push	dword [esi+0x110]	; arg2 = esi->parent_card
# 436874:	ff b6 0c 01 00 00	push	dword [esi+0x10c]	; arg1 = esi->parent_controller
# 43687a:	e8 d1 f2 ff ff		call	0x435b50		; dispatch_event_to_single_card(esi->parent_controller, esi->parent_card, EVENT_UNTAP_CARD, eax, 1-player)
# 43687f:	83 c4 14		add	esp, 0x14		; pop 5 arguments off stack
# 436882:	...

patch(0x436847,
      0x56,			# push	esi			; temporary = esi
      "ff b6 10 01 00 00",	# push	dword [esi+0x110]	; arg2 = esi->parent_card
      "ff b6 0c 01 00 00",	# push	dword [esi+0x10c]	; arg1 = esi->parent_controller
      "e8 14 4f bd 01",		# call	0x200b76d		; untap_card(esi->parent_controller, esi->parent_card)
      "83 c4 08",		# add	esp, 0x8		; pop 2 arguments off stack
      "5e",			# pop	esi			; esi = temporary
      "eb 23",			# jmp	0x436882		; goto 0x436882
      (0x90) x 35);		# nop				; remove the rest of the block
