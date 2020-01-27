/*  Copyright 2017-2020, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey,
    University of Kent at Canterbury. */
//#undef DEBUG
#define LOG_LEVEL 3
#include "sgr.h"
#include "JBXVTRenderStyle.h"
#include "JBXVTToken.h"
#include "color.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "rstyle.h"
// Convert 3 bit color to 9 bit color, store at offset
__attribute__((cold))
static void encode_rgb(uint8_t color, uint8_t offset)
{
	// 0xe0 masks the most significant 3 bits
	const uint32_t val = (uint32_t)((color & 0xe0) << offset >> 4);
	LOG("encode_rgb(color: %d, offset: %d) val: 0x%x", color, offset,
		val);
	jbxvt_set_rstyle(jbxvt_get_rstyle()|val);
}
/* c must be uint32_t to allow for shift and OR with rstyle. */
static void sgrc(const uint32_t c, const bool fg)
{
	uint8_t const o = JB_LIKELY(fg) ? 7 : 16;
        rstyle_t const mask=(rstyle_t)0777<<o;
        jbxvt_set_rstyle(jbxvt_get_rstyle()&~mask);
        jbxvt_add_rstyle(fg?JBXVT_RS_FG_INDEX:JBXVT_RS_BG_INDEX);
        jbxvt_set_rstyle(jbxvt_get_rstyle()|c<<o);
}
static bool rgb_or_index(int32_t arg, bool * restrict either,
	bool * restrict index, bool * restrict rgb, const bool is_fg)
{
	const bool rval = *either;
	if (JB_UNLIKELY(rval)) {
		*either = false;
		const bool i = arg != 2;
                *(i?index:rgb) = true;
		jbxvt_add_rstyle(JB_LIKELY(i) ? (is_fg ? JBXVT_RS_FG_INDEX
			: JBXVT_RS_BG_INDEX) : (is_fg ? JBXVT_RS_FG_RGB
			: JBXVT_RS_BG_RGB));
	}
	return rval;
}
// continue if true
static bool handle_color_encoding(const int32_t arg, const bool is_fg,
	bool * restrict index_mode, bool * restrict rgb_mode)
{
	static uint8_t rgb_count;
	bool rval = false;
	if (*index_mode) {
		sgrc((uint32_t)arg, is_fg);
		// exit mode after handling index
		*index_mode = false;
		rval = true;
	} else if (JB_UNLIKELY(*rgb_mode)) {
		const uint8_t o = is_fg ? 0 : 9;
		encode_rgb(arg, 12 - rgb_count * 3 + o);
		// exit mode after 3 colors
		if (++rgb_count > 2)
			*rgb_mode = (rgb_count = 0);
		rval = true;
	}
	return rval;
}
void jbxvt_handle_sgr(xcb_connection_t * xc,
	struct JBXVTToken * restrict token)
{
	bool fg_rgb_or_index = false, bg_rgb_or_index = false, fg_rgb_mode =
		false, fg_index_mode = false, bg_rgb_mode = false,
		bg_index_mode = false;
	for (uint_fast8_t i = 0; i < token->nargs; ++i) {
#if LOG_LEVEL > 5
		LOG("jbxvt_handle_sgr: arg[%d]: %d", i, token->arg[i]);
#endif//LOG_LEVEL>5
		if (rgb_or_index(token->arg[i], &fg_rgb_or_index,
			&fg_index_mode, &fg_rgb_mode, true))
			  continue;
		if (rgb_or_index(token->arg[i], &bg_rgb_or_index,
			&bg_index_mode, &bg_rgb_mode, false))
			  continue;
		if (handle_color_encoding(token->arg[i], true,
			&fg_index_mode, &fg_rgb_mode))
			  continue;
		if (handle_color_encoding(token->arg[i], false,
			&bg_index_mode, &bg_rgb_mode))
			  continue;
		switch (token->arg[i]) {
		case 0 : // reset
			jbxvt_zero_rstyle();
			jbxvt_set_fg(xc, NULL);
			jbxvt_set_bg(xc, NULL);
			break;
#include "sgr_cases.c"
		case 24: // Underline none
			jbxvt_del_rstyle(JBXVT_RS_UNDERLINE);
			jbxvt_del_rstyle(JBXVT_RS_DOUBLE_UNDERLINE);
			break;
		case 26: // reserved
			break;
		case 38: // extended fg colors
			fg_rgb_or_index = true;
			break;
		case 48: // extended bg colors
			bg_rgb_or_index = true;
			break;
		default:
			LOG("unhandled style %d", token->arg[i]);
		}
	}
}
