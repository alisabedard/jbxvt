/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_REQUEST_H
#define JBXVT_REQUEST_H
struct JBXVTToken;
// Load LEDs, SCUSR and SCA
void jbxvt_handle_JBXVT_TOKEN_LL(void * xc, struct JBXVTToken * token);
void jbxvt_handle_JBXVT_TOKEN_REQTPARAM(void * xc, struct JBXVTToken * token);
void jbxvt_handle_JBXVT_TOKEN_RQM(void * xc, struct JBXVTToken * token);
#endif//!JBXVT_REQUEST_H
