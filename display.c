/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "display.h"
#include "paint.h"
#include "xcb_screen.h"
#include "JBXVTOptions.h"
#include "color.h"
#include "font.h"
#include "gc.h"
#include "window.h"
static void setup_gcs(xcb_connection_t * xc, xcb_window_t w)
{
    enum {
        TXT_VM = XCB_GC_FOREGROUND | XCB_GC_BACKGROUND
            | XCB_GC_FONT,
        CUR_VM = XCB_GC_FUNCTION | XCB_GC_PLANE_MASK
    };
    const pixel_t f = jbxvt_get_fg(), b = jbxvt_get_bg();
    xcb_create_gc(xc, jbxvt_get_text_gc(xc), w, TXT_VM,
        (uint32_t[]){f, b, jbxvt_get_normal_font(xc)});
    xcb_create_gc(xc, jbxvt_get_cursor_gc(xc), w, CUR_VM,
        (uint32_t[]){XCB_GX_INVERT, f ^ b});
}
xcb_connection_t * jbxvt_init_display(char * name,
    struct JBXVTOptions * opt)
{
    xcb_connection_t * xc;
    { // screen scope
        int screen = opt->screen;
        xc = jb_get_xcb_connection(NULL, &screen);
        opt->screen = screen;
    }
    jbxvt_init_colors(xc, opt);
    jbxvt_init_fonts(xc, opt);
    jbxvt_create_window(xc, jbxvt_get_root_window(xc),
        opt, (uint8_t *)name);
    setup_gcs(xc, jbxvt_get_vt_window(xc));
    return xc;
}
