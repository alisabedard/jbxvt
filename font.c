// Copyright 2016, Jeffrey E. Bedard
#include "font.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include <stdlib.h>
static uint8_t font_ascent;
static struct JBDim font_size;
struct JBDim jbxvt_get_font_size(void)
{
	return font_size;
}
static xcb_cursor_t get_cursor_from_font(xcb_connection_t * xc,
	const xcb_font_t fid, const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_cursor_t c = xcb_generate_id(xc);
	xcb_create_glyph_cursor(xc, c, fid, fid,
		id, id + 1, fg, fg, fg, bg, bg, bg);
	xcb_close_font(xc, fid);
	return c;
}
xcb_cursor_t jbxvt_get_cursor(xcb_connection_t * xc, const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_font_t f = xcb_generate_id(xc);
	if (!jb_open_font(xc, f, "cursor"))
		return 0;
	return get_cursor_from_font(xc, f, id, fg, bg);
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
