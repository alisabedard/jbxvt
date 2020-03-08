// Copyright 2017, Jeffrey E. Bedard
#include "font.h"
#include <stdlib.h>
#include "JBXVTOptions.h"
#include "libjb/JBDim.h"
#include "libjb/util.h"
#include "libjb/xcb.h"
#include "xcb_id_getter.h"
static uint8_t font_geometry[3]; // width, height, ascent
#define FONT_GETTER(name) XCB_ID_GETTER(jbxvt_get_##name##_font)
FONT_GETTER(normal);
FONT_GETTER(bold);
FONT_GETTER(italic);
struct JBDim jbxvt_get_font_size(void)
{
    return (struct JBDim){.width = font_geometry[0], .height =
        font_geometry[1]};
}
uint8_t jbxvt_get_font_ascent(void)
{
    return font_geometry[2];
}
static void setup_font_metrics(xcb_connection_t * xc,
    const xcb_query_font_cookie_t c)
{
    xcb_query_font_reply_t * r = xcb_query_font_reply(xc,
        c, NULL);
    jb_assert(r, "Cannot get font information");
    uint8_t * restrict g = font_geometry;
    g[0] = r->max_bounds.character_width;
    g[1] = r->font_ascent + r->font_descent;
    g[2] = r->font_ascent;
    free(r);
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
