#include "manalink.h"

int effect_sneak_attack(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance(player, card);
    if( affect_me(instance->targets[0].player, instance->targets[0].card) ){
        haste( instance->targets[0].player, instance->targets[0].card );
    }
    else if( eot_trigger(player, card, event ) ){
        kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
        kill_card(player, card, KILL_REMOVE);
    }
    return 0;
}

