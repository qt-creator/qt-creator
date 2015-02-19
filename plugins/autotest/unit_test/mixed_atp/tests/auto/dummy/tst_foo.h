/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd
** All rights reserved.
** For any questions to The Qt Company, please use contact form at
** http://www.qt.io/contact-us
**
** This file is part of the Qt Creator Enterprise Auto Test Add-on.
**
** Licensees holding valid Qt Enterprise licenses may use this file in
** accordance with the Qt Enterprise License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.
**
** If you have questions regarding the use of this file, please use
** contact form at http://www.qt.io/contact-us
**
****************************************************************************/
#ifndef FOO_H_INCLUDED
#define FOO_H_INCLUDED

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

#endif
