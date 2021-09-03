#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. In the two instances where DrawFullCard and and the one instance where DrawSmallCard is called for a non-effect card in play, replaces the color field of
#    the card_ptr_t that's built on the stack with a value encoding the player and the card number, so the drawing functions can get at its card_instance_t.
# 2. Fixes the image version number for smallcards of activation cards to match their parent.

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

  #######################################################################################
  # Replace card_ptr_t::color for DrawFullCard call in _DRAW_FULL_CARD_NORMAL/*4D1780*/ #
  #######################################################################################
  # Previous contents:
  # 4d17e3:	8b 45 18		mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// the card number
  # 4d17e6:	50			push	eax			; arg2 = eax
  # 4d17e7:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the player number
  # 4d17ea:	50			push	eax			; arg1 = eax
  # 4d17eb:	e8 d0 75 f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
  # 4d17f0:	83 c4 08		add	esp, 0x8		; pop two arguments
  # 4d17f3:	89 85 58 ff ff ff	mov	dword [ebp-0xa8], eax	; local var at -0xa8 = eax
  # 4d17f9:	c7 85 54 ff ff ff 00	mov	dword [ebp-0xac], 0x0	; local var at -0xac = 0
  # 4d1800:	00 00 00
  # 4d1803:	c7 85 5c ff ff ff 01	mov	dword [ebp-0xa4],0x1	; local var at -0xa4 = 1
  # 4d180a:	00 00 00

  seek_and_write($f, 0xd17e3,
		 0xb9, 0xd4, 0xc3, 0x00, 0x00,		# mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll
		 0x8b, 0x45, 0x14,			# mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the player number
		 0x85, 0xc0,				# test	eax, eax		; if (eax
		 0x74, 0x03,				# je	0x4d17f2		;         == 0) goto 0x4d17f2;
		 0x83, 0xc9, 0x01,			# or	ecx, 0x1		; ecx |= 0x1			// low bit: player number
#4d17f2:
		 0x8b, 0x45, 0x18,			# mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// the card number
		 0xc1, 0xe0, 0x10,			# shl	eax, 0x10		; eax <<= 16
		 0x09, 0xc1,				# or	ecx, eax		; ecx |= eax			// hi word: the card number
		 0x89, 0x8d, 0x70, 0xff, 0xff, 0xff,	# mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack
		 0xe9, 0xb2, 0x00, 0x00, 0x00,		# jmp	0x4d18b7		; goto 0x4d18b7	// skip the rest of the code dumbing down color
		 0x90, 0x90, 0x90, 0x90,
		 0x90, 0x90, 0x90, 0x90);		# nop	(eight, so the following code is still aligned)

  #############################################################################################
  # Replace card_ptr_t::color for DrawFullCard call in _DRAW_FULL_CARD_EFFECT_CARDS/*4D1950*/ #
  #############################################################################################
  # Previous contents:
  # 4d1ab9:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
  # 4d1abc:	50			push	eax			; arg2 = eax
  # 4d1abd:	8b 45 10		mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
  # 4d1ac0:	50			push	eax			; arg1 = eax
  # 4d1ac1:	e8 fa 72 f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
  # 4d1ac6:	83 c4 08		add	esp, 0x8		; pop two arguments
  # 4d1ac9:	89 85 58 ff ff ff	mov	dword [ebp-0xa8], eax	; local var at -0xa8 = eax
  # 4d1acf:	c7 85 54 ff ff ff 00	mov	dword [ebp-0xac], 0x0	; local var at -0xac = 0
  # 4d1ad6:	00 00 00
  # 4d1ad9:	c7 85 5c ff ff ff 01	mov	dword [ebp-0xa4], 0x1	; local var at -0xa4 = 1
  # 4d1ae0:	00 00 00

  seek_and_write($f, 0xd1ab9,
		 0xb9, 0xd4, 0xc3, 0x00, 0x00,		# mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll
		 0x8b, 0x45, 0x10,			# mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
		 0x85, 0xc0,				# test	eax, eax		; if (eax
		 0x74, 0x03,				# je	0x4d1ac8		;         == 0) goto 0x4d1ac8
		 0x83, 0xc9, 0x01,			# or	ecx, 0x1		; ecx |= 0x1			// low bit: the player number
#4d1ac8:
		 0x8b, 0x45, 0x14,			# mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
		 0xc1, 0xe0, 0x10,			# shl	eax, 0x10		; eax <<= 16
		 0x09, 0xc1,				# or	ecx, eax		; ecx |= eax			// hi word: the card number
		 0x89, 0x8d, 0x70, 0xff, 0xff, 0xff,	# mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack
		 0xe9, 0xb2, 0x00, 0x00, 0x00,		# jmp	0x4d1b8d		; goto 0x4d1b8d	// skip the rest of the code dumbing down color
		 0x90, 0x90, 0x90, 0x90,
		 0x90, 0x90, 0x90, 0x90);		# nop	(eight, so the following code is still aligned)

  ##################################################################################
  # Replace card_ptr_t::color for DrawSmallCard call in _DRAW_SMALL_CARD/*4D2D80*/ #
  ##################################################################################
  # Previous contents:
  # 4d2de8:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
  # 4d2deb:	50			push	eax			; arg2 = eax
  # 4d2dec:	8b 45 10		mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
  # 4d2def:	50			push	eax			; arg1 = eax
  # 4d2df0:	e8 cb 5f f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
  # 4d2df5:	83 c4 08		add	esp, 0x8		; pop two arguments
  # 4d2df8:	89 85 54 ff ff ff	mov	dword [ebp-0xac], eax	; local var at -0xac = eax
  # 4d2dfe:	c7 85 50 ff ff ff 00	mov	dword [ebp-0xb0], 0x0	; local var at -0xb0
  # 4d2e05:	00 00 00
  # 4d2e08:	c7 85 58 ff ff ff 01	mov	dword [ebp-0xa8], 0x1	; local var at -0xa8 = 1
  # 4d2e0f:	00 00 00

  seek_and_write($f, 0xd2de8,
		 0xb9, 0xd4, 0xc3, 0x00, 0x00,		# mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll
		 0x8b, 0x45, 0x10,			# mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
		 0x85, 0xc0,				# test	eax, eax		; if (eax
		 0x74, 0x03,				# je	0x4d2df7		;         == 0) goto 0x4d2df7
		 0x83, 0xc9, 0x01,			# or	ecx, 0x1		; ecx |= 0x1			// low bit: the player number
#4d2df7:
		 0x8b, 0x45, 0x14,			# mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
		 0xc1, 0xe0, 0x10,			# shl	eax, 0x10		; eax <<= 16
		 0x09, 0xc1,				# or	ecx, eax		; ecx |= eax			// hi word: the card number
		 0x89, 0x8d, 0x70, 0xff, 0xff, 0xff,	# mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack
		 0xe9, 0xb2, 0x00, 0x00, 0x00,		# jmp	0x4d2ebc		; goto 0x4d2ebc	// skip the rest of the code dumbing down color
		 0x90, 0x90, 0x90, 0x90,
		 0x90, 0x90, 0x90, 0x90);		# nop	(eight, so the following code is still aligned)

  ########################################################################
  # Fix image version number in draw_smallcard_activation_card/*4D2B30*/ #
  ########################################################################
  # Previous contents:
  #4d2bad:	e8 de 60 f6 ff				# call	0x438c90		; get_displayed_pic_num_and_pic_int_id_of_damage_source_playercard(&ret_tgt_ignored, player, card);

  seek_and_write($f, 0xd2bad,
		 0xe8, 0x6e, 0xd6, 0xd9, 0x00);		# call 0x1270220		; get_displayed_pic_num_and_pic_int_id_of_parent_playercard(&ret_tgt_ignored, player, card);

  close $f or die "Couldn't close $filename: ?!";


  $filename = "ManalinkEx.dll";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  ############################################################################
  # New function get_displayed_pic_num_and_pic_int_id_of_parent_playercard() #
  ############################################################################
  seek_and_write($f, 0x26f620,	#0x1270220
		 0x55,					# push	ebp			; save stack
		 0x89, 0xe5,				# mov	ebp, esp		; new stack
		 0x83, 0xec, 0x04,			# sub	esp, 0x4		; 4 bytes of local variables
		 0x56,					# push	esi			; save esi

		 0xff, 0x75, 0x10,			# push	dword [ebp+0x10]	; arg2 = card
		 0xff, 0x75, 0x0c,			# push	dword [ebp+0xc]		; arg1 = player
		 0xe8, 0x4e, 0x87, 0x1c, 0xff,		# call	438980			; eax = player_gt_1_or_card_ge_150(player, card)
		 0x83, 0xc4, 0x08,			# add	esp, 0x8		; pop 2 args

		 0x85, 0xc0,				# test	eax, eax		; if (eax
		 0x74, 0x0f,				# je	ok			;     == 0 goto ok

		 0x0d, 0xff, 0xff, 0xff, 0xff,		# or	eax, 0xffffffff		; eax = -1
		 0x8b, 0x4d, 0x08,			# mov	ecx, dword [ebp+0x8]	; ecx = (target_t*)ret_tgt
		 0x89, 0x01,				# mov	dword [ecx], eax	; ret_tgt->player = -1
		 0x89, 0x41, 0x04,			# mov	dword [ecx+0x4], eax	; ret_tgt->card = -1
		 0xeb, 0x51,				# jmp	epilog			; goto epilog
#ok:
		 0x8b, 0x45, 0x0c,			# mov	eax, dword [ebp+0xc]	; eax = card
		 0x8b, 0x4d, 0x10,			# mov	ecx, dword [ebp+0x10]	; ecx = player
		 0xe8, 0x5d, 0x18, 0x19, 0xff,		# call	401ab0			; esi = get_displayed_card_instance(player, card)

		 0x8b, 0x96, 0x10, 0x01, 0x00, 0x00,	# mov	edx, dword [esi+0x110]	; edx = esi->parent_card
		 0x0f, 0xbe, 0x86, 0x0c, 0x01, 0x00, 0,	# movsx	eax, byte [esi+0x10c]	; eax = (int)esi->parent_controller
		 0x8b, 0x4d, 0x08,			# mov	ecx, dword [ebp+0x8]	; ecx = (target_t*)ret_tgt
		 0x89, 0x01,				# mov	dword [ecx], eax	; ret_tgt->player = eax
		 0x89, 0x51, 0x04,			# mov	dword [ecx+0x4], edx	; ret_tgt->card = edx

		 0x8b, 0x46, 0x64,			# mov	eax, dword [esi+0x64]	; eax = esi->display_pic_int_id
		 0x66, 0x89, 0x45, 0xfc,		# mov	word [ebp-0x4], ax	; local_0x4 = eax

		 0xff, 0xb6, 0x10, 0x01, 0x00, 0x00,	# push	dword [esi+0x110]	; arg3 = esi->parent_card
		 0xff, 0xb6, 0x0c, 0x01, 0x00, 0x00,	# push	dword [esi+0x10c]	; arg2 = esi->parent_controller

		 # eax = cards_at_7c7000[esi->original_internal_card_id]->id
		 0x8b, 0x46, 0x3c,			# mov	eax, dword [esi+0x3c]		; eax = esi->original_internal_card_id
		 0x8b, 0x04, 0x85, 0x00, 0x70, 0x7c, 0,	# mov	eax, dword [eax*4+0x7c7000]	; eax = cards_at_7c7000[eax]
		 0x0f, 0xb7, 0x40, 0x24,		# movzx	eax, byte [eax+0x24]		; eax = eax->type

		 0x50,					# push	eax			; arg1 = cards_at_7c7000[esi->original_internal_card_id]->id
		 0xe8, 0xd1, 0x7e, 0x1f, 0xff,		# call	468160			; eax = get_card_image_number(cards_at_7c7000[esi->original_internal_card_id]->id, esi->parent_controller, esi->parent_card)
		 0x83, 0xc4, 0x0c,			# add	esp, 0xc		; pop 3 args

		 0xc1, 0xe0, 0x10,			# shl	eax, 0x10		; eax <<= 16
		 0x66, 0x8b, 0x45, 0xfc,		# mov	ax, word [ebp-0x4]	; LOWORD(eax) = local_0x4
#epilog:
		 0x5e,					# pop	esi			; restore esi
		 0xc9,					# leave				; restore stack
		 0xc3);					# ret				; return

  close $f or die "Couldn't close $filename: ?!";
}
