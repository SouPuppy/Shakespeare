#include <string>

#include "editor.hpp"
#include "screen.hpp"

int main() {
  Terminal terminal;
  size_t width = 0;
  size_t height = 0;
  Editor editor;

  if (!terminal.start(width, height)) return 1;
  Screen screen(width, height);

  for (;;) {
    editor.render(screen);
    terminal.present(screen, editor.cursor_row, editor.cursor_column);

    InputEvent event = terminal.input();

    if (event.key == Key::Escape) break;
    if (event.key == Key::Left || event.key == Key::Right || event.key == Key::Up || event.key == Key::Down) editor.move(event.key);
    if (event.key == Key::Character) editor.document.insert(std::string(1, event.character));
    else if (event.key == Key::Enter) editor.document.insert("\n");
    else if (event.key == Key::Backspace) editor.document.backspace();
    else if (event.key == Key::Delete) editor.document.erase();
  }

  terminal.stop();
}
