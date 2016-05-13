#include "handle_sgr.h"

#include "color.h"
#include "screen.h"

#ifdef DEBUG_SGR
#include <stdio.h>
#endif//DEBUG_SGR

void handle_sgr(struct tokenst * restrict token)
{
	if (token->tk_nargs == 0) {
		scr_change_rendition(RS_NONE);
		return;
	}
	for (uint8_t i = 0; i < token->tk_nargs; i++) {
#ifdef DEBUG_SGR
			fprintf(stderr, "tk_arg[%d]: %d\n", i,
				token->tk_arg[i]);
#endif//DEBUG_SGR
		switch (token->tk_arg[i]) {
		case 0 :
			scr_change_rendition(RS_NONE);
#ifdef DEBUG_SGR
			fprintf(stderr, "--RS_NONE--\n");
#endif//DEBUG_SGR
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
		case 30: 
			scr_change_rendition(RS_F0);
 			break;
		case 31:
			scr_change_rendition(RS_F1);
			break;
		case 32:
			scr_change_rendition(RS_F2);
			break;
		case 33:
			scr_change_rendition(RS_F3);
			break;
		case 34:
			scr_change_rendition(RS_F4);
			break;
		case 35:
			scr_change_rendition(RS_F5);
			break;
		case 36:
			scr_change_rendition(RS_F6);
			break;
		case 37:
			scr_change_rendition(RS_F7);
			break;
		case 39:
			scr_change_rendition(RS_FR);
			break;
			// BG Colors:
		case 40:
			scr_change_rendition(RS_B0);
			break;
		case 41:
			scr_change_rendition(RS_B1);
			break;
		case 42:
			scr_change_rendition(RS_B2);
			break;
		case 43:
			scr_change_rendition(RS_B3);
			break;
		case 44:
			scr_change_rendition(RS_B4);
			break;
		case 45:
			scr_change_rendition(RS_B5);
			break;
		case 46:
			scr_change_rendition(RS_B6);
			break;
		case 47:
			scr_change_rendition(RS_B7);
			break;
		case 49: 
			scr_change_rendition(RS_BR);
			break;
			// Bright FG Colors:
		case 90:
			scr_change_rendition(RS_BF|RS_F0);
			break;
		case 91:
			scr_change_rendition(RS_BF|RS_F1);
			break;
		case 92:
			scr_change_rendition(RS_BF|RS_F2);
			break;
		case 93:
			scr_change_rendition(RS_BF|RS_F3);
			break;
		case 94:
			scr_change_rendition(RS_BF|RS_F4);
			break;
		case 95:
			scr_change_rendition(RS_BF|RS_F5);
			break;
		case 96:
			scr_change_rendition(RS_BF|RS_F6);
			break;
		case 97:
			scr_change_rendition(RS_BF|RS_F7);
			break;
			// Bright BG BColors:
		case 100:
			scr_change_rendition(RS_BB|RS_B0);
			break;
		case 101:
			scr_change_rendition(RS_BB|RS_B1);
			break;
		case 102:
			scr_change_rendition(RS_BB|RS_B2);
			break;
		case 103:
			scr_change_rendition(RS_BB|RS_B3);
			break;
		case 104:
			scr_change_rendition(RS_BB|RS_B4);
			break;
		case 105:
			scr_change_rendition(RS_BB|RS_B5);
			break;
		case 106:
			scr_change_rendition(RS_BB|RS_B6);
			break;
		case 107:
			scr_change_rendition(RS_BB|RS_B7);
			break;

		default:
			// reset_color();
			scr_change_rendition(RS_NONE);
		}
	}
}


