#include "manalink.h"

// global functions
static int haunting_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				instance->targets[11].player = 66;
				instance->targets[11].card = get_protections_from(p, c);
				// Detach from haunted creature, so that the effect card's not sacrificed before it has a chance to trigger
				instance->damage_target_player = -1;
				instance->damage_target_card = -1;
			}
		}
	}

	if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){

		int card_added = add_card_to_hand(player, instance->targets[2].card);
		call_card_function(player, card_added, EVENT_HAUNT);
		obliterate_card(player, card_added);

		kill_card(player, card, KILL_REMOVE);
		instance->targets[11].player = 0;
	}

	if( ! in_play(p, c) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static int haunt_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				instance->targets[11].player = 66;
				instance->targets[11].card = get_protections_from(p, c);
				remove_status(player, card, STATUS_INVISIBLE_FX);
				instance->kill_code = KILL_REMOVE;
			}
		}
	}

	if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.illegal_abilities = instance->targets[11].card;
		td.illegal_state = TARGET_STATE_DYING;
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			int fake = add_card_to_hand(player, instance->targets[2].card);
			int legacy = create_targetted_legacy_effect(player, fake, &haunting_legacy, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[2].card = instance->targets[2].card;
			obliterate_card(player, fake);
		}
		kill_card(player, card, KILL_REMOVE);
		instance->targets[11].player = 0;
	}

	if( ! in_play(p, c) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static int haunt(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &haunt_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		add_status(player, legacy, STATUS_INVISIBLE_FX);
		leg->targets[0].card = card;
		leg->targets[0].player = player;
		leg->number_of_targets = 1;
		leg->targets[2].card = instance->internal_card_id;
	}

	if (comes_into_play(player, card, event)){
		call_card_function(player, card, EVENT_HAUNT);
	}
	return 0;
}

static int magemark(int player, int card, event_t event, int key){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			if( is_enchanted(affected_card_controller, affected_card) ){
				event_result++;
			}
		}
		if( event == EVENT_ABILITIES && key && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			if( is_enchanted(affected_card_controller, affected_card) ){
				event_result |= key;
			}
		}
	}

	return vanilla_aura(player, card, event, player);
}

static int replicate(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, target_definition_t *td, const char *prompt, int one_target_per_spell){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = 1;
		if( td != NULL ){
			load_text(0, prompt);
			result = select_target(td->player, td->card, td, text_lines[0], NULL);
			if( ! result ){
				spell_fizzled = 1;
			}
		}
		if( result == 1){
			instance->info_slot = 1;
			if( one_target_per_spell ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			}
			while( has_mana_multi(player, cless, black, blue, green, red, white) && can_target(td) ){
					int choice = do_dialog(player, player, card, -1, -1, " Replicate\n Stop", 0);
					if( choice == 0 ){
						charge_mana_multi(player, cless, black, blue, green, red, white);
						if( spell_fizzled != 1 ){
							if( td == NULL ){
								instance->info_slot++;
							}
							else{
								if( select_target(td->player, td->card, td, text_lines[0], &(instance->targets[instance->info_slot]))){
									if( one_target_per_spell ){
										state_untargettable(instance->targets[instance->info_slot].player, instance->targets[instance->info_slot].card, 1);
									}
									instance->info_slot++;
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
					else{
						break;
					}
			}
			if( one_target_per_spell ){
				int i;
				for(i=0; i<instance->info_slot; i++){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				}
			}
		}
	}

	return 0;
}

int bloodthirst(int player, int card, event_t event, int counters){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) > 0 ){
			ai_modifier+=25;
		}
		else{
			ai_modifier-=25;
		}
	}

	if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card) &&
		get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) > 0
	  ){
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, counters);
	}

	return 0;
}

static int rfg_id_from_grave(int player, int id){
	int i;
	int result = 0;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			const int *grave = get_grave(i);
			int count = count_graveyard(i)-1;
			while( count > -1 ){
					if( cards_data[grave[count]].id == id ){
						rfg_card_from_grave(i, count);
						result++;
					}
					count--;
			}
		}
	}
	return result;
}

// cards
int card_absolver_thrull(int player, int card, event_t event){

	haunt(player, card, event);

	if (event == EVENT_HAUNT){
		card_instance_t* instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_abyssal_nocturnus(int player, int card, event_t event){
	if( discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, 0) ){
		pump_ability_until_eot(player, card, player, card, 2, 2, 0, SP_KEYWORD_FEAR);
	}
	return 0;
}

int card_aetherplasm(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE ){
		if( current_turn != player && current_phase == PHASE_AFTER_BLOCKING && instance->blocking < 255 ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		instance->targets[0].card = instance->blocking;
		bounce_permanent(player, card);
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		if( result > -1 ){
			block(player, result, 1-player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_angel_of_despair(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;


		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			action_on_target(player, card, 0, KILL_DESTROY);
			get_card_instance(player, card)->number_of_targets = 0;
		}
	}

	return 0;
}

int card_beastmasters_magemark(int player, int card, event_t event){

	card_instance_t *instance = in_play(player, card);

	if( instance && instance->damage_target_player > -1 && ! is_humiliated(player, card)){
		if( event == EVENT_DECLARE_BLOCKERS && current_turn == player ){
			int count;
			for(count = active_cards_count[player]-1; count > -1; count--){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && is_attacking(player, count) && is_enchanted(player, count) ){
					int amount = count_my_blockers(player, count);
					if( amount > 0 ){
						pump_until_eot(player, card, player, count, amount, amount);
					}
				}
			}
		}
	}

	return magemark(player, card, event, 0);
}

int card_belfry_spirit(int player, int card, event_t event){
	/* Belfry Spirit	|3|W|W
	 * Creature - Spirit 1/1
	 * Flying
	 * Haunt
	 * When ~ enters the battlefield or the creature it haunts dies, put two 1/1 |Sblack Bat creature tokens with flying onto the battlefield. */

	haunt(player, card, event);

	if( event == EVENT_HAUNT ){
		generate_tokens_by_id(player, card, CARD_ID_BAT, 2);
	}

	return 0;
}

int card_blind_hunter(int player, int card, event_t event){

	haunt(player, card, event);

	if( event == EVENT_HAUNT ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		td1.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if(target_available(player, card, &td1) && pick_target(&td1, "TARGET_PLAYER") ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
	}

	return 0;
}

int card_borborygmos(int player, int card, event_t event){

	/* Borborygmos	|3|R|R|G|G
	 * Legendary Creature - Cyclops 6/7
	 * Trample
	 * Whenever ~ deals combat damage to a player, put a +1/+1 counter on each creature you control. */

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE) ){
		manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	return 0;
}

int card_castigate(int player, int card, event_t event){

	/* Castigate	|W|B
	 * Sorcery
	 * Target opponent reveals his or her hand. You choose a nonland card from it and exile that card. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_RFG;
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_cerebral_vortex(int player, int card, event_t event){

	/* Cerebral Vortex	|1|U|R
	 * Instant
	 * Target player draws two cards, then ~ deals damage to that player equal to the number of cards he or she has drawn this turn. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 2);
			damage_player(instance->targets[0].player, cards_drawn_this_turn[instance->targets[0].player], player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_conjurers_ban(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int cd = count_deck(1-player);
		int *deck = deck_ptr[1-player];
		int id = cards_data[deck[internal_rand(cd)]].id;
		if( player == HUMAN ){
			id = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//4
		card_instance_t *this = get_card_instance(player, card_added);
		this->targets[2].player = 4;
		this->targets[2].card = get_id(player, card);
		this->targets[3].player = 1;
		if( current_turn == player ){
			this->targets[3].player++;
		}
		create_card_name_legacy(player, card_added, id);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_crash_landing(int player, int card, event_t event){

	/* Crash Landing	|2|G
	 * Instant
	 * Target creature with flying loses flying until end of turn. ~ deals damage to that creature equal to the number of |H1Forests you control. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			negate_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING);
			damage_creature(instance->targets[0].player, instance->targets[0].card,
							count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST)),
							player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with flying.", 1, NULL);
}

int card_cremate(int player, int card, event_t event){

	if( !IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select target card to exile.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}


int card_cry_of_contrition(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_HAUNT ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;
		td2.allow_cancel = 0;
		if( can_target(&td2) && pick_target(&td2, "TARGET_PLAYER") ){
			discard(instance->targets[0].player, 0, player);
		}
	}

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE );
			td1.allow_cancel = 0;
			if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
				int legacy = create_targetted_legacy_effect(player, card, &haunting_legacy, instance->targets[0].player, instance->targets[0].card);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[2].card = instance->internal_card_id;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_culling_sun(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_debtors_knell(int player, int card, event_t event){

	hybrid(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;

		card_instance_t *instance = get_card_instance( player, card );

		instance->targets[0].player = 1-player;
		if( any_in_graveyard_by_type(1-player, TYPE_CREATURE) && ! graveyard_has_shroud(1-player) ){
			if( any_in_graveyard_by_type(player, TYPE_CREATURE)&& ! graveyard_has_shroud(player) ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
				}
			}
		}
		else{
			instance->targets[0].player = player;
			if (!any_in_graveyard_by_type(player, TYPE_CREATURE)){
				return 0;
			}
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_GRAVE, 1, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
		}
	}

	return global_enchantment(player, card, event);
}

int card_djinn_illuminatus(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		if( has_mana_to_cast_id(player, event, id) ){
			if( charge_mana_from_id(player, -1, event, id) ){
				copy_spell(player, id);
			}
		}
	}

	return 0;
}

int card_droning_bureaucrats(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(-1)) ){
			tap_card(player, card);
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && get_cmc(i, count) == instance->info_slot ){
						int leg = pump_ability_until_eot(instance->parent_controller, instance->parent_card, i, count, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
						if (leg >= 0){
							get_card_instance(instance->parent_controller, leg)->targets[3].player = PAUE_CANT_ATTACK;
						}
					}
					count--;
			}
		}
	}

	return 0;
}

int card_drowned_rusalka(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		discard(player, 0, player);
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_U(1), 0, NULL, NULL);
}

int card_dryad_sophisticate(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) && !(player_bits[player] & PB_NONSTANDARD_LANDWALK_DISABLED) ){
		if( player == attacking_card_controller && card == attacking_card ){
			int landwalk = 0;
			int count = active_cards_count[1-player]-1;
			while( count > -1 ){
					if( in_play(1-player, count) && is_what(1-player, count, TYPE_LAND) && ! is_basic_land(1-player, count) ){
						landwalk = 1;
						break;
					}
					count--;
			}
			if( landwalk ){
				event_result = 1;
			}
		}
	}
	return 0;
}

// Sand token --> vanilla

int card_dune_brood_nephilim(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SAND, &token);
		token.pow = 1;
		token.tou = 1;
		token.qty = count_permanents_by_type(player, TYPE_LAND);
		generate_token(&token);
	}

	return 0;
}

int card_electrolyze(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 1, 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if (divide_damage(player, card, &td)){
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fencers_magemark(int player, int card, event_t event){
	return magemark(player, card, event, KEYWORD_FIRST_STRIKE);
}

int card_feral_animist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, get_power(player, instance->parent_card), 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
}

int card_gatherer_of_graces(int player, int card, event_t event){

	/* Gatherer of Graces	|1|G
	 * Creature - Human Druid 1/2
	 * ~ gets +1/+1 for each Aura attached to it.
	 * Sacrifice an Aura: Regenerate ~. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += count_auras_enchanting_me(player, card);
	}

	if( land_can_be_played & LCBP_REGENERATION ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select an Aura to sacrifice.");
		test.subtype = SUBTYPE_AURA;

		if( event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE0(player, card) && new_can_sacrifice_as_cost(player, card, &test) ){
			return can_regenerate(player, card);
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
					new_sacrifice(player, card, player, SAC_AS_COST, &test);
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t* instance = get_card_instance(player, card);
			int pp = instance->parent_controller, pc = instance->parent_card;
			if (in_play(pp, pc) && can_regenerate(pp, pc)){
				regenerate_target(pp, pc);
			}
		}
	}

	return 0;
}

int card_gelectrode(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_ghost_council_of_orzhova(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			lose_life(1-player, 1);
			gain_life(player, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		remove_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_X(1), 0, NULL, NULL);
}

int card_ghostway(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_RFG_UNTIL_EOT);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_giant_solifuge(int player, int card, event_t event){
	hybrid(player, card, event);
	haste(player, card, event);
	return 0;
}

int card_gigadrowse(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_U(1), &td, "TARGET_PERMANENT", 1);
}

int card_glint_eye_nephilim(int player, int card, event_t event){

	/* Glint-Eye Nephilim	|U|B|R|G
	 * Creature - Nephilim 2/2
	 * Whenever ~ deals combat damage to a player, draw that many cards.
	 * |1, Discard a card: ~ gets +1/+1 until end of turn. */

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		if (hand_count[player] <= 0){
			return 0;
		}
		return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_X(1), hand_count[player]);
	}

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_REPORT_DAMAGE_DEALT) ){
		draw_cards(player, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int p = instance->damage_target_player, c = instance->damage_target_card;
		if (in_play(p, c)){
			pump_until_eot_merge_previous(p, c, p, c, 1,1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL);
}

int card_graven_dominator(int player, int card, event_t event){

	haunt(player, card, event);

	if( event == EVENT_HAUNT ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						if( ! (i==player && count == card) ){
							set_pt_and_abilities_until_eot(player, card, i, count, 1, 1, 0, 0, 0);
						}
					}
					count--;
			}
		}
	}

	return 0;
}

int card_gristleback(int player, int card, event_t event){

	bloodthirst(player, card, event, 1);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		gain_life(player, get_card_instance(instance->parent_controller, instance->parent_card)->power);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_gruul_guildmage(int player, int card, event_t event){
	/*
	  Gruul Guildmage {R/G}{R/G}
	  Creature - Human Shaman 2/2
	  ({R/G} can be paid with either {R} or {G}.)
	  {3}{R}, Sacrifice a land: Gruul Guildmage deals 2 damage to target player.
	  {3}{G}: Target creature gets +2/+2 until end of turn.
	*/
	hybrid(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.allow_cancel = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;


	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(3, 1), 0, &td1, NULL) ){
			if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(3, 1), 0, &td2, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XR(3, 1), 0, &td1, NULL) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XG(3, 1), 0, &td2, NULL) ){
				int ai_choice = 0;
				if( current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Damage player\n Pump a creature\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if (choice == 2){
			cancel = 1;
			return 0;
		}

		if( charge_mana_for_activated_ability(player, card, MANACOST_XGR(3, (choice == 1), (choice == 0)))){
			instance->number_of_targets = 0;
			if( choice == 0 ){
				if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					pick_target(&td1, "TARGET_PLAYER");
				}
				else{
					spell_fizzled = 1;
				}
			}
			if( choice == 1 ){
				pick_target(&td2, "TARGET_CREATURE");
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td1) ){
			damage_player(instance->targets[0].player, 2, instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td2) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}
	return 0;
}

int card_gruul_war_plow(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_XGR(1, 1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 4, 4, 0, 0);
	}

	return boost_creature_type(player, card, event, -1, 0, 0, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
}


int card_hatching_plans(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, 3);
	}

	return global_enchantment(player, card, event);
}


int card_hissing_miasma(int player, int card, event_t event)
{
  // Whenever a creature attacks you, its controller loses 1 life.
  if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER|DAT_SEPARATE_TRIGGERS, 1-player, -1))
	lose_life(current_turn, 1);

  return global_enchantment(player, card, event);
}

int card_infiltrators_magemark(int player, int card, event_t event)
{
  card_instance_t *instance = get_card_instance(player, card);

  if (event == EVENT_BLOCK_LEGALITY && in_play(player, card) && instance->damage_target_player >= 0
	  && attacking_card_controller == player && is_enchanted(attacking_card_controller, attacking_card)
	  && !check_for_ability(affected_card_controller, affected_card, KEYWORD_DEFENDER))
	event_result = 1;

  return magemark(player, card, event, 0);
}

int card_invoke_the_firemind(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		instance->targets[1].player = 0;
		if( spell_fizzled != 1 ){
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				instance->info_slot = x_value;
				int choice = 0;
				if( can_target(&td) ){
					int ai_choice = 1;
					if( hand_count[player]+instance->info_slot < 7 ){
						ai_choice = 0;
					}
					choice = do_dialog(player, player, card, -1, -1, " You draw\n Damage creature or player\n Cancel", ai_choice);
				}
				if( choice == 2 ){
					spell_fizzled = 1;
					return 0;
				}
				instance->targets[1].player = 1+choice;
			}
			if( instance->targets[1].player == 2 ){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[1].player == 1 ){
			draw_cards(player, instance->info_slot);
		}
		if( instance->targets[1].player == 2 && valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// izzet chronarch --> archeomancer

int card_izzet_guildmage(int player, int card, event_t event){

	hybrid(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		if( player == trigger_cause_controller && get_cmc(trigger_cause_controller,trigger_cause) < 3 && ! is_humiliated(player, card) &&
			! is_what(trigger_cause_controller,trigger_cause, TYPE_CREATURE) && ! is_what(trigger_cause_controller,trigger_cause, TYPE_LAND)
		  ){
			int trig = 0;
			int red = 0;
			int blue = 0;
			if( has_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) && is_what(trigger_cause_controller,trigger_cause, TYPE_SORCERY) ){
				trig = 1;
				red = 1;
			}
			if( has_mana_for_activated_ability(player, card, MANACOST_XU(2, 1)) && is_what(trigger_cause_controller,trigger_cause, TYPE_INSTANT) ){
				trig = 1;
				blue = 1;
			}
			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						charge_mana_for_activated_ability(player, card, 2, 0, blue, 0, red, 0);
						if( spell_fizzled != 1 ){
							copy_spell_from_stack(player, trigger_cause_controller,trigger_cause);
						}
				}
			}
		}
	}
	return 0;
}

int card_killer_instinct(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			show_deck( HUMAN, deck, 1, "Killer Instict revealed this card", 0, 0x7375B0 );
			if( is_what(-1, deck[0], TYPE_CREATURE) ){
				int result = put_into_play_a_card_from_deck(player, player, 0);
				if( result != -1 ){
					create_targetted_legacy_effect(player, card, &haste_and_sacrifice_eot, player, result);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_leap_of_flame(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 0, 0, KEYWORD_FLYING, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_UR(1, 1), &td, "TARGET_CREATURE", 1);
}

int card_leyline_of_lightning(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		if( player == trigger_cause_controller && ! is_what(trigger_cause_controller,trigger_cause, TYPE_LAND) && can_target(&td) &&
			! is_humiliated(player, card)
		  ){
			int trig = 0;
			if( has_mana(player, COLOR_COLORLESS, 1) ){
				trig = 1;
			}
			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
							damage_player(instance->targets[0].player, 1, player, card);
							instance->number_of_targets = 1;
						}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_leyline_of_lifeforce(int player, int card, event_t event){
	type_uncounterable(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	return global_enchantment(player, card, event);
}

int card_leyline_of_singularity(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_RESOLVE_SPELL && ! check_battlefield_for_id(2, CARD_ID_MIRROR_GALLERY) ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && ! is_what(i, count, TYPE_LAND) && ! is_legendary(i, count) ){
							true_verify_legend_rule(i, count, get_id(i, count));
						}
						count--;
				}
			}
		}

		if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller){
			if( ! is_what(trigger_cause_controller,trigger_cause, TYPE_LAND) && ! is_legendary(trigger_cause_controller,trigger_cause) ){
				if( ! check_battlefield_for_id(2, CARD_ID_MIRROR_GALLERY) ){
					if(event == EVENT_TRIGGER){
						event_result |= 2;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
							true_verify_legend_rule(trigger_cause_controller,trigger_cause, get_id(trigger_cause_controller,trigger_cause));
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_leyline_of_the_meek(int player, int card, event_t event){

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_token(affected_card_controller, affected_card) && ! is_humiliated(player, card)){
		event_result++;
	}

	return global_enchantment(player, card, event);
}

int card_leyline_of_the_void(int player, int card, event_t event)
{
  /* Leyline of the Void	|2|B|B
   * Enchantment
   * If ~ is in your opening hand, you may begin the game with it on the battlefield.
   * If a card would be put into an opponent's graveyard from anywhere, exile it instead. */

  if( ! is_unlocked(player, card, event, 34) ){ return 0; }

  if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, 1-player, NULL);

  return global_enchantment(player, card, event);
}

int card_martyred_rusalka(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, effect_cannot_attack_until_eot, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_CAN_TARGET, MANACOST_W(1), 0, &td, "TARGET_CREATURE");
}

int card_mimeofacture(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_PERMANENT);
				this_test.id = get_id(instance->targets[i].player, instance->targets[i].card);
				new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_XU(3, 1), &td, "TARGET_PERMANENT", 1);
}

static int effect_mizzium_transreliquat(int player, int card, event_t event)
{
  /* Combining this and card_mizzium_transreliquat() into a common function is more trouble than it's worth.  In particular, DIALOG() will try to write to
   * info_slot, which is where the effect's function is stored. */
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT);
  if (IS_AI(player))
	td.special = TARGET_SPECIAL_NOT_ME;

  int rval = attachment_granting_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XUR(1,1,1), 0, &td, "TARGET_ARTIFACT");

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  card_instance_t* parent = get_card_instance(instance->parent_controller, instance->parent_card);
	  int p = parent->damage_target_player, c = parent->damage_target_card;
	  int t_player = instance->targets[0].player, t_card = instance->targets[0].card;
	  if (p >= 0 && c >= 0 && validate_arbitrary_target(&td, t_player, t_card))
		cloning_and_verify_legend(p, c, t_player, t_card);
	}

  return rval;
}
int card_mizzium_transreliquat(int player, int card, event_t event)
{
  /* Mizzium Transreliquat	|3
   * Artifact
   * |3: ~ becomes a copy of target artifact until end of turn.
   * |1|U|R: ~ becomes a copy of target artifact and gains this ability. */

  cloning_card(player, card, event);

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT);
  if (IS_AI(player))
	td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_CAN_ACTIVATE && !(can_use_activated_abilities(player, card) && can_target(&td)))
	return 0;

  enum
  {
	CHOICE_EOT = 1,
	CHOICE_PERM
  } choice = DIALOG(player, card, event,
					"Copy until end of turn", 1, 1, DLG_MANA(MANACOST_X(3)),
					"Copy permanently", 1, 2, DLG_MANA(MANACOST_XUR(1,1,1)));

  if (event == EVENT_CAN_ACTIVATE)
	return choice;
  else if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;
	  if (cancel != 1)
		pick_target(&td, "TARGET_ARTIFACT");
	}
  else	// EVENT_RESOLVE_ACTIVATION
	{
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c) && valid_target(&td))
		{
		  get_card_instance(p, c)->number_of_targets = 0;
		  int t_player = instance->targets[0].player, t_card = instance->targets[0].card;
		  switch (choice)
			{
			  case CHOICE_EOT:
				/* Most of this is due to the possibility of the permanent ability being activated in response.  The activateable effect card added by
				 * resolution of the permanent ability needs to be removed when it changes type for the temporary ability, and then restored when the temporary
				 * ability wears off. */
				;int any_effects = 0;
				card_instance_t* inst;
				int cc;
				for (cc = 0; cc < active_cards_count[p]; ++cc)
				  if ((inst = in_play(p, cc)) && inst->internal_card_id == LEGACY_EFFECT_ACTIVATED && inst->info_slot == (int)effect_mizzium_transreliquat
					  && inst->damage_target_player == p && inst->damage_target_card == c)
					{
					  kill_card(p, cc, KILL_REMOVE);
					  ++any_effects;
					}

				int leg = shapeshift_target(p, c, p, c, t_player, t_card, SHAPESHIFT_UNTIL_EOT | SHAPESHIFT_EFFECT_WHEN_REMOVE);

				if (leg != -1 && any_effects)
				  {
					set_legacy_image(p, CARD_ID_MIZZIUM_TRANSRELIQUAT, leg);
					card_instance_t* legacy = get_card_instance(p, leg);
					legacy->targets[2].player = (int)effect_mizzium_transreliquat;
				  }
				break;

			  case CHOICE_PERM:
				set_legacy_image(p, CARD_ID_MIZZIUM_TRANSRELIQUAT, create_targetted_legacy_activate(p, c, effect_mizzium_transreliquat, p, c));
				cloning_and_verify_legend(p, c, t_player, t_card);
				break;
			}
		}
	}

  return 0;
}

static int find_target_for_moratorium_stone(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_LAND);
	this_test.type_flag = DOESNT_MATCH;

	instance->targets[0].player = -1;
	instance->targets[1].player = -1;
	instance->targets[1].card = -1;

	int i, k;
	int max = 0, rval = 0;
	for(i=0; i<2; i++){
		const int *grave = get_grave(i);
		for(k=0; k<count_graveyard(i); k++){
			if( ! is_what(-1, grave[i], TYPE_LAND) ){
				this_test.id = cards_data[grave[i]].id;
				int new_max = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)-check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
				if( new_max > max ){
					max = new_max;
					instance->targets[0].player = i;
					instance->targets[1].player = k;
					instance->targets[1].card = graveyard_source[i][k];
					rval = 1;
				}
			}
		}
	}
	return rval;
}

int card_moratorium_stone(int player, int card, event_t event){
	/*
	  Moratorium Stone |1

	  Artifact, 1

	  {2}, {T}: Exile target card from a graveyard.

	  {2}{W}{B}, {T}, Sacrifice Moratorium Stone: Exile target nonland card from a graveyard, all other cards from graveyards with the same name as that card,
	  and all permanents with that name.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
			if( count_graveyard(player) > 0 && ! graveyard_has_shroud(player) && player != AI ){
				return 1;
			}
			if( count_graveyard(1-player) > 0 && ! graveyard_has_shroud(1-player) ){
				return 1;
			}
		}
		if( player == AI ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_XBW(2, 1, 1), 0, NULL, NULL) ){
				if( find_target_for_moratorium_stone(player, card) ){
					return 1;
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int abilities[3] = {	1,
								generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_XBW(2, 1, 1), 0, NULL, NULL)
		};
		int priorities[3] = {	5,
								find_target_for_moratorium_stone(player, card) ? 10 : 0
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Exile a card from a graveyard", abilities[0], priorities[0],
							"Moratorium effect", abilities[1], priorities[1]);

		if ( !choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XBW(2, (choice == 2), (choice == 2))) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card.");
			this_test.type_flag = DOESNT_MATCH;
			if( choice == 1 || (choice == 2 && player == HUMAN) ){
				if( select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &this_test, 0, 1) != -1 ){
					tap_card(player, card);
					if( choice == 2 ){
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
			if( choice == 2 && player == AI ){
				if( find_target_for_moratorium_stone(player, card) ){
					tap_card(player, card);
					kill_card(player, card, KILL_SACRIFICE);
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
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			if( instance->info_slot == 1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
			if( instance->info_slot == 2 ){
				const int *grave = get_grave(instance->targets[0].player);
				int id = cards_data[grave[selected]].id;
				rfg_id_from_grave(2, id);

				test_definition_t this_test2;
				default_test_definition(&this_test2, TYPE_PERMANENT);
				this_test2.id = id;
				new_manipulate_all(player, card, 2, &this_test2, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int card_mortify(int player, int card, event_t event){
	/*
	  Mortify |1|W|B
	  Instant
	  Destroy target creature or enchantment.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature or enchantment.", 1, NULL);
}

int card_mourning_thrull(int player, int card, event_t event){
	spirit_link_effect(player, card, event, player);
	hybrid(player, card, event);
	return 0;
}

static int necromancers_magemark_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card ) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					seek_grave_for_id_to_reanimate(instance->targets[0].player, -1, instance->targets[0].player, cards_data[instance->targets[0].card].id,
													REANIMATEXTRA_RETURN_TO_HAND2);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

int card_necromancers_magemark(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == player ){
			if( in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				card_instance_t *this = get_card_instance(affected_card_controller, affected_card);
				if( this->kill_code > 0 && this->kill_code != KILL_REMOVE ){
					if( is_enchanted(affected_card_controller, affected_card) ){
						int legacy = create_legacy_effect(player, card, &necromancers_magemark_effect);
						card_instance_t *leg = get_card_instance(player, legacy);
						leg->targets[0].player = get_owner(affected_card_controller, affected_card);
						leg->targets[0].card = get_original_internal_card_id(affected_card_controller, affected_card);
					}
				}
			}
		}
	}

	return magemark(player, card, event, 0);
}

int card_niv_mizzet_the_firemind(int player, int card, event_t event){

	/* Niv-Mizzet, the Firemind	|2|U|U|R|R
	 * Legendary Creature - Dragon Wizard 4/4
	 * Flying
	 * Whenever you draw a card, ~ deals 1 damage to target creature or player.
	 * |T: Draw a card. */

	check_legend_rule(player, card, event);

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);
		instance->number_of_targets = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER")){
			damage_target0(player, card, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_a_card(player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_order_of_the_stars(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].player = select_a_protection(player);
	}
	if( instance->targets[1].player > -1 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, instance->targets[1].player);
	}
	return 0;
}

int card_orzhov_guildmage(int player, int card, event_t event){

	hybrid(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_XW(2, 1), 0, &td, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XW(2, 1), 0, &td, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XB(2, 1), 0, NULL, NULL) ){
				int ai_choice = 1;
				if( life[player] < 6 && would_validate_arbitrary_target(&td, player, -1) ){
					ai_choice = 0;
				}
				choice = do_dialog(player, player, card, -1, -1, " Target gains 1 life\n All lose 1 life\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XBW(2, (choice == 1), (choice == 0))) ){
			instance->info_slot = 0;
			if( choice == 0 ){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_PLAYER");
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = choice+66;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			gain_life(instance->targets[0].player, 1);
		}
		if( instance->info_slot == 67 ){
			lose_life(player, 1);
			lose_life(1-player, 1);
		}
	}
	return 0;
}

int card_orzhov_pontiff(int player, int card, event_t event){

	haunt(player, card, event);

	if( event == EVENT_HAUNT ){
		int ai_choice = 1;
		if( count_creatures_by_toughness(1-player, 1, 0) > 1  ){
			ai_choice = 0;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Weaken opponent's creatures\n Pump your creatures", ai_choice);
		if( choice == 0 ){
			pump_subtype_until_eot(player, card, 1-player, -1, -1, -1, 0, 0);
		}
		else{
			pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
		}
	}

	return 0;
}

int card_orzhova_the_church_of_deals(int player, int card, event_t event){

	/* Orzhova, the Church of Deals	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |3|W|B, |T: Target player loses 1 life and you gain 1 life. */

	if( ! IS_GAA_EVENT(event) && event != EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;
		if( ! paying_mana() && CAN_ACTIVATE(player, card, MANACOST_XWB(4,1,1)) && can_target(&td) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Suck life from a player\n Cancel", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}

		instance->info_slot = choice;

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if (charge_mana_for_activated_ability(player, card, MANACOST_XWB(3,1,1)) && pick_target(&td, "TARGET_PLAYER") ){
				tapped_for_mana_color = -1;
				instance->number_of_targets = 1;
				instance->info_slot = 1;
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				lose_life(instance->targets[0].player, 1);
				gain_life(player, 1);
			}
		}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_parallectric_feedback(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( card_on_stack_controller != -1 && card_on_stack != -1 ){
			return 99;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			damage_player(instance->targets[1].player, get_cmc(instance->targets[0].player, instance->targets[0].card), player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_petrified_wood_kin(int player, int card, event_t event){

	cannot_be_countered(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_trap_condition(1-player, TRAP_DAMAGE_TAKEN));

	return 0;
}

int card_pillory_of_the_sleepless(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			lose_life(instance->damage_target_player, 1);
		}
	}

	return card_pacifism(player, card, event);
}

int card_plagued_rusalka(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST_B(1), 0, &td, "TARGET_CREATURE");
}

static int effect_predatory_focus(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_source_player == player
	  && damage->damage_target_card != -1)
	{
	  damage->damage_target_player = 1-player;
	  damage->damage_target_card = -1;
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_predatory_focus(int player, int card, event_t event)
{
  /* Predatory Focus	|3|G|G
   * Sorcery
   * You may have creatures you control assign their combat damage this turn as though they weren't blocked. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL && affect_me(player, card))
	{
	  if (do_dialog(player, player, card, -1, -1, " Ignore blockers\n No effect", 0) == 0)
		create_legacy_effect(player, card, effect_predatory_focus);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_primeval_light(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_pyromatics(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_XR(1, 1), &td, "TARGET_CREATURE_OR_PLAYER", 0);
}

int card_rabble_rouser(int player, int card, event_t event)//UNUSEDCARD
{
  bloodthirst(player, card, event, 1);

  // |R, |T: Attacking creatures get +X/+0 until end of turn, where X is ~'s power.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;

	  card_instance_t* instance = get_card_instance(player, card);
	  int pow = get_power(instance->parent_controller, instance->parent_card);

	  pump_creatures_until_eot(player, card, current_turn, 0, pow,0, 0,0, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_R(1), 0, NULL, NULL);
}

int card_repeal(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PERMANENT") ){
			int cost = get_cmc( instance->targets[0].player, instance->targets[0].card );
			if( has_mana(player, COLOR_COLORLESS, cost) ){
				charge_mana(player, COLOR_COLORLESS, cost );
			}
			else{
				 spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_revenant_patriarch(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 0, 0, 1);

	cannot_block(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			if( instance->info_slot == 1 ){
				int result = charge_changed_mana(player, card, 0, 0, 0, 0, 1);
				if( result < 1 ){
					spell_fizzled = 1;
				}
				else{
					instance->targets[1].player = result;
				}
			}
		}
	}

	if( instance->targets[1].player == 2 && comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			target_player_skips_his_next_attack_step(player, card, instance->targets[0].player, 0);
		}
	}

	return 0;
}

int card_rumbling_slum (int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(HUMAN, 1, player, card);
		damage_player(AI, 1, player, card);
	}

	return 0;
}

int card_sanguine_praetor(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_B(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int result = new_sacrifice(player, card, player, SAC_JUST_MARK | SAC_RETURN_CHOICE, &this_test);
			if( result > -1 ){
				int s_player = BYTE2(result);
				int s_card = BYTE3(result);
				instance->info_slot = get_cmc(s_player, s_card);
				kill_card(s_player, s_card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.cmc = instance->info_slot;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return 0;
}

int card_scab_clan_mauler(int player, int card, event_t event){
	return bloodthirst(player, card, event, 2);
}

int card_schismotivate(int player, int card, event_t event){
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_target(&td1);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature for +4/+0.", 0, 1 | GS_LITERAL_PROMPT) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			new_pick_target(&td1, "Select target creature for -4/+0.", 1, 1 | GS_LITERAL_PROMPT);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 0);
		}
		if( validate_target(player, card, &td1, 1) ){
			pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, -4, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_scorched_rusalka(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_PLAYER");
}

int card_seize_the_soul(int player, int card, event_t event){
	/* Seize the Soul	|2|B|B
	 * Instant
	 * Destroy target non|Swhite, non|Sblack creature. Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield.
	 * Haunt
	 * When the creature ~ haunts dies, destroy target non|Swhite, non|Sblack creature. Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_HAUNT ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.illegal_color = COLOR_TEST_BLACK | COLOR_TEST_WHITE;
		if( can_target(&td2) && pick_target(&td2, "TARGET_CREATURE") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.color_forced = COLOR_TEST_WHITE;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
	}

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK | COLOR_TEST_WHITE;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.color_forced = COLOR_TEST_WHITE;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);

			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE );
			td1.allow_cancel = 0;
			td1.illegal_state = TARGET_STATE_DESTROYED;
			if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
				int legacy = create_targetted_legacy_effect(player, card, &haunting_legacy, instance->targets[0].player, instance->targets[0].card);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[2].card = instance->internal_card_id;
				kill_card(player, card, KILL_REMOVE);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonblack, nonwhite creature.", 1, NULL);
}

int card_shadow_lance(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_XB(1, 1), 0, NULL, NULL);
		}

		if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1));
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->damage_target_player, instance->damage_target_card, 2, 2);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_shattering_spree(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_R(1), &td, "TARGET_ARTIFACT", 1);
}

int card_shrieking_grotesque(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 1, 0, 0, 0, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 1, 0, 0, 0, 0);
		}
	}

	if( instance->targets[1].player == 2 && comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			discard(instance->targets[0].player, 0, player);
		}
	}

	return 0;
}

int card_silhana_ledgewalker(int player, int card, event_t event){

	hexproof(player, card, event);

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( ! check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_skarrg_the_rage_pit(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_GR(1, 1), 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump a creature\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_GR(1, 1)) ){
					instance->number_of_targets = 0;
					if( pick_target(&td, "TARGET_CREATURE") ){
						tap_card(player, card);
						instance->info_slot = 1;
					}
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
			if( valid_target(&td) ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										1, 1, KEYWORD_TRAMPLE, 0);
			}
		}
	}

	return 0;
}

int card_skarrgan_firebird(int player, int card, event_t event){//UNUSEDCARD

	bloodthirst(player, card, event, 3);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana(player, COLOR_RED, 3) && get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) > 0 ){
			return GA_RETURN_TO_HAND;
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana(player, COLOR_RED, 3);
			if( spell_fizzled != 1 ){
				return GAPAID_REMOVE;
			}
   }

	return 0;
}

int card_skarrgan_pit_skulk(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( get_power(affected_card_controller, affected_card) < get_power(player, card) ){
				event_result = 1;
			}
		}
	}

	return bloodthirst(player, card, event, 1);
}

int card_skeletal_vampire(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BAT, &token);
		token.pow = 1;
		token.tou = 1;
		token.qty = 2;
		generate_token(&token);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_BAT, 0, 0, 0, 0, 0, -1, 0) ){
			int result = generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST0, 0, NULL, NULL);
			if( result ){
				return result;
			}
			return has_mana_for_activated_ability(player, card, MANACOST_XB(3, 2));
		}
	}

	if( event == EVENT_ACTIVATE ){
		int result = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION, MANACOST0, 0, NULL, NULL);
		if( result ){
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 2;
		}
		if( instance->info_slot == 2 ){
			charge_mana_for_activated_ability(player, card, MANACOST_XB(3, 2));
		}
		if( spell_fizzled != 1 && ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_BAT, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 2 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BAT, &token);
			token.pow = 1;
			token.tou = 1;
			token.qty = 2;
			generate_token(&token);
		}
	}

	return 0;
}

int card_smogsteed_rider(int player, int card, event_t event){

	// Whenever ~ attacks, each other attacking creature gains fear until end of turn.
	if( ! is_humiliated(player, card) && (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS) ){
		if(	declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.not_me = 1;
			this_test.state = STATE_ATTACKING;
			pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_FEAR, &this_test);
		}
	}

	return 0;
}

int card_souls_of_the_faultless(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_dealt(event);
	if (damage && damage->damage_target_card == card && damage->damage_target_player == player){
		get_card_instance(player, card)->info_slot += damage->targets[16].player;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot <= 0){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				gain_life(player, instance->info_slot);
				lose_life(current_turn, instance->info_slot);
				instance->info_slot = 0;
		}
	}

	return 0;
}

int card_spelltithe_enforcer(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int sac = 1;
		if( has_mana(1-player, COLOR_COLORLESS, 1) ){
			charge_mana(1-player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				sac = 0;
			}
		}
		if( sac == 1 ){
			impose_sacrifice(player, card, 1-player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	return 0;
}

int card_starved_rusalka(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_G(1), 0, NULL, NULL);
}

int card_stitch_in_time(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if( flip_a_coin(player, card) ){
			return card_time_walk(player, card, event);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_storm_herd(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_PEGASUS, &token);
		token.pow = 1;
		token.tou = 1;
		token.qty = life[player];
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_sword_of_the_paruns(int player, int card, event_t event){

	/* Sword of the Paruns	|4
	 * Artifact - Equipment
	 * As long as equipped creature is tapped, tapped creatures you control get +2/+0.
	 * As long as equipped creature is untapped, untapped creatures you control get +0/+2.
	 * |3: You may tap or untap equipped creature.
	 * Equip |3 */

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == player ){
			if( event == EVENT_POWER && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				if( is_tapped(instance->targets[8].player, instance->targets[8].card) && is_tapped(affected_card_controller, affected_card) ){
					event_result+=2;
				}
			}
			if( event == EVENT_TOUGHNESS && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				if(  ! is_tapped(instance->targets[8].player, instance->targets[8].card) && ! is_tapped(affected_card_controller, affected_card) ){
					event_result+=2;
				}
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 3);
	}
	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 3) ){
			if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
				choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap or Untap equipped creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			activate_basic_equipment(player, card, 3);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, MANACOST_X(3));
				if( spell_fizzled != 1 ){
					instance->info_slot = 67;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 ){
			int ai_choice = is_tapped(instance->targets[8].player, instance->targets[8].card);
			int choice = do_dialog(player, player, card, -1, -1, " Tap equipped creature\n Untap equipped creature", ai_choice);
			if( choice == 1 ){
				untap_card(instance->targets[8].player, instance->targets[8].card);
			}
			else{
				tap_card(instance->targets[8].player, instance->targets[8].card);
			}
		}
	}
	return 0;
}

static int teysa_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		int p = instance->targets[0].player;
		int c = instance->targets[0].card;

		if( ! in_play(p, c) ){
			instance->targets[1].card = 66;
		}

		if( (instance->token_status & STATUS_INVISIBLE_FX) && instance->targets[1].card == 66 ){
			remove_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! is_humiliated(p, c) ){
			if (event == EVENT_GRAVEYARD_FROM_PLAY ){
				if( affect_me(p, c) ){
					if( get_card_instance(p, c)->kill_code > 0 ){
						instance->targets[1].card = 66;
					}
					return 0;
				}
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.color = COLOR_TEST_BLACK;

				count_for_gfp_ability(player, card, event, instance->targets[0].player, 0, &this_test);
			}

			if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && instance->targets[11].player > 0){
				int trig_player = instance->targets[1].card == 66 ? player : p;
				int trig_card = instance->targets[1].card == 66 ? card : c;
				if( affect_me(trig_player, trig_card ) ){
					if(event == EVENT_TRIGGER){
						//Make all trigges mandatoy for now
						event_result |= RESOLVE_TRIGGER_MANDATORY;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
							token_generation_t token;
							default_token_definition(player, card, CARD_ID_SPIRIT, &token);
							token.t_player = instance->targets[0].player;
							token.qty = instance->targets[11].player;
							token.color_forced = COLOR_TEST_WHITE;
							if (instance->targets[1].card != 66){
								token.color_forced = get_sleighted_color_test(p, c, COLOR_TEST_WHITE);
								token.no_sleight = 1;
							}
							token.key_plus = KEYWORD_FLYING;
							generate_token(&token);
							instance->targets[11].player = 0;
							if( instance->targets[1].card == 66 ){
								kill_card(player, card, KILL_REMOVE);
							}
					}
				}
			}
		}
		if( event == EVENT_CLEANUP && instance->targets[1].card == 66 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_teysa_orzhov_scion(int player, int card, event_t event){
	/* Teysa, Orzhov Scion	|1|W|B
	 * Legendary Creature - Human Advisor 2/3
	 * Sacrifice three |Swhite creatures: Exile target creature.
	 * Whenever another |Sblack creature you control dies, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &teysa_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->token_status |= STATUS_INVISIBLE_FX;
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return can_sacrifice_as_cost(player, 3, TYPE_CREATURE, 0, 0, 0, get_sleighted_color_test(player, card, COLOR_TEST_WHITE), 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, get_sleighted_color_text(player, card, "Select a %s creature to sacrifice.", COLOR_WHITE));
			test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			instance->number_of_targets = 0;
			int sacs[3];
			int sc = 0;
			while( sc < 3 ){
					int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
					if (!sac){
						cancel = 1;
						break;
					}
					sacs[sc] = BYTE3(sac);
					sc++;
			}
			int i;
			for(i=0; i<sc; i++){
				state_untargettable(player, sacs[i], 0);
			}
			if( sc == 3 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					for(i=0; i<sc; i++){
						kill_card(player, sacs[i], KILL_SACRIFICE);
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
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_tibor_and_lumia(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0,
								get_sleighted_color_test(player, card, COLOR_TEST_BLUE) | get_sleighted_color_test(player, card, COLOR_TEST_RED),
								0, 0, 0, -1, 0)
	  ){
		int mode = 0;
		if( get_color(instance->targets[1].player, instance->targets[1].card) & get_sleighted_color_test(player, card, COLOR_TEST_BLUE) ){
			mode+=1;
		}
		if( get_color(instance->targets[1].player, instance->targets[1].card) & get_sleighted_color_test(player, card, COLOR_TEST_RED) ){
			mode+=2;
		}
		if( (mode & 1) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
		if( mode & 2 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			new_damage_all(player, card, 2, 1, 0, &this_test);
		}
	}
	return 0;
}

int card_tin_street_hooligan(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 1, 0, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 1, 0, 0);
		}
	}

	if( instance->targets[1].player == 2 && comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_train_of_thoughts(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return replicate(player, card, event, MANACOST_XU(1, 1), 0, 0, 0);
}

int card_ulasht(int player, int card, event_t event){

	/* Ulasht, the Hate Seed	|2|R|G
	 * Legendary Creature - Hellion Hydra 0/0
	 * ~ enters the battlefield with a +1/+1 counter on it for each other |Sred creature you control and a +1/+1 counter on it for each other |Sgreen creature
	 * you control.
	 * |1, Remove a +1/+1 counter from ~: Choose one - Ulasht deals 1 damage to target creature; or put a 1/1 |Sgreen Saproling creature token onto the
	 * battlefield. */

	check_legend_rule(player, card, event);

	if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		int amount = 0;
		int count = 0;
		while(count < active_cards_count[player] ){
				if( in_play(player, count) && count != card && is_what(player, count, TYPE_CREATURE) ){
					if( get_color(player, count) & get_sleighted_color_test(player, card, COLOR_TEST_GREEN) ){
						amount++;
					}
					if( get_color(player, count) & get_sleighted_color_test(player, card, COLOR_TEST_RED) ){
						amount++;
					}
				}
				count++;
		}
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, amount);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 1;
		if( can_target(&td) ){
			int ai_choice = 1;
			if( !(current_phase > PHASE_MAIN1 && current_phase < PHASE_DECLARE_BLOCKERS) ){
				ai_choice = 0;
			}
			choice = do_dialog(player, player, card, -1, -1, " Damage creature\n Generate Saproling\n Cancel", ai_choice);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if(choice == 0){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_CREATURE");
			}
			if( spell_fizzled != 1 ){
				remove_1_1_counter(player, card);
				instance->info_slot = 66+choice;
			}

		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card);
		}

		if( instance->info_slot == 67 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
			instance->info_slot = 1;
		}

	}

	return 0;
}

int card_vedalken_plotter(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND );
		td.preferred_controller = player;
		td.allowed_controller = player;
		td.allow_cancel = 0;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND );
		td1.preferred_controller = 1-player;
		td1.allowed_controller = 1-player;
		td1.allow_cancel = 0;

		if( can_target(&td) && can_target(&td1) && new_pick_target(&td, "Select target land you control.", 0, GS_LITERAL_PROMPT) ){
			if( new_pick_target(&td1, "Select target land your opponent controls.", 1, GS_LITERAL_PROMPT) ){
				exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
														instance->targets[1].player, instance->targets[1].card);
			}
		}
	}

	return 0;
}

int card_vertigo_spawn(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn !=player && event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event) && ! is_humiliated(player, card) ){
		effect_frost_titan(player, card, 1-player, instance->blocking);
	}

	return 0;
}

int card_wee_dragonauts(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 2, 0);
	}

	return 0;
}

int card_gold(int player, int card, event_t event);
int card_wild_cantor(int player, int card, event_t event){

	hybrid(player, card, event);

	return card_gold(player, card, event);
}

int card_wildsize(int player, int card, event_t event){
	/* Wildsize	|2|G
	 * Instant
	 * Target creature gets +2/+2 and gains trample until end of turn.
	 * Draw a card. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, KEYWORD_TRAMPLE | VANILLA_PUMP_DRAW_A_CARD, 0);
}

int card_witch_maw_nephilim(int player, int card, event_t event){

	// Whenever you cast a spell, you may put two +1/+1 counters on ~.
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_1_1_counters(player, card, 2);
	}

	// Whenever ~ attacks, it gains trample until end of turn if its power is 10 or greater.
	if (declare_attackers_trigger(player, card, event, 0, player, card) && get_power(player, card) >= 10){
		pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_TRAMPLE, 0);
	}

	return 0;
}

int card_wreak_havok(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( new_pick_target(&td, "Select target creature or land.", 0, 1 | GS_LITERAL_PROMPT) ){
			state_untargettable(player, card, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wurmweaver_coil(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_G(3), 0, NULL, NULL);
		}

		if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_G(3)) ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_WURM, &token);
			token.pow = 6;
			token.tou = 6;
			generate_token(&token);
		}
	}

	return generic_aura(player, card, event, player, 6, 6, 0, 0, 0, 0, 0);
}

int card_yore_tiller_nephilim(int player, int card, event_t event)
{
  // Whenever ~ attacks, return target creature card from your graveyard to the battlefield tapped and attacking.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && any_in_graveyard_by_type(player, TYPE_CREATURE) && !graveyard_has_shroud(player)
	  && declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_CREATURE);
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY_ATTACKING, 1, AI_MAX_CMC, &this_test);
	}

  return 0;
}

int card_izzet_boilerworks(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_RED, COLOR_BLUE, CARD_ID_VOLCANIC_ISLAND );
}

int card_gruul_turf(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_RED, COLOR_GREEN, CARD_ID_TAIGA);
}

int card_azorius_chancery(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_WHITE, COLOR_BLUE, CARD_ID_TUNDRA );
}


