// Copyright 2017, Jeffrey E. Bedard
#include "mode.h"
#include <stdbool.h>
#include <string.h>
#include "JBXVTPrivateModes.h"
static struct JBXVTPrivateModes jbxvt_saved_mode;
// Set default values for private modes
struct JBXVTPrivateModes * jbxvt_get_modes(void)
{
	// Set defaults:
	static struct JBXVTPrivateModes m = {.decanm = true,
		.decawm = false, .dectcem = true,
		.charset = {CHARSET_ASCII, CHARSET_ASCII}};
	return &m;
}
// Restore private modes.
void jbxvt_restore_modes(void)
{
	memcpy(jbxvt_get_modes(), &jbxvt_saved_mode,
		sizeof(struct JBXVTPrivateModes));
}
// Save private modes.
void jbxvt_save_modes(void)
{
	memcpy(&jbxvt_saved_mode, jbxvt_get_modes(),
		sizeof(struct JBXVTPrivateModes));
}
