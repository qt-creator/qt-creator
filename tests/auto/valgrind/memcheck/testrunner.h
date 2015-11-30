/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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
