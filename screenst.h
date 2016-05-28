#ifndef SCREENST_H
#define SCREENST_H

#include <stdbool.h>
#include <stdint.h>

/*  Structure describing the current state of the screen.
 */
struct screenst {
	uint8_t **text;		// backup copy of text
	uint32_t **rend;	// rendition styles
	Point margin; 		// scroll margins, top and bottom
	Point cursor;		// cursor position, row and column
	bool decom:1;		// origin mode flag
	bool wrap:1;		// auto-wrap flag
	bool wrap_next:1;	// wrap before the next printed character
	bool insert:1;		// insert mode flag
};

#endif//!SCREENST_H
