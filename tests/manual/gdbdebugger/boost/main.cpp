
#include <boost/optional.hpp>

using namespace boost;

struct Large
{
    Large() { d1 = d2 = d3 = d4 = d5 = 0; }

    double d1;
    double d2;
    double d3;
    double d4;
    double d5;
};

void testOptional()
{
    optional<int> i;
    optional<double> d;
    optional<Large> l;
    i = 1;
    l = Large();
    i = 2;
    i = 3;
}

int main()
{
    testOptional();
}
