// -*- c-basic-offset:2 -*-
#include <windows.h>

#include "manalink.h"

void UNUSED1(void){}
void UNUSED2(void){}
void UNUSED3(void){}

static const int cardclass_offset_player = 0;
static const int cardclass_offset_card = 4;
static const int cardclass_offset_instance_copy = 12;
static const int territoryclass_offset_card_hwnds = 0;
static const int territoryclass_offset_card_hwnd_length = 4;

#define critical_section_for_display		EXE_TYP_PTR(CRITICAL_SECTION, 0x620288)
#define critical_section_for_drawing		EXE_TYP_PTR(CRITICAL_SECTION, 0x56b950)
#define hdc_4EB7AC							EXE_TYP(HDC, 0x4eb7ac)
#define hwnd_FullCardClass					EXE_TYP(HWND, 0x715ca0)
#define hwnd_TerritoryClass_opponent		EXE_TYP(HWND, 0x739c1c)
#define hwnd_TerritoryClass_player			EXE_TYP(HWND, 0x73680c)
#define option_Layout						EXE_DWORD(0x787260)
#define option_ShowAbilitiesOnCards			EXE_DWORD(0x787244)
#define option_ShowAllCardsSummonSickness	EXE_DWORD(0x787258)
#define option_ShowPowerToughnessOnCards	EXE_DWORD(0x787240)
#define path_CardArt						EXE_STR(0x60eda0)
#define pen_autotap_blue					EXE_TYP(HPEN, 0x608d40)

#define str_Activation						EXE_BYTE_PTR(0x62bd98)
#define str_Damage							EXE_STR(0x73b538)
#define str_MultiBlock_Creature				EXE_STR(0x739be4)
#define str_Upkeep							EXE_BYTE_PTR(0x7375c4)
#define strs_Colorless_Black_Blue_Green_Red_White	((char(*)[20])0x608e10)

// from Drawcardlib.dll
#define DLLIMPORT __attribute__((dllimport))
DLLIMPORT int DrawSmallCard(HDC hdc, const RECT* card_rect, const card_ptr_t* cp, int version, int mode, int player, int card);
DLLIMPORT int DrawSmallCardTitle(HDC hdc, const RECT* dest_rect, const char* card_title, int color, int controlled_by_owner, const card_ptr_t* cp, int player, int card);
DLLIMPORT int DrawCardBack(HDC hdc, const RECT* dest_rect, int mode);
DLLIMPORT void draw_special_counters(HDC hdc, const RECT* rect, int player, int card);
DLLIMPORT void draw_standard_counters(HDC hdc, const RECT* rect, int player, int card);

#define blt_dying_pic						EXE_FN(void, 0x4d49b0, HDC hdc, RECT* rect)
#define blt_summon_pic						EXE_FN(void, 0x4d4820, HDC hdc, RECT* rect)
#define draw_damage							EXE_FN(void, 0x4d3630, HDC hdc, RECT* rect, int amt)
#define draw_manastripes					EXE_FN(int, 0x4d4600, HDC hdc, RECT* rect, int player, int card)
#define draw_oublietted_effect				EXE_FN(int, 0x4d48b0, HDC hdc, RECT* rect)
#define draw_powertoughness_on_smallcard	EXE_FN(void, 0x4d3320, HDC hdc, RECT* card_rect, int pow, int tgh)
#define get_displayed_abilities				EXE_FN(int, 0x438d70, unsigned int player, unsigned int card)
#define get_displayed_csvid					EXE_FN(int, 0x438b00, int player, int card)
#define get_displayed_damage_on_card		EXE_FN(int, 0x438aa0, unsigned int player, unsigned int card)
#define get_displayed_kill_code				EXE_FN(int, 0x4391c0, unsigned int player, unsigned int card)
#define get_displayed_owned_by_opponent		EXE_FN(int, 0x438ec0, int player, int card)
#define get_displayed_power					EXE_FN(int, 0x438d10, unsigned int player, unsigned int card)
#define get_displayed_state					EXE_FN(state_t, 0x438e20, unsigned int player, unsigned int card)
#define get_displayed_toughness				EXE_FN(int, 0x438d40, unsigned int player, unsigned int card)
#define get_displayed_type					EXE_FN(int, 0x438ce0, int player, int card)
#define get_displayed_unknown0x70			EXE_FN(uint32_t, 0x438f60, unsigned int player, unsigned int card)
#define memcpy_displayed_card_instance_t	EXE_FN(void, 0x439300, card_instance_t* dst, int player, int card)
#define read_graphics_file					EXE_FN(HBITMAP, 0x459fa0, const char* path)
#define read_graphics_file2					EXE_FN(HBITMAP, 0x466720, const char* path)
#define return_displayed_bitfield_of_1summonsick_2tapped_4attacking_8blocking_10damage_target_player_and_damage_target_card_20oublietted	EXE_FN(int, 0x438b80, int player, int card)
#define set_palette_and_stretch_mode		EXE_FN(void, 0x466050, HDC hdc)

// Not in mingw includes.  Perplexing.
WINGDIAPI BOOL WINAPI AlphaBlend(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);

static void stretchblt_from_bmp(HDC hdc, RECT* dest, HBITMAP src_bmp, int src_x, int src_y, int src_wid, int src_hgt)
{
  if (!(hdc && dest && src_bmp))
    return;

  EnterCriticalSection(critical_section_for_drawing);
  int saved_dc = SaveDC(hdc);
  SelectObject(hdc_4EB7AC, src_bmp);
  int dest_x = dest->left;
  int dest_y = dest->top;
  int dest_wid = dest->right < dest->left ? src_wid : dest->right - dest->left;
  int dest_hgt = dest->bottom < dest->top ? src_hgt : dest->bottom - dest->top;

  BLENDFUNCTION blend;
  blend.BlendOp = AC_SRC_OVER;
  blend.BlendFlags = 0;
  blend.SourceConstantAlpha = 255;
  blend.AlphaFormat = AC_SRC_ALPHA;

  AlphaBlend(hdc, dest_x, dest_y, dest_wid, dest_hgt, hdc_4EB7AC, src_x, src_y, src_wid, src_hgt, blend);

  // Shouldn't we reselect the old object first?

  RestoreDC(hdc, saved_dc);

  LeaveCriticalSection(critical_section_for_drawing);
}

static void stretchblt_bmp(HDC hdc, RECT* dest, HBITMAP src_bmp)
{
  if (!src_bmp)
    return;

  BITMAP bmp;
  GetObject(src_bmp, sizeof(BITMAP), &bmp);
  stretchblt_from_bmp(hdc, dest, src_bmp, 0, 0, bmp.bmWidth, bmp.bmHeight);
}

static void premultiply_alpha_bitmap(BITMAP* bmp)
{
  unsigned char* bits = (unsigned char*)bmp->bmBits;
  int sz = bmp->bmWidth * bmp->bmHeight;
  int px;
  for (px = 0; px < sz; ++px)
    {
      bits[0] = bits[0] * bits[3] / 256;
      bits[1] = bits[1] * bits[3] / 256;
      bits[2] = bits[2] * bits[3] / 256;
      bits += 4;
    }
}

static void premultiply_alpha_hbitmap(HBITMAP hbmp)
{
  BITMAP bmp;
  GetObject(hbmp, sizeof(BITMAP), &bmp);
  premultiply_alpha_bitmap(&bmp);
}

STATIC_ASSERT(sizeof(uint64_t) == sizeof(unsigned long long), unsigned_long_long_is_exactly_64_bits);

static unsigned long long get_icons(int player, int card, int is_damage_card)
{
  if (player < 0 || player > 1 || card < 0 || card >= 150)
	return 0;

  if (get_setting(SETTING_ABILITY_ICON_MAX) < 0)
	return -1ULL;

  unsigned long long rval = 0;

  keyword_t kw = 0;
  int spkw = 0;
  int not_valid_card = 0;
  type_t typ;
  {
    card_instance_t* instance = get_displayed_card_instance(player, card);

    EnterCriticalSection(critical_section_for_display);

    iid_t iid = instance->internal_card_id;

	/* This check is the equivalent of !in_play(player, card), using instance instead of calling get_card_instance()
	 * internally. */
    if (iid >= 0
		  && (instance->state & (STATE_OUBLIETTED|STATE_INVISIBLE|STATE_IN_PLAY)) == STATE_IN_PLAY)
      {
		if (is_damage_card == -1)	// i.e., caller doesn't know
		  is_damage_card = cards_data[iid].id == 901;

		if (is_damage_card)
		  {
			spkw = instance->targets[16].card & (SP_KEYWORD_DEATHTOUCH | SP_KEYWORD_LIFELINK | SP_KEYWORD_WITHER);

			if (instance->regen_status & KEYWORD_INFECT)
			  rval |= 1ULL<<ICON_INFECT;

			if (instance->token_status & STATUS_FIRST_STRIKE_DAMAGE)
			  rval |= 1ULL<<ICON_FIRST_STRIKE;

			if (instance->token_status & STATUS_TRAMPLE_DAMAGE)
			  rval |= 1ULL<<ICON_TRAMPLE;

			typ = TYPE_EFFECT;
		  }
		else
		  {
			kw = (keyword_t)instance->regen_status;
			spkw = get_special_abilities_by_instance(instance);
			if (spkw == -1)
			  spkw = 0;
			//untap_status = (untap_status_t)instance->untap_status;

			typ = (type_t)cards_data[iid].type;

			// This is mildly dangerous; cards_data can change during speculation.  But it's rare, and magic.exe already does it lots.
			// const card_data_t* cd = &cards_data[iid];
			// if ((typ & (TYPE_INSTANT|TYPE_INTERRUPT)) && (cd->type & TYPE_PERMANENT))
			// rval |= 1ULL<<ICON_FLASH;	// won't actually show up in Manalink since all icons are suppressed while off battlefield

			if (is_humiliated_by_instance(instance))
			  rval |= 1ULL<<ICON_LOST_ABILITIES;
			else if (!can_use_activated_abilities_by_instance(instance))
			  rval |= 1ULL<<ICON_CANT_ACTIVATE;
		  }
      }
	else
	  not_valid_card = 1;

    LeaveCriticalSection(critical_section_for_display);
  }

  if (is_damage_card || not_valid_card || (typ & TYPE_EFFECT))
    return rval;

  if (kw)
    {
      // Infuriating that display order doesn't match keyword order.
      if (kw & (KEYWORD_FLYING | KEYWORD_REACH | KEYWORD_BANDING | KEYWORD_PROT_CREATURES | KEYWORD_TRAMPLE | KEYWORD_DEFENDER))
		{
		  if (kw & KEYWORD_FLYING)
			rval |= 1ULL<<ICON_FLYING;
		  if (kw & KEYWORD_REACH)
			rval |= 1ULL<<ICON_REACH;
		  if (kw & KEYWORD_BANDING)
			rval |= 1ULL<<ICON_BANDING;
		  if (kw & KEYWORD_TRAMPLE)
			rval |= 1ULL<<ICON_TRAMPLE;
		  //if (kw & KEYWORD_PROT_CREATURES)	// no bit actually assigned; this means something else in Manalink
		  //rval |= 1ULL<<ICON_PROT_CREATURES;
		  if (kw & KEYWORD_DEFENDER)
			rval |= 1ULL<<ICON_DEFENDER;
		}

      if (kw & (KEYWORD_REGENERATION | KEYWORD_DOUBLE_STRIKE | KEYWORD_FIRST_STRIKE | KEYWORD_SHROUD | KEYWORD_INFECT))
		{
		  if (kw & KEYWORD_DOUBLE_STRIKE)
			rval |= 1ULL<<ICON_DOUBLE_STRIKE;
		  else if (kw & KEYWORD_FIRST_STRIKE)
			rval |= 1ULL<<ICON_FIRST_STRIKE;

		  if (kw & KEYWORD_REGENERATION)
			rval |= 1ULL<<ICON_REGENERATION;
		  if (kw & KEYWORD_SHROUD)
			rval |= 1ULL<<ICON_SHROUD;
		  if (kw & KEYWORD_INFECT)
			rval |= 1ULL<<ICON_INFECT;
		}

      if (kw & KEYWORD_BASIC_LANDWALK)
		rval |= (kw & KEYWORD_BASIC_LANDWALK) << ICON_SWAMPWALK;	// KEYWORD_SWAMPWALK is the first, so this is right

      if (kw & (KEYWORD_PROT_COLORED | KEYWORD_PROT_ARTIFACTS))
		rval |= (kw & (KEYWORD_PROT_COLORED | KEYWORD_PROT_ARTIFACTS)) << 4;	// KEYWORD PROT_BLACK 1<<11 to ICON_PROT_BLACK 15
    }

  //if (untap_status & UNTAP_STATUS_DOESNT_UNTAP)	// no bit assigned
  //rval |= 1ULL<<ICON_DOESNT_UNTAP;

  if (spkw)
    {
	  if (spkw & SP_KEYWORD_VIGILANCE)
		rval |= 1ULL<<ICON_VIGILANCE;
	  if (spkw & SP_KEYWORD_CANNOT_BLOCK)
		rval |= 1ULL<<ICON_CANT_BLOCK;
	  if (spkw & SP_KEYWORD_UNBLOCKABLE)
		rval |= 1ULL<<ICON_UNBLOCKABLE;
	  if (spkw & SP_KEYWORD_SHADOW)
		rval |= 1ULL<<ICON_SHADOW;
	  if (spkw & SP_KEYWORD_HORSEMANSHIP)
		rval |= 1ULL<<ICON_HORSEMANSHIP;
	  if (spkw & SP_KEYWORD_FEAR)
		rval |= 1ULL<<ICON_FEAR;
	  if (spkw & SP_KEYWORD_INTIMIDATE)
		rval |= 1ULL<<ICON_INTIMIDATE;
	  if (spkw & SP_KEYWORD_DEATHTOUCH)
		rval |= 1ULL<<ICON_DEATHTOUCH;
	  if (spkw & SP_KEYWORD_LIFELINK)
		rval |= 1ULL<<ICON_LIFELINK;
	  if (spkw & SP_KEYWORD_FLANKING)
		rval |= 1ULL<<ICON_FLANKING;
	  //if (spkw & SP_KEYWORD_RAMPAGE)	// no bit assigned
	  //rval |= 1ULL<<ICON_RAMPAGE;
	  if (spkw & SP_KEYWORD_BUSHIDO)
		rval |= 1ULL<<ICON_BUSHIDO;
	  if (spkw & SP_KEYWORD_WITHER)
		rval |= 1ULL<<ICON_WITHER;
	  //if (spkw & SP_KEYWORD_EXALTED)	// no bit assigned
	  //rval |= 1ULL<<ICON_EXALTED;
	  if (spkw & SP_KEYWORD_INDESTRUCTIBLE)
		rval |= 1ULL<<ICON_INDESTRUCTIBLE;
	  if (spkw & SP_KEYWORD_HASTE)
		rval |= 1ULL<<ICON_HASTE;
	  if (spkw & SP_KEYWORD_HEXPROOF)
		rval |= 1ULL<<ICON_HEXPROOF;
      //if (kw2 & SP_KEYWORD_PERSIST)	// no bit assigned
	  //rval |= 1ULL<<ICON_PERSIST;
      //if (kw2 & SP_KEYWORD_UNDYING)	// no bit assigned
	  //rval |= 1ULL<<ICON_UNDYING;
      //if (kw2 & SP_KEYWORD_NINJUTSU)	// no bit assigned, and can't display anyway in Manalink since all icons are suppressed while off battlefield
	  //rval |= 1ULL<<ICON_NINJUTSU;
      //if (kw2 & SP_KEYWORD_CANT_BE_COUNTERED)	// no bit assigned, and can't display anyway in Manalink since all icons are suppressed while off battlefield
	  //rval |= 1ULL<<ICON_CANT_BE_COUNTERED;
      //if (kw2 & (SP_KEYWORD_PROT_MISC | SP_KEYWORD_GRANTED_PROT_MISC))	// no bit assigned
	  //rval |= 1ULL<<ICON_PROT_MISC;
      if (spkw & SP_KEYWORD_SKULK)
		rval |= 1ULL<<ICON_SKULK;
      if (spkw & SP_KEYWORD_MENACE)
		rval |= 1ULL<<ICON_MENACE;
    }

  return rval;
}

/* The executable version has prototype void copy_rect_for_ability(RECT* dest, keyword_t keyword_bit, RECT* src,
 * keyword_t displayable_keyword_bits), instead of letting the caller sanely compute the position itself. */
static void copy_rect_for_ability(RECT* dest, const RECT* src, int pos)
{
  if (!dest)
    return;

  int max_icons = get_setting(SETTING_ABILITY_ICON_MAX);
  if (max_icons < 0)
    max_icons = ICON_MAX + 1;
  if (!src || pos >= max_icons)
    {
      SetRect(dest, 0,0, 0,0);
      return;
    }

  int size_plus_1 = get_setting(SETTING_ABILITY_ICON_SIZE);
  if (size_plus_1 > 0)
    size_plus_1 += 1;
  else
    size_plus_1 = 16 * (src->right - src->left) / 100 + 1;	// originally 17*, reduced to 16 so it can fit 6 across on upper rows

  int x = src->left + 1;
  int y = src->bottom - size_plus_1;

  int right_border = src->right - size_plus_1 - 26 * (src->right - src->left) / 100;
  int full_width = FALSE;
  int i;
  for (i = 0; i < pos; ++i)
    {
      x += size_plus_1;
      if (x >= right_border)
		{
		  x = src->left + 1;
		  y -= size_plus_1;
		  if (!full_width && y < src->bottom - (src->bottom - src->top) / 3)
			{
			  full_width = TRUE;
			  right_border = src->right - size_plus_1;
			}
		}
    }

  SetRect(dest, x,y, x + size_plus_1 - 1,y + size_plus_1 - 1);
}

static HBITMAP bmp_abilities_pic = NULL;

// The executable version had prototype void draw_abilities(HDC hdc, RECT* rect, keyword_t keyword_bits_to_display).
static void draw_abilities(HDC hdc, RECT* rect, int player, int card, int is_damage_card)
{
  if (!hdc || !rect || !option_ShowAbilitiesOnCards)
	return;

  static int once = 1;
  if (once)
    {
      once = 0;

	  char path[MAX_PATH];
	  int ability_icon_file_as_int = get_setting(SETTING_ABILITY_ICON_FILE);
	  scnprintf(path, MAX_PATH, "%s\\%s", path_CardArt, (const char*)ability_icon_file_as_int);
	  bmp_abilities_pic = read_graphics_file2(path);
	  premultiply_alpha_hbitmap(bmp_abilities_pic);
	}

  if (!bmp_abilities_pic)
	return;

  unsigned long long icons = get_icons(player, card, is_damage_card ? 1 : 0);
  if (!icons)
    return;

  int saved_dc = SaveDC(hdc);

  BITMAP bmp;
  GetObject(bmp_abilities_pic, sizeof(BITMAP), &bmp);

  int src_size = bmp.bmHeight / 5;

  RECT dest;

  int pos = 0;
  int bit;
  while ((bit = __builtin_ffsll(icons)))
    {
      --bit;
      icons &= ~(1ULL<<bit);

      int src_x = (bit / 5) * src_size;
      int src_y = (bit % 5) * src_size;

      copy_rect_for_ability(&dest, rect, pos++);
      if (bit > ICON_MAX || dest.right == 0)
		break;

      stretchblt_from_bmp(hdc, &dest,
						  bmp_abilities_pic, src_x, src_y, src_size, src_size);
    }

  RestoreDC(hdc, saved_dc);
}

static int card_instances_will_have_same_icons(card_instance_t* inst1, card_instance_t* inst2)
{
  // By the time we get here, we already know that the cards have the same iid, so no change in flash, or legendary.
  if ((inst1->regen_status ^ inst2->regen_status)
      & (((KEYWORD_SHROUD<<1)-1) /*| KEYWORD_PROT_CREATURES*/ | KEYWORD_DEFENDER | KEYWORD_INFECT | KEYWORD_DOUBLE_STRIKE))
    return 0;

  //if ((inst1->untap_status ^ inst2->untap_status) & UNTAP_STATUS_DOESNT_UNTAP)	// no bit assigned
  //return 0;

  if (is_humiliated_by_instance(inst1) != is_humiliated_by_instance(inst2))
	return 0;

  int spkw1 = get_special_abilities_by_instance(inst1);
  if (spkw1 == -1)
	spkw1 = 0;
  int spkw2 = get_special_abilities_by_instance(inst2);
  if (spkw2 == -1)
	spkw2 = 0;

  if ((spkw1 ^ spkw2) & (SP_KEYWORD_VIGILANCE | SP_KEYWORD_CANNOT_BLOCK | SP_KEYWORD_UNBLOCKABLE | SP_KEYWORD_SHADOW
						 | SP_KEYWORD_HORSEMANSHIP | SP_KEYWORD_FEAR | SP_KEYWORD_INTIMIDATE | SP_KEYWORD_DEATHTOUCH
						 | SP_KEYWORD_LIFELINK | SP_KEYWORD_FLANKING | SP_KEYWORD_BUSHIDO | SP_KEYWORD_WITHER
						 | SP_KEYWORD_INDESTRUCTIBLE | SP_KEYWORD_HASTE | SP_KEYWORD_HEXPROOF | SP_KEYWORD_SKULK
						 | SP_KEYWORD_MENACE))
    return 0;

  return 1;
}

static char* ability_tooltip_names[ICON_MAX + 1] = { 0 };
char* counter_names[COUNTER_end + 1] = { 0 };

// Prerequisite: load_text(0, "ABILITYWORDS") has already been called.
// This is currently the earliest C function called by the exe.
void initialize_ability_tooltip_names(void)
{
  if (!ability_tooltip_names[0])
	{
	  int i;
	  for (i = 0; i <= ICON_MAX; ++i)
		{
		  ability_tooltip_names[i] = strdup(text_lines[i]);	// won't ever be freed
		  ASSERT(strlen(ability_tooltip_names[i]) <= 49 && "initialize_ability_tooltip_names(): ability name too long; must be 49 characters or less");
		}

	  // Now copy the original ones to where the exe expects them
	  ability_icon_t internal_from_icon[17] =
		{
		  ICON_FLYING,
		  ICON_REACH,
		  ICON_BANDING,
		  ICON_TRAMPLE,
		  ICON_FIRST_STRIKE,
		  ICON_REGENERATION,
		  ICON_SWAMPWALK,
		  ICON_ISLANDWALK,
		  ICON_FORESTWALK,
		  ICON_MOUNTAINWALK,
		  ICON_PLAINSWALK,
		  ICON_PROT_BLACK,
		  ICON_PROT_BLUE,
		  ICON_PROT_GREEN,
		  ICON_PROT_RED,
		  ICON_PROT_WHITE,
		  ICON_PROT_ARTIFACTS
		};
	  for (i = 0; i < 17; ++i)
		{
		  strncpy(ability_tooltip_names_exe[i], ability_tooltip_names[internal_from_icon[i]], 49);
		  ability_tooltip_names_exe[i][49] = '\0';
		}

	  // Counter names
	  load_text(0, "CUECARD_COUNTERS_SPECIAL");
	  for (i = 0; i <= COUNTER_end; ++i)
		counter_names[i] = strdup(text_lines[i]);	// won't ever be freed
	}

  init_subtype_text();
}

/* The executable version is inlined and results in an index into UIStrings.txt::ABILITYWORDS, which is later looked
 * up.  Skip the middleman. */
static const char* get_name_for_ability_icon_at_point(const RECT* rect, int player, int card, int x, int y)
{
  if (!rect || !option_ShowAbilitiesOnCards)
    return NULL;

  unsigned long long icons = get_icons(player, card, -1);
  if (!icons)
    return NULL;

  RECT dest;
  POINT pt;	pt.x = x;	pt.y = y;

  int pos = 0;
  int bit;
  while ((bit = __builtin_ffsll(icons)))
    {
      --bit;
      icons &= ~(1ULL<<bit);

      copy_rect_for_ability(&dest, rect, pos++);
      if (bit > ICON_MAX || dest.right == 0)
		break;

      if (PtInRect(&dest, pt))
		return ability_tooltip_names[bit];
    }
  return NULL;
}

LRESULT __stdcall wndproc_CardClass_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int player, card;
  card_instance_t actual;

  switch (msg)
	{
	  case WM_COMMAND:	// Popup menu
		switch (LOWORD(wparam))
		  {
			case 0x72:	// Card name from Original Type submenu
			  {
				player = GetWindowLongA(hwnd, cardclass_offset_player);
				card = GetWindowLongA(hwnd, cardclass_offset_card);

				card_instance_t* instance = get_displayed_card_instance(player, card);
				int oici = instance->original_internal_card_id;
				int csvid = cards_data[oici].id;
				SendMessage(hwnd_FullCardClass, 0x401, csvid, 0);
			  }
			  return 0;

			case 0x264:	// Bury this card
			  if (debug_mode)
				{
				  int old_phasestopper = option_PhaseStoppers[current_turn][current_phase];
				  option_PhaseStoppers[current_turn][current_phase] &= ~1;

				  player = GetWindowLongA(hwnd, cardclass_offset_player);
				  card = GetWindowLongA(hwnd, cardclass_offset_card);

				  card_instance_t* instance = get_card_instance(player, card);
				  instance->token_status |= STATUS_OBLITERATED;	// prevents it from being put into graveyard

				  if (in_play(player, card) && is_what(player, card, TYPE_PERMANENT))
					kill_card(player, card, KILL_STATE_BASED_ACTION);	// Added
				  else
					kill_card(player, card, KILL_BURY);

				  option_PhaseStoppers[current_turn][current_phase] = old_phasestopper;
				}
			  return 0;
		  }
		break;	// And fall through to call to wndproc_CardClass() below

	  case 0x432:	// redraw title
		if (!IsWindowVisible(hwnd))
		  return 0;

		player = GetWindowLong(hwnd, cardclass_offset_player);
		card = GetWindowLong(hwnd, cardclass_offset_card);
		if (card == -1 && player != -1)
		  return 0;

		int stashed_instance_cast_to_int = GetWindowLong(hwnd, cardclass_offset_instance_copy);
		card_instance_t* stashed = (card_instance_t *)stashed_instance_cast_to_int;
		memcpy_displayed_card_instance_t(&actual, player, card);
		if (!card_instances_should_be_displayed_identically(stashed, &actual)
			|| (option_ShowAbilitiesOnCards && !card_instances_will_have_same_icons(stashed, &actual)))
		  {
			InvalidateRect(hwnd, 0, 0);
			return 0;
		  }

		if (stashed->unknown0x70 == actual.unknown0x70)	// highlight color
		  return 0;

		if (actual.unknown0x70 == 0)
		  {
			/* i.e., the default color - in modern frames, this is drawn without a shadow, while can-activate/
			 * must-activate are drawn with one, so always redraw the whole card if the new color is default */
			InvalidateRect(hwnd, 0, 0);
			return 0;
		  }

		if ((actual.state & STATE_TAPPED)
			|| actual.internal_card_id == -1)
		  {
			InvalidateRect(hwnd, 0, 0);
			return 0;
		  }

		int csvid = cards_data[actual.internal_card_id].id;
		if (csvid >= 901 && csvid <= 907)
		  {
			InvalidateRect(hwnd, 0, 0);
			return 0;
		  }

		HDC hdc = GetDC(hwnd);
		set_palette_and_stretch_mode(hdc);
		RECT r;
		GetClientRect(hwnd, &r);
		int controlled_by_owner = !(actual.state & STATE_OWNED_BY_OPPONENT) == !player;
		uint32_t title_hilit = actual.unknown0x70;
		const card_ptr_t* cp = cards_ptr[csvid];
		DrawSmallCardTitle(hdc, &r, cp->name, title_hilit, controlled_by_owner | 0x8, cp, player, card);
		ReleaseDC(hwnd, hdc);
		stashed->unknown0x70 = actual.unknown0x70;

		return 0;

	  case 0x437:	// tooltips
		;char tooltip[100];
		tooltip[0] = 0;

		player = GetWindowLongA(hwnd, cardclass_offset_player);
		card = GetWindowLongA(hwnd, cardclass_offset_card);
		POINT pt;
		pt.x = LOWORD(lparam);
		pt.y = HIWORD(lparam);

		if (player != -1 && card == -1)
		  strcpy(tooltip, EXE_STR(0x6286f4));	// CUECARD_SMALLCARD[0] = Damage to player
		else
		  {
			RECT client_rect, rect;
			GetClientRect(hwnd, &client_rect);

			memcpy_displayed_card_instance_t(&actual, player, card);
			card_instance_t* instance = &actual;

			if (instance->state & STATE_TAPPED)
			  {
				int rotate = pt.x;
				pt.x = pt.y;
				pt.y = client_rect.bottom - rotate;
			  }

			const char* ability_tooltip;

			if (((instance->untap_status & (UNTAP_STATUS_COULD_UNTAP|UNTAP_STATUS_WILL_UNTAP))
				 == (UNTAP_STATUS_COULD_UNTAP|UNTAP_STATUS_WILL_UNTAP))
				&& (EXE_FN(void, 0x4d4c20, RECT*, RECT*)(&rect, &client_rect),	// copy_untap_status_rect()
					PtInRect(&rect, pt)))
			  strcpy(tooltip, EXE_STR(0x78f0cc));	// CUECARD_SMALLCARD[1] = This card will untap
			else if ((ability_tooltip = get_name_for_ability_icon_at_point(&client_rect, player, card, pt.x, pt.y)))
			  {
				/* Replace detection of ability icon tooltips.  Magic.exe inlines the logic to get an index into
				 * Text.res::ABILITYWORDS, (which, much later, it dereferences), or -1 if the mouse isn't over an
				 * ability icon.  We instead put it all into one function, get_name_for_ability_icon_at_point(),
				 * which returns either the strings or NULL.  get_name_for_ability_icon_at_point() also checks
				 * option_ShowAbilitiesOnCards and works correctly for the icons on damage cards, both of which the
				 * exe version neglects to do. */

				strcpy(tooltip, ability_tooltip);
			  }
			else if (instance->damage_on_card > 0
					 && (EXE_FN(void, 0x4d3770, RECT*, RECT*)(&rect, &client_rect),	// copy_damage_rect()
						 PtInRect(&rect, pt)))
			  sprintf(tooltip, EXE_STR(0x786230), instance->damage_on_card);	// CUECARD_SMALLCARD[2] = Damage: %d
			else if (instance->special_counters > 0
					 && (EXE_FN(void, 0x4d3b60, RECT*, RECT*, int)(&rect, &client_rect, instance->special_counters),	// copy_special_counter_rect()
						 PtInRect(&rect, pt)))
			  get_counter_name_by_type(tooltip, instance->special_counter_type, instance->special_counters);
			else if (instance->counters2
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && client_rect.left + 2 * (client_rect.right - client_rect.left) / 3 <= pt.x)
			  get_counter_name_by_type(tooltip, BYTE0(instance->targets[18].player), instance->counters2);
			else if (instance->counters3
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && client_rect.left + (client_rect.right - client_rect.left) / 3 <= pt.x
					 && pt.x < client_rect.left + 2 * (client_rect.right - client_rect.left) / 3)
			  get_counter_name_by_type(tooltip, BYTE1(instance->targets[18].player), instance->counters3);
			else if (instance->counters4
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && pt.x < client_rect.left + (client_rect.right - client_rect.left) / 3)
			  get_counter_name_by_type(tooltip, BYTE2(instance->targets[18].player), instance->counters4);
			else if (instance->counters5
					 && 53 * client_rect.bottom / 100 < pt.y && pt.y < 89 * client_rect.bottom / 100
					 && (client_rect.right + client_rect.left) / 2 <= pt.x)
			  get_counter_name_by_type(tooltip, BYTE3(instance->targets[18].player), instance->counters5);
			else if ((instance->counters || instance->counters_m1m1)
					 && 53 * client_rect.bottom / 100 < pt.y && pt.y < 89 * client_rect.bottom / 100
					 && pt.x < (client_rect.right + client_rect.left) / 2)
			  {
				if (instance->counters_m1m1 == 0)
				  get_counter_name_by_type(tooltip, COUNTER_P1_P1, instance->counters);
				else if (instance->counters == 0)
				  get_counter_name_by_type(tooltip, COUNTER_M1_M1, instance->counters_m1m1);
				else
				  {
					char m1m1[100];
					get_counter_name_by_type(m1m1, COUNTER_M1_M1, instance->counters_m1m1);
					char p1p1[100];
					get_counter_name_by_type(p1p1, COUNTER_P1_P1, instance->counters);
					scnprintf(tooltip, 100, "%s; %s", m1m1, p1p1);
				  }
			  }
			else if (!(instance->state & STATE_OWNED_BY_OPPONENT) != !player && pt.y < 12 * client_rect.bottom / 100)
			  strcpy(tooltip, EXE_STR(0x7283b4));	// CUECARD_SMALLCARD[3] = Card is not controlled by owner
			else if (5 * client_rect.right / 100 < pt.x
					 && 95 * client_rect.right / 100 > pt.x
					 && 15 * client_rect.bottom / 100 < pt.y
					 && 95 * client_rect.bottom / 100 > pt.y)
			  {
				if (instance->state & (STATE_TARGETTED | STATE_CANNOT_TARGET))
				  {
					switch (instance->state & (STATE_TARGETTED | STATE_CANNOT_TARGET))
					  {
						case STATE_TARGETTED | STATE_CANNOT_TARGET:	strcpy(tooltip, EXE_STR(0x739dd0));	break;	// CUECARD_SMALLCARD[6] = Is a target, can't target again
						case STATE_TARGETTED:							strcpy(tooltip, EXE_STR(0x620d5c));	break;	// CUECARD_SMALLCARD[4] = Is a target
						case STATE_CANNOT_TARGET:						strcpy(tooltip, EXE_STR(0x737e24));	break;	// CUECARD_SMALLCARD[5] = Can't target this
					  }
				  }
				else if (instance->kill_code == KILL_DESTROY)
				  strcpy(tooltip, EXE_STR(0x786f08));	// CUECARD_SMALLCARD[7] = Dying
				else
				  {
					int is_summonsick = ((instance->state & STATE_SUMMON_SICK)
										 && (option_ShowAllCardsSummonSickness
											 || (cards_data[instance->internal_card_id].type & TYPE_CREATURE)));
					int is_phased = instance->state & STATE_OUBLIETTED;
					if (is_summonsick && is_phased)
					  sprintf(tooltip, "%s, %s",
							  EXE_STR(0x728374),	// CUECARD_SMALLCARD[8] = Summoning sickness
							  EXE_STR(0x62bcf0));	// CUECARD_SMALLCARD[9] = Phased
					else if (is_summonsick)
					  strcpy(tooltip, EXE_STR(0x728374));	// CUECARD_SMALLCARD[8] = Summoning sickness
					else if (is_phased)
					  strcpy(tooltip, EXE_STR(0x62bcf0));	// CUECARD_SMALLCARD[9] = Phased
				  }
			  }
		  }

		if (tooltip[0])
		  strcpy((char *)wparam, tooltip);

		if (option_Layout != 2)
		  SendMessageA(hwnd, WM_COMMAND, 0x6E, 0);
		return tooltip[0] != 0;
	}

  return EXE_STDCALL_FN(LRESULT, 0x489d60, HWND, UINT, WPARAM, LPARAM)(hwnd, msg, wparam, lparam);	// wndproc_CardClass()
}

LRESULT __stdcall wndproc_MainClass_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
	{
	  case WM_COMMAND:	// Popup menu
		;int cmd = LOWORD(wparam);
		switch (cmd)
		  {
            case 0x26b:
            case 0x26c:
            case 0x26d:
            case 0x26e:
              if (!debug_mode)
				return 0;

			  int player = (cmd == 0x26b || cmd == 0x26d) ? 0 : 1;
			  int onto_bf = cmd == 0x26b || cmd == 0x26c;
			  int iid = choose_a_card(onto_bf
									  ? (player == 0 ? "Pick a card to put on battlefield"
										 : "Pick a card to put on opponent's battlefield")
									  : (player == 0 ? "Pick a card to put in your hand"
										 : "Pick a card to put in opponent's hand"),
									  -1, -1);
			  if (iid < 0)
				return 0;

			  int old_phasestopper = option_PhaseStoppers[current_turn][current_phase];
			  option_PhaseStoppers[current_turn][current_phase] &= ~1;

			  int c = add_card_to_hand(player, iid);
			  if (c != -1)
				{
				  update_rules_engine(check_card_for_rules_engine(iid));
				  if (onto_bf)
					put_into_play(player, c);
				}

			  option_PhaseStoppers[current_turn][current_phase] = old_phasestopper;
			  EXE_FN(int, 0x437ec0, int, int)(0, 0xFF);	// redisplay_all()

			  return 0;
		  }
		break;	// And fall through to call to wndproc_CardClass() below
	}

  return EXE_STDCALL_FN(LRESULT, 0x4933c0, HWND, UINT, WPARAM, LPARAM)(hwnd, msg, wparam, lparam);	// wndproc_MainClass()
}

//[[[ display experience counters
#define bmp_poison_pic						EXE_TYP(HBITMAP, 0x5b9a64)
#define copy_opponent_name_until_dash		EXE_FN(void, 0x4391f0, char*)
#define hwnd_LifeClass_player				EXE_TYP(HWND, 0x738c7c)
#define lifeclass_offset_liched				EXE_DWORD(0x4d7544)
#define lifeclass_offset_life				EXE_DWORD(0x4d753c)
#define lifeclass_offset_player_counters	EXE_DWORD(0x4d7540)
#define stretchblt_with_transparency_where_stencil_is_right_half	EXE_FN(void, 0x466540, HDC, RECT*, HBITMAP)

static HBITMAP get_bmp_experience(void)
{
  static HBITMAP bmp_experience_pic = NULL;
  if (!bmp_experience_pic)
    {
      char path[264];
      sprintf(path, "%s\\exper.pic", path_CardArt);
      bmp_experience_pic = read_graphics_file(path);
      premultiply_alpha_hbitmap(bmp_experience_pic);
    }
  return bmp_experience_pic;
}

static void paint_one_counter_type(HDC hdc, int width, int height, HBITMAP bmp, int num_counters, void (*blt_fn)(HDC, RECT*, HBITMAP))
{
  // Is this ever overengineered.
  int counter_width = width;
  int counter_height = (4 * height) / 3;
  if (num_counters <= 12)
    {
      counter_width /= 4;
      counter_height /= 4;	// height/3
    }
  else if (num_counters <= 15)
    {
      counter_width /= 5;
      counter_height /= 5;	// 4height/15: < 1/3
    }
  else if (num_counters <= 24)
    {
      counter_width /= 6;
      counter_height /= 6;	// 4height/18: < 1/4
    }
  else if (num_counters <= 35)
    {
      counter_width /= 7;
      counter_height /= 7;	// 4height/21: < 1/5
    }
  else if (num_counters <= 48)
    {
      counter_width /= 8;
      counter_height /= 8;	// height/6
    }
  else if (num_counters <= 63)
    {
      counter_width /= 9;
      counter_height /= 9;	// 4height/27: < 1/6
    }
  else if (num_counters <= 80)
    {
      counter_width /= 10;
      counter_height /= 10;	// 4height/30: < 1/7
    }
  else if (num_counters <= 99)
    {
      counter_width /= 11;
      counter_height /= 11;	// 4height/33: < 1/8
    }
  else if (num_counters <= 108)
    {
      counter_width /= 12;
      counter_height /= 12;	// height/9
    }
  else if (num_counters <= 117)
    {
      counter_width /= 13;
      counter_height /= 13;	// 4height/39: < 1/9
    }
  else if (num_counters <= 140)
    {
      counter_width /= 14;
      counter_height /= 14;	// 4height/42: < 1/10
    }
  else if (num_counters <= 165)
    {
      counter_width /= 15;
      counter_height /= 15;	// 4height/45: < 1/11
    }
  else if (num_counters <= 192)
    {
      counter_width /= 16;
      counter_height /= 16;	// height/12
    }
  else if (num_counters <= 204)
    {
      counter_width /= 17;
      counter_height /= 17;	// 4height/51: < 1/12
    }
  else if (num_counters <= 234)
    {
      counter_width /= 18;
      counter_height /= 18;	// 4height/54: < 1/13
    }
  else	// up to 266
    {
      counter_width /= 19;
      counter_height /= 19;	// 4height/57: < 1/14
    }

  int x = 0;
  int y = 0;
  RECT rc;
  int i;
  for (i = 0; i < num_counters; ++i)
    {
      SetRect(&rc, x, y, x + counter_width, y + counter_height);
      blt_fn(hdc, &rc, bmp);
      x += counter_width;
      if (x + counter_width > width)
		{
		  x = 0;
		  y += counter_height;
		}
    }
}

static void paint_half_of_two_counter_types(HDC hdc, int width, int height, int offset_x,
											HBITMAP bmp, int num_counters, void (*blt_fn)(HDC, RECT*, HBITMAP))
{
  int counter_width = width;
  int counter_height = (4 * height) / 3;
  // width is halved for this, hence e.g. /10 becomes 5 across
  if (num_counters <= 6)
    {
      counter_width /= 4;
      counter_height /= 4;	// height/3
    }
  else if (num_counters <= 12)
    {
      counter_width /= 6;
      counter_height /= 6;	// 4height/18: < 1/4
    }
  else if (num_counters <= 24)
    {
      counter_width /= 8;
      counter_height /= 8;	// height/6
    }
  else if (num_counters <= 35)
    {
      counter_width /= 10;
      counter_height /= 10;	// 4height/30: < 1/7
    }
  else if (num_counters <= 54)
    {
      counter_width /= 12;
      counter_height /= 12;	// height/9
    }
  else if (num_counters <= 70)
    {
      counter_width /= 14;
      counter_height /= 14;	// 4height/42: < 1/10
    }
  else if (num_counters <= 96)
    {
      counter_width /= 16;
      counter_height /= 16;	// height/12
    }
  else if (num_counters <= 117)
    {
      counter_width /= 18;
      counter_height /= 18;	// 4height/54: < 1/13
    }
  else if (num_counters <= 150)
    {
      counter_width /= 20;
      counter_height /= 20;	// height/15
    }
  else if (num_counters <= 176)
    {
      counter_width /= 22;
      counter_height /= 22;	// 4height/66: < 1/16
    }
  else if (num_counters <= 216)
    {
      counter_width /= 24;
      counter_height /= 24;	// height/18
    }
  else if (num_counters <= 247)
    {
      counter_width /= 26;
      counter_height /= 26;	// 4height/78: < 1/19
    }
  else // up to 294
    {
      counter_width /= 28;
      counter_height /= 28;	// height/21
    }

  int x = 0;
  int y = 0;
  RECT rc;
  int i;
  width /= 2;
  for (i = 0; i < num_counters; ++i)
    {
      SetRect(&rc, offset_x + x, y, offset_x + x + counter_width, y + counter_height);
      blt_fn(hdc, &rc, bmp);
      x += counter_width;
      if (x + counter_width > width)
		{
		  x = 0;
		  y += counter_height;
		}
    }
}

static void paint_two_counter_types(HDC hdc, int width, int height,
									HBITMAP bmp1, int num_counters1, void (*blt_fn1)(HDC, RECT*, HBITMAP),
									HBITMAP bmp2, int num_counters2, void (*blt_fn2)(HDC, RECT*, HBITMAP))
{
  paint_half_of_two_counter_types(hdc, width, height,         0, bmp1, num_counters1, blt_fn1);
  paint_half_of_two_counter_types(hdc, width, height, width / 2, bmp2, num_counters2, blt_fn2);
}

void paint_player_counters(HDC hdc, int width, int height, int player_counters_cast_to_int)
{
  PlayerCounters counters = *(PlayerCounters*)(&player_counters_cast_to_int);
  if (counters.poison_ > 0 && counters.experience_ > 0)
    paint_two_counter_types(hdc, width, height,
							bmp_poison_pic, counters.poison_, stretchblt_with_transparency_where_stencil_is_right_half,
							get_bmp_experience(), counters.experience_, stretchblt_bmp);
  else
    {
      if (counters.poison_ > 0)
		paint_one_counter_type(hdc, width, height, bmp_poison_pic, counters.poison_, stretchblt_with_transparency_where_stencil_is_right_half);
      else if (counters.experience_ > 0)
		paint_one_counter_type(hdc, width, height, get_bmp_experience(), counters.experience_, stretchblt_bmp);
    }
}

LRESULT __stdcall wndproc_LifeClass_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      // Also, part of WM_PAINT is replaced with a call to paint_player_counters() above.

      case WM_USER + 0x37:	// hover
		;
		char dest[300];	// so there's no worries of overflow here
		*dest = 0;
		char* p = dest;

		if (hwnd == hwnd_LifeClass_player)
		  p += sprintf(p, "Your life: ");
		else
		  {
			copy_opponent_name_until_dash(p);
			while (*p)
			  ++p;
			p += sprintf(p, "'s life: ");
		  }

		if (GetWindowLong(hwnd, lifeclass_offset_liched))
		  p += sprintf(p, "LICH");
		else
		  p += sprintf(p, "%ld", GetWindowLong(hwnd, lifeclass_offset_life));

		int stored_player_counters_cast_to_int = GetWindowLong(hwnd, lifeclass_offset_player_counters);
		PlayerCounters stored_player_counters = *(PlayerCounters*)(&stored_player_counters_cast_to_int);

		if (stored_player_counters.poison_)
		  p += sprintf(p, " Poison counters: %d", stored_player_counters.poison_);
		if (stored_player_counters.experience_)
		  p += sprintf(p, " Experience counters: %d", stored_player_counters.experience_);

		dest[99] = 0;	// actual maximum length

		strcpy((char*)wparam, dest);
		return 1;
    }
  return EXE_STDCALL_FN(LRESULT, 0x497300, HWND, UINT, WPARAM, LPARAM)(hwnd, msg, wparam, lparam);	// wndproc_LifeClass()
}
//]]]

#define smallcard_width		EXE_DWORD(0x7286f8)
#define smallcard_height	EXE_DWORD(0x739d50)

int set_smallcard_size(int mainwindow_width)
{
  static int setting_smallcard_size = 0;
  if (setting_smallcard_size == 0)	// unset
	{
	  read_settings();	// Since this function is usually called before the others that call read_settings() (pregame() and after_load_game())

	  setting_smallcard_size = get_setting(SETTING_SMALLCARD_SIZE);
	  if (setting_smallcard_size == 0)	// set to default
		setting_smallcard_size = -8;
	}

  int sz = setting_smallcard_size;
  if (sz < 0)
    sz = mainwindow_width / -sz;

  smallcard_width = smallcard_height = sz;

  return sz;
}

void update_hand_window(HWND hwnd)
{
  int num_cards = GetWindowLong(hwnd, 4);
  int cw = GetWindowLong(hwnd, 0);
  HWND* card_hwnds = (HWND *)cw;

  int v19, v22, v2, v3;
  EXE_FN(void, 0x47d850, int, int, int, int, int, int,
		 int*, int*, int*, int*)(smallcard_width, smallcard_height,
								 GetWindowLong(hwnd, 8),
								 GetWindowLong(hwnd, 12),
								 GetWindowLong(hwnd, 16),
								 GetWindowLong(hwnd, 20),
								 &v19, &v22, &v2, &v3);

  int hand_window_width = smallcard_width + 2*(v2 + v3);
  int hand_window_height = v19 + v22;

  if (hwnd == EXE_PTR_VOID(0x7158c8)	// hwnd_HandClass_player
	  || (hwnd == EXE_PTR_VOID(0x786dc4)	// hwnd_HandClass_opponent
		  && ((EXE_DWORD(0x628c24) & 2) || EXE_DWORD(0x628c28))))
	{
	  int cards_to_show = MIN(num_cards, get_setting(SETTING_HAND_SIZE));

	  if (cards_to_show > 0)
		{
		  int offset = EXE_DWORD(0x7A31A4);

		  hand_window_height += smallcard_height + offset * (cards_to_show - 1);

		  int x = v2 + v3;
		  int y = hand_window_height - v22 - smallcard_height;
		  int j = GetWindowLong(hwnd, 28);
		  HWND hwnd_after = NULL;

		  int i;
		  for (i = 1; i <= cards_to_show; ++i)
			{
			  MoveWindow(card_hwnds[j], x, y, smallcard_width, smallcard_height, 1);

			  if (hwnd_after)
				SetWindowPos(card_hwnds[j], hwnd_after, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
			  else
				BringWindowToTop(card_hwnds[j]);

			  hwnd_after = card_hwnds[j];

			  if (--j < 0)
				j = num_cards - 1;

			  y -= offset;
			}

		  for (i = cards_to_show; i < num_cards; ++i)
			{
			  MoveWindow(card_hwnds[j], -1, -1, 0, 0, 1);

			  if (--j < 0)
				j = num_cards - 1;
			}
		}
	  UpdateWindow(hwnd);
	  SetWindowPos(hwnd, 0, 0, 0, hand_window_width, hand_window_height, SWP_NOZORDER|SWP_NOMOVE);
	}
  else
	{
	  int i;
	  for (i = 0; i < num_cards; ++i)
		MoveWindow(card_hwnds[i], -1, -1, 0, 0, 1);
	  SetWindowPos(hwnd, 0, 0, 0, hand_window_width, hand_window_height, SWP_NOZORDER|SWP_NOMOVE);
	}
}

// Sound effects.  Somewhat misplaced here, but this is as close as Manalink comes to "UI"
static const char* wav_filenames[WAV_HIGHEST + 1] =
{
  "artifact.wav",
  "buried.wav",
  "draw.wav",
  "enchant.wav",
  "endphase.wav",
  "endturn.wav",
  "instant.wav",
  "interupt.wav",
  "GREY.wav",
  "BLACK.wav",
  "BLUE.wav",
  "GREEN.wav",
  "RED.wav",
  "WHITE.wav",
  "lifeloss.wav",
  "sacrfice.wav",
  "sorcery.wav",
  "summon.wav",
  "tap.wav",
  "untap.wav",
  "attack2.wav",
  "block2.wav",
  "damage.wav",
  "destroy.wav",
  "discard.wav",
  "kill.wav",
  "regen.wav",
  "BLACKRED.wav",
  "GREENBLACK.wav",
  "WHITERED.wav",
  "WHITEGREEN.wav",
  "BLACKWHITE.wav",
  "GREENRED.wav",
  "GREENBLUE.wav",
  "WHITEBLUE.wav",
  "BLUEBLACK.wav",
  "REDBLUE.wav",
  "counter.wav",
  "fastfx.wav",
  "changec.wav",
  "changet.wav",
  "control.wav",
  "manaburn.wav",
  "shuffle.wav",
  "shell_loseduel.wav",
  "shell_winduel.wav",
  "aswanjag.wav",
  "callgrav.wav",
  "faerdrag.wav",
  "gembazar.wav",
  "necrazar.wav",
  "polkamix.wav",
  "pandora.wav",
  "prsmdrag.wav",
  "pwrstrgl.wav",
  "catatap.wav",
  "orcart.wav",
  "whimsy.wav",
  "rainbowk.wav",
  "toss.wav",
  "shell_shandalar.wav",
  "shell_tooltime.wav",
  "shell_helpme.wav",
  "shell_hallofrecords.wav",
  "shell_duelmenow.wav",
  "exp1_openfoil.wav",
  "exp1_openbox.wav",
  "exp1_outofpack.wav",
  "exp1_backinpack.wav",
  "Bloodthirst.wav",
  "Devour.wav",
  "Monstrosity.wav",
  "Evolve.wav",
  "Plus_Counter.wav",
  "Minus_Counter.wav",
  "Mill.wav",
  "Raise_Dead.wav",
  "Bushido.wav",
  "Equip.wav",
  "LifeGain.wav",
  "Transform.wav",
};

int play_sound_effect(wav_t soundnum)
{
  if (ai_is_speculating == 1)
    return 0;

  typedef struct	// very little idea what any of these fields do.
  {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int field_1C;
  } sound_t;

  sound_t snd;
  snd.field_0 = 300;
  snd.field_4 = 0;
  snd.field_8 = 0;
  snd.field_C = 0;
  snd.field_10 = 0;
  snd.field_14 = 0;
  snd.field_18 = soundnum;
  snd.field_1C = 0;

  if (soundnum >= WAV_ASWANJAG && soundnum <= WAV_TOSS)
    {
      snd.field_0 = 400;
      if (soundnum == WAV_CATATAP)
		snd.field_14 = -1;
      else
		snd.field_1C |= 4;
    }

  wav_t adj_soundnum = soundnum;
  if (!EXE_FN(int, 0x45a270, wav_t, wav_t*)(soundnum, &adj_soundnum))	// call_IsSndLoaded()
    {
      char path[300];
      strcpy(path, EXE_STR(0x715788));	// path_DuelSounds
      strcat(path, "\\");
      strcat(path, wav_filenames[soundnum]);
      EXE_FN(int, 0x45a1c0, const char*, int, sound_t*)(path, adj_soundnum, &snd);	// call_LoadSnd()
    }

  EXE_FN(int, 0x45a240, int, sound_t*)(adj_soundnum, 0);	// call_PlaySnd()

  return 1;
}

int get_displayed_pic_num_and_pic_csv_id_of_parent_playercard(target_t* ret_parent, int player, int card)
{
  if (player < 0 || player > 1
	  || card < 0 || card >= 150)
	{
	  ret_parent->player = ret_parent->card = -1;
	  return -1;
	}

  card_instance_t* inst = get_displayed_card_instance(player, card);
  ret_parent->player = inst->parent_controller;
  ret_parent->card = inst->parent_card;

  int csvid = inst->display_pic_csv_id;
  int num = get_card_image_number(csvid, inst->parent_controller, inst->parent_card);
  return ((num << 16) & 0xFFFF0000) | (csvid & 0xFFFF);
}

#define path_DuelArt		EXE_STR(0x739e30)
void load_startduel_assets(HGDIOBJ* a1, COLORREF* a2, HGDIOBJ* a3, HGDIOBJ* a4, HGDIOBJ* a5, COLORREF* a6,
						   COLORREF* a7)
{
  char path[264];

  sprintf(path, "%s\\WINBK_StartDuel.bmp", path_DuelArt);
  *a1 = read_graphics_file(path);
  *a2 = 0;
  sprintf(path, "%s\\WINBK_StartDuelButtonNormal.bmp", path_DuelArt);
  *a3 = read_graphics_file(path);
  sprintf(path, "%s\\WINBK_StartDuelButtonDepressed.bmp", path_DuelArt);
  *a4 = read_graphics_file(path);
  sprintf(path, "%s\\WINBK_StartDuelButtonDisabled.pic", path_DuelArt);
  *a5 = read_graphics_file(path);
  *a6 = 0x1000001;
  *a7 = 0x10000BF;
}

void load_startduel_assets2(HGDIOBJ* a1, COLORREF* a2, HGDIOBJ* a3, HGDIOBJ* a4, COLORREF* a5, COLORREF* a6)
{
  char path[264];

  sprintf(path, "%s\\WINBK_StartDuel2.bmp", path_DuelArt);
  *a1 = read_graphics_file(path);
  *a2 = 0;
  sprintf(path, "%s\\WINBK_StartDuelButtonNormal.bmp", path_DuelArt);
  *a3 = read_graphics_file(path);
  sprintf(path, "%s\\WINBK_StartDuelButtonDepressed.bmp", path_DuelArt);
  *a4 = read_graphics_file(path);
  *a5 = 0x1000001;
  *a6 = 0x10000BF;
}

// no need to link gdi32 just for this function when Magic.exe does it already anyway.
#define manalink_CreatePen			EXE_STDCALL_FN(HPEN, 0x4d5ccc, int, int, COLORREF)
#define manalink_CreateSolidBrush	EXE_STDCALL_FN(HBRUSH, 0x4d5c42, COLORREF)
#define manalink_GetStockObject		EXE_STDCALL_FN(HGDIOBJ, 0x4d5c9c, int)
void load_endduel_assets(HGDIOBJ *a1, COLORREF *a2, COLORREF *a3, HBRUSH *a4, HPEN *a5, HPEN *a6, COLORREF *a7,
						 COLORREF *a8)
{
  char path[264];

  sprintf(path, "%s\\WINBK_EndDuel.bmp", path_DuelArt);
  *a1 = read_graphics_file(path);
  *a2 = 0x1000040u;
  *a3 = 0x1000040u;
  if (!(*a4 = manalink_CreateSolidBrush(0x100001Au)))
    *a4 = manalink_GetStockObject(GRAY_BRUSH);
  if (!(*a5 = manalink_CreatePen(0, 0, 0x100008Cu)))
    *a5 = manalink_GetStockObject(WHITE_PEN);
  if (!(*a6 = manalink_CreatePen(0, 0, 0x1000001u)))
    *a6 = manalink_GetStockObject(BLACK_PEN);
  *a7 = 0x1000040u;
  *a8 = 0x10000BFu;
}
#undef manalink_CreatePen
#undef manalink_CreateSolidBrush
#undef manalink_GetStockObject
#undef read_graphics_file
#undef path_DuelArt

#define call_UnloadAllSnds				EXE_FN(int, 0x45a220, void)
#define TENTATIVE_finalize_sound_system	EXE_FN(int, 0x45a160, void)
#define path_base						EXE_STR(0x60ea00)
#define hwnd_shell						(*(HWND*)(0x715780))
int launch_mcu(void)
{
  char cmd_line[264];

  call_UnloadAllSnds();
  TENTATIVE_finalize_sound_system();
  game_type = 4;
  strcpy(cmd_line, path_base);
  strcat(cmd_line, "\\LaunchMCu.exe /MTGshell");
  int rval = WinExec(cmd_line, SW_SHOW);	// :(
  if (rval >= 0x20)
    rval = PostMessageA(hwnd_shell, WM_CLOSE, 0, 0);
  return rval;
}
#undef call_UnloadAllSnds
#undef TENTATIVE_finalize_sound_system
#undef path_base
#undef hwnd_shell

void make_smallcard_visible(int player, int card)
{
  if (ai_is_speculating == 1 || !in_play(player, card))
    return;

  HWND territory_hwnds[2] = { hwnd_TerritoryClass_player, hwnd_TerritoryClass_opponent };
  int i;
  for (i = 0; i < 2; ++i)
    {
      HWND terr = territory_hwnds[i];
      HWND* cards = (HWND*)(int)(GetWindowLong(terr, territoryclass_offset_card_hwnds));
      int numcards = GetWindowLong(terr, territoryclass_offset_card_hwnd_length);
	  int j;
      for (j = 0; j < numcards; ++j)
		if (!IsWindowVisible(cards[j])
			&& card == GetWindowLong(cards[j], cardclass_offset_card)
			&& player == GetWindowLong(cards[j], cardclass_offset_player))
		  {
			ShowWindow(cards[j], SW_SHOW);
			return;
		  }
    }
}

void draw_smallcard_activation_card(HDC hdc, RECT* card_rect, int csvidraw, int player, int card, int parent_player,
									int parent_card)
{
  if (!hdc && !card_rect)
	return;

  card_instance_t* dispinst = get_displayed_card_instance(player, card);
  card_instance_t inst;
  EnterCriticalSection(critical_section_for_display);
  memcpy(&inst, dispinst, sizeof(inst));
  LeaveCriticalSection(critical_section_for_display);

  if (csvidraw == 906 && (int)inst.original_internal_card_id == iid_draw_a_card)
	{
	  DrawCardBack(hdc, card_rect, 0);
	  DrawSmallCardTitle(hdc, card_rect, str_Activation, 0, 1, NULL, -1, -1);
	  return;
	}

  card_ptr_t cp;
  memset(&cp, 0, sizeof(cp));

  cp.id = inst.display_pic_csv_id;
  uint16_t version = inst.display_pic_num;

  static char name[128];
  if (csvidraw == 906)
	strcpy(name, str_Activation);
  else
	name[0] = 0;

  cp.name = name;
  cp.full_name = name;
  cp.expansion = -1;
  cp.color = -1;
  cp.card_type = -1;
  cp.subtype1 = -1;
  cp.subtype2 = -1;
  cp.db_card_type_2 = -1;

  static char emptystr[1];	// Probably useless, but definitely harmless
  emptystr[0] = 0;
  cp.rules_text = emptystr;
  cp.flavor_text = emptystr;

  DrawSmallCard(hdc, card_rect, &cp, version, 1, player, card);
  draw_manastripes(hdc, card_rect, parent_player, parent_card);

  int controlled_by_owner = !(inst.state & STATE_OWNED_BY_OPPONENT) == !player;
  uint32_t title_hilit = inst.unknown0x70;
  DrawSmallCardTitle(hdc, card_rect, cp.name, title_hilit, controlled_by_owner | 0x8, &cp, player, card);
}

void draw_smallcard_special_effect_card(HDC hdc, RECT* card_rect, int csvidraw, int player, int card)
{
  if (!hdc || !card_rect)
	return;

  card_ptr_t cp;
  memset(&cp, 0, sizeof(cp));

  card_instance_t* dispinst = get_displayed_card_instance(player, card);
  card_instance_t inst;
  EnterCriticalSection(critical_section_for_display);
  memcpy(&inst, dispinst, sizeof(inst));
  LeaveCriticalSection(critical_section_for_display);

  csvid_t src_csvid = inst.display_pic_csv_id;
  uint16_t version = inst.display_pic_num;

  static char card_name[100];	// Shandalar only assigns 52 bytes, so should be plenty.
  switch (csvidraw)
	{
	  case 901:;
		int l = sprintf(card_name, "%s: %d", str_Damage, inst.info_slot);
		color_test_t cols = inst.color & COLOR_TEST_ANY_COLORED;
		if (num_bits_set(cols) > 1)
		  strcpy(card_name + l, " (Multi)");
		else if (cols)
		  sprintf(card_name + l, " (%s)", strs_Colorless_Black_Blue_Green_Red_White[single_color_test_bit_to_color(cols)]);
		break;

	  case 902:
		strcpy(card_name, cards_txt[src_csvid].legacy_title_text);
		break;

	  case 903:;
		const char* src = cards_txt[src_csvid].effect_title_text;
		if (!src)
		  src = "";

		if (src_csvid == CARD_ID_FAERIE_DRAGON || src_csvid == CARD_ID_WHIMSY)
		  strcpy(card_name, src);
		else if (inst.eot_toughness > 0)
		  {
			strcpy(card_name, src);
			legacy_name(card_name, player, card);
		  }
		else
		  strcpy(card_name, src);

		if (!*card_name)
		  strcpy(card_name, cards_ptr[src_csvid]->name);
		break;

	  case 905:
		strcpy(card_name, get_card_or_subtype_name(inst.info_slot));
		break;

	  case 906:
		strcpy(card_name, str_Activation);
		break;

	  case 907:
		strcpy(card_name, str_MultiBlock_Creature);
		break;

	  default:
		card_name[0] = 0;
		break;
	}

  cp.id = src_csvid;
  cp.full_name = card_name;
  cp.name = card_name;
  cp.expansion = -1;
  cp.color = CP_COLOR_SPECIAL;
  cp.card_type = -1;
  cp.subtype1 = -1;
  cp.subtype2 = -1;
  cp.db_card_type_2 = -1;

  DrawSmallCard(hdc, card_rect, &cp, version, 73, player, card);

  draw_manastripes(hdc, card_rect, player, card);

  int controlled_by_owner = !(inst.state & STATE_OWNED_BY_OPPONENT) == !player;
  uint32_t title_hilit = inst.unknown0x70;

  DrawSmallCardTitle(hdc, card_rect, cp.name, title_hilit, controlled_by_owner | 0x8, &cp, player, card);

  if (csvidraw == 901)
    draw_abilities(hdc, card_rect, player, card, 1);
}

void draw_smallcard_normal(HDC hdc, RECT* card_rect, int player, int card, int unused_a5, int unused_a6,
						   int unused_a7, int force_to_activation, int force_to_upkeep)
{
  // unused_a5 is a redundantly-fetched copy of inst.unknown0x70.
  // unused_a6 is a redundantly-fetched 4 if inst is attacking and this card isn't being drawn in the attack window.
  // unused_a7 is a redundantly-fetched copy of inst.blocking if this card isn't being drawn in the attack window.
  // force_to_activation and force_to_upkeep are never set to nonzero.

  if (!hdc || !card_rect)
	return;

  int csvid = get_displayed_csvid(player, card);
  if (csvid == -1)
	return;

  int saved_dc = SaveDC(hdc);

  card_ptr_t cp;
  memcpy(&cp, cards_ptr[csvid], sizeof(card_ptr_t));

  if (player <= 1)
	cp.color = (((unsigned int)card & 0xFFFF) << 16) | (player ? 1 : 0) | 0xC3D4;	// magic number for drawcardlib

  if (force_to_activation)
	{
	  cp.name = cp.full_name = str_Activation;
	  cp.color = -1;
	}

  if (force_to_upkeep)
	{
	  cp.name = cp.full_name = str_Upkeep;
	  cp.color = -1;
	}

  int version = get_card_image_number(csvid, player, card);

  DrawSmallCard(hdc, card_rect, &cp, version, 1, player, card);

  if (1)	// (!(player & (TARGET_ZONE_GRAVEYARD | TARGET_ZONE_EXILE)))
	{
	  draw_manastripes(hdc, card_rect, player, card);

	  type_t typ = get_displayed_type(player, card);

	  if (option_ShowPowerToughnessOnCards && (typ & TYPE_CREATURE))
		{
		  int tgh = get_displayed_toughness(player, card);
		  int pow = get_displayed_power(player, card);
		  draw_powertoughness_on_smallcard(hdc, card_rect, pow, tgh);
		}

	  int bitfield = return_displayed_bitfield_of_1summonsick_2tapped_4attacking_8blocking_10damage_target_player_and_damage_target_card_20oublietted(player, card);
	  if (((typ & TYPE_CREATURE) || option_ShowAllCardsSummonSickness) && (bitfield & 1))
		blt_summon_pic(hdc, card_rect);

	  if (bitfield & 0x20)
		draw_oublietted_effect(hdc, card_rect);

	  if ((typ & TYPE_PERMANENT) && get_displayed_kill_code(player, card) == KILL_DESTROY)
		blt_dying_pic(hdc, card_rect);

      draw_abilities(hdc, card_rect, player, card, 0);

	  int dmg = get_displayed_damage_on_card(player, card);
	  if (dmg)
		draw_damage(hdc, card_rect, dmg);

	  RECT rect_counter;
	  rect_counter.left = 7 * (card_rect->right - card_rect->left) / 100 + card_rect->left;
	  rect_counter.right = card_rect->right - 10 * (card_rect->right - card_rect->left) / 100;
	  rect_counter.top = 8 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;
	  rect_counter.bottom = 35 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;

	  draw_special_counters(hdc, card_rect, player, card);

	  rect_counter.top = 35 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;
	  rect_counter.bottom = 62 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;

	  draw_standard_counters(hdc, &rect_counter, player, card);

	  int controlled_by_owner = get_displayed_owned_by_opponent(player, card) == player;
	  uint32_t title_hilit = get_displayed_unknown0x70(player, card);

	  DrawSmallCardTitle(hdc, card_rect, cp.name, title_hilit, controlled_by_owner | 0x8, &cp, player, card);

	  if (player == HUMAN && (get_displayed_state(0, card) & STATE_NO_AUTO_TAPPING))
		{
		  HGDIOBJ old_pen = SelectObject(hdc, pen_autotap_blue);
		  HGDIOBJ brush = GetStockObject(HOLLOW_BRUSH);
		  HGDIOBJ old_brush = SelectObject(hdc, brush);
		  Rectangle(hdc, card_rect->left, card_rect->top, card_rect->right, card_rect->bottom);
		  SelectObject(hdc, old_brush);
		  SelectObject(hdc, old_pen);
		}
	}

  RestoreDC(hdc, saved_dc);
}
