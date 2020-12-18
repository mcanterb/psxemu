#include "catch.hpp"
#include "fff.h"
extern "C" {

#include <PsxCoreFoundation/Base.h>
#include <PsxCoreFoundation/String.h>
}

PCFStringRef internedString() { return PCFCSTR("Interned String"); }

TEST_CASE("PCFString Tests", "[PsxCoreFoundation/String]") {
  SECTION("String Interning Works") {
    PCFCSTR("Intern Some Strings");
    REQUIRE(internedString() == internedString());
  }
}
