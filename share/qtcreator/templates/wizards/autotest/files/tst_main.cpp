@if  "%{TestFrameWork}" == "QtQuickTest"
#include <QtQuickTest/quicktest.h>
@if "%{UseSetupCode}" === "true"
#include "setup.h"

QUICK_TEST_MAIN_WITH_SETUP(example, Setup)
@else

QUICK_TEST_MAIN(example)
@endif
@endif
@if "%{TestFrameWork}" == "GTest" || "%{TestFrameWork}" == "GTest_dyn"
%{Cpp:LicenseTemplate}\

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
@endif
@if "%{TestFrameWork}" == "BoostTest"
#define BOOST_TEST_MODULE My test module
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( %{TestSuiteName} )

BOOST_AUTO_TEST_CASE( %{TestCaseName} )
{
  BOOST_TEST( true /* test assertion */ );
}

BOOST_AUTO_TEST_SUITE_END()
@endif
@if "%{TestFrameWork}" == "BoostTest_dyn"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE My test module
#include <boost/test/unit_test.hpp>
@endif
@if "%{TestFrameWork}" == "Catch2"
@if "%{Catch2NeedsQt}" == "true"
#define CATCH_CONFIG_RUNNER
@else
#define CATCH_CONFIG_MAIN
@endif
#include <catch2/catch.hpp>
@if "%{Catch2NeedsQt}" == "true"
#include <QtGui/QGuiApplication>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    return Catch::Session().run(argc, argv);
}
@endif
@endif
@if "%{TestFrameWork}" == "Catch2_dyn" && "%{Catch2Main}" == "true"
#include <catch2/catch_session.hpp>
@if "%{Catch2NeedsQt}" == "true"
#include <QtGui/QGuiApplication>
@endif

int main( int argc, char* argv[] ) {
    // your setup ...
@if "%{Catch2NeedsQt}" == "true"
    QGuiApplication app(argc, argv);
@endif

  int result = Catch::Session().run( argc, argv );

  // your clean-up...

  return result;
}
@endif
