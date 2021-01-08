#include "catch.hpp"
extern "C" {

#include "TestSystem.hpp"
}

TEST_CASE("CpuTests", "[Types]") {

  SECTION("C0P0 SR parser is sane") {
    CpuCop0 cop0;
    cop0.sr.value = 0b00000000000000000000000000000001;
    REQUIRE(cop0.sr.parsed.currentInteruptEnable);
    cop0.sr.value = 0b00000000000000000000000000000010;
    REQUIRE(!cop0.sr.parsed.currentInteruptEnable);
    REQUIRE(cop0.sr.parsed.currentUserMode);
    cop0.sr.value = 0b00000000000000000000000000000100;
    REQUIRE(cop0.sr.parsed.prevInteruptEnable);
    cop0.sr.value = 0b00000000000000000000000000001000;
    REQUIRE(cop0.sr.parsed.prevUserMode);
    cop0.sr.value = 0b00000000000000000000000000010000;
    REQUIRE(cop0.sr.parsed.oldInteruptEnable);
    cop0.sr.value = 0b00000000000000000000000000100000;
    REQUIRE(cop0.sr.parsed.oldUserMode);

    cop0.sr.value = 0x100;
    REQUIRE(cop0.sr.parsed.interruptMasks == 1);
    cop0.sr.value = 0xFF00;
    REQUIRE(cop0.sr.parsed.interruptMasks == 0xFF);

    cop0.sr.value = 0b00000000000000010000000000000000;
    REQUIRE(cop0.sr.parsed.cacheIsolated);
    cop0.sr.value = 0b00000000000000100000000000000000;
    REQUIRE(cop0.sr.parsed.swapCaches);
    cop0.sr.value = 0b00000000000001000000000000000000;
    REQUIRE(cop0.sr.parsed.writeParity0);
    cop0.sr.value = 0b00000000000010000000000000000000;
    REQUIRE(cop0.sr.parsed.dataCacheHit);
    cop0.sr.value = 0b00000000000100000000000000000000;
    REQUIRE(cop0.sr.parsed.cacheParityError);
    cop0.sr.value = 0b00000000001000000000000000000000;
    REQUIRE(cop0.sr.parsed.tlbShutdown);
    cop0.sr.value = 0b00000000010000000000000000000000;
    REQUIRE(cop0.sr.parsed.bootExceptionVectors);

    cop0.sr.value = 0b00010000000000000000000000000000;
    REQUIRE(cop0.sr.parsed.cop0Enable);
    cop0.sr.value = 0b00100000000100000000000000000000;
    REQUIRE(cop0.sr.parsed.cop1Enable);
    cop0.sr.value = 0b01000000001000000000000000000000;
    REQUIRE(cop0.sr.parsed.cop2Enable);
    cop0.sr.value = 0b10000000010000000000000000000000;
    REQUIRE(cop0.sr.parsed.cop3Enable);
  }
}
