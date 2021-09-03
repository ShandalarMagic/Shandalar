#ifndef SUBTYPES_H
#define SUBTYPES_H

// Our new subtype codes, stored in the "new_type" columns of "manalink.csv"
typedef enum
{
SUBTYPE_MIN_CREATURE_SUBTYPE		= 0x0001,
  SUBTYPE_ALLY                        = 0x0001,	//creature types
  SUBTYPE_ANGEL                       = 0x0002,
  SUBTYPE_ANTEATER                    = 0x0003,
  SUBTYPE_ANTELOPE                    = 0x0004,
  SUBTYPE_APE                         = 0x0005,
  SUBTYPE_ARCHER                      = 0x0006,
  SUBTYPE_ARCHON                      = 0x0007,
  SUBTYPE_ARTIFICER                   = 0x0008,
  SUBTYPE_ASSASSIN                    = 0x0009,
  SUBTYPE_ASSEMBLY_WORKER             = 0x000A,
  SUBTYPE_ATOG                        = 0x000B,
  SUBTYPE_AUROCHS                     = 0x000C,
  SUBTYPE_AVATAR                      = 0x000D,
  SUBTYPE_BADGER                      = 0x000E,
  SUBTYPE_BARBARIAN                   = 0x000F,
  SUBTYPE_BASILISK                    = 0x0010,
  SUBTYPE_BAT                         = 0x0011,
  SUBTYPE_BEAR                        = 0x0012,
  SUBTYPE_BEAST                       = 0x0013,
  SUBTYPE_BEEBLE                      = 0x0014,
  SUBTYPE_BERSERKER                   = 0x0015,
  SUBTYPE_BIRD                        = 0x0016,
  SUBTYPE_BLINKMOTH                   = 0x0017,
  SUBTYPE_BOAR                        = 0x0018,
  SUBTYPE_BRINGER                     = 0x0019,
  SUBTYPE_BRUSHWAGG                   = 0x001A,
  SUBTYPE_CAMARID                     = 0x001B,
  SUBTYPE_CAMEL                       = 0x001C,
  SUBTYPE_CARIBOU                     = 0x001D,
  SUBTYPE_CARRIER                     = 0x001E,
  SUBTYPE_CAT                         = 0x001F,
  SUBTYPE_CENTAUR                     = 0x0020,
  SUBTYPE_CEPHALID                    = 0x0021,
  SUBTYPE_CHIMERA                     = 0x0022,
  SUBTYPE_CITIZEN                     = 0x0023,
  SUBTYPE_CLERIC                      = 0x0024,
  SUBTYPE_COCKATRICE                  = 0x0025,
  SUBTYPE_CONSTRUCT                   = 0x0026,
  SUBTYPE_COWARD                      = 0x0027,
  SUBTYPE_CRAB                        = 0x0028,
  SUBTYPE_CROCODILE                   = 0x0029,
  SUBTYPE_CYCLOPS                     = 0x002A,
  SUBTYPE_DAUTHI                      = 0x002B,
  SUBTYPE_DEMON                       = 0x002C,
  SUBTYPE_DESERTER                    = 0x002D,
  SUBTYPE_DEVIL                       = 0x002E,
  SUBTYPE_DJINN                       = 0x002F,
  SUBTYPE_DRAGON                      = 0x0030,
  SUBTYPE_DRAKE                       = 0x0031,
  SUBTYPE_DREADNOUGHT                 = 0x0032,
  SUBTYPE_DRONE                       = 0x0033,
  SUBTYPE_DRUID                       = 0x0034,
  SUBTYPE_DRYAD                       = 0x0035,
  SUBTYPE_DWARF                       = 0x0036,
  SUBTYPE_EFREET                      = 0x0037,
  SUBTYPE_ELDER                       = 0x0038,
  SUBTYPE_ELDRAZI                     = 0x0039,
  SUBTYPE_ELEMENTAL                   = 0x003A,
  SUBTYPE_ELEPHANT                    = 0x003B,
  SUBTYPE_ELF                         = 0x003C,
  SUBTYPE_ELK                         = 0x003D,
  SUBTYPE_EYE                         = 0x003E,
  SUBTYPE_FAERIE                      = 0x003F,
  SUBTYPE_FERRET                      = 0x0040,
  SUBTYPE_FISH                        = 0x0041,
  SUBTYPE_FLAGBEARER                  = 0x0042,
  SUBTYPE_FOX                         = 0x0043,
  SUBTYPE_FROG                        = 0x0044,
  SUBTYPE_FUNGUS                      = 0x0045,
  SUBTYPE_GARGOYLE                    = 0x0046,
  SUBTYPE_GERM                        = 0x0047,
  SUBTYPE_GIANT                       = 0x0048,
  SUBTYPE_GNOME                       = 0x0049,
  SUBTYPE_GOAT                        = 0x004A,
  SUBTYPE_GOBLIN                      = 0x004B,
  SUBTYPE_GOLEM                       = 0x004C,
  SUBTYPE_GORGON                      = 0x004D,
  SUBTYPE_GRAVEBORN                   = 0x004E,
  SUBTYPE_GREMLIN                     = 0x004F,
  SUBTYPE_GRIFFIN                     = 0x0050,
  SUBTYPE_HAG                         = 0x0051,
  SUBTYPE_HARPY                       = 0x0052,
  SUBTYPE_HELLION                     = 0x0053,
  SUBTYPE_HIPPO                       = 0x0054,
  SUBTYPE_HIPPOGRIFF                  = 0x0055,
  SUBTYPE_HOMARID                     = 0x0056,
  SUBTYPE_HOMUNCULUS                  = 0x0057,
  SUBTYPE_HORROR                      = 0x0058,
  SUBTYPE_HORSE                       = 0x0059,
  SUBTYPE_HOUND                       = 0x005A,
  SUBTYPE_HUMAN                       = 0x005B,
  SUBTYPE_HYDRA                       = 0x005C,
  SUBTYPE_HYENA                       = 0x005D,
  SUBTYPE_ILLUSION                    = 0x005E,
  SUBTYPE_IMP                         = 0x005F,
  SUBTYPE_INCARNATION                 = 0x0060,
  SUBTYPE_INSECT                      = 0x0061,
  SUBTYPE_JELLYFISH                   = 0x0062,
  SUBTYPE_JUGGERNAUT                  = 0x0063,
  SUBTYPE_KAVU                        = 0x0064,
  SUBTYPE_KIRIN                       = 0x0065,
  SUBTYPE_KITHKIN                     = 0x0066,
  SUBTYPE_KNIGHT                      = 0x0067,
  SUBTYPE_KOBOLD                      = 0x0068,
  SUBTYPE_KOR                         = 0x0069,
  SUBTYPE_KRAKEN                      = 0x006A,
  SUBTYPE_LAMMASU                     = 0x006B,
  SUBTYPE_LEECH                       = 0x006C,
  SUBTYPE_LEVIATHAN                   = 0x006D,
  SUBTYPE_LHURGOYF                    = 0x006E,
  SUBTYPE_LICID                       = 0x006F,
  SUBTYPE_LIZARD                      = 0x0070,
  SUBTYPE_MANTICORE                   = 0x0071,
  SUBTYPE_MASTICORE                   = 0x0072,
  SUBTYPE_MERCENARY                   = 0x0073,
  SUBTYPE_MERFOLK                     = 0x0074,
  SUBTYPE_METATHRAN                   = 0x0075,
  SUBTYPE_MINION                      = 0x0076,
  SUBTYPE_MINOTAUR                    = 0x0077,
  SUBTYPE_MONGER                      = 0x0078,
  SUBTYPE_MONGOOSE                    = 0x0079,
  SUBTYPE_MONK                        = 0x007A,
  SUBTYPE_MOONFOLK                    = 0x007B,
  SUBTYPE_MUTANT                      = 0x007C,
  SUBTYPE_MYR                         = 0x007D,
  SUBTYPE_MYSTIC                      = 0x007E,
  SUBTYPE_NAUTILUS                    = 0x007F,
  SUBTYPE_NEPHILIM                    = 0x0080,
  SUBTYPE_NIGHTMARE                   = 0x0081,
  SUBTYPE_NIGHTSTALKER                = 0x0082,
  SUBTYPE_NINJA                       = 0x0083,
  SUBTYPE_NOGGLE                      = 0x0084,
  SUBTYPE_NOMAD                       = 0x0085,
  SUBTYPE_OCTOPUS                     = 0x0086,
  SUBTYPE_OGRE                        = 0x0087,
  SUBTYPE_OOZE                        = 0x0088,
  SUBTYPE_ORB                         = 0x0089,
  SUBTYPE_ORC                         = 0x008A,
  SUBTYPE_ORGG                        = 0x008B,
  SUBTYPE_OUPHE                       = 0x008C,
  SUBTYPE_OX                          = 0x008D,
  SUBTYPE_OYSTER                      = 0x008E,
  SUBTYPE_PEGASUS                     = 0x008F,
  SUBTYPE_PENTAVITE                   = 0x0090,
  SUBTYPE_PEST                        = 0x0091,
  SUBTYPE_PHELDDAGRIF                 = 0x0092,
  SUBTYPE_PHOENIX                     = 0x0093,
  SUBTYPE_PINCHER                     = 0x0094,
  SUBTYPE_PIRATE                      = 0x0095,
  SUBTYPE_PLANT                       = 0x0096,
  SUBTYPE_PRAETOR                     = 0x0097,
  SUBTYPE_PRISM                       = 0x0098,
  SUBTYPE_RABBIT                      = 0x0099,
  SUBTYPE_RAT                         = 0x009A,
  SUBTYPE_REBEL                       = 0x009B,
  SUBTYPE_REFLECTION                  = 0x009C,
  SUBTYPE_RHINO                       = 0x009D,
  SUBTYPE_RIGGER                      = 0x009E,
  SUBTYPE_ROGUE                       = 0x009F,
  SUBTYPE_SALAMANDER                  = 0x00A0,
  SUBTYPE_SAMURAI                     = 0x00A1,
  SUBTYPE_SAND                        = 0x00A2,
  SUBTYPE_SAPROLING                   = 0x00A3,
  SUBTYPE_SATYR                       = 0x00A4,
  SUBTYPE_SCARECROW                   = 0x00A5,
  SUBTYPE_SCORPION                    = 0x00A6,
  SUBTYPE_SCOUT                       = 0x00A7,
  SUBTYPE_SERF                        = 0x00A8,
  SUBTYPE_SERPENT                     = 0x00A9,
  SUBTYPE_SHADE                       = 0x00AA,
  SUBTYPE_SHAMAN                      = 0x00AB,
  SUBTYPE_SHAPESHIFTER                = 0x00AC,
  SUBTYPE_SHEEP                       = 0x00AD,
  SUBTYPE_SIREN                       = 0x00AE,
  SUBTYPE_SKELETON                    = 0x00AF,
  SUBTYPE_SLITH                       = 0x00B0,
  SUBTYPE_SLIVER                      = 0x00B1,
  SUBTYPE_SLUG                        = 0x00B2,
  SUBTYPE_SNAKE                       = 0x00B3,
  SUBTYPE_SOLDIER                     = 0x00B4,
  SUBTYPE_SOLTARI                     = 0x00B5,
  SUBTYPE_SPAWN                       = 0x00B6,
  SUBTYPE_SPECTER                     = 0x00B7,
  SUBTYPE_SPELLSHAPER                 = 0x00B8,
  SUBTYPE_SPHINX                      = 0x00B9,
  SUBTYPE_SPIDER                      = 0x00BA,
  SUBTYPE_SPIKE                       = 0x00BB,
  SUBTYPE_SPIRIT                      = 0x00BC,
  SUBTYPE_SPLINTER                    = 0x00BD,
  SUBTYPE_SPONGE                      = 0x00BE,
  SUBTYPE_SQUID                       = 0x00BF,
  SUBTYPE_SQUIRREL                    = 0x00C0,
  SUBTYPE_STARFISH                    = 0x00C1,
  SUBTYPE_SURRAKAR                    = 0x00C2,
  SUBTYPE_SURVIVOR                    = 0x00C3,
  SUBTYPE_TETRAVITE                   = 0x00C4,
  SUBTYPE_THALAKOS                    = 0x00C5,
  SUBTYPE_THOPTER                     = 0x00C6,
  SUBTYPE_THRULL                      = 0x00C7,
  SUBTYPE_TREEFOLK                    = 0x00C8,
  SUBTYPE_TRISKELAVITE                = 0x00C9,
  SUBTYPE_TROLL                       = 0x00CA,
  SUBTYPE_TURTLE                      = 0x00CB,
  SUBTYPE_UNICORN                     = 0x00CC,
  SUBTYPE_VAMPIRE                     = 0x00CD,
  SUBTYPE_VEDALKEN                    = 0x00CE,
  SUBTYPE_VIASHINO                    = 0x00CF,
  SUBTYPE_VOLVER                      = 0x00D0,
  SUBTYPE_WALL                        = 0x00D1,
  SUBTYPE_WARRIOR                     = 0x00D2,
  SUBTYPE_WEIRD                       = 0x00D3,
  SUBTYPE_WEREWOLF                    = 0x00D4,
  SUBTYPE_WHALE                       = 0x00D5,
  SUBTYPE_WIZARD                      = 0x00D6,
  SUBTYPE_WOLF                        = 0x00D7,
  SUBTYPE_WOLVERINE                   = 0x00D8,
  SUBTYPE_WOMBAT                      = 0x00D9,
  SUBTYPE_WORM                        = 0x00DA,
  SUBTYPE_WRAITH                      = 0x00DB,
  SUBTYPE_WURM                        = 0x00DC,
  SUBTYPE_YETI                        = 0x00DD,
  SUBTYPE_ZOMBIE                      = 0x00DE,
  SUBTYPE_ZUBERA                      = 0x00DF,
  SUBTYPE_ADVISOR                     = 0x00E0,
  SUBTYPE_LAMIA                       = 0x00E1,
  SUBTYPE_NYMPH                       = 0x00E2,
  SUBTYPE_SABLE                       = 0x00E3,
  SUBTYPE_HEAD                        = 0x00E4,
  SUBTYPE_GOD                         = 0x00E5,
  SUBTYPE_REVELER                     = 0x00E6,
  SUBTYPE_NAGA						  = 0x00E7,
  SUBTYPE_PROCESSOR					  = 0x00E8,
  SUBTYPE_SCION						  = 0x00E9,
  SUBTYPE_MAX_USED_CREATURE_SUBTYPE	  = 0x00E9,
// 0x00F0 is used for legendary.
  SUBTYPE_NONE                        = 0x0FFE, //When set, by design the creature lost every other creature type.
  SUBTYPE_ALL_CREATURES               = 0x0FFF,	//FIXME: counts as all possible creature types
  SUBTYPE_MAX_CREATURE_SUBTYPE		  = 0x0FFF,

SUBTYPE_MIN_TYPE					= 0x1000,
  SUBTYPE_ARTIFACT                    = 0x1000,      //card types
  SUBTYPE_CREATURE                    = 0x1001,
  SUBTYPE_ENCHANTMENT                 = 0x1002,
  SUBTYPE_INSTANT                     = 0x1003,
  SUBTYPE_LAND                        = 0x1004,
  SUBTYPE_PLANE                       = 0x1005,
  SUBTYPE_PLANESWALKER                = 0x1006,
  SUBTYPE_SCHEME                      = 0x1007,
  SUBTYPE_SORCERY                     = 0x1008,
  SUBTYPE_TRIBAL                      = 0x1009,
  SUBTYPE_VANGUARD                    = 0x100A,
  SUBTYPE_HERO                        = 0x100B,
  SUBTYPE_CONSPIRACY                  = 0x100C,
  SUBTYPE_PHENOMENON                  = 0x100D,
  SUBTYPE_MAX_USED_TYPE				  = 0x100D,
  SUBTYPE_PRIVATE                     = 0x1FFF,      //internal effects, emblems, data cards, etc.
SUBTYPE_MAX_TYPE					  = 0x1FFF,

SUBTYPE_MIN_SUPERTYPE				= 0x2000,
  SUBTYPE_BASIC                       = 0x2000,      //supertypes
  SUBTYPE_LEGEND	                  = 0x2001,      // Not used at all in the csv; 0xF0 is.
  SUBTYPE_ONGOING                     = 0x2002,
  SUBTYPE_SNOW                        = 0x2003,
  SUBTYPE_WORLD                       = 0x2004,
  SUBTYPE_NONBASIC                    = 0x2005,
  SUBTYPE_ELITE                       = 0x2006,
SUBTYPE_MAX_USED_SUPERTYPE			= 0x2006,
SUBTYPE_MAX_SUPERTYPE				= 0x2FFF,

SUBTYPE_MIN_ARTIFACT_SUBTYPE		= 0x3000,
  SUBTYPE_CONTRAPTION                 = 0x3000,      //artifact types
  SUBTYPE_EQUIPMENT                   = 0x3001,
  SUBTYPE_FORTIFICATION               = 0x3002,
  SUBTYPE_HORDE_ARTIFACT              = 0x3003,		//internal use only
SUBTYPE_MAX_USED_ARTIFACT_SUBTYPE	= 0x3003,
SUBTYPE_MAX_ARTIFACT_SUBTYPE		= 0x3FFF,

SUBTYPE_MIN_ENCHANTMENT_SUBTYPE		= 0x4000,
  SUBTYPE_AURA						= 0x4000,	// This is currently assigned to all aura cards
  /* The following block of aura subtypes are not officially supported and are for our internal use only */
  SUBTYPE_AURA_ARTIFACT				= 0x4001,
  SUBTYPE_AURA_CREATURE				= 0x4002,
  SUBTYPE_AURA_ENCHANTMENT			= 0x4003,
  SUBTYPE_AURA_LAND					= 0x4004,
  SUBTYPE_AURA_PERMANENT			= 0x4005,
  SUBTYPE_AURA_PLAYER				= 0x4006,
  /* End internal use only */
  SUBTYPE_CURSE                       = 0x4010,
  SUBTYPE_SHRINE                      = 0x4020,
SUBTYPE_MAX_USED_ENCHANTMENT_SUBTYPE= 0x4020,
SUBTYPE_MAX_ENCHANTMENT_SUBTYPE		= 0x4FFF,

SUBTYPE_MIN_LAND_SUBTYPE			= 0x5000,
  SUBTYPE_DESERT                      = 0x5000,      //land types
  SUBTYPE_FOREST                      = 0x5001,
  SUBTYPE_ISLAND                      = 0x5002,
  SUBTYPE_LAIR                        = 0x5003,
  SUBTYPE_LOCUS                       = 0x5004,
  SUBTYPE_MINE                        = 0x5005,
  SUBTYPE_MOUNTAIN                    = 0x5006,
  SUBTYPE_PLAINS                      = 0x5007,
  SUBTYPE_POWER_PLANT                 = 0x5008,
  SUBTYPE_SWAMP                       = 0x5009,
  SUBTYPE_TOWER                       = 0x500A,
  SUBTYPE_URZAS                       = 0x500B,
  SUBTYPE_GATE                        = 0x500C,
SUBTYPE_MAX_USED_LAND_SUBTYPE		= 0x500C,
SUBTYPE_MAX_LAND_SUBTYPE			= 0x5FFF,

SUBTYPE_MIN_INSTANT_SORCERY_SUBTYPE	= 0x6000,
  SUBTYPE_ARCANE                      = 0x6000,      //spell types
  SUBTYPE_TRAP                        = 0x6001,
SUBTYPE_MAX_USED_INSTANT_SORCERY_SUBTYPE= 0x6001,
SUBTYPE_MAX_INSTANT_SORCERY_SUBTYPE	= 0x6FFF,

SUBTYPE_MIN_PLANESWALKER_SUBTYPE	= 0x7000,
  SUBTYPE_AJANI                       = 0x7000,      //planeswalker types
  SUBTYPE_BOLAS                       = 0x7001,
  SUBTYPE_CHANDRA                     = 0x7002,
  SUBTYPE_ELSPETH                     = 0x7003,
  SUBTYPE_GARRUK                      = 0x7004,
  SUBTYPE_GIDEON                      = 0x7005,
  SUBTYPE_JACE                        = 0x7006,
  SUBTYPE_KARN                        = 0x7007,
  SUBTYPE_KOTH                        = 0x7008,
  SUBTYPE_LILIANA                     = 0x7009,
  SUBTYPE_NISSA                       = 0x700A,
  SUBTYPE_SARKHAN                     = 0x700B,
  SUBTYPE_SORIN                       = 0x700C,
  SUBTYPE_TEZZERET                    = 0x700D,
  SUBTYPE_VENSER                      = 0x700E,
  SUBTYPE_TAMIYO                      = 0x700F,
  SUBTYPE_TIBALT                      = 0x7010,
  SUBTYPE_VRASKA                      = 0x7011,
  SUBTYPE_DOMRI                       = 0x7012,
  SUBTYPE_RAL                         = 0x7013,
  SUBTYPE_ASHIOK                      = 0x7014,
  SUBTYPE_XENAGOS                     = 0x7015,
  SUBTYPE_DACK                        = 0x7016,
  SUBTYPE_KIORA                       = 0x7017,
  SUBTYPE_DARETTI                     = 0x7018,
  SUBTYPE_FREYALISE                   = 0x7019,
  SUBTYPE_NAHIRI                      = 0x701A,
  SUBTYPE_NIXILIS                     = 0x701B,
  SUBTYPE_TEFERI                      = 0x701C,
  SUBTYPE_UGIN                        = 0x701D,
  SUBTYPE_NARSET                      = 0x701E,
SUBTYPE_MAX_USED_PLANESWALKER_SUBTYPE = 0x701E,
SUBTYPE_MAX_PLANESWALKER_SUBTYPE	  = 0x71FF,

SUBTYPE_MIN_PLANE_SUBTYPE			= 0x8000,
  SUBTYPE_ALARA                       = 0x8000,      //plane types
  SUBTYPE_ARKHOS                      = 0x8001,
  SUBTYPE_BOLASS_MEDITATION_REALM     = 0x8002,
  SUBTYPE_DOMINARIA                   = 0x8003,
  SUBTYPE_EQUILOR                     = 0x8004,
  SUBTYPE_IQUATANA                    = 0x8005,
  SUBTYPE_IR                          = 0x8006,
  SUBTYPE_KALDHEIM                    = 0x8007,
  SUBTYPE_KAMIGAWA                    = 0x8008,
  SUBTYPE_KARSUS                      = 0x8009,
  SUBTYPE_KINSHALA                    = 0x800A,
  SUBTYPE_LORWYN                      = 0x800B,
  SUBTYPE_LUVION                      = 0x800C,
  SUBTYPE_MERCADIA                    = 0x800D,
  SUBTYPE_MIRRODIN                    = 0x800E,
  SUBTYPE_MOAG                        = 0x800F,
  SUBTYPE_MURAGANDA                   = 0x8010,
  SUBTYPE_PHYREXIA                    = 0x8011,
  SUBTYPE_PYRULEA                     = 0x8012,
  SUBTYPE_RABIAH                      = 0x8013,
  SUBTYPE_RATH                        = 0x8014,
  SUBTYPE_RAVNICA                     = 0x8015,
  SUBTYPE_SEGOVIA                     = 0x8016,
  SUBTYPE_SERRAS_REALM                = 0x8017,
  SUBTYPE_SHADOWMOOR                  = 0x8018,
  SUBTYPE_SHANDALAR                   = 0x8019,
  SUBTYPE_ULGROTHA                    = 0x801A,
  SUBTYPE_VALLA                       = 0x801B,
  SUBTYPE_WILDFIRE                    = 0x801C,
  SUBTYPE_ZENDIKAR                    = 0x801D,
  SUBTYPE_AZGOL                       = 0x801E,
  SUBTYPE_BELENON                     = 0x801F,
  SUBTYPE_ERGAMON                     = 0x8020,
  SUBTYPE_FABACIN                     = 0x8021,
  SUBTYPE_INNISTRAD                   = 0x8022,
  SUBTYPE_KEPHALAI                    = 0x8023,
  SUBTYPE_KOLBAHAN                    = 0x8024,
  SUBTYPE_KYNETH                      = 0x8025,
  SUBTYPE_MONGSENG                    = 0x8026,
  SUBTYPE_NEW_PHYREXIA                = 0x8027,
  SUBTYPE_REGATHA                     = 0x8028,
  SUBTYPE_VRYN                        = 0x8029,
  SUBTYPE_XEREX                       = 0x8030,
SUBTYPE_MAX_USED_PLANE_SUBTYPE		= 0x8030,
SUBTYPE_MAX_PLANE_SUBTYPE			= 0x8FFF,
} subtype_t;

// The old hardcoded subtype codes, stored in the "subtype" columns of "manalink.csv"
typedef enum
{
  HARDCODED_SUBTYPE_INCARNATION       =   1,
  HARDCODED_SUBTYPE_ARTIFICER         =   2,
  HARDCODED_SUBTYPE_ADVISOR           =   3,
  HARDCODED_SUBTYPE_PEST              =   4,
  HARDCODED_SUBTYPE_ANGEL             =   5,
  HARDCODED_SUBTYPE_SNOW              =   6,
  HARDCODED_SUBTYPE_APE               =   7,
  HARDCODED_SUBTYPE_NIGHTSTALKER      =   8,
  HARDCODED_SUBTYPE_ARCHER            =   9,
  HARDCODED_SUBTYPE_DRYAD             =  10,
  HARDCODED_SUBTYPE_ARTIFACT          =  11,	// aura with enchant artifact
  HARDCODED_SUBTYPE_ANTELOPE          =  12,
  HARDCODED_SUBTYPE_ASSASSIN          =  13,
  HARDCODED_SUBTYPE_ATOG              =  14,
  HARDCODED_SUBTYPE_AVATAR            =  15,
  HARDCODED_SUBTYPE_ARCHON            =  16,
  HARDCODED_SUBTYPE_PLANT             =  17,
  HARDCODED_SUBTYPE_BADGER            =  18,
  HARDCODED_SUBTYPE_AUROCHS           =  19,
  HARDCODED_SUBTYPE_LOCUS             =  20,
  HARDCODED_SUBTYPE_BLINKMOTH         =  21,
  HARDCODED_SUBTYPE_BASILISK          =  22,
  HARDCODED_SUBTYPE_BAT               =  23,
  HARDCODED_SUBTYPE_BEAR              =  24,
  HARDCODED_SUBTYPE_BEAST             =  25,
  HARDCODED_SUBTYPE_BRINGER           =  26,
  HARDCODED_SUBTYPE_SLIVER            =  27,
  HARDCODED_SUBTYPE_BERSERKER         =  28,
  HARDCODED_SUBTYPE_MONK              =  29,
  HARDCODED_SUBTYPE_BOAR              =  30,
  HARDCODED_SUBTYPE_TRAP              =  31,
  HARDCODED_SUBTYPE_ALLY              =  32,
  HARDCODED_SUBTYPE_OX                =  33,
  HARDCODED_SUBTYPE_CYCLOPS           =  34,
  HARDCODED_SUBTYPE_CAMEL             =  35,
  HARDCODED_SUBTYPE_EQUIPMENT         =  36,
  HARDCODED_SUBTYPE_CEPHALID          =  37,
  HARDCODED_SUBTYPE_CHIMERA           =  38,
  HARDCODED_SUBTYPE_CENTAUR           =  39,
  HARDCODED_SUBTYPE_CLERIC            =  40,
  HARDCODED_SUBTYPE_SHAPESHIFTER      =  41,
  HARDCODED_SUBTYPE_CONSTRUCT         =  42,
  HARDCODED_SUBTYPE_COCKATRICE        =  43,
  HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE =  44,
  HARDCODED_SUBTYPE_CURSE             =  45,
  HARDCODED_SUBTYPE_DEMON             =  46,
  HARDCODED_SUBTYPE_ELDRAZI           =  47,
  HARDCODED_SUBTYPE_DEVIL             =  48,
  HARDCODED_SUBTYPE_CRAB              =  49,
  HARDCODED_SUBTYPE_DJINN             =  50,
  HARDCODED_SUBTYPE_SHAMAN            =  51,
  HARDCODED_SUBTYPE_DRAGON            =  52,
  HARDCODED_SUBTYPE_DAUTHI            =  53,
  HARDCODED_SUBTYPE_DRAKE             =  54,
  HARDCODED_SUBTYPE_DESERTER          =  55,
  HARDCODED_SUBTYPE_DRUID             =  56,
  HARDCODED_SUBTYPE_DWARF             =  57,
  HARDCODED_SUBTYPE_EYE               =  58,
  HARDCODED_SUBTYPE_DREADNOUGHT       =  59,
  HARDCODED_SUBTYPE_DRONE             =  60,
  HARDCODED_SUBTYPE_EFREET            =  61,
  HARDCODED_SUBTYPE_EGG               =  62,
  HARDCODED_SUBTYPE_ELK               =  63,
  HARDCODED_SUBTYPE_ELDER             =  64,
  HARDCODED_SUBTYPE_ELEMENTAL         =  65,
  HARDCODED_SUBTYPE_ELEPHANT          =  66,
  HARDCODED_SUBTYPE_ELF               =  67,
  HARDCODED_SUBTYPE_ENCHANTMENT       =  68,	// aura with enchant enchantment
  HARDCODED_SUBTYPE_NONBASIC_LAND     =  69,
  HARDCODED_SUBTYPE_FLAGBEARER        =  70,
  HARDCODED_SUBTYPE_MINION            =  71,
  HARDCODED_SUBTYPE_FOX               =  72,
  HARDCODED_SUBTYPE_FAERIE            =  73,
  HARDCODED_SUBTYPE_HOMUNCULUS        =  74,
  HARDCODED_SUBTYPE_FROG              =  75,
  HARDCODED_SUBTYPE_GOAT              =  76,
  HARDCODED_SUBTYPE_GRAVEBORN         =  77,
  HARDCODED_SUBTYPE_HARPY             =  78,
  HARDCODED_SUBTYPE_FUNGUS            =  79,
  HARDCODED_SUBTYPE_HELLION           =  80,
  HARDCODED_SUBTYPE_GARGOYLE          =  81,
  HARDCODED_SUBTYPE_HIPPO             =  82,
  HARDCODED_SUBTYPE_HOMARID           =  83,
  HARDCODED_SUBTYPE_GIANT             =  84,
  HARDCODED_SUBTYPE_GNOME             =  85,
  HARDCODED_SUBTYPE_GOBLIN            =  86,
  HARDCODED_SUBTYPE_LIZARD            =  87,
  HARDCODED_SUBTYPE_HYENA             =  88,
  HARDCODED_SUBTYPE_JELLYFISH         =  89,
  HARDCODED_SUBTYPE_SOLDIER           =  90,
  HARDCODED_SUBTYPE_GORGON            =  91,
  HARDCODED_SUBTYPE_HAG               =  92,
  HARDCODED_SUBTYPE_HUMAN             =  93,
  HARDCODED_SUBTYPE_HORROR            =  94,
  HARDCODED_SUBTYPE_HORSE             =  95,
  HARDCODED_SUBTYPE_KAVU              =  96,
  HARDCODED_SUBTYPE_HYDRA             =  97,
  HARDCODED_SUBTYPE_IMP               =  98,
  HARDCODED_SUBTYPE_KIRIN             =  99,
  HARDCODED_SUBTYPE_KOR               = 100,
  HARDCODED_SUBTYPE_JUGGERNAUT        = 101,
  HARDCODED_SUBTYPE_KRAKEN            = 102,
  HARDCODED_SUBTYPE_LAMMASU           = 103,
  HARDCODED_SUBTYPE_KITHKIN           = 104,
  HARDCODED_SUBTYPE_KNIGHT            = 105,
  HARDCODED_SUBTYPE_GOLEM             = 106,
  HARDCODED_SUBTYPE_KOBOLD            = 107,
  HARDCODED_SUBTYPE_LAND              = 108,	// aura with enchant land
  HARDCODED_SUBTYPE_LEECH             = 109,
  HARDCODED_SUBTYPE_UNUSED            = 110,
  HARDCODED_SUBTYPE_MOUNTAIN          = 111,
  HARDCODED_SUBTYPE_LICID             = 112,
  HARDCODED_SUBTYPE_METATHRAN         = 113,
  HARDCODED_SUBTYPE_LEVIATHAN         = 114,
  HARDCODED_SUBTYPE_MONGER            = 115,
  HARDCODED_SUBTYPE_MONGOOSE          = 116,
  HARDCODED_SUBTYPE_DESERT            = 117,
  HARDCODED_SUBTYPE_MOONFOLK          = 118,
  HARDCODED_SUBTYPE_MUTANT            = 119,
  HARDCODED_SUBTYPE_MYSTIC            = 120,
  HARDCODED_SUBTYPE_BIRD              = 121,
  HARDCODED_SUBTYPE_MANTICORE         = 122,
  HARDCODED_SUBTYPE_NAUTILUS          = 123,
  HARDCODED_SUBTYPE_NEPHILIM          = 124,
  HARDCODED_SUBTYPE_MERFOLK           = 125,
  HARDCODED_SUBTYPE_MINOTAUR          = 126,
  HARDCODED_SUBTYPE_NINJA             = 127,
  HARDCODED_SUBTYPE_MASTICORE         = 128,
  HARDCODED_SUBTYPE_OCTOPUS           = 129,
  HARDCODED_SUBTYPE_MYR               = 130,
  HARDCODED_SUBTYPE_ORB               = 131,
  HARDCODED_SUBTYPE_ORGG              = 132,
  HARDCODED_SUBTYPE_OYSTER            = 133,
  HARDCODED_SUBTYPE_OUPHE             = 134,
  HARDCODED_SUBTYPE_NIGHTMARE         = 135,
  HARDCODED_SUBTYPE_NOMAD             = 136,
  HARDCODED_SUBTYPE_ISLAND            = 137,
  HARDCODED_SUBTYPE_OGRE              = 138,
  HARDCODED_SUBTYPE_PENTAVITE         = 139,
  HARDCODED_SUBTYPE_OOZE              = 140,
  HARDCODED_SUBTYPE_ORC               = 141,
  HARDCODED_SUBTYPE_FOREST            = 142,
  HARDCODED_SUBTYPE_PEGASUS           = 143,
  HARDCODED_SUBTYPE_SWAMP             = 144,
  HARDCODED_SUBTYPE_PHELDDAGRIF       = 145,
  HARDCODED_SUBTYPE_PHOENIX           = 146,
  HARDCODED_SUBTYPE_PINCHER           = 147,
  HARDCODED_SUBTYPE_PLAINS            = 148,
  HARDCODED_SUBTYPE_PRISM             = 149,
  HARDCODED_SUBTYPE_RABBIT            = 150,
  HARDCODED_SUBTYPE_REFLECTION        = 151,
  HARDCODED_SUBTYPE_BARBARIAN         = 152,
  HARDCODED_SUBTYPE_RHINO             = 153,
  HARDCODED_SUBTYPE_RAT               = 154,
  HARDCODED_SUBTYPE_CAT               = 155,
  HARDCODED_SUBTYPE_ROGUE             = 156,
  HARDCODED_SUBTYPE_RIGGER            = 157,
  HARDCODED_SUBTYPE_SALAMANDER        = 158,
  HARDCODED_SUBTYPE_SCOUT             = 159,
  HARDCODED_SUBTYPE_SATYR             = 160,
  HARDCODED_SUBTYPE_SCARECROW         = 161,
  HARDCODED_SUBTYPE_SCORPION          = 162,
  HARDCODED_SUBTYPE_SERPENT           = 163,
  HARDCODED_SUBTYPE_SHADE             = 164,
  HARDCODED_SUBTYPE_PERMANENT         = 165,	// aura with enchant permanent
  HARDCODED_SUBTYPE_FISH              = 166,
  HARDCODED_SUBTYPE_PIRATE            = 167,
  HARDCODED_SUBTYPE_SAMURAI           = 168,
  HARDCODED_SUBTYPE_SAPROLING         = 169,
  HARDCODED_SUBTYPE_SERF              = 170,
  HARDCODED_SUBTYPE_SKELETON          = 171,
  HARDCODED_SUBTYPE_SLUG              = 172,
  HARDCODED_SUBTYPE_SHEEP             = 173,
  HARDCODED_SUBTYPE_SLITH             = 174,
  HARDCODED_SUBTYPE_SPAWN             = 175,
  HARDCODED_SUBTYPE_SPECTER           = 176,
  HARDCODED_SUBTYPE_SPHINX            = 177,
  HARDCODED_SUBTYPE_SPIDER            = 178,
  HARDCODED_SUBTYPE_SPIRIT            = 179,
  HARDCODED_SUBTYPE_SOLTARI           = 180,
  HARDCODED_SUBTYPE_SPELLSHAPER       = 181,
  HARDCODED_SUBTYPE_GRIFFIN           = 182,
  HARDCODED_SUBTYPE_SPIKE             = 183,
  HARDCODED_SUBTYPE_TURTLE            = 184,
  HARDCODED_SUBTYPE_THOPTER           = 185,
  HARDCODED_SUBTYPE_TREEFOLK          = 186,
  HARDCODED_SUBTYPE_TROLL             = 187,
  HARDCODED_SUBTYPE_BASIC_LAND        = 188,
  HARDCODED_SUBTYPE_WEREWOLF          = 189,
  HARDCODED_SUBTYPE_UNICORN           = 190,
  HARDCODED_SUBTYPE_VAMPIRE           = 191,
  HARDCODED_SUBTYPE_SPONGE            = 192,
  HARDCODED_SUBTYPE_SQUID             = 193,
  HARDCODED_SUBTYPE_SQUIRREL          = 194,
  HARDCODED_SUBTYPE_STARFISH          = 195,
  HARDCODED_SUBTYPE_WARRIOR           = 196,
  HARDCODED_SUBTYPE_WALL              = 197,
  HARDCODED_SUBTYPE_PRAETOR           = 198,
  HARDCODED_SUBTYPE_THALAKOS          = 199,
  HARDCODED_SUBTYPE_THRULL            = 200,
  HARDCODED_SUBTYPE_WIZARD            = 201,
  HARDCODED_SUBTYPE_WOLVERINE         = 202,
  HARDCODED_SUBTYPE_WOLF              = 203,
  HARDCODED_SUBTYPE_WOMBAT            = 204,
  HARDCODED_SUBTYPE_WORLD             = 205,
  HARDCODED_SUBTYPE_WRAITH            = 206,
  HARDCODED_SUBTYPE_TRISKELAVITE      = 207,
  HARDCODED_SUBTYPE_WURM              = 208,
  HARDCODED_SUBTYPE_YETI              = 209,
  HARDCODED_SUBTYPE_ZOMBIE            = 210,
  HARDCODED_SUBTYPE_NONE              = 211,
  HARDCODED_SUBTYPE_CROCODILE         = 212,
  HARDCODED_SUBTYPE_ILLUSION          = 213,
  HARDCODED_SUBTYPE_INSECT            = 214,
  HARDCODED_SUBTYPE_VEDALKEN          = 215,
  HARDCODED_SUBTYPE_VOLVER            = 216,
  HARDCODED_SUBTYPE_WEIRD             = 217,
  HARDCODED_SUBTYPE_SNAKE             = 218,
  HARDCODED_SUBTYPE_SAND              = 219,
  HARDCODED_SUBTYPE_TETRAVITE         = 220,
  HARDCODED_SUBTYPE_WHALE             = 221,
  HARDCODED_SUBTYPE_WORM              = 222,
  HARDCODED_SUBTYPE_ZUBERA            = 223,
  HARDCODED_SUBTYPE_ASSEMBLY_WORKER   = 224,
  HARDCODED_SUBTYPE_HOUND             = 225,
  HARDCODED_SUBTYPE_LHURGOYF          = 226,
  HARDCODED_SUBTYPE_TRIBAL            = 227,
  HARDCODED_SUBTYPE_SHRINE            = 228,
  HARDCODED_SUBTYPE_CARRIER           = 229,
  HARDCODED_SUBTYPE_MERCENARY         = 230,
  HARDCODED_SUBTYPE_REBEL             = 231,
  HARDCODED_SUBTYPE_VIASHINO          = 232,
  HARDCODED_SUBTYPE_INSTANT           = 233,	// aura with enchant instant
  HARDCODED_SUBTYPE_PLAYER            = 234,	// aura with enchant player or opponent
} hardcoded_subtype_t;
 #endif