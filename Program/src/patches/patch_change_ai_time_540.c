// -*- tab-width:8 -*-
#include "patch.h"

// Updates Magic.exe in-place.
// 1. Reduces AI thinking time by a factor of 10.

int
main(int argc, char** argv)
{
  (void)argc;
  (void)argv;
  FILE* f;

  OPEN("Magic.exe");

  /*************************************************************************************
  * Replace constant in check_timer_for_ai_speculation() at 0x472d10 from 5405 to 540. *
  *************************************************************************************/
  /*
   * Previous contents:
   * 472d10:	e8 7b ff ff ff		# call	0x472c90		; eax = get_usertime_of_current_thread_in_ms()
   * 472d15:	2b 05 64 bf 56 00	# sub	eax, dword [0x56bf64]	; eax -= start_usertime_of_current_thread_in_ms
   * 472d1b:	c1 e0 02		# shl	eax, 0x2		; eax *= 4;
   * 472d1e:	8d 04 80		# lea	eax, [eax+eax*4]	; eax *= 5;
   * 472d21:	8d 04 80		# lea	eax, [eax+eax*4]	; eax *= 5;
   * 472d24:	b9 1d 15 00 00		# mov	ecx, 0x151d		; ecx = 5405
   * 472d29:	99			# cdq				; sign-extend eax into edx
   * 472d2a:	f7 f9			# idiv	ecx			; eax = edx:eax / ecx; edx = edx:eax % ecx
   * 472d2c:	c3			# ret				; return
   */

#define INJ	"\xb9\x1c\x02\x00\x00"	/* mov	ecx, 0x021c		; ecx = 540	*/
  SEEK_AND_WRITE(f, 0x72d24);
#undef INJ

  CLOSE();

  SUCCESS("patch_change_ai_time_540");
}

