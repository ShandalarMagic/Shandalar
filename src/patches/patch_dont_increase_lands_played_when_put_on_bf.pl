#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Don't increase the lands_played global during put_card_in_play() ("put a land on the battlefield"), only put_card_on_stack3() ("play a land").  This keeps
# our "You may play extra lands each turn" cards from seeing lands put on the battlefield but not played as counting towards the limit, and keeps Fastbond from
# damaging you for them (as is now correct).

use strict;
use warnings;
use Manalink::Patch;

######################
# In put_into_play() #
######################
# Previous contents:
# 4b5ec5:	ff 05 fc f1 4e 00	inc	dword [0x4ef1fc]	; ++lands_played

patch("Magic.exe", 0x4b5ec5,
      (0x90) x 6);			# nop
