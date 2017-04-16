/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_REQUEST_H
#define JBXVT_REQUEST_H
struct JBXVTToken;
void jbxvt_handle_JBXVT_TOKEN_REQTPARAM(void * xc __attribute__((unused)),
	struct JBXVTToken * token);
#endif//!JBXVT_REQUEST_H
