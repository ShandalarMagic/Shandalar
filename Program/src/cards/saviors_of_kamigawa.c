#include "manalink.h"

// Global functions
static int sweep(int player, int card, int subtype){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_subtype = subtype;
	td.allow_cancel = 2; //'Done' button

	card_instance_t *instance = get_card_instance(player, card);

	int amount = 0;
	int targetted[100];
	int tc = 0;

	while( can_target(&td) && tc < 100 ){
			instance->number_of_targets = 0;
			if( new_pick_target(&td, get_hacked_land_text(player, card, "Select a %s to bounce.", subtype), 0, GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				targetted[tc] = instance->targets[0].card;
				tc++;
			}
	}
	instance->number_of_targets = 0;

	int i;
	for(i=0; i<tc; i++){
		bounce_permanent(player, targetted[i]);
		amount++;
	}

	return amount;
}

int epic_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
		if(! (card_d->type & TYPE_LAND) ){
			infinite_casting_cost();
		}
	}

	if( player == current_turn && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int trig = 1;
		if( player == AI && instance->targets[1].card == -1 &&
			(instance->targets[2].card == CARD_ID_ENDURING_IDEAL || instance->targets[2].card == CARD_ID_ETERNAL_DOMINION)
		  ){
			trig = 0;
		}
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( instance->targets[2].card == CARD_ID_ENDURING_IDEAL ){
						int result = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
						instance->targets[1].card = result;
					}
					if( instance->targets[2].card == CARD_ID_ENDLESS_SWARM && hand_count[player] > 0 ){
						generate_tokens_by_id(player, card, CARD_ID_SNAKE, hand_count[player]);
					}
					if( instance->targets[2].card == CARD_ID_ETERNAL_DOMINION ){
						target_definition_t td;
						default_target_definition(player, card, &td, TYPE_CREATURE);
						td.zone = TARGET_ZONE_PLAYERS;

						instance->targets[0].player = 1-player;
						if( valid_target(&td) ){
							int result = global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, TYPE_PERMANENT, F1_NO_PWALKER, 0, 0, 0, 0, 0, 0, -1, 0);
							instance->targets[1].card = result;
						}
					}
					if( instance->targets[2].card == CARD_ID_NEVERENDING_TORMENT ){
						target_definition_t td;
						default_target_definition(player, card, &td, TYPE_CREATURE);
						td.zone = TARGET_ZONE_PLAYERS;

						if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
							int i;
							for(i=0; i<hand_count[player]; i++){
								global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
							}
						}
					}
					if( instance->targets[2].card == CARD_ID_UNDYING_FLAMES ){
						target_definition_t td;
						default_target_definition(player, card, &td, TYPE_CREATURE);
						td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

						if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							int *deck = deck_ptr[player];
							while( deck[0] != -1 ){
									show_deck(HUMAN, deck, 1, "Player revealed this", 0, 0x7375B0 );
									if( ! is_what(-1, deck[0], TYPE_LAND) ){
										int amount = get_cmc_by_id( cards_data[deck[0]].id);
										rfg_top_card_of_deck(player);
										damage_creature_or_player(player, card, event, amount);
										break;
									}
									else{
										rfg_top_card_of_deck(player);
									}
							}
						}
					}
			}
		}
	}
	return 0;
}



// cards
int card_adamaro_first_to_desire(int player, int card, event_t event){
	/* Adamaro, First to Desire	|1|R|R
	 * Legendary Creature - Spirit 100/100
	 * ~'s power and toughness are each equal to the number of cards in the hand of the opponent with the most cards in hand. */

	check_legend_rule(player, card, event);

	if (player != -1){
		modify_pt_and_abilities(player, card, event, hand_count[1-player], hand_count[1-player], 0);
	}

	return 0;
}

int card_akki_drillmaster(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	// td.required_state = TARGET_STATE_SUMMONING_SICK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_akuta_born_from_ashes(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && hand_count[player] > hand_count[1-player] &&
		check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) && ! check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE)
	  ){
		int choice = do_dialog(player, player, card, -1, -1," Return Akuta to play\n Pass\n", 0);
		if( choice == 0 && sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_SWAMP, 0, 0, 0, 0, 0, -1, 0) ){
		   put_into_play(player, card);
		   return -1;
		}
		else{
			return -2;
		}
	}

	return 0;
}

// araba mothrider --> devoted retainer

int card_arashi_the_sky_asunder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, instance->parent_card);
		}
	}

	else if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 0, 0, 0, 2, 0, 0) ){
				return 1;
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, -1, 0, 0, 2, 0, 0);
			if( spell_fizzled != 1 ){
				instance->targets[0].player = x_value;
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_ARASHI_THE_SKY_ASUNDER));
			card_instance_t* added_inst = get_card_instance(player, card_added);
			added_inst->state |= STATE_INVISIBLE;	--hand_count[player];

			damage_all(player, card_added, player, instance->targets[0].player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			damage_all(player, card_added, 1-player, instance->targets[0].player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0, 0, 0, 0, -1, 0);

			obliterate_card(player, card_added);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, -1, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_aether_shockwave(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.illegal_state = TARGET_STATE_TAPPED;
		td.required_subtype = SUBTYPE_SPIRIT;
		int my_count_spirit = target_available(player, card, &td);

		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
		int his_count_spirit = target_available(player, card, &td);

		int ai_choice = 1;
		if( my_count_spirit > his_count_spirit ){
			ai_choice = 0;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Tap all Spirits\n Tap all non-Spirit creatures", ai_choice);
		int type = TYPE_PERMANENT;
		int subtype = SUBTYPE_SPIRIT;
		int mode = 0;
		if( choice == 1 ){
			type = TYPE_CREATURE;
			subtype = SUBTYPE_SPIRIT;
			mode = 1;
		}
		manipulate_all(player, card, player, type, 0, subtype, mode, 0, 0, 0, 0, -1, 0, ACT_TAP);
		manipulate_all(player, card, 1-player, type, 0, subtype, mode, 0, 0, 0, 0, -1, 0, ACT_TAP);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// ayumi the last vistor --> livonya silone

// blood clock --> umbilicus

int card_bounteous_kirin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		gain_life(player, get_cmc(instance->targets[1].player, instance->targets[1].card));
	}

	return 0;
}

// captive flame --> Ghitu war cry

int card_celestial_kirin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		int cmc = get_cmc(instance->targets[1].player, instance->targets[1].card);
		manipulate_all(player, card, player, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, cmc, 0, KILL_DESTROY);
		manipulate_all(player, card, 1-player, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, cmc, 0, KILL_DESTROY);
	}

	return 0;
}

int card_charge_across_the_araba(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = sweep(player, card, SUBTYPE_PLAINS);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, instance->info_slot, instance->info_slot, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_choice_of_damnation(int player, int card, event_t event){
	/*
	  Choice of Damnations |5|B
	  Sorcery - Arcane
	  Target opponent chooses a number. You may have that player lose that much life. If you don't, that player sacrifices all but that many permanents.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 0;
			if( instance->targets[0].player != AI ){
				amount = choose_a_number(instance->targets[0].player, "Choose a number.", life[player]);
			}
			else{
				amount = count_subtype(instance->targets[0].player, TYPE_PERMANENT, -1);
				while( amount >= life[instance->targets[0].player] && amount > 0){
						amount--;
				}
			}
			char buffer[100];
			snprintf(buffer, 100, "Opponent chooses %d.", amount );
			do_dialog(player, player, card, -1, -1, buffer, 0);

			int ai_choice = 0;
			int total_permanents_of_opponent = count_subtype(instance->targets[0].player, TYPE_PERMANENT, -1);
			if( life[instance->targets[0].player] - amount >= 6 && total_permanents_of_opponent-amount < 4 ){
				ai_choice = 1;
			}
			snprintf(buffer, 100, " Opponent loses %d life\n Opponent sacrifices all but %d permanents", amount, amount );
			int choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			if( choice == 0 ){
				lose_life(instance->targets[0].player, amount);
			}
			else{
				int to_sac = total_permanents_of_opponent-amount;
				if( to_sac > 0 ){
					impose_sacrifice(player, card, instance->targets[0].player, to_sac, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_cloudhoof_kirin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( can_target(&td) && arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			mill(instance->targets[0].player, get_cmc(instance->targets[1].player, instance->targets[1].card));
		}
	}

	return 0;
}

ARCANE(card_death_denied)
{
  if (event == EVENT_CAN_CAST)
	return !check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
	  int generic_mana_available = has_mana(player, COLOR_ANY, 1);	// A useful trick from card_fireball().
	  max_cards = MIN(max_cards, generic_mana_available);
	  max_cards = MIN(max_cards, 19);

	  if (max_cards <= 0 || graveyard_has_shroud(player))
		{
		  ai_modifier -= 48;
		  instance->info_slot = 0;
		  return 0;
		}

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  if (ai_is_speculating != 1)
		{
		  if (max_cards == 1)
			strcpy(test.message, "Select a creature card.");
		  else
			sprintf(test.message, "Select up to %d creature cards.", max_cards);
		}

	  target_t targets[19];
	  int num_chosen = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &test, max_cards, &targets[0]);

	  SET_BYTE0(instance->info_slot) = num_chosen;

	  if (num_chosen == 0)
		return 0;

	  charge_mana(player, COLOR_COLORLESS, instance->info_slot);
	  if (cancel == 1)
		return 0;

	  SET_BYTE1(instance->info_slot) = player;
	  int leg = create_targetted_legacy_effect(player, card, &empty, player, card);
	  if (leg == -1)
		SET_BYTE2(instance->info_slot) = 255;
	  else
		{
		  SET_BYTE2(instance->info_slot) = leg;
		  card_instance_t* legacy = get_card_instance(player, leg);
		  legacy->token_status |= STATUS_INVISIBLE_FX;
		  ASSERT(sizeof(legacy->targets) == sizeof(targets));
		  memcpy(legacy->targets, targets, sizeof(targets));
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int num_chosen = BYTE0(instance->info_slot);
	  if (num_chosen == 0)
		return 0;

	  int orig_player = BYTE1(instance->info_slot);
	  int leg = BYTE2(instance->info_slot);
	  if (leg == 255)
		return 0;

	  int i;
	  for (i = 0; i < num_chosen; ++i)
		{
		  int selected = validate_target_from_grave(orig_player, leg, player, i);
		  if (selected != -1)
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

  return 0;
}

ARCANE(card_death_of_a_thousand_stings){
	// Target player loses 1 life and you gain 1 life.
	// At the beginning of your upkeep, if you have more cards in hand than each opponent, you may return ~ from your graveyard to your hand.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				lose_life(instance->targets[0].player, 1);
				gain_life(player, 1);
			}
	}
	else if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && hand_count[player] > hand_count[1-player] ){
			int choice = do_dialog(player, player, card, -1, -1," Return Death of a Thousand Stings\n Pass", 0);
			if( choice == 0 ){
				return -1;
			}
			else{
				return -2;
			}
	}
	return 0;
}

ARCANE(card_gaze_of_adamaro)
{
  /* Gaze of Adamaro	|2|R|R
   * Instant - Arcane
   * ~ deals damage equal to the number of cards in target player's hand to that player. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL && valid_target(&td))
	damage_target0(player, card, hand_count[get_card_instance(player, card)->targets[0].player]);

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_descendant_of_kiyomaro(int player, int card, event_t event){
	if( hand_count[player] > hand_count[1-player] ){
		modify_pt_and_abilities(player, card, event, 1, 2, 0);
		if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE) ){
			gain_life(player, 3);
		}
	}
	return 0;
}

int card_descendant_of_masumaro(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_1_1_counters(player, card, hand_count[player]);
		int amount = hand_count[1-player];
		if( amount > count_1_1_counters(player, card) ){
			amount = count_1_1_counters(player, card);
		}
		remove_1_1_counters(player, card, amount);
	}
	return 0;
}

int card_descendant_of_soramaro(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0) &&
		hand_count[player] > 0
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( hand_count[player] > 0 ){
			rearrange_top_x(player, player, hand_count[player]);
		}
	}

	return 0;
}

int card_ebony_owl_netsuke(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[current_turn] > 6 ){
			damage_player(current_turn, 4, player, card);
		}
	}
	return 0;
}

int card_eiganjo_free_riders(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.required_color = COLOR_TEST_WHITE;
	td.illegal_abilities = 0;

	return bounce_permanent_at_upkeep(player, card, event, &td);
}


int card_elder_pine_of_jukai(int player, int card, event_t event){

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		int *deck = deck_ptr[player];
		int amount = 3;
		if( count_deck(player) < amount ){
			amount = count_deck(player);
		}
		show_deck( HUMAN, deck, amount, "Here's the revealed cards", 0, 0x7375B0 );
		int i;
		int found = 0;
		for(i=0; i<amount; i++){
			if( is_what(-1, deck[i], TYPE_LAND) ){
				add_card_to_hand(player, deck[i]);
				remove_card_from_deck(player, i);
				found++;
			}
		}
		amount-=found;
		if( amount > 0 ){
			put_top_x_on_bottom(player, player, amount);
		}
	}

	return soulshift(player, card, event, 2, 1);
}

int card_endless_swarm(int player, int card, event_t event){
	/* Endless Swarm	|5|G|G|G
	 * Sorcery
	 * Put a 1/1 |Sgreen Snake creature token onto the battlefield for each card in your hand.
	 * Epic */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_SNAKE, hand_count[player]);
			int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//2
			card_instance_t *this = get_card_instance(player, card_added);
			this->targets[2].player = 2;
			this->targets[2].card = get_id(player, card);
			create_card_name_legacy(player, card_added, get_id(player, card));
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_enduring_ideal(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//2
			card_instance_t *this = get_card_instance(player, card_added);
			this->targets[2].player = 2;
			this->targets[2].card = get_id(player, card);
			create_card_name_legacy(player, card_added, get_id(player, card));
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_erayo_soratami_ascendant(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		if( get_storm_count() == 4 ){
			instance->targets[2].player = 4;
			true_transform(player, card);
		}
	}

	return 0;
}

int card_erayos_essence(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		instance->targets[2].player++;
		if( instance->targets[2].player == 1 ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return 0;
}

int card_eternal_dominion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		return would_valid_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)  ){
			instance->targets[0].player = 1-player;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, TYPE_PERMANENT, F1_NO_PWALKER, 0, 0, 0, 0, 0, 0, -1, 0);
				int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//2
				card_instance_t *this = get_card_instance(player, card_added);
				this->targets[2].player = 2;
				this->targets[2].card = get_id(player, card);
				create_card_name_legacy(player, card_added, get_id(player, card));
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int card_evermind_impl(int player, int card, event_t event)
{
  // Draw a card.
  // Splice onto Arcane |1|U
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	draw_cards(player, 1);

  return 0;
}
int card_evermind(int player, int card, event_t event)
{
  // Nonexistent mana costs can't be paid
  if (event == EVENT_CAN_CAST && !played_for_free(player, card))
	return 0;
  return arcane_with_splice(player, card, event, card_evermind_impl, MANACOST_XU(1,1));
}

ARCANE(card_exile_into_darkness){
	// Target player sacrifices a creature with converted mana cost 3 or less.
	// At the beginning of your upkeep, if you have more cards in hand than each opponent, you may return ~ from your graveyard to your hand.
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 4, 3);
			}
	}
	else if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && hand_count[player] > hand_count[1-player] ){
			int choice = do_dialog(player, player, card, -1, -1," Return Exile into Darkness\n Pass", 0);
			if( choice == 0 ){
				return -1;
			}
			return -2;
	}
	return 0;
}

int card_feral_lightning(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
			token.qty = 3;
			token.pow = 3;
			token.tou = 1;
			token.special_infos = 66;
			token.s_key_plus = SP_KEYWORD_HASTE;
			generate_token(&token);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

ARCANE(card_footsteps_of_the_goryo){
	// Return target creature card from your graveyard to the battlefield. Sacrifice that creature at the beginning of the next end step.
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int selected = select_a_card(player, player, 2, 0, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected == -1 ){
				spell_fizzled = 1;
			}
			else{
				instance->targets[0].player = selected;
				const int *grave = get_grave(player);
				instance->targets[0].card = grave[selected];
			}
	}
	if(event == EVENT_RESOLVE_SPELL){
		int selected = instance->targets[0].player;
		const int *grave = get_grave(player);
		if( instance->targets[0].card == grave[selected] ){
			int zombie = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			if (zombie != -1 && in_play(player, zombie)){
				int leg = pump_ability_until_eot(player, card, player, zombie, 0, 0, 0, SP_KEYWORD_DIE_AT_EOT);
				if (leg != -1){
					card_instance_t* legacy = get_card_instance(player, leg);
					legacy->targets[3].card = KILL_SACRIFICE;
				}
			}
		}
	}

	return 0;
}

// gaze of adamaro --> storm seeker

int card_ghost_lit_nourisher(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		if( has_mana_multi(player, 3, 0, 0, 1, 0, 0) ){
			return can_target(&td);
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, 3, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 2, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_ghost_lit_raider(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
			}
	}
	else if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 3, 0, 0, 0, 1, 0) ){
				return can_target(&td);
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, 3, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 2, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_ghost_lit_redeemer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 2);
	}
	else if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 1, 0, 0, 0, 0, 1) ){
				return 1;
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, 1, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 ){
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			gain_life(player, 4);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_ghost_lit_stalker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
	}
	else if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 5, 0, 0, 0, 2, 0) && can_sorcery_be_played(player, event) ){
				return can_target(&td);
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, 5, 0, 0, 0, 2, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			if( valid_target(&td) ){
				new_multidiscard(instance->targets[0].player, 4, 0, player);
			}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET|GAA_CAN_SORCERY_BE_PLAYED, 4, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

// glitterfang --> viashino sandstalker

// hand of cruelty / honor --> devoted retainer

int card_haru_onna(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_hidetsugus_second_rite(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int target = instance->targets[0].player;
				if( life[ target ] == 10 ){
					damage_player(target, 10, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int homura_human_ascendant_flip_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CHANGE_TYPE && instance->targets[0].player > -1 && affect_me(instance->targets[0].player, instance->targets[0].card) &&
		instance->targets[1].player != 66
	  ){
		if( instance->targets[1].card == -1 ){
			instance->targets[1].card = get_internal_card_id_from_csv_id(CARD_ID_HOMURAS_ESSENCE);
		}
		event_result = instance->targets[1].card;
	}
	if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
		&& instance->targets[0].player == trigger_cause_controller && instance->targets[0].card == trigger_cause
	  ){
		if (event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if (event == EVENT_RESOLVE_TRIGGER){
				true_transform(instance->targets[0].player, instance->targets[0].card);
				kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}


int card_homuras_essence(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[13].player = CARD_ID_HOMURAS_ESSENCE;
		instance->targets[13].card = CARD_ID_HOMURA_HUMAN_ASCENDANT;
	}

	boost_creature_type(player, card, event, -1, 2, 2, KEYWORD_FLYING, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	return card_ghitu_war_cry(player, card, event);
}

int card_homura_human_ascendant(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  nice_creature_to_sacrifice(player, card);

  double_faced_card(player, card, event);

  int owner, position;
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)
	  && find_in_owners_graveyard(player, card, &owner, &position))
	{
		int to_add = add_card_to_hand(owner, get_grave(owner)[position]);
		set_special_flags3(owner, to_add, SF3_REANIMATED);
		int legacy = create_legacy_effect(owner, to_add,  &homura_human_ascendant_flip_legacy);
		card_instance_t *instance = get_card_instance(owner, legacy);
		instance->targets[0].player = owner;
		instance->targets[0].card = to_add;
		obliterate_card_in_grave(owner, position);
		put_into_play(owner, to_add);
	}

  return 0;
}

static int effect_ideas(int player, int card, event_t event){

  if( eot_trigger(player, card, event ) ){
	 int j;
	 int k = 3;
	 if(hand_count[player] < 3){
		k = hand_count[player];
	 }
	 for(j=0;j<k;j++){
		 discard (player, 0, 0);
	 }
	 kill_card(player, card, KILL_REMOVE);
  }

  return 0;
}

ARCANE(card_ideas_unbound){
	// Draw three cards. Discard three cards at the beginning of the next end step.
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			create_legacy_effect(player, card, &effect_ideas );
			draw_cards (player,3);
	}
	return 0;
}

int card_iizuka_the_restless(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAMURAI, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 ){
			if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SAMURAI, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, SUBTYPE_SAMURAI, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
	}
	return bushido(player, card, event, 2);
}

int card_iname_as_one(int player, int card, event_t event)
{
  /* Iname as One	|8|B|B|G|G
   * Legendary Creature - Spirit 8/8
   * When ~ enters the battlefield, if you cast it from your hand, you may search your library for a Spirit permanent card, put it onto the battlefield, then
   * shuffle your library.
   * When ~ dies, you may exile it. If you do, return target Spirit permanent card from your graveyard to the battlefield. */

	check_legend_rule(player, card, event);

	if (!((trigger_condition == TRIGGER_COMES_INTO_PLAY || event == EVENT_GRAVEYARD_FROM_PLAY) && affect_me(player, card))&& event != EVENT_RESOLVE_TRIGGER){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Spirit permanent.");
	this_test.subtype = SUBTYPE_SPIRIT;

	if( comes_into_play_mode(player, card, event, (not_played_from_hand(player, card) ? 0 : RESOLVE_TRIGGER_AI(player))) ){
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	// AI activates only if there's another targetable Spirit in his graveyard
	int mode = RESOLVE_TRIGGER_OPTIONAL;
	if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card) && (player == AI || ai_is_speculating == 1)
		&& new_special_count_grave(player, &this_test)	// card won't be in graveyard yet during EVENT_GRAVEYARD_FROM_PLAY
		&& !graveyard_has_shroud(player))
		mode = RESOLVE_TRIGGER_MANDATORY;

	if (this_dies_trigger(player, card, event, mode)
	  && exile_from_owners_graveyard(player, card)
	  && new_special_count_grave(player, &this_test)	// card's already been exiled, so still correct to only need one spirit
	  && !graveyard_has_shroud(player))
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);

	return 0;
}

int card_infernal_kirin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( can_target(&td) && arcane_spirit_spell_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			int amount = get_cmc(instance->targets[1].player, instance->targets[1].card);
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);
			ec.effect = EC_ALL_WHICH_MATCH_CRITERIA | EC_DISCARD;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.cmc = amount;
			new_effect_coercion(&ec, &this_test);
		}
	}

	return 0;
}

int card_ivory_crane_netsuke(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[current_turn] > 6 ){
			gain_life(current_turn, 4);
		}
	}
	return 0;
}

int card_jiwari_the_earth_aflame(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities |= KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, instance->parent_card);
		}
	}

	else if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 0, 0, 0, 0, 3, 0) ){
				return 1;
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, -1, 0, 0, 0, 3, 0);
			if( spell_fizzled != 1 ){
				instance->targets[0].player = x_value;
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_JIWARI_THE_EARTH_AFLAME));
			card_instance_t* added_inst = get_card_instance(player, card_added);
			added_inst->state |= STATE_INVISIBLE;   --hand_count[player];

			damage_all(player, card_added, ANYBODY, instance->targets[0].player, 0, 0, KEYWORD_FLYING, 1, 0, 0, 0, 0, 0, 0, -1, 0);

			obliterate_card(player, card_added);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, -1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_kagemaro_first_to_suffer(int player, int card, event_t event){
	/* Kagemaro, First to Suffer	|3|B|B
	 * Legendary Creature - Demon Spirit 100/100
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * |B, Sacrifice Kagemaro: All creatures get -X/-X until end of turn, where X is the number of cards in your hand. */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, card, player, -1, -hand_count[player], -hand_count[player], 0, 0);
		pump_subtype_until_eot(player, card, 1-player, -1, -hand_count[player], -hand_count[player], 0, 0);
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_kaho_minamo_hystorian(int player, int card, event_t event){

	/* Legendary Creature - Human Wizard 2/2
	 * When ~ enters the battlefield, search your library for up to three instant cards and exile them. Then shuffle your library.
	 * |X, |T: You may cast a card with converted mana cost X exiled with ~ without paying its mana cost. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		int amount = 3;
		if( player == AI ){
			amount = 1;
		}
		int removed = 0;
		int selected = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, TYPE_INSTANT, 2, 0, 0, 0, 0, 0, 0, -1, 0);
		if( selected != -1 ){
			instance->targets[removed].card = selected;
			instance->targets[removed].player = create_card_name_legacy(player, card, selected);
			removed++;
			while( removed < amount ){
					selected = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, TYPE_INSTANT, 2, 0, 0, 0, 0, 0, 0, -1, 0);
					if( selected != -1 ){
						instance->targets[removed].player = create_card_name_legacy(player, card, selected);
						instance->targets[removed].card = selected;
						removed++;
					}
					else{
						break;
					}
			}
			instance->targets[5].player = removed;
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		instance->targets[5].player > 0
	  ){
		int i;
		for(i=0; i<instance->targets[5].player; i++){
			if( instance->targets[i].card != -1 && check_rfg(player, instance->targets[i].card)  ){
				if( has_mana_for_activated_ability(player, card, get_cmc_by_id(instance->targets[i].card), 0, 0, 0, 0, 0) ){
					if( can_legally_play_csvid(player, instance->targets[i].card) ){
						return 1;
					}
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		// The card shouldn't be chosen until resolution, just X; but that would badly complicate the AI.
		char buffer[600];
		int pos = scnprintf(buffer, 600, " Cancel\n");
		int owner = get_owner(player, card);
		int i;
		for(i=0; i<instance->targets[5].player; i++){
			if( instance->targets[i].card != -1 && check_rfg(owner, instance->targets[i].card)  ){
				if( has_mana_for_activated_ability(player, card, get_cmc_by_id(instance->targets[i].card), 0, 0, 0, 0, 0) ){
					if( can_legally_play_csvid(player, instance->targets[i].card) ){
						card_ptr_t* c = cards_ptr[ instance->targets[i].card ];
						pos+=scnprintf(buffer+pos, 600-pos, " Play %s\n", c->name );
					}
				}
			}
		}
		int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		if( choice == 0 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, get_cmc_by_id(instance->targets[choice - 1].card), 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				instance->targets[4].player = instance->targets[choice-1].player;
				instance->targets[4].card = instance->targets[choice-1].card;
				int k;
				for (k = choice - 1; k < instance->targets[5].player; k++){
					instance->targets[k].card = instance->targets[k+1].card;
				}
				instance->targets[5].player--;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int owner = get_owner(instance->parent_controller, instance->parent_card);
		if (check_rfg(owner, instance->targets[4].card)){
			play_card_in_exile_for_free(player, owner, instance->targets[4].card);
		}

		if( in_play(player, instance->targets[4].player) ){
			kill_card(player, instance->targets[4].player, KILL_REMOVE);
		}
	}
	return 0;
}

int card_kami_of_the_crescent_moon(int player, int card, event_t event)
{
  /* Kami of the Crescent Moon	|U|U
   * Legendary Creature - Spirit 1/3
   * At the beginning of each player's draw step, that player draws an additional card. */

	check_legend_rule(player, card, event);

	if (event == EVENT_DRAW_PHASE && ! is_humiliated(player, card) ){
		++event_result;
	}

	return 0;
}

int card_kami_of_the_tender_garden(int player, int card, event_t event){

	basic_upkeep(player, card, event, 0, 0, 0, 1, 0, 0);

	return soulshift(player, card, event, 3, 1);
}

int card_kashi_tribe_elite(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == player && has_subtype(affected_card_controller, affected_card, SUBTYPE_SNAKE) &&
		is_legendary(affected_card_controller, affected_card)
	  ){
		event_result |= KEYWORD_SHROUD;
	}

	return freeze_when_damage(player, card, event);
}

int card_kitaki_wars_wage(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int i;
		int p = current_turn;
		for (i=active_cards_count[p];i>=0;i--){
			if( is_what(p, i, TYPE_ARTIFACT) && in_play(p, i) && has_mana(p, COLOR_COLORLESS, 1) ){
				int choice = do_dialog(p, player, card, p, i, " Pay 1\n Sacrifice Artifact", 0);
				if( choice == 0 ){
					charge_mana_multi(p, 1, 0, 0, 0, 0, 0);
					if( spell_fizzled == 1 ){
						choice = 1;
					}
				}
				if( choice == 1 ){
					kill_card( p, i, KILL_SACRIFICE );
				}
			}
		}
	}
	return 0;
}

int card_kemuri_onna(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && comes_into_play(player, card, event) ){
		pick_target(&td, "TARGET_PLAYER");
		discard(instance->targets[0].player, 0, player);
	}

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_kikus_shadow(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(instance->targets[0].player, instance->targets[0].card),
								instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_kiri_onna(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		return card_man_o_war(player, card, event);
	}

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_kiyomaro_first_to_stand(int player, int card, event_t event){
	/* Kiyomaro, First to Stand	|3|W|W
	 * Legendary Creature - Spirit 100/100
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * As long as you have four or more cards in hand, Kiyomaro has vigilance.
	 * Whenever Kiyomaro deals damage, if you have seven or more cards in hand, you gain 7 life. */

	check_legend_rule(player, card, event);

	if( hand_count[player] > 3 ){
		vigilance(player, card, event);
		if( hand_count[player] > 6 && damage_dealt_by_me(player, card, event, 0) ){
			gain_life(player, 7);
		}
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	return 0;
}

int card_kuon_essence(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_kuon_ogre_ascendant(int player, int card, event_t event){

	/* Kuon, Ogre Ascendant	|B|B|B [Kuon's Essence]
	 * Legendary Creature - Ogre Monk 2/4
	 * At the beginning of the end step, if three or more creatures died this turn, flip ~. */

	double_faced_card(player, card, event);

	check_legend_rule(player, card, event);

	if( creatures_dead_this_turn >= 3 && eot_trigger(player, card, event) ){
		true_transform(player, card);
	}

	return 0;
}

int card_kuros_taken(int player, int card, event_t event){

	bushido(player, card, event, 1);

	return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_maga_traitor_to_mortals(int player, int card, event_t event){

  /* Maga, Traitor to Mortals	|X|B|B|B
   * Legendary Creature - Human Wizard 0/0
   * ~ enters the battlefield with X +1/+1 counters on it.
   * When ~ enters the battlefield, target player loses life equal to the number of +1/+1 counters on it. */


	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			lose_life(instance->targets[0].player, count_counters(player, card, COUNTER_P1_P1));
		}
	}

	return 0;
}

int card_manriki_gusari(int player, int card, event_t event){

	/* Manriki-Gusari	|2
	 * Artifact - Equipment
	 * Equipped creature gets +1/+2 and has "|T: Destroy target Equipment."
	 * Equip |1 */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT);
	td1.required_subtype = SUBTYPE_EQUIPMENT;

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 1, 2, 0);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){

		if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
			! is_sick(instance->targets[8].player, instance->targets[8].card) && can_target(&td1) &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 1);
	}
	else if( event == EVENT_ACTIVATE ){
			int equip_cost = get_updated_equip_cost(player, card, 1);
			int choice = 0;
			if( has_mana( player, COLOR_COLORLESS, equip_cost) && can_sorcery_be_played(player, event) ){
				if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
					! is_sick(instance->targets[8].player, instance->targets[8].card) && can_target(&td1) &&
					has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
				  ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Kill equipment\n Do nothing", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 1);
				instance->info_slot = 66;
			}
			else if( choice == 1 ){
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td1, "TARGET_EQUIPMENT") ){
						tap_card(instance->targets[8].player, instance->targets[8].card);
						instance->info_slot = 67;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_masumaro_first_to_live(int player, int card, event_t event){
	/* Masumaro, First to Live	|3|G|G|G
	 * Legendary Creature - Spirit 100/100
	 * ~'s power and toughness are each equal to twice the number of cards in your hand. */

	check_legend_rule(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += 2 * hand_count[player];
	}

	return 0;
}


int card_meishin_the_mind_cage(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	boost_creature_type(player, card, event, -1, -hand_count[player], 0, 0, BCT_INCLUDE_SELF);

	return global_enchantment(player, card, event);
}

int card_michiko_konda_truth_seeker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage = get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player && damage->damage_source_player != player){
				if( damage->targets[4].player == -1 || damage->targets[4].card == -1 ){
					int good = 0;
					if( damage->info_slot > 0 ){
						good = 1;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = 1;
						}
					}

					if( good == 1 ){
						if( instance->info_slot < 0 ){
							instance->info_slot = 0;
						}
						instance->info_slot++;
					}
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				impose_sacrifice(player, card, 1-player, instance->info_slot, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				instance->info_slot = 0;
		}
	}

	return 0;
}

int card_mikokoro_center_of_the_sea(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) &&
			can_use_activated_abilities(player, card)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Everybody draw a card\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
			}
			else{
				 untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				draw_cards(player, 1);
				draw_cards(1-player, 1);
			}
			else{
				return mana_producer(player, card, event);
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_miren_the_moaning_well(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

#define CAN_SAC	(!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_use_activated_abilities(player, card)	\
				 && has_mana_for_activated_ability(player, card, MANACOST_X(4)) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0))

	if (event == EVENT_CAN_ACTIVATE){
		int rval = mana_producer(player, card, event);
		if (rval){
			return rval;
		}
		return CAN_SAC;
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = instance->info_slot = 0;
		if (!paying_mana() && CAN_SAC){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac and gain life\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if (charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
					test_definition_t test;
					new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
					int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
					if (!sac){
						untap_card_no_event(player, card);
						cancel = 1;
						return 0;
					}
					tap_card(player, card);
					instance->info_slot = get_toughness(BYTE2(sac), BYTE3(sac));
					kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				}
				else{
					untap_card_no_event(player, card);
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			gain_life(player, instance->info_slot);
		}
	}

	return mana_producer(player, card, event);
#undef CAN_SAC
}

int card_molting_skin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_BOUNCE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_murmurs_from_beyond(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int cards = 3;
			if( count_deck(player) < cards ){
				cards = count_deck(player);
			}
			if( cards > 0 ){
				int *deck = deck_ptr[player];
				show_deck(HUMAN, deck, cards, "Player revealed these", 0, 0x7375B0 );
				int selected = select_a_card(1-player, player, (TUTOR_FROM_DECK*10)+cards, 1, 1, -1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				int i;
				for(i=cards-1; i>-1; i--){
					if( i != selected ){
						add_card_to_hand(player, deck[i]);
						remove_card_from_deck(player, i);
					}
				}
				mill(player, 1);
			}

			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_neverending_torment(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int i;
				for(i=0; i<hand_count[player]; i++){
					global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
				int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//2
				card_instance_t *this = get_card_instance(player, card_added);
				this->targets[2].player = 2;
				this->targets[2].card = get_id(player, card);
				create_card_name_legacy(player, card_added, get_id(player, card));
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_nightsoil_kami(int player, int card, event_t event){
	return soulshift(player, card, event, 5, 1);
}

int card_nikko_onna(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		return card_monk_realist(player, card, event);
	}

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_o_naginata(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 3, 0, KEYWORD_TRAMPLE, 0);
}

int card_oboro_palace_in_the_clouds(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_BLUE, 1);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if (!paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)){
			return 1;
		}
		if (!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;

		if (is_tapped(player, card) || is_animated_and_sick(player, card) || !can_produce_mana(player, card)){
			choice = 1;
		}
		else if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Add U\n Bounce Oboro\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
						instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->info_slot = 0;
			bounce_permanent(player, instance->parent_card);
		}
	}

	return 0;
}

int card_oni_of_the_wild_places(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.required_color = COLOR_TEST_RED;
	td.illegal_abilities = 0;

	haste(player, card, event);

	return bounce_permanent_at_upkeep(player, card, event, &td);
}

int card_oppressive_will(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( player != AI  ){
				return result;
			}
			else{
				if( ! has_mana(card_on_stack_controller, COLOR_COLORLESS, hand_count[player])  ){
					return result;
				}
			}
		}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, hand_count[player]);
			kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_overwhelming_intellect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int result = card_remove_soul(player, card, event);
		if( result > 0 ){
			instance->targets[1].player = get_cmc(card_on_stack_controller, card_on_stack);
			return result;
		}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, instance->targets[1].player);
			return card_remove_soul(player, card, event);
	}
	else{
		return card_remove_soul(player, card, event);
	}
	return 0;
}

int card_pains_reward(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int life_bid[2] = {0, 0};
			if( player != AI ){
				life_bid[player] = choose_a_number(player, "Bid how much life?", life[player]);
			}
			else{
				life_bid[player] = life[1-player];
				if( check_battlefield_for_id(player, CARD_ID_PLATINUM_EMPERION) ){
					life_bid[player]++;
				}
				else{
					while( life_bid[player] > (life[player]-6) ){
							life_bid[player]--;
					}
				}
			}
			if( player != HUMAN ){
				char buffer[100];
				snprintf(buffer, 100, "Opponent bids %d life.", life_bid[player] );
				do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
			}
			int bid_turn = 1-player;
			while( 1 ){
					if( bid_turn != AI ){
						life_bid[bid_turn] = choose_a_number(bid_turn, "Bid how much life?", life_bid[bid_turn]);
					}
					else{
						if( life_bid[1-bid_turn] >= life[bid_turn] ){
							life_bid[bid_turn] = 0;
						}
						else{
							if( check_battlefield_for_id(bid_turn, CARD_ID_PLATINUM_EMPERION) ){
								life_bid[bid_turn] = life[1-bid_turn]+1;
							}
							else{
								if( life_bid[1-bid_turn]+1 <= (life[bid_turn]-6) ){
									life_bid[bid_turn] = life_bid[1-bid_turn]++;
								}
								else{
									life_bid[bid_turn] = 0;
								}
							}
						}
					}
					if( bid_turn != HUMAN ){
						char buffer[100];
						if( life_bid[bid_turn] > 0 ){
							snprintf(buffer, 100, "Opponent bids %d life.", life_bid[bid_turn] );
						}
						else{
							snprintf(buffer, 100, "Opponent passes.");
						}
						do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
					}
					if( life_bid[bid_turn] > 0 && life_bid[bid_turn] > life_bid[1-bid_turn]){
						bid_turn = 1-bid_turn;
					}
					else{
						break;
					}
			}
			int winner = player;
			if( life_bid[1-player] > life_bid[player] ){
				winner = 1-player;
			}
			lose_life(winner, life_bid[winner]);
			draw_cards(winner, 4);
			kill_card(player, card, KILL_DESTROY);

	}
	return 0;
}

ARCANE(card_path_of_angers_flame){
	// Creatures you control get +2/+0 until end of turn.
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 2, 0, 0, 0);
	}
	return 0;
}

int card_pithing_needle(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Select a card in play\n Choose a card from a list", 0);
		int id = -1;
		if( choice == 0 ){
			int stop = 0;
			while( stop == 0 ){
					pick_target(&td, "TARGET_PERMANENT");
					if( ! is_token(instance->targets[0].player, instance->targets[0].card) ){
						id = get_id(instance->targets[0].player, instance->targets[0].card);
						stop = 1;
					}
					instance->number_of_targets = 1;
			}
		}
		else{
			int stop = 0;
			while( stop == 0 ){
					if( ai_is_speculating != 1 ){
						int card_id = choose_a_card("Choose a card", -1, -1);
						if( is_valid_card(cards_data[card_id].id) ){
							id = cards_data[card_id].id;
							stop = 1;
						}
					}
			}
		}
		if( id != -1 ){
			instance->targets[9].card = id;
			create_card_name_legacy(player, card, id);
			manipulate_all(player, card, player, TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_DISABLE_NONMANA_ACTIVATED_ABILITIES);
			manipulate_all(player, card, 1-player,TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_DISABLE_NONMANA_ACTIVATED_ABILITIES);
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( trigger_cause_controller, trigger_cause ) &&
		reason_for_trigger_controller == affected_card_controller
	  ){
		if( affect_me(player, card) ){ return 0; }
		if( get_id(trigger_cause_controller, trigger_cause) == instance->targets[9].card ){
			if( event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					disable_nonmana_activated_abilities(trigger_cause_controller, trigger_cause, 1);
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int id = instance->targets[9].card;
		if( id != -1 ){
			manipulate_all(player, card, player, TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_ENABLE_NONMANA_ACTIVATED_ABILITIES);
			manipulate_all(player, card, 1-player,TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_ENABLE_NONMANA_ACTIVATED_ABILITIES);
		}
	}
	return 0;
}

// presence of the wise --> gerrard's wisdom

int card_promise_of_the_bunrei(int player, int card, event_t event){
	/* Promise of Bunrei	|2|W
	 * Enchantment
	 * When a creature you control dies, sacrifice ~. If you do, put four 1/1 colorless Spirit creature tokens onto the battlefield. */

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		generate_tokens_by_id(player, card, CARD_ID_SPIRIT, 4);
		kill_card(player, card, KILL_SACRIFICE);
	}
	return global_enchantment(player, card, event);
}

int card_promised_kannushi(int player, int card, event_t event){
	return soulshift(player, card, event, 7, 1);
}

int card_raving_oni_slave(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON) ){
		lose_life(player, 3);
	}

	if( leaves_play(player, card, event) && ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON) ){
		lose_life(player, 3);
	}

	return 0;
}

int card_razorjaw_oni(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK) ){
			event_result = 1;
		}
	}

	return 0;
}

int card_reki_the_history_of_kamigawa(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

ARCANE(card_rending_vines){
	// Destroy target artifact or enchantment if its converted mana cost is less than or equal to the number of cards in your hand.
	// Draw a card.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "DISENCHANT");
		if( IS_AI(player) && get_cmc(instance->targets[0].player, instance->targets[0].card) > hand_count[player] ){
			ai_modifier -= 256;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				if (get_cmc(instance->targets[0].player, instance->targets[0].card) <= hand_count[player]){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
				draw_cards(player, 1);
			}
	}
	return 0;
}

int card_reverence(int player, int card, event_t event)
{
  if (event == EVENT_ATTACK_LEGALITY && affected_card_controller != player && get_power(affected_card_controller, affected_card) <= 2)
	event_result = 1;

  return global_enchantment(player, card, event);
}

int card_rune_tails_essence(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				damage->info_slot = 0;
			}
		}
	}

	return 0;
}

int card_rune_tail_kitsune_ascendant(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	double_faced_card(player, card, event);

	if (life[player] > 29 && in_play(player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->targets[13].player == instance->targets[13].card){	// Don't transform back when we get back here during EVENT_CHANGE_TYPE
			true_transform(player, card);
		}
	}
	return 0;
}

static int effect_sakashima_the_impostor(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  check_legend_rule(p, c, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	instance->targets[1].player = 66;

  if (instance->targets[1].player == 66 && eot_trigger(player, card, event))
	bounce_permanent(p, c);

  if (event == EVENT_CAN_SKIP_TURN)	// Just in case the trigger gets countered
	instance->targets[1].player = 0;

  return generic_activated_ability(player, card, event, 0, MANACOST_XU(2,2), 0, NULL, NULL);
}
int card_sakashima_the_impostor(int player, int card, event_t event)
{
  /* Sakashima the Impostor	|2|U|U
   * Legendary Creature - Human Rogue 3/1
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield, except its name is still ~, it's legendary in addition to its other
   * types, and it gains "|2|U|U: Return ~ to its owner's hand at the beginning of the next end step." */

  check_legend_rule(player, card, event);

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	{
	  add_a_subtype(player, card, SUBTYPE_LEGEND);
	  set_legacy_image(player, CARD_ID_SAKASHIMA_THE_IMPOSTOR, create_targetted_legacy_activate(player, card, effect_sakashima_the_impostor, player, card));
	}

  return 0;
}

int card_sakura_tribe_scout(int player, int card, event_t event)
{
  // |T: You may put a land card from your hand onto the battlefield.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a land to put into play.");
	  new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
}

int card_sasayas_essence(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	if ((event == EVENT_COUNT_MANA || event == EVENT_TAP_CARD)
		&& affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_LAND)){
		// See comments in card_mana_flare().

		if (!in_play(player, card)){
			return 0;
		}

		// Avoid counting lands if we can
		if (event == EVENT_COUNT_MANA){
			if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
				|| !can_produce_mana(affected_card_controller, affected_card) ){
				return 0;
			}
		} else {	// event == EVENT_TAP_CARD
			if (tapped_for_mana_color < 0){
				return 0;
			}
		}

		int id = get_id(affected_card_controller, affected_card);
		int c, other_lands = 0;
		for (c = 0; c < active_cards_count[player]; ++c){
				if( in_play(player, c) && get_id(player, c) == id && c != affected_card){
					++other_lands;
				}
		}

		if (other_lands <= 0){
			return 0;
		}

		if (event == EVENT_COUNT_MANA){
			// A check for EA_MANA_SOURCE is conspicuous by its absence in the exe version of Mana Flare.
			int num_colors = 0;
			color_t col;
			card_instance_t* aff_instance = get_card_instance(affected_card_controller, affected_card);
			for (col = COLOR_COLORLESS; col <= COLOR_ARTIFACT; ++col){
				if (aff_instance->card_color & (1 << col)){
					++num_colors;
				}
			}

			if (num_colors > 0){
				/* This isn't correct - see the comment in card_axebane_guardian() in return_to_ravnica.c.  This card seems much more likely to overflow the
				 * array for declare_mana_available_hex() if it's split up the way Axebane Guardian is, so just assume it has to produce all the same color
				 * for each land. */
				declare_mana_available_hex(affected_card_controller, aff_instance->card_color, other_lands);
			} else {
				declare_mana_available(affected_card_controller, single_color_test_bit_to_color(aff_instance->card_color), other_lands);
			}
		} else {	// event == EVENT_TAP_CARD
			produce_mana_of_any_type_tapped_for(player, card, other_lands);
		}
	}

	return 0;
}

int card_sasaya_orochi_ascendant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.zone = TARGET_ZONE_HAND;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0 ){
		return 1;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		int amount = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_what(player, i, TYPE_LAND) ){
				amount++;
			}
		}
		if( amount > 6 ){
			true_transform(player, instance->parent_card);
		}
	}

	return 0;
}

int card_seed_the_land(int player, int card, event_t event){
	/* Seed the Land	|2|G|G
	 * Enchantment
	 * Whenever a land enters the battlefield, its controller puts a 1/1 |Sgreen Snake creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, 2, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SNAKE, &token);
		token.t_player = instance->targets[1].player;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_seek_the_horizon(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 14, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 14, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sekki_seasons_guide(int player, int card, event_t event){
	/* Sekki, Seasons' Guide	|5|G|G|G
	 * Legendary Creature - Spirit 0/0
	 * ~ enters the battlefield with eight +1/+1 counters on it.
	 * If damage would be dealt to Sekki, prevent that damage, remove that many +1/+1 counters from Sekki, and put that many 1/1 colorless Spirit creature tokens onto the battlefield.
	 * Sacrifice eight Spirits: Return Sekki from your graveyard to the battlefield. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 8);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_SPIRIT) > 7 ){
			return 1;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0) ){
			impose_sacrifice(player, card, player, 7, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
			return 1;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0 && count_1_1_counters(player, card) > 0 ){
				int amount = damage->info_slot;
				if( damage->info_slot >= count_1_1_counters(player, card) ){
					amount = count_1_1_counters(player, card);
				}
				damage->info_slot-=amount;
				remove_1_1_counters(player, card, amount);
				generate_tokens_by_id(player, card, CARD_ID_SPIRIT, amount);
			}
		}
	}
	return 0;
}

int card_shinen_of_lifes_roar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
			if( has_mana_multi(player, 2, 0, 0, 2, 0, 0) ){
				return can_target(&td);
			}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, 2, 0, 0, 2, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				discard_card(player, card);
				return 2;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_LURE);
	}

	return everybody_must_block_me(player, card, event);
}

int card_sink_into_takenuma(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->targets[1] = instance->targets[0];
			instance->info_slot = sweep(player, card, SUBTYPE_SWAMP);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) ){
			new_multidiscard(instance->targets[1].player, instance->info_slot, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_skull_collector(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.required_color = COLOR_TEST_BLACK;
	td.illegal_abilities = 0;

	bounce_permanent_at_upkeep(player, card, event, &td);

	return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_skyfire_kirin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( can_target(&td) && arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			int amount = get_cmc(instance->targets[1].player, instance->targets[1].card);
			if( get_cmc(instance->targets[0].player, instance->targets[0].card) == amount ){
				gain_control_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_soramaro_first_to_dream(int player, int card, event_t event){
	/* Soramaro, First to Dream	|4|U|U
	 * Legendary Creature - Spirit 100/100
	 * Flying
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * |4, Return a land you control to its owner's hand: Draw a card. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
	}

	return 0;
}

ARCANE_WITH_SPLICE(card_spiritual_visit, MANACOST_W(1)){
	/* Spiritual Visit	|W
	 * Instant - Arcane
	 * Put a 1/1 colorless Spirit creature token onto the battlefield.
	 * Splice onto Arcane |W */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_SPIRIT);
	}
	return 0;
}

// stampeding serow --> stampeding wildebeest

ARCANE(card_sunder_from_within){
	// Destroy target artifact or land.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_thoughts_of_ruin(int player, int card, event_t event){
	if ( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int p = player;
			if( i == 1 ){
				p = 1-player;
			}
			impose_sacrifice(player, card, p, hand_count[player], TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_urami(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return 0;
}


int card_tomb_of_urami(int player, int card, event_t event){
	/* Tomb of Urami	""
	 * Legendary Land
	 * |T: Add |B to your mana pool. ~ deals 1 damage to you if you don't control an Ogre.
	 * |2|B|B, |T, Sacrifice all lands you control: Put a legendary 5/5 |Sblack Demon Spirit creature token with flying named Urami onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_BLACK, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;

		if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 3, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Summon Urami\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 2, 2, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					manipulate_all(player, card, player,  TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_SACRIFICE);
					instance->info_slot = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			generate_token_by_id(player, card, CARD_ID_URAMI);
		}
		else{
			if( ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_OGRE) ){
				damage_player(player, 1, player, instance->parent_card);
			}
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_trusted_advisor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.required_color = COLOR_TEST_BLUE;
	td.illegal_abilities = 0;

	if( current_turn == player && event == EVENT_MAX_HAND_SIZE ){
		event_result+=2;
	}
	return bounce_permanent_at_upkeep(player, card, event, &td);;
}

int card_twincast(int player, int card, event_t event)
{
  int rval = twincast(player, card, event, NULL, NULL);

  if (event == EVENT_RESOLVE_SPELL)
	kill_card(player, card, KILL_DESTROY);

  return rval;
}

int card_undying_flame(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int *deck = deck_ptr[player];
				while( deck[0] != -1 ){
						show_deck(HUMAN, deck, 1, "Player revealed this", 0, 0x7375B0 );
						if( ! is_what(-1, deck[0], TYPE_LAND) ){
							int amount = get_cmc_by_id( cards_data[deck[0]].id);
							rfg_top_card_of_deck(player);
							damage_creature_or_player(player, card, event, amount);
							break;
						}
						else{
							rfg_top_card_of_deck(player);
						}
				}
				int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//2
				card_instance_t *this = get_card_instance(player, card_added);
				this->targets[2].player = 2;
				this->targets[2].card = get_id(player, card);
				create_card_name_legacy(player, card_added, get_id(player, card));
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_wine_of_blood_and_iron(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			instance->targets[1].player = 66;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				int amount = get_power(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, instance->parent_card,
										instance->targets[0].player, instance->targets[0].card, amount, 0, 0, 0);
			}
	}

	else if( instance->targets[1].player == 66 && eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
	}

	else{
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 4, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}
	return 0;
}

int card_yuki_onna(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		return card_uktabi_orangutan(player, card, event);
	}

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		bounce_permanent(player, card);
	}

	return 0;
}

