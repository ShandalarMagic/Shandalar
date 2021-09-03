#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Adds a call to save_or_load_supplement() to the ends of save_or_load_ver1() and save_or_load_ver2().

use strict;
use warnings;
use Manalink::Patch;

#################################
# At end of save_or_load_ver1() #
#################################
# Previous contents:
#49f4ea:	8b 45 fc	mov	eax, dword [ebp-0x4]	; eax = v4
#49f4ed:	5f		pop	edi			; restore saved registers
#49f4ee:	5e		pop	esi
#49f4ef:	5b		pop	ebx
#49f4f0:	c9		leave				; restore saved stack
#49f4f1:	c3		ret				; return eax
#49f4f2:	cc		int3
#49f4f3:	cc		int3
#49f4f4:	cc		int3
#49f4f5:	cc		int3
#49f4f6:	cc		int3
#49f4f7:	cc		int3
#49f4f8:	cc		int3
#49f4f9:	cc		int3
patch("Magic.exe", 0x49f4ea,
      "ff 75 fc",		# push	dword [ebp-0x4]		; arg1 = v4
      "e8 40 c6 b6 01",		# call	0x200bb32		; eax = save_or_load_supplement(v4)
      "83 c4 04",		# add	esp, 4			; pop one arg off stack
      "5f",			# pop	edi			; restore saved registers
      "5e",			# pop	esi
      "5b",			# pop	ebx
      "c9",			# leave				; restore saved stack
      "c3");			# ret				; return eax

#################################
# At end of save_or_load_ver2() #
#################################
# Previous contents:
#4a02ac:	8b 45 fc	mov	eax, dword [ebp-0x4]	; eax = v4
#4a02af:	5f		pop	edi			; restore saved registers
#4a02b0:	5e		pop	esi
#4a02b1:	5b		pop	ebx
#4a02b2:	c9		leave				; restore saved stack
#4a02b3:	c3		ret				; return eax
#4a02b4:	cc		int3
#4a02b5:	cc		int3
#4a02b6:	cc		int3
#4a02b7:	cc		int3
#4a02b8:	cc		int3
#4a02b9:	cc		int3
#4a02ba:	cc		int3
#4a02bb:	cc		int3
patch(0x4a02ac,
      "ff 75 fc",		# push	dword [ebp-0x4]		; arg1 = v4
      "e8 7e b8 b6 01",		# call	0x200bb32		; eax = save_or_load_supplement(v4)
      "83 c4 04",		# add	esp, 4			; pop one arg off stack
      "5f",			# pop	edi			; restore saved registers
      "5e",			# pop	esi
      "5b",			# pop	ebx
      "c9",			# leave				; restore saved stack
      "c3");			# ret				; return eax
