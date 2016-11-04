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
#include "size.h"
#include <string.h>
// Set all chars to 'E'
void jbxvt_efill(xcb_connection_t * xc)
{
	LOG("jbxvt_efill");
	// Move to cursor home in order for all characters to appear.
	jbxvt_move(xc, 0, 0, 0);
	for (uint8_t y = 0; y < jbxvt_get_char_size().h; ++y) {
		memset(jbxvt.scr.current->text[y], 'E', jbxvt_get_char_size().w);
		memset(jbxvt.scr.current->rend[y], 0, jbxvt_get_char_size().w << 2);
	}
	jbxvt_repaint(xc);
}
//  Change between the alternate and the main screens
void jbxvt_change_screen(xcb_connection_t * xc, const bool mode_high)
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
	scroll(xc, top, jbxvt.scr.current->margin.b, count);
	jbxvt_draw_cursor(xc);
}
