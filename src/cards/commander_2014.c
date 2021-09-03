#include "manalink.h"

// Functions
int liutenant(int player){
	return player_bits[player] & PB_COMMANDER_IN_PLAY;
}

// New Cards
// White
int card_angel_of_the_dire_hour(int player, int card, event_t event){
	/*
	  Angel of the Dire Hour |5|W|W
	  Creature - Angel
	  Flash
	  Flying
	  When Angel of the Dire Hour enters the battlefield, if you cast it from your hand, exile all attacking creatures.
	  5/4
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) && instance->info_slot != 1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);	};);
	}

	return flash(player, card, event);
}

int card_angelic_field_marshal(int player, int card, event_t event){
	/*
	  Angelic Field Marshal |2|W|W
	  Creature - Angel
	  Flying
	  Lieutenant - As long as you control your commander, Angelic Field Marshal gets +2/+2 and creatures you control have vigilance.
	  3/3
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) && liutenant(player) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=2;
		}
		if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			vigilance(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

int card_benevolent_offering(int player, int card, event_t event){
	/*
	  Benevolent Offering |3|W
	  Instant
	  Choose an opponent. You and that player each put three 1/1 white Spirit creature tokens with flying onto the battlefield.
	  Choose an opponent. You gain 2 life for each creature you control and that player gains 2 life for each creature he or she controls.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_SPIRIT, &token);
				token.t_player = p;
				token.qty = 3;
				token.color_forced = COLOR_TEST_WHITE;
				token.key_plus = KEYWORD_FLYING;
				generate_token(&token);
				};
		);
		APNAP(p,{gain_life(p, (count_subtype(p, TYPE_CREATURE, -1)*2));};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int comeuppance_legacy(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == -1 && damage->damage_source_player != player && damage->info_slot > 0
		  ){
			int amount = damage->info_slot;
			damage->info_slot = 0;
			if( damage->targets[3].player & TYPE_CREATURE ){
				damage_creature(damage->damage_source_player, damage->damage_source_card, amount, player, card);
			}
			else{
				damage_player(damage->damage_source_player, amount, player, card);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_comeuppance(int player, int card, event_t event){
	/*
	  Comeuppance |3|W
	  Instant
	  Prevent all damage that would be dealt to you and planeswalkers you control this turn by sources you don't control.
	  If damage from a creature source is prevented this way, Comeuppance deals that much damage to that creature.
	  If damage from a noncreature source is prevented this way, Comeuppance deals that much damage to the source's controller.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &comeuppance_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_containment_priest(int player, int card, event_t event){
	/*
	  Containment Priest |1|W
	  Creature - Human Cleric
	  Flash
	  If a nontoken creature would enter the battlefield and it wasn't cast, exile it instead.
	  2/2
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
			if( ! is_token(trigger_cause_controller, trigger_cause) && check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST) ){
				if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					card_instance_t *instance = get_card_instance(player, card);
					kill_card(instance->targets[1].player, instance->targets[1].card, KILL_REMOVE);
				}
			}
		}
	}

	return flash(player, card, event);
}

int card_deploy_to_the_front(int player, int card, event_t event){
	/*
	  Deploy to the Front |5|W|W
	  Sorcery
	  Put X 1/1 white Soldier creature tokens onto the battlefield, where X is the number of creatures on the battlefield.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.qty = count_subtype(ANYBODY, TYPE_CREATURE, -1);
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_fell_the_mighty(int player, int card, event_t event){
	/*
	  Fell the Mighty |4|W
	  Sorcery
	  Destroy all creatures with power greater than target creature's power.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td2) ){
			int pow = get_power(instance->targets[0].player, instance->targets[0].card);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.power = pow;
			this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
			APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td2, "TARGET_CREATURE", 1, NULL);
}

int card_hallowed_spiritkeeper(int player, int card, event_t event){
	/*
	  Hallowed Spiritkeeper |1|W|W
	  Creature - Avatar
	  Vigilance
	  When Hallowed Spiritkeeper dies, put X 1/1 white Spirit tokens with flying onto the battlefield, where X is the number of creature cards in your graveyard.
	  3/2
	*/

	vigilance(player, card, event);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = count_graveyard_by_type(player, TYPE_CREATURE);
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return 0;
}

int card_jazal_goldmane(int player, int card, event_t event){
	/*
	  Jazal Goldmane |2|W|W
	  Legendary Creature - Cat Warrior
	  First Strike
	  3WW: Attacking creatures you control get +X/+X until end of turn, where X is the number of attacking creatures.
	  4/4
	*/

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = count_attackers(player);
		int c;
		for(c=active_cards_count[player]-1; c>-1; c--){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && is_attacking(player, c) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, player, c, amount, amount);
			}
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(3, 2), 0, NULL, NULL);
}

// Kor Soldier --> Rhino

int card_nahiri_the_lithomancer(int player, int card, event_t event){
	/*
	  Nahiri, the Lithomancer |3|W|W
	  Planeswalker - Nahiri
	  +2: Put a 1/1 white Kor Soldier creature token onto the battlefield. You may attach an Equipment you control to it.
	  -2: You may put an Equipment card from your hand or graveyard onto the battlefield.
	  -10: Put a colorless Equipment artifact token named Stoneforged Blade onto the battlefield.
	  It has indestructible, "Equipped creature gets +5/+5 and has double strike," and equip 0.
	  Nahiri, the Lithomancer can be your commander.
	  3
	*/
	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an Equipment card.");
		this_test.subtype = SUBTYPE_EQUIPMENT;
		this_test.zone = TARGET_ZONE_HAND;

		card_instance_t *instance = get_card_instance(player, card);

		int priorities[3] = {	can_target(&td) ? 10 : 5,
								check_battlefield_for_special_card(player, card, player, 0, &this_test) || new_special_count_grave(player, &this_test) ? 10 : 0,
								15,
		};

		enum{
			CHOICE_SOLDIER = 1,
			CHOICE_EQUIPMENT,
			CHOICE_BLADE
		} choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						  "Generate a Soldier", 1, priorities[0], 2,
						  "Tutor an Equipment", 1, priorities[1], -2,
						  "Emblem", 1, priorities[2], -10
						  );

		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
		}
		else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_SOLDIER:
				{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_KOR_SOLDIER, &token);
					token.keep_track_of_tokens_generated = 9;
					token.pow = 1;
					token.tou = 1;
					token.color_forced = COLOR_TEST_WHITE;
					generate_token(&token);

					if( can_target(&td) ){
						int k;
						for (k = 0; k < active_cards_count[player]; k++){
							if( in_play(player, k) && is_what(player, k, TYPE_CREATURE) ){
								state_untargettable(player, k, 1);
							}
						}
						unsigned int q;
						for (q = 0; q < token.keep_track_of_tokens_generated; q++){
							state_untargettable(instance->targets[q].player, instance->targets[q].card, 0);
						}

						if( new_pick_target(&td, "Select an equipment card.", 0, GS_LITERAL_PROMPT) ){
							target_definition_t td2;
							default_target_definition(player, card, &td2, TYPE_CREATURE);
							td2.allowed_controller = player;
							td2.preferred_controller = player;
							td2.illegal_abilities = 0;
							if( new_pick_target(&td2, "Select a Kor Soldier to equip.", 1, GS_LITERAL_PROMPT) ){
								equip_target_creature(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
							}
						}

						for (k = 0; k < active_cards_count[player]; k++){
							if( in_play(player, k) && is_what(player, k, TYPE_CREATURE) ){
								state_untargettable(player, k, 0);
							}
						}
					}
				}
				break;
				case CHOICE_EQUIPMENT:
				{
					int result = -1;
					if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
						result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
					}
					if( result == -1 && new_special_count_grave(player, &this_test) ){
						result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
					}
				}
				break;
				case CHOICE_BLADE:
				{
					generate_token_by_id(player, card, CARD_ID_STONEFORGED_BLADE);
				}
				break;
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_stoneforged_blade(int player, int card, event_t event){
	indestructible(player, card, event);
	return vanilla_equipment(player, card, event, 0, 5, 5, KEYWORD_DOUBLE_STRIKE, 0);
}


// Blue (10)

int card_aether_gale(int player, int card, event_t event){
	/*
	  AEther Gale |3|U|U
	  Sorcery
	  Return six target nonland permanents to their owners' hands.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int targets[6] = {0, 0, 0, 0, 0, 0};
		int i;
		for(i=0; i<6; i++){
			if( validate_target(player, card, &td, i) ){
				targets[i] = 1;
				if( has_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_AURA) ){
					targets[i] = 2;
				}
			}
		}
		for(i=0; i<6; i++){
			if( targets[i] == 2 ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		for(i=0; i<6; i++){
			if( targets[i] == 1 ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 6, NULL);
}

int card_breaching_leaviathan(int player, int card, event_t event){
	/*
	  Breaching Leviathan |7|U|U
	  Creature - Leviathan
	  When Breaching Leviathan enters the battlefield, if you cast it from your hand, tap all nonblue creatures. Those creatures don't untap during their controllers' next untap steps.
	  9/9
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) && instance->info_slot != 1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		this_test.color_flag = DOESNT_MATCH;
		APNAP(p,{
					int c;
					for(c = active_cards_count[p]-1; c > -1; c--){
						if( in_play(p, c) && new_make_test_in_play(p, c, -1, &this_test) ){
							effect_frost_titan(player, card, p, c);
						}
					}
				};
		);
	}

	return 0;
}

int card_domineering_will(int player, int card, event_t event){
	/*
	  Domineering Will |3|U
	  Instant
	  Target player gains control of up to three target nonattacking creatures until end of turn. Untap those creatures. They block this turn if able.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.illegal_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			int t = 1;
			while( t < 4 ){
					if( new_pick_target(&td2, "Select target nonattacking creature.", t, GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[t].player, instance->targets[t].card, 1);
						t++;
					}
					else{
						break;
					}
			}
			for(t=1; t<instance->number_of_targets; t++){
				state_untargettable(instance->targets[t].player, instance->targets[t].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->number_of_targets > 1 ){
				int i;
				for(i=1; i<instance->number_of_targets; i++){
					if( validate_target(player, card, &td2, i) ){
						untap_card(instance->targets[i].player, instance->targets[i].card);
						pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 0, 0, 0, SP_KEYWORD_MUST_BLOCK);
						if( instance->targets[0].player != instance->targets[i].player ){
							if( instance->targets[0].player == player ){
								gain_control_until_eot(player, card, instance->targets[i].player, instance->targets[i].card);
							}
							else{
								give_control_until_eot(player, card, instance->targets[i].player, instance->targets[i].card);
							}
						}
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dulcet_siren(int player, int card, event_t event){
	/*
	  Dulcet Sirens |2|U
	  Creature - Siren
	  U, T: Target creature attacks target opponent this turn if able.
	  Morph U
	  1/3
	*/
	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance( player, card);

		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_MUST_ATTACK);
		}

		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_U(1), 0, &td, "TARGET_CREATURE");
	}


	return morph(player, card, event, MANACOST_U(1));
}

int card_intellectual_offering(int player, int card, event_t event){
	/*
	  Intellectual Offering |4|U
	  Instant
	  Choose an opponent. You and that player each draw three cards.
	  Choose an opponent. Untap all nonland permanents you control and all nonland permanents that player controls.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{draw_cards(p, 3);};);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		APNAP(p,{new_manipulate_all(player, card, p, &this_test, ACT_UNTAP);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

//Kraken token --> Rhino token

int card_whale_token(int player, int card, event_t event){
	if( get_special_infos(player, card) == 66 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KRAKEN, &token);
		token.pow = token.tou = 9;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}
	return generic_token(player, card, event);
}

int card_fish_token(int player, int card, event_t event){
	if( get_special_infos(player, card) == 66 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WHALE, &token);
		token.pow = token.tou = 6;
		token.color_forced = COLOR_TEST_BLUE;
		token.special_infos = 66;
		generate_token(&token);
	}
	return generic_token(player, card, event);
}

int card_reef_worm(int player, int card, event_t event){
	/*
	  Reef Worm |3|U
	  Creature - Worm
	  When Reef Worm dies, put a 3/3 blue Fish creature token onto the battlefield with "When this creature dies, put a 6/6 blue Whale creature token onto
	  the battlefield with 'When this creature dies, put a 9/9 blue Kraken creature token onto the battlefield.'"
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_FISH, &token);
		token.pow = token.tou = 3;
		token.special_infos = 66;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}
	return generic_token(player, card, event);
}


int card_stitcher_geralf(int player, int card, event_t event){
	/*
	  Stitcher Geralf |3|U|U
	  Legendary Creature - Human Wizard
	  2U, T: Each player puts the top three cards of his or her library into his or her graveyard.
	  Exile up to two creature cards put into graveyards this way.
	  Put an X/X blue Zombie creature token onto the battlefield, where X is the total power of the cards exiled this way.
	  3/4
	*/

	card_instance_t* instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].player = instance->info_slot = 0;
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		APNAP(p, {special_mill(instance->parent_controller, instance->parent_card, CARD_ID_STITCHER_GERALF, p, 3);};);
		token_generation_t token;
		default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_ZOMBIE, &token);
		token.pow = token.tou = get_card_instance(instance->parent_controller, instance->parent_card)->info_slot;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(2, 1), 0, NULL, NULL);
}

int card_stormsurge_kraken(int player, int card, event_t event){
	/*
	  Stormsurge Kraken |3|U|U
	  Creature - Kraken
	  Hexproof
	  Lieutenant - As long as you control your commander, Stormsurge Kraken gets +2/+2 and has "Whenever Stormsurge Kraken becomes blocked, you may draw two cards."
	  5/5
	*/
	hexproof(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) && liutenant(player) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=2;
		}
		if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && ! is_unblocked(player, card) ){
			if( do_dialog(player, player, card, -1, -1, " Draw 2 cards\n Pass", count_deck(player) < 20 ? 1 : 0) == 0 ){
				draw_cards(player, 2);
			}
		}
	}

	return 0;
}

int card_teferis_emblem(int player, int card, event_t event){
	player_bits[player] |= PB_CAN_USE_PW_ABILITIES_AS_INSTANT;
	return 0;
}

int card_teferi_temporal_archmage(int player, int card, event_t event){
	/*
	  Teferi, Temporal Archmage |4|U|U
	  Planeswalker - Teferi
	  +1: Look at the top two cards of your library. Put one of them into your hand and the other on the bottom of your library.
	  -1: Untap up to four target permanents.
	  -10: You get an emblem with "You may activate loyalty abilities of planeswalkers you control on any player's turn any time you could cast an instant."
	  Tefri, Temporal Archmage can be your commander.
	  5
	*/
	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);
		if( player == AI ){
			td1.required_state = TARGET_STATE_TAPPED;
		}
		td1.preferred_controller = player;

		enum{
			CHOICE_LOOK_AT_TOP2 = 1,
			CHOICE_UNTAP_PERMANENTS,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Look at the top 2 cards", 1, 10, 1,
						"Untap up to 4 permanents", 1, target_available(player, card, &td1) * 5, -1,
						"Emblem", 1, count_subtype(player, TYPE_PERMANENT, SUBTYPE_PLANESWALKER)*6, -10);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			if( choice == CHOICE_UNTAP_PERMANENTS ){
				int i = 0;
				while( i < 4 && can_target(&td1) ){
						if( select_target(player, card, &td1, "Select target permanent to untap", &(instance->targets[i])) ){
							state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
							i++;
						}
						else{
							break;
						}
				}
				for(i=0; i<instance->number_of_targets; i++){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				}
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_LOOK_AT_TOP2:
			{
				if( deck_ptr[player][0] != -1 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to add to your hand");
					this_test.create_minideck = MIN(count_deck(player), 2);
					this_test.no_shuffle = 1;
					new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
					if( deck_ptr[player][0] != 1 ){
						put_top_x_on_bottom(player, player, 1);
					}
				}
			}
			break;

			case CHOICE_UNTAP_PERMANENTS:
				{
					int k;
					for(k=0; k<instance->number_of_targets; k++){
						if( validate_target(player, card, &td1, k) ){
							untap_card(instance->targets[k].player, instance->targets[k].card);
						}
					}
				}
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_TEFERIS_EMBLEM);
				break;
		  }
	}

	return planeswalker(player, card, event, 5);
}

int card_well_of_ideas(int player, int card, event_t event){
	/*
	  Well of Ideas |5|U
	  Enchantment
	  When Well of Ideas enters the battlefield, draw two cards.
	  At the beginning of each other player's draw step, that player draws an additional card.
	  At the beginning of your draw step, draw two additional cards.
	*/
	if( comes_into_play(player, card, event) ){
		draw_cards(player, 2);
	}

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DRAW_PHASE ){
			event_result += (current_turn == player ? 2 : 1);
		}
	}

	return global_enchantment(player, card, event);
}

// Black (11)
int card_demone_of_wailing_agonies(int player, int card, event_t event){
	/*
	  Demon of Wailing Agonies |3|B|B
	  reature - Demon
	  Flying
	  Lieutenant - As long as you control your commander, Demon of Wailing Agonies gets +2/+2 and has "Whenever Demon of Wailing Agonies deals combat damage to a player, that player sacrifices a creature."
	  4/4
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) && liutenant(player) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=2;
		}
		if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_TRACE_DAMAGED_PLAYERS) ){
			card_instance_t *instance = get_card_instance(player, card);
			int dp = instance->targets[1].player;
			instance->targets[1].player = 0;
			if( BYTE0(dp) ){
				impose_sacrifice(player, card, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			if( BYTE1(dp) ){
				impose_sacrifice(player, card, 1, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	return 0;
}

int card_flesh_carver(int player, int card, event_t event){
	/*
	  Flesh Carver |2|B
	  Creature - Human Wizard
	  Intimidate
	  1B, Sacrifice another creature: Put two +1/+1 counters on Flesh Carver.
	  When Flesh Carver dies, put an X/X black Horror creature token onto the battlefield, where where X is Flesh Carver's power.
	  2/2
	*/
	card_instance_t *instance = get_card_instance(player, card);

	intimidate(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card) && in_play(player, card) && get_card_instance(player, card)->kill_code > 0 &&
		get_card_instance(player, card)->kill_code < KILL_REMOVE
	  ){
		instance->targets[1].player = get_power(player, card);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HORROR, &token);
		token.pow = token.tou = instance->targets[1].player;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counters(instance->parent_controller, instance->parent_card, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST_XB(1, 1), 0, NULL, NULL);
}

int card_ghoulcaller_gisa(int player, int card, event_t event){
	/*
	  Ghoulcaller Gisa |3|B|B
	  Legendary Creature - Human Wizard
	  B, T, Sacrifice another creature: Put X 2/2 black Zombie creature tokens onto the battlefield, where X is the sacrificed creature's power.
	  3/4
	*/

	card_instance_t* instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE, MANACOST_B(1), 0, NULL, NULL) ){
			return count_subtype(player, TYPE_CREATURE, -1) > 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			state_untargettable(player, card, 1);
			add_state(player, card, STATE_TAPPED);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int result = sacrifice_and_report_value(player, card, player, SARV_REPORT_POWER, &this_test);
			if( result > -1 ){
				instance->info_slot = result;
				tap_card(player, card); //Send the event now to avoid strange interactions.
				instance->number_of_targets = 0;
			}
			else{
				remove_state(player, card, STATE_TAPPED);
				spell_fizzled = 1;
			}
			state_untargettable(player, card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.qty = instance->info_slot;
		generate_token(&token);
	}

	return 0;
}

int card_infernal_offering(int player, int card, event_t event){
	/*
	  Infernal Offering |4|B
	  Sorcery
	  Choose an opponent. You and that player each sacrifice a creature. Each player who sacrificed a creature this way draws two cards.
	  Choose an opponent. Return a creature card from your graveyard to the battlefield, then that player returns a creature card from his or her graveyard to the battlefield.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					if( can_sacrifice(player, p, 1, TYPE_CREATURE, 0) ){
						impose_sacrifice(player, card, p, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
						draw_cards(player, 2);
					}
				};
		);

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		APNAP(p,{
					if( count_graveyard_by_type(p, TYPE_CREATURE) ){
						new_global_tutor(p, p, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_malicious_affliction(int player, int card, event_t event){
	/*
	  Malicious Affliction |B|B
	  Instant
	  Morbid - When you cast Malicious Affliction, if a creature died this turn, you may copy Malicious Affliction and may choose a new target for the copy.
	  Destroy target nonblack creature.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 0, 1 | GS_LITERAL_PROMPT) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( ! check_special_flags(player, card, SF_NOT_CAST) && can_target(&td) && morbid() ){
				new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 1, GS_LITERAL_PROMPT);
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
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

static int necromantic_selection_legacy(int player, int card, event_t event){

	count_for_gfp_ability_and_store_values(player, card, event, ANYBODY, TYPE_CREATURE, NULL, GFPC_TRACK_DEAD_CREATURES | GFPC_EXTRA_SKIP_TOKENS, 0);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* instance = get_card_instance(player, card);
		int dead_array[10];
		int dac = 0;
		int i;
		for(i=0; i<10; i++){
			if( instance->targets[i].card != -1 ){
				dead_array[dac] = instance->targets[i].card;
				dac++;
			}
		}
		if( dac ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			int result = select_card_from_zone(player, player, dead_array, dac, 1, AI_MAX_CMC, -1, &this_test);
			seek_grave_for_id_to_reanimate(player, card, instance->targets[result].player, cards_data[instance->targets[result].card].id,
											REANIMATE_ADD_BLACK_ZOMBIE);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_necromantic_selection(int player, int card, event_t event){
	/*
	  Necromantic Selection |4|B|B|B
	  Sorcery
	  Destroy all creatures, then return a creature card put into a graveyard this way to the battlefield under your control.
	  It's a black Zombie in addition to its other colors and types. Exile Necromantic Selection.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &necromantic_selection_legacy);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_ob_nilixis_of_the_black_oath(int player, int card, event_t event){
	/*
	  Ob Nixilis of the Black Oath |3|B|B
	  Planeswalker - Nixilis
	  +2: Each opponent loses 1 life. You gain life equal to the life lost this way.
	  -2: Put a 5/5 black Demon creature token with flying onto the battlefield. You lose 2 life.
	  -8: You gain an emblem with "1B, Sacrifice a creature: You gain X life and draw X cards, where X is the sacrificed creature's power."
	  Ob Nixilis of the Black Oath can be your Commander.
	  3
	*/
	if (IS_ACTIVATING(event)){

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);
		if( player == AI ){
			td1.required_state = TARGET_STATE_TAPPED;
		}
		td1.preferred_controller = player;

		enum{
			CHOICE_LIFE = 1,
			CHOICE_DEMON,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Suck life", 1, ((20-life[1-player])*2) + ((20-life[player])*2), 2,
						"Generate Demon", 1, 10, -2,
						"Emblem", 1, 15, -8);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_LIFE:
			{
				int result = lose_life(1-player, 1);
				gain_life(player, result);
			}
			break;

			case CHOICE_DEMON:
				{
					generate_token_by_id(player, card, CARD_ID_DEMON);
					lose_life(player, 2);
				}
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_OB_NIXILIS_EMBLEM);
				break;
		  }
	}

	return planeswalker(player, card, event, 3);
}

int card_ob_nilixis_emblem(int player, int card, event_t event){
	//1B, Sacrifice a creature: You gain X life and draw X cards, where X is the sacrificed creature's power.
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_XB(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1)) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->info_slot);
		draw_cards(player, instance->info_slot);
	}
	return 0;
}

int card_overseer_of_the_damned(int player, int card, event_t event){
	/*
	  Overseer of the Damned |5|B|B
	  Creature - Demon
	  Flying
	  When Overseer of the Damned enters the battlefield, you may destroy target creature.
	  Whenever a nontoken creature an opponent controls dies, put a 2/2 black Zombie creature token onto the battlefield tapped.
	  5/5
	*/
	card_instance_t *instance = get_card_instance( player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( trigger_cause_controller == player && trigger_cause == card ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
			}
		}
	}

	count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = instance->targets[11].card;
		instance->targets[11].card = 0;
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.qty = amount;
		token.action = TOKEN_ACTION_TAPPED;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
	}

	return 0;
}

int card_raving_dead(int player, int card, event_t event){
	/*
	  Raving Dead |4|B
	  Creature - Zombie
	  Deathtouch
	  At the beginning of combat on your turn, choose an opponent at random. Raving Dead attacks that player this combat if able.
	  Whenever Raving Dead deals combat damage to a player, that player loses half his or her life, rounded down.
	  2/6
	*/

	attack_if_able(player, card, event);

	deathtouch(player, card, event);

	if(damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRACE_DAMAGED_PLAYERS)){
		card_instance_t* instance = get_card_instance(player, card);
		int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
		instance->targets[1].player = 0;
		int p;
		for (p = 0; p <= 1; ++p){
			for (; times_damaged[p] > 0; --times_damaged[p]){
				lose_life(p, round_down_value(life[p]));
			}
		}
	}

	return 0;
}

int card_spoils_of_blood(int player, int card, event_t event){
	/*
	  Spoils of Blood |B
	  Instant
	  Put an X/X black Horror creature token onto the battlefield, where X is the number of creatures that died this turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HORROR, &token);
		token.pow = token.tou = creatures_dead_this_turn;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_wake_the_dead(int player, int card, event_t event){
	/*
	  Wake the Dead |X|B|B
	  Instant
	  Cast Wake the Dead only during combat on an opponent's turn.
	  Return X target creatures from your graveyard to the battlefield. Sacrifice those creatures at the beginning of the next end step.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( current_turn != player && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
			return generic_spell(player, card, event, GS_GRAVE_RECYCLER | GS_X_SPELL, NULL, NULL, 1, &this_test);
		}
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		set_special_flags2(player, card, SF2_X_SPELL);
		int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
		int generic_mana_available = has_mana(player, COLOR_ANY, 1);	// A useful trick from card_fireball().
		max_cards = MIN(max_cards, generic_mana_available);
		max_cards = MIN(max_cards, 19);

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		char buf[100];
		if (ai_is_speculating == 1){
			*buf = 0;
		} else if (max_cards == 1){
			strcpy(buf, "Select a creature card.");
		} else {
			sprintf(buf, "Select up to %d creature cards.", max_cards);
		}
		new_default_test_definition(&this_test, TYPE_CREATURE, buf);
		instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, max_cards, &instance->targets[0]);

		if (instance->info_slot > 0){
			charge_mana(player, COLOR_COLORLESS, instance->info_slot);
		}
	}

	if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < instance->info_slot; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				int zombo = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				if( zombo != -1 ){
					pump_ability_t pump;
					default_pump_ability_definition(player, card, &pump, 0, 0, 0, 0);
					pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
					pump.eot_removal_method = KILL_SACRIFICE;
					pump_ability(player, card, player, zombo, &pump);
					++num_validated;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Red (10)
/*
Bitter Feud |4|R --> Furnace of Rath
Enchantment
As Bitter Feud enters the battlefield, choose two players.
If a source controlled by one of the chosen players would deal damage to the other chosen player or a permanent that player controls, that source deals double that damage to that player or permanent instead.
*/

int card_daretti_scrap_savant(int player, int card, event_t event){
	/*
	  Daretti, Scrap Savant |3|R
	  Planeswalker - Daretti
	  +2: Discard up to two cards, then draw that many cards.
	  -2: Sacrifice an artifact. If you do, return target artifact card from your graveyard to the battlefield.
	  -10: You get an emblem with "Whenever an artifact is put into your graveyard from the battlefield, return that card to the battlefield at the beginning of the next end step."
	  Daretti, Scrap Savant can be your commander.
	  3
	*/
	if (IS_ACTIVATING(event)){

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);
		if( player == AI ){
			td1.required_state = TARGET_STATE_TAPPED;
		}
		td1.preferred_controller = player;

		enum{
			CHOICE_DISCARD_DRAW = 1,
			CHOICE_SAC_ARTIFACT,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Discard & draw", 1, 8, 2,
						"Sac & return artifact", count_subtype(player, TYPE_ARTIFACT, -1) > 0, 10, -2,
						"Emblem", 1, 15, -10);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_DISCARD_DRAW:
			{
				int discarded = 0;
				while( hand_count[player] && discarded < 2 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard");
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						discard_card(player, selected);
						discarded++;
					}
					else{
						break;
					}
				}
				draw_cards(player, discarded);
			}
			break;

			case CHOICE_SAC_ARTIFACT:
				{
					test_definition_t test;
					new_default_test_definition(&test, TYPE_ARTIFACT, "Select an arifact to sacrifice.");
					int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
					if( sac ){
						kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
						if( new_special_count_grave(player, &test) ){
							new_default_test_definition(&test, TYPE_ARTIFACT, "Select an arifact card.");
							new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
						}
					}
				}
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_DARETTIS_EMBLEM);
				break;
		  }
	}

	return planeswalker(player, card, event, 3);
}

int card_daretti_emblem(int player, int card, event_t event){
	// Whenever an artifact is put into your graveyard from the battlefield, return that card to the battlefield at the beginning of the next end step.
	card_instance_t *instance = get_card_instance(player, card);

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_ARTIFACT, NULL, GFPC_TRACK_DEAD_CREATURES | GFPC_EXTRA_SKIP_TOKENS, 0);

	if( instance->targets[11].player > 0 && eot_trigger(player, card, event) ){
		int gfc_flag = check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE);
		int added[20];
		int ac = 0;
		int k;
		for(k=0; k<10; k++){
			if( instance->targets[k].player != -1 ){
				if( ! is_what(-1, instance->targets[k].player, TYPE_CREATURE) || ! gfc_flag ){
					int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].player].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
					if( result != -1 ){
						remove_card_from_grave(player, result);
						added[ac] = instance->targets[k].player;
						ac++;
					}
				}
				instance->targets[k].player = -1;
			}
			if( instance->targets[k].card != -1 ){
				if( ! is_what(-1, instance->targets[k].card, TYPE_CREATURE) || ! gfc_flag ){
					int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].card].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
					if( result != -1 ){
						remove_card_from_grave(player, result);
						added[ac] = instance->targets[k].card;
						ac++;
					}
				}
				instance->targets[k].card = -1;
			}
		}
		for(k=0; k<ac; k++){
			int card_added = add_card_to_hand(player, added[k]);
			set_special_flags3(player, card_added, SF3_REANIMATED);
			put_into_play(player, card_added);
		}
		instance->targets[11].player = 0;
	}

	return 0;
}

int card_dualcaster_mage(int player, int card, event_t event){
	/*
	  Dualcaster Mage |1|R|R
	  Creature - Human Wizard
	  Flash
	  When Dualcaster Mage enters the battlefield, copy target instant or sorcery spell. You may choose new targets for the copy.
	  2/2
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( counterspell(player, card, event, NULL, 0) ){
			instance->info_slot = 1;
			return 99;
		}
		else{
			instance->info_slot = 0;
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 ){
			counterspell(player, card, event, NULL, 0);
		}
		else{
			ai_modifier-=30;
		}
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot == 1 ){
			if( counterspell_validate(player, card, NULL, 0) ){
				copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_feldon_of_the_third_path(int player, int card, event_t event){
	/*
	  Feldon of the Third Path |1|R|R
	  Legendary Creature - Human Artificer
	  2R,T: Put a token onto the battlefield that's a copy of target creature card in your graveyard, except it's an artifact in addition to its other types.
	  It gains haste. Sacrifice it at the beginning of the next end step.
	  2/3
	*/
	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XR(2, 1), 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				return ! graveyard_has_shroud(player);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			token_generation_t token;
			default_token_definition(player, card, cards_data[get_grave(player)[selected]].id, &token);
			token.action = TOKEN_ACTION_CONVERT_INTO_ARTIFACT;
			token.legacy = 1;
			token.special_code_for_legacy = &haste_and_sacrifice_eot;
			generate_token(&token);
		}
	}

	return 0;
}

int card_impact_resonance(int player, int card, event_t event){
	/*
	Impact Resonance |1|R
	Instant
	Impact Resonance deals X damage divided as you choose among any number of target creatures, where X is the greatest amount of damage dealt by a source to a permanent or player this turn.
	*/
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
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = get_trap_condition(player, TRAP_MAX_DAMAGE_DEALT);
		}
		if( instance->info_slot == 0 ){
			ai_modifier-=30;
		}
		instance->number_of_targets = 0;
		int trgs = 0;
		while( instance->number_of_targets < instance->info_slot ){
				char msg[100];
				scnprintf(msg, 100, "Select target creature (%d of %d).", trgs+1, instance->info_slot);
				if( new_pick_target(&td, msg, trgs, GS_LITERAL_PROMPT) ){
					add_state(instance->targets[trgs].player, instance->targets[trgs].card, STATE_TARGETTED);
					trgs++;
				}
				else{
					spell_fizzled = 1;
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_incite_rebellion(int player, int card, event_t event){
	/*
	  Incite Rebellion |4|R|R
	  Sorcery
	  For each player, Incite Rebellion deals damage to that player and each creature that player controls equal to the number of creatures he or she controls.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {new_damage_all(player, card, p, count_subtype(p, TYPE_CREATURE, -1), NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_scrap_mastery(int player, int card, event_t event){
	/*
	  Scrap Mastery |3|R|R
	  Sorcery
	  Each player exiles all artifact cards from his or her graveyard, then sacrifices all artifacts he or she controls,
	  then puts all cards he or she exiled this way onto the battlefield.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int artifact_exiled[2][100];
		int aec[2] = {0, 0};
		APNAP(p,{
					int count = count_graveyard(p)-1;
					while( count > -1 ){
							if( is_what(-1, get_grave(p)[count], TYPE_ARTIFACT) ){
								artifact_exiled[p][aec[p]] = get_grave(p)[count];
								aec[p]++;
								rfg_card_from_grave(p, count);
							}
							count--;
					}

				};
		);
		APNAP(p,{
					if( can_sacrifice(player, p, 1, TYPE_ARTIFACT, 0) ){
						test_definition_t this_test;
						default_test_definition(&this_test, TYPE_ARTIFACT);
						new_manipulate_all(player, card, p, &this_test, KILL_SACRIFICE);
					}

				};
		);
		APNAP(p,{
					int i;
					for(i=0; i<aec[p]; i++){
						if( check_rfg(p, cards_data[artifact_exiled[p][i]].id) ){
							int card_added = add_card_to_hand(p, artifact_exiled[p][i]);
							remove_card_from_rfg(p, cards_data[artifact_exiled[p][i]].id);
							put_into_play(p, card_added);
						}
					}

				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_tyrants_familiar(int player, int card, event_t event){
	/*
	  Tyrant's Familiar |5|R|R
	  Creature - Dragon
	  Flying, haste
	  Lieutenant - As long as you control your commander, Tyrant's Familiar gets +2/+2 and has "Whenever Tyrant's Familiar attacks, it deals 7 damage to target creature defending player controls."
	  5/5
	*/
	haste(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) && liutenant(player) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=2;
		}
		if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
			td.allow_cancel = 0;

			card_instance_t *instance = get_card_instance(player, card);
			instance->number_of_targets = 0;

			if( can_target(&td) && new_pick_target(&td, "Select target creature opponent controls.", 0, GS_LITERAL_PROMPT) ){
				damage_target0(player, card, 7);
			}
		}
	}

	return 0;
}

int card_volcanic_offering(int player, int card, event_t event){
	/*
	  Volcanic Offering |4|R
	  Instant
	  Destroy target nonbasic land you don't control and target nonbasic land of an opponent's choice you don't control.
	  Volcanic Offering deals 7 damage to target creature you don't control and 7 damage to target creature of an opponent's choice you don't control.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_LAND);
					td.allowed_controller = 1-player;
					td.preferred_controller = 1-player;
					td.required_subtype = SUBTYPE_BASIC;
					td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
					td.who_chooses = p;
					td.allow_cancel = 0;

					card_instance_t *instance = get_card_instance(player, card);
					instance->number_of_targets = 0;

					char msg[100];
					if( p == player ){
						scnprintf(msg, 100, "Select target nonbasic land you don't control.");
					}
					else{
						scnprintf(msg, 100, "Select target nonbasic land you control.");
					}

					if( can_target(&td) && new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT) ){
						action_on_target(player, card, 0, KILL_DESTROY);
					}
				};
		);

		APNAP(p,{
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.allowed_controller = 1-player;
					td.preferred_controller = 1-player;
					td.who_chooses = p;
					td.allow_cancel = 0;

					card_instance_t *instance = get_card_instance(player, card);
					instance->number_of_targets = 0;

					char msg[100];
					if( p == player ){
						scnprintf(msg, 100, "Select target creature you don't control.");
					}
					else{
						scnprintf(msg, 100, "Select target creature you control.");
					}

					if( can_target(&td) && new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT) ){
						damage_target0(player, card, 7);
					}
				};
		);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_warmonge_hellkite(int player, int card, event_t event){
	/*
	  Warmonger Hellkite |4|R|R
	  Creature - Dragon
	  Flying
	  All creatures attack each combat if able.
	  1R: Attacking creatures get +1/+0 until end of turn.
	  5/5
	*/

	card_instance_t *instance = get_card_instance(player, card);

	all_must_attack_if_able(current_turn, event, -1);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		pump_creatures_until_eot(instance->parent_controller, instance->parent_card, current_turn, 0, 1, 0, 0, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XR(1, 1), 0, NULL, NULL);
}


// Green (11)
int card_creeperhulk(int player, int card, event_t event){
	/*
	  Creeperhulk |3|G|G
	  Creature - Plant Elemental
	  Trample
	  1G: Target creature you control has base power and toughness 5/5 until end of turn and gains trample until end of turn.
	  5/5
	*/

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											5, 5, KEYWORD_TRAMPLE, 0, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XG(1, 1), 0, &td, "Select target creature you control.");
}

int card_elf_druid(int player, int card, event_t event){
	if( get_special_infos(player, card) == 66 && (IS_GAA_EVENT(event) || (event == EVENT_COUNT_MANA && affect_me(player, card))) ){
		return card_elvish_mystic(player, card, event);
	}
	return generic_token(player, card, event);
}

int card_freyalise_llanowars_fury(int player, int card, event_t event){
	/*
	  Freyalise, Llanowar's Fury |3|G|G
	  Planeswalker - Freyalise
	  +2: Put a 1/1 green Elf Druid creature token onto the battlefield with "T: add G to your mana pool."
	  -2: Destroy target artifact or enchantment.
	  -6: Draw a card for each green creature you control.
	  Freyalise, Llanowar's Fury can be your commander.
	  3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

		int priority_draw = 0;
		if( event == EVENT_ACTIVATE ){
			priority_draw = ((hand_count[player]-7)*2) + (check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) * 2) +
							((count_counters(player, card, COUNTER_LOYALTY)-6)*2);
		}
		enum{
			CHOICE_DRUID = 1,
			CHOICE_DISENCHANT,
			CHOICE_DRAW
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Llanowar Elf", 1, 5, 2,
						"Disenchant", can_target(&td), 10, -2,
						"Draw cards", 1, priority_draw, -6);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			if( choice == CHOICE_DISENCHANT){
				pick_target(&td, "DISENCHANT");
			}
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_DRUID:
			{
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_ELF_DRUID, &token);
				token.pow = token.tou = 1;
				token.color_forced = COLOR_TEST_GREEN;
				token.special_infos = 66;
				generate_token(&token);
			}
			break;

			case CHOICE_DISENCHANT:
				{
					if( valid_target(&td) ){
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
					}
				}
				break;

			case CHOICE_DRAW:
				draw_cards(player, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test));
				break;
		  }
	}

	return planeswalker(player, card, event, 3);
}

int card_grave_sifter(int player, int card, event_t event){
	/* Grave Sifter	|5|G
	 * Creature - Elemental Beast 5/7
	 * When ~ enters the battlefield, each player chooses a creature type and returns any number of cards of that type from his or her graveyard to his or her hand. */

	if( comes_into_play(player, card, event) ){
		APNAP(p,{
				int max = count_graveyard_by_type(p, TYPE_CREATURE);
				if( max ){
					int sub = select_a_subtype_full_choice(player, card, p, get_grave(p), 0);
					int added[max];
					int ac = 0;

					test_definition_t test;
					new_default_test_definition(&test, TYPE_CREATURE, get_subtype_text("Select %a creature card.", sub));
					test.subtype = sub;

					while( max  ){
							int selected = new_select_a_card(p, p, TUTOR_FROM_GRAVE, 0, AI_FIRST_FOUND, -1, &test);
							if( selected != -1 ){
								added[ac] = get_grave(p)[selected];
								remove_card_from_grave(p, selected);
								ac++;
								max--;
							}
							else{
								break;
							}
					}

					int k;
					for(k=0; k<ac; k++){
						int card_added = add_card_to_hand(p, added[k]);
						from_grave_to_hand_triggers(p, card_added);
					}
				}
			}
		);
	}

	return 0;
}

int card_lifeblood_hydra(int player, int card, event_t event){
	/*
	  Lifeblood Hydra |X|G|G|G
	  Creature - Hydra
	  Trample
	  Lifeblood Hydra enters the battlefield with X +1/+1 counters on it.
	  When Lifeblood Hydra dies, you gain life and draw cards equal to its power.
	  0/0
	*/
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->info_slot = x_value;
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = count_counters(player, card, COUNTER_P1_P1);
		gain_life(player,	amount);
		draw_cards(player, amount);
	}

	return 0;
}

static int siege_behemoth_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	card_instance_t* damage = combat_damage_being_prevented(event);

	if( damage && damage->damage_source_card == instance->damage_target_card && damage->damage_source_player == instance->damage_target_player &&
		damage->damage_target_card != -1 && (damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
	  ){
		damage->damage_target_player = 1-player;
		damage->damage_target_card = -1;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_siege_behemoth(int player, int card, event_t event){
	/*
	  Siege Behemoth |5|G|G
	  Creature - Beast
	  Hexproof
	  As long as Siege Behemoth is attacking, for each creature you control, you may have that creature assign combat damage as though it were not blocked.
	  7/4
	*/
	hexproof(player, card, event);

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && ! is_humiliated(player, card) ){
		int c;
		for(c=active_cards_count[player]-1; c>-1; c--){
			if( in_play(player, c) && is_attacking(player, c) && ! is_unblocked(player, c) ){
				if( do_dialog(player, player, c, -1, -1, " Deal damage normally\n Ignore blocker", 1) == 1 ){
					create_targetted_legacy_effect(player, card, &siege_behemoth_legacy, player, c);
				}
			}
		}
	}

  return 0;
}

int card_song_of_the_dryads(int player, int card, event_t event){
	/*
	  Song of the Dryads |2|G
	  Enchantment - Aura
	  Enchant permanent
	  Enchanted permanent is a colorless Forest land.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
			int lt = get_hacked_subtype(player, card, SUBTYPE_FOREST);
			instance->dummy3 = 0; //Hardcoded internal card id for basic Swamp
			if( lt == SUBTYPE_ISLAND ){
				instance->dummy3 = 1; //Hardcoded internal card id for basic Island
			}
			if( lt == SUBTYPE_FOREST ){
				instance->dummy3 = 2; //Hardcoded internal card id for basic Forest
			}
			if( lt == SUBTYPE_MOUNTAIN ){
				instance->dummy3 = 3; //Hardcoded internal card id for basic Mountain
			}
			if( lt == SUBTYPE_PLAINS ){
				instance->dummy3 = 4; //Hardcoded internal card id for basic Plains
			}
			event_result = instance->dummy3;
		}

		if( leaves_play(player, card, event) ){
			humiliate(player, card, p, c, 0);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		humiliate(player, card, instance->targets[0].player, instance->targets[0].card, 1);
	}

	return targeted_aura(player, card, event, &td, "TARGET_PERMANENT");
}

// Treefolk --> Rhino

int card_sylvan_offering(int player, int card, event_t event){
	/*
	  Sylvan Offering |X|G
	  Sorcery
	  Choose an opponent. You and that player each put an X/X green Treefolk creature token onto the battlefield.
	  Choose an opponent. You and that player each put X 1/1 green Elf Warrior creature tokens onto the battlefield.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		APNAP(p,{
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_TREEFOLK, &token);
				token.t_player = p;
				token.pow = token.tou = instance->info_slot;
				token.color_forced = COLOR_TEST_GREEN;
				generate_token(&token);

				default_token_definition(player, card, CARD_ID_ELF_WARRIOR, &token);
				token.t_player = p;
				token.pow = token.tou = 1;
				token.qty = instance->info_slot;
				token.color_forced = COLOR_TEST_GREEN;
				generate_token(&token);
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_thunderfoot_baloth(int player, int card, event_t event){
	/*
	  Thunderfoot Baloth |4|G|G
	  Creature - Beast
	  Trample
	  Lieutenant - As long as you control your commander, Thunderfoot Baloth gets +2/+2 and other creatures you control get +2/+2 and have trample.
	  5/5
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) && liutenant(player) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player ){
			event_result+=2;
		}
		if( event == EVENT_ABILITIES && affected_card_controller == player && affected_card != card &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_titania_protector_of_argoth(int player, int card, event_t event){
	/*
	  Titania, Protector of Argoth |3|G|G
	  Legendary Creature - Elemental
	  When Titania, Protector of Argoth enters the battlefield, return target land card from your graveyard to the battlefield.
	  Whenever a land you control is put into a graveyard from the battlefield, put a 5/3 green Elemental creature token onto the battlefield.
	  5/3
	*/
	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		if( count_graveyard_by_type(player, TYPE_LAND) && ! graveyard_has_shroud(player) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
		}
	}

	count_for_gfp_ability(player, card, event, player, TYPE_LAND, NULL);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *instance = get_card_instance(player, card);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 5;
		token.tou = 3;
		token.qty = instance->targets[11].card;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);

		instance->targets[11].card = 0;
	}

	return 0;
}

int card_wave_of_vitriol(int player, int card, event_t event){
	/*
	  Wave of Vitriol |5|G|G
	  Sorcery
	  Each player sacrifices all artifacts, enchantments, and nonbasic lands he or she controls.
	  For each land sacrificed this way, its controller may search his or her library for a basic land card and put it onto the battlefield tapped.
	  Then each player who searched his or her library this way shuffles it.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int land_sacrificed[2] = {0, 0};
		APNAP(p,{
					if( can_sacrifice(player, p, 1, TYPE_PERMANENT, 0) ){
						int count = active_cards_count[p]-1;
						while( count > -1 ){
								if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) ){
									int kill_me = 0;
									if( is_what(p, count, TYPE_ARTIFACT | TYPE_ENCHANTMENT) ){
										kill_me = 1;
									}
									if( is_what(p, count, TYPE_LAND) && ! has_subtype(p, count, SUBTYPE_BASIC) ){
										kill_me = 1;
										land_sacrificed[p]++;
									}
									if( kill_me ){
										kill_card(p, count, KILL_SACRIFICE);
									}
								}
								count--;
						}
					}
				};
		);
		APNAP(p,{
					if( do_dialog(p, player, card, -1, -1, " Tutor basic lands\n Pass", 0)  == 0 ){
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
						this_test.subtype = SUBTYPE_BASIC;
						this_test.qty = land_sacrificed[p];
						new_global_tutor(p, p, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
						shuffle(p);
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_wolfcallers_howl(int player, int card, event_t event){
	/*
	  Wolfcaller's Howl |3|G
	  Enchantment
	  At the beginning of your upkeep, put X 2/2 green Wolf creature tokens onto the battlefield,
	  where X is the number of your opponents with four or more cards in hand.
	*/
	upkeep_trigger_ability_mode(player, card, event, player, hand_count[1-player] > 3 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WOLF, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

// Lands (3)
static int arcane_lighthouse_legacy(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == 1-player ){
		player_bits[1-player] |= PB_CANT_HAVE_OR_GAIN_SHROUD;
		player_bits[1-player] |= PB_CANT_HAVE_OR_GAIN_HEXPROOF;

		if (!is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			return 0;
		}

		event_result &= ~KEYWORD_SHROUD;

		card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
		if (aff->targets[16].card < 0){
			aff->targets[16].card = 0;
		}
		aff->targets[16].card &= ~SP_KEYWORD_HEXPROOF;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_arcane_lighthouse(int player, int card, event_t event){
	/*
	  Arcane Lighthouse
	  Land
	  T: Add 1 to your mana pool.
	  1, T: Until end of turn, creatures your opponents control lose hexproof and shroud and can't have hexproof or shroud.
	*/

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Hexproof & Shroud removal\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
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
			create_legacy_effect(instance->parent_controller, instance->parent_card, arcane_lighthouse_legacy);
		}
	}

	return 0;
}

int card_flamekin_village(int player, int card, event_t event){
	/*
	  Flamekin Village
	  Land
	  As Flamekin Village enters the battlefield, you may reveal an Elemental card from your hand. If you don't, Flamekin Village enters the battlefield tapped.
	  T: Add R to your mana pool.
	  R,T:Target creature gains haste until end of turn.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_ELEMENTAL);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(2), 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Give haste\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_R(1)) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 && valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return 0;
}

int card_myriad_landscape(int player, int card, event_t event){
	/*
	  Myriad Landscape
	  Land
	  Myriad Landscape enters the battlefield tapped.
	  T: Add 1 to your mana pool.
	  2, T, Sacrifice Myriad Landscape: Search your library for up to two basic land cards that share a land type, put them onto the battlefield tapped,
	  then shuffle your library.
	*/

	comes_into_play_tapped(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer_tapped(player, card, event);
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(3), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Tutor lands\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				tap_card(player, card);
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			int lands[2] = {-1, -1};

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land.");
			this_test.no_shuffle = 1;
			this_test.subtype = SUBTYPE_BASIC;

			int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_FIRST_FOUND, -1, &this_test);
			if( selected != -1 ){
				lands[0] = deck_ptr[player][selected];
				remove_card_from_deck(player, selected);

				int subtype = SUBTYPE_SWAMP;
				if( cards_data[lands[0]].id == CARD_ID_ISLAND ){
					subtype = SUBTYPE_ISLAND;
				}
				if( cards_data[lands[0]].id == CARD_ID_FOREST ){
					subtype = SUBTYPE_FOREST;
				}
				if( cards_data[lands[0]].id == CARD_ID_MOUNTAIN ){
					subtype = SUBTYPE_MOUNTAIN;
				}
				if( cards_data[lands[0]].id == CARD_ID_PLAINS ){
					subtype = SUBTYPE_PLAINS;
				}
				new_default_test_definition(&this_test, TYPE_LAND, get_subtype_text("Select a basic %s.", subtype));
				this_test.subtype = SUBTYPE_BASIC;
				this_test.sub2 = subtype;
				this_test.no_shuffle = 1;
				this_test.subtype_flag = F2_MULTISUBTYPE_ALL;
				selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_FIRST_FOUND, -1, &this_test);
				if( selected != -1 ){
					lands[1] = deck_ptr[player][selected];
					remove_card_from_deck(player, selected);
				}
			}

			shuffle(player);

			int i;
			for(i=0; i<2; i++){
				if( lands[i] != -1 ){
					int card_added = add_card_to_hand(player, lands[i]);
					add_state(player, card_added, STATE_TAPPED);
					put_into_play(player, card_added);
				}
			}

		}
	}

	return 0;
}

// Artifacts (6)
int card_assault_suit(int player, int card, event_t event){
	/*
	  Assault Suit |4
	  Artifact - Equipment
	  Equipped creature gets +2/+2, has haste, can't attack you or a planeswalker you control, and can't be sacrificed.
	  At the beginning of each opponent's upkeep, you may have that player gain control of equipped creature until end of turn. If you do, untap it.
	  Equip 3
	*/
	if( is_equipping(player, card) ){

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_ATTACK_LEGALITY && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			if( current_turn != player ){
				return 1;
			}
		}

		if( leaves_play(player, card, event) ){
			remove_special_flags2(instance->targets[8].player, instance->targets[8].card, SF2_CANNOT_BE_SACRIFICED);
		}

		upkeep_trigger_ability_mode(player, card, event, 1-player, player != AI ? RESOLVE_TRIGGER_OPTIONAL : 0);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			untap_card(instance->targets[8].player, instance->targets[8].card);
			give_control_of_self(instance->targets[8].player, instance->targets[8].card);
		}
	}

	return vanilla_equipment(player, card, event, 3, 2, 2, 0, SP_KEYWORD_HASTE);
}

int card_commanders_sphere(int player, int card, event_t event){
	/*
	  Commander's Sphere |3
	  Artifact
	  T: Add to your mana pool one mana of any color in your commander's color identity.
	  Sacrifice Commander's Sphere: Draw a card.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
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
	}


	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect_force_color(player, card, instance->info_slot);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && instance->info_slot > 0 ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( instance->info_slot > 0 && mana_producer(player, card, event) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}


	if( event == EVENT_ACTIVATE ){
		int choice = instance->targets[1].player = 0;
		if( instance->info_slot > 0 && mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ){
				int ai_choice = hand_count[player] < 3 && count_subtype(player, TYPE_LAND, -1) > 5 ? 1 : 0;
				choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Sac & draw\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->targets[1].player = 1;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player > 0 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_crown_of_doom(int player, int card, event_t event)
{
  /*
	Crown of Doom |3
	Artifact
	Whenever a creature attacks you or a planeswalker you control, it gets +2/+0 until end of turn.
	2: Target player other than Crown of Doom's owner gains control of it. Activate this ability only during your turn.
  */
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 1-player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  pump_until_eot(player, card, current_turn, attackers[amt], 2, 0);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		give_control_of_self(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(2), 0, &td, NULL);
}

int card_loreseekers_stone(int player, int card, event_t event)
{
	/*
	  Loreseeker's Stone |6
	  Artifact
	  3,T: Draw three cards. This ability costs 1 more to activate for each card in your hand.
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 3);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3+hand_count[player]), 0, NULL, NULL);
}

int card_masterwork_of_ingenuity(int player, int card, event_t event)
{
	/*
	  Masterwork of Ingenuity |1
	  Artifact - Equipment
	  You may have Masterwork of Ingenuity enter the battlefield as a copy of any Equipment on the battlefield.
	*/
	target_definition_t td;
	if (event == EVENT_RESOLVE_SPELL){
		base_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		if( player == AI ){
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
			if( ! can_target(&td) ){
				td.allowed_controller = player;
				td.preferred_controller = player;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		base_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
		int tf = can_target(&td);
		if( ! tf ){
			td.allowed_controller = player;
			td.preferred_controller = player;
			tf = can_target(&td);
		}
		if( ! tf ){
			ai_modifier-=35;
		}
	}

	return enters_the_battlefield_as_copy_of_any(player, card, event, &td, "TARGET_EQUIPMENT");
}

int card_unstable_obelisk(int player, int card, event_t event){
	/*
	  Unstable Obelisk |3
	  Artifact
	  T: Add 1 to your mana pool.
	  7, T, Sacrifice Unstable Obelisk: Destroy target permanent.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	if( event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_X(8), 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Kill a permanent\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(7)) && pick_target(&td, "TARGET_PERMANENT") ){
				tap_card(player, card);
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

// Reprints
int card_armistice(int player, int card, event_t event){
	/*
	  Armistice |2|W (Mercadian Masques)
	  Enchantment
	  {3}{W}{W}: You draw a card and target opponent gains 3 life.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		draw_cards(player, 1);
		gain_life(instance->targets[0].player, 3);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_XW(3, 2), 0, &td, NULL);
}

int card_annihilate(int player, int card, event_t event){
	/*
	  Annihilate |3|B|B (Invasion)
	  Instant
	  Destroy target nonblack creature. It can't be regenerated.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target target nonblack creature.", 1, NULL);
}

int card_syphon_mind(int player, int card, event_t event){
	/*
	  Syphon Mind |3|B (Onslaught)
	  Sorcery
	  Each other player discards a card. You draw a card for each card discarded this way.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int prev_hand = hand_count[1-player];
		discard(1-player, 0, player);
		int amount = prev_hand - hand_count[1-player];
		if( amount ){
			draw_cards(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int fools_demise_legacy(int legacy_p, int legacy_c, event_t event){

	card_instance_t* legacy_instance = get_card_instance(legacy_p, legacy_c);

	if( resolve_graveyard_trigger(legacy_p, legacy_c, event) ){
		int owner = legacy_instance->targets[11].player &= ~2;
		seek_grave_for_id_to_reanimate(legacy_p, legacy_c, owner, legacy_instance->targets[11].card, REANIMATE_DEFAULT);
		kill_card(legacy_p, legacy_c, KILL_REMOVE);
	}

	return 0;
}

int card_fools_demise(int player, int card, event_t event){
	/*
	  Fool's Demise |4|U (Time Spiral)
	  Enchantment - Aura
	  Enchant creature
	  When enchanted creature dies, return that card to the battlefield under your control.
	  When Fool's Demise is put into a graveyard from the battlefield, return Fool's Demise to its owner's hand.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	immortal_enchantment(player, card, event);

	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affect_me(instance->damage_target_player, instance->damage_target_card) ){
				card_instance_t *affected = get_card_instance(instance->damage_target_player, instance->damage_target_card);
				if( ! is_token(instance->damage_target_player, instance->damage_target_card) && affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
					int legacy = create_legacy_effect(player, card, &fools_demise_legacy);
					card_instance_t *legacy_instance = get_card_instance(player, legacy);
					legacy_instance->targets[11].player = 2 | get_owner(instance->damage_target_player, instance->damage_target_card);
					legacy_instance->targets[11].card = cards_data[get_original_internal_card_id(instance->damage_target_player, instance->damage_target_card)].id;
				}
			}
		}
	}

	return vanilla_aura(player, card, event, player);
}

int card_zoetic_cavern(int player, int card, event_t event){
	/*
	  Zoetic Cavern (Future Sight)
	  Land
	  {T}: Add {1} to your mana pool.
	  Morph {2} (You may cast this card face down as a 2/2 creature for {3}. Turn it face up any time for its morph cost.)
	*/
	if( IS_GAA_EVENT(event) || (event == EVENT_COUNT_MANA && affect_me(player, card)) ){
		return mana_producer(player, card, event);
	}
	return morph(player, card, event, MANACOST_X(2));
}

// Skyhunter Skirmisher --> vanilla
