/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "qmlprofilerapplication.h"
#include "constants.h"
#include <utils/qtcassert.h>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QProcess>
#include <QtCore/QTimer>
#include <QtCore/QDateTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

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
"    -p=<number>, -port=<number>\n"
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
    m_traceClient(&m_connection),
    m_connectionAttempts(0)
{
    m_connectTimer.setInterval(1000);
    connect(&m_connectTimer, SIGNAL(timeout()), this, SLOT(tryToConnect()));

    connect(&m_connection, SIGNAL(connected()), this, SLOT(connected()));
    connect(&m_connection, SIGNAL(stateChanged(QAbstractSocket::SocketState)), this, SLOT(connectionStateChanged(QAbstractSocket::SocketState)));
    connect(&m_connection, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectionError(QAbstractSocket::SocketError)));

    connect(&m_traceClient, SIGNAL(enabled()), this, SLOT(traceClientEnabled()));
    connect(&m_traceClient, SIGNAL(recordingChanged(bool)), this, SLOT(recordingChanged()));
    connect(&m_traceClient, SIGNAL(range(int,qint64,qint64,QStringList,QString,int)), &m_eventList, SLOT(addRangedEvent(int,qint64,qint64,QStringList,QString,int)));
    connect(&m_traceClient, SIGNAL(complete()), &m_eventList, SLOT(complete()));

    connect(&m_eventList, SIGNAL(error(QString)), this, SLOT(logError(QString)));
    connect(&m_eventList, SIGNAL(dataReady()), this, SLOT(traceFinished()));
    connect(&m_eventList, SIGNAL(parsingStatusChanged()), this, SLOT(parsingStatusChanged()));
}

QmlProfilerApplication::~QmlProfilerApplication()
{
    if (!m_process)
        return;
    logStatus("Terminating process ...");
    m_process->disconnect();
    m_process->terminate();
    if (!m_process->waitForFinished(1000)) {
        logStatus("Killing process ...");
        m_process->kill();
    }
    delete m_process;
}

bool QmlProfilerApplication::parseArguments()
{
    for (int argPos = 1; argPos < arguments().size(); ++argPos) {
        const QString arg = arguments().at(argPos);
        if (arg == QLatin1String("-attach") || arg == QLatin1String("-a")) {
            if (argPos + 1 == arguments().size()) {
                return false;
            }
            m_hostName = arguments().at(++argPos);
            m_runMode = AttachMode;
        } else if (arg == QLatin1String("-port") || arg == QLatin1String("-p")) {
            if (argPos + 1 == arguments().size()) {
                return false;
            }
            const QString portStr = arguments().at(++argPos);
            bool isNumber;
            m_port = portStr.toUShort(&isNumber);
            if (!isNumber) {
                logError(QString("'%1' is not a valid port").arg(portStr));
                return false;
            }
        } else if (arg == QLatin1String("-fromStart")) {
            m_traceClient.setRecording(true);
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
    QString fileName = m_tracePrefix + "_" +
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
    return fileName;
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
        m_traceClient.setRecording(!m_traceClient.isRecording());
    } else if (cmd == Constants::CMD_QUIT
               || cmd == Constants::CMD_QUIT2) {
        if (m_traceClient.isRecording()) {
            m_quitAfterSave = true;
            m_traceClient.setRecording(false);
        } else {
            quit();
        }
    } else {
        logError(QString("Unknown command '%1'").arg(cmd));
        printCommands();
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
        logStatus(QString("Starting '%1 %2' ...").arg(m_programPath, arguments.join(" ")));
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
            logError(QString("Could not connect to %1:%2 for %3 seconds ...").arg(
                         m_hostName, QString::number(m_port), QString::number(m_connectionAttempts)));
    }

    if (m_connection.state() == QAbstractSocket::UnconnectedState) {
        logStatus(QString("Connecting to %1:%2 ...").arg(m_hostName, QString::number(m_port)));
        m_connection.connectToHost(m_hostName, m_port);
    }
}

void QmlProfilerApplication::connected()
{
    m_connectTimer.stop();
    if (m_traceClient.isRecording()) {
        logStatus("Connected. Recording is on.");
    } else {
        logStatus("Connected. Recording is off.");
    }
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

        if (m_traceClient.isRecording()) {
            logError("Process exited while recording, last trace is lost!");
            exit(2);
        } else {
            exit(0);
        }
    } else {
        logError("Process crashed! Exiting ...");
        exit(3);
    }
}

void QmlProfilerApplication::traceClientEnabled()
{
    logStatus("Trace client is attached.");
}

void QmlProfilerApplication::traceFinished()
{
    const QString fileName = traceFileName();
    print(QString("Saving trace to %1.").arg(fileName));
    m_eventList.save(fileName);
    if (m_quitAfterSave)
        quit();
}

void QmlProfilerApplication::parsingStatusChanged()
{
    if (m_verbose) {
        switch (m_eventList.getParsingStatus()) {
        case GettingDataStatus:
            logStatus("Parsing - Getting data ...");
            break;
        case SortingListsStatus:
            logStatus("Parsing - Sorting ...");
            break;
        case SortingEndsStatus:
            logStatus("Parsing - Sorting done");
            break;
        case ComputingLevelsStatus:
            logStatus("Parsing - Computing levels ...");
            break;
        case CompilingStatisticsStatus:
            logStatus("Parsing - Computing statistics ...");
            break;
        case DoneStatus:
            logStatus("Parsing - Done.");
            break;
        }
    }
}

void QmlProfilerApplication::recordingChanged()
{
    QTextStream err(stderr);
    if (m_traceClient.isRecording()) {
        err << "Recording is on." << endl;
    } else {
        err << "Recording is off." << endl;
    }
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
