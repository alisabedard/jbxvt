#ifndef JBXVT_SCROLL_H
#define JBXVT_SCROLL_H

#include <stdint.h>

void scroll(const uint8_t row1, const uint8_t row2,
	const int8_t count);

void scroll1(uint8_t count);

#endif//!JBXVT_SCROLL_H
