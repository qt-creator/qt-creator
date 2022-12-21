// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

class Foo : public QObject
{
    Q_OBJECT

public:
    Foo();
    ~Foo();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void test_case1();
    void test_case1_data();
    void test_caseZero();
    void test_case2();
//    void test_case3() {}
    void test_case4();
    void test_case5() {}
};
