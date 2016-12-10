/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#undef DEBUG
#include "handle_sgr.h"
#include "color.h"
#include "libjb/log.h"
#include "libjb/macros.h"
#include "paint.h"
#include "rstyle.h"
// Convert 3 bit color to 9 bit color, store at offset
__attribute__((cold))
static void encode_rgb(uint8_t color, uint8_t offset)
{
	// 0xe0 masks the most significant 3 bits
	const uint32_t val = (color & 0xe0) << offset >> 4;
	LOG("encode_rgb(color: %d, offset: %d) val: 0x%x", color, offset, val);
	jbxvt_add_rstyle(val);
}
/* c must be uint32_t to allow for shift and OR with rstyle. */
static void sgrc(const uint32_t c, const bool fg)
{
	const uint8_t o = JB_LIKELY(fg) ? 7 : 16;
	jbxvt_del_rstyle(0777 << o);
	jbxvt_add_rstyle((fg ? JBXVT_RS_FG_INDEX
		: JBXVT_RS_BG_INDEX) | c << o);
}
static bool rgb_or_index(int32_t arg, bool * restrict either,
	bool * restrict index, bool * restrict rgb, const bool is_fg)
{
	if (JB_LIKELY(!*either))
		return false;
	*either = false;
	const bool i = arg != 2;
	*(i?index:rgb) = true;
	jbxvt_add_rstyle(JB_LIKELY(i) ? (is_fg ? JBXVT_RS_FG_INDEX
		: JBXVT_RS_BG_INDEX) : (is_fg ? JBXVT_RS_FG_RGB
		: JBXVT_RS_BG_RGB));
	return true;
}
// continue if true
static bool handle_color_encoding(const int32_t arg, const bool is_fg,
	bool * restrict index_mode, bool * restrict rgb_mode)
{
	static uint8_t rgb_count;
	if (*index_mode) {
		sgrc(arg, is_fg);
		// exit mode after handling index
		*index_mode = false;
		return true;
	} else if (JB_UNLIKELY(*rgb_mode)) {
		const uint8_t o = is_fg ? 0 : 9;
		encode_rgb(arg, 12 - rgb_count * 3 + o);
		// exit mode after 3 colors
		if (++rgb_count > 2)
			*rgb_mode = (rgb_count = 0);
		return true;
	}
	return false;
}
void jbxvt_handle_sgr(xcb_connection_t * xc,
	struct JBXVTToken * restrict token)
{
	bool fg_rgb_or_index = false;
	bool bg_rgb_or_index = false;
	bool fg_rgb_mode = false;
	bool fg_index_mode = false;
	bool bg_rgb_mode = false;
	bool bg_index_mode = false;
	for (uint_fast8_t i = 0; i < token->nargs; ++i) {
		LOG("jbxvt_handle_sgr: arg[%d]: %d", i, token->arg[i]);
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
		case 1 :
			jbxvt_add_rstyle(JBXVT_RS_BOLD);
			break;
		case 2: // faint foreground
			sgrc(250, true);
			break;
		case 3:
			jbxvt_add_rstyle(JBXVT_RS_ITALIC);
			break;
		case 4 :
			jbxvt_add_rstyle(JBXVT_RS_UNDERLINE);
			break;
		case 5 :
		case 6: // sub for rapidly blinking
			jbxvt_add_rstyle(JBXVT_RS_BLINK);
			break;
		case 7: // Image negative
			jbxvt_add_rstyle(JBXVT_RS_RVID);
			break;
		case 8: // Invisible text
			jbxvt_add_rstyle(JBXVT_RS_INVISIBLE);
			break;
		case 9: // crossed out
			jbxvt_add_rstyle(JBXVT_RS_CROSSED_OUT);
			break;
		case 17: // Alt font
			jbxvt_add_rstyle(JBXVT_RS_BOLD);
			break;
		case 21: // doubly underlined
			jbxvt_add_rstyle(JBXVT_RS_DOUBLE_UNDERLINE);
			break;
		case 23: // Not italic
			jbxvt_del_rstyle(JBXVT_RS_ITALIC);
			break;
		case 24: // Underline none
			jbxvt_del_rstyle(JBXVT_RS_UNDERLINE);
			jbxvt_del_rstyle(JBXVT_RS_DOUBLE_UNDERLINE);
			break;
		case 27: // Image positive ( rvid off)
			jbxvt_del_rstyle(JBXVT_RS_RVID);
			break;
		case 28: // not invisible
			jbxvt_del_rstyle(JBXVT_RS_INVISIBLE);
			break;
		case 29: // Not crossed out
			jbxvt_del_rstyle(JBXVT_RS_CROSSED_OUT);
			break;
		case 26: // reserved
			break;
		case 38: // extended fg colors
			fg_rgb_or_index = true;
			break;
		case 39: // foreground reset, white
			sgrc(017, true);
			break;
		case 48: // extended bg colors
			bg_rgb_or_index = true;
			break;
		case 49: // background reset, black
			sgrc(0, false);
			break;
		case 30: sgrc(0, true); break; // black
		case 90: sgrc(010, true); break; // grey
		case 31: case 91: sgrc(011, true); break;
		case 32: case 92: sgrc(012, true); break;
		case 33: case 93: sgrc(013, true); break;
		case 34: case 94: sgrc(014, true); break;
		case 35: case 95: sgrc(015, true); break;
		case 36: case 96: sgrc(016, true); break;
		case 37: case 97: sgrc(017, true); break;
		case 40: sgrc(0, false); break;
		case 100: sgrc(010, false); break;
		case 41: case 101: sgrc(011, false); break;
		case 42: case 102: sgrc(012, false); break;
		case 43: case 103: sgrc(013, false); break;
		case 44: case 104: sgrc(014, false); break;
		case 45: case 105: sgrc(015, false); break;
		case 46: case 106: sgrc(016, false); break;
		case 47: case 107: sgrc(017, false); break;
		default:
			LOG("unhandled style %d", token->arg[i]);
		}
	}
}
