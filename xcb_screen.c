// Copyright 2016, Jeffrey E. Bedard
#include "xcb_screen.h"
#include "libjb/xcb.h"
static xcb_screen_t * jbxvt_xcb_screen;
static void init(xcb_connection_t * restrict xc)
{
	jbxvt_xcb_screen = jb_get_xcb_screen(xc);
}
xcb_window_t jbxvt_get_root_window(xcb_connection_t * xc)
{
	if (!jbxvt_xcb_screen)
		init(xc);
	return jbxvt_xcb_screen->root;
}
xcb_colormap_t jbxvt_get_colormap(xcb_connection_t * xc)
{
	if (!jbxvt_xcb_screen)
		init(xc);
	return jbxvt_xcb_screen->default_colormap;
}
