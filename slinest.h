#ifndef SLINEST_H
#define SLINEST_H

#include <stdint.h>

//  structure describing a saved line
struct slinest {
	uint8_t *sl_text;	/* the text of the line */
	uint32_t *sl_rend;	/* the rendition style */
	uint8_t sl_length;	/* length of the line */
	uint8_t canary;		// corruption check
};

#endif//!SLINEST_H
