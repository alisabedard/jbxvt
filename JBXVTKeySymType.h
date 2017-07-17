// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_KEYSYM_TYPE_H
#define JBXVT_KEYSYM_TYPE_H
/*  Different values for ks_type which determine how the value is used to
 *  generate the string.  */
enum JBXVTKeySymType {
	NO_KS,             // No output
	CHAR_KS,           // as printf("%c",ks_value)
	XTERM_KS,          // as printf("\033[%d",ks_value)
	APPKEY_KS,         // as printf("\033O%c",ks_value)
	NONAPP_KS          // as printf("\033[%c",ks_value)
};
#endif//!JBXVT_KEYSYM_TYPE_H
