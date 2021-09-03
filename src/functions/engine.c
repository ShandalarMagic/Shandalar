// -*- c-basic-offset:2 -*-
#include "manalink.h"

// These are fairly common here, but shouldn't be used elsewhere
#define push_affected_card_stack	EXE_FN(void, 0x435C80, void)
#define pop_affected_card_stack		EXE_FN(void, 0x435CD0, void)


void count_colors_of_lands_in_play(void)
{
  // 0x4725C0

  /* Essentially rewritten, as the exe version was poorly treated by previous injections.  It included such gems as counting Blinking Spirit like a Bayou, and
   * similarly for about a dozen other random Homelands/Ice Age cards.  It's more reliable to just query the card instead of checking id by id anyway. */

  int i;
  for (i = 0; i < 8; ++i)
	{
	  basiclandtypes_controlled[0][i] = 0;
	  basiclandtypes_controlled[1][i] = 0;
	  landsofcolor_controlled[0][i] = 0;
	  landsofcolor_controlled[1][i] = 0;
	}

  colors_of_lands_in_play[0] = 0;
  colors_of_lands_in_play[1] = 0;

  int player, card;
  for (player = 0; player < 2; ++player)
	for (card = 0; card < active_cards_count[player]; ++card)
	  if (in_play(player, card) && is_what(player, card, TYPE_LAND))
		{
		  card_instance_t* instance = get_card_instance(player, card);

		  int colors = instance->card_color;
		  int basics;

		  ++landsofcolor_controlled[player][COLOR_ANY];
		  ++basiclandtypes_controlled[player][COLOR_ANY];

		  if (check_special_flags2(player, card, SF2_PRISMATIC_OMEN))
			{
			  colors |= COLOR_TEST_ANY_COLORED;
			  basics = COLOR_TEST_ANY_COLORED;
			}
		  else
			{
			  basics = 0;

			  if (has_subtype(player, card, SUBTYPE_SWAMP))
				basics |= COLOR_TEST_BLACK;
			  if (has_subtype(player, card, SUBTYPE_ISLAND))
				basics |= COLOR_TEST_BLUE;
			  if (has_subtype(player, card, SUBTYPE_FOREST))
				basics |= COLOR_TEST_GREEN;
			  if (has_subtype(player, card, SUBTYPE_MOUNTAIN))
				basics |= COLOR_TEST_RED;
			  if (has_subtype(player, card, SUBTYPE_PLAINS))
				basics |= COLOR_TEST_WHITE;

			  colors |= basics;
			}

		  if (check_special_flags2(player, card, SF2_CONTAMINATION))
			colors = COLOR_TEST_BLACK;

		  if (colors & COLOR_TEST_BLACK)
			++landsofcolor_controlled[player][COLOR_BLACK];
		  if (basics & COLOR_TEST_BLACK)
			++basiclandtypes_controlled[player][COLOR_BLACK];

		  if (colors & COLOR_TEST_BLUE)
			++landsofcolor_controlled[player][COLOR_BLUE];
		  if (basics & COLOR_TEST_BLUE)
			++basiclandtypes_controlled[player][COLOR_BLUE];

		  if (colors & COLOR_TEST_GREEN)
			++landsofcolor_controlled[player][COLOR_GREEN];
		  if (basics & COLOR_TEST_GREEN)
			++basiclandtypes_controlled[player][COLOR_GREEN];

		  if (colors & COLOR_TEST_RED)
			++landsofcolor_controlled[player][COLOR_RED];
		  if (basics & COLOR_TEST_RED)
			++basiclandtypes_controlled[player][COLOR_RED];

		  if (colors & COLOR_TEST_WHITE)
			++landsofcolor_controlled[player][COLOR_WHITE];
		  if (basics & COLOR_TEST_WHITE)
			++basiclandtypes_controlled[player][COLOR_WHITE];

		  if (colors & COLOR_TEST_ARTIFACT)
			++landsofcolor_controlled[player][COLOR_ARTIFACT];

		  if (!(colors & (COLOR_TEST_ANY_COLORED | COLOR_TEST_ARTIFACT)))
			++landsofcolor_controlled[player][COLOR_COLORLESS];

		  if (!(basics & COLOR_TEST_ANY_COLORED))
			++basiclandtypes_controlled[player][COLOR_COLORLESS];

		  colors_of_lands_in_play[player] |= colors;
		}

  colors_of_lands_in_play[0] &= COLOR_TEST_ANY_COLORED;
  colors_of_lands_in_play[1] &= COLOR_TEST_ANY_COLORED;

  /* This is called both from recalculate_all_cards_in_play() and from reassess_all_cards(); in the latter, it's before the checks for whether cards are
   * playable.  So this is a handy spot to check this, less wasteful than doing so continuously, and always gets done in time. */
  if ((player_bits[current_turn] & PB_COUNT_TOTAL_PLAYABLE_LANDS)
	  && (land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED)
	  && lands_played < total_playable_lands(current_turn))
	land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
}

void recalculate_all_cards_in_play(void)
{
  // Original at 0x4351C0

  player_bits[0] &= ~PB_CANT_HAVE_OR_GAIN_ABILITIES_MASK;
  player_bits[1] &= ~PB_CANT_HAVE_OR_GAIN_ABILITIES_MASK;
  player_bits[0] &= ~PB_LANDWALK_DISABLED_MASK;
  player_bits[1] &= ~PB_LANDWALK_DISABLED_MASK;
  player_bits[0] &= ~PB_PLAYER_HAS_SHROUD;
  player_bits[0] &= ~PB_PLAYER_HAS_HEXPROOF;
  player_bits[1] &= ~PB_PLAYER_HAS_SHROUD;
  player_bits[1] &= ~PB_PLAYER_HAS_HEXPROOF;
  player_bits[0] &= ~PB_CAN_USE_PW_ABILITIES_AS_INSTANT;
  player_bits[1] &= ~PB_CAN_USE_PW_ABILITIES_AS_INSTANT;
  player_bits[0] &= ~PB_COMMANDER_IN_PLAY;
  player_bits[1] &= ~PB_COMMANDER_IN_PLAY;

  minimize_nondraining_mana();

  int p, c;

  for (p = 0; p < 2; ++p)
	{
	  card_instance_t* instance = get_card_instance(p, 0);
	  for (c = 0; c < active_cards_count[p]; ++c, ++instance)
		if (instance->internal_card_id != -1
			&& (instance->state & (STATE_OUBLIETTED|STATE_IN_PLAY)) == STATE_IN_PLAY)	// These two (almost) equivalent to in_play(), which also excludes cards with STATE_INVISIBLE set
		  {
			// Original order here: regen status gets all five recalc flags; SET_COLOR; CHANGE_TYPE.  Abilities not forced to recalc.
			instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
			get_abilities(p, c, EVENT_CHANGE_TYPE, -1);
			instance->regen_status |= KEYWORD_RECALC_SET_COLOR|KEYWORD_RECALC_ABILITIES|KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS;
			get_abilities(p, c, EVENT_SET_COLOR, -1);
			get_abilities(p, c, EVENT_ABILITIES, -1);
		  }
	}

  count_colors_of_lands_in_play();

  count_mana();

  player_bits[0] &= ~PB_HAND_REVEALED;
  player_bits[1] &= ~PB_HAND_REVEALED;

  card_instance_t* instance;
  for (p = 0; p < 2; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((instance = in_play(p, c)))
		{
		  if (is_what(p, c, TYPE_CREATURE))
			{
			  get_abilities(p, c, EVENT_TOUGHNESS, -1);
			  if (instance->internal_card_id == -1)	// destroyed during get_abilities by lethal damage or toughness < 0
				continue;
			  get_abilities(p, c, EVENT_POWER, -1);
			}

		  /* 704.5r If a permanent has both a +1/+1 counter and a -1/-1 counter on it, N +1/+1 and N -1/-1 counters are removed from it, where N is the smaller
		   * of the number of +1/+1 and -1/-1 counters on it. */
		  if (instance->counters > 0 && instance->counters_m1m1 > 0 && is_what(p, c, TYPE_PERMANENT)
			  && !(instance->toughness <= 0 && is_what(p, c, TYPE_CREATURE)))
			{
			  int n = MIN(instance->counters, instance->counters_m1m1);
			  instance->counters -= n;
			  instance->counters_m1m1 -= n;
			}

		  /* There may well be a better place for this.  recalculate_all_cards_in_play() is called, at least, after a card or effect card resolves, a turn
		   * begins, at the end of untap, the start of the main phase, and when a permanent is killed. */
		  dispatch_event_with_attacker_to_one_card(p, c, EVENT_STATIC_EFFECTS, 1-p, -1);
		}

  if (ai_is_speculating == 0)	// Usually only checked for != 1.  Makes a difference during startup, I think.
	{
	  if (!(player_bits[0] & PB_HAND_REVEALED) != !(EXE_DWORD(0x628C24) & (1<<0)))
		{
		  if (player_bits[0] & PB_HAND_REVEALED)
			EXE_DWORD(0x628C24) |= 1<<0;
		  else
			EXE_DWORD(0x628C24) &= ~(1<<0);
		}

	  if (!(player_bits[1] & PB_HAND_REVEALED) != !(EXE_DWORD(0x628C24) & (1<<1)))
		{
		  if (player_bits[1] & PB_HAND_REVEALED)
			EXE_DWORD(0x628C24) |= 1<<1;
		  else
			EXE_DWORD(0x628C24) &= ~(1<<1);

		  EXE_FN(void, 0x47D2F0, void*)(EXE_PTR_VOID(0x786DC4));	// TENTATIVE_update_hand_window(hwnd_HandClass_opponent);
		}
	}
}

void phase_changed(int player, int new_phase)
{
  //0x435F20
  if (ai_is_speculating != 1)
	EXE_FN(void, 0x4377e0, int, int)(player, new_phase);	// TENTATIVE_update_display_for_changed_phase(player, new_phase)

  cancel = 0;

  // Begin additions
  dispatch_event(player, 0, EVENT_PHASE_CHANGED);
  // End additions
}

int target_player_skips_untap(int player, int card, event_t event);

int untap_phase(int player)
{
  // Return 1 to skip the rest of the turn (without even a cleanup step!).  The exe only does this if a player dies.
  player_bits[0] &= ~PB_SEND_EVENT_UNTAP_CARD_TO_ALL;
  player_bits[1] &= ~PB_SEND_EVENT_UNTAP_CARD_TO_ALL;
  event_flags &= ~EF_ATTACKER_CHOOSES_BLOCKERS;	// Should get zerod at end of turn, but might as well be sure.
  dispatch_event(player, 0, EVENT_BEGIN_TURN);	// even if untap step will end up being skipped

  if (trace_mode & 2)
	{
	  char buf[200];
	  sprintf(buf, "%d: Entering Untap Phase.\n", ++EXE_DWORD(0x60EC40));
	  EXE_FN(void, 0x4A7D80, const char*)(buf);
	}

  /* The comprehensive rules don't say whether a temporary "You skip your next untap step" effect like Yosei, the Morning Star's goes away when there's a
   * continuous one like Stasis's on the battlefield, and I can't find a ruling.  I'm going to assume the active player chooses, and that he always chooses to
   * use the temporary one (so if the Stasis goes away, he'll untap sooner). */

  card_instance_t* instance;

  // Effects that make a specific player skip his next untap phase.  Also check for Shimmer here.
  int shimmering_lands = 0;
  int p, c;
  for (p = 0; p < 2; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((instance = in_play(p, c)))
		{
		  if (instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)target_player_skips_untap && instance->targets[0].player == player)
			{
			  kill_card(p, c, KILL_REMOVE);
			  return 0;
			}
		  else if (cards_data[instance->internal_card_id].id == CARD_ID_SHIMMER){
			shimmering_lands |= instance->info_slot;
		  }
		}

  // Permanents that make everyone skip their untap phase
  if (check_battlefield_for_id(2, CARD_ID_STASIS)
	  || check_battlefield_for_id(2, CARD_ID_THE_EON_FOG)
	  || check_battlefield_for_id(2, CARD_ID_SANDS_OF_TIME))
	return 0;

  untap_phasing(player, shimmering_lands);

  EXE_FN(int, 0x43A700, int)(player);	// The original version (with the Stasis check removed from the start, and the parts below removed)

  int untapped[151] = {0};
  for (c = 0; c < active_cards_count[player]; ++c)
	if ((instance = in_play(player, c)))
	  {
		if ((instance->untap_status & 3) == 3)
		  {
			// Begin additions
			untapped[c] = 1;
			// End additions
			instance->state &= ~STATE_TAPPED;
		  }

		// This is done in a separate loop in the exe; I don't think that's necessary.
		SET_BYTE0(instance->untap_status) = 0;
	  }

  // Begin additions
  count_colors_of_lands_in_play();
  count_mana();

  for (c = 0; c < active_cards_count[player]; ++c)
	if (untapped[c])
	  {
		if (player_bits[player] & PB_SEND_EVENT_UNTAP_CARD_TO_ALL)
		  dispatch_event(player, c, EVENT_UNTAP_CARD);
		else
		  dispatch_event_with_attacker_to_one_card(player, c, EVENT_UNTAP_CARD, 1-player, -1);
	  }
  // End additions

  recalculate_all_cards_in_play();
  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()

  if (!EXE_DWORD(0x7a34d0))
	EXE_FN(int, 0x439c90, int)(1);	// TENTATIVE_check_for_stops(1)

  // Begin additions
  dispatch_event(player, 0, EVENT_END_OF_UNTAP_STEP);
  // End additions

  EXE_FN(void, 0x43a060, void)();	// mana_burn()

  return is_anyone_dead();
}

extern int hack_ai_decision_phase_upkeep;
int ai_decision_phase(int player, int *phase_code_to_go_to, int *becomes_second_arg_of_main_phase_and_discard_phase, int *becomes_third_arg_of_main_phase);
void mana_burn(void);
int upkeep_phase(int player)
{
  // 0x43acf0

  if (trace_mode & 2)
	{
	  char str[100];
	  sprintf(str, "%d: Entering UpKeep Phase.\n", EXE_DWORD(0x60EC40)++);
	  EXE_FN(void, 0x4a7d80, const char*)(str);	// append_to_trace_txt(str)
	}

  EXE_DWORD(0x7A2FD8) = 0;
  EXE_DWORD(0x7A34D0) = 0;
  EXE_DWORD(0x60A43C) = 0;

  // I strongly suspect this has to do with phase stops.
  if (trace_mode & 2)
	{
	  if (EXE_DWORD(0x4EF1AC) == -1)
		EXE_DWORD(0x60A54C) = 0;
	  else
		EXE_DWORD(0x60A54C) = ((EXE_DWORD(0x4EF1AC) != 4 || EXE_DWORD(0x4EF1B0) != player) && (EXE_DWORD(0x60A4AC) != 4 || EXE_DWORD(0x60A55C) != player));
	}
  else if (ai_is_speculating != 1)
	EXE_DWORD(0x60A54C) = (EXE_DWORD(0x4EF1AC) != -1 && (EXE_DWORD(0x4EF1AC) != 4 || EXE_DWORD(0x4EF1B0) != player));

  // Begin additions
  /* Normally ai speculation runs until end of turn and ai_decision_phase() returns a code to make switch_phase() go back to where speculation started, but
   * main_phase() seems to assume no speculation takes place before it. */
  if (ai_is_speculating != 1 && !(trace_mode & 2))
	{
	  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()
	  EXE_FN(void, 0x435140, int, int)(17, 10);	// TENTATIVE_backup_data_for_ai_frontend_0(17, 5)	// first arg is unique to each call of TENTATIVE_backup_data_for_ai_frontend_0(), ends up in EXE_DWORD(0x60EC3C), and is interpreted in ai_decision_phase(); the latter is a relative amount of time to spend speculating, and is always 30 in exe calls.  Lower here since we're only speculating until end of phase.
	}
 speculate_more:
  if (EXE_DWORD(0x60EC3C) == 17)
	{
	  //card_instance_t* instance = get_card_instance(1,9);
	  EXE_FN(void, 0x498ED0, void)();	// TENTATIVE_restore_data_for_ai_frontend_0()
	  EXE_DWORD(0x72835C) = 0;
	  ai_modifier = 0;
	}
 // End additions

  current_phase = PHASE_BEGIN_UPKEEP;
  phase_changed(player, PHASE_BEGIN_UPKEEP);
  dispatch_trigger(player, TRIGGER_UPKEEP, EXE_STR(0x739C20), 0);	//PROMPT_SPECIALFEPHASE[8]

  EXE_DWORD(0x60A43C) = 1;
  current_phase = PHASE_UPKEEP;
  EXE_FN(void, 0x437620, void)();	// set_all_upkeep_flags_to_0();
  EXE_FN(int, 0x436A20, int, int, const char*, int)(-1, current_phase, EXE_STR(0x60EEAC)/*PROMPT_CHECKFEPHASE[5]*/, EVENT_UPKEEP_PHASE);	// allow_response()
  EXE_DWORD(0x60A43C) = 0;

  dispatch_trigger2(player, TRIGGER_END_UPKEEP, EXE_STR(0x78FA70), 0, 0, 0);	//PROMPT_SPECIALFEPHASE[9]

  EXE_DWORD(0x60A54C) = 0;
  EXE_DWORD(0x620860) = 1;
  EXE_FN(void, 0x477070, void)();	// resolve_damage_cards_and_prevent_damage();

  EXE_DWORD(0x620860) = 0;
  EXE_DWORD(0x7A2FD8) = 0;
  mana_burn();

  // Begin additions
  int phase_code_to_go_to, becomes_second_arg_of_main_phase_and_discard_phase, becomes_third_arg_of_main_phase;
  hack_ai_decision_phase_upkeep = 1;
  ai_decision_phase(player, &phase_code_to_go_to, &becomes_second_arg_of_main_phase_and_discard_phase, &becomes_third_arg_of_main_phase);
  if (phase_code_to_go_to == 4)
	goto speculate_more;
  // End additions

  return is_anyone_dead();
}

int is_legal_block_impl(int blocking_player, int blocking_card, int blocked_player, int blocked_card, keyword_t abils, int landwalks)
{
  // 0x434f90
  card_instance_t* blocking_inst = get_card_instance(blocking_player, blocking_card);
  if (blocking_inst->internal_card_id < 0
	  || !((cards_data[blocking_inst->internal_card_id].type & TYPE_CREATURE) || (blocking_inst->state & STATE_NONCREATURE_CAN_BLOCK))
	  || blocking_inst->state & (STATE_OUBLIETTED|STATE_TAPPED|STATE_BLOCKING)
	  || ((blocking_inst->state & STATE_ATTACKING) && !(get_special_abilities_by_instance(blocking_inst) & SP_KEYWORD_VIGILANCE))
	  || !(blocking_inst->state & STATE_IN_PLAY)
	  || ((abils & KEYWORD_FLYING) && !(get_abilities(blocking_player, blocking_card, EVENT_ABILITIES, -1) & (KEYWORD_REACH|KEYWORD_FLYING))))
	return 0;

  if (abils & KEYWORD_PROT_COLORED)
	{
	  int colorbits = get_color(blocking_player, blocking_card);
	  colorbits <<= 10;	// COLOR_TEST_BLACK => KEYWORD_PROT_BLACK
	  if (colorbits & abils & KEYWORD_PROT_COLORED)
		return 0;
	}
  if (is_what(blocking_player, blocking_card, TYPE_ARTIFACT)
	  && (abils & KEYWORD_PROT_ARTIFACTS))
	return 0;

	if(abils & landwalks & KEYWORD_BASIC_LANDWALK ){
		if( (abils & landwalks & KEYWORD_SWAMPWALK) && !(player_bits[blocked_player] & PB_SWAMPWALK_DISABLED) ){
			return 0;
		}
		if( (abils & landwalks & KEYWORD_ISLANDWALK) && !(player_bits[blocked_player] & PB_ISLANDWALK_DISABLED) ){
			return 0;
		}
		if( (abils & landwalks & KEYWORD_FORESTWALK) && !(player_bits[blocked_player] & PB_FORESTWALK_DISABLED) ){
			return 0;
		}
		if( (abils & landwalks & KEYWORD_MOUNTAINWALK) && !(player_bits[blocked_player] & PB_MOUNTAINWALK_DISABLED) ){
			return 0;
		}
		if( (abils & landwalks & KEYWORD_PLAINSWALK) && !(player_bits[blocked_player] & PB_PLAINSWALK_DISABLED) ){
			return 0;
		}
	}

  // Begin additions
  card_instance_t* blocked_inst = get_card_instance(blocked_player, blocked_card);
  int sp_keywords_blocked = (blocked_inst->targets[16].card < 0 || is_humiliated(blocked_player, blocked_card)) ? 0 : blocked_inst->targets[16].card;
  int sp_keywords_blocking = (blocking_inst->targets[16].card < 0 || is_humiliated(blocking_player, blocking_card)) ? 0 : blocking_inst->targets[16].card;

  if (sp_keywords_blocking & SP_KEYWORD_CANNOT_BLOCK)
	return 0;
  if (sp_keywords_blocked & SP_KEYWORD_UNBLOCKABLE)
	return 0;
  if ((sp_keywords_blocked & SP_KEYWORD_SHADOW) && !(sp_keywords_blocking & (SP_KEYWORD_SHADOW | SP_KEYWORD_SHADOW_HOSER)))
	return 0;
  if (!(sp_keywords_blocked & SP_KEYWORD_SHADOW) && (sp_keywords_blocking & SP_KEYWORD_SHADOW))
	return 0;
  if ((sp_keywords_blocked & SP_KEYWORD_HORSEMANSHIP) && !(sp_keywords_blocking & SP_KEYWORD_HORSEMANSHIP))
	return 0;
  if ((sp_keywords_blocked & SP_KEYWORD_FEAR)
	  && !((get_color(blocking_player, blocking_card) & COLOR_TEST_BLACK) || is_what(blocking_player, blocking_card, TYPE_ARTIFACT)))	// Intentionally not sleightable
	return 0;
  if ((sp_keywords_blocked & SP_KEYWORD_INTIMIDATE)
	  && !(has_my_colors(blocking_player, blocking_card, blocked_player, blocked_card) || is_what(blocking_player, blocking_card, TYPE_ARTIFACT)))
	return 0;
  // End additions

  int result = dispatch_event_with_attacker(blocking_player, blocking_card, EVENT_BLOCK_LEGALITY, blocked_player, blocked_card);

  return result > 0 ? 0 : 1;
}

int is_legal_block(int blocking_player, int blocking_card, int blocked_player, int blocked_card)
{
  // Two copies, at 0x434f30 and 0x4b9220.

  /* No changes here, but moved into C for clarity.
   *
   * It's probably a bad idea overall to change this; it's inlined in about a half dozen places for efficiency, only computing abils and landwalks once and then
   * checking multiple blocking_card's against blocked_card.
   *
   * landwalks_controlled() is based on basiclandtypes_controlled[][], which is set in count_colors_of_lands_in_play(), overridden above. */

  int abils = get_abilities(blocked_player, blocked_card, EVENT_ABILITIES, -1);

  int player1_landwalks, player0_landwalks, landwalks;
  EXE_FN(void, 0x499DF0, int*, int*)(&player1_landwalks, &player0_landwalks);	// landwalks_controlled(&player1_landwalks, &player0_landwalks)
  landwalks = (blocking_player == 1) ? player1_landwalks : player0_landwalks;

  return is_legal_block_impl(blocking_player, blocking_card, blocked_player, blocked_card, abils, landwalks);
}

static int check_attack_legality_if_enchanting_affected_card(int player, int card)
{
  //0x4b7110
  // The exe version is a callback for call_function_for_each_card_in_play(), and the temporaries are saved in can_attack_instead of here.

  push_affected_card_stack();

  event_result = 0;
  affected_card_controller = player;
  affected_card = card;

  // Begin call_function_for_each_card_in_play()
  /* (This differs from dispatch_event_raw() in that the cards are examined in id order, not in timestamp order.  Also, it calls an arbitrary function instead
   * of calling each card's own function with a given event.) */
  {
	int p, c;
	for (p = 0; p < 2; ++p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c))
		  {
			// Begin exe version of check_attack_legality_if_enchanting_affected_card()
			card_instance_t* instance = get_card_instance(player, card);
			if (affect_me(instance->damage_target_player, instance->damage_target_card) && affected_card != -1)
			  {
				call_card_function_i(instance, player, card, EVENT_ATTACK_LEGALITY);
				if (cancel == 1)
				  {
					++event_result;
					cancel = 0;
				  }
			  }
			// End exe version of check_attack_legality_if_enchanting_affected_card()
		  }
  }
  // End call_function_for_each_card_in_play()

  int result = event_result;

  pop_affected_card_stack();

  return result;
}

int can_attack(int player, int card)
{
  //0x434c30
  card_instance_t* instance = get_card_instance(player, card);

  if (instance->internal_card_id >= 0
	  && (!(instance->regen_status & KEYWORD_DEFENDER)
		  || (instance->token_status & STATUS_WALL_CAN_ATTACK))
	  && (is_what(player, card, TYPE_CREATURE) || (instance->state & STATE_NONCREATURE_CAN_ATTACK))
	  && !(instance->state & (STATE_OUBLIETTED|STATE_SUMMON_SICK|STATE_TAPPED))
	  && !(instance->token_status & STATUS_CANT_ATTACK))
	{
	  int old_cancel = cancel;
	  int old_event_result = event_result;
	  int old_affected_card_controller = affected_card_controller;
	  int old_affected_card = affected_card;

	  event_result = 0;
	  affected_card_controller = player;
	  affected_card = card;

	  call_card_function_i(instance, player, card, EVENT_ATTACK_LEGALITY);
	  int new_event_result = event_result;

	  affected_card = old_affected_card;
	  affected_card_controller = old_affected_card_controller;
	  event_result = old_event_result;
	  cancel = old_cancel;

	  if (new_event_result)
		return 0;

	  if (player != AI
		  || (trace_mode & 2)
		  || !check_attack_legality_if_enchanting_affected_card(player, card))
		return ((!(player_bits[1 - player] & PB_OPPONENT_CHECKS_ATTACK_LEGALITY)
				 && !(event_flags & EA_DECLARE_ATTACK))
				|| !dispatch_event(player, card, EVENT_ATTACK_LEGALITY));
	}

  return 0;
}

int effect_asterisk(int player, int card, event_t event)
{
  // 0x4a1bb0
  card_instance_t* instance = get_card_instance(player, card);
  if (!affect_me(instance->damage_target_player, instance->damage_target_card)
	  || affected_card == -1
	  || !((event == EVENT_POWER && (instance->eot_toughness & ASTERISK_AFFECTS_POWER))
		   || (event == EVENT_TOUGHNESS && (instance->eot_toughness & ASTERISK_AFFECTS_TOUGHNESS))))
	return 0;

  int p, c;
  int tp = instance->damage_target_player, tc = instance->damage_target_card;
  int modifier = 0;
  int count_who = instance->eot_toughness & (ASTERISK_COUNT_CONTROLLER | ASTERISK_COUNT_OPPONENT);
  int category = instance->eot_toughness & (ASTERISK_SUBTYPE_OF_INFO_SLOT
											|ASTERISK_HAND_COUNT
											|ASTERISK_CREATURES_IN_GRAVEYARD_PLUS_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_OF_INFO_SLOT
											|ASTERISK_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_MIN1_OF_INFO_SLOT
											|ASTERISK_NONWALL_CREATURES
											|ASTERISK_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_OF_INFO_SLOT
											|ASTERISK_IID_OF_INFO_SLOT
											|ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT);

  switch (category)
	{
	  case ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT:
		/* Dakkon Blackblade 0x403840 (replaced in C): ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT
		 *                                             |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                                             |ASTERISK_COUNT_CONTROLLER
		 *                                             info_slot = COLOR_ANY */
		/* People of the Woods 0x42a910: ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT
		 *                               |ASTERISK_AFFECTS_TOUGHNESS
		 *                               |ASTERISK_COUNT_CONTROLLER
		 *                               info_slot = COLOR_GREEN, token_status |= STATUS_BASICLAND_DEPENDANT */
		/* Nightmare 0x4c5450 (replaced in C): ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT
		 *                                     |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                                     |ASTERISK_COUNT_CONTROLLER
		 *                                     info_slot = COLOR_BLACK, token_status |= STATUS_BASICLAND_DEPENDANT */
		/* Angry Mob 0x4c54c0: ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT
		 *                     |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                     |ASTERISK_COUNT_OPPONENT (set only during controller's turn)
		 *                     info_slot = COLOR_BLACK */
		/* Gaea's Liege 0x4c5570: ASTERISK_BASICLAND_TYPES_OF_INFO_SLOT
		 *                        |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                        |ASTERISK_COUNT_OPPONENT (set only while STATE_ATTACKING)
		 *                        |ASTERISK_COUNT_CONTROLLER (set only while not STATE_ATTACKING)
		 *                        info_slot = COLOR_GREEN */
		if (instance->eot_toughness & ASTERISK_COUNT_CONTROLLER)
		  modifier += basiclandtypes_controlled[tp][get_hacked_color(tp, tc, instance->info_slot)];

		if (instance->eot_toughness & ASTERISK_COUNT_OPPONENT)
		  modifier += basiclandtypes_controlled[1 - tp][get_hacked_color(tp, tc, instance->info_slot)];

		break;

	  case ASTERISK_IID_OF_INFO_SLOT:
		/* Beast of Burden 0x403c00: ASTERISK_TYPE_OF_INFO_SLOT__MUST_ALSO_SET_IID_OF_INFO_SLOT|ASTERISK_IID_OF_INFO_SLOT
		 *                           |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                           |ASTERISK_COUNT_OPPONENT|ASTERISK_COUNT_CONTROLLER
		 *                           info_slot = TYPE_CREATURE */
		/* Gaea's Avenger 0x452cd0: ASTERISK_TYPE_OF_INFO_SLOT__MUST_ALSO_SET_IID_OF_INFO_SLOT|ASTERISK_IID_OF_INFO_SLOT
		 *                          |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                          |ASTERISK_COUNT_OPPONENT
		 *                          info_slot = TYPE_ARTIFACT */
		/* Plague Rats 0x4c5790: ASTERISK_IID_OF_INFO_SLOT
		 *                           |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                           |ASTERISK_COUNT_OPPONENT|ASTERISK_COUNT_CONTROLLER
		 *                           info_slot = (creature's internal_card_id) */
		for (p = 0; p < 2; ++p)
		  if (count_who & (p == tp ? ASTERISK_COUNT_CONTROLLER : ASTERISK_COUNT_OPPONENT))
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_play(p, c))
				{
				  if (instance->eot_toughness & ASTERISK_TYPE_OF_INFO_SLOT__MUST_ALSO_SET_IID_OF_INFO_SLOT)
					{
					  if (is_what(p, c, instance->info_slot))
						++modifier;
					}
				  else if (get_id(p, c) == cards_data[instance->info_slot].id)
					++modifier;
				}
		break;

	  case ASTERISK_NONWALL_CREATURES:
		/* Keldon Warlord 0x4c57f0: ASTERISK_NONWALL_CREATURES
		 *                          |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                          |ASTERISK_COUNT_CONTROLLER */
		for (p = 0; p < 2; ++p)
		  if (count_who & (p == tp ? ASTERISK_COUNT_CONTROLLER : ASTERISK_COUNT_OPPONENT))
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && !has_subtype(p, c, SUBTYPE_WALL))
				++modifier;
		break;

	  case ASTERISK_CREATURES_IN_GRAVEYARD_PLUS_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_OF_INFO_SLOT:
		/* Lhurgoyf 0x40df80: ASTERISK_CREATURES_IN_GRAVEYARD_PLUS_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_OF_INFO_SLOT
		 *                    |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                    |ASTERISK_COUNT_OPPONENT|ASTERISK_COUNT_CONTROLLER
		 *                    info_slot = 0x100 */
		for (p = 0; p < 2; ++p)
		  if (count_who & (p == tp ? ASTERISK_COUNT_CONTROLLER : ASTERISK_COUNT_OPPONENT))
			for (c = 0; c < 500; ++c)
			  {
				int iid = get_grave(p)[c];
				if (iid >= 0 && is_what(-1, iid, TYPE_CREATURE))
				  ++modifier;
			  }

		if (event == EVENT_POWER)
		  modifier += BYTE0(instance->info_slot);
		else
		  modifier += BYTE1(instance->info_slot);
		break;

	  case ASTERISK_HAND_COUNT:
		/* Maro 0x40e4a0 (replaced in C): ASTERISK_HAND_COUNT
		 *                                |ASTERISK_AFFECTS_TOUGHNESS|ASTERISK_AFFECTS_POWER
		 *                                |ASTERISK_COUNT_CONTROLLER
		 *                                info_slot = 0 */
		if (count_who & ASTERISK_COUNT_CONTROLLER)
		  modifier += hand_count[player];
		if (count_who & ASTERISK_COUNT_OPPONENT)
		  modifier += hand_count[1-player];
		break;

	  case ASTERISK_SUBTYPE_OF_INFO_SLOT:
		/* Swarm of Rats 0x4140f0: ASTERISK_SUBTYPE_OF_INFO_SLOT
		 *                         |ASTERISK_AFFECTS_POWER
		 *                         |ASTERISK_COUNT_CONTROLLER
		 *                         info_slot = SUBTYPE_RAT */
		for (p = 0; p < 2; ++p)
		  if (count_who & (p == tp ? ASTERISK_COUNT_CONTROLLER : ASTERISK_COUNT_OPPONENT))
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_play(p, c) && has_subtype(p, c, from_hardcodedsubtype_to_subtype(instance->info_slot)))
				++modifier;
		break;

	  case ASTERISK_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_OF_INFO_SLOT:
		// Unused
		if (event == EVENT_POWER)
		  modifier = BYTE0(instance->info_slot);
		else
		  modifier = BYTE1(instance->info_slot);
		break;

	  case ASTERISK_POWER_IN_BYTE0_TOUGHNESS_IN_BYTE1_MIN1_OF_INFO_SLOT:
		// Unused
		if (event == EVENT_POWER)
		  modifier = BYTE0(instance->info_slot);
		else
		  {
			modifier = BYTE1(instance->info_slot);
			if (event_result + modifier < 1)
			  {
				if (event_result > 1)
				  modifier = 1 - event_result;
				else
				  modifier = 0;
			  }
		  }
		break;
	}

  event_result += modifier;

  /* counter_power/toughness on the effect card, not what it's attached to.  The Sorceress Queen effect looks for this effect when determining the attached
   * card's characteristic-defining power/toughness. */
  if (event == EVENT_POWER)
	instance->counter_power = modifier;
  else
	instance->counter_toughness = modifier;

  return 0;
}

// If an effect is created with source hack_force_effect_change_source.from, change it to be hack_force_effect_change_source.to before sending it to the exe.
HackForceEffectChangeSource hack_force_effect_change_source = {{-1,-1}, {-1,-1}};

int create_legacy_effect_exe(int player, int card, int iid, int t_player, int t_card)
{
  // Not a replacement for the original function at 0x4a09a0, but a front end to make things right if called with an activation card or another effect.

  if (hack_force_effect_change_source.from.player != -1
	  && player == hack_force_effect_change_source.from.player
	  && card == hack_force_effect_change_source.from.card)
	{
	  player = hack_force_effect_change_source.to.player;
	  card = hack_force_effect_change_source.to.card;
	}

  card_instance_t* instance = get_card_instance(player, card);
  int csvid;

  if (instance->internal_card_id == activation_card)
	return EXE_FN(int, 0x4a09a0, int, int, int, int, int)(instance->parent_controller, instance->parent_card, iid, t_player, t_card);

  if (instance->internal_card_id < 0)	// Maybe use original_internal_card_id?
	return EXE_FN(int, 0x4a09a0, int, int, int, int, int)(player, card, iid, t_player, t_card);

  csvid = cards_data[instance->internal_card_id].id;
  if (csvid >= 901 && csvid <= 908)
	{
	  int leg = EXE_FN(int, 0x4a09a0, int, int, int, int, int)(player, card, iid, t_player, t_card);
	  if (leg != -1)
		{
		  card_instance_t* legacy = get_card_instance(player, leg);
		  legacy->damage_source_player = instance->damage_source_player;
		  legacy->damage_source_card = instance->damage_source_card;
		  legacy->original_internal_card_id = instance->original_internal_card_id;
		  legacy->display_pic_csv_id = instance->display_pic_csv_id;
		  legacy->display_pic_num = instance->display_pic_num;
		}
	  return leg;
	}

  if (csvid == CARD_ID_FACE_DOWN_CREATURE && instance->targets[12].player >= 0 && instance->targets[12].player <= available_slots)
	{
	  int leg = EXE_FN(int, 0x4a09a0, int, int, int, int, int)(player, card, iid, t_player, t_card);
	  if (leg != -1)
		{
		  card_instance_t* legacy = get_card_instance(player, leg);
		  legacy->original_internal_card_id = get_internal_card_id_from_csv_id(instance->targets[12].player);
		  legacy->display_pic_csv_id = instance->targets[12].player;
		  legacy->display_pic_num = get_card_image_number(instance->targets[12].player, player, card);
		}
	  return leg;
	}

  return EXE_FN(int, 0x4a09a0, int, int, int, int, int)(player, card, iid, t_player, t_card);
}

// Creates a sourceless effect card, with name/text/image from iid.
int create_legacy_effect_from_iid(int player, int iid, int (*func_ptr)(int, int, event_t), int t_player, int t_card)
{
  int legacy_card = add_card_to_hand(player, LEGACY_EFFECT_CUSTOM);
  if (legacy_card == -1)
	return -1;

  card_instance_t* legacy = get_card_instance(player, legacy_card);
  legacy->state = (player == 1) ? STATE_IN_PLAY | STATE_OWNED_BY_OPPONENT : STATE_IN_PLAY;
  --hand_count[player];

  card_data_t* cd = &cards_data[iid];
  legacy->mana_color = legacy->card_color = legacy->initial_color = legacy->color = cd->color;
  legacy->damage_source_player = player;
  legacy->damage_source_card = legacy_card;

  int csvid = cards_data[iid].id;

  legacy->display_pic_csv_id = csvid;
  int num_pics = cards_ptr[csvid]->num_pics;
  legacy->display_pic_num = num_pics <= 1 ? 0 : internal_rand(num_pics);

  legacy->damage_target_player = t_player;
  legacy->damage_target_card = t_card;

  if (t_player != -1 && t_card != -1)
	{
	  card_instance_t* attached_to = get_card_instance(t_player, t_card);
	  attached_to->regen_status |= KEYWORD_RECALC_ALL;
	}

  legacy->info_slot = (int)func_ptr;

  return legacy_card;
}

int hack_xx = 1;
void format_manacost_into_global_allpurpose_buffer(int ignored, int* mana_cost_array, int x_so_far, int max_x)
{
  // 0x42f1f0

  char intro[300];
  intro[0] = 0;

  if (stack_size > 0)
	// sprintf_CastingActivatingOrProcessing_CardName(...)
	EXE_FN(void, 0x436980, char*, int, int, int)(intro, stack_data[stack_size - 1].generating_event,
												 stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card);

  char str[100];
  str[0] = 0;
  char* p = str;

  if (mana_cost_array[COLOR_COLORLESS] == -1 || mana_cost_array[COLOR_ARTIFACT] == -1)
	{
	  // Begin additions
	  int our_xx = hack_xx;
	  const char* cost;
	  if (hack_xx == 1
		  && stack_size > 0
		  && stack_data[stack_size - 1].generating_event == EVENT_RESOLVE_SPELL
		  && (cost = cards_ptr[get_id(stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card)]->mana_cost_text)
		  && strstr(cost, "|X|X"))
		{
		  our_xx = 2;
		  if (strstr(cost, "|X|X|X"))
			our_xx = 3;
		}

	  int divisor = our_xx == 2 ? 2 : our_xx == 3 ? 3 : 1;
	  const char* fmt = divisor == 1 ? "|X" : divisor == 2 ? "|X|X" : "|X|X|X";
	  // End additions

	  if (max_x == -1)
		p += sprintf(p, EXE_STR(0x728578)/*PROMPT_GRABMANA[1]*/, fmt, x_so_far / divisor);
	  else if (max_x > x_so_far)
		p += sprintf(p, EXE_STR(0x62853C)/*PROMPT_GRABMANA[2]*/, fmt, x_so_far / divisor, max_x / divisor);
	}
  else if (mana_cost_array[COLOR_COLORLESS] || mana_cost_array[COLOR_ARTIFACT])
	{
	  int generic = mana_cost_array[COLOR_COLORLESS] + mana_cost_array[COLOR_ARTIFACT];
	  if (generic > 20)
		while (generic >= 10)
		  {
			p += sprintf(p, "|10");
			generic -= 10;
		  }

	  if (generic)
		p += sprintf(p, "|%d", generic);
	}

  char mana_symbol[3] = {'|', 'B', 0};
  const char* mana_colors = "XBUGRW";
  int i, j;
  for (i = COLOR_BLACK; i <= COLOR_WHITE; ++i)
	if (mana_cost_array[i])
	  {
		mana_symbol[1] = mana_colors[i];

		if (mana_cost_array[i] == -1)
		  {
			if (max_x == -1)
			  p += sprintf(p, EXE_STR(0x728578)/*PROMPT_GRABMANA[1]*/, mana_symbol, x_so_far);
			else if (max_x > x_so_far)
			  p += sprintf(p, EXE_STR(0x62853C)/*PROMPT_GRABMANA[2]*/, mana_symbol, x_so_far, max_x);
		  }
		else
		  for (j = 0; mana_cost_array[i] > j; ++j)
			p += sprintf(p, "%s", mana_symbol);
	  }

  p = (char*)(0x60a690);	// global_all_purpose_buffer[]
  p += sprintf(p, "%s, ", intro);
  sprintf(p, EXE_STR(0x786B00)/*PROMPT_GRABMANA[0]*/, str);
}

void event_activate_then_duplicate_into_stack(int player, int card, int event, int new_attacking_card_controller, int new_attacking_card);
void human_autotap_for_mana(int player, int* amt, int* unknown_v46, int unknown_v47);
int pay_mana_maximally_satisfied(int* pay_mana_array, int x_val, int max_x_val);
int try_to_pay_for_mana_by_autotapping(int player, int* amt, int* unknown_v46, int autotap_flags, int unknown_v47);
extern int pay_mana_xbugrwaU[8];
#define APPLY_MANA_IN_POOL_TO_COSTS(total_paid)	\
	apply_mana_in_pool_to_cost(player, amtspaid, total_paid, amt_of_nonartifact_mana);	\
	apply_mana_in_pool_to_colorless_cost(player, amtspaid, total_paid, amt_of_nonartifact_mana, &x_value, max_x_value);	\
	apply_mana_in_pool_to_x_cost(player, amtspaid, total_paid, amt_of_nonartifact_mana, &x_value, max_x_value)

#define amt_to_apply_to_cost					EXE_FN(int, 0x4300d0, int*, int, int*, int, int, int, int)
#define apply_mana_in_pool_to_colorless_cost	EXE_FN(void, 0x42ef80, int, int*, int*, int, int*, int)
#define apply_mana_in_pool_to_cost				EXE_FN(void, 0x42ec40, int, int*, int*, int)
#define apply_mana_in_pool_to_x_cost			EXE_FN(void, 0x42edc0, int, int*, int*, int, int*, int)
#define apply_mana_to_cost						EXE_FN(void, 0x430060, int*, int, int, int*, int, int, color_t, int*, int*)
#define calculate_needed_mana_colors			EXE_FN(int, 0x42f1a0, int*)
#define copy_mana_pool_to_display				EXE_FN(void, 0x4388d0, void)
#define TENTATIVE_reassess_all_cards			EXE_FN(void, 0x472260, void)
#define resolve_top_card_on_stack				EXE_FN(void, 0x436740, void)
#define single_color_test_bit_to_color_t		EXE_FN(color_t, 0x4358f0, color_test_t)
#define TENTATIVE_update_mana_spent				EXE_FN(void, 0x430260, int*)

#define EXE_2D_ARR(typ, dim1, dim2, addr)		((typ(*)[dim2])addr)	// dim1 for documentation only

#define TENTATIVE_color_clicked_in_mana_summary_pane	EXE_DWORD(0x60a52c)
#define global_all_purpose_buffer				((char*)(0x60a690))
#define WILDGUESS_phase_to_run_to				EXE_DWORD(0x4ef1ac)
#define raw_mana_may_be_spent_as_color			EXE_2D_ARR(int, 2, 11, 0x60a4b4)
#define WILDGUESS_whose_phase_to_run_to			EXE_DWORD(0x4ef1b0)

int charge_mana(int player, color_t color, int amount)
{
  int old_affected_card_controller = affected_card_controller;
  int old_affected_card = affected_card;

  unknown62BCEC = 0;
  EXE_DWORD(0x4d7cb0) = 200;

  int amtspaid[COLOR_ARTIFACT + 1] = {0};
  int total_paid = 0;

  if (!(land_can_be_played & LCBP_CARD_BEING_COPIED))
    {
	  int old_land_can_be_played = land_can_be_played;
	  land_can_be_played |= LCBP_CHARGING_MANA;

#if 0	// Apparently MicroProse's attempt at Power Artifact.  We won't be using it.
      if (pay_mana_xbugrwaU[7] > 0)
		{
		  if (color)
			{
			  if (pay_mana_xbugrwaU[0] > 0)
				{
				  int diff = pay_mana_xbugrwaU[0] - 2*pay_mana_xbugrwaU[7];
				  pay_mana_xbugrwaU[0] = MAX(diff, 1);
				}
			}
		  else if (amount != -1)
			{
			  int diff = amount - 2*pay_mana_xbugrwaU[7];
			  amount = MAX(diff, 1);
			}
		}
#endif

      pay_mana_xbugrwaU[color] += amount;

      int amt_of_nonartifact_mana = has_mana(player, COLOR_ARTIFACT, 1) - has_mana(player, COLOR_ANY, 1);
      int autotap_matching_colors_in_pool = 1;	// a parameter in Shandalar
      int autotap_colored_in_pool_for_colorless, autotap_pool_for_x, autotap_sources_human, autotap_sources_ai;
      if (!ldoubleclicked
		  && !IS_AI(player))
		{
		  autotap_colored_in_pool_for_colorless = autotap_pool_for_x = autotap_sources_human = 0;
		  autotap_sources_ai = 0;
		}
      else
		{
		  //autotap_matching_colors_in_pool = 1;
		  autotap_colored_in_pool_for_colorless = autotap_pool_for_x = autotap_sources_human = 1;
		  if (!IS_AI(player))
			autotap_sources_ai = 0;
		  else
			autotap_sources_ai = 1;
		}

      if (player == AI && !(trace_mode & 2))
		{
		  int any_x_cost = 0;
		  int x = 0;

		  color_t c;
		  for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
			if (pay_mana_xbugrwaU[c] == -1)
			  {
				any_x_cost = 1;
				if (ai_is_speculating == 1)
				  {
					int avail = has_mana(player, c, 1);
					if (c == COLOR_COLORLESS)
					  {
						color_t d;
						for (d = COLOR_COLORLESS; d <= COLOR_ARTIFACT; ++d)
						  if (pay_mana_xbugrwaU[d] > 0)
							avail -= pay_mana_xbugrwaU[d];
					  }

					if (avail > 0)
					  x += avail;
				  }
			  }

		  if (any_x_cost)
			{
			  if (ai_is_speculating == 1)
				{
				  if (x > 1 && internal_rand(3))
					x = internal_rand(x - 1) + 1;

				  if (x > max_x_value && max_x_value != -1)
					x = max_x_value;

				  EXE_DWORD(0x7a2fe4) = x;		// TENTATIVE_ai_branch_value_99_is_abort
				  EXE_FN(void, 0x498f20, void)();	// TENTATIVE_ai_save_values_on_speculative_branch()
				}
			  else
				{
				  EXE_FN(int, 0x499050, void)();	// TENTATIVE_ai_retrieve_values_on_chosen_branch()

				  if (EXE_DWORD(0x7a2fe4) == 99)	// TENTATIVE_ai_branch_value_99_is_abort
					EXE_DWORD(0x7a2fe4) = 0;		// TENTATIVE_ai_branch_value_99_is_abort

				  x = EXE_DWORD(0x7a2fe4);		// TENTATIVE_ai_branch_value_99_is_abort
				}
			}

		  max_x_value = x;
		}

#if 0	// as above
      if (pay_mana_xbugrwaU[7] <= 0)
		x_value = 0;
      else
		x_value = 2 * pay_mana_xbugrwaU[7];
#else
      x_value = 0;	// Shandalar: = additional_x_avail (a parameter)
#endif

      if (max_x_value == 0)
		{
		  color_t c;
		  for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
			if (pay_mana_xbugrwaU[c] == -1)
			  pay_mana_xbugrwaU[c] = 0;
		}

      int pay_mana_has_x = 0;
      color_t c;
      for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
		if (pay_mana_xbugrwaU[c] == -1)
		  pay_mana_has_x = 1;

      if (autotap_matching_colors_in_pool
		  && !pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
		{
		  apply_mana_in_pool_to_cost(player, amtspaid, &total_paid, amt_of_nonartifact_mana);
		  copy_mana_pool_to_display();
		}

      if (autotap_colored_in_pool_for_colorless
		  && !pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
		{
		  apply_mana_in_pool_to_colorless_cost(player, amtspaid, &total_paid, amt_of_nonartifact_mana, &x_value, max_x_value);
		  copy_mana_pool_to_display();
		}

      if (autotap_pool_for_x && pay_mana_has_x
		  && !pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
		{
		  apply_mana_in_pool_to_x_cost(player, amtspaid, &total_paid, amt_of_nonartifact_mana, &x_value, max_x_value);
		  copy_mana_pool_to_display();
		}

      if (autotap_sources_human)
		human_autotap_for_mana(player, amtspaid, &total_paid, 1);

      if (autotap_sources_ai
		  && !pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
		{	// AI-only autotapping
		  try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, AUTOTAP_NO_CREATURES|AUTOTAP_NO_DONT_AUTO_TAP, 1);

		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
			try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, AUTOTAP_NO_DONT_AUTO_TAP, 1);

		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
			try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_NONBASIC_LANDS, 1);

		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
			try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS, 1);

		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
			try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, AUTOTAP_NO_CREATURES, 1);

		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
			try_to_pay_for_mana_by_autotapping(player, amtspaid, &total_paid, 0, 1);
		}

      if (IS_AI(player))
		{
		  if (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value)
			  && !pay_mana_has_x)
			cancel = 1;
		}
      else
		while (!pay_mana_maximally_satisfied(pay_mana_xbugrwaU, x_value, max_x_value))
		  {
			int any_nonx_mana = 0;
			for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
			  if (pay_mana_xbugrwaU[c] > 0)
				{
				  any_nonx_mana = 1;
				  break;
				}

			format_manacost_into_global_allpurpose_buffer(0, pay_mana_xbugrwaU, x_value, max_x_value);
			int card = EXE_FN(int, 0x49C8D0, int, int, int, int, int, char*, int)(player, player, player, 0, 0, global_all_purpose_buffer, any_nonx_mana ? 1 : 3);
			if (EXE_DWORD(0x60a550) != -1 || (card != -1 && card != -2))
			  {
				if (EXE_DWORD(0x60a550) == -1 || card != -1)
				  {
					card_instance_t* inst = get_card_instance(player, card);
					int iid = inst->internal_card_id;
					if (iid >= 0
						&& (cards_data[iid].extra_ability & EA_MANA_SOURCE)
						&& ((cards_data[iid].type & TYPE_INTERRUPT)
							|| (inst->state & (STATE_IN_PLAY | STATE_OUBLIETTED)) == STATE_IN_PLAY))
					  {
						int old_mana_pool[8], old_pay_mana[8], old_x_value, old_max_x_value, i;
						for (i = 0; i < 8; ++i)
						  old_mana_pool[i] = mana_pool[player][i];

						int (*old_charge_mana_addr_of_pre_mana_pool)[8] = charge_mana_addr_of_pre_mana_pool;
						charge_mana_addr_of_pre_mana_pool = &old_mana_pool;
						int* old_charge_mana_addr_of_x_value = charge_mana_addr_of_x_value;
						charge_mana_addr_of_x_value = &old_x_value;
						int (*old_charge_mana_addr_of_pay_mana_xbugrw)[8] = charge_mana_addr_of_pay_mana_xbugrw;
						charge_mana_addr_of_pay_mana_xbugrw = &old_pay_mana;
						int* old_charge_mana_addr_of_max_x_value = charge_mana_addr_of_max_x_value;
						charge_mana_addr_of_max_x_value = &old_max_x_value;

						old_x_value = x_value;
						x_value = 0;
						old_max_x_value = max_x_value;
						max_x_value = -1;
						for (i = 0; i < 8; ++i)
						  {
							old_pay_mana[i] = pay_mana_xbugrwaU[i];
							pay_mana_xbugrwaU[i] = 0;
						  }

						if ((inst->state & (STATE_OUBLIETTED|STATE_IN_PLAY)) != STATE_IN_PLAY)
						  {
							put_card_on_stack2(player, card);
							TENTATIVE_reassess_all_cards();
						  }
						else if (dispatch_event_to_single_card(player, card, EVENT_CAN_ACTIVATE, 1 - player, -1))
						  {
							put_card_or_activation_onto_stack(player, card, EVENT_RESOLVE_ACTIVATION, player, 0);
							needed_mana_colors = calculate_needed_mana_colors(old_pay_mana);
							tapped_for_mana_color = -1;
							int was_tapped = inst->state & STATE_TAPPED;
							event_activate_then_duplicate_into_stack(player, card, EVENT_ACTIVATE, 1 - player, -1);

							needed_mana_colors = 0;

							if (cancel == 1)
							  {
								cancel = 0;
								obliterate_top_card_of_stack();
							  }
							else
							  {
								land_can_be_played &= ~LCBP_CHARGING_MANA;

								if (!was_tapped && (inst->state & STATE_TAPPED))
								  dispatch_event(player, card, EVENT_TAP_CARD);// Needs to either dispatch EVENT_PLAY_ABILITY instead, or also dispatch EVENT_TAPPED_TO_PLAY_ABILITY
								play_sound_effect(WAV_TAP);
								resolve_top_card_on_stack();

								land_can_be_played |= LCBP_CHARGING_MANA;

								TENTATIVE_reassess_all_cards();
							  }
						  }

						charge_mana_addr_of_pre_mana_pool = old_charge_mana_addr_of_pre_mana_pool;
						charge_mana_addr_of_x_value = old_charge_mana_addr_of_x_value;
						charge_mana_addr_of_pay_mana_xbugrw = old_charge_mana_addr_of_pay_mana_xbugrw;
						charge_mana_addr_of_max_x_value = old_charge_mana_addr_of_max_x_value;

						x_value = old_x_value;
						max_x_value = old_max_x_value;

						for (i = 0; i < 8; ++i)
						  pay_mana_xbugrwaU[i] = old_pay_mana[i];

						for (i = 0; i < 8; ++i)
						  mana_pool[player][i] -= old_mana_pool[i];

						amt_of_nonartifact_mana = 0;
						APPLY_MANA_IN_POOL_TO_COSTS(&total_paid);

						for (i = 0; i < 8; ++i)
						  mana_pool[player][i] += old_mana_pool[i];

						copy_mana_pool_to_display();
					  }
				  }
			  }
			else if (EXE_DWORD(0x60a564) == -2)
			  {
				if (WILDGUESS_whose_phase_to_run_to == -1 && WILDGUESS_phase_to_run_to == -1)
				  {
					if (card == -1)
					  cancel = 1;
					break;
				  }
				else if (!any_nonx_mana)
				  break;
			  }
			else if (EXE_DWORD(0x60a564) == -3 && EXE_DWORD(0x60a434) == player && TENTATIVE_color_clicked_in_mana_summary_pane != -1)
			  {
				if (mana_pool[player][TENTATIVE_color_clicked_in_mana_summary_pane] > 0)
				  {
					// Apply directly
					if (TENTATIVE_color_clicked_in_mana_summary_pane != COLOR_ARTIFACT || pay_mana_xbugrwaU[COLOR_ARTIFACT])
					  {
						int col_to_apply_to = -1;
						if (pay_mana_xbugrwaU[TENTATIVE_color_clicked_in_mana_summary_pane])
						  col_to_apply_to = TENTATIVE_color_clicked_in_mana_summary_pane;
						else if (pay_mana_xbugrwaU[COLOR_ARTIFACT])
						  col_to_apply_to = COLOR_ARTIFACT;
						else if (pay_mana_xbugrwaU[COLOR_COLORLESS])
						  col_to_apply_to = COLOR_COLORLESS;

						if (col_to_apply_to != -1)
						  {
							apply_mana_to_cost(pay_mana_xbugrwaU, col_to_apply_to,
											   amt_to_apply_to_cost(pay_mana_xbugrwaU, col_to_apply_to, mana_pool[player],
																	TENTATIVE_color_clicked_in_mana_summary_pane, ldoubleclicked, max_x_value, x_value),
											   &x_value, max_x_value, player, TENTATIVE_color_clicked_in_mana_summary_pane,
											   amtspaid, &total_paid);
							copy_mana_pool_to_display();
							continue;
						  }
					  }

					// Apply via raw_mana_may_be_spent_as_color[][]
					{
					  color_test_t colors = 0;
					  int n;
					  for (n = 0; n < 10 && raw_mana_may_be_spent_as_color[player][n] != -1; ++n)
						if (TENTATIVE_color_clicked_in_mana_summary_pane == LOWORD(raw_mana_may_be_spent_as_color[player][n]))
						  colors |= 1 << BYTE3(raw_mana_may_be_spent_as_color[player][n]);

					  color_test_t paymask = 0;
					  for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
						if (pay_mana_xbugrwaU[c])
						  paymask |= 1 << c;
					  colors &= paymask;

					  if (colors)
						{
						  int num_colors = num_bits_set(colors);

						  color_t col_to_apply_to;
						  if (num_colors == 1)
							col_to_apply_to = single_color_test_bit_to_color_t(colors);
						  else
							{
							  load_text(0, "PROMPT_GRABMANA_DUALUSE");
							  col_to_apply_to = choose_a_color_exe(player, text_lines[0], 1, TENTATIVE_color_clicked_in_mana_summary_pane, colors);
							}

						  apply_mana_to_cost(pay_mana_xbugrwaU, col_to_apply_to,
											 amt_to_apply_to_cost(pay_mana_xbugrwaU, col_to_apply_to, mana_pool[player],
																  TENTATIVE_color_clicked_in_mana_summary_pane, ldoubleclicked, max_x_value, x_value),
											 &x_value, max_x_value, player, TENTATIVE_color_clicked_in_mana_summary_pane,
											 amtspaid, &total_paid);
						  copy_mana_pool_to_display();
						}
					}
				  }
			  }
		  }

	  if (!(old_land_can_be_played & LCBP_CHARGING_MANA))
		land_can_be_played &= ~LCBP_CHARGING_MANA;
    }

  if (cancel == 1)
    {
      TENTATIVE_update_mana_spent(amtspaid);
      color_t c;
      for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c)
		{
		  produce_mana(player, c, amtspaid[c]);
		  amtspaid[c] = 0;
		}
      total_paid = 0;
      x_value = 0;
    }

  int c;
  for (c = 0; c < 7; ++c)
	{
	  mana_paid[c] = amtspaid[c];
	  pay_mana_xbugrwaU[c] = 0;
	}
  pay_mana_xbugrwaU[7] = 0;

  max_x_value = -1;
  unknown62BCEC = -1;

  affected_card_controller = old_affected_card_controller;
  affected_card = old_affected_card;

  return total_paid;
}

int autotap_mana_source(int player, int card)
{
  card_instance_t* inst = get_card_instance(player, card);
  if (inst->state & STATE_TAPPED)
    return 0;

  if ((inst->state & STATE_SUMMON_SICK) && is_what(player, card, TYPE_CREATURE))
	return 0;

  int old_land_can_be_played = land_can_be_played;
  land_can_be_played |= LCBP_CHARGING_MANA;

  int old_pay_mana[8], old_x_value, old_max_x_value;

  int* old_charge_mana_addr_of_x_value = charge_mana_addr_of_x_value;
  charge_mana_addr_of_x_value = &old_x_value;
  int (*old_charge_mana_addr_of_pay_mana_xbugrw)[8] = charge_mana_addr_of_pay_mana_xbugrw;
  charge_mana_addr_of_pay_mana_xbugrw = &old_pay_mana;
  int* old_charge_mana_addr_of_max_x_value = charge_mana_addr_of_max_x_value;
  charge_mana_addr_of_max_x_value = &old_max_x_value;

  old_x_value = x_value;                              // inject before this
  x_value = 0;
  old_max_x_value = max_x_value;
  max_x_value = -1;

  int c;
  for (c = 0; c < 8; ++c)
	{
	  old_pay_mana[c] = pay_mana_xbugrwaU[c];
	  pay_mana_xbugrwaU[c] = 0;
	}

  put_card_or_activation_onto_stack(player, card, EVENT_RESOLVE_ACTIVATION, player, 0);
  needed_mana_colors = calculate_needed_mana_colors(old_pay_mana);
  tapped_for_mana_color = -1;
  int was_tapped = inst->state & STATE_TAPPED;

  event_activate_then_duplicate_into_stack(player, card, EVENT_ACTIVATE, 1 - player, -1);

  needed_mana_colors = 0;

  int rval;
  if (cancel == 1)
	{
	  cancel = 0;
	  obliterate_top_card_of_stack();
	  rval = 0;
	}
  else
	{
	  land_can_be_played &= ~LCBP_CHARGING_MANA;

	  if (!was_tapped && (inst->state & STATE_TAPPED))
		dispatch_event(player, card, EVENT_TAP_CARD);
	  play_sound_effect(WAV_TAP);
	  resolve_top_card_on_stack();

	  rval = 1;
	}

  charge_mana_addr_of_x_value = old_charge_mana_addr_of_x_value;
  charge_mana_addr_of_pay_mana_xbugrw = old_charge_mana_addr_of_pay_mana_xbugrw;
  charge_mana_addr_of_max_x_value = old_charge_mana_addr_of_max_x_value;

  x_value = old_x_value;
  max_x_value = old_max_x_value;

  for (c = 0; c < 8; ++c)
	pay_mana_xbugrwaU[c] = old_pay_mana[c];

  land_can_be_played = old_land_can_be_played;

  return rval;
}
#undef APPLY_MANA_IN_POOL_TO_COSTS

#undef amt_to_apply_to_cost
#undef apply_mana_in_pool_to_colorless_cost
#undef apply_mana_in_pool_to_cost
#undef apply_mana_in_pool_to_x_cost
#undef apply_mana_to_cost

#undef calculate_needed_mana_colors
#undef copy_mana_pool_to_display
#undef TENTATIVE_reassess_all_cards
#undef resolve_top_card_on_stack
#undef single_color_test_bit_to_color_t
#undef TENTATIVE_update_mana_spent

#undef EXE_2D_ARR

#undef TENTATIVE_color_clicked_in_mana_summary_pane
#undef global_all_purpose_buffer
#undef WILDGUESS_phase_to_run_to
#undef raw_mana_may_be_spent_as_color
#undef WILDGUESS_whose_phase_to_run_to

void human_assign_blockers(int player)
{
  // 0x434960

  if (ai_is_speculating == 1)
	return;

  // Begin additions
  int who_chooses;
  if (event_flags & EF_ATTACKER_CHOOSES_BLOCKERS)
	{
	  who_chooses = player;
	  if (current_turn == AI)	// Nothing blocks
		return;
	}
  else
	who_chooses = 1-player;
  // End additions

  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()

  // Begin removals
#if 0
  // This seems to be the remains of primitive AI handling; the values it computes aren't used.
  int indices[16], powers[16], highest_power = 0, pos = 0, c;
  for (c = 0; active_cards_count[player] > c; ++c)
	{
	  card_instance_t* instance = get_card_instance(player, c);
	  if (instance->internal_card_id != -1 && (instance->state & STATE_ATTACKING))
		{
		  int pow = get_abilities(player, c, EVENT_POWER, -1);
		  indices[pos] = c;
		  powers[pos] = pow;
		  ++pos;
		  if (highest_power < pow)
			highest_power = pow;
		}
	}
#endif
  // End removals

  target_definition_t td_choose_blocker;
  base_target_definition(1-player, 0, &td_choose_blocker, TARGET_TYPE_NONCREATURE_CAN_BLOCK | TYPE_CREATURE);
  td_choose_blocker.who_chooses = who_chooses;
  td_choose_blocker.allowed_controller = 1-player;
  td_choose_blocker.preferred_controller = 1-player;
  td_choose_blocker.zone = TARGET_ZONE_0x2000 | TARGET_ZONE_IN_PLAY;
  td_choose_blocker.special = TARGET_SPECIAL_ALLOW_MULTIBLOCKER;
  td_choose_blocker.illegal_state = TARGET_STATE_BLOCKING | TARGET_STATE_TAPPED;
  td_choose_blocker.allow_cancel = 2;	// I suspect this is what makes the "Done" button.

  target_definition_t td_block_which_attacker;
  base_target_definition(player, 0, &td_block_which_attacker, TYPE_CREATURE);
  td_block_which_attacker.who_chooses = who_chooses;
  td_block_which_attacker.allowed_controller = player;
  td_block_which_attacker.preferred_controller = player;
  td_block_which_attacker.required_state = TARGET_STATE_ATTACKING;

  target_t blocker, blocked;

  while (EXE_FN(int, 0x434E70, int)(1-player))	// any_can_block()
	{
	  load_text(0, "PROMPT_CHOOSEBLOCKERS");
	  if (!select_target(1-player, -1000, &td_choose_blocker, text_lines[0], &blocker))
		return;

	  forbid_attack = 0;
	  if (event_flags & EA_PAID_BLOCK)
		{
		  push_affected_card_stack();

		  trigger_cause_controller = blocker.player;
		  trigger_cause = blocker.card;
		  dispatch_trigger(1-player, TRIGGER_PAY_TO_BLOCK, EXE_STR(0x790248)/*PROMPT_TURNSEQUENCE[0]*/, 1);
		  pop_affected_card_stack();
		}

	  if (!forbid_attack
		  && select_target(player, -1000, &td_block_which_attacker, text_lines[1], &blocked))
		{
		  // 0x4350e0 clears blocking, checks legality, and then, if it was illegal, restores the previous values
		  if (EXE_FN(int, 0x4350E0, int, int, int, int)(blocker.player, blocker.card, blocked.player, blocked.card))	// try_block()
			{
			  int band = get_card_instance(blocked.player, blocked.card)->blocking;
			  if (band == 255)
				band = blocked.card;

			  card_instance_t* instance = get_card_instance(blocker.player, blocker.card);
			  instance->blocking = band;
			  instance->state |= STATE_BLOCKING;
			  // Begin removals
			  // if (band_before_being_set_to_blocked.card != 255)	// Suppresse the block sound when blocking a band
			  // End removals
			  play_sound_effect(WAV_BLOCK2);

			  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()

			  if (event_flags & EA_SELECT_BLOCK)
				dispatch_trigger2(current_turn, TRIGGER_BLOCKER_CHOSEN, EXE_STR(0x790074)/*PROMPT_BLOCKERSELECTION[0]*/, 0, blocker.player, blocker.card);
			}
		  else // if (ai_is_speculating != 1)	// Redundant, already checked above
			{
			  load_text(0, "PROMPT_CHOOSEBLOCKERS");
			  set_centerwindow_txt(text_lines[2]);
			  EXE_STDCALL_FN(void, 0x4D5D32, int)(2000);	// Sleep(2000)
			  set_centerwindow_txt("");
			}
		}
	}
}

void allow_response_to_activation(int player, int card)
{
  // originally part of 0x4346e0

  const char* name = EXE_FN(const char*, 0x439690, int, int)(player, card);	// get_displayable_cardname_from_player_card()
  sprintf((char*)(0x60A690)/*global_all_purpose_buffer*/, EXE_STR(0x786264)/*PROMPT_CHECKFEPHASE[3]*/, name);

  int a1 = (EXE_DWORD(0x60A54C) && (player == 0 || (trace_mode & 2))) ? -1 : -2;
  EXE_FN(int, 0x436A20, int, int, const char*, int)(a1, current_phase, EXE_STR(0x60A690), EVENT_ACTIVATE);	// allow_response()
}

int finalize_activation(int player, int card)
{
  // 0x4346e0

  if (cancel == 1)
	return 1;

  card_instance_t* instance = get_card_instance(player, card);
  int iid = get_card_instance(player, card)->internal_card_id;
  // Begin additions
  if (iid == -1)
	iid = instance->backup_internal_card_id;
  // End additions

  int tapping_for_mana = (cards_data[iid].extra_ability & EA_MANA_SOURCE) && tapped_for_mana_color != -1;

  if (!tapping_for_mana && !cant_be_responded_to)
	allow_response_to_activation(player, card);

  EXE_FN(void, 0x436740, void)();	// resolve_top_card_on_stack()

  dispatch_trigger2(current_turn, TRIGGER_TAP_CARD, EXE_STR(0x78fba8), 0, player, card);	// PROMPT_SPECIALFEPHASE[7]

  return 1;
}

extern target_t totally_bletcherous_hack_dont_reset_x_value;
void put_card_or_activation_onto_stack(int player, int card, event_t event, int a4, int a5)
{
  // 0x436550

  if (stack_size >= 32)
	return;

  card_instance_t* instance = get_card_instance(player, card);

  stack_data[stack_size].internal_card_id = instance->internal_card_id;
  stack_data[stack_size].generating_event = event;
  stack_data[stack_size].unknown_sdt = a4;

  int stack_card;
  if (event == EVENT_RESOLVE_SPELL || event == EVENT_RESOLVE_TRIGGER || instance->internal_card_id <= 4)	// a basic land
	stack_card = card;
  else
	{
	  stack_card = add_card_to_hand(player, activation_card);
	  if (stack_card == -1)
		return;
	  card_instance_t* stack_inst = get_card_instance(player, stack_card);
	  int orig_timestamp = stack_inst->timestamp;
	  memcpy(stack_inst, instance, sizeof(card_instance_t));
	  stack_inst->internal_card_id = activation_card;
	  stack_inst->unknown0x14 = 0;
	  stack_inst->kill_code = 0;
	  if (instance->internal_card_id != -1)
		stack_inst->original_internal_card_id = instance->internal_card_id;
	  // Begin additions
	  else
		stack_inst->original_internal_card_id = instance->backup_internal_card_id;
	  // End additions
	  stack_inst->state |= STATE_IN_PLAY;		--hand_count[player];
	  stack_inst->parent_controller = player;
	  stack_inst->parent_card = card;
	  stack_inst->timestamp = orig_timestamp;

	  // Begin additions
	  stack_inst->token_status &= ~STATUS_DYING;	// so kill_card() will reap it, even if the original card sacrificed itself as an activation card
	  // End additions
	}

  stack_cards[stack_size].player = player;
  stack_cards[stack_size].card = stack_card;

  stack_damage_targets[stack_size].player = instance->damage_target_player;
  stack_damage_targets[stack_size].card = instance->damage_target_card;

  if (trigger_condition == (trigger_t)(-1))
	stack_phase[stack_size] = current_phase;
  else
	stack_phase[stack_size] = trigger_condition;

  if (ai_is_speculating != 1)
	EXE_DWORD_PTR(0x628674)[stack_size] = a5;

  ++stack_size;
  stack_cards[stack_size].player = -1;

  if (totally_bletcherous_hack_dont_reset_x_value.player >= 0 && event == EVENT_RESOLVE_SPELL
	  && totally_bletcherous_hack_dont_reset_x_value.player == player
	  && totally_bletcherous_hack_dont_reset_x_value.card == card)
	{
	  totally_bletcherous_hack_dont_reset_x_value.player = -1;
	  EXE_DWORD(0x78676C) = 1;
	}
}

void recopy_card_onto_stack(int a1)
{
  // 0x436450

  int pos = stack_size - 1;

  card_instance_t* stack_inst = get_card_instance(stack_cards[pos].player, stack_cards[pos].card);
  if (stack_inst->internal_card_id == activation_card)
	{
	  int pc = stack_inst->parent_card;
	  int pp = stack_inst->parent_controller;
	  int ts = stack_inst->timestamp;

	  card_instance_t* parent_inst = get_card_instance(stack_inst->parent_controller, stack_inst->parent_card);

	  if (parent_inst->state & STATE_DONT_RECOPY_ONTO_STACK)
		parent_inst->state &= ~STATE_DONT_RECOPY_ONTO_STACK;
	  else
		{
		  memcpy(stack_inst, parent_inst, sizeof(card_instance_t));

		  stack_inst->internal_card_id = activation_card;
		  stack_inst->unknown0x14 = 0;
		  stack_inst->kill_code = 0;
		  stack_inst->state |= STATE_IN_PLAY;
		  stack_inst->parent_controller = pp;
		  stack_inst->parent_card = pc;
		  stack_inst->timestamp = ts;

		  if (parent_inst->internal_card_id != -1)
			stack_inst->original_internal_card_id = parent_inst->internal_card_id;
		  // Begin additions
		  else
			stack_inst->original_internal_card_id = parent_inst->backup_internal_card_id;
		  // End additions

		  if ((int)stack_inst->original_internal_card_id < damage_card
			  || (int)stack_inst->original_internal_card_id >= damage_card + 47)
			{
			  stack_inst->display_pic_csv_id = cards_data[stack_inst->original_internal_card_id].id;
			  // Begin removals
			  // stack_inst->display_pic_num = 0
			  // End removals
			  // Begin additions
			  stack_inst->display_pic_num = get_card_image_number(stack_inst->display_pic_csv_id, pp, pc);
			  // End additions
			}
		}

	  // Begin additions
	  if (stack_inst->internal_card_id == activation_card)
		stack_inst->token_status &= ~STATUS_DYING;	// so kill_card() will reap it, even if the original card sacrificed itself as an activation card
	  // End additions
	}

  if (ai_is_speculating != 1)
	EXE_DWORD_PTR(0x628674)[pos] = a1;

  stack_damage_targets[pos].player = stack_inst->damage_target_player;
  stack_damage_targets[pos].card = stack_inst->damage_target_card;
}

static void comes_into_play_trigger(int player, int card)
{
  // 0x401bb0.  Not moved out of exe, since it has a non-standard calling convention and is only called from put_card_on_stack3().

  push_affected_card_stack();
  get_card_instance(player, card)->regen_status |= KEYWORD_RECALC_ALL;
  dispatch_trigger2(current_turn, TRIGGER_COMES_INTO_PLAY, EXE_STR(0x787108), 0, player, card);	//PROMPT_SPECIALFEPHASE[0]
  pop_affected_card_stack();
}
int put_card_on_stack3(int player, int card)
{
  // 0x433e10

  card_instance_t* instance = get_card_instance(player, card);

  int iid = instance->internal_card_id;
  if (iid == -1)
	{
	  obliterate_top_card_of_stack();
	  land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;	// Addition
	  return 0;
	}

  dispatch_trigger2(current_turn, TRIGGER_SPELL_CAST, EXE_STR(0x715a04), 0, player, card);	//PROMPT_SPECIALFEPHASE[6]

  // Begin additions - allows TRIGGER_SPELL_CAST to safely counter a spell or change its iid
  land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;	// moved from lower, just after STATE_IN_PLAY is set
  iid = instance->internal_card_id;
  if (iid == -1)
	{
	  obliterate_top_card_of_stack();
	  return 0;
	}
  // End additions

  card_data_t* cd = get_card_data(player, card);

  if (!is_what(player, card, TYPE_LAND)
	// && !(is_what(player, card, TYPE_INTERRUPT) && (cd->extra_ability & EA_MANA_SOURCE)	// Removed by patch_make_manasource_interrupts_interruptible.pl
	  )
	{
	  const char* name = EXE_FN(const char*, 0x439690, int, int)(player, card);	// get_displayable_cardname_from_player_card()
	  sprintf((char*)(0x60A690)/*global_all_purpose_buffer*/, EXE_STR(0x737bc0)/*PROMPT_CHECKFEPHASE[2]*/, name);
	  EXE_FN(int, 0x436A20, int, int, const char*, int)(-2, current_phase, EXE_STR(0x60A690)/*global_all_purpose_buffer*/, EVENT_CAST_SPELL);	// allow_response()
	}
  instance->state &= ~STATE_INVISIBLE;
  instance->state |= STATE_IN_PLAY;

  // Begin removals
  // if (instance->internal_card_id != iid)
  //   {
  //     obliterate_top_card_of_stack();
  //     return 0;
  //   }

  uint8_t type = cd->type;
  types_of_cards_on_bf[player] |= type;
  if (ai_is_speculating != 1)
	{
	  if (type & TYPE_CREATURE)
		play_sound_effect(WAV_SUMMON);
	  if (type & TYPE_ARTIFACT)
		play_sound_effect(WAV_ARTIFACT);
	  if (type & TYPE_ENCHANTMENT)
		{
		  if (cd->cc[2] == 9)	// a Planeswalker
			play_sound_effect(WAV_PLANESWALKER);
		  else
			play_sound_effect(WAV_ENCHANT);
		}
	  if (type & TYPE_INSTANT)
		play_sound_effect(WAV_INSTANT);
	  if (type & TYPE_INTERRUPT)
		play_sound_effect(WAV_INTERUPT);
	  if (type & TYPE_SORCERY)
		play_sound_effect(WAV_SORCERY);
	}

  EXE_FN(void, 0x436740, void)();	// resolve_top_card_on_stack()

  if (!(instance->internal_card_id == -1 && is_what(-1, iid, TYPE_PERMANENT)))	// Either an Aura whose target became invalid, or something like Mox Diamond
	comes_into_play_trigger(player, card);

  EXE_FN(void, 0x477070, void)();	// resolve_damage_cards_and_prevent_damage()
  event_flags &= ~EF_RERUN_DAMAGE_PREVENTION;
  EXE_DWORD(0x4EF1B4) = 0x477B30;	// = &kill_card_guts()
  EXE_FN(int, 0x477a90, void)();	// regenerate_or_graveyard_triggers()
  EXE_FN(void, 0x477070, void)();	// resolve_damage_cards_and_prevent_damage() (again!)
  EXE_DWORD(0x4EF19C) = -1;			// card_on_stack_iid

  if (cd->type & TYPE_LAND)
	++lands_played;

  if (cancel == 1)	// Essentially never; will be cleared within both regenerate_or_graveyard_triggers() and resolve_damage_cards_and_prevent_damage()
	{
	  if (ai_is_speculating != 1)
		{
		  load_text(0, "PROMPT_FIZZLE");
		  EXE_FN(int, 0x471D70, const char*)(EXE_STR(0x60A690));	// TENTATIVE_centerwindow_message(global_all_purpose_buffer)
		  EXE_STDCALL_FN(void, 0x4D5D32, int)(2000);	// Sleep(2000)
		  EXE_FN(int, 0x471D70, const char*)("");	// TENTATIVE_centerwindow_message("")
		}
	  EXE_DWORD(0x7A372C) = 1;
	  cancel = 0;
	  return 0;
	}
  else
	{
	  cancel = 0;
	  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()
	  return 1;
	}
}

extern int event_rval;	// set only when calling our event dispatching functions directly
int is_a_tappable_mana_source(int player, int card)
{
  // 0x43a280

#if 0	// Exe version
  card_instance_t* instance;
  card_data_t* cd;
  if (!(instance = in_play(player, card)))
	return 0;
  if (!((cd = &cards_data[instance->internal_card_id])->extra_ability & EA_MANA_SOURCE))
	return 0;
  if (instance->state & STATE_TAPPED)
	return 0;
  if ((instance->state & STATE_SUMMON_SICK) && (cd->type & TYPE_CREATURE))
	return 0;
  return 1;
#else
  // Begin additions
  card_instance_t* instance = in_play(player, card);
  if (instance && (cards_data[instance->internal_card_id].extra_ability & EA_MANA_SOURCE))
	{
	  dispatch_event_with_attacker_to_one_card(player, card, EVENT_CAN_ACTIVATE, 1-player, -1);
	  return event_rval;
	}
  else
	return 0;
  // End additions
#endif
}

void event_activate_then_duplicate_into_stack(int player, int card, int event, int new_attacking_card_controller, int new_attacking_card);
// Interface for exe calls.
void tap_card_for_mana(int player, int card)
{
#if 0	// Exe version
  needed_mana_colors = COLOR_TEST_ANY_COLORED;
  tapped_for_mana_color = -1;
  dispatch_event_with_attacker_to_one_card(player, card, EVENT_ACTIVATE, 1-player, -1);
  if (cancel == 1)
	cancel = 0;
  else
	{
	  if (get_card_instance(player, card)->state & STATE_TAPPED)
		dispatch_event(player, card, EVENT_TAP_CARD);
	  resolve_damage_cards_and_prevent_damage();
	  regenerate_or_graveyard_triggers();
	}
  needed_mana_colors = 0;
#else
  force_activation_for_mana(player, card, COLOR_TEST_COLORLESS);
#endif
}

// Note that this doesn't check EVENT_CAN_ACTIVATE itself, or whether {player,card} has EA_MANA_SOURCE.
void force_activation_for_mana(int player, int card, color_test_t colors)
{
  put_card_or_activation_onto_stack(player, card, EVENT_RESOLVE_ACTIVATION, player, 0);
  needed_mana_colors = colors;
  tapped_for_mana_color = -1;
  int was_tapped = is_tapped(player, card);
  event_activate_then_duplicate_into_stack(player, card, EVENT_ACTIVATE, 1-player, -1);	// Yeah, the one that says "do not call directly".  I'm calling it directly because I'm duplicating the exe function it fixes.
  needed_mana_colors = 0;
  if (cancel == 1)
	{
	  cancel = 0;
	  obliterate_top_card_of_stack();
	}
  else
	{
	  if (was_tapped || !is_tapped(player, card))
		dispatch_event(player, card, EVENT_PLAY_ABILITY);
	  else
		{
		  dispatch_event(player, card, EVENT_TAP_CARD);
		  dispatch_event(player, card, EVENT_TAPPED_TO_PLAY_ABILITY);
		}
	  play_sound_effect(WAV_TAP);
	  EXE_FN(void, 0x436740, void)();	// resolve_top_card_on_stack()
	  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()
	}
}

int card_instances_should_be_displayed_identically(card_instance_t* inst1, card_instance_t* inst2)
{
  // 0x48c9d0

  // (Changes in ability icons and unknown0x70 are detected separately.)
  return (inst1 == inst2
		  || !(inst1->internal_card_id != inst2->internal_card_id
			   || ((inst1->state ^ inst2->state) & (STATE_SUMMON_SICK|STATE_TAPPED|STATE_OWNED_BY_OPPONENT|STATE_TARGETTED|STATE_CANNOT_TARGET|STATE_OUBLIETTED
													|STATE_IS_TRIGGERING))	// added
			   || inst1->damage_on_card != inst2->damage_on_card
			   || inst1->power != inst2->power
			   || inst1->toughness != inst2->toughness
			   || inst1->mana_color != inst2->mana_color
			   || inst1->color != inst2->color
			   || inst1->blocking != inst2->blocking
			   || inst1->info_slot != inst2->info_slot
			   || inst1->special_counters != inst2->special_counters
			   || inst1->counters2 != inst2->counters2
			   || inst1->counters3 != inst2->counters3
			   || inst1->counters4 != inst2->counters4
			   || inst1->unknown0x11a != inst2->unknown0x11a
			   || inst1->counters != inst2->counters
			   || inst1->counters5 != inst2->counters5
			   || inst1->unknown0x122 != inst2->unknown0x122
			   || ((inst1->token_status ^ inst2->token_status) & STATUS_RED_BORDER)
			   || inst1->display_pic_csv_id != inst2->display_pic_csv_id
			   || inst1->display_pic_num != inst2->display_pic_num
			   || inst1->untap_status != inst2->untap_status
			   || inst1->kill_code != inst2->kill_code
			   || inst1->counters_m1m1 != inst2->counters_m1m1	// added
			   || inst1->targets[18].player != inst2->targets[18].player	// added; counter types
			   || inst1->targets[18].card != inst2->targets[18].card	// added; counter types
			   ));
}

int determine_start_duel_winner(void)
{
  // Replacement for the last part of start_duel().
  // This isn't actually reached most of the time - I think it never is, but that's difficult to prove.  Usually, lose_the_game() calls ExitThread() instead.
  int lost0 = life[0] <= 0 || POISON_COUNTERS(0) >= 10;
  int lost1 = life[1] <= 0 || POISON_COUNTERS(1) >= 10;

  if (!lost0 && lost1)
    return 1;
  else if (!lost1 && lost0)
    return 0;
  else
    return -1;
}

int is_anyone_dead(void)
{
  if (life[0] <= 0 || life[1] <= 0
	  || POISON_COUNTERS(0) >= 10 || POISON_COUNTERS(1) >= 10)
	{
	  if (ai_is_speculating != 1)
		{
		  EXE_FN(void, 0x472260, void)();	// TENTATIVE_reassess_all_cards()
		  return 1;
		}
	  else
		{
		  if (POISON_COUNTERS(0) >= 10)
			life[0] = -99;

		  if (POISON_COUNTERS(1) >= 10)
			life[1] = -99;
		}
	}
  return 0;
}

// Returns the most common color either in Deck[] (if plr == -1), or in initial_library[plr].
int sub_495BE0(int plr)
{
  int num_per_col[COLOR_WHITE + 1] = {0};
  int i;

  if (plr == -1)
	{
	  for (i = 0; i < 500; ++i)
		if (Deck[i] != -1 && !(Deck[i] & 0x88000))	// 0x8000 = offered as ante; 0x80000 = not in current deck (moved from 0x4000)
		  {
			color_test_t cols = cards_data[Deck[i] & 0x7FFF].color;
			if (cols & COLOR_TEST_BLACK)
			  ++num_per_col[COLOR_BLACK];
			if (cols & COLOR_TEST_WHITE)
			  ++num_per_col[COLOR_WHITE];
			if (cols & COLOR_TEST_GREEN)
			  ++num_per_col[COLOR_GREEN];
			if (cols & COLOR_TEST_RED)
			  ++num_per_col[COLOR_RED];
			if (cols & COLOR_TEST_BLUE)
			  ++num_per_col[COLOR_BLUE];
		  }
	}
  else
	{
	  for (i = 0; i < 200; ++i)
		{
		  csvid_t csvid = initial_library[plr][i].csvid;
		  int num = initial_library[plr][i].numcards;
		  if (csvid != -1 && num > 0)
			{
			  color_test_t cols = cards_data[get_internal_card_id_from_csv_id(csvid)].color;
			  if (cols & COLOR_TEST_BLACK)
				num_per_col[COLOR_BLACK] += num;
			  if (cols & COLOR_TEST_WHITE)
				num_per_col[COLOR_WHITE] += num;
			  if (cols & COLOR_TEST_GREEN)
				num_per_col[COLOR_GREEN] += num;
			  if (cols & COLOR_TEST_RED)
				num_per_col[COLOR_RED] += num;
			  if (cols & COLOR_TEST_BLUE)
				num_per_col[COLOR_BLUE] += num;
			}
		}
	}

  // Not quite the same as the original, which checks in the order black, white, green, red, blue, for no apparent reason.  The order matters for ties.
  int highest_idx = 0, highest_num = -1;
  for (i = COLOR_BLACK; i <= COLOR_WHITE; ++i)
	if (num_per_col[i] > highest_num)
	  {
		highest_idx = i;
		highest_num = num_per_col[i];
	  }

  return highest_idx;
}
