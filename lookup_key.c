#include "lookup_key.h"

#include "command.h"
#include "jbxvt.h"
#include "libjb/log.h"

#include <stdio.h>
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
// Not part of xcb (yet):
#include <X11/keysym.h>

static bool lk_app_cur;		// cursor keys in app mode
static bool lk_app_kp;		// app keypad keys set

//  Table of function key mappings
static struct KeyMaps func_key_table[] = {
	{XK_F1,		{KS_TYPE_APPKEY,'P'},	{KS_TYPE_XTERM,11}},
	{XK_F2,		{KS_TYPE_APPKEY,'Q'},	{KS_TYPE_XTERM,12}},
	{XK_F3,		{KS_TYPE_APPKEY,'R'},	{KS_TYPE_XTERM,13}},
	{XK_F4,		{KS_TYPE_APPKEY,'S'},	{KS_TYPE_XTERM,14}},
	{XK_F5,		{KS_TYPE_XTERM,15}, {}},
	{XK_F6,		{KS_TYPE_XTERM,17}, {}},
	{XK_F7,		{KS_TYPE_XTERM,18}, {}},
	{XK_F8,		{KS_TYPE_XTERM,19}, {}},
	{XK_F9,		{KS_TYPE_XTERM,20}, {}},
	{XK_F10,	{KS_TYPE_XTERM,21}, {}},
	{XK_F11,	{KS_TYPE_XTERM,23}, {}},
	{XK_F12,	{KS_TYPE_XTERM,24}, {}},
	{XK_Insert,	{KS_TYPE_XTERM,2},  {}},
	{XK_Delete,	{KS_TYPE_XTERM,3},  {}},
	{XK_Page_Up,	{KS_TYPE_XTERM,5},  {}},
	{XK_Page_Down,	{KS_TYPE_XTERM,6},  {}},
	{XK_Menu,	{KS_TYPE_XTERM,29}, {}},
	{}
};

//  PC keys and VT100 keypad function keys
static struct KeyMaps other_key_table[]={
	{ XK_Up,	{KS_TYPE_NONAPP,'A'},	{KS_TYPE_APPKEY,'A'}},
	{ XK_Down,	{KS_TYPE_NONAPP,'B'},	{KS_TYPE_APPKEY,'B'}},
	{ XK_Right,	{KS_TYPE_NONAPP,'C'},	{KS_TYPE_APPKEY,'C'}},
	{ XK_Left,	{KS_TYPE_NONAPP,'D'},	{KS_TYPE_APPKEY,'D'}},
	{ XK_Home,	{KS_TYPE_NONAPP,'h'},	{KS_TYPE_APPKEY,'h'}},
	{ XK_End,	{KS_TYPE_NONAPP,'\0'},	{KS_TYPE_APPKEY,'\0'}},
	{ XK_KP_F1,	{KS_TYPE_APPKEY,'P'},	{KS_TYPE_APPKEY,'P'}},
	{ XK_KP_F2,	{KS_TYPE_APPKEY,'Q'},	{KS_TYPE_APPKEY,'Q'}},
	{ XK_KP_F3,	{KS_TYPE_APPKEY,'R'},	{KS_TYPE_APPKEY,'R'}},
	{ XK_KP_F4,	{KS_TYPE_APPKEY,'S'},	{KS_TYPE_APPKEY,'S'}},
	{}
};

//  VT100 numeric keypad keys
static struct KeyMaps kp_key_table[]={
	{ XK_KP_0,	{KS_TYPE_CHAR,'0'},	{KS_TYPE_APPKEY,'p'}},
	{ XK_KP_1,	{KS_TYPE_CHAR,'1'},	{KS_TYPE_APPKEY,'q'}},
	{ XK_KP_2,	{KS_TYPE_CHAR,'2'},	{KS_TYPE_APPKEY,'r'}},
	{ XK_KP_3,	{KS_TYPE_CHAR,'3'},	{KS_TYPE_APPKEY,'s'}},
	{ XK_KP_4,	{KS_TYPE_CHAR,'4'},	{KS_TYPE_APPKEY,'t'}},
	{ XK_KP_5,	{KS_TYPE_CHAR,'5'},	{KS_TYPE_APPKEY,'u'}},
	{ XK_KP_6,	{KS_TYPE_CHAR,'6'},	{KS_TYPE_APPKEY,'v'}},
	{ XK_KP_7,	{KS_TYPE_CHAR,'7'},	{KS_TYPE_APPKEY,'w'}},
	{ XK_KP_8,	{KS_TYPE_CHAR,'8'},	{KS_TYPE_APPKEY,'x'}},
	{ XK_KP_9,	{KS_TYPE_CHAR,'9'},	{KS_TYPE_APPKEY,'y'}},
	{ XK_KP_Add,	{KS_TYPE_CHAR,'+'},	{KS_TYPE_APPKEY,'k'}},
	{ XK_KP_Subtract,{KS_TYPE_CHAR,'-'},	{KS_TYPE_APPKEY,'m'}},
	{ XK_KP_Multiply,{KS_TYPE_CHAR,'*'},	{KS_TYPE_APPKEY,'j'}},
	{ XK_KP_Divide,	{KS_TYPE_CHAR,'/'},	{KS_TYPE_APPKEY,'o'}},
	{ XK_KP_Separator,{KS_TYPE_CHAR,','},	{KS_TYPE_APPKEY,'l'}},
	{ XK_KP_Decimal,{KS_TYPE_CHAR,'.'},	{KS_TYPE_APPKEY,'n'}},
	{ XK_KP_Enter,	{KS_TYPE_CHAR,'\r'},	{KS_TYPE_APPKEY,'M'}},
	{ XK_KP_Space,	{KS_TYPE_CHAR,' '},	{KS_TYPE_APPKEY,' '}},
	{ XK_KP_Tab,	{KS_TYPE_CHAR,'\t'},	{KS_TYPE_APPKEY,'I'}},
	{}
};

// Set key mode for cursor keys if is_cursor, else for keypad keys
void set_keys(const bool mode_high, const bool is_cursor)
{
	*(is_cursor ? &lk_app_cur : &lk_app_kp) = mode_high;
}

static char * get_format(const enum KSType type)
{
	switch(type) {
	case KS_TYPE_XTERM:
		return "\033[%d~";
	case KS_TYPE_SUN:
		return "\033[%dz";
	case KS_TYPE_APPKEY:
		return "\033O%c";
	case KS_TYPE_NONAPP:
		return "\033[%c";
	default:
		return "%c";
	}
}

//  Look up function key keycode
static char * get_keycode_value(struct KeyMaps * restrict keymaptable,
	xcb_keysym_t keysym, char * buf, const int use_alternate)
{
	for (struct KeyMaps * km = keymaptable; km->km_keysym; ++km) {
		if (km->km_keysym != keysym)
			  continue;
		struct KeyStrings * ks = use_alternate
			? &km->km_alt : &km->km_normal;
		snprintf(buf, KBUFSIZE, get_format(ks->ks_type), ks->ks_value);
		return buf;
	}
	return NULL;
}

static char * get_s(const xcb_keysym_t keysym, char * restrict kbuf)
{
	if (xcb_is_function_key(keysym) || xcb_is_misc_function_key(keysym)
		|| keysym == XK_Next || keysym == XK_Prior)
		return get_keycode_value(func_key_table, keysym, kbuf, false);
	if (xcb_is_cursor_key(keysym) || xcb_is_pf_key(keysym))
		return get_keycode_value(other_key_table, keysym,
			kbuf, lk_app_cur);
	return get_keycode_value(kp_key_table, keysym,
		kbuf, lk_app_kp);
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
		return c - 0x20;
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

//  Convert the keypress event into a string.
uint8_t * lookup_key(void * restrict ev, int_fast16_t * restrict pcount)
{
	static uint8_t kbuf[KBUFSIZE];
	xcb_key_press_event_t * ke = ev;
	xcb_key_symbols_t *syms = xcb_key_symbols_alloc(jbxvt.X.xcb);
	xcb_keysym_t k = xcb_key_press_lookup_keysym(syms, ke, 2);
#ifdef KEY_DEBUG
	LOG("keycode: 0x%x, keysym: 0x%x, state: 0x%x",
		ke->detail, k, ke->state);
#endif//KEY_DEBUG
	xcb_key_symbols_free(syms);
	char *s = get_s(k, (char *)kbuf);
	if (s) {
		uint8_t l = 0;
		while (s[++l]);
		*pcount = l;
#ifdef KEY_DEBUG
		for (uint8_t i = 0; i < l; ++i)
			LOG("s[%d]: 0x%x", i, s[i]);
#endif//KEY_DEBUG
		return (uint8_t *)s;
	}
	if (k >= 0xffe0) {
		// Don't display non-printable characters/modifiers:
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


