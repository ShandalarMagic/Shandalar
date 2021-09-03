#include "manalink.h"


int card_hammer_of_bogardan(int player, int card, event_t event){
    card_instance_t *instance= get_card_instance(player, card);
    if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
        if( ! has_mana_multi(player, 2, 0, 0, 0, 3, 0) ){
            return -2;
        }
        int choice = do_dialog(player, player, card, -1, -1," Return Hammer to hand\n Leave Hammer be\n", 0);
        if( choice == 0 ){
            charge_mana_multi(player, 2, 0, 0, 0, 3, 0);
            if( spell_fizzled != 1 ){
                instance->state &= ~STATE_INVISIBLE;
                hand_count[player]++;
                return -1;
            }
            return -2;
        }
        else{
            return -2;
        }
    }
    return card_volcanic_hammer(player, card, event);
}

int card_smash_to_smithereens(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);
    if( event == EVENT_CAN_CAST ){
        if(  target_available(player, card, &td) ){
            return 1;
        }
    }
    else if( event == EVENT_CAST_SPELL ){
        if( ! affect_me(player, card) ){ return 0; }
        if( ! select_target(player, card, &td, "Select target artifact", NULL) ){
            spell_fizzled = 1;
        }
    }
    else if( event == EVENT_RESOLVE_SPELL ){
        card_instance_t *instance = get_card_instance(player, card);
        if(validate_target(player, card, &td, 0)){
            damage_player(instance->targets[0].player, 3, player, card);
            kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_incinerate(int player, int card, event_t event){
    if(event == EVENT_RESOLVE_SPELL){
        if( spell_fizzled != 1 ){
            card_instance_t *instance = get_card_instance(player, card);
            if( instance->targets[0].card > -1 ){
                int effect = create_legacy_effect_exe(player, card, LEGACY_EFFECT_GENERIC, instance->targets[0].player, instance->targets[0].card);
                card_instance_t *effect_instance = get_card_instance(instance->targets[0].player, effect);
                effect_instance->token_status =0x800000;
                card_instance_t *target_instance = get_card_instance(instance->targets[0].player, instance->targets[0].card);
                target_instance->regen_status = 0x8000000;
            }
        }
    }
    int result =  card_lightning_bolt(player, card, event);
    return result;
}


int is_goblin_tutor(int player, int card ){
    card_instance_t *instance = get_card_instance( player, card );
    if( cards_data[ instance->internal_card_id ].id == CARD_ID_GOBLIN_TUTOR ){
        return 1;
    }
    return 0;
}

int card_goblin_tutor(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL){
        int roll = internal_rand(6) + 1;
        int type = 0;;
        if( roll == 1 ){
            reveal_card(player, player, card, player, card, "You rolled 1, no effect", 1);
        }
        else if( roll == 2 ){
            reveal_card(player, player, card, player, card, "You rolled 2, search for Goblin Tutor", 1);
        }
        else if( roll == 3 ){
            reveal_card(player, player, card, player, card, "You rolled 3, find an Enchantment", 1);
            type = TYPE_ENCHANTMENT;
        }
        else if( roll == 4 ){
            reveal_card(player, player, card, player, card, "You rolled 4, find an Artifact", 1);
            type = TYPE_ARTIFACT;
        }
        else if( roll == 5 ){
            reveal_card(player, player, card, player, card, "You rolled 5, find a Creature", 1);
            type = TYPE_CREATURE;
        }
        else if( roll == 6 ){
            reveal_card(player, player, card, player, card, "You rolled 6, find an Instant or Sorcery", 1);
            type = TYPE_INSTANT | TYPE_SORCERY | TYPE_INTERRUPT;
        }

        if( roll > 2 ){
			if( type > 0 ){
				int mode = 0;
				if( roll == 6 ){
					mode = 2;
				}
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, type, mode, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 4, 0, 0, 0, 0, 0, 0, CARD_ID_GOBLIN_TUTOR, 0, -1, 0);
			}
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_goblin_grenade(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
    td.required_subtype = SUBTYPE_GOBLIN;
    td.allowed_controller = player;
    td.preferred_controller = player;

    card_instance_t *instance = get_card_instance(player, card);

    if(event == EVENT_CAN_CAST && target_available(player, card, &td)){
        return 1;
    }
    else if(event == EVENT_CAST_SPELL ){
        if(!affect_me(player, card)){ return 0; }
        if(!select_target(player, card, &td, "Sacrifice a Goblin", NULL)){
            spell_fizzled = 1;
        }
        else{
            kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE );

            default_target_definition(player, card, &td, TYPE_CREATURE );
            td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
            td.allow_cancel = 0;
            load_text(0, "TARGET_CREATURE_OR_PLAYER");
            if(!select_target(player, card, &td, text_lines[0], NULL)){
                spell_fizzled = 1;
            }
        }
    }
    else if(event == EVENT_RESOLVE_SPELL ){
        damage_creature_or_player(player, card, event, 5);
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_obliterate(int player, int card, event_t event){
    if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
        card_instance_t *instance = get_card_instance(player, card);
        instance->state |= STATE_CANNOT_TARGET;
    }
    return card_jokulhaups(player, card, event);
}

int card_fork(int player, int card, event_t event){
    int result =  card_fork_orig(player, card, event);
    if(event == EVENT_CAN_CAST ){
        if( result == 0 ){
            return 0;
        }
        card_data_t* card_d = get_card_data(card_on_stack_controller, card_on_stack);
        if( card_d->type & TYPE_CREATURE ){
            return 0;
        }
    }
    return result;
}


