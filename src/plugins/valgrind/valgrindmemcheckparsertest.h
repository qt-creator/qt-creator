/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
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
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QDebug>

#include "xmlprotocol/error.h"
#include "xmlprotocol/status.h"
#include "xmlprotocol/threadedparser.h"
#include "xmlprotocol/parser.h"
#include "valgrindrunner.h"

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QProcess;
QT_END_NAMESPACE

namespace Valgrind {
namespace Test {

void dumpError(const Valgrind::XmlProtocol::Error &e);

class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(XmlProtocol::Parser *parser)
    {
        connect(parser, &XmlProtocol::Parser::error,
                this, &Recorder::error);
        connect(parser, &XmlProtocol::Parser::errorCount,
                this, &Recorder::errorCount);
        connect(parser, &XmlProtocol::Parser::suppressionCount,
                this, &Recorder::suppressionCount);
    }

    QList<Valgrind::XmlProtocol::Error> errors;
    QVector<QPair<qint64,qint64> > errorcounts;
    QVector<QPair<QString,qint64> > suppcounts;

public:
    void error(const Valgrind::XmlProtocol::Error &err)
    {
        errors.append(err);
    }

    void errorCount(qint64 unique, qint64 count)
    {
        errorcounts.push_back(qMakePair(unique, count));
    }

    void suppressionCount(const QString &name, qint64 count)
    {
        suppcounts.push_back(qMakePair(name, count));
    }

};

class RunnerDumper : public QObject
{
    Q_OBJECT

public:
    explicit RunnerDumper(ValgrindRunner *runner)
    {
        connect(runner->parser(), &XmlProtocol::ThreadedParser::error,
                this, &RunnerDumper::error);
        connect(runner->parser(), &XmlProtocol::ThreadedParser::internalError,
                this, &RunnerDumper::internalError);
        connect(runner->parser(), &XmlProtocol::ThreadedParser::status,
                this, &RunnerDumper::status);
        connect(runner, &ValgrindRunner::logMessageReceived,
                this, &RunnerDumper::logMessageReceived);
        connect(runner, &ValgrindRunner::processErrorReceived,
                this, &RunnerDumper::processErrorReceived);
    }

    void error(const Valgrind::XmlProtocol::Error &e)
    {
        qDebug() << "error received";
        dumpError(e);
    }
    void internalError(const QString& error)
    {
        qDebug() << "internal error received:" << error;
    }
    void status(const Valgrind::XmlProtocol::Status &status)
    {
        qDebug() << "status received:" << status.state() << status.time();
    }
    void logMessageReceived(const QByteArray &log)
    {
        qDebug() << "log message received:" << log;
    }
    void processErrorReceived(const QString &s)
    {
        Q_UNUSED(s);
        // qDebug() << "error received:" << s; // this can be a lot of text
        m_errorReceived = true;
    }

public:
    bool m_errorReceived = false;
};

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
    void initTest(const QString &testfile, const QStringList &otherArgs = QStringList());

    QTcpServer *m_server = nullptr;
    QProcess *m_process = nullptr;
    QTcpSocket *m_socket = nullptr;
};

} // namespace Test
} // namespace Valgrind
