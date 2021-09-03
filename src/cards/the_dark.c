#include "manalink.h"

// Functions
void half_damage_to_target_half_to_player(int damaging_player, int damaging_card, int amount, int target_location){
	int dmg_to_target = amount/2;
	int dmg_to_player = amount/2;
	if( amount-dmg_to_target != dmg_to_target ){
		dmg_to_target = (amount-1)/2;
		dmg_to_player = (amount+1)/2;
	}
	card_instance_t *instance = get_card_instance(damaging_player, damaging_card);
	damage_creature(instance->targets[target_location].player, instance->targets[target_location].card, dmg_to_target, damaging_player, damaging_card);
	damage_player(damaging_player, dmg_to_player, damaging_player, damaging_card);
}

int land_into_new_basic_land_type(int internal_card_id_of_land_to_change, int basic_land_type_to_become){
	// returns the 'internal card id' value to use in EVENT_CHANGE_TYPE;
	int newtype = -1;
	int new_land_iid = 0;  // Hardcoded internal card id for CARD_ID_SWAMP
	if( basic_land_type_to_become == SUBTYPE_ISLAND ){
		new_land_iid = 1;  // Hardcoded internal card id for CARD_ID_ISLAND;
	}
	if( basic_land_type_to_become == SUBTYPE_FOREST ){
		new_land_iid = 2;  // Hardcoded internal card id for CARD_ID_FOREST;
	}
	if( basic_land_type_to_become == SUBTYPE_MOUNTAIN ){
		new_land_iid = 3;  // Hardcoded internal card id for CARD_ID_MOUNTAIN;
	}
	if( basic_land_type_to_become == SUBTYPE_PLAINS ){
		new_land_iid = 4;  // Hardcoded internal card id for CARD_ID_PLAINS;
	}
	if( cards_data[internal_card_id_of_land_to_change].type == TYPE_LAND ){
		newtype = new_land_iid;
	}
	else{
		newtype = create_a_card_type(new_land_iid);
		if (newtype != -1){
			cards_data[newtype].type |= cards_data[internal_card_id_of_land_to_change].type; // For artifact lands
			cards_data[newtype].power = cards_data[internal_card_id_of_land_to_change].power; // For Dryad Arbor
			cards_data[newtype].toughness = cards_data[internal_card_id_of_land_to_change].toughness; // For Dryad Arbor
		}
	}
	return newtype;
}

// Cards
int card_amnesia(int player, int card, event_t event){
	/*
	  Amnesia |3|U|U|U
	  Sorcery
	  Target player reveals his or her hand and discards all nonland cards.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		reveal_target_player_hand(instance->targets[0].player);

		test_definition_t test;
		default_test_definition(&test, TYPE_LAND);
		test.type_flag = DOESNT_MATCH;

		ec_definition_t ec;
		default_ec_definition(instance->targets[0].player, player, &ec);
		ec.effect = EC_DISCARD | EC_ALL_WHICH_MATCH_CRITERIA;

		new_effect_coercion(&ec, &test);

		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_angry_mob(int player, int card, event_t event){
	/*
	  Angry Mob English |2|W|W
	  Creature - Human 2+x/2+x
	  Trample
	  As long as it's your turn, Angry Mob's power and toughness are each equal to 2 plus the number of Swamps your opponents control.
	  As long as it's not your turn, Angry Mob's power and toughness are each 2.
	*/
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		if( current_turn == player && ! is_humiliated(player, card) ){
			event_result += count_subtype(1-player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
		}
	}

	return 0;
}

int card_apprentice_wizard(int player, int card, event_t event){
	// 0x4cad10
	if (event == EVENT_CAN_ACTIVATE){
		if (!is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card) && has_mana(player, COLOR_BLUE, 1)){
			return 1;
		}
	} else if (event == EVENT_ACTIVATE){
		charge_mana(player, COLOR_BLUE, 1);
		if (cancel != 1){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
		}
	} else if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 36;
	} else if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 36;
	}
	return 0;
}

int card_ashes_ot_ashes(int player, int card, event_t event){
	/*
	  Ashes to Ashes |1|B|B
	  Sorcery
	  Exile two target nonartifact creatures. Ashes to Ashes deals 5 damage to you.*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int good = 0;
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
				good++;
			}
		}
		if( good ){
			damage_player(player, 5, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonartifact creature.", 2, NULL);
}

int card_ball_lightning(int player, int card, event_t event){
	/*
	  Ball Lightning |R|R|R
	  Creature - Elemental 6/1
	  Trample
	  Haste (This creature can attack and {T} as soon as it comes under your control.)
	  At the beginning of the end step, sacrifice Ball Lightning.
	*/
	if( ! is_humiliated(player, card) ){
		haste(player, card, event);
		if( ! check_state(player, card, STATE_OUBLIETTED) && eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_banshee2(int player, int card, event_t event){
	/*
	  Banshee English |2|B|B
	  Creature - Spirit 0/1
	  {X}, {T}: Banshee deals half X damage, rounded down, to target creature or player, and half X damage, rounded up, to you.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		half_damage_to_target_half_to_player(instance->parent_controller, instance->parent_card, instance->info_slot, 0);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(-1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_barls_cage(int player, int card, event_t event)
{

	// 0x428330
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td_creature;
	default_target_definition(player, card, &td_creature, TYPE_CREATURE);

	target_definition_t td_tapped_creature;
	default_target_definition(player, card, &td_tapped_creature, TYPE_CREATURE);
	td_tapped_creature.required_state = TARGET_STATE_TAPPED;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_creature)){
		card_instance_t* instance = get_card_instance(player, card);
		does_not_untap_effect(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 1);
	}

	target_definition_t* td = (player == AI || ai_is_speculating == 1) ? &td_tapped_creature : &td_creature;	// specifically not during resolution
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(3), 0, td, "TARGET_CREATURE");
}

void change_lands_into_new_land_type(int player, int card, event_t event, int orig_basic_land_type, int orig_basic_land_type_flag, int basic_land_type_to_become){
#pragma message"Works fine except for animated lands : if a card that uses this function comes into play after an animated land is animated, it will remain animated forever"

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CHANGE_TYPE && is_what(affected_card_controller, affected_card, TYPE_LAND) && in_play(player, card)){
		if (!(orig_basic_land_type_flag & TARGET_TYPE_HACK_SLEIGHT_LEGACY)){
			orig_basic_land_type = get_hacked_subtype(player, card, orig_basic_land_type);
		}

		int matches = orig_basic_land_type_flag & (MATCH|DOESNT_MATCH);
		if( matches == MATCH && ! has_subtype(affected_card_controller, affected_card, orig_basic_land_type) ){
			return;
		}
		if( matches == DOESNT_MATCH && has_subtype(affected_card_controller, affected_card, orig_basic_land_type) ){
			return;
		}

		if( (orig_basic_land_type_flag & STATUS_BASICLAND_DEPENDANT) && !is_basic_land(affected_card_controller, affected_card) ){
			return;
		}

		if (is_humiliated(player, card)){
			return;
		}

		if (!(orig_basic_land_type_flag & TARGET_TYPE_HACK_SLEIGHT_LEGACY)){
			basic_land_type_to_become = get_hacked_subtype(player, card, basic_land_type_to_become);
		}

		if( cards_data[event_result].type == TYPE_LAND ){
			event_result = land_into_new_basic_land_type(event_result, basic_land_type_to_become);
		}
		else{
			int newtype = -1;
			int k;
			for(k=0; k<16; k++){
				if( instance->targets[k].player == -1 ){
					break;
				}
				else{
					int svalue = instance->targets[k].player;
					if( svalue & (1<<31) ){
						svalue &= ~(1<<31);
					}
					if( svalue == event_result ){
						newtype = instance->targets[k].card;
						break;
					}
				}
			}
			if( newtype == -1 && k < 16 ){
				newtype = land_into_new_basic_land_type(event_result, basic_land_type_to_become);
				if (newtype != -1){
					instance->targets[k].player = event_result;
					if( check_status(affected_card_controller, affected_card, STATUS_ANIMATED) ){
						instance->targets[k].player |= (1<<31);
					}
					instance->targets[k].card = newtype;
				}
			}
			if( newtype != -1 ){
				event_result = newtype;
			}
		}
	}
	if( event == EVENT_CLEANUP ){
		int k;
		for(k=0; k<16; k++){
			if( instance->targets[k].player > -1 ){
				if( instance->targets[k].player & (1<<31) ){
					int q;
					for(q=k; q<16; q++){
						instance->targets[q] = instance->targets[q+1];
					}
				}
			}
		}
	}
}

int card_blood_moon(int player, int card, event_t event){
	// 0x42BFF0

	change_lands_into_new_land_type(player, card, event, SUBTYPE_BASIC, DOESNT_MATCH, SUBTYPE_MOUNTAIN);

	return global_enchantment(player, card, event);
}

int blood_of_the_martyr_legacy(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *instance = get_card_instance(affected_card_controller, affected_card);
		if( instance->internal_card_id == damage_card && instance->info_slot > 0 ){
			if( instance->damage_target_card != -1 ){
				int good = 1;
				if( duh_mode(player) && instance->damage_target_player != player ){
					good = 0;
				}
				if( good ){
					char buffer[100];
					scnprintf(buffer, 100, " Protect this (%d damages)\n Pass", instance->info_slot);
					int ai_choice = (life[player] - instance->info_slot) < 6 ? 1 : 0;
					int choice = do_dialog(player, instance->damage_target_player, instance->damage_target_card, instance->damage_target_player, instance->damage_target_card,
										   buffer, ai_choice);
					if( choice == 0 ){
						instance->damage_target_player = player;
						instance->damage_target_card = -1;
					}
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}


int card_blood_of_the_martyr(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &blood_of_the_martyr_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*
  Bog Imp English |1|B --> vanilla
  Creature - Imp 1/1
  Flying (This creature can't be blocked except by creatures with flying or reach.)
*/

int card_bog_rats(int player, int card, event_t event)
{
  // 0x429040
  if (event == EVENT_BLOCK_LEGALITY
	  && attacking_card_controller == player && attacking_card == card
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL)
	  && !is_humiliated(player, card))
	event_result = 1;

  return 0;
}

int card_bone_flute(int player, int card, event_t event){
	/*
	  Bone Flute |3
	  Artifact
	  {2}, {T}: All creatures get -1/-0 until end of turn.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, ANYBODY, -1, -1, 0, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);

}

int card_book_of_rass(int player, int card, event_t event){
	/*
	  Book of Rass |6
	  Artifact
	  {2}, Pay 2 life: Draw a card.
	*/

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 2, NULL, NULL);

}

int card_brainwash(int player, int card, event_t event){
	/*
	  Brainwash |W
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature can't attack unless its controller pays {3}.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_ATTACK_LEGALITY && affect_me(p, c) ){
			int no_attack = 1;
			if( has_mana(p, COLOR_COLORLESS, 3) ){
				int choice = do_dialog(p, p, c, -1, -1, " Pay 3 to attack with this\n Pass", 0);
				if( choice == 0 ){
					charge_mana(p, COLOR_COLORLESS, 3);
					if( spell_fizzled != 1 ){
						no_attack = 0;
					}
				}
			}
			if( no_attack ){
				event_result = 1;
			}
		}
	}

	return disabling_aura(player, card, event);
}

int card_brothers_of_fire(int player, int card, event_t event){
	/*
	  Brothers of Fire |1|R|R
	  Creature - Human Shaman 2/2
	  {1}{R}{R}: Brothers of Fire deals 1 damage to target creature or player and 1 damage to you.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		damage_target0(player, card, 1);
		damage_player(instance->parent_controller, 1, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1, 2), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

/*
Carnivorous Plant |3|G --> vanilla
Creature - Plant Wall 4/5
Defender
*/

int card_cave_people(int player, int card, event_t event){
	/*
	  Cave People |1|R|R
	  Creature - Human 1/4
	  Whenever Cave People attacks, it gets +1/-2 until end of turn.
	  {1}{R}{R}, {T}: Target creature gains mountainwalk until end of turn.
	*/
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		pump_until_eot(player, card, player, card, 1, -2);
	}


	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, get_hacked_walk(instance->parent_controller, instance->parent_card, KEYWORD_MOUNTAINWALK), 0);
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1, 2), 0, &td, "TARGET_CREATURE");
}

int card_city_of_shadows(int player, int card, event_t event){

	/* City of Shadows	""
	 * Land
	 * |T, Exile a creature you control: Put a storage counter on ~.
	 * |T: Add |X to your mana pool, where X is the number of storage counters on ~. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	if( IS_AI(player) ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if (event == EVENT_CAN_ACTIVATE && CAN_TAP(player, card)){
		if (CAN_ACTIVATE0(player, card) && can_target(&td)){
			if (!IS_AI(player)){
				return 1;
			} else if (land_can_be_played & LCBP_REGENERATION){
				return 99;
			}
		}
		if( can_produce_mana(player, card) && (!IS_AI(player) || count_counters(player, card, COUNTER_STORAGE) > 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;

		int choice = 1;
		if (!paying_mana() && can_target(&td) && CAN_ACTIVATE0(player, card)){
			if (!IS_AI(player) || (land_can_be_played & LCBP_REGENERATION )){
				choice = do_dialog(player, player, card, -1, -1, " Exile creature\n Add mana\n Cancel", 0);
			}
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else if( choice == 0 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					select_target(player, card, &td, "Select target creature you control.", NULL)
				  ){
					if( IS_AI(player) ){
						regenerate_target(instance->targets[0].player, instance->targets[0].card);
					}
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
					tap_card(player, card);
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
				else{
					spell_fizzled = 1;
				}
		}
		else if( choice == 1 ){
			produce_mana_tapped(player, card, COLOR_COLORLESS, count_counters(player, card, COUNTER_STORAGE));
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		add_counter(player, card, COUNTER_STORAGE);
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_STORAGE));
	}

	return 0;
}

int card_cleansing2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_LAND) ){
						int kill = 1;
						if( can_pay_life(i, 1) ){
							if( do_dialog(i, i, count, -1, -1, " Save this land\n Pass", life[player] > 5) == 0 ){
								lose_life(i, 1);
								kill = 0;
							}
						}
						if( kill == 1 ){
							kill_card(i, count, KILL_DESTROY);
						}
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_coal_golem(int player, int card, event_t event){
	/*
	  Coal Golem |5
	  Artifact Creature - Golem 3/3
	  {3}, Sacrifice Coal Golem: Add {R}{R}{R} to your mana pool.
	*/
	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(3), 0, NULL, NULL) ){
			return can_produce_mana(player, card);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			produce_mana(player, COLOR_RED, 3);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_curse_artifact(int player, int card, event_t event){
	/*
	  Curse Artifact English |2|B|B
	  Enchantment - Aura
	  Enchant artifact
	  At the beginning of the upkeep of enchanted artifact's controller, Curse Artifact deals 2 damage to that player unless he or she sacrifices that artifact.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		upkeep_trigger_ability(player, card, event, p);

		if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
			if( DIALOG(p, c, EVENT_ACTIVATE,
						  DLG_FULLCARD(player, card), DLG_SMALLCARD(p, c), DLG_WHO_CHOOSES(p),
						  DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
						  "Take 2 damage", 1, life[player]-2 > 6 ? 2 : 0,
						  "Sacrifice this artifact", 1, 1) == 1
			  ){
				damage_player(p, 2, player, card);
			}
			else
			{
				kill_card(p, c, KILL_SACRIFICE);
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

static int dance_of_many_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p != -1 && instance->damage_source_player != -1 && instance->targets[1].card != 42){
		if (leaves_play(p, c, event)){
			instance->targets[1].card = 42;
			kill_card(instance->damage_source_player, instance->damage_source_card, KILL_SACRIFICE);
		}
		if (leaves_play(instance->damage_source_player, instance->damage_source_card, event)){
			instance->targets[1].card = 42;
			kill_card(p, c, KILL_REMOVE);
		}
	}

	return 0;
}

int card_dance_of_many(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TARGET_TYPE_TOKEN;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	basic_upkeep(player, card, event, 0, 0, 2, 0, 0, 0);

	if( comes_into_play(player, card, event) && can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
		token_generation_t token;
		default_token_definition(player, card, get_id(instance->targets[0].player, instance->targets[0].card), &token);
		token.legacy = 1;
		token.special_code_for_legacy = &dance_of_many_legacy;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_dark_heart_of_the_woods(int player, int card, event_t event){

	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}

	else if( event == EVENT_CAN_CAST ){
			return 1;
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			gain_life(player, 3);
	}
	else{
		return altar_extended(player, card, event, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_dark_sphere(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;
	td.required_type = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_target(&td) && ! is_tapped(player, card) &&
		! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 0x63;
	}
	else if( event == EVENT_ACTIVATE ){
			 if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_DAMAGE") ){
				 card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				 if( target->damage_target_card != -1 ){
					spell_fizzled = 1;
				 }
				 if( player == AI && target->info_slot < 2 ){
					spell_fizzled = 1;
				 }
				 tap_card(player, card);
				 instance->info_slot = (target->info_slot + 1) >> 1;
				 kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			 if( target->info_slot > 0 ){
				 target->info_slot -= instance->info_slot;
			 }
	}
	return 0;
}

int card_deep_water(int player, int card, event_t event){
	// New card
	/*
	  Deep Water English |U|U
	  Enchantment
	  {U}: Until end of turn, if you tap a land you control for mana, it produces {U} instead of any other type.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[2].card == 66 && event == EVENT_CAST_SPELL && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_LAND)
	  ){
		set_special_flags(affected_card_controller, affected_card, SF2_DEEP_WATER);
	}

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return player == AI ? 	generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_U(1), 0, NULL, NULL) :
								generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		get_card_instance(instance->parent_controller, instance->parent_card)->targets[2].card = 66;
		set_special_flags(player, -1, SF2_DEEP_WATER);
	}

	if (event == EVENT_CLEANUP){
		instance->targets[2].player = 0; // Resetting GAA_ONCE_PER_TURN for AI
		instance->targets[2].card = 0;
		remove_special_flags(player, -1, SF2_DEEP_WATER);
	}

	return global_enchantment(player, card, event);
}

int card_diabolic_machine(int player, int card, event_t event){
	/*
	  Diabolic Machine |7
	  Artifact Creature - Construct 4/4
	  {3}: Regenerate Diabolic Machine.
	*/
	return regeneration(player, card, event, MANACOST_X(3));
}

/*
Drowned |1|U --> Drudge Skeletons
Creature - Zombie 1/1
{B}: Regenerate Drowned.
*/

int card_dust_to_dust(int player, int card, event_t event){
	/*
	  Dust to Dust |1|W|W
	  Sorcery
	  Exile two target artifacts.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 2, NULL);
}

int card_eater_of_the_dead(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( new_special_count_grave(2, &this_test) ){
				int rvalue = ! graveyard_has_shroud(2);
				return player == HUMAN ? rvalue : (is_tapped(player, card) ? rvalue : 0);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			instance->targets[0].player = 1-player;
			if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && player != AI ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
					}
				}
			}
			else{
				instance->targets[0].player = player;
			}
			if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) == -1 ){
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( is_tapped(instance->parent_controller, instance->parent_card) ){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
				untap_card(instance->parent_controller, instance->parent_card);
			}
		}
	}
	return 0;
}

int card_electric_eel(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( comes_into_play(player, card, event) > 0 ){
		damage_player(player, 1, player, card);
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, card, player, instance->parent_card, 2, 0);
		damage_player(player, 1, player, instance->parent_card);
	}
	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0);
}

int card_elves_of_the_deep_shadow(int player, int card, event_t event){
	/*
	  Elves of Deep Shadow |G
	  Creature - Elf Druid 1/1
	  {T}: Add {B} to your mana pool. Elves of Deep Shadow deals 1 damage to you.
	*/
	if( event == EVENT_CAN_ACTIVATE || event == EVENT_COUNT_MANA ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		damage_player(player, 1, player, card);
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_erosion(int player, int card, event_t event){
	/*
	  Erosion English |U|U|U
	  Enchantment - Aura
	  Enchant land
	  At the beginning of the upkeep of enchanted land's controller, destroy that land unless that player pays {1} or 1 life.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
			int sac = 1;
			if( has_mana(player, COLOR_COLORLESS, 1) || can_pay_life(player, 1) ){
				int choice = DIALOG(player, card, EVENT_ACTIVATE,
									DLG_FULLCARD(instance->damage_target_player, instance->damage_target_card),
									DLG_WHO_CHOOSES(instance->damage_target_player),
									DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
									"Pay 1", has_mana(player, COLOR_COLORLESS, 1), 10,
									"Pay 1 life", can_pay_life(player, 1), 5,
									"Decline", 1, 1);
				if( choice == 1 && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, instance->damage_target_player, MANACOST_X(1))){
					sac = 0;
				}
				if( choice == 2 ){
					lose_life(instance->damage_target_player, 1);
					sac = 0;
				}
			}
			if( sac ){
				kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY);
			}
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_eternal_flame(int player, int card, event_t event){
	/*
	  Eternal Flame English |2|R|R
	  Sorcery
	  Eternal Flame deals X damage to target opponent, where X is the number of Mountains you control. It deals half X damage, rounded up, to you.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		int amount = count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN);
		damage_target0(player, card, amount);
		damage_player(player, (amount+1)/2, player, card);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_exorcist(int player, int card, event_t event){
	/*
	  Exorcist |W|W
	  Creature - Human Cleric 1/1
	  {1}{W}, {T}: Destroy target black creature.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_fasting(int player, int card, event_t event){

	/* Fasting	|W
	 * Enchantment
	 * At the beginning of your upkeep, put a hunger counter on ~. Then destroy ~ if it has five or more hunger counters on it.
	 * If you would begin your draw step, you may skip that step instead. If you do, you gain 2 life.
	 * When you draw a card, destroy ~. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( life[player] > 6 ){
			ai_modifier-=500;
		}
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_HUNGER);
		if( count_counters(player, card, COUNTER_HUNGER) >= 5 ){
			kill_card(player, card, KILL_DESTROY);
		}
	}

	if( event == EVENT_DRAW_PHASE && current_turn == player ){
		int choice = do_dialog(player, player, card, -1, -1, " Skip draw step (gain 2 life)\n Don't skip draw step", 0);
		if( choice == 0 ){
			event_result -= 99;
			gain_life(player, 2);
		}
	}

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		kill_card(player, card, KILL_DESTROY);
	}
	return global_enchantment(player, card, event);
}

int card_fellwar_stone(int player, int card, event_t event){
	// original code : 0x422360.  Also Exotic Orchard / Sylvok Explorer

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
		instance->mana_color = instance->card_color = get_color_of_mana_produced_by_id(get_id(player, card), COLOR_TEST_ANY_COLORED, player);
	}

	if (instance->mana_color == 0 || instance->mana_color == COLOR_TEST_COLORLESS ){	// Can't produce mana, but can still tap for 0
		if (event == EVENT_CAN_ACTIVATE && player == AI){
			return 0;
		}
		if (event == EVENT_ACTIVATE){
			if (can_produce_mana(player, card)){
				produce_mana_tapped(player, card, COLOR_COLORLESS, 0);
				return 0;
			}
		}
		if (event == EVENT_COUNT_MANA){
			return 0;
		}
	}

	return mana_producer(player, card, event);
}

int card_festival(int player, int card, event_t event){
	// Cast ~ only during an opponent's upkeep.
	if( event == EVENT_CAN_CAST ){
		if( current_turn == 1-player && current_phase == PHASE_UPKEEP ){
			return 1;
		}
	}
	// Creatures can't attack this turn.
	else if( event == EVENT_RESOLVE_SPELL ){
		nobody_can_attack_until_eot(player, card, ANYBODY, -1, -1);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_fire_and_brimstone(int player, int card, event_t event){
	/*
	  Fire and Brimstone English |3|W|W
	  Instant
	  Fire and Brimstone deals 4 damage to target player who declared an attacking creature this turn and 4 damage to you.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
    td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( current_phase > PHASE_DECLARE_ATTACKERS ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_ATTACKING | STATE_ATTACKED;
			if( check_battlefield_for_special_card(player, card, current_turn, 0, &this_test) ){
				return would_validate_arbitrary_target(&td, current_turn, -1);
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = current_turn;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 4, player, card);
			damage_player(player, 4, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fire_drake(int player, int card, event_t event){
	/*
	  Fire Drake English |1|R|R
	  Creature - Drake 1/2
	  Flying
	  {R}: Fire Drake gets +1/+0 until end of turn. Activate this ability only once each turn.
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->parent_controller, c = instance->parent_card;
		pump_until_eot(p, c, p, c, 1, 0);
	}

	if(event == EVENT_POW_BOOST ){
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST_R(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_R(1), 0, NULL, NULL);
}

int card_fissure(int player, int card, event_t event){
	/*
	  Fissure |3|R|R
	  Instant
	  Destroy target creature or land. It can't be regenerated.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature or land.", 1, NULL);
}

int card_flood2(int player, int card, event_t event){
	/*
	  Flood English |U
	  Enchantment
	  {U}{U}: Tap target creature without flying
	*/

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_U(2), 0, &td, "TARGET_CREATURE");
}

int card_fountain_of_youth(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_frankensteins_monster(int player, int card, event_t event){

	/* Frankenstein's Monster	|X|B|B
	 * Creature - Zombie 0/1
	 * As ~ enters the battlefield, exile X creature cards from your graveyard. If you can't, put ~ into its owner's graveyard instead of onto the
	 * battlefield. For each creature card exiled this way, ~ enters the battlefield with a +2/+0, +1/+1, or +0/+2 counter on it. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode\n Cancel", 0);
			}
			if( choice == 0 ){
				int max = count_graveyard_by_type(player, TYPE_CREATURE);
				while( ! has_mana(player, COLOR_COLORLESS, max) ){
						max--;
				}
				charge_mana(player, COLOR_COLORLESS, max);
				if( spell_fizzled != 1 ){
					instance->info_slot = max;
				}
			}
			else if( choice == 1 ){
					charge_mana(player, COLOR_COLORLESS, -1);
					if( spell_fizzled != 1 ){
						instance->info_slot = x_value;
					}
			}
			else{
				spell_fizzled = 1;
			}
		}
		else{
			if( player == AI ){
				spell_fizzled = 1;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

		int i;
		for (i = 0; i < instance->info_slot; ++i){
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
				int choice = 0;
				if (IS_AI(player)){
					if (i == 0){
						choice = 1;
					} else {
						choice = recorded_rand(player, 3);
					}
				} else {
					choice = do_dialog(player, player, card, -1, -1, "+2/+0\n +1/+1\n +0/+2", 1);
				}
				if( choice == 0 ){
					add_counter(player, card, COUNTER_P2_P0);
				} else if (choice == 1){
					add_counter(player, card, COUNTER_P1_P1);
				} else {
					add_counter(player, card, COUNTER_P0_P2);
				}
			}
		}
	}
	return 0;
}

int card_gaeas_touch(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a basic Forest card.");
	this_test.id = CARD_ID_FOREST;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && check_battlefield_for_special_card(player, card, player, 0, &this_test) &&
			can_sorcery_be_played(player, event) && instance->targets[1].player != 66 && can_use_activated_abilities(player, card)
		  ){
			return 1;
		}
		if( can_produce_mana(player, card) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && check_battlefield_for_special_card(player, card, player, 0, &this_test) &&
			can_sorcery_be_played(player, event) && instance->targets[1].player != 66 && can_use_activated_abilities(player, card)
		  ){
			if( can_produce_mana(player, card) && can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Play a basic Forest\n Sacrifice for mana\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				instance->info_slot = 1;
				instance->targets[1].player = 66;
			}
		}
		else if( choice == 1){
				produce_mana(player, COLOR_GREEN, 2);
				kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->info_slot = 0;
		}
	}

	if( event == EVENT_COUNT_MANA ){
		if( affect_me(player, card) && can_produce_mana(player, card) && can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, COLOR_GREEN, 2);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_ghost_ship(int player, int card, event_t event){
	/*
	  Ghost Ship |2|U|U
	  Creature - Spirit 2/4
	  Flying
	  {U}{U}{U}: Regenerate Ghost Ship.
	*/
	return regeneration(player, card, event, MANACOST_U(3));
}


int card_giant_shark(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){

		islandhome(player, card, event);

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_DECLARE_BLOCKERS ){
			if( instance->blocking < 255 ){
				int leader = instance->blocking;
				card_instance_t *leader_instance = get_card_instance(1-player, leader);
				if( leader_instance->damage_on_card > 0 ){
					pump_ability_until_eot(player, card, player, card, 2, 0, KEYWORD_TRAMPLE, 0);
				}
				int i;
				for( i=0; i<active_cards_count[1-player]; i++ ){
					card_instance_t *oba_instance = get_card_instance(1-player, i);
					if( i != leader && oba_instance->state & STATE_ATTACKING && oba_instance->blocking == leader && oba_instance->damage_on_card > 0 ){
						pump_ability_until_eot(player, card, player, card, 2, 0, KEYWORD_TRAMPLE, 0);
					}
				}
			}
			else if( instance->state & STATE_ATTACKING && !is_unblocked(player, card) ){
				int j;
				for( j=0; j<active_cards_count[1-player]; j++ ){
					card_instance_t *blocker_instance = get_card_instance(1-player, j);
					if( blocker_instance->blocking == card && blocker_instance->damage_on_card > 0 ){
						pump_ability_until_eot(player, card, player, card, 2, 0, KEYWORD_TRAMPLE, 0);
					}
				}
			}
		}
	}
	return 0;
}

int card_goblin_caves(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	if( player == AI ){
		td.extra = CARD_ID_MOUNTAIN;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( get_id(instance->damage_target_player, instance->damage_target_card) == CARD_ID_MOUNTAIN ){
			boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 0, 2, 0, BCT_INCLUDE_SELF);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_goblin_digging_team(int player, int card, event_t event)
{
	/*
	  Goblin Digging Team |R
	  Creature - Goblin 1/1
	  {T}, Sacrifice Goblin Digging Team: Destroy target Wall.
	*/

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_WALL");
}

/*
Goblin Hero English |2|R --> vanilla
Creature - Goblin 2/2, 2R (3)
*/

int card_goblin_rock_sled(int player, int card, event_t event){
  // 0x4C90F0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card))
	if (!basiclandtypes_controlled[1-player][get_hacked_color(player, card, COLOR_RED)])
	  event_result = 1;

  if (event == EVENT_CLEANUP && affect_me(player, card))
	if (player == current_turn)
	  instance->info_slot = instance->state & STATE_ATTACKED;

  if (instance->info_slot)
	does_not_untap(player, card, event);

  return 0;
}

int card_goblin_shrine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	if( player == AI ){
		td.extra = CARD_ID_MOUNTAIN;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( get_id(instance->damage_target_player, instance->damage_target_card) == CARD_ID_MOUNTAIN ){
			boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 0, 0, BCT_INCLUDE_SELF);
		}
		if( leaves_play(player, card, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.subtype = SUBTYPE_GOBLIN;
			new_damage_all(player, card, 2, 1, 0, &this_test);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_goblin_wizard(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_GOBLIN;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Goblin permanent.");
	this_test.subtype = SUBTYPE_GOBLIN;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = 0;
		if( player == HUMAN || (player == AI && check_battlefield_for_special_card(player, card, player, 0, &this_test)) ){
			result |= generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE") ){
			result |= 1;
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( (player == HUMAN || (player == AI && check_battlefield_for_special_card(player, card, player, 0, &this_test))) &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Put a Goblin into play\n Give a Goblin prot. from white\n Cancel", 0);
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, choice, 0) ){
				if( choice == 0 ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				if( choice == 1 ){
					if( select_target(player, card, &td, "Select target Goblin", NULL) ){
						instance->number_of_targets = 1;
						instance->info_slot = 2;
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
		if( instance->info_slot == 2 && valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_PROT_WHITE, 0);
		}
	}
	return 0;
}

/*
Goblins of the Flarg English |R --> vanilla
Creature - Goblin Warrior 1/1
Mountainwalk
*/

int card_grave_robbers(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_B(1), 0, NULL, NULL) ){
			if( new_special_count_grave(2, &this_test) ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
				instance->targets[0].player = 1-player;
				if( count_graveyard_by_type(1-player, TYPE_ARTIFACT) > 0 ){
					if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && player != AI ){
						if( pick_target(&td, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
						}
					}
				}
				else{
					instance->targets[0].player = player;
				}
				if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) != -1 ){
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
				gain_life(player, 2);
			}
	}
	return 0;
}

int card_hidden_path(int player, int card, event_t event){
	/*
	  Hidden Path |2|G|G|G|G
	  Enchantment
	  Green creatures have forestwalk.
	*/

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( !(event == EVENT_CAST_SPELL && affect_me(player, card)) && event != EVENT_ABILITIES ){
		return 0;
	}

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI  ){
		ai_modifier+= (check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test)-
						check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test))*5;
	}

	if( in_play(player, card) && event == EVENT_ABILITIES && in_play(affected_card_controller, affected_card) && ! is_humiliated(player, card) ){
		if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
			event_result |= get_hacked_walk(player, card, KEYWORD_FORESTWALK);
		}
	}

	return 0;
}

int card_holy_light(int player, int card, event_t event){
	/*
	  Holy Light |2|W
	  Instant
	  Nonwhite creatures get -1/-1 until end of turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color(player, card, COLOR_TEST_WHITE);
		pump_creatures_until_eot_merge_pt(player, card, ANYBODY, -1, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_inferno2(int player, int card, event_t event){
	/*
	  Inferno |5|R|R
	  Instant
	  Inferno deals 6 damage to each creature and each player.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, ANYBODY, 6, NDA_ALL_CREATURES | NDA_PLAYER_TOO, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_inquisition2(int player, int card, event_t event){
	/*
	  Inquisition |2|B
	  Sorcery
	  Target player reveals his or her hand. Inquisition deals damage to that player equal to the number of white cards in his or her hand.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			reveal_target_player_hand(instance->targets[0].player);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			this_test.zone = TARGET_ZONE_HAND;
			damage_target0(player, card, check_battlefield_for_special_card(player, card, instance->targets[0].player, CBFSC_GET_COUNT, &this_test));
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

/*
Knights of Thorn English |3|W --> vanilla
Creature - Human Knight 2/2
Protection from red; banding

Land Leeches English |1|G|G --> vanilla
Creature - Leech 2/2
First strike
*/

int card_leviathan(int player, int card, event_t event)
{
#define DECL_TEST(test)																											\
  test_definition_t test;																										\
  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a to sacrifice.", SUBTYPE_ISLAND));	\
  test.subtype = get_hacked_subtype(player, card, SUBTYPE_ISLAND)

  // ~ enters the battlefield tapped and doesn't untap during your untap step.
  comes_into_play_tapped(player, card, event);
  does_not_untap(player, card, event);

  // At the beginning of your upkeep, you may sacrifice two |H1Islands. If you do, untap ~.
  // Simplification: only ask to sacrifice once, even if there's multiple upkeeps due to Paradox Haze
  if (trigger_condition == TRIGGER_UPKEEP && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER) && affect_me(player, card)
	  && !is_humiliated(player, card) && current_turn == player && count_upkeeps(player) > 0)
	{
	  DECL_TEST(test);

	  if (event == EVENT_TRIGGER && (test.qty = 2) && new_can_sacrifice_as_cost(player, card, &test))
		event_result |= ((player == HUMAN && ai_is_speculating != 1) ? RESOLVE_TRIGGER_OPTIONAL
						 : is_tapped(player, card) ? 0
						 : RESOLVE_TRIGGER_MANDATORY);

	  if (event == EVENT_RESOLVE_TRIGGER
		  && new_sacrifice(player, card, player, 0, &test)
		  && new_sacrifice(player, card, player, SAC_NO_CANCEL, &test))
		untap_card(player, card);
	}

  // ~ can't attack unless you sacrifice two |H1Islands.
  if (trigger_condition == TRIGGER_PAY_TO_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS
	  && affect_me(player, card) && reason_for_trigger_controller == player && forbid_attack == 0
	  && trigger_cause_controller == player && trigger_cause == card
	  && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER))
	{
	  DECL_TEST(test);

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER
		  && !((test.qty = 2) && new_can_sacrifice_as_cost(player, card, &test) && (test.qty = 1)
			   && new_sacrifice(player, card, player, 0, &test)
			   && new_sacrifice(player, card, player, SAC_NO_CANCEL, &test)))
		forbid_attack = 1;
	}

  return 0;
#undef DECL_TEST
}

int card_living_armor(int player, int card, event_t event){

	/* Living Armor	|4
	 * Artifact
	 * |T, Sacrifice ~: Put X +0/+1 counters on target creature, where X is that creature's converted mana cost. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_P0_P1,
						 get_cmc(instance->targets[0].player, instance->targets[0].card));
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_lurker(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player != 66 ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_SHROUD);
	}
	if( current_turn == player && event == EVENT_DECLARE_ATTACKERS ){
		if( instance->state & STATE_ATTACKING ){
			instance->targets[1].player = 66;
		}
	}
	if( current_turn == 1-player && event == EVENT_DECLARE_BLOCKERS ){
		if( instance->blocking < 255 ){
			instance->targets[1].player = 66;
		}
	}
	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = -1;
	}
	return 0;
}

int card_mana_clash(int player, int card, event_t event){
	/*
	  Mana Clash |R
	  Sorcery
	  You and target opponent each flip a coin.
	  Mana Clash deals 1 damage to each player whose coin comes up tails.
	  Repeat this process until both players' coins come up heads on the same flip.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int cf[2] = {1, 1};
			while( cf[0] || cf[1] ){
					cf[0] = player_flips_a_coin(player, card, player);
					if( cf[0] ){
						damage_player(player, 1, player, card);
					}
					cf[1] = player_flips_a_coin(player, card, 1-player);
					if( cf[1] ){
						damage_player(1-player, 1, player, card);
					}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_mana_vortex(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.allowed_controller = player;
		td1.preferred_controller = player;
		td1.who_chooses = player;
		td1.illegal_abilities = 0;
		if( target_available(player, card, &td1) ){
			if( !select_target(player, card, &td1, "Choose a land to sacrifice.", NULL) ){
				instance->info_slot = 0;
			}
			else {
				kill_card( instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE );
				instance->info_slot = 1;
			}
		}
		else {
			instance->info_slot = 0;
		}
		return 0;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 0 ){
			real_counter_a_spell(player, card, player, card);
		}
	}
	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( in_play(player, card) && count_permanents_by_type(ANYBODY, TYPE_LAND) == 0 ){
		kill_card(player, card, KILL_DESTROY);
	}
	return global_enchantment(player, card, event);
}

int card_marsh_gas(int player, int card, event_t event){
	/*
	  Marsh Gas English |B
	  Instant
	  All creatures get -2/-0 until end of turn.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, ANYBODY, -1, -2, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/*
Marsh Goblins |B|R --> vanilla
Creature - Goblin 1/1
Swampwalk
*/

int card_marsh_viper(int player, int card, event_t event){
	/*
	  Marsh Viper English |3|G
	  Creature - Snake 1/2
	  Whenever Marsh Viper deals damage to a player, that player gets two poison counters. (A player with ten or more poison counters loses the game.)
	*/
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
		card_instance_t *instance = get_card_instance(player, card);
		if (BYTE0(instance->targets[1].player)){
			poison(0, 2);
		}
		if (BYTE1(instance->targets[1].player)){
			poison(1, 2);
		}
	}
	return 0;
}

int card_martyrs_cry(int player, int card, event_t event){
	/*
	  Martyr's Cry English |W|W
	  Sorcery
	  Exile all white creatures. For each creature exiled this way, its controller draws a card.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

		int amount = new_manipulate_all(player, card, player, &this_test, KILL_REMOVE);
		draw_cards(player, amount);

		amount = new_manipulate_all(player, card, 1-player, &this_test, KILL_REMOVE);
		draw_cards(1-player, amount);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int maze_of_ith_legacy(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_prevented(event);
	if (damage){
		card_instance_t* instance = get_card_instance(player, card);

		if( (damage->damage_source_player == instance->targets[0].player && damage->damage_source_card == instance->targets[0].card) ||
			(damage->damage_target_player == instance->targets[0].player && damage->damage_target_card == instance->targets[0].card)
		  ){
			damage->info_slot = 0;
		}
	}

	if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card)){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int maze_of_ith_effect(int player, int card, int t_player, int t_card){
	return create_targetted_legacy_effect( player, card, &maze_of_ith_legacy, t_player, t_card);
}

int card_maze_of_ith(int player, int card, event_t event){
	// original code : 00406100

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if(valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			maze_of_ith_effect(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ATTACKING_CREATURE");
}

int card_merfolk_assassin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = get_hacked_walk(player, card, KEYWORD_ISLANDWALK);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_mind_bomb(int player, int card, event_t event){
	/*
	  Mind Bomb |U
	  Sorcery
	  Each player may discard up to three cards. Mind Bomb deals damage to each player equal to 3 minus the number of cards he or she discarded this way.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard");

		APNAP(p,{
					int dmg = 3;
					while( dmg && hand_count[p] > 0 && (p == HUMAN || (p== AI && life[p]-dmg < 6)) ){
							int selected = new_select_a_card(p, p, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
							if( selected != -1 ){
								new_discard_card(p, selected, player, 0);
								dmg--;
							}
							else{
								break;
							}
					}
					damage_player(p, dmg, player, card);
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static const char* aura_enchanting_creature(int who_chooses, int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player == who_chooses ){
		if( is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE) ){
			return NULL;
		}
	}

	return "must be an Aura enchanting a creature you control.";
}

int card_miracle_worker(int player, int card, event_t event){
	/*
	  Miracle Worker |W
	  Creature - Human Cleric 1/1
	  {T}: Destroy target Aura attached to a creature you control.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)aura_enchanting_creature;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_ENCHANTMENT");
}

int card_morale(int player, int card, event_t event){
	/*
	  Morale |1|W|W
	  Instant
	  Attacking creatures get +1/+1 until end of turn.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		pump_creatures_until_eot_merge_pt(player, card, current_turn, 1, 1, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_murk_dwellers(int player, int card, event_t event){
	/*
	  Murk Dwellers |3|B
	  Creature - Zombie 2/2
	  Whenever Murk Dwellers attacks and isn't blocked, it gets +2/+0 until end of combat.
	*/

	if( ! is_humiliated(player, card) && event == EVENT_DECLARE_BLOCKERS && is_unblocked(player, card) ){
		pump_until_eot(player, card, player, card, 2, 0);
	}

	return 0;
}

int card_nameless_race(int player, int card, event_t event){

	if (card == -1){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		modify_pt_and_abilities(player, card, event, instance->targets[1].player, instance->targets[1].card, 0);
	}

	if( comes_into_play(player, card, event) && can_pay_life(player, 1) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.color = COLOR_TEST_WHITE;

		int max = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
		max+=new_special_count_grave(1-player, &this_test);
		int number = 0;
		if( !IS_AI(player) ){
			number = choose_a_number(player, "Nameless Race: Pay how much life?", max);
			if( number > max ){
				number = max;
			}
		}
		else{
			number = max;
			while( life[player]-number < 6 ){
					number--;
			}
		}
		lose_life(player, number);
		instance->targets[1].player = number;
		instance->targets[1].card = number;
	}
	return 0;
}

int card_necropolis(int player, int card, event_t event){

	/* Necropolis	|5
	 * Artifact Creature - Wall 0/1
	 * Defender
	 * Exile a creature card from your graveyard: Put X +0/+1 counters on ~, where X is the exiled card's converted mana cost. */

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				instance->targets[1].player = 0;
				int selected = select_a_card(player, player, 2, 0, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				if( selected != -1 ){
					const int *grave = get_grave(player);
					instance->targets[1].player = get_cmc_by_internal_id(grave[selected]);
					rfg_card_from_grave(player, selected);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			add_counters(instance->parent_controller, instance->parent_card, COUNTER_P0_P1, instance->targets[1].player);
	}
	return 0;
}

int card_niall_silvain(int player, int card, event_t event){
	/*
	  Niall Silvain English |G|G|G
	  Creature - Ouphe 2/2
	  {G}{G}{G}{G}, {T}: Regenerate target creature.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) && can_be_regenerated(instance->targets[0].player, instance->targets[0].card) ){
		regenerate_target(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(4), 0, &td, "TARGET_CREATURE") ? 99 : 0;
}

static int get_orc_or_goblin(int player, int card, int mode){
	int count = 0;
	int par = 1000;
	int result = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && count != card ){
				if( is_what(player, count, TYPE_PERMANENT) ){
					if( has_subtype(player, count, SUBTYPE_GOBLIN) || has_subtype(player, count, SUBTYPE_ORC) ){
						if( mode == 0 ){
							result = 1;
						}
						if( mode == 1 ){
							if( is_nice_creature_to_sacrifice(player, count) ){
								result = count;
								par = 0;
							}
							else{
								if( par > 0 && get_base_value(player, count) < par ){
									par = get_base_value(player, count);
									result = count;
								}
							}
						}
					}
					if( get_id(player, card) == CARD_ID_ANGEL_OF_JUBILATION ){
						result = 0;
						break;
					}
				}
			}
			count++;
	}
	if( check_battlefield_for_id(1-player, CARD_ID_ANGEL_OF_JUBILATION) ){
		result = 0;
	}
	if( mode == 1 && result == 0 ){
		result = -1;
	}
	return result;
}

int card_orc_general(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return get_orc_or_goblin(player, card, 0);
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( player != AI ){
				if( select_target(player, card, &td, "Select an Orc or Goblin to sacrifice.", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
					if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_GOBLIN) ||
						has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ORC)
					  ){
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						tap_card(player, card);
					}
					else{
						spell_fizzled = 1;
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				int result = get_orc_or_goblin(player, card, 1);
				if( result > -1 ){
					kill_card(player, result, KILL_SACRIFICE);
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && count != card ){
					if( is_what(player, count, TYPE_PERMANENT) ){
						if( has_subtype(player, count, SUBTYPE_ORC) ){
							pump_until_eot(player, instance->parent_card, player, count, 1, 1);
						}
					}
				}
				count--;
		}
	}
	return 0;
}

int card_people_of_the_woods(int player, int card, event_t event){
	/*
	  People of the Woods English |G|G
	  Creature - Human 1/X
	  People of the Woods's toughness is equal to the number of Forests you control.
	*/
	if( event == EVENT_TOUGHNESS && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	}

	return 0;
}

int card_preacher(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	/* Preacher	|1|W|W
	 * Creature - Human Cleric 1/1
	 * You may choose not to untap ~ during your untap step.
	 * |T: Gain control of target creature of an opponent's choice that he or she controls for as long as ~ remains tapped. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.who_chooses = 1 - player;
	td.allowed_controller = 1 - player;
	td.preferred_controller = 1 - player;
	td.allow_cancel = 0;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);
		gain_control_until_source_is_in_play_and_tapped(instance->parent_controller,instance->parent_card, instance->targets[0].player, instance->targets[0].card, GCUS_TAPPED);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "ASHNODS_BATTLEGEAR");	// "Select target creature you control."
}

/*
Pikemen English |1|W --> vanilla
Creature - Human Soldier 1/1
First strike; banding
*/

int card_psychic_allergy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int clr = 0;
		if( player != AI ){
			clr = choose_a_color(player, 1);
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.type_flag = F1_NO_TOKEN;

			int i;
			for(i=1; i<6; i++){
				this_test.color = 1<<i;
				if( check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test) >
					check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test)
				  ){
					clr = i;
				}
			}
		}
		create_card_name_legacy(player, card, CARD_ID_BLACK+(clr-1));
		instance->targets[1].player = 1<<clr;
	}

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_NO_TOKEN;

		if( current_turn != player ){
			this_test.color = instance->targets[1].player;
			int amount = check_battlefield_for_special_card(player, card, current_turn, CBFSC_GET_COUNT, &this_test);
			amount*=count_upkeeps(player);
			if( amount > 0 ){
				damage_player(current_turn, amount, player, card);
			}
		}
		else{
			int count = count_upkeeps(player);
			if( count > 0 ){
				int kill = count;
				if( can_sacrifice(player, player, 2*count, TYPE_LAND, SUBTYPE_ISLAND) ){
					while( count > 0 ){
							if( sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
								impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0);
								kill--;
								count--;
							}
							else{
								break;
							}
					}
				}
				if( kill > 0 ){
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_rag_man(int player, int card, event_t event){
	/*
	  Rag Man English |2|B|B
	  Creature - Human Minion 2/1
	  {B}{B}{B}, {T}: Target opponent reveals his or her hand and discards a creature card at random. Activate this ability only during your turn.
	*/
	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) && hand_count[instance->targets[0].player] > 0){
		reveal_target_player_hand(instance->targets[0].player);
		int opp_hand[hand_count[instance->targets[0].player]];
		int ohc = 0;
		int i;
		for(i=0; i<active_cards_count[instance->targets[0].player]; i++){
			if( in_hand(instance->targets[0].player, i) && is_what(instance->targets[0].player, i, TYPE_CREATURE) ){
				opp_hand[ohc] = i;
				ohc++;
			}
		}
		if( ohc ){
			int rnd = ohc > 1 ? internal_rand(ohc) : 0;
			new_discard_card(instance->targets[0].player, opp_hand[rnd], player, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_B(3), 0, &td, "TARGET_PLAYER");
}

static const char* spell_is_targeting_me(int who_chooses, int player, int card)
{
	card_instance_t* eff = get_card_instance(player, card);
	if( eff->number_of_targets == 0 || eff->number_of_targets > 1)
		return "must have only 1 target";
	if( !(eff->targets[0].player == who_chooses && eff->targets[0].card == -1) )
		return "must target you.";
	if( ! call_card_function(player, card, EVENT_CAN_CHANGE_TARGET) )
		return "has only 1 legal target";

	return NULL;
}

int card_reflecting_mirror(int player, int card, event_t event){

	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)spell_is_targeting_me;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( counterspell(player, card, EVENT_CAN_CAST, NULL, 0) ){
			int cmc = 2*get_cmc(card_on_stack_controller, card_on_stack);
			return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_UNTAPPED, MANACOST_X(cmc), 0, NULL, NULL);
		}
	}

	if(event == EVENT_ACTIVATE ){
		set_special_flags2(card_on_stack_controller, card_on_stack_controller, SF2_REFLECTING_MIRROR);
		call_card_function(card_on_stack_controller, card_on_stack_controller, EVENT_CHANGE_TARGET);
		if( spell_fizzled != 1 ){
			instance->targets[0] = get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9];
			instance->number_of_targets = 1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].player = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[9].card = -1;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->number_of_targets = 1;
		}
		remove_special_flags2(card_on_stack_controller, card_on_stack_controller, SF2_REFLECTING_MIRROR);
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->targets[0] = instance->targets[0];
		}
	}
	return 0;
}

int card_riptide2(int player, int card, event_t event){
	/*
	  Riptide |U
	  Instant
	  Tap all blue creatures.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color(player, card, COLOR_TEST_BLUE);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_TAP);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int effect_runesword(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		}
		if( event == EVENT_POWER && affect_me(instance->targets[0].player, instance->targets[0].card) &&
			! is_humiliated(instance->targets[1].player, instance->targets[1].card)
		  ){
			event_result += 2;
		}

		if (event == EVENT_DEAL_DAMAGE){
			card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
			if (damage->internal_card_id == damage_card && damage->info_slot > 0
				&& damage->damage_source_card == instance->targets[0].card
				&& damage->damage_source_player == instance->targets[0].player
				&& damage->damage_target_card != -1
			   ){
				if( instance->targets[2].player < 3 ){
					instance->targets[2].player = 3;
				}
				int pos = instance->targets[2].player;
				if( pos < 10 ){
					instance->targets[pos].player = damage->damage_target_player;
					instance->targets[pos].card = damage->damage_target_card;
					instance->targets[2].player++;
				}
			}
		}

		if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player && instance->targets[2].player > 3 ){
			if (event == EVENT_TRIGGER){
				event_result = RESOLVE_TRIGGER_MANDATORY;
			}
			else if (event == EVENT_RESOLVE_TRIGGER){
				int i;
				for (i=3; i<instance->targets[2].player; i++){
					cannot_regenerate_until_eot(instance->targets[1].player, instance->targets[1].card, instance->targets[i].player, instance->targets[i].card);
					exile_if_would_be_put_into_graveyard(instance->targets[1].player, instance->targets[1].card,
														 instance->targets[i].player, instance->targets[i].card, 1);
				}
				instance->targets[2].player = 3;
			}
		}
	}
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_runesword(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy_card = create_targetted_legacy_effect(player, instance->parent_card, &effect_runesword, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
			legacy_instance->targets[1].player = instance->parent_controller;
			legacy_instance->targets[1].card = instance->parent_card;
			legacy_instance->number_of_targets = 2;
			legacy_instance->targets[2].card = get_id(player, instance->parent_card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_safe_haven(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		ai_modifier+=count_subtype(player, TYPE_LAND, -1)*2;
	}

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn == player && instance->targets[1].player > 2
		&& count_upkeeps(player)
	  ){
		int result = RESOLVE_TRIGGER_OPTIONAL;
		if( player == AI && instance->targets[1].player > 3 ){
			result = RESOLVE_TRIGGER_MANDATORY;
		}
		if( event == EVENT_TRIGGER ){
			event_result |= result;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				int i;
				for(i=2; i<instance->targets[1].player; i++ ){
					if( check_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id ) ){
						int card_added = add_card_to_hand(instance->targets[i].player, instance->targets[i].card);
						remove_card_from_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id);
						put_into_play(instance->targets[i].player, card_added);
					}
				}
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	if( player == AI ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE") ){
			if( player == HUMAN ){
				return 1;
			}
			else{
				if( land_can_be_played & LCBP_REGENERATION ){
					return 0x63;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( player == AI ){
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
			int iid = ! is_token(instance->targets[0].player, instance->targets[0].card) ?
						get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card) : -1;

			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			if( iid != -1 ){
				card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
				if( parent->targets[1].player < 2 ){
					parent->targets[1].player = 2;
				}
				int pos = parent->targets[1].player;
				if( pos < 10 ){
					parent->targets[pos].player = get_owner(instance->targets[0].player, instance->targets[0].card);
					parent->targets[pos].card = iid;
					parent->targets[1].player++;
					create_card_name_legacy(instance->parent_controller, instance->parent_card, cards_data[iid].id);
				}
			}
		}
	}
	return 0;
}

static const char* aura_enchanting_land(int who_chooses, int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( is_what(instance->damage_target_player, instance->damage_target_card, TYPE_LAND) ){
			return NULL;
		}
	}

	return "must be an Aura enchanting a land.";
}

int card_savaen_elves(int player, int card, event_t event){
	/*
	  Savaen Elves |G
	  Creature - Elf 1/1
	  {G}{G}, {T}: Destroy target Aura attached to a land.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)aura_enchanting_land;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(2), 0, &td, "TARGET_ENCHANTMENT");
}

static int effect_scarecrow(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( affected->internal_card_id == damage_card ){
			if( (get_abilities(affected->damage_source_player, affected->damage_source_card, EVENT_ABILITIES, -1) & KEYWORD_FLYING) &&
				affected->damage_target_player == player && affected->damage_target_card == -1 && affected->info_slot > 0
			  ){
				affected->info_slot = 0;
			}
		}
	}
	else if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_scarecrow(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 create_legacy_effect(player, instance->parent_card, &effect_scarecrow);
	}
	return 0;
}

static int scarwood_bandits_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( leaves_play( instance->targets[1].player, instance->targets[1].card, event) ){
			kill_card(player, instance->targets[2].card, KILL_REMOVE );
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_scarwood_bandits(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( in_play(player, instance->parent_card) ){
				int steal = 1;
				if( has_mana(instance->targets[0].player, COLOR_COLORLESS, 2) ){
					charge_mana(instance->targets[0].player, COLOR_COLORLESS, 2);
					if( spell_fizzled != 1 ){
						steal = 0;
					}
				}
				if( steal == 1 ){
					int legacy = gain_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
					if( legacy > -1 ){
						int l2 = create_targetted_legacy_effect(player, instance->parent_card, &scarwood_bandits_legacy, instance->targets[0].player, instance->targets[0].card);
						card_instance_t *leg = get_card_instance(player, l2);
						leg->targets[1].player = player;
						leg->targets[1].card = instance->parent_card;
						leg->number_of_targets = 2;
						leg->targets[2].card = legacy;
						card_instance_t *parent = get_card_instance(player, instance->parent_card);
						parent->targets[1].player = 66;
					}
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 2, 0, 0, 1, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

/*
Scarwood Goblins |R|G --> vanilla
Creature - Goblin 2/2
*/

int card_scarwood_hag(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	int walk = get_hacked_walk(player, card, KEYWORD_FORESTWALK);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.required_abilities = walk;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( !is_tapped(player, card) && !is_sick(player, card) && can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( (player == AI && current_turn != player && can_target(&td2)) || (player != AI && can_target(&td1)) ){
					return 1;
				}
			}
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 4, 0, 0) ){
				if( can_target(&td) ){
					return 1;
				}
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 4, 0, 0) && can_target(&td) ){
			if( (player == AI && current_turn != player && can_target(&td2)) || (player != AI && can_target(&td1)) ){
				choice = do_dialog(player, player, card, -1, -1,
								   get_hacked_land_text(player, card, " Grant %lwalk\n Remove %lwalk\n Cancel", SUBTYPE_FOREST, SUBTYPE_FOREST), 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 4, 0, 0) ){
					if( player != AI ){
						if( pick_target(&td1, "TARGET_CREATURE") ){
							instance->info_slot = 66;
							instance->number_of_targets = 1;
							tap_card(player, card);
						}
					}
					else{
						if( pick_target(&td2, "TARGET_CREATURE") ){
							instance->info_slot = 66;
							instance->number_of_targets = 1;
							tap_card(player, card);
						}
					}
				}
		}
		else if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
				instance->info_slot = 67;
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				alternate_legacy_text(1, player,
									  pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0,
															 KEYWORD_FORESTWALK, 0));	// pump_ability_until_eot() will hack_walk it
			}
			else if( instance->info_slot == 67 ){
					int good = 0;
					if( player != AI && valid_target(&td1) ){
						good = 1;
					}
					if( player == AI && valid_target(&td2) ){
						good = 1;
					}
					if( good == 1 ){
						alternate_legacy_text(2, player,
											  negate_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, walk));
					}
			}
	}
	return 0;
}

int card_scavenger_folk(int player, int card, event_t event)
{
	/*
	  Scavenger Folk |G
	  Creature - Human 1/1
	  {G}, {T}, Sacrifice Scavenger Folk: Destroy target artifact.
	*/

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_ARTIFACT");
}

int card_season_of_the_witch(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( can_pay_life(player, 2) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay Upkeep\n Pass", life[player]-2 < 6);
			if( choice == 0 ){
				lose_life(player, 2);
				kill = 0;
			}
		}

		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( beginning_of_combat(player, card, event, ANYBODY, -1) ){
		instance->info_slot = 0;
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
				if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) && can_attack(current_turn, count) ){
					while( instance->info_slot < 19 ){
							if( instance->targets[instance->info_slot].player == -1 ){
								instance->targets[instance->info_slot].player = count;
								break;
							}
							if( instance->targets[instance->info_slot].card == -1 ){
								instance->targets[instance->info_slot].card = count;
								break;
							}
							instance->info_slot++;
					}
				}
				count--;
		}
	}

	if( eot_trigger(player, card, event) ){
		int i = 0;
		while( i < 19 ){
				if( instance->targets[i].player != -1 ){
					if( ! check_state(current_turn, instance->targets[i].player, STATE_ATTACKED)){
						kill_card(current_turn, instance->targets[i].player, KILL_DESTROY);
					}
					instance->targets[i].player = -1;
				}
				else{
					break;
				}
				if( instance->targets[i].card != -1 ){
					if( ! check_state(current_turn, instance->targets[i].card, STATE_ATTACKED)){
						kill_card(current_turn, instance->targets[i].card, KILL_DESTROY);
					}
					instance->targets[i].card = -1;
				}
				else{
					break;
				}
				i++;
		}
		instance->info_slot = 0;
	}
	return global_enchantment(player, card, event);
}

/* Sisters of the Flame	|1|R|R	=>battle_for_zendikar.c:card_generic_combat_1_mana_producing_creature
 * Creature - Human Shaman 2/2
 * |T: Add |R to your mana pool. */

int card_skull_of_orm(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an enchantment card.");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_ENCHANTMENT) > 0 ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(5)) ){
			int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
			if( selected != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}
	return 0;
}

/*
Sorrow's Path --> Uber complex + junk. No, thanx.
Land
{T}: Choose two target blocking creatures an opponent controls.
If each of those creatures could block all creatures that the other is blocking, remove both of them from combat.
Each one then blocks all creatures the other was blocking.
Whenever Sorrow's Path becomes tapped, it deals 2 damage to you and each creature you control.
*/

int card_spitting_slug(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_DECLARE_BLOCKERS ){
		if( instance->blocking < 255 ){
			charge_mana_multi(player, 1, 0, 0, 1, 0, 0);
			if( spell_fizzled == 1 ){
				int leader = instance->blocking;
				pump_ability_until_eot(player, card, 1-player, leader, 0, 0, KEYWORD_FIRST_STRIKE, 0);
				int i;
				for( i=0; i<active_cards_count[1-player]; i++ ){
					card_instance_t *oba_instance = get_card_instance(1-player, i);
					if( i != leader && oba_instance->state & STATE_ATTACKING && oba_instance->blocking == leader ){
						pump_ability_until_eot(player, card, 1-player, i, 0, 0, KEYWORD_FIRST_STRIKE, 0);
					}
				}
			}
			else {
				pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
			}
		}
		else if( instance->state & STATE_ATTACKING && !is_unblocked(player, card) ){
			charge_mana_multi(player, 1, 0, 0, 1, 0, 0);
			if( spell_fizzled == 1 ){
				int j;
				for( j=0; j<active_cards_count[1-player]; j++ ){
					card_instance_t *blocker_instance = get_card_instance(1-player, j);
					if( blocker_instance->blocking == card ){
						pump_ability_until_eot(player, card, 1-player, j, 0, 0, KEYWORD_FIRST_STRIKE, 0);
					}
				}
			}
			else {
				pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
			}
		}
	}
	return 0;
}

/*
Squire English |1|W --> VANILLAAAAAA !
Creature - Human Soldier 1/2
*/

int card_standing_stones(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana(player, COLOR_ANY, 1) && !is_tapped(player, card) && !is_animated_and_sick(player, card) &&
			can_produce_mana(player, card) && can_pay_life(player, 1)
		  ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
		ai_modifier -= 36;

		card_instance_t* instance = get_card_instance(player, card);
		instance->state |= STATE_TAPPED;

		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled == 1 ){
			instance->state &= ~STATE_TAPPED;
			return 0;
		}
		produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1);
		if( spell_fizzled == 1 ){
			instance->state &= ~STATE_TAPPED;
			return 0;
		}
		lose_life(player, 1);
	}
	return 0;
}

int card_stone_calendar(int player, int card, event_t event){
	/*
	  Stone Calendar |5
	  Artifact
	  Spells you cast cost up to {1} less to cast.
	*/
	if( ! is_humiliated(player, card) && event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		COST_COLORLESS--;
	}

	return 0;
}

int card_sunken_city(int player, int card, event_t event){
	/*
	  Sunken City |U|U
	  Enchantment
	  At the beginning of your upkeep, sacrifice Sunken City unless you pay {U}{U}.
	  Blue creatures get +1/+1.
	*/
	basic_upkeep(player, card, event, MANACOST_U(2));

	boost_creature_by_color(player, card, event, get_sleighted_color_test(player, card, COLOR_TEST_BLUE), 1, 1, 0, 0);

	return global_enchantment(player, card, event);
}


int card_tangle_kelp(int player, int card, event_t event)
{
  // 0x40f3f0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CLEANUP && affect_me(player, card))
	if (instance->damage_target_player == current_turn)
	  {
		card_instance_t* ench_inst = get_card_instance(instance->damage_target_player, instance->damage_target_card);
		instance->info_slot = ench_inst->state & STATE_ATTACKED;
	  }

  if (comes_into_play(player, card, event))
	tap_card(instance->damage_target_player, instance->damage_target_card);

  if (instance->info_slot)
	does_not_untap(instance->damage_target_player, instance->damage_target_card, event);

  return disabling_aura(player, card, event);
}

int card_the_fallen(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		instance->targets[1].player = 66;
	}

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(current_turn);
		if(event == EVENT_TRIGGER && count > 0 && instance->targets[1].player == 66 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				card_data_t* card_d = &cards_data[ instance->internal_card_id ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(1-player, count_upkeeps(player), player, card);
	}


	return 0;
}

int card_tivadars_crusade(int player, int card, event_t event){
	/*
	  Tivadar's Crusade |1|W|W
	  Sorcery
	  Destroy all Goblins.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_GOBLIN;
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_tormods_crypt(int player, int card, event_t event)
{
  // 0x40c080

  /* Tormod's Crypt	|0
   * Artifact
   * |T, Sacrifice ~: Exile all cards from target player's graveyard. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  card_instance_t* instance = get_card_instance(player, card);

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_UNTAPPED|GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_PLAYER");
  if (event == EVENT_ACTIVATE && IS_AI(player))
	{
	  int tgt = instance->targets[0].player;
	  int mod = 2 * count_graveyard_by_type(tgt, TYPE_ARTIFACT | TYPE_SORCERY | TYPE_CREATURE) + count_graveyard(tgt) - 30;
	  if (tgt == player)
		mod -= 128;

	  if (player == 1)
		ai_modifier += mod;
	  else
		ai_modifier -= mod;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	rfg_whole_graveyard(instance->targets[0].player);

  return rval;
}

static int effect_tower_of_coireall(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_BLOCK_LEGALITY ){
		if( instance->targets[0].player == attacking_card_controller && instance->targets[0].card == attacking_card ){
			if( has_creature_type(affected_card_controller, affected_card, SUBTYPE_WALL) ){
				event_result = 1;
			}
		}
	}
	else if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_tower_of_coireall(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &effect_tower_of_coireall, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_tracker2(int player, int card, event_t event){
	/*
	  Tracker |2|G
	  Creature - Human 2/2
	  {G}{G}, {T}: Tracker deals damage equal to its power to target creature. That creature deals damage equal to its power to Tracker.
	*/

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			// Not a fight.  Stop replacing this with fight().
			get_card_instance(instance->parent_controller, instance->parent_card)->regen_status |= KEYWORD_RECALC_POWER;
			int mypow = get_power(instance->parent_controller, instance->parent_card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, mypow, instance->parent_controller, instance->parent_card);
			if (in_play(instance->parent_controller, instance->parent_card)){
				get_card_instance(instance->targets[0].player, instance->targets[0].card)->regen_status |= KEYWORD_RECALC_POWER;
				int hispow = get_power(instance->targets[0].player, instance->targets[0].card);
				damage_creature(instance->parent_controller, instance->parent_card, hispow, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_G(2), 0, &td, "TARGET_CREATURE");
}

int card_uncle_istvan(int player, int card, event_t event){
	/*
	  Uncle Istvan |1|B|B|B
	  Creature - Human 1/3
	  Prevent all damage that would be dealt to Uncle Istvan by creatures.
	*/
	if( ! is_humiliated(player, card) && event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == card && damage->info_slot > 0 && (damage->targets[3].player & TYPE_CREATURE)
		  ){
			damage->info_slot = 0;
		}
	}

	return 0;
}

static void destroy_nonwall_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (!has_subtype(t_player, t_card, SUBTYPE_WALL))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_venom(int player, int card, event_t event)
{
  // 0x4ca760
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->damage_target_player >= 0)
	{
	  int p = instance->damage_target_player, c = instance->damage_target_card;

	  if (event == EVENT_CHANGE_TYPE && affect_me(p, c) && !is_humiliated(player, card))	// only if the aura loses abilities, not if the enchanted creature does
		get_card_instance(p, c)->destroys_if_blocked |= DIFB_DESTROYS_NONWALLS;

	  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
		{
		  if (p == current_turn && is_attacking(p, c) )
			for_each_creature_blocking_me(p, c, destroy_nonwall_at_end_of_combat, player, card);

		  if (p == 1-current_turn &&  blocking(p, c, event) )
			for_each_creature_blocked_by_me(p, c, destroy_nonwall_at_end_of_combat, player, card);
		}
	}

  return vanilla_aura(player, card, event, player);
}

int card_wand_of_ith(int player, int card, event_t event)
{
  /* Wand of Ith	|4
   * Artifact
   * |3, |T: Target player reveals a card at random from his or her hand. If it's a land card, that player discards it unless he or she pays 1 life. If it isn't
   * a land card, the player discards it unless he or she pays life equal to its converted mana cost. Activate this ability only during your turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->targets[0].player;

	  if (hand_count[p] <= 0)
		{
		  if (player == AI)
			ai_modifier -= 48;
		  return 0;
		}

	  int c = get_random_card_in_hand(p);

	  int cost = is_what(p, c, TYPE_LAND) ? 1 : get_cmc(p, c);

	  char pay_x_life[100];
	  if (ai_is_speculating == 1)
		pay_x_life[0] = 0;
	  else
		{
		  load_text(0, "WANDOFITH");
		  scnprintf(pay_x_life, 100, text_lines[1], cost);	// "Pay %d life."
		}

	  int cpl = can_pay_life(p, cost);

	  // DLG_RANDOM_NO_SAVE because of the random card selection above
	  if (DIALOG(player, card, event,
				 DLG_RANDOM_NO_SAVE, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_WHO_CHOOSES(p), DLG_SMALLCARD(p, c),
				 pay_x_life, cpl, life[p] - cost,
				 text_lines[2], 1, 5) == 1)
		lose_life(p, cost);
	  else
		new_discard_card(p, c, player, 0);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_IN_YOUR_TURN, MANACOST_X(3), 0, &td, "TARGET_PLAYER");
}

int war_barge_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_war_barge(int player, int card, event_t event){
	/*
	  War Barge |4
	  Artifact
	  {3}: Target creature gains islandwalk until end of turn.
	  When War Barge leaves the battlefield this turn, destroy that creature. A creature destroyed this way can't be regenerated.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, get_hacked_walk(player, card, KEYWORD_ISLANDWALK), 0);
			int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &war_barge_legacy,
											instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = instance->parent_controller;
			leg->targets[1].card = instance->parent_card;
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_water_wurm(int player, int card, event_t event){
	/*
	  Water Wurm |U
	  Creature - Wurm 1/1
	  Water Wurm gets +0/+1 as long as an opponent controls an Island.
	*/
	if( ! is_humiliated(player, card) && event == EVENT_TOUGHNESS && affect_me(player, card) ){
		if( check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_ISLAND) ){
			event_result++;
		}
	}

	return 0;
}

static int whippoorwill_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	int t_player = instance->targets[0].player;
	int t_card = instance->targets[0].card;
	if( t_player > -1 ){
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
			if( damage_card == source->internal_card_id && source->damage_target_player == t_player && source->damage_target_card == t_card ){
				state_untargettable(affected_card_controller, affected_card, 1);
			}
		}
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(t_player, t_card) ){
			card_instance_t *this = get_card_instance( t_player, t_card );
			if( this->kill_code > 0 && this->kill_code < KILL_REMOVE ){
				this->kill_code = KILL_REMOVE;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		remove_status(t_player, t_card, STATUS_CANNOT_REGENERATE);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_whippoorwill(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			add_status(instance->targets[0].player, instance->targets[0].card, STATUS_CANNOT_REGENERATE);
			int legacy = create_legacy_effect(player, card, &whippoorwill_legacy);
			get_card_instance(player, legacy)->targets[0] = instance->targets[0];
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 2, 0, 0, 0, &td, "TARGET_CREATURE");
}

static const char* dangerous_attacking_creature(int who_chooses, int player, int card){

	if ( is_attacking(player, card) && is_unblocked(player, card) && life[who_chooses]-get_power(player, card) < 6 ){
		return NULL;
	}

	return "AI helper function";
}

int card_witch_hunter(int player, int card, event_t event){
	/*
	  Witch Hunter |2|W|W
	  Creature - Human Cleric 1/1
	  {T}: Witch Hunter deals 1 damage to target player.
	  {1}{W}{W}, {T}: Return target creature an opponent controls to its owner's hand.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	if( player == AI ){
		td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td1.extra = (int32_t)dangerous_attacking_creature;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER") ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(1, 2), 0, &td1, "TARGET_CREATURE") ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER") ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(1, 2), 0, &td1, "TARGET_CREATURE") ){
				int ai_choice = (life[1-player] == 1 || current_turn == player) ? 0 : 1;
				choice = do_dialog(player, player, card, -1, -1, " Damage player\n Bounce creature\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 0 ){
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(1, 2), 0,
								&td1, "Select target creature an opponent controls.");
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			damage_target0(player, card, 1);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_word_of_binding(int player, int card, event_t event){
	return card_word_of_binding_exe(player, card, event);
}

int card_worms_of_the_earth(int player, int card, event_t event){
	// new card
	/*
	  Worms of the Earth |2|B|B|B
	  Enchantment
	  Players can't play lands.
	  Lands can't enter the battlefield.
	  At the beginning of each upkeep, any player may sacrifice two lands or have Worms of the Earth deal 5 damage to him or her. If a player does either, destroy Worms of the Earth.
	*/
	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_NO_STORAGE, DLG_WHO_CHOOSES(current_turn),
						"Sac two lands", count_subtype(current_turn, TYPE_LAND, -1), count_subtype(current_turn, TYPE_LAND, -1)*5,
						"Take 5 damage", 1, life[current_turn]*2);
		if( choice == 1 ){
			if( sacrifice(player, card, current_turn, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				impose_sacrifice(player, card, current_turn, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		if( choice == 2 ){
			damage_player(current_turn, 5, player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) ){
		player_bits[player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;
		player_bits[1-player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;

		if( !(land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) ){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}

	if( leaves_play(player, card, event) && lands_played < total_playable_lands(current_turn) ){
		land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
		player_bits[player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
		player_bits[1-player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
	}

	return global_enchantment(player, card, event);
}

int card_wormwood_treefolk(int player, int card, event_t event){
	/*
	  Wormwood Treefolk English |3|G|G
	  Creature - Treefolk 4/4
	  {G}{G}: Wormwood Treefolk gains forestwalk until end of turn and deals 2 damage to you.
	  {B}{B}: Wormwood Treefolk gains swampwalk until end of turn and deals 2 damage to you.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return has_mana_for_activated_ability(player, card, MANACOST_B(2)) || has_mana_for_activated_ability(player, card, MANACOST_G(2)) ;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_B(2)) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_G(2)) ){
				choice = do_dialog(player, player, card, -1, -1, " Give Forestwalk\n Give Swampwalk\n Cancel", check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_SWAMP));
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, 0, (choice == 1 ? 2 : 0), 0, (choice == 0 ? 2 : 0), 0, 0) ){
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int kw = instance->info_slot == 66 ? get_hacked_walk(instance->parent_controller, instance->parent_card, KEYWORD_FORESTWALK) :
											get_hacked_walk(instance->parent_controller, instance->parent_card, KEYWORD_SWAMPWALK);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 0, 0, kw, 0);
		damage_player(instance->parent_controller, 2, instance->parent_controller, instance->parent_card);
	}

	return 0;
}

