#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Sets the global variable at 0x4EF188, which I currently have labelled PARTIALLY_extra_abilities_of_all_cards_in_play, to EA_CONTROLLED|EA_FORCE_ATTACK
#    instead of 0 at the start of the game and the end of each turn.
#
# Normally, several bits of each card in play's card_data_t::extra_ability - EA_CONTROLLED|EA_FELLWAR_STONE|EA_DECLARE_ATTACK|EA_BEFORE_COMBAT|EA_FORCE_ATTACK|
# EA_PAID_BLOCK|EA_PAID_ATTACK|EA_LICH|EA_SELECT_BLOCK|EA_SELECT_ATTACK|EA_MARTYR, or 0x1FFC0000 - are |ed into this at the end of get_abilities(player, card,
# EVENT_CHANGE_TYPE, ...), and zeroed in end_turn_phase() and start_duel().
#
# Each bit determines whether a specific event or trigger will be sent to all cards. (Bits 1, 2, and 4 are individually set and cleared elsewhere, and have a
# different purpose which isn't immediately obvious.)  EA_CONTROLLED corresponds to EVENT_CARDCONTROLLED, which is dispatched at the very end of gain_control()
# and exchange_control() with affected_card_controller and affected_card set to the newly-controlled permanent.  We'd like to use this event to enforce the
# legend rule after a control change.  However, it's impractical to set EA_CONTROLLED on every card that's either legendary or could add legendary to other
# cards, and setting it on Rules Engine won't work since it's not guaranteed to be in play.
#
# EA_FORCE_ATTACK, which I'm also setting globally, determines whether to dispatch EVENT_MUST_ATTACK.  Since the actual in-game card that checks for the event
# is usually a LEGACY_EFFECT_CUSTOM, either it has to go onto the legacy card itself (and thus will almost always be on anyway), or be set globally.  There
# isn't much sense in letting a juggernaut-card which itself must attack if able go unfixed for years solely because it only happens to be tested when there's
# an unrelated LEGACY_EFFECT_CUSTOM on the battlefield.

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

  #######################
  # In end_turn_phase() #
  #######################
  # Previous contents:
  # 43d576:	c7 05 88 f1 4e 00  00 00 00 00		; mov    dword [0x004ef188], 0x0	; PARTIALLY_extra_abilities_of_all_cards_in_play = 0

  seek_and_write($f, 0x3d576,
		 0xc7, 0x05, 0x88, 0xf1, 0x4e, 0x00,  0x00, 0x00, 0x00, 0x11);	# mov	dword [0x004ef188], 0x11000000	; PARTIALLY_extra_abilities_of_all_cards_in_play = EA_CONTROLLED|EA_FORCE_ATTACK

  ###################
  # In start_duel() #
  ###################
  # Previous contents:
  # 478b97:	c7 05 88 f1 4e 00  00 00 00 00		; mov    dword [0x004ef188], 0x0	; PARTIALLY_extra_abilities_of_all_cards_in_play = 0

  seek_and_write($f, 0x78b97,
		 0xc7, 0x05, 0x88, 0xf1, 0x4e, 0x00,  0x00, 0x00, 0x00, 0x11);	# mov	dword [0x004ef188], 0x11000000	; PARTIALLY_extra_abilities_of_all_cards_in_play = EA_CONTROLLED|EA_FORCE_ATTACK

  close $f or die "Couldn't close $filename: ?!";
}
