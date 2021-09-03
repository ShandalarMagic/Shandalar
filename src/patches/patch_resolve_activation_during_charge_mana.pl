#!/bin/env perl

# Updates Magic.exe in-place.

# 1. Replace the calls to dispatch_event_to_single_card(player, card, EVENT_ACTIVATE, 1-player, -1) in charge_mana() and autotap_mana_source() to a function in
#    C that does that, then copies the new data into the activation card already on the stack (as during normal activation).  See comments in
#    event_activate_then_duplicate_into_stack() in engine.c.

use strict;
use warnings;
use Manalink::Patch;

####################
# in charge_mana() #
####################
# Previous contents:
#42ea4a:	e8 01 71 00 00	call	0x435b50	; dispatch_event_to_single_card(/*5 args already pushed*/)

#Obsolete - charge_mana() now in C
#patch("Magic.exe", 0x42ea4a,
#      "e8 d4 3f bd 01");	# call	0x2002a23	; event_activate_then_duplicate_into_stack(/*same 5 args already pushed*/)

############################
# in autotap_mana_source() #
############################
# Previous contents:
#42ffb2:	e8 99 5b 00 00	call	0x435b50	; dispatch_event_to_single_card(/*5 args already pushed*/)

patch(0x42ffb2,
      "e8 6c 2a bd 01");	# call	0x2002a23	; event_activate_then_duplicate_into_stack(/*same 5 args already pushed*/)
