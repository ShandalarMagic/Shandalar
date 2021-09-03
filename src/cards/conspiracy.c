#include "manalink.h"

// Functions
int will_of_the_council_impl(int player, int card, int voting_player, char pos1_description[100], char pos2_description[100], int priorities[2]){
	int votes = 1+count_cards_by_id(voting_player, CARD_ID_BRAGOS_REPRESENTATIVE);
	int result = 0;
	while( votes ){
			int tr = DIALOG(player, card, EVENT_ACTIVATE, DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_WHO_CHOOSES(voting_player),
							pos1_description, 1, priorities[0],
							pos2_description, 1, priorities[1]);
			if( tr == 1 ){
				SET_BYTE0(result)++;
			}
			else{
				SET_BYTE1(result)++;
			}
			votes--;
	}
	return result;
}

int will_of_the_council(int player, int card, char pos1_description[100], char pos2_description[100], int priorities[2]){
	int i;
	int result = 0;
	int p_votes[2] = {0, 0};
	for(i=0; i<2; i++){
		int p = i == 0 ? player : 1-player;
		int r2 = will_of_the_council_impl(player, card, p, pos1_description, pos2_description, priorities);
		if( BYTE0(r2)== 0 || BYTE1(r2)== 0 ){
			p_votes[p] = BYTE0(r2) > 0 ? 1 : 2;
		}
		SET_BYTE0(result)+=BYTE0(r2);
		SET_BYTE1(result)+=BYTE1(r2);
	}
	if( p_votes[0] && p_votes[1] && (p_votes[0] != p_votes[1]) ){
		lose_life(0, count_cards_by_id(1, CARD_ID_GRUDGE_KEEPER)*2);
		lose_life(1, count_cards_by_id(0, CARD_ID_GRUDGE_KEEPER)*2);
	}
	return result;
}

void multikicked_with_1_1_counters(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			get_card_instance(player, card)->info_slot = do_multikicker(player, card, event, colorless, black, blue, green, red, white);
		}
		if( get_card_instance(player, card)->info_slot > 0 && spell_fizzled != 1 ){
			enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_card_instance(player, card)->info_slot);
		}
	}
}

void dethrone(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && life[1-player] >= life[player] && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		add_1_1_counter(player, card);
	}
}

int parley(int player, int card){
	int result = 0;
	int i;
	for(i=0; i<2; i++){
		if( deck_ptr[i][0] != -1 ){
			show_deck( i, deck_ptr[i], 1, "Here's the first card of your deck", 0, 0x7375B0 );
			show_deck( 1-i, deck_ptr[i], 1, "Here's the first card of opponent's deck", 0, 0x7375B0 );
			result += ! is_what(-1, deck_ptr[i][0], TYPE_LAND);
		}
	}
	return result;
}


/*************
* Colorless	 *
*************/

int card_hidden_agenda(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
		if( instance->targets[12].card > -1 ){
			return instance->targets[12].card;
		}
	}
	if( player == AI ){
		if( instance->targets[2].card == CARD_ID_CONSPIRACY_BRAGOS_FAVOR ){
			if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
				get_id(affected_card_controller, affected_card) == instance->targets[2].card
			  ){
				COST_COLORLESS--;
			}
			if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, instance->targets[2].card, 0, -1, 0) ){
				true_transform(player, card);
			}
		}
		if( instance->targets[2].card == CARD_ID_CONSPIRACY_UNEXPECTED_POTENTIAL ){
			if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
				get_id(affected_card_controller, affected_card) == get_card_instance(player, card)->targets[2].card
			  ){
				null_casting_cost(affected_card_controller, affected_card);
				COST_COLORLESS+=get_cmc(affected_card_controller, affected_card);
			}
			if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, instance->targets[2].card, 0, -1, 0) ){
				true_transform(player, card);
			}
		}
		if( instance->targets[2].card == CARD_ID_CONSPIRACY_DOUBLE_STROKE || instance->targets[2].card == CARD_ID_CONSPIRACY_IMMEDIATE_ACTION ||
			instance->targets[2].card == CARD_ID_CONSPIRACY_ITERATIVE_ANALYSIS || instance->targets[2].card == CARD_ID_CONSPIRACY_MUZZIOS_PREPARATIONS ||
			instance->targets[2].card == CARD_ID_CONSPIRACY_SECRET_SUMMONING || instance->targets[2].card == CARD_ID_CONSPIRACY_SECRETS_OF_PARADISE
		){
			if( event == EVENT_CAST_SPELL && affected_card_controller == player &&
				get_id(affected_card_controller, affected_card) == instance->targets[2].card
			  ){
				true_transform(player, card);
			}
		}
	}
	else{
		if( event == EVENT_CAN_ACTIVATE ){
			return 99;
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			true_transform(instance->parent_controller, instance->parent_card);
			create_card_name_legacy(instance->parent_controller, instance->parent_card, instance->targets[13].card);
		}
	}
	return vanguard_card(player, card, event, 7, 20, 11);
}

int card_bragos_favor(int player, int card, event_t event){
	/*
	  Brago's Favor
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Spells with the chosen name you cast cost 1 less to cast.
	*/
	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
		get_id(affected_card_controller, affected_card) == get_card_instance(player, card)->targets[2].card
	  ){
		COST_COLORLESS--;
	}

	return 0;
}

int card_double_stroke(int player, int card, event_t event){
	/*
	  Double Stroke
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Whenever you cast an instant or sorcery spell with the chosen name, you may copy it. You may choose new targets for the copy.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_SPELL, 0, 0, 0, 0, 0, instance->targets[2].card, 0, -1, 0) ){
		copy_spell_from_stack(player, instance->targets[1].player, instance->targets[1].card);
	}

	return 0;
}

int card_immediate_action(int player, int card, event_t event){
	/*
	  Immediate Action
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Creatures you control with the chosen name have haste.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( affected_card_controller == player && in_play(affected_card_controller, affected_card_controller) &&
		get_id(affected_card_controller, affected_card_controller) == instance->targets[2].card
	  ){
		haste(affected_card_controller, affected_card_controller, event);
	}

	return 0;
}

int card_iterative_analysis(int player, int card, event_t event){
	/*
	  Iterative Analysis
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Whenever you cast an instant or sorcery spell with the chosen name, you may draw a card.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_SPELL, 0, 0, 0, 0, 0, instance->targets[2].card, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_muzzios_preparations(int player, int card, event_t event){
	/*
	  Muzzio's Preparations
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Each creature you control with the chosen name enters the battlefield with an additional +1/+1 counter on it.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affected_card_controller == player && get_id(affected_card_controller, affected_card) == instance->targets[2].card ){
		add_1_1_counter(affected_card_controller, affected_card);
	}

	return 0;
}

int card_secret_summoning(int player, int card, event_t event){
	/*
	  Secret Summoning
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Whenever a creature with the chosen name enters the battlefield under your control, you may search your library for any number of cards with that name, reveal them, put them into your hand, then shuffle your library.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_CREATURE, 0, 0, 0, 0, 0, instance->targets[2].card, 0, -1, 0) ){
		char msg[100];
		scnprintf(msg, 100, "Select a card named %s", cards_ptr[ instance->targets[2].card ]->name);
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, msg);
		while( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test) != -1 ){
		}
	}

	return 0;
}

int card_secret_of_paradise(int player, int card, event_t event){
	/*
	  Secrets of Paradise
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up at any time and reveal the chosen name.)
	  Creatures you control with the chosen name have "T: Add one mana of any color to your mana pool."
	*/
	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;
	td.illegal_abilities = 0;
	td.extra = instance->targets[2].card > -1 ? instance->targets[2].card : 0;

	// Not 'can_use_activated_abilities(player, card)' as this is usually a continuous ability, and cannot be disabled by cards like Pithing Needle and such
	if (event == EVENT_CAN_ACTIVATE && ! is_humiliated(player, card) ){
		return can_target(&td);
	}

	if (event == EVENT_ACTIVATE ){
		char msg[100];
		scnprintf(msg, 100, "Select a card named %s", cards_ptr[ instance->targets[2].card ]->name);
		if( select_target(player, card, &td, msg, NULL) ){
			instance->number_of_targets = 1;
			produce_mana_tapped_all_one_color(instance->targets[0].player, instance->targets[0].card, COLOR_TEST_ANY_COLORED, 1);
			if (!(instance->targets[0].player == player && instance->targets[0].card == card)){
				/* tap_card() would do this, but clear tapped_for_mana_color first.
				 * And if the card is tapping itself, the event will be dispatched later anyway, so don't double up here. */
				dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
			}
		}
		else {
			cancel = 1;
		}
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card)){
		/* As with permanents_you_control_can_tap_for_mana() above, this is inaccurate when one of the tappable permanents can itself produce mana, since it
		 * will declare mana too.  There's no good solution. */
		int count = target_available(player, card, &td);
		declare_mana_available_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, count);
	}

	return 0;
}

int card_unexpected_potential(int player, int card, event_t event){
	/*
	  Unexpected Potential
	  Conspiracy
	  Hidden agenda (Start the game with this conspiracy face down in the command zone and secretly name a card. You may turn this conspiracy face up any time and reveal the chosen name.)
	  You may spend mana as though it was mana of any color to cast spells with the chosen name.
	*/
	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
		get_id(affected_card_controller, affected_card) == get_card_instance(player, card)->targets[2].card
	  ){
		null_casting_cost(affected_card_controller, affected_card);
		COST_COLORLESS+=get_cmc(affected_card_controller, affected_card);
	}

	return 0;
}


/*

Advantageous Proclamation
Conspiracy
(Start the game with this conspiracy face up in the command zone.)
Your minimum deck size is reduced by five.

Backup Plan
Conspiracy
(Start the game with this conspiracy face up in the command zone.)
Draw an additional hand of seven cards as the game begins. Before taking mulligans, shuffle all but one of your hands into your library.

Power Play
Conspiracy
(Start the game with this conspiracy face up in the command zone.)
You are the starting player. If multiple players would be the starting player, one of those players is chosen at random.

Sentinel Dispatch
Conspiracy
(Start the game with this conspiracy face up in the command zone.)
At the beginning of the first upkeep, put a 1/1 colorless Construct artifact creature token with defender onto the battlefield.

Worldknit
Conspiracy
(Start the game with this conspiracy face up in the command zone.)
As long as every card in your card pool started the game in your library or in the command zone, lands you control have "T: Add one mana of any color to your mana pool."
*/

/*************
*	White	 *
*************/

/*
Brago's Representative 2W --> Vanilla
Creature - Human Advisor
While voting, you get an additional vote. (The votes can be for different choices or for the same choice.)
1/4
*/

int card_apex_hawks(int player, int card, event_t event){
	/*
	  Apex Hawks |2|w
	  Creature - Bird
	  Multikicker 1W
	  Flying
	  Apex Hawks enters the battlefield with a +1/+1 counter for each time it was kicked.
	*/
	multikicked_with_1_1_counters(player, card, event, MANACOST_XW(1, 1));
	return 0;
}

int card_councils_judgement(int player, int card, event_t event){

	// to insert
	// Council's Judgment 1WW
	// Sorcery
	// Will of the council - Starting with you, each player votes for a nonland permanent you don't control. Exile each permanent with the most votes or tied for most votes.

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		int permanents[active_cards_count[1-player]];
		int i;
		for(i=0; i<active_cards_count[1-player]; i++){
			permanents[i] = 0;
		}
		for(i=0; i<2; i++){
			int p = i == 0 ? player : 1-player;
			int voted = 0;
			td.who_chooses = p;
			int total_votes = 1+count_cards_by_id(p, CARD_ID_BRAGOS_REPRESENTATIVE);
			if( p == AI && player != p ){
				while( total_votes ){
						for(i=0; i<active_cards_count[1-player]; i++){
							if( p == 0 ){
								if( BYTE1(permanents[i]) ){
									SET_BYTE0(permanents[i])++;
									total_votes--;
								}
							}
							else{
								if( BYTE0(permanents[i]) ){
									SET_BYTE1(permanents[i])++;
									total_votes--;
								}
							}
							if( ! total_votes ){
								break;
							}
						}
				}
			}
			else{
				char msg[100];
				if( p == player ){
					strcpy(msg, "Select a nonland permanent your don't control.");
				}
				else{
					strcpy(msg, "Select a nonland permanent your control.");
				}
				while( voted < total_votes && can_target(&td) ){
						instance->number_of_targets = 0;
						if( new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT) ){
							if( p == 0 ){
								SET_BYTE0(permanents[instance->targets[0].card])++;
							}
							else{
								SET_BYTE1(permanents[instance->targets[0].card])++;
							}
							add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
						}
						voted++;
				}
			}
		}
		int max_votes = 0;
		int life_loss_player0 = 0;
		int life_loss_player1 = 0;
		for(i=0; i<active_cards_count[1-player]; i++){
			if( BYTE0(permanents[i]) != BYTE1(permanents[i]) && (BYTE0(permanents[i]) == 0 || BYTE1(permanents[i]) == 0) ){
				life_loss_player0 += (2*count_cards_by_id(1, CARD_ID_GRUDGE_KEEPER));
				life_loss_player1 += (2*count_cards_by_id(0, CARD_ID_GRUDGE_KEEPER));
			}
			if( BYTE0(permanents[i]) + BYTE1(permanents[i]) > max_votes ){
				max_votes = BYTE0(permanents[i]) + BYTE1(permanents[i]);
			}
		}
		for(i=active_cards_count[1-player]-1; i>-1;i--){
			if( permanents[i] > 0 ){
				if( BYTE0(permanents[i]) + BYTE1(permanents[i]) == max_votes ){
					kill_card(1-player, i, KILL_REMOVE);
				}
				else{
					remove_state(1-player, i, STATE_TARGETTED);
				}
			}
		}
		lose_life(0, life_loss_player0);
		lose_life(1, life_loss_player1);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*
Courier Hawk |1|W --> Serra Angel
Creature - Bird
Flying, vigilance
*/

int card_council_guardian(int player, int card, event_t event){
	// Council Guardian 5W
	// Creature - Giant Soldier
	// Will of the council - When Council Guardian enters the battlefield, starting with you, each player votes for blue, black, red, or green. Council Guardian gains protection from each color with the most votes or tied for most votes.
	// 5/5
	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result |= get_card_instance(player, card)->info_slot;
	}

	if( comes_into_play(player, card, event) ){
		int clrs[4] = {0, 0, 0, 0};
		int cp[2][4];
		int p_votes[2][4];
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		int i;
		for(i=0; i<2; i++){
			int k;
			for(k=1; k<5; k++){
				test.color = 1<<k;
				cp[i][k-1] = check_battlefield_for_special_card(player, card, i, CBFSC_GET_COUNT, &test);
			}
		}
		for(i=0; i<2; i++){
			int p = i == 0 ? player : 1-player;
			int votes = 1+count_cards_by_id(p, CARD_ID_BRAGOS_REPRESENTATIVE);
			while( votes ){
					int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_WHO_CHOOSES(p),
										"Black", 1, p == player ? 10+(5*cp[p][0]) : 50-(5*cp[p][0]),
										"Blue", 1, p == player ? 10+(5*cp[p][1]) : 50-(5*cp[p][1]),
										"Green", 1, p == player ? 10+(5*cp[p][2]) : 50-(5*cp[p][2]),
										"Red", 1, p == player ? 10+(5*cp[p][3]) : 50-(5*cp[p][3]));
					p_votes[p][choice-1]++;
					clrs[choice-1]++;
					votes--;
			}
		}
		int max_votes = 0;
		for(i=1; i<5; i++){
			if( clrs[i-1] > max_votes){
				max_votes = clrs[i-1];
			}
		}
		for(i=1; i<5; i++){
			if( clrs[i-1] >= max_votes){
				get_card_instance(player, card)->info_slot |= 1<<(10+i);
			}
		}
		for(i=0; i<4; i++){
			if( (p_votes[0][i] == 0 || p_votes[1][i] == 0) && p_votes[0][i] != p_votes[1][i] ){
				lose_life(current_turn, count_cards_by_id(1-current_turn, CARD_ID_GRUDGE_KEEPER)*2);
				lose_life(1-current_turn, count_cards_by_id(current_turn, CARD_ID_GRUDGE_KEEPER)*2);
				break;
			}
		}
	}
	return 0;
}

int card_custodi_soulbinders(int player, int card, event_t event){
	/*
	  Custodi Soulbinders |3|W
	  Creature - Human Cleric
	  Custodi Soulbinders enters the battlefield with X +1/+1 counters on it, where X is the number of other creatures on the battlefield.
	  2W, remove a +1/+1 counter from Custodi Soulbinders: Put a 1/1 white Spirit creature token with flying onto the battlefield.
	*/
	if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		int amount = check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &this_test);
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, amount);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = 2;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(2,1), GVC_COUNTER(GAA_1_1_COUNTER), NULL, NULL);
}

int card_custodi_squire(int player, int card, event_t event){
	/*
	  Custodi Squire |4|W
	  Creature - Spirit Cleric
	  Flying
	  Will of the council - When Custodi Squire enters the battlefield, starting with you,
	  each player votes for an artifact, creature, or enchantment card in your graveyard.
	  Return each card with the most votes or tied for the most votes to your hand.
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_ENCHANTMENT, "Select an artifact, creature, or enchantment card");
		if( new_special_count_grave(player, &test) ){
			int selected[2] = {-1, -1};
			int votes[2] = {1+count_cards_by_id(player, CARD_ID_BRAGOS_REPRESENTATIVE), 1+count_cards_by_id(1-player, CARD_ID_BRAGOS_REPRESENTATIVE)};
			APNAP(p, {selected[p] = new_select_a_card(p, player, TUTOR_FROM_GRAVE, 1, p == player ? AI_MAX_VALUE : AI_MIN_VALUE, -1, &test);};);
			int max_votes = votes[0] > votes[1] ? votes[0] : votes[1];
			int i;
			for(i=0; i<2; i++){
				if( votes[i] >= max_votes ){
					from_grave_to_hand(player, selected[i], TUTOR_HAND);
				}
			}
			if( selected[0] != selected[1] ){
				lose_life(current_turn, count_cards_by_id(1-current_turn, CARD_ID_GRUDGE_KEEPER)*2);
				lose_life(1-current_turn, count_cards_by_id(current_turn, CARD_ID_GRUDGE_KEEPER)*2);
			}
		}
	}
	return 0;
}

int card_guardian_zendikon(int player, int card, event_t event ){
	/*
	  Guardian Zendikon |2|W
	  Enchantment - Aura
	  Enchant land
	  Enchanted land is a 2/6 white Wall creature with defender. It's still a land.
	  When enchanted land dies, return that card to its owner's hand.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	immortal_enchantment(player, card, event);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_SET_COLOR && affect_me(p, c) ){
			event_result |= COLOR_TEST_WHITE;
		}

		if( leaves_play(player, card, event) ){
			reset_subtypes(p, c, 2);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WALL);
		}
	}


	return generic_animating_aura(player, card, event, &td, "TARGET_LAND", 2, 6, KEYWORD_DEFENDER, 0);
}

int card_rousing_of_souls(int player, int card, event_t event){
	// Rousing of Souls 2W
	// Sorcery
	// Parley - Each player reveals the top card of his or her library. For each nonland card revealed this way, you put a 1/1 white Spirit creature token with flying onto the battlefield. Then each player draws a card.

	if( event == EVENT_RESOLVE_SPELL ){
		int spirits = parley(player, card);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = spirits;
		generate_token(&token);

		draw_cards(player, 1);
		draw_cards(1-player, 1);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_unquestioned_authority(int player, int card, event_t event){
	/*
	  Unquestioned Authority |2|W
	  Enchantment - Aura
	  Enchant creature
	  When Unquestioned Authority enters the battlefield, draw a card.
	  Enchanted creature has protection from creatures.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, 1);
		}
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		protection_from_creatures(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}
/*************
* 	Blue	 *
*************/

int card_academy_elite(int player, int card, event_t event){
	/*
	  Academy Elite 3U
	  Creature - Human Wizard
	  Academy Elite enters the battlefield with X +1/+1 counters on it, where X is the number of instant and sorcery cards in all graveyards.
	  2U, Remove a +1/+1 counter from Academy Elite: Draw a card, then discard a card.
	  0/0
	  reprint
	*/
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, count_graveyard_by_type(player, TYPE_SPELL));

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}
	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_XU(2, 1), 0, NULL, NULL);
}

static int basic_land_type_added(int player, int card, event_t event){
	// targets[1].player = mode
	// targets[1].card = color of mana added
	card_instance_t *instance = get_card_instance(player, card);
	int p = is_what(player, card, TYPE_EFFECT | TYPE_ENCHANTMENT) ? instance->damage_target_player : player;
	int c = is_what(player, card, TYPE_EFFECT | TYPE_ENCHANTMENT) ? instance->damage_target_card : card;
	if( p > -1 ){
		if( event == EVENT_CAN_ACTIVATE && ! is_tapped(p, c) && ! is_animated_and_sick(p, c) ){
			return can_produce_mana(p, c);
		}
		if( event == EVENT_ACTIVATE ){
			tap_card(p, c);
			int clr = COLOR_BLACK;
			if( ! check_special_flags2(p, c, SF2_CONTAMINATION) ){
				clr = instance->targets[1].card;
			}
			produce_mana(p, clr, 1);
		}
		if( event == EVENT_COUNT_MANA && affect_me(p, c) && ! is_tapped(p, c) && ! is_animated_and_sick(p, c) && can_produce_mana(p, c) ){
			int clr = COLOR_BLACK;
			if( ! check_special_flags2(p, c, SF2_CONTAMINATION) ){
				clr = instance->targets[1].card;
			}
			declare_mana_available(p, clr, 1);
		}
		if( instance->targets[1].player > 0 && (instance->targets[1].player & 1) && eot_trigger(player, card, event) ){
			reset_subtypes(p, c, 2);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

static int add_a_basic_land_type(int player, int card, int t_player, int t_card, int mode){
	int land_types[5] = {SUBTYPE_SWAMP, SUBTYPE_ISLAND, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN, SUBTYPE_PLAINS};
	int choice = do_dialog(player, player, card, player, card, " Swamp\n Island\n Forest\n Mountain\n Plains", get_deck_color(player, player));
	add_a_subtype(t_player, t_card, land_types[choice]);
	int legacy = create_targetted_legacy_activate(player, card, &basic_land_type_added, t_player, t_card);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[1].player = mode;
	instance->targets[1].card = 1+choice;
	return legacy;
}


int card_grixis_illusionist(int player, int card, event_t event, int clr, int pow, int tou, int key, int s_key){
	/*
	  Grixis Illusionist |U REPRINT
	  Creature - Human Wizard
	  T: Target land you control becomes the basic land type of your choice until end of turn.
	  1/1
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		add_a_basic_land_type(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_LAND");
}

int card_enclave_elite(int player, int card, event_t event){
	/*
	  Enclave Elite |2|U REPRINT
	  Creature - Merfolk Soldier
	  Multikicker 1U
	  Islandwalk
	  Enclave Elite enters the battlefield with a +1/+1 counter on it for each time it was kicked.
	*/
	multikicked_with_1_1_counters(player, card, event, MANACOST_XU(1, 1));
	return 0;
}

static int counter_first_spell_ability_that_targets_me_every_turn(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
			int trig = 0;
			if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
				card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
				int i;
				for(i=0; i<instance->number_of_targets; i++){
					if( instance->targets[i].card == card && instance->targets[i].player == player &&
						! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
					  ){
						trig = 1;
						break;
					}
				}
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
						int i;
						for(i=0; i<instance->number_of_targets; i++){
							if( instance->targets[i].card == card && instance->targets[i].player == player ){
								set_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER);
							}
						}
						real_counter_a_spell(player, card, trigger_cause_controller, trigger_cause);
				}
			}
		}

		/*
		if ( stack_size >= 1 ){
			if( stack_data[stack_size - 1].generating_event != EVENT_RESOLVE_TRIGGER &&
				cards_data[stack_data[stack_size - 1].internal_card_id].id != 904
			  ){
				int trig = 0;
				card_instance_t *instance = get_card_instance(stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card);
				int i;
				for(i=0; i<instance->number_of_targets; i++){
					if( instance->targets[i].card == card && instance->targets[i].player == player &&
						! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
					 ){
						trig = 1;
						break;
					}
				}
				if( trig == 1 ){
					for(i=0; i<instance->number_of_targets; i++){
						if( instance->targets[i].card == card && instance->targets[i].player == player &&
							! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
						 ){
							set_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER);
						}
					}
					instance->internal_card_id = -1;
				}
			}
		}
		*/

		if( event == EVENT_CLEANUP ){
			remove_special_flags2(player, card, SF2_KIRA_GREAT_GLASS_SPINNER);
		}
	}
	return 0;
}

int card_jetting_glasskite(int player, int card, event_t event){
	/*
	  Jetting Glasskite |4|U|U REPRINT
	  Creature - Spirit
	  Flying
	  Whenever Jetting Glasskite becomes the target of a spell or ability for the first time in a turn, counter that spell or ability.
	  4/4
	*/
	counter_first_spell_ability_that_targets_me_every_turn(player, card, event);
	return 0;
}

int card_minamo_scrollkeeper(int player, int card, event_t event){
	/*
	  Minamo Scrollkeeper |1|U REPRINT
	  Creature - Human Wizard
	  Defender
	  Your maximum hand size is increased by one.
	  2/3
	*/
	if( ! is_humiliated(player, card) && event == EVENT_MAX_HAND_SIZE && current_turn == player ){
		event_result++;
	}

	return 0;
}

int card_marchesas_emissary(int player, int card, event_t event){
	/*
	  Marchesa's Emissary |3|U
	  Creature - Human Rogue
	  Hexproof
	  Dethrone
	  2/2

	*/
	hexproof(player, card, event);
	dethrone(player, card, event);
	return 0;
}

int card_marchesas_infiltrator(int player, int card, event_t event){
	/*
	  Marchesa's Infiltrator 2U
	  Creature - Human Rogue
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  Whenever Marchesa's Infiltrator combat damage to a player, draw a card.
	  1/1
	*/
	dethrone(player, card, event);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_muzzio_visionary_architect(int player, int card, event_t event){
	/*
	  Muzzio, Visionary Architect 1UU
	  Legendary Creature - Human Artificer
	  3U, T: Look at the top X cards of your library, where X is the highest converted mana cost among artifacts you control.
	  You may reveal an artifact card from among them and put it onto the battlefield. Put the rest on the bottom of your library in any order.
	  1/3
	*/

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_MAX_CMC, &this_test);
		if( amount > 0 ){
			int result = reveal_x_and_choose_a_card_type(player, card, amount, TYPE_ARTIFACT);
			if( result != -1 ){
				put_into_play(player, result);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(3, 1), 0, NULL, NULL);
}

int card_plea_for_power(int player, int card, event_t event){
	/*
	  Plea for Power 3U
	  Sorcery
	  Will of the council - Starting with you, each player votes for time or knowledge. If time gets more votes, take an extra turn after this one.
	  If knowledge gets more votes or the vote is tied, draw three cards.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		char pos1[100] = "Time (Time Walk)";
		char pos2[100] = "Knowledge (Ancestral Recall)";
		int vote[2] = {0, 0};

		int ai_priorities[2] = {player == AI ? 50 : 0, player == AI ? 0 : 50};
		int result = will_of_the_council(player, card, pos1, pos2, ai_priorities);
		vote[0]+=BYTE0(result);
		vote[1]+=BYTE1(result);

		if( vote[0] > vote[1] ){
			return card_time_walk(player, card, event);
		}
		draw_cards(player, 3);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_screaming_seahawk(int player, int card, event_t event){
	/*
	  Screaming Seahawk |4|U REPRINT
	  Creature - Bird
	  Flying
	  When Screaming Seahawk enters the battlefield, you may search your library for a card named Screaming Seahawk, reveal it, and put it into your hand. If you do, shuffle your library.
	  2/2
	*/
	/* Daru Cavalier	|3|W
	 * Creature - Human Soldier 2/2
	 * First strike
	 * When ~ enters the battlefield, you may search your library for a card named ~, reveal it, and put it into your hand. If you do, shuffle your library. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "");
		this_test.id = get_id(player, card);
		scnprintf(this_test.message, 100, "Select a card named %s.", cards_ptr[this_test.id]->name);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
	}

	return 0;
}

int card_split_decision(int player, int card, event_t event){
	/*
	  Split Decision 1U
	  Instant
	  Will of the council - Choose target instant or sorcery spell. Starting with you, each player votes for denial or duplication.
	  If denial gets more votes, counter that spell. If duplication gets more votes or the vote is tied, copy the spell. You may choose new targets for the copy.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.preferred_controller = player;
		if( counterspell_validate(player, card, &td, 0) ){
			char pos1[100] = "Denial (Counterspell)";
			char pos2[100] = "Duplication (Twincast)";
			int vote[2] = {0, 0};
			card_instance_t *instance = get_card_instance( player, card);

			int ai_priorities[2] = {AI == instance->targets[0].player ? 0 : 50, AI == instance->targets[0].player ? 50 : 0};
			int result = will_of_the_council(player, card, pos1, pos2, ai_priorities);
			vote[0]+=BYTE0(result);
			vote[1]+=BYTE1(result);

			if( vote[0] > vote[1] ){
				real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.preferred_controller = player;
		return counterspell(player, card, event, &td, 0);
	}

	return 0;
}

int card_stasis_cell(int player, int card, event_t event){
	/*
	  Stasis Cell |4|D REPRINT
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature doesn't untap during its controller's untap step.
	  3U: Attach Stasis Cell to target creature.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		does_not_untap(p, c, event);
		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, MANACOST_XU(3, 1), 0, NULL, NULL) ){
				return call_card_function(player, card, EVENT_CAN_MOVE_AURA);
			}
		}
		if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_XU(3, 1)) ){
				return call_card_function(player, card, EVENT_MOVE_AURA);
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			return call_card_function(player, card, EVENT_RESOLVE_MOVING_AURA);
		}
	}
	return disabling_aura(player, card, event);
}

int card_travellers_cloak(int player, int card, event_t event){
	/*
	  Traveler's Cloak |2|U REPRINT
	  Enchantment - Aura
	  Enchant creature
	  As Traveler's Cloak enters the battlefield, choose a land type.
	  When Traveler's Cloak enters the battlefield, draw a card.
	  Enchanted creature has landwalk of the chosen type.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, 1);
			instance->info_slot = 1<<(choose_a_color(player, get_deck_color(player, 1-player))-1);
		}
	}
	return generic_aura(player, card, event, player, 0, 0, instance->info_slot, 0, 0, 0, 0);
}

int card_wind_dancer(int player, int card, event_t event){
	/*
	  Wind Dancer |1|U REPRINT
	  Creature - Faerie
	  Flying
	  T: Target creature gains flying until end of turn.
	  1/1
	*/
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
									0, 0, KEYWORD_FLYING, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

/*************
* 	Black	 *
*************/

int card_bite_of_the_black_rose(int player, int card, event_t event){
	/*
	  Bite of the Black Rose |3|B
	  Sorcery
	  Will of the Council - Starting with you, each player votes for sickness or psychosis.
	  If sickness gets more votes, creatures your opponents control get -2/-2 until end of turn.
	  If psychosis gets more votes or the vote is tied, each opponent discards two cards.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		char pos1[100] = "Sickness (Infest on opponent)";
		char pos2[100] = "Psychosis (Mind Rot on opponent)";
		int vote[2] = {0, 0};

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.toughness = 3;
		this_test.toughness_flag = F5_TOUGHNESS_LESSER_THAN_VALUE;
		int amount = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);

		int ai_priorities[2] = { player == AI && amount > 1 ? 50 : 0, !(player == AI && amount > 1) ? 50 : 0};
		int result = will_of_the_council(player, card, pos1, pos2, ai_priorities);
		vote[0]+=BYTE0(result);
		vote[1]+=BYTE1(result);

		if( vote[0] > vote[1] ){
			pump_subtype_until_eot(player, card, 1-player, -1, -2, -2, 0, 0);
		}
		else{
			new_multidiscard(1-player, 2, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_drakestown_forgotten(int player, int card, event_t event){
	/*
	  Drakestown Forgotten 4B
	  Creature - Zombie
	  Drakestown Forgotten enters the battlefield with X +1/+1 counters on it, where X is the number of creature cards in all graveyards.
	  2B, Remove a +1/+1 counter from Drakestown Forgotten: Target creature gets -1/-1 until end of turn.
	  0/0
	*/
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, count_graveyard_by_type(2, TYPE_CREATURE));

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER | GAA_CAN_TARGET, MANACOST_XB(2,1), 0, &td, "TARGET_CREATURE");
}

/*
Grudge Keeper 1B --> Vanilla
Creature - Zombie Wizard
Whenever players finish voting, each opponent who voted for a choice you didn't vote for loses 2 life.
2/1
*/

int card_infectious_horror(int player, int card, event_t event){
	/*
	  Infectious Horror |3|B REPRINT
	  Creature - Zombie Horror
	  Whenever Infectious Horror attacks, each opponent loses 2 life.
	  2/2
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		lose_life(1-player, 2);
	}

	return 0;
}

int card_necromantic_thirst(int player, int card, event_t event){
	/*
	  Necromantic Thirst |2|B|B REPRINT
	  Enchantment - Aura
	  Enchant creature
	  Whenever enchanted creature deals combat damage to a player, you may return target creature card from your graveyard to your hand.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( damage_dealt_by_me_arbitrary(instance->damage_target_player, instance->damage_target_card, event,
			DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER, player, card)
		  ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(2) ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_quag_vampires(int player, int card, event_t event){
	/*
	  Quag Vampires |B
	  reature - Vampire Rogue
	  Multikicker 1B
	  Swampwalk
	  Quag Vampires enters the battlefield with a +1/+1 counter on it for each time it was kicked.
	  1/1
	*/
	multikicked_with_1_1_counters(player, card, event, MANACOST_XB(1, 1));
	return 0;
}

int card_reign_of_the_pit(int player, int card, event_t event){
	/*
	  Reign of the Pit 4BB
	  Sorcery
	  Each player sacrifices a creature.
	  Put an X/X black Demon creature token with flying onto the battlefield, where X is the total power of the creatures sacrificed this way.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 0;
		int i;
		for(i=0; i<2; i++){
			int p = i == 0 ? player : 1-player;
			impose_sacrifice(player, card, p, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			amount+=get_card_instance(player, card)->targets[2].player;
		}
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DEMON, &token);
		token.pow = amount;
		token.tou = amount;
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_stronghold_discipline(int player, int card, event_t event){
	/*
	  Stronghold Discipline |2|B|B REPRINT
	  Sorcery
	  Each player loses 1 life for each creature he or she controls.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int p = i == 0 ? player : 1-player;
			lose_life(p, count_subtype(p, TYPE_CREATURE, -1));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_tyrants_choice(int player, int card, event_t event){
	/*
	  Tyrant's Choice 1B
	  Sorcery
	  Will of the council - Starting with you, each player votes for death or torture.
	  If death gets the most votes, each opponent sacrifices a creature.
	  If torture gets the most votes or the vote is tied, each opponent loses 4 life.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		char pos1[100] = "Death (Diabolic Edict on opponent)";
		char pos2[100] = "Torture (opponent loses 4 life)";
		int vote[2] = {0, 0};

		int ai_priorities[2] = { life[AI]-4 < 6 ? 50 : 0, life[AI]-4 > 5 ? 50 : 0};
		int result = will_of_the_council(player, card, pos1, pos2, ai_priorities);
		vote[0]+=BYTE0(result);
		vote[1]+=BYTE1(result);

		if( vote[0] > vote[1] ){
			impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else{
			lose_life(1-player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*************
* 	Red	     *
*************/

static int reset_forced_subtype_at_eot(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( eot_trigger(player, card, event) ){
			reset_subtypes(instance->damage_target_player, instance->damage_target_card, 1);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_boldwyr_intimidator(int player, int card, event_t event){
	/*
	  Boldwyr Intimidator |5|R|R REPRINT
	  Creature - Giant Warrior
	  Cowards can't block warriors
	  R: Target creature becomes a Coward until end of turn.
	  2R: Target creature becomes a Warrior until end of turn.
	  5/5
	*/

	if( event == EVENT_BLOCK_LEGALITY ){
		if( has_subtype(attacking_card_controller, attacking_card, SUBTYPE_WARRIOR) &&
			has_subtype(affected_card_controller, affected_card, SUBTYPE_COWARD)
		  ){
			event_result = 1;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_subtype = SUBTYPE_COWARD;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	if( player == AI ){
		td.required_subtype = SUBTYPE_WARRIOR;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	}

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE") ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(2, 1), 0, &td1, "TARGET_CREATURE") ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE") ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XR(2, 1), 0, &td1, "TARGET_CREATURE") ){
				choice = do_dialog(player, player, card, -1, -1, " Turn target in Coward\n Turn target in Warrior\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2*choice, 1)) ){
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
			}
			if( choice == 1 ){
				if( pick_target(&td1, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( (instance->info_slot == 66 && valid_target(&td)) ||(instance->info_slot == 67 && valid_target(&td1)) ){
			int sta = instance->info_slot == 66 ? SUBTYPE_COWARD : SUBTYPE_WARRIOR;
			force_a_subtype(instance->targets[0].player, instance->targets[0].card, sta);
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &reset_forced_subtype_at_eot,
											instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_deathforge_shaman(int player, int card, event_t event){
	/*
	  Deathforge Shaman |4|R REPRINT
	  Creature - Ogre Shaman
	  Multikicker R
	  When Deathforge Shaman enters the battlefield, it deals damage to target player equal to twice the number of times it was kicked.
	  4/3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	multikicker(player, card, event, MANACOST_R(1));

	if( comes_into_play(player, card, event) && kicked(player, card) && instance->info_slot > 0){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		td1.allow_cancel = 0;

		if( can_target(&td1) && pick_target(&td1, "TARGET_PLAYER")  ){
			damage_player(instance->targets[0].player, 2*instance->info_slot, player, card);
		}
	}

	return 0;
}

int card_enraged_evolutionary(int player, int card, event_t event){
	/*
	  Enraged Revolutionary |2|R
	  Creature - Human Warrior
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  2/1

	*/
	dethrone(player, card, event);
	return 0;
}

int card_flowstone_blade(int player, int card, event_t event){
	/*
	  Flowstone Blade |R REPRINT
	  Enchantment - Aura
	  Enchant creature
	  R: Enchanted creature gets +1/-1 until end of turn.
	*/

	if (IS_GAA_EVENT(event)){
		card_instance_t* instance = get_card_instance(player, card);

		if( instance->damage_target_player > -1 ){
			if( event == EVENT_RESOLVE_ACTIVATION ){
				pump_until_eot_merge_previous(player, card,
											  instance->damage_target_player, instance->damage_target_card, 1, -1);
			}
			return generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
		}
	}

	return vanilla_aura(player, card, event, player);
}

/*
Grenzo's Cutthroat |1|R  --> Enraged Evolutionary
Creature - Goblin Rogue
First Strike
Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
1/1
*/

static const char* already_chosen(int who_chooses, int player, int card){

	if( ! check_state(player, card, STATE_TARGETTED) ){
		return NULL;
	}

	return "you've already chosen this.";
}

int card_grenzos_rebuttal(int player, int card, event_t event){
	/*
	  Grenzo's Rebuttal |4|R|R
	  Sorcery
	  Put a 4/4 red Ogre creature token onto the battlefield.
	  Starting with you, each player chooses an artifact, a creature, and a land from among the permanents controlled by the player to his or her left.
	  Destroy each permanent chosen this way.
	*/

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OGRE, &token);
		token.pow = 4;
		token.tou = 4;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);

		APNAP(p,{
					target_definition_t td;
					int types[3] = {TYPE_CREATURE, TYPE_ARTIFACT, TYPE_LAND};
					int k;
					for(k=0; k<3; k++){
						default_target_definition(player, card, &td, types[k]);
						td.who_chooses = p;
						td.allowed_controller = 1-p;
						td.preferred_controller = 1-p;
						td.illegal_abilities = 0;
						td.allow_cancel = 0;
						td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
						td.extra = (int32_t)already_chosen;
						char buffer[100];
						int pos = scnprintf(buffer, 100, " Select a");
						if( k == 0 ){
							scnprintf(buffer + pos, 100-pos, " creature.");
						}
						if( k == 1 ){
							scnprintf(buffer + pos, 100-pos, "n artifact.");
						}
						if( k == 2 ){
							scnprintf(buffer + pos, 100-pos, " land.");
						}
						if( can_target(&td) && select_target(player, card, &td, buffer, &(instance->targets[0])) ){
							instance->number_of_targets = 1;
							add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
						}
					}
				};
		);
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.state = STATE_TARGETTED;
		new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_ignition_team(int player, int card, event_t event){
	/*
	  Ignition Team |5|R|R
	  Creature - Goblin Warrior
	  Ignition Team enters the battlefield with X +1/+1 counters on it, where X i the number of tapped lands on the battlefield.
	  2R, Remove a +1/+1 counter from Ignition Team. Target land becomes a 4/4 red Elemental creature until end of turn. It's still a land.
	  0/0
	*/
	if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.state = STATE_TAPPED;
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &this_test));
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELEMENTAL);
		land_animation2(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 4, 4, 0, 0, 0, NULL);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER | GAA_CAN_TARGET, MANACOST_XR(2,1), 0, &td, "TARGET_LAND");
}

/*
Lizard Warrior |3|R REPRINT
Creature - Lizard Warrior
4/2
*/

int card_mana_geyser(int player, int card, event_t event){
	/*
	  Mana Geyser |3|R|R REPRINT
	  Sorcery
	  Add R to your mana pool for each tapped land your opponents control.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.state = STATE_TAPPED;
		produce_mana(player, COLOR_RED, check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &this_test));
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_orcish_cannonade(int player, int card, event_t event){
	/*
	  Orcish Cannonade |1|R|R REPRINT
	  Instant
	  Orcish Cannonade deals 2 damage to target creature or player and 3 damage to you.
	  Draw a card.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		damage_target0(player, card, 2);
		damage_player(player, 3, player, card);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

/*
Power of Fire |1|R REPRINT --> Hermetic Study
Enchantment - Aura
Enchant creature
Enchanted creatures has "T: This creature deals 1 damage to target creature or player."
*/

int card_scourge_of_the_throne(int player, int card, event_t event){
	/*
	  Scourge of the Throne 4RR
	  Creature - Dragon
	  Flying
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  Whenever Scourge of the Throne attacks for the first time each turn, if it's attacking the player with the most life or tied for most life, untap all attacking creatures. After this phase, there is an additional combat phase.
	  5/5
	*/
	card_instance_t* instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) && life[1-player] >= life[player] && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		add_1_1_counter(player, card);
		instance->info_slot++;
		if (instance->info_slot == 1 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_ATTACKING;
			new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);

			create_legacy_effect(player, card, &finest_hour_legacy);
		}

	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_skitter_of_the_lizard(int player, int card, event_t event){
	/*
	  Skitter of Lizards |R REPRINT
	  Creature - Lizard
	  Multikicker 1R
	  Haste
	  Skitter of Lizards enters the battlefield with a +1/+1 counter on it for each time it was kicked.
	  1/1
	*/
	haste(player, card, event);
	multikicked_with_1_1_counters(player, card, event, MANACOST_XR(1, 1));
	return 0;
}

int card_treasonous_ogre(int player, int card, event_t event){
	/*
	  Treasonous Ogre 3R
	  Creature - Ogre Shaman
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  Pay 3 life: Add R to your mana pool.
	  2/3
	*/
	dethrone(player, card, event);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_pay_life(player, 3) && can_use_activated_abilities(player, card) ){
		declare_mana_available(player, COLOR_RED, 3);
	}

	if( event == EVENT_CAN_ACTIVATE && can_pay_life(player, 3) && can_use_activated_abilities(player, card) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		lose_life(player, 3);
		produce_mana(player, COLOR_RED, 1);
	}

	return 0;
}

int card_uncontrollable_anger(int player, int card, event_t event){
	/*
	  Uncontrollable Anger |2|R|R REPRINT
	  Enchantment - Aura
	  Flash
	  Enchant creature
	  Enchanted creature gets +2/+2 and attacks each turn if able.
	*/
	return generic_aura(player, card, event, player, 2, 2, 0, SP_KEYWORD_MUST_ATTACK, 0, 0, 0);
}

int card_vent_sentinel(int player, int card, event_t event){
	/*
	  Vent Sentinel |3|R REPRINT
	  Creature - Elemental
	  Defender
	  1R,T: Vent Sentinel deals damage to target player equal to the number of creatures with defender you control.
	  2/4
	*/
	cannot_attack(player, card, event);

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, count_defenders(get_card_instance(player, card)->parent_controller));
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, "TARGET_PLAYER");
}

int card_wrap_in_flames(int player, int card, event_t event){
	/*
	  Wrap in Flames |3|R REPRINT
	  Sorcery
	  Wrap in Flames deals 1 damage to each of up to three target creatures. Those creatures can't block this turn.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL ){
		int i;
		for (i = 0; i < instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
				damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

/*************
* 	Green    *
*************/

int card_predators_howl(int player, int card, event_t event){
	/*
	  Predator's Howl |3|G
	  Instant
	  Put a 2/2 green Wolf creature token onto the battlefield.
	  Morbid - Put three 2/2 green Wolf creature tokens onto the battlefield instead if a creature died this turn.
	*/

	if (event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WOLF, &token);
		token.pow = 2;
		token.tou = 2;
		token.color_forced = COLOR_TEST_GREEN;
		token.qty = morbid() ? 3 : 1;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_realm_seeker(int player, int card, event_t event){
	/*
	  Realm Seekers 4GG
	  Creature - Elf Scout
	  Realm Seekers enters the battlefield with X +1/+1 counters on it, where X is the total number of cards in all players' hands.
	  2G, Remove a +1/+1 counter from Realm Seekers: Search your library for a land card, reveal it, put it into your hand, then shuffle your library.
	  0/0
	*/
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, hand_count[player]+hand_count[1-player]);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a land card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_XG(2,1), 0, NULL, NULL);
}

int card_selvalas_charge(int player, int card, event_t event){
	/*
	  Selvala's Charge 4G
	  Sorcery
	  Parley - Each player reveals the top card of his or her library.
	  For each nonland card revealed this way, you put a 3/3 green Elephant creature token onto the battlefield. Then each player draws a card.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int spirits = parley(player, card);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEPHANT, &token);
		token.pow = 3;
		token.tou = 3;
		token.color_forced = COLOR_TEST_GREEN;
		token.qty = spirits;
		generate_token(&token);

		draw_cards(player, 1);
		draw_cards(1-player, 1);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_selvalas_enforcer(int player, int card, event_t event){
	/*
	  Selvala's Enforcer |3|G
	  Creature - Elf Warrior
	  Parley - When Selvala's Enforcer enters the battlefield, each player reveals the top card of his or her library.
	  For each nonland card revealed this way, put a +1/+1 counter on Selvala's Enforcer. Then each player draws a card.
	  2/2
	*/
	if( comes_into_play(player, card, event) ){
		int spirits = parley(player, card);

		add_1_1_counters(player, card, spirits);

		draw_cards(player, 1);
		draw_cards(1-player, 1);

	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_charging_rhino(int player, int card, event_t event){
	/*
	  Charging Rhino |3|G|G REPRINT
	  Creature - Rhino
	  Charging Rhino can't be blocked by more than one creature.
	  4/4
	*/
	if( event == EVENT_DECLARE_BLOCKERS ){
		// If there are fewer than 3 creatures blocking me, then no one is
		// blocking me
		int block_count = 0;
		int count = 0;
		while(count < active_cards_count[1-player]){
			if(in_play(1-player, count) ){
				card_instance_t *instance = get_card_instance( 1-player, count);
				if( instance->blocking == card ){
					block_count++;
				}
			}
			count++;
		}

		if( block_count > 1 ){
			count = 0;
			while(count < active_cards_count[1-player]){
				if(in_play(1-player, count) ){
					card_instance_t *instance = get_card_instance( 1-player, count);
					if( instance->blocking == card ){
						instance->blocking = 255;
						instance->state &= ~(STATE_BLOCKED|STATE_BLOCKING);
					}
				}
				count++;
			}
		}
	}
	return 0;
}

int card_gnarlid_pack(int player, int card, event_t event){
	/*
	  Gnarlid Pack |1|G REPRINT
	  Creature - Beast
	  Multikicker 1G (You may pay an additional 1G any number of times as you cast this spell.)
	  Gnarlid Pack enters the battlefield with a +1/+1 counter on it for each time it was kicked.
	  2/2
	*/
	multikicked_with_1_1_counters(player, card, event, MANACOST_XG(1, 1));
	return 0;
}

int card_provoke(int player, int card, event_t event){
	/*
	  Provoke |1|G REPRINT
	  Instant
	  Untap target creature you don't control. That creature blocks this turn if able.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	if (event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		untap_card(instance->targets[0].player, instance->targets[0].card);
		pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_BLOCK);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*
Sporecap Spider |2|G REPRINT --> vanilla
Creature - Spider
Reach
1/5
*/

/*************
* Multicolor *
*************/

int card_brago_king_eternal(int player, int card, event_t event){
	/*
	 * Brago, King Eternal	|2|W|U
	 * Legendary Creautre - Spirit 2/4
	 * Flying
	 * When ~ deals combat damage to a player, exile any number of target nonland permanents you control, then return those cards to the battlefield under their
	 * owner's control.
	 */
	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allowed_controller = player;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		while( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				add_state(instance->targets[0].player, instance->targets[0].card, STATE_CANNOT_TARGET | STATE_TARGETTED);
		}

		int i = active_cards_count[player]-1;
		while( i > -1 ){
				if( in_play(player, i) && check_state(player, i, STATE_CANNOT_TARGET) && check_state(player, i, STATE_TARGETTED) ){
					remove_state(player, i, STATE_CANNOT_TARGET | STATE_TARGETTED);
					blink_effect(player, i, 0);
				}
				i--;
		}

	}

	return 0;
}

int effect_dacks_duplicant(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		haste(instance->damage_target_player, instance->damage_target_card, event);
		dethrone(instance->damage_target_player, instance->damage_target_card, event);
	}
	return 0;
}

int card_dacks_duplicant(int player, int card, event_t event){
	// to insert
	/*
	  Dack's Duplicate 2UR
	  Creature - Shapeshifter
	  You may have Dack's Duplicate enter the battlefield as a copy of any creature on the battlefield except it gains haste and dethrone. (Whenever it attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  0/0
	*/
	if (enters_the_battlefield_as_copy_of_any_creature(player, card, event)){
		set_legacy_image(player, CARD_ID_DACKS_DUPLICATE, create_targetted_legacy_activate(player, card, effect_dacks_duplicant, player, card));
	}

	return 0;
}

int card_dack_fayden(int player, int card, event_t event)
{
  /* Dack Fayden	|1|U|R
   * Planeswalker - Dack (3)
   * +1: Target player draws two cards, then discards two cards.
   * -2: Gain control of target artifact.
   * -6: You get an emblem with "Whenever you cast a spell that targets one or more permanents, gain control of those permanents." */

  if (IS_ACTIVATING(event))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td_player;
	  default_target_definition(player, card, &td_player, 0);
	  td_player.zone = TARGET_ZONE_PLAYERS;
	  td_player.preferred_controller = player;

	  target_definition_t td_artifact;
	  default_target_definition(player, card, &td_artifact, TYPE_ARTIFACT);

	  enum
	  {
		CHOICE_DRAW_DISCARD = 1,
		CHOICE_STEAL_ARTIFACT,
		CHOICE_EMBLEM
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Draw two, discard two", can_target(&td_player), 5, 1,
						"Steal artifact", can_target(&td_artifact), 5, -2,
						"Emblem", 1, check_battlefield_for_id(player, CARD_ID_DACKS_EMBLEM) ? -1 : 10, -6);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_DRAW_DISCARD:
				pick_target(&td_player, "TARGET_PLAYER");
				break;

			  case CHOICE_STEAL_ARTIFACT:
				pick_target(&td_artifact, "TARGET_ARTIFACT");
				break;

			  case CHOICE_EMBLEM:
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_DRAW_DISCARD:
			  if (valid_target(&td_player))
				{
				  draw_cards(instance->targets[0].player, 2);
				  new_multidiscard(instance->targets[0].player, 2, 0, player);
				}
			  break;

			case CHOICE_STEAL_ARTIFACT:
			  if (valid_target(&td_artifact))
				{
				  if (instance->targets[0].player == player)
					{
					  if (player == AI)
						ai_modifier -= 128;
					}
				  else
					gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
				}
			  break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_DACKS_EMBLEM);
			  break;
		  }
	}

  return planeswalker(player, card, event, 3);
}

static int test_targets_opponents_permanent(int iid, int me, int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);
  int i;
  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[i].player != me && instance->targets[i].card != -1 &&
		is_what(instance->targets[i].player, instance->targets[i].card, TYPE_PERMANENT))
		return 1;

  return 0;
}

int card_dack_faydens_emblem(int player, int card, event_t event)
{
  // Whenever you cast a spell that targets one or more permanents, gain control of those permanents.

	if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller && trigger_cause_controller == player)
	{
		test_definition_t test;
		new_default_test_definition(&test, 0, "");
		test.special_selection_function = test_targets_opponents_permanent;
		test.value_for_special_selection_function = player;

		if (new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		{
			card_instance_t* instance = get_card_instance(player, card);
			card_instance_t* spell = get_card_instance(instance->targets[1].player, instance->targets[1].card);
			int i;
			for (i = 0; i < spell->number_of_targets; ++i)
					if(spell->targets[i].player != player && spell->targets[i].card != -1 &&
						is_what(spell->targets[i].player, spell->targets[i].card, TYPE_PERMANENT))
						gain_control(player, card, spell->targets[i].player, spell->targets[i].card);
		}
	}

	return 0;
}

int card_deathreap_ritual(int player, int card, event_t event){
	/*
	  Deathreap Ritual 2BG
	  Enchantment
	  Morbid - At the beginning of each end step, if a creature died this turn, you may draw a card.
	*/
	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player && morbid() ){
		if(event == EVENT_TRIGGER){
			if( player == HUMAN ){
				event_result |= RESOLVE_TRIGGER_OPTIONAL;
			}
			else{
				event_result |= count_deck(player) > 10 ? RESOLVE_TRIGGER_MANDATORY : 0;
			}
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				draw_cards(player, 1);
		}
	}
	return global_enchantment(player, card, event);
}

int card_extract_from_darkness(int player, int card, event_t event){
	/*
	  Extract from Darkness 3UB
	  Sorcery
	  Each player puts the top two cards of his or her library into his or her graveyard.
	  Then put a creature card from a graveyard onto the battlefield under your control.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		mill(player, 2);
		mill(1-player, 2);

		if( count_graveyard_by_type(1-player, TYPE_CREATURE) + count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);

			card_instance_t *instance = get_card_instance(player, card);

			instance->targets[0].player = 1-player;
			if( count_graveyard_by_type(1-player, TYPE_CREATURE) ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) ){
					target_definition_t td;
					default_target_definition(player, card, &td, 0);
					td.zone = TARGET_ZONE_PLAYERS;
					td.allow_cancel = 0;
					td.illegal_abilities = 0;

					pick_target(&td, "TARGET_PLAYER");
				}
			}
			else{
				instance->targets[0].player = player;
			}
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_flamewright(int player, int card, event_t event){
	/*
	  Flamewright |R|W
	  Creature - Human Artificer
	  1, T: Put a 1/1 colorless Construct artifact creature token with defender onto the battlefield.
	  T, Sacrifice a creature with defender: Flamewright deals 1 damage to target creature or player.
	  Those who can't conspire, create.
	  1/1
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_abilities = KEYWORD_DEFENDER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
		if( has_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			return 1;
		}
		if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && can_target(&td1) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Generate a Construct\n Sac & Damage creature or player\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(1-choice)) ){
				if( choice == 1 ){
					if( select_target(player, card, &td, "Select a creature with Defender to sacrifice.", NULL) ){
						instance->number_of_targets = 1;
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
					}
					else{
						spell_fizzled = 1;
						return 0;
					}
				}
				tap_card(player, card);
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_CONSTRUCT, &token);
			token.key_plus = KEYWORD_DEFENDER;
			generate_token(&token);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			damage_target0(player, card, 1);
		}
	}

	return 0;
}

int card_grenzo_dungeon_warden(int player, int card, event_t event){
	/*
	  Grenzo, Dungeon Warden XBR
	  Legendary Creature - Goblin Rogue
	  Grenzo, Dungeon Warden enters the battlefield with X +1/+1 counters on it.
	  2: Put the bottom card of your library into your graveyard. If it's a creature card with power less than or equal to Grenzo's power,
	  put it onto the battlefield.
	  2/2
	*/
	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, x_value);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		special_mill(instance->parent_controller, instance->parent_card, get_id(instance->parent_controller, instance->parent_card), player, 1);
		//Also needs arrangements in "special_mill" in "deck.c"
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
}

int card_magister_of_worth(int player, int card, event_t event)
{
  /* Magister of Worth	|4|W|B
   * Creature - Angel 4/4
   * Flying
   * Will of the Council - When ~ enters the battlefield, starting with you, each player votes for grace or condemnation.
   * If grace gets more votes, each player returns each creature card from his or graveyard to the battlefield.
   * If condemnation gets more votes or the vote is tied, destroy all creatures other than
   * ~. */

	if (comes_into_play(player, card, event)){
		char pos1[100] = "Grace";
		char pos2[100] = "Condemnation";
		int vote[2] = {0, 0};
		int ai_priorities[2] = {100, player == AI ? 200 : 50 };
		ai_priorities[0] += count_graveyard_by_type(AI, TYPE_CREATURE);
		ai_priorities[0] -= count_graveyard_by_type(HUMAN, TYPE_CREATURE);
		ai_priorities[0] = MAX(ai_priorities[0], 25);	// 75 more creatures in the human's graveyard than the AI's seems unlikely, but.
		int result = will_of_the_council(player, card, pos1, pos2, ai_priorities);
		vote[0]+=BYTE0(result);
		vote[1]+=BYTE1(result);
		if( vote[0] > vote[1] )	{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "");
		  new_reanimate_all(player, card, ANYBODY, &test, REANIMATE_DEFAULT);
		}
		else	// vote tied, or both voted condemnation
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "");
		  test.not_me = 1;
		  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
		}
	}
	return 0;
}

static int marchesa_reanimation_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		seek_grave_for_id_to_reanimate(player, card, instance->targets[0].player, instance->targets[0].card, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int marchesa_effect(int player, int card, event_t event){

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_MARCHESA_THE_BLACK_ROSE ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			card_instance_t *aff = get_card_instance(affected_card_controller, affected_card);
			if( aff->kill_code > 0 ){
				instance->damage_target_player = instance->damage_target_card = -1;
			}
		}
		if( ! is_token(affected_card_controller, affected_card) && count_1_1_counters(affected_card_controller, affected_card) ){
			count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_TRACK_DEAD_CREATURES, 0);
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *instance = get_card_instance( player, card );
		instance->targets[11].card = 0;
		int k;
		for(k=0; k<10; k++){
			if( instance->targets[k].player != -1 ){
				int legacy = create_legacy_effect(player, card, &marchesa_reanimation_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0].player = player;
				leg->targets[0].card = cards_data[instance->targets[k].player].id;
				instance->targets[k].player = -1;
			}
			if( instance->targets[k].card != -1 ){
				int legacy = create_legacy_effect(player, card, &marchesa_reanimation_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0].player = player;
				leg->targets[0].card = cards_data[instance->targets[k].card].id;
				instance->targets[k].card = -1;
			}
		}
		if( instance->damage_target_player == -1 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_marchesa_the_black_rose(int player, int card, event_t event){
	/*
	  Marchesa, the Black Rose 1UBR
	  Legendary Creature - Human Wizard
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  Other creatures you control have dethrone.
	  Whenever a creature you control with a +1/+1 counter on it dies, return that card to the battlefield at the beginning of the next end step.
	  3/3
	*/
	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)marchesa_effect ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &marchesa_effect, player, card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->number_of_targets = 0;
			leg->targets[0].player = leg->targets[0].card = -1;
		}
	}

	int amt;
	if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK | DAT_STORE_IN_INFO_SLOT | DAT_ATTACKS_PLAYER, player, -1))){
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt]) && life[1-player] >= life[player] ){
				add_1_1_counter(current_turn, attackers[amt]);
			}
		}
	}

	return 0;
}

int card_marchesas_smuggler(int player, int card, event_t event, int clr, int pow, int tou, int key, int s_key){
	/*
	  Marchesa's Smuggler |U|R
	  Creature - Human Rogue
	  Dethrone (Whenever this creature attacks the player with the most life or tied for most life, put a +1/+1 counter on it.)
	  1UR: Target creature you control gains haste until end of turn and can't be blocked this turn.
	  1/1
	*/
	dethrone(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_HASTE | SP_KEYWORD_UNBLOCKABLE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XUR(1, 1, 1), 0, &td, "TARGET_CREATURE");
}

int card_selvala_explorer_returned(int player, int card, event_t event){
	/*
	  Selvala, Explorer Returned 1GW
	  Legendary Creature - Elf Scout
	  Parley - T: Each player reveals the top card of his or her library. For each nonland card revealed this way, add G to your mana pool and you gain 1 life. Then each player draws a card.
	  2/4
	*/

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int spirits = parley(player, card);

		produce_mana(player, COLOR_GREEN, spirits);
		gain_life(player, spirits);

		draw_cards(player, 1);
		draw_cards(1-player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

/*
Sky Spirit |1|W|U --> vanilla
reature - Spirit
Flying, first strike
2/2
*/

int card_wood_sage(int player, int card, event_t event){
	/*
	  Wood Sage |U|G REPRINT
	  Creature - Human Druid
	  T: Name a creature card. Reveal the top four cards of your library and put all of them with that name into your hand. Put the rest into your graveyard.
	  1/1
	*/

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		int csvid = -1;
		int cd = count_deck(player);
		if( player == AI ){
			int rnd = internal_rand(cd);
			int try = 0;
			while( ! is_what(-1, deck[rnd], TYPE_CREATURE) && try < 2){
					rnd++;
					if( rnd >= cd ){
						rnd = 0;
						try++;
					}
			}
			if( try < 2 ){
				csvid = cards_data[deck[rnd]].id;
			}
		}
		else{
			csvid = card_from_list(player, 3, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		if( csvid != -1 ){
			int max = cd < 4 ? cd : 4;
			if( max  ){
				show_deck( player, deck, max, "Cards revealed by Sage of the Woods", 0, 0x7375B0 );
				int i;
				for(i=0; i<max; i++){
					if( cards_data[deck[0]].id == csvid ){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
					}
					else{
						mill(player, 1);
					}
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

/***********
* Artifact *
***********/

/*
AEther Searcher 7
Artifact Creature - Construct
Reveal AEther Searcher as you draft it. Reveal the next card you draft and note its name.
When AEther Searcher enters the battlefield, you may search your hand and/or library for a card with a name noted as you drafted cards named AEther Searcher. You may cast it without paying its mana cost. If you searched your library this way, shuffle it.
6/4

Agent of Acquisitions 2
Artifact Creature - Construct
Draft Agent of Acquisitions face up.
Instead of drafting a card from a booster pack, you may draft each card from that booster pack, one at a time. If you do, turn Agent of Acquisitions face down and you can't draft cards for the rest of this draft round. (You may look at booster packs passed to you.)
2/1

Cogwork Grinder |6
Artifact Creature - Construct
Draft Cogwork Grinder face up.
As you draft a card, you may remove it from the draft face down. (Those cards aren't in your card pool.)
Cogwork Grinder enters the battlefield with X +1/+1 counters on it, where X is the number of cards you removed from the draft with cards named Cogwork Grinder.
0/0

Cogwork Spy 3
Artifact Creature - Bird Construct
Reveal Cogwork Spy as you draft it. You may look at the next card drafted from this booster pack.
Flying
Paliano's creations provide ample opportunities to plot the downfall of a neighbor.
2/1


Cogwork Tracker |4
Artifact Creature - Hound Construct
Reveal Cogwork Tracker as you draft it and note the player who passed it to you.
Cogwork Tracker attacks each turn if able.
Cogwork Tracker attacks a player you noted for cards named Cogwork Tracker each turn if able.
4/4

Coercive Portal 4
Artifact
Will of the Council - At the beginning of your upkeep, starting with you, each player votes for carnage or homage. If carnage gets more votes, sacrifice Coercive Portal and destroy all nonland permanents. If homage gets more votes or the vote is tied, draw a card.

Cogwork Librarian 4 => vanilla, plus a hack in draft.c?
Artifact Creature - Construct
Draft Cogwork Librarian face up.
As you draft a card, you may draft an additional card from that booster pack. If you do, put Cogwork Librarian into that booster pack.
3/3

Deal Broker 3
Artifact Creature - Construct
Draft Deal Broker face up.
Immediately after the draft, you may reveal a card in your card pool. Each other player may offer you one card in his or her card pool in exchange. You may accept any one offer.
t: Draw a card, then discard a card.
2/3

Lore Seeker 2
Artifact Creature - Construct
Reveal Lore Seeker as you draft it. After you draft Lore Seeker, you may add a booster pack to the draft. (Your next pick is from that booster pack. Pass it to the next player and it's drafted this draft round.)
2/2

Lurking Automaton 5
Artifact Creature - Construct
Reveal Lurking Automaton as you draft it and note how many cards you've drafted this round, including Lurking Automaton.
Lurking Automaton enters the battlefield with X +1/+1 counters on it, where X is the highest number you noted for cards named Lurking Automaton.
0/0

Whispergear Sneak 1
Artifact Creature - Construct
Draft Whispergear Sneak face up.
During the draft, you may turn Whispergear Sneak face down. If you do, look at any unopened booster pack in the draft or any booster pack not being looked at by another player.
1/1
*/

int card_reito_lantern(int player, int card, event_t event){
	/*
	  Reito Lantern |2 REPRINT
	  Artifact
	  3: Put target card from a graveyard on the bottom of its owner's library.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select a card to put on bottom.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL)  ){
			if( (player != AI && count_graveyard(player) > 0) || count_graveyard(1-player) > 0 ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			instance->targets[0].player = player;
			if( count_graveyard(player) > 0 ){
				if( player != AI && count_graveyard(1-player) > 0 ){
					if( new_pick_target(&td, "TARGET_PLAYER", 0, 0) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
						return 0;
					}
				}
			}
			else{
				instance->targets[0].player = 1-player;
			}
			if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			from_graveyard_to_deck(instance->targets[0].player, selected, 2);
		}
	}

	return 0;
}

int card_runed_servitor(int player, int card, event_t event){
	/*
	  Runed Servitor |2 REPRINT
	  Artifact Creature - Construct
	  When Runed Servitor dies, each player draws a card.
	  2/2
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, 1);
		draw_cards(1-player, 1);
	}

	return 0;
}

int card_silent_arbiter(int player, int card, event_t event){
	/*
	  Silent Arbiter |4 REPRINT
	  Artifact Creature - Construct
	  No more than one creature can attack each combat.
	  No more than one creature can block each combat.
	  1/5
	*/
	if( ! is_humiliated(player, card) ){
		no_more_than_x_creatures_can_attack(player, card, event, ANYBODY, 1);
		no_more_than_x_creatures_can_block(player, card, event, ANYBODY, 1);
	}

	return 0;
}

int card_spectral_searchlight(int player, int card, event_t event){
	/*
	  Spectral Searchlight |3 REPRINT
	  Artifact
	  T: Choose a player. That player adds one mana of any color he or she chooses to his or her mana pool.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == AI ){
			if( would_validate_arbitrary_target(&td, player, -1) ){
				return mana_producer(player, card, event);
			}
		}
		else{
			if( can_target(&td) ){
				return mana_producer(player, card, event);
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		if( player == AI ){
			return mana_producer(player, card, event);
		}
		else{
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
				produce_mana(instance->targets[0].player, choose_a_color(player, 0), 1);
			}
		}
	}

	return 0;
}

int card_vedalken_orrery(int player, int card, event_t event){
	// to insert
	// Also code for Leyline of Anticipation
	/*
	  Vedalken Orrery |4 REPRINT
	  Artifact
	  You may cast nonland cards as though they had flash.
	*/

	if( event == EVENT_CAN_ACTIVATE ){
		int i;
		int result = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) &&
				has_mana_to_cast_iid(player, event, get_card_instance(player, i)->internal_card_id)
			  ){
				int tr = can_legally_play_card_in_hand(player, i);
				if( tr ){
					if( tr == 99 ){
						return 99;
					}
					result = tr;
				}
			}
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		int playable[2][hand_count[player]];
		int pc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) &&
				has_mana_to_cast_iid(player, event, get_card_instance(player, i)->internal_card_id)
			  ){
				if( can_legally_play_card_in_hand(player, i) ){
					playable[0][pc] = get_card_instance(player, i)->internal_card_id;
					playable[1][pc] = i;
					pc++;
				}
			}
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a non-land card to play.");
		this_test.type_flag = DOESNT_MATCH;

		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			if( charge_mana_from_id(player, -1, event, cards_data[playable[0][selected]].id) ){
				play_card_in_hand_for_free(player, playable[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		else{
			spell_fizzled = 1;
			return 0;
		}
	}
	// Needed for Leyline of Anticipation
	return global_enchantment(player, card, event);
}

/***********
* Lands	   *
***********/
/*
Lands (1)

Paliano, the High City
Legendary Land
Reveal Paliano, the High City as you draft it. The player to your right chooses a color, you choose another color, then the player to your left chooses a third color.
T: Add one mana to your mana pool of any color chosen as you drafted cards named Paliano, the High City.
*/
