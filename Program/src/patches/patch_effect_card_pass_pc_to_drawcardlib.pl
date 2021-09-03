#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Passes player and card to DrawSmallCard when drawing effect cards, so their counters can be drawn.

use strict;
use warnings;
use Manalink::Patch;

###########################################
# In draw_smallcard_special_effect_card() #
###########################################
# Previous contents:
#4d2a2c:	6a 01			push	0x1			;arg5 = 1
#4d2a2e:	8b 45 fc		mov	eax, dword [ebp-0x4]	;eax = picnum
#4d2a31:	50			push	eax			;arg4 = eax
#4d2a32:	8d 85 64 ff ff ff	lea	eax, [ebp-0x9c]		;eax = &cp
#4d2a38:	50			push	eax			;arg3 = eax
#4d2a39:	8b 45 0c		mov	eax, dword [ebp+0xc]	;eax = dest_rect
#4d2a3c:	50			push	eax			;arg2 = eax
#4d2a3d:	8b 45 08		mov	eax, dword [ebp+0x8]	;eax = hdc
#4d2a40:	50			push	eax			;arg1 = eax
#4d2a41:	e8 3c 31 00 00		call	0x4d5b82		;DrawSmallCard(hdc, dest_rect, &cp, picnum, 1)
#4d2a46:	83 c4 14		add	esp, 0x14		;pop 5 args
#4d2a49:	8b 45 18		mov	eax, dword [ebp+0x18]	;eax = card
#4d2a4c:	50			push	eax			;arg4 = eax
#4d2a4d:	8b 45 14		mov	eax, dword [ebp+0x14]	;eax = player
#4d2a50:	50			push	eax			;arg3 = eax
#4d2a51:	8b 45 0c		mov	eax, dword [ebp+0xc]	;eax = dest_rect
#4d2a54:	50			push	eax			;arg2 = eax
#4d2a55:	8b 45 08		mov	eax, dword [ebp+0x8]	;eax = hdc
#4d2a58:	50			push	eax			;arg1 = eax
#4d2a59:	e8 a2 1b 00 00		call	0x4d4600		;draw_manastripes(hdc, dest_rect, player, card)
#4d2a5e:	83 c4 10		add	esp, 0x10		;pop 4 args
#4d2a61:	...
patch("Magic.exe", 0x4d2a2c,
      "ff 75 18",			# push	dword [ebp+0x18]	;arg7 = card
      "ff 75 14",			# push	dword [ebp+0x14]	;arg6 = player
      "6a 49",				# push	0x49			;arg5 = 73	// magic number to DrawSmallCard() knows player and card are valid
      "ff 75 fc",			# push	dword [ebp-0x4]		;arg4 = picnum
      "8d 85 64 ff ff ff",		# lea	eax,[ebp-0x9c]		;eax = &cp
      0x50,				# push	eax			;arg3 = eax
      "ff 75 0c",			# push	dword [ebp+0xc]		;arg2 = dest_Rect
      "ff 75 08",			# push	dword [ebp+0x8]		;arg1 = hdc
      "e8 39 31 00 00",			# call	0x4d5b82		;DrawSmallCard(hdc, dest_rect, &cp, picnum, 73, player, card)
      "83 c4 1c",			# add	esp, 0x1c		;pop 5 args
      #We've overrun by three bytes, so replace the call to draw_manastripes() too
      "ff 75 18",			# push	dword [ebp+0x18]	;arg4 = card
      "ff 75 14",			# push	dword [ebp+0x14]	;arg3 = player
      "ff 75 0c",			# push	dword [ebp+0xc]		;arg2 = dest_Rect
      "ff 75 08",			# push	dword [ebp+0x8]		;arg1 = hdc
      "e8 a3 1b 00 00",			# call	0x4d4600		;draw_manastripes(hdc, dest_rect, player, card)
      "83 c4 10",			# add	esp, 0x10		;pop 4 args
      0x90);				# nop				;		// since we've now underrun by one byte
