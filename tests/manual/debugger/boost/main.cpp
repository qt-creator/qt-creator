// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <boost/optional.hpp>
#include <string>

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
    optional<std::string &> sr;
    optional<std::string> s;
    std::string as = "hallo";
    i = 1;
    sr = as;
    s = as;
    l = Large();
    i = 2;
    i = 3;
}

int main()
{
    testOptional();
}
