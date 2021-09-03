// -*- tab-width:4; c-basic-offset:2; -*-
// Drawcardlib: display card and mana cost graphics.
// drawmanatext.c: parse rules text; draw mana cost graphics

#include "drawcardlib.h"

#define MANA_SYMBOLS 63
signed char mana_tags[MANA_SYMBOLS * 3 + 1] =
{
#define MANA_TAG_HYBRID_MAX -38
  -38, 'B', 'U',	// 2 letter tags must be checked before single
  -39, 'B', 'G',
  -40, 'B', 'R',
  -41, 'B', 'W',
  -42, 'U', 'B',
  -43, 'U', 'G',
  -44, 'U', 'R',
  -45, 'U', 'W',
  -46, 'G', 'B',
  -47, 'G', 'U',
  -48, 'G', 'R',
  -49, 'G', 'W',
  -50, 'R', 'B',
  -51, 'R', 'U',
  -52, 'R', 'G',
  -53, 'R', 'W',
  -54, 'W', 'B',
  -55, 'W', 'U',
  -56, 'W', 'G',
  -57, 'W', 'R',
#define MANA_TAG_HYBRID_MIN -57

#define MANA_TAG_TWO_NUMBER_MAX -12
  -12, '1', '0',
  -13, '1', '1',
  -14, '1', '2',
  -15, '1', '3',
  -16, '1', '4',
  -17, '1', '5',
  -18, '1', '6',
  -19, '1', '7',
  -20, '1', '8',
  -21, '1', '9',
  -22, '2', '0',
#define MANA_TAG_TWO_NUMBER_MIN -22

#define MANA_TAG_MONO_HYBRID_MAX -58
  -58, '2', 'B',
  -59, '2', 'U',
  -60, '2', 'G',
  -61, '2', 'R',
  -62, '2', 'W',
#define MANA_TAG_MONO_HYBRID_MIN -62

#define MANA_TAG_PHYREXIAN_MAX -25
  -25, 'P', 'B',
  -26, 'P', 'U',
  -27, 'P', 'G',
  -28, 'P', 'R',
  -29, 'P', 'W',
#define MANA_TAG_PHYREXIAN_MIN -29

#define MANA_TAG_X -1
   -1, 'X', 0,

#define MANA_TAG_ONE_NUMBER_MAX -2
   -2, '0', 0,
   -3, '1', 0,
   -4, '2', 0,
   -5, '3', 0,
   -6, '4', 0,
   -7, '5', 0,
   -8, '6', 0,
   -9, '7', 0,
  -10, '8', 0,
  -11, '9', 0,
#define MANA_TAG_ONE_NUMBER_MIN -11

#define MANA_TAG_SNOW -23
  -23, 'I', 0,

#define MANA_TAG_CHAOS -24
  -24, 'A', 0,

#define MANA_TAG_COLORLESS -30
  -30, 'C', 0,

#define MANA_TAG_COLOR_MAX -31
  -31, 'B', 0,
  -32, 'U', 0,
  -33, 'G', 0,
  -34, 'R', 0,
  -35, 'W', 0,
#define MANA_TAG_COLOR_MIN -35

#define MANA_TAG_TAP -36
  -36, 'T', 0,

#define MANA_TAG_UNTAP -37
  -37, 'Q', 0,

#define MANA_TAG_OR -63
  -63, '/', 0,
  0
};

CountColors mana_tag_to_count_colors[255] = { 0 };


void
init_mana_tags(void)
{
  CountColors single_mana[256] = { 0 };
  single_mana[(uint8_t)'B'] = COUNT_COLOR_BLACK;
  single_mana[(uint8_t)'U'] = COUNT_COLOR_BLUE;
  single_mana[(uint8_t)'G'] = COUNT_COLOR_GREEN;
  single_mana[(uint8_t)'R'] = COUNT_COLOR_RED;
  single_mana[(uint8_t)'W'] = COUNT_COLOR_WHITE;

  int i;
  for (i = 0; mana_tags[i]; i += 3)
	switch (mana_tags[i])
	  {
		case MANA_TAG_COLOR_MIN ... MANA_TAG_COLOR_MAX:
		  mana_tag_to_count_colors[(uint8_t)mana_tags[i]] = single_mana[(uint8_t)mana_tags[i + 1]] | COUNT_COLOR_MONO;
		  break;

		case MANA_TAG_MONO_HYBRID_MIN ... MANA_TAG_MONO_HYBRID_MAX:
		case MANA_TAG_PHYREXIAN_MIN ... MANA_TAG_PHYREXIAN_MAX:
		  mana_tag_to_count_colors[(uint8_t)mana_tags[i]] = single_mana[(uint8_t)mana_tags[i + 2]] | COUNT_COLOR_MONO;
		  break;

		case MANA_TAG_HYBRID_MIN ... MANA_TAG_HYBRID_MAX:
		  mana_tag_to_count_colors[(uint8_t)mana_tags[i]] = single_mana[(uint8_t)mana_tags[i + 1]] | single_mana[(uint8_t)mana_tags[i + 2]] | COUNT_COLOR_HYBRID;
		  break;
	  }
}

void
draw_mana_symbol(HDC hdc, char c, int x, int y, int wid, int hgt)
{
  if (!hdc)
	return;

  if ((uint8_t)c < (uint8_t)(-MANA_SYMBOLS))
	return;

  make_gpic_from_pic(MANASYMBOLS);

  UINT src_hgt;
  GdipGetImageHeight(gpics[MANASYMBOLS], &src_hgt);

  int src_x = (-(int)c - 1) * src_hgt /*sic*/;
  int src_y = 0;

  if (wid < 0)
	wid = src_hgt /*sic*/;
  if (hgt < 0)
	hgt = src_hgt;

  GpGraphics* gfx;
  GdipCreateFromHDC(hdc, &gfx);
  GdipSetInterpolationMode(gfx, InterpolationModeHighQuality);

  /* Gdi+ prefilters an image before scaling it, so even with a fully-transparent borders around each subimage, it'll still sometimes pick up edges from
   * adjacent subimages.  So what I do is first crop the image to a secondary bitmap (which does not interpolate), then blt with interpolation from *that*
   * to the final image. */

  GpBitmap* cropped = NULL;
  GdipCloneBitmapAreaI(src_x, src_y, src_hgt /*sic*/, src_hgt, PixelFormat32bppARGB, gpics[MANASYMBOLS], &cropped);

  GdipDrawImageRectI(gfx, cropped, x, y, wid, hgt);

  GdipDisposeImage(cropped);
  GdipDeleteGraphics(gfx);
}

CountColors
manacost_to_count_colors(const card_ptr_t* cp)
{
  if (!cp)
	return 0;

  CountColors rval = 0;

  if (cp->mana_cost_text)
	{
	  const char* q = cp->mana_cost_text;
	  while (*q)
		if (*q != '|')
		  ++q;
		else
		  rval |= mana_tag_to_count_colors[convert_initial_mana_tag(&q)];
	}
  else
	{
	  // old style calculated from card_ptr_t values
	  if (cp->req_black > 0)
		rval |= COUNT_COLOR_MONO | COUNT_COLOR_BLACK;
	  if (cp->req_blue > 0)
		rval |= COUNT_COLOR_MONO | COUNT_COLOR_BLUE;
	  if (cp->req_green > 0)
		rval |= COUNT_COLOR_MONO | COUNT_COLOR_GREEN;
	  if (cp->req_red > 0)
		rval |= COUNT_COLOR_MONO | COUNT_COLOR_RED;
	  if (cp->req_white > 0)
		rval |= COUNT_COLOR_MONO | COUNT_COLOR_WHITE;
	}

  return rval;
}

/* This takes a pointer to a string.  If it starts with a mana tag, it advances the pointer to the next char after the tag and returns the mana tag's code.
 * If it starts with a | but not a valid mana tag, it advances the pointer by one and returns 0.
 * If it doesn't start with a mana tag, it just returns 0. */
int
convert_initial_mana_tag(const char** src_string)
{
  const char* p = *src_string;

  if (*p != '|')
	return 0;

  ++p;

  int16_t needle = *(const int16_t*)p;

  signed char mana_tag_tag;
  signed char* mana_tag_entry;
  for (mana_tag_entry = mana_tags; ((mana_tag_tag = *mana_tag_entry)); mana_tag_entry += 3)
	{
	  int16_t key = *(int16_t*)(mana_tag_entry + 1);
	  if (!(key & 0xff00))
		{
		  if ((int8_t)needle == (int8_t)key)
			break;
		}
	  else if (needle == key)
		{
		  p = p + 1;
		  break;
		}
	}

  if (mana_tag_tag)
	++p;

  *src_string = p;

  return (unsigned char)mana_tag_tag;
}

int
calc_draw_mana_text(HDC hdc, const RECT* rect, const char* str, int rules_text_flag, int no_additional_space, LoyaltyCost* loyalty)
{
  if (!hdc || !rect || !str)
	return 0;

  int savedc = SaveDC(hdc);

  IntersectClipRect(hdc, 0, 0, 1, 1);
  int rval = draw_mana_text(hdc, rect, str, 0, rules_text_flag, no_additional_space, loyalty);
  RestoreDC(hdc, savedc);

  return rval;
}

int
draw_mana_text(HDC hdc, const RECT* dest_rect, const char* str, int drawflag, int rules_text_flag, int no_additional_space, LoyaltyCost* loyalty)
{
  if (!hdc || !dest_rect || !str || !*str)
	return 0;

  int savedc = SaveDC(hdc);

  TEXTMETRIC metrics;
  GetTextMetrics(hdc, &metrics);

  int font_line_hgt = metrics.tmHeight;

  int drawing_on_fullcard = GetMapMode(hdc) == MM_ISOTROPIC;

  RECT rect;
  int line_hgt;
  rect.bottom = line_hgt = metrics.tmHeight - (drawing_on_fullcard ? 8 : 2);
  rect.top = rect.left = rect.right = 0;
  LPtoDP(hdc, (POINT*)(&rect), 2);

  rect.right = rect.bottom - rect.top;
  rect.bottom = rect.left = rect.top = 0;
  DPtoLP(hdc, (POINT*)(&rect), 2);

  /* now a very crude hack to draw proper symbols on a fullcard as GDI+ does not support GDI's map mode and I don't want to mess with transformation matrixes
   * atm -Mok */

  int sym_ext_wid, sym_wid, sym_hgt;
  {
	int w = drawing_on_fullcard ? metrics.tmHeight : (rect.right - rect.left);

	sym_hgt = (int64_t)(metrics.tmHeight) * 75 / 100;
	sym_wid = (int64_t)w * 75 / 100;
	sym_ext_wid = (int64_t)w * 85 / 100;
  }

  IntersectClipRect(hdc, dest_rect->left, dest_rect->top, dest_rect->right + 1, dest_rect->bottom);

  GdiFlush();

  SelectObject(hdc, GetStockObject(BLACK_BRUSH));
  SelectObject(hdc, GetStockObject(NULL_PEN));

  LoyaltyCost partial_loyalty;
  *partial_loyalty.txt = 0;
  partial_loyalty.top = INT_MIN;
  partial_loyalty.bottom = INT_MIN;

  if (loyalty)
	loyalty->top = INT_MIN;

  char buf[3000];
  SIZE word_wid;
  int posx = dest_rect->left;
  int posy = dest_rect->top;
  int max_wid = 0;
  int after_nl = 1;

#define END_LOYALTY_TEXT(foo) do {					\
  if (loyalty && partial_loyalty.top != INT_MIN)	\
	{												\
	  strcpy(loyalty->txt, partial_loyalty.txt);	\
	  loyalty->top = partial_loyalty.top;			\
	  loyalty->bottom = posy + line_hgt;			\
	  ++loyalty;									\
	  loyalty->top = INT_MIN;						\
	  partial_loyalty.top = INT_MIN;				\
	}												\
} while (0)

  while (*str)
	{
	  if (*str == '\n')
		{
		  END_LOYALTY_TEXT(foo);

		  after_nl = 1;
		  ++str;
		  int h = line_hgt;
		  if (!no_additional_space)
			h += h >> 2;
		  posy += h;
		  posx = dest_rect->left;

		  if (*str == '\r')
			++str;

		  continue;
		}

	  if (after_nl && loyalty)
		{
		  const char* end_of_loyalty_cost = NULL;
		  if (!strncmp(str, "0: ", 3))
			end_of_loyalty_cost = str + 3;
		  else if (str[0] == '+' || str[0] == '-')
			{
			  if (!strncmp(str + 1, "X: ", 3))
				end_of_loyalty_cost = str + 4;
			  else
				{
				  const char* p = str + 1;
				  while (*p && *p >= '0' && *p <= '9')
					++p;
				  if (p > str + 1
					  && !strncmp(p, ": ", 2))
					end_of_loyalty_cost = p + 2;
				}
			}

		  if (end_of_loyalty_cost)
			{
			  char txt[LOYALTY_MOD_TXT_LEN];
			  strncpy(txt, str, LOYALTY_MOD_TXT_LEN);
			  txt[LOYALTY_MOD_TXT_LEN - 1] = 0;
			  char* q = strchr(txt, ':');
			  assert(q && "Loyalty cost too long");
			  *q = 0;
			  END_LOYALTY_TEXT(foo);
			  strcpy(partial_loyalty.txt, txt);
			  partial_loyalty.top = posy;
			  str = end_of_loyalty_cost;

			  after_nl = 0;
			  continue;
			}
		}

	  after_nl = 0;

	  if (*str == ' ')
		{
		  after_nl = 0;
		  while (*++str == ' ')	{}

		  *buf = ' ';
		  *(buf + 1) = 0;

		  GetTextExtentPoint32(hdc, buf, 1, &word_wid);	// width of space char
		  posx += word_wid.cx;
		  continue;
		}

	  {
		int buf_len = 0;
		{
		  char* p = buf;
		  int tag;

		  while ((tag = convert_initial_mana_tag(&str)))
			{
			  *p++ = tag;
			  ++buf_len;
			}
		}
		if (buf_len)
		  {
			if (sym_wid * buf_len + posx > dest_rect->right)	// symbols fit into current line?
			  {
				// nope
				int w = posx - dest_rect->left;
				if (w > max_wid)
				  max_wid = w;
				posy += line_hgt;	// move to next line
				posx = dest_rect->left;
			  }

			char* p = buf;
			for (; buf_len; --buf_len)
			  {
				if (drawflag)	// otherwise, calculating size, so no symbols needed
				  draw_mana_symbol(hdc, *p++,
								   ((uint32_t)(sym_ext_wid - sym_wid) >> 1) + posx,
								   ((uint32_t)(font_line_hgt - sym_hgt) >> 1) + posy,
								   sym_wid, sym_hgt);
				posx += sym_ext_wid;
			  }

			if (*str == '.' || *str == ':' || *str == ',')
			  {
				*buf = *str;
				*(buf + 1) = 0;
				++str;

				GetTextExtentPoint32(hdc, buf, 1, &word_wid);
				TextOut(hdc, posx, posy, buf, 1);

				posx += word_wid.cx;
			  }

			continue;
		  }
	  }

	  {
		char* p = buf;
		int buf_len = 0;
		int switch_back = 0;
		while (*str && *str != ' ' && *str != '\n' && *str != '|')
		  {
			if (*str == '(' && rules_text_flag)
			  {
				if (buf_len)
				  break;

				SelectObject(hdc, fonts[FLAVOR_FONT]);
			  }

			char c = *str++;
			if (c == '\r')
			  continue;

			*p++ = c;
			++buf_len;

			if (c == ')' && rules_text_flag)
			  {
				switch_back = 1;	// switch back to normal font after draw
				break;
			  }
		  }
		*p = 0;

		GetTextExtentPoint32(hdc, buf, buf_len, &word_wid);

		if (word_wid.cx + posx > dest_rect->right)
		  {
			if (posx - dest_rect->left > max_wid)
			  max_wid = posx - dest_rect->left;

			posy += line_hgt;
			posx = dest_rect->left;
		  }
		TextOut(hdc, posx, posy, buf, buf_len);

		posx += word_wid.cx;

		if (switch_back)
		  {
			switch_back = 0;
			SelectObject(hdc, fonts[BIGCARDTEXT_FONT]);
		  }
	  }
	}

  END_LOYALTY_TEXT(foo);

  if (posx - dest_rect->left > max_wid)
	max_wid = posx - dest_rect->left;

  int max_hgt = line_hgt + posy - dest_rect->top;

  RestoreDC(hdc, savedc);
  return (max_hgt << 16) | (max_wid & 0xffff);
#undef END_LOYALTY_TEXT
}

int
CalcDrawManaText(HDC hdc, const RECT* rect, const char* str)
{
  EnterCriticalSection(critical_section_for_drawing);
  int rval = calc_draw_mana_text(hdc, rect, str, 0, 0, NULL);
  LeaveCriticalSection(critical_section_for_drawing);
  return rval;
}

int
DrawManaText(HDC hdc, const RECT* dest_rect, const char* str, int drawflag)
{
  EnterCriticalSection(critical_section_for_drawing);
  int rval = draw_mana_text(hdc, dest_rect, str, drawflag, 0, 0, NULL);
  LeaveCriticalSection(critical_section_for_drawing);
  return rval;
}
