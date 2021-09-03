#include "manalink.h"

// global functions
static int join_forces(int player, int prevent_if_duh){
	charge_mana(player, COLOR_COLORLESS, -1);
	int amount = x_value;
	if (!(prevent_if_duh && duh_mode(1-player))){
		charge_mana(1-player, COLOR_COLORLESS, -1);
		amount += x_value;
	}
	return amount;
}

static int vow(int player, int card, event_t event, int pow, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( instance->damage_target_player != player ){
			cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
		}
	}
	return generic_aura(player, card, event, player, pow, tou, key, s_key, 0, 0, 0);
}

// cards
int card_acorn_catapult(int player, int card, event_t event){
	/* Acorn Catapult	|4
	 * Artifact
	 * |1, |T: ~ deals 1 damage to target creature or player. That creature's controller or that player puts a 1/1 |Sgreen Squirrel creature token onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SQUIRREL, &token);
			token.t_player = instance->targets[0].player;
			generate_token(&token);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_alliance_of_arms(int player, int card, event_t event){
	/* Alliance of Arms	|W
	 * Sorcery
	 * Join forces - Starting with you, each player may pay any amount of mana. Each player puts X 1/1 |Swhite Soldier creature tokens onto the battlefield, where X is the total amount of mana paid this way. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = join_forces(player, 0);
		if( amount > 0 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SOLDIER, &token);
			token.qty = amount;
			generate_token(&token);

			token.t_player = 1-player;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_animar_soul_of_elements(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){

	   int trig = 0;

	   if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
		   trig = 1;
	   }

	   if( trig > 0 ){
		  if(event == EVENT_TRIGGER){
			 event_result |= RESOLVE_TRIGGER_MANDATORY;
		  }
		  else if(event == EVENT_RESOLVE_TRIGGER){
				  add_1_1_counter(player, card);
		  }
	   }
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			COST_COLORLESS -= count_1_1_counters(player, card);
		}
	}

	return 0;
}

int card_archangel_of_strife(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (comes_into_play(player, card, event)){
		int i;
		for(i=0; i<2; i++){
			int ai_choice = 0;
			if( count_permanents_by_type(1-i, TYPE_CREATURE) < count_permanents_by_type(i, TYPE_CREATURE)){
				ai_choice = 1;
			}
			int choice = do_dialog(i, player, card, -1, -1, " War\n Peace", ai_choice);
			instance->targets[i+1].player = choice == 0 ? 3 : 0;
			instance->targets[i+1].card = choice == 1 ? 3 : 0;
		}
	}
	if (instance->targets[1].player != -1){
		boost_creature_type(player, card, event, -1, instance->targets[1].player, instance->targets[1].card, 0, player == 0 ? BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF : BCT_OPPONENT_ONLY);
		boost_creature_type(player, card, event, -1, instance->targets[2].player, instance->targets[2].card, 0, player == 1 ? BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF : BCT_OPPONENT_ONLY);
	}
	return 0;
}

int card_avatar_of_slaughter(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		event_result |= KEYWORD_FIRST_STRIKE;
		event_result |= KEYWORD_DOUBLE_STRIKE;
	}

	all_must_attack_if_able(current_turn, event, -1);

	return 0;
}

int card_basandra_battle_seraph(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_MODIFY_COST_GLOBAL && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
		infinite_casting_cost();
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) &&
		current_turn != player
	  ){
		return 1;
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE")){
				instance->number_of_targets = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										0, 0, 0, SP_KEYWORD_MUST_ATTACK);
			}
	}

	return 0;
}

int card_celestial_force(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(player, 3);
	}

	return 0;
}

int card_champions_helm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if (event == EVENT_ABILITIES &&
		is_equipping(player, card) && is_legendary(instance->targets[8].player, instance->targets[8].card)
	   ){
		hexproof(instance->targets[8].player, instance->targets[8].card, event);
	}
	return vanilla_equipment(player, card, event, 1, 2, 2, 0, 0);
}

int card_chaos_warp(int player, int card, event_t event){

	/* Chaos Warp	|2|R
	 * Instant
	 * The owner of target permanent shuffles it into his or her library, then reveals the top card of his or her library. If it's a permanent card, he or she
	 * puts it onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PERMANENT");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				int t_player = instance->targets[0].player;
				shuffle_into_library(t_player, instance->targets[0].card);
				int *deck = deck_ptr[t_player];
				if( deck[0] != -1 ){
					reveal_card_iid(player, card, deck[0]);
					if( is_what(-1, deck[0], TYPE_PERMANENT) ){
						put_into_play_a_card_from_deck(t_player, t_player, 0);
					}
				}
			 }
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_collective_voyage(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = join_forces(player, 0);
		if( amount > 0 ){
			int i, p;
			for(p=0; p<2; p++){
				for(i=0; i<amount; i++){
					int mode = 14;
					if( i == amount-1 ){
						mode = 4;
					}
					global_tutor(p, p, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, mode, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_command_tower(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && get_id(player, count) == CARD_ID_ELDER_DRAGON_HIGHLANDER ){
					card_instance_t *this = get_card_instance(player, count);
					int fake = this->info_slot;
					instance->info_slot = cards_data[fake].color;
					instance->info_slot |= get_color_from_remainder_text(cards_data[fake].id);
					break;
				}
				count++;
		}
		if( event == EVENT_RESOLVE_SPELL ){
			play_land_sound_effect_force_color(player, card, instance->info_slot);
		}
	}
	else if( instance->info_slot > 0 ){
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_crescendo_of_war(int player, int card, event_t event){

	/* Crescendo of War	|3|W
	 * Enchantment
	 * At the beginning of each upkeep, put a strife counter on ~.
	 * Attacking creatures get +1/+0 for each strife counter on ~.
	 * Blocking creatures you control get +1/+0 for each strife counter on ~. */

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_STRIFE);
	}

	if( event == EVENT_POWER ){
		int ok = 0;
		if (is_attacking(affected_card_controller, affected_card)){
			ok = 1;
		} else if (affected_card_controller == player && player != current_turn){
			card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
			if ((aff->state & STATE_BLOCKING) && aff->blocking != 255){
				ok = 1;
			}
		}
		if (ok){
			event_result += count_counters(player, card, COUNTER_STRIFE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_damia_sage_of_stone(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	deathtouch(player, card, event);

	skip_your_draw_step(player, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = 7-hand_count[player];
		if( amount > 0 ){
			draw_cards(player, amount);
		}
	}

	return 0;
}

int card_death_by_dragons(int player, int card, event_t event){
	/* Death by Dragons	|4|R|R
	 * Sorcery
	 * Each player other than target player puts a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_DRAGON, &token);
			token.t_player = 1-instance->targets[0].player;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_edric_spymaster_of_trest(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	return card_coastal_piracy(player, card, event);
}

int card_flusterstorm(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		int result = card_force_spike(player, card, event);
		if( result > 0 && ! is_what(card_on_stack_controller, card_on_stack, TYPE_PERMANENT) ){
			return result;
		}
		return 0;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int amount = 1+get_storm_count();
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, amount);
			kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_force_spike(player, card, event);
	}

	return 0;
}

int card_dread_cacodemon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot != 1 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.not_me = 1;
			new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
			new_manipulate_all(player, card, player, &this_test, ACT_TAP);
		}
	}

	return 0;
}

int card_ghave_guru_of_spores(int player, int card, event_t event){

	/* Ghave, Guru of Spores	|2|B|G|W
	 * Legendary Creature - Fungus Shaman 0/0
	 * ~ enters the battlefield with five +1/+1 counters on it.
	 * |1, Remove a +1/+1 counter from a creature you control: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * |1, Sacrifice a creature: Put a +1/+1 counter on target creature. */

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 5);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
		if( can_target(&td) ){
			return 1;
		}
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int choice = 0;
			int ai_choice = 0;
			if( can_target(&td) ){
				if( current_phase == PHASE_AFTER_BLOCKING ){
					ai_choice = 1;
				}
				if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)  ){
					choice = do_dialog(player, player, card, -1, -1, " Counter for Saproling\n Sac for +1/+1 counter\n Cancel", ai_choice);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 2  ){
				spell_fizzled = 1;
			}
			else if( choice == 0  ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						if( count_1_1_counters(instance->targets[0].player, instance->targets[0].card) > 0 ){
							remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
							instance->info_slot = 66;
						}
						else{
							spell_fizzled = 1;
						}
					}
			}
			else if( choice == 1  ){
					if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
							instance->number_of_targets = 1;
							instance->info_slot = 67;
						}
					}
					else{
						spell_fizzled = 1;
					}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_homeward_path(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_ACTIVATE && affect_me(player, card)){
		instance->info_slot = 0;
		int choice = 0;

		if (!paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)){
			if (player == AI || ai_is_speculating == 1){
				int c;
				for (c = 0; c < active_cards_count[1 - player] ; ++c){
					if (in_play(1 - player, c) && is_what(1 - player, c, TYPE_CREATURE) && is_stolen(1-player, c)){
						choice = 1;
						break;
					}
				}
				if (choice == 0){
					ai_modifier -= 24;
					return mana_producer(player, card, event);	// Discourage the AI from randomly tapping when not paying mana and no creatures for it to regain
				}
			}
			choice = do_dialog(player, player, card, -1, -1, " Produce 1\n Gain control of owned creatures\n Cancel", choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tapped_for_mana_color = -1;
			instance->state |= STATE_TAPPED;
			if (!charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)){
				instance->state &= ~STATE_TAPPED;
				cancel = 1;
			} else {
				instance->info_slot = 66;
			}
		}

		if( choice == 2 ){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				get_back_your_permanents(player, card, TYPE_CREATURE);

				int fake = get_internal_card_id_from_csv_id(CARD_ID_HOMEWARD_PATH);
				int card_added = add_card_to_hand(1-player, fake);
				get_back_your_permanents(1-player, card_added, TYPE_CREATURE);
				obliterate_card(1-player, card_added);
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_hornet_queen(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.qty = 4;
		token.key_plus = KEYWORD_FLYING;
		token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
		generate_token(&token);
	}

	return 0;
}

// hydra omnivore --> vanilla

static const char* target_is_angel_demon_or_dragon(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (has_subtype(player, card, SUBTYPE_ANGEL)
		  || has_subtype(player, card, SUBTYPE_DEMON)
		  || has_subtype(player, card, SUBTYPE_DRAGON)) ? NULL : EXE_STR(0x73964C); // localized ",subtype"
}

int card_kaalia_of_the_vast(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  /* Whenever ~ attacks an opponent, you may put an Angel, Demon, or Dragon creature card from your hand onto the battlefield tapped and attacking that
   * opponent. */
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_ATTACKS_PLAYER, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_HAND;
	  td.illegal_abilities = 0;
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_is_angel_demon_or_dragon;

	  if (!can_target(&td))
		return 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  instance->targets[0].card = -1;

	  if (player == AI || ai_is_speculating == 1)
		{
		  int c, max_index = -1, max_cmc = -1;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (in_hand(player, c) && is_what(player, c, TYPE_CREATURE)
				&& (has_subtype(player, c, SUBTYPE_ANGEL)
					|| has_subtype(player, c, SUBTYPE_DEMON)
					|| has_subtype(player, c, SUBTYPE_DRAGON))
				&& get_cmc(player, c) > max_cmc)
			  {
				max_cmc = get_cmc(player, c);
				max_index = c;
			  }
		  instance->targets[0].card = max_index;
		}
	  else if (pick_next_target_noload(&td, "Select an Angel, Demon, or Dragon creature card."))
		instance->number_of_targets = 0;
	  else
		instance->targets[0].card = -1;

	  if (instance->targets[0].card != -1)
		{
		  card_instance_t* aff = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		  aff->state |= STATE_TAPPED | STATE_ATTACKING;
		  put_into_play(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_karador_ghost_chieftain(int player, int card, event_t event){
	/*
	  Karador, Ghost Chieftain English |5|W|B|G
	  Legendary Creature - Centaur Spirit 3/4
	  Karador, Ghost Chieftain costs {1} less to cast for each creature card in your graveyard.
	  During each of your turns, you may cast one creature card from your graveyard.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_MODIFY_COST && affect_me(player, card) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		int minus = count_graveyard_by_type(player, TYPE_CREATURE);
		if( minus > 5 ){
			minus = 5;
		}
		COST_COLORLESS -= minus;
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_humiliated(player, card) && current_turn == player ){
		if( instance->info_slot != 66 ){
			int result = 0;
			int i;
			const int *grave = get_grave(player);
			for(i=0; i<count_graveyard(player); i++){
				if( is_what(-1, grave[i], TYPE_CREATURE) && has_mana_to_cast_iid(player, event, grave[i]) &&
					(is_what(-1, grave[i], TYPE_INSTANT | TYPE_INTERRUPT) || can_sorcery_be_played(player, event))
				  ){
					int cpt = can_legally_play_iid(player, grave[i]);
					if( cpt == 99 ){
						return 99;
					}
					if( cpt ){
						result = cpt;
					}
				}
			}
			return result;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int playable[2][count_graveyard(player)];
		int pc = 0;
		int i;
		const int *grave = get_grave(player);
		for(i=0; i<count_graveyard(player); i++){
			if( is_what(-1, grave[i], TYPE_CREATURE) && has_mana_to_cast_iid(player, event, grave[i]) &&
				(is_what(-1, grave[i], TYPE_INSTANT | TYPE_INTERRUPT) || can_sorcery_be_played(player, event))
			  ){
				if( can_legally_play_iid(player, grave[i]) ){
					playable[0][pc] = grave[i];
					playable[1][pc] = i;
					pc++;
				}
			}
		}
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to play.");
		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &test);
		if( selected != -1 && charge_mana_from_id(player, -1, event, cards_data[playable[0][selected]].id)){
			instance->info_slot = 66;
			play_card_in_grave_for_free(player, player, playable[1][selected]); // Not really for free, but cards that check it usually allow cancelling, which would be disastrous here.  Should probably factor out the entire charge-mana/remove-from-grave/play-spell sequence.
			cant_be_responded_to = 1;	// The card cast from graveyard will be respondable to, but the extra activation effect from Karador won't be
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_magmatic_force(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 3);
		}
	}

	return 0;
}

int card_mana_charged_dragon(int player, int card, event_t event)
{
  /* Join forces - Whenever ~ attacks or blocks, each player starting with you may pay any amount of mana. ~ gets +X/+0 until end of turn, where X is the total
   * amount of mana paid this way. */
  if (blocking(player, card, event) || declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  int amt = join_forces(player, 1);
	  if (amt > 0)
		pump_until_eot(player, card, player, card, amt, 0);
	}

  return 0;
}

int card_martyrs_bond(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == player){

		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }

		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_PERMANENT) &&
			! is_what(affected_card_controller, affected_card, TYPE_LAND) &&
			affected->kill_code > 0 && affected->kill_code < KILL_REMOVE
		){
			if( instance->targets[11].player < 0 ){
				instance->targets[11].player = 0;
			}
			int pointer = instance->targets[11].player;
			instance->targets[pointer].card = get_type(affected_card_controller, affected_card);

			instance->targets[11].player++;
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int fs = can_sacrifice(player, 1-player, 1, TYPE_PERMANENT, 0);
		if( fs ){
			int i;
			for(i=0;i<instance->targets[11].player;i++){
				sacrifice(player, card, 1-player, 1, instance->targets[i].card, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		instance->targets[11].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_minds_aglow(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = join_forces(player, 0);
		draw_cards(player, amount);
		draw_cards(1-player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_nin_the_pain_artist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		   if( valid_target(&td)  ){
			   damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, instance->parent_card);
			   draw_cards(instance->targets[0].player, instance->info_slot);
		   }
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, -1, 0, 1, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_riddlekeeper(int player, int card, event_t event)
{
  /* Whenever a creature attacks you or a planeswalker you control, that creature's controller puts the top two cards of his or her library into his or her
   * graveyard. */
  if (declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, 1-player, -1))
	mill(1-player, 2);

  return 0;
}

int card_riku_of_two_reflections(int player, int card, event_t event){

	/* Riku of Two Reflections	|2|U|R|G
	 * Legendary Creature - Human Wizard 2/2
	 * Whenever you cast an instant or sorcery spell, you may pay |U|R. If you do, copy that spell. You may choose new targets for the copy.
	 * Whenever another nontoken creature enters the battlefield under your control, you may pay |G|U. If you do, put a token that's a copy of that creature
	 * onto the battlefield. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		has_mana_multi(player, MANACOST_UR(1, 1))
	  ){
		test_definition_t test;
		default_test_definition(&test, TYPE_SPELL);
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test) &&
			charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_UR(1,1))
		  ){
			copy_spell_from_stack(player, trigger_cause_controller, trigger_cause);
		}
	}

	if(	trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller &&
		has_mana_multi(player, MANACOST_UG(1, 1))
	  ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.not_me = 1;
		test.type_flag = F1_NO_TOKEN;

		if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test) &&
			charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_GU(1,1))
		   ){
			token_generation_t token;
			copy_token_definition(player, card, &token, trigger_cause_controller, trigger_cause);
			generate_token(&token);
		}
	}

	return 0;
}

int card_ruhan_of_the_fomori(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	attack_if_able(player, card, event);

	return 0;
}

int card_scavenging_ooze(int player, int card, event_t event)
{
  /* Scavenging Ooze	|1|G
   * Creature - Ooze 2/2
   * |G: Exile target card from a graveyard. If it was a creature card, put a +1/+1 counter on ~ and you gain 1 life. */

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST_G(1)) && !graveyard_has_shroud(ANYBODY) && (get_grave(0)[0] != -1 || get_grave(1)[0] != -1);

  if (event == EVENT_ACTIVATE)
	{
	  if (!charge_mana_for_activated_ability(player, card, MANACOST_G(1)))
		return 0;

	  if (IS_AI(player))
		{
		  test_definition_t test_creature;
		  new_default_test_definition(&test_creature, TYPE_CREATURE, "");

		  if (select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &test_creature, 0, 1) != -1)
			return 0;	// found a creature card
		}

	  test_definition_t test;
	  new_default_test_definition(&test, 0, "Select a card to exile.");
	  select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &test, 0, 1);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
	  if (selected != -1)
		{
		  if (is_what(-1, get_grave(instance->targets[0].player)[selected], TYPE_CREATURE))
			{
			  if (in_play(instance->parent_controller, instance->parent_card))
				add_1_1_counter(instance->parent_controller, instance->parent_card);
			  gain_life(player, 1);
			}
		  rfg_card_from_grave(instance->targets[0].player, selected);
		}
	}

  return 0;
}

int card_scythe_specter(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) && hand_count[1-player] > 0 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);

		ec_definition_t this_definition;
		default_ec_definition(1-player, 1-player, &this_definition);
		this_definition.ai_selection_mode = AI_MIN_CMC;
		int result = new_effect_coercion(&this_definition, &this_test);
		lose_life(1-player, get_cmc_by_id(result));
	}

	return 0;
}

int card_sewer_nemesis(int player, int card, event_t event){
	/* Sewer Nemesis	|3|B
	 * Creature - Horror 100/100
	 * As ~ enters the battlefield, choose a player.
	 * ~'s power and toughness are each equal to the number of cards in the chosen player's graveyard.
	 * Whenever the chosen player casts a spell, that player puts the top card of his or her library into his or her graveyard. */

	if (card == -1){
		return 0;	// no player chosen if this isn't on the bf
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		pick_target(&td, "TARGET_PLAYER");
		instance->targets[1] = instance->targets[0];
	}

	if( specific_spell_played(player, card, event, instance->targets[1].player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		mill(instance->targets[1].player, 1);
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && instance->targets[1].player >= 0){
		event_result += count_graveyard(instance->targets[1].player);
	}

	return 0;
}

int card_shared_trauma(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = join_forces(player, 0);
		mill(player, amount);
		mill(1-player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int effect_skullbriar_the_walking_grave(int player, int card, event_t event)
{
  return 0;	// just a marker
}
int card_skullbriar_the_walking_grave(int player, int card, event_t event)
{
  /* Skullbriar, the Walking Grave	|B|G
   * Legendary Creature - Zombie Elemental 1/1
   * Haste
   * Whenever ~ deals combat damage to a player, put a +1/+1 counter on it.
   * Counters remain on Skullbriar as it moves to any zone other than a player's hand or library. */

  check_legend_rule(player, card, event);

  haste(player, card, event);

  card_instance_t* instance;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  int p, c;
	  p = get_owner(player, card);
	  for (c = 0; c < active_cards_count[p]; ++c)
		if ((instance = in_play(p, c)) && instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_skullbriar_the_walking_grave)
		  {
			copy_counters(player, card, p, c, 1);
			kill_card(p, c, KILL_REMOVE);
		  }
	}

  if (trigger_condition == TRIGGER_LEAVE_PLAY
	  && (instance = get_card_instance(player, card))
	  && instance->kill_code > 0 && instance->kill_code <= KILL_REMOVE
	  && leaves_play(player, card, event))
	{
	  int legacy, p = get_owner(player, card);
	  if (p != player)
		legacy = create_legacy_effect_for_opponent(player, card, &effect_skullbriar_the_walking_grave, -1, -1);
	  else
		legacy = create_legacy_effect(player, card, &effect_skullbriar_the_walking_grave);

	  if (legacy != -1)
		copy_counters(p, legacy, player, card, 1);
	}

  return card_slith_predator(player, card, event);
}


int card_soul_snare(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && current_turn != player ){
			return can_target(&td);
		}
	}

	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

	return global_enchantment(player, card, event);
}

int card_spell_crumple(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			instance->targets[1].player = card_on_stack_controller;
			instance->targets[1].card = card_on_stack;
			return result;
		}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			set_flags_when_spell_is_countered(player, card, instance->targets[1].player, instance->targets[1].card);
			put_on_bottom_of_deck(instance->targets[1].player, instance->targets[1].card);
			put_on_bottom_of_deck(player, card);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_syphon_flesh(int player, int card, event_t event){
	/* Syphon Flesh	|4|B
	 * Sorcery
	 * Each other player sacrifices a creature. You put a 2/2 |Sblack Zombie creature token onto the battlefield for each creature sacrificed this way. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int result = impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( result > 0 ){
				generate_token_by_id(player, card, CARD_ID_ZOMBIE);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_tariel_reckoner_of_souls(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && count_graveyard_by_type(1-player, TYPE_CREATURE) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			return would_valid_target(&td);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				tutor_random_permanent_from_grave(player, card, instance->targets[0].player, TUTOR_PLAY, TYPE_CREATURE, 1, REANIMATE_DEFAULT);
			}
	}
	return 0;
}

int card_the_mimeoplasm(int player, int card, event_t event)
{
  /* The Mimeoplasm	|2|G|U|B
   * Legendary Creature - Ooze 0/0
   * As ~ enters the battlefield, you may exile two creature cards from graveyards. If you do, it enters the battlefield as a copy of one of those cards with a
   * number of additional +1/+1 counters on it equal to the power of the other card. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && IS_AI(player) && count_graveyard_by_type(ANYBODY, TYPE_CREATURE) < 2)
	ai_modifier += (player == AI ? -1000 : 1000);

  int iid = -1;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->state |= STATE_OUBLIETTED;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to copy.");
	  test.id = 904; // face-down card (draw-a-card effect)
	  test.id_flag = DOESNT_MATCH;

	  if (select_target_from_either_grave(player, card, SFG_NOTARGET, AI_MAX_VALUE, AI_MAX_VALUE, &test, 0, 1) != -1)
		{
		  iid = turn_card_in_grave_face_down(instance->targets[0].player, instance->targets[1].player);

		  strcpy(test.message, "Select a creature card for counters.");
		  int iid2 = select_target_from_either_grave(player, card, SFG_NOTARGET, AI_MAX_VALUE, AI_MAX_VALUE, &test, 2, 3);
		  turn_card_in_grave_face_up(instance->targets[0].player, instance->targets[1].player, iid);
		  if (iid2 == -1)
			iid = -1;
		  else
			{
			  ++hack_silent_counters;
			  add_1_1_counters(player, card, get_base_power_iid(instance->targets[2].player, iid2));
			  --hack_silent_counters;

			  // If both from the same graveyard, remove the one with the higher index first
			  int first, second;
			  if (instance->targets[0].player == instance->targets[2].player)
				{
				  first = MAX(instance->targets[1].player, instance->targets[3].player);
				  second = MIN(instance->targets[1].player, instance->targets[3].player);
				}
			  else
				{
				  first = instance->targets[1].player;
				  second = instance->targets[3].player;
				}
				rfg_card_from_grave(instance->targets[0].player, first);
				rfg_card_from_grave(instance->targets[2].player, second);
				//Reset the target array to avoid unwanted interactions with Duplicant, for example.
				int i;
				for(i=0; i<4; i++){
					instance->targets[i].player = -1;
					instance->targets[i].card = -1;
				}
			}
		}
	  instance->state &= ~STATE_OUBLIETTED;
	}

  enters_the_battlefield_as_copy_of(player, card, event, -1, iid);	// enters_the_battlefield_as_copy_of() only examines iid during EVENT_RESOLVE_SPELL

  return 0;
}

int card_trench_gorger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int amount = 0;

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to exile.");
		this_test.no_shuffle = 1;

		while( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
				amount++;
				if( amount > 9 && player == AI ){
					break;
				}
		}
		shuffle(player);
		instance->targets[1].card = amount;
	}
	if( instance->targets[1].card > 0 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, instance->targets[1].card-6, instance->targets[1].card-6, 0);
	}
	return 0;
}

int card_tribute_to_the_wild(int player, int card, event_t event){
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
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_ARTIFACT | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


int card_vish_kal_blood_arbiter( int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	check_legend_rule(player, card, event);

	lifelink(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int result = 0;
			if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				result = 1;
			}
			if( count_1_1_counters(player, card) > 0 && can_target(&td1) ){
				result = 1;
			}
			return result;
		}
	}

	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				int choice = 0;
				int ai_choice = 0;
				if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					if( count_1_1_counters(player, card) > 0 && can_target(&td1) ){
						ai_choice = 1;
						if( is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING ){
							ai_choice = 0;
						}
						choice = do_dialog(player, player, card, -1, -1, " Pump Vish Kal\n Weaken creature\n Do nothing", ai_choice);
					}
				}
				else{
					 choice = 1;
				}

				if( choice == 0 ){
					if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						instance->targets[1].player = 66+choice;
					}
					else{
						spell_fizzled = 1;
					}
				}
				else if( choice == 1 ){
						if( pick_target(&td1, "TARGET_CREATURE") ){
							int amount = count_1_1_counters(player, card);
							remove_1_1_counters(player, card, amount);
							instance->targets[1].player = 66+choice;
							instance->targets[1].card = amount;
							instance->number_of_targets = 1;
						}
				}
				else{
					 spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 ){
				add_1_1_counters(player, instance->parent_card, instance->targets[2].player);
			}
			if( instance->targets[1].player == 67 ){
				int amount = instance->targets[1].card;
				pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,-amount, -amount);
			}
	}

	return 0;
}

int card_stranglehold(int player, int card, event_t event){

	return global_enchantment(player, card, event);
}

int card_vow_of_duty(int player, int card, event_t event){

	return vow(player, card, event, 2, 2, 0, SP_KEYWORD_VIGILANCE);
}

int card_vow_of_flight(int player, int card, event_t event){

	return vow(player, card, event, 2, 2, KEYWORD_FLYING, 0);
}

int card_vow_of_lightning(int player, int card, event_t event){

	return vow(player, card, event, 2, 2, KEYWORD_FIRST_STRIKE, 0);
}

int card_vow_of_malice(int player, int card, event_t event){

	return vow(player, card, event, 2, 2, 0, SP_KEYWORD_INTIMIDATE);
}

int card_vow_of_wildness(int player, int card, event_t event){

	return vow(player, card, event, 3, 3, KEYWORD_TRAMPLE, 0);
}

int card_zedruu_the_greathearted( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 1, 1) &&
		can_target(&td)
	  ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td1);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 1, 1);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT")){
			instance->targets[1].player = 1-player;
			instance->targets[1].card = -1;
			instance->number_of_targets = 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
				give_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = 0;
		int count = active_cards_count[1-player]-1;
		while( count > -1 ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_PERMANENT) ){
					if( 1-player == AI && ! check_state(1-player, count, STATE_OWNED_BY_OPPONENT) ){
						amount++;
					}
					if( 1-player != AI && check_state(1-player, count, STATE_OWNED_BY_OPPONENT) ){
						amount++;
					}
				}
				count--;
		}
		gain_life(player, amount);
		draw_cards(player, amount);
	}

	return 0;
}


