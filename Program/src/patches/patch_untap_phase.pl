#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Hooks the only call to untap_phase() to call the corresponding funciton in C.
# 2. Removes the initial logging and check for Stasis in untap_phase(), since we do that in C now.

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

  ##################################################################################################
  # Replace call to exe's untap_phase() at 0x43a700 with an injection to 0x200b240 (untap_phase()) #
  ##################################################################################################
  # Previous contents:
  # 439966:	e8 95 0d 00 00		call	0x43a700

  seek_and_write($f, 0x39966,
		 0xe8, 0xd5, 0x18, 0xbd, 0x01);		# call	untap_phase	; 0x0200b240

  ###############################################
  # Remove initial logging and check for Stasis #
  ###############################################
  # Previous contents:
  # 43a70c:	f6 05 40 06 79 00 02	test	byte [0x790640], 0x2	; if ((trace_mode & 2)
  # 43a713:	74 2f			je	0x43a744		;     == 0) goto 0x43a744;
  # 43a715:	a1 40 ec 60 00		mov	eax, ds:0x60ec40	; eax = dword_60EC40
  # 43a71a:	ff 05 40 ec 60 00	inc	dword [0x60ec40]	; ++dword_60EC40
  # 43a720:	50			push	eax			; arg3 = eax
  # 43a721:	68 70 a3 4e 00		push	0x4ea370		; arg2 = "%d: Entering Untap Phase.\n"
  # 43a726:	8d 85 58 fc ff ff	lea	eax, [ebp-0x3a8]	; eax = &str[0] // a local variable on the stack
  # 43a72c:	50			push	eax			; arg1 = eax
  # 43a72d:	e8 62 b7 09 00		call	0x4d5e94		; eax = sprintf(&str[0], "%d: Entering Untap Phase.\n", /*old value of*/dword_60EC40)
  # 43a732:	83 c4 0c		add	esp, 0xc		; pop three arguments off the stack
  # 43a735:	8d 85 58 fc ff ff	lea	eax, [ebp-0x3a8]	; eax = &str[0]
  # 43a73b:	50			push	eax			; arg1 = eax
  # 43a73c:	e8 3f d6 06 00		call	0x4a7d80		; sub_4A7D80(&str[0])
  # 43a741:	83 c4 04		add	esp, 0x4		; pop one argument off the stack
  # 43a744:	68 e9 00 00 00		push	0xe9			; arg1 = CARD_ID_STASIS
  # 43a749:	e8 22 75 03 00		call	0x471c70		; eax = CardTypeFromID(CARD_ID_STASIS)
  # 43a74e:	83 c4 04		add	esp, 0x4		; pop one argument off the stack
  # 43a751:	83 f8 ff		cmp	eax, 0xffffffff		; if (eax - -1
  # 43a754:	74 39			je	0x43a78f		;     == 0) goto 0x43a78f
  # 43a756:	6a 00			push	0x0			; arg19 = 0
  # 43a758:	6a 00			push	0x0			; arg18 = 0
  # 43a75a:	6a 00			push	0x0			; arg17 = 0
  # 43a75c:	6a ff			push	0xffffffff		; arg16 = -1
  # 43a75e:	6a ff			push	0xffffffff		; arg15 = -1
  # 43a760:	6a ff			push	0xffffffff		; arg14 = -1
  # 43a762:	50			push	eax			; arg13 = CardTypeFromID(CARD_ID_STASIS)
  # 43a763:	6a 00			push	0x0			; arg12 = 0
  # 43a765:	6a 00			push	0x0			; arg11 = 0
  # 43a767:	6a 00			push	0x0			; arg10 = 0
  # 43a769:	6a 00			push	0x0			; arg9 = 0
  # 43a76b:	6a 00			push	0x0			; arg8 = 0
  # 43a76d:	6a 00			push	0x0			; arg7 = 0
  # 43a76f:	68 00 02 00 00		push	0x200			; arg6 = TARGET_ZONE_IN_PLAY
  # 43a774:	6a 02			push	0x2			; arg5 = 2
  # 43a776:	6a 02			push	0x2			; arg4 = 2
  # 43a778:	ff 75 08		push	dword [ebp+0x8]		; arg3 = player // the first argument to enclosing function
  # 43a77b:	6a 00			push	0x0			; arg2 = 0
  # 43a77d:	6a 00			push	0x0			; arg1 = 0
  # 43a77f:	e8 bc 31 04 00		call	0x47d940		; eax = target_available(/*19 arguments*/)
  # 43a784:	83 c4 4c		add	esp, 0x4c		; Pop 19 arguments off the stack
  # 43a787:	85 c0			test	eax, eax		; if ((eax|eax)
  # 43a789:	0f 85 53 05 00 00	jne	0x43ace2		;     != 0) goto 0x43ace2	// which returns 0
  # 43a78f:	...

  seek_and_write($f, 0x3a70c,
		 0xe9, 0x7e, 0x00, 0x00, 0x00,	# jmp 0x43a78f
		 (0x90) x 2);			# nop	; (two of them, so the following code is aligned)

  close $f or die "Couldn't close $filename: ?!";
}
