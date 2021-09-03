#include "manalink.h"

// Functions
void extort(int player, int card, event_t event){

	if( in_play(player, card) && has_mana_hybrid(player, COLOR_BLACK, COLOR_WHITE, 1, 0) && trigger_condition == TRIGGER_SPELL_CAST &&
		affect_me(player, card) && player == reason_for_trigger_controller && !is_humiliated(player, card) &&
		trigger_cause_controller == player && trigger_cause != card
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					charge_mana_hybrid(player, card, COLOR_BLACK, COLOR_WHITE, 1, 0);
					if( spell_fizzled != 1 ){
						int result = lose_life(1-player, 1);
						gain_life(player, result);
					}
			}
		}
	}

}

int battalion(int player, int card, event_t event){
	// Battalion - Whenever ~ and at least two other creatures attack, [...]
	return declare_attackers_trigger(player, card, event, 0, player, card) && count_attackers(player) >= 3 ? 1 : 0;
}

static void evolve(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && in_play(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			if( current_phase < PHASE_DECLARE_ATTACKERS ){
				ai_modifier+=20;
			}
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && trigger_cause_controller == player &&
		reason_for_trigger_controller == affected_card_controller && !is_humiliated(player, card)
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			get_card_instance(trigger_cause_controller, trigger_cause)->regen_status |= KEYWORD_RECALC_POWER | KEYWORD_RECALC_TOUGHNESS;
			if( get_power(trigger_cause_controller, trigger_cause) > get_power(player, card) ||
				get_toughness(trigger_cause_controller, trigger_cause) > get_toughness(player, card)
			  ){
				trig = 1;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(player, card);
			}
		}
	}
}

static int cipher_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me_arbitrary(instance->targets[0].player, instance->targets[0].card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_TRIGGER_OPTIONAL, player, card) ){
		copy_spell(player, instance->targets[1].card );
	}

	return 0;
}

void cipher(int player, int card){
	/*
	  [after resolution] You may exile this spell card encoded on a creature you control.
	  Whenever that creature deals combat damage to a player, its controller may cast a copy of the encoded card without paying its mana cost.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_token(player, card) && can_target(&td) ){
		int choice = do_dialog(player, player, card, -1, -1, " Cipher this spell\n Pass", 0);
		if( choice == 0 && pick_target(&td, "TARGET_CREATURE") ){
			int legacy = create_targetted_legacy_effect(player, card, &cipher_legacy, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].card = get_id(player, card);
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	else{
		kill_card(player, card, KILL_DESTROY);
	}
}

int bloodrush(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int pow, int tou, int key, int s_key){

	if (!IS_ACTIVATING_FROM_HAND(event)){
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND){
		/* Make validation and legacy creation go by the original card, not the rules engine.
		 * Ideally, the Rules Engine should do this, probably by interfacing directly with activate() so as to memcpy directly from the temporary card created
		 * to activate with rather than the Rules Engine. */
		int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(instance->targets[2].card));
		card_instance_t* added = get_card_instance(player, card_added);
		added->targets[0] = instance->targets[0];	// struct copy
		added->number_of_targets = 1;
		card = card_added;
		instance = added;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_ATTACKING;

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
			return can_target(&td);
		}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			if (charge_mana_multi(player, colorless, black, blue, green, red, white) && pick_target(&td, "TARGET_CREATURE")){
				discard_card(player, card);
				return 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, pow, tou, key, s_key);
			}
			obliterate_card(player, card);	// This is the temporary card created above, not either the activation card from the stack nor the card originally discarded
	}
	return 0;
}

// Cards
// White
int card_aerial_maneuver(int player, int card, event_t event){
	/* Aerial Maneuver	|1|W
	 * Instant
	 * Target creature gets +1/+1 and gains flying and first strike until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 1, KEYWORD_FLYING|KEYWORD_FIRST_STRIKE, 0);
}

int card_angelic_edict(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_ENCHANTMENT", 1, NULL);
}

int card_angelic_skirmisher(int player, int card, event_t event){

	if( beginning_of_combat(player, card, event, ANYBODY, -1) ){
		int ai_choice = 0;
		if( life[player] < 10 ){
			ai_choice = 1;
		}
		if( life[player] < 6 ){
			ai_choice = 2;
		}
		int key = KEYWORD_FIRST_STRIKE;
		int s_key = 0;
		int choice = do_dialog(player, player, card, -1, -1, " Give First Strike\n Give Vigilance\n Give Lifelink", ai_choice);
		if( choice == 1 ){
			key = 0;
			s_key = SP_KEYWORD_VIGILANCE;
		}
		if( choice == 2 ){
			key = 0;
			s_key = SP_KEYWORD_LIFELINK;
		}
		pump_subtype_until_eot(player, card, player, -1, 0, 0, key, s_key);
	}

	return 0;
}

int card_blind_obedience(int player, int card, event_t event){
	/* Blind Obedience	|1|W
	 * Enchantment
	 * Extort
	 * Artifacts and creatures your opponents control enter the battlefield tapped. */

	extort(player, card, event);

	permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_ARTIFACT | TYPE_CREATURE, NULL);

	return global_enchantment(player, card, event);
}

int card_boros_elite(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}
	return 0;
}

int card_court_street_denizen(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_WHITE;
		this_test.not_me = 1;
		if( can_target(&td) && new_specific_cip(player, card, event, player, 1+player, &this_test) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_daring_skyjak(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FLYING, 0);
	}
	return 0;
}

int card_debtors_pulpit(int player, int card, event_t event){
	/* Debtor's Pulpit	|4|W
	 * Enchantment - Aura
	 * Enchant land
	 * Enchanted land has "|T: Tap target creature." */

	card_instance_t* instance;
	if( IS_GAA_EVENT(event) && (instance = in_play(player, card)) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		target_definition_t td1;
		default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);

		card_instance_t *trg = get_card_instance(t_player, t_card);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(t_player, t_card, &td1, 0) ){
				tap_card(trg->targets[0].player, trg->targets[0].card);
			}
		}
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_dutiful_thrull(int player, int card, event_t event){
	return regeneration(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_frontline_medic(int player, int card, event_t event){

	/* Frontline Medic	|2|W
	 * Creature - Human Cleric 3/3
	 * Battalion - Whenever ~ and at least two other creatures attack, creatures you control gain indestructible until end of turn.
	 * Sacrifice ~: Counter target spell with |X in its mana cost unless its controller pays |3. */

	if( battalion(player, card, event) ){
		pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) &&
			is_x_spell(card_on_stack_controller, card_on_stack)
		  ){
			return 99;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			counterspell(player, card, event, NULL, 0);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3);
	}

	return 0;
}

int card_gideon_champion_of_justice(int player, int card, event_t event){

	/* Gideon, Champion of Justice	|2|W|W
	 * Planeswalker - Gideon (4)
	 * +1: Put a loyalty counter on ~ for each creature target opponent controls.
	 * 0: Until end of turn, ~ becomes an indestructible Human Soldier creature with power and toughness each equal to the number of loyalty counters on
	 * him. He's still a planeswalker. Prevent all damage that would be dealt to him this turn.
	 * -15: Exile all other permanents. */

	double_faced_card(player, card, event);

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_LOYALTY_FROM_CREATURE = 1,
			CHOICE_ANIMATE,
			CHOICE_EXILE_OTHERS
		};
		int priorities[3] = {	count_subtype(1-player, TYPE_CREATURE, -1)*2,
								current_turn < PHASE_DECLARE_BLOCKERS ? count_counters(player, card, COUNTER_LOYALTY)*3 : 0,
								((count_subtype(1-player, TYPE_PERMANENT, -1)-count_subtype(player, TYPE_PERMANENT, -1))*2)+
								((15-count_counters(player, card, COUNTER_LOYALTY))*3)
		};
		int choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Get Loyalty from opponent's creatures", would_validate_arbitrary_target(&td, 1-player, -1), priorities[0], 1,
						"Animate", 1, priorities[1], 0,
						"Exile all the other permanents", 1, priorities[2], -15);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			if( choice == CHOICE_LOYALTY_FROM_CREATURE ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_LOYALTY_FROM_CREATURE:
				if( valid_target(&td)){
					add_counters(instance->parent_controller, instance->parent_card, COUNTER_LOYALTY,
								count_subtype(instance->targets[0].player, TYPE_CREATURE, -1));
				}
				break;

			case CHOICE_ANIMATE:
				true_transform(instance->parent_controller, instance->parent_card);
				break;

			case CHOICE_EXILE_OTHERS:{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_PERMANENT);
				this_test.not_me = 1;
				APNAP(p, {new_manipulate_all(instance->parent_controller, instance->parent_card, p, &this_test, KILL_REMOVE);});
			}
			  break;
		  }
	}

	return planeswalker(player, card, event, 4);
}

int card_gideon_champion_animated(int player, int card, event_t event){

	if (card == -1){
		return 0;
	}

	indestructible(player, card, event);
	// prevent any damage that would be dealt to me
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 ){
					damage->info_slot = 0;
				}
			}
		}

		if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)){
			int counters = count_counters(player, card, COUNTER_LOYALTY);
			event_result += counters;
		}
	}

	if( eot_trigger(player, card, event ) ){
		true_transform(player, card);
	}

	return card_gideon_champion_of_justice(player, card, event);
}

// Guardian of the Gateless --> in multiblock.c

int card_guildscorn_ward(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		protection_from_multicolored(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_hold_the_gates(int player, int card, event_t event){
	if( ( event == EVENT_ABILITIES || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card) ){
		boost_subtype(player, card, event, -1, 0,count_subtype(player, TYPE_LAND, SUBTYPE_GATE), 0,SP_KEYWORD_VIGILANCE, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	}

	return global_enchantment(player, card, event);
}

int card_holy_mantle(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		protection_from_creatures(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_knight_of_obligation(int player, int card, event_t event){
	vigilance(player, card, event);
	extort(player, card, event);
	return 0;
}

int card_knight_watch(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KNIGHT, &token);
		token.qty = 2;
		token.s_key_plus = SP_KEYWORD_VIGILANCE;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_luminate_primordial(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	vigilance(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				gain_life(instance->targets[0].player, get_toughness(instance->targets[0].player, instance->targets[0].card));
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

int card_murder_investigation(int player, int card, event_t event){
	/* Murder Investigation	|1|W
	 * Enchantment - Aura
	 * Enchant creature you control
	 * When enchanted creature dies, put X 1/1 |Swhite Soldier creature tokens onto the battlefield, where X is its power. */

	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		if( graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event) ){
			generate_tokens_by_id(player, card, CARD_ID_SOLDIER, get_power(instance->damage_target_player, instance->damage_target_card));
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_nav_squad_commandos(int player, int card, event_t event)
{
  // Battalion - Whenever ~ and at least two other creatures attack, ~ gets +1/+1 until end of turn. Untap it.
  if (battalion(player, card, event))
	{
	  pump_until_eot(player, card, player, card, 1, 1);
	  untap_card(player, card);
	}

  return 0;
}

// righteous charge --> Portal

int card_shielded_passage(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			prevent_the_next_n_damage(player, card, instance->targets[0].player, instance->targets[0].card, 0, PREVENT_INFINITE, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
}

int card_syndic_of_tithes(int player, int card, event_t event){
	extort(player, card, event);
	return 0;
}

int card_urbis_protector(int player, int card, event_t event){
	/* Urbis Protector	|4|W|W
	 * Creature - Human Cleric 1/1
	 * When ~ enters the battlefield, put a 4/4 |Swhite Angel creature token with flying onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ANGEL);
	}
	return 0;
}

int card_zarichi_tiger(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 2);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XW(1, 1), 0, NULL, NULL);
}

// blue
int card_aetherize(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		new_manipulate_all(player, card, current_turn, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_agoraphobia(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 1), 0, NULL, NULL);
		}
		if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_aura(player, card, event, 1-player, -5, 0, 0, 0, 0, 0, 0);
}

int card_cloudfin_raptor(int player, int card, event_t event){
	evolve(player, card, event);
	return 0;
}

int card_diluvian_primordial(int player, int card, event_t event){

	/* Diluvian Primordial	|5|U|U
	 * Creature - Avatar 5/5
	 * Flying
	 * When ~ enters the battlefield, for each opponent, you may cast up to one target instant or sorcery card from that player's graveyard without paying its
	 * mana cost. If a card cast this way would be put into a graveyard this turn, exile it instead. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		char msg[100] = "Select a spell to play for free.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, msg);
		this_test.type_flag = F1_NO_CREATURE;

		if( comes_into_play(player, card, event) && new_special_count_grave(1-player, &this_test) > 0 && ! graveyard_has_shroud(2) ){
			int selected = new_select_a_card(player, 1-player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				play_card_in_grave_for_free_and_exile_it(player, 1-player, selected);
			}
		}
	}
	return 0;
}

static int enter_the_infinite_legacy(int player, int card, event_t event){

	if( current_turn != player && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return card_spellbook(player, card, event);
}

int card_enter_the_infinite(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, count_deck(player));
		if( hand_count[player] > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_DECK, 1, AI_MIN_VALUE, &this_test);
		}
		create_legacy_effect(player, card, &enter_the_infinite_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// frilled oculus --> basking rootwalla

int card_gridlock(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs) ){
				if( new_pick_target(&td, "TARGET_PERMANENT", trgs, 0) ){
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
		}
		if( trgs < 1 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana(player, COLOR_COLORLESS, trgs);
			if( spell_fizzled != 1 ){
				instance->info_slot = trgs;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hands_of_binding(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature your opponent controls.", 1, NULL);
}

int card_incursion_specialist(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card)
	  ){
		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			if( trigger_cause_controller == player && get_specific_storm_count(player) == 1 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						pump_ability_until_eot(player, card, player, card, 2, 0, 0, SP_KEYWORD_UNBLOCKABLE);
				}
			}
		}
	}

	return 0;
}

int card_keymaster_rogue(int player, int card, event_t event){

	unblockable(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( comes_into_play(player, card, event) && can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_last_thoughts(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		cipher(player, card);
	}

	return basic_spell(player, card, event);;
}

int card_leyline_phantom(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int bounce = 1;
		// Won't return in hand if:

		//Case 1: has lethal damage on it and it hasn't Indestructible
		if( get_card_instance(player, card)->damage_on_card > get_toughness(player, card) &&
			! check_for_special_ability(player, card, SP_KEYWORD_INDESTRUCTIBLE)
		  ){
			bounce = 0;
		}
		// Case 2: has some special kind of damage stamped on which will kill it even if it's not lethal (Deathtouch, for example)
		if( get_card_instance(player, card)->damage_on_card > 0 &&
			check_special_flags(player, card, SF_LETHAL_DAMAGE_DESTROY | SF_LETHAL_DAMAGE_BURY | SF_LETHAL_DAMAGE_EXILE | SF_EXILE_IF_DAMAGED)
		  ){
			bounce = 0;
		}
		if( bounce ){
			bounce_permanent(player, card);
		}
	}
	return 0;
}

int card_metropolis_sprite(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, -1);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
}

int card_mindeye_drake(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			mill(instance->targets[0].player, 5);
		}
	}

	return 0;
}

int card_rapid_hybridization(int player, int card, event_t event){
	/* Rapid Hybridization	|U
	 * Instant
	 * Destroy target creature. It can't be regenerated. That creature's controller puts a 3/3 |Sgreen Frog Lizard creature token onto the battlefield. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_FROG_LIZARD, &token);
			token.t_player = instance->targets[0].player;
			token.pow = 3;
			token.tou = 3;
			token.color_forced = COLOR_TEST_GREEN;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_realmwright(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (comes_into_play(player, card, event))
	{
	  load_text(0, "PHANTASMAL_TERRAIN");
	  instance->targets[4].player = choose_a_color_exe(player, text_lines[1], 0, internal_rand(5) + 1, COLOR_TEST_ANY_COLORED);
	  switch (instance->targets[4].player){
		case COLOR_BLACK:	instance->targets[4].card = SUBTYPE_SWAMP;	break;
		case COLOR_BLUE:	instance->targets[4].card = SUBTYPE_ISLAND;	break;
		case COLOR_GREEN:	instance->targets[4].card = SUBTYPE_FOREST;	break;
		case COLOR_RED:		instance->targets[4].card = SUBTYPE_MOUNTAIN;	break;
		case COLOR_WHITE:	instance->targets[4].card = SUBTYPE_PLAINS;	break;
		default:			instance->targets[4].card = instance->targets[4].player = -1;	break;
	  }
	}

  if (instance->targets[4].player >= 0)
	return all_lands_are_basiclandtype(player, card, event, player, instance->targets[4].player, instance->targets[4].card);

  return 0;
}

int card_sages_row_denizen(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLUE;
		this_test.not_me = 1;
		if( can_target(&td) && new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				mill(instance->targets[0].player, 2);
			}
		}
	}
	return 0;
}

int card_simic_fluxmage(int player, int card, event_t event){

	evolve(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_1_1_COUNTER, MANACOST_XU(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_simic_manipulator(int player, int card, event_t event){

	evolve(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.power_requirement = instance->info_slot | TARGET_PT_LESSER_OR_EQUAL;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			instance->info_slot = count_1_1_counters(player, card);
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = count_1_1_counters(player, card);
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			char buffer[500];
			scnprintf(buffer, 500, "Select target creature with power %d or less.", count_1_1_counters(player, card));
			if( new_pick_target(&td, buffer, 0, 1 | GS_LITERAL_PROMPT) ){
				int amount = get_power(instance->targets[0].player, instance->targets[0].card);
				remove_1_1_counters(player, card, amount);
				instance->info_slot = amount;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_skygames(int player, int card, event_t event){

	if( ! IS_AURA_EVENT(player, card, event) || ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		target_definition_t td1;
		default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;

		card_instance_t *trg = get_card_instance(t_player, t_card);

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE");
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(t_player, t_card, &td1, 0) ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, trg->targets[0].player, trg->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_spell_rupture(int player, int card, event_t event){
	/* Spell Rupture	|1|U
	 * Instant
	 * Counter target spell unless its controller pays |X, where X is the greatest power among creatures you control. */

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, get_highest_power(player));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_stolen_identity(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			copy_token(player, card, instance->targets[0].player, instance->targets[0].card);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target artifact or creature.",1, NULL);
}

int card_totally_lost(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.",1, NULL);
}

int card_voidwalk(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "TARGET_CREATURE", 1, NULL);
}


int card_way_of_the_thief(int player, int card, event_t event)
{
  card_instance_t* instance;
  if (event == EVENT_ABILITIES
	  && (instance = in_play(player, card)) && affect_me(instance->damage_target_player, instance->damage_target_card)
	  && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_GATE))
	unblockable(instance->damage_target_player, instance->damage_target_card, event);

  return generic_aura(player, card, event, player, 2,2, 0,0, 0, 0, 0);
}

// Black

// basilica screecher --> syndic_of_tithes

int card_balustrade_spy(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);


		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				int *deck = deck_ptr[instance->targets[0].player];
				if( deck[0] != -1 ){
					int count = 0;
					while( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
							count++;
					}
					count++;
					show_deck( player, deck, count, "Balustrade Spy revealed...", 0, 0x7375B0 );
					mill(instance->targets[0].player, count);
				}
			}
		}
	}
	return 0;
}

int card_contaminated_ground(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->damage_target_player, instance->damage_target_card) && ! is_humiliated(player, card) ){
			lose_life(instance->damage_target_player, 2);
		}
	}

	return card_evil_presence(player, card, event);
}

int card_corpse_blockade(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, 0, SP_KEYWORD_DEATHTOUCH);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST0, 0, NULL, NULL);
}

int card_crypt_ghast(int player, int card, event_t event){

	produce_mana_when_subtype_is_tapped(player, event, TYPE_PERMANENT, SUBTYPE_SWAMP, COLOR_BLACK, 1);

	extort(player, card, event);

	return 0;
}

int card_deaths_approach(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result-=count_graveyard_by_type(instance->damage_target_player, TYPE_CREATURE);
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_devour_flesh(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	if( life[player] < 6 ){
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( instance->targets[2].card > 0 ){
				gain_life(instance->targets[0].player, instance->targets[2].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_dying_wish(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 && ! is_humiliated(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				lose_life(instance->targets[0].player, get_power(instance->damage_target_player, instance->damage_target_card));
				gain_life(player, get_power(instance->damage_target_player, instance->damage_target_card));
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_gateway_shade(int player, int card, event_t event){
	/* Gateway Shade	|2|B
	 * Creature - Shade 1/1
	 * |B: ~ gets +1/+1 until end of turn.
	 * Tap an untapped Gate you control: ~ gets +2/+2 until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_GATE;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_B(1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Pay B\n Tap a Gate\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
				instance->info_slot = 1;
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					if( tapsubtype_ability(player, card, 1, &td) ){
						instance->number_of_targets = 1;
						instance->info_slot = 2;
					}
					else{
						spell_fizzled = 1;
					}
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
						instance->info_slot, instance->info_slot);
	}

	return 0;
}

int card_grisly_spectacle(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = get_power(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			mill(instance->targets[0].player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE",1, NULL);
}

// gutter skulk --> vanilla

int card_horror_of_the_dim(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_U(1), 0, 0, 0, SP_KEYWORD_HEXPROOF);
}

int card_illness_in_the_ranks(int player, int card, event_t event)
{
  /* Illness in the Ranks	|B
   * Enchantment
   * Creature tokens get -1/-1. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && is_token(affected_card_controller, affected_card)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card))
	event_result -= 1;

  return global_enchantment(player, card, event);
}

static const char* has_mana_to_target_this(int who_chooses, int player, int card)
{
  if (has_mana_multi(who_chooses, MANACOST_XB(get_cmc(player, card), 1)))
	return NULL;
  else
	return "cannot target this";
}

int card_killing_glare(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		td.extra = (int32_t)has_mana_to_target_this;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL,1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		td.extra = -1;
		td.special = 0;
		td.power_requirement = x_value | TARGET_PT_LESSER_OR_EQUAL;
		char buffer[100];
		scnprintf(buffer, 100, "Select target creature with power %d or less.", x_value);
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, buffer, 1, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		td.extra = -1;
		td.special = 0;
		td.power_requirement = instance->info_slot | TARGET_PT_LESSER_OR_EQUAL;
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lord_of_the_void(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int amount = MIN(7, count_deck(1-player));
		if( amount > 0 ){
			int *deck = deck_ptr[1-player];
			show_deck( player, deck, amount, "Lord of the Void will exile...", 0, 0x7375B0 );
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			this_test.no_shuffle = 1;
			this_test.create_minideck = amount;
			if( new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test) > -1 ){
				amount--;
			}
			while(amount > 0 ){
					rfg_card_in_deck(1-player, amount-1);
					amount--;
			}
		}
	}

	return 0;
}

int card_mental_vapors(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_midnight_recovery(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return card_raise_dead(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return card_raise_dead(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_ogre_slumlord(int player, int card, event_t event){
	/* Ogre Slumlord	|3|B|B
	 * Creature - Ogre Rogue 3/3
	 * Whenever another nontoken creature dies, you may put a 1/1 |Sblack Rat creature token onto the battlefield.
	 * Rats you control have deathtouch. */

	if (is_humiliated(player, card)){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		generate_tokens_by_id(player, card, CARD_ID_RAT, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( event == EVENT_ABILITIES && affected_card_controller == player && has_subtype(affected_card_controller, affected_card, SUBTYPE_RAT) &&
		! is_humiliated(player, card)
	  ){
		deathtouch(affected_card_controller, affected_card, event);
	}

	return 0;
}

int card_sepulchral_primordial(int player, int card, event_t event){

	intimidate(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);

		if( comes_into_play(player, card, event) && new_special_count_grave(1-player, &this_test) > 0 && ! graveyard_has_shroud(2) ){
			new_global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_shadow_alley_denizen(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLACK;
		this_test.not_me = 1;
		if( can_target(&td) && new_specific_cip(player, card, event, player, 1+player, &this_test) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_INTIMIDATE);
			}
		}
	}
	return 0;
}

int card_shadow_slice(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

// slate street ruffian --> alley_grifters

int card_smog_elemental(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card) ){
		if( affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
		  ){
			event_result--;
		}
	}

	return 0;
}

// syndicate enforcer --> syndic_of_tithes

static const char* target_must_have_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return count_counters(player, card, -1) ? NULL : "must have counters";
}
int card_thrull_parasite(int player, int card, event_t event)
{
  /* Thrull Parasite	|B
   * Creature - Thrull 1/1
   * Extort
   * |T, Pay 2 life: Remove a counter from target nonland permanent. */

  extort(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.illegal_type = TYPE_LAND;
  td.preferred_controller = ANYBODY;

  if (IS_AI(player) && event != EVENT_RESOLVE_ACTIVATION)
	{
	  /* Prevents the AI from considering removal of counters from permanents without them (thus speculating more on which type of counter to remove from
	   * permanents that *do* have them).  However, it also prevents it from just using this ability to tap or make itself lose life; once in a great while,
	   * this is beneficial - perhaps it has a Death's Shadow or Quest for Renewal or Loxodon Peacekeeper, or the Thrull Parasite's been targeted by Asphyxiate,
	   * or it could target a Skulking Ghost.  I think this is an acceptable tradeoff. */
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_must_have_counters;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  counter_t typ = choose_existing_counter_type(player, player, card, instance->targets[0].player, instance->targets[0].card, CECT_REMOVE, -1, -1);
	  if (typ != COUNTER_invalid)
		remove_counter(instance->targets[0].player, instance->targets[0].card, typ);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 2, &td, "TARGET_PERMANENT");
}

int card_undercity_informer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			if( deck[0] != -1 ){
				int count = 0;
				while( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
						count++;
				}
				count++;
				show_deck( player, deck, count, "Undercity Informer revealed...", 0, 0x7375B0 );
				mill(instance->targets[0].player, count);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_X(1), 0, &td, "TARGET_PLAYER");
}

int card_undercity_plague(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
			discard(instance->targets[0].player, 0, player);
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER",1, NULL);
}

int card_wight_of_precint_six(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=count_graveyard_by_type(1-player, TYPE_CREATURE);
	}

	return 0;
}

// Red

int card_bomber_corps(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, 1);
			instance->number_of_targets = 0;
		}
	}
	return 0;
}

int card_crackling_perimeter(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.required_subtype = SUBTYPE_GATE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
			new_pick_target(&td, "Select an untapped Gate you control.", 0, 1 | GS_LITERAL_PROMPT)
		  ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_player(1-player, 1, instance->parent_controller, instance->parent_card);
	}

	return 0;
}

// ember beast --> mogg flunkies

int card_firefist_striker(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}
	return 0;
}

int card_five_alarm_fire(int player, int card, event_t event){

	/* Five-Alarm Fire	|1|R|R
	 * Enchantment
	 * Whenever a creature you control deals combat damage, put a blaze counter on ~.
	 * Remove five blaze counters from ~: ~ deals 5 damage to target creature or player. */

	if (subtype_deals_damage(player, card, event, player, -1, DDBM_MUST_BE_COMBAT_DAMAGE)){
		card_instance_t* instance = get_card_instance(player, card);
		add_counters(player, card, COUNTER_BLAZE, instance->targets[1].card);
		instance->targets[1].card = 0;
	}

	if (!IS_GAA_EVENT(event)){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		damage_target0(player, card, 5);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTERS(COUNTER_BLAZE, 5), &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_foundry_street_denizen(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_RED;
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			pump_until_eot(player, card, player, card, 1, 0);
		}
	}
	return 0;
}

int card_furious_resistance(int player, int card, event_t event){
	/* Furious Resistance	|R
	 * Instant
	 * Target blocking creature gets +3/+0 and gains first strike until end of turn. */
	if (!IS_CASTING(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;
	return vanilla_pump(player, card, event, &td, 3, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_hellkite_tyrant(int player, int card, event_t event){

	/* Hellkite Tyrant	|4|R|R
	 * Creature - Dragon 6/5
	 * Flying, trample
	 * Whenever ~ deals combat damage to a player, gain control of all artifacts that player controls.
	 * At the beginning of your upkeep, if you control twenty or more artifacts, you win the game. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY || event == EVENT_SHOULD_AI_PLAY ){
		if( count_subtype(player, TYPE_ARTIFACT, -1) > 19 ){
			lose_the_game(1-player);
		}
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int artifacts = count_subtype(player, TYPE_ARTIFACT, -1) - 10;
		if (artifacts > 10){
			if (player == AI){
				ai_modifier += artifacts * artifacts * artifacts;
			} else if (artifacts >= 5){
				ai_modifier -= artifacts * artifacts * artifacts;
			}
		}
	}


	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_GAIN_CONTROL);
	}

	return 0;
}

int card_hellraiser_goblin(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		! is_humiliated(player, card)
	  ){
		haste(affected_card_controller, affected_card, event);
	}

	all_must_attack_if_able(player, event, -1);

	return 0;
}

int card_homing_lightning(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_and_all_the_others_with_the_same_name(player, card, instance->targets[0].player, instance->targets[0].card, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int legion_loyalist_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 0, 0, KEYWORD_FIRST_STRIKE | KEYWORD_TRAMPLE);

		if( event == EVENT_BLOCK_LEGALITY ){
			if( instance->targets[0].player == attacking_card_controller && instance->targets[0].card == attacking_card ){
				if( is_token(affected_card_controller, affected_card) ){
					event_result = 1;
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_legion_loyalist(int player, int card, event_t event){
	haste(player, card, event);
	if( battalion(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		add_legacy_effect_to_all(player, card, &legion_loyalist_effect, player, &this_test);
	}
	return 0;
}

int card_madcap_skills(int player, int card, event_t event){
	/* Madcap Skills	|1|R
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +3/+0 and has menace. */

	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		menace(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, player, 3, 0, 0, 0, 0, 0, 0);
}

int card_mark_for_death(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_BLOCK);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			pump_subtype_until_eot(player, card, instance->targets[0].player, -1, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_massive_raid(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, count_subtype(player, TYPE_CREATURE, -1));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_molten_primordial(int player, int card, event_t event){

	haste(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return 0;
}

int card_mugging(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_ripscale_predators(int player, int card, event_t event){
	/* Ripscale Predator	|4|R|R
	 * Creature - Lizard 6/5
	 * Menace */

	menace(player, card, event);
	return 0;
}

int card_scorchwalker(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_XR(1, 2), 5, 1, 0, 0);
}

int card_skinbrand_goblin(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_R(1), 2, 1, 0, 0);
}

int card_skullcrack(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_cannot_be_prevented_until_eot(player, card);
			damage_player(instance->targets[0].player, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_structural_colapse(int player, int card, event_t event){
	// structural_collapse

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			damage_player(instance->targets[0].player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_tin_street_market(int player, int card, event_t event){

	if( ! IS_AURA_EVENT(player, card, event) && ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST0, 0, NULL, NULL);
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST0, 0, NULL, NULL);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_towering_thunderfist(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_W(1), 0, 0, 0, SP_KEYWORD_VIGILANCE);
}

int card_viashino_shanktail(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_XR(2, 1), 3, 1, KEYWORD_FIRST_STRIKE, 0);
}

int card_warmind_infantry(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_until_eot(player, card, player, card, 2, 0);
	}
	return 0;
}

int card_wrecking_ogre(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_XR(3, 2), 3, 3, KEYWORD_DOUBLE_STRIKE, 0);
}

// Green
// Adaptive Snapjaw --> cloudfin_raptor

int card_alpha_autority(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_DECLARE_BLOCKERS ){
			// If there are fewer than 3 creatures blocking me, then no one is
			// blocking me
			int block_count = 0;
			int count = 0;
			while(count < active_cards_count[1-instance->damage_target_player]){
				if(in_play(1-instance->damage_target_player, count) ){
					card_instance_t *instance2 = get_card_instance( 1-instance->damage_target_player, count);
					if( instance2->blocking == instance->damage_target_card ){
						block_count++;
					}
				}
				count++;
			}

			if( block_count > 1 ){
				count = 0;
				while(count < active_cards_count[1-instance->damage_target_player]){
					if(in_play(1-instance->damage_target_player, count) ){
						card_instance_t *instance2 = get_card_instance( 1-instance->damage_target_player, count);
						if( instance2->blocking == instance->damage_target_card ){
							instance2->blocking = 255;
						}
					}
					count++;
				}
			}
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_HEXPROOF, 0, 0, 0);
}

int card_burst_of_strength(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// crocanura --> cloudfin_raptor

int card_crowned_ceratok(int player, int card, event_t event){
	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card) ){
		if( count_1_1_counters(affected_card_controller, affected_card) > 0 ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}
	return 0;
}

int card_disciple_of_the_old_ways(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_R(1), 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_experiment_one(int player, int card, event_t event){

	evolve(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && count_1_1_counters(player, card) > 1 ){
		return regeneration(player, card, event, MANACOST0);
	}

	if( event == EVENT_ACTIVATE ){
		regeneration(player, card, event, MANACOST0);
		if( spell_fizzled != 1 ){
			remove_1_1_counters(player, card, 2);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return regeneration(player, card, event, MANACOST0);
	}

	return 0;
}

int card_forced_adaption(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){

		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			add_1_1_counter(instance->damage_target_player, instance->damage_target_card);
		}

	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_giant_adephage(int player, int card, event_t event){
	/* Giant Adephage	|5|G|G
	 * Creature - Insect 7/7
	 * Trample
	 * Whenever ~ deals combat damage to a player, put a token onto the battlefield that's a copy of ~. */

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		token_generation_t token;
		copy_token_definition(player, card, &token, player, card);
		generate_token(&token);
	}
	return 0;
}

int card_greenside_watcher(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_GATE;
	if( player == AI ){
		td.required_state = TARGET_STATE_TAPPED;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST0, 0, &td, "Select target Gate.");
}

int card_gyre_sage(int player, int card, event_t event){

	evolve(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) && (player != AI || count_1_1_counters(player, card) > 0)){
		return can_produce_mana(player, card);
	}

	if(event == EVENT_ACTIVATE ){
		produce_mana_tapped(player, card, COLOR_GREEN, count_1_1_counters(player, card));
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) && count_1_1_counters(player, card) > 0 ){
		declare_mana_available(player, COLOR_GREEN, count_1_1_counters(player, card));
	}

	return 0;
}

static int hindervines_legacy(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && (damage->targets[3].player & TYPE_CREATURE)	// probably redundant to status check
	  && count_1_1_counters(damage->damage_source_player, damage->damage_source_card) <= 0)
	damage->info_slot = 0;

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_hindervines(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &hindervines_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ivy_lane_denizen(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_GREEN;
		this_test.not_me = 1;
		if( can_target(&td) && new_specific_cip(player, card, event, player, 1+player, &this_test) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return 0;
}

int card_miming_slime(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.pow = get_highest_power(player);
		token.tou = get_highest_power(player);
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ooze_flux(int player, int card, event_t event){

	/* Ooze Flux	|3|G
	 * Enchantment
	 * |1|G, Remove one or more +1/+1 counters from among creatures you control: Put an X/X |Sgreen Ooze creature token onto the battlefield, where X is the
	 * number of +1/+1 counters removed this way. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(1, 1), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XG(1, 1)) ){
			int cr = 0;
			while( can_target(&td) ){
					if( new_pick_target(&td, "Select a creature your control with a +1/+1 counter.", 0, GS_LITERAL_PROMPT) ){
						instance->number_of_targets = 1;
						remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
						cr++;
					}
					else{
						break;
					}
			}
			if( cr > 0 ){
				instance->info_slot = cr;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.pow = instance->info_slot;
		token.tou = instance->info_slot;
		generate_token(&token);
	}

	return 0;
}

int card_predators_rapport(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(player, get_power(instance->targets[0].player, instance->targets[0].card)+get_toughness(instance->targets[0].player, instance->targets[0].card));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_rust_scarab(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DECLARE_BLOCKERS && (instance->state & STATE_ATTACKING) && ! is_unblocked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		td.allowed_controller =  1-player;
		td.preferred_controller = 1-player;

		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_scab_clan_charger(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_XG(1, 1), 2, 4, 0, 0);
}

int card_serene_remembrance(int player, int card, event_t event)
{
  /* Serene Remembrance	|G
   * Sorcery
   * Shuffle ~ and up to three target cards from a single graveyard into their owners' libraries. */

  if (event == EVENT_CAN_CAST)
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

  if (event == EVENT_CAST_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  base_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;

	  load_text(0, "TARGET_GRAVEYARD");
	  if (!select_target(player, card-1000, &td, text_lines[0], &instance->targets[4]))
		{
		  cancel = 1;
		  return 0;
		}

	  if (graveyard_has_shroud(instance->targets[4].player))
		{
		  int i;
		  for (i = 0; i < 3; ++i)
			instance->targets[i].player = instance->targets[i].card = -1;
		}
	  else
		select_multiple_cards_from_graveyard(player, instance->targets[4].player, 0, AI_MAX_VALUE, NULL, 3, &instance->targets[0]);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int i, num_validated = 0, num_targeted = 0;
	  for (i = 0; i < 3; ++i)
		if (instance->targets[i].card != -1)
		  {
			++num_targeted;
			int selected = validate_target_from_grave(player, card, instance->targets[4].player, i);
			if (selected != -1)
			  {
				from_graveyard_to_deck(instance->targets[4].player, selected, 2);
				++num_validated;
			  }
		  }

	  if (num_validated == 0 && num_targeted > 0)
		{
		  spell_fizzled = 1;
		  kill_card(player, card, KILL_DESTROY);
		}
	  else
		{
		  shuffle_into_library(player, card);
		  if (instance->targets[4].player != player)
			shuffle(instance->targets[4].player);
		}
	}

	return 0;
}

int card_skarrg_goliath(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_XG(5, 2), 9, 9, KEYWORD_TRAMPLE, 0);
}

int card_slaughterhorn(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_G(1), 3, 2, 0, 0);
}

// spire tracer --> orchard spirit

int card_sylvan_primordial(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_CREATURE;
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( comes_into_play(player, card, event) && can_target(&td) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

				char msg[100] = "Select a Forest card.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, msg);
				this_test.subtype = SUBTYPE_FOREST;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
			}
		}
	}

	return 0;
}

int card_tower_defence(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 5, KEYWORD_REACH, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_verdant_haven(int player, int card, event_t event){
	int result = wild_growth_aura_all_one_color(player, card, event, -1, COLOR_TEST_ANY_COLORED, 1);
	if (comes_into_play(player, card, event)){
		gain_life(player, 2);
	}
	return result;
}

int card_wasteland_viper(int player, int card, event_t event){
	deathtouch(player, card, event);
	return bloodrush(player, card, event, MANACOST_G(1), 1, 2, 0, SP_KEYWORD_DEATHTOUCH);
}

// wildwood rebirth --> raise dead

// Gold
int card_alms_beast(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES
	  && affected_card_controller == 1-player
	  && !is_humiliated(player, card)
	  && is_blocking_or_blocked_by(player, card, affected_card_controller, affected_card))
	lifelink(affected_card_controller, affected_card, event);

  return 0;
}

int card_assemble_the_legion(int player, int card, event_t event){

	/* Assemble the Legion	|3|R|W
	 * Enchantment
	 * At the beginning of your upkeep, put a muster counter on ~. Then put a 1/1 |Sred and |Swhite Soldier creature token with haste onto the battlefield for
	 * each muster counter on ~. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_MUSTER);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.qty = count_counters(player, card, COUNTER_MUSTER);
		token.color_forced = COLOR_TEST_RED | COLOR_TEST_WHITE;
		token.s_key_plus = SP_KEYWORD_HASTE;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_aurelia_the_warleader(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	haste(player, card, event);

	vigilance(player, card, event);

	// Whenever ~ attacks for the first time each turn, untap all creatures you control. After this phase, there is an additional combat phase.
	if (instance->targets[2].player != 66 && declare_attackers_trigger(player, card, event, 0, player, card)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
		create_legacy_effect(player, card, &finest_hour_legacy);
		instance->targets[2].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}


	return 0;
}

int card_aurelias_fury(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = x_value;
			set_special_flags2(player, card, SF2_X_SPELL);
		}
		int i;
		for(i=0; i<instance->info_slot; i++){
			new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", i, 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		//All the "side effects" of damage is done into "effect_damage.c" in "damage_effects.c"
		int players_damaged[2] = {0, 0};
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				if( instance->targets[i].card == -1 ){
					int dmg = damage_player(instance->targets[i].player, 1, player, card);
					if( ! players_damaged[instance->targets[i].player] ){//No more than 1 emblem per player
						players_damaged[instance->targets[i].player] = 1;
						// Weirdly, damage dealt to a player is "controlled" by that player
						get_card_instance(instance->targets[i].player, dmg)->targets[10].card = CARD_ID_AURELIAS_FURY;
					}
				}
				else{
					int dmg = damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
					get_card_instance(player, dmg)->targets[3].card |= DMG_TAP_IF_DEALT_DAMAGE_THIS_WAY;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bane_alley_broker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == HUMAN && exiledby_choose(player, card, CARD_ID_BANE_ALLEY_BROKER, EXBY_FIRST_FOUND, 0, "Card", 1) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		int check_exile = exiledby_choose(player, card, CARD_ID_BANE_ALLEY_BROKER, EXBY_FIRST_FOUND, 0, "Card", 1) ;
		int abilities[3] = {	generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL),
								generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_BU(1, 1), 0, NULL, NULL) && check_exile,
								player == HUMAN && check_exile
		};

		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Draw and exile", abilities[0], 5,
							"Pick a card from exile", abilities[1], 10,
							"Show exiled cards", abilities[2], 0);
		if( ! choice  ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 3 ){
			exiledby_choose(player, card, CARD_ID_BANE_ALLEY_BROKER, EXBY_CHOOSE, 0, "Card", 1);
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_UB((choice == 2), (choice == 2))) ){
			tap_card(player, card);
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			draw_cards(player, 1);
			if( hand_count[player] > 0 ){
				int leg = 0, idx = 0;
				char buffer[100];
				scnprintf(buffer, 100, "Select a card to exile");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, buffer);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				exiledby_remember(instance->parent_controller, instance->parent_card, player,
									get_original_internal_card_id(instance->parent_controller, selected), &leg, &idx);
				rfg_card_in_hand(instance->parent_controller, selected);
			}
		}
		if( instance->info_slot == 2 ){
			int rval = exiledby_choose(instance->parent_controller, instance->parent_card, CARD_ID_BANE_ALLEY_BROKER, EXBY_CHOOSE, 0, "Card", 1);
			int* loc = (int*)rval;
			if( loc ){
				int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
				int iid = *loc2 & ~0x80000000;
				*loc2 = -1;
				add_card_to_hand(player, iid);
			}
		}
	}

	return 0;
}

int card_biovisionary(int player, int card, event_t event){
	if( current_turn == player && eot_trigger(player, card, event) ){
		if( count_cards_by_id(player, get_id(player, card)) > 3 ){
			lose_the_game(1-player);
		}
	}
	return 0;
}

int card_borborygmos_enraged(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int max = 3;
		int *deck = deck_ptr[player];
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		if( max > 0 ){
			show_deck( player, deck, max, "Cards revealed by Borborygmos", 0, 0x7375B0 );
		}
		int count = 0;
		while( count < max ){
				if( is_what(-1, deck[0], TYPE_LAND) ){
					add_card_to_hand(player, deck[0]);
					remove_card_from_deck(player, 0);
				}
				else{
					mill(player, 1);
				}
				count++;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	char buffer[100];
	scnprintf(buffer, 100, "Select a land card to discard.");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, buffer);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, &td, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					discard_card(player, selected);
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
		}
	}

	return 0;
}

int card_boros_charm(int player, int card, event_t event){

	/* Boros Charm	|R|W
	 * Instant
	 * Choose one - ~ deals 4 damage to target player; or permanents you control gain indestructible until end of turn; or target creature gains double strike
	 * until end of turn. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	enum{
		CHOICE_DAMAGE_PLAYER = 1,
		CHOICE_ALL_INDESTRUCTIBLE,
		CHOICE_DOUBLE_STRIKE
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Damage player", can_target(&td), would_validate_arbitrary_target(&td, 1-player, -1) && life[1-player] <= 4 ? 20 : 5,
								"All indestructible", 1, 10,
								"Give double strike", can_target(&td1), current_phase == PHASE_AFTER_BLOCKING ? 15 : 10);
			if( ! choice  ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		instance->number_of_targets = 0;

		if( instance->info_slot == CHOICE_DAMAGE_PLAYER ){
			pick_target(&td, "TARGET_PLAYER");
		}

		if( instance->info_slot == CHOICE_DOUBLE_STRIKE ){
			pick_target(&td1, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_DAMAGE_PLAYER ){
			if( valid_target(&td) ){
				damage_player(instance->targets[0].player, 4, player, card);
			}
		}
		if( instance->info_slot == CHOICE_ALL_INDESTRUCTIBLE ){
			int c;
			for (c = 0; c < active_cards_count[player]; ++c){
				if (in_play(player, c) && is_what(player, c, TYPE_PERMANENT)){
					create_targetted_legacy_effect(player, card, indestructible_until_eot, player, c);
				}
			}
		}
		if( instance->info_slot == CHOICE_DOUBLE_STRIKE ){
			if( valid_target(&td1) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_call_of_the_nightwing(int player, int card, event_t event){
	/* Call of the Nightwing	|2|U|B
	 * Sorcery
	 * Put a 1/1 |Sblue and |Sblack Horror creature token with flying onto the battlefield.
	 * Cipher */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HORROR, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_BLACK | COLOR_TEST_BLUE;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
		cipher(player, card);
	}

	return 0;
}

int card_cartel_aristocrat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST0, 0, NULL, NULL);
}

int card_clan_defiance(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_abilities |= KEYWORD_FLYING;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_CAN_CAST && (can_target(&td) || can_target(&td1) || can_target(&td2)) ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		int i;
		for(i=0; i<6; i++){
			instance->targets[i].player = instance->targets[i].card = -1;
		}
		if( can_target(&td) ){
			if( new_pick_target(&td, "Select target creature with flying.", 0, GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				int pos = instance->number_of_targets+2;
				instance->targets[pos].player = 1<<0;
			}
		}
		if( can_target(&td1) ){
			if( new_pick_target(&td1, "Select target creature without flying.", -1, GS_LITERAL_PROMPT) ){
				int tt = instance->number_of_targets-1;
				state_untargettable(instance->targets[tt].player, instance->targets[tt].card, 1);
				int pos = instance->number_of_targets+2;
				instance->targets[pos].player = 1<<1;
			}
		}
		if( can_target(&td2) ){
			if( new_pick_target(&td2, "TARGET_PLAYER", -1, 0) ){
				int pos = instance->number_of_targets+2;
				instance->targets[pos].player = 1<<2;
			}
		}
		for(i=0; i<3; i++){
			if( instance->targets[i].player != -1 && instance->targets[i].card != -1 ){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
		if( instance->number_of_targets == 0 ){
			spell_fizzled = 1;
		}
		else{
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				instance->info_slot = x_value;
				set_special_flags2(player, card, SF2_X_SPELL);
			}
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			int good = 0;
			if( instance->targets[i+3].player > -1 ){
				if( (instance->targets[i+3].player & (1<<0)) && validate_target(player, card, &td, i) ){
					good = 1;
				}
				if( (instance->targets[i+3].player & (1<<1)) && validate_target(player, card, &td1, i) ){
					good = 1;
				}
				if( (instance->targets[i+3].player & (1<<2)) && validate_target(player, card, &td2, i) ){
					good = 1;
				}
				if( good ){
					damage_creature(instance->targets[i].player, instance->targets[i].card, instance->info_slot, player, card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_consuming_aberration(int player, int card, event_t event){
	/* Consuming Aberration	|3|U|B
	 * Creature - Horror 100/100
	 * ~'s power and toughness are each equal to the number of cards in your opponents' graveyards.
	 * Whenever you cast a spell, each opponent reveals cards from the top of his or her library until he or she reveals a land card, then puts those cards into his or her graveyard. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) && player != -1 ){
		event_result+=count_graveyard(1-player);
	}

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int *deck = deck_ptr[1-player];
		if( deck[0] != -1 ){
			int count = 0;
			while( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
					count++;
			}
			count++;
			show_deck(HUMAN, deck, count, "Consuming Aberration revealed...", 0, 0x7375B0 );
			mill(1-player, count);
		}
	}

	return 0;
}

int card_deathpact_angel(int player, int card, event_t event){
	/* Deathpact Angel	|3|W|B|B
	 * Creature - Angel 5/5
	 * Flying
	 * When ~ dies, put a 1/1 |Swhite and |Sblack Cleric creature token onto the battlefield. It has "|3|W|B|B, |T, Sacrifice this creature: Return a card named ~ from your graveyard to the battlefield." */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_CLERIC, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_BLACK | COLOR_TEST_WHITE;
		token.special_infos = 66;
		generate_token(&token);
	}

	return 0;
}

int card_dimir_charm(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	counterspell_target_definition(player, card, &td1, TYPE_SORCERY);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	target_definition_t td3;
	default_target_definition(player, card, &td3, 0);
	td3.zone = TARGET_ZONE_PLAYERS;
	td3.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	enum{
		CHOICE_COUNTER_SORCERY = 1,
		CHOICE_KILL_CREATURE,
		CHOICE_LOOK_TOP_3
	};

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_COUNTERSPELL, &td1, NULL, 1, NULL) ){
			return 99;
		}
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td2, NULL, 1, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td3, NULL, 1, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int options[3] = {	generic_spell(player, card, EVENT_CAN_CAST, GS_COUNTERSPELL, &td1, NULL, 1, NULL),
								generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td2, NULL, 1, NULL),
								generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td3, NULL, 1, NULL)
			};

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Counter a sorcery", options[0], 15,
								"Kill a creature with Pow <=2", options[1], 10,
								"Look at the top 3", options[2], 5);
			if( ! choice  ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		instance->number_of_targets = 0;
		if( instance->info_slot == CHOICE_COUNTER_SORCERY ){
			return counterspell(player, card, event, &td1, 0);
		}
		if( instance->info_slot == CHOICE_KILL_CREATURE ){
			new_pick_target(&td2, "Select target creature with Power 2 or less.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_LOOK_TOP_3 ){
			pick_target(&td3, "TARGET_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_COUNTER_SORCERY ){
			return counterspell(player, card, event, &td1, 0);
		}
		if( instance->info_slot == CHOICE_KILL_CREATURE && valid_target(&td2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot == CHOICE_LOOK_TOP_3  && valid_target(&td3) ){
			int amount = MIN(3, count_deck(instance->targets[0].player));
			if( amount > 0 ){
				int ai_mode = player == instance->targets[0].player ? AI_MAX_VALUE : AI_MIN_VALUE;
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top");
				this_test.create_minideck = amount;
				this_test.no_shuffle = 1;
				int card_added = new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_TPLAYER_HAND, 1, ai_mode, &this_test);
				amount--;
				if( amount ){
					mill(instance->targets[0].player, amount);
				}
				put_on_top_of_deck(instance->targets[0].player, card_added);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dinrova_horror(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		card_instance_t* instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT")){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			discard(instance->targets[0].player, 0, player);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_domri_rade(int player, int card, event_t event){

	/* Domri Rade	|1|R|G
	 * Planeswalker - Domri (3)
	 * +1: Look at the top card of your library. If it's a creature card, you may reveal it and put it into your hand.
	 * -2: Target creature you control fights another target creature.
	 * -7: You get an emblem with "Creatures you control have double strike, trample, hexproof, and haste." */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.allowed_controller = player;

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);
		td3.preferred_controller = 1-player;
		td3.allowed_controller = 1-player;

		enum{
			CHOICE_LOOK_TOP_CARD = 1,
			CHOICE_FIGHT,
			CHOICE_EMBLEM
		};
		int abilities[3] = {	1,
								(player == HUMAN ? (target_available(player, card, &td2) > 1 || (can_target(&td2) && can_target(&td3))) :
													(can_target(&td2) && can_target(&td3))),
								1,
		};
		int priorities[3] = {	8,
								10,
								(count_subtype(player, TYPE_CREATURE, -1)*2)+((7-count_counters(player, card, COUNTER_LOYALTY))*3)
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						"Reveal top card", abilities[0], priorities[0], 1,
						"Two creatures fight", abilities[1], priorities[1], -2,
						"Emblem", abilities[2], priorities[2], -7);

		if (event == EVENT_CAN_ACTIVATE){
			if (!choice)
				return 0;
		}
		else if (event == EVENT_ACTIVATE){
				instance->number_of_targets = 0;
				if( choice == CHOICE_FIGHT ){
					if( player == HUMAN ){
						if( new_pick_target(&td2, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
							state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
							new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
							state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
						}
					}
					else{
						if( pick_target(&td2, "TARGET_CREATURE") ){
							new_pick_target(&td3, "TARGET_CREATURE", 1, 1);
						}
					}
				}
		}
		else	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_LOOK_TOP_CARD:
				{
					int *deck = deck_ptr[player];
					if( deck[0] != -1 ){
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to reveal.");
						this_test.create_minideck = 1;
						this_test.no_shuffle = 1;
						new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
					}
				}
				break;

				case CHOICE_FIGHT:
					if( validate_target(player, card, &td1, 0) && validate_target(player, card, &td1, 1) ){
						fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
					}
					break;

				case CHOICE_EMBLEM:
					generate_token_by_id(player, card, CARD_ID_DOMRIS_EMBLEM);
					break;
			}
	}

	return planeswalker(player, card, event, 3);
}

int card_domri_rades_emblem(int player, int card, event_t event){

	if (event == EVENT_ABILITIES &&
		affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	   ){
		event_result |= KEYWORD_DOUBLE_STRIKE | KEYWORD_TRAMPLE;
		hexproof(affected_card_controller, affected_card, event);
		haste(affected_card_controller, affected_card, event);
	}

	return 0;
}

// drakewing_krasis --> vanilla

static int effect_duskmantle_guildmage(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  // from library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == instance->targets[0].player)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int i;
		  for (i = 0; i < num_cards_milled; ++i)
			lose_life(instance->targets[0].player, 1);
		}
	}

  // from everywhere else
  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, instance->targets[0].player,
																			   RESOLVE_TRIGGER_MANDATORY, NULL))
	lose_life(instance->targets[0].player, 1);

  if (eot_trigger(player, card, event))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_duskmantle_guildmage(int player, int card, event_t event){

	/* Duskmantle Guildmage	|U|B
	 * Creature - Human Wizard 2/2
	 * |1|U|B: Whenever a card is put into an opponent's graveyard from anywhere this turn, that player loses 1 life.
	 * |2|U|B: Target player puts the top two cards of his or her library into his or her graveyard. */


	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 1, 1, 0, 0, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		instance->number_of_targets = 0;

		int choice = 2;
		if( has_mana_for_activated_ability(player, card, MANACOST_XBU(2, 1, 1)) && can_target(&td) ){
			int ai_choice = 1;
			if( has_mana_multi(player, MANACOST_XBU(1, 1, 1)) ){
				if( instance->targets[1].player != 66 && has_mana_for_activated_ability(player, card, MANACOST_XBU(3, 2, 2))){
					ai_choice = 0;
				}
			}
			choice = 1+do_dialog(player, player, card, -1, -1, " Mill as damage\n Mill a player\n Cancel", ai_choice);
		}
		else{
			choice = 1;
		}

		if (choice == 3){
			cancel = 1;
			return 0;
		}

		if( charge_mana_for_activated_ability(player, card, MANACOST_XBU(choice, 1, 1))){
			if( choice == 1 || (choice == 2 && pick_target(&td, "TARGET_PLAYER")) ){
				instance->info_slot = 65+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			parent->targets[1].player = 66;
			int legacy = create_legacy_effect(instance->parent_controller, instance->parent_card, &effect_duskmantle_guildmage);
			card_instance_t *leg = get_card_instance(instance->parent_controller, legacy);
			leg->targets[0].player = 1-player;
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			mill(instance->targets[0].player, 2);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_duskmantle_seer(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		dark_confidant_effect(player, card, player);
		dark_confidant_effect(player, card, 1-player);
	}

	return 0;
}

int card_elusive_krasis(int player, int card, event_t event){
	unblockable(player, card, event);
	evolve(player, card, event);
	return 0;
}

// executioner's swing --> impossible

int card_fathom_mage(int player, int card, event_t event)
{
  /* Fathom Mage	|2|G|U	0x200dba3
   * Creature - Human Wizard 1/1
   * Evolve
   * Whenever a +1/+1 counter is placed on ~, you may draw a card. */

  enable_xtrigger_flags |= ENABLE_XTRIGGER_1_1_COUNTERS;

  evolve(player, card, event);

  if (xtrigger_condition() == XTRIGGER_1_1_COUNTERS && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause == card && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_AI(player);
	  if (event == EVENT_RESOLVE_TRIGGER)
		draw_cards(player, counters_added);
	}

  // If this had counters placed on it "as it was entering the battlefield", it was done long before this, and this won't have gotten the triggers.
  int n;
  if (event == EVENT_RESOLVE_SPELL && (n = count_1_1_counters(player, card)) > 0)
	draw_some_cards_if_you_want(player, card, player, n);

  return 0;
}

int card_firemane_avenger(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 3);
			gain_life(player, 3);
		}
	}
	return 0;
}

int card_fortress_cyclops(int player, int card, event_t event){

	// Whenever ~ attacks, it gets +3/+0 until end of turn.
	if( declare_attackers_trigger(player, card, event, 0, player, card) ){
		pump_until_eot(player, card, player, card, 3, 0);
	}

	// Whenever ~ blocks, it gets +0/+3 until end of turn.
	if( blocking(player, card, event) ){
		pump_until_eot(player, card, player, card, 0, 3);
	}

	return 0;
}

int card_foundry_champion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, count_subtype(player, TYPE_CREATURE, -1));
			instance->number_of_targets = 0;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL) ){
			return 1;
		}
		generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_R(1)) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_W(1)) ){
				int ai_choice = 1;
				if( is_attacking(player, card) && is_unblocked(player, card) ){
					ai_choice = 0;
				}
				choice = do_dialog(player, player, card, -1, -1, " Pump power\n Pump toughness\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_RW((choice == 0), (choice == 1))) ){
			instance->targets[0].player = 1-choice;
			instance->targets[0].card = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
						instance->targets[0].player, instance->targets[0].card);
	}

	return 0;
}

int card_frenzied_tilling(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			char msg[100] = "Select a basic land card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			this_test.subtype = SUBTYPE_BASIC;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_ghor_clan_rampager(int player, int card, event_t event){
	return bloodrush(player, card, event, MANACOST_GR(1, 1), 4, 4, KEYWORD_TRAMPLE, 0);
}

int card_ground_assault(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_subtype(player, TYPE_LAND, -1), player, card);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int has_flying_but_not_prot_red_prot_green(int iid, int me, int player, int card){
	if( check_for_ability(player, card, KEYWORD_FLYING) && ! check_for_ability(player, card, KEYWORD_PROT_RED | KEYWORD_PROT_GREEN) ){
		return 1;
	}
	return 0;
}

int card_gruul_charm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum{ 	CHOICE_CANNOT_BLOCK = 1,
			CHOICE_GET_BACK_PERMANENTS,
			CHOICE_DAMAGE_FLYERS
	};

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;

			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_CREATURE);
			this_test2.special_selection_function = has_flying_but_not_prot_red_prot_green;
			this_test2.toughness = 4;
			this_test2.toughness_flag = 3;

			int priorities[3] = {0, 0, 0};
			if( current_turn == player )
				priorities[CHOICE_CANNOT_BLOCK-1] = count_subtype(player, TYPE_CREATURE, -1)*3;
				priorities[CHOICE_CANNOT_BLOCK-1] += check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)*2;

			int i;
			for(i=0; i<active_cards_count[1-player]; i++)
				priorities[CHOICE_GET_BACK_PERMANENTS-1] += (in_play(1-player, i) && is_stolen(1-player, i))*3;

			priorities[CHOICE_DAMAGE_FLYERS-1] += check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test2)*3;
			priorities[CHOICE_DAMAGE_FLYERS-1] -= check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test2)*2;

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Nonflyers cannot block", 1, priorities[CHOICE_CANNOT_BLOCK-1],
							"Get back your permanents", 1, priorities[CHOICE_GET_BACK_PERMANENTS-1],
							"3 damage to all flyers", 1, priorities[CHOICE_DAMAGE_FLYERS-1]);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_CANNOT_BLOCK ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			pump_creatures_until_eot(player, card, ANYBODY, 0, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK, &this_test);
		}

		if( instance->info_slot == CHOICE_GET_BACK_PERMANENTS ){
			get_back_your_permanents(player, card, TYPE_PERMANENT);
		}

		if( instance->info_slot == CHOICE_DAMAGE_FLYERS ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			new_damage_all(player, card, ANYBODY, 3, 0, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gruul_ragebeast(int player, int card, event_t event){
	/* Gruul Ragebeast	|5|R|G
	 * Creature - Beast 6/6
	 * Whenever ~ or another creature enters the battlefield under your control, that creature fights target creature an opponent controls. */

	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = 1-player;
		td2.allowed_controller = 1-player;
		td2.allow_cancel = 0;
		td2.who_chooses = player;
		td2.illegal_abilities = get_protections_from(instance->targets[1].player, instance->targets[1].card);

		instance->number_of_targets = 0;

		if( can_target(&td2) && new_pick_target(&td2, "TARGET_CREATURE", 0, 0) ){
			fight(instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_high_priest_of_penance(int player, int card, event_t event)
{
  if (damage_dealt_to_me_arbitrary(player, card, event, DDBM_TRIGGER_OPTIONAL, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_LAND;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_NONLAND_PERMANENT"))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return 0;
}

int card_hydroform(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELEMENTAL);
			land_animation2(player, card, instance->targets[0].player, instance->targets[0].card, 1, 3, 3, KEYWORD_FLYING, 0, 0, 0);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

// kingping's pet --> cloudfin_raptor

static int effect_lazav_dimir_mastermind(int player, int card, event_t event);
static void lazav_mastermind_impl(int player, int card, event_t event, int is_effect)
{
  card_instance_t* instance = get_card_instance(player, card);
  int p, c;
  if (is_effect)
	{
	  p = instance->damage_target_player;
	  c = instance->damage_target_card;
	  if (!in_play(p, c))
		return;
	}
  else
	{
	  p = player;
	  c = card;
	}

  check_legend_rule(p, c, event);

  hexproof(p, c, event);

  int num_creatures = 0;
  int iids[500];

  // From library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == 1-player && !is_humiliated(p, c))
	{
	  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
		return;

	  int i;
	  for (i = 0; i < num_cards_milled; ++i)
		if (is_what(-1, cards_milled[i].internal_card_id, TYPE_CREATURE))
		  {
			if (event == EVENT_TRIGGER)
			  {
				event_result |= RESOLVE_TRIGGER_AI(player);
				return;
			  }

			if (event == EVENT_RESOLVE_TRIGGER)
			  {
				iids[num_creatures] = cards_milled[i].internal_card_id;
				++num_creatures;
			  }
		  }
	}

  // From anywhere else
  enable_xtrigger_flags |= ENABLE_XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY;
  if (xtrigger_condition() == XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, 1-player, RESOLVE_TRIGGER_OPTIONAL, &test))
		{
		  iids[0] = trigger_cause;
		  num_creatures = 1;
		}
	}

  if (num_creatures > 0)	// Only possible when resolving an XTRIGGER_MILLED or XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY
	{
	  int chosen;
	  if (IS_AI(player))
		chosen = recorded_rand(player, num_creatures + 1);	// +1 so ai can choose to stay put
	  else
		chosen = show_deck(player, iids, num_creatures, "Lazav, Dimir Mastermind: become creature", 0, 0x7281a4);	// DIALOGBUTTONS[1] = "Cancel"

	  if (chosen >= 0 && chosen < num_creatures)
		{
		  if (!is_effect)
			{
			  add_a_subtype(p, c, SUBTYPE_LEGEND);
			  int leg = create_targetted_legacy_effect(p, c, effect_lazav_dimir_mastermind, p, c);
			  if (leg != -1)
				{
				  card_instance_t* legacy = get_card_instance(p, leg);
				  legacy->state |= STATE_PROCESSING;	// Since it was created during a trigger that it would try to respond to itself
				}
			}

		  cloning(p, c, -1, iids[chosen]);	// no need to verify legend, since he still has only his old name
		}
	}
}
static int effect_lazav_dimir_mastermind(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  lazav_mastermind_impl(player, card, event, 1);

  return 0;
}
int card_lazav_dimir_mastermind(int player, int card, event_t event)
{
  /* Lazav, Dimir Mastermind	|U|U|B|B
   * Legendary Creature - Shapeshifter 3/3
   * Hexproof
   * Whenever a creature card is put into an opponent's graveyard from anywhere, you may have ~ become a copy of that card except its name is still ~, it's
   * legendary in addition to its other types, and it gains hexproof and this ability. */

  cloning_card(player, card, event);

  lazav_mastermind_impl(player, card, event, 0);

  return 0;
}

int card_martial_glory(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			new_pick_target(&td, "TARGET_CREATURE", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 0);
		}
		if( validate_target(player, card, &td, 1) ){
			pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 0, 3);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_master_biomancer(int player, int card, event_t event){
	/*
	  Master Biomancer |2|U|G
	  Creature - Elf Wizard 2/4
	  Each other creature you control enters the battlefield with a number of additional +1/+1 counters on it equal to Master Biomancer's power
	  and as a Mutant in addition to its other types.
	*/
	if( in_play(player, card) && ! affect_me(player, card) && ! is_humiliated(player, card) && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		enters_the_battlefield_with_counters(affected_card_controller, affected_card, event, COUNTER_P1_P1, get_power(player, card));
		if( event == EVENT_CAST_SPELL ){
			add_a_subtype(affected_card_controller, affected_card, SUBTYPE_MUTANT);
		}
	}

	return 0;
}

int card_merciless_eviction(int player, int card, event_t event){
	/* Merciless Eviction	|4|W|B
	 * Sorcery
	 * Choose one -
	 * * Exile all artifacts.
	 * * Exile all creatures.
	 * * Exile all enchantments.
	 * * Exile all planeswalkers. */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int types[4] = {TYPE_ARTIFACT, TYPE_CREATURE, TYPE_ENCHANTMENT, TARGET_TYPE_PLANESWALKER};

		int ai_choice = 0;
		if (IS_AI(player)){
			if( count_subtype(player, TYPE_CREATURE, -1) < count_subtype(1-player, TYPE_CREATURE, -1) ){
				ai_choice = 1;
			} else {
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ENCHANTMENT);
				this_test.subtype = SUBTYPE_PLANESWALKER;
				if( check_battlefield_for_special_card(player, card, player, 4, &this_test) < check_battlefield_for_special_card(player, card, 1-player, 4, &this_test) ){
					ai_choice = 3;
				} else {
					default_test_definition(&this_test, TYPE_ENCHANTMENT);
					this_test.type_flag = F1_NO_PWALKER;
					if( check_battlefield_for_special_card(player, card, player, 4, &this_test) < check_battlefield_for_special_card(player, card, 1-player, 4, &this_test) ){
						ai_choice = 2;
					}
				}
			}
		}

		int choice = do_dialog(player, player, card, -1, -1, " Exile all artifacts\n Exile all creatures\n Exile all enchantments\n Exile all planeswalkers", ai_choice);

		test_definition_t this_test2;
		default_test_definition(&this_test2, types[choice]);
		new_manipulate_all(player, card, 2, &this_test2, KILL_REMOVE);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mind_grind(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		if( x_value > 0 ){
			instance->info_slot = x_value;
			return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[1-player];
		if( deck[0] != -1 ){
			int count = 0;
			int l_count = 0;
			while( deck[count] != -1 && l_count < instance->info_slot ){
					if( is_what(-1, deck[count], TYPE_LAND) ){
						l_count++;
					}
					count++;
			}
			show_deck( player, deck, count, "Mind Grind revealed...", 0, 0x7375B0 );
			mill(1-player, count);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// mortus strider --> infinite cockroaches

int card_mystic_genesis(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->info_slot = get_cmc(card_on_stack_controller, card_on_stack);
			return counterspell(player, card, event, NULL, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_OOZE, &token);
			token.pow = instance->info_slot;
			token.tou = instance->info_slot;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// nimbus swimmer --> krakilin

static int ozbedat_legacy(int player, int card, event_t event){
	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( check_rfg(player, CARD_ID_OBZEDAT_GHOST_COUNCIL) ){
			int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_OBZEDAT_GHOST_COUNCIL));
			remove_card_from_rfg(player, CARD_ID_OBZEDAT_GHOST_COUNCIL);
			put_into_play(player, card_added);
			give_haste(player, card_added);
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_ozbedat_ghost_council(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			lose_life(1-player, 2);
			gain_life(player, 2);
		}
	}

	if( eot_trigger_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player)) ){
		create_legacy_effect(player, card, &ozbedat_legacy);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_one_thousand_lashes(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){

		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			lose_life(instance->damage_target_player, 1);
		}
	}

	return card_arrest(player, card, event);
}

int card_ordruun_veteran(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
	}
	return 0;
}

int card_orzhov_charm(int player, int card, event_t event){

	/* Orzhov Charm	|W|B
	 * Instant
	 * Choose one - Return target creature you control and all Auras you control attached to it to their owner's hand;
	 * or destroy target creature and you lose life equal to its toughness;
	 * or return target creature card with converted mana cost 1 or less from your graveyard to the battlefield. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	char msg[100] = "Select a creature card with CMC 1 or less.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.cmc = 2;
	this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

	enum{ 	CHOICE_BOUNCE = 1,
			CHOICE_KILL,
			CHOICE_REANIMATE
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		if( result ){
			return result;
		}
		result = generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
		if( result ){
			return result;
		}
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int opts[3];
			opts[0] = generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td, NULL, 1, NULL);
			opts[1] = generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td1, NULL, 1, NULL);
			opts[2] = generic_spell(player, card, EVENT_CAN_CAST, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Bounce creature & auras", opts[0], 8,
							"Kill a creature", opts[1], 10,
							"Reanimate creature with CMC 1", opts[2], 6);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == CHOICE_BOUNCE ){
			new_pick_target(&td, "Select a creature to bounce.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_KILL ){
			new_pick_target(&td1, "Select a creature to kill.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_REANIMATE ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_BOUNCE && valid_target(&td) ){
			int p, c;
			for(p=0; p<2; p++){
				for(c=active_cards_count[p]-1; c>-1; c--){
					if( in_play(p, c) && has_subtype(p, c, SUBTYPE_AURA) ){
						card_instance_t *aura = get_card_instance(p, c);
						if( aura->damage_target_player == instance->targets[0].player && aura->damage_target_card == instance->targets[0].card &&
							get_owner(p, c) == player
						  ){
							bounce_permanent(p, c);
						}
					}
				}
			}
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == CHOICE_KILL && valid_target(&td1) ){
			lose_life(player, get_toughness(instance->targets[0].player, instance->targets[0].card));
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot == CHOICE_REANIMATE ){
			int result = validate_target_from_grave_source(player, card, player, 0);
			if( result > -1 ){
				reanimate_permanent(player, card, player, result, REANIMATE_DEFAULT);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_paranoid_delusion(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 3);
			cipher(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_primal_visitation(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 3, 3, 0, SP_KEYWORD_HASTE, 0, 0, 0);
}

int card_prime_speaker_zegana(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if( comes_into_play(player, card, event) ){
		int i;
		int result = -1;
		for(i=0;i<active_cards_count[player]; i++){
			if( i != card && in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
				int x = get_power(player, i);
				if(  x > result ){
					result = x;
				}
			}
		}
		add_1_1_counters(player, card, result);
		draw_cards(player, get_power(player, card));
	}
	return 0;
}

int card_psychic_strike(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			mill(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
}

int card_purge_the_profane(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_rubblehulk(int player, int card, event_t event)
{
  /* Rubblehulk	|4|R|G
   * Creature - Elemental 100/100
   * ~'s power and toughness are each equal to the number of lands you control.
   * Bloodrush - |1|R|G, Discard ~: Target attacking creature gets +X/+X until end of turn, where X is the number of lands you control. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	event_result += count_permanents_by_type(player, TYPE_LAND);

  if (IS_ACTIVATING_FROM_HAND(event))
	{
	  int pt = event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ? count_permanents_by_type(player, TYPE_LAND) : 0;
	  return bloodrush(player, card, event, MANACOST_XRG(1,1,1), pt,pt, 0,0);
	}
  else
	return 0;
}

// ruination wurm --> vanilla

int card_shambleshark(int player, int card, event_t event){
	evolve(player, card, event);
	return flash(player, card, event);
}

int card_signal_the_clans(int player, int card, event_t event){

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		int tutored[2][3];
		int t_count = 0;
		if( player == HUMAN ){
			while( t_count < 3 ){
					int result = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
					if( result != -1 ){
						tutored[0][t_count] = result;
						tutored[1][t_count] = deck[result];
						t_count++;
					}
					else{
						break;
					}
			}
		}
		else{
			int rnd = internal_rand(count_deck(player));
			int rounds = 0;
			while( rounds < 2 && t_count < 3){
					if( is_what(-1, deck[rnd], TYPE_CREATURE) ){
						int good = 1;
						int k;
						for(k=0; k<t_count; k++){
							if( deck[rnd] == tutored[1][k] ){
								good = 0;
							}
						}
						if( good ){
							tutored[0][t_count] = rnd;
							tutored[1][t_count] = deck[rnd];
							t_count++;
						}
					}
					rnd++;
					if( deck[rnd] == -1 ){
						rnd = 0;
						rounds++;
					}
			}
		}
		if( t_count ){
			show_deck( 1-player, tutored[1], t_count, "Cards chosen for Signal the Clans", 0, 0x7375B0 );
			if( t_count == 3 ){
				int good = 1;
				int i, k;
				for(i=0; i<2; i++){
					for(k=i+1; k<3; k++){
						if( tutored[1][i] == tutored[1][k] ){
							good = 0;
							break;
						}
					}
					if( good == 0 ){
						break;
					}
				}
				if( good == 1 ){
					int rnd = internal_rand(3);
					add_card_to_hand(player, tutored[1][rnd]);
					remove_card_from_deck(player, tutored[0][rnd]);
				}
			}
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_simic_charm(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	enum{ 	CHOICE_PUMP = 1,
			CHOICE_HEXPROOF,
			CHOICE_BOUNCE
	};

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int priorities[3] = {	current_turn == player && current_phase == PHASE_AFTER_BLOCKING ? 10 : 5,
									current_turn != player ? 10 : 5,
									current_turn != player ? 8 : 5,
			};

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Pump a creature", can_target(&td1), priorities[0],
							"Hexproof to all", 1, priorities[1],
							"Bounce a creature", can_target(&td2), priorities[2]);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == CHOICE_PUMP ){
			new_pick_target(&td1, "Select a creature to pump.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_BOUNCE ){
			new_pick_target(&td2, "Select a creature to bounce.", 0, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_PUMP && valid_target(&td1) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
		if( instance->info_slot == CHOICE_HEXPROOF ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) ){
						pump_ability_until_eot(player, card, player, count, 0, 0, 0, SP_KEYWORD_HEXPROOF);
					}
					count--;
			}
		}
		if( instance->info_slot == CHOICE_BOUNCE && valid_target(&td2) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_skarrg_guildmage(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.illegal_type = TYPE_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_GR(1, 1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XGR(1, 1, 1), 0, &td, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_GR(1, 1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XGR(1, 1, 1), 0, &td, NULL) ){
				int ai_choice = 0;
				if( (current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS) ||
					(current_turn != player && current_phase < PHASE_DECLARE_BLOCKERS)
				  ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Pump your dudes\n Animate a land\n Cancel", ai_choice);
			}
		}

		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XGR(choice, 1, 1)) ){
			instance->number_of_targets = 0;
			if( choice == 1 ){
				pick_target(&td, "TARGET_LAND");
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_subtype_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, -1, 0, 0, KEYWORD_TRAMPLE, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELEMENTAL);
			land_animation2(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 4, 4, 0, 0, 0, 0);
		}
	}

	return 0;
}

// soul ransom --> impossible

int card_spark_trooper(int player, int card, event_t event){
	lifelink(player, card, event);
	return card_ball_lightning(player, card, event);
}

// soldier token --> rhino token

int card_sunhome_guildmage(int player, int card, event_t event){
	/* Sunhome Guildmage	|R|W
	 * Creature - Human Wizard 2/2
	 * |1|R|W: Creatures you control get +1/+0 until end of turn.
	 * |2|R|W: Put a 1/1 |Sred and |Swhite Soldier creature token with haste onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, MANACOST_XRW(1, 1, 1)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 1;
		if( has_mana_for_activated_ability(player, card, MANACOST_XRW(1, 1, 1)) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_XRW(2, 1, 1)) ){
				if( current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
					ai_choice = 0;
				}
				choice = do_dialog(player, player, card, -1, -1, " Pump your creatures\n Generate a Soldier\n Cancel", ai_choice);
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XRW((choice == 0 ? 1 : 2), 1, 1)) ){
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_subtype_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, -1, 1, 0, 0, 0);
		}
		if( instance->info_slot == 67 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SOLDIER, &token);
			token.color_forced = COLOR_TEST_RED | COLOR_TEST_WHITE;
			token.s_key_plus = SP_KEYWORD_HASTE;
			generate_token(&token);
		}
	}

	return 0;
}

static int effect_thespians_stage(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  if (IS_ACTIVATING(event))
	{
	  card_instance_t* instance = in_play(player, card);
	  if (!instance)
		return 0;

	  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE)
		{
		  // Store attached-to player/card continuously up until activation, since they get set to -1/-1 in the attached card if the attachment leaves play
		  instance->targets[9].player = instance->damage_target_player;
		  instance->targets[9].card = instance->damage_target_card;
		}

	  int p = instance->targets[9].player;
	  int c = instance->targets[9].card;

	  if (p < 0 || c < 0)	// not yet attached
		return 0;

	  card_instance_t* aff = in_play(p, c);
	  if (!aff)
		return 0;

	  target_definition_t td;
	  default_target_definition(p, c, &td, TYPE_LAND);
	  td.preferred_controller = ANYBODY;

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (is_tapped(p, c) || is_animated_and_sick(p, c) || !can_use_activated_abilities(p, c) || !can_target(&td))
			return 0;
		  aff->state |= STATE_TAPPED;
		  count_mana();
		  int rval = has_mana_for_activated_ability(p, c, MANACOST_X(2));
		  aff->state &= ~STATE_TAPPED;
		  return rval;
		}

	  if (event == EVENT_ACTIVATE)
		{
		  aff->state |= STATE_TAPPED;
		  if (!charge_mana_for_activated_ability(p, c, MANACOST_X(2)))
			{
			  aff->state &= ~STATE_TAPPED;
			  return 0;
			}

		  load_text(0, "TARGET_LAND");
		  instance->number_of_targets = 0;
		  int result = select_target(p, c - 1000, &td, text_lines[0], &instance->targets[0]);
		  if (!result)
			{
			  aff->state &= ~STATE_TAPPED;
			  cancel = 1;
			  return 0;
			}
		  instance->number_of_targets = 1;
		  dispatch_event(p, c, EVENT_TAP_CARD);
		}

	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  if (!validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
			{
			  spell_fizzled = 1;
			  return 0;
			}

		  card_instance_t* tgt = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		  /* Find internal_card_id to copy.  We want the current state if it's copying something else, but not an animated state or changed-into-artifact state.
		   *
		   * If it's not a creature, just take a round trip to csvid and back, so we don't copy change-into-artifact effects.
		   * If it's an Assembly Worker, then we want Mishra's Factory.
		   * Otherwise, if it's a Dryad Arbor, then we want Dryad Arbor.
		   * Otherwise, if it has targets[13].player set, assume it's a C manland; targets[13].player has the unanimated csvid.
		   *
		   * In any case, set up targets[12] and targets[13] so manlands work right.  (Cripes, that's wasteful.)
		   */
		  card_data_t* cd = &cards_data[tgt->internal_card_id];
		  int csvid = cd->id;
		  if (cd->type & TYPE_CREATURE)
			{
			  if (csvid == CARD_ID_ASSEMBLY_WORKER)
				csvid = CARD_ID_MISHRAS_FACTORY;
			  else if (csvid != CARD_ID_DRYAD_ARBOR && tgt->targets[13].player >= 0)
				csvid = tgt->targets[13].player;
			}

		  int iid = get_internal_card_id_from_csv_id(csvid);
		  if (iid == 0)	// Swamp - irritatingly inconvenient to store in dummy3
			iid = iid_draw_a_card;

		  get_card_instance(instance->parent_controller, instance->parent_card)->dummy3 = aff->targets[12].card = iid;
		  aff->targets[12].player = aff->targets[13].player = aff->targets[13].card = csvid;

		  // Test suite:
		  // Change to Underground Sea => check manastripes
		  // Change to Tropical Island => check manastripes
		  // Change to Mishra's Factory, then animate => should be Assembly Worker
		  // Change to Assembly Worker, then activate => should be Mishra's Factory until activate, then (after animating) Assembly Worker
		  // Change to Dryad Arbor => should be green
		  // Change to Celestial Colonnade, then animate => should be Celestial Colonnade Animated
		  // Change to Celestial Colonnade Animated, then animate => should be an unanimate Celestial Colonnade, then (after animating) Celestial Colonnade Animated
		  // Change to another Thespian's Stage that's copying something else => should be whatever the other Thespian's Stage was copying (unless it was animated, in which case it should become the land that can animate into that)
		  // Change to Forest, then animate with Living Lands => should be 1/1
		  // Put Living Lands in play then change to Forest => should be 1/1
		  // Change to Swamp => should be a Swamp
		}
	}

  if (event == EVENT_CHANGE_TYPE)
	{
	  // Mishra's Factory only responds during the first pass (wrongly, I think); patch over it
	  if ((land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
		  && cards_data[event_result].id != CARD_ID_MISHRAS_FACTORY
		  && cards_data[event_result].id != CARD_ID_ASSEMBLY_WORKER)
		return 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  int iid = instance->dummy3;
	  if (!iid)
		return 0;
	  if (iid == iid_draw_a_card)
		iid = 0;	// Swamp - irritatingly inconvenient to store in dummy3

	  int p = instance->damage_target_player;
	  int c = instance->damage_target_card;
	  if (!affect_me(p, c))
		return 0;

	  card_instance_t* aff = in_play(p, c);
	  if (!aff)
		return 0;


	  if (land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
		{
		  if (iid >= 0 && cards_data[iid].id == CARD_ID_ASSEMBLY_WORKER)
			event_result = iid;
		  return 0;
		}

	  land_can_be_played |= LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS;	// So manlands can animate themselves
	  event_result = iid;

	  aff->regen_status |= KEYWORD_RECALC_SET_COLOR;	// So dryad arbor is correctly colored
	}

  return 0;
}
int card_thespians_stage(int player, int card, event_t event)
{
  // |2, |T: ~ becomes a copy of target land and gains this ability.
  if (event == EVENT_RESOLVE_SPELL)
	create_targetted_legacy_activate(player, card, effect_thespians_stage, player, card);

  // Don't send through for EVENT_CHANGE_TYPE; let the legacy handle it.
  if (event == EVENT_CHANGE_TYPE)
	return 0;

  // |T: Add |1 to your mana pool.
  return mana_producer(player, card, event);
}

int card_treasury_thrull(int player, int card, event_t event){

	extort(player, card, event);

	// Whenever ~ attacks, you may return target artifact, creature, or enchantment card from your graveyard to your hand.
	if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT, "Select target artifact, creature, or enchantment card.");

		if (new_special_count_grave(player, &this_test) > 0 && !graveyard_has_shroud(player) &&
			declare_attackers_trigger(player, card, event, 0, player, card)
		   ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}
	return 0;
}

int card_truefire_paladine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	vigilance(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, MANACOST_RW(1, 1)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! is_unblocked(player, card) && ! check_for_ability(player, card, KEYWORD_FIRST_STRIKE) ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Pump\n Give First Strike\n Cancel", ai_choice);
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, MANACOST_RW(1, 1));
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_until_eot(instance->parent_controller, instance->parent_card,instance->parent_controller, instance->parent_card, 2, 0);
		}
		if( instance->info_slot == 67 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
	}

	return 0;
}

int card_unexpected_results(int player, int card, event_t event){

	/* Unexpected Results	|2|G|U
	 * Sorcery
	 * Shuffle your library, then reveal the top card. If it's a nonland card, you may cast it without paying its mana cost. If it's a land card, you may put it
	 * onto the battlefield and return ~ to its owner's hand. */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int *deck = deck_ptr[player];
			int bounced = 0;
			if( deck[0] != -1 ){
				shuffle(player);
				reveal_card_iid(player, card, deck[0]);
				if (is_what(-1, deck[0], TYPE_LAND)){
					int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
										"Put onto battlefield", 1, 3,
										"Decline", 1, 1);
					if (choice == 1){
						put_into_play_a_card_from_deck(player, player, 0);
						bounced = 1;
						bounce_permanent(player, card);
					}
				} else if (can_legally_play_iid(player, deck[0])){
					int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
										"Cast", 1, 3,
										"Decline", 1, 1);
					if (choice == 1){
						play_card_in_deck_for_free(player, player, 0);
					}
				}
			}
			if( !bounced ){
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_urban_evolution(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		int legacy = create_legacy_effect(player, card, &check_playable_lands_legacy);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[2].card = get_id(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_vizkopa_confessor(int player, int card, event_t event){

	extort(player, card, event);

	if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( comes_into_play_mode(player, card, event, can_pay_life(player, 1) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( would_validate_arbitrary_target(&td, 1-player, -1) ){
				int amount = hand_count[1-player];
				if( amount > life[player]-6 ){
					amount = life[player]-6;
				}
				if( player != AI ){
					amount = choose_a_number(player, "Pay how much life?", hand_count[1-player]);
					if( amount > life[player] ){
						amount = life[player];
					}
					if( amount < 0 ){
						amount = 0;
					}
				}
				lose_life(player, amount);
				ec_definition_t ec;
				default_ec_definition(1-player, player, &ec);
				ec.cards_to_reveal = amount;
				ec.effect = EC_RFG;

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_effect_coercion(&ec, &this_test);
			}
		}
	}
	return 0;
}

int card_vizkopa_guildmage(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XBW(1, 1, 1), 0, NULL, NULL) ){
			if( can_target(&td) ){
				return 1;
			}
			return player == HUMAN ? 1 : (instance->targets[1].player != 66 ? 1 : 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( instance->targets[1].player != 66 ){
			ai_modifier+=15;
		}
		int choice = 1;
		if( can_target(&td) ){
			int ai_choice = 1;
			if( instance->targets[1].player == 66 ){
				ai_choice = 0;
			}
			choice = do_dialog(player, player, card, -1, -1, " Give lifelink\n You gain, other loses\n Cancel", ai_choice);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if(	charge_mana_for_activated_ability(player, card, MANACOST_XBW(1, 1, 1)) ){
			instance->number_of_targets = 0;
			if( choice == 0 ){
				pick_target(&td, "TARGET_CREATURE");
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_LIFELINK);
		}
		if( instance->info_slot == 67 ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			parent->targets[1].player = 66;
			create_id_legacy(instance->parent_controller, instance->parent_card, -1, -1, 1);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_whispering_madness(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int amount = MAX(hand_count[player], hand_count[1-player]);
		APNAP(p,{
					new_discard_all(p, player);
					draw_cards(p, amount);
				};
		);
		cipher(player, card);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_wojek_halberdier(int player, int card, event_t event){
	if( battalion(player, card, event) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}
	return 0;
}

static int zameck_guildmage_legacy(int player, int card, event_t event){
	if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		enters_the_battlefield_with_counters(affected_card_controller, affected_card, event, COUNTER_P1_P1, 1);
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_zameck_guildmage(int player, int card, event_t event){

	/* Zameck Guildmage	|G|U
	 * Creature - Elf Wizard 2/2
	 * |G|U: This turn, each creature you control enters the battlefield with an additional +1/+1 counter on it.
	 * |G|U, Remove a +1/+1 counter from a creature you control: Draw a card. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_UG(1, 1), 0, NULL, NULL) ){
			if( can_target(&td) ){
				return 1;
			}
			int ai_global_evolution = count_subtype(player, TYPE_LAND, -1) > 5 && current_phase < PHASE_DECLARE_ATTACKERS && current_turn == player;
			return player == HUMAN ? 1 : ( ai_global_evolution ? 1 : 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int ai_global_evolution = count_subtype(player, TYPE_LAND, -1) > 5 && current_phase < PHASE_DECLARE_ATTACKERS && current_turn == player;
		if( ai_global_evolution ){
			ai_modifier+=15;
		}
		int choice = 0;
		if( can_target(&td) ){
			int ai_choice = ai_global_evolution ? 0 : 1;
			choice = do_dialog(player, player, card, -1, -1, " Global evolution\n Remove counter & draw\n Cancel", ai_choice);
		}

		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_UG(1, 1)) ){
			if( choice == 1 ){
				if( new_pick_target(&td, "Select a creature with a +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
					remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			create_legacy_effect(instance->parent_controller, instance->parent_card, &zameck_guildmage_legacy);
		}
		if( instance->info_slot == 67 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_zhur_taa_swine(int player, int card, event_t event){
	return bloodrush(player, card, event, 1, 0, 0, 1, 1, 0, 5, 4, 0, 0);
}

// Hybrid
int card_arrows_of_justice(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( hybrid_casting(player, card, 0) ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// beckon apparition --> already coded

int card_biomass_mutation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_pt_and_abilities_until_eot(player, card, player, -1, instance->info_slot, instance->info_slot, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bioshift(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE);
				td.allowed_controller = instance->targets[0].player;
				td.preferred_controller = instance->targets[0].player;
				new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td, 1) ){
			int amount = count_1_1_counters(instance->targets[0].player, instance->targets[0].card);
			if( amount > 0 ){
				if( player != AI ){
					int result = choose_a_number(player, "Move how many +1/+1 counters?", amount);
					if( result < amount && result > -1 ){
						amount = result;
					}
				}
				remove_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount);
				add_1_1_counters(instance->targets[1].player, instance->targets[1].card, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_spitemare(int player, int card, event_t event);
int card_boros_reckoner(int player, int card, event_t event){

	/* Boros Reckoner	|RW|RW|RW
	 * Creature - Minotaur Wizard 3/3
	 * Whenever ~ is dealt damage, it deals that much damage to target creature or player.
	 * |RW: ~ gains first strike until end of turn. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 1, 0);
		return has_mana_hybrid(player, 1, COLOR_RED, COLOR_WHITE, c1);
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 1, 0);
		charge_mana_hybrid(player, card, 1, COLOR_RED, COLOR_WHITE, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		int pp = instance->parent_controller, pc = instance->parent_card;
		pump_ability_until_eot(pp, pc, pp, pc, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}

	return card_spitemare(player, card, event);
}

int card_burning_tree_emissary(int player, int card, event_t event){
	hybrid(player, card, event);
	if( comes_into_play(player, card, event) ){
		produce_mana_multi(player, 0, 0, 0, 1, 1, 0);
	}
	return 0;
}

int card_coerced_confession(int player, int card, event_t event){

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( hybrid_casting(player, card, 0) ){
			pick_target(&td, "TARGET_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int count = 0;
			int amount = 0;
			while( count < 4 && deck[count] != -1 ){
					if( is_what(-1, deck[count], TYPE_CREATURE) ){
						amount++;
					}
					count++;
			}
			mill(instance->targets[0].player, count);
			draw_cards(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
}

int card_deathcult_rogue(int player, int card, event_t event){
	hybrid(player, card, event);
	if(event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card && ! has_subtype(affected_card_controller, affected_card, SUBTYPE_ROGUE) ){
			event_result = 1;
		}
	}
	return 0;
}

int card_gift_of_orzhova(int player, int card, event_t event){
	hybrid(player, card, event);
	return generic_aura(player, card, event, player, 1, 1, KEYWORD_FLYING, SP_KEYWORD_LIFELINK, 0, 0, 0);
}

int card_immortal_servitude(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, instance->info_slot, 0, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_merfolk_of_the_depths(int player, int card, event_t event){
	hybrid(player, card, event);
	return flash(player, card, event);
}

int card_nightveil_specter(int player, int card, event_t event)
{
  /* Nightveil Specter	|UB|UB|UB
   * Creature - Specter 2/3
   * Flying
   * Whenever ~ deals combat damage to a player, that player exiles the top card of his or her library.
   * You may play cards exiled with ~. */

	hybrid(player, card, event);
	return nightveil_specter_like_ability(player, card, event, NSLA_MUST_PAY_MANACOST_OF_EXILED_CARDS | NSLA_EXILE_ONLY_WITH_COMBAT_DAMAGE |
			NSLA_EXILE_ONLY_WHEN_DAMAGING_PLAYER, 1, get_id(player, card));
}

int card_pit_fight(int player, int card, event_t event){
	/* Pit Fight	|1|RG
	 * Instant
	 * Target creature you control fights another target creature. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				if( player == AI ){
					td.power_requirement = get_toughness(instance->targets[0].player, instance->targets[0].card)-1;
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rubblebelt_raiders(int player, int card, event_t event){

	hybrid(player, card, event);

	// Whenever ~ attacks, put a +1/+1 counter on it for each attacking creature you control.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		add_1_1_counters(player, card, count_attackers(player));
	}
	return 0;
}

int card_shattering_blow(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			instance->number_of_targets = 0;
			pick_target(&td, "TARGET_ARTIFACT");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Artifacts

int card_armored_transport(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE && current_turn == player && is_attacking(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0 && damage->damage_source_player == 1-player
			  ){
				int i;
				for(i=0; i<active_cards_count[1-player]; i++){
					if( in_play(1-player, i) && is_what(1-player, i, TYPE_CREATURE) && damage->damage_source_card == i ){
						card_instance_t *this= get_card_instance( 1-player, i );
						if( this->blocking == card ){
							damage->info_slot = 0;
							break;
						}
					}
				}
			}
		}
	}

	return 0;
}

int card_boros_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, MANACOST_RW(1, 1), 1, 1, KEYWORD_DOUBLE_STRIKE, 0, COLOR_TEST_WHITE | COLOR_TEST_RED, SUBTYPE_SOLDIER);
}

int card_dimir_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, MANACOST_BU(1, 1), 2, 2, 0, SP_KEYWORD_UNBLOCKABLE, COLOR_TEST_BLACK | COLOR_TEST_BLUE, SUBTYPE_HORROR);
}

int card_glaring_spotlight(int player, int card, event_t event)
{
  /* Glaring Spotlight	|1
   * Artifact
   * Creatures your opponents control with hexproof can be the targets of spells and abilities you control as though they didn't have hexproof.
   * |3, Sacrifice ~: Creatures you control gain hexproof until end of turn and are unblockable this turn. */

  if (event == EVENT_ABILITIES && affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && !is_humiliated(player, card))
	set_special_flags(affected_card_controller, affected_card, SF_HEXPROOF_OVERRIDE);

  if (event == EVENT_RESOLVE_ACTIVATION)
	pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_HEXPROOF|SP_KEYWORD_UNBLOCKABLE);

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_guardian_of_the_gateless(int player, int card, event_t event)
{
  creature_can_block_additional(player, card, event, 255);

  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  int num_blocking = count_creatures_this_is_blocking(player, card);
	  if (num_blocking > 0)
		pump_until_eot(player, card, player, card, num_blocking, num_blocking);
	}

  return 0;
}

int card_gruul_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, MANACOST_GR(1, 1), 3, 2, KEYWORD_TRAMPLE, 0, COLOR_TEST_GREEN | COLOR_TEST_RED, SUBTYPE_BEAST);
}

// illusionist's bracers --> impossible

// millennial gargoyle --> vanilla

int card_orzhov_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, MANACOST_BW(1, 1), 1, 4, 0, SP_KEYWORD_LIFELINK, COLOR_TEST_BLACK | COLOR_TEST_WHITE, SUBTYPE_THRULL);
}

int card_razortip_whip(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_player(1-player, 1, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(1), 0, &td, NULL);
}

int card_riot_gear(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 1, 2, 0, 0);
}

int card_simic_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, MANACOST_UG(1, 1), 2, 3, 0, SP_KEYWORD_HEXPROOF, COLOR_TEST_BLUE | COLOR_TEST_GREEN, SUBTYPE_CRAB);
}

int card_skyblinder_staff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) && event == EVENT_BLOCK_LEGALITY ){
		if( instance->targets[8].player == attacking_card_controller && instance->targets[8].card == attacking_card &&
			check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
		  ){
			event_result = 1;
		}
	}

	return vanilla_equipment(player, card, event, 3, 1, 0, 0, 0);
}

// Lands
// "gate" lands --> azorius guildgate
// thespian stage --> impossible

int card_sapphire_drake(int player, int card, event_t event){
	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && affected_card_controller == player ){
		if( count_1_1_counters(affected_card_controller, affected_card) > 0 ){
			event_result |= KEYWORD_FLYING;
		}
	}
	return 0;
}

int card_scatter_arc(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
	}
	return card_negate(player, card, event);
}
