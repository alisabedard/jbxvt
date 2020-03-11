// Copyright 2017, Jeffrey E. Bedard
#undef DEBUG
//#define LOOKUP_KEY_DUMP_TABLES
#include "lookup_key.h"
#include <xcb/xcb_keysyms.h>
#include "JBXVTCommandLimits.h"
#include "JBXVTKeyMaps.h"
#include "JBXVTKeySymType.h"
#include "libjb/log.h"
#include "libjb/util.h"
#include "sbar.h"
enum KeyboardModes {
    CURSOR_MODE = 1, KEYPAD_MODE = 2
};
uint8_t keyboard_mode;
/*  Thanks to Rob McMullen for the following function key mapping tables and
 *  code. */
#include "key_tables.c"
// Reference <X11/keysymdef.h>
/* These macros allow easy generation of keysyms for comparisons.  */
#define K_C(n) (0xff00 | n)
#define K_INS K_C(0x63)
#define K_DEL K_C(0x9f)
#define K_F(n) (0xffbd + n)
// Keypad keys:
#define JBXVT_KEYSYM_FUNCTION(n) K_C(0x90 | n)
// Regular keys:
#define K_N(n) K_C(0x50 | n)
#define K_PU K_N(5)
#define K_PD K_N(6)
#ifdef LOOKUP_KEY_DUMP_TABLES
static bool visible(const uint8_t c)
{
    return c > ' ' && c < '~';
}
static void serialize(const struct JBXVTKeyMaps * k)
{
    if (k->km_keysym) {
        const uint8_t n = k->km_normal[1], a = k->km_alt[1];
        if (visible(n) && visible(a))
            printf("\t{0x%x, {%d, '%c'}, {%d, '%c'}},\n",
                k->km_keysym,
                (int)k->km_normal[0], (int)k->km_normal[1],
                (int)k->km_alt[0], (int)k->km_alt[1]);
        else // mask in hex
            printf("\t{0x%x, {%d, 0x%x}, {%d, 0x%x}},\n",
                k->km_keysym,
                (int)k->km_normal[0], (int)k->km_normal[1],
                (int)k->km_alt[0], (int)k->km_alt[1]);

        serialize(k+1);
    } else
        printf("\t{}\n};\n");
}
static void pr_header(const char * h)
{
    printf("static struct JBXVTKeyMaps %s[]={\n", h);
}
static void dump(void)
{
    pr_header("func_key_table");
    serialize(func_key_table);
    pr_header("other_key_table");
    serialize(other_key_table);
    pr_header("kp_key_table");
    serialize(kp_key_table);
}
#endif// LOOKUP_KEY_DUMP_TABLES
// Set key mode for cursor keys if is_cursor, else for keypad keys
void jbxvt_set_keys(const bool mode_high, const bool is_cursor)
{
    const uint8_t flag = is_cursor ? CURSOR_MODE : KEYPAD_MODE;
    if (mode_high)
        keyboard_mode |= flag;
    else
        keyboard_mode &= ~flag;
}
static char * get_format(const enum JBXVTKeySymType type)
{
    switch(type) {
    case XTERM_KS:
        return "\033[%d~";
    case APPKEY_KS:
        return "\033O%c";
    case NONAPP_KS:
        return "\033[%c";
    default:
        return "%c";
    }
}
static uint8_t * get_buffer(uint8_t * buf,
    struct JBXVTKeyMaps * keymaptable,
    const bool use_alternate)
{
    uint8_t * ks = use_alternate
        ? keymaptable->km_alt : keymaptable->km_normal;
    snprintf((char *)buf, KEY_MAP_BUF_SIZE, get_format(ks[0]), ks[1]);
    return buf;
}
//  Look up function key keycode
static uint8_t * get_keycode_value(struct JBXVTKeyMaps * restrict
    keymaptable, xcb_keysym_t keysym, uint8_t* buf,
    const bool use_alternate)
{
    return keymaptable->km_keysym
        ? (keymaptable->km_keysym == keysym)
            ?  get_buffer(buf, keymaptable, use_alternate)
                : get_keycode_value(keymaptable + 1, keysym,
                    buf, use_alternate)
        : NULL;
}
static uint8_t * get_s(const xcb_keysym_t keysym, uint8_t * kbuf)
{
    if (xcb_is_function_key(keysym) || xcb_is_misc_function_key(keysym)
        || keysym == K_PD || keysym == K_PU)
        return get_keycode_value(func_key_table, keysym, kbuf,
            false);
    if (xcb_is_cursor_key(keysym) || xcb_is_pf_key(keysym))
        return get_keycode_value(other_key_table, keysym,
            kbuf, keyboard_mode & CURSOR_MODE);
    return get_keycode_value(kp_key_table, keysym,
        kbuf, keyboard_mode & KEYPAD_MODE);
}
/* FIXME: Make this portable to non-US keyboards, or write a version
   or table for each type.  Perhaps use libxkbcommon-x11.  */
static const uint8_t shift_map[][2] = {{'1', '!'}, {'2', '@'}, {'3', '#'},
    {'4', '$'}, {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'},
    {'9', '('}, {'0', ')'}, {'-', '_'}, {'=', '+'}, {';', ':'},
    {'\'', '"'}, {'[', '{'}, {']', '}'}, {'\\', '|'}, {'`', '~'},
    {',', '<'}, {'.', '>'}, {'/', '?'}, {}};
__attribute__((const))
static uint8_t shift(const uint8_t c)
{
    if (c >= 'a' && c <= 'z')
        return c - 0x20; // c - SPACE
    for (uint8_t i = 0; shift_map[i][0]; ++i)
        if (shift_map[i][0] == c)
            return shift_map[i][1];
    return c;
}
static void apply_state(const uint16_t state, uint8_t * kbuf)
{
    switch (state) {
    case XCB_MOD_MASK_SHIFT:
    case XCB_MOD_MASK_LOCK:
        LOG("XCB_MOD_MASK_SHIFT/LOCK");
        kbuf[0] = shift(kbuf[0]);
        break;
    case XCB_MOD_MASK_CONTROL:
        LOG("XCB_MOD_MASK_CONTROL");
        kbuf[0] -= kbuf[0] >= 'a' ? 0x60 : 0x40;
        break;
    case XCB_MOD_MASK_1:
        LOG("XCB_MOD_MASK_1");
        kbuf[0] += 0x80;
        break;
    }
}
static xcb_keysym_t get_keysym(xcb_connection_t * c,
    xcb_key_press_event_t * ke)
{
    xcb_key_symbols_t *syms = xcb_key_symbols_alloc(c);
    xcb_keysym_t k = xcb_key_press_lookup_keysym(syms, ke, 2);
    LOG("keycode: 0x%x, keysym: 0x%x, state: 0x%x",
        ke->detail, k, ke->state);
    xcb_key_symbols_free(syms);
    return k;
}
static void page_key_scroll(xcb_connection_t * xc, const int8_t mod)
{
    LOG("KEY scroll");
    jbxvt_set_scroll(xc,
        jbxvt_get_scroll() + mod);

}
__attribute__((const))
static inline bool is_page_up(const uint8_t v)
{
    return v == '5';
}
__attribute__((const))
static inline bool is_page_down(const uint8_t v)
{
    return v == '6';
}
// returns true if parent should return
static bool shift_page_up_down_scroll(xcb_connection_t * xc,
    const uint16_t state, const int_fast16_t pcount, uint8_t * s)
{
    enum {SCROLL_AMOUNT = 10};
    bool rval = true;
    if (state != XCB_MOD_MASK_SHIFT || pcount <= 2)
        rval = false
    /* The following implements a hook into keyboard
       input for shift-pageup/dn scrolling and future
       features.  */
    LOG("Handling shift combination...");
    else if (is_page_up(s[2]))
        page_key_scroll(xc, SCROLL_AMOUNT);
    else if (is_page_down(s[2]))
        page_key_scroll(xc, -SCROLL_AMOUNT);
    else
        rval = false;
    return rval; // true if page up or page down
}
__attribute__((const))
static inline bool is_not_printable(const xcb_keysym_t k)
{
    return k >= 0xffe0;
}
#ifdef DEBUG_KEYS
static void print_s(uint8_t * s, int16_t i)
{
    for (; i >= 0; --i)
        LOG("s[%d]: 0x%x", i, s[i]);
}
#else//!DEBUG_KEYS
#define print_s(s, i)
#endif//DEBUG_KEYS
static uint8_t * handle_keysym(xcb_connection_t * xc,
    uint8_t * kbuf, int_fast16_t * pcount,
    const xcb_keysym_t key, const uint16_t ke_state)
{
    uint8_t * s = get_s(key, (uint8_t *)kbuf);
    if (s) {
        *pcount = jb_strlen((const char *)s);
        print_s(s, *pcount);
        if (shift_page_up_down_scroll(xc, ke_state, *pcount, s))
            s = NULL;
    } else if (!is_not_printable(key)) { // is a printable character
            *pcount = 1;
            kbuf[0] = (uint8_t)key;
            apply_state(ke_state, kbuf);
            s = kbuf;
    }
    return s;
}
//  Convert the keypress event into a string.
uint8_t * jbxvt_lookup_key(xcb_connection_t * xc,
    void * ev, int_fast16_t * pcount)
{
#ifdef LOOKUP_KEY_DUMP_TABLES
    dump();
    exit(0);
#endif//LOOKUP_KEY_DUMP_TABLES
    static uint8_t kbuf[KEY_MAP_BUF_SIZE];
    xcb_key_press_event_t * ke = ev;
    return handle_keysym(xc, kbuf, pcount,
        get_keysym(xc, ke), ke->state);
}
