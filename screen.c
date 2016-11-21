/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury.  */
#include "screen.h"
#include "cursor.h"
#include "libjb/log.h"
#include "mode.h"
#include "repaint.h"
#include "sbar.h"
#include "scr_erase.h"
#include "scr_move.h"
#include "scr_reset.h"
#include "scroll.h"
#include "size.h"
#include <string.h>
static struct JBXVTScreen screens[2], * current;
struct JBXVTScreen * jbxvt_get_screen(void)
{
	return current ? current : (current = screens);
}
struct JBXVTScreen * jbxvt_get_screen_at(const uint8_t i)
{
	return &screens[i];
}
static void efill_area(const struct JBDim c)
{
	for (uint8_t y = 0; y < c.h; ++y) {
		memset(current->text[y], 'E', c.w);
		memset(current->rend[y], 0, c.w << 2);
	}
}
// Set all chars to 'E'
void jbxvt_efill(xcb_connection_t * xc)
{
	LOG("jbxvt_efill");
	// Move to cursor home in order for all characters to appear.
	jbxvt_move(xc, 0, 0, 0);
	efill_area(jbxvt_get_char_size());
	jbxvt_repaint(xc);
}
//  Change between the alternate and the main screens
void jbxvt_change_screen(xcb_connection_t * xc, const bool mode_high)
{
	current = (screens + mode_high);
	jbxvt_get_modes()->charsel = 0;
	/*  Do not call jbxvt_erase_screen(JBXVT_ERASE_ALL) here--It causes
	    corruption of the saved line data in this context.  Test case:
	    $ ls; vi
	    q
	    ls
	    <scroll up>
	 */
	jbxvt_reset(xc);
	jbxvt_move(xc, 0, 0, 0);
	jbxvt_erase_screen(xc, JBXVT_ERASE_AFTER);
}
// Scroll from top to current bottom margin count lines, moving cursor
void jbxvt_index_from(xcb_connection_t * xc,
	const int8_t count, const int16_t top)
{
	jbxvt_set_scroll(xc, 0);
	jbxvt_draw_cursor(xc);
	scroll(xc, top, current->margin.b, count);
	jbxvt_draw_cursor(xc);
}
