#!/bin/env perl

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use Text::CSV::Slurp;
use Manalink::CSV;

my $manalink = Text::CSV::Slurp->load(file => 'xManalink.csv', sep_char=>';');

foreach my $ml (@$manalink)
  {
    print "\n";
    dumprow $ml;
  }
