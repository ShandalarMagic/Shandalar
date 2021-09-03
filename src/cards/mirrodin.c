#include "manalink.h"

// GLOBAL FUNCTIONS

void affinity(int player, int card, event_t event, int type, int subtype){

	if( event == EVENT_MODIFY_COST ){
		COST_COLORLESS-=count_subtype(player, type, subtype);
	}
}

static int clockwork(int player, int card, event_t event, int counters){

	card_instance_t *instance = get_card_instance( player, card );

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, counters);

	if( event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card) ){
		instance->targets[5].player = 66;
	}

	if( current_turn != player && event == EVENT_DECLARE_BLOCKERS && instance->blocking < 255 ){
		instance->targets[5].player = 66;
	}

	if( trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		int trig = 0;
		if( instance->targets[5].player == 66 && count_1_1_counters(player, card) > 0 ){
			trig = 1;
		}
		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					remove_1_1_counter(player, card);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[5].player = 0;
	}

	return 0;
}

int get_average_cmc(int player){

	int *deck = deck_ptr[player];
	int costs[16];
	int i;
	for(i=0; i<16; i++){
		costs[i] = 0;
	}
	int cd = count_deck(player);
	while( cd > -1 ){
			if( ! is_what(-1, deck[cd], TYPE_LAND) ){
				costs[get_cmc_by_id(cards_data[deck[cd]].id)]++;
			}
			cd--;
	}
	int par = 0;
	int result = -1;
	i=0;
	for(i=0; i<16; i++){
		if( costs[i] > par ){
			result = i;
			par = costs[i];
		}
	}
	return result;
}

int nims(int player, int card, event_t event){

	modify_pt_and_abilities(player, card, event, count_permanents_by_type(player, TYPE_ARTIFACT), 0, 0);

	return 0;
}

static int get_average_power(int player){
	int count = 0;
	int pows[16];
	int i;
	for(i=0; i<16; i++){
		pows[i] = 0;
	}
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
				pows[get_power(player, count)]++;
			}
			count++;
	}
	int par = 0;
	int result = -1;
	i=0;
	for(i=0; i<16; i++){
		if( pows[i] > par ){
			result = i;
			par = pows[i];
		}
	}
	return result;
}

static int spellbomb(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	return 0;
}


// CARDS

int card_aether_vial(int player, int card, event_t event){

	/* At the beginning of your upkeep, you may put a charge counter on ~.
	 * |T: You may put a creature card with converted mana cost equal to the number of charge counters on ~ from your hand onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.zone = TARGET_ZONE_HAND;

	char buffer[100];
	scnprintf(buffer, 100, "Select a creature with cmc %d.", count_counters(player, card, COUNTER_CHARGE));
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
	this_test.cmc = count_counters(player, card, COUNTER_CHARGE);
	this_test.zone = TARGET_ZONE_HAND;

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int AI_choice = 0;
			int most_common_cmc = get_most_common_cmc_in_hand(player, TYPE_CREATURE);
			if( count_counters(player, card, COUNTER_CHARGE) >= most_common_cmc ){
				AI_choice = 1;
			}

			char buffer2[50];
			snprintf(buffer2, 50, " Add a counter (currently at %d)\n Do not add",  count_counters(player, card, COUNTER_CHARGE));
			int choice = do_dialog(player, player, card, -1, -1, buffer2, AI_choice);
			if( choice == 0 ){
				add_counter(player, card, COUNTER_CHARGE);
			}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_altar_of_shadows(int player, int card, event_t event){

	/* Altar of Shadows	|7
	 * Artifact
	 * At the beginning of your precombat main phase, add |B to your mana pool for each charge counter on ~.
	 * |7, |T: Destroy target creature. Then put a charge counter on ~. */

	card_instance_t* instance = get_card_instance(player, card);

	if( current_phase == PHASE_MAIN1 && current_turn == player && instance->targets[5].player != 66 ){
		int counters = count_counters(player, card, COUNTER_CHARGE);
		if (counters > 0){
			produce_mana(player, COLOR_BLACK, counters);
		}
		instance->targets[5].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[5].player = 0;
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			add_counter(player, card, COUNTER_CHARGE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 7, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_arc_slogger(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && count_deck(player) > 9 ){
			return can_target(&td);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				int i;
				for(i=0; i<10;i++){
					rfg_top_card_of_deck(player);
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 2);
			 }
	}

	return 0;
}

int card_assert_autority(int player, int card, event_t event){
	affinity(player, card, event, TYPE_ARTIFACT, -1);
	return card_dissipate(player, card, event);
}

int card_aether_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) && can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Draw a card\n Bounce creature\n Do nothing", 1);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 1-choice, 0, choice, 0, 0, 0) ){
					if( choice == 0 ){
						instance->targets[1].player = 66;
						kill_card(player, card, KILL_SACRIFICE);
					}
					if( choice == 1 ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							instance->targets[1].player = 67;
							kill_card(player, card, KILL_SACRIFICE);
						}
					}
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 ){
				draw_cards(player, 1);
			}
			else if( instance->targets[1].player == 67 ){
					if( valid_target(&td) ){
						bounce_permanent(instance->targets[0].player, instance->targets[0].card);
					}
			}
	}

	return spellbomb(player, card, event);
}

int card_auriok_bladewarden(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = get_power(player, instance->parent_card);
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_auriok_steelshaper(int player, int card, event_t event){

	if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
		if( equipments_attached_to_me(player, card, EATM_CHECK) ){
			if( affected_card_controller == player && ( has_creature_type( affected_card_controller, affected_card, SUBTYPE_SOLDIER) ||
				has_creature_type( affected_card_controller, affected_card, SUBTYPE_KNIGHT) )
			  ){
				event_result++;
			}
		}
	}

	return 0;
}

int card_auriok_transfixer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(1), 0, &td, "TARGET_ARTIFACT");
}

static int awe_strike_legacy(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_card == instance->targets[0].card &&
				damage->damage_source_player == instance->targets[0].player &&
				damage->info_slot > 0
			  ){
				gain_life(player, damage->info_slot);
				damage->info_slot = 0;
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_awe_strike(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &awe_strike_legacy,
											instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_barter_in_blood(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0;i<2;i++){
			int p = player;
			if( i == 1){ p = 1-player; }
			impose_sacrifice(player, card, p, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_banshees_blade(int player, int card, event_t event)
{
  /* Banshee's Blade	|2
   * Artifact - Equipment
   * Equipped creature gets +1/+1 for each charge counter on ~.
   * Whenever equipped creature deals combat damage, put a charge counter on ~.
   * Equip |2 */

  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE)))
	add_counters(player, card, COUNTER_CHARGE, packets);

  int counters = count_counters(player, card, COUNTER_CHARGE);
  return vanilla_equipment(player, card, event, 2, counters,counters, 0,0);
}

int card_betrayal_of_flesh(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			return 1;
		}
		else{
			return can_target(&td);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = -1;
		instance->targets[0].card = -1;
		instance->targets[1].player = -1;
		instance->targets[1].card = -1;
		int choice = 0;
		int entwine = 0;
		int ai_choice = 1;
		if( can_target(&td) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Kill a creature\n Reanimate a creature\n");
				if( count_permanents_by_type(player, TYPE_LAND) > 2 ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine && can_sacrifice_as_cost(player, 3, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			int i;
			for(i=0; i<3; i++){
				sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}

		if( choice == 0 || choice == 2){
			if( choice == 2 ){
				td.allow_cancel = 0;
			}
			pick_target(&td, "TARGET_CREATURE");
		}
		if( choice == 1 || choice == 2 ){
			int can_cancel = 1;
			if( choice == 2 ){
				can_cancel = 0;
			}
			int selected = select_a_card(player, player, 2, can_cancel, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected != -1 ){
				const int *grave = get_grave(player);
				instance->targets[1].player = selected;
				instance->targets[1].card = grave[selected];
			}
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].player != -1 ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		if( instance->targets[1].player != -1 ){
			const int *grave = get_grave(player);
			int selected = instance->targets[1].player;
			if( instance->targets[1].card == grave[selected] ){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int blinding_beam_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && current_turn == instance->targets[0].player ){
		if( affected_card_controller == instance->targets[0].player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *this = get_card_instance(affected_card_controller, affected_card);
			this->untap_status &= ~3;
		}
	}

	if( event == EVENT_END_OF_UNTAP_STEP && current_turn == instance->targets[0].player ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_blinding_beam(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 2, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[2] = {target_available(player, card, &td) > 1, can_target(&td1)};
		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Tap creatures", abilities[0], 10,
							"Exhaustion for creatures", abilities[1], 5,
							"Entwine", abilities[0] && abilities[1] && has_mana(player, COLOR_COLORLESS, 1), 15);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 3 ){
			charge_mana(player, COLOR_COLORLESS, 1);
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
			if( choice & 1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
					new_pick_target(&td, "TARGET_CREATURE", 1, 1);
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				}
			}
			if( choice & 2 ){
				new_pick_target(&td1, "TARGET_PLAYER", (choice == 3 ? 2 : 0), 1);
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			int i;
			for(i=0; i<2; i++){
				if( validate_target(player, card, &td, i) ){
					tap_card(instance->targets[i].player, instance->targets[i].card);
				}
			}
		}
		if( instance->info_slot & 2 ){
			int ttv = instance->info_slot == 3 ? 2 : 0;
			if( validate_target(player, card, &td1, ttv) ){
				int legacy = create_legacy_effect(player, card, &blinding_beam_effect);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0].player = instance->targets[ttv].player;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_blinkmoth_urn(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[5].player = 66;
	}

	if( ! is_tapped(player, card) && current_phase == PHASE_MAIN1 && instance->targets[5].player != 66 ){
		produce_mana(current_turn, COLOR_COLORLESS, count_subtype(current_turn, TYPE_ARTIFACT, -1));
		instance->targets[5].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[5].player = 0;
	}

	return 0;
}

int card_blinkmoth_well(int player, int card, event_t event){

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT);
	td2.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && can_target(&td2) ){
			ai_choice = 1;
		}

		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Tap noncreature artifact\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td2, "TARGET_ARTIFACT") ){
					instance->info_slot = 1;
					instance->number_of_targets = 1;
				}
				else{
					 untap_card_no_event(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				if( valid_target(&td2) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				parent->info_slot = 0;
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

int card_bloodscent(int player, int card, event_t event)
{
  /* Bloodscent	|3|G
   * Instant
   * All creatures able to block target creature this turn do so. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  return vanilla_pump(player, card, event, &td, 0,0, 0,SP_KEYWORD_LURE);
}

int card_bosh_iron_golem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 1, 0) && can_target(&td1) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_ARTIFACT") ){
				instance->targets[1].player = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
				instance->number_of_targets = 1;
				pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
				instance->number_of_targets = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td1) ){
				damage_creature_or_player(player, card, event, instance->targets[1].player);
			 }
	}

	return 0;
}

int card_broodstar(int player, int card, event_t event)
{
  /* Broodstar	|8|U|U
   * Creature - Beast 100/100
   * Affinity for artifacts
   * Flying
   * ~'s power and toughness are each equal to the number of artifacts you control. */

  affinity(player, card, event, TYPE_ARTIFACT, -1);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	event_result += count_subtype(player, TYPE_ARTIFACT, -1);

  return 0;
}

int card_chalice_of_the_void(int player, int card, event_t event){

	/* Chalice of the Void	|X|X
	 * Artifact
	 * ~ enters the battlefield with X charge counters on it.
	 * Whenever a player casts a spell with converted mana cost equal to the number of charge counters on ~, counter that spell. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			if( instance->targets[1].player < 0 ){
				instance->targets[1].player = get_average_cmc(1-player);
			}
			if( ! has_mana(player, COLOR_ARTIFACT, instance->targets[1].player*2) ){
				ai_modifier-=1000;
			}
			else{
				charge_mana(player, COLOR_ARTIFACT, instance->targets[1].player*2);
				if( spell_fizzled != 1 ){
					instance->info_slot = instance->targets[1].player;
				}
			}
		}
		else{
			instance->info_slot = (charge_mana_for_double_x(player, COLOR_ARTIFACT) / 2);
		}
		enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);
	}

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
			get_cmc(trigger_cause_controller, trigger_cause) == count_counters(player, card, COUNTER_CHARGE)
		  ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

int card_chrome_mox(int player, int card, event_t event){

	char msg[100] = "Select a nonartifact, nonland card to exile.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND | TYPE_ARTIFACT, msg);
	this_test.type_flag = DOESNT_MATCH;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance( player, card);

	if( player == AI && event == EVENT_MODIFY_COST ){
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test)  ){
			infinite_casting_cost();
		}
	}

	if( comes_into_play(player, card, event) && check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
		int selected = -1;

		if( player == HUMAN ){
			selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
		}
		else{
			int count = 0;
			int max_color = -1;
			while( count < active_cards_count[player] ){
					if( in_hand(player, count) && new_make_test_in_play(player, count, -1, &this_test) ){
						int clr = num_bits_set(get_color(player, count) & COLOR_TEST_ANY_COLORED);
						if( clr > max_color ){
							max_color = clr;
							selected = count;
							if( max_color == 5 ){
								break;
							}
						}
					}
					count++;
			}
		}

		if( selected != -1 ){
			instance->info_slot = get_color(player, selected) & COLOR_TEST_ANY_COLORED;
			rfg_card_in_hand(player, selected);
		}
	}
	return mana_producer(player, card, event);
}

int card_clockwork_beetle(int player, int card, event_t event){

	return clockwork(player, card, event, 2);
}

int card_clockwork_dragon(int player, int card, event_t event){

	return clockwork(player, card, event, 6) + generic_shade(player, card, event, 0, 3, 0, 0, 0, 0, 0, 101, 101, 0, 0);
}

int card_cloudpost(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
		declare_mana_available(player, COLOR_COLORLESS, count_subtype(2, TYPE_LAND, SUBTYPE_LOCUS) );
	}

	if(event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE){
		produce_mana_tapped(player, card, COLOR_COLORLESS, count_subtype(2, TYPE_LAND, SUBTYPE_LOCUS));
	}

	return 0;
}

int card_confusion_in_the_ranks(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( !(trigger_cause_controller == player && trigger_cause == player) ){
			if( specific_cip(player, card, event, 2, 2, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
				card_instance_t *instance = get_card_instance( player, card );

				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_PERMANENT );
				td.required_type = get_type(instance->targets[1].player, instance->targets[1].card);
				td.preferred_controller = 1-instance->targets[1].player;
				td.allowed_controller = 1-instance->targets[1].player;
				td.allow_cancel = 0;
				td.who_chooses = instance->targets[1].player;

				instance->number_of_targets = 0;

				if( can_target(&td) ){
					pick_target(&td, "TARGET_PERMANENT");
					instance->number_of_targets = 1;
					if( instance->targets[1].player == player ){
						exchange_control_of_target_permanents(player, card, instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
					} else {
						exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_consume_spirit(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_BLACK, -1);
			if( spell_fizzled != 1 ){
				set_x_for_x_spells(player, card, event, instance->info_slot);
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && instance->info_slot > 0){
			damage_target0(player, card, instance->info_slot);
			gain_life(player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}
int card_crystal_shard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_U(1)) || has_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
				return can_target(&td);
			}
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
				if( has_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
					choice = do_dialog(player, player, card, -1, -1, " Pay U\n Pay 3\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				int u = choice == 0 ? 1 : 0;
				int x = choice == 1 ? 3 : 0;
				if( charge_mana_for_activated_ability(player, card, MANACOST_XU(x,u)) && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				int bounce = 1;
				if( has_mana(instance->targets[0].player, COLOR_COLORLESS, 1) ){
					int ai_choice = instance->targets[0].player != player ? 0 : 1;
					int choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Pay 1\n Pass", ai_choice);
					if( ! choice ){
						charge_mana(instance->targets[0].player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 ){
							bounce = 0;
						}
					}
				}
				if( bounce == 1 ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			}
	}

	return 0;
}

int card_culling_scales(int player, int card, event_t event){
/*
Culling Scales |3
Artifact
At the beginning of your upkeep, destroy target nonland permanent with the lowest converted mana cost.
(If two or more permanents are tied for lowest cost, target any one of them.)
*/
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->targets[0].player = instance->targets[0].card = -1;
		instance->number_of_targets = 0;

		int i;
		int doubles_count = 0;
		int min_t = 100;
		int count;
		for(i=0; i<2; i++){
			count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && ! is_what(i, count, TYPE_LAND) ){
						if( get_cmc(i, count) < min_t ){
							min_t = get_cmc(i, count);
							doubles_count = 0;
							if( would_validate_arbitrary_target(&td, i, count) ){
								instance->targets[0].player = i;
								instance->targets[0].card = count;
								doubles_count = 1;
							}
						}
						else if( get_cmc(i, count) == min_t ){
								if( would_validate_arbitrary_target(&td, i, count) ){
									if( doubles_count == 0 ){
										instance->targets[0].player = i;
										instance->targets[0].card = count;
										doubles_count = 1;
									}
									else{
										doubles_count++;
									}
								}
						}
					}
					count--;
			}
		}

		if( doubles_count > 1 ){
			td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
			td.extra = min_t;

			char buffer[100];
			scnprintf(buffer, 100, "Select target permanent with CMC %d", min_t);

			new_pick_target(&td, buffer, 0, GS_LITERAL_PROMPT);
		}

		if( instance->targets[0].player > -1 ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_damping_matrix(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE | TYPE_ARTIFACT) ){
							disable_nonmana_activated_abilities(i, count, 1);
						}
						count--;
				}
			}
	}

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE | TYPE_ARTIFACT) ){
		disable_nonmana_activated_abilities(affected_card_controller, affected_card, 1);
	}

	if( leaves_play(player, card, event) ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE | TYPE_ARTIFACT) ){
							disable_nonmana_activated_abilities(i, count, 0);
						}
						count--;
				}
			}
	}
	return 0;
}

int card_deconstruct(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			produce_mana(player, COLOR_GREEN, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int disciple_of_the_vault_legacy(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if (trigger_condition == TRIGGER_LEAVE_PLAY && instance->damage_target_player > -1 &&
		trigger_cause_controller == instance->damage_target_player && trigger_cause == instance->damage_target_card &&
		! is_humiliated(instance->damage_target_player, instance->damage_target_card)
	  ){
		instance->damage_target_player = instance->damage_target_card = -1;	//detach
	}

	if( instance->damage_target_player == -1 || (instance->damage_target_player > -1 && ! is_humiliated(instance->damage_target_player, instance->damage_target_card))
	  ){

	  if (event == EVENT_GRAVEYARD_FROM_PLAY)
		{
		  card_instance_t* affected;
		  if (!(affected = in_play(affected_card_controller, affected_card)))
			return 0;

		  if (is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
			  && affected->kill_code > 0 && affected->kill_code < KILL_REMOVE)
			{
			  instance = get_card_instance(player, card);
			  if (instance->targets[11].player < 0)
				instance->targets[11].player = 0;

			  ++instance->targets[11].player;
			}
		}

	  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card)
		  && instance->targets[11].player > 0)
		{
		  target_definition_t td;
		  default_target_definition(instance->damage_source_player, instance->damage_source_card, &td, 0);
		  td.zone = TARGET_ZONE_PLAYERS;

		  if (would_validate_arbitrary_target(&td, 1-player, -1))
			{
			  if (event == EVENT_TRIGGER)
				event_result |= duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			  else if (event == EVENT_RESOLVE_TRIGGER)
				for (; instance->targets[11].player > 0; --instance->targets[11].player)
				  lose_life(1-player, 1);
			}
		}
	}

	if (event == EVENT_STATIC_EFFECTS && !in_play(instance->damage_source_player, instance->damage_source_card) && instance->targets[11].player <= 0)
		kill_card(player, card, KILL_REMOVE);

	return 0;
}

int card_disciple_of_the_vault(int player, int card, event_t event)
{
  // Whenever an artifact is put into a graveyard from the battlefield, you may have target opponent lose 1 life.
  if (event == EVENT_RESOLVE_SPELL)
	create_targetted_legacy_effect(player, card, &disciple_of_the_vault_legacy, player, card);

  return 0;
}

int card_dreams_grip(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td.preferred_controller = player;


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		if( can_target(&td) ){
			if( can_target(&td1) ){
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Tap\n Untap\n");
				if( has_mana(player, COLOR_COLORLESS, 1) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, 2);
				if( choice == 2 && ! entwine ){
					choice++;
				}
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			charge_mana(player, COLOR_COLORLESS, 1);
		}

		if( choice == 0 || choice == 2){
			pick_target(&td, "TARGET_PERMANENT");
			instance->targets[1] = instance->targets[0];
			if( choice == 2 ){
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			}
			else{
				instance->targets[0].player = -1;
				instance->targets[0].card = -1;
			}
		}
		if( choice == 1 || choice == 2 ){
			if( choice == 2 ){
				td1.allow_cancel = 0;
			}
			pick_target(&td1, "TARGET_PERMANENT");
			if( choice == 2 ){
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
			}
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[1].player > -1 && validate_target(player, card, &td, 1) ){
			tap_card(instance->targets[1].player, instance->targets[1].card);
		}
		if( instance->targets[0].player > -1 && validate_target(player, card, &td1, 0) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dross_harvester(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && life[player] < 9 ){
		ai_modifier-=100;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int amount = instance->targets[11].card;
		gain_life(player, 2*amount);
		instance->targets[11].card = 0;
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		lose_life(player, 4);
	}

	return 0;
}

int card_dross_scorpion(int player, int card, event_t event){
	/*
	  Dross Scorpion |4
	  Artifact Creature - Scorpion 3/1
	  Whenever Dross Scorpion or another artifact creature dies, you may untap target artifact.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
		  ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				instance->targets[11].player++;
			}
		}
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.preferred_controller = player;

		if( resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			int i;
			for(i=0; i<instance->targets[11].player; i++){
				if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
					untap_card(instance->targets[0].player, instance->targets[0].card);
					instance->number_of_targets = 1;
				}
			}
			instance->targets[11].player = 0;
		}
	}

	return 0;
}

int card_duplicant(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_type = TARGET_TYPE_TOKEN;
		td.special = player == AI ? TARGET_SPECIAL_NOT_ME : 0;

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			state_untargettable(player, card, 1);
			if( can_target(&td) && new_pick_target(&td, "Select target nontoken creature.", 0, GS_LITERAL_PROMPT) ){
				real_set_pt(player, card, player, card, get_base_power(instance->targets[0].player, instance->targets[0].card),
							get_base_toughness(instance->targets[0].player, instance->targets[0].card), 0);
				card_ptr_t* c = cards_ptr[ get_id(instance->targets[0].player, instance->targets[0].card) ];
				instance->targets[2].player = c->types[0];
				instance->targets[3].player = c->types[1];
				instance->targets[4].player = c->types[2];
				instance->targets[5].player = c->types[3];
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
			state_untargettable(player, card, 0);
			instance->number_of_targets = 0;
		}
	}

	if( instance->targets[1].player > -1 || instance->targets[1].card > -1 ){
		if( event == EVENT_POWER && affect_me(player, card) ){
			int plus = instance->targets[1].player-2;
			event_result += plus;
		}

		if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
			int plus = instance->targets[1].card-4;
			event_result += plus;
		}
	}

	return 0;
}

int card_electrostatic_bolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int damage = 2;
			if( is_what(instance->targets[0].player, instance->targets[0].card, TYPE_ARTIFACT) ){
				damage = 4;
			}
			damage_creature(instance->targets[0].player, instance->targets[0].card, damage, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_elf_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 1, 0, 0, 1, 0, 0, 0, &td, "TARGET_ENCHANTENT");
}

int card_extraplanar_lens(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_RESOLVE_SPELL ){
		int good =  0;
		if( player != AI ){
			if( pick_target(&td, "TARGET_LAND") ){
				good = 1;
			}
		}
		else{
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_tapped(player, count) && is_basic_land(player, count) ){
						instance->targets[0].player = player;
						instance->targets[0].card = count;
						good = 1;
						break;
					}
					count--;
			}
			if( instance->targets[0].card < 0 ){
				count = active_cards_count[player]-1;
				while( count > -1 ){
						if( in_play(player, count) && is_what(player, count, TYPE_LAND) ){
							instance->targets[0].player = player;
							instance->targets[0].card = count;
							good = 1;
							break;
						}
						count--;
				}
			}
		}
		if( good == 1 ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			instance->targets[1].card = id;
			create_card_name_legacy(player, card, id);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	if( in_play(player, card) && instance->targets[1].card > 0 ){
		// do the mana part
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			if( get_id(affected_card_controller, affected_card) == instance->targets[1].card ){
				return card_mana_flare(player, card, event);
				/*
				card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
				if( event == EVENT_TAP_CARD ){
					produce_mana_from_colors(affected_card_controller, affected_card, card_d->color);
				}
				else if (event == EVENT_COUNT_MANA ){
						if( ! is_tapped(affected_card_controller, affected_card) ){
							int i;
							for(i=0; i<6){
								int clr = 0;
								if( card_d->color & (1 >> i) ){
									clr |= i;
							declare_mana_available(affected_card_controller, instance->targets[9].player, 1);
						}
				}
				*/
			}
		}

	}

	return 0;
}

int card_farsight_mask(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_player == player && damage->damage_source_player == 1-player && damage->damage_target_card == -1 ){
				int good = damage->info_slot;
				if( good == 0 && in_play(damage->damage_source_player, damage->damage_source_card) ){
					card_instance_t *source = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( source->targets[16].player > 0 ){
						good = 1;
					}
				}
				if( good > 0 ){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
		}
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && ! is_tapped(player, card) &&
		reason_for_trigger_controller == player && instance->targets[1].player > 0
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				draw_some_cards_if_you_want(player, card, player, instance->targets[1].player);
				instance->targets[1].player = 0;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_fabricate(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fiery_gambit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int flips = 0;
			while(1){
					int result = flip_a_coin(player, card);
					if( result == 0 ){
						flips = 0;
						break;
					}
					else{
						flips++;
						int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
						if( choice == 1 ){
							break;
						}
					}
					if( flips == 3 ){
						break;
					}
			}
			if( flips > 0 ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
			}
			if( flips > 1 ){
				damage_player(1-player, 6, player, card);
			}
			if( flips > 2 ){
				draw_cards(player, 9);
				manipulate_all(player, card, player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fireshrieker(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 2, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
}

int card_frogmite(int player, int card, event_t event){
	/*
	  Frogmite |4
	  Artifact Creature - Frog 2/2
	  Affinity for artifacts (This spell costs {1} less to cast for each artifact you control.)
	*/
	affinity(player, card, event, TYPE_ARTIFACT, -1);
	return 0;
}

int card_fists_of_the_anvil(int player, int card, event_t event)
{
  /* Fists of the Anvil	|1|R
   * Instant
   * Target creature gets +4/+0 until end of turn. */
  return vanilla_instant_pump(player, card, event, ANYBODY, player, 4,0, 0,0);
}

int card_gate_to_aether(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[current_turn];
		show_deck( HUMAN, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
		if( is_what(-1, deck[0], TYPE_PERMANENT) ){
			int choice = do_dialog(current_turn, player, card, -1, -1, " Put into play\n Pass", 0);
			if( choice == 0 ){
				int card_added = add_card_to_hand(current_turn, deck[0]);
				remove_card_from_deck(current_turn, 0);
				put_into_play(current_turn, card_added);
			}
		}
	}

	return 0;
}

int card_gilded_lotus(int player, int card, event_t event)
{
  // 0x4dbf30

  /* Gilded Lotus	|5
   * Artifact
   * |T: Add three mana of any one color to your mana pool. */

  return artifact_mana_all_one_color(player, card, event, 3, 0);
}

int card_glimmervoid(int player, int card, event_t event)
{
  /* Glimmervoid	""
   * Land
   * At the beginning of the end step, if you control no artifacts, sacrifice ~.
   * |T: Add one mana of any color to your mana pool. */

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player
	  && count_permanents_by_type(player, TYPE_ARTIFACT) <= 0 && eot_trigger(player, card, event))
	kill_card(player, card, KILL_SACRIFICE);

  return mana_producer(player, card, event);
}

int card_goblin_charbelcher(int player, int card, event_t event){
	/* Goblin Charbelcher	|4
	 * Artifact
	 * |3, |T: Reveal cards from the top of your library until you reveal a land card. ~ deals damage equal to the number of nonland cards revealed this way to target creature or player. If the revealed land card was |Ha Mountain, ~ deals double that damage instead. Put the revealed cards on the bottom of your library in any order. */

	if( ! is_unlocked(player, card, event, 12) ){ return 0; }

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td1, 0) ){
				int *deck = deck_ptr[player];
				int i = 0;
				int dmg = 0;
				while( deck[i] != -1 ){
						if( !( cards_data[ deck[i] ].type & TYPE_LAND ) ){
							dmg++;
						}
						else{
							break;
						}
						i++;
				}
				if( deck[i] != -1 ){
					if( has_subtype_by_id(cards_data[ deck[i] ].id, SUBTYPE_MOUNTAIN) ){
						dmg *= 2;
					}
				}

				if( dmg > 0 ){
					damage_creature_or_player(player, card, event, dmg);
				}

				if( i > 0 ){
					put_top_x_on_bottom(player, player, i+1);
				}
			}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

int card_goblin_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 3, 0, 0, 0, 1, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_golem_skin_gauntlets(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( is_equipping(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result += equipments_attached_to_me(instance->targets[8].player, instance->targets[8].card, EATM_REPORT_TOTAL);
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_grab_the_reins(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller = player;
	td2.preferred_controller = player;
	td2.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && (can_target(&td) || (can_target(&td2) && can_target(&td1)) ) ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 0;
		if( can_target(&td) ){
			if( (can_target(&td2) && can_target(&td1)) ){
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Steal a creature\n Fling a creature\n");
				if( has_mana_multi(player, 2, 0, 0, 0, 1, 0) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				if( entwine ){
					ai_choice = 2;
				}
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			charge_mana_multi(player, 2, 0, 0, 0, 1, 0);
		}

		if( choice == 0 || choice == 2){
			pick_target(&td, "TARGET_CREATURE");
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
		}
		if( choice == 1 || choice == 2 ){
			pick_target(&td1, "TARGET_PLAYER");
			instance->targets[2].player = instance->targets[0].player;
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) ){
			effect_act_of_treason(player, card, instance->targets[1].player, instance->targets[1].card);
		}
		if( validate_target(player, card, &td1, 2) ){
			int meek = pick_creature_for_sacrifice(player, card, 1);
			int dmg = get_power(instance->targets[0].player, instance->targets[0].card);
			kill_card(player, meek, KILL_SACRIFICE);
			damage_player(instance->targets[2].player, dmg, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_granite_shard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) || has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
				return can_target(&td);
			}
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Pay R\n Pay 3\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 3*choice, 0, 0, 0, 1-choice, 0) ){
					if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
						instance->number_of_targets = 1;
						tap_card(player, card);
					}
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 1);
			}
	}

	return 0;
}

int card_grid_monitor(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
			infinite_casting_cost();
		}
	}

	return 0;
}

int card_heartwood_shard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) || has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
				return can_target(&td);
			}
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Pay G\n Pay 3\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 3*choice, 0, 0, 1-choice, 0, 0) ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						tap_card(player, card);
					}
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
			}
	}

	return 0;
}

int card_isochron_scepter(int player, int card, event_t event){

	/* Isochron Scepter	|2
	 * Artifact
	 * Imprint - When ~ enters the battlefield, you may exile an instant card with converted mana cost 2 or less from your hand.
	 * |2, |T: You may copy the exiled card. If you do, you may cast the copy without paying its mana cost. */

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_INSTANT, "Choose an instant card with CMC 2 or less.");
		this_test.cmc = 3;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		this_test.zone = TARGET_ZONE_HAND;

		int result;

		if( hand_count[player] > 0 &&
			(result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test)) != -1
		  ){
			instance->info_slot = result;
			create_card_name_legacy(player, card, result);
		}
		else {
			ai_modifier += (player == AI) ? -256 : 256;
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( instance->info_slot > 0 && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK, MANACOST_X(2), 0, NULL, NULL) ){
			return can_legally_play_csvid(player, instance->info_slot );
		}
	}

	if(event == EVENT_ACTIVATE){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK, MANACOST_X(2), 0, NULL, NULL);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		copy_spell( player, instance->info_slot );
	}

	return 0;
}

int card_journey_of_discovery(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 0;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Cultivate\n Play 2 additional lands\n");
				if( has_mana_multi(player, 2, 0, 0, 1, 0, 0) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				if( entwine ){
					ai_choice = 2;
				}
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 2 ){
			charge_mana_multi(player, 2, 0, 0, 1, 0, 0);
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		instance->info_slot = 1+choice;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 14, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		}
		if( instance->info_slot & 2 ){
			int legacy = create_legacy_effect(player, card, &check_playable_lands_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[2].card = get_id(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Krark's Thumb effect is in "flip_a_coin" function.

int card_krark_clan_shaman(int player, int card, event_t event){

	/* Krark-Clan Shaman	|R
	 * Creature - Goblin Shaman 1/1
	 * Sacrifice an artifact: ~ deals 1 damage to each creature without flying. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT);
	td2.allowed_controller = player;
	td2.preferred_controller = player;
	td2.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( ! controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
					spell_fizzled = 1;
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			damage_all(player, card, ANYBODY, 1, 0, 0, KEYWORD_FLYING,MATCH, 0,0, 0,0, 0,0, -1,0);
	}

	return 0;
}

int card_leonin_abunas(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)){
		hexproof(affected_card_controller, affected_card, event);
	}

	return 0;
}

int card_leonin_bladetrap(int player, int card, event_t event){

 //   card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				kill_card(player, card, KILL_SACRIFICE);;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int count = active_cards_count[current_turn]-1;
			while( count > -1 ){
				if( in_play(current_turn, count) && is_attacking(current_turn, count) &&
						! check_for_ability(current_turn, count, KEYWORD_FLYING)
					  ){
						damage_creature(current_turn, count, 2, player, card);
					}
					count--;
			}
	}

	return flash(player, card, event);
}

int card_leonin_den_guard(int player, int card, event_t event)
{
  // As long as ~ is equipped, it gets +1/+1 and has vigilance.
  if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)
	  && equipments_attached_to_me(player, card, EATM_CHECK))
	{
	  if (event == EVENT_ABILITIES)
		vigilance(player, card, event);
	  else
		++event_result;
	}

  return 0;
}

int card_leonin_elder(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT) ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					gain_life(player, 1);
			}
		}
	}

	return 0;
}

int card_leonin_scimitar(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 1, 1, 1, 0, 0);
}

int card_leonin_sun_standard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_lightning_coils(int player, int card, event_t event){

	/* Lightning Coils	|3
	 * Artifact
	 * Whenever a nontoken creature you control dies, put a charge counter on ~.
	 * At the beginning of your upkeep, if ~ has five or more charge counters on it, remove all of them from it and put that many 3/1 |Sred Elemental creature
	 * tokens with haste onto the battlefield. Exile them at the beginning of the next end step. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.type_flag = F1_NO_TOKEN;
		count_for_gfp_ability(player, card, event, player, 0, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counters(player, card, COUNTER_CHARGE, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( count_counters(player, card, COUNTER_CHARGE) >= 5 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int amount = count_counters(player, card, COUNTER_CHARGE);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
			token.qty = amount;
			token.pow = 3;
			token.tou = 1;
			token.s_key_plus = SP_KEYWORD_HASTE;
			token.legacy = 1;
			token.special_code_for_legacy = remove_at_eot;
			generate_token(&token);
			remove_all_counters(player, card, COUNTER_CHARGE);
		}
	}

	return 0;
}

int card_living_hive(int player, int card, event_t event){
	/* Living Hive	|6|G|G
	 * Creature - Elemental Insect 6/6
	 * Trample
	 * Whenever ~ deals combat damage to a player, put that many 1/1 |Sgreen Insect creature tokens onto the battlefield. */

	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_card == card && damage->damage_source_player == player &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  ){
		get_card_instance(player, card)->info_slot += damage->targets[16].player;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot <= 0){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			generate_tokens_by_id(player, card, CARD_ID_INSECT, instance->info_slot);
			instance->info_slot = 0;
		}
	}
	return 0;
}

int card_lodestone_myr(int player, int card, event_t event){

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT);
	td2.allowed_controller = player;
	td2.preferred_controller = player;
	td2.illegal_abilities = 0;
	td2.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_target(&td2);
	}

	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( pick_target(&td2, "TARGET_ARTIFACT") ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
					instance->number_of_targets = 1;
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 1);
	}

	return 0;
}

int card_loxodon_punisher(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += (equipments_attached_to_me(player, card, EATM_REPORT_TOTAL)*2);
	}

	return 0;
}

int card_lumengrid_augur(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;


	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int p = instance->targets[0].player;

			char msg[100] = "Select a card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);

			draw_cards(p, 1);

			if( hand_count[p] > 0 ){
				int selected = new_select_a_card(p, p, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				int untap = is_what(p, selected, TYPE_ARTIFACT);
				new_discard_card(p, selected, player, 0);
				if( untap ){
					untap_card(player, instance->parent_card);
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_lumengrid_sentinel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_cip(player, card, event, player, 1+player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 1;
		}
	}

	return 0;
}

int card_luminous_angel(int player, int card, event_t event){
	/* Luminous Angel	|4|W|W|W
	 * Creature - Angel 4/4
	 * Flying
	 * At the beginning of your upkeep, you may put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.color_forced = COLOR_TEST_WHITE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return 0;
}

int card_march_of_the_machines(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 0;
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && ! is_what(i, count, TYPE_CREATURE) ){
						card_instance_t *lnd = get_card_instance(i, count);
						int k;
						int mnt = 1;
						for(k=0; k<instance->info_slot; k++){
							if( instance->targets[k].player == lnd->internal_card_id ){
								mnt = 0;
								break;
							}
						}
						if( mnt == 1 ){
							int pos = instance->info_slot;
							if( pos < 16 ){
								int newtype = create_a_card_type(lnd->internal_card_id);
								cards_at_7c7000[newtype]->type |= (cards_data[lnd->internal_card_id].type | TYPE_CREATURE);
								cards_at_7c7000[newtype]->power = get_cmc(i, count);
								cards_at_7c7000[newtype]->toughness = get_cmc(i, count);
								instance->targets[pos].player = lnd->internal_card_id;
								instance->targets[pos].card = newtype;

								instance->info_slot++;
							}
						}

					}
					count--;
			}
		}
	}

	if( event == EVENT_CHANGE_TYPE && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
		card_instance_t *lnd = get_card_instance(affected_card_controller, affected_card);
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( lnd->internal_card_id == instance->targets[i].player ){
				lnd->token_status |= STATUS_ANIMATED;
				event_result = instance->targets[i].card;
				break;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
		! is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		){
			card_instance_t *lnd = get_card_instance(affected_card_controller, affected_card);
			int k;
			int mnt = 1;
			for(k=0; k<instance->info_slot; k++){
				if( instance->targets[k].player == lnd->internal_card_id ){
					mnt = 0;
					break;
				}
			}
			if( mnt == 1 ){
				int pos = instance->info_slot;
				if( pos < 16 ){
					int newtype = create_a_card_type(lnd->internal_card_id);
					cards_at_7c7000[newtype]->type |= (cards_data[lnd->internal_card_id].type | TYPE_CREATURE);
					cards_at_7c7000[newtype]->power = get_cmc(affected_card_controller, affected_card);
					cards_at_7c7000[newtype]->toughness = get_cmc(affected_card_controller, affected_card);
					instance->targets[pos].player = lnd->internal_card_id;
					instance->targets[pos].card = newtype;

					instance->info_slot++;
				}
			}

	}

	return global_enchantment(player, card, event);
}

int card_mask_of_memory(int player, int card, event_t event)
{
  /* Mask of Memory	|2
   * Artifact - Equipment
   * Whenever equipped creature deals combat damage to a player, you may draw two cards. If you do, discard a card.
   * Equip |1 */

  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL)))
	for (; packets > 0; --packets)
	  {
		draw_cards(player, 2);
		discard(player, 0, player);
	  }

  return basic_equipment(player, card, event, 1);
}

int card_mass_hysteria(int player, int card, event_t event)
{
  // All creatures have haste.
  boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_INCLUDE_SELF);

  return global_enchantment(player, card, event);
}

int card_megatog(int player, int card, event_t event){
	/* Megatog	|4|R|R
	 * Creature - Atog 3/4
	 * Sacrifice an artifact: ~ gets +3/+3 and gains trample until end of turn. */
	return generic_husk(player, card, event, TYPE_ARTIFACT, 3, 3, KEYWORD_TRAMPLE, 0);
}

int card_mesmeric_orb(int player, int card, event_t event)
{
  player_bits[0] |= PB_SEND_EVENT_UNTAP_CARD_TO_ALL;
  player_bits[1] |= PB_SEND_EVENT_UNTAP_CARD_TO_ALL;

  if (event == EVENT_UNTAP_CARD && !is_humiliated(player, card) && in_play(player, card))
	mill(affected_card_controller, 1);

  return 0;
}

int card_minds_eye(int player, int card, event_t event)
{
  /* Mind's Eye	|5
   * Artifact
   * Whenever an opponent draws a card, you may pay |1. If you do, draw a card. */

  if (card_drawn_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player))
	  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 1))
	draw_a_card(player);

  return 0;
}

int card_molder_slug(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_molten_rain(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int dmg = 0;
			if( ! is_basic_land(instance->targets[0].player, instance->targets[0].card) ){
				dmg = 2;
			}
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( dmg > 0 ){
				damage_player(instance->targets[0].player, dmg, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_myr_incubator(int player, int card, event_t event){
	/* Myr Incubator	|6
	 * Artifact
	 * |6, |T, Sacrifice ~: Search your library for any number of artifact cards, exile them, then put that many 1/1 colorless Myr artifact creature tokens onto the battlefield. Then shuffle your library. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

		int myr = 0;
		int choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
		int *deck = deck_ptr[player];

		if( choice == 1){
			while( deck[0] != -1 ){
					if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						myr++;
					}
					else{
						break;
					}
			}
		}
		else{
			int count = count_deck(player)-1;
			while( count > -1 ){
					if( is_what(-1, deck[count], TYPE_ARTIFACT) ){
						rfg_card_in_deck(player, count);
						myr++;
					}
					count--;
			}
		}

		generate_tokens_by_id(player, card, CARD_ID_MYR, myr);
		shuffle(player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 6, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_myr_retriver(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, CARD_ID_MYR_RETRIEVER, 1, -1, 0);
	}

	return 0;
}

int card_necrogen_mists(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		discard(current_turn, 0, player);
	}

	return global_enchantment(player, card, event);
}

int card_necrogen_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Draw a card\n Make discard\n Do nothing", 1);
			}
			if( choice == 0 ){
				charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 66;
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
			else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
					instance->targets[1].player = 67;
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
			else{
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 ){
				draw_cards(player, 1);
			}
			else if( instance->targets[1].player == 67 ){
					if( valid_target(&td) ){
						discard(instance->targets[0].player, 0, player);
					}
			}
	}

	return spellbomb(player, card, event);
}

int card_needlebug(int player, int card, event_t event){

	return flash(player, card, event);
}

int card_neurok_familiar(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int *deck = deck_ptr[current_turn];
		show_deck( HUMAN, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
		if( is_what(-1, deck[0], TYPE_ARTIFACT) ){
			add_card_to_hand(current_turn, deck[0]);
			remove_card_from_deck(current_turn, 0);
		}
		else{
			mill(player, 1);
		}
	}

	return 0;
}

int card_nightmare_lash(int player, int card, event_t event){

	/* Nightmare Lash	|4
	 * Artifact - Equipment
	 * Equipped creature gets +1/+1 for each |H2Swamp you control.
	 * Equip-Pay 3 life. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_sorcery_be_played(player, event) ){
			if( can_pay_life(player, 3) &&
				(!IS_AI(player) || life[player] >= 9)
			  ){
				return can_activate_basic_equipment(player, card, event, 0);
			}
			if( metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN) ){
				return check_for_equipment_targets(player, card);
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && activate_basic_equipment(player, card, -2) &&
			!(metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN))
		   ){
			lose_life(player, 3);
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		resolve_activation_basic_equipment(player, card);
	}

	if( is_equipping(player, card) ){
		if( (event== EVENT_POWER || event == EVENT_TOUGHNESS) &&
			affect_me(instance->targets[8].player, instance->targets[8].card)
		  ){
			event_result+=count_subtype(player, TYPE_PERMANENT, SUBTYPE_SWAMP);
		}
	}

	return 0;
}

int card_nim_devourer(int player, int card, event_t event){
/*
Nim Devourer |3|B|B
Creature — Zombie 4/1
Nim Devourer gets +1/+0 for each artifact you control.
{B}{B}: Return Nim Devourer from your graveyard to the battlefield, then sacrifice a creature.
		Activate this ability only during your upkeep.
*/
	if( player == AI ){
		if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
			if( has_mana(player, COLOR_BLACK, 2) ){
				int choice = do_dialog(player, player, card, -1, -1," Return Nim Devourer to play\n Pass", count_subtype(player, TYPE_CREATURE, -1) > 0 ? 0 : 1);
				if( choice == 0 ){
					charge_mana(player, COLOR_BLACK, 2);
					if( spell_fizzled != 1){
						remove_state(player, card, STATE_INVISIBLE);
						++hand_count[player];
						put_into_play(player, card);
						controller_sacrifices_a_creature(player, card);
						return -1;
					}
				}
			}
			return -2;
		}
	}
	else{
		if(event == EVENT_GRAVEYARD_ABILITY){
			if( has_mana(player, COLOR_BLACK, 2) && current_phase == PHASE_UPKEEP ){
				return GA_RETURN_TO_PLAY_WITH_EFFECT;
			}
		}
		if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana(player, COLOR_BLACK, 2);
			if( spell_fizzled != 1){
				return GAPAID_REMOVE;
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
			controller_sacrifices_a_creature(player, card);
		}
	}
	return nims(player, card, event);
}

int card_nim_lasher(int player, int card, event_t event){
	return nims(player, card, event);
}

int card_nim_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 2, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nim_shambler(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && instance->kill_code == KILL_DESTROY &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			return can_regenerate(player, card);
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					! controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0)
				  ){
					spell_fizzled = 1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(player, instance->parent_card);
		}
	}

	return nims(player, card, event);
}

int card_nuisance_engine(int player, int card, event_t event){
	/* Nuisance Engine	|3
	 * Artifact
	 * |2, |T: Put a 0/1 colorless Pest artifact creature token onto the battlefield. */

	// original code : 004DAC3A

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_PEST);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

void oblivion_stone_effect(void)
{
	int p, c;
	for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
			if (in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && !is_what(p, c, TYPE_LAND) && count_counters(p, c, COUNTER_FATE) == 0)
				kill_card(p, c, KILL_DESTROY);

	EXE_FN(void, 0x477a90, void)();	// regenerate_or_graveyard_triggers()

	for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
			if (in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && !is_what(p, c, TYPE_LAND))
				remove_all_counters(p, c, COUNTER_FATE);
}

int card_oblivion_stone(int player, int card, event_t event){

	/* Oblivion Stone	|3
	 * Artifact
	 * |4, |T: Put a fate counter on target permanent.
	 * |5, |T, Sacrifice ~: Destroy each nonland permanent without a fate counter on it, then remove all fate counters from all permanents. */

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	if( player == AI ){
		td.illegal_type = TYPE_LAND;
	}
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( player != AI ){
				if( has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && can_target(&td) ){
					return 1;
				}
			}
			else{
				if( has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) &&
					can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
				  ){
					return 1;
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && can_target(&td) && player != AI ){
			if( has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
				choice = do_dialog(player, player, card, -1, -1, " Put a fate counter\n Destroy all\n Do nothing", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 4+choice, 0, 0, 0, 0, 0) ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_PERMANENT") ){
						tap_card(player, card);
						instance->targets[1].player = 66;
						instance->number_of_targets = 1;
					}
				}
				else if( choice == 1 ){
						tap_card(player, card);
						instance->targets[1].player = 67;
						kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 ){
			if( valid_target(&td) ){
				add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FATE);
			}
		}
		else if( instance->targets[1].player == 67 ){
				oblivion_stone_effect();
		}
	}

	if( player == AI && current_turn != player && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && ! is_tapped(player, card) &&
		! is_animated_and_sick(player, card)
	  ){
		if( eot_trigger(player, card, event) && can_target(&td) ){
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				int par = 0;
				int result = -1;
				int count = active_cards_count[player]-1;
				while( count > -1 ){
						if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) && ! is_what(player, count, TYPE_LAND)){
							if( ! count_counters(player, count, COUNTER_FATE) && get_base_value(player, count) > par ){
								instance->targets[0].player = player;
								instance->targets[0].card = count;
								instance->number_of_targets = 1;
								if( would_valid_target(&td) ){
									par = get_base_value(player, count);
									result = count;
								}
							}
						}
						count--;
				}
				if( result > -1 ){
					tap_card(player, card);
					add_counter(player, result, COUNTER_FATE);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	return 0;
}

int card_one_dozen_eyes(int player, int card, event_t event){
	/* One Dozen Eyes	|5|G
	 * Sorcery
	 * Choose one -
	 * * Put a 5/5 |Sgreen Beast creature token onto the battlefield.
	 * * Put five 1/1 |Sgreen Insect creature tokens onto the battlefield.
	 * Entwine |G|G|G */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 1;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " One 5/5 Beast\n 5 1/1 Insects\n");
				if( has_mana(player, COLOR_GREEN, 3) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			charge_mana(player, COLOR_GREEN, 3);
		}
		instance->targets[0].player = 1+choice;

	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].player & 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 5;
			token.tou = 5;
			generate_token(&token);
		}
		if( instance->targets[0].player & 2 ){
			generate_tokens_by_id(player, card, CARD_ID_INSECT, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_override(int player, int card, event_t event){
	/* Override	|2|U
	 * Instant
	 * Counter target spell unless its controller pays |1 for each artifact you control. */

	if(event == EVENT_RESOLVE_SPELL ){
			int amount = count_subtype(player, TYPE_ARTIFACT, -1);
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, amount);
			kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_pearl_shard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1)  ){
				return 0x63;
			}
			if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
				return 0x63;
			}
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Pay W\n Pay 3\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, choice*3, 0, 0, 0, 0, 1-choice) ){
					if( pick_target(&td, "TARGET_DAMAGE") ){
						instance->number_of_targets = 1;
						tap_card(player, card);
					}
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( target->info_slot > 2 ){
					target->info_slot-=2;
				}
				else{
					target->info_slot = 0;
				}
			}
	}

	return 0;
}

int card_predators_strike(int player, int card, event_t event){
	/* Predator's Strike	|1|G
	 * Instant
	 * Target creature gets +3/+3 and gains trample until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 3, KEYWORD_TRAMPLE, 0);
}

int card_promise_of_power(int player, int card, event_t event){
	/* Promise of Power	|2|B|B|B
	 * Sorcery
	 * Choose one -
	 * * You draw five cards and you lose 5 life.
	 * * Put an X/X |Sblack Demon creature token with flying onto the battlefield, where X is the number of cards in your hand.
	 * Entwine |4 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 1;
		if( life[player] > 10 ){
			ai_choice = 0;
		}
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Draw 5 and lose 5 life\n Generate a Demon\n");
				if( has_mana(player, COLOR_COLORLESS, 4) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine && life[player] > 10 ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			charge_mana(player, COLOR_COLORLESS, 4);
		}
		instance->targets[0].player = 1+choice;

	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].player & 1 ){
			draw_cards(player, 5);
			lose_life(player, 5);
		}
		if( instance->targets[0].player & 2 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_DEMON, &token);
			token.pow = hand_count[player];
			token.tou = hand_count[player];
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_proteus_staff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
			int *deck = deck_ptr[instance->targets[0].player];
			int z;
			int card_added = -1;
			for(z=0; z < count_deck(instance->targets[0].player); z++){
				if( cards_data[deck[z]].type & TYPE_CREATURE){
					show_deck(HUMAN, deck, z+1, "These cards were revealed", 0, 0x7375B0 );
					if( ! check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE) ){
						card_added = add_card_to_hand(instance->targets[0].player, deck[z]);
						remove_card_from_deck(instance->targets[0].player, z);
						put_into_play(instance->targets[0].player, card_added);
					}
					break;
				}
			}
			if( z > 0 ){
				put_top_x_on_bottom(instance->targets[0].player, instance->targets[0].player, z);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_CAN_SORCERY_BE_PLAYED, 2, 0, 1, 0, 0, 0, 0,
									&td, "TARGET_CREATURE");
}

int card_psychic_membrane(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && instance->targets[1].player != 66 ){
		if( current_turn == 1-player && current_phase == PHASE_AFTER_BLOCKING &&
			instance->blocking < 255
		  ){
			return 1;
		}
	}

	else if(event == EVENT_ACTIVATE ){
			instance->targets[1].player = 66;
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

// Psychogenic Probe code is in "shuffle()" in "functions.c"

int card_pyrite_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Draw a card\n Damage\n Do nothing", 1);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 1-choice, 0, 0, 0, choice, 0) ){
					if( choice == 0 ){
						instance->targets[1].player = 66;
						kill_card(player, card, KILL_SACRIFICE);
					}
					if( choice == 1 ){
						if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							instance->targets[1].player = 67;
							kill_card(player, card, KILL_SACRIFICE);
						}
					}
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 ){
				draw_cards(player, 1);
			}
			else if( instance->targets[1].player == 67 ){
					if( valid_target(&td) ){
						damage_creature_or_player(player, card, event, 2);
					}
			}
	}

	return spellbomb(player, card, event);
}

int card_quicksilver_fountain(int player, int card, event_t event)
{
  /* Quicksilver Fountain	|3
   * Artifact
   * At the beginning of each player's upkeep, that player puts a flood counter on target non-|H2Island land he or she controls of his or her choice. That land
   * is |Han Island for as long as it has a flood counter on it.
   * At the beginning of each end step, if all lands on the battlefield are |H1Islands, remove all flood counters from them. */

  upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = current_turn;
	  td.preferred_controller = current_turn;
	  td.who_chooses = current_turn;
	  td.allow_cancel = 0;
	  td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_ISLAND);
	  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_LAND"))
		{
		  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FLOOD);
		  int island_iid = get_internal_card_id_from_csv_id(get_hacked_subtype(player, card, CARD_ID_ISLAND));
		  shapeshift_target(player, card, instance->targets[0].player, instance->targets[0].card, -1, island_iid, SHAPESHIFT_COUNTER(COUNTER_FLOOD));
		}
	}

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  int island_subtype = get_hacked_subtype(player, card, SUBTYPE_ISLAND);
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_LAND) && !has_subtype(p, c, island_subtype))
			return 0;

	  if (eot_trigger(player, card, event))
		{
		  // Remove all Quicksilver Fountain effect cards, so they don't all call recalculate_all_cards_in_play() during static effects
		  card_instance_t* inst;
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if ((inst = in_play(p, c)) && is_what(p, c, TYPE_EFFECT) && inst->display_pic_csv_id == CARD_ID_QUICKSILVER_FOUNTAIN)
				kill_card(p, c, KILL_REMOVE);

		  // Remove all flood counters
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_play(p, c) && is_what(p, c, TYPE_LAND))
				remove_all_counters(p, c, COUNTER_FLOOD);

		  recalculate_all_cards_in_play();	// Likely redundant
		}
	}

  // Detect new magical hacks
  if (event == EVENT_RESOLVE_SPELL)
	get_card_instance(player, card)->info_slot = get_hacked_color(player, card, COLOR_BLUE);

  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int new_hacked_color = get_hacked_color(player, card, COLOR_BLUE);
	  if (new_hacked_color != instance->info_slot)
		{
		  instance->info_slot = new_hacked_color;

		  // Update all effects
		  int island_iid = get_internal_card_id_from_csv_id(get_hacked_subtype(player, card, CARD_ID_ISLAND));

		  card_instance_t* inst;
		  int p, c, i;
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if ((inst = in_play(p, c)) && is_what(p, c, TYPE_EFFECT) && inst->damage_source_player == player && inst->damage_source_card == card)
				{
				  inst->targets[1].card = island_iid;
				  for (i = 0; i < 6; ++i)
					inst->hack_mode[i] = instance->hack_mode[i];
				}

		  recalculate_all_cards_in_play();
		}
	}

	return 0;
}

int card_raise_the_alarm(int player, int card, event_t event){
	// original code : 0x4DED1D

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_SOLDIER, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_razor_barrier(int player, int card, event_t event){

	/* Razor Barrier	|1|W
	 * Instant
	 * Target permanent you control gains protection from artifacts or from the color of your choice until end of turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		 return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				int choice = do_dialog(player, player, card, -1, -1,
									   " Prot. from Black\n Prot. from Blue\n Prot. from Green\n Prot from Red\n Prot. from White\n Prot from Artifacts",
									   get_deck_color(player, 1-player) - 1);
				instance->targets[1].card = (1 << (11+choice)) | KEYWORD_RECALC_SET_COLOR;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0,
										instance->targets[1].card, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_reiver_demon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 1 * not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot < 1 ){
			int i;
			for(i=0; i<2; i++ ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							if( ! is_what(i, count, TYPE_ARTIFACT) && !(get_color(i, count) & COLOR_TEST_BLACK) ){
								kill_card(i, count, KILL_BURY);
							}
						}
						count--;
				}
			}
		}
		else{
			instance->info_slot = 0;
		}
	}

	return 0;
}

int card_roar_of_the_kha(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 0;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Pump your dudes\n Untap your dudes\n");
				if( has_mana_multi(player, 1, 0, 0, 0, 0, 1) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			charge_mana_multi(player, 1, 0, 0, 0, 0, 1);
		}
		instance->targets[0].player = 1+choice;

	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].player & 1 ){
			pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
		}
		if( instance->targets[0].player & 2 ){
			manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rust_elemental(int player, int card, event_t event)
{
  /* Rust Elemental	|4
   * Artifact Creature - Elemental 4/4
   * Flying
   * At the beginning of your upkeep, sacrifice an artifact other than ~. If you can't, tap ~ and you lose 4 life. */

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "");
	  test.not_me = 1;

	  if (!new_sacrifice(player, card, player, SAC_NO_CANCEL, &test))
		{
		  tap_card(player, card);
		  lose_life(player, 4);
		}
	}

  return 0;
}

int card_rustmouth_ogre(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_ARTIFACT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 1;
		}
	}

	return 0;
}

int card_sculpting_steel(int player, int card, event_t event)
{
  /* Sculpting Steel	|3
   * Artifact
   * You may have ~ enter the battlefield as a copy of any artifact on the battlefield. */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	base_target_definition(player, card, &td, TYPE_ARTIFACT);

  enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_AN_ARTIFACT");

  return 0;
}

int card_scythe_of_the_wretched(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	int e_player = instance->targets[8].player;
	int e_card = instance->targets[8].card;

	if( is_equipping(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_source_card == e_card && damage->damage_source_player == e_player &&
					damage->damage_target_card != -1
				 ){
					int good = 0;
					if( damage->info_slot > 0 ){
						good = 1;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = 1;
						}
					}
					if( good == 1 ){
						if( instance->info_slot < 1 ){
							instance->info_slot = 1;
						}
						int pos = instance->info_slot;
						if( pos < 8 ){
							instance->targets[pos].player = damage->damage_target_player;
							instance->targets[pos].card = damage->damage_target_card;
							instance->info_slot++;
						}
					}
				}
			}
		}
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( in_play(affected_card_controller, affected_card) &&
				is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
				! is_token(affected_card_controller, affected_card) && instance->info_slot > 1
			  ){
				card_instance_t *dead = get_card_instance( affected_card_controller, affected_card);
				if( dead->kill_code > 0 && dead->kill_code < 4 ){
					int i;
					for(i=1; i<instance->info_slot; i++){
						if( instance->targets[i].player == affected_card_controller &&
							instance->targets[i].card == affected_card
						  ){
							if( instance->targets[11].player < 0 ){
								instance->targets[11].player = 0;
							}
							instance->targets[11].player++;
							instance->targets[i].player |= (1<<20);
							instance->targets[i].card = cards_data[get_original_internal_card_id(affected_card_controller, affected_card)].id;
							break;
						}
					}
				}
			}
		}
		if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card)
		  ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						int k;
						for(k=1; k<8; k++){
							if( instance->targets[k].player > -1 && (instance->targets[k].player & (1<<20)) ){
								instance->targets[k].player &= ~(1<<20);
								int owner = instance->targets[k].player;
								instance->targets[k].player = -1;
								int result = seek_grave_for_id_to_reanimate(player, card, owner, instance->targets[k].card, REANIMATE_DEFAULT);
								if( result > -1 ){
									equip_target_creature(player, card, player, result);
								}
							}
						}
						instance->targets[11].player = 0;
				}
		}
		if( event == EVENT_CLEANUP ){
			instance->info_slot = 1;
		}
	}

	return vanilla_equipment(player, card, event, 4, 2, 2, 0, 0);
}

int card_second_sunrise(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all_dead_this_turn(player, TYPE_PERMANENT | GDC_NONTOKEN | GDC_NONPLANESWALKER);
		reanimate_all_dead_this_turn(1-player, TYPE_PERMANENT | GDC_NONTOKEN | GDC_NONPLANESWALKER);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_seething_song(int player, int card, event_t event){
	/*
	  Seething Song |2|R
	  Instant
	  Add {R}{R}{R}{R}{R} to your mana pool.
	*/
	// just the bare skeleton, probably needs AI hint, Korath please do it
	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_RED, 5);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_serum_tank(int player, int card, event_t event){

	/* Serum Tank	|3
	 * Artifact
	 * Whenever ~ or another artifact enters the battlefield, put a charge counter on ~.
	 * |3, |T, Remove a charge counter from ~: Draw a card. */

	if (comes_into_play(player, card, event)){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if (trigger_condition == TRIGGER_COMES_INTO_PLAY){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ARTIFACT, "");
		test.not_me = 1;

		if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test)){
			add_counter(player, card, COUNTER_CHARGE);
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		draw_a_card(player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), GVC_COUNTER(COUNTER_CHARGE), NULL, NULL);
}

int card_shrapnel_blast(int player, int card, event_t event){
	/*
	  Shrapnel Blast |1|R
	  Instant
	  As an additional cost to cast Shrapnel Blast, sacrifice an artifact.
	  Shrapnel Blast deals 5 damage to target creature or player.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			if( ! sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
				return 0;
			}
		}
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_skeleton_shard(int player, int card, event_t event){

	char msg[100] = "Select an Artifact Creature.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.special_selection_function = &is_artifact_creature_by_internal_id;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && new_special_count_grave(player, &this_test) > 0 ){
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
				return 1;
			}
			if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
				return 1;
			}
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Pay B\n Pay 3\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 3*choice, 1-choice, 0, 0, 0, 0) ){
					if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
						tap_card(player, card);
					}
					else{
						spell_fizzled = 1;
					}
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

int card_slith_bloodletter(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		add_1_1_counter(player, card);
	}

	return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_slith_firewalker(int player, int card, event_t event){

	haste(player, card, event);

	return card_slith_predator(player, card, event);
}

int card_slith_predator(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_slith_strider(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		add_1_1_counter(player, card);
	}

	if( current_turn == player && current_phase == PHASE_DECLARE_BLOCKERS && event == EVENT_DECLARE_BLOCKERS &&
		is_attacking(player, card) && ! is_unblocked(player, card)
	  ){
		draw_cards(player, 1);
	}

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 24;
	}

	return 0;
}

int card_solar_tide(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 0;
		if( get_average_power(1-player) > 2 && get_average_power(player) < 3 ){
			ai_choice = 1;
		}
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Wipe power <= 2\n Wipe power >= 3\n");
				if( can_sacrifice_as_cost(player, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine && count_permanents_by_type(player, TYPE_LAND) > 5 ){
					if( count_permanents_by_type(player, TYPE_CREATURE) <
						(count_permanents_by_type(1-player, TYPE_CREATURE)-2)
					  ){
						ai_choice = 2;
					}
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			int i;
			for(i=0; i<2; i++){
				sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}

		instance->targets[0].player = 1+choice;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++ ){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						int his_pow = get_power(i, count);
						int to_kill = 0;
						if( (instance->targets[0].player & 1) && his_pow < 3 ){
							to_kill = 1;
						}
						if( (instance->targets[0].player & 2) && his_pow > 2 ){
							to_kill = 1;
						}
						if( to_kill == 1 ){
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

int card_soldier_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_IN_COMBAT;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_XW(1,1), 0, &td, "TARGET_ATTACKING_BLOCKING_CREATURE");
}

int card_solemn_simulacrum(int player, int card, event_t event)
{
  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_DUH))
	draw_cards(player, 1);

  return 0;
}

int card_soul_foundry(int player, int card, event_t event){
	/* Soul Foundry	|4
	 * Artifact
	 * Imprint - When ~ enters the battlefield, you may exile a creature card from your hand.
	 * |X, |T: Put a token that's a copy of the exiled card onto the battlefield. X is the converted mana cost of that card. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier-=1000;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			if( selected != -1 ){
				instance->targets[1].player = get_cmc(player, selected);
				instance->targets[1].card = get_id(player, selected);
				rfg_card_in_hand(player, selected);
				create_card_name_legacy(player, card, instance->targets[1].card);
			}
	}

	else if( event == EVENT_CAN_ACTIVATE && instance->targets[1].player > -1 ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, instance->targets[1].player, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( check_rfg(player, instance->targets[1].card) ){
				token_generation_t token;
				default_token_definition(player, card, instance->targets[1].card, &token);
				token.no_sleight = 1;
				generate_token(&token);
			}
	}
	else{
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, instance->targets[1].player, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	return 0;
}

int card_spellweaver_helix(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI  ){
			int my_g = count_graveyard_by_type(player, TYPE_SORCERY);
			int his_g = count_graveyard_by_type(1-player, TYPE_SORCERY);
			if( my_g < 2 && his_g < 2 ){
				ai_modifier-=1000;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int my_g = count_graveyard_by_type(player, TYPE_SORCERY);
		int his_g = count_graveyard_by_type(1-player, TYPE_SORCERY);
		if( my_g > 1 || his_g > 1 ){
			int trg = 1-player;
			if( his_g > 1 ){
				if( my_g > 1 ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						trg = instance->targets[0].player;
					}
					else{
						trg = -1;
					}
				}
			}
			else{
				trg = player;
			}
			int count = 0;
			const int *grave = get_grave(trg);
			while( count < 2 ){
					int selected = select_a_card(player, trg, 2, 0, 1, -1, TYPE_SORCERY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
					if( selected == -1 ){
						break;
					}
					else{
						instance->targets[count+1].card = cards_data[grave[selected]].id;
						rfg_card_from_grave(trg, selected);
						count++;
					}
			}
			if( count < 2 ){
				instance->targets[0].card = -1;
				instance->number_of_targets = 0;
			}
			else{
				instance->targets[0].card = 66;
				instance->number_of_targets = 0;
			}
		}
	}
		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
			player == reason_for_trigger_controller && instance->targets[0].card == 66
		  ){
			int id = get_id(trigger_cause_controller, trigger_cause);
			int to_play = -1;
			if( instance->targets[1].card == id ){
				to_play = instance->targets[2].card;
			}
			else if( instance->targets[2].card == id ){
				to_play = instance->targets[1].card;
			}
			if( to_play > 1 ){
				if(event == EVENT_TRIGGER){
					event_result |= 1+player;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						copy_spell(player, to_play);
				}
			}
		}

	return 0;
}

int card_spikeshot_goblin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_ACTIVATE){
		instance->info_slot = get_power(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			int amount = instance->info_slot;
			if( in_play(instance->parent_controller, instance->parent_card) ){
				amount = get_power(instance->parent_controller, instance->parent_card);
			}
			damage_target0(player, card, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_spoils_of_the_vault(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int id;
		int *deck = deck_ptr[player];
		int cd = count_deck(player);
		if( player == AI ){
			int rnd = internal_rand(cd);
			id = cards_data[deck[rnd]].id;
			card_ptr_t* c = cards_ptr[ id ];
			char buffer[100];
			scnprintf(buffer, 100, "Opponent named %s", c->name );
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
		}
		else{
			id = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		int count = 0;
		while( deck[count] != -1 ){
				if( cards_data[deck[count]].id == id ){
					break;
				}
				count++;
		}
		show_deck( HUMAN, deck, count+1, "These cards were revealed", 0, 0x7375B0 );
		add_card_to_hand(player, deck[count]);
		remove_card_from_deck(player, count);
		lose_life(player, count);
		int i;
		for(i=0; i<count; i++){
			rfg_top_card_of_deck(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_stalking_stones(int player, int card, event_t event){
	return manland_normal(player, card, event, 6, 0, 0, 0, 0, 0);
}

int card_stalking_stones_animated(int player, int card, event_t event){
	return manland_animated(player, card, event, 6, 0, 0, 0, 0, 0);
}

int card_sun_droplet(int player, int card, event_t event)
{
  /* Sun Droplet	|2
   * Artifact
   * Whenever you're dealt damage, put that many charge counters on ~.
   * At the beginning of each upkeep, you may remove a charge counter from ~. If you do, you gain 1 life. */

	card_instance_t* damage = damage_being_dealt(event);
	if (damage && damage->damage_target_card == -1 && damage->damage_target_player == player && !damage_is_to_planeswalker(damage)){
		int good = 0;
		if( damage->info_slot > 0 ){
			good = damage->info_slot;
		}
		else{
			card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
			if( trg->targets[16].player > 0 ){
				good = trg->targets[16].player;
			}
		}
		get_card_instance(player, card)->info_slot += good;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (event == EVENT_TRIGGER && instance->info_slot > 0)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  add_counters(player, card, COUNTER_CHARGE, instance->info_slot);
		  instance->info_slot = 0;
		}
	}

	if( count_counters(player, card, COUNTER_CHARGE) ){
		upkeep_trigger_ability(player, card, event, ANYBODY);	// optional triggers only work during your own upkeep
		if (event == EVENT_UPKEEP_TRIGGER_ABILITY && do_dialog(player, player, card, -1, -1, " Remove a charge counter\n Pass", 0) == 0){
			remove_counter(player, card, COUNTER_CHARGE);
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_sunbeam_spellbomb(int player, int card, event_t event){
	/* Sunbeam Spellbomb	|1
	 * Artifact
	 * |W, Sacrifice ~: You gain 5 life.
	 * |1, Sacrifice ~: Draw a card. */

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_ACTIVATE ){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
				choice = do_dialog(player, player, card, -1, -1, " Draw a card\n Gain 5 life\n Do nothing", 1);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 1-choice, 0, 0, 0, 0, choice) ){
					instance->targets[1].player = 66 + choice;
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 ){
				draw_cards(player, 1);
			}
			else if( instance->targets[1].player == 67 ){
					gain_life(player, 5);
			}
	}

	return spellbomb(player, card, event);
}

int card_sword_of_kaldra(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( is_equipping(player, card) ){
		rfg_when_damage(instance->targets[8].player, instance->targets[8].card, event);

		if (event == EVENT_CHANGE_TYPE && affect_me(instance->targets[8].player, instance->targets[8].card) && !is_humiliated(player, card)){
			get_card_instance(instance->targets[8].player, instance->targets[8].card)->destroys_if_blocked |= DIFB_DESTROYS_ALL;
		}
	}

	return vanilla_equipment(player, card, event, 4, 5, 5, 0, 0);
}

int card_sylvan_scrying(int player, int card, event_t event){
	/*
	  Sylvan Scrying |1|G
	  Sorcery
	  Search your library for a land card, reveal it, and put it into your hand. Then shuffle your library.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		tutor_lands(player, TUTOR_HAND, 1);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


int card_taj_nar_swordsmith(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 1, TYPE_ARTIFACT, 0, SUBTYPE_EQUIPMENT, 0, 0, 0, 0, 0, x_value+1, 3);
		}
	}

	return 0;
}

int card_tangleroot(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		produce_mana(instance->targets[1].player, COLOR_GREEN, 1);
	}

	return 0;
}

// Tel-Jilad Chosen -> vanilla

// Tempest of Ligh -> Tranquillity

int card_temporal_cascade(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 0;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Timetwister without drawing\n Both draws 7 cards\n");
				if( has_mana(player, COLOR_COLORLESS, 2) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				if( entwine ){
					ai_choice = 2;
				}
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 2 ){
			charge_mana(player, COLOR_COLORLESS, 2);
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		instance->info_slot = 1+choice;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			int i;
			for(i=0; i<2; i++){
				int trg = player;
				if( i == 1 ){
					trg = 1-player;
				}
				reshuffle_hand_into_deck(trg, 1);
				reshuffle_grave_into_deck(trg, 0);
			}
		}
		if( instance->info_slot & 2 ){
			int i;
			for(i=0; i<2; i++){
				int trg = player;
				if( i == 1 ){
					trg = 1-player;
				}
				draw_cards(trg, 7);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_thirst_for_knowledge(int player, int card, event_t event){
	/* Original Code: 004E76FB */

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);

		test_definition_t test;
		new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card to discard");
		test.zone = TARGET_ZONE_HAND;
		int selected = -1;
		if( check_battlefield_for_special_card(player, card, player, 0, &test) ){
			selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
		}
		if( selected != -1 ){
			discard_card(player, selected);
		}
		else{
			multidiscard(player, 2, 0);
		}

		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_thought_prison(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI  ){
			if( hand_count[1-player] < 1 ){
				ai_modifier-=1000;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_RFG;
			int id = new_effect_coercion(&this_definition, &this_test);
			instance->targets[1].player = id;
			if( id != -1 ){
				instance->targets[1].card = get_internal_card_id_from_csv_id(id);
				create_card_name_legacy(player, card, id);
			}
		}
	}
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
			player == reason_for_trigger_controller && instance->targets[1].player != -1
		  ){
			int dmg = 0;
			if( has_my_colors(trigger_cause_controller, trigger_cause, -1, instance->targets[1].card) > 0 ){
				dmg = 1;
			}
			if( get_cmc(trigger_cause_controller, trigger_cause) == get_cmc_by_id(instance->targets[1].player) ){
				dmg = 1;
			}
			if( dmg > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						damage_player(trigger_cause_controller, 2, player, card);
				}
			}
		}

	return 0;
}

int card_thoughtcast(int player, int card, event_t event)
{
  // 0x12050f2

  /* Thoughtcast	|4|U
   * Sorcery
   * Affinity for artifacts
   * Draw two cards. */

  affinity(player, card, event, TYPE_ARTIFACT, -1);

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 2);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_tooth_and_nail(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.cmc = 5;
			this_test.cmc_flag = 2;
			this_test.zone = TARGET_ZONE_HAND;

			int choice = 0;
			int ai_choice = 0;
			if( has_mana(player, COLOR_COLORLESS, 2) ){
				choice = do_dialog(player, player, card, -1, -1, " Search for 2 creatures.\n Put 2 Creatures into play.\n Do both.\n Cancel", 2);
			}
			else{
				if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) > 1 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Search for 2 creatures.\n Put 2 Creatures into play.\n Cancel", ai_choice);
				if( choice > 1 ){
					choice++;
				}
			}
			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else{
				if( choice == 2 ){
					charge_mana_multi( player, 2, 0, 0, 0, 0, 0 );
				}
				if( spell_fizzled != 1 ){
					instance->info_slot = choice+1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			char buffer[100];
			scnprintf(buffer, 100, "Select a creature card");
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			this_test.qty = 2;
			int i;
			for(i=0; i<2; i++){
				if( instance->info_slot & (1<<i) ){
					int from = TUTOR_FROM_DECK;
					int dest = TUTOR_HAND;
					int mode = AI_MAX_VALUE;
					if( instance->info_slot & (1<<1) ){
						mode = AI_MAX_CMC;
					}
					if( i == 1 ){
						from = TUTOR_FROM_HAND;
						dest = TUTOR_PLAY;
						mode = AI_MAX_CMC;
					}
					new_global_tutor(player, player, from, dest, 0, mode, &this_test);
					if( i == 0 ){
						shuffle(player);
					}
				}
			}
			kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_tower_of_fortunes(int player, int card, event_t event){

	/* Tower of Fortunes	|4
	 * Artifact
	 * |8, |T: Draw four cards. */

	if(event == EVENT_RESOLVE_ACTIVATION){
		draw_cards(player, 4);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(8), 0, NULL, NULL);
}

int card_troll_ascetic(int player, int card, event_t event){

	hexproof(player, card, event);

	return regeneration(player, card, event, MANACOST_XG(1, 1));
}

int card_scale_of_chiss_goria(int player, int card, event_t event){
	// tooth of chiss-goria
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	affinity(player, card, event, TYPE_ARTIFACT, -1);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0);
		}
	}
	return flash(player, card, event);
}

int card_tower_of_champions(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_creature_pumper(player, card, event, 8, 0, 0, 0, 0, 0, GAA_UNTAPPED+GAA_NONSICK, 6, 6, 0, 0, &td);
}

int card_tower_of_eons(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 10);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 8, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_tower_of_murmurs(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 8);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 8, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_trash_for_treasures(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int selected = select_a_card(player, player, 2, 0, 2, -1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( selected != -1 ){
			if( controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
				const int *grave = get_grave(player);
				instance->targets[1].player = selected;
				instance->targets[1].card = grave[selected];
			}
			else{
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(player);
		int selected = instance->targets[1].player;
		if( grave[selected] == instance->targets[1].card ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_turn_to_dust(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.required_subtype = SUBTYPE_EQUIPMENT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_EQUIPMENT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			produce_mana(player, COLOR_GREEN, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vedalken_archmage(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_vermiculos(int player, int card, event_t event){

	if( specific_cip(player, card, event, 2, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 4, 4);
	}

	return 0;
}

int card_viridian_joiner(int player, int card, event_t event){
	return mana_producing_creature(player, card, event, 12, COLOR_GREEN, get_power(player, card));
}

int card_vorrac_battlehorns(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( is_equipping(player, card) ){
		if( event == EVENT_DECLARE_BLOCKERS && current_turn == player){
			int block_count = 0;
			int count = 0;
			while(count < active_cards_count[1-player]){
					if(in_play(1-player, count) ){
						if( get_card_instance(1-player, count)->blocking == instance->targets[8].card ){
							block_count++;
							if( block_count > 1 ){
								get_card_instance(1-player, count)->blocking = 255;
								get_card_instance(1-player, count)->state &= ~(STATE_BLOCKED|STATE_BLOCKING);
							}
						}
					}
					count++;
			}
		}
	}

	return vanilla_equipment(player, card, event, 1, 0, 0, KEYWORD_TRAMPLE, 0);
}

int card_vulshok_battlegear(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 3, 3, 3, 0, 0);
}

int card_vulshok_battlemaster(int player, int card, event_t event){

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) &&
						has_subtype(i, count, SUBTYPE_EQUIPMENT)
					  ){
						equip_target_creature(i, count, player, card);
					}
					count--;
			}
		}
	}
	return 0;
}

int card_vulshock_gauntlets(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( is_equipping(player, card) ){
		does_not_untap(instance->targets[8].player, instance->targets[8].card, event);
	}

	return vanilla_equipment(player, card, event, 3, 4, 2, 0, 0);
}

int card_wail_of_the_nim(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		int result = card_death_ward(player, card, event);
		if( result > 0 ){
			instance->info_slot = 1;
			return result;
		}
		else{
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 1;
		if( instance->info_slot == 1 ){
			ai_choice = 0;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Regenerate your guys\n Damage all\n");
				if( has_mana(player, COLOR_BLACK, 1) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		}
		else{
			choice = 1;
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			charge_mana(player, COLOR_BLACK, 1);
		}
		instance->info_slot = 1+choice;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						card_instance_t *this = get_card_instance(player, count);
						if( this->kill_code == KILL_DESTROY ){
							regenerate_target(player, count);
						}
					}
					count--;
			}
		}
		if( instance->info_slot & 2 ){
			damage_all(player, card, player, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			damage_all(player, card, 1-player, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_war_elemental(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) < 1 ){
			ai_modifier-=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		if( get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) < 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	damage_effects(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT + DDBM_REPORT_DAMAGE_DEALT) ){
		add_1_1_counters(player, card, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	return 0;
}

int card_welding_jar(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_type = TYPE_ARTIFACT;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target artifact to regenerate.");
}

int card_wizard_replica(int player, int card, event_t event){
	/* Wizard Replica	|3
	 * Artifact Creature - Wizard 1/3
	 * Flying
	 * |U, Sacrifice ~: Counter target spell unless its controller pays |2. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_SPELL_ON_STACK, 0, 0, 1, 0, 0, 0, 0, &td, NULL);
}

int card_wrench_mind(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			char msg[100] = "Select a card to discard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);

			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_ARTIFACT);

			int count = 0;
			while( count < 2 && hand_count[instance->targets[0].player] > 0 ){
					int selected = -1;
					if( instance->targets[0].player == AI ){
						selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test2);
					}
					if( instance->targets[0].player == HUMAN || selected == -1 ){
						selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
					}
					if( is_what(instance->targets[0].player, selected, TYPE_ARTIFACT) ){
						new_discard_card(instance->targets[0].player, selected, player, 0);
						break;
					}
					else{
						new_discard_card(instance->targets[0].player, selected, player, 0);
						count++;
					}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}
