#include "manalink.h"

int card_trinket_mage(int player, int card, event_t event){

    if( comes_into_play(player, card, event) ){
		global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, 2, 3);
	}

    return 0;
}


int card_broodstar(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);
    td.allowed_controller = player;
    td.preferred_controller = player;
    td.illegal_abilities = 0;
    int artifacts = target_available(player, card, &td);
    if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
        if(affect_me(player, card ) ){
            event_result += artifacts;
        }
    }
    else if(event == EVENT_MODIFY_COST ){
        COST_COLORLESS -= artifacts;
    }
    return 0;
}


int card_kami_of_the_crescent_moon(int player, int card, event_t event){
    check_legend_rule(player, card, event);
    if(event == EVENT_DRAW_PHASE){
        event_result++;
    }
    return 0;
}

int card_vodalian_knights(int player, int card, event_t event)
{
  islandhome(player, card, event);
  return card_manta_riders(player, card, event);
}

int card_synchronous_sliver(int player, int card, event_t event){
    if( in_play(player, card) && has_creature_type(affected_card_controller, affected_card, SUBTYPE_SLIVER ) ){
        vigilance( affected_card_controller, affected_card );
    }
    return 0;
}

