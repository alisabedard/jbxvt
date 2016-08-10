/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#include "cmdtok.h"

#include "cursor.h"
#include "jbxvt.h"
#include "libjb/log.h"
#include "libjb/xcb.h"
#include "lookup_key.h"
#include "xevents.h"
#include "xeventst.h"

#include <errno.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

//  Special character returned by get_com_char().
enum ComCharReturn {
	GCC_NULL = 0x100, // Input buffer is empty
	CC_ESC = 033,
	CC_IND = 0x84,
	CC_NEL = 0x85,
	CC_HTS = 0x88,
	CC_RI = 0x8d,
	CC_SS2 = 0x8e,
	CC_SS3 = 0x8f,
	CC_DCS = 0x90,
	CC_SPA = 0x96,
	CC_EPA = 0x97,
	CC_SOS = 0x98,
	CC_DECID = 0x9a,
	CC_CSI = 0x9b,
	CC_ST = 0x9c,
	CC_OSC = 0x9d,
	CC_PM = 0x9e,
	CC_APC = 0x9f
};

//  Flags used to control get_com_char();
enum ComCharFlags {BUF_ONLY=1, GET_XEVENTS=2};

static JBXVTEvent * ev_alloc(xcb_generic_event_t * restrict e)
{
	JBXVTEvent * xe = GC_MALLOC(sizeof(JBXVTEvent));
	xe->type = e->response_type;
	return xe;
}

static void push_xevent(JBXVTEvent * xe)
{
	struct JBXVTEventQueue * q = &jbxvt.com.events;
	xe->next = q->start;
	xe->prev = NULL;
	*(xe->next ? &xe->next->prev : &q->last) = xe;
}

static void handle_focus(xcb_generic_event_t * restrict e)
{
	xcb_focus_in_event_t * f = (xcb_focus_in_event_t *)e;
	if (f->mode)
		  return;
	switch (f->detail) {
	case XCB_NOTIFY_DETAIL_ANCESTOR:
	case XCB_NOTIFY_DETAIL_INFERIOR:
	case XCB_NOTIFY_DETAIL_NONLINEAR:
		break;
	default:
		return;
	}
	JBXVTEvent * xe = ev_alloc(e);
	xe->detail = f->detail;
	push_xevent(xe);
}

static void handle_sel(xcb_generic_event_t * restrict ge)
{
	JBXVTEvent * xe = ev_alloc(ge);
	xcb_selection_request_event_t * e
		= (xcb_selection_request_event_t *)ge;
	xe->time = e->time;
	xe->requestor = e->requestor;
	xe->target = e->target;
	xe->property = e->property;
	xe->window = e->owner;
	push_xevent(xe);
}

static void handle_client_msg(xcb_generic_event_t * restrict ge)
{
	xcb_client_message_event_t * e = (xcb_client_message_event_t *)ge;
	if (e->format == 32 && e->data.data32[0]
		== (long)wm_del_win())
		  exit(0);
}

static void handle_expose(xcb_generic_event_t * restrict ge)
{
	xcb_expose_event_t * e = (xcb_expose_event_t *)ge;
	JBXVTEvent * xe = ev_alloc(ge);
	xe->window = e->window;
	xe->box.x = e->x;
	xe->box.y = e->y;
	xe->box.width = e->width;
	xe->box.height = e->height;
	push_xevent(xe);
}

static void handle_other(xcb_generic_event_t * restrict ge)
{
	xcb_key_press_event_t * e = (xcb_key_press_event_t *)ge;
	JBXVTEvent * xe = ev_alloc(ge);
	xe->window = e->event;
	xe->box.x = e->event_x;
	xe->box.y = e->event_y;
	xe->state = e->state;
	xe->button = e->detail;
	xe->time = e->time;
	push_xevent(xe);
}

static int_fast16_t handle_xev(xcb_generic_event_t * restrict event,
	int_fast16_t * restrict count, const uint8_t flags)
{
	uint8_t * s;

	switch (event->response_type & ~0x80) {
	case XCB_KEY_PRESS:
		s = lookup_key(event, count);
		if (count) {
			jbxvt.com.send_nxt = s;
			jbxvt.com.send_count = *count;
		}
		break;
	case XCB_FOCUS_IN:
	case XCB_FOCUS_OUT:
		handle_focus(event);
		break;
	case XCB_SELECTION_REQUEST:
	case XCB_SELECTION_NOTIFY:
		handle_sel(event);
		break;
	case XCB_CLIENT_MESSAGE:
		handle_client_msg(event);
		break;
	case XCB_EXPOSE:
	case XCB_GRAPHICS_EXPOSURE:
		handle_expose(event);
		break;
	default:
		handle_other(event);
	}
	if (flags & GET_XEVENTS)
		  return GCC_NULL;
	return 0;
}

static int_fast16_t output_to_command(void)
{
	const ssize_t count = write(jbxvt.com.fd, jbxvt.com.send_nxt,
		jbxvt.com.send_count);
	if (jb_check(count != -1, "Could not write to command"))
		exit(1);
	jbxvt.com.send_count -= count;
	jbxvt.com.send_nxt += count;
	return count;
}

static void timer(void)
{
	switch(jbxvt.opt.cursor_attr) {
	case 0: // blinking block
	case 1: // blinking block
	case 3: // blinking underline
	case 5: // blinking bar
	case 7: // blinking overline
		if (!jbxvt.mode.att610)
			draw_cursor();
		break;
	}
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((regparm(1)))
#endif//x86
static int_fast16_t poll_io(int_fast16_t count, fd_set * restrict in_fdset)
{
	FD_SET(jbxvt.com.fd, in_fdset);
	FD_SET(jbxvt.com.xfd, in_fdset);
	fd_set out_fdset;
	FD_ZERO(&out_fdset);
	if (jbxvt.com.send_count > 0)
		FD_SET(jbxvt.com.fd, &out_fdset);
	int sv;
	sv = select(jbxvt.com.width, in_fdset, &out_fdset, NULL,
		&(struct timeval){.tv_sec = 0, .tv_usec = 500000});
	if (sv == -1)
		return count;
	if (FD_ISSET(jbxvt.com.fd, &out_fdset))
		return output_to_command();
	if (!FD_ISSET(jbxvt.com.xfd, in_fdset))
		timer(); // select timed out
	else // jbxvt.com.xfd
		jb_check_x(jbxvt.X.xcb);
	return count;
}

/*  Return the next input character after first passing any keyboard input
 *  to the command.  If flags & BUF_ONLY is true then only buffered characters are
 *  returned and once the buffer is empty the special value GCC_NULL is
 *  returned.  If flags and GET_XEVENTS is true then GCC_NULL is returned
 *  when an X event arrives.
 */
// This is the most often called function.
#if defined(__i386__) || defined(__amd64__)
__attribute__((hot,regparm(1)))
#else
__attribute__((hot))
#endif
static int_fast16_t get_com_char(const int_fast8_t flags)
{
	if (jbxvt.com.stack.top > jbxvt.com.stack.data)
		return(*--jbxvt.com.stack.top);

	if (jbxvt.com.buf.next < jbxvt.com.buf.top)
		return(*jbxvt.com.buf.next++);

	if (flags & BUF_ONLY)
		return(GCC_NULL);
	// Flush here to draw the cursor.
	xcb_flush(jbxvt.X.xcb);
	int_fast16_t count = 0;
	fd_set in_fdset;
	do {
		FD_ZERO(&in_fdset);
		xcb_generic_event_t * e;
		if ((e = xcb_poll_for_event(jbxvt.X.xcb))) {
			jb_check_x(jbxvt.X.xcb);
			const int_fast16_t xev_ret
				= handle_xev(e, &count, flags);
			free(e);
			if (xev_ret)
				  return xev_ret;
		}
		count = poll_io(count, &in_fdset);
	} while(!FD_ISSET(jbxvt.com.fd, &in_fdset));
	uint8_t * d = jbxvt.com.buf.data;
	count = read(jbxvt.com.fd, d, COM_BUF_SIZE);
	if (count < 1) // buffer is empty
		return errno == EWOULDBLOCK ? GCC_NULL : EOF;
	jbxvt.com.buf.next = d;
	jbxvt.com.buf.top = d + count;
	return *jbxvt.com.buf.next++;
}

//  Return true if the character is one that can be handled by scr_string()
#if defined(__i386__) || defined(__amd64__)
	__attribute__((hot,const,regparm(1)))
#else
	__attribute__((hot,const))
#endif//x86
static inline bool is_string_char(register int_fast16_t c)
{
	c &= 0177;
	return c >= ' ' || c == '\n' || c == '\r' || c == '\t';
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void handle_string_char(int_fast16_t c, Token * restrict tk)
{
	uint_fast16_t i = 0;
	tk->nlcount = 0;
	do {
		tk->string[i++] = c;
		c = get_com_char(1);
		if (c == '\n')
			++tk->nlcount;
	} while (is_string_char(c) && i < TKS_MAX);
	tk->length = i;
	tk->string[i] = 0;
	tk->type = TK_STRING;
	if (c != GCC_NULL)
		  push_com_char(c);
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void start_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	if (c >= '<' && c <= '?') {
		tk->private = c;
		c = get_com_char(0);
	}

	//  read any numerical arguments
	uint_fast16_t i = 0;
	do {
		uint_fast16_t n = 0;
		while (c >= '0' && c <= '9') {
			n = n * 10 + c - '0';
			c = get_com_char(0);
		}
		if (i < TK_MAX_ARGS)
			  tk->arg[i++] = n;
		if (c == CC_ESC)
			  push_com_char(c);
		if (c < ' ')
			  return;
		if (c < '@')
			  c = get_com_char(0);
	} while (c < '@' && c >= ' ');
	if (c == CC_ESC)
		  push_com_char(c);
	tk->nargs = i;
	tk->type = c;
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void end_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	uint_fast16_t n = 0;
	while (c >= '0' && c <= '9') {
		n = n * 10 + c - '0';
		c = get_com_char(0);
	}
	tk->arg[0] = n;
	tk->nargs = 1;
	c = get_com_char(0);
	register uint_fast16_t i = 0;
	while ((c & 0177) >= ' ' && i < TKS_MAX) {
		if (c >= ' ')
			  tk->string[i++] = c;
		c = get_com_char(0);
	}
	tk->length = i;
	tk->string[i] = 0;
	tk->type = TK_TXTPAR;
}

static void check_st(Token * t)
{
	int_fast16_t c = get_com_char(0);
	if (c != TK_DECST)
		t->type = TK_NULL;
}

static void start_dcs(Token * t)
{
	int_fast16_t c = get_com_char(0);
	switch (c) {
	case '0':
	case '1':
		LOG("FIXME: User defined keys are unimplemented.");
		return;
	case '$':
		c = get_com_char(0);
		if (c != 'q')
			return;
		// DECRQSS:  Request status string
		c = get_com_char(0); // next char
		switch (c) {
		case '"':
			c = get_com_char(0); // next
			switch (c) {
			case 'q':
				t->type = TK_QUERY_DECSCA;
				check_st(t);
				break;
			case 'p':
				t->type = TK_QUERY_DECSCL;
				check_st(t);
				break;
			}
			break;
		case 'r':
			t->type = TK_QUERY_DECSTBM;
			check_st(t);
			break;
		case 's':
			t->type = TK_QUERY_DECSLRM;
			check_st(t);
			break;
		case 'm':
			t->type = TK_QUERY_SGR;
			check_st(t);
			break;
		case ' ':
			c = get_com_char(0);
			if (c != 'q')
				return;
			t->type = TK_QUERY_DECSCUSR;
			check_st(t);
			break;
		}
		break;
	case '+':
		c = get_com_char(0);
		switch (c) {
		case 'p':
		case 'q':
			LOG("FIXME: termcap support unimplemented");
		}
		break;
	default:
		LOG("Unhandled device control string");
	}
}

#if defined(__i386__) || defined(__amd64__)
	__attribute__((nonnull,regparm(1)))
#else
	__attribute__((nonnull))
#endif//x86
static void handle_esc(int_fast16_t c, Token * restrict tk)
{
	c = get_com_char(0);
	switch(c) {
	case '[': // CSI
		start_esc(c, tk);
		break;
	case ']': // OSC
		end_esc(c, tk);
		break;
	case ' ':
		c = get_com_char(0);
		switch (c) {
		case 'F':
			tk->type = TK_S7C1T;
			break;
		case 'G':
			tk->type = TK_S8C1T;
			break;
		case 'L':
			tk->type = TK_ANSI1;
			break;
		case 'M':
			tk->type = TK_ANSI2;
			break;
		case 'N':
			tk->type = TK_ANSI3;
			break;
		}
		break;
	case '#':
		c = get_com_char(0);
		switch(c) {
		case '3':
			tk->type = TK_DECDHLT;
			break;
		case '4':
			tk->type = TK_DECDHLB;
			break;
		case '5':
			tk->type = TK_DECSWL;
			break;
		case '6':
			tk->type = TK_DECDWL;
			break;
		case '8':
			tk->type = TK_DECALN;
			break;
		}
		break;
	case '(': // G0 charset
	case ')': // G1 charset
		tk->type = c;
		c = get_com_char(0);
		tk->arg[0] = c;
		tk->nargs = 1;
		break;
	case '%': // UTF charset switch
		c = get_com_char(0);
		switch (c) {
		case '@':
			tk->type = TK_CS_DEF;
			break;
		case 'G':
			tk->type = TK_CS_UTF8;
			break;
		}
		break;
	case '7': // DECSC: save cursor
	case '8': // DECRC: restore cursor
	case '=': // DECPAM: keypad to application mode
	case '>': // DECPNM: keypad to numeric mode
	case '^': // DECPM: Privacy message (ended by ESC \)
	case '\\': // ST
		tk->type = c;
		tk->nargs = 0;
		break;
	case '<': // Exit vt52 mode
		jbxvt.mode.decanm = true;
		break;
	case 'A': // vt52 cursor up
	case 'B': // vt52 cursor down
	case 'C': // vt52 cursor left
		tk->type = c;
		break;
	case 'D': // vt52 cursor right
		tk->type = jbxvt.mode.decanm ? TK_IND : TK_CUF;
		break;
	case 'c': // Reset to Initial State
		tk->type = TK_RIS;
		break;
	case 'e': // enable cursor (vt52 GEMDOS)
		jbxvt.mode.dectcem = true;
		break;
	case 'f': // disable cursor (vt52 GEMDOS)
		jbxvt.mode.dectcem = false;
		break;
	case 'E' :
		tk->type = TK_NEL;
		break;
	case 'F': // Enter VT52 graphics mode
		tk->type = TK_ENTGM52;
		break;
	case 'G': // Leave VT52 graphics mode
		tk->type = TK_EXTGM52;
		break;
	case 'H':
		tk->type = jbxvt.mode.decanm ? TK_HTS : TK_HOME;
		break;
	case 'l':
		tk->type = jbxvt.mode.decanm ? TK_MEMLOCK : TK_EL;
		tk->arg[0] = 2;
		tk->nargs = 1;
		break;
	case 'I':
		tk->type = TK_CUU;
		break;
	case 'J': // vt52 erase to end of line
		tk->type = TK_EL;
		tk->arg[0] = 0;
		tk->nargs = 1;
		break;
	case 'j': // save cursor (vt52)
		tk->type = TK_DECSC;
		break;
	case 'K': // vt42 erase to end of screen
		tk->type = TK_ED;
		break;
	case 'k': // restore cursor (vt52)
		tk->type = TK_DECRC;
		break;
	case 'M' :
		tk->type = TK_RI;
		break;
	case 'N' :
		tk->type = TK_SS2;
		break;
	case 'O' :
		tk->type = TK_SS3;
		break;
	case 'P':
		start_dcs(tk);
		break;
	case 'V':
		tk->type = TK_SPA;
		break;
	case 'v': // wrap on
		jbxvt.mode.decawm = true;
		break;
	case 'W':
		tk->type = TK_EPA;
		break;
	case 'w': // wrap off
		jbxvt.mode.decawm = false;
		break;
	case 'X':
		tk->type = TK_SOS;
		break;
	case 'Y':
		tk->type = TK_CUP;
		// -32 to decode, + 1 to be vt100 compatible
		tk->arg[1] = get_com_char(0) - 31;
		tk->arg[0] = get_com_char(0) - 31;
		tk->nargs = 2;
	case 'Z':
		if (jbxvt.mode.decanm) // vt100+ mode
			tk->type = TK_DECID;
		else // I am a VT52
			cprintf("\033/Z");
		break;
	}
}

static void default_token(Token * restrict tk, const int_fast16_t c)
{
	if (c > 0x84 && c < 0x9f) {
		// Pass other controls directly to main switch
		LOG("8-bit sequence passed directly");
		tk->type = c;
	} else if (is_string_char(c))
		handle_string_char(c, tk);
	else {
		tk->type = TK_CHAR;
		tk->tk_char = c;
	}
}

//  Return an input token
void get_token(Token * restrict tk)
{
	memset(tk, 0, sizeof(Token));
	// set token per event:
	if(handle_xevents(tk))
		  return;
	const int_fast16_t c = get_com_char(GET_XEVENTS);
	switch (c) {
	case GCC_NULL:
		tk->type = TK_NULL;
		break;
	case EOF:
		tk->type = TK_EOF;
		break;
	case CC_ESC:
		handle_esc(c, tk);
		break;
	case CC_CSI: // 8-bit CSI
		// Catch this here, since 7-bit CSI is parsed above.
		LOG("CC_CSI");
		start_esc(c, tk);
		break;
	case CC_DCS: // 8-bit DCS
		start_dcs(tk);
		break;
	default:
		default_token(tk, c);
	}
}

