#include "manalink.h"

// Cards that couls change targets

static const char* has_only_1_target_and_could_legally_change_target(int who_chooses, int player, int card)
{
	card_instance_t* eff = get_card_instance(player, card);
	if( eff->number_of_targets == 0 || eff->number_of_targets > 1)
		return "must have only 1 target";
	if( ! call_card_function(player, card, EVENT_CAN_CHANGE_TARGET) )
		return "has only 1 legal target";

	return NULL;
}

int card_deflection(int player, int card, event_t event){

	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);	// Aura spells also target
	td_spell.extra = (int32_t)has_only_1_target_and_could_legally_change_target;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, &td_spell, 0);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		instance->targets[0].player = card_on_stack_controller;
		instance->targets[0].card = card_on_stack;
		call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_CHANGE_TARGET);
		if( spell_fizzled != 1 ){
			instance->targets[1] = get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9];
			instance->number_of_targets = 2;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].player = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].card = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[1];
		}
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_divert(int player, int card, event_t event){

	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)has_only_1_target_and_could_legally_change_target;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_CAST ){
		return card_deflection(player, card, event);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( has_mana(card_on_stack_controller, COLOR_COLORLESS, 2) )
			ai_modifier-=25;
		return card_deflection(player, card, event);
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			charge_mana(instance->targets[0].player, COLOR_COLORLESS, 2);
			if( spell_fizzled == 1 ){
				get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[1];
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_imps_michief(int player, int card, event_t event){

	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)has_only_1_target_and_could_legally_change_target;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_CAST ){
		return card_deflection(player, card, event);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( life[player]-get_cmc(card_on_stack_controller, card_on_stack) < 6)
			ai_modifier-=25;
		return card_deflection(player, card, event);
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[1];
			lose_life(player, get_cmc(instance->targets[0].player, instance->targets[0].card));
		}
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_misdirection(int player, int card, event_t event){

	al_pitchspell(player, card, event, COLOR_TEST_BLUE, 1);

	if (event == EVENT_CAN_CAST ){
		return card_deflection(player, card, event);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLUE, 1) ){
			set_special_flags2(card_on_stack_controller, card_on_stack, SF2_MISDIRECTION);
			card_deflection(player, card, event);
			remove_special_flags2(card_on_stack_controller, card_on_stack, SF2_MISDIRECTION);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		return card_deflection(player, card, event);
	}

	return 0;
}

int quicksilver_dragon_card = -1;
static const char* has_only_1_target_and_could_legally_change_target_and_targets_me(int who_chooses, int player, int card)
{
	card_instance_t* eff = get_card_instance(player, card);
	if( eff->number_of_targets == 0 || eff->number_of_targets > 1)
		return "must have only 1 target";
	if( !(eff->targets[0].player == who_chooses && eff->targets[0].card == quicksilver_dragon_card) )
		return "must target Quicksilver Dragon";
	if( ! call_card_function(player, card, EVENT_CAN_CHANGE_TARGET) )
		return "has only 1 legal target";

	return NULL;
}

int card_quicksilver_dragon(int player, int card, event_t event){
	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)has_only_1_target_and_could_legally_change_target_and_targets_me;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	quicksilver_dragon_card = card;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, 0, 0, 1, 0, 0, 0, 0, &td_spell, NULL) ){
			if( call_card_function(card_on_stack_controller, card_on_stack_controller, EVENT_CHANGE_TARGET) ){
				return 99;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		set_special_flags2(card_on_stack_controller, card_on_stack_controller, SF2_QUICKSILVER_DRAGON);
		call_card_function(card_on_stack_controller, card_on_stack_controller, EVENT_CHANGE_TARGET);
		if( spell_fizzled != 1 ){
			instance->targets[0] = get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9];
			instance->number_of_targets = 1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].player = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].card = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->number_of_targets = 1;
		}
		remove_special_flags2(card_on_stack_controller, card_on_stack_controller, SF2_QUICKSILVER_DRAGON);
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[0];
		}
	}
	return 0;
}

static const char* has_only_1_target_and_could_legally_change_target_and_targets_a_player(int who_chooses, int player, int card)
{
	card_instance_t* eff = get_card_instance(player, card);
	if( eff->number_of_targets == 0 || eff->number_of_targets > 1)
		return "must have only 1 target";
	if( eff->targets[0].card != -1 )
		return "must target a player";
	if( ! call_card_function(player, card, EVENT_CAN_CHANGE_TARGET) )
		return "has only 1 legal target";

	return NULL;
}

int card_rebound(int player, int card, event_t event){

	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)has_only_1_target_and_could_legally_change_target_and_targets_a_player;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, &td_spell, 0);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		instance->targets[0].player = card_on_stack_controller;
		instance->targets[0].card = card_on_stack;
		set_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_REBOUND);
		call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_CHANGE_TARGET);
		if( spell_fizzled != 1 ){
			instance->targets[1] = get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9];
			instance->number_of_targets = 2;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].player = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].card = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->number_of_targets = 1;
		}
		remove_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_REBOUND);
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[1];
		}
		kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

