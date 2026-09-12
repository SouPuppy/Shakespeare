#include <windows.h>

#include <filesystem>
#include <windows.h>
#include <string>

int main() {
  wchar_t module_path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, module_path, MAX_PATH);
  const std::filesystem::path root = std::filesystem::path(module_path).parent_path();
  const auto service = root / "lingo-service.exe";
  const auto popout = root.parent_path().parent_path().parent_path() / "lingo-desktop" / "app" / "popout" / "bin" / "Debug" / "net8.0-windows" / "lingo-popout.exe";
  const auto editor = root / "lingo.exe";
  STARTUPINFOW startup{sizeof(startup)};
  auto launch = [&](const std::filesystem::path& path, PROCESS_INFORMATION& child) {
    std::wstring command = L"\"" + path.wstring() + L"\"";
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, root.wstring().c_str(), &startup, &child)) return false;
    CloseHandle(child.hThread);
    return true;
  };
  PROCESS_INFORMATION service_process{};
  PROCESS_INFORMATION popout_process{};
  PROCESS_INFORMATION editor_process{};
  if (!launch(service, service_process)) return 1;
  if (!launch(popout, popout_process)) { TerminateProcess(service_process.hProcess, 1); CloseHandle(service_process.hProcess); return 1; }
  if (!launch(editor, editor_process)) { TerminateProcess(service_process.hProcess, 1); TerminateProcess(popout_process.hProcess, 1); return 1; }
  WaitForSingleObject(editor_process.hProcess, INFINITE);
  TerminateProcess(service_process.hProcess, 0);
  TerminateProcess(popout_process.hProcess, 0);
  CloseHandle(editor_process.hProcess);
  CloseHandle(service_process.hProcess);
  CloseHandle(popout_process.hProcess);
  return 0;
}
