#include "manalink.h"
#include <math.h>

// Functions
static int skulkin(int player, int card, event_t event, int cless, int target_color, int pwr, int tou, int key, int s_key){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_color = get_sleighted_color_test(player, card, target_color);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, pwr, tou, key, s_key);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(cless), 0, &td, "TARGET_CREATURE");
}

static int mimic(int player, int card, event_t event, int c1, int c2, int pwr, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player == 66 && ! is_humiliated(player, card) ){
		set_pt_and_abilities(player, card, event, pwr, tou, key, s_key);
	}

	if( instance->targets[1].player != 66 && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card)
	  ){
		int trig = 0;
		if( trigger_cause_controller == player ){
			if( (get_color(trigger_cause_controller, trigger_cause) & c1) && (get_color(trigger_cause_controller, trigger_cause) & c2) ){
				trig = 1;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->targets[1].player = 66;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return hybrid(player, card, event);
}

static int hatchling(int player, int card, event_t event, int c1, int c2){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 4);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && count_counters(player, card, COUNTER_M1_M1) > 0 &&
		player == reason_for_trigger_controller && trigger_cause_controller == player && ! is_humiliated(player, card)
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			if( get_color(trigger_cause_controller, trigger_cause) & get_sleighted_color_test(player, card, c1) ){
				trig++;
			}
			if( get_color(trigger_cause_controller, trigger_cause) & get_sleighted_color_test(player, card, c2) ){
				trig++;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				remove_counters(player, card, COUNTER_M1_M1, trig);
			}
		}
	}

	return hybrid(player, card, event);
}

static int retrace(int player, int card, event_t event, int id){

	/* 702.80. Retrace
	 *
	 * 702.80a Retrace appears on some instants and sorceries. It represents a static ability that functions while the
	 * card with retrace is in a player's graveyard. "Retrace" means "You may cast this card from your graveyard by
	 * discarding a land card as an additional cost to cast it." Casting a spell using its retrace ability follows the
	 * rules for paying additional costs in rules 601.2b and 601.2f-h. */
	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_to_cast_id(player, event, id)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;

		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			if( can_legally_play_iid_now(player, get_card_instance(player, card)->internal_card_id, event) ){
				return GA_PLAYABLE_FROM_GRAVE;
			}
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_from_id(player, -1, event, id);
		if( spell_fizzled != 1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}
	return 0;
}

static int count_chroma(int player, int card, int clr1, int clr2, int must_be_permanent)
{
  card_data_t* cd;
  if (player != -1)
	{
	  /* Rule 202.1b Some objects have no mana cost. This normally includes all land cards, any other cards that have no mana symbols where their mana cost
	   * would appear, tokens (unless the effect that creates them specifies otherwise), and nontraditional _Magic_ cards. Having no mana cost represents an
	   * unpayable cost (see rule 117.6). Note that lands are played without paying any costs (see rule 305, "Lands"). */
	  if (is_token(player, card))
		return 0;

	  cd = get_card_data(player, card);
	}
  else
	cd = &cards_data[card];

  // Rules 202.1b again.  Lands are very common, so optimize for them.
  if (cd->type & TYPE_LAND)
	return 0;

  // Permanents only - in Manalink, instants and sorceries and such are on the battlefield while they resolve.
  if (!(cd->type & TYPE_PERMANENT) && must_be_permanent)
	return 0;

  card_ptr_t* cp = cards_ptr[cd->id];

  int result = 0;

  if (clr1 < COLOR_BLACK || clr1 > COLOR_WHITE)
	clr1 = 0;

  if (clr2 < COLOR_BLACK || clr2 > COLOR_WHITE)
	clr2 = 0;

  /* Checking card_ptr_t::req_black etc. and cards_data[].color can't easily distinguish between multicolor cards and hybrid cards when looking for more than
   * one color, even before considering the really weird cases like Bant Sureblade.  We can reliably tell that a card is not hybrid by checking if it has
   * "Modifies Mana Cost" set (it's stored as card_data_t::new_field & 1), but other things set that too.  We can't even reliably tell whether a card is
   * multicolored just from looking at card_data_t::color; consider, for example, Transguild Courier. */

  if (!(cd->new_field & 1))
	{
	  // req_black through req_white are correct and complete, so this is quick and easy way.

	  int clrs = (1<<clr1) | (1<<clr2);

	  if (clrs & COLOR_TEST_BLACK)
		result += cp->req_black;

	  if (clrs & COLOR_TEST_BLUE)
		result += cp->req_blue;

	  if (clrs & COLOR_TEST_GREEN)
		result += cp->req_green;

	  if (clrs & COLOR_TEST_RED)
		result += cp->req_red;

	  if (clrs & COLOR_TEST_WHITE)
		result += cp->req_white;

	  return result;
	}

  /* Currently-appearing mana symbols in mana costs are:
   *
   * |B |U |G |R |W - normal.
   * |0 through |16 - generic.
   * |PB |PU |PG |PR |PW - phyrexian.
   * |BR |BG |RG |RW |GW |GU |WU |WB |UB |UR - hybrid.
   * |2B |2U |2G |2R |2W - monocolor hybrid.
   * |/ - split card.  Doesn't appear on permanents.
   *
   * So we can count by looking for the corresponding letters B U G R W.
   *
   * Since hybrid mana symbols are only counted once for two-color devotion even if both halves match, we skip the character after a match - it's always either
   * a '|' for the start of a new mana symbol, or the second half of a hybrid mana symbol. */

  static const char* clrs = "\tBUGRW";

  const char* p;
  for (p = cp->mana_cost_text; *p; ++p)
	if (*p == clrs[clr1] || *p == clrs[clr2])
	  {
		++result;		// Matched
		++p;			// Skip the next character
		if (*p == 0)	// Original match was the last character in the string
		  break;
	  }

  return result;
}

//This count only colored mana symbols for permanents, for other uses, go fo "count_chroma".
int chroma(int player, int card, int clr1, int clr2){
	int result = 0;
	if( card != -1 ){
		result = count_chroma(player, card, clr1, clr2, 1);
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if(i==player || player == 2){
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && !is_what(i, count, TYPE_EFFECT) ){
							result+=count_chroma(i, count, clr1, clr2, 1);
						}
						count++;
				}
			}
		}
	}
	return result;
}

static void get_mana_from_text(test_definition_t *test, int cless, int black, int blue, int green, int red, int white){
	char buffer[100];
	int pos = scnprintf(buffer, 100, "Pay ");
	if( cless ){
		pos += scnprintf(buffer+pos, 100-pos, "%d", cless);
	}
	if( black ){
		int i;
		for(i=0; i<black; i++){
			pos += scnprintf(buffer+pos, 100-pos, "B");
		}
	}
	if( blue ){
		int i;
		for(i=0; i<blue; i++){
			pos += scnprintf(buffer+pos, 100-pos, "U");
		}
	}
	if( green ){
		int i;
		for(i=0; i<green; i++){
			pos += scnprintf(buffer+pos, 100-pos, "G");
		}
	}
	if( red ){
		int i;
		for(i=0; i<red; i++){
			pos += scnprintf(buffer+pos, 100-pos, "R");
		}
	}
	if( white ){
		int i;
		for(i=0; i<white; i++){
			pos += scnprintf(buffer+pos, 100-pos, "W");
		}
	}
	strcpy(test->message, buffer);
}

// Cards
int card_antler_skulkin(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &persist_granted,
											instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(2), 0, &td, "Select target white creature.");
}

int card_archon_of_justice(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_ashling_the_extinguisher(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				card_instance_t *instance = get_card_instance(player, card);
				instance->number_of_targets = 1;
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

int card_balefire_liege(int player, int card, event_t event){

	/* Balefire Liege	|2|RW|RW|RW
	 * Creature - Spirit Horror 2/4
	 * Other |Sred creatures you control get +1/+1.
	 * Other |Swhite creatures you control get +1/+1.
	 * Whenever you cast a |Sred spell, ~ deals 3 damage to target player.
	 * Whenever you cast a |Swhite spell, you gain 3 life. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller &&
		!is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && !is_humiliated(player, card)
	  ){
		int col = get_color(trigger_cause_controller, trigger_cause);
		int white = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		int red = get_sleighted_color_test(player, card, COLOR_TEST_RED);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if (event == EVENT_TRIGGER &&
			((col & white) || ((col & red) && can_target(&td)))
		   ){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		if (event == EVENT_RESOLVE_TRIGGER){
			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			if (col & white){
				gain_life(player, 3);
			}
			if ((col & COLOR_TEST_RED) && can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
				damage_player(instance->targets[0].player, 3, player, card);
			}
		}
	}

	return liege(player, card, event, COLOR_TEST_WHITE, COLOR_TEST_RED);
}

int card_battlegate_mimic(int player, int card, event_t event){
	return mimic(player, card, event, COLOR_TEST_WHITE, COLOR_TEST_RED, 4, 2, KEYWORD_FIRST_STRIKE, 0);
}

int card_batwing_brume(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_WHITE, 1) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int c1 = get_updated_casting_cost(player, card, -1, event, 1);
			test_definition_t test;
			char mana_text[3][100];
			get_mana_from_text(&test, MANACOST_XW(c1, 1));
			strcpy(mana_text[0], test.message);
			get_mana_from_text(&test, MANACOST_XB(c1, 1));
			strcpy(mana_text[1], test.message);
			int can_play_full_effect = 0;
			if( c1 > 0 ){
				if( c1 > 1 ){
					get_mana_from_text(&test, MANACOST_XBW(c1-1, 1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_XBW(c1-1, 1, 1));
				}
				else{
					get_mana_from_text(&test, MANACOST_BW(1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_BW(1, 1));
				}
			}
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							mana_text[0], has_mana_multi(player, MANACOST_XW(c1, 1)), 5,
							mana_text[1], has_mana_multi(player, MANACOST_XB(c1, 1)), 5,
							mana_text[2], can_play_full_effect, 10);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			charge_mana_multi(player, MANACOST_XBW(
													(c1 > 1 ? (choice != 3 ? c1 : c1-1) : (choice != 3 ? c1 : 0)),
													(choice == 2 || choice == 3),
													(choice == 1 || choice == 3)
													)
								);
		}
		if( spell_fizzled != 1 ){
			instance->info_slot |= 2* (mana_paid[COLOR_WHITE] > 0);
			instance->info_slot |= 4* (mana_paid[COLOR_BLACK] > 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 2) ){
			fog_effect(player, card);
		}
		if( (instance->info_slot & 4) ){
			lose_life(current_turn, count_attackers(current_turn));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_beckon_apparition(int player, int card, event_t event){
	/* Beckon Apparition	|WB
	 * Instant
	 * Exile target card from a graveyard. Put a 1/1 |Swhite and |Sblack Spirit creature token with flying onto the battlefield. */

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && (count_graveyard(player) > 0 || count_graveyard(1-player) > 0) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			instance->targets[0].player = 1-player;
			if( count_graveyard(1-player) > 0 ){
				if( count_graveyard(player) > 0 && player != AI ){
					pick_target(&td, "TARGET_PLAYER");
				}
			}
			else{
				instance->targets[0].player = player;
			}
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) == -1 ){
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_BLACK;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_belligerent_hatchling(int player, int card, event_t event){
	/* Belligerent Hatchling	|3|RW
	 * Creature - Elemental 6/6
	 * First strike
	 * ~ enters the battlefield with four -1/-1 counters on it.
	 * Whenever you cast a |Sred spell, remove a -1/-1 counter from ~.
	 * Whenever you cast a |Swhite spell, remove a -1/-1 counter from ~. */
	return hatchling(player, card, event, COLOR_TEST_WHITE, COLOR_TEST_RED);
}

int card_bloom_tender(int player, int card, event_t event){
	int cols[COLOR_WHITE + 1] = {0};

	if( (event == EVENT_COUNT_MANA && affect_me(player, card)) || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){
		get_card_instance(player, card)->info_slot = 0;
		// Only count colored permanents if they'll actually be needed
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		// Works properly for (possibly laced) lands, despite them using card_data_t::color, due to a special case in get_abilities()
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		int i;
		for (i = COLOR_BLACK; i <= COLOR_WHITE; ++i){
			td.required_color = 1 << i;
			if( can_target(&td) ){
				cols[i] = 1;
				get_card_instance(player, card)->info_slot |= 1<<i;
			}
		}
	}

	return mana_producing_creature_multi(player, card, event, 24, 0, cols[COLOR_BLACK], cols[COLOR_BLUE], cols[COLOR_GREEN], cols[COLOR_RED], cols[COLOR_WHITE]);
}

int card_cache_riders(int player, int card, event_t event){

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		return bounce_permanent_at_upkeep(player, card, event, &td);
	}
	return 0;
}

int card_card_call_the_skybreaker(int player, int card, event_t event){
	/* Call the Skybreaker	|5|UR|UR
	 * Sorcery
	 * Put a 5/5 |Sblue and |Sred Elemental creature token with flying onto the battlefield.
	 * Retrace */

	hybrid(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_RED, 5) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) && can_sorcery_be_played(player, event) ){
			return GA_PLAYABLE_FROM_GRAVE;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_RED, 5);
		if( spell_fizzled != 1 ){
			char msg[100] = "Select a land card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_CAN_CAST){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 5;
		token.tou = 5;
		token.color_forced = COLOR_TEST_BLUE | COLOR_TEST_RED;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_canker_abomination(int player, int card, event_t event){
	enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, count_subtype(1-player, TYPE_CREATURE, -1));
	return hybrid(player, card, event);
}

int card_cankerous_thirst(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		int c1 = get_updated_casting_cost(player, card, -1, event, 3);
		if( has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_GREEN, c1) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int c1 = get_updated_casting_cost(player, card, -1, event, 3);
			test_definition_t test;
			char mana_text[3][100];
			get_mana_from_text(&test, MANACOST_XB(c1, 1));
			strcpy(mana_text[0], test.message);
			get_mana_from_text(&test, MANACOST_XG(c1, 1));
			strcpy(mana_text[1], test.message);
			int can_play_full_effect = 0;
			if( c1 > 0 ){
				if( c1 > 1 ){
					get_mana_from_text(&test, MANACOST_XBG(c1-1, 1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_XBG(c1-1, 1, 1));
				}
				else{
					get_mana_from_text(&test, MANACOST_BG(1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_BG(1, 1));
				}
			}
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							mana_text[0], has_mana_multi(player, MANACOST_XB(c1, 1)), 5*can_target(&td),
							mana_text[1], has_mana_multi(player, MANACOST_XG(c1, 1)), 5*can_target(&td1),
							mana_text[2], can_play_full_effect, 10*(can_target(&td) && can_target(&td1)));
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			charge_mana_multi(player, MANACOST_XBG(
													(c1 > 1 ? (choice != 3 ? c1 : c1-1) : (choice != 3 ? c1 : 0)),
													(choice == 1 || choice == 3),
													(choice == 2 || choice == 3)
													)
								);
		}
		if( spell_fizzled != 1 ){
			if( mana_paid[COLOR_BLACK] > 0 && new_pick_target(&td, "Select target creature for the -3/-3.", -1, GS_LITERAL_PROMPT) ){
				instance->info_slot |= 2;
			}
			if( mana_paid[COLOR_GREEN] > 0 && new_pick_target(&td1, "Select target creature for the +3/+3.", -1, GS_LITERAL_PROMPT) ){
				instance->info_slot |= 4;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 2) && validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
		}
		if( (instance->info_slot & 4) ){
			if( validate_target(player, card, &td1, 1) ){
				pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 3, 3);
			}
			else{
				if( !(instance->info_slot & 2) && validate_target(player, card, &td1, 0) ){
					pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cascade_bluffs(int player, int card, event_t event){
	return card_filter_land(player, card, event, COLOR_BLUE, COLOR_RED, COLOR_TEST_BLUE, COLOR_TEST_RED, " Colorless\n U -> UU\n U -> UR\n U -> RR\n R -> UU\n R -> UR\n R -> RR\n Cancel");
}

int card_cauldron_haze(int player, int card, event_t event){

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int total = instance->number_of_targets = 0;
		if( hybrid_casting(player, card, 0) ){
			while( can_target(&td) ){
					if( select_target(player, card, &td, "Select target Creature.", &(instance->targets[total])) ){
						state_untargettable(instance->targets[total].player, instance->targets[total].card, 1);
						total++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			if( instance->number_of_targets < 1 ){
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				create_targetted_legacy_effect(player, card, &persist_granted, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cenns_enlistment(int player, int card, event_t event){
	/* Cenn's Enlistment	|3|W
	 * Sorcery
	 * Put two 1/1 |Swhite Kithkin Soldier creature tokens onto the battlefield.
	 * Retrace */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		generate_tokens_by_id(player, card, CARD_ID_KITHKIN_SOLDIER, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return retrace(player, card, event, CARD_ID_CENNS_ENLISTMENT);
}

int card_clout_of_the_dominus(int player, int card, event_t event){
	hybrid(player, card, event);
	aura_ability_for_color(player, card, event, COLOR_TEST_RED, 1, 1, 0, SP_KEYWORD_HASTE);
	aura_ability_for_color(player, card, event, COLOR_TEST_BLUE, 1, 1, KEYWORD_SHROUD, 0);
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_cold_eyed_selkie(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_REPORT_DAMAGE_DEALT|DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRIGGER_OPTIONAL) ){
		draw_cards(player, instance->targets[16].player);
		instance->targets[16].player = 0;
	}
	return 0;
}

int card_crackleburr(int player, int card, event_t event){

	if(IS_GAA_EVENT(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.allowed_controller = player;
		td2.preferred_controller = player;
		td2.illegal_abilities = 0;
		td2.required_color = COLOR_TEST_RED;
		td2.illegal_state = TARGET_STATE_TAPPED;

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);
		td3.allowed_controller = player;
		td3.preferred_controller = player;
		td3.illegal_abilities = 0;
		td3.required_color = COLOR_TEST_BLUE;
		td3.required_state = TARGET_STATE_TAPPED;


		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_sick(player, card) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 2, 0, 0, 0);
			if( has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_RED, c1) ){
				if( ! is_tapped(player, card) ){
					int req_targets = 3;
					if( ! (get_color(player, card) & COLOR_TEST_RED) ){
						req_targets = 2;
					}
					if( target_available(player, card, &td2) >= req_targets ){
						return can_target(&td);
					}
				}
				else{
					int req_targets = 3;
					if( ! (get_color(player, card) & COLOR_TEST_BLUE) ){
						req_targets = 2;
					}
					if( target_available(player, card, &td3) >= req_targets ){
						return can_target(&td1);
					}
				}
			}
		}

		if(event == EVENT_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 2, 0, 0, 0);
			if( charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_RED, c1)){
				int choice = 0;
				if( is_tapped(player, card) ){
					int req_targets = 3;
					if( ! (get_color(player, card) & COLOR_TEST_BLUE) ){
						req_targets = 2;
					}
					if( target_available(player, card, &td3) >= req_targets ){
						choice = 1;
					}
				}
				if( spell_fizzled != 1  ){
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							int total = 0;
							state_untargettable(player, card, 1);
							while( total < 2 && can_target(&td2) ){
									if( select_target(player, card, &td2, "Select an untapped red creature.", &(instance->targets[total+1])) ){
										state_untargettable(instance->targets[total+1].player, instance->targets[total+1].card, 1);
										total++;
									}
									else{
										break;
									}
							}
							state_untargettable(player, card, 0);
							instance->number_of_targets = 1;
							int i;
							for(i=0; i<total; i++){
								state_untargettable(instance->targets[i+1].player, instance->targets[i+1].card, 0);
								if( total == 2 ){
									tap_card(instance->targets[i+1].player, instance->targets[i+1].card);
								}
							}
							if( total > 0 ){
								tap_card(player, card);
								instance->info_slot = 66+choice;
							}
							else{
								spell_fizzled = 1;
							}
						}
					}
					else{
						if( pick_target(&td1, "TARGET_CREATURE") ){
							int total = 0;
							state_untargettable(player, card, 1);
							while( total < 2 && can_target(&td3) ){
									if( select_target(player, card, &td3, "Select a tapped blue creature.", &(instance->targets[total+1])) ){
										state_untargettable(instance->targets[total+1].player, instance->targets[total+1].card, 1);
										total++;
									}
									else{
										break;
									}
							}
							instance->number_of_targets = 1;
							state_untargettable(player, card, 0);
							int i;
							for(i=0; i<total; i++){
								state_untargettable(instance->targets[i+1].player, instance->targets[i+1].card, 0);
								if( total == 2 ){
									untap_card(instance->targets[i+1].player, instance->targets[i+1].card);
								}
							}
							if( total > 0 ){
								untap_card(player, card);
								instance->info_slot = 66+choice;
							}
							else{
								spell_fizzled = 1;
							}
						}
					}
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				damage_creature_or_player(player, card, event, 3);
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return hybrid(player, card, event);
}

int card_crag_puca(int player, int card, event_t event)
{
  hybrid(player, card, event);

  // |UR: Switch ~'s power and toughness until end of turn.
  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card))
	return has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_RED,
						   get_cost_mod_for_activated_abilities(player, card, MANACOST_U(1)));

  if (event == EVENT_ACTIVATE)
	charge_mana_hybrid(player, card, 1, COLOR_BLUE, COLOR_RED,
					   get_cost_mod_for_activated_abilities(player, card, MANACOST_U(1)));

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card);
	}

  return 0;
}

int card_creakwood_liege(int player, int card, event_t event){

	/* Creakwood Liege	|1|BG|BG|BG
	 * Creature - Horror 2/2
	 * Other |Sblack creatures you control get +1/+1.
	 * Other |Sgreen creatures you control get +1/+1.
	 * At the beginning of your upkeep, you may put a 1/1 |Sblack and |Sgreen Worm creature token onto the battlefield. */


	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WORM, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN | COLOR_TEST_BLACK;
		generate_token(&token);
	}

	return liege(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_BLACK);
}

int card_deathbringer_liege(int player, int card, event_t event){

	/* Deathbringer Liege	|2|WB|WB|WB
	 * Creature - Horror 3/4
	 * Other |Swhite creatures you control get +1/+1.
	 * Other |Sblack creatures you control get +1/+1.
	 * Whenever you cast a |Swhite spell, you may tap target creature.
	 * Whenever you cast a |Sblack spell, you may destroy target creature if it's tapped. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller &&
		!is_humiliated(player, card) && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
	  ){
		int col = get_color(trigger_cause_controller, trigger_cause);
		int white = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		int black = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		if( col & (white|black) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);

			card_instance_t* instance = get_card_instance(player, card);

			if(event == EVENT_TRIGGER && can_target(&td)){
				event_result = RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( col & white ){
						instance->number_of_targets = 0;
						if( pick_target(&td, "TARGET_CREATURE") ){
							tap_card(instance->targets[0].player, instance->targets[0].card);
						}
					}
					if( col & black ){
						instance->number_of_targets = 0;
						if (IS_AI(player)){
							td.required_state = TARGET_STATE_TAPPED;
						}
						if( can_target(&td) && pick_target(&td, "ROYAL_ASSASSIN") ){
							if (is_tapped(instance->targets[0].player, instance->targets[0].card)){
								kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
							}
						}
					}
			}
		}
	}

	return liege(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_WHITE);
}

int card_deity_of_scars(int player, int card, event_t event){

	/* Deity of Scars	|BG|BG|BG|BG|BG
	 * Creature - Spirit Avatar 7/7
	 * Trample
	 * ~ enters the battlefield with two -1/-1 counters on it.
	 * |BG, Remove a -1/-1 counter from ~: Regenerate ~. */

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		switch( event ){
				case EVENT_CAST_SPELL:
				{
					if( hybrid_casting(player, card, 0) ){
						enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 2);
					}
					else{
						spell_fizzled = 1;
					}
				}
				break;
				case EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE:
					enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 2);
					break;
				default:
					break;
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST0, GVC_COUNTER(COUNTER_M1_M1), NULL, NULL);
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(1));
		if( has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_GREEN, c1) ){
			return result;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(1));
		charge_mana_hybrid(player, card, 1, COLOR_BLACK, COLOR_GREEN, c1);
		if( spell_fizzled != 1 ){
			remove_counter(player, card, COUNTER_M1_M1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card) ){
		regenerate_target(instance->parent_controller, instance->parent_card);
	}

	return 0;
}

int card_desecrator_hag(int player, int card, event_t event){
  /* Desecrator Hag	|2|BG|BG
   * Creature - Hag 2/2
   * When ~ enters the battlefield, return to your hand the creature card in your graveyard with the greatest power. If two or more cards are tied for greatest power, you choose one of them. */

  if( comes_into_play(player, card, event) ){
		const int *grave = get_grave(player);
		int doubles[2][count_graveyard(player)];
		int double_index = 0;
		int max_pow = -1;
		int count = 0;
		int selected = -1;
		while( grave[count] != -1 ){
				if( is_what(-1, grave[count], TYPE_CREATURE) ){
					int pwr = get_base_power_iid(player, grave[count]);
					if( pwr > max_pow ){
						selected = count;
						max_pow = pwr;
						doubles[0][0] = grave[selected];
						doubles[1][0] = selected;
						double_index = 1;
					}
					else if( pwr == max_pow ){
							doubles[0][double_index] = grave[count];
							doubles[1][double_index] = count;
							double_index++;
					}
				}
				count++;
		}
		if( selected != -1 ){
			if( double_index > 1 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select one among these creature cards.");
				selected = select_card_from_zone(player, player, doubles[0], double_index, 1, AI_MAX_VALUE, -1, &this_test);
			}
			int card_added = add_card_to_hand(player, grave[doubles[1][selected]]);
			remove_card_from_grave(player, doubles[1][selected]);
			reveal_card(player, card, player, card_added);
		}
	}

	return hybrid(player, card, event);
}

int card_divinity_of_pride(int player, int card, event_t event){

	lifelink(player, card, event);

	if( life[player] > 24 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, 4, 4, 0);
	}

	return hybrid(player, card, event);
}

int card_dominus_of_fealty(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		upkeep_trigger_ability_mode(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			if( player != instance->targets[0].player ){
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}

	return hybrid(player, card, event);
}

int card_doomgape(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int selected = pick_creature_for_sacrifice(player, card, 1);
		if( selected != -1 ){
			gain_life(player, get_toughness(instance->targets[0].player, instance->targets[0].card));
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}

	return hybrid(player, card, event);
}

int card_dream_fracture(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(instance->targets[0].player, 1);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dream_thief(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && get_stormcolor_count(player, COLOR_BLUE) > 1 ){
		draw_cards(player, 1);
	}

	return hybrid(player, card, event);
}

int card_duergar_assailant(int player, int card, event_t event){

	hybrid(player, card, event);

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_duergar_hedge_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);

		if( count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) > 1 && can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( count_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) > 1 && can_target(&td1) && pick_target(&td1, "TARGET_ENCHANTMENT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return hybrid(player, card, event);
}

int card_duergar_mine_captain(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && is_tapped(player, card) && ! is_sick(player, card) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 1, 0);
		if( has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 1, 0);
		charge_mana_hybrid(player, card, 1, COLOR_RED, COLOR_WHITE, c1);
		if( spell_fizzled != 1 ){
			untap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		pump_creatures_until_eot(player, card, player, 0, 1, 0, 0, 0, &this_test);
	}

	return 0;
}

int card_edge_of_the_divinity(int player, int card, event_t event){
	hybrid(player, card, event);
	aura_ability_for_color(player, card, event, COLOR_TEST_BLACK, 2, 1, 0, 0);
	aura_ability_for_color(player, card, event, COLOR_TEST_WHITE, 1, 2, 0, 0);
	return vanilla_aura(player, card, event, player);
}

int card_endless_horizons(int player, int card, event_t event){

	/* Endless Horizons	|3|W
	 * Enchantment
	 * When ~ enters the battlefield, search your library for any number of |H2Plains cards and exile them. Then shuffle your library.
	 * At the beginning of your upkeep, you may put a card you own exiled with ~ into your hand. */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_PLAINS));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
		this_test.no_shuffle = 1;

		int *deck = deck_ptr[player];
		int index = 0, leg = 0, idx = 0;
		while( 1 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					exiledby_remember(player, card, player, deck[selected], &leg, &idx);
					rfg_card_in_deck(player, selected);
					index++;
				}
				else{
					break;
				}
				if( player == AI && index >= 5 ){
					break;
				}
		}
		shuffle(player);
	}

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player &&
		 exiledby_choose(player, card, CARD_ID_ENDLESS_HORIZONS, EXBY_FIRST_FOUND, 0, "Plains", 1)
	  ){
		 upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int rval = exiledby_choose(player, card, CARD_ID_ENDLESS_HORIZONS, EXBY_CHOOSE, 0, "Plains", 1);
		int* loc = (int*)rval;
		if( loc ){
			int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
			int iid = *loc2 & ~0x80000000;
			*loc2 = -1;
			add_card_to_hand(player, iid);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && player == HUMAN ){
		return exiledby_choose(player, card, CARD_ID_ENDLESS_HORIZONS, EXBY_FIRST_FOUND, 0, "Plains", 1);
	}

	if( event == EVENT_ACTIVATE ){
		exiledby_choose(player, card, CARD_ID_ENDLESS_HORIZONS, EXBY_CHOOSE, 0, "Plains", 1);
		spell_fizzled = 1;
	}

	return global_enchantment(player, card, event);
}

int card_evershrike(int player, int card, event_t event){
	/* Evershrike	|3|WB|WB
	 * Creature - Elemental Spirit 2/2
	 * Flying
	 * ~ gets +2/+2 for each Aura attached to it.
	 * |X|WB|WB: Return ~ from your graveyard to the battlefield. You may put an Aura card with converted mana cost X or less from your hand onto the battlefield attached to it. If you don't, exile ~. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += 2 * count_auras_enchanting_me(player, card);
	}
	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_WHITE, 0) ){
			return GA_RETURN_TO_PLAY_WITH_EFFECT;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_hybrid(player, card, 2, COLOR_BLACK, COLOR_WHITE, 0);
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		int exile_me = 1;
		if( hand_count[player] > 0 ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				int amount = x_value;
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "");
				scnprintf(this_test.message, 100, "Select an Aura card with enchant creature and CMC %d or less.", amount);
				this_test.subtype = SUBTYPE_AURA_CREATURE;
				this_test.cmc = amount+1;
				this_test.cmc_flag = 3;
				this_test.zone = TARGET_ZONE_HAND;
				if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
					int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
					if( result != -1 ){
						put_into_play_aura_attached_to_target(player, result, player, card);
						exile_me = 0;
					}
				}
			}
		}
		if( exile_me == 1 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return hybrid(player, card, event);
}

int card_fable_of_the_wolf_and_owl(int player, int card, event_t event){
	/* Fable of Wolf and Owl	|3|GU|GU|GU
	 * Enchantment
	 * Whenever you cast a |Sgreen spell, you may put a 2/2 |Sgreen Wolf creature token onto the battlefield.
	 * Whenever you cast a |Sblue spell, you may put a 1/1 |Sblue Bird creature token with flying onto the battlefield. */

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me( player, card ) && reason_for_trigger_controller == affected_card_controller
		&& ! is_humiliated(player, card)
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			if( get_color(trigger_cause_controller, trigger_cause) & get_sleighted_color_test(player, card, COLOR_TEST_BLUE) ){
				trig |= 1;
			}
			if( get_color(trigger_cause_controller, trigger_cause) & get_sleighted_color_test(player, card, COLOR_TEST_GREEN) ){
				trig |= 2;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( trig == 3 && ! duh_mode(player) ){
						trig = 1+do_dialog(player, player, card, -1, -1, " Generate a Bird\n Generate a Wolf\n Generate both\n Do nothing", 0);
					}
					if( trig > 3 ){
						trig = 0;
					}
					if( (trig & 1) ){
						token_generation_t token;
						default_token_definition(player, card, CARD_ID_BIRD, &token);
						token.color_forced = COLOR_TEST_BLUE;
						generate_token(&token);
					}
					if( (trig & 2) ){
						generate_token_by_id(player, card, CARD_ID_WOLF);
					}
			}
		}
	}
	return hybrid(player, card, event);
}

int card_fang_skulkin(int player, int card, event_t event){
	return skulkin(player, card, event, 2, COLOR_TEST_BLACK, 0, 0, 0, SP_KEYWORD_WITHER);
}

int card_favor_of_the_overbeing(int player, int card, event_t event){
	hybrid(player, card, event);
	aura_ability_for_color(player, card, event, COLOR_TEST_GREEN, 1, 1, 0, SP_KEYWORD_VIGILANCE);
	aura_ability_for_color(player, card, event, COLOR_TEST_BLUE, 1, 1, KEYWORD_FLYING, 0);
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_fetid_heath(int player, int card, event_t event){
	return card_filter_land(player, card, event, COLOR_BLACK, COLOR_WHITE, COLOR_TEST_BLACK, COLOR_TEST_WHITE, " Colorless\n B -> BB\n B -> BW\n B -> WW\n W -> BB\n W -> BW\n W -> WW\n Cancel");
}

int card_fiery_bombardment(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_X(2), 0, &td, NULL) ){
			if( player == HUMAN ){
				return 1;
			}
			else{
				int c;
				for(c=0; c<active_cards_count[player]; c++){
					if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && chroma(player, c, COLOR_RED, 0) > 0 ){
						return 1;
					}
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( player == AI ){
				int c;
				for(c=0; c<active_cards_count[player]; c++){
					if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && chroma(player, c, COLOR_RED, 0) < 1 ){
						state_untargettable(player, c, 1);
					}
				}
			}
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->info_slot = chroma(BYTE2(sac), BYTE3(sac), COLOR_RED, 0);
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
			if( player == AI ){
				int c;
				for(c=0; c<active_cards_count[player]; c++){
					if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && chroma(player, c, COLOR_RED, 0) < 1 ){
						state_untargettable(player, c, 0);
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
		}
	}

	return 0;
}

int card_figure_of_destiny(int player, int card, event_t event){
	hybrid(player, card, event);

	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[9].player = 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 1, 0);
		return has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_W(1));
		int abilities[3] = {	has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1),
								has_mana_hybrid(player, 3, COLOR_RED, COLOR_WHITE, c1),
								has_mana_hybrid(player, 6, COLOR_RED, COLOR_WHITE, c1)
		};
		int priorities[3] = { 	instance->targets[9].player < 1 ? 10 : -50,
								instance->targets[9].player < 2 ? 15 : -50,
								instance->targets[9].player < 3 ? 20 : -50,
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"2/2 Kithkin Spirit", abilities[0], priorities[0],
						"4/4 Kithkin Spirit Warrior", abilities[1], priorities[1],
						"8/8 Kithkin Spirit Warrior Avatar", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		charge_mana_hybrid(player, card, choice > 1 ? 3*(choice-1) : 1, COLOR_RED, COLOR_WHITE, c1);
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card  );
		if( instance->info_slot == 1 ||
			(instance->info_slot == 2 && has_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_SPIRIT)) ||
			(instance->info_slot == 3 && has_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_WARRIOR))
		  ){
			parent->targets[9].player = instance->info_slot;
			int legacy = real_set_pt(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, pow(2, instance->info_slot), pow(2, instance->info_slot), 0);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if(event == EVENT_ABILITIES && affect_me(player, card ) && ! is_humiliated(player, card) ){
		if( instance->targets[9].player == 3 ){
			event_result |= KEYWORD_FLYING;
			event_result |= KEYWORD_FIRST_STRIKE;
		}
	}
	return 0;
}

int card_fire_at_will(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			int total = 0;
			while( total < 3 ){
					if( select_target(player, card, &td, "Select target creature.", &(instance->targets[total])) ){
						total++;
					}
					else{
						break;
					}
			}
			if( total != 3 ){
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flame_jab(int player, int card, event_t event){
	/* Flame Jab	|R
	 * Sorcery
	 * ~ deals 1 damage to target creature or player.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return retrace(player, card, event, CARD_ID_FLAME_JAB);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		damage_creature_or_player(player, card, event, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_flickerwisp(int player, int card, event_t event){ // UNUSEDCARD
	return 0;
}

int card_flickerwhisp(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;
		td.special = TARGET_SPECIAL_NOT_ME;

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}


int card_flooded_grove(int player, int card, event_t event){
	return card_filter_land(player, card, event, COLOR_BLUE, COLOR_GREEN, COLOR_TEST_BLUE, COLOR_TEST_GREEN, " Colorless\n U -> UU\n U -> UG\n U -> GG\n G -> UU\n G -> UG\n G -> GG\n Cancel");
}

int card_gift_of_the_deity(int player, int card, event_t event){
	hybrid(player, card, event);
	aura_ability_for_color(player, card, event, COLOR_TEST_BLACK, 1, 1, 0, SP_KEYWORD_DEATHTOUCH);
	aura_ability_for_color(player, card, event, COLOR_TEST_GREEN, 1, 1, 0, SP_KEYWORD_LURE);
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_gilder_bairn(int player, int card, event_t event){

	hybrid(player, card, event);

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_TAPPED, MANACOST0, 0, &td, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 1, 0, 0, 0);
			if( has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_GREEN, c1) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 1, 0, 0, 0);
		charge_mana_hybrid(player, card, 1, COLOR_BLUE, COLOR_GREEN, c1);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			untap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			copy_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return 0;
}

int card_glen_elendra_archmage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	persist(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.illegal_type = TYPE_CREATURE;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK, MANACOST_U(1), 0, &td, NULL);
}

int card_gwyllion_hedge_mage(int player, int card, event_t event){
	/* Gwyllion Hedge-Mage	|2|WB
	 * Creature - Hag Wizard 2/2
	 * When ~ enters the battlefield, if you control two or more |H1Plains, you may put a 1/1 |Swhite Kithkin Soldier creature token onto the battlefield.
	 * When ~ enters the battlefield, if you control two or more |H1Swamps, you may put a -1/-1 counter on target creature. */

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);

		if( count_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) > 1 ){
			int choice = duh_mode(player) ? 0 : do_dialog(player, player, card, -1, -1, " Generate a Kithkin\n Pass", 0);
			if( ! choice ){
				generate_token_by_id(player, card, CARD_ID_KITHKIN_SOLDIER);
			}
		}
		if( count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) > 1 && can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return hybrid(player, card, event);
}

int card_hag_hedge_mage(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance( player, card );

		if( count_subtype(player, TYPE_LAND, SUBTYPE_FOREST) > 1 && count_graveyard(player) > 0 && ! graveyard_has_shroud(player) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
		}

		if( count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) > 1 && can_target(&td1) && pick_target(&td1, "TARGET_PLAYER") ){
			discard(instance->targets[0].player, 0, player);
		}

		instance->number_of_targets = 0;
	}

	return hybrid(player, card, event);
}

int card_hallowed_burial(int player, int card, event_t event){
	/* Hallowed Burial	|3|W|W
	 * Sorcery
	 * Put all creatures on the bottom of their owners' libraries. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test, ACT_PUT_ON_BOTTOM);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_hateflayer(int player, int card, event_t event){

	wither(player, card, event);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				damage_target0(player, card, get_power(instance->parent_controller, instance->parent_card));
			}
		}

		return generic_activated_ability(player, card, event, GAA_TAPPED|GAA_CAN_TARGET, MANACOST_XR(2, 1), 0,
										&td1, "TARGET_CREATURE_OR_PLAYER");
	}
	return 0;
}

int card_heartfire_goblin(int player, int card, event_t event){
	return hybrid(player, card, event);
}

int card_helix_pinnacle(int player, int card, event_t event)
{
  /* Helix Pinnacle	|G
   * Enchantment
   * Shroud
   * |X: Put X tower counters on ~.
   * At the beginning of your upkeep, if there are 100 or more tower counters on ~, you win the game. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (current_turn == player && count_counters(player, card, COUNTER_TOWER) >= 100 && upkeep_trigger(player, card, event))
	lose_the_game(1-player);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST_X(IS_AI(player) ? 1 : 0));

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_X(-1)))
	{
	  get_card_instance(player, card)->info_slot = x_value;
	  if (player == AI && !(current_turn == 1-player && current_phase == PHASE_DISCARD))
		ai_modifier -= 2 * x_value - 1;	// Cancels out nearly all the perceived benefit except during opponent's end phase
	}

  if (event == EVENT_GET_SELECTED_CARD)
	EXE_FN(void, 0x499010, int)(0);	// retrieves that will be spent on x into displayed_x_value

  if (event == EVENT_RESOLVE_ACTIVATION)
	add_counters(player, card, COUNTER_TOWER, get_card_instance(player, card)->info_slot);

  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  int counters = count_counters(player, card, COUNTER_TOWER);

	  int mod = 4 + 2 * counters;	// About the minimum that will get the AI to cast it and to add the first counter

	  if (player == HUMAN || (current_turn == 1-player && current_phase == PHASE_DISCARD))
		mod += (counters * counters) / 10 + (counters * counters * counters * counters) / 100000;

	  if (counters >= 100)
		mod += 1000;

	  if (player == HUMAN)
		ai_modifier -= mod;
	  else
		ai_modifier += mod;
	}

	return global_enchantment(player, card, event);
}

int card_hoof_skulkin(int player, int card, event_t event){
	return skulkin(player, card, event, 3, COLOR_TEST_GREEN, 1, 1, 0, 0);
}

int card_hotheaded_giant(int player, int card, event_t event){
	haste(player, card, event);
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && get_stormcolor_count(player, COLOR_RED) < 2 ){
		add_minus1_minus1_counters(player, card, 2);
	}
	return 0;
}

int card_idle_thoughts(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] < 1 ){
		return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_inundate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLUE;
		this_test.color_flag = 1;
		new_manipulate_all(player, card, 2, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_inside_out(int player, int card, event_t event)
{
  modify_cost_for_hybrid_spells(player, card, event, 0);

  // Switch target creature's power and toughness until end of turn.
  // Draw a card.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (hybrid_casting(player, card, 0))
		pick_target(&td, "TARGET_CREATURE");
	  else
		spell_fizzled = 1;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		  draw_cards(player, 1);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}


int card_invert_the_skies(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		int c1 = get_updated_casting_cost(player, card, -1, event, 3);
		if( has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_GREEN, c1) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int c1 = get_updated_casting_cost(player, card, -1, event, 3);
			test_definition_t test;
			char mana_text[3][100];
			get_mana_from_text(&test, MANACOST_XU(c1, 1));
			strcpy(mana_text[0], test.message);
			get_mana_from_text(&test, MANACOST_XG(c1, 1));
			strcpy(mana_text[1], test.message);
			int can_play_full_effect = 0;
			if( c1 > 0 ){
				if( c1 > 1 ){
					get_mana_from_text(&test, MANACOST_XUG(c1-1, 1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_XUG(c1-1, 1, 1));
				}
				else{
					get_mana_from_text(&test, MANACOST_UG(1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_UG(1, 1));
				}
			}
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							mana_text[0], has_mana_multi(player, MANACOST_XU(c1, 1)), 10,
							mana_text[1], has_mana_multi(player, MANACOST_XG(c1, 1)), 5,
							mana_text[2], can_play_full_effect, 15);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			charge_mana_multi(player, MANACOST_XUG(
													(c1 > 1 ? (choice != 3 ? c1 : c1-1) : (choice != 3 ? c1 : 0)),
													(choice == 1 || choice == 3),
													(choice == 2 || choice == 3)
													)
								);
		}
		if( spell_fizzled != 1 ){
			if( mana_paid[COLOR_BLUE] > 0 ){
				instance->info_slot |= 2;
			}
			if( mana_paid[COLOR_GREEN] > 0 ){
				instance->info_slot |= 4;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 2) ){
			pump_subtype_until_eot(player, card, player, -1, 0, 0,  KEYWORD_FLYING, 0);
		}
		if( (instance->info_slot & 4) ){
			negate_ability_until_eot(player, card, 1-player, -1, KEYWORD_FLYING);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jawbone_skulkin(int player, int card, event_t event){
	return skulkin(player, card, event, 2, COLOR_TEST_RED, 0, 0, 0, SP_KEYWORD_HASTE);
}

int card_leering_emblem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && ! is_equipping(player, card) ){
		ai_modifier+=30;
	}

	if( is_equipping(player, card) && specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, instance->targets[8].player, instance->targets[8].card, 2, 2);
	}

	return vanilla_equipment(player, card, event, 2, 0, 0, 0, 0);
}

int card_light_from_within(int player, int card, event_t event){

	if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card) ){
		int amount = chroma(affected_card_controller, affected_card, COLOR_WHITE, 0);
		modify_pt_and_abilities(affected_card_controller, affected_card, event, amount, amount, 0);
	}

	return global_enchantment(player, card, event);
}

int card_merrow_bonegnawer(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, ANYBODY, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLUE, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && count_graveyard(instance->targets[0].player) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_merrow_levitator(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ANY, 0, 0, 0, COLOR_TEST_BLUE, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_mindwrack_liege(int player, int card, event_t event){

	/* Mindwrack Liege	|3|UR|UR|UR
	 * Creature - Horror 4/4
	 * Other |Sblue creatures you control get +1/+1.
	 * Other |Sred creatures you control get +1/+1.
	 * |UR|UR|UR|UR: You may put a |Sblue or |Sred creature card from your hand onto the battlefield. */

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_R(4));
		return has_mana_hybrid(player, 4, COLOR_BLUE, COLOR_RED, c1);
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_R(4));
		charge_mana_hybrid(player, card, 4, COLOR_BLUE, COLOR_RED, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, get_sleighted_color_text2(player, card, "Select a %s or %s creature card.", COLOR_BLUE, COLOR_RED));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE | COLOR_TEST_RED);
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return liege(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_RED);
}

static int mirror_sheen_player = 0;
static const char* target_is_targeting_mirror_sheen_player(int who_chooses, int player, int card)
{
  if (target_me(mirror_sheen_player, -1, player, card))
	return NULL;
  else
	return "targets you";
}

int card_mirror_sheen(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	hybrid(player, card, event);

	if (!IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_INSTANT|TYPE_SORCERY);
	td.preferred_controller = 2;
	td.extra = (int32_t)target_is_targeting_mirror_sheen_player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	mirror_sheen_player = player;

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(1, 2));
		if( has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_RED, c1) ){
			return activate_twincast(player, card, event, &td, NULL);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		activate_twincast(player, card, event, &td, NULL);
		if (spell_fizzled != 1){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(1, 2));
			charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_RED, c1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		activate_twincast(player, card, event, &td, NULL);
	}

	return 0;
}

int card_monstrify(int player, int card, event_t event){
	if (IS_CASTING_FROM_GRAVE(event)){
		return retrace(player, card, event, CARD_ID_MONSTRIFY);
	}
	return card_monstrous_growth(player, card, event);
}

int card_murkfiend_liege(int player, int card, event_t event){

	/* Murkfiend Liege	|2|GU|GU|GU
	 * Creature - Horror 4/4
	 * Other |Sgreen creatures you control get +1/+1.
	 * Other |Sblue creatures you control get +1/+1.
	 * Untap all |Sgreen and/or |Sblue creatures you control during each other player's untap step. */

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn != player && current_phase == PHASE_UNTAP ){
		if( instance->targets[1].player != 66 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "");
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE | COLOR_TEST_GREEN);
			new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
			instance->targets[1].player = 66;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return liege(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_GREEN);
}

int card_necroskitter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	wither(player, card, event);

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
				! is_token(affected_card_controller, affected_card) && count_minus1_minus1_counters(affected_card_controller, affected_card) > 0
			  ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
					if( instance->targets[11].player < 0 ){
						instance->targets[11].player = 0;
					}
					int pos = instance->targets[11].player;
					if( pos < 10 ){
						instance->targets[pos].player = is_stolen(affected_card_controller, affected_card) ? 1-affected_card_controller : affected_card_controller;
						instance->targets[pos].card = get_original_id(affected_card_controller, affected_card);
						instance->targets[11].player++;
					}
				}
			}
		}

		if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller){
			if( affect_me(player, card ) ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						int i;
						for(i=0; i<instance->targets[11].player; i++){
							seek_grave_for_id_to_reanimate(player, card, instance->targets[i].player, instance->targets[i].card, REANIMATE_DEFAULT);
						}
						instance->targets[11].player = 0;
				}
				else if (event == EVENT_END_TRIGGER){
					instance->targets[11].player = 0;
				}
			}
		}
	}

	return 0;
}

int card_needle_specter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	wither(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_REPORT_DAMAGE_DEALT+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		new_multidiscard(1-player, instance->targets[16].player, 0, player);
		instance->targets[16].player = 0;
	}
	return 0;
}

int card_nettle_sentinel(int player, int card, event_t event){
	does_not_untap(player, card, event);
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ANY, 0, 0, 0, COLOR_TEST_GREEN, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}
	return 0;
}

int card_nightmare_incursion(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
			int removed[99];
			int r_count = 0;
			int i;
			for(i=0; i<amount; i++){
				char msg[100] = "Select a card to exile.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.no_shuffle = 1;
				int crd = new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
				if( crd != -1 ){
					removed[r_count] = get_internal_card_id_from_csv_id(crd);
					r_count++;
				}
			}
			if( 1-instance->targets[0].player == HUMAN && r_count > 0 ){
				show_deck( player, removed, r_count, "Cards exiled by Nightmare Incursion", 0, 0x7375B0 );
			}
			shuffle(instance->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_nightsky_mimic(int player, int card, event_t event){
	return mimic(player, card, event, COLOR_TEST_WHITE, COLOR_TEST_BLACK, 4, 4, KEYWORD_FLYING, 0);
}

int card_nip_gwyllion(int player, int card, event_t event){
	lifelink(player, card, event);
	return hybrid(player, card, event);
}

int card_nobilis_of_war(int player, int card, event_t event){
	if( event == EVENT_POWER && affected_card_controller == player && is_attacking(affected_card_controller, affected_card) &&
		! is_humiliated(player, card)
	  ){
		event_result+=2;
	}
	return hybrid(player, card, event);
}

int card_noggle_hedge_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		instance->number_of_targets = 0;
		if( count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) >= 2 && can_target(&td) ){
			while( instance->number_of_targets < 2 && can_target(&td) && new_pick_target(&td, "TARGET_PERMANENT", -1, 0) ){
				state_untargettable(instance->targets[instance->number_of_targets - 1].player, instance->targets[instance->number_of_targets - 1].card, 1);
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		if( count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) >= 2 && can_target(&td1) && new_pick_target(&td1, "TARGET_PLAYER", -1, 0) ){
			damage_player(instance->targets[instance->number_of_targets - 1].player, 2, player, card);
		}
	}

	return hybrid(player, card, event);
}

int card_noxious_hatchling(int player, int card, event_t event){
	/* Noxious Hatchling	|3|BG
	 * Creature - Elemental 6/6
	 * ~ enters the battlefield with four -1/-1 counters on it.
	 * Wither
	 * Whenever you cast a |Sblack spell, remove a -1/-1 counter from ~.
	 * Whenever you cast a |Sgreen spell, remove a -1/-1 counter from ~. */
	wither(player, card, event);
	return hatchling(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_BLACK);
}

int card_nucklavee(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && ! graveyard_has_shroud(2) ){
		char msg[100] = "Select a blue instant card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT, msg);
		this_test.type_flag = F1_NO_CREATURE;
		this_test.color = COLOR_TEST_BLUE;
		if( new_special_count_grave(player, &this_test) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
		strcpy(msg, "Select a red sorcery card.");
		new_default_test_definition(&this_test, TYPE_SORCERY, msg);
		this_test.type_flag = F1_NO_CREATURE;
		this_test.color = COLOR_TEST_RED;
		if( new_special_count_grave(player, &this_test) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return hybrid(player, card, event);
}

int card_oonas_grace(int player, int card, event_t event){
	/* Oona's Grace	|2|U
	 * Instant
	 * Target player draws a card.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return retrace(player, card, event, CARD_ID_OONAS_GRACE);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_overbeing_of_myth(int player, int card, event_t event)
{
	/* Overbeing of Myth	|GU|GU|GU|GU|GU
	 * Creature - Spirit Avatar 100/100
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * At the beginning of your draw step, draw an additional card. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	if (event == EVENT_DRAW_PHASE && current_turn == player && !is_humiliated(player, card)){
	  event_result += 1;
	}

	return hybrid(player, card, event);
}

int card_patrol_signaler(int player, int card, event_t event){
	/* Patrol Signaler	|1|W
	 * Creature - Kithkin Soldier 1/1
	 * |1|W, |Q: Put a 1/1 |Swhite Kithkin Soldier creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_KITHKIN_SOLDIER);
	}

	return generic_activated_ability(player, card, event, GAA_TAPPED, MANACOST_XW(1, 1), 0, NULL, NULL);
}

int card_primalcrux(int player, int card, event_t event){
	/* Primalcrux	|G|G|G|G|G|G
	 * Creature - Elemental 100/100
	 * Trample
	 * Chroma - ~'s power and toughness are each equal to the number of |Sgreen mana symbols in the mana costs of permanents you control. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result+=chroma(player, -1, COLOR_GREEN, 0);
	}

	return 0;
}

int card_puncture_blast(int player, int card, event_t event)
{
  /* Puncture Blast	|2|R
   * Instant
   * Wither
   * ~ deals 3 damage to target creature or player. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  // As a non-permanent, this never gets EVENT_ABILITIES, and so wither() won't ever do anything.
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->targets[16].card == -1)
	instance->targets[16].card = 0;
  instance->targets[16].card |= SP_KEYWORD_WITHER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 3);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_phyrric_revival(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all(player, card, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, REANIMATE_MINUS1_MINUS1_COUNTER);
		kill_card(player, card, KILL_DESTROY);
	}

	return hybrid(player, card, event);
}

int card_quillspike(int player, int card, event_t event){

	/* Quillspike	|2|BG
	 * Creature - Beast 1/1
	 * |BG, Remove a -1/-1 counter from a creature you control: ~ gets +3/+3 until end of turn. */

	if( event == EVENT_MODIFY_COST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return hybrid(player, card, event);
	}

	if(!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_M1_M1;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_G(1));
			return has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_GREEN, c1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_G(1));
		charge_mana_hybrid(player, card, 1, COLOR_BLACK, COLOR_GREEN, c1);
		if( spell_fizzled != 1 && new_pick_target(&td, "Select a creature you control with a -1/-1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 1;
			remove_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_M1_M1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 3, 3);
	}

	return 0;
}

int card_ravens_crime(int player, int card, event_t event){
	/* Raven's Crime	|B
	 * Sorcery
	 * Target player discards a card.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return retrace(player, card, event, CARD_ID_RAVENS_CRIME);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_regal_force(int player, int card, event_t event){
	if(comes_into_play(player, card, event) > 0 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_color = COLOR_TEST_GREEN;
		td.allowed_controller = player;
		td.illegal_abilities = 0;
		td.preferred_controller = player;
		draw_cards(player, target_available(player, card, &td));
	}
	return 0;
}

int card_rekindle_the_flame(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		int choice = 1;
		if( hand_count[1-player] < 1 ){
			choice = do_dialog(player, player, card, -1, -1," Return Rekindle the Flame to hand\n Pass", 0);
		}
		if( choice == 0 ){
			instance->state &= ~STATE_INVISIBLE;
			hand_count[player]++;
			return -1;
		}
		else{
			return -2;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_restless_apparition(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	persist(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 3, 0, 0, 0, 0);
		return has_mana_hybrid(player, 3, COLOR_BLACK, COLOR_WHITE, c1);
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 3, 0, 0, 0, 0);
		charge_mana_hybrid(player, card, 3, COLOR_BLACK, COLOR_WHITE, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 3, 3);
	}

	return hybrid(player, card, event);
}

// goblin soldier token --> rhino token.

int card_rise_of_the_hobgoblins(int player, int card, event_t event){
	/* Rise of the Hobgoblins	|RW|RW
	 * Enchantment
	 * When ~ enters the battlefield, you may pay |X. If you do, put X 1/1 |Sred and |Swhite Goblin Soldier creature tokens onto the battlefield.
	 * |RW: |SRed creatures and |Swhite creatures you control gain first strike until end of turn. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		hybrid(player, card, event);
	}

	if( comes_into_play(player, card, event) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			generate_tokens_by_id(player, card, CARD_ID_GOBLIN_SOLDIER, x_value);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_W(1));
		return has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1);
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_W(1));
		charge_mana_hybrid(player, card, 1, COLOR_RED, COLOR_WHITE, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		pump_creatures_until_eot(instance->parent_controller, instance->parent_card, player, 0, 0, 0, KEYWORD_FIRST_STRIKE, 0, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_riverfall_mimic(int player, int card, event_t event){
	return mimic(player, card, event, COLOR_TEST_RED, COLOR_TEST_BLUE, 3, 3, 0, SP_KEYWORD_UNBLOCKABLE);
}

int card_rugged_prarie(int player, int card, event_t event){
	return card_filter_land(player, card, event, COLOR_WHITE, COLOR_RED, COLOR_TEST_WHITE, COLOR_TEST_RED, " Colorless\n W -> WW\n W -> WR\n W -> RR\n R -> WW\n R -> WR\n R -> RR\n Cancel");
}

int card_sanity_grinding(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int to_reveal = MIN(10, count_deck(player));
			if( to_reveal > 0 ){
				int *deck = deck_ptr[player];
				int result = 0;
				int count = 0;
				while( deck[count] != -1 && count < to_reveal ){
						result+=count_chroma(-1, deck[count], COLOR_BLUE, 0, 0);
						count++;
				}
				show_deck( HUMAN, deck, to_reveal, "Cards revealed with Sanity Grinding", 0, 0x7375B0 );
				put_top_x_on_bottom(player, player, to_reveal);
				if( result > 0 ){
					mill(instance->targets[0].player, result);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_sapling_of_colfenor(int player, int card, event_t event){
	/* Sapling of Colfenor	|3|BG|BG
	 * Legendary Creature - Treefolk Shaman 2/5
	 * Indestructible
	 * Whenever ~ attacks, reveal the top card of your library. If it's a creature card, you gain life equal to that card's toughness, lose life equal to its power, then put it into your hand. */

	indestructible(player, card, event);

	check_legend_rule(player, card, event);

	if (declare_attackers_trigger(player, card, event, 0, player, card) && deck_ptr[player][0] != -1){
		int* deck = deck_ptr[player];
		reveal_card_iid(player, card, deck[0]);
		if( cards_data[deck[0]].type & TYPE_CREATURE ){
			int pwr = get_base_power_iid(player, deck[0]);
			int tou = get_base_toughness_iid(player, deck[0]);

			gain_life(player, tou);
			lose_life(player, pwr);
			add_card_to_hand(player, deck[0]);
			remove_card_from_deck(player, 0);
		}
	}

	return hybrid(player, card, event);
}

int card_savage_conception(int player, int card, event_t event){
	/* Savage Conception	|3|G|G
	 * Sorcery
	 * Put a 3/3 |Sgreen Beast creature token onto the battlefield.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return retrace(player, card, event, CARD_ID_SAVAGE_CONCEPTION);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_BEAST);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 1, NULL);
}

int card_scarecrone(int player, int card, event_t event){
	/*
	  Scarecrone |3
	  Artifact Creature - Scarecrow 1/2
	  {1}, Sacrifice a Scarecrow: Draw a card.
	  {4}, {T}: Return target artifact creature card from your graveyard to the battlefield.
	*/
	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, "Select an artifact creature to reanimate.");
	this_test.special_selection_function = &is_artifact_creature_by_internal_id;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) &&
			can_sacrifice(player, player, 1, TYPE_PERMANENT, SUBTYPE_SCARECROW)
		  ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL) ){
			if( new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[2] = {	generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_X(1), 0, NULL, NULL) &&
								can_sacrifice(player, player, 1, TYPE_PERMANENT, SUBTYPE_SCARECROW),
								generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL) &&
								new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player)
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Sac & draw", abilities[0], 5,
						"Reanimate an artifact creature", abilities[1], 15);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(choice == 1 ? 1 : 4)) ){
			if( choice == 1 ){
				test_definition_t this_test2;
				new_default_test_definition(&this_test2, TYPE_PERMANENT, "Select a Scarecrow to sacrifice.");
				this_test2.subtype = SUBTYPE_SCARECROW;

				int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &this_test2);
				if (!sac){
					spell_fizzled = 1;
				}
				else{
					kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				}
			}

			if( choice == 2 ){
				if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			draw_cards(player, 1);
		}
		if( instance->info_slot == 2 ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
		}
	}

	return 0;
}

int card_scourge_of_the_nobilis(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	hybrid(player, card, event);
	if( in_play(player, card) && instance->damage_target_player != - 1 && ! is_humiliated(player, card) ){
		int clr = get_color(instance->damage_target_player, instance->damage_target_card);
		if( clr & COLOR_TEST_RED ){
			modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, 1, 1, 0);
			if( event == EVENT_CAN_ACTIVATE ){
				if( generic_activated_ability(instance->damage_target_player, instance->damage_target_card, event, 0, MANACOST0, 0, NULL, NULL) ){
					int c1 = get_cost_mod_for_activated_abilities(instance->damage_target_player, instance->damage_target_card, 0, 0, 0, 0, 1, 0);
					return has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1);
				}
			}

			if(event == EVENT_ACTIVATE ){
				int c1 = get_cost_mod_for_activated_abilities(instance->damage_target_player, instance->damage_target_card, 0, 0, 0, 0, 1, 0);
				charge_mana_hybrid(player, card, 1, COLOR_RED, COLOR_WHITE, c1);
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				pump_until_eot(player, instance->parent_card, instance->damage_target_player, instance->damage_target_card, 1, 0);
			}
		}
		if( clr & COLOR_TEST_WHITE ){
			modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, 1, 1, 0);
			lifelink(instance->damage_target_player, instance->damage_target_card, event);
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_selkie_hedge_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_TAPPED;

		if( count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) > 1 && can_target(&td) ){
			if( new_pick_target(&td, "Select target tapped creature.", 0, GS_LITERAL_PROMPT) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
		if( count_subtype(player, TYPE_LAND, SUBTYPE_FOREST) > 1 ){
			int choice = duh_mode(player) ? 0 : do_dialog(player, player, card, -1, -1, " Gain 3 life\n Pass", 0);
			if( ! choice ){
				gain_life(player, 3);
			}
		}
	}

	return hybrid(player, card, event);
}

int card_shell_skulkin(int player, int card, event_t event){
	return skulkin(player, card, event, 3, COLOR_TEST_BLUE, 0, 0, KEYWORD_SHROUD, 0);
}

int card_shorecrasher_mimic(int player, int card, event_t event){
	return mimic(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_BLUE, 5, 3, KEYWORD_TRAMPLE, 0);
}

int card_shrewd_hatchling(int player, int card, event_t event){

	/* Shrewd Hatchling	|3|UR
	 * Creature - Elemental 6/6
	 * ~ enters the battlefield with four -1/-1 counters on it.
	 * |UR: Target creature can't block ~ this turn.
	 * Whenever you cast a |Sblue spell, remove a -1/-1 counter from ~.
	 * Whenever you cast a |Sred spell, remove a -1/-1 counter from ~. */


	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
				int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 1, 0);
				return has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_RED, c1);
			}
		}

		if(event == EVENT_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 1, 0);
			charge_mana_hybrid(player, card, 1, COLOR_BLUE, COLOR_RED, c1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				int p = instance->parent_controller, c = instance->parent_card;
				creature1_cant_block_creature2_until_eot(p, c, instance->targets[0].player, instance->targets[0].card, p, c);
			}
		}
	}

	return hatchling(player, card, event, COLOR_TEST_RED, COLOR_TEST_BLUE);
}

int card_slippery_bogle(int player, int card, event_t event)
{
  hexproof(player, card, event);
  return hybrid(player, card, event);
}

int card_snakeform(int player, int card, event_t event){
	/*
	  Snakeform |2|{G/U}
	  Instant
	  Target creature loses all abilities and becomes a 1/1 green Snake until end of turn.
	  Draw a card.
	*/

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 1;
			hc.toughness = 1;
			hc.subtype = SUBTYPE_SNAKE;
			hc.color = COLOR_TEST_GREEN;
			force_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_SNAKE);
			humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 4, &hc);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_soot_imp(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLACK, 1, 0, 0, -1, 0) ){
		lose_life(instance->targets[1].player, 1);
	}
	return 0;
}

int card_soul_snuffers(int player, int card, event_t event){
	/* Soul Snuffers	|2|B|B
	 * Creature - Elemental Shaman 3/3
	 * When ~ enters the battlefield, put a -1/-1 counter on each creature. */

	if( comes_into_play(player, card, event) ){
		manipulate_type(player, card, ANYBODY, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_M1_M1, 1));
	}
	return 0;
}

int card_spirit_of_the_hearth(int player, int card, event_t event){
	give_hexproof_to_player(player, card, event);
	return 0;
}

int card_spitemare(int player, int card, event_t event){
  /* Spitemare	|2|RW|RW
   * Creature - Elemental 3/3
   * Whenever ~ is dealt damage, it deals that much damage to target creature or player. */

	hybrid(player, card, event);

	if( ! is_humiliated(player, card) ){
	  card_instance_t* damage = damage_being_dealt(event);
	  if (damage
		  && damage->damage_target_card == card && damage->damage_target_player == player)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (instance->info_slot < 10)
			{
			  if (instance->info_slot < 1)
				instance->info_slot = 1;

			  instance->targets[instance->info_slot++].player = damage->info_slot;
			}
		}

	  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (instance->info_slot > 1)
			{
			  if (event == EVENT_TRIGGER)
				event_result |= RESOLVE_TRIGGER_MANDATORY;

			  if (event == EVENT_RESOLVE_TRIGGER)
				{
				  target_definition_t td;
				  default_target_definition(player, card, &td, TYPE_CREATURE);
				  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
				  td.allow_cancel = 0;

				  char prompt[100];
				  prompt[0] = 0;

				  // Free up info_slot and targets, just in case something triggers instantly (and improperly) which eventually results in dealing damage back.
				  int i, dam[10];
				  for (i = 1; i < instance->info_slot; ++i)
					dam[i] = instance->targets[i].player;

				  int numdam = instance->info_slot;
				  instance->info_slot = 1;

				  for (i = 1; i < numdam; ++i)
					{
					  if (ai_is_speculating != 1)
						{
						  load_text(0, "TARGET_CREATURE_OR_PLAYER");
						  scnprintf(prompt, 100, "%s (%d damage)", text_lines[0], dam[i]);
						}
					  instance->number_of_targets = 0;
					  if (can_target(&td) && pick_next_target_noload(&td, prompt))
						damage_target0(player, card, dam[i]);
					}
				}
			}
		}
	}

	return 0;
}

int card_spitting_image(int player, int card, event_t event){
	/* Spitting Image	|4|GU|GU
	 * Sorcery
	 * Put a token that's a copy of target creature onto the battlefield.
	 * Retrace */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		if( has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_GREEN, cless) && check_battlefield_for_special_card(player, card, player, 0, &this_test) &&
			can_sorcery_be_played(player, event)
		  ){
			return can_target(&td) ? GA_PLAYABLE_FROM_GRAVE : 0;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_GREEN, cless);
		if( spell_fizzled != 1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		if( event == EVENT_CAN_CAST ){
			return can_target(&td);
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				copy_token(player, card, instance->targets[0].player, instance->targets[0].card);
			}

			kill_card(player, card, KILL_DESTROY);
		}
	}

	return hybrid(player, card, event);
}

int card_springjack_pasture(int player, int card, event_t event)
{
  /* Springjack Pasture	""
   * Land
   * |T: Add |1 to your mana pool.
   * |4, |T: Put a 0/1 |Swhite Goat creature token onto the battlefield.
   * |T, Sacrifice X Goats: Add X mana of any one color to your mana pool. You gain X life. */

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION || event == EVENT_RESOLVE_SPELL
	  || (event == EVENT_COUNT_MANA && affect_me(player, card)) || (event == EVENT_CHANGE_TYPE && affect_me(player, card)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "Select a Goat to sacrifice.");
	  test.subtype = SUBTYPE_GOAT;

	  int can_common = !is_tapped(player, card) && !is_animated_and_sick(player, card);
	  int can_mana = can_common && can_produce_mana(player, card);
	  int num_goats = max_can_sacrifice_as_cost(player, card, &test);

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  play_land_sound_effect_force_color(player, card, num_goats ? COLOR_TEST_ANY : COLOR_TEST_COLORLESS);
		  return 0;
		}

	  if (event == EVENT_CHANGE_TYPE)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (num_goats)
			instance->info_slot |= (1<<30);	// bearing in mind that DIALOG uses byte0
		  else
			instance->info_slot &= ~(1<<30);
		  return 0;
		}

	  if (event == EVENT_COUNT_MANA)
		{
		  if (num_goats)
			declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, num_goats);
		  else if (can_mana)
			declare_mana_available(player, COLOR_TEST_COLORLESS, 1);
		  return 0;
		}

	  // EVENT_CAN_ACTIVATE/EVENT_ACTIVATE/EVENT_RESOLVE_ACTIVATION.

	  int can_make_goat = (can_common && !paying_mana() && can_use_activated_abilities(player, card)
						   && has_mana_for_activated_ability(player, card, MANACOST_X(4 + (can_mana ? MAX(num_goats, 1) : 0))));

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_MAKE_GOAT,
		CHOICE_SAC_GOATS,
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Produce 1", can_mana, paying_mana() ? 2 : -1,
						"Make a Goat", can_make_goat, 4,
						"Sacrifice Goats", can_mana, paying_mana() ? num_goats : (1 + num_goats) / 2);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  produce_mana_tapped(player, card, COLOR_COLORLESS, 1);
			  break;

			case CHOICE_MAKE_GOAT:
			  add_state(player, card, STATE_TAPPED);
			  if (!charge_mana_for_activated_ability(player, card, MANACOST_X(4)))
				remove_state(player, card, STATE_TAPPED);
			  break;

			case CHOICE_SAC_GOATS:
			  if (num_goats)	// and therefore can sacrifice at least one as cost
				{
				  char marked[2][151] = {{0}};

				  target_definition_t td;
				  default_target_definition(player, card, &td, TYPE_PERMANENT);
				  td.allowed_controller = td.preferred_controller = player;
				  if (check_battlefield_for_id(ANYBODY, CARD_ID_ANGEL_OF_JUBILATION))
					td.illegal_type = TYPE_CREATURE;
				  td.required_subtype = SUBTYPE_GOAT;
				  td.allow_cancel = 3;	//done and cancel buttons

				  int num_roasted = mark_up_to_n_targets_noload(&td, "Select a Goat to sacrifice.", -1, marked);
				  if (cancel == 1)
					return 0;

				  if (num_roasted > 0)
					{
					  int c;
					  for (c = 0; c < active_cards_count[player]; ++c)
						if (marked[player][c])
						  kill_card(player, c, KILL_SACRIFICE);

					  gain_life(player, num_roasted);
					}

				  FORCE(produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, num_roasted));
				}
			  else
				produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 0);
			  break;
		  }
	  else if (choice == CHOICE_MAKE_GOAT)	// and event == EVENT_RESOLVE_ACTIVATION
		generate_token_by_id(player, card, CARD_ID_GOAT);
	}

  return 0;
}

int card_stigma_lasher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	wither(player, card, event);

	if( instance->targets[2].player != 66 && damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int legacy = create_legacy_effect(player, card, &empty);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[3].player = 1-player;
		leg->targets[3].card = CARD_ID_STIGMA_LASHER;
		instance->targets[2].player = 66;
	}

	return 0;
}

int card_stillmoon_cavalier(int player, int card, event_t event){
	/*
	  Stillmoon Cavalier 1{W/B}{W/B}

	  Creature - Zombie Knight 2/1

	  Protection from white and from black

	  {W/B}: Stillmoon Cavalier gains flying until end of turn.

	  {W/B}: Stillmoon Cavalier gains first strike until end of turn.

	  {W/B}{W/B}: Stillmoon Cavalier gets +1/+0 until end of turn.
	*/
	card_instance_t *instance = get_card_instance( player, card);

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(1));
			return has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_WHITE, c1);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(1));
		int abilities[3] = {	has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_WHITE, c1),
								has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_WHITE, c1),
								has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_WHITE, c1)
		};
		int priorities[3] = {
							current_phase < PHASE_DECLARE_BLOCKERS && ! check_for_ability(player, card, KEYWORD_FLYING) ? 10 : 0,
							current_phase == PHASE_AFTER_BLOCKING && ! check_for_ability(player, card, KEYWORD_FIRST_STRIKE) ? 15 : 0,
							current_phase == PHASE_AFTER_BLOCKING ? 10 : 0,
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Gain flying", abilities[0], priorities[0],
							"Gain first strike", abilities[1], priorities[1],
							"+1/+0", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_hybrid(player, card, choice == 3 ? 2 : 1, COLOR_BLACK, COLOR_WHITE, c1) ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, KEYWORD_FLYING, 0);
		}
		if( instance->info_slot == 2 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
		if( instance->info_slot == 3 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 0);
		}
	}

	return 0;
}

int card_sturdy_hatchling(int player, int card, event_t event){

	/* Sturdy Hatchling	|3|GU
	 * Creature - Elemental 6/6
	 * ~ enters the battlefield with four -1/-1 counters on it.
	 * |GU: ~ gains shroud until end of turn.
	 * Whenever you cast a |Sgreen spell, remove a -1/-1 counter from ~.
	 * Whenever you cast a |Sblue spell, remove a -1/-1 counter from ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_U(1));
			return has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_GREEN, c1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_U(1));
		charge_mana_hybrid(player, card, 1, COLOR_BLUE, COLOR_GREEN, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, KEYWORD_SHROUD, 0);
	}

	return hatchling(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_BLUE);
}

int card_suture_spirit(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_DESTROYED;
	td.preferred_controller = player;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_REGENERATION, MANACOST0, 0, &td, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(3));
			if( has_mana_hybrid(player, 3, COLOR_BLACK, COLOR_WHITE, c1) ){
				return 0x63;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_B(3));
		charge_mana_hybrid(player, card, 3, COLOR_BLACK, COLOR_WHITE, c1);
		if( spell_fizzled != 1 ){
			new_pick_target(&td, "Select target creature to regenerate.", 0, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return 0;
}

int card_syphon_life(int player, int card, event_t event){
	/* Syphon Life	|1|B|B
	 * Sorcery
	 * Target player loses 2 life and you gain 2 life.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return retrace(player, card, event, CARD_ID_SYPHON_LIFE);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_talaras_battalion(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST && get_stormcolor_count(player, COLOR_GREEN) < 1 ){
		infinite_casting_cost();
	}

	return 0;
}

int card_thunderblust(int player, int card, event_t event){

	haste(player, card, event);

	persist(player, card, event);

	if( count_minus1_minus1_counters(player, card) > 0 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_TRAMPLE);
	}

	return 0;
}

int card_tilling_treefolk(int player, int card, event_t event)
{
  /* Tilling Treefolk	|2|G
   * Creature - Treefolk Druid 1/3
   * When ~ enters the battlefield, you may return up to two target land cards from your graveyard to your hand. */

  if (comes_into_play(player, card, event) && !graveyard_has_shroud(player))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select target land card.");
	  test.qty = 2;

	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_twilight_mire(int player, int card, event_t event){
	return card_filter_land(player, card, event, COLOR_BLACK, COLOR_GREEN, COLOR_TEST_BLACK, COLOR_TEST_GREEN, " Colorless\n B -> BB\n B -> BG\n B -> GG\n G -> BB\n G -> BG\n G -> GG\n Cancel");
}

int card_twinblade_slasher(int player, int card, event_t event){
	wither(player, card, event);
	return card_basking_rootwalla(player, card, event);
}

int card_umbra_stalker(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		int result = 0;
		int count = 0;
		const int *grave = get_grave(player);
		while( grave[count] != -1 ){
				result+=count_chroma(-1, grave[count], COLOR_BLACK, 0, 0);
				count++;
		}
		event_result+=result;
	}
	return 0;
}

int card_unmake(int player, int card, event_t event){

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( hybrid_casting(player, card, 0) ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_unnerving_assault(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		int c1 = get_updated_casting_cost(player, card, -1, event, 3);
		if( has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_RED, c1) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int c1 = get_updated_casting_cost(player, card, -1, event, 3);
			test_definition_t test;
			char mana_text[3][100];
			get_mana_from_text(&test, MANACOST_XU(c1, 1));
			strcpy(mana_text[0], test.message);
			get_mana_from_text(&test, MANACOST_XR(c1, 1));
			strcpy(mana_text[1], test.message);
			int can_play_full_effect = 0;
			if( c1 > 0 ){
				if( c1 > 1 ){
					get_mana_from_text(&test, MANACOST_XUR(c1-1, 1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_XUR(c1-1, 1, 1));
				}
				else{
					get_mana_from_text(&test, MANACOST_UR(1, 1));
					strcpy(mana_text[2], test.message);
					can_play_full_effect = has_mana_multi(player, MANACOST_UR(1, 1));
				}
			}
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							mana_text[0], has_mana_multi(player, MANACOST_XU(c1, 1)), 5,
							mana_text[1], has_mana_multi(player, MANACOST_XR(c1, 1)), 5,
							mana_text[2], can_play_full_effect, 10);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			charge_mana_multi(player, MANACOST_XUR(
													(c1 > 1 ? (choice != 3 ? c1 : c1-1) : (choice != 3 ? c1 : 0)),
													(choice == 1 || choice == 3),
													(choice == 2 || choice == 3)
													)
								);
		}
		if( spell_fizzled != 1 ){
			if( mana_paid[COLOR_BLUE] > 0 ){
				instance->info_slot |= 2;
			}
			if( mana_paid[COLOR_RED] > 0 ){
				instance->info_slot |= 4;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 2) ){
			pump_subtype_until_eot(player, card, 1-player, -1, -1, 0, 0, 0);
		}
		if( (instance->info_slot & 4) ){
			pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_unwilling_recruit(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			effect_act_of_treason_and_modify_pt_or_abilities(player, card, instance->targets[0].player, instance->targets[0].card,
															instance->info_slot, 0, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE", 1, NULL);
}

int card_voracious_hatchling(int player, int card, event_t event){
	/* Voracious Hatchling	|3|WB
	 * Creature - Elemental 6/6
	 * Lifelink
	 * ~ enters the battlefield with four -1/-1 counters on it.
	 * Whenever you cast a |Swhite spell, remove a -1/-1 counter from ~.
	 * Whenever you cast a |Sblack spell, remove a -1/-1 counter from ~. */
	lifelink(player, card, event);
	return hatchling(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_WHITE);
}

int card_wake_thrasher(int player, int card, event_t event)
{
  player_bits[player] |= PB_SEND_EVENT_UNTAP_CARD_TO_ALL;

  if (event == EVENT_UNTAP_CARD && affected_card_controller == player && !is_humiliated(player, card) && in_play(player, card))
	pump_until_eot_merge_previous(player, card, player, card, 1,1);

  return 0;
}

int card_ward_of_bones(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == 1-player && ! is_humiliated(player, card) ){
		int mod = 0;
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && count_subtype(player, TYPE_CREATURE, -1) < count_subtype(1-player, TYPE_CREATURE, -1) ){
			mod = 1;
			instance->targets[1].player = 66;
		}
		else{
			instance->targets[1].player = 0;
		}
		if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) && count_subtype(player, TYPE_ARTIFACT, -1) < count_subtype(1-player, TYPE_ARTIFACT, -1) ){
			mod = 1;
			instance->targets[2].player = 66;
		}
		else{
			instance->targets[2].player = 0;
		}
		if( is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) ){
			if( ! is_planeswalker(affected_card_controller, affected_card) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ENCHANTMENT);
				this_test.type_flag = F1_NO_PWALKER;
				if( check_battlefield_for_special_card(player, card, player, 4, &this_test) < check_battlefield_for_special_card(player, card, 1-player, 4, &this_test) ){
					mod = 1;
					instance->targets[3].player = 66;
				}
				else{
					instance->targets[3].player = 0;
				}
			}
		}
		if( mod == 1 ){
			infinite_casting_cost();
		}
	}

	return 0;
}

int card_wickerbough_elder(int player, int card, event_t event){

	/* Wickerbough Elder	|3|G
	 * Creature - Treefolk Shaman 4/4
	 * ~ enters the battlefield with a -1/-1 counter on it.
	 * |G, Remove a -1/-1 counter from ~: Destroy target artifact or enchantment. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 1);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_M1_M1) > 0 ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "DISENCHANT");
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_G(1));
		if( spell_fizzled != 1 ){
			if( pick_target(&td, "DISENCHANT") ){
				remove_counter(player, card, COUNTER_M1_M1);
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_wistful_selkie(int player, int card, event_t event){
	if(comes_into_play(player, card, event) > 0 ){
		draw_a_card(player);
	}
	return hybrid(player, card, event);
}

int card_woodlurker_mimic(int player, int card, event_t event){
	return mimic(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_BLACK, 4, 5, 0, SP_KEYWORD_WITHER);
}

int card_worm_harvest(int player, int card, event_t event){
	/* Worm Harvest	|2|BG|BG|BG
	 * Sorcery
	 * Put a 1/1 |Sblack and |Sgreen Worm creature token onto the battlefield for each land card in your graveyard.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;
		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		if( has_mana_hybrid(player, 3, COLOR_BLACK, COLOR_GREEN, cless) && can_sorcery_be_played(player, event) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test) ? GA_PLAYABLE_FROM_GRAVE : 0;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		charge_mana_hybrid(player, card, 3, COLOR_BLACK, COLOR_GREEN, cless);
		if( spell_fizzled != 1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WORM, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN | COLOR_TEST_BLACK;
		token.qty = count_graveyard_by_type(player, TYPE_LAND);
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}

	return hybrid(player, card, event);
}


int card_waves_of_aggression(int player, int card, event_t event){
	/* Waves of Aggression	|3|RW|RW
	 * Sorcery
	 * Untap all creatures that attacked this turn. After this main phase, there is an additional combat phase followed by an additional main phase.
	 * Retrace */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;
		int cless = get_updated_casting_cost(player, card, -1, event, 3);
		if( has_mana_hybrid(player, 2, COLOR_RED, COLOR_WHITE, cless) && can_sorcery_be_played(player, event) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test) ? GA_PLAYABLE_FROM_GRAVE : 0;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		charge_mana_hybrid(player, card, 2, COLOR_RED, COLOR_WHITE, cless);
		if( spell_fizzled != 1 ){
			char msg[100] = "Select a land card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL){
		relentless_assault_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return hybrid(player, card, event);
}
