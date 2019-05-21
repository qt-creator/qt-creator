#include <boost/test/included/unit_test.hpp>
#include <boost/test/parameterized_test.hpp>
#include <boost/bind.hpp>

using namespace boost::unit_test;

class TestClass
{
public:
    void testMethod()
    {
        BOOST_TEST( true );
    }
};

void freeTestFunction()
{
    BOOST_TEST( true );
}

void freeTestFunction2(int i)
{
    BOOST_TEST( i < 4 );
}

test_suite* init_unit_test_suite( int /*argc*/, char* /*argv*/[] )
{
    boost::shared_ptr<TestClass> tester( new TestClass );

    framework::master_test_suite().
        add( BOOST_TEST_CASE( boost::bind( &TestClass::testMethod, tester )));

    framework::master_test_suite().
        add( BOOST_TEST_CASE( &freeTestFunction) );

    int params[] = {1, 2, 3, 4, 5, 6};
    framework::master_test_suite().
        add( BOOST_PARAM_TEST_CASE( &freeTestFunction2,  params, params + 6) );

    return nullptr;
}
