#ifndef GLOBAL_H
#define GLOBAL_H

#include "command.h"
#include "selst.h"
#include <stdbool.h>
#include <X11/Xlib.h>

// Globals:
extern int send_count, comm_fd, fd_width, sline_top;
extern unsigned char *com_buf_next, *com_buf_top, *send_buf, *send_nxt,
       *com_stack_top;
extern unsigned char com_stack[COM_PUSH_MAX], com_buf[COM_BUF_SIZE];
extern struct slinest **sline;
extern uint8_t fheight, fwidth;
extern uint16_t sline_max; // max # of saved lines;
extern int		reverse_wrap;	/* reverse wrap allowed */
extern struct selst selend1, selend2;	/* the selection endpoints */
extern struct selst selanchor;		/* the selection anchor */

#endif//!GLOBAL_H
