/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_TOKENTYPE_H
#define JBXVT_TOKENTYPE_H
// Reference man console_codes(4) and ctlseqs(ms)
//  struct Token types
enum JBXVTTokenType {
	JBXVT_TOKEN_NULL = 0, // null token to be ignored
	// Tokens > 1000 are artificial.
	// Done this way to prevent clash with CSI sequences.
	//  VT100 control sequence token types
	JBXVT_TOKEN_RIS = 1000, // reset to initial state
	JBXVT_TOKEN_ENTGM52 = 1001, // enter vt52 graphics mode (ESC F)
	JBXVT_TOKEN_EXTGM52 = 1002, // exit vt52 graphics mode (ESC G);
	JBXVT_TOKEN_ANSI1 = 1003, // ANSI conformance level 1
	JBXVT_TOKEN_ANSI2 = 1004, // ANSI conformance level 2
	JBXVT_TOKEN_ANSI3 = 1005, // ANSI conformance level 3
	JBXVT_TOKEN_S7C1T = 1006, // 7-bit controls
	JBXVT_TOKEN_S8C1T = 1007, // 8-bit controls
	JBXVT_TOKEN_HOME = 1008, // move cursor to home position
	// Internal tokens
	JBXVT_TOKEN_STRING =  2001, // string of printable characters
	JBXVT_TOKEN_CHAR =  2002, // single character
	JBXVT_TOKEN_EOF = 2003, // read end of file
	JBXVT_TOKEN_ENTRY = 2004, // cursor crossed window boundery
	JBXVT_TOKEN_EXPOSE = 2005, // window has been exposed
	JBXVT_TOKEN_RESIZE = 2006, // main window has been resized
	JBXVT_TOKEN_SBSWITCH = 2007, // switch scrollbar in or out
	JBXVT_TOKEN_SBGOTO = 2008, // scrollbar goto
	JBXVT_TOKEN_SBUP = 2009, // scrollbar move up
	JBXVT_TOKEN_SBDOWN = 2010, // scrollbar move down
	JBXVT_TOKEN_SELSTART = 2011, // start the selection
	JBXVT_TOKEN_SELEXTND = 2012, // extend the selection
	JBXVT_TOKEN_SELDRAG = 2013, // drag the selection
	JBXVT_TOKEN_SELINSRT = 2014, // insert the selection
	JBXVT_TOKEN_SELWORD = 2015, // select a word
	JBXVT_TOKEN_SELLINE = 2016, // select a line
	JBXVT_TOKEN_SELECT = 2017, // confirm the selection
	JBXVT_TOKEN_SELCLEAR = 2018, // selection clear request
	JBXVT_TOKEN_SELNOTIFY = 2019, // selection notify request
	JBXVT_TOKEN_SELREQUEST = 2020, // selection request
	JBXVT_TOKEN_TXTPAR = 2021, // seq with text parameter
	JBXVT_TOKEN_FOCUS = 2022, // keyboard focus event
	// ESC # <num + 3000>
	JBXVT_TOKEN_DHLT = 3003, // double height line, top half
	JBXVT_TOKEN_DHLB = 3004, // double height line, bottom half
	JBXVT_TOKEN_SWL = 3005, // single width line
	JBXVT_TOKEN_DWL = 3006, // double width line
	JBXVT_TOKEN_ALN = 3008, // screen alignment test (e-fill)
	// ESC % char
	JBXVT_TOKEN_CS_DEF = 3050, // Default character set
	JBXVT_TOKEN_CS_UTF8 = 3051, // UTF-8 character set
	// HP
	JBXVT_TOKEN_MEMLOCK = 3100, // HP term, lock memory above cursor
	JBXVT_TOKEN_MEMUNLOCK = 3101, // HP term, unlock memory
	// DSR tokens
	JBXVT_TOKEN_QUERY_SCA = 3500, // cursor attributes
	JBXVT_TOKEN_QUERY_SCL = 3501,
	JBXVT_TOKEN_QUERY_STBM = 3502, // scroll margins
	JBXVT_TOKEN_QUERY_SLRM = 3503, // soft scroll mode
	JBXVT_TOKEN_QUERY_SGR = 3504, // sgr style
	JBXVT_TOKEN_QUERY_SCUSR = 3505,
	// Cursor tokens
	JBXVT_TOKEN_CUU = 'A', // Cursor up
	JBXVT_TOKEN_CUD = 'B', // cursor down
	JBXVT_TOKEN_CUF = 'C', // cursor back
	JBXVT_TOKEN_CUB = 'D', // cursor back
	JBXVT_TOKEN_CNL = 'E', // CNL: cursor next line, first column
	JBXVT_TOKEN_CPL = 'F', // CNL: cursor prev line, first column
	JBXVT_TOKEN_CHA = 'G', // cursor CHaracter Absolute [column]
	JBXVT_TOKEN_CUP = 'H', // position cursor
	JBXVT_TOKEN_CHT = 0x49, // cursor horizontal tab
	JBXVT_TOKEN_ED = 'J', // erase to start or end of screen
	JBXVT_TOKEN_EL = 'K', // erase to start or end of line
	JBXVT_TOKEN_IL = 'L', // IL: insert lines
	JBXVT_TOKEN_DL = 'M', // delete lines
	JBXVT_TOKEN_DCH = 'P', // Delete characters
	JBXVT_TOKEN_ECH = 'X', // ECH: Erase CHaracters
	JBXVT_TOKEN_ICH = '@', // ICH: insert characters
	JBXVT_TOKEN_HPR = 'a', // HPR: horizontal position relative
	JBXVT_TOKEN_DA = 'c', // device attributes request
	JBXVT_TOKEN_VPA = 'd', // vertical position absolute
	JBXVT_TOKEN_VPR = 'e', // vertical position relative
	JBXVT_TOKEN_HPA = '`', // horizontal position absolute
	JBXVT_TOKEN_HVP = 'f', // horizontal and vertical position
	JBXVT_TOKEN_TBC = 'g', // tab clear
	JBXVT_TOKEN_SET = 'h', // set mode
	JBXVT_TOKEN_MC = 'i', // media copy
	JBXVT_TOKEN_SGR = 'm', // set graphics rendition
	JBXVT_TOKEN_RQM = 'p', // request DEC private mode
	JBXVT_TOKEN_SU = 'S', // Scroll Up # lines
	JBXVT_TOKEN_SD = 'T', // Scroll Down # lines
	JBXVT_TOKEN_RESET = 'l', // reset mode
	JBXVT_TOKEN_DSR = 'n', // report status or position
	JBXVT_TOKEN_LL = 'q', /* Load leds, set cursor style,
			   or select character protection attribute.  */
	JBXVT_TOKEN_STBM = 'r', // set top and bottom margins
        // ^-- also restore  mode values
	JBXVT_TOKEN_SAVEPM = 's', // Save  (re)set mode values
	JBXVT_TOKEN_REQTPARAM = 'x', // REQuest Terminal PARAMeters
	JBXVT_TOKEN_ELR = 'z', // Enable Locator Reporting
	JBXVT_TOKEN_SCS0 = '(', // set character set G0
	JBXVT_TOKEN_SCS1 = ')', // set character set G1
	JBXVT_TOKEN_SC = '7', // save cursor position
	JBXVT_TOKEN_RC = '8', // restore cursor position
	JBXVT_TOKEN_PAM = '=', // keypad to applications mode
	JBXVT_TOKEN_PNM = '>', // keypad to numeric mode
	JBXVT_TOKEN_RESTOREPM = '?', // restore private modes
	JBXVT_TOKEN_ESC = 033,
	JBXVT_TOKEN_IND = 0x84,
	JBXVT_TOKEN_NEL = 0x85,
	JBXVT_TOKEN_HTS = 0x88,
	JBXVT_TOKEN_RI = 0x8d,
	JBXVT_TOKEN_SS2 = 0x8e,
	JBXVT_TOKEN_SS3 = 0x8f,
	JBXVT_TOKEN_DCS = 0x90,
	JBXVT_TOKEN_SPA = 0x96,
	JBXVT_TOKEN_EPA = 0x97,
	JBXVT_TOKEN_SOS = 0x98,
	JBXVT_TOKEN_ID = 0x9a,
	JBXVT_TOKEN_CSI = 0x9b,
	JBXVT_TOKEN_ST = 0x9c,	// String Terminator
	JBXVT_TOKEN_OSC = 0x9d,
	JBXVT_TOKEN_PM = 0x9e,	// Privacy message (ended by ESC \ (ST))
	JBXVT_TOKEN_APC = 0x9f
};
#endif//!JBXVT_TOKENTYPE_H
