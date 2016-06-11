#include "handle_sgr.h"
#define DEBUG
#include "color.h"
#include "log.h"
#include "screen.h"

// Convert 8 bit color to 2 bit color, store at offset
static void encode_rgb(uint8_t color, uint8_t offset)
{
	scr_style(((color>>6)&0xf)<<offset);
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
		LOG("handle_sgr: tk_arg[%d]: %x", i, token->tk_arg[i]);
#endif//DEBUG_SGR
		if (fg_rgb_or_index) {
			fg_rgb_or_index = false;
			switch(token->tk_arg[i]) {
			case 2: // rgb mode
				scr_style(RS_FG_RGB);
				fg_rgb_mode = true;
				continue;
			case 5: // index mode
				scr_style(RS_FG_INDEX);
				fg_index_mode = true;
				continue;
			}
		}
		if (bg_rgb_or_index) {
			fg_rgb_or_index = false;
			switch(token->tk_arg[i]) {
			case 2: // rgb mode
				scr_style(RS_FG_RGB);
				bg_rgb_mode = true;
				continue;
			case 5: // index mode
				scr_style(RS_FG_INDEX);
				bg_index_mode = true;
				continue;
			}
		}
		if (fg_index_mode) {
			LOG("fg index: %d\n", token->tk_arg[i]);
			scr_style(token->tk_arg[i]<<6);
			// exit mode after handling index
			fg_index_mode = false;
			continue;
		} else if (fg_rgb_mode) {
			jbputs("FIXME: implement fg color rgb mode\n");
			switch(fg_rgb_count) {
			case 0: // red
				encode_rgb(token->tk_arg[i], 11);
				jbputs("red\n");
				break;
			case 1: // green
				encode_rgb(token->tk_arg[i], 9);
				jbputs("green\n");
				break;
			case 2: // blue
				encode_rgb(token->tk_arg[i], 7);
				jbputs("blue\n");
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
			scr_style(token->tk_arg[i]<<15);
			// exit mode after handling index
			bg_index_mode = false;
			continue;
		} else if (bg_rgb_mode) {
			jbputs("FIXME: implement bg color rgb mode\n");
			switch(bg_rgb_count) {
			case 0: // red
				encode_rgb(token->tk_arg[i], 20);
				jbputs("red\n");
				break;
			case 1: // green
				encode_rgb(token->tk_arg[i], 18);
				jbputs("green\n");
				break;
			case 2: // blue
				encode_rgb(token->tk_arg[i], 16);
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
		case 17: // Alt font
			scr_style(RS_BOLD);
			break;
		case 23: // Not italic
		case 24: // Underline none
		case 27: // Image positive
			scr_style(RS_NONE);
			break;
		case 30:
			scr_style(RS_F0);
			break;
		case 31:
			scr_style(RS_F1);
			break;
		case 32:
			scr_style(RS_F2);
			break;
		case 33:
			scr_style(RS_F3);
			break;
		case 34:
			scr_style(RS_F4);
			break;
		case 35:
			scr_style(RS_F5);
			break;
		case 36:
			scr_style(RS_F6);
			break;
		case 37:
			scr_style(RS_F7);
			break;
		case 26: // reserved
		case 38: // extended fg colors
			fg_rgb_or_index = true;
			break;
		case 39:
			scr_style(RS_FR);
			break;
			// BG Colors:
		case 40:
			scr_style(RS_B0);
			break;
		case 41:
			scr_style(RS_B1);
			break;
		case 42:
			scr_style(RS_B2);
			break;
		case 43:
			scr_style(RS_B3);
			break;
		case 44:
			scr_style(RS_B4);
			break;
		case 45:
			scr_style(RS_B5);
			break;
		case 46:
			scr_style(RS_B6);
			break;
		case 47:
			scr_style(RS_B7);
			break;
		case 48: // extended bg colors
			bg_rgb_or_index = true;
			break;
		case 49:
			scr_style(RS_BR);
			break;
			// Bright FG Colors:
		case 90:
			scr_style(RS_BF|RS_F0);
			break;
		case 91:
			scr_style(RS_BF|RS_F1);
			break;
		case 92:
			scr_style(RS_BF|RS_F2);
			break;
		case 93:
			scr_style(RS_BF|RS_F3);
			break;
		case 94:
			scr_style(RS_BF|RS_F4);
			break;
		case 95:
			scr_style(RS_BF|RS_F5);
			break;
		case 96:
			scr_style(RS_BF|RS_F6);
			break;
		case 97:
			scr_style(RS_BF|RS_F7);
			break;
			// Bright BG BColors:
		case 100:
			scr_style(RS_BB|RS_B0);
			break;
		case 101:
			scr_style(RS_BB|RS_B1);
			break;
		case 102:
			scr_style(RS_BB|RS_B2);
			break;
		case 103:
			scr_style(RS_BB|RS_B3);
			break;
		case 104:
			scr_style(RS_BB|RS_B4);
			break;
		case 105:
			scr_style(RS_BB|RS_B5);
			break;
		case 106:
			scr_style(RS_BB|RS_B6);
			break;
		case 107:
			scr_style(RS_BB|RS_B7);
			break;
		default:
			LOG("unhandled style %d", token->tk_arg[i]);
			//scr_style(RS_NONE);
		}
	}
}


