%{JS: Cpp.licenseTemplate()}
@if  "%{TestFrameWork}" == "GTest"
#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

using namespace testing;

TEST(%{TestSuiteName}, %{TestCaseName})
{
    EXPECT_EQ(1, 1);
    ASSERT_THAT(0, Eq(0));
}

@endif
@if  "%{TestFrameWork}" == "BoostTest"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
BOOST_AUTO_TEST_SUITE( %{TestSuiteName} )

BOOST_AUTO_TEST_CASE( %{TestCaseName} )
{
  BOOST_TEST( true /* test assertion */ );
}

BOOST_AUTO_TEST_SUITE_END()
@endif
@if "%{TestFrameWork}" == "Catch2"
@if "%{Catch2Version}" == "V2"
#include <catch2/catch.hpp>
@else
#include <catch2/catch_test_macros.hpp>
@endif

TEST_CASE("Another test with Catch2", "[fancy]")
{
    REQUIRE(0 == 0);
}

@endif
