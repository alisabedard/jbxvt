#include "repaint.h"


#include "jbxvt.h"
#include "screen.h"
#include "selection.h"
#include "slinest.h"
#include "xvt.h"

#include <stdlib.h>
#include <X11/Xlib.h>

// ascii value representing a pound in at least some fonts:
enum { POUND = 036 };

/*  Paint the text using the rendition value at the screen position.
 */
void paint_rval_text(unsigned char * str, uint32_t rval,
	int len, int x, int y)
{
	bool overstrike = false;

	if (rval & RS_RVID) {
		XSetForeground(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.color.bg);
		XSetBackground(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.color.fg);
	}
	if (rval & RS_BOLD) {
			overstrike = true;
			y +=  jbxvt.X.font->ascent;
			XSetFont(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.font->fid);
	} else {
		overstrike=false;
		y +=  jbxvt.X.font->ascent;
	}
	XDrawImageString(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.tx,x,y,
		(const char *)str,len);
	if (overstrike)
		  XDrawString(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.tx,x + 1,y,
				(const char *)str,len);
	y++;
	if (rval & RS_ULINE)
		XDrawLine(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.tx,x,y,x + len * jbxvt.X.font_width,y);

	if (rval & RS_RVID) {
		XSetForeground(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.color.fg);
		XSetBackground(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.color.bg);
	}
#if 0
	if (rval & RS_BOLD)
		XSetFont(jbxvt.X.dpy,jbxvt.X.gc.tx,jbxvt.X.font->fid);
#endif
}

/* Display the string using the rendition vector at the screen coordinates
 */
static void paint_rvec_text(unsigned char * str,
	unsigned char * rvec, int len, int x, int y)
{
	if (rvec == NULL) {
		paint_rval_text(str,0,len,x,y);
		return;
	}
	while (len > 0) {
		int i;
		for (i = 0; i < len; i++)
			if (rvec[i] != rvec[0])
				break;
		paint_rval_text(str,rvec[0],i,x,y);
		str += i;
		rvec += i;
		len -= i;
		x += i * jbxvt.X.font_width;
	}
}

/* Repaint the box delimited by row1 to row2 and col1 to col2 of the displayed
 * screen from the backup screen.
 */
void repaint(int row1, int row2, int col1, int col2)
{
	unsigned char *s, *str, *r;
	int x, y, x1, y1, x2, width, i, m;
	struct slinest *sl;

	str = (unsigned char *)malloc(cwidth + 1);
	y = row1;
	x1 = MARGIN + col1 * jbxvt.X.font_width;
	y1 = MARGIN + row1 * jbxvt.X.font_height;

	/*  First do any 'scrolled off' lines that are visible.
	 */
	for (i = jbxvt.scr.offset - 1 - row1; y <= row2 && i >= 0; y++, i--) {
		sl = jbxvt.scr.sline.data[i];
		m = (col2 + 1) < sl->sl_length ? (col2 + 1) : sl->sl_length;
		s = sl->sl_text;
		m -= col1;
		for (x = 0; x < m; x++)
			str[x] = s[x + col1] < ' ' ? ' ' : s[x + col1];
		r = sl->sl_rend == NULL ? NULL : sl->sl_rend + col1;
		paint_rvec_text(str,r,m,x1,y1);
		x2 = x1 + m * jbxvt.X.font_width;
		width = (col2 - col1 + 1 - m) * jbxvt.X.font_width;
		if (width > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x2,y1,width,jbxvt.X.font_height,False);
		y1 += jbxvt.X.font_height;
	}


	/*  Now do the remainder from the current screen
	 */
	i = jbxvt.scr.offset > row1 ? 0 : row1 - jbxvt.scr.offset;
	for (; y <= row2; y++, i++) {
		s = screen->text[i];
		m = col1 - 1;
		for (x = col1; x <= col2; x++)
			if (s[x] < ' ')
				str[x - col1] = ' ';
			else {
				str[x - col1] = s[x];
				m = x;
			}
		m++;
		m -= col1;
		r = screen->rend[i][cwidth] == 0 ? NULL : screen->rend[i] + col1;
		paint_rvec_text(str,r,m,x1,y1);
		x2 = x1 + m * jbxvt.X.font_width;
		width = (col2 - col1 + 1 - m) * jbxvt.X.font_width;
		if (width > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,x2,y1,width,jbxvt.X.font_height,False);
		y1 += jbxvt.X.font_height;
	}
	free(str);
	show_selection(row1,row2,col1,col2);
}


