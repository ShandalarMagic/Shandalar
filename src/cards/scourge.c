#include "manalink.h"

// Functions
int landcycling(int player, int card, event_t event, int colorless, int land_subtype1, int land_subtype2){

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "");
			this_test.subtype = land_subtype1;
			if (land_subtype2 <= 0){
				strcpy(this_test.message, get_hacked_land_text(-1, -1, "Select %a card.", land_subtype1));
			} else {
				strcpy(this_test.message, get_hacked_land_text(-1, -1, "Select %a or %s card.", land_subtype1, land_subtype2));
				this_test.sub2 = land_subtype2;
				this_test.subtype_flag = F2_MULTISUBTYPE;
			}
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}


// Cards
int card_ageless_sentinels(int player, int card, event_t event)
{
  // Also code for Elder Land Wurm (previously 0x4c8dd0).
  // Changed creature type handled in has_subtype().

  /* Ruling for Elder Land Wurm: 10/4/2004: Another effect can give Elder Land Wurm defender after it has lost it. Blocking again will cause it to lose that
   * instance of defender too. */
  if (current_turn == 1-player && blocking(player, card, event) && !is_humiliated(player, card) && check_for_ability(player, card, KEYWORD_DEFENDER))
	{
	  // Probably should look for other instances of the this-loses-defender effect and get rid of them first to reduce clutter.
	  int leg = pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_DEFENDER, SP_KEYWORD_DOES_NOT_END_AT_EOT);
	  if (leg != -1)
		get_card_instance(player, leg)->targets[4].player = 0;	// remove keyword

	  get_card_instance(player, card)->targets[5].player = 66;	// subtype hack, for Ageless Sentinels only
	}

  return 0;
}

int card_alpha_status(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(p, c) ){
			int i, j;
			for (i = 0; i < 2; ++i){
				for (j = 0; j < active_cards_count[i]; ++j){
					if (!(i == p && j == c) && in_play (i, j) && is_what(i, j, TYPE_CREATURE) && shares_creature_subtype(p, c, i, j)){
						event_result += 2;
					}
				}
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_ambush_commander(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_LAND);
	this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);

	card_instance_t *instance = get_card_instance(player, card);

	global_type_change(player, card, event, player, TYPE_CREATURE, &this_test, 1, 1, 0, SUBTYPE_ELF, COLOR_TEST_GREEN);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) &&
		can_target(&td)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_ELF, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_ELF, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
	}

	return 0;
}

int card_ancient_ooze(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1){
		int result = 0;
		int count = 0;
		while( count < active_cards_count[player] ){
				if( count != card && in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					result+=get_cmc(player, count);
				}
				count++;
		}
		event_result+=result;
	}
	return 0;
}

// aphetto runecaster --> vanilla

int card_ark_of_blight(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && can_target(&td)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_astral_steel(int player, int card, event_t event){

	/* Astral Steel	|2|W
	 * Instant
	 * Target creature gets +1/+2 until end of turn.
	 * Storm */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "TARGET_CREATURE", 1, 0) ){
					pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 1, 2);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bladewing_the_risen(int player, int card, event_t event){
	/* Bladewing the Risen	|3|B|B|R|R
	 * Legendary Creature - Zombie Dragon 4/4
	 * Flying
	 * When ~ enters the battlefield, you may return target Dragon permanent card from your graveyard to the battlefield.
	 * |B|R: Dragon creatures get +1/+1 until end of turn. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Dragon permanent card.");
		this_test.subtype = SUBTYPE_DRAGON;

		if (new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(2)){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card);
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, ANYBODY, SUBTYPE_DRAGON, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_BR(1,1), 0, NULL, NULL);
}

int card_bladewings_thrall(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON) ){
			event_result |= KEYWORD_FLYING;
		}
	}
	return 0;
}

int card_brain_freeze(int player, int card, event_t event){
	/* Brain Freeze	|1|U
	 * Instant
	 * Target player puts the top three cards of his or her library into his or her graveyard.
	 * Storm */

	if( ! is_unlocked(player, card, event, 11) ){ return 0; }

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "TARGET_PLAYER", 1, 0) ){
					mill(instance->targets[1].player, 3);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			mill(instance->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_break_asunder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			if( is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
}

int card_cabal_conditioning(int player, int card, event_t event)
{
  /* Cabal Conditioning	|6|B
   * Sorcery
   * Any number of target players each discard a number of cards equal to the highest converted mana cost among permanents you control. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allow_cancel = 3;	// Both Done and Cancel buttons

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  while (can_target(&td))
		{
		  if (!new_pick_target(&td, "TARGET_PLAYER", -1, 0))
			{
			  if (instance->targets[instance->number_of_targets].card == -1)	// picked cancel, not done
				cancel = 1;
			  return 0;
			}

		  if (td.allowed_controller == ANYBODY)	// was first choice
			{
			  if (IS_AI(player) && instance->targets[0].player == 1-player)
				break;	// AI never targets self
			  else
				td.allowed_controller = td.preferred_controller = 1 - instance->targets[0].player;
			}
		  else
			break;	// was second choice
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int highest_cmc = get_highest_cmc(player, TYPE_PERMANENT);
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  new_multidiscard(instance->targets[i].player, highest_cmc, 0, player);

	  kill_card(player, card, KILL_DESTROY );
	}

  return 0;
}

int card_cabal_interrogator(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && instance->info_slot > 0 ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);
			ec.cards_to_reveal = instance->info_slot;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_effect_coercion(&ec, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_CAN_SORCERY_BE_PLAYED, MANACOST_XB(-1, 1), 0, &td, "TARGET_PLAYER");
}

int card_call_to_the_grave(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.subtype = SUBTYPE_ZOMBIE;
	this_test.subtype_flag = 1;

	if( !is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			int count = count_upkeeps(current_turn);
			if(event == EVENT_TRIGGER && count > 0 && check_battlefield_for_special_card(player, card, current_turn, 0, &this_test) ){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					card_instance_t *instance= get_card_instance(player, card);
					card_data_t* card_d = &cards_data[ instance->internal_card_id ];
					int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
					while( count > 0 ){
							ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
							count--;
					}
			}
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, 1, TYPE_CREATURE, 0, SUBTYPE_ZOMBIE, 1, 0, 0, 0, 0, -1, 0);
	}

	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player){
		if(event == EVENT_TRIGGER && count_subtype(2, TYPE_CREATURE, -1) < 1 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_carbonize(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );
			if( instance->targets[0].card != -1 ){
				cannot_regenerate_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
				exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			}
			damage_creature_or_player(player, card, event, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_carrion_feeder(int player, int card, event_t event)
{
  /* Carrion Feeder	|B
   * Creature - Zombie 1/1
   * ~ can't block.
   * Sacrifice a creature: Put a +1/+1 counter on ~. */

  cannot_block(player, card, event);
  return generic_husk(player, card, event, TYPE_CREATURE, 101, 101, 0, 0);
}

int card_chartooth_cougar(int player, int card, event_t event)
{
  /* Chartooth Cougar	|5|R
   * Creature - Cat Beast 4/4
   * |R: ~ gets +1/+0 until end of turn.
   * |H2Mountaincycling |2 */

  if (IS_ACTIVATING_FROM_HAND(event))
	return landcycling(player, card, event, 2, SUBTYPE_MOUNTAIN, 0);

  return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,0);
}

int card_consumptive_goo(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			add_1_1_counter(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 2, 2, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_daru_spiritualist(int player, int card, event_t event)
{
  const target_t* targets = any_becomes_target(player, card, event, player, 0, TYPE_CREATURE, SUBTYPE_CLERIC, TYPE_EFFECT|TYPE_NONEFFECT, ANYBODY, RESOLVE_TRIGGER_MANDATORY);
  if (targets)
	for (; targets->player != -1; ++targets)
	  pump_until_eot(player, card, targets->player, targets->card, 0, 2);

  return 0;
}

int card_daru_warchief(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_SOLDIER) ){
			COST_COLORLESS--;
		}
	}

	boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 1, 2, 0, BCT_INCLUDE_SELF+BCT_CONTROLLER_ONLY);

	return 0;
}

int card_dawn_elemental(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				damage->info_slot = 0 ;
			}
		}
	}

	return 0;
}

int card_day_of_the_dragons(int player, int card, event_t event){
	/* Day of the Dragons	|4|U|U|U
	 * Enchantment
	 * When ~ enters the battlefield, exile all creatures you control. Then put that many 5/5 |Sred Dragon creature tokens with flying onto the battlefield.
	 * When ~ leaves the battlefield, sacrifice all Dragons you control. Then return the exiled cards to the battlefield under your control. */

	if( comes_into_play(player, card, event) ){
		int count, actually_a_count = 0;
		for (count = active_cards_count[player]-1; count >= 0; --count){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					exile_card_and_remember_it_on_exiledby(player, card, player, count);
					++actually_a_count;
				}
		}
		generate_tokens_by_id(player, card, CARD_ID_DRAGON, actually_a_count);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_DRAGON;
		new_manipulate_all(player, card, player, &this_test, KILL_BURY);

		int leg = 0;
		int idx = 0;
		int* loc;

		while ((loc = exiledby_find_any(player, card, &leg, &idx)) != NULL){
				int owner = (*loc & 0x80000000) ? 1 : 0;
				int iid = *loc & ~0x80000000;
				*loc = -1;
				int card_added = add_card_to_hand(player, iid);
				if( owner != player ){
					if( player == AI ){
						remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
					}
					else{
						add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
					}
				}
				put_into_play(player, card_added);
				if( owner != player ){
					create_targetted_legacy_effect(player, card, &empty, player, card_added);
				}
		}
	}

	return global_enchantment(player, card, event);
}

int card_decree_of_annihilation(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						int good = 1;
						if( is_what(i, count, TYPE_EFFECT) ){
							good = 0;
						}
						if( in_play(i, count) && is_planeswalker(i, count) ){
							good = 0;
						}
						if( in_play(i, count) && ! is_what(i, count, TYPE_PERMANENT) ){
							good = 0;
						}
						if( good == 1 ){
							kill_card(i, count, KILL_REMOVE);
						}
						count--;
				}
				count = count_graveyard(i)-1;
				while( count > -1 ){
						rfg_card_from_grave(i, count);
						count--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
			draw_cards(player, 1);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}
	else{
		return cycling(player, card, event, 5, 0, 0, 0, 2, 0);
	}

	return 0;
}

int card_decree_of_justice(int player, int card, event_t event){
	/* Decree of Justice	|X|X|2|W|W
	 * Sorcery
	 * Put X 4/4 |Swhite Angel creature tokens with flying onto the battlefield.
	 * Cycling |2|W
	 * When you cycle ~, you may pay |X. If you do, put X 1/1 |Swhite Soldier creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, MANACOST_XW(2, 1));
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			 draw_a_card(player);
			 int tokens = 0;
			 charge_mana(player, COLOR_COLORLESS, -1);
			 if(spell_fizzled != 1 ){
				 tokens = x_value;
			 }
			 if( tokens > 0 ){
				 generate_tokens_by_id(player, card, CARD_ID_SOLDIER, tokens);
			 }
	}
	else if(event == EVENT_CAN_CAST ){
			return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
			int amount = charge_mana_for_double_x(player, COLOR_COLORLESS);
			if( amount > 0 ){
				instance->info_slot = amount/2;
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		   int tokens = instance->info_slot;
		   if( tokens > 0 ){
			  generate_tokens_by_id(player, card, CARD_ID_ANGEL, tokens);
		   }
		   kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_decree_of_pain(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int result = new_manipulate_all(player, card, 2, &this_test, KILL_BURY);
			draw_cards(player, result);
			kill_card(player, card, KILL_DESTROY);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
			draw_cards(player, 1);
			pump_subtype_until_eot(player, card, 2, -1, -2, -2, 0, 0);
	}
	else{
		return cycling(player, card, event, 3, 2, 0, 0, 0, 0);
	}

	return 0;
}

int card_decree_of_savagery(int player, int card, event_t event){

	/* Decree of Savagery	|7|G|G
	 * Instant
	 * Put four +1/+1 counters on each creature you control.
	 * Cycling |4|G|G
	 * When you cycle ~, you may put four +1/+1 counters on target creature. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 4));
			kill_card(player, card, KILL_DESTROY);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
			draw_cards(player, 1);
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.preferred_controller = player;

			card_instance_t *instance = get_card_instance(player, card);
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 4);
			}
	}
	else{
		return cycling(player, card, event, 4, 0, 0, 2, 0, 0);
	}

	return 0;
}

int card_decree_of_silence(int player, int card, event_t event){

	/* Decree of Silence	|6|U|U
	 * Enchantment
	 * Whenever an opponent casts a spell, counter that spell and put a depletion counter on ~. If there are three or more depletion counters on ~, sacrifice it.
	 * Cycling |4|U|U
	 * When you cycle ~, you may counter target spell. */

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		add_counter(player, card, COUNTER_DEPLETION);
		if (count_counters(player, card, COUNTER_DEPLETION) >= 3){	// Part of the triggered ability, not separate
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

static int dimensional_breach_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, instance->targets[0].player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		char msg[100] = "Select a card to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
		int removed_array[36];
		int rac = 0;
		int i;
		for(i=1; i<19; i++){
			if( instance->targets[i].player != -1 && check_rfg(instance->targets[0].player, cards_data[instance->targets[i].player].id) ){
				removed_array[rac] = instance->targets[i].player;
				rac++;
			}
			if( instance->targets[i].card != -1 && check_rfg(instance->targets[0].player, cards_data[instance->targets[i].card].id) ){
				removed_array[rac] = instance->targets[i].card;
				rac++;
			}
		}
		if( rac == 0 ){
			kill_card(player, card, KILL_REMOVE);
			return 0;
		}
		int selected = -1;
		if( player == HUMAN ){
			selected = select_card_from_zone(instance->targets[0].player, instance->targets[0].player, removed_array, rac, 0, AI_MAX_CMC, -1, &this_test);
		}
		else{
			this_test.type = TYPE_ENCHANTMENT;
			this_test.type_flag = DOESNT_MATCH;
			selected = select_card_from_zone(instance->targets[0].player, instance->targets[0].player, removed_array, rac, 0, AI_MAX_CMC, -1, &this_test);
			if( selected == -1 ){
				this_test.type = TYPE_ANY;
				this_test.type_flag = MATCH;
				selected = select_card_from_zone(instance->targets[0].player, instance->targets[0].player, removed_array, rac, 0, AI_MAX_CMC, -1, &this_test);
			}
		}
		for(i=1; i<19; i++){
			if( instance->targets[i].player == removed_array[selected] ){
				instance->targets[i].player = -1;
				break;
			}
			if( instance->targets[i].card == removed_array[selected] ){
				instance->targets[i].card = -1;
				break;
			}
		}
		int card_added = add_card_to_hand(instance->targets[0].player, removed_array[selected]);
		put_into_play(instance->targets[0].player, card_added);
	}

	return 0;
}

int card_dimensional_breach(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			int legacy = create_legacy_effect(player, card, dimensional_breach_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = i;
			int pos = 1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && ! is_token(i, count) ){
						if( pos < 19 ){
							int iid = get_original_internal_card_id(i, count);
							if( leg->targets[pos].player == -1 ){
								leg->targets[pos].player = iid;
							}
							else if( leg->targets[pos].card == -1 ){
									leg->targets[pos].card = iid;
									pos++;
							}
						}
						kill_card(i, count, KILL_REMOVE);
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_dispersal_shield(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( get_cmc(card_on_stack_controller, card_on_stack) <= get_highest_cmc_nonland(player) ){
				return result;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_dragons_breath(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player >= 0 ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_R(1)) ){
			return 1;
		}

		if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, MANACOST_R(1));
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(player, instance->parent_card, instance->damage_target_player, instance->damage_target_card, 1, 0);
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_HASTE, 0, 0, 0);
}

int card_dragons_fangs(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 1, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_dragon_mage(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		APNAP(p,{
				discard_all(p);
				draw_cards(p, 7);
				};
		);
	}
	return 0;
}

int card_dragons_scales(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 2, 0, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_dragons_shadow(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 0, 0, SP_KEYWORD_FEAR, 0, 0, 0);
}

int card_dragon_tyrant(int player, int card, event_t event){

	basic_upkeep(player, card, event, 0, 0, 0, 0, 4, 0);

	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_dragons_wings(int player, int card, event_t event){
	/* Dragon Wings	|1|U
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature has flying.
	 * Cycling |1|U
	 * When a creature with converted mana cost 6 or greater enters the battlefield, you may return ~ from your graveyard to the battlefield attached to that creature. */

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, MANACOST_XU(1, 1));
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_a_card(player);
	}
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_dragonspeaker_shaman(int player, int card, event_t event){

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player){
	   if( has_subtype(affected_card_controller, affected_card, SUBTYPE_DRAGON) ){
		  COST_COLORLESS-=2;
	   }
	}

	return 1;
}

int card_dragonstorm(int player, int card, event_t event){
	/*
	  Dragonstorm |8|R
	  Sorcery
	  Search your library for a Dragon permanent card and put it onto the battlefield. Then shuffle your library.
	  Storm (When you cast this spell, copy it for each spell cast before it this turn.)
	*/
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			int amount = get_storm_count();

			test_definition_t test;
			new_default_test_definition(&test, TYPE_PERMANENT, "Select a Dragon permanent card.");
			test.subtype = SUBTYPE_DRAGON;

			int i;
			for(i=0;i<amount;i++){
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "Select a Dragon permanent card.");
		test.subtype = SUBTYPE_DRAGON;

		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &test);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_edgewalker(int player, int card, event_t event){

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_CLERIC) ){
			COST_BLACK--;
			COST_WHITE--;
		}
	}

	return 1;
}

int card_elvish_aberration(int player, int card, event_t event)
{
  /* Elvish Aberration	|5|G
   * Creature - Elf Mutant 4/5
   * |T: Add |G|G|G to your mana pool.
   * |H2Forestcycling |2 */

  if (IS_ACTIVATING_FROM_HAND(event))
	return landcycling(player, card, event, 2, SUBTYPE_FOREST, 0);

  return mana_producing_creature(player, card, event, 0, COLOR_GREEN, 3);
}

int card_eternal_dragon(int player, int card, event_t event){

	/* Eternal Dragon	|5|W|W
	 * Creature - Dragon Spirit 5/5
	 * Flying
	 * |3|W|W: Return ~ from your graveyard to your hand. Activate this ability only during your upkeep.
	 * |H2Plainscycling |2 */

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		if( has_mana_multi(player, 3, 0, 0, 0, 0, 2) ){
			int choice = do_dialog(player, player, card, -1, -1," Return Eternal Dragon to hand\n Pass\n", 0);
			if( choice == 0 ){
				if (charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XW(3,2))){
					card_instance_t* instance = get_card_instance(player, card);
					instance->state &= ~STATE_INVISIBLE;
					hand_count[player]++;
					return -1;
				}
			}
			return -2;
		}
		else{
			return -2;
		}
	}

	return landcycling(player, card, event, 2, SUBTYPE_PLAINS, 0);
}

int card_fierce_empath(int player, int card, event_t event){

	char msg[100] = "Select a creature card with CMC 6 or more.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.cmc = 5;
	this_test.cmc_flag = 2;

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
	}

	return 0;
}

int card_final_punishment(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			lose_life(instance->targets[0].player, get_trap_condition(instance->targets[0].player, TRAP_DAMAGE_TAKEN));
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_forgotten_ancient(int player, int card, event_t event){

	/* Forgotten Ancient	|3|G
	 * Creature - Elemental 0/3
	 * Whenever a player casts a spell, you may put a +1/+1 counter on ~.
	 * At the beginning of your upkeep, you may move any number of +1/+1 counters from ~ onto other creatures. */

	if( specific_spell_played(player, card, event, 2, RESOLVE_TRIGGER_AI(player), 0,0, 0,0, 0,0, 0,0, -1,0) ){
		add_1_1_counter(player, card);
	}

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && count_1_1_counters(player, card) > 0 ){
		upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		base_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		instance->number_of_targets = 0;
		while (count_1_1_counters(player, card) > 0 && pick_target(&td, "TARGET_CREATURE")){
			move_counters(instance->targets[0].player, instance->targets[0].card, player, card, COUNTER_P1_P1, 1);
			instance->number_of_targets = 0;
		}
	}
	return 0;
}

int card_form_of_the_dragon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_ATTACK_LEGALITY && affected_card_controller != player
		&& !(get_card_instance(affected_card_controller, affected_card)->regen_status & KEYWORD_FLYING)){
		event_result = 1;
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 5);
			instance->number_of_targets = 1;
		}
	}
	if( life[player] != 5 && eot_trigger(player, card, event) ){
		set_life_total(player, 5);
	}

	return global_enchantment(player, card, event);
}

static int frontline_strategist_legacy(int player, int card, event_t event){
	card_instance_t* damage = combat_damage_being_prevented(event);
	if( damage &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		!has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_SOLDIER)
	  ){
		damage->info_slot = 0;
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_frontline_strategist(int player, int card, event_t event){
	if(event == EVENT_TURNED_FACE_UP ){
		create_legacy_effect(player, card, &frontline_strategist_legacy);
	}
	return morph(player, card, event, 0, 0, 0, 0, 0, 1);
}

static int gilded_light_legacy(int player, int card, event_t event){
	give_shroud_to_player(player, card, event);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_gilded_light(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &gilded_light_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
}

int card_goblin_war_strike(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			damage_player(instance->targets[0].player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_GOBLIN), player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_goblin_warchief(int player, int card, event_t event)
{
  /* Goblin Warchief	|1|R|R
   * Creature - Goblin 2/2
   * Goblin spells you cast cost |1 less to cast.
   * Goblin creatures you control have haste. */

  if (event == EVENT_MODIFY_COST_GLOBAL
	  && affected_card_controller == player && has_creature_type(affected_card_controller, affected_card, SUBTYPE_GOBLIN))
	--COST_COLORLESS;

  boost_subtype(player, card, event, SUBTYPE_GOBLIN, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

  return 0;
}

int card_guilty_conscience(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( damage_dealt_by_me_arbitrary(instance->damage_target_player, instance->damage_target_card, event, DDBM_REPORT_DAMAGE_DEALT, player, card) ){
			damage_creature(instance->damage_target_player, instance->damage_target_card, instance->targets[16].player, player, card);
			instance->targets[16].player = 0;
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_hindering_touch(int player, int card, event_t event ){
	/* Hindering Touch	|3|U
	 * Instant
	 * Counter target spell unless its controller pays |2.
	 * Storm */

	if( event == EVENT_CAN_CAST ){
		return card_counterspell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		get_card_instance(player, card)->info_slot = 0;
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			get_card_instance(player, card)->info_slot = get_storm_count();
		}
		return card_counterspell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 2 + (2 * get_card_instance(player, card)->info_slot);
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hunting_pack(int player, int card, event_t event){
	/* Hunting Pack	|5|G|G
	 * Instant
	 * Put a 4/4 |Sgreen Beast creature token onto the battlefield.
	 * Storm */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 4;
			token.tou = 4;
			token.color_forced = COLOR_TEST_GREEN;
			token.qty = get_storm_count();
			generate_token(&token);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = 4;
		token.tou = 4;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_karona_false_god(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	haste(player, card, event);

	// At the beginning of each player's upkeep, that player untaps ~ and gains control of it.
	if( current_turn != player && upkeep_trigger(player, card, event) ){
		untap_card(player, card);
		give_control_of_self(player, card);
	}

	// Whenever ~ attacks, creatures of the creature type of your choice get +3/+3 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		pump_subtype_until_eot(player, card, ANYBODY, select_a_subtype_full_choice(player, card, player, NULL, 2), 3, 3, 0,0);
	}

	return 0;
}

int card_krosan_drover(int player, int card, event_t event){
	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && get_cmc(affected_card_controller, affected_card) > 5){
			COST_COLORLESS-=2;
		}
	}
	return 0;
}

int card_krosan_warchief(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_BEAST) ){
			COST_COLORLESS--;
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && ( land_can_be_played & LCBP_REGENERATION) && is_subtype_dead(player, SUBTYPE_BEAST, 0, 0) ){
			return 0x63;
		}
	}

	if( event == EVENT_ACTIVATE  ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			if( select_target(player, card, &td, "Select a Beast to regenerate", &(instance->targets[0])) ){
				instance->number_of_targets = 1;
				if( ! has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_BEAST) ){
					spell_fizzled = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		regenerate_target(instance->targets[0].player, instance->targets[0].card);
	}

	return 0;
}

static int effect_lethal_vapors(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player != -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_use_activated_abilities(p, c) ){
				int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
				if( has_mana(player, COLOR_COLORLESS, cless) ){
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( player == AI && count_subtype(player, TYPE_CREATURE, -1)-count_subtype(1-player, TYPE_CREATURE, -1) >  1 ){
				ai_modifier-=25;
			}
			int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
			charge_mana(player, COLOR_COLORLESS, cless);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			skip_next_turn(p, c, player);
			kill_card(p, c, KILL_DESTROY);
		}

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_lethal_vapors(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST )
		return 1;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) )
		if( count_subtype(player, TYPE_CREATURE, -1)-count_subtype(1-player, TYPE_CREATURE, -1) >  1 )
			ai_modifier+=25;

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &effect_lethal_vapors);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
		obliterate_card(1-player, fake);
		hand_count[1-player]--;
	}

	if( event == EVENT_ACTIVATE )
		if( player == AI && count_subtype(player, TYPE_CREATURE, -1)-count_subtype(1-player, TYPE_CREATURE, -1) >  1 )
			ai_modifier-=25;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		skip_next_turn(player, instance->parent_card, player);
		kill_card(player, instance->parent_card, KILL_DESTROY);
	}

	if( specific_cip(player, card, event, 2, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) )
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_long_term_plans(int player, int card, event_t event){

	/* Long-Term Plans	|2|U
	 * Instant
	 * Search your library for a card, shuffle your library, then put that card third from the top. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, 0);
		int iid = -1;
		int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			iid = deck_ptr[player][selected];
			remove_card_from_deck(player, selected);
		}
		shuffle(player);
		if( iid != -1 ){
			put_iid_under_the_first_x_cards_of_library(player, iid, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int minds_desire_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if (event == EVENT_SET_LEGACY_EFFECT_NAME && instance->targets[0].player > -1 ){
		scnprintf(set_legacy_effect_name_addr, 51, "%s", cards_ptr[cards_data[instance->targets[0].player].id]->name);
	}
	if( instance->targets[0].card > -1 && event == EVENT_CAN_ACTIVATE ){
		int rval = can_legally_play_iid(player, instance->targets[0].card);
		if( rval == 99 && (cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) ){
			return 99;
		}
		if( rval && !(cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) && ! can_sorcery_be_played(player, event) ){
			rval = 0;
		}
		return rval;
	}
	if( event == EVENT_ACTIVATE ){
		char buffer[500];
		card_ptr_t* c = cards_ptr[ cards_data[instance->targets[0].card].id ];
		scnprintf(buffer, 500, " Play %s\n Cancel", c->name);
		int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
		if( choice == 0 ){
			int iid = instance->targets[0].card;
			instance->targets[0].card = -1;
			play_card_in_exile_for_free(player, player, cards_data[iid].id);
			kill_card(player, card, KILL_REMOVE);
			cant_be_responded_to = 1;
		}
		if( choice == 1 ){
			spell_fizzled = 1;
		}
	}
	if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static int create_minds_desire_legacy(int player, int card){
	int *deck = deck_ptr[player];
	int iid = deck[0];
	rfg_top_card_of_deck(player);
	int legacy = create_legacy_activate(player, card, &minds_desire_legacy);
	card_instance_t *instance = get_card_instance( player, legacy);
	instance->targets[0].card = iid;
	instance->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
	return legacy;
}


int card_minds_desire(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 28) ){ return 0; }

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			int count = get_storm_count();
			while( count > 0 ){
					create_minds_desire_legacy(player, card);
					count--;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_minds_desire_legacy(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_mischievous_quanar(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  // |3|U|U: Turn ~ face down.
  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XU(3,2)))
	return 1;

  if (event == EVENT_ACTIVATE)
	charge_mana_for_activated_ability(player, card, MANACOST_XU(3,2));

  if (event == EVENT_RESOLVE_ACTIVATION)
	turn_face_down(instance->parent_controller, instance->parent_card);

  // Morph |1|U|U
  // When ~ is turned face up, copy target instant or sorcery spell. You may choose new targets for that copy.
  if (event == EVENT_CAN_UNMORPH)
	{
	  if (!has_mana_multi(player, MANACOST_XU(1,2)))
		return 0;

	  int tc = instance->targets[9].card = activate_twincast(player, card, EVENT_CAN_ACTIVATE, NULL, NULL);
	  return tc ? tc : 1;
	}

  int mf = morph(player, card, event, MANACOST_XU(1,2));

  if (event == EVENT_UNMORPH)
	{
	  if (cancel != 1 && card_on_stack_controller != -1 && instance->targets[9].card == 99)
		{
		  activate_twincast(player, card, EVENT_ACTIVATE, NULL, NULL);
		  if (cancel == 1)
			{
			  cancel = 0;
			  instance->targets[9].card = 0;
			  ai_modifier -= 96;
			}
		}
	  else
		ai_modifier -= 96;
	}

  if (event == EVENT_TURNED_FACE_UP)
	{
	  if (instance->targets[9].card == 99)
		activate_twincast(player, card, EVENT_RESOLVE_ACTIVATION, NULL, NULL);
	  cancel = 0;
	}

  return mf;
}

int card_misguided_rage(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_nefashu(int player, int card, event_t event)
{
	// Whenever ~ attacks, up to five target creatures each get -1/-1 until end of turn.
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_STORE_IN_INFO_SLOT, player, card)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		int trgs = 0;
		while( trgs < 5 && can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for (i = 0; i < instance->number_of_targets; ++i){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, -1, -1);
		}
	}

	return 0;
}

int card_noble_templar(int player, int card, event_t event){
	vigilance(player, card, event);
	return landcycling(player, card, event, 2, SUBTYPE_PLAINS, 0);
}

int card_proteus_machine(int player, int card, event_t event){
	if(event == EVENT_TURNED_FACE_UP ){
		add_a_subtype(player, card, select_a_subtype(player, card));
	}
	return morph(player, card, event, 0, 0, 0, 0, 0, 0);
}

int card_putrid_raptor(int player, int card, event_t event)
{
  // Morph - Discard a Zombie card.
#define DECL_TEST(test)												\
  test_definition_t test;											\
  new_default_test_definition(&test, 0, "Select a Zombie card.");	\
  test.subtype = SUBTYPE_ZOMBIE;									\
  test.zone = TARGET_ZONE_HAND

  if (event == EVENT_CAN_UNMORPH)
	{
	  DECL_TEST(test);
	  return check_battlefield_for_special_card(player, card, player, 0, &test);
	}

  if (event == EVENT_UNMORPH)
	{
	  DECL_TEST(test);
	  int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
	  if (selected == -1)
		spell_fizzled = 1;
	  else
		discard_card(player, selected);

	  return 0;
	}

  return morph(player, card, event, MANACOST_X(0));
#undef DECL_TEST
}

int card_pyrostatic_pillar(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, 4, 3) ){
		damage_player(instance->targets[1].player, 2, player, card);
	}
	return global_enchantment(player, card, event);
}

int card_raven_guild_master(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int max = 10;
		if( count_deck(1-player) < max ){
			max = count_deck(1-player) ;
		}
		while( max > 0 ){
				rfg_top_card_of_deck(1-player);
				max--;
		}
	}
	return morph(player, card, event, 2, 0, 2, 0, 0, 0);
}

int card_reaping_the_graves(int player, int card, event_t event){

	/* Reaping the Graves	|2|B
	 * Instant
	 * Return target creature card from your graveyard to your hand.
	 * Storm */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");

		int swap[500][2];
		int max = 1;
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			max += get_storm_count();
		}
		max = MIN(19, max);
		int pos = 0;
		while( max > 0 ){
				int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, pos);
				if( selected != -1  ){
					swap[pos][0] = selected;
					swap[pos][1] = turn_card_in_grave_face_down(player, selected);
					pos++;
					max--;
				}
				else{
					break;
				}
		}
		if( pos > 0 ){
			int q;
			for (q = 0; q < pos; q++){
				turn_card_in_grave_face_up(player, swap[q][0], swap[q][1]);
			}
			if( pos > 1 ){
				int i, k;
				for(i=0; i<pos; i++){
					for(k=i; k<(pos-1); k++){
						if( instance->targets[k].player > instance->targets[k+1].player ){
							SWAP(instance->targets[k], instance->targets[k+1]);	// struct copy
						}
					}
				}
			}
			instance->info_slot = pos;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=(instance->info_slot-1); i>-1; i--){
			int result = validate_target_from_grave(player, card, player, i);
			if( result != -1 ){
				const int *grave = get_grave(player);
				add_card_to_hand(player, grave[result]);
				remove_card_from_grave(player, result);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_riptide_survivor(int player, int card, event_t event){
	if(event == EVENT_TURNED_FACE_UP ){
		multidiscard(player, 2, 0);
		draw_cards(player, 3);
	}
	return morph(player, card, event, 1, 0, 2, 0, 0, 0);
}

int card_root_elemental(int player, int card, event_t event){
	if(event == EVENT_TURNED_FACE_UP ){
		char buffer[100];
		scnprintf(buffer, 100, "Select a Creature card.");
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return morph(player, card, event, 5, 0, 0, 2, 0, 0);
}

int card_rush_of_knowledge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, get_highest_cmc_nonland(player));
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_shoreline_ranger(int player, int card, event_t event){
	return landcycling(player, card, event, 2, SUBTYPE_ISLAND, 0);
}

int card_siege_gang_commander(int player, int card, event_t event){
	/* Siege-Gang Commander	|3|R|R
	 * Creature - Goblin 2/2
	 * When ~ enters the battlefield, put three 1/1 |Sred Goblin creature tokens onto the battlefield.
	 * |1|R, Sacrifice a Goblin: ~ deals 2 damage to target creature or player. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		if( can_target(&td) && has_mana_for_activated_ability(player, card,  1, 0, 0, 0, 1, 0 ) ) {
			return 1;
		}
		return 0;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0 );
		if( spell_fizzled != 1 ){
			if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
				}
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	if( comes_into_play(player, card, event ) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 3);
	}

	return 0;
}

int card_skulltap(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sliver_overlord(int player, int card, event_t event)
{
  /* Sliver Overlord	|W|U|B|R|G
   * Legendary Creature - Sliver Mutant 7/7
   * |3: Search your library for a Sliver card, reveal that card, and put it into your hand. Then shuffle your library.
   * |3: Gain control of target Sliver. */

  if( ! is_unlocked(player, card, event, 29) ){ return 0; }

  check_legend_rule(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !CAN_ACTIVATE(player, card, MANACOST_X(3)))
		return 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.required_subtype = SUBTYPE_SLIVER;

	  target_definition_t td_opp;
	  default_target_definition(player, card, &td_opp, TYPE_PERMANENT);
	  td_opp.required_subtype = SUBTYPE_SLIVER;
	  td_opp.allowed_controller = 1-player;

	  enum
	  {
		CHOICE_TUTOR = 1,
		CHOICE_CONTROL
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Search for a Sliver", 1, 1,
						"Control a Sliver", can_target(&td), IS_AI(player) && can_target(&td_opp) ? 2 : -1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  if (charge_mana_for_activated_ability(player, card, MANACOST_X(3)))
			switch (choice)
			  {
				case CHOICE_TUTOR:
				  break;

				case CHOICE_CONTROL:
				  pick_next_target_noload(&td, "Select target Sliver.");
				  break;
			  }
		}
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_TUTOR:
			  ;test_definition_t test;
			  new_default_test_definition(&test, TYPE_PERMANENT, "Select a Sliver card.");
			  test.subtype = SUBTYPE_SLIVER;
			  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
			  break;

			case CHOICE_CONTROL:
			  if (valid_target(&td))
				gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			  break;
		  }
	}

  return slivercycling(player, card, event);
}

int card_soul_collector(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance(player, card);

	if( sengir_vampire_trigger(player, card, event, 2) ){
		int i;
		for(i=2; i<instance->targets[11].card + 2; i++){
			seek_grave_for_id_to_reanimate(player, card, instance->targets[i].player, instance->targets[i].card, REANIMATE_DEFAULT);
		}
		instance->targets[11].card = 2;
	}

	return morph(player, card, event, 0, 3, 0, 0, 0, 0);
}

int card_sprouting_vines(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
		this_test.subtype = SUBTYPE_BASIC;
		int i, storm = 0;
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			storm = get_storm_count();
		}
		for (i = 0; i < storm; ++i){
			// Deliberately shuffle after each search
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
		this_test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_stifle(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return can_counter_activated_ability(player, card, event, NULL);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  get_card_instance(player, card)->number_of_targets = 0;
	  return cast_counter_activated_ability(player, card, 0);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  resolve_counter_activated_ability(player, card, NULL, 0);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_sulfuric_vortex(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(current_turn, 2, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_temple_of_the_false_god(int player, int card, event_t event){

	/*
		if(event == EVENT_CAN_ACTIVATE && result == 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		if( target_available(player, card, &td) < 5 ){
			return 0;
		}
	}
	*/
	if( event == EVENT_RESOLVE_SPELL	// to play sound
		|| count_permanents_by_type(player, TYPE_LAND) > 4 ){
		return two_mana_land(player, card, event, COLOR_COLORLESS, COLOR_COLORLESS);
	}
	return 0;
}

int card_tendrils_of_agony(int player, int card, event_t event)
{
	/* Tendrils of Agony	|2|B|B
	 * Sorcery
	 * Target player loses 2 life and you gain 2 life.
	 * Storm */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "TARGET_PLAYER", 1, 0) ){
					lose_life(instance->targets[1].player, 2);
					gain_life(player, 2);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_twisted_abomination(int player, int card, event_t event)
{
  /* Twisted Abomination	|5|B
   * Creature - Zombie Mutant 5/3
   * |B: Regenerate ~.
   * |H2Swampcycling |2 */

  if (IS_ACTIVATING_FROM_HAND(event))
	return landcycling(player, card, event, 2, SUBTYPE_SWAMP, 0);

  return regeneration(player, card, event, MANACOST_B(1));
}

int card_unburden(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
}

int card_undead_warchief(int player, int card, event_t event){
	if(!has_creature_type(affected_card_controller, affected_card, SUBTYPE_ZOMBIE )){
		return 0;
	}
	if( affected_card_controller != player){
		return 0;
	}
	if( ! in_play(player, card) ){
		return 0;
	}

	if(event == EVENT_MODIFY_COST_GLOBAL ){
		COST_COLORLESS--;
	}
	else if( event == EVENT_POWER ){
		event_result += 2;
	}
	else if(event == EVENT_TOUGHNESS ){
		event_result++;
	}
	return 0;
}

int card_unspeakable_symbol(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_pay_life(player, 3);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			lose_life(player, 3);
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_vengeful_dead(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(1-player, 1);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_ZOMBIE;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(1-player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

   return 0;
}

int card_wing_shards(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = current_turn;
	td.preferred_controller = current_turn;
	td.who_chooses = current_turn;
	td.illegal_abilities = 0;
	td.required_state = TARGET_STATE_ATTACKING;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td1, "TARGET_PLAYER") ){
			if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
				int i = 0;
				while( i < get_storm_count() && can_target(&td) ){
						pick_target(&td, "LORD_OF_THE_PIT");
						instance->number_of_targets = 1;
						kill_card( instance->targets[0].player,  instance->targets[0].card, KILL_SACRIFICE);
						i++;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td1) ){
			if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
				pick_target(&td, "LORD_OF_THE_PIT");
				instance->number_of_targets = 1;
				kill_card( instance->targets[0].player,  instance->targets[0].card, KILL_SACRIFICE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wirewood_guardian(int player, int card, event_t event){
	return landcycling(player, card, event, 2, SUBTYPE_FOREST, 0);
}

int card_wirewood_symbiote(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ELF;
	td.illegal_abilities = 0;

	return quirion_ranger_ability(player, card, event, &td, "Select an Elf you control.");
}

int card_xantid_swarm(int player, int card, event_t event){
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_player_cant_cast_type(player, card, 1-player, TYPE_ANY);
	}
	return 0;
}

int card_zombie_cutthroat(int player, int card, event_t event)
{
  // Morph - Pay 5 life.
  if (event == EVENT_CAN_UNMORPH)
	return can_pay_life(player, 5);

  if (event == EVENT_UNMORPH)
	{
	  lose_life(player, 5);
	  return 0;
	}

  return morph(player, card, event, MANACOST_X(0));
}

