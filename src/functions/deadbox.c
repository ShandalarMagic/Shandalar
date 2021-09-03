#include "manalink.h"

#define TOKEN_FLAG (1<<31)

void increase_dead_count(int player, int iid, int is_tok){

	int dbc = get_deadbox_card(player);
	if( dbc < 0 ){
		return;
	}
	card_instance_t *instance = get_card_instance( player, dbc );

	if (is_tok){
		iid |= TOKEN_FLAG;
	}

	int* q;
	for (q = &instance->targets[0].player; q <= &instance->targets[17].card; ++q){
		if (*q == -1){
			*q = iid;
			break;
		}
	}

	++hack_silent_counters;
	add_counter(player, dbc, COUNTER_DEATH);
	if( is_tok == 0 ){
		add_counter(player, dbc, COUNTER_CORPSE);
	}
	--hack_silent_counters;

}

int get_dead_count(int player, int mode){
	// mode is the card_type to check plus the flags check "gdc_flags_t" in "manalink.h" for the detail of "mode"

	int amount = 0;

	int p, *q;
	for (p = 0; p < 2; ++p){
		if (p == player || player == 2){
			int dbc = get_deadbox_card(p);
			if (dbc > -1){
				if (mode == TYPE_PERMANENT || mode == TYPE_ANY){
					amount = count_counters(p, dbc, COUNTER_DEATH);
				} else if (mode == (TYPE_PERMANENT|GDC_NONTOKEN) || mode == (TYPE_ANY|GDC_NONTOKEN)){
					amount = count_counters(p, dbc, COUNTER_CORPSE);
				} else {
					card_instance_t* instance = get_card_instance(p, dbc);
					for (q = &instance->targets[0].player; q <= &instance->targets[18].card; ++q){
						if (*q != -1){
							int tok = (*q & TOKEN_FLAG) ? TARGET_TYPE_TOKEN : 0;
							int iid = *q & ~TOKEN_FLAG;
							int typ = get_type(-1, iid) | tok;
							/* Note that TYPE_ENCHANTMENT won't be set for planeswalkers here.  That turns out not to be a problem, since all valid combations
							 * of TYPE_PERMANENT/TYPE_ANY are checked for above - except for combinations of those with GDC_NONPLANESWALKER, which we want to
							 * fail anyway. */

							if ((mode & typ)
								&& !((mode & GDC_NONTOKEN) && tok)
								&& !((mode & GDC_NONPLANESWALKER) && (typ & TARGET_TYPE_PLANESWALKER))){
								++amount;
							}
						}
					}
				}
			}
		}
	}
	return amount;
}

void reset_dead_count(void){

	int p;
	for(p=0; p<2; p++){
		int dbc = get_deadbox_card(p);
		if( dbc < 0 ){
			return;
		}
		if( dbc != -1 ){
			card_instance_t *instance = get_card_instance( p, dbc );
			remove_all_counters(p, dbc, -1);
			int i;
			for(i=0; i<18; i++){
				instance->targets[i].player = -1;
				instance->targets[i].card = -1;
			}
		}
	}
}

int card_deadbox(int player, int card, event_t event){

	if (IS_AI(player)){
		return 0;
	}

	if(event == EVENT_GRAVEYARD_FROM_PLAY){
		if( affected_card_controller == -1 || affected_card == -1 ){ return 0; }
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		card_instance_t *dead = get_card_instance( affected_card_controller, affected_card );
		if( dead->kill_code > 0 && dead->kill_code != KILL_REMOVE ){
			if( is_what(affected_card_controller, affected_card, TYPE_PERMANENT) ){
				int t_player = is_stolen(affected_card_controller, affected_card) ? 1-affected_card_controller : affected_card_controller;
				/* Roughly half the deadbox uses want the state of the card as it left play (i.e., internal_card_id - for example, the Innistrad block morbid
				 * ability should activate for a Sculpting Steel destroyed while copying an artifact creature) and half want it as it is in the graveyard (i.e.,
				 * original_internal_card_id - for example, Grim Return should not be able to target a Sculpting Steel destroyed while copying an artifact
				 * creature).
				 *
				 * I'm not even going to try to sort them out.  For now, store original_internal_card_id mainly because internal_card_id's of dynamically-
				 * created card types stop being valid at end of turn if there's no card in play still using them.
				 *
				 * Probably the way forward is to store the card's type as it dies in the high word of each half-target - only 14 bits are needed for an
				 * internal_card_id.  We'd want to juggle everything in type_t to fit into a word first; currently TARGET_TYPE_PLANESWALKER does not. */
				increase_dead_count(t_player, get_original_internal_card_id(affected_card_controller, affected_card), is_token(affected_card_controller, affected_card));
			}
		}
	}

	if (event == EVENT_CAN_ACTIVATE){
		return !IS_AI(player) && (count_counters(player, card, COUNTER_CORPSE) || count_counters(1-player, get_deadbox_card(1-player), COUNTER_CORPSE));
	}

	if( event == EVENT_ACTIVATE ){
		char buffer[600];
		int i;
		for(i=0; i<2; i++){
			int dbc = get_deadbox_card(i);
			if( dbc > -1 ){
				int pos = 0;
				if( i == player ){
					pos += scnprintf(buffer, 600, "Your");
				}
				else{
					pos += scnprintf(buffer, 600, "Your opponent's");
				}
				pos += scnprintf(buffer+pos, 600-pos, " Deadbox contains:\n");
				card_instance_t* instance = get_card_instance(i, dbc);
				int q;
				int amount = 0;
				for (q = 0; q < 18; ++q){
					if( instance->targets[q].player != -1 ){
						int iid = instance->targets[q].player;
						if( iid & TOKEN_FLAG ){
							iid &= ~TOKEN_FLAG;
						}
						card_ptr_t* c = cards_ptr[ cards_data[iid].id ];
						pos += scnprintf(buffer + pos, 600-pos, " %s\n", c->name);
						amount++;
					}
					if( instance->targets[q].card != -1 ){
						int iid = instance->targets[q].card;
						if( iid & TOKEN_FLAG ){
							iid &= ~TOKEN_FLAG;
						}
						card_ptr_t* c = cards_ptr[ cards_data[iid].id ];
						pos += scnprintf(buffer + pos, 600-pos, " %s\n", c->name);
						amount++;
					}
				}
				if( amount > 0 ){
					do_dialog(player, player, card, -1, -1, buffer, 0);
				}
			}
		}
		cancel = 1;
	}

	if( event == EVENT_CAN_SKIP_TURN && player == 0 ){
		reset_dead_count();
	}

	return 0;
}

static void seek_grave_to_reanimate_this_turn(int player, int selected_type, reanimate_mode_t mode){

	int non_pw = selected_type & GDC_NONPLANESWALKER;
	selected_type &= ~(GDC_NONTOKEN | GDC_NONPLANESWALKER);	/* we effectively force nontoken anyway, just in case there's a different nontoken card of the same
															 * name in the graveyard */

	int p, *q;
	for(p=0; p<2; p++){
		if( p == player || player == 2 ){
			int dbc = get_deadbox_card(p);
			if( dbc > -1 ){
				card_instance_t *instance = get_card_instance( p, dbc );
				for (q = &instance->targets[0].player; q <= &instance->targets[17].card; ++q){
					if (*q != -1 && !(*q & TOKEN_FLAG)
						&& is_what(-1, *q, selected_type)
						&& !(non_pw && is_planeswalker(-1, *q))){
						seek_grave_for_id_to_reanimate(player, -1, p, cards_data[*q].id, mode);
					}
				}
			}
		}
	}
}

void reanimate_all_dead_this_turn(int player, int selected_type){
	return seek_grave_to_reanimate_this_turn(player, selected_type, REANIMATE_DEFAULT);
}

void return_all_dead_this_turn_to_hand(int player, int selected_type){
	return seek_grave_to_reanimate_this_turn(player, selected_type, REANIMATE_RETURN_TO_HAND);
}

int choose_a_dead_this_turn(int player, int card, int t_player, int can_cancel, int ai_selection_mode, test_definition_t *this_test, int target_pos){

	card_instance_t *instance = get_card_instance(player, card);

	int new_array[50][2];
	int na_count[2] = {0, 0};
	int p;
	int my_test = new_get_test_score(this_test);
	for(p=0; p<2; p++){
		if( p == t_player || t_player == 2 ){
			int dbc = get_deadbox_card(p);
			if( dbc > -1 ){
				card_instance_t *deadb = get_card_instance( p, dbc );
				int i;
				for(i=0; i<18; i++){
					int iid = deadb->targets[i].player;
					if( iid != -1 && !(iid & TOKEN_FLAG) && new_make_test(p, iid, my_test, this_test) ){
						new_array[na_count[p]][p] = iid;
						na_count[p]++;
					}
					iid = deadb->targets[i].card;
					if( iid != -1 && !(iid & TOKEN_FLAG) && new_make_test(p, iid, my_test, this_test) ){
						new_array[na_count[p]][p] = iid;
						na_count[p]++;
					}
				}
			}
		}
	}
	for(p=0; p<2; p++){
		if( na_count[p] > 0 ){
			int show_array[50];
			int q;
			for(q=0; q<50; q++){
				show_array[q] = new_array[q][p];
			}
			int selected = select_card_from_zone(player, t_player, show_array, na_count[p], can_cancel, ai_selection_mode, -1, this_test);
			if( selected != -1 ){
				instance->targets[target_pos].player = p;
				instance->targets[target_pos+1].player = selected;
				instance->targets[target_pos+1].card = new_array[selected][p];
				return selected;
			}
		}
	}
	return -1;
}

// This is inaccurate, as there's no way to know if a specific card went into graveyard from play after the exact moment it happens
int has_type_dead_this_turn_in_grave(int player, test_definition_t *this_test){
	int p;
	for(p=0; p<2; p++){
		if( p == player || player == 2 ){
			int dbc = get_deadbox_card(p);
			if( dbc > -1 ){
				card_instance_t *instance = get_card_instance( p, dbc );
				int i = 0;
				for(i=0; i<18; i++){
					int iid = instance->targets[i].player;
					if( iid != -1 && !(iid & TOKEN_FLAG) && new_make_test(p, iid, -1, this_test) ){
						if( seek_grave_for_id_to_reanimate(player, -1, p, cards_data[iid].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD) > -1 ){
							return 1;
						}
					}
					iid = instance->targets[i].card;
					if( iid != -1 && !(iid & TOKEN_FLAG) && new_make_test(p, iid, -1, this_test) ){
						if( seek_grave_for_id_to_reanimate(player, -1, p, cards_data[iid].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD) > -1 ){
							return 1;
						}
					}
				}
			}
		}
	}
	return 0;
}
