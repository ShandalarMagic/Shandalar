#!/bin/perl

use strict;
use warnings;
use feature ':5.10';

use Carp;
use Encode;
use Text::CSV;
use XML::Simple qw/:strict/;

use Manalink::CSV;

my $cr = "\r\n";	# Stupid variable newlines.  This should exactly match the ones used in Manalink.csv and ct_all.csv.

sub usage()
{
  return <<EOT;
Usage: $0 [options] inputfile.xml
--help: this message
--output: make output.pl
--color: output the "Updating sets", "Duplicate card", and "is a reprint" lines in dark blue, and warnings in bright red
--quiet: omit the "Updating sets", "Duplicate card", and "is a reprint" lines
--update: only update rules text and add flavor text; never add new cards

Generalized update flags: These work only on newish cards, and never add new
cards or touch old cards.

--abils: only update ability columns (in both ct_all.csv and Manalink.csv)
--modifies-casting-cost: only update "Modifies Casting Cost" in ct_all.csv
--card-type: only update "Card Type" in Manalink.csv
--mana-source: only update "Flags: Mana Source" in ct_all.csv
--equipment: only updates "Flags: Activate", "Activate after Combat",
             "Activate before Combat", "Play after Combat", and
	     "Play before Combat" in ct_all.csv, and only for Equipment
--tokens: processes cards with "Token" in the type line (normally skipped)
EOT
}

my $first_ml_to_update_for_abils = 'Acid-Spewer Dragon';
my $first_ct_to_update_for_abils = 'Skeleton';

my $manalink_filename = 'Manalink.csv';
my $ctall_filename = 'ct_all.csv';

my $infile = undef;
my $opt_output = 0;
my $opt_color = 0;
my $color_darkblue = '';
my $color_brightred = '';
my $color_normal = '';
my $opt_quiet = 0;
my $opt_update = 0;
my $updates_only = 0;
my $opt_equipment = 0;
my %all_equipment = ();	# Populated only if $opt_equipment is set
my $opt_tokens = 0;
my @ct_cols_to_update = ();
my @mk_cols_to_update = ();
while (1)
  {
    my $arg = shift;
    if (!defined $arg)
      {
	defined $infile and last;
	die usage();
      }
    if ($arg eq '-h' || $arg eq '--help')
      {
	print STDERR usage();
	exit 0;
      }
    if ($arg eq '--output')
      {
	$opt_output = 1;
	next;
      }
    if ($arg eq '--color')
      {
	$opt_color = 1;
	$color_darkblue = "\033[34m";
	$color_brightred = "\033[1;31m";
	$color_normal = "\033[0m";
	next;
      }
    if ($arg eq '--quiet')
      {
	$opt_quiet = 1;
	next;
      }
    if ($arg eq '--update')
      {
	$opt_update = 1;
	$updates_only = 1;
	next;
      }
    if ($arg eq '--abils')
      {
	#$opt_abils = 1;
	$updates_only = 1;
	push @mk_cols_to_update,
	  'AI Banding', 'AI First Strike', 'AI Flying', 'AI Trample', 'AI Wall',
	  'DBA1', 'DBA2', 'DBA3', 'DBA4', 'DBA5', 'DBA6', 'DBA7', 'DBA8',
	  'Inflatable', 'Num of DB Abilities';
	push @ct_cols_to_update,
	  'Ability: Double Strike', 'Ability: Infect', 'Ability: Defender', 'Ability: Changeling', 'Ability: Shroud',
	  'Ability: Prot from Black', 'Ability: Prot from Blue', 'Ability: Prot from Green', 'Ability: Prot from Red', 'Ability: Prot from White', 'Ability: Prot from Artifacts',
	  'Ability: Reach', 'Ability: Regeneration', 'Ability: First Strike', 'Ability: Trample', 'Ability: Banding', 'Ability: Flying',
	  'Ability: Swampwalk', 'Ability: Islandwalk', 'Ability: Forestwalk', 'Ability: Mountainswalk', 'Ability: Plainswalk',
	  'Flags: Inflatable Power', 'Flags: Inflatable Toughness',
	  'Play after Blockers', 'Play before Blockers', 'Play before Combat', 'Play after Combat',
	  'Activate after Blockers', 'Activate before Blockers', 'Activate before Combat', 'Activate after Combat',
	  'Type: Instant';
	next;
      }
    if ($arg eq '--modifies-casting-cost')
      {
	#$opt_modifies_casting_cost = 1;
	$updates_only = 1;
	push @ct_cols_to_update, 'Modifies Casting Cost';
	next;
      }
    if ($arg eq '--card-type')
      {
	$updates_only = 1;
	push @mk_cols_to_update, 'Card Type';
	next;
      }
    if ($arg eq '--mana-source')
      {
	$updates_only = 1;
	push @ct_cols_to_update, 'Flags: Mana Source';
	next;
      }
    if ($arg eq '--equipment')
      {
	$updates_only = 1;
	push @ct_cols_to_update, 'Flags: Activate', 'Activate after Combat', 'Activate before Combat', 'Play after Combat', 'Play before Combat';
	$opt_equipment = 1;
	next;
      }
    if ($arg eq '--tokens')
      {
	$opt_tokens = 1;
	next;
      }
    $arg =~ /^--/ and die usage();
    defined $infile and die usage();
    $infile = $arg;
  }

-e $infile && -f $infile && -r $infile
  or die 'Could not open "$infile"';

sub mywarn
{
  package My::Dummy::Package;	# so carp shows the caller of mywarn(), but not a full backtrace
  Carp::carp $color_brightred, @_, $color_normal;
}

say STDERR qq'Reading "$infile"';
my $xml = XMLin($infile, ForceArray=>['card', 'set'], KeyAttr=>"");

if ($opt_output)
  {
    use Data::Dumper;
    open(my $output, '>', 'output.pl') or die "Couldn't open output.pl for writing: $!";
    say $output '#!/bin/perl';
    my $dmp = Dumper($xml);
    $dmp =~ s/^\s+'(type|flavor|ability|name)_(RU|CN|DE|FR|KO|JP|ES|IT|TW|PT)' =>.*('.*'|".*"|\{\}),?\s*$//gm;
    $dmp =~ s/^\s+'(print_number|pricing_(mid|low|high))' => \{\},?\s*$//gm;
    $dmp =~ s/^\s+'(id|number)' => '[0-9]+'\s*$//gm;
    $dmp =~ s/\n\n+/\n/g;
    say $output $dmp;
    say STDERR "Wrote output.pl";
    exit 0;
  }

exists $xml->{cards} or die '!exists $xml->{cards}';
exists $xml->{cards}->{card} or die '!exists $xml->{cards}->{card}';
my $cards = $xml->{cards}->{card};

my @ct_columns = ct_hdrs();
my %ct_colnums;
foreach my $i (0..$#ct_columns)
  {
    $ct_colnums{$ct_columns[$i]} = $i;
  }

my @ml_columns = ml_hdrs();
my %ml_colnums;
foreach my $i (0..$#ml_columns)
  {
    $ml_colnums{$ml_columns[$i]} = $i;
  }

my @sets = @ml_columns;
while (scalar @sets > 0 && $sets[0] ne '(B) Block')
  {
    shift @sets;
  }
scalar @sets > 0 or die qq[Couldn't find "(B) Block" among $manalink_filename column headers];
my @realsets = @sets;
while (scalar @realsets > 0 && $realsets[0] ne 'Promo')
  {
    shift @realsets;
  }
scalar @realsets > 0 or die qq[Couldn't find "Promo" among $manalink_filename column headers];

my %code;

my %translate_set_names =
  (
   'Archenemy "Schemes"' => 'Archenemy',
   'Commander 2013 Edition' => 'Commander 2013',
   'From the Vault: Annihilation (2014)' => 'From the Vault: Annihilation',
   'Happy Holidays' => 'Promo',
   'Magic 2014 Core Set' => 'Magic 2014',
   'Magic 2015 Core Set' => 'Magic 2015',
   qq'Magic: The Gathering\x{2014}Conspiracy' => 'Magic: The Gathering-Conspiracy',
   qq'Magic: The Gathering\x{2014}Conspiracy "Conspiracies"' => 'Magic: The Gathering-Conspiracy',
   'Media Inserts' => 'Promo',
   'Modern Event Deck 2014' => 'Modern Event Deck',
   'Planechase "Planes"' => 'Planechase',
   'Planechase 2012 Edition "Planes" and "Phenomena"' => 'Planechase 2012',
   'Planechase 2012 Edition' => 'Planechase 2012',
   'Time Spiral "Timeshifted"' => 'Time Spiral',
   'Eighth Edition Box Set' => 'Eighth Edition',
   'Ninth Edition Box Set' => 'Ninth Edition',
  );

my %ignore_set_codes =
  (
   'DPA' => 'Duels of the Planeswalkers',
   'W16' => 'Welcome Deck 2016',
  );

exists $xml->{sets} or die '!exists $xml->{sets}';
exists $xml->{sets}->{set} or die '!exists $xml->{sets}->{set}';
my $xmlsets = $xml->{sets}->{set};
my $n = 0;
{
  my %hsets;
  $hsets{$_} = 1 foreach (@sets);
  foreach my $xset (@{$xmlsets})
    {
      ++$n;
      unless (exists $xset->{name} && !ref $xset->{name})
	{
	  mywarn "No name for set #$n";
	  next;
	}

      if (exists $translate_set_names{$xset->{name}})
	{
	  $xset->{name} = $translate_set_names{$xset->{name}};
	}

      unless (exists $xset->{code} && !ref $xset->{code})
	{
	  mywarn qq[No code for set "$xset->{name}"];
	  next;
	}
      exists $code{$xset->{code}} and die qq[Set "$xset->{name}" has code "$xset->{code}", which is already assigned to set "$code{$xset->{code}}"];
      unless (exists $hsets{$xset->{name}})
	{
	  exists $ignore_set_codes{$xset->{code}}
	    or mywarn qq[Could not find set "$xset->{name}" among $manalink_filename column headers];
	  next;
	}
      $code{$xset->{code}} = $xset->{name};
    }
}

#say STDERR qq'Reading "$manalink_filename"';
#my $manalink = Text::CSV::Slurp->load(file => $manalink_filename, sep_char=>';');
#my %manalink_names;
#foreach my $ml (@$manalink)
#  {
#    $manalink_names{$ml->{'Full Name'}} = $ml;
#  }
#
#say STDERR qq'Reading "$ctall_filename"';
#my $ctall = Text::CSV::Slurp->load(file => $ctall_filename, sep_char=>';');
#my %ctall_names;
#foreach my $ct (@$ctall)
#  {
#    $ctall_names{$ct->{'Full Card Name in CSV (must be exactly the same)'}} = $ct;
#  }

my %colors =
  (
   C=>0,
   S=>0,	# schemes
   P=>0,	# planes
   E=>0,	# phenomena
   B=>1,
   U=>2,
   A=>3,
   #Multi => 4,
   G=>5,
   L=>6,
   R=>7,
   W=>8,
  );

my %mana_req =
  (
   B=>'Req Black',
   U=>'Req Blue',
   G=>'Req Green',
   R=>'Req Red',
   W=>'Req White',
  );

my %mana_test =
  (
   1=>0x1,
   2=>0x1,
   3=>0x1,
   4=>0x1,
   5=>0x1,
   6=>0x1,
   7=>0x1,
   8=>0x1,
   9=>0x1,
   X=>0x1,
   C=>0x1,
   B=>0x2,
   U=>0x4,
   G=>0x8,
   R=>0x10,
   W=>0x20,
  );

my %newtypes =
  (
   Ally=>'1h',
   Angel=>'2h',
   Anteater=>'3h',
   Antelope=>'4h',
   Ape=>'5h',
   Archer=>'6h',
   Archon=>'7h',
   Artificer=>'8h',
   Assassin=>'9h',
   'Assembly-Worker'=>'Ah',
   Atog=>'Bh',
   Aurochs=>'Ch',
   Avatar=>'Dh',
   Badger=>'Eh',
   Barbarian=>'Fh',
   Basilisk=>'10h',
   Bat=>'11h',
   Bear=>'12h',
   Beast=>'13h',
   Beeble=>'14h',
   Berserker=>'15h',
   Bird=>'16h',
   Blinkmoth=>'17h',
   Boar=>'18h',
   Bringer=>'19h',
   Brushwagg=>'1Ah',
   Camarid=>'1Bh',
   Camel=>'1Ch',
   Caribou=>'1Dh',
   Carrier=>'1Eh',
   Cat=>'1Fh',
   Centaur=>'20h',
   Cephalid=>'21h',
   Chimera=>'22h',
   Citizen=>'23h',
   Cleric=>'24h',
   Cockatrice=>'25h',
   Construct=>'26h',
   Coward=>'27h',
   Crab=>'28h',
   Crocodile=>'29h',
   Cyclops=>'2Ah',
   Dauthi=>'2Bh',
   Demon=>'2Ch',
   Deserter=>'2Dh',
   Devil=>'2Eh',
   Djinn=>'2Fh',
   Dragon=>'30h',
   Drake=>'31h',
   Dreadnought=>'32h',
   Drone=>'33h',
   Druid=>'34h',
   Dryad=>'35h',
   Dwarf=>'36h',
   Efreet=>'37h',
   Elder=>'38h',
   Eldrazi=>'39h',
   Elemental=>'3Ah',
   Elephant=>'3Bh',
   Elf=>'3Ch',
   Elk=>'3Dh',
   Eye=>'3Eh',
   Faerie=>'3Fh',
   Ferret=>'40h',
   Fish=>'41h',
   Flagbearer=>'42h',
   Fox=>'43h',
   Frog=>'44h',
   Fungus=>'45h',
   Gargoyle=>'46h',
   Germ=>'47h',
   Giant=>'48h',
   Gnome=>'49h',
   Goat=>'4Ah',
   Goblin=>'4Bh',
   Golem=>'4Ch',
   Gorgon=>'4Dh',
   Graveborn=>'4Eh',
   Gremlin=>'4Fh',
   Griffin=>'50h',
   Hag=>'51h',
   Harpy=>'52h',
   Hellion=>'53h',
   Hippo=>'54h',
   Hippogriff=>'55h',
   Homarid=>'56h',
   Homunculus=>'57h',
   Horror=>'58h',
   Horse=>'59h',
   Hound=>'5Ah',
   Human=>'5Bh',
   Hydra=>'5Ch',
   Hyena=>'5Dh',
   Illusion=>'5Eh',
   Imp=>'5Fh',
   Incarnation=>'60h',
   Insect=>'61h',
   Jellyfish=>'62h',
   Juggernaut=>'63h',
   Kavu=>'64h',
   Kirin=>'65h',
   Kithkin=>'66h',
   Knight=>'67h',
   Kobold=>'68h',
   Kor=>'69h',
   Kraken=>'6Ah',
   Lammasu=>'6Bh',
   Leech=>'6Ch',
   Leviathan=>'6Dh',
   Lhurgoyf=>'6Eh',
   Licid=>'6Fh',
   Lizard=>'70h',
   Manticore=>'71h',
   Masticore=>'72h',
   Mercenary=>'73h',
   Merfolk=>'74h',
   Metathran=>'75h',
   Minion=>'76h',
   Minotaur=>'77h',
   Monger=>'78h',
   Mongoose=>'79h',
   Monk=>'7Ah',
   Moonfolk=>'7Bh',
   Mutant=>'7Ch',
   Myr=>'7Dh',
   Mystic=>'7Eh',
   Nautilus=>'7Fh',
   Nephilim=>'80h',
   Nightmare=>'81h',
   Nightstalker=>'82h',
   Ninja=>'83h',
   Noggle=>'84h',
   Nomad=>'85h',
   Octopus=>'86h',
   Ogre=>'87h',
   Ooze=>'88h',
   Orb=>'89h',
   Orc=>'8Ah',
   Orgg=>'8Bh',
   Ouphe=>'8Ch',
   Ox=>'8Dh',
   Oyster=>'8Eh',
   Pegasus=>'8Fh',
   Pentavite=>'90h',
   Pest=>'91h',
   Phelddagrif=>'92h',
   Phoenix=>'93h',
   Pincher=>'94h',
   Pirate=>'95h',
   Plant=>'96h',
   Praetor=>'97h',
   Prism=>'98h',
   Rabbit=>'99h',
   Rat=>'9Ah',
   Rebel=>'9Bh',
   Reflection=>'9Ch',
   Rhino=>'9Dh',
   Rigger=>'9Eh',
   Rogue=>'9Fh',
   Salamander=>'A0h',
   Samurai=>'A1h',
   Sand=>'A2h',
   Saproling=>'A3h',
   Satyr=>'A4h',
   Scarecrow=>'A5h',
   Scorpion=>'A6h',
   Scout=>'A7h',
   Serf=>'A8h',
   Serpent=>'A9h',
   Shade=>'AAh',
   Shaman=>'ABh',
   Shapeshifter=>'ACh',
   Sheep=>'ADh',
   Siren=>'AEh',
   Skeleton=>'AFh',
   Slith=>'B0h',
   Sliver=>'B1h',
   Slug=>'B2h',
   Snake=>'B3h',
   Soldier=>'B4h',
   Soltari=>'B5h',
   Spawn=>'B6h',
   Specter=>'B7h',
   Spellshaper=>'B8h',
   Sphinx=>'B9h',
   Spider=>'BAh',
   Spike=>'BBh',
   Spirit=>'BCh',
   Splinter=>'BDh',
   Sponge=>'BEh',
   Squid=>'BFh',
   Squirrel=>'C0h',
   Starfish=>'C1h',
   Surrakar=>'C2h',
   Survivor=>'C3h',
   Tetravite=>'C4h',
   Thalakos=>'C5h',
   Thopter=>'C6h',
   Thrull=>'C7h',
   Treefolk=>'C8h',
   Triskelavite=>'C9h',
   Troll=>'CAh',
   Turtle=>'CBh',
   Unicorn=>'CCh',
   Vampire=>'CDh',
   Vedalken=>'CEh',
   Viashino=>'CFh',
   Volver=>'D0h',
   Wall=>'D1h',
   Warrior=>'D2h',
   Weird=>'D3h',
   Werewolf=>'D4h',
   Whale=>'D5h',
   Wizard=>'D6h',
   Wolf=>'D7h',
   Wolverine=>'D8h',
   Wombat=>'D9h',
   Worm=>'DAh',
   Wraith=>'DBh',
   Wurm=>'DCh',
   Yeti=>'DDh',
   Zombie=>'DEh',
   Zubera=>'DFh',
   Advisor=>'E0h',
   Lamia=>'E1h',
   Nymph=>'E2h',
   Sable=>'E3h',
   Head=>'E4h',
   God=>'E5h',
   Reveler=>'E6h',
   Naga=>'E7h',
   Processor=>'E8h',
   Scion=>'E9h',
   Mole=>'EAh',

   Artifact=>'1000h',
   Creature=>'1001h',
   Enchantment=>'1002h',
   Instant=>'1003h',
   Land=>'1004h',
   Plane=>'1005h',
   Planeswalker=>'1006h',
   Scheme=>'1007h',
   Sorcery=>'1008h',
   Tribal=>'1009h',
   Vanguard=>'100Ah',
   Hero=>'100Bh',
   Conspiracy=>'100Ch',
   Phenomenon=>'100Dh',
   Emblem=>'1FFFh',

   Basic=>'2000h',
   Legendary=>'2001h',
   Ongoing=>'2002h',
   Snow=>'2003h',
   World=>'2004h',
   Nonbasic=>'2005h',
   Elite=>'2006h',

   Contraption=>'3000h',
   Equipment=>'3001h',
   Fortification=>'3002h',
   "Horde-Artifact"=>'3003h',
   Clue=>'3004h',

   Aura=>'4000h',
   Curse=>'4010h',
   Shrine=>'4020h',

   Desert=>'5000h',
   Forest=>'5001h',
   Island=>'5002h',
   Lair=>'5003h',
   Locus=>'5004h',
   Mine=>'5005h',
   Mountain=>'5006h',
   Plains=>'5007h',
   "Power-Plant"=>'5008h',
   Swamp=>'5009h',
   Tower=>'500Ah',
   Urzas=>'500Bh',
   "Urza's"=>'500Bh',
   Gate=>'500Ch',

   Arcane=>'6000h',
   Trap=>'6001h',

   Ajani=>'7000h',
   Bolas=>'7001h',
   Chandra=>'7002h',
   Elspeth=>'7003h',
   Garruk=>'7004h',
   Gideon=>'7005h',
   Jace=>'7006h',
   Karn=>'7007h',
   Koth=>'7008h',
   Liliana=>'7009h',
   Nissa=>'700Ah',
   Sarkhan=>'700Bh',
   Sorin=>'700Ch',
   Tezzeret=>'700Dh',
   Venser=>'700Eh',
   Tamiyo=>'700Fh',
   Tibalt=>'7010h',
   Vraska=>'7011h',
   Domri=>'7012h',
   Ral=>'7013h',
   Ashiok=>'7014h',
   Xenagos=>'7015h',
   Dack=>'7016h',
   Kiora=>'7017h',
   Daretti=>'7018h',
   Freyalise=>'7019h',
   Nahiri=>'701Ah',
   Nixilis=>'701Bh',
   Teferi=>'701Ch',
   Ugin=>'701Dh',
   Narset=>'701Eh',
   Arlinn=>'701Fh',

   Alara=>'8000h',
   Arkhos=>'8001h',
   "Bolas's Meditation Realm"=>'8002h',
   Dominaria=>'8003h',
   Equilor=>'8004h',
   Iquatana=>'8005h',
   Ir=>'8006h',
   Kaldheim=>'8007h',
   Kamigawa=>'8008h',
   Karsus=>'8009h',
   Kinshala=>'800Ah',
   Lorwyn=>'800Bh',
   Luvion=>'800Ch',
   Mercadia=>'800Dh',
   Mirrodin=>'800Eh',
   Moag=>'800Fh',
   Muraganda=>'8010h',
   Phyrexia=>'8011h',
   Pyrulea=>'8012h',
   Rabiah=>'8013h',
   Rath=>'8014h',
   Ravnica=>'8015h',
   Segovia=>'8016h',
   "Serra's Realm"=>'8017h',
   Shadowmoor=>'8018h',
   Shandalar=>'8019h',
   Ulgrotha=>'801Ah',
   Valla=>'801Bh',
   Wildfire=>'801Ch',
   Zendikar=>'801Dh',
   Azgol=>'801Eh',
   Belenon=>'801Fh',
   Ergamon=>'8020h',
   Fabacin=>'8021h',
   Innistrad=>'8022h',
   Kephalai=>'8023h',
   Kolbahan=>'8024h',
   Kyneth=>'8025h',
   Mongseng=>'8026h',
   "New Phyrexia"=>'8027h',
   Regatha=>'8028h',
   Vryn=>'8029h',
   Xerex=>'8030h',
  );

my %subtypes =
  (
   Incarnation	=> 1,
   Artificer	=> 2,
   Advisor	=> 3,
   Pest		=> 4,
   Angel	=> 5,
   Snow		=> 6,
   Ape		=> 7,
   Nightstalker	=> 8,
   Archer	=> 9,
   Dryad	=> 10,
   #Aura/Artifact	=> 11,
   Antelope	=> 12,
   Assassin	=> 13,
   Atog		=> 14,
   Avatar	=> 15,
   Archon	=> 16,
   Plant	=> 17,
   Badger	=> 18,
   Aurochs	=> 19,
   Locus	=> 20,
   Blinkmoth	=> 21,
   Basilisk	=> 22,
   Bat		=> 23,
   Bear		=> 24,
   Beast	=> 25,
   Bringer	=> 26,
   Sliver	=> 27,
   Berserker	=> 28,
   Monk		=> 29,
   Boar		=> 30,
   Trap		=> 31,
   Ally		=> 32,
   Ox		=> 33,
   Cyclops	=> 34,
   Camel	=> 35,
   Equipment	=> 36,
   Cephalid	=> 37,
   Chimera	=> 38,
   Centaur	=> 39,
   Cleric	=> 40,
   Shapeshifter	=> 41,
   Construct	=> 42,
   Cockatrice	=> 43,
   #Aura/Artifact Creature	=> 44,
   Curse	=> 45,
   Demon	=> 46,
   Eldrazi	=> 47,
   Devil	=> 48,
   Crab		=> 49,
   Djinn	=> 50,
   Shaman	=> 51,
   Dragon	=> 52,
   Dauthi	=> 53,
   Drake	=> 54,
   Deserter	=> 55,
   Druid	=> 56,
   Dwarf	=> 57,
   Eye		=> 58,
   Dreadnought	=> 59,
   Drone	=> 60,
   Efreet	=> 61,
   Egg		=> 62,
   Elk		=> 63,
   Elder	=> 64,
   Elemental	=> 65,
   Elephant	=> 66,
   Elf		=> 67,
   #Aura/Enchantment	=> 68,
   Nonbasic	=> 69,
   Flagbearer	=> 70,
   Minion	=> 71,
   Fox		=> 72,
   Faerie	=> 73,
   Homunculus	=> 74,
   Frog		=> 75,
   Goat		=> 76,
   Graveborn	=> 77,
   Harpy	=> 78,
   Fungus	=> 79,
   Hellion	=> 80,
   Gargoyle	=> 81,
   Hippo	=> 82,
   Homarid	=> 83,
   Giant	=> 84,
   Gnome	=> 85,
   Goblin	=> 86,
   Lizard	=> 87,
   Hyena	=> 88,
   Jellyfish	=> 89,
   Soldier	=> 90,
   Gorgon	=> 91,
   Hag		=> 92,
   Human	=> 93,
   Horror	=> 94,
   Horse	=> 95,
   Kavu		=> 96,
   Hydra	=> 97,
   Imp		=> 98,
   Kirin	=> 99,
   Kor		=> 100,
   Juggernaut	=> 101,
   Kraken	=> 102,
   Lammasu	=> 103,
   Kithkin	=> 104,
   Knight	=> 105,
   Golem	=> 106,
   Kobold	=> 107,
   #Aura/Land	=> 108,
   Leech	=> 109,
   Unused	=> 110,
   Mountain	=> 111,
   Licid	=> 112,
   Metathran	=> 113,
   Leviathan	=> 114,
   Monger	=> 115,
   Mongoose	=> 116,
   Desert	=> 117,
   Moonfolk	=> 118,
   Mutant	=> 119,
   Mystic	=> 120,
   Bird		=> 121,
   Manticore	=> 122,
   Nautilus	=> 123,
   Nephilim	=> 124,
   Merfolk	=> 125,
   Minotaur	=> 126,
   Ninja	=> 127,
   Masticore	=> 128,
   Octopus	=> 129,
   Myr		=> 130,
   Orb		=> 131,
   Orgg		=> 132,
   Oyster	=> 133,
   Ouphe	=> 134,
   Nightmare	=> 135,
   Nomad	=> 136,
   Island	=> 137,
   Ogre		=> 138,
   Pentavite	=> 139,
   Ooze		=> 140,
   Orc		=> 141,
   Forest	=> 142,
   Pegasus	=> 143,
   Swamp	=> 144,
   Phelddagrif	=> 145,
   Phoenix	=> 146,
   Pincher	=> 147,
   Plains	=> 148,
   Prism	=> 149,
   Rabbit	=> 150,
   Reflection	=> 151,
   Barbarian	=> 152,
   Rhino	=> 153,
   Rat		=> 154,
   Cat		=> 155,
   Rogue	=> 156,
   Rigger	=> 157,
   Salamander	=> 158,
   Scout	=> 159,
   Satyr	=> 160,
   Scarecrow	=> 161,
   Scorpion	=> 162,
   Serpent	=> 163,
   Shade	=> 164,
   #Aura/Permanent	=> 165,
   Fish		=> 166,
   Pirate	=> 167,
   Samurai	=> 168,
   Saproling	=> 169,
   Serf		=> 170,
   Skeleton	=> 171,
   Slug		=> 172,
   Sheep	=> 173,
   Slith	=> 174,
   Spawn	=> 175,
   Specter	=> 176,
   Sphinx	=> 177,
   Spider	=> 178,
   Spirit	=> 179,
   Soltari	=> 180,
   Spellshaper	=> 181,
   Griffin	=> 182,
   Spike	=> 183,
   Turtle	=> 184,
   Thopter	=> 185,
   Treefolk	=> 186,
   Troll	=> 187,
   Basic	=> 188,
   Werewolf	=> 189,
   Unicorn	=> 190,
   Vampire	=> 191,
   Sponge	=> 192,
   Squid	=> 193,
   Squirrel	=> 194,
   Starfish	=> 195,
   Warrior	=> 196,
   Wall		=> 197,
   Praetor	=> 198,
   Thalakos	=> 199,
   Thrull	=> 200,
   Wizard	=> 201,
   Wolverine	=> 202,
   Wolf		=> 203,
   Wombat	=> 204,
   World	=> 205,
   Wraith	=> 206,
   Triskelavite	=> 207,
   Wurm		=> 208,
   Yeti		=> 209,
   Zombie	=> 210,
   #None	=> 211,
   Crocodile	=> 212,
   Illusion	=> 213,
   Insect	=> 214,
   Vedalken	=> 215,
   Volver	=> 216,
   Weird	=> 217,
   Snake	=> 218,
   Sand		=> 219,
   Tetravite	=> 220,
   Whale	=> 221,
   Worm		=> 222,
   Zubera	=> 223,
   "Assembly-Worker"	=> 224,
   Hound	=> 225,
   Lhurgoyf	=> 226,
   Tribal	=> 227,
   Shrine	=> 228,
   Carrier	=> 229,
   Mercenary	=> 230,
   Rebel	=> 231,
   Viashino	=> 232,
  );

my %enchant_subtypes =
  (
   #Search "Enchant clause for Auras" to see the simplifications made to pigeonhole rules text into this
   artifact	=> ['4001h', 11],
     Equipment	=> ['4001h', 11],
   creature	=> ['4002h', 44],
     Wall	=> ['4002h', 44],
   enchantment	=> ['4003h', 68],
   land		=> ['4004h', 108],
     Forest	=> ['4004h', 108],
     Island	=> ['4004h', 108],
     Mountain	=> ['4004h', 108],
     '|H2Plains'=> ['4004h', 108],
     Swamp	=> ['4004h', 108],
     'Forest or Plains'	=> ['4004h', 108],
   permanent	=> ['4005h', 165],
   player       => ['4006h', 234],
     opponent	=> ['4006h', 234],
   instant	=> ['4007h', 233],
   planeswalker	=> ['4008h', 165],
  );

my @hack =	# order is important
  (
   ['a swamp'=>'|Ha s#wamp'],
   ['a Swamp'=>'|Ha S#wamp'],
   [swamps=>'|H1swamps'],
   [Swamps=>'|H1Swamps'],
   [Swamp=>'|H2Swamp'],
   [swamp=>'|H2swamp'],
   ['Ha s#wamp'=>'Ha swamp'],
   ['Ha S#wamp'=>'Ha Swamp'],

   ['an island'=>'|Han i#sland'],
   ['an Island'=>'|Han I#sland'],
   [islands=>'|H1islands'],
   [Islands=>'|H1Islands'],
   [Island=>'|H2Island'],
   [island=>'|H2island'],
   ['Han i#sland'=>'Han island'],
   ['Han I#sland'=>'Han Island'],

   ['a forest'=>'|Ha f#orest'],
   ['a Forest'=>'|Ha F#orest'],
   [forests=>'|H1forests'],
   [Forests=>'|H1Forests'],
   [Forest=>'|H2Forest'],
   [forest=>'|H2forest'],
   ['Ha f#orest'=>'Ha forest'],
   ['Ha F#orest'=>'Ha Forest'],

   ['a mountain'=>'|Ha m#ountain'],
   ['a Mountain'=>'|Ha M#ountain'],
   [mountains=>'|H1mountains'],
   [Mountains=>'|H1Mountains'],
   [Mountain=>'|H2Mountain'],
   [mountain=>'|H2mountain'],
   ['Ha m#ountain'=>'Ha mountain'],
   ['Ha M#ountain'=>'Ha Mountain'],

   ['a plains'=>'|Ha p#lains'],
   ['a Plains'=>'|Ha P#lains'],
   [Plainswalk=>'|H2Plainswalk'],
   [plainswalk=>'|H2plainswalk'],
   [Plainscycling=>'|H2Plainscycling'],
   [plainscycling=>'|H2plainscycling'],
   ['target plains'=>'target |H2plains'],
   ['target Plains'=>'target |H2Plains'],
   ['Target plains'=>'Target |H2plains'],
   ['Target Plains'=>'Target |H2Plains'],
   ['each plains'=>'each |H2plains'],
   ['each Plains'=>'each |H2Plains'],
   ['Each plains'=>'Each |H2plains'],
   ['Each Plains'=>'Each |H2Plains'],
   ['or plains'=>'or |H2plains'],
   ['or Plains'=>'or |H2Plains'],
   [plains=>'|H1plains'],	# Usually no way to automatically distinguish between 1 (plural) and 2 (singular).
   [Plains=>'|H1Plains'],
   [Plains=>'|H2Plains'],
   [plains=>'|H2plains'],
   ['Ha p#lains'=>'Ha plains'],
   ['Ha P#lains'=>'Ha Plains'],
  );

my @sleight =	# order is important
  (
   [black=>'|Sblack'],
   [Black=>'|SBlack'],
   ['nonblack\b'=>'non|Sblack'],
   ['Nonblack\b'=>'Non|Sblack'],
   [q"can't be blocked except by artifact creatures and/or \|Sblack creatures\.\)"=>q"can't be blocked except by artifact creatures and/or black creatures.)"],	# Fear keyword reminder text
   [blue=>'|Sblue'],
   [Blue=>'|SBlue'],
   ['nonblue\b'=>'non|Sblue'],
   ['Nonblue\b'=>'Non|Sblue'],
   [green=>'|Sgreen'],
   [Green=>'|SGreen'],
   ['nongreen\b'=>'non|Sgreen'],
   ['Nongreen\b'=>'Non|Sgreen'],
   ['red(?!uce|istribute)'=>'|Sred'],
   ['Red(?!uce|istribute)'=>'|SRed'],
   ['nonred\b'=>'non|Sred'],
   ['Nonred\b'=>'Non|Sred'],
   [white=>'|Swhite'],
   [White=>'|SWhite'],
   ['nonwhite\b'=>'non|Swhite'],
   ['Nonwhite\b'=>'Non|Swhite'],
  );

sub rgx_protection
{
  my $basecol = shift;
  my $include_everything = shift // 0;
  my $include_all_colors = shift // 0;

  my $rgx = 'Protection (from [a-z]+((, from [a-z]+)?,|and) )?from ';
  if ($include_all_colors)
    {
      if ($include_everything)
	{
	  $rgx .= "(everything|all colors|$basecol)";
	}
      else
	{
	  $rgx .= "(all colors|$basecol)";
	}
    }
  elsif ($include_everything)
    {
      $rgx .= "(everything|$basecol)";
    }
  else
    {
      $rgx .= $basecol;
    }

  return $rgx;
}

my %native_dba =
  (
   Banding=>1,
   Desertwalk=>2,
   'First strike'=>3,
   'Double strike'=>3,
   Flying=>4,
   Forestwalk=>5,
   Vigilance=>6,
   Islandwalk=>7,
   'Legendary landwalk'=>8,
   Mountainwalk=>9,
   Plainswalk=>10,
   Infect=>11,
   (rgx_protection('black'))=>12,
   (rgx_protection('red'))=>13,
   (rgx_protection('white'))=>14,
   Haste=>15,
   Rampage=>16,
#   Regeneration=>17,
   Deathtouch=>18,
   Swampwalk=>19,
   Trample=>20,
   Reach=>21
  );

my %gives_dba =
  (
   Banding=>22,
   'First strike'=>23,
   'Double strike'=>23,
   Flying=>24,
   Forestwalk=>25,
   Vigilance=>26,
   Islandwalk=>27,
   Mountainwalk=>28,
   Plainswalk=>29,
   (rgx_protection('artifacts'))=>30,
   (rgx_protection('black'))=>31,
   (rgx_protection('blue'))=>32,
   (rgx_protection('green'))=>33,
   (rgx_protection('red'))=>34,
   (rgx_protection('white'))=>35,
   Haste=>36,
   Rampage=>37,
#   Regeneration=>38,
   Deathtouch=>39,
   Swampwalk=>40,
   Trample=>41,
   Reach=>42,
  );

#Correct Gatherer errors, and some rare markup that isn't easily automatable
my %fixup_rules =
  (
   'Aeon Chronicler' => ['0\.\(Rather', '0. (Rather'],
   'Dark Maze' => ['defender\.Exile', 'defender. Exile'],
   'Daru Stinger' => ['eachSoldier', 'each Soldier'],
   'Dryad Arbor' => ['Add \|G to your mana pool', 'Add |H|G to your mana pool'],
   'Flash Conscription' => ['turn.That', 'turn. That'],
   "Heretic's Punshment" => ['graveyard.~', 'graveyard. ~'],
   'Jungle Patrol' => [':Put', ': Put'],
   'Knight of Sursi' => ['flanking\(Whenever', 'flanking (Whenever'],

   # These "Plains" should be singular
   'Bant Panorama' => ['Plains', '|H2Plains'],
   'Endless Horizons' => ['Plains', '|H2Plains'],
   'Esper Panorama' => ['Plains', '|H2Plains'],
   'Genju of the Fields' => ['Plains', '|H2Plains'],
   'Gift of Estates' => ['Plains', '|H2Plains'],
   'Karoo' => ['Plains', '|H2Plains'],
   'Lush Growth' => ['Plains', '|H2Plains'],

   'Tithe' => ['additional Plains', 'additional |H2Plains'],
  );

our $lastname = undef;	# contains the full name of the current card (once its parsed) or of the previous card (while parsing the current card's name)

our $rules;	# contains the rules text (without reminder text) of the current card, with the card name replaced by ~

sub native_ability
{
  my $abil = shift;
  my $r = $rules;
  $r =~ s/("|\b(has|have|gains?|gets?|loses?|if|[wW]hen(ever)?)\b).*$//mg;
  return ($r =~ /(^|[,.;]\s*)$abil($|[,.;]| \()/mi) ? 1 : 0;
}
sub gives_ability
{
  my $abil = shift;
  return ($rules =~ /(\b(has|have|gains?|gets?)\b).*\b(?i)$abil\b/) ? 2 : 0;
}
sub loses_ability
{
  my $abil = shift;
  return ($rules =~ /\bloses?\b.*\b(?i)$abil\b/) ? 4 : 0;
}
sub any_ability
{
  my $abil = shift;
  return 0 unless ($rules =~ /\b$abil\b/i);
  return native_ability($abil) | gives_ability($abil) | loses_ability($abil);
}

sub process_pt
{
  my $card = shift;
  my $lml = shift;
  my $lct = shift;
  my $srckey = shift;
  my $destkey = ucfirst $srckey;

  if ($card->{$srckey} eq '*')
    {
      $lml->{$destkey} = 100;
      $lct->{$destkey} = 16384;
    }
  elsif ($card->{$srckey} =~ /^([0-9]+)\+\*$/)
    {
      $lml->{$destkey} = 100 + $1;
      $lct->{$destkey} = 16384 + $1;
    }
  elsif ($card->{$srckey} =~ /^([0-9]+)-\*$/)
    {
      $lml->{$destkey} = 200 + $1;
      $lct->{$destkey} = 16384;
    }
  elsif ($card->{$srckey} =~ /^[0-9]+$/
	 || $card->{$srckey} eq '-1')	# Char-Rumbler, Spinal Parasite
    {
      $lml->{$destkey} = $lct->{$destkey} = $card->{$srckey};
    }
  else
    {
      die qq[Couldn't parse $srckey for card "$lastname": "$card->{$srckey}"];
    }
}

sub fix_encoding
{
  my $str = shift;

  my $win1252 = encode("windows-1252", $str);
  if ($win1252 !~ /\?/)
    {
      return $win1252;
    }

  my $outstr = '';
 OUTER:
  foreach my $utf8 (split //, $str)
    {
      my $win1252 = encode("windows-1252", $utf8);
      if ($win1252 ne '?' || $utf8 eq '?')
	{
	  $outstr .= $win1252;
	}
      else
	{
	  use bytes;
	  foreach my $c (145..151)	# Windows-1252 characters that somehow made it through unscathed
	    {
	      if ($utf8 eq chr($c))
		{
		  $outstr .= chr($c);
		  next OUTER;
		}
	    }

	  if ($utf8 eq chr(226).chr(136).chr(146))	# minus sign
	    {
	      $outstr .= "-";
	    }
	  elsif ($utf8 eq chr(194).chr(147))	# left curly quote
	    {
	      $outstr .= chr(147);
	    }
	  elsif ($utf8 eq chr(194).chr(148))	# right curly quote
	    {
	      $outstr .= chr(148);
	    }
	  else
	    {
	      my $errmsg = "Unknown utf8 character: " . (join '', map { "[".ord($_)."]" } split(//, $utf8));
	      no bytes;
	      mywarn $errmsg . " = '" . $utf8 . "'";
	    }
	  no bytes;
	}
    }

  return $outstr;
}

sub print_fixed_encoding
{
  my $outfile = shift;
  my $str = shift;
  print $outfile fix_encoding($str);
}

my %add_ml;
my %add_ct;
my %id2name;
my %name2id;
my %dfc_fronts;	# cardname of front => id of back
my %dfc_backs;	# cardname of back => id of front
my $delayed_fatals = 0;

$n = 0;
my @newml;

XML_LOOP: foreach my $card (@{$cards})
  {
    ++$n;
    my %lml;
    my %lct;
    { # name
      exists $card->{name} or die "No name for card #$n", (defined $lastname ? qq' (last successful="$lastname")' : '');
      $lastname = $card->{name};
      #say qq'#$n: "$lastname"';

      exists $card->{type} or die qq'No type for card "$lastname"';

      if ($card->{type} =~ /\bToken\b/ && !$opt_tokens)
	{
	  mywarn qq'Skipping token "$lastname"';
	  next XML_LOOP;
	}

      if ($card->{type} =~ /\bVanguard\b/)
	{
	  $lastname = '** AVATAR - ' . $lastname;
	}
      elsif ($card->{type} =~ /\b(Plane|Phenomenon)\b/)
	{
	  $lastname = '*** ' . $lastname;
	}

      $lml{'Full Name'} = $lml{'Short Name'} = $lct{'Name (will be cut down)'} = $lct{'Full Card Name in CSV (must be exactly the same)'} = $lastname;
      if ($card->{type} =~ /\bConspiracy\b/)
	{
	  $lml{'Full Name'} = "** CONSPIRACY \x{2013} " . $lastname;
	  $lastname = "** CONSPIRACY - " . $lastname;
	}
    }

    #say STDERR $n, ": '$lastname'";

    my $rarity;
    { # rarity
      exists $card->{rarity} or die qq'No rarity for card "$lastname"';
      $rarity = $card->{rarity};
      ref($rarity) eq 'HASH' and $rarity = 'M';	# Vintage Masters Power9
      if ($rarity =~ m!^([CURMT]) // \1$!)
	{
	  $rarity = $1;
	}
      $rarity eq 'T' and $rarity = 'S';	# Time Spiral Timeshifted
      $lml{Rarity} = ($token ? 0
		      : ($rarity eq 'C' && 1
			 or $rarity eq 'U' && 4
			 or $rarity eq 'S' && 3
			 or $rarity =~ /^[RM]$/ && 2
			 or die qq[Couldn't parse rarity for card "$lastname": "$rarity"]));
      if ($rarity eq 'C' && $lastname =~ /^([Ss]wamp|[Ii]sland|[Ff]orest|[Mm]ountain|[Pp]lains)/)
	{
	  $rarity = 'L';	# Gatherer Extractor doesn't seem to understand Land rarity
	}
    }

    { # flavor text
      exists $card->{flavor} or die qq'No flavor text for card "$lastname"';
      if (ref $card->{flavor})
	{
	  $lml{'Flavor Text'} = '';
	}
      else
	{
	  my $flav = $card->{flavor};
	  $flav =~ s/\x{a3}/\\n/g;	# ukp sign - translate to newline
	  $flav =~ s/#_//g;	# begin italics
	  $flav =~ s/_#//g;	# end italics
	  $flav =~ s/\{(.*?)\}/|$1/g;
	  $flav =~ /^\|[BUGRW]$/ and $flav = '';	# Basic lands
	  $flav =~ s/"([^"]*?)"/\223$1\224/g;	# curly quotes
	  $lml{'Flavor Text'} = $flav;
	}
    }

    if ($opt_update || !$updates_only)
      { # sets
	my $dup = exists $add_ml{$lastname};
	my $printdup = $dup && !$opt_update && $lastname !~ /^(Plains|Island|Swamp|Mountain|Forest)$/;
	if ($printdup)
	  {
	    $opt_quiet or print STDERR qq[${color_darkblue}Duplicate card "$lastname".\tAlready in sets: ], join(", ", map { qq'"$_"' } grep { exists $add_ml{$lastname}->{$_} && $add_ml{$lastname}->{$_} ne '-' } @realsets);
	  }
	if (!$opt_update)
	  {
	    scalar @{$card->{set}} > 0 or mywarn qq[Card "$lastname" not assigned to any sets];
	    foreach my $k (@{$card->{set}})
	      {
		unless (exists $code{$k})
		  {
		    if (!exists $ignore_set_codes{$k})
		      {
			my $clrnrm = $printdup ? $color_normal : "";
			die qq[${clrnrm}Card "$lastname" assigned to unrecognized set code "$k"];
		      }
		    next;
		  }
		my $found = 0;
		foreach my $kk (@sets)
		  {
		    if ($kk eq $code{$k})
		      {
			if ($dup)
			  {
			    if ($printdup && !$opt_quiet)
			      {
				print STDERR $found == 0 ? qq';\tadding "$kk"' : qq', "$kk"';
			      }
			    $add_ml{$lastname}->{$kk} = $rarity;
			  }
			else
			  {
			    $lml{$kk} = $rarity;
			  }
			++$found;
		      }
		  }
		$found > 0 or mywarn qq[Couldn't find set for card "$lastname"'s set code "$k" despite earlier verification];
	      }
	  }
	if ($dup)
	  {
	    if ($printdup && !$opt_quiet)
	      {
		print STDERR $color_normal, "\n";
	      }
	    if ($lml{'Flavor Text'} ne ''
		&& $add_ml{$lastname}->{'Flavor Text'} eq '')
	      {
		$add_ml{$lastname}->{'Flavor Text'} = $lml{'Flavor Text'};
	      }
	    next XML_LOOP;
	  }
      }

    $rules = '';
    { # rules text, mana source colors
      $lml{'Mana Source Colors'} = 0;
      exists $card->{ability} or die qq'No rules text for card "$lastname"';
      if (ref $card->{ability})
	{
	  $lml{'Rules Text'} = '';
	}
      else
	{
	  $rules = $card->{ability};
	  $rules =~ s/\x{a3}/\n/g;	# ukp sign - translate to newline
	  $rules =~ s/#_//g;	# begin italics
	  $rules =~ s/_#//g;	# end italics
	  $rules =~ s/^\s+//gm;
	  $rules =~ s/\s+$//gm;
	  $rules =~ s/\x{2014}\x{2014}\x{2014}/--FLIP--/g;
	  $rules =~ s/\{S\}/|I/g;	# Snow mana symbol
	  $rules =~ s/\{(.*?)\}/|$1/g;	# Other mana symbols
	  $rules =~ s/CHAOS/|A/g;	# Planechase Chaos symbol
	  $rules =~ s/\b\Q$lastname\E\b/~/g;	# prevent substitution of e.g. Island Fish Jasconius to |H2Island Fish Jasconius

	  if (exists $fixup_rules{$lastname})
	    {
	      my $srch = $fixup_rules{$lastname}[0];
	      my $repl = $fixup_rules{$lastname}[1];
	      $rules =~ s/$srch/$repl/g;
	    }

	  my $rules_text = $rules;	# $rules_text will be marked up for eventual inclusion; $rules text is semi-raw for later matching.

	  #Hack marks
	  if ($rules_text =~ /[Ss]wamp|[Ii]sland|[Ff]orest|[Mm]ountain|[Pp]lains/
	      && $lastname !~ /^(Magical Hack|Mind Bend|Whim of Volrath)$/)
	    {
	      foreach my $w (@hack)
		{
		  $rules_text =~ s/\b$w->[0]/$w->[1]/g;
		}
	    }
	  #Hack marks for mana symbols in reminder text
	  $rules_text =~ s/(\(\|T: Add )(\|[BUGRW])( or )(\|[BUGRW])( to your mana pool\.\))/$1|H$2$3|H$4$5/g;
	  $rules_text =~ s/(\(\|T: Add )(\|[BUGRW])( to your mana pool\.\))/$1|H$2$3/g;

	  if ($rules_text =~ /[Bb]lack|[Bb]lue|[Gg]reen|[Rr]ed|[Ww]hite/)
	    {
	      if ($lastname eq 'Balduvian Shaman')
		{
		  $rules_text =~ s/\bwhite\b/|Swhite/g;	# other color words are in reminder text
		}
	      elsif ($lastname !~ /^(Sleight of Mind|Mind Bend|Whim of Volrath)$/)
		{
		  foreach my $w (@sleight)
		    {
		      $rules_text =~ s/\b$w->[0]/$w->[1]/g;
		    }
		}
	    }

	  $rules_text =~ s/~/$lastname/g;
	  $rules_text =~ s/\n/\\n/g;	# translate actual newlines to newline markers
	  $rules_text =~ s/"([^"]*?)"/\223$1\224/g;	# curly quotes
	  $lml{'Rules Text'} = $rules_text;

	  if (!$opt_update)
	    {
	      my $msc = 0;
	      my $msct = $card->{ability};
	      $msct =~ s/\x{a3}/\n/g;	# ukp sign - translate to newline
	      $msct =~ s/#_.*?_#//g;	# remove italicized text
	      $msct =~ s/^"[^"]*?"/$1/g;	# remove granted abilities
	      while ($msct =~ s/: .*?\b[aA]dd (.*?) to your mana pool/: /)
		{
		  my $msc_this_round = 0;
		  my $mana = $1;
		  my $pristine = $mana;
		  if ($mana =~ /^(X|one|two|three|four|five|six|seven|eight|nine|ten|that much) mana (in any combination of colors|of any (one )?color|of the chosen color|of that color|of any of the exiled card's colors)$/)
		    {
		      $msc = 62;	# BUGRW
		      last;
		    }
		  if ($mana =~ /^(X|one|two|three|four|five|six|seven|eight|nine|ten|that much) mana of any type that land could produce$/
		      || $mana eq q"Ice Cauldron's last noted type and amount of mana"
		      || $mana eq q"one mana of Jeweled Amulet's last noted type")
		    {
		      $msc = 63;	# XBUGRW
		      last;
		    }
		  while ($mana =~ s/\{([CBUGRWX]|[0-9]+)\}//)
		    {
		      my $col = $1;
		      exists $mana_test{$col} or die qq[Couldn't parse mana production for card $lastname: raw "$pristine", unknown "{$col}"];
		      $msc_this_round |= $mana_test{$col};
		    }

		  if ($lastname eq 'Paliano, the High City')
		    {
		      $msc_this_round = 0;
		    }
		  elsif ($msc_this_round == 0)
		    {
		      die qq[Couldn't parse any mana production for card $lastname: "$card->{ability}", this ability "$pristine"];
		    }
		  $msc |= $msc_this_round;
		}
	      if ($msc)
		{
		  $lml{'Mana Source Colors'} = $msc;
		  $lml{'DB Card Type 2'} = 10;
		}
	    }
	}
    }

    my $equipment = 0;
    my $token = 0;
    my $emblem = 0;
    { # type, subtypes
      exists $card->{type} or die qq'No type for card "$lastname"';
      my $type = $card->{type};
      $type =~ s/\x{2019}/'/g;	# Curly apostrophe
      if ($type =~ s/^Token //)
	{
	  $token = 1;
	}
      if ($type =~ m/\bEmblem\b/)
	{
	  $emblem = 1;
	  $lml{'Short Name'} = $type;
	}
      $lml{'Type Text'} = $type;
      if ($type =~ /\bPlaneswalker\b/ && (exists $card->{loyalty}) && !(ref $card->{loyalty}) && $card->{loyalty} =~ /./)
	{
	  $lml{'Type Text'} .= ' (' . $card->{loyalty} . ')';
	  $lml{hack_loyalty} = $card->{loyalty};
	}
      if ($lml{'Type Text'} =~ /\b(Swamp|Island|Forest|Mountain|Plains)\b/)
	{
	  foreach my $w (qw/Swamp Island Forest Mountain Plains/)
	    {
	      $lml{'Type Text'} =~ s/\b$w\b/|H2$w/g;
	    }
	}

      unless ($opt_update && ($type =~ /^Summon / or grep { /^(UNH|UG)$/ } @{$card->{set}}))	# From Unglued/Unhinged, or old-style summon card.  Likely to just break.
	{
	  $lml{'Card Type'} = ($type =~ /\bLand\b/ && 5
			       or $type =~ /\bArtifact\b/ && 1
			       or $type =~ /\bCreature\b/ && 7
			       or $type =~ /\b(Enchantment|Planeswalker)\b/ && 2
			       or $type =~ /\bSorcery\b/ && 6
			       or $type =~ /\bInstant\b/ && 3
			       or $type =~ /\b(Conspiracy|Scheme|Phenomenon|Plane)\b/ && 2
			       or $type =~ /\bEmblem\b/ && 2
			       or die qq[Couldn't parse type for card "$lastname": "$type"]);

	  $lct{'Type: Effect'} = $type =~ /\bEmblem\b/ ? 1 : 0;
	  $lct{'Type: Artifact'} = $type =~ /\bArtifact\b/ ? 1 : 0;
	  $lct{'Type: Interrupt'} = 0;
	  $lct{'Type: Instant'} = ($type =~ /\bInstant\b/
				   or $type =~ /\b(Creature|Land|Enchantment|Planeswalker)\b/
				   && native_ability('flash')) ? 1 : 0;
	  $lct{'Type: Sorcery'} = $type =~ /\bSorcery\b/ ? 1 : 0;
	  $lct{'Type: Enchantment'} = $type =~ /\b(Enchantment|Planeswalker)\b/ ? 1 : 0;
	  $lct{'Type: Creature'} = $type =~ /\bCreature\b/ ? 1 : 0;
	  $lct{'Type: Land'} = $type =~ /\bLand\b/ ? 1 : 0;

	  if ($type =~ /\bWorld\b/)
	    {
	      $lct{Family} = 16;
	    }
	  elsif ($type =~ /\bLegendary\b/)
	    {
	      if ($type =~ /\bLand\b/ && $type !~ /\bCreature\b/)
		{
		  $lct{Family} = 15;
		}
	      else
		{
		  $lct{Family} = 14;
		}
	    }
	  else
	    {
	      $lct{Family} = ($type =~ /\bBasic\b/ && $type =~ /\bLand\b/ && 13
			      or $type =~ /\bLand\b/ && $type =~ /\b([Ss]wamp|[Ii]sland|[Ff]orest|[Mm]ountain|[Pp]lains)\b/ && 12
			      or $type =~ /\bElephant\b/ && 10
			      or $type =~ /\bOrc\b/ && 9
			      or $type =~ /\bLand\b/ && ($type =~ /\bArtifact\b/ || ($type =~ /\bUrza's\b/ && $type =~ /\b(Tower|Mine|Power-Plant)\b/)) && 8
			      or $type =~ /\bRat\b/ && 7
			      or $type =~ /\bDjinn\b/ && 6
			      or $type =~ /\bEfreet\b/ && 5
			      or $type =~ /\bDwarf\b/ && 4
			      or $type =~ /\bGoblin\b/ && 3
			      or $type =~ /\bZombie\b/ && 2
			      or $type =~ /\bMerfolk\b/ && 1
			      or !native_ability('defender') && -1
			      or 0);	# must be last, of course, hence the negated condition for defender just above
	    }

	  # New Types.
	  # Ongoing, Plane, Planeswalker, Tribal, and Vanguard must always be first if present at all.
	  # Scheme must always be first or second if present at all.
	  my @new_types;
	  my $t = $type;
	  my @force_type1 = ([Ongoing=>'2002h', Plane=>'1005h', Planeswalker=>'1006h', Tribal=>'1009h', Vanguard=>'100Ah']);
	  foreach my $ft (@force_type1)
	    {
	      my $r = '\b' . $ft->[0] . '\b';
	      if ($t =~ s/$r//)
		{
		  scalar @new_types == 0 or die qq'card "$lastname" has type $ft->[0] but already had New Types 1 set: "$type"';
		  push @new_types, $ft->[1];
		}
	    }
	  if ($t =~ s/\bScheme\b//)
	    {
	      scalar @new_types <= 1 or die qq'card "$lastname" has type Scheme but already had New Types 1 and 2 set: "$type"';
	      push @new_types, '1007h';
	    }
	  $t =~ s/^\s+//;
	  $t =~ s/\x{2014}//;	# emdash
	  $t =~ s!^([A-Za-z]+) // \1$!$1!g;	# split cards (same type)
	  while ($t ne '')
	    {
	      my $word;
	      if ($type =~ /\bPlane\b/ && $t !~ /^Plane /)	# Plane subtypes can include spaces, sigh
		{
		  $t =~ s/^(.+)\b\s*$//;
		  $word = $1;
		}
	      else
		{
		  $t =~ s/^(\S+)\b\s*//;
		  $word = $1;
		}
	      if (!exists $newtypes{$word})
		{
		  if ($opt_update && $word =~ /^(Chicken)$/)
		    {
		      next;	# Unimplemented
		    }
		  if ($word =~ /^(Token)$/)
		    {
		      next;	# Unused
		    }
		  ++$delayed_fatals;
		  mywarn qq'card "$lastname" has unknown supertype/type/subtype "$word": "$type"';
		}
	      unless (scalar @new_types <= 6)
		{
		  say qq'"$_"' foreach @new_types;
		  die qq'card "$lastname" has supertype/type/subtype "$word" but already had New Types 1-7 set: "$type"';
		}
	      if (!$equipment && $word eq 'Equipment')
		{
		  $equipment = 1;
		  if ($opt_equipment)
		    {
		      $all_equipment{$lastname} = 1;
		    }
		}
	      push @new_types, $newtypes{$word};
	    }
	  #Enchant clause for Auras
	  my $aura_subt1 = undef;
	  my $aura_subt2 = undef;
	  if ($type =~ /\bAura\b/)
	    {
	      unless (scalar @new_types <= 6)
		{
		  say qq'"$_"' foreach @new_types;
		  die qq'card "$lastname" is an Aura but already had New Types 1-7 set: "$type"';
		}

	      $rules =~ /^Enchant (.+?)$/m or die qq'card "$lastname" is an Aura but has no Enchant clause: "$rules"';
	      my $enchant = $1;
	      $enchant =~ s/ \(.+$//;	# reminder text
	      $enchant =~ s/ card in a graveyard$//;	# e.g. Animate Dead
	      $enchant =~ s/ (you|an opponent) controls?$//;	# e.g. Krovikan Plague
	      $enchant =~ s/ with another Aura attached to it$//;	# Daybreak Coronet
	      $enchant =~ s/^non(-?[a-zA-Z]+) //;	# e.g. Krovikan Plague
	      $enchant =~ s/^creature with (power|converted mana cost) [0-9]+ or less/creature/;	# e.g. Threads of Disloyalty
	      $enchant =~ s/^((white|blue|black|red|green) or )?(white|blue|black|red|green) creature/creature/;	# e.g. Controlled Instincts
	      $enchant =~ s/^creature without flying$/creature/;	# e.g. Roots
	      $enchant =~ s/^tapped //;	# e.g. Glimmerdust Nap

	      if ($enchant eq 'artifact creature')	# e.g. Domineer
		{
		  $enchant = 'artifact or creature';
		}
	      if ($enchant =~ /^([a-z]+), ([a-z]+), or ([a-z]+)$/)	# e.g. Imprisoned in the Moon
		{
		  my $enchant1 = $1;
		  my $enchant2 = $2;
		  my $enchant3 = $3;
		  exists $enchant_subtypes{$enchant1} or die qq'card "$lastname" has unknown triple Enchant clause: "$enchant" => "$enchant1"';
		  exists $enchant_subtypes{$enchant2} or die qq'card "$lastname" has unknown triple Enchant clause: "$enchant" => "$enchant2"';
		  exists $enchant_subtypes{$enchant3} or die qq'card "$lastname" has unknown triple Enchant clause: "$enchant" => "$enchant3"';
		  unless (scalar @new_types <= 4)
		    {
		      say qq'"$_"' foreach @new_types;
		      die qq'card "$lastname" is an Aura with three enchant clauses but already had New Types 1-5 set: "$enchant" => "$enchant1", "$enchant2", "$enchant3"';
		    }
		  push @new_types, $enchant_subtypes{$enchant1}[0];
		  push @new_types, $enchant_subtypes{$enchant2}[0];
		  push @new_types, $enchant_subtypes{$enchant3}[0];
		  if ($enchant_subtypes{$enchant1}[1] eq $enchant_subtypes{$enchant2}[1])
		    {
		      $aura_subt1 = $enchant_subtypes{$enchant1}[1];
		      $aura_subt2 = $enchant_subtypes{$enchant3}[1];
		    }
		  else	# third one gets lost :(
		    {
		      $aura_subt1 = $enchant_subtypes{$enchant1}[1];
		      $aura_subt2 = $enchant_subtypes{$enchant2}[1];
		    }
		  if ($aura_subt1 eq $aura_subt2)
		    {
		      $aura_subt2 = undef;
		    }
		}
	      elsif ($enchant =~ /^([a-z]+) or ([a-z]+)$/)	# e.g. Quiet Disrepair
		{
		  my $enchant1 = $1;
		  my $enchant2 = $2;
		  exists $enchant_subtypes{$enchant1} or die qq'card "$lastname" has unknown double Enchant clause: "$enchant" => "$enchant1"';
		  exists $enchant_subtypes{$enchant2} or die qq'card "$lastname" has unknown double Enchant clause: "$enchant" => "$enchant2"';
		  unless (scalar @new_types <= 5)
		    {
		      say qq'"$_"' foreach @new_types;
		      die qq'card "$lastname" is an Aura with two enchant clauses but already had New Types 1-5 set: "$enchant" => "$enchant1", "$enchant2"';
		    }
		  push @new_types, $enchant_subtypes{$enchant1}[0];
		  push @new_types, $enchant_subtypes{$enchant2}[0];
		  $aura_subt1 = $enchant_subtypes{$enchant1}[1];
		  if ($enchant_subtypes{$enchant1}[1] ne $enchant_subtypes{$enchant2}[1])
		    {
		      $aura_subt2 = $enchant_subtypes{$enchant2}[1];
		    }
		}
	      else
		{
		  exists $enchant_subtypes{$enchant} or die qq'card "$lastname" has unknown Enchant clause: "$enchant"';
		  push @new_types, $enchant_subtypes{$enchant}[0];
		  $aura_subt1 = $enchant_subtypes{$enchant}[1];
		}
	    }

	  while (scalar @new_types < 7)
	    {
	      push @new_types, -1;
	    }
	  $lml{'New Types 1'} = $new_types[0];
	  $lml{'New Types 2'} = $new_types[1];
	  $lml{'New Types 3'} = $new_types[2];
	  $lml{'New Types 4'} = $new_types[3];
	  $lml{'New Types 5'} = $new_types[4];
	  $lml{'New Types 6'} = $new_types[5];
	  $lml{'New Types 7'} = $new_types[6];

	  #Subtype and Subtype 2 are funny.  Value 211 for None; Artifact Creatures always get 44 in Subtype 1;
	  #Auras get a subtype in Subtype 1 or 2 according to their Enchant clause (Enchant-Player and Enchant-Instant are valid only in Subtype 1);
	  #non-Aura enchantments need either 211 (None) or 205 (World) in Subtype 1.
	  my @sub_types;

	  my $is_aura = 0;
	  if ($type =~ s/\bAura\b//)
	    {
	      $is_aura = 1;
	      if (defined $aura_subt1)
		{
		  push @sub_types, $aura_subt1;
		  defined $aura_subt2 and push @sub_types, $aura_subt2;
		}
	      else
		{
		  die qq'No aura subtypes found for card "$lastname"';
		}
	    }

	  $t = $type;
	  $t =~ s/^\s+//;
	  $t =~ s/\x{2014}//;	# emdash
	  $t =~ s!^([A-Za-z]+) // \1$!$1!g;	# split cards (same type)
	  while ($t ne '')
	    {
	      last if (scalar @sub_types == 2);
	      $t =~ s/^(\S+)\b\s*//;
	      my $word = $1;
	      next unless exists $subtypes{$word};
	      push @sub_types, $subtypes{$word};
	    }

	  if ($lml{'Card Type'} eq 1	# Artifact
	      && $type =~ /\bArtifact Creature\b/)
	    {
	      unshift @sub_types, 44;
	    }

	  while (scalar @sub_types < 2)
	    {
	      push @sub_types, 211;
	    }

	  if ($lml{'Card Type'} eq 2)	# Enchantment
	    {
	      if (!$is_aura)
		{
		  if ($sub_types[0] eq 227 && $sub_types[1] ne 211)	# First subtype's Tribal, second will be lost; swap them so Tribal's lost instead
		    {
		      $sub_types[0] = $sub_types[1];
		      $sub_types[1] = 227;
		    }

		  if ($sub_types[1] eq 205)	# World
		    {
		      $sub_types[1] = $sub_types[0];
		      $sub_types[0] = 205;
		    }
		  elsif ($sub_types[0] ne 205 && $sub_types[1] eq 211)	# None
		    {
		      $sub_types[1] = $sub_types[0];
		      $sub_types[0] = 211;
		    }
		  elsif ($sub_types[0] ne 205 && $sub_types[0] ne 211)
		    {
		      unshift @sub_types, 211;
		    }
		}
	      # match zany Deckdll.dll validation
	      if (!($sub_types[0] eq 211	# none
		    || $sub_types[0] eq 205	# enchant world
		    || $sub_types[0] eq 108 || $sub_types[1] eq 108	# enchant land
		    || $sub_types[0] eq 197 || $sub_types[1] eq 197	# wall
		    || $sub_types[0] eq 44 || $sub_types[1] eq 44	# enchant creature
		    || $sub_types[0] eq 11 || $sub_types[1] eq 11	# enchant artifact
		    || $sub_types[0] eq 68 || $sub_types[1] eq 68	# enchant enchantment
		    || $sub_types[0] eq 234 # enchant player
		    || $sub_types[0] eq 233 # enchant instant
		    || $sub_types[0] eq 165 || $sub_types[1] eq 165))	# enchant permanent
		{
		  die qq'Bad subtypes for enchantment "$lastname": ', $sub_types[0], '/', $sub_types[1];
		}
	    }

	  $lml{'Subtype'} = $sub_types[0];
	  $lml{'Subtype 2'} = $sub_types[1];

	  # This is, obviously, very incomplete
	  $lct{'cc[2]'} = ($type =~ /\bPlaneswalker\b/ ? 9 : 0);
	}
    }

    if ($opt_update)
      {
	$add_ml{$lastname} = \%lml;
	next XML_LOOP;
      }

    { # Split cards aren't parsing manacost or color right.
      if ($card->{color} eq "C // C")
	{
	  $lastname eq "Alive // Well" and $card->{manacost} = q[{3}{G} // {W}] and $card->{color} = "G // W"
	    or $lastname eq "Armed // Dangerous" and $card->{manacost} = q[{1}{R} // {3}{G}] and $card->{color} = "R // G"
	    or $lastname eq "Assault // Battery" and $card->{manacost} = q[{R} // {3}{G}] and $card->{color} = "R // G"
	    or $lastname eq "Beck // Call" and $card->{manacost} = q[{G}{U} // {4}{W}{U}] and $card->{color} = "GU // WU"
	    or $lastname eq "Boom // Bust" and $card->{manacost} = q[{1}{R} // {5}{R}] and $card->{color} = "R // R"
	    or $lastname eq "Bound // Determined" and $card->{manacost} = q[{3}{B}{G} // {G}{U}] and $card->{color} = "BG // GU"
	    or $lastname eq "Breaking // Entering" and $card->{manacost} = q[{U}{B} // {4}{B}{R}] and $card->{color} = "UB // BR"
	    or $lastname eq "Catch // Release" and $card->{manacost} = q[{1}{U}{R} // {4}{R}{W}] and $card->{color} = "UR // RW"
	    or $lastname eq "Crime // Punishment" and $card->{manacost} = q[{3}{W}{B} // {X}{B}{G}] and $card->{color} = "WB // BG"
	    or $lastname eq "Dead // Gone" and $card->{manacost} = q[{R} // {2}{R}] and $card->{color} = "R // R"
	    or $lastname eq "Down // Dirty" and $card->{manacost} = q[{3}{B} // {2}{G}] and $card->{color} = "B // G"
	    or $lastname eq "Far // Away" and $card->{manacost} = q[{1}{U} // {2}{B}] and $card->{color} = "U // B"
	    or $lastname eq "Fire // Ice" and $card->{manacost} = q[{1}{R} // {1}{U}] and $card->{color} = "R // U"
	    or $lastname eq "Flesh // Blood" and $card->{manacost} = q[{3}{B}{G} // {R}{G}] and $card->{color} = "BG // RG"
	    or $lastname eq "Give // Take" and $card->{manacost} = q[{2}{G} // {2}{U}] and $card->{color} = "G // U"
	    or $lastname eq "Hide // Seek" and $card->{manacost} = q[{R}{W} // {W}{B}] and $card->{color} = "RW // WB"
	    or $lastname eq "Hit // Run" and $card->{manacost} = q[{1}{B}{R} // {3}{R}{G}] and $card->{color} = "BR // RG"
	    or $lastname eq "Illusion // Reality" and $card->{manacost} = q[{U} // {2}{G}] and $card->{color} = "U // G"
	    or $lastname eq "Life // Death" and $card->{manacost} = q[{G} // {1}{B}] and $card->{color} = "G // B"
	    or $lastname eq "Night // Day" and $card->{manacost} = q[{B} // {2}{W}] and $card->{color} = "B // W"
	    or $lastname eq "Odds // Ends" and $card->{manacost} = q[{U}{R} // {3}{R}{W}] and $card->{color} = "UR // RW"
	    or $lastname eq "Order // Chaos" and $card->{manacost} = q[{3}{W} // {2}{R}] and $card->{color} = "W // R"
	    or $lastname eq "Pain // Suffering" and $card->{manacost} = q[{B} // {3}{R}] and $card->{color} = "B // R"
	    or $lastname eq "Profit // Loss" and $card->{manacost} = q[{1}{W} // {2}{B}] and $card->{color} = "W // B"
	    or $lastname eq "Protect // Serve" and $card->{manacost} = q[{2}{W} // {1}{U}] and $card->{color} = "W // U"
	    or $lastname eq "Pure // Simple" and $card->{manacost} = q[{1}{R}{G} // {1}{G}{W}] and $card->{color} = "RG // GW"
	    or $lastname eq "Ready // Willing" and $card->{manacost} = q[{1}{G}{W} // {1}{W}{B}] and $card->{color} = "GW // WB"
	    or $lastname eq "Research // Development" and $card->{manacost} = q[{G}{U} // {3}{U}{R}] and $card->{color} = "GU // UR"
	    or $lastname eq "Rise // Fall" and $card->{manacost} = q[{U}{B} // {B}{R}] and $card->{color} = "UB // BR"
	    or $lastname eq "Rough // Tumble" and $card->{manacost} = q[{1}{R} // {5}{R}] and $card->{color} = "R // R"
	    or $lastname eq "Spite // Malice" and $card->{manacost} = q[{3}{U} // {3}{B}] and $card->{color} = "U // B"
	    or $lastname eq "Stand // Deliver" and $card->{manacost} = q[{W} // {2}{U}] and $card->{color} = "W // U"
	    or $lastname eq "Supply // Demand" and $card->{manacost} = q[{X}{G}{W} // {1}{W}{U}] and $card->{color} = "GW // WU"
	    or $lastname eq "Toil // Trouble" and $card->{manacost} = q[{2}{B} // {2}{R}] and $card->{color} = "B // R"
	    or $lastname eq "Trial // Error" and $card->{manacost} = q[{W}{U} // {U}{B}] and $card->{color} = "WU // UB"
	    or $lastname eq "Turn // Burn" and $card->{manacost} = q[{2}{U} // {1}{R}] and $card->{color} = "U // R"
	    or $lastname eq "Wax // Wane" and $card->{manacost} = q[{G} // {W}] and $card->{color} = "G // W"
	    or $lastname eq "Wear // Tear" and $card->{manacost} = q[{1}{R} // {W}] and $card->{color} = "R // W"
	    or die qq{Couldn't fixup manacost/color for card "$lastname"};
	}
    }

    my $hybrid;
    { # mana cost
      exists $card->{manacost} or die qq'No manacost for card "$lastname"';
      my $cost = $card->{manacost};
      if (ref $cost eq "")
	{}
      elsif (ref $cost eq "HASH")
	{
	  $cost = "";
	}
      else
	{
	  die qq'Unexpected data for manacost for card "$lastname": "', (ref $cost), '"';
	}
      my $splitcard = 0;
      if ($cost =~ s@ // .*$@@g)
	{
	  $splitcard = 1;
	}
      $cost =~ s/\{([0-9]+|X+|[CBUGRW]|[2PBUGRW][BUGRW])\}/|$1/g;
      $cost =~ /[{}]/ and die qq'Could not fully convert manacost for card "$lastname": raw "$card->{manacost}", partial="$cost"';
      $lml{'Cost Text'} = $cost;

      $lml{'Req Colorless'} = 0;
      foreach my $mcp (keys %mana_req)
	{
	  $lml{$mana_req{$mcp}} = 0;
	}

      my $cmcx = 0;
      my $cmcc = 0;
      $hybrid = $cost =~ /\|[2PBUGRW][BUGRW]/;
      my $xcost = $cost =~ /\|X/;
      my $c = $cost;
      while ($c =~ s/\|([0-9]+)(?=\||$)//)
	{
	  $cmcx += $1;
	  $lml{'Req Colorless'} += $1;
	}
      while ($c =~ s/\|2[BUGRW]//)
	{
	  $cmcx += 2;
	  $lml{'Req Colorless'} += 2;
	}
      while ($c =~ s/\|C//)
	{
	  $cmcx += 1;
	  $lml{'Req Colorless'} += 1;
	}
      if ($c =~ s/\|X+//g && $lml{'Req Colorless'} == 0)
	{
	  $lml{'Req Colorless'} = 40;
	}

      foreach my $mcp (keys %mana_req)
	{
	  my $r = "\\\|P?$mcp([BUGRW])?";
	  while ($c =~ s/$r//)
	    {
	      $cmcc += 1;
	      $lml{$mana_req{$mcp}} += 1;
	    }
	}
      $c eq '' or die qq[Couldn't fully parse manacost for card "$lastname" into reqs: raw "$cost", partial="$c"];
      $cmcx + $cmcc eq $card->{converted_manacost} or $splitcard or die qq'Mismatch between raw and computed converted manacost for card "$lastname": raw "$card->{converted_manacost}", computed colorless=$cmcx, computed colored=$cmcc';

      $lct{'Required Colorless Mana (total)'} = $xcost ? 255 : $cmcx;
      $lct{'Required COLOR Mana (total)'} = $cmcc;

      $hybrid && $lct{'cc[2]'} == 0 and $lct{'cc[2]'} = 1;
    }

    { # color
      exists $card->{color} or die qq'No color for card "$lastname"';
      exists $card->{ability} or die qq'No ability for card "$lastname"';
      my $color = $card->{color};
      if ($card->{ability} =~ /^Devoid #_\(This card has no color\.\)/)
	{
	  $color = 'C';
	}
      elsif ($color =~ /A/ && $color =~ /[XUBGRWL]/)
	{
	  $color =~ tr/A//d;	# Colored artifacts and artifact lands
	}
      elsif ($color eq 'GL')
	{
	  $color = 'G';	# Dryad Arbor
	}

      #fixup split cards
      if ($color =~ s@ // @@g)
	{
	  $color =~ s@^([BUGRW])\1$@$1@g;
	}

      my $cpcolor;
      if ($color =~ /^[BUGRW][BUGRW]+$/ && $color !~ /^([BUGRW])\1+$/)
	{
	  $cpcolor = 4;	# multi
	}
      elsif ($emblem)
	{
	  $cpcolor = 3;	# artifact
	}
      elsif (exists $colors{$color})
	{
	  $cpcolor = $colors{$color};
	}
      else
	{
	  die qq'Unexpected color for card "$lastname": "$color"';
	}
      $lml{'Color'} = $cpcolor;

      $lct{'Color Unused'} = 0;
      if ($color =~ /^[LA]$/ && $lml{'Mana Source Colors'} > 0)
	{
	  $lct{'Color ArtMana'}		= $lml{'Mana Source Colors'} & 0x40 ? 1 : 0;
	  $lct{'Color White'}		= $lml{'Mana Source Colors'} & 0x20 ? 1 : 0;
	  $lct{'Color Red'}		= $lml{'Mana Source Colors'} & 0x10 ? 1 : 0;
	  $lct{'Color Green'}		= $lml{'Mana Source Colors'} & 0x08 ? 1 : 0;
	  $lct{'Color Blue'}		= $lml{'Mana Source Colors'} & 0x04 ? 1 : 0;
	  $lct{'Color Black'}		= $lml{'Mana Source Colors'} & 0x02 ? 1 : 0;
	  $lct{'Color Colorless'}	= $lml{'Mana Source Colors'} & 0x01 ? 1 : 0;
	}
      else
	{
	  $lct{'Color ArtMana'}		= 0;
	  $lct{'Color White'}		= $color =~ /W/ ? 1 : 0;
	  $lct{'Color Red'}		= $color =~ /R/ ? 1 : 0;
	  $lct{'Color Green'}		= $color =~ /G/ ? 1 : 0;
	  $lct{'Color Blue'}		= $color =~ /U/ ? 1 : 0;
	  $lct{'Color Black'}		= $color =~ /B/ ? 1 : 0;
	  $lct{'Color Colorless'}	= 0;
	}
    }

    { # power, toughness
      $lml{Power} = $lml{Toughness} = $lct{Power} = $lct{Toughness} = 0;
      if (exists $card->{power} && !(ref $card->{power}))
	{
	  exists $card->{toughness} && !(ref $card->{toughness}) or die qq'card "$lastname" has power "$card->{power}" but no toughness';
	  process_pt($card, \%lml, \%lct, 'power');
	  process_pt($card, \%lml, \%lct, 'toughness');
	}
      elsif (exists $card->{toughness} && !(ref $card->{toughness}))
	{
	  die qq'card "$lastname" has toughness "$card->{toughness}" but no power';
	}
    }

    # Nothing for damage/effect/legacy text or effect/legacy title text.
    $lml{'Damage Text'} = $lml{'Effect Title Text'} = $lml{'Effect Text'} = $lml{'Legacy Title Text'} = $lml{'Legacy Text'} = '';

    # These are difficult to assess automatically, and if they matter at all anywhere, it's only for AI deckbuilding
    $lml{'Mod Direct Fire'} = 0;
    $lml{'Mod Land Destructor'} = 0;
    $lml{'Mod Draw Cards'} = 0;
    $lml{'Mod Discard'} = 0;
    $lml{'Mod Gain Life'} = 0;
    $lml{'Mod Damage Prevention'} = 0;
    $lml{'Mod Recycler'} = 0;
    $lml{'Mod Counter Spell'} = 0;
    $lml{'Mod Control'} = 0;
    $lml{'Mod Combat Manipulator'} = 0;
    $lml{'Mod Enchantments'} = 0;
    $lml{'Mod Artifacts'} = 0;
    $lml{'Mod Creature'} = 0;
    $lml{'Mod Creature Generator'} = 0;
    $lml{'Mod Damage Opponent'} = 0;
    $lml{'Mod Library Manipulation'} = 0;
    $lml{'Mod Dummy 1'} = 0;
    $lml{'Mod Dummy 2'} = 0;
    $lml{'Mod Dummy 3'} = 0;
    $lml{'Mod Dummy 4'} = 0;
    $lml{'AI Inc Power'} = 0;
    $lml{'AI Inc Toughness'} = 0;
    $lml{'AI Power'} = $lml{Power};
    $lml{'AI Toughness'} = $lml{Toughness};
    $lml{'AI Base Value'} = 0;
    $lml{'Dependent Artifact'} = 0;
    $lml{'Dependent Creature'} = 0;
    $lml{'Dependent Wall'} = 0;
    $lml{'VS Black'} = 0;
    $lml{'VS Blue'} = 0;
    $lml{'VS Green'} = 0;
    $lml{'VS Red'} = 0;
    $lml{'VS White'} = 0;
    $lml{'For Black'} = 0;
    $lml{'For Blue'} = 0;
    $lml{'For Green'} = 0;
    $lml{'For Red'} = 0;
    $lml{'For White'} = 0;
    $lml{'Count as Black'} = 0;
    $lml{'Count as Blue'} = 0;
    $lml{'Count as Green'} = 0;
    $lml{'Count as Red'} = 0;
    $lml{'Count as White'} = 0;
    $lml{'VS Black Land'} = 0;
    $lml{'VS Blue Land'} = 0;
    $lml{'VS Green Land'} = 0;
    $lml{'VS Red Land'} = 0;
    $lml{'VS White Land'} = 0;
    $lml{'For Black Land'} = 0;
    $lml{'For Blue Land'} = 0;
    $lml{'For Green Land'} = 0;
    $lml{'For Red Land'} = 0;
    $lml{'For White Land'} = 0;
    $lml{'Count as Black Land'} = 0;
    $lml{'Count as Blue Land'} = 0;
    $lml{'Count as Green Land'} = 0;
    $lml{'Count as Red Land'} = 0;
    $lml{'Count as White Land'} = 0;
    $lml{'AI Flying'} = any_ability('flying');
    $lml{'AI Banding'} = any_ability('banding');
    $lml{'AI Trample'} = any_ability('trample');
    $lml{'AI First Strike'} = any_ability('first strike') || any_ability('double strike');
    $lml{'AI Wall'} = native_ability('defender');
    $lml{'AI Landhome'} = ($rules =~ /^~ can't attack unless defending player controls an? (Plains|Island|Swamp|Mountain|Forest)\.$/m
			   && $rules =~ /^When you control no (Plains|Islands|Swamps|Mountains|Forests), sacrifice ~\.$/m) ? '1' : '0';
    #$lml{'Inflatable'} = 0;	# set below

    if ($token)
      {
	foreach my $k (@sets)
	  {
	    $lml{$k} = '-';
	  }
      }
    else
      {
	foreach my $k (@sets)
	  {
	    exists $lml{$k} or $lml{$k} = '-';
	  }
      }

    if (!$token && !$emblem)
      {
	if (exists $card->{legality_Block} && $card->{legality_Block} eq 'v')
	  {
	    $lml{'(B) Block'} = 'C';
	  }
	if (exists $card->{legality_Vintage} && $card->{legality_Vintage} =~ /^[vr]$/)
	  {
	    $lml{'(T1) Vintage'} = 'C';
	  }
	if (exists $card->{legality_Modern} && $card->{legality_Modern} eq 'v')
	  {
	    $lml{'(T1.M) Modern'} = 'C';
	  }
	if (exists $card->{legality_Legacy} && $card->{legality_Legacy} eq 'v')
	  {
	    $lml{'(T1.X) Extended'} = 'C';
	  }
	if (exists $card->{legality_Standard} && $card->{legality_Standard} eq 'v')
	  {
	    $lml{'(T2) Standard'} = 'C';
	  }
	if (exists $card->{legality_Highlander} && $card->{legality_Highlander} eq 'v')
	  {
	    $lml{'(HI) Highlander'} = 'C';
	  }
	if (exists $card->{legality_Commander})
	  {
	    if ($card->{legality_Commander} eq 'g')
	      {
		$lml{'(EDH) Commander'} = 'C';
	      }
	    if ($card->{legality_Commander} eq 'v')
	      {
		$lml{'(EDH) Commander'} = 'C';
		if (exists $lct{Family} && $lct{Family} =~ /^1[45]$/	# Legendary
		    && exists $lct{'Type: Creature'} && $lct{'Type: Creature'} == 1)
		  {
		    $lml{'(HI / EDH) Legal as Commander'} = 'C';
		  }
	      }
	  }
      }

    $lml{'DB Card Type 2'} //= 0;	# May have been set to 10 earlier.

    my $num_db_abilities = 0;
    $lml{"DBA$_"} = 0 foreach (1..8);

    foreach my $dba (keys %native_dba)
      {
	native_ability($dba) and $lml{'DBA' . ++$num_db_abilities} = $native_dba{$dba};
      }
    if ($rules =~ /: regenerate ~/i)
      {
	$lml{'DBA' . ++$num_db_abilities} = 17;
      }

    my $everything = native_ability('Protection from everything');
    if ($everything || native_ability('Protection from all colors'))
      {
	$lml{'DBA'. ++$num_db_abilities} = $_ foreach (12, 13, 14);	# protection from black, red, white
      }

    foreach my $dba (keys %gives_dba)
      {
	gives_ability($dba) and $lml{'DBA' . ++$num_db_abilities} = $gives_dba{$dba};
      }
    if ($rules =~ /: regenerate [^~]/i)
      {
	$lml{'DBA' . ++$num_db_abilities} = 38;
      }

    $everything = gives_ability('Protection from everything');
    if ($everything || gives_ability('Protection from all colors'))
      {
	$everything and $lml{'DBA' . ++$num_db_abilities} = 30;	# protection from artifacts;
	$lml{'DBA'. ++$num_db_abilities} = $_ foreach (31, 32, 33, 34, 35);	# protection from black, blue, green, red, white
      }

    $num_db_abilities > 8 and $num_db_abilities = 8;
    $lml{'Num of DB Abilities'} = $num_db_abilities;

    # Don't bother to set Sleighted Color, Hack Color, or Hack Mode; it's doesn't get put in the right place for manalink to see it, and Shandalar computes it itself
    $lml{'Sleighted Color'} = 0;
    $lml{'Hack Color'} = 0;
    $lml{'Hack Mode'} = 0;
    # Don't bother to set Enchant Type OWN or Enchant Type OPP; they have such a tiny effect on the AI that it's not worth verifying whether the numbers that've been used are correct
    $lml{'Enchant Type OWN'} = 0;
    $lml{'Enchant Type OPP'} = 0;

    $lml{'CODED CARD'} = 0;
    $lml{'Expansion'} = ($token || $emblem) ? '1000h' : '8h';	# For lack of better ideas.
    $lml{'Expansions Rarity'} = '0h';
    $lml{'Num Pics'} = 1;
    #$lml{'ID'} = ;

    $lct{'Sort order'} = 10000;
    $lct{'Extra Flags (Unused)'} = 0;
    $lct{'Modifies Produced Mana'} = 0;
    my $morph = ($rules =~ /^(Megamorph|Morph)[ -]/m);
    my $dash = ($rules =~ /^(Dash)[ -]/m);
    $lct{'Modifies Casting Cost'} = ($hybrid || $morph || $dash) ? 1 : 0;	# very incomplete - should include anything that changes its own casting cost
    $lct{'Unused'} = 0;
    $lct{'Code Address'} = '0x401000';

    $lct{'Ability: Unknown'} = 0;
    $lct{'Ability: Double Strike'} = native_ability('double strike');
    $lct{'Ability: Infect'} = native_ability('infect');
    $lct{'Ability: Temp Color Changed (not used in this struct)'} = 0;
    $lct{'Ability: Temp Ability Added (not used in this struct)'} = 0;
    $lct{'Ability: Temp Power Changed (not used in this struct)'} = 0;
    $lct{'Ability: Temp Toughness Changed (not used in this struct)'} = 0;
    $lct{'Ability: Temp Transformed (not used in this struct)'} = 0;
    $lct{'Ability: Defender'} = native_ability('defender');
    $lct{'Ability: Prot from Lands'} = 0;
    $lct{'Ability: Banding when Attacking'} = 0;
    $lct{'Ability: Prot from Sorceries'} = 0;
    $lct{'Ability: Changeling'} = native_ability('changeling');
    $lct{'Ability: Prot from Instants'} = 0;
    $lct{'Ability: Shroud'} = native_ability('shroud');
    $lct{'Ability: Prot from Artifacts'} = native_ability(rgx_protection('artifacts', 1, 0));
    $lct{'Ability: Prot from White'} = native_ability(rgx_protection('white', 1, 1));
    $lct{'Ability: Prot from Red'} = native_ability(rgx_protection('red', 1, 1));
    $lct{'Ability: Prot from Green'} = native_ability(rgx_protection('green', 1, 1));
    $lct{'Ability: Prot from Blue'} = native_ability(rgx_protection('blue', 1, 1));
    $lct{'Ability: Prot from Black'} = native_ability(rgx_protection('black', 1, 1));
    $lct{'Ability: Reach'} = native_ability('reach');
    $lct{'Ability: Regeneration'} = ($rules =~ /: regenerate ~/i) ? 1 : 0;
    $lct{'Ability: First Strike'} = native_ability('first strike');
    $lct{'Ability: Trample'} = native_ability('trample');
    $lct{'Ability: Banding'} = native_ability('banding');
    $lct{'Ability: Flying'} = native_ability('flying');
    $lct{'Ability: Plainswalk'} = native_ability('plainswalk');
    $lct{'Ability: Mountainswalk'} = native_ability('mountainwalk');
    $lct{'Ability: Forestwalk'} = native_ability('forestwalk');
    $lct{'Ability: Islandwalk'} = native_ability('islandwalk');
    $lct{'Ability: Swampwalk'} = native_ability('swampwalk');

    $lct{'Flags: Ability Cost'} = 0;	# Very broken in Manalink.  Apparently an attempt to rewrite Gloom.
    $lct{'Flags: Play Cost'} = 0;	# very incomplete - should include anything that changes the casting cost of another card
    $lct{'Flags: Unknown (unused)'} = 0;
    $lct{'Flags: Special Code When Controlled'} = 0;	# needs EVENT_CARDCONTROLLED dispatched - usually a sign of a dangerous hack, i.e. setting globals when a card enters the bf and unsetting them when it leaves
    $lct{'Flags: Fellwar Stone'} = 0;	# needs EVENT_VARIABLE_MANA_SRC sent during count_mana()
    $lct{'Flags: Declare Attack'} = 0;	# needs EVENT_ATTACK_LEGALITY sent.  A source of subtle problems, since there's other common conditions that can cause the event to be sent.
    $lct{'Flags: Before Combat'} = 0;	# needs EVENT_BEFORE_COMBAT sent.
    $lct{'Flags: Force Attack'} = 0;	# needs EVENT_MUST_ATTACK sent.
    $lct{'Flags: Paid Block'} = 0;	# needs TRIGGER_PAY_TO_BLOCK sent.
    $lct{'Flags: Paid Attack'} = ($lml{'Type Text'} =~ /Planeswalker/) ? 1 : 0;	# needs TRIGGER_PAY_TO_ATTACK sent.
    $lct{'Flags: Lich'} = 0;		# needs TRIGGER_GAIN_LIFE sent.  In Shandalar, needs either XTRIGGER_REPLACE_GAIN_LIFE or TRIGGER_GAIN_LIFE sent.
    $lct{'Flags: Select Block'} = 0;	# needs TRIGGER_BLOCKER_CHOSEN sent.
    $lct{'Flags: Select Attack'} = 0;	# needs TRIGGER_ATTACKER_CHOSEN sent.
    $lct{'Flags: Martyr'} = 0;		# needs TRIGGER_END_DAMAGE_PREV sent.
    $lct{'Flags: Act Use X'} = 0;	# shows the value chosen for X when the other player (typically the AI) activates an ability.
    $lct{'Flags: Paid Mana Source'} = 0;# Doesn't actually do anything except prevent a card from ever being autotapped for mana.
    $lct{'Flags: Became Creature'} = 0;	# Set on animated lands and artifacts (by Living Lands, Kormus Bell, Titania's Song).  Doesn't apparently *do* anything.
    $lct{'Flags: Activated Counterspell'} = 0;	# Set, apparently, on activated counterspells.  The differences between this and Flags: Activate Interrupt are unclear.
    $lct{'Flags: Interrupt'} = 0;	# Set on spells that are cast at interrupt speed, i.e. that can target other spells in Manalink.
    $lct{'Flags: Mana Source'} = ($lml{'Mana Source Colors'} == 0 ? 0 : 1);	# Displays manastripes on smallcards, considered for autotapping, etc.
    $lct{'Flags: Unknown (Shandalar)'} = 0;	# Irritating that the column names are all the same, since most of these are knokwn.  The leftmost is "not selectable in cardpicker", the next is "considered the next higher rarity for gold/amulet costs", the next is tentatively "double gold cost", the next is "dungeon treasure", the next is tentatively "more expensive".
    $lct{'Flags: Unknown (Some AI DrawCard stuff)'} = 0;	# Not selectable when the AI is cheating which card to draw.  (I'm fairly sure the cheating-which-card-to-draw can't actually be run.)
    $lct{'Flags: Counters'} = 0;	# Unused.
    $lct{'Flags: Inflatable Toughness'} = $rules =~ m!:.*~ gets [+-][0-9X]+/\+[^0]|:.*\b([pP]ut|[dD]ouble)\b.*/\+[1-9X][0-9X]* counters?\b on ~\b! ? 1 : 0;	# Gets sent EVENT_TOU_BOOST, for seeing how much a card can pump itself.  Activation is the most obvious method, but should also include abilities like Dethrone ("Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.") where it's easily predictable whether a card can increase toughness.
    $lct{'Flags: Inflatable Power'} = $rules =~ m!:.*~ gets \+[1-9X]|:.*\b([pP]ut|[dD]ouble)\b.*\+[1-9X][0-9X]*/[+-][0-9X]+ counters?\b on ~\b! ? 1 : 0;	# Gets sent EVENT_POW_BOOST, for seeing how much a card can pump itself.  Activation is the most obvious method, but should also include abilities like Dethrone ("Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.") where it's easily predictable whether a card can increase power.
    $lml{'Inflatable'} = ($lct{'Flags: Inflatable Toughness'} || $lct{'Flags: Inflatable Power'}) ? 1 : 0;
    $lct{'Flags: Protect'} = 0;		# Can activate (or be cast?) during damage prevention.
    $lct{'Flags: Activate Interrupt'} = 0;	# Can activate at interrupt speed.
    my $activateable = $rules =~ /:/ ? 1 : 0;
    my $inflateable = $lml{'Inflatable'} ? 1 : 0;
    $lct{'Flags: Activate'} = $equipment || $activateable;
    $lct{'Rarity (copied)'} = 0;

    # These activate and play flags always need to be manually checked.
    $lct{'Activate after Combat'} = $lct{'Activate before Combat'} = $equipment || ($activateable && !$inflateable);
    $lct{'Activate before Blockers'} = $activateable && !$inflateable;
    $lct{'Activate after Blockers'} = $activateable;
    $lct{'Play after Combat'} = ($equipment || $lct{'Type: Creature'} || $lct{'Type: Instant'}) ? 1 : 0;
    $lct{'Play before Combat'} = (!$lct{'Type: Creature'} || $lct{'Type: Instant'} || ($rules =~ /\b(get|gain|have)\b/)) ? 1 : 0;	# i.e., all cards except creatures; plus creatures with flash, and creatures that have "get", "gain", or "have" in their rules text.
    if (!$lct{'Play before Combat'} && $lct{'Type: Creature'} && native_ability('haste'))
      {
	$lct{'Play before Combat'} = 1;
	$lct{'Play after Combat'} = 0;
      }
    $lct{'Play after Blockers'} = $lct{'Play before Blockers'} = ($lct{'Type: Interrupt'} || $lct{'Type: Instant'}) ? 1 : 0;

    $lct{'Expansion (copied)'} = 0;
    $lct{'Creature Rating Mod'} = 0;	# Needs manual adjustment, though it's not at *all* clear that this column does what it's been assumed to do.

    foreach (@ml_columns)
      {
	!exists $lml{$_} && $_ ne 'ID' and die qq[Card "$lastname" doesn't have $manalink_filename field "$_" set];
      }
    foreach (@ct_columns)
      {
	!exists $lct{$_} and die qq[Card "$lastname" doesn't have $ctall_filename field "$_" set];
      }

    $add_ml{$lastname} = \%lml;
    $add_ct{$lastname} = \%lct;

    $id2name{$card->{id}} = $lastname;
    $name2id{$lastname} = $card->{id};
    if (exists $card->{back_id} && !(ref $card->{back_id}))
      {
	(exists $card->{id}) && !(ref $card->{id}) or die qq'card "$lastname" has back_id="$card->{back_id}" but no id';
	(exists $card->{number}) && !(ref $card->{number}) or die qq'card "$lastname" has back_id="$card->{back_id}" but no number';
	if ($card->{back_id} == $card->{id})
	  {
	    if ($card->{id} == 410049)	# bad data - Westvale Abbey points to itself instead of Ormendahl, Profane Prince
	      {
		$card->{back_id} = 410050;
	      }
	    else
	      {
		die qq'card "$lastname" has back_id="$card->{back_id}" equal to its id="$card->{id}"';
	      }
	  }
	$card->{number} =~ /^[0-9]+[ab]$/ or die qq'card "$lastname" has back_id="$card->{back_id}" but unparseable number "$card->{number}"';
	if ($card->{number} =~ /^[0-9]+a$/)
	  {
	    $dfc_fronts{$lastname} = $card->{back_id};
	    #warn qq[Setting dfc_fronts{$lastname} to "$card->{back_id}"=>"$id2name{$card->{back_id}}"];
	  }
	else	# ($card->{number} =~ /^[0-9]+b$/)
	  {
	    $dfc_backs{$lastname} = $card->{back_id};
	    #warn qq[Setting dfc_backs{$lastname} to "$card->{back_id}"=>"$id2name{$card->{back_id}}"];
	  }
      }
  }

# EMN cards don't have back_id set, irritating.
if (exists $name2id{"Lone Rider"})
  {
    my @emn_dfc =
      (
       ["Extricator of Sin"		=> "Extricator of Flesh"],
       ["Lone Rider"			=> "It That Rides as One"],
       ["Curious Homunculus"		=> "Voracious Reader"],
       ["Docent of Perfection"		=> "Final Iteration"],
       ["Grizzled Angler"		=> "Grisly Anglerfish"],
       ["Voldaren Pariah"		=> "Abolisher of Bloodlines"],
       ["Conduit of Storms"		=> "Conduit of Emrakul"],
       ["Smoldering Werewolf"		=> "Erupting Dreadwolf"],
       ["Vildin-Pack Outcast"		=> "Dronepack Kindred"],
       ["Kessig Prowler"		=> "Sinuous Predator"],
       ["Shrill Howler"			=> "Howling Chorus"],
       ["Tangleclaw Werewolf"		=> "Fibrous Entangler"],
       ["Ulvenwald Captive"		=> "Ulvenwald Abomination"],
       ["Ulrich of the Krallenhorde"	=> "Ulrich, Uncontested Alpha"],
       ["Cryptolith Fragment"		=> "Aurora of Emrakul"],
       # Melds
       ["Bruna, the Fading Light"	=> "Brisela, Voice of Nightmares"],
       ["Graf Rats"			=> "Chittering Host"],
       ["Hanweir Battlements"		=> "Hanweir, the Writhing Township"],
      );
    foreach my $dfc (@emn_dfc)
      {
	exists $name2id{$dfc->[0]} or die qq{EMN dfc "$dfc->[0]" doesn't exist};
	exists $name2id{$dfc->[1]} or die qq{EMN dfc "$dfc->[1]" doesn't exist};
	$dfc_backs{$dfc->[1]} = $name2id{$dfc->[0]};
	#warn qq[Manually setting dfc_fronts{$dfc->[1]} to "$name2id{$dfc->[0]}"=>"$id2name{$name2id{$dfc->[0]}}"];
	$dfc_fronts{$dfc->[0]} = $name2id{$dfc->[1]};
	#warn qq[Manually setting dfc_backs{$dfc->[0]} to "$name2id{$dfc->[1]}"=>"$id2name{$name2id{$dfc->[1]}}"];
      }
  }

say STDERR "Processed $n copies of ", (scalar keys %add_ml), " different cards.";

# add text to double-faced cards
foreach my $front (sort keys %dfc_fronts)
  {
    my $back = $id2name{$dfc_fronts{$front}};
    defined $back or die '!exists $id2name{$dfc_fronts{'.$front.'} = "'.$dfc_fronts{$front}.'"}';
    #say qq'"$front"\t=>"', $back, '"';
    exists $add_ml{$front} or die '!exists $add_ml{', $front, '}';
    exists $add_ml{$back} or die '!exists $add_ml{', $back, '}';
    exists $add_ct{$back} or die '!exists $add_ct{', $back, '}';

    my $rules = $add_ml{$front}->{'Rules Text'};
    if ($rules =~ /\S/)
      {
	$rules .= '\n';
      }
    $rules .= '(' . $back;

    if ($add_ct{$back}->{'Type: Creature'})
      {
	$rules .= ' - ';
	foreach my $stat (qw/Power Toughness/)
	  {
	    my $val = $add_ml{$back}->{$stat};
	    if ($val < 100)
	      {}
	    elsif ($val == 100)
	      { $val = '*'; }
	    elsif ($val < 200)
	      { $val = '*+' . $val; }
	    else
	      { $val = $val . '-*'; }

	    ($stat eq 'Power') and $val .= '/';
	    $rules .= $val;
	  }
      }
    elsif (exists $add_ml{$back}->{hack_loyalty})
      {
	$rules .= ' - ' . $add_ml{$back}->{hack_loyalty};
      }

    if ($add_ml{$back}->{'Rules Text'} !~ /\S/)
      {
	$rules .= ')';
      }
    else
      {
	if ($add_ml{$back}->{'Rules Text'} =~ /^\+/)	# planeswalkers
	  {
	    $rules .= ')\n';
	  }
	else
	  {
	    $rules .= ') ';
	  }
	$rules .= $add_ml{$back}->{'Rules Text'};
      }

    $add_ml{$front}->{'Rules Text'} = $rules;
    #say '{', $rules, '}';
  }

$delayed_fatals && die "Exiting due to delayed errors";

say STDERR qq'\nMerging "$manalink_filename"';
my $ml_header_line = join(';', @ml_columns);
my $linenum = 0;
my $added_ids = 0;
my $csv = Text::CSV->new({ binary=>1, eol=>$cr, sep_char=>';', quote_space=>0, allow_whitespace=>1, quote_binary=>0, auto_diag=>1 }) or die "Couldn't create Text::CSV: " . Text::CSV->error_diag();
open(my $manalink_in, '<', $manalink_filename) or die "Couldn't open $manalink_filename for reading: $!";
open(my $manalink_out, '>', 'Manalink_out.csv') or die "Couldn't open Manalink_out.csv for writing: $!";
binmode($manalink_out);
my $correct_cols;
my $begin_updates = 0;
MANALINK_LOOP: while (my $row = $csv->getline($manalink_in))
  {
    ++$linenum;
    if ($linenum == 1)
      {
	$csv->combine(@{$row}) or die "Couldn't combine csv fields for $manalink_filename header line: " . Text::CSV->error_diag();
	$ml_header_line . $cr eq $csv->string() or die qq[Mismatch between computed $manalink_filename header line and actual read\nComputed:\n"$ml_header_line$cr"\nActual Read:\n"], $csv->string(), '"]';
	print $manalink_out $csv->string();
	$correct_cols = scalar @{$row};
	next MANALINK_LOOP;
      }

    my $id = $row->[0];
    if ($id eq '#')	# That last screwy line
      {
	$csv->combine(@{$row}) or die "Couldn't combine csv fields for $manalink_filename trailer line: " . Text::CSV->error_diag();
	print $manalink_out $csv->string();
	next MANALINK_LOOP;
      }
    $lastname = $row->[1];
    #Deal with the totally useless added markup in Conspiracy names
    my $cooked_lastname = $lastname;
    $cooked_lastname =~ s/\x{2013}/-/g;
    $cooked_lastname =~ s/\226/-/g;

    if (exists $add_ml{$cooked_lastname})
      {
	my $add = $add_ml{$cooked_lastname};

	if (!$updates_only)
	  {
	    $opt_quiet or say STDERR qq[${color_darkblue}Card "$lastname" is a reprint.${color_normal}];
	  }

	my @updated;
	if (($opt_update || !$updates_only)
	    && $row->[$ml_colnums{'Rules Text'}] ne $add->{'Rules Text'}
	    && $lastname !~ /^(Snow-Covered )?(Swamp|Island|Forest|Mountain|Plains)$/)	# Omit for basic lands
	  {
	    #my $straightened_quotes = $row->[$ml_colnums{'Rules Text'}];
	    #$straightened_quotes =~ tr/\223\224/""/;
	    my $f = fix_encoding($add->{'Rules Text'});
	    if (fix_encoding($row->[$ml_colnums{'Rules Text'}]) ne $f)
	      {
		push @updated, "rules text";
		$row->[$ml_colnums{'Rules Text'}] = $f;
	      }
	  }

	my $update_type_columns = !$updates_only;
	if (($update_type_columns || $opt_update)
	    && $row->[$ml_colnums{'Type Text'}] ne fix_encoding($add->{'Type Text'}))
	  {
	    my $f = fix_encoding($add->{'Type Text'});
	    if (fix_encoding($row->[$ml_colnums{'Type Text'}]) ne $f)
	      {
		if ($lastname eq 'Ral Zarek')
		  {
		    say STDERR '[', fix_encoding($row->[$ml_colnums{'Type Text'}]), '] [', $f, ']';
		  }
		push @updated, "type text";
		$row->[$ml_colnums{'Type Text'}] = $f;
	      }

	    $update_type_columns = 1;
	  }

	if ($update_type_columns)	# Don't update these fields if doing a global update and type text didn't change, to minimize the (many!) false positives
	  {
	    foreach my $t ((map {"New Types $_"} (1..7)), 'Subtype', 'Subtype 2')
	      {
		if (exists $add->{$t}
		    and $row->[$ml_colnums{$t}] ne $add->{$t})
		  {
		    push @updated, "$t ('$row->[$ml_colnums{$t}]' => '$add->{$t}')";
		    $row->[$ml_colnums{$t}] = $add->{$t};
		  }
	      }
	  }

	if (($opt_update || !$updates_only)
	    && $row->[$ml_colnums{'Flavor Text'}] =~ /^\s*$/ && $add->{'Flavor Text'} !~ /^\s*$/)
	  {
	    push @updated, "Added flavor text";
	    $row->[$ml_colnums{'Flavor Text'}] = fix_encoding($add->{'Flavor Text'});
	  }

	if ($updates_only)
	  {
	    if ($lastname eq $first_ml_to_update_for_abils)
	      {
		$begin_updates = 1;
	      }
	    if ($lastname eq 'Rules Engine')
	      {
		$begin_updates = 0;
	      }
	    if (($begin_updates || $row->[$ml_colnums{'CODED CARD'}] eq '0')
		&& !($opt_equipment && !(exists $all_equipment{$lastname})))
	      {
		foreach my $col (@mk_cols_to_update)
		  {
		    if ($row->[$ml_colnums{$col}] ne fix_encoding($add->{$col}))
		      {
			my $f = fix_encoding($add->{$col});
			if (fix_encoding($row->[$ml_colnums{$col}]) ne $f)
			  {
			    push @updated, $col;
			    $row->[$ml_colnums{$col}] = $f;
			  }
		      }
		  }
	      }
	  }

	if (scalar @updated > 0)
	  {
	    if ($updates_only)
	      {
		print STDERR qq["$lastname":\t\t\t];
		my $l = length($lastname) + 3;
		while ($l < 48)
		  {
		    print STDERR "\t";
		    $l += 8;
		  }
		say STDERR ($updated[0] =~ /^Added/ ? '' : 'Updated ' ), join(', ', @updated), '.';
	      }
	    else
	      {
		if ($opt_quiet && scalar @updated > 0)
		  {
		    say STDERR "$lastname:";
		  }
		foreach (@updated)
		  {
		    say STDERR (/^Added/ ? '  ' : '  Updated '), $_, '.';
		  }
	      }
	  }

	if (!$updates_only)
	  {
	    my $updated_sets = 0;
	    my $updated_sets_str = '';
	    foreach (@sets)
	      {
		if ($add->{$_} ne '-')
		  {
		    if ($row->[$ml_colnums{$_}] eq '-')
		      {
			$updated_sets = 1;
			$row->[$ml_colnums{$_}] = $add->{$_};
			$updated_sets_str .= qq[ +"$_"];
		      }
		    elsif ($lastname !~ /^([Ss]wamp|[Ii]sland|[Ff]orest|[Mm]ountain|[Pp]lains)$/	# Omit for basic lands
			   && !/^\(/)	# Omit format-legality sets
		      {
			$updated_sets_str .= qq[ ="$_"];
		      }
		  }
		elsif ($row->[$ml_colnums{$_}] ne '-')
		  {
		    $updated_sets = 1;
		    $row->[$ml_colnums{$_}] = $add->{$_};
		    $updated_sets_str .= qq[ <"$_"];
		  }
	      }
	    if ($updated_sets and !$opt_quiet)
	      {
		print STDERR "${color_darkblue}  Updating sets:", $updated_sets_str, "${color_normal}\n";
	      }
	  }

	$csv->combine(@{$row}) or die qq[Couldn't combine csv fields for $manalink_filename line for existing card "$lastname": ] . Text::CSV->error_diag();
	print $manalink_out $csv->string();
	$correct_cols == scalar @{$row} or mywarn qq[$manalink_filename row for "$lastname" had ], (scalar @{$row}), " columns, expected $correct_cols";

	delete $add_ml{$cooked_lastname};
	next MANALINK_LOOP;
      }

    if ($lastname eq 'Rules Engine' && !$updates_only)
      {
	my %dfc_skipped;
	my @todo;
	foreach my $k (sort keys %add_ml)
	  {
	    if (exists $dfc_backs{$k})
	      {
		++$dfc_skipped{$k};
		next;
	      }

	    push @todo, $k;

	    if (exists $dfc_fronts{$k})
	      {
		my $back = $id2name{$dfc_fronts{$k}};
		exists $add_ml{$back} or die qq@"$k" to be added.  It's the front of a double-faced card.  Its back "$back" has already been eliminated.@;
		push @todo, $back;
		--$dfc_skipped{$back};
	      }
	  }

	foreach my $k (@todo)
	  {
	    say STDERR "Added $k.";
	    my @r;
	    foreach my $c (@ml_columns)
	      {
		if ($c eq 'ID')
		  {
		    push @r, $id + $added_ids++;
		  }
		else
		  {
		    push @r, $add_ml{$k}->{$c};
		  }
	      }
	    $csv->combine(@r) or die qq[Couldn't combine csv fields for $manalink_filename line for new card "$k": ] . Text::CSV->error_diag();
	    print_fixed_encoding($manalink_out, $csv->string());
	    $correct_cols == scalar @r or die qq[Generated $manalink_filename row for "$lastname" had ], (scalar @r), " columns, expected $correct_cols";
	  }

	foreach my $k (sort keys %dfc_skipped)
	  {
	    $dfc_skipped{$k} == 0 or die '$dfc_skipped{', $k, '} == ', $dfc_skipped{$k}, '; expected 0 or !exists';
	  }

	%add_ml = ();
	# And continue on
      }

    $row->[0] += $added_ids;
    $csv->combine(@{$row}) or die qq[Couldn't combine csv fields for $manalink_filename line for existing card "$lastname": ] . Text::CSV->error_diag();
    print $manalink_out $csv->string();
    $correct_cols == scalar @{$row} or mywarn qq[$manalink_filename row for "$lastname" had ], (scalar @{$row}), " columns, expected $correct_cols";
  }

$csv->eof or die "Couldn't read from $manalink_filename: " . $csv->error_diag();
close $manalink_in;
close $manalink_out;
say STDERR qq"Done.";

$opt_update and exit(0);

say STDERR qq'\nMerging "$ctall_filename"';

my $ct_header_line = join(';', @ct_columns);
$linenum = 0;
my $added_ct_lines = 0;
open(my $ctall_in, '<', $ctall_filename) or die "Couldn't open $ctall_filename for reading: $!";
open(my $ctall_out, '>', 'ct_all_out.csv') or die "Couldn't open ct_all_out.csv for writing: $!";
binmode($ctall_out);
$begin_updates = 0;
CTALL_LOOP: while (my $row = $csv->getline($ctall_in))
  {
    ++$linenum;
    if ($linenum == 1)
      {
	$csv->combine(@{$row}) or die "Couldn't combine csv fields for $ctall_filename header line: " . Text::CSV->error_diag();
	$ct_header_line . $cr eq $csv->string() or die qq[Mismatch between computed $ctall_filename header line and actual read\nComputed:\n"$ct_header_line$cr"\nActual Read:\n"], $csv->string(), '"]';
	print $ctall_out $csv->string();
	$correct_cols = scalar @{$row};
	next CTALL_LOOP;
      }

    my $sort_order = $row->[0];
    if ($sort_order eq '#')	# That last screwy line
      {
	$csv->combine(@{$row}) or die "Couldn't combine csv fields for $ctall_filename trailer line: " . Text::CSV->error_diag();
	print $ctall_out $csv->string();
	next CTALL_LOOP;
      }
    $lastname = $row->[2];
    #Deal with the totally useless added markup in Conspiracy names
    my $cooked_lastname = $lastname;
    $cooked_lastname =~ s/\x{2013}/-/g;
    $cooked_lastname =~ s/\226/-/g;

    if ($updates_only)
      {
	if ($lastname eq $first_ct_to_update_for_abils)
	  {
	    $begin_updates = 1;
	  }
	elsif ($sort_order > 10000)
	  {
	    $begin_updates = 0;
	  }

	if ($begin_updates && exists $add_ct{$cooked_lastname})
	  {
	    my $add = $add_ct{$cooked_lastname};
	    my @updated = ();

	    unless ($opt_equipment && !(exists $all_equipment{$lastname}))
	      {
		foreach my $col (@ct_cols_to_update)
		  {
		    if ($row->[$ct_colnums{$col}] ne fix_encoding($add->{$col}))
		      {
			my $f = fix_encoding($add->{$col});
			if (fix_encoding($row->[$ct_colnums{$col}]) ne $f)
			  {
			    push @updated, $col;
			    $row->[$ct_colnums{$col}] = $f;
			  }
		      }
		  }
	      }

	    if (scalar @updated)
	      {
		print STDERR qq["$lastname":\t\t\t];
		my $l = length($lastname) + 3;
		while ($l < 48)
		  {
		    print STDERR "\t";
		    $l += 8;
		  }
		say STDERR 'Updated ', join(', ', @updated), '.';
	      }
	  }
      }

    if (exists $add_ct{$cooked_lastname})
      {
	delete $add_ct{$cooked_lastname};	# Don't touch this row
      }

    if ($sort_order > 10000 && !$added_ct_lines && !$updates_only)
      {
	foreach my $k (sort keys %add_ct)
	  {
	    say STDERR "Added $k.";
	    my @r;
	    foreach my $c (@ct_columns)
	      {
		push @r, $add_ct{$k}->{$c};
	      }
	    $csv->combine(@r) or die qq[Couldn't combine csv fields for $ctall_filename line for new card "$k": ] . Text::CSV->error_diag();
	    print_fixed_encoding($ctall_out, $csv->string());
	    $correct_cols == scalar @r or die qq[Generated $ctall_filename row for "$lastname" had ], (scalar @r), " columns, expected $correct_cols";
	  }
	%add_ct = ();
	$added_ct_lines = 1;
	# And continue on
      }

    $csv->combine(@{$row}) or die qq[Couldn't combine csv fields for $ctall_filename line for existing card "$lastname": ] . Text::CSV->error_diag();
    print $ctall_out $csv->string();
    $correct_cols == scalar @{$row} or mywarn qq[$ctall_filename row for "$lastname" had ], (scalar @{$row}), " columns, expected $correct_cols";
  }

$csv->eof or die "Couldn't read from $ctall_filename: " . $csv->error_diag();
close $ctall_in;
close $ctall_out;
say STDERR qq"Done.";
