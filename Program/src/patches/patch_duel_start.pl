#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Prevent the human from always winning the coin toss against the AI (which is meant to happen at opponent_skill == 0).
#    Note that this is checked before pregame() is called, and so before settings are read.
# 2. Replace writes to opponent_skill during solo duel start to instead write to a global at 0x4e95a8 "singleplayer_mode".
# 3. Eliminates writes to opponent_skill in gauntlet and sealed deck mode.

use strict;
use warnings;
use Manalink::Patch;

#############################################################
# Prevent the human from always going first in start_duel() #
#############################################################
# Previous contents:
# 478c5e:	83 3d 30 a4 60 00 00	cmp    dword [0x60a430], 0x0	; if (opponent_skill - 0
# 478c65:	74 20			je     0x478c87			;     == 0) goto 0x478c87
# 478c67:	...
patch("Magic.exe", 0x478c5e,
      (0x90) x 9);	# nop

#####################################################################
# Replace writes to opponent_skill with writes to singleplayer_mode #
#####################################################################
# Previous contents:
#	445f4c:	c7 05 30 a4 60 00 00 00 00 00		mov	dword [0x60a430], 0x0	; opponent_skill = 0
patch(0x445f4c,"c7 05 a8 95 4e 00 00 00 00 00");	#mov	dword [0x4e95a8], 0x0	; singleplayer_mode = 0
# Previous contents:
#	445f6a:	c7 05 30 a4 60 00 01 00 00 00		mov	dword [0x60a430], 0x1	; opponent_skill = 1
patch(0x445f6a,"c7 05 a8 95 4e 00 01 00 00 00");	#mov	dword [0x4e95a8], 0x1	; singleplayer_mode = 1
# Previous contents:
#	445f88:	c7 05 30 a4 60 00 02 00 00 00		mov	dword [0x60a430], 0x2	; opponent_skill = 2
patch(0x445f88,"c7 05 a8 95 4e 00 02 00 00 00");	#mov	dword [0x4e95a8], 0x2	; singleplayer_mode = 2
# Previous contents:
#	445fa6:	c7 05 30 a4 60 00 03 00 00 00		mov	dword [0x60a430], 0x3	; opponent_skill = 3
patch(0x445fa6,"c7 05 a8 95 4e 00 03 00 00 00");	#mov	dword [0x4e95a8], 0x3	; singleplayer_mode = 3
# Previous contents:
#	445fb2:	c7 05 30 a4 60 00 00 00 00 00		mov	dword [0x60a430], 0x0	; opponent_skill = 0
patch(0x445fb2,"c7 05 a8 95 4e 00 00 00 00 00");	#mov	dword [0x4e95a8], 0x0	; singleplayer_mode = 0


###################################################
# Remove write to opponent_skill in gauntlet mode #
###################################################
# Previous contents:
# 4ad3e3:	a3 30 a4 60 00		mov	[0x60a430], eax
patch(0x4ad3e3,
      (0x90) x 5);	# nop

######################################################
# Remove write to opponent_skill in sealed deck mode #
######################################################
# Previous contents:
# 4ad796:	a3 30 a4 60 00		mov	[0x60a430], eax
patch(0x4ad796,
      (0x90) x 5);	# nop
