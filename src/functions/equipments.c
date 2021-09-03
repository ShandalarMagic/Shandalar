#include "manalink.h"

#define ALLOW_REEQUIP 1

#ifndef ALLOW_REEQUIP
static int is_shroud_equipment(int player, int card){
	int id = get_id(player, card);
	if( id == CARD_ID_LIGHTNING_GREAVES ||
		id == CARD_ID_WHISPERSILK_CLOAK ||
		id == CARD_ID_CLOAK_AND_DAGGER ||
		id == CARD_ID_GENERALS_KABUTO ||
		id == CARD_ID_NEUROK_STEALTHSUIT
	  ){
		return 1;
	}
	return 0;
}
#endif

void unattach(int equipment_controller, int equipment_card)
{
  card_instance_t* instance = get_card_instance(equipment_controller, equipment_card), *inst;
  if (instance->targets[8].player >= 0 && instance->targets[8].player >= 0
	  && (inst = in_play(instance->targets[8].player, instance->targets[8].card)))
	{
	  inst->regen_status |= KEYWORD_RECALC_ALL;
	  int id = get_id(equipment_controller, equipment_card);
	  if ((id == CARD_ID_GRAFTED_WARGEAR || id == CARD_ID_GRAFTED_EXOSKELETON || id == CARD_ID_STITCHERS_GRAFT)
		  && equipment_controller == instance->targets[8].player)
		kill_card(instance->targets[8].player, instance->targets[8].card, KILL_SACRIFICE);
	  if (id == CARD_ID_NIM_DEATHMANTLE)
		reset_subtypes(instance->targets[8].player, instance->targets[8].card, 1);
	}

  instance->damage_target_player = instance->targets[8].player = -1;
  instance->damage_target_card = instance->targets[8].card = -1;
}

static int equipment_legacy(int player, int card, event_t event)
{
  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  // Equipped creature
	  int crp = instance->damage_target_player;
	  int crc = instance->damage_target_card;
	  // Equipment
	  int eqp = instance->damage_source_player;
	  int eqc = instance->damage_source_card;

	  card_instance_t* equipment;
	  if (crp < 0 || crc < 0 || eqp < 0 || eqc < 0	// Equipment or equipped creature storage isn't assigned
		  || !(equipment = in_play(eqp, eqc)))		// Equipment not in play
		{
		  // Just remove the legacy.
		  kill_card(player, card, KILL_REMOVE);
		}
	  else if (!in_play(crp, crc))	/* Equipped creature not in play (despite this effect is attached to it and still in play; but we could get here if the
									 * creature moves to the stack or a hand by fiddling with its bits rather than doing it properly with bounce_permanent()) */
		{
		  // Remove the legacy; and unattach the equipment only if equipment thinks it's still attached to this creature too
		  kill_card(player, card, KILL_REMOVE);
		  if (equipment->targets[8].player == crp && equipment->targets[8].card == crc)
			unattach(eqp, eqc);
		}
	  else if (equipment->targets[8].player != crp || equipment->targets[8].card != crc)	// Equipment no longer attached to this creature
		{
		  // Just remove the legacy, don't unattach from whatever it's attached to now.
		  kill_card(player, card, KILL_REMOVE);
		}
	  else if (!is_what(crp, crc, TYPE_CREATURE)	// Attached object no longer a creature
			   || is_what(eqp, eqc, TYPE_CREATURE)	// Equipment now a creature
			   || is_protected_from(crp, crc, eqp, eqc, IPF_ATTACH_EQUIPMENT))	// Creature has protection from the equipment
		{
		  // Remove the legacy and unattach the equipment.
		  kill_card(player, card, KILL_REMOVE);
		  unattach(eqp, eqc);
		}
	}

  return 0;
}

/* Called from the exe to determine whether a non-effect card_instance_t with damage_target_player and damage_target_card set should be visually attached to its
 * damage_target.
 *
 * Like with ability icons, it's important that any functions called from within this always work only with displayed_card_instances, and never with anything
 * returned from get_card_instance(), or it'll randomly crash.
 *
 * Being stdcall saves three bytes at the call site; a normal (cdecl) function would be one byte too long to fit into the third injection. */
int __stdcall is_displayed_as_attached(int player, int card)
{
	card_instance_t* instance = get_displayed_card_instance(player, card);
	card_data_t* cd = &cards_data[instance->internal_card_id];
	card_ptr_t* cp = cards_ptr[cd->id];

	// Auras
	if (cd->type & TYPE_ENCHANTMENT // enchantment
		&& cp->subtype1 != HARDCODED_SUBTYPE_NONE
		// Everything above here is standard.  New stuff below.
		&& cp->subtype1 != HARDCODED_SUBTYPE_WORLD){	// World enchantments aren't auras
		return 1;
	}

	// Equipment
	if (get_setting(SETTING_ATTACH_EQUIPMENT)
		&& has_subtype_by_id(cd->id, SUBTYPE_EQUIPMENT)	// simplifying, rather than rewrite has_subtype() to work with displayed card instances
		&& instance->targets[8].player >= 0 && instance->targets[8].card >= 0	// first half of is_equipping() for displayed card_instance_t
		&& (get_displayed_card_instance(instance->targets[8].player, instance->targets[8].card)->state & (STATE_INVISIBLE|STATE_IN_PLAY)) == STATE_IN_PLAY){ // almost equivalent to in_play(), which also checks STATE_OUBLIETTED
		return 1;
	}

	return 0;
}

void equip_target_creature(int equipment_controller, int equipment_card, int t_player, int t_card){
	unattach(equipment_controller, equipment_card);
	card_instance_t *instance = get_card_instance(equipment_controller, equipment_card);
	instance->damage_target_player = instance->targets[8].player = t_player;
	instance->damage_target_card = instance->targets[8].card = t_card;
	int leg = create_targetted_legacy_effect(equipment_controller, equipment_card, &equipment_legacy, t_player, t_card);
	if (get_setting(SETTING_ATTACH_EQUIPMENT)){
		get_card_instance(equipment_controller, leg)->token_status |= STATUS_INVISIBLE_FX;
	}
	if( get_id(equipment_controller, equipment_card) == CARD_ID_NIM_DEATHMANTLE ){
		force_a_subtype(t_player, t_card, SUBTYPE_ZOMBIE);
	}
	if( get_id(equipment_controller, equipment_card) == CARD_ID_ASSAULT_SUIT ){
		set_special_flags2(t_player, t_card, SF2_CANNOT_BE_SACRIFICED);
	}
	play_sound_effect(WAV_EQUIP);
}


card_instance_t* is_equipping(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[8].player >= 0 && instance->targets[8].card >= 0 ){
		if( in_play(instance->targets[8].player, instance->targets[8].card) ){
			return instance;
		} else if (get_card_instance(instance->targets[8].player, instance->targets[8].card)->internal_card_id == -1){
			instance->targets[8].player = instance->targets[8].card = -1;
		}
	}
	return NULL;
}

#ifndef ALLOW_REEQUIP
static const char* already_equipping(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_equipping(targeting_player, targeting_card))
	{
	  card_instance_t* instance = get_card_instance(targeting_player, targeting_card);
	  if (instance->targets[8].card == card && instance->targets[8].player == player)
		return "already equipping";
	}
  return NULL;
}
#endif
int check_for_equipment_targets(int player, int card)
{
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;

  int csvid = get_id(player, card);

  if (csvid == CARD_ID_O_NAGINATA)
	td.power_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;

  if (csvid == CARD_ID_GATE_SMASHER)
	td.toughness_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

  if (csvid == CARD_ID_KONDAS_BANNER)
	td.required_subtype = SUBTYPE_LEGEND;

#ifndef ALLOW_REEQUIP
  if (is_equipping(player, card))
	{
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)already_equipping;
	}
#endif

  return can_target(&td);
}

int get_updated_equip_cost(int player, int card, int equip_cost){
	int count = 0;
	int art_count = 0;
	int psp_flag = 0;
	int new_cost = get_cost_mod_for_activated_abilities(player, card, equip_cost, 0, 0, 0, 0, 0);
	while( count < active_cards_count[player] ){
			if( in_play(player, count) ){
				if( is_what(player, count, TYPE_ARTIFACT) ){
					art_count++;
				}
				if( get_id(player, count) == CARD_ID_AURIOK_STEELSHAPER ){
					new_cost--;
				}
				if( get_id(player, count) == CARD_ID_PURESTEEL_PALADIN ){
					psp_flag = 1;
				}
			}
			if( new_cost < 1 ){
				break;
			}
			if( psp_flag && art_count > 2 ){
				new_cost = 0;
				break;
			}
			count++;
	}
	return new_cost;
}

int can_activate_basic_equipment(int player, int card, event_t event, int equip_cost){
	if( can_sorcery_be_played(player, event) && ! is_what(player, card, TYPE_CREATURE) ){
		int new_cost = get_updated_equip_cost(player, card, equip_cost);
		if( has_mana( player, COLOR_COLORLESS, new_cost) ){
			return check_for_equipment_targets(player, card);
		}
	}
	return 0;
}

int activate_basic_equipment(int player, int card, int equip_cost){
	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	int csvid = get_id(player, card);

	if( csvid == CARD_ID_O_NAGINATA ){
		td.power_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;
	} else if( csvid == CARD_ID_GATE_SMASHER ){
		td.toughness_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;
	}
	td.allow_cancel = 0;

	instance->number_of_targets = 0;
	int rval = 0;

	if (equip_cost >= -1){
		int new_cost = get_updated_equip_cost(player, card, equip_cost);
		charge_mana( player, COLOR_COLORLESS, new_cost);
	}
	if( spell_fizzled != 1){
#ifndef ALLOW_REEQUIP
		if( is_equipping(player, card) ){
			state_untargettable(instance->targets[8].player, instance->targets[8].card, 1);
		}
#endif
		if( can_target(&td) && select_target(player, card, &td, "Select creature to equip", NULL) ){
#ifdef ALLOW_REEQUIP
			if (is_equipping(player, card)
				&& instance->targets[0].player == instance->targets[8].player
				&& instance->targets[0].card == instance->targets[8].card){
				ai_modifier -= 96;
			}
#endif
			rval = 1;
		}
#ifndef ALLOW_REEQUIP
		if( is_equipping(player, card) ){
			state_untargettable(instance->targets[8].player, instance->targets[8].card, 0);
		}
#endif
	}

	return rval;
}

int resolve_activation_basic_equipment(int player, int card){
	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	int csvid = get_id(player, card);
	if( csvid == CARD_ID_O_NAGINATA ){
		td.power_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;
	} else if( csvid == CARD_ID_GATE_SMASHER ){
		td.toughness_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;
	}

	if( validate_target(player, card, &td, 0) ){
		equip_target_creature(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		return 1;
	} else {
		return 0;
	}
}

int basic_equipment(int player, int card, event_t event, int equip_cost){
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_activate_basic_equipment(player, card, event, equip_cost);
	}
	else if( event == EVENT_ACTIVATE ){
		activate_basic_equipment(player, card, equip_cost);
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		resolve_activation_basic_equipment(player, card);
	}
	return 0;
}

int living_weapon(int player, int card, event_t event, int equip_cost){

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GERM, &token);
		token.action = TOKEN_ACTION_EQUIP;
		generate_token(&token);
	}

	return basic_equipment(player, card, event, equip_cost);
}

int vanilla_equipment(int player, int card, event_t event, int equip_cost, int p_plus, int t_plus, int k_plus, int k_special ){

	card_instance_t* instance;

	if( (instance = in_play(player, card)) && is_equipping(player, card) ){
		if (p_plus || t_plus || k_plus){
			modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, p_plus, t_plus, k_plus);
		}

		if( k_special > 0 ){
			special_abilities(instance->targets[8].player, instance->targets[8].card, event, k_special, player, card);
		}
	}
	return basic_equipment(player, card, event, equip_cost);
}

int equipments_attached_to_me(int player, int card, int mode){
	int result = 0;
	int i;
	for(i=0; i<2; i++){
		int count = active_cards_count[i]-1;
		while( count > -1 ){
				if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) &&
					has_subtype(i, count, SUBTYPE_EQUIPMENT)
				  ){
					card_instance_t *instance = get_card_instance( i, count );
					if( instance->targets[8].player == player && instance->targets[8].card == card ){
						if( mode == EATM_CHECK ){
							return 1;
						}
						if( mode & EATM_REPORT_TOTAL ){
							result++;
						}
						if( mode & EATM_UNATTACH ){
							unattach(i, count);
						}
						if( mode & EATM_DESTROY ){
							kill_card(i, count, KILL_DESTROY);
						}
					}
				}
				count--;
		}
	}
	return result;
}

int is_equipment(int player, int card ){
	return has_subtype(player, card, SUBTYPE_EQUIPMENT );
}

int altar_equipment(int player, int card, event_t event, int req_type, int p_plus, int t_plus, int k_plus){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, p_plus, t_plus, k_plus);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){

		if( can_sorcery_be_played(player, event) ){
			if( can_sacrifice_as_cost(player, 1, req_type, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return check_for_equipment_targets(player, card);
			}
			else{
				 if( metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN) ){
					return check_for_equipment_targets(player, card);
				}
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( can_target(&td) ){
				if( metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN) ){
					choice = do_dialog(player, player, card, -1, -1," Sac & equip\n Equip for 0\n Do nothing", 1);
				 }
			}
			else{
				 choice = 1;
			}

			instance->number_of_targets = 0;
			if( choice == 0 ){
				if( controller_sacrifices_a_permanent(player, card, req_type, 0) ){
#ifndef ALLOW_REEQUIP
					if( is_equipping(player, card) ){
						state_untargettable(instance->targets[8].player, instance->targets[8].card, 1);
					}
#endif
					if( can_target(&td) && select_target(player, card, &td, "Select creature to equip", NULL) ){
#ifdef ALLOW_REEQUIP
						if (is_equipping(player, card)
							&& instance->targets[0].player == instance->targets[8].player
							&& instance->targets[0].card == instance->targets[8].card){
							ai_modifier -= 96;
						}
#endif
					}
#ifndef ALLOW_REEQUIP
					if( is_equipping(player, card) ){
						state_untargettable(instance->targets[8].player, instance->targets[8].card, 0);
					}
#endif
				}
				else{
					spell_fizzled = 1;
				}
			}
			else if( choice == 1 ){
#ifndef ALLOW_REEQUIP
					if( is_equipping(player, card) ){
						state_untargettable(instance->targets[8].player, instance->targets[8].card, 1);
					}
#endif
					if( can_target(&td) && select_target(player, card, &td, "Select creature to equip", NULL) ){
#ifdef ALLOW_REEQUIP
						if (is_equipping(player, card)
							&& instance->targets[0].player == instance->targets[8].player
							&& instance->targets[0].card == instance->targets[8].card){
							ai_modifier -= 96;
						}
#endif
					}
#ifndef ALLOW_REEQUIP
					if( is_equipping(player, card) ){
						state_untargettable(instance->targets[8].player, instance->targets[8].card, 0);
					}
#endif
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td, 0) ){
				equip_target_creature(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
	}

	return 0;
}

int is_equipped(int player, int card){
	return equipments_attached_to_me(player, card, EATM_CHECK);
}


