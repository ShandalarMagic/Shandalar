#include "manalink.h"

int get_usertime_of_current_thread_in_ms(void);	// exe
extern int start_usertime_of_current_thread_in_ms;

int check_timer_for_ai_speculation(void)
{
	int decision_time = get_setting(SETTING_AI_DECISION_TIME);
	if (decision_time <= 0){
		decision_time = 5405;
	}
	return (100 * (get_usertime_of_current_thread_in_ms() - start_usertime_of_current_thread_in_ms)) / decision_time;
}

int pay_mana_maximally_satisfied(int* pay_mana_array, int x_val, int max_x_val);
int try_to_pay_for_mana_by_autotapping(int player, int* amt, int* unknown_v46, int autotap_flags, int unknown_v47);

void human_autotap_for_mana(int player, int* amt, int* unknown_v46, int unknown_v47){
	// First is standard autotapping, straight out of the exe.

	// 1. Try to pay by tapping relevant basic lands.
	if (!pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)){
		try_to_pay_for_mana_by_autotapping(player, amt, unknown_v46,
										   AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP|AUTOTAP_NO_NONBASIC_LANDS,
										   unknown_v47);
	}
	// 2. Try to pay by tapping all lands.
	if (!pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)){
		try_to_pay_for_mana_by_autotapping(player, amt, unknown_v46,
										   AUTOTAP_NO_CREATURES|AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP,
										   unknown_v47);
	}

	// Everything above here is standard.  New stuff below.

	/* This function is not only called for the human player when you left-double-click to pay a cost, but also for the ai to pay its own costs and when it's
	 * analyzing what the human could play.  The combinations of AUTOTAP_ flags below are all called afterwards (except when it's a human left-double-clicking),
	 * as well as all combinations with AUTOTAP_NO_DONT_AUTOTAP not set; so in those cases, don't call them from here. */
	if ((player != 1 || (trace_mode & 2)) && ai_is_speculating != 1){
		if (get_setting(SETTING_AUTOTAP_ARTIFACTS)){
			// 3a. Try to pay by tapping all lands and artifacts
			if (!pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)){
				try_to_pay_for_mana_by_autotapping(player, amt, unknown_v46,
												   AUTOTAP_NO_CREATURES|AUTOTAP_NO_DONT_AUTO_TAP,
												   unknown_v47);
			}

			if (get_setting(SETTING_AUTOTAP_CREATURES)){
				// 4a. Try to pay by tapping all lands, artifacts, and creatures
				if (!pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)){
					try_to_pay_for_mana_by_autotapping(player, amt, unknown_v46,
													   AUTOTAP_NO_DONT_AUTO_TAP,
													   unknown_v47);
				}
			}
		} else if (get_setting(SETTING_AUTOTAP_CREATURES)){
			// 3b. Try to pay by tapping all lands and creatures
			if (!pay_mana_maximally_satisfied(&PAY_MANA_COLORLESS, x_value, max_x_value)){
				try_to_pay_for_mana_by_autotapping(player, amt, unknown_v46,
												   AUTOTAP_NO_ARTIFACTS|AUTOTAP_NO_DONT_AUTO_TAP,
												   unknown_v47);
			}
		}
	}
}

/* Copies the following data (all int or arrays of int unless noted):
0x4ef170			must_attack
0x4ef174			TENTATIVE_types_of_cards_in_graveyard[2]
0x4ef17c			dword_4EF17C[2]
0x4ef184			current_phase
0x4ef188			event_flags
0x4ef18c			time_walk_flag
0x4ef190			land_can_be_palyed
0x4ef194			cancel
0x4ef198			max_x_value
0x4ef19c			card_on_stack_iid
0x4ef1a0			x_value
0x4ef1a4			player_bits[]
0x4ef1ac			dword_4EF1AC
0x4ef1b0			dword_4EF1B0
0x4ef1b4			dword_4EF1B4
0x4ef1b8			dword_4EF1B8
0x4ef1bc			dword_4EF1BC[2]
0x4ef1c4			hand_count[2]
0x4ef1cc			TENTATIVE_creature_count[2]
0x4ef1d4			TENTATIVE_artifact_count[2]
0x4ef1dc			TENTATIVE_enchantment_count[]
0x4ef1e4			(unreferenced)
0x4ef1e8			(unreferenced)
0x4ef1ec			damage_taken_this_turn[2]
0x4ef1f4			cards_drawn_this_turn[2]	// array of 2 uint16_t
0x4ef1f8			creatures_dead_this_turn
0x4ef1fc			lands_played
0x4ef200..0x4ef20d	char mana_doesnt_drain_from_pool[2][7]	// array of 14 uint8_t - cleared at the start of each turn in sub_435D20
0x4ef20e			number_of_attackers_declared			// int16_t - cleared at the start of each turn in sub_435D20
0x4ef210..0x4ef21b	(unreferenced, except to clear it at the start of each turn in sub_435D20)
0x4ef21c			raw_mana_available_hex[2][51]
0x4ef3b4..0x4ef3bf	(unreferenced)
0x4ef3c0			raw_mana_available[2][8]
0x4ef400			pay_mana_xbugrwaU[8]
0x4ef420..0x4ef45f	(unreferenced)
0x4ef460			basiclandtypes_controlled[2][8]
0x4ef4a0			landsofcolor_controlled[2][8]
0x4ef4e0			mana_pool[2][8]
0x4ef520			active_cards_count[2]
0x4ef528			life[2]
0x4ef530			poison_counters[2]
0x4ef538			liched[2]
0x4ef540			card_instances[2][151]		// array of 302 card_instance_t
0x505728			ante_cards[2][16]
0x5057a8			stack_size
0x5057ac			(unreferenced)
0x5057b0			graveyard[2][500]
0x506750			library[2][500]
0x5076f0			TENTATIVE_timestamp_card[500]
0x507ec0			TENTATIVE_timestamp_player[500]
0x508690			exile[2][500]
0x509630			stack_data[32]
0x5096b0			stack_cards[32]				// array of 32 target_t
0x5097b0			stack_damage_targets[32]	// array of 32 target_t */
// All that needs to be done to backup, save, and load additional data is to add it to the following macro.  The copy is shallow, and assumes sizeof(data) is correct.
extern int mulligans_complete[2];
extern int challenge_round, challenge1, challenge2, challenge3, challenge4;
#define BACKUP_AND_SAVE_LIST				\
  BACKUP_ARRAY(graveyard_source);			\
  BACKUP_SCALAR(next_graveyard_source_id);	\
  BACKUP_SCALAR(turn_count);				\
  BACKUP_SCALAR(always_prompt_for_color);	\
  BACKUP_ARRAY(mulligans_complete);			\
  BACKUP_SCALAR(challenge_round);			\
  BACKUP_SCALAR(challenge1);				\
  BACKUP_SCALAR(challenge2);				\
  BACKUP_SCALAR(challenge3);				\
  BACKUP_SCALAR(challenge4);				\
  BACKUP_SCALAR(counters_added);			\
  /*end*/

#define BACKUP_ARRAY(data)	BACKUP_CMD_A(data)
#define BACKUP_SCALAR(data)	BACKUP_CMD_S(data)

#define BACKUP_CMD_A(data)	static char backup_##data[sizeof(data)]
#define BACKUP_CMD_S(data)	static __typeof__(data) backup_##data
BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
void backup_data_for_ai(void)
{
  // 0x498e30
  memcpy((void*)0x50be70, &must_attack, 0x1a740);
  STATIC_ASSERT(sizeof(card_data_t) == 72, sizeof_card_data_t_must_be_72);
  // This target immediately follows 0x50be70.
  memcpy((void*)0x5265b0, &cards_data[EXE_DWORD(0x7a5380)], 16 * sizeof(card_data_t));
  EXE_DWORD(0x5b9b3c) = ai_modifier;

#define BACKUP_CMD_A(data)	memcpy(backup_##data, data, sizeof(data))
#define BACKUP_CMD_S(data)	backup_##data = data
  BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
}

void restore_data_for_ai(void)
{
  // 0x498e70
  memcpy(&must_attack, (void*)0x50be70, 0x1a740);
  STATIC_ASSERT(sizeof(card_data_t) == 72, sizeof_card_data_t_must_be_72);
  memcpy(&cards_data[EXE_DWORD(0x7a5380)], (void*)0x5265b0, 16 * sizeof(card_data_t));
  ai_modifier = EXE_DWORD(0x5b9b3c);

#define BACKUP_CMD_A(data)	memcpy(data, backup_##data, sizeof(data))
#define BACKUP_CMD_S(data)	data = backup_##data
  BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
}

/* Copies the same data as backup_data_for_ai()/restore_data_for_ai(), as well as the following four arrays.  They seem to keep track of damage dealt for
 * Reverse Damage, Reverse Polarity, and Simulacrum.
0x5098b0	dword_5098B0[]
0x5098b4	dword_5098B4[]
0x50ab70	dword_50AB70[]
0x50be50	dword_50BE50[] */
#define BACKUP_CMD_A(data)	static char backup0_##data[sizeof(data)]
#define BACKUP_CMD_S(data)	static __typeof__(data) backup0_##data
BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
void backup_data_for_ai_0(void)
{
  // 0x498d90
  // This target immediately follows 0x5265b0 in backup_data_for_ai()/restore_data_for_ai().
  memcpy((void*)0x526a30, &must_attack, 0x1cd00);
  EXE_DWORD(0x5bbb44) = 0;
  STATIC_ASSERT(sizeof(card_data_t) == 72, sizeof_card_data_t_must_be_72);
  // This target immediately follows 0x526a30.
  memcpy((void*)0x543730, &cards_data[EXE_DWORD(0x7a5380)], 16 * sizeof(card_data_t));
  // Immediately followed by displayed_mana_pool[][].
  ASSERT(stack_size >= 0);

#define BACKUP_CMD_A(data)	memcpy(backup0_##data, data, sizeof(data))
#define BACKUP_CMD_S(data)	backup0_##data = data
  BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
}

void restore_data_for_ai_0(void)
{
  // 0x498df0
  memcpy(&must_attack, (void*)0x526a30, 0x1cd00);
  STATIC_ASSERT(sizeof(card_data_t) == 72, sizeof_card_data_t_must_be_72);
  memcpy(&cards_data[EXE_DWORD(0x7a5380)], (void*)0x543730, 16 * sizeof(card_data_t));
#define BACKUP_CMD_A(data)	memcpy(data, backup0_##data, sizeof(data))
#define BACKUP_CMD_S(data)	data = backup0_##data
  BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S
}

void copy_to_display_supplement(void)
{
  /* copy_to_display() itself is complex, since it copies out of order, and also translates library[][], graveyard[][], and exile[][] from internal_card_ids to
   * csvids; so I injected a call to this into the function rather than moving the whole thing into C. */
  // (On the other hand, we clearly haven't needed it yet.)
}

// Not, strictly speaking, the correct place for this, but it lets us use the same list to both backup data for ai speculation and to save it in savefiles.
int save_or_load_supplement(int status)
{
  /* Called at the end of save_or_load_ver1() and save_or_load_ver2().  That's not (quite) at the end of the savefiles; there's additional data saved after each
   * of those, varying by game_type. */

  // Example usage:
  // status &= save_or_load_data(some_hunk_of_data, sizeof(some_hunk_of_data));

#define BACKUP_CMD_A(data)	status &= save_or_load_data( data, sizeof(data))
#define BACKUP_CMD_S(data)	status &= save_or_load_data(&data, sizeof(data))
  BACKUP_AND_SAVE_LIST
#undef BACKUP_CMD_A
#undef BACKUP_CMD_S

  if (EXE_DWORD(0x5bc5f8))	// loading_or_saving: is nonzero if loading a game
	--turn_count;	// since it gets incremented after loading

  return status;
}
#undef BACKUP
#undef BACKUP_AND_SAVE_LIST

static int check_destroys_if_blocked_oneside(int lethal_player, int lethal_card, int other_player, int other_card)
{
  card_instance_t* instance = get_card_instance(lethal_player, lethal_card);
  int lethal = instance->destroys_if_blocked;
  int result = 0;
  if (lethal)
	{
	  if ((lethal & DIFB_DESTROYS_ALL) == DIFB_DESTROYS_ALL)	// multiple bits
		result = 1;
	  else if ((lethal & DIFB_DESTROYS_WALLS) && has_subtype(other_player, other_card, SUBTYPE_WALL))
		result = 1;
	  else if ((lethal & DIFB_DESTROYS_NONWALLS) && !has_subtype(other_player, other_card, SUBTYPE_WALL))
		result = 1;

	  if (result != 1
		  && (lethal & DIFB_DESTROYS_UNPROTECTED) && !is_protected_from(other_player, other_card, lethal_player, lethal_card, IPF_DAMAGE))
		result = 1;

	  if (lethal & (DIFB_ASK_CARD | DIFB_ASK_ALL_CARDS))
		{
		  if (lethal & DIFB_ASK_ALL_CARDS)
			result |= dispatch_event_with_attacker(lethal_player, lethal_card, EVENT_CHECK_DESTROY_IF_BLOCKED, other_player, other_card);
		  else
			result |= dispatch_event_with_attacker_to_one_card(lethal_player, lethal_card, EVENT_CHECK_DESTROY_IF_BLOCKED, other_player, other_card);
		}
	}
  return result;
}

/* Sets bit 1 of return value if (attack_player,attack_card) will auto-destroy (defend_player,defend_card) if the latter blocks the former, and bit 2 if
 * (attack_player,attack_card) will be auto-destroyed by (defend_player,defend_card) if the latter blocks the former.
 *
 * Formerly checked for intersection between each card's color and the other's enemy_against_color, with an attempt to deal with walls.  Almost no cards used
 * the functionality, and not even all of those did so correctly - it worked properly for Thicket Basilisk/Cockatrice, Abu Ja'far, Abomination, and Venom;
 * partially for Deathgazer; and not at all for Battering Ram. */
int check_destroys_if_blocked(int attack_player, int attack_card, int defend_player, int defend_card)
{
  int result1 = check_destroys_if_blocked_oneside(attack_player, attack_card, defend_player, defend_card);

  int result2 = check_destroys_if_blocked_oneside(defend_player, defend_card, attack_player, attack_card);
  // swap bits
  result2 = ((result2 & 1) << 1) | ((result2 & 2) >> 1);

  return result1 | result2;
}

/* What this seems to do is, during speculation, store value for later retrieval and returns it; and during actual execution, retrieve the value that was stored
 * during the best path.  Not well understood. */
int remember_ai_value(int player, int value)
{
  EXE_DWORD(0x7A2FE4) = 0;

  if (player == AI && !(trace_mode & 2))
	{
	  if (ai_is_speculating == 1)
		{
		  /* There's special-casing when 99 is retrieved in sub_499050(); it appears that path is always taken during further speculation.  Until this is better
		   * understood, avoid that number entirely, encoding on storage and retrieval. */
		  if (value >= 99)
			++value;
		  EXE_DWORD(0x7A2FE4) = value;
		  EXE_FN(void, 0x498F20, void)();
		}
	  else
		EXE_FN(int, 0x499050, void)();
	}
  else
	return value;

  value = EXE_DWORD(0x7A2FE4);
  if (value >= 99)
	--value;
  return value;
}

// Like internal_rand(), but if player is the AI, stores and retrieves the choice per remember_ai_value()
int recorded_rand(int player, int upperbound)
{
  // 0x401C60

  return remember_ai_value(player, internal_rand(upperbound));
}

void human_assign_blockers(int player);
void ai_assign_blockers(int player)
{
  // 0x4b18e0

  EXE_FN(void, 0x4AF6F0, int)(player);	//TENTATIVE_ai_speculate_on_combat(player);

  EXE_DWORD(0x60827C) = -9999;

  int i;
  for (i = 0; i < 16; ++i)
	EXE_DWORD_PTR(0x6076D8)[i] = 0;

  EXE_FN(int, 0x4B1A50, int, int)(player, 0);

  if (!(ai_is_speculating == 1 || current_turn == HUMAN))
	return;

  // Begin additions
  if (event_flags & EF_ATTACKER_CHOOSES_BLOCKERS)
	{
	  if (!(ai_is_speculating == 1 || current_turn == AI))	// if speculating or AI is attacking, nothing blocks
		human_assign_blockers(player);
	  return;
	}
  // End additions

  for (i = 0; i < EXE_DWORD(0x607D54); ++i)
	{
      forbid_attack = 0;
	  if (event_flags & EA_PAID_BLOCK)
		{
		  EXE_FN(void, 0x435C80, void)();	//push_affected_card_stack()
		  trigger_cause_controller = EXE_DWORD(0x607C2C);	//TENTATIVE_ai_speculation_opponent
		  trigger_cause = EXE_DWORD_PTR(0x608304)[i];		//WILDGUESS_ai_combat_block_card[i]
		  dispatch_trigger(1 - current_turn, TRIGGER_PAY_TO_BLOCK, EXE_STR(0x790248)/*PROMPT_TURNSEQUENCE[0]*/, 1);
		  EXE_FN(void, 0x435CD0, void)();	//pop_affected_card_stack()
		  if (forbid_attack)
			{
			  forbid_attack = 0;
			  continue;
			}
		}

	  uint8_t blk = BYTE0(EXE_DWORD_PTR(0x60775C)[i]);

	  card_instance_t* instance = get_card_instance(EXE_DWORD(0x607C2C),	// TENTATIVE_ai_speculation_opponent
													EXE_DWORD_PTR(0x608304)[i]);	// WILDGUESS_ai_combat_block_card[i]
	  // Begin additions
	  if (instance->blocking != 255)	// Already during one of the two triggers dispatched from here
		continue;
	  // End additions
	  instance->blocking = blk;

	  if (blk != 255)	// Originally !(blk & 0x80) - this failed when blocking ids 128-149
		instance->state |= STATE_BLOCKING;

	  if (ai_is_speculating != 1)
		{
		  /* All this does is SendMessageA(hwnd_MainClass, 0x464u, 0xFFu, 0).  It's always called through a wrapper that checks ai_is_speculating != 1
		   * (except from C where it's already been checked).  wndproc_MainClass responds by calling TENTATIVE_redisplay_all(0, 0xff) at 0x437ec0. */
		  EXE_FN(void, 0x437E20, void)();

		  if (blk != 255)
			play_sound_effect(WAV_BLOCK2);
		}

	  /* This immediately followed the EA_PAID_BLOCK/TRIGGER_PAY_TO_BLOCK.  Putting it down here matches the order when a human blocks; in particular,
	   * instance->blocking is correct during TRIGGER_BLOCKER_CHOSEN, which is necessary for Tromokratis. */
	  if ((event_flags & EA_SELECT_BLOCK) && blk != 255)
		dispatch_trigger2(current_turn, TRIGGER_BLOCKER_CHOSEN, EXE_STR(0x790074), 0,//PROMPT_BLOCKERSELECTION[0]
						  EXE_DWORD(0x607C2C),	//TENTATIVE_ai_speculation_opponent
						  EXE_DWORD_PTR(0x608304)[i]);		//WILDGUESS_ai_combat_block_card[i]
	}
}

extern int raw_mana_available[2][8];
void dispatch_event_raw(event_t event);
void mana_burn(void);
int hack_ai_decision_phase_upkeep = 0;
int ai_decision_phase(int player, int *phase_code_to_go_to, int *becomes_second_arg_of_main_phase_and_discard_phase, int *becomes_third_arg_of_main_phase)
{
  // 0x43d590

  // Begin additions
  int upkeep_speculation = hack_ai_decision_phase_upkeep;
  hack_ai_decision_phase_upkeep = 0;
  // End additions

  int store_raw_mana_available[2][8];

  if (trace_mode & 2)
	{
	  char str[100];
	  sprintf(str, "%d: Entering AI Decision Phase.\n", EXE_DWORD(0x60EC40)++);
	  EXE_FN(void, 0x4a7d80, const char*)(str);	// append_to_trace_txt(str)
	}

  *phase_code_to_go_to = -1;

  if (ai_is_speculating == 1)
	{
	  int p = 1 - player;
	  int col;
	  for (col = 0; col <= 7; ++col)
		{
		  store_raw_mana_available[p][col] = raw_mana_available[p][col];
		  raw_mana_available[p][col] = landsofcolor_controlled[p][col];
		}

	  dispatch_event_raw(EVENT_SHOULD_AI_PLAY);

	  card_instance_t* inst;
	  int c;
	  for (c = 0; c < active_cards_count[AI]; ++c)
		if ((inst = get_card_instance(AI, c))->internal_card_id != -1)
		  call_card_function_i(inst, AI, c, EVENT_CAN_COUNTER);

	  EXE_FN(void, 0x477070, void)();	// resolve_damage_cards_and_prevent_damage();
	  EXE_FN(void, 0x477a90, void)();	// regenerate_or_graveyard_triggers();

	  for (p = 0; p <= 1; ++p)
		for (col = 0; col <= 7; ++col)
		  raw_mana_available[p][col] = store_raw_mana_available[p][col];	// never initialized for p == player

	  recalculate_all_cards_in_play();

	  int ai_opinion = EXE_FN(int, 0x499160, int)(AI)/*ai_opinion_of_gamestate(AI)*/ + ai_modifier;

	  if (life[AI] > 0)
		EXE_DWORD(0x60E9FC) |= 4u;

	  int v16 = 0;	// Left uninitialized in exe version
	  if (EXE_DWORD(0x715B30) < ai_opinion && !EXE_DWORD(0x7A372C))
		{
		  EXE_DWORD(0x715B30) = ai_opinion;
		  EXE_FN(void, 0x4990B0, void)();
		  v16 = EXE_DWORD(0x60E9FC);
		  //v10 = EXE_DWORD(0x738B48);	not referenced again
		}

	  if (EXE_DWORD(0x7282F4) == 999)
		EXE_DWORD(0x7282F4) = -1;

	  EXE_DWORD(0x7282EC) = 0;
	  *becomes_third_arg_of_main_phase = 0;
	  EXE_DWORD(0x7A372C) = 0;

	  if (check_timer_for_ai_speculation() > EXE_DWORD(0x786DCC) / 2
		  && (5 * (5 * opponent_skill + 5) <= EXE_DWORD(0x738B48)
			  || ((EXE_DWORD(0x60E9FC) & 4) < 1 ? 200 : 50) < check_timer_for_ai_speculation()))
		{
		  ai_is_speculating = 0;
		  EXE_DWORD(0x7282F4) = -1;
		  EXE_DWORD(0x60E9FC) = v16;
		}

	  ++EXE_DWORD(0x738B48);

	  if (EXE_DWORD(0x60EC3C) == 1)
		{
		  if (land_can_be_played & TENTATIVE_LCBP_DURING_SECOND_MAIN_PHASE)
			current_phase = PHASE_MAIN2;
		  else
			current_phase = PHASE_MAIN1;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 1;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 2)
		{
		  current_phase = PHASE_NORMAL_COMBAT_DAMAGE;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 2;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 3)
		{
		  *becomes_second_arg_of_main_phase_and_discard_phase = 1;
		  *phase_code_to_go_to = 7;	// discard
		}

	  if (EXE_DWORD(0x60EC3C) == 4)
		{
		  if (land_can_be_played & TENTATIVE_LCBP_DURING_COMBAT)
			current_phase = PHASE_MAIN2;
		  else
			current_phase = PHASE_MAIN1;
		  *becomes_third_arg_of_main_phase = 0;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 4;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 5)
		{
		  current_phase = PHASE_NORMAL_COMBAT_DAMAGE;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 5;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 6)
		{
		  current_phase = PHASE_NORMAL_COMBAT_DAMAGE;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 6;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 7)
		{
		  if (land_can_be_played & TENTATIVE_LCBP_DURING_COMBAT)
			current_phase = PHASE_MAIN2;
		  else
			current_phase = PHASE_MAIN1;
		  *becomes_third_arg_of_main_phase = 0;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 7;
		  *phase_code_to_go_to = 6;
		}

	  if (EXE_DWORD(0x60EC3C) == 8)
		{
		  current_phase = PHASE_NORMAL_COMBAT_DAMAGE;
		  *becomes_second_arg_of_main_phase_and_discard_phase = 8;
		  *phase_code_to_go_to = 6;
		}

	  // Begin additions
	  if (EXE_DWORD(0x60EC3C) == 17)
		*phase_code_to_go_to = 4;	// upkeep
	  // End additions
	}

  // Begin additions
  if (upkeep_speculation)
	return 0;
  // End additions

  if (*phase_code_to_go_to == -1)
	{
	  current_phase = PHASE_CLEANUP2;
	  // v14 = 0;	// not referenced again
	  EXE_FN(int, 0x439C90, int)(PHASE_CLEANUP2);	// TENTATIVE_check_for_stops(PHASE_CLEANUP2);
	  mana_burn();

	  if (EXE_FN(int, 0x43A1B0, void)())	// is_anyone_dead()
		return 1;

	  if (current_turn == EXE_DWORD(0x4EF1B0)
		  && EXE_DWORD(0x4EF1AC) == PHASE_CLEANUP2)
		{
		  EXE_DWORD(0x4EF1B0) = -1;
		  EXE_DWORD(0x4EF1AC) = -1;
		}

	  EXE_FN(int, 0x439A80, void)();
	  play_sound_effect(WAV_ENDTURN);
	}

  return 0;
}
