// Copyright 2016-2017, Jeffrey E. Bedard
#include "utf.h"
#include "JBXVTToken.h"
#include "cmdtok.h"
#include "libjb/log.h"
// Returns the number of additional bytes in the UTF encoded character
__attribute__((const))
uint8_t jbxvt_get_utf_bytes(const uint8_t c)
{
	if ((c & 0xf0) == 0xf0)
		return 3;
	if ((c & 0xe0) == 0xe0)
		return 2;
	if ((c & 0xc0) == 0xc0)
		return 1;
	return 0;
}
// Handle 4-byte characters
void jbxvt_parse_utf8_3(xcb_connection_t * xc,
	struct JBXVTToken * restrict tk, int_fast16_t c)
{
	LOG("utf8_3()\t0x%x\n", (unsigned int)c);
	c = jbxvt_pop_char(xc, c); // 2
	LOG("\t0x%x\n", (unsigned int)c);
	c = jbxvt_pop_char(xc, c); // 3
	LOG("\t0x%x\n", (unsigned int)c);
	switch (c) {
	default:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
// Handle 3-byte characters
void jbxvt_parse_utf8_2(xcb_connection_t * xc,
	struct JBXVTToken * restrict tk, int_fast16_t c)
{
	LOG("utf8_t()\t0x%x\n", (unsigned int)c);
	int_fast16_t c2 = jbxvt_pop_char(xc, c); // take next byte
	LOG("\t0x%x\n", (unsigned int)c2);
	switch (c) {
	case 0x80:
		switch (c2) {
		case 0x90:
			jbxvt_push_char('-');
			break;
		default:
			goto tk_null;
		}
		break;
	case 0x94:
		switch (c2) {
		case 0x80:
			jbxvt_push_char('-');
			break;
		case 0x82:
			jbxvt_push_char('|');
			break;
		case 0xac:
			jbxvt_push_char('+');
			break;
		default:
			goto tk_null;
		}
		break;
	case 0x96:
		switch (c2) {
		case 0xbd: // FIXME:  white down pointing triangle
			jbxvt_push_char('V');
			break;
		default:
			goto tk_null;
		}
	default:
tk_null:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
// Handle 2-byte characters
void jbxvt_parse_utf8_1(struct JBXVTToken * restrict tk, int_fast16_t c)
{
	LOG("utf8_1()\t0x%x\n", (unsigned int)c);
	switch (c) {
	default:
		tk->type = JBXVT_TOKEN_NULL;
	}
}
// Handle 1-byte characters
void jbxvt_parse_utf8_0(struct JBXVTToken * restrict tk, int_fast16_t c)
{
	tk->type = JBXVT_TOKEN_CHAR;
	tk->tk_char = c;
}

