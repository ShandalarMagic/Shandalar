#include "manalink.h"

// Also finds Phenomenon cards (by design)
int is_plane_card(int csvid)
{
  // Subtype Plane/Phenomenon is guaranteed to be first.
  return csvid >= 0 && (cards_ptr[csvid]->types[0] == SUBTYPE_PLANE || cards_ptr[csvid]->types[0] == SUBTYPE_PHENOMENON);
}

static int get_actual_plane(int player){
	int pcc = locate_id(player, CARD_ID_PLANECHASE);
	if( pcc > -1 ){
		card_instance_t *instance = get_card_instance(player, pcc);
		return instance->targets[1].card;
	}
	else{
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && is_plane_card(get_id(player, i)) ){
				return get_id(player, i);
			}
		}
	}
	return -1;
}

int get_new_plane(int current_plane)
{
  static int* plane_csvids = NULL;
  static int num_plane_csvids;

  if (!plane_csvids)
	{
	  // Find all valid plane csvids.  Since we only do this once and it's (relatively) quick, do it in two passes.

	  // First pass: find number.
	  int iid, csvid, num_plane_csvids_first_pass = 0;
	  for (iid = 0; (csvid = cards_data[iid].id) != CARD_ID_RULES_ENGINE; ++iid)
		if (is_plane_card(csvid) && (card_coded[csvid / 8] & (1 << (csvid % 8))))
		  ++num_plane_csvids_first_pass;

	  // Allocate the array
	  ASSERT(num_plane_csvids_first_pass > 0);
	  plane_csvids = (int*)malloc(sizeof(int) * num_plane_csvids_first_pass);
	  ASSERT(plane_csvids != NULL);

	  // Second pass: Populate array
	  num_plane_csvids = 0;
	  for (iid = 0; (csvid = cards_data[iid].id) != CARD_ID_RULES_ENGINE; ++iid)
		if (is_plane_card(csvid) && (card_coded[csvid / 8] & (1 << (csvid % 8))))
		  plane_csvids[num_plane_csvids++] = csvid;
	  ASSERT(num_plane_csvids == num_plane_csvids_first_pass);
	}

  return plane_csvids[internal_rand(num_plane_csvids)];
}

void planeswalk_to(int player, int target_plane, int override){
	int i;
	int actual_plane = -1;
	for(i=0;i<2; i++){
		int count = active_cards_count[i]-1;
		while( count > -1 ){
				if( in_play(i, count) ){
					int id = get_id(i, count);
					if( id == CARD_ID_PLANECHASE ){
						card_instance_t *instance = get_card_instance(i, count);
						if( actual_plane == -1 ){
							actual_plane = instance->targets[1].card;
						}
						instance->targets[1].card = target_plane;
					}
					if( is_plane_card(id) ){
						if( player == i && override != 1 ){
							card_instance_t *plane = get_card_instance(i, count);
							int (*ptFunction)(int, int, event_t) = (void*)cards_data[plane->internal_card_id].code_pointer;
							ptFunction(i, count, EVENT_PLANESWALK_OUT);
						}
						kill_card(i, count, KILL_REMOVE);
					}
				}
				count--;
		}
		int card_added = add_card_to_hand(i, get_internal_card_id_from_csv_id(target_plane));
		convert_to_token(i, card_added);
		put_into_play(i, card_added);
		card_instance_t *instance = get_card_instance(player, card_added);
		instance->targets[1].player = 66;
		set_special_infos(player, card_added, 88);
	}
	if( override != 1 ){
		int fake = add_card_to_hand(player, get_internal_card_id_from_csv_id(target_plane));
		int (*ptFunction)(int, int, event_t) = (void*)cards_data[get_internal_card_id_from_csv_id(target_plane)].code_pointer;
		ptFunction(player, fake, EVENT_PLANESWALK_IN);
		obliterate_card(player, fake);
	}
}

static void casting_the_planar_dice( int player, int card, int result){
	int cp = get_actual_plane(player);
	if( cp > -1 ){
		if( player == AI  ){
			do_dialog(HUMAN, player, card, -1, -1, "Opponent is casting the Planar Dice.", 0);
		}
		if( (get_id(player, card) == CARD_ID_PLANECHASE || get_id(player, card) == CARD_ID_FRACTURED_POWERSTONE) && result == 1 ){
			int np = get_new_plane(cp);
			card_ptr_t* c = cards_ptr[ cp  ];
			card_ptr_t* c1 = cards_ptr[ np  ];
			char buffer[100];
			scnprintf(buffer, 100, "Planeswalking away from %s\nto %s", c->name, c1->name);
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
			planeswalk_to(player, np, 0);
		}
		else if( result == 6 || (get_id(player, card) != CARD_ID_PLANECHASE && result == 5) ){
				card_ptr_t* c = cards_ptr[ cp  ];
				char buffer[100];
				scnprintf(buffer, 100, "The power of %s\nis unleashed!", c->name);
				do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
				int plane = locate_id(player, cp);
				if( plane > -1 ){
					card_instance_t *instance = get_card_instance(player, plane);
					int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->internal_card_id].code_pointer;
					ptFunction(player, plane, EVENT_CHAOS);
				}
		}
		else{
			 do_dialog(HUMAN, player, card, -1, -1, "Nothing happened.", 0);
		}
	}
}

static int generic_plane(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( get_special_infos(player, card) == 88 ){
			return 0;
		}
		int result = 0;
		if( can_sorcery_be_played(player, event) && has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) ){
			result = 1;
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		if( current_phase == PHASE_MAIN2 ){
			ai_modifier+=50;
		}
		int amount = count_counters(player, card, COUNTER_ENERGY);
		charge_mana(player, COLOR_COLORLESS, amount);
		if( spell_fizzled != 1 ){
			add_counter(player, card, COUNTER_ENERGY);
			int cc = internal_rand(6)+1;
			instance->info_slot = cc;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (instance->info_slot > 0){
			casting_the_planar_dice(player, instance->parent_card, instance->info_slot);
		}
	}

	if( count_counters(player, card, COUNTER_ENERGY) > 0 && eot_trigger(player, card, event) ){
		remove_all_counters(player, card, COUNTER_ENERGY);
	}

	return 0;
}


int card_planechase(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 8);

	return generic_plane(player, card, event);
}


// Planechase 2012

// white
int card_felidar_umbra(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && instance->damage_target_player != -1 ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
	}
	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		parent->damage_target_player = instance->targets[0].player;
		parent->damage_target_card = instance->targets[0].card;
	}
	return totem_armor(player, card, event, 0, 0, 0, SP_KEYWORD_LIFELINK);
}

int card_illusory_angel(int player, int card, event_t event){
	if( event == EVENT_MODIFY_COST && get_specific_storm_count(player) < 1 ){
		infinite_casting_cost();
	}
	return 0;
}

// blue
int card_sakashimas_student(int player, int card, event_t event)
{
  /* Sakashima's Student	|2|U|U
   * Creature - Human Ninja 0/0
   * Ninjutsu |1|U
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield, except it's still a Ninja in addition to its other creature types. */

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	add_a_subtype(player, card, SUBTYPE_NINJA);

  return ninjutsu(player, card, event, MANACOST_XU(1,1), CARD_ID_SAKASHIMAS_STUDENT);
}

// red
int card_beetleback_chief(int player, int card, event_t event ){
	/* Beetleback Chief	|2|R|R
	 * Creature - Goblin Warrior 2/2
	 * When ~ enters the battlefield, put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
	}

	return 0;
}

int card_mass_mutiny(int player, int card, event_t event)
{
  /* Mass Mutiny	|3|R|R
   * Sorcery
   * For each opponent, gain control of up to one target creature that player controls until end of turn. Untap those creatures. They gain haste until end of
   * turn. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = 1-player;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_preyseizer_dragon(int player, int card, event_t event)
{
  // Whenever ~ attacks, it deals damage to target creature or player equal to the number of +1/+1 counters on ~.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  int amt = count_1_1_counters(player, card);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") && amt > 0)	// Still needs to trigger and target even if no counters
		damage_creature(instance->targets[0].player, instance->targets[0].card, amt, player, card);
	}

  devour(player, card, event, 2);
  return 0;
}

// green
int card_brindle_shoat(int player, int card, event_t event ){
	/* Brindle Shoat	|1|G
	 * Creature - Boar 1/1
	 * When ~ dies, put a 3/3 |Sgreen Boar creature token onto the battlefield. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BOAR, &token);
		token.pow = 3;
		token.tou = 3;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}
	return 0;
}

int card_dreampod_druid(int player, int card, event_t event )
{
  /* Dreampod Druid	|1|G
   * Creature - Human Druid 2/2
   * At the beginning of each upkeep, if ~ is enchanted, put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

  if (trigger_condition == TRIGGER_UPKEEP && is_enchanted(player, card))
	upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	generate_token_by_id(player, card, CARD_ID_SAPROLING);

  return 0;
}

// gold

int card_baleful_strix(int player, int card, event_t event ){

	deathtouch(player, card, event);

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_dragonlair_spider(int player, int card, event_t event ){
	/* Dragonlair Spider	|2|R|R|G|G
	 * Creature - Spider 5/6
	 * Reach
	 * Whenever an opponent casts a spell, put a 1/1 |Sgreen Insect creature token onto the battlefield. */

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		generate_token_by_id(player, card, CARD_ID_INSECT);
	}
	return 0;
}

// elderwood scion --> skipped (impossible)

int card_etherium_horn_sorcerer(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	cascade(player, card, event, -1);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		bounce_permanent(player, instance->parent_card);
	}
	return generic_activated_ability(player, card, event, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0);
}

int card_indrik_umbra(int player, int card, event_t event){
	return totem_armor(player, card, event, 4, 4, KEYWORD_FIRST_STRIKE, SP_KEYWORD_LURE);
}

int card_krond_the_dawn_clad(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  vigilance(player, card, event);

  // Whenever ~ attacks, if it's enchanted, exile target permanent.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && is_enchanted(player, card)
	  && declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT );
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT"))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

  return 0;
}

int card_maeltrom_wanderer(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
		do_cascade(player, card, -1);
		do_cascade(player, card, -1);
	}
	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( affected_card_controller == player && in_play(affected_card_controller, affected_card) ){
			haste(player, affected_card, event);
		}
	}
	return 0;
}

static int can_legally_play_iid_reversed(int iid, int player, int unused1, int unused2){
	return can_legally_play_iid(player, iid);
}

int card_silent_blade_oni(int player, int card, event_t event ){

	/* Silent-Blade Oni	|3|U|U|B|B
	 * Creature - Demon Ninja 6/5
	 * Ninjutsu |4|U|B
	 * Whenever ~ deals combat damage to a player, look at that player's hand. You may cast a nonland card in it without paying that card's mana cost. */

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_OPPONENT) && hand_count[1-player] > 0 ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card to play for free.");
		this_test.type_flag = DOESNT_MATCH;
		this_test.special_selection_function = &can_legally_play_iid_reversed;
		this_test.value_for_special_selection_function = player;

		int selected = new_select_a_card(player, 1-player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			opponent_plays_card_in_your_hand_for_free(1-player, selected);
		}
	}
	return ninjutsu(player, card, event, 4, 1, 1, 0, 0, 0, CARD_ID_SILENT_BLADE_ONI);
}

int card_thromok_the_insatiable(int player, int card, event_t event ){
	/* Thromok the Insatiable	|3|R|G
	 * Legendary Creature - Hellion 0/0
	 * Devour X, where X is the number of creatures devoured this way */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_state(player, card, STATE_OUBLIETTED);
		int result = devouring(player, card);
		remove_state(player, card, STATE_OUBLIETTED);
		add_1_1_counters(player, card, result * result);
	}

	return 0;
}

int card_vela_the_night_clad(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	if (event == EVENT_ABILITIES
		&& affected_card_controller == player
		&& is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		&& !is_humiliated(player, card)){
		intimidate(affected_card_controller, affected_card, event);
	}

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me(player, card) && trigger_cause_controller == player && reason_for_trigger_controller == player &&
			in_play(trigger_cause_controller, trigger_cause) && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					lose_life(1-player, 1);
			}
		}
	}

	return 0;
}

// artifacts
int card_fractured_powerstone(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;
		if( player != AI && ! paying_mana() && can_sorcery_be_played(player, event) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Cast the planar dice\n Cancel", 0);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				casting_the_planar_dice(player, instance->parent_card, internal_rand(6)+1);
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

int card_sai_of_the_shinobi(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
	   reason_for_trigger_controller == affected_card_controller && trigger_cause_controller == player
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && ! check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE );
			td1.allowed_controller = player;
			td1.preferred_controller = player;

			instance->targets[0].player = trigger_cause_controller;
			instance->targets[0].card = trigger_cause;
			if( valid_target(&td1) ){
				trig = 1;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					 equip_target_creature(player, card, trigger_cause_controller, trigger_cause);
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 1, 1, 0, 0);
}

// Plane cards

int card_academy_at_tolaria_west(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( current_turn == player && eot_trigger(player, card, event) && hand_count[player] < 1 ){
		draw_cards(player, 7);
	}

	if( event == EVENT_CHAOS ){
		discard_all(player);
	}

	return generic_plane(player, card, event);
}

static int agyrem_legacy(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		reanimate_mode_t mode = REANIMATEXTRA_RETURN_TO_HAND2;
		if( instance->targets[0].card > -1 && (instance->targets[0].card & COLOR_TEST_WHITE) ){
			mode = REANIMATE_DEFAULT;
		}
		seek_grave_for_id_to_reanimate(player, card, player, instance->targets[0].player, mode);
		kill_card(player, card, KILL_REMOVE);
	}
	return generic_plane(player, card, event);
}

int card_agyrem(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player == 66 ){
		nobody_can_attack(player, card, event, 1-player);
	}

	if( event == EVENT_CHAOS ){
		instance->targets[1].player = 66;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == player ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].player = get_original_id(affected_card_controller, affected_card);
					instance->targets[pos].card = get_color(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
		){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<instance->targets[11].player; i++){
					int legacy = create_legacy_effect(player, card, &agyrem_legacy);
					card_instance_t *leg = get_card_instance(player, legacy);
					leg->targets[0].player = instance->targets[i].player;
					leg->targets[0].card = instance->targets[i].card;
				}
				instance->targets[11].player = 0;
		}
	}
	return generic_plane(player, card, event);
}

static int effect_indestructible_as_long_as_has_divinity_counter(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  indestructible(instance->damage_target_player, instance->damage_target_card, event);

  if (event == EVENT_STATIC_EFFECTS && !count_counters(instance->damage_target_player, instance->damage_target_card, COUNTER_DIVINITY))
	{
	  kill_card(player, card, KILL_REMOVE);
	  recalculate_all_cards_in_play();	// since this is an effect (kill_card_guts() calls this for permanents)
	}

  return 0;
}
int card_bant(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	// All creatures have exalted.

	/* Planes use targets[1] *and* info_slot.  Fortunately, targets[3] for alternate main storage and targets[2] for tracking are free.  DAT_TRACK is normally
	 * incompatible with DAT_STORE_IN_TARGETS_3, but we get away with it since DAT_ATTACKS_ALONE means we'll never store more than 1 attacking index. */
	if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_ALONE|DAT_STORE_IN_TARGETS_3|DAT_TRACK, 2, -1)){
		card_instance_t* instance = get_card_instance(player, card);
		int attacker = BYTE0(instance->targets[2].player);
		if (in_play(current_turn, attacker)){
			int c, amount = 0;
			for (c = 0; c < active_cards_count[current_turn]; ++c){
				if (in_play(current_turn, c) && is_what(current_turn, c, TYPE_CREATURE) && !is_humiliated(current_turn, c)){
					++amount;
				}
			}
			if (amount > 0){
				pump_until_eot(player, card, player, attacker, amount, amount);
			}
		}
	}

	/* Whenever you roll |C, put a divinity counter on target |Sgreen, |Swhite, or |Sblue creature. That creature is indestructible as long as it has a divinity
	 * counter on it. */
	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.required_color = COLOR_TEST_BLUE | COLOR_TEST_GREEN | COLOR_TEST_WHITE;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_DIVINITY);
			create_targetted_legacy_effect(player, card, effect_indestructible_as_long_as_has_divinity_counter,
										   instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_plane(player, card, event);
}

int card_cliffside_market(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance( player, card );

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0 && instance->targets[1].player != 66){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				card_instance_t *this= get_card_instance(player, card);
				card_data_t* card_d = &cards_data[ this->internal_card_id ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
		}
	}

	if( event == EVENT_PLANESWALK_IN || event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		if( would_valid_target(&td) ){
			int ai_choice = 0;
			if( life[1-player] < life[player] ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Exchange life totals with opponent\n Pass", ai_choice);
			if( choice == 0 ){
				int amount = life[player];
				set_life_total(player, life[1-player]);
				set_life_total(1-player, amount);
			}
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = player;
		td.preferred_controller = player;

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			target_definition_t td1;
			default_target_definition(player, card, &td1, get_type(instance->targets[0].player, instance->targets[0].card));
			td1.allowed_controller = 1-player;
			td1.preferred_controller = 1-player;
			if( new_pick_target(&td1, "TARGET_PERMANENT", 1, 0) ){
				exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
														instance->targets[1].player, instance->targets[1].card);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_eloren_wilds(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			int spc = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//64
			card_instance_t *leg = get_card_instance(player, spc);
			leg->targets[2].player = 64;
			leg->targets[3].player = instance->targets[0].player;
			leg->targets[2].card = get_id(player, card);
			create_card_name_legacy(player, spc, get_id(player, card));
		}
	}

	if( event == EVENT_TAP_CARD || event == EVENT_COUNT_MANA ){
		card_mana_flare(player, card, event);
	}

	return generic_plane(player, card, event);
}

int card_feeding_grounds(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, get_cmc(instance->targets[0].player, instance->targets[0].card));
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( get_color(affected_card_controller, affected_card) & COLOR_TEST_GREEN ){
			COST_COLORLESS--;
		}
		if( get_color(affected_card_controller, affected_card) & COLOR_TEST_RED ){
			COST_COLORLESS--;
		}
	}

	return generic_plane(player, card, event);
}

int card_fields_of_summer(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		gain_life(player, 10);
	}

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 2);
	}

	return generic_plane(player, card, event);
}

// glimmervoid basing --> skipped

int card_goldmeadow2(int player, int card, event_t event){
	/* *** Goldmeadow	""
	 * Plane - Lorwyn
	 * Whenever a land enters the battlefield, that land's controller puts three 0/1 |Swhite Goat creature tokens onto the battlefield.
	 * Whenever you roll |C, put a 0/1 |Swhite Goat creature token onto the battlefield. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		generate_token_by_id(player, card, CARD_ID_GOAT);
	}

	if( specific_cip(player, card, event, player, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		generate_tokens_by_id(player, card, CARD_ID_GOAT, 3);
	}

	return generic_plane(player, card, event);
}

int card_grixis(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;

		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);

		if( ! graveyard_has_shroud(2) && has_dead_creature(2) ){
			instance->targets[0].player = 1-player;
			if( has_dead_creature(1-player) ){
				if( has_dead_creature(player) ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
					}
				}
			}
			else{
				instance->targets[0].player = player;
			}
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_sorcery_be_played(player, event) ){
			if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) ){
				return 1;
			}
			return can_cast_creature_from_grave(player, 5, event);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) ){
			if( can_cast_creature_from_grave(player, 5, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Cast the Planar Dice\n Use Unearth\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
				const int *grave = get_grave(player);
				int selected = -1;

				if( player != AI ){
					selected = select_a_card(player, player, 2, 0, 1, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
				else{
					selected = can_cast_creature_from_grave(player, 6, event);
				}

				if( selected != -1 ){
					int id = cards_data[grave[selected]].id;
					charge_mana_from_id(player, -1, event, id);
					if( spell_fizzled != 1 ){
						rfg_card_from_grave(player, selected);
						instance->targets[1].card = id;
						instance->info_slot = 10;
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
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 10 ){
			token_generation_t token;
			token.legacy = 1;
			token.special_code_for_legacy = &effect_unearth;
			generate_token(&token);
		}
		else{
			casting_the_planar_dice(player, instance->parent_card, instance->info_slot);
		}
	}

	return vanguard_card(player, card, event, 7, 20, 0);
}

int card_immersturm(int player, int card, event_t event){

	/* *** Immersturm	""
	 * Plane - Valla
	 * Whenever a creature enters the battlefield, that creature's controller may have it deal damage equal to its power to target creature or player of his or her choice.
	 * Whenever you roll |C, exile target creature, then return it to the battlefield under its owner's control. */

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pandemonium_effect(instance->targets[1].player, instance->targets[1].card);
	}

	return generic_plane(player, card, event);
}

int card_isle_of_vesuva(int player, int card, event_t event){
	/* *** Isle of Vesuva	""
	 * Plane - Dominaria
	 * Whenever a nontoken creature enters the battlefield, its controller puts a token onto the battlefield that's a copy of that creature.
	 * Whenever you roll |C, destroy target creature and all other creatures with the same name as that creature. */

	card_instance_t *instance = get_card_instance(player, card);

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.id = get_id(instance->targets[0].player, instance->targets[0].card);
			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		}
	}

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
		token_generation_t token;
		copy_token_definition(player, card, &token, instance->targets[1].player, instance->targets[1].card);
		generate_token(&token);
	}

	return generic_plane(player, card, event);
}

static int izzet_steam_maze_legacy(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_SPELL);
	this_test.type_flag = F1_NO_CREATURE;

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
			COST_COLORLESS-=3;
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_izzet_steam_maze(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_SPELL);
	this_test.type_flag = F1_NO_CREATURE;

	vanguard_card(player, card, event, 7, 20, 0);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && trigger_cause_controller == player &&
		! check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST)
	 ){

	   int trig = 0;

	   if( new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, &this_test) ){
		  trig = 1;
	   }

	   if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					copy_spell_from_stack(player, trigger_cause_controller, trigger_cause);
			}
		}
	}

	if( event == EVENT_CHAOS ){
		create_legacy_effect(player, card, &izzet_steam_maze_legacy);
	}

	return generic_plane(player, card, event);
}

int card_krosa2(int player, int card, event_t event){

   vanguard_card(player, card, event, 7, 20, 0);

   if( event == EVENT_CHAOS ){
		produce_mana_multi(player, 0, 1, 1, 1, 1, 1);
   }

	boost_creature_type(player, card, event, -1, 2, 2, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

   return generic_plane(player, card, event);
}

int card_lethe_lake(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			mill(instance->targets[0].player, 10);
		}
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		mill(player, 10);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_llanowar2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = 0;	// generic_plane() sets this

		int choice = 0;
		int can_tap_creature = permanents_you_control_can_tap_for_mana(player, card, EVENT_CAN_ACTIVATE, TYPE_CREATURE, -1, COLOR_GREEN, 2);
		if( can_tap_creature ){
			if( generic_plane(player, card, EVENT_CAN_ACTIVATE)){
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Cast the Planar Dice\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			return permanents_you_control_can_tap_for_mana(player, card, event, TYPE_CREATURE, -1, COLOR_GREEN, 2);
		}
		else if( choice == 1 ){
			return generic_plane(player, card, event);
		}
		else{
			spell_fizzled = 1;
		}
		return 0;
	}

	int result = permanents_you_control_can_tap_for_mana(player, card, event, TYPE_CREATURE, -1, COLOR_GREEN, 2);
	if( event == EVENT_CAN_ACTIVATE && result ){
		return 1;
	}

	if( event == EVENT_CHAOS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	return generic_plane(player, card, event);
}

int card_minamo2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		char msg[100] = "Select a blue card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		this_test.color = COLOR_TEST_BLUE;
		int i;
		for(i=0; i<2; i++){
			if( count_graveyard(i) > 0 ){
				new_global_tutor(i, i, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
		}
	}

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return generic_plane(player, card, event);
}

// murasa --> skipped

int card_naar_isle(int player, int card, event_t event){

	/* Naar Isle	""
	 * Plane - Wildfire
	 * At the beginning of your upkeep, put a flame counter on Naar Isle, then Naar Isle deals damage to you equal to the number of flame counters on it.
	 * Whenever you roll |C, Naar Isle deals 3 damage to target player. */

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			damage_player(instance->targets[0].player, 3, player, card);
		}
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, 2);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_FLAME);
		if( current_turn == player ){
			damage_player(current_turn, count_counters(player, card, COUNTER_FLAME), player, card);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_naya2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.required_color = COLOR_TEST_RED | COLOR_TEST_GREEN | COLOR_TEST_WHITE;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			int amount = count_subtype(player, TYPE_LAND, -1);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
			instance->number_of_targets = 1;
		}
	}

	if( current_turn == player ){
		if(current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2 ){
			land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}

	return generic_plane(player, card, event);
}

int card_otaria(int player, int card, event_t event){

	/* *** Otaria	""
	 * Plane - Dominaria
	 * Instant and sorcery cards in graveyards have flashback. The flashback cost is equal to the card's mana cost.
	 * Whenever you roll |C, take an extra turn after this one. */

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		return can_cast_spell_from_grave(player, 1);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			if( can_cast_spell_from_grave(player, 1) ){
				choice = do_dialog(player, player, card, -1, -1, " Cast the Planar Dice\n Play a spell from grave\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
				const int *grave = get_grave(player);
				char msg[100] = "Select a spell to Flashback.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_SPELL, msg);
				this_test.type_flag = F1_NO_CREATURE;
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					if( can_legally_play_iid(player, grave[selected]) && charge_mana_from_id(player, -1, event, cards_data[grave[selected]].id) ){
						play_card_in_grave_for_free_and_exile_it(player, player, selected);
						cant_be_responded_to = 1;
						instance->info_slot = 10;
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
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot != 10 ){
			casting_the_planar_dice(instance->parent_controller, instance->parent_card, instance->info_slot);
		}
	}

	if( event == EVENT_CHAOS){
		time_walk_effect(player, card);
	}

	return generic_plane(player, card, event);
}

int card_panopticon(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN || event == EVENT_CHAOS){
		draw_cards(player, 1);
	}

	if( event == EVENT_DRAW_PHASE && current_turn == player ){
		event_result++;
	}

	return generic_plane(player, card, event);
}

int card_pools_of_becoming(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS){
		int i;
		for(i=0; i<3; i++){
			int id = get_new_plane(CARD_ID_POOLS_OF_BECOMING);
			card_ptr_t* c = cards_ptr[id];
			char buffer[100];
			scnprintf(buffer, 100, "%s", c->name);
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);

			int iid = get_internal_card_id_from_csv_id(id);
			int card_added = add_card_to_hand(player, iid);
			card_instance_t* plane = get_card_instance(player, card_added);
			plane->state |= STATE_INVISIBLE;	--hand_count[player];
			call_card_function_i(plane, player, card_added, EVENT_CHAOS);
			obliterate_card(player, card_added);
		}
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		char msg[100] = "Select a card to put on bottom.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int amount = 0;
		while( hand_count[player] > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				put_on_bottom_of_deck(player, selected);
				amount++;
		}
		draw_cards(player, amount);
	}

	return generic_plane(player, card, event);
}

int card_ravens_run(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( target_available(player, card, &td) > 2 ){
			instance->number_of_targets = 0;
			int trgs = 0;
			while( trgs < 3 ){
					if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
			}
			while( trgs > 0 ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 0);
					add_minus1_minus1_counters(instance->targets[trgs].player, instance->targets[trgs].card, trgs);
					trgs--;
			}
		}
	}

	if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
		wither(affected_card_controller, affected_card, event);
	}

	return generic_plane(player, card, event);
}

int card_sanctum_of_serra(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_OUT){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	if( event == EVENT_CHAOS ){
		int ai_choice = 0;
		if( life[player] > 20 ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Set your life points to 20\n Pass", ai_choice);
		if( choice == 0 ){
			set_life_total(player, 20);
		}
	}

	return generic_plane(player, card, event);
}

int card_sea_of_sand(int player, int card, event_t event){

	/* *** Sea of Sand	""
	 * Plane - Rabiah
	 * Players reveal each card they draw.
	 * Whenever a player draws a land card, that player gains 3 life.
	 * Whenever a player draws a nonland card, that player loses 3 life.
	 * Whenever you roll |C, put target permanent on top of its owner's library. */

	vanguard_card(player, card, event, 7, 20, 0);

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		int iid = get_original_internal_card_id(trigger_cause_controller, trigger_cause);	// Original, since that'll stay in place even if another trigger makes it be discarded
		if( iid != -1 ){
			reveal_card_iid(player, card, iid);
			if( is_what(-1, iid, TYPE_LAND) ){
				gain_life(player, 3);
			}
			else{
				lose_life(player, 3);
			}
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_shiv2(int player, int card, event_t event){

	/* *** Shiv	""
	 * Plane - Dominaria
	 * All creatures have "|R: This creature gets +1/+0 until end of turn."
	 * Whenever you roll |C, put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		if( has_mana(player, COLOR_RED, 1) ){
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && can_use_activated_abilities(player, count) ){
						return 1;
					}
					count++;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int pump = 0;
		if( has_mana(player, COLOR_RED, 1) ){
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && can_use_activated_abilities(player, count) ){
						pump = 1;
						break;
					}
					count++;
			}
		}
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY))  && can_sorcery_be_played(player, event) ){
			if( pump == 1 ){
				choice = do_dialog(player, player, card, -1, -1, " Cast the Planar Dice\n Pump a creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
				charge_mana(player, COLOR_RED, 1);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					if( !can_use_activated_abilities(player, instance->targets[0].card) ){
						spell_fizzled = 1;
						instance->info_slot = 10;
					}
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 10 ){
			if( valid_target(&td)  ){
				pump_until_eot(player, instance->parent_card, player, instance->targets[0].card, 1, 0);
			}
		}
		else{
			casting_the_planar_dice(player, instance->parent_card, instance->info_slot);
		}
	}

	if( event == EVENT_CHAOS ){
		generate_token_by_id(player, card, CARD_ID_DRAGON);
	}

	return generic_plane(player, card, event);
}

int card_skybreen(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	reveal_top_card(player, card, event);

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			if (is_what(affected_card_controller, affected_card, get_type(-1, deck[0])) && !is_what(affected_card_controller, affected_card, TYPE_LAND)){
				infinite_casting_cost();
			}
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			lose_life(instance->targets[0].player, hand_count[instance->targets[0].player]);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_sokenzan(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	boost_subtype(player, card, event, -1, 1,1, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	int count;
	card_instance_t* inst;
	if( event == EVENT_CHAOS ){
		for (count = 0; count < active_cards_count[current_turn]; ++count){
			if( (inst = in_play(current_turn, count)) && is_what(current_turn, count, TYPE_CREATURE) && (inst->state & STATE_ATTACKED) ){
				untap_card(current_turn, count);
			}
		}
	}

	return generic_plane(player, card, event);
}

int card_stronghold_furnace(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 1);
		}
	}

  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && (damage->damage_target_card == -1 || !is_planeswalker(damage->damage_target_player, damage->damage_target_card)))
	damage->info_slot *= 2;

	return generic_plane(player, card, event);
}

int card_tazeem(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_BLOCK_LEGALITY ){
		event_result = 1;
	}

	if( event == EVENT_CHAOS ){
		draw_cards(player, count_subtype(player, TYPE_LAND, -1));
	}

	return generic_plane(player, card, event);
}

static int aether_flues_effect(int player, int card){
	int card_added = -1;
	if( count_subtype(player, TYPE_CREATURE, -1) > 0 ){
		int selected = pick_creature_for_sacrifice(player, card, 0);
		if( selected != -1 ){
			card_added = metamorphosis(player, selected, TYPE_CREATURE, KILL_SACRIFICE);
		}
	}
	return card_added;
}

int card_the_aether_flues(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		aether_flues_effect(player, card);
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		aether_flues_effect(player, card);
	}

	if( event == EVENT_CHAOS ){
		char msg[100] = "Select a creature card to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.zone = TARGET_ZONE_HAND;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

// the dark barony --> skipped

int card_the_eon_fog(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	// Skip-untap effect handled in engine.c:untap_phase()

	if( event == EVENT_CHAOS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	return generic_plane(player, card, event);
}

int card_the_fourth_sphere(int player, int card, event_t event){
	/* *** The Fourth Sphere	""
	 * Plane - Phyrexia
	 * At the beginning of your upkeep, sacrifice a non|Sblack creature.
	 * Whenever you roll |C, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 1, 0, 0, -1, 0);
	}

	if( event == EVENT_CHAOS ){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_the_great_forest(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 2, KEYWORD_TRAMPLE, 0);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_the_hippodrome(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	boost_creature_type(player, card, event, -1, -5, 0, 0, BCT_INCLUDE_SELF);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.power_requirement = 0;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

static void the_maelstrom_effect(int player){
	int *deck = deck_ptr[player];
	if( deck[0] != -1 ){
		show_deck(player, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
		if( cards_data[deck[0]].type & TYPE_PERMANENT ){
			put_into_play_a_card_from_deck(player, player, 0);
		}
		else{
			 deck[count_deck(player)] = deck[0];
			 remove_card_from_deck(player, 0);
		}
	}
}


int card_the_maelstrom(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		the_maelstrom_effect(player);
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		the_maelstrom_effect(player);
	}

	if( event == EVENT_CHAOS ){
		char msg[100] = "Select a permanent card to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
		if( new_special_count_grave(player, &this_test) > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_turri_island(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			COST_COLORLESS-=2;
		}
	}

	if( event == EVENT_CHAOS ){
		int *deck = deck_ptr[player];
		int amount = 3;
		if( amount > count_deck(player) ){
			amount = count_deck(player);
		}
		if( amount > 0 ){
			show_deck( HUMAN, deck, amount, "Cards revealed by Turri Island.", 0, 0x7375B0 );
			while( amount > 0 ){
					if( is_what(-1, deck[0], TYPE_CREATURE) ){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
						amount--;
					}
					else{
						mill(player, 1);
					}
					amount--;
			}
		}
	}

	return generic_plane(player, card, event);
}

int card_undercity_reaches(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	card_instance_t* damage = combat_damage_being_dealt(event);
	if (damage &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		(damage->targets[3].player & TYPE_CREATURE)	// probably redundant to status check
	   ){
		if (instance->targets[damage->damage_source_player + 4].card < 0){
			instance->targets[damage->damage_source_player + 4].card = 0;
		}
		instance->targets[damage->damage_source_player + 4].card++;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) ){
		if (instance->targets[reason_for_trigger_controller + 4].card <= 0){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_AI(reason_for_trigger_controller);
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			if (--instance->targets[reason_for_trigger_controller + 4].card > 0){
				instance->state &= ~STATE_PROCESSING;	// More optional triggers left.  Must be the first thing done during resolve trigger.
			}
			draw_cards(player, 1);
		}
		else if (event == EVENT_END_TRIGGER){
			instance->targets[reason_for_trigger_controller + 4].card = 0;
		}
	}

	if( event == EVENT_CHAOS ){
		create_legacy_effect(player, card, &card_spellbook);
	}

	return generic_plane(player, card, event);
}

int card_velis_vel(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		int pump = 0;
		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && shares_creature_subtype(affected_card_controller, affected_card, i, count) ){
						pump++;
					}
					count++;
			}
		}
		if( pump > 0 ){
			pump--;
		}
		event_result+=pump;
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_PROT_INTERRUPTS, 0);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

// Planes - PC 2012

// akoum --> skipped

int card_aretopolis(int player, int card, event_t event){

	/* *** Aretopolis	""
	 * Plane - Kephalai
	 * When you planeswalk to Aretopolis or at the beginning of your upkeep, put a scroll counter on Aretopolis, then you gain life equal to the number of
	 * scroll counters on it.
	 * When Aretopolis has ten or more scroll counters on it, planeswalk.
	 * Whenever you roll |C, put a scroll counter on Aretopolis, then draw cards equal to the number of scroll counters on it. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		add_counter(player, card, COUNTER_SCROLL);
		gain_life(player, count_counters(player, card, COUNTER_SCROLL));
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_SCROLL);
		gain_life(player, count_counters(player, card, COUNTER_SCROLL));
		if( count_counters(player, card, COUNTER_SCROLL) >= 10 ){
			planeswalk_to(player, get_new_plane(get_actual_plane(player)), 0);
		}
	}

	if( event == EVENT_CHAOS ){
		add_counter(player, card, COUNTER_SCROLL);
		draw_cards(player, count_counters(player, card, COUNTER_SCROLL));
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

// astral arena --> skipped

int card_bloodhill_bastion(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHAOS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;

		if( check_battlefield_for_special_card(player, card, 2, 0, &this_test) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.preferred_controller = player;
			td.allow_cancel = 0;

			while( 1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					if( ! is_token(instance->targets[0].player, instance->targets[0].card) ){
						blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
						break;
					}
				}
			}
		}
	}

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_ability_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 0, 0, KEYWORD_DOUBLE_STRIKE, SP_KEYWORD_HASTE);
	}

	return generic_plane(player, card, event);
}

int card_edge_of_malacol(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
		instance->untap_status &= ~3;
		add_1_1_counters(affected_card_controller, affected_card, 2);
	}

	if( event == EVENT_CHAOS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	return generic_plane(player, card, event);
}

int card_furnace_layer(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_PLANESWALK_IN ){
		int result = get_random_card_in_hand(player);
		if( result > -1 ){
			if( is_what(player, result, TYPE_LAND) ){
				lose_life(player, 3);
			}
			discard_card(player, result);
		}
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int result = get_random_card_in_hand(player);
		if( result > -1 ){
			if( is_what(player, result, TYPE_LAND) ){
				lose_life(player, 3);
			}
			discard_card(player, result);
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_gavony(int player, int card, event_t event){

	/* *** Gavony	""
	 * Plane - Innistrad
	 * All creatures have vigilance.
	 * Whenever you roll |C, creatures you control gain indestructible until end of turn. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		vigilance(affected_card_controller, affected_card, event);
	}

	if( event == EVENT_CHAOS ){
		pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);
	}

	return generic_plane(player, card, event);
}

// glen elendra -> skipped

int card_grand_ossuary(int player, int card, event_t event){
	/* *** Grand Ossuary	""
	 * Plane - Ravnica
	 * Whenever a creature dies, its controller distributes a number of +1/+1 counters equal to its power among any number of target creatures he or she controls.
	 * Whenever you roll |C, each player exiles all creatures he or she controls and puts X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is the total power of the creatures he or she exiled this way. Then planeswalk. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability_and_store_values(player, card, event, ANYBODY, TYPE_CREATURE, NULL, 1, 1);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;
		if( resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			int i;
			for(i=0; i<instance->targets[1].player; i++){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
			instance->targets[1].player = 0;
			instance->targets[11].card = 0;
		}
	}

	if( event == EVENT_CHAOS ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SAPROLING, &token);

		int i;
		for(i=0; i<2; i++){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int sapro = check_battlefield_for_special_card(player, card, i, CBFSC_GET_TOTAL_POW, &this_test);
			new_manipulate_all(player, card, i, &this_test, KILL_REMOVE);
			token.t_player = i;
			token.qty = sapro;
			generate_token(&token);
		}
		planeswalk_to(player, get_new_plane(get_actual_plane(player)), 0);
	}

	return generic_plane(player, card, event);
}

static void gotd_effect(int player){
	int *deck = deck_ptr[player];
	if( deck[0] != -1 ){
		int count = 0;
		int good = 0;
		while( deck[count] != -1 ){
				if(  is_what(-1, deck[0], TYPE_CREATURE) ){
					good = 1;
					break;
				}
				count++;
		}
		show_deck( HUMAN, deck, count+1, "Card revealed by Grove of the Dreampods.", 0, 0x7375B0 );
		if( good == 1 ){
			put_into_play_a_card_from_deck(player, player, count);
			count--;
		}
		if( count > -1 ){
			count++;
			while( count > 0 ){
					int tpob = 0;
					if( count > 1 ){
						tpob = internal_rand(count);
					}
					deck[count_deck(player)] = deck[tpob];
					remove_card_from_deck(player, tpob);
					count--;
			}
		}
	}
}

int card_grove_of_the_dreampods(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		gotd_effect(player);
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gotd_effect(player);
	}

	if( event == EVENT_CHAOS ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(2) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_hedron_fields_of_agadeem(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if ((event == EVENT_ATTACK_LEGALITY || event == EVENT_BLOCK_LEGALITY)
		&& get_power(affected_card_controller, affected_card) >= 7){
		event_result = 1;
	}

	if( event == EVENT_CHAOS ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELDRAZI, &token);
		token.pow = 7;
		token.tou = 7;
		generate_token(&token);
	}
	return generic_plane(player, card, event);
}

int card_jund2(int player, int card, event_t event){
	/* *** Jund	""
	 * Plane - Alara
	 * Whenever a player casts a |Sblack, |Sred, or |Sgreen creature spell, it gains devour 5.
	 * Whenever you roll |C, put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
	}

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK | COLOR_TEST_GREEN | COLOR_TEST_RED, 0, 0, 0, -1, 0) ){
		add_state(instance->targets[1].player, instance->targets[1].card, STATE_OUBLIETTED);
		int result = devouring(instance->targets[1].player, instance->targets[1].card);
		remove_state(instance->targets[1].player, instance->targets[1].card, STATE_OUBLIETTED);
		add_1_1_counters(instance->targets[1].player, instance->targets[1].card, 5*result);
	}

	return generic_plane(player, card, event);
}

int card_kessig2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WEREWOLF);
			int legacy = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, KEYWORD_TRAMPLE, 0);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[3].player = PAUE_RESET_SUBTYPES_AT_EOT;
		}
	}

	if( event == EVENT_PREVENT_DAMAGE && affected_card_controller == player ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
				! has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_WEREWOLF)
			  ){
				damage->info_slot = 0;
			}
		}
	}
	return generic_plane(player, card, event);
}

int card_kharasha_foothills(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0) ){
			int result = 0;
			while( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					result++;
			}
			if( result > 0 && pick_target(&td, "TARGET_CREATURE") ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, result, player, card);
				instance->number_of_targets = 1;
			}
		}
	}
	return generic_plane(player, card, event);
}

int card_kilnspire_district(int player, int card, event_t event){

	/* Kilnspire District	""
	 * Plane - Ravnica
	 * When you planeswalk to Kilnspire District or at the beginning of your precombat main phase, put a charge counter on Kilnspire District, then add |R to
	 * your mana pool for each charge counter on it.
	 * Whenever you roll |C, you may pay |X. If you do, Kilnspire District deals X damage to target creature or player. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		add_counter(player, card, COUNTER_CHARGE);
		produce_mana(player, COLOR_RED, count_counters(player, card, COUNTER_CHARGE));
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[1].player != 66 ){
		add_counter(player, card, COUNTER_CHARGE);
		produce_mana(player, COLOR_RED, count_counters(player, card, COUNTER_CHARGE));
		instance->targets[1].player = 66;
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, x_value);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_lair_of_the_ashen_idol(int player, int card, event_t event){
	/* *** Lair of the Ashen Idol	""
	 * Plane - Azgol
	 * At the beginning of your upkeep, sacrifice a creature. If you can't, planeswalk.
	 * Whenever you roll |C, any number of target players each put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( ! sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			planeswalk_to(player, get_new_plane(get_actual_plane(player)), 0);
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = player;
		instance->targets[0].card = -1;
		int can_target_self = would_valid_target(&td);

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		int can_target_opp = would_valid_target(&td);

		int choice = DIALOG(player, card, EVENT_ACTIVATE,
							DLG_NO_STORAGE, DLG_NO_CANCEL,
							"You get a Zombie", can_target_self, 2,
							"Opponent gets a Zombie", can_target_opp, 0,
							"Both players get Zombies", can_target_self && can_target_opp, 0,
							"Nobody gets a Zombie", 1, 1);

		if( choice == 1 || choice == 3 ){
			generate_token_by_id(player, card, CARD_ID_ZOMBIE);
		}
		if( choice == 2 || choice == 3 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
			token.t_player = 1-player;
			generate_token(&token);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_mount_keralia(int player, int card, event_t event){

	/* Mount Keralia	""
	 * Plane - Regatha
	 * At the beginning of your end step, put a pressure counter on Mount Keralia.
	 * When you planeswalk away from Mount Keralia, it deals damage equal to the number of pressure counters on it to each creature and each planeswalker.
	 * Whenever you roll |C, prevent all damage that planes named Mount Keralia would deal this game to permanents you control. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		create_id_legacy(player, card, -1, -1, 0);
	}

	if( event == EVENT_PLANESWALK_OUT ){
		int amount = count_counters(player, card, COUNTER_PRESSURE);
		if( amount > 0 ){
			int i;
			for(i=0; i<2; i++){
				if( ! check_for_id_legacy(i, get_id(player, card)) ){
					int count = active_cards_count[i]-1;
					while( count > -1 ){
							if( in_play(i, count) ){
								if( is_what(i, count, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER) ){
									damage_creature(i, count, amount, player, card);
								}
							}
							count--;
					}
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		add_counter(player, card, COUNTER_PRESSURE);
	}

	return generic_plane(player, card, event);
}

int card_nephalia2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CHAOS ){
		char msg[100] = "Select a card to return to hand.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);

		if( ! graveyard_has_shroud(2) && count_graveyard(player) > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	if( eot_trigger(player, card, event) ){
		const int *grave = get_grave(player);
		mill(player, 7);
		if( grave[0] != -1 ){
			int count = count_graveyard(player);
			int rnd = 0;
			if( count > 1 ){
				rnd = internal_rand(count);
			}
			add_card_to_hand(player, grave[rnd]);
			remove_card_from_grave(player, rnd);
		}
	}

	return generic_plane(player, card, event);
}

int card_norns_dominion(int player, int card, event_t event){

	/* Norn's Dominion	""
	 * Plane - New Phyrexia
	 * When you planeswalk away from Norn's Dominion, destroy each nonland permanent without a fate counter on it, then remove all fate counters from all
	 * permanents.
	 * Whenever you roll |C, you may put a fate counter on target permanent. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_OUT ){
		oblivion_stone_effect();
	}

	if(  event == EVENT_CHAOS  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FATE);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_onakke_catacomb(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_SET_COLOR && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		event_result = COLOR_TEST_BLACK;
	}

	if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
		deathtouch(affected_card_controller, affected_card, event);
	}

	if( event == EVENT_CHAOS ){
		pump_subtype_until_eot(player, card, player, -1, 1, 0, KEYWORD_FIRST_STRIKE, 0);
	}

	return generic_plane(player, card, event);
}

int card_orochi_colony(int player, int card, event_t event){

	/* Orochi Colony	""
	 * Plane - Kamigawa
	 * Whenever a creature you control deals combat damage to a player, you may search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library.
	 * Whenever you roll |C, target creature is unblockable this turn. */

	vanguard_card(player, card, event, 7, 20, 0);

	if( subtype_deals_damage(player, card, event, player, -1, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL) ){
		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_orzhova2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if (event == EVENT_CHAOS){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");
		if( new_special_count_grave(1-player, &this_test) && ! graveyard_has_shroud(2) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
		}
	}

	if( event == EVENT_PLANESWALK_OUT ){
		reanimate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, REANIMATE_DEFAULT);
		reanimate_all(1-player, -1, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, REANIMATE_DEFAULT);
	}

	return generic_plane(player, card, event);
}

int card_prahv(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( instance->targets[2].player < 0 ){
		instance->targets[2].player = 0;
	}

	if (event == EVENT_DECLARE_ATTACKERS && current_turn == player && count_attackers(player) > 0){
		instance->targets[2].player |= 1;
	}

	if( !(instance->targets[2].player & 2) && specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		instance->targets[2].player |= 2;
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && (instance->targets[2].player & 1) ){
		infinite_casting_cost();
	}

	if( (instance->targets[2].player & 2) ){
		nobody_can_attack(player, card, event, player);
	}

	if( event == EVENT_CHAOS ){
		gain_life(player, hand_count[player]);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_quicksilver_sea(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_PLANESWALK_IN ){
		scry(player, 4);
	}

	if( instance->targets[1].player != 66 ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		scry(player, 4);
	}

	if( event == EVENT_CHAOS ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			if( show_deck( player, deck, 1, "Click on the card if you wish to play it", 0, 0x7375B0 ) != -1 && can_legally_play_iid(player, deck[0]) ){
				play_card_in_deck_for_free(player, player, 0);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_selesnya_loft_gardens(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if (instance->targets[2].player == 66 && (event == EVENT_TAP_CARD || event == EVENT_COUNT_MANA) && affected_card_controller == player){
		card_mana_flare(player, card, event);
	}

	if( event == EVENT_CHAOS ){
		instance->targets[2].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_stensia(int player, int card, event_t event){

	/* Stensia	""
	 * Plane - Innistrad
	 * Whenever a creature deals damage to one or more players for the first time in a turn, put a +1/+1 counter on it.
	 * Whenever you roll |C, each creature you control gains "|T: This creature deals 1 damage to target player" until end of turn. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_source_player == player && damage->damage_target_card == -1 ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
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

					if( good == 1 && ! check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_DAMAGED_PLAYER) ){
						if( instance->info_slot < 0 ){
							instance->info_slot = 0;
						}
						instance->targets[instance->info_slot].player = damage->damage_source_player;
						instance->targets[instance->info_slot].card = damage->damage_source_card;
						instance->info_slot++;
					}
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i].player != -1 && instance->targets[i].card != -1 &&
						in_play(instance->targets[i].player, instance->targets[i].card)
					  ){
						add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
						set_special_flags(instance->targets[i].player, instance->targets[i].card, SF_DAMAGED_PLAYER);
					}
				}
				instance->info_slot = 0;
		}
	}

	if( event == EVENT_CHAOS ){
		instance->targets[2].player = 66;
	}

	if( eot_trigger(player, card, event) || leaves_play(player, card, event) ){
		remove_special_flags(player, -1, SF_DAMAGED_PLAYER);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_LAND;
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE ){
		if( instance->targets[2].player == 66 && can_target(&td) && can_target(&td1) ){
			return 1;
		}
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			if( instance->targets[2].player == 66 && can_target(&td) && can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Cast the Planar Dice\n Tap & damage a player\n Cancel", 1);
			}
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->targets[1] = instance->targets[0];
				tap_card(instance->targets[0].player, instance->targets[0].card);
				if( pick_target(&td1, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
					instance->info_slot = 10;
				}
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 10 ){
			if( valid_target(&td1) ){
				damage_player(instance->targets[0].player, 1, instance->targets[1].player, instance->targets[1].card);
			}
		}
		else{
			casting_the_planar_dice(player, instance->parent_card, instance->info_slot);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
	}

	return generic_plane(player, card, event);
}

int card_takenuma2(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && reason_for_trigger_controller == player && trigger_cause_controller == player ){
			if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						draw_cards(player, 1);
				}
			}
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

int card_talon_gates(int player, int card, event_t event){

	/* Talon Gates	""
	 * Plane - Dominaria
	 * Any time you could cast a sorcery, you may exile a nonland card from your hand with X time counters on it, where X is its converted mana cost. If the
	 * exiled card doesn't have suspend, it gains suspend.
	 * Whenever you roll |C, remove two time counters from each suspended card you own. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY );
	td.zone = TARGET_ZONE_HAND;
	td.illegal_type = TYPE_LAND;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_CAN_ACTIVATE && can_sorcery_be_played(player, event) ){
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) ){
			return 1;
		}
		return can_target(&td);
	}

	if( event == EVENT_ACTIVATE){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) ){
			if( can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Casting the Planar Dice\n Suspend a card\n Cancel", 1);
			}
		}
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
				if( pick_target(&td, "TARGET_CARD") ){
					instance->number_of_targets = 1;
					instance->info_slot = 10;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 10 ){
			suspend_a_card(instance->parent_controller, instance->parent_card, player, instance->targets[0].card, get_cmc(player, instance->targets[0].card));
		}
		else{
			casting_the_planar_dice(instance->parent_controller, instance->parent_card, instance->info_slot);
		}
	}

	if( player == AI && current_turn != player ){
		if( can_target(&td) && eot_trigger(player, card, event) ){
			if( pick_target(&td, "TARGET_CARD") ){
				instance->number_of_targets = 1;
				suspend_a_card(player, card, player, instance->targets[0].card, get_cmc(player, instance->targets[0].card));
			}
		}
	}

	if( event == EVENT_CHAOS){
		card_instance_t* inst;
		int c;
		for (c = 0; c < active_cards_count[player]; ++c){
			if ((inst = in_play(player, c))
				&& ((inst->internal_card_id == LEGACY_EFFECT_CUSTOM && inst->info_slot == (int)generic_suspend_legacy)
					|| cards_data[inst->internal_card_id].id == CARD_ID_GREATER_GARGADON_SUSPENDED))
				remove_counters(player, c, COUNTER_TIME, 2);
		}
	}

	return generic_plane(player, card, event);
}

int card_the_zephyr_maze(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if( event == EVENT_POWER && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
			event_result+=2;
		}
		else{
			event_result-=2;
		}
	}

	if( event == EVENT_CHAOS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
			instance->number_of_targets = 1;
		}
	}

	return generic_plane(player, card, event);
}

// trails of the mage-ring --> skipped

int card_truga_jungle(int player, int card, event_t event){

	vanguard_card(player, card, event, 7, 20, 0);

	if(event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = 0;	// generic_plane() sets this

		int choice = 1;
		if( generic_plane(player, card, EVENT_CAN_ACTIVATE) ){
			int can_tap_land = permanents_you_control_can_tap_for_mana_all_one_color(player, card, EVENT_CAN_ACTIVATE, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
			if( can_tap_land){
				choice = do_dialog(player, player, card, -1, -1, " Get mana\n Cast the Planar Dice\n Cancel", 1);
			}
		} else {
			choice = 0;
		}

		if( choice == 0 ){
			return permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
		}
		else if( choice == 1 ){
			return generic_plane(player, card, event);
		}
		else{
			spell_fizzled = 1;
		}
		return 0;
	}

	int result = permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
	if( event == EVENT_CAN_ACTIVATE && result ){
		return 1;
	}

	if( event == EVENT_CHAOS ){
		int *deck = deck_ptr[player];
		int max = 3;
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		if( max > 0 ){
			show_deck( HUMAN, deck, 3, "Cards revealed by Truga Jungle", 0, 0x7375B0 );
			int i;
			int rev = max;
			for(i=0; i<rev; i++){
				if( is_what(-1, deck[i], TYPE_LAND) ){
					add_card_to_hand(player, deck[i]);
					remove_card_from_deck(player, i);
					max--;
				}
			}
			if( max > 0 ){
				put_top_x_on_bottom(player, player, max);
			}
		}
	}

	return generic_plane(player, card, event);
}

int card_windriddle_palaces(int player, int card, event_t event){

	/* Windriddle Palaces	""
	 * Plane - Belenon
	 * Players play with the top card of their libraries revealed.
	 * You may play the top card of any player's library.
	 * Whenever you roll |C, each player puts the top card of his or her library into his or her graveyard. */

	card_instance_t *instance = get_card_instance( player, card );

	vanguard_card(player, card, event, 7, 20, 0);

	int *deck_player = deck_ptr[player];
	int *deck_opp = deck_ptr[1-player];

	reveal_top_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		if( deck_player[0] != -1 ){
			if( can_legally_play_iid_now(player, deck_player[0], event) && has_mana_to_cast_id(player, event, cards_data[deck_player[0]].id) ){
				return 1;
			}
		}
		if( deck_opp[0] != -1 ){
			if( can_legally_play_iid_now(player, deck_opp[0], event) && has_mana_to_cast_id(player, event, cards_data[deck_opp[0]].id) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int ai_choice = 1;
		int mode = (1<<3);
		char buffer[500];
		int pos = 0;
		if( has_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY)) && can_sorcery_be_played(player, event) ){
			pos += scnprintf(buffer + pos, 500-pos, " Cast the Planar Dice\n");
			mode |= (1<<0);
		}
		if( deck_player[0] != -1 ){
			if( can_legally_play_iid(player, deck_player[0]) && has_mana_to_cast_id(player, event, cards_data[deck_player[0]].id) ){
				card_ptr_t* c = cards_ptr[ cards_data[deck_player[0]].id  ];
				pos += scnprintf(buffer + pos, 500-pos, " Play %s\n", c->name);
				mode |= (1<<1);
			}
		}
		if( deck_opp[0] != -1 ){
			if( can_legally_play_iid(player, deck_opp[0]) && has_mana_to_cast_id(player, event, cards_data[deck_opp[0]].id) ){
				card_ptr_t* c = cards_ptr[ cards_data[deck_opp[0]].id  ];
				pos += scnprintf(buffer + pos, 500-pos, " Play %s\n", c->name);
				mode |= (1<<2);
				ai_choice = 2;
			}
		}
		int choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
		while( !( mode & (1<<choice)) ){
				choice++;
		}

		instance->info_slot = 10;

		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_ENERGY));
			if( spell_fizzled != 1 ){
				instance->info_slot = internal_rand(6)+1;
				add_counter(player, card, COUNTER_ENERGY);
			}
		}
		else if( choice == 1 ){
				if (charge_mana_from_id(player, -1, event, cards_data[deck_player[0]].id)){
					play_card_in_deck_for_free(player, player, 0);
					cant_be_responded_to = 1;
				}
		}
		else if( choice == 2 ){
				charge_mana_from_id(player, -1, event, cards_data[deck_opp[0]].id);
				if( spell_fizzled != 1 ){
					play_card_in_deck_for_free(player, 1-player, 0);
					cant_be_responded_to = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot != 10 ){
			casting_the_planar_dice(player, instance->parent_card, instance->info_slot);
		}
	}

	if( event == EVENT_CHAOS ){
		mill(player, 1);
		mill(1-player, 1);
	}

	return generic_plane(player, card, event);
}

