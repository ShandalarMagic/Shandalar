// -*- tab-width:8; c-basic-offset:2; -*-
// DeckDll: standalone and integrated deckbuilder.
// deckdll.c: primary interface to Magic.exe/Shandalar.exe

#ifdef USE_STD_MAP_AND_STRING	// generally unset, since it triples(!) the size of our build artifact
#include <map>
#include <string>

#include <boost/algorithm/string.hpp>
#endif

#include <ctype.h>

#include "deckdll.h"
#include "File.h"

//[[[ types and enums
OP_BITWISE(color_test_t);

typedef int (*Int_fn_int)(int);
typedef int (*Int_fn_etc)(...);

enum DBFlags
{
  DBFLAGS_0			= 0,
  DBFLAGS_SHANDALAR		= 1,
  DBFLAGS_STANDALONE		= 2,
  DBFLAGS_NOCARDCOUNTCHECK	= 4,
  DBFLAGS_GAUNTLET		= 8,
  DBFLAGS_EDITDECK		= 0x40,
};

enum DeckType
{
  DT_UNKNOWN		= 0,
  DT_UNRESTRICTED	= 0x1,
  DT_WILD		= 0x2,
  DT_RESTRICTED_T1	= 0x4,
  DT_TOURNAMENT_T1_5	= 0x8,
  DT_HIGHLANDER		= 0x10,
  DT_HAS_ANTE		= 0x20,
};
OP_BITWISE(DeckType);

enum FilterExpansions
{
  FE_0			= 0,
  FE_4TH_EDITION	= 0x2,
  FE_REVISED		= 0x20,
  FE_UNLIMITED		= 0x1000,
  FE_EXPANSIONLIST	= 0x8000,
};
OP_BITWISE(FilterExpansions);

enum FilterColors
{	// Using color_test_t would make too much sense.
  FC_0			= 0,
  FC_WHITE		= 0x0002,
  FC_GREEN		= 0x0004,
  FC_RED		= 0x0008,
  FC_BLACK		= 0x0010,
  FC_BLUE		= 0x0020,
  FC_GOLD		= 0x0040,
  FC_GOLD_ALL		= 0x0100,
  FC_GOLD_ALLSELECTED	= 0x0200,
  FC_GOLD_ANYSELECTED	= 0x0400,
};
OP_BITWISE(FilterColors);

enum FilterNum
{
  FN_0		= 0,
  FN_ENABLE	= 0x1,
  FN_GT		= 0x2,
  FN_LT		= 0x4,
  FN_EQ		= 0x8,
  FN_CC_X	= 0x10,
};
OP_BITWISE(FilterNum);

enum FilterRarities
{
  FR_0		= 0,
  FR_ENABLE	= 0x1,
  FR_COMMON	= 0x2,
  FR_UNCOMMON	= 0x4,
  FR_RARE	= 0x8,
  FR_RESTRICTED	= 0x10,
  FR_BANNED	= 0x20,
};
OP_BITWISE(FilterRarities);

enum FilterSets
{
  FS_0			= 0,
  FS_4TH_EDITION	= 0x0002,
  FS_ASTRAL		= 0x0004,
  FS_ARABIAN_NIGHTS	= 0x0008,
  FS_ANTIQUITIES	= 0x0010,
  FS_THE_DARK		= 0x0040,
  FS_LEGENDS		= 0x0100,
  FS_OTHER		= 0x1000,
};
OP_BITWISE(FilterSets);

enum FilterCardSetsFlags
{	// Seems to be FilterSets rightshifted by one.  How inconvenient.  FSCF_Q_* values are tentative.
  FCSF_0		= 0,
  FCSF_Q_ENABLE		= 0x0001,
  FCSF_Q_ASTRAL		= 0x0002,
  FCSF_ARABIAN_NIGHTS	= 0x0004,
  FCSF_ANTIQUITIES	= 0x0008,
  FCSF_THE_DARK		= 0x0010,
  FCSF_LEGENDS		= 0x0040,
  FCSF_Q_OTHER		= 0x0400,
};
OP_BITWISE(FilterCardSetsFlags);

enum FilterTypes
{
  FT_0				= 0,
  FT_LAND			= 0x00000001,
  FT_LAND_LAND_AND_MANA		= 0x00000002,
  FT_LAND_LAND_ONLY		= 0x00000004,
  FT_LAND_MANA_ONLY		= 0x00000008,
  FT_ARTIFACT			= 0x00000010,
  FT_ARTIFACT_CREATURE		= 0x00000020,
  FT_ARTIFACT_NON_CREATURE	= 0x00000040,
  FT_CREATURE			= 0x00000080,
  FT_CREATURE_CREATURE		= 0x00000100,
  FT_CREATURE_TOKEN		= 0x00000200,
  FT_CREATURE_ARTIFACT		= 0x00000400,
  FT_CREATURE_LIST		= 0x00000800,
  FT_ENCHANTMENT		= 0x00001000,
  FT_ENCHANTMENT_ENCHANTMENTS	= 0x00002000,
  FT_ENCHANTMENT_WORLD		= 0x00004000,
  FT_ENCHANTMENT_LAND		= 0x00008000,
  FT_ENCHANTMENT_CREATURE	= 0x00010000,
  FT_ENCHANTMENT_ARTIFACT	= 0x00020000,
  FT_ENCHANTMENT_ENCHANT	= 0x00040000,
  FT_ENCHANTMENT_PERMANENT	= 0x00080000,
  FT_ENCHANTMENT_PLAYER		= 0x00100000,
  FT_ENCHANTMENT_INSTANT	= 0x00200000,
  FT_INSTANT			= 0x00400000,
  FT_INTERRUPT			= 0x00800000,
  FT_SORCERY			= 0x01000000,
};
OP_BITWISE(FilterTypes);

enum FilterAbilities
{
  FA_0			= 0,
  FA_ENABLE		= 0x1,
  FA_NATIVE		= 0x2,
  FA_GRANTS		= 0x4,

  FA_FLYING		= 0x8,
  FA_FIRSTSTRIKE	= 0x10,	// and double strike
  FA_TRAMPLE		= 0x20,
  FA_REGENERATION	= 0x40,
  FA_BANDING		= 0x80,
  FA_PROTECTION		= 0x100,
  FA_LANDWALK		= 0x200,
  FA_INFECT		= 0x400,
  FA_RAMPAGE		= 0x800,
  FA_REACH		= 0x1000,
  FA_DEATHTOUCH		= 0x2000,
  FA_VIGILANCE		= 0x4000,
  FA_HASTE		= 0x8000,
};
OP_BITWISE(FilterAbilities);

enum Abilities
{
  ABIL_NATIVE_BANDING			= 1,
  ABIL_NATIVE_DESERTWALK		= 2,
  ABIL_NATIVE_FIRSTSTRIKE		= 3,	// or double strike
  ABIL_NATIVE_FLYING			= 4,
  ABIL_NATIVE_FORESTWALK		= 5,
  ABIL_NATIVE_VIGILANCE			= 6,
  ABIL_NATIVE_ISLANDWALK		= 7,
  ABIL_NATIVE_LEGENDARY_LANDWALK	= 8,
  ABIL_NATIVE_MOUNTAINWALK		= 9,
  ABIL_NATIVE_PLAINSWALK		= 10,
  ABIL_NATIVE_INFECT			= 11,
  ABIL_NATIVE_PROTECTION_FROM_BLACK	= 12,
  ABIL_NATIVE_PROTECTION_FROM_RED	= 13,
  ABIL_NATIVE_PROTECTION_FROM_WHITE	= 14,
  ABIL_NATIVE_HASTE			= 15,
  ABIL_NATIVE_RAMPAGE			= 16,
  ABIL_NATIVE_REGENERATION		= 17,
  ABIL_NATIVE_DEATHTOUCH		= 18,
  ABIL_NATIVE_SWAMPWALK			= 19,
  ABIL_NATIVE_TRAMPLE			= 20,
  ABIL_NATIVE_REACH			= 21,
  ABIL_GRANTS_BANDING			= 22,
  ABIL_GRANTS_FIRSTSTRIKE		= 23,	// or double strike
  ABIL_GRANTS_FLYING			= 24,
  ABIL_GRANTS_FORESTWALK		= 25,
  ABIL_GRANTS_VIGILANCE			= 26,
  ABIL_GRANTS_ISLANDWALK		= 27,
  ABIL_GRANTS_MOUNTAINWALK		= 28,
  ABIL_GRANTS_PLAINSWALK		= 29,
  ABIL_GRANTS_PROTECTION_FROM_ARTIFACTS	= 30,
  ABIL_GRANTS_PROTECTION_FROM_BLACK	= 31,
  ABIL_GRANTS_PROTECTION_FROM_BLUE	= 32,
  ABIL_GRANTS_PROTECTION_FROM_GREEN	= 33,
  ABIL_GRANTS_PROTECTION_FROM_RED	= 34,
  ABIL_GRANTS_PROTECTION_FROM_WHITE	= 35,
  ABIL_GRANTS_HASTE			= 36,
  ABIL_GRANTS_RAMPAGE			= 37,
  ABIL_GRANTS_REGENERATION		= 38,
  ABIL_GRANTS_DEATHTOUCH		= 39,
  ABIL_GRANTS_SWAMPWALK			= 40,
  ABIL_GRANTS_TRAMPLE			= 41,
  ABIL_GRANTS_REACH			= 42,
};

struct GlobalDeckEntry
{
  csvid_t GDE_csvid;
  iid_t GDE_iid;
  int GDE_Available;
  int GDE_DecksBits;
};

struct DeckEntry
{
  csvid_t DeckEntry_csvid;
  int DeckEntry_Amount;
  const char* DeckEntry_FullName;
};

enum OrigRaritySet: uint8_t
{
  SET_INVALID		= 0xFF,
  SET_ANTIQUITIES	= 0,
  SET_ARABIAN_NIGHTS	= 1,
  SET_ASTRAL		= 2,
  SET_THE_DARK		= 3,
  SET_LEGENDS		= 4,
  SET_PROMO		= 5,
  SET_UNLIMITED		= 6,
  SET_8TH_EDITION	= 7,
  SET_FALLEN_EMPIRES	= 8,
  SET_TEMPEST		= 9,
  SET_RANDOM		= 10
};

struct OrigRarities
{
  OrigRaritySet set;
  char rarity;
  char exp_rarities[4];
} PACKED;

enum Pack1
{	// Using color_t would make too much sense.
  PACK1_WHITE	= 0,
  PACK1_BLUE	= 1,
  PACK1_BLACK	= 2,
  PACK1_RED	= 3,
  PACK1_GREEN	= 4,
  PACK1_OTHER	= 5,
  PACK1_MAX	= PACK1_OTHER,
};

enum Pack2
{
  PACK2_LAND		= 0,
  PACK2_CREATURE	= 1,
  PACK2_ENCHANTMENT	= 2,
  PACK2_SORCERY		= 3,
  PACK2_INTERRUPT	= 4,
  PACK2_INSTANT		= 5,
  PACK2_MAX		= PACK2_INSTANT,
};

struct Packs
{
  struct Table
  {
    csvid_t csvid;
    int amt;
  };

  Table table[300];
  int num;

  void clear(void)	{ num = 0; }
  void add(csvid_t csvid, int amt)
  {
    table[num].csvid = csvid;
    table[num].amt = amt;
    ++num;
  }
};

struct Restriction
{
  enum Enum
    {
      RST_0		= 0x0,
      RST_ANTE		= 0x1,
      RST_RESTRICTED	= 0x2,
      RST_BANNED	= 0x4,
    };

  csvid_t csvid;
  Enum restriction;
};
OP_BITWISE(Restriction::Enum);

struct Sound
{	// very little idea what any of these fields do.
  int field_0;
  int field_4;
  int field_8;
  int field_C;
  int field_10;
  int field_14;
  int field_18;
  int field_1C;
};

enum SoundFn
{
  SND_InitSnd		= 0,
  SND_ReleaseSnd	= 1,
  SND_LoadSnd		= 2,
  SND_UnloadSnd		= 3,
  SND_UnloadAllSnds	= 4,
  SND_PlaySnd		= 5,
  SND_PlaySndFile	= 6,
  SND_StopSnd		= 7,
  SND_StopAllSnds	= 8,
  SND_PlayMidiFile	= 9,
  SND_SetPitch		= 10,
  SND_GetPitch		= 11,
  SND_SetVol		= 12,
  SND_GetVol		= 13,
  SND_SetPan		= 14,
  SND_GetPan		= 15,
  SND_UpdateSnd		= 16,
  SND_SetSndMarker	= 17,
  SND_PlaySndMarker	= 18,
  SND_GetSndTime	= 19,
  SND_ResetSnd		= 20,
  SND_GetSndState	= 21,
  SND_GetAVISndBuff	= 22,
  SND_ReleaseAVISndBuff	= 23,
  SND_GetSndHWND	= 24,
  SND_IsSndLoaded	= 25,
  SND_GetLRUSnd		= 26,
  SND_MAX		= SND_GetLRUSnd,
};

#define HORZLIST_LASTPIC_INDEX		0
#define HORZLIST_ADDR_INDEX		4
#define HORZLIST_CURRSEL_INDEX		8
#define HORZLIST_LEFTPIC_INDEX		12
#define HORZLIST_WIDTHPLUSSPACE_INDEX	16
#define HORZLIST_PICSINLINE_INDEX	20
#define HORZLIST_SIZE_INDEX		24

#define CARD_CSVID_INDEX	0
#define CARD_AMOUNT_INDEX	4

#define FULLCARD_CSVID_INDEX		0
#define FULLCARD_EXPANDTEXT_INDEX	4

#define CUECARD_FONT_INDEX	0
//]]]

//[[[ globals and constants
static HINSTANCE global_hinstance = NULL;

static CRITICAL_SECTION global_critical_section_for_unknown;
static CRITICAL_SECTION global_critical_section_for_drawing;

//  [[[ paths
static char global_base_directory[MAX_PATH + 1] = {0};
static char global_previous_directory[MAX_PATH + 1] = {0};
static char global_manalink_ini_path[MAX_PATH + 15] = {0};
static char global_playdeck_path[MAX_PATH + 15] = {0};
static char global_dbart_pattern[MAX_PATH + 45] = {0};
//  ]]]

static DBFlags global_db_flags_1 = DBFLAGS_0;
static DBFlags global_db_flags_2 = DBFLAGS_0;

//  [[[ configuration
static bool global_cfg_consolidate = false;
static bool global_cfg_effects = false;
static bool global_cfg_expand_text = false;
static bool global_cfg_music = false;
static bool global_cfg_no_frame = false;
static bool global_cfg_read_by_name = false;
static bool global_cfg_view_all = false;
#define MAX_NAMELEN 80
static char global_cfg_player_name[MAX_NAMELEN] = {0};
static char global_cfg_email[MAX_NAMELEN] = {0};
static char global_cfg_skin_name[MAX_PATH + 1] = {0};
//  ]]]

static int global_current_deck = 0;
static int* global_external_current_deck = NULL;

static int* global_external_deck = NULL;

static GlobalDeckEntry global_deck[1000];
static int global_deck_num_cards = 0;
static int global_deck_num_entries = 0;

static DeckEntry global_edited_deck[300];
static int global_edited_deck_num_entries = 0;

static DeckEntry global_excessive_cards[201];

static bool global_deck_was_edited = false;
static bool global_deckname_set = false;

static char global_deckname[32] = {0};
static char* global_external_deckname = NULL;

static char global_deck_author[81] = {0};
static char global_deck_comments[404] = {0};
static char global_deck_creation_date[22] = {0};
static char global_deck_description[21] = {0};
static char global_deck_edition[16] = {0};
static char global_deck_email[81] = {0};
static char global_deck_filename[(MAX_PATH + 15) + 32 + 5] = {0};
static int global_deck_revision = 1;

static int global_dlg_parameter = 0;
static int global_dlg_result = 0;

static char global_ask_x_dlg_title[128] = {0};
static char global_loaddeck_dlg_filename[2 * MAX_PATH + 15 + 10] = {0};

static bool global_filter_subtype_dlg_mode = false;
static char global_filter_dlg_title[128] = {0};
static int global_filter_gle_dlg_value = 0;

//  [[[ display
static HBRUSH global_brush_gold1 = NULL;
static HBRUSH global_brush_gold2 = NULL;
static HBRUSH global_brush_mediumgrey = NULL;

static COLORREF global_colorref_darkgrey = 0;
static COLORREF global_colorref_flesh = 0;
static COLORREF global_colorref_lavender = 0;
static COLORREF global_colorref_lightgrey = 0;
static COLORREF global_colorref_white = 0;

#define NUM_PICS 37
static HANDLE global_pics[NUM_PICS] = {0};

static HBITMAP global_hbmp = NULL;
static HGDIOBJ global_old_bmp_obj = NULL;

static HPALETTE global_palette = NULL;

static POINT global_smallcard_normal_size = {0, 0};
static POINT global_smallcard_smaller_size = {0, 0};
static POINT global_smallcard_smallest_size = {0, 0};

static int global_smallcard_piclist_height = 0;
static int global_smallcard_piclist_width = 0;

//    [[[ fonts
static LOGFONT global_logfont_template =
  {
    0,	// lfHeight
    0,	// lfWidth
    0,	// lfEscapement
    0,	// lfOrientation
    FW_NORMAL,	// lfWeight
    FALSE,	// lfItalic
    FALSE,	// lfUnderline
    FALSE,	// lfStrikeOut
    ANSI_CHARSET,	// lfCharSet
    OUT_DEFAULT_PRECIS,	// lfOutPrecision
    CLIP_DEFAULT_PRECIS,	// lfClipPrecision
    DEFAULT_QUALITY,	// lfQuality
    DEFAULT_PITCH|FF_DONTCARE,	// lfPitchAndFamily
    {
      'T', 'r', 'e', 'b', 'u', 'c', 'h', 'e', 't', ' ', 'M', 'S', '\0',
      '\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0','\0'
    }	// lfFaceName[LF_FACESIZE]
  };
static HFONT global_font_42 = NULL;
static HFONT global_font_42unused = NULL;
static HFONT global_font_40percent = NULL;
static HFONT global_font_28percent = NULL;
static HFONT global_font_32 = NULL;
//    ]]]

//    [[[ window handles, width/heights, dcs, menus
static HWND global_button_stats_hwnd = NULL;
static WNDPROC global_wndproc_std_ButtonClass = NULL;

static HWND global_cuecard_hwnd = NULL;

static HWND global_decksurface_hwnd = NULL;
static HMENU global_decksurface_popup = NULL;

static HMENU global_filtermenu_default = NULL;
static HMENU global_filtermenu_fourth = NULL;
static HMENU global_filtermenu_gold = NULL;
static HMENU global_filtermenu_land = NULL;
static HMENU global_filtermenu_artifact = NULL;
static HMENU global_filtermenu_creature = NULL;
static HMENU global_filtermenu_enchantment = NULL;
static HMENU global_filtermenu_castcost = NULL;
static HMENU global_filtermenu_power = NULL;
static HMENU global_filtermenu_toughness = NULL;
static HMENU global_filtermenu_ability = NULL;
static HMENU global_filtermenu_rarity = NULL;
static HMENU global_filtermenu_newexp = NULL;

// These are never set false.
static bool global_filtermenu_castcost_enabled = true;
static bool global_filtermenu_power_enabled = true;
static bool global_filtermenu_toughness_enabled = true;
static bool global_filtermenu_ability_enabled = true;
static bool global_filtermenu_rarity_enabled = true;
static bool global_filtermenu_gold_enabled = true;	// And this one isn't even used except for setting it true.

static HWND global_fullcard_hwnd = NULL;
static HMENU global_fullcard_popup = NULL;

static HWND global_horzlist_hwnd = NULL;
static HMENU global_horzlist_popup = NULL;

static HWND global_listbox_hwnd = NULL;

static HWND global_main_hwnd = NULL;
static int global_main_window_width = 0;
static int global_main_window_height = 0;

static WNDPROC global_wndproc_std_SearchClass = NULL;

static int global_smallcard_height = 0;
static int global_smallcard_width = 0;
static HMENU global_smallcard_popup = NULL;

static HWND global_title_hwnd = NULL;

static HDC global_hdc = NULL;
static HDC global_screen_dc = NULL;
//    ]]]
//  ]]]

//  [[[ filters
static FilterExpansions global_filter_expansions = FE_0;
static FilterColors global_filter_colors = FC_0;
static FilterSets global_filter_cardsets = FS_0;
static FilterAbilities global_filter_abilities = FA_0;
static FilterTypes global_filter_cardtypes = FT_0;
static FilterCardSetsFlags global_filter_cardsets_flags = FCSF_0;

static FilterNum global_filter_casting_cost = FN_0;
static int global_filter_casting_cost_value = 0;

static FilterNum global_filter_power = FN_0;
static int global_filter_power_value = 0;
static FilterNum global_filter_toughness = FN_0;
static int global_filter_toughness_value = 0;

static FilterRarities global_filter_rarity = FR_0;

#define CREATURE_LIST_SIZE	10
// 1 bit per creature type, so 10 means a maximum of 320.  The highest used is currently 0xEA, for SUBTYPE_MOLE.
STATIC_ASSERT((CREATURE_LIST_SIZE * 32) > SUBTYPE_MAX_USED_CREATURE_SUBTYPE, Too_Many_Creature_Types);
static uint32_t global_filter_creature_list[CREATURE_LIST_SIZE] = {0};

#define EXPANSION_LIST_SIZE	8
// 1 bit per expansion, so 8 means a maximum of 256.  We currently have 159, including 8 "Format" expansions at the start and 8 "Future Expansion" at the end.
static uint32_t global_filter_expansion_list[EXPANSION_LIST_SIZE] = {0};

// And for both, they're limited by constants in dlgproc_FilterSubtype() to 320 entries.
#define MAX_FILTER_SUBTYPE_SIZE	320
#if ((CREATURE_LIST_SIZE * 32) > MAX_FILTER_SUBTYPE_SIZE)
#  error "sizeof global_filter_creature_list > MAX_FILTER_SUBTYPE_SIZE"
#endif
#if ((EXPANSION_LIST_SIZE * 32) > MAX_FILTER_SUBTYPE_SIZE)
#  error "sizeof global_filter_expansion_list > MAX_FILTER_SUBTYPE_SIZE"
#endif
//  ]]]

static char global_search_string[264] = {0};

//  [[[ sound
static HMODULE global_hmodule_magsnd_dll = NULL;
static int global_sound_status = 0;	// 0 = not loaded, 1 = loaded and ok, 2 = loaded and error I think

static bool global_sound_unk1 = false, global_sound_unk2 = false;

static Int_fn_etc global_sound_fns[SND_MAX + 1] = {0};
//  ]]]

static Packs global_packs[PACK1_MAX+1][PACK2_MAX+1] = {};
static Packs global_packs_copy[PACK1_MAX+1][PACK2_MAX+1] = {};

static char text_lines[500][128];

//  [[[ card database
static int global_available_slots = 0;
static int global_num_expansions = 0;
static int global_expansion_size = 0;
static const card_ptr_t* cards_ptr = NULL;
static card_ptr_t* global_raw_cards_ptr = NULL;
static char* card_coded = NULL;
static char* global_base_txt = NULL;
static char* global_raw_dbinfo = NULL;
static char* global_raw_rarities = NULL;
static OrigRarities* global_origrarities = NULL;
static bool global_db_read = false;
//  ]]]

//  [[[ low-level interface to Shandalar.dll
static bool global_using_main_db = false;
static int (*global_check_card_count_fn)(const DeckEntry*, int, int);
static int (*global_is_valid_card_fn)(int);
static bool (*global_colors_match_fn)(iid_t, color_test_t);
static int (*global_check_colors_inout_edited_deck_fn)(const GlobalDeckEntry*, int, bool);
//  ]]]

int* Gold = NULL;
shandalar_worldmagic_t* Scards = NULL;
// Why are these exported from here?  And with illegal names, no less?
int _PlayerFace;
int _OpponFace;

static Int_fn_int CardIDFromType = NULL;
static Int_fn_int CardTypeFromID = NULL;
static Int_fn_int SellPrice = NULL;

const Restriction restrictions[] =
  {
    {CARD_ID_AMULET_OF_QUOZ,		Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_BRONZE_TABLET,		Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_CONTRACT_FROM_BELOW,	Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_DARKPACT,			Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_DEMONIC_ATTORNEY,		Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_JEWELED_BIRD,		Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_REBIRTH,			Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_TEMPEST_EFREET,		Restriction::RST_ANTE | Restriction::RST_BANNED},
    {CARD_ID_TIMMERIAN_FIENDS,		Restriction::RST_ANTE | Restriction::RST_BANNED},

    {CARD_ID_ANCESTRAL_RECALL,		Restriction::RST_RESTRICTED},
    {CARD_ID_BALANCE,			Restriction::RST_RESTRICTED},
    {CARD_ID_BLACK_LOTUS,		Restriction::RST_RESTRICTED},
    {CARD_ID_BRAINSTORM,		Restriction::RST_RESTRICTED},
    {CARD_ID_CHALICE_OF_THE_VOID,	Restriction::RST_RESTRICTED},
    {CARD_ID_CHANNEL,			Restriction::RST_RESTRICTED},
    {CARD_ID_DEMONIC_CONSULTATION,	Restriction::RST_RESTRICTED},
    {CARD_ID_DEMONIC_TUTOR,		Restriction::RST_RESTRICTED},
    {CARD_ID_DIG_THROUGH_TIME,		Restriction::RST_RESTRICTED},
    {CARD_ID_FASTBOND,			Restriction::RST_RESTRICTED},
    {CARD_ID_FLASH,			Restriction::RST_RESTRICTED},
    {CARD_ID_IMPERIAL_SEAL,		Restriction::RST_RESTRICTED},
    {CARD_ID_LIBRARY_OF_ALEXANDRIA,	Restriction::RST_RESTRICTED},
    {CARD_ID_LIONS_EYE_DIAMOND,		Restriction::RST_RESTRICTED},
    {CARD_ID_LODESTONE_GOLEM,		Restriction::RST_RESTRICTED},
    {CARD_ID_LOTUS_PETAL,		Restriction::RST_RESTRICTED},
    {CARD_ID_MANA_CRYPT,		Restriction::RST_RESTRICTED},
    {CARD_ID_MANA_VAULT,		Restriction::RST_RESTRICTED},
    {CARD_ID_MEMORY_JAR,		Restriction::RST_RESTRICTED},
    {CARD_ID_MERCHANT_SCROLL,		Restriction::RST_RESTRICTED},
    {CARD_ID_MINDS_DESIRE,		Restriction::RST_RESTRICTED},
    {CARD_ID_MOX_EMERALD,		Restriction::RST_RESTRICTED},
    {CARD_ID_MOX_JET,			Restriction::RST_RESTRICTED},
    {CARD_ID_MOX_PEARL,			Restriction::RST_RESTRICTED},
    {CARD_ID_MOX_RUBY,			Restriction::RST_RESTRICTED},
    {CARD_ID_MOX_SAPPHIRE,		Restriction::RST_RESTRICTED},
    {CARD_ID_MYSTICAL_TUTOR,		Restriction::RST_RESTRICTED},
    {CARD_ID_NECROPOTENCE,		Restriction::RST_RESTRICTED},
    {CARD_ID_PONDER,			Restriction::RST_RESTRICTED},
    {CARD_ID_SOL_RING,			Restriction::RST_RESTRICTED},
    {CARD_ID_STRIP_MINE,		Restriction::RST_RESTRICTED},
    {CARD_ID_TIME_VAULT,		Restriction::RST_RESTRICTED},
    {CARD_ID_TIME_WALK,			Restriction::RST_RESTRICTED},
    {CARD_ID_TIMETWISTER,		Restriction::RST_RESTRICTED},
    {CARD_ID_TINKER,			Restriction::RST_RESTRICTED},
    {CARD_ID_TOLARIAN_ACADEMY,		Restriction::RST_RESTRICTED},
    {CARD_ID_TREASURE_CRUISE,		Restriction::RST_RESTRICTED},
    {CARD_ID_TRINISPHERE,		Restriction::RST_RESTRICTED},
    {CARD_ID_VAMPIRIC_TUTOR,		Restriction::RST_RESTRICTED},
    {CARD_ID_WHEEL_OF_FORTUNE,		Restriction::RST_RESTRICTED},
    {CARD_ID_WINDFALL,			Restriction::RST_RESTRICTED},
    {CARD_ID_YAWGMOTHS_BARGAIN,		Restriction::RST_RESTRICTED},
    {CARD_ID_YAWGMOTHS_WILL,		Restriction::RST_RESTRICTED},

    {CARD_ID_CHAOS_ORB,			Restriction::RST_BANNED},
    {CARD_ID_FALLING_STAR,		Restriction::RST_BANNED},
    {CARD_ID_SHAHRAZAD,			Restriction::RST_BANNED},

    {CARD_ID_CONSPIRACY_ADVANTAGEOUS_PROCLAMATION,	Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_BACKUP_PLAN,			Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_BRAGOS_FAVOR,			Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_DOUBLE_STROKE,			Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_IMMEDIATE_ACTION,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_ITERATIVE_ANALYSIS,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_MUZZIOS_PREPARATIONS,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_POWER_PLAY,			Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_SECRETS_OF_PARADISE,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_SECRET_SUMMONING,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_SENTINEL_DISPATCH,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_UNEXPECTED_POTENTIAL,		Restriction::RST_BANNED},
    {CARD_ID_CONSPIRACY_WORLDKNIT,			Restriction::RST_BANNED},

    {CARD_ID_AIR_ELEMENTAL,		Restriction::RST_0},
  };

#define DASH "\227"
//]]]

//[[[ macros and forward declarations
#define FREEZ(memory)		\
	do {			\
	  if (memory)		\
	    {			\
	      free(memory);	\
	      memory = NULL;	\
	    }			\
	} while (0)

#define DELETE_IMPL(fn, obj)	\
	do {			\
	  if (obj)		\
	    {			\
	      fn(obj);		\
	      obj = NULL;	\
	    }			\
	} while (0)

#define DELETE_DC(obj)		DELETE_IMPL(DeleteDC, obj)
#define DELETE_OBJ(obj)		DELETE_IMPL(DeleteObject, obj)
#define DESTROY_MENU(obj)	DELETE_IMPL(DestroyMenu, obj)

#define CHECKMENU_IF(menu, cmd, val)	CheckMenuItem((menu), (cmd), MF_BYCOMMAND | ((val) ? MF_CHECKED : MF_UNCHECKED))

// only used to choose which background music to play, so that's ok.  Evaluates lo twice.
#define RANDRANGE(lo, hi)	(static_cast<int>((static_cast<double>(rand()) / (static_cast<double>(RAND_MAX) + 1.0)) * ((hi) - (lo) + 1) + (lo)))

#define RECT_WIDTH(which) ((which).right - (which).left)
#define RECT_HEIGHT(which) ((which).bottom - (which).top)

static bool check_filters(csvid_t csvid);

LRESULT CALLBACK wndproc_CardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_CardListFilterClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_CueCardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_DeckSurfaceClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_FullCardClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_HorzListClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_MainClass(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK wndproc_TitleClass(HWND, UINT, WPARAM, LPARAM);
//]]]

//[[[ utils and widely-called functions
static void fatal_err(const char* text, HWND hwnd = NULL);
static void
fatal_err(const char* text, HWND hwnd)
{
  MessageBox(hwnd, text, 0, MB_SYSTEMMODAL|MB_ICONERROR);

  if (hwnd)
    SendMessage(hwnd, WM_CLOSE, 0, 0);
  else
    PostQuitMessage(1);
}

void
fatal(const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  fatal_err(buf);
  exit(1);
}

static void popup(const char* fmt, ...) __attribute__((unused,format(printf, 1, 2)));
static void
popup(const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  MessageBox(NULL, buf, 0, MB_SYSTEMMODAL|MB_ICONWARNING);
}

static void logf(const char* fmt, ...) __attribute__((unused,format(printf, 1, 2)));
static void
logf(const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

#if 0
  if (!deck_dll_log)
    deck_dll_log = new File("deck_dll.log", "w");

  deck_dll_log->printf("%s\n", buf);
  deck_dll_log->flush();
#endif

  fprintf(stderr, "%s\n", buf);
  fflush(stderr);
}

static int
load_text(const char* file_name, const char* section_name)
{
  char path[MAX_PATH];
  strcpy(path, file_name);
  if (!strchr(path, '.'))
    strcat(path, ".txt");

  File f(path, "rt");
  if (!f.ok())
    return -1;

  char section_line[160];
  sprintf(section_line, "@%s", section_name);

  char line[160];
  do
    {
      if (!f.readline(line, 160))
	return -1;
    } while (strcmp(line, section_line));

  f.readline(line, 160);
  int num_text = atoi(line);
  if (num_text < 0 || num_text >= 500)
    return -1;

  for (int i = 0; i < num_text; ++i)
    if (!f.readline(text_lines[i], 128))
      return -1;

  return num_text;
}

static void
textout(HDC hdc, int x, int y, const char* txt)
{
  TextOut(hdc, x, y, txt, strlen(txt));
}

static void textoutf(HDC hdc, int x, int y, const char* fmt, ...) __attribute__((format(printf, 4, 5)));
static void
textoutf(HDC hdc, int x, int y, const char* fmt, ...)
{
  char buf[512];
  va_list args;
  va_start(args, fmt);
  int len = vsnprintf(buf, 512, fmt, args);
  va_end(args);
  TextOut(hdc, x, y, buf, len);
}

static int
popup_loaded(HWND hwnd, const char* title, const char* msg, int msgbox_params = MB_ICONINFORMATION)
{
  char txt[128];
  load_text("Menus", msg);
  strcpy(txt, text_lines[0]);

  load_text("Menus", title);
  return MessageBox(hwnd, txt, text_lines[0], msgbox_params);
}

static int
popup_loaded(HWND hwnd, const char* title, const char* msg, const char* sprintf_arg, int msgbox_params = MB_ICONINFORMATION)
{
  char txt[128 + strlen(sprintf_arg)];
  load_text("Menus", msg);
  sprintf(txt, text_lines[0], sprintf_arg);

  load_text("Menus", title);
  return MessageBox(hwnd, txt, text_lines[0], msgbox_params);
}

static void
gdi_flush(HDC hdc)
{
  SelectPalette(hdc, global_palette, 0);
  RealizePalette(hdc);
  GdiFlush();
  SetStretchBltMode(hdc, COLORONCOLOR);
}

static void
draw_background(HDC hdc, RECT* r, HANDLE hbmp)
{
  if (!hdc || !r || !hbmp)
    return;

  BITMAP bmp;
  GetObject(hbmp, sizeof(BITMAP), &bmp);

  EnterCriticalSection(&global_critical_section_for_drawing);

  HGDIOBJ old_obj = SelectObject(global_screen_dc, hbmp);

  gdi_flush(global_screen_dc);

  StretchBlt(hdc,
	     r->left,
	     r->top,
	     (r->right < r->left) ? bmp.bmWidth : r->right - r->left,
	     (r->bottom < r->top) ? bmp.bmHeight : r->bottom - r->top,
	     global_screen_dc,
	     0,
	     0,
	     bmp.bmWidth,
	     bmp.bmHeight,
	     SRCCOPY);

  SelectObject(global_screen_dc, old_obj);

  LeaveCriticalSection(&global_critical_section_for_drawing);
}

static void
set_dlg_text(HWND hdlg, int resource, const char* txt)
{
  SetWindowText(GetDlgItem(hdlg, resource), txt);
}

static void
set_dlg_text(HWND hdlg, int resource, const char* txt, int limit)
{
  HWND item = GetDlgItem(hdlg, resource);
  SetWindowText(item, txt);
  SendMessage(item, EM_SETLIMITTEXT, limit, 0);
}
//]]]

//[[[ cards.dat
static void
translate_backslash_n_to_newline(char* str)
{
  for (char* p = str;; ++p, ++str)
    switch (*str)
      {
	case '\0':
	  *p = 0;
	  return;

	case '\\':
	  if (*(str + 1) == 'n' || *(str + 1) == 'N')
	    {
	      *p = '\n';
	      ++str;
	      break;
	    }
	  // else fall through

	default:
	  *p = *str;
	  break;
      }
}

static inline unsigned int
num_bits_set(unsigned int v)
{
  // From Bit Twiddling Hacks, http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
  v = v - ((v >> 1) & 0x55555555);				// reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);		// temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;	// count
}

static int
read_db_guts(void)
{
  global_db_read = true;

  int record_size;
  if (!global_using_main_db)
    {
      {
	File cards_dat("Cards.dat");
	if (!cards_dat.ok())
	  return 0;

	cards_dat.READ(&global_available_slots, 4);
	cards_dat.READ(&record_size, 4);

	if (!(global_raw_cards_ptr = (card_ptr_t*)malloc(152 * global_available_slots))
	    || !(card_coded = (char*)malloc((global_available_slots + 7) >> 3))
	    || !(global_base_txt = (char*)malloc(record_size)))
	  return 0;

	cards_dat.READ(global_raw_cards_ptr, 152 * global_available_slots);
	cards_dat.READ(global_base_txt, record_size);

	cards_dat.READ(&record_size, 4);
	if (record_size > ((global_available_slots + 7) >> 3))
	  {
	    fatal_err("Fatal error: inconsistent available_slots and card_coded record_size in Cards.dat");
	    return 0;
	  }
	cards_dat.READ(card_coded, record_size);
      }

      {
	bool is_mana_letter[256] = {0};
	is_mana_letter['B'] = true;
	is_mana_letter['U'] = true;
	is_mana_letter['G'] = true;
	is_mana_letter['R'] = true;
	is_mana_letter['W'] = true;

	static char nul = 0;

	unsigned int offset = (unsigned int)global_base_txt;

	card_ptr_t* cp = global_raw_cards_ptr;
	for (int i = 0; i < global_available_slots; ++i, ++cp)
	  {
	    cp->full_name += offset;
	    cp->name += offset;
	    cp->type_text += offset;
	    cp->artist = "None";
	    if (cp->mana_cost_text)
	      cp->mana_cost_text += offset;
	    cp->rules_text += offset;
	    cp->flavor_text += offset;

	    if (!cp->rules_text
		|| !strcasecmp(cp->rules_text, "None"))
	      cp->rules_text = &nul;

	    if (!cp->flavor_text
		|| !strcasecmp(cp->flavor_text, "None")
		|| !strcasecmp(cp->flavor_text, "Blank"))
	      cp->flavor_text = &nul;

	    if (!cp->num_pics)
	      cp->num_pics = 1;

	    cp->req_hybrid = 0;
	    cp->hybrid_type = HYBRID_0;

	    if (cp->color == CP_COLOR_MULTI && cp->mana_cost_text)
	      {
		for (const char* p = cp->mana_cost_text; *p; ++p)
		  if (p[0] == '|' && is_mana_letter[static_cast<unsigned char>(p[1])] && is_mana_letter[static_cast<unsigned char>(p[2])])
		    {
		      color_test_t cols = COLOR_TEST_0;
		      for (int j = 1; j <= 2; ++j)
			switch (p[j])
			  {
			    case 'B':	cols |= COLOR_TEST_BLACK;	if (cp->req_black > 0)	--cp->req_black;	break;
			    case 'U':	cols |= COLOR_TEST_BLUE;	if (cp->req_blue > 0)	--cp->req_blue;		break;
			    case 'G':	cols |= COLOR_TEST_GREEN;	if (cp->req_green > 0)	--cp->req_green;	break;
			    case 'R':	cols |= COLOR_TEST_RED;		if (cp->req_red > 0)	--cp->req_red;		break;
			    case 'W':	cols |= COLOR_TEST_WHITE;	if (cp->req_white > 0)	--cp->req_white;	break;
			    default:
			      fatal("%s: can't parse hybrid mana cost (\"%s\"->\"%c%c%c\")", cp->full_name, cp->mana_cost_text, p[0], p[1], p[2]);
			      break;
			  }

		      // This works only because all hybrid_t values exactly match one of the subset of color_test_t's that can be constructed above
		      hybrid_t hy = static_cast<hybrid_t>(cols);

		      if (++cp->req_hybrid == 1)
			{
			  if ((cols & ~COLOR_TEST_ANY_COLORED)
			      || num_bits_set(cols & COLOR_TEST_ANY_COLORED) != 2)
			    fatal("%s: invalid hybrid mana cost (\"%s\"->\"%c%c%c\", 0x%x)", cp->full_name,
				  cp->mana_cost_text, p[0], p[1], p[2], cols);
			  cp->hybrid_type = hy;
			}
		      else if (cp->hybrid_type != hy)
			fatal("%s: mixed hybrid mana cost (\"%s\"->\"%c%c%c\", 0x%x, previously 0x%x)", cp->full_name,
			      cp->mana_cost_text, p[0], p[1], p[2], hy, cp->hybrid_type);

		      p += 2;
		    }
	      }
	  }

	for (int i = 0; i < global_available_slots; ++i)
	  {
	    cp = &global_raw_cards_ptr[i];
	    translate_backslash_n_to_newline(cp->rules_text);
	    translate_backslash_n_to_newline(cp->flavor_text);
	  }
      }

      cards_ptr = global_raw_cards_ptr;
    }

  {
    File dbinfo_dat("DBInfo.dat");
    if (!dbinfo_dat.ok())
      return 0;

    dbinfo_dat.READ(&record_size, 4);
    if (record_size != global_available_slots)
      return 0;

    int raw_dbinfo_size = 16 * record_size;
    if (!(global_raw_dbinfo = (char*)malloc(raw_dbinfo_size)))
      return 0;

    dbinfo_dat.READ(global_raw_dbinfo, raw_dbinfo_size);
  }

  {
    File rarity_dat("Rarity.dat");
    if (!rarity_dat.ok())
      return 0;

    rarity_dat.READ(&record_size, 4);
    if (record_size != global_available_slots)
      return 0;

    rarity_dat.READ(&global_num_expansions, 4);
    if (global_num_expansions >= 32 * EXPANSION_LIST_SIZE)
      fatal("Too many expansions\nRead %d from rarity.dat\nThis versions of DeckDll.dll supports only %d", global_num_expansions, 32 * EXPANSION_LIST_SIZE - 1);

    global_expansion_size = (3 * global_num_expansions + 7) >> 3;
    int raw_rarities_size = record_size * global_expansion_size;
    if (!(global_raw_rarities = (char*)malloc(raw_rarities_size + 4)))
      return 0;

    rarity_dat.READ(global_raw_rarities, raw_rarities_size);
  }

  return global_available_slots;
}

static void
free_db(void)
{
  FREEZ(global_raw_rarities);
  FREEZ(global_raw_dbinfo);

  if (!global_using_main_db)
    {
      FREEZ(global_base_txt);
      FREEZ(global_raw_cards_ptr);
      FREEZ(card_coded);
    }

  cards_ptr = NULL;
}

static int
read_db(void)
{
  int rval = read_db_guts();
  if (!rval)
    free_db();
  return rval;
}

static void
make_orig_rarities(void)
{
  global_origrarities = (OrigRarities*)malloc(sizeof(OrigRarities) * global_available_slots);

  for (int i = 0; i < global_available_slots; ++i)
    {
      const card_ptr_t* cp = &cards_ptr[i];
      OrigRarities* r = &global_origrarities[i];

      OrigRaritySet rs;
      uint32_t ex = cp->expansion;

      if (ex & 2)
	rs = SET_ANTIQUITIES;
      else if (ex & 4)
	rs = SET_ARABIAN_NIGHTS;
      else if (ex & 8)
	rs = SET_ASTRAL;
      else if (ex & 0x20)
	rs = SET_THE_DARK;
      else if (ex & 0x100)
	rs = SET_LEGENDS;
      else if (ex & 0x200)
	rs = SET_PROMO;
      else if (ex & 0x800)
	rs = SET_UNLIMITED;
      else if ((ex & 0x40) || ((ex & 0x1000) && global_cfg_view_all))
	rs = SET_8TH_EDITION;
      else if (ex & 0x10)
	rs = SET_FALLEN_EMPIRES;
      else if (ex & 0x80)
	rs = SET_TEMPEST;
      else if (ex & 0x400)
	rs = SET_RANDOM;
      else
	{
	  r->set = SET_INVALID;
	  r->rarity = 0;
	  r->exp_rarities[0] = '-';
	  r->exp_rarities[1] = '-';
	  r->exp_rarities[2] = '-';
	  r->exp_rarities[3] = '-';
	  continue;
	}

      r->set = rs;

      switch (BYTE0(cp->rarity))
	{
	  case 1:	r->rarity = 'C';	break;
	  case 4:	r->rarity = 'U';	break;
	  case 2:	r->rarity = 'R';	break;
	  default:	r->rarity = '-';	break;
	}

      uint32_t expr = cp->expansion_rarity;
      for (int j = 0; j < 4; ++j, expr >>= 4)
	{
	  int xr = expr & 0xf;

	  if (!xr || xr > 9)
	    r->exp_rarities[j] = '-';
	  else if (xr < 5)
	    r->exp_rarities[j] = 'U';
	  else if (xr < 9)
	    r->exp_rarities[j] = 'C';
	  else
	    r->exp_rarities[j] = 'R';
	}
    }
}
//]]]

//[[[ config file
static int
config_get_str(int size, const char* keyname, const char* deflt, char* rval)
{
  return GetPrivateProfileString("DeckBuilder", keyname, deflt, rval, size, global_manalink_ini_path);
}

static int
config_get_int(int deflt, const char* keyname)
{
  return GetPrivateProfileInt("DeckBuilder", keyname, deflt, global_manalink_ini_path);
}

static void
read_manalink_ini(void)
{
  global_cfg_consolidate = config_get_int(1, "Consolidate");
  global_cfg_music = config_get_int(1, "Music");
  global_cfg_effects = config_get_int(1, "Effects");
  global_cfg_no_frame = config_get_int(0, "NoFrame");
  global_cfg_expand_text = config_get_int(0, "ExpandTextOnBigCard");
  global_cfg_read_by_name = config_get_int(0, "ReadByName");
  global_cfg_view_all = config_get_int(0, "ViewAll");
  config_get_str(MAX_NAMELEN, "Name", "User", global_cfg_player_name);
  config_get_str(MAX_NAMELEN, "Email", "User E-Mail", global_cfg_email);
  config_get_str(MAX_PATH, "Skin", ".", global_cfg_skin_name);
}

static void
cfg_write_str(const char* str, const char* keyname)
{
  WritePrivateProfileString("DeckBuilder", keyname, str, global_manalink_ini_path);
}

static void
cfg_write_int(int val, const char* keyname)
{
  char buf[256];
  sprintf(buf, "%d", val);
  cfg_write_str(buf, keyname);
}

static void
write_manalink_ini(void)
{
  cfg_write_int(global_cfg_consolidate ? 1 : 0, "Consolidate");
  cfg_write_int(global_cfg_music ? 1 : 0, "Music");
  cfg_write_int(global_cfg_effects ? 1 : 0, "Effects");
  cfg_write_int(global_cfg_no_frame ? 1 : 0, "NoFrame");
  cfg_write_int(global_cfg_expand_text ? 1 : 0, "ExpandTextOnBigCard");
  cfg_write_int(global_cfg_read_by_name ? 1 : 0, "ReadByName");
  cfg_write_int(global_cfg_view_all ? 1 : 0, "ViewAll");
  cfg_write_str(global_cfg_skin_name[0] ? global_cfg_skin_name : ".", "Skin");
}
//]]]

//[[[ entry points/initialization/cleanup
void
Deckdll_initialize_for_shandalar(const card_ptr_t* i_raw_cards_ptr,
				 int i_available_slots,
				 char* i_card_coded,
				 int (*check_card_count_fn)(const DeckEntry*, int, int),
				 int (*is_valid_card_fn)(int),
				 bool (*colors_match_fn)(iid_t, color_test_t),
				 int (*check_colors_inout_edited_deck_fn)(const GlobalDeckEntry*, int, bool))
{
  if (i_available_slots <= 0)
    return;

  cards_ptr = i_raw_cards_ptr;
  global_available_slots = i_available_slots;
  card_coded = i_card_coded;
  global_check_card_count_fn = check_card_count_fn;
  global_is_valid_card_fn = is_valid_card_fn;
  global_colors_match_fn = colors_match_fn;
  global_check_colors_inout_edited_deck_fn = check_colors_inout_edited_deck_fn;

  global_using_main_db = true;
}

static void
delete_brushes(void)
{
  DELETE_OBJ(global_brush_gold1);
  DELETE_OBJ(global_brush_gold2);
  DELETE_OBJ(global_brush_mediumgrey);
}

static bool
create_brushes(void)
{
  global_colorref_white = PALETTEINDEX(191);	// 254, 254, 254
  global_colorref_darkgrey = PALETTEINDEX(1);	// 26, 24, 28
  global_colorref_flesh = PALETTERGB(205, 176, 143);
  global_colorref_lavender = PALETTERGB(174, 178, 239);
  global_colorref_lightgrey = PALETTERGB(247, 247, 246);
  global_brush_mediumgrey = CreateSolidBrush(PALETTEINDEX(134));	// 144, 143, 142
  global_brush_gold1 = CreateSolidBrush(PALETTERGB(128, 128, 80));
  global_brush_gold2 = CreateSolidBrush(PALETTERGB(128, 128, 80));
  if (global_brush_gold1 && global_brush_gold2 && global_brush_mediumgrey)
    return true;
  else
    {
      delete_brushes();
      return false;
    }
}

static void
delete_fonts(void)
{
  DELETE_OBJ(global_font_42);
  DELETE_OBJ(global_font_40percent);
  DELETE_OBJ(global_font_28percent);
  DELETE_OBJ(global_font_42unused);
  DELETE_OBJ(global_font_32);
}

static void
create_fonts(void)
{
  LOGFONT logfont;
  memcpy(&logfont, &global_logfont_template, sizeof(LOGFONT));

  logfont.lfHeight = 42;
  global_font_42 = CreateFontIndirect(&logfont);

  logfont.lfHeight = 38;
  global_font_40percent = CreateFontIndirect(&logfont);

  logfont.lfHeight = 28;
  global_font_28percent = CreateFontIndirect(&logfont);

  logfont.lfHeight = 42;
  global_font_42unused = CreateFontIndirect(&logfont);

  logfont.lfHeight = 32;
  global_font_32 = CreateFontIndirect(&logfont);
}

static void
delete_screen_dc(void)
{
  if (global_screen_dc)	// Check not redundant with DELETE_DC() because of the DeleteCriticalSection call
    {
      DELETE_DC(global_screen_dc);
      DeleteCriticalSection(&global_critical_section_for_drawing);
    }
}

static bool
create_screen_dc(void)
{
  if (!global_screen_dc)
    {
      HDC hdc = GetDC(NULL);
      global_screen_dc = CreateCompatibleDC(hdc);
      ReleaseDC(NULL, hdc);
      InitializeCriticalSection(&global_critical_section_for_drawing);
    }

  return global_screen_dc;
}

static void
delete_pics(void)
{
  for (int i = 0; i < NUM_PICS; ++i)
    DELETE_OBJ(global_pics[i]);
}

static const char*
load_pics(void)
{
  const char* picnames[NUM_PICS] =
    {
      "Ability",	"Antiquit",	"Arabnite",	"Artifact",
      "Astral",		"Black",	"Bldr01c",	"Bldr02c",
      "Bldr03c",	"Bldr04c",	"Bldr05c",	"Blue",
      "CastCost",	"Creature",	"Dark",		"Dekbar1",
      "Dektile4",	"Dektit1",	"Enchant",	"FilterBkg",
      "Fourth",		"Gold",		"Green",	"InfoBkg",
      "Instant",	"Interrupt",	"Land",		"Legends",
      "Other",		"Power",	"Rarity",	"Red",
      "Sorcery",	"Statbak1",	"Tough",	"White",
      "BtnBkg",
    };
  char* rval = NULL;

  for (int i = 0; i < NUM_PICS; ++i)
    {
      char path[MAX_PATH * 3 + 60];
      sprintf(path, global_dbart_pattern, global_cfg_skin_name[0] ? global_cfg_skin_name : ".", picnames[i]);
      if (!(global_pics[i] = LoadImage(path, global_palette, 0, 0)))
	{
	  rval = strdup(path);
	  break;
	}
    }

  if (rval)
    delete_pics();

  return rval;
}

static bool
init_palette(void)
{
  struct LogPalette256
  {
    WORD palVersion;
    WORD palNumEntries;
    PALETTEENTRY palPalEntry[256];
  } pal256 =
      {
	0x300,
	256,
	{
	  //0
	  {0x00, 0x00, 0x00, 0x00},	{0x1A, 0x18, 0x1C, 0x00},	{0x0E, 0x12, 0x4E, 0x00},	{0x0F, 0x12, 0x6D, 0x00},
	  {0x16, 0x2A, 0x4F, 0x00},	{0x14, 0x2C, 0x6E, 0x00},	{0x2A, 0x17, 0x4C, 0x00},	{0x29, 0x16, 0x6E, 0x00},
	  {0x2D, 0x31, 0x4C, 0x00},	{0x2C, 0x32, 0x6D, 0x00},	{0x29, 0x54, 0x25, 0x00},	{0x2D, 0x52, 0x5F, 0x00},
	  {0x4E, 0x0F, 0x14, 0x00},	{0x4F, 0x12, 0x29, 0x00},	{0x4D, 0x2E, 0x14, 0x00},	{0x4A, 0x32, 0x30, 0x00},
	  //16
	  {0x6B, 0x0F, 0x11, 0x00},	{0x6C, 0x12, 0x2E, 0x00},	{0x6D, 0x2F, 0x14, 0x00},	{0x6E, 0x30, 0x2C, 0x00},
	  {0x56, 0x2E, 0x56, 0x00},	{0x5C, 0x53, 0x2A, 0x00},	{0x4F, 0x4E, 0x4E, 0x00},	{0x4C, 0x52, 0x6D, 0x00},
	  {0x52, 0x69, 0x53, 0x00},	{0x51, 0x6B, 0x70, 0x00},	{0x6C, 0x52, 0x4F, 0x00},	{0x6D, 0x53, 0x6B, 0x00},
	  {0x6F, 0x6A, 0x52, 0x00},	{0x70, 0x6E, 0x6E, 0x00},	{0x1C, 0x24, 0x9B, 0x00},	{0x1D, 0x23, 0xDA, 0x00},
	  //32
	  {0x28, 0x58, 0x9E, 0x00},	{0x22, 0x60, 0xE2, 0x00},	{0x54, 0x2F, 0x9A, 0x00},	{0x55, 0x2A, 0xD9, 0x00},
	  {0x59, 0x66, 0x9A, 0x00},	{0x59, 0x68, 0xDC, 0x00},	{0x26, 0x92, 0x28, 0x00},	{0x27, 0x9A, 0x5F, 0x00},
	  {0x23, 0xD8, 0x2A, 0x00},	{0x22, 0xD6, 0x64, 0x00},	{0x5F, 0x95, 0x2A, 0x00},	{0x61, 0x96, 0x65, 0x00},
	  {0x64, 0xD5, 0x25, 0x00},	{0x62, 0xD1, 0x63, 0x00},	{0x2A, 0x99, 0xA2, 0x00},	{0x26, 0x9B, 0xEC, 0x00},
	  //48
	  {0x29, 0xD7, 0xA2, 0x00},	{0x27, 0xD9, 0xF1, 0x00},	{0x52, 0x8B, 0x90, 0x00},	{0x51, 0x8C, 0xAF, 0x00},
	  {0x50, 0xAD, 0x8E, 0x00},	{0x50, 0xAC, 0xB0, 0x00},	{0x70, 0x8C, 0x90, 0x00},	{0x6F, 0x8F, 0xAF, 0x00},
	  {0x6F, 0xAE, 0x90, 0x00},	{0x72, 0xAA, 0xB1, 0x00},	{0x50, 0x8E, 0xCF, 0x00},	{0x4E, 0x90, 0xF3, 0x00},
	  {0x50, 0xAC, 0xD0, 0x00},	{0x4F, 0xAF, 0xF6, 0x00},	{0x6F, 0x90, 0xCE, 0x00},	{0x6E, 0x92, 0xF1, 0x00},

	  //64
	  {0x71, 0xAD, 0xCF, 0x00},	{0x6E, 0xB0, 0xF4, 0x00},	{0x5F, 0xD5, 0xA4, 0x00},	{0x50, 0xCE, 0xD1, 0x00},
	  {0x4F, 0xCE, 0xF9, 0x00},	{0x4F, 0xF1, 0xCE, 0x00},	{0x50, 0xEC, 0xF9, 0x00},	{0x71, 0xCC, 0xD0, 0x00},
	  {0x70, 0xCD, 0xF8, 0x00},	{0x6F, 0xF1, 0xD1, 0x00},	{0x6F, 0xED, 0xF9, 0x00},	{0x98, 0x28, 0x24, 0x00},
	  {0x99, 0x27, 0x59, 0x00},	{0x90, 0x4B, 0x13, 0x00},	{0x8D, 0x4F, 0x30, 0x00},	{0x8E, 0x6F, 0x11, 0x00},
	  //80
	  {0x8F, 0x6D, 0x31, 0x00},	{0xAD, 0x4C, 0x14, 0x00},	{0xAD, 0x50, 0x2D, 0x00},	{0xAD, 0x6E, 0x13, 0x00},
	  {0xAE, 0x6F, 0x31, 0x00},	{0x8D, 0x51, 0x4E, 0x00},	{0x8B, 0x52, 0x6C, 0x00},	{0x8C, 0x6F, 0x52, 0x00},
	  {0x8B, 0x73, 0x6E, 0x00},	{0xAC, 0x51, 0x4E, 0x00},	{0xAD, 0x53, 0x6B, 0x00},	{0xAD, 0x71, 0x4E, 0x00},
	  {0xAC, 0x72, 0x6E, 0x00},	{0xD9, 0x2E, 0x22, 0x00},	{0xCC, 0x31, 0x57, 0x00},	{0xCA, 0x51, 0x12, 0x00},
	  //96
	  {0xCB, 0x51, 0x30, 0x00},	{0xCC, 0x6F, 0x14, 0x00},	{0xCD, 0x70, 0x32, 0x00},	{0xF4, 0x4E, 0x10, 0x00},
	  {0xEB, 0x4F, 0x32, 0x00},	{0xEA, 0x6F, 0x10, 0x00},	{0xEA, 0x71, 0x31, 0x00},	{0xD4, 0x67, 0x5C, 0x00},
	  {0x9F, 0x2E, 0x98, 0x00},	{0x9C, 0x31, 0xD9, 0x00},	{0x95, 0x6A, 0x99, 0x00},	{0x95, 0x69, 0xE1, 0x00},
	  {0xD0, 0x2E, 0xA9, 0x00},	{0xD7, 0x39, 0xCF, 0x00},	{0xD5, 0x66, 0x9F, 0x00},	{0xE2, 0x66, 0xD9, 0x00},
	  //112
	  {0xA2, 0x94, 0x22, 0x00},	{0x90, 0x8C, 0x52, 0x00},	{0x90, 0x8A, 0x72, 0x00},	{0x8D, 0xA7, 0x52, 0x00},
	  {0x8F, 0xAD, 0x71, 0x00},	{0xB1, 0x8C, 0x51, 0x00},	{0xAD, 0x8F, 0x71, 0x00},	{0xB1, 0xAB, 0x4F, 0x00},
	  {0xB0, 0xAB, 0x73, 0x00},	{0xA4, 0xCE, 0x30, 0x00},	{0x9D, 0xD8, 0x5E, 0x00},	{0xD3, 0x9A, 0x2B, 0x00},
	  {0xCD, 0x91, 0x4E, 0x00},	{0xCA, 0x93, 0x6E, 0x00},	{0xCD, 0xAB, 0x4F, 0x00},	{0xCF, 0xAD, 0x72, 0x00},

	  //128
	  {0xEB, 0x90, 0x51, 0x00},	{0xEF, 0x92, 0x70, 0x00},	{0xED, 0xAD, 0x50, 0x00},	{0xF2, 0xB0, 0x71, 0x00},
	  {0xDB, 0xCD, 0x1D, 0x00},	{0xE3, 0xD1, 0x68, 0x00},	{0x90, 0x8F, 0x8E, 0x00},	{0x8D, 0x93, 0xAC, 0x00},
	  {0x91, 0xAB, 0x92, 0x00},	{0x91, 0xAB, 0xB0, 0x00},	{0xAB, 0x93, 0x8D, 0x00},	{0xAD, 0x94, 0xAC, 0x00},
	  {0xB0, 0xAB, 0x92, 0x00},	{0xAF, 0xB0, 0xAE, 0x00},	{0x8D, 0x92, 0xCD, 0x00},	{0x8D, 0x91, 0xF1, 0x00},
	  //144
	  {0x8F, 0xB0, 0xCD, 0x00},	{0x8E, 0xB1, 0xF3, 0x00},	{0xAC, 0x92, 0xCD, 0x00},	{0xAC, 0x91, 0xF5, 0x00},
	  {0xAD, 0xB3, 0xCC, 0x00},	{0xAD, 0xB2, 0xF2, 0x00},	{0xA4, 0xD1, 0xA9, 0x00},	{0x91, 0xCA, 0xD1, 0x00},
	  {0x90, 0xCF, 0xF5, 0x00},	{0x8F, 0xF0, 0xD2, 0x00},	{0x90, 0xEF, 0xF9, 0x00},	{0xB1, 0xCB, 0xD0, 0x00},
	  {0xAF, 0xD1, 0xF3, 0x00},	{0xB1, 0xED, 0xD0, 0x00},	{0xB0, 0xEF, 0xF9, 0x00},	{0xCA, 0x94, 0x8D, 0x00},
	  //160
	  {0xCD, 0x94, 0xAB, 0x00},	{0xCF, 0xB0, 0x90, 0x00},	{0xCC, 0xB4, 0xAE, 0x00},	{0xF1, 0x94, 0x8C, 0x00},
	  {0xEC, 0x95, 0xAB, 0x00},	{0xF1, 0xB1, 0x8F, 0x00},	{0xEF, 0xB2, 0xAE, 0x00},	{0xCB, 0x91, 0xCF, 0x00},
	  {0xCC, 0x93, 0xF1, 0x00},	{0xCD, 0xB4, 0xCC, 0x00},	{0xCC, 0xB3, 0xF2, 0x00},	{0xEC, 0x90, 0xD1, 0x00},
	  {0xF3, 0x90, 0xF1, 0x00},	{0xED, 0xB6, 0xCB, 0x00},	{0xF1, 0xB1, 0xF4, 0x00},	{0xD1, 0xCA, 0x92, 0x00},
	  //176
	  {0xD0, 0xCB, 0xB3, 0x00},	{0xD1, 0xEB, 0x92, 0x00},	{0xD0, 0xEB, 0xB2, 0x00},	{0xEF, 0xCC, 0x91, 0x00},
	  {0xED, 0xD0, 0xB1, 0x00},	{0xF3, 0xEC, 0x91, 0x00},	{0xF5, 0xED, 0xB2, 0x00},	{0xD1, 0xD1, 0xCF, 0x00},
	  {0xCE, 0xD3, 0xF0, 0x00},	{0xD2, 0xEA, 0xD2, 0x00},	{0xD0, 0xF0, 0xF7, 0x00},	{0xED, 0xD4, 0xCD, 0x00},
	  {0xEF, 0xD4, 0xF1, 0x00},	{0xF2, 0xEA, 0xD2, 0x00},	{0xF6, 0xF7, 0xF7, 0x00},	{0xFE, 0xFE, 0xFE, 0x00},

	  //192
	  {0xDE, 0xDE, 0xDE, 0x00},	{0xCB, 0xCB, 0xCB, 0x00},	{0xB8, 0xB8, 0xB8, 0x00},	{0xA4, 0xA4, 0xA4, 0x00},
	  {0x93, 0x93, 0x93, 0x00},	{0x7D, 0x7D, 0x7D, 0x00},	{0x6A, 0x6A, 0x6A, 0x00},	{0x56, 0x56, 0x56, 0x00},
	  {0x43, 0x43, 0x43, 0x00},	{0x2F, 0x2F, 0x2F, 0x00},	{0x1C, 0x1C, 0x1C, 0x00},	{0x98, 0xA7, 0xCD, 0x00},
	  {0x79, 0x91, 0xCE, 0x00},	{0x65, 0x81, 0xBA, 0x00},	{0x51, 0x73, 0xA4, 0x00},	{0x3D, 0x63, 0x90, 0x00},
	  //208
	  {0x28, 0x52, 0x7B, 0x00},	{0x0F, 0x42, 0x68, 0x00},	{0x45, 0xE6, 0xFF, 0x00},	{0x25, 0xD3, 0xFC, 0x00},
	  {0x0C, 0xBA, 0xF1, 0x00},	{0x0F, 0x95, 0xCA, 0x00},	{0x10, 0x75, 0xA7, 0x00},	{0x10, 0x57, 0x83, 0x00},
	  {0x11, 0x94, 0xFF, 0x00},	{0x0E, 0x7E, 0xDD, 0x00},	{0x0B, 0x68, 0xBB, 0x00},	{0x09, 0x53, 0x9A, 0x00},
	  {0x06, 0x3D, 0x78, 0x00},	{0x03, 0x27, 0x56, 0x00},	{0xAA, 0xD4, 0x07, 0x00},	{0x96, 0xB9, 0x08, 0x00},
	  //224
	  {0x82, 0x9E, 0x09, 0x00},	{0x6F, 0x84, 0x0B, 0x00},	{0x5B, 0x69, 0x0C, 0x00},	{0x6F, 0x77, 0xEB, 0x00},
	  {0x4F, 0x55, 0xBC, 0x00},	{0x3F, 0x44, 0xA5, 0x00},	{0x2E, 0x32, 0x8E, 0x00},	{0x64, 0x90, 0x80, 0x00},
	  {0x55, 0x81, 0x69, 0x00},	{0x47, 0x72, 0x53, 0x00},	{0x38, 0x63, 0x3C, 0x00},	{0x24, 0x44, 0x21, 0x00},
	  {0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},
	  //240
	  {0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},
	  {0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},
	  {0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},
	  {0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0x01, 0x01, 0x01, 0x00},	{0xFF, 0xFF, 0xFF, 0x00}
	}
      };

  for (int i = 0; i < 256; ++i)
    {
      PALETTEENTRY* pe = &pal256.palPalEntry[i];
      pe->peFlags = 2;
      SWAP(pe->peRed, pe->peBlue);
    }

  global_palette = CreatePalette(reinterpret_cast<LOGPALETTE*>(&pal256));
  return global_palette;
}

static void
delete_resources(void)
{
  write_manalink_ini();

  if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
    {
      for (int i = 0; i < global_deck_num_entries; ++i)
	{
	  unsigned int v1 = global_deck[i].GDE_Available ? 0x80000 : 0;
	  global_external_deck[i] = global_deck[i].GDE_iid.raw | v1 | (global_deck[i].GDE_DecksBits << 16);
	}

      for (int i = global_deck_num_entries; i < 500; ++i)
	global_external_deck[i] = -1;

      *global_external_current_deck = global_current_deck;
    }

  delete_pics();
  DELETE_OBJ(global_palette);
  delete_brushes();
  DELETE_DC(global_hdc);
  DELETE_OBJ(global_hbmp);
  delete_fonts();
  SetCurrentDirectory(global_previous_directory);
}

static bool
process_cue_cards(MSG* msg)
{
  switch (msg->message)
    {
      case WM_TIMER:
	if (msg->hwnd == global_main_hwnd && msg->wParam == 0)
	  {
	    POINT screen_point;
	    GetCursorPos(&screen_point);

	    KillTimer(global_main_hwnd, 0);

	    HWND hwnd = WindowFromPoint(screen_point);

	    POINT client_point = screen_point;
	    ScreenToClient(hwnd, &client_point);

	    if (int msgnum = SendMessage(hwnd,
					 0x8465,
					 0,
					 MAKELONG(client_point.x, client_point.y)))
	      SendMessage(global_cuecard_hwnd,
			  0x8400,
			  MAKELONG(screen_point.x + 5, screen_point.y + 15),
			  (LPARAM)(text_lines[msgnum - 1]));

	    return true;
	  }
	return false;

      case WM_MOUSEMOVE:
	POINT cur_point;
	GetCursorPos(&cur_point);

	static POINT last_point = {0, 0};

	int dx, dy;
	dx = abs(last_point.x - cur_point.x);
	dy = abs(last_point.y - cur_point.y);

	if (dx + dy >= 2)
	  {
	    last_point = cur_point;

	    SetTimer(global_main_hwnd, 0, 500, NULL);

	    static HWND last_hwnd = NULL;
	    if (last_hwnd != msg->hwnd)
	      {
		last_hwnd = msg->hwnd;
		ShowWindow(global_cuecard_hwnd, SW_HIDE);
	      }
	  }
	return false;

      case WM_LBUTTONDOWN:	case WM_LBUTTONUP:	case WM_LBUTTONDBLCLK:
      case WM_RBUTTONDOWN:	case WM_RBUTTONUP:	case WM_RBUTTONDBLCLK:
      case WM_MBUTTONDOWN:	case WM_MBUTTONUP:	case WM_MBUTTONDBLCLK:
	KillTimer(global_main_hwnd, 0);
	ShowWindow(global_cuecard_hwnd, SW_HIDE);
	return false;

      default:
	return false;
    }
}

/* stdcall version, perversely required to be at _DeckBuilderMain instead of _DeckBuilderMain@12, which makes it inconvenient and error-prone call correctly
 * external to the dll except from assembly */
WPARAM WINAPI
DeckBuilderMain(HWND parent_hwnd, int db_flags_1, int db_flags_2)
{
  if (!global_db_read)
    {
      global_available_slots = read_db();
      if (!global_available_slots)
	{
	  fatal_err("Fatal: Couldn't find Cards.dat, DBInfo.dat or Rarity.dat");
	  return 0;
	}
    }

  srand(GetTickCount());

  global_db_flags_1 = static_cast<DBFlags>(db_flags_1);
  global_db_flags_2 = static_cast<DBFlags>(db_flags_2);

  InitializeCriticalSection(&global_critical_section_for_unknown);

  GetCurrentDirectory(MAX_PATH + 1, global_previous_directory);
  SetCurrentDirectory(global_base_directory);

#define GET_IMPORT(proc_name)	(GetProcAddress(GetModuleHandle(NULL), proc_name))
  CardIDFromType = (Int_fn_int)GET_IMPORT("CardIDFromType");
  SellPrice = (Int_fn_int)GET_IMPORT("SellPrice");
  CardTypeFromID = (Int_fn_int)GET_IMPORT("CardTypeFromID");
  global_external_deck = (int*)GET_IMPORT("deck");
  Gold = (int*)GET_IMPORT("Gold");
  Scards = (shandalar_worldmagic_t*)GET_IMPORT("Scards");
  global_external_deckname = (char*)GET_IMPORT("szDeckName");
  global_external_current_deck = (int*)GET_IMPORT("_currentDeck");
#undef GET_IMPORT

  if (((global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
       && (!CardIDFromType || !Gold || !SellPrice || !global_external_current_deck || !global_external_deck || !Scards))
      || ((global_db_flags_1 & (DBFLAGS_GAUNTLET | DBFLAGS_NOCARDCOUNTCHECK))
	  && (!CardIDFromType || !Gold || !global_external_deckname || !CardTypeFromID)))
    {
      fatal_err("Fatal: Couldn't find External Functions.");
      return 0;
    }

  if (!init_palette())
    {
      fatal_err("Fatal: Couldn't create the palette");
      return 0;
    }

  if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
    global_current_deck = *global_external_current_deck;

  HDC hdc = GetDC(GetDesktopWindow());

  global_hdc = CreateCompatibleDC(hdc);
  global_hbmp = CreateCompatibleBitmap(hdc, 800, 800);
  if (!global_hdc || !global_hbmp)
    {
      fatal_err("Fatal: Couldn't create the app-wide memory DC or bitmap");
      return 0;
    }

  gdi_flush(global_hdc);

  global_old_bmp_obj = SelectObject(global_hdc, global_hbmp);

  ReleaseDC(GetDesktopWindow(), hdc);

  if (!create_screen_dc())
    {
      fatal_err("Fatal: Couldn't create the app-wide memory DC or bitmap");
      return 0;
    }

  if (const char* load_pic_err = load_pics())
    {
      char buf[strlen(load_pic_err) + 80];
      sprintf(buf, "Fatal: Couldn't load deck builder bitmap:\n%s", load_pic_err);
      fatal_err(buf);
      return 0;
    }

  if (!create_brushes())
    {
      fatal_err("Fatal: Couldn't create the window background bitmaps");
      return 0;
    }

  create_fonts();

  make_orig_rarities();

  RECT r;
  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    {
      r.left = 0;
      r.top = 0;
      global_main_window_width = GetSystemMetrics(SM_CXSCREEN);
      global_main_window_height = GetSystemMetrics(SM_CYSCREEN);
    }
  else
    {
      SystemParametersInfo(SPI_GETWORKAREA, 0, &r, 0);
      global_main_window_width = RECT_WIDTH(r) + 1;
      global_main_window_height = RECT_HEIGHT(r) + 1;
    }

  global_main_hwnd = CreateWindowEx(0, "MainClass", "Deck Builder",
				    WS_POPUP | WS_VISIBLE | (global_cfg_no_frame ? 0 : WS_THICKFRAME),
				    r.left, r.top, global_main_window_width, global_main_window_height,
				    parent_hwnd,
				    0,
				    global_hinstance, 0);
  if (!global_main_hwnd)
    {
      fatal_err("Fatal: Couldn't create the main window");
      return 0;
    }

  global_cuecard_hwnd = CreateWindowEx(0, "CueCardClass", "",
				       WS_POPUP,
				       0, 0, 0, 0,
				       global_main_hwnd,
				       0,
				       global_hinstance, 0);
  if (!global_cuecard_hwnd)
    {
      fatal_err("Fatal: Couldn't create the cue card window", global_main_hwnd);
      return 0;
    }

  HACCEL accel = LoadAccelerators(global_hinstance, MAKEINTRESOURCE(RES_ACCEL));
  if (!accel)
    {
      fatal_err("Fatal: Couldn't load the accelerator table", global_main_hwnd);
      return 0;
    }

  ShowWindow(global_main_hwnd, SW_SHOWNORMAL);
  SetTimer(global_main_hwnd, 0, 500, NULL);
  SetForegroundWindow(global_main_hwnd);

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0))
    if (!TranslateAccelerator(global_main_hwnd, accel, &msg)
	&& !process_cue_cards(&msg))
      {
	TranslateMessage(&msg);
	DispatchMessage(&msg);
      }

  delete_resources();
  delete_screen_dc();
  DeleteCriticalSection(&global_critical_section_for_unknown);

  return msg.wParam;
}

WPARAM
deckbuilder_main(HWND parent_hwnd, int db_flags_1, int db_flags_2)
{
  return DeckBuilderMain(parent_hwnd, db_flags_1, db_flags_2);
}

static bool
register_class(const char* classname, WNDPROC wndproc, UINT style, int extra_size, HICON icon, HCURSOR cursor)
{
  WNDCLASS wnd_class;

  wnd_class.style = style;
  wnd_class.lpfnWndProc = wndproc;
  wnd_class.cbClsExtra = 0;
  wnd_class.cbWndExtra = extra_size;
  wnd_class.hInstance = global_hinstance;
  wnd_class.hIcon = icon;
  wnd_class.hCursor = cursor;
  wnd_class.hbrBackground = NULL;
  wnd_class.lpszMenuName = NULL;
  wnd_class.lpszClassName = classname;

  return RegisterClass(&wnd_class);
}

static bool
register_classes(void)
{
  HCURSOR arrow_cursor = LoadCursor(0, IDC_ARROW);
  HICON app_icon = LoadIcon(0, IDI_APPLICATION);

  return (register_class("MainClass",
			 wndproc_MainClass,
			 CS_OWNDC,
			 0,
			 LoadIcon(global_hinstance, MAKEINTRESOURCE(RES_ICON)),
			 arrow_cursor)
	  && register_class("DeckSurfaceClass",
			    wndproc_DeckSurfaceClass,
			    0,
			    0,
			    app_icon,
			    arrow_cursor)
	  && register_class("CardClass",
			    wndproc_CardClass,
			    CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
			    8,
			    app_icon,
			    arrow_cursor)
	  && register_class("FullCardClass",
			    wndproc_FullCardClass,
			    CS_OWNDC | CS_HREDRAW | CS_VREDRAW,
			    8,
			    0,
			    arrow_cursor)
	  && register_class("HorzListClass",
			    wndproc_HorzListClass,
			    CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW,
			    24,
			    0,
			    arrow_cursor)
	  && register_class("CardListFilterClass",
			    wndproc_CardListFilterClass,
			    CS_HREDRAW | CS_VREDRAW,
			    4,
			    0,
			    arrow_cursor)
	  && register_class("CueCardClass",
			    wndproc_CueCardClass,
			    CS_SAVEBITS,
			    4,
			    app_icon,
			    arrow_cursor)
	  && register_class("TitleClass",
			    wndproc_TitleClass,
			    0,
			    0,
			    app_icon,
			    arrow_cursor));
}

static bool
init_deckbuilder(void)
{
  global_deck_was_edited = false;
  global_deckname_set = false;
  global_current_deck = 0;

  InitCommonControls();

  GetModuleFileName(NULL, global_base_directory, MAX_PATH + 1);
  *strrchr(global_base_directory, '\\') = 0;

  sprintf(global_manalink_ini_path, "%s\\Manalink.ini", global_base_directory);
  sprintf(global_playdeck_path, "%s\\PlayDeck\\", global_base_directory);
  sprintf(global_dbart_pattern, "%s\\DBArt\\%%s\\%%s.pic", global_base_directory);

  read_manalink_ini();

  global_available_slots = 0;
  global_raw_rarities = NULL;
  global_raw_dbinfo = NULL;
  global_base_txt = NULL;
  global_raw_cards_ptr = NULL;
  card_coded = NULL;

  if (!register_classes())
    {
      fatal_err("Fatal: Couldn't register the classes");
      return false;
    }

  return true;
}

BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
  switch (reason)
    {
      case DLL_PROCESS_DETACH:
	free_db();
	return 1;

      case DLL_PROCESS_ATTACH:
	global_hinstance = dll;
	return init_deckbuilder() ? 1 : 0;

      default:
	return 1;
    }
}

BOOL WINAPI
DllEntryPoint(HINSTANCE dll, DWORD reason, LPVOID reserved)	// Stupidly enough, there's a direct reference to this alternate name.
{
  return DllMain(dll, reason, reserved);
}
//]]]

//[[[ sounds
static void
clear_sound_imports_table(void)
{
  for (int i = 0; i <= SND_MAX; ++i)
    global_sound_fns[i] = NULL;
}

static bool
init_sound_dll_impl(HWND hwnd, int a2, int a3)	// returns false if needs cleanup
{
  if (global_sound_status)
    return true;

  global_hmodule_magsnd_dll = LoadLibrary("magsnd");

  if (!global_hmodule_magsnd_dll)
    return true;

  for (int i = 0; i <= SND_MAX; ++i)
    if (!(global_sound_fns[i] = reinterpret_cast<Int_fn_etc>(GetProcAddress(global_hmodule_magsnd_dll, reinterpret_cast<const char*>(i + 1)))))
      return false;

  if (!hwnd && !(a3 & 2))
    return false;

  if ((global_sound_fns[SND_InitSnd])(hwnd, a2, a3))
    return false;

  global_sound_unk1 = true;
  if (a3 & 2)
    global_sound_unk2 = true;
  global_sound_status = 1;

  return true;
}

static void
init_sound_dll(HWND hwnd, int a2, int a3)
{
  if (!init_sound_dll_impl(hwnd, a2, a3))
    {
      FreeLibrary(global_hmodule_magsnd_dll);
      clear_sound_imports_table();
    }
}

static void
sound_load(const char* path, int num, Sound* snd)
{
  if (global_sound_status)
    (global_sound_fns[SND_LoadSnd])(path, num, snd);
}

static void
sound_init(const char* path, int num)
{
  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  Sound snd;
  memset(&snd, 0, sizeof(Sound));
  snd.field_0 = 400;

  sound_load(path, num, &snd);

  // set_sound_loop(num, 1) inlined:
  if (global_sound_status == 1)
    (global_sound_fns[SND_SetSndMarker])(num, 1);
}

static void
sound_stop(int a1)
{
  if (global_sound_status && global_sound_status != 2)
    (global_sound_fns[SND_StopSnd])(a1);
}

static void
init_sounds_and_music(void)
{
  if (global_db_flags_1 & DBFLAGS_SHANDALAR)
    return;

  char path[MAX_PATH];
  sprintf(path, "Sound\\LocMus%d.wav", RANDRANGE(1, 19));
  sound_init(path, 1);
  sprintf(path, "DuelSounds\\discard.wav");
  sound_load(path, 3, 0);
  sprintf(path, "DuelSounds\\draw.wav");
  sound_load(path, 2, 0);
  sprintf(path, "DuelSounds\\button.wav");
  sound_load(path, 4, 0);
  sprintf(path, "DuelSounds\\cancel.wav");
  sound_load(path, 5, 0);
}

static void
sound_unload(int idx)
{
  if (global_sound_status)
    (global_sound_fns[SND_UnloadSnd])(idx);
}

static void
free_sounds_and_music(void)
{
  sound_unload(1);
  sound_unload(2);
  sound_unload(3);
  sound_unload(4);
  sound_unload(5);

  // sound_close() inlined:
  if (global_sound_status)
    {
      global_sound_status = 0;

      if (global_sound_unk1 && !global_sound_unk2)
	(global_sound_fns[SND_ReleaseSnd])();

      FreeLibrary(global_hmodule_magsnd_dll);

      clear_sound_imports_table();

      global_hmodule_magsnd_dll = NULL;
      global_sound_unk1 = global_sound_unk2 = false;
    }
}

static void
play_sound(int a1, int a2, int a3, int a4)
{
  if ((global_db_flags_1 & DBFLAGS_SHANDALAR)
      || global_sound_status == 0
      || global_sound_status == 2)
    return;

  Sound snd;

  memset(&snd, 0, sizeof(Sound));
  snd.field_0 = 4 * a2;
  snd.field_4 = 22050 * a3 / 100;
  snd.field_8 = 4 * a4;
  snd.field_1C &= ~1;
  snd.field_1C &= ~10;

  (global_sound_fns[SND_PlaySnd])(a1, &snd);
}

static void
play_music(int a1, int a2, int a3)
{
  if ((global_db_flags_1 & DBFLAGS_SHANDALAR)
      || global_sound_status == 0
      || global_sound_status == 2)
    return;

  Sound snd;

  memset(&snd, 0, sizeof(Sound));
  snd.field_0 = 4 * a2;
  snd.field_4 = 22050;
  snd.field_8 = 4 * a3;
  snd.field_1C |= 1;

  (global_sound_fns[SND_PlaySnd])(a1, &snd);
}
//]]]

//[[[ dialogs
INT_PTR CALLBACK
dlgproc_DeckInfo(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_INITDIALOG:
	load_text("Menus", "TITLEDIALOG");
	SetWindowText(hdlg, text_lines[0]);
	set_dlg_text(hdlg, RES_DECKINFO_DECKTITLE, text_lines[1]);
	set_dlg_text(hdlg, RES_DECKINFO_DESCRIPTION, text_lines[2]);
	set_dlg_text(hdlg, RES_DECKINFO_NAME, text_lines[3]);
	set_dlg_text(hdlg, RES_DECKINFO_EMAIL, text_lines[4]);
	set_dlg_text(hdlg, RES_DECKINFO_DATE, text_lines[5]);
	set_dlg_text(hdlg, RES_DECKINFO_COMMENT, text_lines[7]);
	set_dlg_text(hdlg, RES_DECKINFO_VERSION, text_lines[8]);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK, text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL, text_lines[1]);
	set_dlg_text(hdlg, RES_DECKINFO_DECKTITLE_EDITTEXT, global_deckname, 28);
	set_dlg_text(hdlg, RES_DECKINFO_DESCRIPTION_EDITTEXT, global_deck_description, 18);
	set_dlg_text(hdlg, RES_DECKINFO_NAME_EDITTEXT, global_deck_author, 78);
	set_dlg_text(hdlg, RES_DECKINFO_EMAIL_EDITTEXT, global_deck_email, 78);
	set_dlg_text(hdlg, RES_DECKINFO_DATE_EDITTEXT, global_deck_creation_date, 19);
	set_dlg_text(hdlg, RES_DECKINFO_VERSION_EDITTEXT, "4th Edition", 13);
	set_dlg_text(hdlg, RES_DECKINFO_COMMENT_EDITTEXT, global_deck_comments, 398);
	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[23]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_BUTTON_CANCEL)
	  EndDialog(hdlg, 0);
	else if (LOWORD(wparam) == RES_BUTTON_OK)
	  {
	    char buf[261];

	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_DECKTITLE_EDITTEXT), buf, 261);

	    for (char* p = buf, *q = buf; (*p = *q); ++p, ++q)
	      if (*p == '.')
		--p;

	    if (strcmp(buf, global_deckname))
	      {
		global_deck_was_edited = global_deckname_set = true;
		strcpy(global_deckname, buf);
	      }

	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_DESCRIPTION_EDITTEXT), global_deck_description, 21);
	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_NAME_EDITTEXT), global_deck_author, 81);
	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_EMAIL_EDITTEXT), global_deck_email, 81);
	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_DATE_EDITTEXT), global_deck_creation_date, 22);
	    global_deck_revision = 1;
	    global_deck_edition[GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_VERSION_EDITTEXT), global_deck_edition, 16)] = 0;
	    GetWindowText(GetDlgItem(hdlg, RES_DECKINFO_COMMENT_EDITTEXT), global_deck_comments, 401);

	    sprintf(global_deck_filename, "%s%s.dck", global_playdeck_path, global_deckname);

	    EndDialog(hdlg, 1);
	  }
	return 1;

      default:
	return 0;
    }
}

static void
show_dialog_deckinfo(void)
{
  if (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_DECKINFO), global_main_hwnd, dlgproc_DeckInfo, 0) == -1)
    MessageBox(global_main_hwnd, "Couldn't bring up the title dialog box", "", 0);
}

INT_PTR CALLBACK
dlgproc_GroupMove(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_INITDIALOG:
	load_text("Menus", "GROUPMOVE");
	SetWindowText(hdlg, text_lines[0]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_BLACK,		text_lines[1]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_BLUE,		text_lines[2]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_GREEN,		text_lines[3]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_RED,		text_lines[4]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_WHITE,		text_lines[5]);
	set_dlg_text(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT,	text_lines[6]);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK,			text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL,			text_lines[1]);

	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLACK),	!!(global_dlg_parameter & 1));
	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_BLUE),	!!(global_dlg_parameter & 2));
	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_GREEN),	!!(global_dlg_parameter & 4));
	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_RED),	!!(global_dlg_parameter & 8));
	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_WHITE),	!!(global_dlg_parameter & 0x10));
	EnableWindow(GetDlgItem(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT),	!!(global_dlg_parameter & 0x20));
	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[19]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case RES_BUTTON_OK:
	      global_dlg_result = 0;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_BLACK))
		global_dlg_result |= COLOR_TEST_BLACK;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_BLUE))
		global_dlg_result |= COLOR_TEST_BLUE;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_GREEN))
		global_dlg_result |= COLOR_TEST_GREEN;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_RED))
		global_dlg_result |= COLOR_TEST_RED;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_WHITE))
		global_dlg_result |= COLOR_TEST_WHITE;
	      if (IsDlgButtonChecked(hdlg, RES_GROUPMOVE_BUTTON_ARTIFACT))
		global_dlg_result |= COLOR_TEST_ARTIFACT;
	      global_dlg_result >>= 1;
	      EndDialog(hdlg, 1);
	      break;

	    case RES_BUTTON_CANCEL:
	      global_dlg_result = 0;
	      EndDialog(hdlg, 0);
	      break;
	  }
	return 1;

      default:
	return 0;
    }
}

static bool
show_dialog_groupmove(void)
{
  switch (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_GROUPMOVE), global_main_hwnd, dlgproc_GroupMove, 0))
    {
      case 1:
	return true;

      case -1:
	MessageBox(global_main_hwnd, "Couldn't bring up the group-move dialog box", "", MB_OK);
	return false;

      default:
	return false;
    }
}

INT_PTR CALLBACK
dlgproc_AskX(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_INITDIALOG:
	SetWindowText(hdlg, global_ask_x_dlg_title);

	char buf[12];
	sprintf(buf, "%d", global_dlg_parameter);
	set_dlg_text(hdlg, RES_ASKVALUE_EDITTEXT,	buf);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK,		text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL,		text_lines[1]);

	RECT rect_main, rect_dlg;
	GetWindowRect(global_main_hwnd, &rect_main);
	GetWindowRect(hdlg, &rect_dlg);
	SetWindowPos(hdlg, NULL,
		     rect_main.left + (RECT_WIDTH(rect_main) - RECT_WIDTH(rect_dlg)) / 2,
		     rect_main.top + (RECT_HEIGHT(rect_main) - RECT_HEIGHT(rect_dlg)) / 2,
		     0, 0,
		     SWP_NOZORDER | SWP_NOSIZE);

	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[19]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_BUTTON_CANCEL)
	  {
	    global_dlg_result = 0;
	    EndDialog(hdlg, 0);
	  }
	else if (LOWORD(wparam) == RES_BUTTON_OK)
	  {
	    char txt[12];
	    int len = GetWindowText(GetDlgItem(hdlg, RES_ASKVALUE_EDITTEXT), txt, 10);
	    txt[len] = 0;
	    global_dlg_result = atoi(txt);
	    EndDialog(hdlg, 1);
	  }
	return 1;
    }
  return 0;
}

static INT_PTR
show_dialog_sellxcards(void)
{
  load_text("Menus", "NUMBERTOSELL");
  strcpy(global_ask_x_dlg_title, text_lines[0]);
  INT_PTR rval = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_ASK_VALUE), global_main_hwnd, dlgproc_AskX, 0);
  if (rval == -1)
    {
      MessageBox(global_main_hwnd, "Couldn't bring up the sell #/cards dialog box", "", MB_OK);
      return 0;
    }
  else
    return rval;
}

static INT_PTR
show_dialog_movexcards(void)
{
  load_text("Menus", "NUMBERTOMOVE");
  strcpy(global_ask_x_dlg_title, text_lines[0]);
  INT_PTR rval = DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_ASK_VALUE), global_main_hwnd, dlgproc_AskX, 0);
  if (rval == -1)
    {
      MessageBox(global_main_hwnd, "Couldn't bring up the move #/cards dialog box", "", MB_OK);
      rval = 0;
    }
  return rval;
}

INT_PTR CALLBACK
dlgproc_InfoBox(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_INITDIALOG:
	load_text("Menus", "EXTRACARDSDIALOG");
	SetWindowText(hdlg,				text_lines[0]);
	set_dlg_text(hdlg, RES_INFOBOX_DESCRIPTION,	text_lines[1]);
	set_dlg_text(hdlg, RES_BUTTON_OK,		text_lines[2]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL,		text_lines[3]);

	for (int i = 0; global_excessive_cards[i].DeckEntry_csvid.ok(); ++i)
	  SendDlgItemMessage(hdlg, RES_INFOBOX_LIST, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(global_excessive_cards[i].DeckEntry_FullName));

	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[23]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_BUTTON_CANCEL)
	  EndDialog(hdlg, 0);
	else if (LOWORD(wparam) == RES_BUTTON_OK)
	  {
	    for (int i = 0; global_excessive_cards[i].DeckEntry_csvid.ok(); ++i)
	      for (int j = 0; j < global_deck_num_entries; ++j)
		if (global_deck[j].GDE_csvid == global_excessive_cards[i].DeckEntry_csvid
		    && !global_deck[j].GDE_Available
		    && global_excessive_cards[i].DeckEntry_Amount > 0)
		  {
		    global_deck[j].GDE_Available = 1;
		    global_deck[j].GDE_DecksBits &= ~(1 << global_current_deck);
		    --global_excessive_cards[i].DeckEntry_Amount;
		  }
	    EndDialog(hdlg, 1);
	  }
	return 1;

      default:
	return 0;
    }
}

static bool
show_dialog_infobox(void)
{
  switch (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_INFOBOX), global_main_hwnd, dlgproc_InfoBox, 0))
    {
      case 1:
	return true;

      case -1:
	MessageBox(global_main_hwnd, "Couldn't bring up the title dialog box", "", MB_OK);
	return false;

      default:
	return false;
    }
}

INT_PTR CALLBACK
dlgproc_LoadDeck(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC hdc;

  switch (msg)
    {
      case WM_INITDIALOG:
	load_text("Menus", "LOADDECKDIALOG");
	SetWindowText(hdlg,				text_lines[0]);
	set_dlg_text(hdlg, RES_LOADDECK_PLAYERDECK,	text_lines[1]);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK,		text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL,		text_lines[1]);

	if (HWND hwnd = CreateWindowEx(0, "LISTBOX", "",
				       WS_CHILDWINDOW | WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_SORT,
				       0, 0, 0, 0,
				       hdlg,
				       0,
				       global_hinstance, 0))
	  {
	    char path[2 * MAX_PATH + 15 + 10];
	    sprintf(path, "%s*.dck", global_playdeck_path);

	    SendMessage(hwnd, LB_DIR, DDL_READWRITE, reinterpret_cast<LPARAM>(path));
	    int count = SendMessage(hwnd, LB_GETCOUNT, 0, 0);

	    for (int i = 0; i < count; ++i)
	      {
		char filename[MAX_PATH];
		SendMessage(hwnd, LB_GETTEXT, i, reinterpret_cast<LPARAM>(filename));
		sprintf(path, "%s%s", global_playdeck_path, filename);

		File f(path, "r", true);
		if (f.ok())
		  {
		    char line[464];
		    f.readline(line, 464);
		    f.close();

		    // All this does is ignore deck files that don't begin with a semi-colon then the deck's filename (without .dck).
		    // It's both crushingly slow and completely unnecessary.  Sigh.
		    if (line[0] == ';')
		      {
			char* deckname = &line[1];
			sprintf(path, "%s%s.dck", global_playdeck_path, deckname);

			File g(path, "r", true);
			if (g.ok())
			  SendDlgItemMessage(hdlg, RES_LOADDECK_DECKLIST, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(deckname));
		      }
		  }
	      }

	    SendDlgItemMessage(hdlg, RES_LOADDECK_DECKLIST, CB_SETCURSEL, 0, 0);

	    return 0;
	  }
	else
	  {
	    EndDialog(hdlg, -1);
	    return 1;
	  }

      case WM_ERASEBKGND:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[23]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case RES_BUTTON_OK:
	      char filename[MAX_PATH + 10];
	      SendDlgItemMessage(hdlg, RES_LOADDECK_DECKLIST, CB_GETLBTEXT,
				 SendDlgItemMessage(hdlg, RES_LOADDECK_DECKLIST, CB_GETCURSEL, 0, 0),
				 reinterpret_cast<LPARAM>(filename));
	      sprintf(global_loaddeck_dlg_filename, "%s%s.dck", global_playdeck_path, filename);
	      EndDialog(hdlg, 1);
	      break;

	    case RES_BUTTON_CANCEL:
	      EndDialog(hdlg, 0);
	      break;
	  }
	return 1;

      default:
	return 0;
    }
}

static bool
show_dialog_loaddeck(char* dest)
{
  switch (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_LOADDECK), global_main_hwnd, dlgproc_LoadDeck, 0))
    {
      case 1:
	strcpy(dest, global_loaddeck_dlg_filename);
	return true;

      case -1:
	MessageBox(global_main_hwnd, "Couldn't bring up the Load Deck dialog", "", MB_OK);
	return false;

      default:
	return false;
    }
}

INT_PTR CALLBACK
dlgproc_FilterGLE(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  HDC hdc;

  switch (msg)
    {
      case WM_INITDIALOG:
	char txt[24];
	sprintf(txt, "%d", global_dlg_parameter);
	SetWindowText(GetDlgItem(hdlg, RES_ASKVALUE_EDITTEXT), txt);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK, text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL, text_lines[1]);
	SetWindowText(hdlg, global_filter_dlg_title);
	return 0;

      case WM_ERASEBKGND:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[19]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_BUTTON_CANCEL)
	  EndDialog(hdlg, 0);
	else if (LOWORD(wparam) == RES_BUTTON_OK)
	  {
	    char val[262];
	    GetWindowText(GetDlgItem(hdlg, RES_ASKVALUE_EDITTEXT), val, 261);
	    val[261] = 0;
	    global_filter_gle_dlg_value = atoi(val);
	    EndDialog(hdlg, 1);
	  }
	return 1;

      default:
	return 0;
    }
}

static bool
show_dialog_filter_gle(int textline)
{
  load_text("Menus", "POWERTOUGHNESSCC");
  strcpy(global_filter_dlg_title, text_lines[textline]);

  switch (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_ASK_VALUE), global_main_hwnd, dlgproc_FilterGLE, 0))
    {
      case 1:
	return true;

      case -1:
	MessageBox(global_main_hwnd, "Couldn't bring up the filter dialog box", "", MB_OK);
	return false;

      default:
	return false;
    }
}

INT_PTR CALLBACK
dlgproc_FilterSubtype(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static int* data1 = NULL;
  static int* data2 = NULL;
  HDC hdc;

  switch (msg)
    {
      case WM_INITDIALOG:
	SetWindowText(hdlg, global_filter_dlg_title);

	load_text("Menus", "LONGLIST");
	set_dlg_text(hdlg, RES_FILTERLIST_ENABLEFILTER, text_lines[0]);
	set_dlg_text(hdlg, RES_FILTERLIST_SELECTALL, text_lines[1]);
	set_dlg_text(hdlg, RES_FILTERLIST_CLEARALL, text_lines[2]);

	load_text("Menus", "OKCANCEL");
	set_dlg_text(hdlg, RES_BUTTON_OK, text_lines[0]);
	set_dlg_text(hdlg, RES_BUTTON_CANCEL, text_lines[1]);

	int num;
	num = load_text("Menus", global_filter_subtype_dlg_mode ? "EXPANSIONNAMES" : "CREATURETYPES");

	data1 = reinterpret_cast<int*>(malloc(MAX_FILTER_SUBTYPE_SIZE * sizeof(int)));
	data2 = reinterpret_cast<int*>(malloc(MAX_FILTER_SUBTYPE_SIZE * sizeof(int)));
	SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, FALSE, 0);

	bool checked;
	checked = global_filter_subtype_dlg_mode ? (global_filter_expansions & FE_EXPANSIONLIST) : (global_filter_cardtypes & FT_CREATURE_LIST);
	SendDlgItemMessage(hdlg, RES_FILTERLIST_ENABLEFILTER, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);

	int m, pos;
	if (global_filter_subtype_dlg_mode)
	  {
	    m = LB_INSERTSTRING;
	    pos = -1;
	  }
	else
	  {
	    m = LB_ADDSTRING;
	    pos = 0;
	  }

	for (int i = 0; i < num; ++i)
	  {
	    int idx = SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, m, pos, reinterpret_cast<LPARAM>(text_lines[i]));
	    SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_SETITEMDATA, idx, i);
	  }

	for (int i = 0; i < num; ++i)
	  {
	    int j = SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_GETITEMDATA, i, 0);
	    if (j >= 0)
	      {
		data1[i] = j;
		data2[j] = i;
	      }
	  }

	for (int i = 0; i < num; ++i)
	  {
	    int idx = i >> 5;
	    int bit = 1 << (i & 0x1F);

	    bool sel = global_filter_subtype_dlg_mode ? (global_filter_expansion_list[idx] & bit) : (global_filter_creature_list[idx] & bit);
	    SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_SETSEL, sel, data2[i]);
	  }

	SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_SETCARETINDEX, 0, 0);
	SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, TRUE, 0);
	return 0;

      case WM_ERASEBKGND:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	RECT r;
	GetClientRect(hdlg, &r);
	draw_background(hdc, &r, global_pics[19]);
	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case RES_FILTERLIST_CLEARALL:
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, FALSE, 0);
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_SETSEL, FALSE, -1);
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, TRUE, 0);
	      break;

	    case RES_FILTERLIST_ENABLEFILTER:
	      if (global_filter_subtype_dlg_mode)
		global_filter_expansions ^= FE_EXPANSIONLIST;
	      else
		global_filter_cardtypes ^= FT_CREATURE_LIST;
	      break;

	    case RES_BUTTON_OK:
	      int buf[MAX_FILTER_SUBTYPE_SIZE];
	      memset(buf, -1, sizeof buf);

	      if (global_filter_subtype_dlg_mode)
		memset(global_filter_expansion_list, 0, sizeof global_filter_expansion_list);
	      else
		memset(global_filter_creature_list, 0, sizeof global_filter_creature_list);

	      int items;
	      items = SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_GETSELITEMS, MAX_FILTER_SUBTYPE_SIZE, reinterpret_cast<LPARAM>(buf));

	      for (int i = 0; i < items; ++i)
		if (buf[i] != -1)
		  {
		    int idx = data1[buf[i]] >> 5;
		    int bit = 1 << (data1[buf[i]] & 0x1f);

		    if (global_filter_subtype_dlg_mode)
		      global_filter_expansion_list[idx] |= bit;
		    else
		      global_filter_creature_list[idx] |= bit;
		  }

	      EndDialog(hdlg, 1);
	      FREEZ(data1);
	      FREEZ(data2);
	      return 1;

	    case RES_BUTTON_CANCEL:
	      EndDialog(hdlg, 0);
	      FREEZ(data1);
	      FREEZ(data2);
	      return 1;

	    case RES_FILTERLIST_SELECTALL:
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, FALSE, 0);
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, LB_SETSEL, TRUE, -1);
	      SendDlgItemMessage(hdlg, RES_FILTERLIST_LISTBOX, WM_SETREDRAW, TRUE, 0);
	      return 1;
	  }
	return 1;

      default:
	return 0;
    }
}

static bool
show_dialog_filter_subtype(void)
{
  load_text("Menus", "FILTERDIALOGTITLE");
  strcpy(global_filter_dlg_title, text_lines[global_filter_subtype_dlg_mode ? 0 : 1]);

  switch (DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_FILTER_LIST), global_main_hwnd, dlgproc_FilterSubtype, 0))
    {
      case 1:
	return true;

      case -1:
	MessageBox(global_main_hwnd, "Couldn't bring up the filter dialog box", "", MB_OK);
	return false;

      default:
	return false;
    }
}
//]]]

//[[[ stats dialog
static bool
check_restriction_impl(csvid_t csvid, Restriction::Enum rst)
{
  for (int i = 0; restrictions[i].restriction != Restriction::RST_0; ++i)
    if (restrictions[i].csvid == csvid && (restrictions[i].restriction & rst))
      return true;
  return false;
}

static bool
check_restricted(csvid_t csvid)
{
  return check_restriction_impl(csvid, Restriction::RST_RESTRICTED);
}

static bool
check_banned(csvid_t csvid)
{
  return check_restriction_impl(csvid, Restriction::RST_BANNED);
}

static bool
check_ante(csvid_t csvid)
{
  return check_restriction_impl(csvid, Restriction::RST_ANTE);
}

static bool
check_basic(csvid_t csvid)
{
  switch (csvid.raw)
    {
      case CARD_ID_SWAMP:
      case CARD_ID_ISLAND:
      case CARD_ID_FOREST:
      case CARD_ID_MOUNTAIN:
      case CARD_ID_PLAINS:
      case CARD_ID_SNOW_COVERED_SWAMP:
      case CARD_ID_SNOW_COVERED_ISLAND:
      case CARD_ID_SNOW_COVERED_FOREST:
      case CARD_ID_SNOW_COVERED_MOUNTAIN:
      case CARD_ID_SNOW_COVERED_PLAINS:
      case CARD_ID_WASTES:

	// not basic lands, but still basic
      case CARD_ID_RELENTLESS_RATS:
      case CARD_ID_SHADOWBORN_APOSTLE:
	return true;

      default:
	return false;
    }
}

static DeckType
check_deck_type(void)
{
  DeckType rval = DT_UNKNOWN;

  bool any_duplicate_nonbasics = false;
  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    if (!check_basic(global_edited_deck[i].DeckEntry_csvid))
      {
	if (global_edited_deck[i].DeckEntry_Amount > 1)
	  any_duplicate_nonbasics = true;
	if (check_ante(global_edited_deck[i].DeckEntry_csvid))
	  rval |= DT_HAS_ANTE;
      }

  if (!any_duplicate_nonbasics)
    return rval | DT_HIGHLANDER;

  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    {
      csvid_t csvid = global_edited_deck[i].DeckEntry_csvid;
      if (check_basic(csvid))
	continue;

      if (check_banned(csvid))
	{
	  rval |= DT_UNRESTRICTED;
	  continue;
	}

      int num = global_edited_deck[i].DeckEntry_Amount;
      if (num > 4)
	{
	  rval |= DT_UNRESTRICTED;
	  continue;
	}

      if (!check_restricted(csvid))
	rval |= DT_TOURNAMENT_T1_5;
      else if (num <= 1)
	rval |= DT_RESTRICTED_T1;
      else
	rval |= DT_WILD;
    }

  return rval;
}

static void
show_stats(HDC hdc, int word_width, int word_height)
{
  //[[[ prep_stats()
  enum Stat1
  {
    STAT1_MANASOURCE	= 0,
    STAT1_CREATURE	= 1,
    STAT1_ENCHANTMENT	= 2,
    STAT1_SORCERY	= 3,
    STAT1_INSTANT	= 4,
    STAT1_INTERRUPT	= 5,
    STAT1_LAND		= 6,
    STAT1_ARTIFACT	= 7,
    STAT1_TOTAL		= 8,

    STAT1_0		= 0,
    STAT1_MAX		= STAT1_TOTAL,
  };

  enum Stat2
  {
    STAT2_BLACK		= 0,
    STAT2_BLUE		= 1,
    STAT2_GREEN		= 2,
    STAT2_RED		= 3,
    STAT2_WHITE		= 4,
    STAT2_COLORLESS	= 5,
    STAT2_MULTI		= 6,
    STAT2_TOTAL		= STAT2_MULTI,

    STAT2_0		= 0,
    STAT2_MAX		= STAT2_TOTAL,
  };

  int stats[STAT1_MAX + 1][STAT2_MAX + 1];
  memset(stats, 0, sizeof stats);

  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    {
      csvid_t csvid = global_edited_deck[i].DeckEntry_csvid;
      int amt = global_edited_deck[i].DeckEntry_Amount;
      const card_ptr_t* cp = &cards_ptr[csvid.raw];

      Stat1 stat1;
      switch (cp->card_type)
	{
	  case CP_TYPE_CREATURE:	stat1 = STAT1_CREATURE;		break;
	  case CP_TYPE_ENCHANTMENT:	stat1 = STAT1_ENCHANTMENT;	break;
	  case CP_TYPE_SORCERY:		stat1 = STAT1_SORCERY;		break;
	  case CP_TYPE_INSTANT:		stat1 = STAT1_INSTANT;		break;
	  case CP_TYPE_INTERRUPT:	stat1 = STAT1_INTERRUPT;	break;
	  case CP_TYPE_LAND:		stat1 = STAT1_LAND;		break;
	  case CP_TYPE_ARTIFACT:
	    stat1 = STAT1_ARTIFACT;
	    for (int j = 0; j < 7; ++j)
	      if (cp->types[j] == SUBTYPE_CREATURE)
		{
		  stat1 = STAT1_CREATURE;
		  break;
		}
	    break;
	  default:			continue;
	}

      if (cp->mana_source_colors & (COLOR_TEST_ANY_COLORED | COLOR_TEST_COLORLESS | COLOR_TEST_ARTIFACT))
	{
	  int add[STAT2_MAX + 1] = {0};
	  if (cp->mana_source_colors & (COLOR_TEST_COLORLESS | COLOR_TEST_ARTIFACT))
	    add[STAT2_COLORLESS] += amt;
	  if (cp->mana_source_colors & COLOR_TEST_BLACK)
	    add[STAT2_BLACK] += amt;
	  if (cp->mana_source_colors & COLOR_TEST_BLUE)
	    add[STAT2_BLUE] += amt;
	  if (cp->mana_source_colors & COLOR_TEST_GREEN)
	    add[STAT2_GREEN] += amt;
	  if (cp->mana_source_colors & COLOR_TEST_RED)
	    add[STAT2_RED] += amt;
	  if (cp->mana_source_colors & COLOR_TEST_WHITE)
	    add[STAT2_WHITE] += amt;

	  add[STAT2_TOTAL] += amt;

	  for (int j = STAT2_0; j <= STAT2_MAX; ++j)
	    stats[STAT1_MANASOURCE][j] += add[j];

	  if (stat1 == STAT1_LAND)
	    {
	      for (int j = STAT2_0; j <= STAT2_MAX; ++j)
		stats[STAT1_LAND][j] += add[j];
	      continue;
	    }
	}

      Stat2 stat2;
      switch (cp->color)
	{
	  case CP_COLOR_BLACK:		stat2 = STAT2_BLACK;	break;
	  case CP_COLOR_BLUE:		stat2 = STAT2_BLUE;	break;
	  case CP_COLOR_GREEN:		stat2 = STAT2_GREEN;	break;
	  case CP_COLOR_RED:		stat2 = STAT2_RED;	break;
	  case CP_COLOR_WHITE:		stat2 = STAT2_WHITE;	break;
	  case CP_COLOR_MULTI:
	    stat2 = STAT2_MULTI;
	    if (cp->req_black || (cp->req_hybrid && (cp->hybrid_type & COLOR_TEST_BLACK)))
	      stats[stat1][STAT2_BLACK] += amt;
	    if (cp->req_blue || (cp->req_hybrid && (cp->hybrid_type & COLOR_TEST_BLUE)))
	      stats[stat1][STAT2_BLUE] += amt;
	    if (cp->req_green || (cp->req_hybrid && (cp->hybrid_type & COLOR_TEST_GREEN)))
	      stats[stat1][STAT2_GREEN] += amt;
	    if (cp->req_red || (cp->req_hybrid && (cp->hybrid_type & COLOR_TEST_RED)))
	      stats[stat1][STAT2_RED] += amt;
	    if (cp->req_white || (cp->req_hybrid && (cp->hybrid_type & COLOR_TEST_WHITE)))
	      stats[stat1][STAT2_WHITE] += amt;
	    break;
	  default:			stat2 = STAT2_COLORLESS;break;
	}
      if (stat2 != STAT2_MULTI)
	stats[stat1][stat2] += amt;

      stats[stat1][STAT2_TOTAL] += amt;
    }

  for (int i = STAT1_0 + 1; i < STAT1_MAX; ++i)
    for (int j = STAT2_0; j <= STAT2_MAX; ++j)
      stats[STAT1_TOTAL][j] += stats[i][j];
  //]]]

  //[[[ show_stats_text(), show_stats_values()
  load_text("Menus", "STATSSCREEN");
  SetTextColor(hdc, global_colorref_lightgrey);

  // Column headers
  int x = 40;
  int y = 40;
  textout(hdc, x, y, text_lines[0]);

  x += word_width;
  int stepx = (1000 - x) / 8;
  x += 80;
  int posx = x;
  for (int c = 1; c <= 7; ++c)
    {
      textout(hdc, x, y, text_lines[c]);
      x += stepx;
    }

  int stepy = 65;
  y = 105;
  for (int l = STAT1_0; l <= STAT1_TOTAL; ++l, y += stepy)
    {
      // Row header
      x = 40;
      int textline = l == STAT1_TOTAL ? 7 : (8 + l);
      SetTextColor(hdc, l == STAT1_MANASOURCE ? global_colorref_lavender : global_colorref_lightgrey);
      textout(hdc, x, y, text_lines[textline]);

      // Extra word below "Non-creature".
      if (l == STAT1_ARTIFACT)
	textout(hdc, x, y + word_height, text_lines[textline + 1]);

      // Values
      x = posx;
      if (l != STAT1_MANASOURCE)
	SetTextColor(hdc, global_colorref_flesh);
      for (int c = STAT2_0; c <= STAT2_TOTAL; ++c, x += stepx)
	if (stats[l][c] == 0)
	  textout(hdc, x, y, " " DASH);
	else
	  {
	    int div;
	    if (c == STAT2_TOTAL)
	      div = stats[STAT1_TOTAL][STAT2_TOTAL];
	    else
	      div = stats[l][STAT2_TOTAL];

	    if (div)
	      div = (100 * stats[l][c] + div/2) / div;

	    textoutf(hdc, x, y, "(%d) %d%%", stats[l][c], div);
	  }

      // Extra space below "Mana Sources"
      if (l == STAT1_MANASOURCE)
	y += word_height / 2;

      // Extra space below "Non-creature".
      if (l == STAT1_ARTIFACT)
	y += word_height;
    }
  //]]]
}

static void
fill_stats_window(HDC hdc, RECT r, HFONT font)
{
  SetMapMode(hdc, MM_ANISOTROPIC);
  SetWindowExtEx(hdc, 1000, 750, NULL);
  SetViewportExtEx(hdc, r.right - r.left, r.bottom - r.top, NULL);
  SelectObject(hdc, font);
  SetBkMode(hdc, TRANSPARENT);

  SIZE sz;
  GetTextExtentPoint32(hdc, "Enchantments", strlen("Enchantments"), &sz);

  show_stats(hdc, sz.cx, sz.cy);
}

INT_PTR CALLBACK
dlgproc_DeckStats(HWND hdlg, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_INITDIALOG:
	char txt[512];	// 128 + 2 + 32 + 128 + 3 + 127 = 420, and might as well be a bit wider just in case
	char* p;
	load_text("Menus", "STATSDIALOG");
	p = txt + sprintf(txt, "%s: %s " DASH " ", text_lines[0], global_deckname);

	load_text("Menus", "MULTIDECKTYPES");
	DeckType decktype;
	const char* decktxt;
	decktype = check_deck_type();
	if (decktype & DT_UNRESTRICTED)
	  decktxt = text_lines[1];
	else if (decktype & DT_WILD)
	  decktxt = text_lines[2];
	else if (decktype & DT_RESTRICTED_T1)
	  decktxt = text_lines[3];
	else if (decktype & DT_TOURNAMENT_T1_5)
	  decktxt = text_lines[4];
	else if (decktype & DT_HIGHLANDER)
	  decktxt = text_lines[5];
	else
	  decktxt = text_lines[0];

	p += sprintf(p, "%s", decktxt);

	if (decktype & DT_HAS_ANTE)
	  p += sprintf(p, " / %s", &text_lines[7][1]);

	SetWindowText(hdlg, txt);

	return 0;

      case WM_ERASEBKGND:
	HDC hdc, chdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	chdc = CreateCompatibleDC(hdc);
	gdi_flush(chdc);

	SelectObject(chdc, global_pics[33]);

	RECT r;
	GetClientRect(hdlg, &r);

	BITMAP bmp;
	GetObject(global_pics[33], sizeof(BITMAP), &bmp);

	for (int x = 0; x < r.right; x += bmp.bmWidth)
	  for (int y = 0; y < r.bottom; y += bmp.bmHeight)
	    BitBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, chdc, 0, 0, SRCCOPY);

	fill_stats_window(hdc, r, global_font_32);

	DELETE_DC(chdc);

	return 1;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<INT_PTR>(GetStockObject(HOLLOW_BRUSH));

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_BUTTON_OK || LOWORD(wparam) == RES_BUTTON_CANCEL)
	  EndDialog(hdlg, 0);
	return 1;

      default:
	return 0;
    }
}
//]]]

//[[[ packs
static void
clear_packs(void)
{
  for (int c = 0; c <= PACK1_MAX; ++c)
    for (int t = 0; t <= PACK2_MAX; ++t)
      global_packs[c][t].clear();
}

static void
clear_packs_copy(void)
{
  for (int c = 0; c <= PACK1_MAX; ++c)
    for (int t = 0; t <= PACK2_MAX; ++t)
      global_packs_copy[c][t].clear();
}

static void
count_packs(void)
{
  clear_packs();

  for (int l = 0; l < global_edited_deck_num_entries; ++l)
    {
      int col, typ;
      csvid_t csvid = global_edited_deck[l].DeckEntry_csvid;
      const card_ptr_t* cp = &cards_ptr[csvid.raw];

      switch (cp->color)
	{
	  case CP_COLOR_RED:	col = PACK1_RED;	break;
	  case CP_COLOR_BLACK:	col = PACK1_BLACK;	break;
	  case CP_COLOR_BLUE:	col = PACK1_BLUE;	break;
	  case CP_COLOR_GREEN:	col = PACK1_GREEN;	break;
	  case CP_COLOR_WHITE:	col = PACK1_WHITE;	break;
	  default:		col = PACK1_OTHER;	break;
	}

      switch (cp->card_type)
	{
	  case CP_TYPE_CREATURE:	typ = PACK2_CREATURE;	break;
	  case CP_TYPE_ENCHANTMENT:	typ = PACK2_ENCHANTMENT;break;
	  case CP_TYPE_SORCERY:		typ = PACK2_SORCERY;	break;
	  case CP_TYPE_INTERRUPT:	typ = PACK2_INTERRUPT;	break;
	  case CP_TYPE_INSTANT:		typ = PACK2_INSTANT;	break;

	  case CP_TYPE_LAND:	// weird case here
	    typ = PACK2_LAND;
	    switch (cp->id)
	      {
		case CARD_ID_MOUNTAIN:	col = PACK1_RED;	break;
		case CARD_ID_PLAINS:	col = PACK1_WHITE;	break;
		case CARD_ID_FOREST:	col = PACK1_GREEN;	break;
		case CARD_ID_SWAMP:	col = PACK1_BLACK;	break;
		case CARD_ID_ISLAND:	col = PACK1_BLUE;	break;
		default:		col = PACK1_OTHER;	break;
	      }
	    break;

	  default:		// even weirder case here
	    col = PACK1_OTHER;
	    typ = PACK2_INSTANT;
	    break;
	}

      global_packs[col][typ].add(csvid, global_edited_deck[l].DeckEntry_Amount);
    }
}
//]]]

//[[[ CueCardClass
LRESULT CALLBACK
wndproc_CueCardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_PAINT:
	PAINTSTRUCT paint;
	if (HDC paintdc = BeginPaint(hwnd, &paint))
	  {
	    HFONT font = reinterpret_cast<HFONT>(GetWindowLong(hwnd, CUECARD_FONT_INDEX));

	    gdi_flush(paintdc);

	    RECT r;
	    GetClientRect(hwnd, &r);

	    HBRUSH brush1 = CreateSolidBrush(PALETTERGB(127, 127, 127));
	    FillRect(paintdc, &r, brush1);

	    r.right -= 2;
	    r.bottom -= 2;
	    HBRUSH brush2 = CreateSolidBrush(PALETTERGB(210, 190, 150));
	    FillRect(paintdc, &r, brush2);

	    SetTextColor(paintdc, PALETTERGB(0, 0, 0));
	    SetBkMode(paintdc, TRANSPARENT);
	    char txt[100];
	    GetWindowText(hwnd, txt, 100);
	    SelectObject(paintdc, font);
	    DrawText(paintdc, txt, -1, &r, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

	    EndPaint(hwnd, &paint);

	    DELETE_OBJ(brush1);
	    DELETE_OBJ(brush2);
	  }
	return 0;

      case WM_CREATE:
	if (HFONT font = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, ANSI_CHARSET,
				    OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH|FF_DONTCARE, "Verdana"))
	  {
	    SetWindowLong(hwnd, CUECARD_FONT_INDEX, reinterpret_cast<int>(font));
	    return 0;
	  }
	else
	  return -1;

      case WM_DESTROY:
	if (HFONT font = reinterpret_cast<HFONT>(GetWindowLong(hwnd, CUECARD_FONT_INDEX)))
	  DeleteObject(font);
	return 0;

      case WM_KEYDOWN:
	SetFocus(global_main_hwnd);
	PostMessage(GetFocus(), WM_KEYDOWN, wparam, lparam);
	return 0;

      case 0x8400:
	HFONT font;
	font = reinterpret_cast<HFONT>(GetWindowLong(hwnd, CUECARD_FONT_INDEX));

	int x, y;
	x = LOWORD(wparam);
	y = HIWORD(wparam);

	char* str;
	str = reinterpret_cast<char*>(lparam);

	if (HDC hdc = GetDC(hwnd))
	  {
	    gdi_flush(hdc);
	    SelectObject(hdc, font);

	    SIZE sz;
	    GetTextExtentPoint32(hdc, str, strlen(str), &sz);
	    int width = sz.cx + 10;
	    int height = sz.cy + 6;

	    ReleaseDC(hwnd, hdc);

	    RECT r;
	    GetClientRect(GetParent(hwnd), &r);
	    if (x > r.right - width)
	      x = r.right - width;

	    MoveWindow(hwnd, x, y, width, height, TRUE);
	    SetWindowText(hwnd, str);
	    ShowWindow(hwnd, SW_SHOW);
	    InvalidateRect(hwnd, NULL, TRUE);
	  }
	return 0;

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ TitleClass
LRESULT CALLBACK
wndproc_TitleClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
    {
      case WM_PAINT:
	PAINTSTRUCT paint;
	HDC paintdc;
	paintdc = BeginPaint(hwnd, &paint);
	gdi_flush(paintdc);

	RECT rect;
	GetClientRect(hwnd, &rect);
	InflateRect(&rect, -5, -5);

	if (strlen(global_deckname) >= 20)
	  SelectObject(paintdc, global_font_28percent);
	else
	  SelectObject(paintdc, global_font_40percent);

	SetBkMode(paintdc, TRANSPARENT);
	SetMapMode(paintdc, MM_ANISOTROPIC);

	SetWindowExtEx(paintdc, RECT_WIDTH(rect), RECT_HEIGHT(rect), NULL);
	SetViewportExtEx(paintdc, RECT_WIDTH(rect), RECT_HEIGHT(rect), NULL);

	rect.left += 3;
	rect.top += 3;
	SetTextColor(paintdc, PALETTERGB(20, 46, 77));
	DrawText(paintdc, global_deckname, strlen(global_deckname), &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

	rect.left -= 3;
	rect.top -= 3;
	SetTextColor(paintdc, PALETTERGB(243, 209, 175));
	DrawText(paintdc, global_deckname, strlen(global_deckname), &rect, DT_SINGLELINE|DT_VCENTER|DT_CENTER);

	EndPaint(hwnd, &paint);
	return 0;

      case WM_CREATE:
      case WM_DESTROY:
	return 0;

      case WM_ERASEBKGND:
	HDC hdc, chdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	chdc = CreateCompatibleDC(hdc);
	gdi_flush(chdc);

	SelectObject(chdc, global_pics[17]);

	RECT r;
	GetClientRect(hwnd, &r);

	BITMAP bmp;
	GetObject(global_pics[17], sizeof(BITMAP), &bmp);

	StretchBlt(hdc, 0, 0, r.right, r.bottom, chdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);

	DELETE_DC(chdc);
	return 1;

      case WM_LBUTTONDOWN:
      case WM_RBUTTONDOWN:
	if (!(global_db_flags_1 & (DBFLAGS_EDITDECK|DBFLAGS_SHANDALAR)))
	  {
	    show_dialog_deckinfo();
	    InvalidateRect(hwnd, NULL, TRUE);
	  }
	return 0;

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ FullCardClass and dependencies
static void
fill_back(HWND hwnd, HDC hdc)
{
  HDC chdc = CreateCompatibleDC(hdc);
  gdi_flush(chdc);
  SelectObject(chdc, global_pics[16]);

  RECT rect;
  GetClientRect(hwnd, &rect);

  BITMAP bmp;
  GetObject(global_pics[16], sizeof(BITMAP), &bmp);

  for (int x = 0; x < rect.right; x += bmp.bmWidth)
    for (int y = rect.bottom; y > -29; y -= bmp.bmHeight)
      BitBlt(hdc, x, y, bmp.bmWidth, bmp.bmHeight, chdc, 0, 0, SRCCOPY);

  DELETE_DC(chdc);
}

LRESULT CALLBACK
wndproc_FullCardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  int card_to_draw, expanded_text, wanted_card;
  switch (msg)
    {
      case WM_PAINT:
	card_to_draw = GetWindowLong(hwnd, FULLCARD_CSVID_INDEX);
	expanded_text = GetWindowLong(hwnd, FULLCARD_EXPANDTEXT_INDEX);

	PAINTSTRUCT paint;
	HDC paintdc;
	paintdc = BeginPaint(hwnd, &paint);

	RECT rect;
	GetClientRect(hwnd, &rect);

	gdi_flush(paintdc);

	if (card_to_draw >= 0)
	  DrawFullCard(paintdc, &rect, &cards_ptr[card_to_draw], 0, 1, expanded_text, 0);
	else
	  fill_back(hwnd, paintdc);

	EndPaint(hwnd, &paint);
	return 0;

      case WM_CREATE:
	global_fullcard_popup = CreatePopupMenu();
	load_text("Menus", "FULLCARD");
	AppendMenu(global_fullcard_popup, MF_STRING, RES_FULLCARDMENU_EXPAND, text_lines[0]);
	SetWindowLong(hwnd, FULLCARD_CSVID_INDEX, -1);
	SetWindowLong(hwnd, FULLCARD_EXPANDTEXT_INDEX, global_cfg_expand_text);
	CHECKMENU_IF(global_fullcard_popup, RES_FULLCARDMENU_EXPAND, global_cfg_expand_text);
	return 0;

      case WM_DESTROY:
	global_cfg_expand_text = GetWindowLong(hwnd, FULLCARD_EXPANDTEXT_INDEX);
	DESTROY_MENU(global_fullcard_popup);
	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);
	fill_back(hwnd, hdc);
	return 1;

      case WM_RBUTTONDOWN:
	POINT point;
	point.x = GET_X_LPARAM(lparam);
	point.y = GET_Y_LPARAM(lparam);
	ClientToScreen(hwnd, &point);

	TrackPopupMenu(global_fullcard_popup, TPM_RIGHTBUTTON, point.x, point.y, 0, hwnd, NULL);
	return 0;

      case WM_COMMAND:
	if (LOWORD(wparam) == RES_FULLCARDMENU_EXPAND)
	  SendMessage(hwnd, 0x8401, SendMessage(hwnd, 0x8402, 0, 0) ? 0 : 1, 0);
	return 0;

      case 0x8400:
	card_to_draw = wparam;
	if (GetWindowLong(hwnd, FULLCARD_CSVID_INDEX) != card_to_draw)
	  {
	    if (card_to_draw < 0 && global_edited_deck[0].DeckEntry_Amount > 0)
	      card_to_draw = global_edited_deck[0].DeckEntry_csvid.raw;
	    SetWindowLong(hwnd, FULLCARD_CSVID_INDEX, card_to_draw);
	    InvalidateRect(hwnd, NULL, FALSE);
	  }
	return 0;

      case 0x8401:
	expanded_text = wparam;
	SetWindowLong(hwnd, FULLCARD_EXPANDTEXT_INDEX, expanded_text);
	InvalidateRect(hwnd, NULL, FALSE);
	CHECKMENU_IF(global_fullcard_popup, RES_FULLCARDMENU_EXPAND, expanded_text);
	return expanded_text;

      case 0x8402:
	return GetWindowLong(hwnd, FULLCARD_EXPANDTEXT_INDEX);

      case 0x8466:
	wanted_card = wparam;
	if (wanted_card == GetWindowLong(hwnd, FULLCARD_CSVID_INDEX))
	  InvalidateRect(hwnd, NULL, FALSE);
	return 0;

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ SearchClass and dependencies
static void
update_scroll_range(void)
{
  int r = GetWindowLong(global_horzlist_hwnd, HORZLIST_LASTPIC_INDEX) - GetWindowLong(global_horzlist_hwnd, HORZLIST_PICSINLINE_INDEX);
  if (r < 0)
    r = 0;
  SetScrollRange(global_horzlist_hwnd, 0, 0, r, 0);
}

static void
filter_cards_in_lists(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  int data = SendMessage(hwnd_listbox, LB_GETITEMDATA, SendMessage(hwnd_listbox, LB_GETCURSEL, 0, 0), 0);

  SendMessage(hwnd_listbox, LB_RESETCONTENT, 0, 0);
  SendMessage(hwnd_horzlist, 0x8006, 0, 0);

  UpdateWindow(hwnd_listbox);
  UpdateWindow(hwnd_horzlist);

  int newsel = -1;
  const card_ptr_t* cp = &cards_ptr[0];
  for (csvid_t csvid(0); csvid.raw < global_available_slots; ++csvid.raw, ++cp)
    if (check_filters(csvid) && (global_search_string[0] == '\0'
				 || (global_search_string[0] == '*' && global_search_string[1] == '\0')
				 || PathMatchSpec(cp->name, global_search_string)
				 || PathMatchSpec(cp->rules_text, global_search_string)))
      {
	int idx = SendMessage(hwnd_listbox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(cp->full_name));
	SendMessage(hwnd_listbox, LB_SETITEMDATA, idx, csvid.raw);
	if (csvid.raw == data)
	  newsel = idx;
      }

  if (newsel == -1)
    newsel = 0;

  SendMessage(hwnd_listbox, LB_SETCURSEL, newsel, 0);

  SendMessage(GetParent(hwnd_listbox), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd_listbox), 1), reinterpret_cast<LPARAM>(hwnd_listbox));
  UpdateWindow(hwnd_listbox);

  int count = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);
  for (int i = 0; i < count; ++i)
    SendMessage(hwnd_horzlist, 0x8001, 0, SendMessage(hwnd_listbox, LB_GETITEMDATA, i, 0));

  SendMessage(hwnd_horzlist, 0x8007, 0, 0);

  SendMessage(GetParent(hwnd_horzlist), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd_horzlist), 1), reinterpret_cast<LPARAM>(hwnd_horzlist));
  update_scroll_range();
}

static void
make_new_search(HWND hwnd)
{
  global_search_string[256] = 0;
  global_search_string[0] = '*';

  SendMessage(hwnd, WM_GETTEXT, 255, reinterpret_cast<LPARAM>(&global_search_string[1]));

  char* p = &global_search_string[strlen(global_search_string)];
  if (p == global_search_string)
    strcpy(global_search_string, "*");
  else if (*(p - 1) != '*')
    strcpy(p, "*");

  SendMessage(global_horzlist_hwnd, 0x8007, 0, 0);
  filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);
}

LRESULT CALLBACK
wndproc_SearchClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg == WM_KILLFOCUS)
    make_new_search(hwnd);
  else if (msg == WM_CHAR && wparam == '\r')
    {
      make_new_search(hwnd);
      SetFocus(global_horzlist_hwnd);
      return 0;
    }
  return global_wndproc_std_SearchClass(hwnd, msg, wparam, lparam);
}
//]]]

//[[[ HorzListClass and dependencies
static void
insert_cards_into_deck(csvid_t csvid, int num, DeckEntry* tgt_deck)
{
  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    if (tgt_deck[i].DeckEntry_csvid == csvid)
      {
	tgt_deck[i].DeckEntry_Amount += num;
	global_deck_num_cards += num;
	return;
      }

  tgt_deck[global_edited_deck_num_entries++] = {csvid, num, cards_ptr[csvid.raw].full_name};
  global_deck_num_cards += num;
}

static void
remove_cards_from_deck(csvid_t csvid, int num, DeckEntry* tgt_deck)
{
  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    if (tgt_deck[i].DeckEntry_csvid == csvid)
      {
	if (tgt_deck[i].DeckEntry_Amount == num)
	  {
	    global_deck_num_cards -= num;
	    --global_edited_deck_num_entries;
	    for (; i < global_edited_deck_num_entries; ++i)
	      tgt_deck[i] = tgt_deck[i + 1];
	    return;
	  }

	if (tgt_deck[i].DeckEntry_Amount > num)
	  {
	    tgt_deck[i].DeckEntry_Amount -= num;
	    global_deck_num_cards -= num;
	    return;
	  }

	num -= tgt_deck[i].DeckEntry_Amount;
	global_deck_num_cards -= tgt_deck[i].DeckEntry_Amount;

	for (int j = i; j < global_edited_deck_num_entries - 1; ++j)
	  tgt_deck[j] = tgt_deck[j + 1];

	--global_edited_deck_num_entries;
      }
}

static bool
delete_card_from_global_deck(csvid_t csvid, int num)
{
  int i;
  for (i = 0;; ++i)
    {
      if (i >= global_deck_num_entries)
	return false;

      if (global_deck[i].GDE_csvid == csvid && global_deck[i].GDE_Available == num)
	break;
    }

  if (!num)
    remove_cards_from_deck(csvid, 1, global_edited_deck);

  for (++i; i < global_deck_num_entries; ++i)
    global_deck[i - 1] = global_deck[i];

  --global_deck_num_entries;

  return true;
}

static void
move_colors_fromto_global_deck(bool move_into, int dlgbits)
{
#pragma message "Replaced in Shandalar"

  color_test_t dlgcolors = static_cast<color_test_t>(dlgbits << 1) & (COLOR_TEST_ANY_COLORED | COLOR_TEST_ARTIFACT);

  if (!dlgcolors)
    return;

  for (int i = 0; i < global_deck_num_entries; ++i)
    {
      GlobalDeckEntry* gd = &global_deck[i];
      if (gd->GDE_Available != (move_into ? 1 : 0))
	continue;

      const card_ptr_t* cp = &cards_ptr[gd->GDE_csvid.raw];
      if (global_colors_match_fn ? global_colors_match_fn(gd->GDE_iid, dlgcolors)
	  : (((dlgcolors & COLOR_TEST_BLACK)	&& (cp->color == CP_COLOR_BLACK	|| gd->GDE_iid.raw == 0))	// swamp
	     || ((dlgcolors & COLOR_TEST_BLUE)	&& (cp->color == CP_COLOR_BLUE	|| gd->GDE_iid.raw == 1))	// island
	     || ((dlgcolors & COLOR_TEST_GREEN)	&& (cp->color == CP_COLOR_GREEN	|| gd->GDE_iid.raw == 2))	// forest
	     || ((dlgcolors & COLOR_TEST_RED)	&& (cp->color == CP_COLOR_RED	|| gd->GDE_iid.raw == 3))	// mountain
	     || ((dlgcolors & COLOR_TEST_WHITE)	&& (cp->color == CP_COLOR_WHITE	|| gd->GDE_iid.raw == 4))	// plains
	     || ((dlgcolors & COLOR_TEST_ARTIFACT) && cp->card_type == CP_TYPE_ARTIFACT)))
	{
	  if (move_into)
	    {
	      gd->GDE_Available = 0;
	      gd->GDE_DecksBits |= 1 << global_current_deck;
	      insert_cards_into_deck(gd->GDE_csvid, 1, global_edited_deck);
	    }
	  else
	    {
	      gd->GDE_Available = 1;
	      gd->GDE_DecksBits &= ~(1 << global_current_deck);
	      remove_cards_from_deck(gd->GDE_csvid, 1, global_edited_deck);
	    }
	}
    }
}

static void
set_smallcard_dimensions(void)
{
  int num = global_cfg_consolidate ? global_edited_deck_num_entries : global_deck_num_cards;

  int w, h;
  if (num < 75)
    {
      w = global_smallcard_normal_size.x;
      h = global_smallcard_normal_size.y;
    }
  else if (num < 132)
    {
      w = global_smallcard_smaller_size.x;
      h = global_smallcard_smaller_size.y;
    }
  else
    {
      w = global_smallcard_smallest_size.x;
      h = global_smallcard_smallest_size.y;
    }

  global_smallcard_width = w;
  global_smallcard_height = h;
}

static int
check_colors_inout_edited_deck(bool currently_in)
{
  if (global_check_colors_inout_edited_deck_fn)
    return global_check_colors_inout_edited_deck_fn(global_deck, global_deck_num_entries, currently_in);

  int dlgbits = 0;
  int currently_out = currently_in ? 0 : 1;
  for (int i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_Available == currently_out)
      {
	const card_ptr_t* cp = &cards_ptr[global_deck[i].GDE_csvid.raw];
	switch (cp->color)
	  {
	    case CP_COLOR_BLACK:	dlgbits |= 1;	break;
	    case CP_COLOR_BLUE:		dlgbits |= 2;	break;
	    case CP_COLOR_GREEN:	dlgbits |= 4;	break;
	    case CP_COLOR_RED:		dlgbits |= 8;	break;
	    case CP_COLOR_WHITE:	dlgbits |= 0x10;break;
	    default:					break;
	  }
	if (cp->card_type == CP_TYPE_ARTIFACT)
	  dlgbits |= 0x20;
	switch (global_deck[i].GDE_iid.raw)
	  {
	    case 0:	/*swamp*/	dlgbits |= 1;	break;
	    case 1:	/*island*/	dlgbits |= 2;	break;
	    case 2:	/*forest*/	dlgbits |= 4;	break;
	    case 3:	/*mountain*/	dlgbits |= 8;	break;
	    case 4:	/*plains*/	dlgbits |= 0x10;break;
	    default:					break;
	  }
      }
  return dlgbits;
}

static int
count_card_amount_outside_edited_deck(csvid_t csvid)
{
  int count = 0;
  for (int i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid
	&& !(global_deck[i].GDE_DecksBits & (1 << global_current_deck)))
      ++count;

  return count;
}

static void
draw_small_amount(int amount, RECT* rect)
{
  int old_dc = SaveDC(global_hdc);

  SetMapMode(global_hdc, MM_ANISOTROPIC);
  SetWindowExtEx(global_hdc, 200, 280, 0);
  SetViewportExtEx(global_hdc, rect->right - rect->left, rect->bottom - rect->top, NULL);
  SetViewportOrgEx(global_hdc, rect->left, rect->top, NULL);

  SetBkMode(global_hdc, TRANSPARENT);

  SelectObject(global_hdc, global_font_42);
  SetTextAlign(global_hdc, TA_RIGHT);

  SetTextColor(global_hdc, RGB(0, 0, 0));
  textoutf(global_hdc, 195, 3, "%d", amount);

  SetTextColor(global_hdc, PALETTERGB(0xFF, 0xD9, 0x27));
  textoutf(global_hdc, 192, 0, "%d", amount);

  RestoreDC(global_hdc, old_dc);
}

static iid_t
get_iid_from_global_deck_card(csvid_t csvid)
{
  for (int i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid)
      return global_deck[i].GDE_iid;
  return iid_t(-1);
}

static bool
change_global_deck_card_availability(csvid_t csvid, int mode)
{
  int old_avail, new_avail;
  if (mode == 0)
    {
      old_avail = 0;
      new_avail = 2;
    }
  else if (mode == 1)
    {
      old_avail = 1;
      new_avail = 2;
    }
  else if (mode == 2)
    {
      old_avail = 2;
      new_avail = 0;
    }
  else if (mode == 3)
    {
      old_avail = 2;
      new_avail = 1;
    }
  else
    return false;

  for (int i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid
	&& global_deck[i].GDE_Available == old_avail)
      {
	global_deck[i].GDE_Available = new_avail;

	if (new_avail == 0)
	  global_deck[i].GDE_DecksBits |= 1 << global_current_deck;
	else if (new_avail == 1)
	  global_deck[i].GDE_DecksBits &= ~(1 << global_current_deck);

	return true;
      }

  return false;
}

static void
TENTATIVE_remove_selected_from_horzlist(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  int cursel = SendMessage(hwnd_listbox, LB_GETCURSEL, 0, 0);
  int count = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);

  for (int n = 0; n < count; )
    if (check_filters(csvid_t(static_cast<int>(SendMessage(hwnd_listbox, LB_GETITEMDATA, n, 0)))))
      ++n;
    else
      {
	SendMessage(hwnd_listbox, LB_DELETESTRING, n, 0);
	SendMessage(hwnd_horzlist, 0x8002, n, 0);
	--count;

	if (n < cursel)
	  --cursel;
	else if (n == cursel)
	  cursel = 0;
      }

  if (SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0))
    {
      SendMessage(hwnd_listbox, LB_SETCURSEL, cursel, 0);
      SendMessage(hwnd_horzlist, 0x8007, cursel, 0);
      SendMessage(GetParent(hwnd_listbox), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd_listbox), 1), reinterpret_cast<LPARAM>(hwnd_listbox));
    }
}

static void
horzlist_prep_rectangle(HWND hwnd, int idx, RECT* rect)
{
  RECT r;
  GetClientRect(hwnd, &r);

  int w = GetWindowLong(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX);
  int lft = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);

  SetRect(rect,
	  r.left + w * (idx - lft), r.top,
	  r.left + w * (idx - lft + 1), r.bottom);
}

static void
TENTATIVE_scroll(HWND hwnd_listbox, HWND hwnd_horzlist)
{
  int selected = SendMessage(hwnd_listbox, LB_GETCURSEL, 0, 0);
  int data = SendMessage(hwnd_listbox, LB_GETITEMDATA, selected, 0);
  int numitems = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);

  uint8_t mem[global_available_slots];
  memset(mem, 0, global_available_slots);

  for (int i = 0; i < numitems; ++i)
    mem[SendMessage(hwnd_listbox, LB_GETITEMDATA, i, 0)] = 1;

  for (int i = 0; i < global_available_slots; ++i)
    {
      csvid_t csvid{cards_ptr[i].id};
      if (!mem[csvid.raw] && check_filters(csvid))
	{
	  int idx = SendMessage(hwnd_listbox, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(cards_ptr[i].full_name));
	  SendMessage(hwnd_listbox, LB_SETITEMDATA, idx, csvid.raw);
	  SendMessage(hwnd_horzlist, 0x8005, idx, csvid.raw);
	}
    }

  numitems = SendMessage(hwnd_listbox, LB_GETCOUNT, 0, 0);
  int found = 0;
  for (int i = 0; i < numitems; ++i)
    if (SendMessage(hwnd_listbox, LB_GETITEMDATA, i, 0) == data)
      {
	found = i;
	break;
      }

  SendMessage(hwnd_listbox, LB_SETCURSEL, found, 0);
  SendMessage(hwnd_horzlist, 0x8007, found, 0);
  SendMessage(GetParent(hwnd_listbox), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd_listbox), 1), reinterpret_cast<LPARAM>(hwnd_listbox));
}

static int
sub_40E8D0(csvid_t csvid, bool shifted)
{
  int v3;
  if (!shifted)
    {
      v3 = 1;
      if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR | 0x20) && change_global_deck_card_availability(csvid, 1))
	TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
    }
  else if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR | 0x20))
    {
      global_dlg_parameter = v3 = count_card_amount_outside_edited_deck(csvid);

      if (!show_dialog_movexcards())
	global_dlg_result = 0;

      v3 = MIN(v3, global_dlg_result);

      for (int i = 0; i < v3; ++i)
	change_global_deck_card_availability(csvid, 1);

      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
    }
  else
    {
      show_dialog_movexcards();
      v3 = global_dlg_result;
    }

  return v3;
}

static void
refresh_numofcards_text(void)
{
  char buf[80];

  load_text("Menus", "STATS");
  sprintf(buf, text_lines[0], global_deck_num_cards);
  SetWindowText(global_button_stats_hwnd, buf);
  InvalidateRect(global_button_stats_hwnd, NULL, FALSE);
}

static bool
sub_40E570(HWND hwnd, POINT p2, bool singleclick, bool shifted)
{
  static POINT pt;

  int currsel = SendMessage(hwnd, 0x8003, 0, 0);

  csvid_t csvid;
  csvid.raw = SendMessage(hwnd, 0x8004, currsel, 0);

  if (cards_ptr[csvid.raw].card_type == CP_TYPE_TOKEN)
    return 0;

  RECT r;
  horzlist_prep_rectangle(global_horzlist_hwnd, currsel, &r);
  if (!PtInRect(&r, p2))
    return 0;

  MapWindowPoints(global_horzlist_hwnd, global_main_hwnd, reinterpret_cast<POINT*>(&r), 2);

  int v8 = 0, v13 = 0;

  HWND v12;
  if (!singleclick)
    v12 = global_decksurface_hwnd;
  else
    {
      HWND card = CreateWindowEx(0, "CardClass", "Card",
				 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
				 r.left, r.top, global_smallcard_width, global_smallcard_height,
				 global_main_hwnd,
				 reinterpret_cast<HMENU>(1),
				 global_hinstance, reinterpret_cast<void*>(csvid.raw));
      if (!card)
	return false;
      BringWindowToTop(card);
      UpdateWindow(card);

      v8 = pt.x - r.left;
      v13 = pt.y - r.top;
      SendMessage(card, WM_SYSCOMMAND, SC_MOVE | 2, 0);
      DestroyWindow(card);

      GetCursorPos(&pt);
      v12 = WindowFromPoint(pt);
      while (v12 && v12 != global_main_hwnd && v12 != global_decksurface_hwnd && v12 != global_horzlist_hwnd)
	v12 = GetParent(v12);
    }

  if (v12 == global_decksurface_hwnd)
    {
      int amt = sub_40E8D0(csvid, shifted);
      set_smallcard_dimensions();
      if (global_cfg_effects)
	play_sound(2, 400, 0, 0);

      global_deck_was_edited = true;

      ScreenToClient(v12, &pt);
      pt.x -= v8;
      pt.y -= v13;

      for (int i = 0; i < amt; ++i)
	{
	  if (!SendMessage(v12, 0x84C8, csvid.raw, MAKELONG(pt.x, pt.y)))
	    MessageBeep(0);

	  change_global_deck_card_availability(csvid, 2);
	}

      count_packs();

      if (global_db_flags_1 & 0x20)
	{
	  SendMessage(global_horzlist_hwnd, 0x8007, 0, 0);
	  SendMessage(global_listbox_hwnd, LB_SETCURSEL, 0, 0);
	}
      else
	{
	  SendMessage(global_fullcard_hwnd, 0x8400, csvid.raw, 0);
	  SendMessage(global_horzlist_hwnd, 0x8007, currsel, 0);
	  SendMessage(global_listbox_hwnd, LB_SETCURSEL, currsel, 0);
	}

      if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
	SendMessage(global_horzlist_hwnd, 0x8466, csvid.raw, 0);
    }
  else
    {
      TENTATIVE_scroll(global_listbox_hwnd, global_horzlist_hwnd);
      SendMessage(global_fullcard_hwnd, 0x8400, csvid.raw, 0);
      SendMessage(global_horzlist_hwnd, 0x8007, currsel, 0);
      SendMessage(global_listbox_hwnd, LB_SETCURSEL, currsel, 0);
      if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR))
	SendMessage(global_horzlist_hwnd, 0x8466, csvid.raw, 0);
    }
  refresh_numofcards_text();
  return 1;
}

LRESULT CALLBACK
wndproc_HorzListClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static HDC comphdc = NULL;
  static int sellprice = 0;
  static csvid_t curr_csvid{-1};

  // common vars
  char buf[300];
  HDC hdc;
  RECT r;
  POINT p, p2;
  int last_pic_index, left_pic_index, pics_inline, currsel;
  int* horz_list_addr;

  switch (msg)
    {
      case WM_PAINT:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	left_pic_index = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);
	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);

	PAINTSTRUCT paint;
	hdc = BeginPaint(hwnd, &paint);
	gdi_flush(hdc);

	GetClientRect(hwnd, &r);
	BITMAP bmp;
	GetObject(global_pics[15], sizeof(BITMAP), &bmp);

	HGDIOBJ h;
	h = SelectObject(comphdc, global_pics[15]);
	StretchBlt(hdc, 0, 0, r.right, r.bottom, comphdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
	SelectObject(comphdc, h);

	int max_index;
	max_index = MIN(left_pic_index + pics_inline, last_pic_index);

	for (int i = left_pic_index; i < max_index; ++i)
	  {
	    horzlist_prep_rectangle(hwnd, i, &r);

	    RECT r2;
	    SetRect(&r2, 0, 0, global_smallcard_piclist_width, global_smallcard_piclist_height);
	    csvid_t csvid(horz_list_addr[i]);
	    int amt = count_card_amount_outside_edited_deck(csvid);
	    DrawSmallCard(global_hdc, &r2, &cards_ptr[csvid.raw], 0, 1, 0, 0);
	    if (amt > 1 && (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_SHANDALAR)))
	      draw_small_amount(amt, &r2);

	    r.left += (r.right - r.left - global_smallcard_piclist_width) / 2;
	    r.top += (r.bottom - r.top - global_smallcard_piclist_height) / 2;
	    BitBlt(hdc, r.left, r.top, global_smallcard_piclist_width, global_smallcard_piclist_height, global_hdc, 0, 0, SRCCOPY);
	  }
	EndPaint(hwnd, &paint);
	return 0;

      case WM_CREATE:
	pics_inline = 7;

	GetClientRect(hwnd, &r);
	if (global_smallcard_piclist_width > 0 && r.right)
	  {
	    pics_inline = (r.right - 4) / global_smallcard_piclist_width;
	    if ((r.right - 4) % global_smallcard_piclist_width >= global_smallcard_piclist_width / 4)
	      ++pics_inline;
	  }

	SetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX, 0);
	SetWindowLong(hwnd, HORZLIST_ADDR_INDEX, reinterpret_cast<int>(malloc(sizeof(int))));
	SetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX, -1);
	SetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX, 0);
	SetWindowLong(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX, global_smallcard_piclist_width);
	SetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX, pics_inline);

	update_scroll_range();

	SetScrollPos(hwnd, SB_HORZ, -1, TRUE);

	global_horzlist_popup = CreatePopupMenu();

	hdc = GetDC(hwnd);
	gdi_flush(hdc);
	comphdc = CreateCompatibleDC(hdc);
	gdi_flush(comphdc);
	ReleaseDC(hwnd, hdc);
	return 0;

      case WM_DESTROY:
	free(reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX)));
	DESTROY_MENU(global_horzlist_popup);
	DELETE_DC(comphdc);
	return 0;

      case WM_MOUSEMOVE:
	static POINT last_mousepos = {0, 0};

	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	int dx, dy;
	dx = abs(last_mousepos.x - p.x);
	dy = abs(last_mousepos.y - p.y);
	if (dx + dy < 2)
	  return 0;

	last_mousepos = p;

	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	left_pic_index = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);
	if (last_pic_index <= 0 || left_pic_index < 0)
	  return 0;

	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);

	for (int i = left_pic_index; i < left_pic_index + pics_inline; ++i)
	  {
	    horzlist_prep_rectangle(hwnd, i, &r);
	    if (PtInRect(&r, p))
	      {
		if (SendMessage(hwnd, 0x8003, 0, 0) != i)
		  {
		    SendMessage(hwnd, 0x8007, i, 0);
		    SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), 1), reinterpret_cast<LPARAM>(hwnd));
		  }
		break;
	      }
	  }

	return 0;

      case WM_MOUSEWHEEL:
	SendMessage(hwnd, WM_HSCROLL, SHIWORD(wparam) < 0 ? 1 : 0, 0);
	return 0;

      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
	if (!(global_db_flags_2 & DBFLAGS_SHANDALAR))
	  return 0;

	GetCursorPos(&p);
	p2 = p;
	ScreenToClient(global_main_hwnd, &p);
	ScreenToClient(global_horzlist_hwnd, &p2);

	if (msg == WM_LBUTTONDOWN)
	  {
	    MSG m;
	    Sleep(GetDoubleClickTime());
	    if (PeekMessage(&m, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, PM_NOREMOVE))
	      return 0;
	  }

	SetFocus(hwnd);
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	if (last_pic_index <= 0)
	  return 0;

	SendMessage(hwnd, WM_MOUSEMOVE, wparam & ~MK_LBUTTON, lparam);

	if (!sub_40E570(hwnd, p2, msg == WM_LBUTTONDOWN, wparam & MK_SHIFT))
	  MessageBeep(0);

	return 0;

      case WM_RBUTTONDOWN:
	if (!(global_db_flags_2 & DBFLAGS_STANDALONE))
	  return 0;

	SetFocus(hwnd);
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	if (last_pic_index <= 0)
	  return 0;

	SendMessage(hwnd, WM_MOUSEMOVE, wparam & ~MK_RBUTTON, lparam);

	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	currsel = SendMessage(hwnd, 0x8003, 0, 0);
	horzlist_prep_rectangle(global_horzlist_hwnd, currsel, &r);
	if (!PtInRect(&r, p))
	  return 0;

	ScreenToClient(global_main_hwnd, &p);

	{
	  curr_csvid.raw = SendMessage(hwnd, 0x8004, currsel, 0);
	  iid_t iid = get_iid_from_global_deck_card(curr_csvid);
	  if (!iid.ok())
	    return 0;

	  sellprice = SellPrice(iid.raw);
	}

	load_text("Menus", "SELLCARD");
	sprintf(buf, text_lines[0], sellprice);
	AppendMenu(global_horzlist_popup, MF_ENABLED, RES_SELLMENU_SELLCARDFORX, buf);

	ClientToScreen(hwnd, &p);
	TrackPopupMenu(global_horzlist_popup, TPM_RIGHTBUTTON, p.x + 10, p.y + 10, 0, hwnd, NULL);
	DeleteMenu(global_horzlist_popup, RES_SELLMENU_SELLCARDFORX, MF_BYCOMMAND);
	return 0;

      case WM_HSCROLL:
	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);

	int scrollpos;
	scrollpos = GetScrollPos(hwnd, 0);

	int minpos, maxpos;
	GetScrollRange(hwnd, SB_HORZ, &minpos, &maxpos);

	int tgt;
	switch (LOWORD(wparam))
	  {
	    case SB_LINELEFT:		tgt = scrollpos - 1;		break;
	    case SB_LINERIGHT:		tgt = scrollpos + 1;		break;
	    case SB_PAGELEFT:		tgt = scrollpos - pics_inline;	break;
	    case SB_PAGERIGHT:		tgt = scrollpos + pics_inline;	break;
	    case SB_LEFT:		tgt = minpos;			break;
	    case SB_RIGHT:		tgt = maxpos;			break;
	    case SB_THUMBPOSITION:	tgt = HIWORD(wparam);		break;
	    default:			tgt = scrollpos;		break;
	  }
	tgt = MAX(minpos, tgt);
	tgt = MIN(maxpos, tgt);
	if (tgt != scrollpos)
	  {
	    left_pic_index = tgt;
	    SetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX, tgt);
	    UpdateWindow(hwnd);
	    ScrollWindow(hwnd, GetWindowLong(hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX) * (scrollpos - left_pic_index), 0, NULL, NULL);
	    SetScrollPos(hwnd, SB_HORZ, left_pic_index, TRUE);
	    InvalidateRect(hwnd, NULL, FALSE);
	  }
	return 0;

      case WM_KEYDOWN:
	switch (wparam)
	  {
	    case VK_UP:		case VK_LEFT:	SendMessage(hwnd, WM_HSCROLL, SB_LINELEFT, 0);	break;
	    case VK_DOWN:	case VK_RIGHT:	SendMessage(hwnd, WM_HSCROLL, SB_LINERIGHT, 0);	break;
	    case VK_PRIOR:			SendMessage(hwnd, WM_HSCROLL, SB_PAGELEFT, 0);	break;
	    case VK_NEXT:			SendMessage(hwnd, WM_HSCROLL, SB_PAGERIGHT, 0);	break;
	    case VK_HOME:			SendMessage(hwnd, WM_HSCROLL, SB_LEFT, 0);	break;
	    case VK_END:			SendMessage(hwnd, WM_HSCROLL, SB_RIGHT, 0);	break;
	  }
	return 0;

      case WM_CHAR:
	SendMessage(global_listbox_hwnd, WM_CHAR, wparam, lparam);
	currsel = SendMessage(global_listbox_hwnd, LB_GETCURSEL, 0, 0);
	SendMessage(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, currsel), 0);
	SendMessage(hwnd, 0x8007, currsel, 0);
	SendMessage(GetParent(hwnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hwnd), 1), reinterpret_cast<LPARAM>(hwnd));
	return 0;

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case 1:
	      int amt;
	      amt = count_card_amount_outside_edited_deck(curr_csvid);

	      global_dlg_parameter = 1;
	      global_dlg_result = 1;
	      if ((amt > 1 && show_dialog_sellxcards() == 0)
		  || !global_dlg_result)
		return 0;

	      global_dlg_result = MIN(global_dlg_result, amt);

	      for (int i = 0; i < global_dlg_result; ++i)
		if (!delete_card_from_global_deck(curr_csvid, 1))
		  break;

	      *Gold += global_dlg_result * sellprice;

	      load_text("Menus", "PRICE");
	      sprintf(buf, text_lines[0], *Gold);
	      strncpy(global_deckname, buf, 12);

	      InvalidateRect(global_title_hwnd, NULL, TRUE);
	      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
	      InvalidateRect(global_horzlist_hwnd, NULL, FALSE);
	      return 0;

	    case 2:
	    case 3:
	      bool move_into;
	      move_into = LOWORD(wparam) == 2;

	      global_dlg_parameter = check_colors_inout_edited_deck(move_into);
	      show_dialog_groupmove();
	      move_colors_fromto_global_deck(move_into, global_dlg_result);

	      TENTATIVE_remove_selected_from_horzlist(global_listbox_hwnd, global_horzlist_hwnd);
	      InvalidateRect(global_horzlist_hwnd, 0, TRUE);

	      SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
	      InvalidateRect(global_decksurface_hwnd, NULL, TRUE);
	      return 0;

	    default:
	      return 0;
	  }

      case 0x8001:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	horz_list_addr = reinterpret_cast<int*>(realloc(horz_list_addr, 4 * last_pic_index + 4));
	if (!horz_list_addr)
	  return 0;

	horz_list_addr[last_pic_index++] = lparam;
	SetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX, last_pic_index);
	SetWindowLong(hwnd, HORZLIST_ADDR_INDEX, reinterpret_cast<LONG>(horz_list_addr));
	update_scroll_range();
	InvalidateRect(hwnd, NULL, TRUE);
	return last_pic_index;

      case 0x8002:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	if (static_cast<int>(wparam) >= last_pic_index)
	  return -1;

	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	currsel = GetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX);
	left_pic_index = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);

	{
	  DELETEITEMSTRUCT del;
	  del.CtlType = ODT_LISTBOX;
	  del.itemID = wparam;
	  del.hwndItem = hwnd;
	  del.itemData = horz_list_addr[wparam];
	  del.CtlID = GetDlgCtrlID(hwnd);

	  SendMessage(GetParent(hwnd), WM_DELETEITEM, del.CtlID, reinterpret_cast<LPARAM>(&del));
	}

	for (int i = wparam; i < last_pic_index - 1; ++i)
	  horz_list_addr[i] = horz_list_addr[i + 1];

	SetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX, --last_pic_index);

	if (last_pic_index - 1 < currsel)
	  SetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX, --currsel);

	if (last_pic_index - 1 < left_pic_index)
	  SetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX, --left_pic_index);

	update_scroll_range();
	InvalidateRect(hwnd, NULL, TRUE);

	if (!last_pic_index)
	  {
	    SetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX, -1);
	    SetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX, 0);
	  }

	return last_pic_index;

      case 0x8003:
	return GetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX);

      case 0x8004:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	if (static_cast<int>(wparam) < 0 || static_cast<int>(wparam) >= last_pic_index)
	  return -1;

	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	return horz_list_addr[wparam];

      case 0x8005:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	int insert_index;
	insert_index = wparam;
	if (insert_index >= last_pic_index)
	  return SendMessage(hwnd, 0x8001, 0, lparam);

	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	horz_list_addr = reinterpret_cast<int*>(realloc(horz_list_addr, 4 * last_pic_index + 4));

	if (!horz_list_addr)
	  return -2;

	int idx;
	for (idx = last_pic_index; idx > 0 && idx != insert_index; --idx)
	  horz_list_addr[idx] = horz_list_addr[idx - 1];

	horz_list_addr[idx] = lparam;
	SetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX, last_pic_index + 1);
	SetWindowLong(hwnd, HORZLIST_ADDR_INDEX, reinterpret_cast<LONG>(horz_list_addr));
	update_scroll_range();
	InvalidateRect(hwnd, NULL, TRUE);
	return insert_index;

      case 0x8006:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));

	for (int i = 0; i < last_pic_index; ++i)
	  {
	    DELETEITEMSTRUCT del;
	    del.CtlType = ODT_LISTBOX;
	    del.itemID = i;
	    del.hwndItem = hwnd;
	    del.itemData = horz_list_addr[i];
	    del.CtlID = GetDlgCtrlID(hwnd);

	    SendMessage(GetParent(hwnd), WM_DELETEITEM, del.CtlID, reinterpret_cast<LPARAM>(&del));
	  }

	horz_list_addr = reinterpret_cast<int*>(realloc(horz_list_addr, 4));
	SetWindowLong(hwnd, HORZLIST_ADDR_INDEX, reinterpret_cast<LONG>(horz_list_addr));
	SetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX, 0);
	SetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX, -1);
	SetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX, 0);
	SetScrollRange(hwnd, SB_HORZ, 0, -1, FALSE);
	SetScrollPos(hwnd, SB_HORZ, 0, TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

      case 0x8007:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	currsel = wparam;
	if (currsel >= last_pic_index)
	  return -1;

	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);
	left_pic_index = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);
	SetWindowLong(hwnd, HORZLIST_CURRSEL_INDEX, currsel);

	if (currsel < left_pic_index)
	  SendMessage(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, currsel), 0);
	else if (currsel >= left_pic_index + pics_inline)
	  SendMessage(hwnd, WM_HSCROLL, MAKELONG(SB_THUMBPOSITION, currsel - pics_inline + 1), 0);

	return 0;

      case 0x8008:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);
	SetScrollRange(hwnd, SB_HORZ, 0, MAX(0, last_pic_index - pics_inline), TRUE);
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

      case 0x8464:
	SendMessage(global_main_hwnd, 0x8466, wparam, reinterpret_cast<LPARAM>(hwnd));
	return 0;

      case 0x8466:
	last_pic_index = GetWindowLong(hwnd, HORZLIST_LASTPIC_INDEX);
	horz_list_addr = reinterpret_cast<int*>(GetWindowLong(hwnd, HORZLIST_ADDR_INDEX));
	left_pic_index = GetWindowLong(hwnd, HORZLIST_LEFTPIC_INDEX);
	pics_inline = GetWindowLong(hwnd, HORZLIST_PICSINLINE_INDEX);

	int max_idx;
	max_idx = MIN(last_pic_index, left_pic_index + pics_inline);

	int wanted_card;
	wanted_card = wparam;
	for (int i = left_pic_index; i < max_idx; ++i)
	  if (horz_list_addr[i] == wanted_card)
	    {
	      horzlist_prep_rectangle(hwnd, i, &r);
	      InvalidateRect(hwnd, &r, FALSE);
	    }

	return 0;

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ DeckSurfaceClass and dependencies
static void
reset_dim_and_pos_all_cards(HWND hwnd, RECT* rect)
{
  HWND prev_hwnd = NULL;
  for (HWND next_hwnd = GetTopWindow(hwnd); next_hwnd; next_hwnd = GetWindow(next_hwnd, GW_HWNDNEXT))
    prev_hwnd = next_hwnd;

  int offset_between_cards_x = global_smallcard_width + 8;
  int offset_between_cards_y = 1800 * global_smallcard_height / 10000;

  int offset_from_sides = global_smallcard_height / 10;

  int x = rect->left + offset_from_sides;
  int y = rect->top + offset_from_sides;

  for (hwnd = prev_hwnd; hwnd; hwnd = GetWindow(hwnd, GW_HWNDPREV))
    {
      if (global_smallcard_height + y > rect->bottom)
	{
	  x += offset_between_cards_x;
	  y = rect->top + offset_from_sides;
	}

      SetWindowPos(hwnd, NULL, x, y, global_smallcard_width, global_smallcard_height, SWP_NOZORDER);

      y += offset_between_cards_y;
    }
}

static bool
ask_about_saving_deck(void)
{
  int rval = popup_loaded(global_main_hwnd, "DECKBUILDER", "SAVE", global_deck_filename, MB_ICONQUESTION | MB_YESNOCANCEL);
  if (rval == IDYES)
    rval = SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_SAVEDECK, 0);
  return rval != IDCANCEL;
}

static HWND
find_wanted_window(HWND hwnd, csvid_t csvid)
{
  for (HWND w = GetTopWindow(hwnd); w; w = GetWindow(w, GW_HWNDNEXT))
    if (SendMessage(w, 0x8400, 0, 0) == csvid.raw)
      return w;
  return NULL;
}

static void
add_smallcard_window(HWND hwnd_parent, csvid_t csvid, int num)
{
  if (HWND hwnd = CreateWindowEx(0, "CardClass", "",
				 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
				 0, 0, global_smallcard_width, global_smallcard_height,
				 hwnd_parent,
				 reinterpret_cast<HMENU>(1),
				 global_hinstance, reinterpret_cast<void*>(csvid.raw)))
    {
      BringWindowToTop(hwnd);
      SendMessage(hwnd, 0x8401, num, 0);
    }
}

static void
add_smallcard_window(HWND hwnd_parent, Packs::Table tab)
{
  add_smallcard_window(hwnd_parent, tab.csvid, tab.amt);
}

LRESULT CALLBACK
wndproc_DeckSurfaceClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static int cleared_global_deck_num_entries = 0;
  static int cleared_global_deck_num_cards = 0;

  switch (msg)
    {
      case WM_CREATE:
	if (global_db_flags_1 & (DBFLAGS_GAUNTLET | DBFLAGS_NOCARDCOUNTCHECK))
	  {
	    clear_packs();
	    clear_packs_copy();
	  }

	global_decksurface_popup = CreatePopupMenu();

	if (global_db_flags_1 & DBFLAGS_STANDALONE)
	  {
	    load_text("Menus", "DECKSURFACE_STANDALONE");

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_NEWDECK, text_lines[0]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_LOADDECK, text_lines[1]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_SAVEDECK, text_lines[2]);

	    AppendMenu(global_decksurface_popup, MF_SEPARATOR, 0, NULL);

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[3]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_CLEARDECK, text_lines[4]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_SORTDECK, text_lines[5]);

	    AppendMenu(global_decksurface_popup, MF_SEPARATOR, 0, NULL);

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_MINIMIZE, text_lines[6]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_MUSIC, text_lines[7]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EFFECTS, text_lines[8]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EXIT, text_lines[9]);
	  }
	else if (global_db_flags_1 & DBFLAGS_GAUNTLET)
	  {
	    load_text("Menus", "DECKSURFACE_GAUNTLET");

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_NEWDECK, text_lines[0]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_LOADDECK, text_lines[1]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_SAVEDECK, text_lines[2]);

	    AppendMenu(global_decksurface_popup, MF_SEPARATOR, 0, NULL);

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[3]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_CLEARDECK, text_lines[4]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_SORTDECK, text_lines[5]);

	    AppendMenu(global_decksurface_popup, MF_SEPARATOR, 0, NULL);

	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_MUSIC, text_lines[6]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EFFECTS, text_lines[7]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EXIT, text_lines[8]);
	  }
	else
	  {
	    load_text("Menus", "DECKSURFACE_ADVENTURE");
	    if (global_db_flags_2 & DBFLAGS_SHANDALAR)
	      {
		AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_COLORINTODECK, text_lines[0]);
		AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_COLOROUTOFDECK, text_lines[1]);

		AppendMenu(global_decksurface_popup, MF_SEPARATOR, 0, NULL);
	      }
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[2]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_SORTDECK, text_lines[3]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_MUSIC, text_lines[4]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EFFECTS, text_lines[5]);
	    AppendMenu(global_decksurface_popup, MF_ENABLED, RES_MENU_EXIT, text_lines[6]);
	  }

	return 0;

      case WM_INITMENU:
	CHECKMENU_IF(global_decksurface_popup, RES_MENU_CONSOLIDATE, global_cfg_consolidate);
	CHECKMENU_IF(global_decksurface_popup, RES_MENU_EFFECTS, global_cfg_effects);
	CHECKMENU_IF(global_decksurface_popup, RES_MENU_MUSIC, global_cfg_music);
	return 0;

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case RES_MENU_CONSOLIDATE:
	      global_cfg_consolidate = !global_cfg_consolidate;
	      SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
	      return 0;

	    case RES_MENU_REFRESH:
	      RECT r;
	      GetClientRect(hwnd, &r);
	      InflateRect(&r, -5, -5);
	      LockWindowUpdate(global_main_hwnd);
	      reset_dim_and_pos_all_cards(hwnd, &r);
	      LockWindowUpdate(NULL);
	      return 0;

	    case RES_MENU_CLEARDECK:
	      if (global_edited_deck_num_entries)
		{
		  global_deck_was_edited = false;
		  memcpy(&global_packs_copy[0][0], &global_packs[0][0], sizeof global_packs);
		  clear_packs();

		  cleared_global_deck_num_entries = global_edited_deck_num_entries;
		  global_edited_deck_num_entries = 0;

		  cleared_global_deck_num_cards = global_deck_num_cards;
		  global_deck_num_cards = 0;

		  RemoveMenu(global_decksurface_popup, RES_MENU_CLEARDECK, MF_BYCOMMAND);
		  load_text("Menus", "DECKCLEAR_RESTORE");
		  InsertMenu(global_decksurface_popup, RES_MENU_SORTDECK, MF_BYCOMMAND|MF_ENABLED, RES_MENU_RESTOREDECK, text_lines[1]);
		  SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
		}
	      return 0;

	    case RES_MENU_RESTOREDECK:
	      memcpy(&global_packs[0][0], &global_packs_copy[0][0], sizeof global_packs);
	      clear_packs_copy();
	      global_deck_was_edited = true;

	      global_edited_deck_num_entries = cleared_global_deck_num_entries;
	      cleared_global_deck_num_entries = 0;

	      global_deck_num_cards = cleared_global_deck_num_cards;
	      cleared_global_deck_num_cards = 0;

	      load_text("Menus", "DECKCLEAR_RESTORE");
	      InsertMenu(global_decksurface_popup, RES_MENU_SORTDECK, MF_BYCOMMAND|MF_ENABLED, RES_MENU_CLEARDECK, text_lines[0]);
	      DeleteMenu(global_decksurface_popup, RES_MENU_RESTOREDECK, MF_BYCOMMAND);
	      SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
	      return 0;

	    case RES_MENU_NEWDECK:
	      if (!global_deck_was_edited || ask_about_saving_deck())
		{
		  global_deck_revision = 1;
		  global_deck_num_cards = 0;
		  global_edited_deck_num_entries = 0;

		  InvalidateRect(global_title_hwnd, NULL, TRUE);

		  clear_packs();
		  clear_packs_copy();

		  if (DeleteMenu(global_decksurface_popup, RES_MENU_RESTOREDECK, MF_BYCOMMAND))
		    {
		      load_text("Menus", "DECKCLEAR_RESTORE");
		      InsertMenu(global_decksurface_popup, RES_MENU_SORTDECK, MF_BYCOMMAND|MF_ENABLED, RES_MENU_CLEARDECK, text_lines[0]);
		    }
		  SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);

		  load_text("Menus", "NEWDECK");
		  sprintf(global_deckname, text_lines[0]);

		  global_deck_comments[0] = 0;
		  global_deck_description[0] = 0;

		  strcpy(global_deck_author, global_cfg_player_name);
		  strcpy(global_deck_email, global_cfg_email);

		  GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, "dd/MM/yyyy", global_deck_creation_date, 22);

		  show_dialog_deckinfo();
		}
	      return 0;

	    case RES_MENU_LOADDECK:
	      if (DeleteMenu(global_decksurface_popup, RES_MENU_RESTOREDECK, MF_BYCOMMAND))
		{
		  load_text("Menus", "DECKCLEAR_RESTORE");
		  InsertMenu(global_decksurface_popup, RES_MENU_SORTDECK, MF_BYCOMMAND|MF_ENABLED, RES_MENU_CLEARDECK, text_lines[0]);
		}
	      SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_LOADDECK, 0);
	      return 0;

	    case RES_MENU_SAVEDECK:
	      SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_SAVEDECK, 0);
	      return 0;

	    case RES_MENU_EXIT:
	      SendMessage(global_main_hwnd, WM_CLOSE, 0, 0);
	      return 0;

	    case RES_MENU_SORTDECK:
	      SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
	      return 0;

	    case RES_MENU_MUSIC:
	      SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_MUSIC, 0);
	      return 0;

	    case RES_MENU_EFFECTS:
	      SendMessage(global_main_hwnd, WM_COMMAND, RES_MAINMENU_EFFECTS, 0);
	      return 0;

	    case RES_MENU_MINIMIZE:
	      SendMessage(global_main_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	      SendMessage(GetParent(global_main_hwnd), WM_SYSCOMMAND, SC_MINIMIZE, 0);
	      return 0;

	    case RES_MENU_COLORINTODECK:
	    case RES_MENU_COLOROUTOFDECK:
	      global_dlg_parameter = check_colors_inout_edited_deck(LOWORD(wparam) == RES_MENU_COLOROUTOFDECK);
	      if (show_dialog_groupmove())
		{
		  move_colors_fromto_global_deck(LOWORD(wparam) == RES_MENU_COLORINTODECK, global_dlg_result);
		  count_packs();
		  SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
		  filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);
		}
	      return 0;

	    default:
	      return 0;
	  }

      case WM_DESTROY:
	DESTROY_MENU(global_decksurface_popup);
	return 0;

      case WM_SIZE:
	for (HWND w = GetTopWindow(hwnd); w; w = GetWindow(w, GW_HWNDNEXT))
	  SetWindowPos(w, NULL, 0, 0, global_smallcard_width, global_smallcard_height, SWP_NOZORDER | SWP_NOMOVE);

	if (global_decksurface_hwnd)
	  SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);

	return 0;

      case WM_ERASEBKGND:
	HDC hdc;
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	HDC mosaics[5];
	for (int i = 0; i < 5; ++i)
	  gdi_flush((mosaics[i] = CreateCompatibleDC(hdc)));

	for (int i = 0; i < 5; ++i)
	  SelectObject(mosaics[i], global_pics[i + 6]);

	RECT r;
	GetClientRect(hwnd, &r);

	BITMAP bmp;
	GetObject(global_pics[6], sizeof(BITMAP), &bmp);

	int dx, dy, tgt_dx;
	dy = r.bottom / 5;
	tgt_dx = bmp.bmWidth * r.bottom / (bmp.bmHeight * 5);	// = (width/height) * dy, after reordering for rounding
	for (int i = 1; i < 100; ++i)
	  {
	    dx = r.right / i;
	    if (dx < tgt_dx)
	      {
		if (i != 1)	// already as wide as it can go
		  {
		    int old_dx = r.right / (i - 1);
		    if (abs(tgt_dx - old_dx) < abs(tgt_dx - dx))	// overshot?
		      dx = old_dx;
		  }
		break;
	      }
	  }

	for (int mosx = 0, x = 0; x < r.right; x += dx, ++mosx)
	  for (int y = 0, mosy = mosx; y < r.bottom; y += dy, ++mosy)
	    StretchBlt(hdc,
		       x, y,
		       dx, dy,
		       mosaics[mosy % 5],
		       0, 0,
		       bmp.bmWidth, bmp.bmHeight,
		       SRCCOPY);

	ReleaseDC(hwnd, hdc);
	for (int i = 0; i < 5; ++i)
	  DELETE_DC(mosaics[i]);

	return 1;

      case WM_LBUTTONDOWN:
	BringWindowToTop(hwnd);
	SetFocus(global_main_hwnd);
	return 0;

      case WM_RBUTTONDOWN:
	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);
	ClientToScreen(hwnd, &p);
	TrackPopupMenu(global_decksurface_popup, TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, NULL);
	return 0;

      case 0x8401:
	// this inlines destroy_child_windows(), now unused
	for (HWND w = GetTopWindow(hwnd); w; w = GetTopWindow(hwnd))
	  DestroyWindow(w);

	set_smallcard_dimensions();

	LockWindowUpdate(global_main_hwnd);

	if (global_cfg_consolidate)
	  {
	    for (int i = 0; i <= PACK1_MAX; ++i)
	      for (int j = 0; j <= PACK2_MAX; ++j)
		for (int c = 0; c < global_packs[i][j].num; ++c)
		  add_smallcard_window(hwnd, global_packs[i][j].table[c]);
	  }
	else
	  {
	    for (int i = 0; i <= PACK1_MAX; ++i)
	      for (int j = 0; j <= PACK2_MAX; ++j)
		for (int c = 0; c < global_packs[i][j].num; ++c)
		  for (int d = 0; d < global_packs[i][j].table[c].amt; ++d)
		    add_smallcard_window(hwnd, global_packs[i][j].table[c].csvid, 1);
	  }

	LockWindowUpdate(0);

	SendMessage(hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
	refresh_numofcards_text();
	return 1;

      case 0x8466:
	// this inlines invalidate_wanted_window(), itself now unused
	for (HWND w = GetTopWindow(hwnd); w; w = GetWindow(w, GW_HWNDNEXT))
	  if (SendMessage(w, 0x8400, 0, 0) == static_cast<int>(wparam))
	    InvalidateRect(w, NULL, TRUE);
	return 0;

      case 0x84c8:
	int rval;
	rval = 1;

	{
	  csvid_t csvid{wparam};

	  HWND hwnd_card;

	  if (global_cfg_consolidate && (hwnd_card = find_wanted_window(hwnd, csvid)))
	    {
	      insert_cards_into_deck(csvid, 1, global_edited_deck);
	      SendMessage(hwnd_card, 0x8401, 1 + SendMessage(hwnd_card, 0x8402, 0, 0), 0);
	    }
	  else
	    {
	      hwnd_card = CreateWindowEx(0, "CardClass", "",
					 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
					 GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), global_smallcard_width, global_smallcard_height,
					 hwnd,
					 reinterpret_cast<HMENU>(1),
					 global_hinstance, reinterpret_cast<void*>(csvid.raw));
	      if (hwnd_card)
		{
		  BringWindowToTop(hwnd_card);
		  insert_cards_into_deck(csvid, 1, global_edited_deck);
		  SendMessage(hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
		}
	      else
		rval = 0;
	    }
	}

	if (cleared_global_deck_num_entries)
	  {
	    load_text("Menus", "DECKCLEAR_RESTORE");
	    InsertMenu(global_decksurface_popup, RES_MENU_SORTDECK, MF_BYCOMMAND|MF_ENABLED, RES_MENU_CLEARDECK, text_lines[0]);
	    RemoveMenu(global_decksurface_popup, RES_MENU_RESTOREDECK, MF_BYCOMMAND);
	    cleared_global_deck_num_cards = 0;
	    cleared_global_deck_num_entries = 0;
	  }
	return rval;

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ CardClass and dependencies
static void
TENTATIVE_move_from_owned_cards(HWND hwnd, int x, int y, int shift2_unshift1, int singleclick)
{
  csvid_t csvid{static_cast<int>(GetWindowLong(hwnd, CARD_CSVID_INDEX))};
  int amt = GetWindowLong(hwnd, CARD_AMOUNT_INDEX);

  HWND parent = GetParent(hwnd);

  RECT rect;
  GetWindowRect(hwnd, &rect);

  HWND new_hwnd = hwnd;

  if (amt >= 2)
    {
      POINT p;
      p.x = rect.left;
      p.y = rect.top;

      SendMessage(hwnd, 0x8401, 1, 0);

      MapWindowPoints(NULL, parent, &p, 1);

      new_hwnd = CreateWindowEx(0, "CardClass", "",
				WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS,
				p.x, p.y, global_smallcard_width, global_smallcard_height,
				parent,
				reinterpret_cast<HMENU>(1),
				global_hinstance, reinterpret_cast<void*>(csvid.raw));
      SendMessage(new_hwnd, 0x8401, amt - 1, 0);
      BringWindowToTop(new_hwnd);
    }

  HWND w;
  if (singleclick == 1)
    {
      LockWindowUpdate(global_main_hwnd);

      MapWindowPoints(NULL, global_main_hwnd, reinterpret_cast<POINT*>(&rect), 2);
      MoveWindow(hwnd, rect.left, rect.top, 0, 0, TRUE);

      SetParent(hwnd, global_main_hwnd);
      MoveWindow(hwnd, rect.left, rect.top, RECT_WIDTH(rect), RECT_HEIGHT(rect), TRUE);

      BringWindowToTop(hwnd);

      LockWindowUpdate(NULL);

      SendMessage(hwnd, WM_SYSCOMMAND, SC_MOVE | 2, 0);

      DestroyWindow(hwnd);

      POINT p;
      GetCursorPos(&p);
      HWND nextw = WindowFromPoint(p);
      w = nextw;
      while (nextw && nextw != global_main_hwnd && nextw != global_decksurface_hwnd && nextw != global_horzlist_hwnd)
	{
	  nextw = GetParent(nextw);
	  w = nextw;
	}
    }
  else
    {
      w = global_horzlist_hwnd;
      DestroyWindow(hwnd);
      SetFocus(global_horzlist_hwnd);
    }

  short num = 1;

  if (shift2_unshift1 == 1)
    SendMessage(new_hwnd, 0x8401, amt - 1, 0);
  else
    {
      global_dlg_parameter = amt;

      if (!show_dialog_movexcards())
	global_dlg_result = 0;

      if (global_dlg_result <= 0)
	{
	  SendMessage(new_hwnd, 0x8401, amt, 0);
	  num = 0;
	}
      else if (global_dlg_result < amt)
	{
	  SendMessage(new_hwnd, 0x8401, amt - global_dlg_result, 0);
	  num = global_dlg_result;
	}
      else
	{
	  DestroyWindow(new_hwnd);
	  num = amt;
	}
    }

  for (int i = 0; i < num; ++i)
    change_global_deck_card_availability(csvid, 0);

  remove_cards_from_deck(csvid, num, global_edited_deck);

  count_packs();
  refresh_numofcards_text();

  if (w == global_decksurface_hwnd)
    {
      if (w != parent)
	global_deck_was_edited = 1;

      POINT p;
      ScreenToClient(w, &p);
      p.x -= x;
      p.y -= y;

      for (int i = 0; i < num; ++i)
	{
	  if (!SendMessage(w, 0x84C8, csvid.raw, MAKELONG(p.x, p.y)))
	    MessageBeep(MB_OK);

	  change_global_deck_card_availability(csvid, 2);
	}

      count_packs();
    }
  else if (w == global_horzlist_hwnd)
    {
      set_smallcard_dimensions();

      if (w != parent)
	{
	  global_deck_was_edited = 1;
	  if (global_cfg_effects)
	    play_sound(3, 400, 0, 0);
	}

      for (int i = 0; i < num; ++i)
	if (change_global_deck_card_availability(csvid, 3))
	  TENTATIVE_scroll(global_listbox_hwnd, global_horzlist_hwnd);

      if (global_db_flags_1 & (DBFLAGS_EDITDECK|DBFLAGS_SHANDALAR))
	SendMessage(global_horzlist_hwnd, 0x8466, csvid.raw, 0);
    }
  else
    {
      POINT p;
      ScreenToClient(w, &p);
      p.x -= x;
      p.y -= y;

      change_global_deck_card_availability(csvid, 2);

      for (int i = 0; i < num; ++i)
	if (!SendMessage(global_decksurface_hwnd, 0x84C8, csvid.raw, MAKELONG(p.x, p.y)))
	  MessageBeep(0);

      count_packs();
    }

  if (!global_edited_deck_num_entries)
    global_deck_was_edited = 0;
}

LRESULT CALLBACK
wndproc_CardClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  RECT r;
  csvid_t csvid;
  int amt;
  static int sellprice = 0;

  switch (msg)
    {
      case WM_PAINT:
	amt = GetWindowLong(hwnd, CARD_AMOUNT_INDEX);
	csvid.raw = GetWindowLong(hwnd, CARD_CSVID_INDEX);
	PAINTSTRUCT paint;
	HDC paintdc;
	paintdc = BeginPaint(hwnd, &paint);
	GetClientRect(hwnd, &r);
	gdi_flush(paintdc);
	DrawSmallCard(global_hdc, &r, &cards_ptr[csvid.raw], 0, 1, -1, -1);
	if (amt > 1)
	  draw_small_amount(amt, &r);
	BitBlt(paintdc, 0, 0, r.right, r.bottom, global_hdc, 0, 0, SRCCOPY);
	EndPaint(hwnd, &paint);
	return 0;

      case WM_CREATE:
	csvid.raw = *(int*)(lparam);
	SetWindowLong(hwnd, CARD_CSVID_INDEX, csvid.raw);
	SetWindowLong(hwnd, CARD_AMOUNT_INDEX, 1);
	return (!csvid.ok() || csvid.raw > global_available_slots) ? -1 : 0;

      case WM_DESTROY:
	return 0;

      case WM_SIZE:
	GetWindowRect(hwnd, &r);
	global_smallcard_width = RECT_WIDTH(r);
	global_smallcard_height = RECT_HEIGHT(r);
	return 0;

      case WM_MOUSEMOVE:
	csvid.raw = GetWindowLong(hwnd, CARD_CSVID_INDEX);
	SendMessage(global_fullcard_hwnd, 0x8400, csvid.raw, 0);
	return 0;

      case WM_COMMAND:
	if (LOWORD(wparam) != RES_SELLMENU_SELLCARDFORX)
	  return 0;

	csvid.raw = GetWindowLong(hwnd, CARD_CSVID_INDEX);
	amt = GetWindowLong(hwnd, CARD_AMOUNT_INDEX);

	global_dlg_result = 1;
	global_dlg_parameter = 1;

	if ((amt > 1 && !show_dialog_sellxcards())
	    || global_dlg_result == 0)
	  return 0;

	if (global_dlg_result > amt)
	  global_dlg_result = amt;

	for (int i = 0; i < global_dlg_result; ++i)
	  if (!delete_card_from_global_deck(csvid, 0))
	    break;

	*Gold += global_dlg_result * sellprice;

	count_packs();

	load_text("Menus", "PRICE");
	char buf[500];
	sprintf(buf, text_lines[0], *Gold);
	strncpy(global_deckname, buf, 12);	// That's peculiar.
	InvalidateRect(global_title_hwnd, NULL, TRUE);

	if (global_dlg_result >= amt)
	  DestroyWindow(hwnd);
	else
	  {
	    SetWindowLong(hwnd, CARD_AMOUNT_INDEX, amt - global_dlg_result);
	    InvalidateRect(hwnd, NULL, FALSE);
	  }

	GetClientRect(global_decksurface_hwnd, &r);
	LockWindowUpdate(global_main_hwnd);
	reset_dim_and_pos_all_cards(global_decksurface_hwnd, &r);
	LockWindowUpdate(NULL);
	refresh_numofcards_text();
	return 0;

      case WM_LBUTTONDOWN:
	if (!(global_db_flags_2 & DBFLAGS_SHANDALAR))
	  return 0;

	Sleep(GetDoubleClickTime());

	MSG message;
	if (PeekMessage(&message, hwnd, WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK, PM_NOREMOVE))
	  return 0;

	TENTATIVE_move_from_owned_cards(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), (wparam & MK_SHIFT) ? 2 : 1, 1);

	SendMessage(global_decksurface_hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
	return 0;

      case WM_LBUTTONDBLCLK:
	if (!(global_db_flags_2 & DBFLAGS_SHANDALAR))
	  return 0;

	TENTATIVE_move_from_owned_cards(hwnd, GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam), (wparam & MK_SHIFT) ? 2 : 1, 0);

	SendMessage(global_decksurface_hwnd, WM_COMMAND, RES_MENU_REFRESH, 0);
	return 0;

      case WM_RBUTTONDOWN:
	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	if (global_db_flags_1 & DBFLAGS_STANDALONE)
	  {
	    ClientToScreen(hwnd, &p);
	    HWND parent = GetParent(hwnd);
	    ScreenToClient(parent, &p);
	    SendMessage(parent, WM_RBUTTONDOWN, wparam, MAKELONG(p.x, p.y));
	  }
	else if (global_db_flags_2 & DBFLAGS_STANDALONE)
	  {
	    csvid.raw = GetWindowLong(hwnd, CARD_CSVID_INDEX);
	    iid_t iid = get_iid_from_global_deck_card(csvid);
	    if (iid.ok())
	      {
		sellprice = SellPrice(iid.raw);
		load_text("Menus", "SELLCARD");
		char itemtxt[500];
		sprintf(itemtxt, text_lines[0], sellprice);
		AppendMenu(global_smallcard_popup, MF_ENABLED, RES_SELLMENU_SELLCARDFORX, itemtxt);
		ClientToScreen(hwnd, &p);
		TrackPopupMenu(global_smallcard_popup, TPM_RIGHTBUTTON, p.x + 10, p.y + 10, 0, hwnd, NULL);
		DeleteMenu(global_smallcard_popup, 0, MF_BYPOSITION);
	      }
	  }
	return 0;

      case 0x8400:
	return GetWindowLong(hwnd, CARD_CSVID_INDEX);

      case 0x8401:
	SetWindowLong(hwnd, CARD_AMOUNT_INDEX, LOWORD(wparam));
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

      case 0x8402:
	return GetWindowLong(hwnd, CARD_AMOUNT_INDEX);

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ ButtonClass and dependencies
static bool
is_buttonclass(HWND hwnd)
{
  if (!hwnd)
    return false;

  char classname[10];
  GetClassName(hwnd, classname, 10);
  return !strcasecmp(classname, "Button");
}

LRESULT CALLBACK
wndproc_ButtonClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg == WM_KILLFOCUS)
    lparam = reinterpret_cast<LPARAM>(hwnd);
  else if (msg == WM_SETFOCUS)
    {
      wparam = reinterpret_cast<WPARAM>(hwnd);
      lparam = wparam;
    }
  else
    return CallWindowProc(global_wndproc_std_ButtonClass, hwnd, msg, wparam, lparam);

  if (!is_buttonclass(reinterpret_cast<HWND>(wparam)))
    wparam = 0;
  if (!is_buttonclass(reinterpret_cast<HWND>(lparam)))
    lparam = 0;

  SendMessage(GetParent(hwnd), 0x84c8, wparam, lparam);

  return 0;
}
//]]]

//[[[ MainClass and dependencies
BOOL CALLBACK
enumfunc_change_buttonclass_wndproc(HWND hwnd, LPARAM lparam)
{
  if (is_buttonclass(hwnd))
    global_wndproc_std_ButtonClass = reinterpret_cast<WNDPROC>(SetWindowLong(hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(wndproc_ButtonClass)));
  return TRUE;
}

static void
copy_deck_to_edit(void)
{
  global_deck_num_entries = 0;
  global_deck_num_cards = 0;
  global_edited_deck_num_entries = 0;

  for (int i = 0; i < 500; ++i)
    {
      csvid_t csvid(CardIDFromType(global_external_deck[i]));
      if (!csvid.ok())
	{
	  count_packs();
	  return;
	}

      global_deck[i].GDE_csvid = csvid;
      global_deck[i].GDE_iid = iid_t(global_external_deck[global_deck_num_entries] & 0x7FFF);
      global_deck[i].GDE_Available = 1;
      global_deck[i].GDE_DecksBits = (global_external_deck[global_deck_num_entries] & 0x70000) >> 16;
      ++global_deck_num_entries;

      if (global_deck[i].GDE_DecksBits & (1 << global_current_deck))
	{
	  global_deck[i].GDE_Available = 0;
	  insert_cards_into_deck(global_deck[i].GDE_csvid, 1, global_edited_deck);
	}
    }
}

static bool
save_deck(const char* filename)
{
  File f(filename, "wt", true);
  if (!f.ok())
    return false;

  f.printf(";%s\n", global_deckname);
  f.printf(";%s\n", global_deck_description);
  f.printf(";%s\n", global_deck_author);
  f.printf(";%s\n", global_deck_email);
  f.printf(";%s\n", global_deck_creation_date);
  f.printf(";%d\n", global_deck_revision);
  f.printf(";%s\n", global_deck_edition);
  f.printf(";%s\n", global_deck_comments);
  f.printf("\n");

  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    f.printf(".%d\t%d\t%s\n",
	     global_edited_deck[i].DeckEntry_csvid.raw,
	     global_edited_deck[i].DeckEntry_Amount,
	     global_edited_deck[i].DeckEntry_FullName);

  return true;
}

#ifndef USE_STD_MAP_AND_STRING
static uint16_t
hashstr(const char* str)
{
  /* Exceedingly simple string hasher - takes the low-order 5 bits of each character in the string (which is case-insensitive for simple ascii) and
   * progressively xors it into the return value.  Produces relatively few collisions for our data set, roughly the same as MurmurHash. */
  uint32_t val = 0;
  int pos = 0;
  for (const char* p = str; *p; ++p)
    {
      unsigned int bits = *p & 0x1F;
      val ^= bits << pos;
      pos = (pos + 5) & 0xF;
    }
  return (val ^ (val >> 16)) & 0xFFFF;
}
#endif

static void
deck_parse_line(char* txt, csvid_t* csvid, int* num)
{
  char* p = txt;
  csvid->raw = atoi(p);
  while (*p && !isspace(*p))
    ++p;
  while (*p && isspace(*p))
    ++p;
  *num = atoi(p);

  if (*num <= 0)
    {
      csvid->raw = -1;
      *num = 0;
      return;
    }

  if (!global_cfg_read_by_name)
    {
      if (csvid->raw < 0 || csvid->raw >= global_available_slots)
	{
	  csvid->raw = -1;
	  *num = 0;
	}
      return;
    }

  while (*p && !isspace(*p))
    ++p;
  while (*p && isspace(*p))
    ++p;

  bool found;

#ifdef USE_STD_MAP_AND_STRING
  static std::map<std::string, int> name_to_csvid;
  if (name_to_csvid.empty())
    for (int i = 0; i < global_available_slots; ++i)
      {
	const card_ptr_t* cp = &cards_ptr[i];
	std::string s = cp->full_name;
	boost::to_lower(s);
	int& id = name_to_csvid[s];
	if (id == 0)
	  id = i;
      }

  std::string name = p;
  boost::trim(name);
  std::string lcname = boost::to_lower_copy(name);

  std::map<std::string, int>::const_iterator it = name_to_csvid.find(lcname);
  if (it != name_to_csvid.end())
    {
      csvid->raw = it->second;
      found = true;
    }
  else
    found = false;
#else
  struct PairIntPtr
  {
    int val;
    PairIntPtr* next;
  };
  static PairIntPtr** hashmap = NULL;
  if (!hashmap)
    {
      hashmap = static_cast<PairIntPtr**>(malloc(0x10000 * sizeof(PairIntPtr*)));
      memset(hashmap, 0, 0x10000 * sizeof(PairIntPtr*));

      static PairIntPtr* hashvals;
      hashvals = static_cast<PairIntPtr*>(malloc(global_available_slots * sizeof(PairIntPtr)));
      memset(hashvals, 0, global_available_slots * sizeof(PairIntPtr));

      for (int i = 0; i < global_available_slots; ++i)
	{
	  const card_ptr_t* cp = &cards_ptr[i];
	  uint16_t key = hashstr(cp->full_name);
	  hashvals[i].val = i;
	  hashvals[i].next = hashmap[key];
	  hashmap[key] = &hashvals[i];
	}
    }

  int l = strlen(p);
  while (l > 0 && isspace(p[--l]))
    p[l] = 0;

  uint16_t key = hashstr(p);

  found = false;
  for (PairIntPtr* q = hashmap[key]; q; q = q->next)
    if (!strcasecmp(p, cards_ptr[q->val].full_name))
      {
	found = true;
	break;
      }
#endif

  if (!found)
    {
      if (csvid->raw < 0 || csvid->raw >= global_available_slots)
	{
	  char buf[512];
	  snprintf(buf, 512, ("Name \"%s\" (%d) not found - %d card%s.\n"
			      "Skipped."),
		   p, csvid->raw, *num, *num > 0 ? "s" : "");
	  buf[511] = 0;
	  MessageBox(NULL, buf, "Warning", MB_OK);
	  csvid->raw = -1;
	  *num = 0;
	}
      else
	{
	  char buf[512];
	  snprintf(buf, 512, ("Name \"%s\" (%d) not found - %d card%s.\n"
			      "Current card with this id is \"%s\".\n"
			      "Use it?  (If you answer \"No\", the card will be skipped."),
		   p, csvid->raw, *num, *num > 0 ? "s" : "",
		   cards_ptr[csvid->raw].full_name);
	  buf[511] = 0;
	  if (MessageBox(NULL, buf, "Warning", MB_YESNO) != IDYES)
	    {
	      csvid->raw = -1;
	      *num = 0;
	    }
	}
    }
}

static bool
load_deck(char* filename)
{
  File f(filename, "rt", true);
  if (!f.ok())
    return false;

  memset(global_edited_deck, 0, sizeof global_edited_deck);

  global_deck_num_cards = global_edited_deck_num_entries = 0;
  if (global_db_flags_1 & DBFLAGS_GAUNTLET)
    global_deck_num_entries = 0;

#define STAGE(varname_stem, sz)		\
  char varname_stem[sz + 1];		\
  f.readline(varname_stem, sz + 1);	\
  if (varname_stem[0] != ';')		\
    return false

#define FINALIZE(varname_stem)		\
  strcpy(global_##varname_stem, varname_stem + 1)

  STAGE(deckname, 31);
  STAGE(deck_description, 21);
  STAGE(deck_author, 81);
  STAGE(deck_email, 81);
  STAGE(deck_creation_date, 22);
  STAGE(deck_revision, 16);
  STAGE(deck_edition, 16);
  STAGE(deck_comments, 401);

  FINALIZE(deckname);
  FINALIZE(deck_description);
  FINALIZE(deck_author);
  FINALIZE(deck_email);
  FINALIZE(deck_creation_date);
  global_deck_revision = atoi(&deck_revision[1]);
  FINALIZE(deck_edition);
  FINALIZE(deck_comments);

  char txt[512];
  while (f.readline(txt, 512) && txt[0] != 'v')
    if (txt[0] == '.')
      {
	if (txt[1] == 'v')
	  break;

	csvid_t csvid;
	int num;
	deck_parse_line(&txt[1], &csvid, &num);
	if (num > 0)
	  insert_cards_into_deck(csvid, num, global_edited_deck);
      }

  count_packs();
  return SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);
#undef STAGE
#undef FINALIZE
}

static int
check_card_count(int idx)
{
  if (global_check_card_count_fn)
    return global_check_card_count_fn(global_edited_deck, idx, global_deck_num_cards);

  csvid_t csvid = global_edited_deck[idx].DeckEntry_csvid;
  if (check_basic(csvid))
    return 0;

  for (int i = 0; i < global_edited_deck_num_entries; ++i)
    if (global_edited_deck[i].DeckEntry_csvid == csvid)
      {
	int lim = Scards[5].worldmagic_city ? 0 : 1;
	if (global_deck_num_cards >= 59)
	  return global_edited_deck[i].DeckEntry_Amount - (99 * lim + 4);
	else
	  return global_edited_deck[i].DeckEntry_Amount - (lim + 3);
      }

  return 0;
}

static void
draw_lines(HDC hdc, RECT* r, HGDIOBJ pen1, HPEN pen2, HPEN pen3)
{
  // top and left border
  SelectObject(hdc, pen1);
  MoveToEx(hdc, 0, 0, NULL);
  LineTo(hdc, r->right, 0);
  MoveToEx(hdc, 0, 0, NULL);
  LineTo(hdc, 0, r->bottom);

  // top and left border, moved inward by 1 px
  SelectObject(hdc, pen2);
  MoveToEx(hdc, 1, 1, NULL);
  LineTo(hdc, r->right - 1, 1);
  MoveToEx(hdc, 1, 1, NULL);
  LineTo(hdc, 1, r->bottom - 1);

  // bottom and right border
  SelectObject(hdc, pen3);
  MoveToEx(hdc, r->right - 1, 1, NULL);
  LineTo(hdc, r->right - 1, r->bottom);
  MoveToEx(hdc, 1, r->bottom - 1, NULL);
  LineTo(hdc, r->right, r->bottom - 1);
}

static void
draw_item(DRAWITEMSTRUCT* item, HBRUSH brush, HANDLE hbmp_bkgrd, HPEN pen1, HPEN pen2, COLORREF col, UINT format)
{
  HDC hdc = item->hDC;
  gdi_flush(hdc);

  RECT r;
  CopyRect(&r, &item->rcItem);
  OffsetRect(&r, -item->rcItem.left, -item->rcItem.top);

  if (hbmp_bkgrd)
    draw_background(hdc, &r, hbmp_bkgrd);
  else if (brush)
    FillRect(hdc, &r, brush);

  if (item->itemState & ODS_SELECTED)
    {
      draw_lines(hdc, &r, GetStockObject(BLACK_PEN), pen2, pen1);
      OffsetRect(&r, 2, 2);
    }
  else
    {
      draw_lines(hdc, &r, pen1, pen1, pen2);
      MoveToEx(hdc, r.right - 2, 2, NULL);
      LineTo(hdc, r.right - 2, r.bottom - 1);
      MoveToEx(hdc, 2, r.bottom - 2, NULL);
      LineTo(hdc, r.right - 1, r.bottom - 2);
    }

  SetBkMode(hdc, TRANSPARENT);
  SetTextColor(hdc, col);
  SelectObject(hdc, global_font_28percent);

  char txt[80];
  GetWindowText(item->hwndItem, txt, 50);
  DrawText(hdc, txt, -1, &r, format);
}

LRESULT CALLBACK
wndproc_MainClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static HPEN pen_ltgrey = NULL, pen_medgrey = NULL;
  static HWND button_deckinfo = NULL, button_exit = NULL, button_deck1 = NULL, button_deck2 = NULL, button_deck3 = NULL;
  static HWND cardlistfilter_hwnd = NULL, search_hwnd = NULL;
  static HMENU popup = NULL;

  HDC hdc;

  switch (msg)
    {
      case WM_CREATE:
	pen_ltgrey = CreatePen(PS_SOLID, 0, PALETTEINDEX(193));	// 203, 203, 203
	pen_medgrey = CreatePen(PS_SOLID, 0, PALETTEINDEX(22));	// 79, 78, 78
	//memcpy(&unk_4150C8, &global_logfont_template, 0x3Cu);	unused

	if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
	  init_sound_dll(hwnd, 0, 0);

	init_sounds_and_music();

	sprintf(global_deck_filename, "%sNew.dck", global_playdeck_path);

	if (global_db_flags_1 & DBFLAGS_SHANDALAR)
	  {
	    load_text("Menus", "GOLDTITLE");
	    char buf[160];
	    sprintf(buf, text_lines[0], *Gold);
	    strncpy(global_deckname, buf, 12);
	  }
	else if (global_db_flags_1 & DBFLAGS_EDITDECK)
	  strcpy(global_deckname, " ");
	else
	  {
	    load_text("Menus", "NEWDECK");
	    sprintf(global_deckname, text_lines[0]);
	  }

	global_deck_comments[0] = 0;
	global_deck_description[0] = 0;
	global_deck_revision = 1;
	strcpy(global_deck_author, global_cfg_player_name);
	strcpy(global_deck_email, global_cfg_email);

	GetDateFormat(LOCALE_SYSTEM_DEFAULT, 0, NULL, "dd/MM/yyyy", global_deck_creation_date, 22);

	if (global_db_flags_1 & (DBFLAGS_EDITDECK|DBFLAGS_GAUNTLET|DBFLAGS_SHANDALAR))
	  copy_deck_to_edit();

	global_title_hwnd = CreateWindowEx(0, "TitleClass", "",
					   WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
					   0, 0, 0, 0,
					   hwnd,
					   reinterpret_cast<HMENU>(1),
					   global_hinstance, 0);

	global_fullcard_hwnd = CreateWindowEx(0, "FullCardClass", "",
					      WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
					      0, 0, 0, 0,
					      hwnd,
					      reinterpret_cast<HMENU>(2),
					      global_hinstance, 0);

	global_decksurface_hwnd = CreateWindowEx(0, "DeckSurfaceClass", "",
						 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_BORDER,
						 0, 0, 0, 0,
						 hwnd,
						 reinterpret_cast<HMENU>(7),
						 global_hinstance, 0);

	global_button_stats_hwnd = CreateWindowEx(0, "BUTTON", "",
						  WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
						  0, 0, 0, 0,
						  hwnd,
						  reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_STATS),
						  global_hinstance, 0);

	button_deckinfo = CreateWindowEx(0, "BUTTON", "",
					 WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
					 0, 0, 0, 0,
					 hwnd,
					 reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_DECKINFO),
					 global_hinstance, 0);

	button_exit = CreateWindowEx(0, "BUTTON", "",
				     WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
				     0, 0, 0, 0,
				     hwnd,
				     reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_EXIT),
				     global_hinstance, 0);

	button_deck1 = CreateWindowEx(0, "BUTTON", "",
				      WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
				      0, 0, 0, 0,
				      hwnd,
				      reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_DECK_1),
				      global_hinstance, 0);

	button_deck2 = CreateWindowEx(0, "BUTTON", "",
				      WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
				      0, 0, 0, 0,
				      hwnd,
				      reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_DECK_2),
				      global_hinstance, 0);

	button_deck3 = CreateWindowEx(0, "BUTTON", "",
				      WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | BS_OWNERDRAW,
				      0, 0, 0, 0,
				      hwnd,
				      reinterpret_cast<HMENU>(RES_MAINMENU_BUTTON_DECK_3),
				      global_hinstance, 0);

	global_listbox_hwnd = CreateWindowEx(0, "LISTBOX", "",
					     (WS_CHILDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VSCROLL | WS_THICKFRAME
					      | LBS_NOINTEGRALHEIGHT | LBS_HASSTRINGS | LBS_OWNERDRAWFIXED | LBS_SORT),
					     0, 0, 0, 0,
					     hwnd,
					     reinterpret_cast<HMENU>(4),
					     global_hinstance, 0);

	global_horzlist_hwnd = CreateWindowEx(0, "HorzListClass", "",
					      WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL,
					      0, 0, 0, 0,
					      hwnd,
					      reinterpret_cast<HMENU>(5),
					      global_hinstance, 0);

	cardlistfilter_hwnd = CreateWindowEx(0, "CardListFilterClass", "",
					     WS_CHILDWINDOW | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
					     0, 0, 0, 0,
					     hwnd,
					     reinterpret_cast<HMENU>(3),
					     global_hinstance, 0);

	popup = CreatePopupMenu();
	global_smallcard_popup = CreatePopupMenu();

	search_hwnd = CreateWindowEx(0, "EDIT", "",
				     WS_CHILDWINDOW | WS_VISIBLE | ES_AUTOHSCROLL,
				     0, 0, 0, 0,
				     hwnd,
				     reinterpret_cast<HMENU>(8),
				     global_hinstance, 0);
	if (!search_hwnd
	    || !global_title_hwnd
	    || !global_fullcard_hwnd
	    || !global_listbox_hwnd
	    || !global_horzlist_hwnd
	    || !cardlistfilter_hwnd
	    || !global_decksurface_hwnd
	    || !popup)
	  {
	    DESTROY_MENU(popup);
	    DESTROY_MENU(global_smallcard_popup);
	    return -1;
	  }

	global_wndproc_std_SearchClass = reinterpret_cast<WNDPROC>(SetWindowLong(search_hwnd, GWL_WNDPROC, reinterpret_cast<LONG>(wndproc_SearchClass)));

	SendMessage(global_listbox_hwnd, LB_SETTOPINDEX, 0, 0);
	SendMessage(global_listbox_hwnd, LB_SETCURSEL, 0, 0);
	SendMessage(global_horzlist_hwnd, 0x8007, 0, 0);

	{
	  csvid_t csvid(static_cast<int>(SendMessage(global_horzlist_hwnd, 0x8004, 0, 0)));
	  if (!csvid.ok() && global_deck_num_entries)
	    csvid = global_deck[0].GDE_csvid;
	  SendMessage(global_fullcard_hwnd, 0x8400, csvid.raw, 0);
	}

	if (global_db_flags_1 & DBFLAGS_STANDALONE)
	  {
	    load_text("Menus", "MAINMENU_STANDALONE");
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_NEWDECK, text_lines[0]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_LOADDECK, text_lines[1]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_SAVEDECK, text_lines[2]);
	    AppendMenu(popup, MF_SEPARATOR, 0, 0);
	    AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[3]);
	    AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[4]);
	    AppendMenu(popup, MF_SEPARATOR, 0, 0);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MINIMIZE, text_lines[5]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MUSIC, text_lines[6]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EFFECTS, text_lines[7]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[8]);
	  }
	else if (global_db_flags_1 & DBFLAGS_GAUNTLET)
	  {
	    load_text("Menus", "MAINMENU_GAUNTLET");
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_NEWDECK, text_lines[0]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_LOADDECK, text_lines[1]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_SAVEDECK, text_lines[2]);
	    AppendMenu(popup, MF_SEPARATOR, 0, 0);
	    AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[3]);
	    AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[4]);
	    AppendMenu(popup, MF_SEPARATOR, 0, 0);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_MUSIC, text_lines[5]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EFFECTS, text_lines[6]);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[7]);
	  }
	else
	  {
	    load_text("Menus", "MAINMENU_ADVENTURE");
	    AppendMenu(popup, MF_ENABLED, RES_FULLCARDMENU_EXPAND, text_lines[0]);
	    AppendMenu(popup, MF_ENABLED, RES_MENU_CONSOLIDATE, text_lines[1]);
	    AppendMenu(popup, MF_SEPARATOR, 0, 0);
	    AppendMenu(popup, MF_ENABLED, RES_MAINMENU_EXIT, text_lines[2]);
	  }

	SetFocus(global_horzlist_hwnd);
	load_text("Menus", "DECKINFO");
	SetWindowText(button_deckinfo, text_lines[0]);
	EnumChildWindows(hwnd, enumfunc_change_buttonclass_wndproc, 0);
	refresh_numofcards_text();
	if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
	  {
	    load_text("Menus", "EXIT");
	    SetWindowText(button_exit, text_lines[1]);
	  }
	else
	  {
	    load_text("Menus", "EXIT");
	    SetWindowText(button_exit, text_lines[0]);

	    load_text("Menus", "DECKNUMBERS");
	    SetWindowText(button_deck1, text_lines[0]);
	    SetWindowText(button_deck2, text_lines[1]);
	    SetWindowText(button_deck3, text_lines[2]);

	    switch (global_current_deck)
	      {
		case 0:	SetWindowText(button_deck1, text_lines[3]);	break;
		case 1:	SetWindowText(button_deck2, text_lines[4]);	break;
		case 2:	SetWindowText(button_deck3, text_lines[5]);	break;
	      }
	  }
	return 0;

      case WM_INITMENU:
	CHECKMENU_IF(popup, RES_FULLCARDMENU_EXPAND, SendMessage(global_fullcard_hwnd, 0x8402, 0, 0));
	CHECKMENU_IF(popup, RES_MAINMENU_EFFECTS, global_cfg_effects);
	CHECKMENU_IF(popup, RES_MAINMENU_MUSIC, global_cfg_music);
	CHECKMENU_IF(popup, RES_MENU_CONSOLIDATE, global_cfg_consolidate);
	return 0;

      case WM_CLOSE:
	if (global_db_flags_1 & (DBFLAGS_GAUNTLET|DBFLAGS_STANDALONE|DBFLAGS_NOCARDCOUNTCHECK))
	  {
	    if (!global_deck_was_edited || ask_about_saving_deck() != 2)
	      DestroyWindow(global_main_hwnd);
	  }
	else
	  {
	    int num_failed = 0;
	    for (int i = 0; i < global_edited_deck_num_entries; ++i)
	      {
		int excess = check_card_count(i);
		if (excess > 0)
		  {
		    global_excessive_cards[num_failed].DeckEntry_csvid = global_edited_deck[i].DeckEntry_csvid;
		    global_excessive_cards[num_failed].DeckEntry_Amount = excess;
		    global_excessive_cards[num_failed].DeckEntry_FullName = global_edited_deck[i].DeckEntry_FullName;
		    ++num_failed;
		  }
	      }
	    global_excessive_cards[num_failed].DeckEntry_csvid.raw = -1;

	    if (!num_failed || show_dialog_infobox())
	      DestroyWindow(global_main_hwnd);
	  }
	return 0;

      case WM_DESTROY:
	DELETE_OBJ(pen_ltgrey);
	DELETE_OBJ(pen_medgrey);
	DESTROY_MENU(popup);
	DESTROY_MENU(global_smallcard_popup);
	if (!(global_db_flags_1 & DBFLAGS_SHANDALAR))
	  free_sounds_and_music();
	PostQuitMessage(0);
	return 0;

      case WM_QUERYENDSESSION:
	return (!global_deck_was_edited || ask_about_saving_deck() != 2) ? 1 : 0;

      case WM_SETFOCUS:
	SetFocus(global_horzlist_hwnd);
	return 0;

      case WM_SIZE:
	global_main_window_width = LOWORD(lparam);
	global_main_window_height = HIWORD(lparam);

	LockWindowUpdate(hwnd);

	global_smallcard_width =
	  global_smallcard_normal_size.x =
	  global_smallcard_piclist_width =
	  global_smallcard_height =
	  global_smallcard_normal_size.y =
	  global_smallcard_piclist_height = global_main_window_height * 0.18;

	int pics_inline;
	pics_inline = global_main_window_width / (global_smallcard_width * 1.1);
	if (pics_inline <= 0)
	  pics_inline = 1;
	SetWindowLong(global_horzlist_hwnd, HORZLIST_PICSINLINE_INDEX, pics_inline);
	SetWindowLong(global_horzlist_hwnd, HORZLIST_WIDTHPLUSSPACE_INDEX, global_main_window_width / pics_inline);

	global_smallcard_smaller_size.x = global_smallcard_smaller_size.y = global_smallcard_width * 0.8;
	global_smallcard_smallest_size.x = global_smallcard_smallest_size.y = global_smallcard_width * 0.7;

	LOGFONT font;
	memcpy(&font, &global_logfont_template, sizeof(LOGFONT));

	DELETE_OBJ(global_font_28percent);
	font.lfHeight = 280 * global_main_window_height / 10000;
	global_font_28percent = CreateFontIndirect(&font);

	DELETE_OBJ(global_font_40percent);
	font.lfHeight = 400 * global_main_window_height / 10000;
	global_font_40percent = CreateFontIndirect(&font);

	int hgt6, hgt33;
	hgt6 = 6 * global_main_window_height / 100;
	hgt33 = 33 * global_main_window_height / 100;

	hdc = GetDC(hwnd);
	gdi_flush(hdc);

	SIZE sz;
	GetTextExtentPoint32(hdc, "Load new deck", strlen("Load new deck"), &sz);
	sz.cx += 10;
	sz.cy += sz.cy / 2;

	ReleaseDC(hwnd, hdc);

	RECT rect_title, rect_fullcard, rect_horzlist, rect_cardlistfilter, rect_decksurface, rect_stats;

#define MOVEWINDOW_TL_BR(hwnd, rect)	MoveWindow(hwnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, FALSE)
#define MOVEWINDOW_TL_WH(hwnd, rect)	MoveWindow(hwnd, rect.left, rect.top, rect.right, rect.bottom, FALSE)

        SetRect(&rect_title,
		0, 0,
		hgt33, 10 * global_main_window_height / 100);

        SetRect(&rect_fullcard,
		0, rect_title.bottom,
		hgt33, rect_title.bottom + 5900 * global_main_window_height / 10000);

        SetRect(&rect_horzlist,
		0, global_main_window_height - 22 * global_main_window_height / 100,
		global_main_window_width, global_main_window_height);

        SetRect(&rect_cardlistfilter,
		rect_horzlist.left, rect_horzlist.top - hgt6,
		rect_horzlist.right, rect_horzlist.top);

        SetRect(&rect_decksurface,
		rect_fullcard.right, 0,
		global_main_window_width, rect_fullcard.bottom);

        MOVEWINDOW_TL_BR(global_title_hwnd, rect_title);
        MOVEWINDOW_TL_BR(global_fullcard_hwnd, rect_fullcard);
        MOVEWINDOW_TL_BR(global_decksurface_hwnd, rect_decksurface);

        rect_horzlist.top = rect_horzlist.bottom - global_smallcard_piclist_height - 8 - GetSystemMetrics(SM_CYHSCROLL);
        MOVEWINDOW_TL_BR(global_horzlist_hwnd, rect_horzlist);

        rect_cardlistfilter.top = rect_horzlist.top - hgt6;
	rect_cardlistfilter.bottom = rect_horzlist.top;
        MOVEWINDOW_TL_BR(cardlistfilter_hwnd, rect_cardlistfilter);

        SetRect(&rect_stats, rect_decksurface.left, rect_decksurface.bottom, (rect_decksurface.right - rect_decksurface.left) / 3, rect_cardlistfilter.top - rect_decksurface.bottom - 3);

	MOVEWINDOW_TL_WH(global_button_stats_hwnd, rect_stats);
        MoveWindow(button_exit, rect_stats.left + 2 * rect_stats.right, rect_stats.top, rect_stats.right, rect_stats.bottom, 0);

        MoveWindow(search_hwnd,
		   rect_title.left + ((rect_title.right - (600 * rect_title.right / 1000)) / 2),
		   rect_stats.top,
		   600 * rect_title.right / 1000,
		   700 * MAX(rect_stats.bottom, 0) / 1000,
		   FALSE);

        SendMessage(search_hwnd, WM_SETFONT, reinterpret_cast<WPARAM>(global_font_28percent), 0);

        if (global_db_flags_1 & DBFLAGS_SHANDALAR)
	  {
	    int x = rect_stats.left + rect_stats.right;
	    MoveWindow(button_deck1,
		       x, rect_stats.top,
		       rect_stats.right/3, rect_stats.bottom,
		       FALSE);
	    MoveWindow(button_deck2,
		       x + rect_stats.right/3, rect_stats.top,
		       rect_stats.right/3, rect_stats.bottom,
		       FALSE);
	    MoveWindow(button_deck3,
		       x + 2*rect_stats.right/3, rect_stats.top,
		       rect_stats.right/3, rect_stats.bottom,
		       FALSE);
	  }
        else
	  MoveWindow(button_deckinfo,
		     rect_stats.left + rect_stats.right, rect_stats.top,
		     rect_stats.right, rect_stats.bottom,
		     FALSE);

        SendMessage(global_horzlist_hwnd, 0x8008, 0, global_smallcard_piclist_width);
        SendMessage(cardlistfilter_hwnd, 0x8402,
		    MAKELONG(rect_cardlistfilter.left, rect_cardlistfilter.right),
		    MAKELONG(rect_cardlistfilter.top, rect_cardlistfilter.bottom));

#if 0	// ...doesn't actually do anything.
	int lastpic, leftpic, pics_inline;
        lastpic = GetWindowLong(global_horzlist_hwnd, HORZLIST_LASTPIC_INDEX);
        leftpic = GetWindowLong(global_horzlist_hwnd, HORZLIST_LEFTPIC_INDEX);
        pics_inline = GetWindowLong(global_horzlist_hwnd, HORZLIST_PICSINLINE_INDEX);

        for (int i = leftpic; ; ++i)
	  {
	    int rightpic = MIN(lastpic, leftpic + pics_inline);
	    if (i >= rightpic)
	      break;
	  }
#endif

        InvalidateRect(hwnd, NULL, TRUE);

        BringWindowToTop(global_title_hwnd);
        BringWindowToTop(cardlistfilter_hwnd);
        BringWindowToTop(global_horzlist_hwnd);
        BringWindowToTop(global_button_stats_hwnd);
        BringWindowToTop(button_deckinfo);
        BringWindowToTop(button_exit);

        if (!(global_db_flags_1 & DBFLAGS_SHANDALAR) && global_cfg_music)
          play_music(1, 400, 0);

        return 0;

      case WM_DISPLAYCHANGE:
	SelectObject(global_hdc, global_old_bmp_obj);
	DELETE_OBJ(global_hbmp);

	hdc = GetDC(hwnd);
	gdi_flush(hdc);

	global_hbmp = CreateCompatibleBitmap(hdc, LOWORD(lparam), HIWORD(lparam));

	ReleaseDC(hwnd, hdc);

	SelectObject(global_hdc, global_hbmp);

	if (global_hbmp)
	  ShowWindow(hwnd, SW_SHOW);
	else
	  {
	    MessageBox(hwnd,
		       ("Not enough system memory to run at this screen resolution.\n"
			"\n"
			"Change it back to a smaller size before continuing.\n"
			"(The game will hide itself until then)"),
		       "Deck Builder",
		       MB_ICONWARNING);
	    ShowWindow(hwnd, SW_HIDE);
	  }

	MoveWindow(hwnd, 0, 0, LOWORD(lparam), HIWORD(lparam), TRUE);
	return 0;

      case WM_ERASEBKGND:
	fill_back(hwnd, reinterpret_cast<HDC>(wparam));
	return 1;

      case WM_DRAWITEM:
	DRAWITEMSTRUCT* item;
	item = reinterpret_cast<DRAWITEMSTRUCT*>(lparam);

	draw_item(item, global_brush_mediumgrey, global_pics[36], pen_ltgrey, pen_medgrey,
		  GetFocus() == item->hwndItem && (item->itemState & ODS_SELECTED) ? global_colorref_white : RGB(0, 0, 0),
		  DT_SINGLELINE | DT_VCENTER | DT_CENTER);
	return 1;

      case WM_MEASUREITEM:
	MEASUREITEMSTRUCT* measure;
	measure = reinterpret_cast<MEASUREITEMSTRUCT*>(lparam);

	if (measure->CtlID == 4)
	  {
	    RECT r;
	    GetClientRect(GetDlgItem(hwnd, wparam), &r);

	    measure->itemWidth = r.right;
	    measure->itemHeight = r.bottom / 8;
	  }
	return 0;

      case WM_CTLCOLORBTN:
      case WM_CTLCOLORSTATIC:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	SetTextColor(hdc, GetFocus() == reinterpret_cast<HWND>(lparam) ? global_colorref_white : global_colorref_darkgrey);
	SetBkMode(hdc, TRANSPARENT);
	return reinterpret_cast<LRESULT>(GetStockObject(HOLLOW_BRUSH));

      case WM_CTLCOLORLISTBOX:
	hdc = reinterpret_cast<HDC>(wparam);
	gdi_flush(hdc);

	SetTextColor(hdc, RGB(0, 0, 0));
	return reinterpret_cast<LRESULT>(global_listbox_hwnd == reinterpret_cast<HWND>(lparam) ? global_brush_gold1 : global_brush_gold2);

      case WM_CTLCOLOREDIT:
	hdc = reinterpret_cast<HDC>(wparam);
	SetTextColor(hdc, RGB(0, 0, 0));
	SetBkColor(hdc, RGB(176, 176, 176));
	return reinterpret_cast<LRESULT>(global_brush_mediumgrey);

      case WM_RBUTTONDOWN:
	POINT p;
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);
	ClientToScreen(hwnd, &p);
	TrackPopupMenu(popup, TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, NULL);
	return 0;

      case WM_COMMAND:
	switch (LOWORD(wparam))
	  {
	    case RES_FULLCARDMENU_EXPAND:
	      SendMessage(global_fullcard_hwnd, WM_COMMAND, wparam, lparam);
	      break;

	    case 4:
	    case 5:
	      if (HIWORD(wparam) != 1)
		break;

	      HWND w, tgt;
	      w = reinterpret_cast<HWND>(lparam);
	      tgt = LOWORD(wparam) == 4 ? global_horzlist_hwnd : global_listbox_hwnd;

	      int cursel, csvidraw;
	      cursel = SendMessage(w, w == global_horzlist_hwnd ? 0x8003 : LB_GETCURSEL, 0, 0);
	      csvidraw = SendMessage(w, w == global_horzlist_hwnd ? 0x8004 : LB_GETITEMDATA, cursel, 0);
	      SendMessage(global_fullcard_hwnd, 0x8400, csvidraw, 0);

	      cursel = SendMessage(w, w == global_horzlist_hwnd ? 0x8003 : LB_GETCURSEL, 0, 0);
	      SendMessage(tgt, w == global_horzlist_hwnd ? 0x8007 : LB_SETCURSEL, cursel, 0);
	      break;

	    case RES_MAINMENU_BUTTON_STATS:
	      DialogBoxParam(global_hinstance, MAKEINTRESOURCE(RES_DIALOG_DECKSTATS), global_main_hwnd, dlgproc_DeckStats, 0);
	      break;

	    case RES_MENU_CONSOLIDATE:
	      SendMessage(global_decksurface_hwnd, WM_COMMAND, wparam, lparam);
	      break;

	    case RES_MAINMENU_NEWDECK:
	      SendMessage(global_decksurface_hwnd, WM_COMMAND, RES_MENU_NEWDECK, 0);
	      break;

	    case RES_MAINMENU_LOADDECK:
	      if ((!global_deck_was_edited || ask_about_saving_deck() != 2)
		  && show_dialog_loaddeck(global_deck_filename))
		{
		  if (load_deck(global_deck_filename))
		    {
		      InvalidateRect(global_title_hwnd, NULL, TRUE);
		      global_deck_was_edited = false;
		    }
		  else
		    {
		      popup_loaded(hwnd, "DECKBUILDER", "DECKLOADERROR", global_deck_filename);

		      load_text("Menus", "NEWDECK");
		      sprintf(global_deckname, text_lines[0]);

		      sprintf(global_deck_filename, "%sNew.dck", global_playdeck_path);
		    }
		}
	      break;

	    case RES_MAINMENU_SAVEDECK:
	      load_text("Menus", "NEWDECK");
	      if (!strcmp(global_deckname, text_lines[0]) || !global_deckname[0])
		{
		  popup_loaded(hwnd, "DECKBUILDER", "NAMEYOURDECK");
		  show_dialog_deckinfo();
		  return 2;
		}

	      if (global_deck_num_cards < 40)
		{
		  popup_loaded(hwnd, "DECKBUILDER", "TOOFEWCARDS");
		  return 2;
		}

	      if (global_deck_num_cards > 500 || global_edited_deck_num_entries > 200)
		popup_loaded(hwnd, "DECKBUILDER", "TOOMANYCARDS");

	      sprintf(global_deck_filename, "%s%s.dck", global_playdeck_path, global_deckname);

	      if (global_deckname_set)
		{
		  File f(global_deck_filename, "rt", true);
		  if (f.ok())
		    {
		      f.close();

		      int rval = popup_loaded(hwnd, "DECKBUILDER", "DECKEXISTS", global_deckname, MB_ICONQUESTION | MB_YESNOCANCEL);
		      if (rval == IDNO || rval == IDCANCEL)
			return 2;
		    }
		}

	      global_deckname_set = false;
	      global_deck_was_edited = false;

	      if (save_deck(global_deck_filename))
		popup_loaded(hwnd, "DECKBUILDER", "SAVED", global_deckname);
	      else
		popup_loaded(hwnd, "DECKBUILDER", "DECKSAVEERROR", global_deckname);

	      if (global_db_flags_1 & (DBFLAGS_EDITDECK | DBFLAGS_GAUNTLET | DBFLAGS_NOCARDCOUNTCHECK))
		strcpy(global_external_deckname, global_deckname);

	      return 6;

	    case RES_MAINMENU_EFFECTS:
	      global_cfg_effects = !global_cfg_effects;
	      break;

	    case RES_MAINMENU_MUSIC:
	      if ((global_cfg_music = !global_cfg_music))
		play_music(1, 400, 0);
	      else
		sound_stop(1);
	      break;

	    case RES_MAINMENU_MINIMIZE:
	      SendMessage(global_main_hwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0);
	      break;

	    case RES_MAINMENU_EXIT:
	    case RES_MAINMENU_BUTTON_EXIT:
	      SendMessage(global_main_hwnd, WM_CLOSE, 0, 0);
	      break;

	    case RES_MAINMENU_BUTTON_DECKINFO:
	      show_dialog_deckinfo();
	      InvalidateRect(global_title_hwnd, 0, TRUE);
	      break;

	    case RES_MAINMENU_BUTTON_DECK_1:
	    case RES_MAINMENU_BUTTON_DECK_2:
	    case RES_MAINMENU_BUTTON_DECK_3:
	      if (!(global_db_flags_2 & DBFLAGS_SHANDALAR))
		break;

	      load_text("Menus", "DECKNUMBERS");
	      SetWindowText(button_deck1, text_lines[0]);
	      SetWindowText(button_deck2, text_lines[1]);
	      SetWindowText(button_deck3, text_lines[2]);

	      global_current_deck = LOWORD(wparam) - RES_MAINMENU_BUTTON_DECK_1;
	      global_deck_num_cards = 0;
	      global_edited_deck_num_entries = 0;

	      for (int i = 0; i < global_deck_num_entries; ++i)
		if (global_deck[i].GDE_DecksBits & (1 << global_current_deck))
		  {
		    global_deck[i].GDE_Available = 0;
		    insert_cards_into_deck(global_deck[i].GDE_csvid, 1, global_edited_deck);
		  }
		else
		  global_deck[i].GDE_Available = 1;

	      count_packs();

	      SendMessage(global_decksurface_hwnd, 0x8401, 0, 0);

	      filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);

	      switch (global_current_deck)
		{
		  case 0:	SetWindowText(button_deck1, text_lines[3]);	break;
		  case 1:	SetWindowText(button_deck2, text_lines[4]);	break;
		  case 2:	SetWindowText(button_deck3, text_lines[5]);	break;
		}

	      SetFocus(global_horzlist_hwnd);

	      break;
	  }
	return 0;

      case 0x8466:
	if (reinterpret_cast<HWND>(lparam) != global_fullcard_hwnd)
	  SendMessage(global_fullcard_hwnd, 0x8466, wparam, 0);
	return 0;

      case 0x84c8:
	HWND hwnd1, hwnd2;
	hwnd1 = reinterpret_cast<HWND>(wparam);
	hwnd2 = reinterpret_cast<HWND>(lparam);
	if (GetDlgItem(hwnd, RES_MAINMENU_BUTTON_STATS) == hwnd1
	    || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECKINFO) == hwnd1
	    || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_EXIT) == hwnd1
	    || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_1) == hwnd1
	    || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_2) == hwnd1
	    || GetDlgItem(hwnd, RES_MAINMENU_BUTTON_DECK_3) == hwnd1)
	  SendMessage(hwnd, 0x8401, GetDlgCtrlID(hwnd1), 0);
	else
	  SendMessage(hwnd, 0x8401, 1, 0);

	if (hwnd1)
	  InvalidateRect(hwnd1, NULL, TRUE);

	if (hwnd2)
	  InvalidateRect(hwnd2, NULL, TRUE);

	return 0;

      default:
	return DefWindowProcA(hwnd, msg, wparam, lparam);
    }
}
//]]]

//[[[ CardListFilterClass, dependencies, and related
static void
filterbuttons_setcoords(const RECT* r, int button_number, RECT* rval)
{
  if ((global_db_flags_1 & DBFLAGS_SHANDALAR) && button_number < 10)
    {
      SetRect(rval, -100, -100, -100, -100);
      return;
    }

  int w = r->right - r->left - 12;
  if (w < 0)
    w = 1;

  int dx = w / 25;

  int extra_offset;
  if (button_number >= 24)
    extra_offset = 12;
  else if (button_number >= 19)
    extra_offset = 8;
  else if (button_number >= 11)
    extra_offset = 4;
  else
    extra_offset = 0;

  SetRect(rval, r->left + 1, r->top + 1, r->left + 1 + dx, r->bottom - 2);
  OffsetRect(rval, (button_number - 4) * dx + extra_offset, 0);
}

static bool
point_in_filterbutton(int button_number, RECT* r, POINT p)
{
  RECT r2;
  filterbuttons_setcoords(r, button_number, &r2);
  return PtInRect(&r2, p);
}

static int
get_filter_button_state(HWND hwnd, POINT p)
{
#define CHECK_BUTTON(button_number, txthdr, var, bit, additional_condition...)	\
  if (additional_condition point_in_filterbutton(button_number, &r, p))		\
    {						\
      if (load_text("CueCards", txthdr) == -1)	\
	return 0;				\
      return (var & bit) ? 1 : 2;		\
    }

  RECT r;
  GetClientRect(hwnd, &r);

       CHECK_BUTTON(11,	"WHITE",	global_filter_colors,		FC_WHITE)
  else CHECK_BUTTON(15,	"GREEN",	global_filter_colors,		FC_GREEN)
  else CHECK_BUTTON(14,	"RED",		global_filter_colors,		FC_RED)
  else CHECK_BUTTON(13,	"BLACK",	global_filter_colors,		FC_BLACK)
  else CHECK_BUTTON(16,	"GOLD",		global_filter_colors,		FC_GOLD)
  else CHECK_BUTTON(12,	"BLUE",		global_filter_colors,		FC_BLUE)
  else CHECK_BUTTON(10,	"DARK",		global_filter_cardsets,		FS_THE_DARK,		(global_filter_cardsets_flags & FCSF_THE_DARK) && )
  else CHECK_BUTTON(7,	"ARABIAN",	global_filter_cardsets,		FS_ARABIAN_NIGHTS,	(global_filter_cardsets_flags & FCSF_ARABIAN_NIGHTS) && )
  else CHECK_BUTTON(8,	"ANTIQUITIES",	global_filter_cardsets,		FS_ANTIQUITIES,		(global_filter_cardsets_flags & FCSF_ANTIQUITIES) && )
  else CHECK_BUTTON(6,	"ASTRAL",	global_filter_cardsets,		FS_ASTRAL)
  else CHECK_BUTTON(9,	"LEGENDS",	global_filter_cardsets,		FS_LEGENDS,		(global_filter_cardsets_flags & FCSF_LEGENDS) && )
  else CHECK_BUTTON(5,	"FOURTH",	global_filter_cardsets,		FS_4TH_EDITION)
  else CHECK_BUTTON(4,	"EIGHT",	global_filter_cardsets,		FS_OTHER)
  else CHECK_BUTTON(17,	"LAND",		global_filter_cardtypes,	FT_LAND)
  else CHECK_BUTTON(18,	"ARTIFACT",	global_filter_cardtypes,	FT_ARTIFACT)
  else CHECK_BUTTON(19,	"CREATURE",	global_filter_cardtypes,	FT_CREATURE)
  else CHECK_BUTTON(20,	"ENCHANTMENT",	global_filter_cardtypes,	FT_ENCHANTMENT)
  else CHECK_BUTTON(21,	"INSTANT",	global_filter_cardtypes,	FT_INSTANT)
  else CHECK_BUTTON(22,	"INTERRUPT",	global_filter_cardtypes,	FT_INTERRUPT)
  else CHECK_BUTTON(23,	"SORCERY",	global_filter_cardtypes,	FT_SORCERY)
  else CHECK_BUTTON(24,	"CASTCOST",	global_filter_casting_cost,	FN_ENABLE)
  else CHECK_BUTTON(25,	"POWER",	global_filter_power,		FN_ENABLE)
  else CHECK_BUTTON(26,	"TOUGHNESS",	global_filter_toughness,	FN_ENABLE)
  else CHECK_BUTTON(27,	"ABILITY",	global_filter_abilities,	FA_ENABLE)
  else CHECK_BUTTON(28,	"RARITY",	global_filter_rarity,		FR_ENABLE)
  else
    return 0;
#undef CHECK_BUTTON
}

static HMENU
select_filter_menu(HWND hwnd, POINT p)
{
#define CHECK_BUTTON(button_number, menu)		\
  if (point_in_filterbutton(button_number, &r, p))	\
    return menu;

  RECT r;
  GetClientRect(hwnd, &r);

       CHECK_BUTTON(4,	global_filtermenu_newexp)
  else CHECK_BUTTON(16,	global_filtermenu_gold)
  else CHECK_BUTTON(5,	global_filtermenu_fourth)
  else CHECK_BUTTON(17,	global_filtermenu_land)
  else CHECK_BUTTON(18,	global_filtermenu_artifact)
  else CHECK_BUTTON(19,	global_filtermenu_creature)
  else CHECK_BUTTON(20,	global_filtermenu_enchantment)
  else CHECK_BUTTON(24,	global_filtermenu_castcost)
  else CHECK_BUTTON(25,	global_filtermenu_power)
  else CHECK_BUTTON(26,	global_filtermenu_toughness)
  else CHECK_BUTTON(27,	global_filtermenu_ability)
  else CHECK_BUTTON(28,	global_filtermenu_rarity)
  else
    return global_filtermenu_default;
#undef CHECK_BUTTON
}

static void
draw_filter_button_3d(HDC hdc, RECT* r2, RECT* r3, bool pushed)
{
  RECT r4;
  if (pushed)
    {
      SetRect(&r4, r2->left + 1, r2->top + 1, r2->right, r2->top + 3);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(&r4, r2->left + 1, r2->top + 1, r2->left + 2, r2->bottom);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(r3, r2->left + 3, r2->top + 3, r2->right, r2->bottom);
    }
  else
    {
      SetRect(&r4, r2->left, r2->top, r2->right, r2->top + 1);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
      SetRect(&r4, r2->left, r2->top, r2->left + 1, r2->bottom);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));
      SetRect(&r4, r2->right - 2, r2->top + 1, r2->right - 1, r2->bottom);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(&r4, r2->right - 1, r2->top + 2, r2->right, r2->bottom);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(&r4, r2->left + 1, r2->bottom - 2, r2->right, r2->bottom - 1);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(&r4, r2->left + 1, r2->bottom - 1, r2->right, r2->bottom);
      FillRect(hdc, &r4, static_cast<HBRUSH>(GetStockObject(DKGRAY_BRUSH)));
      SetRect(r3, r2->left + 1, r2->top + 1, r2->right - 2, r2->bottom - 2);
    }
  FillRect(hdc, r3, static_cast<HBRUSH>(GetStockObject(LTGRAY_BRUSH)));
}

static void
draw_filter_button_pic(HDC hdc, const RECT* r, int button_number, int pic_number, bool pushed)
{
  RECT r2, r3;
  filterbuttons_setcoords(r, button_number, &r2);
  draw_filter_button_3d(hdc, &r2, &r3, pushed);
  InflateRect(&r3, -1, -1);

  HANDLE hbmp = global_pics[pic_number];
  SelectObject(global_hdc, hbmp);
  BITMAP bmp;
  GetObject(hbmp, sizeof(BITMAP), &bmp);
  StretchBlt(hdc,
	     r3.left, r3.top,
	     r3.right - r3.left,
	     r3.bottom - r3.top,
	     global_hdc,
	     0, 0,
	     bmp.bmWidth, bmp.bmHeight,
	     SRCCOPY);
  SelectObject(global_hdc, global_hbmp);
}

static void
draw_filter_buttons(HDC hdc, const RECT* r)
{
  HDC chdc = CreateCompatibleDC(hdc);
  gdi_flush(chdc);

  SelectObject(chdc, global_pics[15]);
  BITMAP bmp;
  GetObject(global_pics[15], sizeof(BITMAP), &bmp);

  StretchBlt(hdc, 0, 0, r->right, r->bottom, chdc, 0, 0, bmp.bmWidth, bmp.bmHeight, SRCCOPY);
  DELETE_DC(chdc);

  draw_filter_button_pic(hdc, r, 11, 35, global_filter_colors & FC_WHITE);
  draw_filter_button_pic(hdc, r, 14, 31, global_filter_colors & FC_RED);
  draw_filter_button_pic(hdc, r, 12, 11, global_filter_colors & FC_BLUE);
  draw_filter_button_pic(hdc, r, 15, 22, global_filter_colors & FC_GREEN);
  draw_filter_button_pic(hdc, r, 13, 5, global_filter_colors & FC_BLACK);
  draw_filter_button_pic(hdc, r, 16, 21, global_filter_colors & FC_GOLD);
  draw_filter_button_pic(hdc, r, 17, 26, global_filter_cardtypes & FT_LAND);
  draw_filter_button_pic(hdc, r, 18, 3, global_filter_cardtypes & FT_ARTIFACT);
  draw_filter_button_pic(hdc, r, 19, 13, global_filter_cardtypes & FT_CREATURE);
  draw_filter_button_pic(hdc, r, 20, 18, global_filter_cardtypes & FT_ENCHANTMENT);
  draw_filter_button_pic(hdc, r, 21, 24, global_filter_cardtypes & FT_INSTANT);
  draw_filter_button_pic(hdc, r, 22, 25, global_filter_cardtypes & FT_INTERRUPT);
  draw_filter_button_pic(hdc, r, 23, 32, global_filter_cardtypes & FT_SORCERY);

  if (global_filter_cardsets_flags & FCSF_Q_ENABLE)
    {
      draw_filter_button_pic(hdc, r, 5, 20, global_filter_cardsets & FS_4TH_EDITION);
      draw_filter_button_pic(hdc, r, 6, 4, global_filter_cardsets & FS_ASTRAL);
      draw_filter_button_pic(hdc, r, 8, 1, global_filter_cardsets & FS_ANTIQUITIES);
      draw_filter_button_pic(hdc, r, 10, 14, global_filter_cardsets & FS_THE_DARK);
      draw_filter_button_pic(hdc, r, 7, 2, global_filter_cardsets & FS_ARABIAN_NIGHTS);
      draw_filter_button_pic(hdc, r, 9, 27, global_filter_cardsets & FS_LEGENDS);
      draw_filter_button_pic(hdc, r, 4, 28, global_filter_cardsets & FS_OTHER);
    }

  if (global_filtermenu_castcost_enabled)
    draw_filter_button_pic(hdc, r, 24, 12, global_filter_casting_cost & FN_ENABLE);
  if (global_filtermenu_power_enabled)
    draw_filter_button_pic(hdc, r, 25, 29, global_filter_power & FN_ENABLE);
  if (global_filtermenu_toughness_enabled)
    draw_filter_button_pic(hdc, r, 26, 34, global_filter_toughness & FN_ENABLE);
  if (global_filtermenu_ability_enabled)
    draw_filter_button_pic(hdc, r, 27, 0, global_filter_abilities & FA_ENABLE);
  if (global_filtermenu_rarity_enabled)
    draw_filter_button_pic(hdc, r, 28, 30, global_filter_rarity & FR_ENABLE);
}

static bool
toggle_filterbutton(int n)
{
  switch (n)
    {
#define TOGGLE(var, val)	(((var) ^= (val)), ((var) & (val)))
      case 4:	return TOGGLE(global_filter_cardsets, FS_OTHER);
      case 5:	return TOGGLE(global_filter_cardsets, FS_4TH_EDITION);
      case 6:	return TOGGLE(global_filter_cardsets, FS_ASTRAL);
      case 7:	return TOGGLE(global_filter_cardsets, FS_ARABIAN_NIGHTS);
      case 8:	return TOGGLE(global_filter_cardsets, FS_ANTIQUITIES);
      case 9:	return TOGGLE(global_filter_cardsets, FS_LEGENDS);
      case 10:	return TOGGLE(global_filter_cardsets, FS_THE_DARK);

      case 11:	return TOGGLE(global_filter_colors, FC_WHITE);
      case 12:	return TOGGLE(global_filter_colors, FC_BLUE);
      case 13:	return TOGGLE(global_filter_colors, FC_BLACK);
      case 14:	return TOGGLE(global_filter_colors, FC_RED);
      case 15:	return TOGGLE(global_filter_colors, FC_GREEN);
      case 16:	return TOGGLE(global_filter_colors, FC_GOLD);

      case 17:	return TOGGLE(global_filter_cardtypes, FT_LAND);
      case 18:	return TOGGLE(global_filter_cardtypes, FT_ARTIFACT);
      case 19:	return TOGGLE(global_filter_cardtypes, FT_CREATURE);
      case 20:	return TOGGLE(global_filter_cardtypes, FT_ENCHANTMENT);
      case 21:	return TOGGLE(global_filter_cardtypes, FT_INSTANT);
      case 22:	return TOGGLE(global_filter_cardtypes, FT_INTERRUPT);
      case 23:	return TOGGLE(global_filter_cardtypes, FT_SORCERY);

      case 24:	return !TOGGLE(global_filter_casting_cost, FN_ENABLE);
      case 25:	return !TOGGLE(global_filter_power, FN_ENABLE);
      case 26:	return !TOGGLE(global_filter_toughness, FN_ENABLE);
      case 27:	return !TOGGLE(global_filter_abilities, FA_ENABLE);
      case 28:	return !TOGGLE(global_filter_rarity, FR_ENABLE);

      default:	return false;
#undef TOGGLE
    }
}

static void
create_filter_menus(void)
{
  int n;

#define CREATE_FILTER_MENU(menu, name, base, condition, coda)	\
  if ((n = load_text("Menus", (name))) != -1)	\
    {						\
      (menu) = CreatePopupMenu();		\
      for (int i = 0; i < n; ++i)		\
	condition				\
	  {					\
	    AppendMenu((menu), MF_ENABLED, i + (base), text_lines[i]);	\
	  }					\
      coda;					\
    }

  CREATE_FILTER_MENU(global_filtermenu_default, "FILTERS", RES_FILTERMENU_MAINMENUBUTTONS_ON,,);
  CREATE_FILTER_MENU(global_filtermenu_fourth, "FOURTH", RES_FILTERMENU_FOURTH_UNLIMITED,,);
  CREATE_FILTER_MENU(global_filtermenu_gold, "GOLD", RES_FILTERMENU_GOLD_ALL,,);
  CREATE_FILTER_MENU(global_filtermenu_land, "LAND", RES_FILTERMENU_LAND_LANDANDMANA,,);
  CREATE_FILTER_MENU(global_filtermenu_artifact, "ARTIFACT", RES_FILTERMENU_ARTIFACT_CREATURES,,);

  CREATE_FILTER_MENU(global_filtermenu_creature, "CREATURE", RES_FILTERMENU_CREATURE_CREATURE,
		     if ((global_db_flags_1 & DBFLAGS_STANDALONE) || i != 1),
		     InsertMenu(global_filtermenu_creature, (global_db_flags_1 & DBFLAGS_STANDALONE) ? n - 1 : n - 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL));

  CREATE_FILTER_MENU(global_filtermenu_enchantment, "ENCHANTMENT", RES_FILTERMENU_ENCHANTMENT_ENCHANTMENTS,,);

  if (global_filtermenu_castcost_enabled)
    CREATE_FILTER_MENU(global_filtermenu_castcost, "CASTCOST", RES_FILTERMENU_COST_GREATER,,);

  if (global_filtermenu_power_enabled)
    CREATE_FILTER_MENU(global_filtermenu_power, "POWER", RES_FILTERMENU_POWER_GREATER,,);

  if (global_filtermenu_toughness_enabled)
    CREATE_FILTER_MENU(global_filtermenu_toughness, "TOUGHNESS", RES_FILTERMENU_TOUGHNESS_GREATER,,);

  if (global_filtermenu_ability_enabled)
    CREATE_FILTER_MENU(global_filtermenu_ability, "ABILITY", RES_FILTERMENU_ABILITY_NATIVE,
		       ,
		       InsertMenu(global_filtermenu_ability, 2, MF_BYPOSITION | MF_SEPARATOR, 0, NULL));

  if (global_filtermenu_rarity_enabled)
    CREATE_FILTER_MENU(global_filtermenu_rarity, "RARITY", RES_FILTERMENU_RARITY_COMMON,,);

  CREATE_FILTER_MENU(global_filtermenu_newexp, "NEWEXP", RES_FILTERMENU_EXPANSIONLIST,,);

#undef CREATE_FILTER_MENU
}

static void
destroy_filter_menus(void)
{
  DESTROY_MENU(global_filtermenu_default);
  DESTROY_MENU(global_filtermenu_fourth);
  DESTROY_MENU(global_filtermenu_gold);
  DESTROY_MENU(global_filtermenu_land);
  DESTROY_MENU(global_filtermenu_artifact);
  DESTROY_MENU(global_filtermenu_creature);
  DESTROY_MENU(global_filtermenu_enchantment);
  DESTROY_MENU(global_filtermenu_castcost);
  DESTROY_MENU(global_filtermenu_power);
  DESTROY_MENU(global_filtermenu_toughness);
  DESTROY_MENU(global_filtermenu_ability);
  DESTROY_MENU(global_filtermenu_rarity);
  DESTROY_MENU(global_filtermenu_newexp);
}

static bool
check_card_global_deck_availability(csvid_t csvid)
{
  for (int i = 0; i < global_deck_num_entries; ++i)
    if (global_deck[i].GDE_csvid == csvid && global_deck[i].GDE_Available == 1)
      return true;

  return false;
}

static bool
check_expansion_list_filter(csvid_t csvid)
{
  for (int expid = 0; expid < global_num_expansions; ++expid)
    if ((1 << (static_cast<uint32_t>(expid) & 0x1F)) & global_filter_expansion_list[expid / 32])
      {
	int bit_pos = expid * 3;
	int bit_mask = 7 << (bit_pos & 7);
	int offset = (csvid.raw * global_expansion_size) + (bit_pos / 8);
	if ((MAKEWORD(global_raw_rarities[offset], global_raw_rarities[offset + 1]) & bit_mask) >> (bit_pos & 0x7))
	  return true;
      }

  return false;
}

static bool
is_valid_card(csvid_t csvid)
{
  if (global_is_valid_card_fn)
    return global_is_valid_card_fn(csvid.raw);
  else
    return (csvid.raw < global_available_slots
	    && (global_cfg_view_all
		|| (card_coded[csvid.raw / 8] & (1 << (csvid.raw % 8)))));
}

static bool
check_set_availability(csvid_t csvid, unsigned int allowed_sets)
{
  if (!is_valid_card(csvid))
    return false;

  OrigRarities* r = &global_origrarities[csvid.raw];
  OrigRaritySet rs = r->set;

  if (rs == SET_INVALID)
    return false;

  return (((allowed_sets & 1) && r->exp_rarities[1] != '-')
	  || ((allowed_sets & 4) && r->exp_rarities[0] != '-')
	  || ((allowed_sets & 2) && rs == SET_UNLIMITED)
	  || ((allowed_sets & 8) && rs == SET_ARABIAN_NIGHTS)
	  || ((allowed_sets & 0x10) && !r->set)
	  || ((allowed_sets & 0x20) && (rs == SET_ASTRAL || rs == SET_PROMO))
	  || ((allowed_sets & 0x40) && rs == SET_LEGENDS)
	  || ((allowed_sets & 0x80) && rs == SET_THE_DARK)
	  || ((allowed_sets & 0x100) && rs == SET_8TH_EDITION)
	  || ((allowed_sets & 0x200) && rs == SET_FALLEN_EMPIRES)
	  || ((allowed_sets & 0x800) && rs == SET_RANDOM)
	  || ((allowed_sets & 0x400) && rs == SET_TEMPEST));
}

static bool
check_lands(int cardtype, int mana_source_colors)
{
  return ((global_filter_cardtypes & FT_LAND)
	  && (   (global_filter_cardtypes & FT_LAND_LAND_AND_MANA	&& cardtype == CP_TYPE_LAND && mana_source_colors)
	      || (global_filter_cardtypes & FT_LAND_LAND_ONLY		&& cardtype == CP_TYPE_LAND && !mana_source_colors)
	      || (global_filter_cardtypes & FT_LAND_MANA_ONLY		&& cardtype != CP_TYPE_LAND && mana_source_colors)));
}

static bool
check_artifacts(int cardtype, int subtype1)
{
  return (cardtype == CP_TYPE_ARTIFACT
	  && (global_filter_cardtypes & FT_ARTIFACT)
	  && (subtype1 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE
	      ? (global_filter_cardtypes & FT_ARTIFACT_CREATURE)
	      : (global_filter_cardtypes & FT_ARTIFACT_NON_CREATURE)));
}

static bool
check_creature_list_filter(const card_ptr_t* cp)
{
  for (int crtid = 0; crtid < (CREATURE_LIST_SIZE * 32); ++crtid)
    if ((1 << (static_cast<uint32_t>(crtid) & 0x1F)) & global_filter_creature_list[crtid / 32])
      for (int i = 0; i < 7; ++i)
	if (cp->types[i] == crtid)
	  return true;

  return false;
}

static bool
check_creatures(const card_ptr_t* cp)
{
  if (!(global_filter_cardtypes & FT_CREATURE))
    return false;

  if (((global_filter_cardtypes & FT_CREATURE_CREATURE) && cp->card_type == CP_TYPE_CREATURE)
      || ((global_filter_cardtypes & FT_CREATURE_TOKEN) && cp->card_type == CP_TYPE_TOKEN))
    return true;

  if ((global_filter_cardtypes & FT_CREATURE_ARTIFACT) && cp->card_type == CP_TYPE_ARTIFACT
      && cp->subtype1 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE)
    return true;

  if (global_filter_cardtypes & FT_CREATURE_LIST)
    return check_creature_list_filter(cp);

  return false;
}

static bool
check_enchantments(const card_ptr_t* cp)
{
  if (!(global_filter_cardtypes & FT_ENCHANTMENT) || cp->card_type != CP_TYPE_ENCHANTMENT)
    return false;

  uint16_t s1 = cp->subtype1;
  uint16_t s2 = cp->subtype2;

  bool world = s1 == HARDCODED_SUBTYPE_WORLD || s2 == HARDCODED_SUBTYPE_WORLD;
  if (world && (global_filter_cardtypes & FT_ENCHANTMENT_WORLD))
    return true;

  bool land = s1 == HARDCODED_SUBTYPE_LAND || s2 == HARDCODED_SUBTYPE_LAND;
  if (land && (global_filter_cardtypes & FT_ENCHANTMENT_LAND))
    return true;

  bool creature_nonwall = (s1 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE
			   || s2 == HARDCODED_SUBTYPE_ARTIFACT_CREATURE_OR_AURA_MOSTLY_WITH_ENCHANT_CREATURE);
  bool creature_or_wall = creature_nonwall || s1 == HARDCODED_SUBTYPE_WALL || s2 == HARDCODED_SUBTYPE_WALL;
  if (creature_or_wall && (global_filter_cardtypes & FT_ENCHANTMENT_CREATURE))
    return true;

  bool artifact = s1 == HARDCODED_SUBTYPE_ARTIFACT || s2 == HARDCODED_SUBTYPE_ARTIFACT;
  if (artifact && (global_filter_cardtypes & FT_ENCHANTMENT_ARTIFACT))
    return true;

  bool enchant = s1 == HARDCODED_SUBTYPE_ENCHANTMENT || s2 == HARDCODED_SUBTYPE_ENCHANTMENT;
  if (enchant && (global_filter_cardtypes & FT_ENCHANTMENT_ENCHANT))
    return true;

  bool player = s1 == HARDCODED_SUBTYPE_PLAYER || s2 == HARDCODED_SUBTYPE_PLAYER;
  if (player && (global_filter_cardtypes & FT_ENCHANTMENT_PLAYER))
    return true;

  bool instant = s1 == HARDCODED_SUBTYPE_INSTANT || s2 == HARDCODED_SUBTYPE_INSTANT;
  if (instant && (global_filter_cardtypes & FT_ENCHANTMENT_INSTANT))
    return true;

  bool permanent = s1 == HARDCODED_SUBTYPE_PERMANENT || s2 == HARDCODED_SUBTYPE_PERMANENT;
  if (permanent && (global_filter_cardtypes & FT_ENCHANTMENT_PERMANENT))
    return true;

  bool aura_or_world = world || land || creature_or_wall || artifact || enchant || player || instant || permanent;
  if (!aura_or_world && (global_filter_cardtypes & FT_ENCHANTMENT_ENCHANTMENTS))
    return true;

  return false;
}

static bool
check_casting_cost(const card_ptr_t* cp)
{
  if (!(global_filter_casting_cost & FN_ENABLE))
    return true;

  int colorless = cp->req_colorless;
  if (colorless >= 40)
    return global_filter_casting_cost & FN_CC_X;
  else
    {
      int cmc = colorless + cp->req_black + cp->req_white + cp->req_red + cp->req_green + cp->req_blue + cp->req_hybrid;
      return (   ((global_filter_casting_cost & FN_GT) && global_filter_casting_cost_value <= cmc)
	      || ((global_filter_casting_cost & FN_LT) && global_filter_casting_cost_value >= cmc)
	      || ((global_filter_casting_cost & FN_EQ) && global_filter_casting_cost_value == cmc));
    }
}

static bool
check_power(int cp_power)
{
  if (cp_power > 0)
    cp_power &= 0xFFF;

  return (!(global_filter_power & FN_ENABLE)
	  || ((global_filter_power & FN_GT) && global_filter_power_value <= cp_power)
	  || ((global_filter_power & FN_LT) && global_filter_power_value >= cp_power)
	  || ((global_filter_power & FN_EQ) && global_filter_power_value == cp_power));
}

static bool
check_toughness(int cp_toughness)
{
  if (cp_toughness > 0)
    cp_toughness &= 0xFFF;

  return (!(global_filter_toughness & FN_ENABLE)
	  || ((global_filter_toughness & FN_GT) && global_filter_toughness_value <= cp_toughness)
	  || ((global_filter_toughness & FN_LT) && global_filter_toughness_value >= cp_toughness)
	  || ((global_filter_toughness & FN_EQ) && global_filter_toughness_value == cp_toughness));
}

static bool
check_rarity(csvid_t csvid, int cp_rarity)
{
  if (cp_rarity >= 5)
    {
      char txt[80];
      sprintf(txt, "Card Number %d does not have a valid rarity value!", csvid.raw);
      MessageBox(global_main_hwnd, txt, "Card Error", MB_ICONERROR);
      return false;
    }

  return (!(global_filter_rarity & FR_ENABLE)
	  || ((global_filter_rarity & FR_COMMON)	&& cp_rarity <= 1)
	  || ((global_filter_rarity & FR_UNCOMMON)	&& cp_rarity == 4)
	  || ((global_filter_rarity & FR_RARE)		&& cp_rarity == 2)
	  || ((global_filter_rarity & FR_RESTRICTED)	&& check_restricted(csvid))
	  || ((global_filter_rarity & FR_BANNED)	&& check_banned(csvid)));
}

static bool
check_abilities(csvid_t csvid, int num_abils, char* abils)
{
  if (!(global_filter_abilities & 1))
    return true;

  bool native = global_filter_abilities & 2;
  bool grants = global_filter_abilities & 4;
  if (!native && !grants)
    return false;

  for (int i = 0; i < num_abils; ++i)
    switch (static_cast<Abilities>(abils[i]))
      {
#define MATCH(native_or_granted, bit)	if ((native_or_granted) && (global_filter_abilities & (bit)))	return true;	else break

	case ABIL_NATIVE_FLYING:	MATCH(native, FA_FLYING);
	case ABIL_GRANTS_FLYING:	MATCH(grants, FA_FLYING);

	case ABIL_NATIVE_FIRSTSTRIKE:	MATCH(native, FA_FIRSTSTRIKE);	// or double strike
	case ABIL_GRANTS_FIRSTSTRIKE:	MATCH(grants, FA_FIRSTSTRIKE);	// or double strike

	case ABIL_NATIVE_TRAMPLE:	MATCH(native, FA_TRAMPLE);
	case ABIL_GRANTS_TRAMPLE:	MATCH(grants, FA_TRAMPLE);

	case ABIL_NATIVE_REGENERATION:	MATCH(native, FA_REGENERATION);
	case ABIL_GRANTS_REGENERATION:	MATCH(grants, FA_REGENERATION);

	case ABIL_NATIVE_BANDING:	MATCH(native, FA_BANDING);
	case ABIL_GRANTS_BANDING:	MATCH(grants, FA_BANDING);

	case ABIL_NATIVE_PROTECTION_FROM_BLACK:		MATCH(native, FA_PROTECTION);
	case ABIL_NATIVE_PROTECTION_FROM_RED:		MATCH(native, FA_PROTECTION);
	case ABIL_NATIVE_PROTECTION_FROM_WHITE:		MATCH(native, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_BLACK:		MATCH(grants, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_RED:		MATCH(grants, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_WHITE:		MATCH(grants, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_BLUE:		MATCH(grants, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_GREEN:		MATCH(grants, FA_PROTECTION);
	case ABIL_GRANTS_PROTECTION_FROM_ARTIFACTS:	MATCH(grants, FA_PROTECTION);

	case ABIL_NATIVE_DESERTWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_FORESTWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_ISLANDWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_LEGENDARY_LANDWALK:	MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_MOUNTAINWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_PLAINSWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_NATIVE_SWAMPWALK:		MATCH(native, FA_LANDWALK);
	case ABIL_GRANTS_FORESTWALK:		MATCH(grants, FA_LANDWALK);
	case ABIL_GRANTS_ISLANDWALK:		MATCH(grants, FA_LANDWALK);
	case ABIL_GRANTS_MOUNTAINWALK:		MATCH(grants, FA_LANDWALK);
	case ABIL_GRANTS_PLAINSWALK:		MATCH(grants, FA_LANDWALK);
	case ABIL_GRANTS_SWAMPWALK:		MATCH(grants, FA_LANDWALK);

	case ABIL_NATIVE_INFECT:	MATCH(native, FA_INFECT);

	case ABIL_NATIVE_RAMPAGE:	MATCH(native, FA_RAMPAGE);
	case ABIL_GRANTS_RAMPAGE:	MATCH(grants, FA_RAMPAGE);

	case ABIL_NATIVE_REACH:		MATCH(native, FA_REACH);
	case ABIL_GRANTS_REACH:		MATCH(grants, FA_REACH);

	case ABIL_NATIVE_DEATHTOUCH:	MATCH(native, FA_DEATHTOUCH);
	case ABIL_GRANTS_DEATHTOUCH:	MATCH(grants, FA_DEATHTOUCH);

	case ABIL_NATIVE_VIGILANCE:	MATCH(native, FA_VIGILANCE);
	case ABIL_GRANTS_VIGILANCE:	MATCH(grants, FA_VIGILANCE);

	case ABIL_NATIVE_HASTE:		MATCH(native, FA_HASTE);
	case ABIL_GRANTS_HASTE:		MATCH(grants, FA_HASTE);

#undef MATCH
      }

  return false;
}

static bool
check_filters(csvid_t csvid)
{
  const card_ptr_t* cp = &cards_ptr[csvid.raw];

  if (!strcmp(cp->full_name, "Blank")
      || !strcmp(cp->full_name, "Empty")
      || !strcmp(cp->full_name, "None"))
    return false;

  if ((global_db_flags_1 & (DBFLAGS_EDITDECK|DBFLAGS_SHANDALAR|0x20))
      && !check_card_global_deck_availability(csvid))
    return false;

  switch (cp->color)
    {
      case CP_COLOR_MULTI:
	if (!(global_filter_colors & FC_GOLD))
	  return false;
	if (global_filter_colors & FC_GOLD_ALL)
	  break;

	color_test_t cols;
	if (cp->req_hybrid > 0)
	  cols = static_cast<color_test_t>(cp->hybrid_type);	// This works because all hybrid_t values exactly match a color_test_t
	else
	  cols = COLOR_TEST_0;

	if (cp->req_white > 0)
	  cols |= COLOR_TEST_WHITE;
	if (cp->req_blue > 0)
	  cols |= COLOR_TEST_BLUE;
	if (cp->req_black > 0)
	  cols |= COLOR_TEST_BLACK;
	if (cp->req_red > 0)
	  cols |= COLOR_TEST_RED;
	if (cp->req_green > 0)
	  cols |= COLOR_TEST_GREEN;

	if (global_filter_colors & FC_GOLD_ALLSELECTED)
	  {
	    if (!(global_filter_colors & FC_WHITE)	!= !(cols & COLOR_TEST_WHITE))
	      return false;
	    if (!(global_filter_colors & FC_BLUE)	!= !(cols & COLOR_TEST_BLUE))
	      return false;
	    if (!(global_filter_colors & FC_BLACK)	!= !(cols & COLOR_TEST_BLACK))
	      return false;
	    if (!(global_filter_colors & FC_RED)	!= !(cols & COLOR_TEST_RED))
	      return false;
	    if (!(global_filter_colors & FC_GREEN)	!= !(cols & COLOR_TEST_GREEN))
	      return false;
	  }
	else
	  {
	    if (   !((cols & COLOR_TEST_WHITE)	&& (global_filter_colors & FC_WHITE))
		&& !((cols & COLOR_TEST_BLUE)	&& (global_filter_colors & FC_BLUE))
		&& !((cols & COLOR_TEST_BLACK)	&& (global_filter_colors & FC_BLACK))
		&& !((cols & COLOR_TEST_RED)	&& (global_filter_colors & FC_RED))
		&& !((cols & COLOR_TEST_GREEN)	&& (global_filter_colors & FC_GREEN)))
	      return false;
	  }
	break;

      case CP_COLOR_WHITE:
	if (!(global_filter_colors & FC_WHITE))
	  return false;
	else
	  break;

      case CP_COLOR_BLUE:
	if (!(global_filter_colors & FC_BLUE))
	  return false;
	else
	  break;

      case CP_COLOR_BLACK:
	if (!(global_filter_colors & FC_BLACK))
	  return false;
	else
	  break;

      case CP_COLOR_RED:
	if (!(global_filter_colors & FC_RED))
	  return false;
	else
	  break;

      case CP_COLOR_GREEN:
	if (!(global_filter_colors & FC_GREEN))
	  return false;
	else
	  break;

      case CP_COLOR_LESS:
      case CP_COLOR_LAND:
      case CP_COLOR_ARTIFACT:
	break;

      default:
	return false;
    }

  if (global_filter_expansions & FE_EXPANSIONLIST)
    {
      if (!is_valid_card(csvid) || !check_expansion_list_filter(csvid))
	return false;
    }
  else
    {
      int allowed_sets_mask = 0;

      if (global_filter_cardsets & FS_4TH_EDITION)
	{
	  if (global_filter_expansions & 0x2)
	    allowed_sets_mask = 1;
	  if (global_filter_expansions & 0x1000)
	    allowed_sets_mask |= 2;
	  if (global_filter_expansions & 0x20)
	    allowed_sets_mask |= 4;
	}

      if (global_filter_cardsets & FS_ASTRAL)
	allowed_sets_mask |= 0x20;

      if (global_filter_cardsets & FS_ARABIAN_NIGHTS)
	allowed_sets_mask |= 8;

      if (global_filter_cardsets & FS_ANTIQUITIES)
	allowed_sets_mask |= 0x10;

      if (global_filter_cardsets & FS_LEGENDS)
	allowed_sets_mask |= 0x40;

      if (global_filter_cardsets & FS_OTHER)
	{
	  if (global_filter_expansions & 0x0100)
	    allowed_sets_mask |= 0x100;
	  if (global_filter_expansions & 0x0200)
	    allowed_sets_mask |= 0x200;
	  if (global_filter_expansions & 0x0400)
	    allowed_sets_mask |= 0x400;
	  if (global_filter_expansions & 0x0800)
	    allowed_sets_mask |= 0x800;
	}

      if (global_filter_cardsets & FS_THE_DARK)
	allowed_sets_mask |= 0x80;

      if (!check_set_availability(csvid, allowed_sets_mask))
	return false;
    }

  //char* entry = &global_raw_dbinfo[16 * csvid.raw];
  char* abils = &global_raw_dbinfo[16 * csvid.raw + 8];
  //int dbcardtype2 = *reinterpret_cast<int*>(entry);

  if (!check_lands(cp->card_type, cp->mana_source_colors & (COLOR_TEST_ANY|COLOR_TEST_ARTIFACT))
      && !check_artifacts(cp->card_type, cp->subtype1)
      && !check_creatures(cp)
      && !check_enchantments(cp)
      && !(cp->card_type == CP_TYPE_INSTANT && (global_filter_cardtypes & FT_INSTANT))
      && !(cp->card_type == CP_TYPE_INTERRUPT && (global_filter_cardtypes & FT_INTERRUPT))
      && !(cp->card_type == CP_TYPE_SORCERY && (global_filter_cardtypes & FT_SORCERY)))
    return false;

  if (!check_casting_cost(cp)
      || !check_power(cp->power)
      || !check_toughness(cp->toughness)
      || !check_rarity(csvid, cp->rarity)
      || !check_abilities(csvid, 4, abils))
    return false;

  return true;
}

LRESULT CALLBACK
wndproc_CardListFilterClass(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  static RECT rect = {0, 0, 0, 0};
  static HDC chdc = NULL;
  static HBITMAP bmp = NULL;
  static FilterTypes stored_creature_filters = FT_0;

  HDC hdc;
  HMENU menu;
  POINT p;

  switch (msg)
    {
      case WM_PAINT:
	PAINTSTRUCT paint;
	hdc = BeginPaint(hwnd, &paint);
	gdi_flush(hdc);
	BitBlt(hdc, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, chdc, 0, 0, SRCCOPY);
	EndPaint(hwnd, &paint);
	return 0;

      case WM_CREATE:
	global_filter_colors = FC_WHITE | FC_GREEN | FC_RED | FC_BLACK | FC_BLUE | FC_GOLD | FC_GOLD_ALL;

	global_filter_expansions = FE_4TH_EDITION | FE_REVISED | FE_UNLIMITED | static_cast<FilterExpansions>(0x0F00);

	global_filter_cardtypes = (FT_LAND | FT_ARTIFACT | FT_CREATURE | FT_ENCHANTMENT | FT_INSTANT | FT_INTERRUPT | FT_SORCERY
				   | FT_ARTIFACT_NON_CREATURE | FT_ARTIFACT_CREATURE
				   | FT_LAND_LAND_AND_MANA | FT_LAND_LAND_ONLY
				   | FT_CREATURE_CREATURE | FT_CREATURE_TOKEN | FT_CREATURE_ARTIFACT
				   | (FT_ENCHANTMENT_ENCHANTMENTS | FT_ENCHANTMENT_WORLD
				      | FT_ENCHANTMENT_LAND | FT_ENCHANTMENT_CREATURE | FT_ENCHANTMENT_ARTIFACT | FT_ENCHANTMENT_ENCHANT
				      | FT_ENCHANTMENT_PERMANENT | FT_ENCHANTMENT_PLAYER | FT_ENCHANTMENT_INSTANT));

	global_filter_casting_cost = FN_GT;
	global_filter_casting_cost_value = 0;

	global_filter_power = FN_GT;
	global_filter_power_value = 0;

	global_filter_toughness = FN_GT;
	global_filter_toughness_value = 0;

	global_filter_abilities = (FA_NATIVE | FA_GRANTS
				   | FA_FLYING | FA_FIRSTSTRIKE | FA_TRAMPLE | FA_REGENERATION | FA_BANDING | FA_PROTECTION | FA_LANDWALK
				   | FA_INFECT | FA_RAMPAGE | FA_REACH | FA_DEATHTOUCH | FA_VIGILANCE | FA_HASTE);

	global_filter_rarity = FR_COMMON | FR_UNCOMMON | FR_RARE | FR_RESTRICTED | FR_BANNED;

	memset(global_filter_creature_list, -1, sizeof global_filter_creature_list);
	memset(global_filter_expansion_list, -1, sizeof global_filter_expansion_list);

	if (global_db_flags_1 & (DBFLAGS_STANDALONE | DBFLAGS_NOCARDCOUNTCHECK | DBFLAGS_GAUNTLET | DBFLAGS_EDITDECK))
	  global_filter_cardsets_flags = (FCSF_Q_OTHER | FCSF_LEGENDS | FCSF_THE_DARK | FCSF_ANTIQUITIES
					  | FCSF_ARABIAN_NIGHTS | FCSF_Q_ASTRAL | FCSF_Q_ENABLE);
	else
	  global_filter_cardsets_flags = FCSF_0;

	global_filter_cardsets = (FS_OTHER | FS_LEGENDS | FS_THE_DARK | FS_ANTIQUITIES
				  | FS_ARABIAN_NIGHTS | FS_ASTRAL | FS_4TH_EDITION);

	global_filtermenu_castcost_enabled = true;
	global_filtermenu_power_enabled = true;
	global_filtermenu_toughness_enabled = true;
	global_filtermenu_ability_enabled = true;
	global_filtermenu_rarity_enabled = true;
	global_filtermenu_gold_enabled = true;

	filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);

	create_filter_menus();
	return 0;

      case WM_INITMENU:
	menu = reinterpret_cast<HMENU>(wparam);
	if (menu == global_filtermenu_newexp)
	  CHECKMENU_IF(global_filtermenu_newexp, RES_FILTERMENU_EXPANSIONLIST,	global_filter_expansions & FE_EXPANSIONLIST);
	else if (menu == global_filtermenu_fourth)
	  {
	    CHECKMENU_IF(global_filtermenu_fourth, RES_FILTERMENU_FOURTH_UNLIMITED,	global_filter_expansions & FE_UNLIMITED);
	    CHECKMENU_IF(global_filtermenu_fourth, RES_FILTERMENU_FOURTH_REVISED,	global_filter_expansions & FE_REVISED);
	    CHECKMENU_IF(global_filtermenu_fourth, RES_FILTERMENU_FOURTH_FOURTH,	global_filter_expansions & FE_4TH_EDITION);
	  }
	else if (menu == global_filtermenu_gold)
	  {
	    CHECKMENU_IF(global_filtermenu_gold, RES_FILTERMENU_GOLD_ALL,		global_filter_colors & FC_GOLD_ALL);
	    CHECKMENU_IF(global_filtermenu_gold, RES_FILTERMENU_GOLD_MATCHINGALL,	global_filter_colors & FC_GOLD_ALLSELECTED);
	    CHECKMENU_IF(global_filtermenu_gold, RES_FILTERMENU_GOLD_MATCHINGANY,	global_filter_colors & FC_GOLD_ANYSELECTED);
	  }
	else if (menu == global_filtermenu_land)
	  {
	    CHECKMENU_IF(global_filtermenu_land, RES_FILTERMENU_LAND_LANDANDMANA,	global_filter_cardtypes & FT_LAND_LAND_AND_MANA);
	    CHECKMENU_IF(global_filtermenu_land, RES_FILTERMENU_LAND_LANDONLY,		global_filter_cardtypes & FT_LAND_LAND_ONLY);
	    CHECKMENU_IF(global_filtermenu_land, RES_FILTERMENU_LAND_MANAONLY,		global_filter_cardtypes & FT_LAND_MANA_ONLY);
	  }
	else if (menu == global_filtermenu_artifact)
	  {
	    CHECKMENU_IF(global_filtermenu_artifact, RES_FILTERMENU_ARTIFACT_CREATURES,	global_filter_cardtypes & FT_ARTIFACT_CREATURE);
	    CHECKMENU_IF(global_filtermenu_artifact, RES_FILTERMENU_ARTIFACT_NONCREATURES,global_filter_cardtypes & FT_ARTIFACT_NON_CREATURE);
	  }
	else if (menu == global_filtermenu_creature)
	  {
	    CHECKMENU_IF(global_filtermenu_creature, RES_FILTERMENU_CREATURE_CREATURE,	global_filter_cardtypes & FT_CREATURE_CREATURE);
	    CHECKMENU_IF(global_filtermenu_creature, RES_FILTERMENU_CREATURE_TOKEN,	global_filter_cardtypes & FT_CREATURE_TOKEN);
	    CHECKMENU_IF(global_filtermenu_creature, RES_FILTERMENU_CREATURE_ARTIFACT,	global_filter_cardtypes & FT_CREATURE_ARTIFACT);
	    CHECKMENU_IF(global_filtermenu_creature, RES_FILTERMENU_CREATURE_LIST,	global_filter_cardtypes & FT_CREATURE_LIST);
	  }
	else if (menu == global_filtermenu_enchantment)
	  {
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_ENCHANTMENTS,global_filter_cardtypes & FT_ENCHANTMENT_ENCHANTMENTS);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_WORLD,	global_filter_cardtypes & FT_ENCHANTMENT_WORLD);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_LAND,	global_filter_cardtypes & FT_ENCHANTMENT_LAND);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_CREATURE,	global_filter_cardtypes & FT_ENCHANTMENT_CREATURE);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_ARTIFACT,	global_filter_cardtypes & FT_ENCHANTMENT_ARTIFACT);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_ENCHANT,	global_filter_cardtypes & FT_ENCHANTMENT_ENCHANT);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_PERMANENT,	global_filter_cardtypes & FT_ENCHANTMENT_PERMANENT);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_PLAYER,	global_filter_cardtypes & FT_ENCHANTMENT_PLAYER);
	    CHECKMENU_IF(global_filtermenu_enchantment, RES_FILTERMENU_ENCHANTMENT_INSTANT,	global_filter_cardtypes & FT_ENCHANTMENT_INSTANT);
	  }
	else if (menu == global_filtermenu_castcost)
	  {
	    CHECKMENU_IF(global_filtermenu_castcost, RES_FILTERMENU_COST_GREATER,	global_filter_casting_cost & FN_GT);
	    CHECKMENU_IF(global_filtermenu_castcost, RES_FILTERMENU_COST_LESSER,	global_filter_casting_cost & FN_LT);
	    CHECKMENU_IF(global_filtermenu_castcost, RES_FILTERMENU_COST_EQUAL,		global_filter_casting_cost & FN_EQ);
	    CHECKMENU_IF(global_filtermenu_castcost, RES_FILTERMENU_COST_X,		global_filter_casting_cost & FN_CC_X);
	  }
	else if (menu == global_filtermenu_power)
	  {
	    CHECKMENU_IF(global_filtermenu_power, RES_FILTERMENU_POWER_GREATER,	global_filter_power & FN_GT);
	    CHECKMENU_IF(global_filtermenu_power, RES_FILTERMENU_POWER_LESSER,	global_filter_power & FN_LT);
	    CHECKMENU_IF(global_filtermenu_power, RES_FILTERMENU_POWER_EQUAL,	global_filter_power & FN_EQ);
	  }
	else if (menu == global_filtermenu_toughness)
	  {
	    CHECKMENU_IF(global_filtermenu_toughness, RES_FILTERMENU_TOUGHNESS_GREATER,	global_filter_toughness & FN_GT);
	    CHECKMENU_IF(global_filtermenu_toughness, RES_FILTERMENU_TOUGHNESS_LESSER,	global_filter_toughness & FN_LT);
	    CHECKMENU_IF(global_filtermenu_toughness, RES_FILTERMENU_TOUGHNESS_EQUAL,	global_filter_toughness & FN_EQ);
	  }
	else if (menu == global_filtermenu_ability)
	  {
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_NATIVE,	global_filter_abilities & FA_NATIVE);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_GIVES,	global_filter_abilities & FA_GRANTS);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_FLYING,	global_filter_abilities & FA_FLYING);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_FIRSTSTRIKE,	global_filter_abilities & FA_FIRSTSTRIKE);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_TRAMPLE,	global_filter_abilities & FA_TRAMPLE);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_REGENERATION,global_filter_abilities & FA_REGENERATION);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_BANDING,	global_filter_abilities & FA_BANDING);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_COLORWARD,	global_filter_abilities & FA_PROTECTION);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_LANDWALK,	global_filter_abilities & FA_LANDWALK);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_POISON,	global_filter_abilities & FA_INFECT);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_RAMPAGE,	global_filter_abilities & FA_RAMPAGE);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_REACH,	global_filter_abilities & FA_REACH);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_STONING,	global_filter_abilities & FA_DEATHTOUCH);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_VIGILANCE,	global_filter_abilities & FA_VIGILANCE);
	    CHECKMENU_IF(global_filtermenu_ability, RES_FILTERMENU_ABILITY_HASTE,	global_filter_abilities & FA_HASTE);
	  }
	else if (menu == global_filtermenu_rarity)
	  {
	    CHECKMENU_IF(global_filtermenu_rarity, RES_FILTERMENU_RARITY_COMMON,	global_filter_rarity & FR_COMMON);
	    CHECKMENU_IF(global_filtermenu_rarity, RES_FILTERMENU_RARITY_UNCOMMON,	global_filter_rarity & FR_UNCOMMON);
	    CHECKMENU_IF(global_filtermenu_rarity, RES_FILTERMENU_RARITY_RARE,		global_filter_rarity & FR_RARE);
	    CHECKMENU_IF(global_filtermenu_rarity, RES_FILTERMENU_RARITY_RESTRICTED,	global_filter_rarity & FR_RESTRICTED);
	    CHECKMENU_IF(global_filtermenu_rarity, RES_FILTERMENU_RARITY_BANNED,	global_filter_rarity & FR_BANNED);
	  }
	return 0;

      case WM_DESTROY:
	DELETE_DC(chdc);
	DELETE_OBJ(bmp);
	destroy_filter_menus();
	KillTimer(hwnd, 1);
	return 0;

      case WM_LBUTTONDOWN:
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);

	RECT r, r2;
	GetClientRect(hwnd, &r);
	for (int i = 0; i < 35; ++i)
	  {
	    filterbuttons_setcoords(&r, i, &r2);
	    if (PtInRect(&r2, p))
	      {
		if (global_cfg_effects)
		  play_sound(toggle_filterbutton(i) ? 4 : 5, 400, 0, 0);

		SendMessage(global_horzlist_hwnd, 0x8007, 0, 0);
		filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);
		draw_filter_buttons(chdc, &r);
		InvalidateRect(hwnd, NULL, FALSE);
		return 0;
	      }
	  }
	return 0;

      case WM_RBUTTONDOWN:
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);
	if ((menu = select_filter_menu(hwnd, p)))
	  {
	    ClientToScreen(hwnd, &p);
	    TrackPopupMenu(menu, TPM_RIGHTBUTTON, p.x, p.y, 0, hwnd, NULL);
	  }
	return 0;

      case WM_COMMAND:
	bool refresh_filters;
	refresh_filters = false;
	// This is the part that breaks decompilation.  Starting at 0x40852a, change "FF 24 85 14 8a 40 00" to "E9 02 00 00 00 90 90" to force case 1.

	switch (int cmd = LOWORD(wparam))
	  {
#define TOGGLE_FILTER_ADD(button, val, bit, base, insert1)	\
	    button:				\
	      val ^= bit;			\
	      insert1;				\
	      if (val & base)			\
		refresh_filters = true

#define TOGGLE_FILTER(button, val, bit, base)			TOGGLE_FILTER_ADD(button, val, bit, base,)

#define GLE_FILTER(buttonbase, valbase, dlgcode)	\
		 buttonbase ## _GREATER:		\
	    case buttonbase ## _LESSER:			\
	    case buttonbase ## _EQUAL:			\
	      global_dlg_parameter = valbase ## _value;	\
	      if (!show_dialog_filter_gle(dlgcode))	\
		break;					\
	      valbase ## _value = global_filter_gle_dlg_value;	\
	      valbase &= FN_ENABLE;			\
	      if (cmd == buttonbase ## _GREATER)	\
		valbase |= FN_GT;			\
	      if (cmd == buttonbase ## _LESSER)		\
		valbase |= FN_LT;			\
	      if (cmd == buttonbase ## _EQUAL)		\
		valbase |= FN_EQ;			\
	      if (valbase & FN_ENABLE)			\
		refresh_filters = true

#define TOGGLE_3SETS_FILTER(button, tgt, other1, other2)	\
	    button:						\
	      if (!(global_filter_expansions & tgt)		\
		  || (global_filter_expansions & (other1 | other2)))	\
		global_filter_expansions ^= tgt;		\
	      if (global_filter_cardsets & FS_4TH_EDITION)	\
		refresh_filters = true

	    case RES_FILTERMENU_GOLD_ALL:
	    case RES_FILTERMENU_GOLD_MATCHINGALL:
	    case RES_FILTERMENU_GOLD_MATCHINGANY:
	      FilterColors old_fc;
	      old_fc = global_filter_colors;
	      global_filter_colors &= ~(FC_GOLD_ALL | FC_GOLD_ALLSELECTED | FC_GOLD_ANYSELECTED);
	      if (cmd == RES_FILTERMENU_GOLD_ALL)
		global_filter_colors |= FC_GOLD_ALL;
	      if (cmd == RES_FILTERMENU_GOLD_MATCHINGALL)
		global_filter_colors |= FC_GOLD_ALLSELECTED;
	      if (cmd == RES_FILTERMENU_GOLD_MATCHINGANY)
		global_filter_colors |= FC_GOLD_ANYSELECTED;
	      if (old_fc != global_filter_colors)
		refresh_filters = true;
	      break;

	    case TOGGLE_FILTER(RES_FILTERMENU_LAND_LANDANDMANA,	global_filter_cardtypes,	FT_LAND_LAND_AND_MANA, FT_LAND);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_LAND_LANDONLY,	global_filter_cardtypes,	FT_LAND_LAND_ONLY, FT_LAND);		break;
	    case TOGGLE_FILTER(RES_FILTERMENU_LAND_MANAONLY,	global_filter_cardtypes,	FT_LAND_MANA_ONLY, FT_LAND);		break;

	    case TOGGLE_FILTER(RES_FILTERMENU_ARTIFACT_CREATURES,	global_filter_cardtypes, FT_ARTIFACT_CREATURE,		FT_ARTIFACT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ARTIFACT_NONCREATURES,	global_filter_cardtypes, FT_ARTIFACT_NON_CREATURE,	FT_ARTIFACT);	break;

	    case TOGGLE_FILTER_ADD(RES_FILTERMENU_CREATURE_CREATURE,	global_filter_cardtypes, FT_CREATURE_CREATURE,	FT_CREATURE,
				   global_filter_cardtypes &= ~FT_CREATURE_LIST);	break;
	    case TOGGLE_FILTER_ADD(RES_FILTERMENU_CREATURE_TOKEN,	global_filter_cardtypes, FT_CREATURE_TOKEN,	FT_CREATURE,
				   global_filter_cardtypes &= ~FT_CREATURE_LIST);	break;
	    case TOGGLE_FILTER_ADD(RES_FILTERMENU_CREATURE_ARTIFACT,	global_filter_cardtypes, FT_CREATURE_ARTIFACT,	FT_CREATURE,
				   global_filter_cardtypes &= ~FT_CREATURE_LIST);	break;

	    case RES_FILTERMENU_CREATURE_LIST:
	      global_filter_subtype_dlg_mode = false;
	      if (!show_dialog_filter_subtype())
		break;

	      if (!(global_filter_cardtypes & FT_CREATURE_LIST))
		{
		  global_filter_cardtypes |= stored_creature_filters;
		  stored_creature_filters = FT_0;
		}
	      else if (!stored_creature_filters)
		{
		  stored_creature_filters = global_filter_cardtypes & (FT_CREATURE_CREATURE | FT_CREATURE_TOKEN | FT_CREATURE_ARTIFACT);
		  global_filter_cardtypes &= ~(FT_CREATURE_CREATURE | FT_CREATURE_TOKEN | FT_CREATURE_ARTIFACT);
		}

	      if (global_filter_cardtypes & FT_CREATURE)
		refresh_filters = true;
	      break;

	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_ENCHANTMENTS, global_filter_cardtypes, FT_ENCHANTMENT_ENCHANTMENTS, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_WORLD, global_filter_cardtypes, FT_ENCHANTMENT_WORLD, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_LAND, global_filter_cardtypes, FT_ENCHANTMENT_LAND, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_CREATURE, global_filter_cardtypes, FT_ENCHANTMENT_CREATURE, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_ARTIFACT, global_filter_cardtypes, FT_ENCHANTMENT_ARTIFACT, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_ENCHANT, global_filter_cardtypes, FT_ENCHANTMENT_ENCHANT, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_PERMANENT, global_filter_cardtypes, FT_ENCHANTMENT_PERMANENT, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_PLAYER, global_filter_cardtypes, FT_ENCHANTMENT_PLAYER, FT_ENCHANTMENT);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ENCHANTMENT_INSTANT, global_filter_cardtypes, FT_ENCHANTMENT_INSTANT, FT_ENCHANTMENT);	break;

	    case GLE_FILTER(RES_FILTERMENU_COST,	global_filter_casting_cost, 0);	break;

	    case RES_FILTERMENU_COST_X:
	      global_filter_casting_cost &= FN_ENABLE;
	      global_filter_casting_cost |= FN_CC_X;
	      if (global_filter_casting_cost & FN_ENABLE)
		refresh_filters = true;
	      break;

	    case GLE_FILTER(RES_FILTERMENU_POWER,	global_filter_power, 1);	break;
	    case GLE_FILTER(RES_FILTERMENU_TOUGHNESS,	global_filter_toughness, 2);	break;

	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_NATIVE, global_filter_abilities, FA_NATIVE, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_GIVES, global_filter_abilities, FA_GRANTS, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_FLYING, global_filter_abilities, FA_FLYING, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_FIRSTSTRIKE, global_filter_abilities, FA_FIRSTSTRIKE, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_TRAMPLE, global_filter_abilities, FA_TRAMPLE, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_REGENERATION, global_filter_abilities, FA_REGENERATION, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_BANDING, global_filter_abilities, FA_BANDING, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_COLORWARD, global_filter_abilities, FA_PROTECTION, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_LANDWALK, global_filter_abilities, FA_LANDWALK, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_POISON, global_filter_abilities, FA_INFECT, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_RAMPAGE, global_filter_abilities, FA_RAMPAGE, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_REACH, global_filter_abilities, FA_REACH, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_STONING, global_filter_abilities, FA_DEATHTOUCH, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_VIGILANCE, global_filter_abilities, FA_VIGILANCE, FA_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_ABILITY_HASTE, global_filter_abilities, FA_HASTE, FA_ENABLE);	break;

	    case TOGGLE_FILTER(RES_FILTERMENU_RARITY_COMMON, global_filter_rarity, FR_COMMON, FR_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_RARITY_UNCOMMON, global_filter_rarity, FR_UNCOMMON, FR_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_RARITY_RARE, global_filter_rarity, FR_RARE, FR_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_RARITY_RESTRICTED, global_filter_rarity, FR_RESTRICTED, FR_ENABLE);	break;
	    case TOGGLE_FILTER(RES_FILTERMENU_RARITY_BANNED, global_filter_rarity, FR_BANNED, FR_ENABLE);	break;

	    case RES_FILTERMENU_MAINMENUBUTTONS_ON:
	      global_filter_colors |= (FC_WHITE | FC_GREEN | FC_RED | FC_BLACK | FC_BLUE | FC_GOLD);
	      global_filter_cardsets |= (FS_4TH_EDITION | FS_ASTRAL | FS_ARABIAN_NIGHTS | FS_ANTIQUITIES | FS_THE_DARK | FS_LEGENDS | FS_OTHER);
	      global_filter_cardtypes |= (FT_LAND | FT_ARTIFACT | FT_CREATURE | FT_ENCHANTMENT | FT_INSTANT | FT_INTERRUPT | FT_SORCERY);
	      refresh_filters = true;
	      break;

	    case RES_FILTERMENU_MAINMENUBUTTONS_OFF:
	      global_filter_colors &= ~(FC_WHITE | FC_GREEN | FC_RED | FC_BLACK | FC_BLUE | FC_GOLD);
	      global_filter_cardsets = FS_0;
	      global_filter_cardtypes &= ~(FT_LAND | FT_ARTIFACT | FT_CREATURE | FT_ENCHANTMENT | FT_INSTANT | FT_INTERRUPT | FT_SORCERY);
	      refresh_filters = true;
	      break;

	    case TOGGLE_3SETS_FILTER(RES_FILTERMENU_FOURTH_UNLIMITED, FE_UNLIMITED, FE_REVISED, FE_4TH_EDITION);	break;
	    case TOGGLE_3SETS_FILTER(RES_FILTERMENU_FOURTH_REVISED, FE_REVISED, FE_UNLIMITED, FE_4TH_EDITION);	break;
	    case TOGGLE_3SETS_FILTER(RES_FILTERMENU_FOURTH_FOURTH, FE_4TH_EDITION, FE_REVISED, FE_UNLIMITED);	break;

	    case RES_FILTERMENU_EXPANSIONLIST:
	      global_filter_subtype_dlg_mode = true;
	      if (show_dialog_filter_subtype())
		refresh_filters = true;
	      break;

	    default:
	      return 0;
#undef TOGGLE_FILTER_ADD
#undef TOGGLE_FILTER
#undef GLE_FILTER
#undef TOGGLE_3SETS_FILTER
	  }
	RECT lRect2;
	GetClientRect(hwnd, &lRect2);
	//filterbuttons_setcoords(&lRect2, 1, &lRect1);
	draw_filter_buttons(chdc, &lRect2);
	InvalidateRect(hwnd, NULL, FALSE);
	if (refresh_filters)
	  {
	    SendMessage(global_horzlist_hwnd, 0x8007, 0, 0);
	    filter_cards_in_lists(global_listbox_hwnd, global_horzlist_hwnd);
	  }
	return 0;

      case 0x8402:
	GetClientRect(hwnd, &rect);

	DELETE_DC(chdc);
	hdc = GetDC(hwnd);
	gdi_flush(hdc);

	chdc = CreateCompatibleDC(hdc);
	gdi_flush(chdc);

	bmp = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
	SelectObject(chdc, bmp);
	ReleaseDC(hwnd, hdc);
	draw_filter_buttons(chdc, &rect);
	InvalidateRect(hwnd, NULL, TRUE);
	return 0;

      case 0x8465:
	p.x = GET_X_LPARAM(lparam);
	p.y = GET_Y_LPARAM(lparam);
	return get_filter_button_state(hwnd, p);

      default:
	return DefWindowProc(hwnd, msg, wparam, lparam);
    }
}
//]]]
