#include "scr_extend_selection.h"

#include "change_selection.h"

#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"
#include "xvt.h"

#include <stdlib.h>

/*  Extend the selection.
 */
void scr_extend_selection(int16_t x, int16_t y, const bool drag)
{
	struct selst sesave1, sesave2;
	struct selst *se;

	if (jbxvt.sel.end1.se_type == NOSEL)
		return;

	int16_t col = (x - MARGIN) / jbxvt.X.font_width;
	int16_t row = (y - MARGIN) / jbxvt.X.font_height;
	fix_rc(&row,&col);

	if (jbxvt.sel.end2.se_type == NOSEL) {
		rc_to_selend(row,col,&jbxvt.sel.end2);
		show_selection(0,jbxvt.scr.chars.height - 1,
			0,jbxvt.scr.chars.width - 1);
		return;
	}

	sesave1 = jbxvt.sel.end1;
	sesave2 = jbxvt.sel.end2;
	if (drag) {

		/*  Anchor the start end.
		 */
		jbxvt.sel.end1 = jbxvt.sel.anchor;
		rc_to_selend(row,col,&jbxvt.sel.end2);
		adjust_selection(&jbxvt.sel.end2);
	} else {
		int16_t r1, r2, c1, c2;
		selend_to_rc(&r1,&c1,&jbxvt.sel.end1);
		selend_to_rc(&r2,&c2,&jbxvt.sel.end2);

		/*  Determine which is the nearest endpoint.
		 */
		if (abs(r1 - row) < abs(r2 - row))
			se = &jbxvt.sel.end1;
		else if (abs(r2 - row) < abs(r1 - row))
			se = &jbxvt.sel.end2;
		else if (r1 == r2) {
			if (row < r1)
				se = (c1 < c2) ? &jbxvt.sel.end1
					: &jbxvt.sel.end2;
			else if (row > r1)
				se = (c1 > c2) ? &jbxvt.sel.end1
					: &jbxvt.sel.end2;
			else
				se = abs(c1 - col) < abs(c2 - col)
					? &jbxvt.sel.end1 : &jbxvt.sel.end2;
		} else
			se = &jbxvt.sel.end2;
		rc_to_selend(row,col,se);
		adjust_selection(se);
	}

	change_selection(&sesave1,&sesave2);
}

