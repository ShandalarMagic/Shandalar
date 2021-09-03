#include "manalink.h"
int do_multikicker(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	int result = 0;
	int diff = 0;
	if( ! played_for_free(player, card) ){
		diff = MIN(0, cards_ptr[get_id(player, card)]->req_colorless+true_get_updated_casting_cost(player, card, -1, event, -1));
	}
	int cless = colorless+diff;
	while( 1 ){
			int ctp = MAX(0, cless);
			if( cless > 0 ){
				ctp = MIN(cless, colorless);
			}
			cless+=colorless;
			if( has_mana_multi(player, ctp, black, blue, green, red, white) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
				if( choice == 0 ){
					charge_mana_multi(player, ctp, black, blue, green, red, white);
					if( spell_fizzled != 1 ){
						set_special_flags(player, card, SF_KICKED);
						result++;
					}
				}
				else{
					break;
				}
			}
			else{
				break;
			}
	}
	return result;
}

int multikicker(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		if( ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
			if( spell_fizzled != 1 ){
				instance->info_slot = do_multikicker(player, card, event, colorless, black, blue, green, red, white);
			}
		}
	}
	return 0;
}

static int multikicked(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	return instance->info_slot;
}

int card_abyssal_persecutor(int player, int card, event_t event){
	cannot_lose_the_game(player, card, event, 1-player);
	return 0;
}

int card_admonition_angel(int player, int card, event_t event){

	/* Admonition Angel	|3|W|W|W
	 * Creature - Angel 6/6
	 * Flying
	 * Landfall - Whenever a land enters the battlefield under your control, you may exile target nonland permanent other than ~.
	 * When ~ leaves the battlefield, return all cards exiled with it to the battlefield under their owners' control. */

	card_instance_t *instance = get_card_instance(player, card);
	if( landfall(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.special = TARGET_SPECIAL_NOT_ME;

		if( can_target(&td) && pick_target(&td, "TARGET_NONLAND_PERMANENT") ){
			instance->number_of_targets = 1;
			if( instance->targets[1].player < 2 ){
				instance->targets[1].player = 2;
			}
			int pos = instance->targets[1].player;
			if( pos > 9 ){
				return 0;
			}
			instance->targets[pos].player = instance->targets[0].player;
			int id = rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
			if( id > available_slots ){
				id-=available_slots;
				instance->targets[pos].player = 1-instance->targets[0].player;
			}
			instance->targets[pos].card = id;
			create_card_name_legacy(player, card, id);
			instance->targets[1].player++;
		}
	}
	if( leaves_play(player, card, event) ){
		int pos = instance->targets[1].player;
		int i;
		for(i=2; i<pos; i++){
			if( instance->targets[i].player != -1 ){
				if( check_rfg(instance->targets[i].player, instance->targets[i].card) ){
					int int_id = get_internal_card_id_from_csv_id(instance->targets[i].card);
					int card_added = add_card_to_hand(instance->targets[i].player, int_id);
					put_into_play(instance->targets[i].player, card_added);
				}
			}
		}
	}
	return 0;
}

int card_aether_tradwinds(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			new_pick_target(&td1, "TARGET_PERMANENT", 1, 1);
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		if( validate_target(player, card, &td1, 1) ){
			bounce_permanent(instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_agadeem_occultist(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 0, 0, 0, 0, 0, 0, 0) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.cmc = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY)+1;
			this_test.cmc_flag = 3;
			if( new_special_count_grave(1-player, &this_test) > 0 ){
				return graveyard_has_shroud(2);
			}
		}
	}
	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY);
			char buffer[100];
			scnprintf(buffer, 100, " Select a creature card with CMC %d or less", amount);
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			this_test.cmc = amount+1;
			this_test.cmc_flag = 3;
			if( new_select_target_from_grave(player, card, 1-player, 0, AI_MAX_VALUE, &this_test, 0) > -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, 1-player, 0);
		if( selected > -1 ){
			const int *grave = get_grave(1-player);
			if( get_cmc_by_internal_id(grave[selected]) <= count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY) ){
				reanimate_permanent(player, instance->parent_card, 1-player, selected, REANIMATE_DEFAULT);
			}
		}
	}
	return 0;
}

int card_akoum_battlesinger(int player, int card, event_t event){
	haste(player, card, event);
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		pump_subtype_until_eot(player, card, player, SUBTYPE_ALLY, 1, 0, 0, 0);
	}
	return 0;
}

int card_amulet_of_vigor(int player, int card, event_t event)
{
  test_definition_t this_test;
  default_test_definition(&this_test, TYPE_PERMANENT);
  this_test.state = STATE_TAPPED;
  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  untap_card(instance->targets[1].player, instance->targets[1].card);
	}
  return 0;
}

int card_anowon_the_ruin_sage(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.subtype = SUBTYPE_VAMPIRE;
	this_test.subtype_flag = 1;
	if( !is_humiliated(player, card) ){
		if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			int count = count_upkeeps(player);
			if(event == EVENT_TRIGGER && count > 0 && check_battlefield_for_special_card(player, card, 2, 0, &this_test) ){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					card_instance_t *instance= get_card_instance(player, card);
					card_data_t* card_d = &cards_data[ instance->internal_card_id ];
					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					while( count > 0 ){
							ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
							count--;
					}
			}
		}
	}
	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int i;
		for(i=0; i<2; i++){
			if( check_battlefield_for_special_card(player, card, i, 0, &this_test) ){
				impose_sacrifice(player, card, i, 1, TYPE_CREATURE, 0, SUBTYPE_VAMPIRE, 1, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return 0;
}

int card_arbor_elf(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_FOREST;
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance(player,card);
	if( event == EVENT_ACTIVATE && player == AI ){
		td.required_state = TARGET_STATE_TAPPED;
		if( can_target(&td) ){
			ai_modifier+=50;
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card );
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_LAND");
}

int card_archon_of_redemption(int player, int card, event_t event){

	/* Archon of Redemption	|3|W|W
	 * Creature - Archon 3/4
	 * Flying
	 * Whenever ~ or another creature with flying enters the battlefield under your control, you may gain life equal to that creature's power. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
	   reason_for_trigger_controller == affected_card_controller &&
	   trigger_cause_controller == player
	  ){
		int trig = 0;
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
			check_for_ability(trigger_cause_controller, trigger_cause, KEYWORD_FLYING)
		  ){
			trig = 1;
		}
		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}
		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
				get_card_instance(trigger_cause_controller, trigger_cause)->regen_status |= KEYWORD_RECALC_TOUGHNESS|KEYWORD_RECALC_POWER;
				gain_life(player, get_power(trigger_cause_controller, trigger_cause));
			}
		}
	}
	return 0;
}

int card_avenger_of_zendikar(int player, int card, event_t event){
	/* Avenger of Zendikar	|5|G|G
	 * Creature - Elemental 5/5
	 * When ~ enters the battlefield, put a 0/1 |Sgreen Plant creature token onto the battlefield for each land you control.
	 * Landfall - Whenever a land enters the battlefield under your control, you may put a +1/+1 counter on each Plant creature you control. */

	if( comes_into_play(player, card, event) ){
	   int total = count_permanents_by_type(player, TYPE_LAND);
	   if( total > 0 ){
		  generate_tokens_by_id(player, card, CARD_ID_PLANT, total);
	   }
	}

	if( landfall_mode(player, card, event, RESOLVE_TRIGGER_DUH)  ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && has_subtype(player, count, SUBTYPE_PLANT) && is_what(player, count, TYPE_CREATURE) ){
					add_1_1_counter(player, count);
				}
				count++;
		}
	}
	return 0;
}

int card_basilisk_collar(int player, int card, event_t event)
{
  return vanilla_equipment(player, card, event, 2, 0,0, 0,SP_KEYWORD_DEATHTOUCH|SP_KEYWORD_LIFELINK);
}

int card_bazaar_trader(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TWIDDLE") ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			return would_valid_target(&td1);
		}
	}
	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && select_target(player, card, &td, "Select target artifact, creature or land you control.", &instance->targets[1]) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 2;
			instance->state |= STATE_TAPPED;
		} else {
			instance->number_of_targets = 0;
			cancel = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td1, 0) && validate_target(player, card, &td, 1) ){
			give_control(player, instance->parent_card, instance->targets[1].player, instance->targets[1].card);
		}
	}
	return 0;
}

int card_bestial_menace(int player, int card, event_t event){
	/* Bestial Menace	|3|G|G
	 * Sorcery
	 * Put a 1/1 |Sgreen Snake creature token, a 2/2 |Sgreen Wolf creature token, and a 3/3 |Sgreen Elephant creature token onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			generate_token_by_id(player, card, CARD_ID_ELEPHANT);
			generate_token_by_id(player, card, CARD_ID_WOLF);
			generate_token_by_id(player, card, CARD_ID_SNAKE);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_bloodhusk_ritualist(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;
	card_instance_t *instance = get_card_instance(player, card);
	if( comes_into_play(player, card, event) && instance->info_slot > 0 && can_target(&td) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			new_multidiscard(instance->targets[0].player, instance->info_slot, 0, player);
		}
	}
	return multikicker(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_bojuka_bog(int player, int card, event_t event){
	/* Bojuka Bog	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, exile all cards from target player's graveyard.
	 * |T: Add |B to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			rfg_whole_graveyard(instance->targets[0].player);
		}
	}

	return mana_producer(player, card, event);
}

int card_butcher_of_malakir(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, 2) ){
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		impose_sacrifice(player, card, 1-player, instance->targets[11].card, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		instance->targets[11].card = 0;
	}
	return 0;
}

int card_calcite_snapper(int player, int card, event_t event)
{
  // Landfall - Whenever a land enters the battlefield under your control, you may switch ~'s power and toughness until end of turn.
  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) )
	switch_power_and_toughness_until_eot(player, card, player, card);
  return 0;
}

int card_celestial_colonnade(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, 3, 0, 1, 0, 0, 1);
}

int card_celestial_colonnade_animated(int player, int card, event_t event){
	vigilance(player, card, event);
	return manland_animated(player, card, event, 3, 0, 1, 0, 0, 1);
}

int card_chain_reaction(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int num = count_subtype(2, TYPE_CREATURE, -1);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, 2, num, NDA_ALL_CREATURES, &this_test);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_comet_storm(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->number_of_targets = 0;
			instance->info_slot = x_value;
			int trgs = 0;
			if( player == AI ){
				if( would_validate_arbitrary_target(&td, 1-player, -1) ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					trgs++;
				}
				while( can_target(&td1) && (trgs == 0 || has_mana(player, COLOR_COLORLESS, trgs+1)) && trgs < 18 ){
						if( select_target(player, card, &td1, "Select target creature.", &(instance->targets[trgs])) ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
							trgs++;
						}
						else{
							break;
						}
				}
			}
			else{
				int targeted_players[2] = {0, 0};
				while( 1 ){
						if( ! can_target(&td1) ){
							if( targeted_players[0] ){
								if( ! would_validate_arbitrary_target(&td, 1, -1) ){
									break;
								}
							}
							if( targeted_players[1] ){
								if( ! would_validate_arbitrary_target(&td, 0, -1) ){
									break;
								}
							}
						}
						if( !(trgs == 0 || has_mana(player, COLOR_COLORLESS, trgs+1)) || trgs > 18  ){
							break;
						}

						if( select_target(player, card, &td, "Select target creature or player.", &(instance->targets[trgs])) ){
							if( instance->targets[trgs].card != -1 ){
								state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
								trgs++;
							}
							else{
								if( targeted_players[instance->targets[trgs].player] != 1 ){
									targeted_players[instance->targets[trgs].player] = 1;
									trgs++;
								}
								else{
									break;
								}
							}
						}
						else{
							break;
						}
				}
			}
			int i;
			for(i=0; i<trgs; i++){
				if( instance->targets[i].card != -1 ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				}
			}
			charge_mana(player, COLOR_COLORLESS, trgs-1);
			if( spell_fizzled != 1 ){
				if( trgs > 1 ){
					set_special_flags(player, card, SF_KICKED);
				}
				instance->number_of_targets = trgs;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, instance->info_slot, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_creeping_tar_pit(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, 1, 1, 1, 0, 0, 0);
}

int card_creeping_tar_pit_animated(int player, int card, event_t event){
	unblockable(player, card, event);
	return manland_animated(player, card, event, 1, 1, 1, 0, 0, 0);
}

int card_cunning_sparkmage(int player, int card, event_t event){
	/*
	  Cunning |2|R
	  Creature - Human Shaman 0/1
	  Haste
	  {T}: Cunning Sparkmage deals 1 damage to target creature or player.
	*/
	haste(player, card, event);
	return card_prodigal_sorcerer(player, card, event);
}

int card_deaths_shadow(int player, int card, event_t event){
	if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
		if( affect_me(player, card) && ! check_battlefield_for_id(player, CARD_ID_LICH) ){
			event_result -= life[player];
		}
	}
	return 0;
}

int card_dragonmaster_outcast( int player, int card, event_t event){
	/* Dragonmaster Outcast	|R
	 * Creature - Human Shaman 1/1
	 * At the beginning of your upkeep, if you control six or more lands, put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

	upkeep_trigger_ability(player, card, event, player);
	if( event == EVENT_UPKEEP_TRIGGER_ABILITY && count_permanents_by_type(player, TYPE_LAND) >= 6){
		generate_token_by_id(player, card, CARD_ID_DRAGON);
	}
	return 0;
}

int card_dread_statuary(int player, int card, event_t event){
	return manland_normal(player, card, event, 4, 0, 0, 0, 0, 0);
}

int card_dread_statuary_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, 4, 0, 0, 0, 0, 0);
}

int card_everflowing_chalice(int player, int card, event_t event)
{
  /* Everflowing Chalice	|0
   * Artifact
   * Multikicker |2
   * ~ enters the battlefield with a charge counter on it for each time it was kicked.
   * |T: Add |1 to your mana pool for each charge counter on ~. */
	card_instance_t* instance = get_card_instance(player, card);

	// do not let the AI cast this for 0
	if (IS_AI(player) && event == EVENT_MODIFY_COST && !has_mana(AI, COLOR_ANY, 2))
		COST_COLORLESS += 2;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
			instance->info_slot = 0;
			int diff = 0;
			if( ! played_for_free(player, card) ){
				diff = MIN(0, cards_ptr[get_id(player, card)]->req_colorless+true_get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, -1));
			}
			int cless = 2+diff;
			while( 1 ){
					int ctp = MAX(0, cless);
					if( cless > 0 ){
						ctp = MIN(cless, 2);
					}
					cless+=2;
					if( has_mana(player, COLOR_ARTIFACT, ctp) ){
						int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
						if( choice == 0 ){
							charge_mana(player, COLOR_ARTIFACT, ctp);
							if( spell_fizzled != 1 ){
								set_special_flags(player, card, SF_KICKED);
								instance->info_slot++;
							}
						}
						else{
							break;
						}
					}
					else{
						break;
					}
			}
			if( spell_fizzled != 1 ){
				enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);
			}
		}
	}
	if( event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE && affect_me(player, card)){
		enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card))
	declare_mana_available(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_CHARGE));

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card) && (!IS_AI(player) || count_counters(player, card, COUNTER_CHARGE) > 0);

  if (event == EVENT_ACTIVATE)
	produce_mana_tapped(player, card, COLOR_COLORLESS, count_counters(player, card, COUNTER_CHARGE));

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += (player == AI ? 1 : -1) * count_counters(player, card, COUNTER_CHARGE);

  return 0;
}

int card_explore2(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST && affect_me(player, card) ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 1);
			int effect = create_legacy_effect(player, card, &check_playable_lands_legacy);
			card_instance_t *leg = get_card_instance(player, effect);
			leg->targets[2].card = get_id(player, card);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_eye_of_ugin(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		int p = affected_card_controller;
		int c = affected_card;
		if(has_subtype(p, c, SUBTYPE_ELDRAZI) && is_colorless(p, c) ){
			COST_COLORLESS-=2;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}
	if (event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a colorless creature card.");
		this_test.color = COLOR_TEST_ANY_COLORED;
		this_test.color_flag = DOESNT_MATCH;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(7), 0, NULL, NULL);
}

int card_hada_freeblade(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_halimar_depths(int player, int card, event_t event){
	/* Halimar Depths	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, look at the top three cards of your library, then put them back in any order.
	 * |T: Add |U to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		rearrange_top_x(player, player, 3);
	}

	return mana_producer_fixed(player, card, event, COLOR_BLUE);
}

int card_halimar_excavator(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;
	card_instance_t *instance = get_card_instance( player, card );
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
		instance->number_of_targets = 1;
		mill(instance->targets[0].player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY));
	}
	return 0;
}

int card_hammer_of_ruin(int player, int card, event_t event)
{
  /* Hammer of Ruin	|2
   * Artifact - Equipment
   * Equipped creature gets +2/+0.
   * Whenever equipped creature deals combat damage to a player, you may destroy target Equipment that player controls.
   * Equip |2 */

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS|DDBM_TRIGGER_OPTIONAL))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT);
	  td.required_subtype = SUBTYPE_EQUIPMENT;

	  int p;
	  for (p = 0; p <= 1; ++p)
		{
		  td.allowed_controller = td.preferred_controller = p;
		  for (; times_damaged[p] > 0; --times_damaged[p])
			{
			  instance->number_of_targets = 0;
			  if (pick_target(&td, "TARGET_EQUIPMENT"))
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

  return vanilla_equipment(player, card, event, 2, 2, 0, 0, 0);
}

int card_harabaz_druid(int player, int card, event_t event){
	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY);
	}
	return mana_producing_creature_all_one_color(player, card, event, 24, COLOR_TEST_ANY_COLORED, count);
}

int card_jace_the_mind_sculptor(int player, int card, event_t event){

	/* Jace, the Mind Sculptor	|2|U|U
	 * Planeswalker - Jace (3)
	 * +2: Look at the top card of target player's library. You may put that card on the bottom of that player's library.
	 * 0: Draw three cards, then put two cards from your hand on top of your library in any order.
	 * -1: Return target creature to its owner's hand.
	 * -12: Exile all cards from target player's library, then that player shuffles his or her hand into his or her library. */

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player,card);

		int priority_bstorm = 0;
		int priority_bounce = 0;

		if (event == EVENT_ACTIVATE){
			priority_bstorm = (hand_count[1-player]-hand_count[player])*5;
			td.allowed_controller = 1-player;
			priority_bounce = life[player] < 10 && can_target(&td) ? 15 : 5;
			td.allowed_controller = ANYBODY;
		}

		enum{
			CHOICE_SCRY1 = 1,
			CHOICE_BRAINSTORM,
			CHOICE_BOUNCE,
			CHOICE_ULTIMATUM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Scry 1", can_target(&td1), 10, 2,
						"Brainstorm", 1, priority_bstorm, 0,
						"Bounce creature", can_target(&td), priority_bounce, -1,
						"Ultimatum", can_target(&td1), 30, -12);

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
				case CHOICE_SCRY1:
				case CHOICE_ULTIMATUM:
					pick_target(&td1, "TARGET_PLAYER");
					break;

				case CHOICE_BOUNCE:
					pick_target(&td, "TARGET_CREATURE");
					break;

				default:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_SCRY1:
				if( valid_target(&td1) ){
					show_deck( player, deck_ptr[instance->targets[0].player], 1, "Jace Reveals...", 0, 0x7375B0 );
					int ai_choice = 1;
					if( is_what(-1, deck_ptr[instance->targets[0].player][0], TYPE_LAND) && count_subtype(1-player, TYPE_LAND, -1) > 4 ){
						ai_choice = 0;
					}
					if( do_dialog(player, player, card, -1, -1, " Top\n Bottom", ai_choice) == 1 ){
						put_top_card_of_deck_to_bottom(instance->targets[0].player);
					}
				}
				break;

			case CHOICE_BRAINSTORM:
				{
					draw_cards(player, 3);
					int max = MIN(2, hand_count[player]);
					char msg[100] = "Select a card to put on top of deck.";
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, msg);
					while( max > 0 ){
							int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
							put_on_top_of_deck(player, selected);
							max--;
					}
				}
				break;

			case CHOICE_BOUNCE:
				if( valid_target(&td) ) {
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
				break;

			case CHOICE_ULTIMATUM:
				{
					if( valid_target(&td1) ){
						int p = instance->targets[0].player;
						rfg_whole_library(p);
						int i= active_cards_count[p]-1;
						while( i > -1  ){
								if( in_hand(p, i) ){
									put_on_top_of_deck(p, i);
								}
								i--;
						}
						shuffle(p);
					}
				}
				break;
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_join_the_ranks(int player, int card, event_t event){
	/* Join the Ranks	|3|W
	 * Instant
	 * Put two 1/1 |Swhite Soldier Ally creature tokens onto the battlefield. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			generate_tokens_by_id(player, card, CARD_ID_SOLDIER_ALLY, 2);
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_joraga_warcaller(int player, int card, event_t event)
{
  /* Joraga Warcaller	|G
   * Creature - Elf Warrior 1/1
   * Multikicker |1|G
   * ~ enters the battlefield with a +1/+1 counter on it for each time it was kicked.
   * Other Elf creatures you control get +1/+1 for each +1/+1 counter on ~. */

  multikicker(player, card, event, 1, 0, 0, 1, 0, 0);

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, multikicked(player, card));

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && in_play(player, card)
	  && affected_card_controller == player && affected_card != card
	  && has_creature_type(player, affected_card, SUBTYPE_ELF))
	event_result += count_1_1_counters(player, card);

  return 0;
}

int card_jwari_shapeshifter(int player, int card, event_t event)
{
  /* Jwari Shapeshifter	|1|U
   * Creature - Shapeshifter Ally 0/0
   * You may have ~ enter the battlefield as a copy of any Ally creature on the battlefield. */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  base_target_definition(player, card, &td, TYPE_CREATURE);
	  td.required_subtype = SUBTYPE_ALLY;
	}

  enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_AN_ALLY_CREATURE");

  return 0;
}

int card_kalastria_highborn(int player, int card, event_t event){
	/*
	  Kalastria Highborn |B|B
	  Creature - Vampire Shaman 2/2
	  Whenever Kalastria Highborn or another Vampire you control dies, you may pay {B}. If you do, target player loses 2 life and you gain 2 life.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(player, card) ){
			if( in_play(player, card) && instance->kill_code > 0 && instance->kill_code < KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				instance->targets[11].player++;
			}
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_VAMPIRE;
			count_for_gfp_ability(player, card, event, player, 0, &this_test);
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card ) && instance->kill_code < KILL_DESTROY
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		int rt = can_target(&td) && has_mana(player, COLOR_BLACK, 1) ? RESOLVE_TRIGGER_AI(player) : 0;

		if(event == EVENT_TRIGGER){
			event_result |= rt;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				while( instance->targets[11].player > 0 && has_mana(player, COLOR_BLACK, 1) ){
						instance->number_of_targets = 0;
						charge_mana(player, COLOR_BLACK, 1);
						if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
							lose_life(instance->targets[0].player, 2);
							gain_life(player, 2);
						}
						instance->targets[11].player--;
				}
				instance->targets[11].player = 0;
		}
		else if (event == EVENT_END_TRIGGER){
				instance->targets[11].player = 0;
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		while( instance->targets[11].player > 0 && has_mana(player, COLOR_BLACK, 1) && can_target(&td) ){
				instance->number_of_targets = 0;
				charge_mana(player, COLOR_BLACK, 1);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
					lose_life(instance->targets[0].player, 2);
					gain_life(player, 2);
				}
				instance->targets[11].player--;
		}
		instance->targets[11].player = 0;
	}

	return 0;
}

int card_kazuul_tyrant_of_the_cliffs(int player, int card, event_t event)
{
	/* Kazuul, Tyrant of the Cliffs	|3|R|R
	 * Legendary Creature - Ogre Warrior 5/4
	 * Whenever a creature an opponent controls attacks, if you're the defending player, put a 3/3 |Sred Ogre creature token onto the battlefield unless that creature's controller pays |3. */

	check_legend_rule(player, card, event);

	if (declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, 1-player, -1) &&
		(!has_mana(1-player, COLOR_ANY, 3) || !charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, 1-player, COLOR_COLORLESS, 3))
	  ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OGRE, &token);
		token.pow = token.tou = 3;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);
	}

	return 0;
}

int card_khalni_garden(int player, int card, event_t event){
	/* Khalni Garden	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, put a 0/1 |Sgreen Plant creature token onto the battlefield.
	 * |T: Add |G to your mana pool. */

	comes_into_play_tapped(player, card, event);
	if (comes_into_play(player, card, event)){
		generate_token_by_id(player, card, CARD_ID_PLANT);
	}
	return mana_producer(player, card, event);
}

int card_kitesail(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 1, 0, KEYWORD_FLYING, 0);
}

int card_kitesail_apprentice(int player, int card, event_t event){
	if( is_humiliated(player, card) ){
		return 0;
	}
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		if( equipments_attached_to_me(player, card, EATM_CHECK) ){
			event_result++;
		}
	}
	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		if( equipments_attached_to_me(player, card, EATM_CHECK) ){
			event_result |= KEYWORD_FLYING;
		}
	}
	return 0;
}

int card_kor_firewalker(int player, int card, event_t event){
	/* Kor Firewalker	|W|W
	 * Creature - Kor Soldier 2/2
	 * Protection from |Sred
	 * Whenever a player casts a |Sred spell, you may gain 1 life. */

	if( trigger_condition == TRIGGER_SPELL_CAST &&
		specific_spell_played(player, card, event, 2, RESOLVE_TRIGGER_DUH, 0,0, 0,0, get_sleighted_color_test(player, card, COLOR_TEST_RED),0, 0,0, -1,0)
	  ){
		gain_life(player, 1);
	}
	return 0;
}

int card_lavaclaw_reaches(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XBR(1, 1, 1));
}

int card_lavaclaw_reaches_animated(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) && instance->targets[1].player != 66 ){
			return 1;
		}
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(2)) && can_use_activated_abilities(player, card)){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->targets[2].player = 0;
		if( ! is_tapped(player, card) && can_produce_mana(player, card) && instance->targets[1].player != 66 ){
			if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(2)) && can_use_activated_abilities(player, card)){
				choice = do_dialog(player, player, card, -1, -1, " Get mana\n +X/+0\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				instance->targets[1].player = 66;
				charge_mana_for_activated_ability(player, card, MANACOST_X(-1));
				instance->targets[1].player = 0;
				if( spell_fizzled != 1 ){
					instance->targets[2].card = x_value;
					instance->targets[2].player = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION && instance->targets[2].player == 1 ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, instance->targets[2].card, 0);
	}

	if( eot_trigger(player, card, event ) ){
		remove_status(player, card, STATUS_ANIMATED);
		remove_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
		true_transform(player, card);
	}

	if( event == EVENT_SET_COLOR && affect_me(player, card) ){
		event_result = cards_data[ instance->internal_card_id ].color ;
	}

	return 0;
}

int card_leatherback_baloth(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL && ! is_unlocked(player, card, event, 37) ){
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_lightkeeper_of_emeria(int player, int card, event_t event)
{
  // Multikicker |W
  multikicker(player, card, event, MANACOST_W(1));

  // When ~ enters the battlefield, you gain 2 life for each time it was kicked.
  if (comes_into_play(player, card, event))
	{
	  int amt;
	  for (amt = multikicked(player, card); amt > 0; --amt)
		gain_life(player, 2);
	}

  return 0;
}

int card_lodestone_golem(int player, int card, event_t event){
	if(event == EVENT_MODIFY_COST_GLOBAL ){
		card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
		if(! (card_d->type & TYPE_LAND) && ! (card_d->type & TYPE_ARTIFACT)){
			COST_COLORLESS++;
		}
	}
	return 0;
}

int card_marshals_anthem(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		if( ! graveyard_has_shroud(2) ){
			int amount = count_graveyard_by_type(player, TYPE_CREATURE);
			while( has_mana_multi(player, 1, 0, 0, 0, 0, 1) && instance->info_slot < amount ){
					int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
					if( choice == 0 ){
						charge_mana_multi(player, 1, 0, 0, 0, 0, 1);
						if( spell_fizzled != 1 ){
							instance->info_slot++;
						}
						else{
							instance->info_slot = 0;
							break;
						}
					}
					else{
						break;
					}
			}
		}
	}
	if( comes_into_play(player, card, event) ){
		char buffer[100] = " Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
		int i;
		for(i=0; i<instance->info_slot; i++){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}
	boost_creature_type(player, card, event, -1, 1, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return global_enchantment(player, card, event);
}

int card_mordant_dragon(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	card_instance_t *instance = get_card_instance(player, card);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_REPORT_DAMAGE_DEALT) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->targets[16].player, player, card);
		}
		instance->targets[16].player = 0;
	}
	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_natures_claim(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_ARTIFACT);
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			if( is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(instance->targets[0].player, 4);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_nemesis_trap(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_MODIFY_COST ){
		td.required_color = COLOR_TEST_WHITE;	// do not sleight
		td.illegal_abilities = 0;	// doesn't actually have to be targetable
		if (can_target(&td)){
			COST_COLORLESS-=4;
		}
	}
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			token.legacy = 1;
			token.special_code_for_legacy = &remove_at_eot;
			generate_token(&token);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_novablast_wurm( int player, int card, event_t event)
{
  // Whenever ~ attacks, destroy all other creatures.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
	}
  return 0;
}

int card_omnath_locus_of_mana(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);
  // Green mana doesn't empty from your mana pool as steps and phases end.
  if (event == EVENT_MANA_POOL_DRAINING)
	mana_doesnt_drain_from_pool[player][get_sleighted_color(player, card, COLOR_GREEN)] |= MANADRAIN_DOESNT_DRAIN;
  // ~ gets +1/+1 for each green mana in your mana pool.
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card))
	event_result += mana_in_pool[player][get_sleighted_color(player, card, COLOR_GREEN)] + get_card_instance(player, card)->info_slot;
  // AI
  if (event == EVENT_CAN_ACTIVATE && (player == AI || ai_is_speculating == 1) && !is_humiliated(player, card))
	{
	  int clr = get_sleighted_color(player, card, COLOR_GREEN);
	  int more_avail = has_mana(player, clr, 1) - mana_in_pool[player][clr];
	  return more_avail > 0;
	}
  if (event == EVENT_ACTIVATE)
	{
	  int clr = get_sleighted_color(player, card, COLOR_GREEN);
	  card_instance_t* instance = get_card_instance(player, card);
	  int any = 0;
	  while (1)
		{
		  int prev_in_pool = mana_in_pool[player][clr];
		  instance->info_slot = prev_in_pool;	// Prevent p/t from changing while activating
		  mana_in_pool[player][clr] = 0;
		  mana_in_pool[player][COLOR_ANY] -= prev_in_pool;
		  charge_mana(player, clr, 1);
		  instance->info_slot = 0;
		  mana_in_pool[player][clr] += prev_in_pool;
		  mana_in_pool[player][COLOR_ANY] += prev_in_pool;
		  if (cancel == 1)
			break;
		  else
			{
			  any = 1;
			  mana_in_pool[player][clr] += 1;
			  mana_in_pool[player][COLOR_ANY] += 1;
			}
		}
	  if (any)
		cancel = 0;
	  cant_be_responded_to = 1;
	}
  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  int clr = get_sleighted_color(player, card, COLOR_GREEN);
	  ai_modifier += 24 * mana_in_pool[player][clr];
	}
  return 0;
}

int card_perimeter_captain(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		int c;
		if (event == EVENT_DECLARE_BLOCKERS && current_turn != player ){
			for (c = 0; c < active_cards_count[player]; ++c ){
				if (in_play(player, c) && get_card_instance(player, c)->blocking != 255 && is_what(player, c, TYPE_CREATURE) &&
					check_for_ability(player, c, KEYWORD_DEFENDER)
				 ){
					if( player == AI || do_dialog(player, player, c, -1, -1, " Gain 2 life\n Pass", 0) == 0 ){
						gain_life(player, 2);
					}
				}
			}
		}
	}
	return 0;
}

int card_permafrost_trap(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_MODIFY_COST && get_trap_condition(1-player, TRAP_PERMAFROST_TRAP) ){
		COST_COLORLESS-=2;
		COST_BLUE--;
	}
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->info_slot = 1;
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				if( select_target(player, card, &td, "Select target creature.", &(instance->targets[1])) ){
					instance->info_slot++;
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					effect_frost_titan(player, card, instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_pilgrims_eye(int player, int card, event_t event){
	/* Pilgrim's Eye	|3
	 * Artifact Creature - Thopter 1/1
	 * Flying
	 * When ~ enters the battlefield, you may search your library for a basic land card, reveal it, put it into your hand, then shuffle your library. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		tutor_basic_lands(player, TUTOR_HAND, 1);
	}
	return 0;
}

int card_pulse_tracker(int player, int card, event_t event)
{
  /* Pulse Tracker	|B
   * Creature - Vampire Rogue 1/1
   * Whenever ~ attacks, each opponent loses 1 life. */

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	lose_life(1-current_turn, 1);

  if (event == EVENT_ATTACK_RATING && affect_me(player, card))
	ai_defensive_modifier -= 12;

  return 0;
}

int card_quest_for_renewal(int player, int card, event_t event)
{
  /* Quest for Renewal	|1|G
   * Enchantment
   * Whenever a creature you control becomes tapped, you may put a quest counter on ~.
   * As long as there are four or more quest counters on ~, untap all creatures you control during each other player's untap step. */

  if (event == EVENT_TAP_CARD && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE))
	add_counter(player, card, COUNTER_QUEST);

  if (count_counters(player, card, COUNTER_QUEST) >= 4)
	untap_permanents_during_opponents_untap(player, card, TYPE_CREATURE, &get_card_instance(player, card)->info_slot);

  return global_enchantment(player, card, event);
}

int card_quest_for_the_goblin_lord(int player, int card, event_t event){

	/* Quest for the Goblin Lord	|R
	 * Enchantment
	 * Whenever a Goblin enters the battlefield under your control, you may put a quest counter on ~.
	 * As long as ~ has five or more quest counters on it, creatures you control get +2/+0. */

	if( specific_cip(player, card, event, 2, RESOLVE_TRIGGER_AI(player), TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_QUEST);
	}
	if( event == EVENT_POWER && affected_card_controller == player && count_counters(player, card, COUNTER_QUEST) > 4 ){
		event_result+=2;
	}
	return global_enchantment(player, card, event);
}

int card_quest_for_the_nihil_stone(int player, int card, event_t event)
{
  /* Quest for the Nihil Stone	|B
   * Enchantment
   * Whenever an opponent discards a card, you may put a quest counter on ~.
   * At the beginning of each opponent's upkeep, if that player has no cards in hand and ~ has two or more quest counters on it, you may have that player lose 5
   * life. */

  if (discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_DUH, 0))
	add_counter(player, card, COUNTER_QUEST);

  if (trigger_condition == TRIGGER_UPKEEP && hand_count[1-player] == 0 && count_counters(player, card, COUNTER_QUEST) >= 2)
	upkeep_trigger_ability_mode(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player));

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	lose_life(1-player, 5);

  return global_enchantment(player, card, event);
}

int card_quest_for_ulas_temple(int player, int card, event_t event)
{
  /* Quest for Ula's Temple	|U
   * Enchantment
   * At the beginning of your upkeep, you may look at the top card of your library. If it's a creature card, you may reveal it and put a quest counter on ~.
   * At the beginning of each end step, if there are three or more quest counters on ~, you may put a Kraken, Leviathan, Octopus, or Serpent creature card from
   * your hand onto the battlefield. */

  upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int* deck = deck_ptr[player];

	  if (deck[0] != -1)
		{
		  if (is_what(-1, deck[0], TYPE_CREATURE))
			{
			  if (!(IS_AI(player) && count_counters(player, card, COUNTER_QUEST) >= 3)
				  && reveal_card_optional_iid(player, card, deck[0], NULL))
				add_counter(player, card, COUNTER_QUEST);
			}
		  else if (!IS_AI(player))
			look_at_iid(player, card, deck[0], "Top card of deck:");
		}
	}

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player
	  && count_counters(player, card, COUNTER_QUEST) >= 3)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "Select a Kraken, Leviathan, Octopus, or Serpent creature card.");
		  test.subtype = SUBTYPE_KRAKEN;
		  test.sub2 = SUBTYPE_LEVIATHAN;
		  test.sub3 = SUBTYPE_OCTOPUS;
		  test.sub4 = SUBTYPE_SERPENT;
		  test.subtype_flag = F2_MULTISUBTYPE;
		  new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
		}
	}

  return global_enchantment(player, card, event);
}

int card_raging_ravine(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, 2, 0, 0, 1, 1, 0);
}

int card_raging_ravine_animated(int player, int card, event_t event)
{
  // Whenever this creature attacks, put a +1/+1 counter on it.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	add_1_1_counter(player, card);
  return manland_animated(player, card, event, 2, 0, 0, 1, 1, 0);
}

int card_rest_for_the_weary(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 4;
			if( get_trap_condition(player, TRAP_LANDS_PLAYED) > 0 ){
				amount+=4;
			}
			gain_life(instance->targets[0].player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ruin_ghost(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_LAND");
}

int card_ruthless_cullblade(int player, int card, event_t event){
	if( life[1-player] < 11 ){
		modify_pt_and_abilities(player, card, event, 2, 1, 0);
	}
	return 0;
}

int card_searing_blaze(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				td1.allowed_controller = instance->targets[0].player;
				td1.preferred_controller = instance->targets[0].player;
				if( can_target(&td1) ){
					if( ! select_target(player, card, &td1, "Select target creature.", &(instance->targets[1])) ){
						spell_fizzled = 1;
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int amount = 1;
			if( get_trap_condition(player, TRAP_LANDS_PLAYED) > 0 ){
				amount = 3;
			}
			if( validate_target(player, card, &td, 0) ){
				damage_player(instance->targets[0].player, amount, player, card);
			}
			if( validate_target(player, card, &td1, 1) ){
				damage_creature(instance->targets[1].player, instance->targets[1].card, amount, player, card);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_seer_sundial(int player, int card, event_t event){
	if( has_mana(player, COLOR_COLORLESS, 2) && specific_cip(player, card, event, player, 1+player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		charge_mana(player, COLOR_COLORLESS, 2);
		if( spell_fizzled != 1 ){
			draw_cards(player, 1);
		}
	}
	return 0;
}

int card_sejiri_merfolk(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES
	  && affect_me(player, card)
	  && basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_WHITE)] >= 1)
	{
	  event_result |= KEYWORD_FIRST_STRIKE;
	  lifelink(player, card, event);
	}
  return 0;
}

int card_sejiri_steppe(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		card_instance_t* instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			int kw = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, kw, 0);
		}
	}
	return mana_producer(player, card, event);
}

int card_slavering_nulls(int player, int card, event_t event)
{
  /* Slavering Nulls	|1|R
   * Creature - Goblin Zombie 2/1
   * Whenever ~ deals combat damage to a player, if you control |Ha Swamp, you may have that player discard a card. */

  if (IS_DDBM_EVENT(event)
	  && basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_BLACK)] > 0)
	whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_TRIGGER_OPTIONAL, 0);

  return 0;
}

int card_smoldering_spires(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		card_instance_t* instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}
	return mana_producer(player, card, event);
}

int card_stirring_wildwood(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, 1, 0, 0, 1, 0, 1);
}

int card_stirring_wildwood_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, 1, 0, 0, 1, 0, 1);
}

int card_stoneforge_mystic( int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char msg[100] = "Select an Equipment card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
		this_test.subtype = SUBTYPE_EQUIPMENT;
		this_test.zone = TARGET_ZONE_HAND;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
	}

	if(event == EVENT_ACTIVATE && player == AI ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_EQUIPMENT;
		this_test.zone = TARGET_ZONE_HAND;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier-=50;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select an Equipment card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
		this_test.subtype = SUBTYPE_EQUIPMENT;
		this_test.zone = TARGET_ZONE_HAND;

		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XW(1, 1), 0, NULL, NULL);
}

int card_strength_of_the_tajuru(int player, int card, event_t event){
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
			instance->info_slot = x_value;
			int trgs = 1;
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			while( has_mana(player, COLOR_COLORLESS, 1) && can_target(&td1) && trgs < 18 ){
					int choice = 0;
					if( player != AI ){
						choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
					}
					if( choice == 0 ){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 && select_target(player, card, &td1, "Select target creature.", &(instance->targets[trgs])) ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
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
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			instance->targets[trgs].player = (1<<8);
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int trgs = 1;
		int i;
		for(i=0; i<18; i++){
			if( instance->targets[i].player == (1<<8) ){
				trgs = i;
				break;
			}
		}
		for(i=0; i<trgs; i++){
			if( validate_target(player, card, &td1, i) ){
				add_1_1_counters(instance->targets[i].player, instance->targets[i].card, instance->info_slot);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_summit_apes(int player, int card, event_t event){
	if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) ){
		minimum_blockers(player, card, event, 2);
	}
	return 0;
}

int card_talus_paladin(int player, int card, event_t event){
	if( ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){	// but choose
		int choice = duh_mode(player) ? 3 : DIALOG(player, card, EVENT_ACTIVATE,
												   DLG_NO_STORAGE,
												   "Your Allies gain lifelink", 1, 1,
												   "+1/+1 counter", 1, 2,
												   "Both", 1, 3);
		if (choice & 1){
			pump_subtype_until_eot(player, card, player, SUBTYPE_ALLY, 0, 0, 0, SP_KEYWORD_LIFELINK);
		}
		if (choice & 2){
			add_1_1_counter(player, card);
		}
	}
	return 0;
}

int card_tectonic_edge(int player, int card, event_t event){
	/*
	  Tectonic Edge
	  Land
	  {T}: Add {1} to your mana pool.
	  {1}, {T}, Sacrifice Tectonic Edge: Destroy target nonbasic land. Activate this ability only if an opponent controls four or more lands.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	if (event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		int choice = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_X(2), 0, &td, NULL)
			&& count_subtype(1-player, TYPE_LAND, -1) > 3
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Destroy a nonbasic land\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) &&
					new_pick_target(&td, "Select_target nonbasic land.", 0, 1 | GS_LITERAL_PROMPT)
				  ){
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

static int effect_terastodon(int player, int card, event_t event)
{
  if (effect_put_into_a_graveyard_this_way(player, card, event))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ELEPHANT, &token);
	  token.t_player = get_card_instance(player, card)->targets[0].player;
	  generate_token(&token);
	}
  return 0;
}
int card_terastodon(int player, int card, event_t event)
{
  /* Terastodon	|6|G|G
   * Creature - Elephant 9/9
   * When ~ enters the battlefield, you may destroy up to three target noncreature permanents. For each permanent put into a graveyard this way, its controller
   * puts a 3/3 |Sgreen Elephant creature token onto the battlefield. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_CREATURE;
	  td.allow_cancel = 3;	// done and cancel buttons

	  card_instance_t* instance = get_card_instance(player, card);

	  int i, count = pick_up_to_n_targets(&td, "TARGET_NONCREATURE_PERMANENT", 3);

	  for (i = 0; i < count; ++i)
		{
		  create_targetted_legacy_effect(player, card, effect_terastodon, instance->targets[i].player, instance->targets[i].card);
		  kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_terra_eternal(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( instance->info_slot == 1 ){
		return 0;
	}
	instance->info_slot = 1;
	if( (land_can_be_played & LCBP_REGENERATION) ){
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int c = 0;
				for(c=0;c< active_cards_count[i];c++){
					if( is_what(i, c, TYPE_LAND) && in_play(i, c)){
						card_instance_t *creature = get_card_instance(i, c);
						if( creature->kill_code == KILL_DESTROY || creature->kill_code == KILL_BURY ){
							int is_tap = is_tapped(i, c);
							regenerate_target_exe(i, c);
							if( ! is_tap ){
								untap_card_no_event(i, c);
							}
						}
					}
				}
			}
		}
	}
	instance->info_slot = 0;
	if( event == EVENT_GRAVEYARD_FROM_PLAY && reason_for_trigger_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			card_instance_t *this = get_card_instance(affected_card_controller, affected_card);
			if( this->kill_code == KILL_BURY ){
				this->kill_code = 0;
				this->unknown0x14 = 0;
				event_result++;
			}
		}
	}
	if( event == EVENT_CHECK_PUMP && is_what(affected_card_controller, affected_card, TYPE_LAND) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_tapped(affected_card_controller, affected_card)
	  ){
		pumpable_toughness[player] += 99;
	}
	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_LAND) && in_play(affected_card_controller, affected_card) ){
		card_instance_t *this = get_card_instance(affected_card_controller, affected_card);
		if( this->targets[16].card < 0 ){
			this->targets[16].card = 0;
		}
		if( ! (this->targets[16].card & SP_KEYWORD_INDESTRUCTIBLE) ){
			this->targets[16].card |= SP_KEYWORD_INDESTRUCTIBLE;
		}
	}
	return global_enchantment(player, card, event);
}

int card_thada_adel_acquisitor(int player, int card, event_t event)
{
  /* Thada Adel, Acquisitor	|1|U|U
   * Legendary Creature - Merfolk Rogue 2/2
   * |H2Islandwalk
   * Whenever ~ deals combat damage to a player, search that player's library for an artifact card and exile it. Then that player shuffles his or her
   * library. Until end of turn, you may play that card. */

  check_legend_rule(player, card, event);

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE))
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

	  int csvid = new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
	  if (csvid != -1)
		create_may_play_card_from_exile_effect(player, card, 1-player, csvid, MPCFE_UNTIL_EOT);
	}

  return 0;
}

int card_tideforce_elemental(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	card_instance_t *instance = get_card_instance(player, card);
	if( is_tapped(player, card) && specific_cip(player, card, event, player, 1+player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int choice = 0;
			if( is_tapped(instance->targets[0].player, instance->targets[0].card) ){
				choice = 1;
			}
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
			}
			if( choice == 0 ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_NOT_ME_AS_TARGET, 0, 0, 1, 0, 0, 0, 0,
									&td, "TARGET_CREATURE");
}

int card_treasure_hunt(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		int i=0;
		while(i<500 && deck[i] != -1 ){
			card_data_t* card_d = &cards_data[ deck[i] ];
			if( ! ( card_d->type & TYPE_LAND ) ){
				if( player == AI ){
					show_deck( HUMAN, deck_ptr[player], i+1, "These cards were revealed by Treasure Hunt", 0, 0x7375B0 );
				}
				i++;
				break;
			}
			i++;
		}
		int j;
		for(j=0;j<i;j++){
			add_card_to_hand(player,  deck[0] );
			remove_card_from_deck( player, 0 );
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_tuktuk_scrapper(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	card_instance_t *instance = get_card_instance(player, card);
	if( can_target(&td) && ally_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){	// but can cancel
		if( pick_target(&td, "TARGET_ARTIFACT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_player(instance->targets[0].player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ALLY), player, card);
		}
	}
	return 0;
}

int card_urge_to_feed(int player, int card, event_t event){
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL  ){
		if(  valid_target(&td1) ){
			card_instance_t *instance= get_card_instance(player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.required_subtype = SUBTYPE_VAMPIRE;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			td.illegal_state = TARGET_STATE_TAPPED;
			while( can_target(&td) && spell_fizzled != 1 ){
				if( select_target(player, card, &td, "Choose an untapped Vampire to tap", NULL) ){
					tap_card( instance->targets[0].player, instance->targets[0].card );
					add_1_1_counter( instance->targets[0].player, instance->targets[0].card  );
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// walking atlas --> Sakura-Tribe Scout

int card_wolfbriar_elemental(int player, int card, event_t event){
	/* Wolfbriar Elemental	|2|G|G
	 * Creature - Elemental 4/4
	 * Multikicker |G
	 * When ~ enters the battlefield, put a 2/2 |Sgreen Wolf creature token onto the battlefield for each time it was kicked. */

	multikicker(player, card, event, 0, 0, 0, 1, 0, 0);
	if( multikicked(player, card) && comes_into_play(player, card, event) ){
		int i=0;
		for(i=0;i<multikicked(player, card);i++){
			generate_token_by_id(player, card, CARD_ID_WOLF);
		}
	}
	return 0;
}

int card_wrexial_the_risen_deep(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
		char msg[100] = "Select an Instant or Sorcery card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, msg);
		this_test.can_legally_play = 1;
		int selected = new_select_a_card(player, 1-player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			play_card_in_grave_for_free_and_exile_it(player, 1-player, selected);
		}
	}
	return 0;
}
