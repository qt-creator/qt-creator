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

#ifndef CALLGRINDPARSERTESTS_H
#define CALLGRINDPARSERTESTS_H

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

#endif // CALLGRINDPARSERTESTS_H
