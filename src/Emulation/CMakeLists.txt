# Define an executable
add_executable(psxemu
    src/Bios.c
    src/Bus.c
    src/Cpu.c
    src/Cpu.h
    src/System.c
    src/psxemu.c 
    src/Memory.c 
    src/Memory.h 
    src/Bus.h 
    src/Bios.h 
    src/Devices.h 
    src/Devices.c 
    src/Exceptions.h 
    src/Types.h "src/System.h"  "src/Interrupts.h" "src/Gpu.h" "src/Gpu.c" "src/Exceptions.c" "src/Interrupts.c" "src/Types.c")

# Define the libraries this project depends upon
target_link_libraries(psxemu
    PsxCoreFoundation
    SDL2::SDL2
    SDL2::SDL2main)

# Every library has unit tests, of course
add_executable(testPsxemu
    "tests/tests.cpp"
    src/Bios.c
    src/Bus.c
    src/Cpu.c
    src/Cpu.h
    src/System.c
    src/Memory.c 
    src/Memory.h 
    src/Bus.h 
    src/Bios.h 
    src/Devices.h 
    src/Devices.c 
    src/Exceptions.h 
    src/Types.h "src/System.h"  "src/Interrupts.h" "src/Gpu.h" "src/Gpu.c" "src/Exceptions.c" "src/Interrupts.c" "src/Types.c")

target_compile_definitions(testPsxemu PRIVATE TESTING=1)
target_link_libraries(testPsxemu
    PsxCoreFoundation
    SDL2::SDL2
    Catch2::Catch2)
include(CTest)
include(Catch)
catch_discover_tests(testPsxemu)
add_test(testPsxemu testPsxemu)