// -*- tab-width:4; c-basic-offset:2; -*-
// Drawcardlib: display card and mana cost graphics.
// config.c: read Duel.dat, watermarks.csv, menus.txt, Rarity.dat

#include <ctype.h>

#include "drawcardlib.h"

#define FONTSTXT		"Fonts"
#define EXPANSIONSTXT	"Expansions"
#define CARDBACKTXT		"CardBack"
#define COUNTERSTXT		"Counters"
#define WATERMARKSTXT	"Watermarks"
#define FONT1NAME		"\\TT0530m_.ttf"
#define FONT3NAME		"\\TT0127m_.ttf"
#define FONT4NAME		"\\TT0085m_.ttf"
#define FONT5NAME		"\\TT0298m_.ttf"
#define FONT6NAME		"\\TT0299m_.ttf"
#define FONT7NAME		"\\TT0300m_.ttf"

const char* cfg_pic_names[CARDBK_MAX_FRAMEPART_FRAMESET + 1] =
{
  "CardBack",
  "ManaSymbols",
  "Expansion_Symbols",
  "Watermarks",
  "Cardcounters",
  "dfc",

  "CardBk_Colorless",	"CardBk_Artifact",	"CardBk_Land",	"CardBk_Gold",	"CardBk_Special",
  "CardBk_Black",		"CardBk_Blue",		"CardBk_Green",	"CardBk_Red",	"CardBk_White",

  "CardOv_Colorless",		"CardOv_Artifact",		"CardOv_ColorlessLand",	"CardOv_LandBar",
  "CardOv_Gold",			"CardOv_GoldArtifact",	"CardOv_GoldLand",		"CardOv_GoldBar",
  "CardOv_ColorlessNyx",	"CardOv_ArtifactNyx",	"CardOv_LandNyx",		"CardOv_GoldNyx",

  "CardOv_B",	"CardOv_BGold",	"CardOv_BLand",	"CardOv_BArtifact",	"CardOv_BGoldArtifact",	"CardOv_BNyx",
  "CardOv_U",	"CardOv_UGold",	"CardOv_ULand",	"CardOv_UArtifact",	"CardOv_UGoldArtifact",	"CardOv_UNyx",
  "CardOv_G",	"CardOv_GGold",	"CardOv_GLand",	"CardOv_GArtifact",	"CardOv_GGoldArtifact",	"CardOv_GNyx",
  "CardOv_R",	"CardOv_RGold",	"CardOv_RLand",	"CardOv_RArtifact",	"CardOv_RGoldArtifact",	"CardOv_RNyx",
  "CardOv_W",	"CardOv_WGold",	"CardOv_WLand",	"CardOv_WArtifact",	"CardOv_WGoldArtifact",	"CardOv_WNyx",

  "Tombstone",		"PowerToughness",	"TypeIcon",		"Triggering",
  "LoyaltyBase",	"LoyaltyPlus",		"LoyaltyZero",	"LoyaltyMinus"
};

#define FAKE_EXPANSIONS	8	// The first eight "expansions" aren't expansions, but formats.

ExpansionCategory fallback_expansion_categories[] =
{
  EXPCAT_NONE,		// unused
  EXPCAT_PROMO,		//EXPANSION_PROMO
  EXPCAT_CORE,		//EXPANSION_LIMITED_EDITION_ALPHA
  EXPCAT_CORE,		//EXPANSION_LIMITED_EDITION_BETA
  EXPCAT_CORE,		//EXPANSION_UNLIMITED_EDITION
  EXPCAT_EXPERT,	//EXPANSION_ARABIAN_NIGHTS
  EXPCAT_EXPERT,	//EXPANSION_ANTIQUITIES
  EXPCAT_CORE,		//EXPANSION_REVISED_EDITION
  EXPCAT_EXPERT,	//EXPANSION_LEGENDS
  EXPCAT_EXPERT,	//EXPANSION_THE_DARK
  EXPCAT_EXPERT,	//EXPANSION_FALLEN_EMPIRES
  EXPCAT_CORE,		//EXPANSION_FOURTH_EDITION
  EXPCAT_EXPERT,	//EXPANSION_ICE_AGE
  EXPCAT_REPRINT,	//EXPANSION_CHRONICLES
  EXPCAT_EXPERT,	//EXPANSION_HOMELANDS
  EXPCAT_EXPERT,	//EXPANSION_ALLIANCES
  EXPCAT_EXPERT,	//EXPANSION_MIRAGE
  EXPCAT_EXPERT,	//EXPANSION_VISIONS
  EXPCAT_CORE,		//EXPANSION_FIFTH_EDITION
  EXPCAT_PROMO,		//EXPANSION_ASTRAL
  EXPCAT_PORTAL,	//EXPANSION_PORTAL
  EXPCAT_EXPERT,	//EXPANSION_WEATHERLIGHT
  EXPCAT_EXPERT,	//EXPANSION_TEMPEST
  EXPCAT_EXPERT,	//EXPANSION_STRONGHOLD
  EXPCAT_EXPERT,	//EXPANSION_EXODUS
  EXPCAT_PORTAL,	//EXPANSION_PORTAL_SECOND_AGE
  EXPCAT_EXPERT,	//EXPANSION_URZAS_SAGA
  EXPCAT_EXPERT,	//EXPANSION_URZAS_LEGACY
  EXPCAT_CORE,		//EXPANSION_CLASSIC_SIXTH_EDITION
  EXPCAT_EXPERT,	//EXPANSION_URZAS_DESTINY
  EXPCAT_PORTAL,	//EXPANSION_PORTAL_THREE_KINGDOMS
  EXPCAT_PORTAL,	//EXPANSION_STARTER_1999
  EXPCAT_EXPERT,	//EXPANSION_MERCADIAN_MASQUES
  EXPCAT_BOX,		//EXPANSION_BATTLE_ROYALE_BOX_SET
  EXPCAT_EXPERT,	//EXPANSION_NEMESIS
  EXPCAT_PORTAL,	//EXPANSION_STARTER_2000
  EXPCAT_EXPERT,	//EXPANSION_PROPHECY
  EXPCAT_EXPERT,	//EXPANSION_INVASION
  EXPCAT_BOX,		//EXPANSION_BEATDOWN_BOX_SET
  EXPCAT_EXPERT,	//EXPANSION_PLANESHIFT
  EXPCAT_CORE,		//EXPANSION_SEVENTH_EDITION
  EXPCAT_EXPERT,	//EXPANSION_APOCALYPSE
  EXPCAT_EXPERT,	//EXPANSION_ODYSSEY
  EXPCAT_EXPERT,	//EXPANSION_TORMENT
  EXPCAT_EXPERT,	//EXPANSION_JUDGMENT
  EXPCAT_EXPERT,	//EXPANSION_ONSLAUGHT
  EXPCAT_EXPERT,	//EXPANSION_LEGIONS
  EXPCAT_EXPERT,	//EXPANSION_SCOURGE

  EXPCAT_CORE,		//EXPANSION_EIGHTH_EDITION
  EXPCAT_EXPERT,	//EXPANSION_MIRRODIN
  EXPCAT_EXPERT,	//EXPANSION_DARKSTEEL
  EXPCAT_EXPERT,	//EXPANSION_FIFTH_DAWN
  EXPCAT_EXPERT,	//EXPANSION_CHAMPIONS_OF_KAMIGAWA
  EXPCAT_EXPERT,	//EXPANSION_BETRAYERS_OF_KAMIGAWA
  EXPCAT_EXPERT,	//EXPANSION_SAVIORS_OF_KAMIGAWA
  EXPCAT_CORE,		//EXPANSION_NINTH_EDITION
  EXPCAT_EXPERT,	//EXPANSION_RAVNICA_CITY_OF_GUILDS
  EXPCAT_EXPERT,	//EXPANSION_GUILDPACT
  EXPCAT_EXPERT,	//EXPANSION_DISSENSION
  EXPCAT_EXPERT,	//EXPANSION_COLDSNAP
  EXPCAT_EXPERT,	//EXPANSION_TIME_SPIRAL
  EXPCAT_EXPERT,	//EXPANSION_PLANAR_CHAOS
  EXPCAT_EXPERT,	//EXPANSION_FUTURE_SIGHT
  EXPCAT_CORE,		//EXPANSION_TENTH_EDITION
  EXPCAT_REPRINT,	//EXPANSION_MASTERS_EDITION
  EXPCAT_EXPERT,	//EXPANSION_LORWYN
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_ELVES_VS_GOBLINS
  EXPCAT_EXPERT,	//EXPANSION_MORNINGTIDE
  EXPCAT_EXPERT,	//EXPANSION_SHADOWMOOR
  EXPCAT_EXPERT,	//EXPANSION_EVENTIDE
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_DRAGONS
  EXPCAT_REPRINT,	//EXPANSION_MASTERS_EDITION_II
  EXPCAT_EXPERT,	//EXPANSION_SHARDS_OF_ALARA
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_JACE_VS_CHANDRA
  EXPCAT_EXPERT,	//EXPANSION_CONFLUX
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_DIVINE_VS_DEMONIC
  EXPCAT_EXPERT,	//EXPANSION_ALARA_REBORN
  EXPCAT_CORE,		//EXPANSION_MAGIC_2010
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_EXILED
  EXPCAT_CASUAL,	//EXPANSION_PLANECHASE
  EXPCAT_REPRINT,	//EXPANSION_MASTERS_EDITION_III
  EXPCAT_EXPERT,	//EXPANSION_ZENDIKAR
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_GARRUK_VS_LILIANA
  EXPCAT_DECKS,		//EXPANSION_PREMIUM_DECK_SERIES_SLIVERS
  EXPCAT_EXPERT,	//EXPANSION_WORLDWAKE
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_PHYREXIA_VS_THE_COALITION
  EXPCAT_EXPERT,	//EXPANSION_RISE_OF_THE_ELDRAZI
  EXPCAT_CASUAL,	//EXPANSION_ARCHENEMY
  EXPCAT_CORE,		//EXPANSION_MAGIC_2011
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_RELICS
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_ELSPETH_VS_TEZZERET
  EXPCAT_EXPERT,	//EXPANSION_SCARS_OF_MIRRODIN
  EXPCAT_DECKS,		//EXPANSION_PREMIUM_DECK_SERIES_FIRE_AND_LIGHTNING
  EXPCAT_REPRINT,	//EXPANSION_MASTERS_EDITION_IV
  EXPCAT_EXPERT,	//EXPANSION_MIRRODIN_BESIEGED
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_KNIGHTS_VS_DRAGONS
  EXPCAT_EXPERT,	//EXPANSION_NEW_PHYREXIA
  EXPCAT_CASUAL,	//EXPANSION_MAGIC_THE_GATHERINGCOMMANDER
  EXPCAT_CORE,		//EXPANSION_MAGIC_2012
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_LEGENDS
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_AJANI_VS_NICOL_BOLAS
  EXPCAT_EXPERT,	//EXPANSION_INNISTRAD
  EXPCAT_DECKS,		//EXPANSION_PREMIUM_DECK_SERIES_GRAVEBORN
  EXPCAT_EXPERT,	//EXPANSION_DARK_ASCENSION
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_VENSER_VS_KOTH
  EXPCAT_EXPERT,	//EXPANSION_AVACYN_RESTORED
  EXPCAT_CASUAL,	//EXPANSION_PLANECHASE_2012
  EXPCAT_CORE,		//EXPANSION_MAGIC_2013
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_REALMS
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_IZZET_VS_GOLGARI
  EXPCAT_EXPERT,	//EXPANSION_RETURN_TO_RAVNICA
  EXPCAT_CASUAL,	//EXPANSION_COMMANDERS_ARSENAL
  EXPCAT_EXPERT,	//EXPANSION_GATECRASH
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_SORIN_VS_TIBALT
  EXPCAT_EXPERT,	//EXPANSION_DRAGONS_MAZE
  EXPCAT_REPRINT,	//EXPANSION_MODERN_MASTERS
  EXPCAT_CORE,		//EXPANSION_MAGIC_2014
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_TWENTY
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_HEROES_VS_MONSTERS
  EXPCAT_EXPERT,	//EXPANSION_THEROS
  EXPCAT_CASUAL,	//EXPANSION_COMMANDER_2013
  EXPCAT_EXPERT,	//EXPANSION_BORN_OF_THE_GODS
  EXPCAT_DECKS,		//EXPANSION_DUEL_DECKS_JACE_VS_VRASKA
  EXPCAT_EXPERT,	//EXPANSION_JOURNEY_INTO_NYX
  EXPCAT_DECKS,		//EXPANSION_MODERN_EVENT_DECK
  EXPCAT_CASUAL,	//EXPANSION_CONSPIRACY
  EXPCAT_REPRINT,	//EXPANSION_VINTAGE_MASTERS
  EXPCAT_CORE,		//EXPANSION_MAGIC_2015
  EXPCAT_BOX,		//EXPANSION_FROM_THE_VAULT_ANNIHILATION
  EXPCAT_DECKS		//EXPANSION_DUEL_DECKS_SPEED_VS_CUNNING
};

ExpansionCategory* expansion_categories = NULL;

HFONT fonts[MAX_FONTHANDLES + 1] = { 0 };

char base_dir[MAX_PATH + 1] = {0};
static char current_configfile_name[1000 + MAX_PATH + 1] = {0};

static Rarity** rarities;
static int raw_expansions = 0;
static int rarities_read = 0;
static int total_cards = 0;

Config configs[MAX_CFG + 1];
Config* cfg = &configs[CFG_BASE];
int hack_power = 0, hack_toughness = 0;

int force_frameset = -1, force_nyx = -1;
int cardback_renderer;
int counters_num_rows, counters_num_columns, counters_renderer;
static int expansions_latest_printing;
static int expansions_priority[MAX_EXPCAT + 1];

int watermark_size_x, watermark_size_y, watermarks_num_columns = 1, watermarks_num_rows = 1;
static char* frames_pic_names[FRAME_MAX_LOADED + 2] = { 0 };	// +2, so there's a NULL at the end
static int frames_dup_of[FRAME_MAX_LOADED + 1] = { [0 ... FRAME_MAX_LOADED] = -1 };	// gcc extension: sets all frames_dup_of[0] through [FRAME_MAX_LOADED] to -1

static void
create_1_alpha_xform(GpImageAttributes** xform, ColorMatrix* matrix, int alpha)
{
  if (*xform)
	GdipDisposeImageAttributes(*xform);

  if (alpha >= 255)
	*xform = NULL;
  else
	{
	  GdipCreateImageAttributes(xform);
	  int x, y;
	  for (x = 0; x < 5; ++x)
		for (y = 0; y < 5; ++y)
		  matrix->m[x][y] = (x == y) ? 1 : 0;
	  matrix->m[3][3] = MAX(alpha, 0) / 255.0;
	  GdipSetImageAttributesColorMatrix(*xform, ColorAdjustTypeBitmap, TRUE, matrix, NULL, ColorMatrixFlagsDefault);
	}
}

void
create_alpha_xforms(Config* config)
{
  if (gdiplus_token)	// otherwise, wait until the call from init_gdiplus()
	{
	  create_1_alpha_xform(&config->smallcard_loyalty_curr_alpha_xform, &config->smallcard_loyalty_curr_alpha_xform_matrix, config->smallcard_loyalty_curr_alpha);
	  create_1_alpha_xform(&config->colorless_alpha_xform, &config->colorless_alpha_xform_matrix, config->frames_colorless_alpha);
	}
}

const char*
get_pichandlename(PicHandleNames handle_name, int idx)
{
  if (idx < 0 || idx > 2)
	abort();

  static char buf[3][160];
  char* p = buf[idx];

  if (handle_name <= FRAME_MAX_LOADED)
	{
	  if (handle_name < CARDBK_MIN_FRAMEPART)
		return cfg_pic_names[handle_name];

	  Config* cf = &configs[FRAMESET_OF_FRAMEPART(handle_name)];

	  sprintf(p, "(%s) %s\\DuelArt\\%s: [Frames]%s", cf->option_name, base_dir, cf->configfile_name, cfg_pic_names[BASE_FRAMEPART(handle_name)]);
	}
  else
	sprintf(p, "#%d", (int)handle_name);

  return p;
}

#define CFG_BASE_PTR(Typ, config, member) ((Typ*)((char*)&configs[CFG_BASE] + ((char*)member - (char*)config)))

static int
get_cfg_int_raw(const char* section, const char* key, int deflt)
{
  return GetPrivateProfileInt(section, key, deflt, current_configfile_name);
}

static void
get_cfg_int(Config* config, int* val, const char* section, const char* key, int deflt)
{
  if (config != &configs[CFG_BASE])
	deflt = *CFG_BASE_PTR(int, config, val);
  *val = GetPrivateProfileInt(section, key, deflt, current_configfile_name);
}

static int
get_cfg_int2_raw(const char* section, const char* key1, const char* key2, int deflt)
{
  char buf[104];
  strcpy(buf, key1);
  strcat(buf, key2);
  return GetPrivateProfileInt(section, buf, deflt, current_configfile_name);
}

// Sets rectangle parameters from *Left, *Top, *Width, *Height, with defaults given as left/right/width/height.
static void
get_cfg_rect(Config* config, RECT* rect, const char* section, const char* key, int deflt_l, int deflt_t, int deflt_w, int deflt_h)
{
  RECT* r;
  if (config != &configs[CFG_BASE])
	{
	  r = CFG_BASE_PTR(RECT, config, rect);
	  deflt_l = r->left;
	  deflt_t = r->top;
	}
  else
	r = NULL;

  rect->left = get_cfg_int2_raw(section, key, "Left", deflt_l);
  rect->top = get_cfg_int2_raw(section, key, "Top", deflt_t);

  if (r)
	{
	  deflt_w = r->right - r->left;
	  deflt_h = r->bottom - r->top;
	}

  rect->right = rect->left + get_cfg_int2_raw(section, key, "Width", deflt_w);
  rect->bottom = rect->top + get_cfg_int2_raw(section, key, "Height", deflt_h);
}

// Sets point parameters from *X and *Y.
static void
get_cfg_point(Config* config, POINT* pt, const char* section, const char* key, int deflt_x, int deflt_y)
{
  if (config != &configs[CFG_BASE])
	{
	  POINT* p = CFG_BASE_PTR(POINT, config, pt);
	  deflt_x = p->x;
	  deflt_y = p->y;
	}
  pt->x = get_cfg_int2_raw(section, key, "X", deflt_x);
  pt->y = get_cfg_int2_raw(section, key, "Y", deflt_y);
}

// Sets color from *R, *G, and *B and returns 1.  If any are out of range, sets to defaults and returns 0.
static int
get_cfg_colordef(Config* config, ColorDef* color, const char* section, const char* key, int deflt_r, int deflt_g, int deflt_b)
{
  if (config != &configs[CFG_BASE])
	{
	  ColorDef* deflt = CFG_BASE_PTR(ColorDef, config, color);
	  deflt_r = deflt->r;
	  deflt_g = deflt->g;
	  deflt_b = deflt->b;
	}

  color->r = get_cfg_int2_raw(section, key, "R", deflt_r);
  color->g = get_cfg_int2_raw(section, key, "G", deflt_g);
  color->b = get_cfg_int2_raw(section, key, "B", deflt_b);
  if (color->r >= 0 && color->r <= 255 && color->g >= 0 && color->g <= 255 && color->b >= 0 && color->b <= 255)
	{
	  color->colorref = RGB(color->r, color->g, color->b);
	  return 1;
	}
  else
	{
	  /* If base config's raw read r/g/b were out of range, this will end up being garbage; but that's ok, since they won't ever be drawn since visibility will
	   * be set to 0. */
	  color->colorref = RGB((uint8_t)deflt_r, (uint8_t)deflt_g, (uint8_t)deflt_b);
	  return 0;
	}
}

static void
get_cfg_str_raw(char* dest, int sz_of_dest, const char* section, const char* key, const char* deflt_fmt, ...)
{
  GetPrivateProfileString(section, key, "\1", dest, sz_of_dest, current_configfile_name);
  if (!strcmp(dest, "\1"))
	{
	  va_list args;
	  va_start(args, deflt_fmt);
	  vsnprintf(dest, sz_of_dest, deflt_fmt, args);
	  va_end(args);
	}
  dest[sz_of_dest - 1] = 0;
}

static void
get_cfg_str_dup(char** dest, const char* section, const char* key, const char* deflt_fmt, ...)
{
  char buf[1000];
  GetPrivateProfileString(section, key, "\1", buf, 1000, current_configfile_name);
  if (!strcmp(buf, "\1"))
	{
	  va_list args;
	  va_start(args, deflt_fmt);
	  vsnprintf(buf, 1000, deflt_fmt, args);
	  va_end(args);
	}
  buf[999] = 0;
  if (*dest)
	free(*dest);
  *dest = strdup(buf);
}

static LOGFONT*
cfg_font(const char* font_name, int italic, int* decrement)
{
  static LOGFONT fnt;
  memset(&fnt, 0, sizeof(LOGFONT));

  fnt.lfWidth = 4 * get_cfg_int2_raw(FONTSTXT, "width", font_name, 0);
  fnt.lfHeight = 4 * get_cfg_int2_raw(FONTSTXT, "size", font_name, 20);
  fnt.lfWeight = get_cfg_int2_raw(FONTSTXT, "bold", font_name, 0) ? 600 : 400;

  if (decrement)
	*decrement = get_cfg_int2_raw(FONTSTXT, "decrement", font_name, 1);

  if (italic)
	fnt.lfItalic = 1;

  char buf[104];
  strcpy(buf, "font");
  strcat(buf, font_name);
  GetPrivateProfileString(FONTSTXT, buf, "MS Sans Serif", fnt.lfFaceName, LF_FACESIZE, current_configfile_name);
  return &fnt;
}

static void
rem_font(const char* fontname)
{
  char buf[2 * MAX_PATH + 1];

  strcpy(buf, base_dir);
  strcat(buf, fontname);
  RemoveFontResource(buf);
}

void
del_fonts_and_imgs(void)
{
  int i;
  for (i = 0; i < MAX_PICHANDLES_PLUS_1; ++i)
	if (frames_dup_of[i] >= 0)
	  pics[i] = NULL;	// just set null if this image is a shallow copy of another
	else
	  del_obj(&pics[i]);

  for (i = 0; i < FRAME_MAX_LOADED + 2; ++i)
	if (frames_pic_names[i])
	  {
		free(frames_pic_names[i]);
		frames_pic_names[i] = NULL;
	  }

  for (i = 0; i <= MAX_FONTHANDLES; ++i)
	del_obj((HGDIOBJ*)(&fonts[i]));

  rem_font(FONT1NAME);
  rem_font(FONT3NAME);
  rem_font(FONT4NAME);
  rem_font(FONT5NAME);
  rem_font(FONT6NAME);
  rem_font(FONT7NAME);
}

int
prepare_fonts_and_imgs(void)
{
  char buf[3 * MAX_PATH + 1];

#define ADD_FONT(fontname)	do { strcpy(buf, base_dir); strcat(buf, fontname); AddFontResource(buf); } while (0)
  ADD_FONT(FONT1NAME);
  ADD_FONT(FONT3NAME);
  ADD_FONT(FONT4NAME);
  ADD_FONT(FONT5NAME);
  ADD_FONT(FONT6NAME);
  ADD_FONT(FONT7NAME);
#undef ADD_FONT

  int i, any_failures = 0;
  for (i = 0; frames_pic_names[i]; ++i)
	{
	  switch (BASE_FRAMEPART(i))
		{
		  case POWERTOUGHNESS_MODERN:
			if (!configs[FRAMESET_OF_FRAMEPART(i)].fullcard_powertoughness_box_visible)
			  continue;
			break;

		  case TOMBSTONE_MODERN:
			if (!configs[FRAMESET_OF_FRAMEPART(i)].fullcard_tombstone_visible)
			  continue;
			break;

		  case TYPE_ICON_MODERN:
			if (!configs[FRAMESET_OF_FRAMEPART(i)].fullcard_type_icon_visible)
			  continue;
			break;

		  case TRIGGERING_MODERN:
			if (!configs[FRAMESET_OF_FRAMEPART(i)].smallcard_triggering_visible)
			  continue;
			break;

		  case EXPANSION_SYMBOLS:
			{	// Don't read unless at least one set has visible expansions, either on smallcard or fullcard
			  int fs;
			  for (fs = 0; fs <= MAX_FRAMESET; ++fs)
				if (configs[fs].smallcard_expansion_visible || configs[fs].fullcard_expansion_visible)
				  goto found;
			  //otherwise
			  continue;
			found:
			  break;
			}

		  case DFC_SYMBOLS:
			{	// Don't read unless at least one set has visible dfc symbols
			  int fs;
			  for (fs = 0; fs <= MAX_FRAMESET; ++fs)
				if (configs[fs].fullcard_dfc_visible || configs[fs].smallcard_dfc_visible)
				  goto found2;
			  //otherwise
			  continue;
			found2:
			  break;
			}
		}

	  int j;
	  for (j = 0; j < i; ++j)
		// If this has the same name as a previously loaded image, just copy the pointer.
		// Strcasecmp on the (previously canonicalized) filenames is sufficient for our purposes; opening the files and calling GetFileInformationByHandle() it the robust way, but it's much slower.
		if (pics[j] && !strcasecmp(frames_pic_names[i], frames_pic_names[j]))
		  {
			pics[i] = pics[j];
			frames_dup_of[i] = j;	// ...and mark it so we don't try to free it later
			goto outer_continue;
		  }

	  snprintf(buf, 3 * MAX_PATH + 1, "%s\\CARDART\\%s",
			   base_dir,
			   frames_pic_names[i]);
	  if (!(pics[i] = LoadImage(buf, palette, 0, 0)))
		{
		  popup("drawcardlib.dll", "Could not load %s=\n\"%s\"", get_pichandlename(i, 0), buf);
		  free(frames_pic_names[i]);
		  frames_pic_names[i] = strdup("\1");	// so anything else pointing to it gets a popup instead of being identified as a duplicate
		  any_failures = 1;
		}

	  if (i == WATERMARKS)
		{
		  BITMAP bmp;
		  GetObject(pics[WATERMARKS], sizeof(BITMAP), &bmp);
		  watermark_size_x = bmp.bmWidth / watermarks_num_columns;
		  watermark_size_y = bmp.bmHeight / watermarks_num_rows;
		}

	outer_continue:;
	}

  if (any_failures)
	return 0;

  int decrement;

  LOGFONT* logfont = cfg_font("BigCardTitle", 0, &decrement);
  if ((unsigned int)logfont->lfWidth < 12)
	logfont->lfWidth = 24;
  for (i = BIGCARDTITLE_FONT; i <= BIGCARDTITLE_FONT9; ++i)
	{
	  fonts[i] = CreateFontIndirect(logfont);
	  logfont->lfWidth -= decrement;
	  if (logfont->lfWidth <= 0)
		logfont->lfWidth = 1;
	}

  logfont = cfg_font("BigCardSubtitle", 0, &decrement);
  if ((unsigned int)logfont->lfWidth < 12)
	logfont->lfWidth = 20;
  for (i = BIGCARDTYPE_FONT; i <= BIGCARDTYPE_FONT9; ++i)
	{
	  fonts[i] = CreateFontIndirect(logfont);
	  logfont->lfWidth -= decrement;
	  if (logfont->lfWidth <= 0)
		logfont->lfWidth = 1;
	}

  logfont = cfg_font("BigCardPT", 0, NULL);
  fonts[BIGCARDPT_FONT] = CreateFontIndirect(logfont);

  logfont = cfg_font("BigCardLoyaltyBase", 0, NULL);
  fonts[BIGCARDLOYALTYBASE_FONT] = CreateFontIndirect(logfont);

  logfont = cfg_font("BigCardLoyaltyCost", 0, NULL);
  fonts[BIGCARDLOYALTYCOST_FONT] = CreateFontIndirect(logfont);

  logfont = cfg_font("BigCardVanguard", 0, &decrement);
  if ((unsigned int)logfont->lfWidth < 12)
	logfont->lfWidth = 5;
  for (i = BIGCARDVANGUARD_FONT; i <= BIGCARDVANGUARD_FONT3; ++i)
	{
	  fonts[i] = CreateFontIndirect(logfont);
	  logfont->lfWidth -= decrement;
	  if (logfont->lfWidth <= 0)
		logfont->lfWidth = 1;
	}

  logfont = cfg_font("BigCardText", 0, NULL);
  fonts[BIGCARDTEXT_FONT] = CreateFontIndirect(logfont);

  logfont = cfg_font("BigCardText2", 1, NULL);
  fonts[FLAVOR_FONT] = CreateFontIndirect(logfont);

  logfont = cfg_font("SmallCardTitle", 0, &decrement);
  if ((unsigned int)logfont->lfWidth < 12)
	logfont->lfWidth = 32;
  for (i = SMALLCARDTITLE_FONT; i <= SMALLCARDTITLE_FONT9; ++i)
	{
	  fonts[i] = CreateFontIndirect(logfont);
	  logfont->lfWidth -= decrement;
	  if (logfont->lfWidth <= 0)
		logfont->lfWidth = 1;
	}

  logfont = cfg_font("SmallCardLoyaltyCurr", 0, NULL);
  fonts[SMALLCARDLOYALTYCURR_FONT] = CreateFontIndirect(logfont);

  if (!fonts[BIGCARDTITLE_FONT]
	  || !fonts[BIGCARDTITLE_FONT2]
	  || !fonts[BIGCARDTITLE_FONT3]
	  || !fonts[BIGCARDTITLE_FONT4]
	  || !fonts[BIGCARDTITLE_FONT5]
	  || !fonts[BIGCARDTITLE_FONT6]
	  || !fonts[BIGCARDTITLE_FONT7]
	  || !fonts[BIGCARDTITLE_FONT8]
	  || !fonts[BIGCARDTITLE_FONT9]
	  || !fonts[BIGCARDTYPE_FONT]
	  || !fonts[BIGCARDTYPE_FONT2]
	  || !fonts[BIGCARDTYPE_FONT3]
	  || !fonts[BIGCARDTYPE_FONT4]
	  || !fonts[BIGCARDTYPE_FONT5]
	  || !fonts[BIGCARDTYPE_FONT6]
	  || !fonts[BIGCARDTYPE_FONT7]
	  || !fonts[BIGCARDTYPE_FONT8]
	  || !fonts[BIGCARDTYPE_FONT9]
	  || !fonts[BIGCARDPT_FONT]
	  || !fonts[BIGCARDLOYALTYBASE_FONT]
	  || !fonts[BIGCARDLOYALTYCOST_FONT]
	  || !fonts[BIGCARDVANGUARD_FONT]
	  || !fonts[BIGCARDVANGUARD_FONT2]
	  || !fonts[BIGCARDVANGUARD_FONT3]
	  || !fonts[BIGCARDTEXT_FONT]
	  || !fonts[FLAVOR_FONT]
	  || !fonts[SMALLCARDTITLE_FONT]
	  || !fonts[SMALLCARDTITLE_FONT2]
	  || !fonts[SMALLCARDTITLE_FONT3]
	  || !fonts[SMALLCARDTITLE_FONT4]
	  || !fonts[SMALLCARDTITLE_FONT5]
	  || !fonts[SMALLCARDTITLE_FONT6]
	  || !fonts[SMALLCARDTITLE_FONT7]
	  || !fonts[SMALLCARDTITLE_FONT8]
	  || !fonts[SMALLCARDTITLE_FONT9]
	  || !fonts[SMALLCARDLOYALTYCURR_FONT])
	{
	  popup("drawcardlib.dll", "Could not load fonts");
	  return 0;
	}
  else
	return 1;
}

char* base_frame_base_dir = NULL;
char* base_frame_pic_names[NUM_FRAMEPARTS_PER_FRAMESET] = {0};

static void
read_cfg_file(Config* config)
{
  sprintf(current_configfile_name, "%s\\DuelArt\\%s", base_dir, config->configfile_name);

  const char* section = "SmallCard";
  get_cfg_rect(config, &config->smallcard_art, section, "Art", 79, 161, 860, 786);

  get_cfg_int(config, &config->smallcard_art_first, section, "ArtFirst", 0);

  get_cfg_int(config, &config->smallcard_title_source_top, section, "TitleSourceTop", 4);
  get_cfg_int(config, &config->smallcard_title_source_height, section, "TitleSourceHeight", 15);
  get_cfg_int(config, &config->smallcard_title_dest_height, section, "TitleDestHeight", 200);
  get_cfg_int(config, &config->smallcard_title_center, section, "TitleCenter", 0);

  get_cfg_int(config, &config->smallcard_frame_source_top, section, "FrameSourceTop", 14);
  get_cfg_int(config, &config->smallcard_frame_source_height, section, "FrameSourceHeight", 175);
  get_cfg_int(config, &config->smallcard_frame_dest_top, section, "FrameDestTop", 144);

  config->smallcard_outline_visible = get_cfg_colordef(config, &config->smallcard_outline_color, section, "OutlineColor", 147,147,147);
  config->smallcard_artoutline_visible = get_cfg_colordef(config, &config->smallcard_artoutline_color, section, "ArtOutlineColor", -1,-1,-1);

  get_cfg_rect(config, &config->smallcard_title, section, "Title", 8, -8, 800, 1120);

#define TXT_AND_SHADOW(v, txt, cr,cg,cb, sr,sg,sb, x,y)																	\
  get_cfg_colordef(config, &config->v.color, section, txt "Color", cr,cg,cb);											\
  config->v.shadow_visible = get_cfg_colordef(config, &config->v.shadow_color, section, txt "ShadowColor", sr,sg,sb);	\
  get_cfg_point(config, &config->v.shadow_offset, section, txt "ShadowOffset", x, y)

#define TXT_AND_SHADOW_ALL_DEFAULT(v, txt, deflt)																							\
  TXT_AND_SHADOW(v, txt, configs[CFG_BASE].deflt.color.r, configs[CFG_BASE].deflt.color.g, configs[CFG_BASE].deflt.color.b,					\
				 configs[CFG_BASE].deflt.shadow_color.r, configs[CFG_BASE].deflt.shadow_color.g, configs[CFG_BASE].deflt.shadow_color.b,	\
				 configs[CFG_BASE].deflt.shadow_offset.x, configs[CFG_BASE].deflt.shadow_offset.y)

#define TXT_AND_DEFAULT_SHADOW(v, txt, cr, cg, cb, deflt)																					\
  TXT_AND_SHADOW(v, txt, cr,cg,cb,																											\
				 configs[CFG_BASE].deflt.shadow_color.r, configs[CFG_BASE].deflt.shadow_color.g, configs[CFG_BASE].deflt.shadow_color.b,	\
				 configs[CFG_BASE].deflt.shadow_offset.x, configs[CFG_BASE].deflt.shadow_offset.y)

  TXT_AND_SHADOW(smallcard_title_owned_txt, "TitleOwned", 255,255,255, 47,47,47, 12,12);
  config->smallcard_title_owned_on_light_visible =
	get_cfg_colordef(config, &config->smallcard_title_owned_on_light_color, section,
					 "TitleOwnedOnLightColor", -1,-1,-1);
  config->smallcard_title_owned_on_white_black_visible =
	get_cfg_colordef(config, &config->smallcard_title_owned_on_white_black_color, section,
					 "TitleOwnedOnWhiteBlackColor", -1,-1,-1);
  config->smallcard_title_owned_on_colorless_visible =
	get_cfg_colordef(config, &config->smallcard_title_owned_on_colorless_color, section,
					 "TitleOwnedOnColorlessColor", -1,-1,-1);
  TXT_AND_DEFAULT_SHADOW(smallcard_title_owned_can_activate_txt, "TitleOwnedCanActivate", 241,217,39, smallcard_title_owned_txt);
  TXT_AND_DEFAULT_SHADOW(smallcard_title_owned_must_activate_txt, "TitleOwnedMustActivate", 255,148,17, smallcard_title_owned_txt);
  config->smallcard_title_owned_background_visible = get_cfg_colordef(config, &config->smallcard_title_owned_background_color, section,
																	  "TitleOwnedBackgroundColor", -1,-1,-1);

  TXT_AND_SHADOW_ALL_DEFAULT(smallcard_title_unowned_txt, "TitleUnowned", smallcard_title_owned_txt);
  config->smallcard_title_unowned_on_light_visible =
	get_cfg_colordef(config, &config->smallcard_title_unowned_on_light_color, section,
					 "TitleUnownedOnLightColor", -1,-1,-1);
  config->smallcard_title_unowned_on_white_black_visible =
	get_cfg_colordef(config, &config->smallcard_title_unowned_on_white_black_color, section,
					 "TitleUnownedOnWhiteBlackColor", -1,-1,-1);
  config->smallcard_title_unowned_on_colorless_visible =
	get_cfg_colordef(config, &config->smallcard_title_unowned_on_colorless_color, section,
					 "TitleUnownedOnColorlessColor", -1,-1,-1);
  TXT_AND_SHADOW_ALL_DEFAULT(smallcard_title_unowned_can_activate_txt, "TitleUnownedCanActivate", smallcard_title_owned_can_activate_txt);
  TXT_AND_SHADOW_ALL_DEFAULT(smallcard_title_unowned_must_activate_txt, "TitleUnownedMustActivate", smallcard_title_owned_must_activate_txt);
  config->smallcard_title_unowned_background_visible = get_cfg_colordef(config, &config->smallcard_title_unowned_background_color, section,
																		"TitleUnownedBackgroundColor", 147,147,147);

  get_cfg_int(config, &config->smallcard_triggering_visible, section, "TriggeringVisible", 0);

  get_cfg_int(config, &config->smallcard_show_loyalty_curr, section, "ShowLoyaltyCurr", 0);
  get_cfg_int(config, &config->smallcard_loyalty_curr_suppress_counters, section, "LoyaltyCurrSuppressCounters", 1);
  get_cfg_int(config, &config->smallcard_loyalty_curr_alpha, section, "LoyaltyCurrAlpha", 255);
  get_cfg_rect(config, &config->smallcard_loyalty_curr, section, "LoyaltyCurr", 630, 960, 160, 150);
  get_cfg_rect(config, &config->smallcard_loyalty_curr_box, section, "LoyaltyCurrBox", 587, 920, 220, 250);
  TXT_AND_SHADOW_ALL_DEFAULT(smallcard_loyalty_txt, "Loyalty", smallcard_title_owned_txt);

  get_cfg_int(config, &config->smallcard_dfc_visible, section, "DfcVisible", 0);
  get_cfg_rect(config, &config->smallcard_dfc, section, "Dfc", 12,15,80,112);

  get_cfg_rect(config, &config->smallcard_expansion, section, "Expansion", 587, 9, 208, 147);
  get_cfg_int(config, &config->smallcard_expansion_visible, section, "ExpansionVisible", 0);
  get_cfg_int(config, &config->smallcard_expansion_no_early_core_sets, section, "ExpansionNoEarlyCoreSets", 0);
  get_cfg_int(config, &config->smallcard_expansion_no_early_rarity, section, "ExpansionNoEarlyRarity", 0);
  get_cfg_int(config, &config->smallcard_expansion_force_rarity, section, "ExpansionForceRarity", 0);


  section = "FullCard";

  get_cfg_int(config, &config->fullcard_reread_config_on_redraw, section, "RereadConfigOnRedraw", 0);
  get_cfg_int(config, &config->fullcard_art_first, section, "ArtFirst", configs[CFG_BASE].smallcard_art_first);
  get_cfg_int(config, &config->fullcard_expand_top, section, "ExpandTop", 6050);
  get_cfg_int(config, &config->fullcard_expand_height, section, "ExpandHeight", 3220);

  get_cfg_rect(config, &config->fullcard_frame, section, "Frame", 24, 24, 752, 1152);
  get_cfg_rect(config, &config->fullcard_title, section, "Title", 40, 20, 712, 64);
  get_cfg_rect(config, &config->fullcard_mana, section, "Mana", 48, 32, 712, 48);
  get_cfg_rect(config, &config->fullcard_art, section, "Art", 80, 96, 640, 560);
  get_cfg_rect(config, &config->fullcard_type, section, "Type", 48, 660, 590, 60);
  get_cfg_rect(config, &config->fullcard_rulestext, section, "Rulestext", 92, 736, 616, 336);
  get_cfg_rect(config, &config->fullcard_rulesbox, section, "Rulesbox", 64, 720, 672, 364);
  get_cfg_rect(config, &config->fullcard_watermark, section, "Watermark", 230, 736, 340, 340);
  get_cfg_rect(config, &config->fullcard_powertoughness, section, "Powertoughness", 44, 1084, 704, 72);
  get_cfg_rect(config, &config->fullcard_powertoughness_box, section, "PowertoughnessBox", 553, 1064, 201, 105);
  get_cfg_rect(config, &config->fullcard_loyalty_base, section, "LoyaltyBase", 660, 1092, 110, 72);
  get_cfg_rect(config, &config->fullcard_loyalty_base_box, section, "LoyaltyBaseBox", 640, 1075, 140, 105);
  get_cfg_rect(config, &config->fullcard_loyalty_cost, section, "LoyaltyCost", 30, -12, 80, 50);
  get_cfg_rect(config, &config->fullcard_loyalty_cost_box, section, "LoyaltyCostBox", 20, -27, 101, 88);
  get_cfg_rect(config, &config->fullcard_vanguard_hand, section, "VanguardHand", 100, 900, 100, 100);
  get_cfg_rect(config, &config->fullcard_vanguard_life, section, "VanguardLife", 600, 900, 100, 100);
  get_cfg_rect(config, &config->fullcard_expansion, section, "Expansion", 638, 666, 104, 53);

  get_cfg_int(config, &config->fullcard_curve_mana_diameter, section, "CurveManaDiameter", 0);
  int i;
  for (i = 0; i < FULLCARD_CURVE_MANA_MAX; ++i)
	{
	  char curve_mana[16];
	  sprintf(curve_mana, "CurveMana%d", i + 1);
	  get_cfg_point(config, &config->fullcard_curve_mana[i], section, curve_mana, 400, 100);
	}

  get_cfg_int(config, &config->fullcard_title_center, section, "TitleCenter", 0);
  get_cfg_int(config, &config->fullcard_type_center, section, "TypeCenter", 0);
  get_cfg_int(config, &config->fullcard_show_loyalty_base, section, "ShowLoyaltyBase", 0);
  get_cfg_int(config, &config->fullcard_show_loyalty_cost, section, "ShowLoyaltyCost", 0);
  get_cfg_int(config, &config->fullcard_loyalty_cull, section, "LoyaltyCull", 50);
  get_cfg_int(config, &config->fullcard_loyalty_lowest, section, "LoyaltyLowest", -30);
  get_cfg_int(config, &config->fullcard_watermark_outside_set, section, "WatermarkOutsideSet", 0);
  get_cfg_int(config, &config->fullcard_powertoughness_center, section, "PowertoughnessCenter", 0);
  get_cfg_int(config, &config->fullcard_powertoughness_box_visible, section, "PowertoughnessBoxVisible", 0);
  get_cfg_int(config, &config->fullcard_extended_powertoughness, section, "ExtendedPowertoughness", 0);
  get_cfg_int(config, &config->fullcard_expansion_visible, section, "ExpansionVisible", 1);
  get_cfg_int(config, &config->fullcard_expansion_no_early_core_sets, section, "ExpansionNoEarlyCoreSets", configs[CFG_BASE].smallcard_expansion_no_early_core_sets);
  get_cfg_int(config, &config->fullcard_expansion_no_early_rarity, section, "ExpansionNoEarlyRarity", configs[CFG_BASE].smallcard_expansion_no_early_rarity);
  get_cfg_int(config, &config->fullcard_expansion_force_rarity, section, "ExpansionForceRarity", configs[CFG_BASE].smallcard_expansion_force_rarity);

  get_cfg_int(config, &config->fullcard_tombstone_visible, section, "TombstoneVisible", 0);
  get_cfg_int(config, &config->fullcard_tombstone_title_offset, section, "TombstoneTitleOffset", 28);
  get_cfg_rect(config, &config->fullcard_tombstone, section, "Tombstone", 32,32,32,48);

  get_cfg_int(config, &config->fullcard_dfc_visible, section, "DfcVisible", 0);
  get_cfg_rect(config, &config->fullcard_dfc, section, "Dfc", 37,47,68,71);

  get_cfg_int(config, &config->fullcard_type_icon_visible, section, "TypeIconVisible", 0);
  get_cfg_int(config, &config->fullcard_type_icon_suppressed_by_tombstone, section, "TypeIconSuppressedByTombstone", 0);
  get_cfg_rect(config, &config->fullcard_type_icon, section, "TypeIcon", 35,35,50,55);

  get_cfg_point(config, &config->fullcard_rounding, section, "Rounding", 12, 12);

  get_cfg_colordef(config, &config->fullcard_border_color, section, "BorderColor", 28,28,28);
  config->fullcard_outline_visible = get_cfg_colordef(config, &config->fullcard_outline_color, section, "OutlineColor", -1,-1,-1);
  config->fullcard_artoutline_visible = get_cfg_colordef(config, &config->fullcard_artoutline_color, section, "ArtOutlineColor", -1,-1,-1);

  TXT_AND_SHADOW(fullcard_title_txt, "Title", 255,255,255, 47,47,47, 4,4);
  TXT_AND_SHADOW_ALL_DEFAULT(fullcard_powertoughness_txt, "Powertoughness", fullcard_title_txt);
  TXT_AND_SHADOW_ALL_DEFAULT(fullcard_loyalty_txt, "Loyalty", fullcard_title_txt);
  TXT_AND_SHADOW_ALL_DEFAULT(fullcard_type_txt, "Type", fullcard_title_txt);

  get_cfg_colordef(config, &config->fullcard_rulestext_color, section, "RulestextColor", 47,47,47);

  config->fullcard_title_on_light_visible =
	get_cfg_colordef(config, &config->fullcard_title_on_light_color, section,
					 "TitleOnLightColor", -1,-1,-1);
  config->fullcard_title_on_white_black_visible =
	get_cfg_colordef(config, &config->fullcard_title_on_white_black_color, section,
					 "TitleOnWhiteBlackColor", -1,-1,-1);
  config->fullcard_title_on_colorless_visible =
	get_cfg_colordef(config, &config->fullcard_title_on_colorless_color, section,
					 "TitleOnColorlessColor", -1,-1,-1);
  config->fullcard_rules_on_dark_land_visible =
	get_cfg_colordef(config, &config->fullcard_rules_on_dark_land_color, section,
					 "RulesOnDarkLandColor", -1,-1,-1);
  config->fullcard_rules_on_white_black_land_visible =
	get_cfg_colordef(config, &config->fullcard_rules_on_white_black_land_color, section,
					 "RulesOnWhiteBlackLandColor", -1,-1,-1);
  config->fullcard_vanguard_visible =
	get_cfg_colordef(config, &config->fullcard_vanguard_color, section,
					 "VanguardColor", -1,-1,-1);


  section = "Frames";

  get_cfg_int(config, &config->frames_percent_mixed_on_hybrid, section, "PercentMixedOnHybrid", 30);
  if (config->frames_percent_mixed_on_hybrid < 0)
	config->frames_percent_mixed_on_hybrid = -1;

  get_cfg_int(config, &config->frames_no_early_hybrid_land, section, "NoEarlyHybridLand", 0);
  get_cfg_int(config, &config->frames_two_color_as_hybrid, section, "TwoColorAsHybrid", 0);
  get_cfg_int(config, &config->frames_two_color_as_gold, section, "TwoColorAsGold", 0);
  get_cfg_int(config, &config->frames_translucent_colorless, section, "TranslucentColorless", -1);
  get_cfg_int(config, &config->frames_colorless_alpha, section, "ColorlessAlpha", 255);
  get_cfg_int(config, &config->frames_force_colorless, section, "ForceColorless", 0);
  get_cfg_int(config, &config->frames_facedown_creature, section, "FaceDownCreature", -1);
  get_cfg_int(config, &config->frames_artifact_land, section, "ArtifactLand", 0);

  create_alpha_xforms(config);


  char frame_base_dir[MAX_PATH + 1];
  get_cfg_str_raw(frame_base_dir, MAX_PATH, section, "FrameBaseDir", config == &configs[CFG_BASE] ? "." : base_frame_base_dir);
  if (config == &configs[CFG_BASE])
	base_frame_base_dir = strdup(frame_base_dir);

  char default_image[MAX_PATH + 1];
  get_cfg_str_raw(default_image, MAX_PATH, section, "DefaultImage", "");

  char** frame_pic_name;
  if (config == &configs[CFG_BASE])
	frame_pic_name = &base_frame_pic_names[0];
  else
	frame_pic_name = &frames_pic_names[config->cardbk_base];

  for (i = 0; i < NUM_FRAMEPARTS_PER_FRAMESET; ++i)
	{
	  char filename[MAX_PATH + 1];
	  get_cfg_str_raw(filename, MAX_PATH, section, cfg_pic_names[i + CARDBK_MIN_FRAMEPART],
					  "%s%s",
					  *default_image ? default_image : config == &configs[CFG_BASE] ? cfg_pic_names[i + CARDBK_MIN_FRAMEPART] : base_frame_pic_names[i],
					  *default_image ? "" : config == &configs[CFG_BASE] ? ".pic" : "");

	  if (frame_pic_name[i])
		free(frame_pic_name[i]);

	  if (config == &configs[CFG_BASE])
		frame_pic_name[i] = strdup(filename);
	  else
		{	// Add directory and canonicalize.
		  char path[2 * MAX_PATH + 2];
		  snprintf(path, 2 * MAX_PATH + 1, "%s\\%s", frame_base_dir, filename);

		  // replace [/\\]+ with \\ - it's obscene that PathCanonicalize() can't deal with multiple backslashes, or any forward slashes
		  char* p, *q;
		  for (p = q = path; *q; ++p, ++q)
			{
			  if (*q == '/' || *q == '\\')
				{
				  while (*(q + 1) == '/' || *(q + 1) == '\\')
					++q;
				  *p = '\\';
				}
			  else
				*p = *q;
			}

		  // simplify . and ..
		  char canonicalized[MAX_PATH + 1];
		  PathCanonicalize(canonicalized, path);

		  frame_pic_name[i] = strdup(canonicalized);
		}
	}

#undef TXT_AND_SHADOW
#undef TXT_AND_DEFAULT_SHADOW
#undef TXT_AND_SHADOW_ALL_DEFAULT
}

static void
read_watermark_section(int first_time)
{
  watermarks_num_rows = get_cfg_int_raw(WATERMARKSTXT, "NumRows", 4);
  watermarks_num_rows = MAX(watermarks_num_rows, 1);
  watermarks_num_rows = MIN(watermarks_num_rows, 255);

  watermarks_num_columns = get_cfg_int_raw(WATERMARKSTXT, "NumColumns", 5);
  watermarks_num_columns = MAX(watermarks_num_columns, 1);
  watermarks_num_columns = MIN(watermarks_num_columns, 255);

  if (!first_time)
	{
	  BITMAP bmp;
	  GetObject(pics[WATERMARKS], sizeof(BITMAP), &bmp);
	  watermark_size_x = bmp.bmWidth / watermarks_num_columns;
	  watermark_size_y = bmp.bmHeight / watermarks_num_rows;
	}

  int i;
  for (i = 0; i < 255; ++i)
	{
	  if (!first_time && watermarks[i].name)
		free(watermarks[i].name);
	  watermarks[i].name = NULL;

	  if (!first_time && watermarks[i].expansions && !watermarks[i].copied_expansions)
		free(watermarks[i].expansions);
	  watermarks[i].expansions = NULL;

	  watermarks[i].row = 0;
	  watermarks[i].col = 0;
	  watermarks[i].copied_expansions = 0;
	}

  /* We need raw_expansions to be set, but it won't be yet.  And we can't just call read_rarities, since it calls read_watermarks_csv(), which needs this
   * complete first.  So just read raw_expansions from rarity.dat for now. */

#define BAIL(...) do { popup("drawcardlib.dll", __VA_ARGS__); return; } while (0)
  FILE* rarity_dat;
  if (!(rarity_dat = fopen("rarity.dat", "rb")))
	BAIL("Could not open rarity.dat: %s", strerror(errno));

  int ignore;
  if (fread(&ignore, 4, 1, rarity_dat) != 1)
	BAIL("Could not read number of cards: %s", strerror(errno));

  if (fread(&raw_expansions, 4, 1, rarity_dat) != 1)
	BAIL("Could not read number of expansions: %s", strerror(errno));

  if (raw_expansions < 9)
	BAIL("number of expansions only %d, expected at least 9", raw_expansions);
#undef BAIL

  fclose(rarity_dat);

  int idx = 0;
  char key[16];
  char expansions[1000];
  for (i = 1; i < 255; ++i)
	{
	  sprintf(key, "Row%d", i);
	  int row = get_cfg_int_raw(WATERMARKSTXT, key, -1);
	  if (row < 1 || row > watermarks_num_rows)
		continue;

	  sprintf(key, "Col%d", i);
	  int col = get_cfg_int_raw(WATERMARKSTXT, key, -1);
	  if (col < 1 || col > watermarks_num_columns)
		continue;

	  sprintf(key, "Name%d", i);
	  char *name, *p;
	  name = NULL;
	  get_cfg_str_dup(&name, WATERMARKSTXT, key, "");
	  for (p = name; *p; ++p)
		if (!isascii(*p) || !isalnum(*p))
		  {
			*name = 0;
			break;
		  }
		else
		  *p = tolower(*p);

	  if (!isalpha(*name))
		{
		  free(name);
		  continue;
		}

	  watermarks[idx].name = name;
	  watermarks[idx].row = row - 1;
	  watermarks[idx].col = col - 1;

	  sprintf(key, "Expansions%d", i);
	  get_cfg_str_raw(expansions, 1000, WATERMARKSTXT, key, "");
	  if (isascii(*expansions) && isalnum(*expansions))
		{
		  if (isalpha(*expansions))
			{
			  int j;
			  for (j = 1; j < i; ++j)
				if (!strcasecmp(expansions, watermarks[j].name))
				  {
					watermarks[idx].copied_expansions = 1;
					watermarks[idx].expansions = watermarks[j].expansions;
					break;
				  }
			}
		  else
			{
			  watermarks[idx].expansions = calloc(raw_expansions, 1);	// note that raw_expansions is 8 greater than the maximum permissible EXPANSION_t
			  p = expansions;
			  while (isascii(*p) && isdigit(*p))
				{
				  int exp = atoi(p);
				  if (exp < raw_expansions)
					watermarks[idx].expansions[exp] = 1;

				  while (isascii(*p) && isdigit(*p))	// skip number
					++p;
				  while (*p && isascii(*p) && !isdigit(*p))	// skip trailing whitespace
					++p;
				}
			}
		}

	  ++idx;
	}
}

int
read_cfg(void)
{
  int rval = 1;

  if (!*base_dir)
	{
	  GetModuleFileName(NULL, base_dir, MAX_PATH + 1);
	  *(strrchr(base_dir, '\\')) = 0;
	}

  sprintf(current_configfile_name, "%s\\DuelArt\\Duel.dat", base_dir);

  configs[CFG_BASE].option_name = "";
  configs[CFG_BASE].cardbk_base = 0;	// Handled specially.
  configs[CFG_BASE].frame_base = 0;		// Handled specially.

  configs[CFG_MODERN].option_name = "Modern";
  configs[CFG_MODERN].cardbk_base = CARDBK_MODERN_MIN;
  configs[CFG_MODERN].frame_base = FRAME_MODERN_MIN;

  configs[CFG_MODERN_DFC_FRONT].option_name = "ModernDFCfront";
  configs[CFG_MODERN_DFC_FRONT].cardbk_base = CARDBK_MODERN_DFC_FRONT_MIN;
  configs[CFG_MODERN_DFC_FRONT].frame_base = FRAME_MODERN_DFC_FRONT_MIN;

  configs[CFG_MODERN_DFC_BACK].option_name = "ModernDFCback";
  configs[CFG_MODERN_DFC_BACK].cardbk_base = CARDBK_MODERN_DFC_BACK_MIN;
  configs[CFG_MODERN_DFC_BACK].frame_base = FRAME_MODERN_DFC_BACK_MIN;

  configs[CFG_CLASSIC].option_name = "Classic";
  configs[CFG_CLASSIC].cardbk_base = CARDBK_CLASSIC_MIN;
  configs[CFG_CLASSIC].frame_base = FRAME_CLASSIC_MIN;

  configs[CFG_TIMESHIFT].option_name = "Timeshift";
  configs[CFG_TIMESHIFT].cardbk_base = CARDBK_TIMESHIFT_MIN;
  configs[CFG_TIMESHIFT].frame_base = FRAME_TIMESHIFT_MIN;

  configs[CFG_FUTURESHIFT].option_name = "Futureshift";
  configs[CFG_FUTURESHIFT].cardbk_base = CARDBK_FUTURESHIFT_MIN;
  configs[CFG_FUTURESHIFT].frame_base = FRAME_FUTURESHIFT_MIN;

  configs[CFG_PLANESWALKER].option_name = "Planeswalker";
  configs[CFG_PLANESWALKER].cardbk_base = CARDBK_PLANESWALKER_MIN;
  configs[CFG_PLANESWALKER].frame_base = FRAME_PLANESWALKER_MIN;

  configs[CFG_PLANESWALKER_DFC_FRONT].option_name = "PlaneswalkerDFCfront";
  configs[CFG_PLANESWALKER_DFC_FRONT].cardbk_base = CARDBK_PLANESWALKER_DFC_FRONT_MIN;
  configs[CFG_PLANESWALKER_DFC_FRONT].frame_base = FRAME_PLANESWALKER_DFC_FRONT_MIN;

  configs[CFG_PLANESWALKER_DFC_BACK].option_name = "PlaneswalkerDFCback";
  configs[CFG_PLANESWALKER_DFC_BACK].cardbk_base = CARDBK_PLANESWALKER_DFC_BACK_MIN;
  configs[CFG_PLANESWALKER_DFC_BACK].frame_base = FRAME_PLANESWALKER_DFC_BACK_MIN;

  configs[CFG_TOKEN].option_name = "Token";
  configs[CFG_TOKEN].cardbk_base = CARDBK_TOKEN_MIN;
  configs[CFG_TOKEN].frame_base = FRAME_TOKEN_MIN;

  configs[CFG_SCHEME].option_name = "Scheme";
  configs[CFG_SCHEME].cardbk_base = CARDBK_SCHEME_MIN;
  configs[CFG_SCHEME].frame_base = FRAME_SCHEME_MIN;

  configs[CFG_PLANE].option_name = "Plane";
  configs[CFG_PLANE].cardbk_base = CARDBK_PLANE_MIN;
  configs[CFG_PLANE].frame_base = FRAME_PLANE_MIN;

  configs[CFG_VANGUARD].option_name = "Vanguard";
  configs[CFG_VANGUARD].cardbk_base = CARDBK_VANGUARD_MIN;
  configs[CFG_VANGUARD].frame_base = FRAME_VANGUARD_MIN;

  configs[CFG_CONSPIRACY].option_name = "Conspiracy";
  configs[CFG_CONSPIRACY].cardbk_base = CARDBK_CONSPIRACY_MIN;
  configs[CFG_CONSPIRACY].frame_base = FRAME_CONSPIRACY_MIN;

  configs[CFG_INTERNAL].option_name = "Internal";
  configs[CFG_INTERNAL].cardbk_base = CARDBK_INTERNAL_MIN;
  configs[CFG_INTERNAL].frame_base = FRAME_INTERNAL_MIN;

  configs[CFG_MAGIC2015].option_name = "Magic2015";
  configs[CFG_MAGIC2015].cardbk_base = CARDBK_MAGIC2015_MIN;
  configs[CFG_MAGIC2015].frame_base = FRAME_MAGIC2015_MIN;

  configs[CFG_MAGIC2015_DFC_FRONT].option_name = "Magic2015DFCfront";
  configs[CFG_MAGIC2015_DFC_FRONT].cardbk_base = CARDBK_MAGIC2015_DFC_FRONT_MIN;
  configs[CFG_MAGIC2015_DFC_FRONT].frame_base = FRAME_MAGIC2015_DFC_FRONT_MIN;

  configs[CFG_MAGIC2015_DFC_BACK].option_name = "Magic2015DFCback";
  configs[CFG_MAGIC2015_DFC_BACK].cardbk_base = CARDBK_MAGIC2015_DFC_BACK_MIN;
  configs[CFG_MAGIC2015_DFC_BACK].frame_base = FRAME_MAGIC2015_DFC_BACK_MIN;

  configs[CFG_MAGIC2015_HOLO].option_name = "Magic2015holo";
  configs[CFG_MAGIC2015_HOLO].cardbk_base = CARDBK_MAGIC2015_HOLO_MIN;
  configs[CFG_MAGIC2015_HOLO].frame_base = FRAME_MAGIC2015_HOLO_MIN;

  configs[CFG_MAGIC2015_HOLO_DFC_FRONT].option_name = "Magic2015holoDFCfront";
  configs[CFG_MAGIC2015_HOLO_DFC_FRONT].cardbk_base = CARDBK_MAGIC2015_HOLO_DFC_FRONT_MIN;
  configs[CFG_MAGIC2015_HOLO_DFC_FRONT].frame_base = FRAME_MAGIC2015_HOLO_DFC_FRONT_MIN;

  configs[CFG_MAGIC2015_HOLO_DFC_BACK].option_name = "Magic2015holoDFCback";
  configs[CFG_MAGIC2015_HOLO_DFC_BACK].cardbk_base = CARDBK_MAGIC2015_HOLO_DFC_BACK_MIN;
  configs[CFG_MAGIC2015_HOLO_DFC_BACK].frame_base = FRAME_MAGIC2015_HOLO_DFC_BACK_MIN;

  configs[CFG_MAGIC2015_PLANESWALKER].option_name = "Magic2015planeswalker";
  configs[CFG_MAGIC2015_PLANESWALKER].cardbk_base = CARDBK_MAGIC2015_PLANESWALKER_MIN;
  configs[CFG_MAGIC2015_PLANESWALKER].frame_base = FRAME_MAGIC2015_PLANESWALKER_MIN;

  configs[CFG_MAGIC2015_PLANESWALKER_DFC_FRONT].option_name = "Magic2015planeswalkerDFCfront";
  configs[CFG_MAGIC2015_PLANESWALKER_DFC_FRONT].cardbk_base = CARDBK_MAGIC2015_PLANESWALKER_DFC_FRONT_MIN;
  configs[CFG_MAGIC2015_PLANESWALKER_DFC_FRONT].frame_base = FRAME_MAGIC2015_PLANESWALKER_DFC_FRONT_MIN;

  configs[CFG_MAGIC2015_PLANESWALKER_DFC_BACK].option_name = "Magic2015planeswalkerDFCback";
  configs[CFG_MAGIC2015_PLANESWALKER_DFC_BACK].cardbk_base = CARDBK_MAGIC2015_PLANESWALKER_DFC_BACK_MIN;
  configs[CFG_MAGIC2015_PLANESWALKER_DFC_BACK].frame_base = FRAME_MAGIC2015_PLANESWALKER_DFC_BACK_MIN;

  int i;
  static int first_time = 1;
  if (first_time)
	{
	  configs[CFG_BASE].configfile_name = strdup("Duel.dat");
	  for (i = 0; i <= MAX_FRAMESET; ++i)
		configs[i].configfile_name = NULL;
	}
  for (i = 0; i <= MAX_FRAMESET; ++i)
	{
	  const char* default_dat;
	  switch (i)
		{
		  case CFG_MAGIC2015:
		  case CFG_MAGIC2015_HOLO:
			default_dat = "Modern";
			break;

		  case CFG_MAGIC2015_DFC_FRONT:
		  case CFG_MAGIC2015_HOLO_DFC_FRONT:
			default_dat = "ModernDFCfront";
			break;

		  case CFG_MAGIC2015_DFC_BACK:
		  case CFG_MAGIC2015_HOLO_DFC_BACK:
			default_dat = "ModernDFCback";
			break;

		  case CFG_MAGIC2015_PLANESWALKER:
			default_dat = "Planeswalker";
			break;

		  case CFG_MAGIC2015_PLANESWALKER_DFC_FRONT:
			default_dat = "PlaneswalkerDFCfront";
			break;

		  case CFG_MAGIC2015_PLANESWALKER_DFC_BACK:
			default_dat = "PlaneswalkerDFCback";
			break;

		  default:
			default_dat = configs[i].option_name;
			break;
		}
	  get_cfg_str_dup(&configs[i].configfile_name, EXPANSIONSTXT, configs[i].option_name, "%s.dat", default_dat);
	}

  expansions_latest_printing = get_cfg_int_raw(EXPANSIONSTXT, "LatestPrinting", 0);
  expansions_priority[EXPCAT_NONE] = INT_MIN;
  expansions_priority[EXPCAT_CORE] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityCore", 10);
  expansions_priority[EXPCAT_EXPERT] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityExpert", 10);
  expansions_priority[EXPCAT_PORTAL] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityPortal", 7);
  expansions_priority[EXPCAT_REPRINT] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityReprint", 6);
  expansions_priority[EXPCAT_CASUAL] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityCasual", 4);
  expansions_priority[EXPCAT_PROMO] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityPromo", 3);
  expansions_priority[EXPCAT_BOX] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityBox", 2);
  expansions_priority[EXPCAT_DECKS] = get_cfg_int_raw(EXPANSIONSTXT, "PriorityDecks", 1);

  force_frameset = -1;
  force_nyx = -1;

  char buf[MAX_PATH + 1];

  get_cfg_str_raw(buf, MAX_PATH, EXPANSIONSTXT, "ForceFrame", "");
  if (*buf)
	{
	  char* p = buf;
	  if (!strncasecmp(p, "nyx", 3))
		{
		  p += 3;
		  force_nyx = 1;
		}

	  if (*p)
		{
		  if (force_nyx == -1)
			force_nyx = 0;

		  for (i = 0; i <= MAX_FRAMESET; ++i)
			if (!strcasecmp(p, configs[i].option_name))
			  {
				force_frameset = i;
				break;
			  }
		  if (force_frameset == -1)
			{
			  char buf2[MAX_PATH + 1000];
			  p = buf2 + sprintf(buf2, "Unknown value for [Expansions]ForceFrame: \"%s\"\nValid values are:", buf);
			  for (i = 0; i <= MAX_FRAMESET; ++i)
				p += sprintf(p, "\n%s\nNyx%s", configs[i].option_name, configs[i].option_name);

			  popup("drawcardlib.dll", buf2);

			  rval = 0;
			}
		}
	}

  cardback_renderer = get_cfg_int_raw(CARDBACKTXT, "Renderer", 1);

  counters_num_rows = get_cfg_int_raw(COUNTERSTXT, "NumRows", 12);
  counters_num_columns = get_cfg_int_raw(COUNTERSTXT, "NumColumns", 12);
  counters_renderer = get_cfg_int_raw(COUNTERSTXT, "Renderer", 0);
  get_cfg_str_raw(buf, MAX_PATH, COUNTERSTXT, "Cardcounters", "");
  if (*buf)
	frames_pic_names[CARDCOUNTERS] = strdup(buf);

  read_watermark_section(first_time);

  for (i = 0; i < CARDBK_MIN_FRAMEPART; ++i)
	if (!frames_pic_names[i])
	  {
		snprintf(buf, MAX_PATH, "%s.pic", cfg_pic_names[i]);
		frames_pic_names[i] = strdup(buf);
	  }

  read_cfg_file(&configs[CFG_BASE]);	// Must be read first
  for (i = 0; i <= MAX_FRAMESET; ++i)
	read_cfg_file(&configs[i]);

  free(base_frame_base_dir);
  base_frame_base_dir = NULL;

  for (i = 0; i < NUM_FRAMEPARTS_PER_FRAMESET; ++i)
	{
	  free(base_frame_pic_names[i]);
	  base_frame_pic_names[i] = NULL;
	}

  sprintf(current_configfile_name, "%s\\DuelArt\\Duel.dat", base_dir);	// so fonts are read from Duel.dat

  first_time = 0;
  return rval;
}

static void
read_watermarks_csv(void)
{
  // Lookup table for a partial radix sort on watermark names
  uint8_t lookup[26][256] = {{0}};

  // Populate it
  {
	uint8_t lookup_idx[26] = {0};
	int i;
	for (i = 0; watermarks[i].name; ++i)
	  {
		assert(isascii(watermarks[i].name[0]) && islower(watermarks[i].name[0]));
		int radix = watermarks[i].name[0] - 'a';
		assert(lookup_idx[radix] < 255);
		lookup[radix][lookup_idx[radix]] = i+1;
		++lookup_idx[radix];
	  }
  }

#define BAIL(...) do { popup("drawcardlib.dll", __VA_ARGS__); return; } while (0)
  FILE* watermarks_csv;
  if (!(watermarks_csv = fopen("watermarks.csv", "r")))
	BAIL("Could not open watermarks.csv: %s", strerror(errno));

  char line[1024], *wmark, *p;
  int id = 0, linenum = 0, idx;
  while (fgets(line, 1024, watermarks_csv))
	{
	  ++linenum;
	  if (linenum == 1 && !strncasecmp(line, "id;", 3))
		continue;

	  char* semi = strchr(line, ';');
	  if (!semi)
		BAIL("Could not find first ; in line #%d of watermarks.csv [%s]", linenum, line);

	  id = atoi(line);
	  if (id == 0 && !strncmp(line, "0;", 2))
		BAIL("Id not a number in line #%d of watermarks.csv [%s]", linenum, line);
	  if (id < 0)
		break;

	  if (!(rarities && id >= 0 && id < total_cards))
		BAIL("Unknown id %d in line #%d of watermarks.csv [%s]", id, linenum, line);

	  Rarity* r = rarities[id];

	  ++semi;
	  wmark = semi;
	  semi = strchr(semi, ';');
	  if (semi)
		*semi = '\0';

	  for (p = wmark; *p; ++p)
		if (isascii(*p) && isupper(*p))
		  *p = tolower(*p);

	  int radix = wmark[0];
	  if (isascii(radix) && isalpha(radix))
		{
		  if (radix == 't' && !strcmp(wmark, "timeshift"))
			{
			  r->flags |= RARITYFLAG_TIMESHIFT;
			  continue;
			}

		  if (radix == 'n' && !strcmp(wmark, "nyx"))
			{
			  r->flags |= RARITYFLAG_NYX;
			  continue;
			}

		  if (radix == 'x')
			{
			  if (!strcmp(wmark, "xday"))
				{
				  r->dfc_symbol = DFC_SYMBOL_DAY;
				  continue;
				}
			  if (!strcmp(wmark, "xnight"))
				{
				  r->dfc_symbol = DFC_SYMBOL_NIGHT;
				  continue;
				}
			  if (!strcmp(wmark, "xspark"))
				{
				  r->dfc_symbol = DFC_SYMBOL_SPARK;
				  continue;
				}
			  if (!strcmp(wmark, "xignite"))
				{
				  r->dfc_symbol = DFC_SYMBOL_IGNITE;
				  continue;
				}
			  if (!strcmp(wmark, "xmoon"))
				{
				  r->dfc_symbol = DFC_SYMBOL_MOON;
				  continue;
				}
			  if (!strcmp(wmark, "xemrakul"))
				{
				  r->dfc_symbol = DFC_SYMBOL_EMRAKUL;
				  continue;
				}
			}

		  radix -= 'a';
		  for (idx = 0; lookup[radix][idx]; ++idx)
			if (!strcmp(wmark, watermarks[lookup[radix][idx]-1].name))
			  {
				r->watermark = lookup[radix][idx]-1;
				goto outer_continue;
			  }
		}

	  BAIL("Unknown watermark \"%s\" in line #%d of watermarks.csv [%s]", wmark, linenum, line);

	outer_continue:
	  continue;
	}
  if (id >= 0 && !feof(watermarks_csv))
	BAIL("Could not read from watermarks.csv: %s", strerror(errno));

  if (fclose(watermarks_csv))
	BAIL("Could not close watermarks.csv: %s", strerror(errno));

#undef BAIL
}

static void
read_rarities(void)
{
#define BAIL(...) do { popup("drawcardlib.dll", __VA_ARGS__); return; } while (0)
  rarities_read = 1;

  FILE* rarity_dat;
  if (!(rarity_dat = fopen("rarity.dat", "rb")))
	BAIL("Could not open rarity.dat: %s", strerror(errno));

  if (fread(&total_cards, 4, 1, rarity_dat) != 1)
	BAIL("Could not read number of cards: %s", strerror(errno));

  if (parent == PARENT_MANALINK
	  && total_cards != EXE_DWORD(0x7375ac)/*available_slots*/)
	BAIL("Mismatch between rarity.dat (%d) and cards.dat (%d)", total_cards, EXE_DWORD(0x7375ac));

  if (fread(&raw_expansions, 4, 1, rarity_dat) != 1)
	BAIL("Could not read number of expansions: %s", strerror(errno));

  if (raw_expansions < 9)
	BAIL("number of expansions only %d, expected at least 9", raw_expansions);

  int expansion_size = (raw_expansions * 3 + 7) / 8;

  uint8_t* raw_rarities;

  if (!(raw_rarities = (uint8_t*)calloc(expansion_size, total_cards)))
	BAIL("Could not calloc uint8_t* %d, %d", expansion_size, total_cards);

  int count;
  if ((count = fread(raw_rarities, expansion_size, total_cards, rarity_dat)) != total_cards)
	BAIL("Could only read %d entries, expected %d: %s", count, total_cards, strerror(errno));

  if (fclose(rarity_dat))
	BAIL("Could not close rarity.dat: %s", strerror(errno));

  if (!(rarities = (Rarity**)malloc(total_cards * sizeof(Rarity**))))
	BAIL("Could not malloc Rarity** %d", total_cards * sizeof(Rarity**));

  int card;
  for (card = 0; card < total_cards; ++card)
	{
	  Rarity* r;
	  if (!(r = (Rarity*)malloc(sizeof(Rarity) + raw_expansions + 2)))	// This is wider than it needs to be because of FAKE_EXPANSIONS, but that's ok.
		{
		  rarities = NULL;
		  BAIL("Could not malloc Rarity* %d", sizeof(Rarity) + raw_expansions + 2);
		}

	  rarities[card] = r;

	  int raw_exp;
	  for (raw_exp = FAKE_EXPANSIONS; raw_exp <= raw_expansions; ++raw_exp)
		{
		  int bit_pos = raw_exp * 3;
		  int bit_mask = 0x07 << (bit_pos & 0x07);
		  int offset = card * expansion_size + bit_pos / 8;

		  int exp = raw_exp - FAKE_EXPANSIONS + 1;

		  r->rarity_in_expansion[exp] = ((((uint16_t)(raw_rarities[offset + 1]) << 8) | raw_rarities[offset]) & bit_mask) >> (bit_pos & 0x07);
		}

	  r->watermark = 0xFF;
	  r->dfc_symbol = 0xFF;
	  r->flags = 0;
	}
  free(raw_rarities);

  assign_default_rarities();

#undef BAIL
}

const Rarity*
get_rarity(int csvid)
{
  if (!rarities_read)
	read_rarities();

  if (!rarities || csvid < 0 || csvid >= total_cards)
	return NULL;

  if (rarities[csvid]->default_expansion == 0
	  && csvid > 0
	  && csvid < find_rules_engine_csvid()	// so not a token, vanguard avatar, etc.
	  && csvid != CARD_ID_FACE_DOWN_CREATURE	// should never get a symbol
	  && csvid != CARD_ID_SPAWN_OF_AZAR	// a misplaced token with a valid card (Arena) immediately preceding
	  // Two special cards that shouldn't get expansion symbols, even though the previous card does
	  && csvid != CARD_ID_ARCHENEMY
	  && csvid != CARD_ID_HANDICAP_3_CARDS)
	{
	  int candidate = csvid == CARD_ID_ASSEMBLY_WORKER ? CARD_ID_MISHRAS_FACTORY : csvid - 1;
	  if (rarities[candidate]->default_expansion != 0)
		csvid = candidate;
	}

  return rarities[csvid];
}

static void
read_expansion_categories(void)
{
#define BAIL(...) do { popup("drawcardlib.dll", __VA_ARGS__); goto bail; } while (0)
  if (expansion_categories)
	free(expansion_categories);
  expansion_categories = NULL;

  FILE* menus_txt = NULL;
  if (!(menus_txt = fopen("menus.txt", "r")))
	BAIL("Could not open menus.txt: %s", strerror(errno));

  // Advance to @EXPANSIONCATEGORIES
#define EXPANSIONCATEGORIES	"@EXPANSIONCATEGORIES"
  int strlen_expansioncategories = strlen(EXPANSIONCATEGORIES);
  int last_was_blank = 1;
  char line[1024];
  while (fgets(line, 1024, menus_txt))
	{
	  if (line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
		{
		  last_was_blank = 1;
		  continue;
		}

	  if (last_was_blank
		  && !strncmp(line, EXPANSIONCATEGORIES, strlen_expansioncategories)
		  && isascii(line[strlen_expansioncategories])
		  && (line[strlen_expansioncategories] == '\0' || isspace(line[strlen_expansioncategories])))
		goto found;

	  last_was_blank = 0;
	}
  // If not found:
  BAIL("Couldn't find @EXPANSIONCATEGORIES in Menus.txt");

 found:
  {
	// Number of entries
	int num_entries;
	if (!fgets(line, 1024, menus_txt) || (num_entries = atoi(line)) <= 0)
	  BAIL("Couldn't read number of entries after @EXPANSIONCATEGORIES in Menus.txt");

	int alloc_size = sizeof(ExpansionCategory) * (raw_expansions + 1);
	expansion_categories = malloc(alloc_size);
	// initialize everything to EXPCAT_NONE
	memset(expansion_categories, 0, alloc_size);
	expansion_categories[0] = EXPCAT_NONE;

	int i;
	for (i = 0; i < num_entries && i < raw_expansions; ++i)
	  {
		ExpansionCategory expcat = EXPCAT_NONE;
		if (!fgets(line, 1024, menus_txt)
			|| line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
		  BAIL("Invalid number of entries after @EXPANSIONCATEGORIES: expected %d, found %d", num_entries, i);

		if (!strncasecmp(line, "none", 4))
		  expcat = EXPCAT_NONE;
		else if (!strncasecmp(line, "core", 4))
		  expcat = EXPCAT_CORE;
		else if (!strncasecmp(line, "expert", 6))
		  expcat = EXPCAT_EXPERT;
		else if (!strncasecmp(line, "portal", 6))
		  expcat = EXPCAT_PORTAL;
		else if (!strncasecmp(line, "reprint", 7))
		  expcat = EXPCAT_REPRINT;
		else if (!strncasecmp(line, "casual", 6))
		  expcat = EXPCAT_CASUAL;
		else if (!strncasecmp(line, "promo", 5))
		  expcat = EXPCAT_PROMO;
		else if (!strncasecmp(line, "box", 3))
		  expcat = EXPCAT_BOX;
		else if (!strncasecmp(line, "decks", 5))
		  expcat = EXPCAT_DECKS;
		else
		  BAIL("Invalid expansion category in entry %d after @EXPANSIONCATEGORIES in Menus.txt:\n%s", i+1, line);

		if (i >= FAKE_EXPANSIONS)
		  expansion_categories[i - FAKE_EXPANSIONS + 1] = expcat;
	  }
  }

  return;

 bail:
  {
	// Cleanup any partial initialization
	if (menus_txt)
	  fclose(menus_txt);
	if (expansion_categories)
	  free(expansion_categories);

	// Number of categories set in fallback_expansion_categories[]
	int num_expansion_categories = (sizeof fallback_expansion_categories / sizeof(ExpansionCategory));
	// But at least as many as the number of expansions we read from rarity.dat
	num_expansion_categories = MAX(num_expansion_categories, raw_expansions) + 1;

	int alloc_size = sizeof(ExpansionCategory) * num_expansion_categories;
	expansion_categories = malloc(alloc_size);

	// initialize everything to EXPCAT_NONE
	memset(expansion_categories, 0, alloc_size);
	// overwrite start with our fallback defaults
	memcpy(expansion_categories, fallback_expansion_categories, sizeof fallback_expansion_categories);
  }
#undef BAIL
}

void
assign_default_rarities(void)
{
  read_expansion_categories();

  int card;
  for (card = 0; card < total_cards; ++card)
	{
	  Rarity* r = rarities[card];

	  r->default_expansion = 0;
	  r->default_rarity = NONE;
	  r->flags = 0;
	  r->watermark = 0xFF;
	  r->dfc_symbol = 0xFF;

	  int raw_exp, this_priority, best_priority = expansions_priority[EXPCAT_NONE];

	  for (raw_exp = FAKE_EXPANSIONS; raw_exp <= raw_expansions; ++raw_exp)
		{
		  int exp = raw_exp - FAKE_EXPANSIONS + 1;

		  if (r->rarity_in_expansion[exp] != NONE
			  && (this_priority = expansions_priority[expansion_categories[exp]]) >= best_priority
			  && (this_priority > best_priority		// always use if we've found a new best priority
				  || expansions_latest_printing))	// also use if we've only tied our previous best priority, but using the latest printing option
			{
			  best_priority = this_priority;

			  r->default_rarity = r->rarity_in_expansion[exp];
			  r->default_expansion = exp;
			}
		}
	}

  read_watermarks_csv();
}

ExpansionCategory
expansion_category(int expansion)
{
  if (!rarities_read)
	read_rarities();

  if (expansion < 0 || expansion >= raw_expansions)
	return EXPCAT_NONE;
  return (int)expansion_categories[expansion] >= 0 && expansion_categories[expansion] <= MAX_EXPCAT ? expansion_categories[expansion] : EXPCAT_NONE;
}
