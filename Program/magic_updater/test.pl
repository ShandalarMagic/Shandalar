#!/bin/env perl

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use Text::CSV::Slurp;
use Manalink::CSV;

say "Reading Manalink.csv";
my $manalink = Text::CSV::Slurp->load(file => 'Manalink.csv', sep_char=>';');

say "Reading ct_all.csv";
my $ct_all = Text::CSV::Slurp->load(file => 'ct_all.csv', sep_char=>';');

my %manalink_names;
foreach my $ml (@$manalink)
  {
    $manalink_names{$ml->{'Full Name'}} = $ml;
  }

say "";

foreach my $ct (@$ct_all)
  {
    $ct->{'Sort order'} eq '#' and next;
    my $nam = name $ct;
    $nam =~ /^#/ and next;

    my $ml = $manalink_names{$nam};
    if (!defined $ml)
      {
	say "\nCouldn't find \$manalink_names{'$nam'}";
	next;
      }

    # Planeswalkers: must have Paid Attack, Activate, either Activate-before-Combat or Activate-after-Combat, and not Declare Attack.

    if ($ml->{'Type Text'} =~ /laneswalker/ && $nam !~ /^Gideon.*Animated$/)
      {
	$ct->{'Flags: Declare Attack'} eq '0' or say "$nam: Planeswalker has Declare Attack";
	$ct->{'Flags: Paid Attack'} eq '1' or say "$nam: Planeswalker doesn't have Paid Attack";
	$ct->{'Flags: Activate'} eq '1' or say "$nam: Planeswalker doesn't have Activate";
	$ct->{'Activate before Combat'} eq '1' || $ct->{'Activate after Combat'} eq '1' or say "$nam: Planeswalker doesn't have an activate time";
      }

    # Legendary: must have a C implementation
    if ($ml->{'Type Text'} =~ /legend/i)
      {
	$ct->{'Code Address'} =~ /^0x0*200[0-9a-fA-F]{4}$/ or say "$nam: Legendary without C implementation";
	$ct->{'Family'} =~ /^1[45]$/ or say "$nam: Legendary without Family of 14 or 15 (", $ct->{'Family'}, ')';
      }

    # Morph: must have Modifies Mana Cost
    if ($ml->{'Rules Text'} =~ /(^|\\n)Morph\b/)
      {
	$ct->{'Modifies Casting Cost'} eq '1' or say "$nam: Morph but not Modifies Casting Cost";
      }

    # Bestow: must have Modifies Mana Cost
    if ($ml->{'Rules Text'} =~ /(^|\\n)Bestow\b/ && $nam !~ / Bestowed$/)
      {
	$ct->{'Modifies Casting Cost'} eq '1' or say "$nam: Bestow but not Modifies Casting Cost";
      }

    # 2-color Hybrid: must have Modifies Mana Cost, at least two colors
    if ($ml->{'Cost Text'} =~ /\|[WUBGR][WUBGR]/)
      {
	$ct->{'Modifies Casting Cost'} eq '1' or say "$nam: Hybrid but not Modifies Casting Cost";
	my $clrs = 0;
	($ct->{'Color '.$_} eq '1') && ++$clrs foreach (qw/White Red Green Blue Black/);
	$clrs >= 2 or say "$nam: Hybrid but only $clrs colors";
      }

    # 1-color Hybrid: must have Modifies Mana Cost
    if ($ml->{'Cost Text'} =~ /\|2[WUBGR]/)
      {
	$ct->{'Modifies Casting Cost'} eq '1' or say "$nam: Monocolor hybrid but not Modifies Casting Cost";
      }

    # Phyrexian Mana: must have Modifies Mana Cost
    if ($ml->{'Cost Text'} =~ /\|P[WUBGR]/)
      {
	$ct->{'Modifies Casting Cost'} eq '1' or say "$nam: Phyrexian mana but not Modifies Casting Cost";
      }

    # Color Unused shouldn't be used on anything.  Color ArtMana and Color Colorless should only be used on mana producers.
    $ct->{'Color Unused'} eq '0' or say "$nam: has Color Unused set";
    if ($ct->{'Flags: Mana Source'} ne '1')
      {
	$ct->{'Color ArtMana'} eq '0' or say "$nam: not a mana source but has Color ArtMana set";
	#Disabled; this is improperly set on almost everything that's not colored
	#$ct->{'Color Colorless'} eq '0') or say "$nam: not a mana source but has Color Colorless set";
      }

    # Colored artifacts: should not have Modifies Casting Cost anymore, with a few exceptions
    if ($ml->{'Type Text'} =~ /artifact/i && $ml->{'Cost Text'} =~ /\|[WUBGR]/
	&& !($ml->{'Cost Text'} =~ /\|[WUBGR][WUBGR]/	# Hybrid
	     || $ml->{'Cost Text'} =~ /\|P[WUBGR]/	# Phyrexian mana
	     || $nam eq "Salvage Titan"
	     || $nam eq "Esper Stormblade"	# also hybrid
	     || $nam =~ /^(Fieldmist|Firewild|Mistvein|Veinfire|Wildfield) Borderpost$/))
      {
	$ct->{'Modifies Casting Cost'} eq '0' or say "$nam: Colored artifact still has Modifies Casting Cost";
      }
  }
