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

#include <cpptools/cppfileiterationorder.h>

#include <QtTest>

using namespace CppTools;

//TESTED_COMPONENT=src/plugins/cpptools

class tst_FileIterationOrder: public QObject
{
    Q_OBJECT

private slots:
    void preferWithProjectPart();
    void preferWithReferenceProjectPart();
    void preferWithCommonPrefixToReferenceProjectPart();
    void preferWithCommonPrefixToReferenceFilePath();
};

void tst_FileIterationOrder::preferWithProjectPart()
{
    FileIterationOrder order("b.h", "p1");

    order.insert("c.h", "p1");
    order.insert("a.h");

    const QStringList expected = {
        "c.h",
        "a.h",
    };
    QCOMPARE(order.toStringList(), expected);
}

void tst_FileIterationOrder::preferWithReferenceProjectPart()
{
    FileIterationOrder order("a.h", "/path/p1");

    order.insert("a.cpp", "/path/p2");
    order.insert("z.cpp", "/path/p1");

    const QStringList expected = {
        "z.cpp",
        "a.cpp",
    };
    QCOMPARE(order.toStringList(), expected);
}

void tst_FileIterationOrder::preferWithCommonPrefixToReferenceProjectPart()
{
    FileIterationOrder order("a.h", "/some/path/p1");

    order.insert("a.cpp", "/other/path/p3");
    order.insert("z.cpp", "/some/path/p2");

    const QStringList expected = {
        "z.cpp",
        "a.cpp",
    };
    QCOMPARE(order.toStringList(), expected);
}

void tst_FileIterationOrder::preferWithCommonPrefixToReferenceFilePath()
{
    FileIterationOrder order("b.h", QString());

    order.insert("a.h");
    order.insert("b.cpp");

    const QStringList expected = {
        "b.cpp",
        "a.h",
    };
    QCOMPARE(order.toStringList(), expected);
}

QTEST_MAIN(tst_FileIterationOrder)
#include "tst_fileiterationorder.moc"
