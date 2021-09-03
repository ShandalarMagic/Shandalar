#include "manalink.h"

int card_sundering_titan(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

    if( (comes_into_play(player, card, event) > 0 || leaves_play(player, card, event)) ){
		instance->number_of_targets = 0;

        // target each land
        target_definition_t td;
        default_target_definition(player, card, &td, TYPE_LAND);
        td.allow_cancel = 0;
		
		int trgs = 0;

        td.required_subtype = SUBTYPE_SWAMP;
        if( target_available(player, card, &td) ){
            if( select_target(player, card, &td, "Choose a Swamp", &(instance->targets[trgs])) ){
				trgs++;
			}
        }

        td.required_subtype = SUBTYPE_ISLAND;
        if( target_available(player, card, &td) ){
            if( select_target(player, card, &td, "Choose an Island", &(instance->targets[trgs])) ){
				trgs++;
			}
        }

        td.required_subtype = SUBTYPE_FOREST;
        if( target_available(player, card, &td) ){
            if( select_target(player, card, &td, "Choose a Forest", &(instance->targets[trgs])) ){
				trgs++;
			}
        }


        td.required_subtype = SUBTYPE_MOUNTAIN;
        if( target_available(player, card, &td) ){
            if( select_target(player, card, &td, "Choose a Mountain", &(instance->targets[trgs])) ){
				trgs++;
			}
        }


        td.required_subtype = SUBTYPE_PLAINS;
        if( target_available(player, card, &td) ){
            if( select_target(player, card, &td, "Choose a Plains", &(instance->targets[trgs])) ){
				trgs++;
			}
        }

        // Now destroy each land
		int i;
        for(i=0;i<trgs;i++){
            if( instance->targets[i].player != -1 && in_play( instance->targets[i].player, instance->targets[i].card ) ){
                kill_card( instance->targets[i].player, instance->targets[i].card, KILL_DESTROY );
            }
        }

    }

    return 0;
}


int card_darksteel_colossus(int player, int card, event_t event){
    if( event == EVENT_GRAVEYARD_ABILITY ){
        remove_card_from_grave(player, card);
        int *deck = deck_ptr[player];
        deck[ count_deck(player) ] = get_internal_card_id_from_csv_id( CARD_ID_DARKSTEEL_COLOSSUS );
        gain_life(player, 0);
        shuffle(player);
    }
    else{
        indestructible(player, card, event);
    }
    return 0;
}

int card_bottle_gnomes(int player, int card, event_t event){

    if( event == EVENT_RESOLVE_ACTIVATION ){
        gain_life(player, 3);
    }

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_phyrexian_dreadnought(int player, int card, event_t event){

    if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && 
        reason_for_trigger_controller == player ){

        int trig = 0;

        if( trigger_cause_controller == player && trigger_cause == card ){
            trig = 1;
        }

        if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
            trig = 0;
        }

        if( trig == 1 ){
            if( event == EVENT_TRIGGER){
                event_result |= 2;
            }
            else if( event == EVENT_RESOLVE_TRIGGER ){
					card_instance_t *instance = get_card_instance(player, card);
					instance->state |= STATE_CANNOT_TARGET;
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.allowed_controller = player;
					td.preferred_controller = player;
					td.illegal_abilities = 0;
					int total_power = 0;

					int i=0;
					while( can_target(&td) && spell_fizzled != 1 ){
						if( i > 0 ){
							instance->targets[12] = instance->targets[0];
						}
						if( pick_target(&td, "LORD_OF_THE_PIT") ){
							card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
							target->state |= STATE_CANNOT_TARGET;
							if( i > 0 ){
								instance->targets[i] = instance->targets[0];
								instance->targets[0] = instance->targets[12];
							}
							i++;
						}
						else if (i > 0 ){
							instance->targets[0] = instance->targets[12];
						}
					 }

					int j=0;
					for(j=0;j<i;j++){
						total_power += get_power( instance->targets[j].player, instance->targets[j].card);
						kill_card( instance->targets[j].player, instance->targets[j].card, KILL_SACRIFICE);
					}
					if( total_power < 12 ){
						kill_card(player, card, KILL_SACRIFICE);
					}

					instance->state &= ~STATE_CANNOT_TARGET;
			}
		}
	}

    return 0;
}

