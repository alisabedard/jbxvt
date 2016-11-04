// Copyright 2016, Jeffrey E. Bedard
#include "font.h"
#include "config.h"
#include "cursor.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static uint8_t font_ascent;
static struct JBDim font_size;
struct JBDim jbxvt_get_font_size(void)
{
	return font_size;
}
/* Try to open the named font.  Return true if successful.  Print
   an error message and return false if not successful.  */
static bool open_font(xcb_connection_t * xc, const xcb_font_t f,
	char * name)
{
	xcb_void_cookie_t c = xcb_open_font_checked(xc, f, strlen(name),
		name);
	if (jb_xcb_cookie_has_error(xc, c)) {
		fprintf(stderr, "Cannot open font %s\n", name);
		return false;
	}
	return true;
}
xcb_cursor_t jbxvt_get_cursor(xcb_connection_t * xc, const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_font_t f = xcb_generate_id(xc);
	if (!open_font(xc, f, "cursor"))
		return 0;
	xcb_cursor_t c = xcb_generate_id(xc);
	xcb_create_glyph_cursor(xc, c, f, f,
		id, id + 1, fg, fg, fg, bg, bg, bg);
	xcb_close_font(xc, f);
	return c;
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
xcb_font_t jbxvt_get_normal_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	if (f)
		return f;
	return f = xcb_generate_id(xc);
}
xcb_font_t jbxvt_get_bold_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	if (f)
		return f;
	return f = xcb_generate_id(xc);
}
xcb_font_t jbxvt_get_italic_font(xcb_connection_t * xc)
{
	static xcb_font_t f;
	if (f)
		return f;
	return f = xcb_generate_id(xc);
}
void jbxvt_init_fonts(xcb_connection_t * xc,
	struct JBXVTFontOptions * opt)
{
	xcb_font_t f = jbxvt_get_normal_font(xc);
	jb_require(open_font(xc, f, opt->normal),
		"Could not load the primary font");
	xcb_query_font_cookie_t q = xcb_query_font(xc, f);
	f = jbxvt_get_bold_font(xc);
	if (!open_font(xc, f, opt->bold))
		open_font(xc, f, opt->normal);
	f = jbxvt_get_italic_font(xc);
	if (!open_font(xc, f, opt->italic))
		open_font(xc, f, opt->normal);
	setup_font_metrics(xc, q);
}
