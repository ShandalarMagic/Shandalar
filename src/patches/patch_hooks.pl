#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Hooks various functions, mostly UI-related and preferably seldom-referred-to, to call hooks in C.

use strict;
use warnings;
use Manalink::Patch;

###############################################################################################################
# register_CardClass(): Replace function address of wndproc_CardClass with 200bb50 (wndproc_CardClass_hook()) #
###############################################################################################################
# Previous contents:
#489777:	c7 45 dc 60 9d 48 00	mov	dword [ebp-0x24], 0x489d60	; WndClass.lpfnWndProc = wndproc_CardClass;
patch("Magic.exe", 0x489777,
      "c7 45 dc 50 bb 00 02");		# mov	dword [ebp-0x24], 0x200bb50	; WndClass.lpfnWndProc = wndproc_CardClass_hook;

##############################################################################################
# wndproc_BigCardClass(): Replace calls to wndproc_CardClass() with wndproc_CardClass_hook() #
##############################################################################################
#4aaca4:	68 60 9d 48 00		push	0x489d60	; arg1 = wndproc_CardClass
patch("Magic.exe", 0x4aaca4,
      "68 50 bb 00 02");		# push	0x200bb50	; arg1 = wndproc_CardClass_hook

##############################################################################################################
# register_MainClass(): Replace function address of wndproc_MainClass with 200ee59 (wndproc_Mainclass_hook() #
##############################################################################################################
#493340:	c7 45 dc c0 33 49 00	mov	dword [ebp-0x24], 0x4933c0	; WndClass.lpfnWndProc = wndproc_MainClass
patch("Magic.exe", 0x493340,
      "c7 45 dc 59 ee 00 02");		# mov	dword [ebp-0x24], 0x200ee59	; WndClass.lpfnWndProc = wndproc_MainClass_hook
