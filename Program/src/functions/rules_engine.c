#include <limits.h>
#include "manalink.h"

extern int mulligans_complete[2];

int hack_allow_sorcery_if_rules_engine_is_only_effect = 0;

static int vanguard_avatar[2][10];
static int vcount[2] = {0, 0};
#ifdef CARDS_IN_HAND_THIS_TURN	// unused
static int cards_in_hand_this_turn[2][100];
#endif
static int settings[SETTING_MAX_PLUS_1] = { 0 };
static event_t actual_event = -1;
static int flashback = 0;
static int challenge = 0;
static int rules_engine_card[2] = {-1,-1};
static int deadbox_card[2] = {-1,-1};
static int starting_life_total[2] = {20,20};
static int force_init_upkeep = 0;

/* Current targets usage
 * [0], [1], [2], [4]:	activate_cards_in_hand().  No need to exhaustively sort them out just yet.
 * [3]:					Unused, but currently set to -1 in activate_cards_in_hand()'s EVENT_ACTIVATE handler.
 * [5]-[17]:			Assigned in rules_engine_globals_t.
 * [18]:				Counter storage. */

// Storage is contiguous with Rules Engine's full targets array.
typedef struct
{
  target_t targets[5];	// For use in activate_cards_in_hand().
  uint16_t shorts_per_turn[TRAP_NUMSHORTS_SHORTS_PER_TURN];
  uint8_t unassigned1[20];	// Unassigned storage is spaced out so that values of any given size can be added without invalidating savegames.
  uint16_t shorts_persistent[TRAP_NUMSHORTS_SHORTS_PERSISTENT];
  uint8_t unassigned2[16];
  uint8_t bytes_per_turn[TRAP_NUMBYTES_BYTES_PER_TURN];
  uint8_t unassigned3[10];
  uint8_t bytes_persistent[TRAP_NUMBYTES_BYTES_PERSISTENT];
  uint8_t unassigned4[6];
  uint8_t nybbles_per_turn[TRAP_NUMBYTES_NYBBLES_PER_TURN];
  uint8_t unassigned5[5];
  uint8_t bits_per_turn[TRAP_NUMBYTES_BITS_PER_TURN];
  uint8_t unassigned6[3];
  uint8_t bits_persistent[TRAP_NUMBYTES_BITS_PERSISTENT];
  uint8_t unassigned7[3];
  target_t counters;
} PACKED rules_engine_globals_t;

STATIC_ASSERT(sizeof(rules_engine_globals_t) == sizeof(((card_instance_t*)(NULL))->targets), sizeof_rules_engine_globals_t_must_match_target_array);

int get_setting(int thing){
	return settings[thing];
}

typedef enum {
	SPM_NONE = 0,
	SPM_MOMIR = 1,
	SPM_DRAFT = 2,
	SPM_TESTING = 3
} singleplayer_mode_t;
extern singleplayer_mode_t singleplayer_mode;

// exe interface - no longer used
void set_draft(void){
}
void set_momir(void){
}
void set_testing(void){
}
void set_none(void){
}

int get_momir(void){
	return singleplayer_mode == SPM_MOMIR;
}

static int get_challenge(void){
	return challenge;
}

void set_challenge(int m){
	challenge = m;
	set_challenge_round_to_0();
	ante = 0;
}

int unlimited_allowed_in_deck(int csvid)
{
  static int unlimited_list[] =
	{
	  CARD_ID_FOREST, CARD_ID_ISLAND, CARD_ID_MOUNTAIN, CARD_ID_PLAINS, CARD_ID_SWAMP,
	  //CARD_ID_SNOW_COVERED_FOREST, CARD_ID_SNOW_COVERED_ISLAND, CARD_ID_SNOW_COVERED_MOUNTAIN, CARD_ID_SNOW_COVERED_PLAINS, CARD_ID_SNOW_COVERED_SWAMP,
	  CARD_ID_RELENTLESS_RATS,
	  CARD_ID_SHADOWBORN_APOSTLE
	};

  unsigned int i;
  for (i = 0; i < sizeof(unlimited_list) / sizeof(int); ++i)
	if (unlimited_list[i] == csvid)
	  return 1;

  return 0;
}

static int check_deck_legality(void){
	int vintage_legal = 1;
	int legacy_legal = 1;
	int illegal_card = -1;
	static int restricted_cards_vintage[] = { CARD_ID_ANCESTRAL_RECALL, CARD_ID_BALANCE, CARD_ID_BLACK_LOTUS,
											  CARD_ID_BRAINSTORM, CARD_ID_BURNING_WISH, CARD_ID_CHANNEL,
											  CARD_ID_DEMONIC_CONSULTATION, CARD_ID_DEMONIC_TUTOR,
											  CARD_ID_FASTBOND, CARD_ID_FLASH, CARD_ID_GIFTS_UNGIVEN,
											  CARD_ID_IMPERIAL_SEAL, CARD_ID_LIBRARY_OF_ALEXANDRIA,
											  CARD_ID_LIONS_EYE_DIAMOND, CARD_ID_LOTUS_PETAL, CARD_ID_MANA_CRYPT,
											  CARD_ID_MANA_VAULT, CARD_ID_MEMORY_JAR, CARD_ID_MERCHANT_SCROLL,
											  CARD_ID_MINDS_DESIRE, CARD_ID_MOX_EMERALD, CARD_ID_MOX_JET,
											  CARD_ID_MOX_PEARL, CARD_ID_MOX_RUBY, CARD_ID_MOX_SAPPHIRE,
											  CARD_ID_MYSTICAL_TUTOR, CARD_ID_NECROPOTENCE, CARD_ID_PONDER,
											  CARD_ID_REGROWTH, CARD_ID_SOL_RING, CARD_ID_STRIP_MINE,
											  CARD_ID_THIRST_FOR_KNOWLEDGE, CARD_ID_TIME_VAULT, CARD_ID_TIME_WALK,
											  CARD_ID_TIMETWISTER, CARD_ID_TINKER, CARD_ID_TOLARIAN_ACADEMY,
											  CARD_ID_TRINISPHERE, CARD_ID_VAMPIRIC_TUTOR, CARD_ID_WHEEL_OF_FORTUNE,
											  CARD_ID_WINDFALL, CARD_ID_YAWGMOTHS_BARGAIN, CARD_ID_YAWGMOTHS_WILL };

	static int banned_cards_vintage[] = { CARD_ID_AMULET_OF_QUOZ, CARD_ID_BRONZE_TABLET, CARD_ID_CONTRACT_FROM_BELOW,
										  CARD_ID_DARKPACT, CARD_ID_DEMONIC_ATTORNEY, CARD_ID_FALLING_STAR,
										  CARD_ID_JEWELED_BIRD, CARD_ID_REBIRTH, CARD_ID_SHAHRAZAD,
										  CARD_ID_TEMPEST_EFREET, CARD_ID_TIMMERIAN_FIENDS,

										  CARD_ID_DUH, CARD_ID_JACK_IN_THE_MOX };	// un-sets

	static int banned_cards_legacy[] = { CARD_ID_AMULET_OF_QUOZ, CARD_ID_ANCESTRAL_RECALL, CARD_ID_BALANCE,
										 CARD_ID_BAZAAR_OF_BAGHDAD, CARD_ID_BLACK_LOTUS, CARD_ID_BLACK_VISE,
										 CARD_ID_BRONZE_TABLET, CARD_ID_CHANNEL, CARD_ID_CONTRACT_FROM_BELOW,
										 CARD_ID_DARKPACT, CARD_ID_DEMONIC_ATTORNEY, CARD_ID_DEMONIC_CONSULTATION,
										 CARD_ID_DEMONIC_TUTOR, CARD_ID_EARTHCRAFT, CARD_ID_FALLING_STAR,
										 CARD_ID_FASTBOND, CARD_ID_FLASH, CARD_ID_FRANTIC_SEARCH,
										 CARD_ID_GOBLIN_RECRUITER, CARD_ID_GUSH, CARD_ID_HERMIT_DRUID,
										 CARD_ID_IMPERIAL_SEAL, CARD_ID_JEWELED_BIRD, CARD_ID_LAND_TAX,
										 CARD_ID_LIBRARY_OF_ALEXANDRIA, CARD_ID_MANA_CRYPT, CARD_ID_MANA_DRAIN,
										 CARD_ID_MANA_VAULT, CARD_ID_MEMORY_JAR, CARD_ID_MENTAL_MISSTEP,
										 CARD_ID_MIND_TWIST, CARD_ID_MINDS_DESIRE, CARD_ID_MISHRAS_WORKSHOP,
										 CARD_ID_MOX_EMERALD, CARD_ID_MOX_JET, CARD_ID_MOX_PEARL, CARD_ID_MOX_RUBY,
										 CARD_ID_MOX_SAPPHIRE, CARD_ID_MYSTICAL_TUTOR, CARD_ID_NECROPOTENCE,
										 CARD_ID_OATH_OF_DRUIDS, CARD_ID_REBIRTH, CARD_ID_SHAHRAZAD,
										 CARD_ID_SKULLCLAMP, CARD_ID_SOL_RING, CARD_ID_STRIP_MINE,
										 CARD_ID_SURVIVAL_OF_THE_FITTEST, CARD_ID_TEMPEST_EFREET,
										 CARD_ID_TIME_VAULT, CARD_ID_TIME_WALK, CARD_ID_TIMETWISTER,
										 CARD_ID_TIMMERIAN_FIENDS, CARD_ID_TINKER, CARD_ID_TOLARIAN_ACADEMY,
										 CARD_ID_VAMPIRIC_TUTOR, CARD_ID_WHEEL_OF_FORTUNE, CARD_ID_WINDFALL,
										 CARD_ID_WORLDGORGER_DRAGON, CARD_ID_YAWGMOTHS_BARGAIN,
										 CARD_ID_YAWGMOTHS_WILL,

										 CARD_ID_DUH, CARD_ID_JACK_IN_THE_MOX };	// un-sets

	int *deck = deck_ptr[HUMAN];
	int new_deck[500];
	int i;
	unsigned int j=0;
	for(i = 0; i < 500; i++){
		new_deck[i] = -1;
	}
	for(i = 0; i < 500; i++){
		if( deck[i] > -1 ){
			new_deck[i] = cards_data[ deck[i] ].id;
		}
		else{
			new_deck[i] = -1;
			j = i;
			i = 500;
		}
	}
	int count = 0;
	/*
	while( count < active_cards_count[HUMAN]){
		if(! in_play(HUMAN, count) ){
			new_deck[j++] = get_id(HUMAN, count);
		}
		count++;
	}
	*/

	// deck too small
	if( count_deck(HUMAN) < 52 && gauntlet_round == 0 ){
		return 0;
	}

	// This counts the number of every single card in existence (!) in the deck, rather than just the cards in the deck.
	for(i=0;i<available_slots;i++){
		count = 0;
		for(j = 0;j<500;j++){
			if( new_deck[j] == i ){ count++; }
		}

		if( count > 4 ){
			if (!unlimited_allowed_in_deck(i)){
				int card_added =  add_card_to_hand(HUMAN, get_internal_card_id_from_csv_id(i)  );
				card_instance_t *target = get_card_instance(HUMAN, card_added);
				target->state |= STATE_INVISIBLE;
				--hand_count[HUMAN];
				return card_added;
			}
		}

		if( vintage_legal && count > 1 ){
			for(j = 0;j<sizeof(restricted_cards_vintage)/sizeof(int);j++){
				if( restricted_cards_vintage[j] == i ){
					vintage_legal = 0;
					illegal_card = i;
				}
			}
		}

		if( vintage_legal && count > 1 ){
			for(j = 0;j<sizeof(banned_cards_vintage)/sizeof(int);j++){
				if( banned_cards_vintage[j] == i ){
					vintage_legal = 0;
					illegal_card = i;
				}
			}
		}

		if( legacy_legal && count > 0 ){
			for(j = 0;j<sizeof(banned_cards_legacy)/sizeof(int);j++){
				if( banned_cards_legacy[j] == i ){
					legacy_legal = 0;
					illegal_card = i;
				}
			}
		}
	}

	if( legacy_legal || vintage_legal ){
		return -1;
	}
	int card_added =  add_card_to_hand(HUMAN, get_internal_card_id_from_csv_id(illegal_card)  );
	card_instance_t *target = get_card_instance(HUMAN, card_added);
	target->state |= STATE_INVISIBLE;
	--hand_count[HUMAN];
	return card_added;
}

void read_settings(void){
	// Only read config once
	if (settings[SETTING_SETTING_FILE_READ] == 0){
		settings[SETTING_RULES_ENGINE] = 1;	// RulesEngine defaults on
		settings[SETTING_SETTING_FILE_READ] = 1;

		// if the config file exists, then read in the config settings
		char buffer[200];
		FILE* file = fopen("config.txt", "r");
		if( file != NULL ){
			settings[SETTING_SETTING_FILE_READ] = 2;
			settings[SETTING_MIN_PICS_TO_RANDOMIZE] = 15;
			while( fgets(buffer, 200, file) ){
				const char* p;
				// skip whitespace
				for (p = buffer; *p && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'); ++p) {}

				switch (*p){
#define CFG(str, settingnum)													\
						if (strncasecmp(p, str ":", strlen(str ":")) == 0){		\
							settings[settingnum] = atoi(p + strlen(str ":"));	\
							continue;											\
						}
					case '/': case 0:
						// Ignore blank lines and lines starting with /
						continue;
					case 'a': case 'A':
						CFG("AiDecisionTime", SETTING_AI_DECISION_TIME);
						CFG("AiUsesRulesEngine", SETTING_AI_USES_RULES_ENGINE);
						CFG("AlphabetizeDeck", SETTING_ALPHABETIZE_DECK);
						CFG("Archenemy", SETTING_ARCHENEMY);
						CFG("AttachEquipment", SETTING_ATTACH_EQUIPMENT);
						CFG("AutoHaste", SETTING_AUTO_HASTE);
						CFG("AutoMill", SETTING_AUTO_MILL);
						CFG("AutotapArtifacts", SETTING_AUTOTAP_ARTIFACTS);
						CFG("AutotapCreatures", SETTING_AUTOTAP_CREATURES);
						break;
					case 'c': case 'C':
						CFG("ChallengeMode", SETTING_CHALLENGE_MODE);
						CFG("CheckDeckLegality", SETTING_CHECK_DECK_LEGALITY);
						CFG("CrippleAI", SETTING_CRIPPLE_AI);
						break;
					case 'd': case 'D':
						CFG("DuhMode", SETTING_DUH_MODE);
						CFG("Debug", SETTING_DEBUG);
						break;
					case 'h': case 'H':
						CFG("HandSize", SETTING_HAND_SIZE);
						break;
					case 'm': case 'M':
						CFG("ManaBurn", SETTING_MANA_BURN);
						CFG("MinPicsToRandomize", SETTING_MIN_PICS_TO_RANDOMIZE);
						break;
					case 'q': case 'Q':
						CFG("QuickStart", SETTING_QUICK_START);
						break;
					case 'r': case 'R':
						CFG("RulesEngine", SETTING_RULES_ENGINE);
						break;
					case 's': case 'S':
						CFG("SmallCardSize", SETTING_SMALLCARD_SIZE);
						CFG("SmartTarget", SETTING_SMART_TARGET);
						break;
					case 't': case 'T':
						CFG("TournamentMode", SETTING_TOURNAMENT_MODE);
						break;
					case 'u': case 'U':
						CFG("UnlimitedMana", SETTING_UNLIMITED_MANA);
						break;
					case 'v': case 'V':
						CFG("VerifyHandCount", SETTING_VERIFY_HAND_COUNT);
						break;
#undef CFG
				}
				popup("config.txt", "Unknown line in config.txt: \"%s\"", buffer);
			}
			fclose(file);
		}
	}
}

static void read_and_adjust_settings(void){

	read_settings();

	// if the challenge file exists, then add the challenge flag and remove debug modes
	if (settings[SETTING_CHALLENGE_MODE] || get_challenge()){
		settings[SETTING_CHALLENGE_MODE] = 1;
		debug_mode = 0;
		settings[SETTING_AUTO_MILL] = 0;
		settings[SETTING_UNLIMITED_MANA] = 0;
		settings[SETTING_AUTO_HASTE] = 0;
		settings[SETTING_QUICK_START] = 0;
		settings[SETTING_CRIPPLE_AI] = 0;
		settings[SETTING_DEBUG] = 0;
	} else {
		// Set exe debug mode
		if (settings[SETTING_DEBUG]){
			debug_mode = 1;
			trace_mode = 0;
		}
		// if we are testing, turn on crippleAI, autohaste, unlimited mana, and
		// quickstart and automill
		if (singleplayer_mode == SPM_TESTING){
			force_init_upkeep = 1;
			settings[SETTING_AUTO_MILL] = 1;
			settings[SETTING_UNLIMITED_MANA] = 1;
			settings[SETTING_AUTO_HASTE] = 1;
			settings[SETTING_QUICK_START] = 1;
			settings[SETTING_CRIPPLE_AI] = 1;
		}
	}

	/* Make sure rules_engine_card and deadbox_card globals are correct.  They need to be reset at each new game and each loaded game, since their placement
	 * will vary on how many times you mulligan and whether they were added midgame by a Wish card or such. */
	int p, c;
	for (p = 0; p <= 1; ++p){
		rules_engine_card[p] = -1;
		deadbox_card[p] = -1;
		for (c = 0; c < 150; ++c){
			if (in_play(p, c)){
				int csvid = get_id(p, c);
				if (csvid == CARD_ID_RULES_ENGINE){
					rules_engine_card[p] = c;
				}
				if (csvid == CARD_ID_DEADBOX){
					deadbox_card[p] = c;
				}
			}
		}
	}
}

static UnorderedMapIntInt card_image_numbers = NULL;

int get_card_image_number(int csvid, int player, int card)
{
  // 0x468160

  // This surely belongs somewhere else.  But so do settings.

  if (__builtin_expect(csvid < 0 || player < 0 || card < 0, 0))
	return 0;

  int num_pics = cards_ptr[csvid]->num_pics;
  if (__builtin_expect(num_pics <= 1, 1))
	return 0;

  if (__builtin_expect(num_pics < settings[SETTING_MIN_PICS_TO_RANDOMIZE], 1))
	return (card + player) % num_pics;

  int key = (LOWORD(csvid) << 16) | (BYTE0(player) << 8) | BYTE0(card);
  int* prev_value = UnorderedMapIntInt_fetch(&card_image_numbers, key);
  if (__builtin_expect(prev_value != NULL, 1))
	return *prev_value;

  return *(UnorderedMapIntInt_set(&card_image_numbers, key)) = internal_rand(num_pics);

  /* card_image_numbers should likely be cleared when loading a save game or when starting a new one, but the earliest existing injection at game start happens
   * after cards are drawn. */
}

void pregame(void){

	/* This called after *every card drawn* during the initial draw.
	 * There isn't really much of a better place to put it; any earlier and libraries won't have been initialized. */

	hack_allow_sorcery_if_rules_engine_is_only_effect = 0;

	if (game_type != GAMETYPE_SOLO_DUEL){
		singleplayer_mode = 0;	// Seems to be handled by the dialog code already, but I can't find it, and no reason to risk not clearing it.
	}

	int i;
	for (i = 0; i < 50; ++i){
		is_unlocked(0, 0, 0, i);
	}
	// Check for vanguard avatars in the deck or already drawn
	int p, c;
	for (p = 0; p < 2; ++p){
		if( vcount[p] == 0 ){
			mulligans_complete[p] = 0;
			int *deck = deck_ptr[p];
			for (i = 0; i < 500 && deck[i] != -1; i++){
				if (cards_data[deck[i]].cc[2] == 3){
					int csvid = cards_data[deck[i]].id;
					if (csvid == CARD_ID_AVATAR_MOMIR_VIG){
						singleplayer_mode = SPM_MOMIR;
					}
					if( vcount[p] < 10 ){
						vanguard_avatar[p][vcount[p]] = csvid;
						vcount[p]++;
					}
				}
			}
			for (c = 0; c < active_cards_count[p]; ++c){
				card_instance_t* instance = get_card_instance(p, c);
				if (instance->internal_card_id >= 0 && cards_data[instance->internal_card_id].cc[2] == 3){
					int csvid = cards_data[instance->internal_card_id].id;
					if (csvid == CARD_ID_AVATAR_MOMIR_VIG){
						singleplayer_mode = SPM_MOMIR;
					}
					if( vcount[p] < 10 ){
						vanguard_avatar[p][vcount[p]] = csvid;
						vcount[p]++;
					}
				}
			}
		}
	}


	// if the draft file exists, then add the draft avatar
	if (singleplayer_mode == SPM_DRAFT){
		vanguard_avatar[HUMAN][0] = CARD_ID_DRAFT;
		vcount[HUMAN] = 1;
		vcount[AI] = 0;
	}
	else if (singleplayer_mode == SPM_MOMIR){
		vanguard_avatar[HUMAN][0] = CARD_ID_AVATAR_MOMIR_VIG;
		vcount[HUMAN] = 1;
		vanguard_avatar[AI][0] = CARD_ID_AVATAR_MOMIR_VIG;
		vcount[AI] = 1;
	}

	settings[SETTING_GAMESTART_SETTINGS_APPLIED] = 0;
	read_and_adjust_settings();

	if( vcount[AI] == 0 && get_setting(SETTING_ARCHENEMY) && !get_setting(SETTING_CHALLENGE_MODE)){
		int draft_found = 0;
		int k;
		for(k=0; k<10; k++){
			if( vanguard_avatar[HUMAN][k] == CARD_ID_DRAFT ){
				draft_found++;
				break;
			}
		}
		if( ! draft_found ){
			vanguard_avatar[AI][0] = CARD_ID_ARCHENEMY;
			vcount[AI] = 1;
		}
	}
}

void after_load_game(void)
{
	hack_allow_sorcery_if_rules_engine_is_only_effect = 0;

	// Not strictly correct if the game was saved in turn one, but wouldn't have worked anyway since the other settings were off, too.
	// ...I think.  Challenge setting might get forced on, in which case, challenge_mode_upkeep() should still be called (once).
	// On yet a third hand, if that's true, it was *always* getting called the first turn after a load, which was even worse.
	settings[SETTING_GAMESTART_SETTINGS_APPLIED] = 1;

	read_and_adjust_settings();
}

int quick_start(void){

	if( vcount[HUMAN] ){
		return 1;
	}

	return get_setting(SETTING_QUICK_START);
}

// Has a code pointer assigned, but if the exe calls it, I can't find it.
int fake_random(int num){
	return 0;
}

int is_hidden_agenda(int csvid){
	if( csvid == CARD_ID_CONSPIRACY_BRAGOS_FAVOR ||
		csvid == CARD_ID_CONSPIRACY_DOUBLE_STROKE ||
		csvid == CARD_ID_CONSPIRACY_IMMEDIATE_ACTION ||
		csvid == CARD_ID_CONSPIRACY_ITERATIVE_ANALYSIS ||
		csvid == CARD_ID_CONSPIRACY_MUZZIOS_PREPARATIONS ||
		csvid == CARD_ID_CONSPIRACY_SECRET_SUMMONING ||
		csvid == CARD_ID_CONSPIRACY_SECRETS_OF_PARADISE ||
		csvid == CARD_ID_CONSPIRACY_UNEXPECTED_POTENTIAL
	  ){
		return 1;
	}
	return 0;
}

static void vanguard_check(void){
	// put the vanguard avatars into play
	int p;
	for(p=0;p<2;p++){
		if( ! get_setting(SETTING_CHALLENGE_MODE) ){
			int k;
			for(k=0; k<vcount[p]; k++){
				if( vanguard_avatar[p][k] != -1 ){
					if( ! is_hidden_agenda(vanguard_avatar[p][k]) ){
						int card_added = add_card_to_hand(p, get_internal_card_id_from_csv_id(vanguard_avatar[p][k]));
						put_into_play(p, card_added);
						vanguard_avatar[p][k] = -1;
					}
					else{
						int card_added = add_card_to_hand(p, get_internal_card_id_from_csv_id(CARD_ID_CONSPIRACY_HIDDEN_AGENDA));
						card_instance_t *instance= get_card_instance(p, card_added);
						instance->targets[13].player = CARD_ID_CONSPIRACY_HIDDEN_AGENDA ;
						instance->targets[13].card = vanguard_avatar[p][k];
						put_into_play(p, card_added);
						vanguard_avatar[p][k] = -1;
					}
				}
			}
			vcount[p] = 0;
		}
		else{
			if( p == 0 ){
				apply_challenge_vanguard();
			}
		}
	}
}

// Perform any upkeep triggers for cards in the graveyard
static void resolve_graveyard_upkeep_triggers(int player, int card, event_t event ){
	card_instance_t *instance= get_card_instance(player, card);
	if( current_turn == player ){

		// if any abilities were found, do the upkeep trigger
		if(trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			int grave_count = count_graveyard(player);
			const int *graveyard = get_grave(player);
			int i = 0;
			int ability_found = 0;
			int c1 = count_upkeeps(player);
			while(i<grave_count && ability_found != 1){
					card_data_t* card_d = &cards_data[ graveyard[i] ];
					if( card_d->id == CARD_ID_DEATH_SPARK ){
						if( graveyard[i+1] != -1 && (cards_data[ graveyard[i+1] ].type & TYPE_CREATURE) ){
							ability_found = 1;
						}
					}
					else if( card_d->cc[2] == 5 || card_d->id == CARD_ID_ETERNAL_DRAGON || ( player == AI && card_d->id == CARD_ID_NIM_DEVOURER) ||
							( player == AI && card_d->id == CARD_ID_NECROSAVANT) || ( player == AI && card_d->id == CARD_ID_UNDEAD_GLADIATOR)
						 ){
							ability_found = 1;
					}
					i++;
			}
			if( ability_found && c1 > 0){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						while( c1 > 0 ){
							i = grave_count-1;
							while( i > -1 ){
								card_data_t* card_d = &cards_data[ graveyard[i] ];
								if( card_d->id != CARD_ID_DEATH_SPARK ){
									if( card_d->cc[2] == 5 || card_d->id == CARD_ID_ETERNAL_DRAGON ||
										( player == AI && card_d->id == CARD_ID_NIM_DEVOURER) ||
										( player == AI && card_d->id == CARD_ID_NECROSAVANT)
									  ){
										// put a copy of the card into your hand
										int card_added =  add_card_to_hand(player, graveyard[i] );

										instance->number_of_targets = 1;
										card_instance_t *target = get_card_instance(player, card_added);
										target->state |= STATE_INVISIBLE;
										--hand_count[player];

										int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
										int result = ptFunction(player, card_added, EVENT_GRAVEYARD_ABILITY_UPKEEP);
										if( result == -1 ){ // this card is removed
											remove_card_from_grave(player, i);
										}
										else if( result == -2 ){ // -2 means no card is removed
											obliterate_card_and_recycle(player, card_added);
										}
										else{  // another card was removed
											obliterate_card_and_recycle(player, card_added);
											remove_card_from_grave(player, result);
										}
									}
								}

								else{
										if( graveyard[i+1] != -1 && (cards_data[ graveyard[i+1] ].type & TYPE_CREATURE) ){
											int card_added =  add_card_to_hand(player, graveyard[i] );
											card_instance_t *target = get_card_instance(player, card_added);
											target->state |= STATE_INVISIBLE;
											--hand_count[player];

											int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
											int result = ptFunction(player, card_added, EVENT_GRAVEYARD_ABILITY_UPKEEP);
											if( result == -1 ){ // this card is removed
												remove_card_from_grave(player, i);
											}
											else if( result == -2 ){ // -2 means no card is removed
													obliterate_card_and_recycle(player, card_added);
											}
											else{  // another card was removed
												obliterate_card_and_recycle(player, card_added);
												remove_card_from_grave(player, result);
											}
										}
								}
								i--;
							}
							c1--;
						}
				}
			}
		}
	}
}

static void resolve_dredge_triggers(int player, int card, event_t event ){
	int i;

	// Check for dredge
	if(trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw){
		// see if there are any cards to dredge
		int grave_count = count_graveyard(player);
		const int *graveyard = get_grave(player);
		int ability_found = 0;
		char buffer[500];
		int pos = scnprintf(buffer, 500, "Dredge:\n Pass\n" );
		for(i=0;i<grave_count;i++){
			card_data_t* card_d = &cards_data[ graveyard[i] ];
			if( card_d->cc[2] == 6 ){
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				int result = ptFunction(player, 0, EVENT_GRAVEYARD_ABILITY);
				if( count_deck(player) >= result ){
					ability_found = 1;
					card_ptr_t* c = cards_ptr[ card_d->id ];
					pos += scnprintf(buffer + pos, 500-pos, " %s (%d)\n", c->name, result );
				}
			}
		}

		if( ability_found ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
				if( choice != 0 ){
				   int valid = 0;
				   for(i=0;i<grave_count;i++){
						card_data_t* card_d = &cards_data[ graveyard[i] ];
						if( card_d->cc[2] == 6 ){
							int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
							int result = ptFunction(player, 0, EVENT_GRAVEYARD_ABILITY);
							if( count_deck(player) >= result && ++valid == choice ){
								from_grave_to_hand(player, i, TUTOR_HAND);
								mill( player, result );
								suppress_draw = 1;
							}
						}
					}
				}
			}
		}
	}
}
/*
static int is_unearth(const card_data_t* cd)
{
	* Unearth.  (cc[2] == 9 or Viscera Dragger), is type creature, and is not any of:
   * Carrionette
   * Champion of Stray Souls
   * Chronosavant
   * Firewing Phoenix
   * Gangrenous Goliath
   * Glory
   * Gravecrawler
   * Jarad, Golgari Lich Lord
   * Masked Admirers
   * Necrosavant
   * Rot Farm Skeleton
   * Salvage Titan
   * Sekki, Seasons' Guide
   * Shard Phoenix
   * Skaab Ruinator
   * Tymaret, the Murder King
   * Worldheart Phoenix
   *
   * Why in the bloody blue blazes are all of those *and* unearth *and* flashback *and* planeswalkers all sharing cc[2]==9?
   * There's 256 possible values and we've used only 20 of them.
   */
/*
  if (cd->cc[2] != 9 && cd->id != CARD_ID_VISCERA_DRAGGER)
	return 0;

  if (!(cd->type & TYPE_CREATURE))
	return 0;

  switch (cd->id)
	{
	  case CARD_ID_CHRONOSAVANT:
	  case CARD_ID_FIREWING_PHOENIX:
	  case CARD_ID_GANGRENOUS_GOLIATH:
	  case CARD_ID_GLORY:
	  case CARD_ID_GRAVECRAWLER:
	  case CARD_ID_JARAD_GOLGARI_LICH_LORD:
	  case CARD_ID_MASKED_ADMIRERS:
	  case CARD_ID_NECROSAVANT:
	  case CARD_ID_ROT_FARM_SKELETON:
	  case CARD_ID_SALVAGE_TITAN:
	  case CARD_ID_SEKKI_SEASONS_GUIDE:
	  case CARD_ID_SHARD_PHOENIX:
	  case CARD_ID_SKAAB_RUINATOR:
	  case CARD_ID_TYMARET_THE_MURDER_KING:
	  case CARD_ID_WORLDHEART_PHOENIX:
		return 0;
	}

  return 1;
}
*/

static int activate_cards_in_hand(int player, int card, event_t event){
#pragma message "All of the special cases here are *odious*.  Should be migrated to their card functions, checking the return value from the events to see what to do."
	if (IS_AI(player) && !get_setting(SETTING_AI_USES_RULES_ENGINE)){
		return 0;
	}
	card_instance_t *instance= get_card_instance(player, card);
	int homing_sliver_flag = get_trap_condition(player, TRAP_HOMING_SLIVER);

	// look for cards in hand that have abilities
	int i;
	if( event == EVENT_CAN_ACTIVATE ){
		//return 1;
		if( hand_count[player] > 0 ){
			for(i=0;i<active_cards_count[player]; i++){
				card_instance_t *hand_instance = get_card_instance(player, i);
				if( ! ( hand_instance->state & STATE_INVISIBLE ) && ! ( hand_instance->state & STATE_IN_PLAY )  ){
					int id = hand_instance->internal_card_id;
					if( id > -1 ){
						if( cards_data[id].cc[2] == 4 || cards_data[id].cc[2] == 16 || get_id(player, i) == CARD_ID_DRAGON_WINGS ||
							(homing_sliver_flag && has_subtype(player, i, SUBTYPE_SLIVER))
						  ){
							int (*ptFunction)(int, int, event_t) = (void*)cards_data[id].code_pointer;
							if( ptFunction(player, i, EVENT_CAN_ACTIVATE_FROM_HAND ) ){
								return 1;
							}
						}
					}
				}
			}
		}

		// also check for cards in the graveyard
		const int *graveyard = get_grave(player);
		if( graveyard[0] != -1 ){
			int graveyard_count = count_graveyard(player);
			int any_true = 0;
			for (i = 0; i < graveyard_count; ++i){
				card_data_t* card_d = &cards_data[ graveyard[i] ];
				if( card_d->cc[2] == 9 || card_d->id == CARD_ID_VISCERA_DRAGGER || card_d->cc[2] == 10 || card_d->cc[2] == 17 ||
					card_d->id == CARD_ID_HAAKON_STROMGALD_SCOURGE || card_d->id == CARD_ID_SKARRGAN_FIREBIRD ||
					card_d->id == CARD_ID_MARSHALING_CRY || card_d->id == CARD_ID_UNDEAD_GLADIATOR
				  ){
					int fake = add_card_to_hand(player, graveyard[i]);
					add_state(player, fake, STATE_INVISIBLE);
					--hand_count[player];

					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					int result = ptFunction(player, fake, EVENT_GRAVEYARD_ABILITY);
					obliterate_card_and_recycle(player, fake);
					if (result == 99){
						return result;
					} else if (result){
						any_true = 1;
					}
				}
			}
			return any_true;
		}

		if( check_rfg(player, CARD_ID_TORRENT_ELEMENTAL) && can_sorcery_be_played(player, event) && has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_GREEN, 3)){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
		hack_allow_sorcery_if_rules_engine_is_only_effect = 1;	// must not return from this function without clearing this
		unsigned int cb = 3000;
		char buffer[cb];
		int opts[100][2];
		int opts_count = 1;
		int pos = scnprintf(buffer, cb, " Cancel\n" );
		for (i = 0; i < active_cards_count[player]; i++){
			card_instance_t *hand_instance = get_card_instance(player, i);
			if( ! ( hand_instance->state & STATE_INVISIBLE ) && ! ( hand_instance->state & STATE_IN_PLAY )  ){
				int id = hand_instance->internal_card_id;
				if( id > -1 ){
					if( cards_data[id].cc[2] == 4 || cards_data[id].cc[2] == 16 ||
						cards_data[id].id == CARD_ID_MARSHALING_CRY  ||
						cards_data[id].id == CARD_ID_SILENT_BLADE_ONI ||
						cards_data[id].id == CARD_ID_VISCERA_DRAGGER ||
						cards_data[id].id == CARD_ID_DRAGON_WINGS ||
						(homing_sliver_flag && has_subtype(player, i, SUBTYPE_SLIVER))
					  ){
						int (*ptFunction)(int, int, event_t) = (void*)cards_data[id].code_pointer;
						if( ptFunction(player, i, EVENT_CAN_ACTIVATE_FROM_HAND ) ){
							card_ptr_t* c = cards_ptr[ get_id(player, i) ];
							pos += scnprintf(buffer + pos, cb-pos, " Activate %s\n", c->name );
							opts[opts_count][0] = i;
							opts[opts_count][1] = 1;
							opts_count++;
						}
					}
				}
			}
		}

		// also check for cards in the graveyard
		const int *graveyard = get_grave(player);
		int cage_flag = check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE);
		int graveyard_count = count_graveyard(player);
		for (i = 0; i < graveyard_count; ++i){
				card_data_t* card_d = &cards_data[ graveyard[i] ];
				if( card_d->cc[2] == 9 || card_d->id == CARD_ID_VISCERA_DRAGGER || card_d->cc[2] == 10 || card_d->cc[2] == 17 ||
					card_d->id == CARD_ID_HAAKON_STROMGALD_SCOURGE || card_d->id == CARD_ID_SKARRGAN_FIREBIRD ||
					card_d->id == CARD_ID_MARSHALING_CRY || card_d->id == CARD_ID_UNDEAD_GLADIATOR
				  ){
					int fake = add_card_to_hand(player, graveyard[i]);
					add_state(player, fake, STATE_INVISIBLE);
					--hand_count[player];

					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					int result = ptFunction(player, fake, EVENT_GRAVEYARD_ABILITY );
					if( result ){
						card_ptr_t* c = cards_ptr[ card_d->id ];
						if( (card_d->type & TYPE_CREATURE) ){
							if( ! cage_flag ){
								if( result & GA_PLAYABLE_FROM_GRAVE ){
									pos += scnprintf(buffer + pos, cb-pos, " Play %s\n", c->name );
									opts[opts_count][0] = i;
									opts[opts_count][1] = 0;
									opts_count++;
								}
								if( result & GA_UNEARTH ){
									pos += scnprintf(buffer + pos, cb-pos, " Unearth %s\n", c->name );
									opts[opts_count][0] = i;
									opts[opts_count][1] = 0;
									opts_count++;
								}
							}
							if( result & GA_BASIC ){
								if( result & GA_SCAVENGE  ){
									pos += scnprintf(buffer + pos, cb-pos, " Scavenge %s\n", c->name );
									opts[opts_count][0] = i;
									opts[opts_count][1] = 0;
									opts_count++;
								}
								else{
									pos += scnprintf(buffer + pos, cb-pos, " Activate %s\n", c->name );
									opts[opts_count][0] = i;
									opts[opts_count][1] = 0;
									opts_count++;
								}
							}
							if( result & GA_RETURN_TO_HAND ){
								pos += scnprintf(buffer + pos, cb-pos, " Return %s to hand\n", c->name );
								opts[opts_count][0] = i;
								opts[opts_count][1] = 0;
								opts_count++;
							}
							if( result & GA_RETURN_TO_PLAY ){
								pos += scnprintf(buffer + pos, cb-pos, " Return %s to play\n", c->name );
								opts[opts_count][0] = i;
								opts[opts_count][1] = 0;
								opts_count++;
							}
							if( result & GA_PUT_ON_TOP_OF_DECK ){
								pos += scnprintf(buffer + pos, cb-pos, " Put %s on top of deck\n", c->name );
								opts[opts_count][0] = i;
								opts[opts_count][1] = 0;
								opts_count++;
							}
						}
						else{
							if( result & GA_RETURN_TO_HAND ){
								pos += scnprintf(buffer + pos, cb-pos, " Return %s to hand\n", c->name );
								opts[opts_count][0] = i;
								opts[opts_count][1] = 0;
								opts_count++;
							}
							else{
								if( ! cage_flag ){
									if(card_d->cc[2] == 10 ){
										pos += scnprintf(buffer + pos, cb-pos, " Retrace %s\n", c->name );
										opts[opts_count][0] = i;
										opts[opts_count][1] = 0;
										opts_count++;
									}
									else{
										pos += scnprintf(buffer + pos, cb-pos, " Flashback %s\n", c->name );
										opts[opts_count][0] = i;
										opts[opts_count][1] = 0;
										opts_count++;
									}
								}
							}
						}
					}
					obliterate_card_and_recycle(player, fake);
				}
		}
		if( check_rfg(player, CARD_ID_TORRENT_ELEMENTAL) && can_sorcery_be_played(player, event) && has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_GREEN, 3)){
			pos += scnprintf(buffer + pos, cb-pos, " Return Torrent Elemental to play\n");
			opts[opts_count][0] = -1;
			opts[opts_count][1] = 2;
			opts_count++;
		}
		for (i = 0; i < 5; i++){
			instance->targets[i].player = -1;
			instance->targets[i].card = -1;
		}

		int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
		if( choice == 0 ){
			spell_fizzled = 1;
			hack_allow_sorcery_if_rules_engine_is_only_effect = 0;
			return 0;
		}

		// pay the costs for the selected card
		if( opts[choice][1] == 1 ){
			card_instance_t *hand_instance = get_card_instance(player, opts[choice][0]);
			int id = hand_instance->internal_card_id;
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[id].code_pointer;
			if( !ptFunction(player, opts[choice][0], EVENT_CAN_ACTIVATE_FROM_HAND) ){
				spell_fizzled = 1;
				hack_allow_sorcery_if_rules_engine_is_only_effect = 0;
				return 0;
			}

			int forecast_flag = cards_data[id].cc[2] == 16;
			int result = ptFunction(player, opts[choice][0], EVENT_ACTIVATE_FROM_HAND);

			// result = 2 when a card was cycled
			if( result == 2 && cards_data[id].cc[2] != 16){
				activate_astral_slide();
			}

			instance->targets[2].card = cards_data[id].id;
			instance->targets[4].card = cards_data[id].code_pointer;

			instance->targets[0].card = hand_instance->targets[0].card;
			instance->targets[0].player = hand_instance->targets[0].player;
			hand_instance->number_of_targets = 0;
			instance->targets[2].player = 1<<31;
			instance->targets[1].card = forecast_flag ? opts[choice][0] : -1;
		}
		else if( opts[choice][1] == 2 ){ //Actually, only Torrent Elemental
				if( charge_mana_hybrid(player, card, 2, COLOR_BLACK, COLOR_GREEN, 3) ){
					remove_card_from_rfg(player, CARD_ID_TORRENT_ELEMENTAL);
					instance->targets[2].card = CARD_ID_TORRENT_ELEMENTAL;
				}
		}
		else if( can_legally_play_iid(player, graveyard[opts[choice][0]] ) ){
				card_data_t* card_d = &cards_data[ graveyard[opts[choice][0]] ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				int fake = add_card_to_hand(player, graveyard[opts[choice][0]]);
				add_state(player, fake, STATE_INVISIBLE);
				--hand_count[player];
				int mode = ptFunction(player, fake, EVENT_GRAVEYARD_ABILITY );
				if( ! mode ){
					obliterate_card_and_recycle(player, fake);
					spell_fizzled = 1;
					hack_allow_sorcery_if_rules_engine_is_only_effect = 0;
					return 0;
				}

				int result = ptFunction(player, fake, EVENT_PAY_FLASHBACK_COSTS );

				if ( ! result || spell_fizzled == 1){
					obliterate_card(player, fake);
					spell_fizzled = 1;
					hack_allow_sorcery_if_rules_engine_is_only_effect = 0;
					return 0;
				}

				instance->targets[4].card = cards_data[graveyard[opts[choice][0]]].code_pointer;
				instance->targets[2].card = graveyard[opts[choice][0]];
				instance->targets[2].player = mode;
				if( result == GAPAID_REMOVE ){
					remove_card_from_grave(player, opts[choice][0]);
				}
				if( result == GAPAID_EXILE ){
					rfg_card_from_grave(player, opts[choice][0]);
				}
				// 	GAPAID_REMAIN_IN_GRAVE --> just leave the card in grave
				obliterate_card(player, fake);
		}
		else{
			spell_fizzled = 1;
		}

		hack_allow_sorcery_if_rules_engine_is_only_effect = 0;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			/*
			char buffer2[500];
			card_ptr_t* c1 = cards_ptr[ instance->targets[0].card ];
			scnprintf(buffer2, 500, " %d ", instance->targets[1].player);
			do_dialog(player, player, instance->parent_card, -1, -1, buffer2, 0);
			*/
			if( instance->targets[2].card == CARD_ID_TORRENT_ELEMENTAL ){
				int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_TORRENT_ELEMENTAL));
				add_state(player, card_added, STATE_TAPPED);
				put_into_play(player, card_added);
				return 0;
			}

			if( instance->targets[2].player & (1<<31) ){
				int (*ptFunction)(int, int, event_t) = (void*)instance->targets[4].card;
				ptFunction(instance->parent_controller, instance->parent_card, EVENT_RESOLVE_ACTIVATION_FROM_HAND);
				if( instance->targets[1].card > -1 ){
					card_data_t* card_d = get_card_data(player, instance->targets[1].card);
					if( card_d->cc[2] == 16 ){
						card_instance_t *this = get_card_instance( player, instance->targets[1].card );
						this->info_slot = 1;
					}
				}
			}
			else{
				int mode = instance->targets[2].player;
				if( instance->targets[2].card > -1 ){
					int internal_id = instance->targets[2].card;
					if( mode & GA_PLAYABLE_FROM_GRAVE ){
						flashback = 1;

						/* Was previously removed from graveyard; put it back and cast it from there.  Ideally, we'd want to just call this during
						 * EVENT_ACTIVATE, but that makes it impossible to set the flashback global.  play_card_in_grave_for_free_and_exile_it() probably
						 * obsolesces it, but it'll need more testing than we have time for right now with release approaching. */
						int pos = raw_put_iid_on_top_of_graveyard(player, internal_id);
						increase_trap_condition(player, TRAP_CARDS_TO_GY_FROM_ANYWHERE, -1);    // Since it's not being newly put into the graveyard
						if (cards_data[internal_id].cc[2] == 10){	// retrace - goes back to graveyard
							play_card_in_grave_for_free(player, player, pos);
						}
						else {
							play_card_in_grave_for_free_and_exile_it(player, player, pos);
						}
						flashback = 0;
					}
					else{
						int card_added = add_card_to_hand(player, internal_id);
						add_state(player, card_added, STATE_INVISIBLE);
						if( mode & GA_BASIC ){
							call_card_function(player, card_added, EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY);
							obliterate_card(player, card_added);
						}
						if( mode & GA_RETURN_TO_HAND ){
							remove_state(player, card_added, STATE_INVISIBLE);
						}
						if( mode & GA_PUT_ON_TOP_OF_DECK ){
							remove_state(player, card_added, STATE_INVISIBLE);
							put_on_top_of_deck(player, card_added);
						}

						if( mode & GA_RETURN_TO_PLAY_MODIFIED ){
							call_card_function(player, card_added, EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED);
						}

						if( mode & GA_UNEARTH ){
							convert_to_token(player, card_added);
							set_special_flags(player, card_added, SF_UNEARTH);
						}

						if( mode & (GA_RETURN_TO_PLAY | GA_UNEARTH) ){
							remove_state(player, card_added, STATE_INVISIBLE);
							put_into_play(player, card_added);
						}

						if( mode & GA_UNEARTH ){
							create_targetted_legacy_effect(player, card_added, &effect_unearth, player, card_added);
						}

						if( mode & GA_RETURN_TO_PLAY_WITH_EFFECT ){
							call_card_function(player, card_added, EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY);
						}
					}
				}
			}
	}
	// reset card with Forecast
	if( event == EVENT_CLEANUP && (get_rules_engine_infos(player) & REF_FORECAST) ){
		int count;
		for (count = 0; count < active_cards_count[player]; ++count){
			if( in_hand(player, count) ){
				card_data_t* card_d = get_card_data(player, count);
				if( card_d->cc[2] == 16 ){
					card_instance_t *crd = get_card_instance(player, count);
					crd->info_slot = 0;
				}
			}
		}
	}
	return 0;
}

int get_flashback(void){
	return flashback;
}

#ifdef CARDS_IN_HAND_THIS_TURN	// unused
static int was_card_in_hand(int player, int card){
	return 1; // comment out this code since track_hand is so slow
	int i=0;
	while( cards_in_hand_this_turn[player][i] != -1){
		if( cards_in_hand_this_turn[player][i++] == card ){
			return 1;
		}
	}
	return 0;
}
#endif

#ifdef CARDS_IN_HAND_THIS_TURN	// unused
static void track_hand(int player, int card, event_t event ){
	int i;
	if( comes_into_play(player, card, event) == 1 && player == HUMAN){
		for(i=0;i<100;i++){
			cards_in_hand_this_turn[HUMAN][i] = -1;
			cards_in_hand_this_turn[AI][i] = -1;
		}
	}

	int p;
	for (p=0;p<2;p++){
		for(i=0;i<active_cards_count[p]; i++){
			card_instance_t *card_instance = get_card_instance(p, i);
			if( ! ( card_instance->state & STATE_INVISIBLE ) && ! ( card_instance->state & STATE_IN_PLAY )  ){
				int j=0;
				while( cards_in_hand_this_turn[p][j] != -1 && j < 100 ){
					if( cards_in_hand_this_turn[p][j] == i ){
						break;
					}
					j++;
				}
				cards_in_hand_this_turn[p][j] = i;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		for(i=0;i<100;i++){
			cards_in_hand_this_turn[HUMAN][i] = -1;
			cards_in_hand_this_turn[AI][i] = -1;
		}
	}
}
#endif

static int get_rules_engine_card(int player){
	return rules_engine_card[player];
}

void increment_storm_count(int player, int mode){
	// increment the storm count and stormcreature count if it's the case
	int rec = get_rules_engine_card(player);
	if( rec == -1 ){
		return;
	}
	++hack_silent_counters;
	add_counter(player, rec, COUNTER_ENERGY);
	if( mode & STORM_CREATURE ){
		add_counter(player, rec, COUNTER_CHARGE);
	}
	--hack_silent_counters;

	if (mode & COLOR_TEST_BLACK){
		increase_trap_condition(player, TRAP_STORM_BLACK, 1);
	}
	if (mode & COLOR_TEST_BLUE){
		increase_trap_condition(player, TRAP_STORM_BLUE, 1);
	}
	if (mode & COLOR_TEST_GREEN){
		increase_trap_condition(player, TRAP_STORM_GREEN, 1);
	}
	if (mode & COLOR_TEST_RED){
		increase_trap_condition(player, TRAP_STORM_RED, 1);
	}
	if (mode & COLOR_TEST_WHITE){
		increase_trap_condition(player, TRAP_STORM_WHITE, 1);
	}
}

static void reset_storm_count(int player, int card){
	if (player != HUMAN){	// Human rules engine clears both
		return;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_WEREWOLF;

	if( can_target(&td) ){
		// This reset the storm count for both players and set the "moon phases" for werewolves
		int human_count = get_specific_storm_count(HUMAN);
		int ai_count = get_specific_storm_count(AI);

		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while(count < active_cards_count[i]){
					if( in_play(i, count) ){
						card_instance_t *instance= get_card_instance(i, count);
						if( has_subtype(i, count, SUBTYPE_WEREWOLF) && instance->targets[9].card != 66 ){
							instance->targets[8].player = human_count;
							instance->targets[8].card = ai_count;
							instance->targets[9].card = 66;
						}
						if( get_id(i, count) == CARD_ID_RULES_ENGINE ){
							set_trap_condition(i, TRAP_STORM_BLACK, 0);
							set_trap_condition(i, TRAP_STORM_BLUE, 0);
							set_trap_condition(i, TRAP_STORM_GREEN, 0);
							set_trap_condition(i, TRAP_STORM_RED, 0);
							set_trap_condition(i, TRAP_STORM_WHITE, 0);
							remove_all_counters(i, count, COUNTER_ENERGY);
							remove_all_counters(i, count, COUNTER_CHARGE);
						}
					}
					count++;
			}
		}
	}
	else{
		int p, crd;
		for (p = 0; p <= 1; ++p){
			if ((crd = get_rules_engine_card(p)) >= 0){
				set_trap_condition(p, TRAP_STORM_BLACK, 0);
				set_trap_condition(p, TRAP_STORM_BLUE, 0);
				set_trap_condition(p, TRAP_STORM_GREEN, 0);
				set_trap_condition(p, TRAP_STORM_RED, 0);
				set_trap_condition(p, TRAP_STORM_WHITE, 0);
				remove_all_counters(p, crd, COUNTER_ENERGY);
				remove_all_counters(p, crd, COUNTER_CHARGE);
			}
		}
	}
}

static void keep_storm_count(int player, int card, event_t event ){

	// keep track of storm count
	if( player == HUMAN && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && event == EVENT_END_TRIGGER
	  ){
		card_instance_t *played = get_card_instance(trigger_cause_controller, trigger_cause);
		if( ! ( cards_data[ played->internal_card_id].type & TYPE_LAND )
			//&& ! ( cards_data[ played->internal_card_id].cc[2] == 15 )
		  ){
			int mode = 0;
			if( cards_data[ played->internal_card_id].type & TYPE_CREATURE ){
				mode = STORM_CREATURE;
			}
			if( get_id(trigger_cause_controller, trigger_cause) == CARD_ID_PEER_THROUGH_DEPTHS ){
				increase_trap_condition(trigger_cause_controller, TRAP_PEER_THROUGH_DEPTHS, 1);
			}
			if( get_id(trigger_cause_controller, trigger_cause) == CARD_ID_REACH_THROUGH_MISTS ){
				increase_trap_condition(trigger_cause_controller, TRAP_REACH_THROUGH_MISTS, 1);
			}
			mode |= get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_ANY_COLORED;
			increment_storm_count(trigger_cause_controller, mode);
		}
	}

	if(event == EVENT_CLEANUP){
		reset_storm_count(player, card);
	}
}

int get_specific_storm_count(int player){

	int rec = get_rules_engine_card(player);
	if( rec == -1 ){
		return 0;
	}
	return count_counters(player, rec, COUNTER_ENERGY);
}

int get_storm_count(void){
	int total = get_specific_storm_count(AI)+get_specific_storm_count(HUMAN);
	return total;
}

int get_stormcreature_count(int k){
	int rec = get_rules_engine_card(k);
	if( rec == -1 ){
		return 0;
	}
	return count_counters(k, rec, COUNTER_CHARGE);
}

int get_stormcolor_count(int player, color_t clr)
{
  if (clr >= COLOR_BLACK && clr <= COLOR_WHITE)
	return get_trap_condition(player, (TRAP_STORM_BLACK - COLOR_BLACK) + clr);
  else
	return 0;
}

#if 0	// unused
int get_turn_count(void)
{
  return get_trap_condition(HUMAN, TRAP_TURN_COUNT) + get_trap_condition(AI, TRAP_TURN_COUNT);
}
#endif

int get_specific_turn_count(int player){
  return get_trap_condition(player, TRAP_TURN_COUNT);
}

event_t get_actual_event(void){
	return actual_event;
}

static void static_graveyard_abilities(int player, int card, event_t event){

	// Avoid searching the entire graveyard unless we'll be doing something with this event or trigger.
	switch (event){
		case EVENT_ABILITIES:
		case EVENT_GRAVEYARD_FROM_PLAY:
			break;
		case EVENT_CLEANUP:
			get_card_instance(player, card)->targets[1].player = 0;
			return;
		default:	// not a handled event.
			switch (trigger_condition){
				case TRIGGER_COMES_INTO_PLAY:
				case TRIGGER_DEAL_DAMAGE:
				case TRIGGER_SPELL_CAST:
				case TRIGGER_GRAVEYARD_FROM_PLAY:
					break;
				default:	// not a handled event or trigger
					return;
			}
	}

	// look for cards in graveyard that have static abilities
	card_instance_t *instance = get_card_instance(player, card);
	int i=0;
	const int *graveyard = get_grave(player);
	if( graveyard[0] == -1 ){
		return;
	}

	int ids[] = {   CARD_ID_BLOODGHAST, //0
					CARD_ID_VENGEVINE, //1
					CARD_ID_WONDER, //2
					CARD_ID_SWORD_OF_THE_MEEK, //3
					CARD_ID_DEARLY_DEPARTED, //4
					CARD_ID_CHANDRAS_PHOENIX, //5
					CARD_ID_BLOOD_SPEAKER, //6
					CARD_ID_SOSUKES_SUMMONS, //7
					CARD_ID_VEILBORN_GHOUL, //8
					CARD_ID_NETHER_TRAITOR,//9
					CARD_ID_MASKED_ADMIRERS,//10
					CARD_ID_AUNTIES_SNITCH,//11
					CARD_ID_REACH_OF_BRANCHES,//12
					CARD_ID_BRAWN,//13
					CARD_ID_FILTH, //14
					CARD_ID_VALOR, //15
					CARD_ID_BLADEWINGS_THRALL, //16
					-1, //17
					-1, //18
					CARD_ID_PYREWILD_SHAMAN, // 19
					CARD_ID_ANGER,	// 20
					CARD_ID_AKOUM_FIREBIRD,	// 21
	};

	enum{
		GC_BLOODGHAST = 0,
		GC_VENGEVINE,
		GC_WONDER,
		GC_SWORD_OF_THE_MEEK,
		GC_DEARLY_DEPARTED,
		GC_CHANDRAS_PHOENIX,
		GC_BLOOD_SPEAKER,
		GC_SOSUKES_SUMMONS,
		GC_VEILBORN_GHOUL,
		GC_NETHER_TRAITOR,
		GC_MASKED_ADMIRERS,
		GC_AUNTIES_SNITCH,
		GC_REACH_OF_BRANCHES,
		GC_BRAWN,
		GC_FILTH,
		GC_VALOR,
		GC_BLADEWINGS_THRALL,
		GC_DRAGON_ENCHANTMENTS,
		GC_RECOVER,
		GC_PYREWILD_SHAMAN,
		GC_ANGER,
		GC_AKOUM_FIREBIRD,

		GC_last
	};

	int counts[GC_last] = {0};

	int graveyard_count = count_graveyard(player);
	for (i = 0; i < graveyard_count; ++i){
		card_data_t* card_d = &cards_data[ graveyard[i] ];
		if( card_d->cc[2] == 18 ){
			counts[GC_DRAGON_ENCHANTMENTS]++;
		}
		if( card_d->cc[2] == 20 ){
			counts[GC_RECOVER]++;
		}
		unsigned int k;
		for(k=0; k<(sizeof(ids)/sizeof(ids[0])); k++){
			if( card_d->id == ids[k] ){
				counts[k]++;
				break;
			}
		}
	}

	enum{
		GT_CIP_BLOODGHAST 				= 1<<0,
		GT_CIP_SWORD_OF_THE_MEEK		= 1<<1,
		GT_CIP_DEARLY_DEPARTED			= 1<<2,
		GT_CIP_BLOOD_SPEAKER			= 1<<3,
		GT_CIP_SOSUKES_SUMMONS			= 1<<4,
		GT_CIP_VEILBORN_GHOUL			= 1<<5,
		GT_CIP_REACH_OF_BRANCHES		= 1<<6,
		GT_CIP_BLADEWINGS_THRALL		= 1<<7,
		GT_CIP_DRAGON_ENCHANTMENTS		= 1<<8,
		GT_CIP_AKOUM_FIREBIRD			= 1<<9,
	};

	// Bloodghast, Sword of the Meek, Dearly Departed, Blood Speaker and Sosuke's Summons
	if(	trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == player && trigger_cause_controller == player
	  ){
		int trig_mode = 0;
		if( counts[GC_BLOODGHAST] > 0 && is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig_mode |= GT_CIP_BLOODGHAST;
		}
		if( counts[GC_SWORD_OF_THE_MEEK] > 0
			&& get_power(trigger_cause_controller, trigger_cause) == 1 && get_toughness(trigger_cause_controller, trigger_cause) == 1
		 ){
			trig_mode |= GT_CIP_SWORD_OF_THE_MEEK;
		}
		if( counts[GC_DEARLY_DEPARTED] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_HUMAN)
		  ){
			trig_mode |= GT_CIP_DEARLY_DEPARTED;
		}
		if( counts[GC_BLOOD_SPEAKER] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_PERMANENT) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_DEMON)
		  ){
			trig_mode |= GT_CIP_BLOOD_SPEAKER;
		}
		if( counts[GC_SOSUKES_SUMMONS] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_SNAKE) &&
			! is_token(trigger_cause_controller, trigger_cause)
		  ){
			trig_mode |= GT_CIP_SOSUKES_SUMMONS;
		}
		if( counts[GC_VEILBORN_GHOUL] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_SWAMP)
		  ){
			trig_mode |= GT_CIP_VEILBORN_GHOUL;
		}
		if( counts[GC_REACH_OF_BRANCHES] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_FOREST)
		  ){
			trig_mode |= GT_CIP_REACH_OF_BRANCHES;
		}
		if( counts[GC_BLADEWINGS_THRALL] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_PERMANENT) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_DRAGON)
		  ){
			trig_mode |= GT_CIP_BLADEWINGS_THRALL;
		}
		if( counts[GC_DRAGON_ENCHANTMENTS] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && get_cmc(trigger_cause_controller, trigger_cause) > 5
		  ){
			trig_mode |= GT_CIP_DRAGON_ENCHANTMENTS;
		}
		if( counts[GC_AKOUM_FIREBIRD] > 0 &&
			is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && has_mana_multi(player, MANACOST_XR(4,2))
		  ){
			trig_mode |= GT_CIP_AKOUM_FIREBIRD;
		}
		if( trig_mode ){
			int otp = trigger_cause_controller;
			int otc = trigger_cause;
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int count;
					for ( count = count_graveyard(player)-1; count >= 0; --count){
							if( cards_data[graveyard[count]].id == CARD_ID_BLOODGHAST && (trig_mode & GT_CIP_BLOODGHAST) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Bloodghast to play\n Do not return", 0);
								}
								if( choice == 0 ){
									reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_SWORD_OF_THE_MEEK && (trig_mode & GT_CIP_SWORD_OF_THE_MEEK) ){
								if( duh_mode(player) || do_dialog(player, player, card, -1, -1, " Return Sword of the Meek	to play\n Pass", 0) == 0 ){
									int sword = reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
									equip_target_creature(player, sword, otp, otc);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_DEARLY_DEPARTED && (trig_mode & GT_CIP_DEARLY_DEPARTED) ){
								add_1_1_counter(otp, otc);
							}
							if( cards_data[graveyard[count]].id == CARD_ID_BLOOD_SPEAKER && (trig_mode & GT_CIP_BLOOD_SPEAKER) ){
								add_card_to_hand(player, graveyard[count] );
								remove_card_from_grave(player, count);
							}
							if( cards_data[graveyard[count]].id == CARD_ID_SOSUKES_SUMMONS && (trig_mode & GT_CIP_SOSUKES_SUMMONS) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Sosuke's Summons to hand\n Do not return", 0);
								}
								if( choice == 0 ){
									add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_VEILBORN_GHOUL && (trig_mode & GT_CIP_VEILBORN_GHOUL) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Veilborn Ghoul to hand\n Do not return", 0);
								}
								if( choice == 0 ){
									add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_REACH_OF_BRANCHES && (trig_mode & GT_CIP_REACH_OF_BRANCHES) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Reach of Branches to hand\n Do not return", 0);
								}
								if( choice == 0 ){
									add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_BLADEWINGS_THRALL && (trig_mode & GT_CIP_BLADEWINGS_THRALL) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Bladewing's Thrall to play\n Do not return", 0);
								}
								if( choice == 0 ){
									int card_added = add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
									put_into_play(player, card_added);
								}
							}
							if( cards_data[graveyard[count]].cc[2] == 18 && (trig_mode & GT_CIP_DRAGON_ENCHANTMENTS) ){
								char buffer[100];
								card_ptr_t* c = cards_ptr[ cards_data[graveyard[count]].id ];
								scnprintf(buffer, 100, " Return %s to play\n Pass", c->name);
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, buffer, 0);
								}
								if( choice == 0 ){
									int card_added = add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
									put_into_play_aura_attached_to_target(player, card_added, trigger_cause_controller, trigger_cause);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_AKOUM_FIREBIRD && (trig_mode & GT_CIP_AKOUM_FIREBIRD) ){
								if( do_dialog(player, player, card, -1, -1, " Return Akoum Firebird to bf\n Decline", 0) == 0 &&
									charge_mana_multi_while_resolving_csvid(CARD_ID_AKOUM_FIREBIRD, event, player, MANACOST_XR(4,2))
								  ){
									reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
								}
							}
					}
			}
		}
	}

	enum{
		GT_SC_VENGEVINE = 1<<0,
		GT_SC_MASKED_ADMIRERS = 1<<1,
	};

	// return any vengevine to play if the second creature spell of the turn is cast
	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		player == trigger_cause_controller
	  ){
		int trig = 0;
		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
				if( counts[GC_VENGEVINE] > 0 && get_stormcreature_count(player) == 1 ){
					trig |= GT_SC_VENGEVINE;
				}
				if( counts[GC_MASKED_ADMIRERS] > 0 && has_mana(player, COLOR_GREEN, 2) ){
					trig |= GT_SC_MASKED_ADMIRERS;
				}
			}
		}
		if( trig ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int count = count_graveyard(player)-1;
					while( count > -1 ){
							if( cards_data[graveyard[count]].id == CARD_ID_VENGEVINE && (trig & GT_SC_VENGEVINE) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1, " Return Vengevine to play\n Do not return", 0);
								}
								if( choice == 0 ){
									reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
								}
							}
							if( cards_data[graveyard[count]].id == CARD_ID_MASKED_ADMIRERS && (trig & GT_SC_MASKED_ADMIRERS) && has_mana(player, COLOR_GREEN, 2) ){
								int choice = 0;
								if( duh_mode(player) == 0  ){
									choice = do_dialog(player, player, card, -1, -1,  " Return Masked Admirers to hand\n Do not return", 0);
								}
								if( choice == 0 ){
									charge_mana(player, COLOR_GREEN, 2);
									if( spell_fizzled != 1 ){
										add_card_to_hand(player, graveyard[count] );
										remove_card_from_grave(player, count);
									}
								}
							}
							count--;
					}
			}
		}
	}

	// wonder gives flying to everyone if you control an island
	if( event == EVENT_ABILITIES && affected_card_controller == player ){
		if( in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE)  ){
			if( counts[GC_WONDER] > 0 && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) ){
				event_result |= KEYWORD_FLYING;
			}
			if( counts[GC_BRAWN] > 0 && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) ){
				event_result |= KEYWORD_TRAMPLE;
			}
			if( counts[GC_FILTH] > 0 && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) ){
				event_result |= KEYWORD_SWAMPWALK;
			}
			if( counts[GC_VALOR] > 0 && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) ){
				event_result |= KEYWORD_FIRST_STRIKE;
			}
			if( counts[GC_ANGER] > 0 && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) ){
				haste(affected_card_controller, affected_card, event);
			}
		}
	}

	enum{
		GT_DT_CHANDRAS_PHOENIX	= 1<<0,
		GT_DT_AUNTIES_SNITCH	= 1<<1,
		GT_DT_PYREWILD_SHAMAN	= 1<<2,
	};

	// return chandra's phoenix to your hand
	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		int mode = 0;
		if( get_trap_condition(player, TRAP_CHANDRAS_PHOENIX) ){
			mode |= GT_DT_CHANDRAS_PHOENIX;
		}
		if( get_trap_condition(player, TRAP_PROWL_ROGUE) || get_trap_condition(player, TRAP_PROWL_GOBLIN) ){
			mode |= GT_DT_AUNTIES_SNITCH;
		}
		if( get_trap_condition(1-player, TRAP_DAMAGED_BY_CREATURE) && has_mana(player, COLOR_COLORLESS, 3) ){
			mode |= GT_DT_PYREWILD_SHAMAN;
		}
		if( mode ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int count = count_graveyard(player)-1;
					while (count > -1){
							if( (mode & GT_DT_CHANDRAS_PHOENIX) && cards_data[graveyard[count]].id == CARD_ID_CHANDRAS_PHOENIX ){
								add_card_to_hand(player, graveyard[count] );
								remove_card_from_grave(player, count);
							}
							if( (mode & GT_DT_AUNTIES_SNITCH) && cards_data[graveyard[count]].id == CARD_ID_AUNTIES_SNITCH ){
								int choice = 0;
								if( ! duh_mode(player) ){
									choice = do_dialog(player, player, card, -1, -1, " Return Auntie's Snitch\n Pass", 0);
								}
								if( choice == 0 ){
									add_card_to_hand(player, graveyard[count] );
									remove_card_from_grave(player, count);
								}
							}
							if( (mode & GT_DT_PYREWILD_SHAMAN) && cards_data[graveyard[count]].id == CARD_ID_PYREWILD_SHAMAN &&
								has_mana(player, COLOR_COLORLESS, 3)
							  ){
								int choice = 0;
								if( ! duh_mode(player) ){
									choice = do_dialog(player, player, card, -1, -1, " Return Pyrewild Shaman\n Pass", 0);
								}
								if( choice == 0 && charge_mana_while_resolving_csvid(CARD_ID_PYREWILD_SHAMAN, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 3) ){
									add_card_to_hand(player, graveyard[count]);
									remove_card_from_grave(player, count);
								}
							}
							count--;
					}
					set_trap_condition(player, TRAP_CHANDRAS_PHOENIX, 0);
			}
		}
	}

	// Nether Traitor + Recover
	if( event == EVENT_GRAVEYARD_FROM_PLAY){
		if( affected_card_controller == player ){
			if( in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
		}
	}
	if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) &&
		player == reason_for_trigger_controller
	  ){
		if(event == EVENT_TRIGGER){
			if( counts[GC_NETHER_TRAITOR] || counts[GC_RECOVER] ){
				//Make all trigges mandatoy for now
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				const int *grave = get_grave(player);
				int count = count_graveyard(player)-1;
				while( count > -1 ){
						if( has_mana(player, COLOR_COLORLESS, 1) && cards_data[grave[count]].id == CARD_ID_NETHER_TRAITOR  ){
							int choice = do_dialog(player, player, card, -1, -1, " Return Nether Traitor to play\n Do not return", 0);
							if( choice == 0 ){
								charge_mana(player, COLOR_COLORLESS, 1);
								if( spell_fizzled != 1 ){
									reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
								}
							}
						}
						if( has_mana_multi(player, MANACOST_XU(2, 2)) && cards_data[grave[count]].id == CARD_ID_CONTROVERT  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Controvert to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_XU(2, 2));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( can_pay_life(player, ((life[player]+1)/2)) && cards_data[grave[count]].id == CARD_ID_GARZAS_ASSASSIN  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Garza's Assassin to your hand\n Do not return", 0);
							if( choice == 0 ){
								lose_life(player, ((life[player]+1)/2));
								add_card_to_hand(player, grave[count]);
								remove_card_from_grave(player, count);
								kill = 0;
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( has_mana_multi(player, MANACOST_XB(2, 1)) && cards_data[grave[count]].id == CARD_ID_GRIM_HARVEST  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Grim Harvest to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_XB(2, 1));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( has_mana_multi(player, MANACOST_R(2)) && cards_data[grave[count]].id == CARD_ID_ICEFALL  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Icefall to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_R(2));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( has_mana_multi(player, MANACOST_XB(1, 2)) && cards_data[grave[count]].id == CARD_ID_KROVIKAN_ROT ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Krovikan Rot to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_XB(1, 2));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( has_mana_multi(player, MANACOST_XG(1, 1)) && cards_data[grave[count]].id == CARD_ID_RESIZE  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Resize to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_XG(1, 1));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						if( has_mana_multi(player, MANACOST_XW(1, 1)) && cards_data[grave[count]].id == CARD_ID_SUNS_BOUNTY  ){
							int kill = 1;
							int choice = do_dialog(player, player, card, -1, -1, " Return Sun's Bounty to your hand\n Do not return", 0);
							if( choice == 0 ){
								charge_mana_multi(player, MANACOST_XW(1, 1));
								if( spell_fizzled != 1 ){
									add_card_to_hand(player, grave[count]);
									remove_card_from_grave(player, count);
									kill = 0;
								}
							}
							if( kill == 1){
								rfg_card_from_grave(player, count);
							}
						}
						count--;
				}
				instance->targets[1].player = 0;
		}
	}
}

static void keep_turn_count(int player, int card, event_t event)
{
  if (event == EVENT_BEGIN_TURN && current_turn == player)
	increase_trap_condition(player, TRAP_TURN_COUNT, 1);
}

int card_coa_special_effect(int player, int card, event_t event);

static int initial_upkeep_triggers(int player, int card, event_t event ){

	if( player == HUMAN && ! get_trap_condition(HUMAN, TRAP_FIRST_TURN_UPKEEP) && upkeep_trigger(player, card, event) ){
		int i;

		// deal with serum powder
		if( player == HUMAN ){

			int p = HUMAN;
			int choice = 1;
			int serum_found = 1;
			if( ! is_unlocked(player, card, event, 7) ){ serum_found = 0; }
			while( serum_found == 1 ){
				serum_found = 0;
				for(i=0;i<active_cards_count[p];i++){
					if( in_hand(p, i) && get_id(p, i ) == CARD_ID_SERUM_POWDER ){
						choice = do_dialog(p, p, i, -1, -1, " Take Serum Powder Mulligan\n Keep Hand", 0);
						serum_found = 1;
						break;
					}
				}
				if( serum_found == 0 || choice == 1 ){
					break;
				}

				int hand_size = 0;
				for(i=0;i<active_cards_count[player];i++){
					if( in_hand(player, i) ){
						hand_size++;
						kill_card(player, i, KILL_REMOVE);
					}
				}

				// proceed with normal mulliganning
				int cards = hand_size;
				while(cards > 0 ){
					// draw the appropriate number of cards
					shuffle_vanguard(player, 1);
					draw_cards(player, cards);

					// allow the choice to mulligan
					choice = do_dialog(player, player, card, -1, -1, " Keep\n Mulligan", 0);
					if( choice == 1 ){
						cards--;
					}
					else{
						break;
					}

					// put your hand back into the deck
					for (i = 0; i < active_cards_count[p]; ++i){
						if (in_hand(player, i)){
							put_on_top_of_deck(player, i);
						}
					}
				}
			}


		}
		for(i=0; i<2; i++){
			// leylines and chancellors
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_hand(i, count) ){
						int id = get_id(i, count );
						if( id == CARD_ID_LEYLINE_OF_THE_VOID ||
						   id == CARD_ID_LEYLINE_OF_SANCTITY ||
						   id == CARD_ID_LEYLINE_OF_PUNISHMENT ||
						   id == CARD_ID_LEYLINE_OF_VITALITY ||
						   id == CARD_ID_LEYLINE_OF_SINGULARITY ||
						   id == CARD_ID_LEYLINE_OF_LIGHTNING ||
						   id == CARD_ID_LEYLINE_OF_THE_MEEK ||
						   id == CARD_ID_LEYLINE_OF_LIFEFORCE
						 ){
							card_ptr_t* c_me = cards_ptr[ id ];
							char buffer[100];
							snprintf(buffer, 100, " Put %s into play\n Leave it in hand", c_me->name);
							int choice2 = do_dialog(i, i, count, -1, -1, buffer, 0);
							if( choice2 == 0 ){
							   put_into_play(i, count);
							}
						}
						if( id == CARD_ID_CHANCELLOR_OF_THE_ANNEX ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Chancellor of the Annex\n Pass", 0);
							if( choice2 == 0 ){
								int leg = create_legacy_effect(i, count, card_coa_special_effect);
								set_special_infos(i, leg, 66);
							}
						}
						if( id == CARD_ID_CHANCELLOR_OF_THE_DROSS ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Chancellor of the Dross\n Pass", 0);
							if( choice2 == 0 ){
								lose_life(1-i, 3);
								gain_life(i, 3);
							}
						}
						if( id == CARD_ID_CHANCELLOR_OF_THE_FORGE ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Chancellor of the Forge\n Pass", 0);
							if( choice2 == 0 ){
								token_generation_t token;
								default_token_definition(player, card, CARD_ID_GOBLIN, &token);
								token.s_key_plus = SP_KEYWORD_HASTE;
								generate_token(&token);
							}
						}
						if( id == CARD_ID_CHANCELLOR_OF_THE_SPIRES ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Chancellor of the Spires\n Pass", 0);
							if( choice2 == 0 ){
								mill(1-i, 7);
							}
						}
						if( id == CARD_ID_CHANCELLOR_OF_THE_TANGLE ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Chancellor of the Tangle\n Pass", 0);
							if( choice2 == 0 ){
								increase_trap_condition(i, TRAP_COT_FLAG, 1);
								/*
								int legacy_card = create_legacy_effect(i, count, &effect_mana_drain );
								card_instance_t *legacy = get_card_instance(i, legacy_card);
								legacy->targets[0].player = PHASE_MAIN1;
								legacy->targets[1].card = CARD_ID_CHANCELLOR_OF_THE_TANGLE;
								*/
							}
						}
						if( id == CARD_ID_GEMSTONE_CAVERNS && hand_count[player] > 1 ){
							int choice2 = do_dialog(i, i, count, -1, -1, " Reveal Gemstone Caverns\n Pass", 0);
							if( choice2 == 0 ){
								target_definition_t td;
								default_target_definition(i, count, &td, TYPE_ANY);
								td.zone = TARGET_ZONE_HAND;
								td.allowed_controller = i;
								td.preferred_controller = i;
								td.who_chooses = i;
								td.illegal_abilities = 0;

								card_instance_t *instance2 = get_card_instance(i, count);

								state_untargettable(i, count, 1);

								if( pick_target(&td, "TARGET_CARD") ){
									instance2->number_of_targets = 1;
									rfg_card_in_hand(instance2->targets[0].player, instance2->targets[0].card);
									put_into_play(i, count);
									add_counter(i, count, COUNTER_LUCK);
								}

								state_untargettable(i, count, 0);
							}
						}
					}
					count--;
			}
		}
		increase_trap_condition(HUMAN, TRAP_FIRST_TURN_UPKEEP, 1);
	}

	// deal with challenges
	if( get_setting(SETTING_CHALLENGE_MODE) ){
		if( (get_challenge3() == 5 || get_challenge4() == 0 || get_challenge4() == 1 ||(gauntlet_round >= 29 && gauntlet_round < 40) || (gauntlet_round >= 59)) && current_turn == HUMAN ){
			if( player == HUMAN && upkeep_trigger(player, card, event) ){
				if( get_challenge4() == 0 || get_challenge4() == 1 || (gauntlet_round >= 29 && gauntlet_round < 40) ||
					(gauntlet_round >= 59 && gauntlet_round < 70) || get_challenge3() == 5
				  ){
					apply_challenge_onslaught();
				}
				else if(gauntlet_round >= 70){
						apply_challenge_puzzle_upkeep();
				}
			}
		}
	}


	if( get_setting(SETTING_GAMESTART_SETTINGS_APPLIED) == 0 && ( get_setting(SETTING_AUTO_MILL) || get_setting(SETTING_CRIPPLE_AI) || get_setting(SETTING_CHECK_DECK_LEGALITY) || get_setting(SETTING_CHALLENGE_MODE)) ){
		if( player == HUMAN && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				if (ai_is_speculating != 1){
					settings[SETTING_GAMESTART_SETTINGS_APPLIED] = 1;
					force_init_upkeep = 0;
				}
				if(get_setting(SETTING_AUTO_MILL) ){
					if( count_deck(HUMAN) > 30 ){
						mill(HUMAN, 30);
						mill(AI, 30);
					}
				}
				if( get_setting(SETTING_CRIPPLE_AI) ){
					// put every card back into the deck
					int count = 0;
					while( count < active_cards_count[AI]){
						if(! in_play(AI, count) ){
							put_on_top_of_deck(AI, count);
						}
						count++;
					}
				}
				if(( get_setting(SETTING_CHECK_DECK_LEGALITY) || get_setting(SETTING_CHALLENGE_MODE) ) && get_challenge_round() != 19){
					int illegal_card = check_deck_legality();
					if ( illegal_card > -1 && poison_counters[HUMAN]== 0){
						//gain_life(HUMAN, get_challenge_round());
						if( get_setting(SETTING_CHALLENGE_MODE) ){
							lose_the_game(HUMAN);
						}
						else{
							do_dialog(HUMAN, HUMAN, illegal_card, -1, -1, "Your deck is not legal.", 0);
							lose_the_game(HUMAN);
						}

					}
				}
				if( get_setting(SETTING_CHALLENGE_MODE) ){
					debug_mode = 0;
					challenge_mode_upkeep(0);
				}
			}
		}
	}

	if( event == EVENT_PHASE_CHANGED && current_turn == player && current_phase == PHASE_MAIN1 && get_trap_condition(player, TRAP_COT_FLAG) == 1 ){
		produce_mana(player, COLOR_GREEN, 1);
		set_trap_condition(player, TRAP_COT_FLAG, 0);
	}

	return 0;
}

static int count_cards_actually_in_hand(int player){
	int c, total = 0;
	for (c = 0; c < active_cards_count[player]; ++c){
		if (in_hand(player, c)
			&& !(cards_data[get_card_instance(player, c)->internal_card_id].type & TYPE_EFFECT)){	// e.g. "draw a card"
			++total;
		}
	}
	return total;
}

static void debug_settings(int player, int card, event_t event){
	// if we are in challenge mode, make sure all the debug settings are off
	if( get_setting(SETTING_CHALLENGE_MODE) ){
		if( debug_mode == 1 || get_setting(SETTING_AUTO_MILL) || get_setting(SETTING_UNLIMITED_MANA) || get_setting(SETTING_AUTO_HASTE) || get_setting(SETTING_CRIPPLE_AI) || game_number > 0 ){
			do_dialog(HUMAN, player, card, -1, -1, "Invalid Settings", 0);
			lose_the_game(HUMAN);
		}

		if( gauntlet_round < 30 && (( gauntlet_round - get_challenge_round() > 1)  || ( get_challenge_round() - gauntlet_round > 1))  ){
			char buffer[100];
			snprintf(buffer, 100, " gauntlet: %d, challenge: %d", gauntlet_round , get_challenge_round());
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
			lose_the_game(HUMAN);
		}

		/*
		if(  count_cards_by_id(HUMAN, CARD_ID_RULES_ENGINE) == 0 && current_phase > PHASE_UPKEEP){
			do_dialog(HUMAN, player, card, -1, -1, "No rules engine", 0);
			lose_the_game(HUMAN);
		}
		*/
	}

	// give each play unlimited mana
	if( get_setting(SETTING_UNLIMITED_MANA) == 1 ){
		int p, c;
		for(p=0;p<2;p++){
			for(c=0;c<6;c++){
				if( ! has_mana(p, c, 10) ) {
					produce_mana(p, c, 10);
				}
			}
		}
	}

	// give all creatures haste
	if( get_setting(SETTING_AUTO_HASTE) ){
		haste(affected_card_controller, affected_card, event);
	}

	if( get_setting(SETTING_VERIFY_HAND_COUNT)
		&& event == EVENT_STATIC_EFFECTS
		&& current_phase != PHASE_DRAW){	// the draw-a-card legacy is sometimes considered to be in hand by the exe, sometimes not - just avoid the issue
		get_card_instance(player, card)->info_slot |= REF_LEYLINES;	// so TRAP_FIRST_TURN_UPKEEP is updated
		if (get_trap_condition(HUMAN, TRAP_FIRST_TURN_UPKEEP)){	// to avoid the false positive at start of game
			int actually_in_hand = count_cards_actually_in_hand(player);
			if (actually_in_hand != hand_count[player]){
				if (ai_is_speculating != 1){
					popup("Rules Engine", "Rules Engine detected hand count inconsistency for %s\nhand_count[%d]=%d\ncount_cards_actually_in_hand(%d)=%d",
						  player == 0 ? "human" : "ai", player, hand_count[player], player, actually_in_hand);
				}
				hand_count[player] = actually_in_hand;
			}
		}
	}
}

static int effect_challenge3(int player, int card, event_t event ){
	if( eot_trigger(player, card, event) ){
		life[HUMAN]=0;
	}
	return 0;
}

static int gauntlet_settings(int player, int card, event_t event ){
	if( ! get_setting(SETTING_CHALLENGE_MODE) ){
		return 0;
	}

	// during the pit challenge, legends have shroud
	if( get_challenge4() == 1 && event == EVENT_ABILITIES ){
		if( has_creature_type(affected_card_controller, affected_card, SUBTYPE_LEGEND ) ){
			event_result |= KEYWORD_SHROUD;
		}
	}

	// during the enchanted challenge, enchantments are indestructible
	if( get_challenge4() == 6 ){
		int c=0;
		for(c=0;c< active_cards_count[AI];c++){
			card_data_t* card_d = get_card_data(AI, c);
			if((card_d->type & TYPE_ENCHANTMENT) && in_play(AI, c)){
				indestructible(AI, c, event);
			}
		}
	}

	// if we won the enchanted challenge, your enchantments are indestructible
	if( event == EVENT_GRAVEYARD_FROM_PLAY && is_unlocked(HUMAN, card, event, 36) ){
		int c=0;
		for(c=0;c< active_cards_count[HUMAN];c++){
			card_data_t* card_d = get_card_data(HUMAN, c);
			if((card_d->type & TYPE_ENCHANTMENT) && in_play(HUMAN, c)){
				indestructible(HUMAN, c, event);
			}
		}
	}

	if( (( gauntlet_round >= 71 && gauntlet_round <= 80 ) || get_challenge2() == 1 ) && player == HUMAN){
		skip_your_draw_step(player, event);
	}

	if( get_challenge2() == 3 ){
		if(event == EVENT_MODIFY_COST_GLOBAL && player == HUMAN && affected_card_controller == player
		   && get_trap_condition(player, TRAP_CHALLENGE_EPIC) && !is_what(affected_card_controller, affected_card, TYPE_LAND)){
			infinite_casting_cost();
		}

		// keep track of spells played
		if(trigger_condition == TRIGGER_SPELL_CAST && event == EVENT_END_TRIGGER && affect_me(player, card)
		   && player == reason_for_trigger_controller && player == HUMAN && player == trigger_cause_controller
		   && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)){
			set_trap_condition(player, TRAP_CHALLENGE_EPIC, 1);
		}
	}

	if( get_challenge3() == 8 ){
		if(trigger_condition == TRIGGER_SPELL_CAST && event == EVENT_END_TRIGGER && affect_me(player, card)
		   && player == reason_for_trigger_controller && trigger_cause_controller == HUMAN && player == HUMAN)
		{
			card_instance_t *played= get_card_instance(trigger_cause_controller, trigger_cause);
			if( ! ( cards_data[ played->internal_card_id].type & TYPE_LAND ) ){
				create_legacy_effect(player, card, &effect_challenge3 );
			}
		}
	}
	else if( get_challenge4() == 3 ){
		if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card)
		   && player == reason_for_trigger_controller && trigger_cause_controller == AI && player == AI)
		{
			card_instance_t *played= get_card_instance(trigger_cause_controller, trigger_cause);
			if( ! ( cards_data[ played->internal_card_id].type & TYPE_LAND ) ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
					do_cascade(trigger_cause_controller, trigger_cause, -1);
				}
			}
		}
	}

	int round = get_challenge_round() - 1;

	// end the game if the time limit for puzzle challenge is over
	if( ( gauntlet_round == 72 || gauntlet_round == 73 ) && current_turn == HUMAN && current_phase == PHASE_MAIN1 ){
		life[HUMAN]=0;
		poison_counters[HUMAN]=10;
	}
	else if( gauntlet_round > 70 && gauntlet_round < 81 && current_turn == HUMAN && eot_trigger(player, card, event) ){
		life[HUMAN]=0;
		poison_counters[HUMAN]=10;
	}

	if( round < 49 || gauntlet_round > 59 ){ return 0; }
	/*
	if(event == EVENT_MODIFY_COST_GLOBAL ){
		card_instance_t *instance = get_card_instance( affected_card_controller, affected_card);
		int type = cards_data[ instance->internal_card_id ].type;

		if( round == 49 && ( instance->card_color & COLOR_TEST_BLACK ) && ! ( type & TYPE_LAND ) && ! ( type & TYPE_ARTIFACT ) ){
			infinite_casting_cost();
		}
		else if( round == 50 && ! ( type & TYPE_LAND ) && ! ( type & TYPE_ARTIFACT )&&instance->card_color & COLOR_TEST_BLUE ){
			infinite_casting_cost();
		}
		else if( round == 51 && ! ( type & TYPE_LAND ) && ! ( type & TYPE_ARTIFACT )&&instance->card_color & COLOR_TEST_GREEN ){
			infinite_casting_cost();
		}
		else if( round == 52 && ! ( type & TYPE_LAND ) && ! ( type & TYPE_ARTIFACT )&&instance->card_color & COLOR_TEST_RED ){
			infinite_casting_cost();
		}
		else if( round == 53 && ! ( type & TYPE_LAND ) && ! ( type & TYPE_ARTIFACT )&& instance->card_color & COLOR_TEST_WHITE ){
			infinite_casting_cost();
		}
		else if( round == 54 && type & TYPE_ARTIFACT ){
			infinite_casting_cost();
		}
		else if( round == 55 && is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT | TARGET_TYPE_PLANESWALKER)){
			infinite_casting_cost();
		}
		else if( round == 56 && type & TYPE_CREATURE ){
			infinite_casting_cost();
		}
		else if( round == 57 && is_what(affected_card_controller, affected_card, TYPE_SPELL)){
			infinite_casting_cost();
		}
		else if( round == 58 && type & TYPE_LAND ){
			infinite_casting_cost();
		}
	}
	*/
	return 0;
}

/*
int play_spell_for_free(int player, int id){

	int internal_id = get_internal_card_id_from_csv_id( id );
	int card_added = add_card_to_hand( player, internal_id );

	free_spell_player = player;
	free_spell_card = card_added;
	card_data_t* card_d = &cards_data[ internal_id ];

	if( put_card_on_stack(player, card_added, 0) ){
		put_card_on_stack(player, card_added, 1);
		put_card_on_stack3(player, card_added) ;

		if(  card_d->type & TYPE_LAND ){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}
	else if( can_legally_play_iid(player, internal_id)){
			play_spell_for_free_orig(player, card_added);
	}
	else{
		obliterate_card(player, card_added);
	}
	free_spell_card = -1;

	return card_added;
}
*/

static int effect_discard_trigger(int player, int card, event_t event ){
	card_instance_t* instance = get_card_instance(player, card);
	if( instance->targets[1].player == 66 ){
		int found = 0;
		int count = count_graveyard(player)-1;
		while( count > -1 ){
				if( get_grave(player)[count] == instance->targets[0].player ){
					found = 1;
					if( instance->targets[0].card == 6 ){
						remove_card_from_grave(player, count);
					}
					if( instance->targets[0].card == 8 ){
						rfg_card_from_grave(player, count);
					}
					break;
				}
				count--;
		}
		if( found ){
			int iid = instance->targets[0].player;
			int csvid = cards_data[iid].id;
			if( cards_data[iid].cc[2] == 11 || cards_data[iid].id != CARD_ID_ICHOR_SLICK ){
				int token = generate_reserved_token_by_id(player, CARD_ID_MADNESS_EFFECT);
				set_special_infos(player, token, cards_data[iid].id);
				create_card_name_legacy(player, token, cards_data[iid].id);
			}
			else{
				if( csvid == CARD_ID_METROGNOME ){
					generate_tokens_by_id(player, card, CARD_ID_GNOME, 4);
				}
				else if( csvid == CARD_ID_PSYCHIC_PURGE ){
					lose_life(1-player, 5);
				}
				else if( csvid == CARD_ID_GUERRILLA_TACTICS ){
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
					td.illegal_abilities |= KEYWORD_PROT_RED | KEYWORD_PROT_INSTANTS;

					if( can_target(&td) && new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 4, 0) ){
						damage_creature(instance->targets[4].player, instance->targets[4].card, 4, player, card);
					}
				}
				else{
					int card_added = add_card_to_hand(player, iid);
					if( csvid == CARD_ID_DODECAPOD ){
						add_1_1_counters(player, card_added, 2);
					}
					put_into_play(player, card_added);
				}
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
 }

int card_madness_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int id = get_special_infos(player, card);

	if( comes_into_play(player, card, event) ){
		instance->targets[1].card = current_phase;
		instance->targets[2].card = 66;

	}
	if( instance->targets[2].card == 66  ){
		if( current_phase != instance->targets[1].card ){
			if( id > -1 && instance->targets[3].card < 0 ){
				int internal_id = get_internal_card_id_from_csv_id(id);
				int pos = find_iid_in_rfg(player, internal_id);
				if( pos > -1 ){
					from_exile_to_graveyard(player, pos);
				}
			}
			instance->targets[2].card = 67;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		reason_for_trigger_controller == player){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}


		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					if( id > -1 && instance->targets[3].card < 0 ){
						int internal_id = get_internal_card_id_from_csv_id(id);
						int pos = find_iid_in_rfg(player, internal_id);
						if( pos > -1 ){
							from_exile_to_graveyard(player, pos);
						}
					}
					kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && id > -1 && instance->targets[3].card < 0){
		int int_id = get_internal_card_id_from_csv_id(id);
		card_data_t* card_d = &cards_data[ int_id ];
		int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
		if( ptFunction(player, card, EVENT_CAN_PAY_MADNESS_COST ) ){
			return can_legally_play_iid(player, int_id);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int int_id = get_internal_card_id_from_csv_id(id);
		card_data_t* card_d = &cards_data[ int_id ];
		int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
		ptFunction(player, card, EVENT_PAY_MADNESS_COST );
		if( spell_fizzled != 1 ){
			instance->targets[3].card = id;
			cant_be_responded_to = 1;
			play_card_in_exile_for_free(player, player, instance->targets[3].card);
		}
	}

	return 0;
}

static void discard_triggers(int player, int card, event_t event ){
	if( trigger_condition == TRIGGER_DISCARD && affect_me(player, card) && reason_for_trigger_controller == player &&
		player == trigger_cause_controller && event == EVENT_END_TRIGGER && current_phase > PHASE_MAIN2
	  ){
		card_data_t* card_d = get_card_data(player, trigger_cause);
		if( card_d->cc[2] == 11 || card_d->id == CARD_ID_ICHOR_SLICK ){
			int effect_card = create_legacy_effect(player, card, &effect_discard_trigger );
			card_instance_t *effect = get_card_instance(player, effect_card);
			effect->targets[0].card = get_card_instance(player, trigger_cause)->internal_card_id;
			effect->targets[0].player = 8;
			effect->targets[1].player = 66;

			int token = generate_reserved_token_by_id(player, CARD_ID_MADNESS_EFFECT);
			set_special_infos(player, token, card_d->id);
			create_card_name_legacy(player, token, card_d->id);
		}
	}
}

static void vengeful_pharaoh_effect(int player, int card, event_t event ){

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && current_turn == 1-player
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_ATTACKING;
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
		td.illegal_abilities |= KEYWORD_PROT_BLACK | KEYWORD_PROT_CREATURES;
		td.allow_cancel = 0;

		if( can_target(&td) && (current_phase == PHASE_FIRST_STRIKE_DAMAGE || current_phase == PHASE_NORMAL_COMBAT_DAMAGE)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
				const int *grave = get_grave(player);
				int count = count_graveyard(player)-1;
				while( count > -1 ){
					if( cards_data[grave[count]].id == CARD_ID_VENGEFUL_PHARAOH && can_target(&td) ){
						pick_target(&td, "TARGET_CREATURE");
						card_instance_t *instance = get_card_instance(player, card);
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
						int card_added = add_card_to_hand(player, grave[count]);
						remove_card_from_grave(player, count);
						put_on_top_of_deck(player, card_added);
					}
					count--;
				}
			}
		}
	}

	return;
}

int check_card_for_rules_engine(int card){
	// card --> internal card id of the card to examine
	if( card < 0 ){
		return 0;
	}
	int result = 0;
	// Leylines & 'get_turn_count'
		if( cards_data[card].id == CARD_ID_SERUM_POWDER ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_THE_VOID ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_SANCTITY ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_PUNISHMENT ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_VITALITY ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_SINGULARITY ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_LIGHTNING ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_THE_MEEK ||
			cards_data[card].id == CARD_ID_LEYLINE_OF_LIFEFORCE ||
			cards_data[card].id == CARD_ID_CHANCELLOR_OF_THE_ANNEX ||
			cards_data[card].id == CARD_ID_CHANCELLOR_OF_THE_DROSS ||
			cards_data[card].id == CARD_ID_CHANCELLOR_OF_THE_FORGE ||
			cards_data[card].id == CARD_ID_CHANCELLOR_OF_THE_SPIRES ||
			cards_data[card].id == CARD_ID_CHANCELLOR_OF_THE_TANGLE ||
			cards_data[card].id == CARD_ID_GEMSTONE_CAVERNS ||
			cards_data[card].id == CARD_ID_SERRA_AVENGER
		  ){
			result |= REF_LEYLINES;
		}
	// Traps
		if( cards_data[card].cc[2] == 13 || cards_data[card].id == CARD_ID_MAELSTROM_NEXUS ||
			(cards_data[card].id != CARD_ID_MINDBREAK_TRAP && cards_data[card].id != CARD_ID_RUNEFLARE_TRAP && has_subtype_by_id(cards_data[card].id, SUBTYPE_TRAP)) ||
			cards_data[card].id == CARD_ID_SEARING_BLAZE ||
			cards_data[card].id == CARD_ID_BLOODCHIEF_ASCENSION ||
			cards_data[card].id == CARD_ID_PYREWILD_SHAMAN ||
			cards_data[card].id == CARD_ID_CHANDRAS_PHOENIX ||
			cards_data[card].id == CARD_ID_ARCHENEMY ||
			cards_data[card].id == CARD_ID_BLOODSOAKED_CHAMPION ||
			cards_data[card].id == CARD_ID_HOWL_OF_THE_HORDE
		  ){
			result |= REF_TRAP_CONDITIONS;
		}
	// Storm count
		if( cards_data[card].cc[2] == 12 ||
			cards_data[card].id == CARD_ID_LURE_OF_PREY ||
			cards_data[card].id == CARD_ID_MOGG_CONSCRIPTS ||
			cards_data[card].id == CARD_ID_MINDS_DESIRE ||
			cards_data[card].id == CARD_ID_WING_SHARDS ||
			cards_data[card].id == CARD_ID_MINDBREAK_TRAP ||
			cards_data[card].id == CARD_ID_SIFT_THROUGH_SANDS
		  ){
			result |= REF_STORM_COUNT;
		}
	// Dredge
		if( cards_data[card].cc[2] == 6 ){
			result |= REF_DREDGE;
		}
	// Graveyard upkeep trigger
		if( cards_data[card].cc[2] == 5 ||
			cards_data[card].id == CARD_ID_DEATH_SPARK ||
			cards_data[card].id == CARD_ID_ETERNAL_DRAGON ||
			cards_data[card].id == CARD_ID_NIM_DEVOURER ||
			cards_data[card].id == CARD_ID_NECROSAVANT ||
			cards_data[card].id == CARD_ID_UNDEAD_GLADIATOR
		  ){
			result |= REF_UPKEEP_ABILITY_IN_GRAVE;
		}
	// Static / Triggered abilities in graveyard
		if( cards_data[card].cc[2] == 2 || cards_data[card].cc[2] == 20 || cards_data[card].cc[2] == 18 ||
			cards_data[card].id == CARD_ID_VENGEVINE ||
			cards_data[card].id == CARD_ID_ULAMOG_THE_INFINITE_GYRE ||
			cards_data[card].id == CARD_ID_KOZILEK_BUTCHER_OF_TRUTH ||
			cards_data[card].id == CARD_ID_EMRAKUL_THE_AEONS_TORN ||
			cards_data[card].id == CARD_ID_DARKSTEEL_COLOSSUS ||
			cards_data[card].id == CARD_ID_BLIGHTSTEEL_COLOSSUS ||
			cards_data[card].id == CARD_ID_PROGENITUS ||
			cards_data[card].id == CARD_ID_AUNTIES_SNITCH ||
			cards_data[card].id == CARD_ID_PYREWILD_SHAMAN
		  ){
			result |= REF_CONT_AND_TRIG_ABILITY_IN_GRAVE;
		}

		if( cards_data[card].id == CARD_ID_BRIDGE_FROM_BELOW ){
			result |= REF_BRIDGE_FROM_BELOW;
		}
	/*
	if( !(original_result & (1<<8)) ){
		if( cards_data[card].id == CARD_ID_ELSPETH_KNIGHT_ERRANT ){
			result+=256;
		}
	}
	*/
		if( cards_data[card].id == CARD_ID_KROVIKAN_HORROR ){
			result |= REF_KROVIKAN_HORROR;
		}
	// Discard trigger / Madness
		if( cards_data[card].cc[2] == 11 ||
			cards_data[card].id == CARD_ID_OBSTINATE_BALOTH ||
			cards_data[card].id == CARD_ID_WILT_LEAF_LIEGE ||
			cards_data[card].id == CARD_ID_METROGNOME ||
			cards_data[card].id == CARD_ID_LOXODON_SMITER ||
			cards_data[card].id == CARD_ID_QUAGNOTH ||
			cards_data[card].id == CARD_ID_DODECAPOD ||
			cards_data[card].id == CARD_ID_ICHOR_SLICK
		  ){
			result |= REF_DISCARD_TRIGGER_MADNESS;
		}
	// Cards with activated abilities in graveyard / hand (Cycling, Flashback, Retrace, Unearth and so on)
		if( (cards_data[card].cc[2] == 9 && !(cards_data[card].type & TYPE_ENCHANTMENT)) || //Direct comparison with TYPE_ENCHANTMENT ok here, as part of an inline call to is_planeswalker().
			cards_data[card].cc[2] == 10 || // Retrace
			cards_data[card].cc[2] == 4 || // Cycling
			cards_data[card].cc[2] == 17 || // Scavenge
			cards_data[card].id == CARD_ID_VISCERA_DRAGGER ||
			cards_data[card].id == CARD_ID_HAAKON_STROMGALD_SCOURGE ||
			cards_data[card].id == CARD_ID_SILENT_BLADE_ONI ||
			cards_data[card].id == CARD_ID_MARSHALING_CRY ||
			cards_data[card].id == CARD_ID_UNDEAD_GLADIATOR ||
			cards_data[card].id == CARD_ID_DRAGON_WINGS ||
			cards_data[card].id == CARD_ID_VINEWEFT
		  ){
			result |= REF_ACTIVATED_ABILITIES_HAND_GRAVE;
		}
	// Deadbox
		if( cards_data[card].cc[2] == 14 ){
			result |= REF_DEADBOX;
		}

		if( cards_data[card].id == CARD_ID_VENGEFUL_PHARAOH ){
			result |= REF_VENGEFUL_PHARAOH;
		}
	// Miracle
		if( cards_data[card].cc[2] == 15 ){
			result |= REF_MIRACLES;
		}
	// Forecast
		if( cards_data[card].cc[2] == 16 ){
			result |= REF_FORECAST;
			result |= REF_ACTIVATED_ABILITIES_HAND_GRAVE;
		}
	// Conspire
		if( cards_data[card].cc[2] == 19 ){
			result |= REF_CONSPIRE;
		}
	// Beginning of combat effects
		if( cards_data[card].cc[2] == 22 ){
			result |= REF_BEGINNING_OF_COMBAT_EFFECTS;
		}
	return result;
}

static void shape_deadbox(void){
	deadbox_card[AI] = generate_reserved_token_by_id(AI, CARD_ID_DEADBOX);
	deadbox_card[HUMAN] = generate_reserved_token_by_id(HUMAN, CARD_ID_DEADBOX);
}

static void shape_rules_engine(int infos){
		int z;
		for (z = 0; z < 2; z++){
			rules_engine_card[z] = generate_reserved_token_by_id(z, CARD_ID_RULES_ENGINE); // rules engine
			card_instance_t *instance = get_card_instance(z, rules_engine_card[z]);
			instance->info_slot = infos;
			// Initialize everything from targets[5] up to end of targets array to 0
			memset(&instance->targets[5], 0, sizeof(target_t) * 14);
		}
}

static int check_decks_for_rules_engine(void){

	int i;
	int z;
	int result = 0;
	for(z=0; z < 2; z++){
		int *deck = deck_ptr[z];
		for(i=0; i < count_deck(z); i++ ){
			result |= check_card_for_rules_engine(deck[i]);
		}
		for (i = 0; i < active_cards_count[z]; ++i){
			result |= check_card_for_rules_engine(get_card_instance(z, i)->internal_card_id);
		}
	}
	return result;
}

void recalculate_rules_engine_and_deadbox(void)
{
  int flags = check_decks_for_rules_engine();

  if (flags & REF_DEADBOX)
	{
	  if (get_deadbox_card(HUMAN) == -1)
		shape_deadbox();
	}
  else
	{
	  int db = get_deadbox_card(HUMAN);
	  if (db != -1)
		kill_card(HUMAN, db, KILL_SACRIFICE);
	  db = get_deadbox_card(AI);
	  if (db != -1)
		kill_card(AI, db, KILL_SACRIFICE);
	  deadbox_card[HUMAN] = deadbox_card[AI] = -1;
	}

  if (flags & ~REF_DEADBOX)
	{
	  int re = get_rules_engine_card(HUMAN);
	  if (re == -1)
		shape_rules_engine(flags);
	  else
		{
		  get_card_instance(HUMAN, re)->info_slot = flags;
		  re = get_rules_engine_card(AI);
		  if (re != -1)
			get_card_instance(AI, re)->info_slot = flags;
		}
	}
  else
	{
	  int re = get_rules_engine_card(HUMAN);
	  if (re != -1)
		kill_card(HUMAN, re, KILL_SACRIFICE);
	  re = get_rules_engine_card(AI);
	  if (re != -1)
		kill_card(AI, re, KILL_SACRIFICE);
	  rules_engine_card[HUMAN] = rules_engine_card[AI] = -1;
	}
}

void update_rules_engine(int infos){
	if( infos < 1 ){
		return;
	}
	int re = get_rules_engine_card(HUMAN);
	if( re < 0 ){
		if( infos != 8192 ){
			shape_rules_engine(infos);
		}
	}
	else{
		int i;
		for(i=0; i<2; i++){
			int re_card = get_rules_engine_card(i);
			card_instance_t *instance = get_card_instance( i, re_card );
			instance->info_slot |= infos;
		}
	}
	if( infos & 8192 ){
		if( get_deadbox_card(HUMAN) == -1 ){
			shape_deadbox();
		}
	}
}

static void clear_temporary_traps(card_instance_t* instance);
static void check_trap_conditions(int player, int card, event_t event);

int get_rules_engine_infos(int player){
	int re = get_rules_engine_card(player);
	if( re > -1 ){
		card_instance_t *instance = get_card_instance(player,re);
		return instance->info_slot;
	}
	return 0;
}

int get_deadbox_card(int player){
	return deadbox_card[player];
}

void game_startup(void){

	memset(graveyard_source, -1, sizeof(graveyard_source));
	next_graveyard_source_id = 1;

	int re_infos = 0;

	re_infos = check_decks_for_rules_engine();

	hack_allow_sorcery_if_rules_engine_is_only_effect = 0;

	if( get_setting(SETTING_CHALLENGE_MODE) ){
		re_infos |= REF_LEYLINES; // upkeep triggers
		re_infos |= REF_GAUNTLET; // gauntlet settings
		set_challenge_round();
	}
	if( get_setting(SETTING_ARCHENEMY) ){	// Also set by the Archenemy card, if it's in a deck instead of playing via setting.
		re_infos |= REF_TRAP_CONDITIONS;
	}
	vanguard_check();

	if (get_setting(SETTING_RULES_ENGINE) == 0){
		return;
	}

	if( re_infos & REF_DEADBOX ){
		shape_deadbox();
	}

	if( re_infos > 0 && re_infos != REF_DEADBOX ){
		shape_rules_engine(re_infos);
	}

	return;
}

void set_starting_life_total(int player, int amount ){
	if( amount > 20 ){
		gain_life(player, amount - 20);
	}
	else if( amount < 20 ){
			life[player]-=(20-amount);
	}
	starting_life_total[player] = amount;
}

int get_starting_life_total(int player){
	return starting_life_total[player];
}

static void increase_trap_condition_impl(int player, int trap, int val, int add /*otherwise set*/)
{
  if (add && val == 0)
	return;

  int rec = get_rules_engine_card(player);
  if (rec < 0)
	return;

  card_instance_t* instance = get_card_instance(player, rec);

  // Strictly speaking, this is undefined behavior, but should work on everything Manalink can.
  rules_engine_globals_t* globals = (rules_engine_globals_t*)(&instance->targets[0].player);

  int bit, idx = trap & 0xFF;

  // Uses gcc case ranges extension.  Could be done (more verbosely) with a series of if/elses, of course.
  switch (trap)
	{
	  case TRAP_MIN_SHORTS_PER_TURN ... TRAP_MAX_SHORTS_PER_TURN:
		if (add)
		  val += globals->shorts_per_turn[idx];

		if (val > USHRT_MAX)
		  val = USHRT_MAX;
		else if (val < 0)
		  val = 0;

		globals->shorts_per_turn[idx] = val;
		break;

	  case TRAP_MIN_SHORTS_PERSISTENT ... TRAP_MAX_SHORTS_PERSISTENT:
		if (add)
		  val += globals->shorts_persistent[idx];

		if (val > USHRT_MAX)
		  val = USHRT_MAX;
		else if (val < 0)
		  val = 0;

		globals->shorts_persistent[idx] = val;
		break;

	  case TRAP_MIN_BYTES_PER_TURN ... TRAP_MAX_BYTES_PER_TURN:
		if (add)
		  val += globals->bytes_per_turn[idx];

		if (val > UCHAR_MAX)
		  val = UCHAR_MAX;
		else if (val < 0)
		  val = 0;

		globals->bytes_per_turn[idx] = val;
		break;

	  case TRAP_MIN_BYTES_PERSISTENT ... TRAP_MAX_BYTES_PERSISTENT:
		if (add)
		  val += globals->bytes_persistent[idx];

		if (val > UCHAR_MAX)
		  val = UCHAR_MAX;
		else if (val < 0)
		  val = 0;

		globals->bytes_persistent[idx] = val;
		break;

	  case TRAP_MIN_NYBBLES_PER_TURN ... TRAP_MAX_NYBBLES_PER_TURN:
		if (idx % 2 == 0)
		  {
			// low nybble
			idx /= 2;

			if (add)
			  val += globals->nybbles_per_turn[idx] & 0x0F;

			if (val > 0x0F)
			  val = 0x0F;
			else if (val < 0)
			  val = 0;

			globals->nybbles_per_turn[idx] = (globals->nybbles_per_turn[idx] & ~0x0F) | val;
		  }
		else
		  {
			// high nybble
			idx /= 2;

			if (add)
			  val += (globals->nybbles_per_turn[idx] & 0xF0) >> 4;

			if (val > 0x0F)
			  val = 0x0F;
			else if (val < 0)
			  val = 0;

			globals->nybbles_per_turn[idx] = (globals->nybbles_per_turn[idx] & ~0xF0) | (val << 4);
		  }
		break;

	  case TRAP_MIN_BITS_PER_TURN ... TRAP_MAX_BITS_PER_TURN:
		bit = idx % 8;
		idx /= 8;

		if (val > 0)
		  globals->bits_per_turn[idx] |= 1 << bit;
		else
		  globals->bits_per_turn[idx] &= ~(1 << bit);
		break;

	  case TRAP_MIN_BITS_PERSISTENT ... TRAP_MAX_BITS_PERSISTENT:
		bit = idx % 8;
		idx /= 8;

		if (val > 0)
		  globals->bits_persistent[idx] |= 1 << bit;
		else
		  globals->bits_persistent[idx] &= ~(1 << bit);
		break;
	}
}

int get_trap_condition(int player, int trap)
{
  int rec = get_rules_engine_card(player);
  if (rec < 0)
	return 0;

  card_instance_t* instance = get_card_instance(player, rec);

  // Strictly speaking, this is undefined behavior, but should work on everything Manalink can.
  rules_engine_globals_t* globals = (rules_engine_globals_t*)(&instance->targets[0].player);

  int bit, idx = trap & 0xFF;

  // Uses gcc case ranges extension.  Could be done (more verbosely) with a series of if/elses, of course.
  switch (trap)
	{
	  case TRAP_MIN_SHORTS_PER_TURN ... TRAP_MAX_SHORTS_PER_TURN:
		return globals->shorts_per_turn[idx];

	  case TRAP_MIN_SHORTS_PERSISTENT ... TRAP_MAX_SHORTS_PERSISTENT:
		return globals->shorts_persistent[idx];

	  case TRAP_MIN_BYTES_PER_TURN ... TRAP_MAX_BYTES_PER_TURN:
		return globals->bytes_per_turn[idx];

	  case TRAP_MIN_BYTES_PERSISTENT ... TRAP_MAX_BYTES_PERSISTENT:
		return globals->bytes_persistent[idx];

	  case TRAP_MIN_NYBBLES_PER_TURN ... TRAP_MAX_NYBBLES_PER_TURN:
		if (idx % 2 == 0)
		  {
			// low nybble
			idx /= 2;

			return globals->nybbles_per_turn[idx] & 0x0F;
		  }
		else
		  {
			// high nybble
			idx /= 2;

			return (globals->nybbles_per_turn[idx] & 0xF0) >> 4;
		  }

	  case TRAP_MIN_BITS_PER_TURN ... TRAP_MAX_BITS_PER_TURN:
		bit = idx % 8;
		idx /= 8;

		return (globals->bits_per_turn[idx] & (1 << bit)) ? 1 : 0;

	  case TRAP_MIN_BITS_PERSISTENT ... TRAP_MAX_BITS_PERSISTENT:
		bit = idx % 8;
		idx /= 8;

		return (globals->bits_persistent[idx] & (1 << bit)) ? 1 : 0;
	}

  return 0;
}

// Clear the per-turn trap conditions out at start of turn
static void clear_temporary_traps(card_instance_t* instance)
{
  // Strictly speaking, this is undefined behavior, but should work on everything Manalink can.
  rules_engine_globals_t* globals = (rules_engine_globals_t*)(&instance->targets[0].player);

  memset(globals->shorts_per_turn, 0, sizeof(globals->shorts_per_turn));
  memset(globals->bytes_per_turn, 0, sizeof(globals->bytes_per_turn));
  memset(globals->nybbles_per_turn, 0, sizeof(globals->nybbles_per_turn));
  memset(globals->bits_per_turn, 0, sizeof(globals->bits_per_turn));
}

static void check_trap_conditions(int player, int card, event_t event){

	if( player != HUMAN ){ return; }

	// how many unblocked creatures
	int i, p = current_turn;
	if( event == EVENT_DECLARE_BLOCKERS ){
		for(i=0;i<active_cards_count[p];i++){
			if( is_unblocked(p, i) ){
				increase_trap_condition(current_turn, TRAP_UNBLOCKED_CREATURES, 1);
			}
		}
	}

	// was life gained?
	// handled in "gain_life" in "functions.c"

	// how many Zubera died this turn?
	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return;}
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( affected->kill_code > 0 && affected->kill_code < 4 ){
			if( has_subtype(affected_card_controller, affected_card, SUBTYPE_ZUBERA) ){
				increase_trap_condition(affected_card_controller, TRAP_DEAD_ZUBERA_COUNT, 1);
			}
		}
	}

	if(event == EVENT_DECLARE_BLOCKERS){
		int count = 0;
		while(count < active_cards_count[current_turn]){
			if( is_what(current_turn, count, TYPE_CREATURE) && is_attacking(current_turn, count)  ){
				set_trap_condition(current_turn, TRAP_RAID, 1);
				break;
			}
			count++;
		}
	}

#if 0
	/* Nothing currently checks TRAP_NUMBER_OF_ATTACKING_CREATURES (byte), and TRAP_NEMESIS_TRAP (bit) is inaccurate - the card's mana cost decreases if a white
	 * creature is currently attacking, not if one attacked this turn; and something may well have already destroyed it. */

	// how many creatures are attacking?
	if(event == EVENT_DECLARE_BLOCKERS){
		int count = 0;
		while(count < active_cards_count[current_turn]){
			if( is_what(current_turn, count, TYPE_CREATURE) && is_attacking(current_turn, count)  ){
				increase_trap_condition(current_turn, TRAP_NUMBER_OF_ATTACKING_CREATURES, 1);
				if( get_color(current_turn, count) & COLOR_TEST_WHITE ){
					increase_trap_condition(player, TRAP_NEMESIS_TRAP, 1);
					increase_trap_condition(1-player, TRAP_NEMESIS_TRAP, 1);
				}
			}
			count++;
		}
	}
	else{
		set_trap_condition(current_turn, TRAP_NUMBER_OF_ATTACKING_CREATURES, 0);
		set_trap_condition(1-current_turn, TRAP_NUMBER_OF_ATTACKING_CREATURES, 0);
	}
#endif

	// did an artifact / creature / land come into play?
	if(event == EVENT_END_TRIGGER && trigger_condition == TRIGGER_COMES_INTO_PLAY && reason_for_trigger_controller == affected_card_controller){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT) ){
			increase_trap_condition(trigger_cause_controller, TRAP_ARTIFACTS_PLAYED, 1);
		}
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			increase_trap_condition(trigger_cause_controller, TRAP_LANDS_PLAYED, 1);
		}
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			increase_trap_condition(trigger_cause_controller, TRAP_CREATURES_PLAYED, 1);
			if( get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_GREEN ){
				increase_trap_condition(trigger_cause_controller, TRAP_PERMAFROST_TRAP, 1);
			}
			set_special_flags(trigger_cause_controller, trigger_cause, SF_JUST_CAME_INTO_PLAY);
		}
	}

	if (event == EVENT_END_TRIGGER && trigger_condition == TRIGGER_DISCARD && reason_for_trigger_controller == affected_card_controller){
		increase_trap_condition(trigger_cause_controller, TRAP_DISCARDED_CARDS, 1);
	}
}

void increase_trap_condition(int player, int trap, int amount)
{
  increase_trap_condition_impl(player, trap, amount, 1);
}

void set_trap_condition(int player, int trap, int value)
{
  increase_trap_condition_impl(player, trap, value, 0);
}

static void beginning_of_combat_effects(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_PHASE_CHANGED && current_phase == PHASE_DECLARE_ATTACKERS && instance->targets[1].player != 1 && in_play(player, card)){
		instance->targets[1].player = 1;
		if( ferocious(player, card) ){
			const int *grave = get_grave(player);
			int count = count_graveyard(player)-1;
			while( count > -1 ){
					if( cards_data[grave[count]].id == CARD_ID_FLAMEWAKE_PHOENIX && has_mana(player, COLOR_RED, 1) ){
						if( do_dialog(player, player, card, -1, -1, " Return Flamewake Phoenix to play\n Pass", 0) == 0 ){
							charge_mana(player, COLOR_RED, 1);
							if( spell_fizzled != 1 ){
								reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
							}
						}
					}
					count--;
			}
		}
	}

	if(event == EVENT_PHASE_CHANGED && current_phase != PHASE_DECLARE_ATTACKERS){
		instance->targets[1].player = 0;
	}
}

int card_rules_engine(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player,card);

	if( event == EVENT_CLEANUP ){
		int i;
		for(i=0; i<2; i++){
			remove_special_flags2(i, -1, SF2_HAS_DAMAGED_PLAYER0);
			remove_special_flags2(i, -1, SF2_HAS_DAMAGED_PLAYER1);
		}
	}

	// (I'd think this should be during EVENT_CLEANUP, but it's harmless to preserve the original behavior and delay until EVENT_CAN_SKIP_TURN of next turn)
	if (event == EVENT_CAN_SKIP_TURN){
		clear_temporary_traps(instance);
	}

	debug_settings(player, card, event);
	if ((instance->info_slot & REF_LEYLINES) || force_init_upkeep){
		initial_upkeep_triggers(player, card, event);            // 1
		keep_turn_count(player, card, event);                    // same as above
	}
	if( instance->info_slot & REF_TRAP_CONDITIONS ){
		check_trap_conditions(player, card, event);                   // 2
	}
	if( instance->info_slot & REF_STORM_COUNT ){
		keep_storm_count(player, card, event);                   // 4
	}
	if( instance->info_slot & REF_GAUNTLET ){
		gauntlet_settings(player, card, event);                  // 8
		//apply_challenge_10_to_1(player, card, event);
	}
	if( instance->info_slot & REF_DREDGE ){
		resolve_dredge_triggers(player, card, event);            // 16
	}
	if( instance->info_slot & REF_UPKEEP_ABILITY_IN_GRAVE ){
		resolve_graveyard_upkeep_triggers(player, card, event);  // 32
	}
	if( instance->info_slot & REF_CONT_AND_TRIG_ABILITY_IN_GRAVE ){
		static_graveyard_abilities(player, card, event);         // 64
	}
	if( instance->info_slot & REF_BRIDGE_FROM_BELOW ){
		bridge_from_below(player, card, event);                  // 128
	}
	// 1<<8 unused
	if( instance->info_slot & REF_KROVIKAN_HORROR ){
		krovikan_horror(player, card, event);                    // 512
	}
	if( instance->info_slot & REF_DISCARD_TRIGGER_MADNESS ){
		discard_triggers(player, card, event);                   // 1024
	}
	// (1<<13) --> Deabox
	if( instance->info_slot & REF_VENGEFUL_PHARAOH ){
		vengeful_pharaoh_effect(player, card, event);                 // 16384
	}
	if( instance->info_slot & REF_MIRACLES ){
		miracle(player, card, event);
	}
	// (1<<16) --> Forecast
	// (1<<17) --> Scavenge
	// (1<<18) --> Onslaught's auras auto-returning from grave
	if( instance->info_slot & REF_CONSPIRE ){
		conspire(player, card, event);
	}
	// (1<<20) --> Recover
	if( instance->info_slot & REF_BEGINNING_OF_COMBAT_EFFECTS ){
		beginning_of_combat_effects(player, card, event);
	}
	if( instance->info_slot & REF_ACTIVATED_ABILITIES_HAND_GRAVE ){
		return activate_cards_in_hand(player, card, event);           // 4096
	}

	return 0;
}

