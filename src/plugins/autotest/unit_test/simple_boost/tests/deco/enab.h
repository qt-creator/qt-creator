#pragma once
#include <boost/test/included/unit_test.hpp>

namespace utf = boost::unit_test;

BOOST_AUTO_TEST_SUITE(Suite1, * utf::disabled())
    BOOST_AUTO_TEST_CASE(test1)
    {
      BOOST_TEST(1 != 1);
    }
    BOOST_AUTO_TEST_CASE(Test2, * utf::enabled())
    {
      BOOST_TEST(2 != 2);
    }
    BOOST_AUTO_TEST_CASE(TestIo, * utf::enable_if<true>())
    {
      BOOST_TEST(3 != 3);
    }
    BOOST_AUTO_TEST_CASE(TestDb, * utf::enable_if<false>())
    {
      BOOST_TEST(4 != 4);
    }
BOOST_AUTO_TEST_SUITE_END()
