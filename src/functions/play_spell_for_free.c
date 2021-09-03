#include "manalink.h"
static int free_spell_card = -1;
static int free_spell_player = -1;

int has_no_mana_cost(int player, int card){
	int csvid = player == -1 ? cards_data[card].id : get_id(player, card);
	int result = 0;
	switch( csvid ){
			case CARD_ID_HYPERGENESIS:
			case CARD_ID_ANCESTRAL_VISION:
			case CARD_ID_LIVING_END:
			case CARD_ID_RESTORE_BALANCE:
			case CARD_ID_WHEEL_OF_FATE:
			case CARD_ID_LOTUS_BLOOM:
				result = 1;
				break;
			default:
				break;
	}
	return result;
}

int played_for_free(int player, int card){
	if( player > -1 && card > -1 ){
		return check_special_flags(player, card, SF_PLAYED_FOR_FREE);
	}
	return 0;
}

int not_played_from_hand(int player, int card){
	if( check_special_flags(player, card, SF_NOT_CAST | SF_PLAYED_FROM_GRAVE | SF_PLAYED_FROM_DECK | SF_PLAYED_FROM_EXILE) ){
		return 1;
	}
	return 0;
}

static int real_copy_spell(int player, int internal_id, int special_flags, int special_flags3, card_instance_t* copied){

	int card_added = add_card_to_hand( player, internal_id );
	if (copied){
		// Copy X value and target struct to preserve special flags and such.
		card_instance_t* inst = get_card_instance(player, card_added);
		inst->info_slot = copied->info_slot;
		inst->eot_toughness = copied->eot_toughness;
		int i;
		for(i=0; i<19; i++){
			inst->targets[i] = copied->targets[i];
		}
		set_special_flags2(player, card_added, SF2_COPIED_FROM_STACK);
	}

	free_spell_player = player;
	free_spell_card = card_added;
	card_data_t* card_d = &cards_data[ internal_id ];
	convert_to_token(player, card_added);
	set_special_flags(player, card_added, special_flags);
	set_special_flags3(player, card_added, special_flags3);

	if( put_card_on_stack(player, card_added, 0) ){
		put_card_on_stack(player, card_added, 1);
		put_card_on_stack3(player, card_added) ;

		if(  card_d->type & TYPE_LAND ){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}
	else if( can_legally_play_iid(player, internal_id)){

			--hand_count[player];
			real_put_into_play(player, card_added);

			// put the spell on the stack (doesn't actually work...)
			card_on_stack = card_added;
			card_on_stack_controller = player;

			// if this was a land, count it as played
			if( card_d->type & TYPE_LAND ){
				land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
			}
	}
	else{
		obliterate_card(player, card_added);
	}
	free_spell_card = -1;

	return card_added;
}

int copy_spell(int player, int csvid)
{
  return real_copy_spell(player, get_internal_card_id_from_csv_id(csvid), SF_NOT_CAST|SF_PLAYED_FOR_FREE, 0, NULL);
}

int copy_spell_from_stack(int player, int t_player, int t_card){
	card_instance_t* instance = get_card_instance(t_player, t_card);
	int iid = instance->internal_card_id;
	if (iid == -1){
		iid = instance->backup_internal_card_id;
	}
	return real_copy_spell(player, iid, SF_NOT_CAST | SF_PLAYED_FOR_FREE, 0, instance);
}

target_t totally_bletcherous_hack_dont_reset_x_value = {-1, -1};
static int totally_bletcherous_hack_put_card_on_stack0_without_resetting_x_value(int player, int card)
{
  /* This circumlocution is needed because put_card_on_stack() clears 0x78676C itself; fortunately, a function we have control over is called before its value
   * is used.  Yet another consequence of always forcing these to be free. */
  target_t old_hack_val = totally_bletcherous_hack_dont_reset_x_value;	// struct copy
  totally_bletcherous_hack_dont_reset_x_value.player = player;
  totally_bletcherous_hack_dont_reset_x_value.card = card;
  int rval = put_card_on_stack(player, card, 0);
  totally_bletcherous_hack_dont_reset_x_value = old_hack_val;	// struct copy
  EXE_DWORD(0x78676C) = 0;
  return rval;
}

static int real_play_spell_for_free(int player, int card, int special_flags){

	free_spell_player = player;
	free_spell_card = card;
	int internal_id = get_card_instance(player, card)->internal_card_id;
	card_data_t* card_d = &cards_data[internal_id];

	set_special_flags(player, card, SF_PLAYED_FOR_FREE | special_flags);

	int rval = 1;

	if( totally_bletcherous_hack_put_card_on_stack0_without_resetting_x_value(player, card) ){
		put_card_on_stack(player, card, 1);
		put_card_on_stack3(player, card) ;

		if(  card_d->type & TYPE_LAND ){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}
	else if( can_legally_play_iid(player, internal_id)){

			--hand_count[player];
			real_put_into_play(player, card);

			// put the spell on the stack (doesn't actually work...)
			card_on_stack = card;
			card_on_stack_controller = player;

			// if this was a land, count it as played
			if( card_d->type & TYPE_LAND ){
				land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
			}
			else{
				/* This is almost certainly wrong - even if this always represents "You may cast (whatever) without paying its mana cost", then this should
				 * increment stormcolor too; should increment stormcreature for creatures with flash played on an opponent's turn; and shouldn't ever increment
				 * storm for a creature spell without incrementing stormcreature.  But again, I'll leave it. */
				int mode = 0;
				if( (card_d->type & TYPE_CREATURE) && player == current_turn){
					mode = STORM_CREATURE;
				}
				increment_storm_count(player, mode);
			}
	}
	else{
		remove_special_flags(player, card, SF_PLAYED_FOR_FREE | special_flags);
		rval = 0;
	}
	free_spell_card = -1;

	return rval;
}

int play_card_in_deck_for_free(int player, int t_player, int deck_pos){
	int *deck = deck_ptr[t_player];
	int card_added = -1;
	if( deck[deck_pos] != -1 ){
		int iid = deck[deck_pos];
		if( can_legally_play_iid(player, iid) ){
			remove_card_from_deck(t_player, deck_pos);
			card_added = add_card_to_hand(player, iid);
			if (player != t_player){
				get_card_instance(player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
			}
			if ( ! real_play_spell_for_free(player, card_added, SF_PLAYED_FROM_DECK) ){
				obliterate_card(player, card_added);
				card_added = -1;
			}
		}
	}
	return card_added;
}

static int play_card_in_grave_for_free_impl(int player, int t_player, int g_pos, int special_flags){
	const int *grave = get_grave(t_player);
	int card_added = -1;
	if( grave[g_pos] != -1 ){
		int iid = grave[g_pos];
		if( can_legally_play_iid(player, iid) ){
			remove_card_from_grave(t_player, g_pos);
			card_added = add_card_to_hand(player, iid);
			if (player != t_player){
				get_card_instance(player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
			}
			if( ! real_play_spell_for_free(player, card_added, (special_flags | SF_PLAYED_FROM_GRAVE)) ){
				obliterate_card(player, card_added);
				card_added = -1;
			}
		}
	}
	return card_added;
}

int play_card_in_grave_for_free(int player, int t_player, int g_pos){
	return play_card_in_grave_for_free_impl(player, t_player, g_pos, 0);
}

int play_card_in_grave_for_free_and_exile_it(int player, int t_player, int g_pos){
	return play_card_in_grave_for_free_impl(player, t_player, g_pos, SF_EXILE_ON_RESOLUTION);
}

int flashback_spell(int player, int g_pos){
	return play_card_in_grave_for_free_impl(player, player, g_pos, SF_EXILE_ON_RESOLUTION | SF_FLASHBACK);
}

int play_card_in_exile_for_free_impl(int player, int t_player, int csvid, int special_flags2, int mana_paid_for_this){
	int card_added = -1;
	int iid = get_internal_card_id_from_csv_id(csvid);
	if( can_legally_play_iid(player, iid) ){
		remove_card_from_rfg(t_player, csvid);
		card_added = add_card_to_hand(player, iid);
		get_card_instance(player, card_added)->targets[3].card = mana_paid_for_this;
		set_special_flags2(player, card_added, special_flags2);
		if (player != t_player){
			if( player == HUMAN && t_player == AI ){
				add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
			else{
				remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
		}
		if (!real_play_spell_for_free(player, card_added, SF_PLAYED_FROM_EXILE)){
			obliterate_card(player, card_added);
			card_added = -1;
		}
	}
	return card_added;
}

int play_card_in_exile_for_free(int player, int t_player, int csvid){
	return play_card_in_exile_for_free_impl(player, t_player, csvid, 0, 0);
}

// Needed for counterspells to correctly allow the Commander to be putted into the Command Zone and to cards that need to know how many mana was paid to play them
int play_card_in_exile_for_free_commander(int player, int t_player, int csvid, int mana_paid_for_this){
	return play_card_in_exile_for_free_impl(player, player, csvid, SF2_COMMANDER, mana_paid_for_this);
}

void play_card_in_hand_for_free(int player, int card){
	real_play_spell_for_free(player, card, 0);
}

void play_card_in_hand_madness(int player, int card){
	//We aren't really playing a card from exile, as per Madness ruling, just faking it.
	real_play_spell_for_free(player, card, SF_PLAYED_FROM_EXILE);
}

void opponent_plays_card_in_your_hand_for_free(int player, int card){
	int iid = get_card_instance(player, card)->internal_card_id;
	obliterate_card(player, card);
	int card_added = add_card_to_hand(1-player, iid);
	if( player == AI ){
		add_state(1-player, card_added, STATE_OWNED_BY_OPPONENT);
	}
	else{
		remove_state(1-player, card_added, STATE_OWNED_BY_OPPONENT);
	}
	real_play_spell_for_free(1-player, card_added, 0);
}

int put_into_play(int player, int card){
	// This should be used in any case when a card is directly put into play from zone. "put_into_play_exe" is reserved for any other use.
	set_special_flags(player, card, SF_NOT_CAST|SF_PLAYED_FOR_FREE);
	if (player >= 0 && card >= 0 && in_hand(player, card)){
		--hand_count[player];
	}
	real_put_into_play(player, card);
	return card;
}

int compute_and_check_casting_cost(int player, int unused, int card)
{
  // 200bb14
  // 0x402930.  Parameter unused is always the same as player when called from the exe.

  int result = 0;

  int old_73825c = EXE_DWORD(0x73825C);	// Also written to in get_abilities, stashed and restored in push_affected_card_stack()/pop_affected_card_stack(), and saved and loaded.  Doesn't seem to be read ever except locally.
  int old_affected_card = affected_card;
  int old_affected_card_controller = affected_card_controller;

  if (player == -1 || card == -1)
	goto epilogue;

  int8_t* cost_xbugrwaU = (int8_t*)&COST_COLORLESS;
  int8_t* byte_55CF20 = (int8_t*)EXE_BYTE_PTR(0x55CF20);

  memset(cost_xbugrwaU, 0, 8);
  memset(byte_55CF20, 0, 8);	// Amount of surplus free mana?

  card_instance_t* instance = get_card_instance(player, card);
  affected_card_controller = player;
  affected_card = card;

  EXE_DWORD(0x73825C) = instance->internal_card_id;
  if (EXE_DWORD(0x73825C) < 0)
	goto epilogue;

  int free_spell = card == free_spell_card && player == free_spell_player;

  card_instance_t* inst;
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((inst = in_play(p, c)))
		{
		  card_data_t* cd = &cards_data[inst->internal_card_id];
		  if (cd->extra_ability & EA_PLAY_COST)
			call_card_fn((void*)cd->code_pointer, inst, p, c, EVENT_MODIFY_COST_GLOBAL);
		}

  card_data_t* cd = &cards_data[EXE_DWORD(0x73825C)];
  if (cd->new_field & 1)	// "Modifies Casting Cost" column in ct_all.csv
	call_card_fn((void*)cd->code_pointer, instance, player, card, EVENT_MODIFY_COST);

  card_ptr_t* cp = cards_ptr[cd->id];

  if (!free_spell)
	{
	  int cless = cp->req_colorless;
	  if (cless == 40)
		cless = 0;
	  cost_xbugrwaU[0] += cless;
	}
  if (cost_xbugrwaU[0] < 0)
	{
	  byte_55CF20[0] = cost_xbugrwaU[0];
	  cost_xbugrwaU[0] = 0;
	}

#define COMPUTE_COLOR(col, req_col)				\
  if (!free_spell)								\
	cost_xbugrwaU[col] += cp->req_col;			\
  if (cost_xbugrwaU[col] < 0)					\
	{											\
	  byte_55CF20[col] = cost_xbugrwaU[col];	\
	  cost_xbugrwaU[col] = 0;					\
	}

  COMPUTE_COLOR(COLOR_BLACK, req_black);
  COMPUTE_COLOR(COLOR_BLUE, req_blue);
  COMPUTE_COLOR(COLOR_GREEN, req_green);
  COMPUTE_COLOR(COLOR_RED, req_red);
  COMPUTE_COLOR(COLOR_WHITE, req_white);
#undef COMPUTE_COLOR

  cost_xbugrwaU[COLOR_ARTIFACT] = 0;

  if (cd->type & TYPE_ARTIFACT)
	result = has_mana_multi_a(player, cost_xbugrwaU[COLOR_COLORLESS], cost_xbugrwaU[COLOR_BLACK], cost_xbugrwaU[COLOR_BLUE],
							  cost_xbugrwaU[COLOR_GREEN], cost_xbugrwaU[COLOR_RED], cost_xbugrwaU[COLOR_WHITE]);
  else
	{
	  memcpy(&MANA_COLORLESS, cost_xbugrwaU, 8);
	  result = check_mana_multi(player);
	}

 epilogue:
  if (EXE_BYTE(0x4EF160) == 1)	// TENTATIVE_next_spell_always_castable - set only by card_chain_lightning()
	{
	  EXE_BYTE(0x4EF160) = 0;
	  result = 1;
	}

  affected_card_controller = old_affected_card_controller;
  affected_card = old_affected_card;
  EXE_DWORD(0x73825C) = old_73825c;

  return result;
}

static int effect_may_play_card_from_exile(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  int whose_exile_zone = instance->targets[0].player;
  int csvid = instance->targets[0].card;
  int mode = instance->targets[1].player;
  // instance->targets[1].card: set to 42 while activating

  if (event == EVENT_STATIC_EFFECTS)
	{
	  if (!(mode & MPCFE_FACE_DOWN) && !check_rfg(whose_exile_zone, csvid))
		kill_card(player, card, KILL_REMOVE);
	  return 0;
	}

  if (event == EVENT_CLEANUP && (mode & MPCFE_UNTIL_EOT))
	kill_card(player, card, KILL_REMOVE);

  if (event == EVENT_BEGIN_TURN && (mode & MPCFE_UNTIL_END_OF_YOUR_NEXT_TURN) && current_turn == player)
	instance->targets[1].player |= MPCFE_UNTIL_EOT;

  if (!IS_ACTIVATING(event))
	return 0;

  int can_look_at_card = (mode & MPCFE_FACE_DOWN) && !IS_AI(player);

  if (event == EVENT_CAN_ACTIVATE && instance->targets[1].card == 42)	// can't ever activate while paying mana to activate it
	return 0;

  int can_play, iid, cost;
  if (event == EVENT_RESOLVE_ACTIVATION)
	can_play = iid = cost = -1;
  else
	{
	  iid = get_internal_card_id_from_csv_id(csvid);

	  can_play = can_legally_play_iid_now(player, iid, event);	// 0 or 1 or 99

	  if (!(mode & MPCFE_FACE_DOWN) && !check_rfg(whose_exile_zone, csvid))
		{
		  can_play = 0;
		  cost = 0;
		}
	  else if (mode & MPCFE_FOR_FREE)
		cost = 0;
	  else if (mode & MPCFE_FOR_CMC)
		{
		  int cmc = get_cmc_by_id(csvid);
		  cost = get_updated_casting_cost(player, -1, iid, event, cmc);
		  if (!has_mana(player, is_what(-1, iid, TYPE_ARTIFACT) ? COLOR_ARTIFACT : COLOR_COLORLESS, cost) || has_no_mana_cost(-1, iid))
			can_play = 0;
		}
	  else
		{
		  cost = 0;
		  if (!has_mana_to_cast_iid(player, event, iid) || has_no_mana_cost(-1, iid))
			can_play = 0;
		}
	}

  if (event == EVENT_CAN_ACTIVATE)
	return can_play ? can_play : can_look_at_card;	// in the latter case, can always activate to look at the card, but if can_play is 99, return that instead

  if (event == EVENT_ACTIVATE)
	{
	  if (can_look_at_card && DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_FULLCARD_CSVID(instance->display_pic_csv_id),
									 "Play card", can_play, 1,
									 "Look at card", 1, -1) == 2)
		{
		  // About the only way to show information to only one player in a multiplayer game, other than to put a card in his hand.
		  show_deck(player, &iid, 1, "Face-down card in exile", 0, (int)EXE_STR(0x78f0a8));	// DIALOGBUTTONS[0] - "OK"
		  cancel = 1;
		  return 0;
		}

	  instance->targets[1].card = 42;

	  if (mode & MPCFE_FOR_FREE)
		cancel = 0;	// just in case
	  else if (mode & MPCFE_FOR_CMC)
		charge_mana(player, is_what(-1, iid, TYPE_ARTIFACT) ? COLOR_ARTIFACT : COLOR_COLORLESS, cost);
	  else
		charge_mana_from_id(player, -1, event, csvid);

	  instance->targets[1].card = -1;

	  if (cancel == 1)
		return 0;

	  kill_card(player, card, KILL_REMOVE);

	  if (mode & MPCFE_FACE_DOWN)
		add_csvid_to_rfg(whose_exile_zone, csvid);

	  play_card_in_exile_for_free(player, whose_exile_zone, csvid);
	  cant_be_responded_to = 1;
	}

  return 0;
}

// If MPCFE_FACE_DOWN is set, the card should be obliterated instead of being put in to exile; otherwise, it should already be in exile before calling this.
int create_may_play_card_from_exile_effect(int player, int card, int whose_exile_zone, int csvid, mpcfe_mode_t mode)
{
  int leg;
  if (mode & MPCFE_ATTACH)
	leg = create_targetted_legacy_activate(player, card, effect_may_play_card_from_exile, player, card);
  else
	leg = create_legacy_activate(player, card, effect_may_play_card_from_exile);

  if (leg != -1)
	{
	  card_instance_t* legacy = get_card_instance(player, leg);
	  legacy->targets[0].player = whose_exile_zone;
	  legacy->targets[0].card = csvid;
	  legacy->targets[1].player = mode;
	  if (!(mode & MPCFE_FACE_DOWN))
		create_card_name_legacy(player, leg, csvid);	// EVENT_SET_LEGACY_EFFECT_NAME and EVENT_SET_LEGACY_EFFECT_TEXT don't work on activateable effects
	}

  return leg;
}

static int spell_has_flashback_legacy(int player, int card, event_t event)
{
  /* targets:
   * [0].card: iid of chosen card
   * [1].player: graveyard_source value
   * [1].card: tentative position in graveyard
   * [2].player: flags. If it's left to "-1", it just acts like a normal flashback
						1<<0 --> can play the card without paying its mana cost (Sins of the Past)
   */

  if (event == EVENT_CLEANUP
	  || (event == EVENT_STATIC_EFFECTS && get_card_instance(player, card)->targets[1].card == -1))	// no longer in graveyard
	kill_card(player, card, KILL_REMOVE);

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE)
	{
	  instance->targets[1].card = find_in_graveyard_by_source(player, instance->targets[1].player, instance->targets[1].card);

		if (instance->targets[1].card != -1	// must still be in graveyard
			&& ((instance->targets[2].player & FBL_NO_MANA_COST)
				|| (has_mana_to_cast_iid(player, event, instance->targets[0].card) && ! has_no_mana_cost(-1, instance->targets[0].card) ))
		  ){
			int rval = can_legally_play_iid(player, instance->targets[0].card);
			if( rval == 99 && (cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) ){
				return 99;
			}
			if( rval && !(cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) && ! can_sorcery_be_played(player, event) ){
				rval = 0;
			}
			return rval;
		}
	}

  if (event == EVENT_ACTIVATE
	  && ((instance->targets[2].player & FBL_NO_MANA_COST)
		  || charge_mana_from_id(player, -1, event, cards_data[instance->targets[0].card].id)))
	{
	  kill_card(player, card, KILL_REMOVE);
	  play_card_in_grave_for_free_and_exile_it(player, player, instance->targets[1].card);
	  cant_be_responded_to = 1;	// already had opportunity to respond to the spell; no response to this fake activation
	}

  return 0;
}

void create_spell_has_flashback_legacy(int player, int card, int graveyard_pos, flashback_legacy_t mode)
{
  if (graveyard_pos < 0)
	return;

  int leg = create_legacy_activate(player, card, &spell_has_flashback_legacy);
  card_instance_t* legacy = get_card_instance(player, leg);
  legacy->targets[0].card = get_grave(player)[graveyard_pos];
  legacy->targets[1].player = graveyard_source[player][graveyard_pos];
  legacy->targets[1].card = graveyard_pos;
  legacy->targets[2].player = mode;
}

int can_activate_to_play_cards_from_graveyard(int player, int card, event_t event, type_t types){

	// Activation
	if (event == EVENT_CAN_ACTIVATE){
		const int* grave = get_grave(player);
		int i, rval, result = 0;
		for (i = 0; grave[i] != -1; ++i){
			if ((cards_data[grave[i]].type & types) &&	// deliberately not is_what()
				has_mana_to_cast_iid(player, event, grave[i]) &&
				(rval = can_legally_play_iid_now(player, grave[i], event))
			   ){
				if (rval == 99){
					return 99;
				}
				result = rval;
			}
		}
		return result;
	}

	if (event == EVENT_ACTIVATE){
		const int* grave = get_grave(player);
		int iids[500], positions[500];
		int i, num_cards = 0;
		for (i = 0; grave[i] != -1; ++i){
			if ((cards_data[grave[i]].type & types) &&	// deliberately not is_what()
				has_mana_to_cast_iid(player, event, grave[i]) &&
				can_legally_play_iid_now(player, grave[i], event)
			   ){
				iids[num_cards] = grave[i];
				positions[num_cards] = i;
				++num_cards;
			}
		}

		test_definition_t test;
		default_test_definition(&test, 0);
		int selected = select_card_from_zone(player, player, iids, num_cards, 0, AI_MAX_VALUE, -1, &test);
		if (selected != -1 && charge_mana_from_id(player, -1, event, cards_data[iids[selected]].id)){
			play_card_in_grave_for_free(player, player, positions[selected]);
			cant_be_responded_to = 1;	// Response allowed above, just none from this activation
		} else {
			cancel = 1;
		}
	}

	return 0;
}

int real_put_into_play(int player, int card)
{
  // 0x4b5d90

  if (player < 0 || card < 0
	  || player > 1 || card >= 150)
	return 0;

  card_instance_t* inst = get_card_instance(player, card);
  int csvid = cards_data[inst->internal_card_id].id;
  if ((csvid == CARD_ID_VESUVAN_DOPPELGANGER || csvid == 39 /* original clone */)
	  && !dispatch_event_to_single_card(player, card, EVENT_CAN_CAST, 1 - player, -1))
	{
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  // Use raw type here, not interpreted through get_type() or is_what(), since the other things that maintain these counts also use raw type
  type_t typ = cards_data[inst->internal_card_id].type;
  if (typ & TYPE_CREATURE)
	++creature_count[player];
  if (typ & TYPE_ARTIFACT)
	++artifact_count[player];
  if (typ & TYPE_ENCHANTMENT)
	++enchantment_count[player];

  types_of_cards_on_bf[player] |= typ;
  redraw_libraries();
  inst->state |= STATE_SUMMON_SICK|STATE_INVISIBLE|STATE_IN_PLAY;
  dispatch_event(player, card, EVENT_CAST_SPELL);
  inst->state |= STATE_JUST_CAST;
  if (player == 1)
	inst->state |= STATE_POWER_STRUGGLE;
  dispatch_event_to_single_card(player, card, EVENT_RESOLVE_SPELL, 1 - player, -1);
  inst->state &= ~STATE_INVISIBLE;

  // The point - the executable's version just set trigger_cause_controller and trigger_cause without saving their values
  inst->regen_status |= KEYWORD_RECALC_ALL;
  dispatch_trigger2(current_turn, TRIGGER_COMES_INTO_PLAY, EXE_STR(0x787108)/*PROMPT_SPECIALFEPHASE[0] == "Card into play"*/, 0, player, card);

  if (typ & TYPE_LAND)	// This seems dubious, but whatever
	++lands_played;

  return 0;
}

void play_card_in_opponent_hand_by_test(int player, int card, test_definition_t *this_test){
	int playable[2][hand_count[1-player]];
	int pc = 0;
	int i;
	for(i=0; i<active_cards_count[1-player]; i++){
		if( in_hand(1-player, i) ){
			//Basic check for playability
			if( is_what(1-player, i, TYPE_CREATURE | TYPE_ARTIFACT) || call_card_function(1-player, i, EVENT_CAN_CAST) ){
				if( new_make_test_in_play(1-player, i, -1, this_test) ){
					playable[0][pc] = get_original_internal_card_id(1-player, i);
					playable[1][pc] = i;
					pc++;
				}
			}
		}
	}

	int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_CMC, -1, this_test);
	if( selected != -1 ){
		opponent_plays_card_in_your_hand_for_free(1-player, playable[1][selected]);
	}
}

int card_special_effect(int player, int card, event_t event ){

	int flags = get_card_instance(player, card)->targets[0].card;

	if( flags > -1 ){
		if( flags & SE_EPIC ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->targets[1].card > -1 && current_turn == instance->targets[0].player && upkeep_trigger(player, card, event) ){
				real_copy_spell(player, instance->targets[1].card, SF_NOT_CAST | SF_PLAYED_FOR_FREE, SF3_EPIC, NULL);
			}
		}
		if( event == EVENT_MODIFY_COST_GLOBAL ){
			if( (flags & SE_CSVID_CANT_BE_PLAYED) || (flags & SE_TYPE_CANT_BE_PLAYED) ){
				card_instance_t *instance = get_card_instance(player, card);
				if( affected_card_controller == instance->targets[0].player || instance->targets[0].player == ANYBODY ){
					if( flags & SE_CSVID_CANT_BE_PLAYED ){
						if( get_id(affected_card_controller, affected_card) == instance->targets[1].player ){
							infinite_casting_cost();
						}
					}
					if( (flags & SE_TYPE_CANT_BE_PLAYED) && instance->targets[1].player > -1 ){
						if( is_what(affected_card_controller, affected_card, instance->targets[1].player) ){
							infinite_casting_cost();
						}
					}
				}
			}
		}
		if( flags & SE_CREATURES_CANT_ATTACK ){
			card_instance_t *instance = get_card_instance(player, card);
			nobody_can_attack(player, card, event, instance->targets[0].player);
		}
		if( flags & SE_END_AT_EOT ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->targets[1].card == CARD_ID_ABEYANCE ){
				if( eot_trigger(player, card, event) ){
					test_definition_t this_test;
					default_test_definition(&this_test, TYPE_PERMANENT);
					new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_ENABLE_NONMANA_ACTIVATED_ABILITIES);
					kill_card(player, card, KILL_REMOVE);
				}
			}
			else{
				if( event == EVENT_CLEANUP ){
					if( instance->targets[2].player ){
						instance->targets[2].player--;
					}
					else{
						kill_card(player, card, KILL_REMOVE);
					}
				}
			}
		}
	}

	return 0;
}

int target_player_cant_cast_csvid(int player, int card, int t_player, int csvid){
	int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);
	card_instance_t *instance = get_card_instance(player, card_added);
	instance->targets[0].player = t_player;
	instance->targets[0].card = SE_CSVID_CANT_BE_PLAYED | SE_END_AT_EOT;
	instance->targets[1].player = csvid;
	instance->targets[1].card = get_id(player, card);
	put_into_play(player, card_added);
	create_card_name_legacy(player, card_added, get_id(player, card));
	return card_added;
}

int target_player_cant_cast_type(int player, int card, int t_player, int type){
	int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);
	card_instance_t *instance = get_card_instance(player, card_added);
	instance->targets[0].player = t_player;
	instance->targets[0].card = SE_TYPE_CANT_BE_PLAYED | SE_END_AT_EOT;
	instance->targets[1].player = type;
	instance->targets[1].card = get_id(player, card);
	put_into_play(player, card_added);
	create_card_name_legacy(player, card_added, get_id(player, card));
	return card_added;
}

int target_player_cant_attack(int player, int card, int t_player){
	int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);
	card_instance_t *instance = get_card_instance(player, card_added);
	instance->targets[0].player = t_player;
	instance->targets[0].card = SE_CREATURES_CANT_ATTACK | SE_END_AT_EOT;
	instance->targets[1].card = get_id(player, card);
	put_into_play(player, card_added);
	create_card_name_legacy(player, card_added, get_id(player, card));
	return card_added;
}

void add_flag_to_special_effect(int player, int card, int flag){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].card < 0 ){
		instance->targets[0].card = 0;
	}
	instance->targets[0].card |= flag;
}

int check_for_special_effect_card(int player, int csvid){
	int i;
	for(i=0; i<active_cards_count[player]; i++){
		if( in_play(player, i) && get_id(player, i) == CARD_ID_SPECIAL_EFFECT ){
			if( get_card_instance(player, i)->targets[1].card == csvid ){
				return i;
			}
		}
	}
	return -1;
}

