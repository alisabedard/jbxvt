#include "scr_string.h"

#include "cursor.h"

#include "jbxvt.h"
#include "repaint.h"
#include "repair_damage.h"
#include "screen.h"
#include "scroll.h"
#include "selection.h"
#include "xvt.h"

#include <string.h>

/*  Display the string at the current position.  nlcount is the number of new lines
 *  in the string.
 */
void scr_string(unsigned char * restrict str, int len, int nlcount)
{
	int x, x2, y, n, i;
	unsigned int width;
	unsigned char *s, *r;

	home_screen();
	cursor();
	if (nlcount > 0) {
		if (screen->row > screen->bmargin)
			nlcount = 0;
		else
			nlcount -= screen->bmargin - screen->row;
		if (nlcount < 0)
			nlcount = 0;
		else if (nlcount > screen->row - screen->tmargin)
			nlcount = screen->row - screen->tmargin;
		if (nlcount > MAX_SCROLL)
			nlcount = MAX_SCROLL;
		scroll(screen->tmargin,screen->bmargin,nlcount);
		screen->row -= nlcount;
	}
	while (len > 0) {
		if (*str == '\n') {
			if (screen->row == screen->bmargin)
				scroll(screen->tmargin,screen->bmargin,1);
			else if (screen->row < cheight - 1)
				screen->row++;
			check_selection(screen->row,screen->row);
			screen->wrap_next = 0;
			len--;
			str++;
			continue;
		}
		if (*str == '\r') {
			screen->col = 0;
			screen->wrap_next = 0;
			len--;
			str++;
			continue;
		}
		if (*str == '\t') {
			if (screen->col < cwidth - 1) {
				s = screen->text[screen->row];
				if (s[screen->col] == 0)
					s[screen->col] = '\t';
				screen->col++;
				while (screen->col % 8 != 0
					&& screen->col < cwidth - 1)
					screen->col++;
			}
			len--;
			str++;
			continue;
		}
		if (screen->wrap_next) {
			screen->text[screen->row][cwidth] = 1;
			if (screen->row == screen->bmargin)
				scroll(screen->tmargin,screen->bmargin,1);
			else if (screen->row < cheight - 1)
				screen->row++;
			screen->col = 0;
			screen->wrap_next = 0;
		}
		check_selection(screen->row,screen->row);
		x = MARGIN + jbxvt.X.font_width * screen->col;
		y = MARGIN + jbxvt.X.font_height * screen->row;
		for (n = 0; str[n] >= ' '; n++)
			;
		n = n + screen->col > cwidth ? cwidth - screen->col : n;
		if (screen->insert) {
			s = screen->text[screen->row];
			r = screen->rend[screen->row];
			for (i = cwidth - 1; i >= screen->col + n; i--) {
				s[i] = s[i - n];
				r[i] = r[i - n];
			}
			width = (cwidth - screen->col - n) * jbxvt.X.font_width;
			x2 = x + n * jbxvt.X.font_width;
			if (width > 0) {
				XCopyArea(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.win.vt,jbxvt.X.gc.tx,
					x,y,width,jbxvt.X.font_height,x2,y);
				repair_damage();
			}
		}

		paint_rval_text(str,jbxvt.scr.rstyle,n,x,y);
		memcpy(screen->text[screen->row] + screen->col,str,n);
		if (jbxvt.scr.rstyle == 0)
			memset(screen->rend[screen->row] + screen->col,0,n);
		else {
			for (i = 0; i < n; i++)
				screen->rend[screen->row][screen->col + i] = jbxvt.scr.rstyle;
			screen->rend[screen->row][cwidth] = 1;
		}
		len -= n;
		str += n;
		screen->col += n;
		if (len > 0 && screen->col == cwidth && *str >= ' ') {
			if (screen->wrap) {
				screen->text[screen->row][cwidth] = 1;
				if (screen->row == screen->bmargin)
					scroll(screen->tmargin,screen->bmargin,1);
				else
					screen->row++;
				screen->col = 0;
			} else {
				screen->col = cwidth - 1;
				cursor();
				return;
			}
		}
	}
	if (screen->col == cwidth) {
		screen->col = cwidth - 1;
		screen->wrap_next = screen->wrap;
	}
	cursor();
}


