/*  Copyright 1992, 1993 John Bovey, University of Kent at Canterbury.
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

#include "command.h"

#include "cmdtok.h"
#include "jbxvt.h"
#include "log.h"
#include "screen.h"
#include "token.h"
#include "xeventst.h"
#include "xsetup.h"
#include "xvt.h"
#include "ttyinit.h"
#include "wm_del_win.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
struct keystringst {
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
struct keymapst {
	KeySym km_keysym;
	struct keystringst km_normal;	/* The usual string */
	struct keystringst km_alt;	/* The alternative string */
};

/*  Table of function key mappings
 */
static struct keymapst func_key_table[] = {
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
	{XK_F13,	{KS_TYPE_XTERM,25},	{KS_TYPE_SUN,194}},
	{XK_F14,	{KS_TYPE_XTERM,26},	{KS_TYPE_SUN,195}},
	{XK_F15,	{KS_TYPE_XTERM,28},	{KS_TYPE_SUN,196}},
	{XK_F16,	{KS_TYPE_XTERM,29},	{KS_TYPE_SUN,197}},
	{XK_F17,	{KS_TYPE_XTERM,31},	{KS_TYPE_SUN,198}},
	{XK_F18,	{KS_TYPE_XTERM,32},	{KS_TYPE_SUN,199}},
	{XK_F19,	{KS_TYPE_XTERM,33},	{KS_TYPE_SUN,200}},
	{XK_F20,	{KS_TYPE_XTERM,34},	{KS_TYPE_SUN,201}},
	{XK_F21,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,208}},
	{XK_F22,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,209}},
	{XK_F23,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,210}},
	{XK_F24,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,211}},
	{XK_F25,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,212}},
	{XK_F26,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,213}},
	{XK_F27,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,214}},
	{XK_F28,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,215}},
	{XK_F29,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,216}},
	{XK_F30,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,217}},
	{XK_F31,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,218}},
	{XK_F32,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,219}},
	{XK_F33,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,220}},
	{XK_F34,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,221}},
	{XK_F35,	{KS_TYPE_NONE,0},	{KS_TYPE_SUN,222}},
	{XK_Find,	{KS_TYPE_XTERM,1},	{KS_TYPE_SUN,1}},
	{XK_Insert,	{KS_TYPE_XTERM,2},	{KS_TYPE_SUN,2}},
	{XK_Delete,	{KS_TYPE_XTERM,3},	{KS_TYPE_SUN,3}},
	{XK_Select,	{KS_TYPE_XTERM,4},	{KS_TYPE_SUN,4}},
	{XK_Prior,	{KS_TYPE_XTERM,5},	{KS_TYPE_SUN,5}},
	{XK_Next,	{KS_TYPE_XTERM,6},	{KS_TYPE_SUN,6}},
	{XK_Help,	{KS_TYPE_XTERM,28},	{KS_TYPE_SUN,196}},
	{XK_Menu,	{KS_TYPE_XTERM,29},	{KS_TYPE_SUN,197}},
	{0,		{KS_TYPE_NONE,0},	{KS_TYPE_NONE,0}}
};

//  PC keys and VT100 keypad function keys
static struct keymapst other_key_table[]={
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
static struct keymapst kp_key_table[]={
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
void push_xevent(struct xeventst *xe)
{
	xe->xe_next = command.xev.start;
	xe->xe_prev = NULL;
	if (xe->xe_next != NULL)
		xe->xe_next->xe_prev = xe;
	else
		command.xev.last = xe;
}

struct xeventst * pop_xevent(void)
{
	struct xeventst *xe;

	if (command.xev.last == NULL)
		return(NULL);

	xe = command.xev.last;
	command.xev.last = xe->xe_prev;
	if (command.xev.last != NULL)
		command.xev.last->xe_next = NULL;
	else
		command.xev.start = NULL;
	return(xe);
}

/*  Initialise the command connection.  This should be called after the X
 *  server connection is established.
 */
void init_command(char ** restrict argv)
{
	//  Enable the delete window protocol:
	init_wm_del_win();

	if ((jbxvt.com.fd = run_command(argv)) < 0) {
		sleep(1);
		perror("Quitting");
		quit(1);
	}
	jbxvt.com.buf.next = jbxvt.com.buf.top = jbxvt.com.buf.data;
	jbxvt.com.stack.top = jbxvt.com.stack.data;
	init_cmdtok();
}

//  Set the current cursor keys mode.
void set_cur_keys(const int mode)
{
	command.keys.app_cur = (mode == HIGH);
}

//  Set the current keypad keys mode.
void set_kp_keys(const int mode)
{
	command.keys.app_kp = (mode == HIGH);
}

//  Look up function key keycode
static char * get_keycode_value(struct keymapst * restrict keymaptable,
	KeySym keysym, char * buf, const int use_alternate)
{
	struct keymapst *km;
	struct keystringst *ks;

	for (km = keymaptable; km->km_keysym != 0; km++) {
		if (km->km_keysym == keysym) {
			ks = use_alternate ? &km->km_alt : &km->km_normal;
			switch (ks->ks_type) {
			    case KS_TYPE_NONE:
				return NULL;
			    case KS_TYPE_CHAR:
				sprintf((char *)buf,"%c",ks->ks_value);
				return buf;
			    case KS_TYPE_XTERM:
				sprintf((char *)buf,"\033[%d~",ks->ks_value);
				return buf;
			    case KS_TYPE_SUN:
				sprintf((char *)buf,"\033[%dz",ks->ks_value);
				return buf;
			    case KS_TYPE_APPKEY:
				sprintf((char *)buf,"\033O%c",ks->ks_value);
				return buf;
			    case KS_TYPE_NONAPP:
				sprintf((char *)buf,"\033[%c",ks->ks_value);
				return buf;
			}
		}
	}
	return NULL;
}

static char * get_s(const KeySym keysym, char * restrict kbuf)
{
	if (IsFunctionKey(keysym) || IsMiscFunctionKey(keysym)
		|| keysym == XK_Next || keysym == XK_Prior)
		return get_keycode_value(func_key_table,keysym,
			kbuf,command.keys.sun_fn);
	else if (IsCursorKey(keysym) || IsPFKey(keysym))
		return get_keycode_value(other_key_table,keysym,
			kbuf,command.keys.app_cur);
	return get_keycode_value(kp_key_table,keysym,
		kbuf,command.keys.app_kp);
}

//  Convert the keypress event into a string.
uint8_t * lookup_key(XEvent * restrict ev, int * restrict pcount)
{
	KeySym keysym;
	static char kbuf[KBUFSIZE];

	const int count = XLookupString(&ev->xkey,kbuf,KBUFSIZE,&keysym,NULL);
	char *s = get_s(keysym, kbuf);
	if (s) {
		*pcount = strlen(s);
 	       	return (uint8_t *)s;
	} else {
		if ((ev->xkey.state & Mod1Mask) && (count == 1)) {
			kbuf[0] |= 0200;
			*pcount = 1;
		} else
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
void send_string(uint8_t * buf, int count)
{
	uint8_t *s;
	uint8_t *s1, *s2;
	int i;

	if (count == 0)
		return;

	if (jbxvt.com.send_count == 0) {
		if (command.send != NULL) {
			free(command.send);
			command.send = NULL;
		}
		command.send = (uint8_t *)malloc(count);
		s2 = command.send;
		s1 = buf;
		for (i = 0; i < count; i++, s1++, s2++)
			*s2 = *s1;
		jbxvt.com.send_nxt = command.send;
		jbxvt.com.send_count = count;
	} else {
		s = (uint8_t *)malloc(jbxvt.com.send_count + count);
		memcpy(s,jbxvt.com.send_nxt,jbxvt.com.send_count);
		s2 = s + jbxvt.com.send_count;
		s1 = buf;
		for (i = 0; i < count; i++, s1++, s2++)
			*s2 = *s1;
		free(command.send);
		command.send = jbxvt.com.send_nxt = s;
		jbxvt.com.send_count += count;
	}
}

/*  Send printf formatted output to the command.  Only used for small ammounts
 *  of data.
 */
/*VARARGS1*/
void cprintf(char *fmt,...)
{
	va_list args;
	va_start(args,fmt);
	static uint8_t buf[1024];
	vsnprintf((char *)buf, sizeof(buf), fmt, args);
	va_end(args);
	send_string(buf,strlen((char *)buf));
}


#ifdef TK_DEBUG
//  Print out a token's numerical arguments. Just used by show_token()
static void show_token_args(struct tokenst * restrict tk)
{
	int i;

	for (i = 0; i < tk->tk_nargs; i++) {
		if (i == 0)
			printf(" (%d",tk->tk_arg[i]);
		else
			printf(",%d",tk->tk_arg[i]);
	}
	if (tk->tk_nargs > 0)
		printf(")");
	if (tk->tk_private !=0)
		putchar(tk->tk_private);
}

//  Print out a token's numerical arguments in hex. Just used by show_token()
static void show_hex_token_args(struct tokenst * restrict tk)
{
	int i;

	for (i = 0; i < tk->tk_nargs; i++) {
		if (i == 0)
			printf(" (0x%x",tk->tk_arg[i]);
		else
			printf(",0x%x",tk->tk_arg[i]);
	}
	if (tk->tk_nargs > 0)
		printf(")");
	if (tk->tk_private !=0)
		putchar(tk->tk_private);
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
			printf("token(TK_STRING)");
			printf(" \"%s\"",tk->tk_string);
			break;
		case TK_TXTPAR :
			printf("token(TK_TXTPAR)");
			printf(" (%d) \"%s\"",tk->tk_arg[0],tk->tk_string);
			break;
		case TK_CHAR :
			printf("token(TK_CHAR)");
			printf(" <%o>",tk->tk_char);
			break;
		case TK_EOF :
			printf("token(TK_EOF)");
			show_token_args(tk);
			break;
		case TK_FOCUS :
			printf("token(TK_FOCUS)");
			printf(" <%d>",tk->tk_region);
			show_token_args(tk);
			break;
		case TK_ENTRY :
			printf("token(TK_ENTRY)");
			printf(" <%d>",tk->tk_region);
			show_token_args(tk);
			break;
		case TK_SBSWITCH :
			printf("token(TK_SBSWITCH)");
			show_token_args(tk);
			break;
		case TK_SBGOTO :
			printf("token(TK_SBGOTO)");
			show_token_args(tk);
			break;
		case TK_SBUP :
			printf("token(TK_SBUP)");
			show_token_args(tk);
			break;
		case TK_SBDOWN :
			printf("token(TK_SBDOWN)");
			show_token_args(tk);
			break;
		case TK_EXPOSE :
			printf("token(TK_EXPOSE)");
			printf("(%d)",tk->tk_region);
			show_token_args(tk);
			break;
		case TK_RESIZE :
			printf("token(TK_RESIZE)");
			show_token_args(tk);
			break;
		case TK_SELSTART :
			printf("token(TK_SELSTART)");
			show_token_args(tk);
			break;
		case TK_SELEXTND :
			printf("token(TK_SELEXTND)");
			show_token_args(tk);
			break;
		case TK_SELDRAG :
			printf("token(TK_SELDRAG)");
			show_token_args(tk);
			break;
		case TK_SELINSRT :
			printf("token(TK_SELINSRT)");
			show_token_args(tk);
			break;
		case TK_SELECT :
			printf("token(TK_SELECT)");
			show_token_args(tk);
			break;
		case TK_SELWORD :
			printf("token(TK_SELWORD)");
			show_token_args(tk);
			break;
		case TK_SELLINE :
			printf("token(TK_SELLINE)");
			show_token_args(tk);
			break;
		case TK_SELCLEAR :
			printf("token(TK_SELCLEAR)");
			show_token_args(tk);
			break;
		case TK_SELNOTIFY :
			printf("token(TK_SELNOTIFY)");
			show_hex_token_args(tk);
			break;
		case TK_SELREQUEST :
			printf("token(TK_SELREQUEST)");
			show_hex_token_args(tk);
			break;
		case TK_CUU :
			printf("token(TK_CUU)");
			show_token_args(tk);
			break;
		case TK_CUD :
			printf("token(TK_CUD)");
			show_token_args(tk);
			break;
		case TK_CUF :
			printf("token(TK_CUF)");
			show_token_args(tk);
			break;
		case TK_CUB :
			printf("token(TK_CUB)");
			show_token_args(tk);
			break;
		case TK_CUP :
			printf("token(TK_CUP)");
			show_token_args(tk);
			break;
		case TK_ED :
			printf("token(TK_ED)");
			show_token_args(tk);
			break;
		case TK_EL :
			printf("token(TK_EL)");
			show_token_args(tk);
			break;
		case TK_IL :
			printf("token(TK_IL)");
			show_token_args(tk);
			break;
		case TK_DL :
			printf("token(TK_DL)");
			show_token_args(tk);
			break;
		case TK_DCH :
			printf("token(TK_DCH)");
			show_token_args(tk);
			break;
		case TK_ICH :
			printf("token(TK_ICH)");
			show_token_args(tk);
			break;
		case TK_DA :
			printf("token(TK_DA)");
			show_token_args(tk);
			break;
		case TK_HVP :
			printf("token(TK_HVP)");
			show_token_args(tk);
			break;
		case TK_TBC :
			printf("token(TK_TBC)");
			show_token_args(tk);
			break;
		case TK_SET :
			printf("token(TK_SET)");
			show_token_args(tk);
			break;
		case TK_RESET :
			printf("token(TK_RESET)");
			show_token_args(tk);
			break;
		case TK_SGR :
			printf("token(TK_SGR)");
			show_token_args(tk);
			break;
		case TK_DSR :
			printf("token(TK_DSR)");
			show_token_args(tk);
			break;
		case TK_DECSTBM :
			printf("token(TK_DECSTBM)");
			show_token_args(tk);
			break;
		case TK_DECSWH :
			printf("token(TK_DECSWH)");
			show_token_args(tk);
			break;
		case TK_SCS0 :
			printf("token(TK_SCS0)");
			show_token_args(tk);
			break;
		case TK_SCS1 :
			printf("token(TK_SCS1)");
			show_token_args(tk);
			break;
		case TK_DECSC :
			printf("token(TK_DECSC)");
			show_token_args(tk);
			break;
		case TK_DECRC :
			printf("token(TK_DECRC)");
			show_token_args(tk);
			break;
		case TK_DECPAM :
			printf("token(TK_DECPAM)");
			show_token_args(tk);
			break;
		case TK_DECPNM :
			printf("token(TK_DECPNM)");
			show_token_args(tk);
			break;
		case TK_IND :
			printf("token(TK_IND)");
			show_token_args(tk);
			break;
		case TK_NEL :
			printf("token(TK_NEL)");
			show_token_args(tk);
			break;
		case TK_HTS :
			printf("token(TK_HTS)");
			show_token_args(tk);
			break;
		case TK_RI :
			printf("token(TK_RI)");
			show_token_args(tk);
			break;
		case TK_SS2 :
			printf("token(TK_SS2)");
			show_token_args(tk);
			break;
		case TK_SS3 :
			printf("token(TK_SS3)");
			show_token_args(tk);
			break;
		case TK_DECID :
			printf("token(TK_DECID)");
			show_token_args(tk);
			break;
		case TK_NULL :
			return;
		default :
			printf("unknown token <%o>",tk->tk_type);
			show_token_args(tk);
			break;
	}
	printf("\n");
}
#endif /* DEBUG */
