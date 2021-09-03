#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Removes the special-casing that makes interrupts flagged EA_MANASOURCE themselves uninterruptible.

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

  ##########################
  # In put_card_on_stack() #
  ##########################
  # Previous contents:
  # 433bbd:	80 f9 20		cmp	cl, 0x20		; if (cl - TYPE_INTERRUPT
  # 433bc0:	75 08			jne	0x433bca		;     != 0) goto 0x433bca
  # 433bc2:	66 f7 47 3c 00 10	test	word [edi+0x3c], 0x1000	; if (edi->extra_ability & EA_MANASOURCE
  # 433bc8:	75 69			jne	0x433c33		;     != 0) goto 0x433c33 // skipping response window
  # 433bca:	...

  seek_and_write($f, 0x33bbd,
		 0xeb, 0x0b,		# jmp 0x433bca
		 (0x90) x 11);		# nop (to remove the rest of the test)

  ###########################
  # In put_card_on_stack3() #
  ###########################
  # Previous contents:
  # 433e96:	80 f9 20			cmp	cl, 0x20				; if (cl - TYPE_INTERRUPT
  # 433e99:	75 0c				jne	0x433ea7				;     != 0) goto 0x433ea7
  # 433e9b:	66 f7 84 c0 4c 70 7e 00 00 10	test	word [eax+eax*8+0x7e704c], 0x1000	; if (cards_data[eax]->extra_ability & EA_MANA_SOURCE
  # 433ea5:	75 38				jne	0x433edf				;     != 0) goto 0x433edf	// skipping response window
  # 433ea7:	...

  seek_and_write($f, 0x33e96,
		 0xeb, 0x0f,		# jmp 0x433ea7
		 (0x90) x 15);		# nop (to remove the rest of the test)

  close $f or die "Couldn't close $filename: ?!";
}
