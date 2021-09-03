#!/bin/env perl

# Updates Magic.exe in-place.
# Replaces smallcard size calculation with a call into C.

use strict;
use warnings;
use Manalink::Patch;

########################################################################
# Replace entire function with a jmp to 200b281 (get_protections_from) #
########################################################################
# Previous contents:
# 494d3c:	99		cdq
# 494d3d:	83 e2 07	and	edx, 0x7
# 494d40:	03 c2		add	eax, edx
# 494d42:	c1 f8 03	sar	eax, 0x3
# 494d45:	a3 f8 86 72 00	mov	dword ptr [0x7286f8], eax

patch("Magic.exe", 0x494d3c,
      0x50,		# push	eax		; arg1 = client_rect.right
      "e8 05 95 b7 01",	# call	0x200e247	; eax = set_smallcard_size(client_rect.right)
      "83 c4 04",	# add	esp, 0x4	; pop 1 arg off stack
      "eb 12",		# jmp	0x494d59	; skip rest of previous calculation
      (0x90) x 3);
