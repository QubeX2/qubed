#ifndef QUBED_H
#define QUBED_H

#include "abuf.h"
#include <termios.h>

#define QUBED_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

#define MODE_NORMAL 1
#define MODE_INSERT 2
#define MODE_VISUAL 3

enum editor_key {
  ARROW_LEFT = 1000,
  ARROW_RIGHT,
  ARROW_UP,
  ARROW_DOWN,
};

struct editor_config {
  int mode;
  int cx, cy;
  int screen_rows, screen_cols;
  struct termios orig_termios;
};

void enable_raw_mode();
void disable_raw_mode();
void die(const char *s);
void editor_process_keypress();
int editor_read_key();
void editor_refresh_screen();
void editor_clear_screen();
void editor_draw_rows(struct abuf *ab);
void editor_move_cursor(int key);
int get_window_size(int *rows, int *cols);
int get_cursor_position(int *rows, int *cols);
void set_mode(int mode);
void init_editor();

#endif
