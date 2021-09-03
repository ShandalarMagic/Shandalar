#include "manalink.h"

int card_goblin_king(int player, int card, event_t event){
    boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 1, get_hacked_walk(player, card, KEYWORD_MOUNTAINWALK), 0);
    return 0;
}

int card_gorilla_shaman(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT );
    td.illegal_type = TYPE_CREATURE;
    card_instance_t *instance= get_card_instance(player, card);
    if( event == EVENT_CAN_ACTIVATE  && ! get_forbid_activation_flag(player, card) ){
        if(target_available(player, card, &td)){
            return 1;
        }
    }
    else if( event == EVENT_ACTIVATE ){
        if( select_target(player, card, &td, "Select a non-creature artifact", NULL) ){
			instance->number_of_targets = 1;
            int cost = (2*get_cmc( instance->targets[0].player, instance->targets[0].card ))+1;
            if( has_mana_for_activated_ability(player, card, cost, 0, 0, 0, 0, 0) ){
                charge_mana_for_activated_ability(player, card, cost, 0, 0, 0, 0, 0) ;
            }
            else{
                spell_fizzled = 1;
            }
        }
		else{
			spell_fizzled = 1;
		}
    }
    else if( event == EVENT_RESOLVE_ACTIVATION  && spell_fizzled != 1 ){
        kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
    }
    return 0;
}

int card_squee_goblin_nabob(int player, int card, event_t event){
    card_instance_t *instance= get_card_instance(player, card);
    check_legend_rule(player, card, event);
    if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
        int choice = 0;
        if( ! duh_mode(player) ){
            do_dialog(player, player, card, -1, -1," Return Squee to hand\n Leave Squee be\n", 0);
        }
        if( choice == 0 ){
            instance->state &= ~STATE_INVISIBLE;
            hand_count[player]++;
            return -1;
        }
        else{
            return -2;
        }
    }
    return 0;
}


int card_simian_spirit_guide(int player, int card, event_t event){
    if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
        return 1;
    }
    else if( event == EVENT_ACTIVATE_FROM_HAND ){
        // remove this from the game
        put_on_top_of_deck(player, card );
        rfg_top_card_of_deck(player);
    }
    else if( event == EVENT_REVOLVE_ACTIVATION_FROM_HAND ){
        produce_mana(player, COLOR_RED, 1);
    }
    return 0;
}

int is_goblin(int player, int card){
    if( has_subtype(player, card, SUBTYPE_GOBLIN )){
        return 1;
    }
    return 0;
}

int card_ogre_arsonist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
    td.allow_cancel = 0;

    if(target_available(player, card, &td) && comes_into_play(player, card, event) ){
		select_target(player, card, &td, "Select target land", NULL);
        kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
    }
	return 0;
}

int card_fire_imp(int player, int card, event_t event){
    return cip_damage_creature(player, card, event, 2);
}

int card_balduvian_horde(int player, int card, event_t event){

  if( comes_into_play(player, card, event ) ){
      int did_discard = 0;
	  if(hand_count[player] > 0){
         char buffer[50];
		 load_text(0, "BALDUVIAN_HORDE");
		 snprintf(buffer, 50, " %s\n %s", text_lines[0], text_lines[1]);
		 int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		 if(choice == 0){
			discard(player, 1, 0);
			did_discard = 1;
		 }
	  }
      if(!did_discard){
         kill_card(player, card, KILL_SACRIFICE);
      }
  }

  return 0;
}

int card_two_headed_dragon(int player, int card, event_t event){

    minimum_blockers(player, card, event, 2);

    creature_can_block_additional(player, card, event, 1);

    return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
}

