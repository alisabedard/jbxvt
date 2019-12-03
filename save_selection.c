/*  Copyright 2017, Jeffrey E. Bedard
    Copyright 1992, 1997 John Bovey, University of Kent at Canterbury.*/
#include "save_selection.h"
#include <stdlib.h>
#include <string.h>
#include "JBXVTLine.h"
#include "JBXVTSelectionData.h"
#include "screen.h"
#include "selend.h"
#include "size.h"
#include <string.h>
static void copy(uint8_t * str, uint8_t * scr_text,
        const int16_t start, const size_t len,
        const uint16_t total)
{
    if (!len)
        return; // prevent segmentation fault on empty data.
    char * dest = (char *) str + total - 1;
    char * src = (char *) scr_text + start;
    strncpy(dest, src, len + 1);
}
    __attribute__((const))
static inline bool end_point_on_screen(const struct JBDim pos,
        const struct JBDim char_sz)
{
    return pos.col < char_sz.col && pos.row < char_sz.row;
}
    __attribute__((pure))
static bool on_screen(const struct JBDim * restrict e)
{
    const struct JBDim c = jbxvt_get_char_size();
    return end_point_on_screen(e[0], c)
        && end_point_on_screen(e[1], c);
}
struct CopyData {
    struct JBDim * endpoints;
    uint8_t * string;
    uint32_t total;
    int32_t char_width; /*  Signed type used to prevent mixing of signed
                            and unsigned type in conditional expression
                            later. */
};
struct CopyData * new_CopyData(){
    struct CopyData *d;
    d=malloc(sizeof(struct CopyData));
    d->endpoints=NULL;
    d->string=NULL;
    d->total=0;
    d->char_width=0;
    return d;
}
static size_t copy_line(const int i, const int j,
        struct CopyData * restrict d)
{
    size_t r;
    if (i <= j) {
        /* Use full screen width if not first or last lines of
         * selection, otherwise use the col field in the respective
         * end point.  */
        struct JBDim * restrict e = d->endpoints;
        const int16_t start = i == e[0].index ? e[0].col : 0,
              end = i == j ? e[1].col : d->char_width - 1;
        /* We are positive so long as window exists and has a
         * geometry, so the conversion to size_t is valid for len. */
        const size_t len = (size_t)(end - start),
              new_total = d->total + len;
        d->string = realloc(d->string, (size_t)new_total);
        copy(d->string, jbxvt_get_line(i)->text, start,
                (size_t)len, d->total);
        d->total = new_total;
        d->string[d->total-1]='\n';
        r=copy_line(i + 1, j, d);
    }else
        r=d->total;
    return r;
}
/*  Convert the currently marked screen selection as a text string
    and save it as the current saved selection. */
void jbxvt_save_selection(struct JBXVTSelectionData * sel)
{
    const bool fwd = jbxvt_selcmp(&sel->end[0], &sel->end[1]) <= 0;
    // properly order start and end points:
    struct JBDim e[] = {sel->end[fwd ? 0 : 1],
        sel->end[fwd ? 1 : 0]};
    if(sel->text)
        free(sel->text);
    /* Use a normal int type for total to avoid overflow and failure to
     * detect decrement below 0. */
    // Make sure the selection falls within the screen area:
    if (!on_screen(e))
        return; // Invalid end point found
    struct CopyData d = {.endpoints = e, .total = 1,
        .char_width = (int16_t)jbxvt_get_char_size().width,
        .string=malloc(1)};
    // Copy each line in the selection:
    d.total = copy_line(e[0].index, e[1].index, &d);
    bool last_was_cr = false;
    // Substitute the first null terminator with a carriage return:
    for (size_t i = 0; i < d.total; ++i)
        if (d.string[i] == '\0') {
            if (!last_was_cr) {
                d.string[i] = '\r';
                last_was_cr = true;
            }
        } else
            last_was_cr = false;
    sel->text = d.string = realloc(d.string, d.total);
    d.string[sel->length=d.total-1]=0;//NUL
}
