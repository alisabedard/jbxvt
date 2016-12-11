// Copyright 2016, Jeffrey E. Bedard
#include "font.h"
#include <stdlib.h>
#include <xcb/xcb_cursor.h>
#include "libjb/util.h"
#include "libjb/xcb.h"
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
	font_size.width = r->max_bounds.character_width;
	font_size.height = r->font_ascent + r->font_descent;
	free(r);
}
uint8_t jbxvt_get_font_ascent(void)
{
	return font_ascent;
}
static xcb_font_t get_f(xcb_connection_t * restrict xc,
	xcb_font_t * restrict f)
{
	return *f ? *f : (*f = xcb_generate_id(xc));
}
xcb_font_t jbxvt_get_normal_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	return get_f(xc, &f);
}
xcb_font_t jbxvt_get_bold_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	return get_f(xc, &f);
}
xcb_font_t jbxvt_get_italic_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	return get_f(xc, &f);
}
void jbxvt_init_fonts(xcb_connection_t * xc,
	struct JBXVTFontOptions * opt)
{
	xcb_font_t f = jbxvt_get_normal_font(xc);
	if (!jb_open_font(xc, f, opt->normal))
		jb_require(jb_open_font(xc, f,
			opt->normal = "fixed"), // fallback
			"Could not load the primary font");
	xcb_query_font_cookie_t q = xcb_query_font(xc, f);
	f = jbxvt_get_bold_font(xc);
	if (!jb_open_font(xc, f, opt->bold))
		jb_open_font(xc, f, opt->normal);
	f = jbxvt_get_italic_font(xc);
	if (!jb_open_font(xc, f, opt->italic))
		jb_open_font(xc, f, opt->normal);
	setup_font_metrics(xc, q);
}
