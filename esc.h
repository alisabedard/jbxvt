/*  Copyright 2016, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#ifndef JBXVT_ESC_H
#define JBXVT_ESC_H
#include "Token.h"
void jbxvt_csi(int_fast16_t c, struct Token * restrict tk);
void jbxvt_end_cs(int_fast16_t c, struct Token * restrict tk);
void jbxvt_esc(int_fast16_t c, struct Token * restrict tk);
#endif//!JBXVT_ESC_H
