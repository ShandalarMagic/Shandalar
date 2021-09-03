#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces the six calls to find_and_replace() in draw_fullcard_special_effect_card() that parse |n values with a call to
#    effect_card_text_replace_pipe_n() in C.

use strict;
use warnings;
use Manalink::Patch;

# Previous contents:
#4d215b:	8b 85 10 ff ff ff	mov	eax, dword [ebp-0xf0]	; eax = counter_toughness
#4d2161:	50			push	eax			; arg4 = eax
#4d2162:	8b 85 a8 fe ff ff	mov	eax, dword [ebp-0x158]	; eax = counter_power
#4d2168:	50			push	eax			; arg3 = eax
#4d2169:	68 df f0 4e 00		push	0x4ef0df		; arg2 = "+%d/+%d"
#4d216e:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2174:	50			push	eax			; arg1 = eax
#4d2175:	e8 1a 3d 00 00		call	0x4d5e94		; sprintf(&Dest[0], "+%d/+%d", counter_power, counter_toughness)
#4d217a:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d217d:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2183:	50			push	eax			; arg4 = eax
#4d2184:	6a 00			push	0x0			; arg3 = 0
#4d2186:	68 e7 f0 4e 00		push	0x4ef0e7		; arg2 = "+|n/+|n"
#4d218b:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d2190:	e8 fb 4c f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "+|n/+|n", 0, &Dest[0])
#4d2195:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d2198:	8b 85 10 ff ff ff	mov	eax, dword [ebp-0xf0]	; eax = counter_toughness
#4d219e:	50			push	eax			; arg3 = eax
#4d219f:	68 ef f0 4e 00		push	0x4ef0ef		; arg2 = "+0/+%d"
#4d21a4:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d21aa:	50			push	eax			; arg1 = eax
#4d21ab:	e8 e4 3c 00 00		call	0x4d5e94		; sprintf(&Dest[0], "+0/+%d", counter_toughness)
#4d21b0:	83 c4 0c		add	esp, 0xc		; pop 3 arguments off of stack

#4d21b3:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d21b9:	50			push	eax			; arg4 = eax
#4d21ba:	6a 00			push	0x0			; arg3 = 0
#4d21bc:	68 f6 f0 4e 00		push	0x4ef0f6		; arg2 = "+0/+|n"
#4d21c1:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d21c6:	e8 c5 4c f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "+0/+|n", 0, &Dest[0])
#4d21cb:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d21ce:	8b 85 a8 fe ff ff	mov	eax, dword [ebp-0x158]	; eax = counter_power
#4d21d4:	50			push	eax			; arg3 = eax
#4d21d5:	68 fd f0 4e 00		push	0x4ef0fd		; arg2 = "+%d/+0"
#4d21da:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d21e0:	50			push	eax			; arg1 = eax
#4d21e1:	e8 ae 3c 00 00		call	0x4d5e94		; sprintf(&Dest[0], "+%d/+0", counter_power)
#4d21e6:	83 c4 0c		add	esp, 0xc		; pop 3 arguments off of stack

#4d21e9:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d21ef:	50			push	eax			; arg4 = eax
#4d21f0:	6a 00			push	0x0			; arg3 = 0
#4d21f2:	68 04 f1 4e 00		push	0x4ef104		; arg2 = "+|n/+0"
#4d21f7:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d21fc:	e8 8f 4c f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "+|n/+0", 0, &Dest[0])
#4d2201:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d2204:	8b 85 a8 fe ff ff	mov	eax, dword [ebp-0x158]	; eax = counter_power
#4d220a:	50			push	eax			; arg3 = eax
#4d220b:	68 0b f1 4e 00		push	0x4ef10b		; arg2 = "%d/-0"
#4d2210:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2216:	50			push	eax			; arg1 = eax
#4d2217:	e8 78 3c 00 00		call	0x4d5e94		; sprintf(&Dest[0], "%d/-0", counter_power)
#4d221c:	83 c4 0c		add	esp, 0xc		; pop 3 arguments off of stack

#4d221f:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2225:	50			push	eax			; arg4 = eax
#4d2226:	6a 00			push	0x0			; arg3 = 0
#4d2228:	68 11 f1 4e 00		push	0x4ef111		; arg2 = "-|n/-0"
#4d222d:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d2232:	e8 59 4c f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "-|n/-0", 0, &Dest[0])
#4d2237:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d223a:	8b 85 10 ff ff ff	mov	eax, dword [ebp-0xf0]	; eax = counter_toughness
#4d2240:	50			push	eax			; arg4 = eax
#4d2241:	8b 85 a8 fe ff ff	mov	eax, dword [ebp-0x158]	; eax = counter_power
#4d2247:	50			push	eax			; arg3 = eax
#4d2248:	68 18 f1 4e 00		push	0x4ef118		; arg2 = "+%d/%d"
#4d224d:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2253:	50			push	eax			; arg1 = eax
#4d2254:	e8 3b 3c 00 00		call	0x4d5e94		; sprintf(&Dest[0], "+%d/%d", counter_power, counter_toughness)
#4d2259:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d225c:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2262:	50			push	eax			; arg4 = eax
#4d2263:	6a 00			push	0x0			; arg3 = 0
#4d2265:	68 1f f1 4e 00		push	0x4ef11f		; arg2 = "+|n/-|n"
#4d226a:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d226f:	e8 1c 4c f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "+|n/-|n", 0, &Dest[0])
#4d2274:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d2277:	8b 85 10 ff ff ff	mov	eax, dword [ebp-0xf0]	; eax = counter_toughness
#4d227d:	50			push	eax			; arg3 = eax
#4d227e:	68 d6 b4 4e 00		push	0x4eb4d6		; arg2 = "%d"
#4d2283:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2289:	50			push	eax			; arg1 = eax
#4d228a:	e8 05 3c 00 00		call	0x4d5e94		; sprintf(&Dest[0], "%d", counter_toughness)
#4d228f:	83 c4 0c		add	esp, 0xc		; pop 3 arguments off of stack

#4d2292:	8d 85 ac fe ff ff	lea	eax, [ebp-0x154]	; eax = &Dest[0]
#4d2298:	50			push	eax			; arg4 = eax
#4d2299:	6a 00			push	0x0			; arg3 = 0
#4d229b:	68 27 f1 4e 00		push	0x4ef127		; arg2 = "|n"
#4d22a0:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
#4d22a5:	e8 e6 4b f9 ff		call	0x466e90		; find_and_replace(global_buffer400_for_drawfullcard_special_effect_cards, "|n", 0, &Dest[0])
#4d22aa:	83 c4 10		add	esp, 0x10		; pop 4 arguments off of stack

#4d22ad:	...

patch("Magic.exe", 0x4d215b,
      "8b 85 10 ff ff ff",		# mov	eax, dword [ebp-0xf0]	; eax = counter_toughness	// unchanged
      0x50,				# push	eax			; arg3 = eax			// unchanted
      "8b 85 a8 fe ff ff",		# mov	eax, dword [ebp-0x158]	; eax = counter_power		// unchanged
      0x50,				# push	eax			; arg2 = eax			// unchanged
      "68 b4 89 60 00",			# push	0x6089b4		; arg1 = global_buffer400_for_drawfullcard_special_effect_cards
      "e8 42 b7 b3 01",			# call	0x200d8b5		; effect_card_text_replace_pipe_n(global_buffer400_for_drawfullcard_special_effect_cards, counter_power, counter_toughness)
      "83 c4 0c",			# add	esp, 0xc		; pop 3 arguments off of stack
      "e9 32 01 00 00",			# jmp	0x4d22ad		; goto LABEL_urzas_avenger	// skip rest of original replacements
      "90 90");				# nop, nop
