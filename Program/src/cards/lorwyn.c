#include "manalink.h"

// Functions
int clash(int player, int card){
	int cmc[2];
	APNAP(p, {
				char msg[100];
				if( deck_ptr[p][0] != -1 ){
					if( p == AI ){
						strcpy(msg, "Here's the first card of opponent's deck");
					}
					else{
						strcpy(msg, "Here's the first card of your deck");
					}
					look_at_iid(player, card, deck_ptr[p][0], msg);
					cmc[p] = get_cmc_by_internal_id(deck_ptr[p][0]);
				};
			};
	);
	int result = 0;
	if( cmc[player] > cmc[1-player] ){
		result = 1;
	}

	scrylike_effect(player, player, 1);
	scrylike_effect(1-player, 1-player, 1);

	int count = active_cards_count[player]-1;
	while( count > -1 ){
			if( in_play(player, count) && ! is_humiliated(player, count) ){
				if( get_id(player, count) == CARD_ID_REBELLION_OF_THE_FLAMEKIN && has_mana(player, COLOR_COLORLESS, 1) ){
					int choice = do_dialog(player, player, card, -1, -1, " Activate Rebellion of the Flamekin\n Pass", 0);
					if( choice == 0 ){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 ){
							int s_keyword = 0;
							if( result == 1 ){
								s_keyword = SP_KEYWORD_HASTE;
							}
							token_generation_t token;
							default_token_definition(player, card, CARD_ID_ELEMENTAL_SHAMAN, &token);
							token.pow = 3;
							token.tou = 1;
							token.s_key_plus = s_keyword;
							generate_token(&token);
						}
					}
				}
				if( get_id(player, count) == CARD_ID_SYLVAN_ECHOES && result == 1 ){
					int choice = do_dialog(player, player, card, -1, -1, " Draw a card\n Pass", 0);
					if( choice == 0 ){
						draw_cards(player, 1);
					}
				}
			}
			count--;
	}
	return result;
}

int evoke(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_MODIFY_COST ){
		int c1 = get_updated_casting_cost(player, card, -1, event, colorless);
		if( has_mana_multi( player, c1, black, blue, green, red, white) ){
			null_casting_cost(player, card);
			instance->targets[1].player = 1;
		}
		else{
			instance->targets[1].player = 0;
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->targets[1].player == 1){
			int choice = 1;
			if( has_mana_to_cast_id(player, event, get_id(player, card) ) ){
				choice = do_dialog(player, player, card, -1, -1, " Play normally\n Evoke\n Cancel", 0);
			}
			if( choice == 0 ){
				charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else if( choice == 1 ){
					int c1 = get_updated_casting_cost(player, card, -1, event, colorless);
					charge_mana_multi( player, c1, black, blue, green, red, white);
					if( spell_fizzled != 1 ){
						instance->info_slot = 1;
					}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

int evoked(int player, int card){
	if( ! is_humiliated(player, card) ){
		return get_card_instance(player, card)->info_slot;
	}
	return 0;
}

void lorwyn_need_subtype_land(int player, int card, event_t event, subtype_t subtype){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && !is_tapped(player, card) ){	// e.g. from Kismet
		int tapme = 1;
		if( is_subtype_in_hand(player, subtype) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, 0, get_subtype_text("Select %a card to reveal.", subtype));
			this_test.subtype = subtype;
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if (selected != -1){
				reveal_card(player, card, player, selected);
				tapme = 0;
			}
		}
		if( tapme ){
			get_card_instance(player, card)->state |= STATE_TAPPED;
		}
	}
}

static int harbinger(int player, int card, event_t event, subtype_t subtype){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, get_subtype_text("Select %a card.", subtype));
		this_test.subtype = subtype;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int champion(int player, int card, event_t event, int subtype, int subtype2){

	card_instance_t *instance = get_card_instance(player, card);

	return_from_oblivion(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			int good = 0;
			if( subtype == -1 && subtype2 == -1 && check_battlefield_for_subtype(player, TYPE_CREATURE, -1) ){
				good = 1;
			}
			if( subtype != -1 && check_battlefield_for_subtype(player, TYPE_PERMANENT, subtype) ){
				good = 1;
			}
			if( subtype2 != -1 && check_battlefield_for_subtype(player, TYPE_PERMANENT, subtype2) ){
				good = 1;
			}
			if( good == 0 ){
				ai_modifier-=1000;
			}
		}
	}

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		if( subtype == -1 && subtype2 == -1 ){
			td.required_type = TYPE_CREATURE;
		}

		state_untargettable(player, card, 1);

		if( can_target(&td) ){
			int trg = -1;
			if( player == AI && (subtype != -1 || subtype2 != -1) ){
				int i;
				for(i=0; i<active_cards_count[player]; i++){
					if( in_play(player, i) && is_what(player, i, TYPE_PERMANENT) ){
						int nt = 0;
						if( subtype != -1 && has_subtype(player, i, subtype) ){
							nt++;
						}
						if( subtype2 != -1 && has_subtype(player, i, subtype2) ){
							nt++;
						}
						if( nt == 0 ){
							add_state(player, i, STATE_CANNOT_TARGET);
						}
					}
				}
			}
			if( subtype == -1 && subtype2 == -1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					trg = instance->targets[0].card;
				}
			}
			else{
				char buffer[100];
				int pos = scnprintf(buffer, 100, "Select a ");
				if( subtype > -1 ){
					pos += scnprintf(buffer+pos, 100-pos, get_subtype_text("%s", subtype));
				}
				if( subtype > -1 && subtype2 > -1){
					pos += scnprintf(buffer+pos, 100-pos, " or ");
				}
				if( subtype2 > -1 ){
					pos += scnprintf(buffer+pos, 100-pos, get_subtype_text("%s", subtype2));
				}
				scnprintf(buffer+pos, 100-pos, " permanent to exile.");
				if( new_pick_target(&td, buffer, 0, GS_LITERAL_PROMPT) ){
					instance->number_of_targets = 1;
					int good = 0;
					if( subtype != -1 && has_subtype(player, instance->targets[0].card, subtype) ){
						good = 1;
					}
					if( subtype2 != -1 && has_subtype(player, instance->targets[0].card, subtype2) ){
						good = 1;
					}
					if( good == 1 ){
						trg = instance->targets[0].card;
					}
				}
			}
			if( player == AI && (subtype != -1 || subtype2 != -1) ){
				int i;
				for(i=0; i<active_cards_count[player]; i++){
					if( in_play(player, i) && is_what(player, i, TYPE_PERMANENT) ){
						remove_state(player, i, STATE_CANNOT_TARGET);
					}
				}
			}
			if( trg != -1 ){
				state_untargettable(player, card, 0);
				obliviation(player, card, player, trg);
				if( get_id(player, card) == CARD_ID_MISTBIND_CLIQUE ){
					target_definition_t td1;
					default_target_definition(player, card, &td1, TYPE_CREATURE );
					td1.zone = TARGET_ZONE_PLAYERS;
					td1.allow_cancel = 0;
					if( pick_target(&td1, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
						test_definition_t this_test;
						default_test_definition(&this_test, TYPE_LAND);
						new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_TAP);
					}
				}
				instance->number_of_targets = 0;
			}
			else{
				state_untargettable(player, card, 0);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		else{
			state_untargettable(player, card, 0);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

static int boost_if_subtype_is_present(int player, int card, event_t event, subtype_t subtype, int pow, int tou, int key, int s_key){
	if( affect_me(player, card) && ! is_humiliated(player, card) ){
		if( check_battlefield_for_subtype(player, TYPE_PERMANENT, subtype) ){
			modify_pt_and_abilities(player, card, event, pow, tou, key);
			special_abilities(player, card, event, s_key, player, card);
		}
	}
	return 0;
}

static int hideaway(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	/* This land enters the battlefield tapped. When it does, look at the top four cards of your library, exile one face down, then put the rest on the bottom
	 * of your library. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		int amount = 4;
		if( amount > count_deck(player) ){
			amount = count_deck(player) ;
		}
		if( amount > 0 ){
			card_ptr_t* c = cards_ptr[get_id(player, card)];
			test_definition_t this_test;
			new_default_test_definition(&this_test, 0, "");
			scnprintf(this_test.message, 100, " Select a card to exile with %s.", c->name);
			this_test.create_minideck = amount;
			this_test.no_shuffle = 1;
			int id = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 1, AI_MAX_CMC, &this_test);
			if( id > -1 ){
				instance->targets[2].player = id;
				if( !IS_AI(player) ){
					instance->targets[2].card = create_card_name_legacy(player, card, id);
				}
			}
			amount--;
			if( amount > 0 ){
				put_top_x_on_bottom(player, player, amount);
			}
		}
	}

	if (event == EVENT_ACTIVATE){
		return 0;	// must be handled by callers
	}

	if( event == EVENT_RESOLVE_ACTIVATION && instance->targets[1].player == 1 ){
		int id = instance->targets[2].player;
		card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
		parent->targets[2].player = -1;
		if( check_rfg(player, id) ){
			play_card_in_exile_for_free(player, player, id);
		}
		if( instance->targets[2].card != -1 && player != AI ){
			kill_card(player, instance->targets[2].card, KILL_REMOVE);
		}
		parent->targets[2].card = -1;
		parent->targets[1].player = 0;

		return 0;
	}

	return mana_producer(player, card, event);
}

static int vivid_land(int player, int card, event_t event, int color, color_test_t test_color)
{
  /* Vivid [Land]	""
   * Land
   * ~ enters the battlefield tapped with two charge counters on it.
   * |T: Add |C to your mana pool.
   * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */

  comes_into_play_tapped(player, card, event);

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 2);

  if (event == EVENT_CHANGE_TYPE)
	get_card_instance(player, card)->info_slot = count_counters(player, card, COUNTER_CHARGE) > 0 ? COLOR_TEST_ANY_COLORED : test_color;

  if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect(player, card);	// An argument could be made for play_land_sound_effect_force_color(player, card, COLOR_TEST_ANY_COLORED) instead

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  if (produce_mana_tapped_all_one_color_with_default(player, card, get_card_instance(player, card)->info_slot, 1, test_color)
		  && chosen_colors && !(chosen_colors == test_color))
		remove_counter(player, card, COUNTER_CHARGE);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card))
	declare_mana_available_maybe_hex(player, get_card_instance(player, card)->info_slot, 1);

  return 0;
}

static void get_type_text(int type, test_definition_t *test){
	char buffer[100];
	int pos = 0;
	if( type == TYPE_PERMANENT ){
		scnprintf(buffer, 100, "permanent ");
		strcpy(buffer, test->message);
		return;
	}
	if( type & TYPE_ARTIFACT ){
		pos+=scnprintf(buffer+pos, 100-pos, "artifact ");
	}
	if( type & TYPE_CREATURE ){
		pos+=scnprintf(buffer+pos, 100-pos, "creature ");
	}
	if( type & TYPE_ENCHANTMENT){
		pos+=scnprintf(buffer+pos, 100-pos, "enchantment ");
	}
	if( type & TYPE_LAND ){
		pos+=scnprintf(buffer+pos, 100-pos, "land ");
	}
	if( type & (TYPE_INSTANT | TYPE_INTERRUPT) ){
		pos+=scnprintf(buffer+pos, 100-pos, "instant ");
	}
	if( type & TYPE_SORCERY ){
		pos+=scnprintf(buffer+pos, 100-pos, "sorcery ");
	}
	if( type & TARGET_TYPE_PLANESWALKER ){
		pos+=scnprintf(buffer+pos, 100-pos, "planeswalker ");
	}
	strcpy(test->message, buffer);
}

static void get_color_text(int clr, test_definition_t *test){
	if( clr == COLOR_TEST_COLORLESS ){
		char buffer[100];
		scnprintf(buffer, 100, "colorless");
		strcpy(buffer, test->message);
		return;
	}
	load_text(0, "COLORWORDS");
	int i;
	for(i=1; i<6; i++){
		if( clr & (1<<i) ){
			strcpy(test->message, text_lines[i-1]);
			break;
		}
	}
}

int tapsubtype_ability(int player, int card, int tap_req, target_definition_t *td){
	card_instance_t *instance = get_card_instance(player, card);
	instance->number_of_targets = 0;
	int total = 0;
	test_definition_t test;
	get_type_text(td->required_type, &test);
	test_definition_t test2;
	get_color_text(get_sleighted_color_test(player, card, td->required_color), &test2);
	while( total < tap_req && can_target(td) ){
			char buffer[500];
			scnprintf(buffer, 500, "Select a %s %s %sto tap (%d of %d)",
						td->required_color ? test2.message : "",
						td->required_subtype > -1 ? raw_get_subtype_text(td->required_subtype) : "",
						test.message,
						total+1,
						tap_req);
			if( new_pick_target(td, buffer, total, GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[total].player, instance->targets[total].card, 1);
				total++;
			}
			else{
				break;
			}
	}
	int i;
	for(i=0; i<total; i++){
		state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		if( total == tap_req ){
			tap_card(instance->targets[i].player, instance->targets[i].card);
		}
	}
	if( total == tap_req ){
		return 1;
	}
	return 0;
}

static int buddy_creature(int player, int card, event_t event, subtype_t type){

	if( event == EVENT_MODIFY_COST ){
		if( count_subtype_in_hand(player, type) < 2  ){
			COST_COLORLESS += 3;
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (played_for_free(player, card)){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ANY);
		td.required_subtype = type;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.zone = TARGET_ZONE_HAND;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;
		td.special = TARGET_SPECIAL_NOT_ME;

		int choice = 0;
		if (can_target(&td)){
			if( has_mana(player, COLOR_COLORLESS, 3) ){
				choice = do_dialog(player, player, card, -1, -1, get_subtype_text(" Reveal %a card\n Pay 3\n Cancel", type), 0);
			}
		}
		else{
			return 0;	// already charged
		}

		if( choice == 0 ){
			card_instance_t *instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			if (pick_next_target_noload(&td, get_subtype_text("Choose %a card to reveal.", type))){
				instance->number_of_targets = 0;
				reveal_card(player, card, player, instance->targets[0].card);
			}
		}
		else if( choice == 1 ){
				charge_mana(player, COLOR_COLORLESS, 3);
		}
		else{
			spell_fizzled = 1;
		}
	}
	return 0;
}

// Cards

int card_adder_staff_boggart(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && clash(player, card) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_aethersnipe(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, MANACOST_XU(1, 2));
}

int card_avatar2(int player, int card, event_t event){
	if (player == -1){
		return 0;
	}
	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, life[player], life[player], 0);
		return 0;
	}
	return generic_token(player, card, event);
}

int card_ajani_goldmane(int player, int card, event_t event){

	/* Ajani Goldmane	|2|W|W
	 * Planeswalker - Ajani (4)
	 * +1: You gain 2 life.
	 * -1: Put a +1/+1 counter on each creature you control. Those creatures gain vigilance until end of turn.
	 * -6: Put a |Swhite Avatar creature token onto the battlefield. It has "This creature's power and toughness are each equal to your life total." */

	card_instance_t *instance = get_card_instance(player, card);

	if (IS_ACTIVATING(event)){

		enum{
			CHOICE_GAIN_LIFE = 1,
			CHOICE_PUMP_CRITS,
			CHOICE_AVATAR
		}
		choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						"Gain 2 life", 1, life[player] < 6 ? 20 : 5, 1,
						"Give a +1/+1 counters to your crits", 1, count_subtype(player, TYPE_CREATURE, -1)*3, -1,
						"Generate an Avatar", 1, life[player]*2, -6);

		if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
		else if (event == EVENT_RESOLVE_ACTIVATION){
			switch (choice)
			{
				case CHOICE_GAIN_LIFE:
					gain_life(player, 2);
					break;

					case CHOICE_PUMP_CRITS:
					{
						int count = active_cards_count[player]-1;
						while( count > -1 ){
								if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
									add_counters(player, count, COUNTER_P1_P1, 1);
									pump_ability_until_eot(instance->parent_controller, instance->parent_card, player, count, 0, 0, 0, SP_KEYWORD_VIGILANCE);
								}
								count--;
						}
					}
					break;

					case CHOICE_AVATAR:
					{
						token_generation_t token;
						default_token_definition(player, card, CARD_ID_AVATAR, &token);
						token.special_infos = 66;
						generate_token(&token);
					}
					break;
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_amoeboid_changeling(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE") ){
			int choice = do_dialog(player, player, card, -1, -1, " Give all creature types\n Remove all creature types\n Cancel", 1);
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( instance->info_slot == 66 ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										0, 0, KEYWORD_PROT_INTERRUPTS, 0);
			}
			if( instance->info_slot == 67 ){
				force_a_subtype_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											SUBTYPE_NONE);
			}
		}
	}

	return 0;
}

int card_ancient_amphitheater(int player, int card, event_t event){

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_GIANT);

	return mana_producer(player, card, event);
}

static int effect_aquitects_will(int player, int card, event_t event)
{
  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->damage_target_player, c = instance->damage_target_card;
	  if (p < 0 || c < 0 || !in_play(p, c))
		kill_card(player, card, KILL_REMOVE);
	  else if (!count_counters(p, c, COUNTER_FLOOD))
		{
		  kill_card(player, card, KILL_REMOVE);
		  reset_subtypes(p, c, 2);
		  recalculate_all_cards_in_play();	// since this is an effect (kill_card_guts() calls this for permanents)
		}
	}

  return 0;
}

int card_aquitects_will(int player, int card, event_t event){

	/* Aquitect's Will	|U
	 * Tribal Sorcery - Merfolk
	 * Put a flood counter on target land. That land is |Han Island in addition to its other types for as long as it has a flood counter on it. If you control a
	 * Merfolk, draw a card. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FLOOD);
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, get_hacked_subtype(player, card, SUBTYPE_ISLAND));
			create_targetted_legacy_effect(player, card, &effect_aquitects_will, instance->targets[0].player, instance->targets[0].card);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_MERFOLK) ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_arbiter_of_knollridge(int player, int card, event_t event){
	vigilance(player, card, event);
	if( comes_into_play(player, card, event) ){
		int max = life[player];
		if( life[1-player] > max ){
			max = life[1-player];
		}
		set_life_total(player, max);
		set_life_total(1-player, max);
	}
	return 0;
}

int card_ashling_the_pilgrim(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(player, instance->parent_card);
		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
		if( parent->targets[1].player < 0 ){
			parent->targets[1].player = 0;
		}
		parent->targets[1].player++;
		if( parent->targets[1].player == 3 ){
			int amount = count_1_1_counters(instance->parent_controller, instance->parent_card);
			remove_1_1_counters(instance->parent_controller, instance->parent_card, amount);
			new_damage_all(instance->parent_controller, instance->parent_card, ANYBODY, amount, NDA_PLAYER_TOO | NDA_ALL_CREATURES, NULL);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XR(1, 1), 0, NULL, NULL);
}

int card_ashlings_prerogative(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 1-do_dialog(player, player, card, -1, -1, " Odd\n Even", internal_rand(1));
	}

	if( ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			int cmc = get_cmc(affected_card_controller, affected_card);
			int odd = (cmc%2) > 0;
			if( event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE ){
				if( odd != instance->info_slot ){
					permanents_enters_battlefield_tapped(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
				}
			}
			if( event == EVENT_ABILITIES ){
				if( odd == instance->info_slot ){
					give_haste(affected_card_controller, affected_card);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_aunties_hovel(int player, int card, event_t event){

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_GOBLIN);

	return mana_producer(player, card, event);
}

/* If exactly one priority is > 0, increase them all a bit so the AI will be willing to cast the spell if he can either choose a neutral category, or one whose
 * disadvantage is far outweighed by the good choice's.  Lower ai_modifier if that happens, though. */
void raise_command_singleton_ai_priorities(event_t event, int* priority){
	if (event != EVENT_RESOLVE_SPELL){
		if ((priority[1] > 0 ? 1 : 0) + (priority[2] > 0 ? 1 : 0) + (priority[3] > 0 ? 1 : 0) + (priority[4] > 0 ? 1 : 0) == 1){
			if (event == EVENT_CAST_SPELL){
				ai_modifier -= 48;
			}
			int delta;
			if (priority[1] > 0){
				delta = priority[1] / 3;
				priority[1] *= 100;
				priority[2] += delta;
				priority[3] += delta;
				priority[4] += delta;
			} else if (priority[2] > 0){
				delta = priority[2] / 3;
				priority[1] += delta;
				priority[2] *= 100;
				priority[3] += delta;
				priority[4] += delta;
			} else if (priority[3] > 0){
				delta = priority[3] / 3;
				priority[1] += delta;
				priority[2] += delta;
				priority[3] *= 100;
				priority[4] += delta;
			} else { // priority[4] > 0
				delta = priority[4] / 3;
				priority[1] += delta;
				priority[2] += delta;
				priority[3] += delta;
				priority[4] *= 100;
			}
		}
	}
}

/* Lots of extra code could be added here to help the AI decide what to pick */
int card_austere_command(int player, int card, event_t event){

	if (event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL){
		if (event == EVENT_CAN_CAST && player == HUMAN){
			return 1;	// Always castable, though we may force the AI not to
		}

		typedef enum{
			CHOICE_ARTIFACTS = 1,
			CHOICE_ENCHANTMENTS = 2,
			CHOICE_SMALL_CREATURES = 3,
			CHOICE_LARGE_CREATURES = 4
		} CommandChoices;

		int priority[5];
		if (player == HUMAN || event == EVENT_RESOLVE_SPELL){
			priority[CHOICE_ARTIFACTS] = priority[CHOICE_ENCHANTMENTS] = priority[CHOICE_SMALL_CREATURES] = priority[CHOICE_LARGE_CREATURES] = 1;
		} else {
			priority[CHOICE_ARTIFACTS] = 10 * (count_subtype(1-player, TYPE_ARTIFACT, -1) - count_subtype(player, TYPE_ARTIFACT, -1));
			priority[CHOICE_ENCHANTMENTS] = 10 * (count_subtype(1-player, TYPE_ENCHANTMENT, -1) - count_subtype(player, TYPE_ENCHANTMENT, -1));

			test_definition_t test_small_creatures;
			default_test_definition(&test_small_creatures, TYPE_CREATURE);
			test_small_creatures.cmc = 4;
			test_small_creatures.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
			priority[CHOICE_SMALL_CREATURES] = 20 * (new_manipulate_all(player, card, 1-player, &test_small_creatures, ACT_GET_COUNT)
													 - new_manipulate_all(player, card, player, &test_small_creatures, ACT_GET_COUNT));

			test_definition_t test_large_creatures;
			default_test_definition(&test_large_creatures, TYPE_CREATURE);
			test_large_creatures.cmc = 3;
			test_large_creatures.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
			priority[CHOICE_LARGE_CREATURES] = 30 * (new_manipulate_all(player, card, 1-player, &test_large_creatures, ACT_GET_COUNT)
													 - new_manipulate_all(player, card, player, &test_large_creatures, ACT_GET_COUNT));

			raise_command_singleton_ai_priorities(event, priority);
		}

		CommandChoices choices[2];
		choices[0] = DIALOG(player, card, event,
							DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
							"Destroy all artifacts",		1, priority[CHOICE_ARTIFACTS],
							"Destroy all enchantments",		1, priority[CHOICE_ENCHANTMENTS],
							"Destroy creatures CMC <= 3",	1, priority[CHOICE_SMALL_CREATURES],
							"Destroy creatures CMC >= 4",	1, priority[CHOICE_LARGE_CREATURES]);

		// Nothing else to do for EVENT_CAST_SPELL
		if (event == EVENT_CAN_CAST){
			return choices[0] > 0;
		} else if (event == EVENT_RESOLVE_SPELL){
			int choice, p, c;
			for (choice = 0; choice < 2; ++choice){
				for (p = 0; p < 2; ++p){
					for (c = active_cards_count[p] - 1; c >= 0; --c){
						if (in_play(p, c)){
							int kill = 0;
							switch (choices[choice]){
								case CHOICE_ARTIFACTS:			kill = is_what(p, c, TYPE_ARTIFACT);	break;
								case CHOICE_ENCHANTMENTS:		kill = is_what(p, c, TYPE_ENCHANTMENT);	break;
								case CHOICE_SMALL_CREATURES:	kill = is_what(p, c, TYPE_CREATURE) && get_cmc(p, c) <= 3;	break;
								case CHOICE_LARGE_CREATURES:	kill = is_what(p, c, TYPE_CREATURE) && get_cmc(p, c) >= 4;	break;
							}
							if (kill){
								kill_card(p, c, KILL_DESTROY);
							}
						}
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

// avian changeling --> vanilla

int card_battle_mastery(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_DOUBLE_STRIKE, 0, 0, 0, 0);
}

int card_battlewand_oak(int player, int card, event_t event){

	if( specific_cip(player, card, event, player, ANYBODY, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}

	if( specific_spell_played(player, card, event, player, ANYBODY, TYPE_ANY, 0, SUBTYPE_TREEFOLK, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}

	return 0;
}

int card_blades_of_velis_vel(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( instance->targets[i].player > -1 && validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 2, 0, KEYWORD_PROT_INTERRUPTS, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_blind_spot_giant(int player, int card, event_t event){

	if( (event == EVENT_ATTACK_LEGALITY || event == EVENT_BLOCK_LEGALITY) && affect_me(player, card) && ! is_humiliated(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_GIANT;
		this_test.not_me = 1;

		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			event_result++;
		}
	}

	return 0;
}



int card_boggart_birth_rite(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select target Goblin card.");
	this_test.subtype = SUBTYPE_GOBLIN;

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_boggart_harbinger(int player, int card, event_t event){
	return harbinger(player, card, event, SUBTYPE_GOBLIN);
}

// goblin rogue --> vanilla

int card_boggart_mob(int player, int card, event_t event){
	/* Boggart Mob	|3|B
	 * Creature - Goblin Warrior 5/5
	 * Champion a Goblin
	 * Whenever a Goblin you control deals combat damage to a player, you may put a 1/1 |Sblack Goblin Rogue creature token onto the battlefield. */

	if( subtype_deals_damage(player, card, event, player, SUBTYPE_GOBLIN, DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN_ROGUE, get_card_instance(player, card)->targets[1].card);
		get_card_instance(player, card)->targets[1].card = 0;
	}

	return champion(player, card, event, SUBTYPE_GOBLIN, -1);
}

int card_boggart_shenanigans(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_GOBLIN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		if(	resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				damage_player(instance->targets[0].player, instance->targets[11].card, player, card);
			}
			instance->targets[11].card = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_boggart_sprite_chaser(int player, int card, event_t event){
	return boost_if_subtype_is_present(player, card, event, SUBTYPE_FAERIE, 1, 1, KEYWORD_FLYING, 0);
}

int card_briarhorn(int player, int card, event_t event){

	evoke(player, card, event, MANACOST_XG(1, 1));

	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return flash(player, card, event);
}

int card_brigid_hero_of_kinsbaile(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_BLOCKING | STATE_ATTACKING;
			new_damage_all(instance->parent_controller, instance->parent_card, instance->targets[0].player, 2, 0, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_brion_stoutarm(int player, int card, event_t event){
	/*
	  Brion Stoutarm |2|R|W
	  Legendary Creature - Giant Warrior 4/4
	  Lifelink
	  {R}, {T}, Sacrifice a creature other than Brion Stoutarm: Brion Stoutarm deals damage equal to the sacrificed creature's power to target player.
	*/
	check_legend_rule(player, card, event);

	lifelink(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET,
										MANACOST_R(1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_R(1)) ){
			state_untargettable(player, card, 1);

			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			state_untargettable(player, card, 0);
			if (!sac){
				cancel = 1;
				return 0;
			}
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, instance->info_slot, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_broken_ambitions(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if(counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot) && clash(player, card)){
			mill(instance->targets[0].player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_COUNTERSPELL, NULL, NULL, 1, NULL);
}

int card_caterwauling_boggart(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS && current_turn == player){
		if( has_subtype(attacking_card_controller, attacking_card, SUBTYPE_ELEMENTAL) ||
			has_subtype(attacking_card_controller, attacking_card, SUBTYPE_GOBLIN)
		  ){
			// If there are fewer than 3 creatures blocking me, then no one is
			// blocking me
			int block_count = 0;
			int count = 0;
			while(count < active_cards_count[1-player]){
				if(in_play(1-player, count) ){
					card_instance_t *instance = get_card_instance( 1-player, count);
					if( instance->blocking == attacking_card ){
						block_count++;
					}
				}
				count++;
			}

			if( block_count < 2 ){
				count = 0;
				while(count < active_cards_count[1-player]){
					if(in_play(1-player, count) ){
						card_instance_t *instance = get_card_instance( 1-player, count);
						if( instance->blocking == attacking_card ){
							instance->blocking = 255;
						}
					}
					count++;
				}
			}
		}
	}
	return 0;
}

int card_cenns_heir(int player, int card, event_t event){
	// Whenever ~ attacks, it gets +1/+1 until end of turn for each other attacking Kithkin.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_KITHKIN;
		this_test.state = STATE_ATTACKING;
		int amount = check_battlefield_for_special_card(player, card, current_turn, CBFSC_GET_COUNT+CBFSC_EXCLUDE_ME, &this_test);
		if (amount > 0){
			pump_until_eot(player, card, player, card, amount, amount);
		}
	}
	return 0;
}

int card_cenns_tactician(int player, int card, event_t event){

	if (affected_card_controller == player && count_1_1_counters(affected_card_controller, affected_card) > 1){
		arbitrary_can_block_additional(event, 1);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_SOLDIER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, MANACOST_W(1), 0, &td, "TARGET_CREATURE");
}

int card_chandra_nalaar(int player, int card, event_t event){

	/* Chandra Nalaar	|3|R|R
	 * Planeswalker - Chandra (6)
	 * +1: ~ deals 1 damage to target player.
	 * -X: ~ deals X damage to target creature.
	 * -8: ~ deals 10 damage to target player and each creature he or she controls. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td1;
		default_target_definition(player, card, &td1, 0);
		td1.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		if( player == AI ){
			td2.toughness_requirement = count_counters(player, card, COUNTER_LOYALTY) | TARGET_PT_LESSER_OR_EQUAL;
		}

		enum{
			CHOICE_PING_PLAYER = 1,
			CHOICE_DAMAGE_CREATURE,
			CHOICE_10_DAMAGES
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Ping a player", can_target(&td1), 5, 1,
						"Damage a creature", can_target(&td2), 10, 0,
						"10 damage", can_target(&td1), 15, -8);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			switch (choice)
			{
				case CHOICE_PING_PLAYER:
				case CHOICE_10_DAMAGES:
					pick_target(&td1, "TARGET_PLAYER");
					break;

				case CHOICE_DAMAGE_CREATURE:
				{
					int counters_to_remove = 0;
					if( pick_target(&td2, "TARGET_CREATURE") ){
						counters_to_remove = MIN(count_counters(player, card, COUNTER_LOYALTY), get_toughness(instance->targets[0].player, instance->targets[0].card));
						if( player == HUMAN ){
							counters_to_remove = choose_a_number(player, "Pay how much Loyalty ?", counters_to_remove);
						}
					}
					if( spell_fizzled != 1 ){
						if( counters_to_remove < 1 || counters_to_remove > count_counters(player, card, COUNTER_LOYALTY) ){
							spell_fizzled = 1;
						}
						else{
							remove_counters(player, card, COUNTER_LOYALTY, counters_to_remove);
							instance->targets[1].player = counters_to_remove;
						}
					}
				}
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice)
			{
				case CHOICE_PING_PLAYER:
				{
					if( valid_target(&td1) ){
						damage_target0(player, card, 1);
					}
				}
				break;

				case CHOICE_DAMAGE_CREATURE:
				{
					if( valid_target(&td2) ){
						damage_target0(player, card, instance->targets[1].player);
					}
				}
				break;

				case CHOICE_10_DAMAGES:
				{
					if( valid_target(&td1) ){
						new_damage_all(instance->parent_controller, instance->parent_card, instance->targets[0].player, 10,
										NDA_PLAYER_TOO | NDA_ALL_CREATURES, NULL);
					}
				}
				break;
			}
	}

	return planeswalker(player, card, event, 6);
}

int card_changeling_berserker(int player, int card, event_t event){
	haste(player, card, event);
	champion(player, card, event, -1, -1);
	return 0;
}

int card_changeling_hero(int player, int card, event_t event){
	lifelink(player, card, event);
	champion(player, card, event, -1, -1);
	return 0;
}

int card_cloudgoat_ranger(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KITHKIN_SOLDIER, &token);
		token.pow = token.tou = 1;
		token.qty = 3;
		generate_token(&token);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_KITHKIN;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( target_available(player, card, &td) > 2 ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! tapsubtype_ability(player, card, 3, &td) ){
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								2, 0, KEYWORD_FLYING, 0);
	}

	return 0;
}

int card_cloudthresher(int player, int card, event_t event){

	evoke(player, card, event, MANACOST_XG(2, 2));

	if(comes_into_play(player, card, event) > 0 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		APNAP(p, {new_damage_all(player, card, p, 2, 0, &this_test);};);
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return flash(player, card, event);
}

int card_colfenors_urn(int player, int card, event_t event){
	/*
	  Colfenor's Urn |3
	  Artifact
	  Whenever a creature with toughness 4 or greater is put into your graveyard from the battlefield, you may exile it.
	  At the beginning of the end step, if three or more cards have been exiled with Colfenor's Urn, sacrifice it. If you do, return those cards to the battlefield under their owner's control.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_token(affected_card_controller, affected_card) &&
			get_toughness(affected_card_controller, affected_card) > 3 && get_owner(affected_card_controller, affected_card) == player
		  ){
		   card_instance_t *dead = get_card_instance(affected_card_controller, affected_card);
		   if( dead->kill_code != KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int position = instance->targets[11].player;
				if( position < 10 ){
					instance->targets[position].card = get_original_internal_card_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
		   }
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int dead[10];
		int dc = 0;
		int i, leg = 0, idx = 0;
		for(i=0; i<instance->targets[11].card; i++){
			dead[dc] = instance->targets[i].card;
			dc++;
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile with Colfenor's Urn.");
		while( dc ){
				int selected = select_card_from_zone(player, player, dead, dc, 0, AI_MAX_CMC, -1, &this_test);
				if( selected == -1 ){
					break;
				}
				int iid = dead[selected];
				for(i=selected; i<dc; i++){
					dead[i] = dead[i+1];
				}
				dc--;
				const int* grave = get_grave(player);
				int count = count_graveyard(player)-1;
				while( count > -1 ){
						if( grave[count] == iid ){
							exiledby_remember(player, card, player, iid, &leg, &idx);
							rfg_card_from_grave(player, count);
						}
						count--;
				}
		}
		instance->targets[11].card = 0;
	}

	if( current_turn == player && trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player &&
		! is_humiliated(player, card)
	  ){
		int mode = exiledby_count(player, card, player) >= 3 ? RESOLVE_TRIGGER_MANDATORY : 0;
		if( eot_trigger_mode(player, card, event, player, mode) ){
			state_untargettable(player, card, 1); //To avoid unwanted interactions;
			int leg = 0;
			int idx = 0;
			int* loc;
			while ((loc = exiledby_find_any(player, card, &leg, &idx)) != NULL){
					int owner = (*loc & 0x80000000) ? 1 : 0;
					int iid = *loc & ~0x80000000;
					*loc = -1;
					int card_added = add_card_to_hand(owner, iid);
					put_into_play(owner, card_added);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

// shapeshifter token --> vanilla

int card_crib_swap(int player, int card, event_t event){
	/* Crib Swap	|2|W
	 * Tribal Instant - Shapeshifter
	 * Changeling
	 * Exile target creature. Its controller puts a 1/1 colorless Shapeshifter creature token with changeling onto the battlefield. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SHAPESHIFTER_TOKEN, &token);
			token.t_player = instance->targets[0].player;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_crush_underfoot(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.required_subtype = SUBTYPE_GIANT;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (!can_target(&td1)){
			ai_modifier -= 64;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int t_player = instance->targets[0].player;
			int t_card = instance->targets[0].card;

			if( can_target(&td1) && new_pick_target(&td1, "Choose a Giant creature you control.", 0, GS_LITERAL_PROMPT) ){
				damage_creature(t_player, t_card, get_power(instance->targets[0].player, instance->targets[0].card),
								instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_cryptic_command(int player, int card, event_t event){

	if (event == EVENT_CAN_COUNTER && raw_mana_available[player][COLOR_BLUE] >= 2){
		ai_modifier += 24;
	}

	if (IS_CASTING(player, card, event)){

		target_definition_t td_spell;
		counterspell_target_definition(player, card, &td_spell, 0);

		int can_counter_spell = counterspell(player, card, EVENT_CAN_CAST, &td_spell, -1);

		if (event == EVENT_CAN_CAST && player == HUMAN){
			return can_counter_spell ? 99 : 1;	// Always castable (choices 3 and 4), though we may force the AI not to
		}

		target_definition_t td_permanent;
		default_target_definition(player, card, &td_permanent, TYPE_PERMANENT);

		card_instance_t* instance = get_card_instance(player, card);

		typedef enum{
			CHOICE_INVALID_TARGET = 0,
			CHOICE_COUNTER = 1,
			CHOICE_BOUNCE = 2,
			CHOICE_TAP = 3,
			CHOICE_DRAW = 4
		} CommandChoices;

		int priority[5];
		if (player == HUMAN || event == EVENT_RESOLVE_SPELL){
			priority[CHOICE_COUNTER] = priority[CHOICE_BOUNCE] = priority[CHOICE_TAP] = priority[CHOICE_DRAW] = 1;
		} else {
			priority[CHOICE_COUNTER] = 1000000;	// If this is being cast in the interrupt window, the AI's got to pick this.

			td_permanent.allowed_controller = 1 - player;
			priority[CHOICE_BOUNCE] = can_target(&td_permanent) ? 30 : 5;
			td_permanent.allowed_controller = 2;

			target_definition_t td_untapped_creature;
			default_target_definition(player, card, &td_untapped_creature, TYPE_CREATURE);
			td_untapped_creature.allowed_controller = 1 - player;
			td_untapped_creature.illegal_abilities = 0;
			td_untapped_creature.illegal_state = TARGET_STATE_TAPPED;
			priority[CHOICE_TAP] = can_target(&td_untapped_creature) ? 30 : 0;

			priority[CHOICE_DRAW] = 5 * (count_deck(player) - 5);
			priority[CHOICE_DRAW] = MIN(priority[CHOICE_DRAW], 50);

			// No need for raise_command_singleton_ai_priorities(), since at least two options are always positive
		}

		CommandChoices choices[2];
		choices[0] = DIALOG(player, card, event,
							DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
							"Counter a spell",			can_counter_spell,			priority[CHOICE_COUNTER],
							"Bounce a permanent",		can_target(&td_permanent),	priority[CHOICE_BOUNCE],
							"Tap opponent's creatures",	1,							priority[CHOICE_TAP],
							"Draw a card",				1,							priority[CHOICE_DRAW]);

		if (event == EVENT_CAN_CAST){
			return (choices[0] <= 0 ? 0
					: can_counter_spell ? 99
					: 1);
		} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
			instance->number_of_targets = 0;
			int choice, tgt = 0;
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_COUNTER:
						counterspell(player, card, event, &td_spell, tgt);
						++tgt;
						break;

					case CHOICE_BOUNCE:
						if (!new_pick_target(&td_permanent, "TARGET_PERMANENT", tgt, 1)){
							return 0;
						}
						++tgt;
						break;

					case CHOICE_TAP:
					case CHOICE_DRAW:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
		} else if (event == EVENT_RESOLVE_SPELL){
			int choice, tgt = 0, num_invalid_tgts = 0;
			/* Excerpt from ruling on Cryptic Command 6/7/2013: If it has at least one target, and all its targets are illegal when it tries to resolve, then it
			 * will be countered and none of its effects will happen. */
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_COUNTER:{
						if (!counterspell_validate(player, card, &td_spell, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;
					}
					case CHOICE_BOUNCE:
						if (!validate_target(player, card, &td_permanent, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;

					case CHOICE_TAP:
					case CHOICE_DRAW:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
			if (tgt > 0 && num_invalid_tgts == tgt){
				spell_fizzled = 1;
			} else {
				for (choice = tgt = 0; choice < 2; ++choice){
					switch (choices[choice]){
						case CHOICE_INVALID_TARGET:
							++tgt;
							break;

						case CHOICE_COUNTER:
							kill_card(instance->targets[tgt].player, instance->targets[tgt].card, KILL_BURY);
							++tgt;
							break;

						case CHOICE_BOUNCE:
							bounce_permanent(instance->targets[tgt].player, instance->targets[tgt].card);
							++tgt;
							break;

						case CHOICE_TAP:
							manipulate_all(player, card, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_TAP);
							break;

						case CHOICE_DRAW:
							draw_cards(player, 1);
							break;
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_dauntless_dourbark(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1){
			int i;
			int sum = 0;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && (has_subtype(player, i, SUBTYPE_TREEFOLK) || has_subtype(player, i, SUBTYPE_FOREST)) ){
					sum++;
				}
			}
			event_result +=sum;
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			int sum = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && i != card && has_subtype(player, i, SUBTYPE_TREEFOLK) ){
					sum = 1;
					break;
				}
			}
			if( sum ){
				event_result |= KEYWORD_TRAMPLE;
			}
		}
	}

	return 0;
}

int card_deathrender(int player, int card, event_t event){
	/* Deathrender	|4
	 * Artifact - Equipment
	 * Equipped creature gets +2/+2.
	 * Whenever equipped creature dies, you may put a creature card from your hand onto the battlefield and attach ~ to it.
	 * Equip |2 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		instance->targets[7].player = player;
		instance->targets[7].card = card;
	}

	if( attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char buffer[100];
		scnprintf(buffer, 100, "Select creature to put into play.");
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
		this_test.zone = TARGET_ZONE_HAND;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			if( result > -1 ){
				equip_target_creature(instance->targets[7].player, instance->targets[7].card, player, result);
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 2, 2, 0, 0);
}

int card_dolmen_gate(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_target_card != -1 && damage->damage_target_player == player
	  && is_attacking(damage->damage_target_player, damage->damage_target_card))
	damage->info_slot = 0;

  return 0;
}

int card_doran_the_siege_tower(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if (event == EVENT_POW_BOOST && ! is_humiliated(player, card) ){
		return get_toughness(player, card) - get_power(player, card);
	}

	return 0;
}


int card_dread(int player, int card, event_t event)
{
	/* Dread	|3|B|B|B
	 * Creature - Elemental Incarnation 6/6
	 * Fear
	 * Whenever a creature deals damage to you, destroy it.
	 * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

	// Second ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	fear(player, card, event);

	return card_no_mercy(player, card, event);
}


int card_dreamspoil_witches(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player && current_turn != player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( can_target(&td) && specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			}
		}
	}

	return 0;
}

static int ego_erasure_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, -2, 0, 0);

	if( eot_trigger(player, card, event) ){
		reset_subtypes(instance->targets[0].player, instance->targets[0].card, 1);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_ego_erasure(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int t_player = instance->targets[0].player;
			int i;
			for(i=active_cards_count[t_player]-1; i>-1; i--){
				if( in_play(t_player, i) && is_what(t_player, i, TYPE_CREATURE) ){
					force_a_subtype(t_player, i, 211);
					create_targetted_legacy_effect(player, card, &ego_erasure_effect, t_player, i);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_elvish_branchbender(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_FOREST;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF);
			land_animation2(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1,
							amount, amount, 0, 0, 0, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST0, 0,
									&td, get_hacked_land_text(player, card, "Select target %s you control.", SUBTYPE_FOREST));
}

int card_elvish_harbinger(int player, int card, event_t event){
	harbinger(player, card, event, SUBTYPE_ELF);
	return card_generic_noncombat_1_mana_producing_creature(player, card, event);
}

int card_elvish_promenade(int player, int card, event_t event){
	/* Elvish Promenade	|3|G
	 * Tribal Sorcery - Elf
	 * Put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield for each Elf you control. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_ELF_WARRIOR, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF));
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_epic_proportions(int player, int card, event_t event){
	generic_aura(player, card, event, player, 5, 5, KEYWORD_TRAMPLE, 0, 0, 0, 0);
	return flash(player, card, event);
}

int card_eyeblights_ending(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_ELF;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target non-Elf creature.", 1, NULL);
}

int card_facevaulter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	return 0;
}

int card_faerie_harbinger(int player, int card, event_t event){
	harbinger(player, card, event, SUBTYPE_FAERIE);
	return flash(player, card, event);
}

int card_faerie_taunting(int player, int card, event_t event){

	if( current_turn != player && specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		lose_life(1-player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_faerie_trickery(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_FAERIE;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			rfg_top_card_of_deck(instance->targets[0].player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fallowsage(int player, int card, event_t event){
	if( event == EVENT_TAP_CARD && affect_me(player, card) && ! is_humiliated(player, card) ){
		draw_some_cards_if_you_want(player, card, player, 1);
	}
	return 0;
}

int card_familiars_rouse(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_subtype(player, TYPE_CREATURE, -1) ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		instance->number_of_targets = 0;

		if( pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fathom_trawl(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int found = 0;
			int count = 0;
			int f_deck[count_deck(player)];
			while( deck[0] != -1 && found < 3 ){
					f_deck[count] = deck[0];
					if( ! is_what(-1, deck[0], TYPE_LAND) ){
						found++;
					}
					remove_card_from_deck(player, 0);
					count++;
			}
			if( count > 0 ){
				show_deck( HUMAN, f_deck, count, "Cards revealed by Fathom Trawl", 0, 0x7375B0 );
				count--;
				int putted_back = 0;
				while( count > -1 ){
						int card_added = add_card_to_hand(player, f_deck[count]);
						if( is_what(-1, f_deck[count], TYPE_LAND) ){
							put_on_top_of_deck(player, card_added);
							putted_back++;
						}
						count--;
				}
				if( putted_back > 0 ){
					put_top_x_on_bottom(player, player, putted_back);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_favor_of_the_mighty(int player, int card, event_t event)
{
  /* Favor of the Mighty	|1|W
   * Tribal Enchantment - Giant
   * Each creature with the highest converted mana cost has protection from all colors. */

  if (event == EVENT_RESOLVE_SPELL)
	get_card_instance(player, card)->info_slot = -1;

  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int new_highest = get_highest_cmc(ANYBODY, TYPE_CREATURE);
	  if (new_highest != instance->info_slot)
		{
		  instance->info_slot = new_highest;
		  recalculate_all_cards_in_play();	// so everything's abilities are recalculated
		}
	}

  if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card)
	&& get_cmc(affected_card_controller, affected_card) == get_card_instance(player, card)->info_slot)
	event_result |= KEYWORD_PROT_COLORED;

  return global_enchantment(player, card, event);
}

int card_flamekin_bladewhirl(int player, int card, event_t event){

	return buddy_creature(player, card, event, SUBTYPE_ELEMENTAL);
}

int card_flamekin_spitfire(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(3, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_fodder_launch(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "Select a Goblin permanent to sacrifice.");
		test.subtype = SUBTYPE_GOBLIN;
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -5, -5);
			damage_player(instance->targets[0].player, 5, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_forced_fruition(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(1-player, 7);
	}

	return global_enchantment(player, card, event);
}

int card_gaddock_teeg(int player, int card, event_t event){
	//Also handled mainly in 'generic_spell'
	check_legend_rule(player, card, event);

	if(event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
		if(! (card_d->type & TYPE_LAND) && ! (card_d->type & TYPE_CREATURE) ){
			int cmc = get_cmc_by_id(card_d->id);
			if( cmc > 3 ){
				infinite_casting_cost();
			}
		}
	}

	return 0;
}

int card_galepowder_mage(int player, int card, event_t event){

	// Whenever ~ attacks, exile another target creature. Return that card to the battlefield under its owner's control at the beginning of the next end step.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		card_instance_t* instance = get_card_instance(player, card);

		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE") ){
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_garruk_wildspeaker(int player, int card, event_t event){

	/* Garruk Wildspeaker	|2|G|G
	 * Planeswalker - Garruk (3)
	 * +1: Untap two target lands.
	 * -1: Put a 3/3 |Sgreen Beast creature token onto the battlefield.
	 * -4: Creatures you control get +3/+3 and gain trample until end of turn. */

	if (IS_ACTIVATING(event)){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.required_state = TARGET_STATE_TAPPED;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);

		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_CREATURE);
		this_test2.state = STATE_TAPPED;
		this_test2.state_flag = DOESNT_MATCH;

		int priority_overrun = 0;
		int priority_untap_lands = 0;

		if( event == EVENT_ACTIVATE ){
			priority_overrun = ((check_battlefield_for_special_card(player, card, player, CBFSC_GET_TOTAL_POW, &this_test) + (count_subtype(player, TYPE_CREATURE, -1)*3))-
								check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_TOTAL_POW, &this_test2))*2;

			priority_untap_lands = target_available(player, card, &td1) > 1 ? 15 : 5;
		}

		enum{
			CHOICE_UNTAP_LANDS = 1,
			CHOICE_BEAST,
			CHOICE_OVERRUN
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Untap lands", target_available(player, card, &td) > 1, priority_untap_lands, 1,
						"Generate a Beast", 1, 10, -1,
						"Overrun", 1, priority_overrun, -4);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			switch (choice)
			{
				case CHOICE_UNTAP_LANDS:
				{
					if( pick_target(&td, "TARGET_LAND") ){
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						new_pick_target(&td, "TARGET_LAND", 1, 0);
					}
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				}
				break;

				case CHOICE_BEAST:
				case CHOICE_OVERRUN:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_UNTAP_LANDS:
			{
				int i;
				for(i=0; i<2; i++){
					if( validate_target(player, card, &td, i) ){
						untap_card(instance->targets[i].player, instance->targets[i].card);
					}
				}
			}
			break;

			case CHOICE_BEAST:
				generate_token_by_id(player, card, CARD_ID_BEAST);
				break;

			case CHOICE_OVERRUN:
				pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, -1, 3, 3, KEYWORD_TRAMPLE, 0);
				break;
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_ghostly_changeling(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_XB(1, 1), 1, 1, 0, 0);
}

int card_giant_harbinger(int player, int card, event_t event){
	return harbinger(player, card, event, SUBTYPE_GIANT);
}

int card_giants_ire(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 4, player, card);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_GIANT) ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_gilt_leaf_ambush(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELF_WARRIOR, &token);
		token.qty = 2;
		token.keep_track_of_tokens_generated = 19;
		generate_token(&token);

		card_instance_t *instance = get_card_instance(player, card);

		if( clash(player, card) ){
			unsigned int k;
			for (k = 0; k < token.keep_track_of_tokens_generated; k++){
				pump_ability_until_eot(player, card, instance->targets[k].player, instance->targets[k].card, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_gilt_leaf_palace(int player, int card, event_t event){

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_ELF);

	return mana_producer(player, card, event);
}

int card_gilt_leaf_seer(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		rearrange_top_x(player, player, 2);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL);
}

int card_goldmeadow_stalwart(int player, int card, event_t event){

	return buddy_creature(player, card, event, SUBTYPE_KITHKIN);
}

int card_guile(int player, int card, event_t event)
{
  /* Guile	|3|U|U|U
   * Creature - Elemental Incarnation 6/6
   * ~ can't be blocked except by three or more creatures.
   * If a spell or ability you control would counter a spell, instead exile that spell and you may play that card without paying its mana cost.
   * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

  // Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

  minimum_blockers(player, card, event, 3);
  return 0;
}

int card_hamletback_goliath(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), &test))
		{
		  card_instance_t *instance = get_card_instance(player, card);
		  int amount = get_power(instance->targets[1].player, instance->targets[1].card);
		  add_1_1_counters(player, card, amount);
		}
	}

  return 0;
}

int card_heat_shimmer(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			token.legacy = 1;
			token.special_code_for_legacy = &haste_and_remove_eot;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_hoarders_greed(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		int stop = 0;
		while( stop != 1 && deck[0] != -1 ){
				draw_cards(player, 2);
				lose_life(player, 2);
				stop = ! clash(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_hoofprints_of_the_stag(int player, int card, event_t event){

	/* Hoofprints of the Stag	|1|W
	 * Tribal Enchantment - Elemental
	 * Whenever you draw a card, you may put a hoofprint counter on ~.
	 * |2|W, Remove four hoofprint counters from ~: Put a 4/4 |Swhite Elemental creature token with flying onto the battlefield. Activate this ability only
	 * during your turn. */

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_AI(player))){
		add_counter(player, card, COUNTER_HOOFPRINT);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 4;
		token.tou = 4;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(2, 1), GVC_COUNTERS(COUNTER_HOOFPRINT, 4), NULL, NULL);
}

int card_horde_of_notions(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, 0, 1, 1, 1, 1, 1, 0, NULL, NULL) ){
			if( instance->targets[1].player != 66 && count_graveyard_by_subtype(player, SUBTYPE_ELEMENTAL) > 0 ){
				return ! graveyard_has_shroud(player);
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 1, 1, 1, 1, 1) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, 0, "Select target Elemental card.");
			this_test.subtype = SUBTYPE_ELEMENTAL;
			select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			play_card_in_grave_for_free(player, player, selected);
		}
	}

	return 0;
}

// Elemental shaman token --> rhino token

int card_hostility(int player, int card, event_t event){

	/* Hostility	|3|R|R|R
	 * Creature - Elemental Incarnation 6/6
	 * Haste
	 * If a spell you control would deal damage to an opponent, prevent that damage. Put a 3/1 |Sred Elemental Shaman creature token with haste onto the
	 * battlefield for each 1 damage prevented this way.
	 * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	haste(player, card, event);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == 1-player &&
				damage->info_slot > 0 && (damage->targets[3].player & TYPE_SPELL)
			  ){
				int amount = damage->info_slot;
				damage->info_slot = 0;

				token_generation_t token;
				default_token_definition(player, card, CARD_ID_ELEMENTAL_SHAMAN, &token);
				token.qty = amount;
				token.pow = 3;
				token.tou = 1;
				token.color_forced = COLOR_TEST_RED;
				token.s_key_plus = SP_KEYWORD_HASTE;
				generate_token(&token);
			}
		}
	}

	return 0;
}

int card_howltooth_hollow(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_B(2)) &&
			instance->targets[2].player > -1 && hand_count[player]+hand_count[1-player] == 0
		  ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Play the exiled card\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				charge_mana_for_activated_ability(player, card, MANACOST_B(1));
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 1;
					tap_card(player, card);
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	return hideaway(player, card, event);
}

int card_immaculate_magistrate(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_imperious_perfect(int player, int card, event_t event){
	/* Imperious Perfect	|2|G
	 * Creature - Elf Warrior 2/2
	 * Other Elf creatures you control get +1/+1.
	 * |G, |T: Put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

	// original code : 004DA8DE

	boost_creature_type(player, card, event, SUBTYPE_ELF, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_ELF_WARRIOR);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL);
}

int card_incandescent_soulstoke(int player, int card, event_t event){

	/* Incandescent Soulstoke	|2|R
	 * Creature - Elemental Shaman 2/2
	 * Other Elemental creatures you control get +1/+1.
	 * |1|R, |T: You may put an Elemental creature card from your hand onto the battlefield. That creature gains haste until end of turn. Sacrifice it at the
	 * beginning of the next end step. */

	boost_creature_type(player, card, event, SUBTYPE_ELEMENTAL, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select an Elemental creature card.");
		this_test.subtype = SUBTYPE_ELEMENTAL;
		int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		if( result > -1 ){
			create_targetted_legacy_effect(player, card, &haste_and_sacrifice_eot, player, result);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XR(1, 1), 0, NULL, NULL);
}

int card_incendiary_command(int player, int card, event_t event){

	if (IS_CASTING(player, card, event)){
		if (event == EVENT_CAN_CAST && player == HUMAN){
			return 1;	// Always castable (choices 2 and 4), though we may force the AI not to
		}

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td_player;
		default_target_definition(player, card, &td_player, TYPE_CREATURE);
		td_player.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td_land;
		default_target_definition(player, card, &td_land, TYPE_LAND);
		td_land.required_subtype = SUBTYPE_NONBASIC;

		typedef enum{
			CHOICE_INVALID_TARGET = 0,
			CHOICE_PLAYER = 1,
			CHOICE_CREATURES = 2,
			CHOICE_LAND = 3,
			CHOICE_DISCARD_AND_DRAW = 4
		} CommandChoices;

		int priority[5];
		if (player == HUMAN || event == EVENT_RESOLVE_SPELL){
			priority[CHOICE_PLAYER] = priority[CHOICE_CREATURES] = priority[CHOICE_LAND] = priority[CHOICE_DISCARD_AND_DRAW] = 1;
		} else {
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			if (!would_valid_target(&td_player)){
				priority[CHOICE_PLAYER] = -100;
			} else if (life[1-player] <= 4){
				priority[CHOICE_PLAYER] = 10000;
			} else {
				priority[CHOICE_PLAYER] = 160 / (life[1-player] - 4);	// 5 life=>160; 6 life=>80; 7 life=>53; 8 life=>40; 10 life=>26; 16 life=>13; 20 life=>10
			}

			priority[CHOICE_CREATURES] = 0;
			int p;
			for (p = 0; p < 2; ++p){
				int c, delta = p == player ? -20 : 20;
				for (c = active_cards_count[p] - 1; c >= 0; --c){
					if (in_play(p, c) && is_what(p, c, TYPE_CREATURE)
						&& !check_for_ability(p, c, KEYWORD_PROT_RED)
						&& get_toughness(p, c) - get_card_instance(p, c)->damage_on_card <= 2){
						priority[CHOICE_CREATURES] += delta;
					}
				}
			}

			td_land.allowed_controller = 1-player;
			priority[CHOICE_LAND] = can_target(&td_land) ? 30 : -100;
			td_land.allowed_controller = 2;

			priority[CHOICE_DISCARD_AND_DRAW] = 5 * (2 * (hand_count[player] - (event == EVENT_CAN_CAST ? 0 : 1)) - 3 * hand_count[1-player]);

			raise_command_singleton_ai_priorities(event, priority);
		}

		CommandChoices choices[2];
		choices[0] = DIALOG(player, card, event,
							DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
							"4 damage to player",			can_target(&td_player),	priority[CHOICE_PLAYER],
							"2 damage to all creatures",	1,						priority[CHOICE_CREATURES],
							"Destroy nonbasic land",		can_target(&td_land),	priority[CHOICE_LAND],
							"Discard and draw",				1,						priority[CHOICE_DISCARD_AND_DRAW]);

		if (event == EVENT_CAN_CAST){
			return choices[0] > 0;
		} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
			instance->number_of_targets = 0;
			int choice, tgt = 0;
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_PLAYER:
						ASSERT(tgt == 0);	// or else pick_player_duh would go into the wrong target number
						if (!pick_player_duh(player, card, 1-player, 1)){
							return 0;
						}
						++tgt;
						break;

					case CHOICE_CREATURES:
						break;

					case CHOICE_LAND:
						if (!new_pick_target(&td_land, "TARGET_NONBASIC_LAND", tgt, 1)){
							return 0;
						}
						++tgt;
						break;

					case CHOICE_DISCARD_AND_DRAW:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
		} else if (event == EVENT_RESOLVE_SPELL){
			int choice, tgt = 0, num_invalid_tgts = 0;
			/* Excerpt from ruling on Cryptic Command 6/7/2013: If it has at least one target, and all its targets are illegal when it tries to resolve, then it
			 * will be countered and none of its effects will happen. */
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_PLAYER:
						if (!validate_target(player, card, &td_player, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;

					case CHOICE_LAND:
						if (!validate_target(player, card, &td_land, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;

					case CHOICE_CREATURES:
					case CHOICE_DISCARD_AND_DRAW:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
			if (tgt > 0 && num_invalid_tgts == tgt){
				spell_fizzled = 1;
			} else {
				for (choice = tgt = 0; choice < 2; ++choice){
					switch (choices[choice]){
						case CHOICE_INVALID_TARGET:
							++tgt;
							break;

						case CHOICE_PLAYER:
							damage_player(instance->targets[tgt].player, 4, player, card);
							++tgt;
							break;

						case CHOICE_CREATURES:
							new_damage_all(player, card, 2, 2, NDA_ALL_CREATURES, NULL);
							break;

						case CHOICE_LAND:
							kill_card(instance->targets[tgt].player, instance->targets[tgt].card, KILL_DESTROY);
							++tgt;
							break;

						case CHOICE_DISCARD_AND_DRAW:
						{
							APNAP(p,{
									int cards = hand_count[p];
									new_discard_all(p, player);
									draw_cards(p, cards);
									};
							);
							break;
						}
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_incremental_growth(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<3; i++){
			if( validate_target(player, card, &td, i) ){
				add_1_1_counters(instance->targets[i].player, instance->targets[i].card, i+1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

int card_ingot_chewer(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, MANACOST_R(1));
}

int card_inner_flame_acolyte(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0, 0, SP_KEYWORD_HASTE);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, MANACOST_R(1));
}

int card_inner_flame_igniter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		if( parent->targets[1].player < 0 ){
			parent->targets[1].player = 0;
		}
		parent->targets[1].player++;
		int keyword = 0;
		if( parent->targets[1].player == 3 ){
			keyword = KEYWORD_FIRST_STRIKE;
		}
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 0, keyword, 0);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XR(2, 1), 0, NULL, NULL);
}

int card_jace_beleren(int player, int card, event_t event){

	/* Jace Beleren	|1|U|U
	 * Planeswalker - Jace (3)
	 * +2: Each player draws a card.
	 * -1: Target player draws a card.
	 * -10: Target player puts the top twenty cards of his or her library into his or her graveyard. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td1;
		default_target_definition(player, card, &td1, 0);
		td1.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;
		td2.preferred_controller = player;

		int priorities[3] = {	count_counters(player, card, COUNTER_LOYALTY) < 2 ? 15 : 5,
								10,
								would_validate_arbitrary_target(&td1, 1-player, -1) ? 20-((10-count_counters(player, card, COUNTER_LOYALTY))*2) : 0
		};

		enum{
			CHOICE_BOTH_DRAW = 1,
			CHOICE_TARGET_PLAYER_DRAWS,
			CHOICE_MILL20
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Both players draw", 1, priorities[0], 2,
						"Target player draws", can_target(&td2), priorities[1], -1,
						"Mill 20", can_target(&td1), priorities[2], -10);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
				case CHOICE_BOTH_DRAW:
					break;

				case CHOICE_TARGET_PLAYER_DRAWS:
				{
					if( player == AI && would_validate_arbitrary_target(&td1, 1-player, -1) && count_deck(1-player) < 2 ){
						pick_target(&td1, "TARGET_PLAYER");
					}
					else{
						pick_target(&td2, "TARGET_PLAYER");
					}
				}
				break;

				case CHOICE_MILL20:
					pick_target(&td1, "TARGET_PLAYER");
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_BOTH_DRAW:
				APNAP(p, {draw_cards(p, 1);};);
				break;

			case CHOICE_TARGET_PLAYER_DRAWS:
			{
				if( valid_target(&td1) ){
					draw_cards(instance->targets[0].player, 1);
				}
			}
			break;

			case CHOICE_MILL20:
			{
				if( valid_target(&td1) ){
					mill(instance->targets[0].player, 20);
				}
			}
			break;
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_jagged_scar_archers(int player, int card, event_t event){
	/* Jagged-Scar Archers	|1|G|G
	 * Creature - Elf Archer 100/100
	 * ~'s power and toughness are each equal to the number of Elves you control.
	 * |T: ~ deals damage equal to its power to target creature with flying. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(instance->targets[0].player, instance->targets[0].card, get_power(player, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_judge_of_the_currents(int player, int card, event_t event){

	if( event == EVENT_TAP_CARD && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_MERFOLK) ){
			gain_life(player, 1);
		}
	}

	return 0;
}


int card_kinsbaille_skirmisher(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return 0;
}

int card_kithkin_greatheart(int player, int card, event_t event){
	return boost_if_subtype_is_present(player, card, event, SUBTYPE_GIANT, 1, 1, KEYWORD_FIRST_STRIKE, 0);
}

int card_kithkin_harbinger(int player, int card, event_t event){
	return harbinger(player, card, event, SUBTYPE_KITHKIN);
}

int card_kithkin_mourncaller(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_KITHKIN;
		this_test.subtype_flag = F2_MULTISUBTYPE;
		this_test.sub2 = SUBTYPE_ELF;
		this_test.state = STATE_ATTACKING;

		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && reason_for_trigger_controller == player &&
		instance->kill_code < KILL_DESTROY
	  ){
		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
			draw_cards(player, instance->targets[11].card);
			instance->targets[11].card = 0;
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		draw_cards(player, instance->targets[11].player);
	}

	return 0;
}

int card_knucklebone_witch(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_GOBLIN;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		card_instance_t *instance = get_card_instance( player, card );
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_lairwatch_giant(int player, int card, event_t event){
	creature_can_block_additional(player, card, event, 1);

	if (event == EVENT_DECLARE_BLOCKERS && ! is_humiliated(player, card) ){
		int num_blocking = count_creatures_this_is_blocking(player, card);
		if (num_blocking >= 2)
			pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}

	return 0;
}

int card_lash_out(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
		if( clash(player, card) ){
			damage_player(instance->targets[0].player, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_lignify(int player, int card, event_t event){
	/*
	  Lignify |1|G
	  Tribal Enchantment - Treefolk Aura
	  Enchant creature
	  Enchanted creature is a 0/4 Treefolk with no abilities.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != - 1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if (event == EVENT_ABILITIES && affect_me(p, c)){
			event_result &= ~(get_card_data(p, c)->static_ability);
		}

		if (event == EVENT_POWER && affect_me(p, c)){
			event_result += -get_base_power(p, c);
		}

		if (event == EVENT_TOUGHNESS && affect_me(p, c)){
			event_result += 4 - get_base_toughness(p, c);
		}

	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, SUBTYPE_TREEFOLK, 0, GA_FORBID_HUMILIATE | GA_FORCE_SUBTYPE);
}

int card_liliana_vess(int player, int card, event_t event){

	/* Liliana Vess	|3|B|B
	 * Planeswalker - Liliana (5)
	 * +1: Target player discards a card.
	 * -2: Search your library for a card, then shuffle your library and put that card on top of it.
	 * -8: Put all creature cards from all graveyards onto the battlefield under your control. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_FORCE_DISCARD = 1,
			CHOICE_TUTOR,
			CHOICE_GLOBAL_REANIMATION
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Make player discard", can_target(&td), count_counters(player, card, COUNTER_LOYALTY) < 2 ? 15 : 5, 1,
						"Tutor", 1, 10, -2,
						"Global reanimation", 1, count_graveyard_by_type(ANYBODY, TYPE_CREATURE)*3, -8);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			switch (choice)
			{
				case CHOICE_FORCE_DISCARD:
					pick_target(&td, "TARGET_PLAYER");
					break;

				case CHOICE_TUTOR:
				case CHOICE_GLOBAL_REANIMATION:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_FORCE_DISCARD:
			{
				if( valid_target(&td)){
					discard(instance->targets[0].player, 0, player);
				}
			}
			break;

			case CHOICE_TUTOR:
			{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
			}
			break;

			case CHOICE_GLOBAL_REANIMATION:
			{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_reanimate_all(player, card, ANYBODY, &this_test, REANIMATE_DEFAULT | REANIMATE_ALL_UNDER_CASTERS_CONTROL);
			}
			break;
		}
	}

	return planeswalker(player, card, event, 5);
}

int card_lys_alana_huntmaster(int player, int card, event_t event){
	/* Lys Alana Huntmaster	|2|G|G
	 * Creature - Elf Warrior 3/3
	 * Whenever you cast an Elf spell, you may put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, SUBTYPE_ELF, 0, 0, 0, 0, 0, -1, 0) ){
		generate_token_by_id(player, card, CARD_ID_ELF_WARRIOR);
	}

	return 0;
}

int card_mad_auntie(int player, int card, event_t event){

	boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_GOBLIN;
	td.special = TARGET_SPECIAL_NOT_ME;
	if( player == AI ){
		td.special |= TARGET_SPECIAL_REGENERATION;
	}
	td.required_state = TARGET_STATE_DESTROYED;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select a Goblin permanent to regenerate.");

}

static int effect_makeshift_mannequin(int player, int card, event_t event)
{
  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->damage_target_player, c = instance->damage_target_card;
	  if (p < 0 || c < 0 || !in_play(p, c) || !count_counters(p, c, COUNTER_MANNEQUIN))
		{
		  kill_card(player, card, KILL_REMOVE);
		  return 0;
		}
	}

  return attached_creature_gains_sacrifice_when_becomes_target(player, card, event);
}

int card_makeshift_mannequin(int player, int card, event_t event){

	/* Makeshift Mannequin	|3|B
	 * Instant
	 * Return target creature card from your graveyard to the battlefield with a mannequin counter on it. For as long as that creature has a mannequin counter
	 * on it, it has "When this creature becomes the target of a spell or ability, sacrifice it." */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.ai_selection_mode = AI_MAX_CMC;

	if(event == EVENT_RESOLVE_SPELL){
		int selected;
		if((selected = validate_target_from_grave_source(player, card, player, 0)) != -1){
			reanimate_permanent_with_counter_and_effect(player, card, player, selected, REANIMATE_DEFAULT, COUNTER_MANNEQUIN, effect_makeshift_mannequin);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_marsh_flitter(int player, int card, event_t event){
	/* Marsh Flitter	|3|B
	 * Creature - Faerie Rogue 1/1
	 * Flying
	 * When ~ enters the battlefield, put two 1/1 |Sblack Goblin Rogue creature tokens onto the battlefield.
	 * Sacrifice a Goblin: ~ has base power and toughness 3/3 until end of turn. */

	card_instance_t *instance= get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN_ROGUE, 2);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 3, 3, 0, 0, 0);
	}

	return 0;
}

int card_masked_admirers(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_merrow_commerce(int player, int card, event_t event){

	if( current_turn == player && eot_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_MERFOLK;
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	return global_enchantment(player, card, event);
}

int card_merrow_harbinger(int player, int card, event_t event){
	return harbinger(player, card, event, SUBTYPE_MERFOLK);
}

int card_merrow_reejerey(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	boost_creature_type(player, card, event, SUBTYPE_MERFOLK, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, SUBTYPE_MERFOLK, 0, 0, 0, 0, 0, -1, 0) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		if( can_target(&td) ){
			load_text(0, "MERROW_REEJEREY");
			if( select_target(player, card, &td, text_lines[0], NULL) ){
				instance->number_of_targets = 1;
				int choice = 0;
				if( player != AI ){
					choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
				}
				else{
					if( instance->targets[0].player == player ){
						choice = 1;
					}
				}
				if(choice == 0){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				else{
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
		}

	}

	return 0;
}

int card_mirror_entity(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	changeling_switcher(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, -1,
										instance->info_slot, instance->info_slot, KEYWORD_PROT_INTERRUPTS, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(-1), 0, NULL, NULL);
}

int card_moonglove_extract(int player, int card, event_t event){
	/*
	  Moonglove Extract |3
	  Artifact
	  Sacrifice Moonglove Extract: Moonglove Extract deals 2 damage to target creature or player.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_mistbind_clique(int player, int card, event_t event){
	champion(player, card, event, SUBTYPE_FAERIE, -1);
	return flash(player, card, event);
}

int card_mosswort_bridge(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_G(2)) && instance->targets[2].player > -1 &&
			get_power_sum(player) > 9
		  ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Play the exiled card\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, MANACOST_G(1));
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	return hideaway(player, card, event);
}

int card_mournwhelk(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, MANACOST_XB(3, 1));
}


int card_mulldrifter(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		draw_cards(player, 2);
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, MANACOST_XU(2, 1));
}

static int nameless_inversion_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CLEANUP ){
		reset_subtypes(instance->targets[0].player, instance->targets[0].card, 1);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_nameless_inversion(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			force_a_subtype(instance->targets[0].player, instance->targets[0].card, 211);
			int legacy = create_targetted_legacy_effect(player, card, &nameless_inversion_effect, instance->targets[0].player, instance->targets[0].card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, -3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_nath_of_the_gilt_leaf(int player, int card, event_t event){
	/* Nath of the Gilt-Leaf	|3|B|G
	 * Legendary Creature - Elf Warrior 4/4
	 * At the beginning of your upkeep, you may have target opponent discard a card at random.
	 * Whenever an opponent discards a card, you may put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

	check_legend_rule(player, card, event);

	if( discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player), 0) ){
		generate_token_by_id(player, card, CARD_ID_ELF_WARRIOR);
	}

	if( trigger_condition == TRIGGER_UPKEEP ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		upkeep_trigger_ability_mode(player, card, event, player, would_validate_arbitrary_target(&td, 1-player, -1) ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			discard(1-player, DISC_RANDOM, player);
		}
	}

	return 0;
}

int card_nightshade_stinger(int player, int card, event_t event){
	cannot_block(player, card, event);
	return 0;
}

int card_nova_chaser(int player, int card, event_t event){
	return champion(player, card, event, SUBTYPE_ELEMENTAL, -1);
}

int card_oblivion_ring(int player, int card, event_t event){

	/* Oblivion Ring	|2|W
	 * Enchantment
	 * When ~ enters the battlefield, exile another target nonland permanent.
	 * When ~ leaves the battlefield, return the exiled card to the battlefield under its owner's control. */

	if (event == EVENT_CAN_CAST){
		return 1;
	}

	return_from_oblivion(player, card, event);

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;
		td.illegal_type = TYPE_LAND;
		td.special = TARGET_SPECIAL_NOT_ME;
		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			card_instance_t *instance = get_card_instance(player, card);
			if (player == AI && instance->targets[0].player == AI){
				ai_modifier -= 64;
			}
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
		} else if (player == AI) {
			ai_modifier -= 48;
		}
	}

	return 0;
}

int effect_oonas_prowler(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player != -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_use_activated_abilities(p, c) && hand_count[player] > 0 ){
				int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
				if( has_mana_multi(player, cless, 0, 0, 0, 0, 0) ){
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
			charge_mana_multi(player, cless, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				discard(player, 0, player);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(p, c, p, c, -2, 0);
		}

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

	}

	return 0;
}

int card_oonas_prowler(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &effect_oonas_prowler);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
		obliterate_card(1-player, fake);
		hand_count[1-player]--;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, -2, 0);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_peppersmoke(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_FAERIE) ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


int card_pestermite(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.preferred_controller = 2;
		if( can_target(&td) && select_target(player, card, &td, "Choose a permanent to tap / untap", NULL)){
			twiddle(player, card, 0);
		}
	}

	return flash(player, card, event);
}

int card_pollen_lullaby(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		if( clash(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, 1-player, &this_test, ACT_DOES_NOT_UNTAP);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// kithkin soldier token --> rhino token

int card_militias_pride(int player, int card, event_t event)
{
  // Whenever a nontoken creature you control attacks, you may pay |W. If you do, put a 1/1 |Swhite Kithkin Soldier creature token onto the battlefield tapped and attacking.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && has_mana(player, COLOR_WHITE, 1))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.type_flag = F1_NO_TOKEN;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_AI(player), player, -1, &test)))
		{
		  int old_max_x_value = max_x_value;
		  max_x_value = amt;
		  int rval = !charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_WHITE, -1) || x_value <= 0;
		  max_x_value = old_max_x_value;
		  if (rval)
			return 0;

		  token_generation_t token;
		  default_token_definition(player, card, CARD_ID_KITHKIN_SOLDIER, &token);
		  token.action = TOKEN_ACTION_ATTACKING;
		  token.qty = x_value;
		  generate_token(&token);
		}
	}

  return global_enchantment(player, card, event);
}

int card_ponder(int player, int card, event_t event){
	/*
	  Ponder |U
	  Sorcery
	  Look at the top three cards of your library, then put them back in any order. You may shuffle your library.
	  Draw a card.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		rearrange_top_x(player, player, 3);
		if( do_dialog(player, player, card, -1, -1, " Shuffle\n Pass", internal_rand(1)) == 0 ){
			shuffle(player);
		}
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_primal_command(int player, int card, event_t event){

	if (IS_CASTING(player, card, event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td_player;
		default_target_definition(player, card, &td_player, TYPE_CREATURE);
		td_player.zone = TARGET_ZONE_PLAYERS;
		td_player.preferred_controller = player;

		target_definition_t td_noncreature;
		default_target_definition(player, card, &td_noncreature, TYPE_PERMANENT);
		td_noncreature.illegal_type = TYPE_CREATURE;

		typedef enum{
			CHOICE_INVALID_TARGET = 0,
			CHOICE_GAIN_LIFE = 1,
			CHOICE_NONCREATURE_ON_LIBRARY = 2,
			CHOICE_SHUFFLE_GRAVEYARD = 3,
			CHOICE_TUTOR_CREATURE = 4
		} CommandChoices;

		int priority[5];
		int noncreature_on_deck_choice = -1;
		if (player == HUMAN || event == EVENT_RESOLVE_SPELL){
			priority[CHOICE_GAIN_LIFE] = priority[CHOICE_NONCREATURE_ON_LIBRARY] = priority[CHOICE_SHUFFLE_GRAVEYARD] = priority[CHOICE_TUTOR_CREATURE] = 1;
		} else {
			instance->targets[0].player = player;
			instance->targets[0].card = -1;
			if (!would_valid_target(&td_player)){
				priority[CHOICE_GAIN_LIFE] = -100;
			} else {
				priority[CHOICE_GAIN_LIFE] = 700/(life[player]+6) - life[player]*life[player]/4;
				priority[CHOICE_GAIN_LIFE] = MAX(priority[CHOICE_GAIN_LIFE], 1);
				// 1 life => 100; 3 life => 75; 5 life => 57; 8 life => 34; 10 life => 18; 12 life => 2; 13+ life = 1;
			}

			priority[CHOICE_NONCREATURE_ON_LIBRARY] = -100;
			int c;
			instance->targets[0].player = 1-player;
			for (c = 0; c < active_cards_count[1-player]; ++c){
				instance->targets[0].card = c;
				if (would_valid_target(&td_noncreature)){
					int bv = my_base_value(1-player, c) / 5;
					if (bv > priority[CHOICE_NONCREATURE_ON_LIBRARY]){
						priority[CHOICE_NONCREATURE_ON_LIBRARY] = bv;
						noncreature_on_deck_choice = c;
					}
				}
			}

			int cards_in_library = count_deck(player);
			// No idea how to properly weight this.  I'm just going to assume that it's only good for the caster, and only if few cards are left in library.
			td_player.allowed_controller = player;
			priority[CHOICE_SHUFFLE_GRAVEYARD] = can_target(&td_player) ? (3*count_graveyard(player) - 2*cards_in_library) : -150;
			td_player.allowed_controller = 2;

			priority[CHOICE_TUTOR_CREATURE] = 5 * cards_in_library;
			priority[CHOICE_TUTOR_CREATURE] = MIN(priority[CHOICE_TUTOR_CREATURE], 25);

			raise_command_singleton_ai_priorities(event, priority);
		}

		CommandChoices choices[2];
		choices[0] = DIALOG(player, card, event,
							DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
							"Gain 7 life",						can_target(&td_player),			priority[CHOICE_GAIN_LIFE],
							"Put noncreature on library",		can_target(&td_noncreature),	priority[CHOICE_NONCREATURE_ON_LIBRARY],
							"Shuffle graveyard into library",	can_target(&td_player),			priority[CHOICE_SHUFFLE_GRAVEYARD],
							"Tutor a creature",					1,								priority[CHOICE_TUTOR_CREATURE]);

		if (event == EVENT_CAN_CAST){
			return choices[0] > 0;
		} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
			instance->number_of_targets = 0;
			int choice, tgt = 0;
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_GAIN_LIFE:
						instance->targets[tgt].player = player;
						instance->targets[tgt].card = -1;
						if (duh_mode(player) && would_validate_target(player, card, &td_player, tgt)){
							instance->number_of_targets++;
						} else if (!select_target(player, card, &td_player, "Select target player to gain life.", &instance->targets[tgt])){
							spell_fizzled = 1;
							return 0;
						}
						++tgt;
						break;

					case CHOICE_NONCREATURE_ON_LIBRARY:
						if (noncreature_on_deck_choice >= 0){	// we've already chosen and validated a target above
							instance->targets[tgt].player = 1 - player;
							instance->targets[tgt].card = noncreature_on_deck_choice;
							instance->number_of_targets++;
						} else if (!select_target(player, card, &td_noncreature, "Select target noncreature permanent.", &instance->targets[tgt])){
							spell_fizzled = 1;
							return 0;
						}
						++tgt;
						break;

					case CHOICE_SHUFFLE_GRAVEYARD:
						instance->targets[tgt].player = player;
						instance->targets[tgt].card = -1;
						if (player == AI && would_validate_target(player, card, &td_player, tgt)){
							instance->number_of_targets++;
						} else if (!select_target(player, card, &td_player, "Select target player to shuffle graveyard.", &instance->targets[tgt])){
							spell_fizzled = 1;
							return 0;
						}
						++tgt;
						break;

					case CHOICE_TUTOR_CREATURE:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
		} else if (event == EVENT_RESOLVE_SPELL){
			int choice, tgt = 0, num_invalid_tgts = 0;
			/* Excerpt from ruling on Cryptic Command 6/7/2013: If it has at least one target, and all its targets are illegal when it tries to resolve, then it
			 * will be countered and none of its effects will happen. */
			for (choice = 0; choice < 2; ++choice){
				switch (choices[choice]){
					case CHOICE_GAIN_LIFE:
					case CHOICE_SHUFFLE_GRAVEYARD:
						if (!validate_target(player, card, &td_player, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;

					case CHOICE_NONCREATURE_ON_LIBRARY:
						if (!validate_target(player, card, &td_noncreature, tgt)){
							++num_invalid_tgts;
							choices[choice] = CHOICE_INVALID_TARGET;
						}
						++tgt;
						break;

					case CHOICE_TUTOR_CREATURE:
					case CHOICE_INVALID_TARGET:
						break;
				}
			}
			if (tgt > 0 && num_invalid_tgts == tgt){
				spell_fizzled = 1;
			} else {
				for (choice = tgt = 0; choice < 2; ++choice){
					switch (choices[choice]){
						case CHOICE_INVALID_TARGET:
							++tgt;
							break;

						case CHOICE_GAIN_LIFE:
							gain_life(instance->targets[tgt].player, 7);
							++tgt;
							break;

						case CHOICE_NONCREATURE_ON_LIBRARY:
							put_on_top_of_deck(instance->targets[tgt].player, instance->targets[tgt].card);
							++tgt;
							break;

						case CHOICE_SHUFFLE_GRAVEYARD:
							reshuffle_grave_into_deck(instance->targets[tgt].player, 0);
							++tgt;
							break;

						case CHOICE_TUTOR_CREATURE:{
							test_definition_t this_test;
							new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
							new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
							break;
						}
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_profane_command(int player, int card, event_t event)
{
  if (IS_CASTING(player, card, event))
	{
	  if (event == EVENT_CAN_CAST && check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG))
		return 0;

	  typedef enum
	  {
		CHOICE_LOSE_LIFE = 1,
		CHOICE_REANIMATE,
		CHOICE_WEAKEN,
		CHOICE_FEAR
	  } CommandChoices;

	  card_instance_t* instance = get_card_instance(player, card);

	  int ai = player == AI || ai_is_speculating == 1;

	  int x;
	  if (event == EVENT_CAN_CAST)
		x = has_mana(player, COLOR_ANY, 1) - 2;	// for the BB part; will be used only for computing AI priorities
	  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
		SET_HIWORD(instance->info_slot) = x = x_value;	// low two bytes are storage for dialog choices
	  else
		x = HIWORD(instance->info_slot);

	  target_definition_t td_player;
	  default_target_definition(player, card, &td_player, 0);
	  td_player.zone = TARGET_ZONE_PLAYERS;

	  test_definition_t cmc_lt_x;
	  char msg[100];
	  if (ai || event != EVENT_CAST_SPELL)
		msg[0] = 0;
	  else
		sprintf(msg, "Select target creature card with CMC %d or less.", x);
	  new_default_test_definition(&cmc_lt_x, TYPE_CREATURE, msg);
	  cmc_lt_x.cmc = x + 1;
	  cmc_lt_x.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

	  target_definition_t td_creature_to_weaken;
	  default_target_definition(player, card, &td_creature_to_weaken, TYPE_CREATURE);

	  target_definition_t td_creature_to_fear;
	  default_target_definition(player, card, &td_creature_to_fear, TYPE_CREATURE);
	  td_creature_to_fear.preferred_controller = player;

	  int legality[5] = {0}, priority[5] = {0};
	  if (event != EVENT_RESOLVE_SPELL)	// compute choice legality
		{
		  legality[CHOICE_LOSE_LIFE] = can_target(&td_player);
		  legality[CHOICE_REANIMATE] = !graveyard_has_shroud(player) && new_special_count_grave(player, &cmc_lt_x);
		  legality[CHOICE_WEAKEN] = can_target(&td_creature_to_weaken);
		  legality[CHOICE_FEAR] = 1;
		}

	  if (ai && event != EVENT_RESOLVE_SPELL)	// compute ai priorities
		{
		  if (life[1-player] <= x)
			priority[CHOICE_LOSE_LIFE] = 1000000;
		  else
			priority[CHOICE_LOSE_LIFE] = 1 + (100 * x / life[1-player]);	// 1..100

		  if (legality[CHOICE_REANIMATE])
			{
			  if (check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE)
				  || new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &cmc_lt_x, 18) == -1)
				priority[CHOICE_REANIMATE] = -1000;
			  else
				priority[CHOICE_REANIMATE] = my_base_value(-1, instance->targets[18].card);	// roughly 1..100
			}

		  if (legality[CHOICE_WEAKEN])
			{
			  target_definition_t killable_creature;
			  default_target_definition(player, card, &killable_creature, TYPE_CREATURE);
			  killable_creature.toughness_requirement = x | TARGET_PT_LESSER_OR_EQUAL;
			  killable_creature.allowed_controller = 1-player;

			  priority[CHOICE_WEAKEN] = can_target(&killable_creature) ? 50 : 10;
			}

		  priority[CHOICE_FEAR] = target_available(player, card, &td_creature_to_fear);
		  priority[CHOICE_FEAR] = 10 * MIN(priority[CHOICE_FEAR], x);

		  // Count black or artifact creatures
		  int i, num_can_block = 0;
		  for (i = 0; i < active_cards_count[1-player]; ++i)
			if (in_play(1-player, i) && is_what(1-player, i, TYPE_CREATURE) && !is_tapped(1-player, i)
				&& (is_what(1-player, i, TYPE_ARTIFACT) || (get_color(1-player, i) & COLOR_TEST_BLACK)))
			  ++num_can_block;

		  priority[CHOICE_FEAR] /= (num_can_block + 1);
		}

	  CommandChoices choices[2];
	  choices[0] = DIALOG(player, card, event,
						  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
						  "Player loses X life",	legality[CHOICE_LOSE_LIFE],	priority[CHOICE_LOSE_LIFE],
						  "Reanimate creature",		legality[CHOICE_REANIMATE],	priority[CHOICE_REANIMATE],
						  "Creature gets -X/-X",	legality[CHOICE_WEAKEN],	priority[CHOICE_WEAKEN],
						  "X creatures gain fear",	legality[CHOICE_FEAR],		priority[CHOICE_FEAR]);

	  if (event == EVENT_CAN_CAST)
		return choices[0];
	  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
		{
		  instance->number_of_targets = 0;
		  int choice;
		  for (choice = 0; choice < 2; ++choice)
			switch (choices[choice])
			  {
				case CHOICE_LOSE_LIFE:
				  ASSERT(choice == 0);	// or else pick_player_duh would go into the wrong target number
				  if (!pick_player_duh(player, card, 1-player, 1))
					return 0;
				  break;

				case CHOICE_REANIMATE:
				  // Always stored in targets[18].  Already there if ai != 0.
				  if (!ai && new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &cmc_lt_x, 18) == -1)
					return 0;
				  break;

				case CHOICE_WEAKEN:
				  if (!ai)
					sprintf(msg, "Select target creature to get %d/%d.", -x, -x);
				  if (!pick_next_target_noload(&td_creature_to_weaken, msg))
					return 0;
				  break;

				case CHOICE_FEAR:
				  {
					if (x <= 0)
					  break;

					// This is always the second choice, since they're sorted.
					ASSERT(choice == 1);
					/* If the first choice was reanimate, then there's a target in [18], and [0]-[17] are available.  Otherwise, there's a target in [0], and
					 * [1]-[18] are available.  Either way, 18 possible targets are left. */
					int i, end = MIN(x, 18), old_cancel = cancel;
					for (i = 0; i < end && can_target(&td_creature_to_fear); ++i)
					  {
						if (!ai)
						  sprintf(msg, "Select target creature %d of %d to gain fear.", i + 1, end);
						if (pick_next_target_noload(&td_creature_to_fear, msg))
						  state_untargettable(instance->targets[instance->number_of_targets - 1].player, instance->targets[instance->number_of_targets - 1].card, 1);
						else
						  {
							cancel = old_cancel;
							break;
						  }
					  }
					remove_untargettable_from_all();
					break;
				  }
			  }
		}
	  else	// event == EVENT_RESOLVE_SPELL
		{
		  // All choices are targeted, so no need for a first pass to check for countering like Cryptic Command, Incendiary Command, and Primal Command.
		  int choice, tgt = 0, selected;
		  for (choice = 0; choice < 2; ++choice)
			switch (choices[choice])
			  {
				case CHOICE_LOSE_LIFE:
				  if (validate_target(player, card, &td_player, tgt))
					lose_life(instance->targets[tgt].player, x);
				  ++tgt;
				  break;

				case CHOICE_REANIMATE:
				  selected = validate_target_from_grave(player, card, player, 18);
				  if (selected != -1)
					reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				  break;

				case CHOICE_WEAKEN:
				  if (validate_target(player, card, &td_creature_to_weaken, tgt))
					pump_until_eot(player, card, instance->targets[tgt].player, instance->targets[tgt].card, -x, -x);
				  ++tgt;
				  break;

				case CHOICE_FEAR:
				  // Always the second choice, since they're sorted; so all targets from tgt to instance->number_of_targets - 1 are to gain fear.
				  ASSERT(choice == 1);
				  for (; tgt < instance->number_of_targets; ++tgt)
					if (validate_target(player, card, &td_creature_to_fear, tgt))
					  pump_ability_until_eot(player, card, instance->targets[tgt].player, instance->targets[tgt].card, 0, 0, 0, SP_KEYWORD_FEAR);
				  break;
			  }

		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_prowess_of_the_fair(int player, int card, event_t event){
	/* Prowess of the Fair	|1|B
	 * Tribal Enchantment - Elf
	 * Whenever another nontoken Elf is put into your graveyard from the battlefield, you may put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.subtype = SUBTYPE_ELF;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if(	resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		generate_tokens_by_id(player, card, CARD_ID_ELF_WARRIOR, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_purity(int player, int card, event_t event){

	/* Purity	|3|W|W|W
	 * Creature - Elemental Incarnation 6/6
	 * Flying
	 * If noncombat damage would be dealt to you, prevent that damage. You gain life equal to the damage prevented this way.
	 * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	card_instance_t* damage = noncombat_damage_being_prevented(event);
	if( damage &&
		damage->damage_target_player == player &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  ){
		int amount = damage->info_slot;
		damage->info_slot = 0;
		gain_life(player, amount);
	}

	return 0;
}

// elemental shaman token --> rhino token

int card_rebellion_of_the_flamekin(int player, int card, event_t event){
	return global_enchantment(player, card, event);
}

int card_rootgrapple(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_TREEFOLK) ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target noncreature permanent.", 1, NULL);
}

int card_runed_stalactite(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 1, 1, KEYWORD_PROT_INTERRUPTS, 0);
}

int card_secluded_glen(int player, int card, event_t event){

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_FAERIE);

	return mana_producer(player, card, event);
}

int card_scattering_stroke(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			if( clash(player, card) ){
				int legacy_card = create_legacy_effect(player, card, &effect_mana_drain );
				card_instance_t *legacy = get_card_instance(player, legacy_card);
				legacy->targets[0].card = instance->info_slot;
				if( current_turn == player && current_phase < PHASE_MAIN2 ){
					legacy->targets[0].player = PHASE_MAIN2;
				}
				else{
					legacy->targets[0].player = PHASE_MAIN1;
				}
			}
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_scion_of_oona(int player, int card, event_t event)
{
  /* Scion of Oona	|2|U
   * Creature - Faerie Soldier 1/1
   * Flash
   * Flying
   * Other Faerie creatures you control get +1/+1.
   * Other Faeries you control have shroud. */

  if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && affected_card_controller == player
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_FAERIE)
	  && is_what(affected_card_controller, affected_card, event == EVENT_ABILITIES ? TYPE_PERMANENT : TYPE_CREATURE)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && !affect_me(player, card) && !is_humiliated(player, card))
	{
	  if (event == EVENT_ABILITIES)
		event_result |= KEYWORD_SHROUD;
	  else
		event_result += 1;
	}

  return flash(player, card, event);
}

int card_seedguide_ash(int player, int card, event_t event){

	/* Seedguide Ash	|4|G
	 * Creature - Treefolk Druid 4/4
	 * When ~ dies, you may search your library for up to three |H2Forest cards and put them onto the battlefield tapped. If you do, shuffle your library. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		this_test.qty = 3;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
		shuffle(player);
	}

	return 0;
}

int card_shapesharer(int player, int card, event_t event){

	/* Shapesharer	|1|U
	 * Creature - Shapeshifter 1/1
	 * Changeling
	 * |2|U: Target Shapeshifter becomes a copy of target creature until your next turn. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_SHAPESHIFTER;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(2, 1), 0, &td, NULL) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if (!(charge_mana_for_activated_ability(player, card, MANACOST_XU(2, 1)) &&
			  pick_next_target_noload(&td, "Select target Shapeshifter.") &&
			  pick_next_target_noload(&td1, "Select target creature to copy."))
		   ){
			instance->number_of_targets = 0;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) &&  validate_target(player, card, &td1, 1) ){
			shapeshift_target(instance->parent_controller, instance->parent_card,
							  instance->targets[0].player, instance->targets[0].card,
							  instance->targets[1].player, instance->targets[1].card,
							  SHAPESHIFT_UNTIL_EOT);
		}
	}

	return 0;
}

int card_shelldock_isle(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_U(2)) && instance->targets[2].player > -1 &&
			(count_deck(player) < 21 || count_deck(1-player) < 21)
		  ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Play the exiled card\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, MANACOST_U(1));
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	return hideaway(player, card, event);
}

int card_shields_of_velis_vel(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_subtype_until_eot(player, card, instance->targets[0].player, -1, 0, 1, KEYWORD_PROT_INTERRUPTS, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_shriekmaw(int player, int card, event_t event){
	fear(player, card, event);

	if( comes_into_play(player, card, event) ){
		card_instance_t *instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		td.illegal_type = TYPE_ARTIFACT;
		td.allow_cancel = 0;
		if( can_target(&td) &&
			new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%, nonartifact creature", COLOR_BLACK), 0, GS_LITERAL_PROMPT)
		  ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}

		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return evoke(player, card, event, MANACOST_XB(1, 1));
}

int card_silvergill_adept(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		draw_a_card(player);
		return 0;
	}
	return buddy_creature(player, card, event, SUBTYPE_MERFOLK);
}

int card_silvergill_douser(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) &&
						(has_subtype(player, count, SUBTYPE_MERFOLK) || has_subtype(player, count, SUBTYPE_FAERIE))
					  ){
						amount++;
					}
					count++;
			}
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -amount, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_skeletal_changeling(int player, int card, event_t event){
	return regeneration(player, card, event, MANACOST_XB(1, 1));
}

int card_soulbright_flamekin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		if( parent->targets[1].player < 0 ){
			parent->targets[1].player = 0;
		}
		parent->targets[1].player++;
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, KEYWORD_TRAMPLE, 0);
		if( parent->targets[1].player == 3 ){
			produce_mana(player, COLOR_RED, 8);
		}
	}
	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(2), 0, &td, "Select target creature you control.");
}

int card_sower_of_temptation(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 25) ){ return 0; }

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED);
			}
		}
	}

	return 0;
}

int card_spellstutter_sprite(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL) ){
			return 99;
		}
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( current_turn == player ){
			ai_modifier -= 10;
		}
		if( generic_spell(player, card, EVENT_CAN_CAST, GS_COUNTERSPELL, NULL, NULL, 1, NULL) ){
			return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
		}
	}

	if( comes_into_play(player, card, event) ){
		if( instance->targets[0].player > -1 && instance->targets[0].card > -1 ){
			if( counterspell_validate(player, card, NULL, 0) ){
				if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_FAERIE) >= get_cmc(instance->targets[0].player, instance->targets[0].card) ){
					real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
	}

	return 0;
}

int card_spinerock_knoll(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_R(2)) && instance->targets[2].player > -1 &&
			get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) > 6
		  ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Play the exiled card\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, MANACOST_R(1));
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	return hideaway(player, card, event);
}

int card_spring_cleaning(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( clash(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_springleaf_drum(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) && !(event == EVENT_COUNT_MANA && affect_me(player, card)) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_target(&td) ){
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE){
		if( select_target(player, card, &td, "Choose an untapped creature you control.", NULL)){
			instance->number_of_targets = 1;
			produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1);
			if (cancel != 1){
				// It's the artifact that's tapping for mana, so preserve tapped_for_mana_color.
				int prev = tapped_for_mana_color;
				tap_card(instance->targets[0].player, instance->targets[0].card);
				tapped_for_mana_color = prev;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_target(&td) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);
	}
	return 0;
}

int card_squeaking_pie_sneak(int player, int card, event_t event){
	fear(player, card, event);
	return buddy_creature(player, card, event, SUBTYPE_GOBLIN);
}

int card_stinkdrinker_daredevil(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && has_subtype(affected_card_controller, affected_card, SUBTYPE_GIANT) &&
		! is_humiliated(player, card)
	  ){
		COST_COLORLESS-=2;
	}

	return 0;
}

int card_stonybrook_angler(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			twiddle(player, card, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}


int card_summon_the_school(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_MERFOLK_WIZARD, &token);
		token.pow = token.tou = 1;
		token.qty = 2;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	if(event == EVENT_GRAVEYARD_ABILITY){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.required_subtype = SUBTYPE_MERFOLK;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.illegal_state = TARGET_STATE_TAPPED;
		if( target_available(player, card, &td) > 3 ){
			return GA_RETURN_TO_HAND;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.required_subtype = SUBTYPE_MERFOLK;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.illegal_state = TARGET_STATE_TAPPED;
		if( tapsubtype_ability(player, card, 4, &td) ){
			return GAPAID_REMOVE;
		}
		else{
			spell_fizzled = 1;
		}
	}

	return basic_spell(player, card, event);
}

int card_sunrise_sovereign(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_GIANT, 2, 2, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY);
	return 0;
}

int card_surge_of_thoughtweft(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
		if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_KITHKIN) ){
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}


int card_surgespanner(int player, int card, event_t event){
	if( event == EVENT_TAP_CARD && affect_me(player, card) && ! is_humiliated(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) ){
			charge_mana_multi(player, MANACOST_XU(1, 1));
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return 0;
}

int card_sygg_river_guide(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_MERFOLK;
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
		pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, keyword, 0);
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(1, 1), 0, &td, "Select target Merfolk you control.");
}

// Sylvan echoes --> Rebellion of the flamekin

// Tarfire --> Shock

int card_tar_pitcher( int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	if(event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a Goblin to sacrifice.");
			test.subtype = SUBTYPE_GOBLIN;
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
	}

	return 0;
}

int card_thorn_of_amethyst(int player, int card, event_t event)
{
	if(event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
		if(! (card_d->type & TYPE_LAND) && ! (card_d->type & TYPE_CREATURE)){
			COST_COLORLESS++;
		}
	}
	return 1;
}

int card_thoughtseize(int player, int card, event_t event){

	/* Thoughtseize	|B
	 * Sorcery
	 * Target player reveals his or her hand. You choose a nonland card from it. That player discards that card. You lose 2 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier += 48 * (hand_count[1-player] - 1);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
			lose_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_thoughtweft_trio(int player, int card, event_t event){

	vigilance(player, card, event);

	creature_can_block_additional(player, card, event, 255);

	return champion(player, card, event, SUBTYPE_KITHKIN, -1);
}

int card_thousand_year_elixir(int player, int card, event_t event){

	/* Thousand-Year Elixir	|3
	 * Artifact
	 * You may activate abilities of creatures you control as though those creatures had haste.
	 * |1, |T: Untap target creature. */

	if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	   ){
		set_special_flags2(affected_card_controller, affected_card, SF2_THOUSAND_YEAR_ELIXIR);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}

int card_thundercloud_shaman(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_GIANT);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_GIANT;
		this_test.subtype_flag = 1;
		new_damage_all(player, card, 2, amount, 0, &this_test);
	}

	return 0;
}

int card_thieving_sprites(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) && pick_target(&td, "TARGET_PLAYER") ){
			int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_FAERIE);

			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);
			ec.cards_to_reveal = amount;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_effect_coercion(&ec, &this_test);
		}
	}

	return 0;
}

int card_timber_protector(int player, int card, event_t event)
{
  /* Timber Protector	|4|G
   * Creature - Treefolk Warrior 4/6
   * Other Treefolk creatures you control get +1/+1.
   * Other Treefolk and |H1Forests you control are indestructible. */

  if (event == EVENT_ABILITIES && affected_card_controller == player && affected_card != card
	  && (has_subtype(affected_card_controller, affected_card, SUBTYPE_TREEFOLK)
		  || has_subtype(affected_card_controller, affected_card, SUBTYPE_FOREST))
	  && in_play(player, card) && !is_humiliated(player, card))
	indestructible(affected_card_controller, affected_card, event);

  return boost_creature_type(player, card, event, SUBTYPE_TREEFOLK, 1, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_treefolk_harbinger(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, get_hacked_land_text(player, card, "Select a Treefolk or %s card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		if (!IS_AI(player) || basiclandtypes_controlled[player][COLOR_ANY] >= 3){
			this_test.sub2 = SUBTYPE_TREEFOLK;
			this_test.subtype_flag = F2_MULTISUBTYPE;
		}
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
	}

	if( event == EVENT_POW_BOOST && check_battlefield_for_id(ANYBODY, CARD_ID_DORAN_THE_SIEGE_TOWER) ){
		return get_toughness(player, card) - get_power(player, card);
	}

	return 0;
}

int card_turtleshell_changeling(int player, int card, event_t event)
{
  // Changeling
  changeling_switcher(player, card, event);

  // |1|U: Switch ~'s power and toughness until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XU(1,1), 0, NULL, NULL);
}

int card_vigor(int player, int card, event_t event){

	/* Vigor	|3|G|G|G
	 * Creature - Elemental Incarnation 6/6
	 * Trample
	 * If damage would be dealt to a creature you control other than ~, prevent that damage. Put a +1/+1 counter on that creature for each 1 damage prevented
	 * this way.
	 * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_target_player == player && damage->info_slot > 0 &&
				damage->damage_target_card != card
			  ){
				int amount = damage->info_slot;
				damage->info_slot = 0;
				add_1_1_counters(damage->damage_target_player, damage->damage_target_card, amount);
			}
		}
	}

	return 0;
}

int card_vivid_creek(int player, int card, event_t event){
	/* Vivid Crag	""
	 * Land
	 * ~ enters the battlefield tapped with two charge counters on it.
	 * |T: Add |R to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */
	return vivid_land(player, card, event, COLOR_BLUE, COLOR_TEST_BLUE);
}

int card_vivid_crag(int player, int card, event_t event){
	/* Vivid Creek	""
	 * Land
	 * ~ enters the battlefield tapped with two charge counters on it.
	 * |T: Add |U to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */
	return vivid_land(player, card, event, COLOR_RED, COLOR_TEST_RED);
}

int card_vivid_meadow(int player, int card, event_t event){
	/* Vivid Meadow	""
	 * Land
	 * ~ enters the battlefield tapped with two charge counters on it.
	 * |T: Add |W to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */
	return vivid_land(player, card, event, COLOR_WHITE, COLOR_TEST_WHITE);
}

int card_vivid_grove(int player, int card, event_t event){
	/* Vivid Grove	""
	 * Land
	 * ~ enters the battlefield tapped with two charge counters on it.
	 * |T: Add |G to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */
	return vivid_land(player, card, event, COLOR_GREEN, COLOR_TEST_GREEN);
}

int card_vivid_marsh(int player, int card, event_t event){
	/* Vivid Marsh	""
	 * Land
	 * ~ enters the battlefield tapped with two charge counters on it.
	 * |T: Add |B to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */
	return vivid_land(player, card, event, COLOR_BLACK, COLOR_TEST_BLACK);
}

int card_wanderwine_hub(int player, int card, event_t event){

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_MERFOLK);

	return mana_producer(player, card, event);
}

int card_warspike_changeling(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_R(1), 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_wanderwine_prophet(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) &&
		can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_MERFOLK, 0, 0, 0, 0, 0, -1, 0)
	  ){
		if( sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, SUBTYPE_MERFOLK, 0, 0, 0, 0, 0, -1, 0) ){
			time_walk_effect(player, card);
		}
	}
	return champion(player, card, event, SUBTYPE_MERFOLK, -1);
}

int card_warren_pilfeners(int player, int card, event_t event){

	/* Warren Pilferers	|4|B
	 * Creature - Goblin Rogue 3/3
	 * When ~ enters the battlefield, return target creature card from your graveyard to your hand. If that card is a Goblin card, ~ gains haste until end of turn. */

	if( comes_into_play(player, card, event) ){
		if( has_dead_creature(player) && ! graveyard_has_shroud(2) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");

			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_CREATURE);
			this_test2.subtype = SUBTYPE_GOBLIN;

			int result = -1;

			if( IS_AI(player) && new_special_count_grave(player, &this_test2) > 0 ){
				result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test2);
			}
			else{
				result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
			if( result > -1 && has_subtype(player, result, SUBTYPE_GOBLIN) ){
				give_haste(player, card);
			}
		}
	}

	return 0;
}

int card_whispmare(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( evoked(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return evoke(player, card, event, 0, 0, 0, 0, 0, 1);
}

int card_windbrisk_heights(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana(player, COLOR_WHITE, 2) && instance->targets[2].player > -1 && instance->targets[3].card >= 3 ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Play the exiled card\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana(player, COLOR_WHITE, 1);
				if( spell_fizzled != 1 ){
					instance->targets[1].player = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	// count attackers
	if( event == EVENT_DECLARE_ATTACKERS && current_turn == player ){
		// Calculate number of attacking creatures
		if (instance->targets[3].card < 3){
			if (instance->targets[3].card < 0){
				instance->targets[3].card = 0;
			}
			int count;
			for (count = 0; count < active_cards_count[player]; ++count){
				if (in_play_and_attacking(player, count) && ++instance->targets[3].card >= 3){
					break;
				}
			}
		}
	}

	// end of turn = no attackers
	if( event == EVENT_CLEANUP ){
		instance->targets[3].card = 0;
	}

	return hideaway(player, card, event);
}

int card_wings_of_velis_vel(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			set_pt_and_abilities_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4, KEYWORD_FLYING | KEYWORD_PROT_INTERRUPTS, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_wizened_cenn(int player, int card, event_t event){
	/*
	  Wizened Cenn |W|W
	  Creature - Kithkin Cleric 2/2
	  Other Kithkin creatures you control get +1/+1.
	*/
	boost_creature_type(player, card, event, SUBTYPE_KITHKIN, 1, 1, 0, BCT_CONTROLLER_ONLY);
	return 0;
}

// woodland changeling --> vanilla

int card_wort_boggart_auntie(int player, int card, event_t event){

	/* Wort, Boggart Auntie	|2|B|R
	 * Legendary Creature - Goblin Shaman 3/3
	 * Fear
	 * At the beginning of your upkeep, you may return target Goblin card from your graveyard to your hand. */

	check_legend_rule(player, card, event);

	fear(player, card, event);

	if( trigger_condition == TRIGGER_UPKEEP ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "");
		this_test.subtype = SUBTYPE_GOBLIN;

		int can_tutor = new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player);

		upkeep_trigger_ability_mode(player, card, event, player, can_tutor ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select target Goblin card.");
		this_test.subtype = SUBTYPE_GOBLIN;

		if( new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_wren_run_packmaster(int player, int card, event_t event){
	/* Wren's Run Packmaster	|3|G
	 * Creature - Elf Warrior 5/5
	 * Champion an Elf
	 * |2|G: Put a 2/2 |Sgreen Wolf creature token onto the battlefield.
	 * Wolves you control have deathtouch. */

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			generate_token_by_id(player, card, CARD_ID_WOLF);
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_XG(2, 1), 0, NULL, NULL);
	}

	// Deathtouch for Wolves
	if (event == EVENT_ABILITIES
		&& affected_card_controller == player && has_subtype(affected_card_controller, affected_card, SUBTYPE_WOLF)
		&& !is_humiliated(player, card)
	   ){
		deathtouch(affected_card_controller, affected_card, event);
	}

	return champion(player, card, event, SUBTYPE_ELF, -1);
}

int card_wrens_run_vanquisher(int player, int card, event_t event){
	deathtouch(player, card, event);
	return buddy_creature(player, card, event, SUBTYPE_ELF);
}

int card_wydwen_the_biting_gale( int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION && in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_BU(1, 1), 1, NULL, NULL);
	}

	return flash(player, card, event);
}


