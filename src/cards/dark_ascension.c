#include "manalink.h"

// Functions
static int fateful_hour(int player){
	if( life[player] < 6 ){
		return 1;
	}
	return 0;
}

int undying(int player, int card, event_t event)
{
  if (count_1_1_counters(player, card) <= 0)
	{
	  nice_creature_to_sacrifice(player, card);

	  int owner, position;
	  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)
		  && find_in_owners_graveyard(player, card, &owner, &position))
		reanimate_permanent(owner, -1, owner, position, REANIMATE_PLUS1_PLUS1_COUNTER);	// -1 for card doesn't abort since no legacy is added
	}

  return 0;
}

// Cards

// WHITE
int card_archangels_light(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_graveyard(player)*2);
		reshuffle_grave_into_deck(player, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bar_the_door(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 4, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_break_of_day(int player, int card, event_t event)
{
  /* Break of Day	|1|W
   * Instant
   * Creatures you control get +1/+1 until end of turn.
   * Fateful hour - If you have 5 or less life, those creatures gain indestructible until end of turn. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  pump_creatures_until_eot(player, card, player, 0, 1,1, 0,(fateful_hour(player) ? SP_KEYWORD_INDESTRUCTIBLE : 0), NULL);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_burden_of_guilt(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && instance->damage_target_player != -1 ){
				return 1;
			}
		}

		if( event == EVENT_ACTIVATE  ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			tap_card(instance->damage_target_player, instance->damage_target_card);
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_curse_of_echoes(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, instance->targets[0].player, 1+player, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		copy_spell_from_stack(player, instance->targets[1].player, instance->targets[1].card);
	}

	return curse(player, card, event);
}

int card_curse_of_exhaustion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
			trigger_cause_controller == instance->targets[0].player && instance->targets[1].player != 66
		  ){
			trig = 1;
		}
		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->targets[1].player = 66;
			}
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == instance->targets[0].player &&
		instance->targets[1].player == 66
	  ){
		infinite_casting_cost();
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return curse(player, card, event);
}

int card_elgaud_inquisitor(int player, int card, event_t event){
	/* Elgaud Inquisitor	|3|W
	 * Creature - Human Cleric 2/2
	 * Lifelink
	 * When ~ dies, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	lifelink(player, card, event);

	token_generation_t token;
	default_token_definition(player, card, CARD_ID_SPIRIT, &token);
	token.key_plus = KEYWORD_FLYING;
	token.color_forced = COLOR_TEST_WHITE;
	return symbiotic_creature(player, card, event, &token);
}

static int faiths_shield_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player != -1 ){
		int p = opp;
		int i;
		int count = 0;
		while(count < active_cards_count[p]){
				if( get_color(p, count) & instance->targets[1].player ){
					card_instance_t *this_instance = get_card_instance(p, count);
					for(i=0;i<this_instance->number_of_targets;i++){
						if( this_instance->targets[i].player == player && this_instance->targets[i].card == -1 ){
							this_instance->targets[i].player =-1;
						}
					}
					count++;
				}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_faiths_shield(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( ! fateful_hour(player) ){
			if( valid_target(&td) ){
				int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
										0, 0, keyword, 0);
			}
		}
		else{
			int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_subtype_until_eot(player, card, player, -1, 0, 0, keyword, 0);
			int legacy = create_legacy_effect(player, card, &faiths_shield_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = color_to_color_test(keyword_to_color(keyword));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_gather_the_townsfolk(int player, int card, event_t event){
	/* Gather the Townsfolk	|1|W
	 * Sorcery
	 * Put two 1/1 |Swhite Human creature tokens onto the battlefield.
	 * Fateful hour - If you have 5 or less life, put five of those tokens onto the battlefield instead. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 2;
		if( fateful_hour(player) ){
			amount = 5;
		}
		generate_tokens_by_id(player, card, CARD_ID_HUMAN, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gavony_ironwright(int player, int card, event_t event){

	if( fateful_hour(player) ){
		return boost_creature_type(player, card, event, -1, 1, 4, 0, BCT_CONTROLLER_ONLY);
	}
	return 0;
}

int card_hollowhenge_spirit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_ATTACKING);
		}
	}

	return flash(player, card, event);
}


int card_increasing_devotion(int player, int card, event_t event){
	/* Increasing Devotion	|3|W|W
	 * Sorcery
	 * Put five 1/1 |Swhite Human creature tokens onto the battlefield. If ~ was cast from a graveyard, put ten of those tokens onto the battlefield instead.
	 * Flashback |7|W|W */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 5;
		if( get_flashback(player, card) ){
			amount*=2;
		}
		generate_tokens_by_id(player, card, CARD_ID_HUMAN, amount);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return do_flashback(player, card, event, 7, 0, 0, 0, 0, 2);
}

int card_lingering_souls(int player, int card, event_t event){
	/* Lingering Souls	|2|W
	 * Sorcery
	 * Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield.
	 * Flashback |1|B */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = 2;
		token.pow = 1;
		token.tou = 1;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 1, 1, 0, 0, 0, 0);
}

static int loyal_cathar_legacy(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		const int* grave = get_grave(player);
		int count = count_graveyard(player);
		while( count > -1 ){
				if( cards_data[grave[count]].id == CARD_ID_LOYAL_CATHAR ){
					remove_card_from_grave(player, count);
					int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_UNHALLOWED_CATHAR));
					put_into_play(player, card_added);
					card_instance_t *instance = get_card_instance(player, card_added);
					instance->targets[13].player = CARD_ID_LOYAL_CATHAR;
					instance->targets[13].card = CARD_ID_UNHALLOWED_CATHAR;

				}
				count--;
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_loyal_cathar(int player, int card, event_t event){

	vigilance(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		create_legacy_effect(player, card, &loyal_cathar_legacy);
	}

	return 0;
}

int card_unhallowed_cathar(int player, int card, event_t event){

	double_faced_card(player, card, event);

	cannot_block(player, card, event);

	return 0;
}

int card_midnight_guard(int player, int card, event_t event){

	if( is_tapped(player, card) && specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)  ){
		untap_card(player, card);
	}

	return 0;
}

int card_niblis_of_the_mist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_requiem_angel(int player, int card, event_t event){
	/* Requiem Angel	|5|W
	 * Creature - Angel 5/5
	 * Flying
	 * Whenever another non-Spirit creature you control dies, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_SPIRIT;
		this_test.subtype_flag = DOESNT_MATCH;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = instance->targets[11].card;
		generate_token(&token);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_seance(int player, int card, event_t event){

	if(trigger_condition == TRIGGER_UPKEEP && current_turn == player ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player) ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		char msg[100] = "Select a creature to exile.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			const int *grave = get_grave(player);
			int csvid = cards_data[grave[selected]].id;
			rfg_card_from_grave(player, selected);

			token_generation_t token;
			default_token_definition(player, card, csvid, &token);
			token.legacy = 1;
			token.special_code_for_legacy = &remove_at_eot;
			token.action = TOKEN_ACTION_ADD_SUBTYPE;
			token.action_argument = SUBTYPE_SPIRIT;
			generate_token(&token);
		}
	}

	return global_enchantment(player, card, event);
}

int card_sudden_disappearance(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			manipulate_all(player, card, instance->targets[0].player, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0, ACT_RFG_UNTIL_EOT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_thraben_doomsayer(int player, int card, event_t event){
	/* Thraben Doomsayer	|1|W|W
	 * Creature - Human Cleric 2/2
	 * |T: Put a 1/1 |Swhite Human creature token onto the battlefield.
	 * Fateful hour - As long as you have 5 or less life, other creatures you control get +2/+2. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_HUMAN);
	}

	if( fateful_hour(player) ){
		boost_creature_type(player, card, event, -1, 2, 2, 0, BCT_CONTROLLER_ONLY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_thalia_guardian_of_thraben(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	return card_thorn_of_amethyst(player, card, event);
}

int card_thraben_heretic(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) || count_graveyard_by_type(1-player, TYPE_CREATURE) ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			instance->targets[0].player = 1-player;
			if( count_graveyard_by_type(1-player, TYPE_CREATURE) ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
				}
			}
			else{
				instance->targets[0].player = player;
			}

			int selected = select_a_card(player, instance->targets[0].player, 2, 0, 1, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected == - 1 ){
				spell_fizzled = 1;
			}
			const int *grave = get_grave(instance->targets[0].player);
			instance->targets[1].player = selected;
			instance->targets[1].card = grave[selected];
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		const int *grave = get_grave(instance->targets[0].player);
		int selected = instance->targets[1].player;
		if( instance->targets[1].card == grave[selected] ){
			rfg_card_from_grave(player, selected);
		}
	}

	return 0;
}
// BLUE
int card_artful_dodge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
		kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
	}

	return do_flashback(player, card, event, 0, 0, 1, 0, 0, 0);
}

int card_beguiler_of_wills(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	td.power_requirement = count_subtype(player, TYPE_CREATURE, -1) | 0x2000;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_bone_to_ash(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		return card_remove_soul(player, card, event);
	}
	else{
		return card_remove_soul(player, card, event);
	}
	return 0;
}

int card_call_to_the_kindred(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1  ){

		upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			card_instance_t *target = get_card_instance( instance->damage_target_player, instance->damage_target_card);
			int selected = -1;
			int *deck = deck_ptr[player];
			if( player != AI ){
				selected = select_a_card(player, player, 15, 0, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				int i;
				int par = 0;
				for(i=0; i<5; i++){
					if( is_what(-1, deck[i], TYPE_CREATURE) &&
						shares_creature_subtype(-1, deck[i], -1, target->internal_card_id)
					  ){
						if( get_cmc_by_id(cards_data[deck[i]].id) > par ){
							par = get_cmc_by_id(cards_data[deck[i]].id);
							selected = i;
						}
					}
				}
			}
			int amount = 5;
			if( selected != -1 && shares_creature_subtype(-1, deck[selected], -1, target->internal_card_id) ){
				int card_added = add_card_to_hand(player, deck[selected]);
				remove_card_from_deck(player, selected);
				put_into_play(player, card_added);
				amount--;
			}
			put_top_x_on_bottom(player, player, amount);
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}


int card_chant_of_skifsang(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -13, 0, 0, 0, 0, 0, 0);
}

int card_chill_of_foreboding(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		mill(player, 5);
		mill(1-player, 5);
		kill_card(player, card, KILL_DESTROY);
	}

	return do_flashback(player, card, event, 7, 0, 1, 0, 0, 0);
}

int dungeon_geist_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! in_play(instance->targets[1].player, instance->targets[1].card) ){
		kill_card(player, card, KILL_REMOVE);
	}

	does_not_untap(instance->targets[0].player, instance->targets[0].card, event);

	return 0;
}

int card_dungeon_geist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			int legacy = create_targetted_legacy_effect(player, card, &dungeon_geist_effect,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = card;
		}
	}

	return 0;
}

int card_geralfs_mindcrusher(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			mill(instance->targets[0].player, 5);
		}
	}

	return undying(player, card, event);
}

int card_griptide(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_havengul_runebinder(int player, int card, event_t event){

	/* Havengul Runebinder	|2|U|U
	 * Creature - Human Wizard 2/2
	 * |2|U, |T, Exile a creature card from your graveyard: Put a 2/2 |Sblack Zombie creature token onto the battlefield, then put a +1/+1 counter on each
	 * Zombie creature you control. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int selected = select_a_card(player, player, 2, 0, 3, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected != -1 ){
				remove_card_from_grave(player, selected);
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);

		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.subtype = SUBTYPE_ZOMBIE;
		new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	return 0;
}

int card_headless_skaab(int player, int card, event_t event){

	skaabs(player, card, event, 1);

	comes_into_play_tapped(player, card, event);

	return 0;
}

int card_increasing_confusion(int player, int card, event_t event){
	/* Increasing Confusion	|X|U
	 * Sorcery
	 * Target player puts the top X cards of his or her library into his or her graveyard. If ~ was cast from a graveyard, that player puts twice that many cards into his or her graveyard instead.
	 * Flashback |X|U */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana(player, COLOR_BLUE, 1) && can_target(&td) ){
		return check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ? 0 : GA_PLAYABLE_FROM_GRAVE;
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(-1, 1));
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			return GAPAID_EXILE;
		}
	}
	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) && instance->info_slot > 0 ){
			int amount = instance->info_slot;
			if( get_flashback(player, card) ){
				amount*=2;
			}
			mill(instance->targets[0].player, amount);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

int card_mystic_retrieval(int player, int card, event_t event){
	/* Mystic Retrieval	|3|U
	 * Sorcery
	 * Return target instant or sorcery card from your graveyard to your hand.
	 * Flashback |2|R */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XR(2, 1));
	}

	return card_call_to_mind(player, card, event);
}

int card_niblis_of_the_breath(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0)  ){
				return can_target(&td);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int ai_choice = 0;
			if( current_turn != player  ){
				td.preferred_controller = player;
				ai_choice = 1;
			}
			if( pick_target(&td, "TARGET_CREATURE") ){
				int choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", ai_choice);
				instance->number_of_targets = 1;
				instance->targets[1].player = choice;
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->targets[1].player == 0 ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else if( instance->targets[1].player == 1 ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_relentless_skaabs(int player, int card, event_t event){

	skaabs(player, card, event, 1);

	return undying(player, card, event);
}

int card_saving_grasp(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
			if( get_flashback(player, card) ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}

	return do_flashback(player, card, event, 0, 0, 0, 0, 0, 1);
}

int card_screeching_skaab(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		mill(player, 2);
	}

	return 0;
}

int card_secrets_of_the_dead(int player, int card, event_t event){
	/*
	  Secrets of the Dead |2|U
	  Enchantment
	  Whenever you cast a spell from your graveyard, draw a card.
	*/
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( check_special_flags(trigger_cause_controller, trigger_cause, SF_PLAYED_FROM_GRAVE) &&
			get_owner(trigger_cause_controller, trigger_cause) == player
		  ){
			if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				draw_cards(player, 1);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_shriekgeist(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		mill(1-player, 2);
	}

	return 0;
}

int card_stormbound_geist(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) ){
		if( ! check_for_ability(attacking_card_controller, attacking_card, KEYWORD_FLYING) ){
			event_result = 1;
		}
	}

	return undying(player, card, event);
}

int card_thought_scour(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		mill(instance->targets[0].player, 2);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_tower_geist(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		select_one_and_mill_the_rest(player, player, 2, TYPE_ANY);
	}

	return 0;
}


// BLACK
int card_black_cat(int player, int card, event_t event ){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		if( would_validate_target(player, card, &td, 0) ){
			discard(1-player, DISC_RANDOM, player);
		}
	}

	return 0;
}

int card_chosen_of_markov(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_VAMPIRE;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(instance->targets[0].player, instance->targets[0].card);
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		transform(player, instance->parent_card);
	}

	return 0;
}

static int curse_already_enchanting_player(int csvid, int attached_player){
	int p, c;
	for(p=0; p<2; p++){
		for(c = 0; c < active_cards_count[p]; c++ ){
			if( in_play(p, c) && has_subtype(p, c, SUBTYPE_CURSE) && get_id(p, c) == csvid ){
				if( get_card_instance(p, c)->targets[0].player == attached_player ){
					return 1;
				}
			}
		}
	}
	return 0;
}

int card_curse_of_misfortunes(int player, int card, event_t event){


	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

			if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
				char msg[100] = "Select a Curse card.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ENCHANTMENT, msg);
				this_test.subtype = SUBTYPE_CURSE;

				int curse_array[2][count_deck(player)];
				int cac = 0;

				int i = 0;
				while( deck[i] != -1 ){
						if( has_subtype_by_id(cards_data[deck[i]].id, SUBTYPE_CURSE) &&
							! curse_already_enchanting_player(cards_data[deck[i]].id, instance->targets[0].player)
						  ){
							curse_array[0][cac] = deck[i];
							curse_array[1][cac] = i;
							cac++;
						}
						i++;
				}
				if( cac ){
					int selected = select_card_from_zone(player, player, curse_array[0], cac, 0, AI_MAX_CMC, -1, &this_test);
					if( selected != -1 ){
						int card_added = add_card_to_hand(player, deck[curse_array[1][selected]]);
						remove_card_from_deck(player, curse_array[1][selected]);
						set_special_flags(player, card_added, SF_TARGETS_ALREADY_SET);
						card_instance_t *added = get_card_instance(player, card_added);
						added->targets[0] = instance->targets[0];
						put_into_play(player, card_added);
					}
				}
			}
		}
	}

	return curse(player, card, event);
}

int card_curse_of_thirst(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player > -1 ){
		upkeep_trigger_ability(player, card, event, instance->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int count = 0;
			int amount = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_ENCHANTMENT) &&
						has_subtype(player, count, SUBTYPE_CURSE)
					  ){
						card_instance_t *target = get_card_instance(player, count);
						if( target->targets[0].player == instance->targets[0].player ){
							amount++;
						}
					}
					count++;
			}
			if( amount > 0 ){
				damage_player(instance->targets[0].player, amount, player, card);
			}
		}
	}

	return curse(player, card, event);
}

int card_deadly_allure(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_DEATHTOUCH + SP_KEYWORD_MUST_BE_BLOCKED);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 0, 0, 0, 1, 0, 0);
}

int card_deaths_caress(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 0;
			if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_HUMAN) ){
				amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_falkenrath_torturer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
		if( instance->targets[3].player == 1 ){
			add_1_1_counter(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_farbog_boneflinger(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			card_instance_t* instance = get_card_instance(player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
	}

	return 0;
}

int card_fiend_of_the_shadows(int player, int card, event_t event)
{
  /* Fiend of the Shadows	|3|B|B
   * Creature - Vampire Wizard 3/3
   * Flying
   * Whenever ~ deals combat damage to a player, that player exiles a card from his or her hand. You may play that card for as long as it remains exiled.
   * Sacrifice a Human: Regenerate ~. */

  if (has_combat_damage_been_inflicted_to_a_player(player, card, event) && hand_count[1-player] > 0)
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, 0, "Select a card to exile.");

	  int selected = new_select_a_card(1-player, 1-player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
	  int csvid = get_original_id(1-player, selected);
	  rfg_card_in_hand(1-player, selected);
	  create_may_play_card_from_exile_effect(player, card, 1-player, csvid, MPCFE_ATTACH);
	}

  if (!IS_ACTIVATING(event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_PERMANENT, "Select a Human to sacrifice.");

  if (event == EVENT_CAN_ACTIVATE
	  && regeneration(player, card, event, MANACOST0)
	  && new_can_sacrifice_as_cost(player, card, &test))
	return 99;

  if (event == EVENT_ACTIVATE && regeneration(player, card, event, MANACOST0) && spell_fizzled != 1)
	{
	  new_sacrifice(player, card, player, SAC_AS_COST, &test);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	regeneration(player, card, event, MANACOST0);

  return 0;
}

int card_geralfs_messanger(int player, int card, event_t event){

	/* Geralf's Messenger	|B|B|B
	 * Creature - Zombie 3/2
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, target opponent loses 2 life.
	 * Undying */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event) && target_opponent(player, card)){
		lose_life(1-player, 2);
	}

	return undying(player, card, event);
}

int card_gravecrawler(int player, int card, event_t event){
	/* Gravecrawler	|B
	 * Creature - Zombie 2/1
	 * ~ can't block.
	 * You may cast ~ from your graveyard as long as you control a Zombie. */

	cannot_block(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( can_sorcery_be_played(player, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_ZOMBIE;
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				if( has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, card)) ){
					return GA_PLAYABLE_FROM_GRAVE;
				}
			}
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_from_id(player, -1, event, get_id(player, card)) ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}

int card_gruesome_discovery(int player, int card, event_t event){

	/* Gruesome Discovery	|2|B|B
	 * Sorcery
	 * Target player discards two cards.
	 * Morbid - If a creature died this turn, instead that player reveals his or her hand, you choose two cards from it, then that player discards those
	 * cards. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( morbid() ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, player, &this_definition);
				this_test.qty = 2;
				new_effect_coercion(&this_definition, &this_test);
			}
			else{
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_harrowing_journey(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 3);
			lose_life(instance->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_highborn_ghoul(int player, int card, event_t event){
	intimidate(player, card, event);
	return 0;
}

int card_increasing_ambition(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( get_flashback(player, card) ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 7, 1, 0, 0, 0, 0);
}

int mikaeus_the_unhallowed_undying_effect(int player, int card, event_t event){
	// This is less precise than the standard Unying, but oh well...
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
				}
				else{
					if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
						! has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN) && ! is_token(affected_card_controller, affected_card) &&
						! count_counters(affected_card_controller, affected_card, COUNTER_P1_P1)
						){
						if( instance->targets[1].player < 2 ){
							instance->targets[1].player = 2;
						}
						int pos = instance->targets[1].player;
						if( pos < 19 ){
							instance->targets[pos].player = get_owner(affected_card_controller, affected_card);
							instance->targets[pos].card = cards_data[get_card_instance(affected_card_controller, affected_card)->original_internal_card_id].id;
							instance->targets[1].player++;
						}
					}
				}
			}


			if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && instance->targets[1].player > 2){
				int trig_player = instance->targets[1].card == 66 ? player : p;
				int trig_card = instance->targets[1].card == 66 ? card : c;
				if( affect_me(trig_player, trig_card ) ){
					if(event == EVENT_TRIGGER){
						//Make all trigges mandatoy for now
						event_result |= RESOLVE_TRIGGER_MANDATORY;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
						int i;
						for(i=2; i<instance->targets[1].player; i++){
							if( instance->targets[i].player != -1 ){
								seek_grave_for_id_to_reanimate(player, card, instance->targets[i].player, instance->targets[i].card,
															   REANIMATE_UNDER_OWNER_CONTROL | REANIMATE_NO_CONTROL_LEGACY | REANIMATE_PLUS1_PLUS1_COUNTER);
							}
						}
						instance->targets[1].player = 2;
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

int card_mikaeus_the_unhallowed(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	intimidate(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &mikaeus_the_unhallowed_undying_effect);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->token_status |= STATUS_INVISIBLE_FX;
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
	}

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == -1 && damage->damage_target_player == player ){
					if( has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_HUMAN)  ){
						int good = 0;
						if( damage->info_slot > 0 ){
							good = 1;
						}
						else{
							card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
							if( trg->targets[16].player == 1 ){
								good = 1;
							}
						}

						if( good == 1 ){
							if( instance->info_slot < 0 ){
								instance->info_slot = 0;
							}
							instance->targets[instance->info_slot].player = damage->damage_source_player;
							instance->targets[instance->info_slot].card = damage->damage_source_card;
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
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i].player != -1 && instance->targets[i].card != -1 &&
						in_play(instance->targets[i].player, instance->targets[i].card)
						){
						kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
					}
				}
				instance->info_slot = 0;
			}
		}

		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && ! affect_me(player, card) &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			! has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN)
			){
			event_result++;
		}
	}

	return 0;
}

int card_ravenous_demon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_sorcery_be_played(player, event) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( player == AI ){
				if( instance->targets[2].player != 66 ){
					return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
				}
			}
			else{
				return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			target_player_sacrifices_a_subtype(player, card, TYPE_PERMANENT, SUBTYPE_HUMAN, 0)
			){
			instance->targets[2].player = 66;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		transform(player, instance->parent_card);
	}

	return 0;
}

int card_archdemon_of_greed(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int result = target_player_sacrifices_a_subtype(player, card, TYPE_PERMANENT, SUBTYPE_HUMAN, 0);
		if( result == 0 ){
			tap_card(player, card);
			damage_player(player, 9, player, card);
		}
	}

	return 0;
}

int card_reap_the_seagraf(int player, int card, event_t event){
	/* Reap the Seagraf	|2|B
	 * Sorcery
	 * Put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * Flashback |4|U */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, 4, 0, 1, 0, 0, 0);
}

int card_skirsdag_flayer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0) && can_target(&td) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( target_player_sacrifices_a_subtype(player, card, TYPE_PERMANENT, SUBTYPE_HUMAN, 0) ){
				pick_target(&td, "TARGET_CREATURE");
				instance->number_of_targets = 1;
			}
			else{
				spell_fizzled = 1;
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

int card_sightless_ghoul(int player, int card, event_t event){
	cannot_block(player, card, event);
	return undying(player, card, event);
}

int card_spiteful_shadows(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_card != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == c && damage->damage_target_player == p ){
					int good = 0;
					if( damage->info_slot > 0 ){
						good = damage->info_slot;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = trg->targets[16].player;
						}
					}
					instance->info_slot+=good;
				}
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				damage_player(p, instance->info_slot, p, c);
				instance->info_slot = 0;
			}
		}

	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_tragic_slip(int player, int card, event_t event){

	/* Tragic Slip	|B
	 * Instant
	 * Target creature gets -1/-1 until end of turn.
	 * Morbid - That creature gets -13/-13 until end of turn instead if a creature died this turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = -1;
			if( morbid() ){
				amount = -13;
			}
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int undying_evil_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(p, c) && in_play(p, c) && ! is_token(p, c) && count_counters(p, c, COUNTER_P1_P1) < 1 ){
			card_instance_t *dead = get_card_instance(p, c);
			if( dead->kill_code > 0 && dead->kill_code < KILL_REMOVE ){
				instance->targets[11].card = get_original_internal_card_id(p, c);
				instance->targets[11].player = get_owner(p, c);
				instance->damage_target_player = instance->damage_target_card = -1;
			}
		}
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card)){
		if( instance->targets[11].player != -1 ){
			if(event == EVENT_TRIGGER){
				//Make all trigges mandatoy for now
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					seek_grave_for_id_to_reanimate(instance->targets[11].player, -1, instance->targets[11].player, cards_data[instance->targets[11].card].id,
													REANIMATE_PLUS1_PLUS1_COUNTER);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_undying_evil(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, & undying_evil_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_wakedancer(int player, int card, event_t event){

	/* Wakedancer	|2|B
	 * Creature - Human Shaman 2/2
	 * Morbid - When ~ enters the battlefield, if a creature died this turn, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	if( morbid() && comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

	return 0;
}

int card_zombie_apocalypse(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all(player, card, player, TYPE_CREATURE, 0, SUBTYPE_ZOMBIE, 0, 0, 0, 0, 0, -1, 0, REANIMATE_TAP);
		manipulate_all(player, card, player, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
		manipulate_all(player, card, 1-player,TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// RED

// Afflicted Deserter => Gastaf Shepherd
static int effect_werewolf_ransacker(int player, int card, event_t event)
{
  if (effect_put_into_a_graveyard_this_way(player, card, event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  damage_player(instance->targets[0].player, 3, instance->damage_source_player, instance->damage_source_card);
	}

  return 0;
}
int card_werewolf_ransacker(int player, int card, event_t event)
{
  /* Werewolf Ransacker	"" [Afflicted Deserter]
   * Creature - Werewolf 5/4
   * Whenever this creature transforms into ~, you may destroy target artifact. If that artifact is put into a graveyard this way, ~ deals 3 damage to that
   * artifact's controller.
   * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

  werewolf_moon_phases(player, card, event);

  if (event == EVENT_TRANSFORMED)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT"))
		{
		  create_targetted_legacy_effect(player, card, effect_werewolf_ransacker, instance->targets[0].player, instance->targets[0].card);
		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_alpha_brawl(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int my_damage = get_power(instance->targets[0].player, instance->targets[0].card);
			if( my_damage > 0 ){
				damage_all(instance->targets[0].player, instance->targets[0].card, instance->targets[0].player, my_damage, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			int count = active_cards_count[instance->targets[0].player]-1;
			while(count > -1 ){
					if( in_play(instance->targets[0].player, count) && is_what(instance->targets[0].player, count, TYPE_CREATURE) ){
						int dmg = get_power(instance->targets[0].player, count);
						if( dmg > 0 ){
							damage_creature(instance->targets[0].player, instance->targets[0].card, dmg, instance->targets[0].player, count);
						}
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_burning_oil(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card))){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td)){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 3, 0, 0, 0, 0, 1);
}

int card_blood_feud(int player, int card, event_t event){
	/* Blood Feud	|4|R|R
	 * Sorcery
	 * Target creature fights another target creature. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td, 1) ){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_curse_of_bloodletting(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == -1
	  && damage->damage_target_player == get_card_instance(player, card)->targets[0].player)
	damage->info_slot *= 2;

  return curse(player, card, event);
}

int card_faithless_looting(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		multidiscard(player, 2, 0);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 2, 0, 0, 0, 1, 0);
}

int card_fires_of_undeath(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, 5, 1, 0, 0, 0, 0);
}

int card_forge_devil(int player, int card, event_t event){
	if (comes_into_play(player, card, event)){
		card_instance_t* instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
		}
		damage_player(player, 1, player, card);
	}

	return 0;
}

int card_flayer_of_the_hatebound(int player, int card, event_t event){
	/*
	  Flayer of the Hatebound |5|R
	  Creature - Devil 4/2
	  Undying (When this creature dies, if it had no +1/+1 counters on it, return it to the battlefield under its owner's control with a +1/+1 counter on it.)
	  Whenever Flayer of the Hatebound or another creature enters the battlefield from your graveyard, that creature deals damage equal to its power to target creature or player.
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( check_special_flags3(trigger_cause_controller, trigger_cause, SF3_REANIMATED) &&
			get_owner(trigger_cause_controller, trigger_cause) == player && get_card_instance(player, card)->kill_code < KILL_DESTROY
		  ){
			if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				get_card_instance(trigger_cause_controller, trigger_cause)->regen_status |= KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS;
				pandemonium_effect(trigger_cause_controller, trigger_cause);
			}
		}
	}

	return undying(player, card, event);
}

int card_heckling_fiend(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_ATTACK);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 2, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_hinterland_scourge(int player, int card, event_t event){
	werewolf_moon_phases(player, card, event);
	must_be_blocked(player, card, event);
	return 0;
}

int card_hellrider(int player, int card, event_t event)
{
  haste(player, card, event);

  // Whenever a creature you control attacks, ~ deals 1 damage to defending player.
  if (declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1))
	damage_player(1-player, 1, player, card);

  return 0;
}

int card_markov_warlord(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( can_target(&td) && comes_into_play(player, card, event) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
			pump_ability_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( pick_target(&td, "TARGET_CREATURE") ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}
	return 0;
}

int card_moonveil_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 0, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
}

int card_nearheath_stalker(int player, int card, event_t event){

	return undying(player, card, event);
}

static int legacy_2_minimum_blockers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int t_player = instance->targets[0].player;
	int t_card = instance->targets[0].card;

	if( t_player > -1 ){
		menace(t_player, t_card, event);
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_pyreheart_wolf(int player, int card, event_t event){

	/* Pyreheart Wolf	|2|R
	 * Creature - Wolf 1/1
	 * Whenever ~ attacks, creatures you control gain menace until end of turn.
	 * Undying */

	int count;
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		for (count = active_cards_count[player]-1; count >= 0; --count){
			if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
				create_targetted_legacy_effect(player, card, &legacy_2_minimum_blockers, player, count);
			}
		}
	}

	return undying(player, card, event);
}

int card_scorch_the_fields(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_all(player, card, player, 1, 0, 0, 0, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
			damage_all(player, card, 1-player, 1, 0, 0, 0, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_shattered_perception(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int amount = hand_count[player];
			discard_all(player);
			draw_cards(player, amount);

			if( get_flashback(player, card) ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 5, 0, 0, 0, 1, 0);
}


int card_talons_of_falkenrath(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if(event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(player, instance->parent_card, t_player, t_card, 2, 0);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_tovolars_magehunters(int player, int card, event_t event){

	werewolf_moon_phases(player, card, event);

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && 1-player == trigger_cause_controller &&
		! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				damage_player(1-player, 2, player, card);
		}
	}

	return 0;
}

int card_torch_fiend(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_wrack_with_madness(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int amount = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, amount, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}
// GREEN
int card_briarpack_alpha(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		pick_target(&td, "TARGET_CREATURE");
		pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
	}

	return flash(player, card, event);
}

int card_clinging_mists(int player, int card, event_t event){


	if( event == EVENT_CAN_CAST ){
			return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL){
			fog_effect(player, card);
			if( fateful_hour(player) ){
				int p = current_turn;
				int count = active_cards_count[p]-1;
				while( count > -1 ){
						if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) && is_attacking(p, count) ){
							tap_card(p, count);
							effect_frost_titan(player, card, p, count);
						}
						count--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static const char* is_artifact_or_creature_with_flying(int who_chooses, int player, int card)
{
	if( is_what(player, card, TYPE_ARTIFACT) ){
		return NULL;
	}
	if( is_what(player, card, TYPE_CREATURE) ){
		if ( check_for_ability(player, card, KEYWORD_FLYING) ){
			return NULL;
		}
		return EXE_STR(0x785F94);	// ",abilities"
	}
	return EXE_STR(0x728F6C);	// ",type"
}

int card_crushing_vines(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.extra = (int32_t)is_artifact_or_creature_with_flying;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target artifact or creature with flying.", 1, NULL);
}

int card_dawntrader_elk(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_deranged_outcast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_target(&td) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			if( target_player_sacrifices_a_subtype(player, card, TYPE_PERMANENT, SUBTYPE_HUMAN, 0) ){
				pick_target(&td, "TARGET_CREATURE");
				instance->number_of_targets = 1;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 2);
		}
	}

	return 0;
}

int card_favor_of_the_woods(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( current_turn == 1-player && event == EVENT_DECLARE_BLOCKERS &&
			blocking(instance->damage_target_player, instance->damage_target_card, event) ){
			gain_life(player, 3);
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_feed_the_pack(int player, int card, event_t event){
	/* Feed the Pack	|5|G
	 * Enchantment
	 * At the beginning of your end step, you may sacrifice a nontoken creature. If you do, put X 2/2 |Sgreen Wolf creature tokens onto the battlefield, where X is the sacrificed creature's toughness. */

	if( current_turn == player && trigger_condition == TRIGGER_EOT ){
		int can_trigger = can_sacrifice_as_cost(player, 1, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0);
		if( eot_trigger_mode(player, card, event, player, can_trigger ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
				card_instance_t *instance = get_card_instance( player, card );
				generate_tokens_by_id(player, card, CARD_ID_WOLF, instance->targets[2].card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_ghoultreee(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		COST_COLORLESS-=count_graveyard_by_type(player, TYPE_CREATURE);
	}

	return 0;
}

int card_gravetiller_wurm(int player, int card, event_t event){

	/* Gravetiller Wurm	|5|G
	 * Creature - Wurm 4/4
	 * Trample
	 * Morbid - ~ enters the battlefield with four +1/+1 counters on it if a creature died this turn. */

	if (event == EVENT_RESOLVE_SPELL && morbid()){
		add_1_1_counters(player, card, 4);
	}

	return 0;
}

int card_grim_flowering(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		draw_cards(player, count_graveyard_by_type(player, TYPE_CREATURE));
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_hunger_of_the_howlpack(int player, int card, event_t event){

	/* Hunger of the Howlpack	|G
	 * Instant
	 * Put a +1/+1 counter on target creature.
	 * Morbid - Put three +1/+1 counters on that creature instead if a creature died this turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 1;
			if( morbid() ){
				amount+=2;
			}
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_increasing_savagery(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( get_flashback(player, card) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 10);
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 5);
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 5, 0, 0, 2, 0, 0);
}

int card_krallenhorde_killer(int player, int card, event_t event){

	werewolf_moon_phases(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 4, 4);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, 3, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_lost_in_the_woods(int player, int card, event_t event)
{
  /* Whenever a creature attacks you or a planeswalker you control, reveal the top card of your library. If it's a |H2Forest card, remove that creature from
   * combat. Then put the revealed card on the bottom of your library. */
  int amt;
  if (deck_ptr[player][0] != -1 && (amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 1-player, -1)))
	{
	  /* You should be able to select the order of attackers this triggers for, e.g. if you know the top card of your library is a forest.  Maybe select one of
	   * the remaining attackers each resolution?  Would probably want a visual cue. */
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  /* No guarantee that just because there were cards in this library when the trigger began that there still are.  Admittedly, the least contrived situation
	   * I can think of is that there's an "attacking creatures get +1/+0" effect like Mightstone in play; a creature's power and toughness are switched until
	   * end of turn and it's currently at 1 toughness, so it dies when it's removed from combat; and it has a mill-on-death trigger like Rotcrown Ghoul's. */
	  for (--amt; amt >= 0 && deck_ptr[player][0] != -1; --amt)
		{
		  if (deck_ptr[player][0] == -1)
			break;

		  if (in_play(current_turn, attackers[amt]))
			{
			  reveal_card_iid(player, card, deck_ptr[player][0]);
			  if (has_subtype(-1, deck_ptr[player][0], SUBTYPE_FOREST))
				remove_from_combat(current_turn, attackers[amt]);
			  put_top_card_of_deck_to_bottom(player);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_predator_ooze(int player, int card, event_t event){

	indestructible(player, card, event);

	// Whenever ~ attacks, put a +1/+1 counter on it.
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY | DAT_STORE_IN_TARGETS_3, player, card)){
		add_1_1_counter(player, card);
	}

	// Whenever a creature dealt damage by ~ this turn dies, put a +1/+1 counter on ~.
	if( sengir_vampire_trigger(player, card, event, 2) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_scorned_villager(int player, int card, event_t event){
	/* Scorned Villager	|1|G
	 * Creature - Human Werewolf 1/1
	 * |T: Add |G to your mana pool.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.
	 * (Moonscarred Werewolf - 2/2) Vigilance
	 * |T: Add |G|G to your mana pool.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform Moonscarred Werewolf. */

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	return mana_producing_creature(player, card, event, 24, COLOR_GREEN, 1);
}

int card_moonscarred_werewolf(int player, int card, event_t event){
	/* Scorned Villager	|1|G
	 * Creature - Human Werewolf 1/1
	 * |T: Add |G to your mana pool.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.
	 * (Moonscarred Werewolf - 2/2) Vigilance
	 * |T: Add |G|G to your mana pool.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform Moonscarred Werewolf. */

	vigilance(player, card, event);

	werewolf_moon_phases(player, card, event);

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		return 0;	// Don't make extra-reluctant to attack, due to vigilance
	}
	return mana_producing_creature(player, card, event, 0, COLOR_GREEN, 2);
}

int card_silverpelt_werewolf(int player, int card, event_t event){

	werewolf_moon_phases(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_wolfbitten_captive(int player, int card, event_t event){

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	return card_basking_rootwalla(player, card, event);
}

int card_strangleroor_geist(int player, int card, event_t event){

	haste(player, card, event);

	return undying(player, card, event);
}

int card_trackers_instict(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			select_one_and_mill_the_rest(player, player, 4, TYPE_CREATURE);
			if( get_flashback(player, card) ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 2, 0, 1, 0, 0, 0);
}

int card_ulvenwald_bear(int player, int card, event_t event)
{
  /* Ulvenwald Bear	|2|G
   * Creature - Bear 2/2
   * Morbid - When ~ enters the battlefield, if a creature died this turn, put two +1/+1 counters on target creature. */

  if (morbid() && comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 2);
	}

  return 0;
}

int card_village_survivors(int player, int card, event_t event){

	vigilance(player, card, event);

	if( event == EVENT_ABILITIES && fateful_hour(player) ){
		if(affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
			vigilance(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

int card_vorapede(int player, int card, event_t event){

	vigilance(player, card, event);

	return undying(player, card, event);
}

int card_wild_hunger(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 1, KEYWORD_TRAMPLE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


// GOLD
int card_diregraf_captain(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_ZOMBIE;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);
		if( resolve_gfp_ability(player, card, event, would_validate_arbitrary_target(&td, 1-player, -1) ? RESOLVE_TRIGGER_MANDATORY : 0	) ){
			lose_life(1-player, instance->targets[11].card);
		}
		instance->targets[11].card = 0;
	}

	return boost_creature_type(player, card, event, SUBTYPE_ZOMBIE, 1, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_drogskol_captain(int player, int card, event_t event)
{
  // Other Spirit creatures you control get +1/+1 and have hexproof.
  boost_subtype(player, card, event, SUBTYPE_SPIRIT, 1,1, 0,SP_KEYWORD_HEXPROOF, BCT_CONTROLLER_ONLY);
  return 0;
}

int card_drogskol_reaver(int player, int card, event_t event){

	lifelink(player, card, event);

	return 0;
}

static int empty_ends_at_eot(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_falkenrath_aristocrat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( instance->targets[7].player == 66 ){
		indestructible(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int result = pick_creature_for_sacrifice(player, card, 0);
			if( result != -1 ){
				if( has_subtype(player, result, SUBTYPE_HUMAN) ){
					if( instance->targets[5].player < 0 ){
						instance->targets[5].player = 0;
					}
					instance->targets[5].player++;
				}
				kill_card(player, result, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->targets[7].player = 66;
			create_targetted_legacy_effect(player, instance->parent_card, &empty_ends_at_eot, player, instance->parent_card);
			if( instance->targets[5].player > 0 ){
				add_1_1_counter(player, instance->parent_card);
				parent->targets[5].player--;
			}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[7].player = 0;
	}

	return 0;
}

extern int havengul_lich_hack;
static int can_cast_creature_in_grave_with_havengul_lich(int player, int card, int t_player){
	int i;
	for(i=0; i<2; i++){
		if( i == t_player || t_player == 2 ){
			int cg = count_graveyard(i);
			const int *grave = get_grave(i);
			while( cg > -1 ){
					if( is_what(-1, grave[cg], TYPE_CREATURE) ){
						havengul_lich_hack++;
						int result = has_mana_to_cast_id(player, EVENT_CAN_CAST, cards_data[grave[cg]].id);
						havengul_lich_hack--;
						if( result ){
							return 1;
						}
					}
					cg--;
			}
		}
	}
	return 0;
}

static int can_cast_creature_in_grave(int player, int card, int t_player){
	int i;
	for(i=0; i<2; i++){
		if( i == t_player || t_player == 2 ){
			int cg = count_graveyard(i);
			const int *grave = get_grave(i);
			while( cg > -1 ){
					if( is_what(-1, grave[cg], TYPE_CREATURE) ){
						int result = has_mana_to_cast_id(player, EVENT_CAN_CAST, cards_data[grave[cg]].id);
						if( result ){
							return 1;
						}
					}
					cg--;
			}
		}
	}
	return 0;
}

static int can_use_stolen_ability_with_havengul_lich(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
		if( player != AI ){
			int i=2;
			while( i<10 ){
					if( instance->targets[i].card != -1 ){
						card_data_t card_d = cards_data[ instance->targets[i].card ];
						int (*ptFunction)(int, int, event_t) = (void*)card_d.code_pointer;
						if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
							return 1;
						}
					}
					i++;
			}
		}
	return 0;
}

static int can_cast_this(int iid, int unused, int player, int card)
{
	return has_mana_to_cast_iid(player, EVENT_CAN_CAST, iid);
}

int card_havengul_lich(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_X(1)) && ! graveyard_has_shroud(2) ){
			result |= can_cast_creature_in_grave_with_havengul_lich(player, card, 2);
		}
		result |= (player == AI ? 0 : can_use_stolen_ability_with_havengul_lich(player, card));
		return result;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_cast_creature_in_grave_with_havengul_lich(player, card, 2) ){
			if( can_use_stolen_ability_with_havengul_lich(player, card) ){
				choice = do_dialog(player, player, card, -1, -1, " Cast a creature from grave\n Use a stolen ability\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				instance->targets[0].player = 1-player;
				if( can_cast_creature_in_grave(player, card, 1-player) ){
					if( player != AI && can_cast_creature_in_grave(player, card, player) ){
						if( pick_target(&td, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
						}
						else{
							spell_fizzled = 1;
							return 0;
						}
					}
				}
				else{
					instance->targets[0].player = player;
				}

				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");
				this_test.special_selection_function = &can_cast_this;
				int selected = select_target_from_grave_source(player, card, instance->targets[0].player, 0, AI_MAX_CMC, &this_test, 1);
				if( selected != -1 ){
					instance->info_slot = 66;
				}
				if( selected == -1 ){
					spell_fizzled = 1;
					return 0;
				}
			}
		}
		else if( choice == 1 ){
				int act_array[8];
				int aa_count = 0;
				int i=2;
				while( i<10 ){
						if( instance->targets[i].card != -1 ){
							card_data_t card_d = cards_data[ instance->targets[i].card ];
							int (*ptFunction)(int, int, event_t) = (void*)card_d.code_pointer;
							if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
								act_array[aa_count] = instance->targets[i].card;
								aa_count++;
							}
						}
						i++;
				}
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select an ability to activate.");
				int selected = select_card_from_zone(player, player, act_array, aa_count, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					card_data_t* card_d = &cards_data[ act_array[selected] ];
					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					ptFunction(player, card, EVENT_ACTIVATE);
					if( spell_fizzled != 1 ){
						instance->targets[1].card = act_array[selected];
						instance->info_slot = 67;
					}
					else{
						return 0;
					}
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
		}
		else{
			spell_fizzled = 1;
			return 0;
		}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			if( instance->info_slot == 66 ){
				int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
				if( selected != -1 ){
					const int *grave = get_grave(instance->targets[0].player);
					int id = cards_data[grave[selected]].id;
					if( charge_mana_from_id(player, -1, event, id) ){
						if( (cards_data[grave[selected]].extra_ability & EA_ACT_ABILITY) ||
							(cards_data[grave[selected]].extra_ability & EA_MANA_SOURCE)
						  ){
							int i = 2;
							while(i<10 ){
								if( parent->targets[i].player == -1 ){
									parent->targets[i].card = grave[selected];
									break;
								}
								i++;
							}
						}
						int dead = play_card_in_grave_for_free(player, instance->targets[0].player, selected);
						if( instance->targets[0].player != player ){
							card_instance_t *this = get_card_instance(player, dead);
							this->state |= STATE_OWNED_BY_OPPONENT;
						}
					}
				}
			}
			if( instance->info_slot == 67 ){
				card_data_t* card_d = &cards_data[ instance->targets[1].card ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				ptFunction(instance->parent_controller, instance->parent_card, EVENT_RESOLVE_ACTIVATION);
			}
	}

	if( event == EVENT_CLEANUP ){
		int i;
		for(i=2; i<10; i++){
			instance->targets[i].player = -1;
			instance->targets[i].card = -1;
		}
	}

	return 0;
}

int card_huntmaster_of_the_fells(int player, int card, event_t event){

	/* Huntmaster of the Fells	|2|R|G [Ravager of the Fells]
	 * Creature - Human Werewolf 2/2
	 * Whenever this creature enters the battlefield or transforms into ~, put a 2/2 |Sgreen Wolf creature token onto the battlefield and you gain 2 life.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~. */

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( event == EVENT_TRANSFORMED || comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_WOLF);
		gain_life(player, 2);
	}

	return 0;
}
int card_ravager_of_the_fells(int player, int card, event_t event)
{
  /* Ravager of the Fells	"" [Huntmaster of the Fells]
   * Creature - Werewolf 4/4
   * Trample
   * Whenever this creature transforms into ~, it deals 2 damage to target opponent and 2 damage to up to one target creature that player controls.
   * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

  werewolf_moon_phases(player, card, event);

  if (event == EVENT_TRANSFORMED && target_opponent(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
	  td_creature.allowed_controller = td_creature.preferred_controller = instance->targets[0].player;

	  if (can_target(&td_creature) && new_pick_target(&td_creature, "TARGET_CREATURE", 1, 0))
		{
		  damage_player(instance->targets[0].player, 2, player, card);
		  damage_creature(instance->targets[1].player, instance->targets[1].card, 2, player, card);
		}
	  else
		damage_player(instance->targets[0].player, 2, player, card);
	}

  return 0;
}

int card_immerwolf(int player, int card, event_t event){

	/* Immerwolf	|1|R|G
	 * Creature - Wolf 2/2
	 * Intimidate
	 * Other Wolf and Werewolf creatures you control get +1/+1.
	 * Non-Human Werewolves you control can't transform. */

	intimidate(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
		affected_card_controller == player && ! affect_me(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		( has_subtype(affected_card_controller, affected_card, SUBTYPE_WOLF) ||
		  has_subtype(affected_card_controller, affected_card, SUBTYPE_WEREWOLF) ) &&
		in_play(player, card) && in_play(affected_card_controller, affected_card) &&
		!is_humiliated(player, card)
	  ){
		modify_pt_and_abilities(affected_card_controller, affected_card, event, 1, 1, 0);
	}

	return 0;
}

int card_sorins_emblem(int player, int card, event_t event){

	return boost_creature_type(player, card, event, -1, 1, 0, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
}

static int sorin_lord_effect(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);
	int i;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		int good = 0;
		for(i=0; i<3; i++){
			if( affected_card_controller == instance->targets[i].player && affected_card == instance->targets[i].card ){
				good = 1;
				break;
			}
		}
		if( good == 1 ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				if( instance->targets[11].player < 3 ){
					instance->targets[11].player = 3;
				}
				int pos = instance->targets[11].player;
				instance->targets[pos].player = affected_card_controller;
				if( is_stolen(affected_card_controller, affected_card) ){
					instance->targets[pos].player = 1-affected_card_controller;
				}
				instance->targets[pos].card = get_id(affected_card_controller, affected_card);
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 3 && resolve_graveyard_trigger(player, card, event) == 1 ){
		for(i=3; i<instance->targets[11].player; i++){
			seek_grave_for_id_to_reanimate(instance->targets[6].player, instance->targets[6].card, instance->targets[i].player, instance->targets[i].card, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_sorin_lord_of_innistrad(int player, int card, event_t event){

	/* Sorin, Lord of Innistrad	|2|W|B
	 * Planeswalker - Sorin (3)
	 * +1: Put a 1/1 |Sblack Vampire creature token with lifelink onto the battlefield.
	 * -2: You get an emblem with "Creatures you control get +1/+0."
	 * -6: Destroy up to three target creatures and/or other planeswalkers. Return each card put into a graveyard this way to the battlefield under your
	 * control. */

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);
		td.special = TARGET_SPECIAL_NOT_ME;
		if (player == AI){
			td.allowed_controller = 1-player;
		}

		card_instance_t *instance = get_card_instance( player, card );

		enum{
			CHOICE_VAMPIRE = 1,
			CHOICE_EMBLEM,
			CHOICE_KILL_PERMANENTS
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Generate a Vampire", 1, 10, 1,
						"Emblem", 1, ! check_battlefield_for_id(player, CARD_ID_SORINS_EMBLEM) ? count_subtype(player, TYPE_CREATURE, -1)*3 : 0, -2,
						"Kill permanents", can_target(&td), 15, -6);

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
				case CHOICE_VAMPIRE:
				case CHOICE_EMBLEM:
					break;

				case CHOICE_KILL_PERMANENTS:
				{
					int i = 0;
					while( i < 3 && can_target(&td) ){
							if( new_pick_target(&td, "Select another target creature or planeswalker.", i, GS_LITERAL_PROMPT) ){
								state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
								i++;
							}
							else{
								break;
							}
					}
					int k;
					for(k=0; k<i; k++){
						state_untargettable(instance->targets[k].player, instance->targets[k].card, 0);
					}
				}
				break;
			}
		}
		else{	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice)
			{
				case CHOICE_VAMPIRE:
				{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_VAMPIRE, &token);
					token.s_key_plus = SP_KEYWORD_LIFELINK;
					token.pow = 1;
					token.tou = 1;
					generate_token(&token);
				}
				break;

				case CHOICE_EMBLEM:
					generate_token_by_id(player, card, CARD_ID_SORINS_EMBLEM);
					break;

				case CHOICE_KILL_PERMANENTS:
				{
					int legacy = create_legacy_effect(player, instance->parent_card, &sorin_lord_effect);
					card_instance_t *leg = get_card_instance(player, legacy);
					int i;
					for(i=0; i<3; i++){
						leg->targets[i] = instance->targets[i];
					}
					leg->targets[6].player = player;
					leg->targets[6].card = instance->parent_card;
					for(i=0; i<3; i++){
						if( validate_target(player, card, &td, i) ){
							kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
						}
					}
				}
				break;
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_stromkirk_captain(int player, int card, event_t event){

	return boost_creature_type(player, card, event, SUBTYPE_VAMPIRE, 1, 1, KEYWORD_FIRST_STRIKE, BCT_CONTROLLER_ONLY);
}

// ARTIFACTS
int card_avacyns_collar(int player, int card, event_t event){
	/* Avacyn's Collar	|1
	 * Artifact - Equipment
	 * Equipped creature gets +1/+0 and has vigilance.
	 * Whenever equipped creature dies, if it was a Human, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield.
	 * Equip |2 */

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && instance->damage_target_card > -1 ){
		if( has_subtype(instance->damage_target_player, instance->damage_target_card, SUBTYPE_HUMAN) ){
			instance->targets[2].player = 66;
		}
		else{
			instance->targets[2].player = 0;
		}
	}

	if( attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( instance->targets[2].player == 66 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.pow = 1;
			token.tou = 1;
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = COLOR_TEST_WHITE;
			generate_token(&token);
		}
	}

	return vanilla_equipment(player, card, event, 2, 1, 0, 0, SP_KEYWORD_VIGILANCE);
}


int card_chalice_of_life(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[8].card = get_starting_life_total(player);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
		if( life[player] > (instance->targets[8].card + 9) ){
			transform(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_chalice_of_death(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 5);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_elbrus_the_binding_blade(int player, int card, event_t event){

	/* Elbrus, the Binding Blade	|7 [Withengar Unbound]
	 * Legendary Artifact - Equipment
	 * Equipped creature gets +1/+0.
	 * When equipped creature deals combat damage to a player, unattach ~, then transform it.
	 * Equip |1 */

	check_legend_rule(player, card, event);

	double_faced_card(player, card, event);

	if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER)){
		unattach(player, card);
		transform(player, card);
	}

	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_withengar_unbound(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	intimidate(player, card, event);

	return 0;
}

int card_executioners_hood(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( is_equipping(player, card) ){
		int p = instance->targets[8].player;
		int c = instance->targets[8].card;
		intimidate(p, c, event);
	}

	return basic_equipment(player, card, event, 2);
}

int card_heavy_mattock(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( is_equipping(player, card) ){
		int p = instance->targets[8].player;
		int c = instance->targets[8].card;
		int amount = 1;
		if( has_subtype(p, c, SUBTYPE_HUMAN) ){
			amount = 2;
		}
		modify_pt_and_abilities(p, c, event, amount, amount, 0);
	}

	return basic_equipment(player, card, event, 2);
}

int card_helvault(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	if( player == AI ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller = 1-player;
	td2.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == HUMAN && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL) ){
			return 1;
		}
		if( player == AI && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION, MANACOST_X(1), 0, &td1, NULL) ){
			return 99;
		}
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED |GAA_CAN_TARGET, MANACOST_X(7), 0, &td2, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int choice = 1;
		if( (player == HUMAN && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL)) ||
			(player == AI && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION, MANACOST_X(1), 0, &td1, NULL))
		  ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED |GAA_CAN_TARGET, MANACOST_X(7), 0, &td2, NULL) ){
				choice = 1+do_dialog(player, player, card, -1, -1, " Exile your creature\n Exile opponent creature\n Cancel", 1);
			}
		}
		else{
			choice = 2;
		}
		if( choice == 3 ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = choice;
		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
				if( player == HUMAN ){
					new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				}
				else{
					new_pick_target(&td1, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				}
				if( spell_fizzled != 1 ){
					tap_card(player, card);
				}
			}
		}
		else if( choice == 2 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(7)) ){
					if(	new_pick_target(&td2, "Select target creature you don't control.", 0, 1 | GS_LITERAL_PROMPT) ){
						tap_card(player, card);
					}
				}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int good = 0;
		if( instance->info_slot == 1 ){
			if( player == HUMAN && valid_target(&td) ){
				good = 1;
			}
			if( player == AI && valid_target(&td1) ){
				good = 1;
			}
		}
		if( instance->info_slot == 2 ){
			if( valid_target(&td2) ){
				good = 1;
			}
		}
		if( good ){
			if( instance->info_slot == 1 && player == AI ){
				// cheating with the AI so we could convince it to exile one of its creatures if it's dying
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
			exile_permanent_and_remember_it(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	if( this_dies_trigger(player, card, event, 2) ){
		int i;
		for(i=2; i<instance->targets[1].card; i++){
			int iid = instance->targets[i].card;
			if( check_rfg(instance->targets[i].player, cards_data[iid].id) ){
				int card_added = add_card_to_hand(instance->targets[i].player, iid);
				remove_card_from_rfg(instance->targets[i].player, cards_data[iid].id);
				put_into_play(instance->targets[i].player, card_added);
			}
		}
	}

	return 0;
}

int card_jar_of_eyeballs(int player, int card, event_t event){

	/* Jar of Eyeballs	|3
	 * Artifact
	 * Whenever a creature you control dies, put two eyeball counters on ~.
	 * |3, |T, Remove all eyeball counters from ~: Look at the top X cards of your library, where X is the number of eyeball counters removed this way. Put one
	 * of them into your hand and the rest on the bottom of your library in any order. */


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counters(player, card, COUNTER_EYEBALL, 2 * instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
		return count_counters(player, card, COUNTER_EYEBALL);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(3));
		if( spell_fizzled != 1 ){
			instance->info_slot = count_counters(player, card, COUNTER_EYEBALL);
			remove_all_counters(player, card, COUNTER_EYEBALL);
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		impulse_effect(player, instance->info_slot, 0);
	}

	return 0;
}

int card_warden_of_the_wall(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_ABILITIES && affect_me(player, card) && current_turn != player ){
		event_result |= KEYWORD_FLYING;
	}

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && current_turn != player ){
		if( instance->targets[12].card == -1 ){
			int newtype = create_a_card_type(instance->internal_card_id);
			cards_at_7c7000[newtype]->type |= (cards_data[instance->internal_card_id].type | TYPE_CREATURE);
			cards_at_7c7000[newtype]->power = 2;
			cards_at_7c7000[newtype]->toughness = 3;
			instance->targets[12].card = newtype;
		}
		event_result = instance->targets[12].card;
	}

	return mana_producer(player, card, event);
}

int card_wolfhunters_quiver(int player, int card, event_t event){

	/* Wolfhunter's Quiver	|1
	 * Artifact - Equipment
	 * Equipped creature has "|T: This creature deals 1 damage to target creature or player" and "|T: This creature deals 3 damage to target Werewolf creature."
	 * Equip |5 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){

		if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
			! is_sick(instance->targets[8].player, instance->targets[8].card)
		  ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE);
			td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
			td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);

			return can_target(&td1);
		}
		return can_activate_basic_equipment(player, card, event, 5);
	}
	else if( event == EVENT_ACTIVATE ){
			int equip_cost = get_updated_equip_cost(player, card, 5);
			int choice = 0;
			if( has_mana( player, COLOR_COLORLESS, equip_cost) && can_sorcery_be_played(player, event) ){
				if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
					! is_sick(instance->targets[8].player, instance->targets[8].card)
				   ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap & damage\n Do nothing", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 5);
				instance->info_slot = 66;
			}
			else if( choice == 1 ){
				target_definition_t td1;
					default_target_definition(player, card, &td1, TYPE_CREATURE);
					td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
					td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);
					if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
						instance->number_of_targets = 1;
						instance->info_slot = 67;
						tap_card(instance->targets[8].player, instance->targets[8].card);
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
			if( instance->info_slot == 67 ){
				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE);
				td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
				td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);
				if( valid_target(&td1) ){
					int dmg = 1;
					if( instance->targets[0].player != -1 &&
						has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WEREWOLF)
					  ){
						dmg = 3;
					}
					damage_creature(instance->targets[0].player, instance->targets[0].card, dmg,
									instance->targets[8].player, instance->targets[8].card);
				}
			}
	}

	return 0;
}

// LANDS
int card_grim_backwoods(int player, int card, event_t event){

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller = player;
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 1, 0, 1, 0, 0) && can_target(&td2) ){
			ai_choice = 1;
		}

		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac & draw\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0);
				if( spell_fizzled != 1){
					sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
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
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				draw_cards(player, 1);
				parent->info_slot = 0;
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

int card_haunted_fengraf(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			ai_choice = 1;
		}

		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Get random creature\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana(player, COLOR_COLORLESS, 3);
				if( spell_fizzled != 1){
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					 untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				tutor_random_permanent_from_grave(player, card, player, TUTOR_HAND, TYPE_CREATURE, 1, REANIMATE_DEFAULT);
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

int card_vault_of_the_archangel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 1) && count_permanents_by_type(player, TYPE_CREATURE) > 0 ){
			ai_choice = 1;
		}

		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Pump your creatures\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_multi(player, 2, 1, 0, 0, 0, 1);
				if( spell_fizzled != 1){
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
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			pump_subtype_until_eot(player, instance->parent_card, player, -1, 0, 0, 0, SP_KEYWORD_LIFELINK+SP_KEYWORD_DEATHTOUCH);
			parent->info_slot = 0;
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
