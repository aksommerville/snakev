/* slc_system.h
 * Every libc thing we do is wrapped here.
 * Game logic should only include "slc" headers.
 */
 
#ifndef SLC_SYSTEM_H
#define SLC_SYSTEM_H

void slc_signal_init();
int slc_signal_get();

void slc_abort(const char *fmt,...);

int slc_termios_init();

int slc_poll_stdin(int to_ms);
#define SLC_STDIN_LEFT     1
#define SLC_STDIN_RIGHT    2
#define SLC_STDIN_UP       3
#define SLC_STDIN_DOWN     4
#define SLC_STDIN_ESC      5
#define SLC_STDIN_ENTER    6

void slc_get_terminal_size(int *w,int *h);

void slc_terminal_clear();
void slc_terminal_to_end();
void slc_terminal_show_cursor(int show);

void slc_terminal_write(int col,int row,const char *src);

int slc_now_ms();

#endif
