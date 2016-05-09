#ifndef SCR_REFRESH_H
#define SCR_REFRESH_H

/*  Refresh the region of the screen delimited by the aruments.  Used to
 *  repair after minor exposure events.
 */
void scr_refresh(int x, int y, int width, int height);

#endif//SCR_REFRESH_H
