// Copyright 2016-2017, Jeffrey E. Bedard
#ifndef JBXVT_UTF_H
#define JBXVT_UTF_H
#include <xcb/xcb.h>
struct JBXVTToken;
// Returns the number of additional bytes in the UTF encoded character
uint8_t jbxvt_get_utf_bytes(const uint8_t c) __attribute__((const));
// Handle 4-byte characters
void jbxvt_parse_utf8_3(xcb_connection_t * xc,
    struct JBXVTToken * restrict tk, int_fast16_t c);
// Handle 3-byte characters
void jbxvt_parse_utf8_2(xcb_connection_t * xc,
    struct JBXVTToken * restrict tk, int_fast16_t c);
// Handle 2-byte characters
void jbxvt_parse_utf8_1(struct JBXVTToken * restrict tk, int_fast16_t c);
// Handle 1-byte characters
void jbxvt_parse_utf8_0(struct JBXVTToken * restrict tk, int_fast16_t c);
#endif//!JBXVT_UTF_H
