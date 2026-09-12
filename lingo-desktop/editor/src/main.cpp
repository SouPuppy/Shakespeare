#include <string>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fstream>
#include <sstream>

#include "editor.hpp"
#include "screen.hpp"

int main() {
  std::ofstream debug_log("lingo-editor.log", std::ios::app);
  const auto log = [&](const std::string& message) {
    debug_log << message << std::endl;
    OutputDebugStringA((message + "\n").c_str());
  };
  log("editor started");
  Terminal terminal;
  size_t width = 0;
  size_t height = 0;
  Editor editor;

  if (!terminal.start(width, height)) {
    log("terminal start failed error=" + std::to_string(GetLastError()));
    return 1;
  }
  log("terminal started width=" + std::to_string(width) + " height=" + std::to_string(height));
  Screen screen(width, height);
  HANDLE service = INVALID_HANDLE_VALUE;
  for (int attempt = 0; attempt < 50 && service == INVALID_HANDLE_VALUE; ++attempt) {
    service = CreateFileA("\\\\.\\pipe\\lingo", GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
    if (service == INVALID_HANDLE_VALUE) Sleep(20);
  }
  log(service == INVALID_HANDLE_VALUE ? "service connect failed error=" + std::to_string(GetLastError()) : "service connected");
  auto sync_service = [&] {
    if (service == INVALID_HANDLE_VALUE) { OutputDebugStringA("[editor] service pipe unavailable\n"); return; }
    std::string text = editor.document.buffer.getText();
    for (size_t position = 0; (position = text.find('"', position)) != std::string::npos; position += 2) text.insert(position, "\\");
    for (size_t position = 0; (position = text.find('\n', position)) != std::string::npos; position += 2) { text.replace(position, 1, "\\n"); }
    const std::string message = "{\"type\":\"document/change\",\"text\":\"" + text + "\"}\n{\"type\":\"cursor/change\",\"offset\":" + std::to_string(editor.document.cursor) + "}\n";
    DWORD written = 0;
    const bool success = WriteFile(service, message.data(), static_cast<DWORD>(message.size()), &written, nullptr) != 0;
    std::ostringstream debug;
    debug << "sync success=" << success << " bytes=" << written << " cursor=" << editor.document.cursor;
    log(debug.str());
  };
  sync_service();

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
    sync_service();
  }

  terminal.stop();
  if (service != INVALID_HANDLE_VALUE) {
    CloseHandle(service);
  }
}
