#!/bin/env perl

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use Text::CSV::Slurp;
use Manalink::CSV;

my $ct_all = Text::CSV::Slurp->load(file => 'xct_all.csv', sep_char=>';');

foreach my $ct (@$ct_all)
  {
    print "\n";
    dumprow $ct;
  }
