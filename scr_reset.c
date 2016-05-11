#include "scr_reset.h"

#include "cursor.h"

#include "jbxvt.h"
#include "repaint.h"
#include "sbar.h"
#include "screen.h"
#include "screenst.h"
#include "scroll.h"
#include "selection.h"
#include "slinest.h"
#include "ttyinit.h"
#include "xvt.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

/*  Reset the screen - called whenever the screen needs to be repaired completely.
 */
void scr_reset(void)
{
	Window root;
	int x, y, n, i, j, onscreen;
	unsigned int width, height, u;
	unsigned char **r1, **r2, **s1, **s2;
	struct slinest *sl;

	XGetGeometry(jbxvt.X.dpy,jbxvt.X.win.vt,&root,
		&x,&y,&width,&height,&u, &u);
	int16_t cw = (width - 2 * MARGIN) / jbxvt.X.font_width;
	int16_t ch = (height - 2 * MARGIN) / jbxvt.X.font_height;

	if (jbxvt.scr.current->text == NULL || cw != jbxvt.scr.chars.width || ch != jbxvt.scr.chars.height) {

		jbxvt.scr.offset = 0;
		/*  Recreate the screen backup arrays.
		 *  The screen arrays are one byte wider than the screen and
		 *  the last byte is used as a flag which is non-zero of the
		 *  line wrapped automatically.
		 */
		s1 = (unsigned char **)malloc(ch * sizeof(unsigned char *));
		s2 = (unsigned char **)malloc(ch * sizeof(unsigned char *));
		r1 = (unsigned char **)malloc(ch * sizeof(unsigned char *));
		r2 = (unsigned char **)malloc(ch * sizeof(unsigned char *));
		for (y = 0; y < ch; y++) {
			s1[y] = (unsigned char *)malloc(cw + 1);
			s2[y] = (unsigned char *)malloc(cw + 1);
			r1[y] = (unsigned char *)malloc(cw + 1);
			r2[y] = (unsigned char *)malloc(cw + 1);
			memset(s1[y],0,cw + 1);
			memset(s2[y],0,cw + 1);
			memset(r1[y],0,cw + 1);
			memset(r2[y],0,cw + 1);
		}
		if (jbxvt.scr.s1.text != NULL) {

			/*  Now fill up the screen from the old screen
			    and saved lines.  */
			if (jbxvt.scr.s1.row >= ch) {
				/* scroll up to save any lines that will be lost.
				 */
				scroll1(jbxvt.scr.s1.row - ch + 1);
				jbxvt.scr.s1.row = ch - 1;
			}
			/* calculate working no. of lines.
			 */
			i = jbxvt.scr.sline.top + jbxvt.scr.s1.row + 1;
			j = i > ch ? ch - 1 : i - 1;
			i = jbxvt.scr.s1.row;
			jbxvt.scr.s1.row = j;
			onscreen = 1;
			for (; j >= 0; j--) {
				if (onscreen) {
					n = cw < jbxvt.scr.chars.width ? cw : jbxvt.scr.chars.width;
					memcpy(s1[j],jbxvt.scr.s1.text[i],n);
					memcpy(s2[j],jbxvt.scr.s2.text[i],n);
					memcpy(r1[j],jbxvt.scr.s1.rend[i],n);
					memcpy(r2[j],jbxvt.scr.s2.rend[i],n);
					s1[j][cw] = jbxvt.scr.s1.text[i][jbxvt.scr.chars.width];
					s2[j][cw] = jbxvt.scr.s2.text[i][jbxvt.scr.chars.width];
					r1[j][cw] = jbxvt.scr.s1.rend[i][jbxvt.scr.chars.width];
					r2[j][cw] = jbxvt.scr.s2.rend[i][jbxvt.scr.chars.width];
					i--;
					if (i < 0) {
						onscreen = 0;
						i = 0;
					}
				} else {
					if (i >= jbxvt.scr.sline.top)
						break;
					sl = jbxvt.scr.sline.data[i];
					n = cw < sl->sl_length ? cw : sl->sl_length;
					memcpy(s1[j],sl->sl_text,n);
					free(sl->sl_text);
					if (sl->sl_rend != NULL) {
						memcpy(r1[j],sl->sl_rend,n);
						r1[j][cw] =
							sl->sl_rend[sl->sl_length];
						free(sl->sl_rend);
					}
					free((void *)sl);
					i++;
				}
			}
			if (onscreen)
				abort();
			for (j = i; j < jbxvt.scr.sline.top; j++)
				jbxvt.scr.sline.data[j - i]
					= jbxvt.scr.sline.data[j];
			for (j = jbxvt.scr.sline.top - i;
				j < jbxvt.scr.sline.top; j++)
				jbxvt.scr.sline.data[j] = NULL;
			jbxvt.scr.sline.top -= i;

			for (y = 0; y < jbxvt.scr.chars.height; y++) {
				free(jbxvt.scr.s1.text[y]);
				free(jbxvt.scr.s2.text[y]);
				free(jbxvt.scr.s1.rend[y]);
				free(jbxvt.scr.s2.rend[y]);
			}
			free((void *)jbxvt.scr.s1.text);
			free((void *)jbxvt.scr.s2.text);
			free((void *)jbxvt.scr.s1.rend);
			free((void *)jbxvt.scr.s2.rend);
		}
		jbxvt.scr.s1.text = s1;
		jbxvt.scr.s2.text = s2;
		jbxvt.scr.s1.rend = r1;
		jbxvt.scr.s2.rend = r2;

		jbxvt.scr.chars.width = cw;
		jbxvt.scr.chars.height = ch;
		jbxvt.scr.pixels.width = width;
		jbxvt.scr.pixels.height = height;
		jbxvt.scr.s1.tmargin = 0;
		jbxvt.scr.s1.bmargin = jbxvt.scr.chars.height - 1;
		jbxvt.scr.s1.decom = 0;
		jbxvt.scr.s1.wrap_next = 0;
		jbxvt.scr.s2.tmargin = 0;
		jbxvt.scr.s2.bmargin = jbxvt.scr.chars.height - 1;
		jbxvt.scr.s2.decom = 0;
		jbxvt.scr.s2.wrap_next = 0;
		scr_start_selection(0,0,CHAR);
	}
	tty_set_size(jbxvt.scr.chars.width,jbxvt.scr.chars.height);

	if (jbxvt.scr.current->col >= jbxvt.scr.chars.width)
		jbxvt.scr.current->col = jbxvt.scr.chars.width - 1;
	if (jbxvt.scr.current->row >= jbxvt.scr.chars.height)
		jbxvt.scr.current->row = jbxvt.scr.chars.height - 1;
	sbar_show(jbxvt.scr.chars.height + jbxvt.scr.sline.top - 1, jbxvt.scr.offset,
		jbxvt.scr.offset + jbxvt.scr.chars.height - 1);
	repaint(0,jbxvt.scr.chars.height - 1,0,jbxvt.scr.chars.width - 1);
	cursor();
}

