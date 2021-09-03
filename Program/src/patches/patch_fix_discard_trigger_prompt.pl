#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Changes the prompt for the discard trigger from "Draw a card" (PROMPT_SPECIALFEPHASE[5]) to the fixed string "Discarding".

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

  #####################
  # In discard_card() #
  #####################
  # Previous contents:
  # 4337ca:	68 d4 58 71 00		push	0x7158d4	; arg3 = str_draw_a_card

  seek_and_write($f, 0x337ca,
		 0x68, 0x9c, 0x95, 0x4e, 0x00);	# push	0x4e959c	; arg3 = "Discarding."

  ########
  # Data #
  ########
  # Previous contents: all 0

  seek_and_write($f, 0xe959c,
		 (map { ord $_ } (split //, "Discarding")),
		 0x00);

  close $f or die "Couldn't close $filename: ?!";
}
