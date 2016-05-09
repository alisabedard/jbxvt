#include "scr_request_selection.h"


#include "jbxvt.h"
#include "screen.h"
#include "selection.h"

/*  Predicate function used when waiting for selection events.  If arg is
 *  NULL then we return true for Selection Notify events.  If arg is not
 *  NULL then it is assumed to point to a time and we also return true for
 *  keyboard events that arrive after more than SEL_KEY_DEL after the time.
 */
static Bool sel_pred(Display * restrict dpy __attribute__((unused)),
	XEvent * restrict ev, char * restrict arg)
{
	if (ev->type == SelectionNotify)
		return (True);
	if (arg != NULL && ev->type == KeyPress &&
				(ev->xkey.time - *(Time *)arg) > SEL_KEY_DEL)
		return (True);
	return (False);
}



/*  Wait for the selection to arrive and move it to the head of the
 *  queue.  We wait until we either get the selection or we get
 *  keyboard input that was generated more than SEL_KEY_DEL after time.
 */
static void wait_for_selection(Time time)
{
	XEvent event;

	XPeekIfEvent(jbxvt.X.dpy,&event,sel_pred,(char *)&time);
	if (event.type == SelectionNotify) {
		XIfEvent(jbxvt.X.dpy,&event,sel_pred,NULL);
		XPutBackEvent(jbxvt.X.dpy,&event);
	}
}

//  Send the selection to the command after converting LF to CR.
static void send_selection(unsigned char * str, const int count)
{
	int i;

	for (i = 0; i < count; i++)
		if (str[i] == '\n')
			str[i] = '\r';
	send_string(str,count);
}

//  Request the current primary selection
void scr_request_selection(int time, int x, int y)
{
	Atom sel_property;

	/*  First check that the release is within the window.
	 */
	if (x < 0 || x >= pwidth || y < 0 || y >= pheight)
		return;

	if (jbxvt.sel.text != NULL) {

		/* The selection is internal
		 */
		send_selection(jbxvt.sel.text,jbxvt.sel.length);
		return;
	}

	if (XGetSelectionOwner(jbxvt.X.dpy,XA_PRIMARY) == None) {

		/*  No primary selection so use the cut buffer.
		 */
		Atom actual_type;
		int actual_format;
		unsigned long nitems, bytes_after, nread;
		unsigned char *data;

		nread = 0;
		do {
			if (XGetWindowProperty(jbxvt.X.dpy,
				DefaultRootWindow(jbxvt.X.dpy),
				XA_CUT_BUFFER0, nread / 4,PROP_SIZE,
				False, XA_STRING,&actual_type,
				&actual_format, &nitems,&bytes_after,
				&data) != Success)
				return;
			if (nitems == 0 || data == NULL)
				return;
			send_selection(data,nitems);
			nread += nitems;
			XFree(data);
		} while (bytes_after > 0);
		return;
	}

	sel_property = XInternAtom(jbxvt.X.dpy,"VT_SELECTION",False);
	XConvertSelection(jbxvt.X.dpy,XA_PRIMARY,XA_STRING,sel_property,jbxvt.X.win.vt,time);
	wait_for_selection(time);
}

//  Respond to a notification that a primary selection has been sent
void scr_paste_primary(const int time __attribute__((unused)),
	const Window window, const Atom property)
{
	Atom actual_type;
	int actual_format;
	unsigned long nitems, bytes_after, nread;
	unsigned char *data;

	if (property == None)
		return;
	nread = 0;
	do {
		if (XGetWindowProperty(jbxvt.X.dpy,window,property,nread / 4,
			PROP_SIZE,True, AnyPropertyType,&actual_type,
			&actual_format, &nitems,&bytes_after,&data) != Success)
			return;
		if (actual_type != XA_STRING)
			return;
		if (nitems == 0 || data == NULL)
			return;
		send_selection(data,nitems);
		nread += nitems;
		XFree(data);
	} while (bytes_after > 0);
}


