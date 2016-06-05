/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.  */

#include "command.h"

#include "cmdtok.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "token.h"
#include "ttyinit.h"
#include "wm_del_win.h"
#include "xeventst.h"
#include "xsetup.h"
#include "xvt.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb_keysyms.h>

static struct {
	// chars waiting to be sent to the command:
	uint8_t *send;
	// start and end of queue:
	struct {
		struct xeventst *start, *last;
	} xev;
	struct {
		bool app_cur:1; // cursor keys in app mode
		bool app_kp:1; // application keypad keys set
		bool sun_fn:1; // use sun function key mapping
	} keys;
} command;

/*  Thanks to Rob McMullen for the following function key mapping tables
 *  and code.  */
/*  Structure used to describe the string generated by a function key,
 *  keypad key, etc.  */
struct KeyStrings {
	uint8_t ks_type;	// the way to generate the string (see below)
	uint8_t ks_value;	// value used in creating the string
};

/*  Different values for ks_type which determine how the value is used to
 *  generate the string.
 */
enum KSType {
	KS_TYPE_NONE,		// No output
	KS_TYPE_CHAR,           // as printf("%c",ks_value)
	KS_TYPE_XTERM,          // as printf("\033[%d",ks_value)
	KS_TYPE_SUN,            // as printf("\033[%dz",ks_value)
	KS_TYPE_APPKEY,         // as printf("\033O%c",ks_value)
	KS_TYPE_NONAPP          // as printf("\033[%c",ks_value)
};

/*  Structure used to map a keysym to a string.
 */
struct KeyMaps {
	xcb_keysym_t km_keysym;
	struct KeyStrings km_normal;	/* The usual string */
	struct KeyStrings km_alt;	/* The alternative string */
};

/*  Table of function key mappings
 */
static struct KeyMaps func_key_table[] = {
	{XK_F1,		{KS_TYPE_XTERM,11},	{KS_TYPE_SUN,224}},
	{XK_F2,		{KS_TYPE_XTERM,12},	{KS_TYPE_SUN,225}},
	{XK_F3,		{KS_TYPE_XTERM,13},	{KS_TYPE_SUN,226}},
	{XK_F4,		{KS_TYPE_XTERM,14},	{KS_TYPE_SUN,227}},
	{XK_F5,		{KS_TYPE_XTERM,15},	{KS_TYPE_SUN,228}},
	{XK_F6,		{KS_TYPE_XTERM,17},	{KS_TYPE_SUN,229}},
	{XK_F7,		{KS_TYPE_XTERM,18},	{KS_TYPE_SUN,230}},
	{XK_F8,		{KS_TYPE_XTERM,19},	{KS_TYPE_SUN,231}},
	{XK_F9,		{KS_TYPE_XTERM,20},	{KS_TYPE_SUN,232}},
	{XK_F10,	{KS_TYPE_XTERM,21},	{KS_TYPE_SUN,233}},
	{XK_F11,	{KS_TYPE_XTERM,23},	{KS_TYPE_SUN,192}},
	{XK_F12,	{KS_TYPE_XTERM,24},	{KS_TYPE_SUN,193}},
	{XK_Insert,	{KS_TYPE_XTERM,2},	{KS_TYPE_SUN,2}},
	{XK_Delete,	{KS_TYPE_XTERM,3},	{KS_TYPE_SUN,3}},
	{XK_Page_Up,	{KS_TYPE_XTERM,5},	{KS_TYPE_SUN,5}},
	{XK_Page_Down,	{KS_TYPE_XTERM,6},	{KS_TYPE_SUN,6}},
	{XK_Menu,	{KS_TYPE_XTERM,29},	{KS_TYPE_SUN,197}},
	{0,		{KS_TYPE_NONE,0},	{KS_TYPE_NONE,0}}
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
	{0,		{KS_TYPE_NONE,0},	{KS_TYPE_NONE,0}}
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
	{0,		{KS_TYPE_NONE,0},	{KS_TYPE_NONE,0}}
};

//  Push a mini X event onto the queue
void push_xevent(struct xeventst * xe)
{
	xe->xe_next = command.xev.start;
	xe->xe_prev = NULL;
	if (xe->xe_next)
		xe->xe_next->xe_prev = xe;
	else
		command.xev.last = xe;
}

struct xeventst * pop_xevent(void)
{
	struct xeventst * xe = command.xev.last;
	if(xe) {
		command.xev.last = xe->xe_prev;
		if (command.xev.last)
			  command.xev.last->xe_next = NULL;
		else
			  command.xev.start = NULL;
	}
	return(xe);
}

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.  */
void init_command(char ** restrict argv)
{
	//  Enable the delete window protocol:
	wm_del_win();

	if ((jbxvt.com.fd = run_command(argv)) < 0)
		  quit(1, WARN_RES RES_SSN);
	jbxvt.com.buf.next = jbxvt.com.buf.top = jbxvt.com.buf.data;
	jbxvt.com.stack.top = jbxvt.com.stack.data;
}

// Set key mode for cursor keys if is_cursor, else for keypad keys
void set_keys(const bool mode_high, const bool is_cursor)
{
	if(is_cursor)
		  command.keys.app_cur = mode_high;
	else
		  command.keys.app_kp = mode_high;
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
		switch (ks->ks_type) {
		case KS_TYPE_NONE:
			return NULL;
		case KS_TYPE_CHAR:
			snprintf((char *)buf, KBUFSIZE,
				"%c",ks->ks_value);
			return buf;
		case KS_TYPE_XTERM:
			snprintf((char *)buf, KBUFSIZE,
				"\033[%d~",ks->ks_value);
			return buf;
		case KS_TYPE_SUN:
			snprintf((char *)buf, KBUFSIZE,
				"\033[%dz",ks->ks_value);
			return buf;
		case KS_TYPE_APPKEY:
			snprintf((char *)buf, KBUFSIZE,
				"\033O%c",ks->ks_value);
			return buf;
		case KS_TYPE_NONAPP:
			snprintf((char *)buf, KBUFSIZE,
				"\033[%c",ks->ks_value);
			return buf;
		}
	}
	return NULL;
}

static char * get_s(const xcb_keysym_t keysym, char * restrict kbuf)
{
	if (xcb_is_function_key(keysym) || xcb_is_misc_function_key(keysym)
		|| keysym == XK_Next || keysym == XK_Prior)
		return get_keycode_value(func_key_table, keysym,
			kbuf, command.keys.sun_fn);
	else if (xcb_is_cursor_key(keysym) || xcb_is_pf_key(keysym))
		return get_keycode_value(other_key_table, keysym,
			kbuf, command.keys.app_cur);
	return get_keycode_value(kp_key_table, keysym,
		kbuf, command.keys.app_kp);
}

//  Convert the keypress event into a string.
uint8_t * lookup_key(void * restrict ev, int16_t * restrict pcount)
{
	KeySym keysym;
	static char kbuf[KBUFSIZE];

	XKeyEvent * ke = ev;
	const int16_t count = XLookupString(ke, kbuf, KBUFSIZE, &keysym, NULL);
	char *s = get_s(keysym, kbuf);
	if (s) {
		uint8_t l = 0;
		while (s[++l]);
		*pcount = l;
		return (uint8_t *)s;
	} else {
		if((ke->state & Mod1Mask) && (count == 1))
			kbuf[0] |= 0200;
		*pcount = count;
		return (uint8_t *)kbuf;
	}
}

//  Push an input character back into the input queue.
void push_com_char(const int c)
{
	if (jbxvt.com.stack.top < jbxvt.com.stack.data + COM_PUSH_MAX)
		*jbxvt.com.stack.top++ = c;
}

//  Send count characters directly to the command.
void send_string(uint8_t * restrict buf, const uint8_t count)
{
	if (!count)
		return;

	uint8_t *s1, *s2;
	if (likely(!jbxvt.com.send_count)) {
		if (command.send) {
			free(command.send);
			command.send = NULL;
		}
		command.send = malloc(count);
		s2 = command.send;
		s1 = buf;
		for (uint8_t i = 0; i < count; ++i, ++s1, ++s2)
			*s2 = *s1;
		jbxvt.com.send_nxt = command.send;
		jbxvt.com.send_count = count;
	} else {
		uint8_t * s = malloc(jbxvt.com.send_count + count);
		memcpy(s , jbxvt.com.send_nxt, jbxvt.com.send_count);
		s2 = s + jbxvt.com.send_count;
		s1 = buf;
		for (uint8_t i = 0; i < count; ++i, ++s1, ++s2)
			*s2 = *s1;
		free(command.send);
		command.send = jbxvt.com.send_nxt = s;
		jbxvt.com.send_count += count;
	}
}

/*  Send printf formatted output to the command.  Only used for small ammounts
 *  of data.  */
/*VARARGS1*/
void cprintf(char *fmt,...)
{
	va_list args;
	va_start(args,fmt);
	static uint8_t buf[7];
	// + 1 to include \0 terminator.
	const int l = vsnprintf((char *)buf, sizeof(buf), fmt, args) + 1;
	va_end(args);
	send_string(buf, l);
}


#ifdef TK_DEBUG
//  Print out a token's numerical arguments. Just used by show_token()
static void show_token_args(struct tokenst * restrict tk)
{
	for (uint8_t i = 0; i < tk->tk_nargs; i++) {
		if (i == 0)
			dprintf(STDERR_FILENO, " (%d",tk->tk_arg[i]);
		else
			dprintf(STDERR_FILENO, ",%d",tk->tk_arg[i]);
	}
	if (tk->tk_nargs > 0)
		jbputs(")");
	if (tk->tk_private !=0)
		putchar(tk->tk_private);
}

//  Print out a token's numerical arguments in hex. Just used by show_token()
static void show_hex_token_args(struct tokenst * restrict tk)
{
	int i;

	for (i = 0; i < tk->tk_nargs; i++) {
		dprintf(STDERR_FILENO, i ? ",0x%x" : " (0x%x",
			tk->tk_arg[i]);
	}
	if (tk->tk_nargs > 0)
		  jbputs(")");
	char buf[2] = { tk->tk_private, 0};
	if (tk->tk_private !=0)
		  jbputs(buf);
}

//  Print out the contents of an input token - used for debugging.
void show_token(struct tokenst * tk)
{
	/*  Screen out token types that are not currently of interest.
	 */
	switch (tk->tk_type) {
		case TK_SELDRAG :
			return;
	}

	switch (tk->tk_type) {
		case TK_STRING :
			jbputs("token(TK_STRING): ");
			jbputs((char *)tk->tk_string);
			break;
		case TK_TXTPAR :
			dprintf(STDERR_FILENO, "token(TK_TXTPAR):"
				" %s(%d)", tk->tk_string, tk->tk_arg[0]);
			break;
		case TK_CHAR :
			dprintf(STDERR_FILENO, "token(TK_CHAR): %o",
				tk->tk_char);
			break;
		case TK_EOF :
			jbputs("token(TK_EOF)");
			show_token_args(tk);
			break;
		case TK_FOCUS :
			dprintf(STDERR_FILENO, "token(TK_FOCUS)"
				" <%d>", tk->tk_region);
			show_token_args(tk);
			break;
		case TK_ENTRY :
			dprintf(STDERR_FILENO, "token(TK_ENTRY)"
				" <%d>", tk->tk_region);
			show_token_args(tk);
			break;
		case TK_SBSWITCH :
			jbputs("token(TK_SBSWITCH)");
			show_token_args(tk);
			break;
		case TK_SBGOTO :
			jbputs("token(TK_SBGOTO)");
			show_token_args(tk);
			break;
		case TK_SBUP :
			jbputs("token(TK_SBUP)");
			show_token_args(tk);
			break;
		case TK_SBDOWN :
			jbputs("token(TK_SBDOWN)");
			show_token_args(tk);
			break;
		case TK_EXPOSE :
			dprintf(STDERR_FILENO, "token(TK_EXPOSE)"
				"(%d)",tk->tk_region);
			show_token_args(tk);
			break;
		case TK_RESIZE :
			jbputs("token(TK_RESIZE)");
			show_token_args(tk);
			break;
		case TK_SELSTART :
			jbputs("token(TK_SELSTART)");
			show_token_args(tk);
			break;
		case TK_SELEXTND :
			jbputs("token(TK_SELEXTND)");
			show_token_args(tk);
			break;
		case TK_SELDRAG :
			jbputs("token(TK_SELDRAG)");
			show_token_args(tk);
			break;
		case TK_SELINSRT :
			jbputs("token(TK_SELINSRT)");
			show_token_args(tk);
			break;
		case TK_SELECT :
			jbputs("token(TK_SELECT)");
			show_token_args(tk);
			break;
		case TK_SELWORD :
			jbputs("token(TK_SELWORD)");
			show_token_args(tk);
			break;
		case TK_SELLINE :
			jbputs("token(TK_SELLINE)");
			show_token_args(tk);
			break;
		case TK_SELCLEAR :
			jbputs("token(TK_SELCLEAR)");
			show_token_args(tk);
			break;
		case TK_SELNOTIFY :
			jbputs("token(TK_SELNOTIFY)");
			show_hex_token_args(tk);
			break;
		case TK_SELREQUEST :
			jbputs("token(TK_SELREQUEST)");
			show_hex_token_args(tk);
			break;
		case TK_CUU :
			jbputs("token(TK_CUU)");
			show_token_args(tk);
			break;
		case TK_CUD :
			jbputs("token(TK_CUD)");
			show_token_args(tk);
			break;
		case TK_CUF :
			jbputs("token(TK_CUF)");
			show_token_args(tk);
			break;
		case TK_CUB :
			jbputs("token(TK_CUB)");
			show_token_args(tk);
			break;
		case TK_CUP :
			jbputs("token(TK_CUP)");
			show_token_args(tk);
			break;
		case TK_ED :
			jbputs("token(TK_ED)");
			show_token_args(tk);
			break;
		case TK_EL :
			jbputs("token(TK_EL)");
			show_token_args(tk);
			break;
		case TK_IL :
			jbputs("token(TK_IL)");
			show_token_args(tk);
			break;
		case TK_DL :
			jbputs("token(TK_DL)");
			show_token_args(tk);
			break;
		case TK_DCH :
			jbputs("token(TK_DCH)");
			show_token_args(tk);
			break;
		case TK_ICH :
			jbputs("token(TK_ICH)");
			show_token_args(tk);
			break;
		case TK_DA :
			jbputs("token(TK_DA)");
			show_token_args(tk);
			break;
		case TK_HVP :
			jbputs("token(TK_HVP)");
			show_token_args(tk);
			break;
		case TK_TBC :
			jbputs("token(TK_TBC)");
			show_token_args(tk);
			break;
		case TK_SET :
			jbputs("token(TK_SET)");
			show_token_args(tk);
			break;
		case TK_RESET :
			jbputs("token(TK_RESET)");
			show_token_args(tk);
			break;
		case TK_SGR :
			jbputs("token(TK_SGR)");
			show_token_args(tk);
			break;
		case TK_DSR :
			jbputs("token(TK_DSR)");
			show_token_args(tk);
			break;
		case TK_DECSTBM :
			jbputs("token(TK_DECSTBM)");
			show_token_args(tk);
			break;
		case TK_DECSWH :
			jbputs("token(TK_DECSWH)");
			show_token_args(tk);
			break;
		case TK_SCS0 :
			jbputs("token(TK_SCS0)");
			show_token_args(tk);
			break;
		case TK_SCS1 :
			jbputs("token(TK_SCS1)");
			show_token_args(tk);
			break;
		case TK_DECSC :
			jbputs("token(TK_DECSC)");
			show_token_args(tk);
			break;
		case TK_DECRC :
			jbputs("token(TK_DECRC)");
			show_token_args(tk);
			break;
		case TK_DECPAM :
			jbputs("token(TK_DECPAM)");
			show_token_args(tk);
			break;
		case TK_DECPNM :
			jbputs("token(TK_DECPNM)");
			show_token_args(tk);
			break;
		case TK_IND :
			jbputs("token(TK_IND)");
			show_token_args(tk);
			break;
		case TK_NEL :
			jbputs("token(TK_NEL)");
			show_token_args(tk);
			break;
		case TK_HTS :
			jbputs("token(TK_HTS)");
			show_token_args(tk);
			break;
		case TK_RI :
			jbputs("token(TK_RI)");
			show_token_args(tk);
			break;
		case TK_SS2 :
			jbputs("token(TK_SS2)");
			show_token_args(tk);
			break;
		case TK_SS3 :
			jbputs("token(TK_SS3)");
			show_token_args(tk);
			break;
		case TK_DECID :
			jbputs("token(TK_DECID)");
			show_token_args(tk);
			break;
		case TK_NULL :
			return;
		default :
			dprintf(STDERR_FILENO, "unknown token <%o>",
				tk->tk_type);
			show_token_args(tk);
			break;
	}
	jbputs("\n");
}
#endif /* DEBUG */
