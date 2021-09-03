#!/bin/env perl

# Updates Magic.exe in-place.
# 1. Hooks the only call to dlgproc_do_dialog to call dlgproc_do_dialog_hook() in C.

use strict;
use warnings;
use Manalink::Patch;

#########################################################################################
# Replace function address of dlgproc_do_dialog with 200b489 (dlgproc_do_dialog_hook()) #
#########################################################################################
# Previous contents:
# 471bfb:	68 90 a0 4a 00		push	0x4aa090	; arg4 = &dlgproc_do_dialog

patch("Magic.exe", 0x471bfb,
      "68 89 b4 00 02");	# push 0x200b489
