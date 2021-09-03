#!/bin/env perl

package Manalink::Patch;

use strict;
use warnings;
use Carp qw/croak/;
use Exporter;
use Scalar::Util qw/looks_like_number/;

our $VERSION = 1.00;
our @ISA = qw/Exporter/;
our @EXPORT = qw/patch string_literal jmp_to call_to/;

my $last_filename = undef;
my $last_raw = 0;
my $filename;

my $f;

END
  {
    if (defined $last_filename)
      {
	my $lf = $last_filename;
	$last_filename = undef;
	close $f or die "Couldn't close $lf: $!";
      }
  }

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

      if (defined $last_filename)
	{
	  my $lf = $last_filename;
	  $last_filename = undef;
	  close $f or die "Couldn't close $lf: $!";
	}
      open($f, "+<", $filename) or croak "Couldn't open $filename: $!";
      binmode($f);
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

      if (defined $last_filename)
	{
	  my $lf = $last_filename;
	  $last_filename = undef;
	  close $f or die "Couldn't close $lf: $!";
	}
      open($f, "+<", $filename) or croak "Couldn't open $filename: $!";
      binmode($f);
      $last_filename = $filename;

      $last_raw = 0;
      $lc_filename = lc $filename;
    }

  exists $offsets{$lc_filename} or croak qq/Unknown target file "$filename"/;

  $seekpos >= $offsets{$lc_filename} or croak sprintf qq/Seekpos $seekpos is less than $filename's base offset %x/, $offsets{$lc_filename};
  $seekpos -= $offsets{$lc_filename};

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
}

sub jmp_to_impl
{
  my $opcode = shift;

  my @args;
  my $filename = shift;

  defined $filename or croak "needs at least two non-filename arguments";

  if ($filename =~ /^raw$/i)
    {
      push @args, $filename;
      push @args, shift;	# actual filename
    }
  elsif (looks_like_number $filename)
    {
      unshift @_, $filename;
    }
  else
    {
      push @args, $filename;
    }

  scalar @_ >= 2 or croak "needs at least two non-filename arguments";
  (scalar @_) % 2 == 0 or croak "needs an even number of non-filename arguments";

  while (scalar @_ > 0)
    {
      my $seekpos = shift;
      my $newloc = shift;
      push @args, $seekpos;
      my $diff = $newloc - $seekpos - 5;
      if ($diff < 0)
	{	# force proper 32-bit 2's-complement representation
	  $diff += 1;
	  $diff += 0xFFFFFFFF;
	  $diff >= 0 or die;
	}
      push @args, sprintf("%s %02x %02x %02x %02x",
			  $opcode,
			  $diff & 0xFF,
			  ($diff >> 8) & 0xFF,
			  ($diff >> 16) & 0xFF,
			  ($diff >> 24) & 0xFF);
      patch(@args);
      @args = ();
    }
}

sub jmp_to
{
  jmp_to_impl("e9", @_);
}

sub call_to
{
  jmp_to_impl("e8", @_);
}

1;
