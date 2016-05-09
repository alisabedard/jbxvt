#include "scr_extend_selection.h"

#include "change_selection.h"
#include "global.h"
#include "screen.h"
#include "selection.h"
#include "xvt.h"

#include <stdlib.h>

/*  Extend the selection.
 */
void scr_extend_selection(int16_t x, int16_t y, const bool drag)
{
	struct selst sesave1, sesave2;
	struct selst *se;

	if (selend1.se_type == NOSEL)
		return;

	int16_t col = (x - MARGIN) / fwidth;
	int16_t row = (y - MARGIN) / fheight;
	fix_rc(&row,&col);

	if (selend2.se_type == NOSEL) {
		rc_to_selend(row,col,&selend2);
		show_selection(0,cheight - 1,0,cwidth - 1);
		return;
	}

	sesave1 = selend1;
	sesave2 = selend2;
	if (drag) {

		/*  Anchor the start end.
		 */
		selend1 = selanchor;
		rc_to_selend(row,col,&selend2);
		adjust_selection(&selend2);
	} else {
		int16_t r1, r2, c1, c2;
		selend_to_rc(&r1,&c1,&selend1);
		selend_to_rc(&r2,&c2,&selend2);

		/*  Determine which is the nearest endpoint.
		 */
		if (abs(r1 - row) < abs(r2 - row))
			se = &selend1;
		else if (abs(r2 - row) < abs(r1 - row))
			se = &selend2;
		else if (r1 == r2) {
			if (row < r1)
				se = (c1 < c2) ? &selend1 : &selend2;
			else if (row > r1)
				se = (c1 > c2) ? &selend1 : &selend2;
			else
				se = abs(c1 - col) < abs(c2 - col) ? &selend1 : &selend2;
		} else
			se = &selend2;
		rc_to_selend(row,col,se);
		adjust_selection(se);
	}

	change_selection(&sesave1,&sesave2);
}

