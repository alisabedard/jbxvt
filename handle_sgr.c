/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "handle_sgr.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "paint.h"
// Convert 3 bit color to 9 bit color, store at offset
__attribute__((cold))
static void encode_rgb(uint8_t color, uint8_t offset)
{
	color >>= 5;
	color <<= 1;
	jbxvt.scr.rstyle |= color << offset;
}
/* c must be uint32_t to allow for shift and OR with rstyle. */
static void sgrc(const uint32_t c, const bool fg)
{
	const uint8_t o = JB_LIKELY(fg) ? 7 : 16;
	jbxvt.scr.rstyle &= ~(0777<<o);
	jbxvt.scr.rstyle |= (fg ? JBXVT_RS_FG_INDEX : JBXVT_RS_BG_INDEX) | c << o;
}
static bool rgb_or_index(int32_t arg, bool * restrict either,
	bool * restrict index, bool * restrict rgb, const bool is_fg)
{
	if (JB_LIKELY(!*either))
		return false;
	*either = false;
	const bool i = arg != 2;
	*(i?index:rgb) = true;
	jbxvt.scr.rstyle |= JB_LIKELY(i) ? (is_fg ? JBXVT_RS_FG_INDEX
		: JBXVT_RS_BG_INDEX) : (is_fg ? JBXVT_RS_FG_RGB
		: JBXVT_RS_BG_RGB);
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
#define SGRFG(c) sgrc(c, true)
#define SGRBG(c) sgrc(c, false)
void jbxvt_handle_sgr(xcb_connection_t * xc,
	struct Token * restrict token)
{
	bool fg_rgb_or_index = false;
	bool bg_rgb_or_index = false;
	bool fg_rgb_mode = false;
	bool fg_index_mode = false;
	bool bg_rgb_mode = false;
	bool bg_index_mode = false;
	for (uint_fast8_t i = 0; i < token->nargs; ++i) {
#ifdef DEBUG_SGR
		LOG("jbxvt_handle_sgr: arg[%d]: %d", i, token->arg[i]);
#endif//DEBUG_SGR
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
			jbxvt.scr.rstyle = JBXVT_RS_NONE;
			jbxvt_set_fg(xc, NULL);
			jbxvt_set_bg(xc, NULL);
			break;
		case 1 :
			jbxvt.scr.rstyle |= JBXVT_RS_BOLD;
			break;
		case 2: // faint
			SGRFG(250);
			break;
		case 3:
			jbxvt.scr.rstyle |= JBXVT_RS_ITALIC;
			break;
		case 4 :
			jbxvt.scr.rstyle |= JBXVT_RS_UNDERLINE;
			break;
		case 5 :
		case 6: // sub for rapidly blinking
			jbxvt.scr.rstyle |= JBXVT_RS_BLINK;
			break;
		case 7: // Image negative
			jbxvt.scr.rstyle |= JBXVT_RS_RVID;
			break;
		case 8: // Invisible text
			jbxvt.scr.rstyle |= JBXVT_RS_INVISIBLE;
			break;
		case 9: // crossed out
			jbxvt.scr.rstyle |= JBXVT_RS_CROSSED_OUT;
			break;
		case 17: // Alt font
			jbxvt.scr.rstyle |= JBXVT_RS_BOLD;
			break;
		case 21: // doubly underlined
			jbxvt.scr.rstyle |= JBXVT_RS_DOUBLE_UNDERLINE;
			break;
		case 23: // Not italic
			jbxvt.scr.rstyle &= ~JBXVT_RS_ITALIC;
			break;
		case 24: // Underline none
			jbxvt.scr.rstyle &= ~JBXVT_RS_UNDERLINE;
			jbxvt.scr.rstyle &= ~JBXVT_RS_DOUBLE_UNDERLINE;
			break;
		case 27: // Image positive ( rvid off)
			jbxvt.scr.rstyle &= ~JBXVT_RS_RVID;
			break;
		case 28: // not invisible
			jbxvt.scr.rstyle &= ~JBXVT_RS_INVISIBLE;
			break;
		case 29: // Not crossed out
			jbxvt.scr.rstyle &= ~JBXVT_RS_CROSSED_OUT;
			break;
		case 26: // reserved
			break;
		case 38: // extended fg colors
			fg_rgb_or_index = true;
			break;
		case 39: // foreground reset, white
			SGRFG(017);
			break;
		case 48: // extended bg colors
			bg_rgb_or_index = true;
			break;
		case 49: // background reset, black
			SGRBG(0);
			break;
		case 30: SGRFG(0); break; // black
		case 90: SGRFG(010); break; // grey
		case 31: case 91: SGRFG(011); break;
		case 32: case 92: SGRFG(012); break;
		case 33: case 93: SGRFG(013); break;
		case 34: case 94: SGRFG(014); break;
		case 35: case 95: SGRFG(015); break;
		case 36: case 96: SGRFG(016); break;
		case 37: case 97: SGRFG(017); break;
		case 40: SGRBG(0); break;
		case 100: SGRBG(010); break;
		case 41: case 101: SGRBG(011); break;
		case 42: case 102: SGRBG(012); break;
		case 43: case 103: SGRBG(013); break;
		case 44: case 104: SGRBG(014); break;
		case 45: case 105: SGRBG(015); break;
		case 46: case 106: SGRBG(016); break;
		case 47: case 107: SGRBG(017); break;
		default:
			LOG("unhandled style %d", token->arg[i]);
		}
	}
}
