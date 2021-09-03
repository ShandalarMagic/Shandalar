#include "manalink.h"

// Returns nonzero if there's a non-humiliated Angel of Jubilation in play.  (check_battlefield_for_special_card returns true for humiliated ones.)
static int angel_of_jubilation_on_bf(void)
{
  card_instance_t* inst;
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((inst = in_play(p, c)) && cards_data[inst->internal_card_id].id == CARD_ID_ANGEL_OF_JUBILATION && !is_humiliated(p, c))
		return 1;
  return 0;
}

int can_sacrifice_this_as_cost(int player, int card){
	if( ! check_special_flags2(player, card, SF2_CANNOT_BE_SACRIFICED) ){
		if (is_what(player, card, TYPE_CREATURE) ){
			return !angel_of_jubilation_on_bf();
		}
		else{
			return 1;
		}
	}
	return 0;
}

static int new_can_sacrifice_as_cost_impl(int player, int card, int amount, test_definition_t* test)
{
  int c = 0, found = 0, allow_creatures = 1;
  if (angel_of_jubilation_on_bf())
	{
	  // A very common case
	  if (test->type == TYPE_CREATURE && test->type_flag == MATCH)
		return 0;

	  /* Check here, rather than attempting to alter test, which might not have anything to do with creatures at all: Angel of Jubilation should still prevent
	   * you from picking an artifact creature for "Sacrifice an artifact". */
	  allow_creatures = 0;
	}

  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c)
		&& is_what(player, c, TYPE_PERMANENT)
		&& !(c == card && test->not_me == 1)
		&& new_make_test_in_play(player, c, -1, test)
		&& (allow_creatures || !is_what(player, c, TYPE_CREATURE))
		&& ! check_special_flags2(player, c, SF2_CANNOT_BE_SACRIFICED)
		&& ++found >= amount && amount >= 0)
	  return 1;

  if (amount < 0)
	return found;

  return 0;
}

// Verify that "player" could sacrifice amount permanents matching test for paying a cost.
int new_can_sacrifice_as_cost(int player, int card, test_definition_t* test)
{
  return new_can_sacrifice_as_cost_impl(player, card, test->qty, test);
}

int max_can_sacrifice_as_cost(int player, int card, test_definition_t* test)
{
  return new_can_sacrifice_as_cost_impl(player, card, -1, test);
}

int can_sacrifice_as_cost(int player, int amount, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t test;
	new_default_test_definition(&test, type, "");
	test.type_flag = flag1;
	test.subtype = subtype;
	test.subtype_flag = flag2;
	test.color = color;
	test.color_flag = flag3;
	test.id = id;
	test.id_flag = flag4;
	test.cmc = cc;
	test.cmc_flag = flag5;
	test.qty = amount;
	return new_can_sacrifice_as_cost(player, -1, &test);
}

int can_sacrifice_type_as_cost(int player, int amount, type_t type)
{
  test_definition_t test;
  new_default_test_definition(&test, type, "");
  test.qty = amount;
  return new_can_sacrifice_as_cost(player, -1, &test);
}

int can_cause_sacrifice(int player, int t_player)
{
  int c;
  if (player != t_player)
	for (c = 0; c < active_cards_count[t_player]; ++c)
	  if (in_play(t_player, c))
		switch (get_id(t_player, c))
		  {
			case CARD_ID_TAJURU_PRESERVER:
			case CARD_ID_SIGARDA_HOST_OF_HERONS:
			  if (!is_humiliated(t_player, c))
				return 0;
			  break;
		  }
  return 1;
}

int new_can_sacrifice(int player, int card, int t_player, test_definition_t* test)
{
  int amount = test ? test->qty : 1;
  if (amount <= 0)
	return 1;

  if (!can_cause_sacrifice(player, t_player))
	return 0;

  int not_me = test && test->not_me == 1 && player == t_player ? card : -1;
  int c, count = 0;
  for (c = 0; c < active_cards_count[t_player]; ++c)
	if (in_play(t_player, c)
		&& c != not_me
		&& (test ? new_make_test_in_play(t_player, c, -1, test) : is_what(t_player, c, TYPE_PERMANENT))
		&& ! check_special_flags2(t_player, c, SF2_CANNOT_BE_SACRIFICED)
		&& ++count >= amount)
	  return 1;

  return 0;
}

// Verify that a spell or an ability from "player" could make "t_player" sacrifice a specific type of permanent
int can_sacrifice(int player, int t_player, int amount, type_t type, subtype_t subtype)
{
  test_definition_t test;
  new_default_test_definition(&test, type, "");
  test.subtype = subtype;
  test.qty = amount;

  return new_can_sacrifice(player, -1, t_player, &test);
}

int is_nice_creature_to_sacrifice(int player, int card){

	card_data_t* card_d = get_card_data(player, card);

	if( card_d->cc[2] == 9 && ! is_what(player, card, TYPE_ENCHANTMENT) ){
		return 1;
	}

	if( card_d->cc[2] == 2 || card_d->cc[2] == 5 ){
		return 1;
	}

	if( check_for_special_ability(player, card, SP_KEYWORD_CANNON_FODDER) ){
		return 1;
	}
	return 0;
}

int get_best_permanent_for_sacrifice(int player, int type){
	int result = -1;
	int count = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && is_what(player, count, type) && ! check_special_flags2(player, count, SF2_CANNOT_BE_SACRIFICED) &&
				is_nice_creature_to_sacrifice(player, count) && get_card_instance(player, count)->kill_code < KILL_DESTROY
			  ){
				result = count;
				break;
			}
			count++;
	}
	return result;
}

int new_get_special_permanent_for_sacrifice(int player, int card, test_definition_t* test){

	int count = 0;
	int result = -1;
	while( count < active_cards_count[player] ){
		if( in_play(player, count) && new_make_test_in_play(player, count, -1, test) && is_nice_creature_to_sacrifice(player, count) &&
			! check_special_flags2(player, count, SF2_CANNOT_BE_SACRIFICED)
		  ){
			card_instance_t *instance = get_card_instance(player, count);
			if( !(instance->state & STATE_CANNOT_TARGET) && !(test->not_me == 1 && count == card) && instance->kill_code < KILL_DESTROY ){
				return count;
			}
		}
		count++;
	}
	count = 0;
	int par = 1000;
	while( count < active_cards_count[player] ){
		if( in_play(player, count) && new_make_test_in_play(player, count, -1, test) && ! check_special_flags2(player, count, SF2_CANNOT_BE_SACRIFICED) ){
			card_instance_t *instance = get_card_instance(player, count);
			if( !(instance->state & STATE_CANNOT_TARGET) && !(test->not_me == 1 && count == card) && instance->kill_code < KILL_DESTROY ){
				if( get_base_value(player, count) < par ){
					par = get_base_value(player, count);
					result = count;
				}
			}
		}
		count++;
	}
	return result;
}

static int get_special_permanent_for_sacrifice(int player, int type, int flag1, int subtype, int flag2,
											   int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t test;
	new_default_test_definition(&test, type, "");
	test.type_flag = flag1;
	test.subtype = subtype;
	test.subtype_flag = flag2;
	test.color = color;
	test.color_flag = flag3;
	test.id = id;
	test.id_flag = flag4;
	test.cmc = cc;
	test.cmc_flag = flag5;
	return new_get_special_permanent_for_sacrifice(player, -1, &test);
}

static void get_sacrifice_prompt_text(char* prompt, int maxlen, int type, int invert)
{
  if (ai_is_speculating == 1){
	*prompt = 0;
	return;
  }

  type &= TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT|TYPE_LAND|TARGET_TYPE_PLANESWALKER;
  char typetext[100];
  if (!invert)
	scnprintf(prompt, maxlen, "Select %s to sacrifice.", type_text(typetext, 100, type, 0));
  else
	scnprintf(prompt, maxlen, "Select %s permanent to sacrifice.", type_text(typetext, 100, type, TYPETEXT_INVERT));
}

static const char* can_sacrifice_this(int who_chooses, int player, int card)
{
	if( ! check_special_flags2(player, card, SF2_CANNOT_BE_SACRIFICED) ){
		return NULL;
	}
	return "can't sacririce this";
}

int pick_creature_for_sacrifice(int player, int card, int cannot_cancel){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.who_chooses = player;
	td.allow_cancel = cannot_cancel ? 0 : 2;
	td.extra = (int32_t)can_sacrifice_this;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	int result = -1;

	if( can_target(&td) ){
		if( player != AI ){
			if( new_pick_target(&td, "LORD_OF_THE_PIT", 0, 0) ){
				instance->number_of_targets = 1;
				result = instance->targets[0].card;
			}
		}
		else{
			result = get_best_permanent_for_sacrifice(player, TYPE_CREATURE);
			if( result == -1 ){
				if( new_pick_target(&td, "LORD_OF_THE_PIT", 0, 0) ){
					instance->number_of_targets = 1;
					result = instance->targets[0].card;
				}
			}
		}
	}

	return result;
}

int pick_permanent_for_sacrifice(int player, int card, int type, int cannot_cancel){

	target_definition_t td;
	default_target_definition(player, card, &td, type);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.who_chooses = player;
	td.allow_cancel = 1-cannot_cancel;
	td.illegal_state = TARGET_STATE_DYING;
	td.extra = (int32_t)can_sacrifice_this;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	int result = -1;

	if( can_target(&td) ){
		if( player != AI || (result = get_best_permanent_for_sacrifice(player, type)) == -1){
			char prompt[200];
			get_sacrifice_prompt_text(prompt, 200, type, 0);
			select_target(player, card, &td, prompt, &instance->targets[0]);

			if (cancel != 1){
				instance->number_of_targets = 1;
				result = instance->targets[0].card;
			}
		}
	}

	return result;
}

int pick_special_permanent_for_sacrifice(int player, int card, int cannot_cancel, int type, int flag1, int subtype,
										int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){

	target_definition_t td;
	default_target_definition(player, card, &td, type);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.who_chooses = player;
	td.allow_cancel = 1-cannot_cancel;
	td.illegal_state = TARGET_STATE_DYING;
	td.extra = (int32_t)can_sacrifice_this;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	int result = -1;

	if( can_target(&td) ){
		if( player != AI ){
			char prompt[200];
			get_sacrifice_prompt_text(prompt, 200, type, flag1 == DOESNT_MATCH);
			select_target(player, card, &td, prompt, &instance->targets[0]);

			if (cancel != 1 && make_test_in_play(instance->targets[0].player, instance->targets[0].card, -1, type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5)){
				instance->number_of_targets = 1;
				result = instance->targets[0].card;
			}
		}
		else{
			result = get_special_permanent_for_sacrifice(player, type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5);
		}
	}

	return result;
}

static test_definition_t* sacrifice_test;
static const char* passes_test_definition(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (new_make_test_in_play(player, card, -1, sacrifice_test) && ! check_special_flags2(player, card, SF2_CANNOT_BE_SACRIFICED) )
	return NULL;
  else
	return "Illegal sacrifice";
}
static int new_sacrifice_impl(int player, int card, int t_player, sacrifice_t options, test_definition_t* test, char* marked_for_sacrifice){
	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = t_player;
	td.preferred_controller = t_player;
	td.who_chooses = t_player;
	td.allow_cancel = (options & (SAC_NO_CANCEL | SAC_DONE)) ^ SAC_NO_CANCEL;	// exe expects 1 to show a cancel button, 2 to show a done button, 3 to show both.
	td.illegal_state = TARGET_STATE_DYING;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)passes_test_definition;

	if (test->not_me == 1){
		td.special |= TARGET_SPECIAL_NOT_ME;
	}

	int type = test->type;
	type &= (TYPE_CREATURE|TYPE_ARTIFACT|TYPE_ENCHANTMENT|TYPE_LAND|TARGET_TYPE_PLANESWALKER);
	if (type){
		if (test->type_flag == MATCH){
			td.required_type = type;
		} else if (test->type_flag == DOESNT_MATCH){
			td.illegal_type = type;
		}
	}

	if ((options & SAC_AS_COST) && angel_of_jubilation_on_bf()){
		td.illegal_type |= TYPE_CREATURE;
	}

	if ((options & SAC_CAUSED) && !can_cause_sacrifice(player, t_player)){
		cancel = 1;
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	int previous_cancel = cancel;

	sacrifice_test = test;
	if ((options & SAC_ALL_OR_NONE) ? (target_available(player, card, &td) < test->qty) : !can_target(&td)){
		cancel = 1;
		return 0;
	}

	char default_prompt[200];
	const char* prompt;
	if (test->message[0]){
		prompt = test->message;
	} else {
		get_sacrifice_prompt_text(default_prompt, 200, type, test->type_flag == DOESNT_MATCH);
		prompt = default_prompt;
	}

	int cannot_cancel = (options & (SAC_NO_CANCEL | SAC_DONE)) == SAC_NO_CANCEL;

	int old_number_of_targets = instance->number_of_targets;
	target_t old_target0 = instance->targets[0];	// struct copy

	char marked[151] = {0};

	int num_picked = 0;
	target_t last_picked = { -1, -1 };
	while (num_picked < test->qty){
		cancel = 0;

		target_t tgt = { -1, -1 };

		if (IS_AI(t_player)){
			int sac = new_get_special_permanent_for_sacrifice(t_player, player == t_player ? card : -1, test);
			if( sac != -1 ){
				tgt.player = t_player;
				tgt.card = sac;
			}
		}

		if (tgt.card == -1){
			instance->number_of_targets = 0;
			sacrifice_test = test;	select_target(player, card, &td, prompt, &tgt);
		}

		if (tgt.card < 0){
			if (cannot_cancel){
				continue;
			} else {
				cancel = tgt.card == -1 ? 1 : 0;
				break;
			}
		}

		if (marked[tgt.card]){
			get_card_instance(tgt.player, tgt.card)->state &= ~STATE_TARGETTED;
			--num_picked;
			marked[tgt.card] = 0;
		} else {
			get_card_instance(tgt.player, tgt.card)->state |= STATE_TARGETTED | (IS_AI(t_player) ? STATE_CANNOT_TARGET : 0);
			++num_picked;
			marked[tgt.card] = 1;
			last_picked = tgt;	// struct copy
		}
	}

	instance->number_of_targets = old_number_of_targets;
	instance->targets[0] = old_target0;	// struct copy

	if (cancel == 1 || num_picked == 0 || ((options & SAC_ALL_OR_NONE) && num_picked < test->qty)){
		cancel = 1;
		int c;
		for (c = 0; c < active_cards_count[t_player]; ++c){
			if (marked[c]){
				card_instance_t* inst = get_card_instance(t_player, c);
				inst->state &= ~(STATE_TARGETTED | STATE_CANNOT_TARGET);
			}
		}
		return 0;
	}

	cancel = previous_cancel;

	if (options & SAC_SET_TARGETS){
		// I suspect maybe one card uses each of these.
		instance->number_of_targets = 1;
		instance->targets[0]		= last_picked;	// struct copy
		instance->targets[1].player = get_color(last_picked.player, last_picked.card);
		instance->targets[1].card	= get_id(last_picked.player, last_picked.card);
		instance->targets[2].player	= get_power(last_picked.player, last_picked.card);
		instance->targets[2].card	= get_toughness(last_picked.player, last_picked.card);
		instance->targets[3].card	= get_card_instance(last_picked.player, last_picked.card)->internal_card_id;

		int csvid = get_id(player, card);
		if (csvid == CARD_ID_FALKENRATH_TORTURER){
			instance->targets[3].player = has_subtype(last_picked.player, last_picked.card, SUBTYPE_HUMAN);
		}
		if (csvid == CARD_ID_WARREN_WEIRDING && has_subtype(last_picked.player, last_picked.card, SUBTYPE_GOBLIN)){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN_ROGUE, &token);
			token.t_player = last_picked.player;
			token.qty = 2;
			token.action = TOKEN_ACTION_HASTE;
			generate_token(&token);
		}
	}

	int c;
	for (c = 0; c < active_cards_count[t_player]; ++c){
		if (marked[c]){
			card_instance_t* inst = get_card_instance(t_player, c);
			inst->state &= ~STATE_TARGETTED;
			inst->state |= STATE_CANNOT_TARGET;	// Hopefully prevent any death triggers from interfering with them.
		}
	}
	if (!(options & SAC_JUST_MARK)){
		for (c = 0; c < active_cards_count[t_player]; ++c){
			if (marked[c]){
				card_instance_t* inst = get_card_instance(t_player, c);
				inst->state &= ~STATE_CANNOT_TARGET;
				if (in_play(t_player, c)){	// Some horrible non-targeted death trigger got rid of it already, augh.
					kill_card(t_player, c, KILL_SACRIFICE);
				}
			}
		}
	}

	int rval = num_picked;
	if (options & SAC_RETURN_CHOICE){
		SET_BYTE2(rval) = last_picked.player;
		SET_BYTE3(rval) = last_picked.card;
	}

	if (marked_for_sacrifice){
		memcpy(marked_for_sacrifice, marked, sizeof(marked));
	}

	cancel = previous_cancel;
	return rval;
}

int new_sacrifice(int player, int card, int t_player, sacrifice_t options, test_definition_t* test)
{
  return new_sacrifice_impl(player, card, t_player, options, test, NULL);
}

int mark_sacrifice(int player, int card, int t_player, sacrifice_t options, test_definition_t* test, char* marked_for_sacrifice)
{
  return new_sacrifice_impl(player, card, t_player, options, test, marked_for_sacrifice);
}

int sacrifice(int player, int card, int t_player, int cannot_cancel, int type, int flag1,
			  int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5)
{
  test_definition_t test;
  new_default_test_definition(&test, type, "");	// Blank prompt here makes new_sacrifice() choose a good default later
  test.type_flag = flag1;
  test.subtype = subtype;
  test.subtype_flag = flag2;
  test.color = color;
  test.color_flag = flag3;
  test.id = id;
  test.id_flag = flag4;
  test.cmc = cc;
  test.cmc_flag = flag5;

  int options = cannot_cancel == 1 ? SAC_NO_CANCEL : (cannot_cancel == -1 ? SAC_DONE : cannot_cancel);
  return new_sacrifice(player, card, t_player, options | SAC_SET_TARGETS, &test);
}

int controller_sacrifices_a_permanent(int player, int card, int req_type, sacrifice_t options)
{
  test_definition_t test;
  new_default_test_definition(&test, req_type, "");	// Blank prompt here makes new_sacrifice() choose a good default later
  return new_sacrifice(player, card, player, options, &test);
}

int player_sacrifices_a_permanent(int src_player, int src_card, int t_player, int req_type, sacrifice_t options)
{
  test_definition_t test;
  new_default_test_definition(&test, req_type, "");	// Blank prompt here makes new_sacrifice() choose a good default later
  return new_sacrifice(src_player, src_card, t_player, options, &test);
}

// only usable with basic land subtypes; use player_sacrifices_a_permanent if you just want a land
int player_sacrifices_a_hacked_land(int src_player, int src_card, int t_player, subtype_t land_subtype, sacrifice_t options)
{
  if (can_cause_sacrifice(src_player, t_player))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(src_player, src_card, "Select %a to sacrifice.", land_subtype));
	  test.subtype = get_hacked_subtype(src_player, src_card, land_subtype);
	  return new_sacrifice(src_player, src_card, t_player, options, &test);
	}
  else
	return 0;
}

int impose_sacrifice(int player, int card, int t_player, int amount, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4,
					int cc, int flag5){

	/* For future development: if amount >= 2, then this should choose all the sacrifice targets first, then sacrifice them all at once; with an option to only
	 * sacrifice if all of amount are chosen (rather than all legal sacrifices, if there aren't enough).  Prevents edge cases such as with Teysa, Orzhov Scion
	 * ("Sacrifice three white creatures: Exile target creature.  Whenever another black creature you control dies, put a 1/1 white Spirit creature with flying
	 * onto the battlefield.") where you can sacrifice the first creature, then choose the newly-generated Spirit as the second. */

	if( amount < 1 ){
		return 0;
	}

	if (!can_cause_sacrifice(player, t_player)){
		return 0;
	}

	int i;
	for(i = 0; i < amount; ++i){
		if (!sacrifice(player, card, t_player, 1, type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5)){
			return i;
		}
	}
	return i;
}

void controller_sacrifices_a_creature(int player, int card){
	sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
}

void player_sacrifices_a_creature(int s_player, int s_card, int t_player){
	sacrifice(s_player, s_card, t_player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
}

int prevent_damage_sacrificing_a_creature(int player, int card, event_t event, int mode){
	// mode = 1 --> Circle of Despair
	// mode = 2 --> Martyr's Cause

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && can_target(&td) ){
		int result = 0;

		if( mode == 1 && has_mana(player, COLOR_COLORLESS, 1) ){
			result = 0x63;
		}

		else if( mode == 2 ){
				 result = 0x63;
		}

		return result;
	}
	else if( event == EVENT_ACTIVATE ){
			 int resolved = 0;
			 if( mode == 1 ){
				 charge_mana(player, COLOR_COLORLESS, 1);
				 if( spell_fizzled != 1 ){
					 resolved = 1;
				 }
			 }
			 else if( mode == 2 ){
					  resolved = 1;
			 }

			 if( resolved == 1 && controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
				 pick_target(&td, "TARGET_DAMAGE");
			 }
			 else{
				  spell_fizzled = 1;
			 }
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			 if( target->info_slot > 0 ){
				 target->info_slot = 0;
			 }
	}
	return global_enchantment(player, card, event);
}

int check_battlefield_for_sacrifice(int player, int type, int flag1, int subtype, int flag2,
									int color, int flag3, int id, int flag4, int cc, int flag5){

	if( player < 2 ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && make_test_in_play(player, count, -1, type, flag1, subtype, flag2,
																color, flag3, id, flag4, cc, flag5)
				  ){
					return 1;
				}
				count++;
		}
	}

	else if( player == 2 ){
			 int i;
			 for(i=0; i<2; i++){
				 int count = 0;
				 while( count < active_cards_count[i] ){
						if( in_play(i, count) && make_test_in_play(i, count, -1, type, flag1, subtype, flag2,
																	color, flag3, id, flag4, cc, flag5)
						  ){
							return 1;
						}
						count++;
				 }
			 }
	}

	return 0;
}

int can_activate_altar_basic(int player, int card, int act_mode, int type){

	int score = 0;
	if( ! is_sick(player, card) && (act_mode & 1) ){
		score+=1;
	}
	if( ! is_animated_and_sick(player, card) && (act_mode & 2) ){
		score+=2;
	}
	if( ! is_tapped(player, card) && (act_mode & 4) ){
		score+=4;
	}
	if( current_phase == PHASE_UPKEEP && (act_mode & 8) ){
		score+=8;
	}
	if( current_turn == player && (act_mode & 16) ){
		score+=16;
	}
	if( score == act_mode ){
		return can_sacrifice_as_cost(player, 1, type, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int altar_basic_activation(int player, int card, int act_mode, int type){

	card_instance_t *instance = get_card_instance(player, card);

	if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
		int result = pick_permanent_for_sacrifice(player, card, type, 0);
		if( result != -1 ){
			instance->number_of_targets = 1;
			instance->targets[1].player = get_power(player, result);
			instance->targets[1].card = get_toughness(player, result);
			instance->targets[2].player = get_color(player, result);
			instance->targets[2].card = get_cmc(player, result);
			kill_card(player, result, KILL_SACRIFICE);
			if( act_mode & 4 ){
				tap_card(player, card);
			}
			return 1;
		}
	}
	return 0;
}

int altar_basic(int player, int card, event_t event, int act_mode, int type){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_activate_altar_basic(player, card, act_mode, type);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int result = altar_basic_activation(player, card, act_mode, type);
		if( result == 0 ){
			spell_fizzled = 1;
			return 0;
		}
	}

	return 0;
}

int altar_extended(int player, int card, event_t event, int act_mode, int type, int flag1,
					int subtype, int flag2, int color, int flag3, int id, int flag4, int cmc, int flag5){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int score = 0;
		if( ! is_sick(player, card) && (act_mode & 1) ){
			score+=1;
		}
		if( ! is_animated_and_sick(player, card) && (act_mode & 2) ){
			score+=2;
		}
		if( ! is_tapped(player, card) && (act_mode & 4) ){
			score+=4;
		}
		if( current_phase == PHASE_UPKEEP && (act_mode & 8) ){
			score+=8;
		}
		if( current_turn == player && (act_mode & 16) ){
			score+=16;
		}
		if( score == act_mode ){
			return can_sacrifice_as_cost(player, 1, type, flag1, subtype, flag2, color, flag3, id, flag4, cmc, flag5);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			int result = pick_special_permanent_for_sacrifice(player, card, 0, type, flag1, subtype, flag2, color,
															  flag3, id, flag4, cmc, flag5);
			if( result != -1 ){
				instance->targets[1].player = get_power(player, result);
				instance->targets[1].card = get_toughness(player, result);
				instance->targets[2].player = get_color(player, result);
				instance->targets[2].card = get_cmc(player, result);
				kill_card(player, result, KILL_SACRIFICE);
				if( act_mode & 4 ){
					tap_card(player, card);
				}
			}
			else{
				spell_fizzled = 1;
			}
	}

	return 0;
}

int sacrifice_and_report_value(int player, int card, int t_player, int flags, test_definition_t *test){
	// default : report the "internal_card_id" of the chosen permanent at the moment it was sacrificed
	//			(so, if you're saccing a Clone copying a Serra Angel, it will report the IID of Serra Angel).
	// If you need a different value, use the flags listed in "manalink.h" in "sacrifice_and_report_value_flags_t"
	int result = -1;
	int sac_flags = SAC_JUST_MARK | SAC_RETURN_CHOICE;
	sac_flags |= ((flags & SARV_EXTRA_IMPOSE_SACRIFICE) ? SAC_NO_CANCEL | SAC_CAUSED : 0);
	int sac = new_sacrifice(player, card, t_player, sac_flags, test);
	if( sac > -1 ){
		int s_player = BYTE2(sac);
		int s_card = BYTE3(sac);
		result = get_card_instance(s_player, s_card)->internal_card_id;
		if( flags & SARV_REPORT_POWER ){
			result = get_power(s_player, s_card);
		}
		if( flags & SARV_REPORT_TOUGHNESS ){
			result = get_toughness(s_player, s_card);
		}
		if( flags & SARV_REPORT_COLORS ){
			result = get_color(s_player, s_card);
		}
		if( flags & SARV_REPORT_CMC ){
			result = get_cmc(s_player, s_card);
		}
		kill_card(s_player, s_card, KILL_SACRIFICE);
	}
	return result;
}
