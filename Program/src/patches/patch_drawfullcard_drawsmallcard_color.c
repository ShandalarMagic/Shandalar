// -*- tab-width:8 -*-
#include "patch.h"

// Updates Magic.exe in-place.
// 1. In the two instances where DrawFullCard and and the one instance where DrawSmallCard is called for a non-effect card in play, replaces the color field of
//    the card_ptr_t that's built on the stack with a value encoding the player and the card number, so the drawing functions can get at its card_instance_t.

int
main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  FILE* f;

  OPEN("Magic.exe");

  /*************************************************************************************
  * Replace card_ptr_t::color for DrawFullCard call in _DRAW_FULL_CARD_NORMAL (4D1780) *
  *************************************************************************************/
  /*
   * Previous contents:
   * 4d17e3:	8b 45 18		mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// the card number
   * 4d17e6:	50			push	eax			; arg2 = eax
   * 4d17e7:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the player number
   * 4d17ea:	50			push	eax			; arg1 = eax
   * 4d17eb:	e8 d0 75 f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
   * 4d17f0:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d17f3:	89 85 58 ff ff ff	mov	dword [ebp-0xa8], eax	; local var at -0xa8 = eax
   * 4d17f9:	c7 85 54 ff ff ff 00	mov	dword [ebp-0xac], 0x0	; local var at -0xac = 0
   * 4d1800:	00 00 00
   * 4d1803:	c7 85 5c ff ff ff 01	mov	dword [ebp-0xa4],0x1	; local var at -0xa4 = 1
   * 4d180a:	00 00 00
   */

#define INJ	"\xb9\xd4\xc3\x00\x00"		/* mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll		*/\
		"\x8b\x45\x14"			/* mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the player number	*/\
		"\x85\xc0"			/* test	eax, eax		; if (eax								*/\
		"\x74\x03"			/* je	0x4d17f2		;         == 0) goto 0x4d17f2;						*/\
		"\x83\xc9\x01"			/* or	ecx, 0x1		; ecx |= 0x1			// low bit: player number		*/\
/*4d17f2:*/	"\x8b\x45\x18"			/* mov	eax, dword [ebp+0x18]	; eax = fifth argument to enclosing function	// the card number	*/\
		"\xc1\xe0\x10"			/* shl	eax, 0x10		; eax <<= 16								*/\
		"\x09\xc1"			/* or	ecx, eax		; ecx |= eax			// hi word: the card number		*/\
		"\x89\x8d\x70\xff\xff\xff"	/* mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack		*/\
		"\xe9\xb2\x00\x00\x00"		/* jmp	0x4d18b7		; goto 0x4d18b7	// skip the rest of the code dumbing down color		*/\
		"\x90\x90\x90\x90"		/*													*/\
		"\x90\x90\x90\x90"		/* nop	(eight, so the following code is still aligned)							*/
  SEEK_AND_WRITE(f, 0xd17e3);
#undef INJ

  /********************************************************************************************
   * Replace card_ptr_t::color for DrawFullCard call in _DRAW_FULL_CARD_EFFECT_CARDS (4D1950) *
   *******************************************************************************************/
  /* Previous contents:
   * 4d1ab9:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
   * 4d1abc:	50			push	eax			; arg2 = eax
   * 4d1abd:	8b 45 10		mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
   * 4d1ac0:	50			push	eax			; arg1 = eax
   * 4d1ac1:	e8 fa 72 f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
   * 4d1ac6:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d1ac9:	89 85 58 ff ff ff	mov	dword [ebp-0xa8], eax	; local var at -0xa8 = eax
   * 4d1acf:	c7 85 54 ff ff ff 00	mov	dword [ebp-0xac], 0x0	; local var at -0xac = 0
   * 4d1ad6:	00 00 00
   * 4d1ad9:	c7 85 5c ff ff ff 01	mov	dword [ebp-0xa4], 0x1	; local var at -0xa4 = 1
   * 4d1ae0:	00 00 00
   */

#define INJ	"\xb9\xd4\xc3\x00\x00"		/* mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll		*/\
		"\x8b\x45\x10"			/* mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number	*/\
		"\x85\xc0"			/* test	eax, eax		; if (eax								*/\
		"\x74\x03"			/* je	0x4d1ac8		;         == 0) goto 0x4d1ac8						*/\
		"\x83\xc9\x01"			/* or	ecx, 0x1		; ecx |= 0x1			// low bit: the player number		*/\
/*4d1ac8:*/	"\x8b\x45\x14"			/* mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number	*/\
		"\xc1\xe0\x10"			/* shl	eax, 0x10		; eax <<= 16								*/\
		"\x09\xc1"			/* or	ecx, eax		; ecx |= eax			// hi word: the card number		*/\
		"\x89\x8d\x70\xff\xff\xff"	/* mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack		*/\
		"\xe9\xb2\x00\x00\x00"		/* jmp	0x4d1b8d		; goto 0x4d1b8d	// skip the rest of the code dumbing down color		*/\
		"\x90\x90\x90\x90"		/*													*/\
		"\x90\x90\x90\x90"		/* nop	(eight, so the following code is still aligned)							*/
  SEEK_AND_WRITE(f, 0xd1ab9);
#undef INJ

  /*********************************************************************************
   * Replace card_ptr_t::color for DrawSmallCard call in _DRAW_SMALL_CARD (4D2D80) *
   ********************************************************************************/
  /* Previous contents:
   * 4d2de8:	8b 45 14		mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number
   * 4d2deb:	50			push	eax			; arg2 = eax
   * 4d2dec:	8b 45 10		mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number
   * 4d2def:	50			push	eax			; arg1 = eax
   * 4d2df0:	e8 cb 5f f6 ff		call	0x438dc0		; eax = get_displayed_card_instance_t_color(arg1, arg2)
   * 4d2df5:	83 c4 08		add	esp, 0x8		; pop two arguments
   * 4d2df8:	89 85 54 ff ff ff	mov	dword [ebp-0xac], eax	; local var at -0xac = eax
   * 4d2dfe:	c7 85 50 ff ff ff 00	mov	dword [ebp-0xb0], 0x0	; local var at -0xb0
   * 4d2e05:	00 00 00
   * 4d2e08:	c7 85 58 ff ff ff 01	mov	dword [ebp-0xa8], 0x1	; local var at -0xa8 = 1
   * 4d2e0f:	00 00 00
   */

#define INJ	"\xb9\xd4\xc3\x00\x00"		/* mov	ecx, 0xc3d4		; ecx = 0xc3d4	// a magic number recognized by drawcardlib.dll		*/\
		"\x8b\x45\x10"			/* mov	eax, dword [ebp+0x10]	; eax = third argument to enclosing function	// the player number	*/\
		"\x85\xc0"			/* test	eax, eax		; if (eax								*/\
		"\x74\x03"			/* je	0x4d2df7		;         == 0) goto 0x4d2df7						*/\
		"\x83\xc9\x01"			/* or	ecx, 0x1		; ecx |= 0x1			// low bit: the player number		*/\
/*4d2df7:*/	"\x8b\x45\x14"			/* mov	eax, dword [ebp+0x14]	; eax = fourth argument to enclosing function	// the card number	*/\
		"\xc1\xe0\x10"			/* shl	eax, 0x10		; eax <<= 16								*/\
		"\x09\xc1"			/* or	ecx, eax		; ecx |= eax			// hi word: the card number		*/\
		"\x89\x8d\x70\xff\xff\xff"	/* mov	dword [ebp-0x90], ecx	; local var at -0x90 = ecx	// card_ptr_t::color on stack		*/\
		"\xe9\xb2\x00\x00\x00"		/* jmp	0x4d2ebc		; goto 0x4d2ebc	// skip the rest of the code dumbing down color		*/\
		"\x90\x90\x90\x90"		/*													*/\
		"\x90\x90\x90\x90"		/* nop	(eight, so the following code is still aligned)							*/
  SEEK_AND_WRITE(f, 0xd2de8);

  CLOSE();

  SUCCESS("patch_drawfullcard_drawsmallcard_color");
}
