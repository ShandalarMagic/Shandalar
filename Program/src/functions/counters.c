#include <limits.h>

#include "manalink.h"

int hack_silent_counters = 0;
int counters_added = 0;	// backed up and saved

/* Relevant data in card_instance_t:
 * special_counters: number of "special" counters (preferred type determined by card csvid; stored in special_counter_type)
 * counters2: drawn in upper right of lower section.  Originally -0/-2 counters for Spirit Shackle; now arbitrary.
 * counters3: drawn in upper center of lower section.  Originally -1/-1 counters for Unstable Mutation; now arbitrary.
 * counters4: drawn in upper left of lower section.  Originally -0/-1 counters for Orcish Catapult; now arbitrary.
 * counters: drawn lower left of lower section.  Always +1/+1 counters (originally using text from Dwarven Weaponsmith).
 * counters5: drawn in lower right of lower section.  Originally +1/+1 counters from Ashnod's Transmogrant; now arbitray.
 * counters_m1m1: drawn in lower left of lower section with +1/+1 counters.  Originally unused.  Always -1/-1 counters.
 * targets[18].player: byte0: type of counters2.  byte1: type of counters3.  byte2: type of counters4.  byte3: type of counters5.
 * targets[18].card: byte0: non-displayed counters slot A.  byte1: non-displayed counters slot B.  byte2: type of byte0.  byte3: type of byte1.
 */

static void play_counters_sound(wav_t wav){
	if (hack_silent_counters <= 0){
		play_sound_effect(wav);
	}
}

int get_counter_type_by_id(int csvid)
{
  // 0x4d4ca0

  switch (csvid)
	{
	  case CARD_ID_ABOROTH:
	  case CARD_ID_ANCESTRAL_KNOWLEDGE:
	  case CARD_ID_ARCTIC_NISHOBA:
	  case CARD_ID_ARCTIC_WOLVES:///
	  case CARD_ID_ARNJLOTS_ASCENT:
	  case CARD_ID_BALDUVIAN_FALLEN:
	  case CARD_ID_BALDUVIAN_SHAMAN:	// on others///
	  case CARD_ID_BLIZZARD:
	  case CARD_ID_BRAID_OF_FIRE:
	  case CARD_ID_BRAND_OF_ILL_OMEN:
	  case CARD_ID_BREATH_OF_DREAMS:	// on others///
	  case CARD_ID_COLD_SNAP:
	  case CARD_ID_CORROSION:	// plus COUNTER_RUST on others
	  case CARD_ID_COVER_OF_WINTER:
	  case CARD_ID_DECOMPOSITION:	// on others///
	  case CARD_ID_DYSTOPIA:///
	  case CARD_ID_EARTHEN_GOO:
	  case CARD_ID_ELEPHANT_GRASS:
	  case CARD_ID_ENERGY_STORM:
	  case CARD_ID_FIRESTORM_HELLKITE:
	  case CARD_ID_FLOW_OF_MAGGOTS:
	  case CARD_ID_FREYALISES_RADIANCE:///
	  case CARD_ID_FYNDHORN_POLLEN:
	  case CARD_ID_GALLOWBRAID:
	  case CARD_ID_GLACIAL_CHASM:
	  case CARD_ID_GLACIAL_PLATING:
	  case CARD_ID_HALLS_OF_MIST:///
	  case CARD_ID_HEART_OF_BOGARDAN:
	  case CARD_ID_HEAT_WAVE:///
	  case CARD_ID_HERALD_OF_LESHRAC:
	  case CARD_ID_HIBERNATIONS_END:
	  case CARD_ID_ILLUSIONARY_FORCES:
	  case CARD_ID_ILLUSIONARY_PRESENCE:///
	  case CARD_ID_ILLUSIONARY_TERRAIN:
	  case CARD_ID_ILLUSIONARY_WALL:
	  case CARD_ID_ILLUSIONS_OF_GRANDEUR:
	  case CARD_ID_INFERNAL_DARKNESS:///
	  case CARD_ID_INNER_SANCTUM:
	  case CARD_ID_JOTUN_GRUNT:
	  case CARD_ID_JOTUN_OWL_KEEPER:
	  case CARD_ID_JUJU_BUBBLE:
	  case CARD_ID_KARPLUSAN_MINOTAUR:
	  case CARD_ID_KJELDORAN_JAVELINEER:
	  case CARD_ID_KROVIKAN_WHISPERS:
	  case CARD_ID_MADDENING_WIND:
	  case CARD_ID_MAGMATIC_CORE:
	  case CARD_ID_MALIGNANT_GROWTH:	// plus COUNTER_GROWTH
	  case CARD_ID_MANA_CHAINS:	// on others///
	  case CARD_ID_MESMERIC_TRANCE:
	  case CARD_ID_MIND_HARNESS:///
	  case CARD_ID_MORINFEN:
	  case CARD_ID_MUSICIAN:	// plus COUNTER_MUSIC on others
	  case CARD_ID_MWONVULI_OOZE:///
	  case CARD_ID_MYSTIC_MIGHT:///
	  case CARD_ID_MYSTIC_REMORA:
	  case CARD_ID_NAKED_SINGULARITY:///
	  case CARD_ID_PHOBIAN_PHANTASM:
	  case CARD_ID_PHYREXIAN_ETCHINGS:
	  case CARD_ID_PHYREXIAN_SOULGORGER:
	  case CARD_ID_POLAR_KRAKEN:
	  case CARD_ID_PRISMATIC_CIRCLE:///
	  case CARD_ID_PSYCHIC_VORTEX:
	  case CARD_ID_REALITY_TWIST:///
	  case CARD_ID_REVERED_UNICORN:
	  case CARD_ID_RITUAL_OF_SUBDUAL:///
	  case CARD_ID_RONOM_HULK:///
	  case CARD_ID_ROYAL_DECREE:///
	  case CARD_ID_SHELTERING_ANCIENT:
	  case CARD_ID_SNOWFALL:///
	  case CARD_ID_SOLDEVI_SIMULACRUM:
	  case CARD_ID_SPLINTER_TOKEN:
	  case CARD_ID_SURVIVOR_OF_THE_UNSEEN:///
	  case CARD_ID_SUSTAINING_SPIRIT:
	  case CARD_ID_THOUGHT_LASH:
	  case CARD_ID_TIDAL_CONTROL:///
	  case CARD_ID_TOMBSTONE_STAIRWELL:
	  case CARD_ID_TORNADO:	// plus COUNTER_VELOCITY
	  case CARD_ID_UKTABI_EFREET:
	  case CARD_ID_VARCHILDS_WAR_RIDERS:
	  case CARD_ID_VEXING_SPHINX:
	  case CARD_ID_VOLUNTEER_RESERVES:///
	  case CARD_ID_WALL_OF_SHARDS:
	  case CARD_ID_WAVE_OF_TERROR:
	  case CARD_ID_YAVIMAYA_ANTS:
		return COUNTER_AGE;

	  case CARD_ID_HANKYU:					return COUNTER_AIM;///
	  case CARD_ID_ARCHERY_TRAINING:		return COUNTER_ARROW;///
	  case CARD_ID_SERRATED_ARROWS:			return COUNTER_ARROWHEAD;

	  case CARD_ID_FIVE_ALARM_FIRE:
		return COUNTER_BLAZE;

	  case CARD_ID_BLOODLETTER_QUILL:
	  case CARD_ID_RAKDOS_RITEKNIFE:
		return COUNTER_BLOOD;

	  case CARD_ID_BOUNTY_HUNTER:			return COUNTER_BOUNTY;///
	  case CARD_ID_GWAFA_HAZID_PROFITEER:	return COUNTER_BRIBERY;///
	  case CARD_ID_OSAI_VULTURES:			return COUNTER_CARRION;

	  case CARD_ID_ETHER_VIAL:	// AEther Vial
	  case CARD_ID_ALTAR_OF_SHADOWS:
	  case CARD_ID_ANGELHEART_VIAL:///
	  case CARD_ID_ARCANE_SPYGLASS:///
	  case CARD_ID_ASTRAL_CORNUCOPIA:
	  case CARD_ID_BANSHEES_BLADE:
	  case CARD_ID_BATON_OF_COURAGE:///
	  case CARD_ID_BLACK_MANA_BATTERY:
	  case CARD_ID_BLACK_MARKET:
	  case CARD_ID_BLUE_MANA_BATTERY:
	  case CARD_ID_CHALICE_OF_THE_VOID:
	  case CARD_ID_CHIMERIC_EGG:
	  case CARD_ID_CHIMERIC_MASS:
	  case CARD_ID_CLEARWATER_GOBLET:
	  case CARD_ID_COALITION_RELIC:
	  case CARD_ID_CONVERSION_CHAMBER:
	  case CARD_ID_CULLING_DAIS:
	  case CARD_ID_DARKSTEEL_REACTOR:
	  case CARD_ID_DOOR_OF_DESTINIES:
	  case CARD_ID_DRUIDS_REPOSITORY:
	  case CARD_ID_ENGINEERED_EXPLOSIVES:
	  case CARD_ID_ETERNITY_VESSEL:
	  case CARD_ID_EVERFLOWING_CHALICE:
	  case CARD_ID_GEMSTONE_ARRAY:
	  case CARD_ID_GOLDEN_URN:
	  case CARD_ID_GOLEM_FOUNDRY:
	  case CARD_ID_GREEN_MANA_BATTERY:
	  case CARD_ID_GRINDCLOCK:
	  case CARD_ID_HELIOPHIAL:///
	  case CARD_ID_ICE_CAULDRON:///
	  case CARD_ID_INFUSED_ARROWS:///
	  case CARD_ID_JEWELED_AMULET:///
	  case CARD_ID_JINXED_CHOKER:///
	  case CARD_ID_KILNSPIRE_DISTRICT:
	  case CARD_ID_KYREN_TOY:
	  case CARD_ID_LIGHTNING_COILS:
	  case CARD_ID_LIGHTNING_REAVER:
	  case CARD_ID_LIGHTNING_STORM:	// while on the stack///
	  case CARD_ID_LUX_CANNON:
	  case CARD_ID_MAGISTRATES_SCEPTER:
	  case CARD_ID_MANA_BLOOM:
	  case CARD_ID_MANA_CACHE:///
	  case CARD_ID_MIRRODINS_CORE:
	  case CARD_ID_NECROGEN_CENSER:
	  case CARD_ID_OPALINE_BRACERS:
	  case CARD_ID_OROCHI_HATCHERY:
	  case CARD_ID_OTHERWORLD_ATLAS:
	  case CARD_ID_PENTAD_PRISM:
	  case CARD_ID_RATCHET_BOMB:
	  case CARD_ID_RED_MANA_BATTERY:
	  case CARD_ID_RIPTIDE_REPLICATOR:
	  case CARD_ID_SERUM_TANK:
	  case CARD_ID_SHRIEKHORN:
	  case CARD_ID_SHRINE_OF_BOUNDLESS_GROWTH:
	  case CARD_ID_SHRINE_OF_BURNING_RAGE:
	  case CARD_ID_SHRINE_OF_LIMITLESS_POWER:
	  case CARD_ID_SHRINE_OF_LOYAL_LEGIONS:
	  case CARD_ID_SHRINE_OF_PIERCING_VISION:
	  case CARD_ID_SIGIL_OF_DISTINCTION:
	  case CARD_ID_SPAWNING_PIT:///
	  case CARD_ID_SPHERE_OF_THE_SUNS:
	  case CARD_ID_SPHINX_BONE_WAND:
	  case CARD_ID_SUN_DROPLET:
	  case CARD_ID_SURGE_NODE:
	  case CARD_ID_SURRAKAR_SPELLBLADE:
	  case CARD_ID_TALON_OF_PAIN:
	  case CARD_ID_TENDO_ICE_BRIDGE:
	  case CARD_ID_TITAN_FORGE:
	  case CARD_ID_TRIGON_OF_CORRUPTION:
	  case CARD_ID_TRIGON_OF_INFESTATION:
	  case CARD_ID_TRIGON_OF_MENDING:
	  case CARD_ID_TRIGON_OF_RAGE:
	  case CARD_ID_TRIGON_OF_THOUGHT:
	  case CARD_ID_TUMBLE_MAGNET:
	  case CARD_ID_UMEZAWAS_JITTE:
	  case CARD_ID_VENTIFACT_BOTTLE:
	  case CARD_ID_VIVID_CRAG:
	  case CARD_ID_VIVID_CREEK:
	  case CARD_ID_VIVID_GROVE:
	  case CARD_ID_VIVID_MARSH:
	  case CARD_ID_VIVID_MEADOW:
	  case CARD_ID_WHITE_MANA_BATTERY:
		return COUNTER_CHARGE;

#ifdef UNSETS
	  case CARD_ID_B_I_N_G_O:				return COUNTER_CHIP;///
#endif

	  case CARD_ID_SCAVENGING_GHOUL:		return COUNTER_CORPSE;
	  case CARD_ID_ICATIAN_MONEYCHANGER:	return COUNTER_CREDIT;
	  case CARD_ID_PRISM_ARRAY:				return COUNTER_CRYSTAL;
	  case CARD_ID_DELIFS_CUBE:				return COUNTER_CUBE;
	  case CARD_ID_TRADE_CARAVAN:			return COUNTER_CURRENCY;
	  case CARD_ID_BOGARDAN_PHOENIX:		return COUNTER_DEATH;

	  case CARD_ID_ERTAIS_MEDDLING:	// on others, while in exile///
	  case CARD_ID_DELAYING_SHIELD:
		return COUNTER_DELAY;

	  case CARD_ID_DECREE_OF_SILENCE:
	  case CARD_ID_FORCE_BUBBLE:///
	  case CARD_ID_HICKORY_WOODLOT:
	  case CARD_ID_LAND_CAP:
	  case CARD_ID_LAVA_TUBES:
	  case CARD_ID_PEAT_BOG:
	  case CARD_ID_REMOTE_FARM:
	  case CARD_ID_RIVER_DELTA:
	  case CARD_ID_SANDSTONE_NEEDLE:
	  case CARD_ID_SAPRAZZAN_SKERRY:
	  case CARD_ID_TIMBERLINE_RIDGE:
	  case CARD_ID_VELDT:
		return COUNTER_DEPLETION;

	  case CARD_ID_DESCENT_INTO_MADNESS:	return COUNTER_DESPAIR;

	  case CARD_ID_BLOODTHIRSTY_OGRE:
	  case CARD_ID_PIOUS_KITSUNE:
		return COUNTER_DEVOTION;

	  case CARD_ID_MYOJIN_OF_CLEANSING_FIRE:
	  case CARD_ID_MYOJIN_OF_INFINITE_RAGE:
	  case CARD_ID_MYOJIN_OF_LIFES_WEB:
	  case CARD_ID_MYOJIN_OF_NIGHTS_REACH:
	  case CARD_ID_MYOJIN_OF_SEEING_WINDS:
		return COUNTER_DIVINITY;

	  case CARD_ID_ARMAGEDDON_CLOCK:
	  case CARD_ID_YOUR_INESCAPABLE_DOOM:
		return COUNTER_DOOM;

	  case CARD_ID_RASPUTIN_DREAMWEAVER:	return COUNTER_DREAM;
	  case CARD_ID_SOUL_ECHO:				return COUNTER_ECHO;///
	  case CARD_ID_ESSENCE_BOTTLE:			return COUNTER_ELIXIR;
	  case CARD_ID_ENERGY_VORTEX:			return COUNTER_ENERGY;///
	  case CARD_ID_MAGOSI_THE_WATERVEIL:	return COUNTER_EON;
	  case CARD_ID_JAR_OF_EYEBALLS:			return COUNTER_EYEBALL;

	  case CARD_ID_ANCIENT_HYDRA:
	  case CARD_ID_BLASTODERM:
	  case CARD_ID_CLOUDSKATE:
	  case CARD_ID_DEFENDER_EN_VEC:
	  case CARD_ID_JOLTING_MERFOLK:
	  case CARD_ID_PARALLAX_DEMENTIA:///
	  case CARD_ID_PARALLAX_NEXUS:
	  case CARD_ID_PARALLAX_TIDE:
	  case CARD_ID_PARALLAX_WAVE:
	  case CARD_ID_PHYREXIAN_PROWLER:///
	  case CARD_ID_REJUVENATION_CHAMBER:///
	  case CARD_ID_RUSTING_GOLEM:///
	  case CARD_ID_SAPROLING_BURST:
	  case CARD_ID_SKYSHROUD_BEHEMOTH:
	  case CARD_ID_SKYSHROUD_RIDGEBACK:
	  case CARD_ID_TANGLE_WIRE:
	  case CARD_ID_WOODRIPPER:
		return COUNTER_FADE;

	  case CARD_ID_AVEN_MIMEOMANCER:///
	  case CARD_ID_KANGEE_AERIE_KEEPER:
	  case CARD_ID_SOULCATCHERS_AERIE:
		return COUNTER_FEATHER;

	  case CARD_ID_AZORS_ELOCUTORS:			return COUNTER_FILIBUSTER;
	  case CARD_ID_NAAR_ISLE:				return COUNTER_FLAME;
	  case CARD_ID_SPOROGENESIS:			return COUNTER_FUNGUS;	// on others///

#ifdef UNSETS
	  case CARD_ID_TEMP_OF_THE_DAMNED:		return COUNTER_FUNK;///
#endif

	  case CARD_ID_GOBLIN_BOMB:
	  case CARD_ID_INCENDIARY:///
	  case CARD_ID_POWDER_KEG:
		return COUNTER_FUSE;

	  case CARD_ID_BRIBERS_PURSE:			return COUNTER_GEM;
	  case CARD_ID_MOMENTUM:				return COUNTER_GROWTH;///

	  case CARD_ID_LUDEVICS_TEST_SUBJECT:
	  case CARD_ID_TRIASSIC_EGG:
		return COUNTER_HATCHLING;

	  case CARD_ID_FYLGJA:///
	  case CARD_ID_URSINE_FYLGJA:///
		return COUNTER_HEALING;

	  case CARD_ID_HOOFPRINTS_OF_THE_STAG:	return COUNTER_HOOFPRINT;
	  case CARD_ID_TEMPORAL_DISTORTION:		return COUNTER_HOURGLASS;	// on others///
	  case CARD_ID_FASTING:					return COUNTER_HUNGER;
	  case CARD_ID_NECROPOLIS_OF_AZAR:		return COUNTER_HUSK;

	  case CARD_ID_DARK_DEPTHS:
	  case CARD_ID_ICEBERG:
	  case CARD_ID_WOOLLY_RAZORBACK:
		return COUNTER_ICE;

	  case CARD_ID_DISEASED_VERMIN:
	  case CARD_ID_FESTERING_WOUND:///
		return COUNTER_INFECTION;

	  case CARD_ID_DIVINE_INTERVENTION:		return COUNTER_INTERVENTION;
	  case CARD_ID_QUARANTINE_FIELD:		return COUNTER_ISOLATION;
	  case CARD_ID_ICATIAN_JAVELINEERS:		return COUNTER_JAVELIN;

	  case CARD_ID_BAKU_ALTAR:
	  case CARD_ID_BLADEMANE_BAKU:
	  case CARD_ID_BUDOKA_PUPIL:	case CARD_ID_ICHIGA_WHO_TOPPLES_OAKS:
	  case CARD_ID_CALLOW_JUSHI:	case CARD_ID_JARAKU_THE_INTERLOPER:
	  case CARD_ID_CUNNING_BANDIT:	case CARD_ID_AZAMUKI_TREACHERY_INCARNATE:
	  case CARD_ID_FAITHFUL_SQUIRE:	case CARD_ID_KAISO_MEMORY_OF_LOYALTY:
	  case CARD_ID_HIRED_MUSCLE:	case CARD_ID_SCARMAKER:
	  case CARD_ID_PETALMANE_BAKU:
	  case CARD_ID_QUILLMANE_BAKU:///
	  case CARD_ID_SKULLMANE_BAKU:///
	  case CARD_ID_WAXMANE_BAKU:
		return COUNTER_KI;

	  case CARD_ID_BEASTBREAKER_OF_BALA_GED:
	  case CARD_ID_BRIMSTONE_MAGE:
	  case CARD_ID_CARAVAN_ESCORT:
	  case CARD_ID_CORALHELM_COMMANDER:
	  case CARD_ID_ECHO_MAGE:
	  case CARD_ID_ENCLAVE_CRYPTOLOGIST:
	  case CARD_ID_GUUL_DRAZ_ASSASSIN:
	  case CARD_ID_HADA_SPY_PATROL:
	  case CARD_ID_HALIMAR_WAVEWATCH:
	  case CARD_ID_HEDRON_FIELD_PURISTS:
	  case CARD_ID_IKIRAL_OUTRIDER:
	  case CARD_ID_JORAGA_TREESPEAKER:
	  case CARD_ID_KABIRA_VINDICATOR:
	  case CARD_ID_KARGAN_DRAGONLORD:
	  case CARD_ID_KAZANDU_TUSKCALLER:
	  case CARD_ID_KNIGHT_OF_CLIFFHAVEN:
	  case CARD_ID_LIGHTHOUSE_CHRONOLOGIST:
	  case CARD_ID_LORD_OF_SHATTERSKULL_PASS:
	  case CARD_ID_NIRKANA_CUTTHROAT:
	  case CARD_ID_NULL_CHAMPION:
	  case CARD_ID_SKYWATCHER_ADEPT:
	  case CARD_ID_STUDENT_OF_WARFARE:
	  case CARD_ID_TRANSCENDENT_MASTER:
	  case CARD_ID_ZULAPORT_ENFORCER:
		return COUNTER_LEVEL;

	  case CARD_ID_MIND_UNBOUND:
	  case CARD_ID_MYTH_REALIZED:
	  case CARD_ID_SCROLL_OF_THE_MASTERS:
		return COUNTER_LORE;

	  case CARD_ID_AJANI_CALLER_OF_THE_PRIDE:
	  case CARD_ID_AJANI_GOLDMANE:
	  case CARD_ID_AJANI_MENTOR_OF_HEROES:
	  case CARD_ID_AJANI_STEADFAST:
	  case CARD_ID_AJANI_VENGEANT:
	  case CARD_ID_ASHIOK_NIGHTMARE_WEAVER:
	  case CARD_ID_CHANDRA_ABLAZE:
	  case CARD_ID_CHANDRA_NALAAR:
	  case CARD_ID_CHANDRA_PYROMASTER:
	  case CARD_ID_CHANDRA_ROARING_FLAME:
	  case CARD_ID_CHANDRA_THE_FIREBRAND:
	  case CARD_ID_DACK_FAYDEN:
	  case CARD_ID_DARETTI_SCRAP_SAVANT:
	  case CARD_ID_DOMRI_RADE:
	  case CARD_ID_ELSPETH_KNIGHT_ERRANT:
	  case CARD_ID_ELSPETH_SUNS_CHAMPION:
	  case CARD_ID_ELSPETH_TIREL:
	  case CARD_ID_FREYALISE_LLANOWARS_FURY:
	  case CARD_ID_GARRUK_APEX_PREDATOR:
	  case CARD_ID_GARRUK_CALLER_OF_BEASTS:
	  case CARD_ID_GARRUK_PRIMAL_HUNTER:
	  case CARD_ID_GARRUK_RELENTLESS:			case CARD_ID_GARRUK_THE_VEIL_CURSED:
	  case CARD_ID_GARRUK_WILDSPEAKER:
	  case CARD_ID_GIDEON_ALLY_OF_ZENDIKAR:
	  case CARD_ID_GIDEON_BATTLE_FORGED:
	  case CARD_ID_GIDEON_CHAMPION_OF_JUSTICE:	case CARD_ID_GIDEON_CHAMPION_ANIMATED:
	  case CARD_ID_GIDEON_JURA:					case CARD_ID_GIDEON_JURA_ANIMATED:
	  case CARD_ID_JACE_ARCHITECT_OF_THOUGHT:
	  case CARD_ID_JACE_BELEREN:
	  case CARD_ID_JACE_MEMORY_ADEPT:
	  case CARD_ID_JACE_TELEPATH_UNBOUND:
	  case CARD_ID_JACE_THE_LIVING_GUILDPACT:
	  case CARD_ID_JACE_THE_MIND_SCULPTOR:
	  case CARD_ID_KARN_LIBERATED:
	  case CARD_ID_KIORA_MASTER_OF_THE_DEPTHS:
	  case CARD_ID_KIORA_THE_CRASHING_WAVE:
	  case CARD_ID_KOTH_OF_THE_HAMMER:
	  case CARD_ID_LILIANA_DEFIANT_NECROMANCER:
	  case CARD_ID_LILIANA_OF_THE_DARK_REALMS:
	  case CARD_ID_LILIANA_OF_THE_VEIL:
	  case CARD_ID_LILIANA_VESS:
	  case CARD_ID_NAHIRI_THE_LITHOMANCER:
	  case CARD_ID_NARSET_TRANSCENDENT:
	  case CARD_ID_NICOL_BOLAS_PLANESWALKER:
	  case CARD_ID_NISSA_REVANE:
	  case CARD_ID_NISSA_SAGE_ANIMIST:
	  case CARD_ID_NISSA_WORLDWAKER:
	  case CARD_ID_OB_NIXILIS_OF_THE_BLACK_OATH:
	  case CARD_ID_OB_NIXILIS_REIGNITED:
	  case CARD_ID_RAL_ZAREK:
	  case CARD_ID_SARKHAN_THE_DRAGONSPEAKER:
	  case CARD_ID_SARKHAN_THE_MAD:
	  case CARD_ID_SARKHAN_UNBROKEN:
	  case CARD_ID_SARKHAN_VOL:
	  case CARD_ID_SORIN_LORD_OF_INNISTRAD:
	  case CARD_ID_SORIN_MARKOV:
	  case CARD_ID_SORIN_SOLEMN_VISITOR:
	  case CARD_ID_TAMIYO_THE_MOON_SAGE:
	  case CARD_ID_TEFERI_TEMPORAL_ARCHMAGE:
	  case CARD_ID_TEZZERET_AGENT_OF_BOLAS:
	  case CARD_ID_TEZZERET_THE_SEEKER:
	  case CARD_ID_TIBALT_THE_FIEND_BLOODED:
	  case CARD_ID_UGIN_THE_SPIRIT_DRAGON:
	  case CARD_ID_VENSER_THE_SOJOURNER:
	  case CARD_ID_VRASKA_THE_UNSEEN:
	  case CARD_ID_XENAGOS_THE_REVELER:
		return COUNTER_LOYALTY;

	  case CARD_ID_CHANCE_ENCOUNTER:
	  case CARD_ID_GEMSTONE_CAVERNS:
		return COUNTER_LUCK;

	  case CARD_ID_MAGNETIC_WEB:			return COUNTER_MAGNET;	// on others///
	  case CARD_ID_MINE_LAYER:				return COUNTER_MINE;	// on others///
	  case CARD_ID_GEMSTONE_MINE:			return COUNTER_MINING;
#pragma message "Cyclopean Tomb: Needs rewrite"
	  case CARD_ID_CYCLOPEAN_TOMB:			return COUNTER_MIRE;	// on others
	  case CARD_ID_ASSEMBLE_THE_LEGION:		return COUNTER_MUSTER;
	  case CARD_ID_MERSEINE:				return COUNTER_NET;
	  case CARD_ID_CELESTIAL_CONVERGENCE:	return COUNTER_OMEN;///
	  case CARD_ID_ORCISH_MINE:				return COUNTER_ORE;

	  case CARD_ID_BARRINS_CODEX:
	  case CARD_ID_PRIVATE_RESEARCH:///
		return COUNTER_PAGE;

	  case CARD_ID_TORTURE_CHAMBER:			return COUNTER_PAIN;
	  case CARD_ID_DREAD_WIGHT:				return COUNTER_PARALYZATION;	// on others///
	  case CARD_ID_LOTUS_BLOSSOM:			return COUNTER_PETAL;
	  case CARD_ID_VOODOO_DOLL:				return COUNTER_PIN;

	  case CARD_ID_PLAGUE_BOILER:
	  case CARD_ID_TRAVELING_PLAGUE:///
	  case CARD_ID_WITHERING_HEX:///
		return COUNTER_PLAGUE;

	  case CARD_ID_CORAL_REEF:				return COUNTER_POLYP;
#ifdef UNSETS
	  case CARD_ID_WATER_GUN_BALLOON_GAME:	return COUNTER_POP;///
#endif

	  case CARD_ID_HELLION_CRUCIBLE:
	  case CARD_ID_MAGMA_MINE:
	  case CARD_ID_MOUNT_KERALIA:
		return COUNTER_PRESSURE;

	  case CARD_ID_COCOON:					return COUNTER_PUPA;

	  case CARD_ID_ARCHMAGE_ASCENSION:
	  case CARD_ID_BEASTMASTER_ASCENSION:
	  case CARD_ID_BLOODCHIEF_ASCENSION:
	  case CARD_ID_IOR_RUIN_EXPEDITION:
	  case CARD_ID_KHALNI_HEART_EXPEDITION:
	  case CARD_ID_LUMINARCH_ASCENSION:
	  case CARD_ID_PYROMANCER_ASCENSION:
	  case CARD_ID_QUEST_FOR_ANCIENT_SECRETS:
	  case CARD_ID_QUEST_FOR_PURE_FLAME:///
	  case CARD_ID_QUEST_FOR_RENEWAL:
	  case CARD_ID_QUEST_FOR_ULAS_TEMPLE:
	  case CARD_ID_QUEST_FOR_THE_GEMBLADES:
	  case CARD_ID_QUEST_FOR_THE_GOBLIN_LORD:
	  case CARD_ID_QUEST_FOR_THE_GRAVELORD:
	  case CARD_ID_QUEST_FOR_THE_HOLY_RELIC:
	  case CARD_ID_QUEST_FOR_THE_NIHIL_STONE:
	  case CARD_ID_SOUL_STAIR_EXPEDITION:
	  case CARD_ID_SUNSPRING_EXPEDITION:
	  case CARD_ID_ZEKTAR_SHRINE_EXPEDITION:
		return COUNTER_QUEST;

	  case CARD_ID_ALL_HALLOWS_EVE:	// while in exile
	  case CARD_ID_ENDLESS_SCREAM:///
		return COUNTER_SCREAM;

	  case CARD_ID_ARETOPOLIS:				return COUNTER_SCROLL;
	  case CARD_ID_ROC_HATCHLING:			return COUNTER_SHELL;///
	  case CARD_ID_PALLIATION_ACCORD:		return COUNTER_SHIELD;///
#ifdef UNSETS
	  case CARD_ID_SHOE_TREE:				return COUNTER_SHOE;///
#endif
	  case CARD_ID_CEPHALID_VANDAL:			return COUNTER_SHRED;
	  case CARD_ID_CHROMATIC_ARMOR:			return COUNTER_SLEIGHT;
	  case CARD_ID_GUTTER_GRIME:			return COUNTER_SLIME;
	  case CARD_ID_SMOKESTACK:				return COUNTER_SOOT;

	  case CARD_ID_DEATHSPORE_THALLID:
	  case CARD_ID_ELVISH_FARMER:
	  case CARD_ID_FERAL_THALLID:
	  case CARD_ID_MYCOLOGIST:
	  case CARD_ID_PALLID_MYCODERM:
	  case CARD_ID_PSYCHOTROPE_THALLID:
	  case CARD_ID_SAVAGE_THALLID:
	  case CARD_ID_SPORE_FLOWER:
	  case CARD_ID_SPORESOWER_THALLID:	// on others
	  case CARD_ID_SPOROLOTH_ANCIENT:
	  case CARD_ID_THALLID:
	  case CARD_ID_THALLID_DEVOURER:
	  case CARD_ID_THALLID_GERMINATOR:
	  case CARD_ID_THALLID_SHELL_DWELLER:
	  case CARD_ID_THORN_THALLID:
	  case CARD_ID_UTOPIA_MYCON:
	  case CARD_ID_VITASPORE_THALLID:
		return COUNTER_SPORE;

	  case CARD_ID_BOTTOMLESS_VAULT:
	  case CARD_ID_CALCIFORM_POOLS:///
	  case CARD_ID_CITY_OF_SHADOWS:
	  case CARD_ID_CRUCIBLE_OF_THE_SPIRIT_DRAGON:
	  case CARD_ID_DREADSHIP_REEF:///
	  case CARD_ID_DWARVEN_HOLD:
	  case CARD_ID_FOUNTAIN_OF_CHO:///
	  case CARD_ID_FUNGAL_REACHES:///
	  case CARD_ID_HOLLOW_TREES:
	  case CARD_ID_ICATIAN_STORE:
	  case CARD_ID_MAGE_RING_NETWORK:
	  case CARD_ID_MERCADIAN_BAZAAR:///
	  case CARD_ID_MOLTEN_SLAGHEAP:///
	  case CARD_ID_RUSHWOOD_GROVE:///
	  case CARD_ID_SALTCRUSTED_STEPPE:///
	  case CARD_ID_SAND_SILOS:
	  case CARD_ID_SAPRAZZAN_COVE:///
	  case CARD_ID_SUBTERRANEAN_HANGAR:///
		return COUNTER_STORAGE;

	  case CARD_ID_CRESCENDO_OF_WAR:		return COUNTER_STRIFE;

	  case CARD_ID_GRIMOIRE_OF_THE_DEAD:
	  case CARD_ID_PURSUIT_OF_KNOWLEDGE:
		return COUNTER_STUDY;

	  case CARD_ID_NIGHT_DEALINGS:			return COUNTER_THEFT;
#ifdef UNSETS
	  case CARD_ID_RED_HOT_HOTTIE:			return COUNTER_THIRD_DEGREE_BURN;	// on others///
#endif

	  case CARD_ID_HOMARID:
	  case CARD_ID_TIDAL_INFLUENCE:
		return COUNTER_TIDE;

		// Time counters.  These fall into three categories: suspend; vanishing; and other things.
		// Suspend:
	  case CARD_ID_AEON_CHRONICLER:	// while in exile
	  case CARD_ID_ANCESTRAL_VISION:	// while in exile
	  case CARD_ID_ARC_BLADE:	// while in exile///
	  case CARD_ID_BENALISH_COMMANDER:	// while in exile
	  case CARD_ID_CHRONOMANTIC_ESCAPE:	// while in exile
	  case CARD_ID_CORPULENT_CORPSE:	// while in exile///
	  case CARD_ID_CURSE_OF_THE_CABAL:	// while in exile
	  case CARD_ID_CYCLICAL_EVOLUTION:	// while in exile///
	  case CARD_ID_DEEP_SEA_KRAKEN:	// while in exile
	  case CARD_ID_DETRITIVORE:	// while in exile
	  case CARD_ID_DICHOTOMANCY:	// while in exile
	  case CARD_ID_DIVINE_CONGREGATION:	// while in exile///
	  case CARD_ID_DURKWOOD_BALOTH:	// while in exile
	  case CARD_ID_DUSKRIDER_PEREGRINE:	// while in exile
	  case CARD_ID_EPOCHRASITE:	// while in exile
	  case CARD_ID_ERRANT_EPHEMERON:	// while in exile
	  case CARD_ID_FESTERING_MARCH:	// while in exile
	  case CARD_ID_FUNGAL_BEHEMOTH:	// while in exile
	  case CARD_ID_GIANT_DUSTWASP:	// while in exile
	  case CARD_ID_GREATER_GARGADON:	case CARD_ID_GREATER_GARGADON_SUSPENDED:	// while in exile
	  case CARD_ID_HEROES_REMEMBERED:	// while in exile
	  case CARD_ID_HYPERGENESIS:	// while in exile
	  case CARD_ID_INFILTRATOR_IL_KOR:	// while in exile///
	  case CARD_ID_ITH_HIGH_ARCANIST:	// while in exile
	  case CARD_ID_IVORY_GIANT:	// while in exile
	  case CARD_ID_KELDON_HALBERDIER:	// while in exile///
	  case CARD_ID_KNIGHT_OF_SURSI:	// while in exile
	  case CARD_ID_LIVING_END:	// while in exile
	  case CARD_ID_LOTUS_BLOOM:	// while in exile
	  case CARD_ID_MINDSTAB:	// while in exile///
	  case CARD_ID_NANTUKO_SHAMAN:	// while in exile
	  case CARD_ID_NIHILITH:	// while in exile
	  case CARD_ID_PARDIC_DRAGON:	// while in exile
	  case CARD_ID_PETRIFIED_PLATING:	// while in exile///
	  case CARD_ID_PHTHISIS:	// while in exile
	  case CARD_ID_PLUNDER:	// while in exile///
	  case CARD_ID_REALITY_STROBE:	// while in exile
	  case CARD_ID_RESTORE_BALANCE:	// while in exile
	  case CARD_ID_RIFTMARKED_KNIGHT:	// while in exile
	  case CARD_ID_RIFT_BOLT:	// while in exile
	  case CARD_ID_RIFTWING_CLOUDSKATE:	// while in exile
	  case CARD_ID_ROILING_HORROR:	// while in exile
	  case CARD_ID_SEARCH_FOR_TOMORROW:	// while in exile
	  case CARD_ID_SHADE_OF_TROKAIR:	// while in exile
	  case CARD_ID_SHIVAN_METEOR:	// while in exile
	  case CARD_ID_SHIVAN_SAND_MAGE:	// on others///
	  case CARD_ID_VEILING_ODDITY:	// while in exile///
	  case CARD_ID_VISCERID_DEEPWALKER:	// while in exile///
	  case CARD_ID_WHEEL_OF_FATE:	// while in exile
		//Vanishing
	  case CARD_ID_AVEN_RIFTWATCHER:
	  case CARD_ID_CALCIDERM:
	  case CARD_ID_CHRONOZOA:
	  case CARD_ID_DEADLY_GRUB:///
	  case CARD_ID_DEADWOOD_TREEFOLK:
	  case CARD_ID_KELDON_MARAUDERS:
	  case CARD_ID_LAVACORE_ELEMENTAL:///
	  case CARD_ID_LOST_AURAMANCERS:
	  case CARD_ID_MAELSTROM_DJINN:
	  case CARD_ID_RAVAGING_RIFTWURM:
	  case CARD_ID_REALITY_ACID:
	  case CARD_ID_SOULTETHER_GOLEM:
	  case CARD_ID_TIDEWALKER:///
	  case CARD_ID_WANING_WURM:///
		// Others:
	  case CARD_ID_DUST_OF_MOMENTS:	// on others///
	  case CARD_ID_INFINITE_HOURGLASS:///
	  case CARD_ID_TIME_BOMB:
	  case CARD_ID_TIME_VAULT:	// just in case WOTC flipflops again
	  case CARD_ID_TIMEBENDER:	// on others///
	  case CARD_ID_TIMECRAFTING:	// on others///
	  case CARD_ID_TOURACHS_GATE:
		return COUNTER_TIME;

	  case CARD_ID_HELIX_PINNACLE:			return COUNTER_TOWER;

	  case CARD_ID_TRAP_DIGGER:				return COUNTER_TRAP;	// on others///
	  case CARD_ID_LEGACYS_ALLURE:			return COUNTER_TREASURE;

	  case CARD_ID_DISCORDANT_DIRGE:
	  case CARD_ID_LILTING_REFRAIN:
	  case CARD_ID_MIDSUMMER_REVEL:
	  case CARD_ID_RECANTATION:
	  case CARD_ID_RUMBLING_CRESCENDO:
	  case CARD_ID_SERRAS_HYMN:///
	  case CARD_ID_SERRAS_LITURGY:
	  case CARD_ID_TORCH_SONG:
	  case CARD_ID_VILE_REQUIEM:
	  case CARD_ID_WAR_DANCE:
	  case CARD_ID_YISAN_THE_WANDERER_BARD:
		return COUNTER_VERSE;

	  case CARD_ID_LIVING_ARTIFACT:			return COUNTER_VITALITY;
	  case CARD_ID_ROGUE_SKYCAPTAIN:		return COUNTER_WAGE;
	  case CARD_ID_MERCADIAN_LIFT:			return COUNTER_WINCH;

	  case CARD_ID_FREYALISES_WINDS:	// on others///
	  case CARD_ID_CYCLONE:
		return COUNTER_WIND;

	  case CARD_ID_DJINN_OF_WISHES:
	  case CARD_ID_RING_OF_THREE_WISHES:
		return COUNTER_WISH;

	  case CARD_ID_DEADBOX:	// Arbitrary
		return COUNTER_DEATH;

		// Arbitrary
	  case CARD_ID_RULES_ENGINE:	//(none)
	  case CARD_ID_ELDER_DRAGON_HIGHLANDER:	//(none)
	  case CARD_ID_PLANECHASE:	//(none)
	  case CARD_ID_SPAT:	//(none)
		return COUNTER_ENERGY;

	  default:
		return -1;
	}
}

int get_counter_type_by_instance(card_instance_t* instance)
{
  int iid = instance->internal_card_id;

  if (iid == -1)
	iid = instance->backup_internal_card_id;

  if (iid == activation_card)
	iid = instance->original_internal_card_id;

  int csvid;
  if (iid == LEGACY_EFFECT_CUSTOM)
	{
	  if (instance->info_slot == (int)generic_suspend_legacy)
		return COUNTER_TIME;
	  else
		csvid = instance->display_pic_int_id;
	}
  else
	csvid = cards_data[iid].id;

  return get_counter_type_by_id(csvid);
}

void get_counter_name_by_type(char* dest, counter_t counter_type, int num)
{
  if (num <= 0)
	dest = 0;
  else
	{
	  if (counter_type > COUNTER_end)
		counter_type = COUNTER_end;

	  scnprintf(dest, 100, "%s: %d", counter_names[counter_type], num);
	}
}

void get_special_counters_name(char* dest, int csvid, int num)
{
  // 0x48cb80
  get_counter_name_by_type(dest, get_counter_type_by_id(csvid), num);
}

static int get_updated_counters_number_as_cost(int player, int card, counter_t type, int number)
{
  if (card >= 0 && get_id(player, card) == CARD_ID_MELIRAS_KEEPERS && !is_humiliated(player, card))
	return 0;

  if (type == COUNTER_M1_M1 && (card < 0 || is_what(player, card, TYPE_CREATURE)) && check_battlefield_for_id(player, CARD_ID_MELIRA_SYLVOK_OUTCAST))
	return 0;

  return number;
}

int get_updated_counters_number(int player, int card, counter_t type, int number)
{
  if (number <= 0)
	return number;

  number = get_updated_counters_number_as_cost(player, card, type, number);
  if (!number)
	return 0;

  if (card >= 0 && is_what(player, card, TYPE_EFFECT))
	return number;

  card_instance_t* instance;
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((instance = in_play(p, c)))
		switch (cards_data[instance->internal_card_id].id)
		  {
			case CARD_ID_DOUBLING_SEASON:		// self only: doubles everything
			  if (p != player)
				continue;
			  // else fall through
			case CARD_ID_SELESNYA_LOFT_GARDENS:	// everyone: doubles everything
			  number *= 2;
			  break;

			case CARD_ID_HARDENED_SCALES:
				if (p == player)
					if (type == COUNTER_P1_P1)
						number++;
						break;
			case CARD_ID_CORPSEJACK_MENACE:		// self only: doubles +1/+1
			  if (p != player)
				continue;
			  // else fall through
			case CARD_ID_PRIMAL_VIGOR:			// everyone: doubles +1/+1
			  if (type == COUNTER_P1_P1)
				number *= 2;
			  break;
		  }

  return number;
}

static void add_or_remove_counters_impl(int player, int card, counter_t type, int num)
{
  if (num == 0)
	return;

  int orig_num = num;

  int p, t;

  card_instance_t* instance = get_card_instance(player, card);
  uint8_t* t18 = (void*)(&instance->targets[18].player);
  uint8_t* position = NULL, *label = NULL;

  switch (type)
	{
	  case COUNTER_P1_P1:
		p = t = +1;
		position = &instance->counters;
		break;

	  case COUNTER_M1_M1:
		p = t = -1;
		position = &instance->counters_m1m1;
		break;

	  case COUNTER_P0_P1:	p = +0;	t = +1;	break;
	  case COUNTER_P0_P2:	p = +0;	t = +2;	break;
	  case COUNTER_P1_P0:	p = +1;	t = +0;	break;
	  case COUNTER_P1_P2:	p = +1;	t = +2;	break;
	  case COUNTER_P2_P0:	p = +2;	t = +0;	break;
	  case COUNTER_P2_P2:	p = +2;	t = +2;	break;
	  case COUNTER_M0_M1:	p = -0;	t = -1;	break;
	  case COUNTER_M0_M2:	p = -0;	t = -2;	break;
	  case COUNTER_M1_M0:	p = -1;	t = -0;	break;
	  case COUNTER_M2_M1:	p = -2;	t = -1;	break;
	  case COUNTER_M2_M2:	p = -2;	t = -2;	break;

	  default:
		p = t = 0;
		break;
	}

  if (num != 0)
	{
	  // Unless we're using a dedicated slot, try to find a slot already using this counter type.
	  // If adding counters, assign a new slot if necessary; if removing, return if no slot previously assigned.
	  if (!position)
		{
#define CHECK_COMMON(pos, lbl)			\
		  if (lbl == type)				\
			{							\
			  if (pos == 0)				\
				/* Should have been cleared already, but we'll do it now, then keep looking. */	\
				lbl = COUNTER_invalid;	\
			  else						\
				{						\
				  position = &pos;		\
				  label = &lbl;			\
				  goto found;			\
				}						\
			}

		  // counters2, counters3, counters4, counters5: slot free if counter storage is 0.  Label stored in bytes 0-3 of targets[18].player.
#define CHECK_IN_COUNTERS(pos, lbl_byte)		CHECK_COMMON(instance->pos, t18[lbl_byte])
		  // Bytes 0 and 1 of targets[18].card: slot free if label is COUNTER_invalid.  Label stored in bytes 2 and 3 of targets[18].card.
		  // (Those correspond to t18[4] and t18[5] for the slots, and t18[6] and t18[7] for the labels, respectively.)
#define CHECK_IN_TARGETS(pos_byte, lbl_byte)	CHECK_COMMON(t18[pos_byte], t18[lbl_byte])

		  CHECK_COMMON(instance->special_counters, instance->special_counter_type);
		  CHECK_IN_COUNTERS(counters2, 0);
		  CHECK_IN_COUNTERS(counters3, 1);
		  CHECK_IN_COUNTERS(counters4, 2);
		  CHECK_IN_COUNTERS(counters5, 3);
		  CHECK_IN_TARGETS(4, 6);
		  CHECK_IN_TARGETS(5, 7);

#undef CHECK_IN_COUNTERS
#undef CHECK_IN_TARGETS

		  // No slot has this type assigned.  If removing counters, then we're done.
		  if (num < 0)
			return;

		  // Find a free slot to assign to this type.

		  // If nothing in the special_counters slot and we're adding a counter of the preferred type, use that.
		  if (instance->special_counters == 0)
			{
			  unsigned int preferred_type = get_counter_type_by_instance(instance);
			  if (preferred_type == type)
				{
				  position = &instance->special_counters;
				  label = &instance->special_counter_type;
				  goto found;
				}
			}

		  // counters2, counters3, counters4, counters5: slot free if counter storage is 0.  Label stored in bytes 0-3 of targets[18].player.
#define TRY_ASSIGN_IN_COUNTERS(pos, lbl_byte)	\
		  if (instance->pos == 0)			\
			{								\
			  position = &instance->pos;	\
			  label = &t18[lbl_byte];		\
			  goto found;					\
			}

		  // Bytes 0 and 1 of targets[18].card: slot free if label is COUNTER_invalid.  Label stored in bytes 2 and 3 of targets[18].card.
		  // (Those correspond to t18[4] and t18[5] for the slots, and t18[6] and t18[7] for the labels, respectively.)
#define TRY_ASSIGN_IN_TARGETS(pos_byte, lbl_byte)	\
		  if (t18[lbl_byte] == COUNTER_invalid)		\
			{								\
			  position = &t18[pos_byte];	\
			  label = &t18[lbl_byte];		\
			  goto found;					\
			}

		  TRY_ASSIGN_IN_COUNTERS(counters2, 0);
		  TRY_ASSIGN_IN_COUNTERS(counters3, 1);
		  TRY_ASSIGN_IN_COUNTERS(counters4, 2);
		  TRY_ASSIGN_IN_COUNTERS(counters5, 3);

		  // No other visible slots free, so consider adding in the special counter slot even if doesn't match the preferred type.
		  if (instance->special_counters == 0)
			{
			  position = &instance->special_counters;
			  label = &instance->special_counter_type;
			  goto found;
			}

		  TRY_ASSIGN_IN_TARGETS(4, 6);
		  TRY_ASSIGN_IN_TARGETS(5, 7);

#undef TRY_ASSIGN_IN_COUNTERS
#undef TRY_ASSIGN_IN_TARGETS

		  // Otherwise, nowhere to store.  We'll still apply power and toughness later, but otherwise do nothing.

		found:;
		}

	  // If we found a slot, update the slot and its label
	  if (position)
		{
		  if (label && *label == COUNTER_invalid)	// Slot previously unused
			*position = 0;

		  // truncate to maximum of 255 and minimum of 0
		  if (num > 0)
			{
			  if (num + *position > 255)
				num = 255 - *position;
			}
		  else if (num < 0)
			{
			  if (num + *position < 0)
				num = 0 - *position;
			}

		  *position += num;

		  // If it's not a dedicated slot:
		  if (label)
			{
			  if (orig_num > 0)		// If adding counters, make sure that the slot is labelled.
				*label = type;
			  else if (*position == 0)	// If removing counters and we just removed the last one, free the label, and maybe move other counters there.
				{
				  *label = COUNTER_invalid;	// mark unused

				  /* If it was the label for special_counters, counters2, counters3, counters4, or counters5 (which are displayed), and one of the two
				   * non-displayed slots (in targets[18].card byte 0 or 1) have counters, move the latter to the former. */
				  if (label == &instance->special_counter_type || label <= &t18[3])
					{
					  // Try to move targets[18].card:0, labeled in targets[18].card:2
					  if (t18[4] != 0 && t18[6] != COUNTER_invalid)
						{
						  *position = t18[4];
						  *label = t18[6];
						  t18[4] = 0;
						  t18[6] = COUNTER_invalid;
						}
					  // Try to move targets[18].card:1, labeled in targets[18].card:3
					  else if (t18[5] != 0 && t18[7] != COUNTER_invalid)
						{
						  *position = t18[5];
						  *label = t18[7];
						  t18[5] = 0;
						  t18[7] = COUNTER_invalid;
						}
					}
				}
			}
		}

	  // Apply power and toughness and, if we added a counter, play a sound.
	  if (num)
		{
		  if (p)
			{
			  instance->counter_power += p * num;
			  instance->regen_status |= KEYWORD_RECALC_POWER;
			}

		  if (t)
			{
			  instance->counter_toughness += t * num;
			  instance->regen_status |= KEYWORD_RECALC_TOUGHNESS;
			}

		  if (num > 0)
			play_counters_sound(WAV_COUNTER);
		}
	}

  if (type == COUNTER_P1_P1 && num > 0 && (enable_xtrigger_flags & ENABLE_XTRIGGER_1_1_COUNTERS))
	{
	  int old_counters_added = counters_added;
	  counters_added = num;
	  dispatch_xtrigger2(player, XTRIGGER_1_1_COUNTERS, counters_added > 1 ? "+1/+1 counters" : "+1/+1 counter", 0, player, card);
	  counters_added = old_counters_added;
	}
}

// num must be > 0, or no effect
void add_counters(int player, int card, counter_t type, int num)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->internal_card_id == activation_card)
	{
	  player = instance->parent_controller;
	  card = instance->parent_card;
	}
  num = get_updated_counters_number(player, card, type, num);
  if (num > 0)
	add_or_remove_counters_impl(player, card, type, num);
}

/* num must be > 0, or no effect.  Assumes caller has already called get_updated_counters_number().  This still accounts for putting counters on
 * Melira's Keepers, but not onto effect cards.  Do not call for the latter. */
void add_counters_predoubled(int player, int card, counter_t type, int num)
{
  ASSERT(!is_what(player, card, TYPE_EFFECT));
  if (get_id(player, card) == CARD_ID_MELIRAS_KEEPERS && !is_humiliated(player, card))
	return;

  add_or_remove_counters_impl(player, card, type, num);
}

// unaffected by doubling effects; num may be positive or negative
void add_counters_as_cost(int player, int card, counter_t type, int num)
{
  if (num > 0)
	{
	  num = get_updated_counters_number_as_cost(player, card, type, num);
	  if (!num)
		return;
	}

  add_or_remove_counters_impl(player, card, type, num);
}

// num must be > 0, or no effect
void remove_counters(int player, int card, counter_t type, int num)
{
  if (num > 0)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->internal_card_id == activation_card)
		{
		  player = instance->parent_controller;
		  card = instance->parent_card;
		}
	  add_or_remove_counters_impl(player, card, type, -num);
	}
}

/* For each counter on {src_player, src_card}, add one of that type to {t_player, t_card}.  If raw is set, add the exact amount (even for Melira's Keepers or
 * Melira, Sylvok Outcast, or if Doubling Season etc. are present). */
void copy_counters(int t_player, int t_card, int src_player, int src_card, int raw)
{
  card_instance_t* instance = get_card_instance(src_player, src_card);
  if (instance->internal_card_id == activation_card)
	instance = get_card_instance(instance->parent_controller, instance->parent_card);

  card_instance_t* t_instance = get_card_instance(t_player, t_card);
  if (t_instance->internal_card_id == activation_card)
	{
	  t_player = t_instance->parent_controller;
	  t_card = t_instance->parent_card;
	}

  uint8_t* t18 = (void*)(&instance->targets[18].player);

  struct
  {
	counter_t typ;
	int num;
  } counters[9];
  int num_types = 0;

#define ADD_TYPE(pos, lbl)											\
  if (pos && (counters[num_types].typ = lbl) != COUNTER_invalid)	\
	{																\
	  counters[num_types].num = pos;								\
	  ++num_types;													\
	}

  ADD_TYPE(instance->special_counters, instance->special_counter_type);
  ADD_TYPE(instance->counters2, t18[0]);
  ADD_TYPE(instance->counters3, t18[1]);
  ADD_TYPE(instance->counters4, t18[2]);
  ADD_TYPE(instance->counters5, t18[3]);
  ADD_TYPE(instance->counters, COUNTER_P1_P1);
  ADD_TYPE(instance->counters_m1m1, COUNTER_M1_M1);
  ADD_TYPE(t18[4], t18[6]);
  ADD_TYPE(t18[5], t18[7]);

#undef ADD_TYPE

  int i;
  if (raw)
	for (i = 0; i < num_types; ++i)
	  add_or_remove_counters_impl(t_player, t_card, counters[i].typ, counters[i].num);
  else
	for (i = 0; i < num_types; ++i)
	  add_counters(t_player, t_card, counters[i].typ, counters[i].num);
}

/* Moves number of counter_type from {src_player, src_card} to {t_player, t_card}.  Moves all counters of all types if counter_type is -1; moves all of
 * counter_type if number is -1. */
void move_counters(int t_player, int t_card, int src_player, int src_card, counter_t counter_type, int number)
{
  // No effect if moving to same card
  if (t_card == src_card && t_player == src_player)
	return;

  // It's legal to attempt to move counters onto Melira's Keepers, and the spell or effect doesn't fizzle, but nothing happens.
  if (get_id(t_player, t_card) == CARD_ID_MELIRAS_KEEPERS && !is_humiliated(t_player, t_card))
	return;

  if ((unsigned char)counter_type == COUNTER_invalid)
	{
	  // If t_player controls a Melira, Sylvok Outcast, -1/-1 counters don't move.
	  if (check_battlefield_for_id(t_player, CARD_ID_MELIRA_SYLVOK_OUTCAST))
		{
		  // Hack - remove all -1/-1 counters, move everything else, then put them back
		  card_instance_t* instance = get_card_instance(src_player, src_card);
		  int counters_m1m1 = instance->counters_m1m1;
		  instance->counters_m1m1 = 0;

		  ++hack_silent_counters;
		  copy_counters(t_player, t_card, src_player, src_card, 1);
		  remove_all_counters(src_player, src_card, -1);
		  --hack_silent_counters;

		  instance->counters_m1m1 = counters_m1m1;
		}
	  else
		{
		  ++hack_silent_counters;
		  copy_counters(t_player, t_card, src_player, src_card, 1);
		  remove_all_counters(src_player, src_card, -1);
		  --hack_silent_counters;
		}
	}
  else
	{
	  // If t_player controls a Melira, Sylvok Outcast, -1/-1 counters don't move.
	  if (check_battlefield_for_id(t_player, CARD_ID_MELIRA_SYLVOK_OUTCAST) && counter_type == COUNTER_M1_M1)
		return;

	  if (number < 0)
		number = 255;

	  int curr = count_counters(src_player, src_card, counter_type);
	  number = MIN(number, curr);

	  remove_counters(src_player, src_card, counter_type, number);
	  add_counters_predoubled(t_player, t_card, counter_type, number);
	}
}

static int count_counters_impl(card_instance_t* instance, counter_t type)
{
  if (type == COUNTER_P1_P1)
	return instance->counters;	// Dedicated slot for +1/+1 counters

  if (type == COUNTER_M1_M1)
	return instance->counters_m1m1;	// Dedicated slot for -1/-1 counters

  uint8_t* t18 = (void*)(&instance->targets[18].player);

  int tot;
  if ((unsigned char)type == COUNTER_invalid)	// Total count of all counters
	{
	  tot = (instance->special_counters		// dedicated to a type determined by card's csvid
			 + instance->counters			// dedicated to +1/+1 counters
			 + instance->counters_m1m1);	// dedicated to -1/-1 counters

	  // Dynamically-assigned counter types.
	  // Four visible
	  if (t18[0] != COUNTER_invalid)
		tot += instance->counters2;
	  if (t18[1] != COUNTER_invalid)
		tot += instance->counters3;
	  if (t18[2] != COUNTER_invalid)
		tot += instance->counters4;
	  if (t18[3] != COUNTER_invalid)
		tot += instance->counters5;
	  // Two more invisible
	  if (t18[6] != COUNTER_invalid)
		tot += t18[4];
	  if (t18[7] != COUNTER_invalid)
		tot += t18[5];

	  return tot;
	}

  /* General case.  Be a little extra paranoid in case a card has gotten the same counter type both in special_counters (type determined by csvid) and an
   * assignable counter slot, presumeably by an uncontrolled copy effect. */

  tot = 0;

  if (instance->special_counter_type == type)
	tot += instance->special_counters;

  // Four visible
  if (t18[0] == type)
	tot += instance->counters2;
  if (t18[1] == type)
	tot += instance->counters3;
  if (t18[2] == type)
	tot += instance->counters4;
  if (t18[3] == type)
	tot += instance->counters5;
  // Two more invisible
  if (t18[6] == type)
	tot += t18[4];
  if (t18[7] == type)
	tot += t18[5];

  return tot;
}

// use -1 to get total count
int count_counters(int player, int card, counter_t type)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->internal_card_id == activation_card)
	instance = get_card_instance(instance->parent_controller, instance->parent_card);

  return count_counters_impl(instance, type);
}

/* Just like count_counters(), but if called for an activation_card, return the counters that were on th parent card as it was activated, not how many are on it
 * now. */
int count_counters_no_parent(int player, int card, counter_t type)
{
  return count_counters_impl(get_card_instance(player, card), type);
}

// use -1 to remove all of all types
void remove_all_counters(int player, int card, counter_t type)
{
  if ((unsigned char)type == COUNTER_invalid)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->special_counters = 0;
	  instance->special_counter_type = -1;
	  instance->counters2 = 0;
	  instance->counters3 = 0;
	  instance->counters4 = 0;
	  instance->counters = 0;
	  instance->counters5 = 0;
	  instance->counters_m1m1 = 0;
	  instance->targets[18].player = -1;
	  instance->targets[18].card = 0x0000FFFF;

	  instance->counter_power = 0;
	  instance->counter_toughness = 0;
	  instance->regen_status |= KEYWORD_RECALC_POWER | KEYWORD_RECALC_TOUGHNESS;
	}
  else
	remove_counters(player, card, type, 255);
}

// May use ANYBODY for player and/or -1 for counter_type to indicate any player's cards or any kind of counter.  Returns a total count.
int count_counters_by_counter_and_card_type(int player, counter_t counter_type, type_t type)
{
  int p, c, result = 0;
  for (p = 0; p <= 1; ++p)
	if (p == player || player == ANYBODY)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, type))
			result += count_counters(p, c, counter_type);

  return result;
}

// May use ANYBODY for player and/or -1 for counter_type to indicate any player's cards or any kind of counter.  Stops looking after the first match.
int has_any_counters(int player, counter_t counter_type, type_t type)
{
  int p, c;
  for (p = 0; p <= 1; ++p)
	if (p == player || player == ANYBODY)
	  for (c = 0; c <= active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, type) && count_counters(p, c, counter_type))
		  return 1;

  return 0;
}

/* The return values here, which should all be between -10000 and 10000, indicate how much the AI values a counter type on a card it controls.  They're inverted
 * if the opponent controls the card.
 *
 * If adding is nonzero, the AI is considering adding another counter to the card (there will always be at least one counter of that type already there);
 * otherwise, it's considering removing a counter of that type, possibly the last.
 *
 * So, for example, COUNTER_EON returns 0 for a non-native card, else 4 if adding, else 9001: this means, the AI doesn't care one way or the other about Eon
 * counters on a Grizzly Bears; is very slightly in favor of adding a second (or third, etc.) Eon counter to a Magosi, the Waterveil it controls; is very
 * slightly against adding a second (or third, etc.) Eon counter to a Magosi, the Waterveil the human player controls; is very, very much against removing an
 * Eon counter from its own Magosi, the Waterveil; and is very, very much in favor of removing an Eon counter from the human player's Magosi, the Waterveil. */
static int raw_counter_value(int player, int t_player, int t_card, counter_t counter_type, int adding)
{
  card_instance_t* instance = get_card_instance(t_player, t_card);
  int iid = instance->internal_card_id >= 0 ? instance->internal_card_id : instance->backup_internal_card_id;
  int csvid = cards_data[iid].id;

#define IS(type)			(is_what(t_player, t_card, (type)))
#define ANY_CONTROLLER(x)	(player == t_player ? (x) : -(x))	// i.e., invert value if controlled by opponent.  Since this is normally done anyway, specifying ANY_CONTROLLER() means that the eventual value is x no matter who controls {t_player, t_card}.
#define IS_NATIVE()			(get_counter_type_by_instance(instance) == (int)counter_type)
#define NATIVE0(native)		(IS_NATIVE() ? (native) : 0)
#define CREATURE0(creature)	(IS(TYPE_CREATURE) ? creature : 0)

  if (adding == 2)
	{
	  /* Adding a counter, not necessarily one already present.  So if one already present, use add values (which assign low values for counters which only
	   * matter if there's at least one, and gain no benefit from multiples); if none present, use remove values (which assign high values for counters which
	   * only matter if there's at least one). */
	  if (count_counters(t_player, t_card, counter_type) > 0)
		adding = 1;
	  else
		adding = 0;
	}

  int max_level, benefit;
  switch (counter_type)
	{
	  case COUNTER_P1_P1:
		if (!IS(TYPE_CREATURE))
		  return 0;
		switch (csvid)
		  {
			default:
			  if (!check_special_flags(t_player, t_card, SF_MODULAR))
				return 1200;
			  // else fall through
			case CARD_ID_ANIMAR_SOUL_OF_ELEMENTS:
			case CARD_ID_APOCALYPSE_HYDRA:
			case CARD_ID_ASHLING_THE_PILGRIM:
			case CARD_ID_CANOPY_CRAWLER:
			case CARD_ID_DEATHBRINGER_THOCTAR:
			case CARD_ID_ETCHED_ORACLE:
			case CARD_ID_FERTILID:
			case CARD_ID_FESTERCREEP:
			case CARD_ID_GYRE_SAGE:
			case CARD_ID_HERALD_OF_WAR:
			case CARD_ID_JORAGA_WARCALLER:
			case CARD_ID_MIKAEUS_THE_LUNARCH:
			case CARD_ID_MOLTEN_HYDRA:
			case CARD_ID_MYCOLOTH:
			case CARD_ID_PENTAVUS:
			case CARD_ID_SHINEWEND:
			case CARD_ID_SPIKE_FEEDER:
			case CARD_ID_SPIKE_WEAVER:
			case CARD_ID_TETRAVUS:
			case CARD_ID_THOUGHT_GORGER:
			case CARD_ID_TRISKELAVUS:
			case CARD_ID_TRISKELION:
			case CARD_ID_TWILIGHT_DROVER:
			case CARD_ID_ULASHT_THE_HATE_SEED:
			case CARD_ID_VISH_KAL_BLOOD_ARBITER:
			case CARD_ID_WORKHORSE:
			  return 7300;
		  }

	  case COUNTER_M1_M1:		return CREATURE0(-3200);
	  case COUNTER_P0_P1:		return CREATURE0(150);
	  case COUNTER_P0_P2:		return CREATURE0(300);
	  case COUNTER_P1_P0:		return CREATURE0(600);
	  case COUNTER_P1_P2:		return CREATURE0(1800);
	  case COUNTER_P2_P0:		return CREATURE0(2400);
	  case COUNTER_P2_P2:		return CREATURE0(4800);
	  case COUNTER_M0_M1:		return CREATURE0(-1600);
	  case COUNTER_M0_M2:		return CREATURE0(-7200);
	  case COUNTER_M1_M0:		return CREATURE0(-1563);
	  case COUNTER_M2_M1:		return CREATURE0(-4800);
	  case COUNTER_M2_M2:		return CREATURE0(-9600);
	  case COUNTER_AGE:			return NATIVE0(-1000);
	  case COUNTER_AIM:			return NATIVE0(125);
	  case COUNTER_ARROW:		return NATIVE0(440);
	  case COUNTER_ARROWHEAD:	return NATIVE0(490);
	  case COUNTER_AWAKENING:	return IS(TYPE_LAND) ? 6 : 0;
	  case COUNTER_BLAZE:		return IS_NATIVE() ? 52 : !IS(TYPE_LAND) ? 0 : adding ? -4 : -401;

	  case COUNTER_BLOOD:
		switch (csvid)
		  {
			case CARD_ID_RAKDOS_RITEKNIFE:	return 8800;
			case CARD_ID_BLOODLETTER_QUILL:	return -160;
			default:						return 0;
		  }

	  case COUNTER_BOUNTY:		return CREATURE0(adding ? -6 : -601);
	  case COUNTER_BRIBERY:		return CREATURE0(adding ? -7 : -701);
	  case COUNTER_CARRION:		return NATIVE0(40);
	  case COUNTER_CHIP:		return NATIVE0(36);
	  case COUNTER_CORPSE:		return NATIVE0(430);
	  case COUNTER_CREDIT:		return NATIVE0(38);
	  case COUNTER_CRYSTAL:		return NATIVE0(441);
	  case COUNTER_CUBE:		return NATIVE0(47);
	  case COUNTER_CURRENCY:	return NATIVE0(41);
	  case COUNTER_DEATH:		return NATIVE0(adding ? -5 : -5001);

	  case COUNTER_DELAY:
		if (csvid == CARD_ID_DELAYING_SHIELD
			|| (iid == LEGACY_EFFECT_CUSTOM && instance->display_pic_int_id == CARD_ID_ERTAIS_MEDDLING))
		  return -850;
		else
		  return 0;

	  case COUNTER_DEPLETION:
		if (!IS_NATIVE())
		  return 0;

		switch (csvid)
		  {
			case CARD_ID_DECREE_OF_SILENCE:	return -9999;
			case CARD_ID_FORCE_BUBBLE:		return -540;

			case CARD_ID_LAND_CAP:
			case CARD_ID_LAVA_TUBES:
			case CARD_ID_RIVER_DELTA:
			case CARD_ID_TIMBERLINE_RIDGE:
			case CARD_ID_VELDT:
			default:
			  return -350;

			case CARD_ID_HICKORY_WOODLOT:
			case CARD_ID_PEAT_BOG:
			case CARD_ID_REMOTE_FARM:
			case CARD_ID_SANDSTONE_NEEDLE:
			case CARD_ID_SAPRAZZAN_SKERRY:
			  return 480;
		  }

	  case COUNTER_DESPAIR:		return NATIVE0(16);
	  case COUNTER_DEVOTION:	return NATIVE0(420);
	  case COUNTER_DIVINITY:	return IS_NATIVE() ? 9600 : adding ? 5 : 8001;

	  case COUNTER_DOOM:		return (csvid == CARD_ID_ARMAGEDDON_CLOCK && life[player] > life[1-player]) ? 13 : adding ? -8 : -801;

	  case COUNTER_DREAM:
		if (!IS_NATIVE()
			|| (csvid == CARD_ID_RASPUTIN_DREAMWEAVER && adding && count_counters(t_player, t_card, COUNTER_DREAM) >= 7))
		  return 0;
		else
		  return 410;

	  case COUNTER_ECHO:		return NATIVE0(65);
	  case COUNTER_ELIXIR:		return NATIVE0(44);
	  case COUNTER_ENERGY:		return NATIVE0(380);
	  case COUNTER_EON:			return !IS_NATIVE() ? 0 : adding ? 4 : 9001;
	  case COUNTER_EYEBALL:		return NATIVE0(70);

	  case COUNTER_FADE:
		if (!IS_NATIVE())
		  return 0;
		else if (csvid == CARD_ID_PARALLAX_DEMENTIA && instance->damage_target_player == 1-player)
		  return ANY_CONTROLLER(-9997);
		else
		  return 9200;

	  case COUNTER_FATE:		return adding ? 0 : 801;
	  case COUNTER_FEATHER:		return IS_NATIVE() ? 8600 : adding ? 7 : 701;
	  case COUNTER_FILIBUSTER:	return NATIVE0(10000);
	  case COUNTER_FLAME:		return NATIVE0(ANY_CONTROLLER(-650));
	  case COUNTER_FLOOD:		return adding ? 0 : -301;
	  case COUNTER_FUNGUS:		return -190;
	  case COUNTER_FUNK:		return NATIVE0(9000);
	  case COUNTER_FUSE:		return IS_NATIVE() ? 400 : IS(TYPE_CREATURE) ? -800 : 0;
	  case COUNTER_GEM:			return NATIVE0(442);
	  case COUNTER_GLYPH:		return -210;
	  case COUNTER_GOLD:		return !IS(TYPE_CREATURE) ? 0 : adding ? -3 : -6001;
	  case COUNTER_GROWTH:		return IS_NATIVE() ? 0 : csvid == CARD_ID_MOMENTUM ? 480 : 15;
	  case COUNTER_HATCHLING:	return NATIVE0(7900);
	  case COUNTER_HEALING:		return NATIVE0(39);
	  case COUNTER_HOOFPRINT:	return NATIVE0(8000);
	  case COUNTER_HOURGLASS:	return adding || !is_tapped(t_player, t_card) ? -9 : -2001;
	  case COUNTER_HUNGER:		return NATIVE0(510);
	  case COUNTER_HUSK:		return NATIVE0(8100);

	  case COUNTER_ICE:
		switch (csvid)
		  {
			case CARD_ID_DARK_DEPTHS:		return -330;
			case CARD_ID_WOOLLY_RAZORBACK:	return -230;
			default:						return -2;
			case CARD_ID_ICEBERG:			return 80;
		  }

	  case COUNTER_INFECTION:	return NATIVE0(7800);
	  case COUNTER_INTERVENTION:return NATIVE0(24);
	  case COUNTER_ISOLATION:	return 0;
	  case COUNTER_JAVELIN:		return NATIVE0(701);
	  case COUNTER_KI:			return NATIVE0(511);

	  case COUNTER_LEVEL:
		if (!IS_NATIVE())
		  return 0;

		switch (csvid)
		  {
			case CARD_ID_BEASTBREAKER_OF_BALA_GED:	max_level = 4;	benefit = 7500;	break;
			case CARD_ID_BRIMSTONE_MAGE:			max_level = 3;	benefit = 7500;	break;
			case CARD_ID_CARAVAN_ESCORT:			max_level = 5;	benefit =  500;	break;
			case CARD_ID_CORALHELM_COMMANDER:		max_level = 4;	benefit =  500;	break;
			case CARD_ID_ECHO_MAGE:					max_level = 4;	benefit =  500;	break;
			case CARD_ID_ENCLAVE_CRYPTOLOGIST:		max_level = 3;	benefit =  500;	break;
			case CARD_ID_GUUL_DRAZ_ASSASSIN:		max_level = 4;	benefit =  500;	break;
			case CARD_ID_HADA_SPY_PATROL:			max_level = 3;	benefit = 7500;	break;
			case CARD_ID_HALIMAR_WAVEWATCH:			max_level = 5;	benefit =  500;	break;
			case CARD_ID_HEDRON_FIELD_PURISTS:		max_level = 5;	benefit = 7500;	break;
			case CARD_ID_IKIRAL_OUTRIDER:			max_level = 4;	benefit = 7500;	break;
			case CARD_ID_JORAGA_TREESPEAKER:		max_level = 5;	benefit =  500;	break;
			case CARD_ID_KABIRA_VINDICATOR:			max_level = 5;	benefit = 7500;	break;
			case CARD_ID_KARGAN_DRAGONLORD:			max_level = 8;	benefit =  500;	break;
			case CARD_ID_KAZANDU_TUSKCALLER:		max_level = 6;	benefit =  500;	break;
			case CARD_ID_KNIGHT_OF_CLIFFHAVEN:		max_level = 4;	benefit = 7500;	break;
			case CARD_ID_LIGHTHOUSE_CHRONOLOGIST:	max_level = 7;	benefit =  500;	break;
			case CARD_ID_LORD_OF_SHATTERSKULL_PASS:	max_level = 6;	benefit =  500;	break;
			case CARD_ID_NIRKANA_CUTTHROAT:			max_level = 3;	benefit = 7500;	break;
			case CARD_ID_NULL_CHAMPION:				max_level = 4;	benefit = 7500;	break;
			case CARD_ID_SKYWATCHER_ADEPT:			max_level = 3;	benefit = 7500;	break;
			case CARD_ID_STUDENT_OF_WARFARE:		max_level = 7;	benefit =  500;	break;
			case CARD_ID_TRANSCENDENT_MASTER:		max_level = 12;	benefit =  500;	break;
			case CARD_ID_ZULAPORT_ENFORCER:			max_level = 3;	benefit = 7500;	break;
			default:	return 0;
		  }

		if (adding && count_counters(t_player, t_card, COUNTER_LEVEL) >= max_level)
		  return 0;
		else
		  return benefit;

	  case COUNTER_LORE:		return NATIVE0(8300);
	  case COUNTER_LOYALTY:		return IS(TARGET_TYPE_PLANESWALKER) ? 8500 : 0;
	  case COUNTER_LUCK:		return !IS_NATIVE() ? 0 : csvid == CARD_ID_CHANCE_ENCOUNTER ? 9998 : adding ? 3 : 301;
	  case COUNTER_MAGNET:		return !IS(TYPE_CREATURE) ? 0 : adding ? -10 : -1001;
	  case COUNTER_MANIFESTATION:	return 0;
	  case COUNTER_MANNEQUIN:	return !IS(TYPE_CREATURE) ? 0 : adding ? -11 : -7001;
	  case COUNTER_MATRIX:		return 46;
	  case COUNTER_MINE:		return !IS(TYPE_LAND) ? 0 : adding ? -12 : -7002;
	  case COUNTER_MINING:		return NATIVE0(470);
	  case COUNTER_MIRE:		return !IS(TYPE_LAND) ? 0 : adding ? -1 : -101;
	  case COUNTER_MUSIC:		return -700;
	  case COUNTER_MUSTER:		return NATIVE0(130);

	  case COUNTER_NET:
		if (csvid != CARD_ID_MERSEINE)
		  return 0;
		else if (instance->damage_target_player == 1-player)
		  return ANY_CONTROLLER(8);
		else if (instance->damage_target_player == player)
		  return ANY_CONTROLLER(-400);
		else
		  return 0;

	  case COUNTER_OMEN:
		if (csvid != CARD_ID_CELESTIAL_CONVERGENCE)
		  return 0;
		if (life[player] > life[1-player])
		  return ANY_CONTROLLER(9997);
		else if (life[player] < life[1-player])
		  return ANY_CONTROLLER(-10000);
		else
		  return 0;

	  case COUNTER_ORE:			return NATIVE0(-530);
	  case COUNTER_PAGE:		return NATIVE0(370);
	  case COUNTER_PAIN:		return NATIVE0(60);
	  case COUNTER_PARALYZATION:return -250;
	  case COUNTER_PETAL:		return NATIVE0(81);
	  case COUNTER_PETRIFICATION:	return 0;
	  case COUNTER_PHYLACTERY:	return adding ? 2 : 8001;
	  case COUNTER_PIN:			return NATIVE0(-50);

	  case COUNTER_PLAGUE:
		if (!IS_NATIVE())
		  return 0;
		if (instance->damage_target_player == player)
		  return ANY_CONTROLLER(-100);
		else if (instance->damage_target_player == 1-player)
		  return ANY_CONTROLLER(100);
		else
		  return 100;

	  case COUNTER_POLYP:		return NATIVE0(110);
	  case COUNTER_POP:			return NATIVE0(35);

	  case COUNTER_PRESSURE:
		if (csvid == CARD_ID_MOUNT_KERALIA)
		  return ANY_CONTROLLER(360);
		else
		  return NATIVE0(360);

	  case COUNTER_PUPA:		return NATIVE0(-260);

	  case COUNTER_QUEST:
		if (!IS_NATIVE())
		  return 0;

		switch (csvid)
		  {
			case CARD_ID_ARCHMAGE_ASCENSION:		max_level = 6;	benefit = 6000;	break;
			case CARD_ID_BEASTMASTER_ASCENSION:		max_level = 7;	benefit = 6000;	break;
			case CARD_ID_BLOODCHIEF_ASCENSION:		max_level = 3;	benefit = 6000;	break;
			case CARD_ID_IOR_RUIN_EXPEDITION:		max_level = 3;	benefit = 350;	break;
			case CARD_ID_KHALNI_HEART_EXPEDITION:	max_level = 3;	benefit = 350;	break;
			case CARD_ID_LUMINARCH_ASCENSION:		max_level = 4;	benefit = 6000;	break;
			case CARD_ID_PYROMANCER_ASCENSION:		max_level = 2;	benefit = 6000;	break;
			case CARD_ID_QUEST_FOR_ANCIENT_SECRETS:	max_level = 5;	benefit = 10;	break;
			case CARD_ID_QUEST_FOR_PURE_FLAME:		max_level = 4;	benefit = 350;	break;
			case CARD_ID_QUEST_FOR_RENEWAL:			max_level = 4;	benefit = 350;	break;
			case CARD_ID_QUEST_FOR_ULAS_TEMPLE:		max_level = 3;	benefit = 6000;	break;
			case CARD_ID_QUEST_FOR_THE_GEMBLADES:	max_level = 0;	benefit = 350;	break;
			case CARD_ID_QUEST_FOR_THE_GOBLIN_LORD:	max_level = 5;	benefit = 6000;	break;
			case CARD_ID_QUEST_FOR_THE_GRAVELORD:	max_level = 3;	benefit = 350;	break;
			case CARD_ID_QUEST_FOR_THE_HOLY_RELIC:	max_level = 5;	benefit = 350;	break;
			case CARD_ID_QUEST_FOR_THE_NIHIL_STONE:	max_level = 2;	benefit = 350;	break;
			case CARD_ID_SOUL_STAIR_EXPEDITION:		max_level = 3;	benefit = 350;	break;
			case CARD_ID_SUNSPRING_EXPEDITION:		max_level = 3;	benefit = 350;	break;
			case CARD_ID_ZEKTAR_SHRINE_EXPEDITION:	max_level = 3;	benefit = 350;	break;
			default:	return 0;
		  }

		if (adding && count_counters(t_player, t_card, COUNTER_LEVEL) >= max_level)
		  return 0;
		else
		  return benefit;

	  case COUNTER_RUST:		return IS(TYPE_ARTIFACT) ? -600 : 0;

	  case COUNTER_SCREAM:
		if (iid == LEGACY_EFFECT_CUSTOM && instance->display_pic_int_id == CARD_ID_ALL_HALLOWS_EVE)
		  return -500;

		if (csvid == CARD_ID_ENDLESS_SCREAM)
		  {
			if (instance->damage_target_player == player)
			  return ANY_CONTROLLER(120);
			if (instance->damage_target_player == 1-player)
			  return ANY_CONTROLLER(-120);
		  }

		return 0;

	  case COUNTER_SCROLL:	return csvid == CARD_ID_ARETOPOLIS ? ANY_CONTROLLER(34) : 0;
	  case COUNTER_SHELL:	return NATIVE0(-280);
	  case COUNTER_SHIELD:	return NATIVE0(50);
	  case COUNTER_SHOE:	return NATIVE0(1400);
	  case COUNTER_SHRED:	return NATIVE0(7400);
	  case COUNTER_SLEEP:	return -261;
	  case COUNTER_SLEIGHT:	return NATIVE0(-290);	// deliberately not checking damage_target_player
	  case COUNTER_SLIME:	return NATIVE0(7700);
	  case COUNTER_SOOT:	return NATIVE0(14);
	  case COUNTER_SPORE:	return NATIVE0(550);
	  case COUNTER_STORAGE:	return NATIVE0(560);
	  case COUNTER_STRIFE:	return NATIVE0(320);
	  case COUNTER_STUDY:	return NATIVE0(7600);
	  case COUNTER_THEFT:	return NATIVE0(140);
	  case COUNTER_THIRD_DEGREE_BURN:	return adding ? 0 : -15;
	  case COUNTER_TIDE:
		if (!IS_NATIVE())
		  return 0;

		if (count_counters(t_player, t_card, COUNTER_TIDE == 3))
		  return -300;
		else
		  return 28;

	  case COUNTER_TIME:
		if (iid == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)generic_suspend_legacy)
		  return -901;

		if (!IS_NATIVE())
		  return 0;

		switch (csvid)
		  {
			case CARD_ID_CHRONOZOA:
			case CARD_ID_DEADLY_GRUB:
			case CARD_ID_LOST_AURAMANCERS:
			case CARD_ID_REALITY_ACID:
			  return -150;

			default:
			  return 9100;
		  }

	  case COUNTER_TOWER:	return NATIVE0(30);
	  case COUNTER_TRAINING:return 0;
	  case COUNTER_TRAP:	return !IS(TYPE_LAND) ? 0 : adding ? 1 : 101;
	  case COUNTER_TREASURE:return NATIVE0(5000);
	  case COUNTER_VELOCITY:return csvid == CARD_ID_TORNADO ? -550 : 0;	// its native type is COUNTER_AGE, not this

	  case COUNTER_VERSE:
		if (!IS_NATIVE())
		  return 0;

		switch (csvid)
		  {
			case CARD_ID_MIDSUMMER_REVEL:
			case CARD_ID_RECANTATION:
			case CARD_ID_SERRAS_LITURGY:
			case CARD_ID_VILE_REQUIEM:
			  return 7000;

			case CARD_ID_DISCORDANT_DIRGE:
			  if (hand_count[1-t_player] < count_counters(t_player, t_card, COUNTER_VERSE) + (adding ? 0 : 1))
				return 7000;
			  // else fall through
			default:
			  return 530;
		  }

	  case COUNTER_VITALITY:return NATIVE0(37);
	  case COUNTER_WAGE:	return NATIVE0(-520);
	  case COUNTER_WINCH:	return NATIVE0(310);

	  case COUNTER_WIND:
		if (csvid == CARD_ID_CYCLONE)
		  return life[player] > life[1-player] ? ANY_CONTROLLER(12) : ANY_CONTROLLER(-13);
		else
		  return -13;

	  case COUNTER_WISH:	return NATIVE0(540);

	  case COUNTER_CHARGE:
		switch (csvid)
		  {
			case CARD_ID_DARKSTEEL_REACTOR:
			  return 9999;
			case CARD_ID_CONVERSION_CHAMBER:
			case CARD_ID_CULLING_DAIS:
			case CARD_ID_DOOR_OF_DESTINIES:
			case CARD_ID_GOLEM_FOUNDRY:
			case CARD_ID_LIGHTNING_REAVER:
			case CARD_ID_LUX_CANNON:
			case CARD_ID_MAGISTRATES_SCEPTER:
			case CARD_ID_OROCHI_HATCHERY:
			case CARD_ID_OTHERWORLD_ATLAS:
			case CARD_ID_RIPTIDE_REPLICATOR:
			case CARD_ID_SHRINE_OF_LOYAL_LEGIONS:
			case CARD_ID_SPHINX_BONE_WAND:
			case CARD_ID_TITAN_FORGE:
			case CARD_ID_UMEZAWAS_JITTE:
			  return 6500;
			case CARD_ID_KILNSPIRE_DISTRICT:
			  return ANY_CONTROLLER(450);
			default:
			  return NATIVE0(450);
		  }

	  case COUNTER_end:		return 0;
	  case COUNTER_invalid:	return 0;
	}

  return 0;

#undef IS
#undef CONTROLLER
#undef IS_NATIVE
#undef NATIVE0
#undef CREATURE0
}

// Returns COUNTER_invalid if cancelled, or if there are no counters.
counter_t choose_existing_counter_type(int who_chooses, int src_player, int src_card, int t_player, int t_card, cect_t mode, int smallcard_player, int smallcard_card)
{
  STATIC_ASSERT(sizeof(counter_t) >= 2, counter_t_must_be_at_least_two_bytes_wide);

  char choices[10][100];
  int i;
  for (i = 0; i < 10; ++i)
	choices[i][0] = 0;
  counter_t counter_types[10];
  int priorities[10];

  int num_types = 0;

  card_instance_t* instance = get_card_instance(t_player, t_card);
  uint8_t* t18 = (void*)(&instance->targets[18].player);

  struct
  {
	uint8_t num;
	uint8_t typ;
  } counter_data[9] =
	  {
		{ instance->special_counters, instance->special_counter_type },	// Top
		{ instance->counters2, t18[0] },	// Upper right
		{ instance->counters3, t18[1] },	// Upper center
		{ instance->counters4, t18[2] },	// Upper left
		{ instance->counters5, t18[3] },	// Lower right
		{ instance->counters, COUNTER_P1_P1 },		// Lower left
		{ instance->counters_m1m1, COUNTER_M1_M1 },	// Also lower left
		{ t18[4], t18[6] },	// undisplayed 1
		{ t18[5], t18[7] }	// undisplayed 2
	  };

  int add_remove_mode = mode & (CECT_REMOVE | CECT_ADD_OR_REMOVE | CECT_MOVE);
  ASSERT(num_bits_set(add_remove_mode) <= 1);
#define CECT_ADD ((cect_t)(1<<8))
  if (!add_remove_mode)
	add_remove_mode |= CECT_ADD;

  for (i = 0; i < 9; ++i)
	if (counter_data[i].num > 0 && counter_data[i].typ != COUNTER_invalid)
	  {
		counter_types[num_types] = counter_data[i].typ;
		scnprintf(choices[num_types], 100, "%s: %d", counter_names[counter_data[i].typ], counter_data[i].num);

		int pri_add = INT_MIN, pri_remove = INT_MIN;
		if (add_remove_mode & (CECT_ADD | CECT_ADD_OR_REMOVE))	// consider adding a counter to {t_player,t_card}
		  {
			pri_add = raw_counter_value(who_chooses, t_player, t_card, counter_data[i].typ, 1);
			if (who_chooses != t_player)
			  pri_add = -pri_add;
		  }
		if (add_remove_mode & CECT_MOVE)	// consider adding a counter to {smallcard_player,smallcard_card}
		  {
			pri_add = raw_counter_value(who_chooses, smallcard_player, smallcard_card, counter_data[i].typ, 2);
			if (who_chooses != smallcard_player)
			  pri_add = -pri_add;
		  }
		if (add_remove_mode & (CECT_REMOVE | CECT_ADD_OR_REMOVE | CECT_MOVE))	// consider removing a counter from {t_player,t_card}
		  {
			pri_remove = -raw_counter_value(who_chooses, t_player, t_card, counter_data[i].typ, 0);
			if (who_chooses != t_player)
			  pri_remove = -pri_remove;
		  }

		switch (add_remove_mode)
		  {
			case CECT_REMOVE:
			  priorities[num_types] = pri_remove;
			  break;

			case CECT_ADD_OR_REMOVE:
			  if (pri_remove > pri_add)
				{
				  priorities[num_types] = pri_remove;
				  counter_types[num_types] |= (1<<9);
				}
			  else
				priorities[num_types] = pri_add;
			  break;

			case CECT_MOVE:
			  priorities[num_types] = (pri_add - pri_remove) / 2;
			  break;

			default:	// add
			  priorities[num_types] = pri_add;
			  break;
		  }

		if (mode & CECT_CONSIDER_ALL)
		  {
			int pri = priorities[num_types];

			if (pri < 0)
			  pri = (pri / 3) + 3334;	// map raw values of [-10,000..-1] to [1..3,334]
			else if (pri > 0)
			  pri = (99 * pri) + 9999;	// map raw values of [1..10,000] to [10098..999999] (the maximum priority DIALOG() can take and still treat normally)
			else
			  pri = 5775;				// map raw value of 0 to 5,775 == sqrt(3) times as likely as raw value of -1, and 1/sqrt(3) times as likely as raw value of 1

			priorities[num_types] = pri;
		  }

		++num_types;
	  }

  if (num_types == 0)
	{
	  if (!(mode & CECT_AI_CAN_CANCEL))
		cancel = 1;
	  return COUNTER_invalid;
	}

  if (mode & CECT_AUTOCHOOSE_BEST)
	{
	  int best_pri = INT_MIN;
	  counter_t best = COUNTER_invalid;
	  for (i = 0; i < num_types; ++i)
		if (priorities[i] > best_pri)
		  {
			best_pri = priorities[i];
			best = i;
		  }

	  if (best_pri <= 0 && (mode & (IS_AI(who_chooses) ? CECT_HUMAN_CAN_CANCEL : CECT_AI_CAN_CANCEL)))
		{
		  if (!(mode & CECT_AI_CAN_CANCEL))
			cancel = 1;
		  return COUNTER_invalid;
		}
	  else
		return counter_types[best];
	}

  if (mode & CECT_AI_CAN_CANCEL)
	{
	  mode &= ~CECT_HUMAN_CAN_CANCEL;	// We'll be displaying a cancel option with this, so suppress the automatic one
	  strcpy(choices[num_types], EXE_STR(0x7281A4));	// localized Cancel
	  counter_types[num_types] = COUNTER_invalid;

	  if (mode & CECT_CONSIDER_ALL)
		priorities[num_types] = 5775;	// same as a raw value of 0 would get
	  else
		priorities[num_types] = 1;
	}

#undef ADD_TYPE

  const char* prompt;
  if (mode & CECT_MOVE)
	prompt = "Move a counter";
  else if (mode & CECT_ADD_OR_REMOVE)
	prompt = "Add or remove a counter";
  else if (mode & CECT_REMOVE)
	prompt = "Remove a counter";
  else
	prompt = "Add a counter";

  int choice;

#define ADD_CHOICE(x)	choices[x][0] ? choices[x] : NULL, 1, priorities[x]
#define ADD_CHOICES		ADD_CHOICE(0), ADD_CHOICE(1), ADD_CHOICE(2), ADD_CHOICE(3), ADD_CHOICE(4), ADD_CHOICE(5), ADD_CHOICE(6), ADD_CHOICE(7), ADD_CHOICE(8), ADD_CHOICE(9)

  // We avoid DLG_OMIT_ILLEGAL by making sure we pass NULL when we run out of choices, and not having any valid choices after invalid ones.
  choice = DIALOG(src_player, src_card, EVENT_ACTIVATE,
				  DLG_FULLCARD(t_player, t_card), DLG_SMALLCARD(smallcard_player, smallcard_card),
				  DLG_HEADER(prompt), DLG_WHO_CHOOSES(who_chooses), DLG_NO_STORAGE, DLG_RANDOM,
				  (mode & CECT_HUMAN_CAN_CANCEL ? DLG_NO_OP : DLG_NO_CANCEL),
				  (mode & CECT_AI_NO_DIALOG ? DLG_NO_DISPLAY_FOR_AI : DLG_NO_OP),
				  ADD_CHOICES);

#undef ADD_CHOICE
#undef ADD_CHOICES

  if (choice == 0 || counter_types[choice - 1] == COUNTER_invalid)
	{
	  if (!(mode & CECT_AI_CAN_CANCEL))
		cancel = 1;
	  return COUNTER_invalid;
	}
  else
	return counter_types[choice - 1];
}
