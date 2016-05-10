#include "handle_sgr.h"

#include "color.h"
#include "screen.h"

void handle_sgr(struct tokenst * restrict token)
{
	if (token->tk_nargs == 0) {
		scr_change_rendition(RS_NONE);
		return;
	}
	for (uint8_t i = 0; i < token->tk_nargs; i++) {
#ifdef DEBUG
			fprintf(stderr, "tk_arg[%d]: %d\n", i,
				token->tk_arg[i]);
#endif//DEBUG
		switch (token->tk_arg[i]) {
		case 0 :
			scr_change_rendition(RS_NONE);
#ifdef DEBUG
			fprintf(stderr, "--RS_NONE--\n");
#endif//DEBUG
			reset_color();
			break;
		case 1 :
			scr_change_rendition(RS_BOLD);
			break;
		case 3:
			scr_change_rendition(RS_ITALIC);
			break;
		case 4 :
			scr_change_rendition(RS_ULINE);
			break;
		case 5 :
		case 6: // sub for rapidly blinking
			scr_change_rendition(RS_BLINK);
			break;
		case 7 :
			scr_change_rendition(RS_RVID);
			break;
			// FG Colors:
		case 30: set_fg("black"); break;
		case 31: set_fg("red3"); break;
		case 32: set_fg("green3"); break;
		case 33: set_fg("yellow3"); break;
		case 34: set_fg("blue3"); break;
		case 35: set_fg("magenta3"); break;
		case 36: set_fg("cyan3"); break;
		case 37: set_fg("grey90"); break;
		case 39: reset_fg(); break;
			 // BG Colors:
		case 40: set_bg("black"); break;
		case 41: set_bg("red3"); break;
		case 42: set_bg("green3"); break;
		case 43: set_bg("yellow3"); break;
		case 44: set_bg("blue3"); break;
		case 45: set_bg("magenta3"); break;
		case 46: set_bg("cyan3"); break;
		case 47: set_bg("grey90"); break;
		case 49: reset_bg(); break;
			// Bright FG Colors:
		case 90: set_fg("grey"); break;
		case 91: set_fg("red"); break;
		case 92: set_fg("green"); break;
		case 93: set_fg("yellow"); break;
		case 94: set_fg("blue"); break;
		case 95: set_fg("magenta"); break;
		case 96: set_fg("cyan"); break;
		case 97: set_fg("white"); break;
			// Bright BG Colors:
		case 100: set_bg("grey"); break;
		case 101: set_bg("red"); break;
		case 102: set_bg("green"); break;
		case 103: set_bg("yellow"); break;
		case 104: set_bg("blue"); break;
		case 105: set_bg("magenta"); break;
		case 106: set_bg("cyan"); break;
		case 107: set_bg("white"); break;

		default:
			// reset_color();
			scr_change_rendition(RS_NONE);
		}
	}
}


