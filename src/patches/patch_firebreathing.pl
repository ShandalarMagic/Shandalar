#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Makes activation of Firebreathing not target the creature it's enchanting (which was correct for its 4th edition wording).

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

  ###########################
  # In card_firebreathing() #
  ###########################
  # Previous contents:
  # 4ba381:	c6 46 36 01		mov	byte [esi+0x36], 0x1		; esi->number_of_targets = 1

  seek_and_write($f, 0xba381,
		 (0x90) x 4);				# nop	; comment out the instruction

  close $f or die "Couldn't close $filename: ?!";
}
