#include "manalink.h"

int get_test_score(int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){

	int TEST_TYPE = 0;

	if( (type) && flag1 == 0){
		TEST_TYPE+=(1<<0);
	}

	if( (subtype) && flag2 == 0){
		TEST_TYPE+=(1<<1);
	}

	if( (color) && flag3 == 0){
		TEST_TYPE+=(1<<2);
	}

	if( (id) && flag4 == 0){
		TEST_TYPE+=(1<<3);
	}

	if( cc > -1 && flag5 == 0){
		TEST_TYPE+=(1<<4);
	}

	if( (type) && flag1 == 1){
		TEST_TYPE+=(1<<5);
	}

	if( (subtype) && flag2 > 0){
		TEST_TYPE+=(1<<6);
	}

	if( (color) && flag3 == 1){
		TEST_TYPE+=(1<<7);
	}

	if( (id) && flag4 == 1){
		TEST_TYPE+=(1<<8);
	}

	if( cc > -1 && flag5 == 1){
		TEST_TYPE+=(1<<9);
	}

	if( cc > -1 && flag5 == 2){
		TEST_TYPE+=(1<<10);
	}

	if( cc > -1 && flag5 == 3){
		TEST_TYPE+=(1<<11);
	}

	if( (type) && flag1 == 2){
		TEST_TYPE+=(1<<12);
	}

	if( (type) && flag1 == 3){
		TEST_TYPE+=(1<<13);
	}

	if( (id) && flag4 == 2){
		TEST_TYPE+=(1<<14);
	}

	if( (id) && flag4 == 3){
		TEST_TYPE+=(1<<15);
	}

	if( (type) && flag1 == 4){
		TEST_TYPE+=(1<<16);
	}

	if( color == 0 && flag3 == 2){
		TEST_TYPE+=(1<<17);
	}

	if( (type) && flag1 == 5){
		TEST_TYPE+=(1<<18);
	}

	if( (type) && flag1 == 6){
		TEST_TYPE+=(1<<19);
	}

	return TEST_TYPE;
}

int new_get_test_score(test_definition_t *this_test){

	int test_type = 0;

	/* All these different bits seem unnecessary, since each *_flag can only have one value - neither this nor new_make_test() nor new_make_test_in_play() can
	 * ever set both 1<<0 and 1<<5, for example.  Why not just assign one bit for everything that passes type_flag, one for everything that passes color_flag,
	 * etc?  I.e., for type_flag, we'd have
		switch (this_test->type_flag){
			case MATCH:
			case DOESNT_MATCH:
			case F1_NO_CREATURE:
			case F1_NO_TOKEN:
			case F1_NO_PWALKER:
			case F1_TOKEN:
			case F1_NONARTIFACT_CREATURE:
				if (this_type > 0){
					test_type |= 1<<0;
				}
				break;
			default:
				break;
		}
	 * and in new_make_test() and new_make_test_in_play()
		if (this_type > 0){
			int pass;
			switch (this_test->type_flag){
				case MATCH:
					pass = is_what(player, to_examine, this_test->type);
					break;
				case DOESNT_MATCH:
					pass = !is_what(player, to_examine, this_test->type);
					break;
				case F1_NO_CREATURE:
					pass = is_what(player, to_examine, this_test->type) && !is_what(player, to_examine, TYPE_CREATURE);
					break;
				case F1_NO_TOKEN:
					pass = is_what(player, to_examine, this_test->type) && !is_token(player, to_examine);
					break;
				case F1_NO_PWALKER:
					pass = is_what(player, to_examine, this_test->type) && !is_planeswalker(player, to_examine);
					break;
				case F1_TOKEN:
					pass = is_what(player, to_examine, this_test->type) && is_token(player, to_examine);
					break;
				case F1_NONARTIFACT_CREATURE:
					pass = is_what(player, to_examine, TYPE_CREATURE) && !is_what(player, to_examine, TYPE_ARTIFACT);
					break;
				default:
					pass = 0;
					break;
			}
			if (pass){
				test |= 1<<0;
			}
		}
	 *
	 * For that matter, they seem completely unnecessary anyway.  There isn't anything that would be interested in whether something partially passed a test or
	 * not; eliminate new_get_test_score() and just return 0 in new_make_test() and new_make_test_in_play() whenever a test fails.  Replace the last bit of the
	 * above with:

				default:
					pass = 1;
					break;
			}
			if (!pass){
				return 0;
			}
		}
	 *
	 * and you won't have to worry about all these bitfields or whether to make or test a second_test_type; this function won't be needed at all, let alone slow
	 * enough that it needs to be preprocessed; and new_make_test() and new_make_test_in_play() will execute faster - no need to check color and subtype and
	 * power if we see right away that we're looking at an enchantment, not a creature. */

	/* There's at least one place (new_global_tutor()) that inspects test score to see whether the condition is "any card" or a card with conditions.  It's
	 * considered to be "any card" if test score is 0, or if (test score is 1<<0 and test_type->type == TYPE_ANY).  This functionality - though not necessarily
	 * the specific numbers - will need to be preserved. */

	if( this_test->type > 0 && this_test->type_flag == MATCH){
		test_type+=(1<<0);
	}

	if( this_test->subtype > 0 && this_test->subtype_flag == 0 ){
		test_type+=(1<<1);
	}

	if( this_test->color > 0 && this_test->color_flag == MATCH){
		test_type+=(1<<2);
	}

	if( this_test->id > 0 && this_test->id_flag == 0){
		test_type+=(1<<3);
	}

	if( this_test->cmc > -1 && this_test->cmc_flag == 0){
		test_type+=(1<<4);
	}

	if( this_test->type > 0 && this_test->type_flag == DOESNT_MATCH){
		test_type+=(1<<5);
	}

	if( this_test->subtype > 0 && this_test->subtype_flag == 1 ){
		test_type+=(1<<6);
	}

	if( this_test->color > 0 && this_test->color_flag == DOESNT_MATCH){
		test_type+=(1<<7);
	}

	if( this_test->id > 0 && this_test->id_flag == 1){
		test_type+=(1<<8);
	}

	if( this_test->cmc > -1 && this_test->cmc_flag == 1){
		test_type+=(1<<9);
	}

	if( this_test->cmc > -1 && this_test->cmc_flag == 2){
		test_type+=(1<<10);
	}

	if( this_test->cmc > -1 && this_test->cmc_flag == 3){
		test_type+=(1<<11);
	}

	if( this_test->type > 0 && this_test->type_flag == F1_NO_CREATURE){
		test_type+=(1<<12);
	}

	if( this_test->type > 0 && this_test->type_flag == F1_NO_TOKEN){
		test_type+=(1<<13);
	}

	if(  this_test->id > 0 && this_test->id_flag == 2){ // ?
		test_type+=(1<<14);
	}

	if(  this_test->id > 0 && this_test->id_flag == 3){
		test_type+=(1<<15);
	}

	if( this_test->type > 0 && this_test->type_flag == F1_NO_PWALKER){
		test_type+=(1<<16);
	}

	if( this_test->color == 0 && this_test->color_flag == F3_MULTICOLORED){
		test_type+=(1<<17);
	}

	if( this_test->color == 0 && this_test->color_flag == F3_MONOCOLORED){
		test_type+=(1<<18);
	}

	if( this_test->type > 0 && this_test->type_flag == F1_IS_TOKEN){
		test_type+=(1<<19);
	}

	if( this_test->keyword || this_test->keyword_flag){
		test_type+=(1<<20);
	}

	if( this_test->power > -1 && this_test->power_flag == 0){
		test_type+=(1<<22);
	}

	if( this_test->power > -1 && this_test->power_flag == 1){
		test_type+=(1<<23);
	}

	if( this_test->toughness > -1 && this_test->toughness_flag == 0){
		test_type+=(1<<24);
	}

	if( this_test->toughness > -1 && this_test->toughness_flag == 1){
		test_type+=(1<<25);
	}

	if( this_test->state > 0 && this_test->state_flag == 0){
		test_type+=(1<<26);
	}

	if( this_test->state > 0 && this_test->state_flag == 1){
		test_type+=(1<<27);
	}

	if( this_test->power > -1 && this_test->power_flag == 2){
		test_type+=(1<<28);
	}

	if( this_test->power > -1 && this_test->power_flag == 3){
		test_type+=(1<<29);
	}

	if( this_test->toughness > -1 && this_test->toughness_flag == 2){
		test_type+=(1<<30);
	}

	if( this_test->toughness > -1 && this_test->toughness_flag == 3){
		test_type+=(1<<31);
	}

	if (this_test->owner != ANYBODY){
		test_type |= 1<<31;
	}

	return test_type;
}

static int check_multisubtype_by_csvid(int csvid, int sub1, int sub2, int sub3, int sub4, int sub5){
	if( sub1 > 0 && has_subtype_by_id(csvid, sub1) ){
		return 1;
	}
	if( sub2 > 0 && has_subtype_by_id(csvid, sub2) ){
		return 1;
	}
	if( sub3 > 0 && has_subtype_by_id(csvid, sub3) ){
		return 1;
	}
	if( sub4 > 0 && has_subtype_by_id(csvid, sub4) ){
		return 1;
	}
	if( sub5 > 0 && has_subtype_by_id(csvid, sub5) ){
		return 1;
	}
	return 0;
}

static int check_multisubtype_all_by_csvid(int csvid, int sub1, int sub2, int sub3, int sub4, int sub5){
	if( sub1 > 0 && !has_subtype_by_id(csvid, sub1) ){
		return 0;
	}
	if( sub2 > 0 && !has_subtype_by_id(csvid, sub2) ){
		return 0;
	}
	if( sub3 > 0 && !has_subtype_by_id(csvid, sub3) ){
		return 0;
	}
	if( sub4 > 0 && !has_subtype_by_id(csvid, sub4) ){
		return 0;
	}
	if( sub5 > 0 && !has_subtype_by_id(csvid, sub5) ){
		return 0;
	}
	return 1;
}

static int check_multisubtype(int player, int card, int sub1, int sub2, int sub3, int sub4, int sub5){
	if( sub1 > 0 && has_subtype(player, card, sub1) ){
		return 1;
	}
	if( sub2 > 0 && has_subtype(player, card, sub2) ){
		return 1;
	}
	if( sub3 > 0 && has_subtype(player, card, sub3) ){
		return 1;
	}
	if( sub4 > 0 && has_subtype(player, card, sub4) ){
		return 1;
	}
	if( sub5 > 0 && has_subtype(player, card, sub5) ){
		return 1;
	}
	return 0;
}

static int check_multisubtype_all(int player, int card, int sub1, int sub2, int sub3, int sub4, int sub5){
	if( sub1 > 0 && !has_subtype(player, card, sub1) ){
		return 0;
	}
	if( sub2 > 0 && !has_subtype(player, card, sub2) ){
		return 0;
	}
	if( sub3 > 0 && !has_subtype(player, card, sub3) ){
		return 0;
	}
	if( sub4 > 0 && !has_subtype(player, card, sub4) ){
		return 0;
	}
	if( sub5 > 0 && !has_subtype(player, card, sub5) ){
		return 0;
	}
	return 1;
}

void new_default_test_definition(test_definition_t *this_test, int type, const char* msg){
	if (ai_is_speculating == 1){
		*this_test->message = 0;
	} else {
		strcpy(this_test->message, msg);
	}
	this_test->type = type;
	this_test->type_flag = MATCH;
	this_test->subtype = 0;
	this_test->subtype_flag = 0;
	this_test->color = 0;
	this_test->color_flag = MATCH;
	this_test->id = 0;
	this_test->id2 = 0;
	this_test->id_flag = 0;
	this_test->cmc = -1;
	this_test->cmc_flag = 0;
	this_test->keyword = 0;
	this_test->keyword_flag = 0;
	this_test->power = -1;
	this_test->power_flag = 0;
	this_test->toughness = -1;
	this_test->toughness_flag = 0;
	this_test->state = 0;
	this_test->state_flag = 0;
	this_test->has_mana_to_pay_cmc = 0;
	this_test->zone = TARGET_ZONE_IN_PLAY;
	this_test->not_me = 0;
	this_test->ai_selection_mode = AI_MAX_VALUE;
	this_test->sub2 = 0;
	this_test->sub3 = 0;
	this_test->sub4 = 0;
	this_test->sub5 = 0;
	this_test->qty = 1;
	this_test->no_shuffle = 0;
	this_test->create_minideck = 0;
	this_test->can_legally_play = 0;
	this_test->special_selection_function = NULL;
	this_test->value_for_special_selection_function = -1;
	this_test->painters_servant_hack[0] = -1;
	this_test->painters_servant_hack[1] = -1;
	this_test->owner = ANYBODY;
	this_test->storage = -1;
}

void default_test_definition(test_definition_t *this_test, int type){
	new_default_test_definition(this_test, type, "Select a card.");
}

int new_make_test_in_play(int player, int to_examine, int test_type, test_definition_t *this_test){
	// "test_type" now is unused, but it's left here as removing it will require too many recoding

	int id = get_id(player, to_examine);

	if( this_test->can_legally_play )
		if( ! can_legally_play_iid(player, get_card_instance(player, to_examine)->internal_card_id) )
			return 0;

	if( this_test->type ){
		if( this_test->type_flag == MATCH && ! is_what(player, to_examine, this_test->type) ){
			return 0;
		}
		if( this_test->type_flag == DOESNT_MATCH && is_what(player, to_examine, this_test->type) ){
			return 0;
		}
		if( this_test->type_flag == F1_NO_CREATURE && (!is_what(player, to_examine, this_test->type) || is_what(player, to_examine, TYPE_CREATURE)) ){
			return 0;
		}
		if( this_test->type_flag == F1_NO_TOKEN && (!is_what(player, to_examine, this_test->type) || is_token(player, to_examine)) ){
			return 0;
		}
		if( this_test->type_flag == F1_NO_PWALKER && (!is_what(player, to_examine, this_test->type) || is_planeswalker(player, to_examine)) ){
			return 0;
		}
		if( this_test->type_flag == F1_IS_TOKEN && (!is_what(player, to_examine, this_test->type) || !is_token(player, to_examine)) ){
			return 0;
		}
		if( this_test->type_flag == F1_NONARTIFACT_CREATURE && ( ! is_what(player, to_examine, TYPE_CREATURE) || is_what(player, to_examine, TYPE_ARTIFACT) ) ){
			return 0;
		}
		/*
		if( this_test->type_flag == F1_MATCH_OR_FLASH && ! is_token(player, to_examine, TYPE_CREATURE) ){
			return 0;
		}
		*/
		if( this_test->type_flag == F1_MATCH_ALL && (get_type(player, to_examine) & this_test->type) != this_test->type ){
			return 0;
		}
	}

	if( this_test->subtype ){
		if( this_test->subtype_flag == F2_MULTISUBTYPE && ! check_multisubtype(player, to_examine, this_test->subtype, this_test->sub2, this_test->sub3, this_test->sub4, this_test->sub5) ){
			return 0;
		}
		if( this_test->subtype_flag == F2_MULTISUBTYPE_ALL && ! check_multisubtype_all(player, to_examine, this_test->subtype, this_test->sub2, this_test->sub3, this_test->sub4, this_test->sub5) ){
			return 0;
		}
		if( this_test->subtype_flag == MATCH && ! has_subtype(player, to_examine, this_test->subtype) ){
			return 0;
		}
		if( this_test->subtype_flag == DOESNT_MATCH && has_subtype(player, to_examine, this_test->subtype) ){
			return 0;
		}
	}

	if( this_test->color ){
		int clr = get_color(player, to_examine);
		if( this_test->color == COLOR_TEST_COLORLESS ){
			if( this_test->color_flag == MATCH && clr != 0 ){
				return 0;
			}
			if( this_test->color_flag == DOESNT_MATCH && clr == 0 ){
				return 0;
			}
		}
		else{
			if( this_test->color_flag == MATCH && !(clr & this_test->color) ){
				return 0;
			}
			if( this_test->color_flag == DOESNT_MATCH && (clr & this_test->color) ){
				return 0;
			}
		}
	}

	if( this_test->color == 0 ){
		if( this_test->color_flag == F3_MULTICOLORED && count_colors(player, to_examine) <= 1 ){
			return 0;
		}
		if( this_test->color_flag == F3_MONOCOLORED && count_colors(player, to_examine) != 1 ){
			return 0;
		}
	}

	if( this_test->id == -1 ){ //We're checking agains a card with no name, so the test is always negative.
		return 0;
	}

	if( this_test->id ){
		if( this_test->id_flag == F4_TOKEN_W_SPECIAL_INFOS && ( ! is_token(player, to_examine) || get_special_infos(player, to_examine) != this_test->id)  ){
			return 0;
		}
		if( this_test->id_flag == F4_DOUBLE_ID_TUTOR && id != this_test->id && id != this_test->id2 ){
			return 0;
		}
		if( this_test->id_flag == MATCH && id != this_test->id ){
			return 0;
		}
		if( this_test->id_flag == DOESNT_MATCH && id == this_test->id ){
			return 0;
		}
	}

	if( this_test->cmc > -1 ){
		int cmc = get_cmc(player, to_examine);
		if( this_test->cmc_flag == MATCH && cmc != this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == DOESNT_MATCH && cmc == this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == F5_CMC_GREATER_THAN_VALUE && cmc <= this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == F5_CMC_LESSER_THAN_VALUE && cmc >= this_test->cmc ){
			return 0;
		}
	}

	if( this_test->keyword ){
		if( this_test->keyword_flag == MATCH && ! check_for_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
		if( this_test->keyword_flag == DOESNT_MATCH && check_for_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
		if( this_test->keyword_flag == F0_HAS_SP_KEYWORD && ! check_for_special_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
		if( this_test->keyword_flag == F0_DOESNT_HAVE_SP_KEYWORD && check_for_special_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
	}

	if( this_test->power > -1 ){
		int pow = get_power(player, to_examine);
		if( this_test->power_flag == MATCH && pow != this_test->power ){
			return 0;
		}
		if( this_test->power_flag == DOESNT_MATCH && pow == this_test->power ){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_GREATER_THAN_VALUE && pow <= this_test->power){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_LESSER_THAN_VALUE && pow >= this_test->power ){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_1_1_COUNTERS_GREATER_THAN_VALUE && count_1_1_counters(player, to_examine) <= this_test->power ){
			return 0;
		}
	}

	if( this_test->toughness > -1 ){
		int tou = get_toughness(player, to_examine);
		if( this_test->toughness_flag == MATCH && tou != this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == DOESNT_MATCH && tou == this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == F5_TOUGHNESS_GREATER_THAN_VALUE && tou <= this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == F5_TOUGHNESS_LESSER_THAN_VALUE && tou >= this_test->toughness ){
			return 0;
		}
	}

	if( this_test->state ){
		if( this_test->state_flag == MATCH && ! check_state(player, to_examine, this_test->state) ){
			return 0;
		}
		if( this_test->state_flag == DOESNT_MATCH && check_state(player, to_examine, this_test->state) ){
			return 0;
		}
	}

	if( this_test->special_selection_function != NULL ){
		if( ! this_test->special_selection_function(get_card_instance(player, to_examine)->internal_card_id, this_test->value_for_special_selection_function, player, to_examine) ){
			return 0;
		}
	}
	if( this_test->has_mana_to_pay_cmc ){
		if( this_test->has_mana_to_pay_cmc == 1 ){
			if( ! has_mana(player, COLOR_COLORLESS, get_cmc(player, to_examine)) ){
				return 0;
			}
		}
		/*
		if( has_mana_to_pay_cmc == 2 ){
			if( ! has_mana_to_cast_iid(player, event, COLOR_COLORLESS, get_cmc(player, to_examine)) ){
				return 0;
			}
		}
		*/
	}
	if (this_test->owner != ANYBODY && get_owner(player, to_examine) != this_test->owner){
		return 0;
	}
	return 1;
}

int make_test_in_play(int player, int to_examine, int test_type, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t this_test;
	default_test_definition(&this_test, type);
	this_test.type = type;
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	return new_make_test_in_play(player, to_examine, test_type, &this_test);
}

int new_make_test(int player, int to_examine, int test_type, test_definition_t *this_test){
	// "test_type" now is unused, but it's left here as removing it will require too many recoding

	int id = cards_data[to_examine].id;

	if( this_test->can_legally_play )
		if( ! can_legally_play_iid(player, to_examine) )
			return 0;

	if( this_test->type ){
		if( this_test->type_flag == MATCH && ! is_what(-1, to_examine, this_test->type) ){
			return 0;
		}
		if( this_test->type_flag == DOESNT_MATCH && is_what(-1, to_examine, this_test->type) ){
			return 0;
		}
		if( this_test->type_flag == F1_NO_CREATURE && (!is_what(-1, to_examine, this_test->type) || is_what(-1, to_examine, TYPE_CREATURE)) ){
			return 0;
		}
		if( this_test->type_flag == F1_NO_PWALKER && (!is_what(-1, to_examine, this_test->type) || is_planeswalker(-1, to_examine)) ){
			return 0;
		}
		if( this_test->type_flag == F1_NONARTIFACT_CREATURE && ( ! is_what(-1, to_examine, TYPE_CREATURE) || is_what(-1, to_examine, TYPE_ARTIFACT) ) ){
			return 0;
		}
		/*
		if( this_test->type_flag == F1_MATCH_OR_FLASH && ! is_token(-1, to_examine, TYPE_CREATURE) ){
			return 0;
		}
		*/
		if( this_test->type_flag == F1_MATCH_ALL && (get_type(-1, to_examine) & this_test->type) != this_test->type ){
			return 0;
		}
	}

	if( this_test->subtype ){
		if( this_test->subtype_flag == F2_MULTISUBTYPE && ! check_multisubtype_by_csvid(id, this_test->subtype, this_test->sub2, this_test->sub3, this_test->sub4, this_test->sub5) ){
			return 0;
		}
		if( this_test->subtype_flag == F2_MULTISUBTYPE_ALL && ! check_multisubtype_all_by_csvid(id, this_test->subtype, this_test->sub2, this_test->sub3, this_test->sub4, this_test->sub5) ){
			return 0;
		}
		if( this_test->subtype_flag == MATCH && ! has_subtype_by_id(id, this_test->subtype) ){
			return 0;
		}
		if( this_test->subtype_flag == DOESNT_MATCH && has_subtype_by_id(id, this_test->subtype) ){
			return 0;
		}
	}

	int clr = 0;
	if (this_test->color > 0 || this_test->color_flag == F3_MONOCOLORED || this_test->color_flag == F3_MULTICOLORED){
		if (this_test->painters_servant_hack[player] == -1){
			this_test->painters_servant_hack[player] = get_global_color_hack(player);
		}
		clr = get_color_by_internal_id_no_hack(to_examine) | this_test->painters_servant_hack[player];
	}
	if( this_test->color ){
		if( this_test->color_flag == MATCH && ! (clr & this_test->color) ){
			return 0;
		}
		if( this_test->color_flag == DOESNT_MATCH && (clr & this_test->color) ){
			return 0;
		}
	}

	if( this_test->color == 0 ){
		if( this_test->color_flag == F3_MULTICOLORED && real_count_colors(clr) <= 1 ){
			return 0;
		}
		if( this_test->color_flag == F3_MONOCOLORED && real_count_colors(clr) != 1 ){
			return 0;
		}
	}

	if( this_test->id == -1 ){ //We're checking agains a card with no name, so the test is always negative.
		return 0;
	}

	if( this_test->id ){
		if( this_test->id_flag == F4_DOUBLE_ID_TUTOR && id != this_test->id && id != this_test->id2 ){
			return 0;
		}
		if( this_test->id_flag == MATCH && id != this_test->id ){
			return 0;
		}
		if( this_test->id_flag == DOESNT_MATCH && id == this_test->id ){
			return 0;
		}
	}

	if( this_test->cmc > -1 ){
		int cmc = get_cmc_by_id( cards_data[to_examine].id );
		if( this_test->cmc_flag == MATCH && cmc != this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == DOESNT_MATCH && cmc == this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == F5_CMC_GREATER_THAN_VALUE && cmc <= this_test->cmc ){
			return 0;
		}
		if( this_test->cmc_flag == F5_CMC_LESSER_THAN_VALUE && cmc >= this_test->cmc ){
			return 0;
		}
	}

	if( this_test->keyword ){
		if( this_test->keyword_flag == MATCH && ! (cards_data[to_examine].static_ability & this_test->keyword)  ){
			return 0;
		}
		if( this_test->keyword_flag == DOESNT_MATCH && (cards_data[to_examine].static_ability & this_test->keyword) ){
			return 0;
		}
		/* Currently disabled as there's no way to know if a card has a "special_ability" if it's not in play
		if( this_test->keyword_flag == F0_HAS_SP_KEYWORD && ! check_for_special_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
		if( this_test->keyword_flag == F0_DOESNT_HAVE_SP_KEYWORD && check_for_special_ability(player, to_examine, this_test->keyword) ){
			return 0;
		}
		*/
	}

	if( this_test->power > -1 ){
		int pow = get_base_power_iid(player, to_examine);
		if( this_test->power_flag == MATCH && pow != this_test->power ){
			return 0;
		}
		if( this_test->power_flag == DOESNT_MATCH && pow == this_test->power ){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_GREATER_THAN_VALUE && pow <= this_test->power){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_LESSER_THAN_VALUE && pow >= this_test->power ){
			return 0;
		}
		if( this_test->power_flag == F5_POWER_1_1_COUNTERS_GREATER_THAN_VALUE ){
			// Cards not in play have no counters
			return 0;
		}
	}

	if( this_test->toughness > -1 ){
		int tou = get_base_toughness_iid(player, to_examine);
		if( this_test->toughness_flag == MATCH && tou != this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == DOESNT_MATCH && tou == this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == F5_TOUGHNESS_GREATER_THAN_VALUE && tou <= this_test->toughness ){
			return 0;
		}
		if( this_test->toughness_flag == F5_TOUGHNESS_LESSER_THAN_VALUE && tou >= this_test->toughness ){
			return 0;
		}
	}

	if( this_test->special_selection_function != NULL ){
		if( ! this_test->special_selection_function(to_examine, this_test->value_for_special_selection_function, player, -1) ){
			return 0;
		}
	}
	if( this_test->has_mana_to_pay_cmc ){
		if( this_test->has_mana_to_pay_cmc == 1 ){
			if( ! has_mana(player, COLOR_COLORLESS, get_cmc_by_id(cards_data[to_examine].id)) ){
				return 0;
			}
		}
		/*
		if( has_mana_to_pay_cmc == 2 ){
			if( ! has_mana_to_cast_iid(player, event, COLOR_COLORLESS, get_cmc(player, to_examine)) ){
				return 0;
			}
		}
		*/
	}
	if (this_test->owner != ANYBODY && player != this_test->owner){
		return 0;
	}

	return 1;
}

int make_test(int player, int to_examine, int test_type, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t this_test;
	default_test_definition(&this_test, type);
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	return new_make_test(player, to_examine, test_type, &this_test);
}

int my_base_value_by_id( int csvid ){
	card_ptr_t* c = cards_ptr[ csvid ];
	int result = c->ai_base_value;

	if( result < 1 ){
		result = 0;
		int iid = get_internal_card_id_from_csv_id(csvid);
		if( is_what(-1, iid, TYPE_CREATURE) ){
			result += get_base_power_iid(-1, iid);
			result += get_base_toughness_iid(-1, iid);
			result -= get_cmc_by_id(csvid)*2;
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].static_ability & (1<<i) ){
					result+=4;
				}
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=8;
				}
			}
			// Flash
			if(cards_data[iid].type & TYPE_INSTANT){
				result+=10;
			}
		}
		if( is_what(-1, iid, TYPE_ARTIFACT)){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(c->mana_source_colors)*2;
			}
			result-=get_cmc_by_id(csvid);
		}
		if( is_what(-1, iid, TYPE_ENCHANTMENT) || is_planeswalker(-1, iid) ){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(c->mana_source_colors)*2;
			}
			result-=get_cmc_by_id(csvid);
			if( is_planeswalker(-1, iid) ){
				result+=20;
			}
		}
		if( is_what(-1, iid, TYPE_LAND)){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=2;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(c->mana_source_colors)*2;
				result+=10;
			}
		}
		if( is_what(-1, iid, TYPE_SPELL) && ! is_what(-1, iid, TYPE_CREATURE) ){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			result-=get_cmc_by_id(csvid);
		}
		result*=5;
		if( result < 0 ){
			result = 0;
		}
	}
	return result;
}

int my_base_value(int player, int card ){
	int result = 0;
	if( player == -1 ){
		result = my_base_value_by_id(cards_data[card].id);
	}
	else{
		int cmc = in_play(player, card) ? -get_cmc(player, card) : get_cmc(player, card);

		card_instance_t *instance = get_card_instance( player, card );
		int iid = instance->internal_card_id;
		int csvid = cards_data[iid].id;
		card_ptr_t* cp = cards_ptr[csvid];

		result = 0;
		if( is_what(player, card, TYPE_CREATURE) ){
			result+=get_power(player, card);
			result+=get_toughness(player, card);
			result-=cmc*2;
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].static_ability & (1<<i) ){
					result+=4;
				}
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=8;
				}
			}
			// Flash
			if( (get_card_data(player, card)->type & TYPE_INSTANT) && !in_play(player, card) ){
				result+=10;
			}
		}
		if( is_what(player, card, TYPE_ARTIFACT)){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(cp->mana_source_colors)*2;
			}
			result-=cmc;
		}
		if( is_what(player, card, TYPE_ENCHANTMENT) || is_planeswalker(player, card) ){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(cp->mana_source_colors)*2;
			}
			result-=cmc;
			if( is_planeswalker(player, card) ){
				result+=20;
			}
		}
		if( is_what(player, card, TYPE_LAND)){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=2;
				}
			}
			if( cards_data[iid].extra_ability & EA_MANA_SOURCE ){
				result+=real_count_colors(cp->mana_source_colors)*2;
				result+=10;
			}
		}
		if( is_what(player, card, TYPE_SPELL) && ! is_what(player, card, TYPE_CREATURE) ){
			int i;
			for(i=0; i<32; i++){
				if( cards_data[iid].extra_ability & (1<<i) ){
					result+=4;
				}
			}
			result-=cmc;
		}
		result*=5;
		if( result < 0 ){
			result = 0;
		}
	}
	return result;
}

extern int hack_illusionary_mask_event;	// must set to event when setting test_definition_t::has_mana_to_pay_cmc to 2
int check_battlefield_for_special_card(int player, int card, int t_player, int mode, test_definition_t *this_test){
	//	mode = 0 -> return 1 if a card of the requested type is found. Otherwise, it'll return 0;
	//	mode = 1 -> return the best choice among the cards of the requested type. If nothing is found, it will return -1;
	//	mode = 2 -> return the worst return AI best choice of that card. If nothing is found, it will return -1;
	//	mode = 4 -> return the global count of cards of the requested type.
	//	mode = 8 -> cards found must be legal targets for (player, card).  Strenuously avoid.

	target_definition_t td;
	card_instance_t* instance = NULL;

	if (mode & CBFSC_LEGAL_TARGET){
		default_target_definition(player, card, &td, this_test->type);
		td.allowed_controller = t_player;
		td.preferred_controller = t_player;
		if (this_test->type_flag == DOESNT_MATCH){
			td.required_type = this_test->zone == TARGET_ZONE_IN_PLAY ? TYPE_PERMANENT : TYPE_PERMANENT | TYPE_SPELL;
			td.illegal_type = this_test->type | TYPE_EFFECT;
		}

		instance = get_card_instance(player, card);
	}

	int result = (mode & (CBFSC_AI_MAX_VALUE | CBFSC_AI_MIN_VALUE)) ? -1 : 0;
	int par = (mode & (CBFSC_GET_MIN_CMC | CBFSC_AI_MIN_VALUE)) ? 1000 : 0;
	int i;
	for(i=0; i<2; i++){
		if( t_player == 2 || i == t_player ){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( (in_play(i, count) && this_test->zone == TARGET_ZONE_IN_PLAY && ! check_state(i, count, STATE_CANNOT_TARGET) ) ||
						(in_hand(i, count) && this_test->zone == TARGET_ZONE_HAND)){
						if( new_make_test_in_play(i, count, -1, this_test) ){
							if( this_test->not_me == 0 || (this_test->not_me == 1 && !( i == player && count == card)) ){
								int good = 1;
								if( mode & CBFSC_LEGAL_TARGET ){
									instance->targets[0].player = i;
									instance->targets[0].card = count;
									instance->number_of_targets = 1;
									if( ! valid_target(&td) ){
										good = 0;
									}
								}
								if( this_test->has_mana_to_pay_cmc == 1 && ! has_mana(player, COLOR_COLORLESS, get_cmc(i, count)) ){
									good = 0;
								}
								if( this_test->has_mana_to_pay_cmc == 2 && ! has_mana_to_cast_iid(player, hack_illusionary_mask_event, get_original_internal_card_id(i, count)) ){
									good = 0;
								}
								if( i == player && count == card && (mode & CBFSC_EXCLUDE_ME) ){
									good = 0;
								}
								if( good == 1 ){
									if( mode == CBFSC_CHECK_ONLY ){
										return 1;
									}
									if( mode & CBFSC_GET_COUNT ){
										result++;
									}
									if( (mode & CBFSC_GET_TOTAL_TOU) && is_what(i, count, TYPE_CREATURE) ){
										result+=get_toughness(i, count);
									}
									if( (mode & CBFSC_GET_TOTAL_POW) && is_what(i, count, TYPE_CREATURE) ){
										result+=get_power(i, count);
									}
									if( (mode & CBFSC_GET_TOTAL_CMC) ){
										result+=get_cmc(i, count);
									}
									if( (mode & CBFSC_GET_MAX_TOU) && is_what(i, count, TYPE_CREATURE) ){
										if( par < get_toughness(i, count)  ){
											result = par = get_toughness(i, count);
										}
									}
									if( (mode & CBFSC_GET_MAX_POW) && is_what(i, count, TYPE_CREATURE) ){
										if( par < get_power(i, count)  ){
											result = par = get_power(i, count);
										}
									}
									if( (mode & CBFSC_GET_MAX_CMC) ){
										if( par < get_cmc(i, count)  ){
											result = par = get_cmc(i, count);
										}
									}
									if( (mode & CBFSC_GET_MIN_CMC) ){
										if( par > get_cmc(i, count)  ){
											result = par = get_cmc(i, count);
										}
									}
									if( (mode & CBFSC_AI_MAX_VALUE) ){
										if( ! is_token(i, count) ){
											if( my_base_value(i, count) > par){
												par = my_base_value(i, count);
												result = count;
											}
										}
										else{
											if( get_power(i, count) > par){
												par = get_power(i, count);
												result = count;
											}
										}
									}
									if( (mode & CBFSC_AI_MIN_VALUE) ){
										if( ! is_token(i, count) ){
											if( my_base_value(i, count) < par){
												par = my_base_value(i, count);
												result = count;
											}
										}
										else{
											if( get_power(i, count) < par){
												par = get_power(i, count);
												result = count;
											}
										}
									}
								}
							}
						}
					}
					count--;
			}
		}
	}
	return result;
}

int select_card_to_play_from_zone(int player, int card, event_t event, const int *zone, test_definition_t *test, int mode){
	int ng[count_zone(player, zone)];
	int ngc = 0;
	int count = 0;
	int result = -1;
	while( zone[count] != -1 ){
			if( new_make_test(player, zone[count], -1, test)  ){
				int good = 0;
				if( ! mode && has_mana_to_cast_iid(player, event, zone[count]) ){
					good = 1;
				}
				if( mode == 1 ){
					int cless = get_updated_casting_cost(player, -1, event, zone[count], -1);
					if( has_mana(player, COLOR_COLORLESS, cless) ){
						good = 1;
					}
				}
				if( good ){
					ng[ngc] = zone[count];
					ngc++;
				}
			}
			count++;
	}
	int selected = select_card_from_zone(player, player, ng, ngc, 0, AI_MAX_CMC, -1, test);
	if( selected != -1 ){
		count = 0;
		while( zone[count] != -1 ){
				if( zone[count] == ng[selected] ){
					result = count;
					break;
				}
				count++;
		}
	}
	return result;
}

int check_battlefield_for_name(int player, int type, int name){
	APNAP(p,{ 
				if( player == ANYBODY || p == player ){
					int c;
					for(c=0; c<active_cards_count[p]; c++){
						if( in_play(p, c) && is_what(p, c, type) && get_card_name(p, c) == name ){
							return 1;
						}
					}
				}
			};
	);
	return 0;
}

