// Copyright 2017-2020, Jeffrey E. Bedard
#include "rstyle.h"
static rstyle_t rstyle_value; // bit mask store
void jbxvt_zero_rstyle(void)
{
    rstyle_value ^= rstyle_value;
}
void jbxvt_add_rstyle(const uint8_t val)
{
    if (val != 128)
        rstyle_value |= (rstyle_t)(1<<val);
}
void jbxvt_set_rstyle(const rstyle_t val)
{
    rstyle_value=val;
}
void jbxvt_del_rstyle(const uint8_t val)
{
    rstyle_value&=((rstyle_t)~(1<<val));
}
rstyle_t jbxvt_get_rstyle(void)
{
    return rstyle_value;
}
