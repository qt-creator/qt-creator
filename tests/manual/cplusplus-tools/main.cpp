/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

/*
 Follow includes
 */
#include <QDebug>
#include <QString>
#include <iostream>
#include <vector>
#include <cstdio>
//#include <Windows.h>
//#include <linux/version.h>
#include "dummy.h"
#include "detail/header.h"

/*
 Complete includes
 */
//#include <QDe
//#include <QtCor
//#include <QtCore/QXmlStream

//#include <ios
//#include <vec
//#include <cstd

//#include <Win
//#include <lin

//#include "dum
//#include "deta
//#include "detail/hea


using namespace test;

int fi = 10;
extern int xi;
const int ci = 1;

namespace {
int ai = 100;
int afunc() {
    return fi * xi + ai + ci;
}
}

/*
 Follow symbols
    - Expect some issues when finding the best function overload and with templates.
    - Try using a local namespace directive instead of the global one.
 */
using namespace test;
void testFollowSymbols()
{
    //using namespace test;

    Dummy dummy;
    Dummy::sfunc();
    Dummy::ONE;
    Dummy::PI;
    dummy.bla(fi);
    dummy.bla("bla");
    dummy.one = "one";
    Dummy::Internal internal;
    internal.one = "one";
    Dummy::INT i;
    Dummy::Values V;
    Dummy::v1;
    freefunc1();
    freefunc2(10);
    freefunc2("s");
    freefunc3(dummy);
    freefunc3(dummy, 10);
    freefunc3(10, 10);
    freefunc3(1.0);
    afunc();
    i;
    V;
}

/*
 Complete symbols
    - Check function arguments.
 */
void testCompleteSymbols()
{
    test::Dummy dummy;
    test::Dummy::Internal internal;

//    in
//    Dum
//    Dummy::s
//    Dummy::O
//    Dummy::P
//    dummy.
//    dummy.b
//    dummy.bla(
//    dummy.o
//    Dummy::In
//    internal.o
//    Dummy::Internal::
//    freefunc2
//    using namespace st
//    afun
}

/*
 Complete snippets
 */
void testCompleteSnippets()
{
//    for
//    class
//    whil
}

/*
 Find usages
    - Go to other files for more options.
 */
void testFindUsages()
{
    Dummy();
    Dummy::sfunc();
    Dummy::ONE;
    xi;
    fi;
    ci;
    ai;
    afunc();
    freefunc1();
    freefunc2("s");
}

/*
 Rename
    - Compile to make sure.
    - Go to other files for more options.
 */
void testRename()
{
    fi;
    ci;
    ai;
    afunc();
    testCompleteSnippets();
}

/*
 Type hierarchy
 */
void testTypeHierarchy()
{
    test::GrandChildDummy();
    D();
}

/*
 Switch declaration/definition
    - Use functions from Dummy.
 */

/*
 Switch header/source
    - Use dummy.h and dummy.cpp.
 */

int main()
{
    return 0;
}
