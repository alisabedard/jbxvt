// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_CASES_H
#define JBXVT_CASES_H
// Note:  Cast to (void) to avoid warnings about unused arguments
#define PARMS xcb_connection_t * xc, struct JBXVTToken * token
#define NOPARM_XC() (void)xc;
#define NOPARM_TOKEN() (void)token;
#define NOPARM() NOPARM_XC(); NOPARM_TOKEN();
#define HANDLE(name) static void jbxvt_handle_JBXVT_TOKEN_##name(PARMS)
// Used to specify external case actions:
#ifdef __GNUC__
#define EXTERN_ALIAS(nalias, source) __attribute__((weakref, alias(#source)))\
    static void jbxvt_handle_JBXVT_TOKEN_##nalias(PARMS);
#else//!__GNUC__
#define EXTERN_ALIAS(alias, source) static void \
    (* jbxvt_handle_JBXVT_TOKEN_##alias)(PARMS) = source
#endif//__GNUC__
// Used to specify duplicate case actions:
#define ALIAS(alias, source) EXTERN_ALIAS(alias, \
    jbxvt_handle_JBXVT_TOKEN_##source)
#endif//!JBXVT_CASES_H
