#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Moves various functions entirely into C:
# get_protections_from()
# resolve_trigger()
# is_legal_block()/is_legal_block2()
# is_legal_block_impl()
# count_colors_of_lands_in_play()
# backup_data_for_ai()
# restore_data_for_ai()
# backup_data_for_ai_0()
# restore_data_for_ai_0()
# phase_changed()
# can_attack()
# check_destroys_if_blocked()
# ante_top_card_of_library()
# get_internal_card_id_from_csv_id() and CardTypeFromID()
# damage_creature()
# format_manacost_into_global_allpurpose_buffer()
# draw_a_card()
# ai_assign_blockers()
# get_card_image_number()
# dispatch_event_raw()
# human_assign_blockers()
# tap_card()
# mana_burn()
# get_counter_type_by_id()
# get_special_counters_name()
# upkeep_phase()
# ai_decision_phase()
# put_card_or_activation_onto_stack()
# recopy_card_onto_stack()
# finalize_activation()
# card_holy_armor()
# card_blessing()
# put_card_on_stack3()
# compute_and_check_casting_cost()
# card_instances_should_be_displayed_identically()
# raw_put_card_in_graveyard()
# remove_card_from_grave()
# current_event_or_trigger_is_forced()
# kill_card()
# get_card_or_subtype_name()
# play_sound_effect()
# update_hand_window()

use strict;
use warnings;
use Manalink::Patch;

########################################################################
# Replace entire function with a jmp to 200b281 (get_protections_from) #
########################################################################
# Previous contents:
# 401e20:	55			push	ebp
# 401e21:	8b ec			mov	ebp, esp
# 401e23:	83 ec 08		sub	esp, 0x8

patch("Magic.exe", 0x401e20,
      "e9 5c 94 c0 01",	# jmp 0x0200b281
      0x90);		# nop	(so the following code is still aligned)

###################################################################
# Replace entire function with a jmp to 200b286 (resolve_trigger) #
###################################################################
# Previous contents:
# 434800:	55			push   ebp
# 434801:	8b ec			mov    ebp, esp
# 434803:	81 ec 24 03 00 00	sub    esp, 0x324
patch(0x434800,
      "e9 81 6a bd 01",	# jmp 0x0200b286
      (0x90) x 4);	# nop	(four, so the following code is still aligned)

##################################################################
# Replace entire function with a jmp to 200b394 (is_legal_block) #
##################################################################
# Previous contents:
# 434f30:	55			push	ebp
# 434f31:	8b ec			mov	ebp, esp
# 434f33:	83 ec 10		sub	esp, 0x10
patch(0x434f30,
      "e9 5f 64 bd 01",	# jmp 0x200b394
      0x90);		# nop

##################################################################
# Replace entire function with a jmp to 200b394 (is_legal_block) #
##################################################################
# Previous contents:
# 4b9220:	55			push	ebp
# 4b9221:	8b ec			mov	ebp, esp
# 4b9223:	83 ec 10		sub	esp, 0x10
patch(0x4b9220,
      "e9 6f 21 b5 01",	# jmp 0x200b394
      0x90);		# nop

#######################################################################
# Replace entire function with a jmp to 200b399 (is_legal_block_impl) #
#######################################################################
# Previous contents:
# 434f90:	55			push	ebp
# 434f91:	8b ec			mov	ebp, esp
# 434f93:	83 ec 08		sub	esp, 0x8
patch(0x434f90,
      "e9 04 64 bd 01",	# jmp 0x200b399
      0x90);		# nop

#################################################################################
# Replace entire function with a jmp to 200b39e (count_colors_of_lands_in_play) #
#################################################################################
# Previous contents:
# 4725c0:	55			push	ebp
# 4725c1:	8b ec			mov	ebp, esp
# 4725c3:	83 ec 64		sub	esp, 0x64
patch(0x4725c0,
      "e9 d9 8d b9 01",	# jmp 0x200b39e
      0x90);		# nop

######################################################################
# Replace entire function with a jmp to 200b3da (backup_data_for_ai) #
######################################################################
# 498e30:	56			push	esi
# 498e31:	57			push	edi
# 498e32:	be 70 f1 4e 00		mov	esi, 0x4ef170
patch(0x498e30,
      "e9 a5 25 b7 01",	# jmp 0x200b3da
      (0x90) x 2);	# nop, nop

#######################################################################
# Replace entire function with a jmp to 200b3df (restore_data_for_ai) #
#######################################################################
# 498e70:	56			push	esi
# 498e71:	57			push	edi
# 498e72:	bf 70 f1 4e 00		mov	edi, 0x4ef170
patch(0x498e70,
      "e9 6a 25 b7 01",	# jmp 0x200b3df
      (0x90) x 2);	# nop, nop

########################################################################
# Replace entire function with a jmp to 200b3e4 (backup_data_for_ai_0) #
########################################################################
# 498d90:	56			push	esi
# 498d91:	57			push	edi
# 498d92:	be 70 f1 4e 00		mov	esi, 0x4ef170
patch(0x498d90,
      "e9 4f 26 b7 01",	# jmp 0x200b3e4
      (0x90) x 2);	# nop, nop

#########################################################################
# Replace entire function with a jmp to 200b3e9 (restore_data_for_ai_0) #
#########################################################################
# 498df0:	56			push	esi
# 498df1:	57			push	edi
# 498df2:	bf 70 f1 4e 00		mov	edi, 0x4ef170
patch(0x498df0,
      "e9 f4 25 b7 01",	# jmp 0x200b3e9
      (0x90) x 2);	# nop, nop

#################################################################
# Replace entire function with a jmp to 200b452 (phase_changed) #
#################################################################
# 435f20:	55			push	ebp
# 435f21:	8b ec			mov	ebp, esp
# 435f23:	83 3d 74 85 72 00 01	cmp	dword [0x728574], 0x1
patch(0x435f20,
      "e9 2d 55 bd 01",	# jmp 0x200b452
      (0x90) x 5);	# nop

##############################################################
# Replace entire function with a jmp to 200b42f (can_attack) #
##############################################################
# 434c30:	55			push	ebp
# 434c31:	8b ec			mov	ebp, esp
# 434c33:	83 ec 18		sub	esp, 0x18
patch(0x434c30,
      "e9 fa 67 bd 01",	# jmp 0x200b42f
      0x90);		# nop

#############################################################################
# Replace entire function with a jmp to 200b470 (check_destroys_if_blocked) #
#############################################################################
# 437700:	55			push   ebp
# 437701:	8b ec			mov    ebp, esp
# 437703:	83 ec 04		sub    esp, 0x4
patch(0x437700,
      "e9 6b 3d bd 01",	# jmp 0x200b470
      0x90);		# nop

############################################################################
# Replace entire function with a jmp to 200b484 (ante_top_card_of_library) #
############################################################################
# 458070:	55			push   ebp
# 458071:	8b ec			mov    ebp, esp
# 458073:	83 ec 08		sub    esp, 0x8

patch(0x458070,
      "e9 0f 34 bb 01",	# jmp 0x200b484
      0x90);		# nop

################################################################
# Replace entire function with a jmp to 2007127 (after_damage) #
################################################################
#477340:	55			push	ebp
#477341:	8b ec			mov	ebp, esp
#477343:	83 ec 08		sub	esp, 0x8

patch(0x477340,
      "e9 e2 fd b8 01",	# jmp 0x2007127
      0x90);		# nop

###################
# get_abilities() #
###################
#4352d0:	55			push	ebp
#4352d1:	8b ec			mov	ebp, esp
#4352d3:	83 ec 28		sub	esp, 0x28
patch(0x4352d0,
      "e9 86 c2 bc 01",	# jmp 0x200155b
      0x90);		# nop

######################################
# get_internal_card_id_from_csv_id() #
######################################
#479df0:	55			push	ebp
#479df1:	8b ec			mov	ebp, esp
#479df3:	56			push	esi
#479df4:	8b 55 08		mov	edx, dword [ebp+0x8]
patch(0x479df0,
      "e9 87 19 b9 01",	# jmp 0x200b77c
      (0x90) x 2);	# nop

####################
# CardTypeFromID() #
####################
#471c70:	8b 4c 24 04		mov	ecx, dword [esp+0x4]
#471c74:	53			push	ebx
patch(0x471c70,
      "e9 07 9b b9 01");	# jmp 0x200b77c

#####################
# damage_creature() #
#####################
#4a66c0:	55		push	ebp
#4a66c1:	8b ec		mov	ebp, esp
#4a66c3:	83 ec 0c	sub	esp, 0xc
patch(0x4a66c0,
      "e9 2f 51 b6 01",	# jmp 0x200b7f4
      0x90);

###################################################
# format_manacost_into_global_allpurpose_buffer() #
###################################################
#42f1f0:	55			push	ebp
#42f1f1:	8b ec			mov	ebp, esp
#42f1f3:	81 ec b8 01 00 00	sub	esp, 0x1b8
patch(0x42f1f0,
      "e9 a9 53 bd 01",	# jmp 0x200459e
      (0x90) x 4);

#################
# draw_a_card() #
#################
#433190:	55		push	ebp
#433191:	8b ec		mov	ebp, esp
#433193:	83 ec 10	sub	esp, 0x10
patch(0x433190,
      "e9 93 f8 bc 01",	# jmp 0x2002a28
      0x90);

########################
# ai_assign_blockers() #
########################
#4b18e0:	55		push	ebp
#4b18e1:	8b ec		mov	ebp, esp
#4b18e3:	83 ec 08	sub	esp, 0x8
patch(0x4b18e0,
      "e9 96 ff b4 01",	# jmp 0x200187b
      0x90);

###########################
# get_card_image_number() #
###########################
#468160:	55		push   ebp
#468161:	8b ec		mov    ebp, esp
#468163:	83 7d 08 ff	cmp    dword [ebp+0x8], -1
patch(0x468160,
      "e9 53 9b b9 01",	# jmp 0x2001cb8
      (0x90) x 2);

########################
# dispatch_event_raw() #
########################
#435a40:	55		push	ebp
#435a41:	8b ec		mov	ebp, esp
#435a43:	83 ec 14	sub	esp, 0x14
patch(0x435a40,
      "e9 98 b6 bc 01",	# jmp 0x20010dd
      0x90);

###########################
# human_assign_blockers() #
###########################
#434960:	55			push	ebp
#434961:	8b ec			mov	ebp, esp
#434963:	81 ec b4 00 00 00	sub	esp, 0xb4
patch(0x434960,
      "e9 b6 6f bd 01",	# jmp 0x200b91b
      (0x90) x 4);

##############
# tap_card() #
##############
#4a3170:	55		push	ebp
#4a3171:	8b ec		mov	ebp, esp
#4a3173:	56		push	esi
#4a3174:	8b 45 08	mov	eax, dword [ebp+0x8]
patch(0x4a3170,
      "e9 ab 87 b6 01",	# jmp 0x200b920
      (0x90) x 2);

###############
# mana_burn() #
###############
#43a060:	55		push	ebp
#43a061:	8b ec		mov	ebp, esp
#43a063:	83 ec 08	sub	esp, 0x8
patch(0x43a060,
      "e9 d9 18 bd 01",	# jmp 0x200b93e
      0x90);

############################
# get_counter_type_by_id() #
############################
# (This was already injected into to call the C version, but it jumped through hoops - not completely successfully - to keep existing cases in the exe.)
#4d4ca0:	55		push	ebp
#4d4ca1:	8b ec		mov	ebp, esp
#4d4ca3:	83 ec 04	sub	esp, 0x4
patch(0x4d4ca0,
      "e9 d1 d0 b2 01",	# jmp 0x2001d76
      0x90);

###############################
# get_special_counters_name() #
###############################
# This was already injected into to call get_counter_type_by_id() (!) in C, but it jumped through hoops to keep existing cases in the exe.  The C call was
# completely nonfunctional.
#48cb80:	55	push	ebp
#48cb81:	8b ec	mov	ebp, esp
#48cb83:	53	push	ebx
#48cb84:	56	push	esi
patch(0x48cb80,
      "e9 f5 ed b7 01");	# jmp 0x200b97a

################
# upkeep_phase #
################
#43acf0:	55		push	ebp
#43acf1:	8b ec		mov	ebp, esp
#43acf3:	83 ec 68	sub	esp, 0x68
patch(0x43acf0,
      "e9 25 0d bd 01",	# jmp 0x200ba1a
      0x90);

#####################
# ai_decision_phase #
#####################
#43d590:	55			push	ebp
#43d591:	8b ec			mov	ebp, esp
#43d593:	81 ec 28 01 00 00	sub	esp, 0x128
patch(0x43d590,
      "e9 8a e4 bc 01",	# jmp 0x200ba1f
      (0x90) x 4);

#####################################
# put_card_or_activation_onto_stack #
#####################################
#436550:	55		push	ebp
#436551:	8b ec		mov	ebp, esp
#436553:	83 ec 0c	sub	esp, 0xc
patch(0x436550,
      "e9 fc 54 bd 01",	# jmp 0x200ba51
      0x90);

##########################
# recopy_card_onto_stack #
##########################
#436450:	55		push	ebp
#436451:	8b ec		mov	ebp, esp
#436453:	83 ec 18	sub	esp, 0x18
patch(0x436450,
      "e9 01 56 bd 01",	# jmp 0x200ba56
      0x90);

#######################
# finalize_activation #
#######################
#4346e0:	55		push	ebp
#4346e1:	8b ec		mov	ebp, esp
#4346e3:	83 ec 10	sub	esp, 0x10
patch(0x4346e0,
      "e9 76 73 bd 01",	# jmp 0x200ba5b
      0x90);

#############################
# is_a_tappable_mana_source #
#############################
#43a280:	55		push	ebp
#43a281:	8b ec		mov	ebp, esp
#43a283:	56		push	esi
#43a284:	8b 45 08	mov	eax, dword [ebp+0x8]
patch(0x43a280,
      "e9 e0 17 bd 01",	# jmp 0x200ba65
      (0x90) x 2);

#####################
# tap_card_for_mana #
#####################
#43a2d0:	55					push	ebp
#43a2d1:	8b ec					mov	ebp, esp
#43a2d3:	56					push	esi
#43a2d4:	c7 05 68 2b 7a 00 3e 00 00 00		mov	dword [0x7a2b68], 0x3e
patch(0x43a2d0,
      "e9 95 17 bd 01",	# jmp 0x200ba6a
      (0x90) x 9);

###################
# card_holy_armor #
###################
#Jmp to the C implementation instead of setting in ct_all.csv, since 0x4af110() checks for it by address.
#4b9910:	55		push	ebp
#4b9911:	8b ec		mov	ebp, esp
#4b9913:	83 ec 04	sub	esp, 0x4
patch(0x4b9910,
      "e9 a5 21 b5 01",	# jmp 0x200baba
      0x90);

#################
# card_blessing #
#################
#Jmp to the C implementation instead of setting in ct_all.csv, since 0x4af110() checks for it by address.
#4b9d20:	55		push	ebp
#4b9d21:	8b ec		mov	ebp, esp
#4b9d23:	83 ec 04	sub	esp, 0x4
patch(0x4b9d20,
      "e9 9a 1d b5 01",	# jmp 0x200babf
      0x90);

######################
# put_card_on_stack3 #
######################
#433e10:	55		push   ebp
#433e11:	8b ec		mov    ebp, esp
#433e13:	83 ec 10	sub    esp, 0x10
patch(0x433e10,
      "e9 be 7c bd 01",	# jmp 0x200bad3
      0x90);

##################################
# compute_and_check_casting_cost #
##################################
#402930:	55			push	ebp
#402931:	8b ec			mov	ebp, esp
#402933:	ff 35 5c 82 73 00	push	dword [0x73825c]
patch(0x402930,
      "e9 df 91 c0 01",	# jmp 0x200bb14
      (0x90) x 4);

##################################################
# card_instances_should_be_displayed_identically #
##################################################
#48c9d0:	55		push	ebp
#48c9d1:	8b ec		mov	ebp, esp
#48c9d3:	83 ec 04	sub	esp, 0x4
patch(0x48c9d0,
      "e9 44 f1 b7 01",	# jmp 0x200bb19
      0x90);

#############################
# raw_put_card_in_graveyard #
#############################
#477ec0:	55		push	ebp
#477ec1:	8b ec		mov	ebp, esp
#477ec3:	83 ec 0c	sub	esp, 0xc
patch(0x477ec0,
      "e9 63 3c b9 01",	# jmp 0x200bb28
      0x90);

##########################
# remove_card_from_grave #
##########################
#477f30:	55		push	ebp
#477f31:	8b ec		mov	ebp, esp
#477f33:	8b 55 0c	mov	edx, DWORD PTR [ebp+0xc]
patch(0x477f30,
      "e9 f8 3b b9 01",	# jmp 0x200bb2d
      0x90);

######################################
# current_event_or_trigger_is_forced #
######################################
#476f10:	55		push	ebp
#476f11:	8b ec		mov	ebp, esp
#476f13:	a1 e8 bc 62 00	mov	eax, dword [0x62bce8]
patch(0x476f10,
      "e9 22 4c b9 01",	# jmp 0x200bb37
      (0x90) x 3);

#############
# kill_card #
#############
#4779f0:	55		push	ebp
#4779f1:	8b ec		mov	ebp, esp
#4779f3:	83 ec 04	sub	esp, 0x4
patch(0x4779f0,
      "e9 74 41 b9 01",	# jmp 0x200bb69
      0x90);

############################
# get_card_or_subtype_name #
############################
#4671a0:	55		push   ebp
#4671a1:	8b ec		mov    ebp, esp
#4671a3:	83 7d 08 00	cmp    dword [ebp+0x8], 0x0
patch(0x4671a0,
      "e9 a9 ab b9 01",	# jmp 0x2001d4e
      (0x90) x 2);

#####################
# play_sound_effect #
#####################
#435f50:	55			push   ebp
#435f51:	8b ec			mov    ebp, esp
#435f53:	81 ec 30 01 00 00	sub    esp, 0x130
patch(0x435f50,
      "e9 ed 82 bd 01",	# jmp 0x200e242
      (0x90) x 4);

######################
# update_hand_window #
######################
#47d2f0:	55			push	ebp
#47d2f1:	8b ec			mov	ebp, esp
#47d2f3:	83 ec 54		sub	esp, 0x54
patch(0x47d2f0,
      "e9 57 0f b9 01",	# jmp 0x200e24c
      0x90);
