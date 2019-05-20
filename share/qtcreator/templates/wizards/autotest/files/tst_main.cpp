@if  "%{TestFrameWork}" == "QtQuickTest"
#include <QtQuickTest/quicktest.h>

QUICK_TEST_MAIN(example)
@endif
@if "%{TestFrameWork}" == "GTest"
%{Cpp:LicenseTemplate}\
#include "%{TestCaseFileWithHeaderSuffix}"

#include <gtest/gtest.h>

int main(int argc, char *argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
@endif
@if "%{TestFrameWork}" == "BoostTest"
#define BOOST_TEST_MODULE %{TestSuiteName}
#include <boost/test/included/unit_test.hpp>

BOOST_AUTO_TEST_CASE( %{TestCaseName} )
{
  BOOST_TEST( true /* test assertion */ );
}
@endif
