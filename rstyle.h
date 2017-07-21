// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_RSTYLE_H
#define JBXVT_RSTYLE_H
#include <stdint.h>
typedef uint32_t rstyle_t;
void jbxvt_add_rstyle(const rstyle_t val);
void jbxvt_del_rstyle(const rstyle_t val);
rstyle_t jbxvt_get_rstyle(void);
void jbxvt_zero_rstyle(void);
#endif//!JBXVT_RSTYLE_H
