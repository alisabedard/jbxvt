// Copyright 2017-2020, Jeffrey E. Bedard
#ifndef JBXVT_RSTYLE_H
#define JBXVT_RSTYLE_H
#include <stdint.h>
/* Use the stdint type here instead of a typedef to enum JBXVTRenderStyle in
   order to ensure 32 bits across platforms.  */
typedef uint32_t rstyle_t;
// val indicates bit on which to operate
void jbxvt_add_rstyle(const uint8_t val);
// val indicates bit on which to operate
void jbxvt_del_rstyle(const uint8_t val);
void jbxvt_set_rstyle(const rstyle_t val);
rstyle_t jbxvt_get_rstyle(void);
void jbxvt_zero_rstyle(void);
#endif//!JBXVT_RSTYLE_H
