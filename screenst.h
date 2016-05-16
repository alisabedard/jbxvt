#ifndef SCREENST_H
#define SCREENST_H

#include <stdbool.h>
#include <stdint.h>

/*  Structure describing the current state of the screen.
 */
struct screenst {
	unsigned char **text;	/* backup copy of screen->text */
	uint32_t **rend; // rendition styles
	int row;	/* cursor position */
	int col;	/* ditto */
	int tmargin;	/* top scroll margin */
	int bmargin;	/* bottom scroll margin */
	bool decom:1;	/* origin mode flag */
	bool wrap:1;	/* auto-wrap flag */
	bool wrap_next:1;	/* wrap before the next printed character */
	bool insert:1;	/* insert mode flag */
};

#endif//!SCREENST_H
