#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Always sends EVENT_CAN_ACTIVATE when manually drawing mana during charge_mana(), rather than assuming tapped permanents and summoning-sick creatures can
#    never activate, and (otherwise) lands always can.
# 2. Allows cards flagged both EA_MANA_SOURCE and EA_ACT_ABILITY to be activated whenever one with just EA_MANA_SOURCE can, by essentially gutting
#    is_mana_source_but_not_act_ability().

use strict;
use warnings;
use Manalink::Patch;

#######################################################
# Skip check against tapped and summon-sick creatures #
#######################################################
# Previous contents:
#42e913:	f6 46 08 10		test	byte [esi+0x8], 0x10		; if ((instance->state & STATE_TAPPED)
#42e917:	0f 85 86 02 00 00	jne	0x42eba3			;     != 0) goto 0x42eba3
#42e91d:	f7 46 08 00 00 02 00	test	dword [esi+0x8], 0x20000	; if ((instance->state & STATE_SUMMON_SICK)
#42e924:	74 0e			je	0x42e934			;     == 0) goto 0x42e934
#42e926:	8b 46 6c		mov	eax, dword [esi+0x6c]		; eax = instance->internal_card_id
#42e929:	8b 04 85 00 70 7c 00	mov	eax, dword [eax*4+0x7c7000]	; eax = card_data_pointers[eax]
#42e930:	f6 40 28 02		test	byte [eax+0x28], 0x2		; if (((byte)(eax->type) & TYPE_CREATURE)
#42e934:	0f 85 69 02 00 00	jne	0x42eba3			;     != 0) goto 0x42eba3
#42e93a:	8b 45 08		mov	eax, dword [ebp+0x8]		; ...

# Obsolete - charge_mana() now in C
#patch("Magic.exe", 0x42e913,
#      "eb 25",	# jmp 0x42e93a
#      (0x90) x 2);

################################
# Skip check against TYPE_LAND #
################################
# Previous contents:
#42e9b9:	8b 45 f0		mov	eax, dword [ebp-0x10]		; eax = iid
#42e9bc:	c1 e0 03		shl	eax, 0x3			; eax <<= 3
#42e9bf:	8a 8c c0 38 70 7e 00	mov	cl, byte [eax+eax*8+0x7e7038]	; cl = cards_data[iid].type
#42e9c6:	f6 c1 01		test	cl, 0x1				; if ((cl & TYPE_LAND)
#42e9c9:	75 23			jne	0x42e9ee			;     != 0) goto 0x42e9ee
#42e9cb:	6a ff			...					; dispatch_event(...,CAN_ACTIVATE,...)

# Obsolete - charge_mana() now in C
#patch(0x42e9b9,
#      "eb 10",	# jmp 0x42e9cb
#      0x90);

###################################################################################################
# Allow cards flagged both mana-source and act-ability to be activated whenever a mana-source can #
###################################################################################################
# Previous contents:
#435c58:	f6 84 c0 4c 70 7e 00 01	test	byte [eax+eax*8+0x7e704c], 0x1	; if (cards_data[iid].extra_ability & EA_ACT_ABILITY
#435c60:	74 02			je	0x435c64			;     == 0) goto 0x435c64
#patch("Magic.exe", 0x435c58,
#      "eb 0a",	# jmp 0x435c64
#      (0x90) x 6);
# Revert; now handled directly in dispatch_event_to_single_card() for the problematic case, and the original version is needed when called from sub_475A30()
patch("Magic.exe", 0x435c58,
      "f6 84 c0 4c 70 7e 00 01");
