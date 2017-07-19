// Copyright 2017, Jeffrey E. Bedard
#include "font.h"
#include <stdlib.h>
#include "JBXVTOptions.h"
#include "libjb/JBDim.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include "xcb_id_getter.h"
static uint8_t font_ascent;
static struct JBDim font_size;
struct JBDim jbxvt_get_font_size(void)
{
	return font_size;
}
static void setup_font_metrics(xcb_connection_t * xc,
	const xcb_query_font_cookie_t c)
{
	xcb_query_font_reply_t * r = xcb_query_font_reply(xc,
		c, NULL);
	jb_assert(r, "Cannot get font information");
	font_ascent = r->font_ascent;
	font_size.width = (uint16_t)r->max_bounds.character_width;
	font_size.height = r->font_ascent + r->font_descent;
	free(r);
}
uint8_t jbxvt_get_font_ascent(void)
{
	return font_ascent;
}
void jbxvt_init_fonts(xcb_connection_t * xc,
	struct JBXVTOptions * opt)
{
	xcb_font_t f = jbxvt_get_normal_font(xc);
	if (!jb_open_font(xc, f, opt->normal_font))
		jb_require(jb_open_font(xc, f,
			opt->normal_font = "fixed"), // fallback
			"Could not load the primary font");
	xcb_query_font_cookie_t q = xcb_query_font(xc, f);
	f = jbxvt_get_bold_font(xc);
	if (!jb_open_font(xc, f, opt->bold_font))
		jb_open_font(xc, f, opt->normal_font);
	f = jbxvt_get_italic_font(xc);
	if (!jb_open_font(xc, f, opt->italic_font))
		jb_open_font(xc, f, opt->normal_font);
	setup_font_metrics(xc, q);
}
#define FONT_GETTER(name) XCB_ID_GETTER(jbxvt_get_##name##_font)
FONT_GETTER(normal);
FONT_GETTER(bold);
FONT_GETTER(italic);
