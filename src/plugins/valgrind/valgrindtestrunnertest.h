/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QObject>
#include <QStringList>

#include "xmlprotocol/error.h"

namespace Valgrind {

namespace XmlProtocol {
class ThreadedParser;
}

namespace Memcheck {
class MemcheckRunner;
}

namespace Test {

class ValgrindTestRunnerTest : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindTestRunnerTest(QObject *parent = 0);

private slots:
    void init();
    void cleanup();

    void testLeak1();
    void testLeak2();
    void testLeak3();
    void testLeak4();

    void testUninit1();
    void testUninit2();
    void testUninit3();

    void testFree1();
    void testFree2();

    void testInvalidjump();
    void testSyscall();
    void testOverlap();

    void logMessageReceived(const QByteArray &message);
    void internalError(const QString &error);
    void error(const Valgrind::XmlProtocol::Error &error);

private:
    QString runTestBinary(const QString &binary, const QStringList &vArgs = QStringList());

    XmlProtocol::ThreadedParser *m_parser = 0;
    Memcheck::MemcheckRunner *m_runner = 0;
    QList<QByteArray> m_logMessages;
    QList<XmlProtocol::Error> m_errors;
    bool m_expectCrash = false;
};

} // namespace Test
} // namespace Valgrind
