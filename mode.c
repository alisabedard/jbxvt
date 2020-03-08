// Copyright 2017, Jeffrey E. Bedard
#include "mode.h"
#include <string.h>
#include "JBXVTPrivateModes.h"
#include "JBXVTToken.h"
#include "libjb/log.h"
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
void jbxvt_handle_JBXVT_TOKEN_ELR(void * xc __attribute__((unused)),
    struct JBXVTToken * token)
{
    int16_t * restrict t = token->arg;
    struct JBXVTPrivateModes * restrict m = jbxvt_get_modes();
    switch (t[0]) {
    case 2:
        LOG("ELR Once");
        m->elr_once = true;
        // FALLTHROUGH
    case 1:
        LOG("ELR Mode");
        m->elr = true;
        break;
    default:
        LOG("ELR Disabled");
        m->elr = m->elr_once = false;
    }
    m->elr_pixels = t[1] == 1;
}
// Return the current Control Sequence Initializer
const char * jbxvt_get_csi(void)
{
    return jbxvt_get_modes()->s8c1t ? "\233" : "\033[";
}
