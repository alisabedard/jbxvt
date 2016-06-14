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

static inline void sgrfg(uint8_t c)
{
	sgrc(c, true);
}

static inline void sgrbg(uint8_t c)
{
	sgrc(c, false);
}

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
	uint8_t fg_rgb_count = 0;
	uint8_t bg_rgb_count = 0;
	for (uint_fast8_t i = 0; i < token->tk_nargs; ++i) {
#ifdef DEBUG_SGR
		LOG("handle_sgr: tk_arg[%d]: %d", i, token->tk_arg[i]);
#endif//DEBUG_SGR
		if (fg_rgb_or_index) {
			fg_rgb_or_index = false;
			switch(token->tk_arg[i]) {
			case 5: // index mode
				scr_style(RS_FG_INDEX);
				fg_index_mode = true;
				continue;
			case 2: // rgb mode
				scr_style(RS_FG_RGB);
				fg_rgb_mode = true;
				continue;
			}
		}
		if (bg_rgb_or_index) {
			bg_rgb_or_index = false;
			switch(token->tk_arg[i]) {
			case 5: // index mode
				scr_style(RS_BG_INDEX);
				bg_index_mode = true;
				continue;
			case 2: // rgb mode
				scr_style(RS_BG_RGB);
				bg_rgb_mode = true;
				continue;
			}
		}
		if (fg_index_mode) {
			LOG("fg index MUSTMATCH: %d\n", token->tk_arg[i]);
			const uint8_t j = token->tk_arg[i];
			jbxvt.scr.rstyle &= ~(0777<<7);
			jbxvt.scr.rstyle |= j<<7;
			// exit mode after handling index
			fg_index_mode = false;
			continue;
		} else if (fg_rgb_mode) {
			jbputs("FIXME: test fg color rgb mode\n");
			switch(fg_rgb_count) {
			case 0: // red
				encode_rgb(token->tk_arg[i], 12);
				LOG("red");
				break;
			case 1: // green
				encode_rgb(token->tk_arg[i], 9);
				LOG("green");
				break;
			case 2: // blue
				encode_rgb(token->tk_arg[i], 6);
				LOG("blue");
				break;
			}
			// exit mode after 3 colors
			if (++fg_rgb_count > 2) {
				fg_rgb_mode = false;
			}
			continue;
		}
		if (bg_index_mode) {
			jbputs("FIXME: implement bg color index mode\n");
			dprintf(STDERR_FILENO, "index: %d\n", token->tk_arg[i]);
			const uint8_t j = token->tk_arg[i];
			jbxvt.scr.rstyle &= ~(0777<<7);
			jbxvt.scr.rstyle |= j<<7;
			// exit mode after handling index
			bg_index_mode = false;
			continue;
		} else if (bg_rgb_mode) {
			jbputs("FIXME: test bg color rgb mode\n");
			switch(bg_rgb_count) {
			case 0: // red
				encode_rgb(token->tk_arg[i], 21);
				jbputs("red\n");
				break;
			case 1: // green
				encode_rgb(token->tk_arg[i], 18);
				jbputs("green\n");
				break;
			case 2: // blue
				encode_rgb(token->tk_arg[i], 15);
				jbputs("blue\n");
				break;
			}
			// exit mode after 3 colors
			if (++bg_rgb_count > 2) {
				bg_rgb_mode = false;
			}
			continue;
		}
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
			sgrfg(017);
			break;
		case 48: // extended bg colors
			bg_rgb_or_index = true;
			break;
		case 49: // background reset, black
			sgrbg(0);
			break;
		case 30: sgrfg(0); break; // black
		case 90: sgrfg(010); break; // grey
		case 31:
		case 91: sgrfg(011); break;
		case 32:
		case 92: sgrfg(012); break;
		case 33:
		case 93: sgrfg(013); break;
		case 34:
		case 94: sgrfg(014); break;
		case 35:
		case 95: sgrfg(015); break;
		case 36:
		case 96: sgrfg(016); break;
		case 37:
		case 97: sgrfg(017); break;
		case 40: sgrbg(0); break;
		case 100: sgrbg(010); break;
		case 41:
		case 101: sgrbg(011); break;
		case 42:
		case 102: sgrbg(012); break;
		case 43:
		case 103: sgrbg(013); break;
		case 44:
		case 104: sgrbg(014); break;
		case 45:
		case 105: sgrbg(015); break;
		case 46:
		case 106: sgrbg(016); break;
		case 47:
		case 107: sgrbg(017); break;
		default:
			LOG("unhandled style %d", token->tk_arg[i]);
		}
	}
}


