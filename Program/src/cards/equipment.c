#include "manalink.h"

int card_bonesplitter(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if(event == EVENT_POWER && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result+=2;
	}
	return basic_equipment(player, card, event, 1);
}

int card_skullclamp(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) && ! is_humiliated(player, card) ){
		if(event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			card_instance_t *affected= get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code != KILL_REMOVE && ! check_special_flags2(player, card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE) ){
				instance->targets[11].player = 66;
			}
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, 2);
	}

	return vanilla_equipment(player, card, event, 1, 1, -1, 0, 0);
}

int card_lightning_greaves(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 0, 0, 0, KEYWORD_SHROUD, SP_KEYWORD_HASTE);
}

int card_empyrial_plate(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if((event==EVENT_TOUGHNESS || event == EVENT_POWER) && affect_me(instance->targets[8].player, instance->targets[8].card)  ){
		event_result += hand_count[player];
	}
	return basic_equipment(player, card, event, 2);
}

int card_loxodon_warhammer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 3, 0, KEYWORD_TRAMPLE);
		lifelink(instance->targets[8].player, instance->targets[8].card, event);
	}

	return basic_equipment(player, card, event, 3);

}

int card_sword_of_light_and_shadow(int player, int card, event_t event)
{
  /* Sword of Light and Shadow	|3
   * Artifact - Equipment
   * Equipped creature gets +2/+2 and has protection from |Swhite and from |Sblack.
   * Whenever equipped creature deals combat damage to a player, you gain 3 life and you may return up to one target creature card from your graveyard to your
   * hand.
   * Equip |2 */

  if (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_ABILITIES)
			event_result |= get_sleighted_protection(player, card, KEYWORD_PROT_WHITE | KEYWORD_PROT_BLACK);
		  else
			event_result += 2;
		}
	}

  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER)))
	for (; packets > 0; --packets)
	  {
		gain_life(player, 3);
		if (any_in_graveyard_by_type(player, TYPE_CREATURE))
		  global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	  }

  return basic_equipment(player, card, event, 2);
}

int card_sword_of_fire_and_ice(int player, int card, event_t event)
{
  /* Sword of Fire and Ice	|3
   * Artifact - Equipment
   * Equipped creature gets +2/+2 and has protection from |Sred and from |Sblue.
   * Whenever equipped creature deals combat damage to a player, ~ deals 2 damage to target creature or player and you draw a card.
   * Equip |2 */

  if (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_ABILITIES)
			event_result |= get_sleighted_protection(player, card, KEYWORD_PROT_RED | KEYWORD_PROT_BLUE);
		  else
			event_result += 2;
		}
	}

  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  for (; packets > 0; --packets)
		{
			instance->number_of_targets = 0;
			if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
				damage_target0(player, card, 2);
			draw_a_card(player);
		}

	}

  return basic_equipment(player, card, event, 2);
}

