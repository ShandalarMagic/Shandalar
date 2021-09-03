#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Replaces _REAL_TARGET_AVAILABLE(), _REAL_SELECT_TARGET(), and _REAL_VALIDATE_TARGET() with calls to corresponding functions in C.

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

  #####################################
  # Rewrite targetting functions in C #
  #####################################
  # Replace entire function with an injection to 0x0200a9ee (real_target_available)
  # Previous contents:
  # 47d940:	55		push	ebp
  # 47d941:	8b ec		mov	ebp, esp
  # 47d943:	83 ec 28	sub	esp, 0x28

  seek_and_write($f, 0x7d940,
		 0xe9, 0xa9, 0xd0, 0xb8, 0x01,	# jmp 0x0200a9ee
		 0x90);				# nop	(so the following code is still aligned)

  # Replace entire function with an injection to 0x0200a9f3 (real_select_target)
  # Previous contents:
  # 47e9d0:	55			push	ebp
  # 47e9d1:	8b ec			mov	ebp, esp
  # 47e9d3:	81 ec bc 03 00 00	sub	esp, 0x3bc

  seek_and_write($f, 0x7e9d0,
		 0xe9, 0x1e, 0xc0, 0xb8, 0x01,	# jmp 0x0200a9f3
		 0x90, 0x90, 0x90, 0x90);	# nop	(4 of them, so the following code is still aligned)

  # Replace entire function with an injection to 0x0200a9f8 (real_validate_target)
  # Previous contents:
  # 47db90:	55			push   ebp
  # 47db91:	8b ec			mov    ebp, esp
  # 47db93:	81 ec 24 01 00 00	sub    esp, 0x124

  seek_and_write($f, 0x7db90,
		 0xe9, 0x63, 0xce, 0xb8, 0x01,	# jmp 0x0200a9f8
		 0x90, 0x90, 0x90, 0x90);	# nop	(4 of them, so the following code is still aligned)

  close $f or die "Couldn't close $filename: ?!";
}
