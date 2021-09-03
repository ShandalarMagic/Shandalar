#ifndef DRAWCARDLIB_H	// -*- tab-width:4; c-basic-offset:2; -*-
#define DRAWCARDLIB_H 1
// Drawcardlib: display card and mana cost graphics.

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define NOMINMAX
#include <windows.h>
#include <shlwapi.h>

// windows.h defines this to either LoadImageA or LoadImageW.
// We want the function in image.dll.
#undef LoadImage

#undef PACKED	// (unconditionally!) redefined within manalink.h
#include "manalink.h"

#include "gdiplus.h"

typedef enum
{
  // low byte: same as color_test_t
  COUNT_COLOR_BLACK		= 0x0002,
  COUNT_COLOR_BLUE		= 0x0004,
  COUNT_COLOR_GREEN		= 0x0008,
  COUNT_COLOR_RED		= 0x0010,
  COUNT_COLOR_WHITE		= 0x0020,

  COUNT_COLOR_COLOR_MASK= COUNT_COLOR_BLACK | COUNT_COLOR_BLUE | COUNT_COLOR_GREEN | COUNT_COLOR_RED | COUNT_COLOR_WHITE,

  // byte 2: presence of hybrid and/or mono-colored mana
  COUNT_COLOR_HYBRID	= 0x000100,	// set if there's any two-color hybrid in the mana cost
  COUNT_COLOR_MONO		= 0x000200,	// set if there's any single-color mana in the mana cost: this forces a gold frame
} CountColors;

typedef enum
{
  BIGCARDTITLE_FONT = 0,
  BIGCARDTITLE_FONT2,
  BIGCARDTITLE_FONT3,
  BIGCARDTITLE_FONT4,
  BIGCARDTITLE_FONT5,
  BIGCARDTITLE_FONT6,
  BIGCARDTITLE_FONT7,
  BIGCARDTITLE_FONT8,
  BIGCARDTITLE_FONT9,
  BIGCARDTYPE_FONT,
  BIGCARDTYPE_FONT2,
  BIGCARDTYPE_FONT3,
  BIGCARDTYPE_FONT4,
  BIGCARDTYPE_FONT5,
  BIGCARDTYPE_FONT6,
  BIGCARDTYPE_FONT7,
  BIGCARDTYPE_FONT8,
  BIGCARDTYPE_FONT9,
  BIGCARDPT_FONT,
  BIGCARDLOYALTYBASE_FONT,
  BIGCARDLOYALTYCOST_FONT,
  BIGCARDTEXT_FONT,
  SMALLCARDTITLE_FONT,
  SMALLCARDLOYALTYCURR_FONT,
  FLAVOR_FONT,

  MAX_FONTHANDLES = FLAVOR_FONT
} FontHandleNames;

typedef struct
{
#define LOYALTY_MOD_TXT_LEN	8
  char txt[LOYALTY_MOD_TXT_LEN];
  int top;
  int bottom;
} LoyaltyCost;

typedef enum
{
  PARENT_MANALINK,
  PARENT_SHANDALAR,
  PARENT_OTHER
} Parent;

typedef enum
{
  CARDBACK = 0,
  MANASYMBOLS,
  EXPANSION_SYMBOLS,
  WATERMARKS,
  CARDCOUNTERS,

#define FRAMEPARTS(frameset)																															\
  CARDBK_##frameset##_COLORLESS,	CARDBK_##frameset##_ARTIFACT,	CARDBK_##frameset##_LAND,	CARDBK_##frameset##_GOLD,	CARDBK_##frameset##_SPECIAL,\
  CARDBK_##frameset##_BLACK,		CARDBK_##frameset##_BLUE,		CARDBK_##frameset##_GREEN,	CARDBK_##frameset##_RED,	CARDBK_##frameset##_WHITE,	\
																																						\
  CARDOV_##frameset##_COLORLESS,		CARDOV_##frameset##_ARTIFACT,		CARDOV_##frameset##_COLORLESS_LAND,	CARDOV_##frameset##_LAND_BAR,			\
  CARDOV_##frameset##_GOLD,				CARDOV_##frameset##_GOLD_ARTIFACT,	CARDOV_##frameset##_GOLD_LAND,		CARDOV_##frameset##_GOLD_BAR,			\
  CARDOV_##frameset##_COLORLESS_NYX,	CARDOV_##frameset##_ARTIFACT_NYX,	CARDOV_##frameset##_LAND_NYX,		CARDOV_##frameset##_GOLD_NYX,			\
																																						\
  CARDOV_##frameset##_B,	CARDOV_##frameset##_BGOLD,	CARDOV_##frameset##_BLAND,	CARDOV_##frameset##_BARTIFACT,	CARDOV_##frameset##_BGOLD_ARTIFACT,	CARDOV_##frameset##_BNYX,	\
  CARDOV_##frameset##_U,	CARDOV_##frameset##_UGOLD,	CARDOV_##frameset##_ULAND,	CARDOV_##frameset##_UARTIFACT,	CARDOV_##frameset##_UGOLD_ARTIFACT, CARDOV_##frameset##_UNYX,	\
  CARDOV_##frameset##_G,	CARDOV_##frameset##_GGOLD,	CARDOV_##frameset##_GLAND,	CARDOV_##frameset##_GARTIFACT,	CARDOV_##frameset##_GGOLD_ARTIFACT, CARDOV_##frameset##_GNYX,	\
  CARDOV_##frameset##_R,	CARDOV_##frameset##_RGOLD,	CARDOV_##frameset##_RLAND,	CARDOV_##frameset##_RARTIFACT,	CARDOV_##frameset##_RGOLD_ARTIFACT, CARDOV_##frameset##_RNYX,	\
  CARDOV_##frameset##_W,	CARDOV_##frameset##_WGOLD,	CARDOV_##frameset##_WLAND,	CARDOV_##frameset##_WARTIFACT,	CARDOV_##frameset##_WGOLD_ARTIFACT, CARDOV_##frameset##_WNYX,	\
																																						\
  TOMBSTONE_##frameset,		POWERTOUGHNESS_##frameset,	TYPE_ICON_##frameset,		TRIGGERING_##frameset,												\
  LOYALTY_BASE_##frameset,	LOYALTY_PLUS_##frameset,	LOYALTY_ZERO_##frameset,	LOYALTY_MINUS_##frameset,											\
  CARDBK_##frameset##_MIN = CARDBK_##frameset##_COLORLESS,							CARDBK_##frameset##_MAX = LOYALTY_MINUS_##frameset

  FRAMEPARTS(MODERN),		CARDBK_MIN_FRAMEPART = CARDBK_MODERN_MIN, CARDBK_MAX_FRAMEPART_FRAMESET = CARDBK_MODERN_MAX,
#define NUM_FRAMEPARTS_PER_FRAMESET (CARDBK_MODERN_MAX - CARDBK_MODERN_MIN + 1)
  // No further entries in cfg_pic_names[] below - the frameparts for the other three framesets reuse the modern frameset's names, but in their own sections.
  FRAMEPARTS(CLASSIC),
  FRAMEPARTS(TIMESHIFT),
  FRAMEPARTS(FUTURESHIFT),
  FRAMEPARTS(PLANESWALKER),
  FRAMEPARTS(TOKEN),
  FRAMEPARTS(SCHEME),
  FRAMEPARTS(PLANE),
  FRAMEPARTS(VANGUARD),
  FRAMEPARTS(INTERNAL),
  FRAMEPARTS(MAGIC2015),
  FRAMEPARTS(MAGIC2015_HOLO),
  FRAMEPARTS(MAGIC2015_PLANESWALKER),	CARDBK_MAX_FRAMEPART_ALL = CARDBK_MAGIC2015_PLANESWALKER_MAX,

#define BASE_FRAMEPART(framepart)	(framepart < CARDBK_MIN_FRAMEPART ? framepart : ((framepart - CARDBK_MIN_FRAMEPART) % NUM_FRAMEPARTS_PER_FRAMESET) + CARDBK_MIN_FRAMEPART)
#define FRAMESET_OF_FRAMEPART(framepart)		(framepart < CARDBK_MIN_FRAMEPART ? 0 : (framepart - CARDBK_MIN_FRAMEPART) / NUM_FRAMEPARTS_PER_FRAMESET)
#define FRAMEPART_FROM_BASE(framepart, config)	(framepart < CARDBK_MIN_FRAMEPART ? framepart : config->cardbk_base + framepart - CARDBK_MIN_FRAMEPART)

#undef FRAMEPARTS

  FRAME_MAX_LOADED = CARDBK_MAX_FRAMEPART_ALL,

  // The rest are generated, not loaded, so don't have entries in cfg_pic_names[] below.

  // Assumptions:
  // Ordering within each frameset - and thus within each colorset, and for each color among monocolor frames, and for each color pair among two-color frames - must always be the same.
  // Framesets must also be the same size.
  // Nyx framesets always have the same offset from their base frameset.

  // A set of five variants by color.
#define X_COLOR(x)		x##BLACK,x##BLUE,x##GREEN,x##RED,x##WHITE

  // A set of ten variants by color pairs.  Note that e.g. FRAME_MODERN_SPELL_GOLD is not within [FRAME_MODERN_SPELL_GOLD_MIN...FRAME_MODERN_SPELL_GOLD_MAX].
#define X_COLOR_2(x)	x##WHITE_BLUE,x##BLUE_BLACK,x##BLACK_RED,x##RED_GREEN,x##GREEN_WHITE,		\
						x##WHITE_BLACK,x##BLACK_GREEN,x##GREEN_BLUE,x##BLUE_RED,x##RED_WHITE,x##MIN = x##WHITE_BLUE, x##MAX = x##RED_WHITE

  // A set of 22 variants by all combinations of color (1*none, 5*1-color, 5*1-color-gold, 10*2-color, 1*3+-color).
#define FRAMES_PER_COLORSET	22
#define COLORSET(x)		x##COLORLESS, X_COLOR(x), X_COLOR(x##GOLD_), X_COLOR_2(x##GOLD_), x##GOLD, x##MIN = x##COLORLESS, x##MAX = x##GOLD

  // A set of 98 variants, by spell (including an extra 10 Ravnica-style 2-color hybrid, instead of the usual gold hybrid), artifact, land, and artifact land.
#define FRAMESET(x)		COLORSET(x##SPELL_), COLORSET(x##ARTIFACT_), COLORSET(x##LAND_), COLORSET(x##ARTIFACT_LAND_),	\
						X_COLOR_2(x##SPELL_HYBRID_), x##MIN = x##SPELL_COLORLESS, x##MAX = x##SPELL_HYBRID_MAX

  FRAMESET(FRAME_MODERN_),			// Basic (modern) frames
  FRAMESET(FRAME_MODERN_NYX_),		// Modern frames with Theros-style Nyx for multitype enchantments - must be done at the color level, annoyingly enough
  FRAMESET(FRAME_CLASSIC_),			// Old-style frames.
  FRAMESET(FRAME_CLASSIC_NYX_),		// Old-style frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_TIMESHIFT_),		// Planar Chaos timeshifted frames.
  FRAMESET(FRAME_TIMESHIFT_NYX_),	// Planar Chaos timeshifted frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_FUTURESHIFT_),		// Future Sight futureshifted frames.
  FRAMESET(FRAME_FUTURESHIFT_NYX_),	// Future Sight futureshifted frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_PLANESWALKER_),	// Planeswalker frames.
  FRAMESET(FRAME_PLANESWALKER_NYX_),// Planeswalker frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_TOKEN_),			// Token frames.
  FRAMESET(FRAME_TOKEN_NYX_),		// Token frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_SCHEME_),			// Archenemy Scheme frames.
  FRAMESET(FRAME_SCHEME_NYX_),		// Archenemy Scheme frames, somehow with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_PLANE_),			// Planechase Plane frames.
  FRAMESET(FRAME_PLANE_NYX_),		// Planechase Plane frames, somehow with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_VANGUARD_),		// Vanguard avatar frames.
  FRAMESET(FRAME_VANGUARD_NYX_),	// Vanguard avatar frames, somehow with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_INTERNAL_),		// Internal effects, emblems, Rules Engine and Deadbox, etc.
  FRAMESET(FRAME_INTERNAL_NYX_),	// Internal effects etc., somehow with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_MAGIC2015_),		// M15 frames
  FRAMESET(FRAME_MAGIC2015_NYX_),	// M15 frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_MAGIC2015_HOLO_),		// Rare/mythic rare M15 frames
  FRAMESET(FRAME_MAGIC2015_HOLO_NYX_),	// Rare/mythic rare M15 frames with Theros-style Nyx for multitype enchantments
  FRAMESET(FRAME_MAGIC2015_PLANESWALKER_),		// Planeswalker M15 frames
  FRAMESET(FRAME_MAGIC2015_PLANESWALKER_NYX_),	// Planeswalker M15 frames with Theros-style Nyx for multitype enchantments

  FRAME_MIN = FRAME_MODERN_MIN,
  FRAME_MAX = FRAME_MAGIC2015_PLANESWALKER_NYX_MAX,
#define FRAMES_PER_FRAMESET	(FRAME_MODERN_NYX_MIN - FRAME_MODERN_MIN)

#define BASE_FRAME(frame)					(frame < FRAME_MIN ? frame : (((frame - FRAME_MIN) % FRAMES_PER_FRAMESET) + FRAME_MIN))
#define IS_NYX_FRAME(frame, base_frame)		((frame - base_frame) % (2 * FRAMES_PER_FRAMESET) >= FRAMES_PER_FRAMESET)

#undef X_COLOR
#undef X_COLOR_2
#undef COLORSET
#undef FRAMESET

  MAX_PICHANDLES_PLUS_1, PICHANDLE_INVALID = MAX_PICHANDLES_PLUS_1
} PicHandleNames;

typedef struct
{
  uint8_t default_expansion;
  uint8_t default_rarity;
  uint8_t watermark;					// index into watermarks[] array; 0xFF means no watermark
  uint8_t flags;
#define RARITYFLAG_TIMESHIFT	(1<<0)	// Has a Planar Chaos timeshift or Future Sight futureshift frame
#define RARITYFLAG_NYX			(1<<1)	// Is a multitype enchantment
  uint8_t rarity_in_expansion[0];
} Rarity;

typedef enum
{
  EXPCAT_NONE = 0,	// always lowest priority
  EXPCAT_CORE,
  EXPCAT_EXPERT,
  EXPCAT_PORTAL,
  EXPCAT_REPRINT,
  EXPCAT_CASUAL,
  EXPCAT_PROMO,
  EXPCAT_BOX,
  EXPCAT_DECKS,

  MAX_EXPCAT = EXPCAT_DECKS
} ExpansionCategory;

extern char base_dir[MAX_PATH + 1];
extern CRITICAL_SECTION* critical_section_for_display;
extern CRITICAL_SECTION* critical_section_for_drawing;
extern HDC* spare_hdc;
extern HFONT fonts[MAX_FONTHANDLES + 1];
extern ULONG_PTR gdiplus_token;
extern GpBitmap* gpics[MAX_PICHANDLES_PLUS_1];
extern HANDLE pics[MAX_PICHANDLES_PLUS_1];
extern HPALETTE palette;
extern Parent parent;

/**************** Configuration ****************/
typedef struct
{
  int r, g, b;	// The raw values read from config
  COLORREF colorref;
} ColorDef;
#define COLORDEF_AND_VIS(x) ColorDef x##_color; int x##_visible

typedef struct
{
  ColorDef color;
  ColorDef shadow_color;
  POINT shadow_offset;
  int shadow_visible;
} TextWithShadow;

typedef struct
{
  RECT smallcard_art, smallcard_expansion, smallcard_title, smallcard_loyalty_curr, smallcard_loyalty_curr_box;
  int smallcard_art_first;
  int smallcard_title_source_top, smallcard_title_source_height, smallcard_title_dest_height;
  int smallcard_frame_source_top, smallcard_frame_source_height, smallcard_frame_dest_top;
  int smallcard_triggering_visible;
  int smallcard_show_loyalty_curr, smallcard_loyalty_curr_suppress_counters, smallcard_loyalty_curr_alpha;
  int smallcard_expansion_visible, smallcard_expansion_no_early_core_sets, smallcard_expansion_no_early_rarity, smallcard_expansion_force_rarity;
  COLORDEF_AND_VIS(smallcard_outline);
  COLORDEF_AND_VIS(smallcard_artoutline);
  TextWithShadow smallcard_title_owned_txt;
  TextWithShadow smallcard_title_owned_can_activate_txt;
  TextWithShadow smallcard_title_owned_must_activate_txt;
  TextWithShadow smallcard_title_unowned_txt;
  TextWithShadow smallcard_title_unowned_can_activate_txt;
  TextWithShadow smallcard_title_unowned_must_activate_txt;
  COLORDEF_AND_VIS(smallcard_title_owned_background);
  COLORDEF_AND_VIS(smallcard_title_unowned_background);
  TextWithShadow smallcard_loyalty_txt;

  int fullcard_reread_config_on_redraw, fullcard_art_first, fullcard_watermark_outside_set;
  int fullcard_expand_top, fullcard_expand_height, fullcard_curve_mana_diameter, fullcard_title_center;
  int fullcard_show_loyalty_base, fullcard_show_loyalty_cost, fullcard_loyalty_cull, fullcard_loyalty_lowest;
  int fullcard_powertoughness_center, fullcard_powertoughness_box_visible, fullcard_extended_powertoughness;
  int fullcard_expansion_visible, fullcard_expansion_no_early_core_sets, fullcard_expansion_no_early_rarity, fullcard_expansion_force_rarity;
  int fullcard_tombstone_visible, fullcard_tombstone_title_offset, fullcard_type_icon_visible, fullcard_type_icon_suppressed_by_tombstone;
  RECT fullcard_frame, fullcard_title, fullcard_mana, fullcard_art, fullcard_type, fullcard_rulestext, fullcard_rulesbox, fullcard_watermark;
  RECT fullcard_powertoughness, fullcard_powertoughness_box;
  RECT fullcard_loyalty_base, fullcard_loyalty_base_box, fullcard_loyalty_cost, fullcard_loyalty_cost_box;
  RECT fullcard_expansion, fullcard_tombstone, fullcard_type_icon;
#define FULLCARD_CURVE_MANA_MAX	13
  POINT fullcard_curve_mana[FULLCARD_CURVE_MANA_MAX];
  POINT fullcard_rounding;
  ColorDef fullcard_border_color, fullcard_rulestext_color;
  COLORDEF_AND_VIS(fullcard_outline);
  COLORDEF_AND_VIS(fullcard_artoutline);
  TextWithShadow fullcard_title_txt;
  TextWithShadow fullcard_powertoughness_txt;
  TextWithShadow fullcard_loyalty_txt;
  TextWithShadow fullcard_type_txt;
  COLORDEF_AND_VIS(fullcard_title_on_light);
  COLORDEF_AND_VIS(fullcard_title_on_white_black);
  COLORDEF_AND_VIS(fullcard_rules_on_dark_land);
  COLORDEF_AND_VIS(fullcard_rules_on_white_black_land);

  int frames_percent_mixed_on_hybrid;
  int frames_no_early_hybrid_land;
  int frames_two_color_as_hybrid, frames_two_color_as_gold;
  int frames_translucent_colorless, frames_colorless_alpha, frames_force_colorless;
  GpImageAttributes* colorless_alpha_xform;
  GpImageAttributes* smallcard_loyalty_curr_alpha_xform;

  // Unclear if these have to outlive the call to GdipSetImageAttributesColorMatrix().  Doesn't hurt to keep them around, though.
  ColorMatrix colorless_alpha_xform_matrix, smallcard_loyalty_curr_alpha_xform_matrix;

  int frames_facedown_creature;
  int frames_artifact_land;

#undef COLORDEF_AND_VIS
#undef COLORDEF_AND_SHADOW_VIS_OFFSET

  char* configfile_name;

  // Constants
  const char* option_name;
  int cardbk_base;
  int frame_base;
} Config;
#define CFG_MODERN			0
#define CFG_CLASSIC			1
#define CFG_TIMESHIFT		2
#define CFG_FUTURESHIFT		3
#define CFG_PLANESWALKER	4
#define CFG_TOKEN			5
#define CFG_SCHEME			6
#define CFG_PLANE			7
#define CFG_VANGUARD		8
#define CFG_INTERNAL		9
#define CFG_MAGIC2015		10
#define CFG_MAGIC2015_HOLO	11
#define CFG_MAGIC2015_PLANESWALKER	12
#define MAX_FRAMESET		CFG_MAGIC2015_PLANESWALKER
#define CFG_BASE			MAX_FRAMESET + 1	// must be greater than MAX_FRAMESET
#define MAX_CFG				CFG_BASE

typedef enum
{
  FRAMES_TRANSLUCENT_COLORLESS_ALSO_NORMAL_FULLCARD_ART = 1,
  FRAMES_TRANSLUCENT_COLORLESS_NO_SMALLCARD_FRAME = 2,
  FRAMES_TRANSLUCENT_COLORLESS_REDRAW_SMALLCARD_ART = 4,
  FRAMES_TRANSLUCENT_COLORLESS_ENABLED = (1<<31)	// Used internally to simplify logic a bit.
} FramesTranslucentColorless;

typedef struct
{
  char* name;
  uint8_t* expansions;	// May be NULL; else an array indexed by EXPANSION_t
  uint8_t row;
  uint8_t col;
  uint8_t copied_expansions;	// if 1, expansions is a copy of another malloced pointer, and shouldn't be freed itself
} Watermark;
extern Watermark watermarks[256];	// Up to 255 may be defined, plus one as a sentinel

extern Config configs[MAX_CFG + 1];
extern Config* cfg;	// configuration for current frametype.  Only accurate after a call to select_frame().
extern int force_frameset, force_nyx;
extern int cardback_renderer;
extern int counters_num_rows, counters_num_columns, counters_renderer;
extern int hack_power, hack_toughness;	// An ugly hack to make sure Theros enchantment-creatures show their power/toughness, even though the base cards are non-creature versions
extern int watermark_size_x, watermark_size_y, watermarks_num_columns, watermarks_num_rows;
extern int suppress_next_counters;

typedef enum
{
  RENDERER_GDI = 0,
  RENDERER_GDIPLUS = 1,
} renderer_t;

/******************************** Extern functions ********************************/
#define DLLIMPORT __attribute__((dllimport))

/**************** In image.dll ****************/
DLLIMPORT HANDLE LoadImage(const char*, HPALETTE, int, int);

/**************** In Cardartlib.dll ****************/
DLLIMPORT void DestroyAllBigArts(void);
DLLIMPORT void DestroyAllSmallArts(void);
DLLIMPORT int DrawBigArt(HDC hdc, const RECT* rect, int id, int version);
DLLIMPORT int DrawSmallArt(HDC hdc, const RECT* rect, int id, int version);
DLLIMPORT int IsBigArtRightSize(int id, int version, int width, int height);
DLLIMPORT int IsSmallArtIn(int id, int version);
DLLIMPORT int LoadBigArt(int id, int version, int width, int height);
DLLIMPORT int LoadSmallArt(int id, int version, int width, int height);
DLLIMPORT int ReloadSmallArtIfWrongSize(int id, int version, int width, int heigt);

/**************** Public API ****************/
BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved);
int CalcDrawManaText(HDC hdc, const RECT* rect, const char* str);
int DestroyAllCardBackgrounds(void);
int DrawCardBack(HDC hdc, const RECT* dest_rect, /* An appalling hack to use this as an interface for other functions, but still safer than adding new imports. */ unsigned int mode, int arg1, int arg2, int arg3, int arg4);
int DrawFullCard(HDC hdc, const RECT* rect, const card_ptr_t* cp, int version, int big_art_style, int expand_text_box, const char* illus);
int DrawManaText(HDC hdc, const RECT* dest_rect, const char* str, int drawflag);
int DrawSmallCard(HDC hdc, const RECT* card_rect, const card_ptr_t* cp, int version, int mode, int player, int card);
int DrawSmallCardTitle(HDC hdc, const RECT* dest_rect, const char* card_title, int color, int controlled_by_owner);
const Rarity* get_rarity(int csvid);
ExpansionCategory expansion_category(int expansion);
void initialize_for_shandalar(card_data_t* real_cards_data, card_ptr_t* real_cards_ptr);
void draw_special_counters(HDC hdc, const RECT* rect, int player, int card);
void get_special_counter_rect(RECT* rect_dest, const RECT* rect_src, int num);
void draw_standard_counters(HDC hdc, const RECT* rect, int player, int card);

/**************** In config.c ****************/
void assign_default_rarities(void);
void create_alpha_xforms(Config* config);
void del_fonts_and_imgs(void);
int prepare_fonts_and_imgs(void);
int read_cfg(void);

/**************** In drawcardlib.c ****************/
void del_obj(HGDIOBJ* obj);
void draw_rect_outline(HDC hdc, const RECT* rect, COLORREF color);
void draw_text_with_shadow(HDC hdc, RECT* rect, const TextWithShadow* tws, COLORREF override_color, const char* str, int len);	// override_color, len == -1 for default
void draw_text_with_shadow_at_x(HDC hdc, RECT* rect, int x_pos, const TextWithShadow* tws, COLORREF override_color, const char* str, int len);	// override_color, len == -1 for default
void gdip_blt(HDC hdc, const RECT* dest_rect, PicHandleNames frame, int src_x, int src_y, int width, int height, GpImageAttributes* alpha_xform);
void gdip_blt_whole(HDC hdc, const RECT* dest_rect, PicHandleNames frame, GpImageAttributes* alpha_xform);
const card_data_t* get_card_data_from_csvid(uint32_t csvid);
card_instance_t* get_displayed_card_instance(int player, int card);
const char* get_pichandlename(PicHandleNames handle_name, int idx);
int is_planeswalker_by_cp(const card_ptr_t* cp);
void popup(const char* title, const char* fmt, ...);
PicHandleNames select_frame(const card_ptr_t* cp, const Rarity* r);
int find_rules_engine_csvid(void);
void make_gpic_from_pic(PicHandleNames picnum);

/**************** In drawfullcard.c ****************/
void blt_expansion_symbol(HDC hdc, const RECT* dest_rect, EXPANSION_t expansion, rarity_t rarity, int smallcard);

/**************** In drawmanacost.c ****************/
int calc_draw_mana_text(HDC hdc, const RECT* rect, const char* str, int rules_text_flag, int no_additional_space, LoyaltyCost* loyalty);
int draw_mana_text(HDC hdc, const RECT* dest_rect, const char* str, int drawflag, int rules_text_flag, int no_additional_space, LoyaltyCost* loyalty);
int convert_initial_mana_tag(const char** src_string);
void draw_mana_symbol(HDC hdc, char c, int x, int y, int wid, int hgt);
CountColors manacost_to_count_colors(const card_ptr_t* cp);
void init_mana_tags(void);

/**************** In get_displayed_card_instance.asm ****************/
card_instance_t* get_displayed_card_instance_manalink(int player, int card);

#endif
