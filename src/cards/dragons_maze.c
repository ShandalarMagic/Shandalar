#include "manalink.h"

// Functions
static int gatekeeper(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && count_subtype(player, TYPE_LAND, SUBTYPE_GATE) > 1){
		return 1;
	}

	return 0;

}

// Cards
// White
int card_boros_mastiff(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_LIFELINK);
	}
	return 0;

}

int card_haazda_snare_squad(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may pay |W. If you do, tap target creature an opponent controls.

	store_attackers(player, card, event, 0, player, card, NULL);

	if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);

		if(can_target(&td) && has_mana(player, COLOR_WHITE, 1) && is_attacking(player, card) ){
			if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
				if( charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_WHITE, 1) ){
					instance->number_of_targets = 0;
					if ( pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS") ){
						tap_card(instance->targets[0].player, instance->targets[0].card);
					}
				}
			}
		}
		instance->info_slot = 0;
	}
	
	return 0;
}

int card_lyev_decree(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				detain(player, card, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature opponent controls", 2, NULL);
}

int card_maze_sentinel(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		! is_humiliated(player, card)
	  ){
		if( affected_card == card || count_colors(affected_card_controller, affected_card) > 1 ){
			vigilance(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

int card_renounce_the_guilds(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {
					test_definition_t test;
					new_default_test_definition(&test, TYPE_PERMANENT, "Select a multicolored permanent to sacrifice.");
					test.color_flag = F3_MULTICOLORED;
					int sac = new_sacrifice(player, card, p, SAC_CAUSED|SAC_NO_CANCEL|SAC_JUST_MARK|SAC_RETURN_CHOICE, &test);
					if (sac){
						kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
					}
		
		});
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

}

int card_riot_control(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		prevent_the_next_n_damage(player, card, player, -1, 0, PREVENT_INFINITE, 0, 0);
		gain_life(player, count_subtype(1-player, TYPE_CREATURE, -1));
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_scion_of_vitu_ghazi(int player, int card, event_t event){
	/* Scion of Vitu-Ghazi	|3|W|W
	 * Creature - Elemental 4/4
	 * When ~ enters the battlefield, if you cast it from your hand, put a 1/1 |Swhite Bird creature token with flying onto the battlefield, then populate. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) && instance->info_slot != 1 ){
		generate_token_by_id(player, card, CARD_ID_BIRD);
		populate(player, card);
	}

	return 0;
}

// steeple rock --> vanilla

int card_sunspire_gatekeeper(int player, int card, event_t event){

	if( gatekeeper(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KNIGHT, &token);
		token.s_key_plus = SP_KEYWORD_VIGILANCE;
		generate_token(&token);
	}

	return 0;
}

int card_wake_the_reflections(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		populate(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// Blue

int card_aetherling(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum{
		CHOICE_EXILE = 1,
		CHOICE_UNBLOCKABLE,
		CHOICE_P1_M1,
		CHOICE_M1_P1
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int priorities[4] = {	5,
								current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS && ! check_for_special_ability(player, card, SP_KEYWORD_UNBLOCKABLE) ? 15 : 0,
								is_attacking(player, card) && is_unblocked(player, card) ? 10 : 0,
								is_attacking(player, card) && ! is_unblocked(player, card) ? 10 : 0
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Exile until eot", has_mana_for_activated_ability(player, card, MANACOST_U(1)), priorities[0],
						"Unblockable until eot", has_mana_for_activated_ability(player, card, MANACOST_U(1)), priorities[1],
						"+1/-1", has_mana_for_activated_ability(player, card, MANACOST_X(1)), priorities[2],
						"-1/+1", has_mana_for_activated_ability(player, card, MANACOST_X(1)), priorities[3]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XU(((choice == 3 || choice == 4) ? 1 : 0), ((choice == 1 || choice == 2) ? 1 : 0))) ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_EXILE ){
			remove_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == CHOICE_UNBLOCKABLE ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
		if( instance->info_slot == CHOICE_P1_M1 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, -1);
		}
		if( instance->info_slot == CHOICE_M1_P1 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, -1, 1);
		}
	}

	return 0;
}

int card_hidden_strings(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = played_for_free(player, card)+is_token(player, card);
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PERMANENT") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( can_target(&td) ){
				new_pick_target(&td, "TARGET_PERMANENT", 1, 0);
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		int good = 0;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				good++;
				twiddle(player, card, i);
			}
		}
		if( good > 0 ){
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_maze_glider(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( ! is_humiliated(player, card) && count_colors(affected_card_controller, affected_card) > 1 ){
			event_result |= KEYWORD_FLYING;
		}
	}

	return 0;
}

int card_mindstatic(int player, int card, event_t event){
	/* Mindstatic	|3|U
	 * Instant
	 * Counter target spell unless its controller pays |6. */

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 6);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_opal_lake_gatekeeper(int player, int card, event_t event){

	if( gatekeeper(player, card, event) ){
		draw_some_cards_if_you_want(player, card, player, 1);
	}

	return 0;
}

int card_runners_bane(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( comes_into_play(player, card, event) ){
			tap_card(instance->damage_target_player, instance->damage_target_card);
		}
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

// trait doctoring --> impossible

int card_uncovered_clues(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int max = 4;
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		int tutored = 0;
		while( max > 0 && tutored < 2 ){
				char msg[100] = "Select an instant or sorcery card.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_SPELL, msg);
				this_test.type_flag = F1_NO_CREATURE;
				this_test.create_minideck = max;
				this_test.no_shuffle = 1;
				if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
					max--;
				}
				tutored++;
		}
		if( max > 0 ){
			put_top_x_on_bottom(player, player, max);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// Black

// bane alley blackguard --> vanilla

int card_blood_scrivener(int player, int card, event_t event){

	/* Blood Scrivener	|1|B
	 * Creature - Zombie Wizard 2/1
	 * If you would draw a card while you have no cards in hand, instead draw two cards and lose 1 life. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && hand_count[player] <= 0 && !suppress_draw){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot == 1 || is_humiliated(player, card) ){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				instance->info_slot = 1;	// Prevent from triggering on its own draws
				draw_cards(player, 2);
				lose_life(player, 1);
				instance->info_slot = 0;
				suppress_draw = 1;
		}
	}
	return 0;
}

int card_crypt_incursion(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );

			const int *grave = get_grave(instance->targets[0].player);
			int amount = 0;
			int cg = count_graveyard(instance->targets[0].player)-1;
			while( cg > -1 ){
					if( is_what(-1, grave[cg], TYPE_CREATURE) ){
						rfg_card_from_grave(instance->targets[0].player, cg);
						amount+=3;
					}
					cg--;
			}
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_fatal_fumes(int player, int card, event_t event){
	/* Fatal Fumes	|3|B
	 * Instant
	 * Target creature gets -4/-2 until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -4, -2, 0, 0);
}

int card_hired_torturer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			if( hand_count[instance->targets[0].player] > 0 ){
				int t_hand[100];
				int th_count = 0;
				int revealed[1];
				int count = 0;
				while( count < active_cards_count[instance->targets[0].player] ){
						if( in_hand(instance->targets[0].player, count) ){
							card_instance_t *crd = get_card_instance( instance->targets[0].player, count );
							t_hand[th_count] = crd->internal_card_id;
							th_count++;
						}
						count++;
				}
				revealed[0] = t_hand[0];
				if( th_count > 1 ){
					revealed[0] = t_hand[internal_rand(th_count)];
				}
				show_deck(HUMAN, revealed, 1, "Hired Torturer revealed...", 0, 0x7375B0 );
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_ONLY_TARGET_OPPONENT, 3, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}


int card_maze_abomination(int player, int card, event_t event)
{
  deathtouch(player, card, event);

  if (event == EVENT_ABILITIES
	  && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && count_colors(affected_card_controller, affected_card) > 1
	  && !is_humiliated(player, card))
	deathtouch(affected_card_controller, affected_card, event);

  return 0;
}

int card_pontiff_of_blight(int player, int card, event_t event){

	if( has_mana_hybrid(player, COLOR_BLACK, COLOR_WHITE, 1, 0) && !is_humiliated(player, card) &&
		specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	 ){
		int count = active_cards_count[player];
		while( count > -1 && has_mana_hybrid(player, COLOR_BLACK, COLOR_WHITE, 1, 0) ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					charge_mana_hybrid(player, card, COLOR_BLACK, COLOR_WHITE, 1, 0);
					if( spell_fizzled != 1 ){
						int result = lose_life(1-player, 1);
						gain_life(player, result);
					}
				}
				count--;
		}
	}
	return 0;
}

// rakdos drake --> dead reveler

int card_sinister_possession(int player, int card, event_t event)
{
  // Whenever enchanted creature attacks or blocks, its controller loses 2 life.
  card_instance_t* instance;
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS || event == EVENT_DECLARE_BLOCKERS)
	  && (instance = in_play(player, card))
	  && instance->damage_target_player >= 0
	  && (declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card)
		  || blocking(instance->damage_target_player, instance->damage_target_card, event)))
	lose_life(instance->damage_target_player, 2);

  // Enchant creature
  return vanilla_aura(player, card, event, 1-player);
}

int card_ubal_sar_gatekeeper(int player, int card, event_t event){

	if( gatekeeper(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
	}

	return 0;
}

// Red
int card_awe_for_the_guilds(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = 0;
		this_test.color_flag = F3_MONOCOLORED;
		creatures_cannot_block(player, card, &this_test, 1);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_clear_a_path(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_DEFENDER;

	if (event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with defender.", 1, NULL);
}

int card_maze_rusher(int player, int card, event_t event){

	/* Maze Rusher	|5|R
	 * Creature - Elemental 6/3
	 * Haste
	 * Multicolored creatures you control have haste. */

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( ! is_humiliated(player, card) && (count_colors(affected_card_controller, affected_card) > 1 || affected_card == card) ){
			haste(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

int card_possibility_storm(int player, int card, event_t event){
	/*
	  Possibility |3|R|R
	  Enchantment
	  Whenever a player casts a spell from his or her hand, that player exiles it, then exiles cards from the top of his or her library until
	  he or she exiles a card that shares a card type with it. That player may cast that card without paying its mana cost.
	  Then he or she puts all cards exiled with Possibility Storm on the bottom of his or her library in a random order.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card) && ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
		! not_played_from_hand(trigger_cause_controller, trigger_cause)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				instance->targets[1].player = trigger_cause_controller;
				instance->targets[1].card = trigger_cause;
				int *deck = deck_ptr[instance->targets[1].player];
				int count = 0;
				if( deck[0] != -1 ){
					int type = get_type(instance->targets[1].player, instance->targets[1].card);
					int good = 0;
					while( deck[count] != -1 ){
							if( is_what(-1, deck[count], type)){
								good = 1;
								break;
							}
							count++;
					}
					show_deck(HUMAN, deck, count+1, "Cards revealed by Possibility Storm", 0, 0x7375B0 );
					if( good == 1 && can_legally_play_iid(instance->targets[1].player, deck[count]) ){
						int csvid = cards_data[deck[count]].id;
						card_ptr_t* c = cards_ptr[ csvid ];
						char buffer[100];
						scnprintf(buffer, 100, " Play %s\n Pass", c->name);
						int choice = do_dialog(instance->targets[1].player, player, card, -1, -1, buffer, 0);
						if( choice == 0 ){
							rfg_card_in_deck(player, count);
							play_card_in_exile_for_free(instance->targets[1].player, instance->targets[1].player, csvid);
						}
						else{
							count++;
						}
					}
				}
				put_on_top_of_deck(instance->targets[1].player, instance->targets[1].card);
				count++;
				put_top_x_on_bottom_in_random_order(instance->targets[1].player, count);
		}
	}

	return global_enchantment(player, card, event);
}

int card_punish_the_enemy(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1) ;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			damage_player(instance->targets[0].player, 3, player, card);
		}
		if( validate_target(player, card, &td1, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 3, player, card);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_pyrewild_shaman(int player, int card, event_t event){
	return bloodrush(player, card, event, 1, 0, 0, 0, 1, 0, 3, 1, 0, 0);
}

int card_rubblebelt_maaka(int player, int card, event_t event){
	return bloodrush(player, card, event, 0, 0, 0, 0, 1, 0, 3, 3, 0, 0);
}

int card_smelt_ward_gatekeeper(int player, int card, event_t event){

	if( gatekeeper(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_weapon_surge(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(1, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, player, 0, 1, 0, KEYWORD_FIRST_STRIKE, 0, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 0, KEYWORD_FIRST_STRIKE, 0);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(1, 1));
}

// Green

// battering krasis --> cloudfin_raptor

int card_kraul_warrior(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_XG(5, 1), 3, 3, 0, 0);
}

int card_maze_behemoth(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( ! is_humiliated(player, card) && count_colors(affected_card_controller, affected_card) > 1 ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_mending_touch(int player, int card, event_t event){
	return card_death_ward(player, card, event);
}

int card_mutants_prey(int player, int card, event_t event){
	/* Mutant's Prey	|G
	 * Instant
	 * Target creature you control with a +1/+1 counter on it fights target creature an opponent controls. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature you control with a +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "Select target creature opponent controls", 1, 1 | GS_LITERAL_PROMPT);
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

int card_phytoburst(int player, int card, event_t event){
	/* Phytoburst	|1|G
	 * Sorcery
	 * Target creature gets +5/+5 until end of turn. */
	if (!IS_CASTING(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_pump(player, card, event, &td, 5, 5, 0, 0);
}

int card_renegade_krasis(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && trigger_cause_controller == player &&
		reason_for_trigger_controller == affected_card_controller && !is_humiliated(player, card)
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			if( get_power(trigger_cause_controller, trigger_cause) > get_power(player, card) ||
				get_toughness(trigger_cause_controller, trigger_cause) > get_toughness(player, card)
			  ){
				trig = 1;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(player, card);
					int i;
					for(i=0; i<active_cards_count[player]; i++){
						if( i != card && is_what(player, i, TYPE_CREATURE) && count_1_1_counters(player, i) > 0 ){
							add_1_1_counter(player, i);
						}
					}
			}
		}
	}
	return 0;
}

int card_skylasher(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	return flash(player, card, event);
}

int card_surali_gatekeeper(int player, int card, event_t event){

	if( gatekeeper(player, card, event) ){
		gain_life(player, 7);
	}

	return 0;
}

int card_thrashing_mossdog(int player, int card, event_t event){
	return scavenge(player, card, event, 4, 0, 0, 2, 0, 0);
}

// gold
int card_advent_of_the_wurm(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.pow = 5;
		token.tou = 5;
		token.key_plus = KEYWORD_TRAMPLE;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// armored wolf rider --> vanilla

// ascended lawmage --> aven_fleetwing

int card_beetleform_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 2, 2, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_UG(1, 1), 0, NULL, NULL);
}

int card_blast_of_genius(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			draw_cards(player, 3);
			if( hand_count[player] > 0 ){
				char msg[100] = "Select a card to discard.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
				int dmg = get_cmc(player, selected);
				discard_card(player, selected);
				damage_creature_or_player(player, card, event, dmg);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_blaze_commando(int player, int card, event_t event){
	/* Blaze Commando	|3|R|W
	 * Creature - Minotaur Soldier 5/3
	 * Whenever an instant or sorcery spell you control deals damage, put two 1/1 |Sred and |Swhite Soldier creature tokens with haste onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_player == player){
				int good = 0;
				if( ! in_play(damage->damage_source_player, damage->damage_source_card) ){
					good++;
				}
				else if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_SPELL) &&
						! is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE)
					  ){
						good++;
				}
				if( damage->info_slot > 0 ){
					good++;
				}
				else{
					card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( trg->targets[16].player > 0 ){
						good++;
					}
				}

				if( good > 1 ){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
		}
	}

	if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && ! is_humiliated(player, card)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_SOLDIER, &token);
				token.qty = 2*instance->targets[1].player;
				token.color_forced = COLOR_TEST_RED | COLOR_TEST_WHITE;
				token.s_key_plus = SP_KEYWORD_HASTE;
				generate_token(&token);
				instance->targets[1].player = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_blood_baron_of_vizkopa(int player, int card, event_t event){
	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
		!is_humiliated(player, card) && in_play(player, card)
	   ){
		lifelink(player, card, event);
		if( life[player] > 29 && life[1-player] < 11 ){
			modify_pt_and_abilities(player, card, event, 6, 6, KEYWORD_FLYING);
		}
	}
	return 0;
}

// boros battleshaper --> impossible

int card_bred_for_the_hunt(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_player == player &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		count_1_1_counters(damage->damage_source_player, damage->damage_source_card) > 0
	  ){
		get_card_instance(player, card)->info_slot++;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player && !is_humiliated(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot <= 0){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<instance->info_slot; i++){
					draw_some_cards_if_you_want(player, card, player, 1);
				}
				instance->info_slot = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_bronzebeak_moa(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			pump_until_eot(player, card, player, card, 3, 3);
		}
	}
	return 0;

}

int card_carnage_gladiator(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS && blocking(affected_card_controller, affected_card, event) && !is_humiliated(player, card) ){
		lose_life(affected_card_controller, 1);
	}
	return regeneration(player, card, event, 1, 1, 0, 0, 1, 0);

}

int card_council_of_the_absolute(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		if( player == AI ){
			int *deck = deck_ptr[1-player];
			int rounds = 0;
			if( deck[0] != -1 ){
				int i = internal_rand(count_deck(1-player));
				while( 1 ){
						if( deck[i] != -1 && ! is_what(-1, deck[i], TYPE_LAND) && ! is_what(-1, deck[i], TYPE_CREATURE) ){
							break;
						}
						i++;
						if( deck[i] == -1 ){
							i = 0;
							rounds++;
						}
						if( rounds > 1 ){
							break;
						}
				}
				if( rounds < 2 ){
					instance->targets[1].card = cards_data[deck[i]].id;
				}
				else{
					instance->targets[1].card = CARD_ID_WRATH_OF_GOD;
				}
			}
			else{
				instance->targets[1].card = CARD_ID_WRATH_OF_GOD;
			}
		}
		else{
			if( ai_is_speculating != 1 ){
				instance->targets[1].card = card_from_list(player, 3, TYPE_LAND | TYPE_CREATURE, 1, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		create_card_name_legacy(player, card, instance->targets[1].card);
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card)){
		if( get_id(affected_card_controller, affected_card) == instance->targets[1].card ){
			if( affected_card_controller == player ){
				COST_COLORLESS-=2;
			}
			else{
				infinite_casting_cost();
			}
		}
	}
	return 0;
}

int card_deadbridge_chant(int player, int card, event_t event){

	const int *grave = get_grave(player);

	if( comes_into_play(player, card, event) ){
		mill(player, 10);
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( grave[0] != -1 ){
			int tutored[1];
			int rnd = 0;
			if( count_graveyard(player) > 1 ){
				rnd = internal_rand(count_graveyard(player));
			}
			tutored[0] = grave[rnd];
			show_deck( HUMAN, tutored, 1, "Deadbridge Chant selected...", 0, 0x7375B0 );
			if( is_what(-1, grave[rnd], TYPE_CREATURE) ){
				reanimate_permanent(player, card, player, rnd, REANIMATE_DEFAULT);
			}
			else{
				from_grave_to_hand(player, rnd, TUTOR_HAND);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_debt_to_the_deathless(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int result = lose_life(1-player, 2*instance->info_slot);
		gain_life(player, result);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_deputy_of_acquittals(int player, int card, event_t event)
{
  // When ~ enters the battlefield, you may return another target creature you control to its owner's hand.
  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.special = TARGET_SPECIAL_NOT_ME;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

  return flash(player, card, event);
}

int card_dragonshift(int player, int card, event_t event){
	/*
	  Dragonshift |1|U|R
	  Instant
	  Until end of turn, target creature you control becomes a 4/4 blue and red Dragon, loses all abilities, and gains flying.
	  Overload {3}{U}{U}{R}{R} (You may cast this spell for its overload cost. If you do, change its text by replacing all instances of "target" with "each.")
	*/

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XUR(3, 2, 2));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 4;
			hc.toughness = 4;
			hc.subtype = SUBTYPE_DRAGON;
			hc.color = COLOR_TEST_BLUE | COLOR_TEST_RED;
			hc.keyword = KEYWORD_FLYING;
			if( instance->info_slot & 2 ){
				int c;
				for (c = active_cards_count[player]-1; c > -1; c--){
					if (in_play(player, c) && is_what(player, c, TYPE_CREATURE)){
						force_a_subtype(player, c, SUBTYPE_DRAGON);
						humiliate_and_set_pt_abilities(player, card, player, c, 4, &hc);
					}
				}
			}
			else{
				if (valid_target(&td)){
					force_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_DRAGON);
					humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 4, &hc);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XUR(3, 2, 2));
}

int card_drown_in_filth(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			mill(player, 4);
			int amount = count_graveyard_by_type(player, TYPE_LAND);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
		}
		else{
			cancel = 1;
		}
		kill_card(player, card, KILL_BURY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_emmara_tandris(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_target_player == player && damage->info_slot > 0 &&
				is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE) &&
				is_token(damage->damage_target_player, damage->damage_target_card)
			  ){
				damage->info_slot = 0;
			}
		}
	}

	return 0;
}

int card_exava_rakdos_blood_witch(int player, int card, event_t event){

	/* Exava, Rakdos Blood Witch	|2|B|R
	 * Legendary Creature - Human Cleric 3/3
	 * First strike, haste
	 * Unleash
	 * Each other creature you control with a +1/+1 counter on it has haste. */

	haste(player, card, event);

	check_legend_rule(player, card, event);

	unleash(player, card, event);

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		count_1_1_counters(affected_card_controller, affected_card) >= 1 && !is_humiliated(player, card)
	  ){
		haste(affected_card_controller, affected_card, event);
	}

	return 0;
}

int card_fluxcharger(int player, int card, event_t event)
{
  // Whenever you cast an instant or sorcery spell, you may switch ~'s power and toughness until end of turn.
  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0))
	switch_power_and_toughness_until_eot(player, card, player, card);

  return 0;
}

int card_gaze_of_granite(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;
		this_test.cmc = get_card_instance(player, card)->info_slot+1;
		this_test.cmc_flag = 3;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_gleam_of_battle(int player, int card, event_t event)
{
  // Whenever a creature you control attacks, put a +1/+1 counter on it.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  add_1_1_counter(current_turn, attackers[amt]);
	}

  return global_enchantment(player, card, event);
}

int card_goblin_test_pilot(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int trgs[2][100];
			int t_count[2];
			int k;
			for(k=0; k<2; k++){
				t_count[k] = 0;
				instance->targets[0].player = k;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
				if( would_valid_target(&td) ){
					trgs[k][0] = -1;
					t_count[k]++;
				}
			}
			for(k=0; k<2; k++){
				int count = 0;
				while( count < active_cards_count[k] ){
						if( in_play(k, count) ){
							instance->targets[0].player = k;
							instance->targets[0].card = count;
							instance->number_of_targets = 1;
							if( would_valid_target(&td) ){
								trgs[k][t_count[k]] = count;
								t_count[k]++;
							}
						}
						count++;
				}
			}
			if( t_count[1-player] > 0 ){
				instance->targets[0].player = 1-player;
				if( t_count[player] > 0 ){
					instance->targets[0].player = internal_rand(2);
				}
				int rnd = 0;
				if( t_count[instance->targets[0].player] > 1 ){
					rnd = trgs[instance->targets[0].player][internal_rand(t_count[instance->targets[0].player])];
				}
				instance->targets[0].card = rnd;
			}
			else{
				instance->targets[0].player = player;
				int rnd = 0;
				if( t_count[instance->targets[0].player] > 1 ){
					rnd = trgs[instance->targets[0].player][internal_rand(t_count[instance->targets[0].player])];
				}
				instance->targets[0].card = rnd;
			}
			instance->number_of_targets = 1;
			tap_card(player, card);
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return 0;
}

int card_goblin_war_drums(int player, int card, event_t event);
int card_gruul_war_chant(int player, int card, event_t event)
{
  if (event == EVENT_POWER && affected_card_controller == player && is_attacking(affected_card_controller, affected_card) && ! is_humiliated(player, card) )
	event_result += 1;

  return card_goblin_war_drums(player, card, event);
}

int card_haunter_of_nightveil(int player, int card, event_t event){
	boost_creature_type(player, card, event, -1, -1, 0, 0, BCT_OPPONENT_ONLY);
	return 0;
}

int card_jelenn_sphinx(int player, int card, event_t event){
	vigilance(player, card, event);
	// Whenever ~ attacks, other attacking creatures get +1/+1 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		this_test.not_me = 1;
		pump_creatures_until_eot(player, card, player, 0, 1, 1, 0, 0, &this_test);
	}
	return 0;
}

int card_korozda_gorgon(int player, int card, event_t event){

	/* Korozda Gorgon	|3|B|G
	 * Creature - Gorgon 2/5
	 * Deathtouch
	 * |2, Remove a +1/+1 counter from a creature you control: Target creature gets -1/-1 until end of turn. */
	 
	deathtouch(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	deathtouch(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_target(&td) ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td1, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			instance->number_of_targets = 0;
			if( new_pick_target(&td, "Select a creature you control with a +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
				remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				pick_target(&td1, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return 0;
}

int card_krasis_incubation(int player, int card, event_t event)
{
  /* Krasis Incubation	|2|G|U
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature can't attack or block, and its activated abilities can't be activated.
   * |1|G|U, Return ~ to its owner's hand: Put two +1/+1 counters on enchanted creature. */

	card_instance_t* instance = get_card_instance(player, card);

	if(IS_GAA_EVENT(event) && instance->damage_target_player > -1){

		if (event == EVENT_RESOLVE_ACTIVATION){
			add_1_1_counters(instance->damage_target_player, instance->damage_target_card, 2);
		}

		return generic_activated_ability(player, card, event, GAA_BOUNCE_ME, MANACOST_XGU(1,1,1), 0, NULL, NULL);
	}

  return card_arrest(player, card, event);
}

int card_lavinia_of_the_tenth(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;
		this_test.cmc = 5;
		this_test.cmc_flag = 3;
		new_manipulate_all(player, card, 1-player, &this_test, ACT_DETAIN);
	}

	return 0;
}

static int li_legacy(int player, int card, event_t event){
	if( beginning_of_combat(player, card, event, player, -1) ){
		card_instance_t *instance = get_card_instance(player, card);
		int i;
		for(i=0; i<19; i++){
			if( instance->targets[i].player != -1 ){
				if( check_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id) ){
					int card_added = add_card_to_hand(instance->targets[i].player, instance->targets[i].card);
					remove_card_from_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id);
					put_into_play(instance->targets[i].player, card_added);
					give_haste(instance->targets[i].player, card_added);
				}
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_legions_initiative(int player, int card, event_t event){

	boost_creature_by_color(player, card, event, COLOR_TEST_RED, 1, 0, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	boost_creature_by_color(player, card, event, COLOR_TEST_WHITE, 0, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_RW(1, 1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE  ){
		charge_mana_for_activated_ability(player, card, MANACOST_RW(1, 1));
		if( spell_fizzled != 1 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int count = active_cards_count[player]-1;
		int legacy = -1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					if( ! is_token(player, count) ){
						if( legacy == -1 ){
							legacy = create_legacy_effect(player, card, &li_legacy);
						}
						card_instance_t *leg = get_card_instance(player, legacy);
						int i;
						for(i=0; i<19; i++){
							if( leg->targets[i].player == -1 ){
								leg->targets[i].player = get_owner(player, count);
								leg->targets[i].card = get_original_internal_card_id(player, count);
								break;
							}
						}
					}
					kill_card(player, count, KILL_REMOVE);
				}
				count--;
		}
	}

	return global_enchantment(player, card, event);
}

int card_master_of_cruelties(int player, int card, event_t event)
{
  deathtouch(player, card, event);

  if (event == EVENT_ATTACK_LEGALITY && affected_card_controller == player && ! is_humiliated(player, card) )
	{
	  card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
	  if (aff->state & STATE_ATTACKING)	// Already attacking
		return 0;

	  if (affected_card != card)
		{
		  if (get_card_instance(player, card)->state & STATE_ATTACKING)
			event_result = 1;
		}
	  else
		{
		  int c;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if ((aff = in_play(player, c)) && (aff->state & STATE_ATTACKING))
			  {
				event_result = 1;
				break;
			  }
		}
	}

  if (event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && is_unblocked(player, card) && ! is_humiliated(player, card) )
	{
	  negate_combat_damage_this_turn(player, card, player, card, 0);
	  if (life[1-player] > 1)
		lose_life(1-player, life[1-player] - 1);
	  else if (life[1-player] < 1)
		gain_life(1-player, 1 - life[1-player]);
	  /* Amusingly, the latter will only work when the defending player should have already lost the game from having 0 or less life, but it hasn't kicked in
	   * yet because Manalink only checks that at end of phase; not when a Platinum Angel or such has prevented him from losing due to having 0 or less life. */
	}

  return 0;
}

int card_maw_of_obzedat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
}

int card_melek_izzet_paragon(int player, int card, event_t event)
{
  /* Melek, Izzet Paragon	|4|U|R
   * Legendary Creature - Weird Wizard 2/4
   * Play with the top card of your library revealed.
   * You may cast the top card of your library if it's an instant or sorcery card.
   * Whenever you cast an instant or sorcery spell from your library, copy it. You may choose new targets for the copy. */

  check_legend_rule(player, card, event);

  reveal_top_card(player, card, event);

  if (event == EVENT_CAN_ACTIVATE && ! is_humiliated(player, card))
	{
	  int iid = deck_ptr[player][0];
	  if (iid != -1 && is_what(-1, iid, TYPE_INSTANT | TYPE_SORCERY) && has_mana_to_cast_iid(player, event, iid))
		return can_legally_play_iid_now(player, iid, event);
	}

  if (event == EVENT_ACTIVATE && charge_mana_from_id(player, -1, event, cards_data[deck_ptr[player][0]].id))
	{
	  play_card_in_deck_for_free(player, player, 0);	// Yet another case where it's not free.
	  cant_be_responded_to = 1;
	}

  if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card)
	  && check_special_flags(trigger_cause_controller, trigger_cause, SF_PLAYED_FROM_DECK)
	  && specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SORCERY|TYPE_INSTANT,0, 0,0, 0,0, 0,0, -1,0))
	copy_spell_from_stack(player, trigger_cause_controller, trigger_cause);

  return 0;
}

int card_mirko_vosk_mind_drinker(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
		mind_funeral_effect(player, card, 1-player);
	}

	return 0;
}

int card_morgue_burst(int player, int card, event_t event){

	/* Morgue Burst	|4|B|R
	 * Sorcery
	 * Return target creature card from your graveyard to your hand. ~ deals damage to target creature or player equal to the power of the card returned this
	 * way. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_CAN_CAST && can_target(&td) && any_in_graveyard_by_type(player, TYPE_CREATURE) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 1) != -1 ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 1);
		if( selected != -1 ){
			int iid = get_grave(player)[selected];
			from_grave_to_hand(player, selected, TUTOR_HAND);
			/* Note that the card may no longer be in player's hand by now, as in the case of a Golgari Brownscale (triggers immediately for life gain); plenty
			 * of other things trigger on life gain, which may eventually result in a discard. */
			if ( valid_target(&td) ){
				damage_target0(player, card, get_base_power_iid(player, iid));
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_nivix_cyclops(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot_merge_previous(player, card, player, card, 3, 0);
		int leg = create_targetted_legacy_effect(player, card, effect_defender_can_attack_until_eot, player, card);
		if (leg != -1){
			get_card_instance(player, leg)->token_status |= STATUS_INVISIBLE_FX;
		}
	}

	return 0;
}

int card_notion_thief(int player, int card, event_t event){

	/* Notion Thief	|2|U|B
	 * Creature - Human Rogue 3/1
	 * Flash
	 * If an opponent would draw a card except the first one he or she draws in each of his or her draw steps, instead that player skips that draw and you draw
	 * a card. */

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && ! is_humiliated(player, card) &&
		reason_for_trigger_controller != player && !suppress_draw
	  ){
		if( event == EVENT_TRIGGER){
			event_result |= 2u;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				if( current_phase == PHASE_DRAW ){
					instance->info_slot++;
					if( instance->info_slot <= 1 ){
						return 0;
					}
				}
				draw_cards(player, 1);
				suppress_draw = 1;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return flash(player, card, event);
}

int card_obzedats_aid(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a permanent card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, msg);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
}

int card_pilfered_plans(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			mill(instance->targets[0].player, 2);
			draw_cards(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

static int effect_plasm_capture(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[0].player < 1 ){
		FORCE(produce_mana_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, instance->targets[0].card, NULL));
		instance->targets[0].player = 1;
		kill_card(player, card, KILL_REMOVE);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[0].player = 0;
	}
	return 0;
}

int card_plasm_capture(int player, int card, event_t event){

	/* Plasm Capture	|G|G|U|U
	 * Instant
	 * Counter target spell. At the beginning of your next precombat main phase, add X mana in any combination of colors to your mana pool, where X is that
	 * spell's converted mana cost. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		counterspell(player, card, event, NULL, 0);
		if( spell_fizzled != 1 ){
			instance->info_slot = get_cmc(card_on_stack_controller, card_on_stack);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);

			int legacy_card = create_legacy_effect(player, card, &effect_plasm_capture );
			card_instance_t *legacy = get_card_instance(player, legacy_card);
			legacy->targets[0].card = instance->info_slot;
			legacy->targets[1].card = get_id(player, card);
			legacy->targets[0].player = 0;
			if( current_turn == player ){
				legacy->targets[0].player++;
			}
		}

		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int effect_progenitor_mimic(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (!is_token(instance->damage_target_player, instance->damage_target_card))
	upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	copy_token(instance->damage_target_player, instance->damage_target_card, instance->damage_target_player, instance->damage_target_card);

  return 0;
}
int card_progenitor_mimic(int player, int card, event_t event)
{
  /* Progenitor Mimic	|4|G|U
   * Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield except it gains "At the beginning of your upkeep, if this creature isn't
   * a token, put a token onto the battlefield that's a copy of this creature." */

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	set_legacy_image(player, CARD_ID_PROGENITOR_MIMIC, create_targetted_legacy_effect(player, card, effect_progenitor_mimic, player, card));

  return 0;
}

int card_ral_zarek(int player, int card, event_t event){

	/* Ral Zarek	|2|U|R
	 * Planeswalker - Ral (4)
	 * +1: Tap target permanent, then untap another target permanent.
	 * -2: ~ deals 3 damage to target creature or player.
	 * -7: Flip five coins. Take an extra turn after this one for each coin that comes up heads. */

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);
		td1.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		enum {
			CHOICE_TAP_AND_UNTAP = 1,
			CHOICE_DAMAGE_PLAYER,
			CHOICE_COIN_WALK
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Tap & untap", target_available(player, card, &td) > 1, 10, 1,
						"Lightning Bolt", can_target(&td2), 5, -2,
						"Coin Walk", 1, 15, -7);
		if (event == EVENT_CAN_ACTIVATE){
			if (!choice)
				return 0;	// else fall through to planeswalker()
		}

		else if (event == EVENT_ACTIVATE){
				instance->number_of_targets = 0;
				if( choice == CHOICE_TAP_AND_UNTAP ){
					if( new_pick_target(&td, "Select a permanent to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						new_pick_target(&td1, "Select a permanent to untap.", 1, 1 | GS_LITERAL_PROMPT);
					}
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				}
				if( choice == CHOICE_DAMAGE_PLAYER ){
					pick_target(&td2, "TARGET_CREATURE_OR_PLAYER");
				}
		}

		else{	// event == EVENT_RESOLVE_ACTIVATION
			if( choice == CHOICE_TAP_AND_UNTAP ){
				if( validate_target(player, card, &td, 0) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				if( validate_target(player, card, &td1, 1) ){
					untap_card(instance->targets[1].player, instance->targets[1].card);
				}
			}
			else if( choice == CHOICE_DAMAGE_PLAYER ){
					if( valid_target(&td2) ){
						damage_target0(player, card, 3);
					}
			}
			else if( choice == CHOICE_COIN_WALK ){
					int tw = 0;
					int i;
					for(i=0; i<5; i++){
						tw += flip_a_coin(instance->parent_controller, instance->parent_card);
					}
					for(i=0; i<tw; i++){
						time_walk_effect(instance->parent_controller, instance->parent_card);
					}
			}
		}
	}
	return planeswalker(player, card, event, 4);
}

int card_reap_intellect(int player, int card, event_t event){

	/* Reap Intellect	|X|2|U|B
	 * Sorcery
	 * Target opponent reveals his or her hand. You choose up to X nonland cards from it and exile them. For each card exiled this way, search that player's
	 * graveyard, hand, and library for any number of cards with the same name as that card and exile them. Then that player shuffles his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		int result = would_valid_target(&td) && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
		instance->targets[0].player = -1;
		instance->targets[0].card = -1;
		instance->number_of_targets = 0;
		return result;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			instance->info_slot = x_value;
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			reveal_target_player_hand(instance->targets[0].player);
			int amount = instance->info_slot;

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");

			while( amount > 0 ){
					int i;
					int to_exile[2][hand_count[instance->targets[0].player]];
					int tec = 0;
					for(i=0; i<active_cards_count[instance->targets[0].player]; i++){
						if( in_hand(instance->targets[0].player, i) && ! is_what(instance->targets[0].player, i, TYPE_LAND) ){
							to_exile[0][tec] = get_original_internal_card_id(instance->targets[0].player, i);
							to_exile[1][tec] = i;
							tec++;
						}
					}
					if( ! tec ){
						break;
					}
					int result = select_card_from_zone(player, player, to_exile[0], tec, 0, AI_MAX_VALUE, -1, &this_test);
					if( result != -1 ){
						int id = get_id(instance->targets[0].player, to_exile[1][result]);
						rfg_card_in_hand(instance->targets[0].player, to_exile[1][result]);
						lobotomy_effect(player, instance->targets[0].player, id, 2);
						amount--;
					}
					else{
						break;
					}
			}
			shuffle(instance->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_render_silent(int player, int card, event_t event){
/*
Render Silent |W|U|U|
Instant
Counter target spell. Its controller can't cast spells this turn.
*/

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			target_player_cant_cast_type(player, card, instance->targets[0].player, TYPE_ANY);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
		return 0;
	}
	return card_counterspell(player, card, event);
}

// restore the peace --> impossible

int card_rot_farm_skeleton(int player, int card, event_t event){
	/* Rot Farm Skeleton	|2|B|G
	 * Creature - Plant Skeleton 4/1
	 * ~ can't block.
	 * |2|B|G, Put the top four cards of your library into your graveyard: Return ~ from your graveyard to the battlefield. Activate this ability only any time you could cast a sorcery. */

	cannot_block(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY && can_sorcery_be_played(player, event) ){
		if( has_mana_multi( player, 2, 1, 0, 1, 0, 0) ){
			return GA_RETURN_TO_PLAY;
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi( player, 2, 1, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				mill(player, 4);
				return GAPAID_REMOVE;
			}
	}

	return 0;
}

int card_ruric_thar_the_unbowed(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	attack_if_able(player, card, event);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
		damage_player(instance->targets[1].player, 6, player, card);
	}

	return 0;
}

int card_savageborn_hydra(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED, MANACOST0, 0, NULL, NULL) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		return has_mana_hybrid(player, 1, COLOR_RED, COLOR_GREEN, c1);
	}

	if(event == EVENT_ACTIVATE  ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		charge_mana_hybrid(player, card, 1, COLOR_RED, COLOR_GREEN, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(player, instance->parent_card);
	}

	return card_ivy_elemental(player, card, event);
}

int card_scab_clan_giant(int player, int card, event_t event){
	/* Scab-Clan Giant	|4|R|G
	 * Creature - Giant Warrior 4/5
	 * When ~ enters the battlefield, it fights target creature an opponent controls chosen at random. */

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		int cr_array[500];
		int cr_count = 0;
		int count = 0;
		while( count < active_cards_count[1-player] ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					instance->targets[0].card = count;
					if (valid_target(&td)){
						cr_array[cr_count] = count;
						cr_count++;
					}
				}
				count++;
		}
		if( cr_count > 0 ){
			int trg = cr_array[cr_count > 1 ? internal_rand(cr_count) : 0];
			instance->targets[0].card = trg;
			fight(player, card, 1-player, trg);
		}
		instance->number_of_targets = 0;
	}

	return 0;
}

static int showstopper_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, 0);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.allowed_controller = 1-player;

		if( resolve_gfp_ability(player, card, event, can_target(&td2) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			int i;
			for(i=0; i<instance->targets[11].card; i++){
				td2.illegal_abilities = instance->targets[i+2].player;
				if( can_target(&td2) && pick_target(&td2, "TARGET_CREATURE") ){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
				}
				instance->number_of_targets = 0;
			}
		}
		instance->targets[11].card = 0;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}
int card_showstopper(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &showstopper_legacy);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[0].card = get_id(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_sin_collector(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( would_validate_target(player, card, &td2, 0) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(1-player, player, &this_definition);
			this_definition.effect = EC_RFG;
			new_effect_coercion(&this_definition, &this_test);
		}
	}
	return 0;
}

int card_sire_of_insanity(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		APNAP(p, {new_discard_all(p, player);};);
	}

	return 0;
}

int card_species_gorger(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;

		return bounce_permanent_at_upkeep(player, card, event, &td);
	}
	return 0;
}

// spike jester --> vanilla

int card_tajic_blade_of_the_legion(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	if( battalion(player, card, event) ){
		pump_until_eot(player, card, player, card, 5, 5);
	}

	return 0;
}

int card_teysa_envoy_of_ghosts(int player, int card, event_t event){
	/* Teysa, Envoy of Ghosts	|5|W|B
	 * Legendary Creature - Human Advisor 4/4
	 * Vigilance, protection from creatures
	 * Whenever a creature deals combat damage to you, destroy that creature. Put a 1/1 |Swhite and |Sblack Spirit creature token with flying onto the battlefield. */

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	protection_from_creatures(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
					! check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
				  ){
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
						instance->targets[instance->info_slot].player = damage->damage_source_player;
						instance->targets[instance->info_slot].card = damage->damage_source_card;
						instance->info_slot++;
					}
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && ! is_humiliated(player, card)
	  ){
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

						token_generation_t token;
						default_token_definition(player, card, CARD_ID_SPIRIT, &token);
						token.pow = 1;
						token.tou = 1;
						token.key_plus = KEYWORD_FLYING;
						token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_BLACK;
						generate_token(&token);
					}
				}
				instance->info_slot = 0;
		}
	}

	return 0;
}

int card_tithe_drinker(int player, int card, event_t event){
	extort(player, card, event);
	lifelink(player, card, event);
	return 0;
}

int card_trostanis_summoner(int player, int card, event_t event){
	/* Trostani's Summoner	|5|G|W
	 * Creature - Elf Shaman 1/1
	 * When ~ enters the battlefield, put a 2/2 |Swhite Knight creature token with vigilance, a 3/3 |Sgreen Centaur creature token, and a 4/4 |Sgreen Rhino creature token with trample onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KNIGHT, &token);
		token.pow = 2;
		token.tou = 2;
		token.s_key_plus = SP_KEYWORD_VIGILANCE;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);

		default_token_definition(player, card, CARD_ID_CENTAUR, &token);
		token.pow = 3;
		token.tou = 3;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);

		default_token_definition(player, card, CARD_ID_RHINO, &token);
		token.pow = 4;
		token.tou = 4;
		token.key_plus = KEYWORD_TRAMPLE;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return 0;
}

int card_unflinching_courage(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, KEYWORD_TRAMPLE, SP_KEYWORD_LIFELINK, 0, 0, 0);
}

int card_varolz_the_scar_striped(int player, int card, event_t event){
	/* Varolz, the Scar-Striped	|1|B|G
	 * Legendary Creature - Troll Warrior 2/2
	 * Each creature card in your graveyard has scavenge. The scavenge cost is equal to its mana cost.
	 * Sacrifice another creature: Regenerate ~. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( (land_can_be_played & LCBP_REGENERATION) && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( can_sacrifice_as_cost(player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return can_regenerate(player, card);
			}
		}
		return can_sorcery_be_played(player, event) && can_cast_creature_from_grave(player, 1, event);
	}

	if( event == EVENT_ACTIVATE ){
		if( land_can_be_played & LCBP_REGENERATION ){
			state_untargettable(player, card, 1);
			if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)) && controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0)){
				instance->number_of_targets = 0;
				instance->info_slot = 66;
			}
			else{
				spell_fizzled = 1;
			}
			state_untargettable(player, card, 0);
		}
		else{
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);

			int selected = -1;
			if( player == AI ){
				selected = can_cast_creature_from_grave(player, 2, event);
			}
			else{
				selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, 1, -1, &this_test);
			}
			if( selected != -1 ){
				int good = 1;
				const int *grave = get_grave(player);
				if( ! is_what(-1, grave[selected], TYPE_CREATURE) || ! has_mana_to_cast_id(player, event, cards_data[grave[selected]].id) ){
					good = 0;
				}
				if( good == 1 ){
					charge_mana_from_id(player, -1, event, cards_data[grave[selected]].id);
					if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						instance->targets[1].player = get_base_power_iid(player, grave[selected]);
						instance->info_slot = 67;
						rfg_card_from_grave(player, selected);
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
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 66 && can_regenerate(player, instance->parent_card) ){
			regenerate_target(player, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player);
		}
	}

	return 0;
}

int card_viashino_firstblade(int player, int card, event_t event){
	haste(player, card, event);
	if( comes_into_play(player, card, event) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}

	return 0;
}

int card_voice_of_resurgence(int player, int card, event_t event){
	/* Voice of Resurgence	|G|W
	 * Creature - Elemental 2/2
	 * Whenever an opponent casts a spell during your turn or when ~ dies, put a |Sgreen and |Swhite Elemental creature token onto the battlefield with "This creature's power and toughness are each equal to the number of creatures you control." */

	if( (current_turn == player && specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)) ||
		this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)
	  ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.special_infos = 67;
		token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return 0;
}

int card_vorel_of_the_hull_clade(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			copy_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_UG(1, 1), 0, &td, "TWIDDLE");
}

int card_warleaders_helix(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_warped_physique(int player, int card, event_t event){
	/* Warped Physique	|U|B
	 * Instant
	 * Target creature gets +X/-X until end of turn, where X is the number of cards in your hand. */
	return vanilla_instant_pump(player, card, event, 2, 1-player, hand_count[player], -hand_count[player], 0, 0);
}

// woodlot crawler --> vanilla

// zhur thaa ancient --> mana flare

int card_zhur_taa_druid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_player(1-player, 1, player, instance->parent_card);
	}
	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

// Split
int card_alive_well(int player, int card, event_t event){
	/*
	  Alive |3|G
	  Sorcery
	  Put a 3/3 green Centaur creature token onto the battlefield.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Well |W
	  Sorcery
	  You gain 2 life for each creature you control.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_W(1), 1, 0, 1, NULL, NULL);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int priority_alive = 10;
	int priority_well = 0;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		priority_well = count_subtype(player, TYPE_CREATURE, -1) ? (40-(life[player]*2))+(count_subtype(player, TYPE_CREATURE, -1) * 2) : 0;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_CENTAUR, &token);
			token.pow = 3;
			token.tou = 3;
			token.color_forced = COLOR_TEST_GREEN;
			generate_token(&token);
		}
		if( instance->info_slot & 2 ){
			gain_life(player, 2*count_subtype(player, TYPE_CREATURE, -1));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, priority_alive, MANACOST_W(1), 1, priority_well, 1, "Alive", "Well");
}

int card_armed_dangerous(int player, int card, event_t event){
	/*
	  Armed |1|R
	  Sorcery
	  Target creature gets +1/+1 and gains double strike until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Dangerous |3|G
	  Sorcery
	  All creatures able to block target creature this turn do so.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XG(3, 1), can_target(&td), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		if( ! new_pick_target(&td, "Select target creature for Armed.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		if( ! new_pick_target(&td, "Select target creature for Dangerous.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1, KEYWORD_DOUBLE_STRIKE, 0);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td, ttv) ){
				pump_ability_until_eot(player, card, instance->targets[ttv].player, instance->targets[ttv].card, 0, 0, 0, SP_KEYWORD_LURE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XG(3, 1), can_target(&td), 5, 1, "Armed", "Dangerous");
}

static int beck_legacy(int player, int card, event_t event){
	if( specific_cip(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_some_cards_if_you_want(player, card, player, 1);
	}
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_beck_call(int player, int card, event_t event){
	/*
	  Beck |U|G
	  Sorcery
	  Whenever a creature enters the battlefield this turn, you may draw a card.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Call |4|W|U
	  Sorcery
	  Put four 1/1 white Bird creature tokens with flying onto the battlefield.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_XUW(4, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1){
			create_legacy_effect(player, card, &beck_legacy);
		}
		if( instance->info_slot & 2 ){
			generate_tokens_by_id(player, card, CARD_ID_BIRD, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 5, MANACOST_XUW(4, 1, 1), 1, 10, 1, "Beck", "Call");
}

int card_breaking_entering(int player, int card, event_t event){
	/*
	  Breaking |U|B
	  Sorcery
	  Target player puts the top eight cards of his or her library into his or her graveyard.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Entering |4|B|R
	  Sorcery
	  Put a creature card from a graveyard onto the battlefield under your control. It gains haste until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XBR(4, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		if( ! new_pick_target(&td, "Select target player for Breaking.", 0, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				mill(instance->targets[0].player, 8);
			}
		}
		if( instance->info_slot & 2 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int t_player = 1-player;
			int selected = -1;
			int counts[2] = {count_graveyard_by_type(0, TYPE_CREATURE), count_graveyard_by_type(1, TYPE_CREATURE)};
			while( selected == -1 && counts[0]+counts[1] > 0 ){
					if( counts[1-player] ){
						strcpy(this_test.message, "Select a creature card for Entering (opponent's graveyard)");
						selected = new_select_a_card(player, 1-player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
						t_player = 1-player;
					}
					if( selected == -1 && counts[player] ){
						strcpy(this_test.message, "Select a creature card for Entering (opponent's graveyard)");
						selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
						t_player = player;
					}
			}
			if( selected != -1 ){
				reanimate_permanent(player, card, t_player, selected, REANIMATE_HASTE_UNTIL_EOT);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 5, MANACOST_XBR(4, 1, 1), 1, 10, 1, "Breaking", "Entering");
}

int card_catch_release(int player, int card, event_t event){
	/*
	  Catch |1|U|R
	  Sorcery
	  Gain control of target permanent until end of turn. Untap it. It gains haste until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Release |4|R|W
	  Sorcery
	  Each player sacrifices an artifact, a creature, an enchantment, a land, and a planeswalker.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XRW(4, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		if( ! new_pick_target(&td, "Select target permanent for Catch.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		if( instance->info_slot & 2 ){
			type_t typs[] = { TYPE_ARTIFACT, TYPE_CREATURE, TYPE_ENCHANTMENT, TYPE_LAND, TARGET_TYPE_PLANESWALKER };
			int i;
			for (i = 0; i < 5; ++i){
				impose_sacrifice(player, card, player, 1, typs[i], 0, 0, 0, 0, 0, 0, 0, -1, 0);
				impose_sacrifice(player, card, 1-player, 1, typs[i], 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XRW(4, 1, 1), 1, 5, 1, "Catch", "Release");
}

int card_down_dirty(int player, int card, event_t event){
	/*
	  Down |3|B
	  Sorcery
	  Target player discards two cards.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Dirty |2|G
	  Sorcery
	  Return target card from your graveyard to your hand.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		int can_play_dirty = count_graveyard(player) && ! graveyard_has_shroud(player);

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XG(2, 1), can_play_dirty, 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		if( ! new_pick_target(&td, "Select target player for Down.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		int ttv = instance->number_of_targets > 0 ? 1 : 0;
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select target creature card for Entering.");
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, ttv) == -1 ){
			spell_fizzled = 1;
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	int can_play_dirty = count_graveyard(player) && ! graveyard_has_shroud(player);

	int priority_down = 5*hand_count[1-player];
	int priority_dirty = 10;

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			int selected = validate_target_from_grave_source(player, card, player, ttv);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_down, MANACOST_XG(2, 1), can_play_dirty, priority_dirty, 1, "Down", "Dirty");
}

int card_far_away(int player, int card, event_t event){
	/*
	  Far |1|U
	  Instant
	  Return target creature to its owner's hand.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Away |2|B
	  Instant
	  Target player sacrifices a creature.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XB(2, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( ! new_pick_target(&td, "Select target creature for Far.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;
		if( ! new_pick_target(&td2, "Select target player for Away.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td2, ttv) ){
				impose_sacrifice(player, card, instance->targets[ttv].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 8, MANACOST_XB(2, 1), can_target(&td2), 10, 1, "Far", "Away");
}

int card_flesh_blood(int player, int card, event_t event){
	/*
	  Flesh |3|B|G
	  Sorcery
	  Exile target creature card from a graveyard. Put X +1/+1 counters on target creature, where X is the power of the card you exiled.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Blood |R|G
	  Sorcery
	  Target creature you control deals damage equal to its power to target creature or player.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		td2=td;
		td2.allowed_controller = player;

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);
		td3.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		int can_play_flesh = count_graveyard_by_type(2, TYPE_CREATURE) && ! graveyard_has_shroud(2) && can_target(&td);
		int can_play_blood = can_target(&td2) && can_target(&td3);

		return generic_split_card(player, card, event, can_play_flesh, 0, MANACOST_GR(1, 1), can_play_blood, 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card for Flesh.");

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		if( new_pick_target(&td, "Select target creature for Flesh.", 0, 1 | GS_LITERAL_PROMPT) ){
			if( select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 1, 2) != -1 ){
				instance->number_of_targets = 1;
			}
			else{
				instance->number_of_targets = 0;
			}
		}
		else{
			spell_fizzled = 1;
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.allowed_controller = player;

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);
		td3.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		int ttv = (instance->info_slot & 1) ? 3 : 0;
		if( new_pick_target(&td2, "Select target creature you control for Blood.", ttv, 1 | GS_LITERAL_PROMPT) ){
			if( ! new_pick_target(&td3, "Select target creature or player for Blood.", ttv+1, 1 | GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 0;
			}
		}
		else{
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	td2=td;
	td2.allowed_controller = player;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);
	td3.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	int can_play_flesh = count_graveyard_by_type(2, TYPE_CREATURE) && ! graveyard_has_shroud(2) && can_target(&td);
	int can_play_blood = can_target(&td2) && can_target(&td3);


	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				int selected = validate_target_from_grave_source(player, card, instance->targets[1].player, 2);
				if( selected != -1 ){
					const int *grave = get_grave(instance->targets[1].player);
					int amount = get_base_power_iid(instance->targets[1].player, grave[selected]);
					rfg_card_from_grave(instance->targets[1].player, selected);
					add_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount);
				}
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = (instance->info_slot & 1) ? 3 : 0;
			if( validate_target(player, card, &td2, ttv) && validate_target(player, card, &td3, ttv+1) ){
				damage_creature(instance->targets[ttv+1].player, instance->targets[ttv+1].card,
								get_power(instance->targets[ttv].player, instance->targets[ttv].card),
								instance->targets[ttv].player, instance->targets[ttv].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_play_flesh, 8, MANACOST_GR(1, 1), can_play_blood, 10, 1, "Flesh", "Blood");
}

int card_give_take(int player, int card, event_t event){
	/*
	  Give |2|G
	  Sorcery
	  Put three +1/+1 counters on target creature.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Take |2|U
	  Sorcery
	  Remove all +1/+1 counters from target creature you control. Draw that many cards.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.allowed_controller = player;
		if( player == AI ){
			td2.special = TARGET_SPECIAL_REQUIRES_COUNTER;
			td2.extra = COUNTER_P1_P1;
			SET_BYTE1(td2.extra)+=2;
		}

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XU(2, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		if( ! new_pick_target(&td, "Select target creature for Give.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.allowed_controller = player;
		if( player == AI ){
			td2.special = TARGET_SPECIAL_REQUIRES_COUNTER;
			td2.extra = COUNTER_P1_P1;
			SET_BYTE1(td2.extra)+=2;
		}
		if( ! new_pick_target(&td2, "Select target creature for Take.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;
	td2.allowed_controller = player;
	if( player == AI ){
		td2.special = TARGET_SPECIAL_REQUIRES_COUNTER;
		td2.extra = COUNTER_P1_P1;
		SET_BYTE1(td2.extra)+=2;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 3);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td2, ttv) ){
				int amount = count_1_1_counters(instance->targets[ttv].player, instance->targets[ttv].card);
				remove_1_1_counters(instance->targets[ttv].player, instance->targets[ttv].card, amount);
				draw_cards(player, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XU(2, 1), can_target(&td2), 8, 1, "Give", "Take");
}


int card_profit_loss(int player, int card, event_t event){
	/*
	  Profit |1|W
	  Instant
	  Creatures you control get +1/+1 until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Loss |2|B
	  Instant
	  Creatures your opponents control get -1/-1 until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_XB(2, 1), 1, 0, 0, NULL, NULL);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1){
			pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
		}
		if( instance->info_slot & 2 ){
			pump_subtype_until_eot(player, card, 1-player, -1, -1, -1, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 8, MANACOST_XB(2, 1), 1, 10, 1, "Profit", "Loss");
}

int card_protect_serve(int player, int card, event_t event){
	/*
	  Protect |2|W
	  Instant
	  Target creature gets +2/+4 until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Serve |1|U
	  Instant
	  Target creature gets -6/-0 until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XU(1, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		if( ! new_pick_target(&td, "Select target creature for Protect.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		if( ! new_pick_target(&td2, "Select target creature for Serve.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 4);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td, ttv) ){
				pump_until_eot(player, card, instance->targets[ttv].player, instance->targets[ttv].card, -6, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 5, MANACOST_XU(1, 1), can_target(&td2), 5, 1, "Protect", "Serve");
}

int card_ready_willing(int player, int card, event_t event){

	/* Ready // Willing
	 * |1|G|W|/|1|W|B
	 * Instant // Instant
	 * Creatures you control gain indestructible until end of turn. Untap each creature you control.
	 * //
	 * Creatures you control gain deathtouch and lifelink until end of turn.
	 * Fuse */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_XBW(1, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1){
			pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
		}
		if( instance->info_slot & 2 ){
			pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_DEATHTOUCH|SP_KEYWORD_LIFELINK, NULL);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 10, MANACOST_XB(2, 1), 1, 8, 1, "Ready", "Willing");
}

int card_toil_trouble(int player, int card, event_t event){
	/*
	  Toil (Toil/Trouble) |2|B
	  Sorcery, 2B
	  Target player draws two cards and loses 2 life.

	  Trouble (Toil/Trouble) |2|R
	  Sorcery
	  Trouble deals damage to target player equal to the number of cards in that player's hand.

	  Fuse (You may cast one or both halves of this card from your hand.)
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XR(2, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.preferred_controller = player;
		if( ! new_pick_target(&td, "Select target player for Toil.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;
		if( ! new_pick_target(&td2, "Select target player for Trouble.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	int priority_toil = (21-(hand_count[player]*3))+life[player];
	int priority_trouble = 5;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( would_validate_arbitrary_target(&td2, 1-player, -1) ){
			priority_trouble = 3*hand_count[1-player];
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				draw_cards(instance->targets[0].player, 2);
				lose_life(instance->targets[0].player, 2);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td, ttv) ){
				damage_player(instance->targets[ttv].player, hand_count[instance->targets[ttv].player], player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_toil, MANACOST_XR(2, 1), can_target(&td2), priority_trouble, 1, "Toil", "Trouble");
}

int card_turn_burn(int player, int card, event_t event){
	/*
	  Turn |2|U
	  Instant
	  Target creature loses all abilities and becomes a 0/1 red Weird until end of turn.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Burn |1|R
	  Instant
	  Burn deals 2 damage to target creature or player.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XR(1, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( ! new_pick_target(&td, "Select target creature for Turn.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		if( ! new_pick_target(&td2, "Select target creature or player for Burn.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				test_definition_t hc;
				default_test_definition(&hc, 0);
				hc.power = 0;
				hc.toughness = 1;
				hc.subtype = SUBTYPE_WEIRD;
				hc.color = COLOR_TEST_RED;
				force_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WEIRD);
				humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 4, &hc);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td2, ttv) ){
				damage_creature(instance->targets[ttv].player, instance->targets[ttv].card, 2, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 5, MANACOST_XR(1, 1), can_target(&td2), 10, 1, "Turn", "Burn");
}

int card_wear_tear(int player, int card, event_t event){
	/*
	  Wear |1|R
	  Instant
	  Destroy target artifact.
	  Fuse (You may cast one or both halves of this card from your hand.)

	  Tear |W
	  Instant
	  Destroy target enchantment.
	  Fuse (You may cast one or both halves of this card from your hand.)
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_W(1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		if( ! new_pick_target(&td, "Select target artifact for Wear.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);
		if( ! new_pick_target(&td2, "Select target enchantment for Tear.", -1, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->number_of_targets-1;
			if( validate_target(player, card, &td2, ttv) ){
				kill_card(instance->targets[ttv].player, instance->targets[ttv].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 8, MANACOST_W(1), can_target(&td2), 10, 1, "Wear", "Tear");
}

// Artifacts

int card_azorius_cluestone(int player, int card, event_t event){
	/* Abzan Banner	|3
	 * Artifact
	 * |T: Add |W, |B, or |G to your mana pool.
	 * |W|B|G, |T, Sacrifice ~: Draw a card. */
	/* Azorius Cluestone	|3
	 * Artifact
	 * |T: Add |W or |U to your mana pool.
	 * |W|U, |T, Sacrifice ~: Draw a card. */
	/* Boros Cluestone	|3
	 * Artifact
	 * |T: Add |R or |W to your mana pool.
	 * |R|W, |T, Sacrifice ~: Draw a card. */
	/* Dimir Cluestone	|3
	 * Artifact
	 * |T: Add |U or |B to your mana pool.
	 * |U|B, |T, Sacrifice ~: Draw a card. */
	/* Golgari Cluestone	|3
	 * Artifact
	 * |T: Add |B or |G to your mana pool.
	 * |B|G, |T, Sacrifice ~: Draw a card. */
	/* Gruul Cluestone	|3
	 * Artifact
	 * |T: Add |R or |G to your mana pool.
	 * |R|G, |T, Sacrifice ~: Draw a card. */
	/* Izzet Cluestone	|3
	 * Artifact
	 * |T: Add |U or |R to your mana pool.
	 * |U|R, |T, Sacrifice ~: Draw a card. */
	/* Jeskai Banner	|3
	 * Artifact
	 * |T: Add |U, |R, or |W to your mana pool.
	 * |U|R|W, |T, Sacrifice ~: Draw a card. */
	/* Mardu Banner	|3
	 * Artifact
	 * |T: Add |R, |W, or |B to your mana pool.
	 * |R|W|B, |T, Sacrifice ~: Draw a card. */
	/* Orzhov Cluestone	|3
	 * Artifact
	 * |T: Add |W or |B to your mana pool.
	 * |W|B, |T, Sacrifice ~: Draw a card. */
	/* Rakdos Cluestone	|3
	 * Artifact
	 * |T: Add |B or |R to your mana pool.
	 * |B|R, |T, Sacrifice ~: Draw a card. */
	/* Selesnya Cluestone	|3
	 * Artifact
	 * |T: Add |G or |W to your mana pool.
	 * |G|W, |T, Sacrifice ~: Draw a card. */
	/* Simic Cluestone	|3
	 * Artifact
	 * |T: Add |G or |U to your mana pool.
	 * |G|U, |T, Sacrifice ~: Draw a card. */
	/* Sultai Banner	|3
	 * Artifact
	 * |T: Add |B, |G, or |U to your mana pool.
	 * |B|G|U, |T, Sacrifice ~: Draw a card. */
	/* Temur Banner	|3
	 * Artifact
	 * |T: Add |G, |U, or |R to your mana pool.
	 * |G|U|R, |T, Sacrifice ~: Draw a card. */

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) && event != EVENT_COUNT_MANA ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( instance->targets[1].player == 66 ){
			return 0;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( instance->targets[1].player == 66 ){
			return 0;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ){
			instance->targets[1].player = 66;
			int cl[5] = {0, 0, 0, 0, 0};
			int i;
			for(i=0; i<5; i++){
				if( cards_data[instance->internal_card_id].color & (1<<(i+1)) ){
					cl[i]++;
				}
			}
			if( has_mana_for_activated_ability(player, card, 0, cl[0], cl[1], cl[2], cl[3], cl[4]) &&
				can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			  ){
				ai_choice = 1;
			}
			instance->targets[1].player = 0;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & draw\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->targets[1].player = 66;
			int cl[COLOR_WHITE + 1] = {0};
			int i;
			for (i = COLOR_BLACK; i <= COLOR_WHITE; ++i){
				if (cards_data[instance->internal_card_id].color & (1 << i)){
					++cl[i];
				}
			}
			charge_mana_for_activated_ability(player, card, cl[0], cl[1], cl[2], cl[3], cl[4], cl[5]);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				 instance->targets[1].player = 0;
			}
		}
		if( choice == 2){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			draw_cards(player, 1);
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

// Lands
static int get_gate_code(int csvid){
	int gates[10] = { CARD_ID_AZORIUS_GUILDGATE, CARD_ID_BOROS_GUILDGATE, CARD_ID_DIMIR_GUILDGATE, CARD_ID_GOLGARI_GUILDGATE,
					CARD_ID_GRUUL_GUILDGATE, CARD_ID_IZZET_GUILDGATE, CARD_ID_ORZHOV_GUILDGATE, CARD_ID_RAKDOS_GUILDGATE,
					CARD_ID_SELESNYA_GUILDGATE, CARD_ID_SIMIC_GUILDGATE };
	int k;
	for (k = 0; k < 10; ++k){
		if (csvid == gates[k]){
			return 1<<k;
		}
	}

	return 0;
}

static int get_global_gate_code(int player){
	int score = 0;
	int c;
	for (c = 0; c < active_cards_count[player]; ++c){
		if (in_play(player, c) && has_subtype(player, c, SUBTYPE_GATE)){
			score |= get_gate_code(get_id(player, c));
		}
	}

	return score;
}

static int count_gates(int player){
	return num_bits_set(get_global_gate_code(player));
}

int card_mazes_end(int player, int card, event_t event){
	/* Maze's End	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |C to your mana pool.
	 * |3, |T, Return ~ to its owner's hand: Search your library for a Gate card, put it onto the battlefield, then shuffle your library. If you control ten or more Gates with different names, you win the game. */

	card_instance_t *instance = get_card_instance( player, card );

	comes_into_play_tapped(player, card, event);

	if (event == EVENT_SHOULD_AI_PLAY){
		int gates = count_gates(player);
		if (player == AI){
			ai_modifier += gates * gates * gates;
		} else if (gates >= 7) {
			ai_modifier -= gates * gates * gates;
		}
		if (gates >= 10){
			lose_the_game(1-player);
		}
	}

	if( event != EVENT_ACTIVATE && event != EVENT_RESOLVE_ACTIVATION ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Tutor a Gate\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			charge_mana_for_activated_ability(player, card, MANACOST_X(3));
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				tap_card(player, card);
				bounce_permanent(player, card);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
		if( choice == 2){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a Gate card.");
			this_test.subtype = SUBTYPE_GATE;
			if( player == HUMAN ){
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
			}
			else{
				int code = get_global_gate_code(player);
				int *deck = deck_ptr[player];
				int gate_array[2][count_deck(player)];
				int gac = 0;
				int count = 0;
				while( deck[count] != -1 ){
						if( has_subtype_by_id(cards_data[deck[count]].id, SUBTYPE_GATE) ){
							if( !( get_gate_code(cards_data[deck[count]].id) & code) ){
								gate_array[0][gac] = deck[count];
								gate_array[1][gac] = count;
								gac++;
							}
						}
						count++;
				}
				int selected = select_card_from_zone(player, player, gate_array[0], gac, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					put_into_play_a_card_from_deck(player, player, gate_array[1][selected]);
				}
			}
			if (count_gates(player) >= 10){
				lose_the_game(1-player);
			}
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

