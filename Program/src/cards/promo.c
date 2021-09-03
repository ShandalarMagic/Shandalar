#include "manalink.h"

// Cards originally printed as promos; Astral cards

static int ai_choice_for_arena(int player, int card, target_t* ret_tgt){
	card_instance_t* instance;
	int c, best_rating = -1;
	uint32_t prots = get_protections_from(player, card);

	ret_tgt->player = ret_tgt->card = -1;
	for (c = 0; c < active_cards_count[AI]; ++c){
		if (in_play(AI, c)
			&& cards_data[(instance = get_card_instance(AI, c))->internal_card_id].type & TYPE_CREATURE
			&& instance->kill_code != KILL_SACRIFICE	// How would it even get here?
			&& !(instance->state & STATE_CANNOT_TARGET)){
			/* Straight out of the exe.  There's definitely room for improvement here.
			 * For starters, if the human player chose first, the chosen creature should be taken into account.  This will always pick a
			 *    vanilla 4/4 creature over either a 6/1 or a 1/6, even when fighting a known 5/5.
			 * Most abilities, as well as EA_MANA_SOURCE and EA_ACT_ABILITY, should make the AI less likely to pick a given creature.
			 * Protections are an exception.  Especially if the human player's picked and the candidate has protection from that creature.
			 * Base card value should also be looked at. */
			uint32_t abils = get_abilities(AI, c, EVENT_ABILITIES, -1);
			if (abils & prots){
				continue;
			}
			int rating = (get_abilities(AI, c, EVENT_POWER, -1)
						  + get_abilities(AI, c, EVENT_TOUGHNESS, -1)
						  + num_bits_set(abils)
						  + num_bits_set(abils & (KEYWORD_FLYING | KEYWORD_FIRST_STRIKE))
						  + num_bits_set(cards_data[instance->internal_card_id].extra_ability & (EA_MANA_SOURCE | EA_ACT_ABILITY)));

			if (best_rating < rating){
				best_rating = rating;
				ret_tgt->player = AI;
				ret_tgt->card = c;
			}
		}
	}

  return best_rating >= 0;
}

int card_arena(int player, int card, event_t event){
	/* Arena	""
	 * Land
	 * |3, |T: Tap target creature you control and target creature of an opponent's choice he or she controls. Those creatures fight each other. */

	if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		ai_modifier += 48;
		return 0;
	}

	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
		return 0;
	}

	if (event != EVENT_CAN_ACTIVATE && event != EVENT_ACTIVATE && event != EVENT_RESOLVE_ACTIVATION){
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td_me, td_opp;

	default_target_definition(player, card, &td_me, TYPE_CREATURE);
	td_me.allowed_controller = td_me.preferred_controller = player;

	default_target_definition(player, card, &td_opp, TYPE_CREATURE);
	td_opp.allowed_controller = td_opp.preferred_controller = td_opp.who_chooses = 1 - player;
	td_opp.allow_cancel = 0;

	if (event == EVENT_CAN_ACTIVATE){
		return (!is_tapped(player, card) && !is_animated_and_sick(player, card) && !paying_mana()
				&& can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(3))
				&& can_target(&td_me) && can_target(&td_opp));
	}

	if (event == EVENT_ACTIVATE){
		if (paying_mana()){
			cancel = 1;
			return 0;
		}

		charge_mana_for_activated_ability(player, card, MANACOST_X(3));

		if (cancel != 1){
			instance->number_of_targets = 0;

			if (player == HUMAN){
				if (!pick_target(&td_me, "ARENA")){
					cancel = 1;
					return 0;
				}

				instance->state |= STATE_TAPPED;
				if (trace_mode & 2){
					((int (*)(int, int))(0x4720E0))(player, 0);	// networking
					new_pick_target(&td_opp, "ARENA", 1, 0);
				} else {
					if (!ai_choice_for_arena(player, card, &instance->targets[1])){
						// shouldn't be possible currently, but in case validate_target()'s definition drifts
						new_pick_target(&td_opp, "ARENA", 1, 0);
					}
					instance->number_of_targets = 2;
				}
				do_dialog(player, player, card, instance->targets[1].player, instance->targets[1].card, text_lines[1] /*"Selected for Arena"*/, 0);
			} else {	// player == AI
				if (trace_mode & 2){
					cancel = pick_target(&td_me, "ARENA") ? 0 : 1;
				} else {
					if (ai_choice_for_arena(player, card, &instance->targets[0])){
						instance->number_of_targets = 1;
						cancel = 0;
					} else {
						// shouldn't be possible currently, but in case validate_target()'s definition drifts
						cancel = pick_target(&td_opp, "ARENA") ? 0 : 1;
					}
				}

				if (cancel != 1){
					instance->state |= STATE_TAPPED;

					do_dialog(player, player, card, instance->targets[0].player, instance->targets[0].card, text_lines[1] /*"Selected for Arena"*/, 0);
					new_pick_target(&td_opp, "ARENA", 1, 0);

					if (trace_mode & 2){
						((int (*)(int, int))(0x4720E0))(1 - player, 0);	// networking
					}
				}
			}

			if (cancel == 1){
				instance->number_of_targets = 0;
			}
		}

		return 0;
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		int target_0_valid = validate_target(player, card, &td_me, 0);
		int target_1_valid = validate_target(player, card, &td_opp, 1);

		if (target_0_valid && target_1_valid){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			if (in_play(instance->targets[0].player, instance->targets[0].card)){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			if (in_play(instance->targets[1].player, instance->targets[1].card)){
				tap_card(instance->targets[1].player, instance->targets[1].card);
			}
		}
		/* Otherwise, if one target one was valid, the exe version still tapped it and did damage to it.  This contradicts ruling 9/25/2006: "If either target
		 * is illegal at the time the ability resolves (whether because it has left the battlefield, because it has gained Shroud or Protection, or for any
		 * other reason), the ability taps the remaining target, but no damage is dealt. If both targets are illegal, the ability is countered." */
		else if (target_0_valid){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		} else if (target_1_valid){
			tap_card(instance->targets[1].player, instance->targets[1].card);
		} else {
			spell_fizzled = 1;
		}

		get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
		return 0;
	}

	return 0;
}

int card_aswan_jaguar(int player, int card, event_t event)
{
  /* Aswan Jaguar	|1|G|G
   * Creature - Cat 2/2
   * When ~ comes into play, choose a random creature subtype from those in target opponent's deck.
   * |G|G, |T: Destroy target creature of the chosen subtype. It can't be regenerated. */

  if (comes_into_play(player, card, event) && target_opponent(player, card))
	{
	  // Equal chance for each creature type present in opponent's library, no matter how many times it appears, or whether it's on a creature or a Tribal.

	  char creature_type_is_present[SUBTYPE_MAX_USED_CREATURE_SUBTYPE + 1] = {0};

	  int* deck = deck_ptr[1-player];
	  int i, j;
	  for (i = 0; i < 500 && deck[i] != -1; ++i)
		{
		  int16_t* types = cards_ptr[cards_data[deck[i]].id]->types;
		  for (j = 0; j <= 5; ++j)	// ignore types[6], per has_new_types()
			if (types[j] >= SUBTYPE_MIN_CREATURE_SUBTYPE && types[j] <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE)
			  creature_type_is_present[types[j]] = 1;
		}

	  subtype_t creature_types_present[SUBTYPE_MAX_USED_CREATURE_SUBTYPE + 1];
	  int num_creature_types_present = 0;
	  for (i = 0; i <= SUBTYPE_MAX_USED_CREATURE_SUBTYPE; ++i)
		if (creature_type_is_present[i])
		  creature_types_present[num_creature_types_present++] = i;

	  card_instance_t* instance = get_card_instance(player, card);

	  if (num_creature_types_present == 0)
		instance->info_slot = 0;
	  else if (num_creature_types_present == 1)
		instance->info_slot = creature_types_present[0];
	  else if (player == 1 && (trace_mode & 2))
		{
		  EXE_FN(int, 0x4a70c0, int, int)(1, 26);	// receive_network_packet(1, 26)
		  instance->info_slot = EXE_DWORD(0x628C10);
		}
	  else
		{
		  instance->info_slot = creature_types_present[internal_rand(num_creature_types_present)];
		  if (trace_mode & 2)
			{
			  EXE_DWORD(0x628C10) = instance->info_slot;
			  EXE_BYTE(0x628C0C) = 26;
			  EXE_FN(int, 0x4a6bc0, int, int)(0, 26);	// send_network_packet(0, 26)
			}
		}

	  if (instance->info_slot > 0)
		create_subtype_name_legacy(player, card, instance->info_slot);
	}

  if (event == EVENT_GET_SELECTED_CARD)
	EXE_FN(void, 0x498FD0, int)(0);

  if (!IS_GAA_EVENT(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  if (instance->info_slot <= 0)
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_subtype = instance->info_slot;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  play_sound_effect(WAV_ASWANJAG);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_G(2), 0, &td,
								   get_subtype_text("Select target %s creature.", instance->info_slot));
}

int card_call_from_the_grave(int player, int card, event_t event){
	/*
	  Call from the Grave	|2|B
	  Sorcery
	  Put a random creature from a random graveyard into play under your control.
	  Call from the Grave deals to you an amount of damage equal to that creature's converted mana cost.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int corpses[500];
		int cc = 0;
		int i;
		for(i=0; i<2; i++){
			const int *grave = get_grave(i);
			int count = 0;
			while( grave[count] != -1 ){
					if( is_what(-1, grave[count], TYPE_CREATURE) ){
						corpses[cc] = count;
						if( i != player ){
							corpses[cc] |= (1<<31);
						}
						cc++;
					}
					count++;
			}
		}
		int rnd = internal_rand(cc);
		int t_player = corpses[rnd] & (1<<31) ? 1-player : player;
		int pos = corpses[rnd];
		if( t_player != player ){
			pos &= ~(1<<31);
		}
		int card_added = reanimate_permanent(player, card, t_player, pos, REANIMATE_DEFAULT);
		damage_player(player, get_cmc(player, card_added), player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_faerie_dragon(int player, int card, event_t event)
{
  // 0x4735C0

  /* Faerie Dragon	|2|G|G
   * Creature - Faerie Dragon 1/3
   * Flying
   * |1|G|G: Play a random effect. */

  target_t tgts[300]; // [sp+Ch] [bp-960h]@14

  if (event == EVENT_CAN_ACTIVATE)
	return can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XG(1,2));

  if (event == EVENT_GET_SELECTED_CARD)
	EXE_FN(void, 0x498FD0, int)(0);

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_XG(1,2)))
	{
	  int abil = (trace_mode & 2) ? EXE_FN(int, 0x44E070, int, int)(player, 20) : internal_rand(20);	// 0x44E070 either picks a random number and sends it over the network, or awaits a number of the network

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->info_slot = abil;

	  int numtgts = EXE_FN(int, 0x473370, int, int, target_t*)(player, card, tgts);	// picks random targets, sends/receives over network

	  int chosen = (trace_mode & 2) ? EXE_FN(int, 0x44E070, int, int)(player, numtgts) : internal_rand(numtgts);

	  instance->targets[0].player = tgts[chosen].player;
	  instance->targets[0].card = tgts[chosen].card;
	  instance->number_of_targets = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (!instance->number_of_targets)
		return 0;

	  // 0x473720

	  int abil = instance->info_slot;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;

	  if (!valid_target(&td))
		{
		  cancel = 1;
		  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
		  return 0;
		}

	  int tgt_player = instance->targets[0].player;
	  int tgt_card = instance->targets[0].card;

	  card_instance_t* tgt_inst = get_card_instance(tgt_player, tgt_card);

	  int effect_card = -1;

	  char buf[300];

	  if (abil >= 0 && abil < 20 && abil != 13 && abil != 1)
		{
		  load_text(0, "FAERIEDRAGON_MESSAGES");
		  scnprintf(buf, 300, "\n%s", text_lines[abil]);
		  do_dialog(player, player, card, tgt_player, tgt_card, buf, 0);
		  play_sound_effect(WAV_FAERDRAG);
		}

	  switch (abil)
		{
		  case 0:
			effect_card = create_legacy_effect_exe(player, card, LEGACY_EFFECT_GENERIC, tgt_player, tgt_card);
			if (effect_card != -1)
			  {
				card_instance_t* legacy = get_card_instance(player, effect_card);
				legacy->info_slot = KEYWORD_TRAMPLE;
				legacy->counter_power = tgt_inst->power;
				legacy->token_status |= STATUS_BERSERK;
			  }
			break;

		  case 1:
			if (tgt_inst->power >= 3)
			  {
				cancel = 1;
				load_text(0, "FAERIEDRAGON_TAWNOSWAND");
				sprintf(buf, "\n%s", text_lines[1]);
				do_dialog(player, player, card, tgt_player, tgt_card, buf, 0);
			  }
			else
			  {
				load_text(0, "FAERIEDRAGON_TAWNOSWAND");
				sprintf(buf, "\n%s", text_lines[0]);
				do_dialog(player, player, card, tgt_player, tgt_card, buf, 0);
				effect_card = pump_ability_until_eot(player, card, tgt_player, tgt_card, 0,0, 0,SP_KEYWORD_UNBLOCKABLE);
				play_sound_effect(WAV_FAERDRAG);
			  }
			break;

		  case 2:
			;int tgh = get_toughness(tgt_player, tgt_card);

			if (tgh >= 5)
			  tgh = 4;
			else
			  tgh = tgh - 1;

			effect_card = pump_until_eot(player, card, tgt_player, tgt_card, 4, -tgh);
			break;

		  case 3:
			// change_color_and_kill_other_change_color_effects
			EXE_FN(int, 0x401730, int, int, int, int, int)(tgt_player, tgt_card, get_sleighted_color_test(player, card, COLOR_TEST_GREEN), player, card);
			break;

		  case 4:
			// change_color_and_kill_other_change_color_effects
			EXE_FN(int, 0x401730, int, int, int, int, int)(tgt_player, tgt_card, get_sleighted_color_test(player, card, COLOR_TEST_WHITE), player, card);
			break;

		  case 5:
			// change_color_and_kill_other_change_color_effects
			EXE_FN(int, 0x401730, int, int, int, int, int)(tgt_player, tgt_card, get_sleighted_color_test(player, card, COLOR_TEST_RED), player, card);
			break;

		  case 6:
			damage_creature(tgt_player, tgt_card, 3, instance->parent_controller, instance->parent_card);
			break;

		  case 7:
			effect_card = pump_ability_until_eot(player, card, tgt_player, tgt_card, 0,0, KEYWORD_FLYING,0);
			break;

		  case 8:
			effect_card = pump_until_eot(player, card, tgt_player, tgt_card, 3,3);
			break;

		  case 9:
			effect_card = pump_ability_until_eot(player, card, tgt_player, tgt_card, 0,0, KEYWORD_BANDING,0);
			break;

		  case 10:
			// change_color_and_kill_other_change_color_effects
			EXE_FN(int, 0x401730, int, int, int, int, int)(tgt_player, tgt_card, get_sleighted_color_test(player, card, COLOR_TEST_BLACK), player, card);
			break;

		  case 11:
			// change_color_and_kill_other_change_color_effects
			EXE_FN(int, 0x401730, int, int, int, int, int)(tgt_player, tgt_card, get_sleighted_color_test(player, card, COLOR_TEST_BLUE), player, card);
			break;

		  case 12:
			effect_card = cannot_regenerate_until_eot(player, card, tgt_player, tgt_card);
			break;

		  case 13:
			load_text(0, "FAERIEDRAGON_TWIDDLE");
			scnprintf(buf, 300, "\n%s\n %s\n %s", text_lines, text_lines[1], text_lines[2]);
			if (do_dialog(player, player, card, tgt_player, tgt_card, buf, (tgt_inst->state & STATE_TAPPED) ? 0 : 1))
			  untap_card(tgt_player, tgt_card);
			else
			  tap_card(tgt_player, tgt_card);

			play_sound_effect(WAV_FAERDRAG);
			break;

		  case 14:
			effect_card = pump_until_eot(player, card, tgt_player, tgt_card, -2,0);
			break;

		  case 15:
			bounce_permanent(tgt_player, tgt_card);
			break;

		  case 16:
			damage_creature(tgt_player, tgt_card, 1, instance->parent_controller, instance->parent_card);
			break;

		  case 17:
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, tgt_player, tgt_card, 0, 2, 0, 0, 0);
			break;

		  case 18:
			gain_life(tgt_player, get_power(tgt_player, tgt_card));
			kill_card(tgt_player, tgt_card, KILL_REMOVE);
			break;

		  case 19:
			if (ai_is_speculating != 1)
			  {
				play_sound_effect(WAV_ORCART);
				EXE_STDCALL_FN(void, 0x4D5D32, int)(3000);	// Sleep(3000);
			  }
			++hack_silent_counters;
			add_counter(tgt_player, tgt_card, COUNTER_M0_M1);
			--hack_silent_counters;
			play_sound_effect(WAV_CATATAP);
			break;

		  default:
			load_text(0, "FAERIEDRAGON_ERROR");
			scnprintf(buf, 300, "\n%s", text_lines[0]);
			do_dialog(player, player, card, tgt_player, tgt_card, buf, 0);
			break;
		}

	  if (effect_card != -1)
		{
		  card_instance_t* legacy = get_card_instance(player, effect_card);
		  legacy->display_pic_int_id = EXE_DWORD_PTR(0x4EC234)[abil];	// The csvids of the mimicked effects
		  legacy->display_pic_num = get_card_image_number(legacy->display_pic_int_id, player, card);
		}

	  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
	}

  return 0;
}

int card_gem_bazaar(int player, int card, event_t event){
	/*
	  Gem Bazaar
	  When Gem Bazaar comes into play, choose a random color.
	  |T: Add to your mana pool one mana of the color last chosen. Then choose a random color.
	*/
	if( comes_into_play(player, card, event) ){
		play_sound_effect(WAV_GEMBAZAR);
		get_card_instance(player, card)->info_slot = 1<<(1+internal_rand(5));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		play_sound_effect(WAV_GEMBAZAR);
		card_instance_t *instance = get_card_instance(player, card);
		get_card_instance(instance->parent_controller, instance->parent_card)->info_slot = 1<<(1+internal_rand(5));
	}

	return mana_producer(player, card, event);
}

int card_giant_badger(int player, int card, event_t event){
	/*
	  Giant Badger |1|G|G
	  Creature - Badger 2/2
	  Whenever Giant Badger blocks, it gets +2/+2 until end of turn.
	*/
	if( ! is_humiliated(player, card) && blocking(player, card, event) ){
		int amount = 2*count_creatures_this_is_blocking(player, card);
		pump_until_eot(player, card, player, card, amount, amount);
	}
	return 0;
}

int card_goblin_polka_band(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = 2;

  int ai = player == AI || ai_is_speculating == 1;

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (!(can_use_activated_abilities(player, card)
			&& has_mana_for_activated_ability(player, card, MANACOST_XR(2, ai ? 1 : 0))
			&& !is_tapped(player, card)
			&& !is_sick(player, card)))
		return 0;

	  if (ai)
		{
		  // Only activate if opponent controls more potential targets than player.
		  td.allowed_controller = td.preferred_controller = 1-player;
		  int num_opp = target_available(player, card, &td);
		  td.allowed_controller = td.preferred_controller = player;
		  int num_self = target_available(player, card, &td);

		  if (num_opp == 0 || num_self >= num_opp)
			return 0;
		}

	  return 1;
	}

  if (event == EVENT_GET_SELECTED_CARD)
	{
	  EXE_FN(void, 0x498FD0, int)(1);
	  EXE_FN(void, 0x499010, int)(0);
	}

  if (event == EVENT_ACTIVATE)
	{
	  if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
		{
		  /* I... *think*... that 9 is the highest target number that hasn't been co-opted for local storage.  Which is pretty crazy, actually; we need to
		   * create parallel storage.  Shouldn't be too difficult - 1) clear it in create_card_instance(); 2) track down everywhere in the exe that memcpy()s a
		   * card_instance_t; some notable examples are start and end of ai speculation, display, and change of control; 3) rewrite saving and loading, which is
		   * overdue anyway. */
		  int old_max_x_value = max_x_value;
		  max_x_value = 10;

		  if (ai)
			{
			  max_x_value = target_available(player, card, &td);
			  if (max_x_value > 10)
				max_x_value = 10;
			}

		  charge_mana(player, COLOR_RED, -1);
		  max_x_value = old_max_x_value;
		  if (cancel != 1)
			{
			  instance->state |= STATE_TAPPED;

			  if (x_value <= 0)
				return 0;

			  // Choose the targets.  This is done during resolution in the original version.

			  target_t targets[500];
			  int p, c, end = 0, num = x_value;
			  for (p = 0; p < 2; ++p)
				{
				  instance->targets[0].player = p;
				  for (c = 0; c < active_cards_count[player]; ++c)
					{
					  instance->targets[0].card = c;
					  if (would_valid_target(&td))
						targets[end++] = instance->targets[0];	// struct copy
					}
				}

			  num = MIN(num, end);
			  if (num <= 0)
				return 0;

			  instance->number_of_targets = 0;
			  for (c = 0; c < num; ++c)
				{
				  int choice = internal_rand(end);
				  instance->targets[c] = targets[choice];	// struct copy
				  // Don't choose this target again
				  targets[choice] = targets[end - 1];	// struct copy
				  --end;
				  ++instance->number_of_targets;
				}
			}
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  play_sound_effect(WAV_POLKAMIX);	// EVERYBODY POLKA!  Ha ha ha!  Ho ho ho!

	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			if (has_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_GOBLIN))
			  does_not_untap_effect(player, instance->parent_card, instance->targets[i].player, instance->targets[i].card, EDNT_TAP_TARGET, 1);
			else
			  tap_card(instance->targets[i].player, instance->targets[i].card);
		  }
	}

  return 0;
}

int card_goblin_tutor(int player, int card, event_t event){
	/* Goblin Tutor	|R
	 * Instant
	 * Roll a six-sided die for ~. On a 1, ~ has no effect. Otherwise, search your library for the indicated card, reveal that card to all players, and put it
	 * into your hand. Shuffle your library afterwards.
	 * 2 - Any ~
	 * 3 - Any enchantment
	 * 4 - Any artifact
	 * 5 - Any creature
	 * 6 - Any sorcery, instant, or interrupt */

	if(event == EVENT_CAN_CAST){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
		int roll = internal_rand(6) + 1;
		int type = 0;
		if( roll == 1 ){
			do_dialog(player, player, card, -1, -1, "Rolled 1: no effect", 1);
		}
		else if( roll == 2 ){
			do_dialog(player, player, card, -1, -1, "Rolled 2: search for Goblin Tutor", 1);
		}
		else if( roll == 3 ){
			do_dialog(player, player, card, -1, -1, "Rolled 3: find an enchantment", 1);
			type = TYPE_ENCHANTMENT;
		}
		else if( roll == 4 ){
			do_dialog(player, player, card, -1, -1, "Rolled 4: find an artifact", 1);
			type = TYPE_ARTIFACT;
		}
		else if( roll == 5 ){
			do_dialog(player, player, card, -1, -1, "Rolled 5: find a creature", 1);
			type = TYPE_CREATURE;
		}
		else if( roll == 6 ){
			do_dialog(player, player, card, -1, -1, "Rolled 6: find an instant or sorcery", 1);
			type = TYPE_INSTANT | TYPE_SORCERY | TYPE_INTERRUPT;
		}

		if( roll > 2 ){
			if( type > 0 ){
				int mode = 0;
				if( roll == 6 ){
					mode = 2;
				}
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, type, mode, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 4, 0, 0, 0, 0, 0, 0, CARD_ID_GOBLIN_TUTOR, 0, -1, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mana_crypt(int player, int card, event_t event){
	/*
	  Mana Crypt 0
	  Artifact
	  At the beginning of your upkeep, flip a coin. If you lose the flip, Mana Crypt deals 3 damage to you.
	  {T}: Add {2} to your mana pool.
	*/
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( ! flip_a_coin(player, card) ){
			damage_player(player, 3, player, card);
		}
	}

	return card_sol_ring(player, card, event);
}

/*
Nalathni Dragon |2|R|R --> Dragon Whelp
Creature - Dragon 1/1
Flying; banding
{R}: Nalathni Dragon gets +1/+0 until end of turn.
	If this ability has been activated four or more times this turn, sacrifice Nalathni Dragon at the beginning of the next end step.
*/

// Spawn of Azar token --> rhino token

int card_necropolis_of_azar(int player, int card, event_t event)
{
  // 0x4bd380

  /* Necropolis of Azar	|2|B|B
   * Enchantment
   * Whenever a non|Sblack creature is put into any graveyard from play, put a husk counter on ~.
   * |5, Remove a husk counter from ~: Put a Spawn of Azar token into play. Treat this token as a |Sblack creature with a random power and toughness, each no
   * less than 1 and no greater than 3, that has |H2swampwalk. */

  if (event == EVENT_CAN_CAST)
	return 1;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		test.color_flag = DOESNT_MATCH;

		count_for_gfp_ability(player, card, event, ANYBODY, 0, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counters(player, card, COUNTER_HUSK, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

  if (event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_HUSK) <= 0)
	return 0;
  int rval = generic_activated_ability(player, card, event, 0, MANACOST_X(5), 0, NULL, NULL);
  if (event == EVENT_ACTIVATE && cancel != 1)
	remove_counter(player, card, COUNTER_HUSK);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  play_sound_effect(WAV_NECRAZAR);
	  int parent_controller = get_card_instance(player, card)->parent_controller;

	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SPAWN_OF_AZAR, &token);
	  if (trace_mode & 2)
		{
		  // 0x44E070 either picks a random number and sends it over the network, or awaits a number of the network
		  token.pow = 1 + EXE_FN(int, 0x44E070, int, int)(parent_controller, 3);
		  token.tou = 1 + EXE_FN(int, 0x44E070, int, int)(parent_controller, 3);
		}
	  else
		{
		  token.pow = 1 + internal_rand(3);
		  token.tou = 1 + internal_rand(3);
		}
	  generate_token(&token);
	}

  return rval;
}

int card_orcish_catapult(int player, int card, event_t event){
	/*
	  Randomly distribute |X -0/-1 counters among a random number of random target creatures.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		instance->info_slot = x_value;
		int max_trgs = 0;
		int trgs[2][18];
		int p;
		for(p=0; p<2; p++){
			int c;
			for(c = 0; c<active_cards_count[p]; c++){
				if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
					if( would_validate_arbitrary_target(&td, p, c) ){
						trgs[0][max_trgs] = p;
						trgs[1][max_trgs] = c;
						max_trgs++;
					}
					if( max_trgs == 18 ){
						break;
					}
				}
			}
			if( max_trgs == 18 ){
				break;
			}
		}
		if( max_trgs ){
			for(p=0; p<max_trgs; p++){
				if( internal_rand(10)+1 >= 5 ){
					int pos = instance->number_of_targets;
					instance->targets[pos].player = trgs[0][p];
					instance->targets[pos].card = trgs[1][p];
					instance->number_of_targets++;
				}
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i, amount = instance->info_slot, sound_played = 0;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				int cnters = 1;
				if( amount > 1 ){
					cnters = internal_rand(amount);
				}
				amount-=cnters;

				if (ai_is_speculating != 1 && !sound_played){
					play_sound_effect(WAV_ORCART);
					EXE_STDCALL_FN(void, 0x4D5D32, int)(3000);	// Sleep(3000);
				}
				++hack_silent_counters;
				add_counters(instance->targets[i].player, instance->targets[i].card, COUNTER_M0_M1, cnters);
				--hack_silent_counters;
				if (ai_is_speculating != 1 && !sound_played){
					sound_played = 1;
					play_sound_effect(WAV_CATATAP);
				}
			}
			if( amount < 1 ){
				break;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_pandoras_box(int player, int card, event_t event){
	/*
	  Pandora's Box	|5
	  |3, |T: Choose a random non-artifact creature card from all players' libraries.
	  For each player, flip a coin. If the flip ends up heads, put a token creature into play
	  and treat it as though an exact copy of the chosen creature card were just played.
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		play_sound_effect(WAV_PANDORA);
		int corpses[500];
		int cc = 0;
		int i;
		for(i=0; i<2; i++){
			int *deck = deck_ptr[i];
			int count = 0;
			while( deck[count] != -1 ){
					if( is_what(-1, deck[count], TYPE_CREATURE) && ! is_what(-1, deck[count], TYPE_ARTIFACT) ){
						corpses[cc] = deck[count];
						cc++;
					}
					count++;
			}
		}
		int revealed_by_pandoras_box[1] = {corpses[internal_rand(cc)]};
		show_deck( player, revealed_by_pandoras_box, 1, "Pandora's Box chosen...", 0, 0x7375B0 );

		token_generation_t token;
		default_token_definition(player, card, cards_data[revealed_by_pandoras_box[0]].id, &token);
		token.no_sleight = 1;

		for(i=0; i<2; i++){
			if( player_flips_a_coin(player, card, i) ){
				token.t_player = i;
				generate_token(&token);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL);
}

int card_power_struggle(int player, int card, event_t event){
	/*
	  Power Struggle	|2|U|U|U
	  At the beginning of each player's upkeep, that player exchanges control of random target artifact, creature or land he or she controls,
	  for control of random target permanent of the same type that a random opponent controls.
	*/
	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT | TYPE_LAND);
		int players_permanents[active_cards_count[player]];
		int ppc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && is_what(player, i, TYPE_CREATURE | TYPE_ARTIFACT | TYPE_LAND) ){
				if( would_validate_arbitrary_target(&td, player, i) ){
					players_permanents[ppc] = i;
					ppc++;
				}
			}
		}
		if( ppc ){
			int my_permanent_to_give = players_permanents[ppc];
			if( ppc > 1 ){
				my_permanent_to_give = players_permanents[internal_rand(ppc)];
			}
			int type = get_type(player, my_permanent_to_give);
			int opp_permanents[active_cards_count[1-player]];
			int opc = 0;
			for(i=0; i<active_cards_count[1-player]; i++){
				if( in_play(1-player, i) && is_what(1-player, i, type) ){
					if( would_validate_arbitrary_target(&td, 1-player, i) ){
						opp_permanents[opc] = i;
						opc++;
					}
				}
			}
			if( opc ){
				exchange_control_of_target_permanents(player, card, player, my_permanent_to_give, 1-player, opp_permanents[internal_rand(opc)]);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_prismatic_dragon(int player, int card, event_t event){
	/*
	  Prismatic Dragon	|2|W|W
	  Creature - Dragon
	  Flying
	  At the beginning of your upkeep, Prismatic Dragon becomes a random color permanently.
	  |2: Prismatic Dragon becomes a random color permanently.
	*/
	if( ! is_humiliated(player, card) ){
		if( current_turn == player && upkeep_trigger(player, card, event) ){
			get_card_instance(player, card)->info_slot = 1<<(1+internal_rand(5));
		}

		if( event == EVENT_SET_COLOR && affect_me(player, card) && get_card_instance(player, card)->info_slot > 0){
			event_result = get_card_instance(player, card)->info_slot;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		play_sound_effect(WAV_PRSMDRAG);
		card_instance_t *instance = get_card_instance(player, card);
		get_card_instance(instance->parent_controller, instance->parent_card)->info_slot = 1<<(1+internal_rand(5));
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
}

static void sewers_of_estark_prevent_damage_effect(int player, int card, int t_player, int t_card){
	prevent_all_damage_dealt_by_target(player, card, t_player, t_card, DDBM_MUST_BE_COMBAT_DAMAGE);
}

int card_rainbow_knights(int player, int card, event_t event){
	/*
	  Rainbow Knights	|W|W
	  When Rainbow Knights comes into play, it gains protection from a random color permanently.
	  |1: First strike until end of turn.
	  |W|W: +0/+0, +1/+0, or +2/+0 until end of turn, chosen at random.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( comes_into_play(player, card, event) ){
			get_card_instance(player, card)->info_slot = 1<<(11+internal_rand(5));
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			event_result |= get_card_instance(player, card)->info_slot;
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_W(2), 0, NULL, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"First strike", has_mana_for_activated_ability(player, card, MANACOST_X(1)), 15*(!check_for_ability(player, card, KEYWORD_FIRST_STRIKE)),
						"Random pump", has_mana_for_activated_ability(player, card, MANACOST_W(2)), 10);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XW(choice == 1, 2*(choice == 2))) ){
			instance->targets[1].player = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 1 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, 0, KEYWORD_FIRST_STRIKE);
		}
		if( instance->targets[1].player == 2 ){
			play_sound_effect(WAV_RAINBOWK);
			int rnd = internal_rand(3)+1;
			if( rnd < 3 ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, rnd, 0);
			}
		}
	}

	return 0;
}

int card_sewers_of_estark(int player, int card, event_t event){
	/*
	  Sewers of Estark
	  Instant
	  Choose target creature.
	  If it's attacking, it can't be blocked this turn.
	  If it's blocking, prevent all combat damage that would be dealt this combat by it and each creature it's blocking.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = player == AI ? (current_turn == player ? STATE_ATTACKING : STATE_BLOCKING) : 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( check_state(instance->targets[0].player, instance->targets[0].card, STATE_ATTACKING) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			}
			if( check_state(instance->targets[0].player, instance->targets[0].card, STATE_BLOCKING) ){
				prevent_all_damage_dealt_by_target(player, card, instance->targets[0].player, instance->targets[0].card, DDBM_MUST_BE_COMBAT_DAMAGE);
				for_each_creature_blocked_by_me(player, card, sewers_of_estark_prevent_damage_effect, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_whimsy(int player, int card, event_t event){
	return card_whimsy_exe(player, card, event);
}

/*
Windseeker Centaur |1|R|R --> Serra Angel
Creature - Centaur 2/2
Vigilance
*/

