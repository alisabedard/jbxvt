// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_JBXVTPRIVATEMODES_H
#define JBXVT_JBXVTPRIVATEMODES_H
#include <stdbool.h>
#include <stdint.h>
enum JBXVTCharacterSet{
	CHARSET_GB, CHARSET_ASCII, CHARSET_SG0, CHARSET_SG1, CHARSET_SG2,
	CHARSET_SG3, JBXVT_CHARSET_SHIFT_REGISTER = 5
};
struct JBXVTPrivateModes {
	/* Character set registers: G[0-3], and one more
	   for single-shift register:  */
	uint8_t charset[6];
	/* charsel is the index into charset[], indicating the current
	   character set register in use.  */
	uint8_t charsel:2;
	bool att610:1;		// stop blinking cursor
	bool decanm:1;		// DECANM -- ANSI/VT52
	bool decawm:1;		// DECAWM auto-wrap flag
	bool deccolm:1;		// 132 column mode
	bool decom:1;		// origin mode flag
	bool decpff:1;		// DECPFF: print form feed
	bool decsclm:1;		// DECSCLM: slow scroll mode
	bool decscnm:1;		// DECSCNM: reverse-video mode
	bool dectcem:1;		// DECTCEM -- show cursor
	bool gm52:1;		// VT52 graphics mode
	bool insert:1;		// insert mode flag
	bool lnm:1;		// new line / line feed mode
	bool mouse_x10:1;	// ptr coord on button press
	bool mouse_vt200:1;	// ptr press+release
	bool mouse_vt200hl:1;	// highlight tracking
	bool mouse_btn_evt:1;	// button event tracking
	bool mouse_any_evt:1;	// all motion tracking
	bool mouse_focus_evt:1; // focus tracking
	bool mouse_ext:1;	// UTF-8 coords
	bool mouse_sgr:1;	// sgr scheme
	bool mouse_urxvt:1;	// decimal integer coords
	bool mouse_alt_scroll:1;// send cursor up/down instead
	bool ptr_xy:1;		// send x y on button press/release
	bool ptr_cell:1;	// cell motion mouse tracking
	bool s8c1t:1;		// 7 or 8 bit controls
	bool bpaste:1;		// bracketed paste mode
	bool elr:1;		// locator report
	bool elr_once:1;	// locator report once
	bool elr_pixels:1;	// locator report in pixel format
	bool ss2:1;		// use G2 charset on next character only
	bool ss3:1;		// use G3 charset on next character only
	const int8_t __pad:7;
};
#endif//!JBXVT_JBXVTPRIVATEMODES_H
