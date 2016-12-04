// Copyright 2016, Jeffrey E. Bedard
#include "rstyle.h"
static rstyle_t rstyle_value;
void jbxvt_zero_rstyle(void)
{
	rstyle_value = 0;
}
void jbxvt_add_rstyle(const rstyle_t val)
{
	rstyle_value |= val;
}
void jbxvt_del_rstyle(const rstyle_t val)
{
	rstyle_value &= ~val;
}
rstyle_t jbxvt_get_rstyle(void)
{
	return rstyle_value;
}
