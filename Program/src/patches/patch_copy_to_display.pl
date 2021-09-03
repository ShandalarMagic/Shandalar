#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. Inject a call to copy_to_display_supplement() within copy_to_display().

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

  ###################################
  # Inject within copy_to_display() #
  ###################################
  # Previous contents:
  # 40b928:	68 88 02 62 00		push	0x620288	; arg1 = &CriticalSection
  # 40b92d:	e8 ea a4 0c 00		call	0x4d5e1c	; EnterCriticalSection(&CriticalSection)
  # 40b932:	...

  seek_and_write($f, 0xb928,
		 0xe9, 0x13, 0x4a, 0xe6, 0x00,		# jmp 0x1270340
		 (0x90) x 5);				# 5 nops (clear the function call)

  close $f or die "Couldn't close $filename: ?!";


  $filename = "ManalinkEx.dll";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  ###############
  # Call into C #
  ###############
  seek_and_write($f, 0x26f740,	#0x1270340
		 0x68, 0x88, 0x02, 0x62, 0x00,		# push	0x620288		; arg1 = &CriticalSection
		 0xe8, 0xd2, 0x5a, 0x26, 0xff,		# call	0x4d5e1c		; EnterCriticalSection(&CriticalSection)
		 0xe8, 0x9f, 0xb0, 0xd9, 0x00,		# call	0x200b3ee		; copy_to_display_supplement()
		 0xe9, 0xde, 0xb5, 0x19, 0xff);		# jmp	0x40b932		; return whence we came

  close $f or die "Couldn't close $filename: ?!";
}
