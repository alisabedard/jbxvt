#include "repaint.h"

#include "color.h"
#include "cursor.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "selection.h"
#include "show_selection.h"
#include "slinest.h"
#include "xvt.h"

#include <stdlib.h>

static void set_rval_colors(const uint32_t rval)
{
	// normal foregrounds:
	if (rval & RS_F0)
		  set_fg(COLOR_0);
	else if (rval & RS_F1)
		  set_fg(COLOR_1);
	else if (rval & RS_F2)
		  set_fg(COLOR_2);
	else if (rval & RS_F3)
		  set_fg(COLOR_3);
	else if (rval & RS_F4)
		  set_fg(COLOR_4);
	else if (rval & RS_F5)
		  set_fg(COLOR_5);
	else if (rval & RS_F6)
		  set_fg(COLOR_6);
	else if (rval & RS_F7)
		  set_fg(COLOR_7);
	else if (rval & RS_FR)
		  reset_fg();
	else if (rval & RS_BF) {
		// bright foregrounds:
		if (rval & RS_F0)
		  	  set_fg(BCOLOR_0);
		else if (rval & RS_F1)
		  	  set_fg(BCOLOR_1);
		else if (rval & RS_F2)
		  	  set_fg(BCOLOR_2);
		else if (rval & RS_F3)
		  	  set_fg(BCOLOR_3);
		else if (rval & RS_F4)
		  	  set_fg(BCOLOR_4);
		else if (rval & RS_F5)
		  	  set_fg(BCOLOR_5);
		else if (rval & RS_F6)
		  	  set_fg(BCOLOR_6);
		else if (rval & RS_F7)
		  	  set_fg(BCOLOR_7);
	}
	// normal backgrounds:
	if (rval & RS_B0)
		  set_bg(COLOR_0);
	else if (rval & RS_B1)
		  set_bg(COLOR_1);
	else if (rval & RS_B2)
		  set_bg(COLOR_2);
	else if (rval & RS_B3)
		  set_bg(COLOR_3);
	else if (rval & RS_B4)
		  set_bg(COLOR_4);
	else if (rval & RS_B5)
		  set_bg(COLOR_5);
	else if (rval & RS_B6)
		  set_bg(COLOR_6);
	else if (rval & RS_B7)
		  set_bg(COLOR_7);
	else if (rval & RS_BR)
		  reset_bg();
	else if (rval & RS_BB) {
		// bright backgrounds:
		if (rval & RS_B0)
		  	  set_bg(BCOLOR_0);
		else if (rval & RS_B1)
		  	  set_bg(BCOLOR_1);
		else if (rval & RS_B2)
		  	  set_bg(BCOLOR_2);
		else if (rval & RS_B3)
		  	  set_bg(BCOLOR_3);
		else if (rval & RS_B4)
		  	  set_bg(BCOLOR_4);
		else if (rval & RS_B5)
		  	  set_bg(BCOLOR_5);
		else if (rval & RS_B6)
		  	  set_bg(BCOLOR_6);
		else if (rval & RS_B7)
		  	  set_bg(BCOLOR_7);
	}
}

//  Paint the text using the rendition value at the screen position.
void paint_rval_text(unsigned char * restrict str, uint32_t rval,
	uint8_t len, int16_t x, int16_t y)
{
	set_rval_colors(rval);
	XGCValues v;
	XGetGCValues(jbxvt.X.dpy, jbxvt.X.gc.tx,
		GCForeground|GCBackground, &v);
	if (rval & RS_RVID) { // Reverse looked up colors.
		XSetForeground(jbxvt.X.dpy, jbxvt.X.gc.tx, v.background);
		XSetBackground(jbxvt.X.dpy, jbxvt.X.gc.tx, v.foreground);
	}
	y+= jbxvt.X.font->ascent;

	// Draw text with background:
	XDrawImageString(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.tx,x,y,
		(const char *)str,len);
	if (rval & RS_BOLD) // Fake bold:
		  XDrawString(jbxvt.X.dpy,jbxvt.X.win.vt,
			  jbxvt.X.gc.tx,x + 1,y, (const char *)str,len);
	y++; // Advance for underline.
	if (rval & RS_ULINE)
		XDrawLine(jbxvt.X.dpy,jbxvt.X.win.vt,jbxvt.X.gc.tx,
			x,y,x + len * jbxvt.X.font_width,y);
	reset_color();
}

// Display the string using the rendition vector at the screen coordinates
static void paint_rvec_text(unsigned char * str,
	uint32_t * rvec, unsigned int len,
	int x, int y)
{
	LOG("paint_rvec_text()");
	if (rvec == NULL) {
		paint_rval_text(str,0,len,x,y);
		return;
	}
	while (len > 0) {
		unsigned int i;
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

/* Repaint the box delimited by row1 to row2 and col1 to col2
   of the displayed screen from the backup screen.  */
void repaint(int row1, int row2, int col1, int col2)
{
	LOG("repaint(%d, %d, %d, %d)", row1, row2, col1, col2);
	unsigned char * str = malloc(jbxvt.scr.chars.width + 1);
	int y = row1;
	int x1 = MARGIN + col1 * jbxvt.X.font_width;
	int y1 = MARGIN + row1 * jbxvt.X.font_height;
	int i;
	//  First do any 'scrolled off' lines that are visible.
	for (i = jbxvt.scr.offset - 1 - row1;
		y <= row2 && i >= 0; y++, i--) {
		struct slinest * sl = jbxvt.scr.sline.data[i];
		if(!sl) continue; // prevent segfault
		unsigned int m = (col2 + 1) < sl->sl_length
			? (col2 + 1) : sl->sl_length;
		unsigned char * s = sl->sl_text;
		m -= col1;
		for (unsigned int x = 0; x < m; x++)
			str[x] = s[x + col1] < ' ' ? ' ' : s[x + col1];
		paint_rvec_text(str,sl->sl_rend?sl->sl_rend+col1:NULL,
			m,x1,y1);
		const int x2 = x1 + m * jbxvt.X.font_width;
		const unsigned int width = (col2 - col1 + 1 - m)
			* jbxvt.X.font_width;
		if (width > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
				x2,y1,width,jbxvt.X.font_height,False);
		y1 += jbxvt.X.font_height;
	}

	// Do the remainder from the current screen:
	i = jbxvt.scr.offset > row1 ? 0 : row1 - jbxvt.scr.offset;
	for (; y <= row2; y++, i++) {
		unsigned char * s = jbxvt.scr.current->text[i];
		int m = col1 - 1;
		for(uint8_t x = col1; x <= col2; x++) {
			if (s[x] < ' ') {
				str[x - col1] = ' ';
			} else {
				str[x - col1] = s[x];
				m = x;
			}
		}
		m++;
		m -= col1;
		paint_rvec_text(str, jbxvt.scr.current->rend[i]
			[jbxvt.scr.chars.width]
			? jbxvt.scr.current->rend[i] + col1 : NULL,
			m, x1, y1);
		const int x2 = x1 + m * jbxvt.X.font_width;
		const unsigned int width = (col2 - col1 + 1 - m)
			* jbxvt.X.font_width;
		if (width > 0)
			XClearArea(jbxvt.X.dpy,jbxvt.X.win.vt,
				x2,y1,width,jbxvt.X.font_height,False);
		y1 += jbxvt.X.font_height;
	}
	free(str);
	show_selection(row1,row2,col1,col2);
}


