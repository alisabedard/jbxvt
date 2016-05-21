/*  Copyright 1992, 1994 John Bovey, University of Kent at Canterbury.
 *
 *  Redistribution and use in source code and/or executable forms, with
 *  or without modification, are permitted provided that the following
 *  condition is met:
 *
 *  Any redistribution must retain the above copyright notice, this
 *  condition and the following disclaimer, either as part of the
 *  program source code included in the redistribution or in human-
 *  readable materials provided with the redistribution.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS".  Any express or implied
 *  warranties concerning this software are disclaimed by the copyright
 *  holder to the fullest extent permitted by applicable law.  In no
 *  event shall the copyright-holder be liable for any damages of any
 *  kind, however caused and on any theory of liability, arising in any
 *  way out of the use of, or inability to use, this software.
 *
 *  -------------------------------------------------------------------
 *
 *  In other words, do not misrepresent my work as your own work, and
 *  do not sue me if it causes problems.  Feel free to do anything else
 *  you wish with it.
 */

#include "xvt.h"

#include "cmdtok.h"
#include "color.h"
#include "command.h"
#include "config.h"
#include "cursor.h"
#include "handle_sgr.h"
#include "init_display.h"
#include "jbxvt.h"
#include "log.h"
#include "sbar.h"
#include "scr_delete_characters.h"
#include "scr_erase.h"
#include "scr_extend_selection.h"
#include "scr_insert_characters.h"
#include "scr_move.h"
#include "scr_refresh.h"
#include "scr_request_selection.h"
#include "scr_reset.h"
#include "scr_string.h"
#include "scr_tab.h"
#include "screen.h"
#include "selection.h"
#include "token.h"
#include "ttyinit.h"
#include "xsetup.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool jbxvt_size_set;

static enum ModeValue handle_reset(struct tokenst * restrict token)
{
	LOG("handle_reset()");
	enum ModeValue mode = (token->tk_type == TK_SET)
		? JBXVT_MODE_HIGH : JBXVT_MODE_LOW;
	if (token->tk_private == '?') {
		switch (token->tk_arg[0]) {
		case 1 :
			set_cur_keys(mode);
			break;
		case 6 :
			jbxvt.scr.current->decom = mode == JBXVT_MODE_HIGH;
			break;
		case 7 :
			jbxvt.scr.current->wrap = mode == JBXVT_MODE_HIGH;
			break;
		case 47: // switch to main screen
			scr_change_screen(mode);
			break;
		case 1049: // Fix stale chars in vi
			scr_change_screen(mode);
			break;
		default:
			LOG("Unhandled: %d\n", token->tk_arg[0]);
		}
	} else if (token->tk_private == 0) {
		switch (token->tk_arg[0]) {
		case 4 :
			jbxvt.scr.current->insert = mode == JBXVT_MODE_HIGH;
			break;
		default:
			LOG("Unhandled: %d\n", token->tk_arg[0]);
		}
	}
	return mode;
}

static void handle_cup(struct tokenst * restrict token,
	int16_t * restrict x, int16_t * restrict y) // Position cursor
{
	LOG("handle_cup()");
	if (token->tk_nargs == 1) {
		*x = 0;
		*y = token->tk_arg[0] ? token->tk_arg[0] - 1 : 0;
	} else {
		*y = token->tk_arg[0] - 1;
		*x = token->tk_arg[1] - 1;
	}
	scr_move(*x,*y,0);
}

static void handle_txtpar(struct tokenst * restrict token)
{
	switch (token->tk_arg[0]) {
	case 0 :
		change_name(token->tk_string, true, true);
		break;
	case 1 :
		change_name(token->tk_string, false, true);
		break;
	case 2 :
		change_name(token->tk_string, true, false);
		break;
	case 4: // change colors
	case 12: // cursor color
	case 13: // pointer color
	case 17: // highlight color
	case 19: // underline color
	case 46: // log file
	case 50: // font
	default:
		LOG("unhandled txtpar: %d", token->tk_arg[0]);
		break;
	}

}

/* set top and bottom margins */
static void handle_decstbm(struct tokenst * restrict token)
{
	LOG("handle_decstbm()");
	if (token->tk_private == '?')
		  //  xterm uses this combination to reset parameters.
		  return;
	if (token->tk_nargs < 2 || token->tk_arg[0]
		>= token->tk_arg[1])
		  scr_set_margins(0,10000);
	else
		  scr_set_margins(token->tk_arg[0]
			  - 1,token->tk_arg[1] - 1);

}

static void handle_tk_char(const uint8_t tk_char)
{
	switch (tk_char) {
	case '\n': // handle line feed
		scr_index();
		break;
	case '\r': // handle carriage return
		scr_move(0,0,ROW_RELATIVE);
		break;
	case '\b': // handle a backspace
		scr_move(-1,0,COL_RELATIVE|ROW_RELATIVE);
		break;
	case '\t': // handle tab
		scr_tab();
		break;
	case '\007': // ring the bell
		XBell(jbxvt.X.dpy,0);
		break;
	}
}

static void handle_tk_expose(struct tokenst * restrict t)
{
	LOG("handle_tk_expose()");
	switch (t->tk_region) {
	case SCREEN :
		if(jbxvt_size_set)
			scr_refresh(t->tk_arg[0],t->tk_arg[1],
				t->tk_arg[2],t->tk_arg[3]);
		else {
			/*  Force a full reset if an exposure event
			 *  arrives after a resize.  */
			scr_reset();
			jbxvt_size_set = true;
		}
		break;
	case SCROLLBAR :
		sbar_reset();
		break;
	}
}

void jbxvt_app_loop(void)
{
	LOG("app_loop");
	struct tokenst token;
	int32_t n;
	int16_t x, y;
app_loop_head:
	get_token(&token);
	switch (token.tk_type) {
	case TK_STRING :
		scr_string(token.tk_string,token.tk_length,
			token.tk_nlcount);
		break;
	case TK_CHAR :
		handle_tk_char(token.tk_char);
		break;
	case TK_EOF :
		quit(0);
		break;
	case TK_ENTRY :	// keyboard focus changed
		scr_focus(token.tk_arg[0]|SCR_FOCUS_ENTRY);
		break;
	case TK_FOCUS :
		scr_focus(token.tk_arg[0]|SCR_FOCUS_FOCUS);
		break;
	case TK_EXPOSE: // window exposed
		handle_tk_expose(&token);
		break;
	case TK_RESIZE :
		jbxvt_size_set = false;
		resize_window();
		break;
	case TK_TXTPAR:	// change title or icon name
		handle_txtpar(&token);
		break;
	case TK_SBSWITCH :
		switch_scrollbar();
		break;
	case TK_SBGOTO :
		scr_move_to(token.tk_arg[0]); break;
	case TK_SBUP :
		scr_move_by(token.tk_arg[0]); break;
	case TK_SBDOWN :
		scr_move_by(-token.tk_arg[0]); break;
	case TK_SELSTART :
		scr_start_selection(token.tk_arg[0],token.tk_arg[1],CHAR);
		break;
	case TK_SELEXTND :
		scr_extend_selection(token.tk_arg[0],token.tk_arg[1],0);
		break;
	case TK_SELDRAG :
		scr_extend_selection(token.tk_arg[0],token.tk_arg[1],1);
		break;
	case TK_SELWORD :
		scr_start_selection(token.tk_arg[0],token.tk_arg[1],WORD);
		break;
	case TK_SELLINE :
		scr_start_selection(token.tk_arg[0],token.tk_arg[1],LINE);
		break;
	case TK_SELECT :
		scr_make_selection(token.tk_arg[0]);
		break;
	case TK_SELCLEAR :
		scr_clear_selection();
		break;
	case TK_SELREQUEST :
		scr_send_selection(token.tk_arg[0],token.tk_arg[1],
			token.tk_arg[2],token.tk_arg[3]);
		break;
	case TK_SELINSRT :
		scr_request_selection(token.tk_arg[0],
			token.tk_arg[1],token.tk_arg[2]);
		break;
	case TK_SELNOTIFY :
		// arg 0 is time, unused
		scr_paste_primary(token.tk_arg[1],token.tk_arg[2]);
		break;
	case TK_CUU :	/* cursor up */
		n = token.tk_arg[0];
		n = n == 0 ? -1 : -n;
		scr_move(0,n,ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUD :	/* cursor down */
		n = token.tk_arg[0];
		n = n == 0 ? 1 : n;
		scr_move(0,n,ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUF :	/* cursor forward */
		n = token.tk_arg[0];
		n = n == 0 ? 1 : n;
		scr_move(n,0,ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_CUB :	/* cursor back */
		n = token.tk_arg[0];
		n = n == 0 ? -1 : -n;
		scr_move(n,0,ROW_RELATIVE | COL_RELATIVE);
		break;
	case TK_HVP :
	case TK_CUP :	/* position cursor */
		handle_cup(&token, &x, &y);
		break;
	case TK_ED :
		scr_erase_screen(token.tk_arg[0]);
		break;
	case TK_EL :
		scr_erase_line(token.tk_arg[0]);
		break;
	case TK_IL :
		n = token.tk_arg[0];
		if (n == 0)
			  n = 1;
		scr_insert_lines(n);
		break;
	case TK_DL :
		n = token.tk_arg[0];
		if (n == 0)
			  n = 1;
		scr_delete_lines(n);
		break;
	case TK_DCH :
		n = token.tk_arg[0];
		if (n == 0)
			  n = 1;
		scr_delete_characters(n);
		break;
	case TK_ICH :
		n = token.tk_arg[0];
		if (n == 0)
			  n = 1;
		scr_insert_characters(n);
		break;
	case TK_DA :
		LOG("TK_DA");
		break;
	case TK_TBC :
		LOG("TK_TBC");
		break;
	case TK_SET :
	case TK_RESET :
		handle_reset(&token);
		break;
	case TK_SGR :
		handle_sgr(&token);
		break;
	case TK_DSR :		/* request for information */
		LOG("TK_DSR");
		switch (token.tk_arg[0]) {
		case 6 :
			scr_report_position();
			break;
		case 7 :	/* display name */
			scr_report_display();
			break;
		}
		break;
	case TK_DECSTBM:
		handle_decstbm(&token);
		break;
	case TK_DECSWH :		/* ESC # digit */
		LOG("TK_DECSWH");
		break;
	case TK_DECSC :
		scr_save_cursor();
		break;
	case TK_DECRC :
		scr_restore_cursor();
		break;
	case TK_DECPAM :
		set_kp_keys(JBXVT_MODE_HIGH);
		break;
	case TK_DECPNM :
		set_kp_keys(JBXVT_MODE_LOW);
		break;
	case TK_IND :		/* Index (same as \n) */
		scr_index();
		break;
	case TK_NEL :
		break;
	case TK_HTS :
		break;
	case TK_RI :		/* Reverse index */
		scr_rindex();
		break;
	case TK_SS2 :
		break;
	case TK_SS3 :
		break;
	case TK_DECID :
		LOG("TK_DECID");
		cprintf("\033[?6c");	/* I am a VT102 */
		break;
	}
#ifdef TK_DEBUG
	show_token(&token);
#endif /* TK_DEBUG */
	goto app_loop_head;
}

