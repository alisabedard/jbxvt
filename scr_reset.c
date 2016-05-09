#include "scr_reset.h"

#include "cursor.h"
#include "global.h"
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
	int16_t cw = (width - 2 * MARGIN) / fwidth;
	int16_t ch = (height - 2 * MARGIN) / fheight;

	if (screen->text == NULL || cw != cwidth || ch != cheight) {

		offset = 0;
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
		if (screen1.text != NULL) {

			/*  Now fill up the screen from the old screen
			    and saved lines.  */
			if (screen1.row >= ch) {
				/* scroll up to save any lines that will be lost.
				 */
				scroll1(screen1.row - ch + 1);
				screen1.row = ch - 1;
			}
			/* calculate working no. of lines.
			 */
			i = sline_top + screen1.row + 1;
			j = i > ch ? ch - 1 : i - 1;
			i = screen1.row;
			screen1.row = j;
			onscreen = 1;
			for (; j >= 0; j--) {
				if (onscreen) {
					n = cw < cwidth ? cw : cwidth;
					memcpy(s1[j],screen1.text[i],n);
					memcpy(s2[j],screen2.text[i],n);
					memcpy(r1[j],screen1.rend[i],n);
					memcpy(r2[j],screen2.rend[i],n);
					s1[j][cw] = screen1.text[i][cwidth];
					s2[j][cw] = screen2.text[i][cwidth];
					r1[j][cw] = screen1.rend[i][cwidth];
					r2[j][cw] = screen2.rend[i][cwidth];
					i--;
					if (i < 0) {
						onscreen = 0;
						i = 0;
					}
				} else {
					if (i >= sline_top)
						break;
					sl = sline[i];
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
			for (j = i; j < sline_top; j++)
				sline[j - i] = sline[j];
			for (j = sline_top - i; j < sline_top; j++)
				sline[j] = NULL;
			sline_top -= i;

			for (y = 0; y < cheight; y++) {
				free(screen1.text[y]);
				free(screen2.text[y]);
				free(screen1.rend[y]);
				free(screen2.rend[y]);
			}
			free((void *)screen1.text);
			free((void *)screen2.text);
			free((void *)screen1.rend);
			free((void *)screen2.rend);
		}
		screen1.text = s1;
		screen2.text = s2;
		screen1.rend = r1;
		screen2.rend = r2;

		cwidth = cw;
		cheight = ch;
		pwidth = width;
		pheight = height;
		screen1.tmargin = 0;
		screen1.bmargin = cheight - 1;
		screen1.decom = 0;
		screen1.wrap_next = 0;
		screen2.tmargin = 0;
		screen2.bmargin = cheight - 1;
		screen2.decom = 0;
		screen2.wrap_next = 0;
		scr_start_selection(0,0,CHAR);
	}
	tty_set_size(cwidth,cheight);

	if (screen->col >= cwidth)
		screen->col = cwidth - 1;
	if (screen->row >= cheight)
		screen->row = cheight - 1;
	sbar_show(cheight + sline_top - 1, offset, offset + cheight - 1);
	repaint(0,cheight - 1,0,cwidth - 1);
	cursor();
}

