#include "manalink.h"

int karoo(int player, int card, event_t event, int color1, int color2, int ai_card ){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
    td.allowed_controller = player;
    td.preferred_controller = player;
    td.allow_cancel = 0;
    td.illegal_abilities = 0;

    card_instance_t *instance = get_card_instance(player, card);
    if(player == AI ){
        int id = get_internal_card_id_from_csv_id(ai_card);
        instance->internal_card_id = id;
        return 0;
    }

    int result = two_mana_land(player, card, event, color1, color2);
    if( event == EVENT_RESOLVE_SPELL && player != AI ){
		int result2 = check_for_cip_effects_removal(player, card);
		if( result2 != 2 ){
			tap_card(player, card);
		}
        select_target(player, card, &td, "Choose a land to bounce", NULL);
        bounce_permanent( instance->targets[0].player, instance->targets[0].card );
    }
    return result;
}

int card_dryad_arbor(int player, int card, event_t event){
    colored_artifact(player, card, event);
    return mana_producer_fixed(player, card, event, COLOR_GREEN);
}


int card_forbidden_orchard(int player, int card, event_t event){
    if( event == EVENT_RESOLVE_ACTIVATION ){
        generate_token_by_id(opp, CARD_ID_SPIRIT);
    }
    return mana_producer(player, card, event);
}

int karoo_attempted_fix(int player, int card, event_t event, int color1, int color2, int ai_card ){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
    td.allowed_controller = player;
    td.preferred_controller = player;
    td.allow_cancel = 0;
    td.illegal_abilities = 0;

    card_instance_t *instance = get_card_instance(player, card);

    int result = two_mana_land(player, card, event, color1, color2);
    if( event == EVENT_RESOLVE_SPELL ){
        tap_card(player, card);
    }
    else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
        if( count_permanents_by_type(player, card, TYPE_LAND) == 0 ){
            ai_modifier -= 100;
        }
        else{
            life[HUMAN]=10;
            ai_modifier += 100;
        }
    }
    else if( comes_into_play(player, card, event) > 0 ){
        if( player == AI && count_permanents_by_type(player, card, TYPE_LAND) > 1 ){
            instance->state |= STATE_CANNOT_TARGET;
        }
        select_target(player, card, &td, "Choose a land to bounce", NULL);
        if( player == AI ){
            instance->state &= ~STATE_CANNOT_TARGET;
        }
        bounce_permanent( instance->targets[0].player, instance->targets[0].card );
    }
    return result;
}

/*
int check_for_non_karoo_land(int player, int mode ){

	int count = 0;
	while( 	count < active_cards_count[player] ){
			if( in_play(player, count) && is_what(player, count, TYPE_LAND) ){
				card_instance_t *instance = get_card_instance(player, count);
				if( instance->targets[17].card == -1 || 
					(instance->targets[17].card > 0 && ! (instance->targets[17].card & 4))
				  ){
					if( mode == 0 ){
						return 1;
					}
					if( mode == 1 ){
						return count;
					}
				}
			}
			count++;
	}
	int result = 0-mode;
	
	return result;
}
*/

int karoo2(int player, int card, event_t event, int color1, int color2){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
    td.allowed_controller = player;
    td.preferred_controller = player;
    td.allow_cancel = 0;
    td.illegal_abilities = 0;
	if( player == AI ){
		td.required_state = TARGET_STATE_TAPPED;
    }

    card_instance_t *instance = get_card_instance(player, card);
/*
    if(player == AI ){
        int id = get_internal_card_id_from_csv_id(ai_card);
        instance->internal_card_id = id;
        return 0;
    }
*/

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! can_target(&td) ){
			ai_modifier-=1000;
		}
    }

    if( event == EVENT_RESOLVE_SPELL ){
		if( player != AI ){
			select_target(player, card, &td, "Choose a land to bounce", NULL);
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
		}
		else{
			int trg = -1;
			int count = active_cards_count[player]-1;
			while(count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_LAND) && is_tapped(player, count) ){
						if( count != card ){
							trg = count;
							break;
						}
					}
					count--;
			}
			if( trg == -1 ){
				trg = card;
			}
			bounce_permanent(player, trg);
		}
		int result2 = check_for_cip_effects_removal(player, card);
		if( result2 != 2 && in_play(player, card) ){
			tap_card(player, card);
		}
    }
    return two_mana_land(player, card, event, color1, color2);
}


 int card_izzet_boilerworks(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_RED, COLOR_BLUE, CARD_ID_VOLCANIC_ISLAND );
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
 
 int card_gruul_turf(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_RED, COLOR_GREEN, CARD_ID_TAIGA);
 }
 
 int card_azorius_chancery(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_WHITE, COLOR_BLUE, CARD_ID_TUNDRA );
 }
 
 int card_rakdos_carnarium(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_RED, COLOR_BLACK, CARD_ID_BADLANDS );
 }
 
 int card_simic_growth_chamber(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_GREEN, COLOR_BLUE, CARD_ID_TROPICAL_ISLAND );
 }
 
 int card_orzhov_basilica(int player, int card, event_t event){
    return karoo( player, card, event, COLOR_WHITE, COLOR_BLACK, CARD_ID_SCRUBLAND );
 }

int sac_land(int player, int card, event_t event, int base_color, int color1, int color2){
	if( event == EVENT_RESOLVE_SPELL ){
		get_card_instance(player, card)->info_slot = (1<<base_color) | (1<<color1) | (1<<color2);
		int result = check_for_cip_effects_removal(player, card);
		if( result != 2 ){
			tap_card(player, card);
		}
	}
    else if(event == EVENT_ACTIVATE ){
        int choice = 0;
        if( ! paying_mana() ){
            choice = do_dialog(player, player, card, -1, -1, " 1 Mana Ability\n Sac Ability\n Cancel", 0);
        }
        if( choice == 1 ){
			produce_mana_tapped2(player, card, color1, 1, color2, 1);
            kill_card(player, card, KILL_SACRIFICE);
        }
        else if(choice == 2){
            spell_fizzled = 1;
        }
        else{
            return mana_producer_fixed(player, card, event, base_color);
        }
		return 0;
    }

	return mana_producer_fixed(player, card, event, base_color);
}

int card_dwarven_ruins(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_RED, COLOR_RED, COLOR_RED );
}

int card_ebon_stronghold(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_BLACK, COLOR_BLACK, COLOR_BLACK );
}

int card_havenwood_battleground(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_GREEN, COLOR_GREEN, COLOR_GREEN );
}

int card_ruins_of_trokair(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE );
}

int card_svyelunite_temple(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_BLUE, COLOR_BLUE, COLOR_BLUE );
}

int card_ancient_spring(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK );
}

int card_geothermal_crevice(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_RED, COLOR_BLACK, COLOR_GREEN );
}

int card_irrigation_ditch(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_WHITE, COLOR_GREEN, COLOR_BLUE );
}

int card_sulfur_vent(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_BLACK, COLOR_RED, COLOR_BLUE );
}

int card_tinder_farm(int player, int card, event_t event){
    return sac_land(player, card, event, COLOR_GREEN, COLOR_RED, COLOR_WHITE );
}

int card_cloudpost(int player, int card, event_t event){

    comes_into_play_tapped(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

    if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
        declare_mana_available(player, COLOR_COLORLESS, count_subtype(2, TYPE_LAND, SUBTYPE_LOCUS) );
    }

    if(event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
		return 1;
    }

    if( event == EVENT_ACTIVATE){
		produce_mana_tapped(player, card, COLOR_COLORLESS, count_subtype(2, TYPE_LAND, SUBTYPE_LOCUS));
    }
	
    return 0;
}

void play_land_sound_effect(int player, int card){
	card_data_t* cd = get_card_data(player, card);
	play_land_sound_effect_force_color(player, card, cd->color);
}

void play_land_sound_effect_force_color(int player, int card, int colors){
	if (!is_what(player, card, TYPE_LAND)){
		// For e.g. Magus of the Coffers and other cards call lands' functions
		return;
	}

	colors &= COLOR_TEST_ANY_COLORED;	// so e.g. COLOR_TEST_WHITE below also includes COLOR_TEST_WHITE|COLOR_TEST_COLORLESS, for Karoo; and COLOR_TEST_ARTIFACT is treated like COLOR_TEST_COLORLESS.

	wav_t wav1 = -1, wav2 = -1;
	switch (colors){
		// colorless
		case 0:					wav1 = WAV_GREY;	break;

		// one color
		case COLOR_TEST_BLACK:	wav1 = WAV_BLACK;	break;
		case COLOR_TEST_BLUE:	wav1 = WAV_BLUE;	break;
		case COLOR_TEST_GREEN:	wav1 = WAV_GREEN;	break;
		case COLOR_TEST_RED:	wav1 = WAV_RED;		break;
		case COLOR_TEST_WHITE:	wav1 = WAV_WHITE;	break;

		// two colors
		case COLOR_TEST_WHITE|COLOR_TEST_BLUE:	wav1 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_BLUE|COLOR_TEST_BLACK:	wav1 = WAV_BLUEBLACK;	break;
		case COLOR_TEST_BLACK|COLOR_TEST_RED:	wav1 = WAV_BLACKRED;	break;
		case COLOR_TEST_RED|COLOR_TEST_GREEN:	wav1 = WAV_REDGREEN;	break;
		case COLOR_TEST_GREEN|COLOR_TEST_WHITE:	wav1 = WAV_GREENWHITE;	break;

		case COLOR_TEST_WHITE|COLOR_TEST_BLACK:	wav1 = WAV_WHITEBLACK;	break;
		case COLOR_TEST_BLACK|COLOR_TEST_GREEN:	wav1 = WAV_BLACKGREEN;	break;
		case COLOR_TEST_GREEN|COLOR_TEST_BLUE:	wav1 = WAV_GREENBLUE;	break;
		case COLOR_TEST_BLUE|COLOR_TEST_RED:	wav1 = WAV_BLUERED;		break;
		case COLOR_TEST_RED|COLOR_TEST_WHITE:	wav1 = WAV_REDWHITE;	break;

		// three colors - bit of a hack
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_WHITE|COLOR_TEST_BLUE):	wav1 = WAV_RED;		wav2 = WAV_BLACKGREEN;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLUE|COLOR_TEST_BLACK):	wav1 = WAV_GREEN;	wav2 = WAV_REDWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLACK|COLOR_TEST_RED):	wav1 = WAV_WHITE;	wav2 = WAV_GREENBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_RED|COLOR_TEST_GREEN):	wav1 = WAV_BLUE;	wav2 = WAV_WHITEBLACK;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_GREEN|COLOR_TEST_WHITE):	wav1 = WAV_BLACK;	wav2 = WAV_BLUERED;		break;

		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_WHITE|COLOR_TEST_BLACK):	wav1 = WAV_BLUE;	wav2 = WAV_REDGREEN;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLACK|COLOR_TEST_GREEN):	wav1 = WAV_RED;		wav2 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_GREEN|COLOR_TEST_BLUE):	wav1 = WAV_WHITE;	wav2 = WAV_BLACKRED;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLUE|COLOR_TEST_RED):	wav1 = WAV_BLACK;	wav2 = WAV_GREENWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_RED|COLOR_TEST_WHITE):	wav1 = WAV_GREEN;	wav2 = WAV_BLUEBLACK;	break;


		// four colors - slightly less of a hack.
		// (While there aren't any naturally four-color lands that I know of, these can still be played by Reflecting Pool, Exotic Orchard, etc.)
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_BLACK:	wav1 = WAV_REDGREEN;	wav2 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_BLUE:		wav1 = WAV_BLACKRED;	wav2 = WAV_GREENWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_GREEN:	wav1 = WAV_WHITEBLUE;	wav2 = WAV_BLACKRED;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_RED:		wav1 = WAV_GREENWHITE;	wav2 = WAV_BLUEBLACK;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_WHITE:	wav1 = WAV_BLUEBLACK;	wav2 = WAV_REDGREEN;	break;

		// five colors - going with gembazar, even though City of Brass is silent.  (It's better than polkamix?)
		case COLOR_TEST_ANY_COLORED:			wav1 = WAV_GEMBAZAR;	break;
	}

	if (wav1 != (wav_t)-1){
		play_sound_effect(wav1);
	}
	if (wav2 != (wav_t)-1){
		play_sound_effect(wav2);
	}
}
