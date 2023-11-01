// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "xmlprotocol/error.h"

#include <QObject>
#include <QStringList>

namespace Valgrind { class ValgrindProcess; }

namespace Valgrind::Test {

class ValgrindTestRunnerTest : public QObject
{
    Q_OBJECT

public:
    explicit ValgrindTestRunnerTest(QObject *parent = nullptr);

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

    ValgrindProcess *m_runner = nullptr;
    QList<QByteArray> m_logMessages;
    QList<XmlProtocol::Error> m_errors;
    bool m_expectCrash = false;
};

} // namespace Valgrind::Test
