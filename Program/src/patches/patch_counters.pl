#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Move draw_special_counters() into drawcardlib.dll, through the admittedly hacky (but safer than a new import) mechanism of giving extra paramets to
#    DrawCardBack().
# 2. Move get_special_counter_rect() into drawcardlib.dll, through the same mechanism.
# 3. Move draw_standard_counters() into drawcardlib.dll, and, instead of calling a separate function to get the number of each type of counters, give it a
#    player/card pair as arguments so we can change the semantics of counters2, counters3, counters4, and counters5.

use strict;
use warnings;
use Manalink::Patch;

#####################################################
# Move draw_special_counters() into drawcardlib.dll #
#####################################################
# Previous contents:
# 4d3a00:	55		push	ebp			; save stack
# 4d3a01:	8b ec		mov	ebp, esp		; new stack
# 4d3a03:	83 ec 64	sub	esp, 0x64		; 100 bytes of locals
# 4d3a06:	53		push	ebx			; save ebx
# 4d3a07:	56		push	esi			; save esi
# 4d3a08:	57		push	edi			; save esi
# 4d3a09:	83 7d 08 00	cmp	dword [ebp+0x8], 0x0	; if (hdc - 0
# 4d3a0d:	74 06		je	0x4d3a15		;     == 0) goto 0x4d3a15;	// return 0
# 4d3a0f:	83 7d 0c 00	cmp	dword [ebp+0xc], 0x0	; if (rect - 0
# 4d3a13:	75 07		jne	0x4d3a1c		;     != 0) goto 0x4d3a1c;
# 4d3a15:	33 c0		xor	eax, eax		; eax = 0;			// rval = 0
# 4d3a17:	e9 35 01 00 00	jmp	0x4d3b51		; goto 0x4d3b51;			// return
# 4d3a1c:	83 7d 10 00	cmp	dword [ebp+0x10], 0x0	; if (counter_type - 0
# 4d3a20:	7c 06		jl	0x4d3a28		;     < 0) goto 0x4d3a28;	// return 0
# 4d3a22:	...

#patch("Magic.exe", 0x4d3a00,
#      0x55,			# push	ebp			; save stack
#      "89 e5",			# mov	ebp, esp		; new stack
#      "ff 75 14",		# push	dword [ebp+0x14]	; arg7 = num
#      "ff 75 10",		# push	dword [ebp+0x10]	; arg6 = counter_type
#      "ff 75 0c",		# push	dword [ebp+0x0c]	; arg5 = rect
#      "ff 75 08",		# push	dword [ebp+0x08]	; arg4 = hdc
#      "68 01 0a 26 7d",		# push	0x7d260a01		; arg3 = magic number to call drawcardlib.dll:draw_special_counters()
#      "6a ff",			# push	0xffffffff		; arg2 = magic number not to draw card back
#      "6a ff",			# push	0xffffffff		; arg1 = magic number not to draw card back
#      "e8 77 21 00 00",		# call	0x4d5b94		; DrawCardBack(...)
#      "83 c4 1c",		# add	esp, 0x1c		; pop 7 args off stack
#      "c9",			# leave
#      "c3");			# ret

# Withdrawn
patch("Magic.exe", 0x4d3a00,
      0x55,
      "8b ec",
      "83 ec 64",
      0x53,
      0x56,
      0x57,
      "83 7d 08 00",
      "74 06",
      "83 7d 0c 00",
      "75 07",
      "33 c0",
      "e9 35 01 00 00",
      "83 7d 10 00",
      "7c 06");

############################################################################################################################################
# Replace calls to get_counter_type_by_id() and draw_special_counters() with call into drawcardlib.dll, and give it a player/card directly #
############################################################################################################################################
#4d3185:	50		push	eax			; arg4 = eax
#4d3186:	8b 45 fc	mov	eax, dword [ebp-0x4]	;   eax = csvid
#4d3189:	50		push	eax			;   arg1 = eax
#4d318a:	e8 11 1b 00 00	call	0x4d4ca0		;   eax = get_counter_type_by_id(...)
#4d318f:	83 c4 04	add	esp, 0x4		;   pop one arg off stack
#4d3192:	50		push	eax			; arg3 = eax
#4d3193:	8b 45 0c	mov	eax, dword [ebp+0xc]	; eax = card_rect
#4d3196:	50		push	eax			; arg2 = eax
#4d3197:	8b 45 08	mov	eax, dword [ebp+0x8]	; eax = hdc
#4d319a:	50		push	eax			; arg1 = eax
#4d319b:	e8 60 08 00 00	call	0x4d3a00		; draw_special_counters(...)
#4d31a0:	83 c4 10	add	esp,0x10
patch("Magic.exe", 0x4d3185,
      "ff 75 14",		# push	dword [ebp+0x14]	; arg7 = card
      "ff 75 10",		# push	dword [ebp+0x10]	; arg6 = player
      "ff 75 0c",		# push	dword [ebp+0xc]		; arg5 = card_rect
      "ff 75 08",		# push	dword [ebp+0x8]		; arg4 = hdc
      "68 01 0a 26 7d",		# push	0x7d260a01		; arg3 = magic number to call drawcardlib.dll:get_special_counter_rect()
      "6a ff",			# push	0xffffffff		; arg2 = magic number not to draw card back
      "6a ff",			# push	0xffffffff		; arg1 = magic number not to draw card back
      "e8 f5 29 00 00",		# call	0x4d5b94		; DrawCardBack(...)
      "83 c4 1c",		# add	esp, 0x1c		; pop 7 args off stack
      0x90);

########################################################
# Move get_special_counter_rect() into drawcardlib.dll #
########################################################
# Previous contents:
# 4d3b60:	55		push	ebp			; save stack
# 4d3b61:	8b ec		mov	ebp, esp		; new stack
# 4d3b63:	83 ec 54	sub	esp, 0x54		; 84 bytes of locals
# 4d3b66:	53		push	ebx			; save ebx
# 4d3b67:	56		push	esi			; save esi
# 4d3b68:	57		push	edi			; save edi
# 4d3b69:	83 7d 08 00	cmp	dword [ebp+0x8], 0x0	; if (rect_dest - 0
# 4d3b6d:	75 05		jne	0x4d3b74		;     != 0) goto 0x4d3b74
# 4d3b6f:	e9 44 01 00 00	jmp	0x4d3cb8		; goto 0x4d3cb8			// return
# 4d3b74:	83 7d 0c 00	cmp	dword [ebp+0xc], 0x0	; if (rect_src - 0
# 4d3b78:	75 16		jne	0x4d3b90		;     != 0) goto 0x4d3b90
# 4d3b7a:	6a 00		push	0x0			; arg5 = 0
# 4d3b7c:	6a 00		push	0x0			; arg4 = 0
# 4d3b7e:	6a 00		push	0x0			; arg3 = 0	// and eventually call SetRect(rect_dest, 0, 0, 0, 0) and return
# 4d3b80:	...
patch(0x4d3b60,
      0x55,			# push	ebp			; save stack
      "89 e5",			# mov	ebp, esp		; new stack
      "ff 75 10",		# push	dword [ebp+0x10]	; arg6 = num
      "ff 75 0c",		# push	dword [ebp+0xc]		; arg5 = rect_src
      "ff 75 08",		# push	dword [ebp+0x8]		; arg4 = rect_dest
      "68 03 0a 26 7d",		# push	0x7d260a03		; arg3 = magic number to call drawcardlib.dll:get_special_counter_rect()
      "6a ff",			# push	0xffffffff		; arg2 = magic number not to draw card back
      "6a ff",			# push	0xffffffff		; arg1 = magic number not to draw card back
      "e8 1a 20 00 00",		# call	0x4d5b94		; DrawCardBack(...)
      "83 c4 18",		# add	esp, 0x18		; pop 6 args off stack
      "c9",			# leave
      "c3",			# ret
      0x90);			# nop

#########################################################################################
# Move draw_standard_counters() into drawcardlib.dll and give it a player/card directly #
#########################################################################################
#4d31fa:	8d 85 28 ff ff ff	# lea	eax, [ebp-0xd8]		; eax = &counters5
#4d3200:	50			# push	eax			; arg7 = eax
#4d3201:	8d 85 20 ff ff ff	# lea	eax, [ebp-0xe0]		; eax = &counters
#4d3207:	50			# push	eax			; arg6 = eax
#4d3208:	8d 85 24 ff ff ff	# lea	eax, [ebp-0xdc]		; eax = &counters4
#4d320e:	50			# push	eax			; arg5 = eax
#4d320f:	8d 85 2c ff ff ff	# lea	eax, [ebp-0xd4]		; eax = &counters3
#4d3215:	50			# push	eax			; arg4 = eax
#4d3216:	8d 85 30 ff ff ff	# lea	eax, [ebp-0xd0]		; eax = &counters2
#4d321c:	50			# push	eax			; arg3 = eax
#4d321d:	8b 45 14		# mov	eax, dword [ebp+0x14]	; eax = card
#4d3220:	50			# push	eax			; arg2 = eax
#4d3221:	8b 45 10		# mov	eax, dword [ebp+0x10]	; eax = player
#4d3224:	50			# push	eax			; arg1 = player
#4d3225:	e8 d6 57 f6 ff		# call	0x438a00		; get_displayed_standard_counters_counts(player, card, &counters2, &counters3, &counters4, &counters, &counters5);
#4d322a:	83 c4 1c		# add	esp, 0x1c		; pop 7 arguments

#4d322d:	8b 85 28 ff ff ff	# mov	eax, dword [ebp-0xd8]	; eax = counters5
#4d3233:	50			# push	eax			; arg7 = eax
#4d3234:	8b 85 20 ff ff ff	# mov	eax, dword [ebp-0xe0]	; eax = counters
#4d323a:	50			# push	eax			; arg6 = eax
#4d323b:	8b 85 24 ff ff ff	# mov	eax, dword [ebp-0xdc]	; eax = counters4
#4d3241:	50			# push	eax			; arg5 = eax
#4d3242:	8b 85 2c ff ff ff	# mov	eax, dword [ebp-0xd4]	; eax = counters3
#4d3248:	50			# push	eax			; arg4 = eax
#4d3249:	8b 85 30 ff ff ff	# mov	eax, dword [ebp-0xd0]	; eax = counters2
#4d324f:	50			# push	eax			; arg3 = eax
#4d3250:	8d 85 38 ff ff ff	# lea	eax, [ebp-0xc8]		; eax = &rect_counter
#4d3256:	50			# push	eax			; arg2 = eax
#4d3257:	8b 45 08		# mov	eax, dword [ebp+0x8]	; eax = hdc
#4d325a:	50			# push	eax			; arg1 = eax
#4d325b:	e8 60 0a 00 00		# call	0x4d3cc0		; draw_standard_counters(hdc, &rect_counter, counter2, counters3, counters4, counters, counters5);
#4d3260:	83 c4 1c		# add	esp, 0x1c		; pop 7 arguments
#4d3263:	...
patch(0x4d31fa,
      "ff 75 14",			# push	dword [ebp+0x14]	; arg7 = card
      "ff 75 10",			# push	dword [ebp+0x10]	; arg6 = player
      "8d 85 38 ff ff ff",		# lea	eax, [ebp-0xc8]		; eax = &rect_counter
      0x50,				# push	eax			; arg5 = eax
      "ff 75 08",			# push	dword [ebp+0x8]		; arg4 = hdc
      "68 02 0a 26 7d",			# push	0x7d260a02		; arg3 = magic number to call drawcardlib.dll:draw_standard_counters()
      "6a ff",				# push	0xffffffff		; arg2 = magic number not to draw card back
      "6a ff",				# push	0xffffffff		; arg1 = magic number not to draw card back
      "e8 7c 29 00 00",			# call	0x4d5b94		; DrawCardBack(...)
      "83 c4 1c",			# add	esp, 0x1c		; pop 7 args off stack
      "eb 46",				# jmp	0x4d3263		; skip all the nop's
      (0x90) x 70);			# nop				; overwrite the rest of these two function calls
