// -*- tab-width:4; c-basic-offset:2; -*-
// Drawcardlib: display card and mana cost graphics.
// drawfullcard.c: draw fullsize cards

#include "drawcardlib.h"

Watermark watermarks[256];	// Up to 255 may be defined, plus one as a sentinel

void
blt_expansion_symbol(HDC hdc, const RECT* dest_rect, EXPANSION_t expansion, rarity_t rarity, int smallcard)
{
#define EXPANSIONS_WIDE	6	// Number of expansions wide (times four rarities)
#define EXPANSIONS_TALL	25	// Number of expansions tall

  if (!hdc || !dest_rect || !pics[EXPANSION_SYMBOLS])
	return;

  int src_x, src_y;
  switch (rarity)
	{
	  case NONE:
		return;

	  case COMMON:
	  case LAND:
		src_x = 0;
		break;

	  case UNCOMMON:
		src_x = 1;
		break;

	  case RARE:
		src_x = 2;
		break;

	  case MYTHIC:
	  case SPECIAL:
	  default:
		src_x = 3;
		break;
	}

  int no_early_core_sets, no_early_rarity, force_rarity;
  if (smallcard)
	{
	  no_early_core_sets = cfg->smallcard_expansion_no_early_core_sets;
	  no_early_rarity = cfg->smallcard_expansion_no_early_rarity;
	  force_rarity = cfg->smallcard_expansion_force_rarity;
	}
  else
	{
	  no_early_core_sets = cfg->fullcard_expansion_no_early_core_sets;
	  no_early_rarity = cfg->fullcard_expansion_no_early_rarity;
	  force_rarity = cfg->fullcard_expansion_force_rarity;
	}

  if (no_early_core_sets
	  && (expansion == EXPANSION_LIMITED_EDITION_ALPHA
		  || expansion == EXPANSION_LIMITED_EDITION_BETA
		  || expansion == EXPANSION_UNLIMITED_EDITION
		  || expansion == EXPANSION_REVISED_EDITION
		  || expansion == EXPANSION_FOURTH_EDITION
		  || expansion == EXPANSION_FIFTH_EDITION))
	return;
  else if (force_rarity >= 1 && force_rarity <= 4)
	src_x = force_rarity - 1;
  else if (no_early_rarity
		   && expansion <= EXPANSION_PORTAL_SECOND_AGE && expansion != EXPANSION_EXODUS)
	src_x = 0;

  src_y = expansion - 1;
  if (src_y < 0 || src_y >= (EXPANSIONS_WIDE * EXPANSIONS_TALL))
	return;

  src_x += 4 * (src_y / EXPANSIONS_TALL);
  src_y %= EXPANSIONS_TALL;

  GpGraphics* gfx;

  make_gpic_from_pic(EXPANSION_SYMBOLS);
  GdipCreateFromHDC(hdc, &gfx);
  GdipSetInterpolationMode(gfx, InterpolationModeHighQualityBicubic);

  UINT width, height;
  GdipGetImageWidth(gpics[EXPANSION_SYMBOLS], &width);
  GdipGetImageHeight(gpics[EXPANSION_SYMBOLS], &height);

  int sym_wid = width / (EXPANSIONS_WIDE * 4);
  int sym_hgt = height / EXPANSIONS_TALL;

  int dest_x = dest_rect->left;
  int dest_y = dest_rect->top;
  int dest_wid = (dest_rect->right >= dest_rect->left) ? dest_rect->right - dest_rect->left : sym_wid;
  int dest_hgt = (dest_rect->bottom >= dest_rect->top) ? dest_rect->bottom - dest_rect->top : sym_hgt;

  /* Gdi+ prefilters an image before scaling it, so even with a fully-transparent borders around each subimage, it'll still sometimes pick up edges from
   * adjacent subimages.  So what I do is first crop the image to a secondary bitmap (which does not interpolate), then blt with interpolation from *that*
   * to the final image. */

  GpBitmap* cropped = NULL;
  GdipCloneBitmapAreaI(src_x * sym_wid, src_y * sym_hgt, sym_wid, sym_hgt, PixelFormat32bppARGB, gpics[EXPANSION_SYMBOLS], &cropped);

  GdipDrawImageRectI(gfx, cropped, dest_x, dest_y, dest_wid, dest_hgt);

  GdipDisposeImage(cropped);
  GdipDeleteGraphics(gfx);
}

// Returns 0 if text should use PowertoughnessColor, 1 if should use TitleOnLightColor
static int
blt_powertoughness_box(HDC hdc, const RECT* dest_rect, PicHandleNames frame, GpImageAttributes* alpha_xform)
{
  PicHandleNames ptbox = FRAMEPART_FROM_BASE(POWERTOUGHNESS_MODERN, cfg);
  if (!hdc || !dest_rect || !pics[ptbox])
	return 0;

  typedef enum
  {
	PTY_WHITE = 0,
	PTY_BLUE,
	PTY_BLACK,
	PTY_RED,
	PTY_GREEN,
	PTY_COLORLESS,
	PTY_GOLD,
	PTY_ARTIFACT
  } PowerToughnessY;

  typedef enum
  {
	PTX_NORMAL = 0,
	PTX_GOLD,
	PTX_DUAL_LAND
  } PowerToughnessX;

  int src_x = PTX_NORMAL, src_y, light = 0;

  // Normalize to modern frame set
  frame = BASE_FRAME(frame);

  if (cfg->fullcard_extended_powertoughness)	// Futureshift logic
	switch (frame)
	  {
		case FRAME_MODERN_SPELL_COLORLESS:
		/* These are impossible, other than CARDOV_*_COLORLESS.  They're included to silence an enumeration value not handled in switch warning; but I don't
		 * want to use a default since I need the warning for FRAME_MODERN_MIN ... FRAME_MODERN_MAX. */
		case 0 ... FRAME_MAX_LOADED:
		case FRAME_MODERN_SPELL_GOLD_BLACK ... FRAME_MODERN_SPELL_GOLD_WHITE:
		case FRAME_MODERN_ARTIFACT_GOLD_BLACK ... FRAME_MODERN_ARTIFACT_GOLD_WHITE:
		case FRAME_MODERN_LAND_GOLD_BLACK ... FRAME_MODERN_LAND_GOLD_WHITE:
		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK ... FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE:
		case FRAME_MODERN_NYX_MIN ... FRAME_MAX:
		case PICHANDLE_INVALID:
		  src_x = PTX_NORMAL;		src_y = PTY_COLORLESS;	light = 1;	break;

		case FRAME_MODERN_ARTIFACT_COLORLESS:
		  src_x = PTX_NORMAL;		src_y = PTY_ARTIFACT;	light = 1;	break;

		case FRAME_MODERN_LAND_COLORLESS:			case FRAME_MODERN_ARTIFACT_LAND_COLORLESS:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_COLORLESS;	light = 1;	break;

		case FRAME_MODERN_SPELL_BLACK:				case FRAME_MODERN_ARTIFACT_BLACK:
		case FRAME_MODERN_LAND_BLACK:				case FRAME_MODERN_ARTIFACT_LAND_BLACK:
		case FRAME_MODERN_SPELL_HYBRID_WHITE_BLACK:
		case FRAME_MODERN_SPELL_HYBRID_BLUE_BLACK:
		  src_x = PTX_NORMAL;		src_y = PTY_BLACK;		light = 0;	break;

		case FRAME_MODERN_SPELL_BLUE:				case FRAME_MODERN_ARTIFACT_BLUE:
		case FRAME_MODERN_LAND_BLUE:				case FRAME_MODERN_ARTIFACT_LAND_BLUE:
		case FRAME_MODERN_SPELL_HYBRID_GREEN_BLUE:
		case FRAME_MODERN_SPELL_HYBRID_WHITE_BLUE:
		  src_x = PTX_NORMAL;		src_y = PTY_BLUE;		light = 0;	break;

		case FRAME_MODERN_SPELL_GREEN:				case FRAME_MODERN_ARTIFACT_GREEN:
		case FRAME_MODERN_LAND_GREEN:				case FRAME_MODERN_ARTIFACT_LAND_GREEN:
		case FRAME_MODERN_SPELL_HYBRID_BLACK_GREEN:
		case FRAME_MODERN_SPELL_HYBRID_RED_GREEN:
		  src_x = PTX_NORMAL;		src_y = PTY_GREEN;		light = 0;	break;

		case FRAME_MODERN_SPELL_RED:				case FRAME_MODERN_ARTIFACT_RED:
		case FRAME_MODERN_LAND_RED:					case FRAME_MODERN_ARTIFACT_LAND_RED:
		case FRAME_MODERN_SPELL_HYBRID_BLUE_RED:
		case FRAME_MODERN_SPELL_HYBRID_BLACK_RED:
		  src_x = PTX_NORMAL;		src_y = PTY_RED;		light = 0;	break;

		case FRAME_MODERN_SPELL_WHITE:				case FRAME_MODERN_ARTIFACT_WHITE:
		case FRAME_MODERN_LAND_WHITE:				case FRAME_MODERN_ARTIFACT_LAND_WHITE:
		case FRAME_MODERN_SPELL_HYBRID_RED_WHITE:
		case FRAME_MODERN_SPELL_HYBRID_GREEN_WHITE:
		  src_x = PTX_NORMAL;		src_y = PTY_WHITE;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD_WHITE_BLUE:	case FRAME_MODERN_ARTIFACT_GOLD_WHITE_BLUE:
		case FRAME_MODERN_SPELL_GOLD_GREEN_BLUE:	case FRAME_MODERN_ARTIFACT_GOLD_GREEN_BLUE:
		  src_x = PTX_GOLD;			src_y = PTY_BLUE;		light = 1;	break;

		case FRAME_MODERN_LAND_GOLD_WHITE_BLUE:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE_BLUE:
		case FRAME_MODERN_LAND_GOLD_GREEN_BLUE:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_GREEN_BLUE:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_BLUE;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD_BLUE_BLACK:	case FRAME_MODERN_ARTIFACT_GOLD_BLUE_BLACK:
		case FRAME_MODERN_SPELL_GOLD_WHITE_BLACK:	case FRAME_MODERN_ARTIFACT_GOLD_WHITE_BLACK:
		  src_x = PTX_GOLD;			src_y = PTY_BLACK;		light = 1;	break;

		case FRAME_MODERN_LAND_GOLD_BLUE_BLACK:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_BLACK:
		case FRAME_MODERN_LAND_GOLD_WHITE_BLACK:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE_BLACK:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_BLACK;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD_BLACK_RED:		case FRAME_MODERN_ARTIFACT_GOLD_BLACK_RED:
		case FRAME_MODERN_SPELL_GOLD_BLUE_RED:		case FRAME_MODERN_ARTIFACT_GOLD_BLUE_RED:
		  src_x = PTX_GOLD;			src_y = PTY_RED;		light = 1;	break;

		case FRAME_MODERN_LAND_GOLD_BLACK_RED:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_RED:
		case FRAME_MODERN_LAND_GOLD_BLUE_RED:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_RED:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_RED;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD_RED_GREEN:		case FRAME_MODERN_ARTIFACT_GOLD_RED_GREEN:
		case FRAME_MODERN_SPELL_GOLD_BLACK_GREEN:	case FRAME_MODERN_ARTIFACT_GOLD_BLACK_GREEN:
		  src_x = PTX_GOLD;			src_y = PTY_GREEN;		light = 1;	break;

		case FRAME_MODERN_LAND_GOLD_RED_GREEN:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_RED_GREEN:
		case FRAME_MODERN_LAND_GOLD_BLACK_GREEN:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_GREEN:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_GREEN;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD_GREEN_WHITE:	case FRAME_MODERN_ARTIFACT_GOLD_GREEN_WHITE:
		case FRAME_MODERN_SPELL_GOLD_RED_WHITE:		case FRAME_MODERN_ARTIFACT_GOLD_RED_WHITE:
		  src_x = PTX_GOLD;			src_y = PTY_WHITE;		light = 1;	break;

		case FRAME_MODERN_LAND_GOLD_GREEN_WHITE:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_GREEN_WHITE:
		case FRAME_MODERN_LAND_GOLD_RED_WHITE:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_RED_WHITE:
		  src_x = PTX_DUAL_LAND;	src_y = PTY_WHITE;		light = 1;	break;

		case FRAME_MODERN_SPELL_GOLD:				case FRAME_MODERN_ARTIFACT_GOLD:
		case FRAME_MODERN_LAND_GOLD:				case FRAME_MODERN_ARTIFACT_LAND_GOLD:
		  src_x = PTX_GOLD;			src_y = PTY_GOLD;		light = 1;	break;
	  }
  else
	switch (frame)
	  {
		case FRAME_MODERN_SPELL_COLORLESS:
		case FRAME_MODERN_LAND_COLORLESS:
		case FRAME_MODERN_ARTIFACT_LAND_COLORLESS:
		case FRAME_MODERN_LAND_GOLD_MIN ... FRAME_MODERN_LAND_GOLD_MAX:
		case FRAME_MODERN_ARTIFACT_LAND_GOLD_MIN ... FRAME_MODERN_ARTIFACT_LAND_GOLD_MAX:
		case FRAME_MODERN_SPELL_HYBRID_MIN ... FRAME_MODERN_SPELL_HYBRID_MAX:
		/* These are impossible, other than CARDOV_*_COLORLESS.  They're included to silence an enumeration value not handled in switch warning; but I don't
		 * want to use a default since I need the warning for FRAME_MODERN_MIN ... FRAME_MODERN_MAX. */
		case 0 ... FRAME_MAX_LOADED:
		case FRAME_MODERN_SPELL_GOLD_BLACK ... FRAME_MODERN_SPELL_GOLD_WHITE:
		case FRAME_MODERN_ARTIFACT_GOLD_BLACK ... FRAME_MODERN_ARTIFACT_GOLD_WHITE:
		case FRAME_MODERN_LAND_GOLD_BLACK ... FRAME_MODERN_LAND_GOLD_WHITE:
		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK ... FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE:
		case FRAME_MODERN_NYX_MIN ... FRAME_MAX:
		case PICHANDLE_INVALID:
		  src_y = PTY_COLORLESS;
		  break;

		case FRAME_MODERN_SPELL_WHITE:				case FRAME_MODERN_ARTIFACT_WHITE:
		case FRAME_MODERN_LAND_WHITE:				case FRAME_MODERN_ARTIFACT_LAND_WHITE:
		  src_y = PTY_WHITE;
		  break;
		case FRAME_MODERN_SPELL_BLUE:				case FRAME_MODERN_ARTIFACT_BLUE:
		case FRAME_MODERN_LAND_BLUE:				case FRAME_MODERN_ARTIFACT_LAND_BLUE:
		  src_y = PTY_BLUE;
		  break;
		case FRAME_MODERN_SPELL_BLACK:				case FRAME_MODERN_ARTIFACT_BLACK:
		case FRAME_MODERN_LAND_BLACK:				case FRAME_MODERN_ARTIFACT_LAND_BLACK:
		  src_y = PTY_BLACK;
		  break;
		case FRAME_MODERN_SPELL_RED:				case FRAME_MODERN_ARTIFACT_RED:
		case FRAME_MODERN_LAND_RED:					case FRAME_MODERN_ARTIFACT_LAND_RED:
		  src_y = PTY_RED;
		  break;
		case FRAME_MODERN_SPELL_GREEN:				case FRAME_MODERN_ARTIFACT_GREEN:
		case FRAME_MODERN_LAND_GREEN:				case FRAME_MODERN_ARTIFACT_LAND_GREEN:
		  src_y = PTY_GREEN;
		  break;
		case FRAME_MODERN_SPELL_GOLD:				case FRAME_MODERN_ARTIFACT_GOLD:
		case FRAME_MODERN_LAND_GOLD:				case FRAME_MODERN_ARTIFACT_LAND_GOLD:
		case FRAME_MODERN_SPELL_GOLD_MIN ... FRAME_MODERN_SPELL_GOLD_MAX:
		case FRAME_MODERN_ARTIFACT_GOLD_MIN ... FRAME_MODERN_ARTIFACT_GOLD_MAX:
		  src_y = PTY_GOLD;
		  break;
		case FRAME_MODERN_ARTIFACT_COLORLESS:
		  src_y = PTY_ARTIFACT;
		  break;
	  }

  GpGraphics* gfx;

  make_gpic_from_pic(ptbox);
  GdipCreateFromHDC(hdc, &gfx);
  GdipSetInterpolationMode(gfx, InterpolationModeHighQuality);

  UINT width, height;
  GdipGetImageWidth(gpics[ptbox], &width);
  GdipGetImageHeight(gpics[ptbox], &height);
  if (cfg->fullcard_extended_powertoughness)
	width /= 3;
  height /= 8;

  int dest_x = dest_rect->left;
  int dest_y = dest_rect->top;
  int dest_wid = (dest_rect->right >= dest_rect->left) ? (UINT)(dest_rect->right - dest_rect->left) : width;
  int dest_hgt = (dest_rect->bottom >= dest_rect->top) ? (UINT)(dest_rect->bottom - dest_rect->top) : height;

  GdipDrawImageRectRectI(gfx, gpics[ptbox],
						 dest_x, dest_y, dest_wid, dest_hgt,
						 src_x * width, src_y * height, width, height,
						 UnitPixel, alpha_xform, NULL, NULL);

  GdipDeleteGraphics(gfx);

  return light;
}

static void
blt_type_icon(HDC hdc, const RECT* dest_rect, const card_ptr_t* cp, PicHandleNames frame)
{
  PicHandleNames iconimg = FRAMEPART_FROM_BASE(TYPE_ICON_MODERN, cfg);

  if (!hdc || !dest_rect || !iconimg)
	return;

  int src_y;

  // Normalize to modern frame set
  PicHandleNames base_frame = BASE_FRAME(frame);

  // Dark image for light backgrounds is 0.  Light image for dark backgrounds is 1.
  // Light backgrounds: artifact, colorless, white, gold.  Dark backgrounds: blue, black, green, red.
  switch (base_frame)
	{
	  case FRAME_MODERN_SPELL_BLACK:	case FRAME_MODERN_SPELL_BLUE:	case FRAME_MODERN_SPELL_GREEN:	case FRAME_MODERN_SPELL_RED:
	  case FRAME_MODERN_LAND_BLACK:		case FRAME_MODERN_LAND_BLUE:	case FRAME_MODERN_LAND_GREEN:	case FRAME_MODERN_LAND_RED:
	  case FRAME_MODERN_LAND_GOLD_BLUE_BLACK:			case FRAME_MODERN_LAND_GOLD_BLACK_RED:
	  case FRAME_MODERN_LAND_GOLD_RED_GREEN:			case FRAME_MODERN_LAND_GOLD_GREEN_WHITE:
	  case FRAME_MODERN_LAND_GOLD_BLACK_GREEN:			case FRAME_MODERN_LAND_GOLD_GREEN_BLUE:
	  case FRAME_MODERN_LAND_GOLD_BLUE_RED:				case FRAME_MODERN_LAND_GOLD_RED_WHITE:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_BLACK:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_RED:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD_RED_GREEN:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_GREEN_WHITE:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_GREEN:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_GREEN_BLUE:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_RED:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_RED_WHITE:
	  case FRAME_MODERN_SPELL_HYBRID_BLUE_BLACK:		case FRAME_MODERN_SPELL_HYBRID_BLACK_RED:
	  case FRAME_MODERN_SPELL_HYBRID_RED_GREEN:			case FRAME_MODERN_SPELL_HYBRID_GREEN_WHITE:
	  case FRAME_MODERN_SPELL_HYBRID_BLACK_GREEN:		case FRAME_MODERN_SPELL_HYBRID_GREEN_BLUE:
	  case FRAME_MODERN_SPELL_HYBRID_BLUE_RED:			case FRAME_MODERN_SPELL_HYBRID_RED_WHITE:
		src_y = 1;
		break;

	  default:
#if 0
	  case FRAME_MODERN_SPELL_COLORLESS:				case FRAME_MODERN_SPELL_WHITE:
	  case FRAME_MODERN_SPELL_GOLD_MIN ... FRAME_MODERN_SPELL_GOLD_MAX:
	  case FRAME_MODERN_SPELL_GOLD:

	  case FRAME_MODERN_ARTIFACT_MIN ... FRAME_MODERN_ARTIFACT_MAX:

	  case FRAME_MODERN_LAND_COLORLESS:					case FRAME_MODERN_LAND_WHITE:
	  case FRAME_MODERN_LAND_GOLD_WHITE_BLUE:			case FRAME_MODERN_LAND_GOLD_WHITE_BLACK:
	  case FRAME_MODERN_LAND_GOLD:

	  case FRAME_MODERN_ARTIFACT_LAND_COLORLESS:		case FRAME_MODERN_ARTIFACT_LAND_WHITE:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE_BLUE:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE_BLACK:
	  case FRAME_MODERN_ARTIFACT_LAND_GOLD:

	  case FRAME_MODERN_SPELL_HYBRID_WHITE_BLUE:		case FRAME_MODERN_SPELL_HYBRID_WHITE_BLACK:
		// plus CARDOV_MODERN_COLORLESS and CARDOV_MODERN_SPECIAL
#endif
		src_y = 0;
		break;
	}

  int src_x = -1;

  // Find card type(s).  Surprisingly complex.
  typedef enum
  {
	ICON_TYPE_ARTIFACT = 0,
	ICON_TYPE_CREATURE,
	ICON_TYPE_ENCHANTMENT,
	ICON_TYPE_INSTANT,
	ICON_TYPE_LAND,
	ICON_TYPE_MULTIPLE,
	ICON_TYPE_PLANESWALKER,
	ICON_TYPE_SORCERY
  } IconType;

  const card_data_t* cd;

  if (IS_NYX_FRAME(frame, base_frame))	// If a Nyx frame, then we're drawing an enchantment+something else, so multi.
	src_x = ICON_TYPE_MULTIPLE;
  else if (cp->types[0] == SUBTYPE_TRIBAL)	// If a Tribal (and they're all in types[0] currently), always something else, so multi.
	src_x = ICON_TYPE_MULTIPLE;
  else if ((cd = get_card_data_from_csvid(cp->id)))
	switch (cd->type & (TYPE_PERMANENT | TYPE_SPELL))
	  {
		case 0:	return;
		case TYPE_LAND:			src_x = ICON_TYPE_LAND;		break;
		case TYPE_CREATURE | TYPE_INSTANT:	// creature with flash
		case TYPE_CREATURE:		src_x = ICON_TYPE_CREATURE;	break;
		case TYPE_ENCHANTMENT:	src_x = cp->types[0] == SUBTYPE_PLANESWALKER ? ICON_TYPE_PLANESWALKER : ICON_TYPE_ENCHANTMENT;	break;
		case TYPE_SORCERY:		src_x = ICON_TYPE_SORCERY;	break;
		case TYPE_INSTANT | TYPE_INTERRUPT:
		case TYPE_INTERRUPT:
		case TYPE_INSTANT:		src_x = ICON_TYPE_INSTANT;	break;
		case TYPE_ARTIFACT:		src_x = ICON_TYPE_ARTIFACT;	break;
		default:	// two or more bits set
		  src_x = ICON_TYPE_MULTIPLE;
	  }
  else
	{
	  const int16_t* typ;
	  for (typ = &cp->types[0]; typ <= &cp->types[5]; ++typ)
		switch (*typ)
		  {
			case SUBTYPE_ARTIFACT:		src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_ARTIFACT;	break;
			case SUBTYPE_CREATURE:		src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_CREATURE;	break;
			case SUBTYPE_ENCHANTMENT:	src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_ENCHANTMENT;	break;
			case SUBTYPE_INSTANT:		src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_INSTANT;	break;
			case SUBTYPE_LAND:			src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_LAND;	break;
			case SUBTYPE_PLANE:
			case SUBTYPE_PLANESWALKER:	src_x = src_x != -1 && src_x != ICON_TYPE_PLANESWALKER ? ICON_TYPE_MULTIPLE : ICON_TYPE_PLANESWALKER;	break;
			case SUBTYPE_SORCERY:		src_x = src_x != -1 ? ICON_TYPE_MULTIPLE : ICON_TYPE_SORCERY;	break;
			case SUBTYPE_TRIBAL:		src_x = ICON_TYPE_MULTIPLE;	break;
		  }
	}

  if (src_x == -1)
	return;

  GpGraphics* gfx;

  make_gpic_from_pic(iconimg);
  GdipCreateFromHDC(hdc, &gfx);
  GdipSetInterpolationMode(gfx, InterpolationModeHighQuality);

  UINT width, height;
  GdipGetImageWidth(gpics[iconimg], &width);
  GdipGetImageHeight(gpics[iconimg], &height);
  width /= 8;
  height /= 2;

  int dest_x = dest_rect->left;
  int dest_y = dest_rect->top;
  int dest_wid = (dest_rect->right >= dest_rect->left) ? (UINT)(dest_rect->right - dest_rect->left) : width;
  int dest_hgt = (dest_rect->bottom >= dest_rect->top) ? (UINT)(dest_rect->bottom - dest_rect->top) : height;

  GdipDrawImageRectRectI(gfx, gpics[iconimg],
						 dest_x, dest_y, dest_wid, dest_hgt,
						 src_x * width, src_y * height, width, height,
						 UnitPixel, NULL, NULL, NULL);

  GdipDeleteGraphics(gfx);
}

static int
cost_values_to_cost_text(const card_ptr_t* cp, char* dest)
{
  char buf[104];
  unsigned int cost_colorless;
  int cost_black, cost_blue, cost_green, cost_red, cost_white;

  if (!cp || !dest)
	return 0;

  cost_black = cp->req_black;
  cost_blue = cp->req_blue;
  cost_green = cp->req_green;
  cost_red = cp->req_red;
  cost_white = cp->req_white;
  cost_colorless = cp->req_colorless;

  char* p = buf;
  if (cost_colorless)
	{
	  if ((int)cost_colorless == -1
		  || cost_colorless == 72
		  || cost_colorless == 40)
		{
		  *p++ = '|';
		  *p++ = 'X';
		}
	  else if (cost_colorless <= 9)
		{
		  *p++ = '|';
		  *p++ = '0' + cost_colorless;
		}
	  else if (cost_colorless <= 16)
		{
		  *p++ = '|';
		  *p++ = '1';
		  *p++ = '0' + cost_colorless - 10;
		}
	  // otherwise just displays nothing!
	}

#define COLORED_MANA(var, chr)					\
  for (; var != 0; --var)						\
	{											\
	  *p++ = '|';								\
	  *p++ = chr;								\
	}

  COLORED_MANA(cost_white,	'W');
  COLORED_MANA(cost_green,	'G');
  COLORED_MANA(cost_red,	'R');
  COLORED_MANA(cost_black,	'B');
  COLORED_MANA(cost_blue,	'U');

#undef COLORED_MANA

  if (p == buf)
	{
	  *p++ = '|';
	  *p++ = '0';
	}

  *p = 0;

  strcpy(dest, buf);

  return p - buf;
}

static int
draw_fullcard_art(HDC hdc, RECT* rect, uint32_t id, int pic_version)
{
  RECT rect_big_art_pixels;
  CopyRect(&rect_big_art_pixels, &cfg->fullcard_mana);
  LPtoDP(hdc, (POINT*)(&rect_big_art_pixels), 2);

  LoadBigArt(id, pic_version, rect_big_art_pixels.right - rect_big_art_pixels.left, rect_big_art_pixels.bottom - rect_big_art_pixels.top);
  DrawBigArt(hdc, rect, id, pic_version);
  return IsBigArtRightSize(id, pic_version, rect_big_art_pixels.right - rect_big_art_pixels.left, rect_big_art_pixels.bottom - rect_big_art_pixels.top);
}

static int
has_graveyard_ability(const card_ptr_t* cp)
{
  const card_data_t* cd = get_card_data_from_csvid(cp->id);
  if (!cd)
	return 0;

  switch (cd->cc[2])
	{
	  case 0:	// Probably a bug that it's cc[2]==0.
		return cp->id == CARD_ID_KROVIKAN_HORROR;
	  case 1:	// Has alternate casting cost
		return cp->id == CARD_ID_HAAKON_STROMGALD_SCOURGE;
	  case 2:	// Continuous responders to EVENT_GRAVEYARD_ABILITY.
		switch (cp->id)
		  {
			// These don't actually have graveyard abilities, but are implemented that way for their "whenever put into a graveyard" abilities
			case CARD_ID_BLIGHTSTEEL_COLOSSUS:
			case CARD_ID_DARKSTEEL_COLOSSUS:
			case CARD_ID_DREAD:
			case CARD_ID_EMRAKUL_THE_AEONS_TORN:
			case CARD_ID_GUILE:
			case CARD_ID_KOZILEK_BUTCHER_OF_TRUTH:
			case CARD_ID_LEGACY_WEAPON:
			case CARD_ID_PROGENITUS:
			case CARD_ID_PURITY:
			case CARD_ID_SERRA_AVATAR:
			case CARD_ID_ULAMOG_THE_INFINITE_GYRE:
			case CARD_ID_VIGOR:
			case CARD_ID_WORLDSPINE_WURM:
			  // And this has a cc[2] with bit 2 set so it gets EVENT_MODIFY_MANA_PROD
			case CARD_ID_FERTILE_GROUND:
			  return 0;
			default:
			  return 1;
		  }
	  case 4:	// Activates from hand, including cycling and bloodrush.
		switch (cp->id)
		  {
			case CARD_ID_ETERNAL_DRAGON:
			case CARD_ID_PYREWILD_SHAMAN:
			case CARD_ID_UNDEAD_GLADIATOR:
			case CARD_ID_VISCERA_DRAGGER:
			  return 1;
			default:
			  return 0;
		  }
	  case 5:	// Reanimates self
		return 1;
	  case 6:	// Dredge
		return 1;
	  case 9:	// Activated responders to EVENT_GRAVEYARD_ABILITY, mostly Flashback and Unearth.  Annoyingly, also includes Planeswalkers.
		return cp->types[0] != SUBTYPE_PLANESWALKER;
	  case 10:	// Retrace.  These two are probably bugs.
		return cp->id != CARD_ID_GREAT_SABLE_STAG && cp->id != CARD_ID_TERRA_STOMPER;
	  case 12:	// Storm count
		return cp->id == CARD_ID_VENGEVINE;
	  case 13:	// Trap conditions, including Prowl
		return cp->id == CARD_ID_AUNTIES_SNITCH;
	  case 17:	// Scavenge
		return 1;
	  case 18:	// Returns to play when a creature with cmc >= 6 is played
		return 1;
	  default:
		return 0;
	}
}

static int
manacost_width(HDC hdc, const RECT* dest_rect, const card_ptr_t* cp)
{
  char cost_text[104];

  if (!hdc || !dest_rect || !cp)
	return 0;

  int len;
  if (cp->mana_cost_text)
	{
	  len = 0;
	  const char* q = cp->mana_cost_text;
	  while (*q)
		if (*q++ == '|')
		  ++len;
	}
  else
	{
	  // old style calculated from card_ptr_t values
	  cost_values_to_cost_text(cp, cost_text);
	  len = strlen(cost_text) >> 1;
	}

  len *= (dest_rect->bottom - dest_rect->top);
  return len;
}

static void
draw_manacost(HDC hdc, RECT* dest_rect, const card_ptr_t* cp)
{
  char buf[104];
  const char* p;

  if (!hdc || !dest_rect || !cp)
	return;

  int len;
  if (cp->mana_cost_text)
	{
	  len = 0;
	  const char* q = p = cp->mana_cost_text;
	  while (*q)
		if (*q++ == '|')
		  ++len;
	}
  else
	{
	  // old style calculated from card_ptr_t values
	  cost_values_to_cost_text(cp, buf);
	  len = strlen(buf) >> 1;
	  p = buf;
	}

  if (cfg->fullcard_curve_mana_diameter > 0)
	{
	  // Ignore dest_rect, use cfg->fullcard_curve_mana[] instead
	  int diam = cfg->fullcard_curve_mana_diameter;
	  int rad = diam / 2;
	  int pos = 0;
	  while (*p && pos < FULLCARD_CURVE_MANA_MAX)
		{
		  int8_t tag = convert_initial_mana_tag(&p);
		  if (!tag)
			++p;
		  else
			{
			  draw_mana_symbol(hdc, tag, cfg->fullcard_curve_mana[pos].x - rad, cfg->fullcard_curve_mana[pos].y - rad, diam, diam);
			  ++pos;
			}
		}
	}
  else
	{
	  int width_height = dest_rect->bottom - dest_rect->top;	// mana symbols have same width/height
	  int posx = dest_rect->right - (len * width_height);
	  int posy = dest_rect->top;

	  while (*p)
		{
		  int8_t tag = convert_initial_mana_tag(&p);
		  if (!tag)
			++p;
		  else
			{
			  draw_mana_symbol(hdc, tag, posx, posy, width_height, width_height);
			  posx += width_height;
			}
		}
	}
}

static void
strip_hack_and_sleight_symbols(char* dest, const char* src)
{
  while ((*dest = *src))
	{
	  if (*src == '|'
		  && (*(src + 1) == 'H' || *(src + 1) == 'S'))
		src += (*(src + 2) >= '1' && *(src + 2) <= '4') ? 3 : 2;
	  else
		{
		  ++dest;
		  ++src;
		}
	}
}

static int
strip_planeswalker_base_loyalty(char* srcdest)
{
  int l = strlen(srcdest);
  if (l <= 4)
	return 0;

  char* p = srcdest + l - 1;
  if (*p != ')')
	return 0;

  for (--p; p >= srcdest && *p >= '0' && *p <= '9'; --p)
	{}

  if (p < srcdest || *p != '(' || *(p + 1) == ')')
	return 0;

  int loy = atoi(p + 1);

  --p;
  if (p < srcdest || *p != ' ')
	return 0;

  *p = 0;
  return loy;
}

static COLORREF
get_fullcard_title_text_override_color(PicHandleNames framenum, GpImageAttributes* alpha_xform)
{
  if (cfg->fullcard_title_on_light_visible || cfg->fullcard_title_on_white_black_visible)
	switch (BASE_FRAME(framenum))
	  {
		case FRAME_MODERN_SPELL_HYBRID_WHITE_BLACK:
		  if (cfg->fullcard_title_on_white_black_visible)
			return cfg->fullcard_title_on_white_black_color.colorref;

		  // otherwise, fall through
		case FRAME_MODERN_LAND_COLORLESS:
		case FRAME_MODERN_LAND_WHITE:
		case FRAME_MODERN_LAND_GOLD_MIN ... FRAME_MODERN_LAND_GOLD_MAX:
		case FRAME_MODERN_LAND_GOLD:

		case FRAME_MODERN_ARTIFACT_LAND_COLORLESS:
		case FRAME_MODERN_ARTIFACT_LAND_WHITE:
		case FRAME_MODERN_ARTIFACT_LAND_GOLD_MIN ... FRAME_MODERN_ARTIFACT_LAND_GOLD_MAX:
		case FRAME_MODERN_ARTIFACT_LAND_GOLD:

		case FRAME_MODERN_ARTIFACT_COLORLESS:
		case FRAME_MODERN_ARTIFACT_WHITE:
		case FRAME_MODERN_ARTIFACT_GOLD_MIN ... FRAME_MODERN_ARTIFACT_GOLD_MAX:
		case FRAME_MODERN_ARTIFACT_GOLD:

		case FRAME_MODERN_SPELL_COLORLESS:
		case FRAME_MODERN_SPELL_WHITE:
		case FRAME_MODERN_SPELL_GOLD_MIN ... FRAME_MODERN_SPELL_GOLD_MAX:
		case FRAME_MODERN_SPELL_GOLD:
		case FRAME_MODERN_SPELL_HYBRID_WHITE_BLUE:
		case FRAME_MODERN_SPELL_HYBRID_RED_WHITE:
		case FRAME_MODERN_SPELL_HYBRID_GREEN_WHITE:
		  if (cfg->fullcard_title_on_light_visible)
			return cfg->fullcard_title_on_light_color.colorref;
		  break;

		default:
		  if (alpha_xform && cfg->fullcard_title_on_light_visible)	// CARDOV_*_COLORLESS
			return cfg->fullcard_title_on_light_color.colorref;
		  break;
	  }
  return (COLORREF)(-1);
}

static COLORREF
get_fullcard_rulestext_color(PicHandleNames framenum)
{
  if (cfg->fullcard_rules_on_dark_land_visible || cfg->fullcard_rules_on_white_black_land_visible)
	switch (BASE_FRAME(framenum))
	  {
		case FRAME_MODERN_LAND_GOLD_WHITE_BLACK:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_WHITE_BLACK:
		  if (cfg->fullcard_rules_on_white_black_land_visible)
			return cfg->fullcard_rules_on_white_black_land_color.colorref;
		  // otherwise, fall through
		case FRAME_MODERN_LAND_BLACK:				case FRAME_MODERN_ARTIFACT_LAND_BLACK:
		case FRAME_MODERN_LAND_BLUE:				case FRAME_MODERN_ARTIFACT_LAND_BLUE:
		case FRAME_MODERN_LAND_GREEN:				case FRAME_MODERN_ARTIFACT_LAND_GREEN:
		case FRAME_MODERN_LAND_RED:					case FRAME_MODERN_ARTIFACT_LAND_RED:
		case FRAME_MODERN_LAND_GOLD_BLUE_BLACK:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_BLACK:
		case FRAME_MODERN_LAND_GOLD_BLACK_RED:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_RED:
		case FRAME_MODERN_LAND_GOLD_RED_GREEN:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_RED_GREEN:
		case FRAME_MODERN_LAND_GOLD_BLACK_GREEN:	case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLACK_GREEN:
		case FRAME_MODERN_LAND_GOLD_GREEN_BLUE:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_GREEN_BLUE:
		case FRAME_MODERN_LAND_GOLD_BLUE_RED:		case FRAME_MODERN_ARTIFACT_LAND_GOLD_BLUE_RED:
		  if (cfg->fullcard_rules_on_dark_land_visible)
			return cfg->fullcard_rules_on_dark_land_color.colorref;
		  break;
	  }

  return cfg->fullcard_rulestext_color.colorref;
}

static int
has_manacost(const card_ptr_t* cp)
{
  return ((int8_t)cp->card_type != CP_TYPE_LAND
		  && (int8_t)cp->card_type != CP_TYPE_TOKEN
		  && (int8_t)cp->card_type != CP_TYPE_SCHEME
		  && (int8_t)cp->card_type != CP_TYPE_PLANE
		  && (int8_t)cp->card_type != CP_TYPE_NONE);
}

typedef enum
  {
	TFM_NORMAL,
	TFM_COLORLESS,
	TFM_COLORLESS_ALSO_NORMAL_FULLCARD_ART,
	TFM_CARDBACK,
  } TranslucentFrameMode;

static TranslucentFrameMode
draw_art_first(HDC hdc, uint32_t csvid, int pic_version, PicHandleNames framenum, int* suppress_later_art, int* big_art_exists)
{
  int colorless_option, cardback;
  if (framenum == CARDBACK)
	{
	  colorless_option = cfg->frames_facedown_creature;
	  cardback = 1;
	  framenum = FRAME_MODERN_SPELL_COLORLESS;
	}
  else
	{
	  colorless_option = cfg->frames_translucent_colorless;
	  cardback = 0;
	}

#pragma message "suppress for nyx frames"
  if (BASE_FRAME(framenum) == FRAME_MODERN_SPELL_COLORLESS && colorless_option >= 0)
	{
	  *suppress_later_art = 1;

	  if (cardback)
		{
		  make_gpic_from_pic(CARDBACK);
		  gdip_blt_whole(hdc, &cfg->fullcard_frame, CARDBACK, NULL);
		  *big_art_exists = 1;
		  return TFM_CARDBACK;
		}
	  else
		{
		  *big_art_exists = draw_fullcard_art(hdc, &cfg->fullcard_frame, csvid, pic_version);
		  if (colorless_option & FRAMES_TRANSLUCENT_COLORLESS_ALSO_NORMAL_FULLCARD_ART)
			return TFM_COLORLESS_ALSO_NORMAL_FULLCARD_ART;
		  else
			return TFM_COLORLESS;
			draw_fullcard_art(hdc, &cfg->fullcard_art, csvid, pic_version);
		}
	}
  else
	{
	  if (cfg->fullcard_art_first)
		{
		  *big_art_exists = draw_fullcard_art(hdc, &cfg->fullcard_art, csvid, pic_version);
		  *suppress_later_art = 1;
		}

	  return TFM_NORMAL;
	}
}

static void
draw_fullcard_frame(HDC hdc, PicHandleNames framenum, GpImageAttributes** alpha_xform, TranslucentFrameMode translucent_frame)
{
  if (translucent_frame == TFM_NORMAL)
	gdip_blt_whole(hdc, &cfg->fullcard_frame, framenum, NULL);
  else
	{
	  framenum = FRAMEPART_FROM_BASE(CARDOV_MODERN_COLORLESS, cfg);
	  *alpha_xform = cfg->colorless_alpha_xform;
	  gdip_blt_whole(hdc, &cfg->fullcard_frame, framenum, *alpha_xform);
	}
}

static COLORREF
draw_fullcard_tombstone_and_title(HDC hdc, const card_ptr_t* cp, PicHandleNames framenum, GpImageAttributes* alpha_xform)
{
  RECT rect_title;
  CopyRect(&rect_title, &cfg->fullcard_title);

  int tombstone_drawn = 0;
  if (cfg->fullcard_tombstone_visible && has_graveyard_ability(cp))
	{
	  rect_title.left += cfg->fullcard_tombstone_title_offset;
	  gdip_blt_whole(hdc, &cfg->fullcard_tombstone, FRAMEPART_FROM_BASE(TOMBSTONE_MODERN, cfg), NULL);
	  tombstone_drawn = 1;
	}

  if (cfg->fullcard_type_icon_visible && !(tombstone_drawn && cfg->fullcard_type_icon_suppressed_by_tombstone))
	blt_type_icon(hdc, &cfg->fullcard_type_icon, cp, framenum);

  int rt = 0;
  if (has_manacost(cp) && cfg->fullcard_curve_mana_diameter <= 0)
	rt = manacost_width(hdc, &cfg->fullcard_mana, cp);
  if (rt > 0)
	rt = cfg->fullcard_mana.right - rt - 6; // wanted space between name and mana cost
  else
	rt = cfg->fullcard_title.right;

  SIZE textsize;
  int fnt, len = strlen(cp->full_name);
  for (fnt = BIGCARDTITLE_FONT; fnt < BIGCARDTITLE_FONT9; ++fnt)
	{
	  SelectObject(hdc, fonts[fnt]);
	  GetTextExtentPoint32(hdc, cp->full_name, len, &textsize);
	  if (rect_title.left + textsize.cx <= rt)
		goto found_bigcardtitle_font;
	}
  SelectObject(hdc, fonts[BIGCARDTITLE_FONT9]);
 found_bigcardtitle_font:;

  COLORREF title_text_override_color = get_fullcard_title_text_override_color(framenum, alpha_xform);

  int pos = (cfg->fullcard_title_center
			 ? (rect_title.left + rect_title.right) / 2
			 : rect_title.left);

  SetTextAlign(hdc, cfg->fullcard_title_center ? TA_CENTER : TA_LEFT);

  draw_text_with_shadow_at_x(hdc, &rect_title, pos, &cfg->fullcard_title_txt, title_text_override_color, cp->full_name, len);

  if (cfg->fullcard_title_center)
	SetTextAlign(hdc, TA_LEFT);

  return title_text_override_color;
}

static int
draw_fullcard_type(HDC hdc, const card_ptr_t* cp, COLORREF title_text_override_color, int is_a_planeswalker, int is_a_creature)
{
  int loyalty = 0;

  if (cp->card_type != (uint32_t)(-1) && cp->card_type != CP_TYPE_NONE)	// deliberately non-narrowed
	{
	  char buf[100];	// longest observed: Jeska, Warrior Adept at 44

	  strip_hack_and_sleight_symbols(buf, cp->type_text);

	  if (is_a_planeswalker && !is_a_creature && cfg->fullcard_show_loyalty_base)
		loyalty = strip_planeswalker_base_loyalty(buf);

	  SIZE textsize;
	  int fnt, len = strlen(buf);
	  for (fnt = BIGCARDTYPE_FONT; fnt < BIGCARDTYPE_FONT9; ++fnt)
		{
		  SelectObject(hdc, fonts[fnt]);
		  GetTextExtentPoint32(hdc, buf, len, &textsize);
		  if (cfg->fullcard_type.left + textsize.cx <= cfg->fullcard_type.right)
			goto found_bigcardtype_font;
		}
	  SelectObject(hdc, fonts[BIGCARDTYPE_FONT9]);
	found_bigcardtype_font:

	  draw_text_with_shadow(hdc, &cfg->fullcard_type, &cfg->fullcard_type_txt, title_text_override_color, buf, len);
	}

  return loyalty;
}

static void
draw_fullcard_watermark(HDC hdc, const Rarity* r, const RECT* rect_rulestext)
{
  if (!r || r->watermark == 0xFF)
	return;

  Watermark* wmark = &watermarks[r->watermark];

  if (wmark->expansions && !cfg->fullcard_watermark_outside_set && !wmark->expansions[r->default_expansion])
	return;

  RECT rect_watermark;
  CopyRect(&rect_watermark, &cfg->fullcard_watermark);
  if (rect_rulestext->top < cfg->fullcard_rulestext.top)	// i.e., it was expanded
	{
	  int delta = (cfg->fullcard_rulestext.top - rect_rulestext->top) / 2;
	  rect_watermark.top -= delta;
	  rect_watermark.bottom -= delta;
	}

  int x, y;
  x = wmark->col * watermark_size_x;
  y = wmark->row * watermark_size_y;
  gdip_blt(hdc, &rect_watermark, WATERMARKS, x, y, watermark_size_x, watermark_size_y, NULL);
}

#define MAX_LOYALTY_MODS 5

static int
fullcard_exclude_expanded_text_box(HDC hdc, const card_ptr_t* cp, char* inout_rules_text, RECT* rect_rulestext, RECT* rect_rulesbox, int is_a_planeswalker)
{
  int txt_hgt = 0;

  strip_hack_and_sleight_symbols(inout_rules_text, cp->rules_text);

  if (*inout_rules_text)
	{
	  SelectObject(hdc, fonts[BIGCARDTEXT_FONT]);

	  TEXTMETRIC metrics;
	  GetTextMetrics(hdc, &metrics);

	  // This will just get thrown away, to be recomputed later when actually drawing text
	  LoyaltyCost loyalty_costs[MAX_LOYALTY_MODS + 1];	// last is a sentinel
	  LoyaltyCost* lc = is_a_planeswalker && cfg->fullcard_show_loyalty_cost ? loyalty_costs : NULL;

	  txt_hgt += HIWORD(calc_draw_mana_text(hdc, rect_rulestext, inout_rules_text, 1, 0, lc)) + metrics.tmHeight / 2;
	}

  if (*cp->flavor_text)
	{
	  SelectObject(hdc, fonts[FLAVOR_FONT]);

	  txt_hgt += HIWORD(calc_draw_mana_text(hdc, rect_rulestext, cp->flavor_text, 0, 1, NULL));
	}

  if (rect_rulestext->bottom - rect_rulestext->top < txt_hgt)
	{
	  int diff = rect_rulestext->top - rect_rulesbox->top;
	  rect_rulestext->top = rect_rulestext->bottom - txt_hgt;
	  rect_rulesbox->top = rect_rulestext->top - diff;

	  int savedc = SaveDC(hdc);
	  ExcludeClipRect(hdc, rect_rulesbox->left, rect_rulesbox->top + 1, rect_rulesbox->right, rect_rulesbox->bottom - 1);
	  return savedc;
	}
  else
	return 0;	// fits, no need to expand
}

static void
fullcard_expand_text_box(HDC hdc, RECT* rect_rulesbox, PicHandleNames framenum, GpImageAttributes* alpha_xform, TranslucentFrameMode translucent_frame)
{
  if (translucent_frame != TFM_NORMAL)
	framenum = FRAMEPART_FROM_BASE(CARDOV_MODERN_COLORLESS, cfg);

  UINT width, height;
  GdipGetImageWidth(gpics[framenum], &width);
  GdipGetImageHeight(gpics[framenum], &height);

  // Blit from full width to full width with clipping instead of just to rect_rulesbox to avoid artifacts from the horrid nearest-neighbor filtering.
  int savedc = SaveDC(hdc);
  IntersectClipRect(hdc, rect_rulesbox->left, rect_rulesbox->top + 1, rect_rulesbox->right, rect_rulesbox->bottom - 1);
  rect_rulesbox->left = cfg->fullcard_frame.left;
  rect_rulesbox->right = cfg->fullcard_frame.right;

  gdip_blt(hdc, rect_rulesbox, framenum,
		   0, (height * cfg->fullcard_expand_top / 10000),
		   width, (height * (cfg->fullcard_expand_height) / 10000), alpha_xform);
  RestoreDC(hdc, savedc);
}

static void
draw_fullcard_loyalty_costs(HDC hdc, const RECT* rect_rulestext, LoyaltyCost* loyalty_costs, GpImageAttributes* alpha_xform)
{
  RECT rect_loyalty_cost;
  rect_loyalty_cost.left = cfg->fullcard_loyalty_cost.left;
  rect_loyalty_cost.right = cfg->fullcard_loyalty_cost.right;

  RECT rect_loyalty_cost_box;
  rect_loyalty_cost_box.left = cfg->fullcard_loyalty_cost_box.left;
  rect_loyalty_cost_box.right = cfg->fullcard_loyalty_cost_box.right;
  int loyalty_cost_box_hgt = cfg->fullcard_loyalty_cost_box.bottom - cfg->fullcard_loyalty_cost_box.top;

  SelectObject(hdc, fonts[BIGCARDLOYALTYCOST_FONT]);
  SetTextAlign(hdc, TA_CENTER);

  int i;
  for (i = 0; i < MAX_LOYALTY_MODS && loyalty_costs[i].top != INT_MIN; ++i)
	{
	  if (loyalty_costs[i].top >= rect_rulestext->bottom - cfg->fullcard_loyalty_cull)
		continue;

	  rect_loyalty_cost_box.top = (loyalty_costs[i].top + loyalty_costs[i].bottom) / 2 + cfg->fullcard_loyalty_cost_box.top;
	  rect_loyalty_cost_box.bottom = rect_loyalty_cost_box.top + loyalty_cost_box_hgt;
	  if (rect_loyalty_cost_box.bottom > rect_rulestext->bottom - cfg->fullcard_loyalty_lowest)
		{
		  rect_loyalty_cost_box.top += (rect_rulestext->bottom - cfg->fullcard_loyalty_lowest - rect_loyalty_cost_box.bottom);
		  rect_loyalty_cost_box.bottom = rect_rulestext->bottom - cfg->fullcard_loyalty_lowest;
		}

	  rect_loyalty_cost.top = rect_loyalty_cost_box.top - cfg->fullcard_loyalty_cost_box.top + cfg->fullcard_loyalty_cost.top;
	  rect_loyalty_cost.bottom = rect_loyalty_cost_box.bottom - cfg->fullcard_loyalty_cost_box.bottom + cfg->fullcard_loyalty_cost.bottom;

	  PicHandleNames frm;
	  if (loyalty_costs[i].txt[0] == '-' || loyalty_costs[i].txt[0] == '\227')
		frm = FRAMEPART_FROM_BASE(LOYALTY_MINUS_MODERN, cfg);
	  else if (loyalty_costs[i].txt[0] == '+')
		frm = FRAMEPART_FROM_BASE(LOYALTY_PLUS_MODERN, cfg);
	  else
		frm = FRAMEPART_FROM_BASE(LOYALTY_ZERO_MODERN, cfg);

	  gdip_blt_whole(hdc, &rect_loyalty_cost_box, frm, alpha_xform);

	  int pos = (rect_loyalty_cost.right + rect_loyalty_cost.left) / 2;

	  draw_text_with_shadow_at_x(hdc, &rect_loyalty_cost, pos, &cfg->fullcard_loyalty_txt, -1, loyalty_costs[i].txt, -1);
	}

  SetTextAlign(hdc, TA_LEFT);
}

static void
draw_fullcard_textbox_contents(HDC hdc, const card_ptr_t* cp, RECT* rect_rulestext, char* rules_text, const Rarity* r, PicHandleNames framenum, GpImageAttributes* alpha_xform, int is_a_planeswalker)
{
  if (!*rules_text)	// i.e., not already computed
	strip_hack_and_sleight_symbols(rules_text, cp->rules_text);

  SetTextColor(hdc, get_fullcard_rulestext_color(framenum));

  LoyaltyCost loyalty_costs[MAX_LOYALTY_MODS + 1];	// last is a sentinel
  int show_loyalty_costs = is_a_planeswalker && cfg->fullcard_show_loyalty_cost;

  // Watermark - draw after calculating text height and maybe expanding textbox, but before actually drawing text
  draw_fullcard_watermark(hdc, r, rect_rulestext);

  RECT rect_flavor_text;
  CopyRect(&rect_flavor_text, rect_rulestext);

  if (*rules_text)
	{
	  SelectObject(hdc, fonts[BIGCARDTEXT_FONT]);

	  TEXTMETRIC metrics;
	  GetTextMetrics(hdc, &metrics);

	  rect_flavor_text.top += HIWORD(draw_mana_text(hdc, rect_rulestext, rules_text, 1, 1, 0, show_loyalty_costs ? loyalty_costs : NULL)) + metrics.tmHeight / 3;

	  if (show_loyalty_costs)
		draw_fullcard_loyalty_costs(hdc, rect_rulestext, loyalty_costs, alpha_xform);
	}

  SelectObject(hdc, fonts[FLAVOR_FONT]);

  if (*cp->flavor_text)
	draw_mana_text(hdc, &rect_flavor_text, cp->flavor_text, 1, 0, 1, NULL);
}

static void
draw_fullcard_power_toughness(HDC hdc, PicHandleNames framenum, GpImageAttributes* alpha_xform)
{
  SelectObject(hdc, fonts[BIGCARDPT_FONT]);

  COLORREF pt_color = cfg->fullcard_powertoughness_txt.color.colorref;
  if (cfg->fullcard_powertoughness_box_visible
	  && blt_powertoughness_box(hdc, &cfg->fullcard_powertoughness_box, framenum, alpha_xform)
	  && cfg->fullcard_title_on_light_visible)
	pt_color = cfg->fullcard_title_on_light_color.colorref;

  char buf[100];
  *buf = 0;

  // Power
  if (hack_power < 100)
	sprintf(buf, "%d/", hack_power);
  else if (hack_power == 100)
	strcpy(buf, "*/");
  else if (hack_power < 200)
	sprintf(buf, "%d+*/", hack_power - 100);
  else // hack_power >= 200
	sprintf(buf, "%d-*/", hack_power - 200);

  char* p = buf + strlen(buf);

  // Toughness
  if (hack_toughness < 100)
	sprintf(p, "%d", hack_toughness);
  else if (hack_toughness == 100)
	strcpy(p, "*");
  else if (hack_toughness < 200)
	sprintf(p, "%d+*", hack_toughness - 100);
  else // hack_toughness >= 200
	sprintf(p, "%d-*", hack_toughness - 200);

  int pos = (cfg->fullcard_powertoughness_center
			 ? (cfg->fullcard_powertoughness.right + cfg->fullcard_powertoughness.left) / 2
			 : cfg->fullcard_powertoughness.right);
  SetTextAlign(hdc, cfg->fullcard_powertoughness_center ? TA_CENTER : TA_RIGHT);

  draw_text_with_shadow_at_x(hdc, &cfg->fullcard_powertoughness, pos, &cfg->fullcard_powertoughness_txt, pt_color, buf, -1);
}

static void
draw_fullcard_base_loyalty(HDC hdc, int loyalty, GpImageAttributes* alpha_xform)
{
  gdip_blt_whole(hdc, &cfg->fullcard_loyalty_base_box, FRAMEPART_FROM_BASE(LOYALTY_BASE_MODERN, cfg), alpha_xform);

  SelectObject(hdc, fonts[BIGCARDLOYALTYBASE_FONT]);

  int pos = (cfg->fullcard_loyalty_base.right + cfg->fullcard_loyalty_base.left) / 2;

  SetTextAlign(hdc, TA_CENTER);
  char buf[10];
  sprintf(buf, "%d", loyalty);

  draw_text_with_shadow_at_x(hdc, &cfg->fullcard_loyalty_base, pos, &cfg->fullcard_loyalty_txt, -1, buf, -1);
}

static void
draw_fullcard_border_initial(HDC hdc)
{
  /* This is done before drawing anything else, to ensure there's no transparent strips left over no matter how horribly Windows' drawing functions disagree on
   * where the edges should be. */

  HBRUSH solid_brush = CreateSolidBrush(cfg->fullcard_border_color.colorref);
  SelectObject(hdc, solid_brush);
  SelectObject(hdc, GetStockObject(NULL_PEN));

  // left
  Rectangle(hdc, cfg->fullcard_frame.left-2, cfg->fullcard_frame.top-2, cfg->fullcard_frame.left+50, cfg->fullcard_frame.bottom+2);
  // right
  Rectangle(hdc, cfg->fullcard_frame.right-50, cfg->fullcard_frame.top-2, cfg->fullcard_frame.right+2, cfg->fullcard_frame.bottom+2);
  // top
  Rectangle(hdc, cfg->fullcard_frame.left-2, cfg->fullcard_frame.top-2, cfg->fullcard_frame.right+2, cfg->fullcard_frame.top+50);
  // bottom
  Rectangle(hdc, cfg->fullcard_frame.left-2, cfg->fullcard_frame.bottom-50, cfg->fullcard_frame.right+2, cfg->fullcard_frame.bottom+2);

  if (solid_brush)
	DeleteObject(solid_brush);
}

static void
draw_fullcard_border(HDC hdc)
{
  // Make very, very sure that that the edges are even, even if there are multiple visible frame elements abutting one (as in the futureshifted frames)
  ExcludeClipRect(hdc, cfg->fullcard_frame.left+2, cfg->fullcard_frame.top+2, cfg->fullcard_frame.right-2, cfg->fullcard_frame.bottom-2);

  HBRUSH solid_brush = CreateSolidBrush(cfg->fullcard_border_color.colorref);
  SelectObject(hdc, solid_brush);
  SelectObject(hdc, GetStockObject(NULL_PEN));

  RoundRect(hdc, 0, 0, 800, 1200, cfg->fullcard_rounding.x, cfg->fullcard_rounding.y);

  if (solid_brush)
	DeleteObject(solid_brush);
}

int
DrawFullCard(HDC hdc, const RECT* dest_rect, const card_ptr_t* cp, int pic_version, int big_art_style, int expand_text_box, const char* illus)
{
  /* big_art_style is 0 or 2 in _BIGCARD_WNDPROC; 2 in _TENTATIVE_DIALOG_MULLIGAN and sub_4842A0; 17 in sub_43DBE0() and sub_43E620() and sub_470F80() and
   * sub_4D1950() and sub_4D1BC0; 18 in DialogFunc() and sub_46A530() and sub_46B950() and sub_4AA090; varies in sub_4D1780 (second-last param, may be 17, 18,
   * 0, or 2) */
  (void)big_art_style;
  (void)illus;

  if (!hdc || !dest_rect || !cp)
	return 0;

  EnterCriticalSection(critical_section_for_drawing);

  if (configs[CFG_BASE].fullcard_reread_config_on_redraw)
	{
	  read_cfg();
	  assign_default_rarities();
	}

  int savedc = SaveDC(hdc);
  SetMapMode(hdc, MM_ISOTROPIC);
  SetWindowExtEx(hdc, 800, 1200, NULL);

  SetViewportExtEx(hdc, dest_rect->right - dest_rect->left, dest_rect->bottom - dest_rect->top, NULL);
  SetWindowOrgEx(hdc, 400, 600, NULL);
  SetViewportOrgEx(hdc, (dest_rect->right + dest_rect->left) / 2, (dest_rect->bottom + dest_rect->top) / 2, NULL);

  const Rarity* r = get_rarity(cp->id);
  PicHandleNames framenum = select_frame(cp, r);

  int is_a_creature = (hack_power || hack_toughness
					   || (int8_t)cp->card_type == CP_TYPE_CREATURE || (int8_t)cp->card_type == CP_TYPE_TOKEN
					   || ((int8_t)cp->card_type == CP_TYPE_ARTIFACT
						   && (cp->types[0] == SUBTYPE_CREATURE
							   || cp->types[1] == SUBTYPE_CREATURE
							   || cp->types[2] == SUBTYPE_CREATURE
							   || cp->types[3] == SUBTYPE_CREATURE
							   || cp->types[4] == SUBTYPE_CREATURE
							   || cp->types[5] == SUBTYPE_CREATURE
							   || cp->types[6] == SUBTYPE_CREATURE)));
  int is_a_planeswalker = is_planeswalker_by_cp(cp);

  GpImageAttributes* alpha_xform = NULL;
  int suppress_later_art = 0, big_art_exists = 0;

  char rules_text[2000];	// longest observed: Dance of the Dead at 658
  *rules_text = 0;

  SetBkMode(hdc, TRANSPARENT);
  SetTextAlign(hdc, TA_LEFT);

  draw_fullcard_border_initial(hdc);

  RECT rect_rulestext, rect_rulesbox;
  CopyRect(&rect_rulestext, &cfg->fullcard_rulestext);
  CopyRect(&rect_rulesbox, &cfg->fullcard_rulesbox);

  TranslucentFrameMode translucent_frame = draw_art_first(hdc, cp->id, pic_version, framenum, &suppress_later_art, &big_art_exists);

  int exclude_text_box_dc = 0;
  if (expand_text_box)
	exclude_text_box_dc = fullcard_exclude_expanded_text_box(hdc, cp, rules_text, &rect_rulestext, &rect_rulesbox, is_a_planeswalker);

  if (translucent_frame == TFM_COLORLESS_ALSO_NORMAL_FULLCARD_ART)
	draw_fullcard_art(hdc, &cfg->fullcard_art, cp->id, pic_version);	// after clipping expanded text box

  draw_fullcard_frame(hdc, framenum, &alpha_xform, translucent_frame);

  if (cfg->fullcard_outline_visible)
	draw_rect_outline(hdc, &cfg->fullcard_frame, cfg->fullcard_outline_color.colorref);

  COLORREF title_text_override_color = draw_fullcard_tombstone_and_title(hdc, cp, framenum, alpha_xform);

  int loyalty = draw_fullcard_type(hdc, cp, title_text_override_color, is_a_planeswalker, is_a_creature);

  if (cfg->fullcard_expansion_visible && r)
	blt_expansion_symbol(hdc, &cfg->fullcard_expansion, r->default_expansion, r->default_rarity, 0);

  if (has_manacost(cp))
	draw_manacost(hdc, &cfg->fullcard_mana, cp);

  if (!suppress_later_art)
	big_art_exists = draw_fullcard_art(hdc, &cfg->fullcard_art, cp->id, pic_version);

  if (cfg->fullcard_artoutline_visible)
	draw_rect_outline(hdc, &cfg->fullcard_art, cfg->fullcard_artoutline_color.colorref);

  if (exclude_text_box_dc)
	{
	  RestoreDC(hdc, exclude_text_box_dc);
	  fullcard_expand_text_box(hdc, &rect_rulesbox, framenum, alpha_xform, translucent_frame);
	}

	draw_fullcard_textbox_contents(hdc, cp, &rect_rulestext, rules_text, r, framenum, alpha_xform, is_a_planeswalker);

  if (is_a_creature)
	draw_fullcard_power_toughness(hdc, framenum, alpha_xform);

  if (is_a_planeswalker && loyalty != 0 && !is_a_creature)
	draw_fullcard_base_loyalty(hdc, loyalty, alpha_xform);

  draw_fullcard_border(hdc);

  RestoreDC(hdc, savedc);
  LeaveCriticalSection(critical_section_for_drawing);
  return big_art_exists;
}

int
DrawCardBack(HDC hdc, const RECT* dest_rect,
			 // An appalling hack to use this as an interface for other functions, but still safer than adding new imports.
			 unsigned int mode, int arg1, int arg2, int arg3, int arg4)
{
  int rval;
  RECT pic_rect;
  int borderx, bordery;

  if (!hdc || !dest_rect)
	return 0;

  EnterCriticalSection(critical_section_for_drawing);

  if ((int)hdc == -1 && (int)dest_rect == -1 && (mode & 0xFFFFFF00) == 0x7D260A00)
	{
	  switch (mode & 0xFF)
		{
		  case 1:
			draw_special_counters((HDC)arg1, (RECT*)arg2, arg3, arg4);
			rval = 0;
			break;

		  case 2:
			draw_standard_counters((HDC)arg1, (RECT*)arg2, arg3, arg4);
			rval = 0;
			break;

		  case 3:
			get_special_counter_rect((RECT*)arg1, (const RECT*)arg2, arg3);
			rval = 0;
			break;

		  default:
			rval = 0;
			break;
		}
	}
  else
	{
	  borderx = (dest_rect->right - dest_rect->left) * 3 / 100;
	  if (borderx < 1)
		borderx = 1;

	  bordery = (dest_rect->bottom - dest_rect->top) / 50;
	  if (bordery < 1)
		bordery = 1;

	  FillRect(hdc, dest_rect, GetStockObject(BLACK_BRUSH));

	  SetRect(&pic_rect,
			  dest_rect->left + borderx, dest_rect->top + bordery,
			  dest_rect->right - borderx, dest_rect->bottom - bordery);

	  if (cardback_renderer == RENDERER_GDIPLUS)
		gdip_blt_whole(hdc, &pic_rect, CARDBACK, NULL);
	  else
		{
		  int savedc = SaveDC(hdc);
		  SelectObject(*spare_hdc, pics[CARDBACK]);

		  BITMAP bmp;
		  GetObject(pics[CARDBACK], sizeof(BITMAP), &bmp);
		  int src_width = bmp.bmWidth;
		  int src_height = bmp.bmHeight;

		  StretchBlt(hdc, dest_rect->left, dest_rect->top, dest_rect->right - dest_rect->left, dest_rect->bottom - dest_rect->top,
					 *spare_hdc, 0, 0, src_width, src_height, SRCCOPY);

		  RestoreDC(hdc, savedc);
		}

	  rval = 1;
	}

  LeaveCriticalSection(critical_section_for_drawing);
  return rval;
}
