#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces the three checks to see whether a card is an attached aura with a call to C, so that attached equipment can be displayed the same way.
#    The called function is stdcall, so that it can squeeze into the third injection point; this is why args aren't popped after the call.
# 2. Replaces destroy_attached_auras_and_obliterate_card() with a call to the corresponding function in C.

use strict;
use warnings;

my $filename;

main();

sub seek_and_write
{
  my $file = shift;
  my $seekpos = shift;
  seek $file, $seekpos, 0 or die(sprintf "Couldn't seek to %x in $filename: %s", $seekpos, $?);
  print $file pack("C" . scalar @_, @_) or die (sprintf "Couldn't write" . (" 0x%02x" x scalar @_) . " at %x in $filename: %s", @_, $seekpos, $?);
}

sub main
{
  my $f;

  $filename = "Magic.exe";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  ###########################
  # In sub_437EC0() (first) #
  ###########################
  # Previous contents:
  # 4381ec:	8b 45 dc                mov	eax, dword [ebp-0x24]		; eax = var_0x24
  # 4381ef:	8b 04 85 00 bb 73 00    mov	eax, dword [eax*4+0x73bb00]	; eax = cards_ptr[eax]
  # 4381f6:	83 78 14 02             cmp	dword [eax+0x14], 0x2		; if (eax->card_type - 2
  # 4381fa:	0f 85 89 01 00 00       jne	0x438389			;     != 0) goto 0x438389
  # 438200:	66 81 78 18 d3 00       cmp	word [eax+0x18], 0xd3		; if (eax->subtype1 - SUBTYPE_NONE
  # 438206:	0f 84 7d 01 00 00       je	0x438389			;     == 0) goto 0x438389

  seek_and_write($f, 0x381ec,
		 0x8b, 0x45, 0xe0,			# mov	eax, dword [ebp-0x20]	; eax = card
		 0x50,					# push	eax			; arg2 = eax
		 0x8b, 0x45, 0xf0,			# mov	eax, dword [ebp-0x10]	; eax = player
		 0x50,					# push	eax			; arg1 = eax
		 0xe8, 0x04, 0x28, 0xbd, 0x01,		# call	is_displayed_attached_equipment_or_aura	; 0x200a9fd
		 0x85, 0xc0,				# test	eax, eax		; if ((eax|eax)
		 0x0f, 0x84, 0x88, 0x01, 0x00, 0x00,	# jz	0x438389		;     == 0) goto 0x438389
		 (0x90) x 11);				# nop	; (eleven of them, to overwrite replaced code)

  ############################
  # In sub_437EC0() (second) #
  ############################
  # Previous contents:
  # 43825e:	8b 45 dc		mov	eax, dword [ebp-0x24]		; eax = var_0x24
  # 438261:	8b 04 85 00 bb 73 00	mov	eax, dword [eax*4+0x73bb00]	; eax = cards_ptr[eax]
  # 438268:	83 78 14 02		cmp	dword [eax+0x14], 0x2		; if (eax->card_type - 2
  # 43826c:	75 1e			jne	0x43828c			;     != 0) goto 0x43828c
  # 43826e:	66 81 78 18 d3 00	cmp	word [eax+0x18], 0xd3		; if (eax->subtype1 - SUBTYPE_NONE
  # 438274:	74 16			je	0x43828c			;     == 0) goto 0x43828c

  seek_and_write($f, 0x3825e,
		 0x8b, 0x45, 0xd0,			# mov	eax, dword [ebp-0x30]	; eax = card
		 0x50,					# push	eax			; arg2 = eax
		 0x8b, 0x45, 0xcc,			# mov	eax, dword [ebp-0x34]	; eax = player
		 0x50,					# push	eax			; arg1 = eax
		 0xe8, 0x92, 0x27, 0xbd, 0x01,		# call	is_displayed_attached_equipment_or_aura	; 0x200a9fd
		 0x85, 0xc0,				# test	eax, eax		; if ((eax|eax)
		 0x74, 0x1d,				# jz	0x43828c		;     == 0) goto 0x438389
		 (0x90) x 7);				# nop	; (seven of them, to overwrite replaced code)

  #############################
  # In wndproc_TerritoryClass #
  #############################
  # Previous contents:
  # 49a91f:	8b 04 85 00 bb 73 00	mov	eax, dword [eax*4+0x73bb00]	; eax = cards_ptr[eax]
  # 49a926:	83 78 14 02		cmp	dword [eax+0x14], 0x2		; if (eax->card_type - 2
  # 49a92a:	0f 85 c1 00 00 00	jne	0x49a9f1			;     != 0) goto 0x49a9f1
  # 49a930:	66 81 78 18 d3 00	cmp	word [eax+0x18], 0xd3		; if (eax->subtype1 - SUBTYPE_NONE
  # 49a936:	0f 84 b5 00 00 00	je	0x49a9f1			;     == 0) goto 0x49a9f1

  seek_and_write($f, 0x9a91f,
		 0x8b, 0x85, 0x7c, 0xff, 0xff, 0xff,	# mov	eax, dword [ebp-0x84]	; eax = card
		 0x50,					# push	eax			; arg2 = eax
		 0x8b, 0x85, 0x78, 0xff, 0xff, 0xff,	# mov	eax, dword [ebp-0x88]	; eax = player
		 0x50,					# push	eax			; arg1 = eax
		 0xe8, 0xcb, 0x00, 0xb7, 0x01,		# call	is_displayed_attached_equipment_or_aura	; 0x200a9fd
		 0x85, 0xc0,				# test	eax, eax		; if ((eax|eax)
		 0x0f, 0x84, 0xb7, 0x00, 0x00, 0x00,	# jz	0x49a9f1		;     == 0) goto 0x49a9f1
		 (0x90) x 2);				# nop	; (two of them, to overwrite replaced code)

  #######################################################################################################
  # Replace entire function with an injection to 0x200b222 (destroy_attached_auras_and_obliterate_card) #
  #######################################################################################################
  # Previous contents:
  # 477d90:	55			push	ebp
  # 477d91:	8b ec			mov	ebp,esp
  # 477d93:	81 ec 70 09 00 00	sub	esp,0x970

  seek_and_write($f, 0x77d90,
		 0xe9, 0x8d, 0x34, 0xb9, 0x01,	# jmp 0x0200b222
		 (0x90) x 4);			# nop	(four, so the following code is still aligned)

  close $f or die "Couldn't close $filename: ?!";
}
