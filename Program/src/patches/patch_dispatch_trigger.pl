#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces dispatch_trigger() with a call to the corresponding function in C.

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

  #############################################################################
  # Replace entire function with an injection to 0x200b1cd (dispatch_trigger) #
  #############################################################################
  # Previous contents:
  # 4371e0:	55		push	ebp
  # 4371e1:	8b ec		mov	ebp, esp
  # 4371e3:	83 ec 24	sub	esp, 0x24

  seek_and_write($f, 0x371e0,
		 0xe9, 0xe8, 0x3f, 0xbd, 0x01,	# jmp 0x0200b1cd
		 0x90);				# nop	(so the following code is still aligned)

  close $f or die "Couldn't close $filename: ?!";
}
