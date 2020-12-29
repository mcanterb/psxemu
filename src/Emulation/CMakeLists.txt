# Define an executable
add_executable(psxemu
    "src/Bios.c"
    src/Bus.c
    "src/Cpu/Cpu.c"
    src/System.c
    src/psxemu.c 
    src/Memory.c 
    src/Devices.c 
    "src/Gpu.c" "src/Exceptions.c" "src/Interrupts.c" "src/Types.c" "src/Clock.c")

# Define the libraries this project depends upon
target_link_libraries(psxemu
    PsxCoreFoundation
    SDL2::SDL2
    SDL2::SDL2main)

# Every library has unit tests, of course
add_executable(testPsxemu
    "tests/tests.cpp"
    "src/Bios.c"
    src/Bus.c
    "src/Cpu/Cpu.c"
    src/System.c
    src/Memory.c 
    src/Devices.c 
    "src/Gpu.c" "src/Exceptions.c" "src/Interrupts.c" "src/Types.c" "src/Clock.c")

target_compile_definitions(testPsxemu PRIVATE TESTING=1)
target_link_libraries(testPsxemu
    PsxCoreFoundation
    SDL2::SDL2
    Catch2::Catch2)
include(CTest)
include(Catch)
catch_discover_tests(testPsxemu)
add_test(testPsxemu testPsxemu)