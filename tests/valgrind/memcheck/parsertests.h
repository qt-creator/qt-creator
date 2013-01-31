/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef PARSERTESTS_H
#define PARSERTESTS_H

#include <QObject>
#include <QPair>
#include <QStringList>
#include <QVector>
#include <QDebug>

#include <valgrind/xmlprotocol/error.h>
#include <valgrind/xmlprotocol/status.h>
#include <valgrind/xmlprotocol/threadedparser.h>
#include <valgrind/xmlprotocol/parser.h>
#include <valgrind/memcheck/memcheckrunner.h>

QT_BEGIN_NAMESPACE
class QTcpServer;
class QTcpSocket;
class QProcess;
QT_END_NAMESPACE

void dumpError(const Valgrind::XmlProtocol::Error &e);

class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(Valgrind::XmlProtocol::Parser *parser, QObject *parent = 0)
    : QObject(parent)
    {
        connect(parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
                this, SLOT(error(Valgrind::XmlProtocol::Error)));
        connect(parser, SIGNAL(errorCount(qint64, qint64)),
                this, SLOT(errorCount(qint64, qint64)));
        connect(parser, SIGNAL(suppressionCount(QString, qint64)),
                this, SLOT(suppressionCount(QString, qint64)));
    }

    QList<Valgrind::XmlProtocol::Error> errors;
    QVector<QPair<qint64,qint64> > errorcounts;
    QVector<QPair<QString,qint64> > suppcounts;

public Q_SLOTS:
    void error(const Valgrind::XmlProtocol::Error &err)
    {
        errors.append(err);
    }

    void errorCount(qint64 uniq, qint64 count)
    {
        errorcounts.push_back(qMakePair(uniq, count));
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
    explicit RunnerDumper(Valgrind::Memcheck::MemcheckRunner *runner, Valgrind::XmlProtocol::ThreadedParser *parser)
        : QObject()
        , m_errorReceived(false)
    {
        connect(parser, SIGNAL(error(Valgrind::XmlProtocol::Error)),
                this, SLOT(error(Valgrind::XmlProtocol::Error)));
        connect(parser, SIGNAL(internalError(QString)),
                this, SLOT(internalError(QString)));
        connect(parser, SIGNAL(status(Valgrind::XmlProtocol::Status)),
                this, SLOT(status(Valgrind::XmlProtocol::Status)));
        connect(runner, SIGNAL(standardErrorReceived(QByteArray)),
                this, SLOT(standardErrorReceived(QByteArray)));
        connect(runner, SIGNAL(standardOutputReceived(QByteArray)),
                this, SLOT(standardOutputReceived(QByteArray)));
        connect(runner, SIGNAL(logMessageReceived(QByteArray)),
                this, SLOT(logMessageReceived(QByteArray)));
        connect(runner, SIGNAL(processErrorReceived(QString, QProcess::ProcessError)),
                this, SLOT(processErrorReceived(QString)));
    }

public slots:
    void error(const Valgrind::XmlProtocol::Error &e)
    {
        qDebug() << "error received";
        dumpError(e);
    }
    void internalError(const QString& error)
    {
        qDebug() << "internal error received:" << error;
    }
    void standardErrorReceived(const QByteArray &err)
    {
        Q_UNUSED(err);
        // qDebug() << "STDERR received:" << err; // this can be a lot of text
    }
    void standardOutputReceived(const QByteArray &out)
    {
        qDebug() << "STDOUT received:" << out;
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
    bool m_errorReceived;

};

class ParserTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
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
    void initTest(const QLatin1String &testfile, const QStringList &otherArgs = QStringList());

    QTcpServer *m_server;
    QProcess *m_process;
    QTcpSocket *m_socket;
};

#endif // PARSERTESTS_H
