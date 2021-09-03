#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces the logic for human left-double-click mana autotapping with a call into C.  Note that the chunk of code that's replaced is also called for AI
#    autotapping, in addition to later ai-only code.
# 2. Enable the "Don't auto tap this card" right-click menu option for all mana sources, not just lands.

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

  #######################################################################################################
  # In charge_mana(): replace the logic for human left-double-click mana autotapping with a call into C #
  #######################################################################################################
  # Previous contents:
  # 42e24e:	a1 98 f1 4e 00		mov	eax, [0x4ef198]			; eax = max_x_value
  # 42e253:	50			push	eax				; arg3 = eax
  # 42e254:	a1 a0 f1 4e 00		mov	eax, [0x4ef1a0]			; eax = x_value
  # 42e259:	50			push	eax				; arg2 = eax
  # 42e25a:	68 00 f4 4e 00		push	0x4ef400			; arg1 = &PAY_MANA_COLORLESS
  # 42e25f:	e8 cc 0e 00 00		call	0x42f130			; eax = pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)
  # 42e264:	83 c4 0c		add	esp, 0xc			; pop 3 arguments off of stack
  # 42e267:	85 c0			test	eax, eax			; if ((eax&eax)
  # 42e269:	75 1a			jne	0x42e285			;     != 0) goto 0x42e285
  # 42e26b:	8b 45 f8		mov	eax, dword [ebp-0x8]		; eax = v47
  # 42e26e:	50			push	eax				; arg5 = eax
  # 42e26f:	6a 1e			push	0x1e				; arg4 = AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP|AUTOTAP_NO_NONBASIC_LANDS
  # 42e271:	8d 45 f4		lea	eax, [ebp-0xc]			; eax = &v46
  # 42e274:	50			push	eax				; arg3 = eax
  # 42e275:	8d 45 b4		lea	eax, [ebp-0x4c]			; eax = &amt
  # 42e278:	50			push	eax				; arg2 = eax
  # 42e279:	8b 45 08		mov	eax, dword [ebp+0x8]		; eax = player
  # 42e27c:	50			push	eax				; arg1 = eax
  # 42e27d:	e8 1e 12 00 00		call	0x42f4a0			; eax = try_to_pay_for_mana_by_autotapping(player, &amt, &v46, AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP|AUTOTAP_NO_NONBASIC_LANDS, v47)
  # 42e282:	83 c4 14		add	esp, 0x14			; pop 5 arguments off of stack

  # 42e285:	a1 98 f1 4e 00		mov	eax, [0x4ef198]			; eax = max_x_value
  # 42e28a:	50			push	eax				; arg3 = eax
  # 42e28b:	a1 a0 f1 4e 00		mov	eax, [0x4ef1a0]			; eax = x_value
  # 42e290:	50			push	eax				; arg2 = eax
  # 42e291:	68 00 f4 4e 00		push	0x4ef400			; arg1 = &PAY_MANA_COLORLESS
  # 42e296:	e8 95 0e 00 00		call	0x42f130			; eax = pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)
  # 42e29b:	83 c4 0c		add	esp, 0xc			; pop 3 arguments off of stack
  # 42e29e:	85 c0			test	eax, eax			; if ((eax&eax)
  # 42e2a0:	75 1a			jne	0x42e2bc			;     != 0) goto 0x42e2bc
  # 42e2a2:	8b 45 f8		mov	eax, dword [ebp-0x8]		; eax = v47
  # 42e2a5:	50			push	eax				; arg5 = eax
  # 42e2a6:	6a 1c			push	0x1c				; arg4 = AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP
  # 42e2a8:	8d 45 f4		lea	eax, [ebp-0xc]			; eax = &v46
  # 42e2ab:	50			push	eax				; arg3 = eax
  # 42e2ac:	8d 45 b4		lea	eax, [ebp-0x4c]			; eax = &amt
  # 42e2af:	50			push	eax				; arg2 = eax
  # 42e2b0:	8b 45 08		mov	eax, dword [ebp+0x8]		; eax = player
  # 42e2b3:	50			push	eax				; arg1 = eax
  # 42e2b4:	e8 e7 11 00 00		call	0x42f4a0			; eax = try_to_pay_for_mana_by_autotapping(player, &amt, &v46, AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP, v47)
  # 42e2b9:	83 c4 14		add	esp, 0x14			; pop 5 arguments off of stack
  # 42e2bc:	...

  seek_and_write($f, 0x2e24e,
		 0xff, 0x75, 0xf8,		# push	dword [ebp-0x8]		; arg4 = v47
		 0x8d, 0x45, 0xf4,		# lea	eax, [ebp-0xc]		; eax = &v46
		 0x50,				# push	eax			; arg3 = eax
		 0x8d, 0x45, 0xb4,		# lea	eax, [ebp-0x4c]		; eax = &amt
		 0x50,				# push	eax			; arg2 = eax
		 0xff, 0x75, 0x08,		# push	dword [ebp+0x8]		; arg1 = player
		 0xe8, 0xde, 0xad, 0xbe, 0xef,	# call	human_autotap_for_mana	; eax = human_autotap_for_mana(player, &amt, &v46, v47)
		 0x83, 0xc4, 0x10,		# add	esp, 0x10		; pop 4 arguments off of stack
		 0xeb, 0x56,			# jmp	0x42e2bc		; skip over the following nop's
		 (0x90) x 86);			# nop	(eighty-six! of them, to overwrite the rest of the code that the C function duplicates

  ###############################################################################################################################
  # In wndproc_CardClass(): enable the "Don't auto tap this card" right-click menu option for all mana sources, not just lands. #
  ###############################################################################################################################
  # Previous contents:
  # 48c292:	83 bd 10 f7 ff ff 00	cmp	dword [ebp-0x8f0], 0x0		; if (is_a_land - 0
  # 48c299:	74 46			je	0x48c2e1			;     == 0) goto 0x48c2e1
  # 48c29b:	...

  seek_and_write($f, 0x8c292,
		 (0x90) x 9);			# nop	(nine of them, to disable the check)

  close $f or die "Couldn't close $filename: ?!";
}
