#include "manalink.h"

// global functions

int has_convoked_mana_extended_generic(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int cmc_modifier){
	int up_cless = (cless == 40 || cless == -1) ? 0 : cless; // cless == 40 or -1 meaning 'X in casting cost'.
	if( cless != 40 && cless != -1 && cmc_modifier ){
		up_cless = get_updated_casting_cost(player, card, -1, event, cless);
	}
	int tapped[active_cards_count[player]];
	int tc = 0;
	int tapped_for_color[active_cards_count[player]][5];
	int i,k;
	for(i=0; i<active_cards_count[player]; i++){
		for(k=0; k<5; k++){
			tapped_for_color[i][k] = 0;
		}
	}
	int rval = 0;
	int attempt = 0;
	while( ! rval && attempt < 50 ){
		int clrs[6] = {up_cless, black, blue, green, red, white};
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && ! is_tapped(player, i) && is_what(player, i, TYPE_CREATURE) ){
				int clr = get_color(player, i);
				int col_found = 0;
				for(k=1; k<6; k++){
					if( (clr & (1<<k)) && clrs[k] > 0 && tapped_for_color[i][k-1] != 1){
						clrs[k]--;
						col_found = 1;
						tapped_for_color[i][k-1] = 1;
						add_state(player, i, STATE_TAPPED);
						tapped[tc] = i;
						tc++;
						break;
					}
				}
				if( ! col_found && clrs[0]){
					clrs[0]--;
					add_state(player, i, STATE_TAPPED);
					tapped[tc] = i;
					tc++;
				}
			}
			if( clrs[0]+clrs[1]+clrs[2]+clrs[3]+clrs[4]+clrs[5] == 0 ){
				break;
			}
		}
		rval = has_mana_multi(player, clrs[0], clrs[1], clrs[2], clrs[3], clrs[4], clrs[5]);
		for(i=0; i<tc; i++){
			remove_state(player, tapped[i], STATE_TAPPED);
		}
		if( rval && tc > 0 ){
			rval = 2;
		}
		tc = 0;
		attempt++;
	}
	return rval;
}

int has_convoked_mana_extended(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){
	return has_convoked_mana_extended_generic(player, card, event, cless, black, blue, green, red, white, 1);
}

int has_convoked_mana(int player, int card, int cless, int green, int white ){

	return has_convoked_mana_extended(player, card, EVENT_CAN_CAST, cless, 0, 0, green, 0, white);
}

static int will_tap_permanent_for_convoke(int player, int card, int *clrs){
	int clr = get_color(player, card);
	if( clr == COLOR_COLORLESS ){
		if( clrs[0] ){
			clrs[0]--;
			return 1;
		}
	}
	else if( real_count_colors(clr) == 1 ){
			int k;
			int col_found = 0;
			for(k=1; k<6; k++){
				if( (clr & (1<<k)) && clrs[k] > 0 ){
					clrs[k]--;
					return 1;
				}
			}
			if( ! col_found ){
				if( clrs[0] ){
					clrs[0]--;
					return 1;
				}
			}
	}
	else{
		int choice = DIALOG(player, card, EVENT_CAST_SPELL,
							DLG_NO_STORAGE,
							"Tap for Colorless", clrs[COLOR_COLORLESS], 0,
							"Tap for Black", (clr & COLOR_TEST_COLORLESS) && clrs[COLOR_BLACK], 0,
							"Tap for Blue", (clr & COLOR_TEST_BLUE) && clrs[COLOR_BLUE], 0,
							"Tap for Green", (clr & COLOR_TEST_GREEN) && clrs[COLOR_GREEN], 0,
							"Tap for Red", (clr & COLOR_TEST_RED) && clrs[COLOR_RED], 0,
							"Tap for White", (clr & COLOR_TEST_WHITE) && clrs[COLOR_WHITE], 0);
		if( choice ){
			clrs[choice-1]--;
			return 1;
		}
	}
	return 0;
}

int charge_convoked_mana_extended_generic(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int check_for_cc_modifier){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	int up_cless = (cless == 40 || cless == -1) ? 0 : cless; // cless == 40 or -1 meaning 'X in casting cost'.
	if( check_for_cc_modifier ){
		up_cless = get_updated_casting_cost(player, card, -1, event, cless);
	}
	int x_flag = (cless == 40 || cless == -1) ? 1 : 0;

	int tapped[active_cards_count[player]];
	int tc = 0;

	card_instance_t *instance = get_card_instance( player, card );

	int plus = 0;
	int mode = 0;
	if( can_target(&td) ){
		if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
			int ai_choice = is_what(player, card, TYPE_INSTANT | TYPE_INTERRUPT) && current_turn != player ? 0 : 1;
			mode = do_dialog(player, player, card, -1, -1, " Use Convoke\n Play normally\n Cancel", ai_choice);
		}
	}
	else{
		mode = 1;
	}

	int creatures_tapped_for_convoke = 0;
	if( mode == 0 ){
		if( player == HUMAN ){
			int clrs[6] = {up_cless, black, blue, green, red, white};
			while( can_target(&td) ){
					if( select_target(player, card, &td, "Select a creature to tap for Convoke.", NULL) ){
						instance->number_of_targets = 0;
						if( will_tap_permanent_for_convoke(instance->targets[0].player, instance->targets[0].card, clrs) ){
							add_state(instance->targets[0].player, instance->targets[0].card, STATE_TAPPED);
							tapped[tc] = instance->targets[0].card;
							tc++;
							creatures_tapped_for_convoke++;
						}
						else if( x_flag ){
								plus++;
								add_state(instance->targets[0].player, instance->targets[0].card, STATE_TAPPED);
								tapped[tc] = instance->targets[0].card;
								tc++;
								creatures_tapped_for_convoke++;
						}
						if( clrs[0]+clrs[1]+clrs[2]+clrs[3]+clrs[4]+clrs[5] == 0 && ! x_flag ){
							break;
						}
					}
					else{
						break;
					}
			}
			charge_mana_multi(player, clrs[0], clrs[1], clrs[2], clrs[3], clrs[4], clrs[5]);
			if( x_flag && has_mana(player, COLOR_COLORLESS, 1) ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 ){
					plus+=x_value;
				}
			}
			if( spell_fizzled == 1 ){
				int i;
				for(i=0; i<tc; i++){
					remove_state(player, tapped[i], STATE_TAPPED);
				}
			}
		}
		else{
			int rval = 0;
			int tapped_for_color[active_cards_count[player]][5];
			int i,k;
			for(i=0; i<active_cards_count[player]; i++){
				for(k=0; k<5; k++){
					tapped_for_color[i][k] = 0;
				}
			}
			int attempt = 0;
			while( attempt < 50 ){
					int clrs[6] = {up_cless, black, blue, green, red, white};
					for(i=0; i<active_cards_count[player]; i++){
						if( in_play(player, i) && ! is_tapped(player, i) && is_what(player, i, TYPE_CREATURE) ){
							int clr = get_color(player, i);
							int col_found = 0;
							for(k=1; k<6; k++){
								if( (clr & (1<<k)) && clrs[k] > 0 && tapped_for_color[i][k-1] != 1){
									clrs[k]--;
									col_found = 1;
									tapped_for_color[i][k-1] = 1;
									add_state(player, i, STATE_TAPPED);
									tapped[tc] = i;
									tc++;
									creatures_tapped_for_convoke++;
									break;
								}
							}
							if( ! col_found ){
								if( clrs[0] ){
									add_state(player, i, STATE_TAPPED);
									tapped[tc] = i;
									tc++;
									clrs[0]--;
									creatures_tapped_for_convoke++;
								}
								else if( x_flag ){
										add_state(player, i, STATE_TAPPED);
										tapped[tc] = i;
										tc++;
										plus++;
										creatures_tapped_for_convoke++;
								}
							}
						}
						if( clrs[0]+clrs[1]+clrs[2]+clrs[3]+clrs[4]+clrs[5] == 0 && ! x_flag ){
							break;
						}
					}
					rval = has_mana_multi(player, clrs[0], clrs[1], clrs[2], clrs[3], clrs[4], clrs[5]);
					if( rval ){
						charge_mana_multi(player, clrs[0], clrs[1], clrs[2], clrs[3], clrs[4], clrs[5]);
						if( x_flag && has_mana(player, COLOR_COLORLESS, 1) ){
							charge_mana(player, COLOR_COLORLESS, -1);
							if( spell_fizzled != 1 ){
								plus+=x_value;
							}
						}
						if( spell_fizzled == 1 ){
							rval = 0;
						}
						else{
							break;
						}
					}
					plus = 0;
					for(i=0; i<tc; i++){
						remove_state(player, tapped[i], STATE_TAPPED);
					}
					creatures_tapped_for_convoke = 0;
					tc = 0;
					attempt++;
			}
			if( ! rval ){
				spell_fizzled = 1;
			}
		}
		int result = spell_fizzled != 1 ? (plus ? plus : 1) : 0;
		if( result ){
			result |= (creatures_tapped_for_convoke ? 1<<31 : 0);
		}
		return result;
	}
	if( mode == 1 ){
		if( check_for_cc_modifier ){
			charge_mana_from_id(player, -1, event, get_id(player, card));
		}
		else{
			charge_mana_multi(player, cless, black, blue, green, red, white);
		}
		if( spell_fizzled != 1 ){
			if (cless == -1){
				return x_value;	// And no way to tell the difference between 0 and cancel from the return value - but you should just check for spell_fizzled == 1 anyway
			} else {
				return 1;
			}
		}
	}
	if( mode == 2 ){
		spell_fizzled = 1;
	}
	return 0;
}

int charge_convoked_mana_extended(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){
	return charge_convoked_mana_extended_generic(player, card, event, cless, black, blue, green, red, white, 1);
}

int charge_convoked_mana(int player, int card, int cless, int green, int white ){

	return charge_convoked_mana_extended(player, card, EVENT_CAST_SPELL, cless, 0, 0, green, 0, white);
}

int ravnica_manachanger(int player, int card, event_t event, int plus_black, int plus_blue, int plus_green, int plus_red, int plus_white){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		card_ptr_t* c = cards_ptr[ get_id(player, card) ];
		int cless = get_updated_casting_cost(player, card, -1, event, -1);
		if( has_mana_multi(player, cless-1, c->req_black+plus_black, c->req_blue+plus_blue,
							c->req_green+plus_green, c->req_red+plus_red, c->req_white+plus_white)
		  ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}
	return 0;
}

int charge_changed_mana(int player, int card, int plus_black, int plus_blue, int plus_green, int plus_red, int plus_white){
	int special_color = COLOR_BLACK;
	if( plus_blue ){
		special_color = COLOR_BLUE;
	}
	if( plus_green ){
		special_color = COLOR_GREEN;
	}
	if( plus_red ){
		special_color = COLOR_RED;
	}
	if( plus_white ){
		special_color = COLOR_WHITE;
	}
	card_ptr_t* c = cards_ptr[ get_id(player, card) ];
	int cless = get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, c->req_colorless);
	if( cless < 1 ){
		cless = 0;
	}
	int cless2 = cless-1;
	if( cless2 < 1 ){
		cless2 = 0;
	}
	int choice = 0;
	if( has_mana_multi(player, cless-1, c->req_black+plus_black, c->req_blue+plus_blue, c->req_green+plus_green, c->req_red+plus_red, c->req_white+plus_white)){
		if( player == AI ){
			choice = 1;
		}
		else{
			int manacost[5] = { c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white };
			int manacost_mod[5] = { c->req_black+plus_black,  c->req_blue+plus_blue, c->req_green+plus_green, c->req_red+plus_red, c->req_white+plus_white };
			char buffer[500];
			int pos = scnprintf(buffer, 500, " Pay ");
			if( cless == -1 ){
				pos += scnprintf(buffer + pos, 500-pos, "X" );
			}
			else if( cless > 0 ){
					pos += scnprintf(buffer + pos, 500-pos, "%d", cless );
			}

			int q;
			for(q=0; q<5; q++){
				if( manacost[q] > 0 ){
					int w;
					for(w=0;w<manacost[q]; w++){
						if( q == 0 ){
							pos += scnprintf(buffer + pos, 500-pos, "B" );
						}
						if( q == 1 ){
							pos += scnprintf(buffer + pos, 500-pos, "U" );
						}
						if( q == 2 ){
							pos += scnprintf(buffer + pos, 500-pos, "G" );
						}
						if( q == 3 ){
							pos += scnprintf(buffer + pos, 500-pos, "R" );
						}
						if( q == 4 ){
							pos += scnprintf(buffer + pos, 500-pos, "W" );
						}
					}
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, "\n Pay ");
			if( cless2 == -1 ){
				pos += scnprintf(buffer + pos, 500-pos, "X" );
			}
			else if( cless2 > 0 ){
					pos += scnprintf(buffer + pos, 500-pos, "%d", cless2 );
			}

			q=0;
			for(q=0; q<5; q++){
				if( manacost_mod[q] > 0 ){
					int w;
					for(w=0;w<manacost_mod[q]; w++){
						if( q == 0 ){
							pos += scnprintf(buffer + pos, 500-pos, "B" );
						}
						if( q == 1 ){
							pos += scnprintf(buffer + pos, 500-pos, "U" );
						}
						if( q == 2 ){
							pos += scnprintf(buffer + pos, 500-pos, "G" );
						}
						if( q == 3 ){
							pos += scnprintf(buffer + pos, 500-pos, "R" );
						}
						if( q == 4 ){
							pos += scnprintf(buffer + pos, 500-pos, "W" );
						}
					}
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, "\n Cancel");
			choice = do_dialog(player, player, card, -1, -1, buffer , 0);
		}
	}
	if( choice == 0 ){
		charge_mana_multi(player, cless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white);
		if( spell_fizzled != 1 ){
			int result = 1 + (mana_paid[special_color] > 0);
			return result;
		}
		else{
			return 0;
		}
	}
	else if( choice == 1 ){
			charge_mana_multi(player, cless2, c->req_black+plus_black, c->req_blue+plus_blue, c->req_green+plus_green, c->req_red+plus_red, c->req_white+plus_white);
			if( spell_fizzled != 1 ){
				return 2;
			}
			else{
				return 0;
			}
	}
	else{
		spell_fizzled = 1;
		return 0;
	}
	return 0;
}

static void radiance_pump(int player, int card, int t_player, int t_card, int power, int toughness, int ability, int sp_ability, int effect ){

	int i;
	for(i=0; i<2; i++){
		int count = active_cards_count[i]-1;
		while(count > -1){
			if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
				if( (i == t_player && count == t_card) || has_my_colors(t_player, t_card, i, count) > 0 ){
					pump_ability_until_eot(player, card, i, count, power, toughness, ability, sp_ability );
					if( effect == ACT_UNTAP ){
						untap_card(i, count);
					}
				}
			}
			count--;
		}
	}

}

int transmute(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int my_cmc){
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND && has_mana_multi(player, colorless, black, blue, green, red, white) ){
		if( can_sorcery_be_played(player, event) ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, colorless, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		test_definition_t test;
		new_default_test_definition(&test, 0, "");
		test.cmc = my_cmc;
		test.cmc_flag = MATCH;
		scnprintf(test.message, 100, "Select a card with CMC %d.", my_cmc);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}
	return 0;
}

int dredge(int player, int card, event_t event, int amount){
	if( event == EVENT_GRAVEYARD_ABILITY ){
		return amount;
	}
	return 0;
}

int card_generic_shockland(int player, int card, event_t event){

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* land = get_card_instance(player, card);

	  int played_normally = !played_for_free(player, card) && !(land->state & STATE_IN_PLAY);	// The latter will be true if real_put_into_play() was called

	  int cpl = can_pay_life(player, 2);

	  int ai_pay_life_priority = -1;

	  if (player == AI
		  && cpl
		  && life[AI] >= 8	// This is very blunt.  Ideally we'd want the AI to speculate on both paths and see whether a newly-castable spell is worth paying 2 life.
		  && (land->state & (STATE_INVISIBLE | STATE_OUBLIETTED | STATE_IN_PLAY | STATE_TAPPED)) == STATE_INVISIBLE	// not already tapped by something else, and being put into play normally (some cards, like Rampant Growth, call put_into_play() before setting STATE_TAPPED)
		  && !check_battlefield_for_id(player, CARD_ID_AMULET_OF_VIGOR))	// it'll untap itself anyway
		{
		  // see if this land will help the AI cast a spell
		  land->state &= ~STATE_INVISIBLE;	// temporarily off stack
		  land->state |= STATE_IN_PLAY;		// temporarily in play
		  count_mana();
		  int c;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (in_hand(player, c))
			  {
				int iid = get_card_instance(player, c)->internal_card_id;
				if (iid > -1
					&& has_mana_to_cast_iid(player, event, iid))	// card is castable with land untapped
				  {
					// tap the land and see if card becomes uncastable
					land->state |= STATE_TAPPED;
					count_mana();
					int can_cast = has_mana_to_cast_iid(player, event, iid);
					land->state &= ~STATE_TAPPED;
					count_mana();
					if (!can_cast)
					  {
						ai_pay_life_priority = 10;
						break;
					  }
				  }
			  }
		  land->state &= ~STATE_IN_PLAY;	// back out of play
		  land->state |= STATE_INVISIBLE;	// back onto the stack
		  count_mana();
		}

	  int choice = DIALOG(player, card, event,
						  DLG_NO_STORAGE, DLG_NO_CANCEL, /* Added manually, so we can grey it out if not played normally */
						  "Enter the battlefield tapped", 1, 1,
						  "Pay 2 life", cpl, ai_pay_life_priority,
						  "Cancel", played_normally, -1);

	  if (choice == 1)
		land->state |= STATE_TAPPED;	// avoid sending event
	  else if (choice == 2)
		lose_life(player, 2);
	  else
		cancel = 1;	// must set manually due to DLG_NO_STORAGE flag, and because it's not autogenerated due to DLG_NO_CANCEL flag
	}

  return mana_producer(player, card, event);
}

static int signet2(int player, int card, event_t event, int c1, int c2){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		instance->targets[1].player != 66 && has_mana(player, COLOR_COLORLESS, 1)
	  ){
		if( can_produce_mana(player, card) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_COLORLESS, 1);
		instance->targets[1].player = 0;
		if( spell_fizzled != 1 ){
			produce_mana_tapped2(player, card, c1, 1, c2, 1);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 2) && instance->targets[1].player != 66 && !is_animated_and_sick(player, card) ){
		  /* Best we can do - this says that it can produce one mana of either color1 or color2.  There may be a way to manipulate the has_mana() backend data
		   * to account for color-fixing, but it's not understood well enough yet.
		   *
		   * Investigating card_celestial_prism(), and in particular dword_738B48, dword_7A2FE4, sub_498F20(), and sub_499050(), might help, but I doubt it;
		   * celestial prism (and the former asm version of signet) doesn't handle EVENT_COUNT_MANA at all. */

		  declare_mana_available_hex(player, (1<<c1) | (1<<c2), 1);
		}
	}

	return 0;
}

// cards
int card_agrus_kos_wojek_veteran(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever ~ attacks, attacking |Sred creatures get +2/+0 and attacking |Swhite creatures get +0/+2 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  test.color = COLOR_TEST_RED;
	  pump_creatures_until_eot(player, card, current_turn, 0, 2,0, 0,0, &test);

	  test.color = COLOR_TEST_WHITE;
	  pump_creatures_until_eot(player, card, current_turn, 0, 0,2, 0,0, &test);
	}

  return 0;
}

int card_auratouched_mage(int player, int card, event_t event){

	/* Auratouched Mage	|5|W
	 * Creature - Human Wizard 3/3
	 * When ~ enters the battlefield, search your library for an Aura card that could enchant it. If ~ is still on the battlefield, put that Aura card onto the
	 * battlefield attached to it. Otherwise, reveal the Aura card and put it into your hand. Then shuffle your library. */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an Aura card with enchant creature.");
		this_test.subtype = SUBTYPE_AURA_CREATURE;

		int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
		if( result > -1 && in_play(player, card) ){
			put_into_play_aura_attached_to_target(player, result, player, card);
		}
	}

	return 0;
}

int card_autochthon_wurm(int player, int card, event_t event){

	return generic_spell_with_convoke(player, card, event);
}

int card_azorius_signet(int player, int card, event_t event){
	// First printed in Dissension, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_BLUE, COLOR_WHITE);
}

int card_bathe_in_light(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int kw = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			radiance_pump(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, kw, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_blazing_archon(int player, int card, event_t event){

	if( current_turn != player ){
		nobody_can_attack(player, card, event, 1-player);
	}

	return 0;
}

int card_bloodbond_march(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int i;
		for(i=0; i<2; i++){
			const int *grave = get_grave(i);
			int count = count_graveyard(i)-1;
			while( count > -1 ){
					if( cards_data[grave[count]].id == id ){
						int card_added = add_card_to_hand(i, grave[count]);
						remove_card_from_grave(i, count);
						put_into_play(i, card_added);
					}
					count--;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_bloodletter_quill(int player, int card, event_t event)
{
  /* Bloodletter Quill	|3
   * Artifact
   * |2, |T, Put a blood counter on ~: Draw a card, then lose 1 life for each blood counter on ~.
   * |U|B: Remove a blood counter from ~. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  enum
	  {
		CHOICE_DRAW = 1,
		CHOICE_REMOVE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Draw a card", CAN_TAP(player, card), 1, DLG_MANA(MANACOST_X(2)),
						"Remove a blood counter", 1, count_counters(player, card, COUNTER_BLOOD), DLG_MANA(MANACOST_UB(1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_DRAW:
			  add_counter(player, card, COUNTER_BLOOD);
			  tap_card(player, card);
			  if (player == AI)
				ai_modifier += 48;	// so it'll even consider activating
			  break;
			case CHOICE_REMOVE:
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_DRAW:
			  draw_a_card(player);
			  lose_life(player, count_counters(player, card, COUNTER_BLOOD));
			  break;
			case CHOICE_REMOVE:
			  remove_counter(player, card, COUNTER_BLOOD);
			  break;
		  }
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	lose_life(player, count_counters(player, card, COUNTER_BLOOD));

  return 0;
}

int card_boros_fury_shield(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 0, 1, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 0, 1, 0);
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			negate_combat_damage_this_turn(player, card, instance->targets[0].player, instance->targets[0].card, 0);
			if( instance->targets[1].player  == 2 ){
				damage_player(instance->targets[0].player, get_power(instance->targets[0].player, instance->targets[0].card), player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_boros_guildmage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_state = TARGET_STATE_SUMMONING_SICK;
	td1.preferred_controller = player;
	td1.allowed_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( player != AI ){
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ||  has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
				return can_target(&td);
			}
		}
		else{
			if( current_phase < PHASE_DECLARE_ATTACKERS && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ){
				return can_target(&td1);
			}
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ){
				return can_target(&td);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		int red = 1;
		int white = 1;
		if( current_phase < PHASE_DECLARE_ATTACKERS && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) && can_target(&td1) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Give First Strike\n Give Haste\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2){
			spell_fizzled = 1;
		}
		else if( choice == 0){
				red=0;
		}
		else if( choice == 1 ){
				white = 0;
		}
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, red, white);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 66+choice;
			instance->number_of_targets = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->info_slot == 66 ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
			}
			if( instance->info_slot == 67 ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}
	return 0;
}

int card_boros_recruit(int player, int card, event_t event){

	hybrid(player, card, event);

	return 0;
}

int card_boros_signet(int player, int card, event_t event){
	return signet2(player, card, event, COLOR_WHITE, COLOR_RED);
}

// boros swiftblade --> vanilla

int card_brainspoil(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_state = TARGET_STATE_ENCHANTED;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return transmute(player, card, event, 1, 2, 0, 0, 0, 0, 5);
}

int card_bramble_elemental(int player, int card, event_t event){

	/* Bramble Elemental	|3|G|G
	 * Creature - Elemental 4/4
	 * Whenever an Aura becomes attached to ~ or enters the battlefield attached to ~, put two 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	if( aura_attached_to_me(player, card, event, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 2);
	}

	return 0;
}

int brightflame_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *dmg = get_card_instance( affected_card_controller, affected_card );
		if( dmg->internal_card_id == damage_card && dmg->info_slot > 0 ){
			if( dmg->damage_source_player == instance->targets[0].player && dmg->damage_source_card == instance->targets[0].card ){
				instance->targets[1].player = instance->targets[1].player < 0 ? 0 : instance->targets[1].player;
				instance->targets[1].player+=dmg->info_slot;
			}
		}
	}

	if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		gain_life(player, instance->targets[1].player);
		instance->targets[1].player = 0;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_brightflame(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){

		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );
			int legacy = create_legacy_effect(player, card, &brightflame_legacy);
			get_card_instance(player, legacy)->targets[0].player = player;
			get_card_instance(player, legacy)->targets[0].card = card;
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while(count > -1){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && !(i==instance->targets[0].player && count == instance->targets[0].card) &&
						has_my_colors(instance->targets[0].player, instance->targets[0].card, i, count) > 0
					  ){
						damage_creature(i, count, instance->info_slot, player, card);
					}
					count--;
				}
			}
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, card);
			//kill_card(player, legacy, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_carven_caryatid(int player, int card, event_t event){
	// original code : 0040E990

	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	return 0;
}

int card_centaur_safeguard(int player, int card, event_t event){
	hybrid(player, card, event);
	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_DUH)){
		gain_life(player, 3);
	}
	return 0;
}

static int covg_legacy(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
				gain_life(player, damage->info_slot);
				damage->info_slot = 0;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_chant_of_vitu_ghazi(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell_with_convoke(player, card, event);
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &covg_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_chord_of_calling(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST){
		card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
		if( has_convoked_mana_extended(player, card, event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
			null_casting_cost(player, card);
			instance->targets[1].player = 1;
		}
		else{
			instance->targets[1].player = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			return 0;
		}
		if( played_for_free(player, card) || is_token(player, card) ){
			instance->info_slot = 0;
			return 0;
		}
		if( instance->targets[1].player == 1 ){
			if( charge_convoked_mana_extended(player, card, event, MANACOST_G(3)) ){
				int result = charge_convoked_mana_extended_generic(player, card, event, MANACOST_X(-1), 0);
				if( result & (1<<31) ){
					result &= ~(1<<31);
				}
				instance->info_slot = result;
			}
		}
		else{
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		x_value = 0;
		char buffer[100];
		scnprintf(buffer, 100, "Select a creature card with CMC %d or less", instance->info_slot);
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, buffer);
		test.cmc = instance->info_slot+1;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_chorus_of_the_conclave(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	card_instance_t *instance = get_card_instance( player, card );
	if( has_mana(player, COLOR_COLORLESS, 1) && specific_spell_played(player, card, event, player, 1+player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			add_1_1_counters(instance->targets[1].player, instance->targets[1].card, x_value);
		}
	}
	return 0;
}

int card_circu_dimir_lobotomist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	check_legend_rule(player, card, event);

	int removed_by_circu[99];

	card_instance_t *instance = get_card_instance( player, card );

	if( can_target(&td) && specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLACK | COLOR_TEST_BLUE, 0, 0, 0, -1, 0) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			int *deck = deck_ptr[instance->targets[0].player];
			int to_remove = 0;
			if( get_color(instance->targets[1].player, instance->targets[1].card) & COLOR_TEST_BLACK ){
				to_remove++;
			}
			if( get_color(instance->targets[1].player, instance->targets[1].card) & COLOR_TEST_BLUE ){
				to_remove++;
			}
			int i;
			for(i=0; i<to_remove; i++){
				int id = cards_data[deck[instance->targets[0].player]].id;
				rfg_top_card_of_deck(instance->targets[0].player);
				if( instance->targets[1].player < 0 ){
					instance->targets[1].player = 0;
				}
				int pos = instance->targets[1].player;
				removed_by_circu[pos] = id;
			}
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == 1-player ){
		int i;
		for(i=0; i<instance->targets[1].player; i++){
			if( get_id(affected_card_controller, affected_card) == removed_by_circu[i] ){
				infinite_casting_cost();
				break;
			}
		}
	}


	return 0;
}

int card_cleansing_beam(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while(count > -1){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && !(i==instance->targets[0].player && count == instance->targets[0].card) &&
						has_my_colors(instance->targets[0].player, instance->targets[0].card, i, count) > 0
						){
						damage_creature(i, count, 2, player, card);
					}
					count--;
				}
			}
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return transmute(player, card, event, 1, 2, 0, 0, 0, 0, 5);
}

int card_cloudstone_curio(int player, int card, event_t event){

	card_instance_t *curio = get_card_instance(player, card);

	if(trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
	   reason_for_trigger_controller == player && trigger_cause_controller == player
	   ){
		int trig = 0;
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_PERMANENT) &&
			!is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT)
			){
			trig = 1;
		}

		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}

		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){

				int orig_trigger_cause_controller = trigger_cause_controller;
				int orig_trigger_cause = trigger_cause;
				state_untargettable(orig_trigger_cause_controller, orig_trigger_cause, 1);

				target_definition_t td;
				default_target_definition(player, card, &td, get_type(trigger_cause_controller, trigger_cause));
				td.allowed_controller = player;
				td.preferred_controller = player;
				td.illegal_abilities = 0;

				if( can_target(&td) && pick_target(&td, "CURIO") ){
					curio->number_of_targets = 1;
					bounce_permanent(player, curio->targets[0].card);
				}

				state_untargettable(orig_trigger_cause_controller, orig_trigger_cause, 0);
			}
		}
	}

	return 0;
}

int card_clutch_of_the_undercity(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			lose_life(instance->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return transmute(player, card, event, 1, 1, 1, 0, 0, 0, 4);
}

int card_compulsive_research(int player, int card, event_t event){
	/* Original Code: 004E81DF */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card);
			draw_cards(instance->targets[0].player, 3);
			int discarded = 0;
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card to discard");
			test.zone = TARGET_ZONE_HAND;

			test_definition_t test2;
			default_test_definition(&test2, TYPE_LAND);
			test2.zone = TARGET_ZONE_HAND;
			while( hand_count[instance->targets[0].player] && discarded < 2 ){
					int selected = -1;
					if( player == HUMAN ){
						selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &test);
					}
					else{
						selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &test2);
						if( selected == -1 ){
							selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &test);
						}
					}
					int is_land = is_what(player, selected, TYPE_LAND);
					discard_card(player, selected);
					if( is_land ){
						break;
					}
					discarded++;
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_concerted_effort(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = 0;
		int key = 0;
		int s_key  = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					// Leaves out weird protections and landwalks.  Oh well.
					key |= (get_abilities(player, count, EVENT_ABILITIES, -1)
							& (KEYWORD_FLYING | KEYWORD_FIRST_STRIKE | KEYWORD_DOUBLE_STRIKE | KEYWORD_BASIC_LANDWALK
							   | KEYWORD_PROT_COLORED | KEYWORD_PROT_ARTIFACTS | KEYWORD_TRAMPLE));
					if( !(s_key & SP_KEYWORD_VIGILANCE) && check_for_special_ability(player, count, SP_KEYWORD_VIGILANCE) ){
						s_key |= SP_KEYWORD_VIGILANCE;
					}
					if( !(s_key & SP_KEYWORD_FEAR) && check_for_special_ability(player, count, SP_KEYWORD_FEAR) ){
						s_key |= SP_KEYWORD_FEAR;
					}
				}
				count++;
		}
		if (key || s_key){
			pump_subtype_until_eot(player, card, player, -1, 0, 0, key | KEYWORD_RECALC_SET_COLOR, s_key);
		}
	}

	return global_enchantment(player, card, event);
}

int card_conclave_phalanx(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell_with_convoke(player, card, event);
	}

	if( comes_into_play(player, card, event)  ){
		gain_life(player, count_permanents_by_type(player, TYPE_CREATURE));
	}

	return 0;
}

int card_congregation_at_dawn(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		int tutored = 0;
		while( tutored < 3 ){
				int creature = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 11, &this_test);
				if( creature != -1 ){
					instance->targets[tutored].card = creature;
					tutored++;
				}
				else{
					break;
				}

		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_consult_the_necrosages(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int choice = 0;
			if( instance->targets[0].player == 1-player ){
				choice = 1;
			}
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Target player draws\n Target player discard", 0);
			}
			if( choice == 0 ){
				draw_cards(instance->targets[0].player, 2);
			}
			if( choice == 1 ){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_convolute(int player, int card, event_t event){
	/* Convolute	|2|U
	 * Instant
	 * Counter target spell unless its controller pays |4. */

	if( event == EVENT_RESOLVE_SPELL){
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, 4);
			kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_copy_enchantment(int player, int card, event_t event)
{
  /* Copy Enchantment	|2|U
   * Enchantment
   * You may have ~ enter the battlefield as a copy of any enchantment on the battlefield. */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	base_target_definition(player, card, &td, TYPE_ENCHANTMENT);

  enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_AN_ENCHANTMENT");

  return global_enchantment(player, card, event);
}

int card_crown_of_convergence(int player, int card, event_t event ){

	reveal_top_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 1)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		put_top_card_of_deck_to_bottom(player);
	}

	if( affected_card_controller == player && (event == EVENT_POWER || event == EVENT_TOUGHNESS) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			if( is_what(-1, deck[0], TYPE_CREATURE) && has_my_colors(-1, deck[0], affected_card_controller, affected_card) ){
				event_result++;
			}
		}
	}

	return 0;
}

int card_darkblast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return dredge(player, card, event, 3);
}

void dark_confidant_effect(int player, int card, int t_player){
	int *deck = deck_ptr[t_player];
	if( deck[0] != -1 ){
		int card_added = add_card_to_hand(t_player, deck[0] );
		remove_card_from_deck( t_player, 0 );
		int cmc = get_cmc(t_player, card_added);
		lose_life(t_player, cmc);
		reveal_card(player, card, t_player, card_added);
	}
}

int card_dark_confidant(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		dark_confidant_effect(player, card, player);
	}

	return 0;
}

int card_devouring_light(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST){
		if( has_convoked_mana_extended(player, card, EVENT_CAN_CAST, MANACOST_XW(1, 2)) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_IN_COMBAT;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int result = charge_convoked_mana_extended(player, card, event, MANACOST_XW(1, 2));
			if( result & (1<<31) ){
				td.allow_cancel = 0;
			}
		}
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature.", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dimir_cutpurse(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_cards(player, 1);
		discard(1-player, 0, player);
	}

	return 0;
}

static int effect_dimir_doppelganger(int player, int card, event_t event);
static int dimir_doppelganger_impl(int player, int card, event_t event, int is_effect)
{
  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card), *parent;
  int p, c;
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  p = instance->parent_controller;
	  c = instance->parent_card;
	  parent = get_card_instance(p, c);
	}
  else
	{
	  p = player;
	  c = card;
	  parent = instance;
	}

  if (is_effect)
	{
	  p = parent->damage_target_player;
	  c = parent->damage_target_card;
	}

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(p, c, MANACOST_XUB(1,1,1)) && !graveyard_has_shroud(ANYBODY) && has_dead_creature(ANYBODY);

  if (event == EVENT_ACTIVATE
	  && charge_mana_for_activated_ability(p, c, MANACOST_XUB(1,1,1))
	  && select_target_from_either_grave(player, card, 0, AI_MAX_VALUE, AI_MAX_VALUE, &test, 1, 2) == -1)
	cancel = 1;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int selected = validate_target_from_grave_source(player, card, instance->targets[1].player, 2);
	  if (selected != -1)
		{
		  int iid = get_grave(instance->targets[1].player)[selected];
		  rfg_card_from_grave(instance->targets[1].player, selected);

		  if (in_play(p, c))
			{
			  if (!is_effect)
				create_targetted_legacy_activate(p, c, effect_dimir_doppelganger, p, c);

			  cloning_and_verify_legend(p, c, -1, iid);
			}
		}
	}

  return 0;
}
static int effect_dimir_doppelganger(int player, int card, event_t event)
{
  return dimir_doppelganger_impl(player, card, event, 1);
}
int card_dimir_doppelganger(int player, int card, event_t event)
{
  /* Dimir Doppelganger	|1|U|B
   * Creature - Shapeshifter 0/2
   * |1|U|B: Exile target creature card from a graveyard. ~ becomes a copy of that card and gains this ability. */

  cloning_card(player, card, event);

  return dimir_doppelganger_impl(player, card, event, 0);
}

int card_dimir_guildmage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_sorcery_be_played(player, event) ){
		if( has_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0) || has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			int choice = 0;
			if( instance->targets[0].player == 1-player ){
				choice = 1;
			}
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Target player draws\n Target player discard\n Cancel", 0);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				int blue = 1;
				int black = 0;
				if( choice == 1 ){
					blue = 0;
					black = 1;
				}
				charge_mana_for_activated_ability(player, card, 3, black, blue, 0, 0, 0);
				if( spell_fizzled != 1 ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->info_slot == 66 ){
				draw_cards(instance->targets[0].player, 1);
			}
			if( instance->info_slot == 67 ){
				discard(instance->targets[0].player, 0, player);
			}
		}
	}
	return 0;
}

int card_dimir_house_guard(int player, int card, event_t event)
{
  /* Dimir House Guard	|3|B
   * Creature - Skeleton 2/3
   * Fear
   * Sacrifice a creature: Regenerate ~.
   * Transmute |1|B|B */

  fear(player, card, event);

  if (IS_ACTIVATING(event) && (land_can_be_played & LCBP_REGENERATION))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  if (IS_AI(player))
		test.not_me = 1;

	  if (event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE0(player, card) && new_can_sacrifice_as_cost(player, card, &test))
		return can_regenerate(player, card);

	  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST0))
		new_sacrifice(player, card, player, SAC_AS_COST, &test);

	  if (event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card))
		regenerate_target(instance->parent_controller, instance->parent_card);
	}

  return transmute(player, card, event, MANACOST_XB(1,2), 4);
}

int card_dimir_infiltrator(int player, int card, event_t event){
	unblockable(player, card, event);
	return transmute(player, card, event, 1, 1, 1, 0, 0, 0, 2);
}

int card_dimir_machinations(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");

	}
	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int t_player = instance->targets[0].player;
				int amount = 3;
				if( count_deck(t_player) < amount ){
					amount = count_deck(t_player);
				}
				while( amount > 0 ){
						int selected = show_deck( player, deck_ptr[t_player], amount, "Select a card to remove", 0, 0x7375B0 );
						if( selected != -1 ){
							remove_card_from_deck(t_player, selected);
							amount--;
						}
						else{
							break;
						}
				}
				if( amount > 0  ){
					rearrange_top_x(t_player, player, amount);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return transmute(player, card, event, 1, 2, 0, 0, 0, 0, 3);
}

int card_dimir_signet(int player, int card, event_t event){
	return signet2(player, card, event, COLOR_BLACK, COLOR_BLUE);
}

int card_dogpile(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.required_state = TARGET_STATE_ATTACKING;

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, target_available(player, card, &td1));
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_doubling_season(int player, int card, event_t event){

  return global_enchantment(player, card, event);
}

int card_dream_leash(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT );
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;
	td1.required_state = TARGET_STATE_TAPPED;

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td1);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PERMANENT");
	}
	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				return card_confiscate(player, card, event);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	return 0;
}

int card_drift_of_phantasms(int player, int card, event_t event){
	return transmute(player, card, event, 1, 0, 2, 0, 0, 0, 3);
}

int card_drooling_groodion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE );
	td2.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0) && can_target(&td1) && can_target(&td2) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0);
			if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				pick_target(&td1, "TARGET_CREATURE");
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				new_pick_target(&td2, "TARGET_CREATURE", 1, 0);
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				instance->number_of_targets = 2;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td1, 0) ){
				pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			}
			if( validate_target(player, card, &td2, 1) ){
				pump_until_eot(player, instance->parent_card, instance->targets[1].player, instance->targets[1].card, -2, -2);
			}
	}

	return 0;
}

int card_dryads_caress(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 0, 1, 0);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 0, 0, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_permanents_by_type(player, TYPE_CREATURE));
		if( instance->targets[1].player == 2 ){
			manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_duskmantle_house_of_shadows(int player, int card, event_t event){
	/* Duskmantle, House of Shadow	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |U|B, |T: Target player puts the top card of his or her library into his or her graveyard. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && !(is_tapped(player, card)) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 1, 1, 0, 0, 0) && can_target(&td1) && can_use_activated_abilities(player, card) ){
			ai_choice = 1;
		}

		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Mill a player\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 0, 1, 1, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			 spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			if( valid_target(&td1) ){
				mill(instance->targets[0].player, 1);
			}
			parent->info_slot = 0;
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_elvish_skysweeper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_abilities = KEYWORD_FLYING;
	td.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 0) && can_target(&td) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_empty_the_catacombs(int player, int card, event_t event){

	/* Empty the Catacombs	|3|B
	 * Sorcery
	 * Each player returns all creature cards from his or her graveyard to his or her hand. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");

		int num_returned[2];
		num_returned[current_turn] = from_grave_to_hand_multiple(current_turn, &test);
		num_returned[1-current_turn] = from_grave_to_hand_multiple(1-current_turn, &test);

		if (player == AI && num_returned[player] == 0){
			ai_modifier -= 96;
		}

		ai_modifier += 12 * (num_returned[AI] - num_returned[HUMAN]);

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_excruciator(int player, int card, event_t event){
	my_damage_cannot_be_prevented(player, card, event);
	return 0;
}

int card_faiths_fetters(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	if( player == AI ){
		td.required_type = TYPE_CREATURE;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(player, 4);
			disable_nonmana_activated_abilities(instance->targets[0].player, instance->targets[0].card, 1);
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	if( in_play(player, card) && instance->targets[0].player != -1 && instance->targets[0].card != -1 ){
		if( is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ){
			cannot_attack(instance->targets[0].player, instance->targets[0].card, event);
			cannot_block(instance->targets[0].player, instance->targets[0].card, event);
		}
	}
	if( leaves_play(player, card, event) ){
		disable_nonmana_activated_abilities(instance->targets[0].player, instance->targets[0].card, 0);
	}
	return 0;
}

int card_farseek(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a, %s, %s, or %s card.",
																				SUBTYPE_PLAINS, SUBTYPE_ISLAND, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
		this_test.sub2 = get_hacked_subtype(player, card, SUBTYPE_ISLAND);
		this_test.sub3 = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);
		this_test.sub4 = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
		this_test.subtype_flag = F2_MULTISUBTYPE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY );
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_festival_of_the_guildpact(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			prevent_the_next_n_damage(player, card, player, -1, instance->info_slot, 0, 0, 0);
		}
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flamekin_zealot(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, SP_KEYWORD_HASTE);
	}
	return 0;
}

int card_fiery_conclusion(int player, int card, event_t event)
{
  /* Fiery Conclusion	|1|R	0x200dd38
   * Instant
   * As an additional cost to cast ~, sacrifice a creature.
   * ~ deals 5 damage to target creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 5);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_SAC_CREATURE_AS_COST, &td, "TARGET_CREATURE", 1, NULL);
}

int card_firemane_angel(int player, int card, event_t event){

	if( current_turn == player && event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		int mode = (1<<0) | (1<<3);
		char buffer[100];
		int pos = scnprintf(buffer, 100, " Gain 1 life\n");
		if( has_mana_multi(player, 6, 0, 0, 0, 2, 2) ){
			mode |= 1<<1;
			mode |= 1<<2;
			pos += scnprintf(buffer + pos, 100-pos, " Return Firemane Angel to play\n Gain life & Bring back FMAngel\n ");
		}
		pos += scnprintf(buffer + pos, 100-pos, " Pass\n");
		int choice = do_dialog(player, player, card, -1, -1, buffer, (mode & (1<<2)) ? 0 : 2);
		while( !((1<<choice) & mode) ){
				choice++;
		}
		if( choice == 0 || choice == 2 ){
			gain_life(player, 1);
		}
		if( choice == 1 || choice == 2 ){
			charge_mana_multi(player, 6, 0, 0, 0, 2, 2);
			if( spell_fizzled != 1 ){
				put_into_play(player, card);
				return -1;
			}
			else{
				return -2;
			}
		}
		return -2;
	}

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(player, 1);
	}

	return 0;
}

int card_fists_of_ironwood(int player, int card, event_t event){
	/* Fists of Ironwood	|1|G
	 * Enchantment - Aura
	 * Enchant creature
	 * When ~ enters the battlefield, put two 1/1 |Sgreen Saproling creature tokens onto the battlefield.
	 * Enchanted creature has trample. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 2);
				return generic_aura(player, card, event, player, 0, 0, KEYWORD_TRAMPLE, 0, 0, 0, 0);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else{
		return generic_aura(player, card, event, player, 0, 0, KEYWORD_TRAMPLE, 0, 0, 0, 0);
	}

	return 0;
}

int effect_flame_fusillade(int player, int card, event_t event)
{
  // Until end of turn, permanents you control gain "|T: This permanent deals 1 damate to target creature or player."

  // Local data usage:
  // targets[0]: Player or creature targeted to be dealt damaged.
  // targets[1]: Permanent being tapped to deal damage.
  // targets[2].player: Types of permanents that can be activated.  Must be set when effect is created.

  if (eot_trigger(player, card, event))
	kill_card(player, card, KILL_REMOVE);

  if (event != EVENT_CAN_ACTIVATE && event != EVENT_ACTIVATE && event != EVENT_RESOLVE_ACTIVATION)
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td_tap;
  default_target_definition(player, card, &td_tap, instance->targets[2].player);
  td_tap.allowed_controller = player;
  td_tap.preferred_controller = player;
  td_tap.illegal_abilities = 0;
  td_tap.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;

#define DECL_TGT(p, c, td_tgt)								\
  target_definition_t td_tgt;								\
  default_target_definition(p, c, &td_tgt, TYPE_CREATURE);	\
  td_tgt.zone = TARGET_ZONE_CREATURE_OR_PLAYER

#define ABILITY(p, c, event, td_tgt)	\
  generic_activated_ability(p, c, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td_tgt, "TARGET_CREATURE_OR_PLAYER")

  if (event == EVENT_CAN_ACTIVATE && !is_humiliated(player, card))
	{
	  int p = player, c;
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, instance->targets[2].player))
		  {
			DECL_TGT(p, c, td_tgt);
			if (ABILITY(p, c, event, td_tgt))
			  return 1;
		  }
	}

  if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;

	  char typetext[100], prompt[200];
	  if (ai_is_speculating == 1)
		prompt[0] = 0;
	  else
		scnprintf(prompt, 200, "Select %s you control.", type_text(typetext, 100, instance->targets[2].player, 0));

	  if (pick_next_target_noload(&td_tap, prompt))
		{
		  int p = instance->targets[0].player;
		  int c = instance->targets[0].card;

		  if (can_use_activated_abilities(p, c))
			{
			  DECL_TGT(p, c, td_tgt);
			  if (can_target(&td_tgt))
				{
				  instance->targets[1] = instance->targets[0];	// struct copy

				  card_instance_t* activating = get_card_instance(p, c);
				  int old_number_of_targets = activating->number_of_targets;	// since select_targets() overwrites it
				  load_text(0, "TARGET_CREATURE_OR_PLAYER");
				  if (select_target(p, c, &td_tgt, text_lines[0], &instance->targets[0]))
					{
					  activating->number_of_targets = old_number_of_targets;
					  instance->number_of_targets = 1;
					  tap_card(p, c);
					  return 0;
					}
				  activating->number_of_targets = old_number_of_targets;
				}
			}
		}
	  instance->number_of_targets = 0;
	  spell_fizzled = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int p = instance->targets[1].player;
	  int c = instance->targets[1].card;
	  DECL_TGT(p, c, td_tgt);

	  if (validate_arbitrary_target(&td_tgt, instance->targets[0].player, instance->targets[0].card))
		damage_creature(instance->targets[0].player, instance->targets[0].card, 1, p, c);
	}

  return 0;
#undef DECL_TGT
#undef ABILITY
}

int card_flame_fusillade(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int leg = create_legacy_activate(player, card, &effect_flame_fusillade);
	  if (leg >= 0)
		get_card_instance(player, leg)->targets[2].player = TYPE_PERMANENT;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

static int flash_conscript_damage_effect(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_source_card == c && damage->damage_source_player == p ){
					if( damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE) ){
						int good = damage->info_slot;
						if( good < 1){
							card_instance_t *trg = get_card_instance(p, c);
							if( trg->targets[16].player > 0 ){
								good = trg->targets[16].player;
							}
						}

						if( good > 0 ){
							if( instance->targets[1].player < 0 ){
								instance->targets[1].player = 0;
							}
							instance->targets[1].player+=good;
						}
					}
				}
			}
		}

		if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					gain_life(player, instance->targets[1].player);
					instance->targets[1].player = 0;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_flash_conscript(int player, int card, event_t event){

	ravnica_manachanger(player, card, event, 0, 0, 0, 0, 1);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = instance->targets[1].player = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			charge_changed_mana(player, card, 0, 0, 0, 0, 1);
		}
		if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
			instance->targets[1].player = mana_paid[COLOR_WHITE] > 0 ? 2 : 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			if( instance->targets[1].player == 2 ){
				int l1 = create_targetted_legacy_effect(player, card, &flash_conscript_damage_effect, instance->targets[0].player, instance->targets[0].card);
				add_status(player, l1, STATUS_INVISIBLE_FX);
			}
			if( instance->targets[0].player == player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
			else{
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flight_of_fancy(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				draw_cards(player, 2);
				return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else{
		return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
	}

	return 0;
}

int card_flow_of_ideas(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_followed_footsteps(int player, int card, event_t event){
	/* Followed Footsteps	|3|U|U
	 * Enchantment - Aura
	 * Enchant creature
	 * At the beginning of your upkeep, put a token that's a copy of enchanted creature onto the battlefield. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->damage_target_player, instance->damage_target_card);
			generate_token(&token);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_frenzied_goblin(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may pay |R. If you do, target creature can't block this turn.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && has_mana(player, COLOR_RED, 1))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  if (can_target(&td)
		  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)
		  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_RED, 1))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  instance->number_of_targets = 0;
		  if (pick_target(&td, "TARGET_CREATURE"))
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

  return 0;
}

int card_flickerform(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_XW(2, 2), 0, NULL, NULL);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			exile_permanent_and_auras_attached(player, instance->parent_card, instance->damage_target_player, instance->damage_target_card, EPAAC_STORE_AURAS_RETURN_EOT);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_gather_courage(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ghosts_of_the_innocent(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			int amount = damage->info_slot;
			damage->info_slot-=((amount+1)/2);
		}
	}
	return 0;
}

int card_glare_of_subdual(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE | TYPE_ARTIFACT);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST){
		return 1;
	}
	else if(event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
			return can_target(&td1);
	}
	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				select_target(player, card, &td, "Select a creature of yours to tap", NULL)
			){
				tap_card(instance->targets[0].player, instance->targets[0].card );
				select_target(player, card, &td1, "Select a creature / artifact to tap", NULL);
				instance->number_of_targets = 1;
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td1, 0) ){
				tap_card( instance->targets[0].player, instance->targets[0].card );
			}
	}
	return global_enchantment(player, card, event);
}

int card_gleancrawler(int player, int card, event_t event){
	hybrid(player, card, event);
	if( current_turn == player && eot_trigger(player, card, event) ){
		return_all_dead_this_turn_to_hand(player, TYPE_CREATURE);
	}
	return 0;
}

int card_glimpse_the_unthinkable(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 10);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_golgari_brownscale(int player, int card, event_t event){

	return dredge(player, card, event, 2);
}

int card_golgari_germination(int player, int card, event_t event){
	/* Golgari Germination	|1|B|G
	 * Enchantment
	 * Whenever a nontoken creature you control dies, put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.type_flag = F1_NO_TOKEN;

		count_for_gfp_ability(player, card, event, player, 0, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = instance->targets[11].card;
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, amount);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_golgari_grave_troll(int player, int card, event_t event){
	/* Golgari Brownscale	|1|G|G
	 * Creature - Lizard 2/3
	 * When ~ is put into your hand from your graveyard, you gain 2 life.
	 * Dredge 2 */

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_TOUGHNESS && instance->targets[0].card != 3 && affect_me(player, card)){
		event_result++;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int count = count_graveyard_by_type(player, TYPE_CREATURE);
			add_1_1_counters(player, card, count);
			instance->targets[0].card = 3;
	}
	else if( event == EVENT_GRAVEYARD_ABILITY ){
		return 6;
	}
	else if( event == EVENT_CAN_ACTIVATE && count_1_1_counters(player, card) < 1 ){
		return 0;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			remove_1_1_counter(instance->parent_controller, instance->parent_card);
	}
	return regenerate(player, card, event, COLOR_COLORLESS, 1);
}

int card_golgari_guildmage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;;

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_sorcery_be_played(player, event) ){
		if( has_mana_for_activated_ability(player, card, 4, 1, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0){
			if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && graveyard_has_shroud(2) ){
				return 1;
			}
		}
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 0) ){
			return can_target(&td);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 4, 1, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 &&
				can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && graveyard_has_shroud(2)
			  ){
				if( current_phase >= PHASE_MAIN2 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Sac and Raise Dead\n Add +1/+1 counter\n Cancel", ai_choice);
			}
			if( choice == 0 ){
				charge_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 66;
					instance->number_of_targets = 1;
				}
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 4, 1, 0, 0, 0, 0);
					if( spell_fizzled != 1 ){
						int selected = select_a_card(player, player, TUTOR_FROM_GRAVE, 0, 1, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
						if( selected != -1 && sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
							const int *grave = get_grave(player);
							instance->targets[0].player = selected;
							instance->targets[0].card = grave[selected];
							instance->info_slot = 67;
						}
						else{
							spell_fizzled = 1;
						}
					}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
			if( instance->info_slot == 67 ){
				const int *grave = get_grave(player);
				int selected = instance->targets[0].player;
				if( instance->targets[0].card == grave[selected] ){
					add_card_to_hand(player, grave[selected]);
					remove_card_from_grave(player, selected);
				}
			}
	}
	return 0;
}

int card_golgari_rotwurm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) &&
		can_target(&td)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return 0;
}

int card_golgari_signet(int player, int card, event_t event){
	return signet2(player, card, event, COLOR_BLACK, COLOR_GREEN);
}

int card_golgari_thug(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 0  && ! graveyard_has_shroud(2) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_DECK, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return dredge(player, card, event, 4);
}

int card_graveshell_scarab(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return dredge(player, card, event, 1);
}

int card_greater_mossdog(int player, int card, event_t event){

	return dredge(player, card, event, 3);
}

int card_grozoth(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(4))){
		if (player == AI && !(get_abilities(player, card, EVENT_ABILITIES, -1) & KEYWORD_DEFENDER)){
			return 0;
		}
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(4));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int leg = pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 0, 0, KEYWORD_DEFENDER, 0);
		if (leg != -1){
			get_card_instance(instance->parent_controller, leg)->targets[4].player = 0;	// remove keyword
		}
	}

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		while( global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, 9, 0) != -1 ){
		}
	}
	return transmute(player, card, event, MANACOST_XU(1, 2), 9);
}

int card_gruul_signet(int player, int card, event_t event){
	// First printed in Guildpact, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_RED, COLOR_GREEN);
}

int card_guardian_of_vitu_ghazi(int player, int card, event_t event){

	vigilance(player, card, event);

	return generic_spell_with_convoke(player, card, event);
}

int card_hammerfist_giant(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, instance->parent_card, 2, 4, NDA_PLAYER_TOO, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_helldozer( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			if( ! is_basic_land(instance->targets[0].player, instance->targets[0].card) ){
				untap_card(player, instance->parent_card);
			}
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 3, 0, 0, 0, 0, 0, &td, "TARGET_LAND");
}

int card_hex(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && target_available(player, card, &td) > 5 ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		while(trgs < 6 ){
			if( new_pick_target(&td, "TARGET_PLAYER", trgs, 0) ){
				state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
				trgs++;
			}
		}
		int i;
		for(i=0; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		if( trgs < 6 ){
			instance->number_of_targets = 0;
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<6; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hour_of_reckoning(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_hunted_dragon(int player, int card, event_t event){
	/* Hunted Dragon	|3|R|R
	 * Creature - Dragon 6/6
	 * Flying, haste
	 * When ~ enters the battlefield, target opponent puts three 2/2 |Swhite Knight creature tokens with first strike onto the battlefield. */

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KNIGHT, &token);
		token.pow = token.tou = 2;
		token.t_player = 1-player;
		token.qty = 3;
		token.key_plus = KEYWORD_FIRST_STRIKE;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_hunted_horror(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_CENTAUR, &token);
		token.t_player = 1-player;
		token.qty = 2;
		token.key_plus = get_sleighted_protection(player, card, KEYWORD_PROT_BLACK);
		generate_token(&token);
	}

	return 0;
}

int card_hunted_lammasu(int player, int card, event_t event){
	/* Hunted Lammasu	|2|W|W
	 * Creature - Lammasu 5/5
	 * Flying
	 * When ~ enters the battlefield, target opponent puts a 4/4 |Sblack Horror creature token onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HORROR, &token);
		token.t_player = 1-player;
		token.color_forced = COLOR_TEST_BLACK;
		token.pow = 4;
		token.tou = 4;
		generate_token(&token);
	}
	return 0;
}

int card_hunted_phantasm(int player, int card, event_t event){
	/* Hunted Phantasm	|1|U|U
	 * Creature - Spirit 4/6
	 * ~ can't be blocked.
	 * When ~ enters the battlefield, target opponent puts five 1/1 |Sred Goblin creature tokens onto the battlefield. */

	unblockable(player, card, event);

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN, &token);
		token.t_player = 1-player;
		token.qty = 5;
		generate_token(&token);
	}

	return 0;
}

int card_hunted_troll(int player, int card, event_t event){
	/* Hunted Troll	|2|G|G
	 * Creature - Troll Warrior 8/4
	 * When ~ enters the battlefield, target opponent puts four 1/1 |Sblue Faerie creature tokens with flying onto the battlefield.
	 * |G: Regenerate ~. */

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_FAERIE, &token);
		token.t_player = 1-player;
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_BLUE;
		token.key_plus = KEYWORD_FLYING;
		token.qty = 4;
		generate_token(&token);
	}

	return regeneration(player, card, event, 0, 0, 0, 1, 0, 0);
}

int card_induce_paranoia(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 1, 0, 0, 0, 0);

	if( event == EVENT_CAN_CAST ){
		return card_counterspell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 1, 0, 0, 0, 0);
		}
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		if( instance->targets[1].player == 2 ){
			mill(instance->targets[0].player, cmc);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ivy_dancer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FORESTWALK, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_izzet_signet(int player, int card, event_t event){
	// First printed in Guildpact, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_BLUE, COLOR_RED);
}

int card_keening_banshee(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td1) && pick_target(&td1, "TARGET_CREATURE")){
		pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		instance->number_of_targets = 0;
	}

	return 0;
}

int card_last_gasp(int player, int card, event_t event){
	/*
	  Last Gasp |1|B
	  Instant
	  Target creature gets -3/-3 until end of turn.
	*/
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -3, -3, 0, 0);
}

int card_leave_no_trace(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ENCHANTMENT");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int c1 = get_color(instance->targets[0].player, instance->targets[0].card);
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) && colors_shared(c1, get_color(i, count)) > 0 ){
							kill_card(i, count, KILL_DESTROY);
						}
						count--;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_life_from_the_loam(int player, int card, event_t event)
{
  /* Life from the Loam	|1|G
   * Sorcery
   * Return up to three target land cards from your graveyard to your hand.
   * Dredge 3 */

  if (event == EVENT_GRAVEYARD_ABILITY)
	return 3;

  if (!IS_CASTING(player, card, event))
	return 0;
  test_definition_t test;
  new_default_test_definition(&test, TYPE_LAND, "Select up to three target land cards.");
  return spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 3, &test, 0);
}

int card_light_of_sanction(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_player == player && damage->damage_target_player == player && damage->damage_target_card != -1 ){
				damage->info_slot = 0;
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_lightning_helix(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_lore_broker(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		APNAP(p,{
				draw_cards(p, 1);
				discard(p, 0, player);
				};
		);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_lurking_informant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	hybrid(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int choice = 0;
			if( player != AI ){
				int selected = show_deck( player, deck, 1, "Select a card to mill", 0, 0x7375B0 );
				if( selected == -1 ){
					choice = 1;
				}
			}
			else{
				if( get_base_value(-1, deck[0]) < 25 ){
					choice = 1;
				}
			}
			if( choice == 0 ){
				mill(instance->targets[0].player, 1);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_loxodon_hierarch(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		gain_life(player, 4);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int c = active_cards_count[player]-1;
		while( c > -1 ){
				if( c!= instance->parent_card && in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
					if( can_be_regenerated(player, c) ){
						regenerate_target( player, c );
					}
				}
				c--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_REGENERATION, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_mark_of_eviction(int player, int card, event_t event){

	/* Mark of Eviction	|U
	 * Enchantment - Aura
	 * Enchant creature
	 * At the beginning of your upkeep, return enchanted creature and all Auras attached to that creature to their owners' hands. */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && current_turn == player && upkeep_trigger(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) && has_subtype(i, count, SUBTYPE_AURA) ){
						card_instance_t *this = get_card_instance(i, count);
						if( this->targets[0].player == instance->targets[0].player &&
							this->targets[0].card == instance->targets[0].card
						  ){
							bounce_permanent(i, count);
						}
					}
					count--;
			}
		}
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_mindleech_mass(int player, int card, event_t event){
	/* Mindleech Mass	|5|U|B|B
	 * Creature - Horror 6/6
	 * Trample
	 * Whenever ~ deals combat damage to a player, you may look at that player's hand. If you do, you may cast a nonland card in it without paying that card's mana cost. */

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) && hand_count[1-player] > 0 ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card to cast for free.");
		this_test.type_flag = DOESNT_MATCH;
		this_test.can_legally_play = 1;

		play_card_in_opponent_hand_by_test(player, card, &this_test);
	}

	return 0;
}

int card_mindmoil(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int amount = hand_count[player];

		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to put on bottom.");

		while( hand_count[player] ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_FIRST_FOUND, -1, &test);
				put_on_bottom_of_deck(player, selected);
		}

		draw_cards(player, amount);
	}
	return global_enchantment(player, card, event);
}

int card_mnemonic_nexus(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<2; i++){
			reshuffle_grave_into_deck(i, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_moldervine_cloak(int player, int card, event_t event){
	/* Moldervine Cloak	|2|G
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +3/+3.
	 * Dredge 2 */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		return 2;
	}
	return generic_aura(player, card, event, player, 3, 3, 0, 0, 0, 0, 0);
}

int card_moonlight_bargain(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		int *deck = deck_ptr[player];
		int amount = 5;
		if( amount > count_deck(player) ){
			amount = count_deck(player);
		}
		if( amount > 0 ){
			int life_to_pay = 0;
			if( player != AI ){
				while( amount > 0 ){
						int selected = show_deck( player, deck, amount, "Pick a card to add to your hand", 0, 0x7375B0 );
						if( selected != -1 ){
							add_card_to_hand(player, deck[selected]);
							remove_card_from_deck(player, selected);
							life_to_pay+=2;
							amount--;
						}
						else{
							break;
						}
				}
			}
			else{
				while( life[player]-life_to_pay > 6 && amount > 0){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
						life_to_pay+=2;
						amount--;
				}
			}
			lose_life(player, life_to_pay);
			mill(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_moroii(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 1);
	}

	return 0;
}

int card_mortipede(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_LURE);
	}

	return generic_activated_ability(player, card, event, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_muddle_the_mixture(int player, int card, event_t event)
{
  /* Muddle the Mixture	|U|U
   * Instant
   * Counter target instant or sorcery spell.
   * Transmute |1|U|U */

  if (IS_ACTIVATING_FROM_HAND(event))
	return transmute(player, card, event, MANACOST_XU(1,2), 2);

  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY);

  return counterspell(player, card, event, &td, 0);
}

int card_necroplasm(int player, int card, event_t event){

	if( current_turn == player ){

		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			add_1_1_counter(player, card);
		}

		if( eot_trigger(player, card, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.cmc = count_1_1_counters(player, card);
			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		}
	}

	return dredge(player, card, event, 2);
}

// nightguard phalanx --> serra angel

int card_nightmare_void(int player, int card, event_t event){

	/* Nightmare Void	|3|B
	 * Sorcery
	 * Target player reveals his or her hand. You choose a card from it. That player discards that card.
	 * Dredge 2 */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_effect_coercion(&ec, &this_test);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return dredge(player, card, event, 2);
}

int card_nullmage_shepherd(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_state = TARGET_STATE_TAPPED;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td) > 3 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td1);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && tapsubtype_ability(player, card, 4, &td) ){
			if( pick_target(&td1, "DISENCHANT") ){
				instance->number_of_targets = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_nullstone_gargoyle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[2].player = get_storm_count()-(get_stormcreature_count(player)+get_stormcreature_count(1-player));
	}

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
		instance->targets[2].player++;
		if( instance->targets[2].player == 1 ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return 0;
}

int card_oathsworn_giant(int player, int card, event_t event){

	if (event == EVENT_ABILITIES){
		if (affect_me(player, card)){
			// Separate check, in case this (somehow) becomes a noncreature, and it (somehow) becomes relevant whether that noncreature has vigilance.
			vigilance(player, card, event);
		} else if (affected_card_controller == player
				   && affected_card != card
				   && in_play(player, card)
				   && in_play(affected_card_controller, affected_card)
				   && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
				   && !is_humiliated(player, card)){
			vigilance(affected_card_controller, affected_card, event);
		}
	}

	return boost_creature_type(player, card, event, -1, 0, 2, 0, BCT_CONTROLLER_ONLY);
}

int card_orzhov_signet(int player, int card, event_t event){
	// First printed in Guildpact, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_BLACK, COLOR_WHITE);
}

int card_overwhelm2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 3, 3, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_pariahs_shield(int player, int card, event_t event){

	/* Pariah's Shield	|5
	 * Artifact - Equipment
	 * All damage that would be dealt to you is dealt to equipped creature instead.
	 * Equip |3 */

	card_instance_t *instance = get_card_instance( player, card );

	if( is_equipping(player, card) ){
		pariah_effect(player, -1, event, instance->targets[8].player, instance->targets[8].card);
	}

	return basic_equipment(player, card, event, 3);
}

int card_putrefy(int player, int card, event_t event){
	/*
	  Putrefy |1|B|G
	  Instant
	  Destroy target artifact or creature. It can't be regenerated.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target target creature or artifact.", 1, NULL);
}

int card_phytohydra(int player, int card, event_t event){

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->damage_target_player == player && damage->damage_target_card == card && damage->info_slot > 0 ){
		   int quantity = damage->info_slot;
		   int trampling = 0;
		   if( get_abilities(damage->damage_source_player, damage->damage_source_card, EVENT_ABILITIES, -1) & KEYWORD_TRAMPLE){
			  if( get_toughness(player, card) < quantity){
				 trampling = quantity - get_toughness(player, card);
				 quantity-=trampling;
			  }
		   }
		   add_1_1_counters(player, card, quantity);
		   damage->info_slot = 0;
		   if( trampling > 0 ){
			  damage_player(damage->damage_target_player, trampling,
							damage->damage_source_player, damage->damage_source_card);
		   }
		}
	}

 return 0;
}

int card_plague_boiler(int player, int card, event_t event){

	/* Plague Boiler	|3
	 * Artifact
	 * At the beginning of your upkeep, put a plague counter on ~.
	 * |1|B|G: Put a plague counter on ~ or remove a plague counter from it.
	 * When ~ has three or more plague counters on it, sacrifice it. If you do, destroy all nonland permanents. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_PLAGUE);
	}

	if( event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_PLAGUE) >= 3 ){
			kill_card(player, card, KILL_SACRIFICE);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;
			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int choice = 0;
		if( count_counters(player, card, COUNTER_PLAGUE) > 0 ){
			choice = do_dialog(player, player, card, -1, -1, " Add a counter\n Remove a counter", 0);
		}
		if( choice == 0 ){
			add_counter(player, card, COUNTER_PLAGUE);
		}
		else{
			remove_counter(player, card, COUNTER_PLAGUE);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XBG(1,1,1), 0, NULL, NULL);
}

int card_pollenbright_wings(int player, int card, event_t event)
{
  /* Pollenbright Wings	|4|G|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has flying.
   * Whenever enchanted creature deals combat damage to a player, put that many 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

  if (attached_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_REPORT_DAMAGE_DEALT))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int saprolings = instance->targets[16].player;
	  instance->targets[16].player = 0;
	  generate_tokens_by_id(player, card, CARD_ID_SAPROLING, saprolings);
	}

  return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_primordial_sage(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_privileged_position(int player, int card, event_t event){

	hybrid(player, card, event);

	if (event == EVENT_ABILITIES
		&& affected_card_controller == player
		&& affected_card != card
		&& in_play(player, card)
		&& in_play(affected_card_controller, affected_card)
		&& is_what(affected_card_controller, affected_card, TYPE_PERMANENT)
	   ){
		hexproof(affected_card_controller, affected_card, event);
	}

	return global_enchantment(player, card, event);
}

int card_psychic_drain(int player, int card, event_t event){
	return generic_x_spell(player, card, event, TARGET_ZONE_PLAYERS, 0, 16392);
}

int card_rain_of_embers(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, 2, 1, NDA_ALL_CREATURES+NDA_PLAYER_TOO, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_rakdos_signet(int player, int card, event_t event){
	// First printed in Dissension, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_BLACK, COLOR_RED);
}

int card_rally_the_righteous(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			radiance_pump(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0, 0, 0, ACT_UNTAP);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_razias_purification(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( can_sacrifice(player, i, 1, TYPE_PERMANENT, 0) ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_PERMANENT);
				td.allowed_controller = i;
				td.preferred_controller = i;
				td.illegal_abilities = 0;
				td.who_chooses = i;
				td.allow_cancel = 0;

				if( target_available(player, card, &td) > 3 ){
					int k;
					for(k=0; k<3; k++){
						pick_target(&td, "TARGET_PERMANENT");
						instance->number_of_targets = 1;
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
					}
					int count = active_cards_count[i]-1;
					while( count > -1 ){
							if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
								if( check_state(i, count, STATE_CANNOT_TARGET) ){
									state_untargettable(i, count, 0);
								}
								else{
									kill_card(i, count, KILL_SACRIFICE);
								}
							}
							count--;
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_razia_boros_archangel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			if( pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 2;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			prevent_the_next_n_damage(instance->parent_controller, instance->parent_card,
									  instance->targets[0].player, instance->targets[0].card,
									  3, PREVENT_REDIRECT, instance->targets[1].player, instance->targets[1].card);
		}
	}

	return 0;
}

int card_remand(int player, int card, event_t event){
	/*
	  Remand |1|U
	  Instant
	  Counter target spell. If that spell is countered this way, put it into its owner's hand instead of into that player's graveyard.
	  Draw a card.
	*/
	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( manage_counterspell_linked_hacks(player, card, instance->targets[0].player, instance->targets[0].card) != KILL_REMOVE ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ribbons_of_night(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 1, 0, 0, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 1, 0, 0, 0);
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			gain_life(player, 4);
			if( instance->targets[1].player == 2 ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rolling_spoil(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 1, 0, 0, 0, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 1, 0, 0, 0, 0);
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_LAND");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( instance->targets[1].player  == 2 ){
				pump_subtype_until_eot(player, card, 2, -1, -1, -1, 0, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sandsower(int player, int card, event_t event){
	/* Sandsower	|3|W
	 * Creature - Spirit 1/3
	 * Tap three untapped creatures you control: Tap target creature. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( target_available(player, card, &td) > 2 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && tapsubtype_ability(player, card, 3, &td) ){
			if( pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if(valid_target(&td1) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_savra_queen_of_the_golgari(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == player ){
		card_instance_t* affected = in_play(affected_card_controller, affected_card);
		if (affected && is_what(affected_card_controller, affected_card, TYPE_PERMANENT) &&
			affected->kill_code == KILL_SACRIFICE && !check_special_flags(affected_card_controller, affected_card, SF_KILL_STATE_BASED_ACTION)
		   ){
			int colors = get_color(affected_card_controller, affected_card);
			int black = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			int green = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
			if (!(colors & (black | green))){
				return 0;
			}
			if( instance->targets[11].player < 0 ){
				instance->targets[11].player = 0;
			}
			int pos = instance->targets[11].player;
			if( pos < 10 ){
				instance->targets[pos].player = (colors & black) ? 1 : 0;
				instance->targets[pos].player |= (colors & green) ? 2 : 0;
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int cpl = can_pay_life(player, 1);
		int colors[10];
		/* Store previous triggers and clear them, in case the sacrifice this imposes causes another sacrifice of a black/green creature - consider
		 * Abyssal Gatekeeper */
		int i, num = instance->targets[11].player;
		instance->targets[11].player = 0;
		for (i = 0; i < num; ++i){
			colors[i] = instance->targets[i].player;
		}
		for (i = 0; i < num; ++i){
			int choice;
			switch (colors[i]){
				case 1:	// black
					if (cpl && life[player] >= 1){
						choice = do_dialog(player, player, card, -1, -1, " Pay 2 life\n _Gain 2 life\n _Both\n Pass", life[player] >= 8 ? 0 : 3);
					} else {
						choice = 0;
					}
					break;
				case 3:	// black and green
					if (cpl && life[player] >= 2){
						choice = do_dialog(player, player, card, -1, -1, " Pay 2 life\n Gain 2 life\n Both\n Pass", 2);
						break;
					}
					// else fall through
				case 2:	// green
					choice = do_dialog(player, player, card, -1, -1, " _Pay 2 life\n Gain 2 life\n _Both\n Pass", 1);
					break;

				default:
					return 0;
			}

			++choice;	// now 1 for pay life; 2 for gain life; 3 for both; 4 for neither
			if (choice & 1){
				lose_life(player, 2);
				impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			if (choice & 2){
				gain_life(player, 2);
			}
		}
	}
	return 0;
}

int card_scatter_the_seeds(int player, int card, event_t event){

	/* Scatter the Seeds	|3|G|G
	 * Instant
	 * Convoke
	 * Put three 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_scion_of_the_wild(int player, int card, event_t event)
{
  /* Crusader of Odric	|2|W
   * Creature - Human Soldier 100/100
   * ~'s power and toughness are each equal to the number of creatures you control. */
  /* Scion of the Wild	|1|G|G
   * Creature - Avatar 100/100
   * ~'s power and toughness are each equal to the number of creatures you control. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	event_result += creature_count[player];

  return 0;
}

// searing meditation is a placeholder, the actual one is in "gain_life" in "functions.c"

int card_seed_spark(int player, int card, event_t event){
	/* Seed Spark	|3|W
	 * Instant
	 * Destroy target artifact or enchantment. If |G was spent to cast ~, put two 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 1, 0, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[1].player = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 1, 0, 0);
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "DISENCHANT");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( instance->targets[1].player == 2 ){
				generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_seeds_of_strength(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int i;
		for(i=0; i<3; i++){
			if(i>0){
				td.allow_cancel = 0;
				select_target(player, card, &td, "Select target creature.", &(instance->targets[i]));
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<3; i++){
			if( validate_target(player, card, &td, i) ){
				pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 1, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_seismic_peak(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			produce_mana(player, COLOR_RED, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_selesnya_evangel(int player, int card, event_t event){//UNUSEDCARD
	/* Selesnya Evangel	|G|W
	 * Creature - Elf Shaman 1/2
	 * |1, |T, Tap an untapped creature you control: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && target_available(player, card, &td1) > 1 ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		state_untargettable(player, card, 1);
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td1, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			tap_card(instance->targets[0].player, instance->targets[0].card);
			tap_card(player, card);
		}
		state_untargettable(player, card, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}

	return 0;
}

int card_selesnya_guildmage(int player, int card, event_t event){
	/* Selesnya Guildmage	|GW|GW
	 * Creature - Elf Wizard 2/2
	 * |3|G: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * |3|W: Creatures you control get +1/+1 until end of turn. */

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 0) ||  has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 1) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		int green  = 1;
		int white = 1;
		if( current_phase > PHASE_DECLARE_ATTACKERS && current_phase < PHASE_MAIN2 && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 1) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 1) ){
				choice = do_dialog(player, player, card, -1, -1, " Generate a Saproling\n Pump your guys\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2){
			spell_fizzled = 1;
		}
		else if( choice == 0){
				white=0;
		}
		else if( choice == 1 ){
				green= 0;
		}
		charge_mana_for_activated_ability(player, card, 3, 0, 0, green, 0, white);
		if( spell_fizzled != 1 ){
			instance->info_slot = 66+choice;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				generate_token_by_id(player, card, CARD_ID_SAPROLING);
			}
			if( instance->info_slot == 67 ){
				pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 1, 0, 0);
			}
	}
	return 0;
}

int card_selesnya_signet(int player, int card, event_t event){
	return signet2(player, card, event, COLOR_WHITE, COLOR_GREEN);
}

int card_shadow_of_doubt(int player, int card, event_t event){

	hybrid(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		create_id_legacy(player, card, -1, -1, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_shambling_shell(int player, int card, event_t event){
	/* Shambling Shell	|1|B|G
	 * Creature - Plant Zombie 3/1
	 * Sacrifice ~: Put a +1/+1 counter on target creature.
	 * Dredge 3 */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		return 3;
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_shred_memory(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return ((count_graveyard(player) > 0 && !graveyard_has_shroud(player))
				|| (count_graveyard(1-player) > 0 && !graveyard_has_shroud(1-player)));
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (pick_target(&td, "TARGET_PLAYER")){
			select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MIN_VALUE : AI_MAX_VALUE, NULL, 4, &instance->targets[1]);
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for (i = 1; i <= 4; ++i){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
			if (selected != -1){
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return transmute(player, card, event, MANACOST_XB(1, 2), 2);
}

int card_simic_signet(int player, int card, event_t event){
	// First printed in Dissension, but here to keep local to the other signets.
	return signet2(player, card, event, COLOR_BLUE, COLOR_GREEN);
}

int card_sins_of_the_past(int player, int card, event_t event){


	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_SPELL) >  0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select target sorcery or instant card.");
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			create_spell_has_flashback_legacy(player, card, selected, FBL_NO_MANA_COST);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

static const char* is_blocking_ssd(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(card, targeting_card))
	return NULL;
  else
	return "must be blocking Sisters of the Stone Death";
}

static const char* ssd_is_blocking_him(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(targeting_card, card))
	return NULL;
  else
	return "must be blocked by Sisters of the Stone Death";
}

int card_sisters_of_stone_death(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller =  1-player;
	td2.preferred_controller = 1-player;
	td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td2.extra = (int32_t)is_blocking_ssd;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller =  1-player;
	td1.preferred_controller = 1-player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int32_t)ssd_is_blocking_him;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( current_phase < PHASE_DECLARE_BLOCKERS && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_CREATURE") ){
			return 1;
		}
		if( current_turn == player ){
			if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td2, "TARGET_CREATURE") ){
				return 1;
			}
		}
		if( current_turn != player ){
			if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td1, "TARGET_CREATURE") ){
				return 1;
			}
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(2, 1), 0, NULL, NULL) &&
			exiledby_choose(player, card, CARD_ID_SISTERS_OF_STONE_DEATH, EXBY_MAX_VALUE, 0, "creature", 1)
		  ){
			return 1;
		}
	}

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){

		int can_force_block = current_phase < PHASE_DECLARE_BLOCKERS && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_CREATURE");
		int can_exile_blocked_blocking_creature = 0;
		if( current_turn == player ){
			can_exile_blocked_blocking_creature = (current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td2, "TARGET_CREATURE"));
		}
		else{
			can_exile_blocked_blocking_creature = (current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td1, "TARGET_CREATURE"));
		}
		int can_return_exiled_creature = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XB(2, 1), 0, NULL, NULL) && exiledby_choose(player, card, CARD_ID_SISTERS_OF_STONE_DEATH, EXBY_MAX_VALUE, 0, "creature", 1);

		int priority_force_block = current_phase < PHASE_DECLARE_BLOCKERS && can_force_block ? 10 : 0;
		int priority_exile_blocked_blocking_creature = current_phase == PHASE_AFTER_BLOCKING && can_exile_blocked_blocking_creature ? 15 : 0;
		int priority_return_exiled_creature = current_phase < PHASE_DECLARE_BLOCKERS && can_return_exiled_creature ? 20 : 0;
		enum{
			CHOICE_FBLOCK = 1,
			CHOICE_EXILE_BLOCK = 2,
			CHOICE_RETURN_C = 3,
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Force a creature to block", can_force_block, priority_force_block, DLG_MANA(MANACOST_G(1)),
						"Exile a creature blocking / blocked by me", can_exile_blocked_blocking_creature, priority_exile_blocked_blocking_creature, DLG_MANA(MANACOST_BG(1, 1)),
						"Return an exiled creature", can_return_exiled_creature, priority_return_exiled_creature, DLG_MANA(MANACOST_XB(2, 1))
						);

		if (event == EVENT_ACTIVATE){
				switch (choice){
					case CHOICE_FBLOCK:
					{
						if( ! select_target(player, card, &td, "Select a creature that will be forced to block this", &(instance->targets[0])) ){
							spell_fizzled = 1;
							return 0;
						}
						break;

					}
					case CHOICE_EXILE_BLOCK:
					{
						if( current_turn == player ){
							if( ! select_target(player, card, &td2, "Select a creature that is blocking Sisters of the Stone Death", &(instance->targets[0])) ){
								spell_fizzled = 1;
								return 0;
							}
						}
						else{
							if( ! select_target(player, card, &td1, "Select a creature that is blocked by Sisters of the Stone Death", &(instance->targets[0])) ){
								spell_fizzled = 1;
								return 0;
							}
						}
						break;
					}
					case CHOICE_RETURN_C:
					{
						int rval = exiledby_choose(player, card, CARD_ID_SISTERS_OF_STONE_DEATH, EXBY_CHOOSE, 0, "creature", 1);
						int* loc = (int*)rval;
						if (!loc)
						  spell_fizzled = 1;
						else
						  {
							instance->targets[1].player = -1;
							instance->targets[1].card = *loc;
						  }
							break;
					}

				}
		}
		else	// event == EVENT_RESOLVE_ACTIVATION
			{
			  switch (choice)
				{
				  case CHOICE_FBLOCK:
					{
						if( valid_target(&td) ){
							target_must_block_me(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
						}
						break;
					}

				  case CHOICE_EXILE_BLOCK:
					{
						if( (current_turn == player && valid_target(&td2)) || (current_turn != player && valid_target(&td1)) ){
							exile_card_and_remember_it_on_exiledby(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
						}
						break;
					}
				  case CHOICE_RETURN_C:
					{
					  int* loc = exiledby_find(player, instance->parent_card, instance->targets[1].card, NULL, NULL);
					  if (!loc)
						spell_fizzled = 1;
					  else
						{
						  *loc = -1;
						  int owner = (instance->targets[1].card & 0x80000000) ? 1 : 0;
						  int iid = instance->targets[1].card & ~0x80000000;
						  if (remove_card_from_rfg(owner, cards_data[iid].id))
							{
							  int nightmare = add_card_to_hand(player, iid);
							  if (player != owner)
								get_card_instance(player, nightmare)->state ^= STATE_OWNED_BY_OPPONENT;
							  create_targetted_legacy_effect(player, instance->parent_card, empty, player, nightmare);
							  put_into_play(player, nightmare);
							}
						  else
							spell_fizzled = 1;
						}
						break;
					}

				}
		}
	}
	return 0;
}

int card_smash2(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			draw_cards(player, 1);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sparkmage_apprentice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return 0;
}

int card_spawnbroker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE);
			td1.allowed_controller = 1-player;
			td1.preferred_controller = 1-player;
			td1.power_requirement = get_power(instance->targets[0].player, instance->targets[0].card) | TARGET_PT_LESSER_OR_EQUAL;
			if( can_target(&td1) && new_pick_target(&td1, "TARGET_CREATURE", 1, 0) ){
				exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
														instance->targets[1].player, instance->targets[1].card);
			}
		}
	}

	return 0;
}

int card_stinkweed_imp(int player, int card, event_t event){
	deathtouch(player, card, event);
	return dredge(player, card, event, 5);
}

int card_stoneseeder_hierophant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( is_tapped(player, card) && specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_LAND");
}

int card_stoneshaker_shaman(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = current_turn;
	td.preferred_controller = current_turn;
	td.illegal_abilities = 0;
	td.who_chooses = current_turn;
	td.allow_cancel = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance( player, card );

	if( eot_trigger(player, card, event) && can_target(&td) && can_sacrifice(player, current_turn, 1, TYPE_LAND, 0) ){
		if( pick_target(&td, "TARGET_LAND") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			instance->number_of_targets = 1;
		}
	}

	return 0;
}

int card_strands_of_undeath(int player, int card, event_t event)
{
  // When ~ enters the battlefield, target player discards two cards.
  if (comes_into_play(player, card, event))
	{
	  target_definition_t td1;
	  default_target_definition(player, card, &td1, 0);
	  td1.zone = TARGET_ZONE_PLAYERS;
	  td1.allow_cancel = 0;

	  if (can_target(&td1) && new_pick_target(&td1, "TARGET_PLAYER", 1, 1))
		new_multidiscard(get_card_instance(player, card)->targets[1].player, 2, 0, player);
	}

  // |B: Regenerate enchanted creature.
  if (land_can_be_played & LCBP_REGENERATION)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (instance && instance->damage_target_player != -1)
		{
		  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_B(1))
			  && can_regenerate(instance->damage_target_player, instance->damage_target_card))
			return 99;

		  if (event == EVENT_ACTIVATE)
			charge_mana_for_activated_ability(player, card, MANACOST_B(1));

		  if (event == EVENT_RESOLVE_ACTIVATION)
			regenerate_target(instance->damage_target_player, instance->damage_target_card);
		}
	}

  return vanilla_aura(player, card, event, player);
}

int card_sundering_vitae(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "DISENCHANT") ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, 1);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sunforger(int player, int card, event_t event){

	/* Sunforger	|3
	 * Artifact - Equipment
	 * Equipped creature gets +4/+0.
	 * |R|W, Unattach ~: Search your library for a |Sred or |Swhite instant card with converted mana cost 4 or less and cast that card without paying its mana
	 * cost. Then shuffle your library.
	 * Equip |3 */

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 4, 0, 0);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 1) ){
			return 1;
		}
		else{
			return can_activate_basic_equipment(player, card, event, 3);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( can_activate_basic_equipment(player, card, event, 3) ){
				if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 1) ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Play a free instant\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 3);
				instance->info_slot = 66;
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 1);
					if( spell_fizzled != 1 ){
						unattach(player, card);
						instance->info_slot = 67;
					}
					instance->number_of_targets = 0;
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 67 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_INSTANT);
				this_test.color = COLOR_TEST_RED | COLOR_TEST_WHITE;
				this_test.cmc = 5;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
				int result = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
				if (result == -1){
					spell_fizzled = 1;
				}
				else{
					play_card_in_deck_for_free(player, player, result);
				}
			}
	}
	return 0;
}

int card_sunhome_enforcer(int player, int card, event_t event){
	spirit_link_effect(player, card, event, player);
	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_sunhome_fortress_of_the_legion(int player, int card, event_t event){
	/* Sunhome, Fortress of the Legion	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |2|R|W, |T: Target creature gains double strike until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 1, 1) && can_use_activated_abilities(player, card) && can_target(&td) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Give Double Strike\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				instance->info_slot = 1;
			}
			else{
				 untap_card_no_event(player, card);
			}
		}

		if( choice == 2 ){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td) ){
					pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
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

int card_suppression_field(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		set_cost_mod_for_activated_abilities(2, -1, 3, &this_test);
	}

	if( event == EVENT_CAST_SPELL ){
		if( is_what(affected_card_controller, affected_card, TYPE_PERMANENT) ){
			set_cost_mod_for_activated_abilities(affected_card_controller, affected_card, 3, 0);
		}
	}


	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		remove_cost_mod_for_activated_abilities(2, -1, 3, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_surge_of_zeal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			radiance_pump(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_svoghtos_the_restless_tomb(int player, int card, event_t event){
	return manland_normal(player, card, event, 3, 1, 0, 1, 0, 0);
}

int card_svoghtos_animated(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1){
		event_result += count_graveyard_by_type(player, TYPE_CREATURE);
	}
	if (card == -1){	// Not on bf.  Though I can't imagine how the animated version would be called in that case.
		return 0;
	}
	return manland_animated(player, card, event, 3, 1, 0, 1, 0, 0);
}

int card_szadeck_lord_of_secrets(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	damage_effects(player, card, event);
	return 0;
}

int card_telling_time(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int amount = 3;
		if( amount > count_deck(player) ){
			amount = count_deck(player);
		}

		char msg[100] = "Select a card to put on your hand.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		this_test.create_minideck = amount;
		this_test.no_shuffle = 1;

		if( amount > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
			amount--;
			if( amount > 0 ){
				strcpy(msg, "Select a card to put on bottom of deck.");
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.create_minideck = amount;
				this_test.no_shuffle = 1;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_BOTTOM_OF_DECK, 1, AI_MIN_VALUE, &this_test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_thoughtpicker_witch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if(  count_deck(instance->targets[0].player) > 0 ){
				global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK+11, TUTOR_RFG, 1, AI_MAX_VALUE, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_three_dreams(int player, int card, event_t event){

	/* Three Dreams	|4|W
	 * Sorcery
	 * Search your library for up to three Aura cards with different names, reveal them, and put them into your hand. Then shuffle your library. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if (event == EVENT_RESOLVE_SPELL ){
		int count = 0;
		int tutored[3] = {-1, -1, -1};
		int stop = 0;
		int *deck = deck_ptr[player];
		while( count < 3 && stop == 0 ){
				int selected = select_a_card(player, player, TUTOR_FROM_DECK, 0, 1, -1, TYPE_ENCHANTMENT, 0, SUBTYPE_AURA, 0, 0, 0, 0, 0, -1, 0);
				if( selected != -1 ){
					int i;
					for(i=0; i<3; i++){
						if( tutored[i] == cards_data[deck[selected]].id ){
							stop = 1;
						}
					}
					if( stop != 1 ){
						tutored[count] = cards_data[deck[selected]].id;
						add_card_to_hand(player, deck[selected]);
						remove_card_from_deck(player, selected);
						count++;
					}
				}
				else{
					stop = 1;
				}
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_thundersong_trumpeter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int leg = pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			if (leg >= 0){
				get_card_instance(instance->parent_controller, leg)->targets[3].player = PAUE_CANT_ATTACK;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_tolsimir_wolfblood(int player, int card, event_t event){
	/* Tolsimir Wolfblood	|4|G|W
	 * Legendary Creature - Elf Warrior 3/4
	 * Other |Sgreen creatures you control get +1/+1.
	 * Other |Swhite creatures you control get +1/+1.
	 * |T: Put a legendary 2/2 |Sgreen and |Swhite Wolf creature token named Voja onto the battlefield. */

	// original code : 004E0980

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	boost_creature_by_color(player, card, event, COLOR_TEST_GREEN, 1, 1, 0, BCT_CONTROLLER_ONLY);

	boost_creature_by_color(player, card, event, COLOR_TEST_WHITE, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( player == AI && upkeep_trigger(player, card, event) ){
		instance->targets[12].player = check_battlefield_for_id(player, CARD_ID_VOJA);
	}


	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		if( player == HUMAN ){
			return result;
		}
		if( player == AI ){
			if( instance->targets[12].player != 1 && result ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_VOJA);
	}

	return 0;
}

static int transluminant_legacy(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_transluminant(int player, int card, event_t event){
	/* Transluminant	|1|G
	 * Creature - Dryad Shaman 2/2
	 * |W, Sacrifice ~: Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield at the beginning of the next end step. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		create_legacy_effect(player, card, &transluminant_legacy);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_trophy_hunter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( damage->damage_source_card == card && damage->damage_source_player == player ){
				if( instance->targets[0].player < 10 ){
					if( instance->targets[0].player < 1 ){
						instance->targets[0].player = 1;
					}
					int pos = instance->targets[0].player;
					instance->targets[pos].player = damage->damage_target_player;
					instance->targets[pos].card = damage->damage_target_card;
					instance->targets[0].player++;
				}
			}
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
			){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[0].player > 0 ){
					int i;
					for(i=0; i<instance->targets[0].player; i++){
						if( instance->targets[i+1].player == affected_card_controller &&
							instance->targets[i+1].card == affected_card
							){
							if( instance->targets[11].player < 0 ){
								instance->targets[11].player = 0;
							}
							instance->targets[11].player++;
							instance->targets[i+1].card = get_id(affected_card_controller, affected_card);
						}
					}
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
		){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			add_1_1_counters(player, card, instance->targets[11].player);
			instance->targets[11].player = 0;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_tunnel_vision(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int cd = count_deck(instance->targets[0].player);
			int *deck = deck_ptr[instance->targets[0].player];
			int id = cards_data[deck[internal_rand(cd)]].id;
			if( player == HUMAN ){
				id = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			int count = 0;
			int milled = 0;
			while( deck[count] != -1 ){
				if( cards_data[deck[count]].id == id ){
					milled = 1;
					break;
				}
				count++;
			}
			show_deck( HUMAN, deck, count+1, "Target player revealed these cards", 0, 0x7375B0 );
			if( milled == 1 && count > 0 ){
				mill(instance->targets[0].player, count);
			}
			else{
				shuffle(instance->targets[0].player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_twilight_drover(int player, int card, event_t event){

	/* Twilight Drover	|2|W
	 * Creature - Spirit 1/1
	 * Whenever a creature token leaves the battlefield, put a +1/+1 counter on ~.
	 * |2|W, Remove a +1/+1 counter from ~: Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. */

	if( trigger_condition == TRIGGER_LEAVE_PLAY && affect_me( player, card) && reason_for_trigger_controller == player ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && is_token(trigger_cause_controller, trigger_cause) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.qty = 2;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_XW(2,1), 0, NULL, NULL);
}

int card_twisted_justice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
			int result = pick_creature_for_sacrifice(instance->targets[0].player, card, 1);
			if( result != -1 ){
				draw_cards(player, get_power(instance->targets[0].player, result));
				kill_card(instance->targets[0].player, result, KILL_SACRIFICE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ursapine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_vedalken_dismisser(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI && count_subtype(1-player, TYPE_CREATURE, -1) < 1 ){
		ai_modifier-=200;
	}

	if( comes_into_play(player, card, event) && can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
		put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
	}

	return 0;
}

int card_veteran_armorer(int player, int card, event_t event){
	/*
	  Veteran Armorer |1|W
	  Creature - Human Soldier 2/2
	  Other creatures you control get +0/+1.
	*/
	boost_creature_type(player, card, event, -1, 0, 1, 0, BCT_CONTROLLER_ONLY);
	return 0;
}

// viashino fangtail --> prodigal sorcerer

int card_vigor_mortis(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 1, 0, 0);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 1, 0, 0);
		}
		if( spell_fizzled != 1 ){
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(player);
		int selected = instance->targets[0].player;
		if( grave[selected] == instance->targets[0].card ){
			int zombo = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			if( instance->targets[1].player == 2 ){
				add_1_1_counter(player, zombo);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vinelasher_kudzu(int player, int card, event_t event){

	if( specific_cip(player, card, event, player, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_vitu_ghazi_the_city_tree(int player, int card, event_t event){
	/* Vitu-Ghazi, the City-Tree	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |2|G|W, |T: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 1) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Generate a Saproling\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 1);
			if( spell_fizzled != 1){
				instance->info_slot = 66;
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
			if( instance->info_slot == 66 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				generate_token_by_id(player, card, CARD_ID_SAPROLING);
				parent->info_slot = 0;
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_voyager_staff(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && can_target(&td) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
	}

	return 0;
}

int card_vulturous_zombie(int player, int card, event_t event)
{
  /* Vulturous Zombie	|3|B|G
   * Creature - Plant Zombie 3/3
   * Flying
   * Whenever a card is put into an opponent's graveyard from anywhere, put a +1/+1 counter on ~. */

  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL))
	add_1_1_counter(player, card);

  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == 1-player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		add_1_1_counters(player, card, num_cards_milled);
	}

  return 0;
}

int card_warp_world(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int total = manipulate_all(player, card, i, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_PUT_ON_TOP);
			total += manipulate_all(player, card, i, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_PUT_ON_TOP);
			shuffle(i);
			int *deck = deck_ptr[i];
			if( i != player ){
				show_deck( HUMAN, deck, total, "Opponent revealed these", 0, 0x7375B0 );
			}
			else{
				show_deck( HUMAN, deck, total, "You revealed these", 0, 0x7375B0 );
			}
			int count = total-1;
			while( count > -1 ){
					if( is_what(-1, deck[count], TYPE_PERMANENT) && ! is_what(-1, deck[count], TYPE_ENCHANTMENT) ){
						put_into_play_a_card_from_deck(i, i, count);
						total--;
					}
					count--;
			}
			count = total-1;
			while( count > -1 ){
					if( is_what(-1, deck[count], TYPE_ENCHANTMENT) ){
						put_into_play_a_card_from_deck(i, i, count);
						total--;
					}
					count--;
			}
			if( total > 0 ){
				put_top_x_on_bottom(i, i, total);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_woebringer_demon(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int result = impose_sacrifice(player, card, current_turn, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( result < 1 && in_play(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_wojek_siren(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			radiance_pump(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1, 0, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_selesnya_sanctuary(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_GREEN, COLOR_WHITE, CARD_ID_SAVANNAH );
}

int card_golgari_rot_farm(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_GREEN, COLOR_BLACK, CARD_ID_BAYOU );
}

int card_boros_garrison(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_RED, COLOR_WHITE, CARD_ID_PLATEAU );
}

int card_dimir_aqueduct(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_BLACK, COLOR_BLUE, CARD_ID_UNDERGROUND_SEA );
}


