/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

//  Token types
typedef enum {
	TK_NULL = 0, // null token to be ignored
	TK_STRING =  1, // string of printable characters
	TK_CHAR =  2, // single character
	TK_EOF = 3, // read end of file
	TK_ENTRY = 4, // cursor crossed window boundery
	TK_EXPOSE = 5, // window has been exposed
	TK_RESIZE = 6, // main window has been resized
	TK_SBSWITCH = 7, // switch scrollbar in or out
	TK_SBGOTO = 8, // scrollbar goto
	TK_SBUP = 9, // scrollbar move up

	TK_SBDOWN = 10, // scrollbar move down
	TK_SELSTART = 11, // start the selection
	TK_SELEXTND = 12, // extend the selection
	TK_SELDRAG = 13, // drag the selection
	TK_SELINSRT = 14, // insert the selection
	TK_SELWORD = 15, // select a word
	TK_SELLINE = 16, // select a line
	TK_SELECT = 17, // confirm the selection
	TK_SELCLEAR = 18, // selection clear request
	TK_SELNOTIFY = 19, // selection notify request

	TK_SELREQUEST = 20, // selection request
	TK_TXTPAR = 21, // seq with text parameter
	TK_FOCUS = 22, // keyboard focus event

	// DEC VT100 control sequence token types
	// Tokens > 1000 are artificial.
	// Done this way to prevent clash with CSI sequences.
	TK_RIS = 1000, // reset to initial state
	TK_ENTGM52 = 1001, // enter vt52 graphics mode (ESC F)
	TK_EXTGM52 = 1002, // exit vt52 graphics mode (ESC G);

	TK_CUU = 'A', // Cursor up
	TK_CUD = 'B', // cursor down
	TK_CUF = 'C', // cursor back
	TK_CUB = 'D', // cursor back
	TK_CNL = 'E', // CNL: cursor next line, first column
	TK_CPL = 'F', // CNL: cursor prev line, first column
	TK_CHA = 'G', // cursor CHaracter Absolute [column]
	TK_CUP = 'H', // position cursor

	TK_ED = 'J', // erase to start or end of screen
	TK_EL = 'K', // erase to start or end of line
	TK_IL = 'L', // IL: insert lines
	TK_DL = 'M', // delete lines
	TK_DCH = 'P', // Delete characters
	TK_ECH = 'X', // ECH: Erase CHaracters
	TK_ICH = '@', // ICH: insert characters
	TK_HPR = 'a', // HPR: horizontal position relative
	TK_DA = 'c', // device attributes request
	TK_VPA = 'd', // vertical position absolute
	TK_VPR = 'e', // vertical position relative
	TK_HPA = '`', // horizontal position absolute
	TK_HVP = 'f', // horizontal and vertical position
	TK_TBC = 'g', // tab clear
	TK_SU = 'S', // Scroll Up # lines
	TK_SD = 'T', // Scroll Down # lines
	TK_SET = 'h', // set mode
	TK_RESET = 'l', // reset mode
	TK_SGR = 'm', // set graphics rendition
	TK_DSR = 'n', // report status or position
	TK_DECSTBM = 'r', // set top and bottom margins
        // ^-- also restore DEC mode values
	TK_DECSAVEPM = 's', // Save DEC (re)set mode values

	TK_DECSWH = '#', // set character width or height
	TK_DECALN = '8', // DECALN test, fill with E
	TK_SCS0 = '(', // set character set G0
	TK_SCS1 = ')', // set character set G1
	TK_DECSC = '7', // save cursor position
	TK_DECRC = '8', // restore cursor position
	TK_DECPAM = '=', // keypad to applications mode
	TK_DECPNM = '>', // keypad to numeric mode

	TK_IND = 0x100, // index downward
	TK_NEL = 0x101, // beginning of next line
	TK_HTS = 0x102, // horizontal tab set
	TK_RI = 0x103, // reverse index
	TK_SS2 = 0x104, // single shift 2
	TK_SS3 = 0x105, // single shift 3
	TK_DECID = 0x106, // request terminal ID
} TokenType;

#endif//!TOKEN_H
