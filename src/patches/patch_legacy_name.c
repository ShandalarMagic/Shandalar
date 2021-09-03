// -*- tab-width:8 -*-
#include "patch.h"

// Updates Magic.exe in-place.
// 1. Replace the calls to strcpy and copy_string_alternative_by_pipe_hash_digit() for card titles with one to legacy_name().

int
main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  FILE* f;

  OPEN("Magic.exe");

  /***************************************************************************************************************
   * Replace call to copy_string_alternative_by_pipe_hash_digit() in get_displayable_cardname_from_player_card() *
   * (which is called, for example, to get the first line of the prompt in do_dialog())                          *
   ***************************************************************************************************************/
  /*
   * Previous contents:
   * 4397f2:	68 24 ab 56 00		push   0x56ab24			; arg2 = global_buffer_for_get_displayable_cardname_from_player_card
   * 4397f7:	8d 45 b4		lea    eax, [ebp-0x4c]		; eax = &source[0]	// a 52-byte buffer on the stack
   * 4397fa:	50			push   eax			; arg1 = eax
   * 4397fb:	e8 40 ba 09 00		call   0x4d5240			; strcpy(&source[0], global_buffer_for_get_displayable_cardname_from_player_card)
   * 439800:	83 c4 08		add    esp, 0x8			; pop two arguments
   * 439803:	8b 46 4c		mov    eax, dword [esi+0x4c]	; eax = esi->eot_toughness
   * 439806:	50			push   eax			; arg3 = eax
   * 439807:	8d 45 b4		lea    eax,[ebp-0x4c]		; eax = &source[0]
   * 43980a:	50			push   eax			; arg2 = eax
   * 43980b:	68 24 ab 56 00		push   0x56ab24			; arg1 = global_buffer_for_get_displayable_cardname_from_player_card
   * 439810:	e8 bb b6 09 00		call   0x4d4ed0			; copy_string_alternative_by_pipe_hash_digit(global_buffer_for_get_displayable_cardname_from_player_card, &source[0], esi->eot_toughness)
   * 439815:	83 c4 0c		add    esp, 0xc			; pop three arguments
   * 439818:	...
   */

#define INJ	"\xff\x75\x0c"		/* push	dword [ebp+0xc]		; arg3 = second argument to enclosing function	// card			*/\
		"\xff\x75\x08"		/* push	dword [ebp+0x8]		; arg2 = first argument to enclosing function	// player		*/\
		"\x68\x24\xab\x56\x00"	/* push	0x56ab24		; arg1 = global_buffer_for_get_displayable_cardname_from_player_card	*/\
		"\xe8\x00\x12\xbd\x01"	/* call	0x200aa02		; legacy_name(arg1, arg2, arg3)						*/\
		"\x83\xc4\x0c"		/* add	esp, 0x0c		; pop three arguments							*/\
		"\xeb\x11"		/* jmp 0x439818			; skip over the following nop's						*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90"			/* nop (seventeen, to overwrite dead code)								*/
  SEEK_AND_WRITE(f, 0x397f2);
#undef INJ

  /**********************************************************************************************************
   * Replace call to copy_string_alternative_by_pipe_hash_digit() in draw_fullcard_special_effect_cards (1) *
   **********************************************************************************************************/
  /*
   * Previous contents:
   * 4d1d9c:	68 08 8d 60 00		push	0x608d08		; arg2 = global_buffer_for_draw_fullcard_special_effect_cards
   * 4d1da1:	8d 85 1c ff ff ff	lea	eax, [ebp-0xe4]		; eax = &source[0]	// a 52-byte buffer on the stack
   * 4d1da7:	50			push	eax			; arg1 = eax
   * 4d1da8:	e8 93 34 00 00		call	0x4d5240		; strcpy(&source[0], global_buffer52_for_draw_fullcard_special_effect_cards)
   * 4d1dad:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d1db0:	8b 45 18		mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// card
   * 4d1db3:	50			push	eax			; arg2 = eax
   * 4d1db4:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// player
   * 4d1db7:	50			push	eax			; arg1 = eax
   * 4d1db8:	e8 93 70 f6 ff		call	0x438e50		; eax = get_displayed_eot_toughness(player, card)
   * 4d1dbd:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d1dc0:	50			push	eax			; arg3 = eax
   * 4d1dc1:	8d 85 1c ff ff ff	lea	eax, [ebp-0xe4]		; eax = &source[0]
   * 4d1dc7:	50			push	eax			; arg2 = eax
   * 4d1dc8:	68 08 8d 60 00		push	0x608d08		; arg1 = global_buffer52_for_draw_fullcard_special_effect_cards
   * 4d1dcd:	e8 fe 30 00 00		call	0x4d4ed0		; copy_string_alternative_by_pipe_hash_digit(global_buffer52_for_draw_fullcard_special_effect_cards, &source[0], get_displayed_eot_toughness(player, card))
   * 4d1dd2:	83 c4 0c		add	esp, 0xc		; pop three arguments
   * 4d1dd5:	...
   */

#define INJ	"\xff\x75\x18"		/* push	dword [ebp+0x18]	; arg3 = fifth argument to enclosing function	// card			*/\
		"\xff\x75\x14"		/* push	dword [ebp+0x14]	; arg2 = fourth argument to enclosing function	// player		*/\
		"\x68\x08\x8d\x60\x00"	/* push	0x608d08		; arg1 = global_buffer52_for_draw_fullcard_special_effect_cards		*/\
		"\xe8\x56\x8c\xb3\x01"	/* call	0x200aa02		; legacy_name(arg1, arg2, arg3)						*/\
		"\x83\xc4\x0c"		/* add	esp, 0x0c		; pop three arguments							*/\
		"\xeb\x24"		/* jmp 0x4d1dd5			; skip over the following nop's						*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/* nop (thirty-six, to overwrite dead code)													*/
  SEEK_AND_WRITE(f, 0xd1d9c);
#undef INJ

  /**********************************************************************************************************
   * Replace call to copy_string_alternative_by_pipe_hash_digit() in draw_fullcard_special_effect_cards (2) *
   **********************************************************************************************************/
  /*
   * Previous contents:
   * 4d2499:	68 b4 89 60 00		push	0x6089b4		; arg2 = global_buffer400_for_draw_fullcard_special_effect_cards
   * 4d249e:	8d 85 14 fd ff ff	lea	eax, [ebp-0x2ec]	; eax = &v23[0]		// a 400-byte buffer on the stack
   * 4d24a4:	50			push	eax			; arg1 = eax
   * 4d24a5:	e8 96 2d 00 00		call	0x4d5240		; strcpy(&v23[0], global_buffer400_for_draw_fullcard_special_effect_cards)
   * 4d24aa:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d24ad:	8b 45 18		mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// card
   * 4d24b0:	50			push	eax			; arg2 = eax
   * 4d24b1:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// player
   * 4d24b4:	50			push	eax			; arg1 = eax
   * 4d24b5:	e8 96 69 f6 ff		call	0x438e50		; eax = get_displayed_eot_toughness(player, card)
   * 4d24ba:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d24bd:	50			push	eax			; arg3 = eax
   * 4d24be:	8d 85 14 fd ff ff	lea	eax, [ebp-0x2ec]	; eax = &v23[0]
   * 4d24c4:	50			push	eax			; arg2 = eax
   * 4d24c5:	68 b4 89 60 00		push	0x6089b4		; arg1 = global_buffer400_for_draw_fullcard_special_effect_cards
   * 4d24ca:	e8 01 2a 00 00		call	0x4d4ed0		; copy_string_alternative_by_pipe_hash_digit(global_buffer400_for_draw_fullcard_special_effect_cards, &v23[0], get_displayed_eot_toughness(player, card))
   * 4d24cf:	83 c4 0c		add	esp, 0xc		; pop three arguments
   * 4d24d2:	...
   */

#define INJ	"\xff\x75\x18"		/* push	dword [ebp+0x18]	; arg3 = fifth argument to enclosing function	// card			*/\
		"\xff\x75\x14"		/* push	dword [ebp+0x14]	; arg2 = fourth argument to enclosing function	// player		*/\
		"\x68\xb4\x89\x60\x00"	/* push	0x6089b4		; arg1 = global_buffer400_for_draw_fullcard_special_effect_cards	*/\
		"\xe8\x59\x85\xb3\x01"	/* call	0x200aa02		; legacy_name(arg1, arg2, arg3)						*/\
		"\x83\xc4\x0c"		/* add	esp, 0x0c		; pop three arguments							*/\
		"\xeb\x24"		/* jmp 0x4d1dd5			; skip over the following nop's						*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/* nop (thirty-six, to overwrite dead code)													*/
  SEEK_AND_WRITE(f, 0xd2499);
#undef INJ

  /********************************************************************************************
   * Replace call to copy_string_alternative_by_pipe_hash_digit() in draw_small_effect_card() *
   ********************************************************************************************/
  /*
   * Previous contents:
   *
   * 4d27d4:	68 c8 87 60 00		push	0x6087c8		; arg2 = global_buffer_for_draw_small_effect_card
   * 4d27d9:	8d 85 1c ff ff ff	lea	eax, [ebp-0xe4]		; eax = &source[0]	// a 52-byte buffer on the stack
   * 4d27df:	50			push	eax			; arg1 = eax
   * 4d27e0:	e8 5b 2a 00 00		call	0x4d5240		; strcpy(&source[0], global_buffer_for_draw_small_effect_card)
   * 4d27e5:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d27e8:	8b 45 18		mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// card
   * 4d27eb:	50			push	eax			; arg2 = eax
   * 4d27ec:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// player
   * 4d27ef:	50			push	eax			; arg1 = eax
   * 4d27f0:	e8 5b 66 f6 ff		call	0x438e50		; eax = get_displayed_eot_toughness(player, card)
   * 4d27f5:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d27f8:	50			push	eax			; arg3 = eax
   * 4d27f9:	8d 85 1c ff ff ff	lea	eax, [ebp-0xe4]		; eax = &source[0]
   * 4d27ff:	50			push	eax			; arg2 = eax
   * 4d2800:	68 c8 87 60 00		push	0x6087c8		; arg1 = global_buffer_for_draw_small_effect_card
   * 4d2805:	e8 c6 26 00 00		call	0x4d4ed0		; copy_string_alternative_by_pipe_hash_digit(global_buffer_for_draw_small_effect_card, &source[0], get_displayed_eot_toughness(player, card))
   * 4d280a:	83 c4 0c		add	esp, 0xc		; pop three arguments
   * 4d280d:	...
   */

#define INJ	"\xff\x75\x18"		/* push	dword [ebp+0x18]	; arg3 = fifth argument to enclosing function	// card			*/\
		"\xff\x75\x14"		/* push	dword [ebp+0x14]	; arg2 = fourth argument to enclosing function	// player		*/\
		"\x68\xc8\x87\x60\x00"	/* push	0x6087c8		; arg1 = global_buffer_for_draw_small_effect_card			*/\
		"\xe8\x1e\x82\xb3\x01"	/* call	0x200aa02		; legacy_name(arg1, arg2, arg3)						*/\
		"\x83\xc4\x0c"		/* add	esp, 0x0c		; pop three arguments							*/\
		"\xeb\x24"		/* jmp 0x4d280d			; skip over the following nop's						*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/*													*/\
		"\x90\x90\x90\x90"	/* nop (thirty-six, to overwrite dead code)													*/
  SEEK_AND_WRITE(f, 0xd27d4);
#undef INJ

  CLOSE();

  SUCCESS("patch_legacy_name");
}

