// Copyright 2016, Jeffrey E. Bedard
#ifndef JBXVT_RSTYLE_H
#define JBXVT_RSTYLE_H
#include <stdint.h>
void jbxvt_add_rstyle(const uint32_t val);
void jbxvt_del_rstyle(const uint32_t val);
uint32_t jbxvt_get_rstyle(void);
void jbxvt_zero_rstyle(void);
#endif//!JBXVT_RSTYLE_H
