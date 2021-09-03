#include "manalink.h"

// Functions
void generic_wish_effect(int player, int card, int type, int ai_card){
	if( get_id(player, card) == CARD_ID_DEATH_WISH ){
		int amount = (life[player]+1)/2;
		lose_life(player, amount);
	}
	int iid = -1;
	if( player == HUMAN ){
		if( ai_is_speculating != 1 ){
			iid = choose_a_card("Choose a card", -1, -1);
			if (is_what(-1, iid, type) &&
				!is_what(-1, iid, TYPE_EFFECT) &&
				cards_ptr[cards_data[iid].id]->types[0] != SUBTYPE_CONSPIRACY
			   ){
				add_card_to_hand( player, iid );
			}
			else{
				iid = -1;
			}
		}
	}
	else{
		iid = get_internal_card_id_from_csv_id(ai_card);
		int card_added = add_card_to_hand(player, iid);
		reveal_card(player, card, player, card_added);
	}
	if (iid != -1){
		update_rules_engine(check_card_for_rules_engine(iid));
	}
}

int generic_wish(int player, int card, event_t event, int type, int ai_card){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL){
		generic_wish_effect(player, card, type, ai_card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

// Cards

int card_ancestors_chosen(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, count_graveyard(player));
	}
	return 0;
}

int card_anger(int player, int card, event_t event){
	/* Anger	|3|R
	 * Creature - Incarnation 2/2
	 * Haste
	 * As long as ~ is in your graveyard and you control |Ha Mountain, creatures you control have haste. */

	haste(player, card, event);
	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( affected_card_controller == player && in_play(affected_card_controller, affected_card) ){
			if( count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) > 0 || count_cards_by_id(HUMAN, CARD_ID_SPAT) > 0 ) {
				give_haste( affected_card_controller, affected_card );
			}
		}
	}
	return 0;
}

int card_anurid_brushhopper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 1 ){
		return generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			new_multidiscard(player, 2, 0, player);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && in_play(instance->parent_controller, instance->parent_card) ){
		remove_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card);
	}

	return 0;
}

int card_anurid_swarmsnapper(int player, int card, event_t event)
{
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  can_block_additional_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1);
	}

  return generic_activated_ability(player, card, event, GAA_NONE, MANACOST_XG(1,1), 0, NULL, NULL);
}

int card_arcane_teachings(int player, int card, event_t event){
	if(	(event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card) ){
		card_instance_t* instance = in_play(player, card);
		if (instance && affect_me(instance->damage_target_player, instance->damage_target_card))
			event_result += 2;
	}
	return card_hermetic_study(player, card, event);
}

int card_balthor_the_defiled(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	boost_creature_type(player, card, event, SUBTYPE_MINION, 1, 1, 0, BCT_INCLUDE_SELF);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK) | get_sleighted_color_test(player, card, COLOR_TEST_RED);
		APNAP(p, {new_reanimate_all(p, -1, p, &this_test, REANIMATE_DEFAULT);};);
	}

	return generic_activated_ability(player, card, event, GAA_RFG_ME, MANACOST_B(3), 0, NULL, NULL);
}

int card_battle_screech(int player, int card, event_t event){
	/* Battle Screech	|2|W|W
	 * Sorcery
	 * Put two 1/1 |Swhite Bird creature tokens with flying onto the battlefield.
	 * Flashback-Tap three untapped |Swhite creatures you control. */

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_ABILITY){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.illegal_state = TARGET_STATE_TAPPED;
		td.required_color = COLOR_TEST_WHITE;
		if( target_available(player, card, &td) > 2 ){
			return GA_PLAYABLE_FROM_GRAVE;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.illegal_state = TARGET_STATE_TAPPED;
		td.required_color = COLOR_TEST_WHITE;
		int tapped = 0;
		while( tapped < 3 ){
				if( tapped > 0 ){
					td.allow_cancel = 0;
				}
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					tap_card(instance->targets[0].player, instance->targets[0].card);
					tapped++;
				}
				else{
					if( tapped < 1 ){
						break;
					}
				}
		}
		if( tapped < 3 ){
			spell_fizzled = 1;
		}
		else{
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_BIRD, 2);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return basic_spell(player, card, event);
}

int card_battlefield_scrounger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && has_threshold(player) && instance->targets[4].player != 66 ){
		return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on bottom of your library.");
			int result = select_multiple_cards_from_graveyard(player, player, 0, AI_MIN_VALUE, &this_test, 3, &instance->targets[0]);
			if( result == 3 ){
				int i;
				for(i=0; i<3; i++){
					from_graveyard_to_deck(player, instance->targets[i].player, 2);
				}
				instance->targets[4].player = 66;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 3, 3);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[4].player = 0;
	}

	return 0;
}

int card_benevolent_bodyguard(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_book_burning(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int i;
			int dmg = 0;
			for(i=0; i<2; i++){
				int ai_choice = 0;
				if( life[i] - 6 < 6 ){
					ai_choice = 1;
				}
				int choice = do_dialog(i, player, card, -1, -1, " Take 6 damage\n No thanks", ai_choice);
				if( choice == 0 ){
					damage_player(i, 6, player, card);
					dmg = 1;
					break;
				}
			}
			if( dmg != 1 ){
				mill(instance->targets[0].player, 6);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

// brawn --> vanilla

int card_breaking_point(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		int dmg = 0;
		for(i=0; i<2; i++){
			int ai_choice = 0;
			if( life[i] - 6 < 6 ){
				ai_choice = 1;
			}
			int choice = do_dialog(i, player, card, -1, -1, " Take 6 damage\n No thanks", ai_choice);
			if( choice == 0 ){
				damage_player(i, 6, player, card);
				dmg = 1;
				break;
			}
		}
		if( dmg != 1 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, 2, &this_test, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_browbeat(int player, int card, event_t event){

	/* Browbeat	|2|R
	 * Sorcery
	 * Any player may have ~ deal 5 damage to him or her. If no one does, target player draws three cards. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			int AI_choice = 1;
			if(life[AI] > 9 && player != AI ){
				AI_choice = 0;
			}
			int took_damage = 0;
			APNAP(p,
				  if (!took_damage && do_dialog(p, player, card, -1, -1, " Take 5\n No Thanks", AI_choice) == 0){
					took_damage = 1;
					damage_player(p, 5, player, card);
				  });
			if( !took_damage ){
				draw_cards(instance->targets[0].player, 3);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_burning_wish(int player, int card, event_t event){
	return generic_wish(player, card, event, TYPE_SORCERY, CARD_ID_FIREBALL );
}

int card_cabal_therapy(int player, int card, event_t event){

	/* Cabal Therapy	|B
	 * Sorcery
	 * Name a nonland card. Target player reveals his or her hand and discards all cards with that name.
	 * Flashback-Sacrifice a creature. */

	if( ! IS_GS_EVENT(player, card, event) && event != EVENT_GRAVEYARD_ABILITY && event != EVENT_PAY_FLASHBACK_COSTS ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_GRAVEYARD_ABILITY){
		return can_sorcery_be_played(player, event) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && can_target(&td) ? GA_PLAYABLE_FROM_GRAVE : 0;
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return GAPAID_EXILE;
		}
	}

	if (event == EVENT_RESOLVE_SPELL){
		card_instance_t *instance = get_card_instance(player, card);
		if( valid_target(&td) && hand_count[instance->targets[0].player] > 0){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card.");
			this_test.type_flag = DOESNT_MATCH;
			int result = name_a_card(player, instance->targets[0].player, &this_test, 3);
			if( result > -1 ){
				test_definition_t this_test2;
				default_test_definition(&this_test2, TYPE_ANY);
				this_test2.id = result;

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, instance->targets[0].player, &this_definition);
				this_definition.effect = EC_DISCARD | EC_ALL_WHICH_MATCH_CRITERIA;
				new_effect_coercion(&this_definition, &this_test2);
			}
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

int card_cephalid_constable(int player, int card, event_t event){

	/* Cephalid Constable	|1|U|U
	 * Creature - Cephalid Wizard 1/1
	 * Whenever ~ deals combat damage to a player, return up to that many target permanents that player controls to their owners' hands. */

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_REPORT_DAMAGE_DEALT) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		int i;
		instance->number_of_targets = 0;
		for(i=0; i<instance->targets[16].player && can_target(&td) ; i++){
			if( new_pick_target(&td, "TARGET_PERMANENT", -1, 1) ){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
			} else {
				break;
			}
		}
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			bounce_permanent(instance->targets[i].player, instance->targets[i].card);
		}
		instance->targets[16].player = 0;
	}

	return 0;
}

int card_cephalid_inkshrouder(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, KEYWORD_SHROUD, SP_KEYWORD_UNBLOCKABLE);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_chastise(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = get_power(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature.", 1, NULL);
}

int card_commander_eesha(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	protection_from_creatures(player, card, event);

	return 0;
}

int card_crush_of_wurms(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.qty = 3;
		token.pow = 6;
		token.tou = 6;
		generate_token(&token);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XG(6, 3));
}

int card_cunning_wish(int player, int card, event_t event){
	return generic_wish(player, card, event, TYPE_INSTANT | TYPE_INTERRUPT, CARD_ID_ANCESTRAL_RECALL );
}

int card_death_wish(int player, int card, event_t event){
	return generic_wish(player, card, event, TYPE_ANY, CARD_ID_LILIANA_OF_THE_DARK_REALMS );
}

int card_dwarven_bloodboiler(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.illegal_abilities = 0;
	td1.required_subtype = SUBTYPE_DWARF;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( tapsubtype_ability(player, card, 1, &td1) ){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}

	return 0;
}

int card_dwarven_driller(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int ai_choice = 0;
			if( life[instance->targets[0].player] - 2 < 6 ){
				ai_choice = 1;
			}
			int choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Take 2 damage\n No thanks", ai_choice);
			if( choice == 0 ){
				damage_player(instance->targets[0].player, 2, player, card);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_LAND");
}

int card_elephant_guide(int player, int card, event_t event)
{
  /* Elephant Guide	|2|G
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +3/+3.
   * When enchanted creature dies, put a 3/3 |Sgreen Elephant creature token onto the battlefield. */

  if (attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_token_by_id(player, card, CARD_ID_ELEPHANT);

  return generic_aura(player, card, event, player, 3, 3, 0, 0, 0, 0, 0);
}

int card_envelop(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SORCERY);

	return counterspell(player, card, event, &td, 0);
}

int card_epic_struggle(int player, int card, event_t event){

	/* Epic Struggle	|2|G|G
	 * Enchantment
	 * At the beginning of your upkeep, if you control twenty or more creatures, you win the game. */

	if( (event == EVENT_SHOULD_AI_PLAY ||
		 (current_turn == player && upkeep_trigger(player, card, event))) &&
		count_subtype(player, TYPE_CREATURE, -1) >= 20
	  ){
		lose_the_game(1-player);
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int creatures = count_subtype(player, TYPE_CREATURE, -1) - 10;
		if (creatures > 10){
			if (player == AI){
				ai_modifier += creatures * creatures * creatures;
			} else if (creatures >= 5){
				ai_modifier -= creatures * creatures * creatures;
			}
		}
	}

	return global_enchantment(player, card, event);
}

// filth --> vanilla

// elemental card --> rhino token

int card_firecat_blitz(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		if( (played_for_free(player, card) || is_token(player, card)) && ! get_flashback(player, card) ){
			instance->info_slot = 0;
			return 0;
		}
		if( get_flashback(player, card) ){
			int sn = 0;
			if( can_sacrifice_type_as_cost(player, 1, TYPE_LAND) ){
				int mountain_number = count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN);
				int sacced[mountain_number];
				test_definition_t test;
				new_default_test_definition(&test, TYPE_LAND, "Select a Mountain to sacrifice.");
				test.subtype = SUBTYPE_MOUNTAIN;
				while( sn < mountain_number ){
						int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
						if (!sac){
							break;
						}
						state_untargettable(BYTE2(sac), BYTE3(sac), 1);
						sacced[sn] = BYTE3(sac);
						sn++;
				}
				int i;
				for(i=(sn-1); i>-1; i--){
					kill_card(player, sacced[i], KILL_SACRIFICE);
				}
			}
			instance->info_slot = sn;
		}
		else{
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL_CAT, &token);
		token.qty = instance->info_slot;
		token.pow = 1;
		token.tou = 1;
		token.legacy = 1;
		token.special_code_for_legacy = &haste_and_remove_eot;
		generate_token(&token);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, MANACOST_R(2));
}

int card_flaring_pain(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		damage_cannot_be_prevented_until_eot(player, card);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, MANACOST_R(1));
}

int card_flash_of_insight(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		if( (played_for_free(player, card) || is_token(player, card)) && ! get_flashback(player, card) ){
			instance->info_slot = 0;
			return 0;
		}
		if( get_flashback(player, card) ){
			instance->info_slot = 0;
			char msg[100] = "Select a blue card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			this_test.color = COLOR_TEST_BLUE;
			while( new_special_count_grave(player, &this_test) > 0 ){
					if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						instance->info_slot++;
					}
					else{
						break;
					}
			}
		}
		else{
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1  ){
				instance->info_slot = x_value;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			impulse_effect(player, instance->info_slot, 0);
		}
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return do_flashback(player, card, event, MANACOST_XU(1, 1));
}

int card_fledgling_dragon(int player, int card, event_t event){
  // Threshold - As long as seven or more cards are in your graveyard, ~ gets +3/+3 and has "|R: ~ gets +1/+0 until end of turn."
	if( has_threshold(player) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += 3;
		}
		return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
	}
	return 0;
}

int card_forcemage_advocate(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, NULL) ){
			if( count_graveyard(1-player) > 0 && ! graveyard_has_shroud(1-player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card.");
			if( select_target_from_grave_source(player, card, 1 - player, 0, AI_MIN_VALUE, &this_test, 1) == -1){
				spell_fizzled = 1;
			}
			else{
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}

		int selected = validate_target_from_grave(player, card, 1 - player, 1);
		if (selected != -1){
			from_grave_to_hand(1 - player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_genesis(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		if( has_mana_multi(player, MANACOST_XG(2, 1)) && ! graveyard_has_shroud(2) ){
			int choice = do_dialog(player, player, card, -1, -1," Return a creature\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, MANACOST_XG(2, 1));
				if( spell_fizzled != 1 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
					if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) == -1 ){
						spell_fizzled = 1;
					}
				}
			}
		}
		return -2;
	}
	return 0;
}

int card_glory(int player, int card, event_t event){
	/* Glory	|3|W|W
	 * Creature - Incarnation 3/3
	 * Flying
	 * |2|W: Choose a color. Creatures you control gain protection from the chosen color until end of turn. Activate this ability only if ~ is in your graveyard. */

	if(event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XW(2, 1))){
		return GA_BASIC;
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XW(2, 1));
		if( spell_fizzled != 1 ){
			return GAPAID_REMAIN_IN_GRAVE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
	}
	return 0;
}

int card_golden_wish(int player, int card, event_t event){
	/* Golden Wish	|3|W|W
	 * Sorcery
	 * You may choose an artifact or enchantment card you own from outside the game, reveal that card, and put it into your hand. Exile ~. */

	return generic_wish(player, card, event, TYPE_ARTIFACT | TYPE_ENCHANTMENT, CARD_ID_MOAT );
}

int card_grip_of_amnesia(int player, int card, event_t event){//UNUSEDCARD

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int choice = 0;
		if( count_graveyard(instance->targets[0].player) > 0 ){
			choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Exile your graveyard\n Pass", 0);
		}
		if( choice == 0 ){
			rfg_whole_graveyard(instance->targets[0].player);
		}
		if( choice == 1 ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grizzly_fate(int player, int card, event_t event){
	/* Grizzly Fate	|3|G|G
	 * Sorcery
	 * Put two 2/2 |Sgreen Bear creature tokens onto the battlefield.
	 * Threshold - Put four 2/2 |Sgreen Bear creature tokens onto the battlefield instead if seven or more cards are in your graveyard.
	 * Flashback |5|G|G */

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 2;
		if( has_threshold(player) ){
			amount = 4;
		}
		generate_tokens_by_id(player, card, CARD_ID_BEAR, amount);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, MANACOST_XG(5, 2));
}

int card_guiltfeeder(int player, int card, event_t event){

	fear(player, card, event);

	if( ! is_humiliated(player, card) && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS && is_unblocked(player, card) ){
		lose_life(1-player, count_graveyard(1-player));
	}

	return 0;
}

int card_hapless_researcher(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_hunting_grounds(int player, int card, event_t event){
	if( in_play(player, card) && has_threshold(player) && ! is_humiliated(player, card) &&
		specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player), TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return global_enchantment(player, card, event);
}

int card_jeska_warrior_adept(int player, int card, event_t event)
{
  /* Jeska, Warrior Adept	|2|R|R
   * Legendary Creature - Human Barbarian Warrior 3/1
   * First strike, haste
   * |T: ~ deals 1 damage to target creature or player. */

  check_legend_rule(player, card, event);

  haste(player, card, event);

  return card_prodigal_sorcerer(player, card, event);
}

int card_keep_watch(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		draw_cards(player, check_battlefield_for_special_card(player, card, current_turn, CBFSC_GET_COUNT, &this_test));
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_krosan_reclamation(int player, int card, event_t event){
	/* Krosan Reclamation	|1|G
	 * Instant
	 * Target player shuffles up to two target cards from his or her graveyard into his or her library.
	 * Flashback |1|G */

	if( ! IS_GS_EVENT(player, card, event) && event != EVENT_GRAVEYARD_ABILITY && event != EVENT_PAY_FLASHBACK_COSTS ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (pick_target(&td, "TARGET_PLAYER")){
			select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MAX_VALUE : AI_MIN_VALUE, NULL, 2, &instance->targets[1]);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i, any_shuffled_in = 0;
		for (i = 1; i <= 2; ++i){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
			if (selected != -1){
				from_graveyard_to_deck(instance->targets[0].player, selected, 2);
				any_shuffled_in = 1;
			}
		}
		if (any_shuffled_in){
			shuffle(instance->targets[0].player);
		}
		kill_card(player, card, get_flashback(player, card) ? KILL_REMOVE : KILL_DESTROY);
	}

	return do_flashback(player, card, event, MANACOST_XG(1, 1));
}

int card_krosan_verge(int player, int card, event_t event)
{
  // ~ enters the battlefield tapped.
  comes_into_play_tapped(player, card, event);

  // |T: Add |1 to your mana pool.
  // |2, |T, Sacrifice ~: Search your library for |Ha Forest card and |Ha Plains card and put them onto the battlefield tapped. Then shuffle your library.
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (paying_mana())
		return mana_producer(player, card, event);

	  int can_common = !is_tapped(player, card) && !is_animated_and_sick(player, card);
	  int can_mana = can_common && can_produce_mana(player, card);
	  int can_tutor = (can_common && can_sacrifice_this_as_cost(player, card)
					   && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(3)));

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_TUTOR
	  } choice = DIALOG(player, card, event,
						"Produce mana", can_mana, -1,	// AI only taps for mana if already paying mana for something
						"Search for lands", can_tutor, 1);

	  if (event == EVENT_CAN_ACTIVATE || choice == 0)
		return choice;
	  else if (choice == CHOICE_MANA)
		return mana_producer(player, card, event);
	  else if (event == EVENT_ACTIVATE)	// and CHOICE_TUTOR
		{
		  add_state(player, card, STATE_TAPPED);
		  if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
			kill_card(player, card, KILL_SACRIFICE);
		  else
			remove_state(player, card, STATE_TAPPED);
		}
	  else	// EVENT_RESOLVE_ACTIVATION, CHOICE_TUTOR
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		  test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		  test.no_shuffle = 1;
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &test);
		  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_PLAINS));
		  test.subtype = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &test);
		}

	  return 0;
	}
  else
	return mana_producer(player, card, event);
}

int card_lava_dart(int player, int card, event_t event){
	/* Lava Dart	|R
	 * Instant
	 * ~ deals 1 damage to target creature or player.
	 * Flashback-Sacrifice |Ha Mountain. */

	if( ! IS_GS_EVENT(player, card, event) && event != EVENT_GRAVEYARD_ABILITY && event != EVENT_PAY_FLASHBACK_COSTS ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_GRAVEYARD_ABILITY && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0) ){
		return can_target(&td) ? GA_PLAYABLE_FROM_GRAVE : 0;
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a Mountain to sacrifice.");
		test.subtype = SUBTYPE_MOUNTAIN;	// can't Hack
		test.subtype_flag = MATCH;
		if (new_sacrifice(player, card, player, 0, &test)){
			return GAPAID_EXILE;
		} else {
			cancel = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			damage_target0(player, card, 1);
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

int card_living_wish(int player, int card, event_t event){
	return generic_wish(player, card, event, TYPE_CREATURE | TYPE_LAND, CARD_ID_GAEAS_CRADLE);
}

int card_mental_note(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		mill( player,  2 );
		draw_a_card( player );
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_miraris_wake(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 9) ){ return 0; }

	if( in_play(player, card) && ! is_humiliated(player, card) && affected_card_controller == player ){
		boost_creature_type(player, card, event, -1, 1, 1, 0, BCT_CONTROLLER_ONLY);
		return card_mana_flare(player, card, event);
	}

	return global_enchantment(player, card, event);
}

int card_mist_of_stagnation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_abilities = 0;
		td.preferred_controller = current_turn;
		td.who_chooses = current_turn;
		td.allow_cancel = 0;

		int tc = 0;
		int max = count_graveyard(current_turn);
		instance->number_of_targets = 0;
		while( max && can_target(&td) ){
				if( new_pick_target(&td, "Select a permanent to untap.", tc, GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[tc].player, instance->targets[tc].card, 1);
					tc++;
				}
				max--;
		}
		int i;
		for(i=0; i<tc; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			untap_card(instance->targets[i].player, instance->targets[i].card);
		}
		instance->number_of_targets = 0;
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( is_tapped(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_PERMANENT) ){
			get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
		}
	}

	return global_enchantment(player, card, event);
}

int card_morality_shift(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		const int *grave = get_grave(player);
		int cg = count_graveyard(player)-1;
		int count = 0;
		while( deck[0] != -1 ){
				mill(player, 1);
		}
		while( cg > -1 ){
				deck[count] = grave[cg];
				remove_card_from_grave(player, cg);
				cg--;
				count++;
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_nantuko_monastery(int player, int card, event_t event){
	return manland_normal(player, card, event, MANACOST_GW(1, 1));
}

int card_nantuko_monastery_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, MANACOST_GW(1, 1));
}

int card_nomad_mythmaker(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	/* Nomad Mythmaker	|2|W
	 * Creature - Human Nomad Cleric 2/2
	 * |W, |T: Put target Aura card from a graveyard onto the battlefield under your control attached to a creature you control. */

	target_definition_t td;
	base_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return (generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_W(1), 0, NULL, NULL)
				&& any_in_graveyard_by_subtype(ANYBODY, SUBTYPE_AURA_CREATURE)
				&& !graveyard_has_shroud(2));
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_W(1)) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select target Aura card with enchant creature.");
			this_test.subtype = SUBTYPE_AURA_CREATURE;

			if (select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1) != -1){
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected == -1 ){
			return 0;
		}

		int card_added = reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_RETURN_TO_HAND);
		if( card_added == -1 ){
			return 0;
		}

		target_definition_t td1;
		base_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allowed_controller = player;
		td1.preferred_controller = player;
		td1.allow_cancel = 0;

		if (can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
			put_into_play_aura_attached_to_target(player, card_added, instance->targets[0].player, instance->targets[0].card);
		}
		else {
			kill_card(player, card_added, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_phantom_centaur(int player, int card, event_t event){
	/* Phantom Centaur	|2|G|G
	 * Creature - Centaur Spirit 2/0
	 * Protection from |Sblack
	 * ~ enters the battlefield with three +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */
	/* Phantom Flock	|3|W|W
	 * Creature - Bird Soldier Spirit 0/0
	 * Flying
	 * ~ enters the battlefield with three +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);
	phantom_effect(player, card, event, 0);
	return 0;
}

int card_phantom_nishoba(int player, int card, event_t event){
	/* Phantom Nishoba	|5|G|W
	 * Creature - Cat Beast Spirit 0/0
	 * Trample
	 * ~ enters the battlefield with seven +1/+1 counters on it.
	 * Whenever ~ deals damage, you gain that much life.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 7);

	spirit_link_effect(player, card, event, player);

	phantom_effect(player, card, event, 0);

	return 0;
}

int card_nullmage_advocate(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			if( count_graveyard(1-player) >= 2 && ! graveyard_has_shroud(1 - player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( pick_target(&td, "DISENCHANT") &&
				select_multiple_cards_from_graveyard(player, 1 - player, -1, AI_MIN_VALUE, NULL, 2, &instance->targets[1]) == 2
			  ){
				tap_card(player, card);
			}
		}
		else {
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}

		int i;
		for (i = 1; i <= 2; ++i){
			int selected = validate_target_from_grave(player, card, 1 - player, i);
			if (selected != -1){
				from_grave_to_hand(1 - player, selected, TUTOR_HAND);
			}
		}
	}

	return 0;
}

int card_phantom_nantuko(int player, int card, event_t event){
	/* Phantom Nantuko	|2|G
	 * Creature - Insect Spirit 0/0
	 * Trample
	 * ~ enters the battlefield with two +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~.
	 * |T: Put a +1/+1 counter on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	phantom_effect(player, card, event, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

	if( player == AI && current_turn != player && eot_trigger(player, card, event) &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) &&
		charge_mana_for_activated_ability(player, card, MANACOST0)
	  ){
		tap_card(player, card);
		add_1_1_counter(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_phantom_nomad(int player, int card, event_t event){
	/* Phantom Nomad	|1|W
	 * Creature - Spirit Nomad 0/0
	 * ~ enters the battlefield with two +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */
	/* Phantom Tiger	|2|G
	 * Creature - Cat Spirit 1/0
	 * ~ enters the battlefield with two +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);
	phantom_effect(player, card, event, 0);
	return 0;
}

// phantom tiger --> phantom nomad

int prismatic_strands_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_PREVENT_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage_card == damage->internal_card_id && damage->info_slot > 0 && (damage->initial_color & (1<<instance->targets[1].card)) )
			damage->info_slot = 0;
	}

	if(event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_prismatic_strands(int player, int card, event_t event){

	/* Prismatic Strands	|2|W
	 * Instant
	 * Prevent all damage that sources of the color of your choice would deal this turn.
	 * Flashback-Tap an untapped |Swhite creature you control. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &prismatic_strands_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].card = choose_a_color(player, get_deck_color(player, 1-player));
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	if (!IS_CASTING_FROM_GRAVE(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_color = COLOR_TEST_WHITE;

	if( event == EVENT_GRAVEYARD_ABILITY ){
		return can_target(&td) ? GA_PLAYABLE_FROM_GRAVE : 0;
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( new_pick_target(&td, "Select an untapped white creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			return GAPAID_EXILE;
		}
	}
	return 0;
}

int card_pulsemage_advocate(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( count_graveyard(1-player) >= 3 && !graveyard_has_shroud(1-player) ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && !graveyard_has_shroud(1-player) ){
					return 1;
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
				if( select_multiple_cards_from_graveyard(player, 1 - player, -1, AI_MIN_VALUE, NULL, 3, &instance->targets[1]) == 3){
					tap_card(player, card);
				}
				else {
					spell_fizzled = 1;
				}
			}
			else {
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, instance->parent_card, player, selected, REANIMATE_DEFAULT);
		}

		int i;
		for (i = 1; i <= 3; ++i){
			selected = validate_target_from_grave(player, card, 1 - player, i);
			if (selected != -1){
				from_grave_to_hand(1 - player, selected, TUTOR_HAND);
			}
		}
	}

	return 0;
}

static int has_flashback_by_internal_id(int iid, int unused1, int unused2, int unsed3){
	// Approximation, until we find a better method
	return is_what(-1, iid, TYPE_SPELL) && !is_what(-1, iid, TYPE_PERMANENT) && cards_data[iid].cc[2] == 9 ? 1 : 0;
}

int card_quiet_speculation(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card with Flashback.");
			this_test.qty = 3;
			this_test.special_selection_function = &has_flashback_by_internal_id;
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_MAX_VALUE, &this_test);
			shuffle(instance->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_ray_of_revelation(int player, int card, event_t event){// not enabled
	/* Ray of Revelation	|1|W
	 * Instant
	 * Destroy target enchantment.
	 * Flashback |G */

	if( ! IS_GS_EVENT(player, card, event) && event != EVENT_GRAVEYARD_ABILITY && event != EVENT_PAY_FLASHBACK_COSTS ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
	}

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

	return do_flashback(player, card, event, MANACOST_G(1));
}

int card_scalpelexis(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		int stop = 0;
		int count = 0;
		int *deck = deck_ptr[1-player];
		while( stop != 1){
				int x;
				int max = 4;
				while( count+max > count_deck(1-player) ){
						max--;
				}
				stop = 1;
				if( max > 0 ){
					for(x = count; x < count+max; x++){
						if( deck[x] == -1 ){
							break;
						}
						else{
							int y;
							for(y = x+1; y < count+max; y++){
								if( deck[x] == deck[y] ){
									stop = 0;
									break;
								}
							}
						}
					}
					count = x;
				}
		}
		if( count > 0 ){
			show_deck(HUMAN, deck, count, "Scalpelexis will exile these cards.", 0, 0x7375B0 );
			int i;
			for(i=0;i<count;i++){
				rfg_top_card_of_deck(1-player);
			}
		}
	}
	return 0;
}

int card_silver_seraph(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && has_threshold(player) ){
		boost_creature_type(player, card, event, -1, 2, 2, 0, BCT_CONTROLLER_ONLY);
	}
	return 0;
}

int card_solitary_confinement(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		give_shroud_to_player(player, card, event);

		skip_your_draw_step(player, event);

		// prevent any damage that would be dealt to me
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card && ! check_state(player, card, STATE_CANNOT_TARGET) ){
				if( damage->damage_target_player == player && damage->damage_target_card == -1 && damage->info_slot > 0 ){
					damage->info_slot = 0;
				}
			}
		}

		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int kill = 1;
			if( hand_count[player] > 0 ){
				char msg[100] = "Select a card to discard.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected > -1 ){
					discard_card(player, selected);
					kill = 0;
				}
			}
			if( kill == 1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_soulcatchers_aerie(int player, int card, event_t event){

	/* Soulcatchers' Aerie	|1|W
	 * Enchantment
	 * Whenever a Bird is put into your graveyard from the battlefield, put a feather counter on ~.
	 * Bird creatures get +1/+1 for each feather counter on ~. */

	if( ! is_humiliated(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) ){
		int count = count_counters(player, card, COUNTER_FEATHER);
		boost_creature_type(player, card, event, SUBTYPE_BIRD, count, count, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_BIRD;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_counters(player, card, COUNTER_FEATHER, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_spirit_cairn(int player, int card, event_t event)
{
	/* Spirit Cairn	|2|W
	 * Enchantment
	 * Whenever a player discards a card, you may pay |W. If you do, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if (discard_trigger(player, card, event, 2, RESOLVE_TRIGGER_AI(player), 0)
	  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_WHITE, 1))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SPIRIT, &token);
	  token.key_plus = KEYWORD_FLYING;
	  token.color_forced = COLOR_TEST_WHITE;
	  generate_token(&token);
	}

  return global_enchantment(player, card, event);
}

int card_spurnmage_advocate(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			if( count_graveyard(1-player) >= 2 && ! graveyard_has_shroud(1 - player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( new_pick_target(&td, "Select target attacking creature.", 0, 1 | GS_LITERAL_PROMPT) &&
				select_multiple_cards_from_graveyard(player, 1 - player, -1, AI_MIN_VALUE, NULL, 2, &instance->targets[1]) == 2
			  ){
				tap_card(player, card);
			}
		}
		else {
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}

		int i;
		for (i = 1; i <= 2; ++i){
			int selected = validate_target_from_grave(player, card, 1 - player, i);
			if (selected != -1){
				from_grave_to_hand(1 - player, selected, TUTOR_HAND);
			}
		}
	}

	return 0;
}

int card_stitch_together(int player, int card, event_t event){
	/* Stitch Together	|B|B
	 * Sorcery
	 * Return target creature card from your graveyard to your hand.
	 * Threshold - Return that card from your graveyard to the battlefield instead if seven or more cards are in your graveyard. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");
	this_test.ai_selection_mode = has_threshold(player) ? AI_MAX_CMC : AI_MAX_VALUE;

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			if( has_threshold(player) ){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
			else{
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_sutured_ghoul(int player, int card, event_t event){
	/* Sutured Ghoul	|4|B|B|B
	 * Creature - Zombie 100/100
	 * Trample
	 * As ~ enters the battlefield, exile any number of creature cards from your graveyard.
	 * ~'s power is equal to the total power of the exiled cards and its toughness is equal to their total toughness. */

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		add_state(player, card, STATE_OUBLIETTED); //Or it will be killed before ever entering in play.
		instance->targets[0].card = instance->targets[1].card = 0;
		const int *grave = get_grave(player);
		int tc = count_graveyard_by_type(player, TYPE_CREATURE);
		while( tc ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");
				if( player == AI ){
					this_test.id = CARD_ID_ANGER;
					this_test.id_flag = DOESNT_MATCH;
				}
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
				if( selected != -1 ){
					instance->targets[0].card += get_base_power_iid(player, grave[selected]);
					instance->targets[1].card += get_base_toughness_iid(player, grave[selected]);
					rfg_card_from_grave(player, selected);
					tc--;
				}
				else{
					break;
				}
		}
		remove_state(player, card, STATE_OUBLIETTED);
	}

	if( player >= 0 && card >= 0 && in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( event == EVENT_POWER && affect_me(player,card) && instance->targets[0].card > 0 ){
			event_result += instance->targets[0].card;
		}

		if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[1].card > 0 ){
			event_result += instance->targets[1].card;
		}
	}

	return 0;
}

int card_sylvan_safekeeper(int player, int card, event_t event){
	/* Sylvan Safekeeper	|G
	 * Creature - Human Wizard 1/1
	 * Sacrifice a land: Target creature you control gains shroud until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_LAND, "Select a land to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 1);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_SHROUD, 0);
		}
	}

	return 0;
}

int card_test_of_endurance(int player, int card, event_t event)
{
  /* Test of Endurance	|2|W|W
   * Enchantment
   * At the beginning of your upkeep, if you have 50 or more life, you win the game. */

  if (current_turn == player && life[player] >= 50 && upkeep_trigger(player, card, event))
	lose_the_game(1-player);

  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  int l = life[player] - 30;
	  if (l > 0)
		ai_modifier += (player == AI ? 1 : -1) * life[player] * life[player] * life[player];
	}

  return global_enchantment(player, card, event);
}

int card_thriss_nantuko_primus(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 5, 5);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_CREATURE");
}

// valor --> vanilla

int card_vigilant_sentry(int player, int card, event_t event){

	if( has_threshold(player) ){

		modify_pt_and_abilities(player, card, event, 1, 1, 0);

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.required_state = TARGET_STATE_IN_COMBAT;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 3, 3);
			}
		}

		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
	}
	return 0;
}

