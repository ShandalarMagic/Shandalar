#include "manalink.h"

static int al_lands(int player, int card, event_t event, int subtype, int untapped)
{
  if (event == EVENT_CAST_SPELL && affect_me(player, card) && IS_AI(player))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  if (untapped)
		{
		  test.state = STATE_TAPPED;
		  test.state_flag = DOESNT_MATCH;
		}
	  test.subtype = subtype;
	  test.not_me = 1;

	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		ai_modifier -= 512;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  play_land_sound_effect(player, card);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND,
								  get_hacked_land_text(-1, -1,
													   untapped ? "Select an untapped %s to sacrifice."
													   : "Select %a to sacrifice.",
													   subtype));
	  if (untapped)
		{
		  test.state = STATE_TAPPED;
		  test.state_flag = DOESNT_MATCH;
		}
	  test.subtype = subtype;

	  if (!new_sacrifice(player, card, player, SAC_NO_CANCEL, &test))
		kill_card(player, card, KILL_STATE_BASED_ACTION);

	  return 1;
	}

  return 0;
}

int card_multi_purpose_al_token(int player, int card, event_t event){
	// 66 --> Graveborn (Kjeldoran Dead)
	// 67 --> Survivor (Kjeldoran Home Guard)
	// 68 --> Hippo (Pheldagriff)
	// 69 --> Starfish (Spiny Starfish)

	if( get_special_infos(player, card) == 66 || get_special_infos(player, card) == 70){
		haste(player, card, event);
	}

	if( event == EVENT_SET_COLOR && affect_me(player, card) ){
		if( get_special_infos(player, card) == 66 || get_special_infos(player, card) == 70 ){
			event_result = COLOR_TEST_RED;
			event_result |= COLOR_TEST_BLACK;
		}
		else if( get_special_infos(player, card) == 67 ){
			event_result = COLOR_TEST_WHITE;
		}
		else if( get_special_infos(player, card) == 68 ){
			event_result = COLOR_TEST_GREEN;
		}
		else if( get_special_infos(player, card) == 69 ){
			event_result = COLOR_TEST_BLUE;
		}
	}
	else if( event == EVENT_POWER && affect_me(player, card) ){
		if( get_special_infos(player, card) == 68 ){
			event_result++;
		}
		else if( get_special_infos(player, card) == 66 || get_special_infos(player, card) == 70){
			event_result+=3;
		}
	}

	else if( get_special_infos(player, card) == 66 && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_SACRIFICE);
	}


	return 0;
}

void al_pitchspell(int player, int card, event_t event, int clr, int life_to_pay){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ANY);
	this_test.color = clr;
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_MODIFY_COST ){
		card_instance_t *instance= get_card_instance(player, card);
		if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) > 1 ){
			int good = 1;
			int c1 = get_updated_casting_cost(player, card, -1, event, 0);
			if( ! has_mana(player, COLOR_COLORLESS, c1) ){
				good = 0;
			}
			if( good && life_to_pay > 0 && ! can_pay_life(player, life_to_pay) ){
				good = 0;
			}
			if( good == 1 ){
				null_casting_cost(player, card);
				instance->info_slot = 1;
			}
			else{
				instance->info_slot = 0;
			}
		}
		else{
			instance->info_slot = 0;
		}
	}
}

int casting_al_pitchspell(int player, int card, event_t event, int clr, int life_to_pay){
	char buffer[100];
	int pos = scnprintf(buffer, 100, "Select a", buffer);
	int i;
	for(i=1; i<6; i++){
		if( clr == (1<<i) ){
			card_ptr_t* c = cards_ptr[ CARD_ID_BLACK+(i-1) ];
			pos += scnprintf(buffer + pos, 100-pos, " %s ", c->name);
			break;
		}
	}
	pos += scnprintf(buffer + pos, 100-pos, "card to exile.");

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = clr;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance= get_card_instance(player, card);

	int result = played_for_free(player, card);
	result |= is_token(player, card);
	int c1 = get_updated_casting_cost(player, card, -1, event, 0);
	if( result == 0 ){
		int choice = 0;
		if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) > 0 ){
			int good = 1;
			if( ! has_mana(player, COLOR_COLORLESS, c1) ){
				good = 0;
			}
			if( good && life_to_pay > 0 && ! can_pay_life(player, life_to_pay) ){
				good = 0;
			}
			if( good == 1 ){
				if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
					char buffer2[100];
					pos = scnprintf(buffer2, 100, " Exile a");
					for(i=1; i<6; i++){
						if( clr == (1<<i) ){
							card_ptr_t* c = cards_ptr[ CARD_ID_BLACK+(i-1) ];
							pos += scnprintf(buffer2 + pos, 100-pos, " %s ", c->name);
							break;
						}
					}
					pos += scnprintf(buffer2 + pos, 100-pos, "card");
					if( life_to_pay > 0 ){
						pos += scnprintf(buffer2 + pos, 100-pos, " and pay %d life\n", life_to_pay);
					}
					else{
						pos += scnprintf(buffer2 + pos, 100-pos, "\n", life_to_pay);
					}
					pos += scnprintf(buffer2 + pos, 100-pos, " Play normally\n Cancel");
					choice = do_dialog(player, player, card, -1, -1, buffer2, 0);
				}
			}
			else{
				choice = 1;
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, c1);
			if( spell_fizzled != 1 ){
				state_untargettable(player, card, 1);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_CMC, -1, &this_test);
				if( selected != -1 ){
					rfg_card_in_hand(player, selected);
					state_untargettable(player, card, 0);
					if( life_to_pay > 0 ){
						lose_life(player, 1);
					}
					return 1;
				}
				else{
					state_untargettable(player, card, 0);
					return 0;
				}
			}
		}
		else if( choice == 1 ){
			if( instance->info_slot == 1 ){
				charge_mana_from_id(player, card, event, get_id(player, card));
				if( spell_fizzled != 1 ){
					return 1;
				}
			}
			else{
				return 1;
			}
		}
		else{
			return 0;
		}
	}
	return result;
}

// Cards

static int ad_cantrip(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( upkeep_trigger(player, card, event) ){
		int ai_choice = 0;
		if( 1-player == AI && count_deck(1-player) < 5 ){
			ai_choice = 2;
		}
		else if( 1-player == AI && count_deck(1-player) < 10 ){
			ai_choice = 1;
		}

		int choice = do_dialog(instance->targets[1].player, player, card, -1, -1, " Draw 2 cards\n Draw 1 card\n Pass", 0);
		if(player == AI) choice = ai_choice;
		if( choice < 2 ){
			draw_cards(instance->targets[1].player, 2-choice);
		}

		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_arcane_denial(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		cantrip(player, card, 1);

		int legacy2 = create_legacy_effect(player, card, &ad_cantrip );
		card_instance_t *leg2 = get_card_instance(player, legacy2);
		leg2->targets[1].player = instance->targets[0].player;
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		return card_counterspell(player, card, event);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_ashnods_cylix(int player, int card, event_t event)
{
  // |3, |T: Target player looks at the top three cards of his or her library, puts one of them back on top of his or her library, then exiles the rest.
  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);

		int max = MIN(3, count_deck(instance->targets[0].player));
		if( max ){
			test_definition_t test;
			new_default_test_definition(&test, 0, "Select a card to put on top.");
			test.create_minideck = max;
			test.no_shuffle = 1;
			int selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_DECK, 1, AI_MAX_VALUE, -1, &test);
			int card_added = deck_ptr[instance->targets[0].player][selected];
			remove_card_from_deck(instance->targets[0].player, selected);
			max--;
			if( max ){
				int i;
				for(i=0; i<max; i++){
					rfg_top_card_of_deck(instance->targets[0].player);
				}
			}
			raw_put_iid_on_top_of_deck(instance->targets[0].player, card_added);
		}
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_PLAYER");
}

int card_balduvian_dead(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XR(2, 1), 0, NULL, NULL) ){
			return count_graveyard_by_type(player, TYPE_CREATURE) > 0;
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GRAVEBORN, &token);
		token.pow = 3;
		token.tou = 1;
		token.special_infos = 66;
		generate_token(&token);
	}

	return 0;
}

int card_balduvian_horde(int player, int card, event_t event){

	if( comes_into_play(player, card, event ) ){
		int did_discard = 0;
		if(hand_count[player] > 0){
			char buffer[50];
			load_text(0, "BALDUVIAN_HORDE");
			snprintf(buffer, 50, " %s\n %s", text_lines[0], text_lines[1]);
			int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
			if(choice == 0){
				discard(player, DISC_RANDOM, player);
				did_discard = 1;
			}
		}
		if(!did_discard){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_balduvian_trading_post(int player, int card, event_t event){

	if (al_lands(player, card, event, SUBTYPE_MOUNTAIN, TARGET_STATE_TAPPED)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;

	if (event == EVENT_ACTIVATE){
		int choice = 0;
		if (!paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(3)) && can_use_activated_abilities(player, card) && can_target(&td)){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Damage an attacker\n Cancel", 1);
		}

		card_instance_t* instance = get_card_instance(player, card);
		instance->info_slot = choice;

		if( choice == 0){
			produce_mana_tapped2(player, card, COLOR_COLORLESS, 1, COLOR_RED, 1);
		}

		if( choice == 1 ){
			tap_card(player, card);
			instance->number_of_targets = 0;
			if (!(charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && pick_target(&td, "TARGET_CREATURE"))){
				 untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->info_slot == 1 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			declare_mana_available(player, COLOR_COLORLESS, 1);
			declare_mana_available(player, COLOR_RED, 1);
		}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_balduvian_war_makers(int player, int card, event_t event)
{
  // 0x1202759

  /* Balduvian War-Makers	|4|R
   * Creature - Human Barbarian 3/3
   * Haste
   * Rampage 1 */

  haste(player, card, event);
  rampage(player, card, event, 1);
  return 0;
}

int card_bestial_fury(int player, int card, event_t event){

	/* Bestial Fury	|2|R
	 * Enchantment - Aura
	 * Enchant creature
	 * When ~ enters the battlefield, draw a card at the beginning of the next turn's upkeep.
	 * Whenever enchanted creature becomes blocked, it gets +4/+0 and gains trample until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL  ){
		if( valid_target(&td) ){
			cantrip(player, card, 1);
		}
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_ATTACK_RATING && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			ai_defensive_modifier -= 48;
		}
		if( event == EVENT_DECLARE_BLOCKERS && is_attacking(instance->damage_target_player, instance->damage_target_card) &&
			! is_unblocked(instance->damage_target_player, instance->damage_target_card)
		  ){
			pump_ability_until_eot(player, card, instance->damage_target_player, instance->damage_target_card, 4, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

static int effect_hunt(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		card_instance_t *instance = get_card_instance(player, card);
		remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_bounty_of_the_hunt(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		al_pitchspell(player, card, event, COLOR_TEST_GREEN, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( casting_al_pitchspell(player, card, event, COLOR_TEST_GREEN, 0) ){
				int x;
				for(x = 0; x < 3; x++){
					// Maybe the only creature on the bf is a Maro.
					if (can_target(&td1) && new_pick_target(&td1, "TARGET_CREATURE", x, 1)){
						add_state(instance->targets[x].player, instance->targets[x].card, STATE_TARGETTED);
					}
				}
				for(x = 0; x < 3; x++){
					if (instance->targets[x].player != -1 && instance->targets[x].card != -1){
						remove_state(instance->targets[x].player, instance->targets[x].card, STATE_TARGETTED);
					}
				}
			}
			else{
				spell_fizzled = 1;
			}
	}

	if( event == EVENT_RESOLVE_SPELL ){
			int x;
			for(x = 0; x < 3; x++){
				if( validate_target(player, card, &td1, x)){
					add_1_1_counter(instance->targets[x].player, instance->targets[x].card);
					create_targetted_legacy_effect(player, card, &effect_hunt, instance->targets[x].player, instance->targets[x].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_browse(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 2, 0, 2, 0, 0, 0) && can_use_activated_abilities(player, card) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE){
		charge_mana_for_activated_ability(player, card, 2, 0, 2, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = -1;
		int *deck = deck_ptr[player];
		while( selected == -1 ){
			selected = show_deck( player, deck, 5, "Pick a card", 0, 0x7375B0 );
		}
		add_card_to_hand(player, deck[selected]);
		remove_card_from_deck(player, selected);

		int i;
		for(i=0; i<4; i++ ){
			rfg_top_card_of_deck(player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_chaos_harlequin(int player, int card, event_t event)
{
  // |R: Exile the top card of your library. If that card is a land card, ~ gets -4/-0 until end of turn. Otherwise, ~ gets +2/+0 until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pp = instance->parent_controller;
	  int pc = instance->parent_card;
	  if (in_play(pp, pc))
		{
		  int is_land = deck_ptr[pp][0] != -1 && is_what(-1, deck_ptr[pp][0], TYPE_LAND);

		  if (deck_ptr[pp][0] != -1)
			rfg_top_card_of_deck(pp);

		  if (is_land)
			alternate_legacy_text(1, pp, pump_until_eot_merge_previous(pp, pc, pp, pc, -4,0));
		  else
			alternate_legacy_text(2, pp, pump_until_eot_merge_previous(pp, pc, pp, pc, 2,0));
		}
	  else if (deck_ptr[pp][0] != -1)
		rfg_top_card_of_deck(pp);
	}

  if (event == EVENT_POW_BOOST)
	return has_mana(player, COLOR_RED, 1);	// Assume it gets +1/+0 on average.

  return generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
}

int card_contagion(int player, int card, event_t event){

	/* Contagion	|3|B|B
	 * Instant
	 * You may pay 1 life and exile a |Sblack card from your hand rather than pay ~'s mana cost.
	 * Distribute two -2/-1 counters among one or two target creatures. */

	al_pitchspell(player, card, event, COLOR_TEST_BLACK, 1);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLACK, 1) ){
				int x;
				for(x = 0; x < 2; x++){
					new_pick_target(&td1, "TARGET_CREATURE", x, 0);
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int x;
			for(x = 0; x < 2; x++){
				if( validate_target(player, card, &td1, x)){
					add_counter(instance->targets[x].player, instance->targets[x].card, COUNTER_M2_M1);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_death_spark(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && has_mana(player, COLOR_COLORLESS, 1) ){
		int choice = do_dialog(player, player, card, -1, -1," Return Death Spark to your hand\n Pass", 0);
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1){
				instance->state &= ~STATE_INVISIBLE;
				hand_count[player]++;
				return -1;
			}
		}
		return -2;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_dimishing_returns(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hand_count[player] + count_graveyard(player) + count_deck(player) < 10 ){
			ai_modifier-=200;
		}
		else{
			ai_modifier+=(140-(hand_count[player]*20));
			ai_modifier+=((count_subtype(player, TYPE_LAND, -1)*10)-60);
			ai_modifier+=((hand_count[1-player]*20)-140);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					int count = active_cards_count[p]-1;
					while( count > -1 ){
							if( in_hand(p, count) ){
								put_on_top_of_deck(p, count);
							}
							count--;
					}
					reshuffle_grave_into_deck(p, 0);
				};
		);
		int x;
		for(x = 0; x < 10; x++){
			rfg_top_card_of_deck(player);
		}
		APNAP(p,{draw_some_cards_if_you_want(player, card, p, 7);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_diseased_vermin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	// Whenever ~ deals combat damage to a player, put an infection counter on it.
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		add_counter(player, card, COUNTER_INFECTION);
	}

	// At the beginning of your upkeep, ~ deals X damage to target opponent previously dealt damage by it, where X is the number of infection counters on it.
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, count_counters(player, card, COUNTER_INFECTION), player, card);
		}
	}

	return 0;
}

int card_elvish_spirit_guide(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
		kill_card(player, card, KILL_REMOVE);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		produce_mana(player, COLOR_GREEN, 1);
	}
	return 0;
}

int card_energy_arc(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_state = TARGET_STATE_ATTACKING;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		while( can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		if( trgs > 0 ){
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			instance->info_slot = trgs;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				untap_card(instance->targets[i].player, instance->targets[i].card);
				maze_of_ith_effect(player, card, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_exile(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_WHITE;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(player, get_toughness(instance->targets[0].player, instance->targets[0].card));
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_fatal_lore(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int ai_choice = 1;
		if( count_subtype(1-player, TYPE_CREATURE, -1) == 1 || count_subtype(1-player, TYPE_CREATURE, -1) == 2 ){
			ai_choice = 0;
		}
		int choice = do_dialog(1-player, player, card, -1, -1, " Opponent draws 3 cards\n Opp. kill creatures and you draw", ai_choice);
		if( choice == 0 ){
			draw_cards(player, 3);
		}
		else{
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
			td.illegal_abilities = 0;

			card_instance_t *instance = get_card_instance(player, card);

			int count = 0;
			while( can_target(&td) && count < 2 ){
					if( new_pick_target(&td, "Select a creature your opponent controls.", count, GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[count].player, instance->targets[count].card, 1);
						count++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_BURY);
			}
			instance->number_of_targets = 0;
			draw_some_cards_if_you_want(player, card, 1-player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_feast_or_famine(int player, int card, event_t event){
	/* Feast or Famine	|3|B
	 * Instant
	 * Choose one -
	 * * Put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * * Destroy target nonartifact, non|Sblack creature. It can't be regenerated. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 2;
	td.preferred_controller = 1-player;
	td.illegal_color = COLOR_TEST_BLACK;
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			int ai_choice = 0;
			if( can_target(&td) ){
				ai_choice = 1;
				choice = do_dialog(player, player, card, -1, -1," Generate a Zombie\n Kill a creature\n Cancel", ai_choice);
			}
			instance->info_slot = 66+choice;
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else if( choice == 1 ){
					pick_target(&td, "TARGET_CREATURE");
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 if( instance->info_slot == 67 && valid_target(&td) ){
				 kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			 }
			 else if( instance->info_slot == 66){
					  generate_token_by_id(player, card, CARD_ID_ZOMBIE);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_force_of_will(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	al_pitchspell(player, card, event, COLOR_TEST_BLUE, 1);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLUE, 1) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_gargantuan_gorilla(int player, int card, event_t event){
	/* Gargantuan Gorilla	|4|G|G|G
	 * Creature - Ape 7/7
	 * At the beginning of your upkeep, you may sacrifice |Ha Forest. If you sacrifice a snow |H2Forest this way, ~ gains trample until end of turn. If you don't sacrifice |Ha Forest, sacrifice ~ and it deals 7 damage to you.
	 * |T: ~ deals damage equal to its power to another target creature. That creature deals damage equal to its power to ~. */


	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.allowed_controller = player;
		td1.preferred_controller = player;
		td1.required_subtype = SUBTYPE_FOREST;

		instance->number_of_targets = 0;

		int kill = 1;
		if( can_target(&td1) && new_pick_target(&td1, "Select a Forest to sacrifice.", 0, GS_LITERAL_PROMPT) ){
			if( is_snow_permanent(player, instance->targets[0].card) ){
				pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_TRAMPLE, 0);
			}
			kill_card(player, instance->targets[0].card, KILL_SACRIFICE);
			kill = 0;
		}
		if( kill > 0 ){
			damage_player(player, 7, player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			// Not a fight.  Stop replacing this with fight().
			get_card_instance(instance->parent_controller, instance->parent_card)->regen_status |= KEYWORD_RECALC_POWER;
			int mypow = get_power(instance->parent_controller, instance->parent_card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, mypow, instance->parent_controller, instance->parent_card);
			if (in_play(instance->parent_controller, instance->parent_card)){
				get_card_instance(instance->targets[0].player, instance->targets[0].card)->regen_status |= KEYWORD_RECALC_POWER;
				int hispow = get_power(instance->targets[0].player, instance->targets[0].card);
				damage_creature(instance->parent_controller, instance->parent_card, hispow, instance->targets[0].player, instance->targets[0].card);
			}
       }
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_gorilla_chieftain(int player, int card, event_t event){
	return regeneration(player, card, event, 1, 0, 0, 1, 0, 0);
}

static int target_for_gorilla_shaman(int player, int card, int mode){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance= get_card_instance(player, card);

	int i;
	int trg = -1;
	int par = -1;
	for(i=0; i<2; i++){
		if( i == 1-player || player == HUMAN ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && ! is_what(i, count, TYPE_CREATURE) ){
						int cost = (get_cmc(i, count)*2)+1;
						if( has_mana_for_activated_ability(player, card, MANACOST_X(cost)) ){
							instance->targets[0].player = i;
							instance->targets[0].card = count;
							instance->number_of_targets = 1;
							if( would_validate_target(player, card, &td, 0) ){
								if( mode == 0 || player == HUMAN ){
									return 1;
								}
								if( mode == 1 && get_base_value(i, count) > par ){
									par = get_base_value(i, count);
									trg = count;
								}
							}
						}
					}
					count++;
			}
		}
	}
	if( mode == 1 ){
		return trg;
	}
	return 0;
}

int card_gorilla_shaman(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return target_for_gorilla_shaman(player, card, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[0].player = -1;
		instance->targets[0].card = -1;
		instance->number_of_targets = 0;
		if( player == HUMAN ){
			if( select_target(player, card, &td, "Select a non-creature artifact", NULL) ){
				int cost = (2*get_cmc( instance->targets[0].player, instance->targets[0].card ))+1;
				if( ! has_mana_for_activated_ability(player, card, MANACOST_X(cost)) ){
					spell_fizzled = 1;
					return 0;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		else if( player == AI ){
				int result = target_for_gorilla_shaman(player, card, 1);
				if( result != -1 ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = result;
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
		}
		if( spell_fizzled != 1 ){
			int cost = (2*get_cmc( instance->targets[0].player, instance->targets[0].card ))+1;
			charge_mana_for_activated_ability(player, card, cost, 0, 0, 0, 0, 0);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}
	return 0;
}

int card_gorilla_war_cry(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && current_turn == player && current_phase == PHASE_BEFORE_BLOCKING ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_ATTACKING;
			add_legacy_effect_to_all(player, card, &fx_menace, current_turn, &this_test);
			cantrip(player, card, 1);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// guerrilla tactics --> shock

int card_heart_of_yavimaya(int player, int card, event_t event){

	if (al_lands(player, card, event, SUBTYPE_FOREST, 0)){
		return 0;
	}

	if( (event == EVENT_COUNT_MANA && affect_me(player, card)) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_ACTIVATE){
		instance->info_slot = instance->number_of_targets = 0;
		int choice = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE") ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump a creature\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if (charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE")){
				instance->info_slot = 1;
				tap_card(player, card);
			}
		}

		if( choice == 2){
			spell_fizzled = 1;
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 1 && valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return 0;
}

int card_helm_of_obedience(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 35) ){ return 0; }

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		special_mill(instance->parent_controller, instance->parent_card, CARD_ID_HELM_OF_OBEDIENCE, 1-player, instance->info_slot);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_ONLY_TARGET_OPPONENT, -1, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_hail_storm(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		new_damage_all(player, card, current_turn, 2, 0, &this_test);
		new_damage_all(player, card, player, 1, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_inheritance(int player, int card, event_t event){

	if(event == EVENT_GRAVEYARD_FROM_PLAY){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && affected->kill_code > 0 &&
			affected->kill_code < KILL_REMOVE) {
			card_instance_t* instance = get_card_instance(player, card);
		   if( instance->targets[11].player < 0 ){
			  instance->targets[11].player = 0;
		   }
		   instance->targets[11].player++;
		}
	}

	if( resolve_graveyard_trigger(player, card, event) ){
		card_instance_t* instance = get_card_instance(player, card);
		int num = instance->targets[11].player;
		instance->targets[11].player = 0;
		int i = 0;
		for (i = 0; i < num && has_mana(player, COLOR_COLORLESS, 3); ++i){
			charge_mana(player, COLOR_COLORLESS, 3);
			if( spell_fizzled != 1){
				draw_a_card(player);
			} else {
				break;
			}
		}
	}

	return global_enchantment(player, card, event);
}

static int skip_draw_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;

	if( p > -1 && current_turn == p && event == EVENT_DRAW_PHASE ){
		event_result-=99;
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static int effect_gargoyle(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p > -1 ){
		if( instance->targets[1].player > -1 ){
			skip_your_draw_step(p, event);
		}

		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affect_me(p, c) && ! is_token(p, c) ){
				card_instance_t *affected = get_card_instance(p, c);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
					instance->targets[11].player = 66;
					instance->targets[1].player = p;
					if( is_stolen(p, c) ){
						instance->targets[1].player = 1-p;
					}
				}
				if( affected->kill_code == KILL_REMOVE ){
					kill_card(player, card, KILL_REMOVE);
				}
			}
		}

		if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){
			instance->targets[2].player = 66;
			instance->targets[11].player = 0;
		}

		if( instance->targets[2].player == 66 && eot_trigger(player, card, event) ){
			int result = seek_grave_for_id_to_reanimate(player, card, instance->targets[1].player, instance->targets[1].card, REANIMATE_DEFAULT);
			int legacy = create_legacy_effect(instance->targets[1].player, result, &skip_draw_legacy);
			card_instance_t *leg= get_card_instance(instance->targets[1].player, legacy);
			leg->targets[0].player = p;
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}


int ivory_gargoyle_impl(int player, int card, event_t event, int x, int b, int u, int g, int r, int w){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && ! is_token(player, card) ){
		int legacy = create_my_legacy(player, card, &effect_gargoyle);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].card = get_id(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		kill_card(player, instance->parent_card, KILL_REMOVE);
	}

	return generic_activated_ability(player, card, event, 0, x, b, u, g, r, w, 0, 0, 0);
}

int card_ivory_gargoyle(int player, int card, event_t event)
{
	return ivory_gargoyle_impl(player, card, event, MANACOST_XW(4,1));
}

int card_juniper_order_advocate(int player, int card, event_t event){

	if( ! is_tapped(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
		affected_card_controller == player ){
		if( get_color(affected_card_controller, affected_card) & COLOR_TEST_GREEN ){
			event_result++;
		}
	}

	return 0;
}

int card_kaysa(int player, int card, event_t event)
{
  /* Kaysa	|3|G|G
   * Legendary Creature - Elf Druid 2/3
   * |SGreen creatures you control get +1/+1. */

  check_legend_rule(player, card, event);

  if ((event == EVENT_TOUGHNESS || event == EVENT_POWER)
	  && affected_card_controller == player
	  && (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_GREEN))
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && !is_humiliated(player, card))
	++event_result;

  return 0;
}

int card_keeper_of_tresserhorn(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && is_unblocked(player, card) ){
		lose_life(1-player, 2);
		negate_combat_damage_this_turn(player, card, player, card, 0);
	}
	return 0;
}

int card_kjeldoran_home_guard(int player, int card, event_t event)
{
  /* Kjeldoran Home Guard	|3|W
   * Creature - Human Soldier 1/6
   * At end of combat, if ~ attacked or blocked this combat, put a -0/-1 counter on ~ and put a 0/1 |Swhite Deserter creature token onto the battlefield. */

  if (trigger_condition == TRIGGER_END_COMBAT
	  && (get_card_instance(player, card)->state & (STATE_ATTACKING|STATE_ATTACKED|STATE_BLOCKING|STATE_BLOCKED))
	  && end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
		add_counter(player, card, COUNTER_M0_M1);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DESERTER, &token);
		token.pow = 0;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);

	}

  return 0;
}

int card_kjeldoran_outpost(int player, int card, event_t event){
	/* Kjeldoran Outpost	""
	 * Land
	 * If ~ would enter the battlefield, sacrifice |Ha Plains instead. If you do, put ~ onto the battlefield. If you don't, put it into its owner's graveyard.
	 * |T: Add |W to your mana pool.
	 * |1|W, |T: Put a 1/1 |Swhite Soldier creature token onto the battlefield. */

	if (al_lands(player, card, event, SUBTYPE_PLAINS, 0)){
		return 0;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if (!paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XW(2,1))){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Generate a Soldier\n Cancel", 1);
		}

		card_instance_t* instance = get_card_instance(player, card);
		instance->info_slot = choice;

		if (choice == 0){
			return mana_producer(player, card, event);
		}
		if (choice == 1){
			tap_card(player, card);
			if (!charge_mana_for_activated_ability(player, card, MANACOST_XW(1,1))){
				untap_card_no_event(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->info_slot == 1 ){
			generate_token_by_id(player, card, CARD_ID_SOLDIER);
		}
	}
	else {
		return mana_producer(player, card, event);
	}

	return 0;
}

int krovikan_horror(int player, int card, event_t event){
  int cnt = count_graveyard_by_id(player, CARD_ID_KROVIKAN_HORROR);

  if( trigger_condition == TRIGGER_EOT && affect_me(player, card) && cnt > 0){
	 if(event == EVENT_TRIGGER){
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	 }
	 else if(event == EVENT_RESOLVE_TRIGGER){
		const int *graveyard = get_grave(player);
		int count = count_graveyard(player)-1;
		int activate = -1;
		int horrors = 0;
		int position[3];
		while( count > -1){
			  if( cards_data[  graveyard[count] ].id == CARD_ID_KROVIKAN_HORROR ){
				 position[horrors] = count;
				 horrors++;
			  }
			  count--;
		}

		if( horrors == 1 && (cards_data[ graveyard[position[0]+1] ].type & TYPE_CREATURE) ){
		   activate = position[0];
		}
		else if(horrors > 1){
				int k;
				for(k = 0; k < horrors; k++){
					if( cards_data[ graveyard[position[k]+1] ].type & TYPE_CREATURE){
					   activate = position[k];
					}
				}
		}

		if( activate > -1 ){
		   int choice = 0;
		   if( ! duh_mode(player) ){
			do_dialog(player, player, card, -1, -1," Return Krovikan Horror\n Do not return Krovikan Horror\n", 0);
		   }
		   if( choice == 0 ){
			  add_card_to_hand(player, graveyard[activate]);
			  remove_card_from_grave(player, activate);
		   }
		}
	 }
   }
  return 0;
}

int card_krovikan_horror(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

int card_lake_of_the_dead(int player, int card, event_t event)
{
  if (al_lands(player, card, event, SUBTYPE_SWAMP, 0))
	return 0;

#define CAN_SAC can_sacrifice_as_cost(player, 1, TYPE_LAND, MATCH, get_hacked_subtype(player, card, SUBTYPE_SWAMP), MATCH, 0,0,0,0,-1,0)

  if (event == EVENT_COUNT_MANA)
	{
	  if (affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card))
		declare_mana_available(player, COLOR_BLACK, CAN_SAC ? 4 : 1);
	}
  else if (event == EVENT_ACTIVATE)
	{
	  int choice = 1;

	  if (CAN_SAC)
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_NO_STORAGE,
						"Generate B", 1, 1,
						"Sacrifice a Swamp for BBBB", 1, 1);

	  if (choice == 1)
		produce_mana_tapped(player, card, COLOR_BLACK, 1);
	  else if (choice == 2 && sacrifice(player, card, player, 0, TYPE_LAND, MATCH, get_hacked_subtype(player, card, SUBTYPE_SWAMP), MATCH, 0,0,0,0,-1,0))
		produce_mana_tapped(player, card, COLOR_BLACK, 4);
	  else
		spell_fizzled = 1;
	}
  else
	return mana_producer(player, card, event);

  return 0;
#undef CAN_SAC
}

int card_lat_nam_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		   target_definition_t td;
		   base_target_definition(player, card, &td, TYPE_NONE);
		   td.allowed_controller = player;
		   td.preferred_controller = player;
		   td.zone = TARGET_ZONE_HAND;

		   if( pick_target(&td, "TARGET_CARD") ){
			   shuffle_into_library(instance->targets[0].player, instance->targets[0].card);
			   cantrip(player, card, 2);
		   }

		   kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_library_of_lat_nam(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
			int ai_choice = 0;
			int choice = do_dialog(1-player, player, card, -1, -1, " Cantrip for 3\n Demonic Tutor", ai_choice);
			if( choice == 0 ){
				cantrip(player, card, 3);
			}
			else{
				test_definition_t this_test;
				char buffer[100] = "Select a card to tutor.";
				new_default_test_definition(&this_test, TYPE_ANY, buffer);
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_lim_duls_paladin(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		char buffer[100] = "Select a card to discard.";
		new_default_test_definition(&this_test, TYPE_ANY, buffer);

		int kill = 1;
		if( hand_count[player] ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				kill = 0;
			}
		}
		if( kill ){
			draw_cards(player, 1);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) ){
		if( is_unblocked(player, card) ){
			lose_life(1-player, 4);
			negate_combat_damage_this_turn(player, card, player, card, 0);
		}
		else{
			pump_until_eot(player, card, player, card, 6, 3);
		}
	}
	return 0;
}

int card_lim_dul_vault(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		int max = 5;
		while( max > 0 && deck[max-1] == -1 ){
				max--;
		}
		if( max > 0 ){
			if( player == AI ){
				rearrange_top_x(player, player, max);
			}
			else{
				while( 1 ){
						show_deck( player, deck_ptr[player], max, "Here are the top 5 cards of your library", 0, 0x7375B0 );
						int choice = 0;
						if( can_pay_life(player, 1) ){
							choice = do_dialog(player, player, card, -1, -1, " Rearrange\n Pay 1 life and continue", 0);
						}
						if(choice == 0){
							rearrange_top_x(player, player, max);
							break;
						}
						else{
							lose_life(player, 1);
							int i;
							for(i = 0; i < max; i++){
								deck[count_deck(player)] = deck[0];
								remove_card_from_deck(player, 0);
							}
						}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_lodestone_bauble(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select up to 4 basic land cards.");
	this_test.subtype = SUBTYPE_BASIC;

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		(new_special_count_grave(player, &this_test) > 0 || new_special_count_grave(1-player, &this_test) > 0) &&
		can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		return ! graveyard_has_shroud(2);
	}

	else if( event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && pick_target(&td, "TARGET_PLAYER")){
			select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, 4, &instance->targets[1]);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		int i, num_validated = 0, num_targets = 0;
		for (i = 1; i <= 4; ++i){
			if (instance->targets[i].player != -1){
				++num_targets;
				int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
				if (selected != -1){
					from_graveyard_to_deck(instance->targets[0].player, selected, 1);
					++num_validated;
				}
			}
		}
		if (num_validated > 1){
			rearrange_top_x(player, instance->targets[0].player, num_validated);
		}
		if (num_validated == 0 && num_targets > 0){
			spell_fizzled = 1;
		} else {
			int result = cantrip(player, card, 1);
			card_instance_t *leg = get_card_instance(player, result);
			leg->targets[1].player = instance->targets[0].player;
		}
	}

	return 0;
}

int card_lord_of_tresserhorn(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance(player, card);

  if( comes_into_play(player, card, event) ){
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.illegal_abilities = 0;
	  td.allow_cancel = 0;

	  int total = 0;

	  while( can_target(&td) && total < 2 ){
			 pick_target(&td, "LORD_OF_THE_PIT");
			 if( valid_target(&td) ){
				 kill_card(player, instance->targets[0].card, KILL_SACRIFICE);
				 total++;
			 }
	  }
	  lose_life(player, 2);
	  draw_cards(1-player, 2);
  }

  return card_drudge_skeletons(player, card, event);
}

int card_misfortune(int player, int card, event_t event){

	/* Misfortune	|1|B|R|G
	 * Sorcery
	 * An opponent chooses one - You put a +1/+1 counter on each creature you control and gain 4 life; or you put a -1/-1 counter on each creature that player
	 * controls and ~ deals 4 damage to him or her. */

	if( event == EVENT_RESOLVE_SPELL ){
			int ai_choice = 0;
			if( life[1-player]-4 > 5 && !check_battlefield_for_subtype(1-player, TYPE_CREATURE, -1) ){
				ai_choice = 1;
			}
			int choice = do_dialog(1-player, player, card, -1, -1, " Opponent is lucky\n You're unlucky", ai_choice);
			if( choice == 0 ){
				manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
				gain_life(player, 4);
			}
			else{
				manipulate_type(player, card, 1-player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_M1_M1, 1));
				damage_player(1-player, 4, player, card);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int effect_nb(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance(player, card);

  if( event == EVENT_ABILITIES && affect_me(instance->targets[0].player, instance->targets[0].card) ){
	  event_result |= instance->targets[1].card;
  }

  return 0;
}

int card_natures_blessing(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int keyword = 0;
			int ai_choice = 2;
			if( check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_TRAMPLE) ){
				ai_choice = 1;
			}
			if( check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FIRST_STRIKE) ){
				ai_choice = 0;
			}
			int choice = do_dialog(player, player, card, -1, -1," Add +1/+1 counter\n Give First Strike\n Give Trample\n Give Banding", ai_choice);
			if( choice == 0 ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
			else  if( choice == 1 ){
				keyword = KEYWORD_FIRST_STRIKE;
			}
			else  if( choice == 2 ){
				keyword = KEYWORD_TRAMPLE;
			}
			else  if( choice == 3 ){
				keyword = KEYWORD_BANDING;
			}

			if( keyword > 0 ){
				int leg = create_targetted_legacy_effect(player, card, &effect_nb, instance->targets[0].player, instance->targets[0].card);
				card_instance_t *tok = get_card_instance(player, leg);
				tok->targets[1].card = keyword;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_GW(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_natures_chosen(int player, int card, event_t event){
	/*
	  Nature's Chosen |G
	  Enchantment - Aura
	  Enchant creature you control
	  {0}: Untap enchanted creature. Activate this ability only during your turn and only once each turn.
	  Tap enchanted creature: Untap target artifact, creature, or land. Activate this ability only if enchanted creature is white and
	  is untapped and only once each turn.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	if( ! IS_AURA_EVENT(player, card, event) && ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
		td1.preferred_controller = player;

		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		enum{
			CHOICE_UNTAP_ENCHANTED_CRIT = 1,
			CHOICE_UNTAP_PERMANENT
		};

		if( event == EVENT_CAN_ACTIVATE ){
			int abil1 = (instance->targets[1].player > -1 && !(instance->targets[1].player & 1)) ? 1 : 0;
			if( abil1 && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
				return 1;
			}
			int abil2 = (instance->targets[1].player > -1 && !(instance->targets[1].player & 2)) ? 1 : 0;
			if( abil2 && ! is_tapped(p, c) && (get_color(player, card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE)) &&
				generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL)
			  ){
				return 1;
			}
		}
		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = 0;
			int abils[2] = {0, 0};
			int abil1 = (instance->targets[1].player > -1 && !(instance->targets[1].player & CHOICE_UNTAP_ENCHANTED_CRIT)) ? 1 : 0;
			if( abil1 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL) ){
				abils[0] = 1;
			}
			int abil2 = (instance->targets[1].player > -1 && !(instance->targets[1].player & CHOICE_UNTAP_PERMANENT)) ? 1 : 0;
			if( abil2 && ! is_tapped(p, c) && (get_color(player, card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE)) &&
				generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL)
			  ){
				abils[1] = 1;
			}
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Untap enchanted creature", abils[0], is_tapped(p, c) ? 10 : 0,
							"Untap permanent", abils[1], 10);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
				instance->info_slot = choice;
				if( choice == CHOICE_UNTAP_ENCHANTED_CRIT ){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player |= CHOICE_UNTAP_ENCHANTED_CRIT;
				}
				if( choice == CHOICE_UNTAP_PERMANENT ){
					if( new_pick_target(&td1, "Select target artifact, creature, or land.", 0, 1 | GS_LITERAL_PROMPT) ){
						tap_card(p, c);
						if( instance->targets[1].player < 0 ){
							instance->targets[1].player = 0;
						}
						instance->targets[1].player |= CHOICE_UNTAP_PERMANENT;
					}
				}
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == CHOICE_UNTAP_ENCHANTED_CRIT ){
				untap_card(p, c);
			}
			if( instance->info_slot == CHOICE_UNTAP_PERMANENT && valid_target(&td1) ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_noble_steeds(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}
	else if( event == EVENT_CAN_CAST ){
			return 1;
	}
	return vanilla_creature_pumper(player, card, event, 1, 0, 0, 0, 0, 1, 0, 0, 0, KEYWORD_FIRST_STRIKE, 0, &td);
}

int card_phantasmal_fiend(int player, int card, event_t event)
{
  card_instance_t *instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  // |B: ~ gets +1/-1 until end of turn.
	  // |1|U: Switch ~'s power and toughness until end of turn.
	  enum
	  {
		CHOICE_PUMP = 1,
		CHOICE_SWITCH
	  } choice = DIALOG(player, card, event,
						"+1/-1", 1, 1, DLG_MANA(MANACOST_B(1)),
						"Switch power and toughness", 1, 2, DLG_MANA(MANACOST_XU(1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice && can_use_activated_abilities(player, card);
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_PUMP:
			  alternate_legacy_text(1, player, pump_until_eot(player, card, instance->parent_controller, instance->parent_card, 1, -1));
			  break;

			case CHOICE_SWITCH:
			  alternate_legacy_text(2, player, switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card));
			  break;
		  }
	}

	return 0;
}

int card_phelddagrif(int player, int card, event_t event){
	/* Phelddagrif	|1|G|W|U
	 * Legendary Creature - Phelddagrif 4/4
	 * |G: ~ gains trample until end of turn. Target opponent puts a 1/1 |Sgreen Hippo creature token onto the battlefield.
	 * |W: ~ gains flying until end of turn. Target opponent gains 2 life.
	 * |U: Return ~ to its owner's hand. Target opponent may draw a card. */

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum{
		CHOICE_TRAMPLE = 1,
		CHOICE_FLYING,
		CHOICE_BOUNCE
	};

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, NULL) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_G(1)) ||
				has_mana_for_activated_ability(player, card, MANACOST_W(1)) ||
				has_mana_for_activated_ability(player, card, MANACOST_U(1))
			  ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		int priorities[3] = {	is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING && ! is_unblocked(player, card) ? 10 : 1,
								current_phase < PHASE_DECLARE_BLOCKERS ? 10 : 1,
								5
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Trample", has_mana_for_activated_ability(player, card, MANACOST_G(1)), priorities[0],
						"Flying", has_mana_for_activated_ability(player, card, MANACOST_W(1)), priorities[1],
						"Bounce", has_mana_for_activated_ability(player, card, MANACOST_U(1)), priorities[2]);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, 0,
															0,
															(choice == CHOICE_BOUNCE ? 1 : 0),
															(choice == CHOICE_TRAMPLE ? 1 : 0),
															0,
															(choice == CHOICE_FLYING ? 1 : 0))
			){
				instance->info_slot = choice;
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->info_slot == CHOICE_TRAMPLE ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
										0, 0, KEYWORD_TRAMPLE, 0);
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_HIPPO, &token);
				token.t_player = instance->targets[0].player;
				token.pow = token.tou = 1;
				token.color_forced = COLOR_TEST_GREEN;
				generate_token(&token);
			}

			if( instance->info_slot == CHOICE_FLYING ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
										0, 0, KEYWORD_FLYING, 0);
				gain_life(instance->targets[0].player, 2);
			}

			if( instance->info_slot == CHOICE_BOUNCE ){
				draw_some_cards_if_you_want(instance->parent_controller, instance->parent_card, instance->targets[0].player, 1);
				if( in_play(instance->parent_controller, instance->parent_card) ){
					bounce_permanent(instance->parent_controller, instance->parent_card);
				}
			}
		}
	}

	return 0;
}

int card_phyrexian_boon(int player, int card, event_t event)
{
  /* Phyrexian Boon	|2|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+1 as long as it's |Sblack. Otherwise, it gets -1/-2. */

  if (event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_SHOULD_AI_PLAY)
	{
	  card_instance_t* instance;
	  if (!(instance = in_play(player, card))
		  || instance->damage_target_player < 0
		  || (event != EVENT_SHOULD_AI_PLAY && !affect_me(instance->damage_target_player, instance->damage_target_card)))
		return 0;

	  int black = get_color(instance->damage_target_player, instance->damage_target_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
	  switch (event)
		{
		  case EVENT_SHOULD_AI_PLAY:
			ai_modifier += (!black == !(instance->damage_target_player == AI)) ? 24 : -24;
			break;

		  case EVENT_POWER:
			event_result += black ? 2 : -1;
			break;

		  case EVENT_TOUGHNESS:
			event_result += black ? 1 : -2;
			break;

		  default:
			break;
		}
	}

  return vanilla_aura(player, card, event, ANYBODY);
}

int card_phyrexian_devourer(int player, int card, event_t event){

	/* Phyrexian Devourer	|6
	 * Artifact Creature - Construct 1/1
	 * When ~'s power is 7 or greater, sacrifice it.
	 * Exile the top card of your library: Put X +1/+1 counters on ~, where X is the exiled card's converted mana cost. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_STATIC_EFFECTS && get_power(player, card) >= 7 ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	if( event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE0(player, card) && deck_ptr[player][0] != -1 ){
		return 1;
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int *deck = deck_ptr[player];
			int amount = get_cmc_by_id(cards_data[deck[0]].id);

			show_deck( player, deck, 1, "You will remove this card", 0, 0x7375B0 );

			rfg_top_card_of_deck(player);
			instance->targets[0].card = amount;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = instance->targets[0].card;
		if( amount > 0 ){
			add_1_1_counters(player, instance->parent_card, amount);
		}
	}

	return 0;
}

int card_phyrexian_war_beast(int player, int card, event_t event){

	if( leaves_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		damage_player(player, 1, player, card);
	}

	return 0;
}

int card_pillage(int player, int card, event_t event){
/*
Pillage |1|R|R
Sorcery
Destroy target artifact or land. It can't be regenerated.
*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target target artifact or land.", 1, NULL);
}

int card_primitive_justice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_ARTIFACT")  ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			int trgs = 1;
			while( can_target(&td) && (has_mana_multi(player, MANACOST_XR(1, 1)) || has_mana_multi(player, MANACOST_XG(1, 1))) ){
					int choice = 0;
					if( has_mana_multi(player, MANACOST_XR(1, 1)) ){
						if( has_mana_multi(player, MANACOST_XG(1, 1)) ){
							choice = do_dialog(player, player, card, -1, -1, " Continue (pay 1R)\n  Continue (pay 1G)\n Stop", 1);
						}
					}
					else{
						choice = 1;
					}
					if( choice == 0 ){
						charge_mana_multi(player, MANACOST_XR(1, 1));
						if( spell_fizzled != 1 && new_pick_target(&td, "TARGET_ARTIFACT", trgs, 0) ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
							trgs++;
						}
						else{
							break;
						}
					}
					else if( choice == 1 ){
							charge_mana_multi(player, MANACOST_XG(1, 1));
							if( spell_fizzled != 1 && new_pick_target(&td, "TARGET_ARTIFACT", trgs, 0) ){
								state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
								instance->info_slot |= (1<<trgs);
								trgs++;
							}
							else{
								break;
							}
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				if( instance->info_slot & (1<<i) ){
					gain_life(player, 1);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_pyrokinesis(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	al_pitchspell(player, card, event, COLOR_TEST_RED, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( casting_al_pitchspell(player, card, event, COLOR_TEST_RED, 0) ){
				int x;
				for(x = 0; x < 4; x++){
					new_pick_target(&td1, "TARGET_CREATURE", x, 0);
				}
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		   divide_damage(player, card, &td1);
		   kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reinforcements(int player, int card, event_t event){
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select up to 3 creature cards.");

	if (event == EVENT_CAN_CAST){
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
	}
	else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		if (count_graveyard_by_type(player, TYPE_CREATURE) <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
		}

		card_instance_t* instance = get_card_instance(player, card);
		select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, 3, &instance->targets[0]);
	}
	else if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < 3; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_graveyard_to_deck(player, selected, 1);
				++num_validated;
			}
		}
		if (num_validated > 1){
			rearrange_top_x(player, player, num_validated);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reprisal(int player, int card, event_t event){

	/* Reprisal	|1|W
	 * Instant
	 * Destroy target creature with power 4 or greater. It can't be regenerated. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
}

int card_ritual_of_the_machine(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;
	td1.allow_cancel = 0;
	td1.illegal_color = COLOR_TEST_BLACK;
	td1.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
			pick_target(&td1, "TARGET_CREATURE");
		}
		else{
			 spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td1) ){
				gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
			 }
			 kill_card(player, card, KILL_DESTROY);
   }

	return 0;
}

int card_rogue_skycaptain(int player, int card, event_t event){

	/* At the beginning of your upkeep, put a wage counter on ~. You may pay |2 for each wage counter on it. If you don't, remove all wage counters from ~ and
	 * an opponent gains control of it. */
	if( current_turn == player && !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int swap = 0;
				while( count > 0 ){
						add_counter(player, card, COUNTER_WAGE);
						swap = 1;
						int amount = 2*count_counters(player, card, COUNTER_WAGE);
						if( has_mana(player, COLOR_COLORLESS, amount) ){
							int choice = do_dialog(player, player, card, -1, -1, " Pay Skycaptain's wager\n Pass", 0);
							if( choice == 0 ){
								charge_mana(player, COLOR_COLORLESS, amount);
								if( spell_fizzled != 1 ){
									swap = 0;
								}
							}
						}
						if( swap == 1 ){
							break;
						}
						count--;
				}
				if( swap == 1 ){
					remove_all_counters(player, card, COUNTER_WAGE);
					give_control(player, card, player, card);
				}
		}
	}

	return 0;
}

int effect_scars_of_the_veteran(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].card > 0 && event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage_card == damage->internal_card_id && damage->damage_target_player == instance->targets[0].player &&
			damage->damage_target_card == instance->targets[0].card &&
			damage->info_slot > 0
		  ){
			int amount = damage->info_slot;
			int prevention = instance->targets[1].card;
			if( amount <= prevention ){
				amount = 0;
				prevention-=damage->info_slot;
			}
			else{
				prevention = 0;
				amount-= instance->targets[1].card;
			}
			instance->targets[1].card = prevention;
			damage->info_slot = amount;
		}
	}

	if( eot_trigger(player, card, event) ){
		if( instance->targets[0].card != -1 ){
			int counters = 7 - instance->targets[1].card;
			if( counters > 0 ){
				add_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_P0_P1, counters);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}


int card_scars_of_the_veteran(int player, int card, event_t event){

	/* You may exile a |Swhite card from your hand rather than pay ~'s mana cost.
	 * Prevent the next 7 damage that would be dealt to target creature or player this turn. At the beginning of the next end step, put a +0/+1 counter on that
	 * creature for each 1 damage prevented this way. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	al_pitchspell(player, card, event, COLOR_TEST_WHITE, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( casting_al_pitchspell(player, card, event, COLOR_TEST_WHITE, 0) ){
				pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
			}
			else{
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				 int leg = create_targetted_legacy_effect(player, card, &effect_scars_of_the_veteran, instance->targets[0].player, instance->targets[0].card);
				 card_instance_t *tok = get_card_instance(player, leg);
				 tok->targets[0].player = instance->targets[0].player;
				 tok->targets[0].card = instance->targets[0].card;
				 tok->targets[1].card = 7;
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sheltered_valley(int player, int card, event_t event){

	/* Sheltered Valley	""
	 * Land
	 * If ~ would enter the battlefield, instead sacrifice each other permanent named ~ you control, then put ~ onto the battlefield.
	 * At the beginning of your upkeep, if you control three or fewer lands, you gain 1 life.
	 * |T: Add |1 to your mana pool. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.id = get_id(player, card);
		this_test.not_me = 1;
		new_manipulate_all(player, card, player, &this_test, KILL_SACRIFICE);
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(player, TYPE_LAND, -1) < 4 ){
			gain_life(player, 1);
		}
	}

	return mana_producer(player, card, event);
}

int card_shield_sphere(int player, int card, event_t event){

	/* Shield Sphere	|0
	 * Artifact Creature - Wall 0/6
	 * Defender
	 * Whenever ~ blocks, put a -0/-1 counter on it. */

	if( current_turn == 1-player && blocking(player, card, event) ){
		add_counter(player, card, COUNTER_M0_M1);
	}

	return 0;
}

int card_sol_grail(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int ai_choice = 0;
		if( player == AI ){
			ai_choice = get_deck_color(player, player);
		}
		card_instance_t *instance = get_card_instance( player, card );
		instance->info_slot = 1<<choose_a_color(player, ai_choice);
	}

	return mana_producer(player, card, event);
}

int card_soldevi_digger(int player, int card, event_t event){


	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && count_graveyard(player) > 0 ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int top_grave = count_graveyard(player)-1;
		int *deck = deck_ptr[player];
		const int *grave = get_grave(player);
		deck[count_deck(player)] = grave[top_grave];
		remove_card_from_grave(player, top_grave);
	}

	return 0;
}

int card_soldevi_excavations(int player, int card, event_t event){

	if (al_lands(player, card, event, SUBTYPE_ISLAND, TARGET_STATE_TAPPED)){
		return 0;
	}

	if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			declare_mana_available(player, COLOR_COLORLESS, 1);
			declare_mana_available(player, COLOR_BLUE, 1);
		}
	} else if (event == EVENT_ACTIVATE){
		int choice = 0;
		if (!paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(3))){
			choice = do_dialog(player, player, card, -1, -1, " Add 1U\n Scry 1 \n Cancel", 0);
		}

		card_instance_t* instance = get_card_instance(player, card);
		instance->info_slot = choice;

		if( choice == 0 ){
			produce_mana_tapped2(player, card, COLOR_COLORLESS, 1, COLOR_BLUE, 1);
		} else if (choice == 1){
			tap_card(player, card);
			if (!charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)){
				 untap_card_no_event(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	} else if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot == 1){
			scrylike_effect(player, player, 1);
		}
	} else {
	  return mana_producer(player, card, event);
	}

	return 0;
}

int card_soldier_of_fortune(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			shuffle(instance->targets[0].player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_PLAYER");
}

static int spiny_starfish_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( eot_trigger(player, card, event) ){
			if( ! is_humiliated(instance->damage_target_player, instance->damage_target_card) &&
				! check_state(instance->damage_target_player, instance->damage_target_card, STATE_OUBLIETTED)
			  ){
				token_generation_t token;
				default_token_definition(instance->damage_target_player, instance->damage_target_card, CARD_ID_STARFISH, &token);
				token.pow = 0;
				token.tou = 1;
				token.color_forced = COLOR_TEST_BLUE;
				generate_token(&token);
			}
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_spiny_starfish(int player, int card, event_t event){
	/* Spiny Starfish	|2|U
	 * Creature - Starfish 0/1
	 * |U: Regenerate ~.
	 * At the beginning of each end step, if ~ regenerated this turn, put a 0/1 |Sblue Starfish creature token onto the battlefield for each time it regenerated this turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return regeneration(player, card, event, MANACOST_U(1));
	}

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card) ){
		regenerate_target(instance->parent_controller, instance->parent_card);
		create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &spiny_starfish_legacy,
										instance->parent_controller, instance->parent_card);
	}

	return 0;
}

int card_splinter_token(int player, int card, event_t event)
{
  cumulative_upkeep(player, card, event, MANACOST_G(1));

  card_instance_t* instance;
  if (trigger_condition == TRIGGER_LEAVE_PLAY
	  && (instance = get_card_instance(player, card))
	  && instance->damage_source_player >= 0 && instance->damage_source_card >= 0 && in_play(instance->damage_source_player, instance->damage_source_card)
	  && leaves_play(player, card, event))
	new_damage_all(player, card, instance->damage_source_player, 1, NDA_PLAYER_TOO, NULL);
  return 0;
}
int card_splintering_wind(int player, int card, event_t event)
{
  /* Splintering Wind	|2|G|G
   * Enchantment
   * |2|G: ~ deals 1 damage to target creature. Put a 1/1 |Sgreen Splinter creature token onto the battlefield. It has flying and "Cumulative upkeep |G." When
   * it leaves the battlefield, it deals 1 damage to you and each creature you control. */

	if( !IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		damage_target0(player, card, 1);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPLINTER_TOKEN, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN;
		token.key_plus = KEYWORD_FLYING;
		token.action = TOKEN_ACTION_DONT_COPY_TOKEN_SOURCE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(2,1), 0, &td, "TARGET_CREATURE");
}

int card_surge_of_strength(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	char msg[100] = "Select a green or red card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);
	this_test.color = COLOR_TEST_GREEN | COLOR_TEST_RED;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST &&  generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL) ){
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, cmc, 0, KEYWORD_TRAMPLE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ali_from_cairo(int player, int card, event_t event);
int card_sustaining_spirit(int player, int card, event_t event)
{
  /* Sustaining Spirit	|1|W
   * Creature - Angel Spirit 0/3
   * Cumulative upkeep |1|W
   * Damage that would reduce your life total to less than 1 reduces it to 1 instead. */

  cumulative_upkeep(player, card, event, MANACOST_XW(1,1));

  return card_ali_from_cairo(player, card, event);
}

int card_swamp_mosquito(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS ){
		if( is_unblocked(player, card) && ! is_humiliated(player, card) ){
			poison_counters[1-player]++;
		}
	}
	return 0;
}

int card_taste_of_paradise(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	multikicker(player, card, event, 1, 0, 0, 1, 0, 0);

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, (instance->info_slot+1)*3);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_thawing_glaciers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && affect_me(player, card) && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && ! is_tapped(player, card) ){
			return 1;
		}
	}

	else if( event == EVENT_ACTIVATE && affect_me(player, card) ){
			if( player == AI ){
				ai_modifier+=100;
			}
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1){
				tap_card(player, card);
				instance->info_slot = 66;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 tutor_basic_land(player, 1, 1);
	}

	else if( instance->info_slot == 66 && eot_trigger(player, card, event) ){
			bounce_permanent(player, card);
	}

	return 0;
}

static int can_rfg_top_n_cards_of_deck(int player, int card, int number_of_age_counters)
{
  return count_deck(player) >= number_of_age_counters;
}
static int rfg_top_n_cards_of_deck_thunk(int player, int card, int number_of_age_counters)
{
  rfg_top_n_cards_of_deck(player, number_of_age_counters);
  return 1;
}
int card_thought_lash(int player, int card, event_t event)
{
	/* Cumulative upkeep-Exile the top card of your library.
	 * When a player doesn't pay ~'s cumulative upkeep, that player exiles all cards from his or her library.
	 * Exile the top card of your library: Prevent the next 1 damage that would be dealt to you this turn. */

	if (global_enchantment(player, card, event)){
		return 1;
	}

	if (cumulative_upkeep_general(player, card, event, can_rfg_top_n_cards_of_deck, rfg_top_n_cards_of_deck_thunk)){
		rfg_whole_library(player);
	}

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.required_type = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_target(&td) && count_deck(player) > 0 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 99;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_DAMAGE") ){
				instance->number_of_targets = 1;
				rfg_top_card_of_deck(player);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > 0 ){
				target->info_slot--;
			}
	}

	return 0;
}

int card_tornado(int player, int card, event_t event)
{
  /* Tornado	|4|G
   * Enchantment
   * Cumulative upkeep |G
   * |2|G, Pay 3 life for each velocity counter on ~: Destroy target permanent and put a velocity counter on ~. Activate this ability only once each turn. */

  if (global_enchantment(player, card, event))
	return 1;

  cumulative_upkeep(player, card, event, MANACOST_G(1));

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	  add_counter(player, card, COUNTER_VELOCITY);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_ONCE_PER_TURN, MANACOST_XG(2,1),
								   3 * count_counters(player, card, COUNTER_VELOCITY), &td, "TARGET_PERMANENT");
}

int card_unlikely_alliance(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_IN_COMBAT;
	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}
	else if( event == EVENT_CAN_CAST ){
			return 1;
	}
	return vanilla_creature_pumper(player, card, event, 1, 0, 0, 0, 0, 1, 0, 0, 2, 0, 0, &td);
}

static int rtrue(int player, int card, int number_of_age_counters)
{
  return 1;
}

static int generate_survivors(int player, int card, int number_of_age_counters)
{
	token_generation_t token;
	default_token_definition(player, card, CARD_ID_SURVIVOR, &token);
	token.t_player = 1-player;
	token.pow = token.tou = 1;
	token.color_forced = COLOR_TEST_RED;
	token.qty = number_of_age_counters;
	generate_token(&token);
	return 1;
}
int card_varchilds_war_riders(int player, int card, event_t event)
{
  /* Varchild's War-Riders	|1|R
   * Creature - Human Warrior 3/4
   * Cumulative upkeep-Put a 1/1 |Sred Survivor creature token onto the battlefield under an opponent's control.
   * Trample; rampage 1 */

  cumulative_upkeep_general(player, card, event, rtrue, generate_survivors);

  rampage(player, card, event, 1);

  return 0;
}

int card_whip_vine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( current_phase == PHASE_AFTER_BLOCKING && instance->blocking < 255 ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = instance->blocking;
				instance->number_of_targets = 1;
				return would_valid_target(&td);
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = instance->blocking;
			instance->number_of_targets = 1;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			does_not_untap_until_im_tapped(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( current_phase == PHASE_UNTAP && current_turn == player && event == EVENT_UNTAP && affect_me(player, card) ){
		int ai_choice = 0;
		if( instance->targets[0].player > -1 &&
			(! in_play(instance->targets[0].player, instance->targets[0].card) || ! is_tapped(instance->targets[0].player, instance->targets[0].card))
		  ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Leave tapped\n Untap", ai_choice);
		if( choice == 0 ){
			instance->untap_status &= ~3;
		}
	}

	return 0;
}

int card_wild_aesthir(int player, int card, event_t event)
{
  // 0x1202070

  /* Wild Aesthir	|2|W
   * Creature - Bird 1/1
   * Flying, first strike
   * |W|W: ~ gets +2/+0 until end of turn. Activate this ability only once each turn. */

#define GAA(event)	generic_activated_ability(player, card, (event), GAA_ONCE_PER_TURN, MANACOST_W(2), 0, NULL, NULL)

  if (event == EVENT_POW_BOOST && GAA(EVENT_CAN_ACTIVATE))
	return 2;

  if (!IS_GAA_EVENT(event))
	return 0;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c))
		pump_until_eot(p, c, p, c, 2,0);
	}

  return GAA(event);
#undef GAA
}

int card_whirling_catapult(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_deck(player) > 1 ){
		return has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			rfg_top_card_of_deck(player);
			rfg_top_card_of_deck(player);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, ANYBODY, 1, NDA_PLAYER_TOO, &this_test);
	}

	return 0;
}

int card_yavimaya_ants(int player, int card, event_t event){

	/* Yavimaya Ants	|2|G|G
	 * Creature - Insect 5/1
	 * Trample, haste
	 * Cumulative upkeep |G|G */

	haste(player, card, event);

	cumulative_upkeep(player, card, event, 0, 0, 0, 2, 0, 0);

	return 0;
}

