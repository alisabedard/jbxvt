/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_SAVE_SELECTION_H
#define JBXVT_SAVE_SELECTION_H
struct JBXVTSelectionData;
/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void jbxvt_save_selection(struct JBXVTSelectionData * sel);
#endif//!JBXVT_SAVE_SELECTION_H
