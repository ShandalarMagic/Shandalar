#!/bin/env perl

# Updates Magic.exe in-place.

# 1. Sets the accelerator for "Gold" in the choose_a_card() dialog to O instead of G.  G conflicts with the accelerator for green, so neither could be used.

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

  # Previous contents:
  # 000f7650: 8000 2600 4700 6f00 6c00 6400 0000 0000  ..&.G.o.l.d.....
  #                ^^^^ ^^

  seek_and_write($f, 0xf7652,
		 ord('G'), 0x00, ord('&'));	# to G&old

  close $f or die "Couldn't close $filename: ?!";
}
