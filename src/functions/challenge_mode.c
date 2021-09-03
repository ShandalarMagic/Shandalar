#include "manalink.h"

int challenge_round = 0;
int unlocked[50];
int challenge1 = -1;
int challenge2 = -1;
int challenge3 = -1;
int challenge4 = -1;

char challenge_names[][50] = {
	// 1
	"", "Tribal Trials", "Vanguard Vengeance", "Chipping Away", "Onslaught", "Shandalar 2.0", "BANNED!", "Planeswalker Decimation", "Magic: The Puzzling",
	"That's Not My Deck!",
	// 2
	"Super Mulligan", "What Hand?", "That's Big...", "Epic", "There can be only one", "March of the Machines", "Land Ho!", "Dude...", "Face the Hydra", "",
	// 3
	"Big Dumb Object", "Enchanted Evening", "Master Warlock", "Urza Wannabe", "Instant Gratification", "Gimme!", "March of the Machines", "Easy as 1, 2, 3...",
	"The Perfect Storm", "Sliver Nation",
	// 4
	"Chaos Orb", "Pit Fighting", "Life's a Lich", "Cascade", "ABC's", "Challenge Challenge", "Enchanted", "Budget deck for ya", "Archenemy's Cohort", "Back to '95"
};

char unlock_codes[][10] = {
	"", "magic", "mtg", "jabber", "begin", "goodwork", "master", "quitter", "nuts", "hardy",
	"routish", "headache", "buuurp", "bondage", "annoying", "wired", "shiny", "keeks", "fangs", "",
	"diane", "james", "tricia", "greg", "christine", "me", "ryan", "owen", "dylan", "katie",
	"onna", "monna", "poeia", "clean", "patch", "oath", "nerds", "willy", "wonka", "roskopp"
};

static int lock_ids[] = {
	// 1
	-1,
	CARD_ID_MYSTIC_ENFORCER,
	CARD_ID_CITY_OF_TRAITORS,
	CARD_ID_MYSTIC_REMORA,
	CARD_ID_QUIRION_DRYAD,
	CARD_ID_GOBLIN_WELDER,
	CARD_ID_CURSED_SCROLL,
	CARD_ID_SERUM_POWDER,
	CARD_ID_CONUNDRUM_SPHINX,
	CARD_ID_MIRARIS_WAKE,
	//2
	CARD_ID_ROUT,
	CARD_ID_BRAIN_FREEZE,
	CARD_ID_GOBLIN_CHARBELCHER,
	CARD_ID_STANDSTILL,
	CARD_ID_STREET_WRAITH,
	CARD_ID_TANGLE_WIRE,
	CARD_ID_GEMSTONE_MINE,
	CARD_ID_KIKI_JIKI_MIRROR_BREAKER,
	-2, // Face the Hydra --> The Slayer.
	-1,
	// 3
	CARD_ID_KARN_SILVER_GOLEM,
	CARD_ID_ILLUSIONS_OF_GRANDEUR,
	CARD_ID_DONATE,
	CARD_ID_TEZZERET_THE_SEEKER,
	CARD_ID_MIGHT_OF_ALARA,
	CARD_ID_SOWER_OF_TEMPTATION,
	CARD_ID_UMEZAWAS_JITTE,
	CARD_ID_BLOODBRAID_ELF,
	CARD_ID_MINDS_DESIRE,
	CARD_ID_SLIVER_OVERLORD,
	// 4
	CARD_ID_VIOLENT_ULTIMATUM,
	-1, // Pit Fighting
	CARD_ID_PHYREXIAN_NEGATOR,
	CARD_ID_BITUMINOUS_BLAST,
	CARD_ID_LEYLINE_OF_THE_VOID,
	CARD_ID_HELM_OF_OBEDIENCE,
	-1, // Enchanted
	CARD_ID_LEATHERBACK_BALOTH,
	-1, // Archenemy's Cohort
	-1, // Back to '95
	// 5
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

typedef enum
{
	// Tribal decks
	CD_KOBOLDS	= -11,
	CD_ZOMBIES	= -12,
	CD_FAERIES	= -13,
	CD_KITHKIN	= -14,
	CD_MERFOLKS	= -15,
	CD_RATS		= -16, // 30 x Relentless Rats
	CD_ELVES	= -17,
	CD_GOBLINS	= -18,
	CD_SOLDIERS	= -19,
	CD_SLIVERS	= -20, // Counter-Slivers

	CD_GROW				= -21,
	CD_SHARUUM			= -22, // Colored artifacts
	CD_SPAWNWRITHER		= -23,
	CD_TURBOFOG			= -24,
	CD_GOLD_CREATURES	= -25,
	CD_BURN				= -26,
	CD_2_LAND_WHITE		= -27,
	CD_NO_LAND			= -28,
	CD_WORKSHOP			= -29,
	CD_CLONE_HUMAN_DECK	= -30,
	CD_FAST_SLIVERS		= -31,
	CD_VAMPIRES			= -32,
	CD_VISELING			= -33,
	CD_BIG_BURN			= -34,
	CD_AWWFFINITY		= -35,
	CD_YGGDRASIL_GB		= -36,
	CD_NYKOTORIAN_ZOO	= -37,
	CD_SPLICERS			= -38,
	CD_FAST_METALCRAFT	= -39,
	CD_PROLIFERATE		= -40,
	CD_MAVERICK			= -41,
	CD_CAW_BLADE		= -42,
	CD_REC_SUR			= -43,

	// Archenemy's Minions
	CD_ANGUS_MC_KENZIE	= -44,
	CD_XIRA_ARIEN		= -45,
	CD_TUKNIR_DEATHLOK	= -46,
	CD_ANIMAR			= -47,
	CD_EDRIC			= -48,
	CD_DORAN			= -49,
	CD_KAALIA			= -50,

	// special decks, not randomly choosable
	CD_MIN_SPECIAL_DECK	= -97,
	CD_ALL_KOBOLDS		= -97,
	CD_ALL_STRIP_MINE	= -98,
	CD_ALL_MOUNTAINS	= -99,
	CD_HYDRA			= -100,
} challenge_decks_t;

static int cd_list[] = {
	// kobolds
	CD_KOBOLDS,
	CARD_ID_KOBOLD_DRILL_SERGEANT, 4,
	CARD_ID_KOBOLD_OVERLORD, 4,
	CARD_ID_KOBOLD_TASKMASTER, 4,
	CARD_ID_KOBOLDS_OF_KHER_KEEP, 4,
	CARD_ID_CRIMSON_KOBOLDS, 4,
	CARD_ID_CROOKSHANK_KOBOLDS, 4,
	CARD_ID_KELDON_WARLORD, 3,
	CARD_ID_GAUNTLET_OF_MIGHT, 3,
	CARD_ID_HOWLING_MINE, 4,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_MOUNTAIN, 22,
	// zombies
	CD_ZOMBIES,
	CARD_ID_CARNOPHAGE, 4,
	CARD_ID_SARCOMANCY, 4,
	CARD_ID_BAD_MOON, 4,
	CARD_ID_UNDEAD_WARCHIEF, 4,
	CARD_ID_LORD_OF_THE_UNDEAD, 4,
	CARD_ID_CEMETERY_REAPER, 4,
	CARD_ID_GRAVEBORN_MUSE, 4,
	CARD_ID_DARK_RITUAL, 4,
	CARD_ID_SKINRENDER, 4,
	CARD_ID_SWAMP, 24,
	// faeries
	CD_FAERIES,
	CARD_ID_BITTERBLOSSOM, 4,
	CARD_ID_SCION_OF_OONA, 4,
	CARD_ID_WYDWEN_THE_BITING_GALE, 4,
	CARD_ID_WASP_LANCER, 4,
	CARD_ID_CLOUD_OF_FAERIES, 4,
	CARD_ID_CONTROL_MAGIC, 4,
	CARD_ID_DURESS, 4,
	CARD_ID_CRYPTIC_COMMAND, 4,
	CARD_ID_COUNTERSPELL, 4,
	CARD_ID_UNDERGROUND_SEA, 4,
	CARD_ID_POLLUTED_DELTA, 4,
	CARD_ID_ISLAND, 8,
	CARD_ID_SWAMP, 4,
	CARD_ID_FLOODED_STRAND, 4,
	// kithkin
	CD_KITHKIN,
	CARD_ID_KNIGHT_OF_MEADOWGRAIN, 4,
	CARD_ID_FIGURE_OF_DESTINY, 4,
	CARD_ID_WIZENED_CENN, 4,
	CARD_ID_GOLDMEADOW_HARRIER, 4,
	CARD_ID_GOLDMEADOW_STALWART, 4,
	CARD_ID_CLOUDGOAT_RANGER, 4,
	CARD_ID_HONOR_OF_THE_PURE, 4,
	CARD_ID_CRUSADE, 4,
	CARD_ID_SPECTRAL_PROCESSION, 4,
	CARD_ID_PLAINS, 24,
	// merfolk
	CD_MERFOLKS,
	CARD_ID_LORD_OF_ATLANTIS, 4,
	CARD_ID_COUNTERSPELL, 4,
	CARD_ID_SILVERGILL_ADEPT, 4,
	CARD_ID_WISTFUL_SELKIE, 4,
	CARD_ID_MERROW_REEJEREY, 4,
	CARD_ID_FORCE_OF_WILL, 4,
	CARD_ID_MERFOLK_SOVEREIGN, 4,
	CARD_ID_STANDSTILL, 4,
	CARD_ID_CURSECATCHER, 4,
	CARD_ID_DAZE, 4,
	CARD_ID_ISLAND, 20,
	// rats
	CD_RATS,
	CARD_ID_RELENTLESS_RATS, 30,
	CARD_ID_DARK_RITUAL, 4,
	CARD_ID_CABAL_RITUAL, 4,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_SWAMP, 17,
	// elves
	CD_ELVES,
	CARD_ID_CHAMELEON_COLOSSUS, 4,
	CARD_ID_ELVISH_CHAMPION, 4,
	CARD_ID_ELVISH_ARCHDRUID, 4,
	CARD_ID_REGAL_FORCE, 4,
	CARD_ID_LLANOWAR_ELVES, 4,
	CARD_ID_IMPERIOUS_PERFECT, 4,
	CARD_ID_WRENS_RUN_VANQUISHER, 4,
	CARD_ID_ELADAMRI_LORD_OF_LEAVES, 2,
	CARD_ID_WORSHIP, 2,
	CARD_ID_WINDSWEPT_HEATH, 4,
	CARD_ID_FYNDHORN_ELVES, 4,
	CARD_ID_WOODED_FOOTHILLS, 4,
	CARD_ID_PENDELHAVEN, 1,
	CARD_ID_FOREST, 11,
	CARD_ID_SAVANNAH, 4,
	// goblins
	CD_GOBLINS,
	CARD_ID_GOBLIN_KING, 1,
	CARD_ID_GOBLIN_WARCHIEF, 4,
	CARD_ID_MOGG_FANATIC, 4,
	CARD_ID_SIEGE_GANG_COMMANDER, 3,
	CARD_ID_GOBLIN_LACKEY, 4,
	CARD_ID_GOBLIN_PILEDRIVER, 4,
	CARD_ID_GOBLIN_MATRON, 4,
	CARD_ID_GOBLIN_SHARPSHOOTER, 2,
	CARD_ID_GOBLIN_CHIEFTAIN, 4,
	CARD_ID_GOBLIN_RINGLEADER, 4,
	CARD_ID_RITE_OF_FLAME, 4,
	CARD_ID_MOUNTAIN, 18,
	CARD_ID_WASTELAND, 4,
	// soldiers
	CD_SOLDIERS,
	CARD_ID_CRUSADE, 3,
	CARD_ID_CAPTAIN_OF_THE_WATCH, 2,
	CARD_ID_PREEMINENT_CAPTAIN, 4,
	CARD_ID_FIELD_MARSHAL, 4,
	CARD_ID_VETERAN_ARMORSMITH, 4,
	CARD_ID_VETERAN_ARMORER, 4,
	CARD_ID_ELITE_VANGUARD, 4,
	CARD_ID_VETERAN_SWORDSMITH, 4,
	CARD_ID_HONOR_OF_THE_PURE, 3,
	CARD_ID_AJANI_GOLDMANE, 4,
	CARD_ID_PLAINS, 24,
	// slivers
	CD_SLIVERS,
	CARD_ID_WINGED_SLIVER, 4,
	CARD_ID_MUSCLE_SLIVER, 4,
	CARD_ID_SINEW_SLIVER, 4,
	CARD_ID_CRYSTALLINE_SLIVER, 4,
	CARD_ID_ABSORB, 4,
	CARD_ID_COUNTERSPELL, 4,
	CARD_ID_FORCE_OF_WILL, 4,
	CARD_ID_DAZE, 4,
	CARD_ID_WATCHER_SLIVER, 2,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_ANCESTRAL_RECALL, 1,
	CARD_ID_TIME_WALK, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SAVANNAH, 2,
	CARD_ID_TUNDRA, 4,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_FLOODED_STRAND, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	// grow
	CD_GROW,
	CARD_ID_QUIRION_DRYAD, 4,
	CARD_ID_LORESCALE_COATL, 4,
	CARD_ID_TARMOGOYF, 4,
	CARD_ID_PONDER, 4,
	CARD_ID_SLEIGHT_OF_HAND, 4,
	CARD_ID_FORCE_OF_WILL, 4,
	CARD_ID_DAZE, 4,
	CARD_ID_BRAINSTORM, 4,
	CARD_ID_NIMBLE_MONGOOSE, 4,
	CARD_ID_WEREBEAR, 4,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_ISLAND, 4,
	CARD_ID_FOREST, 4,
	CARD_ID_POLLUTED_DELTA, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	// sharuum
	CD_SHARUUM,
	CARD_ID_MAGISTER_SPHINX, 4,
	CARD_ID_SHARUUM_THE_HEGEMON, 4,
	CARD_ID_GRAND_ARBITER_AUGUSTIN_IV, 4,
	CARD_ID_ETHERIUM_SCULPTOR, 4,
	CARD_ID_LODESTONE_GOLEM, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_MOAT, 4,
	CARD_ID_CAREFUL_STUDY, 4,
	CARD_ID_CHROME_MOX, 4,
	CARD_ID_INKWELL_LEVIATHAN, 2,
	CARD_ID_UNDERGROUND_SEA, 4,
	CARD_ID_TUNDRA, 4,
	CARD_ID_SCRUBLAND, 4,
	CARD_ID_REFLECTING_POOL, 4,
	CARD_ID_FLOODED_STRAND, 4,
	CARD_ID_POLLUTED_DELTA, 2,
	// spawnwrithe
	CD_SPAWNWRITHER,
	CARD_ID_NATURALIZE, 4,
	CARD_ID_SPAWNWRITHE, 4,
	CARD_ID_WHIRLING_DERVISH, 4,
	CARD_ID_SLITH_PREDATOR, 4,
	CARD_ID_GIANT_GROWTH, 4,
	CARD_ID_CONCORDANT_CROSSROADS, 4,
	CARD_ID_HARMONIZE, 4,
	CARD_ID_LLANOWAR_ELVES, 4,
	CARD_ID_FYNDHORN_ELVES, 4,
	CARD_ID_PENDELHAVEN, 2,
	CARD_ID_FOREST, 22,
	// turbofog
	CD_TURBOFOG,
	CARD_ID_FOG, 4,
	CARD_ID_HOWLING_MINE, 4,
	CARD_ID_MILLSTONE, 4,
	CARD_ID_LUMINARCH_ASCENSION, 4,
	CARD_ID_IVORY_TOWER, 4,
	CARD_ID_WRATH_OF_GOD, 4,
	CARD_ID_LAND_TAX, 2,
	CARD_ID_FINAL_JUDGMENT, 2,
	CARD_ID_HOLY_DAY, 4,
	CARD_ID_FONT_OF_MYTHOS, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	CARD_ID_TEMPLE_GARDEN, 4,
	CARD_ID_PLAINS, 6,
	CARD_ID_FOREST, 6,
	// gold creatures
	CD_GOLD_CREATURES,
	CARD_ID_WOOLLY_THOCTAR, 4,
	CARD_ID_RHOX_WAR_MONK, 4,
	CARD_ID_STOIC_ANGEL, 4,
	CARD_ID_LIGHTNING_ANGEL, 4,
	CARD_ID_GADDOCK_TEEG, 4,
	CARD_ID_GAEAS_SKYFOLK, 4,
	CARD_ID_LOXODON_HIERARCH, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_TUNDRA, 4,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_TAIGA, 1,
	CARD_ID_PLATEAU, 1,
	CARD_ID_FLOODED_STRAND, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	CARD_ID_WOODED_FOOTHILLS, 2,
	CARD_ID_QASALI_PRIDEMAGE, 4,
	CARD_ID_JENARA_ASURA_OF_WAR, 4,
	// burn
	CD_BURN,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_CHAIN_LIGHTNING, 4,
	CARD_ID_FLAME_RIFT, 4,
	CARD_ID_FLAME_JAVELIN, 4,
	CARD_ID_LAVA_SPIKE, 4,
	CARD_ID_SPARK_ELEMENTAL, 4,
	CARD_ID_INCINERATE, 4,
	CARD_ID_SHARD_VOLLEY, 4,
	CARD_ID_MOUNTAIN, 16,
	CARD_ID_FIREBLAST, 4,
	CARD_ID_BARBARIAN_RING, 4,
	CARD_ID_PRICE_OF_PROGRESS, 4,
	// 2-land white
	CD_2_LAND_WHITE,
	CARD_ID_ISAMARU_HOUND_OF_KONDA, 4,
	CARD_ID_SAVANNAH_LIONS, 4,
	CARD_ID_ELITE_VANGUARD, 4,
	CARD_ID_GOLDMEADOW_HARRIER, 4,
	CARD_ID_GOLDMEADOW_STALWART, 4,
	CARD_ID_SOLTARI_FOOT_SOLDIER, 4,
	CARD_ID_SOLTARI_TROOPER, 4,
	CARD_ID_SOLTARI_MONK, 4,
	CARD_ID_SOLTARI_PRIEST, 4,
	CARD_ID_FIGURE_OF_DESTINY, 4,
	CARD_ID_KNIGHT_OF_MEADOWGRAIN, 4,
	CARD_ID_DISENCHANT, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_PLAINS, 2,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_WIZENED_CENN, 4,
	// no land
	CD_NO_LAND,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_GRIM_MONOLITH, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_CHROME_MOX, 4,
	CARD_ID_DARK_RITUAL, 4,
	CARD_ID_CABAL_RITUAL, 4,
	CARD_ID_MIND_STONE, 4,
	CARD_ID_LOTUS_PETAL, 1,
	CARD_ID_HYPNOTIC_SPECTER, 4,
	CARD_ID_JUGGERNAUT, 4,
	CARD_ID_JUZAM_DJINN, 4,
	CARD_ID_LODESTONE_GOLEM, 4,
	CARD_ID_CATHODION, 4,
	CARD_ID_COALITION_RELIC, 4,
	CARD_ID_DARKSTEEL_INGOT, 4,
	CARD_ID_BLACK_VISE, 4,
	CARD_ID_EVERFLOWING_CHALICE, 3,
	// workshop
	CD_WORKSHOP,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_JUGGERNAUT, 4,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_MANA_VAULT, 1,
	CARD_ID_TRISKELION, 4,
	CARD_ID_ANCESTRAL_RECALL, 1,
	CARD_ID_TIME_WALK, 1,
	CARD_ID_TIMETWISTER, 1,
	CARD_ID_DEMONIC_TUTOR, 1,
	CARD_ID_DARK_RITUAL, 4,
	CARD_ID_LIBRARY_OF_ALEXANDRIA, 1,
	CARD_ID_MISHRAS_WORKSHOP, 4,
	CARD_ID_STRIP_MINE, 1,
	CARD_ID_UNDERGROUND_SEA, 4,
	CARD_ID_WASTELAND, 4,
	CARD_ID_WINTER_ORB, 4,
	CARD_ID_ICY_MANIPULATOR, 2,
	CARD_ID_SU_CHI, 4,
	CARD_ID_TINKER, 1,
	CARD_ID_POLLUTED_DELTA, 4,
	CARD_ID_TOLARIAN_ACADEMY, 1,
	CARD_ID_WINDFALL, 1,
	CARD_ID_INKWELL_LEVIATHAN, 1,
	CARD_ID_FLOODED_STRAND, 1,
	CARD_ID_VAMPIRIC_TUTOR, 1,
	CARD_ID_IMPERIAL_SEAL, 1,
	// Clone human deck - handled special
	CD_CLONE_HUMAN_DECK,
	// faster slivers
	CD_FAST_SLIVERS,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_ANCESTRAL_RECALL, 1,
	CARD_ID_TIME_WALK, 1,
	CARD_ID_MUSCLE_SLIVER, 4,
	CARD_ID_SINEW_SLIVER, 4,
	CARD_ID_WINGED_SLIVER, 4,
	CARD_ID_CRYSTALLINE_SLIVER, 4,
	CARD_ID_PLATED_SLIVER, 4,
	CARD_ID_VIRULENT_SLIVER, 4,
	CARD_ID_FORCE_OF_WILL, 4,
	CARD_ID_DAZE, 4,
	CARD_ID_TUNDRA, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_TUNDRA, 4,
	CARD_ID_MISTY_RAINFOREST, 4,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_HARMONIC_SLIVER, 2,
	// vampires
	CD_VAMPIRES,
	CARD_ID_VAMPIRE_NOCTURNUS, 4,
	CARD_ID_VAMPIRE_LACERATOR, 4,
	CARD_ID_VAMPIRE_NIGHTHAWK, 4,
	CARD_ID_MALAKIR_BLOODWITCH, 4,
	CARD_ID_GATEKEEPER_OF_MALAKIR, 4,
	CARD_ID_BLOODGHAST, 4,
	CARD_ID_TENDRILS_OF_CORRUPTION, 4,
	CARD_ID_NIGHTS_WHISPER, 4,
	CARD_ID_MIND_SLUDGE, 2,
	CARD_ID_MARSH_FLATS, 4,
	CARD_ID_VERDANT_CATACOMBS, 4,
	CARD_ID_SWAMP, 16,
	CARD_ID_BAD_MOON, 2,
	// viseling
	CD_VISELING,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MOX_DIAMOND, 4,
	CARD_ID_BLACK_VISE, 4,
	CARD_ID_HOWLING_MINE, 4,
	CARD_ID_FONT_OF_MYTHOS, 4,
	CARD_ID_WINTER_ORB, 4,
	CARD_ID_SUDDEN_IMPACT, 4,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_CHAIN_LIGHTNING, 4,
	CARD_ID_INCINERATE, 4,
	CARD_ID_MOUNTAIN, 13,
	CARD_ID_MISHRAS_WORKSHOP, 4,
	// big burn
	CD_BIG_BURN,
	CARD_ID_MANA_FLARE, 4,
	CARD_ID_HEARTBEAT_OF_SPRING, 4,
	CARD_ID_GAUNTLET_OF_MIGHT, 4,
	CARD_ID_RAMPANT_GROWTH, 4,
	CARD_ID_KODAMAS_REACH, 4,
	CARD_ID_FIREBALL, 4,
	CARD_ID_DISINTEGRATE, 4,
	CARD_ID_HURRICANE, 4,
	CARD_ID_HARMONIZE, 4,
	CARD_ID_WOODED_FOOTHILLS, 4,
	CARD_ID_STOMPING_GROUND, 4,
	CARD_ID_MOUNTAIN, 10,
	CARD_ID_TAIGA, 4,
	CARD_ID_ARID_MESA, 4,
	// awwfinity
	CD_AWWFFINITY,
	CARD_ID_ANCESTRAL_RECALL,1,
	CARD_ID_ARCBOUND_WORKER,4,
	CARD_ID_BLACK_LOTUS,1,
	CARD_ID_FROGMITE,4,
	CARD_ID_LODESTONE_GOLEM,4,
	CARD_ID_MASTER_OF_ETHERIUM,4,
	CARD_ID_MOX_EMERALD,1,
	CARD_ID_MOX_JET,1,
	CARD_ID_MOX_PEARL,1,
	CARD_ID_MOX_RUBY,1,
	CARD_ID_MOX_SAPPHIRE,1,
	CARD_ID_SOL_RING,1,
	CARD_ID_SU_CHI,1,
	CARD_ID_TIME_WALK,1,
	CARD_ID_THOUGHTCAST,4,
	CARD_ID_DARKSTEEL_CITADEL,4,
	CARD_ID_SEAT_OF_THE_SYNOD,4,
	CARD_ID_TOLARIAN_ACADEMY,1,
	CARD_ID_MISHRAS_WORKSHOP,4,
	CARD_ID_ISLAND,6,
	CARD_ID_MYR_ENFORCER,4,
	CARD_ID_BROODSTAR,4,
	// yggdrasil b/g
	CD_YGGDRASIL_GB,
	CARD_ID_MAELSTROM_PULSE, 4,
	CARD_ID_SPIRITMONGER, 4,
	CARD_ID_SHRIEKMAW, 3,
	CARD_ID_VERDANT_CATACOMBS, 4,
	CARD_ID_BAYOU, 4,
	CARD_ID_OVERGROWN_TOMB, 4,
	CARD_ID_BIRDS_OF_PARADISE, 4,
	CARD_ID_SWAMP, 4,
	CARD_ID_URBORG, 1,
	CARD_ID_FOREST, 3,
	CARD_ID_VAMPIRE_NIGHTHAWK, 4,
	CARD_ID_VISARA_THE_DREADFUL, 1,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_RAVENOUS_BALOTH, 4,
	CARD_ID_SCUTE_MOB, 4,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_GATEKEEPER_OF_MALAKIR, 4,
	CARD_ID_HARMONIZE, 3,
	CARD_ID_BEACON_OF_UNREST, 1,
	CARD_ID_PENDELHAVEN, 1,
	//_nykotorian_zoo
	CD_NYKOTORIAN_ZOO,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_WILD_NACATL, 4,
	CARD_ID_KIRD_APE, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_TARMOGOYF, 4,
	CARD_ID_WHEEL_OF_FORTUNE, 1,
	CARD_ID_QASALI_PRIDEMAGE, 4,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_LIGHTNING_HELIX, 2,
	CARD_ID_FIREBLAST, 1,
	CARD_ID_CHAIN_LIGHTNING, 4,
	CARD_ID_FOREST, 1,
	CARD_ID_PLAINS, 1,
	CARD_ID_MOUNTAIN, 1,
	CARD_ID_PLATEAU, 3,
	CARD_ID_TAIGA, 2,
	CARD_ID_SAVANNAH, 1,
	CARD_ID_WOODED_FOOTHILLS, 3,
	CARD_ID_WINDSWEPT_HEATH, 3,
	CARD_ID_ARID_MESA, 3,
	CARD_ID_FIGURE_OF_DESTINY, 2,
	CARD_ID_GRIM_LAVAMANCER, 4,
	//_splicers
	CD_SPLICERS,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MASTER_SPLICER, 4,
	CARD_ID_WING_SPLICER, 4,
	CARD_ID_BLADE_SPLICER, 4,
	CARD_ID_VITAL_SPLICER, 4,
	CARD_ID_NOBLE_HIERARCH, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_TEMPERED_STEEL, 4,
	CARD_ID_UNSUMMON, 4,
	CARD_ID_FOREST, 1,
	CARD_ID_PLAINS, 1,
	CARD_ID_ISLAND, 1,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_TUNDRA, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	CARD_ID_SEASIDE_CITADEL, 4,
	//_Fast_Metalcraft
	CD_FAST_METALCRAFT,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MEMNITE, 4,
	CARD_ID_FLAYER_HUSK, 4,
	CARD_ID_SIGNAL_PEST, 4,
	CARD_ID_CARAPACE_FORGER, 4,
	CARD_ID_EZURIS_BRIGADE, 2,
	CARD_ID_ARDENT_RECRUIT, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_TEMPERED_STEEL, 4,
	CARD_ID_ETCHED_CHAMPION, 4,
	CARD_ID_FOREST, 4,
	CARD_ID_PLAINS, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_TEMPLE_GARDEN, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	//_Proliferate
	CD_PROLIFERATE,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_VAULT, 1,
	CARD_ID_CLOUDFIN_RAPTOR, 4,
	CARD_ID_EXPERIMENT_ONE, 4,
	CARD_ID_GYRE_SAGE, 4,
	CARD_ID_THRUMMINGBIRD, 4,
	CARD_ID_PLAXCASTER_FROGLING, 4,
	CARD_ID_RENEGADE_KRASIS, 4,
	CARD_ID_CYTOPLAST_ROOT_KIN, 4,
	CARD_ID_TEZZERETS_GAMBIT, 4,
	CARD_ID_TRISKELION, 4,
	CARD_ID_FOREST, 4,
	CARD_ID_ISLAND, 3,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_BREEDING_POOL, 4,
	CARD_ID_MISTY_RAINFOREST, 4,
	// GW Maverick
	CD_MAVERICK,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_NOBLE_HIERARCH, 4,
	CARD_ID_QASALI_PRIDEMAGE, 4,
	CARD_ID_KNIGHT_OF_THE_RELIQUARY, 4,
	CARD_ID_STONEFORGE_MYSTIC, 4,
	CARD_ID_THALIA_GUARDIAN_OF_THRABEN, 4,
	CARD_ID_GADDOCK_TEEG, 2,
	CARD_ID_RAFIQ_OF_THE_MANY, 1,
	CARD_ID_AJANI_CALLER_OF_THE_PRIDE, 2,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_GREEN_SUNS_ZENITH, 4,
	CARD_ID_BATTERSKULL, 1,
	CARD_ID_UMEZAWAS_JITTE, 1,
	CARD_ID_FOREST, 4,
	CARD_ID_PLAINS, 3,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_TEMPLE_GARDEN, 4,
	CARD_ID_WINDSWEPT_HEATH, 4,
	// WU Caw-Blade
	CD_CAW_BLADE,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_COUNTERSPELL, 4,
	CARD_ID_FORCE_OF_WILL, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_WRATH_OF_GOD, 2,
	CARD_ID_OBLIVION_RING, 4,
	CARD_ID_JACE_THE_MIND_SCULPTOR, 4,
	CARD_ID_ELSPETH_KNIGHT_ERRANT, 2,
	CARD_ID_STONEFORGE_MYSTIC, 4,
	CARD_ID_SQUADRON_HAWK, 4,
	CARD_ID_BATTERSKULL, 1,
	CARD_ID_UMEZAWAS_JITTE, 1,
	CARD_ID_SWORD_OF_FIRE_AND_ICE, 1,
	CARD_ID_SWORD_OF_FEAST_AND_FAMINE, 1,
	CARD_ID_TUNDRA, 4,
	CARD_ID_HALLOWED_FOUNTAIN, 4,
	CARD_ID_FLOODED_STRAND, 4,
	CARD_ID_ISLAND, 4,
	CARD_ID_PLAINS, 3,
	// Rec-Sur
	CD_REC_SUR,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_BIRTHING_POD, 2,
	CARD_ID_MIMIC_VAT, 2,
	CARD_ID_LIVING_DEATH, 2,
	CARD_ID_SURVIVAL_OF_THE_FITTEST, 2,
	CARD_ID_PERNICIOUS_DEED, 2,
	CARD_ID_OBLIVION_RING, 2,
	CARD_ID_VINDICATE, 4,
	CARD_ID_WALL_OF_ROOTS, 4,
	CARD_ID_SQUADRON_HAWK, 4,
	CARD_ID_SPIKE_FEEDER, 2,
	CARD_ID_SPIKE_WEAVER, 2,
	CARD_ID_FACELESS_BUTCHER, 1,
	CARD_ID_SPIRITMONGER, 1,
	CARD_ID_KARMIC_GUIDE, 1,
	CARD_ID_DERANGED_HERMIT, 1,
	CARD_ID_PRIMEVAL_TITAN, 1,
	CARD_ID_GRAVE_TITAN, 1,
	CARD_ID_SUN_TITAN, 1,
	CARD_ID_ELESH_NORN_GRAND_CENOBITE, 1,
	CARD_ID_TAIGA, 4,
	CARD_ID_BAYOU, 4,
	CARD_ID_SCRUBLAND, 4,
	CARD_ID_CITY_OF_BRASS, 3,
	CARD_ID_TEMPLE_GARDEN, 1,
	CARD_ID_GODLESS_SHRINE, 1,
	CARD_ID_OVERGROWN_TOMB, 1,
	//_Angus_Mackenzie_UGW_Exhalted
	CD_ANGUS_MC_KENZIE,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_UNSUMMON, 4,
	CARD_ID_NOBLE_HIERARCH, 4,
	CARD_ID_JHESSIAN_INFILTRATOR, 4,
	CARD_ID_RHOX_WAR_MONK, 4,
	CARD_ID_RAFIQ_OF_THE_MANY, 4,
	CARD_ID_BATTLEGRACE_ANGEL, 2,
	CARD_ID_BANESLAYER_ANGEL, 2,
	CARD_ID_ENLISTED_WURM, 3,
	CARD_ID_FOREST, 1,
	CARD_ID_ISLAND, 1,
	CARD_ID_PLAINS, 1,
	CARD_ID_TUNDRA, 2,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_SEASIDE_CITADEL, 4,
	CARD_ID_BREEDING_POOL, 3,
	CARD_ID_HALLOWED_FOUNTAIN, 1,
	CARD_ID_TEMPLE_GARDEN, 3,
	//_Xira_Arien_BGR_Jund
	CD_XIRA_ARIEN,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_JET, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_WHEEL_OF_FORTUNE, 1,
	CARD_ID_PUTRID_LEECH, 4,
	CARD_ID_BLOODBRAID_ELF, 4,
	CARD_ID_BROODMATE_DRAGON, 4,
	CARD_ID_SPROUTING_THRINAX, 4,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_TERMINATE, 4,
	CARD_ID_MAELSTROM_PULSE, 4,
	CARD_ID_BITUMINOUS_BLAST, 2,
	CARD_ID_GARRUK_WILDSPEAKER, 3,
	CARD_ID_FOREST, 1,
	CARD_ID_MOUNTAIN, 1,
	CARD_ID_SWAMP, 1,
	CARD_ID_TAIGA, 4,
	CARD_ID_BAYOU, 4,
	CARD_ID_BADLANDS, 4,
	CARD_ID_SAVAGE_LANDS, 4,
	CARD_ID_WOODED_FOOTHILLS, 2,
	CARD_ID_BLOODSTAINED_MIRE, 2,
	//_Tuknir_Deathlok_RG_Zoo
	CD_TUKNIR_DEATHLOK,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_WHEEL_OF_FORTUNE, 1,
	CARD_ID_FIGURE_OF_DESTINY, 4,
	CARD_ID_GRIM_LAVAMANCER, 4,
	CARD_ID_KIRD_APE, 4,
	CARD_ID_TARMOGOYF, 4,
	CARD_ID_RIP_CLAN_CRASHER, 4,
	CARD_ID_GARRUKS_COMPANION, 4,
	CARD_ID_LIGHTNING_BOLT, 4,
	CARD_ID_CHAIN_LIGHTNING, 4,
	CARD_ID_RANCOR, 4,
	CARD_ID_FOREST, 5,
	CARD_ID_MOUNTAIN, 5,
	CARD_ID_STOMPING_GROUND, 4,
	CARD_ID_TAIGA, 4,
	CARD_ID_WOODED_FOOTHILLS, 4,
	//_Animar,_Soul_of_Elements_RUG
	CD_ANIMAR,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_WHEEL_OF_FORTUNE, 1,
	CARD_ID_BIRDS_OF_PARADISE, 4,
	CARD_ID_HELLSPARK_ELEMENTAL, 4,
	CARD_ID_NEST_INVADER, 4,
	CARD_ID_GHITU_SLINGER, 4,
	CARD_ID_SEA_DRAKE, 4,
	CARD_ID_FLAMETONGUE_KAVU, 3,
	CARD_ID_RAVENOUS_BALOTH, 3,
	CARD_ID_SIMIC_SKY_SWALLOWER, 2,
	CARD_ID_KOZILEK_BUTCHER_OF_TRUTH, 2,
	CARD_ID_FOREST, 3,
	CARD_ID_ISLAND, 3,
	CARD_ID_MOUNTAIN, 3,
	CARD_ID_VOLCANIC_ISLAND, 4,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_TAIGA, 4,
	CARD_ID_MISTY_RAINFOREST, 2,
	CARD_ID_WOODED_FOOTHILLS, 2,
	//_Edric,_Spymaster_of_Trest_UG
	CD_EDRIC,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_MOX_SAPPHIRE, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_RITE_OF_REPLICATION, 4,
	CARD_ID_RANCOR, 4,
	CARD_ID_NOBLE_HIERARCH, 4,
	CARD_ID_JHESSIAN_INFILTRATOR, 4,
	CARD_ID_WALL_OF_ROOTS, 4,
	CARD_ID_CYTOPLAST_ROOT_KIN, 4,
	CARD_ID_TRADEWIND_RIDER, 2,
	CARD_ID_SOWER_OF_TEMPTATION, 2,
	CARD_ID_TRISKELION, 4,
	CARD_ID_FOREST, 6,
	CARD_ID_ISLAND, 6,
	CARD_ID_TROPICAL_ISLAND, 4,
	CARD_ID_MISTY_RAINFOREST, 4,
	CARD_ID_BREEDING_POOL, 4,
	//_Doran,_the_Siege_Tower_WBG_Treefolks
	CD_DORAN,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_EMERALD, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_BIRDS_OF_PARADISE, 4,
	CARD_ID_TREEFOLK_HARBINGER, 4,
	CARD_ID_CHAMELEON_COLOSSUS, 4,
	CARD_ID_DAUNTLESS_DOURBARK, 4,
	CARD_ID_INDOMITABLE_ANCIENTS, 2,
	CARD_ID_TIMBER_PROTECTOR, 4,
	CARD_ID_VINDICATE, 4,
	CARD_ID_PATH_TO_EXILE, 4,
	CARD_ID_FOREST, 5,
	CARD_ID_PLAINS, 1,
	CARD_ID_SWAMP, 1,
	CARD_ID_SAVANNAH, 4,
	CARD_ID_BAYOU, 4,
	CARD_ID_OVERGROWN_TOMB, 4,
	CARD_ID_TEMPLE_GARDEN, 4,
	CARD_ID_SCRUBLAND, 1,
	//_Kaalia_of_the_Vast_WRB_Evil_Angels
	CD_KAALIA,
	CARD_ID_MOX_JET, 1,
	CARD_ID_MOX_PEARL, 1,
	CARD_ID_MOX_RUBY, 1,
	CARD_ID_BLACK_LOTUS, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_WHEEL_OF_FORTUNE, 1,
	CARD_ID_SOL_RING, 1,
	CARD_ID_MANA_CRYPT, 1,
	CARD_ID_EMERIA_ANGEL, 4,
	CARD_ID_BANESLAYER_ANGEL, 4,
	CARD_ID_EXALTED_ANGEL, 4,
	CARD_ID_ANGEL_OF_DESPAIR, 4,
	CARD_ID_PLATINUM_ANGEL, 2,
	CARD_ID_AKROMA_ANGEL_OF_FURY, 1,
	CARD_ID_AKROMA_ANGEL_OF_WRATH, 1,
	CARD_ID_VINDICATE, 4,
	CARD_ID_TERMINATE, 4,
	CARD_ID_MOUNTAIN, 1,
	CARD_ID_PLAINS, 6,
	CARD_ID_SWAMP, 5,
	CARD_ID_BADLANDS, 4,
	CARD_ID_SCRUBLAND, 4,
	CARD_ID_PLATEAU, 4,
	CD_HYDRA,
	/*
	CARD_ID_HYDRA_HEAD, 11
	CARD_ID_RAVENOUS_BRUTE_HEAD, 4
	CARD_ID_SAVAGE_VIGOR_HEAD, 1
	CARD_ID_SHRIEKING_TITAN_HEAD, 1
	CARD_ID_SNAPPING_FANG_HEAD, 1
	CARD_ID_DISORIENTING_GLOWER, 5
	CARD_ID_DISTRACT_THE_HYDRA, 5
	CARD_ID_GROWN_FROM_THE_STUMP, 4
	CARD_ID_HYDRAS_IMPENETRABLE_HIDE, 4
	CARD_ID_NECK_TANGLE, 3
	CARD_ID_NOXIOUS_HYDRA_BREATH, 4
	CARD_ID_STRIKE_THE_WEAK_SPOT, 2
	CARD_ID_SWALLOW_THE_HERO_WHOLE, 5
	CARD_ID_TORN_BETWEEN_HEADS, 4
	CARD_ID_UNIFIED_LUNGE, 6
	*/
	-2
};

int get_cd_number(void){
	int i = 0;
	int pos = 0;
	while( cd_list[i] != -2 ){
			if (cd_list[i] < 0 && cd_list[i] > CD_MIN_SPECIAL_DECK){
				pos++;
			}
			i++;
	}
	return pos;
}

int get_unlock_code(int id){
	if( id > -1 ){
		unsigned int i;
		for(i=0; i<(sizeof(lock_ids)/sizeof(lock_ids[0])); i++){
			if( lock_ids[i] == id ){
				return i;
			}
		}
		return 0;
	}
	return 0;
}

int is_unlocked_by_challenge(int player, int unlock_level){
	if( player == AI ){
		return 1;
	}
	char buffer[200];
	scnprintf(buffer, 200, "Faces//%s.jat", unlock_codes[unlock_level]);
	FILE *file = fopen(buffer, "r");
	if( file != NULL  ){
		fclose(file);
		unlocked[unlock_level] = 1;
		return 1;
	}
	return 0;
}

int is_unlocked(int player, int card, event_t event, int unlock_level){
	if( player == AI || unlocked[unlock_level] == 1 || get_setting(SETTING_TOURNAMENT_MODE) ){
		return 1;
	}

	if( unlock_level != 31 && unlock_level != 36 && unlock_level != 38 && unlock_level != 39 && player > 0 && card > 0 ){
		unlock_level = get_unlock_code(get_id(player, card));
		if( unlock_level == 0 ){
			return 0;
		}
	}

	char buffer[200];

	if( unlocked[unlock_level] == 2 && lock_ids[unlock_level] != -1 && lock_ids[unlock_level] != -2){
		card_ptr_t* c = cards_ptr[ lock_ids[unlock_level] ];
		//if( unlock_level >= 10 && unlock_level < 20 ){ unlock_level++; }
		int challenge_number = unlock_level % 10;
		int challenge_set = 1+(unlock_level - challenge_number)/10;
		if( event != 0 ){
			if( comes_into_play(player, card, event) == 1 || event == EVENT_RESOLVE_ACTIVATION ){
				int pos = scnprintf(buffer, 200, " You must unlock this card \nby completing challenge\nset %d number %d\n", challenge_set, challenge_number+1);
				scnprintf(buffer+pos, 200-pos, "%s", challenge_names[unlock_level] );
				do_dialog(player, player, card , -1, -1, buffer, 0);
			}
		}
		scnprintf(c->name, strlen(c->name)+1,"%d #%d", challenge_set, challenge_number+1);
		//fprintf(file1, "already known as locked %d\n", unlocked[unlock_level] );
		return 0;
	}

	scnprintf(buffer, 200, "Faces//%s.jat", unlock_codes[unlock_level]);
	FILE *file = fopen(buffer, "r");
	if( file != NULL  ){
		fclose(file);
		unlocked[unlock_level] = 1;
		return 1;
	}

	if( event != 0 && comes_into_play(player, card, event) == 1 && lock_ids[unlock_level] != -1 && lock_ids[unlock_level] != -2){
		//if( unlock_level >= 10 && unlock_level < 20 ){ unlock_level++; }
		int challenge_number = unlock_level % 10;
		int challenge_set = 1+(unlock_level - challenge_number)/10;
		int pos = scnprintf(buffer, 200, " You must unlock this card \nby completing challenge\nset %d number %d\n", challenge_set, challenge_number+1);
		scnprintf(buffer+pos, 200-pos, "%s", challenge_names[unlock_level] );
		do_dialog(player, player, card , -1, -1, buffer, 0);
	}
	//fprintf(file1, "setting check to failed %d\n", unlocked[unlock_level] );
	unlocked[unlock_level] = 2;

	//fclose(file1);
	return 0;
}

int get_challenge_round(void){
	return challenge_round;
}

int check_type(type_t type){
	target_definition_t td;
	default_target_definition(0, 0, &td, type);
	td.allowed_controller = AI;
	td.preferred_controller = AI;
	td.illegal_abilities = 0;
	if( target_available(0, 0, &td) == 5 ){
		td.allowed_controller = HUMAN;
		td.preferred_controller = HUMAN;
		if( target_available(0, 0, &td) == 1 ){
			return 1;
		}
	}
	return 0;
}

#if 0
int unlocked_challenge[100];
int opponent_turns = 0;
void apply_challenge_10_to_1(int player, int card, event_t event){
	return; // this challenge is obsolete
	if( player != HUMAN ){ return; }

	int round = gauntlet_round - 1;
	int success = 0;
	if( round == 59 && check_type(TYPE_LAND) ){
		success = 1;
	}
	else if( round == 60 ){
		int ai_ok=0;
		int human_ok=0;
		int count = 0;
		while(count < active_cards_count[AI]){
			card_data_t* card_d = get_card_data(AI, count);
			if((card_d->type & TYPE_CREATURE) && in_play(AI, count) && get_power(AI, count) == 5 ){
				ai_ok = 1;
			}
			count++;
		}
		count = 0;
		while(count < active_cards_count[HUMAN]){
			card_data_t* card_d = get_card_data(HUMAN, count);
			if((card_d->type & TYPE_CREATURE) && in_play(HUMAN, count) && get_power(HUMAN, count) == 1 ){
				human_ok = 1;
			}
			count++;
		}
		if( ai_ok == 1 && human_ok == 1 ){
			success = 1;
		}
	}
	else if( round == 61 && count_graveyard(HUMAN) == 1 && count_graveyard(AI) == 5 ){
		success = 1;
	}
	else if( round == 62 && life[HUMAN] == 1 && life[AI] == 5 ){
		success = 1;
	}
	else if( round == 63 && hand_count[HUMAN] == 1 && hand_count[AI] == 5 ){
		success = 1;
	}
	else if( round == 64 && count_deck(HUMAN) == 1 && count_deck(AI) == 5 ){
		success = 1;
	}
	else if( round == 65 && count_rfg(HUMAN) == 1 && count_rfg(AI) == 5 ){
		success = 1;
	}
	else if( round == 66 && check_type(TYPE_CREATURE) ){
		success = 1;
	}
	else if( round == 67 ){
		// keep track of consecutive turns
		if(trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				if(current_turn==HUMAN){
					opponent_turns = 0;
				}
				else{
					opponent_turns++;
					if( opponent_turns == 5 ){
						success = 1;
					}
				}
			}
		}

	}
	else if( round == 68 && check_type(TYPE_ARTIFACT) ){
		success = 1;
	}
	else if( round == 69 && count_cards_by_id(HUMAN, 528) == 1 &&  count_cards_by_id(AI, 528) == 5 ){
		success = 1;
	}

	if( success == 1 ){
		raw_set_poison(HUMAN, 1);
		unlocked_challenge[round] = 2;
	}
}

void apply_challenge_10_to_1_upkeep(void){
	int round = gauntlet_round - 1;
	//if( life[HUMAN] == 20 ){ gain_life(HUMAN, round); }
	if(round > 59 && unlocked_challenge[round-1] != 2 ){
		do_dialog(0, 0, 0, -1, -1, "Sorry, you did not meet the victory condition for the previous round.", 1);
		lose_the_game(HUMAN);
	}

	if( unlocked_challenge[round] < 1 ){
		unlocked_challenge[round] = 1;
		if( round == 59 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must control 1 land while your opponent has 5.", 1);
		}
		else if( round == 60 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must control a 1/1 while your opponent has a 5/5.", 1);
		}
		else if( round == 61 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 card in the graveyard while your opponent has 5.", 1);
		}
		else if( round == 62 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 life while your opponent has 5.", 1);
		}
		else if( round == 63 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 card in hand while your opponent has 5.", 1);
		}
		else if( round == 64 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 card in your deck while your opponent has 5.", 1);
		}
		else if( round == 65 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 card exiled while your opponent has 5.", 1);
		}
		else if( round == 66 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 creature while your opponent has 5.", 1);
		}
		else if( round == 67 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, your opponent must take 5 consecutive turns.", 1);
		}
		else if( round == 68 ){
			do_dialog(0, 0, 0, -1, -1, "Before you can win this challenge, you must have 1 artifact while your opponent has 5.", 1);
		}
		else if( round == 69 ){
			do_dialog(0, 0, 0, -1, -1, "An 11th challenge... how unfair.\nBefore you can win this challenge, you must have 1 Strip Mine while your opponent has 5.", 1);
		}
	}
}
#endif

static int warp_card(type_t required_type, type_t illegal_type, const char* text){
	while (1){
		if(ai_is_speculating != 1){
			int iid = choose_a_card(text, -1, -1);
			if (is_what(-1, iid, required_type)
				&& !(illegal_type && is_what(-1, iid, illegal_type))
			   ){
				update_rules_engine(check_card_for_rules_engine(iid));
				int card_added = add_card_to_hand(HUMAN, iid);
				return card_added;
			}
		}
	}
	return -1;
}

static int get_card_human(int csvid){
	int iid = get_internal_card_id_from_csv_id( csvid );
	update_rules_engine(check_card_for_rules_engine(iid));
	int card_added = add_card_to_hand( HUMAN, iid );
	put_into_play(HUMAN, card_added);
	return card_added;
}

static int get_card(int csvid){
	int iid = get_internal_card_id_from_csv_id( csvid );
	update_rules_engine(check_card_for_rules_engine(iid));
	int card_added = add_card_to_hand( AI, iid );
	put_into_play(AI, card_added);
	return card_added;
}

void apply_challenge_not_my_deck_upkeep(void){
	int count = 0;
	while( count < active_cards_count[HUMAN]){
		if(! in_play(HUMAN, count) ){
			put_on_top_of_deck(HUMAN, count);
		}
		count++;
	}
	int *deck = deck_ptr[HUMAN];
	int cd = count_deck(HUMAN);
	int i, clrs = 0;
	for(i=0;i<cd;i++){
		if( ! is_what(-1, deck[i], TYPE_LAND) && cards_data[deck[i]].color > 1 ){
			clrs |= cards_data[deck[i]].color;
		}
	}
	create_random_test_deck(HUMAN, -1, clrs, 0, available_slots, cd);
	// life[HUMAN] = clrs;
	// Replace every non-land with another card of the same casting cost
	/*
	int i;
	for(i=0;i<count_deck(HUMAN);i++){
		card_ptr_t* c = cards_ptr[ cards_data[ deck[i] ].id ];

		int attempts = 0;
		int card_found = deck[i];

		// do not replace lands
		if( cards_data[ deck[i] ].type & TYPE_LAND ){
			attempts = 4000;
		}
		while( attempts++ < 4000){
			int card = get_random_card();
			int id = get_internal_card_id_from_csv_id( card );
			card_ptr_t* c1 = cards_ptr[ card];
			if( ! ( cards_data[ id ].type & TYPE_LAND ) &&  c->req_colorless == c1->req_colorless && c->req_black == c1->req_black && c->req_blue == c1->req_blue &&
				  c->req_green == c1->req_green && c->req_red == c1->req_red && c->req_white == c1->req_white ){
				attempts = 4000;
				card_found = id;
			}
		}
		deck[i] = card_found;
	}
	*/
	draw_cards(HUMAN, 7);
	//raw_set_poison(HUMAN, 1);
	return;
}

void apply_challenge_puzzle_upkeep(void){
	int round = gauntlet_round - 1;

	if( challenge1 == 8 ){
		if ( POISON_COUNTERS(HUMAN) == 0 ){
			apply_challenge_not_my_deck_upkeep();
		}
		return;
	}

	// Remove our hand and deck
	if( challenge1 == 7){
		int count = 0;
		while( count < active_cards_count[HUMAN]){
			if(! in_play(HUMAN, count) ){
				put_on_top_of_deck(HUMAN, count);
			}
			count++;
		}
		int *deck = deck_ptr[HUMAN];
		int i;
		for(i=0;i<10;i++){
			deck[i] = get_internal_card_id_from_csv_id( CARD_ID_SWAMP );
		}
		deck[10]=-1;

		while( count < active_cards_count[AI]){
			if(! in_play(AI, count) ){
				put_on_top_of_deck(AI, count);
			}
			count++;
		}
		deck = deck_ptr[AI];
		for(i=0;i<60;i++){
			deck[i] = get_internal_card_id_from_csv_id( CARD_ID_SWAMP );
		}


	}

	// Add free cards to hand
	int* deck = deck_ptr[AI];
	if( round == 70 ){
		life[AI]=8;
		life[HUMAN]=9;
		get_card_human(CARD_ID_SWAMP);
		get_card_human(CARD_ID_SWAMP);
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add 2 sorcery and 1 creature to your hand.\n\nWarning: the next round requires you to win in your upkeep, so you probably want to set a stop there.", 1);
		warp_card(TYPE_SORCERY, 0, "Choose sorcery #1");
		warp_card(TYPE_SORCERY, 0, "Choose sorcery #2");
		warp_card(TYPE_CREATURE, 0, "Choose a creature");
	}
	else if( round == 71 ){
		life[AI]=8;
		get_card_human(CARD_ID_BADLANDS);
		get_card_human(CARD_ID_PLATEAU);
		do_dialog(0, 0, 0, -1, -1, "Win the game before your draw step.  You may add 2 instants to your hand.", 1);
		warp_card(TYPE_INSTANT | TYPE_INTERRUPT, 0, "Choose instant #1");
		warp_card(TYPE_INSTANT | TYPE_INTERRUPT, 0, "Choose instant #2");
	}
	else if( round == 72 ){
		deck[2]=-1;
		do_dialog(0, 0, 0, -1, -1, "Win the game before your draw step.  You may add any 4 cards to your hand.", 1);
		warp_card(TYPE_ANY, 0, "Choose card #1");
		warp_card(TYPE_ANY, 0, "Choose card #2");
		warp_card(TYPE_ANY, 0, "Choose card #3");
		warp_card(TYPE_ANY, 0, "Choose card #4");
	}
	else if( round == 73 ){
		life[AI]=72;
		deck[30]=-1;
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card_human(CARD_ID_ISLAND);
		get_card( CARD_ID_WALL_OF_AIR );
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add any 3 non-artifacts to your hand.", 1);
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #1");
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #2");
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #3");
	}
	else if( round == 74 ){
		life[AI]=72;
		get_card_human(CARD_ID_MOX_SAPPHIRE);
		get_card_human(CARD_ID_MOX_SAPPHIRE);
		get_card_human(CARD_ID_MOX_RUBY);
		get_card_human(CARD_ID_MOX_RUBY);
		get_card_human(CARD_ID_MOX_RUBY);
		get_card_human(CARD_ID_MOX_RUBY);
		get_card_human(CARD_ID_MOX_RUBY);
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add any 3 non-artifacts to your hand.", 1);
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #1");
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #2");
		warp_card(TYPE_ANY, TYPE_ARTIFACT, "Choose card #3");
	}
	else if( round == 75 ){
		life[AI]=22;
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add 4 creatures, 1 instant, and 1 artifact to your hand.", 1);
		warp_card(TYPE_CREATURE, 0, "Choose creature #1");
		warp_card(TYPE_CREATURE, 0, "Choose creature #2");
		warp_card(TYPE_CREATURE, 0, "Choose creature #3");
		warp_card(TYPE_CREATURE, 0, "Choose creature #4");
		warp_card(TYPE_INSTANT | TYPE_INTERRUPT, 0, "Choose an instant");
		warp_card(TYPE_ARTIFACT, 0, "Choose an artifact");
	}
	else if( round == 76 ){
		life[AI]=12;
		get_card_human(CARD_ID_TAIGA);
		get_card_human(CARD_ID_CRUCIBLE_OF_WORLDS);
		add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_FIREBALL  ) );
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add an enchantment and an artifact to your hand.", 1);
		warp_card(TYPE_ENCHANTMENT, 0, "Choose an enchantment");
		warp_card(TYPE_ARTIFACT, 0, "Choose an artifact");
	}
	else if( round == 77 ){
		life[HUMAN]=1;
		life[AI]=36;
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add 3 creatures, 1 instant, 1 enchantment, and 1 artifact to your hand.", 1);
		warp_card(TYPE_CREATURE, 0, "Choose creature #1");
		warp_card(TYPE_CREATURE, 0, "Choose creature #2");
		warp_card(TYPE_CREATURE, 0, "Choose creature #3");
		warp_card(TYPE_INSTANT | TYPE_INTERRUPT, 0, "Choose an instant");
		warp_card(TYPE_ENCHANTMENT, 0, "Choose an enchantment");
		warp_card(TYPE_ARTIFACT, 0, "Choose an artifact");
	}
	else if( round == 78 ){
		life[AI]=16;
		get_card_human(CARD_ID_BADLANDS);
		get_card_human(CARD_ID_BADLANDS);
		get_card_human(CARD_ID_BADLANDS);
		get_card_human(CARD_ID_BADLANDS);
		add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_EYE_OF_UGIN ) );
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add any 2 cards to your hand.", 1);
		warp_card(TYPE_ANY, 0, "Choose card #1");
		warp_card(TYPE_ANY, 0, "Choose card #2");
	}
	else if( round == 79 ){
		life[AI]=34;
		get_card_human(CARD_ID_SWAMP);
		get_card_human(CARD_ID_FOREST);
		get_card_human(CARD_ID_FOREST);
		do_dialog(0, 0, 0, -1, -1, "Win the game before your 2nd main phase.  You may add 1 creature and 3 sorcery to your hand.", 1);
		warp_card(TYPE_CREATURE, 0, "Choose a creature");
		warp_card(TYPE_SORCERY, 0, "Choose sorcery #1");
		warp_card(TYPE_SORCERY, 0, "Choose sorcery #2");
		warp_card(TYPE_SORCERY, 0, "Choose sorcery #3");
	}
}


void apply_challenge_vanguard(void){
	int avatars[] = { CARD_ID_AVATAR_CHAOS_ORB , CARD_ID_AVATAR_SERRA_ANGEL, CARD_ID_AVATAR_MOMIR_VIG,
					CARD_ID_AVATAR_HEARTWOOD_STORYTELLER, CARD_ID_AVATAR_ONI_OF_WILD_PLACES,
					CARD_ID_AVATAR_NEKRATAAL, CARD_ID_AVATAR_SERRA_ANGEL, CARD_ID_AVATAR_PRODIGAL_SORCERER,
					CARD_ID_AVATAR_AKROMA, CARD_ID_AVATAR_REAPER_KING };
	if( challenge_round > 9 && challenge_round < 20 ){
		generate_reserved_token_by_id(AI, avatars[challenge_round - 10 ]);
		if( challenge_round == 12 ){
			generate_reserved_token_by_id(HUMAN, CARD_ID_AVATAR_MOMIR_VIG);
		}
		else if( challenge_round == 19 ){
			generate_reserved_token_by_id(HUMAN, CARD_ID_AVATAR_CHAOS_ORB );
		}
	}

}

void apply_challenge_onslaught(void){

	int cards[] = { CARD_ID_SACRED_NECTAR, CARD_ID_SPAWNWRITHE, CARD_ID_BITTERBLOSSOM, CARD_ID_RELENTLESS_RATS,
					CARD_ID_MEDDLING_MAGE, CARD_ID_COAT_OF_ARMS, CARD_ID_TIME_WALK, CARD_ID_FLAME_JAVELIN,
					CARD_ID_UNDERWORLD_DREAMS, CARD_ID_OBLITERATE };

	int round = gauntlet_round-1;
	//gain_life(AI, 2);
	if( round >= 29 && round < 39 ){
		add_csvid_to_rfg(AI, cards[round - 29]);
		play_card_in_exile_for_free(AI, AI, cards[round - 29]);
	}

	// decimation
	if( round >= 59 && round <= 68 ){
		mill(HUMAN, 6);
		poison(HUMAN, 1);
		life[HUMAN]-=2;
	}

	// chaos orb
	if( challenge4 == 0 || challenge3 == 5 ){
		int i;
		int count = 0;
		int permanents[1000];
		int p_count = 0;
		for(i=0;i<active_cards_count[HUMAN];i++){
			if( in_play(HUMAN, i) && get_id(HUMAN, i) && is_what(HUMAN, i, TYPE_PERMANENT) ){
				permanents[p_count] = i;
				p_count++;
			}
		}
		if( p_count > 0 ){
			int r = internal_rand(count);
			if( challenge4 == 0 ){
				if( challenge_round < 3 ){
					tap_card(HUMAN, permanents[r]);
				}
				else if( challenge_round < 6 ){
						bounce_permanent(HUMAN, permanents[r]);
				}
				else{
					kill_card(HUMAN, permanents[r], KILL_BURY);
				}
			}
			if( challenge3 == 5 ){
				if( challenge_round < 5 ){
					token_generation_t token;
					copy_token_definition(AI, -1, &token, HUMAN, permanents[r]);
					token.no_sleight = 1;
					generate_token(&token);
				}
				else{
					int card_added = add_card_to_hand(AI, get_internal_card_id_from_csv_id(CARD_ID_RULES_ENGINE));
					card_instance_t* temp = get_card_instance(AI, card_added);
					temp->state |= STATE_INVISIBLE;
					--hand_count[AI];
					gain_control(AI, card_added, HUMAN, permanents[r]);
					obliterate_card(AI, card_added);
				}
			}
		}
	}

	// pit fighting
	if( challenge4 == 1 && internal_rand(3) == 1 ){
		token_generation_t token;
		default_token_definition(AI, -1, CARD_ID_PIT_SCORPION, &token);
		token.no_sleight = 1;
		generate_token(&token);
	}

}

int get_challenge2(void){
	return challenge2;
}

int get_challenge3(void){
	return challenge3;
}

int get_challenge4(void){
	return challenge4;
}

int is_beta_land(int id){
	if( id == CARD_ID_SWAMP || id == CARD_ID_ISLAND || id == CARD_ID_FOREST || id == CARD_ID_MOUNTAIN ||
		id == CARD_ID_PLAINS || id == CARD_ID_BAYOU || id == CARD_ID_TAIGA || id == CARD_ID_TROPICAL_ISLAND ||
		id == CARD_ID_TUNDRA || id == CARD_ID_VOLCANIC_ISLAND || id == CARD_ID_SAVANNAH || id == CARD_ID_PLATEAU ||
		id == CARD_ID_UNDERGROUND_SEA || id == CARD_ID_BADLANDS || id == CARD_ID_SCRUBLAND  ){
		return 1;
	}
	return 0;
}

int is_challenge_card(int id){
	int i;
	for(i=0;i<40;i++){
		if( lock_ids[i] == id ){
			return 1;
		}
	}
	return 0;
}

int from_1995(int id){
	if( id <= CARD_ID_RUBINIA_SOULSINGER ){
		return 1;
	}
	if(
		id == CARD_ID_MEMORY_LAPSE ||
		id == CARD_ID_ANABA_SHAMAN ||
		id == CARD_ID_MERCHANT_SCROLL ||
		id == CARD_ID_IHSANS_SHADE ||
		id == CARD_ID_CEMETERY_GATE ||
		id == CARD_ID_GREATER_WEREWOLF ||
		id == CARD_ID_MARJHAN ||
		id == CARD_ID_RETRIBUTION ||
		id == CARD_ID_SEA_SPRITE ||
		id == CARD_ID_WILLOW_FAERIE
	  ){
		return 1;
	}
	if( id >= CARD_ID_ABBEY_GARGOYLES && id <= CARD_ID_WILLOW_PRIESTESS ){
		return 1;
	}
	if( id >= CARD_ID_ABBEY_MATRON && id <= CARD_ID_WIZARDS_SCHOOL ){
		return 1;
	}
	if( id >= CARD_ID_AEOLIPILE && id <= CARD_ID_ZELYON_SWORD ){
		return 1;
	}
	if( id>=CARD_ID_ORDER_OF_LEITBUR && id <= CARD_ID_ZELYON_SWORD ){
		return 1;
	}
	return 0;
}

void check_challenge2_deck_legality(void){
	int *deck = deck_ptr[HUMAN];
	int i, j;
	int result = -1;

	if( challenge2 == 4 ){
		// deck too small
		if( count_deck(HUMAN) < 85 ){
			result = 0;
		}
		for(i=0;i<count_deck(HUMAN);i++){
			for(j=i+1;j<count_deck(HUMAN);j++){
				if( deck[i] == deck[j] ){
					result = i;
					break;
				}
			}
			if( result > -1 ){
				break;
			}
		}
	}

	if( challenge4 == 8 ){
		// deck too small
		if( count_deck(HUMAN) < 85 ){
			result = 0;
		}
		for(i=0;i<count_deck(HUMAN);i++){
			for(j=i+1;j<count_deck(HUMAN);j++){
				if( ! is_basic_land_by_id(cards_data[deck[i]].id) ){
					if( deck[i] == deck[j] ){
						result = i;
						break;
					}
				}
			}
			if( result > -1 ){
				break;
			}
		}
	}

	if( result == -1 ){
		int exception = 0;
		int counts[10];
		int main_color = 0;
		for(i=0;i<10;i++){
			counts[i] = 0;
		}
		for(i=0;i<count_deck(HUMAN);i++){
			// Check challenge 1 decks
			if( gauntlet_round >= 71 && ! is_beta_land( cards_data[ deck[i] ].id )  ){
				result = i;
			}

			// Check challenge2 decks
			if( challenge2 == 5 && ! ( cards_data[ deck[i] ].type & TYPE_ARTIFACT ) ){
				result = i;
			}
			else if( challenge2 == 6 && ! ( cards_data[ deck[i] ].type & TYPE_LAND ) ){
				result = i;
			}
			else if( challenge2 == 7 && ! ( cards_data[ deck[i] ].type & TYPE_CREATURE ) ){
				result = i;
			}
			else if( challenge2 == 2 && ( cards_data[ deck[i] ].cc[0] + cards_data[ deck[i] ].cc[1] < 4 ) && ! ( cards_data[ deck[i] ].type & TYPE_LAND ) ){
				result = i;
			}

			// check challenge 3 decks
			if( challenge3 == 0 && !( is_what(-1, deck[i], TYPE_CREATURE) && is_what(-1, deck[i], TYPE_ARTIFACT)) ){
				card_ptr_t* c2 = cards_ptr[ cards_data[ deck[i] ].id ];
				char buffer[50];
				sprintf(buffer, " Illegal card (type): \n %s", c2->name);
				do_dialog(0,0,0, -1, -1, buffer, 0);
				result = i;
			}
			else if( challenge3 == 1 && !is_what(-1, deck[i], TYPE_ENCHANTMENT) && ! is_beta_land( cards_data[ deck[i] ].id )  ){
				result = i;
			}
			else if( challenge3 == 2 && ! ( cards_data[ deck[i] ].type & TYPE_SORCERY ) && ! is_beta_land( cards_data[ deck[i] ].id )  ){
				result = i;
			}
			else if( challenge3 == 3 && cards_data[ deck[i] ].cc[2] != 9  && ! is_beta_land( cards_data[ deck[i] ].id )  ){
				exception++;
				if( exception > 6 ){
					result = i;
				}
			}
			else if (challenge3 == 4 && !is_what(-1, deck[i], TYPE_INSTANT | TYPE_INTERRUPT) && !is_beta_land(cards_data[deck[i]].id)){
					result = i;
			}
			else if( challenge3 == 6 && ! ( cards_data[ deck[i] ].type & TYPE_ARTIFACT ) && ! is_beta_land( cards_data[ deck[i] ].id )  ){
					result = i;
			}
			else if( challenge3 == 6 && ( cards_data[ deck[i] ].type & TYPE_CREATURE )  ){
				result = i;
			}
			else if( challenge3 == 7 && ! is_beta_land( cards_data[ deck[i] ].id ) ){
				int cmc = cards_data[ deck[i] ].cc[0] + cards_data[ deck[i] ].cc[1];
				if( cmc < 1 || cmc > 10 ){
					result = i;
				}
				else{
					counts[cmc-1]++;
				}
			}

			// check challenge 4 decks
			card_ptr_t* c = cards_ptr[ cards_data[ deck[0] ].id ];
			card_ptr_t* c1 = cards_ptr[ cards_data[ deck[i] ].id ];
			if( challenge4 == 4 && memcmp(c->name, c1->name, 1 ) != 0 ){
				result = i;
			}
			else if( challenge4 == 5 && ! ( is_beta_land( cards_data[ deck[i] ].id) || is_challenge_card( cards_data[ deck[i] ].id ) ) ){
				result = i;
			}
			else if( challenge4 == 7 ){
					if( cards_data[ deck[i] ].type & TYPE_LAND ){
						if(  ! is_beta_land( cards_data[ deck[i] ].id)  ){
							result = i;
						}
					}
					else{
						if( main_color == 0 && cards_data[ deck[i] ].color != 1 ){
							main_color = cards_data[ deck[i] ].color;
						}
						else{
							if( cards_data[ deck[i] ].color != 1 && cards_data[ deck[i] ].color != main_color ){
								char buffer[50];
								sprintf(buffer, " Illegal card (color): \n %s", c1->name);
								do_dialog(0,0,0, -1, -1, buffer, 0);
								result = i;
							}
						}
					}

					if( c1->rarity != 4 && c1->rarity != 1 ){
						char buffer[50];
						sprintf(buffer, " Illegal card (rarity): \n %s", c1->name);
						do_dialog(0,0,0, -1, -1, buffer, 0);
						result = i;
					}
			}
			else if( challenge4 == 9 && ! from_1995(cards_data[deck[i]].id) ){
					char buffer[50];
					sprintf(buffer, "Illegal card (expansion): \n%s", c1->name);
					do_dialog(0,0,0, -1, -1, buffer, 0);
					result = i;
			}
		}

		if( challenge3 == 7 && result == -1 ){
			// update the counts for human's hand too
			for( i=0;i<active_cards_count[HUMAN];i++){
				card_instance_t *instance = get_card_instance(HUMAN, i);
				int id = instance->internal_card_id;
				int cmc = cards_data[ id ].cc[0] + cards_data[ id ].cc[1];
				if( cmc > 0 && cmc < 11 ){
					counts[cmc-1]++;
				}
			}
			result = -1;
			for(i=0;i<10;i++){
				if( counts[i] != 4 ){
					result = 12;
				}
			}
		}
	}

	if( result > -1 ){
		lose_the_game(HUMAN);
	}
}

void set_challenge_round_to_0(void){
	challenge_round = 0;
}
void set_challenge_round(void){
	// jump ahead if we already completed some rounds
	int choice = 0;
	int player= HUMAN;
	if( gauntlet_round == 0 ){ challenge_round = 0; }
	if( challenge_round == 0 ){
		int challenges_unlocked[4] = {0, 0, 0, 0};
		int challenges_numbers[4] = {0, 0, 0, 0};
		int q,z;
		for(q=0; q<4; q++){
			for(z=0; z<10; z++){
				if( q == 0 ){
					if( z == 0 ){
						z++;
					}
				}
				if( lock_ids[(q*10)+z] != -2 ){
					challenges_numbers[q]++;
					if( is_unlocked_by_challenge(player, (q*10)+z) ){
						challenges_unlocked[q] |= (1<<z);
					}
				}
				else{
					break;
				}
			}
		}

		char msg[1000];
		int pos = 0;
		for(q=0; q<4; q++){
			pos += scnprintf(msg + pos, 1000-pos, " Challenge %d (%d unlocked of %d)\n", q+1, num_bits_set(challenges_unlocked[q]), challenges_numbers[q]);
		}
		choice = do_dialog(player, 0, 0, -1, -1, msg, 0);

		char buffer[1000];
		pos = 0;
		for(z=0; z<challenges_numbers[choice]; z++){
			if( choice == 0 && z == 0 ){
				z++;
			}
			pos += scnprintf(buffer + pos, 1000-pos, " %s ", challenge_names[(choice*10)+z]);
			if( challenges_unlocked[choice] & (1<<z) ){
				pos += scnprintf(buffer + pos, 1000-pos, "(unlocked)");
			}
			pos += scnprintf(buffer + pos, 1000-pos, "\n");
		}
		if( choice == 0 ){
			choice = do_dialog(player, 0,0, -1, -1, buffer, 0);
			challenge1 = choice;
			challenge2 = challenge3 = challenge4 = -1;
			challenge_round = 10 * choice;
			if( challenge_round > 20 && choice != 7 && choice != 8 ){ challenge_round--; }

			if( choice == 8 ){
				check_challenge2_deck_legality();
			}

			if( challenge_round > 0 ){
				gauntlet_round = challenge_round + 1;
			}
		}
		else if( choice == 1 ){
				choice = do_dialog(player, 0,0, -1, -1, buffer, 0);
				challenge2 = choice;
				challenge1 = challenge3 = challenge4 = -1;
				check_challenge2_deck_legality();
		}
		else if( choice == 2 ){
				choice = do_dialog(player, 0,0, -1, -1, buffer, 0);
				challenge3 = choice;
				challenge1 = challenge2 = challenge4 = -1;
				check_challenge2_deck_legality();
		}
		else if( choice == 3 ){
				choice = do_dialog(player, 0,0, -1, -1, buffer,  0);
				challenge4 = choice;
				challenge1 = challenge2 = challenge3 = -1;
				check_challenge2_deck_legality();
		}
	}
}

void challenge_mode_upkeep(int from_vanguard){
	if( turn_count != 1){
		return;
	}

	// super mulligan
	if( challenge2 == 0){
		gain_life(HUMAN, 4);
		int count = 0;
		while( count < active_cards_count[HUMAN]){
			if(! in_play(HUMAN, count) ){
				put_on_top_of_deck(HUMAN, count);
			}
			count++;
		}
		draw_cards(HUMAN, 2);
	}

	// for the first 10 games, lose 2 life per game
	if( challenge_round >= 20 && challenge_round < 29 ){
		life[HUMAN] -= 2+2*(challenge_round%10);
		life[AI] += 2+2*(challenge_round%10);
	}

	// in the shandalar rounds, mess up life totals
	if( challenge_round >= 39 && challenge_round < 44 ){
		life[HUMAN] = 10;
		life[AI] = 50;
	}

	// set the decks for each round
	if( challenge_round < 10 && challenge2 == -1 && challenge3 == -1 && challenge4 == -1 ){
		set_opponent_deck( -(10+challenge_round + 1) );
	}
	else if( challenge4 == 1 && challenge_round == 0 ){
		set_opponent_deck( CD_KOBOLDS );
	}
	else if( challenge4 == 1 && challenge_round == 1 ){
		set_opponent_deck( CD_FAERIES );
	}
	else if( challenge4 == 1 && challenge_round == 2 ){
		set_opponent_deck( CD_RATS );
	}
	else if( challenge4 == 1 && challenge_round == 3 ){
		set_opponent_deck( CD_ZOMBIES );
	}
	else if( challenge4 == 1 && challenge_round == 4 ){
		set_opponent_deck( CD_SPAWNWRITHER );
	}
	else if( challenge4 == 1 && challenge_round == 5 ){
		set_opponent_deck( CD_SOLDIERS );
	}
	else if( challenge4 == 1 && challenge_round == 6 ){
		set_opponent_deck( CD_BURN );
	}
	else if( challenge4 == 1 && challenge_round == 7 ){
		set_opponent_deck( CD_RATS );
	}
	else if( challenge3 == 9  ){
		set_opponent_deck( CD_FAST_SLIVERS );
	}
	else if( challenge_round == 8 && challenge3 > -1 && challenge3 != 5  && challenge3 != 0 ){
		set_opponent_deck( CD_CLONE_HUMAN_DECK );
	}
	else if( challenge2 == 8 ){
			set_opponent_deck( CD_HYDRA );
	}

	// enchanted challenge
	else if( challenge4 == 6 && challenge_round == 0 ){
		set_opponent_deck( CD_TURBOFOG );
	}
	else if( challenge4 == 6 && challenge_round == 1 ){
		set_opponent_deck( CD_VISELING );
	}
	else if( challenge4 == 6 && challenge_round == 2 ){
		set_opponent_deck( CD_SPAWNWRITHER );
	}
	else if( challenge4 == 6 && challenge_round == 3 ){
		set_opponent_deck( CD_RATS );
	}
	else if( challenge4 == 6 && challenge_round == 4 ){
		set_opponent_deck( CD_VAMPIRES );
	}
	else if( challenge4 == 6 && challenge_round == 5 ){
		set_opponent_deck( CD_BIG_BURN );
	}
	else if( challenge4 == 6 && challenge_round == 6 ){
		set_opponent_deck( CD_NO_LAND );
	}
	else if( challenge4 == 6 && challenge_round == 7 ){
		set_opponent_deck( CD_BURN );
	}
	else if( challenge4 == 6 && challenge_round == 8 ){
		set_opponent_deck( CD_CLONE_HUMAN_DECK );
	}
	else if( challenge4 == 7 && challenge_round == 8 ){
		set_opponent_deck( CD_CLONE_HUMAN_DECK );
	}


	else if( challenge_round == 11 ){
		set_opponent_deck( CD_GROW );
	}
	else if( challenge_round == 13 ){
		set_opponent_deck( CD_SHARUUM );
	}
	else if( challenge_round == 14 ){
		set_opponent_deck( CD_SPAWNWRITHER );
	}
	else if( challenge_round == 15 ){
		set_opponent_deck( CD_RATS );
	}
	else if( challenge_round == 16 ){
		set_opponent_deck( CD_TURBOFOG );
	}
	else if( challenge_round == 17 ){
		set_opponent_deck( CD_ZOMBIES );
	}
	else if( challenge_round == 18 ){
		set_opponent_deck( CD_SOLDIERS );
	}
	else if( challenge_round == 19 ){
		set_opponent_deck( CD_GOLD_CREATURES );
	}
	else if( challenge_round == 28 ){
		set_opponent_deck( CD_2_LAND_WHITE );
	}
	else if( challenge_round == 30 ){
		set_opponent_deck( CD_SPAWNWRITHER );
	}
	else if( challenge_round == 32 ){
		set_opponent_deck( CD_RATS );
	}
	else if( challenge_round == 34 ){
		set_opponent_deck( CD_KOBOLDS );
	}
	else if( challenge_round == 37 ){
		set_opponent_deck( CD_TURBOFOG );
	}
	else if( challenge_round == 38 ){
		set_opponent_deck( CD_ALL_MOUNTAINS );
	}
	else if( challenge_round == 39 ){
		set_opponent_deck( CD_MERFOLKS );
	}
	else if( challenge_round == 40 ){
		set_opponent_deck( CD_FAERIES );
	}
	else if( challenge_round == 41 ){
		set_opponent_deck( CD_SOLDIERS );
	}
	else if( challenge_round == 42 ){
		set_opponent_deck( CD_ELVES );
	}
	else if( challenge_round == 43 ){
		set_opponent_deck( CD_ZOMBIES );
	}
	else if( challenge_round == 44 ){
		set_opponent_deck( CD_TURBOFOG );
	}
	else if( challenge_round == 47 ){
		set_opponent_deck( CD_KITHKIN );
	}
	else if( challenge_round == 48 ){
		set_opponent_deck( CD_ALL_MOUNTAINS );
	}
	else if( challenge_round == 49 ){
		set_opponent_deck( CD_GOLD_CREATURES );
	}
	else if( challenge_round == 50 ){
		set_opponent_deck( CD_FAERIES );
	}
	else if( challenge_round == 51 ){
		set_opponent_deck( CD_SHARUUM );
	}
	else if( challenge_round == 52 ){
		set_opponent_deck( CD_SPAWNWRITHER );
	}
	else if( challenge_round == 53 ){
		set_opponent_deck( CD_GROW );
	}
	else if( challenge_round == 54 ){
		set_opponent_deck( CD_KITHKIN );
	}
	else if( challenge_round == 55 ){
		set_opponent_deck( CD_SLIVERS );
	}
	else if( challenge_round == 56 ){
		set_opponent_deck( CD_TURBOFOG );
	}
	else if( challenge_round == 57 ){
		set_opponent_deck( CD_SOLDIERS );
	}
	else if( challenge_round == 58 ){
		set_opponent_deck( CD_NO_LAND );
	}
	else if( challenge_round > 69 && challenge_round < 80 ){
		set_opponent_deck( CD_ALL_MOUNTAINS );
	}
	else{
		set_opponent_deck( -(11+internal_rand(get_cd_number())) );
	}

	// start the game with some card in play
	int start_game_card[100];
	int i;
	for(i=0;i<100;i++){
		start_game_card[i] = -1;
	}
	start_game_card[0] = CARD_ID_KOBOLD_TASKMASTER;
	start_game_card[1] = CARD_ID_CEMETERY_REAPER;
	start_game_card[2] = CARD_ID_SCION_OF_OONA;
	start_game_card[3] = CARD_ID_WIZENED_CENN;
	start_game_card[4] = CARD_ID_MERFOLK_SOVEREIGN;
	start_game_card[5] = CARD_ID_RELENTLESS_RATS;
	start_game_card[6] = CARD_ID_ELVISH_ARCHDRUID;
	start_game_card[7] = CARD_ID_GOBLIN_WARCHIEF;
	start_game_card[8] = CARD_ID_CAPTAIN_OF_THE_WATCH;
	start_game_card[9] = CARD_ID_SLIVER_QUEEN;
	if( start_game_card[challenge_round] != -1 && challenge2 == -1 && challenge3 == -1 && challenge4 == -1 ){
		get_card( start_game_card[challenge_round] );
	}
	else if( challenge_round == 39 ){
		get_card(CARD_ID_LEVIATHAN);
		get_card(CARD_ID_SERENDIB_EFREET);
	}
	else if( challenge_round == 40 ){
		get_card(CARD_ID_SAVANNAH_LIONS);
		get_card(CARD_ID_ELDER_LAND_WURM);
	}
	else if( challenge_round == 41 ){
		get_card(CARD_ID_GOBLIN_BALLOON_BRIGADE);
		get_card(CARD_ID_MANA_FLARE);
	}
	else if( challenge_round == 42 ){
		get_card(CARD_ID_FUNGUSAUR);
		get_card(CARD_ID_SYLVAN_LIBRARY);
	}
	else if( challenge_round == 43 ){
		get_card(CARD_ID_HYPNOTIC_SPECTER);
		get_card(CARD_ID_GREED);
	}
	else if( challenge_round == 44 ){
		get_card(CARD_ID_THE_ABYSS);
		get_card(CARD_ID_PLATINUM_ANGEL);
	}
	else if( challenge_round == 45 ){
		get_card(CARD_ID_BLACK_VISE);
		get_card(CARD_ID_THE_RACK);
	}
	else if( challenge_round == 46 ){
		get_card(CARD_ID_IVORY_TOWER);
		get_card(CARD_ID_IVORY_TOWER);
	}
	else if( challenge_round == 47 ){
		get_card(CARD_ID_BLOOD_MOON);
		get_card(CARD_ID_TITANIAS_SONG);
	}
	else if( challenge_round == 48 ){
		get_card(CARD_ID_HONDEN_OF_CLEANSING_FIRE);
		get_card(CARD_ID_HONDEN_OF_SEEING_WINDS);
		get_card(CARD_ID_HONDEN_OF_LIFES_WEB);
		get_card(CARD_ID_HONDEN_OF_NIGHTS_REACH);
		get_card(CARD_ID_HONDEN_OF_INFINITE_RAGE);
		get_card(CARD_ID_DRAGON_ENGINE);
	}
	else if( challenge_round >= 59 && challenge_round < 70 ){
		get_card(CARD_ID_LEYLINE_OF_THE_VOID);
		get_card(CARD_ID_PRESENCE_OF_THE_MASTER);
		if( challenge_round == 59 ){
			get_card(CARD_ID_LILIANA_VESS);
		}
		else if( challenge_round == 60 ){
			get_card(CARD_ID_JACE_BELEREN);
		}
		else if( challenge_round == 61 ){
			get_card(CARD_ID_AJANI_GOLDMANE);
		}
		else if( challenge_round == 62 ){
			get_card(CARD_ID_CHANDRA_NALAAR);
		}
		else if( challenge_round == 63 ){
			get_card(CARD_ID_GARRUK_WILDSPEAKER);
		}
		else if( challenge_round == 64 ){
			get_card(CARD_ID_LILIANA_VESS);
			get_card(CARD_ID_JACE_BELEREN);
			get_card(CARD_ID_AJANI_GOLDMANE);
			get_card(CARD_ID_CHANDRA_NALAAR);
			get_card(CARD_ID_GARRUK_WILDSPEAKER);
		}
		else if( challenge_round == 65 ){
			get_card(CARD_ID_GIDEON_JURA);
			get_card(CARD_ID_VENSER_THE_SOJOURNER);
		}
		else if( challenge_round == 66 ){
			get_card(CARD_ID_ELSPETH_TIREL);
			get_card(CARD_ID_JACE_THE_MIND_SCULPTOR);
		}
		else if( challenge_round == 67 ){
			get_card(CARD_ID_ELSPETH_KNIGHT_ERRANT);
			get_card(CARD_ID_AJANI_VENGEANT);
		}
		else if( challenge_round == 68 ){
			get_card(CARD_ID_SORIN_MARKOV);
			get_card(CARD_ID_NICOL_BOLAS_PLANESWALKER);
		}

	}
	else if( challenge2 == 6 ){
			int card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id(CARD_ID_EXPLORATION) );
			put_into_play(HUMAN, card_added);
	}
	else if( challenge2 == 7 ){
			int card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_CITY_OF_BRASS ));
			put_into_play(HUMAN, card_added);
	}
	else if( challenge3 == 0 ){
			int card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_SOL_RING ));
			put_into_play(HUMAN, card_added);
	}
	else if( challenge3 == 9 && challenge_round == 0 ){
		get_card( CARD_ID_PLATED_SLIVER );
	}
	else if( challenge3 == 9 && challenge_round == 1 ){
		get_card( CARD_ID_MUSCLE_SLIVER );
	}
	else if( challenge3 == 9 && challenge_round == 2 ){
		get_card( CARD_ID_BLADE_SLIVER);
	}
	else if( challenge3 == 9 && challenge_round == 3 ){
		get_card( CARD_ID_BONESPLITTER_SLIVER);
	}
	else if( challenge3 == 9 && challenge_round == 4 ){
		get_card( CARD_ID_MIGHT_SLIVER);
	}
	else if( challenge3 == 9 && challenge_round == 5 ){
		get_card( CARD_ID_COAT_OF_ARMS);
	}
	else if( challenge3 == 9 && challenge_round == 6 ){
		get_card(CARD_ID_SLIVER_QUEEN);
	}
	else if( challenge3 == 9 && challenge_round == 7 ){
		get_card( CARD_ID_CRYSTALLINE_SLIVER);
		get_card( CARD_ID_WINGED_SLIVER);
	}
	else if( challenge3 == 9 && challenge_round == 8 ){
		get_card( CARD_ID_VIRULENT_SLIVER);
		get_card( CARD_ID_VIRULENT_SLIVER);
	}

	// pit challenge
	else if( challenge4 == 1 && challenge_round == 0 ){
		get_card(CARD_ID_DEATH_PIT_OFFERING);
	}
	else if( challenge4 == 1 && challenge_round == 1 ){
		get_card(CARD_ID_JARETH_LEONINE_TITAN);
	}
	else if( challenge4 == 1 && challenge_round == 2 ){
		get_card(CARD_ID_ARCANIS_THE_OMNIPOTENT);
	}
	else if( challenge4 == 1 && challenge_round == 3 ){
		get_card(CARD_ID_VISARA_THE_DREADFUL);
	}
	else if( challenge4 == 1 && challenge_round == 4 ){
		get_card(CARD_ID_SILVOS_ROGUE_ELEMENTAL);
	}
	else if( challenge4 == 1 && challenge_round == 5 ){
		get_card(CARD_ID_RORIX_BLADEWING);
	}
	else if( challenge4 == 1 && challenge_round == 6 ){
		get_card(CARD_ID_JESKA_WARRIOR_ADEPT);
	}
	else if( challenge4 == 1 && challenge_round == 7 ){
		get_card(CARD_ID_LORD_OF_THE_PIT);
		get_card( CARD_ID_PIT_SCORPION);
		get_card( CARD_ID_PIT_SCORPION);
		get_card( CARD_ID_PIT_SCORPION);
	}
	else if( challenge4 == 1 && challenge_round > 7 ){
		for(i=0;i<challenge_round-3;i++){
			get_card( CARD_ID_PIT_SCORPION);
		}
	}
	if( challenge4 == 1 ){
		get_card(CARD_ID_BLOOD_MOON);
		get_card(CARD_ID_PRESENCE_OF_THE_MASTER);
	}
	// lich challenge
	else if( challenge4 == 2 ){
		int card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_LICH ) );
		put_into_play(HUMAN, card_added);
		card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_LIBRARY_OF_LENG ) );
		put_into_play(HUMAN, card_added);
		card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_LIBRARY_OF_LENG ) );
		put_into_play(HUMAN, card_added);
	}
	// enchanting challenge
	if( challenge4 == 6 && ( challenge_round == 0 || challenge_round == 8 ) ){
		get_card( CARD_ID_LEYLINE_OF_THE_VOID);
	}
	if( challenge4 == 6 && (challenge_round == 2 || challenge_round == 8 ) ){
		get_card(CARD_ID_BLOOD_MOON);
	}
	if( challenge4 == 6 && (challenge_round == 3 || challenge_round == 8 ) ){
		get_card(CARD_ID_MOAT);
	}
	if( challenge4 == 6 && (challenge_round == 4  || challenge_round == 8 )){
		get_card( CARD_ID_ENERGY_FLUX);
		get_card( CARD_ID_ENERGY_FLUX);
		get_card( CARD_ID_ENERGY_FLUX);
		get_card( CARD_ID_ENERGY_FLUX);
	}
	if( challenge4 == 6 && (challenge_round == 5 || challenge_round == 8 ) ){
		get_card(CARD_ID_MANA_FLARE);
	}
	if( challenge4 == 6 && (challenge_round == 6 || challenge_round == 8 ) ){
		for(i=1;i<6;i++){
			int card_added = get_card(CARD_ID_KARMA);
			card_instance_t *instance = get_card_instance(AI, card_added);
			instance->hack_mode[0] = (1<<i);
		}
	}
	if( challenge4 == 6 && (challenge_round == 7 || challenge_round == 8 ) ){
		get_card(CARD_ID_DOUBLING_SEASON);
		get_card(CARD_ID_AWAKENING_ZONE);
	}
	if( challenge4 == 6 && (challenge_round == 1 || challenge_round == 8 ) ){
		get_card(CARD_ID_NETHER_VOID);
	}
	else if( challenge4 == 7 && challenge_round == 8 ){
		get_card( CARD_ID_GLORIOUS_ANTHEM );
	}

	// Archenemy's Cohort challenge
	else if( challenge4 == 8 && challenge_round == 0 ){
			 get_card( CARD_ID_ANGUS_MACKENZIE );
			 // set_opponent_deck( CD_ANGUS_MC_KENZIE );
			 set_opponent_deck( CD_MAVERICK );
	}
	else if( challenge4 == 8 && challenge_round == 1 ){
			 get_card( CARD_ID_XIRA_ARIEN );
			 set_opponent_deck( CD_XIRA_ARIEN );
	}
	else if( challenge4 == 8 && challenge_round == 2 ){
			 get_card( CARD_ID_TUKNIR_DEATHLOCK );
			 set_opponent_deck( CD_TUKNIR_DEATHLOK );
	}
	else if( challenge4 == 8 && challenge_round == 3 ){
			 get_card( CARD_ID_GLISSA_THE_TRAITOR );
			 set_opponent_deck( CD_YGGDRASIL_GB );
	}
	else if( challenge4 == 8 && challenge_round == 4 ){
			 get_card( CARD_ID_ANIMAR_SOUL_OF_ELEMENTS );
			 set_opponent_deck( CD_ANIMAR );
	}
	else if( challenge4 == 8 && challenge_round == 5 ){
			 get_card( CARD_ID_EDRIC_SPYMASTER_OF_TREST );
			 set_opponent_deck( CD_EDRIC );
	}
	else if( challenge4 == 8 && challenge_round == 6 ){
			 set_opponent_deck( CD_DORAN );
			 get_card( CARD_ID_DORAN_THE_SIEGE_TOWER );
	}
	else if( challenge4 == 8 && challenge_round == 7 ){
			 get_card( CARD_ID_KAALIA_OF_THE_VAST );
			 set_opponent_deck( CD_KAALIA );
	}
	else if( challenge4 == 8 && challenge_round == 8 ){
			   set_opponent_deck(CD_CLONE_HUMAN_DECK);
			   get_card( CARD_ID_ARCHENEMY );
	}
	else if( challenge2 == 8 ){
			// hydra
			/*
			int q;
			int shh = 2+((challenge_round+1)/2)+(get_deck_legality(-1, -1)-1);
			for(q=0; q<shh; q++){
				get_card(CARD_ID_HYDRA_HEAD);
			}
			*/
	}
	// free scorpion
	if( gauntlet_round < 70 && is_unlocked(HUMAN, 0, EVENT_CAN_CAST, 31) ){
		int card_added = add_card_to_hand( HUMAN, get_internal_card_id_from_csv_id( CARD_ID_PIT_SCORPION  ) );
		put_into_play(HUMAN, card_added);
	}

	// Archenemy's power
	if( gauntlet_round < 70 && is_unlocked(HUMAN, 0, EVENT_CAN_CAST, 38) ){
		token_generation_t token;
		default_token_definition(HUMAN, -1, CARD_ID_ARCHENEMY, &token);
		token.special_infos = 88;
		generate_token(&token);
	}

	// Back to '95
	if( gauntlet_round < 70 && is_unlocked(HUMAN, 0, EVENT_CAN_CAST, 39) ){
		token_generation_t token;
		default_token_definition(HUMAN, -1, (CARD_ID_BLACK-1)+get_deck_color(HUMAN, HUMAN), &token);
		token.special_infos = 88;
		generate_token(&token);
	}

	// Face the Hydra reward
	if( gauntlet_round < 70 && is_unlocked(HUMAN, 0, EVENT_CAN_CAST, 28) ){
		// hydra
		//generate_token_by_id(player, CARD_ID_THE_SLAYER);
	}

	// Determine the "banned" cards for the "Banned" challenge
	if( gauntlet_round > 48 && gauntlet_round < 59){
		int *deck = deck_ptr[HUMAN];
		int cd = count_deck(HUMAN);
		int rnd = internal_rand(cd);
		while( is_what(-1, deck[rnd], TYPE_LAND) ){
				rnd++;
				if( rnd > cd ){
					cd = 0;
				}
		}
		int card_added = generate_reserved_token_by_id(HUMAN, CARD_ID_SPECIAL_EFFECT);//1024
		card_instance_t *this = get_card_instance(HUMAN, card_added);
		this->targets[2].player = 1024;
		this->targets[2].card = cards_data[deck[rnd]].id;
		create_card_name_legacy(HUMAN, card_added, cards_data[deck[rnd]].id);
		if( (get_challenge_round() - 1) > 53 ){
			int rnd2 = internal_rand(cd);
			while( is_what(-1, deck[rnd2], TYPE_LAND) && cards_data[deck[rnd2]].id != rnd ){
					rnd2++;
					if( rnd2 > cd ){
						cd = 0;
					}
			}
			card_added = generate_reserved_token_by_id(HUMAN, CARD_ID_SPECIAL_EFFECT);//1024
			this = get_card_instance(HUMAN, card_added);
			this->targets[2].player = 1024;
			this->targets[2].card = cards_data[deck[rnd2]].id;
			create_card_name_legacy(HUMAN, card_added, cards_data[deck[rnd]].id);
		}
	}

	// unlock cards every 10th round or so
	int unlock_id = 0;
	if( challenge_round == 10 && challenge2 == -1  && challenge3 == -1 && challenge1 == 0 && challenge4 == -1){
		unlock_id = 1;
	}
	else if( challenge2 > -1 && challenge_round == 10 ){
		unlock_id = 10 + challenge2;
	}
	else if( challenge3  > -1 && challenge_round == 9 ){
		unlock_id = 20 + challenge3;
	}
	else if( challenge4 > -1 && challenge_round == 9 ){
		unlock_id = 30 + challenge4;
	}
	else if( challenge_round == 20 && challenge1 == 1){
		unlock_id = 2;
	}
	else if( challenge_round == 29 && challenge1 == 2){
		unlock_id = 3;
	}
	else if( challenge_round == 39 && challenge1 == 3 ){
		unlock_id = 4;
	}
	else if( challenge_round == 49 && challenge1 == 4){
		unlock_id = 5;
	}
	else if( challenge_round == 59 && challenge1 == 5){
		unlock_id = 6;
	}
	else if( challenge_round == 69 && challenge1 == 6){
		unlock_id = 7;
	}
	else if( gauntlet_round == 81 && challenge1 == 7){
		unlock_id = 8;
	}
	else if( gauntlet_round == 91 && challenge1 == 8){
		unlock_id = 9;
	}

	if( unlock_id > 0 ){
		char buffer[200];
		if( challenge4 == 1 && challenge_round == 9 ){
			scnprintf(buffer, 200, "You have unleashed the power of the pit!");
		}
		else if( challenge4 == 6 && challenge_round == 9 ){
			scnprintf(buffer, 200, "You have become an auramancer!");
		}
		else if( challenge4 == 8 && challenge_round == 9 ){
			scnprintf(buffer, 200, "You have stolen Archenemy's power!");
		}
		else if( challenge4 == 9 && challenge_round == 9 ){
			scnprintf(buffer, 200, "You have won the Five Color Magic!");
		}
		else{
			card_ptr_t* c = cards_ptr[ lock_ids[unlock_id] ];
			scnprintf(buffer, 200, "You have unlocked a card: %s", c->full_name);
		}
		do_dialog(0,0,0,-1, -1, buffer, 0);

		scnprintf(buffer, 200, "Faces//%s.jat", unlock_codes[unlock_id]);
		FILE *file = fopen(buffer, "w");
		fprintf(file, "%d", 1 );
		fclose(file);
		if( ! ( challenge4 > -1 && challenge4 == 1 && challenge_round == 9 ) ){
			lose_the_game(HUMAN);
		}
	}

	challenge_round++;
}

void add_card(int card, int quantity){
	card = get_internal_card_id_from_csv_id(card);
	int i,j;
	int *deck = deck_ptr[AI];
	for(i=0;i<500;i++){
		if( deck[i] == -1 ){
			for(j=0;j<quantity;j++){
				deck[j+i] = card;
			}
			break;
		}
	}
}

void set_opponent_deck(int deck_id){
	int i;
	int count = 0;
	while( count < active_cards_count[AI]){
		if(! in_play(AI, count) ){
			put_on_top_of_deck(AI, count);
		}
		count++;
	}

	int *deck = deck_ptr[AI];
	for(i=0;i<500;i++){
		deck[i] = -1;
	}
	if( deck_id == CD_CLONE_HUMAN_DECK ){
		// clone human deck
		count=0;
		int *deck2 = deck_ptr[HUMAN];
		while(deck2[count] != -1 ){
				deck[count] = deck2[count];
				count++;
		}
		for( i=0;i<active_cards_count[HUMAN];i++){
			if( in_hand(HUMAN, i) ){
				card_instance_t *instance = get_card_instance(HUMAN, i);
				deck[count] = instance->internal_card_id;
				count++;
			}
		}
	}
	else if( deck_id == CD_ALL_KOBOLDS ){
			count = 0;
			int fake = get_internal_card_id_from_csv_id(CARD_ID_CRIMSON_KOBOLDS);
			while( count < 200 ){
					deck[count] = fake;
					count++;
			}
	}
	else if( deck_id == CD_ALL_STRIP_MINE ){
			count = 0;
			int fake = get_internal_card_id_from_csv_id(CARD_ID_STRIP_MINE);;
			while( count < 200 ){
					deck[count] = fake;
					count++;
			}
	}
	else if( deck_id == CD_ALL_MOUNTAINS ){
			count = 0;
			int fake = get_internal_card_id_from_csv_id(CARD_ID_MOUNTAIN);
			while( count < 200 ){
					deck[count] = fake;
					count++;
			}
	}
	else{
		i = 0;
		int pos = -1;
		while( cd_list[i] != -2 ){
				if( cd_list[i] == deck_id ){
					pos = i+1;
					break;
				}
				i++;
		}
		if( pos > -1 ){
			while( cd_list[pos] > -1 ){
					add_card(cd_list[pos], cd_list[pos+1]);
					pos+=2;
			}
		}
		else{
			life[HUMAN] = 0;
		}
	}
	if( deck_id == CD_NO_LAND ){
		shuffle_vanguard(AI, 0);
		int id = get_internal_card_id_from_csv_id( CARD_ID_MOX_JET );
		for(i=0;i<60;i++){
			if( deck[i] == id ){
				deck[i] = deck[0];
				deck[0] = id;
				break;
			}
		}
	}
	else if(deck_id != CD_ALL_KOBOLDS && deck_id != CD_ALL_STRIP_MINE && deck_id != CD_ALL_MOUNTAINS){
			shuffle_vanguard(AI, 1);
	}
	draw_cards(AI, 7);
}

int card_back_to_95(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE && get_special_infos(player, card) == 88 && instance->targets[1].player != 66 ){
		return 1;
	}
	if( event == EVENT_ACTIVATE ){
		int choice = do_dialog(player, player, card, -1, -1, " Life\n Death\n Cancel", 0);
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( choice == 0 ){
				if( get_id(player, card) == CARD_ID_BLACK ){
					instance->targets[1].card = CARD_ID_WILL_O_THE_WISP;
				}
				if( get_id(player, card) == CARD_ID_BLUE ){
					instance->targets[1].card = CARD_ID_WALL_OF_AIR;
				}
				if( get_id(player, card) == CARD_ID_GREEN ){
					instance->targets[1].card = CARD_ID_WALL_OF_ICE;
				}
				if( get_id(player, card) == CARD_ID_RED ){
					instance->targets[1].card = CARD_ID_WALL_OF_STONE;
				}
				if( get_id(player, card) == CARD_ID_WHITE ){
					instance->targets[1].card = CARD_ID_WALL_OF_SWORDS;
				}
			}
			if( choice == 1 ){
				if( get_id(player, card) == CARD_ID_BLACK ){
					instance->targets[1].card = CARD_ID_TERROR;
				}
				if( get_id(player, card) == CARD_ID_BLUE ){
					instance->targets[1].card = CARD_ID_UNSUMMON;
				}
				if( get_id(player, card) == CARD_ID_GREEN ){
					instance->targets[1].card = CARD_ID_HURRICANE;
				}
				if( get_id(player, card) == CARD_ID_RED ){
					instance->targets[1].card = CARD_ID_LIGHTNING_BOLT;
				}
				if( get_id(player, card) == CARD_ID_WHITE ){
					instance->targets[1].card = CARD_ID_SWORDS_TO_PLOWSHARES;
				}
			}
			instance->targets[1].player = 66;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int fake = get_internal_card_id_from_csv_id(instance->targets[1].card);
		int card_added = add_card_to_hand(player, fake);
		if( is_what(-1, fake, TYPE_PERMANENT) ){
			put_into_play(player, card_added);
		}
	}
	return 0;
}

