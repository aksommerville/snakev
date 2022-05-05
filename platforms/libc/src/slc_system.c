#include "slc_system.h"
#include <string.h>
#include <termio.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/poll.h>

/* Globals.
 */
 
static struct termios slc_termios_restore={0};
static int slc_termios_restore_present=0;
static char slc_input[256];
static int slc_inputp=0;
static int slc_inputc=0;
static volatile int slc_sigc=0;
static int slc_time0_s=0;
static int slc_term_colc=80;
static int slc_term_rowc=25;

/* Read size of terminal.
 */
 
static void slc_read_terminal_size() {
  struct winsize ws={0};
  if (ioctl(STDOUT_FILENO,TIOCGWINSZ,&ws)>=0) {
    slc_term_colc=ws.ws_col;
    slc_term_rowc=ws.ws_row;
  }
}

/* Signals.
 */
 
static void slc_rcvsig(int sigid) {
  switch (sigid) {
    case SIGINT: if (++slc_sigc>=3) slc_abort("Too many unprocessed signals"); break;
    case SIGWINCH: slc_read_terminal_size(); break;
  }
}

void slc_signal_init() {
  signal(SIGINT,slc_rcvsig);
  signal(SIGWINCH,slc_rcvsig);
}

int slc_signal_get() {
  int retval=slc_sigc;
  slc_sigc=0;
  return retval;
}

/* Abort.
 */
 
void slc_abort(const char *fmt,...) {
  if (fmt) {
    va_list vargs;
    va_start(vargs,fmt);
    vfprintf(stderr,fmt,vargs);
    fprintf(stderr,"\n");
  }
  exit(1);
}

/* Set up termios.
 */
 
static void slc_termios_quit() {
  if (slc_termios_restore_present) {
    tcsetattr(STDIN_FILENO,TCSANOW,&slc_termios_restore);
  }
  slc_terminal_show_cursor(1);
}
 
int slc_termios_init() {
  atexit(slc_termios_quit);
  if (tcgetattr(STDIN_FILENO,&slc_termios_restore)>=0) {
    struct termios termios=slc_termios_restore;
    termios.c_lflag&=~(ICANON|ECHO);
    if (tcsetattr(STDIN_FILENO,TCSANOW,&termios)>=0) {
      slc_termios_restore_present=1;
    }
  }
  slc_read_terminal_size();
  return 0;
}

/* Parse stdin.
 */
 
static int slc_parse_stdin(int *input,const char *src,int srcc) {

  /**
  fprintf(stderr,"FROM STDIN:");
  int i=0; for (;i<srcc;i++) fprintf(stderr," %02x",(unsigned char)src[i]);
  fprintf(stderr,"\n");
  /**/
  
  /* Here's what I get for ESC,LEFT,RIGHT,UP,DOWN.
   * TODO There's probably a "right" way to do this, and this surely isn't it.
   * FROM STDIN: 1b        ESC
   * FROM STDIN: 1b 5b 44  LEFT
   * FROM STDIN: 1b 5b 43  RIGHT
   * FROM STDIN: 1b 5b 41  UP
   * FROM STDIN: 1b 5b 42  DOWN
   */
   
  *input=0;
  if (srcc<1) return 0;
  
  if ((src[0]==0x0a)||(src[0]==0x0d)) {
    *input=SLC_STDIN_ENTER;
    return 1;
  }
  
  if (src[0]!=0x1b) return 1;
  
  if ((srcc>=3)&&(src[1]==0x5b)) switch (src[2]) {
    case 0x44: *input=SLC_STDIN_LEFT; return 3;
    case 0x43: *input=SLC_STDIN_RIGHT; return 3;
    case 0x41: *input=SLC_STDIN_UP; return 3;
    case 0x42: *input=SLC_STDIN_DOWN; return 3;
  }
  
  *input=SLC_STDIN_ESC;
  return 1;
}

/* Poll, read, and parse stdin.
 */
 
int slc_poll_stdin(int to_ms) {
  while (slc_inputc) {
    int input=0,err;
    if ((err=slc_parse_stdin(&input,slc_input+slc_inputp,slc_inputc))<=0) return -1;
    if (slc_inputc-=err) slc_inputp+=err;
    else slc_inputp=0;
    if (input) return input;
  }
  struct pollfd pollfd={.fd=STDIN_FILENO,.events=POLLIN|POLLERR|POLLHUP};
  if (poll(&pollfd,1,to_ms)<=0) return 0;
  if (pollfd.revents) {
    int err=read(STDIN_FILENO,slc_input,sizeof(slc_input));
    if (err<=0) return -1;
    slc_inputp=0;
    slc_inputc=err;
  }
  while (slc_inputc) {
    int input=0,err;
    if ((err=slc_parse_stdin(&input,slc_input+slc_inputp,slc_inputc))<=0) return -1;
    if (slc_inputc-=err) slc_inputp+=err;
    else slc_inputp=0;
    if (input) return input;
  }
  return 0;
}

/* Last reported terminal size.
 */
 
void slc_get_terminal_size(int *w,int *h) {
  *w=slc_term_colc;
  *h=slc_term_rowc;
}

/* Clear screen.
 */
 
void slc_terminal_clear() {
  if (write(STDOUT_FILENO,"\x1b[2J",4)!=4) slc_abort("write failed");
}

/* Write a newline at the bottom of the screen to put the cursor back in a sane position.
 */
 
void slc_terminal_to_end() {
  slc_terminal_write(0,slc_term_rowc-1,"\n");
}

/* Write something at a specific position.
 */
 
void slc_terminal_write(int col,int row,const char *src) {
  if (!src) return;
  int srcc=strlen(src);
  if ((col<0)||(row<0)||(row>=slc_term_rowc)) return;
  if (col>slc_term_colc-srcc) srcc=slc_term_colc-col;
  if (srcc<1) return;
  char movecmd[32];
  int movecmdc=snprintf(movecmd,sizeof(movecmd),"\x1b[%d;%dH",row+1,col+1);
  if ((movecmdc<0)||(movecmdc>=sizeof(movecmd))) return;
  if (write(STDOUT_FILENO,movecmd,movecmdc)!=movecmdc) slc_abort("write failed");
  if (write(STDOUT_FILENO,src,srcc)!=srcc) slc_abort("write failed");
}

/* Show or hide cursor.
 */
 
void slc_terminal_show_cursor(int show) {
  if (show) {
    if (write(STDOUT_FILENO,"\x1b[?25h",6)!=6) slc_abort("write failed");
  } else {
    if (write(STDOUT_FILENO,"\x1b[?25l",6)!=6) slc_abort("write failed");
  }
}

/* Current time in milliseconds.
 */
 
int slc_now_ms() {
  struct timeval tv={0};
  gettimeofday(&tv,0);
  if (!slc_time0_s) slc_time0_s=tv.tv_sec;
  return (tv.tv_sec-slc_time0_s)*1000+tv.tv_usec/1000;
}
