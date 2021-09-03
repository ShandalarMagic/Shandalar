#!/bin/env perl

# Updates Deckdll.dll in-place.
# 1. Flags the AUTO segment of Deckdll.dll as writeable (so we can fix pointers at runtime from Shandalar.dll)

use strict;
use warnings;
use Manalink::Patch;

#############
# in header #
#############
# Previous contents:
#194:		20 00 00 60
patch("RAW", "Deckdll.dll", 0x194,
      "20 00 00 e0");
