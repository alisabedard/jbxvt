/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "scroll_up.h"

#include "jbxvt.h"
#include "log.h"
#include "screen.h"

#include <stdlib.h>
#include <string.h>

static uint16_t get_count(const uint16_t count, uint16_t * restrict n)
{
	/*  If count is greater than MAX_SCROLL then scroll in
	 *  installments.  */
	*n = count > MAX_SCROLL ? MAX_SCROLL : count;
	return count - *n;
}

static void shift_sline_data(const uint16_t n)
{
	for (int16_t i = jbxvt.scr.sline.max - n - 1; i >= 0; i--) {
		jbxvt.scr.sline.data[i + n] = jbxvt.scr.sline.data[i];
	}
}

// free lines that scroll off the top of the screen.
static void free_invisible_lines(const uint16_t n)
{
	LOG("free_invisible_lines(%d)", n);
	for (uint16_t i = 1; i <= n; i++) {
		struct slinest *sl
			= jbxvt.scr.sline.data[jbxvt.scr.sline.max - i];
		if(!sl) continue;
		free(sl->sl_text);
		if(sl->sl_rend)
			  free(sl->sl_rend);
		free(sl);
	}
}

static int16_t get_line_width(const uint16_t i)
{
	if(!jbxvt.scr.s1.text[i])
		  return 0; // line does not exist
	int16_t w = jbxvt.scr.chars.width - 1;
	while(w >= 0 && !jbxvt.scr.s1.text[i][w])
		  w++;
	return w + 1;
}

/*  Scroll jbxvt.scr.s1 up by count lines saving lines as needed.  This is used
 *  after the screen size is reduced.
 */
void scroll_up(uint16_t count) // unsigned as only going up
{
	LOG("scroll1(%d)", count);
	while (count > 0) {
		uint16_t n;
		count=get_count(count, &n);
		free_invisible_lines(n);
		shift_sline_data(n);
		for (uint16_t i = 0; i < n; i++) {
			uint32_t * r = jbxvt.scr.s1.rend[i];
			int16_t j = get_line_width(i);
			struct slinest * sl = (struct slinest *)
				malloc(sizeof(struct slinest));
			sl->sl_text = (uint8_t *)malloc(j + 1);
			if(!jbxvt.scr.s1.text[i])
				  abort();
			memcpy(sl->sl_text,jbxvt.scr.s1.text[i],j);
			sl->sl_text[j] = jbxvt.scr.s1.text[i]
				[jbxvt.scr.chars.width];
			if (r[jbxvt.scr.chars.width] == 0)
				sl->sl_rend = NULL;
			else {
				sl->sl_rend = malloc((j + 1)
					*sizeof(uint32_t));
				memcpy(sl->sl_rend,r,(j*sizeof(uint32_t)));
			}
			sl->sl_length = j;
			jbxvt.scr.sline.data[n - i - 1] = sl;
		}
		jbxvt.scr.sline.top += n;
		if (jbxvt.scr.sline.top > jbxvt.scr.sline.max)
			jbxvt.scr.sline.top = jbxvt.scr.sline.max;
		for (uint16_t j = n; j < jbxvt.scr.chars.height; j++) {
			jbxvt.scr.s1.text[j - n] = jbxvt.scr.s1.text[j];
			jbxvt.scr.s1.rend[j - n] = jbxvt.scr.s1.rend[j];
		}
	}
}

