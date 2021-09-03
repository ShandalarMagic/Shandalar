#!/bin/env perl

# Updates Magic.exe in-place to eliminate references to ManalinkEx.dll, either
# directly or (where a function has already been moved into C) by just removing
# the instruction.

use strict;
use warnings;
use Manalink::Patch;

foreach my $addr (0x42e953,	# in charge_mana() - now in C
		  0x42eac9,	# in charge_mana() - now in C
		  0x43549e,	# in get_abilities() - now in C
		  0x4671bc,	# in get_card_or_subtype_name() - now in C
		  0x46a028,	# in load_startduel_assets2() - now in C
		  0x46b71b,	# in load_endduel_assets() - now in C
		  0x440f06,	# in launch_mcu() - now in C
		  0x46a05e,	# in load_startduel_assets2() - now in C
		  0x46a08b,	# in load_startduel_assets2() - now in C
		  0x46a118,	# in load_startduel_assets() - now in C
		  0x46a14e,	# in load_startduel_assets() - now in C
		  0x46a17b,	# in load_startduel_assets() - now in C
		  0x472815,	# in count_colors_of_lands_in_play() - now in C
		  0x479bf7,	# in shuffle() - now in C
		  0x48cbb4,	# in get_special_counters_name() - now in C
		  0x4d4ca9)	# in get_counter_type_by_id() - now in C
  {
    patch("Magic.exe", $addr,
	  (0x90) x 5);
  }

##################################################################################################################################
# Transfer target injection out of dlgproc_GauntletPage() (to call set_challenge()) from ManalinkEx.dll to card_generic_legend() #
##################################################################################################################################
# Previous contents:
# 4486cd:	e9 1c c0 db 00			jmp    0x12046ee
# And in ManalinkEx.dll:
#12046ee:	c7 05 80 02 62 00 00 00 00 00	mov	dword ptr 0x620280, 0x0
#12046f8:	50				push	eax
#12046f9:	e8 4b d6 df 00			call	0x2001d49
#12046fe:	83 c4 04			add	esp, 0x4
#1204701:	90				nop
#1204702:	e9 cb 3f 24 ff			jmp	0x4486d2
patch("Magic.exe", 0x4486cd,
      "e9 fb a9 fb ff");			# jmp	0x4030cd
patch(0x4030cd,	# unused space within what was previously card_generic_legend()
      "c7 05	80 02 62 00	00 00 00 00",	# mov	dword ptr 0x620280, 0x0
      0x50,					# push	eax
      "e8 6c ec bf 01",				# call	0x2001d49		; set_challenge(eax)
      "83 c4 04",				# add	esp, 0x4
      "e9 ed 55 04 00");			# jmp	0x4486d2
#Last used: 0x4030e4

##############################################################
# Move string constant "\LaunchML.exe" out of ManalinkEx.dll #
##############################################################
# Previous contents:
# 440f74:	68 04 49 20 01			push   0x1204904
patch(0x440f74,
      "68 e5 30 40 00");			# push	0x4030e5
patch(0x4030e5,	# unused space within what was previously card_generic_legend()
      string_literal('\LaunchML.exe'),		# db	'\LaunchML.exe',0
      (0x90) x 5);
#Last used: 0x4030f7

###############################################################
# Move smallcard icon drawing injection out of ManalinkEx.dll #
###############################################################
# Previous contents (with context):
# 4d3071:	e8 3a ea f2 ff		call	0x401ab0
# 4d3076:	90			nop
# 4d3077:	90			nop
# 4d3078:	90			nop
# 4d3079:	90			nop
# 4d307a:	90			nop
# 4d307b:	90			nop
# 4d307c:	e9 30 19 d3 00		jmp	0x12049b1
# 4d3081:	90			nop
# 4d3082:	90			nop
# 4d3083:	90			nop
# 4d3084:	90			nop
# 4d3085:	90			nop
# 4d3086:	5e			pop	esi
# And in ManalinkEx.dll:
#12049b1:	90			nop
#12049b2:	90			nop
#12049b3:	90			nop
#12049b4:	90			nop
#12049b5:	90			nop
#12049b6:	90			nop
#12049b7:	90			nop
#12049b8:	90			nop
#12049b9:	90			nop
#12049ba:	90			nop
#12049bb:	8b 85 4c ff ff ff	mov	eax, dword [ebp-0xb4]
#12049c1:	50			push	eax
#12049c2:	8b 45 14		mov	eax, dword [ebp+0x14]
#12049c5:	50			push	eax
#12049c6:	8b 45 10		mov	eax, dword [ebp+0x10]
#12049c9:	50			push	eax
#12049ca:	e8 e6 da df 00		call	0x20024b5
#12049cf:	83 c4 0c		add	esp, 0xc
#12049d2:	90			nop
#12049d3:	90			nop
#12049d4:	89 85 4c ff ff ff	mov	dword [ebp-0xb4], eax
#12049da:	90			nop
#12049db:	90			nop
#12049dc:	90			nop
#12049dd:	90			nop
#12049de:	e9 9e e6 2c ff		jmp	0x4d3081
patch(0x4d3076,
      "e9 7d 00 f3 ff",			# jmp 0x4030f8
      (0x90) x 6);			# nop		; over the old call into ManalinkEx.dll (for clarity only, since we never execute that address)
patch(0x4030f8,	# unused space within what was previously card_generic_legend()
      "8b 85 4c ff ff ff",		# mov	eax, dword [ebp-0xb4]
      0x50,				# push	eax
      "8b 45 14",			# mov	eax, dword [ebp+0x14]
      0x50,				# push	eax
      "8b 45 10",			# mov	eax, dword [ebp+0x10]
      0x50,				# push	eax
      "e8 a9 f3 bf 01",			# call	0x20024b5
      "83 c4 0c",			# add	esp, 0xc
      "89 85 4c ff ff ff",		# mov	dword [ebp-0xb4], eax
      "e9 6c ff 0c 00");		# jmp	0x4d3086
#Last used: 0x403119

###############################################################
# Move smallcard icon tooltip injection out of ManalinkEx.dll #
###############################################################
# Previous contents (with context):
# 489e7c:	e8 2f 7c f7 ff		call	0x401ab0
# 489e81:	e9 7a 61 de 00		jmp	0x1270000
# 489e86:	90			nop
# 489e87:	90			nop
# 489e88:	90			nop
# 489e89:	90			nop
# 489e8a:	90			nop
# 489e8b:	90			nop
# 489e8c:	90			nop
# 489e8d:	90			nop
# 489e8e:	90			nop
# 489e8f:	90			nop
# 489e90:	90			nop
# 489e91:	5e			pop	esi
# And in ManalinkEx.dll:
#1270000:	8b 85 f4 fe ff ff	mov	eax, dword [ebp-0x10c]
#1270006:	50			push	eax
#1270007:	8b 45 f0		mov	eax, dword [ebp-0x10]
#127000a:	50			push	eax
#127000b:	8b 45 f8		mov	eax, dword [ebp-0x8]
#127000e:	50			push	eax
#127000f:	e8 a1 24 d9 00		call	0x20024b5
#1270014:	83 c4 12		add	esp, 0x12
#1270017:	89 85 f4 fe ff ff	mov	dword [ebp-0x10c], eax
#127001d:	e9 6f 9e 21 ff		jmp	0x489e91
patch(0x489e81,
      "e9 94 92 f7 ff");		# jmp 0x40311a
patch(0x40311a,	# unused space within what was previously card_generic_legend() and card_angel_of_mercy()
      "8b 85 f4 fe ff ff",		# mov	eax, dword [ebp-0x10c]
      0x50,				# push	eax
      "8b 45 f0",			# mov	eax, dword [ebp-0x10]
      0x50,				# push	eax
      "8b 45 f8",			# mov	eax, dword [ebp-0x8]
      0x50,				# push	eax
      "e8 87 f3 bf 01",			# call	0x20024b5
      "83 c4 12",			# add	esp, 0x12
      "89 85 f4 fe ff ff",		# mov	dword [ebp-0x10c], eax
      "e9 55 6d 08 00",			# jmp 0x489e91
#Last used: 0x40313b
      (0x90) x 4);
#Last cleared: 0x40313f

###########################################
# Remove injection from setup_shellmenu() #
###########################################
# This injection is actually small enough that it can fit in its original location, so move it back.
# Previous contents:
# 4cea26:	e9 1a 5d d3 00		jmp	0x1204745
# 4cea2b:	68 56 01 00 00		push	0x156
# 4cea30:	6a 62			push	0x62
# 4cea32:	68 17 01 00 00		push	0x117
# 4cea37:	6a 23			push	0x23
# 4cea39:	90			nop
# 4cea3a:	90			nop
# 4cea3b:	90			nop
# 4cea3c:	90			nop
# 4cea3d:	90			nop
# 4cea3e:	90			nop
# 4cea3f:	90			nop
# 4cea40:	90			nop
# 4cea41:	90			nop
# 4cea42:	90			nop
# 4cea43:	90			nop
# 4cea44:	90			nop
# 4cea45:	90			nop
# 4cea46:	90			nop
# 4cea47:	90			nop
# 4cea48:	90			nop
# 4cea49:	90			nop
# 4cea4a:	90			nop
# 4cea4b:	90			nop
# 4cea4c:	90			nop
# 4cea4d:	90			nop
# 4cea4e:	90			nop
# 4cea4f:	90			nop
# 4cea50:	90			nop
# 4cea51:	90			nop
# 4cea52:	90			nop
# 4cea53:	90			nop
# 4cea54:	90			nop
# 4cea55:	90			nop
# 4cea56:	90			nop
# 4cea57:	90			nop
# 4cea58:	90			nop
# 4cea59:	90			nop
# 4cea5a:	90			nop
# 4cea5b:	90			nop
# 4cea5c:	90			nop
# 4cea5d:	90			nop
# 4cea5e:	b8 90 8c 62 00		mov	eax, 0x628c90
# And in ManalinkEx.dll:
#1204745:	68 0d 01 00 00		push	0x10d
#120474a:	6a 6b			push	0x6b
#120474c:	90			nop
#120474d:	90			nop
#120474e:	90			nop
#120474f:	68 ce 00 00 00		push	0xce
#1204754:	6a 2c			push	0x2c
#1204756:	90			nop
#1204757:	90			nop
#1204758:	90			nop
#1204759:	90			nop
#120475a:	90			nop
#120475b:	90			nop
#120475c:	90			nop
#120475d:	68 90 8c 62 00		push	0x628c90
#1204762:	e8 55 13 2d ff		call	0x4d5abc
#1204767:	68 1c ee 4e 00		push	0x4eee1c
#120476c:	6a 00			push	0x0
#120476e:	e8 cd a4 2c ff		call	0x4cec40
#1204773:	83 c4 08		add	esp, 0x8
#1204776:	e9 b0 a2 2c ff		jmp	0x4cea2b
patch(0x4cea26,
      "68 0d 01 00 00",			# push	0x10d
      "6a 6b",				# push	0x6b
      "68 ce 00 00 00",			# push	0xce
      "6a 2c",				# push	0x2c
      "68 90 8c 62 00",			# push	0x628c90
      "e8 7e 70 00 00",			# call	0x4d5abc
      "68 1c ee 4e 00",			# push	0x4eee1c
      "6a 00",				# push	0x0
      "e8 f6 01 00 00",			# call	0x4cec40
      "83 c4 08",			# add	esp, 0x8
      "68 56 01 00 00",			# push	0x156
      "6a 62",				# push	0x62
      "68 17 01 00 00",			# push	0x117
      "6a 23");				# push	0x23
#And 3 nop's to spare.

###################################################
# Retarget other injection from setup_shellmenu() #
###################################################
# This one is, alas, three bytes too long, and far enough removed from the
# previous one (and *another* 3 nop's that follow that a little later) that
# moving everything in between isn't worth the effort.  So put it into
# card_angel_of_mercy as before.
# Previous contents:
# 4cebb6:	e9 c1 5b d3 00		jmp	0x120477c
# 4cebbb:	6a 00			push	0x0
# 4cebbd:	6a 00			push	0x0
# 4cebbf:	68 11 01 00 00		push	0x111
# 4cebc4:	6a 1c			push	0x1c
# 4cebc6:	90			nop
# 4cebc7:	90			nop
# 4cebc8:	90			nop
# 4cebc9:	90			nop
# 4cebca:	90			nop
# 4cebcb:	90			nop
# 4cebcc:	90			nop
# 4cebcd:	90			nop
# 4cebce:	90			nop
# 4cebcf:	90			nop
# 4cebd0:	90			nop
# 4cebd1:	90			nop
# 4cebd2:	90			nop
# 4cebd3:	b8 c0 90 60 00		mov	eax, 0x6090c0
# And in ManalinkEx.dll:
#120477c:	6a 00			push	0x0
#120477e:	6a 00			push	0x0
#1204780:	68 c6 00 00 00		push	0xc6
#1204785:	6a 26			push	0x26
#1204787:	90			nop
#1204788:	90			nop
#1204789:	90			nop
#120478a:	68 c0 90 60 00		push	0x6090c0
#120478f:	e8 28 13 2d ff		call	0x4d5abc
#1204794:	e9 22 a4 2c ff		jmp	0x4cebbb

patch(0x4cebb6,
      "e9 81 45 f3 ff");		# jmp	0x40313c
patch(0x40313c,	# unused space within what was previously card_angel_of_mercy()
      #x40313c:
      "6a 00",				# push	0x0
      #x40313e:
      "6a 00",				# push	0x0
      #x403140:
      "68 c6 00 00 00",			# push	0xc6
      #x403145:
      "6a 26",				# push	0x26
      #x403147:
      "68 c0 90 60 00",			# push	0x6090c0
      #x40314c:
      "e8 6b 29 0d 00",			# call	0x4d5abc
      #x403151:
      "e9 65 ba 0c 00",			# jmp	0x4cebbb
#Last used: #x403155
      (0x90) x 3);
#Last cleared: #x403158
#(Last usable in this function: 0x40319f)
