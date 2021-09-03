#include "manalink.h"

// Global functions

static int outlast(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_SORCERY_BE_PLAYED, cless, black, blue, green, red, white, 0, NULL, NULL);
}

int prowess_mode(int player, int card, event_t event, int mode){
	return specific_spell_played(player, card, event, player, mode, TYPE_CREATURE, DOESNT_MATCH, 0, 0, 0, 0, 0, 0, -1, 0);
}

int prowess(int player, int card, event_t event){
	// Prowess (Whenever you cast a noncreature spell, [do something].
	if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		pump_until_eot_merge_previous(player, card, player, card, 1, 1);
		return 1;
	}
	return 0;
}

static int raid(int player, int card){
	//Raid - If you attacked with a creature this turn, [do something].
	return get_trap_condition(current_turn, TRAP_RAID);
}

static int raid_cip(int player, int card, event_t event){
	//Raid - When [this] enters the battlefield, if you attacked with a creature this turn, [do something].
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( raid(player, card) ){
			return comes_into_play(player, card, event);
		}
	}
	return 0;
}

int ferocious(int player, int card){
	//Ferocious - if you control a creature with power 4 or greater, [do something].
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.power = 3;
	this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
	return check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
}

static int turn_face_up_revealing_card(int player, int card, event_t event, int color_test){

	if( event == EVENT_CAN_UNMORPH ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.color = get_sleighted_color_test(player, card, color_test);
		this_test.zone = TARGET_ZONE_HAND;
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}
	else if( event == EVENT_UNMORPH ){
			int clr = COLOR_BLACK;
			switch (color_test){
					// The common cases - exactly one color and nothing else
					case COLOR_TEST_BLUE:	clr = COLOR_BLUE;	break;
					case COLOR_TEST_GREEN:	clr = COLOR_GREEN;	break;
					case COLOR_TEST_RED:	clr = COLOR_RED;		break;
					case COLOR_TEST_WHITE:	clr = COLOR_WHITE;	break;
					default: break;
			}
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card to reveal.", clr));
			this_test.color = get_sleighted_color_test(player, card, color_test);
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				reveal_card(player, card, player, selected);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else{
		return morph(player, card, event, MANACOST0);
	}
	return 0;
}

static void boost_creatures_by_counters(int player, int card, event_t event, int counter_type, int amount, int pow, int tou, int key, int s_key, bct_t flags ){
	if( ! in_play(player, card) || is_humiliated(player, card) ){
		return;
	}
	if( ! is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		return;
	}
	if( (flags & BCT_CONTROLLER_ONLY) && affected_card_controller != player ){
		return;
	}
	if( !(flags & BCT_INCLUDE_SELF) && (affected_card_controller == player && affected_card == card) ){
		return;
	}
	if( count_counters(affected_card_controller, affected_card, counter_type) < amount ){
		return;
	}
	if( event == EVENT_POWER ){
		event_result+=pow;
	}
	if( event == EVENT_TOUGHNESS ){
		event_result+=tou;
	}
	if( event == EVENT_ABILITIES ){
		event_result |= key;
		special_abilities(affected_card_controller, affected_card, event, s_key, player, card);
	}
}

const char* is_face_down_creature(int who_chooses, int player, int card)
{
	if(	get_id(player, card) == CARD_ID_FACE_DOWN_CREATURE){
		return NULL;
	}

	return "must be a face-down creature.";
}


// Cards

// White
int card_abzan_battle_priest(int player, int card, event_t event){
	/*
	  Abzan Battle Priest |3|W
	  Creature - Human Cleric
	  Outlast W (W, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control with a +1/+1 counter on it has lifelink.
	  3/2
	*/
	boost_creatures_by_counters(player, card, event, COUNTER_P1_P1, 1, 0, 0, 0, SP_KEYWORD_LIFELINK, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return outlast(player, card, event, MANACOST_W(1));
}

int card_abzan_falconer(int player, int card, event_t event){
	/*
	  Abzan Falconer |2|W
	  Creature - Human Soldier
	  Outlast W (W, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control wth a +1/+1 counter on it has flying.
	  2/3
	*/
	boost_creatures_by_counters(player, card, event, COUNTER_P1_P1, 1, 0, 0, KEYWORD_FLYING, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return outlast(player, card, event, MANACOST_W(1));
}

int card_ainok_bon_kin(int player, int card, event_t event){
	/*
	  Ainok Bond-Kin |1|W
	  Creature - Hound Soldier
	  Outlast 1W (1W, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control with a +1/+1 counter on it has first strike.
	*/
	if( event == EVENT_ABILITIES && player == affected_card_controller && ! is_humiliated(player, card) ){
		if( count_1_1_counters(affected_card_controller, affected_card) > 0 ){
			event_result |= KEYWORD_FIRST_STRIKE;
		}
	}

	return outlast(player, card, event, MANACOST_XW(1, 1));
}

/*
Alabaster Kirin |3|W --> Serra Angel
Creature - Kirin
Flying, vigilance
2/3
*/

int card_brave_the_sands(int player, int card, event_t event){
	/*
	  Brave the Sands |1|W
	  Enchantment
	  Creatures you control have vigilance.
	  Each creature you control can block an additional creature.
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller , affected_card, TYPE_CREATURE) ){
			event_flags |= EA_SELECT_BLOCK;
			creature_can_block_additional(affected_card_controller , affected_card, event, 1);
			special_abilities(affected_card_controller , affected_card, event, SP_KEYWORD_VIGILANCE, player, card);
		}
	}
	return global_enchantment(player, card, event);
}

int card_dazzling_ramparts(int player, int card, event_t event){
	/*
	  Dazzling Ramparts |4|W
	  Creature - Wall
	  Defender
	  1W, T: Tap target creature.
	  0/7
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_defiant_strike(int player, int card, event_t event){
	/*
	  Defiant Strike |W
	  Instant
	  Target creature gets +1/+0 until end of turn.
	  Draw a card.
	*/
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 0, VANILLA_PUMP_DRAW_A_CARD, 0);
}

static void destroy_all_permanents_attached_to_me(int player, int card){
	APNAP(p, {
				int c = active_cards_count[p]-1;
				while( c > -1 ){
						if( in_play(p, c) && is_what(p, c, TYPE_PERMANENT) ){
							card_instance_t *instance = get_card_instance(p, c);
							if( instance->damage_target_player == player && instance->damage_target_card == card ){
								kill_card(p, c, KILL_DESTROY);
							}
						}
						c--;
				}
			};
	);
}

int card_end_hostilities(int player, int card, event_t event){
	/*
	  End Hostilities |3|W|W
	  Sorcery
	  Destroy all creatures and all permanents attached to creatures.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {
					int c = active_cards_count[p]-1;
					while( c > -1 ){
							if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
								destroy_all_permanents_attached_to_me(p, c);
								kill_card(p, c, KILL_DESTROY);
							}
							c--;
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_feat_of_resistance(int player, int card, event_t event){
	/*
	  Feat of Resistance |1|W
	  Instant
	  Put an +1/+1 counter on target creature you control. It gains protection from the color of your choice until end of turn.
	*/
	if(event == EVENT_CHECK_PUMP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;
		if( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) && can_target(&td) ){
			pumpable_power[player]++;
			pumpable_toughness[player]++;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, select_a_protection(player), 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}

int card_firehoof_cavalry(int player, int card, event_t event){
	/*
	  Firehoof Cavalry |1|W
	  Creature - Human Berserker
	  3R: Firehoof Cavalry gets +2/+0 and gains trample until end of turn.
	  1/1
	*/
	return generic_shade(player, card, event, 0, MANACOST_XR(3, 1), 2, 0, KEYWORD_TRAMPLE, 0);
}

// Warrior --> Rhino token

int card_herald_of_anafenza(int player, int card, event_t event){
	/*
	  Herald of Anafenza |W
	  Creature - Human Soldier
	  Outlast 2W (2W, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Whenever you activate Herald of Anafenza's outlast ability, put a 1/1 white Warrior creature token onto the battlefield.
	*/
	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return outlast(player, card, event, MANACOST_XW(2, 1));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		add_1_1_counter(instance->parent_controller, instance->parent_card);
		token_generation_t token;
		default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_WARRIOR, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_high_sentinel_of_arashin(int player, int card, event_t event){
	/*
	  High Sentinels of Arashin |3|W
	  Creature - Bird Soldier
	  Flying
	  High Sentinels of Arashin gets +1/+1 for each other creature you control with a +1/+1 counter on it.
	  3W: Put a +1/+1 counter on target creature.
	  3/4
	*/
	if( ! is_humiliated(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && c != card && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) ){
				event_result++;
			}
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XW(3, 1), 0, &td, "TARGET_CREATURE");
}

/*
Jeskai Student |1|W --> Jeskai Windscout
Creature - Human Monk
Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
1/3

Kill Shot |2|W --> Rebuke
Instant
Destroy target attacking creature.

Mardu Hateblade |W --> Moonglove changeling
Creature - Human Warrior
B: Mardu Hateblade gains deathtouch until end of turn.
1/1
*/

int card_mardu_hordechief(int player, int card, event_t event){
	/*
	  Mardu Hordechief |2|W
	  Creature - Human Warrior
	  Raid - When Mardu Hordechief enters the battlefield, if you attacked with a creature this turn, put a 1/1 white Warrior creature token onto the battlefield.
	  2/3
	*/
	if( raid_cip(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WARRIOR, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_master_of_pearls(int player, int card, event_t event){
	/*
	  Master of Pearls |1|W
	  Creature - Human Monk
	  Morph 3WW
	  When Master of Pearls is turned face up, creatures you control get +2/+2 until end of turn.
	  2/2
	*/
	if( event == EVENT_TURNED_FACE_UP ){
		pump_subtype_until_eot(player, card, player, -1, 2, 2, 0, 0);
	}
	return morph(player, card, event, MANACOST_XW(3, 2));
}

int card_rush_of_battle(int player, int card, event_t event){
	/*
	  Rush of Battle |3|W
	  Sorcery
	  Creatures you control get +2/+1 until end of turn. Warrior creatures you control gain lifelink until end of turn.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int c = active_cards_count[player]-1;
		while( c > -1 ){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
					if( has_subtype(player, c, SUBTYPE_WARRIOR) ){
						pump_ability_until_eot(player, card, player, c, 2, 1, 0, SP_KEYWORD_LIFELINK);
					}
					else{
						pump_until_eot(player, card, player, c, 2, 1);
					}
				}
				c--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_sage_eye_harrier(int player, int card, event_t event){
	/*
	  Sage-Eye Harrier |4|W
	  Creature - Bird Warrior
	  Flying
	  Morph 3W
	  1/5
	*/
	return morph(player, card, event, MANACOST_XW(3, 1));
}

int card_salt_road_patrol(int player, int card, event_t event){
	/*
	  Salt Road Patrol |3|W
	  Creature - Human Scout
	  Outlast 1W (1W,T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  2/5
	*/
	return outlast(player, card, event, MANACOST_XW(1, 1));
}

int card_seeker_of_the_way(int player, int card, event_t event){
	/*
	  Seeker of the Way |1|W
	  Creature - Human Warrior
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn)
	  Whenever you cast a noncreature spell, Seeker of the Way gains lifelink until end of turn.
	  2/2
	*/
	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_ability_until_eot(player, card, player, card, 1, 1, 0, SP_KEYWORD_LIFELINK);
	}
	return 0;
}

int card_siegecraft(int player, int card, event_t event){
	/*
	  Siegecraft |3|W
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+4.
	*/
	return generic_aura(player, card, event, player, 2, 4, 0, 0, 0, 0, 0);
}

int card_suspension_field(int player, int card, event_t event){
	/*
	  Suspension Field |1|W
	  Enchantment
	  When Suspension Field enters the battlefield, you may exile target creature with toughness 3 or greater until Suspension Field leaves the battlefield.
	  (That creature returns under its owner's control.)
	*/
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.toughness_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;

		if( can_target(&td) ){
			ai_modifier+=24;
		}
	}

	return_from_oblivion(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.toughness_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( new_pick_target(&td, "Select target creature with toughness 3 or less.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_take_up_arms(int player, int card, event_t event){
	/*
	  Take Up Arms |4|W
	  Instant
	  Put three 1/1 white Warrior creature tokens onto the battlefield.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WARRIOR, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = 3;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_timely_hordemate(int player, int card, event_t event){
	/*
	  Timely Hordemate |3|W
	  Creature - Human Warrior
	  Raid - When Timely Hordemate enters the battlefield, if you attacked this turn, return target creature card with converted mana cost 2 or less from your graveyard to the battlefield.
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		char msg[100] = "Select a creature card with CMC 2 or less.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.cmc = 3;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		if( raid(player, card) && new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) && comes_into_play(player, card, event) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}


/*
Venerable Lammasu |6|W --> vanilla
Creature - Lammasu
Flying
5/4
*/

int card_war_behemoth(int player, int card, event_t event){
	/*
	  War Behemoth |5|W
	  Creature - Beast
	  Morph 4W
	  3/6
	*/
	return morph(player, card, event, MANACOST_XW(4, 1));
}

int card_watcher_of_the_roost(int player, int card, event_t event){
	/*
	  Watcher of the Roost |2|W
	  Creature - Bird Soldier
	  Flying
	  Morph-Reveal a white card from your hand.
	  When Watcher of the Roost is turned face up, you gain 2 life.
	  2/1
	*/
	if( event == EVENT_TURNED_FACE_UP ){
		gain_life(player, 2);
	}

	return turn_face_up_revealing_card(player, card, event, COLOR_TEST_WHITE);
}

int card_wingmate_roc(int player, int card, event_t event){
	/*
	  Wingmate Roc |3|W|W
	  Creature - Bird
	  Flying
	  Raid - When Wingmate Roc enters the battlefield, if you attacked with a creature this turn, puat a 3/4 white Bird creature token with flying onto the battlefield.
	  3/4
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		gain_life(player, count_attackers(player));
	}

	if( raid_cip(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BIRD, &token);
		token.pow = 3;
		token.tou = 4;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

// Blue

int card_blinding_spray(int player, int card, event_t event){
	/* todo
	   Blinding Spray |4|U
	   Instant
	   Creatures your opponents control get -4/-0 until end of turn.
	   Draw a card.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 1-player, -1, -4, 0, 0, 0);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_clever_impersonator(int player, int card, event_t event){
	/*
	  Clever Impersonator |2|U|U
	  Creature - Shapeshifter
	  You have may Clever Impersonator enter the battlefield as a copy of any nonland permanent on the battlefield.
	  0/0
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;

		if( new_pick_target(&td, "TARGET_NONLAND_PERMANENT", 0, 0) ){
			instance->number_of_targets = 0;
			if (instance->targets[0].card != card || instance->targets[0].player != player)	// didn't pick self
				set_legacy_image(player, CARD_ID_CLEVER_IMPERSONATOR, create_targetted_legacy_effect(player, card, empty, player, card));
		}
	}

	enters_the_battlefield_as_copy_of(player, card, event, instance->targets[0].player, instance->targets[0].card);

	return 0;
}

int card_dig_through_time(int player, int card, event_t event){
	/*
	  Dig Through Time |6|U|U
	  Instant
	  Delve
	  Look at the top seven cards of your library. Put two of them into your hand and the rest on the bottom of your library in any order.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int to_look = MIN(count_deck(player), 7);
		if( to_look ){
			int amount = MIN(2, to_look);

			while( amount && to_look ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to add to your hand");
					this_test.create_minideck = to_look;
					this_test.no_shuffle = 1;
					new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
					amount--;
					to_look--;
			}
			if( to_look-amount > 0 ){
				put_top_x_on_bottom(player, player, to_look-amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return delve(player, card, event);
}

int card_disdainful_stroke(int player, int card, event_t event){
	/*
	  Disdainful Stroke |1|U
	  Instant
	  Counter target spell with converted mana cost 4 or greater.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.extra = 4;
	td.special = TARGET_SPECIAL_CMC_GREATER_OR_EQUAL;

	return counterspell(player, card, event, &td, 0);
}

int card_dragons_eye_savants(int player, int card, event_t event){
	/*
	  Dragon's Eye Savants |1|U
	  Creature - Human Wizard
	  Morph-Reveal a blue card in your hand.
	  When Dragon's Eye Savants is turned face up, look at target opponent's hand.
	  0/6
	*/
	if( event == EVENT_TURNED_FACE_UP ){
		reveal_target_player_hand(1-player);
	}

	return turn_face_up_revealing_card(player, card, event, COLOR_TEST_BLUE);
}

int card_embodiment_of_spring(int player, int card, event_t event){
	/*
	  Embodiment of Spring |U
	  Creature - Elemental
	  1G, T, Sacrifice Embodiment of Spring: Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library.
	  0/3
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_land(player, 1, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_XG(1, 1), 0, NULL, NULL);
}

int card_force_away(int player, int card, event_t event){
	/*
	  Force Away |1|U
	  Instant
	  Return target creature to its owner's hand.
	  Ferocious - If you control a creature with power 4 or greater, you may draw a card. If you do, discard a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			if( ferocious(player, card) && do_dialog(player, player, card, -1, -1, " Draw & discard\n Pass", count_deck(player) <= 10) == 0 ){
				draw_cards(player, 1);
				discard(player, 0, player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_glacial_stalker(int player, int card, event_t event){
	/*
	  Glacial Stalker |5|U
	  Creature - Elemental
	  Morph 4U
	  4/5
	*/
	return morph(player, card, event, MANACOST_XU(4, 1));
}

int card_icy_blast(int player, int card, event_t event){
	/*
	  Icy Blast |X|U
	  Instant
	  Tap X target creatures.
	  Ferocious - If you control a creature with power 4 or greater, those creatures don't untap during their controllers' next untap steps.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		instance->number_of_targets = 0;
		while( has_mana(player, COLOR_COLORLESS, trgs+1) && can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		if( trgs > 0 ){
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
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

	if(event == EVENT_RESOLVE_SPELL ){
		int fer = ferocious(player, card);
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				if( fer ){
					effect_frost_titan(player, card, instance->targets[i].player, instance->targets[i].card);
				}
				else{
					tap_card(instance->targets[i].player, instance->targets[i].card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jeskai_elder(int player, int card, event_t event){
	/*
	  Jeskai Elder |1|U
	  Creature - Human Monk
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  Whenever Jeskai Elder deals combat damage to a player, you may draw a card. If you do, discard a card.
	  1/2
	*/
	prowess(player, card, event);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER | DDBM_TRIGGER_OPTIONAL) ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return 0;
}

int card_jeskai_windscout(int player, int card, event_t event){
	/*
	  Jeskai Windscout |2|U
	  Creature - Bird Scout
	  Flying
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  2/1
	*/
	prowess(player, card, event);
	return 0;
}

int card_khero_spellsnatcher(int player, int card, event_t event){
	/* todo
	   Kheru Spellsnatcher |3|U
	   Creature - Naga Wizard
	   Morph 4UU
	   When Kheru Spellsnatcher is turned face up, counter target spell. If that spell is countered this way, exile it instead of putting it into its owner's graveyard. You may cast that card without paying its mana cost as long as it remains exiled.
	   3/3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST || event == EVENT_CHANGE_TYPE || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return morph(player, card, event, MANACOST_XU(4, 2));
	}

	if ( event == EVENT_CAN_UNMORPH ){
		if( has_mana_multi(player, MANACOST_XU(4, 2)) ){
			return counterspell(player, card, EVENT_CAN_CAST, NULL, 0);
		}
	}
	if( event == EVENT_UNMORPH ){
		charge_mana_multi(player, MANACOST_XU(4, 2));
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_TURNED_FACE_UP ){
		if( counterspell_validate(player, card, NULL, 0) ){
			int result = manage_counterspell_linked_hacks(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			if( result != KILL_REMOVE ){
				int csvid = cards_data[get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card)].id;
				put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
				rfg_top_card_of_deck(instance->targets[0].player);
				create_may_play_card_from_exile_effect(player, card, instance->targets[0].player, csvid, MPCFE_FOR_FREE);
			}
		}
	}

	return 0;
}

int card_mistfire_weaver(int player, int card, event_t event){
	/*
	  Mistfire Weaver |3|U
	  Creature - Djinn Wizard
	  Flying
	  Morph 2U
	  When Mistfire Weaver is turned face up, target creature you control gains hexproof until end of turn.
	  3/1
	*/
	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target creature you control", 0, GS_LITERAL_PROMPT) ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HEXPROOF);
		}
	}

	return morph(player, card, event, MANACOST_XU(2, 1));
}

int card_monastery_flock(int player, int card, event_t event){
	/*
	  Monastery Flock |2|U
	  Creature - Bird
	  Defender, flying
	  Morph U
	  0/5
	*/
	return morph(player, card, event, MANACOST_U(1));
}

int card_mystic_of_the_hidden_way(int player, int card, event_t event){
	/*
	  Mystic of the Hidden Way |4|U
	  Creature - Human Monk
	  Mystic of the Hidden Way can't be blocked.
	  Morph 2U
	  3/2
	*/
	unblockable(player, card, event);

	return morph(player, card, event, MANACOST_XU(2, 1));
}

int card_pearl_lake_ancient(int player, int card, event_t event){
	/*
	  Pearl Lake Ancient |5|U|U
	  Creature - Leviathan
	  Flash
	  Pearl Lake Ancient can't be countered.
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  Return three lands you control to their owner's hand: Return Pearl Lake Ancient to its owner's hand.
	  6/7
	*/
	card_instance_t *instance = get_card_instance(player, card);

	cannot_be_countered(player, card, event);

	prowess(player, card, event);
	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return count_subtype(player, TYPE_LAND, -1) > 2;
		}
	}

	if( event == EVENT_ACTIVATE ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		int trgs = 0;
		instance->number_of_targets = 0;
		while( trgs < 3 && can_target(&td) ){
				if( new_pick_target(&td, "Select target land your control.", trgs, GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			if( trgs == 3 ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		if( trgs < 3 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		bounce_permanent(instance->parent_controller, instance->parent_card);
	}

	return flash(player, card, event);
}

int card_quiet_contemplation(int player, int card, event_t event){
	/*
	  Quiet Contemplation |2|U
	  Enchantment
	  Whenever you cast a noncreature spell, you may pay 1. If you do, tap target creature an opponent controls and it doesn't untap during its controller's next untap step.
	*/

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		if( has_mana(player, COLOR_COLORLESS, 1) && prowess_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 && new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			instance->number_of_targets = 0;
		}
	}

	return global_enchantment(player, card, event);
}

/*
Riverwheel Aerialists |5|U --> --> Jeskai Windscout
Creature - Djinn Monk
Flying
Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn)
4/5
*/

/*
Scaldkin |3|U
Creature - Elemental --> Crackling Triton
Flying
2R, Sacrifice Scaldkin: Scaldkin deals 2 damage to target creature or player.
2/2
*/

/*
Scion of Glaciers |2|U|U --> Metropolis Sprite
Creature - Elemental
U: Scion of Glaciers gets +1/-1 until end of turn.
2/5
*/

int card_set_adrift(int player, int card, event_t event){
	/*
	  Set Adrift |5|U
	  Sorcery
	  Delve
	  Put target nonland permanent on top of its owner's library.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			int result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
			if( result == 2 ){
				td.allow_cancel = 0;
			}
		}
		new_pick_target(&td, "Select target nonland permanent.", 0, 1 | GS_LITERAL_PROMPT);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int singing_bell_strike_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);

	int aura_player = instance->targets[0].player;
	if( aura_player > -1 ){
		int aura_card = instance->targets[0].card;

		if( leaves_play(aura_player, aura_card, event)){
			kill_card(player, card, KILL_REMOVE);
		}
		card_instance_t *attached = get_card_instance( aura_player, aura_card);
		int enc_player = attached->damage_target_player;
		int enc_card = attached->damage_target_card;

		if( is_what(player, card, TYPE_EFFECT) ){
			if( enc_player != player && ! check_status(player, card, STATUS_INVISIBLE_FX) ){
				add_status(player, card, STATUS_INVISIBLE_FX);
			}

			if( enc_player == player && check_status(player, card, STATUS_INVISIBLE_FX) ){
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			untap_card(enc_player, enc_card);
		}

		return granted_generic_activated_ability(player, card, enc_player, enc_card, event, 0, MANACOST_X(6), 0, NULL, NULL);
	}
	return 0;
}

int card_singing_bell_strike(int player, int card, event_t event){
	/*
	  Singing Bell Strike |1|U
	  Enchantment - Aura
	  Enchant creature
	  When Singing Bell Strike enters the battlefield, tap enchanted creature.
	  Enchanted creature doesn't untap during its controller's untap step.
	  Enchanted creature has "6: Untap this creature."
	*/
	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(instance->targets[0].player, instance->internal_card_id);
		int legacy = create_legacy_activate(instance->targets[0].player, fake, &singing_bell_strike_ability);
		obliterate_card(instance->targets[0].player, fake);
		card_instance_t *leg = get_card_instance(instance->targets[0].player, legacy);
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
	}

	if( comes_into_play(player, card, event) ){
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		if( instance->damage_target_player == player && IS_GAA_EVENT(event) ){
			if( event == EVENT_RESOLVE_ACTIVATION ){
				untap_card(instance->damage_target_player, instance->damage_target_card);
			}
			return granted_generic_activated_ability(player, card, instance->damage_target_player, instance->damage_target_card, event, 0, MANACOST_X(6), 0, NULL, NULL);
		}
	}

	return disabling_aura(player, card, event);
}

int card_stubborn_denial(int player, int card, event_t event){
	/*
	  Stubborn Denial |U
	  Instant
	  Counter target noncreature spell unless its opponent pays 1.
	  Ferocious - If you control a creature with power 4 or greater, counter that spell instead.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.illegal_type = TYPE_CREATURE;

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, &td, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( ferocious(player, card) ){
			return counterspell(player, card, event, &td, 0);
		}
		else{
			counterspell_resolve_unless_pay_x(player, card, &td, 0, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_taigams_scheming(int player, int card, event_t event){
	/*
	  Taigam's Scheming |1|U
	  Sorcery
	  Look at the top five cards of your library. Put any number of them into your graveyard and the rest back on top of your library in any order.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int to_look = MIN(count_deck(player), 5);
		if( to_look ){
			int added[to_look];
			int ac = 0;
			while( to_look ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put into your graveyard");
					this_test.create_minideck = to_look;
					int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						added[ac] = deck_ptr[player][selected];
						remove_card_from_deck(player, selected);
						ac++;
						to_look--;
					}
					else{
						break;
					}
			}
			if( ac ){
				int i;
				for(i=0; i<ac; i++){
					int card_added = add_card_to_hand(player, added[i]);
					put_on_top_of_deck(player, card_added);
				}
				mill(player, ac);
			}
			if( to_look ){
				rearrange_top_x(player, player, to_look);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_thousand_winds(int player, int card, event_t event){
	/*
	  Thousand Winds |4|U|U
	  Creature - Elemental
	  Flying
	  Morph 5UU
	  When Thousand Winds is turned face up, return all other tapped creatures to their owners' hands.
	  5/6
	*/
	if( event == EVENT_TURNED_FACE_UP ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;
		this_test.not_me = 1;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, ACT_BOUNCE);};);
	}

	return morph(player, card, event, MANACOST_XU(5, 2));
}

int card_treasure_cruise(int player, int card, event_t event){
	/*
	  Treasure Cruise |7|U
	  Sorcery
	  Delve
	  Draw three cards.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return delve(player, card, event);
}

int card_waterwhirl(int player, int card, event_t event){
	/*
	  Waterwhirl |4|U|U
	  Instant
	  Return up to two target creatures to their owners' hands.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

/*
Weave Fate |3|U --> Divination
Instant
Draw two cards.
*/

/*
Wetland Sambar |1|U --> vanilla
Creature - Elk
2/1
*/

int card_whirlwind_adept(int player, int card, event_t event){
	/* todo
	   Whirlwind Adept |4|U
	   Creature - Djinn Monk
	   Hexproof
	   Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	   4/2
	*/
	hexproof(player, card, event);
	prowess(player, card, event);
	return 0;
}

// Black


int card_bellowing_saddlebrute(int player, int card, event_t event){
	/*
	  Bellowing Saddlebrute|3|B
	  Creature - Orc Warrior
	  Raid - When Bellowing Saddlebrute enters the battlefield, you lose 4 life unless you attacked with a creature this turn.
	  4/5
	*/

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( ! raid(player, card) && life[player]-4 < 6 ){
			ai_modifier-=35;
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( ! raid(player, card) && ! is_humiliated(player, card) && comes_into_play(player, card, event) ){
			lose_life(player, 4);
		}
	}

	return 0;
}

int card_bitter_revelation(int player, int card, event_t event){
	/*
	  Bitter Revelation |3|B
	  Sorcery
	  Look at the top four cards of your library. Put two of them into your hand and the rest into your graveyard. You lose 2 life.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int to_add = MIN(2, count_deck(player));
		int to_reveal = MIN(4, count_deck(player));

		if( to_reveal ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card.");
			this_test.qty = to_add;
			this_test.create_minideck = to_reveal;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
			int to_put_on_bottom = to_reveal-to_add;
			if( to_put_on_bottom > 0){
				put_top_x_on_bottom(player, player, to_put_on_bottom);
			}
		}
		lose_life(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_bloodsoaked_champion(int player, int card, event_t event){
	/* todo
	   Bloodsoaked Champion |B
	   Creature - Human Warrior
	   Bloodsoaked Champion can't block.
	   Raid - 1B: Return Bloodsoaked Champion from your graveyard to the battlefield. Activate this ability only if you attacked with a creature this turn.
	   2/1
	*/
	cannot_block(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_multi(player, MANACOST_XB(1, 1)) && raid(player, card) ){
			return GA_RETURN_TO_PLAY;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XB(1, 1));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}

int card_dead_drop(int player, int card, event_t event){
	/*
	  Dead Drop |9|B
	  Sorcery
	  Delve
	  Target player sacrifices two creatures.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			if( ! cast_spell_with_delve(player, card) ){
				spell_fizzled = 1;
				return 0;
			}
		}
		pick_target(&td, "TARGET_PLAYER");
	}

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			impose_sacrifice(player, card, get_card_instance(player, card)->targets[0].player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/*
Debilitating Injury |1|B --> Dead Weight
Enchantment - Aura
Enchant creature
Enchanted creature gets -2/-2.
*/

int card_disowned_ancestor(int player, int card, event_t event){
	/*
	  Disowned Ancestor |B
	  Creature - Spirit Warrior
	  Outlast 1B (1B, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  0/4
	*/
	return outlast(player, card, event, MANACOST_XB(1, 1));
}

/*
Dutiful Return |3|B --> March of the Returned
Sorcery
Return up to two target creature cards from your graveyard to your hand.
*/

int card_empty_the_pits(int player, int card, event_t event){
	/*
	  Empty the Pits |X|X|B|B|B|B
	  Instant
	  Delve
	  Put X 2/2 black Zombie creature tokens onto the battlefield tapped.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL) ){
			int req_mana = 2;
			int cmc_mod = true_get_updated_casting_cost(player, card, -1, event, 0);
			if( cmc_mod < 0 ){
				req_mana+=cmc_mod;
			}
			if( has_mana(player, COLOR_COLORLESS, req_mana) || count_graveyard(player) >= req_mana ){
				return 1;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		instance->info_slot = 0;
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			if( count_graveyard(player) > 1 && do_dialog(player, player, card, -1, -1, " Use Delve\n Pass", 0) == 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile for Delve.");
				while( count_graveyard(player) > 1 ){
						if( select_multiple_cards_from_graveyard(player, player, 0, AI_MIN_VALUE, &this_test, 2, &instance->targets[0]) ){
							rfg_card_from_grave(player, validate_target_from_grave(player, card, player, 1));
							rfg_card_from_grave(player, validate_target_from_grave(player, card, player, 0));
							instance->info_slot += 2;
							x_value += 2;
						}
						else{
							break;
						}
				}
			}
			int cmc_mod = true_get_updated_casting_cost(player, card, -1, event, 0);
			if( cmc_mod < 0 ){
				instance->info_slot -= cmc_mod;
				x_value -= cmc_mod;
			}
			if( instance->info_slot == 1 ){
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.qty = instance->info_slot / 2;
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grim_haruspex(int player, int card, event_t event){
	/*
	  Grim Haruspex |2|B
	  Creature - Human Wizard
	  Morph B
	  Whenever another nontoken creature you control dies, draw a card.
	  3/2
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && reason_for_trigger_controller == player &&
		instance->kill_code < KILL_DESTROY
	  ){
		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			draw_cards(player, instance->targets[11].card);
			instance->targets[11].card = 0;
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( instance->targets[11].player > 0 ){
			draw_cards(player, instance->targets[11].player);
		}
	}

	return morph(player, card, event, MANACOST_B(1));
}

/*
Gurmag Swiftwing |1|B --> Raging Goblin
Creature - Bat
Flying, first strike, haste
1/2
*/

int card_kheru_bloodsucker(int player, int card, event_t event){
	/*
	  Kheru Bloodsucker |2|B
	  Creature - Vampire
	  Whenever a creature you control with toughness 4 or greater dies, each opponent loses 2 life and you gain 2 life.
	  2B, Sacrifice another creature: Put a +1/+1 counter on Kheru Bloodsucker.
	  2/2
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.toughness = 3;
		this_test.toughness_flag = F5_TOUGHNESS_GREATER_THAN_VALUE;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = instance->targets[11].card;
		while( amount ){
				lose_life(1-player, 2);
				gain_life(player, 2);
				amount--;
		}
		instance->targets[11].card = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_counter(instance->parent_controller, instance->parent_card, COUNTER_P1_P1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST_XB(2, 1), 0, NULL, NULL);
}

int card_kheru_dreadmaw(int player, int card, event_t event){
	/*
	  Kheru Dreadmaw |4|B
	  Creature - Zombie Crocodile
	  Defender
	  1G, Sacrifice another creature: You gain life equal to the sacrificed creature's toughness.
	  4/4
	*/
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST_XG(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XG(1, 1)) ){
			state_untargettable(player, card, 1);
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK | SAC_RETURN_CHOICE | SAC_AS_COST, &test);
			state_untargettable(player, card, 0);
			if( ! sac ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = get_toughness(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->info_slot);
	}

	return 0;
}

int card_krumar_bond_kin(int player, int card, event_t event){
	/*
	  Krumar Bond-Kin |3|B|B
	  Creature - Orc Warrior
	  Morph 4B
	  5/3
	*/
	return morph(player, card, event, MANACOST_XB(4, 1));
}

int card_mardu_skullhunter(int player, int card, event_t event){
	/*
	  Mardu Skullhunter |1|B
	  Creature - Human Warrior
	  Mardu Skullhunter enters the battlefield tapped.
	  Raid - When Mardu Skullhunter enters the battlefield, if you attacked with a creature this turn, target opponent discards a card.
	  2/1
	*/
	comes_into_play_tapped(player, card, event);

	if( raid_cip(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		if( would_validate_arbitrary_target(&td, 1-player, -1)  ){
			discard(1-player, 0, player);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_mer_ek_nightblade(int player, int card, event_t event){
	/*
	  Mer-Ek Nightblade |3|B
	  Creature - Orc Assassin
	  Outlast B (B, T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control with a +1/+1 counter on it has deathtouch.
	  2/3
	*/
	boost_creatures_by_counters(player, card, event, COUNTER_P1_P1, 1, 0, 0, 0, SP_KEYWORD_DEATHTOUCH, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return outlast(player, card, event, MANACOST_B(1));
}

static int molting_snakeskin_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);

	int aura_player = is_what(player, card, TYPE_EFFECT) ? instance->targets[0].player : player;
	if( aura_player > -1 ){
		int aura_card = is_what(player, card, TYPE_EFFECT) ? instance->targets[0].card : card;

		if( leaves_play(aura_player, aura_card, event) && is_what(player, card, TYPE_EFFECT) ){
			kill_card(player, card, KILL_REMOVE);
		}
		card_instance_t *attached = get_card_instance( aura_player, aura_card);
		int enc_player = attached->damage_target_player;
		int enc_card = attached->damage_target_card;

		if( is_what(player, card, TYPE_EFFECT) ){
			if( enc_player != player && ! check_status(player, card, STATUS_INVISIBLE_FX) ){
				add_status(player, card, STATUS_INVISIBLE_FX);
			}

			if( enc_player == player && check_status(player, card, STATUS_INVISIBLE_FX) ){
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(enc_player, enc_card) ){
			regenerate_target(enc_player, enc_card);
		}

		if( enc_player == player ){
			return granted_generic_activated_ability(player, card, enc_player, enc_card, event, GAA_REGENERATION, MANACOST_XB(2, 1), 0, NULL, NULL);
		}
	}
	return 0;
}

int card_molting_snakeskin(int player, int card, event_t event){
	/*
	  Molting Snakeskin |B
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+0 and has "2B: Regenerate this creature."
	*/
	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(instance->targets[0].player, instance->internal_card_id);
		int legacy = create_legacy_activate(instance->targets[0].player, fake, &molting_snakeskin_ability);
		obliterate_card(instance->targets[0].player, fake);
		card_instance_t *leg = get_card_instance( instance->targets[0].player, legacy);
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( instance->damage_target_player == player && IS_GAA_EVENT(event) ){
			return molting_snakeskin_ability(player, card, event);
		}
	}

	return generic_aura(player, card, event, player, 2, 0, 0, 0, 0, 0, 0);
}

int card_murderous_cut(int player, int card, event_t event){
	/*
	  Murderous Cut |4|B
	  Instant
	  Delve
	  Destroy target creature.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		int result = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
		}
		td.allow_cancel = (result == 2 ? 0 : 1);
		instance->number_of_targets = 0;
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_necropolis_fiend(int player, int card, event_t event){
	/*
	  Necropolis Fiend |7|B|B
	  Creature - Demon
	  Delve
	  Flying
	  X, T, Exile X cards from your graveyard: Target creature gets -X/-X until end of turn.
	  4/5
	*/

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL) ){
			return count_graveyard(player) > 0;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		instance->info_slot = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			this_test.id = cards_data[iid_draw_a_card].id;
			this_test.id_flag = DOESNT_MATCH;
			int gcc = count_graveyard(player);
			int removed[2][gcc];
			int rc = 0;
			int ai_max = get_toughness(instance->targets[0].player, instance->targets[0].card)-
						get_card_instance(instance->targets[0].player, instance->targets[0].card)->damage_on_card;
			while( gcc && (player == HUMAN || (player == AI && instance->info_slot < ai_max)) ){
					int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						removed[0][rc] = selected;
						removed[1][rc] = turn_card_in_grave_face_down(player, selected);
						rc++;
						gcc--;
					}
					else{
						break;
					}
			}
			if( rc > 0 ){
				int k;
				for(k=0; k<rc; k++){
					if( removed[0][k] > removed[0][k+1] ){
						int swap1 = removed[0][k];
						int swap2 = removed[1][k];
						removed[0][k] = removed[0][k+1];
						removed[1][k] = removed[1][k+1];
						removed[0][k+1] = swap1;
						removed[1][k+1] = swap2;
					}
				}
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(rc)) ){
					instance->info_slot = rc;
					rc--;
					while( rc > -1 ){
							turn_card_in_grave_face_up(player, removed[0][rc], removed[1][rc]);
							rfg_card_from_grave(player, removed[0][rc]);
							rc--;
					}
					tap_card(player, card);
				}
				else{
					rc--;
					while( rc > -1 ){
							turn_card_in_grave_face_up(player, removed[0][rc], removed[1][rc]);
							rc--;
					}
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
							-instance->info_slot, -instance->info_slot);
		}
	}

	return delve(player, card, event);
}

int card_raiders_spoil(int player, int card, event_t event){
	/*
	  Raiders' Spoils |3|B
	  Enchantment
	  Creatures you control get +1/+0.
	  Whenever a Warrior you control deals combat damage to a player, you may pay 1 life. If you do, draw a card.
	*/
	boost_creature_type(player, card, event, -1, 1, 0, 0, BCT_CONTROLLER_ONLY);

	if( subtype_deals_damage(player, card, event, player, SUBTYPE_WARRIOR, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		card_instance_t* instance = get_card_instance(player, card);
		int i;
		for(i=0; i<instance->targets[1].card; i++){
			if( can_pay_life(player, 1) && do_dialog(player, player, card, -1, -1, " Pay 1 life and draw\n Pass", (life[player] < 6 || count_deck(player) <= 10)) == 0 ){
				lose_life(player, 1);
				draw_cards(player, 1);
			}
			else{
				break;
			}
		}
		instance->targets[1].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_rakshasas_secret(int player, int card, event_t event){
	/*
	  Rakshasa's Secret |2|B
	  Sorcery
	  Target opponent discards two cards. Put the top two cards of your library into your graveyard.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td) ){
			new_multidiscard(get_card_instance(player, card)->targets[0].player, 2, 0, player);
			mill(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

static const char* has_tou_lesser_than_available_p1_p1_counters(int who_chooses, int player, int card){
	if( (get_toughness(player, card) - get_card_instance(player, card)->damage_on_card) <=
		count_counters_by_counter_and_card_type(player, COUNTER_P1_P1, TYPE_CREATURE)
	 ){
		return NULL;
	}

	return "AI helper function.";
}

int card_retribution_of_the_ancients(int player, int card, event_t event){
	/*
	  Retribution of the Ancients |B
	  Enchantment
	  B, Remove X +1/+1 counters from among creatures you control: Target creature gets -X/-X until end of turn.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.extra = (int32_t)has_tou_lesser_than_available_p1_p1_counters;
	}

	target_definition_t tdc;
	default_target_definition(player, card, &tdc, TYPE_CREATURE);
	tdc.allowed_controller = player;
	tdc.preferred_controller = player;
	tdc.illegal_abilities = 0;
	tdc.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	tdc.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_B(1), 0, &td, NULL) ){
			return can_target(&tdc);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			int max = 0;
			if( player == AI ){
				instance->number_of_targets = 0;
				if( pick_target(&td, "TARGET_CREATURE") ){
					max = get_toughness(instance->targets[0].player, instance->targets[0].card) - get_card_instance(instance->targets[0].player, instance->targets[0].card)->damage_on_card;
				}
			}
			int removed = 0;
			while( can_target(&tdc) && (player == HUMAN || (player == AI && removed < max)) ){
					if( new_pick_target(&tdc, "Select a creature you control with a +1/+1 counter.", 1, GS_LITERAL_PROMPT) ){
						remove_1_1_counter(instance->targets[1].player, instance->targets[1].card);
						removed++;
					}
					else{
						break;
					}
			}
			if( player == HUMAN ){
				if( removed > 0 ){
					td.allow_cancel = 0;
				}
				instance->number_of_targets = 0;
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = removed;
				}
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		if (valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
							-instance->info_slot, -instance->info_slot);
		}
	}

	return 0;
}

int card_rite_of_the_serpent(int player, int card, event_t event){
	/*
	  Rite of the Serpent |4|B|B
	  Sorcery
	  Destroy target creature. If that creature had a +1/+1 counter on it, put a 1/1 green Snake creature token onto the battlefield.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_1_1_counters(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( amount ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_SNAKE, &token);
				token.pow = token.tou = 1;
				token.color_forced = COLOR_TEST_GREEN;
				generate_token(&token);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*
Rotting Mastodon |4|B --> vanilla
Creature - Zombie Elephant
2/8
*/

int card_ruthless_ripper(int player, int card, event_t event){
	/*
	  Ruthless Ripper |B
	  Creature - Human Assassin
	  Deathtouch
	  Morph-Reveal a black card in your hand.
	  When Ruthless Ripper is turned face up, target player loses 2 life.
	  1/1
	*/

	deathtouch(player, card, event);

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			lose_life(get_card_instance(player, card)->targets[0].player, 2);
		}
	}

	return turn_face_up_revealing_card(player, card, event, COLOR_TEST_BLACK);
}


int card_shambling_attendants(int player, int card, event_t event){
	/*
	  Shambling Attendants |7|B
	  Creature - Zombie
	  Delve
	  Deathtouch
	  3/5
	*/
	deathtouch(player, card, event);
	return delve(player, card, event);
}

int card_sidisis_pet(int player, int card, event_t event){
	/* todo
	   Sidisi's Pet |3|B
	   Creature - Zombie Ape
	   Lifelink
	   Morph 1B
	   1/4
	*/
	lifelink(player, card, event);

	return morph(player, card, event, MANACOST_XB(1, 1));
}

/*
Sultai Scavenger |5|B --> Tombstalker
Creature - Bird Warrior
Delve
Flying
3/3
*/

int card_swarm_of_bloodflies(int player, int card, event_t event){
	/* todo
	   Swarm of Bloodflies |4|B
	   Creature - Insect
	   Flying
	   Swarm of Bloodflies enters the battlefield with two +1/+1 counters on it.
	   Whenever another creature dies, put a +1/+1 counter on Swarm of Bloodflies.
	   0/0
	*/
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *instance = get_card_instance(player, card);
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

/*
Throttle |4|B --> Grasp of Darkness
Instant
Target creature gets -4/-4 until end of turn.
*/

int card_unyielding_krumar(int player, int card, event_t event){
	/*
	  Unyielding Krumar |3|B
	  Creature - Orc Warrior
	  1W: Unyielding Krumar gains first strike until end of turn.
	  3/3
	*/
	return generic_shade(player, card, event, 0, MANACOST_XW(1, 1), 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

// Red
int card_ainok_tracker(int player, int card, event_t event){
	/* todo
	   Ainok Tracker |5|R
	   Creature - Hound Scout
	   First strike
	   Morph 4R
	   3/3
	*/
	return morph(player, card, event, MANACOST_XR(4, 1));
}

int card_arrow_storm(int player, int card, event_t event){
	/*
	  Arrow Storm |3|R|R
	  Sorcery
	  Arrow Storm deals 4 damage to target creature or player.
	  Raid - If you attacked with a creature this turn, instead Arrow Storm deals 5 damage to that creature or player and the damage can't be prevented.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4+raid(player, card));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int ashcloud_phoenix_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CHANGE_TYPE && instance->targets[0].player > -1 && affect_me(instance->targets[0].player, instance->targets[0].card) &&
		instance->targets[1].player != 66
	  ){
		if( instance->targets[1].card == -1 ){
			instance->targets[1].card = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);
		}
		event_result = instance->targets[1].card;
	}
	if( comes_into_play(instance->targets[0].player, instance->targets[0].card, event) ){
		turn_face_down(instance->targets[0].player, instance->targets[0].card);
		instance->targets[1].player = 66;
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_ashcloud_phoenix(int player, int card, event_t event){
	/*
	  Ashcloud Phoenix |2|R|R
	  Creature - Phoenix
	  Flying
	  When Ashcloud Phoenix dies, return it to the battlefield face down.
	  Morph 4RR
	  When Ashcloud Phoenix is turned face up, it deals 2 damage to each player.
	  4/1
	*/

	nice_creature_to_sacrifice(player, card);

	if( event == EVENT_TURNED_FACE_UP ){
		APNAP(p, {damage_player(p, 2, player, card);};);
	}

	int owner, position;
	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position) ){
		const int *grave = get_grave(owner);
		int card_added = add_card_to_hand(owner, grave[position]);
		remove_card_from_grave(owner, position);
		int legacy = create_legacy_effect(player, card, &ashcloud_phoenix_legacy);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[0].player = owner;
		instance->targets[0].card = card_added;
		instance->number_of_targets = 1;
		set_special_flags3(owner, card_added, SF3_REANIMATED);
		put_into_play(owner, card_added);
	}

	return morph(player, card, event, MANACOST_XR(4, 2));
}

int card_barrage_of_boulders(int player, int card, event_t event){
	/*
	  Barrage of Boulders |2|R
	  Sorcery
	  Barrage of Boulders deals 1 damage to each creature you don't control.
	  Ferocious - If you control a creature with power 4 or greater, creatures can't block this turn.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int fer = ferocious(player, card);
		int c = active_cards_count[1-player]-1;
		while( c > -1 ){
				if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) ){
					if( fer ){
						pump_ability_until_eot(player, card, 1-player, c, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
					}
					damage_creature(1-player, c, 1, player, card);
				}
				c--;
		}
		if( fer ){
			pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}
/*
Bloodfire Expert |2|R -->  Jeskai Windscout
Creature - Efreet Monk
Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
3/1
*/

int card_bloodfire_mentor(int player, int card, event_t event){
	/*
	  Bloodfire Mentor 2|R
	  Creature - Efreet Shaman
	  2U, T: Draw a card, then discard a card.
	  0/5
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(2, 1), 0, NULL, NULL);
}

int card_bring_low(int player, int card, event_t event){
	/*
	  Bring Low |3|R
	  Instant
	  Bring Low deals 3 damage to target creature. If that creature has a +1/+1 counter on it, Bring Low deals 5 damage to it instead.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			int dmg = count_1_1_counters(instance->targets[0].player, instance->targets[0].card) > 0 ? 5 : 3;
			damage_target0(player, card, dmg);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int burn_away_legacy(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event) ){
			get_card_instance(instance->damage_target_player, instance->damage_target_card)->kill_code = KILL_REMOVE;
			rfg_whole_graveyard(instance->damage_target_player);
		}
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_burn_away(int player, int card, event_t event){
	/*
	  Burn Away |4|R
	  Instant
	  Burn Away deals 6 damage to target creature. When that creature dies this turn, exile all cards from its controller's graveyard.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			create_targetted_legacy_effect(player, card, &burn_away_legacy, instance->targets[0].player, instance->targets[0].card);
			damage_target0(player, card, 6);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_canyon_lurker(int player, int card, event_t event){
	/*
	  Canyon Lurkers |4|R
	  Creature - Human Rogue
	  Morph 3R
	  5/2
	*/
	return morph(player, card, event, MANACOST_XR(3, 1));
}

int card_craters_claw(int player, int card, event_t event){
	/*
	  Crater's Claws |X|R
	  Sorcery
	  Crater's Claws deals X damage to target creature or player.
	  Ferocious - Crater's Claws deals X plus 2 damage to that creature or player instead if you control a creature with power 4 or greater.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int dmg = ferocious(player, card) ? instance->info_slot + 2 : instance->info_slot;
			damage_target0(player, card, dmg);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_dragon_grip(int player, int card, event_t event){
	/*
	  Dragon Grip |2|R
	  Enchantment - Aura
	  Ferocious - If you control a creature with power 4 or greater, you may cast Dragon Grip as though it had flash.
	  Enchant creature
	  Enchanted creature gets +2/+0 and has first strike.
	*/

	if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) && ! ferocious(player, card) ){
		infinite_casting_cost();
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){ //straight from 'flash'
		ai_modifier -= 16;
	}

	return generic_aura(player, card, event, player, 2, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

/*
Dragon-Style Twins |3|R|R --> Jeskai Windscout
Creature - Human Monk
Double strike
Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
*/

int card_goblinslide(int player, int card, event_t event){
	/* todo
	   Goblinslide |2|R
	   Enchantment
	   Whenever you cast a noncreature spell, you may pay 1. If you do, put a 1/1 red Goblin creature token with haste onto the battlefield.
	*/

	if( has_mana(player, COLOR_COLORLESS, 1) && prowess_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN, &token);
			token.color_forced = COLOR_TEST_RED;
			token.pow = token.tou = 1;
			token.s_key_plus = SP_KEYWORD_HASTE;
			generate_token(&token);
		}
	}

	return global_enchantment(player, card, event);
}

int card_horde_ambusher(int player, int card, event_t event){
	/*
	  Horde Ambusher |1|R
	  Creature - Human Berserker
	  Whenever Horde Ambusher blocks, it deals 1 damage to you.
	  Morph-Reveal a red card in your hand.
	  When Horde Ambusher is turned face up, target creature can't block this turn.
	  2/2
	*/
	if( blocking(player, card, event) ){
		damage_player(player, 1, player, card);
	}

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			instance->number_of_targets = 0;
		}
	}

	return turn_face_up_revealing_card(player, card, event, COLOR_TEST_RED);
}

int card_hordeling_outburst(int player, int card, event_t event){
	/*
	  Hordeling Outburst English |1|R|R
	  Sorcery
	  Put three 1/1 red Goblin creature tokens onto the battlefield.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return	generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int howl_of_the_horde_legacy(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		int k;
		for(k=0; k<(ferocious(player, card) ? 2 : 1); k++){
			copy_spell_from_stack(player, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_REMOVE);
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_howl_of_the_horde(int player, int card, event_t event){
	/*
	  Howl of the Horde |2|R
	  Sorcery
	  When you cast your next instant or sorcery spell this turn, copy that spell. You may choose new targets for the copy.
	  Raid - If you attacked with a creature this turn, when you cast your next instant or sorcery spell this turn, copy that spell an additional time. You may choose new targets for the copy.
	*/
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		int amount = count_subtype(player, TYPE_LAND, -1);
		ai_modifier += ((amount * 5)-15);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &howl_of_the_horde_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_mocking_instigator(int player, int card, event_t event){
	// wrong name
	/*
	  Jeering Instigator |1|R
	  Creature - Goblin Rogue
	  Morph 2R
	  When Mocking Instigator is turned face up, if it's your turn, untap target creature and gain control if it until end of turn. That creature gains haste until end of turn.
	  2/1
	*/
	if( event == EVENT_TURNED_FACE_UP && current_turn == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_abilities = KEYWORD_PROT_RED;
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->targets[0].player == player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
			else{
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			instance->number_of_targets = 0;
		}
	}

	return morph(player, card, event, MANACOST_XR(2, 1));
}

int card_leaping_master(int player, int card, event_t event){
	/*
	  Leaping Master |1|R
	  Creature - Human Monk
	  2W: Leaping Master gains flying until end of turn.
	  2/1
	*/
	return generic_shade(player, card, event, 0, MANACOST_XW(2, 1), 0, 0, KEYWORD_FLYING, 0);
}

/*
Mardu Blazebringer |2|R --> Fog Elemental
Creature - Ogre Warrior
When Mardu Blazebringer attacks or blocks, sacrifice it at end of combat.
4/4
*/

int card_mardu_heart_piercer(int player, int card, event_t event){
	/*
	  Mardu Heart-Piercer |3|R
	  Creature - Human Archer
	  Raid - When Mardu Heart-Piercer enters the battlefield, if you attacked with a creature this turn,
	  Mardu Heart-Piercer deals 2 damage to target creature or player.
	  2/3
	*/
	if( raid_cip(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, 2);
			get_card_instance(player, card)->number_of_targets = 0;
		}
	}

	return 0;
}

int card_mardu_warshrieker(int player, int card, event_t event){
	/*
	  Mardu Warshrieker |3|R
	  Creature - Orc Shaman
	  Raid - When Mardu Warshrieker enters the battlefield, if you attacked with a creature this turn, add RWB to your mana pool.
	  3/3
	*/
	if( raid_cip(player, card, event) ){
		produce_mana_multi(player, MANACOST_BRW(1, 1, 1));
	}

	return 0;
}

int card_monastery_swiftspear(int player, int card, event_t event){
	/*
	  Monastery Swiftspear |R
	  Creature - Human Monk
	  Haste
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  1/2
	*/

	haste(player, card, event);
	prowess(player, card, event);
	return 0;
}

int card_sarkhan_the_dragonspeaker(int player, int card, event_t event){
	/*
	  Sarkhan, the Dragonspeaker |3|R|R
	  Planeswalker - Sarkhan
	  +1: Until end of turn, Sarkhan, the Dragonspeaker becomes a legendary 4/4 red Dragon creature with flying, indestructible, and haste. (He doesn't lose loyalty while he's not a planeswalker.)
	  -3: Sarkhan, the Dragonspeaker deals 4 damage to target creature.
	  -6: You get an emblem with "At the beginning of your draw step, draw two additional cards" and "At the beginning of your end step, discard your hand."
	  4
	*/
	double_faced_card(player, card, event);

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		enum{
			CHOICE_ANIMATE = 1,
			CHOICE_DAMAGE,
			CHOICE_EMBLEM
		} choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						  "Turn into a Dragon", 1, (current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS ? 10 : 5), 1,
						  "Damage creature", can_target(&td), (count_counters(player, card, COUNTER_LOYALTY) > 3 ? 8 : 3), -3,
						  "Emblem", 1, (count_counters(player, card, COUNTER_LOYALTY) > 6 ? 15 : 2), -6
						  );

		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			if( choice == CHOICE_DAMAGE ){
				pick_target(&td, "TARGET_CREATURE");
			}
		}
		else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_ANIMATE:
					true_transform(instance->parent_controller, instance->parent_card);
					break;
				case CHOICE_DAMAGE:
				{
					damage_creature(instance->targets[0].player, instance->targets[0].card, 4, instance->parent_controller, instance->parent_card);
					break;
				}
				case CHOICE_EMBLEM:
					generate_token_by_id(player, card, CARD_ID_SARKHANS_DRAGONSPEAKER_EMBLEM);
					break;
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_sarkhan_the_dragonspeaker_animated(int player, int card, event_t event){
	haste(player, card, event);
	indestructible(player, card, event);
	if( eot_trigger(player, card, event) ){
		get_card_instance(player, card)->targets[9].card = 0;
		remove_special_flags(player, card, SF_WONT_RECEIVE_DAMAGE);
		true_transform(player, card);
	}
	return planeswalker(player, card, event, 4);
}

int card_sarkhan_the_dragonspeaker_emblem(int player, int card, event_t event){
	if( current_turn == player && event == EVENT_DRAW_PHASE ){
		event_result+=2;
	}
	if( current_turn == player && eot_trigger(player, card, event) ){
		discard_all(player);
	}
	return 0;
}

/*
Summit Prowler |2|R|R --> vanilla
Creature - Yeti
4/3
*/

int card_swift_kick(int player, int card, event_t event){
	/*
	  Swift Kick |3|R
	  Instant
	  Target creature you control gets +1/+0 until end of turn. It fights target creature you don't control.
	*/

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
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 0);
			if( validate_target(player, card, &td1, 1) ){
				fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tormenting_voice(int player, int card, event_t event){
	/*
	  Tormenting Voice |1|R
	  Sorcery
	  As an additional cost to cast Tormenting Voice, discard a card.
	  Draw two cards.
	*/
	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			return hand_count[player] > 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		discard(player, 0, player);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/*
Valley Dasher |1|R --> Flameborn Hellion
Creature - Human Berserker
Haste
Valley Dasher attacks each turn if able.
*/

int card_war_name_aspirant(int player, int card, event_t event){
	/*
	  War-Name Aspirant |1|R
	  Creature - Human Warrior
	  Raid - War-Name Aspirant enters the battlefield with a +1/+1 counter on it if you attacked with a creature this turn.
	  War-Name Aspirant can't be blocked by creatures with power 1 or less.
	  2/1
	*/
	if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && attacking_card == card && ! is_humiliated(player, card) ){
		if( get_power(affected_card_controller, affected_card) < 2 ){
			event_result = 1;
		}
	}

	if((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		if( raid(player, card) ){
			enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);
		}
	}

	return 0;
}


// Green
/*
Alpine Grizzly |2|G--> vanilla
Creature - Bear
4/2
*/

int card_archers_parapet(int player, int card, event_t event){
	/*
	  Archer's Parapet |1|G
	  Creature - Wall
	  Defender
	  1B, T: Each opponent loses 1 life.
	  0/5
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		lose_life(1-player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XB(1, 1), 0, NULL, NULL);
}

/*
Awaken the Bear |2|G --> Predator's Strike
Instant
Target creature gets +3/+3 and gains trample until end of turn.
*/

int card_become_immense(int player, int card, event_t event){
	/*
	  Become Immense |5|G
	  Instant
	  Delve
	  Target creature gets +6/+6 until end of turn.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			if( ! cast_spell_with_delve(player, card) ){
				spell_fizzled = 1;
				return 0;
			}
		}
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 6, 6);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dragonscale_boon(int player, int card, event_t event){
	/*
	  Dragonscale Boon |3|G
	  Instant
	  Put two +1/+1 counters on target creature and untap it.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 2);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_feed_the_clan(int player, int card, event_t event){
	/*
	  Feed the Clan |1|G
	  Instant
	  You gain 5 life.
	  Ferocious - You gain 10 life instead if you control a creature with power 4 or greater.
	*/

	if(event == EVENT_RESOLVE_SPELL){
		int amount = ferocious(player, card) ? 10 : 5;
		gain_life(player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*
Hardened Scales |G --> Doubling Season
Enchantment
If one or more +1/+1 counters would be placed on a creature you control, that many plus one +1/+1 counters are placed on it instead.
*/

int card_heir_of_wilds(int player, int card, event_t event){
	/*
	  Heir of the Wilds |1|G
	  Creature - Human Warrior
	  Deathtouch
	  Ferocious - Whenever Heir of the Wilds attacks, if you control a creature with power 4 or greater, Heir of the Wilds gets +1/+1 until end of turn.
	  2/2
	*/
	deathtouch(player, card, event);

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) && ferocious(player, card) ){
		pump_until_eot(player, card, player, card, 1, 1);
	}

	return 0;
}

/* todo
Highland Game |1|G --> Onulet
Creature - Elk
When Highland Game dies, you gain 2 life.
2/1
*/

int card_hooded_hydra(int player, int card, event_t event){
	/*
	  Hooded Hydra |X|G|G
	  Creature - Snake Hydra
	  Hooded Hydra enters the battlefield with X +1/+1 counters on it.
	  When Hooded Hydra dies, put a 1/1 green Snake creature token onto the battlefield for each +1/+1 counter on it.
	  Morph 3GG
	  As Hooded Hydra is turned face up, put five +1/+1 counters on it.
	  0/0
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SNAKE, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_GREEN;
		token.qty = count_1_1_counters(player, card);
		generate_token(&token);
	}

	if( event == EVENT_TURNED_FACE_UP ){
		add_1_1_counters(player, card, 5);
	}


	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = 1;
		if( ! is_token(player, card) && instance->info_slot == 1 ){
			result = casting_permanent_with_morph(player, card, event);
		}
		if( result == 1 && ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, x_value);
			}
		}
	}
	else{
		return morph(player, card, event, MANACOST_XG(3, 2));
	}

	return 0;
}

/*
Hooting Mandrills |5|G --> Tombstalker
Creature - Ape
Delve
Trample
4/4
*/

int card_kin_tree_warden(int player, int card, event_t event){
	/* todo
	   Kin-Tree Warden |G
	   Creature - Human Warrior
	   2: Regenerate Kin-Tree Warden.
	   Morph G
	   1/1
	*/
	if( IS_GAA_EVENT(event) ){
		return regeneration(player, card, event, MANACOST_X(2));
	}

	return morph(player, card, event, MANACOST_G(1));
}

int card_longshot_squad(int player, int card, event_t event){
	/*
	  Longshot Squad |3|G
	  Creature - Hound Archer
	  Outlast 1G (1G,T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control with a +1/+1 counter on it has reach.
	  3/3
	*/
	boost_creatures_by_counters(player, card, event, COUNTER_P1_P1, 1, 0, 0, KEYWORD_REACH, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return outlast(player, card, event, MANACOST_XG(1, 1));
}

static int meandering_towershell_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && beginning_of_combat(player, card, event, RESOLVE_TRIGGER_MANDATORY, player) ){
		int iid = instance->targets[0].card;
		if( iid > -1 && check_rfg(player, cards_data[iid].id) ){
			int card_added = add_card_to_hand(player, iid);
			remove_card_from_rfg(player, cards_data[iid].id);
			add_state(player, card, STATE_TAPPED | STATE_ATTACKING);
			put_into_play(player, card_added);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_meandering_towershell(int player, int card, event_t event){
	/*
	  Meandering Towershell |3|G|G
	  Creature - Turtle
	  Islandwalk
	  Whenever Meandering Towershell attacks, exile it. Return it to the battlefield under your control tapped and attacking at the beginning of the next declare attackers step on your next turn.
	  5/9
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		int legacy = create_legacy_effect(player, card, &meandering_towershell_legacy);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[0].card = get_original_internal_card_id(player, card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_pine_walker(int player, int card, event_t event){
	/*
	  Pine Walker |3|G|G
	  Creature - Elemental
	  Morph 4G
	  Whenever Pine Walker or another creature you control is turned face up, untap that creature.
	  5/5
	*/
	// the rest of the effect is in 'flip_card'
	return morph(player, card, event, MANACOST_XG(4, 1));
}

int card_rattleclaw_mystic(int player, int card, event_t event){
	/*
	  Rattleclaw Mystic |1|G
	  Creature - Human Shaman
	  T: Add G, U, or R to your mana pool.
	  Morph 2
	  When Rattleclaw Mystic is turned face up, add GUR to your mana pool.
	  2/1
	*/
	if (IS_ACTIVATING(event) || (event == EVENT_COUNT_MANA && affect_me(player, card))){
		return mana_producing_creature_all_one_color(player, card, event, 0, COLOR_TEST_GREEN|COLOR_TEST_BLUE|COLOR_TEST_RED, 1);
	}

	if( event == EVENT_TURNED_FACE_UP ){
		produce_mana_multi(player, MANACOST_UGR(1, 1, 1));
	}

	return morph(player, card, event, MANACOST_X(2));
}

int card_roar_of_challenge(int player, int card, event_t event){
	/*
	  Roar of Challenge |2|G
	  Sorcery
	  All creatures able to block target creature this turn do so.
	  Ferocious - That creature gains indestructible until end of turn if you control a creature with power 4 or greater.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;


	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card );
		if( valid_target(&td) ){
			int skey = ferocious(player, card) ? SP_KEYWORD_LURE | SP_KEYWORD_INDESTRUCTIBLE : SP_KEYWORD_LURE;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, skey);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*
Sagu Archer |4|G --> Pine Walker
Creature - Naga Archer
Reach
Morph 4G
2/5
*/

int card_savage_punch(int player, int card, event_t event){
	/*
	  Savage Punch |1|G
	  Sorcery
	  Target creature you control fights target creature you don't control.
	  Ferocious - The creature you control gets +2/+2 until end of turn before it fights if you control a creature with power 4 or greater.
	*/

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
		if( validate_target(player, card, &td, 0) ){
			if( ferocious(player, card) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			}
			if( validate_target(player, card, &td1, 1) ){
				fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/*
Scout the Borders |2|G --> Grisly Salvage
Sorcery
Reveal the top five cards of your library. You may put a creature or land card from among them into your hand. Put the rest into your graveyard.
*/

int card_see_the_unwritten(int player, int card, event_t event){
	/*
	  See the Unwritten |4|G|G
	  Sorcery
	  Reveal the top eight cards of your library. You may put a creature card from among them onto the battlefield. Put the rest into your graveyard.
	  Ferocious - If you control a creature with power 4 or greater, you may put two creature cards onto the battlefield instead of one.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int max = MIN(8, count_deck(player));
		if( max > 0 ){
			show_deck( 1-player, deck_ptr[player], max, "See the Unwritten revealed...", 0, 0x7375B0 );

			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			this_test.create_minideck = max;
			this_test.qty = ferocious(player, card) ? 2 : 1;
			this_test.no_shuffle = 1;

			max-= new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);

			if( max > 0 ){
				mill(player, max);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_smoke_teller(int player, int card, event_t event){
	/*
	  Smoke Teller |1|G
	  Creature - Human Shaman
	  1U: Look at target face-down creature.
	  2/2
	*/

	if (!IS_GAA_EVENT(event) || player == AI ){ //Obviously, there's no use for AI
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = (int32_t)is_face_down_creature;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		int true_identity = get_internal_card_id_from_csv_id( get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[12].player );
		reveal_card_iid(player, card, true_identity);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(1, 1), 0, &td, "Select target face-down creature");
}

int card_sultay_flayer(int player, int card, event_t event){
	/*
	  Sultai Flayer |3|G
	  Creature - Naga Shaman
	  Whenever a creature you control with toughness 4 or greater dies, you gain 4 life.
	  3/4
	*/

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.toughness = 3;
		this_test.toughness_flag = F5_TOUGHNESS_GREATER_THAN_VALUE;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = instance->targets[11].card;
		while( amount ){
				gain_life(player, 4);
				amount--;
		}
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_temur_charger(int player, int card, event_t event){
	/*
	  Temur Charger |1|G
	  Creature - Horse
	  Morph-Reveal a green card in your hand.
	  When Temur Charger is turned face up, target creature gains trample until end of turn.
	  3/1
	*/

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
			instance->number_of_targets = 0;
		}
	}

	return turn_face_up_revealing_card(player, card, event, COLOR_TEST_GREEN);
}

int card_trail_of_mystery(int player, int card, event_t event){
	/* todo
	   Trail of Mystery |1|G
	   Enchantment
	   Whenever a face-down creature enters the battlefield under your control, you may search your library for a basic land card, reveal it, put it into your hand, then shuffle your library.
	   Whenever a permanent you control is turned face up, if it's a creature, it gets +2/+2 until end of turn.
	*/
	// The rest of the effect is in 'flip_card'.

	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, 0, 0, 0, 0, CARD_ID_FACE_DOWN_CREATURE, 0, -1, 0) ){
		tutor_basic_land(player, 0, 0);
	}

	return global_enchantment(player, card, event);
}

/*
Tusked Colossodon |4|G|G --> vanilla
Creature - Beast
6/5
*/

int card_tuskguard_captain(int player, int card, event_t event){
	/*
	  Tuskguard Captain |2|G
	  Creature - Human Warrior
	  Outlast G (G,T: Put a +1/+1 counter on this creature. Outlast only as a sorcery.)
	  Each creature you control with a +1/+1 counter on it has trample.
	  2/3
	*/
	boost_creatures_by_counters(player, card, event, COUNTER_P1_P1, 1, 0, 0, KEYWORD_TRAMPLE, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return outlast(player, card, event, MANACOST_G(1));
}

int card_windstorm(int player, int card, event_t event){
	/*
	  Windstorm |X|G
	  Instant
	  Windstorm deals X damage to each creature with flying.
	  This card is a reprint
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, ANYBODY, get_card_instance(player, card)->info_slot, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
}

int card_wolly_loxodon(int player, int card, event_t event){
	/*
	  Woolly Loxodon |5|G|G
	  Creature - Elephant Warrior
	  Morph 5G
	  6/7
	*/
	return morph(player, card, event, MANACOST_XG(5, 1));
}

// Multicolored (25)

int card_abomination_of_gudul(int player, int card, event_t event){
	/*
	  Abomination of Gudul |3|B|G|U
	  Creature - Horror
	  Flying
	  Whenever Abomination of Gudul deals combat damage to a player, you may draw a card. If you do, discard a card.
	  Morph 2BGU
	  3/4
	*/
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER | DDBM_TRIGGER_OPTIONAL) ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return morph(player, card, event, MANACOST_XBUG(2, 1, 1, 1));
}

int card_abzan_ascendancy(int player, int card, event_t event){
	/*
	  Abzan Ascendancy |W|B|G
	  Enchantment
	  When Abzan Ascendancy enters the battlefield, put a +1/+1 counter on each creature you control.
	  Whenever a nontoken creature you control dies, put a 1/1 white Spirit creature token with flying onto the battlefield.
	*/

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_EXTRA_SKIP_TOKENS, 0);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* instance = get_card_instance(player, card);
		int amount = instance->targets[11].card;
		instance->targets[11].card = 0;

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		token.qty = amount;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_abzan_charm(int player, int card, event_t event){
	/*
	  Abzan Charm |W|B|G
	  Instant
	  Choose one -
	  * Exile target creature with power 3 or greater.
	  * You draw two cards and you lose 2 life.
	  * Distribute two +1/+1 counters among one or two target creatures.
	  */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_EXILE = 1,
		CHOICE_DRAW,
		CHOICE_COUNTERS
	};

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.power_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Exile a creature with POW 3 or more", can_target(&td1), 15,
								"Draw 2 and lose 2 life", 1, 5,
								"2 +1/+1 counters", can_target(&td2), 10);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == CHOICE_EXILE ){
			new_pick_target(&td1, "Select target creature with power 3 or higher.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_COUNTERS ){
			if( new_pick_target(&td2, "Select target creature for the first +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
				new_pick_target(&td2, "Select target creature for the second +1/+1 counter.", 1, 1 | GS_LITERAL_PROMPT);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_EXILE ){
			if( valid_target(&td1) ){
				action_on_target(player, card, 0, KILL_REMOVE);
			}
		}
		if( instance->info_slot == CHOICE_DRAW ){
			draw_cards(player, 2);
			lose_life(player, 2);
		}
		if( instance->info_slot == CHOICE_COUNTERS ){
			int i;
			for(i=0; i<2; i++){
				if( validate_target(player, card, &td2, i) ){
					add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_abzan_guide(int player, int card, event_t event){
	/*
	  Abzan Guide |3|W|B|G
	  Creature - Human Warrior
	  Lifelink
	  Morph 2WBG
	  4/4
	*/
	lifelink(player, card, event);
	return morph(player, card, event, MANACOST_XBGW(2, 1, 1, 1));
}

int card_anafenza_the_foremost(int player, int card, event_t event){
	/*
	  Anafenza, the Foremost |W|B|G
	  Legendary Creature - Human Soldier
	  Whenever Anafenza, the Foremost attacks, put a +1/+1 counter on another target tapped creature you control.
	  If a creature card would be put into an opponent's graveyard from anywhere, exile it instead.
	  4/4
	*/
	check_legend_rule(player, card, event);

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.required_state = TARGET_STATE_TAPPED;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;
		if( can_target(&td) && new_pick_target(&td, "Select another tapped creature you control.", 0, GS_LITERAL_PROMPT) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, 1-player, &this_test);

	return 0;
}

int card_ankle_shanker(int player, int card, event_t event){
	/*
	  Ankle Shanker |2|R|W|B
	  Creature - Goblin Berserker
	  Haste
	  Whenever Ankle Shanker attacks, creatures you control gain first strike and deathtouch until end of turn.
	  2/2
	*/
	haste(player, card, event);

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_FIRST_STRIKE, SP_KEYWORD_DEATHTOUCH);
	}

	return 0;
}

int card_armament_corps(int player, int card, event_t event){
	/*
	  Armament Corps |2|W|B|G
	  Creature - Human Soldier
	  When Armament Corps enters the battlefield, distribute two +1/+1 counters among one or two target creatures you control.
	  4/4
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.allowed_controller = player;
		td2.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);
		int i;
		for(i=0; i<2; i++){
			if( can_target(&td2) ){
				if( new_pick_target(&td2, "Select target creature you control.", 0, GS_LITERAL_PROMPT) ){
					add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
			else{
				break;
			}
		}
	}

	return 0;
}

int card_avalanche_tusker(int player, int card, event_t event){
	/*
	  Avalanche Tusker |2|G|U|R
	  Creature - Elephant Warrior
	  Whenever Avalanche Tusker attacks, target creature defending player controls blocks this turn if able.
	  6/4
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;
		if( can_target(&td) && new_pick_target(&td, "Select creature you opponent controls.", 0, GS_LITERAL_PROMPT) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_BLOCK);
		}
	}

	return 0;
}

int card_bears_companion(int player, int card, event_t event){
	/*
	  Bear's Companion |2|G|UR
	  Creature - Human Warrior
	  When Bear's Companion enters the battlefield, put a 4/4 green Bear creature token onto the battlefield.
	  2/2
	*/
	if( comes_into_play(player, card, event) ){
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_BEAR, &token);
	  token.pow = token.tou = 4;
	  token.color_forced = COLOR_TEST_GREEN;
	  generate_token(&token);
	}

	return 0;
}

int card_butcher_of_the_horde(int player, int card, event_t event){
	/*
	  Butcher of the Horde |1|R|W|B
	  Creature - Demon
	  Flying
	  Sacrifice another creature: Butcher of the Horde gains your choice of vigilance, lifelink, or haste until end of turn.
	  5/4
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int skey [3] = {SP_KEYWORD_VIGILANCE, SP_KEYWORD_LIFELINK, SP_KEYWORD_HASTE};
		int ai_choice = 0;
		if( is_sick(instance->parent_controller, instance->parent_card) && current_phase < PHASE_DECLARE_ATTACKERS ){
			ai_choice = 2;
		}
		if( life[player] < 6 ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Vigilance\n Lifelink\n Haste", ai_choice);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, 0, skey[choice]);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST0, 0, NULL, NULL);
}

int card_chief_of_the_edge(int player, int card, event_t event){
	/*
	  Chief of the Edge |W|B
	  Creature - Human Warrior
	  Other Warrior creatures you control get +1/+0.
	  3/2
	*/
	boost_creature_type(player, card, event, SUBTYPE_WARRIOR, 1, 0, 0, BCT_CONTROLLER_ONLY);
	return 0;
}

int card_chief_of_the_scale(int player, int card, event_t event){
	/*
	  Chief of the Scale |W|B
	  Creature - Human Warrior
	  Other Warrior creatures you control get +0/+1.
	  2/3
	*/
	boost_creature_type(player, card, event, SUBTYPE_WARRIOR, 0, 1, 0, BCT_CONTROLLER_ONLY);
	return 0;
}

int card_crackling_doom(int player, int card, event_t event){
	/*
	  Crackling Doom |R|W|B
	  Instant
	  Crackling Doom deals 2 damage to each opponent. Each opponent sacrifices a creature with the greatest power among creatures he or she controls.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		damage_player(1-player, 2, player, card);
		if( can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int max_pow = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_MAX_POW, &this_test);
			int c, creatures_found = 0;
			for(c=0; c<active_cards_count[1-player]; c++){
				if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) ){
					if( get_power(1-player, c) != max_pow ){
						state_untargettable(1-player, c, 1);
					}
					else{
						creatures_found++;
					}
				}
			}
			if( creatures_found > 1 ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);
				td.allowed_controller = 1-player;
				td.who_chooses = 1-player;
				td.allow_cancel = 0;

				if( new_pick_target(&td, "Select a creature to sacrifice.", 0, GS_LITERAL_PROMPT) ){
					card_instance_t *instance = get_card_instance(player, card);
					add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
				}

			}
			for(c=active_cards_count[1-player]-1; c>-1; c--){
				if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) ){
					if( check_state(1-player, c, STATE_CANNOT_TARGET) ){
						state_untargettable(1-player, c, 0);
					}
					else{
						if( creatures_found == 1 || (creatures_found > 1 && check_state(1-player, c, STATE_TARGETTED)) ){
							kill_card(1-player, c, KILL_SACRIFICE);
						}
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int death_frenzy_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		gain_life(player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_death_frenzy(int player, int card, event_t event){
	/*
	  Death Frenzy |3|B|G
	  Sorcery
	  All creatures get -2/-2 until end of turn. Whenever a creature dies this turn, you gain 1 life.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &death_frenzy_legacy);
		APNAP(p, {pump_subtype_until_eot(player, card, p, -1, -2, -2, 0, 0);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_deflecting_palm(int player, int card, event_t event){
	/*
	  Deflecting Palm |R|W
	  Instant
	  The next time a source of your choice would deal damage to you this turn, prevent that damage. If damage is prevented this way, Deflecting Palm deals that much damage to that source's controller.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			card_instance_t *instance = get_card_instance(player, card);
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			int amount = target->info_slot;
			target->info_slot = 0;
			damage_player(target->damage_source_player, amount, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td1, "TARGET_DAMAGE", 1, NULL);
}


int card_duneblast(int player, int card, event_t event){
	/*
	  Duneblast |4|W|B|G
	  Sorcery
	  Choose up to one creature. Destroy the rest.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);
		if( new_pick_target(&td, "Select a creature.", 0, GS_LITERAL_PROMPT) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);});
		if( instance->number_of_targets > 0){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_efreet_weaponmaster(int player, int card, event_t event){
	/*
	  Efreet Weaponmaster |3|U|R|W
	  Creature - Efreet Monk
	  First strike
	  When Efreet Weaponmaster enters the battlefield or is turned face up, another target creature you control gets +3/+0 until end of turn.
	  Morph 2URW
	  4/3
	*/
	if( comes_into_play(player, card, event) || event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		if( can_target(&td) && new_pick_target(&td, "Select another target creature.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 0);

		}
	}

	return morph(player, card, event, MANACOST_XURW(2, 1, 1, 1));
}


int card_flying_crane_technique(int player, int card, event_t event){
	/*
	  Flying Crane Technique |3|U|R|W
	  Instant
	  Untap all creatures you control. They gain flying and double strike until end of turn.
	*/
	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
		pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_FLYING | KEYWORD_DOUBLE_STRIKE, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

/*
Highspire Mantis |2|R|W --> vanilla
Creature - Insect
Flying, trample
3/3
*/

int card_icefeather_aven(int player, int card, event_t event){
	/*
	  Icefeather Aven |U|G
	  Creature - Bird Shaman
	  Flying
	  Morph 1GU
	  When Icefeather Aven is turned face up, you may return another target creature to its owner's hand.
	  2/2
	*/

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return morph(player, card, event, MANACOST_XUG(1, 1, 1));
}

static int has_1_1_counter(int unused, int unused2, int player, int card){
	return count_1_1_counters(player, card) ? 1 : 0;
}

int card_ivorytusk_fortress(int player, int card, event_t event){
	/*
	  Ivorytusk Fortress |2W|B|G
	  Creature - Elephant
	  Untap each creature you control with a +1/+1 counter on it during each other player's untap step.
	  5/7
	*/
	if( ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if (current_phase == PHASE_UNTAP && current_turn == 1-player){
			if( instance->info_slot != 77){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.special_selection_function = &has_1_1_counter;
				new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
				instance->info_slot = 77;
			}
		}
		else if (instance->info_slot == 77){
				instance->info_slot = 0;
		}
	}
	return 0;
}

int card_jeskai_ascendancy(int player, int card, event_t event){
	/*
	  Jeskai Ascendancy |U|R|W
	  Enchantment
	  Whenever you cast a noncreature spell, creatures you control get +1/+1 until end of turn. Untap those creatures.
	  Whenever you cast a noncreature spell, you may draw a card. If you do, discard a card.
	*/
	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int c = active_cards_count[player]-1;
		while( c > -1 ){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
					untap_card(player, c);
					pump_until_eot_merge_previous(player, card, player, c, 1, 1);
				}
				c--;
		}
		if( do_dialog(player, player, card, -1, -1, " Draw & discard\n Pass", count_deck(player) <= 10) == 0 ){
			draw_cards(player, 1);
			discard(player, 0, player);
		}
	}
	return global_enchantment(player, card, event);
}

int card_jeskai_charm(int player, int card, event_t event){
	/*
	  Jeskai Charm |U|R|W
	  Instant
	  Choose one -
	  * Put target creature on top of its owner's library.
	  * Jeskai Charm deals 4 damage to target opponent.
	  * Creatures you control get +1/+1 and gain lifelink until end of turn.
	  */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_BOUNCE = 1,
		CHOICE_DAMAGE_PLAYER,
		CHOICE_PUMP
	};

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int priorities[3] = {	8,
									life[1-player] <= 4 ? 15 : 5,
									current_phase == PHASE_AFTER_BLOCKING ? 10 : 5
								};
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Bounce a creature", can_target(&td1), priorities[0],
								"4 damage to target opponent", would_validate_arbitrary_target(&td2, 1-player, -1), priorities[1],
								"+1/+1 & Lifelink", 1, priorities[2]);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == CHOICE_BOUNCE ){
			pick_target(&td1, "TARGET_CREATURE");
		}
		if( instance->info_slot == CHOICE_DAMAGE_PLAYER ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_BOUNCE ){
			if( valid_target(&td1) ){
				action_on_target(player, card, 0, ACT_BOUNCE);
			}
		}
		if( instance->info_slot == CHOICE_DAMAGE_PLAYER ){
			if( valid_target(&td2) ){
				damage_target0(player, card, 4);
			}
		}
		if( instance->info_slot == CHOICE_PUMP ){
			pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, SP_KEYWORD_LIFELINK);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int kheru_lich_lord_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( instance->damage_target_player > -1 ){
		haste(instance->damage_target_player, instance->damage_target_card, event);
		if( event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result |= KEYWORD_FLYING | KEYWORD_TRAMPLE;
		}
		if( eot_trigger(player, card, event) ){
			if( ! check_state(instance->damage_target_player, instance->damage_target_card, STATE_OUBLIETTED) ){
				add_card_to_rfg(get_owner(instance->damage_target_player, instance->damage_target_card),
								get_original_internal_card_id(instance->damage_target_player, instance->damage_target_card));
				kill_card(instance->damage_target_player, instance->damage_target_card, KILL_REMOVE);
				return 0;
			}
			kill_card(player, card, KILL_REMOVE);
		}
		if( leaves_play(instance->damage_target_player, instance->damage_target_card, event) ){
			add_card_to_rfg(get_owner(instance->damage_target_player, instance->damage_target_card),
							get_original_internal_card_id(instance->damage_target_player, instance->damage_target_card));
		}
	}
	return 0;
}

int card_kheru_lich_lord(int player, int card, event_t event){
	/*
	  Kheru Lich Lord |3|B|G|U
	  Creature - Zombie Wizard
	  At the beginning of your upkeep, you may pay 2B. If you do, return a creature card at random from your graveyard to the battlefield. It gains flying, trample, and haste. Exile that card at the beginning of the next end step. If that card would leave the battlefield, exile it instead of putting it anywhere else.
	  4/4
	*/
	upkeep_trigger_ability_mode(player, card, event, player, has_mana_multi(player, MANACOST_XB(2, 1)) && count_graveyard_by_type(player, TYPE_CREATURE) ?
								RESOLVE_TRIGGER_AI(player) : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( has_mana_multi(player, MANACOST_XB(2, 1)) ){
			charge_mana_multi(player, MANACOST_XB(2, 1));
			if( spell_fizzled != 1 ){
				int result = tutor_random_permanent_from_grave(player, card, player, TUTOR_PLAY, TYPE_CREATURE, 1, REANIMATE_DEFAULT);
				if( result > -1 ){
					convert_to_token(player, result);
					set_special_flags(player, card, SF_UNEARTH); //Not really, but the effect is the same.
					create_targetted_legacy_effect(player, card, &kheru_lich_lord_legacy, player, result);
				}
			}
		}
	}

	return 0;
}

// Spirit Warrior --> rhino token.

int card_kin_tree_invocation(int player, int card, event_t event){
	/*
	  Kin-Tree Invocation |B|G
	  Sorcery
	  Put an X/X black and green Spirit Warrior creature token onto the battlefield, where X is the greatest toughness among creatures you control.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT_WARRIOR, &token);
		token.pow = token.tou = check_battlefield_for_special_card(player, card, player, CBFSC_GET_MAX_TOU, &this_test);
		token.color_forced = COLOR_TEST_BLACK | COLOR_TEST_GREEN;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*
Mantis Rider |U|R|W --> Lightning Angel
Creature - Human Monk
Flying, vigilance, haste
*/

int card_mardu_ascendancy(int player, int card, event_t event){
	/*
	  Mardu Ascendancy |R|W|B
	  Enchantment
	  Whenever a nontoken creature you control attacks, put a 1/1 red Goblin creature token onto the battlefield tapped and attacking.
	  Sacrifice Mardu Ascendancy: Creatures you control get +0/+3 until end of turn.
	*/
	if( xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.type_flag = F1_NO_TOKEN;

		int amt;
		if ((amt = declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_AI(player), player, -1, &test))){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN, &token);
			token.action = TOKEN_ACTION_ATTACKING;
			token.qty = amt;
			generate_token(&token);
		}
	}

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, card, player, -1, 0, 3, 0, 0);
	}

	return global_enchantment(player, card, event);
}

static void mardu_charm_token_pump(token_generation_t* token, int card_added, int number)
{
  pump_ability_until_eot(token->s_player, token->s_card, token->t_player, card_added, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_mardu_charm(int player, int card, event_t event){
	/*
	  Mardu Charm |R|W|B
	  Instant
	  Choose one -
	  * Mardu Charm deals 4 damage to target creature.
	  * Put two 1/1 white Warrior creature tokens onto the battlefield. They gain first strike until end of turn.
	  * Target opponent reveals his or her hand. You choose a noncreature, nonland card from it. That player discards that card.
	  */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_DAMAGE_CREATURE = 1,
		CHOICE_WARRIORS,
		CHOICE_DURESS
	};

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int priorities[3] = {	15,
									current_phase == PHASE_AFTER_BLOCKING ? 10 : 5,
									8
								};
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"4 damage to a creature", can_target(&td1), priorities[0],
								"2 warriors", 1, priorities[1],
								"Duress", would_validate_arbitrary_target(&td2, 1-player, -1), priorities[2]);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == CHOICE_DAMAGE_CREATURE ){
			pick_target(&td1, "TARGET_CREATURE");
		}
		if( instance->info_slot == CHOICE_DURESS ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_DAMAGE_CREATURE){
			if( valid_target(&td1) ){
				damage_target0(player, card, 4);
			}
		}
		if( instance->info_slot == CHOICE_WARRIORS ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_WARRIOR, &token);
			token.pow = token.tou = 1;
			token.qty = 2;
			token.color_forced = COLOR_TEST_WHITE;
			token.special_code_on_generation = &mardu_charm_token_pump;
			generate_token(&token);
		}
		if( instance->info_slot == CHOICE_DURESS ){
			if( valid_target(&td2) ){
				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);

				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a nonland, noncreature card.");
				this_test.type_flag = DOESNT_MATCH;
				new_effect_coercion(&ec, &this_test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mardu_roughrider(int player, int card, event_t event){
	/*
	  Mardu Roughrider |2|R|W|B
	  Creature - Orc Warrior
	  Whenever Mardu Roughrider attacks, target creature can't block this turn.
	  5/4
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

int card_master_the_way(int player, int card, event_t event){
	/*
	  Master the Way |3|U|R
	  Sorcery
	  Draw a card. Master the Way deals damage to target creature or player equal to the number of cards in your hand.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, 1);
			damage_target0(player, card, hand_count[player]);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_mindswipe(int player, int card, event_t event){
	/*
	  Mindswipe |X|U|R
	  Instant
	  Counter target spell unless its controller pays X. Mindswipe deals X damage to that spell's controller.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		if( counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot) ){
			damage_player(instance->targets[0].player, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL | GS_X_SPELL, NULL, NULL, 1, NULL);
}

int card_narset_enlightened_master(int player, int card, event_t event){
	/*
	  Narset, Enlightened Master |3|U|R|W
	  Legendary Creature - Human Monk
	  First strike, hexproof
	  Whenever Narset, Enlightented Master attacks, exile the top four cards of your library. Until end of turn, you may cast noncreature cards exiled with
	  Narset this turn without paying their mana costs.
	  3/2
	*/
	check_legend_rule(player, card, event);
	hexproof(player, card, event);

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card)){
		int count = 0;
		while( deck_ptr[player][0] != -1 && count < 4 ){
			int iid = deck_ptr[player][0];
			if (iid != -1){
				rfg_top_card_of_deck(player);
				if (!is_what(-1, iid, TYPE_CREATURE | TYPE_LAND)){
					create_may_play_card_from_exile_effect(player, card, player, cards_data[iid].id, MPCFE_FOR_FREE | MPCFE_UNTIL_EOT);
				}
			}
			count++;
		}
	}

	return 0;
}

int card_ponyback_brigade(int player, int card, event_t event){
	/*
	  Ponyback Brigade |3|R|W|B
	  Creature - Goblin Warrior
	  When Ponyback Brigade enters the battlefield or is turned face up, put three 1/1 red Goblin creature tokens onto the battlefield.
	  Morph 2RWB
	  2/2
	*/
	if( event == EVENT_TURNED_FACE_UP || comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 3);
	}

	return morph(player, card, event, MANACOST_XBRW(2, 1, 1, 1));
}

int card_rakshasa_deathdealer(int player, int card, event_t event){
	/*
	  Rakshasa Deathdealer |B|G
	  Creature - Cat Demon
	  BG: Rakshasa Deathdealer gets +2/+2 until end of turn.
	  BG: Regenerate Rakshasa Deathdealer.
	  2/2
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_BG(1, 1), 0, NULL, NULL);
		if( result ){
			return result;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_BG(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int result = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION, MANACOST_BG(1, 1), 0, NULL, NULL);
		generic_activated_ability(player, card, event, 0, MANACOST_BG(1, 1), 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			instance->info_slot = result ? 1 : 2;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 2 ){
			pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_BG(1, 1), 0, NULL, NULL);
	}

	return 0;
}

/*
Rakshasa Vizier |2|B|G|U --> vanilla, the effect is in "rfg_card_from_grave"
Creature - Cat Demon
Whenever one or more cards are put into exile from your graveyard, put that many +1/+1 counters on Rakshasa Vizier.
4/4
*/

static void give_trample(int player, int card, int t_player, int t_card)
{
	pump_ability_until_eot(player, card, t_player, t_card, 0, 0, KEYWORD_TRAMPLE, 0);
}

int card_ride_down(int player, int card, event_t event){
	/*
	  Ride Down |R|W
	  Instant
	  Destroy target blocking creature. Creatures that were blocked by that creature this combat gain trample until end of turn.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_BLOCKING;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			for_each_creature_blocked_by_me(instance->targets[0].player, instance->targets[0].card, give_trample, player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target blocking creature.", 1, NULL);
}

int card_sage_of_the_inward_eye(int player, int card, event_t event){
	/*
	  Sage of the Inward Eye |2|U|R|W
	  Creature - Djinn Wizard
	  Flying
	  Whenever you cast a noncreature spell, creatures you control gain lifelink until end of turn.
	  3/4
	*/
	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_LIFELINK);
	}

	return 0;
}

int card_sagu_mauler(int player, int card, event_t event){
	/*
	  Sagu Mauler |4|U|G
	  Creature - Beast
	  Trample, hexproof
	  Morph 3GU (You may cast this card face down as a 2/2 creature for 3. Turn it face up any time for its morph cost.)
	  6/6
	*/
	hexproof(player, card, event);
	return morph(player, card, event, MANACOST_XUG(3, 1, 1));
}

int card_savage_knucklebone(int player, int card, event_t event){
	/*
	  Savage Knuckleblade |G|U|R
	  Creature - Ogre Warrior
	  2G: Savage Knuckleblade gets +2/+2 until end of turn. Activate this ability only once each turn.
	  2U: Return Savage Knuckleblade to its owner's hand.
	  R: Savage Knuckleblade gains haste until end of turn.
	  4/4
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[2].player = 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_XG(2, 1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int abilities[3] = {	generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST_XG(2, 1), 0, NULL, NULL),
								has_mana_for_activated_ability(player, card, MANACOST_XU(2, 1)),
								has_mana_for_activated_ability(player, card, MANACOST_R(1))
		};
		int priorities[4] = {	10,
								8,
								current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS && check_state(player, card, STATE_SUMMON_SICK) ?
								15 : 5
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Pump", abilities[0], priorities[0],
							"Return to owner's hand", abilities[1], priorities[1],
							"Give Haste", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, ((choice == 1 || choice == 2) ? 2 : 0),
															0,
															(choice == 2 ? 1 : 0),
															(choice == 1 ? 1 : 0),
															(choice == 3 ? 1 : 0),
															0)

		  ){
			instance->info_slot = choice;
			if( choice == 1 ){
				instance->targets[2].player |= (1<<player);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
		}
		if( instance->info_slot == 2 ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 3 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return 0;
}

int card_secret_plan(int player, int card, event_t event){
	/*
	  Secret Plan |G|U
	  Enchantment
	  Face-down creatures you control get +0/+1.
	  Whenever a permanent you control is turned face up, draw a card.
	*/
	// The other half of the effect is in "flip_card"

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && event == EVENT_TOUGHNESS ){
			if( get_id(affected_card_controller, affected_card) == CARD_ID_FACE_DOWN_CREATURE ){
				event_result++;
			}
		}
	}

	return global_enchantment(player, card, event);
}


int card_sidisi_brood_tyrant(int player, int card, event_t event){
	/*
	  Sidisi, Brood Tyrant |1|B|G|U
	  Legendary Creature - Naga Shaman
	  Whenever Sidisi, Brood Tyrant enters the battlefield or attacks, put the top three cards of your library into your graveyard.
	  Whenever one or more creature cards are put into your graveyard from your library, put a 2/2 black Zombie creature token onto the battlefield.
	  3/3
	*/
	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) || declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		mill(player, 3);
	}

	enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
	if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player && ! is_humiliated(player, card)
	  ){
		if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;

		if (event == EVENT_RESOLVE_TRIGGER){
			int zombo = 0;
			int i;
			for (i = 0; i< num_cards_milled; i++){
				if( cards_milled[i].source != -1 && is_what(-1,  cards_milled[i].internal_card_id, TYPE_CREATURE) ){
					zombo = 1;
					break;
				}
			}
			if( zombo ){
				generate_token_by_id(player, card, CARD_ID_ZOMBIE);
			}
		}
	}

	return 0;
}

int card_siege_rhino(int player, int card, event_t event){
	/*
	  Siege Rhino |1|W|B|G
	  Creature - Rhino
	  Trample
	  When Siege Rhino enters the battlefield, each opponent loses 3 life and you gain 3 life.
	  4/5
	*/
	if( comes_into_play(player, card, event) ){
		lose_life(1-player, 3);
		gain_life(player, 3);
	}

	return 0;
}

int card_snowhorn_rider(int player, int card, event_t event){
	/*
	  Snowhorn Rider |3|G|U|R
	  Creature - Human Warrior
	  Trample
	  Morph 2GUR
	  5/5
	*/
	return morph(player, card, event, MANACOST_XUGR(2, 1, 1, 1));
}

int card_sorin_solemn_visitor(int player, int card, event_t event){
	/*
	  Sorin, Solemn Visitor |2|W|B
	  Planeswalker - Sorin
	  +1: Until your next turn, creatures you control get +1/+0 and gain lifelink.
	  -2: Put a 2/2 black Vampire creature token with flying onto the battlefield.
	  -6: You get an emblem with "At the beginning of each opponent's upkeep, that player sacrifices a creature."
	  4
	*/
	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		card_instance_t *instance = get_card_instance(player, card);

		int priorities[3] = {	count_subtype(player, TYPE_CREATURE, -1) * 3,
								5,
								count_subtype(1-player, TYPE_CREATURE, -1) * 3,
		};

		enum{
			CHOICE_PUMP = 1,
			CHOICE_VAMPIRE,
			CHOICE_EMBLEM
		} choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						  "Pump your creatures", 1, priorities[0], 1,
						  "Generate a Vampire", 1, priorities[1], -2,
						  "Emblem", 1, priorities[2], -6
						  );

		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
		}
		else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_PUMP:
				{
					pump_ability_t pump;
					default_pump_ability_definition(player, card, &pump, 1, 0, 0, SP_KEYWORD_LIFELINK);
					pump.paue_flags = PAUE_END_AT_THE_BEGINNING_OF_YOUR_NEXT_TURN;
					pump_ability_by_test(instance->parent_controller, instance->parent_card, player, &pump, NULL);
				}
				break;
				case CHOICE_VAMPIRE:
				{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_VAMPIRE, &token);
					token.pow = 2;
					token.tou = 2;
					token.color_forced = COLOR_TEST_BLACK;
					token.key_plus = KEYWORD_FLYING;
					generate_token(&token);
					break;
				}
				case CHOICE_EMBLEM:
					generate_token_by_id(player, card, CARD_ID_SORINS_SOLEMN_EMBLEM);
					break;
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_sorin_solemn_visitor_emblem(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_sultai_ascendancy(int player, int card, event_t event){
	/*
	  Sultai Ascendancy |G|B|U
	  Enchantment
	  At the beginning of your upkeep, look at the top two cards of your library.
	  Put any number of them into your graveyard and the rest on top of your library in any order.
	*/
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = MIN(2, count_deck(player));
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put into your graveyard.");
		this_test.no_shuffle = 1;
		while( amount ){
				this_test.create_minideck = amount;
				if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_MIN_VALUE, &this_test) != -1 ){
					amount--;
				}
				else{
					break;
				}
		}
		if( amount > 1 ){
			rearrange_top_x(player, player, amount);
		}
	}

	return global_enchantment(player, card, event);
}

int card_sultai_charm(int player, int card, event_t event){
	/*
	  Sultai Charm |B|G|U
	  Instant
	  Choose one -
	  * Destroy target monocolored creature.
	  * Destroy target artifact or enchantment.
	  * Draw two cards, then discard a card.
	  */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_KILL_MONOCOLORED = 1,
		CHOICE_DISENCHANT,
		CHOICE_DRAW2_DISCARD
	};

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = (int32_t)target_is_monocolored;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;

		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
								"Kill a monocolored creature", can_target(&td1), 10,
								"Disenchant", can_target(&td2), 8,
								"Draw 2 and discard", 1, 5);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}

		if( instance->info_slot == CHOICE_KILL_MONOCOLORED ){
			new_pick_target(&td1, "Select target monocolored creature.", 0, GS_LITERAL_PROMPT);
		}

		if( instance->info_slot == CHOICE_DISENCHANT ){
			pick_target(&td2, "DISENCHANT");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot == CHOICE_KILL_MONOCOLORED && valid_target(&td1)) ||
			(instance->info_slot == CHOICE_DISENCHANT && valid_target(&td2))
		){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot == CHOICE_DRAW2_DISCARD ){
			draw_cards(player, 2);
			discard(player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sultai_soothsayer(int player, int card, event_t event){
	/*
	  Sultai Soothsayer |2|B|U|G
	  Creature - Naga Shaman 2/5, 2UBG (5)
	  When Sultai Soothsayer enters the battlefield, look at the top four cards of your library. Put one of them into your hand and the rest into your graveyard.
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		reveal_top_cards_of_library_and_choose(player, card, player, 5, 1, TUTOR_HAND, 0, TUTOR_GRAVE, 1, &test);
	}

	return 0;
}

int card_surrak_dragonclaw(int player, int card, event_t event){
	/* todo
	   Surrak Dragonclaw |2|G|U|R
	   Legendary Creature - Human Warrior
	   Flash
	   Surrak Dragonclaw can't be countered.
	   Creature spells you control can't be countered.
	   Other creatures you control have trample.
	   6/6
	*/
	cannot_be_countered(player, card, event);

	type_uncounterable(player, card, event, player, TYPE_CREATURE, NULL);

	boost_creature_type(player, card, event, -1, 0, 0, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY);

	return flash(player, card, event);
}


int card_temur_ascendancy(int player, int card, event_t event){
	/*
	  Temur Ascendancy |G|U|R
	  Enchantment
	  Creatures you control have haste.
	  Whenever a creature with power 4 or greater enters the battlefield under your control, you may draw a card.
	*/
	boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_CHECK_DECK_COUNT(player, 1), &this_test) ){
			draw_cards(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_temur_charm(int player, int card, event_t event){
	/*
	  Temur Charm |G|U|R
	  Instant
	  Choose one -
	  * Target creature you control gets +1/+1 until end of turn. It fights target creature you don't control.
	  * Counter target spell unless its controller pays 3.
	  * Creatures with power 3 or less can't block this turn.
	  */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_FIGHT = 1,
		CHOICE_COUNTER,
		CHOICE_CANNOT_BLOCK
	};

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller = 1-player;

	target_definition_t td3;
	counterspell_target_definition(player, card, &td3, TYPE_ANY);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_COUNTERSPELL, &td3, NULL, 1, NULL) ){
			return 99;
		}
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;

		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int abils[3] = {can_target(&td1) && can_target(&td2),
							generic_spell(player, card, EVENT_CAN_CAST, GS_COUNTERSPELL, &td3, NULL, 1, NULL),
							1};

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
								"Make two creatures fight", abils[0], 10,
								"Counterspell", abils[1], 15,
								"Crits with POW < 4 cannot block", abils[2], 5);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}

		if( instance->info_slot == CHOICE_FIGHT ){
			if( new_pick_target(&td1, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				new_pick_target(&td2, "Select target creature your opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
			}
		}

		if( instance->info_slot == CHOICE_COUNTER ){
			return counterspell(player, card, event, &td3, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_FIGHT ){
			if( validate_target(player, card, &td1, 0) && validate_target(player, card, &td2, 1) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
				fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		if( instance->info_slot == CHOICE_COUNTER ){
			counterspell_resolve_unless_pay_x(player, card, &td3, 0, 3);
		}
		if( instance->info_slot == CHOICE_CANNOT_BLOCK ){
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			pump.paue_flags = PAUE_END_AT_EOT;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.power = 4;
			this_test.power_flag = F5_POWER_LESSER_THAN_VALUE;

			pump_ability_by_test(player, card, ANYBODY, &pump, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_trap_essence(int player, int card, event_t event){
	/*
	  Trap Essence |G|U|R
	  Instant
	  Counter target creature spell. Put two +1/+1 counters on up to one target creature.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = card_on_stack_controller;
		instance->targets[0].card = card_on_stack;
		instance->number_of_targets = 1;
		if( can_target(&td) ){
			new_pick_target(&td, "TARGET_CREATURE", 1, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->number_of_targets > 1 && validate_target(player, card, &td, 1) ){
			add_1_1_counters(instance->targets[1].player, instance->targets[1].card, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

const char* target_is_nonland(int who_chooses, int player, int card){
	if( ! is_what(player, card, TYPE_LAND) ){
		return NULL;
	}
	return "must be a nonland permanent.";
}

int card_utter_end(int player, int card, event_t event){
	/*
	  Utter End |2|W|B
	  Instant
	  Exile target nonland permanent.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.extra = (int32_t)target_is_nonland;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 1, NULL);
}

int card_villainous_wealth(int player, int card, event_t event){

	/*
	  Villainous Wealth |X|B|G|U
	  Sorcery
	  Target opponent exiles the top X cards of his or her library. You may cast any number of nonland cards with converted mana cost X or less from among them without paying their mana cost.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int t_player = instance->targets[0].player;
			int max = MIN(count_deck(t_player), instance->info_slot);
			if( max ){
				int *deck = deck_ptr[t_player];
				show_deck(player, deck, max, "Cards revealed with Villainous Wealth.", 0, 0x7375B0 );
				int i;
				int cards_exiled[max];
				for(i=0; i<max; i++){
					cards_exiled[i] = deck[0];
					rfg_card_in_deck(t_player, 0);
				}
				int playable[max];
				int pc = 0;
				for(i=0; i<max; i++){
					if( is_what(-1, cards_exiled[i], TYPE_SPELL) && ! is_what(-1, cards_exiled[i], TYPE_CREATURE) &&
						get_cmc_by_internal_id(cards_exiled[i]) <= instance->info_slot && can_legally_play_iid(player, cards_exiled[i])
					  ){
						playable[pc] = cards_exiled[i];
						pc++;
					}
				}
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_SPELL, "Select a spell to play for free.");
				while( pc ){
						int selected = select_card_from_zone(player, player, playable, pc, 0, AI_MAX_VALUE, -1, &this_test);
						if( selected != -1 ){
							int csvid = cards_data[playable[selected]].id;
							int k;
							for(k=0; k<max; k++){
								if( cards_exiled[k] == playable[selected] ){
									cards_exiled[k] = -1;
									break;
								}
							}
							for(k=selected; k<pc; k++){
								playable[k] = playable[k+1];
							}
							pc--;
							play_card_in_exile_for_free(player, t_player, csvid);
						}
						else{
							break;
						}
				}
				for(i=0; i<max; i++){
					if( cards_exiled[i] != -1 ){
						int pos = find_iid_in_rfg(t_player, cards_exiled[i]);
						from_exile_to_graveyard(player, pos);
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_warden_of_the_eye(int player, int card, event_t event){
	/*
	  Warden of the Eye |2|U|R|W
	  Creature - Djinn Wizard
	  When Warden of the Eye enters the battlefield, return target noncreature, nonland card from your graveyard to your hand.
	  3/3
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a noncreature, nonland card.");
		this_test.type_flag = DOESNT_MATCH;
		if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_winterflame(int player, int card, event_t event){
	/*
	  Winterflame |1|U|R
	  Instant
	  Choose one or both -
	  * Tap target creature.
	  * Winterflame deals 2 damage to target creature.
	  */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{
		CHOICE_TAP = 1,
		CHOICE_DAMAGE,
		CHOICE_BOTH
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Tap creature", 1, 8,
							"Damage creature", 1, 10,
							"Do both", 1, 15);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot & CHOICE_TAP ){
			if( new_pick_target(&td, "Select target creature to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
				add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
			}
			else{
				return 0;
			}
		}
		if( instance->info_slot & CHOICE_DAMAGE ){
			int tt = (instance->info_slot & CHOICE_TAP) ? 1 : 0;
			if( new_pick_target(&td, "Select target creature to damage.", tt, 1 | GS_LITERAL_PROMPT) ){
				add_state(instance->targets[tt].player, instance->targets[tt].card, STATE_TARGETTED);
			}
			else{
				return 0;
			}
		}
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)){
			if( instance->info_slot & CHOICE_TAP ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				damage_target0(player, card, 2);
			}
		}
		if( instance->number_of_targets == 2 && validate_target(player, card, &td, 1) && instance->info_slot == CHOICE_BOTH ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_zurgo_helmsmasher(int player, int card, event_t event){
	/*
	  Zurgo Helmsmasher |2|R|W|B
	  Legendary Creature - Orc Warrior
	  Haste
	  Zurgo Helmsmasher attacks each combat if able.
	  Zurgo Helmsmasher has indestructible as long as it's your turn.
	  Whenever a creature dealt damage by Zurgo Helmsmasher this turn dies, put a +1/+1 counter on Zurgo Helmsmasher.
	  7/2
	*/
	check_legend_rule(player, card, event);
	attack_if_able(player, card, event);
	haste(player, card, event);
	if( current_turn == player ){
		indestructible(player, card, event);
	}
	if( sengir_vampire_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

// Lands (30)
/*
Bloodfell Caves (and all the others similar lands from KoT) --> Akoum Refuge
Land
Bloodfell Caves enters the battlefield tapped.
When Bloodfell Caves enters the battlefield, you gain 1 life.
T: Add B or R to your mana pool.

Mystic Monastery (and the other 3 color lands from KoT) --> Shivan Oasis
Land
Mystic Monastery enters the battlefield tapped.
T: Add U, R, or W to your mana pool.
*/

int card_tomb_of_the_spirit_dragon(int player, int card, event_t event){
	/*
	  Tomb of the Spirit Dragon
	  Land
	  {T}: Add {1} to your mana pool.
	  {2}, {T}: You gain 1 life for each colorless creature you control.
	*/
	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE  ){
		int choice = instance->info_slot = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Gain life\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				instance->info_slot = 1;
				tap_card(player, card);
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
		int amount = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && get_color(player, c) == 0 ){
				amount++;
			}
		}
		gain_life(player, amount);
	}

	return 0;
}

// Artifacts

/*
Abzan Banner |3 --> Azorius Cluestone (same code for all Banners)
{T}: Add {W}, {B}, or {G} to your mana pool.
{W}{B}{G}, {T}, Sacrifice Abzan Banner: Draw a card.
*/

int card_altar_of_the_brood(int player, int card, event_t event){
	/*
	  Altar of the Brood	|1
	  Artifact
	  Whenever another permanent enters the battlefield under your control, each opponent puts the top card of his or her library into his or her graveyard.
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			mill(1-player, 1);
		}
	}

	return 0;
}

int card_bribers_purse(int player, int card, event_t event){
	/*
	  Briber's Purse	|X
	  Artifact
	  Briber's Purse enters the battlefield with X gem counters on it.
	  {1}, {T}, Remove a gem counter from Briber's Purse: Target creature can't attack or block this turn.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		if (played_for_free(player, card) || is_token(player, card)){
			instance->info_slot = 0;
		} else {
			charge_mana(player, COLOR_COLORLESS, -1);
			instance->info_slot = x_value;
		}
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_GEM, instance->info_slot);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_t pump;
		default_pump_ability_definition(player, card, &pump, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		pump.paue_flags = PAUE_END_AT_EOT | PAUE_CANT_ATTACK;
		pump_ability(player, card, instance->targets[0].player, instance->targets[0].card, &pump);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), GVC_COUNTER(COUNTER_GEM), &td, "TARGET_CREATURE");
}

int card_cranial_archive(int player, int card, event_t event){
	/*
	  Cranial Archive |2
	  Artifact
	  {2}, Exile Cranial Archive: Target player shuffles his or her graveyard into his or her library. Draw a card.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		reshuffle_grave_into_deck(instance->targets[0].player, 0);
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_RFG_ME | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_PLAYER");
}

int card_dragon_throne_of_tarkir(int player, int card, event_t event){
	/*
	  Dragon Throne of Tarkir |4
	  Legendary Artifact - Equipment
	  Equipped creature has defender and "{2}, {T}: Other creatures you control gain trample and get +X/+X until end of turn, where X is this creature's power."
	  Equip {3}
	*/
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 0, 0, KEYWORD_DEFENDER);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if (is_equipping(player, card)){
			int result = granted_generic_activated_ability(player, card, instance->targets[8].player, instance->targets[8].card, event,
															GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
			if( result ){
				return result;
			}
		}
		return can_activate_basic_equipment(player, card, event, 3);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		instance->number_of_targets = 0;
		int can_pump = 0;
		int can_equip = can_activate_basic_equipment(player, card, event, 3);
		if(is_equipping(player, card)){
			can_pump = granted_generic_activated_ability(player, card, instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE,
														GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Equip", can_equip, 10,
						"Pump other creatures", can_pump, 15);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = choice;

		if( choice == 1 ){
			return basic_equipment(player, card, event, 3);
		}
		if( choice == 2 ){
			if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST_X(2)) ){
				tap_card(instance->targets[8].player, instance->targets[8].card);
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			return basic_equipment(player, card, event, 3);
		}
		if( instance->info_slot == 2 ){
			int amount = get_power(instance->targets[8].player, instance->targets[8].card);
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						if( count != instance->targets[8].card ){
							pump_ability_until_eot(instance->parent_controller, instance->parent_card, player, count, amount, amount, KEYWORD_TRAMPLE, 0);
						}
					}
					count--;
			}
		}
	}

	return 0;
}

static const char* is_colorless_creature(int who_chooses, int player, int card)
{
	if( get_color (player, card) == 0 ){
		return NULL;
	}
	return "check only function";
}

int card_ghostfire_blade(int player, int card, event_t event){
	/*
	  Ghostfire Blade |1
	  Artifact - Equipment
	  Equipped creature gets +2/+2.
	  Equip {3}
	  Ghostfire Blade's equip ability costs {2} less to activate if it targets a colorless creature.
	*/

	if( event == EVENT_CAN_ACTIVATE ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.extra = (int32_t)is_colorless_creature;

		if( can_target(&td) ){
			return basic_equipment(player, card, event, 1);
		}
		else{
			return basic_equipment(player, card, event, 3);
		}
	}
	else if( event == EVENT_ACTIVATE){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = player;
			td.preferred_controller = player;

			card_instance_t *instance = get_card_instance(player, card);

			instance->number_of_targets = 0;

			if( new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				int to_eq_p = instance->targets[0].player;
				int to_eq_c = instance->targets[0].card;
				int amount = get_updated_equip_cost(player, card, (get_color(to_eq_p, to_eq_c) == 0 ? 1 : 3));
				charge_mana(player, COLOR_COLORLESS, amount);
				if( spell_fizzled != 1 ){
					instance->targets[0].player = to_eq_p;
					instance->targets[0].card = to_eq_c;
					instance->number_of_targets = 1;
				}
			}
	}
	else{
		return vanilla_equipment(player, card, event, 3, 2, 2, 0, 0);
	}
	return 0;
}

int card_heart_piercer_bow(int player, int card, event_t event){
	/*
	  Heart-Piercer Bow |2
	  Artifact - Equipment
	  Whenever equipped creature attacks, Heart-Piercer Bow deals 1 damage to target creature defending player controls.
	  Equip {1}
	*/
	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, instance->targets[8].player, instance->targets[8].card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;
			td.allow_cancel = 0;

			instance->number_of_targets = 0;

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				damage_target0(player, card, 1);
			}
		}
	}

	return basic_equipment(player, card, event, 1);
}
/*
Jeskai Banner	|3 --> Azorius Cluestone (same code for all Banners)
Artifact
{T}: Add {U}, {R}, or {W} to your mana pool.
{U}{R}{W}, {T}, Sacrifice Jeskai Banner: Draw a card.
*/

int card_lens_of_clarity(int player, int card, event_t event){
	/*
	  Lens of Clarity	|1
	  Artifact
	  You may look at the top card of your library and at face-down creatures you don't control. (You may do this at any time.)
	*/

	if (!IS_GAA_EVENT(event) || player == AI || is_humiliated(player, card) ){ //Obviously, there's no use for AI
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = (int32_t)is_face_down_creature;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.allowed_controller = 1-player;

	if( event == EVENT_CAN_ACTIVATE ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		card_instance_t *instance = get_card_instance(player, card);
		int choice = instance->number_of_targets = 0;
		if( can_target(&td) ){
			choice = do_dialog(player, player, card, -1, -1, " Check top card of deck\n Peek at face-down creature\n Cancel", 0);
		}
		if( choice == 0 ){
			show_deck( player, deck_ptr[player], 1, "Here's the first card of deck", 0, 0x7375B0 );
		}
		if( choice == 1 && new_pick_target(&td, "Select a face-down creature you don't control.", 0, 1 | GS_LITERAL_PROMPT) ){
			int true_identity = get_internal_card_id_from_csv_id( get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[12].player );
			reveal_card_iid(player, card, true_identity);
		}
		spell_fizzled = 1; //It's not really an activated ability
	}

	return 0;
}


/*
Mardu Banner	|3 --> Azorius Cluestone (same code for all Banners)
Artifact
{T}: Add {R}, {W}, or {B} to your mana pool.
{R}{W}{B}, {T}, Sacrifice Mardu Banner: Draw a card.

Sultai Banner	|3 --> Azorius Cluestone (same code for all Banners)
Artifact
{T}: Add {B}, {G}, or {U} to your mana pool.
{B}{G}{U}, {T}, Sacrifice Sultai Banner: Draw a card.

Temur Banner	|3 --> Azorius Cluestone (same code for all Banners)
Artifact
{T}: Add {G}, {U}, or {R} to your mana pool.
{G}{U}{R}, {T}, Sacrifice Temur Banner: Draw a card.
*/

int card_ugins_nexus(int player, int card, event_t event){
	/*
	  Ugin's Nexus	|5
	  Legendary Artifact
	  If a player would begin an extra turn, that player skips that turn instead.
	  If Ugin's Nexus would be put into a graveyard from the battlefield, instead exile it and take an extra turn after this one.
	*/
	check_legend_rule(player, card, event);

	int owner, position;
	if(this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position) ){
		rfg_card_from_grave(owner, position);
		time_walk_effect(player, card);
	}

	return 0;
}

int card_witness_of_the_ages(int player, int card, event_t event){
	/*
	  Witness of the Ages	|6
	  Artifact Creature - Golem 4/4
	  Morph {5} (You may cast this card face down as a 2/2 creature for {3}. Turn it face up any time for its morph cost.)
	*/
	return morph(player, card, event, MANACOST_X(5));
}
