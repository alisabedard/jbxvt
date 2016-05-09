#include "global.h"

int send_count;
int comm_fd = -1;	/* file descriptor connected to the command */
int fd_width;	/* width of file descriptors being used */
unsigned char *send_buf;	/* characters waiting to be sent to the command */
unsigned char *send_nxt;	/* next character to be sent */
int send_count;	/* number of characters waiting to be sent */
int sline_top;	/* high water mark of saved scrolled lines */
struct slinest **sline;	/* main array of saved lines */
int offset;	/* current vertical offset for displaying saved lines */
uint32_t rstyle;	/* rendition style and current character flags */
uint8_t fheight;	/* height of a character in the font */
uint8_t fwidth;	/* width of a font character */
int		reverse_wrap;	/* reverse wrap allowed */
bool save_rstyle;	/* when it needs to be saved */
uint16_t sline_max; // max # of saved lines;
struct selst selend1, selend2;	/* the selection endpoints */
struct selst selanchor;		/* the selection anchor */

//  Variables used for buffered command input.
unsigned char com_buf[COM_BUF_SIZE];
unsigned char *com_buf_next, *com_buf_top;
unsigned char com_stack[COM_PUSH_MAX];	/* stack of pushed back characters */
unsigned char *com_stack_top;

