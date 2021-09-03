#!/bin/env perl

package Manalink::Patch;

use strict;
use warnings;
use Carp qw/croak/;
use Exporter;
use Scalar::Util qw/looks_like_number/;

our $VERSION = 1.00;
our @ISA = qw/Exporter/;
our @EXPORT = qw/patch string_literal/;

my $last_filename = undef;
my $last_raw = 0;
my $filename;

my %offsets =
  (
   "raw" => 0,
   "magic.exe" => 0x400000,
   "manalinkex.dll" => 0x1000c00,
   "shandalar.exe" => 0x400c00
  );

sub string_literal
{
  my $rval = '';
  foreach (split(//, $_[0]))
    {
      $rval .= sprintf('%02x ', ord($_));
    }
  $rval .= '00';	# trailing nul
  return $rval;
}

sub patch
{
  my ($filename, $seekpos, $raw, $lc_filename);
  $filename = shift;

  if ($filename =~ /^raw$/i)
    {
      $filename = shift;
      $seekpos = shift;
      $last_filename = $filename;
      $last_raw = 1;
      $lc_filename = 'raw';
    }
  elsif (looks_like_number $filename)
    {
      $seekpos = $filename;
      defined $last_filename or croak "No filename specified";
      $filename = $last_filename;
      $lc_filename = $last_raw ? 'raw' : lc $filename;
    }
  else
    {
      $seekpos = shift;
      $last_filename = $filename;
      $last_raw = 0;
      $lc_filename = lc $filename;
    }

  exists $offsets{$lc_filename} or croak qq/Unknown target file "$filename"/;

  $seekpos >= $offsets{$lc_filename} or croak sprintf qq/Seekpos $seekpos is less than $filename's base offset %x/, $offsets{$lc_filename};
  $seekpos -= $offsets{$lc_filename};

  my $f;
  open($f, "+<", $filename) or croak "Couldn't open $filename: $!";

  binmode($f);

  seek $f, $seekpos, 0 or croak(sprintf "Couldn't seek to %x in $filename: %s", $seekpos, $?);

  my @args;
  foreach my $b (@_)
    {
      if (looks_like_number $b)
	{
	  push @args, $b;
	}
      elsif ($b =~ /^\s*((0x)?[0-9a-f]{2}|0)(\s+((0x)?[0-9a-f]{2}|0))*\s*$/)
	{
	  my @splt = split ' ', $b;
	  foreach my $h (@splt)
	    {
	      push @args, hex $h;
	    }
	}
      else
	{
	  croak qq/Couldn't interpret string value: "$b"/;
	}
    }

  print $f pack("C" . scalar @args, @args) or croak(sprintf "Couldn't write" . (" 0x%02x" x scalar @args) . " at %x in $filename: %s", @args, $seekpos, $?);

  close $f or die "Couldn't close $filename: $!";
}

1;
