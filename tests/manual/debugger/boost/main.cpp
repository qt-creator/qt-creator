/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
