#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Fix the one instance where phase_changed() is called just before mana_burn() and is_anyone_dead() instead of after them.  Not an issue previously, but I'm
# adding an event here so Battering Ram and Desecration Demon can trigger before anything else at the start of combat, and that happens to be the one where the
# call order is reversed.

use strict;
use warnings;
use Manalink::Patch;

###################
# In main_phase() #
###################
# Previous contents:
# 43be47:	ff 35 84 f1 4e 00	push	dword [0x4ef184]	; arg2 = current_phase
# 43be4d:	ff 75 08		push	dword [ebp+0x8]		; arg1 = player
# 43be50:	e8 cb a0 ff ff		call	0x435f20		; eax = phase_changed(player, current_phase)
# 43be55:	83 c4 08		add	esp, 0x8		; pop two arguments off stack
# 43be58:	e8 03 e2 ff ff		call	0x43a060		; eax = mana_burn()
# 43be5d:	e8 4e e3 ff ff		call	0x43a1b0		; eax = is_anyone_dead()
# 43be62:	85 c0			test	eax, eax		; if (eax & eax
# 43be64:	74 0a			je	0x43be70		;     == 0) goto 0x43be70
# 43be66:	b8 01 00 00 00		mov	eax, 0x1		; eax = 1;
# 43be6b:	e9 33 10 00 00		jmp	0x43cea3		; goto 0x43cea3	// returns eax
# 43be70:	...

patch("Magic.exe", 0x43be47,
      "e8 14 e2 ff ff",			# call	0x43a060		; eax = mana_burn()
      "e8 5f e3 ff ff",			# call	0x43a1b0		; eax = is_anyone_dead()
      "85 c0",				# test	eax, eax		; if (eax & eax
      "74 0a",				# je	0x43be5f		;     == 0) goto 0x43be5f
      "b8 01 00 00 00",			# mov	eax, 0x1		; eax = 1;
      "e9 44 10 00 00",			# jmp	0x43cea3		; goto 0x43cea3	// returns eax
#43cea3:
      "ff 35 84 f1 4e 00",		# push	dword [0x4ef184]	; arg2 = current_phase
      "ff 75 08",			# push	dword [ebp+0x8]		; arg1 = player
      "e8 b3 a0 ff ff",			# call	0x435f20		; eax = phase_changed(player, current_phase)
      "83 c4 08");			# add	esp, 0x8		; pop two arguments off stack
#43be70:
