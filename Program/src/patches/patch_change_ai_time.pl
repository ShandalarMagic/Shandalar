#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Moves the function that checks the base amount of time the AI's spent speculating into C.
# 2. Adds a call to after_load_game() in C at the end of each of the four load game functions.

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

  #####################################################################################
  # Replace function check_timer_for_ai_speculation() at 0x472d10 with a call into C. #
  #####################################################################################

  # Previous contents:
  # 472d10:	e8 7b ff ff ff		# call	0x472c90		; eax = get_usertime_of_current_thread_in_ms()
  # 472d15:	2b 05 64 bf 56 00	# sub	eax, dword [0x56bf64]	; eax -= start_usertime_of_current_thread_in_ms
  # 472d1b:	c1 e0 02		# shl	eax, 0x2		; eax *= 4;
  # 472d1e:	8d 04 80		# lea	eax, [eax+eax*4]	; eax *= 5;
  # 472d21:	8d 04 80		# lea	eax, [eax+eax*4]	; eax *= 5;
  # 472d24:	b9 1d 15 00 00		# mov	ecx, 0x151d		; ecx = 5405
  # 472d29:	99			# cdq				; sign-extend eax into edx
  # 472d2a:	f7 f9			# idiv	ecx			; eax = edx:eax / ecx; edx = edx:eax % ecx
  # 472d2c:	c3			# ret				; return

  seek_and_write($f, 0x72d10,
		 0xe9, 0xf2, 0x7c, 0xb9, 0x01,	# jmp	0x200aa07	;  check_timer_for_ai_speculation() in C
		 (0x90) x 24);			# nop			;  24 no-ops, to overwrite the rest of the function

  #############################################################
  # Add call to after_load_game() at end of load_sealeddeck() #
  #############################################################

  # Previous contents:
  #(4a083d:	5f			# pop	edi	; restore saved value of edi)
  #(4a083e:	5e			# pop	esi	; restore saved value of esi)
  #(4a083f:	5b			# pop	ebx	; restore saved value of ebx)
  # 4a0840:	c9			# leave		; deallocate local variables
  # 4a0841:	c3			# ret		; return
  # 4a0842:	cc			# int3
  # 4a0843:	cc			# int3
  # 4a0844:	cc			# int3
  # 4a0845:	cc			# int3
  # 4a0846:	cc			# int3
  # 4a0847:	cc			# int3
  # 4a0848:	cc			# int3
  # 4a0849:	cc			# int3
  # 4a084a:	cc			# int3
  # 4a084b:	cc			# int3
  # 4a084c:	cc			# int3
  # 4a084d:	cc			# int3
  # 4a084e:	cc			# int3
  # 4a084f:	cc			# int3
  # 4a0850:	...					; start of next function

  seek_and_write($f, 0xa0840,
		 0x50,				# push	eax		; save return value
		 0xe8, 0xc6, 0xa1, 0xb6, 0x01,	# call	0x200aa0c	; after_load_game() in C
		 0x58,				# pop	eax		; restore return value
		 0xc9,				# leave			; deallocate local variables
		 0xc3);				# ret			; return

  ###############################################################
  # Jump to end of load_sealeddeck() at end of load_gametype0() #
  ###############################################################

  # Previous contents:
  # 49f739:	5f			# pop	edi	; restore saved value of edi
  # 49f73a:	5e			# pop	esi	; restore saved value of esi
  # 49f73b:	5b			# pop	ebx	; restore saved value of ebx
  # 49f73c:	c9			# leave		; deallocate local variables
  # 49f73d:	c3			# ret		; return
  # 49f73e:	cc			# int3
  # 49f73f:	cc			# int3
  # 49f740:	...					; start of next function

  seek_and_write($f, 0x9f739,
		 0xe9, 0xff, 0x10, 0x00, 0x00);	# jmp 0x4a083d		; goto tail of load_sealeddeck()

  ##############################################################
  # Jump to end of load_sealeddeck() at end of load_soloduel() #
  ##############################################################

  # Previous contents:
  # 4a049e:	5f			# pop	edi	; restore saved value of edi
  # 4a049f:	5e			# pop	esi	; restore saved value of esi
  # 4a04a0:	5b			# pop	ebx	; restore saved value of ebx
  # 4a04a1:	c9			# leave		; deallocate local variables
  # 4a04a2:	c3			# ret		; return
  # 4a04a3:	cc			# int3
  # 4a04a4:	cc			# int3
  # 4a04a5:	cc			# int3
  # 4a04a6:	cc			# int3
  # 4a04a7:	cc			# int3
  # 4a04a8:	cc			# int3
  # 4a04a9:	cc			# int3
  # 4a04aa:	cc			# int3
  # 4a04ab:	cc			# int3
  # 4a04ac:	cc			# int3
  # 4a04ad:	cc			# int3
  # 4a04ae:	cc			# int3
  # 4a04af:	cc			# int3
  # 4a04b0:	...					; start of next function

  seek_and_write($f, 0xa049e,
		 0xe9, 0x9a, 0x03, 0x00, 0x00);	# jmp 0x4a083d		; goto tail of load_sealeddeck()

  ##############################################################
  # Jump to end of load_sealeddeck() at end of load_gauntlet() #
  ##############################################################

  # Previous contents:
  # 4a06e2:	5f			# pop	edi	; restore saved value of edi
  # 4a06e3:	5e			# pop	esi	; restore saved value of esi
  # 4a06e4:	5b			# pop	ebx	; restore saved value of ebx
  # 4a06e5:	c9			# leave		; deallocate local variables
  # 4a06e6:	c3			# ret		; return
  # 4a06e7:	cc			# int3
  # 4a06e8:	cc			# int3
  # 4a06e9:	cc			# int3
  # 4a06ea:	cc			# int3
  # 4a06eb:	cc			# int3
  # 4a06ec:	cc			# int3
  # 4a06ed:	cc			# int3
  # 4a06ee:	cc			# int3
  # 4a06ef:	cc			# int3
  # 4a06f0:	...					; start of next function

  seek_and_write($f, 0xa06e2,
		 0xe9, 0x56, 0x01, 0x00, 0x00);	# jmp 0x4a083d		; goto tail of load_sealeddeck()

  close $f or die "Couldn't close $filename: ?!";
}
