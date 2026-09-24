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
    src/MySEQ.server.cpp
    src/NetworkServer.cpp
    src/MemReader.cpp
    src/EQGameScanner.cpp
    src/Debugger.cpp
    src/Spawn.cpp
    src/Item.cpp
    src/World.cpp
    src/IniReader.cpp
    src/stdafx.cpp
    MySEQ.server.rc
)
target_compile_definitions(MySEQ.server PRIVATE WIN32 _WINDOWS)
target_compile_options(MySEQ.server PRIVATE /EHsc /W4 /analyze)
target_link_libraries(MySEQ.server PRIVATE wsock32 iphlpapi ws2_32 psapi kernel32)
target_include_directories(MySEQ.server PRIVATE include)
target_precompile_headers(MySEQ.server PRIVATE include/stdafx.h)

# Embed a manifest requesting themed (Common Controls v6) visual styles -
# without it, buttons/edit boxes render with the classic Windows 95 look.
if (MSVC)
    target_link_options(MySEQ.server PRIVATE /MANIFEST:NO)
endif()

# MySEQ.server looks for its .ini files next to the executable / in the
# working directory at runtime; copy them into the build output so a plain
# "Run" in the IDE works without manual setup.
add_custom_command(TARGET MySEQ.server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/resources/myseqserver.ini" "${CMAKE_SOURCE_DIR}/resources/config.ini"
        "$<TARGET_FILE_DIR:MySEQ.server>"
)

# --- OffsetScanningTests: offline smoke tests for the byte-pattern scanner --
# Exercises EQGameScanner::findEQPointerOffset/findEQAbsolutePointer and
# IniReader::readEscapeStrings against synthetic PE/ini files on disk - no
# real eqgame.exe needed. A plain console app with a process exit code
# (0 = all checks passed), so it's CI-runnable as-is.
add_executable(OffsetScanningTests
    tests/OffsetScanningTests.cpp
    src/EQGameScanner.cpp
    src/IniReader.cpp
    src/stdafx.cpp
)
target_compile_definitions(OffsetScanningTests PRIVATE WIN32 _WINDOWS)
target_compile_options(OffsetScanningTests PRIVATE /EHsc /W4 /analyze)
target_include_directories(OffsetScanningTests PRIVATE include)
target_precompile_headers(OffsetScanningTests PRIVATE include/stdafx.h)
