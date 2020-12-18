#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "fff.h"
extern "C" {

#include <PsxCoreFoundation/Base.h>
#include <PsxCoreFoundation/Data.h>
}
DEFINE_FFF_GLOBALS;

FAKE_VOID_FUNC1(Mock_PCFPanic, const char *);

TEST_CASE("PCFData Tests", "[PsxCoreFoundation/Data]") {
  test_PCFPanic = Mock_PCFPanic;
  RESET_FAKE(Mock_PCFPanic);
  size_t capacity = 10;
  auto data = PCFDataNew(capacity);
  REQUIRE(data != NULL);

  SECTION("Get/Set") {
    PCFDataSetByte(data, 5, 10);
    REQUIRE(PCFDataGetByte(data, 5) == 10);
    REQUIRE(Mock_PCFPanic_fake.call_count == 0);
  }

  SECTION("Get beyond range panics") {
    PCFDataGetByte(data, capacity + 1);
    REQUIRE(Mock_PCFPanic_fake.call_count == 1);
  }

  SECTION("Set beyond range panics") {
    PCFDataSetByte(data, capacity + 1, 0xAB);
    REQUIRE(Mock_PCFPanic_fake.call_count == 1);
  }

  SECTION("Verify Assert") {
    assert(0 == 1);
    REQUIRE(Mock_PCFPanic_fake.call_count == 1);
  }

  PCFRelease(data);
}
