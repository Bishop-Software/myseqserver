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
target_link_libraries(MySEQ.server PRIVATE wsock32 iphlpapi ws2_32 psapi kernel32)
target_include_directories(MySEQ.server PRIVATE include)
target_precompile_headers(MySEQ.server PRIVATE include/stdafx.h)

# --- myseqserverc: the single-file console build -----------------------------
add_executable(myseqserverc
    src/myseqserverc.cpp
)
target_compile_definitions(myseqserverc PRIVATE WIN32 _CONSOLE)
target_link_libraries(myseqserverc PRIVATE ws2_32)

# Both binaries look for their .ini files next to the executable / in the
# working directory at runtime; copy them into the build output so a plain
# "Run" in the IDE works without manual setup.
foreach(target MySEQ.server myseqserverc)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_SOURCE_DIR}/myseqserver.ini" "${CMAKE_SOURCE_DIR}/config.ini"
            "$<TARGET_FILE_DIR:${target}>"
    )
endforeach()
