#include <limits.h>

#include "manalink.h"

static void mark_creatures(int itgt, int iinfo_slot, int t_player, int t_card)
{
  int16_t** tgt = (int16_t**)itgt;
  **tgt = t_card;
  ++*tgt;
}

int card_abu_jafar(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_DESTROYS_ALL;

  if (event == EVENT_RESOLVE_THIS_DIES_TRIGGER	// this is the legacy, not the creature
	  || (((player == current_turn && (instance->state & STATE_ATTACKING))
		   || (player == 1-current_turn && (instance->state & STATE_BLOCKING)))
		  && !is_humiliated(player, card)))
	{
	  // Checking this directly breaks encapsulation.  Horrid.
	  if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card) && instance->kill_code != KILL_REMOVE)
		{
		  /* Have to store affected creatures while this is still in play.
		   *
		   * Store one card index per word in the targets array, and assume all are controlled by the other player.  Not updateable if some other simultaneous
		   * trigger causes a control change (Sower of Temptations, perhaps), but putting only one per target isn't an option while copy effects use targets[12]. */
		  instance->number_of_targets = 0;
		  int16_t* tgt = (int16_t*)(&instance->targets[0].player);

		  if (player == current_turn)
			for_each_creature_blocking_me(player, card, mark_creatures, (uint32_t)(&tgt), 0);

		  if (player == 1-current_turn)
			for_each_creature_blocked_by_me(player, card, mark_creatures, (uint32_t)(&tgt), 0);
		}

	  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
		{
		  card_instance_t* corpse = get_card_instance(instance->damage_source_player, instance->damage_source_card);
		  int16_t* tgt;
		  for (tgt = (int16_t*)(&corpse->targets[0].player); *tgt != -1; ++tgt)
			kill_card(1-player, *tgt, KILL_BURY);

		  event_flags |= EF_RERUN_DAMAGE_PREVENTION;
		}
	}

  return 0;
}

int card_aladdin(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			gain_control_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card,
															instance->targets[0].player, instance->targets[0].card,
															GCUS_CONTROLLED);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 1, 0, 0, 0, 2, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_aladdins_lamp(int player, int card, event_t event){

	if(trigger_condition == TRIGGER_REPLACE_CARD_DRAW && reason_for_trigger_controller == player
		&& affect_me(player, card) && !suppress_draw
	   ){
		int result = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
		if( result && (has_mana_for_activated_ability(player, card, MANACOST_X(2)) || ! duh_mode(player)) ){
			if (event == EVENT_TRIGGER){
				event_result = RESOLVE_TRIGGER_AI(player);
			}
			else if (event == EVENT_RESOLVE_TRIGGER){
					if( charge_mana_for_activated_ability(player, card, MANACOST_X(0)) ){
						charge_mana(player, COLOR_COLORLESS, -1);
						if( spell_fizzled != 1 ){
							tap_card(player, card);
							int amount = MIN(x_value, count_deck(player));
							test_definition_t this_test;
							default_test_definition(&this_test, TYPE_ANY);
							this_test.create_minideck = amount;
							this_test.no_shuffle = 1;
							int card_added = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
							amount--;
							if( amount ){
								put_top_x_on_bottom_in_random_order(player, amount);
							}
							put_on_top_of_deck(player, card_added);
						}
					}
			}
		}
	}

	return 0;
}

int card_aladdins_ring(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(8), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_ali_baba(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_WALL");
}

int card_ali_from_cairo(int player, int card, event_t event)
{
  // 0x4c48a0

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 48;

  card_instance_t* damage;
  if (event == EVENT_DAMAGE_REDUCTION
	  && event_result < 1
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->damage_target_player == player && damage->damage_target_card == -1)
	event_result = MIN(1, life[player]);	// If life already below 1, then neither increase nor decrease it

  if (event == EVENT_ATTACK_RATING && affect_me(player, card))
	ai_defensive_modifier += 24;

  if (event == EVENT_BLOCK_RATING && affect_me(player, card))
	ai_defensive_modifier -= 128;

  if (event == EVENT_SHOULD_AI_PLAY && in_play(player, card))
	{
	  if (player == AI)
		ai_modifier += 480;
	  else
		ai_modifier -= 480;
	}

  return 0;
}

int card_army_of_allah(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
		int p;
		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				card_data_t* card_d = get_card_data(p, count);
				if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
					card_instance_t *affected = get_card_instance(p, count);
					if( affected->state & STATE_ATTACKING ){
						pump_until_eot(player, card, p, count, 2, 0);
					}
				}
				count++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


// bird maiden --> vanilla

int card_bottle_of_suleiman(int player, int card, event_t event){
	/* Bottle of Suleiman	|4
	 * Artifact
	 * |1, Sacrifice ~: Flip a coin. If you win the flip, put a 5/5 colorless Djinn artifact creature token with flying onto the battlefield. If you lose the flip, ~ deals 5 damage to you. */

	if( event == EVENT_ACTIVATE && player == AI ){
		ai_modifier+=(5*(count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1)));
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( flip_a_coin(player, card) ){
			generate_token_by_id(player, card, CARD_ID_DJINN);
		}
		else{
			damage_player(player, 5, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(1), 0, NULL, NULL);
}

int card_brass_man(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		does_not_untap(player, card, event);

		if( is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 1) && current_turn == player &&
			upkeep_trigger_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
		  ){
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				untap_card(player, card);
			}
		}
	}

	return 0;
}

// camel --> hardcoded. Does the ability work at all ?

int card_bazaar_of_baghdad(int player, int card, event_t event){
	// 0x4A8520

	if(event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_ACTIVATE && player == AI ){
		const int *deck = deck_ptr[player];
		if( deck[10] == -1 ){ // If our deck is too thin, AI won't use the ability
			ai_modifier-=100;
		}
		else{
			deck = get_grave(player);
			if( deck[29] == -1 ){ // Convincing the AI to use the ability until we have at least 30 cards in grave
				ai_modifier+=25;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
		multidiscard(player, 3, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_city_in_a_bottle(int player, int card, event_t event){
#define IS_ARABIAN_NIGHTS(csvid) (((csvid) >= CARD_ID_ABU_JAFAR && (csvid) <= CARD_ID_YDWEN_EFREET)	\
								  || (csvid) == CARD_ID_BRASS_MAN || (csvid) == CARD_ID_DANCING_SCIMITAR)
	if( event == EVENT_RESOLVE_SPELL || event == EVENT_STATIC_EFFECTS ){
		int i, count;
		for (i = 0; i < 2; i++){
			for (count = active_cards_count[i] - 1; count >= 0; --count){
				if( !(i == player && count == card) && in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && !is_token(i, count)){
					int csvid = get_id(i, count);
					if (IS_ARABIAN_NIGHTS(csvid)){
						kill_card(i, count, KILL_SACRIFICE);
					}
				}
			}
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && !is_humiliated(player, card)){
		int csvid = get_id(affected_card_controller, affected_card);
		if (IS_ARABIAN_NIGHTS(csvid)){
			infinite_casting_cost();
		}
	}

	return 0;
#undef IS_ARABIAN_NIGHTS
}

int card_city_of_brass(int player, int card, event_t event){
	//0x4A86B0
	if (event == EVENT_TAP_CARD && affect_me(player, card)){
		damage_player(player, 1, player, card);
		return 0;
	} else {
		return mana_producer(player, card, event);
	}
}

int card_cyclone(int player, int card, event_t event)
{
  // 0x4308f0

  /* At the beginning of your upkeep, put a wind counter on ~, then sacrifice ~ unless you pay |G for each wind counter on it. If you pay, ~ deals damage equal
   * to the number of wind counters on it to each creature and each player. */

  if (event == EVENT_CAN_CAST)
	return 1;

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  add_counter(player, card, COUNTER_WIND);
	  int counters = count_counters(player, card, COUNTER_WIND);
	  ASSERT(counters > 0);
	  if( (IS_AI(player) && EXE_FN(int, 0x430C00, int)(counters))	// That function's specific to Cyclone, and returns nonzero if the AI thinks it's better to sacrifice
		  || !charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_GREEN, counters))
		kill_card(player, card, KILL_SACRIFICE);
	  else
		new_damage_all(player, card, ANYBODY, counters, NDA_PLAYER_TOO, NULL);
	}

  // AI
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 8 * (3 * (life[player] - life[1 - player]) + 6);

  if (event == EVENT_SHOULD_AI_PLAY && count_counters(player, card, COUNTER_WIND) > has_mana(player, COLOR_GREEN, 1))
	kill_card(player, card, KILL_DESTROY);

  return 0;
}

int card_cuombajj_witches(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.who_chooses = 1-player;
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				if( instance->targets[0].card > -1 ){
					add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
				}
				if( new_pick_target(&td1, "TARGET_CREATURE_OR_PLAYER", 1, 1) ){
					tap_card(player, card);
				}
				if( instance->targets[0].card > -1 ){
					remove_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( validate_target(player, card, &td, 0) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card);
		}
		if( validate_target(player, card, &td1, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 1, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;

}

// dancing scimitar --> vanilla

int card_dandan(int player, int card, event_t event){
	landhome(player, card, event, SUBTYPE_ISLAND);
	return 0;
}

static int effect_desert(int player, int card, event_t event){
	if ((trigger_condition == TRIGGER_END_COMBAT || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card)){
		if (event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		if (event == EVENT_RESOLVE_TRIGGER || event == EVENT_SHOULD_AI_PLAY){
			card_instance_t* instance = get_card_instance(player, card);
			if (instance->damage_target_player != -1){
				damage_creature(instance->damage_target_player, instance->damage_target_card, 1,
								instance->damage_source_player, instance->damage_source_card);
			}
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_desert2(int player, int card, event_t event){
	// original code : 0x4A8970

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = current_turn;
	td1.preferred_controller = current_turn;
	td1.required_state = TARGET_STATE_ATTACKING;

	if (player == HUMAN && ai_is_speculating != 1){	// Original interface
		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;

			int choice = 0;

			if (!paying_mana() && can_target(&td1)){
				load_text(0, "DESERT");
				char prompt[600];
				sprintf(prompt, " %s\n %s\n %s", text_lines[1], text_lines[2], text_lines[3]);
				choice = do_dialog(player, player, card, -1, -1, prompt, 0);
			}

			if (choice == 0){
				return mana_producer(player, card, event);
			} else if (choice == 1 && pick_target(&td1, "DESERT")){
				tap_card(player, card);
			} else {
				cancel = 1;
			}
		} else if (event == EVENT_RESOLVE_ACTIVATION && instance->number_of_targets == 1){
			if (valid_target(&td1)){
				create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &effect_desert, instance->targets[0].player, instance->targets[0].card);
			} else {
				cancel = 1;
			}
			get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
		} else {
			return mana_producer(player, card, event);
		}

		return 0;
	} else {	// AI interface
		if( trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) && reason_for_trigger_controller == player ){

			if (is_tapped(player, card) || is_animated_and_sick(player, card) || !can_use_activated_abilities(player, card) || current_turn == player){
				return 0;
			}

			int count, num_deserts = 1;
			// Count number of other activateable deserts
			for (count = 0; count < active_cards_count[player]; ++count){
				if (in_play(player, count) && count != card && get_id(player, count) == CARD_ID_DESERT
					&& !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_use_activated_abilities(player, card)	// activateable
					&& !(get_card_instance(player, count)->state & STATE_PROCESSING)){	// already activated
					++num_deserts;
				}
			}

			td1.toughness_requirement = num_deserts | TARGET_PT_LESSER_OR_EQUAL | TARGET_PT_INCLUDE_DAMAGE;
			if (!can_target(&td1)){
				return 0;
			}

			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			} else if (event == EVENT_RESOLVE_TRIGGER){
				if (pick_target(&td1, "TARGET_CREATURE")){
					instance->number_of_targets = 1;
					tap_card(player, card);
					damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
				}
			}
		}

		return mana_producer(player, card, event);
	}
}

int card_desert_nomads(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( affected->internal_card_id == damage_card ){
			if( affected->damage_target_player == player &&  affected->damage_target_card == card && affected->display_pic_csv_id == CARD_ID_DESERT ){
				affected->info_slot = 0;
			}
		}
	}
	return 0;
}

int card_desert_twister(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_diamond_valley(int player, int card, event_t event){
	// 0x4A8B40
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->targets[1].card);
	}
	return altar_basic(player, card, event, 4, TYPE_CREATURE);
}

int card_drop_of_honey(int player, int card, event_t event){
	// 0x430FA0

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		base_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if( can_target(&td) ){
			int i;
			int t_player = -1;
			int t_card = -1;
			int doubles_count = 1;
			int min_t = 100;
			int count;
			for(i=0; i<2; i++){
				count = active_cards_count[i]-1;
				while( count > -1 ){
					   if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						   if( get_power(i, count) < min_t ){
							   t_player = i;
							   t_card = count;
							   min_t = get_power(i, count);
							   doubles_count = 1;
							   instance->targets[doubles_count].player = t_player;
							   instance->targets[doubles_count].card = t_card;
						   }
						   else if( get_power(i, count) == min_t ){
									doubles_count++;
									instance->targets[doubles_count].player = i;
									instance->targets[doubles_count].card = count;
						   }


					   }
					   count--;
				}
			}

			if( doubles_count > 1 ){
				i=0;
				for(i=0; i<2; i++){
					count = 0;
					while( count < active_cards_count[i] ){
							if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
								card_instance_t *this = get_card_instance(i, count);
								this->state |= STATE_CANNOT_TARGET;
							}
							count++;
					}
				}
				int k;
				for(k=0; k<doubles_count; k++){
					card_instance_t *this = get_card_instance(instance->targets[k+1].player, instance->targets[k+1].card);
					this->state  &= ~STATE_CANNOT_TARGET;
				}
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					t_player = instance->targets[0].player;
					t_card = instance->targets[0].card;
					for(i=0; i<2; i++){
						count = 0;
						while( count < active_cards_count[i] ){
								if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
									card_instance_t *this = get_card_instance(i, count);
									if( this->state & STATE_CANNOT_TARGET ){
										this->state  &= ~STATE_CANNOT_TARGET;
									}
								}
								count++;
						}
					}
				}
			}

			if( t_player != -1 && t_card != -1 ){
				kill_card(t_player, t_card, KILL_BURY);
			}
		}
	}

	if( event == EVENT_STATIC_EFFECTS && ! count_subtype(ANYBODY, TYPE_CREATURE, -1) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_ebony_horse(int player, int card, event_t event)
{
  // 0x424560

#define DECL_TGT(td)	target_definition_t td;											\
						default_target_definition(player, card, &td, TYPE_CREATURE);	\
						td.allowed_controller = td.preferred_controller = player;		\
						td.required_state = TARGET_STATE_ATTACKING

#define ABILITY()		generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_ATTACKING_CREATURE")

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 3 * total_power_of_creatures_by_color[AI][COLOR_ANY];

  if (event == EVENT_CAN_ACTIVATE)
	{
	  DECL_TGT(td);
	  return ABILITY();
	}

  if (event == EVENT_ACTIVATE)
	{
	  DECL_TGT(td);
	  ABILITY();
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  DECL_TGT(td);
	  if (valid_target(&td))
		{
		  maze_of_ith_effect(player, card, instance->targets[0].player, instance->targets[0].card);
		  untap_card(instance->targets[0].player, instance->targets[0].card);
		}

	  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
	}

  return 0;
#undef DECL_TGT
#undef ABILITY
}

int card_el_hajjaj(int player, int card, event_t event){
	// 4C8070
	spirit_link_effect(player, card, event, player);
	return 0;
}

int card_elephant_graveyard(int player, int card, event_t event){
	// 0x4A8D40

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ELEPHANT;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		return mana_producer(player, card, event);
	}

#define CAN_REGEN	(!paying_mana() && (land_can_be_played & LCBP_REGENERATION)	\
					 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE"))
#define CAN_MANA	(mana_producer(player, card, EVENT_CAN_ACTIVATE))

	if( event == EVENT_CAN_ACTIVATE ){
		if (CAN_REGEN){
			return 99;
		}
		return CAN_MANA;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->info_slot = 0;
		if (CAN_REGEN){
			if (CAN_MANA){
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Regenerate target Elephant\n Cancel", 1);
			} else {
				choice = 1;
			}
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1){
				instance->number_of_targets = 0;
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "ELEPHANT_GRAVEYARD") ){
					instance->info_slot = 1;
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 67 ){
			if( can_be_regenerated(instance->targets[0].player, instance->targets[0].card) ){
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
#undef CAN_REGEN
#undef CAN_MANA
}

int card_erg_raiders(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL || (event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card)) ){
		get_card_instance(player, card)->info_slot = 66;
	}

	if( ! is_humiliated(player, card) && current_turn == player && eot_trigger(player, card, event) ){
		if( get_card_instance(player, card)->info_slot != 66 ){
			damage_player(player, 2, player, card);
		}
		get_card_instance(player, card)->info_slot = 0;
	}

	return 0;
}


static int erhnam_djinn_ai_pick_target(int player, int card, target_definition_t* td)
{
  //0x452bc0 (called only from exe version of Erhnam Djinn)
  int best_rating = INT_MIN;
  int best_card = -1;

  int c;
  for (c = 0; c < active_cards_count[HUMAN]; ++c)
	if (in_play(HUMAN, c) && would_validate_arbitrary_target(td, HUMAN , c))
	  {
		card_instance_t* instance = get_card_instance(HUMAN, c);
		int abils = instance->regen_status;
		int rating = 0;

		if (abils & get_hacked_walk(player, card, KEYWORD_FORESTWALK))
		  rating += 200;

		if (abils & KEYWORD_DEFENDER)
		  rating += 50;

		if (instance->state & STATE_TAPPED)
		  rating += 3;

		int iid = instance->internal_card_id;
		if (cards_data[iid].extra_ability & EA_ACT_ABILITY)
		  rating += 2;

		if (cards_data[iid].extra_ability & EA_MANA_SOURCE)
		  rating += 1;

		rating += num_bits_set(abils & ~KEYWORD_NONABILITIES);

		rating -= 2 * instance->power;

		if (rating > best_rating)
		  {
			best_rating = rating;
			best_card = c;
		  }
	  }

  return best_card;
}

static int effect_erhnam_djinn(int player, int card, event_t event)
{
  //0x4577b0
  /* Targets:
   * 0: permanent this is attached to (also in card_instance_t::damage_target_player/card)
   * 1.player: landwalk type to add
   * 1.card: 1 iff at least one cleanup phase has been seen */
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) && affected_card != -1)
	event_result |= instance->targets[1].player;

  if (current_phase == PHASE_UPKEEP && player == current_turn && instance->targets[1].card == 1)
	{
	  if (instance->damage_target_card != -1)
		{
		  card_instance_t* ench_inst = get_card_instance(instance->damage_target_player, instance->damage_target_card);
		  ench_inst->regen_status |= KEYWORD_RECALC_ABILITIES;
		}
	  instance->targets[1].card = 0;	// prevent re-entrance
	  kill_card(player, card, KILL_REMOVE);
	}

  if (event == EVENT_CLEANUP)
	instance->targets[1].card = 1;

  return 0;
}

int card_erhnam_djinn(int player, int card, event_t event)
{
  //0x452990
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = 1-player;
  td.required_subtype = SUBTYPE_WALL;
  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
  td.allow_cancel = 0;

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && player == AI && !(trace_mode & 2)
	  && basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_GREEN)] > 0)
	{
	  int tgt = erhnam_djinn_ai_pick_target(player, card, &td);
	  if (tgt >= 0)
		ai_modifier -= 96 * get_card_instance(HUMAN, tgt)->power / MAX(1, life[player]);
	}

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;

	  if (player == AI && !(trace_mode & 2))
		{
		  int tgt = erhnam_djinn_ai_pick_target(player, card, &td);
		  if (tgt >= 0)
			{
			  instance->targets[0].player = 1-player;
			  instance->targets[0].card = tgt;
			  instance->number_of_targets = 1;
			}
		}
	  else if (can_target(&td))
		pick_target(&td, "ERHNAM_DJINN");

	  if (instance->number_of_targets == 1)
		{
		  int leg = create_targetted_legacy_effect(player, card, effect_erhnam_djinn, instance->targets[0].player, instance->targets[0].card);
		  card_instance_t* legacy = get_card_instance(player, leg);
		  legacy->targets[1].player = get_hacked_walk(player, card, KEYWORD_FORESTWALK);
		}
	}

  if (event == EVENT_SHOULD_AI_PLAY
	  && current_phase == PHASE_DISCARD
	  && !basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_GREEN)])
	ai_modifier += 96;

  return 0;
}

int card_eye_for_an_eye(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			damage_player(target->damage_source_player, target->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
}

int card_fishliver_oil(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, get_hacked_walk(player, card, KEYWORD_ISLANDWALK), 0, 0, 0, 0);
}

int card_flying_carpet(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

// flying men --> vanilla

int card_ghazban_ogre(int player, int card, event_t event){
/*
Ghazbán Ogre English |G
Creature — Ogre 2/2
At the beginning of your upkeep, if a player has more life than each other player, the player with the most life gains control of
Ghazbán Ogre.
*/
	if( current_turn == player && upkeep_trigger_mode(player, card, event, (life[player] != life[1-player] ? RESOLVE_TRIGGER_MANDATORY : 0)) ){
		if( life[player] > life[1-player] ){
			gain_control_permanently(player, player, card);
		}
		if( life[player] < life[1-player] ){
			gain_control_permanently(1-player, player, card);
		}
	}

	return 0;
}

int card_giant_tortoise(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
			event_result += is_tapped(player, card) ? 0 : 3;
		}
	}
	return 0;
}

int check_for_guardian_beast_protection(int t_player, int t_card){
	if( is_what(t_player, t_card, TYPE_ARTIFACT) && ! is_what(t_player, t_card, TYPE_CREATURE) ){
		int i;
		for(i=0; i<active_cards_count[t_player]; i++ ){
			if( in_play(t_player, i) && get_id(t_player, i) == CARD_ID_GUARDIAN_BEAST && ! is_humiliated(t_player, i) && ! is_tapped(t_player, i) ){
				return 1;
			}
		}
	}
	return 0;
}

int card_guardian_beast(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
			! is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			if( ! is_tapped(player, card) ){
				indestructible(affected_card_controller, affected_card, event);
			}
		}
	}
	return 0;
}

int card_hasran_ogress(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card) ){
			int dmg = 3;
			if( has_mana(player, COLOR_COLORLESS, 2) ){
				charge_mana(player, COLOR_COLORLESS, 2);
				if( spell_fizzled != 1 ){
					dmg = 0;
				}
			}
			if( dmg ){
				damage_player(player, dmg, player, card);
			}
		}
	}
	return 0;
}

int card_hurr_jackal(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		cannot_regenerate_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
	}
	int mode = GAA_UNTAPPED | GAA_CAN_TARGET;
	mode |= player == AI ? GAA_REGENERATION : 0;

	return generic_activated_ability(player, card, event, mode, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int effect_ifh_biff_efreet(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player != -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_use_activated_abilities(p, c) ){
				int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 1, 0, 0);
				if( has_mana_multi(player, cless, 0, 0, 1, 0, 0) ){
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 1, 0, 0);
			charge_mana_multi(player, cless, 0, 0, 1, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			new_damage_all(p, c, 2, 1, NDA_PLAYER_TOO, &this_test);
		}

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

	}

	return 0;
}

int card_ifh_biff_efreet(int player, int card, event_t event ){
	//0x453880

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &effect_ifh_biff_efreet);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
		obliterate_card(1-player, fake);
		hand_count[1-player]--;
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(instance->parent_controller, instance->parent_card, 2, 1, NDA_PLAYER_TOO, &this_test);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_island_fish_jasconius(int player, int card, event_t event)
{
	// 0x4C8E40

	// ~ doesn't untap during your untap step.
	does_not_untap(player, card, event);

	// At the beginning of your upkeep, you may pay |U|U|U. If you do, untap ~.
	// Simplification: only ask to pay once, even if there's multiple upkeeps due to Paradox Haze
	if( trigger_condition == TRIGGER_UPKEEP && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER) && affect_me(player, card) &&
		! is_humiliated(player, card) && count_upkeeps(player) > 0 && current_turn == player)
	{
		if (event == EVENT_TRIGGER && has_mana(player, COLOR_BLUE, 3) ){
			event_result |= ((player == HUMAN && ai_is_speculating != 1) ? RESOLVE_TRIGGER_OPTIONAL
						 : (is_tapped(player, card) ? RESOLVE_TRIGGER_MANDATORY : 0));
		}

		if( event == EVENT_RESOLVE_TRIGGER && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_BLUE, 3) ){
			untap_card(player, card);
		}
	}

	// ~ can't attack unless defending player controls |Han Island.
	// When you control no |H1Islands, sacrifice ~.
	islandhome(player, card, event);

	return 0;
}

int card_island_of_wak_wak(int player, int card, event_t event){
	// 0x4A8F70

	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, -1, 0, 0, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_FLYING");
}

int card_jandors_saddlebags(int player, int card, event_t event)
{
  // 0x424dc0

#define DECL_TGT(td)	target_definition_t td;											\
						default_target_definition(player, card, &td, TYPE_CREATURE);	\
						td.preferred_controller = player

#define ABILITY()		generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE")

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 3 * total_power_of_creatures_by_color[AI][COLOR_ANY];

  if (event == EVENT_CAN_ACTIVATE)
	{
	  DECL_TGT(td);
	  return ABILITY();
	}

  if (event == EVENT_ACTIVATE)
	{
	  DECL_TGT(td);
	  ABILITY();
	  if (player == AI && cancel != 1)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (!(get_card_instance(instance->targets[0].player, instance->targets[0].card)->state & STATE_TAPPED))
			ai_modifier -= 24;
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  DECL_TGT(td);
	  if (valid_target(&td))
		untap_card(instance->targets[0].player, instance->targets[0].card);

	  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
	}

  return 0;
#undef DECL_TGT
#undef ABILITY
}

int card_jandors_ring(int player, int card, event_t event){

	if (event == EVENT_CAN_ACTIVATE){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.zone = TARGET_ZONE_HAND;
			this_test.state = STATE_JUST_DRAWED;
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_hand(player, count) && check_state(player, count, STATE_JUST_DRAWED) ){
						discard_card(player, count);
						tap_card(player, card);
						break;
					}
					count--;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_jeweled_bird(int player, int card, event_t event)
{
  // 0x44F560

  /* Jeweled Bird	|1
   * Artifact
   * Remove ~ from your deck before playing if you're not playing for ante.
   * |T: Put ~ into the ante. If you do, put all other cards you own from the ante into your graveyard, then draw a card. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* activation = get_card_instance(player, card);
	  int p = activation->parent_controller, c = activation->parent_card;
	  card_instance_t* instance = in_play(p, c);
      if (!instance)
        return 0;

	  int oid = instance->original_internal_card_id;
	  if (is_token(p, c))
		oid = -1;

	  if (ante_cards[player][1] != -1	// anteing at least two cards
		  || (ante_cards[player][0] != -1	// anteing at least one card, and...
			  && (is_token(p, c)	// It'll successfully go to ante, then disappear.
				  // From the exe version.  I don't know whether expansion_rarity is meaningful anymore, but enh.
				  || cards_ptr[oid]->expansion_rarity <= cards_ptr[cards_data[ante_cards[player][0]].id]->expansion_rarity)))
		ai_modifier += player == AI ? 48 : -48;

	  obliterate_card(p, c);

	  int i, prev_ante[16];
	  for (i = 0; i < 16; ++i)
		{
		  prev_ante[i] = ante_cards[player][i];
		  ante_cards[player][i] = -1;
		}
	  ante_cards[player][0] = oid;

	  for (i = 0; i < 16; ++i)
		if (prev_ante[i] != -1)
		  {
			// Pretend they came from exile - close enough.
			int pos = add_card_to_rfg(player, prev_ante[i]);
			from_exile_to_graveyard(player, pos);
		  }

	  draw_a_card(player);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_jihad(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.color = COLOR_TEST_WHITE;
			ai_modifier+=10*check_battlefield_for_special_card(player, card, player, 0, &this_test);
			this_test.type_flag = F1_NO_TOKEN;
			this_test.color = 1<<get_deck_color(player, 1-player);
			ai_modifier+=15*check_battlefield_for_special_card(player, card, 1-player, 0, &this_test);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( in_play(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player &&
		(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE))
	  ){
		switch( event ){
				case EVENT_POWER:
					event_result += 2;
					break;
				case EVENT_TOUGHNESS:
					event_result ++;
					break;
				default:
					break;
		}
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, instance->info_slot);
		this_test.type_flag = F1_NO_TOKEN;
		if( ! check_battlefield_for_special_card(player, card, 1-player, 0, &this_test) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_junun_efreet(int player, int card, event_t event){
	basic_upkeep(player, card, event, MANACOST_B(2));
	return 0;
}

int card_juzam_djinn(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(player, 1, player, card);
	}

	return 0;
}

int card_khabal_ghoul(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier-=(15-(creatures_dead_this_turn*5));
	}

	if( eot_trigger(player, card, event) ){
		add_1_1_counters(player, card, creatures_dead_this_turn);
	}
	return 0;
}

static const char* is_djinn_or_efreet(int who_chooses, int player, int card){
	if( has_subtype(player, card, SUBTYPE_DJINN) || has_subtype(player, card, SUBTYPE_EFREET) ){
		return NULL;
	}

	return "must be a Djinn or an Efreet";
}

int card_king_suleiman(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_djinn_or_efreet; // So we could ditch TARGET_SPECIAL_DJINN_OR_EFREET

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "KING_SULEIMAN");
}

int card_kird_ape(int player, int card, event_t event){
	// original code : 00453E50

	if( event == EVENT_POWER && affect_me(player, card) && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) ){
		event_result++;
	}

	if( event == EVENT_TOUGHNESS && affect_me(player, card) &&
		check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST)
	  ){
		event_result+=2;
	}

	return 0;
}

int card_library_of_alexandria(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( hand_count[player] == 7 && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int ai_choice = instance->info_slot = 0;
		if( hand_count[player] == 7 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Draw a card\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				instance->info_slot = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			draw_cards(player, 1);
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_magnetic_mountain(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLUE;
		this_test.state = STATE_TAPPED;
		ai_modifier += (5*check_battlefield_for_special_card(player, card, 1-player, 0, &this_test)-
						5*check_battlefield_for_special_card(player, card, player, 0, &this_test));
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		this_test.state = STATE_TAPPED;
		if( in_play(affected_card_controller, affected_card) && new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
			get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
		}
	}

	if( upkeep_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		this_test.state = STATE_TAPPED;
		int count = active_cards_count[current_turn]-1;
		int ai_best_choice = -1;
		while( count > -1 ){
				if( in_play(current_turn, count) && new_make_test_in_play(current_turn, count, -1, &this_test) ){
					int leave_tapped = 1;
					if( !has_mana(current_turn, COLOR_COLORLESS, 4) ){
						break;
					} else {
						int choice = 0;
						if( current_turn == AI ){
							if( ai_best_choice == -1 ){
								ai_best_choice = check_battlefield_for_special_card(player, card, current_turn, CBFSC_AI_MAX_VALUE, &this_test);
							}
							if( count != ai_best_choice ){
								choice = 1;
							}
						}
						if( current_turn == HUMAN ){
							choice = do_dialog(current_turn, current_turn, count, -1, -1, " Untap this\n Leave tapped", 0);
						}
						if( choice == 0 ){
							charge_mana(current_turn, COLOR_COLORLESS, 4);
							if( spell_fizzled != 1 ){
								leave_tapped = 0;
							}
						}
					}
					if( ! leave_tapped ){
						untap_card(current_turn, count);
						ai_best_choice = -1;
					}
				}
				count--;
		}
	}

	return 0;
}

int card_merchant_ship(int player, int card, event_t event){
	// 0x453FD0
	if( ! is_humiliated(player, card) ){
		islandhome(player, card, event);

		if( current_turn == player && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS && is_unblocked(player, card) ){
			gain_life(player, 2);
		}
	}

	return 0;
}

// metamorphosis --> uncodeable (mana usage restriction)

int card_mijae_djinn(int player, int card, event_t event){

	if( event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card) ){
		if( ! flip_a_coin(player, card) ){
			remove_state(player, card, STATE_ATTACKING);
			tap_card(player, card);
		}
	}

	return 0;
}

// Moorish Cavalry --> vanilla

int nafs_asp_legacy(int player, int card, event_t event){

	if( current_turn == get_card_instance(player, card)->targets[0].player && event == EVENT_DRAW_PHASE ){
		int dmg = 1;
		if( has_mana(current_turn, COLOR_COLORLESS, 1) ){
			charge_mana_while_resolving(player, card, EVENT_DRAW_PHASE, current_turn, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				dmg = 0;
			}
		}
		if( dmg ){
			damage_player(get_card_instance(player, card)->targets[0].player, dmg, player, card);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_nafs_asp(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
		int value = get_card_instance(player, card)->targets[1].player;
		int t_player = BYTE0(value) ? 0 : 1;
		int legacy = create_legacy_effect(player, card, &nafs_asp_legacy);
		get_card_instance(player, legacy)->targets[0].player = t_player;
	}

	return 0;
}

int card_oasis(int player, int card, event_t event){
	// 0x47A010

	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		if (player == AI){
			ai_modifier += 48;
		}
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY );
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot > 0 ){
				dmg->info_slot--;
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 0, 0,
									 &td, "TARGET_DAMAGE");
}

int check_pt_for_old_man(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int rm = 0;
		if( ! is_tapped(instance->targets[1].player, instance->targets[1].card) ){
			rm = 1;
		}
		if( ! in_play(instance->targets[1].player, instance->targets[1].card) ){
			rm = 1;
		}
		if( rm == 0 && get_power(instance->damage_target_player, instance->damage_target_card) >
			get_power(instance->targets[1].player, instance->targets[1].card)
		  ){
			rm =  1;
		}
		if( rm ){
			kill_card(player, instance->targets[2].card, KILL_REMOVE);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_old_man_of_the_sea(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = get_power(player, card) | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			int result = gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			int result2 = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &check_pt_for_old_man,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, result2);
			leg->token_status |= STATUS_INVISIBLE_FX;
			leg->targets[1].player = instance->parent_controller;
			leg->targets[1].card = instance->parent_card;
			leg->targets[2].card = result;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_oubliette(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		td.allowed_controller = 1-player;
		if( ! can_target(&td) ){
			ai_modifier-=25;
		}
		else{
			ai_modifier+=15;
		}
		td.allowed_controller = 2;
	}

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			exile_permanent_and_auras_attached(player, card, instance->targets[0].player, instance->targets[0].card,
				EPAAC_STORE_AURAS_RETURN_IF_SOURCE_LEAVES_PLAY | EPAAC_STORE_COUNTERS_RETURN_IF_SOURCE_LEAVES_PLAY | EPAAC_RETURN_TO_PLAY_TAPPED);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_piety2(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_BLOCKING;
		pump_creatures_until_eot(player, card, ANYBODY, 0, 0, 3, 0, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

const char* target_is_destroyed(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  // Normally, we'd just use td.required_state = TARGET_STATE_DESTROYED, but that will fail if the target can't regenerate.
  return (get_card_instance(player, card)->kill_code == KILL_DESTROY || get_card_instance(player, card)->kill_code == KILL_BURY) ? NULL : EXE_STR(0x786770);	//",destroyed"
}
const char* target_is_attached_to_a_land(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->damage_target_card >= 0 && instance->damage_target_player >= 0
	  && is_what(instance->damage_target_player, instance->damage_target_card, TYPE_LAND))
	return NULL;
  else
	return "not attached to a land";
}

int card_pyramids(int player, int card, event_t event)
{
  // 0x44faf0

  /* Pyramids	|6
   * Artifact
   * |2: Choose one - Destroy target Aura attached to a land; or the next time target land would be destroyed this turn, remove all damage marked on it
   * instead. */

  if (!IS_GAA_EVENT(event))
	return 0;

  const char* prompt;
  generic_activated_ability_flags_t mode;
  target_definition_t td;

  // As with the exe version, always activate for pseudo-regeneration during the regeneration phase, and always activate for destruction otherwise.
  if (land_can_be_played & LCBP_REGENERATION)
	{
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.preferred_controller = player;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_is_destroyed;

	  prompt = "TARGET_LAND";
	  mode = GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION;	// Not GAA_REGENERATION, which is incompatible with targeting

	  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (player == AI
			  && (instance->targets[0].player != AI || check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_REGENERATION)))
			ai_modifier -= 48;

		  /* An extremely stripped-down version of regenerate_target(), since this isn't quite the same as regeneration - it's not tapped; if it's a creature,
		   * it's not removed from combat; and it's not affected by can't-regenerate effects. */
		  card_instance_t* tgt = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		  tgt->kill_code = 0;
		  tgt->unknown0x14 = 0;
		  tgt->token_status &= ~STATUS_DYING;
		  tgt->damage_on_card = 0;

		  play_sound_effect(WAV_REGEN);
		}
	}
  else
	{
	  default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	  td.required_subtype = SUBTYPE_AURA;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_is_attached_to_a_land;

	  prompt = "Select target Aura attached to a land.";
	  mode = GAA_CAN_TARGET | GAA_LITERAL_PROMPT;

	  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (player == AI)
			ai_modifier += instance->targets[0].player == AI ? -48 : 48;
		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

  return generic_activated_ability(player, card, event, mode, MANACOST_X(2), 0, &td, prompt);
}

// repentant blacksmith --> vanilla

int ring_of_ma_ruf_legacy(int player, int card, event_t event){

	if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW
		&& reason_for_trigger_controller == player
		&& affect_me(player, card)
		&& !suppress_draw
		&& get_card_instance(player, card)->targets[1].player != 66
	   ){
		if (event == EVENT_TRIGGER){
			event_result = RESOLVE_TRIGGER_MANDATORY;
		}
		else if (event == EVENT_RESOLVE_TRIGGER){
				generic_wish_effect(player, card, TYPE_ANY, CARD_ID_BLACK_LOTUS);
				suppress_draw = 1;
				get_card_instance(player, card)->targets[1].player = 66;
				add_state(player, card, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_ring_of_ma_ruf(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION){
		create_legacy_effect(player, card, &ring_of_ma_ruf_legacy);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_RFG_ME, 5, 0, 0, 0, 0, 0, 0, NULL, NULL);
}

int rukh_legacy(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		// Well, I don't know if this has the SUBTYPE_BIRD, as per recent wording, but since it's in the hardcoded range of CSV IDs, it's better to use it
		generate_token_by_id(player, card, CARD_ID_RUKH);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_rukh_egg(int player, int card, event_t event){
	/* Rukh Egg	|3|R
	 * Creature - Bird 0/3
	 * When ~ dies, put a 4/4 |Sred Bird creature token with flying onto the battlefield at the beginning of the next end step. */

	if(  this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		create_legacy_effect(player, card, &rukh_legacy);
	}

	return 0;
}

int sandals_check(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event) &&
			in_play(instance->targets[1].player, instance->targets[1].card)
		  ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_sandals_of_abdallah(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, get_hacked_walk(instance->parent_controller, instance->parent_card, KEYWORD_ISLANDWALK), 0);
		int l2 = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &sandals_check,
												instance->targets[0].player, instance->targets[0].card);
		get_card_instance(player, l2)->targets[1].player = instance->parent_controller;
		get_card_instance(player, l2)->targets[1].card = instance->parent_card;
		get_card_instance(player, l2)->number_of_targets = 2;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_sandstorm(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		new_damage_all(player, card, ANYBODY, 1, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_serendib_djinn(int player, int card, event_t event){
	if( event == EVENT_UPKEEP_TRIGGER_ABILITY){
		if( player == HUMAN ){
			impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( has_subtype_by_id(cards_data[get_card_instance(player, card)->targets[3].card].id, SUBTYPE_ISLAND)){
				damage_player(player, 3, player, card);
			}
		}
		else{
			int result = impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, SUBTYPE_ISLAND, DOESNT_MATCH, 0, 0, 0, 0, -1, 0);
			if( ! result ){
				impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				damage_player(player, 3, player, card);
			}
		}
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_ABILITIES){
		if( ! check_battlefield_for_subtype(player, TYPE_LAND, -1) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

// serendib efree --> juzam djinn

// sharazad --> uncodeable

int card_sindbad(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_hand(player, count) && check_state(player, count, STATE_JUST_DRAWED) ){
					reveal_card(instance->parent_controller, instance->parent_card, player, count);
					if( ! is_what(player, count, TYPE_LAND) ){
						discard_card(player, count);
					}
					break;
				}
				count--;
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_singing_tree(int player, int card, event_t event){

	if (event == EVENT_ATTACK_RATING && affect_me(player, card))
		ai_defensive_modifier += 12;

	if (event == EVENT_BLOCK_RATING && affect_me(player, card))
		ai_defensive_modifier -= 12;

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION){
		if ( valid_target(&td)){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, -1, 0, 0, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_sorceress_queen(int player, int card, event_t event){
  // 0x4c6f60

  /* Sorceress Queen	|1|B|B
   * Creature - Human Wizard 1/1
   * |T: Target creature other than ~ becomes 0/2 until end of turn. */

	if (event == EVENT_ATTACK_RATING && affect_me(player, card))
		ai_defensive_modifier += 12;

	if (event == EVENT_BLOCK_RATING && affect_me(player, card))
		ai_defensive_modifier -= 12;

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_NOT_ME;

	if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t *instance = get_card_instance( player, card );
		set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 2, 0, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_ANOTHER_CREATURE");
}

// stone throwing devils --> vanilla

int card_unstable_mutation(int player, int card, event_t event)
{
  // 0x4bc7a0

  // At the beginning of the upkeep of enchanted creature's controller, put a -1/-1 counter on that creature.
  card_instance_t* instance = in_play(player, card);

  if (instance)
	upkeep_trigger_ability(player, card, event, instance->damage_target_player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY && instance)
	add_minus1_minus1_counters(instance->damage_target_player, instance->damage_target_card, 1);

  // Enchanted creature gets +3/+3.
  int rval = generic_aura(player, card, event, player, 3,3, 0,0, 0,0,0);
  if (event == EVENT_CAST_SPELL && affect_me(player, card) && cancel != 1)
	{
	  if (!instance)
		instance = get_card_instance(player, card);

	  if (is_sick(instance->targets[0].player, instance->targets[0].card)
		  || is_tapped(instance->targets[0].player, instance->targets[0].card))
		ai_modifier -= 99;
	}

  if (event == EVENT_SHOULD_AI_PLAY && instance && instance->damage_target_player >= 0 && instance->damage_target_card >= 0)
	{
	  card_instance_t* aff = get_card_instance(instance->damage_target_player, instance->damage_target_card);
	  aff->counter_power -= 2;
	  aff->counter_toughness -= 2;
	  aff->regen_status |= KEYWORD_RECALC_POWER | KEYWORD_RECALC_TOUGHNESS;
	  if (player == AI && current_turn == AI && !(aff->state & STATE_ATTACKED))
		ai_modifier -= 60;
	}

  return rval;
}

// war elephant --> vanilla

int card_wyluli_wolf(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_ydwen_efreet(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event) ){
		if( ! flip_a_coin(player, card) ){
			remove_state(player, card, STATE_BLOCKING);
			remove_state(player, card, STATE_UNKNOWN8000);
			pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

