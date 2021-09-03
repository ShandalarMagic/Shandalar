#include "manalink.h"

// Global functions
static int num_colors_paid(int player, int card, event_t event)
{
  /* TENTATIVE_mana_paid_for_spell[] at 0x55CEF0 (an int array) doesn't seem to always be set, and charge_spell_cost() (0x402680) sometimes overwrites
   * mana_paid[] by calling charge_mana() more than once.  The right numbers can probably be pulled from the TENTATIVE_mana_spent[] stack, but that's
   * significantly more work.  For now, charge the mana ourselves. */

  if (event == EVENT_MODIFY_COST && has_mana_to_cast_id(player, event, get_id(player, card)))
	null_casting_cost(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !played_for_free(player, card) && !is_token(player, card))
	{
	  char old_always_prompt_for_color = always_prompt_for_color;
	  always_prompt_for_color = 1;
	  int success = charge_mana_from_id(player, card, event, get_id(player, card));
	  always_prompt_for_color = old_always_prompt_for_color;

	  if (success)
		{
		  int colors = 0, col;
		  for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col)
			if (mana_paid[col] > 0)
			  ++colors;

		  return MAX(0, colors);
		}
	}

  return 0;
}

void store_num_colors_paid_in_info_slot(int player, int card, event_t event)
{
  int colors = num_colors_paid(player, card, event);
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	get_card_instance(player, card)->info_slot = colors;
  // battle_for_zendikar.c:card_skyrider_elf() depends on this happening exactly at EVENT_CAST_SPELL
}

#define USE_MANA_PAID_FOR_SUNBURST 1
#ifdef USE_MANA_PAID_FOR_SUNBURST
static void sunburst(int player, int card, event_t event, int ctype)
{
  /* 702.43. Sunburst
   * 702.43a Sunburst is a static ability that functions as an object is entering the battlefield from the stack. "Sunburst" means "If this object is entering
   * the battlefield from the stack as a creature, it enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it. If this object
   * is entering the battlefield from the stack and isn't entering the battlefield as a creature, it enters the battlefield with a charge counter on it for each
   * color of mana spent to cast it."
   * 702.43b Sunburst applies only as the spell is resolving and only if one or more colored mana was spent on its costs. Mana paid for additional or
   * alternative costs applies.
   * 702.43c Sunburst can also be used to set a variable number for another ability. If the keyword is used in this way, it doesn't matter whether the ability
   * is on a creature spell or on a noncreature spell.
   * Example: The ability "Modular -- Sunburst" means "This permanent enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it"
   * and "When this permanent is put into a graveyard from the battlefield, you may put a +1/+1 counter on target artifact creature for each +1/+1 counter on
   * this permanent."
   * 702.43d If an object has multiple instances of sunburst, each one works separately. */

  int colors = num_colors_paid(player, card, event);
  if (colors > 0)
	{
	  ++hack_silent_counters;
	  add_counters(player, card, ctype, colors);
	  --hack_silent_counters;
	}
}
#else
static int count_mana_for_sunburst(int player ){

	int i;
	int result = 0;
	for(i=1; i<6; i++){
		if( has_mana(player, i, 1) ){
			result++;
		}
	}
	return result;
}

static int charge_mana_for_sunburst(int player, int card, event_t event){

	int c1 = get_updated_casting_cost(player, card, -1, event, -1);
	int required = c1;
	if( required > 5 ){
		required = 5;
	}
	int number = 0;
	if( player == AI ){
		int i;
		for(i=1; i<6; i++){
			if( has_mana(player, i, 1) ){
				charge_mana(player, i, 1);
				if( spell_fizzled != 1 ){
					number++;
				}
			}
			if( number == required ){
				break;
			}
		}
		if( number < c1 ){
			charge_mana(player, COLOR_COLORLESS, c1-number);
			if( spell_fizzled == 1 ){
				return 0;
			}
		}
	}
	else{
		char buffer[500];
		int pos = scnprintf(buffer, 500, " Cancel\n");
		pos +=scnprintf(buffer+pos, 500-pos, " Pay %d\n", c1);
		int comb[100];
		comb[1] = c1;
		int combc = 2;
		test_definition_t this_test;
		int a;
		for(a=1; a<6; a++){
			int cl[6] = {0, 0, 0, 0, 0, 0};
			cl[a] = 1;
			if( has_mana_multi( player, c1-1, cl[1], cl[2], cl[3], cl[4], cl[5]) ){
				mana_into_string(c1-1, cl[1], cl[2], cl[3], cl[4], cl[5], &this_test);
				pos +=scnprintf(buffer+pos, 500-pos, " Pay %s\n", this_test.message);
				comb[combc] = c1-1;
				int q;
				for(q=1; q<6; q++){
					if( cl[q] == 1 ){
						comb[combc] |= 1<<(q+3);
					}
				}
				combc++;
			}
			if( required > 1 && a < 5 ){
				int b;
				for(b=a+1; b<6; b++){
					cl[b] = 1;
					if( has_mana_multi( player, c1-2, cl[1], cl[2], cl[3], cl[4], cl[5]) ){
						mana_into_string(c1-2, cl[1], cl[2], cl[3], cl[4], cl[5], &this_test);
						pos +=scnprintf(buffer+pos, 500-pos, " Pay %s\n", this_test.message);
						comb[combc] = c1-2;
						int q;
						for(q=1; q<6; q++){
							if( cl[q] == 1 ){
								comb[combc] |= 1<<(q+3);
							}
						}
						combc++;
					}
					if( required > 2 && a < 4 ){
						int c;
						for(c=b+1; c<6; c++){
							cl[c] = 1;
							if( has_mana_multi( player, c1-3, cl[1], cl[2], cl[3], cl[4], cl[5]) ){
								mana_into_string(c1-3, cl[1], cl[2], cl[3], cl[4], cl[5], &this_test);
								pos +=scnprintf(buffer+pos, 500-pos, " Pay %s\n", this_test.message);
								comb[combc] = c1-3;
								int q;
								for(q=1; q<6; q++){
									if( cl[q] == 1 ){
										comb[combc] |= 1<<(q+3);
									}
								}
								combc++;
							}
							if( required > 3 && a < 3 ){
								int d;
								for(d=c+1; d<6; d++){
									cl[d] = 1;
									if( has_mana_multi( player, c1-4, cl[1], cl[2], cl[3], cl[4], cl[5]) ){
										mana_into_string(c1-4, cl[1], cl[2], cl[3], cl[4], cl[5], &this_test);
										pos +=scnprintf(buffer+pos, 500-pos, " Pay %s\n", this_test.message);
										comb[combc] = c1-4;
										int q;
										for(q=1; q<6; q++){
											if( cl[q] == 1 ){
												comb[combc] |= 1<<(q+3);
											}
										}
										combc++;
									}
									if( required > 4 && a < 2 ){
										int e;
										for(e=d+1; e<6; e++){
											cl[e] = 1;
											if( has_mana_multi( player, c1-5, cl[1], cl[2], cl[3], cl[4], cl[5]) ){
												mana_into_string(c1-5, cl[1], cl[2], cl[3], cl[4], cl[5], &this_test);
												pos +=scnprintf(buffer+pos, 500-pos, " Pay %s\n", this_test.message);
												comb[combc] = c1-5;
												int q;
												for(q=1; q<6; q++){
													if( cl[q] == 1 ){
														comb[combc] |= 1<<(q+3);
													}
												}
												combc++;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		if( choice == 0 ){
			spell_fizzled = 1;
			return 0;
		}
		else{
			int cl[6] = {0, 0, 0, 0, 0, 0};
			int k;
			for(k=7; k>3; k--){
				if( comb[choice] & (1<<k) ){
					cl[k-3] = 1;
					comb[choice] &= ~(1<<k);
					number++;
				}
			}
			cl[0] = comb[choice];
			charge_mana_multi(player, cl[0], cl[1], cl[2], cl[3], cl[4], cl[5]);
			if( spell_fizzled == 1 ){
				return 0;
			}
		}
	}
	return number;
}

static void sunburst(int player, int card, event_t event, int ctype){

	/* 702.43. Sunburst
	 * 702.43a Sunburst is a static ability that functions as an object is entering the battlefield from the stack. "Sunburst" means "If this object is entering
	 * the battlefield from the stack as a creature, it enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it. If this
	 * object is entering the battlefield from the stack and isn't entering the battlefield as a creature, it enters the battlefield with a charge counter on it
	 * for each color of mana spent to cast it."
	 * 702.43b Sunburst applies only as the spell is resolving and only if one or more colored mana was spent on its costs. Mana paid for additional or
	 * alternative costs applies.
	 * 702.43c Sunburst can also be used to set a variable number for another ability. If the keyword is used in this way, it doesn't matter whether the ability
	 * is on a creature spell or on a noncreature spell.
	 * Example: The ability "Modular -- Sunburst" means "This permanent enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast
	 * it" and "When this permanent is put into a graveyard from the battlefield, you may put a +1/+1 counter on target artifact creature for each +1/+1 counter
	 * on this permanent."
	 * 702.43d If an object has multiple instances of sunburst, each one works separately. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
			null_casting_cost(player, card);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			int required = c->req_colorless;
			if( required > 5 ){
				required = 5;
			}
			if( count_mana_for_sunburst(player) < required ){
				ai_modifier-=100;
			}
		}
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			int number = charge_mana_for_sunburst(player, card, event);
			instance->info_slot = number;
			add_counters(player, card, ctype, instance->info_slot);
		}
	}
}
#endif

static int bringers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_MODIFY_COST ){
		if( has_mana_multi(player, 0, 1, 1, 1, 1, 1 ) ){
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			COST_COLORLESS-=c->req_colorless;
			COST_BLACK-=c->req_black;
			COST_BLUE-=c->req_blue;
			COST_GREEN-=c->req_green;
			COST_RED-=c->req_red;
			COST_WHITE-=c->req_white;
			instance->info_slot = 66;
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 66 ){
				card_ptr_t* c = cards_ptr[ get_id(player, card) ];
				int cless = get_updated_casting_cost(player, card, -1, event, c->req_colorless);
				int choice = 0;
				if( has_mana_multi(player, 0, 1, 1, 1, 1, 1 ) ){
					if( has_mana_multi(player, cless, c->req_black, c->req_blue, c->req_green,
										c->req_red, c->req_white)
					  ){
						choice = do_dialog(player, player, card, -1, -1, " Pay BUGRW\n Play normally\n Cancel", 0);
					}
				}
				else{
					choice = 1;
				}

				if( choice == 0 ){
					charge_mana_multi(player, 0, 1, 1, 1, 1, 1 );
				}
				else if(choice == 1){
						charge_mana_multi(player, cless, c->req_black, c->req_blue, c->req_green,
										  c->req_red, c->req_white);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	return 0;
}

static int can_activate_instant_equipment(int player, int card, event_t event, int equip_cost, int manacolor){
		int c[5] = {0, 0, 0, 0, 0};
		c[manacolor-1]+=2;
		if( has_mana_for_activated_ability(player, card, 0, c[0], c[1],c[2],c[3],c[4]) ){
			return check_for_equipment_targets(player, card);
		}

		if( can_sorcery_be_played(player, event) ){
			int nc = get_updated_equip_cost(player, card, equip_cost);
			if( has_mana(player, COLOR_COLORLESS, nc) ){
				return check_for_equipment_targets(player, card);
			}
		}
		return 0;
}

static int instant_equipment(int player, int card, event_t event, int equip_cost, int manacolor, int pow, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		if( s_key > 0 ){
			special_abilities(instance->targets[8].player, instance->targets[8].card, event, s_key, player, card);
		}
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, pow, tou, key);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_activate_instant_equipment(player, card, event, equip_cost, manacolor);
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			int ai_choice = 0;
			int nc = get_updated_equip_cost(player, card, equip_cost);
			int c[5] = {0, 0, 0, 0, 0};
			c[manacolor-1]+=2;
			if( has_mana(player, COLOR_COLORLESS, nc) && can_sorcery_be_played(player, event) ){
				if( has_mana_for_activated_ability(player, card, 0, c[0], c[1],c[2],c[3],c[4]) ){
					ai_choice = 1;
					if( nc < 2 ){
						ai_choice = 0;
					}
					choice = do_dialog(player, player, card, -1, -1, " Normal equip\n Instant equip\n Cancel", ai_choice);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else if( choice == 0 ){
					charge_mana(player, COLOR_COLORLESS, nc);
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 0, c[0], c[1],c[2],c[3],c[4]);
			}

			if( spell_fizzled != 1 ){
				activate_basic_equipment(player, card, -2);
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			resolve_activation_basic_equipment(player, card);
	}

	return 0;
}

static int effect_rite_of_passage(int player, int card, event_t event){
	if (event == EVENT_AFTER_DAMAGE){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(instance->damage_target_player, instance->damage_target_card)){
			add_1_1_counter(instance->damage_target_player, instance->damage_target_card);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	// just in case
	if (event == EVENT_CLEANUP && affect_me(player, card)){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

// Cards

int card_abunas_chant(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td.required_type = 0;

	enum{	CHOICE_PREVENT = 1,
			CHOICE_GAIN,
			CHOICE_ENTWINE
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, NULL, 1, NULL) ){
			return 99; // 0x63
		}
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int opts[3] = {	generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, NULL, 1, NULL),
						1,
						has_mana(player, COLOR_COLORLESS, 2)
		};
		int choice = choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						  "Prevent 5 damage", opts[0], 10,
						  "Gain 5 life", opts[1], life[player] < 6 ? 15 : 5,
						  "Entwine", opts[2], 20);
		if( ! choice  ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_ENTWINE ){
			charge_mana(player, COLOR_COLORLESS, 2);
		}

		if( choice & CHOICE_PREVENT){
			new_pick_target(&td, "Select target damage card that will damage a creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & CHOICE_GAIN ){
			gain_life(player, 5);
		}
		if( instance->info_slot & CHOICE_PREVENT ){
			if( valid_target(&td) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( target->info_slot <= 5 ){
					target->info_slot = 0;
				}
				else{
					target->info_slot-=5;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_acquire(int player, int card, event_t event){
	if( IS_GS_EVENT(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card");
		return steal_permanent_from_target_opponent_deck(player, card, event, &this_test);
	}
	return 0;
}

int card_all_suns_dawn(int player, int card, event_t event){

	/* All Suns' Dawn	|4|G
	 * Sorcery
	 * For each color, return up to one target card of that color from your graveyard to your hand. Exile ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( count_graveyard(player) > 0 ){
			return ! graveyard_has_shroud(2);
		}
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		const int *grave = get_grave(player);
		load_text(0, "COLORWORDS");
		test_definition_t this_test;
		int pos = 0;
		int doubles[5][2];

		int i;
		for(i=1; i<6; i++){
			char msg[100];
			scnprintf(msg, 100, " Select a %s card.", text_lines[i-1]);
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			this_test.color = 1<<i;
			this_test.id = 904;
			this_test.id_flag = DOESNT_MATCH;
			if( new_special_count_grave(player, &this_test) ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					instance->targets[pos].player = selected;
					instance->targets[pos].card = grave[selected];
					doubles[pos][0] = selected;
					doubles[pos][1] = turn_card_in_grave_face_down(player, selected);
					pos++;
				}
			}
		}
		if( pos > 0 ){
			for (i = 0; i < pos; i++){
				turn_card_in_grave_face_up(player, doubles[i][0], doubles[i][1]);
			}
			int k;
			for (i = 0; i < pos; i++){
				for (k = i; k < pos - 1; k++){
					if( instance->targets[i].player > instance->targets[k].player ){
						SWAP(instance->targets[k], instance->targets[i]);	// struct copy
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
		int i, selected;
		for(i=instance->info_slot-1; i>-1; i--){
			if( (selected = validate_target_from_grave(player, card, player, i)) != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_arcbound_wanderer(int player, int card, event_t event){
	/* Arcbound Wanderer	|6
	 * Artifact Creature - Golem 0/0
	 * Modular-Sunburst */
	modular(player, card, event, 0);
	sunburst(player, card, event, COUNTER_P1_P1);
	return 0;
}

int card_artificiers_intuition(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		this_test.zone = TARGET_ZONE_HAND;

		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card to discard.");
			this_test.zone = TARGET_ZONE_HAND;

			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card with CMC 1 or less.");
		this_test.cmc = 2;
		this_test.cmc_flag = 3;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_auriok_champion(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_DUH, &test))
		gain_life(player, 1);
	}

  return 0;
}


int card_auriok_salvagers(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select an artifact card with CMC 1 or less.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
	this_test.cmc = 2;
	this_test.cmc_flag = 3;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XW(1, 1), 0, NULL, NULL) ){
			if( new_special_count_grave(player, &this_test) > 0 ){
				return ! graveyard_has_shroud(2);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XW(1, 1)) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}
	return 0;
}

int card_auriok_windwalker(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.required_subtype = SUBTYPE_EQUIPMENT;
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
		return can_target(&td1);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( new_pick_target(&td, "Select target Equipment you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				if( new_pick_target(&td1, "Select target creature you control.", 1, 1 | GS_LITERAL_PROMPT) ){
					tap_card(player, card);
					instance->number_of_targets = 2;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			equip_target_creature(instance->targets[0].player, instance->targets[0].card,
								  instance->targets[1].player, instance->targets[1].card);
		}
	}

	return 0;
}

int card_avarice_totem(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) && in_play(instance->parent_controller, instance->parent_card) ){
			exchange_control_of_target_permanents(instance->parent_controller, instance->parent_card,
												  instance->parent_controller, instance->parent_card,
												  instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(5), 0, &td, "Select target permanent opponent controls.");
}

int card_battered_golem(int player, int card, event_t event){

	does_not_untap(player, card, event);

	if( is_tapped(player, card) && specific_cip(player, card, event, 2, RESOLVE_TRIGGER_AI(player), TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	return 0;
}

int card_beacon_of_creation(int player, int card, event_t event){
	/* Beacon of Creation	|3|G
	 * Sorcery
	 * Put a 1/1 |Sgreen Insect creature token onto the battlefield for each |H2Forest you control. Shuffle ~ into its owner's library. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_INSECT, count_subtype(player, TYPE_LAND, SUBTYPE_FOREST) );
		shuffle_into_library(player, card);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_beacon_of_destruction(int player, int card, event_t event){

 	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 5);
			shuffle_into_library(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_beacon_of_immortality(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = life[instance->targets[0].player];
			gain_life(instance->targets[0].player, amount);
			shuffle_into_library(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_beacon_of_tomorrows(int player, int card, event_t event){

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
			if( instance->targets[0].player == player ){
				time_walk_effect(player, card);
			}
			else{
				int fake = add_card_to_hand(instance->targets[0].player, get_original_internal_card_id(player, card));
				time_walk_effect(instance->targets[0].player, fake);
				obliterate_card(instance->targets[0].player, fake);
			}
			shuffle_into_library(player, card);
			return 0;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}


int card_beacon_of_unrest(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( (count_graveyard_by_type(player, TYPE_CREATURE | TYPE_ARTIFACT) && ! graveyard_has_shroud(player)) ||
			(count_graveyard_by_type(1-player, TYPE_CREATURE | TYPE_ARTIFACT) && ! graveyard_has_shroud(1-player))
		  ){
			return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ARTIFACT, "Select an artifact or creature card");

		if(  select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1) == -1 ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
			shuffle_into_library(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_blasting_station(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_blind_creeper(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, -1, -1);
	}
	return 0;
}

int card_bringer_of_the_black_dawn(int player, int card, event_t event){

	if( player == current_turn && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		upkeep_trigger_ability_mode(player, card, event, player, can_pay_life(player, 2) ? RESOLVE_TRIGGER_CHECK_LIFE_TOTAL(player, 2) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 2);
		global_tutor(player, player, 1, TUTOR_DECK, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return bringers(player, card, event);
}

int card_bringer_of_the_blue_dawn(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_CHECK_DECK_COUNT(player, 2));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, 2);
	}

	return bringers(player, card, event);
}

int card_bringer_of_the_green_dawn(int player, int card, event_t event){
	/* Bringer of the Green Dawn	|7|G|G
	 * Creature - Bringer 5/5
	 * You may pay |W|U|B|R|G rather than pay ~'s mana cost.
	 * Trample
	 * At the beginning of your upkeep, you may put a 3/3 |Sgreen Beast creature token onto the battlefield. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_BEAST);
	}

	return bringers(player, card, event);
}

int card_bringer_of_the_red_dawn(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( player == current_turn && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		upkeep_trigger_ability_mode(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;

			if( player != instance->targets[0].player ){
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}

	return bringers(player, card, event);
}

int card_bringer_of_the_white_dawn(int player, int card, event_t event){

	if( player == current_turn && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int can_tutor = count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && ! graveyard_has_shroud(player);

		upkeep_trigger_ability_mode(player, card, event, player, can_tutor ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && ! graveyard_has_shroud(player) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return bringers(player, card, event);
}

int card_channel_the_suns(int player, int card, event_t event){
	/*
	  Channel the Suns	|3|G
	  Sorcery
	  Add {W}{U}{B}{R}{G} to your mana pool.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana_multi(player, 0, 1, 1, 1, 1, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_chimeric_coils(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(1+(player == AI)), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(1));
		if( spell_fizzled != 1 ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				set_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
				instance->info_slot = x_value;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
		add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_CONSTRUCT);
		artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1,
							instance->info_slot, instance->info_slot, 0, 0);
		parent->targets[1].player = 0;
	}

	return 0;
}

int card_clearwater_goblet(int player, int card, event_t event)
{
  /* Clearwater Goblet	|5
   * Artifact
   * Sunburst
   * At the beginning of your upkeep, you may gain life equal to the number of charge counters on ~. */

  upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	gain_life(player, count_counters(player, card, COUNTER_CHARGE));

  sunburst(player, card, event, COUNTER_CHARGE);
  return 0;
}

int card_clock_of_omens(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT );
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) ){
			return target_available(player, card, &td) > 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( tapsubtype_ability(player, card, 2, &td) ){
				instance->number_of_targets = 0;
				new_pick_target(&td1, "Select target artifact to untap.", 0, GS_LITERAL_PROMPT);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_composite_golem(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana_multi(player, 0, 1, 1, 1, 1, 1);
		kill_card(player, card, KILL_SACRIFICE);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		int i;
		for(i=1; i<6; i++){
			declare_mana_available(player, i, 1);
		}
	}

	return 0;
}

int card_condescend(int player, int card, event_t event){
	/* Condescend	|X|U
	 * Instant
	 * Counter target spell unless its controller pays |X. Scry 2. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			return counterspell(player, card, event, NULL, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot)){
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_conjurers_bouble(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( count_graveyard(player) > 0 && ! graveyard_has_shroud(player) ){
			char msg[100] = "Select a card to put on bottom of deck.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
		}
		tap_card(player, card);
		kill_card(player, card, KILL_SACRIFICE);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[0].player > -1 ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				from_graveyard_to_deck(player, selected, 2);
			}
		}
		draw_cards(player, 1);
	}

	return 0;
}

int card_cosmic_larva(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( can_sacrifice_as_cost(player, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			int ai_choice = 0;
			if( count_subtype(player, TYPE_LAND, -1) < 3 || life[1-player] > get_power(player, card) ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", ai_choice);
			if( choice == 0  ){
				sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				kill = 0;
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_cranial_plating(int player, int card, event_t event){
	int amount = count_subtype(player, TYPE_ARTIFACT, -1);
	return instant_equipment(player, card, event, 1, COLOR_BLACK, amount, 0, 0, 0);
}

int card_crucible_of_worlds(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && ! is_humiliated(player, card) ){
		if( can_sorcery_be_played(player, event) && !(land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) ){
			if( count_graveyard_by_type(player, TYPE_LAND) > 0 && instance->targets[0].player != 1){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[0].player = 1;
		char msg[100] = "Select a land to play from graveyard.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, msg);
		int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		if( result != -1 ){
			cant_be_responded_to = 1;
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
			++lands_played;
			int c;
			for(c = active_cards_count[player]-1; c > -1; c--){
				if( in_play(player, c) && ! is_humiliated(player, c) ){
					int csvid = get_id(player, c);
					switch( csvid ){
							case(CARD_ID_HORN_OF_GREED):
								draw_cards(player, 1);
								break;

							case(CARD_ID_CITY_OF_TRAITORS):
								kill_card(player, c, KILL_SACRIFICE);
								break;

							case(CARD_ID_FASTBOND):
							{
								if( lands_played > 1 ){
									damage_player(player, 1, player, c);
								}
							}
							break;
					}
				}
			}
		}
		else{
			spell_fizzled = 1;
		}
		instance->targets[0].player = 0;
	}

	return 0;
}

int card_desecration_elemental(int player, int card, event_t event){

	fear(player, card, event);

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_devour_in_shadows(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int life_loss = get_toughness(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			lose_life(player, life_loss);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_door_to_nothingness( int player, int card, event_t event){

	/* Door to Nothingness	|5
	 * Artifact
	 * ~ enters the battlefield tapped.
	 * |W|W|U|U|B|B|R|R|G|G, |T, Sacrifice ~: Target player loses the game. */

	comes_into_play_tapped(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_NONE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
		   lose_the_game( instance->targets[0].player );
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, 0, 2, 2, 2, 2, 2, 0, &td, "TARGET_PLAYER");
}

int card_doubling_cube(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		card_instance_t* instance = get_card_instance(player, card);
		instance->state |= STATE_TAPPED;
		charge_mana(player, COLOR_COLORLESS, 3);
		if( spell_fizzled == 1){
			instance->state &= ~STATE_TAPPED;
		}
		else {
			int m[6];
			int i;
			for(i=0; i<6; i++){
				m[i] = mana_pool[player][i];
			}
			// Ruling 12/1/2004: Any restrictions on the mana in your pool aren't copied.
			m[COLOR_COLORLESS] += mana_pool[player][COLOR_ARTIFACT];
			produce_mana_tapped_multi(player, card, m[COLOR_COLORLESS], m[COLOR_BLACK], m[COLOR_BLUE], m[COLOR_GREEN], m[COLOR_RED], m[COLOR_WHITE]);
		}
	}

	return 0;
}

int card_early_frost(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "TARGET_LAND", 3, NULL);
}

int card_ebon_drake(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		lose_life(player, 1);
	}

	return 0;
}

static int endless_whispers_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( eot_trigger(player, card, event) ){
		int t_player = instance->targets[0].player;
		int owner = instance->targets[1].player;
		int iid = instance->targets[1].card;
		seek_grave_for_id_to_reanimate(t_player, -1, owner, cards_data[iid].id, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_endless_whispers(int player, int card, event_t event){
	/*
	  Endless Whispers |2|B|B
	  Enchantment
	  Each creature has "When this creature dies, choose target opponent.
	  That player puts this card from its owner's graveyard onto the battlefield under his or her control at the beginning of the next end step."
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
			if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_token(affected_card_controller, affected_card) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
					if( instance->info_slot < 0 ){
						instance->info_slot = 0;
					}
					target_definition_t td;
					default_target_definition(affected_card_controller, affected_card, &td, TYPE_CREATURE );
					td.zone = TARGET_ZONE_PLAYERS;

					if( would_validate_arbitrary_target(&td, 1-affected_card_controller, -1) ){
						int pos = instance->info_slot;
						if( pos < 10 ){
							instance->targets[pos].player = 0;
							instance->targets[pos].card = -1;
							SET_BYTE0(instance->targets[pos].player) |= affected_card_controller;
							SET_BYTE1(instance->targets[pos].player) |= get_owner(affected_card_controller, affected_card);
							instance->targets[pos].card = get_original_internal_card_id(affected_card_controller, affected_card);
							instance->info_slot++;
						}
					}
				}
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY &&
			player == reason_for_trigger_controller && affect_me(player, card) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->info_slot; i++){
						int legacy = create_legacy_effect(player, card, &endless_whispers_legacy);
						card_instance_t *leg = get_card_instance( player, legacy );
						leg->targets[0].player = BYTE0(instance->targets[i].player) == 0 ? 1 : 0;
						leg->targets[1].player = BYTE1(instance->targets[i].player);
						leg->targets[1].card = instance->targets[i].card;
					}
					instance->info_slot = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_energy_chamber(int player, int card, event_t event)
{
  /* Energy Chamber	|2
   * Artifact
   * At the beginning of your upkeep, choose one - Put a +1/+1 counter on target artifact creature; or put a charge counter on target noncreature artifact. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT"))
		add_counter(instance->targets[0].player, instance->targets[0].card,
					is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ? COUNTER_P1_P1 : COUNTER_CHARGE);
	}

  return 0;
}

int card_engineered_explosives(int player, int card, event_t event){

	/* Engineered Explosives	|X
	 * Artifact
	 * Sunburst
	 * |2, Sacrifice ~: Destroy each nonland permanent with converted mana cost equal to the number of charge counters on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			infinite_casting_cost();
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) &&
		!played_for_free(player, card) && !is_token(player, card)
	  ){
		int paid[7] = {0};

		int cless = get_updated_casting_cost(player, card, -1, event, 0);
		if (cless > 0){
			char old_always_prompt_for_color = always_prompt_for_color;
			always_prompt_for_color = 1;
			charge_mana(player, COLOR_COLORLESS, cless);
			always_prompt_for_color = old_always_prompt_for_color;
			if (cancel == 1){
				return 0;
			}
			memcpy(paid, mana_paid, sizeof(paid));
		}

		char old_always_prompt_for_color = always_prompt_for_color;
		always_prompt_for_color = 1;
		charge_mana(player, COLOR_COLORLESS, -1);
		always_prompt_for_color = old_always_prompt_for_color;
		if (cancel == 1){
			return 0;
		}

		int colors = 0, col;
		for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col){
			if (mana_paid[col] > 0 || paid[col] > 0){
				++colors;
			}
		}

		if (colors > 0){
			++hack_silent_counters;
			add_counters(player, card, COUNTER_CHARGE, colors);
			--hack_silent_counters;
		}
	}

	if(event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(2));
		if(spell_fizzled != 1){
			instance->info_slot = count_counters(player, card, COUNTER_CHARGE);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		this_test.cmc = instance->info_slot;
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
	}

	return 0;
}

int card_ensouled_scimitar(int player, int card, event_t event){

	/* Ensouled Scimitar	|3
	 * Artifact - Equipment
	 * |3: ~ becomes a 1/5 Spirit artifact creature with flying until end of turn.
	 * Equipped creature gets +1/+5.
	 * Equip |2 */

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 1, 5, 0);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(3), 0, NULL, NULL) ){
			if( !IS_AI(player) || !is_equipping(player, card) ){
				return 1;
			}
		}
		return !is_what(player, card, TYPE_CREATURE) && can_activate_basic_equipment(player, card, event, 2);
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 2) && !is_what(player, card, TYPE_CREATURE) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, MANACOST_X(3), 0, NULL, NULL) ){
				if( !IS_AI(player) || !is_equipping(player, card) ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Animate\n Cancel", 1);
				}
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			activate_basic_equipment(player, card, 2);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(3), 0, NULL, NULL);
				if( spell_fizzled != 1 ){
					instance->info_slot = 67;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 ){
			add_a_subtype(player, instance->parent_card, SUBTYPE_SPIRIT);
			artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 1, 5, KEYWORD_FLYING, 0);
		}
	}

	return 0;
}

int card_etched_oracle(int player, int card, event_t event){

	/* Etched Oracle	|4
	 * Artifact Creature - Wizard 0/0
	 * Sunburst
	 * |1, Remove four +1/+1 counters from ~: Target player draws three cards. */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), GVC_COUNTERS(COUNTER_P1_P1, 4), &td, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			instance->number_of_targets = 0;
			charge_mana_for_activated_ability(player, card, MANACOST_X(1));
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
				remove_1_1_counters(player, card, 4);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				draw_cards(instance->targets[0].player, 3);
			}
		}
	}

	sunburst(player, card, event, COUNTER_P1_P1);
	return 0;
}

int card_eternal_witness(int player, int card, event_t event)
{
  // 0x4e41fa

  /* Eternal Witness	|1|G|G
   * Creature - Human Shaman 2/1
   * When ~ enters the battlefield, you may return target card from your graveyard to your hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, 0, "Select target card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_eyes_of_the_watcher(int player, int card, event_t event){

	if( has_mana(player, COLOR_COLORLESS, 1) &&
		specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			scry(player, 2);
		}
	}
	return global_enchantment(player, card, event);
}

int card_fangren_pathcutter(int player, int card, event_t event)
{
  // Whenever ~ attacks, attacking creatures gain trample until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 0,0, KEYWORD_TRAMPLE,0, &test);
	}

  return 0;
}

int card_ferocious_charge(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_fill_with_fright(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_fist_of_suns(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && has_mana_multi(player, 0, 1, 1, 1, 1, 1) ){
		int c;
		int result = 0;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_hand(player, c) && ! is_what(player, c, TYPE_LAND) ){
				int this_result = can_legally_play_iid(player, get_original_internal_card_id(player, c));
				if( this_result && (player == HUMAN || (player == AI && ! has_mana_to_cast_id(player, EVENT_CAN_CAST, get_id(player, c)))) ){
					if( this_result == 99 ){
						return 99;
					}
					result = this_result;
				}
			}
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_multi(player, 0, 1, 1, 1, 1, 1);
		if( spell_fizzled != 1 ){
			int playable[2][hand_count[player]];
			int pc = 0;
			int c;
			for(c=0; c<active_cards_count[player]; c++){
				if( in_hand(player, c) && ! is_what(player, c, TYPE_LAND) ){
					int this_result = can_legally_play_iid(player, get_original_internal_card_id(player, c));
					if( this_result && (player == HUMAN || (player == AI && ! has_mana_to_cast_id(player, EVENT_CAN_CAST, get_id(player, c)))) ){
						playable[0][pc] = get_original_internal_card_id(player, c);
						playable[1][pc] = c;
						pc++;
					}
				}
			}
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to play.");
			this_test.zone = TARGET_ZONE_HAND;
			int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				play_card_in_hand_for_free(player, playable[1][selected]);
				cant_be_responded_to = 1;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

int card_fleshgrafter(int player, int card, event_t event){
	/* Fleshgrafter	|2|B
	 * Creature - Human Warrior 2/2
	 * Discard an artifact card: ~ gets +2/+2 until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	return 0;
}

int card_furnace_whelp(int player, int card, event_t event){
	// 0x4C4C10
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,0);
}

int card_gemstone_array(int player, int card, event_t event)
{
  /* Gemstone Array	|4
   * Artifact
   * |2: Put a charge counter on ~.
   * Remove a charge counter from ~: Add one mana of any color to your mana pool. */

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL && player == AI && !(trace_mode & 2))
	instance->state |= STATE_NO_AUTO_TAPPING;	// AI will still draw mana, but reluctantly.

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += 12 * count_counters(player, card, COUNTER_CHARGE);

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card))
	declare_mana_available_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, count_counters(player, card, COUNTER_CHARGE));

  if (IS_ACTIVATING(event))
	{
	  int can_generate_mana = can_produce_mana(player, card) && count_counters(player, card, COUNTER_CHARGE) > 0;
	  int can_add_counter = !paying_mana() && can_use_activated_abilities(player, card);

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_COUNTER = 2
	  } choice;

	  if (event == EVENT_ACTIVATE && can_generate_mana && paying_mana())	// Force choice; don't display dialog
		instance->info_slot = choice = CHOICE_MANA;
	  else
		choice = DIALOG(player, card, event,
						"Generate mana",		can_generate_mana,	paying_mana() ? 3 : -1,
						"Add charge counter",	can_add_counter,	1,						DLG_MANA(MANACOST_X(2)));

	  /* It's legal to pay some of the cost for adding a charge counter by removing charge counters.  (Sometimes it's even a good idea.  For instance, you have
	   * two Doubling Seasons; or a March of the Machines and a Training Grounds; or you've enchanted it with Power Artifact; or...) */

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  ai_modifier -= 12;
			  if (produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1))
				{
				  remove_counter(player, card, COUNTER_CHARGE);
				  tapped_for_mana_color = -2;
				}
			  break;

			case CHOICE_COUNTER:
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_MANA:
			  break;

			case CHOICE_COUNTER:
			  if (in_play(instance->parent_controller, instance->parent_card))
				add_counter(player, card, COUNTER_CHARGE);
			  break;
		  }
	}

  return 0;
}

int card_goblin_cannon(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
			kill_card(instance->parent_controller, instance->parent_card, KILL_SACRIFICE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_grafted_wargear(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 3, 2, 0);
	}

	return basic_equipment(player, card, event, 0);	// graft effect hard-coded to CARD_ID_GRAFTED_WARGEAR in unattach()
}

int card_granulate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i = 0; i < 2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && ! is_what(i, count, TYPE_LAND) && get_cmc(i, count) < 5 ){
						kill_card(i, count, KILL_DESTROY);
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_guardian_idol(int player, int card, event_t event){
	/* Guardian Idol	|2
	 * Artifact
	 * ~ enters the battlefield tapped.
	 * |T: Add |C to your mana pool.
	 * |2: ~ becomes a 2/2 Golem artifact creature until end of turn. */

	comes_into_play_tapped(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == AI ){
			if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(3), 0, NULL, NULL) ){
				return 1;
			}
		}
		else{
			if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(2), 0, NULL, NULL) ){
				return 1;
			}
		}
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int abilities[2] = {	mana_producer(player, card, EVENT_CAN_ACTIVATE),
								0
		};
		if( player == AI ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, MANACOST_X(3), 0, NULL, NULL) ){
				abilities[1] = 1;
			}
		}
		else{
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, MANACOST_X(2), 0, NULL, NULL) ){
				abilities[1] = 1;
			}
		}

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						  "Produce mana", abilities[0], 1,
						  "Animate", abilities[1], current_phase < PHASE_DECLARE_ATTACKERS ? 10 : 0);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 1 ){
			instance->info_slot = choice;
			return mana_producer(player, card, event);
		}
		if( choice == 2){
			if( player == AI ){
				add_state(player, card, STATE_TAPPED);
			}
			generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(2), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
			if( player == AI ){
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 2 ){
			add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_GOLEM);
			artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 2, 2, 0, 0);
		}
	}

	return 0;
}

int card_healers_headdress(int player, int card, event_t event){

	/* Healer's Headdress	|2
	 * Artifact - Equipment
	 * Equipped creature gets +0/+2 and has "|T: Prevent the next 1 damage that would be dealt to target creature or player this turn."
	 * |W|W: Attach ~ to target creature you control.
	 * Equip |1 */

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 0, 2, 0);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.extra = damage_card;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = td1.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE ){
		if (is_equipping(player, card)){
			int result = granted_generic_activated_ability(player, card, instance->targets[8].player, instance->targets[8].card, event,
													GAA_UNTAPPED | GAA_CAN_TARGET| GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
			if( result ){
				return result;
			}
		}
		return instant_equipment(player, card, event, 1, COLOR_WHITE, 0, 2, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		instance->number_of_targets = 0;
		int c1 = get_updated_equip_cost(player, card, 1);
		int can_instant_equip = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XW(c1, 2), 0, &td1, "TARGET_CREATURE");
		int can_basic_equip = can_activate_basic_equipment(player, card, event, 1);
		int can_prevent_damage = 0;
		if (is_equipping(player, card)){
			can_prevent_damage = granted_generic_activated_ability(player, card, instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE,
																	GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Equip", can_basic_equip, 15,
						"Instant equip", can_instant_equip, 10,
						"Prevent damage", can_prevent_damage, 50);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}

		if( choice == 1 || choice == 2 ){
			int ec = get_updated_equip_cost(player, card, 1);
			if( charge_mana_for_activated_ability(player, card, MANACOST_XW(choice == 1 ? ec : 0, 2*(choice == 2))) ){
				if( new_pick_target(&td1, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT)  ){
					instance->info_slot = 1;
				}
			}
		}
		if( choice == 3 ){
			if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST0) &&
				pick_target(&td, "TARGET_DAMAGE")
			  ){
				tap_card(instance->targets[8].player, instance->targets[8].card);
				instance->info_slot = 2;
			}
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td1) ){
			equip_target_creature(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 2 && valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > 0 ){
				target->info_slot--;
			}
		}
	}

	return 0;
}

static int can_generate_kaldra(int player)
{
  card_instance_t* instance;
  int c, kaldra_count = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if ((instance = in_play(player, c)))
	  {
		switch (cards_data[instance->internal_card_id].id)
		  {
			case CARD_ID_HELM_OF_KALDRA:	kaldra_count |= 1;	break;
			case CARD_ID_SWORD_OF_KALDRA:	kaldra_count |= 2;	break;
			case CARD_ID_SHIELD_OF_KALDRA:	kaldra_count |= 4;	break;
			default:
			  continue;
		  }
		if (kaldra_count == 7)
		  return 1;
	  }

  return 0;
}

static void equip_kaldra(token_generation_t* token, int kaldra, int number)
{
  if (number != 0)
	return;

  card_instance_t* instance;
  int c;
  for (c = 0; c < active_cards_count[token->t_player]; ++c)
	if ((instance = in_play(token->t_player, c)))
	  switch (cards_data[instance->internal_card_id].id)
		{
		  case CARD_ID_HELM_OF_KALDRA:
		  case CARD_ID_SWORD_OF_KALDRA:
		  case CARD_ID_SHIELD_OF_KALDRA:
			equip_target_creature(token->t_player, c, token->t_player, kaldra);
			break;
		}
}

int card_helm_of_kaldra(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		if( event == EVENT_ABILITIES && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			haste(instance->targets[8].player, instance->targets[8].card, event);
			event_result |= KEYWORD_FIRST_STRIKE | KEYWORD_TRAMPLE;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

#define CAN_EQUIP	can_activate_basic_equipment(player, card, event, 2)
#define CAN_KALDRA	(has_mana_for_activated_ability(player, card, MANACOST_X(1)) && can_generate_kaldra(player))

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return CAN_EQUIP || CAN_KALDRA;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if (CAN_EQUIP){
			if (CAN_KALDRA){
				int ai_choice = check_battlefield_for_id(player, CARD_ID_KALDRA) ? 0 : 1;
				choice = do_dialog(player, player, card, -1, -1, " Equip\n Summon Kaldra\n Cancel", ai_choice);
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
			activate_basic_equipment(player, card, 2);
		}
		if( choice == 1 ){
			charge_mana_for_activated_ability(player, card, MANACOST_X(1));
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 0 ){
			resolve_activation_basic_equipment(player, card);
		}
		if (instance->info_slot == 1 && can_generate_kaldra(player)){
			// 12/1/2004: The token isn't put onto the battlefield unless you control all three Equipment when the ability resolves.
			token_generation_t token;
			default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_KALDRA, &token);
			token.special_code_on_generation = &equip_kaldra;
			generate_token(&token);
		}
	}

	return 0;
#undef CAN_EQUIP
#undef CAN_KALDRA
}

int card_horned_helm(int player, int card, event_t event){
	return instant_equipment(player, card, event, 1, COLOR_GREEN, 1, 1, KEYWORD_TRAMPLE, 0);
}

int card_hoverguard_sweepers(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
		int i;
		for(i=0; i<2; i++){
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				break;
			}
		}
	}
	return 0;
}

static const char* target_has_charge_or_1_1_counter(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (count_counters(player, card, COUNTER_P1_P1) || count_counters(player, card, COUNTER_CHARGE)) ? NULL : "must have counters";
}

int card_ion_storm(int player, int card, event_t event)
{
  /* Ion Storm	|2|R
   * Enchantment
   * |1|R, Remove a +1/+1 counter or a charge counter from a permanent you control: ~ deals 2 damage to target creature or player. */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td_counter;
	  base_target_definition(player, card, &td_counter, TYPE_PERMANENT);
	  td_counter.allowed_controller = player;
	  td_counter.preferred_controller = player;
	  td_counter.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td_counter.extra = (int)target_has_charge_or_1_1_counter;

	  target_definition_t td_dmg;
	  default_target_definition(player, card, &td_dmg, TYPE_CREATURE);
	  td_dmg.zone = TARGET_ZONE_CREATURE_OR_PLAYER;


	  if (event == EVENT_CAN_ACTIVATE)
		return CAN_ACTIVATE(player, card, MANACOST_XR(1,1)) && can_target(&td_counter) && can_target(&td_dmg);

	  if (event == EVENT_ACTIVATE)
		{
		  int choice;
		  card_instance_t* instance = get_card_instance(player, card);
		  instance->number_of_targets = 0;
		  if (charge_mana_for_activated_ability(player, card, MANACOST_XR(1,1))
			  && pick_target(&td_counter, "TARGET_PERMANENT")
			  && (choice = DIALOG(player, card, event,
								  DLG_AUTOCHOOSE_IF_1,
								  "Remove a +1/+1 counter",	count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_P1_P1), 1,
								  "Remove a charge counter",	count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE), 2)))
			{
			  target_t permanent_w_counter = instance->targets[0];	// struct copy
			  instance->number_of_targets = 0;
			  if (pick_target(&td_dmg, "TARGET_CREATURE_OR_PLAYER"))
				remove_counter(permanent_w_counter.player, permanent_w_counter.card, choice == 1 ? COUNTER_P1_P1 : COUNTER_CHARGE);
			}
		}

	  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_dmg))
		damage_target0(player, card, 2);
	}

  return global_enchantment(player, card, event);
}

int card_joiner_adept(int player, int card, event_t event){
	return permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
}

int card_kaldra(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	return 0;
}

int card_krark_clan_ironworks(int player, int card, event_t event){
	// krark clan ironworks
	if(event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_ACTIVATE ){
		if( sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			 produce_mana(player, COLOR_COLORLESS, 2);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) &&
		can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		declare_mana_available(player, COLOR_COLORLESS, 2);
	}

	return 0;
}

int card_leonin_squire(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card with CMC 1 or less.");
		this_test.cmc = 2;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		int trig_mode = new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) ? RESOLVE_TRIGGER_MANDATORY : 0;
		if( comes_into_play_mode(player, card, event, trig_mode) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_lose_hope(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_magma_giant(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		APNAP(p, {new_damage_all(player, card, p, 2, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
	}

	return 0;
}

int card_magma_jet(int player, int card, event_t event){

	/* Magma Jet	|1|R
	 * Instant
	 * ~ deals 2 damage to target creature or player.
	 * Scry 2. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_magnetic_theft(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT );
	td1.required_subtype = SUBTYPE_EQUIPMENT;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE );
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td2);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td1, "TARGET_EQUIPMENT") ){
			new_pick_target(&td2, "TARGET_CREATURE", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td1, 0) && validate_target(player, card, &td2, 1) ){
				equip_target_creature(instance->targets[0].player, instance->targets[0].card,
									  instance->targets[1].player, instance->targets[1].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

#pragma message "Probably needs its own PLAYER_BITS"
int card_mephidross_vampire(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affected_card_controller == player && ! affect_me(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		add_a_subtype(affected_card_controller, affected_card, SUBTYPE_VAMPIRE);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					add_a_subtype(player, count, SUBTYPE_VAMPIRE);
				}
				count--;
		}
	}

	if( event == EVENT_DEAL_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_source_player == player && damage->damage_target_card != -1 &&
			is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE) &&
			is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
			(damage->info_slot > 0 ||
			 get_card_instance(damage->damage_source_player, damage->damage_source_card)->targets[16].player > 0)	// wither/infect damage
		  ){
			// 12/1/2004: If the creature is dealt damage by more than one source at the same time, it gets only one counter.
			// Search for existing legacies originating from this Rite of Passage attached to target.
			int p, c;
			for (p = 0; p <= 1; ++p){
				for (c = 0; c < active_cards_count[player]; ++c){
					card_instance_t* instance = get_card_instance(p, c);
					if (instance->damage_source_card == card && instance->damage_source_player == player
						&& instance->damage_target_card == damage->damage_target_card && instance->damage_target_player == damage->damage_target_player
						&& instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_rite_of_passage){
						return 0;
					}
				}
			}
			create_targetted_legacy_effect(player, card, effect_rite_of_passage, damage->damage_source_player, damage->damage_source_card);
		}
	}

	if( leaves_play(player, card, event) ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					reset_subtypes(player, count, 2);
				}
				count--;
		}
	}

	return 0;
}

int card_moriok_rigger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_ARTIFACT, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_mycosynth_golem(int player, int card, event_t event){

	affinity(player, card, event, TYPE_ARTIFACT, -1);

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( affected_card_controller == player && is_what(affected_card_controller,  affected_card, TYPE_ARTIFACT) &&
			is_what(affected_card_controller,  affected_card, TYPE_CREATURE)
		  ){
			COST_COLORLESS-=count_subtype(player, TYPE_ARTIFACT, -1);
		}
	}

	return 0;
}

int card_myr_quadropod(int player, int card, event_t event)
{
  // |3: Switch ~'s power and toughness until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
}

int card_myr_servitor(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.id = CARD_ID_MYR_SERVITOR;
		APNAP(p, {new_reanimate_all(p, -1, p, &this_test, REANIMATE_DEFAULT);});
	}

	return 0;
}

int card_neurok_stealthsuit(int player, int card, event_t event){
	return instant_equipment(player, card, event, 1, COLOR_BLUE, 0, 0, KEYWORD_SHROUD, 0);
}

int card_nights_whispers(int player, int card, event_t event){
	/*
	  Night's Whisper |1|B
	  Sorcery
	  You draw two cards and you lose 2 life.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		lose_life(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_opaline_bracers(int player, int card, event_t event){
	/* Opaline Bracers	|4
	 * Artifact - Equipment
	 * Sunburst
	 * Equipped creature gets +X/+X, where X is the number of charge counters on ~.
	 * Equip |2 */
	sunburst(player, card, event, COUNTER_CHARGE);
	int num = count_counters(player, card, COUNTER_CHARGE);
	return vanilla_equipment(player, card, event, 2, num,num, 0,0);
}

int card_paradise_mantle(int player, int card, event_t event){

	/* Paradise Mantle	|0
	 * Artifact - Equipment
	 * Equipped creature has "|T: Add one mana of any color to your mana pool."
	 * Equip |1 */

	card_instance_t *instance = get_card_instance(player, card);

	int e_player = instance->targets[8].player;
	int e_card = instance->targets[8].card;

#define CAN_GET_MANA	(is_equipping(player, card) && !is_tapped(e_player, e_card) && !is_sick(e_player, e_card) && can_produce_mana(player, card))
#define CAN_EQUIP		(can_activate_basic_equipment(player, card, event, 1))
#define CHOICE_MANA		1
#define CHOICE_EQUIP	2

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return CAN_GET_MANA || CAN_EQUIP;
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = (CAN_GET_MANA ? CHOICE_MANA : 0) | (CAN_EQUIP ? CHOICE_EQUIP : 0);
			if (choice == (CHOICE_MANA|CHOICE_EQUIP)){
				choice = 1 << do_dialog(player, player, card, -1, -1, " Get mana\n Change equipped creature\n Cancel", 0);
			}

			if( choice == CHOICE_MANA ){
				produce_mana_tapped_all_one_color(e_player, e_card, COLOR_TEST_ANY_COLORED, 1);
				if (cancel != 1){
					// tap_card() would do this, but clear tapped_for_mana_color first.
					dispatch_event(e_player, e_card, EVENT_TAP_CARD);
				}
			}

			else if( choice == CHOICE_EQUIP ){
					activate_basic_equipment(player, card, 1);
					instance->info_slot = CHOICE_EQUIP;
			}
			else{
				spell_fizzled = 1;
			}

			instance->info_slot = choice;
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == CHOICE_EQUIP ){
				resolve_activation_basic_equipment(player, card);
			}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_GET_MANA){
		declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);
	}

	return 0;
#undef CAN_GET_MANA
#undef CAN_EQUIP
#undef CHOICE_MANA
#undef CHOICE_EQUIP
}

int card_pentad_prism(int player, int card, event_t event){

	/* Pentad Prism	|2
	 * Artifact
	 * Sunburst
	 * Remove a charge counter from ~: Add one mana of any color to your mana pool. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( count_counters(player, card, COUNTER_CHARGE) && can_produce_mana(player, card)){
			return 1;
		}
	}

	if (event == EVENT_ACTIVATE){
		if (produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1)){
			remove_counter(player, card, COUNTER_CHARGE);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		int count = count_counters(player, card, COUNTER_CHARGE);
		if (count > 0){
			declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, count);
		}
	}

	sunburst(player, card, event, COUNTER_CHARGE);
	return 0;
}

int card_plunge_into_darkness(int player, int card, event_t event)
{
  /* Choose one - Sacrifice any number of creatures, then you gain 3 life for each sacrificed creature; or pay X life, then look at the top X cards of your
   * library, put one of those cards into your hand, and exile the rest.
   * Entwine |B */
  if (event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL)
	{
	  int can_gain_life = count_permanents_by_type(player, TYPE_CREATURE) > 0;
	  int can_look = can_pay_life(player, 1);
	  enum
	  {
		CHOICE_GAIN_LIFE = 1,
		CHOICE_LOOK = 2,
		CHOICE_ENTWINE = 3,	// == CHOICE_GAIN_LIFE|CHOICE_LOOK
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						/* Always legal to pick any of these, but AI won't unless it'll be able to do something at resolution */
						"Sacrifice and gain life", 1, can_gain_life ? 1 : -1,
						"Look at X and choose one", 1, can_look && life[player] >= 7 ? 2 : -1,
						"Entwine", 1, can_gain_life && can_look && life[player] >= 4 ? 4 : -1, DLG_MANA(MANACOST_B(1)));

	  if (event == EVENT_CAN_CAST)
		return choice;
	  else if (event == EVENT_RESOLVE_SPELL)
		{
		  if (choice & CHOICE_GAIN_LIFE)
			{
			  int num_sacrificed = 0;
			  while (controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, SAC_DONE))
				{
				  ++num_sacrificed;
				  if (IS_AI(player) && count_permanents_by_type(player, TYPE_CREATURE) < 3)
					break;
				}
			  gain_life(player, 3 * num_sacrificed);
			}

		  if ((choice & CHOICE_LOOK) && can_pay_life(player, 1))
			{
			  int amount;
			  if (player == AI || ai_is_speculating == 1)
				amount = MAX(0, life[player] - 6);
			  else
				{
				  amount = choose_a_number(player, "Pay how much life?", life[player] - 1);
				  if (amount > life[player])
					amount = life[player];
				}

			  if (amount > 0)
				{
				  lose_life(player, amount);

				  test_definition_t test;
				  default_test_definition(&test, 0);
				  test.create_minideck = amount;
				  test.no_shuffle = 1;
				  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &test);

				  rfg_top_n_cards_of_deck(player, amount - 1);
				}
			}

		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_possessed_portal(int player, int card, event_t event){

	/* Possessed Portal	|8
	 * Artifact
	 * If a player would draw a card, that player skips that draw instead.
	 * At the beginning of each end step, each player sacrifices a permanent unless he or she discards a card. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && !suppress_draw && ! is_humiliated(player, card) ){
		if( event == EVENT_TRIGGER){
			event_result |= 2u;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				suppress_draw = 1;
		}
	}

	if( eot_trigger(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int p = i == 0 ? current_turn : 1-current_turn;
			int choice = 1;
			if( hand_count[player] ){
				int ai_priorities[2] = { 10-(2*count_subtype(player, TYPE_PERMANENT, -1)), 2*count_subtype(player, TYPE_PERMANENT, -1)};
				int ai_choice = ai_priorities[1] > ai_priorities[0] ? 1 : 0;
				choice = do_dialog(p, player, card, player, card, " Discard\n Sac a permanent", ai_choice);
			}
			if( choice == 0 ){
				discard(p, 0, player);
			}
			if( choice == 1 ){
				impose_sacrifice(player, card, p, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return 0;
}

int card_raksha_golden_cub(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES) && ! is_humiliated(player, card) &&
		equipments_attached_to_me(player, card, EATM_CHECK)
	  ){
		boost_creature_type(player, card, event, SUBTYPE_CAT, 2, 2, KEYWORD_DOUBLE_STRIKE, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	}

	return 0;
}

int card_razormane_masticore(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int choice = do_dialog(player, player, card, -1, -1, " Discard\n Pass", 0);
			if( choice == 0 ){
				kill--;
				discard(player, 0, player);
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if (trigger_condition == TRIGGER_DRAW_PHASE && affect_me(player, card) && player == reason_for_trigger_controller && current_turn == player){
		if( ! is_humiliated(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);

			if (event == EVENT_TRIGGER){
				event_result |= (can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0);
			}
			else if (event == EVENT_RESOLVE_TRIGGER){
				instance->number_of_targets = 0;
				if (pick_target(&td, "TARGET_CREATURE")){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
				}
			}
		}
	}

	return 0;
}

int card_reversal_of_fortune(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			char msg[100] = "Select an instant or sorcery card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_SPELL, msg);
			this_test.type_flag = F1_NO_CREATURE;

			int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1  ){
				card_instance_t *crd = get_card_instance(instance->targets[0].player, selected);
				if( can_legally_play_iid(player, crd->internal_card_id) ){
					copy_spell(player, get_id(instance->targets[0].player, selected));
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_PLAYER", 1, NULL);
}

int card_rite_of_passage(int player, int card, event_t event){

	if( event == EVENT_DEAL_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player && damage->damage_target_card != -1 &&
			is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE) &&
			(damage->info_slot > 0 ||
			 get_card_instance(damage->damage_source_player, damage->damage_source_card)->targets[16].player > 0)	// wither/infect damage
		  ){
			// 12/1/2004: If the creature is dealt damage by more than one source at the same time, it gets only one counter.
			// Search for existing legacies originating from this Rite of Passage attached to target.
			int p, c;
			for (p = 0; p <= 1; ++p){
				for (c = 0; c < active_cards_count[player]; ++c){
					card_instance_t* instance = get_card_instance(p, c);
					if (instance->damage_source_card == card && instance->damage_source_player == player
						&& instance->damage_target_card == damage->damage_target_card && instance->damage_target_player == damage->damage_target_player
						&& instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_rite_of_passage){
						return 0;
					}
				}
			}
			create_targetted_legacy_effect(player, card, effect_rite_of_passage, damage->damage_target_player, damage->damage_target_card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_roar_of_reclamation(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		APNAP(p, {new_reanimate_all(p, -1, p, &this_test, REANIMATE_DEFAULT);});
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rude_awakening(int player, int card, event_t event){

	enum{	CHOICE_UNTAP = 1,
			CHOICE_ANIMATE_LANDS,
			CHOICE_ENTWINE
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		int priorities[3];
		this_test.state = STATE_TAPPED;
		priorities[0] = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		this_test.state_flag = DOESNT_MATCH;
		priorities[1] = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		priorities[2] = 50;

		int choice = choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						  "Untap your lands", 1, priorities[0],
						  "Animate your lands", 1, priorities[1],
						  "Entwine", has_mana_multi(player, MANACOST_XG(2, 1)), priorities[2]);
		if( ! choice  ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_ENTWINE ){
			charge_mana_multi(player, MANACOST_XG(2, 1));
		}

		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_LAND);
		if( instance->info_slot & CHOICE_UNTAP ){
			new_manipulate_all(player, card, player, &this_test2, ACT_UNTAP);
		}
		if( instance->info_slot & CHOICE_ANIMATE_LANDS ){
			int c = active_cards_count[player]-1;
			while( c > -1 ){
					if( in_play(player, c) && is_what(player, c, TYPE_LAND) ){
						turn_into_creature(player, card, player, c, 1, 2, 2);
					}
					c--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_serum_visions(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
		scry(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_skullcage(int player, int card, event_t event){

	if( (hand_count[1-player] < 3 || hand_count[1-player] > 4) && ! is_humiliated(player, card) ){
		upkeep_trigger_ability(player, card, event, 1-player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			damage_player(1-player, 2, player, card);
		}
	}

	return 0;
}

int card_skyreach_manta(int player, int card, event_t event){
	/* Skyreach Manta	|5
	 * Artifact Creature - Fish 0/0
	 * Sunburst
	 * Flying */
	sunburst(player, card, event, COUNTER_P1_P1);
	return 0;
}

int card_solarion(int player, int card, event_t event)
{
  /* Solarion	|7
   * Artifact Creature - Construct 0/0
   * Sunburst
   * |T: Double the number of +1/+1 counters on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	sunburst(player, card, event, COUNTER_P1_P1);

	if (event == EVENT_RESOLVE_ACTIVATION)
		add_counters(instance->parent_controller, instance->parent_card,
					COUNTER_P1_P1, count_counters(instance->parent_controller, instance->parent_card, COUNTER_P1_P1));

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_sparring_collar(int player, int card, event_t event){
	return instant_equipment(player, card, event, 1, COLOR_RED, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_staff_of_domination(int player, int card, event_t event){
	/*
	  Staff of Domination |3

	  Artifact

	  {1}: Untap Staff of Domination.

	  {2}, {T}: You gain 1 life.

	  {3}, {T}: Untap target creature.

	  {4}, {T}: Tap target creature.

	  {5}, {T}: Draw a card.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum{
		CHOICE_UNTAP = 1,
		CHOICE_GAIN_LIFE,
		CHOICE_UNTAP_CREATURE,
		CHOICE_TAP_CREATURE,
		CHOICE_DRAW
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	if( player == AI ){
		td1.required_state = TARGET_STATE_TAPPED;
	}

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int abilities[5];
		int i;
		for(i=1; i<6; i++){
			abilities[i-1] = 0;
			if( has_mana_for_activated_ability(player, card, MANACOST_X(i)) ){
				if( i == CHOICE_UNTAP ){
					abilities[i-1] = 1;
				}
				if( ! is_tapped(player, card) ){
					if( i == CHOICE_GAIN_LIFE || i == CHOICE_DRAW ){
						abilities[i-1] = 1;
					}
					if( i == CHOICE_UNTAP_CREATURE && can_target(&td1) ){
						abilities[i-1] = 1;
					}
					if( i == CHOICE_TAP_CREATURE && can_target(&td) ){
						abilities[i-1] = 1;
					}
				}
			}
		}
		int priorities[5] = {	is_tapped(player, card) ? 20 : 0,
								life[player] < 6 ? 15 : 0,
								10,
								10,
								count_subtype(player, TYPE_LAND, -1) > 5 ? 12 : 5
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Untap Staff of Domination", abilities[0], priorities[0],
							"Gain 1 life", abilities[1], priorities[1],
							"Untap a creature", abilities[3], priorities[3],
							"Tap a creature", abilities[2], priorities[2],
							"Draw a card", abilities[4], priorities[4]);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(choice)) ){
			switch( choice ){
					case( CHOICE_UNTAP ):
							instance->info_slot = choice;
							break;
					case( CHOICE_GAIN_LIFE ):
							tap_card(player, card);
							instance->info_slot = choice;
							break;
					case( CHOICE_UNTAP_CREATURE ):
					{
						if( new_pick_target(&td1, "Select target creature to untap.", 0, 1 | GS_LITERAL_PROMPT) ){
							tap_card(player, card);
							instance->info_slot = choice;
						}
					}
					break;
					case( CHOICE_TAP_CREATURE ):
					{
						if( new_pick_target(&td, "Select target creature to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
							tap_card(player, card);
							instance->info_slot = choice;
						}
					}
					break;
					case( CHOICE_DRAW ):
							tap_card(player, card);
							instance->info_slot = choice;
							break;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int choice = instance->info_slot;
		switch( choice ){
				case( CHOICE_UNTAP ):
						untap_card(instance->parent_controller, instance->parent_card);
						break;
				case( CHOICE_GAIN_LIFE ):
						gain_life(player, 1);
						break;
				case( CHOICE_UNTAP_CREATURE ):
					{
						if( valid_target(&td1) ){
							untap_card(instance->targets[0].player, instance->targets[0].card);
						}
					}
					break;
				case( CHOICE_TAP_CREATURE ):
					{
						if( valid_target(&td) ){
							tap_card(instance->targets[0].player, instance->targets[0].card);
						}
					}
					break;
				case( CHOICE_DRAW ):
					draw_cards(player, 1);
					break;
		}
	}

	return 0;
}

int card_stand_firm(int player, int card, event_t event){

	if (event == EVENT_CHECK_PUMP ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 1, 0, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_stasis_cocoon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE) ){
			cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
			cannot_block(instance->damage_target_player, instance->damage_target_card, event);
		}
		if( instance->targets[1].player != instance->damage_target_player || instance->targets[1].card != instance->damage_target_card ){
			if( instance->targets[1].player != -1 && instance->targets[1].card != -1 ){
				disable_all_activated_abilities(instance->targets[1].player, instance->targets[1].card, 0);
			}
			disable_all_activated_abilities(instance->damage_target_player, instance->damage_target_card, 1);
			instance->targets[1].player = instance->damage_target_player;
			instance->targets[1].card = instance->damage_target_card;
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

int card_steelshapers_gift(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an Equipment card.");
		this_test.subtype = SUBTYPE_EQUIPMENT;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_summoners_egg(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.zone = TARGET_ZONE_HAND;

		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier-=500;
		}
	}

	if( comes_into_play(player, card, event) ){
		int result = -1;
		if( hand_count[player] > 0 ){
			if( player == HUMAN ){
				test_definition_t this_test2;
				new_default_test_definition(&this_test2, TYPE_ANY, "Select a card to exile.");
				this_test2.zone = TARGET_ZONE_HAND;

				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test2);
				if( selected != -1 ){
					result = get_original_internal_card_id(player, selected);
					rfg_card_in_hand(player, selected);
					create_card_name_legacy(player, card, cards_data[result].id);
				}
			}
			else{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.zone = TARGET_ZONE_HAND;

				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
				if( selected != -1 ){
					result = get_original_internal_card_id(player, selected);
					rfg_card_in_hand(player, selected);
				}
			}
		}
		instance->targets[1].card = result;
	}

	if( instance->targets[1].card > -1 && this_dies_trigger(player, card, event, 2) ){
		int id = cards_data[instance->targets[1].card].id;
		if( check_rfg(player, id) ){
			remove_card_from_rfg(player, id);
			int card_added = add_card_to_hand(player, instance->targets[1].card);
			put_into_play(player, card_added);
		}
	}

	return 0;
}

int card_suncrusher(int player, int card, event_t event){

	/* Suncrusher	|9
	 * Artifact Creature - Construct 3/3
	 * Sunburst
	 * |4, |T, Remove a +1/+1 counter from ~: Destroy target creature.
	 * |2, Remove a +1/+1 counter from ~: Return ~ to its owner's hand. */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(2), 0, NULL, NULL);
		}

		if(event == EVENT_ACTIVATE ){
			int abilities[2] = {
									generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_1_1_COUNTER,
																MANACOST_X(4), 0, &td, NULL),
									generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_1_1_COUNTER, MANACOST_X(2), 0, NULL, NULL)
			};

			int priorities[2] = {5, count_1_1_counters(player, card) < 2 ? 10 : 0};

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Destroy a creature", abilities[0], priorities[0],
							"Bounce Suncrusher", abilities[1], priorities[1]);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(4/choice)) ){
				instance->number_of_targets = 0;
				if( choice == 1 && pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
				}
				if( spell_fizzled != 1 ){
					remove_1_1_counter(player, card);
					instance->info_slot = choice;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 && valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( instance->info_slot == 2 ){
				bounce_permanent(instance->parent_controller, instance->parent_card);
			}
		}
	}

	sunburst(player, card, event, COUNTER_P1_P1);
	return 0;
}

// Suntouched myr --> Skyreach Manta

int card_sylvok_explorer(int player, int card, event_t event){

	return card_fellwar_stone(player, card, event);
}

int card_synod_centurion(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( count_subtype(player, TYPE_ARTIFACT, -1) < 1 ){
			ai_modifier-=1000;
		}
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		int found = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( i != card && in_play(player, i) && is_what(player, i, TYPE_ARTIFACT) ){
				found++;
				break;
			}
		}
		if( found == 0 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_tel_jilad_justice(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

// Thought Courier --> Merfolk Looter

int card_tornado_elemental(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, 2, 6, 0, &this_test);
	}
	return card_thorn_elemental(player, card, event);
}

int card_trinket_mage(int player, int card, event_t event){

	/* Trinket Mage	|2|U
	 * Creature - Human Wizard 2/2
	 * When ~ enters the battlefield, you may search your library for an artifact card with converted mana cost 1 or less, reveal that card, and put it into
	 * your hand. If you do, shuffle your library. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card with CMC 1 or less");
		this_test.cmc = 2;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_vedalken_mastermind(int player, int card, event_t event){
	/* Vedalken Mastermind	|U|U
	 * Creature - Vedalken Wizard 1/2
	 * |U, |T: Return target permanent you control to its owner's hand. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_U(1), 0,
									&td, "Select target permanent you control.");
}

int card_vedalken_shackles(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = 1-player;
	}
	td.power_requirement = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		char buffer[100];
		scnprintf(buffer, 100, "Select target creature with power %d or less.", count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND));
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(2), 0, &td, buffer);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		td.power_requirement = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) | TARGET_PT_LESSER_OR_EQUAL;
		if( valid_target(&td) ){
			gain_control_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card,
															instance->targets[0].player, instance->targets[0].card, GCUS_TAPPED);
		}
	}

	return 0;
}

int card_wayfarers_bauble(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_land(player, 1, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
}
