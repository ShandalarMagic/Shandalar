#include "manalink.h"

static int pw_identity(int player, int card){
	int i;
	card_ptr_t* cp = cards_ptr[get_id(player, card)];
	for (i = 0; i < 6; ++i){
		if (cp->types[i] >= SUBTYPE_MIN_PLANESWALKER_SUBTYPE && cp->types[i] <= SUBTYPE_MAX_PLANESWALKER_SUBTYPE){
			return cp->types[i];
		}
	}
	return -1;
}

int check_for_alternate_identities(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	int identity = pw_identity(player, card);
	if (identity < 0){
		return 0;
	}

	int count, l_count = 0;
	for( count = 0; count < active_cards_count[player]; ++count ){
		if( in_play(player, count) && ! is_what(player, count, TYPE_EFFECT) ){
			if( pw_identity(player, count) == identity ){
				l_count++;
			}
			else{
				state_untargettable(player, count, 1);
			}
		}
	}
	if( l_count > 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		if( select_target(player, card, &td, "Planeswalker Uniqueness Rule: choose a planeswalker to sacrifice.", &(instance->targets[0])) ){
			instance->number_of_targets = 1;
			remove_state(player, -1, STATE_CANNOT_TARGET);	// before killing, in case it causes a trigger that targets something
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_STATE_BASED_ACTION);
		}
	}
	remove_state(player, -1, STATE_CANNOT_TARGET);
	if( l_count > 1 ){
		return 1;
	}
	return 0;
}

static int attacking_planeswalker(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_SET_LEGACY_EFFECT_NAME){
		snprintf(set_legacy_effect_name_addr, 51, "%s", get_card_name_by_id(instance->display_pic_csv_id));
	} else if (event == EVENT_SET_LEGACY_EFFECT_TEXT){
		if (instance->targets[1].player >= 0 && instance->targets[1].card >= 0 && in_play(instance->targets[1].player, instance->targets[1].card)){
			snprintf(set_legacy_effect_name_addr, 399, "Attacking %s with %d loyalty.",
					 get_card_name_by_id(instance->display_pic_csv_id),
					 count_counters(instance->targets[1].player, instance->targets[1].card, COUNTER_LOYALTY));
		} else {
			snprintf(set_legacy_effect_name_addr, 399, "Attacked %s.", get_card_name_by_id(instance->display_pic_csv_id));
		}
	} else if (event == EVENT_TRIGGER && trigger_condition == TRIGGER_END_COMBAT){
		// Just remove immediately instead of putting on the stack, since it's not an actual trigger
		remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_ATTACKING_PWALKER);
		kill_card(player, card, KILL_REMOVE);
	} else {
		card_instance_t* damage = combat_damage_being_dealt(event);
		if (damage &&
			damage->damage_source_player == instance->targets[0].player && damage->damage_source_card == instance->targets[0].card &&
			damage->damage_target_player == 1-instance->targets[0].player && damage->damage_target_card == -1
		   ){
			/* Reattaching the damage card to the planeswalker now, or even doing so as it's created, makes for a better interface and would deal damage
			 * correctly, but would probably confuse damage prevention cards and the AI.  So make the damage card switch targets when it resolves. */
			damage->targets[4] = instance->targets[1];	// struct copy
		}
	}
	return 0;
}

int gideon_jura_force_attack(int player, int card, event_t event);
int fx_attack_gideon_battle_forged(int player, int card, event_t event);

void choose_who_attack(int player, int card){
	int planeswalkers[10][2];
	char buffer[600];
	int pos = scnprintf(buffer, 600, " Attack opponent\n" );
	int ai_choice = 0;
	int count = 0;
	int walker_count = 0;
	int gideon_found = 0;
	int this_pow = get_attack_power(player, card);

	// Must-attack-a-specific-planeswalker effects.
	int gideon_all = (int)(&gideon_jura_force_attack);
	int gideon_single = (int)(&fx_attack_gideon_battle_forged);
	for (count = 0; count < active_cards_count[1-player]; ++count){
		card_instance_t* inst = in_play(1-player, count);
		if (inst && inst->internal_card_id == LEGACY_EFFECT_CUSTOM
			&& (inst->info_slot == gideon_all
				|| (inst->info_slot == gideon_single && inst->damage_target_card == card && inst->damage_target_player == player))
			&& in_play(inst->damage_source_player, inst->damage_source_card)
			&& is_planeswalker(inst->damage_source_player, inst->damage_source_card)
			&& inst->damage_source_player != player
		   ){
			if (!gideon_found){
				gideon_found = 1;
				pos = scnprintf(buffer, 600, " _Attack opponent\n");
			}
			/* This is a bit redundant, since all must-attack-this-planeswalker effects are (AFAIK) on Gideon planeswalkers, and there's nothing that overrides
			 * the planeswalker uniqueness rule; but might as well futureproof. */
			walker_count++;
			planeswalkers[walker_count][0] = inst->damage_source_card;
			planeswalkers[walker_count][1] = count_counters(inst->damage_source_player, inst->damage_source_card, COUNTER_LOYALTY);
			pos += scnprintf(buffer + pos, 600-pos, " Attack %s\n", get_card_name_by_id(get_id(inst->damage_source_player, inst->damage_source_card)));
		}
	}

	// Check for other planeswalkers.
	if (!gideon_found){
		for (count = 0; count < active_cards_count[1-player]; ++count){
			if( in_play(1-player, count) && is_planeswalker(1-player, count) ){
				walker_count++;
				planeswalkers[walker_count][0] = count;
				planeswalkers[walker_count][1] = count_counters(1-player, count, COUNTER_LOYALTY);
				pos += scnprintf(buffer + pos, 600-pos, " Attack %s\n", get_card_name_by_id(get_id(1-player, count)));
			}
		}
	}

	// AI part, choose to attack a Planeswalker or not.
	if (!IS_AI(player)){
		ai_choice = gideon_found ? 1 : 0;
	} else if( gideon_found || life[1-player]-this_pow > 6 ){ // If not forced to attack a planewalker and we could put the opponent in the "Red Zone", do so
		// First step: check if there are other creatures attacking Planeswalkers
		for (count = 0; count < active_cards_count[player]; ++count){
				if( in_play(player, count) && is_what(player, count, TYPE_EFFECT) ){
					card_instance_t *instance = get_card_instance(player, count);
					if( instance->info_slot == (int)attacking_planeswalker ){
						planeswalkers[instance->targets[2].card][1]-=get_attack_power(instance->damage_target_player, instance->damage_target_card);
					}
				}
		}
		//Second step: choose a planeswalker this creature can kill
		for(count = 1; count <= walker_count; count++){
			if( planeswalkers[count][1] - this_pow < 1 ){
				planeswalkers[count][1] = 0;
				ai_choice = count;
				break;
			}
		}
		if( ai_choice == 0 ){
			//Third step: if we can't deal enough damage to kill a Planeswalker with this attack, choose the one with the highest Loyalty
			int par = 1;
			for(count = 1; count <= walker_count; count++){
				if( planeswalkers[count][1] > par ){
					par = planeswalkers[count][1];
					ai_choice = count;
					break;
				}
			}
		}
	}

	int choice = ai_choice;
	if( walker_count > 0 && !gideon_found ){
		choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
	}

	if( choice > 0 ){
		set_special_flags(player, card, SF_ATTACKING_PWALKER);
		int legacy = create_targetted_legacy_effect(player, card, &attacking_planeswalker, player, card);
		set_special_flags(player, legacy, SF_ATTACKING_PWALKER);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].player = 1-player;
		leg->targets[1].card = planeswalkers[ choice ][0];
		leg->targets[2].player = 777;
		leg->targets[2].card = choice;
		leg->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME | EVENT_SET_LEGACY_EFFECT_TEXT;
		/* Change effect display to the planeswalker by brute force - legacy_name() doesn't get called if we just call
		 * create_targeted_legacy_effect(1-player, planeswalkers[choice], &attacking_planeswalker, player, card) above and I'm not sure why yet. */
		leg->display_pic_csv_id = get_id(1 - player, planeswalkers[choice][0]);
		leg->display_pic_num = get_card_image_number(leg->display_pic_csv_id, 1 - player, planeswalkers[choice][0]);
	}
}

int planeswalker(int player, int card, event_t event, int loyalty){

	card_instance_t *instance = get_card_instance(player, card);

	// if somehow this is not a planeswalker, bail out
	card_data_t* card_d = get_card_data(player, card);
	if( card_d->cc[2] != 9 && event != EVENT_CAN_CAST ){
		return 0;
	}

	// comes into play with some loyalty
	enters_the_battlefield_with_counters(player, card, event, COUNTER_LOYALTY, loyalty);

	// die if duplicate planeswalker
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		check_for_alternate_identities(player, card);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[9].player = 2;
	}

	// die if 0 loyalty
	if( event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_LOYALTY) == 0 && BYTE0(instance->targets[9].player) == 2 ){
		SET_SBYTE0(instance->targets[9].player) = 3;
		kill_card(player, card, KILL_STATE_BASED_ACTION);
		return 0;
	}

	if( event == EVENT_SHOULD_AI_PLAY ){
		ai_modifier += 24 * count_counters(player, card, COUNTER_LOYALTY);
		return should_ai_play(player, card);
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	// once per turn... sorcery only
	if( event == EVENT_CAN_ACTIVATE && instance->targets[9].card != 1 ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( can_sorcery_be_played(player, event) || (player_bits[player] & PB_CAN_USE_PW_ABILITIES_AS_INSTANT) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if (spell_fizzled != 1){
			instance->targets[9].card = 1;

			int cost = SBYTE1(instance->targets[9].player);
			if (cost){
				add_counters_as_cost(player, card, COUNTER_LOYALTY, cost);
			}
			SET_SBYTE1(instance->targets[9].player) = 0;

		}
	}

	if( event == EVENT_CAN_SKIP_TURN ){
		instance->targets[9].card = 0;
	}

	// when creatures attack, prompt for who they attack
	if( trigger_condition == TRIGGER_PAY_TO_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS && current_turn == 1-player){
		if( affect_me( trigger_cause_controller, trigger_cause ) && reason_for_trigger_controller == affected_card_controller ){
			if(event == EVENT_TRIGGER ){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					choose_who_attack(affected_card_controller, affected_card);
			}
		}
	}

	/* When damage is dealt to a player, choose to hit the walker instead.
	 *
	 * Ideally, this should cause the damage card to reattach to the planeswalker and then resolve normally, so it can be prevented.  All the effects that look
	 * at damage, though, assume damage_target_card != -1 is a creature.  Damage that *does* get attached to a planeswalker *does* resolve normally; currently
	 * this can only happen with Fated Conflagration. */
	card_instance_t* damage = noncombat_damage_being_prevented(event);	// sic
	if( damage && damage->damage_source_player != player &&
		damage->damage_target_player == player && damage->damage_target_card == -1 &&
		in_play(player, card) && !check_battlefield_for_id(player, CARD_ID_PALISADE_GIANT)
	  ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_planeswalker(player, count) ){
					char buffer[100];
					snprintf(buffer, 100, " Damage this PW (%d loyalty)\n Do not damage this PW", count_counters(player, count, COUNTER_LOYALTY));
					int ai_choice = 0;
					if( damage->info_slot >= life[player] ){
						ai_choice = 1;
					}
					int choice = do_dialog(1-player, player, count, -1, -1, buffer, ai_choice);
					if( choice == 0 ){
						int dmg = damage->info_slot;
						damage->info_slot = 0;
						play_sound_effect(WAV_DAMAGE);
						remove_counters(player, count, COUNTER_LOYALTY, dmg);

#if 0
						damage->damage_target_card = card;	// Not yet robust enough
#endif
						/* Cards are recalculated (and thus static effects run) after spell resolution, but before damage resolves, so check again here -
						 * http://www.slightlymagic.net/forum/viewtopic.php?t=13968 */
						if (count_counters(player, count, COUNTER_LOYALTY) == 0 && BYTE0(get_card_instance(player, count)->targets[9].player) == 2){
							SET_SBYTE0(get_card_instance(player, count)->targets[9].player) = 3;
							kill_card(player, count, KILL_STATE_BASED_ACTION);
							return 0;
						}
						break;
					}
				}
				count--;
		}
	}

	// reset the flag for "fog"	effects
	if( check_special_flags(player, card, SF_WONT_RECEIVE_DAMAGE) && eot_trigger(player, card, event) ){
		remove_special_flags(player, card, SF_WONT_RECEIVE_DAMAGE);
	}
	return 0;
}


