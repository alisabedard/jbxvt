/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef SBAR_H
#define SBAR_H

#include <stdint.h>

void sbar_reset(void);
void sbar_show(uint16_t length, const int16_t low,
	const int16_t high);

#endif//!SBAR_H
