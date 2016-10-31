/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.  */
#include "screen.h"
#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scroll.h"
#include <string.h>
#define FSZ jbxvt.X.font.size
#define GET_X(op) p.w op##= FSZ.w; p.y op##= FSZ.h; return p;
static xcb_screen_t * jbxvt_screen;
static void init(xcb_connection_t * restrict c)
{
	jbxvt_screen = jb_get_xcb_screen(c);
}
xcb_window_t jbxvt_get_root_window(xcb_connection_t * c)
{
	if (!jbxvt_screen)
		init(c);
	return jbxvt_screen->root;
}
xcb_colormap_t jbxvt_get_colormap(xcb_connection_t * c)
{
	if (!jbxvt_screen)
		init(c);
	return jbxvt_screen->default_colormap;
}
struct JBDim jbxvt_get_char_size(struct JBDim p)
{
	GET_X(/);
}
struct JBDim jbxvt_get_pixel_size(struct JBDim p)
{
	GET_X(*);
}
#undef GET_X
#undef FSZ
/*  Fix the coordinates so that they are within the screen
    and do not lie within empty space.  */
void jbxvt_fix_coordinates(struct JBDim * restrict rc)
{
#define CSZ jbxvt.scr.chars
	if(!CSZ.h || !CSZ.w)
		  return; // prevent segfault on bad window size.
	JB_LIMIT(rc->x, CSZ.w - 1, 0);
	JB_LIMIT(rc->y, CSZ.h - 1, 0);
#undef CSZ
}
// Set all chars to 'E'
void jbxvt_efill(void)
{
	LOG("jbxvt_efill");
	// Move to cursor home in order for all characters to appear.
	jbxvt_move(0, 0, 0);
	for (uint8_t y = 0; y < jbxvt.scr.chars.h; ++y) {
		memset(jbxvt.scr.current->text[y], 'E', jbxvt.scr.chars.w);
		memset(jbxvt.scr.current->rend[y], 0, jbxvt.scr.chars.w << 2);
	}
	jbxvt_repaint();
}
//  Change between the alternate and the main screens
void jbxvt_change_screen(const bool mode_high)
{
	jbxvt.scr.current = &jbxvt.scr.s[mode_high];
	jbxvt.mode.charsel = 0;
	/*  Do not call jbxvt_erase_screen(JBXVT_ERASE_ALL) here--It causes
	    corruption of the saved line data in this context.  Test case:
	    $ ls; vi
	    q
	    ls
	    <scroll up>
	*/
	jbxvt_reset();
	jbxvt_move(0,0,0);
	jbxvt_erase_screen(JBXVT_ERASE_AFTER);
}
//  Change the rendition style.
void jbxvt_style(const uint32_t style)
{
	// This allows combining styles, 0 resets
	jbxvt.scr.rstyle = style ? jbxvt.scr.rstyle | style : 0;
}
// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(const int8_t count, const int16_t top)
{
	jbxvt_set_scroll(0);
	jbxvt_draw_cursor();
	scroll(top, jbxvt.scr.current->margin.b, count);
	jbxvt_draw_cursor();
}
