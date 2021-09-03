#include "manalink.h"

// Functions

static int provoke_impl(int src_player, int src_card, int player, int card, event_t event)
{
  // Uses src_player/src_card as the source of the legacy card, but deliberately uses player/card to store targetting
  if (!is_humiliated(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.preferred_controller = 1-player;

	  card_instance_t* instance = get_card_instance(player, card);

	  char prompt[200];
	  if (ai_is_speculating == 1)
		prompt[0] = 0;
	  else
		{
		  load_text(0, "TARGET_CREATURE_OPPONENT_CONTROLS");
		  scnprintf(prompt, 200, "%s: Provoke: %s", get_card_name_by_instance(instance), text_lines[0]);
		}

	  instance->number_of_targets = 0;
	  if (pick_next_target_noload(&td, prompt))
		{
		  int leg = target_must_block_me(player, card, instance->targets[0].player, instance->targets[0].card, 1);
		  if (player != src_player || card != src_card)
			{
			  // Reseat the legacy's text and image to the source card's
			  card_instance_t* legacy = get_card_instance(player, leg);
			  legacy->display_pic_csv_id = get_id(src_player, src_card);
			  legacy->display_pic_num = get_card_image_number(legacy->display_pic_csv_id, src_player, src_card);
			}

		  untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;
}

static int provoke(int player, int card, event_t event)
{
  // When this attacks, you may have target creature defending player controls untap and block it if able.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	provoke_impl(player, card, player, card, event);

  return 0;
}

static void amplify(int player, int card, event_t event, int subtype, int amount){

	if( event == EVENT_RESOLVE_SPELL ){
		add_state(player, card, STATE_OUBLIETTED);
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, get_subtype_text("Select %a card.", subtype));
		this_test.subtype = subtype;
		add_1_1_counters(player, card, amount*reveal_cards_from_your_hand(player, card, &this_test));
		remove_state(player, card, STATE_OUBLIETTED);
	}
}

// Cards

int card_akroma_angel_of_wrath(int player, int card, event_t event)
{
  /* Akroma, Angel of Wrath	|5|W|W|W
   * Legendary Creature - Angel 6/6
   * Flying, first strike, vigilance, trample, haste, protection from |Sblack and from |Sred */

  check_legend_rule(player, card, event);

  vigilance(player, card, event);
  haste(player, card, event);

  return 0;
}

int card_akromas_devoted(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && has_creature_type(affected_card_controller, affected_card, SUBTYPE_CLERIC) && in_play(affected_card_controller, card) ){
		vigilance(affected_card_controller, affected_card, event);
	}

	return 0;
}

int card_aphetto_exterminator(int player, int card, event_t event){
	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
		}
	}
	return morph(player, card, event, MANACOST_XB(3, 1));
}

int card_bane_of_the_living(int player, int card, event_t event)
{
  // Morph |X|B|B
  // When ~ is turned face up, all creatures get -X/-X until end of turn.
  if (event == EVENT_UNMORPH)
	{
	  if (charge_mana_multi(player, MANACOST_XB(-1,2)))
		get_card_instance(player, card)->info_slot = x_value;

	  return 0;
	}

  if (event == EVENT_TURNED_FACE_UP)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  pump_subtype_until_eot(player, card, 2, -1, -instance->info_slot, -instance->info_slot, 0, 0);
	}

  return morph(player, card, event, MANACOST_XB(-1,2));
}

int card_beacon_of_destiny(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->damage_target_player = instance->parent_controller;
			target->damage_target_card = instance->parent_card;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target damage card that targets you.");
}

int card_blade_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 1, 0, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_brontotherium(int player, int card, event_t event){
	return provoke(player, card, event);
}

int card_brood_sliver(int player, int card, event_t event){
	/* Brood Sliver	|4|G
	 * Creature - Sliver 3/3
	 * Whenever a Sliver deals combat damage to a player, its controller may put a 1/1 colorless Sliver creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 ){
				if( has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_SLIVER) ){
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
						if( instance->info_slot < 0 ){
							instance->info_slot = 0;
						}
						instance->targets[instance->info_slot].player = damage->damage_source_player;
						instance->info_slot++;
					}
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && ! is_humiliated(player, card)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i].player != -1 ){
						token_generation_t token;
						default_token_definition(player, card, CARD_ID_SLIVER, &token);
						token.t_player = instance->targets[i].player;
						generate_token(&token);
					}
				}
				instance->info_slot = 0;
		}
	}
	return slivercycling(player, card, event);
}

int card_caller_of_the_claw(int player, int card, event_t event){

	if( player == AI && event == EVENT_CAN_CAST ){
		if( get_dead_count(player, TYPE_CREATURE | GDC_NONTOKEN) < 1 ){
			ai_modifier-=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAR, &token);
		token.qty = get_dead_count(player, TYPE_CREATURE | GDC_NONTOKEN);
		generate_token(&token);
	}

	return flash(player, card, event);
}

int card_canopy_crawler(int player, int card, event_t event){

	amplify(player, card, event, SUBTYPE_BEAST, 1);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = count_1_1_counters(instance->parent_controller, instance->parent_card);
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_chromeshell_crab(int player, int card, event_t event){
	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allowed_controller = 1-player;
		td1.preferred_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && can_target(&td1) ){
			if( select_target(player, card, &td, "Select a creature your control", &(instance->targets[0])) ){
				if( select_target(player, card, &td1, "Select a creature opponent controls", &(instance->targets[1])) ){
					exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
															instance->targets[1].player, instance->targets[1].card);
				}
			}
		}
	}
	return morph(player, card, event, MANACOST_XU(4, 1));
}

int card_clickslither(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
			! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								2, 2, KEYWORD_TRAMPLE, 0);
	}

	return 0;
}

int card_corpse_harvester(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Zombie card.");
		this_test.subtype = SUBTYPE_ZOMBIE;
		this_test.no_shuffle = 1;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);

		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_SWAMP));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE, MANACOST_XB(1, 1), 0, NULL, NULL);
}

int crypt_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.required_subtype = SUBTYPE_SLIVER;
		td2.required_state = TARGET_STATE_DESTROYED;
		if( player == AI ){
			td2.special = TARGET_SPECIAL_REGENERATION;
		}


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				if( can_regenerate(instance->targets[2].player, instance->targets[2].card) ){
					regenerate_target(instance->targets[2].player, instance->targets[2].card);
				}
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
												&td2, "Select target Sliver to regenerate.");
	}

	return 0;
}

int card_crypt_sliver(int player, int card, event_t event){
	/*
	  Crypt Sliver |1|B
	  Creature - Sliver 1/1
	  All Slivers have "{T}: Regenerate target Sliver."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &crypt_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &crypt_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_dark_supplicant(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( can_sacrifice_as_cost(player, 3, TYPE_PERMANENT, 0, SUBTYPE_CLERIC, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_PERMANENT, "Select a Cleric to sacrifice.");
			test.subtype = SUBTYPE_CLERIC;
			int sacced = 0;
			int sacs[3];
			while( sacced < 3 ){
					int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
					if (!sac){
						cancel = 1;
						break;
					}
					sacs[sacced] = BYTE3(sac);
					sacced++;
			}
			if( sacced == 3 ){
				tap_card(player, card);
			}
			int i;
			for(i=0; i<sacced; i++){
				state_untargettable(player, sacs[i], 0);
				if( sacced == 3 ){
					kill_card(player, sacs[i], KILL_SACRIFICE);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Scion of Darkness card.");
		this_test.id = CARD_ID_SCION_OF_DARKNESS;
		this_test.zone = TARGET_ZONE_HAND;

		int result = -1;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		}
		if( result == -1 && new_special_count_grave(player, &this_test) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		}
		if( result == -1 ){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	return 0;
}

int card_daru_sanctifier(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return morph(player, card, event, MANACOST_XW(1, 1));
}

int card_defender_of_the_order(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		pump_subtype_until_eot(player, card, player, -1, 0, 2, 0, 0);
	}

	return morph(player, card, event, MANACOST_W(2));
}

int card_deftblade_elite(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	provoke(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		maze_of_ith_effect(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(1, 1), 0, NULL, NULL);
}

int card_dreamborn_muse(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		mill(current_turn, hand_count[current_turn]);
	}

	return 0;
}

int card_drinker_of_sorrow(int player, int card, event_t event){

	cannot_block(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE) ){
		impose_sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_echo_tracer(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return morph(player, card, event, 2, 0, 1, 0, 0, 0);
}

int card_embalmed_brawler(int player, int card, event_t event){

	amplify(player, card, event, SUBTYPE_ZOMBIE, 1);

	// Whenever ~ attacks or blocks, you lose 1 life for each +1/+1 counter on it.
	if (declare_attackers_trigger(player, card, event, 0, player, card) ||
		(blocking(player, card, event) && !is_humiliated(player, card))
	   ){
		lose_life(player, count_1_1_counters(player, card));
	}

	return 0;
}

int card_essence_sliver(int player, int card, event_t event){

	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* source = get_card_instance(affected_card_controller, affected_card);

		if (source->internal_card_id == damage_card && source->info_slot > 0
			&& source->damage_source_player >= 0 && source->damage_source_card >= 0
			&& has_subtype(source->damage_source_player, source->damage_source_card, SUBTYPE_SLIVER)){
			gain_life(source->damage_source_player, source->info_slot);
		}
	}

	return 0;
}

int card_feral_throwback(int player, int card, event_t event){

	amplify(player, card, event, SUBTYPE_BEAST, 2);

	provoke(player, card, event);

	return 0;
}

int card_frenetic_raptor(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && has_subtype(affected_card_controller, affected_card, SUBTYPE_BEAST) && ! is_humiliated(player, card) ){
		event_result = 1;
	}

	return 0;
}

int card_gempalm_avenger(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		draw_a_card(player);
		pump_subtype_until_eot(player, card, 2, SUBTYPE_SOLDIER, 1, 1, KEYWORD_FIRST_STRIKE, 0);
	}
	else{
		return cycling(player, card, event, MANACOST_XW(2, 1));
	}
	return 0;
}

int card_gempalm_incinerator(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		draw_a_card(player);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_GOBLIN);
		if( amount > 0 && can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
		}
	}
	else{
		return cycling(player, card, event, MANACOST_XR(1, 1));
	}
	return 0;
}

int card_gempalm_polluter(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		draw_a_card(player);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		int amount = count_subtype(2, TYPE_PERMANENT, SUBTYPE_ZOMBIE);
		if( amount > 0 && can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			lose_life(instance->targets[0].player, amount);
		}
	}
	else{
		return cycling(player, card, event, MANACOST_B(2));
	}
	return 0;
}

int card_gempalm_sorcerer(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		draw_a_card(player);
		pump_subtype_until_eot(player, card, 2, SUBTYPE_WIZARD, 0, 0, KEYWORD_FLYING, 0);
	}
	else{
		return cycling(player, card, event, MANACOST_XU(2, 1));
	}
	return 0;
}

int card_gempalm_strider(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		draw_a_card(player);
		pump_subtype_until_eot(player, card, 2, SUBTYPE_ELF, 2, 2, 0, 0);
	}
	else{
		return cycling(player, card, event, MANACOST_XG(2, 2));
	}
	return 0;
}

int card_ghastly_remains(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	amplify(player, card, event, SUBTYPE_ZOMBIE, 1);

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP && has_mana(player, COLOR_BLACK, 3) ){
		int choice = do_dialog(player, player, card, -1, -1," Return Ghastly Remains to hand\n Pass\n", 0);
		if( choice == 0 ){
			charge_mana(player, COLOR_BLACK, 3);
			if( spell_fizzled != 1 ){
				instance->state &= ~STATE_INVISIBLE;
				hand_count[player]++;
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

// glowrider --> thorn_of_amethyst

int card_goblin_clearcutter(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		ai_modifier -= 36;
		if( sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) ){
			produce_mana_tapped_any_combination_of_colors(player, card, COLOR_TEST_RED|COLOR_TEST_GREEN, 3, NULL);
		}
		else{
			spell_fizzled = 1;
		}
	}

	return 0;
}

int card_goblin_goon(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) ){
			if( count_subtype(player, TYPE_CREATURE, -1) <= count_subtype(1-player, TYPE_CREATURE, -1) ){
				event_result = 1;
			}
		}

		if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) ){
			if( count_subtype(player, TYPE_CREATURE, -1) <= count_subtype(1-player, TYPE_CREATURE, -1) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

// goblin grappler -> brontotherium

int card_goblin_lookout(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0)
		  ){
			tap_card(player, card);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, SUBTYPE_GOBLIN, 2, 0, 0, 0);
	}

	return 0;
}

int card_graveborn_muse(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ZOMBIE);
		lose_life(player, count);
		draw_cards(player, count);
	}

	return 0;
}

int card_havok_demon(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_subtype_until_eot(player, card, ANYBODY, -1, -5, -5, 0, 0);
	}
	return 0;
}

int card_hollow_specter(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			ec_definition_t ec;
			default_ec_definition(1-player, player, &ec);
			ec.cards_to_reveal = x_value;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_effect_coercion(&ec, &this_test);
		}
	}
	return 0;
}

int card_hunter_sliver(int player, int card, event_t event, int trigger_mode)
{
  // All Sliver creatures have provoke. (When a Sliver attacks, its controller may have target creature defending player controls untap and block it if able.)
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_SLIVER;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_TRACK, 2, -1, &test)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			provoke_impl(player, card, current_turn, attackers[amt], event);
		}
	}

  return slivercycling(player, card, event);
}

int card_imperial_hellkite(int player, int card, event_t event){
	if( event == EVENT_TURNED_FACE_UP ){
		if( player == AI || do_dialog(player, player, card, -1, -1, " Tutor a Dragon\n Pass", 0) == 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, 0, "Select a Dragon card.");
			this_test.subtype = SUBTYPE_DRAGON;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}
	return morph(player, card, event, MANACOST_XR(6, 2));
}

int card_infernal_caretaker(int player, int card, event_t event){
	/* Infernal Caretaker	|3|B
	 * Creature - Human Cleric 2/2
	 * Morph |3|B
	 * When ~ is turned face up, return all Zombie cards from all graveyards to their owners' hands. */

	if( event == EVENT_TURNED_FACE_UP ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "");
		test.subtype = SUBTYPE_ZOMBIE;

		int i;
		for(i=0; i<2; i++){
			from_grave_to_hand_multiple(i ? current_turn : 1-current_turn, &test);
		}
	}
	return morph(player, card, event, MANACOST_XB(3, 1));
}

int card_keeper_of_the_nine_gales(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_BIRD;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;
	td.special = TARGET_SPECIAL_NOT_ME;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td2, NULL) ){
			return target_available(player, card, &td) > 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int total = 0;
		int bt[2];
		while( total < 2 ){
				instance->number_of_targets = 0;
				if( new_pick_target(&td, "Select another Bird to tap.", 0, GS_LITERAL_PROMPT) ){
					bt[total] = instance->targets[0].card;
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
					total++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<total; i++){
			state_untargettable(player, bt[i], 0);
		}
		if( total == 2 ){
			instance->number_of_targets = 0;
			if( pick_target(&td2, "TARGET_PERMANENT") ){
				for(i=0; i<total; i++){
					tap_card(player, bt[i]);
				}
				tap_card(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_kilnmouth_dragon(int player, int card, event_t event){
	/* Kilnmouth Dragon	|5|R|R
	 * Creature - Dragon 5/5
	 * Amplify 3
	 * Flying
	 * |T: ~ deals damage equal to the number of +1/+1 counters on it to target creature or player. */

	amplify(player, card, event, SUBTYPE_DRAGON, 3);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = count_1_1_counters(player, instance->parent_card);
			damage_creature_or_player(player, card, event, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_krosan_cloudscrapper(int player, int card, event_t event){
	basic_upkeep(player, card, event, MANACOST_G(2));
	return morph(player, card, event, MANACOST_XG(7, 2));
}

int card_lavaborn_muse(int player, int card, event_t event){

	if (hand_count[1-player] <= 2){
		upkeep_trigger_ability(player, card, event, 1 - player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if (hand_count[1-player] <= 2){	// has to be checked each time, stupidly enough, due to things like Farsight Mask
			damage_player(current_turn, 3, player, card);
		}
	}

	return 0;
}

int magma_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		td2.required_subtype = SUBTYPE_SLIVER;


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				int amount = count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_SLIVER);
				pump_until_eot(instance->targets[0].player, instance->targets[0].card, instance->targets[2].player, instance->targets[2].card, amount, 0);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
												&td2, "Select target Sliver to pump.");
	}

	return 0;
}

int card_magma_sliver(int player, int card, event_t event){
	/*
	  Magma Sliver |3|R
	  Creature - Sliver 3/3
	  All Slivers have "{T}: Target Sliver creature gets +X/+0 until end of turn, where X is the number of Slivers on the battlefield."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &magma_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &magma_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_mistform_ultimus(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	changeling_switcher(player, card, event);
	return 0;
}

int card_nantuko_vigilante(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_ARTIFACT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return morph(player, card, event, MANACOST_XG(1, 1));
}

int card_noxious_ghoul(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_PERMANENT, 0, SUBTYPE_ZOMBIE, 0, 0, 0, 0, 0, -1, 0) ){
		APNAP(p, {
					int count = active_cards_count[p]-1;
					while( count > -1 ){
							if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) && ! has_subtype(p, count, SUBTYPE_ZOMBIE) ){
								pump_until_eot(player, card, p, count, -1, -1);
							}
							count--;
					}
				};
		);
	}

	return 0;
}

int card_patron_of_the_wild(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
	}

	return morph(player, card, event, MANACOST_G(2));
}

int card_phage_the_untouchable(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	deathtouch(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( event == EVENT_RESOLVE_SPELL && instance->info_slot == 1 && ! is_humiliated(player, card) ){
		lose_the_game(player);
	}

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		lose_the_game(1-player);
	}

	return 0;
}

int card_planar_guide(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test, ACT_RFG_UNTIL_EOT);
	}

	return generic_activated_ability(player, card, event, GAA_RFG_ME, MANACOST_XW(3, 1), 0, NULL, NULL);
}

int card_plated_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 1, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_riptide_director(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_WIZARD));
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(2, 2), 0, NULL, NULL);
}

int card_riptide_mangler(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > 0 && event == EVENT_POWER && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=instance->targets[1].player;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI && instance->targets[1].player > 0 ){
		td.power_requirement = instance->targets[1].player | TARGET_PT_GREATER_OR_EQUAL;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->targets[1].player = get_power(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XU(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_root_sliver(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL ){
		cannot_be_countered(player, card, event);

		test_definition_t this_test;
		default_test_definition(&this_test, 0);
		this_test.subtype = SUBTYPE_SLIVER;

		type_uncounterable(player, card, event, player, 0, &this_test);
	}
	else{
		type_uncounterable(player, card, event, player, 0, NULL);
	}

	return 0;
}

int card_scion_of_darkness(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE) ){
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			new_global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return cycling(player, card, event, MANACOST_X(3));
}

int card_seedborn_muse(int player, int card, event_t event){
	untap_permanents_during_opponents_untap(player, card, TYPE_PERMANENT, &get_card_instance(player, card)->info_slot);
	return 0;
}

int card_shifting_sliver(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && has_subtype(attacking_card_controller, attacking_card, SUBTYPE_SLIVER) && ! is_humiliated(player, card) ){
		if( ! has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER) ){
			event_result = 1;
		}
	}

	return 0;
}

int card_skinthinner(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_color = COLOR_TEST_BLACK;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return morph(player, card, event, MANACOST_XB(3, 2));
}

static int sacrifice_at_eot(int player, int card, event_t event)
{
  if (eot_trigger(player, card, event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
	}

  return 0;
}
int card_skirk_alarmist(int player, int card, event_t event)
{
  /* Skirk Alarmist	|1|R
   * Creature - Human Wizard 1/2
   * Haste
   * |T: Turn target face-down creature you control face up. At the beginning of the next end step, sacrifice it. */

  haste(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;
  td.extra = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  create_targetted_legacy_effect(player, card, &sacrifice_at_eot, instance->targets[0].player, instance->targets[0].card);
	  flip_card(instance->targets[0].player, instance->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST0, 0, &td, "Select target face-down creature you control.");
}

int card_skirk_marauder(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, 2);
		}
	}

	return morph(player, card, event, MANACOST_XR(2, 1));
}

int card_skirk_outrider(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
			check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_BEAST)
		  ){
			event_result+=2;
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_BEAST)){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_spectral_sliver(int player, int card, event_t event){
	/*
	  Spectral Sliver |2|B
	  Creature - Sliver Spirit 2/2
	  All Sliver creatures have "{2}: This creature gets +1/+1 until end of turn."
	*/
	return sliver_with_shared_shade_ability(player, card, event, 1, 1, 0, 0);
}

int card_synapse_sliver(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( subtype_deals_damage(player, card, event, 2, SUBTYPE_SLIVER, DDBM_MUST_DAMAGE_PLAYER+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		draw_some_cards_if_you_want(player, card, current_turn, instance->targets[1].card);
		instance->targets[1].card = 0;
	}
	return slivercycling(player, card, event);
}

int card_timberwatch_elf(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_ELF);
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_totem_speaker(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), TYPE_PERMANENT, 0, SUBTYPE_BEAST, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 3);
	}

	return 0;
}

int card_toxin_sliver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	card_instance_t* damage = combat_damage_being_dealt(event);
	if (damage &&
		damage->damage_target_card != -1 &&
		has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_SLIVER) &&
		instance->info_slot < 40
	   ){
		unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
		creatures[2 * instance->info_slot] = damage->damage_target_player;
		creatures[2 * instance->info_slot + 1] = damage->damage_target_card;
		instance->info_slot++;
	}

	if(trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player && instance->info_slot > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
			int i;
			for (i = 0; i < instance->info_slot; ++i){
				if( in_play(creatures[2 * i], creatures[2 * i + 1]) ){
					kill_card(creatures[2 * i], creatures[2 * i + 1], KILL_BURY);
				}
			}
			instance->info_slot = 0;
		}
	}
	return slivercycling(player, card, event);
}

int card_tribal_forcemage(int player, int card, event_t event){
	if( event == EVENT_TURNED_FACE_UP ){
		int subt = select_a_subtype(player, card);
		pump_subtype_until_eot(player, card, 2, subt, 2, 2, KEYWORD_TRAMPLE, 0);
	}
	return morph(player, card, event, MANACOST_XG(1, 1));
}

int card_voidmage_apprentice(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  // Morph |2|U|U
  // When ~ is turned face up, counter target spell.
  if (event == EVENT_CAN_UNMORPH)
	{
	  if (!has_mana_multi(player, MANACOST_XU(2,2)))
		return 0;

	  int cs = instance->targets[9].card = counterspell(player, card, EVENT_CAN_CAST, NULL, -1);
	  return cs ? cs : 1;
	}

  int mf = morph(player, card, event, MANACOST_XU(2,2));

  if (event == EVENT_UNMORPH)
	{
	  if (cancel != 1 && card_on_stack_controller != -1 && instance->targets[9].card == 99)
		{
		  counterspell(player, card, EVENT_CAST_SPELL, NULL, 0);
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

	if (event == EVENT_TURNED_FACE_UP){
		if (instance->targets[9].card == 99 && counterspell_validate(player, card, NULL, 0)){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		cancel = 0;
	}

	return mf;
}

int card_wall_of_deceit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		turn_face_down(instance->parent_controller, instance->parent_card);
	}

	return morph(player, card, event, MANACOST_U(1));
}

// wall of hope --> wall of essence

int card_warbreak_trumpeter(int player, int card, event_t event)
{
  /* Warbreak Trumpeter	|R
   * Creature - Goblin 1/1
   * Morph |X|X|R
   * When ~ is turned face up, put X 1/1 |Sred Goblin creature tokens onto the battlefield. */

  if (event == EVENT_UNMORPH)
	{
	  charge_mana(player, COLOR_RED, 1);
	  if (cancel != 1)
		{
		  int result = charge_mana_for_double_x(player, COLOR_COLORLESS);
		  if (cancel != 1)
			get_card_instance(player, card)->info_slot = result / 2;
		}

	  return 0;
	}

  if (event == EVENT_TURNED_FACE_UP)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  generate_tokens_by_id(player, card, CARD_ID_GOBLIN, instance->info_slot);
	  instance->info_slot = 0;
	}

  return morph(player, card, event, MANACOST_R(1));
}

int card_ward_sliver(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = select_a_protection(player);
	}
	if( instance->info_slot > 0 ){
		boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, instance->info_slot, BCT_INCLUDE_SELF);
	}
	return slivercycling(player, card, event);
}

// windborn muse --> propaganda

int card_wirewood_channeler(int player, int card, event_t event){
	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_subtype(2, TYPE_PERMANENT, SUBTYPE_ELF);
	}
	return mana_producing_creature_all_one_color(player, card, event, 24, COLOR_TEST_ANY_COLORED, count);
}

int card_wirewood_hivemaster(int player, int card, event_t event){
	/* Wirewood Hivemaster	|1|G
	 * Creature - Elf 1/1
	 * Whenever another nontoken Elf enters the battlefield, you may put a 1/1 |Sgreen Insect creature token onto the battlefield. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.subtype = SUBTYPE_ELF;
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), &this_test) ){
			generate_token_by_id(player, card, CARD_ID_INSECT);
		}
	}

	return 0;
}

int card_withered_wretch(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) ){
			if( (count_graveyard(player) > 0 && ! graveyard_has_shroud(player)) || (count_graveyard(1-player) > 0 && ! graveyard_has_shroud(1-player)) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);

			select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &this_test, 0, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
		}
	}

	return 0;
}


