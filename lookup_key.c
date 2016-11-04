// Copyright 2016, Jeffrey E. Bedard
#include "lookup_key.h"
#include "command.h"
#include "libjb/log.h"
#include "sbar.h"
#include <string.h>
#include <xcb/xcb_keysyms.h>
//#define DEBUG_KEYS
#ifndef DEBUG_KEYS
#undef LOG
#define LOG(...)
#endif
static struct { // key modes
	bool cursor, // app mode cursor keys
	     keypad; // keypad keys
} self;
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
	{K_F(1),	{KS_TYPE_APPKEY,'P'},	{KS_TYPE_XTERM,11}},
	{K_F(2),	{KS_TYPE_APPKEY,'Q'},	{KS_TYPE_XTERM,12}},
	{K_F(3),	{KS_TYPE_APPKEY,'R'},	{KS_TYPE_XTERM,13}},
	{K_F(4),	{KS_TYPE_APPKEY,'S'},	{KS_TYPE_XTERM,14}},
	{K_F(5),	{KS_TYPE_XTERM,15}, {}},
	{K_F(6),	{KS_TYPE_XTERM,17}, {}},
	{K_F(7),	{KS_TYPE_XTERM,18}, {}},
	{K_F(8),	{KS_TYPE_XTERM,19}, {}},
	{K_F(9),	{KS_TYPE_XTERM,20}, {}},
	{K_F(10),	{KS_TYPE_XTERM,21}, {}},
	{K_F(11),	{KS_TYPE_XTERM,23}, {}},
	{K_F(12),	{KS_TYPE_XTERM,24}, {}},
	{K_INS,		{KS_TYPE_XTERM,2},  {}},
	{K_DEL,		{KS_TYPE_XTERM,3},  {}},
	{K_PU,		{KS_TYPE_XTERM,5},  {}},
	{K_PD,		{KS_TYPE_XTERM,6},  {}},
	{}
};
//  PC keys and VT100 keypad function keys
static struct JBXVTKeyMaps other_key_table[]={
	// regular:
	{ K_N(2), {KS_TYPE_NONAPP,'A'},{KS_TYPE_APPKEY,'A'}}, // up
	{ K_N(4), {KS_TYPE_NONAPP,'B'},{KS_TYPE_APPKEY,'B'}}, // down
	{ K_N(3), {KS_TYPE_NONAPP,'C'},{KS_TYPE_APPKEY,'C'}}, // left
	{ K_N(1), {KS_TYPE_NONAPP,'D'},{KS_TYPE_APPKEY,'D'}}, // right
	{ K_N(0), {KS_TYPE_NONAPP,'h'},{KS_TYPE_APPKEY,'h'}}, // home
	{ K_N(7), {KS_TYPE_NONAPP,'\0'},{KS_TYPE_APPKEY,'\0'}}, // end
	// keypad:
	{ KP_F(7), {KS_TYPE_NONAPP,'A'},{KS_TYPE_APPKEY,'A'}}, // up
	{ KP_F(9), {KS_TYPE_NONAPP,'B'},{KS_TYPE_APPKEY,'B'}}, // down
	{ KP_F(8), {KS_TYPE_NONAPP,'C'},{KS_TYPE_APPKEY,'C'}}, // left
	{ KP_F(6), {KS_TYPE_NONAPP,'D'},{KS_TYPE_APPKEY,'D'}}, // right
	{ KP_F(5), {KS_TYPE_NONAPP,'h'},{KS_TYPE_APPKEY,'h'}}, // home
	{ KP_F(0xc), {KS_TYPE_NONAPP,'\0'},{KS_TYPE_APPKEY,'\0'}}, // end
	{ KP_F(1), {KS_TYPE_APPKEY,'P'},{KS_TYPE_APPKEY,'P'}}, // f1
	{ KP_F(2), {KS_TYPE_APPKEY,'Q'},{KS_TYPE_APPKEY,'Q'}}, // f2
	{ KP_F(3), {KS_TYPE_APPKEY,'R'},{KS_TYPE_APPKEY,'R'}}, // f3
	{ KP_F(4), {KS_TYPE_APPKEY,'S'},{KS_TYPE_APPKEY,'S'}}, // f4
	{}
};
#define KP_N(n) K_C(0xb0 | n)
//  VT100 numeric keypad keys
static struct JBXVTKeyMaps kp_key_table[]={
	{ KP_N(0),	{KS_TYPE_CHAR,'0'},	{KS_TYPE_APPKEY,'p'}},
	{ KP_N(1),	{KS_TYPE_CHAR,'1'},	{KS_TYPE_APPKEY,'q'}},
	{ KP_N(2),	{KS_TYPE_CHAR,'2'},	{KS_TYPE_APPKEY,'r'}},
	{ KP_N(3),	{KS_TYPE_CHAR,'3'},	{KS_TYPE_APPKEY,'s'}},
	{ KP_N(4),	{KS_TYPE_CHAR,'4'},	{KS_TYPE_APPKEY,'t'}},
	{ KP_N(5),	{KS_TYPE_CHAR,'5'},	{KS_TYPE_APPKEY,'u'}},
	{ KP_N(6),	{KS_TYPE_CHAR,'6'},	{KS_TYPE_APPKEY,'v'}},
	{ KP_N(7),	{KS_TYPE_CHAR,'7'},	{KS_TYPE_APPKEY,'w'}},
	{ KP_N(8),	{KS_TYPE_CHAR,'8'},	{KS_TYPE_APPKEY,'x'}},
	{ KP_N(9),	{KS_TYPE_CHAR,'9'},	{KS_TYPE_APPKEY,'y'}},
	{ K_C(0xab),{KS_TYPE_CHAR,'+'},{KS_TYPE_APPKEY,'k'}},
	{ K_C(0xad),{KS_TYPE_CHAR,'-'},{KS_TYPE_APPKEY,'m'}},
	{ K_C(0xaa),{KS_TYPE_CHAR,'*'},{KS_TYPE_APPKEY,'j'}},
	{ K_C(0xaf),{KS_TYPE_CHAR,'/'},{KS_TYPE_APPKEY,'o'}},
	{ K_C(0xac),{KS_TYPE_CHAR,','},{KS_TYPE_APPKEY,'l'}},
	{ K_C(0xae),{KS_TYPE_CHAR,'.'},{KS_TYPE_APPKEY,'n'}},
	{ K_C(0x8d),{KS_TYPE_CHAR,'\r'},{KS_TYPE_APPKEY,'M'}},
	{ K_C(0x80),{KS_TYPE_CHAR,' '},{KS_TYPE_APPKEY,' '}},
	{ K_C(0x89),{KS_TYPE_CHAR,'\t'},{KS_TYPE_APPKEY,'I'}},
	{}
};
// Set key mode for cursor keys if is_cursor, else for keypad keys
void jbxvt_set_keys(const bool mode_high, const bool is_cursor)
{
	*(is_cursor ? &self.cursor : &self.keypad) = mode_high;
}
static char * get_format(const enum JBXVTKeySymType type)
{
	switch(type) {
	case KS_TYPE_XTERM:
		return "\033[%d~";
	case KS_TYPE_APPKEY:
		return "\033O%c";
	case KS_TYPE_NONAPP:
		return "\033[%c";
	default:
		return "%c";
	}
}
//  Look up function key keycode
static uint8_t * get_keycode_value(struct JBXVTKeyMaps * restrict
	keymaptable, xcb_keysym_t keysym,
	uint8_t* buf, const int use_alternate)
{
	for (struct JBXVTKeyMaps * km = keymaptable; km->km_keysym; ++km) {
		if (km->km_keysym != keysym)
			  continue;
		struct JBXVTKeyStrings * ks = use_alternate
			? &km->km_alt : &km->km_normal;
		snprintf((char *)buf, KBUFSIZE, get_format(ks->ks_type),
			ks->ks_value);
		return buf;
	}
	return NULL;
}
static uint8_t * get_s(const xcb_keysym_t keysym, uint8_t * restrict kbuf)
{
	if (xcb_is_function_key(keysym) || xcb_is_misc_function_key(keysym)
		|| keysym == K_PD || keysym == K_PU)
		return get_keycode_value(func_key_table, keysym, kbuf,
			false);
	if (xcb_is_cursor_key(keysym) || xcb_is_pf_key(keysym))
		return get_keycode_value(other_key_table, keysym,
			kbuf, self.cursor);
	return get_keycode_value(kp_key_table, keysym,
		kbuf, self.keypad);
}
/* FIXME: Make this portable to non-US keyboards, or write a version
   or table for each type.  Perhaps use libxkbcommon-x11.  */
static const uint8_t shift_map[][2] = {{'1', '!'}, {'2', '@'}, {'3', '#'},
	{'4', '$'}, {'5', '%'}, {'6', '^'}, {'7', '&'}, {'8', '*'},
	{'9', '('}, {'0', ')'}, {'-', '_'}, {'=', '+'}, {';', ':'},
	{'\'', '"'}, {'[', '{'}, {']', '}'}, {'\\', '|'}, {'`', '~'},
	{',', '<'}, {'.', '>'}, {'/', '?'}, {}};
__attribute__((const))
static uint8_t shift(uint8_t c)
{
	if (c >= 'a' && c <= 'z')
		return c - 0x20; // c - SPACE
	for (uint8_t i = 0; shift_map[i][0]; ++i) {
		if (shift_map[i][0] == c)
			  return shift_map[i][1];
	}
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
#ifdef KEY_DEBUG
	LOG("keycode: 0x%x, keysym: 0x%x, state: 0x%x",
		ke->detail, k, ke->state);
#endif//KEY_DEBUG
	xcb_key_symbols_free(syms);
	return k;
}
//  Convert the keypress event into a string.
uint8_t * jbxvt_lookup_key(xcb_connection_t * restrict xc,
	void * restrict ev, int_fast16_t * restrict pcount)
{
	static uint8_t kbuf[KBUFSIZE];
	xcb_key_press_event_t * ke = ev;
	const xcb_keysym_t k = get_keysym(xc, ke);
	uint8_t * s = get_s(k, (uint8_t *)kbuf);
	if (s) {
		*pcount = strlen((const char *)s);
#ifdef KEY_DEBUG
		for (int_fast16_t i = *pcount; i >= 0; --i)
			LOG("s[%d]: 0x%x", i, s[i]);
#endif//KEY_DEBUG
		/* The following implements a hook into keyboard input
		 for shift-pageup/dn scrolling and future features.  */
		if (ke->state == XCB_MOD_MASK_SHIFT && *pcount > 2) {
			LOG("Handling shift combination...");
			switch (s[2]) {
			case '5':
				LOG("KEY scroll down");
				jbxvt_set_scroll(xc, jbxvt_get_scroll() + 10);
				goto do_not_display;
				break;
			case '6':
				LOG("KEY scroll up");
				jbxvt_set_scroll(xc, jbxvt_get_scroll() - 10);
				goto do_not_display;
				break;
			}
		}
		return s;
	}
	if (k >= 0xffe0) { // Don't display non-printable chars
do_not_display:
		*pcount = 0;
		return NULL;
	}
	kbuf[0] = k;
	apply_state(ke->state, kbuf);
#ifdef KEY_DEBUG
	LOG("kbuf: 0x%hhx", kbuf[0]);
#endif//KEY_DEBUG
	*pcount = 1;
	return (uint8_t *)kbuf;
}
