@if "%{TestFrameWork}" == "Catch2"
#include <catch2/catch.hpp>
@else
#include <catch2/catch_test_macros.hpp>
@endif

TEST_CASE("My first test with Catch2", "[fancy]")
{
    REQUIRE(0 == 0);
}
