// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_MODE_H
#define JBXVT_MODE_H
struct JBXVTToken;
// Return the current Control Sequence Initializer
const char * jbxvt_get_csi(void);
void jbxvt_handle_JBXVT_TOKEN_ELR(void * xc, struct JBXVTToken * token);
// Get pointer to statically allocated mode data structure.
struct JBXVTPrivateModes * jbxvt_get_modes(void);
// Restore private modes.
void jbxvt_restore_modes(void);
// Save private modes.
void jbxvt_save_modes(void);
#endif//!JBXVT_MODE_H
