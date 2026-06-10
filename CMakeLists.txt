cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 14)
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("BatLauncherWinService")

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_executable (BatLauncherWinService "BatLauncherWinService.cpp" "BatLauncherWinService.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET BatLauncherWinService PROPERTY CXX_STANDARD 20)
endif()

