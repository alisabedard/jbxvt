#include "handle_sgr.h"

#include "color.h"
#include "log.h"
#include "screen.h"

void handle_sgr(struct tokenst * restrict token)
{
	if (token->tk_nargs == 0) {
		scr_style(RS_NONE);
		return;
	}
	bool fg_rgb_mode = false;
	bool fg_index_mode = false;
	bool bg_rgb_mode = false;
	bool bg_index_mode = false;
	for (uint_fast8_t i = 0; i < token->tk_nargs; ++i) {
		LOG("handle_sgr: tk_arg[%d]: %x", i, token->tk_arg[i]);
		if (fg_index_mode || bg_index_mode) {
			jbputs("FIXME: implement color index mode\n");
		}
		if (fg_rgb_mode || bg_rgb_mode) {
			jbputs("FIXME: implement color rgb mode\n");
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
		case 7 :
			scr_style(RS_RVID);
			break;
			// FG Colors:
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
		case 38: // rgb or index mode
			if (token->tk_nargs == 1)
				fg_index_mode = true;
			else
				fg_rgb_mode = true;
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
		case 48: // rgb or index mode
			if (token->tk_nargs == 1)
				bg_index_mode = true;
			else
				bg_rgb_mode = true;
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
			LOG("unhandled style %x", token->tk_arg[i]);
			scr_style(RS_NONE);
		}
	}
}


