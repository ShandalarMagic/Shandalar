#include "manalink.h"

// Functions
int fetchland(int player, int card, event_t event, subtype_t subtype1, subtype_t subtype2, int lifecost){
	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}
	else if( event == EVENT_CAN_ACTIVATE ){
		return CAN_TAP(player, card) && (!lifecost || can_pay_life(player, lifecost)) && CAN_ACTIVATE0(player, card);
	}
	else if(event == EVENT_ACTIVATE ){
			if( player == AI && (!lifecost || life[player] > 6) ){
				ai_modifier+=1000;
			}
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				tap_card(player, card);
				if (lifecost){
					lose_life(player, lifecost);
				}
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a or %s card.", subtype1, subtype2));
			this_test.subtype = get_hacked_subtype(player, card, subtype1);
			this_test.sub2 = get_hacked_subtype(player, card, subtype2);
			this_test.subtype_flag = F2_MULTISUBTYPE;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

static int crown(int player, int card, event_t event, int pow, int tou, int key, int s_key)
{
  card_instance_t* instance;

  if (event == EVENT_CAN_ACTIVATE)
	return (instance = in_play(player, card)) && instance->damage_target_card >= 0 && CAN_ACTIVATE0(player, card) && can_sacrifice_this_as_cost(player, card);

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST0))
	{
	  instance = get_card_instance(player, card);
	  instance->targets[0].player = instance->damage_target_player;
	  instance->targets[0].card = instance->damage_target_card;
	  kill_card(player, card, KILL_SACRIFICE);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  instance = get_card_instance(player, card);
	  int p, c;
	  int marked[2][151] = {{0}};
	  if (in_play(instance->targets[0].player, instance->targets[0].card))
		{
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE)
				  && ((c == instance->targets[0].card && p == instance->targets[0].player)
					  || shares_creature_subtype(instance->targets[0].player, instance->targets[0].card, p, c)))
				marked[p][c] = 1;

		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (marked[p][c] && in_play(p, c))
				pump_ability_until_eot(player, card, p, c, pow, tou, key, s_key);
		}
	}

  return generic_aura(player, card, event, player, pow, tou, key, s_key, 0, 0, 0);
}

void check_for_turned_face_up_card_interactions(int player, int card, int new_identity){
		// What would have been so terrible with making this a trigger instead of trying to simulate one, badly?
		card_instance_t *instance = get_card_instance(player, card);
		int i, count;
		for(i=0;i<2;i++){
			for (count = 0; count < active_cards_count[i]; ++count){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && ! is_humiliated(i, count) ){
						int csvid = get_id(i, count);
						switch( csvid ){
								case CARD_ID_APHETTO_RUNECASTER:
									draw_some_cards_if_you_want(i, count, i, 1);
									break;
								case CARD_ID_PINE_WALKER:
								{
									if( player == i ){
										untap_card(player, card);
									}
									break;
								}
								case CARD_ID_TRAIL_OF_MYSTERY:
								{
									if( player == i && is_what(-1, new_identity, TYPE_CREATURE) ){
										pump_until_eot(i, count, player, card, 2, 2);
									}
									break;
								}
								case CARD_ID_SALT_ROAD_AMBUSHERS:
									if (player == i && is_what(-1, new_identity, TYPE_CREATURE)){
										add_1_1_counters(player, card, 2);
									}
									break;
								case CARD_ID_SECRET_PLANS:
								{
									if( player == i ){
										draw_cards(player, 1);
									}
									break;
								}
								case CARD_ID_MASTERY_OF_THE_UNSEEN:
								{
									if( player == i ){
										gain_life(player, count_subtype(player, TYPE_CREATURE, -1));
									}
									break;
								}
								case CARD_ID_TEMUR_WAR_SHAMAN:
								{
									if( player == i && is_what(-1, new_identity, TYPE_CREATURE) ){
										target_definition_t td;
										default_target_definition(player, card, &td, TYPE_CREATURE);
										td.allowed_controller = 1-player;
										td.preferred_controller = 1-player;

										instance->number_of_targets = 0;

										if( can_target(&td) ){
											if( do_dialog(player, player, card, -1, -1, " Make this creature Fight\n Pass", 1) == 0 ){
												if( new_pick_target(&td, "Select target creature you don't control.", 0, GS_LITERAL_PROMPT) ){
													fight(player, card, instance->targets[0].player, instance->targets[0].card);
												}
											}
										}
									}
									break;
								}
								default:
									break;
						}
					}
			}
		}

		int iid_deathmist_raptor = get_internal_card_id_from_csv_id(CARD_ID_DEATHMIST_RAPTOR);
		const int* gy = get_grave(player), *gysrc = graveyard_source[player];
		int grafdigger_checked = 0, max_gysrc_id = next_graveyard_source_id;
		for (i = 0; i < 500 && gy[0] != -1; ++i){
			if (gy[i] == iid_deathmist_raptor
				&& ((gysrc[i] & 0x00FFFE00) >> 9) < max_gysrc_id){	// Don't consider any cards put into the graveyard since we started checking
				if (!grafdigger_checked){
					grafdigger_checked = 1;
					if (check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE)){
						break;
					}
				}
				switch (DIALOG(player, card, EVENT_ACTIVATE,
							   DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_FULLCARD_ID(iid_deathmist_raptor),
							   "Return face-up",		1, 4,
							   "Return face_down",	1, has_mana_multi(player, MANACOST_XG(4,1)) ? 8 : 2,
							   "Decline",				1, 1)){
					case 1:
						if (reanimate_permanent(player, -1, player, i, REANIMATE_DEFAULT) != -1){
							--i;
						}
						break;
					case 2:
						if (reanimate_permanent_with_function(player, -1, player, i, REANIMATE_DEFAULT, turn_face_down) != -1){
							--i;
						}
						break;
				}
			}
		}
}

void flip_card(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	int new_identity = get_internal_card_id_from_csv_id( instance->targets[12].player );
	if( instance->targets[12].player != -1 ){
		check_for_turned_face_up_card_interactions(player, card, new_identity);

		instance->targets[12].card = new_identity;

		dispatch_event_to_single_card_overriding_function(player, card, EVENT_TURNED_FACE_UP, new_identity);

		// See also cloning_and_verify_legend(), which should parallel this.
		instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
		get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
		if (in_play(player, card)){
			verify_legend_rule(player, card, get_id(player, card));
		}
	}
}

int card_face_down_creature(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	int flip = 0;

	// illusionary mask code
	if( instance->targets[12].player > available_slots-1 ){
		if( is_tapped(player, card) || (instance->state & STATE_ATTACKING) ){
			flip = 1;
		}
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = 2;
		td.extra = damage_card;
		td.required_type = 0;
		if( event == EVENT_CAN_ACTIVATE && can_target(&td) ){
			// see if any damage is coming my way
			int p;
			for(p=0;p<2;p++){
				int count = 0;
				while(count < active_cards_count[p]){
					instance->targets[0].player = p;
					instance->targets[0].card = count;
					if( would_valid_target(&td) ){
						card_instance_t *damage = get_card_instance(p, count);
						if( damage->damage_target_card == card && damage->damage_target_player == player && damage->damage_source_player == player && damage->info_slot > 0 ){
							flip = 1;
							return 0x63;
						}
					}
					count++;
				}
			}
		}
		else if( event == EVENT_ACTIVATE ){
			flip = 1;
		}
	}
	else{
		if( event == EVENT_CAN_ACTIVATE && instance->targets[12].player > -1 ){
			return dispatch_event_to_single_card_overriding_function(player, card, EVENT_CAN_UNMORPH,
																	get_internal_card_id_from_csv_id(instance->targets[12].player));
		}
		else if( event == EVENT_ACTIVATE ){
				dispatch_event_to_single_card_overriding_function(player, card, EVENT_UNMORPH,
																  get_internal_card_id_from_csv_id(instance->targets[12].player));
				if( spell_fizzled != 1 ){
					flip = 1;
					instance->state |= STATE_DONT_RECOPY_ONTO_STACK;
				}
		}
	}

	// turn the card face up
	if( flip == 1 ){
		flip_card(player, card);
	}

	return 0;
}

// Beware, this only works for cards that originally had morph, cards with an ixidron_legacy attached, and cards that call can_exist_while_face_down().
void turn_face_down(int player, int card){
	if (player >= 0 && card >= 0){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->internal_card_id == -1){
		  return;	// card not in play, not on stack, not in a hand
		}
		instance->targets[12].player = get_original_id(player, card);
		instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);
		/* Force immediate change of type and color calculation.  The letting it fall through to morph(), as it would on cards natively with morph, would only
		 * do the former.  Set internal_card_id directly instead of going through EVENT_CHANGE_TYPE, since (especially if this is a Vesuvan Shapeshifter) this
		 * may temporarily be a creature type that doesn't have morph. */
		instance->regen_status |= KEYWORD_RECALC_SET_COLOR;
		instance->backup_internal_card_id = instance->internal_card_id = instance->targets[12].card;
		get_abilities(player, card, EVENT_SET_COLOR, -1);
	}
}

int casting_permanent_with_morph(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	int cless = played_for_free(player, card) ? 0 : get_updated_casting_cost(player, -1, get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE), event, 3);
	cless = MAX(0, cless);
	int choice = 0;
	if( has_mana(player, COLOR_COLORLESS, cless) ){
		if( played_for_free(player, card) || has_mana_to_cast_iid(player, event, instance->internal_card_id) ){
			if( ! played_for_free(player, card) ){
				choice = do_dialog(player, player, card, -1, -1, " Play face-down\n Play normally\n Cancel", played_for_free(player, card));
			}
			else{
				choice = do_dialog(player, player, card, -1, -1, " Play face-down\n Play normally", played_for_free(player, card));
			}

		}
	}
	else{
		choice = 1;
	}

	if( choice == 0 ){
		charge_mana(player, COLOR_COLORLESS, cless);
		if( spell_fizzled != 1 ){
			instance->targets[12].player = get_id(player, card);
			instance->targets[12].card = get_internal_card_id_from_csv_id( CARD_ID_FACE_DOWN_CREATURE );
			instance->targets[16].card = 0;

			// Normally these aren't done until the card is in play, but we need it to display the updated values on the stack as well.
			instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES|KEYWORD_RECALC_SET_COLOR|KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS;
			get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
			get_abilities(player, card, EVENT_SET_COLOR, -1);	// usually this happens before change type
			get_abilities(player, card, EVENT_ABILITIES, -1);
			get_abilities(player, card, EVENT_TOUGHNESS, -1);
			get_abilities(player, card, EVENT_POWER, -1);
			return 2;
		}
	}

	if( choice == 1 ){
		if( ! played_for_free(player, card) ){
			charge_mana_from_id(player, card, event, get_id(player, card));
		}
		if( spell_fizzled != 1 &&
			( get_id(player, card) == CARD_ID_BLISTERING_FIRECAT ||
			 get_id(player, card) == CARD_ID_FLEDGLING_MAWCOR
			)
		  ){
			instance->targets[1].player = 66;
		}
		return 1;
	}
	return 0;
}

// A card without morph that has a turn-self-face-down ability.
int can_exist_while_face_down(int player, int card, event_t event)
{
	if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
		card_instance_t* inst = get_card_instance(player, card);
		if (inst->targets[12].card != -1){
			event_result = inst->targets[12].card;
		}
	}
	return 0;
}

int morph(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	if( event == EVENT_HAS_MORPH ){
		return 1;
	}

	// show the face down creature when we are morphed
	if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->targets[12].card != -1){
			event_result = instance->targets[12].card;
		}
	}

	if( event == EVENT_MODIFY_COST ){
		int cless = get_updated_casting_cost(player, -1, get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE), event, 3);
		cless = MAX(0, cless);
		card_instance_t* instance = get_card_instance(player, card);
		if( has_mana(player, COLOR_COLORLESS, cless) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) && get_card_instance(player, card)->info_slot == 1){
			if( ! casting_permanent_with_morph(player, card, event) ){
				spell_fizzled = 1;
			}
		}
	}

	else if ( event == EVENT_CAN_UNMORPH && has_mana_multi(player, colorless, black, blue, green, red, white) ){
			  return 1;
	}
	else if( event == EVENT_UNMORPH ){
			 charge_mana_multi(player, colorless, black, blue, green, red, white);
	}
	return 0;
}

void double_faced_card(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[12].card > 0 && event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
		event_result = instance->targets[12].card;
		instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
	}
	if (event == EVENT_CAN_SKIP_TURN){
	   instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
	   get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
	   get_abilities(player, card, EVENT_ABILITIES, -1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[13].player = get_id(player, card);
		instance->targets[13].card = get_id(player, card);
	}
}

// Cards
int card_accursed_centaur(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_aether_charge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, SUBTYPE_BEAST, 0, 0, 0, 0, 0, -1, 0) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		if( would_validate_target(player, card, &td, 0) ){
			damage_player(1-player, 4, instance->targets[1].player, instance->targets[1].card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_aggravated_assault(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 2, 0) ){
		return can_sorcery_be_played(player, event);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 2, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, instance->parent_card, player, &this_test, ACT_UNTAP);
		get_an_additional_combat_phase();
	}

	return global_enchantment(player, card, event);
}

int card_airborne_aid(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, count_subtype(2, TYPE_PERMANENT, SUBTYPE_BIRD));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_akromas_blessing(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 0, 0, 0, 0, 0, 1);
}

int card_akromas_vengeance(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 3, 0, 0, 0, 0, 0);
}

int card_ancestors_prophet(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_CLERIC;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 4 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! tapsubtype_ability(player, card, 5, &td1) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 10);
	}

	return 0;
}

int card_aphetto_dredging(int player, int card, event_t event){
	if (event == EVENT_CAN_CAST){
		return 1;
	} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		int subt = select_a_subtype(player, card);

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, get_subtype_text("Select up to three %s creature cards.", subt));
		this_test.subtype = subt;
		this_test.subtype_flag = MATCH;

		if (new_special_count_grave(player, &this_test) == 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
		}

		card_instance_t* instance = get_card_instance(player, card);
		select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, 3, &instance->targets[0]);
	} else if (event == EVENT_RESOLVE_SPELL){
		int i;
		for (i = 0; i < 3; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_aphetto_grifter(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_WIZARD;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 1 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td2);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( tapsubtype_ability(player, card, 2, &td1) ){
				if( pick_target(&td2, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_arcanis_the_omnipotent(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if(event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 2, 0, 0, 0) ){
			return 1;
		}
		if( ! is_sick(player, card) && ! is_tapped(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}
	else if (event == EVENT_ACTIVATE ){
			int choice = 0;
			if( ! is_sick(player, card) && ! is_tapped(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( has_mana_for_activated_ability(player, card, 2, 0, 2, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Draw 3\n Return to Hand\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 2*choice, 0, 2*choice, 0, 0, 0) ){
					if( choice == 0 ){
						tap_card(player, card);
					}
					instance->info_slot = 66+choice;
				}
			}
	}
	else if (event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 67 ){
				if( in_play(instance->parent_controller, instance->parent_card) ){
					bounce_permanent( instance->parent_controller, instance->parent_card );
				}
			}
			if( instance->info_slot == 66 ){
				draw_cards(player, 3);
			}
	}

	return 0;
}

int card_astral_slide(int player, int card, event_t event){
	return global_enchantment(player, card, event);
}

// activate Astral Slide
void activate_astral_slide(){

	int p,c;
	for(p=0;p<2;p++){
		for(c=0;c<active_cards_count[p];c++){
			if( in_play(p, c) && CARD_ID_ASTRAL_SLIDE == get_id(p, c) ){
				target_definition_t td;
				default_target_definition(p, c, &td, TYPE_CREATURE );
				td.allow_cancel = 0;
				td.preferred_controller = 1-p;

				int ai_choice = 0;

				if( p == AI ){
					td.allowed_controller = 1-p;
					if( ! can_target(&td) ){
						ai_choice = 1;
					}
					td.allowed_controller = 2;
				}

				if( can_target(&td) ){
					int choice = do_dialog(p, p, c, -1, -1, " Activate Astral Slide\n Pass", ai_choice);
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							card_instance_t *instance = get_card_instance(p, c);
							instance->number_of_targets = 1;
							remove_until_eot(p, c, instance->targets[0].player, instance->targets[0].card);
						}
					}
				}
			}
			if( in_play(p, c) && CARD_ID_INVIGORATING_BOON == get_id(p, c) ){
				target_definition_t td;
				default_target_definition(p, c, &td, TYPE_CREATURE);
				td.preferred_controller = p;

				card_instance_t *instance = get_card_instance(p, c);

				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
			if( in_play(p, c) && CARD_ID_LIGHTNING_RIFT == get_id(p, c) ){
				target_definition_t td;
				default_target_definition(p, c, &td, TYPE_CREATURE );
				td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
				td.preferred_controller = 1-p;

				card_instance_t *instance = get_card_instance(p, c);

				int ai_choice = 0;

				if( p == AI ){
					td.allowed_controller = 1-p;
					if( ! can_target(&td) ){
						ai_choice = 1;
					}
					td.allowed_controller = 2;
				}

				if( can_target(&td) ){
					int choice = do_dialog(p, p, c, -1, -1, " Activate Lightning Rift\n Pass", ai_choice);
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							instance->number_of_targets = 1;
							damage_creature(instance->targets[0].player, instance->targets[0].card, 2, p, c);
						}
					}
				}
			}
		}
	}
}

int card_aura_extraction(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_ENCHANTMENT") ){
			instance->number_of_targets = 1;
			if( is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
}

int card_aurification(int player, int card, event_t event)
{
  /* Aurification	|2|W|W
   * Enchantment
   * Whenever a creature deals damage to you, put a gold counter on it.
   * Each creature with a gold counter on it is a Wall in addition to its other creature types and has defender.
   * When ~ leaves the battlefield, remove all gold counters from all creatures. */

  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == -1 && damage->damage_target_player == player && !damage_is_to_planeswalker(damage)
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && in_play(damage->damage_source_player, damage->damage_source_card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (instance->number_of_targets < 10)
		{
		  instance->targets[instance->number_of_targets].player = damage->damage_source_player;
		  instance->targets[instance->number_of_targets].card = damage->damage_source_card;
		  ++instance->number_of_targets;
		}
	  else	// no more room in targets; just add the counter now
		add_counter(damage->damage_source_player, damage->damage_source_card, COUNTER_GOLD);
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (event == EVENT_TRIGGER && instance->number_of_targets > 0)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int i;
		  for (i = 0; i < instance->number_of_targets; ++i)
			add_counter(instance->targets[i].player, instance->targets[i].card, COUNTER_GOLD);
		  instance->number_of_targets = 0;
		}
	}

  // Wall subtype added by a hack in has_subtype(), rather than add_a_subtype(), so that it follows moved gold counters.

  if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && count_counters(affected_card_controller, affected_card, COUNTER_GOLD))
	event_result |= KEYWORD_DEFENDER;

  if (leaves_play(player, card, event))
	{
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			remove_all_counters(p, c, COUNTER_GOLD);
	}

  return global_enchantment(player, card, event);
}

int card_avarax(int player, int card, event_t event){

	haste(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.id = get_id(player, card);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
	}

	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_aven_brigadier(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_BIRD, 1, 1, 0, 0);
	boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 1, 1, 0, 0);
	return 0;
}

int card_barren_moor(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if( in_play(player, card ) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND ) {
		return mana_producer(player, card, event);
	}
	else{
		return cycling(player, card, event, 0, 1, 0, 0, 0, 0);
	}
}

int card_battlefield_medic(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	//
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_CLERIC);
		if( target->info_slot < amount ){
			target->info_slot = 0;
		}
		else{
			target->info_slot-=amount;
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 0, 0,
									&td, "TARGET_CREATURE");
}

int card_biorhythm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return can_target(&td);
		}
		else{
			return 1;
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			set_life_total(player, count_permanents_by_type(player, TYPE_CREATURE));
			set_life_total(1-player, count_permanents_by_type(1-player, TYPE_CREATURE));
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_birchlore_rangers(int player, int card, event_t event){
	// See also card_heritage_druid() in morningtide.c.

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_subtype = SUBTYPE_ELF;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && target_available(player, card, &td) > 1 ){
		return 1;
	}

	if( event == EVENT_ACTIVATE){
		if( tapsubtype_ability(player, card, 2, &td) ){
			instance->number_of_targets = 2;
			// See comments in tap_a_permanent_you_control_for_mana(), which ideally should be generalized to handle this card.
			produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1);
			tapped_for_mana_color = -2;
			/* EVENT_TAP_CARD already sent for both elves.  It'll get sent twice for this if this happens to be one of the ones chosen; but it's not worth the
			 * effort to fix. */
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) ){
		int count = target_available(player, card, &td);
		if (count > 2){
			/* Same issues and solution as card_axebane_guardian() in return_to_ravnica.c and permanents_you_control_can_tap_for_mana_all_one_color() in
			 * produce_mana.c. */
			count /= 2;
			int i;
			for (i = 0; i < 5; ++i){
				if ((count + i) >= 5){
					declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, (count + i) / 5);
				}
			}
		}
	}

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 24;
	}
	if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 24;
	}

	return morph(player, card, event, 0, 0, 0, 1, 0, 0);
}

int card_blistering_firecat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_TURNED_FACE_UP ){
		instance->targets[1].player = 66;
	}

	if( instance->targets[1].player == 66 ){
		haste(player, card, event);
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return morph(player, card, event, 0, 0, 0, 0, 2, 0);
}

int card_bloodline_shaman(int player, int card, event_t event){

	/* Bloodline Shaman	|1|G
	 * Creature - Elf Wizard Shaman 1/1
	 * |T: Choose a creature type. Reveal the top card of your library. If that card is a creature card of the chosen type, put it into your hand. Otherwise,
	 * put it into your graveyard. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int subt = select_a_subtype_full_choice(instance->parent_controller, instance->parent_card, instance->parent_controller, NULL, 0);
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			reveal_card_iid(player, card, deck[0]);
			if( is_what(-1, deck[0], TYPE_CREATURE) && has_subtype(-1, deck[0], subt) ){
				add_card_to_hand(player, deck[0]);
				remove_card_from_deck(player, 0);
			}
			else{
				mill(player, 1);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK, MANACOST0, 0, NULL, NULL);
}

int card_bloodstained_mire(int player, int card, event_t event){
	/* Bloodstained Mire	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Swamp or |H2Mountain card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN, 1);
}

int card_boneknitter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ZOMBIE;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && ( land_can_be_played & LCBP_REGENERATION) && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && can_target(&td) ){
		return 0x63;
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			if( ! can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return morph(player, card, event, 2, 1, 0, 0, 0, 0);
}

int card_brightstone_ritual(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_RED, count_subtype(2, TYPE_PERMANENT, SUBTYPE_GOBLIN));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_broodhatch_nantuko(int player, int card, event_t event){

	int m = morph(player, card, event, 2, 0, 0, 1, 0, 0);
	if (event == EVENT_CAN_UNMORPH){
		return m;
	}

	return card_saber_ants(player, card, event);
}

int card_cabal_archon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) &&
		can_target(&td)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_CLERIC, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_CLERIC, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
	}

	return 0;
}

int card_catapult_master(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_SOLDIER;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 4 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td2);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( tapsubtype_ability(player, card, 5, &td1) ){
				if( pick_target(&td2, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_catapult_squad(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_SOLDIER;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.required_state = TARGET_STATE_IN_COMBAT;
	td2.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 1 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td2);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( tapsubtype_ability(player, card, 2, &td1) ){
				if( pick_target(&td2, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
		}
	}

	return 0;
}

int card_centaur_glade(int player, int card, event_t event){
	/* Centaur Glade	|3|G|G
	 * Enchantment
	 * |2|G|G: Put a 3/3 |Sgreen Centaur creature token onto the battlefield. */

	// original code : 004E1DEB

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 2, 0, 0, 2, 0, 0) ){
			return 1;
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 2, 0, 0);
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			generate_token_by_id(player, card, CARD_ID_CENTAUR);
	}

	return global_enchantment(player, card, event);
}

int card_choking_tethers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		return cycling(player, card, event, MANACOST_XU(1, 1));
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, MANACOST_XU(1, 1));
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}


	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 4, NULL);
}

int card_contested_cliffs(int player, int card, event_t event){
	/* Contested Cliffs	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |R|G, |T: Target Beast creature you control fights target creature an opponent controls. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = 1-player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE );
	td2.preferred_controller = player;
	td2.allowed_controller = player;
	td2.required_subtype = SUBTYPE_BEAST;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 1, 0) && can_use_activated_abilities(player, card) &&
			can_target(&td) && can_target(&td2)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Beast fighting\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			instance->number_of_targets = 0;
			if (charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 1, 0)
				&& pick_next_target_noload(&td2, "Select a Beast creature you control.")
				&& new_pick_target(&td, "TARGET_CREATURE", 1, 1) ){
				instance->info_slot = 1;
			}
			else{
				untap_card_no_event(player, card);
				instance->number_of_targets = 0;
				spell_fizzled = 1;
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( validate_target(player, card, &td2, 0) && validate_target(player, card, &td, 1) ){
					fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
				}
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

int card_convalescent_care(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( life[player] < 6 ){
			gain_life(player, 3);
			draw_cards(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_cover_of_darkness(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = select_a_subtype(player, card);
	}

	return card_intimidation(player, card, event);
}

int card_crown_of_ascension(int player, int card, event_t event){
	return crown(player, card, event, 0, 0, KEYWORD_FLYING, 0);
}

int card_crown_of_awe(int player, int card, event_t event){
	return crown(player, card, event, 0, 0, KEYWORD_PROT_BLACK+KEYWORD_PROT_RED, 0);
}

int card_crown_of_fury(int player, int card, event_t event){
	return crown(player, card, event, 1, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_crown_of_suspicion(int player, int card, event_t event){
	return crown(player, card, event, 2, -1, 0, 0);
}

int card_crown_of_vigor(int player, int card, event_t event){
	return crown(player, card, event, 1, 1, 0, 0);
}

int card_cruel_revival(int player, int card, event_t event){

	/* Cruel Revival	|4|B
	 * Instant
	 * Destroy target non-Zombie creature. It can't be regenerated. Return up to one target Zombie card from your graveyard to your hand. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_ZOMBIE;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_PERMANENT, "Select target Zombie card.");
			this_test.subtype = SUBTYPE_ZOMBIE;
			if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(2) ){
				select_target_from_grave_source(player, card, player, SFG_NO_SPELL_FIZZLED, AI_MAX_VALUE, &this_test, 1);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		int selected = validate_target_from_grave_source(player, card, player, 1);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cryptic_gateway(int player, int card, event_t event){

	/* Cryptic Gateway	|5
	 * Artifact
	 * Tap two untapped creatures you control: You may put a creature card from your hand that shares a creature type with each creature tapped this way onto
	 * the battlefield. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_state = TARGET_STATE_TAPPED;
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td) > 1 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int total = 0;
			while( total < 2 && can_target(&td) ){
					if( select_target(player, card, &td, "Select a creature.", &(instance->targets[total])) ){
						instance->number_of_targets = 1;
						state_untargettable(instance->targets[total].player, instance->targets[total].card, 1);
						total++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<total; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				if( total == 2 ){
					tap_card(instance->targets[i].player, instance->targets[i].card);
				}
			}
			if( total != 2 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		int selected = new_global_tutor(player, player, TUTOR_FROM_HAND, 0, 0, AI_MAX_CMC, &this_test);
		if( selected != -1 ){
			if( shares_creature_subtype(player, selected, instance->targets[0].player, instance->targets[0].card) &&
				shares_creature_subtype(player, selected, instance->targets[1].player, instance->targets[1].card)
			  ){
				put_into_play(player, selected);
			}
		}
	}

	return 0;
}

int card_death_match(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.who_chooses = instance->targets[1].player;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
		}
	}

	return global_enchantment(player, card, event);
}

int card_death_pulse(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, -4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, 2, 0, 0, 0, 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}

		charge_mana_multi(player, colorless, 2, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return 0;
}

int card_demystify(int player, int card, event_t event ){
	/*
	  Demystify |W
	  Instant
	  Destroy target enchantment.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_dirge_of_the_dread(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 2, -1, 0, 0, 0, SP_KEYWORD_FEAR);
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, 1, 0, 0, 0, 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}

		charge_mana_multi(player, colorless, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_FEAR);
		}
	}

	return 0;
}

int card_discombobulate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		rearrange_top_x(player, player, 4);
		return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_dispersing_orb(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_disruptive_pitmage(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 1);
	}

	return morph(player, card, event, 0, 0, 1, 0, 0, 0);
}

int card_doomed_necromancer(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_B(1), 0, NULL, NULL) ){
			if( has_dead_creature(player) && ! graveyard_has_shroud(player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			int selected = select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
			if( selected != -1 ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
	}

	return 0;
}

int card_doubtless_one(int player, int card, event_t event){

	spirit_link_effect(player, card, event, player);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		event_result+=count_subtype(2, TYPE_PERMANENT, SUBTYPE_CLERIC);
	}

	return 0;
}

int card_dragon_roost(int player, int card, event_t event){
	/* Dragon Roost	|4|R|R
	 * Enchantment
	 * |5|R|R: Put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */
	// original code : 004E1EEE

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 2, 0) ){
		return 1;
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 5, 0, 0, 0, 2, 0);
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			generate_token_by_id(player, card, CARD_ID_DRAGON);
	}

	return global_enchantment(player, card, event);
}

// dream chisel --> vanilla

int card_dwarven_blastminer(int player, int card, event_t event){

	return card_dwarven_miner(player, card, event) + morph(player, card, event, 0, 0, 0, 0, 1, 0);
}

int card_ebonblade_reaper(int player, int card, event_t event)
{
  /* Ebonblade Reaper	|2|B
   * Creature - Human Cleric 1/1
   * Whenever ~ attacks, you lose half your life, rounded up.
   * Whenever ~ deals combat damage to a player, that player loses half his or her life, rounded up.
   * Morph |3|B|B */

  if (declare_attackers_trigger(player, card, event, DAT_STORE_IN_TARGETS_3, player, card))
	lose_life(player, (life[player] + 1) / 2);

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  int p;
	  for (p = 0; p <= 1; ++p)
		for (; times_damaged[p] > 0; --times_damaged[p])
		  lose_life(p, (life[p] + 1) / 2);
	}

  return morph(player, card, event, MANACOST_XB(3,2));
}

int card_elvish_vanguard(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_PERMANENT);
	this_test.subtype = SUBTYPE_ELF;
	this_test.not_me = 1;

	if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_elvish_pioneer(int player, int card, event_t event){
	/*
	  Elvish Pioneer |G
	  Creature - Elf Druid 1/1
	  When Elvish Pioneer enters the battlefield, you may put a basic land card from your hand onto the battlefield tapped.
	*/
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
		test.zone = TARGET_ZONE_HAND;
		if( check_battlefield_for_special_card(player, card, player, 0, &test) ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &test);
		}
	}

	return 0;
}

int card_enchantresss_presence(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY){
		return global_enchantment(player, card, event);
	}
	return card_argothian_enchantress(player, card,  event);
}


int card_entrails_feaster(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int tp = 1;
		char msg[100] = "Select a creature card to exile.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1){
			add_1_1_counter(player, card);
			tp = 0;
		}
		if( tp ){
			tap_card(player, card);
		}
	}

	return 0;
}

int card_erratic_explosion(int player, int card, event_t event){

	/* Erratic Explosion	|2|R
	 * Sorcery
	 * Choose target creature or player. Reveal cards from the top of your library until you reveal a nonland card. ~ deals damage equal to that card's
	 * converted mana cost to that creature or player. Put the revealed cards on the bottom of your library in any order. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				int dmg = 0;
				int count = 0;
				while( deck[count] != -1 ){
						if( ! is_what(-1, deck[count], TYPE_LAND) ){
							break;
						}
						count++;
				}
				show_deck(player, deck, count+1, "Erratic Explosion revealed these cards.", 0, 0x7375B0);
				show_deck(1-player, deck, count+1, "Erratic Explosion revealed these cards.", 0, 0x7375B0);
				dmg = get_cmc_by_internal_id(deck[count]);
				if( dmg > 0 ){
					damage_creature_or_player(player, card, event, dmg);
				}
				put_top_x_on_bottom(player, player, count+1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_exalted_angel(int player, int card, event_t event){
	spirit_link_effect(player, card, event, player);
	return morph(player, card, event, 2, 0, 0, 0, 0, 2);
}

int card_explosive_vegetation(int player, int card, event_t event){
	/*
	  Explosive Vegetation |3|G
	  Sorcery
	  Search your library for up to two basic land cards and put them onto the battlefield tapped. Then shuffle your library.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
		test.subtype = SUBTYPE_BASIC;
		test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_false_cure(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int legacy = create_legacy_effect(player, card, &until_eot_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[3].card = CARD_ID_FALSE_CURE;
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_festering_goblin(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return 0;
}

int card_flooded_strand(int player, int card, event_t event){
	/* Flooded Strand	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Plains or |H2Island card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_PLAINS, SUBTYPE_ISLAND, 1);
}

int card_forgotten_cave(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if( in_play(player, card ) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND ) {
		return mana_producer(player, card, event);
	}
	else{
		return cycling(player, card, event, 0, 0, 0, 0, 1, 0);
	}
}

int card_future_sight(int player, int card, event_t event){
	reveal_top_card(player, card, event);

	card_instance_t* instance = get_card_instance(player, card);

	// see if we can play the top card of the deck
	int *deck = deck_ptr[player];
	int id = deck[0] != -1 ? cards_data[ deck[0] ].id : -1;
	int can_cast;

	if( event == EVENT_CAN_ACTIVATE && id != -1 && ! is_humiliated(player, card) && (can_cast = can_legally_play_iid_now(player, deck[0], event)) &&
		has_mana_to_cast_id(player, event, id) && instance->targets[0].player != 1
	  ){
		return can_cast;
	}
	else if( event == EVENT_ACTIVATE ){
			instance->targets[0].player = 1;
			charge_mana_from_id(player, -1, event, id);
			instance->targets[0].player = 0;
			if (cancel != 1){
				play_card_in_deck_for_free(player, player, 0);
				cant_be_responded_to = 1;
			}
	}

	return global_enchantment(player, card, event);
}

int card_gangrenous_goliath(int player, int card, event_t event)
{
  if (event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS)
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.preferred_controller = player;
	  td.allowed_controller = player;
	  td.illegal_state = TARGET_STATE_TAPPED;
	  td.required_subtype = SUBTYPE_CLERIC;

	  if (event == EVENT_GRAVEYARD_ABILITY && target_available(player, card, &td) >= 3)
		return GA_RETURN_TO_HAND;

	  if (event == EVENT_PAY_FLASHBACK_COSTS && pick_up_to_n_targets_noload(&td, "Select an untapped Cleric.", 3) == 3)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int i;
		  for (i = 0; i < 3; ++i)
			tap_card(instance->targets[i].player, instance->targets[i].card);

		  instance->number_of_targets = 0;
		  return GAPAID_REMOVE;
		}
	}

  return 0;
}

int card_gigapede(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && hand_count[player] > 0 ){
		int choice = do_dialog(player, player, card, -1, -1," Return Gigapede to hand\n Pass\n", 0);
		if( choice == 0 ){
			discard(player, 0, player);
			instance->state &= ~STATE_INVISIBLE;
			hand_count[player]++;
			return -1;
		}
		else{
			return -2;
		}
	}
	return 0;
}

int card_goblin_burrows(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( (event == EVENT_COUNT_MANA && affect_me(player, card)) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_GOBLIN;

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(2, 1), 0, &td, "TARGET_CREATURE") ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump a Goblin\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = instance->info_slot = 0;
			instance->state |= STATE_TAPPED;
			if (charge_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
			else{
				untap_card_no_event(player, card);
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
			}
		}
	}

	return 0;
}

int card_goblin_piledriver( int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +2/+0 until end of turn for each other attacking Goblin.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  test.subtype = SUBTYPE_GOBLIN;
	  test.not_me = 1;
	  int num = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test);

	  pump_until_eot(player, card, player, card, 2 * num, 0);
	}

  return 0;
}

int card_goblin_pyromancer(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		pump_subtype_until_eot(player, card, 2, SUBTYPE_GOBLIN, 3, 0, 0, 0);
	}

	if( eot_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_GOBLIN;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return 0;
}

int card_goblin_sharpshooter( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	does_not_untap(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		untap_card(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_goblin_sledder(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select a Goblin to sacrifice.");
	test.subtype = SUBTYPE_GOBLIN;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td1) ){
		return new_can_sacrifice_as_cost(player, card, &test);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( new_sacrifice(player, card, player, 0, &test) ){
			instance->number_of_targets = 0;
			if (can_target(&td1) && pick_target(&td1, "TARGET_CREATURE")){
				return 0;
			}
		}
		spell_fizzled = 1;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return 0;
}

int card_goblin_taskmaster(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_GOBLIN;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td1, "TARGET_CREATURE");
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td1, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0);
		}
	}

	return morph(player, card, event, 0, 0, 0, 0, 1, 0);
}

int card_grand_coliseum(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == HUMAN || (player == AI && life[player] > 5) ){
			return mana_producer(player, card, event);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].card = mana_pool[COLOR_BLACK+8*player] + mana_pool[COLOR_BLUE+8*player] + mana_pool[COLOR_GREEN+8*player] + mana_pool[COLOR_RED+8*player] + mana_pool[COLOR_WHITE+8*player];
		mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		if( instance->targets[1].card <  mana_pool[COLOR_BLACK+8*player] + mana_pool[COLOR_BLUE+8*player] + mana_pool[COLOR_GREEN+8*player] + mana_pool[COLOR_RED+8*player] + mana_pool[COLOR_WHITE+8*player]){
			damage_player(player, 1, player, instance->parent_card);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( player == HUMAN || (player == AI && life[player] > 5) ){
			return mana_producer(player, card, event);
		}
	}
	return 0;
}

int card_gratuitous_violence(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_source_player == player
	  && is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE)
	  && (damage->damage_target_card == -1 || !is_planeswalker(damage->damage_target_player, damage->damage_target_card)))
	damage->info_slot *= 2;

  return global_enchantment(player, card, event);
}

int card_gravespawn_sovereign(int player, int card, event_t event){

	/* Gravespawn Sovereign	|4|B|B
	 * Creature - Zombie 3/3
	 * Tap five untapped Zombies you control: Put target creature card from a graveyard onto the battlefield under your control. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td1;
	base_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_ZOMBIE;
	td1.preferred_controller = player;
	td1.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && target_available(player, card, &td1) >= 5 &&
		CAN_ACTIVATE0(player, card) && has_dead_creature(2)
	  ){
		return ! graveyard_has_shroud(2);
	}

	if(event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");

		if (!(select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 6, 7) != -1 &&
			  tapsubtype_ability(player, card, 5, &td1)
		   )){
			cancel = 1;
		}
		instance->number_of_targets = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[6].player, 7);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[6].player, selected, REANIMATE_DEFAULT);
		}
	}

	return 0;
}

int card_grinning_demon(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 2);
	}
	return morph(player, card, event, 2, 2, 0, 0, 0, 0);
}

int card_haunted_cadaver(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int choice = do_dialog(player, player, card, -1, -1, " Sac and make opponent discard\n Pass", 0);
		if( choice == 0 ){
			new_multidiscard(1-player, 3, 0, player);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return morph(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_head_games(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = hand_count[1-player];
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card.");

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_PUT_ON_TOP+EC_ALL_WHICH_MATCH_CRITERIA;
			new_effect_coercion(&this_definition, &this_test);

			int *deck = deck_ptr[1-player];

			while( amount > 0){
					int selected = new_select_a_card(player, 1-player, TUTOR_FROM_DECK, 1, AI_MIN_VALUE, -1, &this_test);
					add_card_to_hand(1-player, deck[selected]);
					remove_card_from_deck(1-player, selected);
					amount--;
			}
			shuffle(1-player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_heedless_one(int player, int card, event_t event){
	/* Heedless One	|3|G
	 * Creature - Elf Avatar 100/100
	 * Trample
	 * ~'s power and toughness are each equal to the number of Elves on the battlefield. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_ELF);
	}

	return 0;
}

int card_headhunter(int player, int card, event_t event)
{
  /* Headhunter	|1|B
   * Creature - Human Cleric 1/1
   * Whenever ~ deals combat damage to a player, that player discards a card.
   * Morph |B */

  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, 0);

  return morph(player, card, event, MANACOST_B(1));
}

int card_hystrodon(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		draw_cards(player, 1);
	}
	return morph(player, card, event, 1, 0, 0, 2, 0, 0);
}

int card_information_dealer(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		rearrange_top_x(player, player, count_subtype(2, TYPE_PERMANENT, SUBTYPE_WIZARD));
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_infest(int player, int card, event_t event){
	/*
	  Infest |1|B|B
	  Sorcery
	  All creatures get -2/-2 until end of turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int p = i == 0 ? player : 1-player;
			pump_subtype_until_eot(player, card, p, -1, -2, -2, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_insurrection(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			insurrection_effect(player, card);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_invigorating_boon(int player, int card, event_t event){
	return global_enchantment(player, card, event);
}

int card_ironfist_crusher(int player, int card, event_t event)
{
  creature_can_block_additional(player, card, event, 255);
  return morph(player, card, event, MANACOST_XW(3,1));
}

int card_ixidor_reality_sculptor(int player, int card, event_t event)
{
  /* Ixidor, Reality Sculptor	|3|U|U
   * Legendary Creature - Human Wizard 3/4
   * Face-down creatures get +1/+1.
   * |2|U: Turn target face-down creature face up. */

  check_legend_rule(player, card, event);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && get_id(affected_card_controller, affected_card) == CARD_ID_FACE_DOWN_CREATURE
	  && !is_humiliated(player, card))
	event_result++;

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.extra = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  flip_card(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_XU(2, 1), 0, &td, "Select target face-down creature.");
}

int card_jareth_leonine_titan(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
		pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0, 0, keyword, 0 );
	}

	if( event == EVENT_DECLARE_BLOCKERS ){
		if( instance->blocking < 255 ){
			pump_until_eot(player, card, player, card, 7, 7 );
		}
	}
	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_kamahl_fist_of_krosa(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.illegal_type = TYPE_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE  ){
		if( has_mana_for_activated_ability(player, card, MANACOST_G(1)) && can_target(&td) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, MANACOST_XG(2, 3)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_G(1)) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_XG(2, 3)) ){
				if( count_subtype(player, TYPE_CREATURE, -1) > 2 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card , -1, -1, " Animate a land\n Overrun\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, MANACOST_XG((choice == 1 ? 2 : 0), (choice == 1 ? 3 : 1)));
			if( spell_fizzled != 1 ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_LAND") ){
						instance->info_slot = 66+choice;
						instance->number_of_targets = 1;
					}
				}
				if( choice == 1 ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			land_animation2(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1, 1, 0, 0, 0, 0);
		}
		if( instance->info_slot == 67 ){
			pump_subtype_until_eot(player, instance->parent_card, player, -1, 3, 3, KEYWORD_TRAMPLE, 0);
		}
	}
	return 0;
}

int card_krosan_colossus(int player, int card, event_t event){
	return morph(player, card, event, 6, 0, 0, 2, 0, 0);
}

int card_krosan_groundshaker(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_BEAST;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 1, 0, 0, 0, &td1, "TARGET_CREATURE");
}

int card_krosan_tusker(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		return cycling(player, card, event, 2, 0, 0, 1, 0, 0);
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, 2, 0, 0, 1, 0, 0);
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);
		int choice = IS_AI(player) ? 0 : do_dialog(player, player, card, -1, -1, " Search for a basic land\n Pass", 0);
		if( choice == 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.subtype = SUBTYPE_BASIC;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	return 0;
}

// lighting rift --> invigorting boon

int card_lonely_sandbar(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if( in_play(player, card ) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND ) {
		return mana_producer(player, card, event);
	}
	else{
		return cycling(player, card, event, 0, 0, 1, 0, 0, 0);
	}
}

int card_mana_echoes(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return 1;
	}

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int quantity = 0;
		if( check_for_ability(instance->targets[1].player, instance->targets[1].card, KEYWORD_PROT_INTERRUPTS)){
			quantity = count_permanents_by_type(player, TYPE_CREATURE);
		}
		else{
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) &&
					shares_creature_subtype(instance->targets[1].player, instance->targets[1].card, player, i)
				  ){
					quantity++;
				}
			}
		}
		produce_mana(instance->targets[1].player, COLOR_COLORLESS, quantity);
	}

   return global_enchantment(player, card, event);
}

int card_mobilization(int player, int card, event_t event){
	/* Mobilization	|2|W
	 * Enchantment
	 * Soldier creatures have vigilance.
	 * |2|W: Put a 1/1 |Swhite Soldier creature token onto the battlefield. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1) ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SOLDIER);
	}

	if( event == EVENT_ABILITIES && has_creature_type(affected_card_controller, affected_card, SUBTYPE_SOLDIER) && in_play(affected_card_controller, card) ){
		vigilance(affected_card_controller, affected_card, event);
	}

	return global_enchantment(player, card, event);
}

int card_mythic_proportions(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 8, 8, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_nameless_one(int player, int card, event_t event){
	/* Nameless One	|3|U
	 * Creature - Wizard Avatar 100/100
	 * ~'s power and toughness are each equal to the number of Wizards on the battlefield.
	 * Morph |2|U */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result+=count_subtype(2, TYPE_PERMANENT, SUBTYPE_WIZARD);
	}

	return morph(player, card, event, 2, 0, 1, 0, 0, 0);
}

int card_nantuko_husk(int player, int card, event_t event){
	// original code : 012039A8
	/* Nantuko Husk	|2|B
	 * Creature - Zombie Insect 2/2
	 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */
	/* Bloodthrone Vampire	|1|B
	 * Creature - Vampire 1/1
	 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */
	/* Phyrexian Ghoul	|2|B
	 * Creature - Zombie 2/2
	 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */
	/* Vampire Aristocrat	|2|B
	 * Creature - Vampire Rogue 2/2
	 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */
	return generic_husk(player, card, event, TYPE_CREATURE, 2, 2, 0, 0);
}

int card_nova_cleric(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, instance->parent_card, 2, &this_test, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_oblation(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int owner = get_owner(instance->targets[0].player, instance->targets[0].card);
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			shuffle(owner);
			draw_cards(owner, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 1, NULL);
}

int card_oversold_cemetery(int player, int card, event_t event){

	if( current_turn == player && !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0 && count_graveyard_by_type(player, TYPE_CREATURE) >= 4 ){
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

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if (count_graveyard_by_type(player, TYPE_CREATURE) >= 4){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return global_enchantment(player, card, event);
}

int card_overwhelming_instinct(int player, int card, event_t event)
{
  // Whenever you attack with three or more creatures, draw a card.
  if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_WITH_3_OR_MORE, player, -1))
	draw_a_card(player);

  return global_enchantment(player, card, event);
}

int card_patriarchs_bidding(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int subtype_chosen[2];
		APNAP(p, {subtype_chosen[p] = select_a_subtype_full_choice(player, card, p, get_grave(p), 0);};);
		APNAP(p, {
				test_definition_t test;
				new_default_test_definition(&test, TYPE_PERMANENT, "");
				test.subtype = subtype_chosen[0];
				test.sub2 = subtype_chosen[1];
				test.subtype_flag = F2_MULTISUBTYPE;
				new_reanimate_all(p, -1, p, &test, REANIMATE_DEFAULT);
				};
		);
		kill_card( player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static const char* target_is_attached_to_creature(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->damage_target_card >= 0 && is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE))
	return NULL;
  else
	return "attached to a creature";
}
int card_piety_charm(int player, int card, event_t event)
{
  /* Piety Charm	|W
   * Instant
   * Choose one - Destroy target Aura attached to a creature; or target Soldier creature gets +2/+2 until end of turn; or creatures you control gain vigilance
   * until end of turn. */

  if (IS_CASTING(player, card, event))
	{
	  target_definition_t td_aura;
	  default_target_definition(player, card, &td_aura, TYPE_ENCHANTMENT);
	  td_aura.required_subtype = SUBTYPE_AURA;
	  td_aura.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td_aura.extra = (int)target_is_attached_to_creature;

	  target_definition_t td_soldier;
	  default_target_definition(player, card, &td_soldier, TYPE_CREATURE);
	  td_soldier.required_subtype = SUBTYPE_SOLDIER;
	  td_soldier.preferred_controller = player;

	  card_instance_t* instance = get_card_instance(player, card);

	  enum
	  {
		CHOICE_AURA = 1,
		CHOICE_SOLDIER,
		CHOICE_VIGILANCE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Destroy an Aura", 1, 10, DLG_LITERAL_TARGET(&td_aura, "Select target Aura attached to a creature."),
						"Soldier gets +2/+2", 1, 10, DLG_LITERAL_TARGET(&td_soldier, "Select target Soldier creature."),
						"Vigilance", 1, creature_count[player] > 0 && current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS ? 10 : 1);

	  if (event == EVENT_CAN_CAST)
		return choice;
	  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
		{
		  if (choice == CHOICE_VIGILANCE && player == AI)
			{
			  if (current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS)
				{
				  int c;
				  card_instance_t* inst;
				  for (c = 0; c < active_cards_count[player]; ++c)
					if ((inst = in_play(player, c)) && is_what(player, c, TYPE_CREATURE) && CAN_TAP(player, c) && !is_humiliated(player, c)
						&& !(inst->state & STATE_VIGILANCE))
					  ai_modifier += 12;
				}
			  else
				ai_modifier -= 48;	// discourage casting on opponent's turn or after combat, but allow e.g. for Black Vise
			}
		}
	  else	// EVENT_RESOLVE_SPELL
		{
		  switch (choice)
			{
			  case CHOICE_AURA:
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				break;

			  case CHOICE_SOLDIER:
				alternate_legacy_text(1, player, pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2,2));
				break;

			  case CHOICE_VIGILANCE:
				pump_creatures_until_eot(player, card, player, 2, 0,0, 0,SP_KEYWORD_VIGILANCE, NULL);
				break;
			}

		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_polluted_delta(int player, int card, event_t event){
	/* Polluted Delta	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Han Island or |H2Swamp card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_ISLAND, SUBTYPE_SWAMP, 1);
}

int card_primal_boost(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, 0, 0, 1, 0, 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}

		charge_mana_multi(player, colorless, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return 0;
}

int card_profane_prayers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_CLERIC);
			damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ravenous_baloth(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 4);
	}

	return altar_extended(player, card, event, 0, TYPE_PERMANENT, 0, SUBTYPE_BEAST, 0, 0, 0, 0, 0, -1, 0);
}


int card_read_the_runes(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			draw_cards(player, 1);
			int choice = 0;
			if( count_permanents_by_type(player, TYPE_PERMANENT) > 0 ){
				choice = do_dialog(player, player, card, -1, -1, " Discard\n Sac a permanent", 0);
			}
			if( choice == 0 ){
				discard(player, 0, player);
			}
			else{
				sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reckless_one(int player, int card, event_t event){
	/* Reckless One	|3|R
	 * Creature - Goblin Avatar 100/100
	 * Haste
	 * ~'s power and toughness are each equal to the number of Goblins on the battlefield. */

	haste(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_GOBLIN);
	}

	return 0;
}

int card_reminisce(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			reshuffle_grave_into_deck(instance->targets[0].player, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_renewed_faith(int player, int card, event_t event){//UNUSEDCARD

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 6);
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, 0, 0, 0, 0, 1) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		int colorless = 1;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}

		charge_mana_multi(player, colorless, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);
		gain_life(player, 2);
	}

	return 0;
}

int card_riptide_laboratory(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT );
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.required_subtype = SUBTYPE_WIZARD;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0) && can_use_activated_abilities(player, card) &&
			can_target(&td1)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Bounce a wizard\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WIZARD) ){
					instance->info_slot = 1;
				}
				else{
					 untap_card_no_event(player, card);
					 spell_fizzled = 1;
				}
			}
			else{
				 untap_card_no_event(player, card);
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td1) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
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

int card_riptide_replicator(int player, int card, event_t event){

	/* Riptide Replicator	|X|4
	 * Artifact
	 * As ~ enters the battlefield, choose a color and a creature type.
	 * ~ enters the battlefield with X charge counters on it.
	 * |4, |T: Put an X/X creature token of the chosen color and type onto the battlefield, where X is the number of charge counters on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_ARTIFACT, -1);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		add_counters(player, card, COUNTER_CHARGE, instance->info_slot);
		instance->targets[0].card = select_a_subtype(player, card);
		instance->targets[0].player = 1<<choose_a_color_and_show_legacy(player, card, player, -1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_RIPTIDE_REPLICATOR_TOKEN, &token);
		token.pow = token.tou = count_counters(player, card, COUNTER_CHARGE);
		token.color_forced = instance->targets[0].player;
		token.no_sleight = 1;
		token.action = TOKEN_ACTION_ADD_SUBTYPE;
		token.action_argument = instance->targets[0].card;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_riptide_shapeshifter(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int subt = select_a_subtype(player, card);
			int count = 0;
			while( deck[count] != -1 ){
					if( has_subtype(-1, deck[count], subt) && is_what(-1, deck[count], TYPE_CREATURE) ){
						break;
					}
					count++;
			}
			show_deck( player, deck, count, "Cards revealed by Riptide Replicator.", 0, 0x7375B0 );
			if( has_subtype(-1, deck[count], subt) && is_what(-1, deck[count], TYPE_CREATURE) ){
				put_into_play_a_card_from_deck(player, player, count);
			}
			shuffle(player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 2, 0, 2, 0, 0, 0, 0, 0, 0);
}

int card_rorix_bladewing(int player, int card, event_t event)
{
  /* Rorix Bladewing	|3|R|R|R
   * Legendary Creature - Dragon 6/5
   * Flying, haste */

  check_legend_rule(player, card, event);
  haste(player, card, event);
  return 0;
}

static int rotlung_reanimator_legacy(int player, int card, event_t event){

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
						if( get_card_instance(p, c)->kill_code < KILL_REMOVE ){
							if( instance->targets[11].player < 0 ){
								instance->targets[11].player = 0;
							}
							instance->targets[11].player++;
						}
					}
				}
				else{
					test_definition_t this_test;
					default_test_definition(&this_test, TYPE_PERMANENT);
					this_test.subtype = SUBTYPE_CLERIC;

					count_for_gfp_ability(player, card, event, instance->targets[0].player, 0, &this_test);
				}
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
							default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
							token.t_player = instance->targets[0].player;
							token.qty = instance->targets[11].player;
							token.color_forced = COLOR_TEST_BLACK;
							if (instance->targets[1].card != 66){
								token.color_forced = get_sleighted_color_test(p, c, COLOR_TEST_BLACK);
								token.no_sleight = 1;
							}
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

int card_rotlung_reanimator(int player, int card, event_t event){
	/* Rotlung Reanimator	|2|B
	 * Creature - Zombie Cleric 2/2
	 * Whenever ~ or another Cleric dies, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &rotlung_reanimator_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->token_status |= STATUS_INVISIBLE_FX;
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
	}
	return 0;
}

int card_seaside_haven(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 1) && can_use_activated_abilities(player, card) &&
			can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_BIRD, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac a Bird & Draw\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_BIRD) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
					instance->info_slot = 1;
				}
				else{
					 spell_fizzled = 1;
				}
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				draw_cards(player, 1);
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

int card_secluded_steppe(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if( in_play(player, card ) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND ) {
		return mana_producer(player, card, event);
	}
	else{
		return cycling(player, card, event, 0, 0, 0, 0, 0, 1);
	}
}

int card_shared_triumph(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[0].card = select_a_subtype(player, card);
	}

	if( instance->targets[0].card > 0 && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		has_subtype(affected_card_controller, affected_card, instance->targets[0].card)
	  ){
		event_result++;
	}

	return global_enchantment(player, card, event);
}

int card_shepherd_of_rot(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_ZOMBIE);
		lose_life(player, amount);
		lose_life(1-player, amount);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_sigil_of_the_new_dawn(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			! is_token(affected_card_controller, affected_card)
		  ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				if( instance->targets[0].player < 1 ){
					instance->targets[0].player = 1;
				}
				int pos = instance->targets[0].player;
				instance->targets[pos].card = get_original_id(affected_card_controller, affected_card);
				instance->targets[0].player++;
			}
		}
	}

	if( has_mana_multi(player, 1, 0, 0, 0, 0, 1) && instance->targets[0].player > 1 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller){
		if( affect_me(player, card ) ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					while( has_mana_multi(player, 1, 0, 0, 0, 0, 1) && instance->targets[0].player > 1 ){
						char buffer[500];
						int pos = scnprintf(buffer, 500, " Pass\n");
						int i;
						for(i=1; i<instance->targets[0].player; i++){
							if( instance->targets[i].card != -1 ){
								card_ptr_t* c = cards_ptr[ instance->targets[i].card ];
								pos += scnprintf(buffer + pos, 500-pos, " Bring back %s\n", c->name );
							}
						}
						int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
						if( choice > 0 ){
							if( instance->targets[choice].card != -1 ){
								int count = count_graveyard(player)-1;
								const int *grave = get_grave(player);
								while( count > -1 ){
										if( cards_data[grave[count]].id == instance->targets[choice].card ){
											add_card_to_hand(player, grave[count]);
											remove_card_from_grave(player, count);
											break;
										}
										count--;
								}
							}
							int k;
							for(k=choice; k<instance->targets[0].player; k++){
								instance->targets[k].card = instance->targets[k+1].card;
							}
							instance->targets[0].player--;
						}
						else{
							break;
						}
					}
					instance->targets[0].player = 1;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[0].player = 1;
	}

	return global_enchantment(player, card, event);
}

int card_silent_specter(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		new_multidiscard(1-player, 2, 0, player);
	}

	return morph(player, card, event, 3, 2, 0, 0, 0, 0);
}

int card_silklash_spider(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, instance->parent_card, 2, instance->info_slot, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, 0, -1, 0, 0, 2, 0, 0, 0, 0, 0);
}

int card_silvos_rogue_elemental(int player, int card, event_t event)
{
  /* Silvos, Rogue Elemental	|3|G|G|G
   * Legendary Creature - Elemental 8/5
   * Trample
   * |G: Regenerate ~. */

  check_legend_rule(player, card, event);
  return regeneration(player, card, event, MANACOST_G(1));
}

int card_skirk_fire_marshal(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_GOBLIN;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 4 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! tapsubtype_ability(player, card, 5, &td1) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, instance->parent_card, 2, 10, NDA_PLAYER_TOO, &this_test);
	}

	return 0;
}

int card_skirk_prospector(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
			produce_mana(player, COLOR_RED, 1);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, COLOR_RED, 1);
		}
	}
	return 0;
}

int card_slate_of_ancestry(int player, int card, event_t event){

	if(event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_animated_and_sick(player, card) && ! is_tapped(player, card) ){
			return has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( player == AI && count_subtype(player, TYPE_CREATURE, -1) < hand_count[player] ){
			ai_modifier-=50;
		}
		charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
		if(spell_fizzled != 1){
			tap_card(player, card);
			discard_all(player);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && affect_me(player, card) ){
		draw_cards(player, count_subtype(player, TYPE_CREATURE, -1));
	}

	return 0;
}

int card_slice_and_dice(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 2, 4, NDA_ALL_CREATURES, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		int colorless = 2;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, 0, 0, 0, 1, 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		int colorless = 2;
		int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		colorless -=amount;

		if( colorless < 0 ){
			colorless = 0;
		}

		charge_mana_multi(player, colorless, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 2, 1, NDA_ALL_CREATURES, &this_test);
	}

	return 0;
}

int card_smother(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td) ){
			if( player != AI ){
				int i;
				for(i=0; i<2; i++){
					int count = 0;
					while( count < active_cards_count[i] ){
							if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
								if( ! is_protected_from_me(player, card, i, count) ){
									if( get_cmc(i, count) < 4 ){
										return 1;
									}
								}
							}
							count++;
					}
				}
			}
			else{
				int i = 1-player;
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							if( ! is_protected_from_me(player, card, i, count) ){
								if( get_cmc(i, count) < 4  ){
									return 1;
								}
							}
						}
						count++;
				}

			}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( player != AI ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					if( get_cmc(instance->targets[0].player, instance->targets[0].card) > 3 ){
						spell_fizzled = 1;
					}
				}
			}
			else{
				int i = 1-player;
				int count = 0;
				int b_value = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							if( ! is_protected_from_me(player, card, i, count) ){
								if( get_cmc(i, count) < 4 ){
									if( get_base_value(i, count) > b_value ){
										b_value = get_base_value(i, count);
										instance->targets[0].player = 1-player;
										instance->targets[0].card = count;
									}
								}
							}
						}
						count++;
				}
			}
	}

	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_snarling_undorak(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_BEAST;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0) ){
		return can_target(&td1);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			if( pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return morph(player, card, event, 1, 0, 0, 2, 0, 0);
}

int card_solar_blast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		return cycling(player, card, event, 1, 0, 0, 0, 2, 0);
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, 1, 0, 0, 0, 2, 0);
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return card_lightning_bolt(player, card, event);
}

int card_soulless_one(int player, int card, event_t event){
	/* Soulless One	|3|B
	 * Creature - Zombie Avatar 100/100
	 * ~'s power and toughness are each equal to the number of Zombies on the battlefield plus the number of Zombie cards in all graveyards. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_ZOMBIE);
		event_result += count_graveyard_by_subtype(ANYBODY, SUBTYPE_ZOMBIE);
	}

	return 0;
}

int card_sparksmith(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_subtype(2, TYPE_PERMANENT, SUBTYPE_GOBLIN), player, instance->parent_card);
			damage_player(player, count_subtype(2, TYPE_PERMANENT, SUBTYPE_GOBLIN), player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_starlit_sanctum(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;
		int ai_choice = 2;
		if( life[player] < 6 ){
			ai_choice = 1;
		}
		if( ! paying_mana() && can_use_activated_abilities(player, card) &&
			can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, SUBTYPE_CLERIC, 0, 0, 0, 0, 0, -1, 0)
		  ){
			char buffer[500];
			int pos = 0;
			int mode = (1<<0)+(1<<3);
			pos += scnprintf(buffer + pos, 500-pos, " Produce mana\n", buffer);
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Sac & gain life\n", buffer);
				mode+=(1<<1);
			}
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Sac & make opponent lose life\n", buffer);
				mode+=(1<<2);
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
		}
			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else if( choice == 0 ){
					return mana_producer(player, card, event);
			}
			else{
				int white = 1;
				int black = 0;
				if( choice == 2 ){
					white = 0;
					black = 1;
				}
				charge_mana_for_activated_ability(player, card, 0, black, 0, 0, 0, white);
				if( spell_fizzled != 1 ){
					if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, SUBTYPE_CLERIC, 0, 0, 0, 0, 0, -1, 0) ){
						if( choice == 1 ){
							instance->info_slot = choice;
						}
						if( choice == 2 && pick_target(&td1, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
							instance->info_slot = choice;
						}
					}
					else{
						spell_fizzled = 1;
					}
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				if( instance->info_slot == 1 ){
					gain_life(player, instance->targets[2].card);
				}
				if( instance->info_slot == 2 && valid_target(&td1) ){
					lose_life(instance->targets[0].player, instance->targets[2].player);
				}
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

int card_starstorm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;

	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( instance->info_slot > 0 ){
			   damage_all(player, card, player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			   damage_all(player, card, 1-player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 3, 0, 0, 0, 0, 0);
}

int card_steely_resolve(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[0].card = select_a_subtype(player, card);
	}

	if( instance->targets[0].card > 0 && event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		has_subtype(affected_card_controller, affected_card, instance->targets[0].card)
	  ){
		event_result |= KEYWORD_SHROUD;
	}

	return global_enchantment(player, card, event);
}

int card_supreme_inquisitor(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_WIZARD;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allow_cancel = 0;
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 4 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td2);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( tapsubtype_ability(player, card, 5, &td1) ){
				if( pick_target(&td2, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			this_test.qty = 5;
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
			shuffle(instance->targets[0].player);
		}
	}

	return 0;
}

int card_symbiotic_beast(int player, int card, event_t event){
	token_generation_t token;
	default_token_definition(player, card, CARD_ID_INSECT, &token);
	token.qty = 4;
	return symbiotic_creature(player, card, event, &token);
}

int card_symbiotic_elf(int player, int card, event_t event){
	token_generation_t token;
	default_token_definition(player, card, CARD_ID_INSECT, &token);
	token.qty = 2;
	return symbiotic_creature(player, card, event, &token);
}

int card_symbiotic_wurm(int player, int card, event_t event){
	// original code : 004E0DDC
	token_generation_t token;
	default_token_definition(player, card, CARD_ID_INSECT, &token);
	token.qty = 7;
	return symbiotic_creature(player, card, event, &token);
}

int card_tempting_wurm(int player, int card, event_t event){

	/* Tempting Wurm	|1|G
	 * Creature - Wurm 5/5
	 * When ~ enters the battlefield, each opponent may put any number of artifact, creature, enchantment, and/or land cards from his or her hand onto the
	 * battlefield. */

	if (comes_into_play(player, card, event)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a permanent to put into play.");
		this_test.type_flag = F1_NO_PWALKER;
		this_test.zone = TARGET_ZONE_HAND;
		while( check_battlefield_for_special_card(player, card, 1-player, 0, &this_test) ){
			if( new_global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test) == -1 ){
				break;
			}
		}
	}

	return 0;
}

int card_tephraderm(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player ){
					int good = damage->info_slot;
					if( good < 1){
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = trg->targets[16].player;
						}
					}

					if( good > 0 ){
						if( instance->info_slot < 0 ){
							instance->info_slot = 0;
						}
						if( instance->info_slot < 10 ){
							instance->targets[instance->info_slot].player = damage->damage_source_player;
							if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
								instance->targets[instance->info_slot].card = damage->damage_source_card;
							}
							else{
								instance->targets[instance->info_slot].card = -1;
							}
							instance->targets[instance->info_slot+1].player = good;
							instance->info_slot+=2;
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
				for(i=0;i<instance->info_slot;i+=2){
					damage_creature(instance->targets[i].player, instance->targets[i].card, instance->targets[i+1].player, player, card);
				}
				instance->info_slot = 0;
		}
	}
	return 0;
}

int card_tranquil_thicket(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	if( in_play(player, card ) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND ) {
		return mana_producer(player, card, event);
	}
	else{
		return cycling(player, card, event, 0, 0, 0, 1, 0, 0);
	}
}

int card_tribal_unity(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			pump_subtype_until_eot(player, card, 2, select_a_subtype(player, card), instance->info_slot, instance->info_slot, 0, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_true_believer(int player, int card, event_t event){
	give_shroud_to_player(player, card, event);
	return 0;
}

int card_undead_gladiator(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( player == AI ){
		if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && hand_count[player] > 0 && has_mana_multi(player, 1, 1, 0, 0, 0, 0) ){
			int choice = do_dialog(player, player, card, -1, -1," Return Undead Gladiator to hand\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, 1, 1, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					discard(player, 0, player);
					instance->state &= ~STATE_INVISIBLE;
					hand_count[player]++;
					return -1;
				}
				else{
					return -2;
				}
			}
			else{
				return -2;
			}
		}
	}
	else{
		if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XB(1, 1)) && current_phase == PHASE_UPKEEP && hand_count[player] ){
			return GA_RETURN_TO_HAND;
		}
		if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, MANACOST_XB(1, 1));
			if( spell_fizzled != 1 ){
				discard(player, 0, player);
				return GAPAID_REMOVE;
			}
		}
	}
	return cycling(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_unholy_grotto(int player, int card, event_t event){

	char msg[100] = "Select a Zombie card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
	this_test.subtype = SUBTYPE_ZOMBIE;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_use_activated_abilities(player, card) &&
			new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(2)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Return a Zombie to deck\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				else{
					 spell_fizzled = 1;
				}
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				int selected = validate_target_from_grave(player, card, player, 0);
				if( selected != -1 ){
					const int *grave = get_grave(player);
					int card_added = add_card_to_hand(player, grave[selected]);
					remove_card_from_grave(player, selected);
					put_on_top_of_deck(player, card_added);
				}
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

int card_visara_the_dreadful(int player, int card, event_t event)
{
  /* Visara the Dreadful	|3|B|B|B
   * Legendary Creature - Gorgon 5/5
   * Flying
   * |T: Destroy target creature. It can't be regenerated. */

  check_legend_rule(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_voice_of_the_woods(int player, int card, event_t event){
	/* Voice of the Woods	|3|G|G
	 * Creature - Elf 2/2
	 * Tap five untapped Elves you control: Put a 7/7 |Sgreen Elemental creature token with trample onto the battlefield. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_ELF;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td1) > 4 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! tapsubtype_ability(player, card, 5, &td1) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 7;
		token.tou = 7;
		token.color_forced = COLOR_TEST_GREEN;
		token.key_plus = KEYWORD_TRAMPLE;
		generate_token(&token);
	}

	return 0;
}

int card_voidmage_prodigy(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = card_spiketail_hatchling(player, card, event);
		if( result > 0 && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_WIZARD, 0, 0, 0, 0, 0, -1, 0) &&
			has_mana_for_activated_ability(player, card, 0, 0, 2, 0, 0, 0)
		  ){
			return result;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 2, 0, 0, 0) ;
		if( spell_fizzled != 1 ){
			if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_WIZARD, 0, 0, 0, 0, 0, -1, 0) ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td, 0) ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}

	return morph(player, card, event, 0, 0, 1, 0, 0, 0);
}

int card_wall_of_mulch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0)  ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_WALL, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_WALL, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_weathered_wayfarer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) &&
			count_permanents_by_type(player, TYPE_LAND) < count_permanents_by_type(1-player, TYPE_LAND)
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_weird_harvest(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		APNAP(p, {
					int choice = 0;
					if( ! duh_mode(p) ){
						choice = do_dialog(p, player, card, player, card, " Search your library\n Pass", 0);
					}
					if( choice == 0 ){
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE, "Select a Creature card.");
						this_test.qty = instance->info_slot;
						new_global_tutor(p, p, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
						shuffle(p);
					}
				};
			);
			kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_wellwisher(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_ELF);
		gain_life(player, amount);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, MANACOST_X(0), 0, 0, 0);
}

int card_whipcorder(int player, int card, event_t event){
	return card_master_decoy(player, card, event) | morph(player, card, event,0, 0, 0, 0, 0, 1);
}

int card_windswept_heath(int player, int card, event_t event){
	/* Windswept Heath	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Forest or |H2Plains card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_FOREST, SUBTYPE_PLAINS, 1);
}

int card_wirewood_herald(int player, int card, event_t event ){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char msg[100] = "Select an Elf card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
		this_test.subtype = SUBTYPE_ELF;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_wirewood_lodge(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT );
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.required_subtype = SUBTYPE_ELF;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) && can_use_activated_abilities(player, card) &&
			can_target(&td1)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Untap an Elf\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ;
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELF) ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				else{
					 spell_fizzled = 1;
				}
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td1) ){
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
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

int card_wirewood_pride(int player, int card, event_t event){
	/* Wirewood Pride	|G
	 * Instant
	 * Target creature gets +X/+X until end of turn, where X is the number of Elves on the battlefield. */
	int amt = (event == EVENT_CHECK_PUMP || event == EVENT_RESOLVE_SPELL) ? count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_ELF) : 0;
	return vanilla_instant_pump(player, card, event, ANYBODY, player, amt, amt, 0, 0);
}

int card_wirewood_savage(int player, int card, event_t event){
	if( specific_cip(player, card, event, player, 1+player, TYPE_PERMANENT, 0, SUBTYPE_BEAST, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_wooded_foothills(int player, int card, event_t event){
	/* Wooded Foothills	""
	 * Land
	 * |T, Pay 1 life, Sacrifice ~: Search your library for |Ha Mountain or |H2Forest card and put it onto the battlefield. Then shuffle your library. */
	return fetchland(player, card, event, SUBTYPE_MOUNTAIN, SUBTYPE_FOREST, 1);
}

int card_words_of_war(int player, int card, event_t event){

	/* Words of War	|2|R
	 * Enchantment
	 * |1: The next time you would draw a card this turn, ~ deals 2 damage to target creature or player instead. */

	card_instance_t *instance = get_card_instance( player, card );
	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		CAN_ACTIVATE(player, card, MANACOST_X(1))// Worded as an activated ability that causes a delayed triggered ability
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1;
		}
		else if(event == EVENT_RESOLVE_TRIGGER && charge_mana_for_activated_ability_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, MANACOST_X(1))){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);
				td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					damage_creature_or_player(player, card, event, 2);
					instance->number_of_targets = 1;
					suppress_draw = 1;
				}
		}
	}
	return global_enchantment(player, card, event);
}

int card_words_of_waste(int player, int card, event_t event){

	/* Words of Waste	|2|B
	 * Enchantment
	 * |1: The next time you would draw a card this turn, each opponent discards a card instead. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		CAN_ACTIVATE(player, card, MANACOST_X(1))// Worded as an activated ability that causes a delayed triggered ability
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1;
		}
		else if(event == EVENT_RESOLVE_TRIGGER && charge_mana_for_activated_ability_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, MANACOST_X(1))){
				discard(1-player, 0, player);
				suppress_draw = 1;
		}
	}
	return global_enchantment(player, card, event);
}

int card_words_of_wilding(int player, int card, event_t event){

	/* Words of Wilding	|2|G
	 * Enchantment
	 * |1: The next time you would draw a card this turn, put a 2/2 |Sgreen Bear creature token onto the battlefield instead. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		CAN_ACTIVATE(player, card, MANACOST_X(1))// Worded as an activated ability that causes a delayed triggered ability
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1;
		}
		else if(event == EVENT_RESOLVE_TRIGGER && charge_mana_for_activated_ability_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, MANACOST_X(1))){
				generate_token_by_id(player, card, CARD_ID_BEAR);
				suppress_draw = 1;
		}
	}
	return global_enchantment(player, card, event);
}

int card_words_of_wind(int player, int card, event_t event){

	/* Words of Wind	|2|U
	 * Enchantment
	 * |1: The next time you would draw a card this turn, each player returns a permanent he or she controls to its owner's hand instead. */

	card_instance_t *instance = get_card_instance( player, card );
	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		CAN_ACTIVATE(player, card, MANACOST_X(1))// Worded as an activated ability that causes a delayed triggered ability
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_OPTIONAL;
		}
		else if(event == EVENT_RESOLVE_TRIGGER && charge_mana_for_activated_ability_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, MANACOST_X(1))){
			APNAP(p, {
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_PERMANENT);
				td.allowed_controller = p;
				td.preferred_controller = p;
				td.who_chooses = p;
				td.allow_cancel = 0;
				instance->number_of_targets = 0;
				if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			});
			instance->number_of_targets = 0;
			suppress_draw = 1;
		}
	}
	return global_enchantment(player, card, event);
}

int card_words_of_worship(int player, int card, event_t event){

	/* Words of Worship	|2|W
	 * Enchantment
	 * |1: The next time you would draw a card this turn, you gain 5 life instead. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		CAN_ACTIVATE(player, card, MANACOST_X(1))// Worded as an activated ability that causes a delayed triggered ability
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1;
		}
		else if(event == EVENT_RESOLVE_TRIGGER && charge_mana_for_activated_ability_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, MANACOST_X(1))){
				gain_life(player, 5);
				suppress_draw = 1;
		}
	}
	return global_enchantment(player, card, event);
}

