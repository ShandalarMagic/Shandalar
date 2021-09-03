#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Hooks the call to wndproc_LifeClass to call wndproc_LifeClass_hook() in C.
# 2. Replaces the end of start_duel() inspecting the full width of player_counters[] with a call to determine_start_duel_winner() in C (which inspects only the
#    low-order byte).
# 3. Replaces the inline painting of poison counters in wndproc_LifeClass::WM_PAINT to call paint_player_counters() in C.  This is significantly easier than
#    replacing the entire handler.
# 4. Replaces is_anyone_dead() with its C version.
# And, happily, all other references to poison_counters[] in Magic.exe are either already placed in ManalinkEh.dll or don't care about the actual value, just
# where to copy it.

use strict;
use warnings;
use Manalink::Patch;

###############################################################################################################
# register_LifeClass(): Replace function address of wndproc_CardClass with 200e30f (wndproc_LifeClass_hook()) #
###############################################################################################################
# Previous contents:
#49711a:	c7 45 dc 00 73 49 00	mov	dword [ebp-0x24], 0x497300	; WndClass.lpfnWndProc = wndproc_LifeClass

patch("Magic.exe", 0x49711a,
      "c7 45 dc 0f e3 00 02");		# mov	dword [ebp-0x24], 0x200e30f	; WndClass.lpfnWndProc = wndproc_LifeClass_hook;

##########################################################################
# start_duel(): Replace end with a call to determine_start_duel_winner() #
##########################################################################
# Previous contents:
#4790ae:	83 3d 28 f5 4e 00 00	cmp	dword [0x4ef528], 0x0		; if (life[0] - 0
#4790b5:	7e 22			jle	0x4790d9			;     < 0) goto 0x4790d9;
#4790b7:	83 3d 30 f5 4e 00 0a	cmp	dword [0x4ef530], 0xa		; if (poison_couners[0] - 10

patch(0x4790ae,
      "e8 63 c4 b8 01",	# call 0x2005516	; eax = determine_start_duel_winner()
      "e9 4e 00 00 00",	# jmp 0x479106		; goto function epilogue
      (0x90) x 6);	# nop

##############################################################################################################
# wndproc_LifeClass WM_PAINT handler: call paint_player_counters() instead of drawing poison counters inline #
##############################################################################################################
# Previous contents:
#4979ff:	8b 85 e4 fd ff ff	mov	eax, dword [ebp-0x21c]	; eax = Rect.right
#497a05:	99			cdq				; (fix truncation for the later division)
#497a06:	83 e2 03		and	edx, 0x3		; (still fixing truncation)
#497a09:	03 c2			add	eax, edx		;  (still fixing truncation)
#497a0b:	c1 f8 02		sar	eax, 0x2		; eax /= 8
#497a0e:	89 85 a0 fd ff ff	mov	dword [ebp-0x260],eax	; v19 = eax
#497a14:	b9 03 00 00 00		mov	ecx, 0x3		; ecx = 3
#497a19:	8b 85 e8 fd ff ff	mov	eax, dword [ebp-0x218]	; eax = Rect.bottom
#497a1f:	99			cdq    				; edx:eax = eax (with sign extension)
#497a20:	f7 f9			idiv	ecx			; eax /= 3 (signed)
#497a22:	89 85 5c fd ff ff	mov	dword [ebp-0x2a4],eax	; v17 = eax

patch(0x4979ff,
      "ff b5 b0 fd ff ff",	# push	dword [ebp-0x250]	; arg4 = my_player_counters
      "ff b5 e8 fd ff ff",	# push	dword [ebp-0x218]	; arg3 = Rect.bottom
      "ff b5 e4 fd ff ff",	# push	dword [ebp-0x21c]	; arg2 = Rect.right
      "ff b5 58 fd ff ff",	# push	dword [ebp-0x2a8]	; arg1 = hdc
      "e8 f8 68 b7 01",		# call	0x200e314		; eax = paint_player_counters(hdc, Rect.right, Rect.bottom, my_player_counters)
      "83 c4 10",		# add	esp, 0x10		; pop 4 arguments off stack
      "e9 c7 00 00 00",		# jmp	0x497aeb		; skip end of 
      (0x90) x 4);		# nop

##############################################
# Replace is_anyone_dead with its C version. #
##############################################
# Previous contents:
#43a1b0:	55		# push	ebp
#43a1b1:	8b ec		# mov	ebp, esp
#43a1b3:	83 ec 08	# sub	esp, 0x8
patch(0x43a1b0,
      "e9 64 41 bd 01",		# jmp 0x200e319
      0x90);

