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

#ifndef TESTRUNNER_H
#define TESTRUNNER_H

#include <QObject>
#include <QStringList>

#include <valgrind/xmlprotocol/error.h>

namespace Valgrind {

namespace XmlProtocol {
class ThreadedParser;
}

namespace Memcheck {
class MemcheckRunner;
}

class TestRunner : public QObject
{
    Q_OBJECT

public:
    explicit TestRunner(QObject *parent = 0);

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testLeak1();
    void testLeak2();
    void testLeak3();
    void testLeak4();

    void uninit1();
    void uninit2();
    void uninit3();

    void free1();
    void free2();

    void invalidjump();
    void syscall();
    void overlap();

private Q_SLOTS:
    void logMessageReceived(const QByteArray &message);
    void internalError(const QString &error);
    void error(const Valgrind::XmlProtocol::Error &error);

private:
    QString runTestBinary(const QString &binary, const QStringList &vArgs = QStringList());

    XmlProtocol::ThreadedParser *m_parser;
    Memcheck::MemcheckRunner *m_runner;
    QList<QByteArray> m_logMessages;
    QList<XmlProtocol::Error> m_errors;
    bool m_expectCrash;
};

} // namespace Valgrind

#endif // TESTRUNNER_H
