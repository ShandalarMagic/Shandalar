#!/bin/env perl

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';

use Capture::Tiny 'capture_merged';

my $dll = "ManalinkEh.dbg";

if (scalar @ARGV > 0
    && $ARGV[0] eq '-c')
  {
    shift @ARGV;
    $dll = shift @ARGV;
  }

if (scalar @ARGV == 0)
  {
    unshift @ARGV, "dump.dmp";
  }

my $dim = "[1;30m";
my $yellow = "[0;33m";
my $byellow = "[1;33m";
my $blue = "[0;34m";
my $bblue = "[1;34m";
my $cyan = "[0;36m";
my $norm = "[m";

my @addr;
open(my $exe, '<', "src/Magic-trace.c") or die "open src/Magic-trace: $!";
my $idx = 0;
while (<$exe>)
{
  chomp;
  s/[\n\r]+$//;
  if (m@^//----- \(([14][0-9A-F]+)\) -----+$@)
    {
      $idx = hex $1;
    }
  elsif (!m@^/@)
    {
      push @addr, [ $idx, $_ ];
    }
}
close $exe;

my @args = ();

$| = 1;

my $merged = '';

sub run_gdb
{
  if (scalar @args > 0)
    {
      $merged .= capture_merged { system "gdb", "-batch", "Magic.exe", "-ex", "sym $dll", @args; };
    }
  @args = ();

  $merged =~ s/\r//g;
  my @lines = split /\n/, $merged;
  foreach (@lines)
    {
      if (/Line ([0-9]+) of "([^"]+)" starts at address 0x[0-9a-fA-F]+ <([a-zA-Z0-9_@]+)(\+[0-9a-fA-F]*)?> and ends at 0x[0-9a-fA-F]+ <([a-zA-Z0-9_@]+)(\+[0-9a-fA-F]*)?>\.$/)
	{
	  say "$dim$2:$1:$yellow$3$norm(...)";
	}
      elsif (/^No line number information available for address 0x[0-9a-fA-F]+ <([a-zA-Z0-9_@]+)(\+[0-9a-fA-F]*)?>$/)
	{
	  say "$bblue$1$norm(...)";
	}
      elsif (/^No line number information available for address (0x[0-9a-fA-f]+)$/)
	{
	  say "$dim$1$norm";
	}
      elsif (/^PROCESSING/)
	{
	  say "$byellow$_$norm";
	}
      else
	{
	  s/\(([^()]+)\)/$norm($blue$1$norm)$cyan/g;
	  say "$cyan", $_, "$norm";
	}
    }

  $merged = '';
}

sub echo
{
  my $cmt = shift;
  if (scalar @args == 0)
    {
      $merged .= $cmt;
      $merged .= "\n";
    }
  else
    {
      push @args, "-ex", "echo $cmt\\n";
    }
}

LINE: while (<>)
  {
    chomp;
    s/[\n\r]+$//;
    if (/^#?[0-9]+[: ]+0x0*([41][0-9a-fA-F]+)/)
      {
	my $line = $_;
	my $hx = $1;
	my $dec = hex $hx;
	my $prev = $addr[0];
	my $found = 0;
	foreach my $el (@addr)
	  {
	    if ($el->[0] > $dec)
	      {
		echo $prev->[1];
		$found = 1;
		last;
	      }
	    $prev = $el;
	  }
	if ($found != 1)
	  {
	    echo $line;
	  }
      }
    elsif (/^#?[0-9]+[: ]+(0x[0-9a-fA-F]+)/)
      {
	push @args, "-ex", "info line *$1";
      }
    else
      {
	echo $_;
      }
    if (scalar @args >= 100)
      {
	run_gdb();
      }
  }

run_gdb();
