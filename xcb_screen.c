// Copyright 2017, Jeffrey E. Bedard
#include "xcb_screen.h"
#include "libjb/xcb.h"
static xcb_screen_t * get_xcb_screen(xcb_connection_t * restrict xc)
{
    static xcb_screen_t * s;
    return s ? s : (s = jb_get_xcb_screen(xc));
}
xcb_window_t jbxvt_get_root_window(xcb_connection_t * xc)
{
    static xcb_window_t w;
    return w ? w : (w = get_xcb_screen(xc)->root);
}
xcb_colormap_t jbxvt_get_colormap(xcb_connection_t * xc)
{
    static xcb_colormap_t c;
    return c ? c : (c = get_xcb_screen(xc)->default_colormap);
}
