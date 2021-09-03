#include "manalink.h"

// Functions

// Cards
int card_acolythe_of_xathrid(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XB(1, 1), 0, &td, "TARGET_PLAYER");
}

int card_act_of_treason(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].player != player ){
				effect_act_of_treason( player, card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int effect_must_attack(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].card > -1 ){
		attack_if_able(instance->targets[0].player, instance->targets[0].card, event);
	}
	if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_alluring_siren(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = opp;
	td.preferred_controller = opp;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		if( player == AI && current_turn == player ){
			ai_modifier-=100;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, instance->parent_card, &effect_must_attack, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target creature your opponent controls.");
}

int card_ant_queen(int player, int card, event_t event){
	/* Ant Queen	|3|G|G
	 * Creature - Insect 5/5
	 * |1|G: Put a 1/1 |Sgreen Insect creature token onto the battlefield. */

	if(event == EVENT_RESOLVE_ACTIVATION ) {
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL);
}

int card_awakener_druid(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = SUBTYPE_FOREST;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_LAND") ){
				add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_TREEFOLK);
				land_animation2(player, card, instance->targets[0].player, instance->targets[0].card, 2, 4, 5, 0, 0, 0, 0);
			}
		}
	}

	return 0;
}

int card_baneslayer_angel(int player, int card, event_t event){
	lifelink(player, card, event);
	protection_from_subtype(player, card, event, SUBTYPE_DEMON);
	protection_from_subtype(player, card, event, SUBTYPE_DRAGON);
	return 0;
}

int card_berserkers_of_blood_ridge(int player, int card, event_t event){
	attack_if_able(player, card, event);
	return 0;
}

int card_bountiful_harvest(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_subtype(player, TYPE_LAND, -1));
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}


int card_burning_inquiry(int player, int card, event_t event){
	/*
	  Burning Inquiry |R
	  Sorcery
	  Each player draws three cards, then discards three cards at random.
	*/

	if(event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {
					draw_cards(p, 3);
					new_multidiscard(p, 3, DISC_RANDOM, player);
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_burst_of_speed(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_HASTE);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_capricious_efreet(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);
		td1.illegal_type = TYPE_LAND;
		td1.allowed_controller = 1-player;
		td1.preferred_controller = 1-player;

		instance->number_of_targets = 0;
		int chosen = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			chosen++;
		}
		if( can_target(&td1) ){
			int max = 0;
			while( max < 2 ){
					if( new_pick_target(&td1, "TARGET_PERMANENT", max+chosen, 0) ){
						state_untargettable(instance->targets[max+chosen].player, instance->targets[max+chosen].card, 1);
						max++;
					}
					else{
						break;
					}
			}
			chosen+=max;
		}
		if( chosen > 0 ){
			int rnd = internal_rand(chosen);
			if( chosen == 1 ){
				rnd = 0;
			}
			kill_card(instance->targets[rnd].player, instance->targets[rnd].card, KILL_DESTROY);
		}
		remove_state(1-player, -1, STATE_CANNOT_TARGET);
	}

	return 0;
}

int card_captain_of_the_watch( int player, int card, event_t event){
	/* Captain of the Watch	|4|W|W
	 * Creature - Human Soldier 3/3
	 * Vigilance
	 * Other Soldier creatures you control get +1/+1 and have vigilance.
	 * When ~ enters the battlefield, put three 1/1 |Swhite Soldier creature tokens onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = 3;
		generate_token(&token);
	}

	vigilance(player, card, event);
	if(	in_play(player, card) && (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
		affected_card_controller == player && has_creature_type(player, affected_card, SUBTYPE_SOLDIER) &&
		! affect_me(player, card)
	  ){
		switch( event ){
				case EVENT_POWER:
				case EVENT_TOUGHNESS:
					event_result++;
					break;
				case EVENT_ABILITIES:
					vigilance(player, affected_card, event);
					break;
				default:
					break;
		}
	}

	return 0;
}

int card_cemetery_reaper(int player, int card, event_t event){
	/* Cemetery Reaper	|1|B|B
	 * Creature - Zombie 2/2
	 * Other Zombie creatures you control get +1/+1.
	 * |2|B, |T: Exile target creature card from a graveyard. Put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			if( (count_graveyard_by_type(AI, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(AI)) ||
				(count_graveyard_by_type(HUMAN, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(HUMAN))
			  ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(2, 1)) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");
			if( select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &this_test, 0, 1) != -1 ){
				int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
				if( selected != -1 ){
					rfg_card_from_grave(instance->targets[0].player, selected);
					tap_card(player, card);
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

	if(event == EVENT_RESOLVE_ACTIVATION){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
	}

	return boost_creature_type(player, card, event, SUBTYPE_ZOMBIE, 1, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_child_of_night(int player, int card, event_t event){

	lifelink(player, card, event);

	return 0;
}

int card_deadly_recluse(int player, int card, event_t event){
	/* Deadly Recluse	|1|G
	 * Creature - Spider 1/2
	 * Reach
	 * Deathtouch */

	/* Deathgaze Cockatrice	|2|B|B
	 * Creature - Cockatrice 2/2
	 * Flying
	 * Deathtouch */

	/* Giant Scorpion	|2|B
	 * Creature - Scorpion 1/3
	 * Deathtouch */

	/* Greater Basilisk	|3|G|G
	 * Creature - Basilisk 3/5
	 * Deathtouch */

	/* Kessig Recluse	|2|G|G
	 * Creature - Spider 2/3
	 * Reach
	 * Deathtouch */

	/* Pharika's Chosen	|B
	 * Creature - Snake 1/1
	 * Deathtouch */

	/* Sedge Scorpion	|G
	 * Creature - Scorpion 1/1
	 * Deathtouch */

	/* Thornweald Archer	|1|G
	 * Creature - Elf Archer 2/1
	 * Reach
	 * Deathtouch */

	/* Tidehollow Strix	|U|B
	 * Artifact Creature - Bird 2/1
	 * Flying
	 * Deathtouch */

	/* Typhoid Rats	|B
	 * Creature - Rat 1/1
	 * Deathtouch */

	deathtouch(player, card, event);
	return 0;
}

int card_divine_verdict(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking or blocking creature.", 1, NULL);
}

int card_gargoyle_castle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(6), 0, NULL, NULL) ){
			int ai_choice = count_permanents_by_type(player, TYPE_LAND) > 6 ? 1 : 0;
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Generate a Gargoyle\n Cancel", ai_choice);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(5)) ){
				tap_card(player, card);
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GARGOYLE, &token);
		token.pow = 3;
		token.tou = 4;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return 0;
}

int card_gorgon_flail(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 1, 1, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_guardian_seraph(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_player != player && damage->damage_target_card == -1 && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				damage->info_slot--;
			}
		}
	}

	return 0;
}

int card_harms_way(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td1, NULL, 1, NULL) ){
			return can_target(&td2);
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td1, "TARGET_DAMAGE") ){
			new_pick_target(&td2, "TARGET_CREATURE_OR_PLAYER", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td2, 1) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot <= 2 ){
				target->damage_target_player = instance->targets[1].player;
				target->damage_target_card = instance->targets[1].card;
			}
			else{
				target->info_slot-=2;
				damage_creature(instance->targets[1].player, instance->targets[1].card, 2, target->damage_source_player, target->damage_source_card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hive_mind(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_SORCERY | TYPE_INSTANT, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		copy_spell_from_stack(1-instance->targets[1].player, instance->targets[1].player, instance->targets[1].card);
	}

	return global_enchantment(player, card, event);
}

int card_honor_of_the_pure( int player, int card, event_t event){

	boost_creature_by_color(player, card, event, COLOR_TEST_WHITE, 1, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	return global_enchantment(player, card, event);
}

// illusionary servant --> skulking ghost

// jackal familiar --> mogg conscripts

// kalonian behemoth --> vanilla

int card_lurking_predators(int player, int card, event_t event){

	if( ai_is_speculating !=1 && specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			show_deck( player, deck, 1, "Here's the first card of deck.", 0, 0x7375B0 );
			if( is_what(-1, deck[0], TYPE_CREATURE) ){
				put_into_play_a_card_from_deck(player, player, 0);
			}
			else{
				int choice = do_dialog(player, player, card, -1, -1, " Put the first card on bottom\n Pass", 0);
				if( choice == 0 ){
					put_top_card_of_deck_to_bottom(player);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_magebane_armor(int player, int card, event_t event){

	card_instance_t* instance;

	if( (instance = in_play(player, card)) && is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 2, 4, 0);
		if( event == EVENT_ABILITIES && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result &= ~KEYWORD_FLYING;
		}

		card_instance_t* damage = noncombat_damage_being_prevented(event);
		if( damage && damage->damage_target_card == instance->targets[8].card && damage->damage_target_player == instance->targets[8].player ){
			damage->info_slot = 0;
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_master_of_the_wild_hunt(int player, int card, event_t event){
	/* Master of the Wild Hunt	|2|G|G
	 * Creature - Human Shaman 3/3
	 * At the beginning of your upkeep, put a 2/2 |Sgreen Wolf creature token onto the battlefield.
	 * |T: Tap all untapped Wolf creatures you control. Each Wolf tapped this way deals damage equal to its power to target creature. That creature deals damage equal to its power divided as its controller chooses among any number of those Wolves. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WOLF, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		int mode = 0;
		if( player != AI ){
			mode = do_dialog(1-player, player, card, -1, -1, " Damaging : auto mode\n Damaging : manual mode", 0);
		}

		int incoming_damage = get_power(instance->targets[0].player, instance->targets[0].card);
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( is_what(player, count, TYPE_CREATURE) && has_subtype(player, count, SUBTYPE_WOLF) && in_play(player, count) && ! is_tapped(player, count) ){
					tap_card(player, count);
					damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(player, count), player, count);
					if( mode == 0 ){
						int wolfie = get_toughness(player, count);
						if(incoming_damage >= wolfie){
							damage_creature(player, count, wolfie,instance->targets[0].player, instance->targets[0].card );
							incoming_damage-=wolfie;
						}
						else if(incoming_damage > 0){
								damage_creature(player, count, incoming_damage,instance->targets[0].player, instance->targets[0].card );
								incoming_damage = 0;
						}
					}
					if( mode == 1 ){
						if(incoming_damage > 0){
							get_card_instance(player, count)->token_status |= STATUS_RED_BORDER;
							int number = choose_a_number(player, "Assign how much damage?", incoming_damage);
							get_card_instance(player, count)->token_status &= ~STATUS_RED_BORDER;
							if(number > incoming_damage){
								number = incoming_damage;
							}
							damage_creature(player, count, number, instance->targets[0].player, instance->targets[0].card );
							incoming_damage-=number;
						}
					}
				}
				count--;
		}
	}


	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_mirror_of_fate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a card to put into your deck.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int max = 0;
		int count = count_rfg(player);
		int tutored[7];
		int *rfg = rfg_ptr[player];
		while( max < 7 && count > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_RFG, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					tutored[max] = rfg[selected];
					remove_card_from_rfg(player, cards_data[rfg[selected]].id);
					max++;
					count--;
				}
				else{
					break;
				}
		}
		int c1 = count_deck(player)-1;
		while( c1 > -1 ){
				rfg_card_in_deck(player, c1);
				c1--;
		}
		if( max > 0 ){
			count = 0;
			while( count < max ){
					int card_added = add_card_to_hand(player, tutored[count]);
					put_on_top_of_deck(player, card_added);
					count++;
			}
			rearrange_top_x(player, player, max);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_open_the_vaults(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{reanimate_all(p, -1, p, TYPE_ARTIFACT | TYPE_ENCHANTMENT,  0, 0, 0, 0, 0, 0, 0, -1, 0, REANIMATE_DEFAULT);};);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_palace_guard(int player, int card, event_t event)
{
	// 415910
	creature_can_block_additional(player, card, event, 255);

	return 0;
}

int card_planar_cleansing(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;
		APNAP(p,{new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_rhox_pikemaster(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 0, 0, KEYWORD_FIRST_STRIKE, BCT_CONTROLLER_ONLY);
	return 0;
}

int card_sphinx_ambassador(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int *deck = deck_ptr[1-player];
		if( deck[0] != -1 ){
			char msg[100] = "Select a card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			if( player == AI ){
				new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			}
			int selected = new_select_a_card(player, 1-player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
			if( selected != -1 ){
				int id = -1;
				if( 1-player != AI ){
					id = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
				else{
					int os = new_select_a_card(1-player, 1-player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
					id = os != -1 ? cards_data[deck[os]].id : CARD_ID_AIR_ELEMENTAL;
					card_ptr_t* c = cards_ptr[ id  ];
					char buffer[100];
					scnprintf(buffer, 100, " Opponent named: %s", c->name);
					do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
				}
				if( player == AI ){
					card_ptr_t* c = cards_ptr[ cards_data[deck[selected]].id  ];
					char buffer[100];
					scnprintf(buffer, 100, " Opponent selected: %s", c->name);
					do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
				}
				if( cards_data[deck[selected]].id != id && is_what(-1, deck[selected], TYPE_CREATURE) ){
					int card_added = add_card_to_hand(player, deck[selected]);
					remove_card_from_deck(1-player, selected);
					put_into_play(player, card_added);
					if( 1-player == AI ){
						add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
					}
					else{
						remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
					}
				}
			}
			shuffle(1-player);
		}
	}
	return 0;
}

static const char* target_is_skeleton_vampire_or_zombie(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (has_subtype(player, card, SUBTYPE_SKELETON) || has_subtype(player, card, SUBTYPE_VAMPIRE) || has_subtype(player, card, SUBTYPE_ZOMBIE)
		  ? NULL : EXE_STR(0x73964C));	//",subtype"
}
int card_undead_slayer(int player, int card, event_t event)
{
  /* Undead Slayer	|2|W
   * Creature - Human Cleric 2/2
   * |W, |T: Exile target Skeleton, Vampire, or Zombie. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_is_skeleton_vampire_or_zombie;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT,
								   MANACOST_W(1), 0, &td, "Select target Skeleton, Vampire, or Zombie.");
}

int card_vampire_nocturnus(int player, int card, event_t event){
	reveal_top_card(player, card, event);
	int* deck = deck_ptr[player];
	if( in_play(player, card) && (get_color_by_internal_id(player, deck[0]) & COLOR_TEST_BLACK) ){
		boost_creature_type(player, card, event, SUBTYPE_VAMPIRE, 2, 1, KEYWORD_FLYING, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	}
	return 0;
}

int card_veteran_armorsmith( int player, int card, event_t event){
	return boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 0, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_veteran_swordsmith( int player, int card, event_t event){
	return boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 1, 0, 0, BCT_CONTROLLER_ONLY);
}

int card_xathrid_demon(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( count_subtype(player, TYPE_CREATURE, -1) > 1 ){
			state_untargettable(player, card, 1);
			int result = pick_creature_for_sacrifice(player, card, 1);
			if( result > -1 ){
				lose_life(1-player, get_power(player, result));
				kill_card(player, result, KILL_SACRIFICE);
				kill = 0;
			}
			state_untargettable(player, card, 0);
		}
		if( kill > 0 ){
			tap_card(player, card);
			lose_life(player, 7);
		}
	}

	return 0;
}

