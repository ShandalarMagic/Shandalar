#include "manalink.h"

int card_army_of_allah(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL){
        int p;
        for( p = 0; p < 2; p++){
            int count = 0;
            while(count < active_cards_count[p]){
                card_data_t* card_d = get_card_data(p, count);
                if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
                    card_instance_t *affected = get_card_instance(p, count);
                    if( affected->state & STATE_ATTACKING ){
                        pump_until_eot(player, card, p, count, 2, 0);
                    }
                }
                count++;
            }
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_rout(int player, int card, event_t event){
    if( ! is_unlocked(player, card, event, 10) ){ return 0; }
    if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) ){
        COST_COLORLESS += 2;
    }
    return card_wrath_of_god(player, card, event);
}

int card_dust_to_dust(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		if( player == AI ){
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
		}
		if (target_available(player, card, &td) > 1){
			return 1;
		}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;
			if (pick_target(&td, "TARGET_ARTIFACT")){
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
				new_pick_target(&td, "TARGET_ARTIFACT", 1, 1);
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		td.preferred_controller = 2;
			int i;
			for(i=0; i<2; i++){
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fire_and_brimstone(int player, int card, event_t event){
//    target_definition_t td;
//    default_target_definition(player, card, &td, TYPE_CREATURE);
//    td.zone = TARGET_ZONE_PLAYERs;
    card_instance_t *firenbrim = get_card_instance(player, card);
    if( current_phase <= PHASE_MAIN1 ){
        firenbrim->info_slot = 0;
    }
    else if( current_phase >= PHASE_DECLARE_ATTACKERS && current_phase <= PHASE_NORMAL_COMBAT_DAMAGE ){
        int i;
        for( i=0; i<active_cards_count[current_turn]; i++ ){
            card_instance_t *creature = get_card_instance(current_turn, i);
            if( creature->state & STATE_ATTACKING ){
                firenbrim->info_slot = 1;
                break;
            }
        }
    }
    else if( eot_trigger(player, card, event) ){
        firenbrim->info_slot = 0;
    }
    if( event == EVENT_CAN_CAST ){
        if( firenbrim->info_slot > 0 ){
            return 1;
        }
        else if( firenbrim->info_slot == 0 ){
            int sibling_card_can_be_cast = 0;
            int j;
            for( j=0; j<active_cards_count[player]; j++ ){
                card_instance_t *in_hand_instance = get_card_instance(player, j);
                if( in_hand(player, j) && !( in_hand_instance->state & STATE_INVISIBLE ) && get_id(player, j) == 746 && in_hand_instance->info_slot > 0 ){
                    sibling_card_can_be_cast = 1;
                    break;
                }
            }
            if( sibling_card_can_be_cast == 1 ){
                return 1;
            }
        }
    }
    else if( event == EVENT_RESOLVE_SPELL ){
        damage_player(current_turn, 4, player, card);
        damage_player(player, 4, player, card);
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int effect_festival(int player, int card, event_t event ){
    card_instance_t *instance = get_card_instance(player, card);
    if( affect_me(instance->targets[0].player, instance->targets[0].card) ){
        if( event == EVENT_ATTACK_LEGALITY ){
            event_result = 1;
        }
    }
    else if( eot_trigger(player, card, event) ){
        kill_card(player, card, KILL_REMOVE);
    }
    return 0;
}

int card_festival(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        if( current_turn == 1-player && current_phase == PHASE_UPKEEP ){
            return 1;
        }
    }
    else if( event == EVENT_RESOLVE_SPELL ){
        int i;
        for( i=0; i<active_cards_count[1-player]; i++ ){
            card_data_t* card_d = get_card_data(1-player, i);
            if( (card_d->type & TYPE_CREATURE) && in_play(1-player, i) ){
                int legacy_card = create_targetted_legacy_effect(player, card, &effect_festival, 1-player, i);
                card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
                legacy_instance->targets[0].player = 1-player;
                legacy_instance->targets[0].card = i;
            }
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

