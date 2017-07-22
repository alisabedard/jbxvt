// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_RSTYLE_H
#define JBXVT_RSTYLE_H
#include <stdint.h>
/* Use the stdint type here instead of a typedef to enum JBXVTRenderStyle in
 * order to ensure 32 bits across platforms.  */
typedef uint32_t rstyle_t;
void jbxvt_add_rstyle(const rstyle_t val);
void jbxvt_del_rstyle(const rstyle_t val);
rstyle_t jbxvt_get_rstyle(void);
void jbxvt_zero_rstyle(void);
#endif//!JBXVT_RSTYLE_H
