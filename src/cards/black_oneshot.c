#include "manalink.h"


int card_perish(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL){
        int p;
        int color = 1 << get_sleighted_color(player, card, COLOR_GREEN);
        for( p = 0; p < 2; p++){
            int count = active_cards_count[p] -1;
            while(count >= 0 ){
                card_data_t* card_d = get_card_data(p, count);
                if((card_d->type & TYPE_CREATURE) && in_play(p, count) && ( get_color(p, count) & color ) ){
                    kill_card( p, count, KILL_BURY );
                }
                count--;
            }
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_diabolic_edict(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE );
    td.zone = TARGET_ZONE_PLAYERS;

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAN_CAST ){
        return can_target(&td);
    }
    else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
    }
    else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY );
    }
    return 0;

}

int card_infernal_contract(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
		int result = 1;
		if( player == AI && !(life[player] > 1 || ! can_pay_life(player, 1)) ){
			result = 0;
		}
		return result;
    }
    else if( event == EVENT_RESOLVE_SPELL ){
			int amount = life[player]/2;
			lose_life(player, amount);	
			draw_cards(player, 4);
			kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}


int card_unmask(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE);
    td1.zone = TARGET_ZONE_PLAYERS;
	
    card_instance_t *instance = get_card_instance(player, card);

	al_pitchspell(player, card, event, COLOR_TEST_BLACK, 0);

    if(event == EVENT_CAN_CAST ){
		return can_target(&td1);
    }

    if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLACK, 0) ){
			pick_target(&td1, "TARGET_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
    }
    if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			effect_coercion(instance->targets[0].player, player, 1, 1, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
    }
	return 0;
}

int card_demonic_consultation(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
        int *deck = deck_ptr[player];
        int choice = get_internal_card_id_from_csv_id( cards_data[ deck[6] ].id );
        if( player == HUMAN ){
            if( ai_is_speculating != 1 ){
                choice = choose_a_card("Choose a card", -1, -1);
            }
        }

        // remove the top 6
        int i=0;
        for(i=0;i<6;i++){
            rfg_top_card_of_deck(player);
        }
        while( deck[0] != -1 && deck[0] != choice ){
            rfg_top_card_of_deck(player);
        }

        // put the card into hand
        if( deck[0] != -1 ){
            add_card_to_hand(player, deck[0] );
            remove_card_from_deck( player, 0 );
        }

        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}


int card_death_cloud(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
        instance->info_slot = x_value;
    }
    else if( event == EVENT_RESOLVE_SPELL ){
        int j;
        for(j=0;j<2;j++){
            int p = player;
            if( j == 1){ p = 1-player; }
			lose_life(p, instance->info_slot);

            int count = instance->info_slot;

			multidiscard(p, count, 0);

			impose_sacrifice(player, card, p, count, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);

			impose_sacrifice(player, card, p, count, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);

        }
        kill_card(player, card, KILL_DESTROY);
    }
    else if ( event == EVENT_CAN_CAST ){
			return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
    }
    return 0;
}

int card_smallpox(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
			int p, i;
			for(i=0;i<2;i++){
				p = player;
				if( i == 1){ p = 1-player; }
				lose_life(p, 1);
				discard(p, 0, 0);
				impose_sacrifice(player, card, i, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				impose_sacrifice(player, card, i, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_pox(int player, int card, event_t event){
    if( event == EVENT_RESOLVE_SPELL ){
        int p, j;
        for(j=0;j<2;j++){
            p = player;
            if( j == 1){ p = 1-player; }
			int amount = (life[p]+2)/3;
			lose_life(p, amount);

            int count = ( (hand_count[p]+2)/3 );
            int i;
            for(i=0;i<count;i++){
                discard(p, 0, 0);
            }

            // sac a creature
            count = count_subtype(p, TYPE_CREATURE, -1)/3;
			impose_sacrifice(player, card, p, count, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);

            // sac a land
            count = count_subtype(p, TYPE_LAND, -1)/3;
			impose_sacrifice(player, card, p, count, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);

        }
        kill_card(player, card, KILL_DESTROY);
    }
    else if ( event == EVENT_CAN_CAST ){
        return 1;
    }
    return 0;
}

int card_barter_in_blood(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
        int i;
        for(i=0;i<2;i++){
            int p = player;
            if( i == 1){ p = 1-player; }
			impose_sacrifice(player, card, p, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_hellfire(int player, int card, event_t event){
    if(event == EVENT_RESOLVE_SPELL){
        int p;
        int dead=0 ;
        for( p = 0; p < 2; p++){
            int count = 0;
            while(count < active_cards_count[p]){
                card_data_t* card_d = get_card_data(p, count);
                card_instance_t *instance = get_card_instance( p, count );
                if((card_d->type & TYPE_CREATURE) && in_play(p, count) && !( instance->card_color & COLOR_TEST_BLACK) ){
                    kill_card( p, count, KILL_DESTROY );
                    dead++;
                }
                count++;
            }
        }
        damage_player(player, dead+3, player, card);
        kill_card(player, card, KILL_DESTROY);
    }
    else if(event == EVENT_CAN_CAST ){
        return 1;
    }
    return 0;
}


