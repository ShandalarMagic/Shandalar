#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.

#    When a card is activated from within charge_mana(), several global variables are copied into locals and then zeroed - x_value, max_x_value, pay_mana_xbugrw
# (an array starting at PAY_MANA_COLORLESS and proceeding through PAY_MANA_WHITE and one additional (artifact?) for a total of 7 dwords).  mana_pool[8*player]
# is also copied into a local before activation, though it's not cleared.
#    Since the activated card needs to know which colors of mana to activate for, the pay_mana_xbugrw array is consolidated into a color_test_t value and
# stored in needed_colors.  Unfortunately, 1) the *amount* of each color needed is lost, and we sometimes need it (e.g., for Axebane Guardian); and 2) before
# the global values are restored but after needed_colors is itself zeroed, the EVENT_TAP_CARD that Mana Flare and similar cards work through is dispatched.
#    So what we need to do is store the addresses of the four local variables that temporarily store these globals into four *other* globals sometime before
# we start calling card functions, and store NULLs there afterwards so there's no chance they'll have stale values.  Both of these need to be done in two
# places: in charge_mana(), where they're being manually tapped to pay for a cost; and in autotap_mana_source(), where sources are drawn from by both the AI
# and when a human double-clicks to automatically pay a spell or ability's mana cost.  mana_pool isn't cached before calls from autotap_mana_source().

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

  ###########################################################################################
  # Jump out to injection to store local variables' addresses in globals from charge_mana() #
  ###########################################################################################

  # Previous contents:
  # 42e953:	a1 a0 f1 4e 00		mov	eax, dword [0x4ef1a0]	; eax = x_value
  # 42e958:	...

  #Obsolete - charge_mana() now in C
  #seek_and_write($f, 0x2e953,
  #	       0xe9, 0x38, 0x17, 0xe4, 0x00);	# jmp	0x1270090

  ##################################################################################################
  # Jump out to injection to NULL globals containing local variables' addresses from charge_mana() #
  ##################################################################################################

  # Previous contents:
  # 42eac9:	8b 85 50 ff ff ff	mov	eax, dword [ebp-0xb0]	; eax = localvar_0xb0
  # 42eacf:	...

  #Obsolete - charge_mana() now in C
  #seek_and_write($f, 0x2eac9,
  #	       0xe9, 0x12, 0x16, 0xe4, 0x00,	# jmp	0x12700e0
  #	       0x90);				# nop	; so the following code is still aligned

  ###################################################################################################
  # Jump out to injection to store local variables' addresses in globals from autotap_mana_source() #
  ###################################################################################################

  # Previous contents:
  # 42ff0a:	a1 a0 f1 4e 00		mov	eax, dword [0x4ef1a0]	; eax = x_value
  # 42ff0f:	...

  #Obsolete - autotap_mana_source() now in C
  #seek_and_write($f, 0x2ff0a,
  #		 0xe9, 0xf5, 0x01, 0xe4, 0x00);	# jmp	0x1270104

  ##########################################################################################################
  # Jump out to injection to NULL globals containing local variables' addresses from autotap_mana_source() #
  ##########################################################################################################

  # Previous contents:
  # 43001a:	8b 45 d0		mov	eax, dword [ebp-0x30]	; eax = localvar_0x30
  # 43001d:     a3 a0 f1 4e 00		mov	[0x4ef1a0], eax		; x_value = eax
  # 430022:     ...

  #Obsolete - autotap_mana_source() now in C
  #seek_and_write($f, 0x3001a,
  #		 0xe9, 0x1b, 0x01, 0xe4, 0x00,	# jmp	0x127013a
  #		 0x90, 0x90, 0x90);		# nop	; three nops so the following code is still aligned

  #####################################
  # Initial values of these variables #
  #####################################

  # This is in space that used to contain cards_data[], and is currently between asm-coded cards.

  seek_and_write($f, 0xd9434,
		 0x00, 0x00, 0x00, 0x00,		# NULL for charge_mana_pre_mana_pool
		 0xa0, 0xf1, 0x4e, 0x00,		# 0x4ef1a0 = &x_value
		 0x00, 0xf4, 0x4e, 0x00,		# 0x4ef400 = &pay_mana_colorless
		 0x98, 0xf1, 0x4e, 0x00);		# 0x4ef198 = &max_x_value

  close $f or die "Couldn't close $filename: ?!";


  $filename = "ManalinkEx.dll";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  #####################################################################################
  # The injection to store local variables' addresses in globals (from charge_mana()) #
  #####################################################################################

  #Obsolete - charge_mana() now in C
  #seek_and_write($f, 0x26f490,	# mapped to 0x1270090
  #	       0xff, 0x35, 0x34, 0x94, 0x4d, 0x00,	# push	dword [0x4d9434]	; store old value of 0x4d9434 on stack
  #	       0x8d, 0x85, 0x30, 0xff, 0xff, 0xff,	# lea	eax, [ebp-0xd0]		; store &local_copy_of_mana_pool...
  #	       0xa3, 0x34, 0x94, 0x4d, 0x00,		# mov	dword [0x4d9434], eax	; ...at 0x4d9434
  #	       0xff, 0x35, 0x38, 0x94, 0x4d, 0x00,	# push	dword [0x4d9438]	; store old value of 0x4d9438 on stack
  #	       0x8d, 0x85, 0x50, 0xff, 0xff, 0xff,	# lea	eax, [ebp-0xb0]		; store &local_copy_of_x_value...
  #	       0xa3, 0x38, 0x94, 0x4d, 0x00,		# mov	dword [0x4d9438], eax	; ...at 0x4d9438
  #	       0xff, 0x35, 0x3c, 0x94, 0x4d, 0x00,	# push	dword [0x4d943c]	; store old value of 0x4d943c on stack
  #	       0x8d, 0x85, 0x54, 0xff, 0xff, 0xff,	# lea	eax, [ebp-0xac]		; store &local_copy_of_pay_mana_xbugrw...
  #	       0xa3, 0x3c, 0x94, 0x4d, 0x00,		# mov	dword [0x4d943c], eax	; ...at 0x4d943c
  #	       0xff, 0x35, 0x40, 0x94, 0x4d, 0x00,	# push	dword [0x4d9440]	; store old value of 0x4d9440 on stack
  #	       0x8d, 0x85, 0x74, 0xff, 0xff, 0xff,	# lea	eax, [ebp-0x8c]		; store &local_copy_of_max_x_value...
  #	       0xa3, 0x40, 0x94, 0x4d, 0x00,		# mov	dword [0x4d9440], eax	; ...at 0x4d9440
  #	       0xa1, 0xa0, 0xf1, 0x4e, 0x00,		# mov	eax, dword [0x4ef1a0]	; eax = x_value	; the command replaced by the jmp here
  #	       0xe9, 0x7a, 0xe8, 0x1b, 0xff,		# jmp	0x42e958		; jmp to command following injection
  #	       0x90, 0x90);				# nop				; two nops to pad to next fragment

  ############################################################################################
  # The injection to NULL globals containing local variables' addresses (from charge_mana()) #
  ############################################################################################

  #Obsolete - charge_mana() now in C
  #seek_and_write($f, 0x26f4e0,	# mapped to 0x12700e0
  #	       0x8f, 0x05, 0x40, 0x94, 0x4d, 0x00,	# pop	dword [0x4d9440]	; restore old value of 0x4d9440 from stack
  #	       0x8f, 0x05, 0x3c, 0x94, 0x4d, 0x00,	# pop	dword [0x4d943c]	; restore old value of 0x4d943c from stack
  #	       0x8f, 0x05, 0x38, 0x94, 0x4d, 0x00,	# pop	dword [0x4d9438]	; restore old value of 0x4d9438 from stack
  #	       0x8f, 0x05, 0x34, 0x94, 0x4d, 0x00,	# pop	dword [0x4d9434]	; restore old value of 0x4d9434 from stack
  #	       0x8b, 0x85, 0x50, 0xff, 0xff, 0xff,	# mov	eax, dword [ebp-0xb0]	; eax = localvar_0xb0	; the command replaced by the jmp here
  #	       0xe9, 0xcc, 0xe9, 0x1b, 0xff,		# jmp	0x42eacf		; jmp to command following injection
  #	       0x90);					# nop				; nop to pad to next fragment

  #############################################################################################
  # The injection to store local variables' addresses in globals (from autotap_mana_source()) #
  #############################################################################################

  #seek_and_write($f, 0x26f504,	# mapped to 0x1270104
  #		 0xff, 0x35, 0x38, 0x94, 0x4d, 0x00,	# push	dword [0x4d9438]	; store old value of 0x4d9438 on stack
  #		 0x8d, 0x45, 0xd0,			# lea	eax, [ebp-0x30]		; store &local_copy_of_x_value...
  #		 0xa3, 0x38, 0x94, 0x4d, 0x00,		# mov	dword [0x4d9438], eax	; ...at 0x4d9438
  #		 0xff, 0x35, 0x3c, 0x94, 0x4d, 0x00,	# push	dword [0x4d943c]	; store old value of 0x4d943c on stack
  #		 0x8d, 0x45, 0xd8,			# lea	eax, [ebp-0x28]		; store &local_copy_of_pay_mana_xburgrw...
  #		 0xa3, 0x3c, 0x94, 0x4d, 0x00,		# mov	[0x4d943c], eax		; ...at 0x4d943c
  #		 0xff, 0x35, 0x40, 0x94, 0x4d, 0x00,	# push	dword [0x4d9440]	; store old value of 0x4d9440 on stack
  #		 0x8d, 0x45, 0xc4,			# lea	eax, [ebp-0x3c]		; store &local_copy_of_max_x_value...
  #		 0xa3, 0x40, 0x94, 0x4d, 0x00,		# mov	[0x4d9440], eax		; ...at 0x4d9440
  #		 0xa1, 0xa0, 0xf1, 0x4e, 0x00,		# mov	eax, [0x4ef1a0]		; eax = x_value	; the command replaced by the jmp here
  #		 0xe9, 0xd7, 0xfd, 0x1b, 0xff,		# jmp	0x42ff0f		; jmp to cmomand following injection
  #		 0x90, 0x90);				# nop				; two nops to pad to next fragment

  ############################################################################################
  # The injection to NULL globals containing local variables' addresses (from charge_mana()) #
  ############################################################################################

  #seek_and_write($f, 0x26f53a,	# mapped to 0x127013a
  #		 0x8f, 0x05, 0x40, 0x94, 0x4d, 0x00,	# pop	dword [0x4d9440]	; restore old value of 0x4d9440 from stack
  #		 0x8f, 0x05, 0x3c, 0x94, 0x4d, 0x00,	# pop	dword [0x4d943c]	; restore old value of 0x4d943c from stack
  #		 0x8f, 0x05, 0x38, 0x94, 0x4d, 0x00,	# pop	dword [0x4d9438]	; restore old value of 0x4d9438 from stack
  #		 0x8b, 0x45, 0xd0,			# mov	eax, dword [ebp-0x30]	; eax = localvar_0x30	; the command replaced by the jmp here
  #		 0xa3, 0xa0, 0xf1, 0x4e, 0x00,		# mov	[0x4ef1a0], eax		; x_value = eax		; the other command replaced by the jmp here
  #		 0xe9, 0xc9, 0xfe, 0x1b, 0xff,		# jmp	0x430022		; jmp to command following injection
  #		 0x90);					# nop				; nops to align

  close $f or die "Couldn't close $filename: ?!";
}
