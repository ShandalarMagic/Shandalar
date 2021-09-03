#include "manalink.h"

extern int target_controller, target_card;
int sub_4378E0(int, int, const char*, int, int, int, int, int, int*, target_t*, int, int);
int sub_45CEC0(void);
int sub_45CF00(void);
int sub_47E980(int, int);
int sub_47EF10(int, int);
int sub_47EF50(int, int);
int sub_498F20(void);
int sub_499050(void);

static int hack_gpf_player = -1;
static int hack_gpf_card = -1;

int
get_protections_from(int player, int card)
{
  // Original at 0x401E20

  int kw = KEYWORD_SHROUD;	// everything that targets calls get_protections_from(); everything that doesn't, doesn't.

  card_instance_t* instance = get_card_instance(player, card);
  int iid;
  while (instance->internal_card_id == activation_card)
	{
	  player = instance->parent_controller;
	  card = instance->parent_card;
	  instance = get_card_instance(player, card);
	}

  if (instance->internal_card_id == -1)
	iid = instance->backup_internal_card_id;
  else
	iid = instance->internal_card_id;

  uint8_t typ = cards_data[iid].type;

  if (typ & TYPE_SORCERY)
	kw |= KEYWORD_PROT_SORCERIES;

  if ((typ & (TYPE_INSTANT | TYPE_INTERRUPT))		// protection from instants also protects from old-style interrupts
	  && !(typ & (TYPE_PERMANENT | TYPE_SORCERY)))	// ... but not from permanents with flash, or instant-speed sorceries
	kw |= KEYWORD_PROT_INSTANTS;

  if (typ & TYPE_CREATURE)
	kw |= KEYWORD_PROT_CREATURES;

  if (typ & TYPE_LAND)
	kw |= KEYWORD_PROT_LANDS;

  if (typ & TYPE_ARTIFACT)
	kw |= KEYWORD_PROT_ARTIFACTS;

  kw |= (get_color(player, card) & COLOR_TEST_ANY_COLORED) << 10;

  /* We'll use these globals if one of the targeting frontends called by the exe - real_validate_target(), real_select_target(), real_target_available() is
   * called with KEYWORD_SHROUD set in illegal abilities; that doesn't happen any other way than through an immediately-preceeding call to
   * get_protections_from(). */
  hack_gpf_player = player;
  hack_gpf_card = card;

  return kw;
}

is_protected_from_mode_t protection_mode = IPF_TARGET;
int
is_protected_from(int protected_player, int protected_card, int from_player, int from_card, is_protected_from_mode_t new_protection_mode)
{
  int abils;
  if (protected_card >= 0)
	abils = get_abilities(protected_player, protected_card, EVENT_ABILITIES, -1);
  else
#if 1
	return 0;
#else
	return is_player_protected_from(protected_player, from_player, from_card, new_protection_mode);
#endif

  if (from_card >= 0)
	{
	  int gpf = get_protections_from(from_player, from_card) & ~KEYWORD_SHROUD;	// Shroud is checked for in the targeting functions independently of this

	  if (abils & gpf)
		return 1;
	}

#if 0
  /* Obviously embryonic.  We'll want to designate one of the KEYWORD_PROT_* bits to indicate an event should be sent to (protected_player,protected_card) to
   * find out, and another to indicate an event should be sent to all cards.  But not quite yet. */

  if (abils & (KEYWORD_SEND_PROTECTION_MSG_TO_ONE_CARD | KEYWORD_SEND_PROTECTION_MSG_TO_ALL))
	{
	  is_protected_from_mode_t old_protection_mode = protection_mode;
	  protection_mode = new_protection_mode;

	  int result;
	  if (abils & KEYWORD_SEND_PROTECTION_MSG_TO_ALL)
		result = dispatch_event_with_attacker(protected_player, protected_card, EVENT_IS_PROTECTED_FROM, from_player, from_card);
	  else
		result = dispatch_event_with_attacker_to_one_card(protected_player, protected_card, EVENT_IS_PROTECTED_FROM, from_player, from_card);

	  protection_mode = old_protection_mode;
	  if (result)
		return 1;
	}
#endif

  return 0;
}

static int target_match_type(card_instance_t* tgt_instance, int tgt_player, int tgt_card, int special, int type)
{
  // int internal_id = (special & TARGET_SPECIAL_0x200) ? (int)tgt_instance->original_internal_card_id : tgt_instance->internal_card_id;
  // TARGET_SPECIAL_0x200 set for Vesuvan Doppelganger, Copy Artifact, and the exe version of clone - it really should be the target's current type under modern rules so ignore
  int internal_id = tgt_instance->internal_card_id;

  if (special & TARGET_SPECIAL_EFFECT_CARD)
	{
	  if (is_what(-1, tgt_instance->original_internal_card_id, type & (TYPE_ANY | TARGET_TYPE_PLANESWALKER)))
		return 1;

	  type |= TYPE_EFFECT;
	}
  else
	{
	  if (is_what(tgt_player, tgt_card, type & (TYPE_ANY | TARGET_TYPE_PLANESWALKER)))
		return 1;
	}

  if ((type & TARGET_TYPE_TOKEN) && is_token(tgt_player, tgt_card))
	return 1;

  if ((type & TARGET_TYPE_NONCREATURE_CAN_BLOCK) && (tgt_instance->state & STATE_NONCREATURE_CAN_BLOCK))
	return 1;

  if ((type & TYPE_EFFECT)
	  && (cards_data[internal_id].id == 903	// various "ability" legacy (Stoning / TakeAbility / Noattack)
		  || cards_data[internal_id].id == 905))	// "hunting" legacy from Aswan Jaguar ability
	return 1;

  if ((type & TARGET_TYPE_DAMAGE_LEGACY) && cards_data[internal_id].id == 901)
	return 1;

  if ((type & TARGET_TYPE_HACK_SLEIGHT_LEGACY) && cards_data[internal_id].id == 902)
	return 1;

  if ((type & TARGET_TYPE_DRAW_CARD_LEGACY) && cards_data[internal_id].id == 904)
	return 1;

  return 0;
}

static int
validate_target_impl(int player, int card,	// Beware - these will both be -1 when called from the exe
					 int tgt_player, int tgt_card, char* return_error_str,
					 int who_chooses, int allowed_controller, int preferred_controller, int zone,
					 int required_type, int illegal_type, int required_abilities, int illegal_abilities,
					 int required_color, int illegal_color, int extra, int required_subtype,
					 int power_requirement, int toughness_requirement, int special, int required_state,
					 int illegal_state)
{
  int rval = 1;
  char error_str[200];

#define FAIL_STR(err_str)				\
  do									\
	{									\
	  rval = 0;							\
	  if (err_str[0] != ',')			\
		strcat(&error_str[0], ",");		\
	  strcat(&error_str[0], err_str);	\
	  goto epilog;						\
	} while (0)

#define FAILURE(error_addr)							\
	do												\
	  {												\
		rval = 0;									\
		strcat(&error_str[0], EXE_STR(error_addr));	\
		goto epilog;								\
	  } while (0)

  if (tgt_player == -1
	  || (tgt_card != -1
		  && get_card_instance(tgt_player, tgt_card)->internal_card_id == -1))
	{
	  if (return_error_str)
		*return_error_str = 0;
	  return 0;
	}

  error_str[0] = 0;
  if (tgt_card == -1)
	{
	  // Targeting a player.

	  int can_target_player_0, can_target_player_1;

	  if (zone && !(zone & TARGET_ZONE_PLAYERS))
		can_target_player_0 = can_target_player_1 = 0;
	  else if (((who_chooses == HUMAN || (trace_mode & 2)) && (allowed_controller & 2))
			   || (who_chooses == AI && (preferred_controller & 2)))
		can_target_player_0 = can_target_player_1 = 1;
	  else if (((who_chooses == HUMAN || (trace_mode & 2)) && (allowed_controller & 1))
			   || (who_chooses == AI && (preferred_controller & 1)))
		{
		  can_target_player_0 = 0;
		  can_target_player_1 = 1;
		}
	  else
		{
		  can_target_player_0 = 1;
		  can_target_player_1 = 0;
		}

	  if ((illegal_abilities & KEYWORD_SHROUD)
		  && ((player_bits[tgt_player] & PB_PLAYER_HAS_SHROUD) || (who_chooses != tgt_player && (player_bits[tgt_player] & PB_PLAYER_HAS_HEXPROOF))))
		FAILURE(0x785F94);//",abilities"

	  if ((tgt_player == 0 && !can_target_player_0)
		  || (tgt_player == 1 && !can_target_player_1))
		FAILURE(0x7366D8);//",player"
	}
  else
	{
	  card_instance_t* tgt_instance = get_card_instance(tgt_player, tgt_card);

	  if (tgt_instance->internal_card_id < 0 || (tgt_instance->state & (STATE_OUBLIETTED|STATE_CANNOT_TARGET)))
		FAILURE(0x785E64);//",can't target this"

	  target_zone_t in_zone = in_play(tgt_player, tgt_card) ? TARGET_ZONE_IN_PLAY : in_hand(tgt_player, tgt_card) ? TARGET_ZONE_HAND : TARGET_ZONE_ON_STACK;
	  if (in_zone == TARGET_ZONE_IN_PLAY && !(cards_data[tgt_instance->internal_card_id].type & (TYPE_PERMANENT | TYPE_EFFECT)))	// raw typecheck instead of is_what() is deliberate
		FAILURE(0x785E64);//",can't target this"	// A non-permanent spell, currently resolving.  Probably the card doing the targeting.

	  if (zone && !(zone & in_zone))
		FAILURE(0x7875BC);//",where"

	  {
		int query_controller = (who_chooses == HUMAN || (trace_mode & 2)) ? allowed_controller : preferred_controller;
		int ok_controller;
		int ok_owner;

		if (query_controller & TARGET_PLAYER_OWNER)
		  {
			ok_owner = (query_controller & 2) ? -1 : (query_controller & 1) ? 1 : 0;
			ok_controller = ((query_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER) ? ok_owner : -1;
		  }
		else
		  {
			ok_controller = (query_controller & 2) ? -1 : (query_controller & 1) ? 1 : 0;
			ok_owner = ((query_controller & TARGET_PLAYER_OWNER_AND_CONTROLLER) == TARGET_PLAYER_OWNER_AND_CONTROLLER) ? ok_controller : -1;
		  }

		if (ok_controller != -1 && tgt_player != ok_controller)
		  FAILURE(0x786398);//",controller"

		if (ok_owner != -1
			&& ((tgt_instance->state & STATE_OWNED_BY_OPPONENT) ? !ok_owner : ok_owner))
		  FAILURE(0x62BBBC);//",owner"
	  }

	  if ((special & TARGET_SPECIAL_EFFECT_CARD)
		  && !(cards_data[tgt_instance->internal_card_id].type & TYPE_EFFECT))
		FAIL_STR("effect");

	  if (required_type && !target_match_type(tgt_instance, tgt_player, tgt_card, special, required_type))
		FAILURE(0x728F6C);//",type"

	  if (illegal_type && target_match_type(tgt_instance, tgt_player, tgt_card, special, illegal_type))
		FAILURE(0x728F6C);//",type"

	  if (cards_data[tgt_instance->internal_card_id].code_pointer == 0x401010	// multiblocker shadow
		  && !(special & TARGET_SPECIAL_ALLOW_MULTIBLOCKER))
		FAILURE(0x728F6C);//",type"

	  if (required_abilities
		  && (tgt_instance->regen_status & required_abilities) != (uint32_t)required_abilities)
		FAILURE(0x785F94);//",abilities"

	  if (illegal_abilities && tgt_instance->internal_card_id != damage_card
		  && in_zone == TARGET_ZONE_IN_PLAY)
		{
		  if (tgt_instance->regen_status & (illegal_abilities & ~(KEYWORD_PROT_INTERRUPTS|KEYWORD_SHROUD)))
			FAILURE(0x785F94);//",abilities"

		  /* KEYWORD_SHROUD is always set if illegal_abilities has been set through get_protections_from(), and never otherwise; it's a reliable indicator of
		   * whether the choice is really targeting in the game sense or just an untargeted choice.  Just checking illegal_abilities != 0 isn't quite
		   * sufficient, since there's occasional non-targeted choices that do restrict by ability - some examples are Raging River. */
		  if (illegal_abilities & KEYWORD_SHROUD)
			{
			  if (tgt_instance->regen_status & KEYWORD_SHROUD)
				{
				  if (get_id(tgt_player, tgt_card) != CARD_ID_AUTUMN_WILLOW
					  || !(tgt_instance->targets[1].player > 0
						   && (tgt_instance->targets[1].player & (1 << who_chooses))))
					FAILURE(0x785F94);//",abilities"
				}

			  if (who_chooses != tgt_player
				  && check_for_special_ability(tgt_player, tgt_card, SP_KEYWORD_HEXPROOF)
				  && (!is_what(tgt_player, tgt_card, TYPE_CREATURE)
					  || !check_special_flags(tgt_player, tgt_card, SF_HEXPROOF_OVERRIDE)))
				FAILURE(0x785F94);//",abilities"
			}
		}

	  if ((required_color && !(required_color & get_color(tgt_player, tgt_card)))
		  || (illegal_color && (illegal_color & get_color(tgt_player, tgt_card))))
		FAILURE(0x739060);//",color"

	  if (special & TARGET_SPECIAL_EXTRA_FUNCTION)
		{
		  ASSERT(extra != -1 && extra != 0);

		  typedef const char* (*Predicate)(int, int, int, int, int);
		  STATIC_ASSERT(sizeof(int32_t) == sizeof(Predicate), Predicate_type_must_fit_into_an_int32_t);

		  Predicate predicate = (Predicate)extra;
		  int p = player, c = card;
		  if (p != -1 && c != -1)
			{
			  card_instance_t* instance = get_card_instance(p, c);
			  if (instance->internal_card_id == activation_card)
				{
				  p = instance->parent_controller;
				  c = instance->parent_card;
				}
			}
		  const char* err = predicate(who_chooses, tgt_player, tgt_card, p, c);
		  if (err)
			FAIL_STR(err);
		}
	  else if (extra != -1)
		{
		  if (special & TARGET_SPECIAL_CMC_LESSER_OR_EQUAL)
			{
			  if (extra >= 0 && !(get_cmc(tgt_player, tgt_card) <= extra))
				FAILURE(",converted mana cost");
			}
		  else if (special & TARGET_SPECIAL_CMC_GREATER_OR_EQUAL)
			{
			  if (extra >= 0 && !(get_cmc(tgt_player, tgt_card) >= extra))
				FAILURE(",converted mana cost");
			}
		  else if (special & TARGET_SPECIAL_NOT_LAND_SUBTYPE)
			{
			  if (has_subtype(tgt_player, tgt_card,
							  extra+1 == COLOR_BLACK ? SUBTYPE_SWAMP
							  : extra+1 == COLOR_BLUE ? SUBTYPE_ISLAND
							  : extra+1 == COLOR_GREEN ? SUBTYPE_FOREST
							  : extra+1 == COLOR_RED ? SUBTYPE_MOUNTAIN
							  : SUBTYPE_PLAINS))
				FAILURE(0x73964C);//",subtype"
			}
		  else if (special & TARGET_SPECIAL_BASIC_LAND)
			{
			  if (!sub_47EF50(tgt_instance->internal_card_id, extra))
				FAILURE(0x73964C);//",subtype"
			}
		  else if (special & TARGET_SPECIAL_REQUIRES_COUNTER)
			{
			  int counter_num = MAX(1, BYTE1(extra));
			  int counter_type = BYTE0(extra);
			  if (count_counters(tgt_player, tgt_card, counter_type) < counter_num)
				FAIL_STR("must have counters");
			}
		  else if (extra > 4)
			{
			  int match = (tgt_instance->internal_card_id == extra
						   || cards_data[tgt_instance->internal_card_id].id == cards_data[extra].id);
			  if (special & TARGET_SPECIAL_EXTRA_NOT_IID)
				{
				  if (match)
					FAILURE(0x73B57C);//",card type"
				}
			  else if (!match)
				FAILURE(0x73B57C);//",card type"
			}
		  else if (!has_subtype(tgt_player, tgt_card,
								extra+1 == COLOR_BLACK ? SUBTYPE_SWAMP
								: extra+1 == COLOR_BLUE ? SUBTYPE_ISLAND
								: extra+1 == COLOR_GREEN ? SUBTYPE_FOREST
								: extra+1 == COLOR_RED ? SUBTYPE_MOUNTAIN
								: SUBTYPE_PLAINS))
			FAILURE(0x73964C);//",subtype"
		}

	  if (required_subtype != -1 && !(special & TARGET_SPECIAL_DAMAGE_PERMANENT_WITH_SUBTYPE) )
		{
		  int is_required_subtype;
		  if (required_subtype == SUBTYPE_NONBASIC)	// Special-cased since it isn't actually set on anything
			is_required_subtype = !is_basic_land(tgt_player, tgt_card) && is_what(tgt_player, tgt_card, TYPE_LAND);
		  else if (required_subtype == SUBTYPE_LEGEND)	// Special-cased due to Leyline of Singularity
			is_required_subtype = (is_legendary(tgt_player, tgt_card)
								   || (!is_what(tgt_player, tgt_card, TYPE_LAND) && check_battlefield_for_id(2, CARD_ID_LEYLINE_OF_SINGULARITY)));
		  else
			is_required_subtype = has_subtype(tgt_player, tgt_card, required_subtype);

		  if (special & TARGET_SPECIAL_ILLEGAL_SUBTYPE)
			{
			  if (is_required_subtype)
				FAILURE(0x73964C);//",subtype"
			}
		  else if (!is_required_subtype
				   || (required_subtype == SUBTYPE_WALL
					   && cards_data[tgt_instance->internal_card_id].id == CARD_ID_WALL_OF_SHADOWS
					   && !is_humiliated(tgt_player, tgt_card)))
			FAILURE(0x73964C);//",subtype"
		}

	  if (power_requirement != -1)
		{
		  int digits = power_requirement & TARGET_PT_MASK;
		  int comp   = power_requirement & (TARGET_PT_LESSER_OR_EQUAL | TARGET_PT_GREATER_OR_EQUAL);

		  int power = tgt_instance->power;

		  if ((!comp && power != digits)
			  || (comp == TARGET_PT_GREATER_OR_EQUAL && (int16_t)power < digits)
			  || (comp == TARGET_PT_LESSER_OR_EQUAL && (int16_t)power > digits))
			FAILURE(0x618C1C);//",power"
		}

	  if (toughness_requirement != -1)
		{
		  int digits = toughness_requirement & TARGET_PT_MASK;
		  int comp   = toughness_requirement & (TARGET_PT_LESSER_OR_EQUAL | TARGET_PT_GREATER_OR_EQUAL);

		  int toughness = tgt_instance->toughness;
		  if (toughness_requirement & TARGET_PT_INCLUDE_DAMAGE)
			toughness -= tgt_instance->damage_on_card;

		  if ((!comp && toughness != digits)
			  || (comp == TARGET_PT_GREATER_OR_EQUAL && (int16_t)toughness < digits)
			  || (comp == TARGET_PT_LESSER_OR_EQUAL && (int16_t)toughness > digits))
			FAILURE(0x620600);//",toughness"
		}

	  if (special)
		{
		  if ((special & TARGET_SPECIAL_WALL)
			  && (!has_subtype(tgt_player, tgt_card, SUBTYPE_WALL)
				  || (cards_data[tgt_instance->internal_card_id].id == CARD_ID_WALL_OF_SHADOWS
					  && !is_humiliated(tgt_player, tgt_card))))
			FAILURE(0x7369AC);//",walls"

		  if ((special & TARGET_SPECIAL_NON_WALL)
			  && has_subtype(tgt_player, tgt_card, SUBTYPE_WALL))
			FAILURE(0x7369AC);//",walls"

		  if ((special & TARGET_SPECIAL_SPELL_ON_STACK)
			  && (card_on_stack_controller == -1
				  || card_on_stack_controller != tgt_player
				  || card_on_stack != tgt_card
				  || (tgt_instance->state & STATE_JUST_CAST)))
			FAILURE(0x7865F8);//",spell"

		  if ((special & TARGET_SPECIAL_BASIC_LAND)
			  && !sub_47EF10(tgt_player, tgt_card))
			FAILURE(0x7A3730);//",basic land"

		  if ((special & TARGET_SPECIAL_ARTIFACT_CREATURE)
			  && (cards_data[tgt_instance->internal_card_id].type & (TYPE_ARTIFACT|TYPE_CREATURE)) != (TYPE_ARTIFACT|TYPE_CREATURE))
			FAILURE(0x7398EC);//",artifact creature"

			if (special & TARGET_SPECIAL_DAMAGE_PLAYER ){
				if(	tgt_instance->damage_target_player != who_chooses ||
					tgt_instance->damage_target_card != -1 ||
					tgt_instance->targets[4].player > -1 || // Damaging planeswalker
					tgt_instance->targets[4].card > -1 // Damaging planeswalker
				  ){
					FAILURE(0x786C30);//",target player"
				}
			}

		  if ((special & TARGET_SPECIAL_DAMAGE_ANY_PLAYER) && tgt_instance->damage_target_card != -1)
			FAILURE(0x786C30);//",target player"

		  if ((special & TARGET_SPECIAL_DAMAGE_CREATURE) && tgt_instance->damage_target_card == -1)
			FAIL_STR("must damage creature");

		  if ((special & TARGET_SPECIAL_DAMAGE_LEGENDARY_CREATURE)
			  && (tgt_instance->damage_target_card == -1 || ! is_legendary(tgt_instance->damage_target_player, tgt_instance->damage_target_card)))
			FAIL_STR("must damage legendary creature");

		  if ((special & TARGET_SPECIAL_DAMAGE_PERMANENT_WITH_SUBTYPE)
			  && (tgt_instance->damage_target_card == -1 || ! has_subtype(tgt_instance->damage_target_player, tgt_instance->damage_target_card, required_subtype)))
			{
			  char buffer[100];
			  strcpy(buffer, get_subtype_text("must damage %a", required_subtype));
			  FAIL_STR(buffer);
			}

		  if ((special & TARGET_SPECIAL_ELEPHANT_MAMMOTH)
			  && !has_subtype(tgt_player, tgt_card, SUBTYPE_ELEPHANT))
			FAILURE(0x4EC749);//",elephant/mammoth"

		  if ((special & TARGET_SPECIAL_DJINN_OR_EFREET)
			  && !has_subtype(tgt_player, tgt_card, SUBTYPE_EFREET)
			  && !has_subtype(tgt_player, tgt_card, SUBTYPE_DJINN))
			FAILURE(0x4EC75B);//",djinn/efreet"

		  if (special & TARGET_SPECIAL_NOT_ME)
			{
			  ASSERT(player != -1 && card != -1);
			  if (player == tgt_player && card == tgt_card)
				FAIL_STR("cannot target self");
			}
		}

	  // Spells and abilities on the stack can't target themslves - 114.4
	  if (in_zone == TARGET_ZONE_ON_STACK
		  && ((card == tgt_card && player == tgt_player	// spell
			   && !(special & TARGET_SPECIAL_NOT_ME))	// avoid repeating message
			  || (tgt_instance->internal_card_id == activation_card
				  && card == tgt_instance->parent_card && player == tgt_instance->parent_controller))	//ability
		  && player != -1 && card != -1)
		FAIL_STR(",cannot target self");

	  if (required_state)
		{
		  if ((required_state & TARGET_STATE_TAPPED_OR_BLOCKING)
			  && !(tgt_instance->state & (STATE_TAPPED|STATE_BLOCKING)))
			FAILURE(0x78F10C);//",tapped"

		  if (required_state & TARGET_STATE_DESTROYED)
			{
			  if (tgt_instance->kill_code != KILL_DESTROY)
				FAILURE(0x786770);//",destroyed"

			  if ((special & TARGET_SPECIAL_REGENERATION)
				  && (tgt_instance->token_status & STATUS_CANNOT_REGENERATE))
				FAIL_STR("cannot be regenerated");
			}

		  if ((required_state & TARGET_STATE_TAPPED)
			  && !(tgt_instance->state & STATE_TAPPED))
			FAILURE(0x78F10C);//",tapped"

		  if ((required_state & TARGET_STATE_ATTACKING)
			  && !(tgt_instance->state & STATE_ATTACKING))
			FAILURE(0x715B3C);//",attacking"

		  if ((required_state & TARGET_STATE_ATTACKED)
			  && !(tgt_instance->state & STATE_ATTACKED))
			FAILURE(0x78689C);//",attacked"

		  if ((required_state & TARGET_STATE_ISBLOCKED)
			  && !(tgt_instance->state & STATE_ISBLOCKED))
			FAILURE(0x78735C);//",blocked"

		  if ((required_state & TARGET_STATE_BLOCKING)
			  && (tgt_player == current_turn
				  || !(tgt_instance->state & STATE_BLOCKING)
				  || tgt_instance->blocking == 255))
			FAILURE(0x62C044);//",blocking"

		  if (required_state & TARGET_STATE_IN_COMBAT)
			{
			  int not_blocking = 0;
			  int not_attacking = 0;

			  if (!(tgt_instance->state & STATE_ATTACKING))
				not_attacking = 1;

			  if (current_phase <= PHASE_MAIN1
				  || current_phase >= PHASE_MAIN2
				  || tgt_player == current_turn
				  || tgt_instance->blocking == 255)
				not_blocking = 1;

			  if (not_attacking && not_blocking)
				FAILURE(0x738B4C);//",attacking/blocking"
			}

		  if (required_state & TARGET_STATE_ENCHANTED)
			{
			  int p, c;
			  for (p = 0; p < 2; ++p)
				for (c = 0; c < active_cards_count[p]; ++c)
				  {
					card_instance_t* aura_inst = get_card_instance(p, c);
					if (aura_inst->internal_card_id != -1
						&& is_what(p, c, TYPE_ENCHANTMENT)
						&& aura_inst->damage_target_player == tgt_player
						&& aura_inst->damage_target_card == tgt_card)
					  goto is_enchanted;
				  }
			  //else
			  FAILURE(0x739F60);//",enchanted"
			is_enchanted:;
			}

		  if ((required_state & TARGET_STATE_JUST_CAST)
			  && !(tgt_instance->state & STATE_JUST_CAST))
			FAILURE(0x7A2D7C);//",casted"

		  if ((required_state & TARGET_STATE_SPELL_RESOLVED)
			  && (!(tgt_instance->state & STATE_JUST_CAST)
				  || (tgt_instance->state & STATE_INVISIBLE)))
			FAILURE(0x7392D4);//",cast resolved"

		  if ((required_state & TARGET_STATE_DAMAGED)
			  && !sub_47E980(tgt_player, tgt_card))
			FAILURE(0x6282CC);//",damaged"

		  if ((required_state & TARGET_STATE_COULD_UNTAP)
			  && !(tgt_instance->untap_status & 1))
			FAILURE(0x7370C8);//",can untap"

		  if ((required_state & TARGET_STATE_WILL_UNTAP)
			  && !(tgt_instance->untap_status & 2))
			FAILURE(0x73977C);//",will untap"

		  if ((required_state & TARGET_STATE_SUMMONING_SICK)
			  && !(tgt_instance->state & STATE_SUMMON_SICK))
			FAILURE(0x4EC769);//",summoning sickness
		}

	  if (illegal_state)
		{
		  if ((illegal_state & TARGET_STATE_TAPPED)
			  && (tgt_instance->state & STATE_TAPPED))
			FAILURE(0x78F10C);//",tapped"

		  if ((illegal_state & (TARGET_STATE_ATTACKING | TARGET_STATE_IN_COMBAT))
			  && (tgt_instance->state & STATE_ATTACKING))
			FAILURE(0x715B3C);//",attacking"

		  if ((illegal_state & TARGET_STATE_ATTACKED)
			  && (tgt_instance->state & STATE_ATTACKED))
			FAILURE(0x78689C);//",attacked"

		  if ((illegal_state & TARGET_STATE_ISBLOCKED)
			  && (tgt_instance->state & STATE_ISBLOCKED))
			FAILURE(0x78735C);//",blocked"

		  if ((illegal_state & (TARGET_STATE_BLOCKING | TARGET_STATE_IN_COMBAT))
			  && tgt_instance->blocking != 255 && tgt_player != current_turn)
			FAILURE(0x62C044);//",blocking"

		  if (illegal_state & TARGET_STATE_ENCHANTED)
			{
			  int p, c;
			  for (p = 0; p < 2; ++p)
				for (c = 0; c < active_cards_count[p]; ++c)
				  {
					card_instance_t* aura_inst = get_card_instance(p, c);
					if (aura_inst->internal_card_id != -1
						&& is_what(p, c, TYPE_ENCHANTMENT)
						&& aura_inst->damage_target_player == tgt_player
						&& aura_inst->damage_target_card == tgt_card)
					  FAILURE(0x739F60);//",enchanted"
				  }
			}

		  if ((illegal_state & TARGET_STATE_JUST_CAST)
			  && (tgt_instance->state & STATE_JUST_CAST))
			FAILURE(0x7A2D7C);//",casted"

		  if ((illegal_state & TARGET_STATE_SPELL_RESOLVED)
			  && (tgt_instance->state & STATE_JUST_CAST) && !(tgt_instance->state & STATE_INVISIBLE))
			FAILURE(0x7392D4);//",cast resolved"

		  if ((illegal_state & TARGET_STATE_DAMAGED)
			  && sub_47E980(tgt_player, tgt_card))
			FAILURE(0x6282CC);//",damaged"

		  if ((illegal_state & TARGET_STATE_COULD_UNTAP)
			  && (tgt_instance->untap_status & 1))
			FAILURE(0x7370C8);//",can untap"

		  if ((illegal_state & TARGET_STATE_WILL_UNTAP)
			  && (tgt_instance->untap_status & 2))
			FAILURE(0x73977C);//",will untap"

		  if ((illegal_state & TARGET_STATE_SUMMONING_SICK)
			  && (tgt_instance->state & STATE_SUMMON_SICK)
			  && (cards_data[tgt_instance->internal_card_id].type & TYPE_CREATURE))
			FAILURE(0x4EC769);//",summoning sickness"
		}
	}

 epilog:
  if (return_error_str)
	{
	  if (error_str[0])
		strcpy(return_error_str, &error_str[1]);		// skipping the initial comma
	  else
		*return_error_str = 0;
	}

  return rval;
#undef FAIL_STR
#undef FAILURE
}

int
real_validate_target(int tgt_player, int tgt_card, char* return_error_str,
					 int who_chooses, int allowed_controller, int preferred_controller, int zone,
					 int required_type, int illegal_type, int required_abilities, int illegal_abilities,
					 int required_color, int illegal_color, int extra, int required_subtype,
					 int power_requirement, int toughness_requirement, int special, int required_state,
					 int illegal_state)
{
  // Call only from the exe, which already sets preferred_controller properly.

  int player, card;
  if (illegal_abilities & KEYWORD_SHROUD)
	{
	  player = hack_gpf_player;
	  card = hack_gpf_card;
	}
  else
	player = card = -1;

  return validate_target_impl(player, card,
							  tgt_player, tgt_card, return_error_str,
							  who_chooses, allowed_controller, preferred_controller, zone,
							  required_type, illegal_type, required_abilities, illegal_abilities,
							  required_color, illegal_color, extra, required_subtype,
							  power_requirement, toughness_requirement, special, required_state,
							  illegal_state);
}

static int
target_available_impl(int player, int card,
					  int* return_count, int test_damage_target_or_source,
					  int who_chooses, int allowed_controller, int preferred_controller, int zone,
					  int required_type, int illegal_type, int required_abilities, int illegal_abilities,
					  int required_color, int illegal_color, int extra, int required_subtype,
					  int power_requirement, int toughness_requirement, int special, int required_state,
					  int illegal_state)
{
  int p;
  int count;
  int i;

  if (test_damage_target_or_source != 0 && test_damage_target_or_source != 1 && test_damage_target_or_source != 2)
	return 0;

  if (EXE_DWORD(0x60E9FC) & 1)
	who_chooses = test_damage_target_or_source;

  count = 0;

  for (p = 0; p < 2; ++p)
	if (validate_target_impl(player, card,
							 p, -1, NULL,
							 who_chooses, allowed_controller, preferred_controller, zone,
							 required_type, illegal_type, required_abilities, illegal_abilities,
							 required_color, illegal_color, extra, required_subtype,
							 power_requirement, toughness_requirement, special, required_state,
							 illegal_state))
	  {
		if (!return_count)
		  return 1;
		++count;
	  }

  if (who_chooses == HUMAN || (trace_mode & 2))
	p = (allowed_controller & 2) ? HUMAN : AI;
  else
	p = (preferred_controller & 3) ? AI : HUMAN;

  for (i = 0; i < 2; ++i, p ^= 1)
	{
	  int c, actives = MAX(active_cards_count[0], active_cards_count[1]);	// really!

	  for (c = 0; c < actives; ++c)
		{
		  int pl, cd;
		  card_instance_t* instance = get_card_instance(p, c);

		  if (instance->internal_card_id == -1)
			continue;

		  if (test_damage_target_or_source)
			{
			  if (instance->internal_card_id != damage_card)
				continue;

			  if (test_damage_target_or_source == 1)
				{
				  pl = (int)(int8_t)(instance->damage_target_player);
				  cd = instance->damage_target_card;
				}
			  else	// test_damage_target_or_source == 2
				{
				  pl = (int)(int8_t)(instance->damage_source_player);
				  cd = instance->damage_source_card;
				}
			}
		  else
			{
			  pl = p;
			  cd = c;
			}

		  if (validate_target_impl(player, card,
								   pl, cd, NULL,
								   who_chooses, allowed_controller, preferred_controller, zone,
								   required_type, illegal_type, required_abilities, illegal_abilities,
								   required_color, illegal_color, extra, required_subtype,
								   power_requirement, toughness_requirement, special, required_state,
								   illegal_state))
			{
			  if (!return_count)
				return 1;
			  ++count;
			}
		}
	}

  if (return_count)
	*return_count = count;

  return count > 0 ? 1 : 0;
}

int
real_target_available(int* return_count, int test_damage_target_or_source,
					  int who_chooses, int allowed_controller, int preferred_controller, int zone,
					  int required_type, int illegal_type, int required_abilities, int illegal_abilities,
					  int required_color, int illegal_color, int extra, int required_subtype,
					  int power_requirement, int toughness_requirement, int special, int required_state,
					  int illegal_state)
{
  int player, card;
  if (illegal_abilities & KEYWORD_SHROUD)
	{
	  player = hack_gpf_player;
	  card = hack_gpf_card;
	}
  else
	player = card = -1;

  return target_available_impl(player, card,
							   return_count, test_damage_target_or_source,
							   who_chooses, allowed_controller, preferred_controller, zone,
							   required_type, illegal_type, required_abilities, illegal_abilities,
							   required_color, illegal_color, extra, required_subtype,
							   power_requirement, toughness_requirement, special, required_state,
							   illegal_state);
}

static int
target_available_maybe_force(int player, int card,
							 int* return_count, int test_damage_target_or_source,
							 int who_chooses, int allowed_controller, int preferred_controller, int zone,
							 int required_type, int illegal_type, int required_abilities, int illegal_abilities,
							 int required_color, int illegal_color, int extra, int required_subtype,
							 int power_requirement, int toughness_requirement, int special, int required_state,
							 int illegal_state,
							 int allow_cancel)
{
  int result = target_available_impl(player, card,
									 return_count, test_damage_target_or_source,
									 who_chooses, allowed_controller, preferred_controller, zone,
									 required_type, illegal_type, required_abilities, illegal_abilities,
									 required_color, illegal_color, extra, required_subtype,
									 power_requirement, toughness_requirement, special, required_state,
									 illegal_state);

  if (!result && !(allow_cancel & 1)
	  && (who_chooses == AI || ai_is_speculating == 1)
	  && (allowed_controller & 2)
	  && !(preferred_controller & 2))
	{
	  // Perhaps better to not check result, and always add to return_count.
	  preferred_controller ^= 1;
	  result = target_available_impl(player, card,
									 return_count, test_damage_target_or_source,
									 who_chooses, allowed_controller, preferred_controller, zone,
									 required_type, illegal_type, required_abilities, illegal_abilities,
									 required_color, illegal_color, extra, required_subtype,
									 power_requirement, toughness_requirement, special, required_state,
									 illegal_state);
	}

  return result;
}

const char* hack_prepend_prompt = NULL;

static int
select_target_impl(int player, int card,
				   int who_chooses, int allowed_controller, int preferred_controller, int zone,
				   int required_type, int illegal_type, int required_abilities, int illegal_abilities,
				   int required_color, int illegal_color, int extra, int required_subtype,
				   int power_requirement, int toughness_requirement, int special, int required_state,
				   int illegal_state,
				   const char* prompt, int allow_cancel, target_t* ret_tgt)
{
  if ((who_chooses != 1 || trace_mode & 2)
	  && ai_is_speculating != 1)
	{
	  int can_target_player_0, can_target_player_1, hwnd_SpellMinimized_is_visible, rval;

	  if (zone && !(zone & TARGET_ZONE_PLAYERS))
		can_target_player_0 = can_target_player_1 = 0;
	  else if (allowed_controller & 2)
		can_target_player_0 = can_target_player_1 = 1;
	  else if (allowed_controller & 1)
		{
		  can_target_player_0 = 0;
		  can_target_player_1 = 1;
		}
	  else
		{
		  can_target_player_0 = 1;
		  can_target_player_1 = 0;
		}

	  // If can target a spell or ability on the stack, and there's at least one there besides this, don't minimize the spellchain window.
	  if ((zone & TARGET_ZONE_ON_STACK)
		  && (stack_size > 1
			  || (stack_size == 1 && !(stack_cards[0].card == card && stack_cards[0].player == player))))
		hwnd_SpellMinimized_is_visible = 1;
	  else
		hwnd_SpellMinimized_is_visible = sub_45CEC0();

	  target_t tgt = { -1, -3 };

	  if (prompt && hack_prepend_prompt && ai_is_speculating != 1)
		{
		  sprintf((char*)0x60A690, "%s: %s", hack_prepend_prompt, prompt);
		  prompt = EXE_STR(0x60A690);
		}
	  else if (!prompt)
		prompt = EXE_STR(0x60A690);	// global_all_purpose_buffer

	  while (1)
		{
		  target_t tmp_tgt;
		  int unknown_v44;

		  rval = sub_4378E0(who_chooses, -1, prompt, allow_cancel, -1, -1, -1, -1, &unknown_v44, &tmp_tgt, can_target_player_1, can_target_player_0);
		  if (rval)
			{
			  card_instance_t* instance;
			  if (tmp_tgt.card < 0
				  || (instance = get_card_instance(tmp_tgt.player, tmp_tgt.card))->internal_card_id < 0
				  || (special & TARGET_SPECIAL_ALLOW_MULTIBLOCKER)
				  || cards_at_7c7000[instance->internal_card_id]->code_pointer != 0x401010)	// multiblocker card
				{
				  char err_reason[208];

				  if (validate_target_impl(player, card,
										   tmp_tgt.player, tmp_tgt.card, err_reason,
										   who_chooses, allowed_controller, preferred_controller, zone,
										   required_type, illegal_type, required_abilities, illegal_abilities,
										   required_color, illegal_color, extra, required_subtype,
										   power_requirement, toughness_requirement, special, required_state,
										   illegal_state))
					{
					  tgt = tmp_tgt;
					  break;
					}

				  if (ai_is_speculating != 1)
					{
					  if (*err_reason)
						{
						  char illegal_tgt[200];
						  sprintf(illegal_tgt, EXE_STR(0x73940C)/*"Illegal target (%s)."*/, err_reason);
						  display_error_message(illegal_tgt);
						}
					  else
						display_error_message(EXE_STR(0x73732C)/*"Illegal target."*/);
					}
				}
			  else if (ai_is_speculating != 1)
				{
				  char illegal_tgt[200];
				  sprintf(illegal_tgt, EXE_STR(0x73940C)/*"Illegal target (%s)."*/, EXE_STR(0x728F6C)/*",type"*/);
				  display_error_message(illegal_tgt);
				}

			  continue;
			}

		  if (unknown_v44 != -2)
			continue;

		  if (EXE_DWORD(0x4EF1B0) == -1 && EXE_DWORD(0x4EF1AC) == -1)
			{
			  tgt = tmp_tgt;
			  break;
			}

		  if (zone & TARGET_ZONE_0x2000)
			break;
		}

	  *ret_tgt = tgt;
	  if (!hwnd_SpellMinimized_is_visible)
		sub_45CF00();
	  if (rval)
		EXE_DWORD(0x60A530) = 0;
	  set_centerwindow_txt("");
	  text_lines[0][0] = 0;
	  return rval;
	}
  else
	{
	  int can_target_player_0, can_target_player_1, num_candidates, p, c;
	  target_t candidates[60];

	  if ((EXE_DWORD(0x60E9FC) & 1)
		  && (allowed_controller & 2))
		preferred_controller = allowed_controller;

	  // int v30 = who_chooses; // never referenced again
	  // int v31 = (allowed_controller & 2) ? -1 : (allowed_controller & 1); // never referenced again
	  // int v32 = (preferred_controller & 2) ? -1 : (preferred_controller & 1); // never referenced again
	  // int v33 = required_type; // never referenced again
	  // int v34 = required_color; // never referenced again

	  if (zone && !(zone & TARGET_ZONE_PLAYERS))
		can_target_player_0 = can_target_player_1 = 0;
	  else if (preferred_controller & 2)
		can_target_player_0 = can_target_player_1 = 1;
	  else if (preferred_controller & 1)
		{
		  can_target_player_0 = 0;
		  can_target_player_1 = 1;
		}
	  else
		{
		  can_target_player_0 = 1;
		  can_target_player_1 = 0;
		}

	  if (cancel == 1)
		return 0;

	  num_candidates = 0;

	  for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  {
			card_instance_t* instance = get_card_instance(p, c);
			if (instance->internal_card_id != -1
				&& validate_target_impl(player, card,
										p, c, NULL,
										who_chooses, allowed_controller, preferred_controller, zone,
										required_type, illegal_type, required_abilities, illegal_abilities,
										required_color, illegal_color, extra, required_subtype,
										power_requirement, toughness_requirement, special, required_state,
										illegal_state))
			  {
				candidates[num_candidates].player = p;
				candidates[num_candidates].card = c;
				++num_candidates;
				if (num_candidates >= 58)
				  goto break2;
			  }
		  }
	break2:

	  if (can_target_player_1)
		{
		  candidates[num_candidates].player = 1;
		  candidates[num_candidates].card = -1;
		  ++num_candidates;
		}

	  if (can_target_player_0)
		{
		  candidates[num_candidates].player = 0;
		  candidates[num_candidates].card = -1;
		  ++num_candidates;
		}

	  if (num_candidates == 0)
		return 0;

	  EXE_DWORD(0x4ED954) = 3;

	  if (ai_is_speculating == 1)
		{
		  EXE_DWORD(0x7A2FE4) = internal_rand(num_candidates);
		  EXE_DWORD(0x78FA30) = (candidates[EXE_DWORD(0x7A2FE4)].player < 1 ? 0 : 0x100) | (candidates[EXE_DWORD(0x7A2FE4)].card & 0xff) | 0x4000;

		  sub_498F20();
		}
	  else
		{
		  sub_499050();

		  if (EXE_DWORD(0x7A2FE4) == 99 || num_candidates <= EXE_DWORD(0x7A2FE4))
			EXE_DWORD(0x7A2FE4) = internal_rand(num_candidates);
		}

	  EXE_DWORD(0x60A550) = candidates[EXE_DWORD(0x7A2FE4)].player;
	  *ret_tgt = candidates[EXE_DWORD(0x7A2FE4)];
	  return 1;
	}
}

static int
select_target_maybe_force(int player, int card,
						  int who_chooses, int allowed_controller, int preferred_controller, int zone,
						  int required_type, int illegal_type, int required_abilities, int illegal_abilities,
						  int required_color, int illegal_color, int extra, int required_subtype,
						  int power_requirement, int toughness_requirement, int special, int required_state,
						  int illegal_state,
						  const char* prompt, int allow_cancel, target_t* ret_tgt)
{
  int result = select_target_impl(player, card,
								  who_chooses, allowed_controller, preferred_controller, zone,
								  required_type, illegal_type, required_abilities, illegal_abilities,
								  required_color, illegal_color, extra, required_subtype,
								  power_requirement, toughness_requirement, special, required_state,
								  illegal_state,
								  prompt, allow_cancel, ret_tgt);

  if (!result && !(allow_cancel & 1)
	  && (who_chooses == AI || ai_is_speculating == 1)
	  && (allowed_controller & 2)
	  && !(preferred_controller & 2))
	{
	  preferred_controller ^= 1;
	  result = select_target_impl(player, card,
								  who_chooses, allowed_controller, preferred_controller, zone,
								  required_type, illegal_type, required_abilities, illegal_abilities,
								  required_color, illegal_color, extra, required_subtype,
								  power_requirement, toughness_requirement, special, required_state,
								  illegal_state,
								  prompt, allow_cancel, ret_tgt);
	  if (result)
		ai_modifier -= 32;
	}

  return result;
}

int
real_select_target(int who_chooses, int allowed_controller, int preferred_controller, int zone,
				   int required_type, int illegal_type, int required_abilities, int illegal_abilities,
				   int required_color, int illegal_color, int extra, int required_subtype,
				   int power_requirement, int toughness_requirement, int special, int required_state,
				   int illegal_state,
				   const char* prompt, int allow_cancel, target_t* ret_tgt)
{
  int player, card;
  if (illegal_abilities & KEYWORD_SHROUD)
	{
	  player = hack_gpf_player;
	  card = hack_gpf_card;
	}
  else
	player = card = -1;

  return select_target_maybe_force(player, card,
								   who_chooses, allowed_controller, preferred_controller, zone,
								   required_type, illegal_type, required_abilities, illegal_abilities,
								   required_color, illegal_color, extra, required_subtype,
								   power_requirement, toughness_requirement, special, required_state,
								   illegal_state,
								   prompt, allow_cancel, ret_tgt);
}

int auto_targets[500];
int auto_targets_target[500];
int read_auto_target_file = 0;

// Like default_target_definition, but doesn't set illegal_abilities.
void base_target_definition(int player, int card, target_definition_t *td, int type)
{
  td->who_chooses           = player;
  td->allowed_controller    = 2;
  td->preferred_controller  = 1 - player;
  td->zone                  = TARGET_ZONE_IN_PLAY;
  td->required_type         = type;
  td->illegal_type          = 0;
  td->required_abilities    = 0;
  td->illegal_abilities     = 0;
  td->required_color        = 0;
  td->illegal_color         = 0;
  td->extra                 = -1;
  td->required_subtype      = -1;
  td->power_requirement     = -1;
  td->toughness_requirement = -1;
  td->special               = 0;
  td->required_state        = 0;
  td->illegal_state         = 0;
  td->allow_cancel          = 1;
  td->player                = player;
  td->card                  = card;
}

void default_target_definition(int player, int card, target_definition_t* td, int type)
{
  base_target_definition(player, card, td, type);
  td->illegal_abilities = get_protections_from(player, card);
}

void counterspell_target_definition(int player, int card, target_definition_t* td, int type)
{
  base_target_definition(player, card, td, type);
  td->zone = 0;
  td->special = TARGET_SPECIAL_SPELL_ON_STACK;
}

void counter_activated_target_definition(int player, int card, target_definition_t* td, int type)
{
  base_target_definition(player, card, td, type);
  td->zone = 0;
  td->special = TARGET_SPECIAL_EFFECT_CARD;
}

int can_target(target_definition_t *td)
{
  return target_available_maybe_force(td->player, td->card,
									  NULL,
									  0,
									  td->who_chooses,
									  td->allowed_controller,
									  td->preferred_controller,
									  td->zone,
									  td->required_type,
									  td->illegal_type,
									  td->required_abilities,
									  td->illegal_abilities,
									  td->required_color,
									  td->illegal_color,
									  td->extra,
									  td->required_subtype,
									  td->power_requirement,
									  td->toughness_requirement,
									  td->special,
									  td->required_state,
									  td->illegal_state,
									  td->allow_cancel);
}

int target_available(int player, int card, target_definition_t *td)
{
  int count = 0;
  target_available_maybe_force(player, card,
							   &count,
							   0,
							   td->who_chooses,
							   td->allowed_controller,
							   td->preferred_controller,
							   td->zone,
							   td->required_type,
							   td->illegal_type,
							   td->required_abilities,
							   td->illegal_abilities,
							   td->required_color,
							   td->illegal_color,
							   td->extra,
							   td->required_subtype,
							   td->power_requirement,
							   td->toughness_requirement,
							   td->special,
							   td->required_state,
							   td->illegal_state,
							   td->allow_cancel);
  return count;
}

int choose_default_target(int player, int card, target_definition_t *td){
	card_instance_t *instance = get_card_instance(player, card);

	// if there is only 1 target, then pick it
	if( get_setting(SETTING_SMART_TARGET) ){

		// read in which cards should auto target
		if( read_auto_target_file == 0 ){
			char buffer[500];
			FILE *file = fopen("TargetsHuman.txt", "r");
			int count = 0;
			while( fscanf(file, "%[.]", buffer) == 1 ){
				fscanf(file, "%[0-9]", buffer);
				int num = atoi(buffer);
				auto_targets[count] = num;
				auto_targets_target[count] = HUMAN;
				fscanf(file, "%[^\n]", buffer);
				fscanf(file, "%[\n]", buffer);
				count++;
			}
			fclose(file);
			file = fopen("TargetsAI.txt", "r");
			while( fscanf(file, "%[.]", buffer) == 1 ){
				fscanf(file, "%[0-9]", buffer);
				int num = atoi(buffer);
				auto_targets[count] = num;
				auto_targets_target[count] = AI;
				fscanf(file, "%[^\n]", buffer);
				fscanf(file, "%[\n]", buffer);
				count++;
			}
			fclose(file);
			auto_targets[count] = -1;
			read_auto_target_file = 1;
		}

		// see if the card being used has a default target
		int id = get_id(player, card);

		int i;
		int original_allowed_controller = td->allowed_controller;
		int original_preferred_controller = td->preferred_controller;

		for(i=0;i<500;i++){
			if( auto_targets[i] >= 0 && auto_targets[i] == id ){
				td->allowed_controller = auto_targets_target[i];
				td->preferred_controller = auto_targets_target[i];
				break;
			}
		}

		if( target_available(player, card, td) == 1 ){
			int p;
			for(p=0;p<2;p++){
				for(i=-1;i<active_cards_count[p];i++){
					instance->targets[0].player = p;
					instance->targets[0].card = i;
					instance->number_of_targets = 0;
					if( would_validate_target(player, card, td, 0 )){
						return 1;
					}
				}
			}
		}
		else{
			td->allowed_controller = original_allowed_controller;
			td->preferred_controller = original_preferred_controller;
		}
	}
	return 0;
}

// Selects a target into (td->player, td->card)->targets[0].  Loads prompt from Tetx.res.  Sets spell_fizzled if cancelled.
int pick_target(target_definition_t *td, const char *prompt ){
	load_text(0, prompt);
	int result = select_target(td->player, td->card, td, text_lines[0], NULL);
	if( ! result ){
		spell_fizzled = 1;
	}
	return result;
}

/* Selects a target into (td->player, td->card)->targets[ret_location], or into the next unused slot (counting by instance->number_of_targets) if
	ret_location is -1. */
int new_pick_target(target_definition_t *td, const char *prompt, int ret_location, int mode ){
	// Mode:	1<<0 --> Set "spell_fizzled" to 1 if no target is chosen
	//			1<<7 (GS_LITERAL_PROMPT) --> Allow the use of a custom prompt text

	card_instance_t *instance = get_card_instance(td->player, td->card);
	if( ret_location == -1 ){
		ret_location = instance->number_of_targets;
	}
	if( !(mode & GS_LITERAL_PROMPT) ){
		load_text(0, prompt);
		prompt = text_lines[0];
	}
	int result = select_target(td->player, td->card, td, prompt, &(instance->targets[ret_location]));
	if( ! result && (mode & 1) ){
		spell_fizzled = 1;
	}
	return result;
}

/* Selects a target into next (td->player, td->card)'s unused target slot (counting by instance->number_of_targets).  prompt is used literally, not loaded from Text.res.  Sets
 * spell_fizzled if cancelled. */
int pick_next_target_noload(target_definition_t *td, const char *prompt){
	card_instance_t* instance = get_card_instance(td->player, td->card);
	int result = select_target(td->player, td->card, td, prompt, &instance->targets[instance->number_of_targets]);
	if (!result){
		spell_fizzled = 1;
	}
	return result;
}

// Just like pick_up_to_n_targets(), but uses prompt literally instead of loading from Text.res.
int pick_up_to_n_targets_noload(target_definition_t* td, const char* prompt, int num)
{
  if (num <= 0)
	return 0;

  card_instance_t* instance = get_card_instance(td->player, td->card);
  instance->number_of_targets = 0;

  int len = strlen(prompt) + 50;
  char prompt_w_count[len];
  prompt_w_count[0] = 0;

  char fmt[100];
  if (IS_AI(td->who_chooses))
	strcpy(fmt, "%s %d %d");
  else
	{
	  load_text(0, "TARGET_COUNT");
	  strcpy(fmt, (td->allow_cancel & 2)
			 ? text_lines[1]	// "%s (%d of up to %d)"
			 : text_lines[0]);	// "%s (%d of %d)"
	}

  int already_chose_player[2] = {0};
  target_t tgt;

  int i, picked_cancel = 0;
  for (i = 0; i < num && can_target(td); ++i)
	{
	  if (!IS_AI(td->who_chooses))
		scnprintf(prompt_w_count, len, fmt, prompt, i + 1, num);

	  if (select_target(td->player, td->card - 1000, td, prompt_w_count, &tgt))
		{
		  if (tgt.card != -1)
			get_card_instance(tgt.player, tgt.card)->state |= STATE_TARGETTED | STATE_CANNOT_TARGET;
		  else if (!already_chose_player[tgt.player])
			already_chose_player[tgt.player] = 1;
		  else
			{
			  if (!IS_AI(td->who_chooses))	// handles both speculation and AI
				{
				  load_text(0, "FIREBALL");
				  display_error_message(text_lines[1]);	// "Illegal target (may only target once)."
				}
			  --i;
			  continue;
			}

		  instance->targets[i] = tgt;	// struct copy
		  ++instance->number_of_targets;
		}
	  else
		{
		  picked_cancel = tgt.card == -1;
		  break;
		}
	}

  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[i].card != -1)
	  get_card_instance(instance->targets[i].player, instance->targets[i].card)->state &= ~(STATE_TARGETTED | STATE_CANNOT_TARGET);

  if (picked_cancel)
	{
	  instance->number_of_targets = 0;
	  cancel = 1;
	}

  return instance->number_of_targets;
}

/* Selects up to num targets starting from (td->player, td->card)->targets[0].  Returns actual number chosen (also in number_of_targets).  Set allow_cancel to 3
 * to show both Done and Cancel buttons.  Sets spell_fizzled only if the Cancel button is clicked, not if the Done button is. */
int pick_up_to_n_targets(target_definition_t* td, const char* prompt, int num)
{
  char loaded[300];
  if (ai_is_speculating == 1)
	loaded[0] = 0;
  else
	{
	  load_text(0, prompt);
	  scnprintf(loaded, 300, "%s", text_lines[0]);
	}
  return pick_up_to_n_targets_noload(td, loaded, num);
}

/* Just like pick_up_to_n_targets_noload(), but doesn't change (td->player,td->card)'s number_of_targets or targets array, instead marking targets in
 * marked[][].  If the Cancel button is pushed, returns 0 and sets cancel to 1, but leaves previously-chosen targets set in marked.  marked should be a char
 * array[2][151] initialized to 0.  Set num to -1 to allow any number of targets.  This should only be used for non-targetted things resolving immediately after
 * chosen, since they won't track change of control or be seen by when-this-becomes-targeted triggers. */
int mark_up_to_n_targets_noload(target_definition_t* td, const char* prompt, int num, char (*marked)[151])
{
  if (num == 0)
	return 0;
  if (num < 0)
	num = 500;

  target_t tgt;

  int p, c, i, picked_cancel = 0, num_tgts = 0;
  for (i = 0; i < num && can_target(td); ++i)
	if (select_target(td->player, td->card - 1000, td, prompt, &tgt))
	  {
		++num_tgts;
		marked[tgt.player][tgt.card] = 1;
		get_card_instance(tgt.player, tgt.card)->state |= STATE_TARGETTED | STATE_CANNOT_TARGET;
	  }
	else
	  {
		picked_cancel = tgt.card == -1;
		break;
	  }

  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if (marked[p][c])
		get_card_instance(p, c)->state &= ~(STATE_TARGETTED | STATE_CANNOT_TARGET);

  if (picked_cancel)
	{
	  num_tgts = 0;
	  cancel = 1;
	}

  return num_tgts;
}

/* Selects a target into next (player, card)'s unused target slot (counting by instance->number_of_targets), while using (td->player, td->card) as the
 * targeting source.  Sets spell_fizzled if cancelled. */
int pick_next_target_arbitrary(target_definition_t *td, const char *prompt, int player, int card)
{
  load_text(0, prompt);
  return pick_next_target_noload_arbitrary(td, text_lines[0], player, card);
}

/* Selects a target into next (player, card)'s unused target slot (counting by instance->number_of_targets), while using (td->player, td->card) as the targeting
 * source.  prompt is used literally, not loaded from Text.res.  Sets spell_fizzled if cancelled. */
int pick_next_target_noload_arbitrary(target_definition_t *td, const char *prompt, int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);

  int result = select_target_maybe_force(td->player, td->card,
										 td->who_chooses, td->allowed_controller, td->preferred_controller, td->zone,
										 td->required_type, td->illegal_type, td->required_abilities, td->illegal_abilities,
										 td->required_color, td->illegal_color, td->extra, td->required_subtype,
										 td->power_requirement, td->toughness_requirement, td->special, td->required_state,
										 td->illegal_state,
										 prompt, td->allow_cancel, &instance->targets[instance->number_of_targets]);

  if (result)
	++instance->number_of_targets;
  else
	spell_fizzled = 1;

  return result;
}

/* Selects a target into ret_location, or (player, card)->targets[0] if NULL.  Targeting source is player/card instead of td->player/td->card.  prompt is used
 * literally, not loaded from Text.res.  Never sets spell_fizzled. */
int select_target(int player, int card, target_definition_t *td, const char *prompt, target_t *ret_location){
	card_instance_t* instance;
	if (card < -500){	// Tired of this mangling number_of_targets.
		instance = NULL;
		card += 1000;
		ASSERT(ret_location);
	} else {
		instance = get_card_instance(player, card);
		if (ret_location == NULL){
			ret_location = &(instance->targets[0]);
		}

		if (choose_default_target(player, card, td)){
			return 1;
		}
	}

	int result = select_target_maybe_force(player, card,
										   td->who_chooses, td->allowed_controller, td->preferred_controller, td->zone,
										   td->required_type, td->illegal_type, td->required_abilities, td->illegal_abilities,
										   td->required_color, td->illegal_color, td->extra, td->required_subtype,
										   td->power_requirement, td->toughness_requirement, td->special, td->required_state,
										   td->illegal_state,
										   prompt, td->allow_cancel, ret_location);
	if (result){
		if (instance){
			instance->number_of_targets++;
		}
		return 1;
	}

	return 0;
}

/* If duh mode is on, pick preferred_controller (which is opponent by default) if he's a valid target, or else cancel if that's allowed.
 * Otherwise, forwards to pick_target(td, "TARGET_PLAYER"). */
int pick_player_duh(int player, int card, int preferred_controller, int allow_cancel)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = preferred_controller;
  td.allow_cancel = allow_cancel;

  if (duh_mode(player))
	{
	  instance->targets[0].player = preferred_controller;
	  instance->targets[0].card = -1;
	  instance->number_of_targets = 1;
	  if (would_valid_target(&td))
		return 1;
	  else if (allow_cancel & 1)
		{
		  cancel = 1;
		  return 0;
		}
	}

  instance->number_of_targets = 0;
  return can_target(&td) && pick_target(&td, "TARGET_PLAYER");
}

// (player/card) targets 1-player in its targets[0] and returns nonzero, else returns 0.
int target_opponent(int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = 1-player;
  td.allow_cancel = 0;

  instance->targets[0].player = 1-player;
  instance->targets[0].card = -1;
  instance->number_of_targets = 1;

  if (would_valid_target(&td))
	return 1;
  else
	{
	  instance->targets[0].player = -1;
	  instance->number_of_targets = 0;
	  cancel = 1;
	  return 0;
	}
}

/* Returns nonzero if (player/card) can target 1-player, else 0.
 * The difference from target_opponent() is that it doesn't assign the target, increase number_of_targets, or set cancel. */
int opponent_is_valid_target(int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = 1-player;
  td.allow_cancel = 0;

  target_t old_t0 = instance->targets[0];	// struct copy
  int old_num_tgt = instance->number_of_targets;

  instance->targets[0].player = 1-player;
  instance->targets[0].card = -1;
  instance->number_of_targets = 1;

  int rval = would_valid_target(&td);

  instance->targets[0] = old_t0;	// struct copy
  instance->number_of_targets = old_num_tgt;

  return rval;
}

int valid_target(target_definition_t *td ){
	return validate_target(td->player, td->card, td, 0);
}
int would_valid_target(target_definition_t *td ){
	return would_validate_target(td->player, td->card, td, 0);
}

int validate_arbitrary_target(target_definition_t* td, int tgt_player, int tgt_card){
	int32_t old_preferred_controller = td->preferred_controller;
	td->preferred_controller = td->allowed_controller;
	int rval = would_validate_arbitrary_target(td, tgt_player, tgt_card);
	td->preferred_controller = old_preferred_controller;
	return rval;
}
int would_validate_arbitrary_target(target_definition_t* td, int tgt_player, int tgt_card){
	target_controller = tgt_player;
	target_card = tgt_card;

	return validate_target_impl(td->player, td->card,
								target_controller, target_card, NULL,
								td->who_chooses, td->allowed_controller, td->preferred_controller, td->zone,
								td->required_type, td->illegal_type, td->required_abilities, td->illegal_abilities,
								td->required_color, td->illegal_color, td->extra, td->required_subtype,
								td->power_requirement, td->toughness_requirement, td->special, td->required_state,
								td->illegal_state);
}

int validate_target(int player, int card, target_definition_t *td, int target_number){
	int32_t old_preferred_controller = td->preferred_controller;
	td->preferred_controller = td->allowed_controller;
	int rval = would_validate_target(player, card, td, target_number);
	td->preferred_controller = old_preferred_controller;
	return rval;
}
int would_validate_target(int player, int card, target_definition_t *td, int target_number){

	card_instance_t *instance = get_card_instance(player, card);

	if(target_number > instance->number_of_targets - 1){
		target_number = 0;
	}

	target_controller = instance->targets[target_number].player;
	target_card = instance->targets[target_number].card;

	int result = validate_target_impl(player, card,
									target_controller,
									target_card,
									NULL,
									td->who_chooses,
									td->allowed_controller,
									td->preferred_controller,
									td->zone,
									td->required_type,
									td->illegal_type,
									td->required_abilities,
									td->illegal_abilities,
									td->required_color,
									td->illegal_color,
									td->extra,
									td->required_subtype,
									td->power_requirement,
									td->toughness_requirement,
									td->special,
									td->required_state,
									td->illegal_state
									);

	return result;
}

int autoselect_target( int player, int card, int i, int j, int k, int who_chooses,
	int allowed_controller,
	int preferred_controller,
	int zone,
	int required_type,
	int illegal_type,
	int required_abilities,
	int illegal_abilities,
	int required_color,
	int illegal_color,
	int extra,
	int required_subtype,
	int power_requirement,
	int toughness_requirement,
	int special,
	int required_state,
	int illegal_state,
	int prompt,
	int allow_cancel,
	int ret_location){

	if( who_chooses == 1 ){
		return 0;
	}


	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.allowed_controller = allowed_controller;
	td.preferred_controller = preferred_controller;
	td.zone = zone;
	td.required_type = required_type;
	td.illegal_type = illegal_type;
	td.required_color = required_color;
	td.illegal_color = illegal_color;
	td.extra = extra;
	td.required_subtype = required_subtype;
	td.power_requirement = power_requirement;
	td.toughness_requirement = toughness_requirement;
	td.special = special;
	td.required_state = required_state;
	td.illegal_state = illegal_state;
	td.allow_cancel = allow_cancel;

	if( choose_default_target(player, card, &td) ){
		//card_instance_t *instance = get_card_instance(player, card);
		//gain_life(HUMAN, instance->targets[0].player+1);
		//poison_counters[HUMAN]= 2 + instance->targets[0].card;
		return 1;
	}
	else{
		//spell_fizzled = 1;
		return 0;
	}


	return 0;
	char buffer[1500];
	snprintf(buffer, 1500, "player %d\ncard %d\ni %d\nj %d\nk %d\nwho_chooses %d\n allowed_controller %d\n preferred_controller, %d\n zone, %d\n required_type,%d\n illegal_type, %d\n required_abilities, %d\n illegal_abilities, %d\n allow_cancel, %d\n ret_location %d\n",
	player, card,
	i,
	j,
	k,
	who_chooses,
	allowed_controller,
	preferred_controller,
	zone,
	required_type,
	illegal_type,
	required_abilities,
	illegal_abilities,
	allow_cancel,
	ret_location
	 );
	do_dialog(0, 0, 0, -1, -1, buffer, 0);
	return 0;
}

int is_protected_from_me(int player, int card, int t_player, int t_card){

	card_instance_t *instance = get_card_instance(player, card);
	int clr = instance->card_color;
	if( get_id(t_player, t_card) == CARD_ID_GAEAS_REVENGE && (clr & COLOR_TEST_GREEN) ){
		return 1;
	}

	if( get_id(t_player, t_card) == CARD_ID_SUQATA_FIREWALKER && (clr & COLOR_TEST_RED) ){
		return 1;
	}

	if( get_id(t_player, t_card) == CARD_ID_AZORIUS_FIRST_WING && is_what(player, card, TYPE_ENCHANTMENT) ){
		return 1;
	}

	if( get_id(t_player, t_card) == CARD_ID_ENEMY_OF_THE_GUILDPACT && count_colors(player, card) > 1 ){
		return 1;
	}

	if( get_id(t_player, t_card) == CARD_ID_GUARDIAN_OF_THE_GUILDPACT && count_colors(player, card) == 1 ){
		return 1;
	}

	if( get_id(t_player, t_card) == CARD_ID_EMRAKUL_THE_AEONS_TORN && clr != COLOR_TEST_COLORLESS && is_what(player, card, TYPE_SPELL) &&
		! is_what(player, card, TYPE_PERMANENT)
	  ){
		return 1;
	}
	/*
	if( is_what(t_player, t_card, TYPE_CREATURE) && check_special_flags2(t_player, t_card, SF2_DENSE_FOLIAGE) && is_what(player, card, TYPE_SPELL) &&
		! is_what(player, card, TYPE_PERMANENT)
	  ){
		return 1;
	}
	*/
	if( player != t_player && get_id(t_player, t_card) == CARD_ID_FIENDSLAYER_PALADIN && ((clr & COLOR_TEST_BLACK) || (clr & COLOR_TEST_RED)) ){
		return 1;
	}
	return 0;
}

void give_shroud_to_player(int player, int card, event_t event){
	player_bits[player] |= PB_PLAYER_HAS_SHROUD;
}

void give_hexproof_to_player(int player, int card, event_t event){
	player_bits[player] |= PB_PLAYER_HAS_HEXPROOF;
}

int target_me(int t_player, int t_card, int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);
  int i;
  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[i].player == t_player && instance->targets[i].card == t_card)
	  return 1;

  return 0;
}

/* When {t_player,t_card} becomes targeted by a spell controlled by controlled_by, returns the spell (else NULL).
 * {player,card} is solely the card that highlights for the trigger.  Doesn't check is_humiliated() or in_play(). */
const target_t* becomes_target_of_spell(int player, int card, event_t event, int t_player, int t_card, int controlled_by, resolve_trigger_t trigger_mode)
{
  static target_t rval;
  if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller
	  && (controlled_by == ANYBODY || trigger_cause_controller == controlled_by))
	{
	  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
		return NULL;

	  if (target_me(t_player, t_card, trigger_cause_controller, trigger_cause) && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND | TYPE_EFFECT))
		{
		  if (event == EVENT_TRIGGER)
			{
			  if (trigger_mode == RESOLVE_TRIGGER_DUH)
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

			  event_result |= trigger_mode;
			  return NULL;
			}
		  else // event == EVENT_RESOLVE_TRIGGER;
			{
			  rval.player = trigger_cause_controller;
			  rval.card = trigger_cause;
			  return &rval;
			}
		}
	}

  return NULL;
}

/* When {t_player,t_card} becomes targeted by a spell or ability controlled by controlled_by, returns the spell or effect.
 * {player,card} is solely the card that highlights for the trigger.  Doesn't check is_humiliated() or in_play(). */
const target_t* becomes_target_of_spell_or_effect(int player, int card, event_t event, int t_player, int t_card, int controlled_by)
{
  const target_t* btos;
  if ((btos = becomes_target_of_spell(player, card, event, t_player, t_card, controlled_by, RESOLVE_TRIGGER_MANDATORY)))
	return btos;

  static target_t rval;
  if ((event == EVENT_PLAY_ABILITY || event == EVENT_TAPPED_TO_PLAY_ABILITY || event == EVENT_TRIGGER_RESOLVED)
	  && (controlled_by == ANYBODY || affected_card_controller == controlled_by)
	  && target_me(t_player, t_card, affected_card_controller, affected_card)
	  && get_card_instance(affected_card_controller, affected_card)->info_slot != (int)legacy_effect_pump_ability_until_eot
	  )
	{
	  rval.player = affected_card_controller;
	  rval.card = affected_card;
	  return &rval;
	}

  return NULL;
}

static int is_targeting_type(int player, int card, int controller_of_target, int or_player, type_t targeted_type, int subtype)
{
  card_instance_t* instance = get_card_instance(player, card);
  int i;
  for (i = 0; i < instance->number_of_targets; ++i)
	if ((controller_of_target != ANYBODY ? instance->targets[i].player == controller_of_target
		 : (instance->targets[i].player == 0 || instance->targets[i].player == 1))
		&& (instance->targets[i].card == -1 ? or_player
			: (in_play(instance->targets[i].player, instance->targets[i].card)
			   && is_what(instance->targets[i].player, instance->targets[i].card, targeted_type)
			   && (subtype == -1 || has_subtype(instance->targets[i].player, instance->targets[i].card, subtype)))))
	  return 1;

  return 0;
}

// Populates arr with array of unique creatures controlled by controller_of_target targeted by (player,card), then returns it.
static target_t* fill_arr(int player, int card, int controller_of_target, int or_player, type_t targeted_type, int subtype, target_t* arr)
{
  card_instance_t* instance = get_card_instance(player, card);
  ASSERT(instance->number_of_targets < 20);
  int i, j, last = -1;
  for (i = 0; i < instance->number_of_targets; ++i)
	if ((controller_of_target != ANYBODY ? instance->targets[i].player == controller_of_target
		 : (instance->targets[i].player == 0 || instance->targets[i].player == 1))
		&& (instance->targets[i].card == -1 ? or_player
			: (in_play(instance->targets[i].player, instance->targets[i].card)
			   && is_what(instance->targets[i].player, instance->targets[i].card, targeted_type)
			   && (subtype == -1 || has_subtype(instance->targets[i].player, instance->targets[i].card, subtype)))))
	  {
		for (j = 0; j < i; ++j)
		  if (instance->targets[i].player == instance->targets[j].player
			  && instance->targets[i].card == instance->targets[j].card)
			goto outer_continue;	// only triggers once per creature even if it's targeted more than once by the same spell/effect, per Wild Defiance ruling

		arr[++last] = instance->targets[i];	// struct copy

	  outer_continue:;
	  }

  arr[++last].player = -1;
  arr[last].card = -1;

  return arr;
}

/* The general case.  Returns array of unique targeted_types (and players, if or_player is nonzero) controlled by controller_of_target currently being targeted
 * by a targeting_type (TYPE_EFFECT: an effect; other types: a spell of that type being cast) controlled by controller_of_trigger, ending in a {-1,-1} entry; or
 * NULL if none. */
const target_t* any_becomes_target(int player, int card, event_t event, int controller_of_target, int or_player, type_t targeted_type, int subtype, type_t targeting_type, int controller_of_trigger, resolve_trigger_t resolve_trigger)
{
  static target_t arr[20];

  if (!in_play(player, card) || is_humiliated(player, card))
	return NULL;

  if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  if (!(event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER)
		  || !(targeting_type & TYPE_NONEFFECT)
		  || (controller_of_trigger != ANYBODY && trigger_cause_controller != controller_of_trigger))
		return NULL;

	  if (is_what(trigger_cause_controller, trigger_cause, targeting_type & TYPE_NONEFFECT)
		  && is_targeting_type(trigger_cause_controller, trigger_cause, controller_of_target, or_player, targeted_type, subtype))
		{
		  if (event == EVENT_TRIGGER)
			{
			  if (resolve_trigger == RESOLVE_TRIGGER_DUH)
				resolve_trigger = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

			  event_result |= resolve_trigger;
			  return NULL;
			}
		  else // event == EVENT_RESOLVE_TRIGGER;
			return fill_arr(trigger_cause_controller, trigger_cause, controller_of_target, or_player, targeted_type, subtype, arr);
		}
	}

  if (event == EVENT_PLAY_ABILITY || event == EVENT_TAPPED_TO_PLAY_ABILITY || event == EVENT_TRIGGER_RESOLVED)
	{
	  if ((targeting_type & TYPE_EFFECT)
		  && (controller_of_trigger == ANYBODY || controller_of_trigger == affected_card_controller)
		  && is_targeting_type(affected_card_controller, affected_card, controller_of_target, or_player, targeted_type, subtype))
		return fill_arr(affected_card_controller, affected_card, controller_of_target, or_player, targeted_type, subtype, arr);
	}

  return NULL;
}

/* A common case of any_becomes_target().  Returns array of unique creatures controlled by controller_of_target currently being targeted, ending in a {-1,-1}
 * entry; or NULL if none. */
const target_t* any_creature_becomes_target_of_spell_or_effect(int player, int card, event_t event, int controller_of_target)
{
  return any_becomes_target(player, card, event, controller_of_target, 0, TYPE_CREATURE, -1, TYPE_EFFECT|TYPE_NONEFFECT, ANYBODY, RESOLVE_TRIGGER_MANDATORY);
}

// "(Creature this effect is attached to) gains 'When this creature becomes the target of a spell or ability, sacrifice it.'"
int attached_creature_gains_sacrifice_when_becomes_target(int player, int card, event_t event)
{
  card_instance_t* instance;
  if ((instance = in_play(player, card))
	  && instance->damage_target_card >= 0
	  && in_play(instance->damage_target_player, instance->damage_target_card)
	  && !is_humiliated(instance->damage_target_player, instance->damage_target_card)
	  && becomes_target_of_spell_or_effect(player, card, event, instance->damage_target_player, instance->damage_target_card, ANYBODY))
	{
	  kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
	  return 1;
	}
  return 0;
}

// "When enchanted creature becomes the target of a spell or ability, sacrifice/destroy/bury/exile (this aura)."
int kill_attachment_when_creature_is_targeted(int player, int card, event_t event, kill_t kill_code)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (in_play(player, card)
	  && instance->damage_target_card >= 0
	  && in_play(instance->damage_target_player, instance->damage_target_card)
	  && !is_humiliated(player, card)
	  && becomes_target_of_spell_or_effect(player, card, event, instance->damage_target_player, instance->damage_target_card, ANYBODY))
	{
	  kill_card(player, card, kill_code);
	  return 1;
	}
  return 0;
}

// "When enchanted creature becomes the target of a spell, sacrifice/destroy/bury/exile (this aura)."
int kill_attachment_when_creature_is_targeted_by_spell(int player, int card, event_t event, kill_t kill_code)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (in_play(player, card)
	  && instance->damage_target_card >= 0
	  && in_play(instance->damage_target_player, instance->damage_target_card)
	  && !is_humiliated(player, card)
	  && becomes_target_of_spell(player, card, event, instance->damage_target_player, instance->damage_target_card, ANYBODY, RESOLVE_TRIGGER_MANDATORY))
	{
	  kill_card(player, card, kill_code);
	  return 1;
	}
  return 0;
}
