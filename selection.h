/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SELECTION_H
#define JBXVT_SELECTION_H
#include <stdbool.h>
#include <stdint.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
enum JBXVTSelectionUnit;
struct JBDim;
/*  Determine if the current selection overlaps row1-row2.
    If it does, then remove it from the screen.  */
void jbxvt_check_selection(xcb_connection_t * xc,
	const int16_t row1, const int16_t row2);
// clear the current selection:
void jbxvt_clear_selection(void);
//  Return the atom corresponding to "CLIPBOARD"
xcb_atom_t jbxvt_get_clipboard(xcb_connection_t * xc);
// Return selection end points: first, second, and anchor
struct JBDim * jbxvt_get_selection_end_points(void);
enum JBXVTSelectionUnit jbxvt_get_selection_unit(void);
bool jbxvt_is_selected(void);
//  Make the selection currently delimited by the selection end markers.
void jbxvt_make_selection(xcb_connection_t * xc);
//  respond to a request for our current selection.
void jbxvt_send_selection(xcb_connection_t * xc,
	const xcb_time_t time, const uint32_t requestor,
	const uint32_t target, const uint32_t property);
// start selection using specified unit:
void jbxvt_start_selection(xcb_connection_t * xc,
	const struct JBDim p, enum JBXVTSelectionUnit unit);
#endif//!JBXVT_SELECTION_H
