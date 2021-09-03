#!/bin/env perl

#Usage:
#1. create .git/info/attributes and add the following line (without the #)
#/magic_updater/Manalink.csv	diff=parse-ml
#
#2. run "git config diff.parse-ml.textconv parse-ml"
#
#3. run "git config diff.parse-ml.xfuncname '^Full Name => (.*)'"
#
#4. optionally, run "git config diff.parse-ml.cachetextconv true"
#
#5. make a symlink from something/in/your/path/parse-ml to this script

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use FindBin;
use lib "$FindBin::RealBin/.";

use Text::CSV::Slurp;
use Manalink::CSV;

my $manalink = Text::CSV::Slurp->load(file => $ARGV[0], sep_char=>';');

foreach my $ml (@$manalink)
  {
    print "\n";
    dumprow $ml;
  }
