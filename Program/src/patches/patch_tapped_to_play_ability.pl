#!/bin/env perl

# Updates Magic.exe and ManalinkEx.dll in-place.
# 1. Adds an extra EVENT_TAPPED_TO_PLAY_ABILITY to activate().  Normally, activate() dispatches either EVENT_PLAY_ABILITY or EVENT_TAP_CARD if its prior
# dispatch of EVENT_ACTIVATE is successful and uncancelled, depending on whether the activating permanent became tapped; unfortunately, several other things
# dispatch EVENT_TAP_CARD as well, so it's nontrivial to indicate that "this EVENT_TAP_CARD was because an ability was activated".  Just always dispatching
# EVENT_PLAY_ABILITY (and sometimes EVENT_TAP_CARD in addition) won't work because there's a number of cards that trigger on "Whenever X activates an ability
# or becomes tapped".  Adding an extra event is the safest route; and it's sufficient and somewhat easier to just add one when EVENT_TAP_CARD is being sent than
# for both.
#
# The surrounding code looks something like
#
# if (!was_cancelled)
#   {
#     if (was_already_tapped || !(instance->state & STATE_TAPPED))
#       dispatch_event(player, card, EVENT_PLAY_ABILITY);
#     else
#       dispatch_event(player, card, EVENT_TAP_CARD);
#     // etc.

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

  #########################################################################################
  # Injects into the EVENT_TAP_CARD branch of activate() to dispatch an additional event. #
  #########################################################################################
  # Previous contents:
  # 43460d:	68 81 00 00 00	push	0x81			; arg3 = EVENT_TAP_CARD
  # 434612:	8b 45 10	mov	eax, dword [ebp+0x10]	; eax = card
  # 434615:	50		push	eax			; arg2 = card
  # 434616:	8b 45 0c	mov	eax, dword [ebp+0xc]	; eax = player
  # 434619:	50		push	eax			; arg1 = player
  # 43461a:	e8 91 13 00 00	call	0x4359b0		; dispatch_event(player, card, EVENT_TAP_CARD)
  # 43461f:	83 c4 0c	add	esp, 0xc		; pop three arguments off stack
  # 434622:	eb 15		jmp	0x434639		; skip the EVENT_PLAY_ABILITY branch.

  seek_and_write($f, 0x3460d,
		 0xe9, 0xbe, 0xbb, 0xe3, 0x00);	# jmp 0x12701d0

  close $f or die "Couldn't close $filename: ?!";


  $filename = "ManalinkEx.dll";
  open($f, "+<", $filename) or die "Couldn't open $filename: ?!";
  binmode($f);

  seek_and_write($f, 0x26f5d0,	#0x12701d0
		 0x68, 0x81, 0x00, 0x00, 0x00,	# push	0x81			; arg3 = EVENT_TAP_CARD
		 0xff, 0x75, 0x10,		# push	dword [ebp+0x10]	; arg2 = card
		 0xff, 0x75, 0x0c,		# push	dword [ebp+0xc]		; arg1 = player
		 0xe8, 0xd0, 0x57, 0x1c, 0xff,	# call	0x4359b0		; dispatch_event(player, card, EVENT_TAP_CARD)
		 0x83, 0xc4, 0x0c,		# add	esp, 0xc		; pop three arguments off stack
		 0x68, 0x00, 0x0c, 0x00, 0x00,	# push	0xc00			; arg3 = EVENT_TAPPED_TO_PLAY_ABILITY
		 0xff, 0x75, 0x10,		# push	dword [ebp+0x10]	; arg2 = card
		 0xff, 0x75, 0x0c,		# push	dword [ebp+0xc]		; arg1 = player
		 0xe8, 0xbd, 0x57, 0x1c, 0xff,	# call	0x4359b0		; dispatch_event(player, card, EVENT_TAPPED_TO_PLAY_ABILITY)
		 0x83, 0xc4, 0x0c,		# add	esp, 0xc		; pop three arguments off stack
		 0xe9, 0x3e, 0x44, 0x1c, 0xff,	# jmp	0x434639		; goto whence we came
		 0x90);				# nop

  close $f or die "Couldn't close $filename: ?!";
}
