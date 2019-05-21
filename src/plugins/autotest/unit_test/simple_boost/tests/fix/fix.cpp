#define BOOST_TEST_MODULE fixture example
#include <boost/test/included/unit_test.hpp>

struct F {
  F() : i( 0 ) { BOOST_TEST_MESSAGE( "setup fixture" ); }
  ~F()         { BOOST_TEST_MESSAGE( "teardown fixture" ); }
  int i;
};

BOOST_FIXTURE_TEST_CASE( test_case1, F )
{
  BOOST_TEST( i++ == 1 );
}

BOOST_FIXTURE_TEST_CASE( test_case2, F, * boost::unit_test::disabled() )
{
  BOOST_CHECK_EQUAL( i, 1 );
}
