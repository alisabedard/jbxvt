// Copyright 2017, Jeffrey E. Bedard
#include "tab.h"
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTScreen.h"
#include "JBXVTToken.h"
#include "config.h"
#include "cursor.h"
#include "libjb/JBDim.h"
#include "libjb/log.h"
#include "screen.h"
#include "sbar.h"
#include "size.h"
static bool tab_stops[JBXVT_MAX_COLUMNS];
// establish tab stop every 8 columns
static void reset_tabs(const short i)
{
    if (i < JBXVT_MAX_COLUMNS) {
        tab_stops[i] = (i % 8 == 0);
        reset_tabs(i + 1);
    }
}
// Set tab stops:
// -1 clears all, -2 sets default
void jbxvt_set_tab(short i, const bool value)
{
    if (i == -1) // clear all
        memset(&tab_stops, 0, JBXVT_MAX_COLUMNS);
    else if (i == -2)
        reset_tabs(0);
    else if (i >= 0) // assign
        tab_stops[i] = value;
}
//  Tab to the next tab_stop.
void jbxvt_tab(xcb_connection_t * xc, const short count)
{
    LOG("jbxvt_tab()");
    if (count < 1)
        return;
    jbxvt_set_scroll(xc, 0);
    struct JBXVTScreen * restrict scr = jbxvt_get_current_screen();
    struct JBDim c = scr->cursor;
    { // text scope
        uint8_t * restrict text = scr->line[c.y].text;
        text[c.x] = ' ';
        { // cw scope
            const uint16_t cw = jbxvt_get_char_size().w;
            // Advance to the next valid tab stop
            while (!tab_stops[++c.x] && c.x < cw)
                ;
        }
    }
    scr->cursor.x = c.x;
    jbxvt_tab(xc, count - 1);
}
// Handle DECTBC:
void jbxvt_handle_JBXVT_TOKEN_TBC(void * xc __attribute__((unused)),
    struct JBXVTToken * token)
{
    const uint8_t t = token->arg[0];
    /* Note:  Attempting to simplify this results
       in vttest test failure.  */
    if (t == 3) // clear all
        jbxvt_set_tab(-1, false);
    else if (!t) // clear at cursor
        jbxvt_set_tab(jbxvt_get_x(), false);
}
