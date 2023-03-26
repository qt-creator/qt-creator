// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

/*
 Follow includes
 */
#include <QDebug>
#include <QString>
#include <iostream>
#include <vector>
#include <cstdio>
//#include <windows.h>
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
