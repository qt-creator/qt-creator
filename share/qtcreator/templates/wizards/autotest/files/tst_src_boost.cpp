%{Cpp:LicenseTemplate}\
@if "%{TestFrameWork}" == "BoostTest_dyn"
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE( %{TestSuiteName} )

BOOST_AUTO_TEST_CASE( %{TestCaseName} )
{
  BOOST_TEST( true /* test assertion */ );
}

BOOST_AUTO_TEST_SUITE_END()
