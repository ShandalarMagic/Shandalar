#!/bin/env perl

package Manalink::CSV;

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use Carp qw/croak/;
use Exporter;

use FindBin;

our $VERSION = 1.00;
our @ISA = qw/Exporter/;
our @EXPORT = qw/dumprow isct isml name ml_hdrs ct_hdrs/;

my @ml_hdr = ();
my @ct_hdr = ();

sub init_ml_hdr
{
  scalar @ml_hdr > 0 and return;

  my $mlcsv;
  open $mlcsv, '<', "Manalink.csv"
    or open $mlcsv, '<', "$FindBin::RealBin/Manalink.csv"
    or croak "open Manalink.csv: $!";

  my $raw = <$mlcsv>;
  chomp $raw;
  $raw =~ s/[\n\r]+$//;

  @ml_hdr = grep { $_ =~ /./ && $_ ne '""' } split /;/, $raw;

  close $mlcsv;
}

sub init_ct_hdr
{
  scalar @ct_hdr > 0 and return;

  my $ctcsv;
  open $ctcsv, '<', "ct_all.csv"
    or open $ctcsv, '<', "$FindBin::RealBin/ct_all.csv"
    or croak "open ct_all.csv: $!";

  my $raw = <$ctcsv>;
  chomp $raw;
  $raw =~ s/[\n\r]+$//;

  @ct_hdr = grep { $_ =~ /./ && $_ ne '""' } split /;/, $raw;

  close $ctcsv;
}

sub isml
{
  my $r = shift;
  exists $r->{'ID'} && defined $r->{'ID'} && $r->{'ID'} ne '';
}

sub isct
{
  my $r = shift;
  exists $r->{'Full Card Name in CSV (must be exactly the same)'}
    && defined $r->{'Full Card Name in CSV (must be exactly the same)'}
    && $r->{'Full Card Name in CSV (must be exactly the same)'} ne '';
}

sub ml_hdrs
{
  init_ml_hdr();
  my @copy = @ml_hdr;
  return @copy;
}

sub ct_hdrs
{
  init_ct_hdr();
  my @copy = @ct_hdr;
  return @copy;
}

use Data::Dumper;

my @colortest_txt =
  (
   [ 1, "X"],
   [ 2, "B"],
   [ 4, "U"],
   [ 8, "G"],
   [16, "R"],
   [32, "W"],
   [64, "A"],
  );

sub dumpml
{
  my $ml = shift;

  init_ml_hdr();

  foreach my $key (@ml_hdr)
    {
      my $val = $ml->{$key};
      if ($val !~ /^\s*[-0]?\s*$/)
	{
	  if ($key eq 'Mana Source Colors')
	    {
	      my $txt = '';
	      my $v = $val;
	      foreach (@colortest_txt)
		{
		  if ($v & $_->[0])
		    {
		      $txt .= $_->[1];
		      $v &= ~($_->[0]);
		    }
		}
	      if ($v)
		{
		  $txt .= "|" . $v;
		}
	      say "$key => $val ($txt)";
	    }
	  else
	    {
	      say "$key => $val";
	    }
	}
    }
}

sub dumpct
{
  my $ct = shift;

  init_ct_hdr();

  foreach my $key (@ct_hdr)
    {
      my $val = $ct->{$key};
      $val =~ /^\s*0?\s*$/ or say "$key => $val";
    }
}

sub dumprow
{
  if (isml $_[0])
    {
      dumpml $_[0];
    }
  else
    {
      dumpct $_[0];
    }
}

sub name
{
  my $r = shift;
  (isml $r) ? $r->{'Full Name'} : $r->{'Full Card Name in CSV (must be exactly the same)'};
}

1;
