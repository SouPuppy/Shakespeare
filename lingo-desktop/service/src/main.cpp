#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <algorithm>
#include <mutex>
#include <thread>
#include <vector>
#include <string>
#include <iostream>
#include "lingo/core/document.hpp"

namespace { std::mutex clients_mutex; std::vector<HANDLE> clients; std::string document; size_t cursor = 0; lingo::core::Document core_document;
void broadcast(const std::string& value) { std::lock_guard lock(clients_mutex); const auto line=value+"\n"; for(auto h:clients){DWORD written=0;WriteFile(h,line.data(),(DWORD)line.size(),&written,nullptr);} }
void serve(HANDLE pipe) { {std::lock_guard lock(clients_mutex);clients.push_back(pipe);} broadcast(R"({"type":"status","connected":true})"); char buffer[4096]{}; DWORD read=0; while(ReadFile(pipe,buffer,sizeof(buffer)-1,&read,nullptr)&&read){buffer[read]=0;std::string message(buffer);auto t=message.find("\"text\":\"");if(t!=std::string::npos){auto s=t+8;document=message.substr(s,message.find('"',s)-s);core_document.replace(document);}auto o=message.find("\"offset\":");if(o!=std::string::npos){cursor=std::min(document.size(),std::stoull(message.substr(o+9)));core_document.set_cursor(cursor);}broadcast("{\"type\":\"sentence\",\"text\":\""+core_document.current_sentence()+"\"}");} {std::lock_guard lock(clients_mutex);clients.erase(std::remove(clients.begin(),clients.end(),pipe),clients.end());} CloseHandle(pipe); }
}

int main() {
  HANDLE pipe = CreateNamedPipeA("\\\\.\\pipe\\lingo", PIPE_ACCESS_DUPLEX,
      PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 2, 4096, 4096, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) return 1;
  std::cout << "Listening: \\\\.\\pipe\\lingo" << std::endl;
  if (!ConnectNamedPipe(pipe, nullptr) && GetLastError() != ERROR_PIPE_CONNECTED) return 1;
  const char status[] = R"({"type":"status","connected":true}
)";
  DWORD written = 0;
  WriteFile(pipe, status, static_cast<DWORD>(sizeof(status) - 1), &written, nullptr);
  std::thread(serve, pipe).detach();
  for (;;) {
    pipe = CreateNamedPipeA("\\\\.\\pipe\\lingo", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 8, 4096, 4096, 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) return 1;
    if (ConnectNamedPipe(pipe, nullptr) || GetLastError() == ERROR_PIPE_CONNECTED) std::thread(serve, pipe).detach();
    else CloseHandle(pipe);
  }
  /* char buffer[4096]{}; DWORD read = 0;
  while (ReadFile(pipe, buffer, sizeof(buffer) - 1, &read, nullptr) && read) {
    buffer[read] = 0; std::cout << buffer << std::flush;
  }
  DisconnectNamedPipe(pipe); CloseHandle(pipe); */
}
