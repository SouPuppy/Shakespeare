#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include "lingo/core/document.hpp"

namespace {
constexpr char pipe_name[] = "\\\\.\\pipe\\lingo";
std::mutex clients_mutex;
std::vector<HANDLE> clients;
lingo::core::Document document;
std::mutex log_mutex;
std::ofstream log_file("lingo-service.log", std::ios::app);

void broadcast(const std::string& message) {
  const std::string line = message + "\n";
  std::lock_guard lock(clients_mutex);
  { std::lock_guard log_lock(log_mutex); log_file << "broadcast clients=" << clients.size() << " message=" << message << std::endl; }
  for (auto it = clients.begin(); it != clients.end();) {
    DWORD written = 0;
    if (WriteFile(*it, line.data(), static_cast<DWORD>(line.size()), &written, nullptr)) ++it;
    else { CloseHandle(*it); it = clients.erase(it); }
  }
}

std::string json_text(const std::string& message) {
  const auto marker = message.find("\"text\":\"");
  if (marker == std::string::npos) return {};
  std::string result;
  bool escaped = false;
  for (size_t i = marker + 8; i < message.size(); ++i) {
    const char c = message[i];
    if (escaped) { result += c == 'n' ? '\n' : c; escaped = false; continue; }
    if (c == '\\') { escaped = true; continue; }
    if (c == '"') break;
    result += c;
  }
  return result;
}

std::string json_escape(const std::string& value) {
  std::string result;
  for (char c : value) {
    if (c == '\\') result += "\\\\";
    else if (c == '"') result += "\\\"";
    else if (c == '\n') result += "\\n";
    else if (c == '\r') result += "\\r";
    else result += c;
  }
  return result;
}

void process_line(const std::string& line) {
  { std::lock_guard lock(log_mutex); log_file << "line=" << line << std::endl; }
  if (line.find("document/change") != std::string::npos) document.replace(json_text(line));
  const auto offset = line.find("\"offset\":");
  if (offset != std::string::npos) {
    try { document.set_cursor(std::stoull(line.substr(offset + 9))); } catch (...) { return; }
  }
  const std::string focus = document.current_sentence();
  { std::lock_guard lock(log_mutex); log_file << "focus=" << focus << std::endl; }
  broadcast("{\"type\":\"focus\",\"sentence\":\"" + json_escape(focus) + "\",\"word\":\"" + json_escape(document.current_word()) + "\"}");
}

void serve(HANDLE pipe) {
  { std::lock_guard lock(clients_mutex); clients.push_back(pipe); }
  std::string pending;
  char buffer[4096]{};
  DWORD read = 0;
  while (ReadFile(pipe, buffer, sizeof(buffer), &read, nullptr) && read) {
    pending.append(buffer, read);
    size_t end = 0;
    while ((end = pending.find('\n')) != std::string::npos) {
      process_line(pending.substr(0, end));
      pending.erase(0, end + 1);
    }
  }
  std::lock_guard lock(clients_mutex);
  clients.erase(std::remove(clients.begin(), clients.end(), pipe), clients.end());
  CloseHandle(pipe);
}
}

int main() {
  std::cout << "Listening: " << pipe_name << std::endl;
  for (;;) {
    HANDLE pipe = CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 8, 8192, 8192, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return 1;
    if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) {
      const char status[] = R"({"type":"status","connected":true}
)";
      DWORD written = 0;
      WriteFile(pipe, status, static_cast<DWORD>(sizeof(status) - 1), &written, nullptr);
      std::thread(serve, pipe).detach();
    }
    else CloseHandle(pipe);
  }
}
