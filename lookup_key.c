// Copyright 2017, Jeffrey E. Bedard
#undef DEBUG
#include "lookup_key.h"
#include <stdio.h>
#include <string.h>
#include <xcb/xcb_keysyms.h>
#include "JBXVTKeySymType.h"
#include "command.h"
#include "libjb/log.h"
#include "sbar.h"
enum KeyboardModes {
	CURSOR_MODE = 1, KEYPAD_MODE = 2
};
uint8_t keyboard_mode;
// Reference <X11/keysymdef.h>
#define K_C(n) (0xff00 | n)
#define K_INS K_C(0x63)
#define K_DEL K_C(0x9f)
#define K_F(n) (0xffbd + n)
// Keypad keys:
#define KP_F(n) K_C(0x90 | n)
// Regular keys:
#define K_N(n) K_C(0x50 | n)
#define K_PU K_N(5)
#define K_PD K_N(6)
//  Table of function key mappings
static struct JBXVTKeyMaps func_key_table[] = {
	{K_F(1),	{APPKEY_KS,'P'},	{XTERM_KS,11}},
	{K_F(2),	{APPKEY_KS,'Q'},	{XTERM_KS,12}},
	{K_F(3),	{APPKEY_KS,'R'},	{XTERM_KS,13}},
	{K_F(4),	{APPKEY_KS,'S'},	{XTERM_KS,14}},
	{K_F(5),	{XTERM_KS,15}, {}},
	{K_F(6),	{XTERM_KS,17}, {}},
	{K_F(7),	{XTERM_KS,18}, {}},
	{K_F(8),	{XTERM_KS,19}, {}},
	{K_F(9),	{XTERM_KS,20}, {}},
	{K_F(10),	{XTERM_KS,21}, {}},
	{K_F(11),	{XTERM_KS,23}, {}},
	{K_F(12),	{XTERM_KS,24}, {}},
	{K_INS,		{XTERM_KS,2},  {}},
	{K_DEL,		{XTERM_KS,3},  {}},
	{K_PU,		{XTERM_KS,5},  {}},
	{K_PD,		{XTERM_KS,6},  {}},
	{}
};
//  PC keys and VT100 keypad function keys
static struct JBXVTKeyMaps other_key_table[] = {
	// regular:
	{ K_N(2), {NONAPP_KS,'A'},{APPKEY_KS,'A'}}, // up
	{ K_N(4), {NONAPP_KS,'B'},{APPKEY_KS,'B'}}, // down
	{ K_N(3), {NONAPP_KS,'C'},{APPKEY_KS,'C'}}, // left
	{ K_N(1), {NONAPP_KS,'D'},{APPKEY_KS,'D'}}, // right
	{ K_N(0), {NONAPP_KS,'h'},{APPKEY_KS,'h'}}, // home
	{ K_N(7), {NONAPP_KS,'\0'},{APPKEY_KS,'\0'}}, // end
	// keypad:
	{ KP_F(7), {NONAPP_KS,'A'},{APPKEY_KS,'A'}}, // up
	{ KP_F(9), {NONAPP_KS,'B'},{APPKEY_KS,'B'}}, // down
	{ KP_F(8), {NONAPP_KS,'C'},{APPKEY_KS,'C'}}, // left
	{ KP_F(6), {NONAPP_KS,'D'},{APPKEY_KS,'D'}}, // right
	{ KP_F(5), {NONAPP_KS,'h'},{APPKEY_KS,'h'}}, // home
	{ KP_F(0xc), {NONAPP_KS,'\0'},{APPKEY_KS,'\0'}}, // end
	{ KP_F(1), {APPKEY_KS,'P'},{APPKEY_KS,'P'}}, // f1
	{ KP_F(2), {APPKEY_KS,'Q'},{APPKEY_KS,'Q'}}, // f2
	{ KP_F(3), {APPKEY_KS,'R'},{APPKEY_KS,'R'}}, // f3
	{ KP_F(4), {APPKEY_KS,'S'},{APPKEY_KS,'S'}}, // f4
	{}
};
#define KP_N(n) K_C(0xb0 | n)
//  VT100 numeric keypad keys
static struct JBXVTKeyMaps kp_key_table[]={
	{ KP_N(0),	{CHAR_KS,'0'},	{APPKEY_KS,'p'}},
	{ KP_N(1),	{CHAR_KS,'1'},	{APPKEY_KS,'q'}},
	{ KP_N(2),	{CHAR_KS,'2'},	{APPKEY_KS,'r'}},
	{ KP_N(3),	{CHAR_KS,'3'},	{APPKEY_KS,'s'}},
	{ KP_N(4),	{CHAR_KS,'4'},	{APPKEY_KS,'t'}},
	{ KP_N(5),	{CHAR_KS,'5'},	{APPKEY_KS,'u'}},
	{ KP_N(6),	{CHAR_KS,'6'},	{APPKEY_KS,'v'}},
	{ KP_N(7),	{CHAR_KS,'7'},	{APPKEY_KS,'w'}},
	{ KP_N(8),	{CHAR_KS,'8'},	{APPKEY_KS,'x'}},
	{ KP_N(9),	{CHAR_KS,'9'},	{APPKEY_KS,'y'}},
	{ K_C(0xab),	{CHAR_KS,'+'},	{APPKEY_KS,'k'}},
	{ K_C(0xad),	{CHAR_KS,'-'},	{APPKEY_KS,'m'}},
	{ K_C(0xaa),	{CHAR_KS,'*'},	{APPKEY_KS,'j'}},
	{ K_C(0xaf),	{CHAR_KS,'/'},	{APPKEY_KS,'o'}},
	{ K_C(0xac),	{CHAR_KS,','},	{APPKEY_KS,'l'}},
	{ K_C(0xae),	{CHAR_KS,'.'},	{APPKEY_KS,'n'}},
	{ K_C(0x8d),	{CHAR_KS,'\r'},	{APPKEY_KS,'M'}},
	{ K_C(0x80),	{CHAR_KS,' '},	{APPKEY_KS,' '}},
	{ K_C(0x89),	{CHAR_KS,'\t'},	{APPKEY_KS,'I'}},
	{}
};
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
static uint8_t * get_buffer(uint8_t * restrict buf,
	struct JBXVTKeyMaps * restrict keymaptable,
	const bool use_alternate)
{
	struct JBXVTKeyStrings * ks = use_alternate
		? &keymaptable->km_alt : &keymaptable->km_normal;
	snprintf((char *)buf, KBUFSIZE, get_format(ks->ks_type),
		ks->ks_value);
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
static uint8_t * get_s(const xcb_keysym_t keysym, uint8_t * restrict kbuf)
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
static void apply_state(const uint16_t state, uint8_t * restrict kbuf)
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
static xcb_keysym_t get_keysym(xcb_connection_t * restrict c,
	xcb_key_press_event_t * restrict ke)
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
static bool shift_page_up_down_scroll(xcb_connection_t * restrict xc,
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
static uint8_t * handle_keysym(xcb_connection_t * restrict xc,
	uint8_t * restrict kbuf, int_fast16_t * restrict pcount,
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
uint8_t * jbxvt_lookup_key(xcb_connection_t * restrict xc,
	void * restrict ev, int_fast16_t * restrict pcount)
{
	static uint8_t kbuf[KBUFSIZE];
	xcb_key_press_event_t * ke = ev;
	return handle_keysym(xc, kbuf, pcount,
		get_keysym(xc, ke), ke->state);
}
