#include "manalink.h"

// ---- CARDS ----

int card_angels_trumpet(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		// All creatures have vigilance.
		if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			vigilance(affected_card_controller, affected_card, event);
		}

		/* At the beginning of each player's end step, tap all untapped creatures that player controls that didn't attack this turn. ~ deals damage to the player
		 * equal to the number of creatures tapped this way. */
		if( eot_trigger(player, card, event) ){
			card_instance_t* inst;
			int count, dmg = 0;
			for (count = 0; count < active_cards_count[current_turn]; ++count){
				if ((inst = in_play(current_turn, count)) && is_what(current_turn, count, TYPE_CREATURE) && !(inst->state & (STATE_ATTACKED | STATE_TAPPED))){
					tap_card(current_turn, count);
					dmg++;
				}
			}
			if( dmg > 0 ){
				damage_player(current_turn, dmg, player, card);
			}
		}
	}
	return 0;
}

int card_anthroplasm(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = instance->info_slot;
		if( amount < count_1_1_counters(player, instance->parent_card) ){
			int difference = count_1_1_counters(player, instance->parent_card) - amount;
			remove_1_1_counters(player, instance->parent_card, difference);
		}
		if( amount > count_1_1_counters(player, instance->parent_card) ){
			int difference = amount - count_1_1_counters(player, instance->parent_card);
			add_1_1_counters(player, instance->parent_card, difference);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(-1), 0, NULL, NULL);
}

int card_archivist(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK, MANACOST0, 0, NULL, NULL);
}

int card_aura_flux(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
				if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_ENCHANTMENT) &&
					!(player == current_turn && count == card)
				  ){
					int choice = 1;
					if( has_mana(current_turn, COLOR_COLORLESS, 2) ){
						choice = do_dialog(current_turn, current_turn, count, -1, -1, " Pay upkeep\n Pass", 0);
						if( choice == 0 ){
							charge_mana(current_turn, COLOR_COLORLESS, 2);
							if( spell_fizzled == 1 ){
								choice = 1;
							}
						}
					}
					if( choice == 1 ){
						kill_card(current_turn, count, KILL_SACRIFICE);
					}
				}
				count--;
		}
	}

	return global_enchantment(player, card, event);
}

int card_avalanche_riders(int player, int card, event_t event){
	haste( player, card, event );
	echo(player, card, event, 3, 0, 0, 0, 1, 0);
	return card_ogre_arsonist(player, card, event);
}

int card_beast_of_burden(int player, int card, event_t event){
	/*
	  Beast of Burden |6
	  Artifact Creature - Golem x/x
	  Beast of Burden's power and toughness are each equal to the number of creatures on the battlefield.
	*/
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += count_subtype(ANYBODY, TYPE_CREATURE, -1);
	}

	return 0;
}

int card_blessed_reversal(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int amount = 0;
		int count = 0;
		while( count < active_cards_count[1-player] ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					if( is_attacking(1-player, count) && ! check_special_flags(1-player, count, SF_ATTACKING_PWALKER) ){
						amount++;
					}
				}
				count++;
		}
		gain_life(player, amount*3);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_bone_shredder(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XB(2, 1));

	return card_nekrataal(player, card, event);
}

int card_brink_of_madness(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && hand_count[player] < 1 && upkeep_trigger(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			new_discard_all(1-player, player);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_cloud_of_faeries(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		untap_lands(player, card, 2);
	}
	return cycling(player, card, event, MANACOST_X(2));
}

int card_crop_rotation(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST  ){
		if( basic_spell(player, card, event) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_crawlspace(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		no_more_than_x_creatures_can_attack(player, card, event, 1-player, 1);
	}
	return 0;
}

int card_defense_grid(int player, int card, event_t event){
	/*
	  Defense Grid |2
	  Artifact
	  Each spell costs {3} more to cast except during its controller's turn.
	*/
	if( ! is_humiliated(player, card) && event == EVENT_MODIFY_COST_GLOBAL ){
		if( ! is_what(affected_card_controller, affected_card, TYPE_LAND) && current_turn != affected_card_controller ){
			COST_COLORLESS+=3;
		}
	}

	return 0;
}

int card_defense_of_the_heart(int player, int card, event_t event){

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(current_turn);
		if(event == EVENT_TRIGGER && count > 0 && count_subtype(1-player, TYPE_CREATURE, -1) > 2 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
				this_test.qty = 2;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				shuffle(player);
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_defender_of_chaos(int player, int card, event_t event){
	// also code for Defender of the Law
	return flash(player, card, event);
}

int card_delusion_of_mediocrity(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 10);
	}

	if( leaves_play(player, card, event) ){
		lose_life(player, 10);
	}

	return global_enchantment(player, card, event);
}

int card_deranged_hermit(int player, int card, event_t event){
	/* Deranged Hermit	|3|G|G
	 * Creature - Elf 1/1
	 * Echo |3|G|G
	 * When ~ enters the battlefield, put four 1/1 |Sgreen Squirrel creature tokens onto the battlefield.
	 * Squirrel creatures get +1/+1. */

	echo(player, card, event, MANACOST_XG(3, 2));

	boost_creature_type(player, card, event, SUBTYPE_SQUIRREL, 1, 1, 0, BCT_INCLUDE_SELF);

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SQUIRREL, 4);
	}

	return 0;
}

static const char* target_is_attached_to_creature(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	card_instance_t* instance = get_card_instance(player, card);
	if (instance->damage_target_card >= 0 && is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE))
		return NULL;
	else
		return "attached to a creature";
}
int card_devout_harpist(int player, int card, event_t event)
{
	/* Devout Harpist	|W
	 * Creature - Human 1/1
	 * |T: Destroy target Aura attached to a creature. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
  td.required_subtype = SUBTYPE_AURA;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_is_attached_to_creature;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST0, 0, &td, "Select target Aura attached to a creature.");
}

int card_engineered_plague(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		td.allowed_controller = 1-player;

		if (!can_target(&td)){
			ai_modifier -= 128;
		}
	}

	if (comes_into_play(player, card, event)){
		instance->info_slot = select_a_subtype(player, card) + 1;
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS)
		&& instance->info_slot > 0
		&& in_play(player, card)
		&& ! is_humiliated(player, card)
		&& has_subtype(affected_card_controller, affected_card, instance->info_slot - 1)){
		event_result--;
	}

	return global_enchantment(player, card, event);
}

int card_erase(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_eviscerator(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		lose_life(player, 5);
	}

	return 0;
}

int card_expendable_troops(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_LITERAL_PROMPT | GAA_CAN_TARGET |GAA_SACRIFICE_ME, MANACOST0, 0,
									&td, "Select target attacking or blocking creature.");
}

int card_faerie_conclave(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XU(1, 1));
}

int card_faerie_conclave_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, MANACOST_XU(1, 1));
}

int card_fleeting_image(int player, int card, event_t event){

   card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 1), 0, NULL, NULL);
}

int card_forbidding_watchtower(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XW(1, 1));
}

int card_forbidding_watchtower_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, MANACOST_XW(1, 1));
}

int card_frantic_search(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		multidiscard(player, 2, 0);
		untap_lands(player, card, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);;
}

int card_ghitu_encampment(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XR(1, 1));
}

int card_ghitu_encampment_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, MANACOST_XR(1, 1));
}

int card_ghitu_slinger(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(2, 1));

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.preferred_controller = 1-player;
		td.allow_cancel = 0;
		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			   damage_creature_or_player(player, card, event, 2);
			}
		}
	}

	return 0;
}

int card_ghitu_war_cry(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE");
}

int card_goblin_welder(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 5) ){ return 0; }

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td0;
	default_target_definition(player, card, &td0, TYPE_ARTIFACT);
	td0.allowed_controller = player;
	td0.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && target_available(player, card, &td0) && ! graveyard_has_shroud(player) ){
				return 1;
			}
			if( count_graveyard_by_type(1-player, TYPE_ARTIFACT) > 0  && target_available(player, card, &td1) && ! graveyard_has_shroud(1-player) ){
				return 1;
			}
		}
	}
	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		char msg[100];
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			target_definition_t td3;
			default_target_definition(player, card, &td3, TYPE_ARTIFACT);
			if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && target_available(player, card, &td0) && ! graveyard_has_shroud(player) ){
				td3 = td0;
				strcpy(msg, "Select target artifact you control");
				if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && target_available(player, card, &td1) && ! graveyard_has_shroud(player) ){
					td3.allowed_controller = ANYBODY;
					strcpy(msg, "Select target artifact.");
				}
			}
			else{
				strcpy(msg, "Select target artifact your opponent controls");
				td3 = td1;
			}
			if( new_pick_target(&td3, msg, 0, 1 | GS_LITERAL_PROMPT) ){
				int mode = AI_MAX_CMC;
				if( instance->targets[0].player != player ){
					mode = AI_MIN_CMC;
				}
				if( select_target_from_grave_source(player, card, instance->targets[0].player, 0, mode, &this_test, 1) != -1 ){
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td2) ){
			int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_ARTIFACT, 0) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
				}
				reanimate_permanent(instance->targets[0].player, -1, instance->targets[0].player, selected, REANIMATE_DEFAULT);
			}
		}
	}

	return 0;
}

int card_grim_monolith (int player, int card, event_t event){
	return monolith(player, card, event, 4);
}

int card_harmonic_convergence(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 2, &this_test, ACT_PUT_ON_TOP);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);;
}

int card_hidden_gibbons(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_INSTANT | TYPE_INTERRUPT, NULL, SUBTYPE_APE);
}

int card_hope_and_glory(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 1, 1);
				untap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_impending_disaster(int player, int card, event_t event){

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(current_turn);
		if(event == EVENT_TRIGGER && count > 0 && count_subtype(2, TYPE_LAND, -1) > 6 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_LAND);
				APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_iron_maiden(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, 1-player, hand_count[1-player]-4 > 0 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int dmg = hand_count[1-player]-4;
		damage_player(1-player, dmg, player, card);
	}

	return 0;
}

static const char* dead_artifact_creature(int who_chooses, int player, int card){
	if( is_what(player, card, TYPE_CREATURE) && is_what(player, card, TYPE_ARTIFACT) ){
		if( get_card_instance(player, card)->kill_code == KILL_DESTROY ){
			if( who_chooses == HUMAN ){
				return NULL;
			}
			if( who_chooses == AI ){
				if( can_be_regenerated(player, card) ){
					return NULL;
				}
			}
		}
	}

	return "must be an artifact creature to regenerate.";
}

int card_jhoira_toolbox(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)dead_artifact_creature;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_REGENERATION | GAA_LITERAL_PROMPT, MANACOST_X(2), 0,
									&td, "Select target artifact creature to regenerate.");
}

int card_karmic_guide(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XW(3, 2));

	if( comes_into_play(player, card, event) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
	}

	return 0;
}

int card_knighthood(int player, int card, event_t event){
	boost_creature_type(player, card, event, -1, 0, 0, KEYWORD_FIRST_STRIKE, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return global_enchantment(player, card, event);
}

// lone wolf --> thorn elemental

int card_lurking_skirge(int player, int card, event_t event){
	double_faced_card(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, 0);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_a_subtype(player, card, SUBTYPE_IMP);
		get_card_instance(player, card)->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_LURKING_SKIRGE_ANIMATED);;
	}

	return global_enchantment(player, card, event);
}

// lukirng skirge animated --> vanilla

static int effect_memory_jar(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( eot_trigger(player, card, event ) ){
		new_discard_all(instance->targets[0].player, player);
		int i;
		for(i=1; i<19; i++){
			if( instance->targets[i].player != -1 && check_rfg(instance->targets[0].player, cards_data[instance->targets[i].player].id) ){
				add_card_to_hand(instance->targets[0].player, instance->targets[i].player);
				remove_card_from_rfg(instance->targets[0].player, cards_data[instance->targets[i].player].id);
			}
			if( instance->targets[i].card != -1 && check_rfg(instance->targets[0].player, cards_data[instance->targets[i].card].id) ){
				add_card_to_hand(instance->targets[0].player, instance->targets[i].card);
				remove_card_from_rfg(instance->targets[0].player, cards_data[instance->targets[i].card].id);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_memory_jar(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		// RFG hands
		int p;
		for(p=0;p<2;p++){
			int legacy = create_legacy_effect(player, card, &effect_memory_jar);
			card_instance_t *instance = get_card_instance(player, legacy);
			instance->targets[0].player = p;
			instance->targets[0].card = 1;
			int count = active_cards_count[p]-1;
			while( count > -1 ){
					if( in_hand(p, count) ){
						card_instance_t *crd = get_card_instance(p, count);
						int pos = instance->targets[0].card;
						if( pos < 19 ){
							if( instance->targets[pos].player == -1 ){
								instance->targets[pos].player = crd->internal_card_id;
							}
							else{
								instance->targets[pos].card = crd->internal_card_id;
								instance->targets[0].card++;
							}
						}
						rfg_card_in_hand(p, count);
					}
					count--;
			}
			draw_cards(p, 7);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}


int card_martyrs_cause(int player, int card, event_t event){

	return prevent_damage_sacrificing_a_creature(player, card, event, 2);
}

int card_might_of_oaks(int player, int card, event_t event){
	/*
	  Might of Oaks |3|G
	  Instant
	  Target creature gets +7/+7 until end of turn.
	*/
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 7, 7, 0, 0);
}

int card_miscalculation(int player, int card, event_t event){

	/* Miscalculation	|1|U
	 * Instant
	 * Counter target spell unless its controller pays |2.
	 * Cycling |2 */

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_molten_hydra(int player, int card, event_t event){
	/*
	  Molten Hydra |1|R
	  Creature - Hydra 1/1
	  {1}{R}{R}: Put a +1/+1 counter on Molten Hydra.
	  {T}, Remove all +1/+1 counters from Molten Hydra:
	  Molten Hydra deals damage to target creature or player equal to the number of +1/+1 counters removed this way.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XR(1, 2), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = instance->targets[1].card = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XR(1, 2), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, NULL, NULL) ){
				int ai_choice = 0;
				if( current_phase != PHASE_AFTER_BLOCKING && count_1_1_counters(player, card) > 2 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Add counter\n Deal damage\n Do nothing", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, (choice == 0 ? 1 : 0), 0, 0, 0, (choice == 0 ? 2 : 0), 0) ){
			if( choice == 1 ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					tap_card(player, card);
					int amount = count_1_1_counters(player, card);
					instance->targets[1].card = amount;
					remove_1_1_counters(player, card, amount);
					instance->info_slot = 67;
					instance->number_of_targets = 1;
				}
				else{
					return 0;
				}
			}
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			if( valid_target(&td) ){
				damage_target0(player, card, instance->targets[1].card);
			}
		}
	}

	return 0;
}

int card_mother_of_runes(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_multanis_acolyte(int player, int card, event_t event){

	echo(player, card, event, MANACOST_G(2));

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_multani_maro_sorcerer(int player, int card, event_t event){
	/* Multani, Maro-Sorcerer	|4|G|G
	 * Legendary Creature - Elemental 100/100
	 * Shroud
	 * ~'s power and toughness are each equal to the total number of cards in all players' hands. */

	check_legend_rule(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += hand_count[0] + hand_count[1];
	}

	return 0;
}

int card_no_mercy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_humiliated(player, card) ){
		return 0;
	}

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
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

	return global_enchantment(player, card, event);
}

int card_opal_avenger(int player, int card, event_t event){
	double_faced_card(player, card, event);
	if( life[player] < 11 ){
		add_a_subtype(player, card, SUBTYPE_SOLDIER);
		get_card_instance(player, card)->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_OPAL_AVENGER_ANIMATED);
	}
	return global_enchantment(player, card, event);
}

// opal avenger animated --> vanilla

int card_opal_champion(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_CREATURE, NULL, SUBTYPE_TREEFOLK);
}

// opal champion animated --> vanilla

int card_opportunity(int player, int card, event_t event){
	/*
	  Opportunity |4|U|U
	  Instant
	  Target player draws four cards.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			draw_cards(get_card_instance(player, card)->targets[0].player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_ostracize(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return opponent_is_valid_target(player, card);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_opponent(player, card);
	}
	if(event == EVENT_RESOLVE_SPELL ){
		if( opponent_is_valid_target(player, card) ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			new_effect_coercion(&ec, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_palinchron(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		untap_lands(player, card, 7);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent( instance->parent_controller, instance->parent_card );
		}
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 2), 0, NULL, NULL);
}

int card_peace_and_quiet(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 2, NULL);
}

int card_phyrexian_broodlings(int player, int card, event_t event)
{
  /* Phyrexian Broodlings	|1|B|B
   * Creature - Minion 2/2
   * |1, Sacrifice a creature: Put a +1/+1 counter on ~. */
	  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  int num_can_sac = max_can_sacrifice_as_cost(player, card, &test);
	  if (num_can_sac <= 0)
		return 0;

	  return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_X(1), num_can_sac);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (in_play(instance->parent_controller, instance->parent_card))
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_X(1), 0, NULL, NULL);
}

int card_phyrexian_plaguelord(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
				int ai_choice = count_subtype(player, TYPE_CREATURE, -1) > 1 ? 1 : 0;
				choice = do_dialog(player, player, card, -1, -1, " Sacrifice self for -4/-4\n Sacrifice creature for -1/-1\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 0 ){
			instance->info_slot = 4;
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
		}
		if( choice == 1 ){
			instance->info_slot = 1;
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->info_slot == 4 ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -instance->info_slot, -instance->info_slot);
			}
			if( instance->info_slot == 1 ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								-instance->info_slot, -instance->info_slot);
			}
		}
	}

	return 0;
}

int card_phyrexian_reclamation(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(1, 1), 2, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1)) ){
			int selected = select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
			if( selected == -1 ){
				spell_fizzled = 1;
				return 0;
			}
			else{
				lose_life(player, 2);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_planar_collapse(int player, int card, event_t event){

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if(event == EVENT_TRIGGER && count_subtype(2, TYPE_CREATURE, -1) > 3){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				state_untargettable(player, card, 1);
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				APNAP(p,{new_manipulate_all(player, card, p, &this_test, KILL_BURY);};);
				state_untargettable(player, card, 0);
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_purify(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT | TYPE_ARTIFACT);
		APNAP(p,{new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_SACRIFICE);
	}

	return basic_spell(player, card, event);;
}

int card_pyromancy(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->number_of_targets = 1;
				int count = active_cards_count[player]-1;
				int hand_array[60];
				int hand_index = 0;
				while(count > -1 ){
					if( in_hand(player, count) ){
						hand_array[hand_index] = count;
						hand_index++;
					}
					count--;
				}

				int discarded = internal_rand(hand_index);
				instance->info_slot = get_cmc(player, hand_array[discarded]);
				discard_card(player, hand_array[discarded]);
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && instance->info_slot > 0 ){
			damage_target0(player, card, instance->info_slot);
		}
	}

	return 0;
}

int card_quicksilver_amulet(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_ACTIVATE && player == AI){
		this_test.cmc = 4;
		this_test.cmc_flag = 2;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier -= 25;
		}
		this_test.cmc = -1;
		this_test.cmc_flag = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_rack_and_ruin(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 2, NULL);
}

int card_radiants_dragoons(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XW(3, 1));

	if( comes_into_play(player, card, event) ){
	   gain_life(player, 5);
	}

	return 0;
}

int card_radiants_judgement(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with Power 4 or greater.", 1, NULL);
}

int card_radiant_archangel(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS)  && affect_me(player, card) && ! is_humiliated(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.not_me = 1;
		event_result += check_battlefield_for_special_card(player, card, 2, CBFSC_GET_COUNT, &this_test);
	}

	return 0;
}

int card_rancor(int player, int card, event_t event){
	// original code : 01203F1F

	immortal_enchantment(player, card, event);

	return generic_aura(player, card, event, player, 2, 0, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_raven_familiar(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XU(2, 1));

	if( comes_into_play(player, card, event) ){
		impulse_effect(player, 3, 0);
	}

	return 0;
}

int card_rebuild(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, 2, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_SACRIFICE);
	}

	return us_cycling(player, card, event);
}

int card_repopulate(int player, int card, event_t event){

	/* Repopulate	|1|G
	 * Instant
	 * Shuffle all creature cards from target player's graveyard into that player's library.
	 * Cycling |2 */

	if (!IS_CASTING(player, card, event)){
		return us_cycling(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			const int* grave = get_grave(instance->targets[0].player);
			int count = count_graveyard(instance->targets[0].player)-1;
			int amount = 0;
			while( count > -1 ){
					if( is_what(-1, grave[count], TYPE_CREATURE) ){
						from_graveyard_to_deck(instance->targets[0].player, count, 2);
						amount++;
					}
					count--;
			}
			if( amount > 0 ){
				shuffle(instance->targets[0].player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_ring_of_gix(int player, int card, event_t event){

	echo(player, card, event, MANACOST_X(3));

	return card_icy_manipulator(player, card, event);
}

int card_scrapheap(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_ARTIFACT | TYPE_ENCHANTMENT, 0);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		gain_life(player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_second_chance(int player, int card, event_t event){

	if( life[player] < 6 && current_turn == player && upkeep_trigger(player, card, event) ){
		return card_time_walk(player, card, EVENT_RESOLVE_SPELL);
	}

	return global_enchantment(player, card, event);
}

int card_sick_and_tired(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, -1, -1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_simian_grunts(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(2, 1));

	return flash(player, card, event);
}

int card_snap2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			untap_lands(player, card, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_spawning_pool(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XB(1, 1));
}

int card_spawning_pool_animated(int player, int card, event_t event){

	if( ! paying_mana() && IS_GAA_EVENT(event) ){
		return regeneration(player, card, event, MANACOST_B(1));
	}

	return manland_animated(player, card, event, MANACOST_XB(1, 1));
}

int card_subversion(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int result = lose_life(1-player, 1);
		gain_life(player, result);
	}

	return global_enchantment(player, card, event);
}

int card_swat(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with Power 2 or less.", 1, NULL);
}

int card_thran_lens(int player, int card, event_t event)
{
  if (event == EVENT_SET_COLOR && ! is_humiliated(player, card)
	  && in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_PERMANENT)
	  && in_play(player, card))
	event_result = 0;	// COLOR_TEST_COLORLESS doesn't work, and I have no idea why.

  return 0;
}

int card_thran_war_machine(int player, int card, event_t event){
	echo(player, card, event, MANACOST_X(4));
	attack_if_able(player, card, event);
	return 0;
}

int card_thran_weaponry(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	echo(player, card, event, MANACOST_X(4));

	choose_to_untap(player, card, event);

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && affect_me(player, card) ){
		int ai_choice = 0;
		if( count_subtype(player, TYPE_CREATURE, -1) > count_subtype(1-player, TYPE_CREATURE, -1) ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Untap\n Leave tapped", ai_choice);
		if( choice == 1 ){
			instance->untap_status &= ~3;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		parent->targets[1].player = 66;
	}

	if( is_tapped(player, card) ){
		if( instance->targets[1].player == 66 ){
			boost_creature_type(player, card, event, -1, 2, 2, 0, BCT_INCLUDE_SELF);
		}
	}
	else{
		instance->targets[1].player = 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_ticking_gnomes(int player, int card, event_t event){

	echo(player, card, event, MANACOST_X(3));

	return card_mogg_fanatic(player, card, event);
}

int card_tinker(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST ){
		if( basic_spell(player, card, event) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_treetop_village(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return manland_normal(player, card, event, MANACOST_XG(1, 1));
}

int card_treetop_village_ape(int player, int card, event_t event){
	return manland_animated(player, card, event, MANACOST_XG(1, 1));
}

int card_unearth(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}

	char msg[100] = "Select a creature card with CMC 3 or less.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.cmc = 4;
	this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_urzas_blueprints(int player, int card, event_t event){

	echo(player, card, event, MANACOST_X(6));

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_viashino_cutthroath(int player, int card, event_t event){
	haste(player, card, event);
	if( eot_trigger(player, card, event) ){
		bounce_permanent(player, card);
	}
	return 0;
}

int card_viashino_heretic(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_player(instance->targets[0].player, amount, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, "TARGET_ARTIFACT");
}

// viashino sandscout --> viashino cutthroat

int card_walking_sponge(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int abils[4] = {0, KEYWORD_FLYING, KEYWORD_TRAMPLE, KEYWORD_FIRST_STRIKE};
			int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
								"Target loses Flying", check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING), 10,
								"Target loses Trample", check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_TRAMPLE), 8,
								"Target loses First Strike", check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FIRST_STRIKE), 5);
			negate_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, abils[choice]);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_weatherseed_treefolk(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags(player, card, SF_CORRECTLY_RESOLVED);
	}

	if( graveyard_from_play(player, card, event) && instance->targets[9].card == 66 && ! is_token(player, card) ){
		add_card_to_hand(player, instance->internal_card_id);
		instance->kill_code = KILL_REMOVE;
	}

	int owner, position;
	if (check_special_flags(player, card, SF_CORRECTLY_RESOLVED) && ! is_token(player, card) &&
		this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position)
	  ){
		const int *grave = get_grave(owner);
		add_card_to_hand(owner, grave[position]);
		remove_card_from_grave(owner, position);
	}

	return 0;
}

int card_wheel_of_torture(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, 1-player, 3-hand_count[1-player] > 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int dmg = 3-hand_count[1-player];
		damage_player(1-player, dmg, player, card);
	}

	return 0;
}

int card_wing_snare(int player, int card, event_t event){
	/*
	  Wing Snare |2|G
	  Sorcery
	  Destroy target creature with flying.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with flying.", 1, NULL);
}

int card_yavimaya_granger(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(2, 1));

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		tutor_basic_land(player, 1, 1);
	}

	return 0;
}

