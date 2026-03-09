#pragma once

#include <windows.h>
#include <sddl.h>
#include <shlobj_core.h>
#include <tlhelp32.h>
#include <string>
#include <thread>
#include <atomic>
#include <vector>
#include <sstream>

SERVICE_STATUS g_ServiceStatus{};
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
HANDLE g_StopEvent = nullptr;
std::atomic<bool> g_Running{true};

#define PIPE_NAME L"\\\\.\\pipe\\BatLauncherServicePipe"