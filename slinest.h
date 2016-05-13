#ifndef SLINEST_H
#define SLINEST_H

#include <stdint.h>

//  structure describing a saved line
struct slinest {
	unsigned char *sl_text;	/* the text of the line */
	uint32_t *sl_rend;	/* the rendition style */
	int sl_length;	/* length of the line */
};

#endif//!SLINEST_H
