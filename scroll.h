#ifndef JBXVT_SCROLL_H
#define JBXVT_SCROLL_H

#include <stdint.h>

void scroll(const uint8_t row1, const uint8_t row2,
	const int16_t count);

void scroll1(uint16_t count);

#endif//!JBXVT_SCROLL_H
