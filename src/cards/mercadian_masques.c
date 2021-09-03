#include "manalink.h"

// Functions
int generic_subtype_searcher(int player, int card, event_t event, int subtype, int val, int mode){
	// mode = 0 --> CMC < val
	// mode = 1 --> CMC <= val
	// mode = 2 --> any CMC

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "");
		this_test.subtype = subtype;
		if( mode != 2 ){
			this_test.cmc = val+mode;
			this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
			if (ai_is_speculating != 1){
				scnprintf(this_test.message, 100, "Select %s permanent card with CMC %d or less.", get_subtype_text("%a", subtype), val + mode - 1);
			}
		} else if (ai_is_speculating != 1){
			strcpy(this_test.message, get_subtype_text("Select %a permanent card.", subtype));
		}
		parent->targets[2].card = 2+new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DECK_SEARCHER, val, 0, 0, 0, 0, 0, 0, 0, 0);
}

static int count_type_in_hand(int player, int type){
	// 0x100 = TYPE_PLANESWALKER
	// 0x200 = TYPE_TRIBAL

	int count = 0;
	int result = 0;
	while( count < active_cards_count[player] ){
			if( in_hand(player, count) ){
				if( type & 0x100 ){
					if( is_planeswalker(player, count) ){
						result++;
					}
				}
				else if( type & 0x200 ){
						if( ! is_what(player, count, TYPE_CREATURE) && is_tribal(player, count) ){
							result++;
						}
				}
				else{
					if( is_what(player, count, type)  ){
						result++;
					}
				}

			}
			count++;
	}
	return result;
}

static int part_of_ramos(int player, int card, event_t event, int manacolor){

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			declare_mana_available(player, manacolor, 2);
		}
		else{
			declare_mana_available(player, manacolor, 1);
		}
	}

	else if( event == EVENT_CAN_ACTIVATE ){
		return can_produce_mana(player, card);
	}

	else if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Tap to add mana\n Sac to add mana\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				ai_modifier -= 36;

				produce_mana(player, manacolor, 1);
				kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

static int depletion_land(int player, int card, event_t event, int manacolor)
{
  /* Land
   * ~ enters the battlefield tapped with two depletion counters on it.
   * |T, Remove a depletion counter from ~: Add |C|C to your mana pool. If there are no depletion counters on ~, sacrifice it. */

  comes_into_play_tapped(player, card, event);
  enters_the_battlefield_with_counters(player, card, event, COUNTER_DEPLETION, 2);

  if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect(player, card);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card) && count_counters(player, card, COUNTER_DEPLETION);

  if (event == EVENT_ACTIVATE)
	{
	  remove_counter(player, card, COUNTER_DEPLETION);
	  produce_mana_tapped(player, card, manacolor, 2);
	  if (count_counters(player, card, COUNTER_DEPLETION) == 0)
		kill_card(player, card, KILL_SACRIFICE);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card) && count_counters(player, card, COUNTER_DEPLETION))
	declare_mana_available(player, manacolor, 2);

  return 0;
}

static int rishadan_delinquents(int player, int card, event_t event, int mana_req){

	if( comes_into_play(player, card, event) ){
		int sac = 1;
		if( has_mana(1-player, COLOR_COLORLESS, mana_req) ){
			charge_mana(1-player, COLOR_COLORLESS, mana_req);
			if( spell_fizzled != 1 ){
				sac = 0;
			}
		}
		if( sac == 1 ){
			impose_sacrifice(player, card, 1-player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}


// Cards

int card_alley_grifters(int player, int card, event_t event ){

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && !is_unblocked(player, card) ){
		discard(1-player, 0, player);
	}
	return 0;
}

int card_ancestral_mask(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			int plus = 2*(count_subtype(2, TYPE_ENCHANTMENT, -1)-1);
			if( plus < 0 ){
				plus = 0;
			}
			event_result+=plus;
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_arms_dealer(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, instance->parent_card);
		}
	}

	return 0;
}

int card_arrest(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		disable_all_activated_abilities(instance->targets[0].player, instance->targets[0].card, 1);
	}
	if( in_play(player, card) && instance->damage_target_player > -1 && instance->damage_target_card > -1 ){
		cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
		cannot_block(instance->damage_target_player, instance->damage_target_card, event);
		if( leaves_play(player, card, event) ){
			disable_all_activated_abilities(instance->damage_target_player, instance->damage_target_card, 0);
		}
	}
	return disabling_aura(player, card, event);
}

int card_bargaining_table(int player, int card, event_t event ){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, hand_count[1-player], 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_bifurcate(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TARGET_TYPE_TOKEN;
	td.preferred_controller = player;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );
			tutor_card_with_the_same_name(instance->targets[0].player, instance->targets[0].card, player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nontoken creature", 1, NULL);
}

int card_black_market(int player, int card, event_t event ){

	/* Black Market	|3|B|B
	 * Enchantment
	 * Whenever a creature dies, put a charge counter on ~.
	 * At the beginning of your precombat main phase, add |B to your mana pool for each charge counter on ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counters(player, card, COUNTER_CHARGE, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[1].card != 66 ){
		produce_mana(player, COLOR_BLACK, count_counters(player, card, COUNTER_CHARGE));
		instance->targets[1].card = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_blood_oath(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int types[8] = {TYPE_LAND, TYPE_CREATURE, TYPE_ENCHANTMENT, TYPE_SORCERY, TYPE_INSTANT, TYPE_ARTIFACT, 0x100, 0x200};
			int choice = do_dialog(player, player, card, -1, -1, " Land\n Creature\n Enchantment\n Sorcery\n Instant\n Artifact\n Planeswalker\n Tribal", internal_rand(8));
			int sel_type = types[choice];


			int amount = count_type_in_hand(instance->targets[0].player, sel_type);

			reveal_target_player_hand(instance->targets[0].player);

			if( amount > 0 ){
				damage_player(instance->targets[0].player, 3*amount, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_bog_witch(int player, int card, event_t event ){

	int active = can_use_activated_abilities(player, card) && has_mana(player, COLOR_BLACK, 1) &&
				 ! is_tapped(player, card) && ! is_sick(player, card);

	if( event == EVENT_CAN_ACTIVATE && active && hand_count[player] > 0 ){
		return 1;
	}

	else if( event == EVENT_ACTIVATE ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->state |= STATE_TAPPED;
			charge_mana(player, COLOR_BLACK, 1);
			if( spell_fizzled == 1 ){
				instance->state &= ~STATE_TAPPED;
			} else {
				discard(player, 0, player);
				produce_mana_tapped(player, card, COLOR_BLACK, 3);
			}
	}

	else if( event == EVENT_COUNT_MANA ){
			if( affect_me(player, card) && active && hand_count[player] > 0 ){
				declare_mana_available(player, COLOR_BLACK, 3);
			}
	}

	return 0;
}

int card_briar_patch(int player, int card, event_t event)
{
  // Whenever a creature attacks you, it gets -1/-0 until end of turn.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK | DAT_ATTACKS_PLAYER, 1-player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  pump_until_eot(player, card, current_turn, attackers[amt], -1,0);
	}

  return global_enchantment(player, card, event);
}

int card_bribery(int player, int card, event_t event){
	// original code : 0040F730
	if( IS_GS_EVENT(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");
		return steal_permanent_from_target_opponent_deck(player, card, event, &this_test);
	}
	return 0;
}

int card_cackling_witch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, -1, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_cateran_brute(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 2, 1);
}

int card_cateran_enforcer(int player, int card, event_t event ){
	fear(player, card, event);

	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 4, 1);
}

int card_cateran_kidnapper(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 3, 1);
}

int card_cateran_overlord(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( (land_can_be_played & LCBP_REGENERATION) && can_regenerate(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)  ){
			if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				instance->targets[7].player = 66;
				return 0x63;
			}
		}
		return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 6, 0);
	}

	else if( event == EVENT_ACTIVATE ){
			if( instance->targets[7].player == 66 && charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)  ){
				sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 6, 0);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[7].player == 66 ){
				regenerate_target(player, instance->parent_card);
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card);
				parent->targets[7].player = 0;
			}
			else{
				return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 6, 0);
			}
	}

	return 0;
}

int card_cateran_persuader(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 1, 1);
}

int card_cateran_slavers(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, 5, 1);
}

int card_cateran_summons(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_PERMANENT, 0, SUBTYPE_MERCENARY, 0, 0, 0, 0, 0, -1, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cave_in(int player, int card, event_t event){

	al_pitchspell(player, card, event, COLOR_TEST_RED, 0);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! casting_al_pitchspell(player, card, event, COLOR_TEST_RED, 0) ){
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			damage_all(player, card, player, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			damage_all(player, card, 1-player, 2, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_chameleon_spirit(int player, int card, event_t event){

	if (card == -1){
		return 0;	// no power/toughness except on battlefield
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].player = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( instance->targets[1].player > 0 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			test_definition_t test;
			default_test_definition(&test, TYPE_PERMANENT);
			test.color = instance->targets[1].player;

			event_result += check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &test);
		}
	}

	return 0;
}

int card_charisma(int player, int card, event_t event){

	/* Charisma	|U|U|U
	 * Enchantment - Aura
	 * Enchant creature
	 * Whenever enchanted creature deals damage to a creature, gain control of the other creature for as long as ~ remains on the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card != -1 && damage->damage_source_player == t_player &&
					damage->damage_source_card == t_card && damage->info_slot > 0
				  ){
					if( instance->info_slot < 1 ){
						instance->info_slot = 1;
					}
					instance->targets[instance->info_slot].player = damage->damage_target_player;
					instance->targets[instance->info_slot].card = damage->damage_target_card;
					instance->info_slot++;
				}
			}
		}
		if( instance->info_slot > 1 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0;i<instance->info_slot;i++){
						if( instance->targets[i+1].player != -1 && instance->targets[i+1].card != -1 ){
							gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[i+1].player, instance->targets[i+1].card, 0);
						}
					}
					instance->info_slot = 1;
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_cho_arrim_alchemist(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( target->info_slot > 0 ){
			gain_life(player, target->info_slot);
		}
		target->info_slot = 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DAMAGE_PREVENTION|GAA_DISCARD, 1, 0, 0, 0, 0, 2, 0,
									&td, "TARGET_DAMAGE");
}

int card_cho_arrim_bruiser(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may tap up to two target creatures.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 3;
	  if (player == AI)
		{
		  td.allowed_controller = 1-player;
		  td.preferred_controller = 1-player;
		  td.illegal_state = TARGET_STATE_TAPPED;
		}

	  card_instance_t* instance = get_card_instance(player, card);
	  pick_up_to_n_targets(&td, "TARGET_CREATURE", 2);
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		tap_card(instance->targets[i].player, instance->targets[i].card);
	}

  return 0;
}

int card_cho_manno_revolutionary(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	prevent_all_damage(player, card, event);

	return 0;
}

int card_cinder_elemental(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME|GAA_CAN_TARGET,
													-1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE_PLAYER");
}

int card_collective_unconscious(int player, int card, event_t event){
	/*
	  Collective Unconscious |4|G|G
	  Sorcery
	  Draw a card for each creature you control.
	*/

	if(event == EVENT_RESOLVE_SPELL){
		draw_cards(player, count_subtype(player, TYPE_CREATURE, -1));
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_coastal_piracy(int player, int card, event_t event){
	// original code : 00405650
/*
Coastal Piracy |2|U|U
Enchantment
Whenever a creature you control deals combat damage to an opponent, you may draw a card.
*/

	check_damage_test(player, -1, event, DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE, player, card, TYPE_CREATURE, NULL);

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[8].player > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if (--instance->targets[8].player > 0){
						instance->state &= ~STATE_PROCESSING;	// More optional triggers left.  Must be the first thing done during resolve trigger.
					}
					draw_cards(player, 1);
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[8].player = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_common_cause(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;
	td.allowed_controller = 2;
	td.preferred_controller = 2;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_type = TYPE_ARTIFACT;
	td1.allowed_controller = 2;
	td1.preferred_controller = 2;
	td1.illegal_abilities = 0;
	td1.required_color = COLOR_TEST_BLACK;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.illegal_type = TYPE_ARTIFACT;
	td2.allowed_controller = 2;
	td2.preferred_controller = 2;
	td2.illegal_abilities = 0;
	td2.required_color = COLOR_TEST_BLUE;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);
	td3.illegal_type = TYPE_ARTIFACT;
	td3.allowed_controller = 2;
	td3.preferred_controller = 2;
	td3.illegal_abilities = 0;
	td3.required_color = COLOR_TEST_GREEN;

	target_definition_t td4;
	default_target_definition(player, card, &td4, TYPE_CREATURE);
	td4.illegal_type = TYPE_ARTIFACT;
	td4.allowed_controller = 2;
	td4.preferred_controller = 2;
	td4.illegal_abilities = 0;
	td4.required_color = COLOR_TEST_RED;

	target_definition_t td5;
	default_target_definition(player, card, &td5, TYPE_CREATURE);
	td5.illegal_type = TYPE_ARTIFACT;
	td5.allowed_controller = 2;
	td5.preferred_controller = 2;
	td5.illegal_abilities = 0;
	td5.required_color = COLOR_TEST_WHITE;

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && !is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)){
		int total = target_available(player, card, &td);
		if( total == target_available(player, card, &td1) || total == target_available(player, card, &td2) ||
			total == target_available(player, card, &td3) || total == target_available(player, card, &td4) ||
			total == target_available(player, card, &td5)
			){
			event_result+=2;
		}
	}

	return global_enchantment(player, card, event);
}

int card_crackdown(int player, int card, event_t event ){

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		){
		if( ! (get_color(affected_card_controller, affected_card) & COLOR_TEST_WHITE) &&
			get_power(affected_card_controller, affected_card) > 2
			){
			card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
			instance->untap_status &= ~3;
		}
	}

	return global_enchantment(player, card, event);
}

int card_crooked_scales(int player, int card, event_t event)
{
	/* Crooked Scales	|4
	 * Artifact
	 * |4, |T: Flip a coin. If you win the flip, destroy target creature an opponent controls. If you lose the flip, destroy target creature you control unless
	 * you pay |3 and repeat this process. */

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td_opp;
  default_target_definition(player, card, &td_opp, TYPE_CREATURE);
  td_opp.allowed_controller = td_opp.preferred_controller = 1-player;

  target_definition_t td_own;
  default_target_definition(player, card, &td_own, TYPE_CREATURE);
  td_own.allowed_controller = td_own.preferred_controller = player;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP(player, card) && CAN_ACTIVATE(player, card, MANACOST_X(4)) && can_target(&td_opp) && can_target(&td_own);

  if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;
	  if (charge_mana_for_activated_ability(player, card, MANACOST_X(4))
		  && pick_target(&td_opp, "TARGET_CREATURE_OPPONENT_CONTROLS")
		  && new_pick_target(&td_own, "ASHNODS_BATTLEGEAR", 1, 1))	// "Select target creature you control."
		tap_card(player, card);
	  else
		instance->number_of_targets = 0;	// in case it was cancelled during second target choice
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int valid_opp = valid_target(&td_opp), valid_own = validate_target(player, card, &td_own, 1);
	  if (!valid_opp && !valid_own)
		{
		  spell_fizzled = 1;
		  return 0;
		}

	  while (1)
		{
		  if (flip_a_coin(player, card))
			{
			  if (valid_opp)
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			  break;
			}
		  else if (has_mana(player, COLOR_ANY, 3)
				   && do_dialog(player, player, card, valid_own ? instance->targets[1].player : -1, valid_own ? instance->targets[1].card : -1,
								" Pay 3\n Destroy creature you control", 0) == 0
				   && charge_mana_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, player, COLOR_COLORLESS, 3))
			continue;
		  else
			{
			  if (valid_own)
				kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
			  break;
			}
		}
	}

  return 0;
}

int card_crumbling_sanctuary(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->info_slot > 0 ){
				int amount = damage->info_slot;
				if( count_deck(damage->damage_target_player) < amount ){
					amount = count_deck(damage->damage_target_player);
				}
				damage->info_slot-=amount;
				int i;
				for(i=0; i<amount; i++){
					rfg_top_card_of_deck(damage->damage_target_player);
				}
			}
		}
	}

	return 0;
}

int card_custom_depot(int player, int card, event_t event ){

	if( has_mana(player, COLOR_COLORLESS, 1) && specific_spell_played(player, card, event, player, 1+player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			draw_cards(player, 1);
			discard(player, 0, player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_dawnstrider(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		fog_effect(player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

static void destroy_nonblack_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (!(get_color(t_player, t_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK)))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_deathgazer(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_ASK_CARD;

  if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
	  && !(get_color(attacking_card_controller, attacking_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK)))
	event_result |= 1;

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_nonblack_at_end_of_combat, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_nonblack_at_end_of_combat, player, card);
	}

  return 0;
}

int card_deepwood_drummer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD|GAA_CAN_TARGET, 0, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_deepwood_ghoul(int player, int card, event_t event){
	/*
	  Deepwood Ghoul |2|B
	  Creature - Zombie 2/1
	  Pay 2 life: Regenerate Deepwood Ghoul.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST0, 2, NULL, NULL);
}

int card_dehydration(int player, int card, event_t event)
{
  // 0x4088d0
  card_instance_t* instance = get_card_instance(player, card);

  does_not_untap(instance->damage_target_player, instance->damage_target_card, event);

  return disabling_aura(player, card, event);
}

int card_delraich(int player, int card, event_t event){
	/*
	  Delraich |6|B
	  Creature - Horror 6/6
	  Trample
	  You may sacrifice three black creatures rather than pay Delraich's mana cost.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( can_sacrifice_as_cost(player, 3, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Sac 3 creatures\n Pay mana cost\n Cancel", 1);
			}
			if( choice == 0 ){
				if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0) ){
					impose_sacrifice(player, card, player, 2, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0);
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

int card_devout_witness(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD|GAA_CAN_TARGET, 1, 0, 0, 0, 0, 1, 0, &td, "DISENCHANT");
}

int card_diplomatic_immunity(int player, int card, event_t event ){
	/*
	  Diplomatic Immunity |1|U
	  Enchantment - Aura
	  Enchant creature
	  Shroud (A permanent with shroud can't be the target of spells or abilities.)
	  Enchanted creature has shroud.
	*/

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_SHROUD, 0, 0, 0, 0);
}

int card_distorting_lens(int player, int card, event_t event){
	/*
	  Distorting Lens English
	  Artifact, 2
	  {T}: Target permanent becomes the color of your choice until end of turn.
	*/
	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		change_color(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
					1<<(choose_a_color(player, get_deck_color(player, 1-player))), 2 | 4);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PERMANENT");
}

int card_dust_bowl(int player, int card, event_t event){

	/* Dust Bowl	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |3, |T, Sacrifice a land: Destroy target nonbasic land. */

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) &&
			can_target(&td) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Destroy a nonbasic land\n Cancel", ai_choice);
		}
		instance->info_slot = 0;

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			if (charge_mana_for_activated_ability(player, card, MANACOST_X(3))
				&& pick_target_nonbasic_land(player, card, 0)
			   ){
				if (controller_sacrifices_a_permanent(player, card, TYPE_LAND, IS_AI(player) ? SAC_NO_CANCEL : 0)){
					instance->info_slot = 1;
					return 0;
				}
			}
			untap_card_no_event(player, card);
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
			}
			else{
				return mana_producer(player, card, event);
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_embargo(int player, int card, event_t event ){

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP &&
		! is_what(affected_card_controller, affected_card, TYPE_LAND)
	  ){
		card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
		instance->untap_status &= ~3;
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 2);
	}

	return global_enchantment(player, card, event);
}

int card_enslaved_horror(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_eye_of_ramos(int player, int card, event_t event ){

	return part_of_ramos(player, card, event, COLOR_BLUE);
}

int card_forced_march(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, instance->info_slot+1, 3, KILL_DESTROY);
			manipulate_all(player, card, 1-player,TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, instance->info_slot+1, 3, KILL_DESTROY);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_foster(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && affected->kill_code > 0 &&
			affected->kill_code < 4) {
		   if( instance->targets[11].player < 0 ){
			  instance->targets[11].player = 0;
		   }
		   instance->targets[11].player++;
		}
	}

	if( instance->targets[11].player > 0 && has_mana(player, COLOR_COLORLESS, 1) && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY &&
		player == reason_for_trigger_controller
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1+player;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0;i<instance->targets[11].player;i++){
					if( has_mana(player, COLOR_COLORLESS, 1) ){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 ){
							int *deck = deck_ptr[player];
							if( deck[0] != -1 ){
								int count = 0;
								while(1){
									if( is_what(-1, deck[count], TYPE_CREATURE) ){
										add_card_to_hand(player, deck[count]);
										remove_card_from_deck(player, count);
										if( player == AI ){
											show_deck( HUMAN, deck, count+1, "Opponent revealed these cards", 0, 0x7375B0 );
										}
										break;
									}
									count++;
								}
								if( count > 0 ){
									mill(player, count);
								}
							}
							else{
								break;
							}
						}
						else{
							break;
						}
					}
					else{
						break;
					}
				}
				instance->targets[11].player = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_furious_assault(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( can_target(&td1) && specific_spell_played(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		if( pick_target(&td1, "TARGET_PLAYER") ){
			damage_player(instance->targets[0].player, 1, player, card);
			instance->number_of_targets = 1;
		}
	}

	return global_enchantment(player, card, event);
}

int card_game_preserve(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *h_deck = deck_ptr[HUMAN];
		int *ai_deck = deck_ptr[AI];
		show_deck( HUMAN, h_deck, 1, "Here's the first card of your deck", 0, 0x7375B0 );
		show_deck( HUMAN, ai_deck, 1, "Here's the first card of opponent's deck", 0, 0x7375B0 );
		if( is_what(-1, h_deck[0], TYPE_CREATURE) && is_what(-1, ai_deck[0], TYPE_CREATURE) ){
			put_into_play_a_card_from_deck(player, player, 0);
			put_into_play_a_card_from_deck(1-player, 1-player, 0);
		}
	}

	return global_enchantment(player, card, event);
}

int card_generals_regalia(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
		can_target(&td) && can_target(&td1) && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0)
	  ){
		return 0x63;
	}
	else if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( pick_target(&td, "TARGET_DAMAGE") ){
				instance->number_of_targets = 1;
				card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( dmg->damage_target_player == player && dmg->damage_target_card == -1 ){
					instance->targets[1].player = instance->targets[0].player;
					instance->targets[1].card = instance->targets[0].card;
					pick_target(&td1, "TARGET_CREATURE");
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *dmg = get_card_instance(instance->targets[1].player, instance->targets[1].card);
			dmg->damage_target_card = instance->targets[0].card;
			if( get_id(instance->targets[1].player, instance->targets[1].card) == CARD_ID_CHO_MANNO_REVOLUTIONARY ){
				dmg->info_slot = 0;
			}
	}
	return 0;
}

int card_gush(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = SUBTYPE_ISLAND;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		if( target_available(player, card, &td) > 1 ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if ( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL  && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_LAND);
			td.required_subtype = SUBTYPE_ISLAND;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;

			int choice = 0;
			if( has_mana_to_cast_iid(player, event, instance->internal_card_id) ){
				choice = do_dialog(player, player, card, -1, -1, " Return 2 Islands\n Play normally\n Cancel", 0);
			}

			if( choice == 0 ){
				int trgs = 0;
				while( trgs < 2 ){
						if( new_pick_target(&td, "Select an Island to bounce.", trgs, GS_LITERAL_PROMPT) ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
							trgs++;
						}
						else{
							break;
						}
				}
				int i;
				for(i=0; i<trgs; i++){
					if( trgs == 2 ){
						bounce_permanent(instance->targets[i].player, instance->targets[i].card);
					}
					else{
						state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					}
				}
				instance->number_of_targets = 0;
				if( trgs < 2 ){
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hammer_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = instance->info_slot;
		manipulate_all(player, instance->parent_card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, amount+1, 3, KILL_DESTROY);
		manipulate_all(player, instance->parent_card, 1-player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, amount+1, 3, KILL_DESTROY);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, -1, 0, 0, 0, 1, 0, 0, 0, 0);
}

int card_haunted_crossroads(int player, int card, event_t event){

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return ! graveyard_has_shroud(2);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				from_graveyard_to_deck(player, selected, 1);
			}
	}
	return global_enchantment(player, card, event);
}

int card_heart_of_ramos(int player, int card, event_t event ){

	return part_of_ramos(player, card, event, COLOR_RED);
}

int card_hickory_woodlot(int player, int card, event_t event ){
	/* Hickory Woodlot	""
	 * Land
	 * ~ enters the battlefield tapped with two depletion counters on it.
	 * |T, Remove a depletion counter from ~: Add |G|G to your mana pool. If there are no depletion counters on ~, sacrifice it. */
	return depletion_land(player, card, event, COLOR_GREEN);
}

int card_high_market(int player, int card, event_t event){

	/* High Market	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |T, Sacrifice a creature: You gain 1 life. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && !(is_tapped(player, card)) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;

		if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac and gain 1 life\n Cancel", 1);
		}

		instance->info_slot = choice;

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
					tap_card(player, card);
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
				else{
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_hired_giant(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		if( do_dialog(1-player, player, card, -1, -1, " Tutor a land\n Pass", (1-player == AI ? 0 : 1)) == 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
			new_global_tutor(1-player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 1, &this_test);
		}
	}

	return 0;
}

int card_honor_the_fallen(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			int amount = 0;
			for(i=0; i<2; i++){
				const int *grave = get_grave(i);
				int count = count_graveyard(i);
				while( count > -1 ){
						if( is_what(-1, grave[count], TYPE_CREATURE) ){
							rfg_card_from_grave(i, count);
							amount++;
						}
						count--;
				}
			}
			gain_life(player, amount);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int horn_of_plenty_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( eot_trigger(player, card, event) ){
		draw_cards(instance->targets[0].player, 1);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_horn_of_plenty(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && has_mana(trigger_cause_controller, COLOR_COLORLESS, 1) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				if( player == AI ){
					event_result |= 2;
				}
				else{
					if( trigger_cause_controller == player ){
						event_result |= 1+player;
					}
					else{
						event_result |= 2;
					}
				}
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					charge_mana(trigger_cause_controller, COLOR_COLORLESS, 1);
					if( spell_fizzled != 1 ){
						int legacy = create_legacy_effect(player, card, &horn_of_plenty_legacy);
						card_instance_t *instance = get_card_instance( player, legacy );
						instance->targets[0].player = trigger_cause_controller;
					}
			}
		}
	}
	return 0;
}

int card_horn_of_ramos(int player, int card, event_t event ){

	return part_of_ramos(player, card, event, COLOR_GREEN);
}

int card_howling_wolf(int player, int card, event_t event ){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int id = get_id(player, card);
		int num = 0;
		while( num < 3 ){
				int crd = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, 0, 0, 0, 0, 0, 0, id, 0, -1, 0);
				if( crd == -1 ){
					break;
				}
				else{
					num++;
				}
		}
	}

	return part_of_ramos(player, card, event, COLOR_GREEN);
}

int card_hunted_wumpus(int player, int card, event_t event ){
	/*
	  Hunted Wumpus |3|G
	  Creature - Beast 6/6
	  When Hunted Wumpus enters the battlefield, each other player may put a creature card from his or her hand onto the battlefield.
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to put into play.");
		this_test.zone = TARGET_ZONE_HAND;
		if( check_battlefield_for_special_card(player, card, 1-player, 0, &this_test) ){
			new_global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_intimidation(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE))
	fear(affected_card_controller, affected_card, event);

  return global_enchantment(player, card, event);
}

int card_invigorate(int player, int card, event_t event){
	/*
	  Invigorate |2|G
	  Instant
	  If you control a Forest, rather than pay Invigorate's mana cost, you may have an opponent gain 3 life.
	  Target creature gets +4/+4 until end of turn.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CHECK_PUMP ){
		if( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) ||
			check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST)
		  ){
			pumpable_power[player] += 4;
			pumpable_toughness[player] += 4;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Make opponent gain life\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				gain_life(1-player, 3);
				td.allow_cancel = 0;
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	return 0;
}

int card_ivory_mask(int player, int card, event_t event){
	give_shroud_to_player(player, card, event);
	return global_enchantment(player, card, event);
}

int card_karns_touch(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.illegal_type = TYPE_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			artifact_animation(player, card, instance->targets[0].player, instance->targets[0].card, 1, -1, -1, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select targer noncreature artifact", 1, NULL);
}

int card_kris_mage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, 0, 0, 0, 0, 1, 0, 0,
									&td, "TARGET_CREATURE_OR_PLAYER");
}

int card_kyren_negotiations(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_target(&td1) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td);
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td1, "TARGET_CREATURE") ){
				instance->targets[1].player = instance->targets[0].player;
				instance->targets[1].card = instance->targets[0].card;
				instance->number_of_targets = 1;
				if( pick_target(&td, "TARGET_PLAYER") ){
					tap_card(instance->targets[1].player, instance->targets[1].card);
					instance->number_of_targets = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				damage_player(instance->targets[0].player, 1, player, card);
			}
	}
	return global_enchantment(player, card, event);
}

int card_kyren_toy(int player, int card, event_t event){
	return mana_battery_generic(player, card, event, COLOR_COLORLESS, 1);
}

int card_land_grant(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, event, instance->internal_card_id) ){
				choice = do_dialog(player, player, card, -1, -1, " Reveal your hand\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				reveal_target_player_hand(player);
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_larceny(int player, int card, event_t event){
	// original code : 0x40DC50

	card_instance_t *instance = get_card_instance(player, card);
	card_instance_t* damage = combat_damage_being_dealt(event);

	if( damage &&
		damage->damage_source_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
		){
		if (damage->damage_target_player == 0){
			SET_BYTE0(instance->info_slot)++;
		} else {
			SET_BYTE1(instance->info_slot)++;
		}
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			int num[2] = { BYTE0(instance->info_slot), BYTE1(instance->info_slot) };
			instance->info_slot = 0;
			new_multidiscard(0, num[0], 0, player);
			new_multidiscard(1, num[1], 0, player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_liability(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY  ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( make_test_in_play(affected_card_controller, affected_card, -1, TYPE_PERMANENT, 3, 0, 0, 0, 0, 0, 0, -1, 0) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				instance->targets[pos].player = affected_card_controller;
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int i;
		for(i=0; i<instance->targets[11].player; i++ ){
			if( instance->targets[i].player != -1 ){
				lose_life(instance->targets[i].player, 1);
			}
		}
		instance->targets[11].player = 0;
	}
	return global_enchantment(player, card, event);
}

int card_lumbering_satyr(int player, int card, event_t event ){

	return boost_creature_type(player, card, event, -1, 0, 0, get_hacked_walk(player, card, KEYWORD_FORESTWALK), BCT_INCLUDE_SELF);
}

int card_lunge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
			pick_target(&td1, "TARGET_PLAYER");
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 2, player, card);
		}
		if( valid_target(&td1) ){
			damage_player(instance->targets[0].player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_magistrates_scepter(int player, int card, event_t event){

	/* Magistrate's Scepter	|3
	 * Artifact
	 * |4, |T: Put a charge counter on ~.
	 * |T, Remove three charge counters from ~: Take an extra turn after this one. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_use_activated_abilities(player, card)  ){
		if( count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && player != AI ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->targets[8].player = -1;
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
			if( count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Add a counter\n Gain an additional turn\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, (1-choice)*4, 0, 0, 0, 0, 0) ){
				if( choice == 1 ){
					remove_counters(player, card, COUNTER_CHARGE, 3);
				}
				instance->targets[1].player = choice;
				instance->state |= STATE_TAPPED;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 0 ){
			add_counter(player, card, COUNTER_CHARGE);
		}
		else if( instance->targets[1].player == 1 ){
				time_walk_effect(instance->parent_controller, instance->parent_card);
		}
	}

	if( player == AI && current_turn != player && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && eot_trigger(player, card, event)
	  ){
		charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
			add_counter(player, card, COUNTER_CHARGE);
		}
	}
	return 0;
}

int card_megatherium(int player, int card, event_t event ){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( ! has_mana(player, COLOR_COLORLESS, hand_count[player]) ){
			ai_modifier-=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		int kill_me = 1;
		if( has_mana(player, COLOR_COLORLESS, hand_count[player]) ){
			charge_mana(player, COLOR_COLORLESS, hand_count[player]);
			if( spell_fizzled != 1 ){
				kill_me = 0;
			}
		}
		if( kill_me == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_mercadian_atlas(int player, int card, event_t event ){

	if( current_turn == player && ! lands_played && ! is_humiliated(player, card) &&
		eot_trigger_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player))
	  ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_mercadian_lift(int player, int card, event_t event){

	/* Mercadian Lift	|2
	 * Artifact
	 * |1, |T: Put a winch counter on ~.
	 * |T, Remove X winch counters from ~: You may put a creature card with converted mana cost X from your hand onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_use_activated_abilities(player, card)  ){
		if( player != AI ){
			return has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0);
		}
		else{
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				int i=0;
				for(i=0;i<active_cards_count[player];i++){
					if( in_hand(player, i) && is_what(player, i, TYPE_CREATURE) &&
						get_cmc(player, i) == count_counters(player, card, COUNTER_WINCH)
					  ){
						return 1;
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && player != AI ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Add a counter\n Put a creature into play\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 1-choice, 0, 0, 0, 0, 0) ){
				if( choice == 0 ){
					tap_card(player, card);
					instance->targets[1].player = 100;
				}
				else if( choice == 1 ){
						int amount = 0;
						int par = 0;
						if( player != AI ){
							amount = choose_a_number(player, "Remove how many winch counters?", count_counters(player, card, COUNTER_WINCH));
							if( amount > count_counters(player, card, COUNTER_WINCH) || amount < 1 ){
								spell_fizzled = 1;
							}
						}
						else{
							 int i;
							 for(i=0;i<active_cards_count[player]; i++){
								 if( in_hand(player, i) && is_what(player, i, TYPE_CREATURE) ){
									 if( get_cmc(player, i) <= count_counters(player, card, COUNTER_WINCH) && get_base_value(player, i) > par ){
										amount = get_cmc(player, i);
										par = get_base_value(player, i);
									}
								}
							}
						}
						remove_counters(player, card, COUNTER_WINCH, amount);
						instance->targets[1].player = amount;
						tap_card(player, card);
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 100 ){
			add_counter(player, card, COUNTER_WINCH);
		}
		else if( instance->targets[1].player > -1 ){
				char buffer[500];
				snprintf(buffer, 100, "Select a creature card with CMC %d", instance->targets[1].player);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
				this_test.cmc = instance->targets[1].player;
				new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
	}

	if( player == AI && current_turn != player && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && eot_trigger(player, card, event)
	  ){
		if( count_counters(player, card, COUNTER_WINCH) < get_most_common_cmc_in_hand(player, TYPE_CREATURE) ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				add_counter(player, card, COUNTER_WINCH);
			}
		}
	}
	return 0;
}

int card_midnight_ritual(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return !check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
		int generic_mana_available = has_mana(player, COLOR_ANY, 1);	// A useful trick from card_fireball().
		max_cards = MIN(max_cards, generic_mana_available);
		max_cards = MIN(max_cards, 19);

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		char buf[100];
		if (ai_is_speculating == 1){
			*buf = 0;
		} else if (max_cards == 1){
			strcpy(buf, "Select a creature card.");
		} else {
			sprintf(buf, "Select up to %d creature cards.", max_cards);
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buf);
		instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MIN_VALUE, &this_test, max_cards, &instance->targets[0]);

		if (instance->info_slot > 0){
			charge_mana(player, COLOR_COLORLESS, instance->info_slot);
		}
	} else if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < instance->info_slot; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				rfg_card_from_grave(player, selected);
				++num_validated;
			}
		}
		if (num_validated > 0){
			generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, num_validated);
		} else if (instance->info_slot > 0) {
			spell_fizzled = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_misstep(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				int p = instance->targets[0].player;
				int count = active_cards_count[p]-1;
				while( count > -1 ){
						if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) ){
							does_not_untap_effect(player, card, p, count, 0, 1);
						}
						count--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_moment_of_silence(int player, int card, event_t event)
{
  // Target player skips his or her next combat phase this turn.
  target_definition_t td_player;
  default_target_definition(player, card, &td_player, 0);
  td_player.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_CAN_CAST)
	return can_target(&td_player);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	pick_target(&td_player, "TARGET_PLAYER");


  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td_player))
		target_player_skips_his_next_attack_step(player, card, instance->targets[0].player, 1);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_monkey_cage(int player, int card, event_t event){
	/* Monkey Cage	|5
	 * Artifact
	 * When a creature enters the battlefield, sacrifice ~ and put X 2/2 |Sgreen Ape creature tokens onto the battlefield, where X is that creature's converted mana cost. */

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance( player, card );
		int cmc = get_cmc(instance->targets[1].player, instance->targets[1].card);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_APE, &token);
		token.qty = cmc;
		token.color_forced = COLOR_TEST_GREEN;
		token.pow = token.tou = 2;
		generate_token(&token);
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_monlit_wake(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *instance = get_card_instance( player, card );
		gain_life(player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_muzzle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card && damage->info_slot > 0){
				if( damage->damage_source_player == t_player && damage->damage_source_card == t_card ){
					damage->info_slot = 0;
				}
			}
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_natural_affinity(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_LAND);
		land_animation2(player, card, 2, -1, 1, 2, 2, 0, 0, 0, &this_test2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_nether_spirit(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) == 1 ){
			int choice = 0;
			if( ! duh_mode(player) ){
				choice = do_dialog(player, player, card, -1, -1," Return Nether Spirit\n Pass", 0);
			}
			if( choice == 0 ){
				put_into_play(player, card);
				return -1;
			}
			else{
				return -2;
			}
		}
		else{
			return -2;
		}
	}

	return 0;
}

int card_noble_purpose(int player, int card, event_t event){
	// original code : 0x403D20

	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* source = get_card_instance(affected_card_controller, affected_card);

		if (source->internal_card_id == damage_card && source->info_slot > 0
			&& source->damage_source_player == player && source->damage_source_card >= 0
			&& (source->token_status & (STATUS_FIRST_STRIKE_DAMAGE | STATUS_COMBAT_DAMAGE))
			&& is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE)){
			gain_life(player, source->info_slot);
		}
	}

	return global_enchantment(player, card, event);
}

int card_notorious_assassin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, 2, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_orims_cure(int player, int card, event_t event){
	/*
	  Orim's Cure |1|W
	  Instant
	  If you control a Plains, you may tap an untapped creature you control rather than pay Orim's Cure's mana cost.
	  Prevent the next 4 damage that would be dealt to target creature or player this turn.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.allowed_controller = player;
		td2.preferred_controller = player;
		td2.illegal_abilities = 0;
		td2.illegal_state = TARGET_STATE_TAPPED;

		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) && can_target(&td2) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Tap a creature you control\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				target_definition_t td2;
				default_target_definition(player, card, &td2, TYPE_CREATURE);
				td2.allowed_controller = player;
				td2.preferred_controller = player;
				td2.illegal_abilities = 0;
				td2.illegal_state = TARGET_STATE_TAPPED;
				if( new_pick_target(&td2, "Select a creature you control to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
					action_on_target(player, card, 0, ACT_TAP);
					td.allow_cancel = 0;
					instance->number_of_targets = 0;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_DAMAGE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot <= 4 ){
				target->info_slot = 0;
			}
			else{
				target->info_slot-=4;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
	}

	return 0;
}

int card_overtaker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->targets[0].player != player ){
				effect_act_of_treason(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, 3, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_pangosaur(int player, int card, event_t event){

	if( specific_cip(player, card, event, 2, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_peat_bog(int player, int card, event_t event){
	/* Peat Bog	""
	 * Land
	 * ~ enters the battlefield tapped with two depletion counters on it.
	 * |T, Remove a depletion counter from ~: Add |B|B to your mana pool. If there are no depletion counters on ~, sacrifice it. */
	return depletion_land(player, card, event, COLOR_BLACK);
}

int card_power_matrix(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return vanilla_creature_pumper(player, card, event, 0, 0, 0, 0, 0, 0, GAA_UNTAPPED, 1, 1, KEYWORD_FIRST_STRIKE + KEYWORD_TRAMPLE + KEYWORD_FLYING, 0, &td);
}

static int puffer_extract_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > 0 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event,
								instance->targets[1].player, instance->targets[1].player, 0);
	}
	if( eot_trigger(player, card, event) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_puffer_extract(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(player, instance->parent_card, &puffer_extract_effect,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = instance->info_slot;
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, -1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_pulverize(int player, int card, event_t event){
	/*
	  Pulverize |4|R|R
	  Sorcery
	  You may sacrifice two Mountains rather than pay Pulverize's mana cost.
	  Destroy all artifacts.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) > 1 ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Sacrifice 2 Mountains\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				if( sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0) ){
					impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0);
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_puppets_verdict(int player, int card, event_t event)
{
  // Flip a coin. If you win the flip, destroy all creatures with power 2 or less. If you lose the flip, destroy all creatures with power 3 or greater.
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");

	  if (flip_a_coin(player, card))
		{
		  test.power = 3;
		  test.power_flag = F5_POWER_LESSER_THAN_VALUE;
		}
	  else
		{
		  test.power = 2;
		  test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		}

	  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_ramosian_sergeant(int player, int card, event_t event){

  return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 3, 0);
}

int card_ramosian_captain(int player, int card, event_t event){

  return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 5, 0);
}

int card_ramosian_commander(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 6, 0);
}

int card_ramosian_liutenant(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 4, 0);
}

int card_ramosian_sky_marshall(int player, int card, event_t event ){

	return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 7, 0);
}

int card_reverent_mantra(int player, int card, event_t event){

	al_pitchspell(player, card, event, COLOR_TEST_WHITE, 0);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! casting_al_pitchspell(player, card, event, COLOR_TEST_WHITE, 0) ){
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_subtype_until_eot(player, card, player, -1, 0, 0, keyword, 0);
			pump_subtype_until_eot(player, card, 1-player, -1, 0, 0, keyword, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_remote_farm(int player, int card, event_t event){
	/* Remote Farm	""
	 * Land
	 * ~ enters the battlefield tapped with two depletion counters on it.
	 * |T, Remove a depletion counter from ~: Add |W|W to your mana pool. If there are no depletion counters on ~, sacrifice it. */
	return depletion_land(player, card, event, COLOR_WHITE);
}

int card_renounce(int player, int card, event_t event)
{
  /* Renounce	|1|W
   * Instant
   * Sacrifice any number of permanents. You gain 2 life for each permanent sacrificed this way. */

  if (!IS_CASTING(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_PERMANENT, "");

  if (event == EVENT_CAN_CAST)
	return new_can_sacrifice(player, card, player, &test);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (IS_AI(player))
		{
		  int c, max_can_sac = 0;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (in_play(player, c) && is_what(player, c, TYPE_PERMANENT))
			  ++max_can_sac;

		  test.qty = recorded_rand(player, max_can_sac + 1);
		}
	  else
		test.qty = 151;

	  if (test.qty > 0)
		gain_life(player, 2 * new_sacrifice(player, card, player, SAC_NO_CANCEL|SAC_DONE, &test));

	  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_revive(int player, int card, event_t event){

	char msg[100] = "Select a green card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);
	this_test.color = COLOR_TEST_GREEN;

	if( event == EVENT_CAN_CAST && new_special_count_grave(player, &this_test) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Rishadan Airship --> Skywinder Drake

int card_rishadan_brigand(int player, int card, event_t event){

	return rishadan_delinquents(player, card, event, 3);
}

int card_rishadan_cutpurse(int player, int card, event_t event){

	return rishadan_delinquents(player, card, event, 1);
}

int card_rishadan_footpad(int player, int card, event_t event){

	return rishadan_delinquents(player, card, event, 2);
}

int card_rishadan_pawnshop(int player, int card, event_t event){

	/* Rishadan Pawnshop	|2
	 * Artifact
	 * |2, |T: Shuffle target nontoken permanent you control into its owner's library. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TARGET_TYPE_TOKEN;
	td.allowed_controller = player;
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);
		shuffle_into_library(instance->targets[0].player, instance->targets[0].card);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_PERMANENT");
}

int card_rishadan_port(int player, int card, event_t event){
	/* Rishadan Port	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |1, |T: Tap target land. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && can_target(&td1) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Tap target land\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_LAND") ){
				instance->info_slot = 66;
				instance->number_of_targets = 1;
			}
			else{
				 untap_card_no_event(player, card);
			}
		}
		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				if( valid_target(&td1) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				parent->info_slot = 0;
			}
	}
	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_rushwood_elemental(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_rushwood_herbalist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_DESTROYED;
	td.preferred_controller = player;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_REGENERATION|GAA_DISCARD,
									0, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_saber_ants(int player, int card, event_t event ){
	/* Saber Ants	|3|G
	 * Creature - Insect 2/3
	 * Whenever ~ is dealt damage, you may put that many 1/1 |Sgreen Insect creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == card && damage->damage_target_player == player ){
					int good = damage->info_slot;
					if( good == 0 && in_play(damage->damage_source_card, damage->damage_source_player) ){
						card_instance_t *source = get_card_instance( damage->damage_source_player, damage->damage_source_card );
						if( source->targets[16].player > 0 ){
							good += source->targets[16].player;
						}
					}
					instance->info_slot+=good;
				}
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE &&
			player == reason_for_trigger_controller && affect_me(player, card )
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					generate_tokens_by_id(player, card, CARD_ID_INSECT, instance->info_slot);
					instance->info_slot = 0;
			}
			else if (event == EVENT_END_TRIGGER){
				instance->info_slot = 0;
			}
		}


	return 0;
}

int card_sand_squid(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			does_not_untap_until_im_tapped(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *parent = get_card_instance( player, instance->parent_card );
			parent->targets[1] = instance->targets[0];
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_sandstone_needle(int player, int card, event_t event){
	/* Sandstone Needle	""
	 * Land
	 * ~ enters the battlefield tapped with two depletion counters on it.
	 * |T, Remove a depletion counter from ~: Add |R|R to your mana pool. If there are no depletion counters on ~, sacrifice it. */
	return depletion_land(player, card, event, COLOR_RED);
}

int card_saprazzan_heir(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && current_phase == PHASE_AFTER_BLOCKING && is_attacking(player, card) &&
		! is_unblocked(player, card) && instance->targets[1].player != 66
	  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			instance->targets[1].player = 66;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 3);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_saprazzan_skerry(int player, int card, event_t event){
	/* Saprazzan Skerry	""
	 * Land
	 * ~ enters the battlefield tapped with two depletion counters on it.
	 * |T, Remove a depletion counter from ~: Add |U|U to your mana pool. If there are no depletion counters on ~, sacrifice it. */
	return depletion_land(player, card, event, COLOR_BLUE);
}

int card_seismic_mage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, 2, 0, 0, 0, 1, 0, 0, &td, "TARGET_LAND");
}

int card_sever_soul(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_silverglade_elemental(int player, int card, event_t event, int mana_req){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 4, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_silverglade_pathfinder(int player, int card, event_t event){

	/* Silverglade Pathfinder	|1|G
	 * Creature - Dryad Spellshaper 1/1
	 * |1|G, |T, Discard a card: Search your library for a basic land card and put that card onto the battlefield tapped. Then shuffle your library. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, 1, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_sizzle(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			damage_player(1-player, 3, player, card);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Skulking fugitive --> Skulking ghost

int card_skull_of_ramos(int player, int card, event_t event){

	return part_of_ramos(player, card, event, COLOR_BLACK);
}

int card_snuff_out(int player, int card, event_t event){
	/*
	  Snuff Out |3|B
	  Instant
	  If you control a Swamp, you may pay 4 life rather than pay Snuff Out's mana cost.
	  Destroy target nonblack creature. It can't be regenerated.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) && can_pay_life(player, 4) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Pay 4 life\n Play normally\n Cancel", life[player] >= 10 ? 0 : 1);
			}
			if( choice == 0 ){
				lose_life(player, 4);
				td.allow_cancel = 0;
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		if( spell_fizzled != 1 ){
			new_pick_target(&td, "Select target nonblack creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
	}

	return 0;
}

int card_soothsaying(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
		has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 3, 0, 2, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Rearrange\n Shuffle\n Do nothing", 0);
			}

			if( choice == 0 ){
				charge_mana_for_activated_ability(player, card, -1, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					instance->info_slot = x_value;
				}
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 3, 0, 2, 0, 0, 0);
					if( spell_fizzled != 1 ){
						instance->info_slot = -1;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				rearrange_top_x(player, player, instance->info_slot);
			}
			else if( instance->info_slot == -1 ){
					shuffle(player);
			}
	}
	return global_enchantment(player, card, event);
}

int card_spidersilk_armor(int player, int card, event_t event){

	boost_creature_type(player, card, event, -1, 0, 1, KEYWORD_REACH, BCT_CONTROLLER_ONLY);

	return global_enchantment(player, card, event);
}

int card_spiritual_focus(int player, int card, event_t event){

	if (discard_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, DISCARD_NO_NORMAL)){
		gain_life(player, 2);
		draw_some_cards_if_you_want(player, card, player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_spontaneous_generation(int player, int card, event_t event){
	/* Spontaneous Generation	|3|G
	 * Sorcery
	 * Put a 1/1 |Sgreen Saproling creature token onto the battlefield for each card in your hand. */

	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return 1;
		}
		else{
			if( hand_count[player] > 1 ){
				return 1;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, hand_count[player]);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_squee_goblin_nabob(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	check_legend_rule(player, card, event);
	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		int choice = 0;
		if( ! duh_mode(player) ){
			choice = do_dialog(player, player, card, -1, -1," Return Squee to hand\n Leave Squee be\n", 0);
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

int card_squeeze(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && is_what(affected_card_controller, affected_card, TYPE_SORCERY) ){
		COST_COLORLESS+=3;
	}

	return global_enchantment(player, card, event);
}

int card_statecraft(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				damage->info_slot = 0;
			}
			if( damage->damage_source_card != -1 && damage->damage_source_player == player &&
				damage->info_slot > 0
			  ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
					damage->info_slot = 0;
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

static const char* colored_damage_source_for_sc(int who_chooses, int player, int card, int targeting_player, int targeting_card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		int clr = dmg->initial_color;
		if( in_play(dmg->damage_source_player, dmg->damage_source_card) ){
			clr = get_color(dmg->damage_source_player, dmg->damage_source_card);
		}
		if( clr & get_card_instance(targeting_player, targeting_card)->info_slot ){
			return NULL;
		}
	}

	return EXE_STR(0x739060);//",color"
}

int card_story_circle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( ! IS_GAA_EVENT(event) || instance->info_slot < COLOR_TEST_BLACK ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION | TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = (int32_t)colored_damage_source_for_sc;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			dmg->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_W(1), 0, &td, "TARGET_DAMAGE");
}

int card_task_force(int player, int card, event_t event)
{
  if (in_play(player, card)
	  && !is_humiliated(player, card)
	  && becomes_target_of_spell_or_effect(player, card, event, player, card, ANYBODY))
	pump_until_eot(player, card, player, card, 0, 3);

  return 0;
}

int card_tectonic_break(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				impose_sacrifice(player, card, i, instance->info_slot, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_thrashing_wumpus(int player, int card, event_t event)
{
  return pestilence_impl(player, card, event, 0, COLOR_BLACK);
}

int card_thwart(int player, int card, event_t event){
	/* Thwart	|2|U|U
	 * Instant
	 * You may return three |H1Islands you control to their owner's hand rather than pay ~'s mana cost.
	 * Counter target spell. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) > 2 ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
				choice = do_dialog(player, player, card, -1, -1, " Bounce 3 Islands\n Cast normally\n Cancel", 0);
			}
			if( choice == 0 ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_LAND);
				td.allowed_controller = player;
				td.preferred_controller = player;
				td.required_subtype = SUBTYPE_ISLAND;
				td.illegal_abilities = 0;

				instance->number_of_targets = 0;

				int trgs = 0;
				while( trgs < 3 ){
						if( new_pick_target(&td, "Select an Island to return to your hand.", trgs, GS_LITERAL_PROMPT) ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
							trgs++;
						}
						else{
							break;
						}
				}
				int i;
				for(i=0; i<trgs; i++){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					if( trgs == 3 ){
						bounce_permanent(instance->targets[i].player, instance->targets[i].card);
					}
				}
				if( trgs < 3 ){
					spell_fizzled = 1;
					return 0;
				}
				instance->number_of_targets = 0;
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
		if( spell_fizzled != 1 ){
			return counterspell(player, card, event, NULL, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return counterspell(player, card, event, NULL, 0);
	}

	return 0;
}

int card_tooth_of_ramos(int player, int card, event_t event){

	return part_of_ramos(player, card, event, COLOR_WHITE);
}

int card_toymaker(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.illegal_type = TYPE_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			artifact_animation(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0, 0, 0, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_trade_routes(int player, int card, event_t event){
	/*
	  Trade Routes |1|U
	  Enchantment
	  {1}: Return target land you control to its owner's hand.
	  {1}, Discard a land card: Draw a card.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	test_definition_t test;
	new_default_test_definition(&test, TYPE_LAND, "Select a land card to discard.");
	test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) ){
		if( can_target(&td) ){
			return 1;
		}
		return check_battlefield_for_special_card(player, card, player, 0, &test);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Return a target land to your hand", can_target(&td), 5,
							"Discard a land card and draw", check_battlefield_for_special_card(player, card, player, 0, &test), 5 + (7-hand_count[player]));
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if( choice == 1 ){
				new_pick_target(&td, "Select target land you control.", 0, 1 | GS_LITERAL_PROMPT);
			}
			if( choice == 2 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
				if( selected != -1 ){
					discard_card(player, selected);
				}
				else{
					spell_fizzled = 1;
				}
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td) ){
			action_on_target(player, card, 0, ACT_BOUNCE);
		}
		if( instance->info_slot == 2 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_two_headed_dragon(int player, int card, event_t event){
	/* Two-Headed Dragon	|4|R|R
	 * Creature - Dragon 4/4
	 * Flying
	 * Menace
	 * ~ can block an additional creature each combat.
	 * |1|R: ~ gets +2/+0 until end of turn. */

	menace(player, card, event);

	creature_can_block_additional(player, card, event, 1);

	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 2, 0, 0, 0);
}

int card_undertaker(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && hand_count[player] > 0 && count_graveyard_by_type(player, TYPE_CREATURE) > 0
	  ){
		return ! graveyard_has_shroud(2);
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
				if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
					discard(player, 0, player);
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
	}
	return 0;
}

int card_unmask(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	al_pitchspell(player, card, event, COLOR_TEST_BLACK, 0);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLACK, 0) ){
			pick_target(&td1, "TARGET_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_uphill_battle(int player, int card, event_t event){

	/* Uphill Battle	|2|R
	 * Enchantment
	 * Creatures played by your opponents enter the battlefield tapped. */

	permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_CREATURE, NULL);

	return global_enchantment(player, card, event);
}

int card_vendetta(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 1) ){
				int amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
				lose_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vine_dryad(int player, int card, event_t event){

	al_pitchspell(player, card, event, COLOR_TEST_GREEN, 0);

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! casting_al_pitchspell(player, card, event, COLOR_TEST_GREEN, 0) ){
			spell_fizzled = 1;
		}
	}

	return flash(player, card, event);
}

static int war_tax_legacy(int player, int card, event_t event){
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_war_tax(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_XU(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS ){
			ai_modifier+=15;
		}
		else{
			ai_modifier-=50;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
		parent->targets[1].player = instance->info_slot;
		create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &war_tax_legacy,
										instance->parent_controller, instance->parent_card);
	}

	if( instance->targets[1].player > 0 ){
		if( ! is_humiliated(player, card) ){
			tax_attack(player, card, event);
		}

		if( eot_trigger(player, card, event) ){
			instance->targets[1].player = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_waterfront_bouncer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, 0, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_wave_of_reckoning(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);


	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int p;
			for( p = 0; p < 2; p++){
				int count = active_cards_count[p]-1;
				while( count > -1 ){
						if( is_what(p, count, TYPE_CREATURE) && in_play(p, count) ){
							int amount = get_power(p, count);
							damage_creature(p, count, amount, p, count);
						}
						count--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}
