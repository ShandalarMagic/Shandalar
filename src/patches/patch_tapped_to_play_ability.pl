#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. Adds an extra EVENT_TAPPED_TO_PLAY_ABILITY to activate().  Normally, activate() dispatches either EVENT_PLAY_ABILITY or EVENT_TAP_CARD if its prior
# dispatch of EVENT_ACTIVATE is successful and uncancelled, depending on whether the activating permanent became tapped; unfortunately, several other things
# dispatch EVENT_TAP_CARD as well, so it's nontrivial to indicate that "this EVENT_TAP_CARD was because an ability was activated".  Just always dispatching
# EVENT_PLAY_ABILITY (and sometimes EVENT_TAP_CARD in addition) won't work because there's a number of cards that trigger on "Whenever X activates an ability
# or becomes tapped".  Adding an extra event is the safest route; and it's sufficient and somewhat easier to just add one when EVENT_TAP_CARD is being sent than
# for both.
#
# The surrounding code looks something like
#
# if (!was_cancelled)
#   {
#     if (was_already_tapped || !(instance->state & STATE_TAPPED))
#       dispatch_event(player, card, EVENT_PLAY_ABILITY);
#     else
#       dispatch_event(player, card, EVENT_TAP_CARD);
#     // etc.

use strict;
use warnings;
use Manalink::Patch;

#########################################################################################
# Injects into the EVENT_TAP_CARD branch of activate() to dispatch an additional event. #
#########################################################################################
# Previous contents:
# 43460d:	68 81 00 00 00	push	0x81			; arg3 = EVENT_TAP_CARD
# 434612:	8b 45 10	mov	eax, dword [ebp+0x10]	; eax = card
# 434615:	50		push	eax			; arg2 = card
# 434616:	8b 45 0c	mov	eax, dword [ebp+0xc]	; eax = player
# 434619:	50		push	eax			; arg1 = player
# 43461a:	e8 91 13 00 00	call	0x4359b0		; dispatch_event(player, card, EVENT_TAP_CARD)
# 43461f:	83 c4 0c	add	esp, 0xc		; pop three arguments off stack
# 434622:	eb 15		jmp	0x434639		; skip the EVENT_PLAY_ABILITY branch.

patch("Magic.exe", 0x43460d,
      "e9 90 ea fc ff");	# jmp 0x4030a2

patch(0x4030a2,	# unused space within what was previously card_generic_legend()
      "68 81 00 00 00",		# push	0x81			; arg3 = EVENT_TAP_CARD
      "ff 75 10",		# push	dword [ebp+0x10]	; arg2 = card
      "ff 75 0c",		# push	dword [ebp+0xc]		; arg1 = player
      "e8 fe 28 03 00",		# call	0x4359b0		; dispatch_event(player, card, EVENT_TAP_CARD)
      "83 c4 0c",		# add	esp, 0xc		; pop three arguments off stack
      "68 00 0c 00 00",		# push	0xc00			; arg3 = EVENT_TAPPED_TO_PLAY_ABILITY
      "ff 75 10",		# push	dword [ebp+0x10]	; arg2 = card
      "ff 75 0c",		# push	dword [ebp+0xc]		; arg1 = player
      "e8 eb 28 03 00",		# call	0x4359b0		; dispatch_event(player, card, EVENT_TAPPED_TO_PLAY_ABILITY)
      "83 c4 0c",		# add	esp, 0xc		; pop three arguments off stack
      "e9 6c 15 03 00");	# jmp	0x434639		; goto whence we came
#last used: 0x4030cc
