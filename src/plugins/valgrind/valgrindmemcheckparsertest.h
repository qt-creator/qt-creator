// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/process.h>

#include <QStringList>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
class QTcpServer;
QT_END_NAMESPACE

namespace Valgrind::Test {

class ValgrindMemcheckParserTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void testMemcheckSample1();
    void testMemcheckSample2();
    void testMemcheckSample3();
    void testMemcheckCharm();
    void testHelgrindSample1();

    void testValgrindCrash();
    void testValgrindGarbage();

    void testParserStop();
    void testRealValgrind();
    void testValgrindStartError_data();
    void testValgrindStartError();

private:
    void initTest(const QString &testfile, const QStringList &otherArgs = {});

    QTcpServer *m_server = nullptr;
    std::unique_ptr<Utils::Process> m_process;
    std::unique_ptr<QTcpSocket> m_socket;
};

} // namespace Valgrind::Test
