#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>

#include "lingo/core/document.hpp"

namespace {
constexpr char input_pipe_name[] = "\\\\.\\pipe\\lingo";
constexpr char focus_pipe_name[] = "\\\\.\\pipe\\lingo-focus";
std::mutex document_mutex;
lingo::core::Document document;
std::mutex focus_mutex;
HANDLE focus_pipe = INVALID_HANDLE_VALUE;
std::mutex log_mutex;
std::ofstream log_file("lingo-service.log", std::ios::app);

void log(const std::string& message) {
  std::lock_guard lock(log_mutex);
  log_file << message << std::endl;
}

bool write_message(HANDLE pipe, const std::string& message) {
  const std::string line = message + "\n";
  DWORD written = 0;
  if (!WriteFile(pipe, line.data(), static_cast<DWORD>(line.size()), &written, nullptr) ||
      written != line.size()) {
    log("WriteFile failed error=" + std::to_string(GetLastError()));
    return false;
  }
  return true;
}

std::string json_text(const std::string& message) {
  const auto marker = message.find("\"text\":\"");
  if (marker == std::string::npos) return {};
  std::string result;
  bool escaped = false;
  for (size_t position = marker + 8; position < message.size(); ++position) {
    const char character = message[position];
    if (escaped) {
      if (character == 'n') result += '\n';
      else if (character == 'r') result += '\r';
      else if (character == 't') result += '\t';
      else result += character;
      escaped = false;
    } else if (character == '\\') {
      escaped = true;
    } else if (character == '"') {
      break;
    } else {
      result += character;
    }
  }
  return result;
}

std::string json_escape(const std::string& value) {
  std::string result;
  for (const char character : value) {
    if (character == '\\') result += "\\\\";
    else if (character == '"') result += "\\\"";
    else if (character == '\n') result += "\\n";
    else if (character == '\r') result += "\\r";
    else if (character == '\t') result += "\\t";
    else result += character;
  }
  return result;
}

std::string focus_message() {
  std::lock_guard lock(document_mutex);
  return "{\"type\":\"focus\",\"sentence\":\"" +
         json_escape(document.current_sentence()) + "\",\"word\":\"" +
         json_escape(document.current_word()) + "\"}";
}

void broadcast_focus() {
  const std::string message = focus_message();
  std::lock_guard lock(focus_mutex);
  if (focus_pipe == INVALID_HANDLE_VALUE) {
    log("focus has no subscriber");
    return;
  }
  if (write_message(focus_pipe, message)) {
    log("focus sent " + message);
  } else {
    CloseHandle(focus_pipe);
    focus_pipe = INVALID_HANDLE_VALUE;
    log("focus subscriber disconnected");
  }
}

void process_line(const std::string& line) {
  log("line=" + line);
  {
    std::lock_guard lock(document_mutex);
    if (line.find("document/change") != std::string::npos) {
      document.replace(json_text(line));
    }
    const auto offset_marker = line.find("\"offset\":");
    if (offset_marker != std::string::npos) {
      try {
        document.set_cursor(std::stoull(line.substr(offset_marker + 9)));
      } catch (...) {
        log("invalid cursor offset");
        return;
      }
    }
    log("focus=" + document.current_sentence() + " word=" + document.current_word());
  }
  broadcast_focus();
}

void serve_input(HANDLE pipe) {
  std::string pending;
  char buffer[4096]{};
  DWORD bytes_read = 0;
  while (ReadFile(pipe, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read) {
    pending.append(buffer, bytes_read);
    size_t end = 0;
    while ((end = pending.find('\n')) != std::string::npos) {
      process_line(pending.substr(0, end));
      pending.erase(0, end + 1);
    }
  }
  log("editor disconnected");
  CloseHandle(pipe);
}

void focus_server() {
  for (;;) {
    HANDLE pipe = CreateNamedPipeA(
        focus_pipe_name, PIPE_ACCESS_OUTBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 8192, 8192, 0,
        nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
      log("CreateNamedPipe focus failed");
      return;
    }
    if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) {
      CloseHandle(pipe);
      continue;
    }
    {
      std::lock_guard lock(focus_mutex);
      focus_pipe = pipe;
    }
    log("focus subscriber connected");
    if (!write_message(pipe, R"({"type":"status","connected":true})") ||
        !write_message(pipe, focus_message())) {
      std::lock_guard lock(focus_mutex);
      CloseHandle(pipe);
      focus_pipe = INVALID_HANDLE_VALUE;
      continue;
    }
    while (true) {
      {
        std::lock_guard lock(focus_mutex);
        if (focus_pipe != pipe) break;
      }
      Sleep(100);
    }
  }
}
}

int main() {
  log("service started");
  std::cout << "Listening: " << input_pipe_name << std::endl;
  std::thread(focus_server).detach();
  for (;;) {
    HANDLE pipe = CreateNamedPipeA(
        input_pipe_name, PIPE_ACCESS_INBOUND,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 8, 0, 8192, 0,
        nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
      log("CreateNamedPipe input failed");
      return 1;
    }
    if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
      log("editor accepted");
      std::thread(serve_input, pipe).detach();
    } else {
      CloseHandle(pipe);
    }
  }
}
