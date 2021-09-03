#include "manalink.h"

// Functions
static int do_flashback_with_life(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int lif){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_GRAVEYARD_ABILITY && can_pay_life(player, lif) ){
		return do_flashback(player, card, event, colorless, black, blue, green, red, white);
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( pay_flashback(player, instance->internal_card_id, event, colorless, black, blue, green, red, white) ){
			lose_life(player, lif);
			return 1;
		}
	}
	return 0;
}

static int grave_recycler(int player, int card, event_t event, int crds){

		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int sac = 1;
			if( count_graveyard(player) >= crds){
				char buffer[100];
				scnprintf(buffer, 100, "Select a card to put on bottom of your deck.");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, buffer);
				int amount = 0;
				while( 1 ){
						int canc = 0;
						if( amount > 0 ){
							canc = 1;
						}
						if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_BOTTOM_OF_DECK, canc, AI_MAX_VALUE, &this_test) != -1 ){
							amount++;
							if( amount == crds ){
								sac--;
								break;
							}
						}
						else{
							break;
						}
				}
			}
			if( sac ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}

	return 0;
}

static int grave_eater(int player, int card, event_t event, int crds){

	// Whenever ~ attacks or blocks, any player may exile [crds] cards from his or her graveyard. If a player does, ~ assigns no combat damage this turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card) ||
		(blocking(player, card, event) && !is_humiliated(player, card))
	   ){
		if( count_graveyard(1-player) >= crds ){
			char buffer[100];
			scnprintf(buffer, 100, " Remove %d cards from your grave\n Pass", crds);
			int choice = do_dialog(1-player, player, card, -1, -1, buffer, 0);
			if( choice == 0 ){
				char buffer2[100];
				scnprintf(buffer2, 100, "Select a card to exile.");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, buffer2);
				int amount = 0;
				while( 1 ){
						if( new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
							amount++;
							if( amount == crds ){
								negate_combat_damage_this_turn(player, card, player, card, 0);
								break;
							}
						}
						else{
							break;
						}
				}
			}
		}
	}
	return 0;
}

static int dream(int player, int card, event_t event, int at_random){
	card_instance_t *instance= get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! affect_me(player, card) ){ return 0; }
		int number = hand_count[player] + 1;
		while( number > hand_count[player] ){
			number = choose_a_number(player, "Discard how many cards?", hand_count[player]);
		}
		multidiscard(player, number, at_random);
		instance->info_slot = number;
	}
	return 0;
}

static int possessed(int player, int card, event_t event, int clr){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = clr;

	card_instance_t *instance= get_card_instance(player, card);

	if( has_threshold(player) ){
		modify_pt_and_abilities(player, card, event, 1, 1, 0);
		if( event == EVENT_SET_COLOR && affect_me(player, card) ){
			event_result = COLOR_TEST_BLACK;
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	return 0;
}


// Cards

int card_acorn_harvest(int player, int card, event_t event){
	/* Acorn Harvest	|3|G
	 * Sorcery
	 * Put two 1/1 |Sgreen Squirrel creature tokens onto the battlefield.
	 * Flashback-|1|G, Pay 3 life. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_SQUIRREL, 2);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback_with_life(player, card, event, 1, 0, 0, 1, 0, 0, 3);
}

int card_ambassador_laquatus(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 3);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_anurid_scavenger(int player, int card, event_t event){
	return grave_recycler(player, card, event, 1);
}

int card_aquamoeba(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_arrogant_wurm(int player, int card, event_t event){
	return madness(player, card, event, 2, 0, 0, 1, 0, 0);
}

// balshan collaborator --> frozen shade

int card_balthor_the_stout(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_BARBARIAN;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	boost_creature_type(player, card, event, SUBTYPE_BARBARIAN, 1, 1, 0, 0);

	if( event == EVENT_CAN_ACTIVATE && target_available(player, card, &td) > 1 ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_NOT_ME_AS_TARGET, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_NOT_ME_AS_TARGET, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0);
		}
	}

	return 0;
}

int card_basking_rootwalla(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && instance->targets[1].card != 66 ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1  ){
			instance->targets[1].card = 66;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		pump_until_eot(player, card, player, instance->parent_card, 2, 2);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].card = 0;
	}

	return madness(player, card, event, 0, 0, 0, 0, 0, 0);
}

int card_breakthrough(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int x = instance->info_slot;
		draw_cards (player,4);
		int s = hand_count[player] - x;
		if( s ){
			new_multidiscard(player, s, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_cabal_coffers(int player, int card, event_t event){
	// original code : 0x4E279B

	if(event == EVENT_CAN_ACTIVATE){
		return (!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card) && has_mana(player, COLOR_COLORLESS, 3)
				&& (player != AI || count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) > 2));
	}

	else if(event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->state |= STATE_TAPPED;
		charge_mana(player, COLOR_COLORLESS, 2);
		if( spell_fizzled != 1 ){
			produce_mana_tapped(player, card, COLOR_BLACK, count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP));
		}
		else{
			untap_card_no_event(player, card);
		}
	}

	else if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if(!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card) && has_mana(player, COLOR_COLORLESS, 3)){
			int amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
			declare_mana_available(player, COLOR_BLACK, amount);
		}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_cabal_ritual(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		if( has_threshold(player) ){
			produce_mana(player, COLOR_BLACK, 5 );
		}
		else{
			produce_mana(player, COLOR_BLACK, 3 );
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_carrion_rats(int player, int card, event_t event){
	return grave_eater(player, card, event, 1);
}

int card_carrion_wurm(int player, int card, event_t event){
	return grave_eater(player, card, event, 3);
}

int card_centaur_chieftain(int player, int card, event_t event){

	/* Centaur Chieftain	|3|G
	 * Creature - Centaur 3/3
	 * Haste
	 * Threshold - As long as seven or more cards are in your graveyard, ~ has "When ~ enters the battlefield, creatures you control get +1/+1 and gain trample until end of turn." */

	if (has_threshold(player) && comes_into_play(player, card, event)){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, KEYWORD_TRAMPLE, 0);
	}

	haste(player, card, event);

	return 0;
}

int card_cephalid_sage(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && has_threshold(player) ){
		draw_cards(player, 3);
		new_multidiscard(player, 2, 0, player);
	}

	return 0;
}

int card_cephalid_vandal(int player, int card, event_t event){

	/* Cephalid Vandal	|1|U
	 * Creature - Cephalid Rogue 1/1
	 * At the beginning of your upkeep, put a shred counter on ~. Then put the top card of your library into your graveyard for each shred counter on ~. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_SHRED);
		mill(player, count_counters(player, card, COUNTER_SHRED));
	}

	return 0;
}

int card_chainers_edict(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_GRAVEYARD_ABILITY){
		// can only play as sorcery
		if( can_sorcery_be_played(player, event) && has_mana_multi(player, 5, 2, 0, 0, 0, 0) ){
			return 1;
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, 5, 2, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				return 1;
			}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		td.zone = TARGET_ZONE_PLAYERS;
		pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;

}

int card_chainer_dementia_master(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	boost_creature_type(player, card, event, SUBTYPE_NIGHTMARE, 1, 1, 0, BCT_INCLUDE_SELF);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 3, 0, 0, 0, 0) ){
		if( has_dead_creature(2) && ! graveyard_has_shroud(2) ){
			return can_pay_life(player, 3);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 3, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			if( has_dead_creature(1-player) ){
				if( has_dead_creature(player) ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
					}
				}
			}
			else{
				instance->targets[0].player = player;
			}
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_CMC, &this_test, 1) != -1 ){
				lose_life(player, 3);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			int zombo = reanimate_permanent(player, instance->parent_card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
			if( zombo != -1 ){
				force_a_subtype(player, zombo, SUBTYPE_NIGHTMARE);
			}
		}
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_NIGHTMARE;
		new_manipulate_all(player, card, 2, &this_test, KILL_REMOVE);
	}

	return 0;
}

int card_churning_eddy(int player, int card, event_t event)
{
  /* Churning Eddy	|3|U
   * Sorcery
   * Return target creature and target land to their owners' hands. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);

  target_definition_t td_land;
  default_target_definition(player, card, &td_land, TYPE_LAND);

  if (event == EVENT_CAN_CAST)
	return can_target(&td_creature) && can_target(&td_land);

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  if (pick_target(&td_creature, "TARGET_CREATURE"))
		new_pick_target(&td_land, "TARGET_LAND", 1, 1);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (validate_target(player, card, &td_creature, 0))
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);

	  if (validate_target(player, card, &td_land, 1))
		bounce_permanent(instance->targets[1].player, instance->targets[1].card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_cleansing_meditation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int tr = has_threshold(player);
		if( tr ){
			instance->info_slot = count_graveyard(player)-1;
		}
		else{
			instance->info_slot = -1;
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		APNAP(p,{ new_manipulate_all(player, card, p, &this_test, KILL_DESTROY); };);
		if( tr ){
			new_reanimate_all(player, card, player, &this_test, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_compulsion(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0) ){
		if( hand_count[player] > 0 ){
			return 1;
		}
		if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int choice = 0;
			if( hand_count[player] > 0 ){
				if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && player != AI ){
					choice = do_dialog(player, player, card, -1, -1, " Discard & draw\n Sac Compulsion & draw\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0){
				discard(player, 0, player);
			}
			else if( choice == 1){
					kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_crippling_fatigue(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback_with_life(player, card, event, 1, 1, 0, 0, 0, 0, 3);
}

int card_dawn_of_the_dead(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 1);
		if( has_dead_creature(player) && ! graveyard_has_shroud(2) ){
			char buffer[100];
			scnprintf(buffer, 100, "Select a creature card.");
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			if( result > -1 ){
				create_targetted_legacy_effect(player, card, &haste_and_remove_eot, player, result);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_deep_analysis(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback_with_life(player, card, event, MANACOST_XU(1, 1), 3);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td2) ){
			draw_cards(get_card_instance(player, card)->targets[0].player, 2);
		}
		kill_card(player, card, get_flashback() ? KILL_REMOVE : KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td2, "TARGET_PLAYER", 1, NULL);
}

int card_devestating_dreams(int player, int card, event_t event){
	// devastating dreams

	card_instance_t *instance= get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				impose_sacrifice(player, card, i, instance->info_slot, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_damage_all(player, card, i, instance->info_slot, 0, &this_test);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return dream(player, card, event, 1);
}

int card_dwell_on_the_past(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (pick_target(&td, "TARGET_PLAYER")){
			select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MAX_VALUE : AI_MIN_VALUE, NULL, 4, &instance->targets[1]);
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		int i, any_shuffled_in = 0;
		for (i = 1; i <= 4; ++i){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
			if (selected != -1){
				from_graveyard_to_deck(instance->targets[0].player, selected, 2);
				any_shuffled_in = 1;
			}
		}
		if (any_shuffled_in){
			shuffle(instance->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_faceless_butcher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	return_from_oblivion(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		state_untargettable(player, card, 1);
		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
			   obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		state_untargettable(player, card, 0);
	}

	return 0;
}

static int false_memories_legacy(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		char buffer2[100];
		scnprintf(buffer2, 100, "Select a card to exile.");
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, buffer2);
		const int *grave = get_grave(player);
		int amount = 0;
		while( amount < 7 && grave[0] != -1 ){
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test) != -1 ){
					amount++;
				}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_false_memories(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			mill(player, 7);
			create_legacy_effect(player, card, &false_memories_legacy);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_far_wanderings(int player, int card, event_t event){

	/* Far Wanderings	|2|G
	 * Sorcery
	 * Search your library for a basic land card and put that card onto the battlefield tapped. Then shuffle your library.
	 * Threshold - If seven or more cards are in your graveyard, instead search your library for three basic land cards and put them onto the battlefield
	 * tapped. Then shuffle your library. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			tutor_basic_lands(player, TUTOR_PLAY_TAPPED, has_threshold(player) ? 3 : 1);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_fiery_temper(int player, int card, event_t event){
	madness(player, card, event, 0, 0, 0, 0, 1, 0);
	return card_lightning_bolt(player, card, event);
}

int card_flaming_gambit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		instance->info_slot = x_value;
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE );
				td1.allowed_controller = instance->targets[0].player;
				td1.preferred_controller = instance->targets[0].player;
				td1.who_chooses = instance->targets[0].player;
				if( can_target(&td1) && new_pick_target(&td1, "TARGET_CREATURE", 1, 0) ){
					damage_creature(instance->targets[1].player, instance->targets[1].card, instance->info_slot, player, card);
				}
				else{
					damage_player(instance->targets[0].player, instance->info_slot, player, card);
				}
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, MANACOST_R(2));
}

int card_frantic_purification(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_ENCHANTMENT");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return madness(player, card, event, 0, 0, 0, 0, 0, 1);
}

int card_gloomdrifter(int player, int card, event_t event){

	/* Gloomdrifter	|3|B
	 * Creature - Zombie Minion 2/2
	 * Flying
	 * Threshold - As long as seven or more cards are in your graveyard, ~ has "When ~ enters the battlefield, non|Sblack creatures get -2/-2 until end of
	 * turn." */

	if( comes_into_play(player, card, event) && has_threshold(player) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "");
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		this_test.color_flag = 1;
		pump_creatures_until_eot(player, card, ANYBODY, 0, -2, -2, 0, 0, &this_test);
	}

	return 0;
}

int card_grim_lavamancer(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		if( count_graveyard(player) > 1 && ! is_tapped(player, card) && ! is_sick(player, card) &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0)
		  ){
			return can_target(&td1);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
					pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
					tap_card(player, card);
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION && valid_target(&td1) ){
		damage_creature_or_player(player, card, event, 2);
	}

	return 0;
}

int card_gurzigost(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_DEAL_DAMAGE && instance->info_slot == 1){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == player &&  damage->damage_source_card == card && damage->info_slot > 0 ){
				damage->damage_target_card = -1;
			}
		}
	}

	if( event == EVENT_DECLARE_BLOCKERS && instance->state & STATE_ATTACKING && ! is_unblocked(player, card)){
		if( has_mana(player, COLOR_GREEN, 2) && hand_count[player] > 0 ){
			int choice = do_dialog(player, player, card, -1, -1, " Deal damage normally\n Ignore Blocker", 1);
			if( choice == 1 ){
				charge_mana(player, COLOR_GREEN, 2);
				if( spell_fizzled != 1 ){
					discard(player, 0, player);
					instance->info_slot = 1;
				}
			}
		}
	}
	return grave_recycler(player, card, event, 2);
}

int card_hell_bent_rider(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_PROT_WHITE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD_RANDOM, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_hypnox(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) && instance->info_slot != 1 ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			int count = active_cards_count[1-player]-1;
			instance->targets[1].player = 1;
			while( count > -1 ){
					if( in_hand(1-player, count) ){
						card_instance_t *crd = get_card_instance(1-player, count);
						int pos = instance->targets[1].player;
						instance->targets[pos].card = crd->internal_card_id;
						create_card_name_legacy(player, card, get_id(1-player, count));
						rfg_card_in_hand(1-player, count);
						instance->targets[1].player++;
					}
					count--;
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=1; i<instance->targets[1].player; i++){
			add_card_to_hand(1-player, instance->targets[i].card);
		}
	}

	return 0;
}

int card_hypochondria(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
		if( hand_count[player] > 0 ){
			return 0x63;
		}
		if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 0x63;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			int choice = 0;
			if( hand_count[player] > 0 ){
				if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && player != AI ){
					choice = do_dialog(player, player, card, -1, -1, " Discard\n Sac Hypochondria\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0){
				if( pick_target(&td, "TARGET_DAMAGE") ){
					instance->number_of_targets = 1;
					discard(player, 0, player);
				}
			}
			else if( choice == 1){
					if( pick_target(&td, "TARGET_DAMAGE") ){
						kill_card(player, card, KILL_SACRIFICE);
					}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( dmg->info_slot < 3 ){
			dmg->info_slot = 0;
		}
		else{
			dmg->info_slot -= 3;
		}
	}

	return global_enchantment(player, card, event);
}

int card_ichorid(int player, int card, event_t event){

	/* Ichorid	|3|B
	 * Creature - Horror 3/1
	 * Haste
	 * At the beginning of the end step, sacrifice ~.
	 * At the beginning of your upkeep, if ~ is in your graveyard, you may exile a |Sblack creature card other than ~ from your graveyard. If you do, return ~
	 * to the battlefield. */

	haste(player, card, event);
	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		int found = 0;
		const int *graveyard = get_grave(player);
		int i;
		for(i=0;i<count_graveyard(player);i++){
			if(graveyard[i] != -1 && ( cards_data[  graveyard[i] ].color & COLOR_TEST_BLACK ) && ( cards_data[  graveyard[i] ].type & TYPE_CREATURE ) ){
				found++;
			}
		}
		if( found > 1 ){
			int choice = do_dialog(player, player, card, -1, -1," Return Ichorid\n Do not return Ichorid", 0);
			if( choice == 0 ){
				int i_loc = -1;
				int iid = -1;
				// first remove ichorid from the grave
				int cg = count_graveyard(player);
				for (i = 0; i < cg; ++i){
					if( cards_data[graveyard[i]].id == CARD_ID_ICHORID ){
						i_loc = i;
						iid = turn_card_in_grave_face_down(player, i);
						break;
					}
				}
				ASSERT(iid != -1);

				// choose a black dude to remove
				int selected = select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MIN_VALUE, -1, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0);
				turn_card_in_grave_face_up(player, i_loc, iid);

				if( selected != -1){
					if( i_loc > selected ){
						--i_loc;
					}
					rfg_card_from_grave(player, selected);

					// put ichorid into play
					reanimate_permanent(player, -1, player, i_loc, REANIMATE_DEFAULT);
				}
			}
		}
		return -2;
	}
	else if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_insidious_dreams(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top of your deck.");
		this_test.no_shuffle = 1;

		int i;
		int tutored[instance->info_slot];
		for (i = 0; i < instance->info_slot; ++i){
			tutored[i] = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}

		shuffle(player);

		int really_tutored = 0;

		for (i = 0; i < instance->info_slot; ++i){
			if(tutored[i] != -1){
				put_on_top_of_deck(player, tutored[i]);
				really_tutored++;
			}
		}

		if( really_tutored > 1 ){
			rearrange_top_x(player, player, really_tutored);
		}

		kill_card(player, card, KILL_DESTROY);
	}
	return dream(player, card, event, 0);
}

static int insist_effect(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					state_untargettable(trigger_cause_controller, trigger_cause, 0);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_insist(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( current_phase < PHASE_DECLARE_ATTACKERS ){
			ai_modifier+=25;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		create_legacy_effect(player, card, &insist_effect);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_krosan_restorer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int trgs = 0;
			int max_trgs = 1;
			if( has_threshold(player) ){
				max_trgs = 3;
			}
			while( trgs < max_trgs ){
					if( new_pick_target(&td, "TARGET_LAND", trgs, 0) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			if( trgs > 0 ){
				instance->info_slot = trgs;
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				untap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
	}

	return 0;
}

int card_laquatus_champion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->targets[0].card = lose_life(instance->targets[0].player, 6);
		}
	}

	if( leaves_play(player, card, event) ){
		gain_life(instance->targets[0].player, instance->targets[0].card);
	}

	return regeneration(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_last_laugh(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	// Whenever a permanent other than ~ is put into a graveyard from the battlefield, ~ deals 1 damage to each creature and each player.
	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int n = instance->targets[11].card;
		instance->targets[11].card = 0;
		for (; n > 0; --n){
			new_damage_all(player, card, ANYBODY, 1, NDA_PLAYER_TOO, NULL);
		}
	}

	// When no creatures are on the battlefield, sacrifice ~.
	if (event == EVENT_STATIC_EFFECTS && ! check_battlefield_for_subtype(ANYBODY, TYPE_CREATURE, -1) && instance->targets[11].card <= 0){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_liquify(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);
	td.extra = 3;
	td.special |= TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;

	if (event == EVENT_RESOLVE_SPELL){
		if (counterspell_validate(player, card, &td, 0)){
			card_instance_t* instance = get_card_instance(player, card);
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
		return 0;
	} else {
		return counterspell(player, card, event, &td, 0);
	}

	return 0;
}

int card_mesmeric_fiend(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && target_opponent(player, card) ){
		ec_definition_t ec;
		default_ec_definition(instance->targets[0].player, player, &ec);
		ec.effect = EC_RFG;

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card.");
		this_test.type_flag = DOESNT_MATCH;
		instance->targets[1].card = new_effect_coercion(&ec, &this_test);

		if( instance->targets[1].card != -1 ){
			create_card_name_legacy(player, card, instance->targets[1].card);
		}
	}

	if( leaves_play(player, card, event) ){
		if( instance->targets[1].card != -1 ){
			add_card_to_hand(instance->targets[0].player, get_internal_card_id_from_csv_id(instance->targets[1].card));
			remove_card_from_rfg(instance->targets[0].player, instance->targets[1].card);
		}
	}

	return 0;
}

int card_mind_sludge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				new_multidiscard(instance->targets[0].player, count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP), 0, player);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_morningtide(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				int count = count_graveyard(i)-1;
				while( count > -1 ){
						rfg_card_from_grave(i, count);
						count--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mortal_combat(int player, int card, event_t event){

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 19 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					lose_the_game(1-player);
			}
		}
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int creatures = count_graveyard_by_type(player, TYPE_CREATURE) - 10;
		if (creatures > 10){
			if (player == AI){
				ai_modifier += creatures * creatures * creatures;
			} else if (creatures >= 5){
				ai_modifier -= creatures * creatures * creatures;
			}
		}
		if (creatures >= 20){
			lose_the_game(1-player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_mortiphobia(int player, int card, event_t event){
	/*
	  Mortiphobia |1|B|B
	  Enchantment
	  {1}{B}, Discard a card: Exile target card from a graveyard.
	  {1}{B}, Sacrifice Mortiphobia: Exile target card from a graveyard.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( (count_graveyard(player) > 0 && ! graveyard_has_shroud(player)) || (count_graveyard(1-player) > 0 && ! graveyard_has_shroud(1-player)) ){
			if( generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_XB(1, 1), 0, NULL, NULL) ){
				return 1;
			}
			if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XB(1, 1), 0, NULL, NULL) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DISCARD, MANACOST_XB(1, 1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST_XB(1, 1), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Discard\n Sac Mortiphobia\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1)) ){
			if( select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &this_test, 0, 1) != -1 ){
				instance->info_slot = choice;
				if( choice == 0 ){
					discard(player, 0, player);
				}
				if( choice == 1 ){
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
		}
	}

	return 0;
}

int card_mutilate(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
			pump_subtype_until_eot(player, card, 2, -1, -amount, -amount, 0, 0);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mystic_familiar(int player, int card, event_t event){
	if( has_threshold(player)  ){
		modify_pt_and_abilities(player, card, event, 1, 1, get_sleighted_protection(player, card, KEYWORD_PROT_BLACK));
	}
	return 0;
}

int card_nantuko_cultivator(int player, int card, event_t event){
	char msg[100] = "Select a land card to discard.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);
	this_test.zone = TARGET_ZONE_HAND;
	if( comes_into_play(player, card, event) ){
		int count = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		int amount = 0;
		while( count > 0 ){
				int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( result != -1 ){
					discard_card(player, result);
					amount++;
					count--;
				}
				else{
					break;
				}
		}
		add_1_1_counters(player, card, amount);
		draw_cards(player, amount);
	}
	return 0;
}

int card_nantuko_shade(int player, int card, event_t event){
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_B(1), 1,1);
}

int card_narcissism(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
		if( count_graveyard(player) > 0 || count_graveyard(1-player) > 0 ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			int choice = 0;
			if( hand_count[player] > 0 ){
				if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && player != AI ){
					choice = do_dialog(player, player, card, -1, -1, " Discard\n Sacrifice\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0 || choice == 1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					if( choice == 0 ){
						discard(player, 0, player);
					}
					else{
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return global_enchantment(player, card, event);
}

int card_nostalgic_dreams(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return 1;
	} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		int max_cards = count_graveyard(player);
		max_cards = MIN(max_cards, hand_count[player]);
		max_cards = MIN(max_cards, 19);

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, NULL, max_cards, &instance->targets[0]);

		if (instance->info_slot > 0){
			multidiscard(player, instance->info_slot, 0);
		}
	} else if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < instance->info_slot; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
				++num_validated;
			}
		}
		if (num_validated == 0 && instance->info_slot > 0){
			spell_fizzled = 1;
			kill_card(player, card, KILL_DESTROY);
		} else {
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_obsessive_search(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 1);
			kill_card(player, card, KILL_DESTROY);
	}
	return madness(player, card, event, 0, 0, 1, 0, 0, 0);
}

int card_organ_grinder(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return count_graveyard(player) > 2 ? 1 : 0;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_PLAYER") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			int count = 0;
			while( count < 3 ){
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
					count++;
			}
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
		}
	}

	return 0;
}

static int overmaster_effect(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_SPELL) ){
			state_untargettable(affected_card_controller, affected_card, 1);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_overmaster(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( current_phase < PHASE_DECLARE_ATTACKERS ){
			ai_modifier+=25;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		create_legacy_effect(player, card, &overmaster_effect);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_parallel_evolution(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int p, c, marked[2][151] = {{0}};
			for (p = 0; p < 2; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && is_token(p, c)){
						marked[p][c] = 1;
					}
				}
			}
			for (p = 0; p < 2; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					if (marked[p][c]){
						token_generation_t token;
						copy_token_definition(player, card, &token, p, c);
						token.t_player = p;
						generate_token(&token);
					}
				}
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 4, 0, 0, 3, 0, 0);
}

int card_pardic_arsonist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	if( comes_into_play(player, card, event) && has_threshold(player) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 3);
		}
	}

	return 0;
}

// pardic collaborator --> frozen shade

int card_pay_no_heed(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return card_guardian_angel_exe(player, card, event);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_DAMAGE");
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td1) ){
				card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				dmg->info_slot = 0;
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_petradon(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		int removed = 0;
		while( can_target(&td) && removed < 2){
			if( pick_target(&td, "TARGET_LAND") ){
				removed++;
				instance->number_of_targets = 1;
				int result = rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
				instance->targets[removed+1].player = instance->targets[0].player;
				if( result > available_slots ){
					instance->targets[removed+1].player = 1-instance->targets[0].player;
					result-=available_slots;
				}
				instance->targets[removed+1].card = result;
				create_card_name_legacy(player, card, result);
			}
		}
		instance->targets[3].player = removed;
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=0; i<instance->targets[3].player; i++){
			int card_added = add_card_to_hand(instance->targets[i+1].player, get_internal_card_id_from_csv_id(instance->targets[i+1].card));
			remove_card_from_rfg(player, instance->targets[i+1].card);
			put_into_play(instance->targets[i+1].player, card_added);
		}
	}

	return 0;
}

static int plagiarize_legacy(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == get_card_instance(player, card)->targets[0].player && !suppress_draw){
		if( event == EVENT_TRIGGER){
			event_result |= 2u;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				draw_cards(player, 1);
				suppress_draw = 1;
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_plagiarize(int player, int card, event_t event){

	/* Plagiarize	|3|U
	 * Instant
	 * Until end of turn, if target player would draw a card, instead that player skips that draw and you draw a card. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int legacy = create_legacy_effect(player, card, &plagiarize_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0].player = instance->targets[0].player;
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_possessed_aven(int player, int card, event_t event){
	return possessed(player, card, event, COLOR_TEST_BLUE);
}

int card_possessed_barbarian(int player, int card, event_t event){
	return possessed(player, card, event, COLOR_TEST_RED);
}

int card_possessed_centaur(int player, int card, event_t event){
	return possessed(player, card, event, COLOR_TEST_GREEN);
}

int card_possessed_nomad(int player, int card, event_t event){
	vigilance(player, card, event);
	return possessed(player, card, event, COLOR_TEST_WHITE);
}

int card_putrid_imp(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
	}

	if( has_threshold(player) ){
		if( ( event == EVENT_POWER || event == EVENT_TOUGHNESS ) && affect_me(player, card) ){
			event_result++;
		}
		cannot_block(player, card, event);
	}
	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_pyromania(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 ){
			int choice = 0;
			if( hand_count[player] > 0 ){
				if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && player != AI ){
					choice = do_dialog(player, player, card, -1, -1, " Discard\n Sac Mortiphobia\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0 || choice == 1 ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					if( choice == 0 ){
						discard(player, 0, player);
					}
					else{
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_rancid_earth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_LAND");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				if( has_threshold(player) ){
					test_definition_t this_test;
					default_test_definition(&this_test, TYPE_CREATURE);
					new_damage_all(player, card, 2, 1, NDA_PLAYER_TOO, &this_test);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_restless_dreams(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return 1;
	} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
		max_cards = MIN(max_cards, hand_count[player]);
		max_cards = MIN(max_cards, 19);

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		char buf[100];
		if (ai_is_speculating == 1){
			*buf = 0;
		} else if (max_cards == 1){
			strcpy(buf, "Select a creature card.");
		} else {
			sprintf(buf, "Select up to %d creature cards.", max_cards);
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buf);
		instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, max_cards, &instance->targets[0]);

		if (instance->info_slot > 0){
			multidiscard(player, instance->info_slot, 0);
		}
	} else if (event == EVENT_RESOLVE_SPELL){
		int i;
		for (i = 0; i < instance->info_slot; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_retraced_image(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && hand_count[player] > 0 ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int revealed = -1;
			if( player == AI ){
				int count = 0;
				int par = 0;
				while( count < active_cards_count[player] ){
						if( in_hand(player, count) && is_what(player, count, TYPE_PERMANENT) ){
							if( check_battlefield_for_id(2, get_id(player, count)) ){
								if( get_cmc(player, count) > par ){
									par = get_cmc(player, count);
									revealed = count;
								}
							}
						}
						count++;
				}
			}
			else{
				char buffer[100];
				scnprintf(buffer, 100, "Select a permanent card to reveal.");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_PERMANENT, buffer);
				revealed = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			}
			if( revealed != -1 ){
				reveal_card(player, card, player, revealed);
				if( check_battlefield_for_id(2, get_id(player, revealed)) ){
					put_into_play(player, revealed);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int remove_counters_at_eot(int player, int card, event_t event)
{
  if (eot_trigger(player, card, event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  remove_counters(instance->damage_target_player, instance->damage_target_card, instance->targets[1].card, instance->targets[1].player);
	  kill_card(player, card, KILL_REMOVE);
	}
  return 0;
}

static void add_ssc_counter(int player, int card, int t_player, int t_card){
	int legacy = create_targetted_legacy_effect(player, card, &remove_counters_at_eot, t_player, t_card);
	card_instance_t *leg = get_card_instance(player, legacy);
	leg->targets[1].player = 1;
	leg->targets[1].card = COUNTER_M1_M1;
	add_minus1_minus1_counters(t_player, t_card, 1);
}

int card_shambling_swarm(int player, int card, event_t event){

	/* Shambling Swarm	|1|B|B|B
	 * Creature - Horror 3/3
	 * When ~ dies, distribute three -1/-1 counters among one, two, or three target creatures. For each -1/-1 counter you put on a creature this way, remove a
	 * -1/-1 counter from that creature at the beginning of the next end step. */

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance= get_card_instance(player, card);

		instance->number_of_targets = 0;
		int ssc;
		for (ssc = 0; ssc < 3 && can_target(&td); ++ssc){
				if (new_pick_target(&td, "TARGET_CREATURE", -1, 0)){
					add_ssc_counter(player, card, instance->targets[ssc].player, instance->targets[ssc].card);
				} else {
					break;
				}
		}
	}

	return 0;
}

int card_sickening_dreams(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, 2, instance->info_slot, NDA_PLAYER_TOO, &this_test);
			kill_card(player, card, KILL_DESTROY);
	}
	return dream(player, card, event, 0);
}

int card_skullscorch(int player, int card, event_t event){


	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int choice = 0;
			int ai_choice = 1;
			if( life[instance->targets[0].player]-4 < 6 || hand_count[instance->targets[0].player] < 1 ){
				ai_choice = 0;
			}
			choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Discard 2 at random\n Take 4 damage", ai_choice);
			if( choice == 0 ){
				new_multidiscard(instance->targets[0].player, 2, DISC_RANDOM, player);
			}
			else{
				damage_player(instance->targets[0].player, 4, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_sonic_seizures(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				discard(player, DISC_RANDOM, player);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 3);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_strength_of_isolation(int player, int card, event_t event){
	madness(player, card, event, 0, 0, 0, 0, 0, 1);
	return generic_aura(player, card, event, player, 1, 2, get_sleighted_protection(player, card, KEYWORD_PROT_BLACK), 0, 0, 0, 0);
}

int card_strength_of_lunacy(int player, int card, event_t event){
	madness(player, card, event, 0, 1, 0, 0, 0, 0);
	return generic_aura(player, card, event, player, 2, 1, get_sleighted_protection(player, card, KEYWORD_PROT_WHITE), 0, 0, 0, 0);
}

int card_stupefying_touch(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, GA_FORBID_ALL_ACTIVATED_ABILITIES);
}

static int tainted_land(int player, int card, event_t event, int color)
{
  card_instance_t *instance = get_card_instance(player, card);
  if (event == EVENT_CHANGE_TYPE || event == EVENT_RESOLVE_SPELL)
	{
	  if (event == EVENT_CHANGE_TYPE && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS))
		{
		  land_can_be_played |= LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS;	// delay until other cards have a change to change to or from swamp
		  return 0;
		}

	  if (check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP))
		instance->info_slot = COLOR_TEST_COLORLESS|COLOR_TEST_BLACK|color;
	  else
		instance->info_slot = COLOR_TEST_COLORLESS;

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  play_land_sound_effect_force_color(player, card, instance->info_slot);
		  return 0;	// so mana_producer() doesn't play the colorless sound
		}
	}

  return mana_producer(player, card, event);
}

int card_tainted_field(int player, int card, event_t event)
{
  return tainted_land(player, card, event, COLOR_TEST_WHITE);
}

int card_tainted_isle(int player, int card, event_t event)
{
  return tainted_land(player, card, event, COLOR_TEST_BLUE);
}

int card_tainted_peak(int player, int card, event_t event)
{
  return tainted_land(player, card, event, COLOR_TEST_RED);
}

int card_tainted_wood(int player, int card, event_t event)
{
  return tainted_land(player, card, event, COLOR_TEST_GREEN);
}

int card_transcendence(int player, int card, event_t event){
	if( in_play(player, card) ){
		cannot_lose_the_game_for_having_less_than_0_life(player, card, event, player);
		if( life[player] >= 20 ){
			lose_the_game(player);
		}
	}
	return global_enchantment(player, card, event);
}

int card_turbulent_dreams(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int number = hand_count[player] + 1;
			while( number > hand_count[player] ){
				number = choose_a_number(player, "Discard how many cards?", hand_count[player]);
			}
			if( number > target_available(player, card, &td) ){
				number = target_available(player, card, &td);
			}
			multidiscard(player, number, 0);
			int i;
			for(i=0; i<number; i++){
				if( new_pick_target(&td, "TARGET_PERMANENT", i, 0) ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
				}
			}
			for(i=0; i<number; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			instance->info_slot = number;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					bounce_permanent(instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_vengeful_dreams(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;
	td.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;

			char msg[100] = "Select a card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);

			int amount = 0;
			while( can_target(&td) && amount < hand_count[player] ){
					if( new_pick_target(&td, "TARGET_CREATURE", amount, 0) ){
						state_untargettable(instance->targets[amount].player, instance->targets[amount].card, 1);
						amount++;
					}
					else{
						break;
					}
			}

			int i;
			for(i=0; i<amount; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}

			if( amount > 0 ){
				int c_amount = amount;
				while( amount > 0){
						int mode = amount != c_amount ? 1 : 0;
						int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, mode, AI_MIN_VALUE, -1, &this_test);
						if( selected != -1 ){
							discard_card(player, selected);
							amount--;
						}
						else{
							break;
						}
				}
				if( amount > 0 ){
					spell_fizzled = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_violent_eruption(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int i;
			for(i=0; i<4; i++){
				new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", i, 0);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			divide_damage(player, card, &td);
			kill_card(player, card, KILL_DESTROY);
	}
	return madness(player, card, event, 1, 0, 0, 0, 2, 0);
}

