#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Change masking of iids with 0x3FFF to 0x7FFF, to increase the maximum representable iid from 16382 to 32766.
# 2. Change setting, removing, and checking bit 0x4000 in iids to 0x80000
# 2a. Move sub_495BE0() into C

use strict;
use warnings;
use Manalink::Patch;

###########################
# In show_deck_back_end() #
###########################
# Previous contents:
# 46c55f:	25 ff 3f 00 00		and	eax, 0x3fff			; eax &= 0x3fff
patch("Magic.exe",
      0x46c55f,"25 ff 7f 00 00");	# and	eax, 0x7fff			; eax &= 0x7fff

#######################
# In CardIDFromType() #
#######################
# Previous contents:
# 471cc0:	81 65 08 ff 3f 00 00	and	dword [ebp+0x8], 0x3fff		; param1 &= 0x3fff
patch(0x471cc0,"81 65 08 ff 7f 00 00");	# and	dword [ebp+0x8], 0x7fff		; param1 &= 0x7fff

###################
# In CardInDeck() #
###################
# Previous contents:
# 471cf3:	25 00 40 00 00		and	eax, 0x4000			; eax &= 0x4000
patch(0x471cf3,"25 00 00 08 00");	# and	eax, 0x80000			; eax &= 0x80000

######################
# In SetCardInDeck() #
######################
# Previous contents:
# 471d0c:	81 0c 85 8c 58 7a 00 00 40 00 00	or	dword [eax*4+0x7a588c], 0x4000	; deck[eax] |= 0x4000
patch(0x471d0c,"81 0c 85 8c 58 7a 00 00 00 08 00");	# or	dword [eax*4+0x7a588c], 0x80000	; deck[eax] |= 0x80000
# Previous contents:
# 471d1c:	81 24 85 8c 58 7a 00 ff bf 00 00	and	dword [eax*4+0x7a588c],0xbfff	; deck[eax] &= (0xffff & ~0x4000)
patch(0x471d1c,"81 24 85 8c 58 7a 00 ff ff 00 00");	# and	dword [eax*4+0x7a588c],0xffff	; deck[eax] &= 0xffff

####################################
# In insert_iid_into_deck_sorted() #
####################################
# Previous contents:
# 479a5c:	25 ff 3f 00 00		and	eax, 0x3fff			; eax &= 0x3fff
patch(0x479a5c,"25 ff 7f 00 00");	# and	eax, 0x7fff			; eax &= 0x7fff

#############################
# In remove_iid_from_deck() #
#############################
# Previous contents:
# 479b03:	25 ff 3f 00 00		and	eax, 0x3fff			; eax &= 0x3fff
patch(0x479b03,"25 ff 7f 00 00");	# and	eax, 0x7fff			; eax &= 0x7fff

#######################################
# Move sub_495BE0 into C at 0x200ec2e #
#######################################
# Previous contents:
# 495be0:	55			push	ebp
# 495be1:	8b ec			mov	ebp, esp
# 495be3:	83 ec 2c		sub	esp, 0x2c
jmp_to(0x495be0 => 0x200ec2e);
