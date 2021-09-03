// -*- tab-width:8 -*-
#include "patch.h"

// Updates Magic.exe in-place.
// 1. Replace function card_generic_legend() at 0x403090 with a call to check_legend_rule() at 0x200aa11 in C.

int
main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  FILE* f;

  OPEN("Magic.exe");

  /***********************************************************************************************************
  * Replace function card_generic_legend() at 0x403090 with a call to check_legend_rule() at 0x200aa11 in C. *
  ***********************************************************************************************************/
  /*
   * Previous contents:
   * 403090:	55			push	ebp			; save old stack
   * 403091:	8b ec			mov	ebp, esp		; new stack
   * 403093:	56			push	esi			; save old value of esi
   * 403094:	8b 45 10		mov	eax, dword [ebp+0x10]	; eax = event
   */

#define INJ	"\xe9\x7c\x79\xc0\x01"	/* jmp 0x200aa11		; check_legend_rule(), which happens to take the same arguments		*/\
		"\x90\x90"		/* nop (two, so the following (dead) code is aligned)							*/
  SEEK_AND_WRITE(f, 0x3090);
#undef INJ

  CLOSE();

  SUCCESS("patch_legend_rule");
}

