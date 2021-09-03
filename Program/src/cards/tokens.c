#include "manalink.h"

// Any special_infos that's a characteristic-defining ability needs to be special-cased in get_copyable_special_infos() in token_generation.c.

int card_dragon_token(int player, int card, event_t event){
	if( get_special_infos(player, card) == 66 ){
		devour(player, card, event, 2);
	}
	if( get_special_infos(player, card) == 67 && IS_GAA_EVENT(event) ){
		return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
	}
	return generic_token(player, card, event);
}

int card_beast_token(int player, int card, event_t event){
	return generic_token(player, card, event);
}

int card_insect(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affect_me(player, card) && get_special_infos(player, card) == 67){
		event_result |= KEYWORD_INFECT;
	}

	return generic_token(player, card, event);
}

int card_gold(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE)
	return can_sacrifice_this_as_cost(player, card) && can_produce_mana(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  ai_modifier -= 36;
	  if (produce_mana_all_one_color(player, get_card_instance(player, card)->mana_color, 1))
		{
		  tapped_for_mana_color = -2;
		  kill_card(player, card, KILL_SACRIFICE);
		}
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card))
	declare_mana_available_hex(player, get_card_instance(player, card)->card_color & (COLOR_TEST_ANY | COLOR_TEST_ARTIFACT), 1);

  return 0;
}

int card_golem_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_saproling(int player, int card, event_t event)
{
  // Tokens from Saproling Burst
  if (event == EVENT_ABILITIES && affect_me(player, card) && get_special_infos(player, card) == 66)	// well before power and toughness, but after change_type is complete
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->damage_source_player >= 0 && instance->damage_source_card >= 0 && in_play(instance->damage_source_player, instance->damage_source_card))
		{
		  int pt = count_counters(instance->damage_source_player, instance->damage_source_card, COUNTER_FADE);
		  instance->targets[5].player = instance->targets[5].card = pt;
		}
	  else
		kill_card(player, card, KILL_BURY);

	  return 0;
	}

  return generic_token(player, card, event);
}

int card_plant_token(int player, int card, event_t event){

  return generic_token(player, card, event);
}

int card_spirit2(int player, int card, event_t event){

	if( get_special_infos(player, card) == 67 ){
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

   return generic_token(player, card, event);
}

int card_thopter2(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE && get_special_infos(player, card) == 66 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)){
		return can_target(&td);
	}

	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
		  damage_creature_or_player(player, card, event, 1);
	}

	return generic_token(player, card, event);
}

int card_triskelavite(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_target(&td);
	}

	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			damage_creature_or_player(player, card, event, 1);
	}

	return generic_token(player, card, event);
}

int card_minion(int player, int card, event_t event){
	if (card == -1){
		return 0;
	}
	return generic_token(player, card, event);
}

int card_zombie(int player, int card, event_t event){
	return generic_token(player, card, event);
}

int card_illusion_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			event_result &= ~KEYWORD_FLYING;
		}
	}

	return generic_token(player, card, event);
}

int card_wolf_token(int player, int card, event_t event){
	if( get_special_infos(player, card) == 66 ){
		if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)){
			event_result += count_graveyard_by_id(2, CARD_ID_SOUND_THE_CALL);
		}
	}
	return generic_token(player, card, event);
}

// token made by kalitas
int card_vampire(int player, int card, event_t event){
	if (card == -1){
		return 0;
	}
	return generic_token(player, card, event);
}

int card_ooze(int player, int card, event_t event)
{
  if (card == -1)
	return 0;

  // Tokens from Gutter Grime
  if (event == EVENT_ABILITIES && affect_me(player, card) && get_special_infos(player, card) == 66)	// well before power and toughness, but after change_type is complete
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pt;
	  if (instance->damage_source_player >= 0 && instance->damage_source_card >= 0 && in_play(instance->damage_source_player, instance->damage_source_card))
		pt = count_counters(instance->damage_source_player, instance->damage_source_card, COUNTER_SLIME);
	  else
		pt = 0;
	  instance->targets[5].player = instance->targets[5].card = pt;
	}

  // "Splitting" Tokens from Mitotic Slime
  if (get_special_infos(player, card) == 67 && graveyard_from_play(player, card, event))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_OOZE, &token);
	  token.qty = 2;
	  token.pow = 1;
	  token.tou = 1;
	  generate_token(&token);
	}

  return generic_token(player, card, event);
}

int card_graveborn_token(int player, int card, event_t event){

	haste(player, card, event);

	if( get_special_infos(player, card) == 66 && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_token(player, card, event);
}

int card_wurm_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_snake_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_elemental_token(int player, int card, event_t event){

	if (card == -1){
		return 0;
	}

	if( get_special_infos(player, card) == 66 ){
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( get_special_infos(player, card) == 67 ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=count_subtype(player, TYPE_CREATURE, -1);
		}
	}

	return generic_token(player, card, event);
}

int card_demon_token(int player, int card, event_t event){
	return generic_token(player, card, event);
}

int card_twin_token(int player, int card, event_t event){
	if (card == -1){
		return 0;
	}
	return generic_token(player, card, event);
}

int card_human_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return generic_token(player, card, event);
}

int card_dragon_spirit(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int owner = get_owner(player, card);
		int csvid = CARD_ID_TATSUMASA_THE_DRAGONS_FANG;
		if( check_rfg(owner, csvid) ){
			int fake = get_internal_card_id_from_csv_id(csvid);
			remove_card_from_rfg(owner, csvid);
			int card_added = add_card_to_hand(owner, fake);
			put_into_play(owner, card_added);
		}
	}
	return generic_token(player, card, event);
}

int card_centaur_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_hellion_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_horror_token(int player, int card, event_t event){

	if (card == -1){
		return 0;
	}

	return generic_token(player, card, event);
}

int card_bird_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_goblin_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}
	if( get_special_infos(player, card) == 67 ){
		haste(player, card, event);
	}
	return generic_token(player, card, event);
}

int card_spider_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_bat_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && check_rfg(player, CARD_ID_SENGIR_NOSFERATU)  ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( check_rfg(player, CARD_ID_SENGIR_NOSFERATU)  ){
				int count = count_rfg(player);
				int *rfg = rfg_ptr[player];
				while( count > -1  ){
						if( cards_data[rfg[count]].id == CARD_ID_SENGIR_NOSFERATU ){
							int card_added = add_card_to_hand(player, rfg[count]);
							remove_card_from_rfg(player, CARD_ID_SENGIR_NOSFERATU);
							put_into_play(player, card_added);
							break;
						}
						count--;
				}
			}
		}
	}
	return generic_token(player, card, event);
}

int card_cat_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_ape_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_knight_token(int player, int card, event_t event){

	return generic_token(player, card, event);
}

int card_monk_token(int player, int card, event_t event){
	prowess(player, card, event);
	return generic_token(player, card, event);
}

int card_rhino_token(int player, int card, event_t event){

	if (card == -1){
		return 0;
	}

	if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && player == reason_for_trigger_controller
		&& get_special_infos(player, card) == 66 && eot_trigger(player, card, event)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_token(player, card, event);
}

int card_survivor_token(int player, int card, event_t event){
	return generic_token(player, card, event);
}

int card_assassin_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
			lose_the_game(1-player);
		}
	}

	return generic_token(player, card, event);
}

int card_elemental_cat(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}
	if( get_special_infos(player, card) == 67 ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=count_subtype(player, TYPE_CREATURE, -1);
		}
	}

	return generic_token(player, card, event);
}

int card_cleric_token(int player, int card, event_t event){

	if( get_special_infos(player, card) == 66 ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 3, 2, 0, 0, 0, 1) ){
				return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}

		if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 3, 2, 0, 0, 0, 1) ;
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			seek_grave_for_id_to_reanimate(player, card, player, CARD_ID_DEATHPACT_ANGEL, REANIMATE_DEFAULT);
		}
	}

	return generic_token(player, card, event);
}

int card_eldrazi_spawn(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE && get_special_infos(player, card) != 66 ){
		return 1;
	}
	if( event == EVENT_ACTIVATE ){
		produce_mana(player, COLOR_COLORLESS, 1);
		kill_card(player, card, KILL_SACRIFICE);
	}
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && get_special_infos(player, card) != 66 ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}
	if( get_special_infos(player, card) == 66 ){
		annihilator(player, card, event, 1);
	}
	return generic_token(player, card, event);
}

