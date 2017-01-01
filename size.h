// Copyright 2017, Jeffrey E. Bedard
#ifndef JBXVT_SIZE_H
#define JBXVT_SIZE_H
struct JBDim;
// Convert char size/position to pixel size/position
struct JBDim jbxvt_chars_to_pixels(struct JBDim c)
	__attribute__((pure));
// Return size module's stored size converted to characters
struct JBDim jbxvt_get_char_size(void);
// Return size module's stored pixel size
struct JBDim jbxvt_get_pixel_size(void);
// Convert pixel size/position to char size/position
struct JBDim jbxvt_pixels_to_chars(struct JBDim p)
	__attribute__((pure));
void jbxvt_set_pixel_size(const struct JBDim size);
#endif//!JBXVT_SIZE_H
