#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Hooks the only call to create_card_instance() to call the corresponding funciton in C.

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

  ####################################################################################################
  # Replace call to exe's create_card_instance() at 0x4796a0 with 0x200b272 (create_card_instance()) #
  ####################################################################################################
  # Previous contents:
  # 479656:	e8 45 00 00 00		call	0x4796a0

  seek_and_write($f, 0x79656,
		 0xe8, 0x17, 0x1c, 0xb9, 0x01);		# call	create_card_instance	; 0x0200b272

  close $f or die "Couldn't close $filename: ?!";
}
