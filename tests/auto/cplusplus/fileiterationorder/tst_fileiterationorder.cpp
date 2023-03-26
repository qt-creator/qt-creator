// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <cppeditor/cppfileiterationorder.h>

#include <QtTest>

using namespace CppEditor;

//TESTED_COMPONENT=src/plugins/cppeditor

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

QTEST_GUILESS_MAIN(tst_FileIterationOrder)
#include "tst_fileiterationorder.moc"
