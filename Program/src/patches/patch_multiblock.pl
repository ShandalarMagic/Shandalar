#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces process_multiblock() with a call to the corresponding function in C.
# 2. Hooks the already-existing hook for card_multiblocker *again*.

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

  ###############################################################################
  # Replace entire function with an injection to 0x200b245 (process_multiblock) #
  ###############################################################################
  # Previous contents:
  # 4b3850:	55		push	ebp
  # 4b3851:	8b ec		mov	ebp, esp
  # 4b3853:	83 ec 1c	sub	esp, 0x1c

  seek_and_write($f, 0xb3850,
		 0xe9, 0xf0, 0x79, 0xb5, 0x01,	# jmp 0x0200b245
		 0x90);				# nop	(so the following code is still aligned)

  ###################################################################################
  # Replace entire function with an injection to 0x200b24a (card_multiblocker_hook) #
  ###################################################################################
  # Previous contents:
  # 401010:	e9 1b 74 05 00	jmp	0x458430	; Ha!

  seek_and_write($f, 0x1010,
		 0xe9, 0x35, 0xa2, 0xc0, 0x01);	# jmp 0x0200b24a

  close $f or die "Couldn't close $filename: ?!";
}
