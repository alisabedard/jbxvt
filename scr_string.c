#include "scr_string.h"

#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "repaint.h"
#include "repair_damage.h"
#include "screen.h"
#include "scroll.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

/*  Display the string at the current position.  nlcount is the number of new lines
 *  in the string.  */
void scr_string(uint8_t * restrict str, int len, int nlcount)
{
#ifdef SCR_DEBUG
	LOG("scr_string(%s, len: %d, nlcount: %d)", str, len, nlcount);
#endif//SCR_DEBUG
	int x, x2, y, n, i;
	unsigned int width;
	uint8_t *s;

	home_screen();
	cursor();
	if (nlcount > 0) {
		if (jbxvt.scr.current->row > jbxvt.scr.current->bmargin)
			nlcount = 0;
		else
			nlcount -= jbxvt.scr.current->bmargin
				- jbxvt.scr.current->row;
		if (nlcount < 0)
			nlcount = 0;
		else if (nlcount > jbxvt.scr.current->row
			- jbxvt.scr.current->tmargin)
			nlcount = jbxvt.scr.current->row
				- jbxvt.scr.current->tmargin;
		if (nlcount > MAX_SCROLL)
			nlcount = MAX_SCROLL;
		scroll(jbxvt.scr.current->tmargin,
			jbxvt.scr.current->bmargin,nlcount);
		jbxvt.scr.current->row -= nlcount;
	}
	while (len > 0) {
		if (*str == '\n') {
			if (jbxvt.scr.current->row
				== jbxvt.scr.current->bmargin)
				scroll(jbxvt.scr.current->tmargin,
					jbxvt.scr.current->bmargin,1);
			else if (jbxvt.scr.current->row
				< jbxvt.scr.chars.height - 1)
				jbxvt.scr.current->row++;
			check_selection(jbxvt.scr.current->row,
				jbxvt.scr.current->row);
			jbxvt.scr.current->wrap_next = 0;
			len--;
			str++;
			continue;
		}
		if (*str == '\r') {
			jbxvt.scr.current->col = 0;
			jbxvt.scr.current->wrap_next = 0;
			len--;
			str++;
			continue;
		}
		if (*str == '\t') {
			if (jbxvt.scr.current->col
				< jbxvt.scr.chars.width - 1) {
				s = jbxvt.scr.current->text
					[jbxvt.scr.current->row];
				if (s[jbxvt.scr.current->col] == 0)
					s[jbxvt.scr.current->col] = '\t';
				jbxvt.scr.current->col++;
				while (jbxvt.scr.current->col % 8 != 0
					&& jbxvt.scr.current->col
					< jbxvt.scr.chars.width - 1)
					jbxvt.scr.current->col++;
			}
			len--;
			str++;
			continue;
		}
		if (jbxvt.scr.current->wrap_next) {
			jbxvt.scr.current->text[jbxvt.scr.current->row]
				[jbxvt.scr.chars.width] = 1;
			if (jbxvt.scr.current->row
				== jbxvt.scr.current->bmargin)
				scroll(jbxvt.scr.current->tmargin,
					jbxvt.scr.current->bmargin,1);
			else if (jbxvt.scr.current->row
				< jbxvt.scr.chars.height - 1)
				jbxvt.scr.current->row++;
			jbxvt.scr.current->col = 0;
			jbxvt.scr.current->wrap_next = 0;
		}
		check_selection(jbxvt.scr.current->row,
			jbxvt.scr.current->row);
		x = MARGIN + jbxvt.X.font_width * jbxvt.scr.current->col;
		y = MARGIN + jbxvt.X.font_height * jbxvt.scr.current->row;
		for (n = 0; str[n] >= ' '; n++)
			;
		n = n + jbxvt.scr.current->col
			> jbxvt.scr.chars.width
			? jbxvt.scr.chars.width
			- jbxvt.scr.current->col : n;
		if (jbxvt.scr.current->insert) {
			uint32_t *r;
			s = jbxvt.scr.current->text[jbxvt.scr.current->row];
			r = jbxvt.scr.current->rend[jbxvt.scr.current->row];
			for (i = jbxvt.scr.chars.width - 1;
				i >= jbxvt.scr.current->col + n; i--) {
				s[i] = s[i - n];
				r[i] = r[i - n];
			}
			width = (jbxvt.scr.chars.width
				- jbxvt.scr.current->col - n)
				* jbxvt.X.font_width;
			x2 = x + n * jbxvt.X.font_width;
			if (width > 0) {
				XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,
					jbxvt.X.win.vt,jbxvt.X.gc.tx,
					x,y,width,jbxvt.X.font_height,x2,y);
				repair_damage();
			}
		}
		
		memcpy(jbxvt.scr.current->text[jbxvt.scr.current->row]
			+ jbxvt.scr.current->col,str,n);
		/* Clear memory cells which are not part of the
			desired output string.  */
		memset(jbxvt.scr.current->text[jbxvt.scr.current->row]
			+ jbxvt.scr.current->col + n, 0,
			jbxvt.scr.chars.width
			- jbxvt.scr.current->col - n);
#ifdef SCR_DEBUG
		LOG("n: %d, strlen: %lu", n, strlen((const char *)
			jbxvt.scr.current->text[jbxvt.scr.current->row]));
#endif//SCR_DEBUG
			
		paint_rval_text(str,jbxvt.scr.rstyle,n,x,y);
		if (jbxvt.scr.rstyle == 0)
			memset(jbxvt.scr.current->rend
				[jbxvt.scr.current->row]
				+ jbxvt.scr.current->col,
				0,n*sizeof(uint32_t));
		else {
			for (i = 0; i < n; i++)
				jbxvt.scr.current->rend[jbxvt.scr.current->row]
					[jbxvt.scr.current->col + i]
					= jbxvt.scr.rstyle;
			jbxvt.scr.current->rend[jbxvt.scr.current->row]
				[jbxvt.scr.chars.width] = 1;
		}
		len -= n;
		str += n;
		jbxvt.scr.current->col += n;
		if (len > 0 && jbxvt.scr.current->col
			== jbxvt.scr.chars.width && *str >= ' ') {
			if (jbxvt.scr.current->wrap) {
				jbxvt.scr.current->text[jbxvt.scr.current->row]
					[jbxvt.scr.chars.width] = 1;
				if (jbxvt.scr.current->row
					== jbxvt.scr.current->bmargin)
					scroll(jbxvt.scr.current->tmargin,
						jbxvt.scr.current->bmargin,1);
				else
					jbxvt.scr.current->row++;
				jbxvt.scr.current->col = 0;
			} else {
				jbxvt.scr.current->col
					= jbxvt.scr.chars.width - 1;
				cursor();
				return;
			}
		}
	}
	if (jbxvt.scr.current->col == jbxvt.scr.chars.width) {
		jbxvt.scr.current->col = jbxvt.scr.chars.width - 1;
		jbxvt.scr.current->wrap_next = jbxvt.scr.current->wrap;
	}
	cursor();
}


