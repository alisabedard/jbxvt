/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#define LOG_LEVEL 3
#if LOG_LEVEL == 0
#undef DEBUG
#endif//LOG_LEVEL
#include "xvt.h"
#include "JBXVTPrivateModes.h"
#include "JBXVTScreen.h"
#include "JBXVTToken.h"
#include "cases.h"
#include "cmdtok.h"
#include "command.h"
#include "cursor.h"
#include "double.h"
#include "dsr.h"
#include "edit.h"
#include "erase.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "lookup_key.h"
#include "mc.h"
#include "mode.h"
#include "move.h"
#include "request.h"
#include "sbar.h"
#include "scr_reset.h"
#include "screen.h"
#include "scroll.h"
#include "selreq.h"
#include "size.h"
#include "string.h"
#include "tab.h"
#include "tk_char.h"
#include "window.h"
#include <unistd.h>
// Return a default of 1 if arg is 0:
__attribute__((const))
static int16_t get_n(const int16_t arg)
{
    return arg ? arg : 1;
}
// Return the 0-based coordinate value:
__attribute__((const))

static int16_t get_0(int16_t arg)
{
    return get_n(arg) - 1;
}
// CUP and HVP, move cursor
static void cup(xcb_connection_t * xc, int16_t * restrict t)
{
    // subtract 1 for 0-based coordinates
    enum {
        COL, ROW, REL = JBXVT_ROW_RELATIVE | JBXVT_COLUMN_RELATIVE,
    };
    const int16_t row = get_0(t[ROW]), col = get_0(t[COL]);
    jbxvt_move(xc, row, col, jbxvt_get_modes()->decom ? REL : 0);
}
// Return value sanitized for tokens with optional arguments, defaulting to 1
static int16_t get_arg(struct JBXVTToken * t)
{
    return t->nargs > 0 && t->arg[0] ? t->arg[0] : 1;
}
static void jbxvt_handle_JBXVT_TOKEN_ALN(xcb_connection_t * xc,
    struct JBXVTToken * token) // screen alignment test
{
    NOPARM_TOKEN();
    jbxvt_efill(xc);
}
static void jbxvt_handle_JBXVT_TOKEN_CHA(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor character absolute column
{
    jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_ROW_RELATIVE);
}
static void jbxvt_handle_JBXVT_TOKEN_CHAR(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    jbxvt_handle_tk_char(xc, token->tk_char);
}
static void jbxvt_handle_JBXVT_TOKEN_CHT(xcb_connection_t * xc,
    struct JBXVTToken * token) // Cursor Horizontal Tab
{
    jbxvt_tab(xc, get_arg(token));
}
static void jbxvt_handle_JBXVT_TOKEN_CPL(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor previous line
{
    jbxvt_move(xc, 0, -get_arg(token), 0);
}
static void jbxvt_handle_JBXVT_TOKEN_CNL(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor next line
{
    jbxvt_move(xc, 0, get_arg(token), 0);
}
// Select character set per token parameter
static void charset(const char c, const uint8_t i)
{
    switch(c) {
#define CS(l, cs, d) case l:LOG(d);\
        jbxvt_get_modes()->charset[i]=CHARSET_##cs;break;
        CS('A', GB, "UK ASCII");
        CS('0', SG0, "SG0: special graphics");
        CS('1', SG1, "SG1: alt char ROM standard graphics");
        CS('2', SG2, "SG2: alt char ROM special graphics");
    default: // reset
        LOG("Unknown character set");
        // fall through
        CS('B', ASCII, "US ASCII");
    }
#undef CS
}
static void jbxvt_handle_JBXVT_TOKEN_CS_G0(xcb_connection_t * xc,
    struct JBXVTToken * token){
    (void)xc;
    charset(token->arg[0], 0);
};
static void jbxvt_handle_JBXVT_TOKEN_CS_G1(xcb_connection_t * xc,
    struct JBXVTToken * token){
    (void)xc;
    charset(token->arg[0], 1);
};
static void jbxvt_handle_JBXVT_TOKEN_CS_G2(xcb_connection_t * xc,
    struct JBXVTToken * token){
    (void)xc;
    charset(token->arg[0], 2);
};
static void jbxvt_handle_JBXVT_TOKEN_CS_G3(xcb_connection_t * xc,
    struct JBXVTToken * token){
    (void)xc;
    charset(token->arg[0], 3);
};
ALIAS(CS_ALT_G1, CS_G1);
ALIAS(CS_ALT_G2, CS_G2);
ALIAS(CS_ALT_G3, CS_G3);
// Move cursor relative to current position
static void mv(xcb_connection_t * xc, const int16_t x, const int16_t y)
{
    jbxvt_move(xc, x, y, JBXVT_ROW_RELATIVE | JBXVT_COLUMN_RELATIVE);
}
static void jbxvt_handle_JBXVT_TOKEN_CUB(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor back
{
    mv(xc, -get_arg(token), 0);
}
static void jbxvt_handle_JBXVT_TOKEN_CUD(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor down
{
    mv(xc, 0, get_arg(token));
}
static void jbxvt_handle_JBXVT_TOKEN_CUF(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor forward
{
    mv(xc, get_arg(token), 0);
}
// Set cursor position, absolute mode.
static void jbxvt_handle_JBXVT_TOKEN_CUP(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    cup(xc, token->arg);
}
ALIAS(HVP, CUP); // horizontal vertical position
static void jbxvt_handle_JBXVT_TOKEN_CUU(xcb_connection_t * xc,
    struct JBXVTToken * token) // cursor up
{
    mv(xc, 0, -get_arg(token));
}
static void jbxvt_handle_JBXVT_TOKEN_DL(xcb_connection_t * xc,
    struct JBXVTToken * token) // delete line
{
    jbxvt_index_from(xc, get_arg(token), jbxvt_get_y());
}
static void jbxvt_handle_JBXVT_TOKEN_DSR(xcb_connection_t * xc,
    struct JBXVTToken * token) // device status report
{
    NOPARM_XC();
    jbxvt_handle_dsr(token->arg[0]);
}
static void jbxvt_handle_JBXVT_TOKEN_DWL(xcb_connection_t * xc,
    struct JBXVTToken * token) // double width line
{
    NOPARM_TOKEN();
    jbxvt_set_double_width_line(xc, true);
}
static void jbxvt_handle_JBXVT_TOKEN_ED(xcb_connection_t * xc,
    struct JBXVTToken * token) // erase display
{
    jbxvt_erase_screen(xc, token->arg[0]); // don't use get_arg()
}
static void jbxvt_handle_JBXVT_TOKEN_EL(xcb_connection_t * xc,
    struct JBXVTToken * token) // erase line
{
    jbxvt_erase_line(xc, token->arg[0]); // don't use get_arg()
}
static void jbxvt_handle_JBXVT_TOKEN_DA(xcb_connection_t * xc,
    struct JBXVTToken * token) // DECID
{
#ifdef JBXVT_NO_DPRINTF
    uint8_t size;
    fd_t const fd  = jbxvt_get_fd();
    NOPARM();
    size = 0;
    char const *csi = csi = jbxvt_get_csi();
    while(csi[++size])
        ;
    write(fd, csi, size);
    write(fd, "?6c", 3);
#else // ! JBXVT_NO_DPRINTF
    // print terminal id
    NOPARM();
    dprintf(jbxvt_get_fd(), "%s?6c", jbxvt_get_csi()); // VT102
#endif // JBXVT_NO_DPRINTF
}
ALIAS(ID, DA); // DECID
static void jbxvt_handle_JBXVT_TOKEN_DCH(xcb_connection_t * xc,
    struct JBXVTToken * token) // delete character
{
    jbxvt_edit_characters(xc, get_arg(token), true);
}
ALIAS(ECH, DCH); // erase character
// set vt52 graphics mode
static void gm52(const bool set)
{
    struct JBXVTPrivateModes * restrict m = jbxvt_get_modes();
    m->charsel = (m->gm52 = set) ? 1 : 0;
}
static void jbxvt_handle_JBXVT_TOKEN_ENTGM52(xcb_connection_t * xc,
    struct JBXVTToken * token) // enter vt52 graphics mode
{
    NOPARM();
    gm52(true);
}
static void jbxvt_handle_JBXVT_TOKEN_EXTGM52(xcb_connection_t * xc,
    struct JBXVTToken * token) // exit vt52 graphics mode
{
    NOPARM();
    gm52(false);
}
static void jbxvt_handle_JBXVT_TOKEN_HOME(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    (void)token;
    jbxvt_set_scroll(xc, 0);
    jbxvt_move(xc, 0, 0, 0);
}
static void jbxvt_handle_JBXVT_TOKEN_HPA(xcb_connection_t * xc,
    struct JBXVTToken * token) // horizontal position absolute
{
    jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_ROW_RELATIVE);
}
static void jbxvt_handle_JBXVT_TOKEN_HPR(xcb_connection_t * xc,
    struct JBXVTToken * token) // horizontal position relative
{
    jbxvt_move(xc, get_0(token->arg[0]), 0, JBXVT_COLUMN_RELATIVE
        | JBXVT_ROW_RELATIVE);
}
static void jbxvt_handle_JBXVT_TOKEN_HTS(xcb_connection_t * xc,
    struct JBXVTToken * token) // horizontal tab stop
{
    NOPARM();
    jbxvt_set_tab(jbxvt_get_x(), true);
}
static void jbxvt_handle_JBXVT_TOKEN_ICH(xcb_connection_t * xc,
    struct JBXVTToken * token) // insert blank characters
{
    jbxvt_edit_characters(xc, get_arg(token), false);
}
static void jbxvt_handle_JBXVT_TOKEN_IL(xcb_connection_t * xc,
    struct JBXVTToken * token) // insert line
{
    jbxvt_index_from(xc, -get_arg(token), jbxvt_get_y());
}
// index
static void jbxvt_handle_JBXVT_TOKEN_IND(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    jbxvt_index_from(xc, get_arg(token), jbxvt_get_margin()->t);
}
// next line (move to the first position on the next line
static void jbxvt_handle_JBXVT_TOKEN_NEL(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM_TOKEN();
    jbxvt_move(xc, 0, jbxvt_get_y() + 1, 0);
}
static void jbxvt_handle_JBXVT_TOKEN_PAM(xcb_connection_t * xc,
    struct JBXVTToken * token) // application mode keys
{
    NOPARM();
    jbxvt_set_keys(true, false);
}
static void jbxvt_handle_JBXVT_TOKEN_PM(xcb_connection_t * xc,
    struct JBXVTToken * token) // privacy message
{
    NOPARM();
    jbxvt_get_current_screen()->decpm = true;
}
static void jbxvt_handle_JBXVT_TOKEN_PNM(xcb_connection_t * xc,
    struct JBXVTToken * token) // numeric key mode
{
    NOPARM();
    jbxvt_set_keys(false, false);
}
static void jbxvt_handle_JBXVT_TOKEN_RC(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM_TOKEN();
    jbxvt_restore_cursor(xc);
}
EXTERN_ALIAS(RESET, jbxvt_dec_reset);
EXTERN_ALIAS(SET, jbxvt_dec_reset);
static void jbxvt_handle_JBXVT_TOKEN_RI(xcb_connection_t * xc,
    struct JBXVTToken * token) // reverse index
{
    jbxvt_index_from(xc, -get_arg(token), jbxvt_get_margin()->top);
}
static void jbxvt_handle_JBXVT_TOKEN_RIS(xcb_connection_t * xc,
    struct JBXVTToken * token) // reset to initial state
{
    NOPARM_TOKEN();
    jbxvt_get_modes()->dectcem = true;
    jbxvt_reset(xc);
}
static void set_s8c1t(const bool val)
{
    jbxvt_get_modes()->s8c1t = val;
}
static void jbxvt_handle_JBXVT_TOKEN_S7C1T(xcb_connection_t * xc,
    struct JBXVTToken * token) // 7-bit control sequences
{
    NOPARM();
    set_s8c1t(false);
}
static void jbxvt_handle_JBXVT_TOKEN_S8C1T(xcb_connection_t * xc,
    struct JBXVTToken * token) // 8-bit control sequences
{
    NOPARM();
    set_s8c1t(true);
}
static void jbxvt_handle_JBXVT_TOKEN_SAVEPM(xcb_connection_t * xc,
    struct JBXVTToken * token) // save private modes
{
    NOPARM();
    jbxvt_save_modes();
}
static void jbxvt_handle_JBXVT_TOKEN_SBGOTO(xcb_connection_t * xc,
    struct JBXVTToken * token) // Scroll to where token arg[0] is at top of screen.
{
    jbxvt_scroll_to(xc, token->arg[0]);
}
static void jbxvt_handle_JBXVT_TOKEN_SBSWITCH(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM_TOKEN();
    jbxvt_toggle_scrollbar(xc);
}
static void jbxvt_handle_JBXVT_TOKEN_SC(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM();
    jbxvt_save_cursor();
}
static void handle_scroll(xcb_connection_t * xc, const int16_t arg)
{
    const struct JBDim m = *jbxvt_get_margin();
    // scroll arg lines within margin m:
    scroll(xc, m.top, m.bot, arg);
}
static void jbxvt_handle_JBXVT_TOKEN_SD(xcb_connection_t * xc,
    struct JBXVTToken * token) // scroll down
{
    handle_scroll(xc, -token->arg[0]);
}
EXTERN_ALIAS(SGR, jbxvt_handle_sgr);
static void jbxvt_handle_JBXVT_TOKEN_SS2(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM();
    jbxvt_get_modes()->ss2 = true;
}
static void jbxvt_handle_JBXVT_TOKEN_SS3(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    NOPARM();
    jbxvt_get_modes()->ss3 = true;
}
static void jbxvt_handle_JBXVT_TOKEN_ST(xcb_connection_t * xc,
    struct JBXVTToken * token) // string terminator
{
    NOPARM();
    jbxvt_get_current_screen()->decpm = false;
}
static void jbxvt_handle_JBXVT_TOKEN_STBM(xcb_connection_t * xc,
    struct JBXVTToken * token) // set top and bottom margins
{
    NOPARM_XC();
    int16_t * restrict t = token->arg;
    LOG("JBXVT_TOKEN_STBM args: %d, 0: %d, 1: %d",
        (int)token->nargs, t[0], t[1]);
    if (token->private == JBXVT_TOKEN_RESTOREPM) {
        jbxvt_restore_modes();
    } else { // set margins
        const bool rst = token->nargs < 2 || t[0] >= t[1];
        struct JBDim * restrict m = jbxvt_get_margin();
        m->top = rst ? 0 : get_0(t[0]);
        m->bot = (rst ? jbxvt_get_char_size().h : get_n(t[1])) - 1;
    }
}
static void jbxvt_handle_JBXVT_TOKEN_STRING(xcb_connection_t * xc,
    struct JBXVTToken * token)
{
    jbxvt_string(xc, token->string, token->length, token->nlcount);
}
static void jbxvt_handle_JBXVT_TOKEN_SU(xcb_connection_t * xc,
    struct JBXVTToken * token) // scroll up
{
    handle_scroll(xc, token->arg[0]);
}
static void jbxvt_handle_JBXVT_TOKEN_SWL(xcb_connection_t * xc,
    struct JBXVTToken * token) // single width line
{
    NOPARM_TOKEN();
    jbxvt_set_double_width_line(xc, false);
}
// vertical position, absolute or relative
static void vp(xcb_connection_t * xc, const uint16_t arg, const bool relative)
{
    jbxvt_move(xc, 0, get_0((int16_t)arg), JBXVT_COLUMN_RELATIVE |
        (relative ? JBXVT_ROW_RELATIVE : 0));
}
static void jbxvt_handle_JBXVT_TOKEN_VPA(xcb_connection_t * xc,
    struct JBXVTToken * token) // vertical position absolute
{
    vp(xc, (uint16_t)token->arg[0], false);
}
static void jbxvt_handle_JBXVT_TOKEN_VPR(xcb_connection_t * xc,
    struct JBXVTToken * token) // vertical position relative
{
    vp(xc, (uint16_t)token->arg[0], true);
}
bool jbxvt_parse_token(xcb_connection_t * xc)
{
    struct JBXVTToken token;
    jbxvt_get_token(xc, &token);
    bool rval = true; // keep going
    switch (token.type) {
    case JBXVT_TOKEN_EOF:
        LOG("JBXVT_TOKEN_EOF");
        rval = false; // stop token parsing loop, quit
        break;
#include "cases.c"
    default:
#ifdef DEBUG
        if(token.type) { // Ignore JBXVT_TOKEN_NULL
            LOG("Unhandled token: %d (0x%x)",
                token.type, token.type);
            // Exit now so we can implement it!
            rval = false;
        }
#endif//DEBUG
        break;
    }
    return rval;
}
