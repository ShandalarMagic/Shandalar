// Interactive searching and manipulation of libraries/graveyards/exile.

#include "manalink.h"

int show_deck_exe(int player, const int* deck, int num_cards, const char* prompt, int suppress_done_label, /* const char* */int done_label);

int show_deck(int player, const int* deck, int num_cards, const char* prompt, int suppress_done_label, /* const char* */int done_label)
{
	call_sub_437E20_unless_ai_is_speculating();	// force redisplay
	return show_deck_exe(player, deck, num_cards, prompt, suppress_done_label, done_label);
}

int get_average_value_from_zone(int targ_player, const int *s_location, int zone_count, test_definition_t *this_test, int my_test, int mode){
	int total = 0;
	int found = 0;
	int i;
	for(i=0; i<zone_count; i++){
		if( new_make_test(targ_player, s_location[i], my_test, this_test) ){
			found++;
			if( mode == AI_MAX_CMC || mode == AI_MIN_CMC || mode == AI_GOOD_TO_PUT_IN_GRAVE){
				total+=get_cmc_by_internal_id(s_location[i]);
			}
			if( mode == AI_MAX_VALUE || mode == AI_MIN_VALUE){
				total+=get_base_value(-1, s_location[i]);
			}
		}
	}

	return found ? total / found : 0;
}

const char* hack_override_show_deck_done_button_label = NULL;
int select_card_from_zone(int player, int targ_player, const int *s_location, int zone_count, int must_select, int ai_selection_mode, int test_type, test_definition_t *this_test){
	int selected = -1;
	if( player != AI ){
		while( 1 ){
				const char* label = hack_override_show_deck_done_button_label;
				if (!label){
					label = EXE_STR(0x7375B0);	// DIALOGBUTTONS[2] - "Done"
				}
				selected = show_deck( player, s_location, zone_count, this_test->message, 0, (int)label );
				if( must_select == 0 ){
					if( selected != -1 ){
						if( ! new_make_test( targ_player, s_location[selected], test_type, this_test)
						  ){
							selected = -1;
						}
						break;
					}
					else{
						 break;
					}
				}
				else{
					if( selected != -1 && new_make_test( targ_player, s_location[selected], test_type, this_test) ){
						break;
					}
				}
		}
		if( selected == -1 ){
			return -1;
		}
	}

	else{
		int count = 0;
		int av_value = get_average_value_from_zone(targ_player, s_location, zone_count, this_test, test_type, ai_selection_mode);
		int ai_pick[zone_count];
		int apc = 0;

		while( count < zone_count ){
				if( new_make_test( targ_player, s_location[count], test_type, this_test) ){
					if( ai_selection_mode == AI_FIRST_FOUND || (ai_selection_mode == AI_RANDOM && internal_rand(100)+1 > 50) ){
						selected = count;
						break;
					}

					if( ai_selection_mode == AI_GOOD_TO_PUT_IN_GRAVE ){
						if( good_to_put_in_grave(-1, s_location[count]) ){
							selected = count;
							break;
						}
					}

					int amount = -1;

					if( ai_selection_mode == AI_MAX_VALUE || ai_selection_mode == AI_MIN_VALUE ){
						if( this_test->has_mana_to_pay_cmc == 0 ||
							(this_test->has_mana_to_pay_cmc == 1 && has_mana(player, COLOR_COLORLESS, get_cmc_by_internal_id(s_location[count])))
						  ){
							amount = get_base_value(-1, s_location[count]);
						}
					}

					if( ai_selection_mode == AI_MAX_CMC || ai_selection_mode == AI_MIN_CMC || ai_selection_mode == AI_GOOD_TO_PUT_IN_GRAVE ){
						if( this_test->has_mana_to_pay_cmc == 0 ||
							(this_test->has_mana_to_pay_cmc == 1 && has_mana(player, COLOR_COLORLESS, get_cmc_by_internal_id(s_location[count])))
						  ){
							amount = get_cmc_by_internal_id(s_location[count]);
						}
					}

					if( (ai_selection_mode == AI_MAX_VALUE || ai_selection_mode == AI_MAX_CMC || ai_selection_mode == AI_GOOD_TO_PUT_IN_GRAVE) &&
						amount >= av_value
					  ){
						ai_pick[apc] = count;
						apc++;
					}
					if( (ai_selection_mode == AI_MIN_VALUE || ai_selection_mode == AI_MIN_CMC) && amount <= av_value + 1 ){
						ai_pick[apc] = count;
						apc++;
					}
				}
				count++;
		}

		if( selected == -1 && apc > 0 ){
			selected = ai_pick[internal_rand(apc)];
		}
	}
	return selected;
}

int new_select_a_card(int player, int targ_player, int search_location, int must_select, int ai_selection_mode, int test_type, test_definition_t *this_test){

	if( search_location < 1 ){
	   return -1;
	}
	int zone_count = 0;

	const int *s_location = deck_ptr[targ_player];

	if( this_test->create_minideck > 0 ){
		zone_count = this_test->create_minideck;
	}

	if( search_location == TUTOR_FROM_DECK && this_test->create_minideck > 0 && check_battlefield_for_id(1-player, CARD_ID_AVEN_MINDCENSOR) ){
		zone_count = 4;
		if( zone_count > count_deck(targ_player) ){
			zone_count = count_deck(targ_player);
		}
	}

	if( search_location == TUTOR_FROM_GRAVE && graveyard_has_shroud(targ_player) ){
	   return -1;
	}

	if( search_location == TUTOR_FROM_GRAVE || search_location == TUTOR_FROM_GRAVE_NOTARGET ){
		s_location = get_grave(targ_player);
	}

	if( search_location == TUTOR_FROM_RFG ){
	   s_location = rfg_ptr[targ_player];
	}

	int hand_index[150];
	if( search_location == TUTOR_FROM_HAND ){
		int i;
		for(i=0; i<150; i++){
			hand_index[i] = -1;
		}
		int hand_c = 0;
		int count = 0;
		while( count < active_cards_count[targ_player] ){
				if( in_hand(targ_player, count) && ! check_state(targ_player, count, STATE_CANNOT_TARGET) ){
					card_instance_t *this = get_card_instance(targ_player, count);
					hand_index[hand_c] = this->internal_card_id;
					hand_c++;
					zone_count++;
				}
				count++;
		}
		s_location = hand_index;
	}

	if( s_location[0] == -1 ){
		return -1;
	}

	if( zone_count < 1 ){
		while( s_location[zone_count] != -1){
				zone_count++;
		}
	}

	int selected = select_card_from_zone(player, targ_player, s_location, zone_count, must_select, ai_selection_mode, -1, this_test);

	if( selected != -1 && search_location == TUTOR_FROM_HAND ){
		int my_id = cards_data[s_location[selected]].id;
		int count = 0;
		while( count < active_cards_count[targ_player] ){
				if( in_hand(targ_player, count) && get_id(targ_player, count) == my_id){
					selected = count;
					break;
				}
				count++;
		}
	}

	return selected;
}

int select_a_card(int player, int targ_player, int search_location, int must_select, int ai_selection_mode, int test_type, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t this_test;
	default_test_definition(&this_test, type);
	this_test.type = type;
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	return new_select_a_card(player, targ_player, search_location, must_select, ai_selection_mode, test_type, &this_test);
}

int get_tutoring_denial(int player){
		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while(count < active_cards_count[i] ){
					if( in_play(i, count) ){
						if( is_what(i, count, TYPE_PERMANENT) ){
							if( get_id(i, count) == CARD_ID_MINDLOCK_ORB ){
								return 1;
							}
							if( get_id(i, count) == CARD_ID_STRANGLEHOLD && i != player ){
								return 1;
							}
							if( get_id(i, count) == CARD_ID_LEONIN_ARBITER ){
								card_instance_t *instance = get_card_instance( i, count);
								if( instance->targets[i+2].player != 66 ){
									if( has_mana(player, COLOR_COLORLESS, 2) ){
										int choice = do_dialog(player, i, count, -1, -1, " Deactivate Leonin Arbiter until EOT\n Pass", 0);
										if( choice == 0 ){
											charge_mana(player, COLOR_COLORLESS, 2);
											if( spell_fizzled != 1 ){
												instance->targets[i+2].player = 66;
											}
											else{
												return 1;
											}
										}
										else{
											return 1;
										}
									}
									else{
										return 1;
									}
								}
							}
						}
						if( is_what(i, count, TYPE_EFFECT) ){
							card_instance_t *instance = get_card_instance( i, count);
							if( instance->targets[2].card == CARD_ID_SHADOW_OF_DOUBT ){
								return 1;
							}
						}
					}
					count++;
			}
		}
	return 0;
}

int new_global_tutor(int player, int targ_player, int search_location, int destination, int must_select, int ai_selection_mode, test_definition_t *this_test){

	if( search_location == TUTOR_FROM_DECK && this_test->create_minideck < 1 ){
		if( get_tutoring_denial(player) ){
			return -1;
		}
	}

	int avoid_shuffle = this_test->no_shuffle;

	// This is inane, but can't be removed because of the ridiculous number of cards that do the shuffling *themselves* afterwards
	if( this_test->qty > 1 && !avoid_shuffle ){
		avoid_shuffle = 1;
	}

	int test_type = new_get_test_score(this_test);

	int q;
	int tutored[this_test->qty];
	int tc = 0;
	int grafdiggers_cage_flag = ((destination == TUTOR_PLAY || destination == TUTOR_PLAY_TAPPED || destination == TUTOR_PLAY_ATTACKING)
								 && check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE));
	int nsl = search_location;
	if (nsl == TUTOR_FROM_GRAVE_NOTARGET){
		nsl = TUTOR_FROM_GRAVE;
	}
	for(q=0; q<this_test->qty; q++){
		int selected = new_select_a_card(player, targ_player, search_location, must_select, ai_selection_mode, test_type, this_test);
		if( selected != -1 ){
			const int *s_location = deck_ptr[targ_player];

			if( nsl == TUTOR_FROM_GRAVE ){
				s_location = get_grave(targ_player);
			}

			if( nsl == TUTOR_FROM_RFG ){
				s_location = rfg_ptr[targ_player];
			}

			if( nsl == TUTOR_FROM_HAND ){
				if( destination == TUTOR_GET_ID ){
					tutored[tc] = get_id(targ_player, selected);
					tc++;
				}
				else{
					tutored[tc] = selected;
					tc++;
				}
			}

			if( nsl == TUTOR_FROM_DECK ){
				if( player == targ_player ){
					set_trap_condition(targ_player, TRAP_DECK_WAS_SEARCHED, 1);
				}
				if( grafdiggers_cage_flag && is_what(-1, s_location[selected], TYPE_CREATURE) ){
					if( destination == TUTOR_PLAY || destination == TUTOR_PLAY_TAPPED || destination == TUTOR_PLAY_ATTACKING ){
						tutored[tc] = -1;
					}
				}
				else{
					tutored[tc] = s_location[selected];
					remove_card_from_deck(targ_player, selected);
					tc++;
				}
			}

			if( nsl == TUTOR_FROM_GRAVE ){
				if( grafdiggers_cage_flag && is_what(-1, s_location[selected], TYPE_CREATURE) ){
					if( destination == TUTOR_PLAY || destination == TUTOR_PLAY_TAPPED || destination == TUTOR_PLAY_ATTACKING ){
						tutored[tc] = -1;
					}
				}
				else{
					tutored[tc] = s_location[selected];
					remove_card_from_grave(targ_player, selected);
					tc++;
				}
			}

			if( nsl == TUTOR_FROM_RFG ){
				tutored[tc] = s_location[selected];
				remove_card_from_rfg(targ_player, cards_data[s_location[selected]].id);
				tc++;
			}
			
			if( this_test->create_minideck > 1 ){
				this_test->create_minideck--;
				if( this_test->create_minideck == 0 ){
					break;
				}
			}
		}
		else{
			break;
		}
	}
	if( search_location == TUTOR_FROM_DECK && ! avoid_shuffle ){
		shuffle(targ_player);
	}
	int card_added = -1;
	int really_tutored = 0;
	int storage_loc = 0;
	for(q=0; q<tc; q++){
		if( tutored[q] != -1 ){
			really_tutored++;
			switch( destination ){
					case TUTOR_RFG:
					{
						if( nsl == TUTOR_FROM_HAND ){
							card_added = get_id(targ_player, tutored[q]);
							rfg_card_in_hand(targ_player, tutored[q]);
						}
						else{
							card_added = cards_data[tutored[q]].id;
							add_card_to_rfg(targ_player, tutored[q]);
						}
						play_sound_effect(WAV_DESTROY);
					}
					break;

					case TUTOR_BOTTOM_OF_DECK:
					case TUTOR_BOTTOM_OF_DECK_RANDOM:
					{
						if( nsl == TUTOR_FROM_HAND ){
							int *t_deck = deck_ptr[targ_player];
							t_deck[count_deck(targ_player)] = get_original_internal_card_id(targ_player, tutored[q]);
							obliterate_card(targ_player, tutored[q]);
						}
						else{
							card_added = cards_data[tutored[q]].id;
							int *t_deck = deck_ptr[targ_player];
							t_deck[count_deck(targ_player)] = tutored[q];
						}
					}
					break;

					case TUTOR_GRAVE:
					{
						// It's currently in targ_player's hand as card_added
						card_added = nsl == TUTOR_FROM_HAND ? tutored[q] : add_card_to_hand(targ_player, tutored[q]);
						if (nsl == TUTOR_FROM_DECK){
							put_on_top_of_deck(targ_player, card_added);
							mill(targ_player, 1);	// so cards specifically triggering on library-to-graveyard, or replacing that, work correctly
						}
						else if (nsl == TUTOR_FROM_HAND){
								new_discard_card(targ_player, card_added, player, 0);	// So cards specifically triggering on discard work correctly
						}
						else{
							kill_card(targ_player, card_added, KILL_DESTROY);
						}
					}
					break;

					case TUTOR_HAND:
					case TUTOR_TPLAYER_HAND:
					case TUTOR_HAND_AND_MARK:
					{
						int dest_player = destination == TUTOR_TPLAYER_HAND ? targ_player : player;
						card_added = add_card_to_hand(dest_player, tutored[q]);
						// avoid_shuffle==2 suppresses the automatic reveal, for reveal_top_cards_of_library_and_choose().  It's not part of the public interface.
						if( player == (destination == TUTOR_HAND ? AI : HUMAN) && avoid_shuffle != 2
							&& (nsl == TUTOR_FROM_GRAVE
								|| nsl == TUTOR_FROM_RFG
								|| !(test_type == 0
									 || (test_type == 1<<0 && this_test->type == TYPE_ANY)))
						  ){
							reveal_card(player, card_added, -1, -1);
						}
						if( destination == TUTOR_HAND_AND_MARK ){
							add_state(dest_player, card_added, STATE_TARGETTED);
						}
						if( nsl == TUTOR_FROM_GRAVE ){
							from_grave_to_hand_triggers(dest_player, card_added);
						}
					}
					break;

					case TUTOR_PLAY:
					case TUTOR_PLAY_TAPPED:
					case TUTOR_PLAY_ATTACKING:
					{
						card_added = nsl == TUTOR_FROM_HAND ? tutored[q] : add_card_to_hand(player, tutored[q]);
						card_instance_t *instance = get_card_instance(player, card_added);
						if( player != targ_player ){
							if( player == HUMAN ){
								add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
							}
							else{
								remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
							}
						}
						if( destination == TUTOR_PLAY_TAPPED || destination == TUTOR_PLAY_ATTACKING ){
							instance->state |= STATE_TAPPED;
						}
						if( nsl == TUTOR_FROM_GRAVE ){
							set_special_flags3(player, card_added, SF3_REANIMATED);
						}
						put_into_play(player, card_added);
						if( destination == TUTOR_PLAY_ATTACKING ){
							instance->state |= STATE_ATTACKING;
							choose_who_attack(player, card_added);
						}
					}
					break;

					case TUTOR_DECK:
					{
						card_added = nsl == TUTOR_FROM_HAND ? tutored[q] : add_card_to_hand(targ_player, tutored[q]);
						if( test_type != 0 && this_test->type > 0 && this_test->type != TYPE_ANY && player == AI ){
							reveal_card(player, card_added, -1, -1);
						}
						put_on_top_of_deck(targ_player, card_added);
					}
					break;

					default:
					break;
			}
			if( this_test->storage > -1 ){
				get_card_instance(player, this_test->storage)->targets[storage_loc].card = tutored[q];
				storage_loc++;
			}
		}
	}

	if( nsl == TUTOR_FROM_DECK ){
		if( player == targ_player ){
			int amount = count_cards_by_id(1-player, CARD_ID_OB_NIXILIS_UNSHACKLED);
			if( amount ){
				lose_life(player, amount*10);
				int result = locate_id(1-player, CARD_ID_OB_NIXILIS_UNSHACKLED);
				impose_sacrifice(1-player, result, player, amount, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	return this_test->qty > 1 ? really_tutored : card_added;
}

int global_tutor(int player, int targ_player, int search_location, int destination, int must_select, int ai_selection_mode, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t this_test;
	default_test_definition(&this_test, type);
	this_test.type = type;
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	return new_global_tutor(player, targ_player, search_location, destination, must_select, ai_selection_mode, &this_test);
}

int tutor_from_grave_to_hand_except_for_dying_card(int player, int card, int must_select, int ai_selection_mode, test_definition_t* test)
{
  /* Find card in graveyard and make it unselectable without disturbing its placement.
   * If not there - maybe it was stolen or a token or has already been removed by something else - this won't do anything.
   *
   * It's conceivable that there's some sort of effect somewhere that can trigger during the new_global_tutor() call to shuffle graveyard into library or do
   * some other shenanigans that will move the iid_draw_a_card elsewhere ("Whenever a player has more than 4 cards in his hand, he shuffles his graveyard into
   * his library"?  "While Yixlid Jailer Variant is in a graveyard, all other cards in all graveyards lose all abilities" + an Eldrazi?), but I doubt it.
   *
   * It would be much more of a concern if the chosen card was put on the battlefield or played for free; either of those could very easily cause a reshuffling.
   * (Consider reanimating Barishi with a Pandemonium in play, or Timetwister.)  If such a case were needed, it would need support within new_global_tutor to
   * change the iid_draw_a_card's back into their original iids before removing the selected card. */

  if (graveyard_has_shroud(player))
	return 0;

  int owner, position, iid = -1;
  if (find_in_owners_graveyard(player, card, &owner, &position)
	  && owner == player)	// If it's in the other player's graveyard, then don't even bother - we aren't searching that one
	iid = turn_card_in_grave_face_down(player, position);

  int rval = -1;
  if (new_special_count_grave(player, test))
	rval = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, must_select, ai_selection_mode, test);

  // Restore the unselectable card, if indeed there was one.  Have to search, since its position may have changed.
  if (iid != -1)
	for (position = 0; get_grave(player)[position] != -1; ++position)
	  if (get_grave(player)[position] == iid_draw_a_card)
		{
		  turn_card_in_grave_face_up(player, position, iid);
		  break;
		}

  return rval;
}

// [player] reveals the top [num_cards] of his library.
int reveal_top_cards_of_library(int player, int num_cards)
{
  int count = count_deck(player);
  num_cards = MIN(count, num_cards);
  if (num_cards <= 0)
	return 0;

  if (ai_is_speculating != 1 && (player == AI || (trace_mode & 2)))
	show_deck(1-player, deck_ptr[player], num_cards, "Opponent reveals", 0, 0x78F0A8);	// Localized "OK"

  return num_cards;
}

/* [reveal_rest or Look at] the top [num_cards] of your library.  Choose a [test] card from among them, [reveal_chosen], and move it to [destination_chosen].
 * Move the rest to [destination_rest]. */
int reveal_top_cards_of_library_and_choose(int src_player, int src_card, int t_player, int num_cards, int must_select, tutor_t destination_chosen, int reveal_chosen, tutor_t destination_rest, int reveal_rest, test_definition_t* test)
{
  if (reveal_rest)
	num_cards = reveal_top_cards_of_library(t_player, num_cards);
  else
	{
	  int count = count_deck(t_player);
	  num_cards = MIN(count, num_cards);
	}
  if (num_cards <= 0)
	return -1;

  int num_to_pick = MIN(num_cards, test->qty);
  if (num_to_pick <= 0)
	return -1;

  test->qty = 1;
  test->create_minideck = num_cards;

  int i, selected = -1;
  int chosen_iids[num_to_pick], num_chosen = 0;	// only used for destinations that have to be dealt with all at once, after moving the unchosen cards

  int grafdiggers_cage = ((destination_chosen == TUTOR_PLAY || destination_chosen == TUTOR_PLAY_TAPPED || destination_chosen == TUTOR_PLAY_ATTACKING)
						  && check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE));

  for (i = 0; i < num_to_pick; ++i)
	{
	  selected = select_card_from_zone(src_player, t_player, deck_ptr[t_player], num_cards, must_select, test->ai_selection_mode, -1, test);
	  if (selected == -1)
		break;

	  --num_cards;

	  int iid = deck_ptr[t_player][selected];

	  if (reveal_chosen)
		reveal_card_iid(src_player, src_card, iid);

	  switch (destination_chosen)
		{
		  case TUTOR_HAND:
		  case TUTOR_HAND_AND_MARK:
			remove_card_from_deck(t_player, selected);
			int ca = add_card_to_hand(src_player, iid);
			if( destination_chosen == TUTOR_HAND_AND_MARK && ca > -1 ){
				add_state(t_player, ca, STATE_TARGETTED);
			}
			break;

		  case TUTOR_GRAVE:
			remove_card_from_deck(t_player, selected);
			chosen_iids[num_chosen++] = iid;
			break;

		  case TUTOR_PLAY:
		  case TUTOR_PLAY_TAPPED:
		  case TUTOR_PLAY_ATTACKING:
			if (!(grafdiggers_cage && is_what(-1, deck_ptr[t_player][selected], TYPE_CREATURE)))
			  {
				remove_card_from_deck(t_player, selected);
				chosen_iids[num_chosen++] = iid;
				break;
			  }
			// else fall through to TUTOR_DECK
		  case TUTOR_DECK:	// Have to make it n'th from the top, where n is num_cards before the above decrement.
			;int n;
			for (n = selected; n < num_cards; ++n)
			  SWAP(deck_ptr[t_player][n], deck_ptr[t_player][n + 1]);
			break;

		  case TUTOR_RFG:
			rfg_card_in_deck(t_player, selected);
			break;

		  case TUTOR_GET_ID:
			selected = cards_data[iid].id;
			// And fall through to TUTOR_GET_POSITION
		  case TUTOR_GET_POSITION:
			if (num_to_pick > 1)
			  {
				char buf[200];
				sprintf(buf, "%s:%d\nillegal destination_chosen: %d with qty=%d", __FILE__, __LINE__, destination_chosen, num_to_pick);
				backtrace_and_abort(buf);
				break;
			  }
			break;

		  case TUTOR_BOTTOM_OF_DECK:
			put_card_in_deck_to_bottom(t_player, selected);
			break;

		  case TUTOR_TPLAYER_HAND:
			remove_card_from_deck(t_player, selected);
			add_card_to_hand(t_player, iid);
			break;

		  case TUTOR_BOTTOM_OF_DECK_RANDOM:
			{
				char buf[200];
				sprintf(buf, "%s:%d\nillegal destination_chosen: %d", __FILE__, __LINE__, destination_chosen);
				backtrace_and_abort(buf);
			}
			break;
		}
	}

  if (num_cards > 0)
	switch (destination_rest)
	  {
		case TUTOR_HAND_AND_MARK:
		case TUTOR_HAND:
			for (i = 0; i < num_cards; ++i){
				int ca = add_card_to_hand(t_player, deck_ptr[t_player][i]);
				if( ca > -1 && destination_rest == TUTOR_HAND_AND_MARK ){
					add_state(t_player, ca, STATE_TARGETTED);
				}
			}
			obliterate_top_n_cards_of_deck(t_player, num_cards);
		break;

		case TUTOR_DECK:			rearrange_top_x(t_player, t_player, num_cards);	break;
		case TUTOR_GRAVE:			mill(t_player, num_cards);	break;
		case TUTOR_RFG:			rfg_top_n_cards_of_deck(t_player, num_cards);	break;
		case TUTOR_BOTTOM_OF_DECK:put_top_x_on_bottom(t_player, t_player, num_cards);	break;

		case TUTOR_TPLAYER_HAND:
		  for (i = 0; i < num_cards; ++i)
			add_card_to_hand(1-t_player, deck_ptr[t_player][i]);
		  obliterate_top_n_cards_of_deck(t_player, num_cards);
		  break;

		case TUTOR_BOTTOM_OF_DECK_RANDOM:	put_top_x_on_bottom_in_random_order(t_player, num_cards);	break;

		case TUTOR_PLAY:
		case TUTOR_PLAY_TAPPED:
		case TUTOR_GET_ID:
		case TUTOR_PLAY_ATTACKING:
		case TUTOR_GET_POSITION:;
		  char buf[200];
		  sprintf(buf, "%s:%d\nillegal destination_rest: %d", __FILE__, __LINE__, destination_rest);
		  backtrace_and_abort(buf);
		  break;
	  }

  if (num_chosen > 0)
	switch (destination_chosen)
	  {
		case TUTOR_GRAVE:
		  for (i = num_chosen - 1; i >= 0; --i)
			raw_put_iid_on_top_of_deck(t_player, chosen_iids[i]);
		  mill(t_player, num_chosen);
		  break;

		case TUTOR_PLAY:
		case TUTOR_PLAY_TAPPED:
		case TUTOR_PLAY_ATTACKING:
		  for (i = 0; i < num_chosen; ++i)
			{
			  int c = add_card_to_hand(src_player, chosen_iids[i]);
			  card_instance_t* inst = get_card_instance(src_player, c);
			  if (t_player != src_player)
				inst->state ^= STATE_OWNED_BY_OPPONENT;

			  if (destination_chosen == TUTOR_PLAY_ATTACKING)
				{
				  inst->state |= STATE_TAPPED|STATE_ATTACKING;
				  put_into_play(src_player, c);
				  choose_who_attack(src_player, c);
				}
			  else
				{
				  if (destination_chosen == TUTOR_PLAY_TAPPED)
					inst->state |= STATE_TAPPED;
				  put_into_play(src_player, c);
				}
			}
		  break;

		default:
		  // Should be impossible
		  ;char buf[200];
		  sprintf(buf, "%s:%d\nillegal destination_chosen: %d with num_chosen=%d", __FILE__, __LINE__, destination_chosen, num_chosen);
		  backtrace_and_abort(buf);
		  break;
	  }

  return selected;	// position of last card chosen; or its csvid if destination_chosen was TUTOR_GET_ID
}

// Identical to reveal_top_cards_of_library_and_choose(), but specify a type bitfield instead of a full test_definition_t.  Constructs an appropriate prompt.
int reveal_top_cards_of_library_and_choose_type(int src_player, int src_card, int t_player, int num_cards, int must_select, tutor_t destination_chosen, int reveal_chosen, tutor_t destination_rest, int reveal_rest, type_t legal_types)
{
  test_definition_t test;
  if (legal_types == 0 || legal_types == TYPE_ANY || legal_types == TYPE_NONEFFECT)
	default_test_definition(&test, TYPE_ANY);
  else
	{
	  new_default_test_definition(&test, legal_types, "Select a ");

	  if (ai_is_speculating != 1)
		{
		  int l = strlen(test.message);
		  type_text(test.message + l, 100 - l, legal_types, 0);
		  l = strlen(test.message);	// Go, Schlemiel, go!
		  scnprintf(test.message + l, 100 - l, " card.");
		}
	}

  return reveal_top_cards_of_library_and_choose(src_player, src_card, t_player, num_cards, must_select, destination_chosen, reveal_chosen, destination_rest, reveal_rest, &test);
}

int select_one_and_mill_the_rest(int player, int t_player, int number, int type){

	int card_added = -1;
	int *deck = deck_ptr[t_player];
	if( number > count_deck(t_player) ){
		 number = count_deck(t_player);
	}
	if( number > 0 ){
		int selected = -1;
		while( selected == -1 ){
				selected = show_deck( player, deck, number, "Pick a card", 0, 0x7375B0 );
		}
		if( is_what(-1, deck[selected], type) ){
			card_added = add_card_to_hand(t_player, deck[selected]);
			remove_card_from_deck(t_player, selected);
			number--;
		}
		mill(player, number);
	}

	return card_added;
}

int select_one_and_put_the_rest_on_bottom(int player, int t_player, int number){

	int selected = -1;
	int *deck = deck_ptr[t_player];
	while( selected == -1 ){
			selected = show_deck( player, deck, number, "Pick a card", 0, 0x7375B0 );
	}
	int card_added = add_card_to_hand(t_player, deck[selected]);
	remove_card_from_deck(t_player, selected);
	put_top_x_on_bottom(player, t_player, number-1);

	return card_added;
}

int reveal_x_and_choose_a_card_type( int player, int card, int to_reveal, int selected_type ){

	if( count_deck(player) < to_reveal ){
		to_reveal = count_deck(player);
	}

	test_definition_t this_test;
	char buffer[100];
	int pos = scnprintf(buffer, 100, " Select a");
	if( selected_type == TYPE_CREATURE ){
		pos += scnprintf(buffer + pos, 100-pos, " creature");
	}
	if( selected_type == TYPE_LAND ){
		pos += scnprintf(buffer + pos, 100-pos, " land");
	}
	if( selected_type == TYPE_ARTIFACT ){
		pos += scnprintf(buffer + pos, 100-pos, "n artifact");
	}
	if( selected_type == TYPE_ENCHANTMENT ){
		pos += scnprintf(buffer + pos, 100-pos, " enchantment");
	}
	pos += scnprintf(buffer + pos, 100-pos, " card.");
	new_default_test_definition(&this_test, selected_type, buffer);
	this_test.create_minideck = to_reveal;
	this_test.no_shuffle = 1;

	int card_added = -1;

	if( to_reveal > 0 ){
		card_added = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		if( card_added != -1 ){
			to_reveal--;
		}
		if( to_reveal > 0 ){
			put_top_x_on_bottom(player, player, to_reveal);
		}
	}

	return card_added;
}

void tutor_lands(int player, tutor_t destination, int num){
	test_definition_t test;
	new_default_test_definition(&test, TYPE_LAND, "Choose a land card.");
	test.qty = num;
	new_global_tutor(player, player, TUTOR_FROM_DECK, destination, 0, AI_MAX_VALUE, &test);
}

void tutor_basic_land(int player, int put_in_play, int tap_it){

	int mode = TUTOR_HAND;
	if( put_in_play == 1 ){
		mode = TUTOR_PLAY;
	}
	if( tap_it == 1 ){
		mode = TUTOR_PLAY_TAPPED;
	}
	tutor_basic_lands(player, mode, 1);
}

void tutor_basic_lands(int player, tutor_t destination, int num){
	test_definition_t test;
	new_default_test_definition(&test, TYPE_LAND, "Choose a basic land card.");
	test.subtype = SUBTYPE_BASIC;
	test.subtype_flag = MATCH;
	test.qty = num;
	test.no_shuffle = 1;
	new_global_tutor(player, player, TUTOR_FROM_DECK, destination, 0, AI_MAX_VALUE, &test);
	shuffle(player);
}

// Tutor two basic lands, one onto bf tapped, one into hand, then shuffle
void cultivate(int player){
	test_definition_t test;
	new_default_test_definition(&test, TYPE_LAND, "Choose a basic land card to put on the battlefield tapped.");
	test.subtype = SUBTYPE_BASIC;
	test.subtype_flag = MATCH;
	test.no_shuffle = 1;

	if (new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &test) != -1){
		strcpy(test.message, "Choose a basic land card to put in your hand.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

	shuffle(player);
}

// Simple n^2 insertion sort on array[].player, descending.  Poorly suited if number is greater than about 30.
void descending_insertion_sort_on_player(int number, target_t* array){
	target_t value;
	int i, j;
	for (i = 1; i < number; ++i){
		value = array[i];	// struct copy
		for (j = i; j > 0 && value.player > array[j - 1].player; --j){
			array[j] = array[j - 1];	// struct copy
		}
		array[j] = value;	// struct copy
	}
}

int select_multiple_cards_from_graveyard(int player, int targ_player, int must_select_all /*if < 0, not enforced; but failing to select all is considered cancelling */, int ai_selection_mode, test_definition_t* this_test /*optional*/, int max_targets, target_t* ret_location){
	// no reason this won't work on higher except that 1) the sort will take forever, and 2) it won't be storable in a card's target array.
	ASSERT(max_targets > 0 && max_targets <= 19);

	test_definition_t default_test;
	if (this_test){
		if (this_test->id){
			ASSERT(this_test->id == 904/*draw a card*/ && this_test->id_flag == DOESNT_MATCH);
		} else {
			this_test->id = 904;/*draw a card*/
			this_test->id_flag = DOESNT_MATCH;
		}
	} else {
		new_default_test_definition(&default_test, 0, "");
		if (ai_is_speculating != 1){
			if (max_targets == 1){
				strcpy(default_test.message, "Select a card.");
			} else if (must_select_all) {
				sprintf(default_test.message, "Select %d cards.", max_targets);
			} else {
				sprintf(default_test.message, "Select up to %d cards.", max_targets);
			}
		}
		default_test.id = 904;/*draw a card*/
		default_test.id_flag = DOESNT_MATCH;
		this_test = &default_test;
	}

	int i;
	// Clear targets first, so we don't have to worry about it later if too few cards are chosen
	for (i = 0; i < max_targets; ++i){
		ret_location[i].player = ret_location[i].card = -1;
	}

	if (graveyard_has_shroud(targ_player)){
		if (must_select_all) {
			spell_fizzled = 1;
		}
		return 0;
	}

	int count = count_graveyard(targ_player);
	if (count == 0){
		if (must_select_all){
			spell_fizzled = 1;
		}
		return 0;
	} else if (count < max_targets){
		if (must_select_all){
			spell_fizzled = 1;
			return 0;
		} else {
			max_targets = count;
		}
	}

	// Select cards, changing each chosen card to a face-down creature so it can't be chosen again and is visually distinct
	int num_chosen = max_targets;
	for (i = 0; i < max_targets; ++i){
		int selected = new_select_a_card(player, targ_player, TUTOR_FROM_GRAVE, must_select_all > 0 ? 1 : 0, ai_selection_mode, -1, this_test);

		if (selected == -1){
			num_chosen = i;
			break;
		} else {
			ret_location[i].player = selected;
			ret_location[i].card = turn_card_in_grave_face_down(targ_player, selected);
		}
	}

	// Change the face-down cards back to what they were
	for (i = 0; i < num_chosen; ++i){
		turn_card_in_grave_face_up(targ_player, ret_location[i].player, ret_location[i].card);
	}

	// Sort so earliest targets have the highest indices.  Simple n^2 insertion sort, since n is always 19 or below.
	descending_insertion_sort_on_player(num_chosen, ret_location);

	if (must_select_all && num_chosen < max_targets){
		spell_fizzled = 1;
	}

	return num_chosen;
}

// test only needs to be defined during EVENT_CAST_SPELL.
int spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(int player, int card, event_t event, int max_cards, test_definition_t* test, int dont_kill_self)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int num_chosen = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, test, max_cards, &instance->targets[0]);
	  if (num_chosen < max_cards)
		ai_modifier += (player == AI ? -24 : 24) * (max_cards - num_chosen);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int i, num_validated = 0, num_targeted = 0, selected;
	  for (i = 0; i < max_cards; ++i)
		if (instance->targets[i].card != -1)
		  {
			++num_targeted;
			if ((selected = validate_target_from_grave(player, card, player, i)) != -1)
			  {
				from_grave_to_hand(player, selected, TUTOR_HAND);
				++num_validated;
			  }
		  }

	  if (num_validated == 0 && num_targeted > 0)
		spell_fizzled = 1;

	  if (!dont_kill_self)
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Choose one or both - Return target [type1] card from your graveyard to your hand; and/or return target [type2] card from your graveyard to your hand.
 * Not really appropriate if type1 == type2, for which you want spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(). */
int spell_return_one_or_two_cards_from_gy_to_hand(int player, int card, event_t event, type_t type1, type_t type2)
{
  if (event == EVENT_CAN_CAST)
	{
	  if (((type1 | type2) & (TYPE_PERMANENT | TARGET_TYPE_PLANESWALKER)) == TYPE_PERMANENT)
		{
		  // have to check separately, or else a planeswalker in the graveyard will be a false positive
		  if (!any_in_graveyard_by_type(player, type1) && !any_in_graveyard_by_type(player, type2))
			return 0;
		}
	  else if (!any_in_graveyard_by_type(player, type1 | type2))	// Otherwise, can just iterate through graveyard once
		return 0;

	  return !graveyard_has_shroud(player);
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  instance->targets[0].player = instance->targets[0].card = -1;
	  instance->targets[1].player = instance->targets[1].card = -1;

	  int iid = -1, i;
	  type_t types[2] = { type1, type2 };
	  test_definition_t test;
	  for (i = 0; i <= 1; ++i)
		if (any_in_graveyard_by_type(player, types[i]))
		  {
			new_default_test_definition(&test, types[i], "");
			char type_txt[100];
			type_text(type_txt, 100, types[i], 0);
			scnprintf(test.message, 100, "Select target %s card.", type_txt);

			if (select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test, i) == -1)
			  cancel = 0;	// Might still choose the other type; or might have done so already.
			else if (i == 0)
			  iid = turn_card_in_grave_face_down(player, instance->targets[i].player);
		  }

	  if (iid != -1)
		turn_card_in_grave_face_up(player, instance->targets[0].player, iid);

	  if (instance->targets[0].player == -1 && instance->targets[1].player == -1)	// chose neither
		cancel = 1;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int i, num_validated = 0;;
	  for (i = 0; i <= 1; ++i)
		if (validate_target_from_grave_source(player, card, player, i) >= 0)
		  {
			from_grave_to_hand(player, instance->targets[i].player, TUTOR_HAND);
			++num_validated;
		  }

	  if (num_validated == 0)
		spell_fizzled = 1;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

// Separates the first number cards in source_deck[] into two piles.  AI attempts to divide evenly by base_value.
// Preconditions: piles is an array of int[2][number].  Elements of source_deck[] are internal_card_ids (such as from deck_ptr[], get_grave(), or rfg_ptr[]).
// Postcondition: For each i in [0..number), one of piles[0][i] and piles[1][i] will be set to source_deck[i], and the other iid_draw_a_card.
void separate_into_two_piles(int who_separates, const int* source_deck, int number, void* piles_)
{
  int (*piles)[number] = piles_;
  int i;
  for (i = 0; i < number; ++i)
	{
	  piles[0][i] = source_deck[i];
	  piles[1][i] = iid_draw_a_card;
	}

  if (number <= 1)
	return;

  if (who_separates == AI || ai_is_speculating == 1)
	{
	  /* Evenly separating a pile of things with various scores into two piles of roughly equal total scores is a variant of the Knapsack Problem, and is NP
	   * complete.  On the other hand, we could solve it exactly, since we know number is always very small (5 or under).  But, it's not worth even that much
	   * effort.  What we're going to do instead is sort on score, then for each card, put it into the pile with the lowest total score so far. */
	  int scores[2] = {0,0};

	  target_t vals[number];	// vals[].player = score, vals[].card = index into source deck
	  for (i = 0; i < number; ++i)
		{
		  vals[i].player = my_base_value_by_id(source_deck[i]);
		  vals[i].player = MAX(1, vals[i].player);
		  vals[i].card = i;
		}

	  descending_insertion_sort_on_player(number, vals);

	  for (i = 0; i < number; ++i)
		if (scores[0] <= scores[1])
		  // Leave in pile 0
		  scores[0] += vals[i].player;
		else
		  {
			// Move to pile 1
			scores[1] += vals[i].player;
			SWAP(piles[0][vals[i].card], piles[1][vals[i].card]);
		  }
	}
  else
	while (1)
	  {
		int selected = show_deck(who_separates, piles[0], number, "Choose pile 1.", 0, 0x7375B0);
		if (selected == -1)
		  break;

		SWAP(piles[0][selected], piles[1][selected]);
	  }
}

// Chooses between two piles of cards each with number cards.
// Preconditions: piles is an array of int[2][number].  Elements of piles[][] are internal_card_ids.  ai_selection_mode is AI_MAX_VALUE or AI_MIN_VALUE.  choice_txt is 11 chars or less.
// Returns chosen pile, 0 or 1.
int choose_between_two_piles(int chooser, int number, void* piles_, int ai_selection_mode, const char* choice_txt)
{
  ASSERT(ai_selection_mode == AI_MIN_VALUE || ai_selection_mode == AI_MAX_VALUE);

  int (*piles)[number] = piles_;

  if (chooser == AI || ai_is_speculating == 1)
	{
	  int scores[2] = {0, 0};

	  int i, j;
	  for (i = 0; i < 2; ++i)
		for (j = 0; j < number; ++j)
		  if (piles[i][j] != -1
			  && piles[i][j] != iid_draw_a_card)
			{
			  int val = my_base_value_by_id(piles[i][j]);
			  val = MAX(1, val);
			  scores[i] += val;
			}

	  int chosen;
	  if (scores[0] > scores[1])
		chosen = ai_selection_mode == AI_MAX_VALUE ? 0 : 1;
	  else
		chosen = ai_selection_mode == AI_MAX_VALUE ? 1 : 0;

	  show_deck(HUMAN, piles[chosen], number, "Opponent chose this pile.", 0, (int)"OK");

	  return chosen;
	}
  else
	{
	  int selected, chosen = 1;
	  do
		{
		  chosen = 1 - chosen;
		  selected = show_deck(chooser, piles[chosen], number, "Click a card to see the other pile.", 0, (int)choice_txt);
		} while (selected != -1);

	  return chosen;
	}
}

// 1-who_chooses separates the top number cards of player's deck into two piles.  who_chooses picks a pile to put in player's hand.
// The rest are put in put_rest_where, which must currently be TUTOR_GRAVE or TUTOR_BOTTOM_OF_DECK.
void effect_fof(int player, int who_chooses, int number, int put_rest_where)
{
  int num_cards_in_deck = count_deck(player);
  number = MIN(number, num_cards_in_deck);

  ASSERT((put_rest_where == TUTOR_GRAVE || put_rest_where == TUTOR_BOTTOM_OF_DECK) && "others unimplemented");

  if (number > 0)
	{
	  int piles[2][number];

	  separate_into_two_piles(1 - who_chooses, deck_ptr[player], number, piles);

	  int chosen = choose_between_two_piles(who_chooses, number, piles, who_chooses == player ? AI_MAX_VALUE : AI_MIN_VALUE, "Put in hand");

	  // Obliterate the existing cards on top of the deck
	  obliterate_top_n_cards_of_deck(player, number);

	  // Put the ones from the chosen pile into hand one at a time
	  int i;
	  for (i = 0; i < number; ++i)
		if (piles[chosen][i] != iid_draw_a_card && piles[chosen][i] != -1)
		  add_card_to_hand(player, piles[chosen][i]);

	  // Put the ones from the unchosen pile back on top of the library
	  int num_unchosen = 0;
	  for (i = number - 1; i >= 0; --i)
		if (piles[1 - chosen][i] != iid_draw_a_card && piles[1 - chosen][i] != -1)
		  {
			raw_put_iid_on_top_of_deck(player, piles[1 - chosen][i]);
			++num_unchosen;
		  }

	  if (num_unchosen > 0)
		switch (put_rest_where)
		  {
			case TUTOR_GRAVE:
			  mill(player, num_unchosen);
			  break;

			case TUTOR_BOTTOM_OF_DECK:
			  for (i = 0; i < num_unchosen; ++i)
				put_top_card_of_deck_to_bottom(player);
			  if (num_unchosen > 1)
				rearrange_bottom_x(player, player, num_unchosen);
			  break;
		  }
	}
}

void alphabetize_deck(int player, int* deck, int num_cards){
	int count = count_deck(player);
	if( num_cards < count || deck != deck_ptr[player] ){ return; }
	//set_trap_condition(player, TRAP_DECK_WAS_SEARCHED, 1);
	if( count < 10 || ! get_setting(SETTING_ALPHABETIZE_DECK) ){ return; }
	if( player == AI ){ return; }

	//FILE *file = fopen("sort.txt", "w");
	//int *deck = deck_ptr[player];

	int i;
	int deck2[500];
	for(i=0;i<500;i++){
		deck2[i] = -1;
	}
	for(i=0;i<count;i++){
		int card = deck[i];
		card_ptr_t* c = cards_ptr[ cards_data[ deck[i] ].id ];
		//fprintf(file,"%d) Card=%s\n", i, c->name );

		int j;
		int index = i;
		for(j=0;j<i;j++){
			card_ptr_t* c1 = cards_ptr[ cards_data[ deck2[j] ].id ];
			if( strcmp( c1->name, c->name  ) > 0  ){
				index = j;
				//fprintf(file," --Fitting before %s\n", c1->name );
				break;
			}
			else{
				//fprintf(file," Skipping %s\n", c1->name );
			}
		}
		for(j=i+1;j>=index;j--){
			deck2[j] = deck2[j-1];
		}
		deck2[index] = card;

		// print out the deck
		//for(j=0;j<=i;j++){
		//    card_ptr_t* c1 = cards_ptr[ cards_data[ deck2[j] ].id ];
		//    fprintf(file,"   %d) %s\n", j, c1->name );
		//}

	}
	for(i=0;i<500;i++){
		deck[i] = deck2[i];
	}
	//fclose(file);
}

void scrylike_effect(int player, int t_player, int remaining){
	int *deck = deck_ptr[t_player];
	if( remaining > count_deck(t_player) ){
		remaining = count_deck(t_player);
	}
	if( remaining > 0 ){
		if (player == AI || ai_is_speculating == 1){
			if (player != t_player){
				for (; remaining > 0; --remaining){
					put_top_card_of_deck_to_bottom(t_player);
				}
			}
		} else {
			// put cards to bottom first
			while( remaining > 0 ){
					int selected = show_deck( player, deck, remaining, "Choose a card to put on the bottom", 0, 0x7375B0 );
					if( selected != -1 ){
						deck[ count_deck(t_player) ] = deck[selected];
						remove_card_from_deck( t_player, selected );
						remaining--;
					}
					else{
						break;
					}
			}
		}

		// put cards to the top now
		if( remaining > 1 ){
			rearrange_top_x(t_player, player, remaining);
		}
	}
}

void scry(int player, int number_of_cards){
	scrylike_effect(player, player, number_of_cards);
	dispatch_xtrigger2(player, XTRIGGER_SCRY, "Scrying", 0, player, number_of_cards);
}

void rearrange_top_x(int t_player, int who_chooses, int number){
	// t_player = target player
	// who_chooses = player who rearranges

	int *deck = deck_ptr[t_player];
	int count = count_deck(t_player);
	if( count < number ){
		number = count;
	}
	if( number == 1 ){
		show_deck( who_chooses, deck, 1, "Top card of library", 0, 0x7375B0 );
	}
	else{
		while( number > 1){
			int selected = -1;
			while( selected == -1 ){
				selected = show_deck( who_chooses, deck, number, "Pick a card to put on top", 0, 0x7375B0 );
			}

			int transit = deck[ number-1 ];
			deck[ number-1 ] = deck[selected];
			deck[selected] = transit;
			number--;
		}
	}
}

int show_bottom_of_deck(int player, int* deck, int num_cards, const char* prompt, int count_in_deck){
	// This is a moderate hack - instead of passing deck[] to show_deck, we find the exact position so that the bottom num_cards cards are shown
	return show_deck(player, &deck[count_in_deck - num_cards], num_cards, prompt, 0, 0x7375B0);
}

void rearrange_bottom_x(int t_player, int who_chooses, int number){
	// t_player = target player
	// who_chooses = player who rearranges

	int* deck = deck_ptr[t_player];
	int count = count_deck(t_player);
	if (count < number){
		number = count;
	}

	if (number == 1){
		show_bottom_of_deck(who_chooses, deck, 1, "Bottom card of library", count);
	} else {
		for (; number > 1; --number){
			int selected;
			do {
				selected = show_bottom_of_deck(who_chooses, deck, number, "Pick a card to put on bottom", count);
			} while (selected == -1);

			SWAP(deck[count - number], deck[count - number + selected]);
		}
	}
}

int select_best_card_from_top_x(int player, int t_player, int amount){

	int result = -1;
	int *deck = deck_ptr[t_player];
	int count = 0;
	int par = 0;
	if( player != t_player ){
		par = 1000;
	}

	while( count < amount ){
			card_ptr_t* c = cards_ptr[ cards_data[deck[count]].id ];
			if( player == t_player ){
				if( c->ai_base_value > par ){
					par = c->ai_base_value;
					result = count;
				}
			}
			else{
				if( c->ai_base_value < par ){
					par = c->ai_base_value;
					result = count;
				}
			}
			count++;
	}
	return result;
}

void put_top_x_on_bottom(int player, int t_player, int remaining ){

	int *deck = deck_ptr[t_player];

	while( remaining > 0 ){
			if( remaining == 1 ){
				deck[count_deck(t_player)] = deck[0];
				remove_card_from_deck(t_player, 0);
				remaining--;
			}
			else{
				char msg[100] = "Select a card to put on bottom.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.create_minideck = remaining;
				this_test.no_shuffle = 1;
				int mode = AI_MAX_VALUE;
				if( player != t_player ){
					mode = AI_MIN_VALUE;
				}
				int selected = new_select_a_card(player, t_player, TUTOR_FROM_DECK, 1, mode, -1, &this_test);
				if( selected != -1 ){
					deck[count_deck(t_player)] = deck[selected];
					remove_card_from_deck(t_player, selected);
					remaining--;
				}
			}
	}
}

void put_top_x_on_bottom_in_random_order(int player, int amount)
{
  if (amount <= 0)
	return;

  int* deck = deck_ptr[player];

  // Store top amount cards of deck
  int i, cards[amount];
  for (i = 0; i < amount; ++i)
	cards[i] = deck[i];

  // Remove them from deck
  obliterate_top_n_cards_of_deck(player, amount);

  int pos = count_deck(player);

  for (; amount > 0; --amount, ++pos)
	{
	  // Put a random card from array on bottom of deck
	  int selected = internal_rand(amount);
	  ASSERT(deck[pos] == -1);
	  ASSERT(pos == 0 || deck[pos - 1] != -1);
	  deck[pos] = cards[selected];

	  // Replace the randomly chosen card from array with the last one
	  cards[selected] = cards[amount - 1];
	}
}

void tutor_multiple_card_from_hand(int who_chooses, int t_player, int who_gets_the_card, int destination, int must_select, int ai_selection_mode, test_definition_t *this_test){
	int hand_copy[2][hand_count[t_player]];
	int hcc = 0;
	int count = 0;
	while( count < active_cards_count[t_player] ){
			if( in_hand(t_player, count) ){
				hand_copy[0][hcc] = get_original_internal_card_id(t_player, count);
				hand_copy[1][hcc] = count;
				hcc++;
			}
			count++;
	}
	this_test->zone = TARGET_ZONE_HAND;
	int amount = MIN(this_test->qty, check_battlefield_for_special_card(t_player, -1, t_player, CBFSC_GET_COUNT, this_test));
	if( amount ){
		int i;
		int tutored[amount];
		int tc = 0;
		while( amount ){
				int selected = select_card_from_zone(who_chooses, who_chooses, hand_copy[0], hcc, 0, ai_selection_mode, -1, this_test);
				if( selected != -1 ){
					tutored[tc] = hand_copy[0][selected];
					obliterate_card(t_player, hand_copy[1][selected]);
					tc++;
					for(i=selected; i<hcc; i++){
						hand_copy[0][i] = hand_copy[0][i+1];
						hand_copy[1][i] = hand_copy[1][i+1];
					}
					hcc--;
					amount--;
				}
				else{
					break;
				}
		}
		for(i=0; i<tc; i++){
			if( destination == TUTOR_PLAY || destination == TUTOR_PLAY_TAPPED ){
				int card_added = add_card_to_hand(who_gets_the_card, tutored[i]);
				if( destination == TUTOR_PLAY_TAPPED ){
					add_state(who_gets_the_card, card_added, STATE_TAPPED);
				}
				if( t_player != who_gets_the_card ){
					if( t_player == AI && who_gets_the_card == HUMAN){
						add_state(who_gets_the_card, card_added, STATE_OWNED_BY_OPPONENT);
					}
					if( t_player == HUMAN && who_gets_the_card == HUMAN){
						remove_state(who_gets_the_card, card_added, STATE_OWNED_BY_OPPONENT);
					}
				}
				put_into_play(who_gets_the_card, card_added);
			}
		}
	}
}
