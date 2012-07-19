/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
