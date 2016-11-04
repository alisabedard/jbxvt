// Copyright 2016, Jeffrey E. Bedard
#include "rstyle.h"
static uint32_t rstyle_value;
void jbxvt_zero_rstyle(void)
{
	rstyle_value = 0;
}
void jbxvt_add_rstyle(const uint32_t val)
{
	rstyle_value |= val;
}
void jbxvt_del_rstyle(const uint32_t val)
{
	rstyle_value &= ~val;
}
uint32_t jbxvt_get_rstyle(void)
{
	return rstyle_value;
}
