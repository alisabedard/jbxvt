#include "handle_sgr.h"
//#define DEBUG
#include "color.h"
#include "log.h"
#include "screen.h"

// Convert 3 bit color to 9 bit color, store at offset
static void encode_rgb(uint8_t color, uint8_t offset)
{
	scr_style(((color>>5)&07)<<offset);
}

static void sgrc(const uint8_t c, const bool fg)
{
	const uint8_t o = fg ? 7 : 16;
	jbxvt.scr.rstyle &= ~(0777<<o);
	jbxvt.scr.rstyle |= (fg?RS_FG_INDEX:RS_BG_INDEX);
	uint32_t r = c;
	jbxvt.scr.rstyle |= r << o;
}

static bool rgb_or_index(int32_t arg, bool * restrict either,
	bool * restrict index, bool * restrict rgb)
{
	if (!*either)
		return false;
	*either = false;
	if (arg == 5) {
		scr_style(RS_FG_INDEX);
		*index = true;
	} else if (arg == 2) {
		scr_style(RS_FG_RGB);
		*rgb = true;
	}
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
	} else if (unlikely(*rgb_mode)) {
		const uint8_t o = is_fg ? 0 : 9;
		jbputs("FIXME: test fg color rgb mode\n");
		switch(rgb_count) {
		case 0: // red
			encode_rgb(arg, 12 + o);
			LOG("red: %d", arg);
			break;
		case 1: // green
			encode_rgb(arg, 9 + o);
			LOG("green: %d", arg);
			break;
		case 2: // blue
			encode_rgb(arg, 6 + o);
			LOG("blue: %d", arg);
			break;
		}
		// exit mode after 3 colors
		if (++rgb_count > 2) {
			*rgb_mode = false;
			rgb_count = 0;
		}
		return true;
	}
	return false;
}

#define SGRFG(c) sgrc(c, true)
#define SGRBG(c) sgrc(c, false)

void handle_sgr(struct tokenst * restrict token)
{
	if (token->tk_nargs == 0) {
		scr_style(RS_NONE);
		return;
	}
	bool fg_rgb_or_index = false;
	bool bg_rgb_or_index = false;
	bool fg_rgb_mode = false;
	bool fg_index_mode = false;
	bool bg_rgb_mode = false;
	bool bg_index_mode = false;
	for (uint_fast8_t i = 0; i < token->tk_nargs; ++i) {
#ifdef DEBUG_SGR
		LOG("handle_sgr: tk_arg[%d]: %d", i, token->tk_arg[i]);
#endif//DEBUG_SGR
		if (rgb_or_index(token->tk_arg[i], &fg_rgb_or_index,
			&fg_index_mode, &fg_rgb_mode))
			  continue;
		if (rgb_or_index(token->tk_arg[i], &bg_rgb_or_index,
			&bg_index_mode, &bg_rgb_mode))
			  continue;
		if (handle_color_encoding(token->tk_arg[i], true,
			&fg_index_mode, &fg_rgb_mode))
			  continue;
		if (handle_color_encoding(token->tk_arg[i], false,
			&bg_index_mode, &bg_rgb_mode))
			  continue;
		switch (token->tk_arg[i]) {
		case 0 :
			scr_style(RS_NONE);
			set_fg(NULL);
			set_bg(NULL);
			break;
		case 1 :
			scr_style(RS_BOLD);
			break;
		case 3:
			scr_style(RS_ITALIC);
			break;
		case 4 :
			scr_style(RS_ULINE);
			break;
		case 5 :
		case 6: // sub for rapidly blinking
			scr_style(RS_BLINK);
			break;
		case 7: // Image negative
			scr_style(RS_RVID);
			break;
		case 8: // Invisible text
			scr_style(RS_INVISIBLE);
			break;
		case 17: // Alt font
			scr_style(RS_BOLD);
			break;
		case 23: // Not italic
		case 24: // Underline none
		case 27: // Image positive
			scr_style(RS_NONE);
			break;
				case 38: // extended fg colors
			fg_rgb_or_index = true;
			break;
		case 26: // reserved
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
		case 31:
		case 91: SGRFG(011); break;
		case 32:
		case 92: SGRFG(012); break;
		case 33:
		case 93: SGRFG(013); break;
		case 34:
		case 94: SGRFG(014); break;
		case 35:
		case 95: SGRFG(015); break;
		case 36:
		case 96: SGRFG(016); break;
		case 37:
		case 97: SGRFG(017); break;
		case 40: SGRBG(0); break;
		case 100: SGRBG(010); break;
		case 41:
		case 101: SGRBG(011); break;
		case 42:
		case 102: SGRBG(012); break;
		case 43:
		case 103: SGRBG(013); break;
		case 44:
		case 104: SGRBG(014); break;
		case 45:
		case 105: SGRBG(015); break;
		case 46:
		case 106: SGRBG(016); break;
		case 47:
		case 107: SGRBG(017); break;
		default:
			LOG("unhandled style %d", token->tk_arg[i]);
		}
	}
}


