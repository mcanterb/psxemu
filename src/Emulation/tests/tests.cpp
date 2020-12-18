#define CATCH_CONFIG_MAIN
#include "catch.hpp"
extern "C" {

#include "../src/Types.h"
}

TEST_CASE("Data Sanity Tests", "[Types]") {

  SECTION("GpuCommand parser is valid") {
    GpuCommand command = GpuPacketToCommand(0xAA000011);
    REQUIRE(command.parsed.command == 0xAA);
    REQUIRE(command.parsed.parameters == 0x11);
  }
}
