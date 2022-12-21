// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QDebug>

class CallgrindParserTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanup();

    void testHeaderData();
    void testSimpleFunction();
    void testCallee();
    void testInlinedCalls();

    void testMultiCost();
    void testMultiPos();
    void testMultiPosAndCost();

    void testCycle();
    void testRecursiveCycle();

    void testRecursion();
};
