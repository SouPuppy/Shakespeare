#include <windows.h>

#include <filesystem>
#include <windows.h>
#include <string>

int main() {
  wchar_t module_path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, module_path, MAX_PATH);
  const std::filesystem::path root = std::filesystem::path(module_path).parent_path();
  const auto popout = root.parent_path().parent_path().parent_path() / "lingo-desktop" / "app" / "popout" / "bin" / "Debug" / "net8.0-windows" / "lingo.exe";
  STARTUPINFOW startup{sizeof(startup)};
  auto launch = [&](const std::filesystem::path& path, PROCESS_INFORMATION& child) {
    std::wstring command = L"\"" + path.wstring() + L"\"";
    if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, 0, nullptr, root.wstring().c_str(), &startup, &child)) return false;
    CloseHandle(child.hThread);
    return true;
  };
  PROCESS_INFORMATION popout_process{};
  if (!launch(popout, popout_process)) return 1;
  WaitForSingleObject(popout_process.hProcess, INFINITE);
  CloseHandle(popout_process.hProcess);
  return 0;
}
