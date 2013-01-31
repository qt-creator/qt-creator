/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "qmlprofilerapplication.h"
#include "constants.h"
#include <utils/qtcassert.h>
#include <QStringList>
#include <QTextStream>
#include <QProcess>
#include <QTimer>
#include <QDateTime>
#include <QFileInfo>
#include <QDebug>

using namespace QmlJsDebugClient;

static const char usageTextC[] =
"Usage:\n"
"    qmlprofiler [options] [program] [program-options]\n"
"    qmlprofiler [options] -attach [hostname]\n"
"\n"
"QML Profiler retrieves QML tracing data from a running application.\n"
"The data collected can then be visualized in Qt Creator.\n"
"\n"
"The application to be profiled has to enable QML debugging. See the Qt Creator\n"
"documentation on how to do this for different Qt versions.\n"
"\n"
"Options:\n"
"    -help  Show this information and exit.\n"
"    -fromStart\n"
"           Record as soon as the engine is started, default is false.\n"
"    -p <number>, -port <number>\n"
"           TCP/IP port to use, default is 3768.\n"
"    -v, -verbose\n"
"           Print debugging output.\n"
"    -version\n"
"           Show the version of qmlprofiler and exit.\n";

static const char commandTextC[] =
"Commands:\n"
"    r, record\n"
"           Switch recording on or off.\n"
"    q, quit\n"
"           Terminate program.";

static const char TraceFileExtension[] = ".qtd";

QmlProfilerApplication::QmlProfilerApplication(int &argc, char **argv) :
    QCoreApplication(argc, argv),
    m_runMode(LaunchMode),
    m_process(0),
    m_tracePrefix(QLatin1String("trace")),
    m_hostName(QLatin1String("127.0.0.1")),
    m_port(3768),
    m_verbose(false),
    m_quitAfterSave(false),
    m_qmlProfilerClient(&m_connection),
    m_v8profilerClient(&m_connection),
    m_connectionAttempts(0)
{
    m_connectTimer.setInterval(1000);
    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(tryToConnect()));

    connect(&m_connection, SIGNAL(connected()), this, SLOT(connected()));
    connect(&m_connection, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(connectionStateChanged(QAbstractSocket::SocketState)));
    connect(&m_connection, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));

    connect(&m_qmlProfilerClient, SIGNAL(enabledChanged()), this, SLOT(traceClientEnabled()));
    connect(&m_qmlProfilerClient, SIGNAL(recordingChanged(bool)), this, SLOT(recordingChanged()));
    connect(&m_qmlProfilerClient, SIGNAL(range(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)), &m_eventList, SLOT(addRangedEvent(int,qint64,qint64,QStringList,QmlJsDebugClient::QmlEventLocation)));
    connect(&m_qmlProfilerClient, SIGNAL(complete()), this, SLOT(qmlComplete()));

    connect(&m_v8profilerClient, SIGNAL(enabledChanged()), this, SLOT(profilerClientEnabled()));
    connect(&m_v8profilerClient, SIGNAL(v8range(int,QString,QString,int,double,double)), &m_eventList, SLOT(addV8Event(int,QString,QString,int,double,double)));
    connect(&m_v8profilerClient, SIGNAL(complete()), this, SLOT(v8Complete()));

    connect(&m_eventList, SIGNAL(error(QString)), this, SLOT(logError(QString)));
    connect(&m_eventList, SIGNAL(dataReady()), this, SLOT(traceFinished()));
}

QmlProfilerApplication::~QmlProfilerApplication()
{
    if (!m_process)
        return;
    logStatus("Terminating process...");
    m_process->disconnect();
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        logStatus("Killing process...");
        m_process->kill();
    }
    delete m_process;
}

bool QmlProfilerApplication::parseArguments()
{
    for (int argPos = 1; argPos < arguments().size(); ++argPos) {
        const QString arg = arguments().at(argPos);
        if (arg == QLatin1String("-attach") || arg == QLatin1String("-a")) {
            if (argPos + 1 == arguments().size())
                return false;
            m_hostName = arguments().at(++argPos);
            m_runMode = AttachMode;
        } else if (arg == QLatin1String("-port") || arg == QLatin1String("-p")) {
            if (argPos + 1 == arguments().size())
                return false;
            const QString portStr = arguments().at(++argPos);
            bool isNumber;
            m_port = portStr.toUShort(&isNumber);
            if (!isNumber) {
                logError(QString("'%1' is not a valid port").arg(portStr));
                return false;
            }
        } else if (arg == QLatin1String("-fromStart")) {
            m_qmlProfilerClient.setRecording(true);
            m_v8profilerClient.setRecording(true);
        } else if (arg == QLatin1String("-help") || arg == QLatin1String("-h") || arg == QLatin1String("/h") || arg == QLatin1String("/?")) {
            return false;
        } else if (arg == QLatin1String("-verbose") || arg == QLatin1String("-v")) {
            m_verbose = true;
        } else if (arg == QLatin1String("-version")) {
            print(QString("QML Profiler based on Qt %1.").arg(qVersion()));
            ::exit(1);
            return false;
        } else {
            if (m_programPath.isEmpty()) {
                m_programPath = arg;
                m_tracePrefix = QFileInfo(m_programPath).fileName();
            } else {
                m_programArguments << arg;
            }
        }
    }

    if (m_runMode == LaunchMode
            && m_programPath.isEmpty())
        return false;

    if (m_runMode == AttachMode
            && !m_programPath.isEmpty())
        return false;

    return true;
}

void QmlProfilerApplication::printUsage()
{
    print(QLatin1String(usageTextC));
    print(QLatin1String(commandTextC));
}

int QmlProfilerApplication::exec()
{
    QTimer::singleShot(0, this, SLOT(run()));
    return QCoreApplication::exec();
}

void QmlProfilerApplication::printCommands()
{
    print(QLatin1String(commandTextC));
}

QString QmlProfilerApplication::traceFileName() const
{
    QString fileName = m_tracePrefix + QLatin1Char('_') +
            QDateTime::currentDateTime().toString(QLatin1String("yyMMdd_hhmmss")) + TraceFileExtension;
    if (QFileInfo(fileName).exists()) {
        QString baseName;
        int suffixIndex = 0;
        do {
            baseName = QFileInfo(fileName).baseName()
                    + QString::number(suffixIndex++);
        } while (QFileInfo(baseName + TraceFileExtension).exists());
        fileName = baseName + TraceFileExtension;
    }

    return QFileInfo(fileName).absoluteFilePath();
}

void QmlProfilerApplication::userCommand(const QString &command)
{
    QString cmd = command.trimmed();
    if (cmd == Constants::CMD_HELP
            || cmd == Constants::CMD_HELP2
            || cmd == Constants::CMD_HELP3) {
        printCommands();
    } else if (cmd == Constants::CMD_RECORD
               || cmd == Constants::CMD_RECORD2) {
        m_qmlProfilerClient.setRecording(!m_qmlProfilerClient.isRecording());
        m_v8profilerClient.setRecording(!m_v8profilerClient.isRecording());
        m_qmlDataReady = false;
        m_v8DataReady = false;
    } else if (cmd == Constants::CMD_QUIT
               || cmd == Constants::CMD_QUIT2) {
        print(QLatin1String("Quit"));
        if (m_qmlProfilerClient.isRecording()) {
            m_quitAfterSave = true;
            m_qmlDataReady = false;
            m_v8DataReady = false;
            m_qmlProfilerClient.setRecording(false);
            m_v8profilerClient.setRecording(false);
        } else {
            quit();
        }
    }
}

void QmlProfilerApplication::run()
{
    if (m_runMode == LaunchMode) {
        m_process = new QProcess(this);
        QStringList arguments;
        arguments << QString::fromLatin1("-qmljsdebugger=port:%1,block").arg(m_port);
        arguments << m_programArguments;

        m_process->setProcessChannelMode(QProcess::MergedChannels);
        connect(m_process, SIGNAL(readyRead()), this, SLOT(processHasOutput()));
        connect(m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(processFinished()));
        logStatus(QString("Starting '%1 %2'...").arg(m_programPath, arguments.join(" ")));
        m_process->start(m_programPath, arguments);
        if (!m_process->waitForStarted()) {
            logError(QString("Could not run '%1': %2").arg(m_programPath, m_process->errorString()));
            exit(1);
        }

    }
    m_connectTimer.start();
}

void QmlProfilerApplication::tryToConnect()
{
    Q_ASSERT(!m_connection.isConnected());
    ++ m_connectionAttempts;

    if (!m_verbose && !(m_connectionAttempts % 5)) {// print every 5 seconds
        if (!m_verbose)
            logError(QString("Could not connect to %1:%2 for %3 seconds...").arg(
                         m_hostName, QString::number(m_port), QString::number(m_connectionAttempts)));
    }

    if (m_connection.state() == QAbstractSocket::UnconnectedState) {
        logStatus(QString("Connecting to %1:%2...").arg(m_hostName, QString::number(m_port)));
        m_connection.connectToHost(m_hostName, m_port);
    }
}

void QmlProfilerApplication::connected()
{
    m_connectTimer.stop();
    print(QString(QLatin1String("Connected to host:port %1:%2. Wait for profile data or type a command (type 'help'' to show list of commands).")).arg(m_hostName).arg((m_port)));
    QString recordingStatus(QLatin1String("Recording Status: %1"));
    if (!m_qmlProfilerClient.isRecording() && !m_v8profilerClient.isRecording())
        recordingStatus = recordingStatus.arg(QLatin1String("Off"));
    else
        recordingStatus = recordingStatus.arg(QLatin1String("On"));
    print(recordingStatus);
}

void QmlProfilerApplication::connectionStateChanged(QAbstractSocket::SocketState state)
{
    if (m_verbose)
        qDebug() << state;
}

void QmlProfilerApplication::connectionError(QAbstractSocket::SocketError error)
{
    if (m_verbose)
        qDebug() << error;
}

void QmlProfilerApplication::processHasOutput()
{
    QTC_ASSERT(m_process, return);
    while (m_process->bytesAvailable()) {
        QTextStream out(stdout);
        out << m_process->readAll();
    }
}

void QmlProfilerApplication::processFinished()
{
    QTC_ASSERT(m_process, return);
    if (m_process->exitStatus() == QProcess::NormalExit) {
        logStatus(QString("Process exited (%1).").arg(m_process->exitCode()));

        if (m_qmlProfilerClient.isRecording()) {
            logError("Process exited while recording, last trace is lost!");
            exit(2);
        } else {
            exit(0);
        }
    } else {
        logError("Process crashed! Exiting...");
        exit(3);
    }
}

void QmlProfilerApplication::traceClientEnabled()
{
    if (m_verbose)
        qDebug() << "Trace client is attached.";
    logStatus("Trace client is attached.");
    // blocked server is waiting for recording message from both clients
    // once the last one is connected, both messages should be sent
    m_qmlProfilerClient.sendRecordingStatus();
    m_v8profilerClient.sendRecordingStatus();
}

void QmlProfilerApplication::profilerClientEnabled()
{
    if (m_verbose)
        qDebug() << "Profiler client is attached.";
    logStatus("Profiler client is attached.");

    // blocked server is waiting for recording message from both clients
    // once the last one is connected, both messages should be sent
    m_qmlProfilerClient.sendRecordingStatus();
    m_v8profilerClient.sendRecordingStatus();
}

void QmlProfilerApplication::traceFinished()
{
    const QString fileName = traceFileName();

    if (m_eventList.save(fileName))
        print(QString("Saving trace to %1.").arg(fileName));

    if (m_quitAfterSave)
        quit();
}

void QmlProfilerApplication::recordingChanged()
{
    if (m_qmlProfilerClient.isRecording())
        print(QLatin1String("Recording is on."));
    else
        print(QLatin1String("Recording is off."));
}

void QmlProfilerApplication::print(const QString &line)
{
    QTextStream err(stderr);
    err << line << endl;
}

void QmlProfilerApplication::logError(const QString &error)
{
    QTextStream err(stderr);
    err << "Error: " << error << endl;
}

void QmlProfilerApplication::logStatus(const QString &status)
{
    if (!m_verbose)
        return;
    QTextStream err(stderr);
    err << status << endl;
}

void QmlProfilerApplication::qmlComplete()
{
    m_qmlDataReady = true;
    if (m_v8profilerClient.status() != QDeclarativeDebugClient::Enabled || m_v8DataReady) {
        m_eventList.complete();
        // once complete is sent, reset the flag
        m_qmlDataReady = false;
    }
}

void QmlProfilerApplication::v8Complete()
{
    m_v8DataReady = true;
    if (m_qmlProfilerClient.status() != QDeclarativeDebugClient::Enabled || m_qmlDataReady) {
        m_eventList.complete();
        // once complete is sent, reset the flag
        m_v8DataReady = false;
    }
}
