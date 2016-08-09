/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/

#ifndef TOKEN_TYPE_H
#define TOKEN_TYPE_H

// Reference man console_codes(4) and ctlseqs(ms)

//  Token types
typedef enum {
	TK_NULL = 0, // null token to be ignored
	// Tokens > 1000 are artificial.
	// Done this way to prevent clash with CSI sequences.
	TK_STRING =  2001, // string of printable characters
	TK_CHAR =  2002, // single character
	TK_EOF = 2003, // read end of file
	TK_ENTRY = 2004, // cursor crossed window boundery
	TK_EXPOSE = 2005, // window has been exposed
	TK_RESIZE = 2006, // main window has been resized
	TK_SBSWITCH = 2007, // switch scrollbar in or out
	TK_SBGOTO = 2008, // scrollbar goto
	TK_SBUP = 2009, // scrollbar move up

	TK_SBDOWN = 2010, // scrollbar move down
	TK_SELSTART = 2011, // start the selection
	TK_SELEXTND = 2012, // extend the selection
	TK_SELDRAG = 2013, // drag the selection
	TK_SELINSRT = 2014, // insert the selection
	TK_SELWORD = 2015, // select a word
	TK_SELLINE = 2016, // select a line
	TK_SELECT = 2017, // confirm the selection
	TK_SELCLEAR = 2018, // selection clear request
	TK_SELNOTIFY = 2019, // selection notify request

	TK_SELREQUEST = 2020, // selection request
	TK_TXTPAR = 2021, // seq with text parameter
	TK_FOCUS = 2022, // keyboard focus event

	// DEC VT100 control sequence token types
	TK_RIS = 1000, // reset to initial state
	TK_ENTGM52 = 1001, // enter vt52 graphics mode (ESC F)
	TK_EXTGM52 = 1002, // exit vt52 graphics mode (ESC G);
	TK_ANSI1 = 1003, // ANSI conformance level 1
	TK_ANSI2 = 1004, // ANSI conformance level 2
	TK_ANSI3 = 1005, // ANSI conformance level 3
	TK_S7C1T = 1006, // 7-bit controls
	TK_S8C1T = 1007, // 8-bit controls

	// ESC # <num + 3000>
	TK_DECDHLT = 3003, // double height line, top half
	TK_DECDHLB = 3004, // double height line, bottom half
	TK_DECSWL = 3005, // single width line
	TK_DECDWL = 3006, // double width line
	TK_DECALN = 3008, // screen alignment test (e-fill)

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
	TK_DECLL = 'q', /* Load leds, set cursor style,
			   or select character protection attribute.  */
	TK_DECSTBM = 'r', // set top and bottom margins
        // ^-- also restore DEC mode values
	TK_DECSAVEPM = 's', // Save DEC (re)set mode values
	TK_DECREQTPARAM = 'x', // REQuest Terminal PARAMeters
	TK_DECELR = 'z', // Enable Locator Reporting
	TK_SCS0 = '(', // set character set G0
	TK_SCS1 = ')', // set character set G1
	TK_DECSC = '7', // save cursor position
	TK_DECRC = '8', // restore cursor position
	TK_DECPAM = '=', // keypad to applications mode
	TK_DECPNM = '>', // keypad to numeric mode

	TK_DECPM = '^', // Privacy message (ended by ESC \)
	TK_DECST = '\\', // String Terminator
	TK_ESC = 033,
	TK_IND = 0x84,
	TK_NEL = 0x85,
	TK_HTS = 0x88,
	TK_RI = 0x8d,
	TK_SS2 = 0x8e,
	TK_SS3 = 0x8f,
	TK_DCS = 0x90,
	TK_SPA = 0x96,
	TK_EPA = 0x97,
	TK_SOS = 0x98,
	TK_DECID = 0x9a,
	TK_CSI = 0x9b,
	TK_ST = 0x9c,
	TK_OSC = 0x9d,
	TK_PM = 0x9e,
	TK_APC = 0x9f
} TokenType;

#endif//!TOKEN_H
