#include "editor.h"
#include <asm-generic/ioctls.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

struct editor_config E;

/**
 *
 */
void init_editor() {
  E.mode = MODE_NORMAL;
  E.cx = 0;
  E.cy = 0;

  if (get_window_size(&E.screen_rows, &E.screen_cols) == -1)
    die("get_window_size");
}

/**
 *
 */
void editor_move_cursor(int key) {
  switch (key) {

  case ARROW_LEFT:
  case 'h':
    if (E.cx != 0) {
      E.cx--;
    }
    break;

  case ARROW_RIGHT:
  case 'l':
    if (E.cx != E.screen_cols - 1) {
      E.cx++;
    }
    break;

  case ARROW_UP:
  case 'k':
    if (E.cy != 0) {
      E.cy--;
    }
    break;

  case ARROW_DOWN:
  case 'j':
    if (E.cy != E.screen_rows - 1) {
      E.cy++;
    }
    break;
  }
}

/**
 *
 */
int get_cursor_position(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
    return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
      break;
    if (buf[i] == 'R')
      break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[')
    return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
    return -1;
  return 0;
}

/**
 *
 */
int get_window_size(int *rows, int *cols) {
  struct winsize ws;

  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
      return -1;

    return get_cursor_position(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}

/**
 *
 */
void editor_draw_rows(struct abuf *ab) {
  for (int y = 0; y < E.screen_rows; y++) {
    if (y == E.screen_rows / 3) {
      char welcome[80];
      int welclen = snprintf(welcome, sizeof(welcome),
                             "qubed editor -- version %s", QUBED_VERSION);
      if (welclen > E.screen_cols)
        welclen = E.screen_cols;

      int padding = (E.screen_cols - welclen) / 2;
      if (padding) {
        ab_append(ab, "~", 1);
        padding--;
      }
      while (padding--)
        ab_append(ab, " ", 1);

      ab_append(ab, welcome, welclen);
    } else {
      ab_append(ab, "~", 1);
    }

    ab_append(ab, "\x1b[K", 3);
    if (y < E.screen_rows - 1) {
      ab_append(ab, "\r\n", 2);
    }
  }
}

/**
 *
 */
void editor_clear_screen() {
  write(STDOUT_FILENO, "\x1b[2J", 4);
  write(STDOUT_FILENO, "\x1b[H", 3);
}

/**
 *
 */
void editor_refresh_screen() {
  struct abuf ab = ABUF_INIT;

  ab_append(&ab, "\x1b[?25l", 6);
  ab_append(&ab, "\x1b[H", 3);

  editor_draw_rows(&ab);

  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  ab_append(&ab, buf, strlen(buf));

  // ab_append(&ab, "\x1b[H", 3);
  ab_append(&ab, "\x1b[?25h", 6);

  write(STDOUT_FILENO, ab.b, ab.len);
  ab_free(&ab);
}

/**
 *,
 */
int editor_read_key() {
  int nread;
  char c;
  while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
    if (nread == -1 && errno != EAGAIN)
      die("read");
  }
  if (c == '\x1b') {
    char seq[3];
    if (read(STDIN_FILENO, &seq[0], 1) != 1)
      return '\x1b';
    if (read(STDIN_FILENO, &seq[1], 1) != 1)
      return '\x1b';
    if (seq[0] == '[') {
      switch (seq[1]) {
      case 'A':
        return ARROW_UP;
      case 'B':
        return ARROW_DOWN;
      case 'C':
        return ARROW_RIGHT;
      case 'D':
        return ARROW_LEFT;
      }
    }
    return '\x1b';
  } else {
    return c;
  }
}

/**
 *
 */
void set_mode(int mode) { E.mode = mode; }

/**
 *
 */
void editor_process_keypress() {
  int c = editor_read_key();

  switch (c) {
  case CTRL_KEY('q'):
    editor_clear_screen();
    exit(0);
    break;

  case CTRL_KEY('i'):
    set_mode(MODE_INSERT);
    break;

  case CTRL_KEY('n'):
    set_mode(MODE_NORMAL);
    break;

  case ARROW_LEFT:
  case ARROW_RIGHT:
  case ARROW_UP:
  case ARROW_DOWN:
    editor_move_cursor(c);
    break;

  case 'h':
  case 'j':
  case 'k':
  case 'l':
    if (E.mode == MODE_NORMAL) {
      editor_move_cursor(c);
    }
    break;
  }
}

/**
 * enable_raw_mode
 */
void enable_raw_mode() {
  if (tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
    die("tcgetattr");

  atexit(disable_raw_mode);

  struct termios raw = E.orig_termios;
  raw.c_iflag &= ~(BRKINT | INPCK | ISTRIP | ICRNL | IXON);
  raw.c_oflag &= ~(OPOST);
  raw.c_cflag |= (CS8);
  raw.c_lflag &= ~(ICRNL | ECHO | IEXTEN | ICANON | ISIG);
  raw.c_cc[VMIN] = 0;
  raw.c_cc[VTIME] = 1;

  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
    die("tcsetattr");
}

/**
 * disable_raw_mode
 */
void disable_raw_mode() {
  if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

/**
 * die
 */
void die(const char *s) {
  editor_clear_screen();

  perror(s);
  exit(1);
}
