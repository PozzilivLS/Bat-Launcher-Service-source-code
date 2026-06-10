#include "BatLauncherWinService.h"

void log(const std::wstring &msg) {
  wchar_t *appdataFilePath = 0;
  SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, NULL, &appdataFilePath);

  std::wstringstream ss;
  ss << appdataFilePath << L"\\BatLauncher\\BatLauncher.log";

  HANDLE hFile =
      CreateFileW(ss.str().c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                  OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

  if (hFile != INVALID_HANDLE_VALUE) {
    DWORD written;
    WriteFile(hFile, msg.c_str(), (DWORD)(msg.size() * sizeof(wchar_t)),
              &written, nullptr);
    WriteFile(hFile, L"\r\n", 4, &written, nullptr);
    CloseHandle(hFile);
  }
}

bool runBat(const std::wstring &path) {
  STARTUPINFOW si{};
  PROCESS_INFORMATION pi{};
  si.cb = sizeof(si);

  std::wstring cmd = L"cmd.exe /C \"" + path + L"\"";

  bool result = CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                               CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi);

  if (!result) return false;

  WaitForSingleObject(pi.hProcess, INFINITE);

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  return true;
}

bool terminateExe(const std::wstring &exeNames) {
  std::vector<std::wstring> namesList;
  log(exeNames);

  std::wstring st = L"";
  for (int i = 0; i < exeNames.length(); i++) {
    if (exeNames[i] == L'/') {
      namesList.emplace_back(st);
      st = L"";
    } else {
      st += exeNames[i];
      log(st);
    }
  }
  namesList.emplace_back(st);
  log(st);

  ////////////////////

  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnap == INVALID_HANDLE_VALUE) return false;

  PROCESSENTRY32W pe;
  pe.dwSize = sizeof(pe);

  for (const auto &prName : namesList) {
    if (Process32FirstW(hSnap, &pe)) {
      do {
        if (wcscmp(pe.szExeFile, prName.c_str()) == 0) {
          HANDLE hProc =
              OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
          if (hProc) {
            TerminateProcess(hProc, 0);
            CloseHandle(hProc);
          }
        }
      } while (Process32NextW(hSnap, &pe));
    }
  }
  CloseHandle(hSnap);

  return true;
}

void handleClient(HANDLE hPipe) {
  char buffer[512];
  DWORD bytesRead;

  while (ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, nullptr)) {
    std::string request(buffer, bytesRead);
    std::string response;

    if (request.rfind("RUN ", 0) == 0) {
      std::wstring path(request.begin() + 4, request.end());
      bool ok = runBat(path);
      response = ok ? "OK" : "ERROR";
    } else if (request.rfind("TERM ", 0) == 0) {
      std::wstring name(request.begin() + 5, request.end());
      bool ok = terminateExe(name);
      response = ok ? "OK" : "ERROR";
    } else if (request == "PING") {
      response = "PONG";
    } else {
      response = "UNKNOWN";
    }

    DWORD written;
    WriteFile(hPipe, response.c_str(), (DWORD)response.size(), &written,
              nullptr);
  }

  FlushFileBuffers(hPipe);
  DisconnectNamedPipe(hPipe);
  CloseHandle(hPipe);
}

void pipeServer() {
  while (g_Running) {
    SECURITY_ATTRIBUTES sa{};
    PSECURITY_DESCRIPTOR pSD = nullptr;

    ConvertStringSecurityDescriptorToSecurityDescriptorW(
        L"D:(A;;GA;;;SY)(A;;GA;;;BA)(A;;GRGW;;;IU)", SDDL_REVISION_1, &pSD,
        nullptr);

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;

    HANDLE hPipe =
        CreateNamedPipeW(PIPE_NAME, PIPE_ACCESS_DUPLEX,
                         PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                         PIPE_UNLIMITED_INSTANCES, 4096, 4096, 0, &sa);

    LocalFree(pSD);

    if (hPipe == INVALID_HANDLE_VALUE) return;

    BOOL connected = ConnectNamedPipe(hPipe, nullptr)
                         ? TRUE
                         : (GetLastError() == ERROR_PIPE_CONNECTED);

    if (connected) {
      std::thread(handleClient, hPipe).detach();
    } else {
      CloseHandle(hPipe);
    }
  }
}

void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
  if (ctrlCode == SERVICE_CONTROL_STOP) {
    g_Running = false;
    SetEvent(g_StopEvent);
    g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
  }
}

void WINAPI ServiceMain(DWORD, LPWSTR *) {
  g_StatusHandle =
      RegisterServiceCtrlHandlerW(L"BatLauncherService", ServiceCtrlHandler);

  g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  g_ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
  g_ServiceStatus.dwWaitHint = 10000;
  g_ServiceStatus.dwCheckPoint = 1;
  SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

  g_StopEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

  std::thread(pipeServer).detach();

  g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
  g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
  SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

  WaitForSingleObject(g_StopEvent, INFINITE);

  g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
  SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

int wmain() {
  SERVICE_TABLE_ENTRYW ServiceTable[] = {
      {(LPWSTR)L"BatLauncherService", ServiceMain}, {nullptr, nullptr}};

  StartServiceCtrlDispatcherW(ServiceTable);

  return 0;
}
