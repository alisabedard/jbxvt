#ifndef SELST_H
#define SELST_H

/*  selection endpoint types.
 */
enum seltype {
	SCREENSEL,
	SAVEDSEL,
	NOSEL
};

/*  structure describing a selection endpoint.
 */
struct selst {
	enum seltype se_type;
	int se_index;	/* index into the sline or screen array */
	int se_col;	/* column of the character */
};

#endif//!SELST_H
