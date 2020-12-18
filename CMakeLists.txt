# CMakeList.txt : CMake project for psxemu, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")
#enable_testing()
project ("psxemu")

# Must use GNUInstallDirs to install libraries into correct
# locations on all platforms.
include(GNUInstallDirs)

# Include Boost as an imported target
find_package(SDL2 CONFIG REQUIRED)
find_package(Catch2 REQUIRED)

# Targets that we develop here
enable_testing()
add_subdirectory("src/PsxCoreFoundation")
add_subdirectory("src/Emulation")



#find_package(SDL2 CONFIG REQUIRED)

# Add source to this project's executable.
#add_library (psxemu STATIC "PsxCoreFoundation/Core.c" "Emulation/System.c" "Emulation/Bus.c" "Emulation/Cpu.c" "Emulation/Bios.c" "PsxCoreFoundation/Data.c" "PsxCoreFoundation/Exception.c")
#set_target_properties(psxemu PROPERTIES PUBLIC_HEADER "PsxCoreFoundation/Data.h" FRAMEWORK True)
#add_executable(main "psxemu.c")

#set_target_properties(main LINK_FLAGS /SUBSYSTEM:WINDOWS)
#target_link_libraries(psxemu PRIVATE SDL2::SDL2)
#target_link_libraries(main PRIVATE psxemu SDL2::SDL2main)

# TODO: Add tests and install targets if needed.
#find_package(Catch2 REQUIRED)
#add_executable(tests "src/PsxCoreFoundation/tests/tests.cpp")
#target_link_libraries(tests PRIVATE psxemu Catch2::Catch2)
#include(CTest)
#include(Catch)
#catch_discover_tests(tests)