#include "manalink.h"

// ------------ General functions

int tempest_painland(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	return generic_painland(player, card, event);
}


static int tempest_medallion(int player, int card, event_t event, int clr){
	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && (get_color(affected_card_controller, affected_card) & clr) ){
			COST_COLORLESS--;
		}
	}

   return 0;
}

int buyback(int player, int card, int colorless, int black, int blue, int green, int red, int white){
	if( ! is_token(player, card) ){
		int diff = 0;
		if( ! played_for_free(player, card) ){
			diff = MIN(0, cards_ptr[get_id(player, card)]->req_colorless+true_get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, -1));
		}
		diff-=(2*count_cards_by_id(player, CARD_ID_MEMORY_CRYSTAL));
		int cless = MAX(0, colorless+diff);
		if( has_mana_multi(player, cless, black, blue, green, red, white) ){
			int choice = do_dialog(player, player, card, -1, -1, " Buyback\n Play normally", 0);
			if( choice == 0 ){
				charge_mana_multi(player, cless, black, blue, green, red, white);
				if( spell_fizzled != 1 ){
					return 1;
				}
			}
		}
	}
	return 0;
}

int generic_licid(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int pow, int tou, int key, int s_key,
					int pref_controller
  ){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && instance->targets[2].card > 0 ){
		event_result = instance->targets[2].card;
	}

	if( ! is_what(player, card, TYPE_CREATURE) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, 0, black, blue, green, red, white, 0, 0, 0);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->targets[2].card = instance->damage_target_player = instance->damage_target_card = -1;
			parent->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
			get_abilities(instance->parent_controller, instance->parent_card, EVENT_CHANGE_TYPE, -1);
		}
		return generic_aura(player, card, event, player, pow, tou, key, s_key, 0, 0, 0);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = pref_controller;

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, colorless, black, blue, green, red, white, 0,
										&td, "TARGET_CREATURE");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, colorless, black, blue, green, red, white, 0,
										&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			int newtype = create_a_card_type(parent->internal_card_id);
			cards_at_7c7000[newtype]->type = TYPE_ENCHANTMENT;
			cards_at_7c7000[newtype]->power = 0;
			cards_at_7c7000[newtype]->toughness = 0;
			parent->targets[2].card = newtype;
			attach_aura_to_target(instance->parent_controller, instance->parent_card, event, instance->targets[0].player, instance->targets[0].card);
			parent->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
			get_abilities(instance->parent_controller, instance->parent_card, EVENT_CHANGE_TYPE, -1);
		}
	}

	return 0;
}

const char* activating_sliver(int who_chooses, int player, int card){
	if(	can_use_activated_abilities(player, card) ){
		return NULL;
	}
	return "this Sliver cannot use activated abilities.";
}

int sliver_shared_shade_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_SLIVER;
		td.extra = (int32_t)activating_sliver;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.illegal_abilities = 0;

		if( event == EVENT_CAN_ACTIVATE ){
			int cc;
			for(cc=0; cc < active_cards_count[player]; cc++){
				if( in_play(player, cc) && is_what(player, cc, TYPE_PERMANENT) && has_subtype(player, cc, SUBTYPE_SLIVER) ){
					if( generic_activated_ability(player, cc, EVENT_CAN_ACTIVATE, 0, MANACOST_X(2), 0, NULL, NULL) ){
						return 1;
					}
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			char buffer[100];
			scnprintf(buffer, 100, "%s: select a Sliver to activate.", cards_ptr[get_id(p, c)]->name);
			if( new_pick_target(&td, buffer, 0, 1 | GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 1;
				if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) ){
					charge_mana_for_activated_ability(instance->targets[0].player, instance->targets[0].card, MANACOST_X(2));
				}
				else{
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pow = instance->targets[2].player > 0 ? instance->targets[2].player : 0;
			int tou = instance->targets[2].card > 0 ? instance->targets[2].card : 0;
			int key = instance->targets[3].player > 0 ? instance->targets[3].player : 0;
			int s_key = instance->targets[3].card > 0 ? instance->targets[3].card : 0;
			if( ! key && ! s_key ){
				pump_until_eot(instance->targets[0].player, instance->targets[0].card, instance->targets[0].player, instance->targets[0].card, pow, tou);
			}
			else{
				pump_ability_until_eot(instance->targets[0].player, instance->targets[0].card, instance->targets[0].player, instance->targets[0].card,
										pow, tou, key, s_key);
			}
		}
	}

	return 0;
}

int sliver_with_shared_shade_ability(int player, int card, event_t event, int pow, int tou, int key, int s_key){

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &sliver_shared_shade_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->targets[2].player = pow;
		instance->targets[2].card = tou;
		instance->targets[3].player = key;
		instance->targets[3].card = s_key;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &sliver_shared_shade_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->targets[2].player = pow;
		instance->targets[2].card = tou;
		instance->targets[3].player = key;
		instance->targets[3].card = s_key;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int shared_sliver_activated_ability(int player, int card, event_t event, int mode, int cless, int black, int blue, int green, int red, int white,
									uint32_t variable_costs, target_definition_t *td2, const char *prompt){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_SLIVER;
	td.extra = (int32_t)activating_sliver;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.illegal_state = (mode & GAA_UNTAPPED) ? TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK : 0;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && in_play(player, card) ){
		int cc;
		for(cc=0; cc < active_cards_count[player]; cc++){
			if( in_play(player, cc) && is_what(player, cc, TYPE_PERMANENT) && has_subtype(player, cc, SUBTYPE_SLIVER) ){
				int result = generic_activated_ability(player, cc, EVENT_CAN_ACTIVATE, mode, cless, black, blue, green, red, white, variable_costs, td2, prompt);
				if( result ){
					return result;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( new_pick_target(&td, "Select a Sliver to activate", 0, 1 | GS_LITERAL_PROMPT) ){
			instance->number_of_targets = 1;
			if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) ){
				generic_activated_ability(instance->targets[0].player, instance->targets[0].card, event,
											mode, cless, black, blue, green, red, white, variable_costs, td2, prompt);
				if( spell_fizzled != 1 ){
					instance->targets[2] = get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0];
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

//--------------- Cards

int card_abandon_hope(int player, int card, event_t event){

	/* Abandon Hope	|X|1|B
	 * Sorcery
	 * As an additional cost to cast ~, discard X cards.
	 * Look at target opponent's hand and choose X cards from it. That player discards those cards. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int original_hand[2][hand_count[player]];
			int ohc = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) ){
					original_hand[0][ohc] = get_original_internal_card_id(player, i);
					original_hand[1][ohc] = i;
					ohc++;
				}
			}
			int to_discard[hand_count[player]];
			int tdc = 0;
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
			while( ohc && has_mana(player, COLOR_COLORLESS, tdc+1) ){
					int selected = select_card_from_zone(player, player, original_hand[0], ohc, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						to_discard[tdc] = original_hand[1][selected];
						tdc++;
						for(i=selected; i<ohc; i++){
							original_hand[0][i] = original_hand[0][i+1];
							original_hand[1][i] = original_hand[1][i+1];
						}
						ohc--;
					}
					else{
						break;
					}
			}
			if( tdc ){
				charge_mana(player, COLOR_COLORLESS, tdc);
				if( spell_fizzled != 1 ){
					for(i=0; i<tdc; i++){
						discard_card(player, to_discard[i]);
					}
					instance->info_slot = tdc;
				}
			}
		}
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.qty = instance->info_slot;

			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_advance_scout(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_W(1), 0, &td, "TARGET_CREATURE");
}

int card_aftershock(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_player(player, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target artifact, creature or land.", 1, NULL);
}

int card_altar_of_dementia(int player, int card, event_t event){

	/* Altar of Dementia	|2
	 * Artifact
	 * Sacrifice a creature: Target player puts a number of cards equal to the sacrificed creature's power from the top of his or her library into his or her
	 * graveyard. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, instance->info_slot);
		}
	}

	return 0;
}

static int aluren_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_what(player, card, TYPE_EFFECT) && instance->targets[0].player > -1 ){
		if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_what(player, i, TYPE_CREATURE) && get_cmc(player, i) <= 3 ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with CMC <= 3 to play.");
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			play_card_in_hand_for_free(player, selected);
			cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
		}
		else{
			spell_fizzled = 1;
		}
	}
	return 0;
}

int card_aluren(int player, int card, event_t event){
	/*
	  Aluren |2|G|G
	  Enchantment
	  Any player may play creature cards with converted mana cost 3 or less without paying their mana cost and as though they had flash.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &aluren_effect);
		card_instance_t *instance = get_card_instance(1-player, legacy);
		instance->targets[0].player = player;
		instance->targets[0].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return aluren_effect(player, card, event);
	}

	return global_enchantment(player, card, event);
}

int card_ancient_runes(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(current_turn, count_subtype(current_turn, TYPE_ARTIFACT, -1), player, card);
	}

	return global_enchantment(player, card, event);
}

int card_ancient_tomb(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		damage_player(player, 2, instance->parent_controller, instance->parent_card);
	}
	return two_mana_land(player, card, event, COLOR_COLORLESS, COLOR_COLORLESS);
}

int card_anoint(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		 return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_DAMAGE") ){
			instance->info_slot = buyback(player, card, 3, 0, 0, 0, 0, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot < 3 ){
				target->info_slot = 0;
			}
			else{
				target->info_slot-=3;
			}
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_apocalypse(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);
		discard_all(player);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_armord_sliver(int player, int card, event_t event){
	/*
	  Armor Sliver |2|W
	  Creature - Sliver 2/2
	  All Sliver creatures have "{2}: This creature gets +0/+1 until end of turn."
	*/
	return sliver_with_shared_shade_ability(player, card, event, 0, 1, 0, 0);
}

int card_auratog(int player, int card, event_t event){
	/*
	  Auratog English |1|W
	  Creature - Atog 1/2
	  Sacrifice an enchantment: Auratog gets +2/+2 until end of turn.
	*/

	if (!IS_GAA_EVENT(event))
		return 0;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ENCHANTMENT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( ! sacrifice(player, card, player, 0, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}
	return 0;
}

int card_barbed_sliver(int player, int card, event_t event){
	/*
	  Barbed Sliver |2|R
	  Creature - Sliver 2/2
	  All Sliver creatures have "{2}: This creature gets +1/+0 until end of turn."
	*/
	return sliver_with_shared_shade_ability(player, card, event, 1, 0, 0, 0);
}

int card_blood_frenzy(int player, int card, event_t event){

	if (event == EVENT_CHECK_PUMP ){
		if( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id)){
			pumpable_power[player] += 4;
		}
	}

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && current_phase == PHASE_AFTER_BLOCKING ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking or blocking creature", 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, 4, 0, 0, 0);
			pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
			pump.eot_removal_method = KILL_DESTROY;
			pump_ability(player, card, instance->targets[0].player, instance->targets[0].card, &pump);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bottle_gnomes(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 3);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_broken_fall(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_BOUNCE_ME | GAA_REGENERATION, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_capsize(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PERMANENT") ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_carrionette(int player, int card, event_t event){
	/* Carrionette	|1|B
	 * Creature - Skeleton 1/1
	 * |2|B|B: Exile ~ and target creature unless that creature's controller pays |2. Activate this ability only if ~ is in your graveyard. */

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XB(2, 2)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		if( can_target(&td) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XB(2, 2));
		if( spell_fizzled != 1){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;

		card_instance_t *instance= get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			charge_mana(instance->targets[0].player, COLOR_COLORLESS, 2);
			if( spell_fizzled == 1 ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int card_choke(int player, int card, event_t event){
	/*
	  Choke |2|G
	  Enchantment
	  Islands don't untap during their controllers' untap steps.
	*/

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*(count_subtype(1-player, TYPE_LAND, SUBTYPE_ISLAND)-count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND));
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP ){
		if( has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_ISLAND)) ){
			does_not_untap(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

const char* sliver_to_regenerate(int who_chooses, int player, int card){
	if(	can_use_activated_abilities(player, card) ){
		if( who_chooses == HUMAN || (who_chooses == AI && can_regenerate(player, card)) ){
			return NULL;
		}
	}
	return "this Sliver cannot regenerate.";
}

int clot_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_DESTROYED;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_SLIVER;
		td.extra = (int32_t)sliver_to_regenerate;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.illegal_abilities = 0;

		if( event == EVENT_CAN_ACTIVATE && in_play(p, c) ){
			if( land_can_be_played & LCBP_REGENERATION ){
				int pp, cc;
				for(pp = 0; pp<2; pp++){
					for(cc=0; cc < active_cards_count[pp]; cc++){
						if( in_play(pp, cc) && is_what(pp, cc, TYPE_PERMANENT) && has_subtype(pp, cc, SUBTYPE_SLIVER) ){
							int result = regeneration(pp, cc, EVENT_CAN_ACTIVATE, MANACOST_X(2));
							if( result ){
								return result;
							}
						}
					}
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( new_pick_target(&td, "Select a Sliver to regenerate.", 0, 1 | GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 1;
				if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) ){
					charge_mana_for_activated_ability(instance->targets[0].player, instance->targets[0].card, MANACOST_X(2));
				}
				else{
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_clot_sliver(int player, int card, event_t event){
	/*
	  Clot Sliver |1|B
	  Creature - Sliver 1/1
	  All Slivers have "{2}: Regenerate this permanent."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &clot_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &clot_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}
	return slivercycling(player, card, event);
}

int card_cloudchaser_eagle(int player, int card, event_t event){
	/*
	  Cloudchaser Eagle |3|W
	  Creature - Bird 2/2
	  Flying
	  When Cloudchaser Eagle enters the battlefield, destroy target enchantment.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		td.allow_cancel = 0;


		if( can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT") ){
			action_on_target(player, card, 0, KILL_DESTROY);
			get_card_instance(player, card)->number_of_targets = 0;
		}
	}

	return 0;
}

int cq_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int cq_player = instance->targets[1].player;
		int cq_card = instance->targets[1].card;
		if( ! is_tapped(cq_player, cq_card) || leaves_play(cq_player, cq_card, event) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_coffin_queen(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	if (event == EVENT_CAN_ACTIVATE){
		return (generic_activated_ability(player, card, event, 0, MANACOST_XB(2,1), 0, NULL, NULL) &&
				any_in_graveyard_by_type(2, TYPE_CREATURE) && !graveyard_has_shroud(2));
	}

	if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_XB(2,1))){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");

		select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &test, 2, 3);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		int selected = validate_target_from_grave_source(player, card, instance->targets[2].player, 3);
		if (selected != -1){
			int zombie = reanimate_permanent(player, card, instance->targets[2].player, selected, REANIMATE_DEFAULT | REANIMATE_NO_CONTROL_LEGACY);

			if( zombie > -1 ){
				int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &cq_effect, player, zombie);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[1].player = instance->parent_controller;
				leg->targets[1].card = instance->parent_card;
				leg->number_of_targets = 2;
				card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
				parent->targets[1] = instance->targets[0];
			}
		}
	}

	return 0;
}

int card_cold_storage(int player, int card, event_t event){
	/*
	  Cold Storage |4
	  Artifact
	  {3}: Exile target creature you control.
	  Sacrifice Cold Storage: Return each creature card exiled with Cold Storage to the battlefield under your control.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	if( player == AI ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	enum{
		CHOICE_EXILE = 1,
		CHOICE_RETURN,
		CHOICE_SHOW_EXILED
	};

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == AI ){
			if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_REGENERATION, MANACOST_X(3), 0, &td, "TARGET_CREATURE") ){
				return 0x63;
			}
		}
		if( player == HUMAN ){
			if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE") ){
				return 1;
			}
			if( exiledby_count(player, card, ANYBODY) ){
				return 1;
			}
		}
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE){
		instance->info_slot = instance->number_of_targets = 0;
		int abils[3] = {
						(player == AI ? generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_REGENERATION, MANACOST_X(3), 0, &td, NULL) :
						generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_X(3), 0, &td, NULL)),
						generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL),
						player == HUMAN && exiledby_count(player, card, ANYBODY)
						};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Exile creature", abils[0], 10,
							"Sac & return creature", abils[1], (instance->targets[1].player > -1 ? instance->targets[1].player*5 : 0),
							"Show exiled creatures", abils[2], 1);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_EXILE ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
				pick_target(&td, "TARGET_CREATURE");
			}
		}
		if( choice == CHOICE_RETURN ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
				instance->targets[1].card = exiledby_detach(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		if( choice == CHOICE_SHOW_EXILED ){
			exiledby_choose(player, card, CARD_ID_COLD_STORAGE, EXBY_CHOOSE, 0, "Creature", 0);
			spell_fizzled = 1;
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == CHOICE_EXILE ){
			if( valid_target(&td) ){
				if( player == AI ){
					regenerate_target(instance->targets[0].player, instance->targets[0].card);
				}
				exile_card_and_remember_it_on_exiledby(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
				int prev = get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].player;
				get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].player = (prev != -1 ? prev+1 : 1);
			}
		}

		if( instance->info_slot == CHOICE_RETURN ){
			int leg = 0;
			int idx = 0;
			int* loc;

			while ((loc = exiledby_find_any(player-2, instance->targets[1].card, &leg, &idx)) != NULL){
					int owner = (*loc & 0x80000000) ? 1 : 0;
					int iid = *loc & ~0x80000000;
					*loc = -1;
					int card_added = add_card_to_hand(player, iid);
					if( owner != player ){
						if( player == AI ){
							remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
						}
						else{
							add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
						}
					}
					put_into_play(player, card_added);
					if( owner != player ){
						create_targetted_legacy_effect(player, card, &empty, player, card_added);
					}
			}
		}
	}

	return 0;
}

int card_commander_greven_il_vec(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	fear(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( count_permanents_by_type(player, TYPE_CREATURE) < 1 ){
			ai_modifier-=128;
		}
	}

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_corpse_dance(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, MANACOST_X(2));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(player);
		int count = count_graveyard(player)-1;
		while( count > -1 ){
				if( is_what(-1, grave[count], TYPE_CREATURE) ){
					reanimate_permanent(player, card, player, count, REANIMATE_HASTE_AND_EXILE_AT_EOT);
					break;
				}
				count--;
		}
		if( instance->info_slot == 1 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_crazed_armodon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_t pump;
		default_pump_ability_definition(instance->parent_controller, instance->parent_card, &pump, 3, 0, KEYWORD_TRAMPLE, 0);
		pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
		pump.eot_removal_method = KILL_DESTROY;
		pump_ability(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, &pump);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_G(1), 0, NULL, NULL);
}

int card_magus_of_the_scroll(int player, int card, event_t event);

int card_cursed_scroll(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 6) ){ return 0; }

	return card_magus_of_the_scroll(player, card, event);
}

int card_darkling_stalker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
		if( ( land_can_be_played & LCBP_REGENERATION) && can_regenerate(player, card) ){
			return 0x63;
		}
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_B(1));
		if( spell_fizzled != 1 ){
			if( ( land_can_be_played & LCBP_REGENERATION) && can_regenerate(player, card) ){
				instance->info_slot = 66;
			}
			else{
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 1);
		}
	}

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 1, 0, 0);
	}

	return 0;
}

int card_dauthi_embrace(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_SHADOW);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_B(2), 0, &td, "TARGET_CREATURE");
}

int card_dauthi_ghoul(int player, int card, event_t event){

	shadow(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			check_for_special_ability(affected_card_controller, affected_card, SP_KEYWORD_SHADOW)
		  ){
			count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *instance = get_card_instance(player, card);
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_dauthi_horror(int player, int card, event_t event){
	shadow(player, card, event);
	if(event == EVENT_BLOCK_LEGALITY ){
		if(player == attacking_card_controller && card == attacking_card ){
			if (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE) ){
				event_result = 1;
			}
		}
	}
	return 0;
}

int card_dauthi_marauder(int player, int card, event_t event){
	shadow(player, card, event);
	return 0;
}

int card_dauthi_mercenary(int player, int card, event_t event){
	shadow(player, card, event);
	return generic_shade(player, card, event, 0, MANACOST_XB(1, 1), 1, 0, 0, 0);
}

int card_dauthi_mindripper(int player, int card, event_t event){

   shadow(player, card, event);

   if( ! is_humiliated(player, card) ){
		if( event == EVENT_CAN_ACTIVATE && is_attacking(player, card) && is_unblocked(player, card) && current_phase == PHASE_AFTER_BLOCKING){
			return 1;
		}

		if( event == EVENT_ACTIVATE){
			kill_card(player, card, KILL_SACRIFICE);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			new_multidiscard(1-player, 3, 0, player);
		}
	}

	return 0;
}


int card_dauthi_slayer(int player, int card, event_t event){
	shadow(player, card, event);
	return attack_if_able(player, card, event);
}

int card_death_pits_of_rath(int player, int card, event_t event){
	/*
	  Death Pits of Rath |3|B|B
	  Enchantment
	  Whenever a creature is dealt damage, destroy it. It can't be regenerated.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card != -1 ){
					if( is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE) ){
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
							instance->targets[instance->info_slot].player = damage->damage_target_player;
							instance->targets[instance->info_slot].card = damage->damage_target_card;
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
						if( in_play(instance->targets[i].player, instance->targets[i].card) ){
							kill_card(instance->targets[i].player, instance->targets[i].card, KILL_BURY);
						}
					}
					instance->info_slot = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_diabolic_edict(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);

}

int card_dirtcowl_wurm(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player
			&& trigger_cause_controller == 1-player
			&& is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
			&& ! check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST) //Bad name in this case, but it's the one used by "put_into_play"
			)
		{
			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			if (event == EVENT_RESOLVE_TRIGGER){
				add_1_1_counter(player, card);
			}
		}
	}
	return 0;
}

int card_dismiss2(int player, int card, event_t event){
	/*
	  Dismiss |2|U|U
	  Instant
	  Counter target spell.
	  Draw a card.
	*/
	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_disturbed_burial(int player, int card, event_t event)
{
  // Buyback |3
  // Return target creature card from your graveyard to your hand.

  if (event == EVENT_CAN_CAST)
	return any_in_graveyard_by_type(player, TYPE_CREATURE) && !graveyard_has_shroud(2);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");

	  if (new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &test, 0) == -1)
		spell_fizzled = 1;
	  else
		get_card_instance(player, card)->info_slot = buyback(player, card, MANACOST_X(3));
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int selected = validate_target_from_grave(player, card, player, 0);
	  if (selected != -1)
		{
		  from_grave_to_hand(player, selected, TUTOR_HAND);
		  if (get_card_instance(player, card)->info_slot)
			{
			  bounce_permanent(player, card);
			  return 0;
			}
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_dracoplasm(int player, int card, event_t event){

   card_instance_t *instance = get_card_instance(player, card);

   if( event == EVENT_RESOLVE_SPELL ){
	   target_definition_t td;
	   default_target_definition(player, card, &td, TYPE_CREATURE);
	   td.preferred_controller = player;
	   td.allowed_controller = player;
	   td.illegal_abilities = 0;

	   add_state(player, card, STATE_OUBLIETTED);
	   instance->targets[1].player = 0;
	   instance->targets[1].card = 0;

	   test_definition_t test;
	   new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
	   while( can_target(&td) ){
		   int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		   if (!sac){
			   break;
		   }
		   instance->targets[1].player += get_power(BYTE2(sac), BYTE3(sac));
		   instance->targets[1].card += get_toughness(BYTE2(sac), BYTE3(sac));
		   kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	   }
	   remove_state(player, card, STATE_OUBLIETTED);
   }

   if( event == EVENT_POWER && affect_me(player, card) ){
	   event_result += (instance->targets[1].player > -1 ? instance->targets[1].player : 0);
   }

   if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
	   event_result += (instance->targets[1].card > -1 ? instance->targets[1].card : 0);
   }

	return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
}

int card_dregs_of_sorrow(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int trgs = 0;
			instance->number_of_targets = 0;
			while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs+1) ){
					if( new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%s creature", COLOR_BLACK), trgs, GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}

			if( trgs > 0 ){
				int k;
				for(k=0; k<trgs; k++){
					state_untargettable(instance->targets[k].player, instance->targets[k].card, 0);
				}
				charge_mana(player, COLOR_COLORLESS, trgs);
				if( spell_fizzled != 1 ){
					instance->info_slot = trgs;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int k;
			int good = 0;
			for(k=0; k<instance->info_slot; k++){
				if( validate_target(player, card, &td, k) ){
					kill_card(instance->targets[k].player, instance->targets[k].card, KILL_DESTROY);
					good++;
				}
			}
			if( good > 0 ){
				draw_cards(player, instance->info_slot);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_earthcraft(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_BASIC;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
		return can_target(&td1);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int creature_to_tap = -1;
			if( new_pick_target(&td, "Select a creature to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
				add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
				creature_to_tap = instance->targets[0].card;
				instance->number_of_targets = 0;
				if( new_pick_target(&td1, "Select a basic land to untap.", 0, 1 | GS_LITERAL_PROMPT) ){
					tap_card(player, creature_to_tap);
				}
				remove_state(player, creature_to_tap, STATE_TARGETTED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			untap_card( instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_echo_chamber(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;
	td.who_chooses = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			token.legacy = 1;
			token.special_code_for_legacy = &haste_and_remove_eot;
			generate_token(&token);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_CAN_SORCERY_BE_PLAYED, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

int vineyard_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 66;
	}

	if( in_play(player, card) && current_phase == PHASE_MAIN1 && instance->info_slot != 66 && ! is_humiliated(player, card) ){
		produce_mana(current_turn, COLOR_GREEN, 2);
		instance->info_slot = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_eladamris_vineyard(int player, int card, event_t event){

	vineyard_effect(player, card, event);

	return global_enchantment(player, card, event);
}

int card_eladamri_lord_of_leaves(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	boost_creature_type(player, card, event, SUBTYPE_ELF, 0, 0, KEYWORD_SHROUD | get_hacked_walk(player, card, KEYWORD_FORESTWALK), 0);

	return 0;
}

int card_elite_javelineers(int player, int card, event_t event){
	/*
	  Elite Javelineer |2|W
	  Creature - Human Soldier 2/2
	  Whenever Elite Javelineer blocks, it deals 1 damage to target attacking creature.
	*/

	if( blocking(player, card, event) && ! is_humiliated(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_ATTACKING;
		td.allow_cancel = 0;
		if( can_target(&td) && new_pick_target(&td, "Select target attacking creature.", 0, GS_LITERAL_PROMPT) ){
			damage_target0(player, card, 1);
		}
		get_card_instance(player, card)->number_of_targets = 0;
	}

	return 0;
}


int card_elven_warhounds(int player, int card, event_t event)
{
  // Whenever ~ becomes blocked by a creature, put that creature on top of its owner's library.
  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && current_turn == player && !is_humiliated(player, card))
	get_card_instance(player, card)->destroys_if_blocked |= DIFB_DESTROYS_ALL;

  if (event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && !is_humiliated(player, card))
	{
	  char marked[2][151] = {{0}};
	  mark_each_creature_blocking_me(player, card, marked);
	  int c;
	  for (c = 0; c < active_cards_count[1-current_turn]; ++c)
		if (marked[1-current_turn][c] && in_play(1-current_turn, c))
		  put_on_top_of_deck(1-current_turn, c);
	}

  return 0;
}

int card_elvish_fury(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = buyback(player, card, MANACOST_X(4));
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_emerald_medallion(int player, int card, event_t event){

	return tempest_medallion(player, card, event, COLOR_TEST_GREEN);
}

int card_emessi_tome(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
		discard(player, 0, player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL);
}

int card_essence_bottle(int player, int card, event_t event)
{
  /* Essence Bottle	|2
   * Artifact
   * |3, |T: Put an elixir counter on ~.
   * |T, Remove all elixir counters from ~: You gain 2 life for each elixir counter removed this way. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !(CAN_TAP(player, card) && can_use_activated_abilities(player, card)))
		return 0;

	  enum
	  {
		CHOICE_COUNTER = 1,
		CHOICE_LIFE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Add a counter", 1, 3, DLG_MANA(MANACOST_X(3)),
						"Gain life", 1, count_counters(player, card, COUNTER_ELIXIR), DLG_MANA(MANACOST_X(0)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_LIFE:
				instance->state |= STATE_TAPPED;
				instance->eot_toughness = count_counters(player, card, COUNTER_ELIXIR);
				remove_all_counters(player, card, COUNTER_ELIXIR);
				ai_modifier -= 32 + 2 * life[player];
				break;

			  case CHOICE_COUNTER:
				instance->state |= STATE_TAPPED;
				if (current_turn != player && current_phase == PHASE_DISCARD)
				  ai_modifier += 64;
				break;
			}
		}
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_COUNTER:
			  add_counter(player, card, COUNTER_ELIXIR);
			  break;

			case CHOICE_LIFE:
			  gain_life(player, 2 * get_card_instance(player, card)->eot_toughness);
			  break;
		  }
	}

  return 0;
}

int card_energizer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( player == AI && current_turn != player && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) &&
		eot_trigger(player, card, event)
	  ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(2));
		if( spell_fizzled != 1 ){
			tap_card(player, card);
			add_1_1_counter(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_escaped_shapeshifter(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		int keyword_mask = 	KEYWORD_FLYING | KEYWORD_FIRST_STRIKE | KEYWORD_TRAMPLE | KEYWORD_PROT_BLACK | KEYWORD_PROT_BLUE | KEYWORD_PROT_GREEN |
							KEYWORD_PROT_RED | KEYWORD_PROT_RED;

		int i;
		int keyword = 0;
		for(i=0; i<active_cards_count[1-player]; i++){
			if( in_play(1-player, i) && is_what(1-player, i, TYPE_CREATURE) && get_id(1-player, i) != get_id(player, card) ){
				int opp_key = get_abilities(1-player, i, EVENT_ABILITIES, -1);
				keyword |= (opp_key & keyword_mask);
			}
			if( keyword == keyword_mask ){
				break;
			}
		}
		event_result |= keyword;
	}

	return 0;
}

int card_evincars_justice(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, 3, 0, 0, 0, 0, 0);
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, 2, 2, NDA_ALL_CREATURES+NDA_PLAYER_TOO, &this_test);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_extinction(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = select_a_subtype(player, card);

		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_feverd_convulsion(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XB(2, 2), 0, &td, "TARGET_CREATURE");
}

int card_field_of_souls(int player, int card, event_t event){
	/* Field of Souls	|2|W|W
	 * Enchantment
	 * Whenever a nontoken creature is put into your graveyard from the battlefield, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		token.qty = instance->targets[11].card;
		generate_token(&token);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_flickering_ward(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_ABILITIES && instance->targets[1].card > -1 && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result |= instance->targets[1].card;
		}
		if( IS_GAA_EVENT(event) ){
			if( event == EVENT_RESOLVE_ACTIVATION ){
				bounce_permanent(instance->parent_controller, instance->parent_card);
			}
			return generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL);
		}
	}

	if (comes_into_play(player, card, event) ){
		instance->targets[1].card = select_a_protection(player);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_flowstone_sculpture(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ABILITIES && affect_me(player, card) && instance->targets[1].card > 0 && ! is_humiliated(player, card) ){
		event_result |= instance->targets[1].card;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( current_phase < PHASE_DECLARE_BLOCKERS ){
			ai_choice = 1;
		}
		if( current_phase == PHASE_AFTER_BLOCKING ){
			int my_pow = get_power(player, card);
			int my_tou = get_toughness(player, card);
			if( is_attacking(player, card) && ! is_unblocked(player, card) ){
				int count = 0;
				int opp_pow = 0;
				int opp_tou = 0;
				while( count < active_cards_count[1-player] ){
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
							card_instance_t *this = get_card_instance(1-player, count);
							if( this->blocking == card ){
								opp_pow = get_power(1-player, count);
								opp_tou = get_toughness(1-player, count);
							}
						}
						count++;
				}
				if( my_tou > opp_pow && my_pow > opp_tou && ! check_for_ability(player, card, KEYWORD_TRAMPLE) ){
					ai_choice = 3;
				}
				if( my_tou <= opp_pow && my_pow >= opp_tou && ! check_for_ability(player, card, KEYWORD_FIRST_STRIKE) ){
					ai_choice = 2;
				}
			}
			if( current_turn != player && instance->blocking < 255 ){
				if( get_toughness(1-player, instance->blocking) <= my_pow && ! check_for_ability(player, card, KEYWORD_FIRST_STRIKE) ){
					ai_choice = 2;
				}
			}
		}
		int choice = do_dialog(player, player, card, -1, -1, " Add +1/+1 counter\n Gains flying\n Gains first strike\n Gains trample\n Cancel", ai_choice);
		if( choice != 4 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				discard(player, 0, player);
				instance->info_slot = 66+choice;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *par = get_card_instance(instance->parent_controller, instance->parent_card);
		if( par->targets[1].card < 0 ){
			par->targets[1].card = 0;
		}
		if( instance->info_slot == 66 ){
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			par->targets[1].card |= KEYWORD_FLYING;
		}
		if( instance->info_slot == 68 ){
			par->targets[1].card |= KEYWORD_FIRST_STRIKE;
		}
		if( instance->info_slot == 69 ){
			par->targets[1].card |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_fools_tome(int player, int card, event_t event){

	if( (event == EVENT_CAN_ACTIVATE && hand_count[player] < 1) || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_furnace_of_rath(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && (damage->damage_target_card == -1 || !is_planeswalker(damage->damage_target_player, damage->damage_target_card)))
	damage->info_slot *= 2;

  return global_enchantment(player, card, event);
}

int card_gerrards_battle_cry(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, -1, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(2, 1), 0, NULL, NULL);
}

int card_goblin_bombardment(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_target0(player, card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

int card_grindstone(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int to_mill = 2;
			if( get_global_color_hack(2) ){
				to_mill = count_deck(instance->targets[0].player);
			}
			else{
				int count = 0;
				while( deck[count] != -1 ){
						int c1 = get_color_by_internal_id_no_hack( deck[count] );
						int c2 = get_color_by_internal_id_no_hack( deck[count+1] );
						if(  (c1 & c2) && (c1 != COLOR_TEST_COLORLESS && c2 != COLOR_TEST_COLORLESS) ){
							to_mill +=2;
						}
						else{
							break;;
						}
						count +=2;
				}
				if( to_mill > count_deck(instance->targets[0].player) ){
					to_mill = count_deck(instance->targets[0].player);
				}
			}
			mill(instance->targets[0].player, to_mill);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_PLAYER");
}

int card_hannas_custody(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && ! is_humiliated(player, card) && in_play(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
			event_result |= KEYWORD_SHROUD;
		}
	}

	return global_enchantment(player, card, event);
}

int card_heart_sliver(int player, int card, event_t event)
{
  // All Sliver creatures have haste.
  boost_subtype(player, card, event, SUBTYPE_SLIVER, 0,0, 0,SP_KEYWORD_HASTE, BCT_INCLUDE_SELF);

  return slivercycling(player, card, event);
}

int card_heartwood_giant(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, get_hacked_subtype(player, card, SUBTYPE_FOREST), 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select a %s to sacrifice", SUBTYPE_FOREST));
			test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_PLAYER") ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				tap_card(player, card);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
	}

	return 0;
}

int card_helm_of_possession(int player, int card, event_t event){

	/* Helm of Possession	|4
	 * Artifact
	 * You may choose not to untap ~ during your untap step.
	 * |2, |T, Sacrifice a creature: Gain control of target creature for as long as you control ~ and ~ remains tapped. */

	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if (IS_AI(player)){
		td.allowed_controller = 1-player;
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);
		gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED | GCUS_TAPPED);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_SACRIFICE_CREATURE, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_horned_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_TRAMPLE, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_humility(int player, int card, event_t event){
	// See also abilities.c:humility_legacy()

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_HUMILIATE);
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		humiliate(player, card, affected_card_controller, affected_card, 1);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test, ACT_DE_HUMILIATE);
	}

	if( event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES ){
		if( !in_play(player, card) || ! in_play(affected_card_controller, affected_card) )
			return 0;

		if (!is_what(affected_card_controller, affected_card, TYPE_CREATURE))
			return 0;

		if (event == EVENT_POWER)
			event_result += (1 - get_base_power(affected_card_controller, affected_card));
		else if (event == EVENT_TOUGHNESS)
			event_result += (1 - get_base_toughness(affected_card_controller, affected_card));
		else if (event == EVENT_ABILITIES){
			event_result &= (KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_TOUGHNESS|KEYWORD_RECALC_POWER|KEYWORD_RECALC_ABILITIES|KEYWORD_RECALC_SET_COLOR);
			card_instance_t* aff = get_card_instance(player, card);
			aff->state &= ~STATE_VIGILANCE;
			// Probably appropriate to remove more, but these are the only ones that give icons, so meh.
			if (aff->targets[16].card != -1)
				aff->targets[16].card &= ~(SP_KEYWORD_FEAR|SP_KEYWORD_INTIMIDATE|SP_KEYWORD_UNBLOCKABLE|SP_KEYWORD_DEATHTOUCH|SP_KEYWORD_LIFELINK
										   |SP_KEYWORD_INDESTRUCTIBLE|SP_KEYWORD_SHADOW|SP_KEYWORD_HEXPROOF|SP_KEYWORD_HORSEMANSHIP|SP_KEYWORD_VIGILANCE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_intuition(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int cards = MIN(count_deck(player), 3);
		if( cards > 0 ){
			int i;
			int chosen[3] = {-1, -1, -1};
			int *deck = deck_ptr[player];
			for(i=0; i<cards; i++){
				int selected = pick_card_from_deck(player);
				chosen[i] = deck[selected];
				remove_card_from_deck(player, selected);
			}

			int card_to_add = -1;
			if( !IS_AI(1-player) ){
				while( card_to_add == -1 ){
						card_to_add = show_deck(1-player, chosen, 3, "Choose a card", 0, 0x7375B0 );
				}
			}
			else{
				i=0;
				int max_value = 1000;
				for(i=0; i<cards; i++){
					card_ptr_t* c = cards_ptr[ cards_data[chosen[i]].id ];
					if( c->ai_base_value < max_value ){
						max_value = c->ai_base_value;
						card_to_add = i;
					}
				}
			}

			add_card_to_hand(player, chosen[card_to_add]);
			chosen[card_to_add] = -1;
			int num_unchosen = 0;
			for (i = 0; i < cards; ++i){
				if( chosen[i] != -1 ){
					raw_put_iid_on_top_of_deck(player, chosen[i]);
					++num_unchosen;
				}
			}
			mill(player, num_unchosen);
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static const char* not_effect_from_monotype_planeswalker(int who_chooses, int player, int card)
{
  /* This is the first card I've seen that can target all of artifact, creature, enchantment, and land, but not planeswalker.  Putting TARGET_TYPE_PLANESWALKER
   * into td.illegal_type is tempting, but won't work - it'll fail for an animated Gideon Jura, a planeswalker-artifact (by Memnarch), or planeswalker-
   * enchantment (by Enchanted Evening). */
  card_instance_t* eff = get_card_instance(player, card);
  if (!is_what(-1, eff->original_internal_card_id, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND)
	  && is_planeswalker(-1, eff->original_internal_card_id)
	  && !check_special_flags2(eff->parent_controller, eff->parent_card, SF2_ENCHANTED_EVENING))	// works properly even if parent card has left play by now
	return "type";

  return NULL;
}

static int interdict_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( instance->targets[0].player > -1 ){
		if( eot_trigger(player, card, event) ){
			disable_all_activated_abilities(instance->targets[0].player, instance->targets[0].card, 0);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_interdict(int player, int card, event_t event)
{
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

  target_definition_t td;
  counter_activated_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT|TYPE_LAND);
  td.extra = (int32_t)not_effect_from_monotype_planeswalker;
  td.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

  card_instance_t *instance = get_card_instance( player, card );

  if (event == EVENT_CAN_CAST)
	return can_counter_activated_ability(player, card, event, &td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  return cast_counter_activated_ability(player, card, 0);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (resolve_counter_activated_ability(player, card, &td, 0))
		{
		  card_instance_t* tgt = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		  disable_all_activated_abilities(tgt->parent_controller, tgt->parent_card, 1);
		  create_targetted_legacy_effect(player, card, &interdict_legacy, tgt->parent_controller, tgt->parent_card);
		  draw_cards(player, 1);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_invulnerability(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_DAMAGE_PREVENTION | GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_DAMAGE") ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *leg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			leg->info_slot = 0;
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jackal_pup(int player, int card, event_t event){
	if ( ! is_humiliated(player, card) && damage_dealt_to_me_arbitrary(player, card, event, 0, player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		damage_player(player, instance->targets[7].card, player, card);
	}
	return 0;
}

int card_jet_medallion(int player, int card, event_t event){

	return tempest_medallion(player, card, event, COLOR_TEST_BLACK);
}

int card_kezzerdrix(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(1-player, TYPE_CREATURE, -1) < 1 ){
			damage_player(player, 4, player, card);
		}
	}

	return 0;
}

int card_kindle(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 2;
			amount+= count_graveyard_by_id( player, get_id(player, card) );
			amount+= count_graveyard_by_id( 1-player, get_id(player, card) );
			damage_target0(player, card, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_knight_of_dawn(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_W(2), 0, NULL, NULL);
}

static const char* is_blocking_kod(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(card, targeting_card))
	return NULL;
  else
	return "must be blocking Knight of Dusk";
}

int card_knight_of_dusk(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller =  1-player;
	td2.preferred_controller = 1-player;
	td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td2.extra = (int32_t)is_blocking_kod;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_B(2), 0, &td2, "Select target creature blocking Knight of Dusk.");
}

int card_krakilin(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->info_slot = x_value;
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);
	}

	return regeneration(player, card, event, MANACOST_XG(1, 1));
}

int card_legacys_allure(int player, int card, event_t event){

	/* Legacy's Allure	|U|U
	 * Enchantment
	 * At the beginning of your upkeep, you may put a treasure counter on ~.
	 * Sacrifice ~: Gain control of target creature with power less than or equal to the number of treasure counters on ~. */

	card_instance_t *instance = get_card_instance( player, card);

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_TREASURE);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}


	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;
	td.power_requirement = count_counters(player, card, COUNTER_TREASURE) | TARGET_PT_LESSER_OR_EQUAL;

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_lightning_blast(int player, int card, event_t event){
	/*
	  Lightning Blast |3|R
	  Instant
	  Lightning Blast deals 4 damage to target creature or player.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_living_death(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_reanimate_all(player, card, ANYBODY, &this_test, REANIMATE_SPECIAL_LIVING_DEAD);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_lobotomy(int player, int card, event_t event){

	/* Lobotomy	|2|U|B
	 * Sorcery
	 * Target player reveals his or her hand, then you choose a card other than a basic land card from it. Search that player's graveyard, hand, and library for
	 * all cards with the same name as the chosen card and exile them. Then that player shuffles his or her library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int opponent = get_card_instance(player, card)->targets[0].player;
			if(hand_count[opponent]==0){
				kill_card(player, card, KILL_DESTROY);
				return 0;
			}
			int cards_array[ 500 ];
			int i=0;
			int hand_index = 0;
			int AI_pick = -1;
			int min_value = 0;
			for(i=0;i<active_cards_count[opponent]; i++){
				card_instance_t *instance = get_card_instance(opponent, i);
				if( ! ( instance->state & STATE_INVISIBLE ) && ! ( instance->state & STATE_IN_PLAY )  ){
					if( ! is_basic_land(opponent, i) ){
						card_ptr_t* c = cards_ptr[ get_id(opponent, i) ];
						if( c->ai_base_value > min_value ){
							min_value = c->ai_base_value;
							AI_pick = hand_index;
						}
					}
					cards_array[hand_index++] = instance->internal_card_id;
				}

			}

			// pick a card
			int selected = AI_pick;
			int valid_choice = 1;
			int id = -1;
			if( player == HUMAN ){
				selected = show_deck( player, cards_array, hand_count[opponent], "Pick a card", 0, 0x7375B0 );
				if( selected == -1 || is_basic_land_by_id(cards_data[cards_array[selected]].id) ){
					valid_choice = 0;
				}
				else{
					 id = cards_data[cards_array[selected]].id;
				}
			}

			if( valid_choice == 1 && id > -1 ){
				lobotomy_effect(player, 1-player, id, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_lotus_petal(int player, int card, event_t event)
{
  // 0x4dd5d1
  return artifact_mana_all_one_color(player, card, event, 1, 1);
}

int card_mana_severance(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
		if( choice == 0 ){
			int count = count_deck(player)-1;
			int *deck = deck_ptr[player];
			while( count > -1 ){
					if( cards_data[deck[count]].type & TYPE_LAND ){
						rfg_card_in_deck(player, count);
					}
					count--;
			}
		}
		else if( choice == 1 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to exile.");
				while( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
				}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_marble_titan(int player, int card, event_t event){

	if( event == EVENT_UNTAP && current_phase == PHASE_UNTAP && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			get_power(affected_card_controller, affected_card) > 2 ){
			card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
			instance->untap_status &= ~3;
		}
	}

	return 0;
}

int card_meditate(int player, int card, event_t event)
{
  /* Meditate	|2|U
   * Instant
   * Draw four cards. You skip your next turn. */

	if (event == EVENT_RESOLVE_SPELL){
		draw_cards(player, 4);
		skip_next_turn(player, card, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int mindwhip_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				discard(instance->targets[2].player, DISC_RANDOM, player);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_X(2), 0,
												&td2, "TARGET_PLAYER");
	}

	return 0;
}

int card_mindwhip_sliver(int player, int card, event_t event){
	/*
	  Mindwhip Sliver |2|B
	  Creature - Sliver 2/2, 2B (3)
	  All Slivers have "{2}, Sacrifice this permanent: Target player discards a card at random. Activate this ability only any time you could cast a sorcery."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &mindwhip_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &mindwhip_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}
	return slivercycling(player, card, event);
}

int card_minion_of_the_wastes(int player, int card, event_t event){
	/* Minion of the Wastes	|3|B|B|B
	 * Creature - Minion 100/100
	 * Trample
	 * As ~ enters the battlefield, pay any amount of life.
	 * ~'s power and toughness are each equal to the life paid as it entered the battlefield. */

	if (card == -1){
		return 0;	// no life was paid if this isn't on the bf
	}

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int to_pay = life[player];
		if( player == AI ){
			to_pay = life[player]-7;
		}

		int amount = choose_a_number(player, "Pay how much life?", to_pay);
		if( amount > life[player] ){
			amount = life[player]-1;
		}
		if( amount < 0 ){
			amount = 0;
		}
		lose_life(player, amount);
		instance->targets[1].player = amount;
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=instance->targets[1].player;
	}

	return 0;
}

int card_mirris_guile(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		rearrange_top_x(player, player, 3);
	}

	return global_enchantment(player, card, event);
}

int mnemonic_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		if( event == EVENT_CAN_ACTIVATE && in_play(p, c) ){
			int pp = player;
			int cc;
			for(cc=0; cc < active_cards_count[pp]; cc++){
				if( in_play(pp, cc) && is_what(pp, cc, TYPE_PERMANENT) && has_subtype(pp, cc, SUBTYPE_SLIVER) ){
					if( generic_activated_ability(pp, cc, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL) ){
						return 1;
					}
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_PERMANENT, "Select a Sliver to sacrifice.");
			test.subtype = SUBTYPE_SLIVER;
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			if( charge_mana_for_activated_ability(BYTE2(sac), BYTE3(sac), MANACOST_X(2)) ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_mnemonic_sliver(int player, int card, event_t event){
	/*
	  Mnemonic Sliver |2|U
	  Creature - Sliver 2/2
	  All Slivers have "{2}, Sacrifice this permanent: Draw a card."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &mnemonic_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &mnemonic_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}
	return slivercycling(player, card, event);
}

int card_mogg_conscripts(int player, int card, event_t event){

  if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && get_stormcreature_count(player) < 1 && ! is_humiliated(player, card) ){
		event_result = 1;
  }

  return 0;
}

int card_mogg_fanatic(int player, int card, event_t event){
	/*
	  Mogg Fanatic |R
	  Creature - Goblin 1/1
	  Sacrifice Mogg Fanatic: Mogg Fanatic deals 1 damage to target creature or player.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_mogg_raider(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;
	td2.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td2, NULL) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_PERMANENT, "Select a Goblin to sacrifice.");
			test.subtype = SUBTYPE_GOBLIN;
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td2, "TARGET_CREATURE") ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return 0;
}

int card_mongrel_pack(int player, int card, event_t event){
	/* Mongrel Pack	|3|G
	 * Creature - Hound 4/1
	 * When ~ dies during combat, put four 1/1 |Sgreen Hound creature tokens onto the battlefield. */

	if (current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HOUND, &token);
		token.qty = 4;
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return 0;
}

int card_mounted_archers(int player, int card, event_t event)
{
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  can_block_additional_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1);
	}

  return generic_activated_ability(player, card, event, GAA_NONE, MANACOST_W(1), 0, NULL, NULL);
}

int card_muscle_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 1, 1, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_natures_revolt(int player, int card, event_t event){

	if( event == EVENT_CHANGE_TYPE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);

		global_type_change(player, card, event, ANYBODY, TYPE_CREATURE, &this_test, 2, 2, 0, 0, 0);
	}

	return global_enchantment(player, card, event);
}

int card_orims_prayer(int player, int card, event_t event)
{
  /* Orim's Prayer	|1|W|W
   * Enchantment
   * Whenever one or more creatures attack you, you gain 1 life for each attacking creature. */

  int num;
  if ((num = declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER, 1-player, -1)))
	gain_life(player, num);

  return global_enchantment(player, card, event);
}

int card_orim_samite_healer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	//
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > 3 ){
				target->info_slot-=3;
			}
			else{
				target->info_slot = 0;
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int card_overrun(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 3, 3, KEYWORD_TRAMPLE, 0);
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_patchwork_gnomes(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(player, instance->parent_card) ){
		regenerate_target(player, instance->parent_card);
	}
	return generic_activated_ability(player, card, event, GAA_DISCARD | GAA_REGENERATION, MANACOST0, 0, NULL, NULL);
}

int card_pearl_medallion(int player, int card, event_t event){

	return tempest_medallion(player, card, event, COLOR_TEST_WHITE);
}

int card_pegasus_refuge(int player, int card, event_t event){
	/* Pegasus Refuge	|3|W
	 * Enchantment
	 * |2, Discard a card: Put a 1/1 |Swhite Pegasus creature token with flying onto the battlefield. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_PEGASUS, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(2), 0, NULL, NULL);
}

int card_perish(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_BURY);

		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_precognition(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		upkeep_trigger_ability_mode(player, card, event, player, would_validate_arbitrary_target(&td, 1-player, -1) ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			int *deck = deck_ptr[1-player];
			if( deck[0] != -1 ){
				int selected = -1;
				if( player == HUMAN ){
					selected = show_deck( player, deck, 1, "Choose a card to put on the bottom", 0, 0x7375B0 );
				}
				else{
					 selected = 0;
				}
				if( selected != -1 ){
					deck[ count_deck(player) ] = deck[0];
					remove_card_from_deck( player, 0 );
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_propaganda(int player, int card, event_t event){
	tax_attack(player, card, event);
	return global_enchantment(player, card, event);
}

int card_puppet_strings(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
			}
			else{
				if( instance->targets[0].player == player ){
					choice = 1;
				}
			}
			if(choice == 0){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_rathi_dragon(int player, int card, event_t event){

	if( player == AI && event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) < 2 ){
			ai_modifier-=500;
		}
	}

	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a Mountain to sacrifice.");
		test.subtype = SUBTYPE_MOUNTAIN;
		int sac[2] = {-1, -1};
		int i;
		for(i=0; i<2; i++){
			sac[i] = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac[i]){
				break;
			}
		}
		if( !sac[0] || ! sac[1] ){
			for(i=0; i<2; i++){
				if( sac[i] ){
					state_untargettable(BYTE2(sac[i]), BYTE3(sac[i]), 0);
				}
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			for(i=0; i<2; i++){
				kill_card(BYTE2(sac[i]), BYTE3(sac[i]), KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

int card_reanimate(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			int zombo = reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
			lose_life(player, get_cmc(player, zombo));
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}

int card_reckless_spite(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int resolved = 0;
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				resolved++;
			}
		}
		if( resolved > 0 ){
			lose_life(player, 5);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 2, NULL);
}

int card_recycle(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		skip_your_draw_step(player, event);

		if( current_turn == player && event == EVENT_MAX_HAND_SIZE){
		   event_result = 2;
		}

		if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == trigger_cause_controller
		   && player == reason_for_trigger_controller ){
		   if(event == EVENT_TRIGGER){
			  event_result |= RESOLVE_TRIGGER_MANDATORY;
		   }
		   else if(event == EVENT_RESOLVE_TRIGGER){
				   draw_a_card(player);
		   }
		}
	}

	return global_enchantment(player, card, event);
}

int card_reflecting_pool(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		int colors = get_color_of_mana_produced_by_id(get_id(player, card), COLOR_TEST_ANY_COLORED, player);
		if (colors < 0){
			colors = 0;
		}
		play_land_sound_effect_force_color(player, card, colors);
		return 0;	// so mana_producer() doesn't play the colorless sound
	}
	else {
		return mana_producer(player, card, event);
	}
}

int card_repentance(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(instance->targets[0].player, instance->targets[0].card),
							instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_respite(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_attackers(current_turn));
		fog_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rolling_thunder(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		instance->number_of_targets = 0;
		int trgs = 0;
		while( 1 ){
				if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
					if( trgs >= instance->info_slot ){
						break;
					}
				}
				else{
					if( ! has_mana(player, COLOR_COLORLESS, trgs+1) ){
						break;
					}
				}
				if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", trgs, 0) ){
					if( instance->targets[trgs].card != -1 ){
						add_state(instance->targets[trgs].player, instance->targets[trgs].card, STATE_TARGETTED);
					}
					trgs++;
				}
				else{
					break;
				}
		}
		if( ! trgs ){
			spell_fizzled = 1;
			return 0;
		}
		int i;
		for(i=0; i<trgs; i++){
			if( instance->targets[i].card != -1 ){
				remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
			}
		}
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_COLORLESS, trgs);
			if( spell_fizzled != 1 ){
				instance->info_slot = trgs;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

   return 0;
}

int card_root_maze(int player, int card, event_t event){

	/* Root Maze	|G
	 * Enchantment
	 * Artifacts and lands enter the battlefield tapped. */

	permanents_enters_battlefield_tapped(player, card, event, ANYBODY, TYPE_ARTIFACT | TYPE_LAND, NULL);

	return global_enchantment(player, card, event);
}

int card_ruby_medallion(int player, int card, event_t event){

	return tempest_medallion(player, card, event, COLOR_TEST_RED);
}

int card_sadistic_glee(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
		}

		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			add_1_1_counters(instance->damage_target_player, instance->damage_target_card, instance->targets[11].card);
			instance->targets[11].card = 0;
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_safeguard(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			negate_combat_damage_this_turn(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XW(2, 1), 0, &td, "TARGET_CREATURE");
}


int card_sapphire_medallion(int player, int card, event_t event){

	return tempest_medallion(player, card, event, COLOR_TEST_BLUE);
}

int card_scalding_tongs(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, hand_count[player] < 4 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(1-player, 1, player, card);
	}

	return 0;
}

int card_sarcomancy (int player, int card, event_t event){
	/* Sarcomancy	|B
	 * Enchantment
	 * When ~ enters the battlefield, put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * At the beginning of your upkeep, if there are no Zombies on the battlefield, ~ deals 1 damage to you. */

	if( comes_into_play(player, card, event ) ){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

	upkeep_trigger_ability_mode(player, card, event, player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ZOMBIE) < 1 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(player, 1, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_scorched_earth(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		instance->number_of_targets = 0;
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		}
		int trgs = 0;
		while( trgs < instance->info_slot && can_target(&td) ){
				if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && ! has_mana(player, COLOR_COLORLESS, trgs+1) ){
					break;
				}
				if( new_pick_target(&td, "TARGET_LAND", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int original_hand[2][hand_count[player]];
			int ohc = 0;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) ){
					original_hand[0][ohc] = get_original_internal_card_id(player, i);
					original_hand[1][ohc] = i;
					ohc++;
				}
			}
			int to_discard[hand_count[player]];
			int tdc = 0;
			while( tdc < instance->number_of_targets ){
					int selected = select_card_from_zone(player, player, original_hand[0], ohc, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						to_discard[tdc] = original_hand[1][selected];
						tdc++;
						for(i=selected; i<ohc; i++){
							original_hand[0][i] = original_hand[0][i+1];
							original_hand[1][i] = original_hand[1][i+1];
						}
						ohc--;
					}
					else{
						break;
					}
			}
			if( tdc < instance->number_of_targets ){
				spell_fizzled = 1;
				return 0;
			}
			charge_mana(player, COLOR_COLORLESS, instance->number_of_targets);
			if( spell_fizzled != 1 ){
				for(i=0; i<instance->number_of_targets; i++){
					discard_card(player, to_discard[i]);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_scroll_rack(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a card to put on top.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);

		int my_hand[100];
		int mhc = 0;
		while( hand_count[player] > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1 ){
					card_instance_t *this = get_card_instance( player, selected );
					my_hand[mhc] = this->internal_card_id;
					mhc++;
					obliterate_card(player, selected);
				}
				else{
					break;
				}
		}
		if( mhc > 0 ){
			draw_cards(player, mhc);
			int i;
			for(i = 0; i < mhc; i++){
				int card_added = add_card_to_hand(player, my_hand[i]);
				put_on_top_of_deck(player, card_added);
			}
			rearrange_top_x(player, player, mhc);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_searing_touch(int player, int card, event_t event){

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
			instance->info_slot = buyback(player, card, MANACOST_X(4));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_seeker_of_skybreak(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_selenia_dark_angel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION && in_play(instance->parent_controller, instance->parent_card) ){
		bounce_permanent(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST0, 2, NULL, NULL);
}

int card_serene_offering(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_shadow_rift(int player, int card, event_t event){
	/* Shadow Rift	|U
	 * Instant
	 * Target creature gains shadow until end of turn.
	 * Draw a card. */
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = ANYBODY;
	return vanilla_pump(player, card, event, &td, 0, 0, VANILLA_PUMP_DRAW_A_CARD, SP_KEYWORD_SHADOW);
}

int card_shocker(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( BYTE0(instance->targets[1].player) ){
			int amount = hand_count[0];
			new_discard_all(0, player);
			draw_cards(0, amount);
		}
		if( BYTE1(instance->targets[1].player) ){
			int amount = hand_count[1];
			new_discard_all(1, player);
			draw_cards(1, amount);
		}
	}

	return 0;
}

int card_skyshroud_ranger(int player, int card, event_t event){

	// |T: You may put a land card from your hand onto the battlefield. Activate this ability only any time you could cast a sorcery.
	if (event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land to put into play.");
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_SORCERY_BE_PLAYED, MANACOST_X(0), 0, NULL, NULL);
}

int card_skyshroud_elves(int player, int card, event_t event){
	if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && !is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card))
			declare_mana_available(player, COLOR_GREEN, 1);
	} else if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE){
		if (!can_produce_mana(player, card)){
			return 0;
		}

		int can_produce_g = !is_tapped(player, card) && !is_sick(player, card);
		if (player == AI){
			if (event == EVENT_CAN_ACTIVATE){
				return can_produce_g;
			} else {
				produce_mana_tapped(player, card, COLOR_GREEN, 1);
				return 0;
			}
		}

		card_instance_t *instance = get_card_instance(player, card);
		int can_produce_rw = instance->info_slot <= 0 && has_mana(player, COLOR_ANY, 1);

		if (event == EVENT_CAN_ACTIVATE){
			return can_produce_g || can_produce_rw;
		}

		int choice;
		if (can_produce_rw){
			choice = COLOR_TEST_RED|COLOR_TEST_WHITE;
			if (can_produce_g){
				choice |= COLOR_TEST_GREEN;
			}
			choice = choose_a_color_exe(player, "What kind of mana?", 1, 0, choice);
			if (choice == -1){
				cancel = 1;
				return 0;
			}
		} else {
			choice = COLOR_GREEN;
		}

		if (choice == COLOR_GREEN){
			produce_mana_tapped(player, card, COLOR_GREEN, 1);
		} else {
			instance->info_slot = 1;
			charge_mana(player, COLOR_COLORLESS, 1);
			instance->info_slot = 0;
			if (cancel == 1){
				return 0;
			}
			produce_mana(player, choice, 1);
		}
	}
	return 0;
}

int card_skyshroud_vampire(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) && !(event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		return generic_shade_amt_can_pump(player, card, 2, 0, MANACOST0, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test));
	}

	return 0;
}

int card_soltari_crusader(int player, int card, event_t event){

	shadow(player, card, event);

	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0, 0);
}

int card_soltari_emissary(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_W(1), 0, 0, 0, SP_KEYWORD_SHADOW);
}

int soltari_guerrillas_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	card_instance_t* damage = combat_damage_being_prevented(event);
	if( damage && instance->targets[1].player > -1 &&
		damage->damage_source_player == instance->damage_target_player && damage->damage_source_card == instance->damage_target_card &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		damage->damage_target_player != player &&
		in_play(instance->targets[0].player, instance->targets[0].card)
	  ){
		int dam = damage->info_slot;
		damage->info_slot = 0;
		damage_creature(instance->targets[1].player, instance->targets[1].card, dam, instance->damage_target_player, instance->damage_target_card);
		kill_card(player, card, KILL_REMOVE);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_soltari_guerrillas(int player, int card, event_t event){

	shadow(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( is_attacking(player, card) && is_unblocked(player, card) && current_phase == PHASE_AFTER_BLOCKING && instance->info_slot != 66 ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE){
		generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		if( spell_fizzled != 1 ){
			instance->info_slot = 66;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &soltari_guerrillas_effect,
													instance->parent_controller, instance->parent_card);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1] = instance->targets[0];
		leg->number_of_targets = 2;
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_soltari_lancer(int player, int card, event_t event){
	shadow(player, card, event);
	if( event == EVENT_ABILITIES && affect_me(player, card) && is_attacking(player, card) && ! is_humiliated(player, card) ){
		event_result |= KEYWORD_FIRST_STRIKE;
	}
	return 0;
}

int card_soltari_trooper(int player, int card, event_t event)
{
	/* Soltari Trooper	|1|W
	 * Creature - Soltari Soldier 1/1
	 * Shadow
	 * Whenever ~ attacks, it gets +1/+1 until end of turn. */

	shadow(player, card, event);

	return when_attacks_pump_self(player, card, event, 1,1);
}

int card_spike_drone(int player, int card, event_t event){

	return generic_spike(player, card, event, 1);
}

int card_spirit_mirror(int player, int card, event_t event){
	/* Spirit Mirror	|2|W|W
	 * Enchantment
	 * At the beginning of your upkeep, if there are no Reflection tokens on the battlefield, put a 2/2 |Swhite Reflection creature token onto the battlefield.
	 * |0: Destroy target Reflection. */

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_IS_TOKEN;
		this_test.subtype = SUBTYPE_REFLECTION;

		upkeep_trigger_ability_mode(player, card, event, player,
									check_battlefield_for_special_card(player, card, ANYBODY, 0, &this_test) ? 0 : RESOLVE_TRIGGER_MANDATORY);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_IS_TOKEN;
		this_test.subtype = SUBTYPE_REFLECTION;

		if( ! check_battlefield_for_special_card(player, card, ANYBODY, 0, &this_test) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_REFLECTION, &token);
			token.pow = 2;
			token.tou = 2;
			token.color_forced = COLOR_TEST_WHITE;
			generate_token(&token);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_REFLECTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0, &td, "Select target Reflection.");
}

int card_starke_of_rath(int player, int card, event_t event)
{
  /* Starke of Rath	|1|R|R
   * Legendary Creature - Human Rogue 2/2
   * |T: Destroy target artifact or creature. That permanent's controller gains control of ~. */

  check_legend_rule(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pp = instance->parent_controller, pc = instance->parent_card;

	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  if (pp != instance->targets[0].player)
		give_control_of_self(pp, pc);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "PUTREFY");	// Select target artifact or creature
}

static const char* can_be_untapped(int who_chooses, int player, int card)
{
	if( ! check_special_flags2(player, card, SF2_COULD_NOT_UNTAP) ){
		return NULL;
	}
	return "this permanent couldn't be untapped";
}

int card_static_orb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_tapped(player, card) && current_phase == PHASE_UNTAP && event == EVENT_UNTAP && instance->info_slot < 2){
		// This block avoid that multiple Static Orbs allow to untap more than 2 permanentes per untap
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && get_id(i, count) == get_id(player, card) && !(i == player && count == card) ){
						get_card_instance(i, count)->info_slot = 2;
					}
					count--;
			}
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_ENCHANTMENT | TARGET_TYPE_PLANESWALKER;
		td.allowed_controller = current_turn;
		td.preferred_controller = current_turn;
		td.required_state = TARGET_STATE_TAPPED;
		td.who_chooses = current_turn;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		td.extra = (int32_t)can_be_untapped;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		while( instance->info_slot < 2 && can_target(&td) ){
				if( select_target(player, card, &td, "Select a permanent to untap", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
					untap_card(instance->targets[0].player, instance->targets[0].card);
					instance->info_slot++;
				}
		}
		get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_staunch_defenders(int player, int card, event_t event){
	// original code : 0x413EE0

	if( comes_into_play(player, card, event) ){
		gain_life(player, 4);
	}

	return 0;
}

int card_storm_front(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_G(2), 0, &td, "Select target creature with flying.");
}

int card_talon_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_FIRST_STRIKE, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_thalakos_seer(int player, int card, event_t event){
	shadow(player, card, event);
	if( leaves_play(player, card, event) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_thumbscrew(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, hand_count[player] > 4 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(1-player, 1, player, card);
	}

	return 0;
}

int card_time_warp(int player, int card, event_t event)
{
	/* Time Warp	|3|U|U
	 * Sorcery
	 * Target player takes an extra turn after this one. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL){
		card_instance_t* instance = get_card_instance(player, card);
		if (valid_target(&td)){
			if( instance->targets[0].player == player ){
				time_walk_effect(player, card);
			}
			else{
				int fake = add_card_to_hand(instance->targets[0].player, get_original_internal_card_id(player, card));
				time_walk_effect(instance->targets[0].player, fake);
				obliterate_card(instance->targets[0].player, fake);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_tooth_and_claw(int player, int card, event_t event){
	/* Tooth and Claw	|3|R
	 * Enchantment
	 * Sacrifice two creatures: Put a 3/1 |Sred Beast creature token named Carnivore onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL)  ){
			return count_subtype(player, TYPE_CREATURE, -1) > 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int sac[2] = {0, 0};
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			sac[0] = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac[0]){
				cancel = 1;
				return 0;
			}
			sac[1] = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac[1]){
				state_untargettable(BYTE2(sac[0]), BYTE3(sac[0]), 0);
				cancel = 1;
				return 0;
			}
			kill_card(BYTE2(sac[0]), BYTE3(sac[0]), KILL_SACRIFICE);
			kill_card(BYTE2(sac[1]), BYTE3(sac[1]), KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_CARNIVORE);
	}

	return global_enchantment(player, card, event);
}

int card_torture_chamber(int player, int card, event_t event){

	/* Torture Chamber	|3
	 * Artifact
	 * At the beginning of your upkeep, put a pain counter on ~.
	 * At the beginning of your end step, ~ deals damage to you equal to the number of pain counters on it.
	 * |1, |T, Remove all pain counters from ~: ~ deals damage to target creature equal to the number of pain counters removed this way. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_PAIN);
	}

	if( count_counters(player, card, COUNTER_PAIN) > 0 && current_turn == player && eot_trigger(player, card, event) ){
		int amount = count_counters(player, card, COUNTER_PAIN);
		damage_player(player, amount, player, card);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = count_counters(player, card, COUNTER_PAIN);
			remove_counters(player, card, COUNTER_PAIN, instance->info_slot);
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
		}
	}

	return 0;
}

int card_tradewind_rider(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.special = TARGET_SPECIAL_NOT_ME;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, &td1, NULL)  ){
			return target_available(player, card, &td) > 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int tapped[2] = {-1, -1};
			int i;
			for(i=0; i<2; i++){
				if( select_target(player, card, &td, "Select a creature to tap.", &(instance->targets[i])) ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
					tapped[i] = instance->targets[i].card;
				}
				else{
					break;
				}
			}
			for(i=0; i<2; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			if( tapped[0] == -1 || tapped[1] == -1 ){
				spell_fizzled = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td1, "TARGET_PERMANENT") ){
				for(i=0; i<2; i++){
					tap_card(player, tapped[i]);
				}
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if(valid_target(&td1) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

static int effect_unstable_shapeshifter(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( ! is_humiliated(instance->damage_target_player, instance->damage_target_card) ){
			if (specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0)){
				cloning_and_verify_legend(instance->damage_target_player, instance->damage_target_card, instance->targets[1].player, instance->targets[1].card);
			}
		}
	}

  return 0;
}
int card_unstable_shapeshifter(int player, int card, event_t event)
{
  /* Unstable Shapeshifter	|3|U
   * Creature - Shapeshifter 0/1
   * Whenever another creature enters the battlefield, ~ becomes a copy of that creature and gains this ability. */

  cloning_card(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	create_targetted_legacy_effect(player, card, &effect_unstable_shapeshifter, player, card);

  return 0;
}

int card_verdant_force(int player, int card, event_t event){
	/* Verdant Force	|5|G|G|G
	 * Creature - Elemental 7/7
	 * At the beginning of each upkeep, put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}

	return 0;
}

int card_vathi_il_dal(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int ai_choice = 1;
			if( is_attacking(instance->targets[0].player, instance->targets[0].card) &&
				is_unblocked(instance->targets[0].player, instance->targets[0].card)
			  ){
				ai_choice = 0;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Turn power to 1\n Turn toughness to 1", ai_choice);
			if( choice == 0 ){
				set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												1, -1, 0, 0, 0);
			}
			else{
				set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												-1, 1, 0, 0, 0);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_wasteland(int player, int card, event_t event){

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
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Destroy a nonbasic land\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					new_pick_target(&td, "TARGET_NONBASIC_LAND", 0, 1)
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

int card_wild_wurm(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		if( ! flip_a_coin(player, card) ){
			bounce_permanent(player, card);
		}
	}
	return 0;
}

int card_winged_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_FLYING, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_whispers_of_the_muse(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, MANACOST_X(5));
	}
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		if( instance->info_slot == 1 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_worthy_cause(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		instance->targets[1].player = get_toughness(BYTE2(sac), BYTE3(sac));
		instance->info_slot = buyback(player, card, MANACOST_X(2));
		if( spell_fizzled != 1 ){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, instance->targets[1].player);

		if( instance->info_slot == 1 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}


int card_winds_of_rath(int player, int card, event_t event)
{
  // Destroy all creatures that aren't enchanted. They can't be regenerated.

	if (event == EVENT_RESOLVE_SPELL){
		char enchanted[2][151] = {{0}};
		int p, c;
		card_instance_t* aura;
		for (p = 0; p <= 1; ++p){
			for (c = 0; c < active_cards_count[p]; ++c){
				if ((aura = in_play(p, c)) && is_what(p, c, TYPE_ENCHANTMENT) && has_subtype(p, c, SUBTYPE_AURA)
					&& aura->damage_target_player >= 0 && aura->damage_target_player <= 1
					&& aura->damage_target_card >= 0 && aura->damage_target_card <= 150
				  ){
					enchanted[aura->damage_target_player][aura->damage_target_card] = 1;
				}
			}

			for (p = 0; p <= 1; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && !enchanted[p][c]){
						kill_card(p, c, KILL_BURY);
					}
				}
			}
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int rootwater_matriarch_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_enchanted(instance->targets[0].player, instance->targets[0].card) ){
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_REMOVE);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_rootwater_matriarch(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	if( duh_mode(player) ){
		td.required_state = TARGET_STATE_ENCHANTED;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		if( instance->targets[0].player > -1 && is_enchanted(instance->targets[0].player, instance->targets[0].card) ){
			int legacy = gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			int l2 = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &rootwater_matriarch_effect,
													instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, l2);
			leg->targets[1].player = instance->parent_controller;
			leg->targets[1].card = legacy;
			leg->number_of_targets = 2;
			add_status(instance->parent_controller, l2, STATUS_INVISIBLE_FX);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_enraging_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, 0, 0, 0, 0, 1, 0, 0, 0, 0, SP_KEYWORD_HASTE, player);
}

int card_leeching_licid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_what(player, card, TYPE_CREATURE) && instance->damage_target_player > -1 ){
		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			damage_player(instance->damage_target_player, 1, player, card);
		}
	}

	return generic_licid(player, card, event, MANACOST_B(1), 0, 0, 0, 0, 1-player);
}

int card_nurturing_licid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && instance->targets[3].card != -1 ){
		event_result = instance->targets[3].card;
	}

	if( ! is_what(player, card, TYPE_CREATURE) ){
		if( instance->damage_target_player > -1 ){
			if( event == EVENT_CAN_ACTIVATE ){
				if( generic_activated_ability(player, card, event, 0, MANACOST_G(1), 0, NULL, NULL) ){
					if( (land_can_be_played & LCBP_REGENERATION) &&
						get_card_instance(instance->damage_target_player, instance->damage_target_card)->kill_code == KILL_DESTROY
					  ){
						return 99;
					}
					return 1;
				}
			}

			if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
					instance->info_slot = 2;
					if( (land_can_be_played & LCBP_REGENERATION) &&
						get_card_instance(instance->damage_target_player, instance->damage_target_card)->kill_code == KILL_DESTROY
					  ){
						instance->info_slot = can_regenerate(instance->damage_target_player, instance->damage_target_card) ? 1 : 2;
					}
				}
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				if( instance->info_slot == 1 && can_regenerate(instance->damage_target_player, instance->damage_target_card) ){
					regenerate_target(instance->damage_target_player, instance->damage_target_card);
				}
				if( instance->info_slot == 2 ){
					card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
					reset_subtypes(instance->parent_controller, instance->parent_card, 2);
					parent->damage_target_player = parent->damage_target_card = parent->targets[3].card = -1;
				}
			}
			return generic_aura(player, card, event, player, 0, 0, KEYWORD_REGENERATION, 0, 0, 0, 0);
		}
	}

	return generic_licid(player, card, event, MANACOST_G(1), 0, 0, 0, 0, player);
}

int card_quickening_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, MANACOST_XW(1, 1), 0, 0, KEYWORD_FIRST_STRIKE, 0, player);
}

int card_stinging_licid(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_what(player, card, TYPE_CREATURE) && instance->damage_target_player > -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			damage_player(instance->damage_target_player, 2, player, card);
		}
	}

	return generic_licid(player, card, event, MANACOST_XU(1, 1), 0, 0, 0, 0, 1-player);
}
