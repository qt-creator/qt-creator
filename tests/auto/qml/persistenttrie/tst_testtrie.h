// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>
#include <QStringList>
#include <QtTest>

class tst_TestTrie : public QObject
{
    Q_OBJECT
public:
    tst_TestTrie();

private slots:
    void initTestCase();
    //void cleanupTestCase();
    //void init();
    //void cleanup();

    void testListAll_data();
    void testMerge_data();
    void testIntersect_data();
    void testCompletion_data();

    void testListAll();
    void testMerge();
    void testIntersect();
    void testCompletion();
};
