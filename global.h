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
extern uint16_t sline_max; // max # of saved lines;

#endif//!GLOBAL_H
