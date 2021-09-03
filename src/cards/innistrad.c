#include "manalink.h"

// GLOBAL FUNCTIONS


void human_moon_phases(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[9].card == 66 && current_phase > PHASE_UPKEEP && current_phase < PHASE_DISCARD){
		instance->targets[9].card = 0;
	}

	if( instance->targets[8].player == 0 && instance->targets[8].card == 0 &&
		upkeep_trigger(player, card, event)
	  ){
		transform(player, card);
	}
}

void werewolf_moon_phases(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[9].card == 66 && current_phase > PHASE_UPKEEP && current_phase < PHASE_DISCARD){
		instance->targets[9].card = 0;
	}

	if( (instance->targets[8].player > 1 || instance->targets[8].card > 1)  &&
		upkeep_trigger(player, card, event)
	  ){
		transform(player, card);
	}

}

static void gather_body_parts(int player, int card, int body_parts){
	int count_dead = count_graveyard_by_type(player, TYPE_CREATURE);
	if( count_dead == body_parts ){
		int count = count_graveyard(player)-1;
		const int *grave = get_grave(player);
		while( count > -1 ){
				if( is_what(-1, grave[count], TYPE_CREATURE) ){
					rfg_card_from_grave(player, count);
				}
				count--;
		}
	}
	else{
		int i;
		for(i=0; i<body_parts; i++ ){
			char msg[100] = "Select a creature card to remove.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
		}
	}
}



void skaabs(int player, int card, event_t event, int body_parts){

	if( event == EVENT_MODIFY_COST ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) < body_parts ){
			infinite_casting_cost();
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			gather_body_parts(player, card, body_parts);
		}
	}
}

int curse(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	return enchant_player(player, card, event, &td);
}

int tutor_random_permanent_from_grave(int player, int card, int t_player, int tutor_type, int type, int amount, reanimate_mode_t reanimation_mode){

	int card_added = -1;
	if( count_graveyard_by_type(t_player, type) > 0 ){
		const int *grave = get_grave(t_player);
		int creature_array[99];
		int count = 0;
		int ca_count = 0;
		while( count < count_graveyard(t_player) ){
				if( is_what(-1, grave[count], type) ){
					creature_array[ca_count] = count;
					ca_count++;
				}
				count++;
		}
		if( ca_count <= amount ){
			count = ca_count-1;
			while( count > -1 ){
					if( tutor_type == TUTOR_PLAY ){
						card_added = reanimate_permanent(player, card, t_player, creature_array[count], reanimation_mode);
					}
					if( tutor_type == TUTOR_HAND ){
						card_added = add_card_to_hand(t_player, grave[creature_array[count]]);
						remove_card_from_grave(t_player, creature_array[count]);
					}
					count--;
			}
		}
		else{
			count = 0;
			while( count < amount && ca_count > 0 ){
					int rnd = internal_rand(ca_count);
					if( tutor_type == TUTOR_PLAY ){
						card_added = reanimate_permanent(player, card, t_player, creature_array[rnd], reanimation_mode);
					}
					if( tutor_type == TUTOR_HAND ){
						card_added = add_card_to_hand(t_player, grave[creature_array[rnd]]);
						remove_card_from_grave(t_player, creature_array[rnd]);
					}
					int i;
					for( i = rnd; i < ca_count; i++){
						 creature_array[i] = creature_array[i+1];
					}
					count++;
					ca_count--;
			}
		}
	}
	return card_added;
}

static int can_transform(int player, int card){

	APNAP(p,{
				int c;
				for(c=0; c<active_cards_count[p]; c++){
					if( in_play(p, c) && ! is_humiliated(p, c) ){
						int csvid = get_id(p, c);
						switch( csvid ){
								case CARD_ID_IMMERWOLF:
								{
									if( has_subtype(player, card, SUBTYPE_WEREWOLF) && ! has_subtype(player, card, SUBTYPE_HUMAN) ){
										if( p == player ){
											return 0;
										}
									}
									break;

								}
								case CARD_ID_BOUND_BY_MOONSILVER:
								{
									card_instance_t *instance = get_card_instance( p, c );
									if( instance->damage_target_player == player && instance->damage_target_card == card ){
										return 0;
									}
									break;

								}
								default:
									break;

						}
					}
				}
			};
	);

	return 1;
}

void transform(int player, int card)
{

	if( ! can_transform(player, card) ){
		return;
	}

	true_transform(player, card);

	if( ! check_special_flags3(player, card, SF3_CARD_IS_FLIPPED) ){
		set_special_flags3(player, card, SF3_CARD_IS_FLIPPED);
	}
	else{
		remove_special_flags3(player, card, SF3_CARD_IS_FLIPPED);
	}

	card_instance_t* instance = get_card_instance(player, card);
	instance->regen_status |= KEYWORD_RECALC_ALL;
	get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
	get_abilities(player, card, EVENT_SET_COLOR, -1);

	if (is_what(player, card, TYPE_CREATURE))
	{
	  get_abilities(player, card, EVENT_POWER, -1);
	  get_abilities(player, card, EVENT_TOUGHNESS, -1);
	}
	call_card_function_i(instance, player, card, EVENT_TRANSFORMED);
	dispatch_event_with_initial_event_result(player, card, EVENT_ANOTHER_PERMANENT_HAS_TRANSFORMED, 0);
	verify_legend_rule(player, card, cards_data[instance->targets[12].card].id);
}

// CARDS

int card_army_of_the_damned(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.qty = 13;
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XB(7, 3));
}

int card_back_from_the_brink(int player, int card, event_t event ){
	/* Back from the Brink	|4|U|U
	 * Enchantment
	 * Exile a creature card from your graveyard and pay its mana cost: Put a token onto the battlefield that's a copy of that card. Activate this ability only any time you could cast a sorcery. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED, MANACOST0, 0, NULL, NULL) ){
			const int *grave = get_grave(player);
			int count = 0;
			while( grave[count] != -1 ){
					if( is_what(-1, grave[count], TYPE_CREATURE) ){
						card_ptr_t* c = cards_ptr[ cards_data[grave[count]].id];
						int costs[6] = {c->req_colorless > 15 ? 0 : c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white};
						costs[0] = get_cost_mod_for_activated_abilities(player, card, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]);
						if( has_mana_multi(player, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]) ){
							return 1;
						}
					}
					count++;
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		int playable[2][count_graveyard(player)];
		int pc = 0;
		const int *grave = get_grave(player);
		int count = 0;
		while( grave[count] != -1 ){
				if( is_what(-1, grave[count], TYPE_CREATURE) ){
					card_ptr_t* c = cards_ptr[ cards_data[grave[count]].id];
					int costs[6] = {c->req_colorless > 15 ? 0 : c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white};
					costs[0] = get_cost_mod_for_activated_abilities(player, card, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]);
					if( has_mana_multi(player, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]) ){
						playable[0][pc] = grave[count];
						playable[1][pc] = count;
						pc++;
					}
				}
				count++;
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			card_ptr_t* c = cards_ptr[ cards_data[playable[0][selected]].id];
			int costs[6] = {c->req_colorless > 15 ? 0 : c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white};
			costs[0] = get_cost_mod_for_activated_abilities(player, card, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]);
			charge_mana_multi(player, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]);
			if( spell_fizzled != 1 ){
				instance->info_slot = cards_data[playable[0][selected]].id;
				rfg_card_from_grave(player, playable[1][selected]);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, instance->info_slot, &token);
		token.no_sleight = 1;
		generate_token(&token);
	}
	return global_enchantment(player, card, event);
}

int card_bloodgift_demon(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( player == AI ){
				if( life[1-player] < 2 || count_deck(1-player) < 2 || life[player] < 6 ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					if( ! valid_target(&td) ){
						instance->targets[0].player = player;
						instance->targets[0].card = -1;
						instance->number_of_targets = 1;
					}
				}
				else{
					instance->targets[0].player = player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					if( ! valid_target(&td) ){
						instance->targets[0].player = 1-player;
						instance->targets[0].card = -1;
						instance->number_of_targets = 1;
					}
				}
			}
			else{
				pick_target(&td, "TARGET_PLAYER");
				instance->number_of_targets = 1;
			}
			lose_life(instance->targets[0].player, 1);
			draw_cards(instance->targets[0].player, 1);
		}
	}

	return 0;
}

int card_bloodline_keeper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_VAMPIRE) > 4 && generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_VAMPIRE) > 4 &&
				generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_B(1), 0, NULL, NULL)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Generate vampire\n Transform\n Do nothing", 1);
			}
		}
		else{
			 choice = 1;
		}
		if( choice == 0 ){
			generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL);
				if( spell_fizzled != 1 ){
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_VAMPIRE, &token);
			token.pow = 2;
			token.tou = 2;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
		if( instance->info_slot == 67){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_cackling_counterpart(int player, int card, event_t event){
	/* Cackling Counterpart	|1|U|U
	 * Instant
	 * Put a token onto the battlefield that's a copy of target creature you control.
	 * Flashback |5|U|U */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XU(5, 2));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			copy_token(player, card, player, instance->targets[0].card);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}

int card_champion_of_the_parish(int player, int card, event_t event ){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_HUMAN;
		this_test.not_me = 1;

		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			add_1_1_counter(player, card);
		}
	}

	return 0;
}

int card_clifftop_retreat(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_PLAINS, SUBTYPE_MOUNTAIN);
}

int card_diregraf_ghoul(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	return 0;
}

int card_divine_reckoning(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected[2] = {-1, -1};
		int count = 0;
		int max_cmc = -1;
		while( count < active_cards_count[AI] ){
				if( in_play(AI, count) && is_what(AI, count, TYPE_CREATURE) ){
					if( get_cmc(AI, count) > max_cmc ){
						max_cmc = get_cmc(AI, count);
						selected[AI] = count;
					}
				}
				count++;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = HUMAN;
		td.preferred_controller = HUMAN;
		td.who_chooses = HUMAN;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;

		if( can_target(&td) ){
			pick_target(&td, "TARGET_CREATURE");
			selected[HUMAN] = instance->targets[0].card;
		}

		int i;
		for(i=0; i<2; i++){
			if( selected[i] != -1 ){
				state_untargettable(i, selected[i], 1);
			}
			count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						card_instance_t *this = get_card_instance(i, count);
						if( ! (this->state & STATE_CANNOT_TARGET) ){
							kill_card(i, count, KILL_DESTROY);
						}
					}
					count--;
			}
			if( selected[i] != -1 ){
				state_untargettable(i, selected[i], 0);
			}
		}

		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XW(5, 2));
}

int card_devils_play(int player, int card, event_t event){
	/* Devil's Play	|X|R
	 * Sorcery
	 * ~ deals X damage to target creature or player.
	 * Flashback |X|R|R|R */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_R(3));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			set_special_flags2(player, card, SF2_X_SPELL);
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
			else{
				return 0;
			}
		}
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) && instance->info_slot > 0 ){
			damage_target0(player, card, instance->info_slot);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_endless_ranks_of_the_dead(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int zombies = count_subtype(player, TYPE_CREATURE, SUBTYPE_ZOMBIE) / 2;
		if( zombies > 0 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
			token.pow = 2;
			token.tou = 2;
			token.qty = zombies;
			generate_token(&token);
		}
	}

	return global_enchantment(player, card, event);
}

static int effect_essence_of_the_wild(int player, int card, event_t event)
{
  if (event == EVENT_CHANGE_TYPE && affected_card_controller == player
	  && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
	  && check_special_flags2(affected_card_controller, affected_card, SF2_INFINITE_REFLECTION))
	{
	  card_instance_t* instance = get_card_instance(affected_card_controller, affected_card);
	  if (instance->targets[12].card != -1)
		event_result = instance->targets[12].card;
	}

  return 0;
}
int card_essence_of_the_wild(int player, int card, event_t event)
{
  /* Essence of the Wild	|3|G|G|G
   * Creature - Avatar 6/6
   * Creatures you control enter the battlefield as a copy of ~. */

  card_instance_t* instance;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  // Prevent a redundant effect card from being added each time a creature is cast (even though it'll be invisible)
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((instance = in_play(player, c)) && instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_essence_of_the_wild)
		  goto found;

	  //otherwise
	  {
		int leg = create_legacy_effect(player, card, effect_essence_of_the_wild);
		if (leg != -1)
		  get_card_instance(player, leg)->token_status |= STATUS_INVISIBLE_FX;
	  }

	found:;
	}

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && affected_card_controller == player && ! is_humiliated(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		int p = affected_card_controller, c = affected_card;
		set_special_flags2(p, c, SF2_INFINITE_REFLECTION);
		cloning(p, c, player, card);
		get_card_instance(p, c)->regen_status |= KEYWORD_RECALC_ALL;
		get_abilities(p, c, EVENT_CHANGE_TYPE, -1);
		get_abilities(p, c, EVENT_SET_COLOR, -1);
		get_abilities(p, c, EVENT_POWER, -1);
		get_abilities(p, c, EVENT_TOUGHNESS, -1);
	}

  return 0;
}

int card_geist_honored_monk(int player, int card, event_t event){
	/* Geist-Honored Monk	|3|W|W
	 * Creature - Human Monk 100/100
	 * Vigilance
	 * ~'s power and toughness are each equal to the number of creatures you control.
	 * When ~ enters the battlefield, put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. */

	vigilance(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) && player != -1){
		event_result+=count_subtype(player, TYPE_CREATURE, -1);
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = 2;
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return 0;
}

int card_garruk_relentless(int player, int card, event_t event){

	/* Garruk Relentless	|3|G
	 * Planeswalker - Garruk (3)
	 * When ~ has two or fewer loyalty counters on him, transform him.
	 * 0: ~ deals 3 damage to target creature. That creature deals damage equal to its power to him.
	 * 0: Put a 2/2 |Sgreen Wolf creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( player == AI ){
			td.power_requirement = (count_counters(player, card, COUNTER_LOYALTY)-1) | TARGET_PT_LESSER_OR_EQUAL;
		}

		enum {
			CHOICE_FIGHT = 1,
			CHOICE_WOLF = 2
		} choice = DIALOG(player, card, event,
						  "Fight a creature", can_target(&td), 2,
						  "Make a Wolf", 1, 1);

		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			switch (choice){
				case CHOICE_FIGHT:
					pick_target(&td, "TARGET_CREATURE");
					break;

				case CHOICE_WOLF:
					break;
			}
		} else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_FIGHT:
				{
					if( valid_target(&td) ){
						int t_pow = get_power(instance->targets[0].player, instance->targets[0].card);
						damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, instance->parent_card);
						remove_counters(instance->parent_controller, instance->parent_card, COUNTER_LOYALTY, t_pow);
					}
					break;
				}
				case CHOICE_WOLF:
					generate_token_by_id(player, card, CARD_ID_WOLF);
					break;
			}
		}
	}

	if( event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_LOYALTY) <= 2 && count_counters(player, card, COUNTER_LOYALTY) > 0 &&
		instance->targets[7].card != 66
	  ){
		instance->targets[7].card = 66;
		transform(player, card);
	}

	return planeswalker(player, card, event, 3);
}

int card_garruk_the_veil_cursed(int player, int card, event_t event){

	/* Garruk, the Veil-Cursed	""
	 * Planeswalker - Garruk (3)
	 * +1: Put a 1/1 |Sblack Wolf creature token with deathtouch onto the battlefield.
	 * -1: Sacrifice a creature. If you do, search your library for a creature card, reveal it, put it into your hand, then shuffle your library.
	 * -3: Creatures you control gain trample and get +X/+X until end of turn, where X is the number of creature cards in your graveyard. */

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		int can_tutor = can_sacrifice(player, player, 1, TYPE_CREATURE, 0);
		int ai_priority_tutor = count_counters(player, card, COUNTER_LOYALTY) >= 2 ? 3 - hand_count[player] : -1;
		int ai_priority_overrun = count_counters(player, card, COUNTER_LOYALTY) - 3 + count_graveyard_by_type(player, TYPE_CREATURE);

		enum {
			CHOICE_WOLF = 1,
			CHOICE_TUTOR = 2,
			CHOICE_OVERRUN = 3
		} choice = DIALOG(player, card, event,
						  DLG_RANDOM, DLG_PLANESWALKER,
						  "Make a Wolf", 1, 2, 1,
						  "Sacrifice and tutor", can_tutor, ai_priority_tutor, -1,
						  "Necro-overrun", 1, ai_priority_overrun, -3);

		if (event == EVENT_ACTIVATE){
			switch (choice){
				case CHOICE_WOLF:
					break;

				case CHOICE_TUTOR:
					break;

				case CHOICE_OVERRUN:
					break;
			}
		} else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_WOLF:{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_WOLF, &token);
					token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
					token.color_forced = COLOR_TEST_BLACK;
					token.pow = 1;
					token.tou = 1;
					generate_token(&token);
					break;
				}

				case CHOICE_TUTOR:
					if (sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
						new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
					} else {
						ai_modifier += (player == AI ? -128 : 128);
					}
					break;

				case CHOICE_OVERRUN:{
					int plus = count_graveyard_by_type(player, TYPE_CREATURE);
					card_instance_t* instance = get_card_instance(player, card);
					pump_subtype_until_eot(player, instance->parent_card, player, -1, plus, plus, KEYWORD_TRAMPLE, 0);
					break;
				}
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_ghost_quarter(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	if( player == AI ){
		td.required_subtype = SUBTYPE_BASIC;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->number_of_targets = 0;
		instance->info_slot = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Kill a land\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
					new_pick_target(&td, "TARGET_LAND", 0, 1)
				  ){
					instance->info_slot = 1;
					tap_card(player, card);
					kill_card(player, card, KILL_SACRIFICE);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				if( instance->targets[0].player == AI ||
					(instance->targets[0].player == HUMAN && do_dialog(instance->targets[0].player, player, card, -1, -1, " Tutor a basic land\n Pass", 0) == 0)
				  ){
					tutor_basic_land(instance->targets[0].player, 1, 0);
				}
			}
		}
	}

	return 0;
}

int card_grimgrin_corpse_born(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	// ~ enters the battlefield tapped and doesn't untap during your untap step.
	comes_into_play_tapped(player, card, event);
	does_not_untap(player, card, event);

	// Whenever Grimgrin attacks, destroy target creature defending player controls, then put a +1/+1 counter on ~.
	if( current_turn == player && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		td.allowed_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			add_1_1_counter(player, card);
		}
	}

	// Sacrifice another creature: Untap ~ and put a +1/+1 counter on it.
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		untap_card(instance->parent_controller, instance->parent_card);
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE|GAA_NOT_ME_AS_TARGET, MANACOST_X(0), 0, NULL, NULL);
}

int card_grimoire_of_the_dead(int player, int card, event_t event){

	/* Grimoire of the Dead	|4
	 * Legendary Artifact
	 * |1, |T, Discard a card: Put a study counter on ~.
	 * |T, Remove three study counters from ~ and sacrifice it: Put all creature cards from all graveyards onto the battlefield under your control. They're |Sblack Zombies in addition to their other colors and types. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, GVC_COUNTERS(COUNTER_STUDY, 3), NULL, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, GVC_COUNTERS(COUNTER_STUDY, 3), NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Add counter\n Undead unleashed\n Cancel", 1);
			}
		}
		else{
			 choice = 1;
		}

		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1-choice)) ){
			if( choice == 0 ){
				discard(player, 0, player);
				instance->info_slot = 66+choice;
				tap_card(player, card);
				if (player == AI && !(current_turn == 1-player && current_phase == PHASE_DISCARD)){
					ai_modifier -= 160;	// same amount that would be granted for going from 2 to 3 counters
				}
			}
			if( choice == 1 ){
				instance->info_slot = 66+choice;
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_STUDY);
		}

		if( instance->info_slot == 67 ){
			reanimate_all(player, card, ANYBODY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0, REANIMATE_ADD_BLACK_ZOMBIE | REANIMATE_ALL_UNDER_CASTERS_CONTROL);
		}
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int counters = count_counters(player, card, COUNTER_STUDY);
		counters = MIN(counters, 3);
		ai_modifier += (player == AI ? 32 : -32 ) * counters * counters;
	}

	return 0;
}

int card_heartless_summoning(int player, int card, event_t event ){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE)  ){
			COST_COLORLESS-=2;
		}
	}

	boost_creature_type(player, card, event, -1, -1, -1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	return global_enchantment(player, card, event);
}

int card_hinterland_harbor(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_ISLAND, SUBTYPE_FOREST);
}

int card_howlpack_alpha(int player, int card, event_t event){
	/* Howlpack Alpha	""
	 * Creature - Werewolf 3/3
	 * Each other creature you control that's a Werewolf or a Wolf gets +1/+1.
	 * At the beginning of your end step, put a 2/2 |Sgreen Wolf creature token onto the battlefield.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if( affected_card_controller == player && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! affect_me(player, card) &&
		! is_humiliated(player, card)
	  ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_WEREWOLF) || has_subtype(affected_card_controller, affected_card, SUBTYPE_WOLF) ){
			event_result++;
		}
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_WOLF);
	}

	return 0;
}

int card_instigator_gang(int player, int card, event_t event){

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( event == EVENT_POWER && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_attacking(affected_card_controller, affected_card) ){
			event_result++;
		}
	}

	return 0;
}

int card_isolated_chapel(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_SWAMP, SUBTYPE_PLAINS);
}

int card_lord_of_lineage(int player, int card, event_t event){

	double_faced_card(player, card, event);

	boost_creature_type(player, card, event, SUBTYPE_VAMPIRE, 2, 2, 0, BCT_CONTROLLER_ONLY);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_VAMPIRE, &token);
		token.pow = 2;
		token.tou = 2;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_mayor_of_avabruck(int player, int card, event_t event){

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	boost_creature_type(player, card, event, SUBTYPE_HUMAN, 1, 1, 0, BCT_CONTROLLER_ONLY);

	return 0;
}

int card_mentor_of_the_meek(int player, int card, event_t event){

  /* Mentor of the Meek	|2|W
   * Creature - Human Soldier 2/2
   * Whenever another creature with power 2 or less enters the battlefield under your control, you may pay |1. If you do, draw a card. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.power = 3;
	  test.power_flag = F5_POWER_LESSER_THAN_VALUE;
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test)
		  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 1))
		draw_a_card(player);
	}

  return 0;
}

int card_mikaeus_the_lunarch(int player, int card, event_t event){

	/* Mikaeus, the Lunarch	|X|W
	 * Legendary Creature - Human Cleric 0/0
	 * ~ enters the battlefield with X +1/+1 counters on it.
	 * |T: Put a +1/+1 counter on Mikaeus.
	 * |T, Remove a +1/+1 counter from Mikaeus: Put a +1/+1 counter on each other creature you control. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( count_1_1_counters(player, card) >= 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Pump Mikaeus\n Pump everybody else\n Cancel", get_toughness(player, card) >= 2 ? 1 : 0);
		}
		if( choice != 2 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
				instance->targets[1].card = 66+choice;
				tap_card(player, card);
				if( choice == 1 ){
					remove_1_1_counter(player, card);
				}
			}
		}
		else{
			 spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].card == 66 ){
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->targets[1].card == 67 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "");
			this_test.not_me = 1;
			new_manipulate_all(instance->parent_controller, instance->parent_card, player, &this_test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
		}
	}

	return 0;
}

int card_mindshrieker(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int pump = get_cmc_by_id( cards_data[deck[0]].id );
			mill(instance->targets[0].player, 1);
			pump_until_eot(player, card, player, instance->parent_card, pump, pump);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_PLAYER");
}

int card_moans_of_the_unhallowed(int player, int card, event_t event){
	/* Moan of the Unhallowed	|2|B|B
	 * Sorcery
	 * Put two 2/2 |Sblack Zombie creature tokens onto the battlefield.
	 * Flashback |5|B|B */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL){
		generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, 2);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, MANACOST_XB(5, 2));
}

int card_moonmist(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						if( has_subtype(i, count, SUBTYPE_HUMAN) && has_subtype(i, count, SUBTYPE_WEREWOLF) ){
							card_instance_t *instance = get_card_instance(i, count);
							if( instance->targets[13].player != -1 ){
								transform(i, count);
							}
						}
						if( !( has_subtype(i, count, SUBTYPE_WOLF) || has_subtype(i, count, SUBTYPE_WEREWOLF))){
							negate_combat_damage_this_turn(player, card, i, count, 0);
						}
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_olivia_voldaren(int player, int card, event_t event){

	/* Olivia Voldaren	|2|B|R
	 * Legendary Creature - Vampire 3/3
	 * Flying
	 * |1|R: ~ deals 1 damage to another target creature. That creature becomes a Vampire in addition to its other types. Put a +1/+1 counter on ~.
	 * |3|B|B: Gain control of target Vampire for as long as you control ~. */

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_NOT_ME;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.required_subtype = SUBTYPE_VAMPIRE;
	if( player == AI ){
		td1.allowed_controller = 1-player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XB(3, 2), 0, &td1, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE  ){
		instance->info_slot = instance->number_of_targets = 0;
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, NULL)  ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XB(3, 2), 0, &td1, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Ping creature\n Steal vampire\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			generic_activated_ability(player, card, event, GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, "Select another target creature");
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				generic_activated_ability(player, card, event, GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST_XB(3, 2), 0, &td1, "Select target Vampire permanent.");
				if( spell_fizzled != 1 ){
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card);
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_VAMPIRE);
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &empty, instance->targets[0].player, instance->targets[0].card);
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			if( instance->parent_controller != instance->targets[0].player ){
				gain_control_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card,
																instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED);
			}
		}
	}

	return 0;
}

int past_in_flames_legacy(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		const int *grave = get_grave(player);
		int i = 0;
		int good = 0;
		while( grave[i] != -1 ){
				if( is_what(-1, grave[i], TYPE_SPELL) && can_play_iid(player, event, grave[i]) ){
					good = 1;
					break;
				}
				i++;
		}
		return good;
	}

	if( event == EVENT_ACTIVATE ){
		int playable[count_graveyard(player)];
		int pc = 0;
		const int *grave = get_grave(player);
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select an instant or sorcery card.");
		int i = 0;
		while( grave[i] != -1 ){
				if( is_what(-1, grave[i], TYPE_SPELL) && can_play_iid(player, event, grave[i]) ){
					playable[pc] = grave[i];
					pc++;
				}
				i++;
		}
		int selected = select_card_from_zone(player, player, playable, pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected == -1 ){
			spell_fizzled = 1;
		} else {
			int id = cards_data[playable[selected]].id;
			if( charge_mana_from_id(player, -1, event, id) ){
				int count = count_graveyard(player)-1;
				while( count > -1 ){
						if( grave[count] == playable[selected] ){
							play_card_in_grave_for_free_and_exile_it(player, player, count);
							cant_be_responded_to = 1;
							break;
						}
						count--;
				}
				instance->targets[0].card = id;
			}
		}
	}
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_past_in_flames(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_activate(player, card, &past_in_flames_legacy);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XR(4, 1));
}

int card_rakish_heir(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);
	card_instance_t* damage = combat_damage_being_dealt(event);

	if( damage &&
		damage->damage_source_player == player && damage->damage_source_card != -1 &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_VAMPIRE) &&
		instance->info_slot < 40
	  ){
		unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
		creatures[2 * instance->info_slot] = damage->damage_source_player;
		creatures[2 * instance->info_slot + 1] = damage->damage_source_card;
		instance->info_slot++;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
				int i;
				for (i = 0; i < instance->info_slot; i++){
					add_1_1_counter(creatures[2 * i], creatures[2 * i + 1]);
				}
				instance->info_slot = 0;
		}
	}

	return 0;
}

int card_reaper_from_the_abyss(int player, int card, event_t event)
{
  /* Reaper from the Abyss	|3|B|B|B
   * Creature - Demon 6/6
   * Flying
   * Morbid - At the beginning of each end step, if a creature died this turn, destroy target non-Demon creature. */

  if (morbid() && eot_trigger(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.required_subtype = SUBTYPE_DEMON;
	  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target non-Demon creature."))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return 0;
}

int card_rooftop_storm(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller,affected_card, TYPE_CREATURE) &&
			has_subtype(affected_card_controller,affected_card, SUBTYPE_ZOMBIE)
		  ){
			card_ptr_t* c = cards_ptr[ get_id(affected_card_controller,affected_card) ];
			COST_COLORLESS-=c->req_colorless;
			COST_BLACK-=c->req_black;
			COST_BLUE-=c->req_blue;
			COST_GREEN-=c->req_green;
			COST_RED-=c->req_red;
			COST_WHITE-=c->req_white;
		}
	}

	return global_enchantment(player, card, event);
}

int card_skirsdag_high_priest(int player, int card, event_t event){

	/* Skirsdag High Priest	|1|B
	 * Creature - Human Cleric 1/2
	 * Morbid - |T, Tap two untapped creatures you control: Put a 5/5 |Sblack Demon creature token with flying onto the battlefield. Activate this ability only
	 * if a creature died this turn. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) && morbid() ){
			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			test.state = STATE_TAPPED;
			test.state_flag = DOESNT_MATCH;
			test.not_me = 1;
			if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test) > 1 ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			state_untargettable(player, card, 1);
			if( tapsubtype_ability(player, card, 2, &td) ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
			state_untargettable(player, card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_DEMON);
	}

	return 0;
}

int card_sulfur_falls(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_ISLAND, SUBTYPE_MOUNTAIN);
}

int card_snapcaster_mage(int player, int card, event_t event ){

	/* Snapcaster Mage	|1|U
	 * Creature - Human Wizard 2/1
	 * Flash
	 * When ~ enters the battlefield, target instant or sorcery card in your graveyard gains flashback until end of turn. The flashback cost is equal to its
	 * mana cost. */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select target instant or sorcery card.");

		if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(2) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
			create_spell_has_flashback_legacy(player, card, selected, 0);
		}
	}
	return flash(player, card, event);
}

int card_skaab_ruinator(int player, int card, event_t event ){
	/* Skaab Ruinator	|1|U|U
	 * Creature - Zombie Horror 5/6
	 * As an additional cost to cast ~, exile three creature cards from your graveyard.
	 * Flying
	 * You may cast ~ from your graveyard. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		int cless = get_updated_casting_cost(player, card, -1, event, 1);
		if( has_mana_multi(player, MANACOST_XU(cless, 2)) && count_graveyard_by_type(player, TYPE_CREATURE) > 3 && can_sorcery_be_played(player, event) ){
			return GA_PLAYABLE_FROM_GRAVE;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		int cless = get_updated_casting_cost(player, card, -1, event, 1);
		charge_mana_multi(player, MANACOST_XU(cless, 2));
		if( spell_fizzled != 1 ){
			gather_body_parts(player, card, 3);
			return GAPAID_REMOVE;
		}
	}

	skaabs(player, card, event, 3);

	return 0;
}

int card_stitchers_apprentice(int player, int card, event_t event ){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HOMUNCULUS, &token);
		token.pow = 2;
		token.tou = 2;
		generate_token(&token);

		card_instance_t *instance = get_card_instance(player, card);
		impose_sacrifice(instance->parent_controller, instance->parent_card, instance->parent_controller, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(1, 1), 0, NULL, NULL);
}

int card_undead_alchemist(int player, int card, event_t event)
{
  /* Undead Alchemist	|3|U
   * Creature - Zombie 4/2
   * If a Zombie you control would deal combat damage to a player, instead that player puts that many cards from the top of his or her library into his or her
   * graveyard.
   * Whenever a creature card is put into an opponent's graveyard from his or her library, exile that card and put a 2/2 |Sblack Zombie creature token onto the
   * battlefield. */

  card_instance_t* instance = get_card_instance(player, card);
  card_instance_t* damage = combat_damage_being_dealt(event);

  if (damage
	  && damage->damage_target_card == -1 && !damage_is_to_planeswalker(damage)
	  && (damage->targets[3].player & TYPE_CREATURE)	// probably redundant to combat check
	  && has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_ZOMBIE))
	{
	  instance->info_slot += damage->info_slot;
	  damage->info_slot = 0;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
	  && instance->info_slot > 0)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= 2;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  mill(1-player, instance->info_slot);
		  instance->info_slot = 0;
		}
	}

  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == 1-player && !is_humiliated(player, card))
	{
	  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
		return 0;

	  int i;
	  for (i = 0; i < num_cards_milled; ++i)
		if (is_what(-1, cards_milled[i].internal_card_id, TYPE_CREATURE))
		  {
			if (event == EVENT_TRIGGER)
			  {
				event_result |= RESOLVE_TRIGGER_MANDATORY;
				return 0;
			  }

			if (event == EVENT_RESOLVE_TRIGGER)
			  {
				if (cards_milled[i].position != -1)
				  {
					int pos = find_in_graveyard_by_source(1-player, cards_milled[i].source, cards_milled[i].position);
					if (pos != -1)
					  rfg_card_from_grave(1-player, pos);
					cards_milled[i].position = -1;	// No longer in graveyard, so keep any other triggers from looking
				  }

				generate_token_by_id(player, card, CARD_ID_ZOMBIE);	// Whether or not we successfully exiled it
			  }
		  }
	}

  return 0;
}

int card_woodland_cemetery(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_SWAMP, SUBTYPE_FOREST);
}

int card_wildblood_pack(int player, int card, event_t event){

	double_faced_card(player, card, event);

	werewolf_moon_phases(player, card, event);

	if( event == EVENT_POWER && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_attacking(affected_card_controller, affected_card) ){
			event_result+=3;
		}
	}

	return 0;
}

int card_intangible_virtue(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			if( affected_card_controller == player && is_token(affected_card_controller, affected_card) ){
				event_result++;
			}
		}
		if( event == EVENT_ABILITIES ){
			if( affected_card_controller == player && is_token(affected_card_controller, affected_card) ){
				vigilance(affected_card_controller, affected_card, event);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_forbidden_alchemy(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if( event == EVENT_RESOLVE_SPELL ){
		select_one_and_mill_the_rest(player, player, 4, TYPE_ANY);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, MANACOST_XB(6, 1));
}

int card_unburial_rites(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		this_test.ai_selection_mode = AI_MAX_CMC;
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
	}

	if(event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XW(3, 1));
}


int card_liliana_of_the_veil(int player, int card, event_t event){

	/* Liliana of the Veil	|1|B|B
	 * Planeswalker - Liliana (3)
	 * +1: Each player discards a card.
	 * -2: Target player sacrifices a creature.
	 * -6: Separate all permanents target player controls into two piles. That player sacrifices all permanents in the pile of his or her choice. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_ALL_DISCARD = 1,
			CHOICE_DIABOLIC_EDICT,
			CHOICE_ULTIMATUM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Each player discards", 1, 5, 1,
						"Sacrifice creature", can_target(&td), count_subtype(1-player, TYPE_CREATURE, -1) ? 10 : 0, -2,
						"Sacrifice pile", can_target(&td), 3*count_subtype(1-player, TYPE_PERMANENT, -1), -6);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			if( choice == CHOICE_DIABOLIC_EDICT || choice == CHOICE_ULTIMATUM ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case( CHOICE_ALL_DISCARD ):
						APNAP(p, {discard(p, 0, player);};);
						break;

				case( CHOICE_DIABOLIC_EDICT ):
						impose_sacrifice(player, instance->parent_card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
						break;
				case( CHOICE_ULTIMATUM ):
				{
						int t_player = instance->targets[0].player;
						if( player != AI ){
							target_definition_t td1;
							default_target_definition(player, card, &td1, TYPE_PERMANENT);
							td1.allowed_controller = t_player;
							td1.preferred_controller = t_player;
							td1.illegal_abilities = 0;
							while( can_target(&td1) ){
									if( pick_target(&td1, "TARGET_PERMANENT") ){
										state_untargettable(t_player, instance->targets[0].card, 1);
									}
									else{
										 break;
									}
							}
						}
						else{
							 int counts[5][2];
							 counts[0][0] = TYPE_CREATURE;
							 counts[0][1] = 0;
							 counts[1][0] = TYPE_LAND;
							 counts[1][1] = 0;
							 counts[2][0] = TYPE_ARTIFACT;
							 counts[2][1] = 0;
							 counts[3][0] = TYPE_ENCHANTMENT;
							 counts[3][1] = 0;
							 counts[4][0] = 999;
							 counts[4][1] = 0;
							 int count = 0;
							 while( count < active_cards_count[t_player] ){
									if( in_play(t_player, count) && is_what(t_player, count, TYPE_PERMANENT)){
										int k;
										for(k=0; k<4; k++){
											if( is_planeswalker(t_player, count)){
												counts[4][1]++;
											}
											else{
												if( is_what(t_player, count, counts[k][0]) ){
													counts[k][1]++;
												}
											}
										}
									}
									count++;
							}
							int i;
							int selected_type = -1;
							int par = 0;
							for(i=0; i<5; i++){
								if( counts[i][1] > par ){
									par = counts[i][1];
									selected_type = counts[i][0];
								}
							}
							count = 0;
							while( count < active_cards_count[t_player] ){
									if( in_play(t_player, count) && is_what(t_player, count, TYPE_PERMANENT) ){
										if( selected_type != 999 ){
											if( is_what(t_player, count, selected_type) ){
												state_untargettable(t_player, count, 1);
											}
										}
										else{
											if( is_planeswalker(t_player, count) ){
												state_untargettable(t_player, count, 1);
											}
										}
									}
									count++;
							}
						}
						int ai_choice = 0;
						if( t_player == AI ){
							int counts[2] = {0,0};
							int count = 0;
							while( count < active_cards_count[t_player] ){
									if( in_play(t_player, count) && is_what(t_player, count, TYPE_PERMANENT) ){
										card_instance_t *this= get_card_instance(t_player, count);
										card_ptr_t* c = cards_ptr[ get_id(player, count) ];
										if( this->state & STATE_CANNOT_TARGET ){
											counts[0]+=c->ai_base_value;
										}
										else{
											counts[1]+=c->ai_base_value;
										}
									}
									count++;
							}
							if( counts[0] > counts[1] ){
								ai_choice = 1;
							}
						}
						int choice2 = do_dialog(t_player, player, card, -1, -1, " Sac signed pile\n Sac unsigned pile", ai_choice);
						int count = active_cards_count[t_player]-1;
						while( count > -1 ){
								if( in_play(t_player, count) && is_what(t_player, count, TYPE_PERMANENT) ){
									card_instance_t *this= get_card_instance(t_player, count);
									if( this->state & STATE_CANNOT_TARGET ){
										if( choice2 == 0 ){
											kill_card(t_player, count, KILL_SACRIFICE);
										}
										else{
											state_untargettable(t_player, count, 0);
										}
									}
									else{
										if( choice2 == 1 ){
											kill_card(t_player, count, KILL_SACRIFICE);
										}
									}
								}
								count--;

						}
				}
				break;
			}
		}
	}
	return planeswalker(player, card, event, 3);
}

int card_doomed_traveller(int player, int card, event_t event){
	/* Doomed Traveler	|W
	 * Creature - Human Soldier 1/1
	 * When ~ dies, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}
	return 0;
}

int card_deranged_assistant(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				if( player != AI ){
					return 1;
				}
				else{
					if( deck[9] != -1 ){
						return 1;
					}
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		tap_card(player, card);
		mill(player, 1);
		return mana_producer(player, card, event);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				if( player != AI ){
					declare_mana_available(player, COLOR_COLORLESS, 1);
				}
				else{
					if( deck[9] != -1 ){
						declare_mana_available(player, COLOR_COLORLESS, 1);
					}
				}
			}
		}
	}
	return 0;
}


int card_blasphemous_act(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		COST_COLORLESS -= count_subtype(ANYBODY, TYPE_CREATURE, -1);
	}

	if(event == EVENT_RESOLVE_SPELL){
		APNAP(p, {new_damage_all(player, card, p, 13, NDA_ALL_CREATURES, NULL);});
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_cloistered_youth(int player, int card, event_t event){

	double_faced_card(player, card, event);

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		transform(player, card);
	}

	return 0;
}

int card_unholy_fiend(int player, int card, event_t event){

	if( current_turn == player && eot_trigger(player, card, event) ){
		lose_life(player, 1);
	}

	return 0;
}

int card_mausoleum_guard(int player, int card, event_t event){
	/* Mausoleum Guard	|3|W
	 * Creature - Human Scout 2/2
	 * When ~ dies, put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = 2;
		generate_token(&token);
	}
	return 0;
}

int card_midnight_haunting(int player, int card, event_t event){
	/* Midnight Haunting	|2|W
	 * Instant
	 * Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = 2;
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_selfless_cathar(int player, int card, event_t event ){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XW(1, 1), 0, NULL, NULL);
}

int card_silverchase_fox(int player, int card, event_t event ){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_XW(1, 1), 0, &td, "TARGET_ENCHANTMENT");
}


int card_lost_in_the_mist(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		counterspell(player, card, event, NULL, 0);
		new_pick_target(&td, "TARGET_PERMANENT", 1, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[1].player, instance->targets[1].card);
		}
		if( validate_target(player, card, &td, 1) ){
			bounce_permanent(instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_skaab_goliath(int player, int card, event_t event ){

	skaabs(player, card, event, 2);

	return 0;
}

int card_stitched_drake(int player, int card, event_t event ){

	skaabs(player, card, event, 1);

	return 0;
}

int card_altars_reap(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_disciple_of_griselbrand(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int result = sacrifice_and_report_value(player, card, player, SARV_REPORT_TOUGHNESS, &this_test);
			if( result > -1 ){
				instance->info_slot = result;
				instance->number_of_targets = 0;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->info_slot);
	}

	return 0;
}

int card_unbreathing_horde(int player, int card, event_t event ){
	/* Unbreathing Horde	|2|B
	 * Creature - Zombie 0/0
	 * ~ enters the battlefield with a +1/+1 counter on it for each other Zombie you control and each Zombie card in your graveyard.
	 * If ~ would be dealt damage, prevent that damage and remove a +1/+1 counter from it. */

	if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		int total = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ZOMBIE) + count_graveyard_by_subtype(player, SUBTYPE_ZOMBIE);
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, total);
	}

	phantom_effect(player, card, event, 0);

	return 0;
}

int card_balefire_dragon(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT+DDBM_REPORT_DAMAGE_DEALT) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 1-player, instance->targets[16].player, NDA_ALL_CREATURES, &this_test);
	}

	return 0;
}

int card_brimstone_volley(int player, int card, event_t event){

	/* Brimstone Volley	|2|R
	 * Instant
	 * ~ deals 3 damage to target creature or player.
	 * Morbid - ~ deals 5 damage to that creature or player instead if a creature died this turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		damage_target0(player, card, morbid() ? 5 : 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_charmbreaker_devils(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		const int *grave = get_grave(player);
		int array[count_graveyard(player)];
		int array_count = 0;
		int count = 0;
		int count_g = count_graveyard(player);
		while( count < count_g ){
				if( is_what(-1, grave[count], TYPE_SORCERY | TYPE_INSTANT | TYPE_INTERRUPT) &&
					! is_what(-1, grave[count], TYPE_PERMANENT)
				  ){
					array[array_count] = count;
					array_count++;
				}
				count++;
		}
		if( array_count > 0 ){
			int random_card = internal_rand(array_count);
			add_card_to_hand(player, grave[array[random_card]]);
			remove_card_from_grave(player, array[random_card]);
		}
	}

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 4, 0);
	}

	return 0;
}

int card_falkenrath_marauders(int player, int card, event_t event){

	haste(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		add_1_1_counters(player, card, 2);
	}

	return 0;
}

int card_vampiric_fury(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, SUBTYPE_VAMPIRE, 2, 0, KEYWORD_FIRST_STRIKE, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_bramblecrush(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target noncreature permanent.", 1, NULL);
}

int card_caravan_vigil(int player, int card, event_t event)
{
  /* Caravan Vigil	|G
   * Sorcery
   * Search your library for a basic land card, reveal it, put it into your hand, then shuffle your library.
   * Morbid - You may put that card onto the battlefield instead of putting it into your hand if a creature died this turn. */

  if (event == EVENT_CAN_CAST)
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int destination = TUTOR_HAND;
	  if (morbid()
		  && (duh_mode(player)
			  || do_dialog(player, player, card, -1, -1, " Put the land into play\n Leave in hand", 0) == 0))
		destination = TUTOR_PLAY;

	  tutor_basic_lands(player, destination, 1);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_creeping_renaissance(int player, int card, event_t event)
{
  /* Creeping Renaissance	|3|G|G
   * Sorcery
   * Choose a permanent type. Return all cards of the chosen type from your graveyard to your hand.
   * Flashback |5|G|G */

  if (event == EVENT_CAN_CAST)
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  const int types[] =
		{
		  -1,
		  TYPE_ARTIFACT,
		  TYPE_CREATURE,
		  TYPE_ENCHANTMENT,
		  TYPE_LAND,
		  TARGET_TYPE_PLANESWALKER,
		  0	// for tribal
		};
	  int type = types[DIALOG(player, card, event,
							  DLG_RANDOM, DLG_NO_CANCEL, DLG_NO_STORAGE,
							  raw_get_subtype_text(SUBTYPE_ARTIFACT), 1, 1,
							  raw_get_subtype_text(SUBTYPE_CREATURE), 1, 1,
							  raw_get_subtype_text(SUBTYPE_ENCHANTMENT), 1, 1,
							  raw_get_subtype_text(SUBTYPE_LAND), 1, 1,
							  raw_get_subtype_text(SUBTYPE_PLANESWALKER), 1, 1,
							  raw_get_subtype_text(SUBTYPE_TRIBAL), 1, 1)];

	  test_definition_t test;
	  new_default_test_definition(&test, type, "");
	  if (type == 0)
		test.subtype = SUBTYPE_TRIBAL;

	  int num_returned = from_grave_to_hand_multiple(player, &test);
	  if (player == AI)
		{
		  if (num_returned == 0)
			ai_modifier -= 96;
		  else
			ai_modifier += 12 * num_returned - 24;
		}

	  kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
	}

  return do_flashback(player, card, event, MANACOST_XG(5, 2));
}

int card_elder_of_laurels(int player, int card, event_t event ){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			int amount = count_subtype(player, TYPE_CREATURE, -1);
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(3, 1), 0, &td1, "TARGET_CREATURE");
}

int card_festerhide_boar(int player, int card, event_t event ){

	/* Festerhide Boar	|3|G
	 * Creature - Boar 3/3
	 * Trample
	 * Morbid - ~ enters the battlefield with two +1/+1 counters on it if a creature died this turn. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		if( ! morbid() ){
			ai_modifier-=25;
		}
	}

	if( event == EVENT_RESOLVE_SPELL && morbid()  ){
		add_1_1_counters(player, card, 2);
	}
	return 0;
}

int card_full_moons_rise(int player, int card, event_t event ){

	boost_creature_type(player, card, event, SUBTYPE_WEREWOLF, 1, 0, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_DESTROYED;
		td.allowed_controller = player;
		td.preferred_controller = player;
		if( player == AI ){
			td.special = TARGET_SPECIAL_REGENERATION;
		}
		td.illegal_abilities = 0;

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int c;
			for(c = active_cards_count[player]-1; c > -1; c--){
				if( in_play(player, c) && has_subtype(player, c, SUBTYPE_WEREWOLF) ){
					if( can_be_regenerated(player, c) ){
						regenerate_target( player, c );
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_gutter_grime(int player, int card, event_t event){

	/* Gutter Grime	|4|G
	 * Enchantment
	 * Whenever a nontoken creature you control dies, put a slime counter on ~, then put a |Sgreen Ooze creature token onto the battlefield with "This
	 * creature's power and toughness are each equal to the number of slime counters on ~." */

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_counters(player, card, COUNTER_SLIME, instance->targets[11].card);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.pow = token.tou = 0;
		token.qty = instance->targets[11].card;
		token.special_infos = 66;
		generate_token(&token);

		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_hamlet_captain(int player, int card, event_t event)
{
  // Whenever ~ attacks or blocks, other Human creatures you control get +1/+1 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  || (blocking(player, card, event) && !is_humiliated(player, card)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_HUMAN;
	  test.not_me = 1;
	  pump_creatures_until_eot(player, card, player, 0, 1,1, 0,0, &test);
	}

  return 0;
}

int card_kessig_cagebreakers(int player, int card, event_t event){

	// Whenever ~ attacks, put a 2/2 |Sgreen Wolf creature token onto the battlefield tapped and attacking for each creature card in your graveyard.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		int amount = count_graveyard_by_type(player, TYPE_CREATURE);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WOLF, &token);
		token.action = TOKEN_ACTION_ATTACKING;
		token.qty = amount;
		generate_token(&token);
	}

	return 0;
}

int card_make_a_wish(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( count_graveyard(player) < 2 ){
			ai_modifier-=25;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int count_g = count_graveyard(player);
		int cards = 0;
		const int *grave = get_grave(player);
		while( cards < 2 && count_g > 0){
				int rnd = count_g-1;
				if( rnd > cards ){
					rnd = internal_rand(count_g);
				}
				add_card_to_hand(player, grave[rnd]);
				remove_card_from_grave(player, rnd);
				cards++;
				count_g--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_moldgraf_monstrosity(int player, int card, event_t event)
{
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  exile_from_owners_graveyard(player, card);
	  tutor_random_permanent_from_grave(player, card, player, TUTOR_PLAY, TYPE_CREATURE, 2, REANIMATE_DEFAULT);
	}

  return 0;
}

int card_parallel_lives(int player, int card, event_t event){
	return global_enchantment(player, card, event);
}

int card_revenant(int player, int card, event_t event);	// in stronghold.c

int card_splinterfright(int player, int card, event_t event){
	/* Splinterfright	|2|G
	 * Creature - Elemental 100/100
	 * Trample
	 * ~'s power and toughness are each equal to the number of creature cards in your graveyard.
	 * At the beginning of your upkeep, put the top two cards of your library into your graveyard. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		mill(player, 2);
	}
	return card_revenant(player, card, event);
}

static int exile_at_end_of_combat(int player, int card, event_t event)
{
  if (end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
		card_instance_t* instance = get_card_instance(player, card);
		if( ! check_state(instance->damage_target_player, instance->damage_target_card, STATE_OUBLIETTED) ||
			is_token(instance->damage_target_player, instance->damage_target_card)
		  ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

void saint_traft_ability(int player, int card, event_t event, int attacker_player, int attacker_card){
	/* Whenever (attacker_player, attacker_card) attacks, put a 4/4 |Swhite Angel creature token with flying
	 * onto the battlefield tapped and attacking. Exile that token at end of combat.*/

	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, attacker_player, attacker_card)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ANGEL, &token);
		token.pow = token.tou = 4;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		token.legacy = 1;
		token.special_code_for_legacy = &exile_at_end_of_combat;
		token.action = TOKEN_ACTION_ATTACKING;
		generate_token(&token);
	}
}

int card_geist_of_saint_traft(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	hexproof(player, card, event);

	saint_traft_ability(player, card, event, player, card);

	return 0;
}

int card_inquisitors_flail(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_dealt(event), *instance;
  if (damage
	  && is_equipping(player, card)
	  && (instance = get_card_instance(player, card))
	  && ((damage->damage_source_card == instance->targets[8].card && damage->damage_source_player == instance->targets[8].player)
		  || (damage->damage_target_card == instance->targets[8].card && damage->damage_target_player == instance->targets[8].player)))
	damage->info_slot *= 2;

  return basic_equipment(player, card, event, 2);
}

int card_gavony_township(int player, int card, event_t event){

	/* Gavony Township	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |2|G|W, |T: Put a +1/+1 counter on each creature you control. */

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_XGW(3, 1, 1), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Add +1/+1 counters\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_XGW(2, 1, 1)) ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			manipulate_type(instance->parent_controller, instance->parent_card, instance->parent_controller, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
		}
	}

	return 0;
}

int card_kessig_wolf_run(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_GR(1, 1), 0, &td1, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Pump a creature\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = instance->info_slot = 0;
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_GR(1, 1)) ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1){
					if( pick_target(&td1, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						instance->info_slot = 1+x_value;
						tap_card(player, card); // Now get the proper event sent
					}
					else{
						remove_state(player, card, STATE_TAPPED);
					}
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
		if (choice == 2){
			cancel = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			int amount = instance->info_slot-1;
			if( valid_target(&td1) ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										amount, 0, KEYWORD_TRAMPLE, 0);
			}
		}
	}

	return 0;
}

int card_tree_of_redemption(int player, int card, event_t event ){
	/* Tree of Redemption	|3|G
	 * Creature - Plant 0/13
	 * Defender
	 * |T: Exchange your life total with ~'s toughness. */

	card_instance_t *instance = get_card_instance(player, card);

	// Deliberately not !is_humiliated() - the exchange is unaffected even if this card later loses its abilities
	if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[1].card > -1 ){
		event_result = instance->targets[1].card;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* parent = in_play(instance->parent_controller, instance->parent_card);
		if( parent ){
			int tgh = get_toughness(instance->parent_controller, instance->parent_card);
			parent->targets[1].card = life[player];
			set_life_total(player, tgh);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_abattoir_ghoul(int player, int card, event_t event ){
	if_a_creature_damaged_by_me_dies_do_something(player, card, event, 1);
	return 0;
}

int card_ancient_grudge(int player, int card, event_t event){
	/* Ancient Grudge	|1|R
	 * Instant
	 * Destroy target artifact.
	 * Flashback |G */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_G(1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_angel_of_flight_alabaster(int player, int card, event_t event ){
	/* Angel of Flight Alabaster	|4|W
	 * Creature - Angel 4/4
	 * Flying
	 * At the beginning of your upkeep, return target Spirit card from your graveyard to your hand. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( any_in_graveyard_by_subtype(player, SUBTYPE_SPIRIT) && ! graveyard_has_shroud(player) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select target Spirt card.");
			this_test.subtype = SUBTYPE_SPIRIT;
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_angelic_overseer(int player, int card, event_t event ){

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_HUMAN) ){
			hexproof(player, card, event);
			indestructible(player, card, event);
		}
	}

	return 0;
}

int card_armored_skaab(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		mill(player, 4);
	}

	return 0;
}

int card_ashmouth_hound(int player, int card, event_t event ){

	if( ! is_humiliated(player, card) ){
		if( current_turn == player && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS ){
			int count = active_cards_count[1-player]-1;
			while( count > -1 ){
					if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
						card_instance_t *this = get_card_instance(1-player, count);
						if( this->blocking == card ){
							damage_creature(1-player, count, 1, player, card);
						}
					}
					count--;
			}
		}

		if( current_turn != player && event == EVENT_DECLARE_BLOCKERS ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->blocking < 255 ){
				damage_creature(1-player, instance->blocking, 1, player, card);
			}
		}
	}

	return 0;
}

int card_avacynian_priest(int player, int card, event_t event ){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_HUMAN;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED | GAA_LITERAL_PROMPT, MANACOST_X(1), 0,
									&td, "Select target non-Human creature.");
}

int card_bane_of_hanweir(int player, int card, event_t event){
	werewolf_moon_phases(player, card, event);
	attack_if_able(player, card, event);
	return 0;
}

int card_battleground_geist(int player, int card, event_t event){

	return boost_creature_type(player, card, event, SUBTYPE_SPIRIT, 1, 0, 0, BCT_CONTROLLER_ONLY);
}

int card_bitterheart_witch(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char msg[100] = "Select a Curse card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, msg);
		this_test.subtype = SUBTYPE_CURSE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return 0;
}

int card_bloodcrazed_neonate(int player, int card, event_t event){

	attack_if_able(player, card, event);

	return card_slith_predator(player, card, event);
}

int card_bonds_of_faith(int player, int card, event_t event){
	/* Bonds of Faith	|1|W
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +2/+2 as long as it's a Human. Otherwise, it can't attack or block. */

	if (event == EVENT_POWER || event == EVENT_TOUGHNESS){
		card_instance_t* instance = in_play(player, card);
		if(instance
		   && affect_me(instance->damage_target_player, instance->damage_target_card)
		   && affected_card_controller != -1
		   && has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN)
		   && !is_humiliated(player, card)){
			event_result += 2;
		}
	}

	if (event == EVENT_ABILITIES){
		card_instance_t* instance = in_play(player, card);
		if(instance
		   && affect_me(instance->damage_target_player, instance->damage_target_card)
		   && affected_card_controller != -1
		   && !has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN)
		   && !is_humiliated(player, card)){
			cannot_attack(affected_card_controller, affected_card, event);
			cannot_block(affected_card_controller, affected_card, event);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;
	td2.required_subtype = SUBTYPE_HUMAN;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);
	td3.required_subtype = SUBTYPE_HUMAN;
	td3.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	if( event == EVENT_CAN_CAST ){
		if( player == HUMAN ){
			return can_target(&td);
		}
		else{
			return can_target(&td2) || can_target(&td3);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( player != AI ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			if( can_target(&td3) ){
				ai_modifier+=15;
				pick_target(&td3, "TARGET_CREATURE");
			}
			else{
				ai_modifier+=10;
				pick_target(&td2, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL || event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA ){
		return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
	}

	return 0;
}

int card_brain_weevil(int player, int card, event_t event ){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_bump_in_the_night(int player, int card, event_t event ){
	/* Bump in the Night	|B
	 * Sorcery
	 * Target opponent loses 3 life.
	 * Flashback |5|R */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XR(3, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_burning_vengeance(int player, int card, event_t event){
	/*
	  Burning Vengeance |2|R
	  Enchantment
	  Whenever you cast a spell from your graveyard, Burning Vengeance deals 2 damage to target creature or player.
	*/
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( check_special_flags(trigger_cause_controller, trigger_cause, SF_PLAYED_FROM_GRAVE) &&
			get_owner(trigger_cause_controller, trigger_cause) == player
		  ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

			get_card_instance(player, card)->number_of_targets = 0;

			if( can_target(&td) && specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER")  ){
					damage_target0(player, card, 2);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}


int card_butchers_cleaver(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && is_equipping(player, card) ){
		card_instance_t* instance = in_play(player, card);
		if( instance && has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_HUMAN) ){
			lifelink(instance->targets[8].player, instance->targets[8].card, event);
		}
	}
	return vanilla_equipment(player, card, event, 3, 3, 0, 0, 0);
}

int card_cellar_door(int player, int card, event_t event)
{
  /* Cellar Door	|2
   * Artifact
   * |3, |T: Target player puts the bottom card of his or her library into his or her graveyard. If it's a creature card, you put a 2/2 |Sblack Zombie creature
   * token onto the battlefield. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  special_mill(instance->parent_controller, instance->parent_card, CARD_ID_CELLAR_DOOR, instance->targets[0].player, 1);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_PLAYER");
}

int card_civilized_scholar(int player, int card, event_t event ){

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);

		draw_cards(player, 1);
		if( hand_count[player] > 0 ){
			char msg[100] = "Select a card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			int selected = -1;
			if( player == AI ){
				this_test.type = TYPE_CREATURE;
				selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				if( selected == -1 ){
					this_test.type = TYPE_ANY;
					selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				}
			}
			else{
				selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
			}
			int flip = 0;
			if( is_what(player, selected, TYPE_CREATURE) ){
				flip = 1;
			}
			discard_card(player, selected);
			if( flip ){
				untap_card(instance->parent_controller, instance->parent_card);
				transform(instance->parent_controller, instance->parent_card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_claustrophobia(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event)  && instance->damage_target_player > -1){
		tap_card(instance->damage_target_player, instance->damage_target_card);
	}

	if( in_play(player, card) && instance->damage_target_card != -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
	}

	return disabling_aura(player, card, event);
}

int card_cobbled_wings(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 1, 0, 0, KEYWORD_FLYING, 0);
}

int card_corpse_lunge(int player, int card, event_t event ){
	/* Corpse Lunge	|2|B
	 * Instant
	 * As an additional cost to cast ~, exile a creature card from your graveyard.
	 * ~ deals damage equal to the exiled card's power to target creature. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return count_graveyard_by_type(player, TYPE_CREATURE) > 0;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		const int *grave = get_grave(player);
		int selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		instance->info_slot = get_base_power_iid(player, grave[selected]);
		if( pick_target(&td, "TARGET_CREATURE") ){
			rfg_card_from_grave(player, selected);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_creepy_doll(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	indestructible(player, card, event);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == player &&
				damage->damage_source_card == card && damage->info_slot > 0
			  ){
				if( instance->info_slot < 4 ){
					instance->info_slot = 4;
				}
				instance->targets[instance->info_slot].player = damage->damage_target_player;
				instance->targets[instance->info_slot].card = damage->damage_target_card;
				instance->info_slot++;
			}
		}
	}

	if( instance->info_slot > 4 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i+4].player != -1 && instance->targets[i+4].card != -1 &&
						in_play(instance->targets[i+4].player, instance->targets[i+4].card)
					  ){
						if (flip_a_coin(player, card)){
							kill_card(instance->targets[i+4].player, instance->targets[i+4].card, KILL_DESTROY);
						}
					}
				}
				instance->info_slot = 4;
		}
	}
	return 0;
}

int card_curse_of_deaths_hold(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){
		if( affected_card_controller == instance->targets[0].player ){
			modify_pt_and_abilities(affected_card_controller, affected_card, event, -1, -1, 0);
		}
	}
	return curse(player, card, event);
}

int card_curse_of_oblivion(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){

		upkeep_trigger_ability(player, card, event, instance->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int p = instance->targets[0].player;
			const int *grave = get_grave(p);
			int i = 0;
			while( grave[0] != -1 && i < 2 ){
					char msg[100] = "Select a card to exile.";
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, msg);
					new_global_tutor(p, p, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test);
					i++;
			}
		}
	}
	return curse(player, card, event);
}

int card_curse_of_stalked_prey(int player, int card, event_t event ){

	card_instance_t* instance;

	if( (instance = in_play(player, card)) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){
		int p = instance->targets[0].player;

		card_instance_t* damage = combat_damage_being_dealt(event);
		if (damage &&
			damage->damage_target_player == p &&
			damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
			instance->info_slot < 40	// targets[1] through targets[10], 8 bytes each; 2 bytes per creature
		   ){
			unsigned char* creatures = (unsigned char*)(&instance->targets[1].player);
			creatures[2 * instance->info_slot] = damage->damage_source_player;
			creatures[2 * instance->info_slot + 1] = damage->damage_source_card;
			instance->info_slot++;
		}

		if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player && instance->info_slot > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					unsigned char* creatures = (unsigned char*)(&instance->targets[1].player);
					int i;
					for (i = 0; i < instance->info_slot; i++){
						add_1_1_counter(creatures[2 * i], creatures[2 * i + 1]);
					}
					instance->info_slot = 0;
			}
		}
	}
	return curse(player, card, event);
}

int card_curse_of_the_bloody_tome(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){

		upkeep_trigger_ability(player, card, event, instance->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			mill(instance->targets[0].player, 2);
		}
	}
	return curse(player, card, event);
}

int card_curse_of_the_nightly_hunt(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){
		all_must_attack_if_able(instance->targets[0].player, event, -1);
	}

	return curse(player, card, event);
}

int card_curse_of_the_pierced_heart(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card) ){
		upkeep_trigger_ability(player, card, event, instance->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			damage_player(instance->targets[0].player, 1, player, card);
		}

	}

	return curse(player, card, event);
}

int card_darkthicket_wolf(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_XG(2, 1), 0, NULL, NULL);
}

int card_daybreak_ranger(int player, int card, event_t event){

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target creature with flying.");
}

int card_dead_weight(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -2, -2, 0, 0, 0, 0, 0);
}

int card_delver_of_secrets(int player, int card, event_t event){

	double_faced_card(player, card, event);

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int *deck = deck_ptr[player];
		int ai_choice = 1;
		if( ! is_what(-1, deck[0], TYPE_PERMANENT) ){
			ai_choice = 0;
		}
		if( player != AI ){
			show_deck( player, deck, 1, "Here's the first card of your deck", 0, 0x7375B0 );
			if( ai_choice == 0 ){
				int choice = do_dialog(player, player, card, -1, -1, " Reveal this card\n Pass", 0);
				if( choice == 0 ){
					transform(player, card);
				}
			}
		}
		else{
			 if( ai_choice == 0 ){
				show_deck( 1-player, deck, 1, "Here's the first card of opponent's deck", 0, 0x7375B0 );
				transform(player, card);
			}
		}
	}

	return 0;
}

int card_demonmail_hauberk(int player, int card, event_t event){
	return altar_equipment(player, card, event, TYPE_CREATURE, 4, 2, 0);
}

int card_desperate_ravings(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL){
		draw_cards(player, 2);
		discard(player, DISC_RANDOM, player);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XU(2, 1));
}

int card_dream_twist(int player, int card, event_t event){
	/* Dream Twist	|U
	 * Instant
	 * Target player puts the top three cards of his or her library into his or her graveyard.
	 * Flashback |1|U */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XU(1, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);


	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 3);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_elder_cathar(int player, int card, event_t event ){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_HUMAN) ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return 0;
}

int card_elite_inquisitor(int player, int card, event_t event ){

	if( in_play(player, card) ){
		vigilance(player, card, event);
		protection_from_subtype(player, card, event, SUBTYPE_WEREWOLF);
		protection_from_subtype(player, card, event, SUBTYPE_VAMPIRE);
		protection_from_subtype(player, card, event, SUBTYPE_ZOMBIE);
	}

	return 0;
}

int effect_evil_twin(int player, int card, event_t event ){
	/*
	  "|U|B, |T: Destroy target creature with the same name as this creature."
	*/
	if (effect_follows_control_of_attachment(player, card, event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if (!in_play(p, c)){
			return 0;
		}

		if( get_card_name(p, c) == -1 ){ //No point of continuing if Evil Twin is cloning a face-down card, which has no name.
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td1;
		default_target_definition(p, c, &td1, TYPE_ANY);
		td1.extra = get_internal_card_id_from_csv_id(get_card_name(p, c));

		if( event == EVENT_CAN_ACTIVATE ){
			return granted_generic_activated_ability(player, card, p, c, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_BU(1, 1), 0, &td1, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			instance->number_of_targets = 0;
			charge_mana_for_activated_ability(p, c, MANACOST_BU(1, 1));
			if( spell_fizzled != 1 ){
				char buffer[100];
				card_ptr_t* c1 = cards_ptr[cards_data[td1.extra].id];
				scnprintf(buffer, 100, "Select a card named %s.", c1->full_name);
				if( select_target(p, c-1000, &td1, buffer, &(instance->targets[0])) ){
					tap_card(p, c);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_evil_twin(int player, int card, event_t event)
{
  /* Evil Twin	|2|U|B
   * Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield except it gains
   "|U|B, |T: Destroy target creature with the same name as
   * this creature." */

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	  set_legacy_image(player, CARD_ID_EVIL_TWIN, create_targetted_legacy_activate(player, card, &effect_evil_twin, player, card));

  return 0;
}

static int falkenrath_noble_effect(int player, int card, event_t event){

	//In case something is copying Pawn of Ulamog and then reverts to its original form.
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_FALKENRATH_NOBLE ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( in_play(affected_card_controller, affected_card) ){
			int kill_code = get_card_instance(affected_card_controller, affected_card)->kill_code;
			if( kill_code ){
				if( instance->damage_target_player > -1 ){
					int p = instance->damage_target_player;
					int c = instance->damage_target_card;
					if( affect_me(p, c) ){
						//Will trigger also if this is not a creature when it goes into the graveyard;
						if( ! is_what(p, c, TYPE_CREATURE) ){
							count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);
						}
						instance->damage_target_player = instance->damage_target_card = -1;
						remove_status(player, card, STATUS_INVISIBLE_FX);
					}
				}
				count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
			}
		}
	}

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	//Reset counts if the legacy is still invisible
	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && event == EVENT_END_TRIGGER ){
		get_card_instance(player, card)->targets[11].player = 0;
	}

	return 0;
}

int card_falkenrath_noble(int player, int card, event_t event){

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)falkenrath_noble_effect ){
					if( (inst->damage_target_player == player && inst->damage_target_card == card) ||
						(inst->targets[0].player == player && inst->targets[0].card == card) )
					{
						found = 1;
						break;
					}
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &falkenrath_noble_effect, player, card);
			card_instance_t *inst = get_card_instance(player, legacy);
			inst->targets[0].player = player;
			inst->targets[0].card = card;
			inst->number_of_targets = 1;
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	}

	if( get_card_instance(player, card)->kill_code <= 0 ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
		}
	}

	return 0;
}

int card_feeling_of_dread(int player, int card, event_t event){
	/* Feeling of Dread	|1|W
	 * Instant
	 * Tap up to two target creatures.
	 * Flashback |1|U */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XU(1, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_feral_ridgewolf(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, MANACOST_XR(1, 1), 2, 0, 0, 0);
}

int card_fiend_hunter(int player, int card, event_t event){
	/*
	  Fiend Hunter English |1|W|W
	  Creature - Human Cleric 1/3
	  When Fiend Hunter enters the battlefield, you may exile another target creature.
	  When Fiend Hunter leaves the battlefield, return the exiled card to the battlefield under its owner's control.
	*/
	return_from_oblivion(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.special = TARGET_SPECIAL_NOT_ME;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_frightful_delusion(int player, int card, event_t event){
	/* Frightful Delusion	|2|U
	 * Instant
	 * Counter target spell unless its controller pays |1. That player discards a card. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1)){
			discard(instance->targets[0].player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_furor_of_the_bitten(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_card != -1 ){
		attack_if_able(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_gallows_warden(int player, int card, event_t event){

	return boost_creature_type(player, card, event, SUBTYPE_SPIRIT, 0, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_galvanic_juggernaut(int player, int card, event_t event){

	does_not_untap(player, card, event);

	attack_if_able(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, 0);
	}

	if( is_tapped(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		untap_card(player, card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_gastaf_howler(int player, int card, event_t event){
	werewolf_moon_phases(player, card, event);
	intimidate(player, card, event);
	return 0;
}

int card_gastaf_shepherd(int player, int card, event_t event){
	double_faced_card(player, card, event);
	human_moon_phases(player, card, event);
	return 0;
}

int card_geistcatchers_rig(int player, int card, event_t event){

	// When ~ enters the battlefield, you may have it deal 4 damage to target creature with flying.
	if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_abilities = KEYWORD_FLYING;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			card_instance_t* instance = get_card_instance(player, card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
		}
	}

	return 0;
}

int card_geistflame(int player, int card, event_t event){
	/* Geistflame	|R
	 * Instant
	 * ~ deals 1 damage to target creature or player.
	 * Flashback |3|R */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XR(3, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_ghostly_possession(int player, int card, event_t event){

	card_instance_t* instance;
	card_instance_t* damage = combat_damage_being_prevented(event);

	if( damage &&
		(instance = in_play(player, card)) && instance->damage_target_card != -1 && ! is_humiliated(player, card) &&
		damage->damage_target_player == instance->damage_target_player && damage->damage_target_card == instance->damage_target_card
	  ){
		damage->info_slot = 0;
	}

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_ghoulcallers_bell(int player, int card, event_t event ){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		mill(player, 1);
		mill(1-player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_ghoulcallers_chant(int player, int card, event_t event)
{
  /* Ghoulcaller's Chant	|B
   * Sorcery
   * Choose one - Return target creature card from your graveyard to your hand; or return two target Zombie cards from your graveyard to your hand. */

	if (!IS_GS_EVENT(player, card, event))
		return 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	enum{
		CHOICE_CREATURE = 1,
		CHOICE_ZOMBIES = 2,
	};

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_AUTOCHOOSE_IF_1,
							"Raise a creature", 1, 2,
							"Raise two Zombies", count_graveyard_by_subtype(player, SUBTYPE_ZOMBIE) >= 2, 3);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}

		switch (choice){
			case CHOICE_CREATURE:
				if (new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1)
					cancel = 1;
				else
					instance->targets[1].player = instance->targets[1].card = -1;
				break;

			case CHOICE_ZOMBIES:
			{
				test_definition_t this_test2;
				new_default_test_definition(&this_test2, TYPE_CREATURE, "Select a Zombie card.");
				this_test2.subtype = SUBTYPE_ZOMBIE;
				select_multiple_cards_from_graveyard(player, player, -1, AI_MAX_VALUE, &this_test2, 2, &instance->targets[0]);
			}
			break;
		}
		instance->info_slot = choice;
	}

	if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0, selected;
		for (i = 0; i < instance->info_slot; ++i){	// choice conveniently also matches the number of cards to raise
			if ((selected = validate_target_from_grave(player, card, player, i)) != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
				++num_validated;
			}
		}

		if (num_validated == 0)
			spell_fizzled = 1;

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ghoulraiser(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		const int *grave = get_grave(player);
		if( grave[0] != -1 ){
			int zombie_array[60];
			int i;
			int zombie_count = 0;
			for(i=0; i<count_graveyard(player); i++){
				if( has_subtype_by_id(cards_data[grave[i]].id, SUBTYPE_ZOMBIE) ){
					zombie_array[zombie_count] = i;
					zombie_count++;
				}
			}
			if( zombie_count > 0 ){
				int random_zombie = 0;
				if( zombie_count > 0 ){
					random_zombie = internal_rand(zombie_count);
				}
				add_card_to_hand(player, grave[zombie_array[random_zombie]]);
				remove_card_from_grave(player, zombie_array[random_zombie]);
			}
		}
	}
	return 0;
}

int card_gnaw_to_the_bone(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL){
		gain_life(player, 2*count_graveyard_by_type(player, TYPE_CREATURE));
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XG(2, 1));
}

int card_grasp_of_phantoms(int player, int card, event_t event){
	/* Grasp of Phantoms	|3|U
	 * Sorcery
	 * Put target creature on top of its owner's library.
	 * Flashback |7|U */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XU(7, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_grave_bramble(int player, int card, event_t event ){

	if( in_play(player, card) ){
		protection_from_subtype(player, card, event, SUBTYPE_ZOMBIE);
	}

	return 0;
}

int card_graveyard_shovel(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			const int *grave = get_grave(player);
			if( grave[0] != -1 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile");
				int selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_GRAVE, 1, AI_MIN_VALUE, -1, &this_test);
				if( is_what(-1, grave[selected], TYPE_CREATURE) ){
					gain_life(player, 2);
				}
				rfg_card_from_grave(player, selected);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_PLAYER");
}

int card_gruesome_deformity(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_card != -1 ){
		intimidate(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_hanweir_watchkeep(int player, int card, event_t event){
	double_faced_card(player, card, event);
	human_moon_phases(player, card, event);
	return 0;
}

int card_harvest_pyre(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int cg = count_graveyard(player);
		if( ! cg ){
			ai_modifier-=25;
		}
		instance->number_of_targets = instance->info_slot = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			while( cg && (player == HUMAN || (player == AI && instance->info_slot < get_toughness(instance->targets[0].player, instance->targets[0].card))) ){
					if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						instance->info_slot++;
						cg--;
					}
					else{
						break;
					}
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_heretics_punishment(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			special_mill(player, card, player, CARD_ID_HERETICS_PUNISHMENT, 3);
			if( instance->info_slot ){
				damage_target0(player, card, instance->info_slot);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(3, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_hollowhenge_scavenger(int player, int card, event_t event){

	/* Hollowhenge Scavenger	|3|G|G
	 * Creature - Elemental 4/5
	 * Morbid - When ~ enters the battlefield, if a creature died this turn, you gain 5 life. */

	if( morbid() && comes_into_play(player, card, event) ){
		gain_life(player, 5);
	}

	return 0;
}

int card_homicidal_brute(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( ! is_humiliated(player, card) && current_turn == player && !(instance->state & STATE_ATTACKED) && eot_trigger(player, card, event) ){
		tap_card(player, card);
		transform(player, card);
	}

	return 0;
}

int card_howlpack_of_estwald(int player, int card, event_t event){
	werewolf_moon_phases(player, card, event);
	return 0;
}

int card_hysterical_blindness(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 1-player, -1, -4, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_infernal_plunge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 0, NULL);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! controller_sacrifices_a_permanent(player, card, TYPE_CREATURE , 0) ){
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			produce_mana(player, COLOR_RED, 3);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_into_the_maw_of_hell(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_LAND") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( validate_target(player, card, &td1, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 13, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_invisible_stalker(int player, int card, event_t event){

	unblockable(player, card, event);
	hexproof(player, card, event);

	return 0;
}

int card_kessig_wolf2(int player, int card, event_t event){

	return generic_shade(player, card, event,  0, MANACOST_XR(1, 1),0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_laboratory_maniac(int player, int card, event_t event){

	/* Laboratory Maniac	|2|U
	 * Creature - Human Wizard 2/2
	 * If you would draw a card while your library has no cards in it, you win the game instead. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) &&
		reason_for_trigger_controller == player && !suppress_draw && ! is_humiliated(player, card)
	  ){
		int trig = 0;
		int * deck = deck_ptr[player];
		if( deck[0] == -1 ){
			trig = 1;
		}
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					lose_the_game(1-player);
					suppress_draw = 1;
			}
		}
	}
	return 0;
}

int card_ludevics_test_subject(int player, int card, event_t event){

	/* Ludevic's Test Subject	|1|U
	 * Creature - Lizard 0/3
	 * Defender
	 * |1|U: Put a hatchling counter on ~. Then if there are five or more hatchling counters on it, remove all of them and transform it. */

	double_faced_card(player, card, event);

	if (event == EVENT_SHOULD_AI_PLAY){
		int counters = count_counters(player, card, COUNTER_HATCHLING);
		counters = MIN(counters, 4);
		ai_modifier += (player == AI ? 8 : -8 ) * counters * counters;	// 1 counter:8 2:32 3:72 4+:128
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_counter(player, card, COUNTER_HATCHLING);
		if( count_counters(player, card, COUNTER_HATCHLING) >= 5 ){
			remove_all_counters(player, card, COUNTER_HATCHLING);
			card_instance_t* instance = get_card_instance(player, card);
			transform(instance->parent_controller, instance->parent_card);
		}
		else if (!(current_phase == PHASE_DISCARD && current_turn == 1-player)){
			ai_modifier += (player == AI ? -64 : +64);	// large enough to normally not activate at other times, but small enough that it'll activate immediately if it can transform
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XU(1, 1), 0, NULL, NULL);
}

int card_lumberknot(int player, int card, event_t event ){

	hexproof(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_makeshift_mauler(int player, int card, event_t event ){

	skaabs(player, card, event, 1);

	return 0;
}

int card_manor_gargoyle(int player, int card, event_t event)
{
  card_instance_t *instance = get_card_instance(player, card);
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, effect_lose_defender_gain_flying_until_eot,
									 instance->parent_controller, instance->parent_card);
	}

  if (instance->regen_status & KEYWORD_DEFENDER)
	indestructible(player, card, event);

  return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
}

int card_manor_skeleton(int player, int card, event_t event ){

	haste(player, card, event);

	return regeneration(player, card, event, MANACOST_XB(1, 1));
}

int card_mask_of_avacyn(int player, int card, event_t event){
	// Equipped creature gets +1/+2 and has hexproof.
	// Equip |3
	return vanilla_equipment(player, card, event, 3, 1, 2, 0,SP_KEYWORD_HEXPROOF);
}

int card_maw_of_the_mire(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_memorys_journey(int player, int card, event_t event)
{
  /* Memory's Journey	|1|U
   * Instant
   * Target player shuffles up to three target cards from his or her graveyard into his or her library.
   * Flashback |G */

  if (IS_CASTING(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;

	  card_instance_t* instance = get_card_instance(player, card);

	  if (event == EVENT_CAN_CAST)
		return can_target(&td);

	  if (event == EVENT_CAST_SPELL && affect_me(player, card)
		  && pick_target(&td, "TARGET_PLAYER"))
		select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MAX_VALUE : AI_MIN_VALUE, NULL, 3, &instance->targets[1]);

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  int i, any_shuffled_in = 0, num_targets = 0;
		  for (i = 1; i <= 3; ++i)
			if (instance->targets[i].player != -1)
			  {
				++num_targets;
				int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
				if (selected != -1)
				  {
					from_graveyard_to_deck(instance->targets[0].player, selected, 2);
					any_shuffled_in = 1;
				  }
			  }

		  if (any_shuffled_in)
			shuffle(instance->targets[0].player);

		  if (!any_shuffled_in		// at least one card was targeted but none were still legal
			  && !valid_target(&td))	// the targeted player isn't still legal
			spell_fizzled = 1;

		  kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
		}
	}

  return do_flashback(player, card, event, MANACOST_G(1));
}

int card_mirror_mad_phantasm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( player == AI && in_play(instance->parent_controller, instance->parent_card)){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		int owner = get_owner(instance->parent_controller, instance->parent_card);
		shuffle_into_library(instance->parent_controller, instance->parent_card);
		int *deck = deck_ptr[owner];
		int count = 0;
		while( deck[count] != -1 ){
				if( cards_data[deck[count]].id == CARD_ID_MIRROR_MAD_PHANTASM ){
					break;
				}
				count++;
		}
		if( deck[count] != -1 ){
			int card_added = add_card_to_hand(owner, deck[count]);
			remove_card_from_deck(owner, count);
			mill(owner, count);
			put_into_play(owner, card_added);
		}
	}

	return generic_activated_ability(player, card, event, player == AI ? GAA_REGENERATION : 0, MANACOST_XU(1, 1), 0, NULL, NULL);
}

int card_moment_of_heroism(int player, int card, event_t event){
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, 0, SP_KEYWORD_LIFELINK);
}

int card_moorland_haunt(int player, int card, event_t event){
	/* Moorland Haunt	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |W|U, |T, Exile a creature card from your graveyard: Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_UW(1, 1), 0, NULL, NULL) &&
			count_graveyard_by_type(player, TYPE_CREATURE) > 0
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Generate a Spirit\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_UW(1, 1)) ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");
					if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						tap_card(player, card);
						instance->info_slot = 1;
					}
				}
				else{
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.color_forced = COLOR_TEST_WHITE;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
	}

	return 0;
}

int card_morkrut_banshee(int player, int card, event_t event){

	/* Morkrut Banshee	|3|B|B
	 * Creature - Spirit 4/4
	 * Morbid - When ~ enters the battlefield, if a creature died this turn, target creature gets -4/-4 until end of turn. */

	if( morbid() && comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
			if (player == AI && instance->targets[0].player == AI){
				ai_modifier -= 500;
			}
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, -4);
		}
	}

	return 0;
}

int card_mulch(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL){
		int amount = MIN(4, count_deck(player));
		int *deck = deck_ptr[player];
		show_deck( player, deck, amount, "Mulch revealed these cards.", 0, 0x7375B0 );
		int i;
		for(i=0; i<amount; i++){
			if( is_what(-1, deck[0], TYPE_LAND) ){
				add_card_to_hand(player, deck[0]);
				remove_card_from_deck(player, 0);
			}
			else{
				mill(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_murder_of_crows(int player, int card, event_t event ){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int i;
		for(i=0; i<instance->targets[11].card; i++){
			if( draw_some_cards_if_you_want(player, card, player, 1) ){
				discard(player, 0, player);
			}
		}
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_nephalia_drownyard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XBU(2, 1, 1), 0, &td1, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Mill a player\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_XBU(1, 1, 1)) && pick_target(&td1, "TARGET_PLAYER") ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}
		else{
			 spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td1) ){
				mill(instance->targets[0].player, 3);
			}
		}
	}

	return 0;
}

int card_night_revelers(int player, int card, event_t event ){

	card_instance_t *instance= get_card_instance(player, card);

	if( instance->state & STATE_SUMMON_SICK ){
		if( check_battlefield_for_subtype(1-player, TYPE_PERMANENT, SUBTYPE_HUMAN) ){
			instance->state &= ~STATE_SUMMON_SICK;
		}
	}

	return 0;
}

int card_night_terrors(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
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

int card_nightbirds_clutches(int player, int card, event_t event){
	/* Nightbird's Clutches	|1|R
	 * Sorcery
	 * Up to two target creatures can't block this turn.
	 * Flashback |3|R */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XR(3, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card,
										0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			}
		}
		kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_nightfall_predator(int player, int card, event_t event){
	/* Nightfall Predator	""
	 * Creature - Werewolf 4/4
	 * |R, |T: ~ fights target creature.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~.
	 * (Daybreak Ranger - 2/2) |T: Daybreak Ranger deals 2 damage to target creature with flying.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform Daybreak Ranger. */

	werewolf_moon_phases(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			fight(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE");
}

int card_one_eyed_scarecrow(int player, int card, event_t event){

	if( affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		! is_humiliated(player, card) && check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
	  ){
		modify_pt_and_abilities(affected_card_controller, affected_card, event, -1, 0, 0);
	}

	return 0;
}

int card_orchard_spirit(int player, int card, event_t event ){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card && ! is_humiliated(player, card) ){
			int abilities = get_abilities(affected_card_controller, affected_card, EVENT_ABILITIES, -1);
			if( !((abilities & KEYWORD_FLYING) || (abilities & KEYWORD_REACH)) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_paraselne(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		if( ! check_battlefield_for_id(2, CARD_ID_ENCHANTED_EVENING) ){
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
		}
		else{
			default_test_definition(&this_test, TYPE_PERMANENT);
		}
		int result = new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		gain_life(player, result);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pitchburn_devils(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 0, 0) ){
			damage_target_creature_or_player(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
	}

	return 0;
}

int card_prey_upon(int player, int card, event_t event){
	/* Prey Upon	|G
	 * Sorcery
	 * Target creature you control fights target creature you don't control. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "Select target creature opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_purify_the_grave(int player, int card, event_t event){
	/* Purify the Grave	|W
	 * Instant
	 * Exile target card from a graveyard.
	 * Flashback |W */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_W(1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			if( count_graveyard(player) && ! graveyard_has_shroud(player) ){
				return 1;
			}
			if( count_graveyard(1-player) && ! graveyard_has_shroud(1-player) ){
				return 1;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
		if( select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &this_test, 0, 1) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
		}
		kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
	}

	return 0;
}

static int rage_thrower_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->damage_target_player;
	int c = instance->damage_target_card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			instance->damage_target_player = instance->damage_target_card = -1;
			instance->targets[12].player = get_protections_from(p, c);
			remove_status(player, card, STATUS_INVISIBLE_FX);
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
		}
	}

	if(	! check_status(player, card, STATUS_INVISIBLE_FX) && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		if( p == -1 && instance->targets[12].player > -1 ){
			td.illegal_abilities = instance->targets[12].player;
		}
		if(	resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			int sd_player = p;
			int sd_card = c;
			if( sd_player == -1 ){
				sd_player = player;
				sd_card = card;
			}
			instance->number_of_targets = 0;
			int i;
			for(i=0; i<instance->targets[11].card; i++){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_PLAYER");
				damage_player(instance->targets[0].player, 2, sd_player, sd_card);
			}
			instance->targets[11].card = 0;
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_rage_thrower(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
		//Create the legacies here, in case something already in play is transformed into a Rage Thrower.
		// if Rage Thrower dies along with other creatures, the legacy will take card of the ability.
		int i;
		int found = 0;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_EFFECT) ){
				if( get_card_instance(player, i)->display_pic_csv_id == CARD_ID_RAGE_THROWER ){
					if( get_card_instance(player, i)->damage_target_player == player &&
						get_card_instance(player, i)->damage_target_card == card
					  ){
						found = 1;
						break;
					}
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &rage_thrower_legacy, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if(	trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		if(	resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			int i;
			for(i=0; i<instance->targets[11].card; i++){
				pick_target(&td, "TARGET_PLAYER");
				instance->number_of_targets = 1;
				damage_player(instance->targets[0].player, 2, player, card);
			}
			instance->targets[11].card = 0;
		}
	}

	return 0;
}

int card_rally_the_peasants(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 2, 0, 0, 0);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			 kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XR(2, 1));
}

int card_rangers_guile(int player, int card, event_t event){
	/* Ranger's Guile	|G
	 * Instant
	 * Target creature you control gets +1/+1 and gains hexproof until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 1, 0, SP_KEYWORD_HEXPROOF);
}

int card_rebuke(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature", 1, NULL);
}

int card_rolling_temblor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = DOESNT_MATCH;
		APNAP(p, {new_damage_all(player, card, p, 2, 0, &this_test);});
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XR(4, 2));
}

int card_runechanters_pike(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && is_equipping(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			int amount = special_count_grave(player, TYPE_PERMANENT, 1, 0, 0, 0, 0, 0, 0, -1, 0);
			event_result+=amount;
		}
		if( event == EVENT_ABILITIES && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result |= KEYWORD_FIRST_STRIKE;
		}
	}

	return basic_equipment(player, card, event, 2);
}

static int iid_has_flashback(int iid, int me, int player, int card){
	if( is_what(-1, iid, TYPE_SPELL) && cards_data[iid].cc[2] == 9 ){
		return 1;
	}
	return 0;
}

int card_runic_repetition(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int *rfg =rfg_ptr[player];
		int count = count_rfg(player);
		while( count > -1 ){
				if( ! is_what(-1, rfg[count], TYPE_PERMANENT) && cards_data[rfg[count]].cc[2] == 9 ){
					return 1;
				}
				count--;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select a card with Flashback.");
		this_test.special_selection_function = iid_has_flashback;
		int selected = new_select_a_card(player, player, TUTOR_FROM_RFG, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			instance->targets[0].player = selected;
			instance->targets[0].card = rfg_ptr[player][selected];
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( rfg_ptr[player][instance->targets[0].player] == instance->targets[0].card ){
			add_card_to_hand(player, instance->targets[0].card);
			remove_card_from_rfg(player, cards_data[instance->targets[0].card].id);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_scourge_of_geier_reach(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += count_subtype(1-player, TYPE_CREATURE, -1);
	}

	return 0;
}

int card_screeching_bat(int player, int card, event_t event){

	double_faced_card(player, card, event);

	upkeep_trigger_ability_mode(player, card, event, player, has_mana_multi(player, MANACOST_XB(2, 2)) ? RESOLVE_TRIGGER_AI(player) : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		charge_mana_multi(player, MANACOST_XB(2, 2));
		if( spell_fizzled != 1 ){
			transform(player, card);
		}
	}

	return 0;
}

int card_selhoff_occultist(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			mill(instance->targets[0].player, 1);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if(	trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		if(	resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			int i;
			for(i=0; i<instance->targets[11].card; i++){
				pick_target(&td, "TARGET_PLAYER");
				instance->number_of_targets = 1;
				mill(instance->targets[0].player, 1);
			}
			instance->targets[11].card = 0;
		}
	}

	return 0;
}

int card_card_sensory_deprivation(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -3, 0, 0, 0, 0, 0, 0);
}

int card_sever_the_bloodline(int player, int card, event_t event){
	/* Sever the Bloodline	|3|B
	 * Sorcery
	 * Exile target creature and all other creatures with the same name as that creature.
	 * Flashback |5|B|B */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XB(5, 2));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			echoing_effect(player, card, instance->targets[0].player, instance->targets[0].card, &td, KILL_REMOVE);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_sharpened_pitchfork(int player, int card, event_t event){

	if( is_equipping(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_HUMAN) ){
			modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 1, 1, 0);
		}
	}
	return vanilla_equipment(player, card, event, 1, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_silent_departure(int player, int card, event_t event){
	/* Silent Departure	|U
	 * Sorcery
	 * Return target creature to its owner's hand.
	 * Flashback |4|U */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XU(4, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
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

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_silver_inlaid_dagger(int player, int card, event_t event){

	int amount = 2;
	if( is_equipping(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_HUMAN) ){
			amount = 3;
		}
	}
	return vanilla_equipment(player, card, event, 2, amount, 0, 0, 0);
}

int card_skeletal_grimace(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	int p = instance->damage_target_player;
	int c = instance->damage_target_card;

	if( in_play(player, card) && c != -1 ){
		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return regeneration(p, c, event, MANACOST_B(1));
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( can_be_regenerated(p, c) ){
				regenerate_target(p,c);
			}
		}
	}

	return generic_aura(player, card, event, player, 1, 1, 0, 0, 0, 0, 0);
}

int card_skirsdag_cultist(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_R(1), 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

static const char* target_is_vampire_werewolf_or_zombie(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (has_subtype(player, card, SUBTYPE_VAMPIRE) || has_subtype(player, card, SUBTYPE_WEREWOLF) || has_subtype(player, card, SUBTYPE_ZOMBIE)
		  ? NULL : EXE_STR(0x73964C));	//",subtype"
}
int card_slayer_of_the_wicked(int player, int card, event_t event)
{
  /* Slayer of the Wicked	|3|W
   * Creature - Human Soldier 3/2
   * When ~ enters the battlefield, you may destroy target Vampire, Werewolf, or Zombie. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_is_vampire_werewolf_or_zombie;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target Vampire, Werewolf, or Zombie."))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return 0;
}

int card_smite_the_monstrous(int player, int card, event_t event){

	/* Smite the Monstrous	|3|W
	 * Instant
	 * Destroy target creature with power 4 or greater. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td1, "Select target creature with power 4 or greater.", 1, NULL);
}

static int spare_from_evil_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if(event == EVENT_BLOCK_LEGALITY ){
		if( p == attacking_card_controller && c == attacking_card ){
			if( ! has_creature_type(affected_card_controller, affected_card, SUBTYPE_HUMAN) ){
				event_result = 1;
			}
		}
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 ){
			return 0;
		}

		if( source->damage_target_player == p && source->damage_target_card == c ){
			if( in_play(source->damage_source_player, source->damage_source_card) &&
				is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE) &&
				! has_subtype(source->damage_source_player, source->damage_source_card, SUBTYPE_HUMAN)
			  ){
				source->info_slot = 0;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}


int card_spare_from_evil(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					create_targetted_legacy_effect(player, card, &spare_from_evil_legacy, player, count);
				}
				count--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_spectral_flight(int player, int card, event_t event){

	return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_spectral_rider(int player, int card, event_t event){

	intimidate(player, card, event);

	return 0;
}

int card_spiders_spawning(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier -= (count_graveyard_by_type(player, TYPE_CREATURE) < 2 ? 15 : 0);
	}

	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIDER, &token);
		token.pow = 1;
		token.tou = 2;
		token.qty = count_graveyard_by_type(player, TYPE_CREATURE);
		generate_token(&token);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XB(6, 1));
}

int card_spidery_grasp(int player, int card, event_t event){

	if(event == EVENT_CHECK_PUMP ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 4, 0, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									2, 4, KEYWORD_REACH, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_stalking_vampire(int player, int card, event_t event){

	double_faced_card(player, card, event);

	upkeep_trigger_ability_mode(player, card, event, player, has_mana_multi(player, MANACOST_XB(2, 2)) ? RESOLVE_TRIGGER_AI(player) : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		charge_mana_multi(player, MANACOST_XB(2, 2));
		if( spell_fizzled != 1 ){
			transform(player, card);
		}
	}

	return 0;
}

int card_stensia_bloodhall(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_XBR(3, 1, 1), 0, &td1, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Damage player\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = 0;
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_XBR(3, 1, 1)) && pick_target(&td1, "TARGET_PLAYER")){
				instance->info_slot = 1;
				tap_card(player, card);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			damage_player(instance->targets[0].player, 2, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_stony_silence(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
		disable_all_activated_abilities(affected_card_controller, affected_card, 1);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
	}

	return global_enchantment(player, card, event);
}

int card_stromkirk_noble(int player, int card, event_t event ){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card && ! is_humiliated(player, card) ){
			if( has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN) ){
				event_result = 1;
			}
		}
	}

	return card_slith_predator(player, card, event);
}

int card_sturmgeist(int player, int card, event_t event ){
	/* Sturmgeist	|3|U|U
	 * Creature - Spirit 100/100
	 * Flying
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * Whenever ~ deals combat damage to a player, draw a card. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_terror_of_kruin_pass(int player, int card, event_t event){

	/* Terror of Kruin Pass	""
	 * Creature - Werewolf 3/3
	 * Double strike
	 * Werewolves you control have menace.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if (event == EVENT_DECLARE_BLOCKERS && current_turn == player && ! is_humiliated(player, card) ){
		int c;
		for (c = 0; c < active_cards_count[player]; ++c){
			if (in_play(player, c) && has_subtype(player, c, SUBTYPE_WEREWOLF)){
				menace(player, c, event);
			}
		}
	}

	return 0;
}

int card_thraben_sentry(int player, int card, event_t event ){

	double_faced_card(player, card, event);

	vigilance(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, 0);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		transform(player, card);
	}

	return 0;
}

int card_traitorous_blood(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		if( valid_target(&td) ){
			effect_act_of_treason_and_modify_pt_or_abilities(player, card, instance->targets[0].player, instance->targets[0].card,
															0, 0, KEYWORD_TRAMPLE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_travel_preparations(int player, int card, event_t event){
	/* Travel Preparations	|1|G
	 * Sorcery
	 * Put a +1/+1 counter on each of up to two target creatures.
	 * Flashback |1|W */

	if( event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_XW(1, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
			}
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_travelers_amulet(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_land(player, 0, 0);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(1), 0, NULL, NULL);
}

int card_trepanation_blade(int player, int card, event_t event)
{
	/* Whenever equipped creature attacks, defending player reveals cards from the top of his or her library until he or she reveals a land card. The creature
	 * gets +1/+0 until end of turn for each card revealed this way. That player puts the revealed cards into his or her graveyard. */
	card_instance_t *instance = get_card_instance(player, card);
	if (declare_attackers_trigger(player, card, event, DAT_TRACK, instance->targets[8].player, instance->targets[8].card)){
		int pump = 0;
		int *deck = deck_ptr[1-player];
		while (deck[pump] != -1 && !is_what(-1, deck[pump], TYPE_LAND)){
			pump++;
		}
		if (deck[pump] != -1){	// still a card, so it must be the revealed land
			pump++;
		}
		if (pump > 0){
			show_deck(player, deck, pump, "Cards revealed with Trepan. Blade", 0, 0x7375B0);
			if (in_play(current_turn, BYTE0(instance->targets[2].player))){
				pump_until_eot(player, card, current_turn, BYTE0(instance->targets[2].player), pump, 0);
			}
			mill(1-player, pump);
		}
	}
	return basic_equipment(player, card, event, 2);
}

int card_tribute_to_hunger(int player, int card, event_t event ){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature to sacrifice");
			int result = sacrifice_and_report_value(player, card, instance->targets[0].player, SARV_REPORT_TOUGHNESS | SARV_EXTRA_IMPOSE_SACRIFICE, &this_test);
			if( result > -1 ){
				gain_life(player, result);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_ulvenwald_primordials(int player, int card, event_t event){
	werewolf_moon_phases(player, card, event);
	return regeneration(player, card, event, MANACOST_G(1));
}

int card_unruly_mob(int player, int card, event_t event ){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}
	return 0;
}

static const char* is_enchantment_or_spirit_permanent(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	if( is_what(player, card, TYPE_ENCHANTMENT) || has_subtype(player, card, SUBTYPE_SPIRIT) ){
		return NULL;
	}
	return EXE_STR(0x73964C);//",subtype"
}

int card_urgent_exorcism(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_enchantment_or_spirit_permanent;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target enchantment or Spirit permanent.", 1, NULL);
}

int card_vampire_interloper(int player, int card, event_t event){
	cannot_block(player, card, event);
	return 0;
}

static const char* target_is_not_vampire_werewolf_or_zombie(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (has_subtype(player, card, SUBTYPE_VAMPIRE) || has_subtype(player, card, SUBTYPE_WEREWOLF) || has_subtype(player, card, SUBTYPE_ZOMBIE)
		  ? EXE_STR(0x73964C) : NULL);	//",subtype"
}
int card_victim_of_night(int player, int card, event_t event)
{
  /* Victim of Night	|B|B
   * Instant
   * Destroy target non-Vampire, non-Werewolf, non-Zombie creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_is_not_vampire_werewolf_or_zombie;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target non-Vampire, non-Werewolf, non-Zombie creature.", 1, NULL);
}

int card_village_bell_ringer(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	return flash(player, card, event);
}

int card_village_cannibals(int player, int card, event_t event ){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_HUMAN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}
	return 0;
}

int card_witchbane_orb(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_CURSE;
		new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
	}

	give_hexproof_to_player(player, card, event);

	return 0;
}

static void bury_vampire(int player, int card, int t_player, int t_card)
{
  kill_card(t_player, t_card, KILL_BURY);
}

int card_wooden_stake(int player, int card, event_t event)
{
  if ((event == EVENT_CHANGE_TYPE || event == EVENT_CHECK_DESTROY_IF_BLOCKED || event == EVENT_DECLARE_BLOCKERS)
	  && is_equipping(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->targets[8].player;
	  int c = instance->targets[8].card;

	  if (event == EVENT_CHANGE_TYPE && affect_me(p, c) && !is_humiliated(player, card))	// The equipment hasn't lost abilities, not the equipped creature
		instance->destroys_if_blocked |= DIFB_ASK_CARD;

	  if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(p, c) && !is_humiliated(player, card)
		  && has_subtype(attacking_card_controller, attacking_card, SUBTYPE_VAMPIRE))
		event_result |= 1;

	  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
		{
		  card_instance_t* equipped = get_card_instance(p, c);
		  if (p == current_turn && (equipped->state & STATE_ATTACKING))
			for_each_creature_blocking_me(p, c, bury_vampire, player, card);

		  if (p == 1-current_turn && (equipped->state & STATE_BLOCKING))
			for_each_creature_blocked_by_me(p, c, bury_vampire, player, card);
		}
	}

	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_woodland_sleuth(int player, int card, event_t event){

	/* Woodland Sleuth	|3|G
	 * Creature - Human Scout 2/3
	 * Morbid - When ~ enters the battlefield, if a creature died this turn, return a creature card at random from your graveyard to your hand. */

	if( morbid() && comes_into_play(player, card, event) ){
		tutor_random_permanent_from_grave(player, card, player, TUTOR_HAND, TYPE_CREATURE, 1, REANIMATE_DEFAULT);
	}

	return 0;
}

int card_wreath_of_geists(int player, int card, event_t event){
	return generic_aura(player, card, event, player, count_graveyard_by_type(player, TYPE_CREATURE), count_graveyard_by_type(player, TYPE_CREATURE), 0, 0, 0, 0, 0);
}

int card_lantern_spirit(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		bounce_permanent(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
}

int card_nevermore(int player, int card, event_t event ){

	meddling_mage_effect(player, card, event);

	return global_enchantment(player, card, event);
}
