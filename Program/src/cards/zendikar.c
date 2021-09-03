#include "manalink.h"

// utility functions
void deck_was_shuffled(int player){
	/*
	int count = 0;
	int p = 1-player;
	while(count < active_cards_count[p]){
		card_data_t* card_d = get_card_data(p, count);
		if(card_d->id == CARD_ID_COSIS_TRICKSTER  && in_play(p, count)){
			add_1_1_counter(p, count);
		}
		count++;
	}
	*/
}


// int tutor_basic_land_tapped(int player){
//    int card = tutor_basic_land(player);
//    if( card != -1 ){
//        tap_card(player, card);
//    }
//    return card;
// }

static int count_allies(int player, int card){
	return count_subtype(player, TYPE_CREATURE, SUBTYPE_ALLY);
}

int landfall_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode){
	return specific_cip(player, card, event, player, trigger_mode,              TYPE_LAND,MATCH, 0,0, 0,0, 0, 0, -1,0);
}

int landfall(int player, int card, event_t event){
	return specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND,MATCH, 0,0, 0,0, 0, 0, -1,0);
}

int ally_trigger(int player, int card, event_t event, int trigger_mode){
	return specific_cip(player, card, event, player, trigger_mode, TYPE_CREATURE,MATCH, SUBTYPE_ALLY,MATCH, 0,0, 0,0, -1,0);
}

static int quest(int player, int card, event_t event, int req_counters, target_definition_t *td, const char *prompt){
	if( event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_QUEST) >= req_counters ){
		int flags = GAA_SACRIFICE_ME;
		if( td != NULL ){
			flags |= GAA_CAN_TARGET;
		}
		return generic_activated_ability(player, card, event, flags, 0, 0, 0, 0, 0, 0, 0, td, prompt);
	}
	if( event == EVENT_ACTIVATE ){
		int flags = GAA_SACRIFICE_ME;
		if( td != NULL ){
			flags |= GAA_CAN_TARGET;
		}
		return generic_activated_ability(player, card, event, flags, 0, 0, 0, 0, 0, 0, 0, td, prompt);
	}
   return global_enchantment(player, card, event);
}

static int expedition(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_counter(player, card, COUNTER_QUEST);
	}
	return quest(player, card, event, 3, NULL, 0);
}

// artifacts
int card_adventuring_gear(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( is_equipping(player, card) && landfall(player, card, event) && in_play(instance->targets[8].player, instance->targets[8].card) ){
		pump_until_eot_merge_previous(player, card, instance->targets[8].player, instance->targets[8].card, 2, 2);
	}
	return basic_equipment(player, card, event, 1);
}

int card_blade_of_the_bloodchief(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( is_equipping(player, card) && event == EVENT_GRAVEYARD_FROM_PLAY){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && affected->kill_code > 0 &&
			affected->kill_code < 4) {
		   if( instance->targets[11].player < 0 ){
			  instance->targets[11].player = 0;
		   }
		   instance->targets[11].player++;
		}
	}

	if( is_equipping(player, card) && instance->targets[11].player > 0 &&
		resolve_graveyard_trigger(player, card, event) == 1 &&
		in_play(instance->targets[8].player, instance->targets[8].card)
	  ){
		int i;
		for(i=0;i<instance->targets[11].player;i++){
			add_1_1_counter(instance->targets[8].player, instance->targets[8].card);
			if( has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_VAMPIRE) ){
			   add_1_1_counter(instance->targets[8].player, instance->targets[8].card);
			}
		}
		instance->targets[11].player = 0;
	}

	return basic_equipment(player, card, event, 1);
}

int card_blazing_torch(int player, int card, event_t event){

	/* Blazing Torch	|1
	 * Artifact - Equipment
	 * Equipped creature can't be blocked by Vampires or Zombies.
	 * Equipped creature has "|T, Sacrifice ~: ~ deals 2 damage to target creature or player."
	 * Equip |1 */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_BLOCK_LEGALITY && is_equipping(player, card)){
		int p = instance->targets[8].player;
		int c = instance->targets[8].card;
		if(attacking_card_controller == p && attacking_card == c ){
			if( has_creature_type(affected_card_controller, affected_card, SUBTYPE_VAMPIRE)
				|| has_creature_type(affected_card_controller, affected_card, SUBTYPE_ZOMBIE)
			  ){
				event_result = 1;
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) ){
			int p = instance->targets[8].player;
			int c = instance->targets[8].card;
			if( ! is_tapped(p, c) && ! is_sick(p, c) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && can_target(&td1)
			  ){
				return 1;
			}
		}
		return can_activate_basic_equipment(player, card, event, 1);
	}

	else if( event == EVENT_ACTIVATE ){
			int equip_cost = get_updated_equip_cost(player, card, 1);
			int choice = 0;
			if( has_mana( player, COLOR_COLORLESS, equip_cost) && can_sorcery_be_played(player, event) && check_for_equipment_targets(player, card) ){
				if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
					! is_sick(instance->targets[8].player, instance->targets[8].card) &&
					has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
				  ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Throw torch\n Do nothing", 1);
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
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)  && pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
						tap_card(instance->targets[8].player, instance->targets[8].card);
						instance->number_of_targets = 1;
						instance->info_slot = 67;
						kill_card(player, card, KILL_SACRIFICE);
					}
					else{
						spell_fizzled = 1;
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
				damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
			}
	}

	return 0;
}

int card_carnage_altar(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_ACTIVATION ){
		draw_a_card(player);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_eldrazi_monument(int player, int card, event_t event){

	// sac a creature during the upkeep
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			kill = 0;
		}
		if( kill ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) && ! is_humiliated(player, card) && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		switch( event ){
				case EVENT_ABILITIES:
				{
					indestructible(affected_card_controller, affected_card, event);
					event_result |= KEYWORD_FLYING;
				}
				break;

				case EVENT_POWER:
				case EVENT_TOUGHNESS:
					event_result++;
					break;

				default:
					break;
		}
	}

	return 0;
}


int card_eternity_vessel(int player, int card, event_t event)
{
  // ~ enters the battlefield with X charge counters on it, where X is your life total.
  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, life[player]);

  // Landfall - Whenever a land enters the battlefield under your control, you may have your life total become the number of charge counters on ~.
  if (landfall_mode(player, card, event,
					!duh_mode(player) ? RESOLVE_TRIGGER_OPTIONAL
					: life[player] < count_counters(player, card, COUNTER_CHARGE) ? RESOLVE_TRIGGER_MANDATORY : 0))
	set_life_total(player, count_counters(player, card, COUNTER_CHARGE));

  return 0;
}

int card_expedition_map(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a land card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_explorers_scope(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		if( ! is_equipping(player, card) && instance->targets[0].player == -1 ){
			ai_modifier+=15;
		}
	}

  // Whenever equipped creature attacks, look at the top card of your library. If it's a land card, you may put it onto the battlefield tapped.
  if (declare_attackers_trigger(player, card, event, 0, instance->targets[8].player, instance->targets[8].card))
	{
	  int* deck = deck_ptr[player];
	  if (deck[0] == -1)
		return 0;

	  int play = 0;
	  if (player == AI || ai_is_speculating == 1)
		play = is_what(-1, deck[0], TYPE_LAND);
	  else
		{
		  if (is_what(-1, deck[0], TYPE_LAND))
			play = reveal_card_optional_iid(player, card, deck[0], "Put on the battlefield tapped");
		  else
			reveal_card_iid(player, card, deck[0]);
		}

	  if (play)
		{
		  int iid = deck[0];
		  obliterate_top_card_of_deck(player);
		  int card_added = add_card_to_hand(player, iid);
		  get_card_instance(player, card_added)->state |= STATE_TAPPED;
		  put_into_play(player, card_added);
		}
	}

	return basic_equipment(player, card, event, 1);
}

int card_hedron_scrabbler(int player, int card, event_t event){
	if(  landfall(player, card, event) ){
		pump_until_eot_merge_previous(player, card, player, card, 1, 1);
	}
	return 0;
}

int card_khalni_gem(int player, int card, event_t event){
	if( comes_into_play(player, card, event) > 0   ){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			select_target(player, card, &td, "Choose a land to bounce", NULL);
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
		}
		if( can_target(&td) ){
			select_target(player, card, &td, "Choose a land to bounce", NULL);
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return artifact_mana_all_one_color(player, card, event, 2, 0);
}

int card_spidersilk_net(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if(event == EVENT_TOUGHNESS && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result+=2;
	}
	else if(event == EVENT_ABILITIES && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result |= KEYWORD_REACH;
	}
	return basic_equipment(player, card, event, 2);
}

int card_trailblazers_boots(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	int count = 0;
	int p = 1-player;
	while(count < active_cards_count[p]){
		card_data_t* card_d = get_card_data(p, count);
		if( (card_d->type & TYPE_LAND)  && in_play(p, count) && ! is_basic_land(p, count) ){
			unblockable(instance->targets[8].player, instance->targets[8].card, event);
			break;
		}
		count++;
	}
	return basic_equipment(player, card, event, 2);
}

int card_trusty_machete(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if(event == EVENT_POWER && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result+=2;
	}
	if(event == EVENT_TOUGHNESS && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result+=1;
	}
	return basic_equipment(player, card, event, 2);
}

// lands
int card_akoum_refuge(int player, int card, event_t event){
	/* Akoum Refuge	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, you gain 1 life.
	 * |T: Add |B or |R to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		gain_life(player, 1);
	}

	return mana_producer(player, card, event);
}

int card_arid_mesa(int player, int card, event_t event){
	/* Arid Mesa	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Mountain or |H2Plains card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_MOUNTAIN, SUBTYPE_PLAINS, 1);
}

int card_marsh_flats(int player, int card, event_t event){
	/* Marsh Flats	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Plains or |H2Swamp card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_PLAINS, SUBTYPE_SWAMP, 1);
}

int card_misty_rainforest(int player, int card, event_t event){
	/* Misty Rainforest	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Forest or |H2Island card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_FOREST, SUBTYPE_ISLAND, 1);
}

int card_scalding_tarn(int player, int card, event_t event){
	/* Scalding Tarn	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Han Island or |H2Mountain card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_ISLAND, SUBTYPE_MOUNTAIN, 1);
}

int card_verdant_catacombs(int player, int card, event_t event){
	/* Verdant Catacombs	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Swamp or |H2Forest card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_SWAMP, SUBTYPE_FOREST, 1);
}

int card_crypt_of_agadeem(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);

	if( event != EVENT_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, 3 ) ) {
			int ai_choice = 0;
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 3 ){
				ai_choice = 1;
			}
			choice = do_dialog(player, player, card, -1, -1, " Generate B\n Add B for each dead guy\n Cancel", ai_choice);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			charge_mana(player, COLOR_COLORLESS, 2);
			if( spell_fizzled != 1  ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
				int count = new_special_count_grave(player, &this_test);
				produce_mana_tapped(player, card, COLOR_BLACK, count);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	return 0;
}

int card_emeria_the_sky_ruin(int player, int card, event_t event ){

	comes_into_play_tapped(player, card, event);

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(current_turn);
		if( count > 0 && count_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) > 6 && count_graveyard_by_type(player, TYPE_CREATURE) > 0 &&
			! graveyard_has_shroud(2)
		  ){
			if(event == EVENT_TRIGGER ){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					card_instance_t *instance= get_card_instance(player, card);
					card_data_t* card_d = &cards_data[ instance->internal_card_id ];
					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					while( count > 0 && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
							ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
							count--;
					}
			}
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return mana_producer(player, card, event);
}

int card_kabira_crossroads(int player, int card, event_t event){
	/* Kabira Crossroads	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, you gain 2 life.
	 * |T: Add |W to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		gain_life(player, 2);
	}

	return mana_producer(player, card, event);
}

int card_magosi_the_waterveil(int player, int card, event_t event){

	/* Magosi, the Waterveil	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |U to your mana pool.
	 * |U, |T: Put an eon counter on ~. Skip your next turn.
	 * |T, Remove an eon counter from ~ and return it to its owner's hand: Take an extra turn after this one. */

	comes_into_play_tapped(player, card, event);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 2, 0, 0, 0) ){
			if( count_counters(player, card, COUNTER_EON) > 0 ){
				choice = do_dialog(player, player, card, -1, -1,
								   " Generate U\n Skip a turn\n Take an extra turn\n Cancel", 0);
			}
			else{
				choice = do_dialog(player, player, card, -1, -1, " Generate U\n Skip a turn\n Cancel", 0);
				if( choice == 2 ){
					choice++;
				}
			}
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1  ){
					instance->info_slot = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else if( choice == 2 ){
				tap_card(player, card);
				instance->info_slot = 2;
				bounce_permanent(player, card);
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				add_counter(instance->parent_controller, instance->parent_card, COUNTER_EON);
				int number = active_cards_count[1-player]+1;
				return card_time_walk(1-player, number, EVENT_RESOLVE_SPELL);
			}
			else if( instance->info_slot == 2) {
					return card_time_walk(player, card, EVENT_RESOLVE_SPELL);
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

void activate_oran_rief_pump(int player, int card){
	int i;
	for(i=0; i<active_cards_count[player]; i++){
		if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) && check_special_flags(player, i, SF_JUST_CAME_INTO_PLAY) ){
			int good = 1;
			if( get_id(player, card) == CARD_ID_ORAN_RIEF_THE_VASTWOOD && ! (get_color(player, i) & COLOR_TEST_GREEN) ){
				good = 0;
			}
			if( good == 1 ){
				add_1_1_counter(player, i);
			}
		}
	}
}

int card_oran_rief_the_vastwood(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->info_slot = 0;
		if( can_use_activated_abilities(player, card) && ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate G\n Pump Creatures\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				activate_oran_rief_pump(player, instance->parent_card);
				card_instance_t *parent= get_card_instance(player, instance->parent_card);
				parent->info_slot = 0;
			}
			else{
				return mana_producer(player, card, event);
			}
	}

	else if( trigger_condition == TRIGGER_EOT && affect_me(player, card ) && reason_for_trigger_controller == player && event == EVENT_TRIGGER ){	// not a real trigger; avoid the "processing" dialog
			remove_special_flags(2, -1, SF_JUST_CAME_INTO_PLAY);
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_piranha_marsh(int player, int card, event_t event){
	/* Piranha Marsh	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, target player loses 1 life.
	 * |T: Add |B to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.allow_cancel = 0;
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER" ) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return mana_producer(player, card, event);
}

int card_soaring_seacliff(int player, int card, event_t event){
	/* Soaring Seacliff	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, target creature gains flying until end of turn.
	 * |T: Add |U to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t*instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE" ) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return mana_producer(player, card, event);
}

int card_teetering_peaks(int player, int card, event_t event){
	/* Teetering Peaks	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, target creature gets +2/+0 until end of turn.
	 * |T: Add |R to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE" ) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}

	return mana_producer(player, card, event);
}

int card_turntimber_grove(int player, int card, event_t event){
	/* Turntimber Grove	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, target creature gets +1/+1 until end of turn.
	 * |T: Add |G to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE" ) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return mana_producer(player, card, event);
}

int card_valakut_the_molten_pinnacle(int player, int card, event_t event)
{
  /* Valakut, the Molten Pinnacle	""
   * Land
   * ~ enters the battlefield tapped.
   * Whenever |Ha Mountain enters the battlefield under your control, if you control at least five other |H1Mountains, you may have ~ deal 3 damage to target
   * creature or player.
   * |T: Add |R to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller && ! is_humiliated(player, card)
	  ){
		int mountain = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);
		if( count_subtype(player, TYPE_LAND, mountain) > 5 ){
			if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_LAND, 0, mountain, 0, 0, 0, 0, 0, -1, 0) ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);
				td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

				card_instance_t* instance = get_card_instance(player, card);
				instance->number_of_targets = 0;
				if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER")){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
				}
			}
		}
	}

	return mana_producer(player, card, event);
}

// green cards
int card_baloth_cage_trap(int player, int card, event_t event){
	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_ARTIFACTS_PLAYED) == 1 ){
			COST_GREEN -= 2;
			COST_COLORLESS -= 1;
		}
	}
	else if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 4;
			token.tou = 4;
			generate_token(&token);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_baloth_woodcrasher(int player, int card, event_t event){
	if( landfall(player, card, event ) ){
		pump_ability_until_eot(player, card, player, card, 4, 4, KEYWORD_TRAMPLE, 0 );
	}
	return 0;
}

int card_beast_hunt(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int i=0;
		int *deck = deck_ptr[player];
		for(i=0;i<3;i++){
			int card_added = add_card_to_hand(player, deck[0] );
			reveal_card(player, card, player, card_added);
			if( cards_data[ deck[0] ].type & TYPE_CREATURE ){
				remove_card_from_deck( player, 0 );
			}
			else{
				kill_card(player, card_added, KILL_DESTROY );
			}
			rfg_top_card_of_deck(player);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_beastmaster_ascension(int player, int card, event_t event)
{
  // Whenever a creature you control attacks, you may put a quest counter on ~.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_DUH | DAT_SEPARATE_TRIGGERS, player, -1))
	add_counter(player, card, COUNTER_QUEST);

  // As long as ~ has seven or more quest counters on it, creatures you control get +5/+5.
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && count_counters(player, card, COUNTER_QUEST) >= 7)
	event_result += 5;

  return global_enchantment(player, card, event);
}

int card_cobra_trap(int player, int card, event_t event){
	/* Cobra Trap	|4|G|G
	 * Instant - Trap
	 * If a noncreature permanent under your control was destroyed this turn by a spell or ability an opponent controlled, you may pay |G rather than pay ~'s mana cost.
	 * Put four 1/1 |Sgreen Snake creature tokens onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_SNAKE, 4 );
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_frontier_guide(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_land(player, 1, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 3, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_gigantiform(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			do_kicker(player, card, MANACOST_X(4));
		}
	}

	if( comes_into_play(player, card, event) && kicked(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.id = get_id(player, card);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( event == EVENT_POWER && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result += 8 - get_base_power(instance->damage_target_player, instance->damage_target_card);
		}
		if( event == EVENT_TOUGHNESS && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result += 8 - get_base_toughness(instance->damage_target_player, instance->damage_target_card);
		}
		if( event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return vanilla_aura(player, card, event, player);
}

int card_grazing_gladehart(int player, int card, event_t event){
	if( landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH) ){
		gain_life(player, 2);
	}
	return 0;
}

int card_greenweaver_druid(int player, int card, event_t event){
	//0x40b410 (Fyndhorn Elder)
	return mana_producing_creature(player, card, event, 36, COLOR_GREEN, 2);
}

int card_harrow(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_LAND);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land to sacrifice.");
		if( ! new_sacrifice(player, card, player, 0, &this_test) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		this_test.subtype = SUBTYPE_BASIC;
		this_test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		shuffle(player);
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_joraga_bard(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		int count =0;
		while(count < active_cards_count[player]){
			card_data_t* card_d = get_card_data(player, count);
			if((card_d->type & TYPE_CREATURE) && in_play(player, count) && has_subtype(player, count, SUBTYPE_ALLY) ){
				pump_ability_until_eot(player, card, player, count, 0, 0, 0, SP_KEYWORD_VIGILANCE );
			}
			count++;
		}
	}
	return 0;
}

int card_khalni_heart_expedition(int player, int card, event_t event){
	/* Khalni Heart Expedition	|1|G
	 * Enchantment
	 * Landfall - Whenever a land enters the battlefield under your control, you may put a quest counter on ~.
	 * Remove three quest counters from ~ and sacrifice it: Search your library for up to two basic land cards, put them onto the battlefield tapped, then
	 * shuffle your library. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 2);
	}
	return expedition(player, card, event);
}

int card_lotus_cobra(int player, int card, event_t event){
	if( landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH) ){
		int color = choose_a_color(player, get_deck_color(player, player));
		produce_mana(player, color, 1);
	}
	return 0;
}

int card_mold_shambler(int player, int card, event_t event){
	kicker(player, card, event, 1, 0, 0, 1, 0, 0);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_CREATURE;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose a noncreature to kill", NULL)){
				card_instance_t *instance = get_card_instance( player, card );
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}
	return 0;
}

int card_nissa_revane(int player, int card, event_t event){

	/* Nissa Revane	|2|G|G
	 * Planeswalker - Nissa (2)
	 * +1: Search your library for a card named Nissa's Chosen and put it onto the battlefield. Then shuffle your library.
	 * +1: You gain 2 life for each Elf you control.
	 * -7: Search your library for any number of Elf creature cards and put them onto the battlefield. Then shuffle your library. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].player = 0;
	}

	if (IS_ACTIVATING(event)){

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_ELF;

		int priority_life = 0;
		int priority_ef = 0;
		if( event == EVENT_ACTIVATE ){
			priority_life = (count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF)*2)+(15-life[player]);
			int c = 0;
			while( deck_ptr[player][c] != -1 ){
					if( is_what(-1, deck_ptr[player][c], TYPE_PERMANENT) && has_subtype_by_id(cards_data[deck_ptr[player][c]].id, SUBTYPE_ELF) ){
						priority_ef+=3;
					}
					c++;
			}
		}

		enum{
			CHOICE_NC = 1,
			CHOICE_LIFE,
			CHOICE_ELF_FEST
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Search a Nissa's Chosen", player == AI ? (instance->targets[1].player != -1 ? 1 : 0) : 1, 10, 1,
						"Add counters to creatures and planeswalkers", 1, priority_life, 1,
						"Elf festival", 1, priority_ef, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  switch (choice)
			{
				case CHOICE_NC:
					{
						this_test.id = CARD_ID_NISSAS_CHOSEN;
						int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
						get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].player = result;
					}
					break;

				case CHOICE_LIFE:
					gain_life(player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF)*2);
					break;

				case CHOICE_ELF_FEST:
					{
						this_test.no_shuffle = 1;
						while( 1 ){
								if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test) == -1 ){
									break;
								}
						}
						shuffle(player);
					}
					break;
			}
		}
	}

	return planeswalker(player, card, event, 2);
}

int card_nissas_chosen(int player, int card, event_t event)
{
  if (graveyard_from_play(player, card, event))
	{
	  put_on_bottom_of_deck(player, card);
	  event_result = 1;
	}

  return 0;
}

int card_oracle_of_mul_daya(int player, int card, event_t event){
	reveal_top_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		int* deck = deck_ptr[player];
		card_data_t* card_d = deck[0] == -1 ? NULL : &cards_data[ deck[0] ];

		if( can_sorcery_be_played(player, event) && deck[0] != -1 && ( card_d->type & TYPE_LAND ) && ! ( land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED )  ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
		play_card_in_deck_for_free(player, player, 0);
		cant_be_responded_to = 1;
	}
	return card_exploration(player, card, event);
}

int card_oran_rief_recluse(int player, int card, event_t event){
	kicker(player, card, event, 2, 0, 0, 1, 0, 0);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		card_instance_t *instance = get_card_instance( player, card );
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.required_abilities = KEYWORD_FLYING;
		td.allow_cancel = 0;
		if( can_target(&td) ){
			pick_target(&td, "TARGET_FLYING" );
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_oran_rief_survivalist (int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_predatory_urge(int player, int card, event_t event){
	/* Predatory Urge	|3|G
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature has "|T: This creature deals damage equal to its power to target creature. That creature deals damage equal to its power to this creature." */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( in_play(player, card) && instance->damage_target_player >= 0 ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;

			target_definition_t td1;
			default_target_definition(p, c, &td1, TYPE_CREATURE);

			if( event == EVENT_CAN_ACTIVATE ){
				return generic_activated_ability(p, c, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
			}
			if( event == EVENT_ACTIVATE ){
				return generic_activated_ability(p, c, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
			}
			if( event == EVENT_RESOLVE_ACTIVATION ){
				card_instance_t *enchanted = get_card_instance(p, c);
				if( validate_target(p, c, &td1, 0) ){
					// Not a fight.  Stop replacing this with fight().
					get_card_instance(p, c)->regen_status |= KEYWORD_RECALC_POWER;
					int mypow = get_power(p, c);
					damage_creature(enchanted->targets[0].player, enchanted->targets[0].card, mypow, p, c);
					if (in_play(p, c)){
						get_card_instance(enchanted->targets[0].player, enchanted->targets[0].card)->regen_status |= KEYWORD_RECALC_POWER;
						int hispow = get_power(enchanted->targets[0].player, enchanted->targets[0].card);
						damage_creature(p, c, hispow, enchanted->targets[0].player, enchanted->targets[0].card);
					}
				}
			}
			return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}

	else{
		return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}
	return 0;

}

int card_primal_bellow(int player, int card, event_t event){
	/* Primal Bellow	|G
	 * Instant
	 * Target creature gets +1/+1 until end of turn for each |H2Forest you control. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_LAND);
	td2.required_subtype = SUBTYPE_FOREST;
	td2.allowed_controller = player;
	td2.preferred_controller = player;
	td2.illegal_abilities = 0;
	int boost = target_available(player, card, &td2);

	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, boost, boost);
		kill_card(player, card, KILL_DESTROY);
	}
	else if( event == EVENT_CHECK_PUMP && has_mana(player, COLOR_GREEN, 1) ){
		pumpable_power[player] += boost;
		pumpable_toughness[player] += boost;
	}
	return 0;
}

int card_quest_for_the_gemblades(int player, int card, event_t event){

	/* Quest for the Gemblades	|1|G
	 * Enchantment
	 * Whenever a creature you control deals combat damage to a creature, you may put a quest counter on ~.
	 * Remove a quest counter from ~ and sacrifice it: Put four +1/+1 counters on target creature. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	// did a creature of mine deal combat damage?
	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		damage->damage_target_card != -1
	  ){
		add_counter(player, card, COUNTER_QUEST);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			add_1_1_counters( instance->targets[0].player, instance->targets[0].card, 4 );
		}
	}

	return quest(player, card, event, 1, &td, "TARGET_CREATURE");
}


int card_rampaging_baloths(int player, int card, event_t event)
{
  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_BEAST, &token);
	  token.pow = 4;
	  token.tou = 4;
	  generate_token(&token);
	}

  return 0;
}

int card_relic_crush(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT" ) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->state |= STATE_CANNOT_TARGET;
			if( can_target(&td) ){
				instance->targets[1].player = instance->targets[0].player;
				instance->targets[1].card = instance->targets[0].card;
				pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT" );
				target->state &= ~STATE_CANNOT_TARGET;
				if(spell_fizzled == 1 ){
					spell_fizzled = 0;
					instance->targets[0].player = instance->targets[1].player;
					instance->targets[0].card = instance->targets[1].card;
					instance->targets[1].player = -1;
				}
			}
			else{
				target->state &= ~STATE_CANNOT_TARGET;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0 )){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( validate_target(player, card, &td, 1 )){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_savage_silhouette(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, 2, 2, 0);

		if( event == EVENT_RESOLVE_ACTIVATION )
			if( can_be_regenerated(instance->damage_target_player, instance->damage_target_card))
				regenerate_target(instance->damage_target_player, instance->damage_target_card);
	}

	return aura_granting_activated_ability(player, card, event, player, GAA_REGENERATION, 1, 0, 0, 1, 0, 0, 0, 0, 0);
}


int card_scute_mob(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(player, TYPE_LAND, -1) > 4 ){
			add_1_1_counters(player, card, 4);
		}
	}

	return 0;
}

int card_scythe_tiger(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		if( target_available(player, card, &td) ){
			if(!select_target(player, card, &td, "Choose a land to sacrifice", NULL)){
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}
	return 0;
}

int card_summoners_trap(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(player, TRAP_SUMMONING_TRAP) ){
			null_casting_cost(player, card);
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int max = 7;
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		if( max > 0 ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			this_test.create_minideck = max;
			this_test.no_shuffle = 1;

			if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test) != 1 ){
				max--;
			}
			if( max > 0 ){
				put_top_x_on_bottom(player, player, max);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_tajuru_archer(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){	// but can cancel
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.required_abilities = KEYWORD_FLYING;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_allies(player, card), player, card);
		}
	}
	return 0;
}

int card_terra_stomper(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	return 0;
}

int card_timbermaw_larva( int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +1/+1 until end of turn for each |H2Forest you control.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  int amt = count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	  if (amt > 0)
		pump_until_eot_merge_previous(player, card, player, card, amt, amt);
	}

  return 0;
}

int card_turntimber_basilisk(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( landfall(player, card, event) == 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			target_must_block_me(player, card, instance->targets[0].player, instance->targets[0].card, 1);
		}
	}
	return 0;
}

int card_turntimber_ranger(int player, int card, event_t event){
	/* Turntimber Ranger	|3|G|G
	 * Creature - Elf Scout Ally 2/2
	 * Whenever ~ or another Ally enters the battlefield under your control, you may put a 2/2 |Sgreen Wolf creature token onto the battlefield. If you do, put a +1/+1 counter on ~. */

	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		add_1_1_counter(player, card);
		generate_token_by_id(player, card, CARD_ID_WOLF);
	}
	return 0;
}

int card_vines_of_vastwood(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int boost = 0;
		if( kicked(player, card) ){
			boost = 4;
		}
		pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
								boost, boost, KEYWORD_SHROUD, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	else if( event == EVENT_CHECK_PUMP && has_mana(player, COLOR_GREEN, 2) ){
		pumpable_power[player] += 4;
		pumpable_toughness[player] += 4;
	}
	kicker(player, card, event, 0, 0, 0, 1, 0, 0);
	return 0;
}

// black
int card_bala_ged_thief(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if (pick_player_duh(player, card, 1-player, 0)){
			int tgt = get_card_instance(player, card)->targets[0].player;
			int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY);

			ec_definition_t ec;
			default_ec_definition(tgt, player, &ec);
			if( amount < hand_count[tgt] ){
				ec.cards_to_reveal = amount;
			}

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_effect_coercion(&ec, &this_test);
		}
		return 0;
	}
	return 0;
}

int card_blood_seeker(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == player && trigger_cause_controller == 1-player
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig =1 ;
		}
		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig =0 ;
		}

		if(  trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					lose_life(1-player, 1);
			}
		}
	}
	return 0;
}

int card_blood_tribute(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.required_subtype = SUBTYPE_VAMPIRE;
	  td.illegal_state = TARGET_STATE_TAPPED;
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;
	  if (target_available(player, card, &td)
		  && do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0) == 0
		  && select_target(player, card, &td, "Select a Vampire you control.", NULL))
		{
		  card_instance_t *instance = get_card_instance(player, card);
		  tap_card(instance->targets[0].player, instance->targets[0].card);
		  set_special_flags(player, card, SF_KICKED);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int life_lost = lose_life(1-player, life[1-player] / 2);

	  if (kicked(player, card))
		gain_life(player, life_lost);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_bloodchief_ascension(int player, int card, event_t event)
{
  /* Bloodchief Ascension	|B
   * Enchantment
   * At the beginning of each end step, if an opponent lost 2 or more life this turn, you may put a quest counter on ~.
   * Whenever a card is put into an opponent's graveyard from anywhere, if ~ has three or more quest counters on it, you may have that player lose 2 life. If
   * you do, you gain 2 life. */

  if (get_trap_condition(1-player, TRAP_LIFE_LOST) >= 2 && eot_trigger(player, card, event))
	add_counter(player, card, COUNTER_QUEST);

  if (count_counters(player, card, COUNTER_QUEST) >= 3)
	{
	  // from library
	  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
	  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
		  && trigger_cause_controller == 1-player && !is_humiliated(player, card))
		{
		  if (event == EVENT_TRIGGER)
			event_result |= duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  lose_life(1-player, num_cards_milled * 2);
			  gain_life(player, num_cards_milled * 2);
			}
		}

	  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, 1-player, RESOLVE_TRIGGER_DUH, NULL))
		{
		  lose_life(1-player, 2);
		  gain_life(player, 2);
		}
	}

  return global_enchantment(player, card, event);
}

int card_bloodghast(int player, int card, event_t event){
	cannot_block(player, card, event);
	if( life[1-player]<=10){
		haste(player, card, event);
	}
	return 0;
}

int card_crypt_ripper(int player, int card, event_t event){
	haste(player, card, event);
	return card_nantuko_shade(player, card, event);
}

int card_desecrated_earth(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! select_target(player, card, &td, "Select target land", NULL) ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			discard(instance->targets[0].player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_disfigure(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! pick_target(&td, "TARGET_CREATURE") ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_feast_of_blood(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		td.required_subtype = SUBTYPE_VAMPIRE;
		td.illegal_abilities = 0;
		td.allowed_controller = player;
		td.preferred_controller = player;
		if( target_available(player, card, &td) > 1 ){
			return 1;
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! pick_target(&td, "TARGET_CREATURE") ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


int card_gatekeeper_of_malakir(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			if( player == AI ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);
				td.zone = TARGET_ZONE_PLAYERS;
				if( would_validate_arbitrary_target(&td, 1-player, -1) ){
					do_kicker(player, card, MANACOST_B(1));
				}
			}
			else{
				do_kicker(player, card, MANACOST_B(1));
			}
		}
	}

	if( comes_into_play(player, card, event) && kicked(player, card) ){
		// choose a player
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if (pick_target(&td, "TARGET_PLAYER")){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_grim_discovery(int player, int card, event_t event)
{
  /* Grim Discovery	|1|B
   * Sorcery
   * Choose one or both - Return target creature card from your graveyard to your hand; and/or return target land card from your graveyard to your hand. */

  return spell_return_one_or_two_cards_from_gy_to_hand(player, card, event, TYPE_CREATURE, TYPE_LAND);
}

int card_guul_draz_specter(int player, int card, event_t event)
{
  /* Guul Draz Specter	|2|B|B
   * Creature - Specter 2/2
   * Flying
   * ~ gets +3/+3 as long as an opponent has no cards in hand.
   * Whenever ~ deals combat damage to a player, that player discards a card. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && hand_count[1-player] == 0)
	event_result += 3;

  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, 0);

  return 0;
}

int card_guul_draz_vampire(int player, int card, event_t event){
	if( life[1-player] <= 10 ){
		if( affect_me(player, card) ){
			if( event == EVENT_POWER ){
				event_result += 2;
			}
			else if( event == EVENT_TOUGHNESS ){
				event_result++;
			}
		}
		fear(player, card, event);
	}
	return 0;
}

int card_hagra_crocodile(int player, int card, event_t event){
	cannot_block(player, card, event);
	if( landfall(player, card, event ) ){
		pump_until_eot_merge_previous(player, card, player, card, 2, 2);
	}
	return 0;
}

int card_hagra_diabolist(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){	// but can cancel
		if (pick_player_duh(player, card, 1-player, 1)){
			card_instance_t* instance = get_card_instance(player, card);
			lose_life(instance->targets[0].player, count_allies(player, card));
		}
	}
	return 0;
}

int card_halo_hunter(int player, int card, event_t event){

	intimidate(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.required_subtype = SUBTYPE_ANGEL;
		td.allow_cancel = 0;

		if (can_target(&td) && select_target(player, card, &td, "Select target Angel.", NULL)){
			card_instance_t *instance = get_card_instance( player, card );
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_heartstabber_mosquito(int player, int card, event_t event){
	kicker(player, card, event, 2, 1, 0, 0, 0, 0);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( select_target(player, card, &td, "Choose a target creature", NULL) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}
	return 0;
}

int card_hideous_end(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) == 1 ){
		if( ! select_target(player, card, &td, "Choose a target creature", NULL) ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
			lose_life(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_kalitas_bloodchief_of_ghet(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				int power = get_power(instance->targets[0].player, instance->targets[0].card);
				int toughness = get_toughness(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );

				token_generation_t token;
				default_token_definition(player, card, CARD_ID_VAMPIRE, &token);
				token.pow = power;
				token.tou = toughness;
				generate_token(&token);
			}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 3, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_malakir_bloodwitch(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int count = count_subtype(player, TYPE_CREATURE, SUBTYPE_VAMPIRE);
		gain_life(player, count);
		lose_life(1-player, count);
	}
	return 0;
}

int card_marsh_casualties(int player, int card, event_t event){

	/* Marsh Casualties	|B|B
	 * Sorcery
	 * Kicker |3
	 * Creatures target player controls get -1/-1 until end of turn. If ~ was kicked, those creatures get -2/-2 until end of turn instead. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_CAN_CAST){
		return can_target(&td);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		pick_target(&td, "TARGET_PLAYER");
	}
	if(event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			int num = -1;
			if( kicked(player, card) ){
				num = -2;
			}
			card_instance_t* instance = get_card_instance(player, card);
			pump_creatures_until_eot(player, card, instance->targets[0].player, 0, num,num, 0,0, NULL);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 3, 0, 0, 0, 0, 0);
	return 0;
}

int card_mindless_null(int player, int card, event_t event){
	if( ! check_battlefield_for_subtype(player, TYPE_CREATURE, SUBTYPE_VAMPIRE) ){
		cannot_block(player, card, event);
	}
	return 0;
}

int card_needlebite_trap(int player, int card, event_t event){

	/* Needlebite Trap	|5|B|B
	 * Instant - Trap
	 * If an opponent gained life this turn, you may pay |B rather than pay ~'s mana cost.
	 * Target player loses 5 life and you gain 5 life. */

	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_LIFE_GAINED) > 0 ){
			COST_COLORLESS -= 5;
			COST_BLACK--;
		}
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			card_instance_t* instance = get_card_instance(player, card);
			lose_life(instance->targets[0].player, 5);
			gain_life( 1-instance->targets[0].player, 5 );
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ob_nixilis_the_fallen(int player, int card, event_t event){

	/* Ob Nixilis, the Fallen	|3|B|B
	 * Legendary Creature - Demon 3/3
	 * Landfall - Whenever a land enters the battlefield under your control, you may have target player lose 3 life. If you do, put three +1/+1 counters on
	 * ~. */

	if( landfall(player, card, event ) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			lose_life(instance->targets[0].player, 3);
			add_1_1_counters(player, card, 3);
		}
	}
	return 0;
}

int card_quest_for_the_gravelord(int player, int card, event_t event){

	/* Quest for the Gravelord	|B
	 * Enchantment
	 * Whenever a creature dies, you may put a quest counter on ~.
	 * Remove three quest counters from ~ and sacrifice it: Put a 5/5 |Sblack Zombie Giant creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE_GIANT, &token);
		token.pow = 5;
		token.tou = 5;
		generate_token(&token);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_counters(player, card, COUNTER_QUEST, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return quest(player, card, event, 3, NULL, 0);
}

int card_ravenous_trap(int player, int card, event_t event){

	/* Ravenous Trap	|2|B|B
	 * Instant - Trap
	 * If an opponent had three or more cards put into his or her graveyard from anywhere this turn, you may pay |0 rather than pay ~'s mana cost.
	 * Exile all cards from target player's graveyard. */

	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_CARDS_TO_GY_FROM_ANYWHERE) >= 3 ){
			COST_COLORLESS -= 2;
			COST_BLACK -= 2;
		}
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			rfg_whole_graveyard(get_card_instance(player, card)->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sadistic_sacrament(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) == 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		select_target(player, card, &td, "Choose a target player", NULL);
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int target = instance->targets[0].player;
		int i=0;
		int num = 3;
		if( kicked(player, card) ){
			num = 15;
		}
		while(i<num){
			int selected = show_deck( player, deck_ptr[target], 500, "Pick a card", 0, 0x7375B0 );
			if( selected != -1){
				int *deck = deck_ptr[target];
				int *rfg = rfg_ptr[target];
				rfg[ count_rfg(target) ] = deck[selected];
				remove_card_from_deck( target, selected );
				i++;
			}
			else{
				break;
			}
		}
		shuffle(target);
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 7, 0, 0, 0, 0, 0);
	return 0;
}

static int effect_sorin_markov2(int player, int card, event_t event){
	if( eot_trigger(player, card, event) ){
		draw_a_card(1-player);
		kill_card(player, card, KILL_SACRIFICE );
	}
	return 0;
}

static int effect_sorin_markov(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( current_phase > PHASE_MAIN2 && instance->targets[0].player != 1 ){
		instance->targets[0].player = 1;
		int i;
		int p = 1-player;
		for(i=0;i<active_cards_count[p];i++){
			card_data_t* card_d = get_card_data(p, i);
			if((card_d->type & TYPE_LAND) && in_play(p, i)){
				tap_card(p, i);
			}
		}
		create_legacy_effect(player, card, &effect_sorin_markov2 );
		card_time_walk(player, card, EVENT_RESOLVE_SPELL);
	}
	return 0;
}

int card_sorin_markov(int player, int card, event_t event){

	/* Sorin Markov	|3|B|B|B
	 * Planeswalker - Sorin (4)
	 * +2: ~ deals 2 damage to target creature or player and you gain 2 life.
	 * -3: Target opponent's life total becomes 10.
	 * -7: You control target player during that player's next turn. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_DAMAGE = 1,
			CHOICE_SET_LIFE,
			CHOICE_MINDSLAVER
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Damage & gain life", can_target(&td), 10, 2,
						"Set life to 10", would_validate_arbitrary_target(&td2, 1-player, -1), (life[1-player]-10)*3, -3,
						"Mindslaver (approx.)", would_validate_arbitrary_target(&td2, 1-player, -1), 20, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
				case CHOICE_DAMAGE:
					pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
					break;

				case CHOICE_SET_LIFE:
				case CHOICE_MINDSLAVER:
				{
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets =  1;
				}
				break;

			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_DAMAGE:
				if ( valid_target(&td)){
					damage_target0(player, card, 2);
					gain_life(player, 2);
				}
				break;

			case CHOICE_SET_LIFE:
				if ( valid_target(&td2)){
					set_life_total(instance->targets[0].player , 10);
				}
				break;

			case CHOICE_MINDSLAVER:
				if ( valid_target(&td2) ){
					create_legacy_effect(player, card, &effect_sorin_markov );
				}
				break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_soul_stair_expedition(int player, int card, event_t event)
{
  /* Soul Stair Expedition	|B
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, you may put a quest counter on ~.
   * Remove three quest counters from ~ and sacrifice it: Return up to two target creature cards from your graveyard to your hand. */

  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH))
	add_counter(player, card, COUNTER_QUEST);

  if (event == EVENT_CAN_ACTIVATE)
	return count_counters(player, card, COUNTER_QUEST) >= 3 && CAN_ACTIVATE0(player, card) && can_sacrifice_this_as_cost(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  if (!charge_mana_for_activated_ability(player, card, MANACOST0))
		return 0;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select up to two target creature cards.");

	  // Translate from EVENT_ACTIVATE to EVENT_CAST_SPELL && affect_me(player, card)
	  int old_affected_card_controller = affected_card_controller;
	  affected_card_controller = player;
	  int old_affected_card = affected_card;
	  affected_card = card;

	  spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, EVENT_CAST_SPELL, 2, &test, 1);

	  affected_card_controller = old_affected_card_controller;
	  affected_card = old_affected_card;

	  kill_card(player, card, KILL_SACRIFICE);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, EVENT_RESOLVE_SPELL, 2, NULL, 1);

  return 0;
}

int card_surrakar_marauder(int player, int card, event_t event){
	if( landfall(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_FEAR);
	}
	return 0;
}

static const char* target_must_have_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return count_counters(player, card, -1) ? NULL : "must have counters";
}
int card_vampire_hexmage(int player, int card, event_t event)
{
  /* Vampire Hexmage	|B|B
   * Creature - Vampire Shaman 2/1
   * First strike
   * Sacrifice ~: Remove all counters from target permanent. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.preferred_controller = ANYBODY;
  if (IS_AI(player) && event != EVENT_RESOLVE_ACTIVATION)
	{
	  /* Prevents the AI from considering removal of counters from permanents without them (thus speculating more on which type of counter to remove from
	   * permanents that *do* have them).  However, it also prevents it from using this ability to kill a Skulking Ghost or activate another whenever-this-
	   * becomes-targeted trigger.  I think this is an acceptable tradeoff. */
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_must_have_counters;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  remove_all_counters(instance->targets[0].player, instance->targets[0].card, -1);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_PERMANENT");
}

int card_vampire_lacerator(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( life[1-player] > 10 ){
			lose_life(player, 1);
		}
	}
	return 0;
}

int card_vampire_nighthawk(int player, int card, event_t event){
	lifelink(player, card, event);
	deathtouch(player, card, event);
	return 0;
}

int card_vampires_bite(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! pick_target(&td, "TARGET_CREATURE") ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int ability = 0;
		if( kicked(player, card) ){
			ability = SP_KEYWORD_LIFELINK;
		}
		pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
								3, 0, 0, ability);
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 2, 1, 0, 0, 0, 0);
	return 0;
}


// red
int card_bladetusk_boar(int player, int card, event_t event){
	intimidate(player, card, event);
	return 0;
}

int card_burst_lightning(int player, int card, event_t event){
	if( kicked(player, card) ){
		return card_lightning_blast(player, card, event);
	}
	int result = card_shock(player, card, event);
	kicker(player, card, event, 4, 0, 0, 0, 0, 0);
	return result;
}

int card_chandra_ablaze(int player, int card, event_t event){

	/* Chandra Ablaze	|4|R|R
	 * Planeswalker - Chandra (5)
	 * +1: Discard a card. If a |Sred card is discarded this way, ~ deals 4 damage to target creature or player.
	 * -2: Each player discards his or her hand, then draws three cards.
	 * -7: Cast any number of red instant and/or sorcery cards from your graveyard without paying their mana costs. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		int priorities[3] = {0, 0, 0};

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.zone = TARGET_ZONE_HAND;

		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		if( event == EVENT_ACTIVATE ){
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED);
			priorities[0] = check_battlefield_for_special_card(player, card, player, 0, &this_test) ? 10 : 0;
			this_test.color = 0;

			priorities[1] = hand_count[player] < 3 && hand_count[1-player] > hand_count[player] ? 15 : 0;

			int c = 0;
			const int *grave = get_grave(player);
			while( grave[c] != -1 ){
					if( is_what(-1, grave[c], TYPE_SPELL) &&
						(get_color_by_internal_id(player, grave[c]) & get_sleighted_color_test(player, card, COLOR_TEST_RED)) &&
						can_legally_play_iid(player, grave[c])
					  ){
						priorities[2]+=5;
					}
					c++;
			}
		}

		enum{
			CHOICE_DISCARD = 1,
			CHOICE_MINI_WOF,
			CHOICE_CAST_FROM_GRAVE
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Discard & damage", can_target(&td), priorities[0], 1,
						"All discard & draw 3", 1, priorities[1], -2,
						"Play spells from grave", 1, priorities[2], -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			switch (choice)
			{
				case CHOICE_DISCARD:
					pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
					break;

				default:
					break;
			}
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
			switch (choice)
			{
				case CHOICE_DISCARD:
				if( hand_count[player] > 0 && valid_target(&td) ){
					/*
					10/1/2009: If you activate Chandra Ablaze's first ability, you don't discard a card until the ability resolves. You may activate the ability even if your hand is empty. You choose a target as you activate the ability even if you have no red cards in hand at that time.
					10/1/2009: As the first ability resolves, nothing happens if your hand is empty. But if you have any cards in hand, you must discard one. If you discard a nonred card, Chandra doesn't deal any damage.
					10/1/2009: If the creature targeted by the first ability is an illegal target by the time it resolves, the entire ability is countered. You won't discard a card.
					*/
					if( player == AI ){
						this_test.color = get_sleighted_color_test(instance->parent_controller, instance->parent_card, COLOR_TEST_RED);
					}
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);

					// was a red card chosen?
					int red_card = get_color(player, selected) & get_sleighted_color_test(instance->parent_controller, instance->parent_card, COLOR_TEST_RED);

					discard_card(player, selected);

					if( red_card ){
						damage_target0(player, card, 4);
					}
				}
				break;

				case CHOICE_MINI_WOF:
				{
					APNAP(p, {
								new_discard_all(p, player);
								draw_cards(p, 3);
							};);
				}
				break;

				case CHOICE_CAST_FROM_GRAVE:
				{
					/* This is a bit complex, since we don't want to allow repeated castings (the cards go back into the graveyard; they don't get exiled or anything
					 * else nearly so convenient), but the list of valid cards would change after each spell if we just cast them one after another, both because the
					 * available targets change after each spell resolves, and because one spell could destroy a Painter's Servant making the other cards red.
					 *
					 * The way it's supposed to work under modern rules is to put all cards on the stack, then pop each one off.  We could, in theory, do that, but it's
					 * difficult to do it cleanly due to the way Manalink handles interrupt timing.  Plus, cancelling during target selection - already problematic when
					 * casting from anywhere but hand - would be particularly disastrous here.  Maybe once put_card_on_stack1() is moved into C.
					 *
					 * So the current approximation is to first choose each card, remove them all from the graveyard, and then, for each, put it back into the graveyard
					 * and cast it from there. */

					const int *grave = get_grave(player);
					int i, sources[500], positions[500], iids[500], num_valid = 0;

					int red = get_sleighted_color_test(player, card, COLOR_TEST_RED);
					int servant = get_global_color_hack(player);

					for (i = 0; i < 500 && grave[i] != -1; ++i){
						if (((get_color_by_internal_id(player, grave[i]) | servant) & red) &&
							is_what(-1, grave[i], TYPE_SPELL) &&
							can_legally_play_iid(player, grave[i])
						   ){
							sources[num_valid] = graveyard_source[player][i];
							positions[num_valid] = i;
							iids[num_valid] = grave[i];
							++num_valid;
						}
					}

					int chosen_iids[500], num_chosen = 0;

					while (num_valid > 0){
						int selected = show_deck(player, iids, num_valid, "Select a card to play.", 0, 0x7375B0);
						if (selected == -1){
							break;
						}

						// Add to list of chosen cards
						chosen_iids[num_chosen] = iids[selected];
						++num_chosen;

						// Remove from graveyard
						int pos = find_in_graveyard_by_source(player, sources[selected], positions[selected]);
						ASSERT(pos != -1);	// nothing has had a chance to remove it from the graveyard except this
						remove_card_from_grave(player, pos);

						// Remove from choosable cards
						--num_valid;
						int j;
						for (j = selected; j < num_valid; ++j){
							sources[j] = sources[j + 1];
							positions[j] = positions[j + 1];
							iids[j] = iids[j + 1];
						}
					}

					// Put each card back in grave and cast it from there
					for (i = 0; i < num_chosen; ++i){
						int pos = raw_put_iid_on_top_of_graveyard(player, chosen_iids[i]);
						increase_trap_condition(player, TRAP_CARDS_TO_GY_FROM_ANYWHERE, -1);    // Since it's not being put into the graveyard now; we're just reversing our previous hack

						play_card_in_grave_for_free(player, player, pos);
						/* play_card_in_grave_for_free() checks can_legally_play_iid().  If that fails, this is a bit inaccurate: the spell was legal to play initially,
						 * but isn't now.  That might be because e.g. it requires two targets, but only one is still valid; in that case, it shouldn't fizzle, but just
						 * resolve on the other target.  Oh well. */
					}
				}
				break;

			}
		}
	}

	return planeswalker(player, card, event, 5);
}

int card_electropotence(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && has_mana_multi(player, MANACOST_XR(2,1)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test)
		  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XR(2,1)))
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_CREATURE);
		  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		  card_instance_t* instance = get_card_instance(player, card);
		  instance->number_of_targets = 0;
		  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
			{
			  int pow = get_power(instance->targets[1].player, instance->targets[1].card);

			  damage_creature(instance->targets[0].player, instance->targets[0].card, pow, player, card);
			}
		}
	}

  return global_enchantment(player, card, event);
}

static int pump_7_0_and_remove_at_eot(int player, int card, event_t event)
{
  if (event == EVENT_POWER)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		event_result += 7;
	}

  return remove_at_eot(player, card, event);
}

int card_elemental_appeal(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 7;
		token.tou = 1;
		token.key_plus = KEYWORD_TRAMPLE;
		token.s_key_plus = SP_KEYWORD_HASTE;
		token.legacy = 1;

		if( kicked(player, card) ){
			// Could do this with TOKEN_ACTION_PUMP_POWER, but that'd result in a second effect card.  Ugly.
			token.special_code_for_legacy = &pump_7_0_and_remove_at_eot;
		} else {
			token.special_code_for_legacy = &remove_at_eot;
		}

		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 5, 0, 0, 0, 0, 0);
	return 0;
}

int card_geyser_glider(int player, int card, event_t event){
	if( landfall(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FLYING, 0);
	}
	return 0;
}

int card_goblin_bushwacker(int player, int card, event_t event){
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, SP_KEYWORD_HASTE);
	}
	kicker(player, card, event, 0, 0, 0, 0, 1, 0);
	return 0;
}

int card_goblin_ruinblaster(int player, int card, event_t event){
	kicker(player, card, event, 0, 0, 0, 0, 1, 0);
	haste(player, card, event);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		if (pick_target_nonbasic_land(player, card, 0)){
			card_instance_t *instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

static int effect_goblin_shortcutter(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	cannot_block(instance->targets[0].player, instance->targets[0].card, event);
	if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_goblin_shortcutter(int player, int card, event_t event){
	if( comes_into_play(player, card, event)  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			card_instance_t *instance = get_card_instance(player, card);
			create_targetted_legacy_effect(player, card, &effect_goblin_shortcutter, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_goblin_war_paint(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, 0, SP_KEYWORD_HASTE, 0, 0, 0);
}

int card_goblin_guide(int player, int card, event_t event)
{
  haste(player, card, event);

  // Whenever ~ attacks, defending player reveals the top card of his or her library. If it's a land card, that player puts it into his or her hand.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  int iid = deck_ptr[1-player][0];
	  reveal_card_iid(player, card, iid);
	  if (iid != -1 && is_what(-1, iid, TYPE_LAND))
		{
		  add_card_to_hand(1-player, iid);
		  obliterate_top_card_of_deck(1-player);
		}
	}

  return 0;
}

int card_hellfire_mongrel(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn == 1-player && hand_count[current_turn] < 3 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			damage_player(current_turn, 2, player, card);
		}
	}
	return 0;
}

int card_highland_berserker(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		pump_subtype_until_eot(player, card, player, SUBTYPE_ALLY, 0, 0, KEYWORD_FIRST_STRIKE, 0 );
	}
	return 0;
}

int card_inferno_trap(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( event == EVENT_MODIFY_COST && get_trap_condition(1-player, TRAP_UNBLOCKED_CREATURES) >= 2 && current_phase >= PHASE_MAIN2 ){
		COST_COLORLESS -= 3;
	}
	else if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_kazuul_warlord(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		int count = 0;
		while(count < active_cards_count[player]){
			card_data_t* card_d = get_card_data(player, count);
			if((card_d->type & TYPE_CREATURE) && in_play(player, count) && has_subtype(player, count, SUBTYPE_ALLY) ){
				add_1_1_counter(player, count );
			}
			count++;
		}
	}
	return 0;
}

int card_lavaball_trap(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_MODIFY_COST && get_trap_condition(1-player, TRAP_LANDS_PLAYED) >= 2 ){
		COST_COLORLESS -= 3;
	}
	else if( event == EVENT_CAN_CAST ){
		if( target_available(player, card, &td) >= 2 ){
			return 1;
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND" );
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		target->state |= STATE_CANNOT_TARGET;
		instance->targets[1].player = instance->targets[0].player;
		instance->targets[1].card = instance->targets[0].card;
		pick_target(&td, "TARGET_LAND" );
		target->state &= ~STATE_CANNOT_TARGET;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int valid = 0;
		if( validate_target(player, card, &td, 0 )){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			valid = 1;
		}
		if( validate_target(player, card, &td, 1 )){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
			valid = 1;
		}
		if( valid == 1 ){
			int p;
			for( p = 0; p < 2; p++){
				int count = 0;
				while(count < active_cards_count[p]){
					card_data_t* card_d = get_card_data(p, count);
					if((card_d->type & TYPE_CREATURE) && in_play(p, count)  ){
						damage_creature(p, count, 4, player, card);
					}
					count++;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_magma_rift(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND );
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td) && can_target(&td1) ){
			return 1;
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td1, "TARGET_LAND") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			pick_target(&td, "TARGET_CREATURE");
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		damage_creature(instance->targets[0].player, instance->targets[0].card, 5, player, card);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mark_of_mutiny(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mire_blight(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( damage_dealt_to_me_arbitrary(instance->damage_target_player, instance->damage_target_card, event, 0, player, card) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY) ;
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_murasa_pyromancer(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){	// but can cancel
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_allies(player, card), player, card);
		}
	}
	return 0;
}

static int effect_obsidian_fireheart(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player;
  int c = instance->damage_target_card;
  if (p > -1)
	{
	  upkeep_trigger_ability(player, card, event, p);
	  if (event == EVENT_UPKEEP_TRIGGER_ABILITY || event == EVENT_SHOULD_AI_PLAY)
		damage_player(p, 1, p, c);

	  if (event == EVENT_STATIC_EFFECTS && !count_counters(p, c, COUNTER_BLAZE))
		kill_card(player, card, KILL_REMOVE);
	}
  else if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

static const char* doesnt_have_a_blaze_counter(int who_chooses, int player, int card){
	return count_counters(player, card, COUNTER_BLAZE) ? "has a blaze counter" : NULL;
}

int card_obsidian_fireheart(int player, int card, event_t event){

	/* Obsidian Fireheart	|1|R|R|R
	 * Creature - Elemental 4/4
	 * |1|R|R: Put a blaze counter on target land without a blaze counter on it. For as long as that land has a blaze counter on it, it has "At the beginning of
	 * your upkeep, this land deals 1 damage to you." */

	if (!IS_GAA_EVENT(event)){
	  return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.extra = (int32_t)doesnt_have_a_blaze_counter;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_BLAZE);
			create_targetted_legacy_effect(player, card, &effect_obsidian_fireheart, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 2, 0, 0, &td, "TARGET_LAND");
}

int card_punishing_fire(int player, int card, event_t event){
	return card_shock(player, card, event);
}

int card_pyromancer_ascension(int player, int card, event_t event){

	/* Pyromancer Ascension	|1|R
	 * Enchantment
	 * Whenever you cast an instant or sorcery spell that has the same name as a card in your graveyard, you may put a quest counter on ~.
	 * Whenever you cast an instant or sorcery spell while ~ has two or more quest counters on it, you may copy that spell. You may choose new targets for the
	 * copy. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	// was an instant or sorcery cast?
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card)
		 && player == reason_for_trigger_controller && player == trigger_cause_controller
		 && ! check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST) )
	{
		int p = trigger_cause_controller;
		int c = trigger_cause;

		if (is_what(p, c, TYPE_SPELL)){
			card_instance_t *instance = get_card_instance(p, c);
			card_instance_t *me = get_card_instance(player, card);

			if(event == EVENT_TRIGGER){
				if( count_counters(player, card, COUNTER_QUEST) < 2 ){
					// is this spell the same as one in the graveyard?
					const int *grave = get_grave(player);
					int i;
					for(i=0;i<count_graveyard(player);i++){
						if( grave[i] == instance->internal_card_id ){
							event_result |= 2;
							me->info_slot = -2;
							break;
						}
					}
				}
				else{
					me->info_slot = instance->internal_card_id;
					event_result |= 1+player;
				}
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				if( me->info_slot == -2 ){
					add_counter(player, card, COUNTER_QUEST);
				}
				else{
					copy_spell_from_stack(player, p, c);
				}
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_ruinous_minotaur(int player, int card, event_t event){
	if( has_combat_damage_been_inflicted_to_opponent(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		td.allowed_controller = player;
		td.preferred_controller = player;
		card_instance_t *instance = get_card_instance( player, card);

		if( can_target(&td) ){
			pick_target(&td, "TARGET_LAND" );
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE );
		}
	}
	return 0;
}

int card_runeflare_trap(int player, int card, event_t event){
	/* Runeflare Trap	|4|R|R
	 * Instant - Trap
	 * If an opponent drew three or more cards this turn, you may pay |R rather than pay ~'s mana cost.
	 * ~ deals damage to target player equal to the number of cards in that player's hand. */

	if( event == EVENT_MODIFY_COST && cards_drawn_this_turn[1-player] >= 3 ){
		COST_COLORLESS -= 4;
		COST_RED--;
	}
	return card_storm_seeker(player, card, event);
}

int card_slaughter_cry(int player, int card, event_t event){
	/* Slaughter Cry	|2|R
	 * Instant
	 * Target creature gets +3/+0 and gains first strike until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3,0, KEYWORD_FIRST_STRIKE,0);
}

int card_spire_barrage(int player, int card, event_t event){

	/* Spire Barrage	|4|R
	 * Sorcery
	 * ~ deals damage to target creature or player equal to the number of |H1Mountains you control. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			damage_target0(player, card, count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN));
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_torch_slinger(int player, int card, event_t event){
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}
	kicker(player, card, event, 1, 0, 0, 0, 1, 0);
	return 0;
}

int card_tuktuk_grunts(int player, int card, event_t event){
	haste(player, card, event);
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_unstable_footing(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			if( can_target(&td) && do_kicker(player, card, 3, 0, 0, 0, 1, 0) ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
				int gc = would_validate_target(player, card, &td, 0) ? 1 : 0;
				if( gc ){
					ai_modifier+=25;
				}
				if( player == HUMAN ){
					pick_target(&td, "TARGET_PLAYER");
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			damage_cannot_be_prevented_until_eot(player, card);
			if( kicked(player, card) && valid_target(&td) ){
				damage_player(instance->targets[0].player, 5, player, card);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_zektar_shrine_expedition(int player, int card, event_t event){
	/* Zektar Shrine Expedition	|1|R
	 * Enchantment
	 * Landfall - Whenever a land enters the battlefield under your control, you may put a quest counter on ~.
	 * Remove three quest counters from ~ and sacrifice it: Put a 7/1 |Sred Elemental creature token with trample and haste onto the battlefield. Exile it at
	 * the beginning of the next end step. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 7;
		token.tou = 1;
		token.key_plus = KEYWORD_TRAMPLE;
		token.s_key_plus = SP_KEYWORD_HASTE;
		token.special_infos = 66;
		generate_token(&token);
	}
	return expedition(player, card, event);
}

// white
int card_armament_master(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card) ){
		if( ! affect_me(player, card) && affected_card_controller == player && has_creature_type( affected_card_controller, affected_card, SUBTYPE_KOR) ){
			event_result += (equipments_attached_to_me(player, card, 0) * EATM_REPORT_TOTAL);
		}
	}

	return 0;
}

int card_arrow_volley_trap(int player, int card, event_t event){

	/* Arrow Volley Trap	|3|W|W
	 * Instant - Trap
	 * If four or more creatures are attacking, you may pay |1|W rather than pay ~'s mana cost.
	 * ~ deals 5 damage divided as you choose among any number of target attacking creatures. */

	if( event == EVENT_MODIFY_COST && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 && count_attackers(current_turn) >= 4 ){
		COST_COLORLESS -= 2;
		COST_WHITE--;
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		char buffer[100];
		int i;
		for (i = 0; i < 5; ++i){
			scnprintf(buffer, 100, "Select target for damage %d", i+1 );
			if (!select_target(player, card, &td, buffer, &instance->targets[i])){
				cancel = 1;
				break;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_bold_defense(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		int p = player;
		int count = 0;
		while(count < active_cards_count[p]){
			card_data_t* card_d = get_card_data(p, count);
			if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
				if( kicked(player, card) ){
					pump_ability_until_eot(player, card, p, count, 2, 2, KEYWORD_FIRST_STRIKE, 0 );
				}
				else{
					pump_until_eot(player, card, p, count, 1, 1 );
				}
			}
			count++;
		}
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 3, 0, 0, 0, 0, 1);
	return 0;
}

int card_brave_the_elements(int player, int card, event_t event){

	/* Brave the Elements	|W
	 * Instant
	 * Choose a color. |SWhite creatures you control gain protection from the chosen color until end of turn. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			int white = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			int count = active_cards_count[player] - 1;
			while (count > -1){
					if (in_play(player, count) && is_what(player, count, TYPE_CREATURE) && (get_color(player, count) & white)){
						pump_ability_until_eot(player, card, player, count, 0, 0, keyword, 0);
					}
					count--;
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_celestial_mantle(int player, int card, event_t event)
{
  /* Celestial Mantle	|3|W|W|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +3/+3.
   * Whenever enchanted creature deals combat damage to a player, double its controller's life total. */

  int packets;
  if ((packets = attached_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER)))
	{
	  int p = get_card_instance(player, card)->damage_target_player;
	  for (; packets > 0; --packets)
		gain_life(p, life[p]);
	}

  return generic_aura(player, card, event, player, 3, 3, 0, 0, 0, 0, 0);
}

int card_conquerors_pledge(int player, int card, event_t event){
	/* Conqueror's Pledge	|2|W|W|W
	 * Sorcery
	 * Kicker |6
	 * Put six 1/1 |Swhite Kor Soldier creature tokens onto the battlefield. If ~ was kicked, put twelve of those tokens onto the battlefield instead. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		int total = 6;
		if( kicked(player, card) ){
			total = 12;
		}
		generate_tokens_by_id(player, card, CARD_ID_KOR_SOLDIER, total);
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 6, 0, 0, 0, 0, 0);
	return 0;
}

int card_day_of_judgment(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		int p;
		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				card_data_t* card_d = get_card_data(p, count);
				if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
					kill_card( p, count, KILL_DESTROY );
				}
				count++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_devout_lightcaster(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.allow_cancel = 0;
		td.required_color = COLOR_TEST_BLACK;

		if( target_available(player, card, &td) ){
			select_target(player, card, &td, "Select target black permanent", NULL);
			card_instance_t *instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE );
		}
	}
	pro_black(player, card, event);
	return 0;
}

int card_emeria_angel (int player, int card, event_t event)
{
  /* Emeria Angel	|2|W|W
   * Creature - Angel 3/3
   * Flying
   * Landfall - Whenever a land enters the battlefield under your control, you may put a 1/1 |Swhite Bird creature token with flying onto the battlefield. */

  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	generate_token_by_id(player, card, CARD_ID_BIRD);

  return 0;
}

int card_felidar_sovereign (int player, int card, event_t event){

	/* Felidar Sovereign	|4|W|W
	 * Creature - Cat Beast 4/6
	 * Vigilance, lifelink
	 * At the beginning of your upkeep, if you have 40 or more life, you win the game. */

	lifelink(player, card, event);

	vigilance(player, card, event);

	if( current_turn==player && life[player] >= 40 && upkeep_trigger(player, card, event) ){
		lose_the_game(1-player);
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int l = life[player] - 20;
		if (l > 0){
			ai_modifier += (player == AI ? 1 : -1) * life[player] * life[player] * life[player];
		}
		if (life[player] >= 40){
			lose_the_game(1-player);
		}
	}

	return 0;
}

int card_iona_shield_of_emeria(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == 1-player && ! is_humiliated(player, card) &&
		instance->targets[1].card > 0
	  ){
		int clr = get_color(affected_card_controller, affected_card);
		card_instance_t *instance2 = get_card_instance( affected_card_controller, affected_card);
		if ((clr & instance->targets[1].card) && !(cards_data[instance2->internal_card_id].type & TYPE_LAND)){
			infinite_casting_cost();
		}
	}
	return 0;
}

int card_journey_to_nowhere(int player, int card, event_t event){

	/* Journey to Nowhere	|1|W
	 * Enchantment
	 * When ~ enters the battlefield, exile target creature.
	 * When ~ leaves the battlefield, return the exiled card to the battlefield under its owner's control. */

	return_from_oblivion(player, card, event);

	if (event == EVENT_CAN_CAST){
		return 1;
	}

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			if (player == AI && instance->targets[0].player == AI){
				ai_modifier -= 64;
			}
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
		} else if (player == AI) {
			ai_modifier -= 48;
		}
	}

	return 0;
}

int card_kabira_evangel(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
		pump_subtype_until_eot(player, card, player, SUBTYPE_ALLY, 0, 0, keyword, 0);
	}
	return 0;
}

int card_kazandu_blademaster(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_1_1_counter(player, card);
	}
	vigilance(player, card, event);
	return 0;
}

int card_kor_aeronaut(int player, int card, event_t event){
	kicker(player, card, event, 1, 0, 0, 0, 0, 1);
	if( comes_into_play(player, card, event) > 0 && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		if( target_available(player, card, &td) ){
			pick_target( &td, "TARGET_CREATURE" );
			card_instance_t *instance = get_card_instance( player, card );
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FLYING, 0);
		}
	}
	return 0;
}

int card_kor_cartographer(int player, int card, event_t event){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		global_tutor(player, player, 1, TUTOR_PLAY_TAPPED, 0, 1, TYPE_LAND, 0, SUBTYPE_PLAINS, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_kor_hookmaster(int player, int card, event_t event){
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) && can_target(&td1) ){
		if( pick_target(&td1, "TARGET_CREATURE") ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

static void kor_outfitter_effect(int player, int card ){
	 target_definition_t td;
	 default_target_definition(player, card, &td, TYPE_ARTIFACT);
	 td.allowed_controller = player;
	 td.preferred_controller = player;
	 td.required_subtype = SUBTYPE_EQUIPMENT;
	 //td.allow_cancel = 0;

	 target_definition_t td1;
	 default_target_definition(player, card, &td1, TYPE_CREATURE);
	 td1.allowed_controller = player;
	 td1.preferred_controller = player;

	 card_instance_t *instance = get_card_instance( player, card );

	 if( can_target(&td) && can_target(&td1) ){
		int eq_controller = -1;
		int eq_card = -1;
		int eq_creature_controller = -1;
		int eq_creature = -1;

		if( select_target(player, card, &td, "Choose an equipment", NULL) ){
			eq_controller = instance->targets[0].player;
			eq_card = instance->targets[0].card;
			instance->number_of_targets = 1;

			if( select_target(player, card, &td1, "Choose a creature to equip", NULL) ){
				eq_creature_controller = instance->targets[0].player;
				eq_creature = instance->targets[0].card;
			}
		}

		if( eq_controller != -1 && eq_card != -1 && eq_creature_controller != -1 && eq_creature != -1 ){
			equip_target_creature( eq_controller, eq_card, eq_creature_controller, eq_creature);
		}
	}
}

int card_kor_outfitter(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		kor_outfitter_effect(player, card);
	}

	return 0;
}

int card_kor_sanctifiers(int player, int card, event_t event){
	kicker(player, card, event, 0, 0, 0, 0, 0, 1);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		td.allow_cancel = 0;
		if( target_available(player, card, &td) ){
			select_target(player, card, &td, "Choose an artifact or enchantment to kill", NULL);
			card_instance_t *instance = get_card_instance( player, card );
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_kor_skyfisher(int player, int card, event_t event ){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);
	if( comes_into_play(player, card, event) ){
		select_target(player, card, &td, "Choose a permanent to bounce", NULL);
		bounce_permanent( instance->targets[0].player, instance->targets[0].card );
	}
	return 0;
}

int card_landbind_ritual(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 2*count_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) );
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_luminarch_ascension(int player, int card, event_t event){

	/* Luminarch Ascension
	 * |1|W
	 * Enchantment
	 * At the beginning of each opponent's end step, if you didn't lose life this turn, you may put a quest counter on ~.
	 * |1|W: Put a 4/4 |Swhite Angel creature token with flying onto the battlefield. Activate this ability only if ~ has four or more quest counters on it. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) && count_counters(player, card, COUNTER_QUEST) >= 4 ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ANGEL, &token);
			token.pow = 4;
			token.tou = 4;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
	}
	if( current_turn == 1-player ){
		if (event == EVENT_BEGIN_TURN){
			instance->targets[1].card = life[player];
			instance->targets[1].player = 1;
			instance->targets[2].player = 1;
		}
		if( instance->targets[1].player == 1 && instance->targets[2].player == 1 ){
			if( life[player] < instance->targets[1].card ){
				instance->targets[2].player = 0;
			}
		}

		if( eot_trigger(player, card, event) ){
			if( instance->targets[2].player == 1 ){
				add_counter(player, card, COUNTER_QUEST);
			}
		}
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		instance->targets[1].player = 0;
		instance->targets[2].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_makindi_shieldmate(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_narrow_escape(int player, int card, event_t event){

	/* Narrow Escape	|2|W
	 * Instant
	 * Return target permanent you control to its owner's hand. You gain 4 life. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT_YOU_CONTROL");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_nimbus_wings(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 2, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_ondu_cleric(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		gain_life(player, count_allies(player, card) );
	}
	return 0;
}

int card_pitfall_trap(int player, int card, event_t event){

	/* Pitfall Trap	|2|W
	 * Instant - Trap
	 * If exactly one creature is attacking, you may pay |W rather than pay ~'s mana cost.
	 * Destroy target attacking creature without flying. */

	if( event == EVENT_MODIFY_COST && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 && count_attackers(current_turn) == 1 ){
		COST_COLORLESS -=2 ;
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;
	td.illegal_abilities |= KEYWORD_FLYING;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		pick_next_target_noload(&td, "Select target attacking creature without flying.");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_quest_for_the_holy_relic(int player, int card, event_t event){

	/* Quest for the Holy Relic	|W
	 * Enchantment
	 * Whenever you cast a creature spell, you may put a quest counter on ~.
	 * Remove five quest counters from ~ and sacrifice it: Search your library for an Equipment card, put it onto the battlefield, and attach it to a creature
	 * you control. Then shuffle your library. */

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_QUEST);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "Select an Equipment card.");
		this_test.subtype = SUBTYPE_EQUIPMENT;

		int equipment = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		if( equipment != -1 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;

			if( pick_target(&td, "TARGET_CREATURE") ){
				card_instance_t* instance = get_card_instance(player, card);
				instance->number_of_targets = 0;
				equip_target_creature(player, equipment, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return quest(player, card, event, 4, NULL, 0);
}

int card_shieldmates_blessing(int player, int card, event_t event){

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
				if( dmg->info_slot <= 3 ){
					dmg->info_slot = 0;
				}
				else{
					dmg->info_slot-=3;
				}
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_steppe_lynx(int player, int card, event_t event){
	if( landfall(player, card, event ) ){
		pump_until_eot_merge_previous(player, card, player, card, 2, 2);
	}
	return 0;
}

int card_sunspring_expedition(int player, int card, event_t event){
	/* Sunspring Expedition	|W
	 * Enchantment
	 * Landfall - Whenever a land enters the battlefield under your control, you may put a quest counter on ~.
	 * Remove three quest counters from ~ and sacrifice it: You gain 8 life. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 8);
	}
	return expedition(player, card, event);
}

int card_windborne_charge(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allow_cancel = 0;
	if( event == EVENT_CAN_CAST ){
		if( target_available(player, card, &td) > 1 ){
			return 1;
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int i;
		for(i=0;i<2;i++){
			select_target(player, card, &td, "Choose target creature", NULL);
			instance->targets[1-i].player = instance->targets[0].player;
			instance->targets[1-i].card = instance->targets[0].card;
			if( i == 0 ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				target->state |= STATE_CANNOT_TARGET;
			}
			else{
				card_instance_t *target = get_card_instance(instance->targets[1].player, instance->targets[1].card);
				target->state &= ~STATE_CANNOT_TARGET;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0;i<2;i++){
			pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card,
									2, 2, KEYWORD_FLYING, 0);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_world_queller(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		if (!in_play(player, card)){	// sacrificed self on a previous upkeep?
			return -1;
		}
		int available_types[6] = { TYPE_CREATURE, TYPE_ARTIFACT, TYPE_LAND, TYPE_ENCHANTMENT, TARGET_TYPE_PLANESWALKER, -1 };

		int ai_choice = 5;
		int k;
		for (k = 0; k <= 4; ++k){
			if (count_permanents_by_type(player, available_types[k]) == 0
				&& count_permanents_by_type(1-player, available_types[k]) > 0 ){
				ai_choice = k;
				break;
			}
		}

		int choice = do_dialog(player, player, card, -1, -1, " Creature\n Artifact\n Land\n Enchantment\n Planeswalker\n Cancel", ai_choice);
		if( choice == 5 ){ return -1; }

		int type = available_types[choice];
		impose_sacrifice(player, card, player, 1, type, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		impose_sacrifice(player, card, 1-player, 1, type, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

// blue
int card_aether_figment(int player, int card, event_t event){
	kicker(player, card, event, 3, 0, 0, 0, 0, 0);
	unblockable(player, card, event);
	if( comes_into_play(player, card, event) && kicked(player, card) ){
		add_1_1_counters(player, card, 2);
	}
	return 0;
}

int card_archive_trap(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_DECK_WAS_SEARCHED) ==1 ){
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
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return would_validate_arbitrary_target(&td, 1-player, -1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Play for free\n Pay full cost\n Cancel", 0);
			}
			if( choice == 1 ){
				charge_mana_from_id(player, card, event, get_id(player, card));
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
		}
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 13);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_archmage_ascension(int player, int card, event_t event)
{
  /* Archmage Ascension	|2|U
   * Enchantment
   * At the beginning of each end step, if you drew two or more cards this turn, you may put a quest counter on ~.
   * As long as ~ has six or more quest counters on it, if you would draw a card, you may instead search your library for a card, put that card into your hand,
   * then shuffle your library. */

  if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw
	  && count_counters(player, card, COUNTER_QUEST) >= 6)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_AI(player);

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  test_definition_t test;
		  default_test_definition(&test, 0);
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
		  suppress_draw = 1;
		}
	}

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player
	  && cards_drawn_this_turn[player] >= 2 && count_counters(player, card, COUNTER_QUEST) < 6
	  && !is_humiliated(player, card)
	 ){
	  if (event == EVENT_TRIGGER){
		  event_result |= RESOLVE_TRIGGER_AI(player);
	  }
	  if (event == EVENT_RESOLVE_TRIGGER){
		  add_counter(player, card, COUNTER_QUEST);
	  }
  }

  return global_enchantment(player, card, event);
}

int card_caller_of_gales(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_cosis_trickster(int player, int card, event_t event){
	return 0;
}

int card_gomazoa(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0) ){
			if( player == HUMAN ){
				return 1;
			}
			else{
				if( current_turn != player && current_phase == PHASE_AFTER_BLOCKING && instance->blocking < 255 ){
					return 1;
				}
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->blocking < 255 ){
				shuffle_into_library(1-player, instance->blocking);
			}
			if( in_play(instance->parent_controller, instance->parent_card) ){
				shuffle_into_library(instance->parent_controller, instance->parent_card);
			}
	}
	return 0;
}

int card_hedron_crab(int player, int card, event_t event){
	if( landfall(player, card, event ) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		select_target(player, card, &td, "Select target player", NULL);
		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 1;
		mill( instance->targets[0].player, 3);
	}
	return 0;
}

int card_into_the_roil(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_LAND;
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! select_target(player, card, &td, "Select target nonland permanent", NULL) ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			bounce_permanent( instance->targets[0].player, instance->targets[0].card);
			if( kicked(player, card) ){
				draw_a_card(player);
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}
	kicker(player, card, event, 1, 0, 1, 0, 0, 0);
	return 0;
}

int card_ior_ruin_expedition(int player, int card, event_t event){
	/* Ior Ruin Expedition	|1|U
	 * Enchantment
	 * Landfall - Whenever a land enters the battlefield under your control, you may put a quest counter on ~.
	 * Remove three quest counters from ~ and sacrifice it: Draw two cards. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
	}
	return expedition(player, card, event);
}

int card_lethargy_trap(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card );
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.required_state = TARGET_STATE_ATTACKING;
		td.illegal_abilities = 0;
		int p = current_turn;
		int i;
		for(i=0;i<active_cards_count[p];i++){
			instance->targets[0].player = p;
			instance->targets[0].card = i;
			if( validate_target(player, card, &td, 0) ){
				pump_until_eot(player, card, p, i, -3, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}
	 else if( event == EVENT_MODIFY_COST && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2  && current_turn == 1-player){
		int i=0;
		int count = 0;
		int p = 1-player;
		for(i=0;i<active_cards_count[p]; i++){
			card_instance_t *instance2 = get_card_instance(p, i);
			if( instance2->state & STATE_ATTACKING ){
				count++;;
			}
		}
		if( count >= 3 ){
			COST_COLORLESS -=3 ;
		}
	}
	return 0;
}

int card_living_tsunami(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn==player){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_LAND);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			if( select_target(player, card, &td, "Choose a land to bounce", NULL ) ){
				card_instance_t *instance= get_card_instance(player, card);
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}
	return 0;
}

int card_lorthos_the_tidemaker(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever ~ attacks, you may pay |8. If you do, tap up to eight target permanents. Those permanents don't untap during their controllers' next untap steps.
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allow_cancel = 3;
	  if (player == AI || ai_is_speculating == 1)
		td.allowed_controller = td.preferred_controller = 1-player;

	  int i, trigger_mode;
	  if ((player == AI || ai_is_speculating == 1) && has_mana(player, COLOR_ANY, 8) && can_target(&td))
		trigger_mode = RESOLVE_TRIGGER_MANDATORY;
	  else
		trigger_mode = RESOLVE_TRIGGER_OPTIONAL;

	  card_instance_t* instance = get_card_instance(player, card);

	  if (declare_attackers_trigger(player, card, event, trigger_mode | DAT_STORE_IN_INFO_SLOT, player, card)
		  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 8)
		  && pick_up_to_n_targets(&td, "TARGET_PERMANENT", 8))
		for (i = 0; i < instance->number_of_targets; ++i)
		  does_not_untap_effect(player, card, instance->targets[i].player, instance->targets[i].card, EDNT_TAP_TARGET, 1);
	}

  return 0;
}

int card_merfolk_seastalkers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.illegal_abilities |= KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 1, 0, 0, 0, 0, &td, "FLOOD");
}

int card_merfolk_wayfinder(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int i=0;
		int *deck = deck_ptr[player];
		for(i=0;i<3;i++){
			int card_added = add_card_to_hand(player, deck[0] );
			reveal_card(player, card, player, card_added);
			remove_card_from_deck( player, 0 );
			if( !has_subtype(player, card_added, SUBTYPE_ISLAND) ){
				put_on_top_of_deck(player, card_added);
				put_top_card_of_deck_to_bottom(player);
			}
		}
	}
	return 0;
}

int card_mindbreak_trap(int player, int card, event_t event){
	if( event == EVENT_MODIFY_COST && get_specific_storm_count(1-player) >= 3 ){
		COST_BLUE -= 2;
		COST_COLORLESS -= 2;
	}
	return card_dissipate(player, card, event);
}

int card_paralyzing_grasp(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_quest_for_ancient_secrets(int player, int card, event_t event)
{
  /* Quest for Ancient Secrets	|U
   * Enchantment
   * Whenever a card is put into your graveyard from anywhere, you may put a quest counter on ~.
   * Remove five quest counters from ~ and sacrifice it: Target player shuffles his or her graveyard into his or her library. */

  // from library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == 1-player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

	  if (event == EVENT_RESOLVE_TRIGGER)
		add_counters(player, card, COUNTER_QUEST, num_cards_milled);
	}

  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, 1-player, RESOLVE_TRIGGER_DUH, NULL))
	add_counter(player, card, COUNTER_QUEST);

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	reshuffle_grave_into_deck(get_card_instance(player, card)->targets[0].player, 0);

  return quest(player, card, event, 5, &td, "TARGET_PLAYER");
}

int card_reckless_scholar(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int p = instance->targets[0].player;
			draw_a_card(p);
			discard(p, 0, player);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_rite_of_replication(int player, int card, event_t event){
	/* Rite of Replication	|2|U|U
	 * Sorcery
	 * Kicker |5
	 * Put a token onto the battlefield that's a copy of target creature. If ~ was kicked, put five of those tokens onto the battlefield instead. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return target_available(player, card, &td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! pick_target(&td, "TARGET_CREATURE") ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			token.qty = kicked(player, card) ? 5 : 1;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	kicker(player, card, event, 5, 0, 0, 0, 0, 0);
	return 0;
}

int card_roil_elemental(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( landfall(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			int legacy = gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
			if( legacy > -1 ){
				instance->info_slot++;
				instance->targets[  1 + instance->info_slot ].card = legacy;
			}
		}
	}
	else if( leaves_play(player, card, event) ){
		int i;
		for(i=2;i<2 + instance->info_slot;i++){
			kill_card(player, instance->targets[i].card, KILL_SACRIFICE );
		}
	}
	return 0;
}

int card_sea_gate_loremaster(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY) );
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_seascape_aerialist(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		int count =0;
		while(count < active_cards_count[player]){
			card_data_t* card_d = get_card_data(player, count);
			if((card_d->type & TYPE_CREATURE) && in_play(player, count) && has_subtype(player, count, SUBTYPE_ALLY) ){
				pump_ability_until_eot(player, card, player, count, 0, 0, KEYWORD_FLYING, 0 );
			}
			count++;
		}
	}
	return 0;
}

int card_shoal_serpent(int player, int card, event_t event){
	if( landfall(player, card, event) ){
		int leg = pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_DEFENDER, 0);
		if (leg != -1){
			get_card_instance(player, leg)->targets[4].player = 0;	// remove keyword
		}
	}
	return 0;
}

int card_spell_pierce(int player, int card, event_t event)
{
  /* Spell Pierce	|U
   * Instant
   * Counter target noncreature spell unless its controller pays |2. */

  target_definition_t td;
  counterspell_target_definition(player, card, &td, 0);
  td.illegal_type = TYPE_CREATURE;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  counterspell_resolve_unless_pay_x(player, card, NULL, 0, 2);
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  return counterspell(player, card, event, &td, 0);
}

int card_sphinx_of_jwar_isle(int player, int card, event_t event){
	if( player == HUMAN ){
		reveal_top_card(player, card, event);
	}
	return 0;
}

int card_sphinx_of_lost_truths (int player, int card, event_t event){
	kicker(player, card, event, 1, 0, 1, 0, 0, 0);
	if( comes_into_play(player, card, event) ){
		//draw_cards(player, 3);
		draw_a_card(player);
		draw_a_card(player);
		draw_a_card(player);
		if( ! kicked(player, card) ){
			discard(player, 0, 0);
			discard(player, 0, 0);
			discard(player, 0, 0);
		}
	}
	return 0;
}

int card_spreading_seas(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}


	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=25;
		pick_target(&td, "TARGET_LAND");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_a_card(player);
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
			instance->targets[1].card = get_internal_card_id_from_csv_id(CARD_ID_ISLAND);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	if( in_play(player, card) && instance->targets[0].player > -1 && instance->targets[1].card > -1 ){
		if( event == EVENT_CHANGE_TYPE ){
			if(affect_me(instance->targets[0].player, instance->targets[0].card)){
				event_result = instance->targets[1].card;
			}
		}
	}
	return 0;
}

int card_summoners_bane(int player, int card, event_t event){
	int result = card_remove_soul(player, card, event);
	if( event == EVENT_RESOLVE_SPELL && spell_fizzled != 1 ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ILLUSION, &token);
		token.special_infos = 66;
		token.pow = 2;
		token.tou = 2;
		generate_token(&token);
	}
	return result;
}

int card_tempest_owl(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	kicker(player, card, event, 4, 0, 1, 0, 0, 0);
	if( comes_into_play(player, card, event) ){
		if( kicked(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_PERMANENT );
			int i;
			for(i=0;i<3;i++){
				if( target_available(player, card, &td) == 0 || spell_fizzled == 1 ){
					break;
				}
				if( pick_target(&td, "TARGET_PERMANENT" ) ){
					card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
					target->state |= STATE_CANNOT_TARGET;
					instance->targets[2-i].player = instance->targets[0].player;
					instance->targets[2-i].card = instance->targets[0].card;
				}
			}
			int j;
			for(j=0;j<i;j++){
				card_instance_t *target = get_card_instance(instance->targets[j].player, instance->targets[j].card);
				target->state &= ~STATE_CANNOT_TARGET;
				tap_card(instance->targets[j].player, instance->targets[j].card);
			}
		}
	}
	return 0;
}

int card_trapfinders_trick(int player, int card, event_t event){

	/* Trapfinder's Trick	|1|U
	 * Sorcery
	 * Target player reveals his or her hand and discards all Trap cards. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			ec_definition_t ec;
			default_ec_definition(get_card_instance(player, card)->targets[0].player, player, &ec);
			ec.effect = EC_DISCARD | EC_ALL_WHICH_MATCH_CRITERIA;

			test_definition_t test;
			new_default_test_definition(&test, 0, "");
			test.subtype = SUBTYPE_TRAP;

			new_effect_coercion(&ec, &test);
		}

		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_trapmakers_snare(int player, int card, event_t event){

	/* Trapmaker's Snare	|1|U
	 * Instant
	 * Search your library for a Trap card, reveal it, and put it into your hand. Then shuffle your library. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, 0, "Select a Trap card.");
		test.subtype = SUBTYPE_TRAP;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_whiplash_trap(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;
	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_CREATURES_PLAYED) >= 2 ){
			COST_BLUE --;
			COST_COLORLESS -= 3;
		}
	}
	else if( event == EVENT_CAN_CAST && target_available(player, card, &td) > 1 ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE" ) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				new_pick_target(&td, "TARGET_CREATURE", 1, 1);
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				if( validate_target(player, card, &td, i) ){
					bounce_permanent(instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_hellkite_charger(int player, int card, event_t event)
{
  haste(player, card, event);

  // Whenever ~ attacks, you may pay |5|R|R. If you do, untap all attacking creatures and after this phase, there is an additional combat phase.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XR(5,2)))
	relentless_assault_effect(player, card);

  return 0;
}

int card_noble_vestige(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ANY);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_ANY_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot >  0){
				dmg->info_slot--;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_DAMAGE");
}

int card_tanglesap(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		fog_special(player, card, ANYBODY, FOG_COMBAT_DAMAGE_ONLY | FOG_CREATURES_WITHOUT_TRAMPLE);
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		 return card_fog(player, card, event);
	}

	return 0;
}
