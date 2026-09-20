cmake_minimum_required(VERSION 3.20)
project(myseqserver CXX)

if (NOT WIN32)
    message(FATAL_ERROR "myseqserver is Windows-only (winsock2, tlhelp32, Win32 GUI APIs).")
endif()

if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "myseqserver only supports 64-bit builds. Configure with a 64-bit generator/toolchain (e.g. -A x64, or the CLion x64 Visual Studio toolchain).")
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions($<$<CONFIG:Debug>:_DEBUG> $<$<CONFIG:Release>:NDEBUG>)

# --- MySEQ.server: the GUI/tray application ---------------------------------
add_executable(MySEQ.server WIN32
    MySEQ.server.cpp
    NetworkServer.cpp
    MemReader.cpp
    EQGameScanner.cpp
    Debugger.cpp
    Spawn.cpp
    Item.cpp
    World.cpp
    IniReader.cpp
    stdafx.cpp
    MySEQ.server.rc
)
target_compile_definitions(MySEQ.server PRIVATE WIN32 _WINDOWS)
target_link_libraries(MySEQ.server PRIVATE wsock32 iphlpapi ws2_32 psapi kernel32)
target_precompile_headers(MySEQ.server PRIVATE stdafx.h)

# --- myseqserverc: the single-file console build -----------------------------
add_executable(myseqserverc
    myseqserverc.cpp
)
target_compile_definitions(myseqserverc PRIVATE WIN32 _CONSOLE)
target_link_libraries(myseqserverc PRIVATE ws2_32)
