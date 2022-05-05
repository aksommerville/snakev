#include "slc_system.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

// These could be much larger, but the game is kind of dull when it's wide open.
#define MAX_FIELD_W  40
#define MAX_FIELD_H  30

/* Globals.
 */
 
static int frame_time_ms=100;
 
// Field (playable area) in cells. A cell is 2x1 characters in the terminal.
static int slc_fieldw,slc_fieldh;

// Rendering offset of the field and borders, it's not always the full terminal size.
static int slc_screenx,slc_screeny;
static int slc_screenw,slc_screenh;

// Value of each cell in the field tells us which direction points toward the head.
static char field[MAX_FIELD_W*MAX_FIELD_H];
static int headx,heady;
static int tailx,taily;
#define SLC_TILE_EMPTY 0
#define SLC_TILE_LEFT 1
#define SLC_TILE_RIGHT 2
#define SLC_TILE_UP 3
#define SLC_TILE_DOWN 4
#define SLC_TILE_HEAD 5
#define SLC_TILE_SNACK 6

static int snakedx=0,snakedy=0;

static int running=0;
static int score=0;

/* Set and draw one tile.
 */
 
static void slc_set_tile(int x,int y,char tileid) {
  if ((x<0)||(y<0)||(x>=slc_fieldw)||(y>=slc_fieldh)) return;
  char *dst=field+y*slc_fieldw+x;
  if (*dst==tileid) return;
  *dst=tileid;
  const char *img="  ";
  switch (tileid) {
    case SLC_TILE_LEFT: img="<<"; break;
    case SLC_TILE_RIGHT: img=">>"; break;
    case SLC_TILE_UP: img="^^"; break;
    case SLC_TILE_DOWN: img="vv"; break;
    case SLC_TILE_HEAD: img="OO"; break;
    case SLC_TILE_SNACK: img=":)"; break;
  }
  slc_terminal_write(slc_screenx+1+(x<<1),slc_screeny+2+y,img);
}

/* Set score.
 */
 
static void slc_set_score(int newscore) {
  if (newscore==score) return;
  score=newscore;
  char msg[32];
  int msgc=snprintf(msg,sizeof(msg),"    %d    ",score);
  if ((msgc<1)||(msgc>=sizeof(msg))) return;
  slc_terminal_write(slc_screenx+(slc_screenw>>1)-(msgc>>1),slc_screeny,msg);
}

/* Game over.
 */
 
static void slc_end_game() {
  running=0;
  int x=slc_screenx+(slc_screenw>>1)-6;
  int y=slc_screeny+(slc_screenh>>1)-1;
  slc_terminal_write(x,y  ,"           ");
  slc_terminal_write(x,y+1," GAME OVER ");
  slc_terminal_write(x,y+2,"           ");
}

/* Generate a new snack.
 */
 
static void slc_generate_snack() {
  int panic=1000;
  while (1) {
    int x=rand()%slc_fieldw;
    int y=rand()%slc_fieldh;
    char *dst=field+y*slc_fieldw+x;
    if (!*dst) {
      slc_set_tile(x,y,SLC_TILE_SNACK);
      return;
    }
    if (!--panic) {
      slc_end_game();
      return;
    }
  }
}

/* Nonzero if the given cell can be moved to.
 */
 
static int slc_check_cell(int x,int y) {
  if (x<0) return 0;
  if (y<0) return 0;
  if (x>=slc_fieldw) return 0;
  if (y>=slc_fieldh) return 0;
  char tile=field[y*slc_fieldw+x];
  if (tile==SLC_TILE_EMPTY) return 1;
  if (tile==SLC_TILE_SNACK) return 2;
  return 0;
}

/* Cascade tail movement.
 */
 
static void slc_cascade_movement() {
  int ox=tailx,oy=taily;
  switch (field[taily*slc_fieldw+tailx]) {
    case SLC_TILE_LEFT: tailx--; break;
    case SLC_TILE_RIGHT: tailx++; break;
    case SLC_TILE_UP: taily--; break;
    case SLC_TILE_DOWN: taily++; break;
    case SLC_TILE_HEAD: tailx=headx; taily=heady; return;
    default: return;
  }
  slc_set_tile(ox,oy,SLC_TILE_EMPTY);
}

/* Update one frame.
 */
 
static void slc_update() {
  if (!running) return;
  if (snakedx||snakedy) {
    int nx=headx+snakedx,ny=heady+snakedy,disp;
    if (disp=slc_check_cell(nx,ny)) {
    
      slc_set_tile(nx,ny,SLC_TILE_HEAD);
      if (disp==2) {
        slc_set_tile(headx,heady,(snakedx<0)?SLC_TILE_LEFT:(snakedx>0)?SLC_TILE_RIGHT:(snakedy<0)?SLC_TILE_UP:SLC_TILE_DOWN);
        slc_generate_snack();
        slc_set_score(score+1);
      } else if ((tailx==headx)&&(taily==heady)) {
        slc_set_tile(headx,heady,SLC_TILE_EMPTY);
        tailx=nx;
        taily=ny;
      } else {
        slc_set_tile(headx,heady,(snakedx<0)?SLC_TILE_LEFT:(snakedx>0)?SLC_TILE_RIGHT:(snakedy<0)?SLC_TILE_UP:SLC_TILE_DOWN);
      }
    
      headx=nx;
      heady=ny;
      
      if (disp!=2) {
        slc_cascade_movement();
      }
    
    } else {
      slc_end_game();
    }
  }
}

/* Receive snake inputs.
 */
 
static void slc_move(int dx,int dy) {
  if (!running) return;
  
  // Don't let him turn backward if a tail exists; assume it's a typo.
  if (score>0) {
    if (dx&&(dx==-snakedx)) return;
    if (dy&&(dy==-snakedy)) return;
  }
  
  snakedx=dx;
  snakedy=dy;
}

/* Prepare game.
 */
 
static void slc_setup(int w,int h) {
  slc_terminal_show_cursor(0);

  if ((w<8)||(h<5)) slc_abort("terminal size %dx%d is too small",w,h);
  slc_fieldw=(w-2)>>1;
  slc_fieldh=h-3;
  if (slc_fieldw>MAX_FIELD_W) slc_fieldw=MAX_FIELD_W;
  if (slc_fieldh>MAX_FIELD_H) slc_fieldh=MAX_FIELD_H;
  
  slc_screenw=(slc_fieldw<<1)+2;
  slc_screenh=slc_fieldh+3;
  slc_screenx=(w>>1)-(slc_screenw>>1);
  slc_screeny=(h>>1)-(slc_screenh>>1);

  slc_terminal_clear();
  
  // Draw the border.
  slc_terminal_write(slc_screenx,slc_screeny+1,"+");
  slc_terminal_write(slc_screenx+slc_screenw-1,slc_screeny+1,"+");
  slc_terminal_write(slc_screenx,slc_screeny+slc_screenh-1,"+");
  slc_terminal_write(slc_screenx+slc_screenw-1,slc_screeny+slc_screenh-1,"+");
  int borderh=slc_screenh-3;
  int y=slc_screeny+2;
  for (;borderh-->0;y++) {
    slc_terminal_write(slc_screenx,y,"|");
    slc_terminal_write(slc_screenx+slc_screenw-1,y,"|");
  }
  char tmp[(MAX_FIELD_W<<1)+1];
  int borderw=slc_fieldw<<1;
  memset(tmp,'-',borderw);
  tmp[borderw]=0;
  slc_terminal_write(slc_screenx+1,slc_screeny+1,tmp);
  slc_terminal_write(slc_screenx+1,slc_screeny+slc_screenh-1,tmp);
  
  memset(field,SLC_TILE_EMPTY,slc_fieldw*slc_fieldh);
  headx=slc_fieldw>>1;
  heady=slc_fieldh>>1;
  tailx=headx;
  taily=heady;
  snakedx=0;
  snakedy=0;
  slc_set_tile(headx,heady,SLC_TILE_HEAD);
  
  slc_generate_snack();
  
  score=-1;
  slc_set_score(0);
  
  running=1;
}

/* Main.
 */

int main(int argc,char **argv) {

  srand(time(0));
  
  slc_signal_init();
  if (slc_termios_init()<0) slc_abort("slc_termios_init failed");
  
  int w=0,h=0;
  slc_get_terminal_size(&w,&h);
  slc_setup(w,h);
  
  int next_frame_time=slc_now_ms();
  int quit_requested=0;
  while (!quit_requested&&!slc_signal_get()) {
  
    int now=slc_now_ms();
    if (now>=next_frame_time) {
      slc_update();
      next_frame_time+=frame_time_ms;
      if (next_frame_time<now) next_frame_time=now; // panic reset
    }
    int timeout=next_frame_time-now;
  
    int input=slc_poll_stdin(timeout);
    if (input<0) slc_abort("slc_poll_stdin failed");
    switch (input) {
      case SLC_STDIN_LEFT: slc_move(-1,0); break;
      case SLC_STDIN_RIGHT: slc_move(1,0); break;
      case SLC_STDIN_UP: slc_move(0,-1); break;
      case SLC_STDIN_DOWN: slc_move(0,1); break;
      case SLC_STDIN_ESC: quit_requested=1; break;
      case SLC_STDIN_ENTER: if (!running) slc_setup(w,h); break;
    }
    
  }
  
  slc_terminal_to_end();
  slc_terminal_show_cursor(1);
  return 0;
}
