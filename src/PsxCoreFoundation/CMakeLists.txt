# Define library. Only source files here!
project(PsxCoreFoundation VERSION 0.1 LANGUAGES C CXX)

add_library(PsxCoreFoundation INTERFACE)
target_sources(PsxCoreFoundation 
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Base.c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Data.c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Exception.c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/String.c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/StringPrintf.c>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/utf8.c>
)
# Define headers for this library. PUBLIC headers are used for
# compiling the library, and will be added to consumers' build
# paths.
target_include_directories(PsxCoreFoundation INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    INTERFACE $<INSTALL_INTERFACE:include>)


# Every library has unit tests, of course
add_executable(testPsxCoreFoundation
    "tests/Data.cpp"
     "include/PsxCoreFoundation/Types.h" "tests/String.cpp")
target_compile_definitions(testPsxCoreFoundation PRIVATE TESTING=1)
target_link_libraries(testPsxCoreFoundation
    PsxCoreFoundation
    Catch2::Catch2)
include(CTest)
include(Catch)
catch_discover_tests(testPsxCoreFoundation)
add_test(testPsxCoreFoundation testPsxCoreFoundation)

# 'make install' to the correct locations (provided by GNUInstallDirs).
install(TARGETS PsxCoreFoundation EXPORT PsxCoreFoundationConfig
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# This makes the project importable from the install directory
# Put config file in per-project dir (name MUST match), can also
# just go into 'cmake'.
install(EXPORT PsxCoreFoundationConfig DESTINATION share/PsxCoreFoundation/cmake)

# This makes the project importable from the build directory
export(TARGETS PsxCoreFoundation FILE PsxCoreFoundationConfig.cmake)
