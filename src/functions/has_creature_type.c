#include "manalink.h"

static const subtype_t arr_from_hardcoded_subtype_to_subtype[] =
{
  0,
  SUBTYPE_INCARNATION,	// HARDCODED_SUBTYPE_INCARNATION
  SUBTYPE_ARTIFICER,	// HARDCODED_SUBTYPE_ARTIFICER
  SUBTYPE_ADVISOR,		// HARDCODED_SUBTYPE_ADVISOR
  SUBTYPE_PEST,			// HARDCODED_SUBTYPE_PEST
  SUBTYPE_ANGEL,		// HARDCODED_SUBTYPE_ANGEL
  SUBTYPE_SNOW,			// HARDCODED_SUBTYPE_SNOW
  SUBTYPE_APE,			// HARDCODED_SUBTYPE_APE
  SUBTYPE_NIGHTSTALKER,	// HARDCODED_SUBTYPE_NIGHTSTALKER
  SUBTYPE_ARCHER,		// HARDCODED_SUBTYPE_ARCHER
  SUBTYPE_DRYAD,		// HARDCODED_SUBTYPE_DRYAD
  SUBTYPE_AURA_ARTIFACT,// HARDCODED_SUBTYPE_ARTIFACT
  SUBTYPE_ANTELOPE,		// HARDCODED_SUBTYPE_ANTELOPE
  SUBTYPE_ASSASSIN,		// HARDCODED_SUBTYPE_ASSASSIN
  SUBTYPE_ATOG,			// HARDCODED_SUBTYPE_ATOG
  SUBTYPE_AVATAR,		// HARDCODED_SUBTYPE_AVATAR
  SUBTYPE_ARCHON,		// HARDCODED_SUBTYPE_ARCHON
  SUBTYPE_PLANT,		// HARDCODED_SUBTYPE_PLANT
  SUBTYPE_BADGER,		// HARDCODED_SUBTYPE_BADGER
  SUBTYPE_AUROCHS,		// HARDCODED_SUBTYPE_AUROCHS
  SUBTYPE_LOCUS,		// HARDCODED_SUBTYPE_LOCUS
  SUBTYPE_BLINKMOTH,	// HARDCODED_SUBTYPE_BLINKMOTH
  SUBTYPE_BASILISK,		// HARDCODED_SUBTYPE_BASILISK
  SUBTYPE_BAT,			// HARDCODED_SUBTYPE_BAT
  SUBTYPE_BEAR,			// HARDCODED_SUBTYPE_BEAR
  SUBTYPE_BEAST,		// HARDCODED_SUBTYPE_BEAST
  SUBTYPE_BRINGER,		// HARDCODED_SUBTYPE_BRINGER
  SUBTYPE_SLIVER,		// HARDCODED_SUBTYPE_SLIVER
  SUBTYPE_BERSERKER,	// HARDCODED_SUBTYPE_BERSERKER
  SUBTYPE_MONK,			// HARDCODED_SUBTYPE_MONK
  SUBTYPE_BOAR,			// HARDCODED_SUBTYPE_BOAR
  SUBTYPE_TRAP,			// HARDCODED_SUBTYPE_TRAP
  SUBTYPE_ALLY,			// HARDCODED_SUBTYPE_ALLY
  SUBTYPE_OX,			// HARDCODED_SUBTYPE_OX
  SUBTYPE_CYCLOPS,		// HARDCODED_SUBTYPE_CYCLOPS
  SUBTYPE_CAMEL,		// HARDCODED_SUBTYPE_CAMEL
  SUBTYPE_EQUIPMENT,	// HARDCODED_SUBTYPE_EQUIPMENT
  SUBTYPE_CEPHALID,		// HARDCODED_SUBTYPE_CEPHALID
  SUBTYPE_CHIMERA,		// HARDCODED_SUBTYPE_CHIMERA
  SUBTYPE_CENTAUR,		// HARDCODED_SUBTYPE_CENTAUR
  SUBTYPE_CLERIC,		// HARDCODED_SUBTYPE_CLERIC
  SUBTYPE_SHAPESHIFTER,	// HARDCODED_SUBTYPE_SHAPESHIFTER
  SUBTYPE_CONSTRUCT,	// HARDCODED_SUBTYPE_CONSTRUCT
  SUBTYPE_COCKATRICE,	// HARDCODED_SUBTYPE_COCKATRICE
  SUBTYPE_AURA_CREATURE,// HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE
  SUBTYPE_CURSE,		// HARDCODED_SUBTYPE_CURSE
  SUBTYPE_DEMON,		// HARDCODED_SUBTYPE_DEMON
  SUBTYPE_ELDRAZI,		// HARDCODED_SUBTYPE_ELDRAZI
  SUBTYPE_DEVIL,		// HARDCODED_SUBTYPE_DEVIL
  SUBTYPE_CRAB,			// HARDCODED_SUBTYPE_CRAB
  SUBTYPE_DJINN,		// HARDCODED_SUBTYPE_DJINN
  SUBTYPE_SHAMAN,		// HARDCODED_SUBTYPE_SHAMAN
  SUBTYPE_DRAGON,		// HARDCODED_SUBTYPE_DRAGON
  SUBTYPE_DAUTHI,		// HARDCODED_SUBTYPE_DAUTHI
  SUBTYPE_DRAKE,		// HARDCODED_SUBTYPE_DRAKE
  SUBTYPE_DESERTER,		// HARDCODED_SUBTYPE_DESERTER
  SUBTYPE_DRUID,		// HARDCODED_SUBTYPE_DRUID
  SUBTYPE_DWARF,		// HARDCODED_SUBTYPE_DWARF
  SUBTYPE_EYE,			// HARDCODED_SUBTYPE_EYE
  SUBTYPE_DREADNOUGHT,	// HARDCODED_SUBTYPE_DREADNOUGHT
  SUBTYPE_DRONE,		// HARDCODED_SUBTYPE_DRONE
  SUBTYPE_EFREET,		// HARDCODED_SUBTYPE_EFREET
  SUBTYPE_BIRD,			// HARDCODED_SUBTYPE_EGG	// Best fit
  SUBTYPE_ELK,			// HARDCODED_SUBTYPE_ELK
  SUBTYPE_ELDER,		// HARDCODED_SUBTYPE_ELDER
  SUBTYPE_ELEMENTAL,	// HARDCODED_SUBTYPE_ELEMENTAL
  SUBTYPE_ELEPHANT,		// HARDCODED_SUBTYPE_ELEPHANT
  SUBTYPE_ELF,			// HARDCODED_SUBTYPE_ELF
  SUBTYPE_AURA_ENCHANTMENT,	// HARDCODED_SUBTYPE_ENCHANTMENT
  SUBTYPE_NONBASIC,		// HARDCODED_SUBTYPE_NONBASIC_LAND
  SUBTYPE_FLAGBEARER,	// HARDCODED_SUBTYPE_FLAGBEARER
  SUBTYPE_MINION,		// HARDCODED_SUBTYPE_MINION
  SUBTYPE_FOX,			// HARDCODED_SUBTYPE_FOX
  SUBTYPE_FAERIE,		// HARDCODED_SUBTYPE_FAERIE
  SUBTYPE_HOMUNCULUS,	// HARDCODED_SUBTYPE_HOMUNCULUS
  SUBTYPE_FROG,			// HARDCODED_SUBTYPE_FROG
  SUBTYPE_GOAT,			// HARDCODED_SUBTYPE_GOAT
  SUBTYPE_GRAVEBORN,	// HARDCODED_SUBTYPE_GRAVEBORN
  SUBTYPE_HARPY,		// HARDCODED_SUBTYPE_HARPY
  SUBTYPE_FUNGUS,		// HARDCODED_SUBTYPE_FUNGUS
  SUBTYPE_HELLION,		// HARDCODED_SUBTYPE_HELLION
  SUBTYPE_GARGOYLE,		// HARDCODED_SUBTYPE_GARGOYLE
  SUBTYPE_HIPPO,		// HARDCODED_SUBTYPE_HIPPO
  SUBTYPE_HOMARID,		// HARDCODED_SUBTYPE_HOMARID
  SUBTYPE_GIANT,		// HARDCODED_SUBTYPE_GIANT
  SUBTYPE_GNOME,		// HARDCODED_SUBTYPE_GNOME
  SUBTYPE_GOBLIN,		// HARDCODED_SUBTYPE_GOBLIN
  SUBTYPE_LIZARD,		// HARDCODED_SUBTYPE_LIZARD
  SUBTYPE_HYENA,		// HARDCODED_SUBTYPE_HYENA
  SUBTYPE_JELLYFISH,	// HARDCODED_SUBTYPE_JELLYFISH
  SUBTYPE_SOLDIER,		// HARDCODED_SUBTYPE_SOLDIER
  SUBTYPE_GORGON,		// HARDCODED_SUBTYPE_GORGON
  SUBTYPE_HAG,			// HARDCODED_SUBTYPE_HAG
  SUBTYPE_HUMAN,		// HARDCODED_SUBTYPE_HUMAN
  SUBTYPE_HORROR,		// HARDCODED_SUBTYPE_HORROR
  SUBTYPE_HORSE,		// HARDCODED_SUBTYPE_HORSE
  SUBTYPE_KAVU,			// HARDCODED_SUBTYPE_KAVU
  SUBTYPE_HYDRA,		// HARDCODED_SUBTYPE_HYDRA
  SUBTYPE_IMP,			// HARDCODED_SUBTYPE_IMP
  SUBTYPE_KIRIN,		// HARDCODED_SUBTYPE_KIRIN
  SUBTYPE_KOR,			// HARDCODED_SUBTYPE_KOR
  SUBTYPE_JUGGERNAUT,	// HARDCODED_SUBTYPE_JUGGERNAUT
  SUBTYPE_KRAKEN,		// HARDCODED_SUBTYPE_KRAKEN
  SUBTYPE_LAMMASU,		// HARDCODED_SUBTYPE_LAMMASU
  SUBTYPE_KITHKIN,		// HARDCODED_SUBTYPE_KITHKIN
  SUBTYPE_KNIGHT,		// HARDCODED_SUBTYPE_KNIGHT
  SUBTYPE_GOLEM,		// HARDCODED_SUBTYPE_GOLEM
  SUBTYPE_KOBOLD,		// HARDCODED_SUBTYPE_KOBOLD
  SUBTYPE_AURA_LAND,	// HARDCODED_SUBTYPE_LAND
  SUBTYPE_LEECH,		// HARDCODED_SUBTYPE_LEECH
  0,					// HARDCODED_SUBTYPE_UNUSED
  SUBTYPE_MOUNTAIN,		// HARDCODED_SUBTYPE_MOUNTAIN
  SUBTYPE_LICID,		// HARDCODED_SUBTYPE_LICID
  SUBTYPE_METATHRAN,	// HARDCODED_SUBTYPE_METATHRAN
  SUBTYPE_LEVIATHAN,	// HARDCODED_SUBTYPE_LEVIATHAN
  SUBTYPE_MONGER,		// HARDCODED_SUBTYPE_MONGER
  SUBTYPE_MONGOOSE,		// HARDCODED_SUBTYPE_MONGOOSE
  SUBTYPE_DESERT,		// HARDCODED_SUBTYPE_DESERT
  SUBTYPE_MOONFOLK,		// HARDCODED_SUBTYPE_MOONFOLK
  SUBTYPE_MUTANT,		// HARDCODED_SUBTYPE_MUTANT
  SUBTYPE_MYSTIC,		// HARDCODED_SUBTYPE_MYSTIC
  SUBTYPE_BIRD,			// HARDCODED_SUBTYPE_BIRD
  SUBTYPE_MANTICORE,	// HARDCODED_SUBTYPE_MANTICORE
  SUBTYPE_NAUTILUS,		// HARDCODED_SUBTYPE_NAUTILUS
  SUBTYPE_NEPHILIM,		// HARDCODED_SUBTYPE_NEPHILIM
  SUBTYPE_MERFOLK,		// HARDCODED_SUBTYPE_MERFOLK
  SUBTYPE_MINOTAUR,		// HARDCODED_SUBTYPE_MINOTAUR
  SUBTYPE_NINJA,		// HARDCODED_SUBTYPE_NINJA
  SUBTYPE_MASTICORE,	// HARDCODED_SUBTYPE_MASTICORE
  SUBTYPE_OCTOPUS,		// HARDCODED_SUBTYPE_OCTOPUS
  SUBTYPE_MYR,			// HARDCODED_SUBTYPE_MYR
  SUBTYPE_ORB,			// HARDCODED_SUBTYPE_ORB
  SUBTYPE_ORGG,			// HARDCODED_SUBTYPE_ORGG
  SUBTYPE_OYSTER,		// HARDCODED_SUBTYPE_OYSTER
  SUBTYPE_OUPHE,		// HARDCODED_SUBTYPE_OUPHE
  SUBTYPE_NIGHTMARE,	// HARDCODED_SUBTYPE_NIGHTMARE
  SUBTYPE_NOMAD,		// HARDCODED_SUBTYPE_NOMAD
  SUBTYPE_ISLAND,		// HARDCODED_SUBTYPE_ISLAND
  SUBTYPE_OGRE,			// HARDCODED_SUBTYPE_OGRE
  SUBTYPE_PENTAVITE,	// HARDCODED_SUBTYPE_PENTAVITE
  SUBTYPE_OOZE,			// HARDCODED_SUBTYPE_OOZE
  SUBTYPE_ORC,			// HARDCODED_SUBTYPE_ORC
  SUBTYPE_FOREST,		// HARDCODED_SUBTYPE_FOREST
  SUBTYPE_PEGASUS,		// HARDCODED_SUBTYPE_PEGASUS
  SUBTYPE_SWAMP,		// HARDCODED_SUBTYPE_SWAMP
  SUBTYPE_PHELDDAGRIF,	// HARDCODED_SUBTYPE_PHELDDAGRIF
  SUBTYPE_PHOENIX,		// HARDCODED_SUBTYPE_PHOENIX
  SUBTYPE_PINCHER,		// HARDCODED_SUBTYPE_PINCHER
  SUBTYPE_PLAINS,		// HARDCODED_SUBTYPE_PLAINS
  SUBTYPE_PRISM,		// HARDCODED_SUBTYPE_PRISM
  SUBTYPE_RABBIT,		// HARDCODED_SUBTYPE_RABBIT
  SUBTYPE_REFLECTION,	// HARDCODED_SUBTYPE_REFLECTION
  SUBTYPE_BARBARIAN,	// HARDCODED_SUBTYPE_BARBARIAN
  SUBTYPE_RHINO,		// HARDCODED_SUBTYPE_RHINO
  SUBTYPE_RAT,			// HARDCODED_SUBTYPE_RAT
  SUBTYPE_CAT,			// HARDCODED_SUBTYPE_CAT
  SUBTYPE_ROGUE,		// HARDCODED_SUBTYPE_ROGUE
  SUBTYPE_RIGGER,		// HARDCODED_SUBTYPE_RIGGER
  SUBTYPE_SALAMANDER,	// HARDCODED_SUBTYPE_SALAMANDER
  SUBTYPE_SCOUT,		// HARDCODED_SUBTYPE_SCOUT
  SUBTYPE_SATYR,		// HARDCODED_SUBTYPE_SATYR
  SUBTYPE_SCARECROW,	// HARDCODED_SUBTYPE_SCARECROW
  SUBTYPE_SCORPION,		// HARDCODED_SUBTYPE_SCORPION
  SUBTYPE_SERPENT,		// HARDCODED_SUBTYPE_SERPENT
  SUBTYPE_SHADE,		// HARDCODED_SUBTYPE_SHADE
  SUBTYPE_AURA_PERMANENT,	// HARDCODED_SUBTYPE_PERMANENT
  SUBTYPE_FISH,			// HARDCODED_SUBTYPE_FISH
  SUBTYPE_PIRATE,		// HARDCODED_SUBTYPE_PIRATE
  SUBTYPE_SAMURAI,		// HARDCODED_SUBTYPE_SAMURAI
  SUBTYPE_SAPROLING,	// HARDCODED_SUBTYPE_SAPROLING
  SUBTYPE_SERF,			// HARDCODED_SUBTYPE_SERF
  SUBTYPE_SKELETON,		// HARDCODED_SUBTYPE_SKELETON
  SUBTYPE_SLUG,			// HARDCODED_SUBTYPE_SLUG
  SUBTYPE_SHEEP,		// HARDCODED_SUBTYPE_SHEEP
  SUBTYPE_SLITH,		// HARDCODED_SUBTYPE_SLITH
  SUBTYPE_SPAWN,		// HARDCODED_SUBTYPE_SPAWN
  SUBTYPE_SPECTER,		// HARDCODED_SUBTYPE_SPECTER
  SUBTYPE_SPHINX,		// HARDCODED_SUBTYPE_SPHINX
  SUBTYPE_SPIDER,		// HARDCODED_SUBTYPE_SPIDER
  SUBTYPE_SPIRIT,		// HARDCODED_SUBTYPE_SPIRIT
  SUBTYPE_SOLTARI,		// HARDCODED_SUBTYPE_SOLTARI
  SUBTYPE_SPELLSHAPER,	// HARDCODED_SUBTYPE_SPELLSHAPER
  SUBTYPE_GRIFFIN,		// HARDCODED_SUBTYPE_GRIFFIN
  SUBTYPE_SPIKE,		// HARDCODED_SUBTYPE_SPIKE
  SUBTYPE_TURTLE,		// HARDCODED_SUBTYPE_TURTLE
  SUBTYPE_THOPTER,		// HARDCODED_SUBTYPE_THOPTER
  SUBTYPE_TREEFOLK,		// HARDCODED_SUBTYPE_TREEFOLK
  SUBTYPE_TROLL,		// HARDCODED_SUBTYPE_TROLL
  SUBTYPE_BASIC,		// HARDCODED_SUBTYPE_BASIC_LAND
  SUBTYPE_WEREWOLF,		// HARDCODED_SUBTYPE_WEREWOLF
  SUBTYPE_UNICORN,		// HARDCODED_SUBTYPE_UNICORN
  SUBTYPE_VAMPIRE,		// HARDCODED_SUBTYPE_VAMPIRE
  SUBTYPE_SPONGE,		// HARDCODED_SUBTYPE_SPONGE
  SUBTYPE_SQUID,		// HARDCODED_SUBTYPE_SQUID
  SUBTYPE_SQUIRREL,		// HARDCODED_SUBTYPE_SQUIRREL
  SUBTYPE_STARFISH,		// HARDCODED_SUBTYPE_STARFISH
  SUBTYPE_WARRIOR,		// HARDCODED_SUBTYPE_WARRIOR
  SUBTYPE_WALL,			// HARDCODED_SUBTYPE_WALL
  SUBTYPE_PRAETOR,		// HARDCODED_SUBTYPE_PRAETOR
  SUBTYPE_THALAKOS,		// HARDCODED_SUBTYPE_THALAKOS
  SUBTYPE_THRULL,		// HARDCODED_SUBTYPE_THRULL
  SUBTYPE_WIZARD,		// HARDCODED_SUBTYPE_WIZARD
  SUBTYPE_WOLVERINE,	// HARDCODED_SUBTYPE_WOLVERINE
  SUBTYPE_WOLF,			// HARDCODED_SUBTYPE_WOLF
  SUBTYPE_WOMBAT,		// HARDCODED_SUBTYPE_WOMBAT
  SUBTYPE_WORLD,		// HARDCODED_SUBTYPE_WORLD
  SUBTYPE_WRAITH,		// HARDCODED_SUBTYPE_WRAITH
  SUBTYPE_TRISKELAVITE,	// HARDCODED_SUBTYPE_TRISKELAVITE
  SUBTYPE_WURM,			// HARDCODED_SUBTYPE_WURM
  SUBTYPE_YETI,			// HARDCODED_SUBTYPE_YETI
  SUBTYPE_ZOMBIE,		// HARDCODED_SUBTYPE_ZOMBIE
  0,					// HARDCODED_SUBTYPE_NONE	// deliberately not SUBTYPE_NONE
  SUBTYPE_CROCODILE,	// HARDCODED_SUBTYPE_CROCODILE
  SUBTYPE_ILLUSION,		// HARDCODED_SUBTYPE_ILLUSION
  SUBTYPE_INSECT,		// HARDCODED_SUBTYPE_INSECT
  SUBTYPE_VEDALKEN,		// HARDCODED_SUBTYPE_VEDALKEN
  SUBTYPE_VOLVER,		// HARDCODED_SUBTYPE_VOLVER
  SUBTYPE_WEIRD,		// HARDCODED_SUBTYPE_WEIRD
  SUBTYPE_SNAKE,		// HARDCODED_SUBTYPE_SNAKE
  SUBTYPE_SAND,			// HARDCODED_SUBTYPE_SAND
  SUBTYPE_TETRAVITE,	// HARDCODED_SUBTYPE_TETRAVITE
  SUBTYPE_WHALE,		// HARDCODED_SUBTYPE_WHALE
  SUBTYPE_WORM,			// HARDCODED_SUBTYPE_WORM
  SUBTYPE_ZUBERA,		// HARDCODED_SUBTYPE_ZUBERA
  SUBTYPE_ASSEMBLY_WORKER,	// HARDCODED_SUBTYPE_ASSEMBLY_WORKER
  SUBTYPE_HOUND,		// HARDCODED_SUBTYPE_HOUND
  SUBTYPE_LHURGOYF,		// HARDCODED_SUBTYPE_LHURGOYF
  SUBTYPE_TRIBAL,		// HARDCODED_SUBTYPE_TRIBAL
  SUBTYPE_SHRINE,		// HARDCODED_SUBTYPE_SHRINE
  SUBTYPE_CARRIER,		// HARDCODED_SUBTYPE_CARRIER
  SUBTYPE_MERCENARY,	// HARDCODED_SUBTYPE_MERCENARY
  SUBTYPE_REBEL,		// HARDCODED_SUBTYPE_REBEL
  SUBTYPE_VIASHINO,		// HARDCODED_SUBTYPE_VIASHINO
};
STATIC_ASSERT(sizeof(arr_from_hardcoded_subtype_to_subtype)/sizeof(subtype_t) == HARDCODED_SUBTYPE_VIASHINO + 1,
			  indices_in__arr_from_hardcoded_subtype_to_subtype__are_mismatched);

/* Only needed for engine.c:effect_asterisk(), field ASTERISK_SUBTYPE_OF_INFO_SLOT.  I'm reasonably certain that only gets called from card_swarm_of_rats() at
 * 0x4140f0, for HARDCODED_SUBTYPE_RAT; that happens to have the same value as SUBTYPE_RAT.  But no reason to risk it. */
subtype_t from_hardcodedsubtype_to_subtype(hardcoded_subtype_t hcs)
{
  if (hcs <= HARDCODED_SUBTYPE_VIASHINO)
	return arr_from_hardcoded_subtype_to_subtype[hcs];
  else
	return hcs;
}

static int is_basic_land_subtype(int subtype){
	if( subtype == SUBTYPE_MOUNTAIN ||
		subtype == SUBTYPE_ISLAND ||
		subtype == SUBTYPE_PLAINS ||
		subtype == SUBTYPE_SWAMP ||
		subtype == SUBTYPE_FOREST
	  ){
		return 1;
	}
	return 0;
}

static int is_creature_type(int new_type){
	return new_type >= SUBTYPE_MIN_CREATURE_SUBTYPE && new_type <= SUBTYPE_MAX_CREATURE_SUBTYPE;
}

static int is_type_type(int new_type){
	return new_type >= SUBTYPE_MIN_TYPE && new_type <= SUBTYPE_MAX_TYPE;
}

static int is_supertype_type(int new_type){
	return new_type >= SUBTYPE_MIN_SUPERTYPE && new_type <= SUBTYPE_MAX_SUPERTYPE;
}

static int has_new_types(int id, int subtype){

	if( subtype == -1 ){
		return 0;
	}

	card_ptr_t* c = cards_ptr[id];

	if( c->types[0] == subtype ||
		c->types[1] == subtype ||
		c->types[2] == subtype ||
		c->types[3] == subtype ||
		c->types[4] == subtype ||
		c->types[5] == subtype
	){
		return 1;
	}

	return 0;
}

static int is_changeling(int player, int card){
	card_instance_t* instance;
	if( player != -1 && (instance = in_play(player, card)) ){
		uint32_t stash_keywords = instance->regen_status;
		int cur_keywords = get_abilities(player, card, EVENT_ABILITIES, -1);
		instance->regen_status = stash_keywords;
		if (cur_keywords & KEYWORD_PROT_INTERRUPTS ){
			return 1;
		}
	}
	else{
		if( cards_data[card].static_ability & KEYWORD_PROT_INTERRUPTS ){
			return 1;
		}
	}
	return 0;
}

void reset_subtypes(int player, int card, int mode){
	// mode = 1 --> Reset forced subtype
	// mode = 2 --> Reset added subtype
	// mode = 3 --> Reset both

	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[15].player == -1 || (mode & 3) == 3 ){
		instance->targets[15].player = 0;
	}
	else{
		if( mode & 1 ){
			instance->targets[15].player &= ~0xFFFF0000;
		}
		if( mode & 2 ){
			instance->targets[15].player &= ~0x0000FFFF;
		}
	}
}

enum{
	ADD_SUBTYPE = 1,
	FORCE_SUBTYPE = 2,
};

static void add_force_subtype_impl(int player, int card, subtype_t subtype, int mode){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[15].player == -1 ){
		instance->targets[15].player = 0;
	}
	if( mode == ADD_SUBTYPE ){
		instance->targets[15].player &= ~0x0000FFFF;	// removes any previously-added subtype, but leaves any forced one in place
		instance->targets[15].player |= subtype & 0xFFFF;
	}
	if( mode == FORCE_SUBTYPE ){
		instance->targets[15].player = 0;	// removes any previously-added subtype
		instance->targets[15].player |= ((unsigned)(subtype & 0xFFFF) << 16);
	}
}

void force_a_subtype(int player, int card, subtype_t subtype){
	add_force_subtype_impl(player, card, subtype, FORCE_SUBTYPE);
}

static int force_a_subtype_until_eot_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( eot_trigger(player, card, event) ){
		reset_subtypes(instance->targets[0].player, instance->targets[0].card, 1);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int force_a_subtype_until_eot(int player, int card, int t_player, int t_card, subtype_t subtype){
	add_force_subtype_impl(t_player, t_card, subtype, FORCE_SUBTYPE);
	int legacy = create_targetted_legacy_effect(player, card, &force_a_subtype_until_eot_legacy, t_player, t_card);
	return legacy;
}

void add_a_subtype(int player, int card, subtype_t subtype){
	add_force_subtype_impl(player, card, subtype, ADD_SUBTYPE);
}

int get_added_subtype(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if (instance->targets[15].player == -1){
		return 0;
	} else {
		return LOWORD(instance->targets[15].player);
	}
}

static int get_forced_subtype(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if (instance->targets[15].player == -1){
		return 0;
	} else {
		return HIWORD(instance->targets[15].player);
	}
}

int has_forced_subtype(int player, int card){
	return get_forced_subtype(player, card) > 0;
}

int has_added_subtype(int player, int card){
	return get_added_subtype(player, card) > 0;
}

int has_subtype_by_id(int id, subtype_t subtype){

	if( subtype == SUBTYPE_LEGEND ){
		int fake = get_internal_card_id_from_csv_id(id);
		if( cards_data[fake].subtype == 0x0e || cards_data[fake].subtype == 0x0f ){
			return 1;
		}
	}

	if( is_creature_type(subtype) && is_changeling(-1, get_internal_card_id_from_csv_id(id)) ){
		return 1;
	}
	if( subtype == SUBTYPE_SNOW && is_basic_land_by_id(id) ){
		return 1;
	}
	if (subtype == SUBTYPE_AURA_CREATURE){
		card_ptr_t* cp = cards_ptr[id];
		return ((cp->subtype1 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE
				 || cp->subtype1 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE)
				&& has_new_types(id, SUBTYPE_AURA));
	}
	return has_new_types(id, subtype);
}

int has_subtype(int player, int card, subtype_t subtype){
	if( player == -1 ){
		return has_subtype_by_id(cards_data[card].id, subtype);
	}
	int id = get_id(player, card);
	card_instance_t *instance = get_card_instance(player, card);

	// Legendary permanents
	if( subtype == SUBTYPE_LEGEND && is_legendary(player, card) ){
		return 1;
	}

	int creature_type = is_creature_type(subtype);
	if (creature_type && !(is_what(player, card, TYPE_CREATURE) || has_subtype(player, card, SUBTYPE_TRIBAL))){
		return 0;
	}

	int is_in_play = in_play(player, card) != NULL;
	if (is_in_play){
		unsigned int forced = get_forced_subtype(player, card);
		if (forced == subtype){
			return 1;
		}
		if (forced && (is_supertype_type(subtype) || is_type_type(subtype))){
			forced = 0;
		}

		if (forced == subtype || (unsigned int)get_added_subtype(player, card) == subtype){
			return 1;
		}
		if (forced){
			return 0;
		}
	}

	if( creature_type ){
		// Mirror Entity effect / Changeling hack
		if( is_changeling(player, card) ){
			return 1;
		}

		// Hacks for cards that add creature subtypes to themselves.  Anything here must also be mirrored in shares_creature_subtype() below.
		if( is_in_play && id == CARD_ID_FIGURE_OF_DESTINY ){
			if( instance->targets[9].player > 0 ){
				if( subtype == SUBTYPE_SPIRIT && (instance->targets[9].player & 1) ){
					return 1;
				}
				if( subtype == SUBTYPE_WARRIOR && (instance->targets[9].player & 2) ){
					return 1;
				}
				if( subtype == SUBTYPE_AVATAR && instance->targets[9].player == 3 ){
					return 1;
				}
			}
		}

		if( is_in_play && id == CARD_ID_WARDEN_OF_THE_FIRST_TREE ){
			if( instance->targets[9].player > 0 ){
				if( subtype == SUBTYPE_WARRIOR && ((instance->targets[9].player & 1) || (instance->targets[9].player & 2)) ){
					return 1;
				}
				if( subtype == SUBTYPE_SPIRIT && (instance->targets[9].player & 2) ){
					return 1;
				}
			}
		}

		if( is_in_play && id == CARD_ID_AGELESS_SENTINELS ){
			if( instance->targets[5].player == 66 ){
				if( subtype == SUBTYPE_GIANT ){
					return 1;
				}
				if( subtype == SUBTYPE_BIRD ){
					return 1;
				}
			}
		}

		if( is_in_play && id == CARD_ID_DUPLICANT ){
			int i;
			for(i=2; i<6; i++){
				if( (unsigned int)instance->targets[i].player == subtype ){
					return 1;
				}
			}
		}

		if (subtype == SUBTYPE_WALL && count_counters(player, card, COUNTER_GOLD) && check_battlefield_for_id(ANYBODY, CARD_ID_AURIFICATION)){
			return 1;
		}
	}

	if (subtype == SUBTYPE_SNOW && count_counters(player, card, COUNTER_ICE) && check_battlefield_for_id(ANYBODY, CARD_ID_RIMEFEATHER_OWL)){
		return 1;
	}

	//  Prismatic Omen hack
	if( is_in_play && is_what(player, card, TYPE_LAND) && is_basic_land_subtype(subtype) ){
		if( check_special_flags2(player, card, SF2_PRISMATIC_OMEN) ){
			return 1;
		}
	}

	if (has_subtype_by_id(id, subtype)){
		return 1;
	}

	static int depth = 0;	// A bandaid for braindead coding that calls this for every invocation of a card's function.
	int rval = 0;
	if (++depth < 10 && instance->internal_card_id >= 0){
		int old_event_result = event_result;
		event_result = subtype;
		call_card_function_i(instance, player, card, EVENT_SUBTYPE);
		rval = event_result == 0;
		event_result = old_event_result;
	}
	--depth;
	return rval;
}

int has_creature_type(int player, int card, subtype_t subtype){

  if( ! is_what(player, card, TYPE_CREATURE) ){
	 return 0;
  }

  return has_subtype(player, card, subtype);
}

int is_tribal(int player, int card){
	return has_subtype(player, card, SUBTYPE_TRIBAL);
}

// Returns true if {p0,c0} shares at least one creature subtype with {p1,c1}.
int shares_creature_subtype(int p0, int c0, int p1, int c1)
{
  int i, j;
  int p[2] = {p0, p1}, c[2] = {c0, c1};	// For convenience.

  // If a card isn't a creature or a tribal, it has no creature subtypes.
  for (i = 0; i < 2; ++i)
	if (!is_what(p[i], c[i], TYPE_CREATURE) && !is_tribal(p[i], c[i]))
	  return 0;

  /* First construct a list of all subtypes each card has, and whether it's a changeling.  Unfortunately, we can't return immediately if one card is a
   * changeling, since the other might not have any creature types.  We can't even return immediately if both are, since one or the other may have lost all
   * creature types, or had its creature types forced to something else. */

#define NUM_SUBTYPES	11
  /* 0 is an added or forced subtype; 1..5 are natural subtypes from card_ptr_t; 6 is Wall added by Aurification; 7..10 are subtype hacks added by specific
   * cards to themselves. */

  int subtypes[2][NUM_SUBTYPES] = {{0}};

  int changeling[2] = {0};
  for (i = 0; i < 2; ++i)
	{
	  card_instance_t* instance = p[i] == -1 ? NULL : in_play(p[i], c[i]);
	  if (instance)
		{
		  int added = get_added_subtype(p[i], c[i]);
		  if (added)
			{
			  if (added == SUBTYPE_NONE)
				return 0;	// If this card has lost all creature types, then it can't share a subtype with the other
			  else if (added == SUBTYPE_ALL_CREATURES)
				{
				  changeling[i] = 1;
				  continue;
				}
			  else
				subtypes[i][0] = added;
			}

		  int forced = get_forced_subtype(p[i], c[i]);
		  if (forced)
			{
			  if (forced == SUBTYPE_NONE)
				return 0;	// If this card has lost all creature types, then it can't share a subtype with the other
			  else if (forced == SUBTYPE_ALL_CREATURES)
				changeling[i] = 1;
			  else
				{
				  subtypes[i][0] = forced;
				  changeling[i] = 0;
				}

			  continue;
			}
		}

	  if (is_changeling(p[i], c[i]))
		{
		  changeling[i] = 1;
		  continue;	// No need to enumerate natural subtypes
		}

	  int csvid = p[i] == -1 ? cards_data[c[i]].id : get_backup_id(p[i], c[i]);
	  card_ptr_t* cp = cards_ptr[csvid];
	  for (j = 1; j < 6; ++j)	// types[0] is always either a type or supertype; types[6] is unused
		subtypes[i][j] = cp->types[j];

	  // Hacks for cards that add creature subtypes to themselves.  Anything here must also be mirrored in has_subtype() above.
	  if (instance)	// and therefore in play
		{
		  if (count_counters(p[i], c[i], COUNTER_GOLD) && check_battlefield_for_id(ANYBODY, CARD_ID_AURIFICATION))
			subtypes[i][6] = SUBTYPE_WALL;

		  switch (csvid)
			{
				case CARD_ID_FIGURE_OF_DESTINY:
				{
					if( instance->targets[9].player > 0 ){
						if( instance->targets[9].player & 1 ){
							subtypes[i][9] = SUBTYPE_SPIRIT;
						}
						if( instance->targets[9].player & 2 ){
							subtypes[i][8] = SUBTYPE_WARRIOR;
						}
						if( instance->targets[9].player == 3 ){
							subtypes[i][7] = SUBTYPE_AVATAR;
						}
					}
				}
				break;

			  case CARD_ID_AGELESS_SENTINELS:
				if (instance->targets[5].player == 66)
				  {
					subtypes[i][7] = SUBTYPE_BIRD;
					subtypes[i][8] = SUBTYPE_GIANT;
				  }
				break;

			  case CARD_ID_DUPLICANT:
				for (j = 2; j < 6; ++j)
				  subtypes[i][j + 5] = instance->targets[j].player;
				break;

			  case CARD_ID_WARDEN_OF_THE_FIRST_TREE:
				{
					if( instance->targets[9].player > 0 ){
						if( instance->targets[9].player & 1 ){
							subtypes[i][9] = SUBTYPE_WARRIOR;
						}
						if( instance->targets[9].player & 2 ){
							subtypes[i][8] = SUBTYPE_SPIRIT;
						}
					}
				}
				break;
			}
		}
	}

  /* Search for matches.  Of particular note is that is_creature_subtype() returns false for 0, which we've used to indicate "unset" above.  It returns true for
   * SUBTYPE_NONE and SUBTYPE_ALL_CREATURES, which would normally be problematic; but since those are only added via force_a_subtype() or add_a_subtype(), never
   * natively in card_ptr_t::types[], we'll either have already returned by now (for SUBTYPE_NONE), or changeling[] will be true (for SUBTYPE_ALL_CREATURES). */

  if (changeling[0])	// If {p1,c1} has any creature types at all, then it's a match.
	{
	  if (changeling[1])
		return 1;

	  for (j = 0; j < NUM_SUBTYPES; ++j)
		if (is_creature_type(subtypes[1][j]))
		  return 1;
	}
  else if (changeling[1])	// If {p0,c0} has any creature types at all, then it's a match.
	{
	  for (j = 0; j < NUM_SUBTYPES; ++j)
		if (is_creature_type(subtypes[0][j]))
		  return 1;
	}
  else
	{
	  // Have to compare one by one.
	  for (i = 0; i < NUM_SUBTYPES; ++i)
		if (is_creature_type(subtypes[0][i]))
		  for (j = 0; j < NUM_SUBTYPES; ++j)
			if (subtypes[0][i] == subtypes[1][j])
			  return 1;
	}

  return 0;
#undef NUM_SUBTYPES
}

// Finds creature subtypes only (by design)
static int get_most_common_subtype_in_zone(const int *zone){
	if( zone == NULL ){
		return -1;
	}
	int subs2[SUBTYPE_MAX_USED_CREATURE_SUBTYPE + 1] = {0};
	int i, count;
	for (count = 0; zone[count] != -1; ++count){
			if( is_what(-1, zone[count], TYPE_CREATURE) ){
				int id = cards_data[zone[count]].id;
				if( !is_changeling(-1, zone[count]) ){
					card_ptr_t* c = cards_ptr[id];
					for (i = 1; i < 6; ++i){	// [0] is always either a type or supertype; [6] is unused
						// deliberately omit SUBTYPE_NONE and SUBTYPE_ALL_CREATURES
						if (c->types[i] >= SUBTYPE_MIN_CREATURE_SUBTYPE && c->types[i] <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE){
							subs2[c->types[i]]++;
						}
					}
				}
			}
	}
	int par = 0, mcsid = -1;
	for (i = 0; i <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE; i++){
		if( subs2[i] > par ){
			par = subs2[i];
			mcsid = i;
		}
	}
	return mcsid;
}

static int get_most_common_subtype_in_deck(int t_player){
	return get_most_common_subtype_in_zone(deck_ptr[t_player]);
}

static int select_a_subtype_impl(int player, int card, int who_chooses, int t_player, const int *zone, int show_legacy){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allow_cancel = player == HUMAN;
	td.who_chooses = who_chooses;
	td.preferred_controller = t_player;
	td.zone = TARGET_ZONE_IN_PLAY | TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

 restart:;
	int result = -1;
	int choice = 0;
	int sc = 0;
	char b1[500];
	int p1 = snprintf(b1, 500, " Auto\n Choose from a card list\n");
	if( can_target(&td)  ){
		choice = 2;
		p1+=snprintf(b1+p1, 500-p1, " Choose from a creature");
	}
	if( who_chooses != AI ){
		choice = do_dialog(who_chooses, player, card, -1, -1, b1, 2);
	}

	int csvid = -1;
	int subs[6] = {-1, -1, -1, -1, -1, -1};
	if( choice == 0 ){
		if( zone == NULL ){
			zone = deck_ptr[t_player];
			result = get_most_common_subtype_in_zone(zone);
			if( result < 1 ){
				zone = get_grave(t_player);
				result = get_most_common_subtype_in_zone(zone);
			}
			if( result < 1 ){
				zone = rfg_ptr[t_player];
				result = get_most_common_subtype_in_zone(zone);
			}
			if( result == -1 ){
				result = internal_rand(SUBTYPE_MAX_USED_CREATURE_SUBTYPE - SUBTYPE_MIN_CREATURE_SUBTYPE) + SUBTYPE_MIN_CREATURE_SUBTYPE;
			}
		}
		else{
			result = get_most_common_subtype_in_zone(zone);
		}
	}
	else if( choice == 1 ){
			csvid = card_from_list(player, 3, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if(choice == 2 ){
			instance->number_of_targets = 0;
			if (!pick_target(&td, "TARGET_CREATURE")){
				goto restart;
			}
			int forced = get_forced_subtype(instance->targets[0].player, instance->targets[0].card);
			if( forced > 0 ){
				int added = get_added_subtype(instance->targets[0].player, instance->targets[0].card);
				if( added > 0 && added != forced ){
					subs[sc++] = added;
					subs[sc++] = forced;
				} else {
					result = get_forced_subtype(instance->targets[0].player, instance->targets[0].card);
				}
			}
			else if( is_changeling(instance->targets[0].player, instance->targets[0].card) ){
				result = get_most_common_subtype_in_deck(t_player);
			}
			else{
				card_instance_t *chosen = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				csvid = get_id(instance->targets[0].player, instance->targets[0].card);
				int added = get_added_subtype(instance->targets[0].player, instance->targets[0].card);
				if( added > 0 ){
					subs[sc] = added;
					sc++;
				}
				if( count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_GOLD) &&
					check_battlefield_for_id(ANYBODY, CARD_ID_AURIFICATION)
					){
					subs[sc] = SUBTYPE_WALL;
					sc++;
				}
				if( csvid == CARD_ID_FIGURE_OF_DESTINY ){
					if( chosen->targets[9].player > 0 ){
						if( chosen->targets[9].player & 1 ){
							subs[sc] = SUBTYPE_SPIRIT;
							sc++;
						}
						if( chosen->targets[9].player & 2 ){
							subs[sc] = SUBTYPE_WARRIOR;
							sc++;
						}
						if( chosen->targets[9].player == 3 ){
							subs[sc] = SUBTYPE_AVATAR;
							sc++;
						}
					}
				}
				if( csvid == CARD_ID_AGELESS_SENTINELS ){
					if( chosen->targets[5].player == 66 ){
						subs[sc] = SUBTYPE_BIRD;
						sc++;
						subs[sc] = SUBTYPE_GIANT;
						sc++;
					}
				}
				if( csvid == CARD_ID_DUPLICANT ){
					int j = 2;
					while( sc < 6 && j < 6 ){
						subs[sc] = chosen->targets[j].player;
						sc++;
						j++;
					}
				}
			}
	}
	if( result == -1 ){
		if( csvid < 0 ){
			csvid = CARD_ID_AIR_ELEMENTAL;
		}
		card_ptr_t* c = cards_ptr[ csvid ];
		int i;
		for (i = 1; i < 6 && sc < 6; ++i){	// [0] is always either a type or supertype; [6] is unused
			if( is_creature_type(c->types[i]) ){
				subs[sc] = c->types[i];
				sc++;
			}
		}

#define OPTION(x)	(subs[x] > 0 && subs[x] <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE) ? raw_get_subtype_text(subs[x]) : "",	\
					(subs[x] > 0 && subs[x] <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE), 1

		int choice2 = DIALOG(player, card, EVENT_ACTIVATE, DLG_WHO_CHOOSES(who_chooses), DLG_NO_STORAGE, DLG_OMIT_ILLEGAL, DLG_NO_CANCEL,
							 OPTION(0),
							 OPTION(1),
							 OPTION(2),
							 OPTION(3),
							 OPTION(4),
							 OPTION(5));
#undef OPTION

		if (choice2 == 0){
			goto restart;
		}
		else {
			result = subs[choice2 - 1];
		}
	}
	if( result > 0 && result != SUBTYPE_NONE ){
		if( is_what(player, card, TYPE_PERMANENT) && show_legacy ){
			int legacy = create_subtype_name_legacy(player, card, result);
			if( show_legacy == 2 ){
				get_card_instance(player, legacy)->targets[0].card = 2;
			}
		}
	}

	return result;
}

int select_a_subtype(int player, int card){

	int t_player = player;
	if( get_id(player, card) == CARD_ID_ENGINEERED_PLAGUE || get_id(player, card) == CARD_ID_TSABOS_DECREE ){
		if( player == AI ){
			t_player = 1-player;
		}
	}

	return select_a_subtype_impl(player, card, player, t_player, NULL, 1);
}

int select_a_subtype_full_choice(int player, int card, int t_player, const int *zone, int show_legacy){
	return select_a_subtype_impl(player, card, t_player, t_player, zone, show_legacy);
}

void protection_from_subtype(int player, int card, event_t event, int subt){

	if( event == EVENT_BLOCK_LEGALITY &&
	    player == attacking_card_controller && card == attacking_card &&
	    has_creature_type(affected_card_controller, affected_card, subt)
	  ){
		event_result = 1;
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( source->internal_card_id == damage_card && source->info_slot > 0 &&
			source->damage_target_player == player && source->damage_target_card == card &&
			in_play(source->damage_source_player, source->damage_source_card) &&
			has_subtype(source->damage_source_player, source->damage_source_card, subt)
		  ){
			source->info_slot = 0;
		}
	}
}

int target_player_sacrifices_a_subtype(int player, int card, int req_type, int req_subtype, int cannot_cancel){
	return sacrifice(player, card, player, cannot_cancel, req_type, 0, req_subtype, 0, 0, 0, 0, 0, -1, 0);
}

int count_untapped_nonsick_subtype(int what_player, int type, int subtype){
  // what_player = 2 -> count the permanents of both players
  // subtype = -1 -> count any nontapped, nonsick permanent of chosen type

  int result = 0;

  if( what_player == 2 ){
	 int i;
	 for(i = 0; i < 2; i++){
		 int count = active_cards_count[i]-1;
		 while( count > -1 ){
			   if( in_play(i, count) && is_what(i, count, type) &&  ! is_tapped(i, count) && ! is_animated_and_sick(i, count) ){
				   if( subtype > 0 ){
					   if( has_subtype(i, count, subtype) ){
						   result++;
					   }
				   }
				   else{
						result++;
				   }

			   }
			   count--;
		 }
	 }
  }

  else if ( what_player == 1 || what_player == 0){
		   int count = active_cards_count[what_player]-1;
		   while( count > -1 ){
				 if( in_play(what_player, count) && is_what(what_player, count, type) &&  ! is_tapped(what_player, count) &&
					 ! is_animated_and_sick(what_player, count)
				  ){
					if( subtype > 0 ){
						if( has_subtype(what_player, count, subtype) ){
							result++;
						}
					}
					else{
						 result++;
					}
				 }
				 count--;
		   }
  }

  return result;
}

int check_for_untapped_nonsick_subtype(int what_player, int type, int subtype){
  // what_player = 2 -> check the permanents of both players
  // subtype = -1 -> check if there's a nontapped, nonsick permanent of chosen type


  if( what_player == 2 ){
	 int i;
	 for(i = 0; i < 2; i++){
		 int count = active_cards_count[i]-1;
		 while( count > -1 ){
			   if( in_play(i, count) && is_what(i, count, type) &&  ! is_tapped(i, count) && ! is_animated_and_sick(i, count) ){
				   if( subtype > 0 ){
					   if( has_subtype(i, count, subtype) ){
						   return 1;
					   }
				   }
				   else{
						return 1;
				   }

			   }
			   count--;
		 }
	 }
  }

  else if ( what_player == 1 || what_player == 0){
		   int count = active_cards_count[what_player]-1;
		   while( count > -1 ){
				 if( in_play(what_player, count) && is_what(what_player, count, type) &&  ! is_tapped(what_player, count) &&
					 ! is_animated_and_sick(what_player, count)
				  ){
					if( subtype > 0 ){
						if( has_subtype(what_player, count, subtype) ){
							return 1;
						}
					}
					else{
						 return 1;
					}
				 }
				 count--;
		   }
  }

  return 0;
}

int count_untapped_subtype(int what_player, int type, int subtype){
  // what_player = 2 -> count the permanents of both players
  // subtype = -1 -> count any nontapped permanent of chosen type

  int result = 0;

  if( what_player == 2 ){
	 int i;
	 for(i = 0; i < 2; i++){
		 int count = active_cards_count[i]-1;
		 while( count > -1 ){
			   if( in_play(i, count) && is_what(i, count, type) && ! is_tapped(i, count)
				   && (subtype <= 0 || has_subtype(i, count, subtype)) ){
				   result++;
			   }
			   count--;
		 }
	 }
  }

  else if ( what_player == 1 || what_player == 0){
		   int count = active_cards_count[what_player]-1;
		   while( count > -1 ){
				 if( in_play(what_player, count) && is_what(what_player, count, type) && ! is_tapped(what_player, count)
					 && (subtype <= 0 || has_subtype(what_player, count, subtype)) ){
					 result++;
				 }
				 count--;
		   }
  }

  return result;
}

int is_subtype_in_hand(int player, int subtype){

  int i;

  for(i=0; i < active_cards_count[player]; i++){
	  if( in_hand(player, i) && has_subtype(player, i, subtype) ){
		 return 1;
	  }
  }

  return 0;
}

int is_subtype_dead(int player, int subtype1, int subtype2, int mode){
		int p;
		for(p=0;p<2;p++){
			if( p == player || player == 2 ){
				int c=0;
				for(c=0;c< active_cards_count[p];c++){
					card_data_t* card_d = get_card_data(p, c);
					if((card_d->type & TYPE_PERMANENT) && in_play(p, c) ){
						int good = 0;
						if( subtype1 > 0 && has_subtype(p, c, subtype1) ){
							good = 1 ;
						}
						if( subtype2 > 0 && has_subtype(p, c, subtype2) ){
							good = 1 ;
						}
						if( subtype2 == -1 && subtype1 == -1 ){
							good = 1 ;
						}
						if( good == 1 ){
							card_instance_t *creature = get_card_instance(p, c);
							if( creature->kill_code > 0 ){
								if( mode == 0 && creature->kill_code == KILL_DESTROY ){
									return 0x63;
								}
								if( mode == 1 && creature->kill_code < KILL_SACRIFICE ){
									return 0x63;
								}
							}
						}
					}
				}
			}
		}
		return 0;
}

int subtype_deals_damage(int player, int card, event_t event, int controller, int subtype, int mode){
	// subtype = -1 --> Any creature

	card_instance_t *instance = get_card_instance(player, card);

	ASSERT(!(mode & DDBM_REPORT_DAMAGE_DEALT) && "unimplemented for subtype_deals_damage");
	ASSERT(!(mode & DDBM_TRACE_DAMAGED_PLAYERS) && "unimplemented for subtype_deals_damage");
	ASSERT(!(mode & DDBM_STORE_IN_TARGETS_9) && "unimplemented for subtype_deals_damage");

	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card
			&& (subtype == -1 || has_subtype(damage->damage_source_player, damage->damage_source_card, subtype))){
			int good = 0;
			if( damage->damage_source_player == controller || controller == 2 ){
				if( damage->info_slot > 0 ){
					good = 1;
				}
				else{
					card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( trg->targets[16].player > 0 ){
						good = 1;
					}
				}
			}

			if (mode & (DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_DAMAGE_PLANESWALKER | DDBM_MUST_DAMAGE_CREATURE)){
				if (damage_is_to_planeswalker(damage)){
					if (!(mode & DDBM_MUST_DAMAGE_PLANESWALKER)){
						good = 0;
					}
				} else if (damage->damage_target_card != -1){
					if (!(mode & DDBM_MUST_DAMAGE_CREATURE)){
						good = 0;
					}
				} else {
					if (!(mode & DDBM_MUST_DAMAGE_PLAYER)){
						good = 0;
					}
				}
			}

			if ((mode & DDBM_MUST_DAMAGE_OPPONENT)
				&& (damage->damage_target_card != -1 || damage->damage_target_player != 1-player || damage_is_to_planeswalker(damage))){
				good = 0;
			}
			if ((mode & DDBM_MUST_BE_COMBAT_DAMAGE)
				&& !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))){
				good = 0;
			}
			if ((mode & DDBM_NOT_ME)
				&& damage->damage_source_player == player && damage->damage_source_card == card){
				good = 0;
			}
			if (good == 1){
				if (instance->targets[1].player < 2){
					instance->targets[1].player = 2;
				}
				if (mode & DDBM_TRACE_DAMAGED_CREATURES){
					int pos = instance->targets[1].player;
					if (in_play(damage->damage_target_player, damage->damage_target_card) && pos < 10){
						instance->targets[pos].player = damage->damage_target_player;
						instance->targets[pos].card = damage->damage_target_card;
					}
				}
				instance->targets[1].player++;
			}
		}
	}

	if( instance->targets[1].player > 2 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if(event == EVENT_TRIGGER){
			int trg = 2;
			if( mode & DDBM_TRIGGER_OPTIONAL ){
				trg = 1+player;
			}
			event_result |= trg;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			instance->targets[1].card = instance->targets[1].player-2;
			instance->targets[1].player = 2;
			return 1;
		} else if (event == EVENT_END_TRIGGER){
			instance->targets[1].player = 2;
		}
	}

	return 0;
}

/* These all rely on SUBTYPE_MIN_* all being even multiples of 0x1000 (except SUBTYPE_MIN_USED_CREATURE_SUBTYPE), and SUBTYPE_MAX_USED_* all being
 * SUBTYPE_MIN_* + 299 or less.  (load_text() can't load more entries than that.) */
static char* strs_creature_type[		(SUBTYPE_MAX_USED_CREATURE_SUBTYPE			& 0xFFF) + 1] = {0};
static char* strs_type_type[			(SUBTYPE_MAX_USED_TYPE						& 0xFFF) + 1] = {0};
static char* strs_supertype_type[		(SUBTYPE_MAX_USED_SUPERTYPE					& 0xFFF) + 1] = {0};
static char* strs_artifact_type[		(SUBTYPE_MAX_USED_ARTIFACT_SUBTYPE			& 0xFFF) + 1] = {0};
static char* strs_enchantment_type[		(SUBTYPE_MAX_USED_ENCHANTMENT_SUBTYPE		& 0xFFF) + 1] = {0};
static char* strs_land_type[			(SUBTYPE_MAX_USED_LAND_SUBTYPE				& 0xFFF) + 1] = {0};
static char* strs_instant_sorcery_type[	(SUBTYPE_MAX_USED_INSTANT_SORCERY_SUBTYPE	& 0xFFF) + 1] = {0};
static char* strs_planeswalker_type[	(SUBTYPE_MAX_USED_PLANESWALKER_SUBTYPE		& 0xFFF) + 1] = {0};
static char* strs_plane_type[			(SUBTYPE_MAX_USED_PLANE_SUBTYPE				& 0xFFF) + 1] = {0};

void init_subtype_text(void)
{
  if (strs_creature_type[0])
	return;

  int i;

  load_text(0, "CREATURE_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_CREATURE_SUBTYPE & 0xFFF); ++i)
	strs_creature_type[i] = strdup(text_lines[i]);
#if NEED_MORE_THAN_300_CREATURE_TYPES
  // Change the limit above from SUBTYPE_MAX_USED_CREATURE_SUBTYPE & 0xFFF to 299, and:
  load_text(0, "CREATURE_TYPE2");
  for (i = 0; i <= (SUBTYPE_MAX_USED_CREATURE_SUBTYPE - 300) & 0xFFF; ++i)
	strs_creature_type[300 + i] = strdup(text_lines[i]);
#endif

  load_text(0, "TYPE_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_TYPE & 0xFFF); ++i)
	strs_type_type[i] = strdup(text_lines[i]);

  load_text(0, "SUPERTYPE_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_SUPERTYPE & 0xFFF); ++i)
	strs_supertype_type[i] = strdup(text_lines[i]);

  load_text(0, "ARTIFACT_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_ARTIFACT_SUBTYPE & 0xFFF); ++i)
	strs_artifact_type[i] = strdup(text_lines[i]);

  load_text(0, "ENCHANTMENT_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_ENCHANTMENT_SUBTYPE & 0xFFF); ++i)
	strs_enchantment_type[i] = strdup(text_lines[i]);

  load_text(0, "LAND_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_LAND_SUBTYPE & 0xFFF); ++i)
	strs_land_type[i] = strdup(text_lines[i]);

  load_text(0, "INSTANT_SORCERY_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_INSTANT_SORCERY_SUBTYPE & 0xFFF); ++i)
	strs_instant_sorcery_type[i] = strdup(text_lines[i]);

  load_text(0, "PLANESWALKER_SUBTYPE_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_PLANESWALKER_SUBTYPE & 0xFFF); ++i)
	strs_planeswalker_type[i] = strdup(text_lines[i]);

  load_text(0, "PLANE_SUBTYPE_TYPE");
  for (i = 0; i <= (SUBTYPE_MAX_USED_PLANE_SUBTYPE & 0xFFF); ++i)
	strs_plane_type[i] = strdup(text_lines[i]);
}

// Returns text corresponding to subtype.  Safely usable in display functions and targeting, since it doesn't call load_text().
const char* raw_get_subtype_text(subtype_t subtype)
{
  // Makes the same assumptions as init_subtype_text() above.

  switch (subtype & 0xF000)
	{
	  case 0:	// Creature subtypes - note SUBTYPE_MIN_CREATURE_SUBTYPE is 1, not 0, unlike the other MINs
		if (subtype <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE)
		  return strs_creature_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_TYPE:
		if (subtype <= SUBTYPE_MAX_USED_TYPE)
		  return strs_type_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_SUPERTYPE:
		if (subtype <= SUBTYPE_MAX_USED_SUPERTYPE)
		  return strs_supertype_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_ARTIFACT_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_ARTIFACT_SUBTYPE)
		  return strs_artifact_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_ENCHANTMENT_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_ENCHANTMENT_SUBTYPE)
		  return strs_enchantment_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_LAND_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_LAND_SUBTYPE)
		  return strs_land_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_INSTANT_SORCERY_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_INSTANT_SORCERY_SUBTYPE)
		  return strs_instant_sorcery_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_PLANESWALKER_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_PLANESWALKER_SUBTYPE)
		  return strs_planeswalker_type[subtype & 0xFFF];
		break;

	  case SUBTYPE_MIN_PLANE_SUBTYPE:
		if (subtype <= SUBTYPE_MAX_USED_PLANE_SUBTYPE)
		  return strs_plane_type[subtype & 0xFFF];
		break;
	}

  return strs_creature_type[0];	// "None"
}

/* Any occurrence of %s or %a in fmt will be replaced by raw_get_subtype_text(subtype).  %a will also insert a "a " or "an " before the subtype's text as
 * appropriate.  Returns a pointer to a static buffer. */
const char* get_subtype_text(const char* fmt, subtype_t subtype)
{
  static char buf[500];
  if (ai_is_speculating == 1)
	{
	  *buf = 0;
	  return buf;
	}

  const char* raw_txt = NULL;
  int raw_len_minus_1 = 0;

  const char* p = fmt;
  char* q = buf;
  for (; (*q = *p); ++p, ++q)
	if (*p == '%' && (*(p + 1) == 's' || *(p + 1) == 'a'))
	  {
		if (!raw_txt)
		  {
			raw_txt = raw_get_subtype_text(subtype);
			raw_len_minus_1 = strlen(raw_txt) - 1;
		  }

		++p;
		if (*p == 'a')
		  {
			*q++ = 'a';

			if (strchr("aeiouAEIOU", raw_txt[0]))
			  *q++ = 'n';

			*q++ = ' ';
		  }

		strcpy(q, raw_txt);
		q += raw_len_minus_1;
	  }

  return buf;
}

static int
fx_change_types(int player, int card, event_t event)
{
  /* Local data:
   * targets[4].player: keywords
   * targets[4].card: sp_keywords
   * targets[5].player: base power
   * targets[5].card: base toughness
   * targets[6].player: types to add
   * targets[6].card: types to remove (done before adding types)
   * targets[7].player: last-seen iid
   * targets[7].card: initial color, if non-zero
   * dummy3: iid to change type to
   */

  card_instance_t *inst = get_card_instance(player, card);
  card_instance_t *ench;

  if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card))
    {
      int was_attached_to_player = inst->damage_target_player, was_attached_to_card = inst->damage_target_card;
	  inst->damage_target_player = inst->damage_target_card = -1;
      if (was_attached_to_card != -1)
		{
		  get_card_instance(was_attached_to_player, was_attached_to_card)->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
		  get_abilities(was_attached_to_player, was_attached_to_card, EVENT_CHANGE_TYPE, -1);
		}
	  return 0;
    }

  if ((event == EVENT_EFFECT_CREATED
       && inst->targets[6].player != -1	// dispatched too soon?
	   && (ench = get_card_instance(inst->damage_target_player, inst->damage_target_card)))
      || (event == EVENT_CHANGE_TYPE && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
		  && affected_card != -1 && affect_me(inst->damage_target_player, inst->damage_target_card)
		  && (ench = in_play(affected_card_controller, affected_card))))
    {
      int change_to, iid = (event == EVENT_CHANGE_TYPE ? event_result
							: ench->internal_card_id >= 0 ? ench->internal_card_id
							: ench->backup_internal_card_id);

      type_t cur_typ = cards_data[iid].type;
      type_t new_typ = cur_typ;
      new_typ &= ~inst->targets[6].card;
      new_typ |= inst->targets[6].player;
      if (cur_typ == new_typ	// already has the right types
		  && (cards_data[iid].static_ability & inst->targets[4].player) == (unsigned)inst->targets[4].player	// has all keywords that would be granted
		  && !inst->targets[5].player	// not granting sp_keywords
		  && (!(new_typ & TYPE_CREATURE)	// either not turning into a creature,
			  || (inst->targets[5].player == -1 && inst->targets[5].card == -1)	// or not setting power or toughness,
			  || (cards_data[iid].power == inst->targets[5].player && cards_data[iid].toughness == inst->targets[5].card)))	// or already has right p/t
		return 0;

      if (iid == inst->targets[7].player
		  && cards_data[inst->dummy3].type == new_typ)
		change_to = inst->dummy3;
      else
		{
		  change_to = create_a_card_type(iid);
		  if (change_to == -1)
			return 0;

		  card_data_t* cd = &cards_data[change_to];
		  cd->type = new_typ;

		  if ((new_typ & TYPE_CREATURE) && !(inst->targets[5].player == -1 && inst->targets[5].card == -1))
			{
			  cd->power = inst->targets[5].player;
			  cd->toughness = inst->targets[5].card;
			}

		  cd->static_ability |= inst->targets[4].player;

		  inst->targets[7].player = iid;
		  inst->dummy3 = change_to;
		  ench->regen_status |= KEYWORD_RECALC_ALL & ~KEYWORD_RECALC_CHANGE_TYPE;
		}

      if (event == EVENT_CHANGE_TYPE)
		event_result = change_to;
      else
		{
		  ench->internal_card_id = change_to;
		  ench->regen_status |= KEYWORD_RECALC_ALL;
		}

	  if (inst->damage_source_card == inst->damage_target_card
		  && inst->damage_source_player == inst->damage_target_player
		  && (inst->targets[6].player & TYPE_CREATURE))
		ench->token_status |= STATUS_LEGACY_TYPECHANGE;
    }

  // Nutty that this has to be called for events besides EVENT_ABILITIES.
  if (inst->damage_target_card != -1 && inst->targets[4].card)
	special_abilities(inst->damage_target_player, inst->damage_target_card, event, inst->targets[4].card, player, card);

  if (event == EVENT_SET_COLOR && affect_me(inst->damage_target_player, inst->damage_target_card)
	  && inst->targets[7].card > 0)
	event_result = inst->targets[7].card & COLOR_TEST_ANY_COLORED;

  if (event == EVENT_CLEANUP
	  && !(inst->token_status & STATUS_PERMANENT))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

static int
add_or_remove_types_legacy_impl(int src_player, int src_card,
								int tgt_player, int tgt_card,
								type_t types_to_remove, type_t types_to_add,
								int pow, int tgh,
								keyword_t kws_to_add, sp_keyword_t sp_kws_to_add,
								color_test_t color, int permanent)
{
  card_instance_t* inst = get_card_instance(tgt_player, tgt_card);
  if (inst->internal_card_id == activation_card)
	{
	  tgt_player = inst->parent_controller;
	  tgt_card = inst->parent_card;
	}

  if (!in_play(tgt_player, tgt_card))
	return -1;

  int leg_card = create_targetted_legacy_effect(src_player, src_card, fx_change_types, tgt_player, tgt_card);
  card_instance_t* legacy = get_card_instance(src_player, leg_card);
  legacy->targets[4].player = kws_to_add;
  legacy->targets[4].card = sp_kws_to_add;
  legacy->targets[5].player = pow;
  legacy->targets[5].card = tgh;
  legacy->targets[6].player = types_to_add;
  legacy->targets[6].card = types_to_remove;
  legacy->targets[7].player = -1;
  legacy->targets[7].card = color;
  if (permanent)
    legacy->token_status |= STATUS_PERMANENT;
  play_sound_effect(WAV_CHANGET);
  call_card_function_i(legacy, src_player, leg_card, EVENT_EFFECT_CREATED);

  return leg_card;
}

/* Animates {player,card} by adding TYPE_CREATURE, with characteristic power, toughness, and keywords; continuously-added sp_keywords; and
 * continuously-overriden color (unless color is 0).  Lasts until end of turn unless permanent is nonzero.  The animated card will have token_status &
 * STATUS_LEGACY_TYPECHANGE set while animated by this effect.  Returns a legacy effect index, but you shouldn't do anything with it except maybe give it
 * alternate text. */
int
animate_self(int player, int card, int pow, int tgh, keyword_t kws_to_add, sp_keyword_t sp_kws_to_add, color_test_t color, int permanent)
{
  return add_or_remove_types_legacy_impl(player, card, player, card, TYPE_NONE, TYPE_CREATURE, pow, tgh, kws_to_add, sp_kws_to_add,
										 get_sleighted_color_test(player, card, color), permanent);
}

// As per animate_self(), but works on a different card than the effect source and doesn't set STATUS_LEGACY_TYPECHANGE.
int
animate_other(int src_player, int src_card, int tgt_player, int tgt_card, int pow, int tgh, keyword_t kws_to_add, sp_keyword_t sp_kws_to_add, color_test_t color, int permanent)
{
  return add_or_remove_types_legacy_impl(src_player, src_card, tgt_player, tgt_card, TYPE_NONE, TYPE_CREATURE, pow, tgh, kws_to_add, sp_kws_to_add,
										 get_sleighted_color_test(src_player, src_card, color), permanent);
}

// {player,card} has an additional subtype subt if it animated itself via animate_self().
void
has_subtype_if_animated_self(int player, int card, event_t event, subtype_t subt)
{
  if (event == EVENT_SUBTYPE && (unsigned)event_result == subt)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->token_status & STATUS_LEGACY_TYPECHANGE)
		event_result = 0;
	}
}

// {player,card} has two additional subtypes subt1 and subt2 if it animated itself via animate_self().
void
has_subtypes_if_animated_self(int player, int card, event_t event, subtype_t subt1, subtype_t subt2)
{
  if (event == EVENT_SUBTYPE && ((unsigned)event_result == subt1 || (unsigned)event_result == subt2))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->token_status & STATUS_LEGACY_TYPECHANGE)
		event_result = 0;
	}
}
