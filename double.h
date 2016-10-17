// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_DOUBLE_H
#define JBXVT_DOUBLE_H
#include <stdbool.h>
#include <stdint.h>
void jbxvt_set_double_width_line(const bool is_dwl);
// Generate a double-width string.  Free the result!
uint8_t * jbxvt_get_double_width_string(uint8_t * in_str, uint16_t * len);
#endif//!JBXVT_DOUBLE_H
