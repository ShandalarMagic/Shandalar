#!/bin/env perl

#Usage:
#1. create .git/info/attributes and add the following line (without the #)
#/magic_updater/ct_all.csv	diff=parse-ct
#
#2. run "git config diff.parse-ct.textconv parse-ct"
#
#3. run "git config diff.parse-ct.xfuncname '^Full Card Name in CSV \(must be exactly the same\) => (.*)'"
#
#4. optionally, run "git config diff.parse-ct.cachetextconv true"
#
#5. make a symlink from something/in/your/path/parse-ct to this script

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use FindBin;
use lib "$FindBin::RealBin/.";

use Text::CSV::Slurp;
use Manalink::CSV;

my $ct_all = Text::CSV::Slurp->load(file => $ARGV[0], sep_char=>';');

foreach my $ct (@$ct_all)
  {
    print "\n";
    dumprow $ct;
  }
