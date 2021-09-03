#!/bin/env perl

package Manalink::CSV;

use strict;
use warnings;
use feature ':5.10';
use mro 'c3';
use Carp qw/croak/;
use Exporter;

use FindBin;
use Scalar::Util qw/looks_like_number/;

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

my %interpret_ml =
  (
   'Color' =>
   [
    'colorless',
    'black',
    'blue',
    'artifact',
    'multicolored',
    'green',
    'land',
    'red',
    'white',
    'special',
   ],
   'Card Type' =>
   [
    'none',
    'artifact',
    'enchantment',
    'instant',
    'interrupt',
    'land',
    'sorcery',
    'creature',
    'token',
    'planeswalker (unused)',
    'scheme (unused)',
    'plane (unused)',
   ],
   'New Types 1' =>
   {
     -1 => '',
     0x1 => 'C:Ally',		0x2 => 'C:Angel',	0x3 => 'C:Anteater',	0x4 => 'C:Antelope',	0x5 => 'C:Ape',		0x6 => 'C:Archer',
     0x7 => 'C:Archon',		0x8 => 'C:Artificer',	0x9 => 'C:Assassin',	0xA => 'C:Assembly-Worker',			0xB => 'C:Atog',
     0xC => 'C:Aurochs',	0xD => 'C:Avatar',	0xE => 'C:Badger',	0xF => 'C:Barbarian',	0x10 => 'C:Basilisk',	0x11 => 'C:Bat',
     0x12 => 'C:Bear',		0x13 => 'C:Beast',	0x14 => 'C:Beeble',	0x15 => 'C:Berserker',	0x16 => 'C:Bird',	0x17 => 'C:Blinkmoth',
     0x18 => 'C:Boar',		0x19 => 'C:Bringer',	0x1A => 'C:Brushwagg',	0x1B => 'C:Camarid',	0x1C => 'C:Camel',	0x1D => 'C:Caribou',
     0x1E => 'C:Carrier',	0x1F => 'C:Cat',	0x20 => 'C:Centaur',	0x21 => 'C:Cephalid',	0x22 => 'C:Chimera',	0x23 => 'C:Citizen',
     0x24 => 'C:Cleric',	0x25 => 'C:Cockatrice',	0x26 => 'C:Construct',	0x27 => 'C:Coward',	0x28 => 'C:Crab',	0x29 => 'C:Crocodile',
     0x2A => 'C:Cyclops',	0x2B => 'C:Dauthi',	0x2C => 'C:Demon',	0x2D => 'C:Deserter',	0x2E => 'C:Devil',	0x2F => 'C:Djinn',
     0x30 => 'C:Dragon',	0x31 => 'C:Drake',	0x32 => 'C:Dreadnought',0x33 => 'C:Drone',	0x34 => 'C:Druid',	0x35 => 'C:Dryad',
     0x36 => 'C:Dwarf',		0x37 => 'C:Efreet',	0x38 => 'C:Elder',	0x39 => 'C:Eldrazi',	0x3A => 'C:Elemental',	0x3B => 'C:Elephant',
     0x3C => 'C:Elf',		0x3D => 'C:Elk',	0x3E => 'C:Eye',	0x3F => 'C:Faerie',	0x40 => 'C:Ferret',	0x41 => 'C:Fish',
     0x42 => 'C:Flagbearer',	0x43 => 'C:Fox',	0x44 => 'C:Frog',	0x45 => 'C:Fungus',	0x46 => 'C:Gargoyle',	0x47 => 'C:Germ',
     0x48 => 'C:Giant',		0x49 => 'C:Gnome',	0x4A => 'C:Goat',	0x4B => 'C:Goblin',	0x4C => 'C:Golem',	0x4D => 'C:Gorgon',
     0x4E => 'C:Graveborn',	0x4F => 'C:Gremlin',	0x50 => 'C:Griffin',	0x51 => 'C:Hag',	0x52 => 'C:Harpy',	0x53 => 'C:Hellion',
     0x54 => 'C:Hippo',		0x55 => 'C:Hippogriff',	0x56 => 'C:Homarid',	0x57 => 'C:Homunculus',	0x58 => 'C:Horror',	0x59 => 'C:Horse',
     0x5A => 'C:Hound',		0x5B => 'C:Human',	0x5C => 'C:Hydra',	0x5D => 'C:Hyena',	0x5E => 'C:Illusion',	0x5F => 'C:Imp',
     0x60 => 'C:Incarnation',	0x61 => 'C:Insect',	0x62 => 'C:Jellyfish',	0x63 => 'C:Juggernaut',	0x64 => 'C:Kavu',	0x65 => 'C:Kirin',
     0x66 => 'C:Kithkin',	0x67 => 'C:Knight',	0x68 => 'C:Kobold',	0x69 => 'C:Kor',	0x6A => 'C:Kraken',	0x6B => 'C:Lammasu',
     0x6C => 'C:Leech',		0x6D => 'C:Leviathan',	0x6E => 'C:Lhurgoyf',	0x6F => 'C:Licid',	0x70 => 'C:Lizard',	0x71 => 'C:Manticore',
     0x72 => 'C:Masticore',	0x73 => 'C:Mercenary',	0x74 => 'C:Merfolk',	0x75 => 'C:Metathran',	0x76 => 'C:Minion',	0x77 => 'C:Minotaur',
     0x78 => 'C:Monger',	0x79 => 'C:Mongoose',	0x7A => 'C:Monk',	0x7B => 'C:Moonfolk',	0x7C => 'C:Mutant',	0x7D => 'C:Myr',
     0x7E => 'C:Mystic',	0x7F => 'C:Nautilus',	0x80 => 'C:Nephilim',	0x81 => 'C:Nightmare',	0x82 => 'C:Nightstalker',
     0x83 => 'C:Ninja',		0x84 => 'C:Noggle',	0x85 => 'C:Nomad',	0x86 => 'C:Octopus',	0x87 => 'C:Ogre',	0x88 => 'C:Ooze',
     0x89 => 'C:Orb',		0x8A => 'C:Orc',	0x8B => 'C:Orgg',	0x8C => 'C:Ouphe',	0x8D => 'C:Ox',		0x8E => 'C:Oyster',
     0x8F => 'C:Pegasus',	0x90 => 'C:Pentavite',	0x91 => 'C:Pest',	0x92 => 'C:Phelddagrif',0x93 => 'C:Phoenix',	0x94 => 'C:Pincher',
     0x95 => 'C:Pirate',	0x96 => 'C:Plant',	0x97 => 'C:Praetor',	0x98 => 'C:Prism',	0x99 => 'C:Rabbit',	0x9A => 'C:Rat',
     0x9B => 'C:Rebel',		0x9C => 'C:Reflection',	0x9D => 'C:Rhino',	0x9E => 'C:Rigger',	0x9F => 'C:Rogue',	0xA0 => 'C:Salamander',
     0xA1 => 'C:Samurai',	0xA2 => 'C:Sand',	0xA3 => 'C:Saproling',	0xA4 => 'C:Satyr',	0xA5 => 'C:Scarecrow',	0xA6 => 'C:Scorpion',
     0xA7 => 'C:Scout',		0xA8 => 'C:Serf',	0xA9 => 'C:Serpent',	0xAA => 'C:Shade',	0xAB => 'C:Shaman',	0xAC => 'C:Shapeshifter',
     0xAD => 'C:Sheep',		0xAE => 'C:Siren',	0xAF => 'C:Skeleton',	0xB0 => 'C:Slith',	0xB1 => 'C:Sliver',	0xB2 => 'C:Slug',
     0xB3 => 'C:Snake',		0xB4 => 'C:Soldier',	0xB5 => 'C:Soltari',	0xB6 => 'C:Spawn',	0xB7 => 'C:Specter',	0xB8 => 'C:Spellshaper',
     0xB9 => 'C:Sphinx',	0xBA => 'C:Spider',	0xBB => 'C:Spike',	0xBC => 'C:Spirit',	0xBD => 'C:Splinter',	0xBE => 'C:Sponge',
     0xBF => 'C:Squid',		0xC0 => 'C:Squirrel',	0xC1 => 'C:Starfish',	0xC2 => 'C:Surrakar',	0xC3 => 'C:Survivor',	0xC4 => 'C:Tetravite',
     0xC5 => 'C:Thalakos',	0xC6 => 'C:Thopter',	0xC7 => 'C:Thrull',	0xC8 => 'C:Treefolk',	0xC9 => 'C:Triskelavite',
     0xCA => 'C:Troll',		0xCB => 'C:Turtle',	0xCC => 'C:Unicorn',	0xCD => 'C:Vampire',	0xCE => 'C:Vedalken',	0xCF => 'C:Viashino',
     0xD0 => 'C:Volver',	0xD1 => 'C:Wall',	0xD2 => 'C:Warrior',	0xD3 => 'C:Weird',	0xD4 => 'C:Werewolf',	0xD5 => 'C:Whale',
     0xD6 => 'C:Wizard',	0xD7 => 'C:Wolf',	0xD8 => 'C:Wolverine',	0xD9 => 'C:Wombat',	0xDA => 'C:Worm',	0xDB => 'C:Wraith',
     0xDC => 'C:Wurm',		0xDD => 'C:Yeti',	0xDE => 'C:Zombie',	0xDF => 'C:Zubera',	0xE0 => 'C:Advisor',	0xE1 => 'C:Lamia',
     0xE2 => 'C:Nymph',		0xE3 => 'C:Sable',	0xE4 => 'C:Head',	0xE5 => 'C:God',	0xE6 => 'C:Reveler',	0xE7 => 'C:Naga',
     0xE8 => 'C:Processor',	0xE9 => 'C:Scion',	0xEA => 'C:Mole',	0xF0 => 'C:Legend',	0xFFE => 'C:None',	0xFFF => 'C:All',

     0x1000 => 'T:Artifact',	0x1001 => 'T:Creature',	0x1002 => 'T:Enchantment',			0x1003 => 'T:Instant',	0x1004 => 'T:Land',
     0x1005 => 'T:Plane',	0x1006 => 'T:Planeswalker',			0x1007 => 'T:Scheme',	0x1008 => 'T:Sorcery',	0x1009 => 'T:Tribal',
     0x100A => 'T:Vanguard',	0x100B => 'T:Hero',	0x100C => 'T:Conspiracy',			0x100D => 'T:Phenomenon',
     0x1FFF => 'T:Private',

     0x2000 => 'S:Basic',	0x2001 => 'S:Legend',	0x2002 => 'S:Ongoing',	0x2003 => 'S:Snow',	0x2004 => 'S:World',	0x2005 => 'S:Nonbasic',
     0x2006 => 'S:Elite',

     0x3000 => 'A:Contraption',	0x3001 => 'A:Equipment',0x3002 => 'A:Fortification',			0x3003 => 'A:Horde',
     0x3004 => 'A:Clue',

     0x4000 => 'E:Aura',	0x4001 => 'E:Aura-artifact',	0x4002 => 'E:Aura-creature',	0x4003 => 'E:Aura-enchantment',
     0x4004 => 'E:Aura-land',	0x4005 => 'E:Aura-permanent',	0x4006 => 'E:Aura-player',	0x4007 => 'E:Aura-instant',
     0x4008 => 'E:Aura-planeswalker',				0x4010 => 'E:Curse',		0x4020 => 'E:Shrine',

     0x5000 => 'L:Desert',	0x5001 => 'L:Forest',	0x5002 => 'L:Island',	0x5003 => 'L:Lair',	0x5004 => 'L:Locus',	0x5005 => 'L:Mine',
     0x5006 => 'L:Mountain',	0x5007 => 'L:Plains',	0x5008 => 'L:Power-Plant',			0x5009 => 'L:Swamp',	0x500A => 'L:Tower',
     0x500B => 'L:Urzas',	0x500C => 'L:Gate',

     0x6000 => 'I:Arcane',	0x6001 => 'I:Trap',

     0x7000 => 'P:Ajani',	0x7001 => 'P:Bolas',	0x7002 => 'P:Chandra',	0x7003 => 'P:Elspeth',	0x7004 => 'P:Garruk',	0x7005 => 'P:Gideon',
     0x7006 => 'P:Jace',	0x7007 => 'P:Karn',	0x7008 => 'P:Koth',	0x7009 => 'P:Liliana',	0x700A => 'P:Nissa',	0x700B => 'P:Sarkhan',
     0x700C => 'P:Sorin',	0x700D => 'P:Tezzeret',	0x700E => 'P:Venser',	0x700F => 'P:Tamiyo',	0x7010 => 'P:Tibalt',	0x7011 => 'P:Vraska',
     0x7012 => 'P:Domri',	0x7013 => 'P:Ral',	0x7014 => 'P:Ashiok',	0x7015 => 'P:Xenagos',	0x7016 => 'P:Dack',	0x7017 => 'P:Kiora',
     0x7018 => 'P:Daretti',	0x7019 => 'P:Freyalise',0x701A => 'P:Nahiri',	0x701B => 'P:Nixilis',	0x701C => 'P:Teferi',	0x701D => 'P:Ugin',
     0x701E => 'P:Narset',	0x701F => 'P:Arlinn',

     0x8000 => 'Pl:Alara',	0x8001 => 'Pl:Arkhos',	0x8002 => "Pl:Bolas's-Meditation-Realm",	0x8003 => 'Pl:Dominaria',
     0x8004 => 'Pl:Equilor',	0x8005 => 'Pl:Iquatana',0x8006 => 'Pl:Ir',	0x8007 => 'Pl:Kaldheim',0x8008 => 'Pl:Kamigawa',0x8009 => 'Pl:Karsus',
     0x800A => 'Pl:Kinshala',	0x800B => 'Pl:Lorwyn',	0x800C => 'Pl:Luvion',	0x800D => 'Pl:Mercadia',0x800E => 'Pl:Mirrodin',0x800F => 'Pl:Moag',
     0x8010 => 'Pl:Muraganda',	0x8011 => 'Pl:Phyrexia',0x8012 => 'Pl:Pyrulea',	0x8013 => 'Pl:Rabiah',	0x8014 => 'Pl:Rath',	0x8015 => 'Pl:Ravnica',
     0x8016 => 'Pl:Segovia',	0x8017 => "Pl:Serra's-Realm",			0x8018 => 'Pl:Shadowmoor',			0x8019 => 'Pl:Shandalar',
     0x801A => 'Pl:Ulgrotha',	0x801B => 'Pl:Valla',	0x801C => 'Pl:Wildfire',0x801D => 'Pl:Zendikar',0x801E => 'Pl:Azgol',	0x801F => 'Pl:Belenon',
     0x8020 => 'Pl:Ergamon',	0x8021 => 'Pl:Fabacin',	0x8022 => 'Pl:Innistrad',			0x8023 => 'Pl:Kephalai',0x8024 => 'Pl:Kolbahan',
     0x8025 => 'Pl:Kyneth',	0x8026 => 'Pl:Mongseng',0x8027 => 'Pl:New-Phyrexia',			0x8028 => 'Pl:Regatha',	0x8029 => 'Pl:Vryn',
     0x8030 => 'Pl:Xerex',
   },
   'Subtype' =>
   {
     1 => 'Incarnation',	2 => 'Artificer',	3 => 'Advisor',		4 => 'Pest',		5 => 'Angel',		6 => 'Snow',
     7 => 'Ape',		8 => 'Nightstalker',	9 => 'Archer',		10 => 'Dryad',		11 => 'Enchant-Artifact',
     12 => 'Antelope',		13 => 'Assassin',	14 => 'Atog',		15 => 'Avatar',		16 => 'Archon',		17 => 'Plant',
     18 => 'Badger',		19 => 'Aurochs',	20 => 'Locus',		21 => 'Blinkmoth',	22 => 'Basilisk',	23 => 'Bat',
     24 => 'Bear',		25 => 'Beast',		26 => 'Bringer',	27 => 'Sliver',		28 => 'Berserker',	29 => 'Monk',
     30 => 'Boar',		31 => 'Trap',		32 => 'Ally',		33 => 'Ox',		34 => 'Cyclops',	35 => 'Camel',
     36 => 'Equipment',		37 => 'Cephalid',	38 => 'Chimera',	39 => 'Centaur',	40 => 'Cleric',		41 => 'Shapeshifter',
     42 => 'Construct',		43 => 'Cockatrice',	44 => 'Artifact-Creature/Enchant-creature',	45 => 'Curse',		46 => 'Demon',
     47 => 'Eldrazi',		48 => 'Devil',		49 => 'Crab',		50 => 'Djinn',		51 => 'Shaman',		52 => 'Dragon',
     53 => 'Dauthi',		54 => 'Drake',		55 => 'Deserter',	56 => 'Druid',		57 => 'Dwarf',		58 => 'Eye',
     59 => 'Dreadnought',	60 => 'Drone',		61 => 'Efreet',		62 => 'Egg',		63 => 'Elk',		64 => 'Elder',
     65 => 'Elemental',		66 => 'Elephant',	67 => 'Elf',		68 => 'Enchant-Enchantment',			69 => 'Nonbasic-Land',
     70 => 'Flagbearer',	71 => 'Minion',		72 => 'Fox',		73 => 'Faerie',		74 => 'Homunculus',	75 => 'Frog',
     76 => 'Goat',		77 => 'Graveborn',	78 => 'Harpy',		79 => 'Fungus',		80 => 'Hellion',	81 => 'Gargoyle',
     82 => 'Hippo',		83 => 'Homarid',	84 => 'Giant',		85 => 'Gnome',		86 => 'Goblin',		87 => 'Lizard',
     88 => 'Hyena',		89 => 'Jellyfish',	90 => 'Soldier',	91 => 'Gorgon',		92 => 'Hag',		93 => 'Human',
     94 => 'Horror',		95 => 'Horse',		96 => 'Kavu',		97 => 'Hydra',		98 => 'Imp',		99 => 'Kirin',
     100 => 'Kor',		101 => 'Juggernaut',	102 => 'Kraken',	103 => 'Lammasu',	104 => 'Kithkin',	105 => 'Knight',
     106 => 'Golem',		107 => 'Kobold',	108 => 'Enchant-Land',	109 => 'Leech',		110 => 'Unused',	111 => 'Mountain',
     112 => 'Licid',		113 => 'Metathran',	114 => 'Leviathan',	115 => 'Monger',	116 => 'Mongoose',	117 => 'Desert',
     118 => 'Moonfolk',		119 => 'Mutant',	120 => 'Mystic',	121 => 'Bird',		122 => 'Manticore',	123 => 'Nautilus',
     124 => 'Nephilim',		125 => 'Merfolk',	126 => 'Minotaur',	127 => 'Ninja',		128 => 'Masticore',	129 => 'Octopus',
     130 => 'Myr',		131 => 'Orb',		132 => 'Orgg',		133 => 'Oyster',	134 => 'Ouphe',		135 => 'Nightmare',
     136 => 'Nomad',		137 => 'Island',	138 => 'Ogre',		139 => 'Pentavite',	140 => 'Ooze',		141 => 'Orc',
     142 => 'Forest',		143 => 'Pegasus',	144 => 'Swamp',		145 => 'Phelddagrif',	146 => 'Phoenix',	147 => 'Pincher',
     148 => 'Plains',		149 => 'Prism',		150 => 'Rabbit',	151 => 'Reflection',	152 => 'Barbarian',	153 => 'Rhino',
     154 => 'Rat',		155 => 'Cat',		156 => 'Rogue',		157 => 'Rigger',	158 => 'Salamander',	159 => 'Scout',
     160 => 'Satyr',		161 => 'Scarecrow',	162 => 'Scorpion',	163 => 'Serpent',	164 => 'Shade',		165 => 'Enchant-Permanent',
     166 => 'Fish',		167 => 'Pirate',	168 => 'Samurai',	169 => 'Saproling',	170 => 'Serf',		171 => 'Skeleton',
     172 => 'Slug',		173 => 'Sheep',		174 => 'Slith',		175 => 'Spawn',		176 => 'Specter',	177 => 'Sphinx',
     178 => 'Spider',		179 => 'Spirit',	180 => 'Soltari',	181 => 'Spellshaper',	182 => 'Griffin',	183 => 'Spike',
     184 => 'Turtle',		185 => 'Thopter',	186 => 'Treefolk',	187 => 'Troll',		188 => 'Basic-Land',	189 => 'Werewolf',
     190 => 'Unicorn',		191 => 'Vampire',	192 => 'Sponge',	193 => 'Squid',		194 => 'Squirrel',	195 => 'Starfish',
     196 => 'Warrior',		197 => 'Wall',		198 => 'Praetor',	199 => 'Thalakos',	200 => 'Thrull',	201 => 'Wizard',
     202 => 'Wolverine',	203 => 'Wolf',		204 => 'Wombat',	205 => 'World',		206 => 'Wraith',	207 => 'Triskelavite',
     208 => 'Wurm',		209 => 'Yeti',		210 => 'Zombie',	211 => 'None',		212 => 'Crocodile',	213 => 'Illusion',
     214 => 'Insect',		215 => 'Vedalken',	216 => 'Volver',	217 => 'Weird',		218 => 'Snake',		219 => 'Sand',
     220 => 'Tetravite',	221 => 'Whale',		222 => 'Worm',		223 => 'Zubera',	224 => 'Assembly-Worker',
     225 => 'Hound',		226 => 'Lhurgoyf',	227 => 'Tribal',	228 => 'Shrine',	229 => 'Carrier',	230 => 'Mercenary',
     231 => 'Rebel',		232 => 'Viashino',	233 => 'Enchant-Instant',			234 => 'Enchant-Player',
   },
   'DBA1' =>
   {
     1 => 'Native Banding',
     2 => 'Native Desertwalk',
     3 => 'Native First Strike/Double Strike',
     4 => 'Native Flying',
     5 => 'Native Forestwalk',
     6 => 'Native Vigilance',
     7 => 'Native Islandwalk',
     8 => 'Native Legendary Landwalk',
     9 => 'Native Mountainwalk',
     10 => 'Native Plainswalk',
     11 => 'Native Infect',
     12 => 'Native Protection from Black',
     13 => 'Native Protection from Red',
     14 => 'Native Protection from White',
     15 => 'Native Haste',
     16 => 'Native Rampage',
     17 => 'Native Regeneration',
     18 => 'Native Deathtouch',
     19 => 'Native Swampwalk',
     20 => 'Native Trample',
     21 => 'Native Reach',
     22 => 'Grants Banding',
     23 => 'Grants Firststrike',
     24 => 'Grants Flying',
     25 => 'Grants Forestwalk',
     26 => 'Grants Vigilance',
     27 => 'Grants Islandwalk',
     28 => 'Grants Mountainwalk',
     29 => 'Grants Plainswalk',
     30 => 'Grants Protection from Artifacts',
     31 => 'Grants Protection from Black',
     32 => 'Grants Protection from Blue',
     33 => 'Grants Protection from Green',
     34 => 'Grants Protection from Red',
     35 => 'Grants Protection from White',
     36 => 'Grants Haste',
     37 => 'Grants Rampage',
     38 => 'Grants Regeneration',
     39 => 'Grants Deathtouch',
     40 => 'Grants Swampwalk',
     41 => 'Grants Trample',
     42 => 'Grants Reach',
   },
  );

foreach (2..7) { $interpret_ml{"New Types $_"} = $interpret_ml{'New Types 1'}; }
foreach (2..8) { $interpret_ml{"DBA$_"} = $interpret_ml{'DBA1'}; }
$interpret_ml{'Subtype 2'} = $interpret_ml{'Subtype'};

sub dumpml
{
  my $ml = shift;

  init_ml_hdr();

  foreach my $key (@ml_hdr)
    {
      my $val = $ml->{$key} // '0';
      if ($val !~ /^\s*[-0]?\s*$/ || $key eq 'Color' || $key eq 'Card Type')
	{
	  if ($key eq 'Mana Source Colors' || $key eq 'Sleighted Color')
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
	      if (exists $interpret_ml{$key})
		{
		  my $v = $val;
		  if ($v =~ /^([0-9a-fA-F]+)h$/)
		    {
		      $v = hex "0x$1";
		    }
		  elsif ($v =~ /^0[xX][0-9a-fA-F]+$/)
		    {
		      $v = hex $v;
		    }

		  if (ref $interpret_ml{$key} eq 'HASH')
		    {
		      if (exists $interpret_ml{$key}->{$v})
			{
			  if ($interpret_ml{$key}->{$v} ne '')
			    {
			      $val = "$val ($interpret_ml{$key}->{$v})";
			    }
			}
		      else
			{
			  $val = "$val (?)";
			}
		    }
		  elsif (ref $interpret_ml{$key} eq 'ARRAY')
		    {
		      if (looks_like_number($v) && $v < scalar(@{$interpret_ml{$key}}))
			{
			  $val = "$val ($interpret_ml{$key}->[$v])";
			}
		      else
			{
			  $val = "$val (?)";
			}
		    }
		  else
		    {
		      die "Unknown interpret_ml reftype for '$key'";
		    }
		}

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
