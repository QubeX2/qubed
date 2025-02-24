#ifndef QUBED_H
#define QUBED_H

#include <termios.h>

#define QUBED_VERSION "0.0.1"
#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

struct editor_config {
  int cx, cy;
  int screen_rows, screen_cols;
  struct termios orig_termios;
};

struct abuf {
  char *b;
  int len;
};

void enable_raw_mode();
void disable_raw_mode();
void die(const char *s);
void editor_process_keypress();
char editor_read_key();
void editor_refresh_screen();
void editor_clear_screen();
void editor_draw_rows(struct abuf *ab);
void editor_move_cursor(char key);
int get_window_size(int *rows, int *cols);
int get_cursor_position(int *rows, int *cols);
void ab_append(struct abuf *ab, const char *s, int len);
void ab_free(struct abuf *ab);

void init_editor();

#endif
