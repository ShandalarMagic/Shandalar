#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Eliminates the prompt to respond to the playing of lands that are also other card types (artifact lands, dryad arbor)

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

  ##########################################################################
  # Eliminate chance to respond to land-and-whatever cards with interrupts #
  ##########################################################################

  # Previous contents:
  # 433bb8:	80 f9 01	cmp	cl, 0x1			; if (cl - 1
  # 433bbb:	74 76		je	0x433c33		;     == 0) goto 0x433c33

  seek_and_write($f, 0x33bb8,
		 0xf6, 0xc1, 0x01,	# test cl, 0x1		; if (cl & 1
		 0x75, 0x76);		# jne 0x433c33		;     != 0) goto 0x433c33

  ###################################################################
  # Eliminate "AIName plays..." dialog from land-and-whatever cards #
  ###################################################################

  # Previous contents:
  # 433cbb:	f6 c1 7e		test	cl,0x7e			; if (cl & 0x7e
  # 433cbe:	0f 84 7d 00 00 00	je	0x433d41		;     == 0) goto 0x433d41

  seek_and_write($f, 0x33cbb,
		 0xf6, 0xc1, 0x01,			# test cl, 0x1	; if (cl & 0x1
		 0x0f, 0x85, 0x7d, 0x00, 0x00, 0x00);	# jne 0x433d41	;     != 0) goto 0x433d41

  ##########################################################################
  # Eliminate chance to respond to land-and-whatever cards with interrupts #
  ##########################################################################

  # Previous contents:
  # 433e91:	80 f9 01	cmp	cl, 0x1			; if (cl - 1
  # 433e94:	74 49		je	0x433edf		;     == 0) goto 0x433edf

  seek_and_write($f, 0x33e91,
		 0xf6, 0xc1, 0x01,	# test cl, 0x1		; if (cl & 1
		 0x75, 0x49);		# jne 0x433c33		;     != 0) goto 0x433edf

  close $f or die "Couldn't close $filename: ?!";
}
