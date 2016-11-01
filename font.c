// Copyright 2016, Jeffrey E. Bedard
#include "font.h"
#include "config.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/util.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
static uint8_t font_ascent;
static struct JBDim font_size;
struct JBDim jbxvt_get_font_size(void)
{
	return font_size;
}
static xcb_font_t get_font(xcb_connection_t * xc, const char * name)
{
	errno = 0;
	xcb_font_t f = xcb_generate_id(xc);
	xcb_void_cookie_t c = xcb_open_font_checked(xc, f,
		strlen(name), name);
	if (jb_xcb_cookie_has_error(xc, c)) {
		if (jbxvt.X.font.normal) // Fall back to normal font first
			return jbxvt.X.font.normal;
		c = xcb_open_font_checked(xc, f, sizeof(FALLBACK_FONT),
			FALLBACK_FONT);
		jb_require(!jb_xcb_cookie_has_error(xc, c),
			"Could not load any fonts");
	}
	return f;
}
static void open_cursor(xcb_connection_t * xc, const xcb_font_t f)
{
	errno = 0;
	xcb_void_cookie_t v = xcb_open_font_checked(xc, f, 6,
		"cursor");
	jb_require(!jb_xcb_cookie_has_error(xc, v),
		"Cannot open cursor font");
}
xcb_cursor_t jbxvt_get_cursor(xcb_connection_t * xc, const uint16_t id,
	const uint16_t fg, const uint16_t bg)
{
	xcb_font_t f = xcb_generate_id(xc);
	open_cursor(xc, f);
	xcb_cursor_t c = xcb_generate_id(xc);
	xcb_create_glyph_cursor(xc, c, f, f,
		id, id + 1, fg, fg, fg, bg, bg, bg);
	xcb_close_font(xc, f);
	return c;
}

static void setup_font_metrics(xcb_connection_t * xc,
	const xcb_query_font_cookie_t c)
{
	errno = 0;
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
void jbxvt_setup_fonts(xcb_connection_t * xc)
{
	jbxvt.X.font.normal = get_font(xc, jbxvt.opt.font);
	const xcb_query_font_cookie_t c = xcb_query_font(xc,
		jbxvt.X.font.normal);
	jbxvt.X.font.bold = get_font(xc, jbxvt.opt.bold_font);
	jbxvt.X.font.italic = get_font(xc, jbxvt.opt.italic_font);
	setup_font_metrics(xc, c);
}

