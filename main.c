#include "editor.h"
#include <asm-generic/ioctls.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

int main(void) {
  enable_raw_mode();
  init_editor();

  while (1) {
    editor_refresh_screen();
    editor_process_keypress();
  }

  return 0;
}
