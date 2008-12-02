/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
***************************************************************************/

#include "gdbengine.h"

#include "assert.h"
#include "debuggerconstants.h"
#include "debuggermanager.h"
#include "gdbmi.h"
#include "procinterrupt.h"

#include "disassemblerhandler.h"
#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"

#include "startexternaldialog.h"
#include "attachexternaldialog.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTime>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QLabel>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>

#if defined(Q_OS_LINUX) || defined(Q_OS_MAC)
#include <unistd.h>
#include <dlfcn.h>
#endif

using namespace Debugger;
using namespace Debugger::Internal;
using namespace Debugger::Constants;

Q_DECLARE_METATYPE(Debugger::Internal::GdbMi);

//#define DEBUG_PENDING  1
//#define DEBUG_SUBITEM  1

#if DEBUG_PENDING
#   define PENDING_DEBUG(s) qDebug() << s
#else
#   define PENDING_DEBUG(s) 
#endif

static const QString tooltipIName = "tooltip";

///////////////////////////////////////////////////////////////////////
//
// GdbSettings
//
///////////////////////////////////////////////////////////////////////

GdbSettings &Debugger::Internal::theGdbSettings()
{
    static GdbSettings settings;
    return settings;
}


///////////////////////////////////////////////////////////////////////
//
// GdbCommandType
//
///////////////////////////////////////////////////////////////////////

enum GdbCommandType
{
    GdbInvalidCommand = 0,

    GdbShowVersion = 100,
    GdbFileExecAndSymbols,
    GdbQueryPwd,
    GdbQuerySources,
    GdbQuerySources2,
    GdbAsyncOutput2,
    GdbExecRun,
    GdbExecRunToFunction,
    GdbExecStep,
    GdbExecNext,
    GdbExecStepI,
    GdbExecNextI,
    GdbExecContinue,
    GdbExecFinish,
    GdbExecJumpToLine,
    GdbExecInterrupt,
    GdbInfoShared,
    GdbInfoProc,
    GdbQueryDataDumper1,
    GdbQueryDataDumper2,
    GdbInitializeSocket1,

    BreakCondition = 200,
    BreakEnablePending,
    BreakSetAnnotate,
    BreakDelete,
    BreakList,
    BreakIgnore,
    BreakInfo,
    BreakInsert,
    BreakInsert1,

    DisassemblerList = 300,

    ModulesList = 400,

    RegisterListNames = 500,
    RegisterListValues,

    StackSelectThread = 600,
    StackListThreads,
    StackListFrames,
    StackListLocals,
    StackListArguments,

    WatchVarAssign = 700,             // data changed by user
    WatchVarListChildren,
    WatchVarCreate,
    WatchEvaluateExpression,
    WatchToolTip,
    WatchDumpCustomSetup,
    WatchDumpCustomValue1,           // waiting for gdb ack
    WatchDumpCustomValue2,           // waiting for actual data
    WatchDumpCustomEditValue,
};

QString dotEscape(QString str)
{
    str.replace(' ', '.');
    str.replace('\\', '.');
    str.replace('/', '.');
    return str;
}

QString currentTime()
{
    return QTime::currentTime().toString("hh:mm:ss.zzz");
}

static int &currentToken()
{
    static int token = 0;
    return token;
}

static bool isSkippableFunction(const QString &funcName, const QString &fileName)
{
    if (fileName.endsWith("kernel/qobject.cpp"))
        return true;
    if (fileName.endsWith("kernel/moc_qobject.cpp"))
        return true;
    if (fileName.endsWith("kernel/qmetaobject.cpp"))
        return true;
    if (fileName.endsWith(".moc"))
        return true;

    if (funcName.endsWith("::qt_metacall"))
        return true;

    return false;
}

static bool isLeavableFunction(const QString &funcName, const QString &fileName)
{
    if (funcName.endsWith("QObjectPrivate::setCurrentSender"))
        return true;
    if (fileName.endsWith("kernel/qmetaobject.cpp")
            && funcName.endsWith("QMetaObject::methodOffset"))
        return true;
    if (fileName.endsWith("kernel/qobject.h"))
        return true;
    if (fileName.endsWith("kernel/qobject.cpp")
            && funcName.endsWith("QObjectConnectionListVector::at"))
        return true;
    if (fileName.endsWith("kernel/qobject.cpp")
            && funcName.endsWith("~QObject"))
        return true;
    if (fileName.endsWith("thread/qmutex.cpp"))
        return true;
    if (fileName.endsWith("thread/qthread.cpp"))
        return true;
    if (fileName.endsWith("thread/qthread_unix.cpp"))
        return true;
    if (fileName.endsWith("thread/qmutex.h"))
        return true;
    if (fileName.contains("thread/qbasicatomic"))
        return true;
    if (fileName.contains("thread/qorderedmutexlocker_p"))
        return true;
    if (fileName.contains("arch/qatomic"))
        return true;
    if (fileName.endsWith("tools/qvector.h"))
        return true;
    if (fileName.endsWith("tools/qlist.h"))
        return true;
    if (fileName.endsWith("tools/qhash.h"))
        return true;
    if (fileName.endsWith("tools/qmap.h"))
        return true;
    if (fileName.endsWith("tools/qstring.h"))
        return true;
    if (fileName.endsWith("global/qglobal.h"))
        return true;

    return false;
}


///////////////////////////////////////////////////////////////////////
//
// GdbEngine
//
///////////////////////////////////////////////////////////////////////

GdbEngine::GdbEngine(DebuggerManager *parent)
{
    q = parent;
    qq = parent->engineInterface();
    init();
}

GdbEngine::~GdbEngine()
{
}

void GdbEngine::init()
{
    m_pendingRequests = 0;
    m_gdbVersion = 100;
    m_shared = 0;
    qq->debugDumpersAction()->setChecked(false);

    m_oldestAcceptableToken = -1;

    // Gdb Process interaction
    connect(&m_gdbProc, SIGNAL(error(QProcess::ProcessError)), this,
        SLOT(gdbProcError(QProcess::ProcessError)));
    connect(&m_gdbProc, SIGNAL(readyReadStandardOutput()), this,
        SLOT(readGdbStandardOutput()));
    connect(&m_gdbProc, SIGNAL(readyReadStandardError()), this,
        SLOT(readGdbStandardError()));
    connect(&m_gdbProc, SIGNAL(finished(int, QProcess::ExitStatus)), q,
        SLOT(exitDebugger()));

    // Custom dumpers
    //m_dumperServerConnection = 0;
    //m_dumperServer = new DumperServer(this);
    //QString name = "gdb-" +
    //    QDateTime::currentDateTime().toString("yyyy_MM_dd-hh_mm_ss_zzz");
    //m_dumperServer->listen(name);
    //connect(m_dumperServer, SIGNAL(newConnection()),
    //   this, SLOT(acceptConnection()));

    //if (!m_dumperServer->isListening()) {
    //    QMessageBox::critical(q->mainWindow(), tr("Dumper Server Setup Failed"),
    //      tr("Unable to create server listening for data: %1.\n"
    //         "Server name: %2").arg(m_dumperServer->errorString(), name),
    //      QMessageBox::Retry | QMessageBox::Cancel);
   // }

    connect(qq->debugDumpersAction(), SIGNAL(toggled(bool)),
        this, SLOT(setDebugDumpers(bool)));

    connect(qq->useCustomDumpersAction(), SIGNAL(toggled(bool)),
        this, SLOT(setCustomDumpersWanted(bool)));

    // Output
    connect(this, SIGNAL(gdbResponseAvailable()),
        this, SLOT(handleResponse()), Qt::QueuedConnection);

    connect(this, SIGNAL(gdbOutputAvailable(QString,QString)),
        q, SLOT(showDebuggerOutput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(gdbInputAvailable(QString,QString)),
        q, SLOT(showDebuggerInput(QString,QString)),
        Qt::QueuedConnection);
    connect(this, SIGNAL(applicationOutputAvailable(QString,QString)),
        q, SLOT(showApplicationOutput(QString,QString)),
        Qt::QueuedConnection);
}

void GdbEngine::gdbProcError(QProcess::ProcessError error)
{
    QString msg;
    switch (error) {
        case QProcess::FailedToStart:
            msg = QString(tr("The Gdb process failed to start. Either the "
                "invoked program '%1' is missing, or you may have insufficient "
                "permissions to invoke the program.")).arg(theGdbSettings().m_gdbCmd);
            break;
        case QProcess::Crashed:
            msg = tr("The Gdb process crashed some time after starting "
                "successfully.");
            break;
        case QProcess::Timedout:
            msg = tr("The last waitFor...() function timed out. "
                "The state of QProcess is unchanged, and you can try calling "
                "waitFor...() again.");
            break;
        case QProcess::WriteError:
            msg = tr("An error occurred when attempting to write "
                "to the Gdb process. For example, the process may not be running, "
                "or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            msg = tr("An error occurred when attempting to read from "
                "the Gdb process. For example, the process may not be running.");
            break;
        default:
            msg = tr("An unknown error in the Gdb process occurred. "
                "This is the default return value of error().");
    }

    q->showStatusMessage(msg, 5000);
    QMessageBox::critical(q->mainWindow(), tr("Error"), msg);
    // act as if it was closed by the core
    q->exitDebugger();
}

static void skipSpaces(const char *&from, const char *to)
{
    while (from != to && QChar(*from).isSpace())
        ++from;
}

static inline bool isNameChar(char c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

#if 0
static void dump(const char *first, const char *middle, const QString & to)
{
    QByteArray ba(first, middle - first);
    Q_UNUSED(to);
    // note that qDebug cuts off output after a certain size... (bug?)
    qDebug("\n>>>>> %s\n%s\n====\n%s\n<<<<<\n",
        qPrintable(currentTime()),
        qPrintable(QString(ba).trimmed()),
        qPrintable(to.trimmed()));
    //qDebug() << "";
    //qDebug() << qPrintable(currentTime())
    //    << " Reading response:  " << QString(ba).trimmed() << "\n";
}
#endif

static void skipTerminator(const char *&from, const char *to)
{
    skipSpaces(from, to);
    // skip '(gdb)'
    if (from[0] == '(' && from[1] == 'g' && from[3] == 'b' && from[4] == ')')
        from += 5;
    skipSpaces(from, to);
}

// called asyncronously as response to Gdb stdout output in
// gdbResponseAvailable()
void GdbEngine::handleResponse()
{
    static QTime lastTime;

    emit gdbOutputAvailable("            ", currentTime());
    emit gdbOutputAvailable("stdout:", m_inbuffer);

#if 0
    qDebug() // << "#### start response handling #### "
        << currentTime()
        << lastTime.msecsTo(QTime::currentTime()) << "ms,"
        << "buf: " << m_inbuffer.left(1500) << "..."
        //<< "buf: " << m_inbuffer
        << "size:" << m_inbuffer.size();
#else
    //qDebug() << "buf: " << m_inbuffer;
#endif

    lastTime = QTime::currentTime();

    while (1) {
        if (m_inbuffer.isEmpty())
            break;

        const char *from = m_inbuffer.constData();
        // FIXME: check for line ending in '\n(gdb)\n'
        const char *to = from + m_inbuffer.size();
        const char *inner;

        //const char *oldfrom = from;

        //skipSpaces(from, to);
        skipTerminator(from, to);
        int token = -1;

        // token is a sequence of numbers
        for (inner = from; inner != to; ++inner)
            if (*inner < '0' || *inner > '9')
                break;
        if (from != inner) {
            token = QString(QByteArray(from, inner - from)).toInt();
            from = inner;
            //qDebug() << "found token " << token;
        }

        if (from == to) {
            //qDebug() << "Returning: " << toString();
            break;
        }

        if (token == -1 && *from != '&' && *from != '~') {
            // FIXME: On Linux the application's std::out is merged in here.
            // High risk of falsely interpreting this as MI output.
            // We assume that we _always_ use tokens, so not finding a token
            // is a positive indication for the presence of application output.
            QString s;
            while (from != to && *from != '\n')
                s += *from++;
            //qDebug() << "UNREQUESTED DATA " << s << " TAKEN AS APPLICATION OUTPUT";
            s += '\n';

            m_inbuffer = QByteArray(from, to - from);
            emit applicationOutputAvailable("app-stdout: ", s);
            continue;
        }

        // next char decides kind of record
        const char c = *from++;
        //qDebug() << "CODE:" << c;

        switch (c) {
            case '*':
            case '+':
            case '=': {
                QByteArray asyncClass;
                for (; from != to; ++from) {
                    const char c = *from;
                    if (!isNameChar(c))
                      break;
                    asyncClass += *from;
                }
                //qDebug() << "ASYNCCLASS" << asyncClass;

                GdbMi record;
                while (from != to && *from == ',') {
                    ++from; // skip ','
                    GdbMi data;
                    data.parseResultOrValue(from, to);
                    if (data.isValid()) {
                        //qDebug() << "parsed response: " << data.toString();
                        record.m_children += data;
                        record.m_type = GdbMi::Tuple;
                    }
                }
                //dump(oldfrom, from, record.toString());
                skipTerminator(from, to);
                m_inbuffer = QByteArray(from, to - from);
                if (asyncClass == "stopped") {
                    handleAsyncOutput(record);
                } else {
                    qDebug() << "INGNORED ASYNC OUTPUT " << record.toString();
                }
                break;
            }

            case '~':
            case '@':
            case '&': {
                QString data = GdbMi::parseCString(from, to);
                handleStreamOutput(data, c);
                //dump(oldfrom, from, record.toString());
                m_inbuffer = QByteArray(from, to - from);
                break;
            }

            case '#': {
                //qDebug() << "CUSTOM OUTPUT, TOKEN" << token;
                QString str;
                for (; from != to && *from >= '0' && *from <= '9'; ++from)
                    str += QLatin1Char(*from);
                ++from; // skip the ' '
                int len = str.toInt();
                QByteArray ba(from, len);
                from += len;
                m_inbuffer = QByteArray(from, to - from);
                m_customOutputForToken[token] += QString(ba);
                break;
            }

            case '^': {
                GdbResultRecord record;

                record.token = token;

                for (inner = from; inner != to; ++inner)
                    if (*inner < 'a' || *inner > 'z')
                        break;

                QByteArray resultClass(from, inner - from);

                if (resultClass == "done")
                    record.resultClass = GdbResultDone;
                else if (resultClass == "running")
                    record.resultClass = GdbResultRunning;
                else if (resultClass == "connected")
                    record.resultClass = GdbResultConnected;
                else if (resultClass == "error")
                    record.resultClass = GdbResultError;
                else if (resultClass == "exit")
                    record.resultClass = GdbResultExit;
                else
                    record.resultClass = GdbResultUnknown;

                from = inner;
                skipSpaces(from, to);
                if (from != to && *from == ',') {
                    ++from;
                    record.data.parseTuple_helper(from, to);
                    record.data.m_type = GdbMi::Tuple;
                    record.data.m_name = "data";
                }
                skipSpaces(from, to);
                skipTerminator(from, to);

                //qDebug() << "\nLOG STREAM:" + m_pendingLogStreamOutput;
                //qDebug() << "\nTARGET STREAM:" + m_pendingTargetStreamOutput;
                //qDebug() << "\nCONSOLE STREAM:" + m_pendingConsoleStreamOutput;
                record.data.setStreamOutput("logstreamoutput",
                    m_pendingLogStreamOutput);
                record.data.setStreamOutput("targetstreamoutput",
                    m_pendingTargetStreamOutput);
                record.data.setStreamOutput("consolestreamoutput",
                    m_pendingConsoleStreamOutput);
                QByteArray custom = m_customOutputForToken[token];
                if (!custom.isEmpty())
                    record.data.setStreamOutput("customvaluecontents",
                        '{' + custom + '}');
                //m_customOutputForToken.remove(token);
                m_pendingLogStreamOutput.clear();
                m_pendingTargetStreamOutput.clear();
                m_pendingConsoleStreamOutput.clear();

                //dump(oldfrom, from, record.toString());
                m_inbuffer = QByteArray(from, to - from);
                handleResultRecord(record);
                break;
            }
            default: {
                qDebug() << "FIXME: UNKNOWN CODE: " << c << " IN " << m_inbuffer;
                m_inbuffer = QByteArray(from, to - from);
                break;
            }
        }
    }

    //qDebug() << "##### end response handling ####\n\n\n"
    //    << currentTime() << lastTime.msecsTo(QTime::currentTime());
    lastTime = QTime::currentTime();
}

#ifdef Q_OS_MAC
static void fixMac(QByteArray &out)
{
    // HACK: gdb on Mac mixes MI1 and MI2 syntax. Not nice.
    // it returns:   9^done,locals={{name="a"},{name="w"}}
    // instead of:   9^done,locals=[{name="a"},{name="w"}]
    if (!out.contains("locals={{name"))
        return;

    static const QByteArray termArray("(gdb) ");
    int pos = out.indexOf(termArray);
    if (pos == -1)
        return;

    int pos1 = out.indexOf("={{");
    if (pos1 == -1)
        return;

    int pos2 = out.indexOf("]]");
    if (pos2 == -1)
        return;

    if (pos1 < pos && pos2 < pos) {
        out[pos1 + 1] = '[';
        out[pos2 + 1] = ']';
    }
}
#endif

void GdbEngine::readGdbStandardError()
{
    QByteArray err = m_gdbProc.readAllStandardError();
    emit applicationOutputAvailable("app-stderr:", err);
}

void GdbEngine::readGdbStandardOutput()
{
    // This is the function called whenever the Gdb process created
    // output. As a rule of thumb, stdout contains _real_ Gdb output
    // as responses to our command (with exception of the data dumpers)
    // and "spontaneous" events like messages on loaded shared libraries.
    // Otoh, stderr contains application output produced by qDebug etc.
    // There is no organized way to pass application stdout output

    // The result of custom data dumpers arrives over the socket _before_
    // the corresponding Gdb "^done" message arrives here over stdout
    // and is merged into the response via m_pendingCustomValueContents.

    // Note that this code here runs syncronized to the arriving
    // output. The completed response will be signalled by a queued
    // connection to the handlers.

    QByteArray out = m_gdbProc.readAllStandardOutput();

    //qDebug() << "\n\n\nPLUGIN OUT: '" <<  out.data() << "'\n\n\n";

    #ifdef Q_OS_MAC
    fixMac(out);
    #endif

    m_inbuffer.append(out);
    //QWB_ASSERT(!m_inbuffer.isEmpty(), return);

    char c = m_inbuffer[m_inbuffer.size() - 1];
    static const QByteArray termArray("(gdb) ");
    if (out.indexOf(termArray) == -1 && c != 10 && c != 13) {
        //qDebug() << "\n\nBuffer not yet filled, waiting for more data to arrive";
        //qDebug() << m_inbuffer.data() << m_inbuffer.size();
        //qDebug() << "\n\n";
        return;
    }

    emit gdbResponseAvailable();
}

void GdbEngine::interruptInferior()
{
    if (m_gdbProc.state() == QProcess::NotRunning)
        return;

    if (q->m_attachedPID) {
        if (interruptProcess(q->m_attachedPID))
            qq->notifyInferiorStopped();
        return;
    }

#ifdef Q_OS_MAC
    sendCommand("-exec-interrupt", GdbExecInterrupt);
    qq->notifyInferiorStopped();
#else
    if (interruptChildProcess(m_gdbProc.pid()))
        qq->notifyInferiorStopped();
#endif
}

void GdbEngine::maybeHandleInferiorPidChanged(const QString &pid0)
{
    int pid = pid0.toInt();
    if (pid == 0) {
        qDebug() << "Cannot parse PID from " << pid0;
        return;
    }
    if (pid == m_inferiorPid)
        return;
    m_inferiorPid = pid;
    qq->notifyInferiorPidChanged(pid); 
}

void GdbEngine::sendSynchronizedCommand(const QString & command,
    int type, const QVariant &cookie, bool needStop)
{
    sendCommand(command, type, cookie, needStop, true);
}

void GdbEngine::sendCommand(const QString &command, int type,
    const QVariant &cookie, bool needStop, bool synchronized)
{
    if (m_gdbProc.state() == QProcess::NotRunning) {
        //qDebug() << "NO GDB PROCESS RUNNING, CMD IGNORED:" << command;
        return;
    }

    bool temporarilyStopped = false;
    if (needStop && q->status() == DebuggerInferiorRunning) {
        q->showStatusMessage(tr("Temporarily stopped"), -1);
        interruptInferior();
        temporarilyStopped = true;
    }

    ++currentToken();
    if (synchronized) {
        ++m_pendingRequests;
        PENDING_DEBUG("   TYPE " << type << " INCREMENTS PENDING TO: "
            << m_pendingRequests << command);
    } else {
        PENDING_DEBUG("   UNKNOWN TYPE " << type << " LEAVES PENDING AT: "
            << m_pendingRequests << command);
    }

    GdbCookie cmd;
    cmd.synchronized = synchronized;
    cmd.command = command;
    cmd.command = QString::number(currentToken()) + cmd.command;
    if (cmd.command.contains("%1"))
        cmd.command = cmd.command.arg(currentToken());
    cmd.type = type;
    cmd.cookie = cookie;

    m_cookieForToken[currentToken()] = cmd;

    //qDebug() << "";
    if (!command.isEmpty()) {
        //qDebug() << qPrintable(currentTime()) << "RUNNING  << cmd.command;
        m_gdbProc.write(cmd.command.toLatin1() + "\r\n");
        //emit gdbInputAvailable(QString(), "         " +  currentTime());
        emit gdbInputAvailable(QString(), "[" + currentTime() + "]    " + cmd.command);
        //emit gdbInputAvailable(QString(), cmd.command);
    }

    if (temporarilyStopped)
        sendCommand("-exec-continue");

    // slows down
    //qApp->processEvents();
}

void GdbEngine::handleResultRecord(const GdbResultRecord &record)
{
    //qDebug() << "TOKEN: " << record.token
    //    << " ACCEPTABLE: " << m_oldestAcceptableToken;
    //qDebug() << "";
    //qDebug() << qPrintable(currentTime()) << "Reading response:  "
    //   << record.toString() << "\n";
    //qDebug() << "\nRESULT" << record.token << record.toString();

    int token = record.token;
    if (token == -1)
        return;

    if (!m_cookieForToken.contains(token)) {
        qDebug() << "NO SUCH TOKEN (ANYMORE): " << token;
        return;
    }

    GdbCookie cmd = m_cookieForToken.take(token);

    // FIXME: this falsely rejects results from the custom dumper recognition
    // procedure, too...
    if (record.token < m_oldestAcceptableToken) {
        //qDebug() << "### SKIPPING OLD RESULT " << record.toString();
        //QMessageBox::information(m_mainWindow, tr("Skipped"), "xxx");
        return;
    }

    // We get _two_ results for a '-exec-foo' command: First a
    // 'running' notification, then a 'stopped' or similar.
    // So put it back.
    if (record.resultClass == GdbResultRunning)
        m_cookieForToken[token] = cmd;

#if 0
    qDebug() << "# handleOutput, "
        << "cmd type: " << cmd.type
        << "cmd synchronized: " << cmd.synchronized
        << "\n record: " << record.toString();
#endif

    // << "\n data: " << record.data.toString(true);

    if (cmd.type != GdbInvalidCommand)
        handleResult(record, cmd.type, cmd.cookie);

    if (cmd.synchronized) {
        --m_pendingRequests;
        PENDING_DEBUG("   TYPE " << cmd.type << " DECREMENTS PENDING TO: "
            << m_pendingRequests << cmd.command);
        if (m_pendingRequests == 0)
            updateWatchModel2();
    } else {
        PENDING_DEBUG("   UNKNOWN TYPE " << cmd.type << " LEAVES PENDING AT: "
            << m_pendingRequests << cmd.command);
    }
}

void GdbEngine::handleResult(const GdbResultRecord & record, int type,
    const QVariant & cookie)
{
    switch (type) {
        case GdbExecNext:
        case GdbExecStep:
        case GdbExecNextI:
        case GdbExecStepI:
        case GdbExecContinue:
        case GdbExecFinish:
            // evil code sharing
        case GdbExecRun:
            handleExecRun(record);
            break;
        case GdbInfoProc:
            handleInfoProc(record);
            break;

        case GdbShowVersion:
            handleShowVersion(record);
            break;
        case GdbFileExecAndSymbols:
            handleFileExecAndSymbols(record);
            break;
        case GdbExecRunToFunction:
            // that should be "^running". We need to handle the resulting
            // "Stopped"
            //handleExecRunToFunction(record);
            break;
        case GdbExecInterrupt:
            break;
        case GdbExecJumpToLine:
            handleExecJumpToLine(record);
            break;
        case GdbQueryPwd:
            handleQueryPwd(record);
            break;
        case GdbQuerySources:
            handleQuerySources(record);
            break;
        case GdbQuerySources2:
            handleQuerySources2(record, cookie);
            break;
        case GdbAsyncOutput2:
            handleAsyncOutput2(cookie.value<GdbMi>());
            break;
        case GdbInfoShared:
            handleInfoShared(record);
            break;
        case GdbInitializeSocket1:
            //qDebug() << " INIT SOCKET" << record.toString();
            break;
        case GdbQueryDataDumper1:
            handleQueryDataDumper1(record);
            break;
        case GdbQueryDataDumper2:
            handleQueryDataDumper2(record);
            break;

        case BreakList:
            handleBreakList(record);
            break;
        case BreakInsert:
            handleBreakInsert(record, cookie.toInt());
            break;
        case BreakInsert1:
            handleBreakInsert1(record, cookie.toInt());
            break;
        case BreakInfo:
            handleBreakInfo(record, cookie.toInt());
            break;
        case BreakEnablePending:
        case BreakDelete:
            // nothing
            break;
        case BreakIgnore:
            handleBreakIgnore(record, cookie.toInt());
            break;
        case BreakCondition:
            handleBreakCondition(record, cookie.toInt());
            break;

        case DisassemblerList:
            handleDisassemblerList(record, cookie.toString());
            break;

        case ModulesList:
            handleModulesList(record);
            break;

        case RegisterListNames:
            handleRegisterListNames(record);
            break;
        case RegisterListValues:
            handleRegisterListValues(record);
            break;

        case StackListFrames:
            handleStackListFrames(record);
            break;
        case StackListThreads:
            handleStackListThreads(record, cookie.toInt());
            break;
        case StackSelectThread:
            handleStackSelectThread(record, cookie.toInt());
            break;
        case StackListLocals:
            handleStackListLocals(record);
            break;
        case StackListArguments:
            handleStackListArguments(record);
            break;

        case WatchVarListChildren:
            handleVarListChildren(record, cookie.value<WatchData>());
            break;
        case WatchVarCreate:
            handleVarCreate(record, cookie.value<WatchData>());
            break;
        case WatchVarAssign:
            handleVarAssign();
            break;
        case WatchEvaluateExpression:
            handleEvaluateExpression(record, cookie.value<WatchData>());
            break;
        case WatchToolTip:
            handleToolTip(record, cookie.toString());
            break;
        case WatchDumpCustomValue1:
            handleDumpCustomValue1(record, cookie.value<WatchData>());
            break;
        case WatchDumpCustomValue2:
            handleDumpCustomValue2(record, cookie.value<WatchData>());
            break;
        case WatchDumpCustomSetup:
            handleDumpCustomSetup(record);
            break;

        default:
            qDebug() << "FIXME: GdbEngine::handleResult: "
                "should not happen" << type;
            break;
    }
}

void GdbEngine::executeDebuggerCommand(const QString &command)
{
    //createGdbProcessIfNeeded();
    if (m_gdbProc.state() == QProcess::NotRunning) {
        qDebug() << "NO GDB PROCESS RUNNING, PLAIN CMD IGNORED: " << command;
        return;
    }

    GdbCookie cmd;
    cmd.command = command;
    cmd.type = -1;

    //m_cookieForToken[currentToken()] = cmd;
    //++currentToken();

    //qDebug() << "";
    //qDebug() << currentTime() << "Running command:   " << cmd.command;
    emit gdbInputAvailable(QString(), cmd.command);
    m_gdbProc.write(cmd.command.toLatin1() + "\r\n");
}

void GdbEngine::handleQueryPwd(const GdbResultRecord &record)
{
    // FIXME: remove this special case as soon as 'pwd'
    // is supported by MI
    //qDebug() << "PWD OUTPUT:" <<  record.toString();
    // Gdb responses _unless_ we get an error first.
    if (record.resultClass == GdbResultDone) {
#ifdef Q_OS_LINUX
        // "5^done,{logstreamoutput="pwd ",consolestreamoutput
        // ="Working directory /home/apoenitz/dev/work/test1.  "}
        m_pwd = record.data.findChild("consolestreamoutput").data();
        int pos = m_pwd.indexOf("Working directory");
        m_pwd = m_pwd.mid(pos + 18);
        m_pwd = m_pwd.trimmed();
        if (m_pwd.endsWith('.'))
            m_pwd.chop(1);
#endif
#ifdef Q_OS_WIN
        // ~"Working directory C:\\Users\\Thomas\\Documents\\WBTest3\\debug.\n"
        m_pwd = record.data.findChild("consolestreamoutput").data();
        m_pwd = m_pwd.trimmed();
#endif
        //qDebug() << "PWD RESULT:" <<  m_pwd;
    }
}

void GdbEngine::handleQuerySources(const GdbResultRecord &record)
{
    if (record.resultClass == GdbResultDone) {
        m_shortToFullName.clear();
        m_fullToShortName.clear();
        // "^done,files=[{file="../../../../bin/gdbmacros/gdbmacros.cpp",
        // fullname="/data5/dev/ide/main/bin/gdbmacros/gdbmacros.cpp"},
        GdbMi files = record.data.findChild("files");
        foreach (const GdbMi &item, files.children()) {
            QString fileName = item.findChild("file").data();
            GdbMi fullName = item.findChild("fullname");
            QString full = fullName.data();
            #ifdef Q_OS_WIN
            full = QDir::cleanPath(full);
            #endif
            if (fullName.isValid() && QFileInfo(full).isReadable()) {
                //qDebug() << "STORING 2: " << fileName << full;
                m_shortToFullName[fileName] = full;
                m_fullToShortName[full] = fileName;
            }
        }
    }
}

void GdbEngine::handleInfoProc(const GdbResultRecord &record)
{
    if (record.resultClass == GdbResultDone) {
        #if defined(Q_OS_MAC)
        //^done,process-id="85075"
        maybeHandleInferiorPidChanged(record.data.findChild("process-id").data());
        #endif

        #if defined(Q_OS_LINUX) || defined(Q_OS_WIN)
        // FIXME: use something more robust
        QRegExp re(QLatin1String("process (\\d+)"));
        QString data = record.data.findChild("consolestreamoutput").data();
        if (re.indexIn(data) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
        #endif
    }
}

void GdbEngine::handleInfoShared(const GdbResultRecord &record)
{
    if (record.resultClass == GdbResultDone) {
        // let the modules handler do the parsing
        handleModulesList(record);
        QList<Module> modules = qq->modulesHandler()->modules();
        bool reloadNeeded = false;
        foreach (const Module &module, modules) {
            // FIXME: read this from some list
            if (!module.symbolsRead && !module.moduleName.contains("Q")) {
                reloadNeeded = true;
                sendCommand("sharedlibrary " + dotEscape(module.moduleName));
            }
        }
        if (reloadNeeded)
            reloadModules();
        continueInferior();
    }
}

void GdbEngine::handleQuerySources2(const GdbResultRecord &record,
    const QVariant &cookie)
{
    if (record.resultClass == GdbResultDone)
        handleAsyncOutput2(cookie.value<GdbMi>());
}

void GdbEngine::handleExecJumpToLine(const GdbResultRecord &record)
{
    // FIXME: remove this special case as soon as 'jump'
    // is supported by MI
    // "&"jump /home/apoenitz/dev/work/test1/test1.cpp:242"
    // ~"Continuing at 0x4058f3."
    // ~"run1 (argc=1, argv=0x7fffb213a478) at test1.cpp:242"
    // ~"242\t x *= 2;"
    //109^done"
    qq->notifyInferiorStopped();
    q->showStatusMessage(tr("Jumped. Stopped."), -1);
    QString output = record.data.findChild("logstreamoutput").data();
    if (!output.isEmpty())
        return;
    QString fileAndLine = output.section(' ', 1, 1);
    QString file = fileAndLine.section(':', 0, 0);
    int line = fileAndLine.section(':', 1, 1).toInt();
    q->gotoLocation(file, line, true);
}

void GdbEngine::handleExecRunToFunction(const GdbResultRecord &record)
{
    // FIXME: remove this special case as soon as there's a real
    // reason given when the temporary breakpoint is hit.
    // reight now we get:
    // 14*stopped,thread-id="1",frame={addr="0x0000000000403ce4",
    // func="foo",args=[{name="str",value="@0x7fff0f450460"}],
    // file="main.cpp",fullname="/tmp/g/main.cpp",line="37"}
    qq->notifyInferiorStopped();
    q->showStatusMessage(tr("Run to Function finished. Stopped."), -1);
    GdbMi frame = record.data.findChild("frame");
    QString file = frame.findChild("fullname").data();
    int line = frame.findChild("line").data().toInt();
    qDebug() << "HIT: " << file << line << " IN " << frame.toString()
        << " -- " << record.toString();
    q->gotoLocation(file, line, true);
}

void GdbEngine::handleStreamOutput(const QString &data, char code)
{
    // Linux
    if (data.contains("[New Thread")) {
        QRegExp re("\\[New Thread 0x([0-9a-f]*) \\(LWP ([0-9]*)\\)\\]");
        if (re.indexIn(data) != -1)
            maybeHandleInferiorPidChanged(re.cap(2));
    }

    // Mac
    if (data.contains("[Switching to process ")) {
        QRegExp re("\\[Switching to process ([0-9]*) local thread 0x([0-9a-f]*)\\]");
        if (re.indexIn(data) != -1)
            maybeHandleInferiorPidChanged(re.cap(1));
    }

    // present it twice: now and together with the next 'real' result
    switch (code) {
        case '~':
            m_pendingConsoleStreamOutput += data;
            break;
        case '@':
            m_pendingTargetStreamOutput += data;
            break;
        case '&':
            m_pendingLogStreamOutput += data;
            // On Windows, the contents seem to depend on the debugger
            // version and/or OS version used.
            if (data.startsWith("warning:"))
                qq->showApplicationOutput(QString(), data);
            break;
    }

#ifdef Q_OS_LINUX
    if (data.startsWith("Pending break") && data.contains("\" resolved")) {
        qDebug() << "SCHEDULING -break-list";
        //m_breakListOnStopNeeded = true;
    }
#endif

#if 0
    if (m_slurpingPTypeOutput)
        qDebug() << "SLURP: " << output.data;

    //  "No symbol \"__dlopen\" in current context."
    //  "No symbol \"dlopen\" in current context."
    if (output.data.startsWith("No symbol ")
            && output.data.contains("dlopen")) {
        m_dlopened = true;
        return;
    }

    // output of 'ptype <foo>'
    if (output.data.startsWith("type = ")) {
        if (output.data.endsWith("{") || output.data.endsWith("{\\n")) {
            // multi-line output started here...
            m_slurpingPTypeOutput = true;
            m_slurpedPTypeOutput = output.data;
        } else {
            // Happens for simple types. Process it immediately
            m_watchHandler->handleTypeContents(output.data);
        }
        return;
    }
    if (m_slurpingPTypeOutput) {
        m_slurpedPTypeOutput += '\n';
        m_slurpedPTypeOutput += output.data;
        if (output.data.startsWith("}")) {
            // this is the last line...
            m_slurpingPTypeOutput = false;
            m_watchHandler->handleTypeContents(m_slurpedPTypeOutput);
            m_slurpedPTypeOutput.clear();
        }
        return;
    }
#endif
}

static bool isExitedReason(const QString &reason)
{
    return reason == QLatin1String("exited-normally")   // inferior exited normally
        || reason == QLatin1String("exited-signalled")  // inferior exited because of a signal
        //|| reason == QLatin1String("signal-received") // inferior received signal
        || reason == QLatin1String("exited");           // inferior exited
}

static bool isStoppedReason(const QString &reason)
{
    return reason == QLatin1String("function-finished")  // -exec-finish
        || reason == QLatin1String("signal-received")  // handled as "isExitedReason"
        || reason == QLatin1String("breakpoint-hit")     // -exec-continue
        || reason == QLatin1String("end-stepping-range") // -exec-next, -exec-step
        || reason == QLatin1String("location-reached")   // -exec-until
        || reason == QLatin1String("access-watchpoint-trigger")
        || reason == QLatin1String("read-watchpoint-trigger")
#ifdef Q_OS_MAC
        || reason.isEmpty()
#endif
    ;
}

void GdbEngine::handleAsyncOutput(const GdbMi &data)
{
    const QString reason = data.findChild("reason").data();

    QString console = data.findChild("consolestreamoutput").data();
    if (console.contains("Stopped due to shared library event") || reason.isEmpty()) {
        ++m_shared;
        //if (m_shared == 2)
        //    tryLoadCustomDumpers();
        //qDebug() << "SHARED LIBRARY EVENT " << data.toString() << m_shared;
        if (qq->useFastStart()) {
            if (1 || m_shared <= 16) { // libpthread?
                sendCommand("info shared", GdbInfoShared);
                //sendCommand("sharedlibrary gdbdebugger ");
                //continueInferior();
            } else {
                // auto-load from now on
                sendCommand("info shared");
                sendCommand("set auto-solib-add on");
                sendCommand("-file-list-exec-source-files", GdbQuerySources);
                sendCommand("-break-list", BreakList);
                //sendCommand("bt");
                //QVariant var = QVariant::fromValue<GdbMi>(data);
                //sendCommand("p 1", GdbAsyncOutput2, var);  // dummy
                continueInferior();
            }
        } else {
            // slow start requested.
            q->showStatusMessage("Loading " + data.toString(), -1);
            continueInferior();
        }
        return;
    }

    if (isExitedReason(reason)) {
        qq->notifyInferiorExited();
        QString msg = "Program exited normally";
        if (reason == "exited") {
            msg = "Program exited with exit code "
                + data.findChild("exit-code").toString();
        } else if (reason == "exited-signalled") {
            msg = "Program exited after receiving signal "
                + data.findChild("signal-name").toString();
        } else if (reason == "signal-received") {
            msg = "Program exited after receiving signal "
                + data.findChild("signal-name").toString();
        }
        q->showStatusMessage(msg, -1);
        q->exitDebugger();
        return;
    }

    tryLoadCustomDumpers();

    // jump over well-known frames
    static int stepCounter = 0;
    if (qq->skipKnownFrames()) {
        if (reason == "end-stepping-range" || reason == "function-finished") {
            GdbMi frame = data.findChild("frame");
            //qDebug() << frame.toString();
            m_currentFrame = frame.findChild("addr").data() + '%' +
                 frame.findChild("func").data() + '%';

            QString funcName = frame.findChild("func").data();
            QString fileName = frame.findChild("file").data();
            if (isLeavableFunction(funcName, fileName)) {
                //qDebug() << "LEAVING" << funcName;
                ++stepCounter;
                q->stepOutExec();
                //stepExec();
                return;
            }
            if (isSkippableFunction(funcName, fileName)) {
                //qDebug() << "SKIPPING" << funcName;
                ++stepCounter;
                q->stepExec();
                return;
            }
            //if (stepCounter)
            //    qDebug() << "STEPCOUNTER:" << stepCounter;
            stepCounter = 0;
        }
    }

    if (isStoppedReason(reason) || reason.isEmpty()) {
        // Need another round trip
        if (reason == "breakpoint-hit") {
            GdbMi frame = data.findChild("frame");
            //qDebug() << frame.toString();
            m_currentFrame = frame.findChild("addr").data() + '%' +
                 frame.findChild("func").data() + '%';

            QApplication::alert(q->mainWindow(), 200);
            sendCommand("-file-list-exec-source-files", GdbQuerySources);
            sendCommand("-break-list", BreakList);
            QVariant var = QVariant::fromValue<GdbMi>(data);
            sendCommand("p 0", GdbAsyncOutput2, var);  // dummy
        } else {
            handleAsyncOutput2(data);
        }
        return;
    }

    qDebug() << "STOPPED FOR UNKNOWN REASON" << data.toString();
    // Ignore it. Will be handled with full response later in the
    // JumpToLine or RunToFunction handlers
#if 1
    // FIXME: remove this special case as soon as there's a real
    // reason given when the temporary breakpoint is hit.
    // reight now we get:
    // 14*stopped,thread-id="1",frame={addr="0x0000000000403ce4",
    // func="foo",args=[{name="str",value="@0x7fff0f450460"}],
    // file="main.cpp",fullname="/tmp/g/main.cpp",line="37"}
    //
    // MAC yields sometimes:
    // stdout:3661*stopped,time={wallclock="0.00658",user="0.00142",
    // system="0.00136",start="1218810678.805432",end="1218810678.812011"}
    q->resetLocation();
    qq->notifyInferiorStopped();
    q->showStatusMessage(tr("Run to Function finished. Stopped."), -1);
    GdbMi frame = data.findChild("frame");
    QString file = frame.findChild("fullname").data();
    int line = frame.findChild("line").data().toInt();
    qDebug() << "HIT: " << file << line << " IN " << frame.toString()
        << " -- " << data.toString();
    q->gotoLocation(file, line, true);
#endif
}


void GdbEngine::handleAsyncOutput2(const GdbMi &data)
{
    qq->notifyInferiorStopped();

    //
    // Breakpoints
    //
    //qDebug() << "BREAK ASYNC: " << output.toString();
    //sendListBreakpoints();
    //attemptBreakpointSynchronization();
    //if (m_breakListOnStopNeeded)
    //    sendListBreakpoints();

    // something reasonably 'invariant'
    // Linux:
    //"79*stopped,reason="end-stepping-range",reason="breakpoint-hit",bkptno="1",
    //thread-id="1",frame={addr="0x0000000000405d8f",func="run1",
    //args=[{name="argc",value="1"},{name="argv",value="0x7fffb7c23058"}],
    //file="test1.cpp",fullname="/home/apoenitz/dev/work/test1/test1.cpp"
    //,line="261"}"
    // Mac: (but only sometimes)
    // "82*stopped,bkpt={number="0",type="step
    // resume",disp="keep",enabled="y",addr="0x43127171",at="<Find::
    // Internal::FindToolWindow::invokeFindIncremental()
    // +225>",thread="1",shlib="/Users/epreuss/dev/ide/main/bin/
    // workbench.app/Contents/PlugIns/Trolltech/libFind.1.0.0.dylib",
    // frame="0xbfffd800",thread="1",times="1"},

    //
    // Stack
    //
    qq->stackHandler()->setCurrentIndex(0);
    updateLocals(); // Quick shot

    int currentId = data.findChild("thread-id").data().toInt();
    sendSynchronizedCommand("-stack-list-frames", StackListFrames);
    if (supportsThreads())
        sendSynchronizedCommand("-thread-list-ids", StackListThreads, currentId);

    //
    // Disassembler
    //
    // Linux:
    //"79*stopped,reason="end-stepping-range",reason="breakpoint-hit",bkptno="1",
    //thread-id="1",frame={addr="0x0000000000405d8f",func="run1",
    //args=[{name="argc",value="1"},{name="argv",value="0x7fffb7c23058"}],
    //file="test1.cpp",fullname="/home/apoenitz/dev/work/test1/test1.cpp",line="261"}"
    // Mac: (but only sometimes)
    m_address = data.findChild("frame").findChild("addr").data();
    qq->reloadDisassembler();

    //
    // Registers
    //
    qq->reloadRegisters();
}

void GdbEngine::handleShowVersion(const GdbResultRecord &response)
{
    if (response.resultClass == GdbResultDone) {
        m_gdbVersion = 100;
        QString msg = response.data.findChild("consolestreamoutput").data();
        QRegExp supported("GNU gdb 6.[6789]");
        if (msg.indexOf(supported) == -1) {
            QStringList list = msg.split("\n");
            while (list.size() > 2)
                list.removeLast();
            msg = tr("The debugger you are using identifies itself as:")
                + "<p><p>" + list.join("<br>") + "<p><p>"
                + tr("This version is not officially supported by Qt Creator.\n"
                     "Debugging will most likely not work well.\n"
                     "Using gdb 6.7 or later is strongly recommended.");
#if 0
            // ugly, but 'Show again' check box...
            static QErrorMessage *err = new QErrorMessage(m_mainWindow);
            err->setMinimumSize(400, 300);
            err->showMessage(msg);
#else
            //QMessageBox::information(m_mainWindow, tr("Warning"), msg);
#endif
        }
        int pos = msg.indexOf("GNU gdb 6.");
        if (pos != -1) {
            m_gdbVersion = 600 + (msg.at(pos + 10).unicode() - '0') * 10;
            //qDebug() << "GDB VERSION " << m_gdbVersion << msg;
        }
    }
}

void GdbEngine::handleFileExecAndSymbols
    (const GdbResultRecord &response)
{
    if (response.resultClass == GdbResultDone) {
        //m_breakHandler->clearBreakMarkers();
    } else if (response.resultClass == GdbResultError) {
        QString msg = response.data.findChild("msg").data();
        QMessageBox::critical(q->mainWindow(), tr("Error"),
            tr("Starting executable failed:\n") + msg);
        QWB_ASSERT(q->status() == DebuggerInferiorRunning, /**/);
        interruptInferior();
    }
}

void GdbEngine::handleExecRun(const GdbResultRecord &response)
{
    if (response.resultClass == GdbResultRunning) {
        qq->notifyInferiorRunning();
        q->showStatusMessage(tr("Running..."), -1);
        //reloadModules();
    } else if (response.resultClass == GdbResultError) {
        QString msg = response.data.findChild("msg").data();
        if (msg == "Cannot find bounds of current function") {
            qq->notifyInferiorStopped();
            //q->showStatusMessage(tr("No debug information available. "
            //  "Leaving function..."), -1);
            //stepOutExec();
        } else {
            QMessageBox::critical(q->mainWindow(), tr("Error"),
                tr("Starting executable failed:\n") + msg);
            QWB_ASSERT(q->status() == DebuggerInferiorRunning, /**/);
            interruptInferior();
        }
    }
}

void GdbEngine::queryFullName(const QString &fileName, QString *full)
{
    *full = fullName(fileName);
}

QString GdbEngine::shortName(const QString &fullName)
{
    return m_fullToShortName.value(fullName, QString());
}

QString GdbEngine::fullName(const QString &fileName)
{
    //QString absName = m_manager->currentWorkingDirectory() + "/" + file; ??
    if (fileName.isEmpty())
        return QString();
    QString full = m_shortToFullName.value(fileName, QString());
    //qDebug() << "RESOLVING: " << fileName << full;
    if (!full.isEmpty())
        return full;
    QFileInfo fi(fileName);
    if (!fi.isReadable())
        return QString();
    full = fi.absoluteFilePath();
    #ifdef Q_OS_WIN
    full = QDir::cleanPath(full);
    #endif
    //qDebug() << "STORING: " << fileName << full;
    m_shortToFullName[fileName] = full;
    m_fullToShortName[full] = fileName;
    return full;
}

QString GdbEngine::fullName(const QStringList &candidates)
{
    QString full;
    foreach (const QString &fileName, candidates) {
        full = fullName(fileName);
        if (!full.isEmpty())
            return full;
    }
    foreach (const QString &fileName, candidates) {
        if (!fileName.isEmpty())
            return fileName;
    }
    return full;
}

void GdbEngine::shutdown()
{
    exitDebugger();
}

void GdbEngine::exitDebugger()
{
    //qDebug() << "EXITING: " << m_gdbProc.state();
    if (m_gdbProc.state() == QProcess::Starting)
        m_gdbProc.waitForStarted();
    if (m_gdbProc.state() == QProcess::Running) {
        interruptInferior();
        sendCommand("kill");
        sendCommand("-gdb-exit");
        // 20s can easily happen when loading webkit debug information
        m_gdbProc.waitForFinished(20000);
        if (m_gdbProc.state() != QProcess::Running) {
            m_gdbProc.terminate();
            m_gdbProc.waitForFinished(20000);
        }
    }
    if (m_gdbProc.state() != QProcess::NotRunning)
        qDebug() << "PROBLEM STOPPING DEBUGGER";

    m_shortToFullName.clear();
    m_fullToShortName.clear();
    m_varToType.clear();
    m_dataDumperState = DataDumperUninitialized;
    m_shared = 0;
    qq->debugDumpersAction()->setChecked(false);
}


int GdbEngine::currentFrame() const
{
    return qq->stackHandler()->currentIndex();
}


bool GdbEngine::startDebugger()
{
    m_inferiorPid = 0;
    QStringList gdbArgs;

    QFileInfo fi(q->m_executable);
    QString fileName = '"' + fi.absoluteFilePath() + '"';

    if (m_gdbProc.state() != QProcess::NotRunning) {
        qDebug() << "GDB IS ALREADY RUNNING!";
        return false;
    }

    //gdbArgs.prepend(QLatin1String("--quiet"));
    gdbArgs.prepend(QLatin1String("mi"));
    gdbArgs.prepend(QLatin1String("-i"));

    if (!q->m_workingDir.isEmpty())
        m_gdbProc.setWorkingDirectory(q->m_workingDir);
    if (!q->m_environment.isEmpty())
        m_gdbProc.setEnvironment(q->m_environment);

    #if 0
    qDebug() << "Command: " << theGdbSettings().m_gdbCmd;
    qDebug() << "WorkingDirectory: " << m_gdbProc.workingDirectory();
    qDebug() << "Environment: " << m_gdbProc.environment();
    qDebug() << "Arguments: " << gdbArgs;
    qDebug() << "BuildDir: " << q->m_buildDir;
    qDebug() << "ExeFile: " << q->m_executable;
    #endif

    q->showStatusMessage("Starting Debugger", -1);
    emit gdbInputAvailable(QString(), theGdbSettings().m_gdbCmd + ' ' + gdbArgs.join(" "));

    m_gdbProc.start(theGdbSettings().m_gdbCmd, gdbArgs);
    m_gdbProc.waitForStarted();

    if (m_gdbProc.state() != QProcess::Running)
        return false;

    q->showStatusMessage(tr("Gdb Running"), -1);

    sendCommand("show version", GdbShowVersion);
    if (qq->useFastStart()) {
        sendCommand("set auto-solib-add off");
        sendCommand("set stop-on-solib-events 1");
    }
    //sendCommand("-enable-timings");
    //sendCommand("set stop-on-solib-events 1");
    sendCommand("set print static-members off"); // Seemingly doesn't work.
    //sendCommand("define hook-stop\n-thread-list-ids\n-stack-list-frames\nend");
    //sendCommand("define hook-stop\nprint 4\nend");
    //sendCommand("define hookpost-stop\nprint 5\nend");
    //sendCommand("define hook-call\nprint 6\nend");
    //sendCommand("define hookpost-call\nprint 7\nend");
    //sendCommand("set print object on"); // works with CLI, but not MI
    //sendCommand("set step-mode on");  // we can't work with that yes
    //sendCommand("set exec-done-display on");
    //sendCommand("set print pretty on");
    //sendCommand("set confirm off");
    //sendCommand("set pagination off");
    sendCommand("set breakpoint pending on", BreakEnablePending);

    // one of the following is needed to prevent crashes in gdb on code like:
    //  template <class T> T foo() { return T(0); }
    //  int main() { return foo<int>(); }
    //  (gdb) call 'int foo<int>'()
    //  /build/buildd/gdb-6.8/gdb/valops.c:2069: internal-error:
    sendCommand("set overload-resolution off");
    //sendCommand("set demangle-style none");

    // From the docs:
    //  Stop means reenter debugger if this signal happens (implies print).
    //  Print means print a message if this signal happens.
    //  Pass means let program see this signal;
    //  otherwise program doesn't know.
    //  Pass and Stop may be combined.
    // We need "print" as otherwise we would get no feedback whatsoever
    // Custom Dumper crashs which happen regularily for when accessing
    // uninitialized variables.
    sendCommand("handle SIGSEGV nopass stop print");

    // This is useful to kill the inferior whenever gdb dies.
    //sendCommand("handle SIGTERM pass nostop print");

    sendCommand("set unwindonsignal on");
    sendCommand("pwd", GdbQueryPwd);

    #ifdef Q_OS_MAC
    sendCommand("-gdb-set inferior-auto-start-cfm off");
    sendCommand("-gdb-set sharedLibrary load-rules "
            "dyld \".*libSystem.*\" "
            "all dyld \".*libauto.*\" "
            "all dyld \".*AppKit.*\" "
            "all dyld \".*PBGDBIntrospectionSupport.*\" "
            "all dyld \".*Foundation.*\" "
            "all dyld \".*CFDataFormatters.*\" "
            "all dyld \".*libobjc.*\" "
            "all dyld \".*CarbonDataFormatters");
    #endif

    if (q->startMode() == q->attachExternal) {
        sendCommand("attach " + QString::number(q->m_attachedPID));
    }

    if (q->startMode() == q->startInternal) {
        sendCommand("-file-exec-and-symbols " + fileName, GdbFileExecAndSymbols);
        #ifdef Q_OS_MAC
        sendCommand("sharedlibrary apply-load-rules all");
        #endif
        sendCommand("-file-list-exec-source-files", GdbQuerySources);
        //sendCommand("-gdb-set stop-on-solib-events 1");
    }

    sendCommand("-data-list-register-names", RegisterListNames);

    // set all to "pending"
    if (q->startMode() == q->attachExternal)
        qq->breakHandler()->removeAllBreakpoints();
    else
        qq->breakHandler()->setAllPending();

    QTimer::singleShot(0, this, SLOT(attemptBreakpointSynchronization()));

    return true;
}

void GdbEngine::continueInferior()
{
    q->resetLocation();
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    emit gdbInputAvailable(QString(), QString());
    sendCommand("-exec-continue", GdbExecContinue);
}

void GdbEngine::runInferior()
{
    q->resetLocation();
    // FIXME: this ignores important startup messages
    setTokenBarrier();
    if (!q->m_processArgs.isEmpty())
        sendCommand("-exec-arguments " + q->m_processArgs.join(" "));
    qq->notifyInferiorRunningRequested();
    emit gdbInputAvailable(QString(), QString());
    sendCommand("-exec-run", GdbExecRun);
#if defined(Q_OS_WIN)
    sendCommand("info proc", GdbInfoProc);
#endif
#if defined(Q_OS_LINUX)
    sendCommand("info proc", GdbInfoProc);
#endif
#if defined(Q_OS_MAC)
    sendCommand("info pid", GdbInfoProc, QVariant(), true);
#endif
}

void GdbEngine::stepExec()
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    emit gdbInputAvailable(QString(), QString());
    sendCommand("-exec-step", GdbExecStep);
}

void GdbEngine::stepIExec()
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    sendCommand("-exec-step-instruction", GdbExecStepI);
}

void GdbEngine::stepOutExec()
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    sendCommand("-exec-finish", GdbExecFinish);
}

void GdbEngine::nextExec()
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    emit gdbInputAvailable(QString(), QString());
    sendCommand("-exec-next", GdbExecNext);
}

void GdbEngine::nextIExec()
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    sendCommand("-exec-next-instruction", GdbExecNextI);
}

void GdbEngine::runToLineExec(const QString &fileName, int lineNumber)
{
    setTokenBarrier();
    qq->notifyInferiorRunningRequested();
    sendCommand("-exec-until " + fileName + ":" + QString::number(lineNumber));
}

void GdbEngine::runToFunctionExec(const QString &functionName)
{
    setTokenBarrier();
    sendCommand("-break-insert -t " + functionName);
    qq->notifyInferiorRunningRequested();
    sendCommand("-exec-continue", GdbExecRunToFunction);
}

void GdbEngine::jumpToLineExec(const QString &fileName, int lineNumber)
{
#if 1
    // not available everywhere?
    //sendCliCommand("tbreak " + fileName + ":" + QString::number(lineNumber));
    sendCommand("-break-insert -t " + fileName + ":" + QString::number(lineNumber));
    sendCommand("jump " + fileName + ":" + QString::number(lineNumber));
    // will produce something like
    //  &"jump /home/apoenitz/dev/work/test1/test1.cpp:242"
    //  ~"Continuing at 0x4058f3."
    //  ~"run1 (argc=1, argv=0x7fffbf1f5538) at test1.cpp:242"
    //  ~"242\t x *= 2;"
    //  23^done"
    q->gotoLocation(fileName, lineNumber, true);
    //setBreakpoint();
    //sendCommand("jump " + fileName + ":" + QString::number(lineNumber));
#else
    q->gotoLocation(fileName, lineNumber, true);
    setBreakpoint(fileName, lineNumber);
    sendCommand("jump " + fileName + ":" + QString::number(lineNumber));
#endif
}

/*!
    \fn void GdbEngine::setTokenBarrier()
    \brief Sets up internal structures to handle a new debugger turn.

    This method is called at the beginnign of all step/next/finish etc.
    debugger functions.
*/

void GdbEngine::setTokenBarrier()
{
    m_oldestAcceptableToken = currentToken();
}

void GdbEngine::setDebugDumpers(bool on)
{
    if (on) {
        qDebug() << "SWITCHING ON DUMPER DEBUGGING";
        sendCommand("set unwindonsignal off");
        q->breakByFunction("qDumpObjectData440");
        //updateLocals();
    } else {
        qDebug() << "SWITCHING OFF DUMPER DEBUGGING";
        sendCommand("set unwindonsignal on");
    }
}

//QByteArray GdbEngine::dumperChannel() const
//{
//    return m_dumperServer->serverName().toLatin1();
//    //QByteArray ba;
//    //ba.setNum(m_dumperServer->serverPort());
//    //return ba;
//}


//////////////////////////////////////////////////////////////////////
//
// Breakpoint specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::breakpointDataFromOutput(BreakpointData *data, const GdbMi &bkpt)
{
    if (!bkpt.isValid())
        return;
    if (!data)
        return;
    data->pending = false;
    data->bpMultiple = false;
    data->bpCondition.clear();
    QStringList files;
    foreach (const GdbMi &child, bkpt.children()) {
        if (child.hasName("number")) {
            data->bpNumber = child.data();
        } else if (child.hasName("func")) {
            data->bpFuncName = child.data();
        } else if (child.hasName("addr")) {
            // <MULTIPLE> happens in constructors. In this case there are
            // _two_ fields named "addr" in the response. On Linux that is...
            if (child.data() == "<MULTIPLE>")
                data->bpMultiple = true;
            else
                data->bpAddress = child.data();
        } else if (child.hasName("file")) {
            files.append(child.data());
        } else if (child.hasName("fullname")) {
            QString fullName = child.data();
            #ifdef Q_OS_WIN
            fullName = QDir::cleanPath(fullName);
            #endif
            files.prepend(fullName);
        } else if (child.hasName("line")) {
            data->bpLineNumber = child.data();
            if (child.data().toInt())
                data->markerLineNumber = child.data().toInt();
        } else if (child.hasName("cond")) {
            data->bpCondition = child.data();
            // gdb 6.3 likes to "rewrite" conditions. Just accept that fact.
            if (data->bpCondition != data->condition && data->conditionsMatch())
                data->condition = data->bpCondition;
        }
        else if (child.hasName("pending")) {
            data->pending = true;
            int pos = child.data().lastIndexOf(':');
            if (pos > 0) {
                data->bpLineNumber = child.data().mid(pos + 1);
                data->markerLineNumber = child.data().mid(pos + 1).toInt();
                files.prepend(child.data().left(pos));
            } else {
                files.prepend(child.data());
            }
        }
    }
    // This field is not present.  Contents needs to be parsed from
    // the plain "ignore" response.
    //else if (child.hasName("ignore"))
    //    data->bpIgnoreCount = child.data();

    QString name = fullName(files);
    if (data->bpFileName.isEmpty())
        data->bpFileName = name;
    if (data->markerFileName.isEmpty())
        data->markerFileName = name;
}

void GdbEngine::sendInsertBreakpoint(int index)
{
    const BreakpointData *data = qq->breakHandler()->at(index);
    QString where;
    if (data->funcName.isEmpty()) {
        where = data->fileName;
#ifdef Q_OS_MAC
        // full names do not work on Mac/MI
        QFileInfo fi(data->fileName);
        where = fi.fileName();
        //where = fi.absoluteFilePath();
#endif
#ifdef Q_OS_WIN
        // full names do not work on Mac/MI
        QFileInfo fi(data->fileName);
        where = fi.fileName();
    //where = m_manager->shortName(data->fileName);
        //if (where.isEmpty())
        //    where = data->fileName;
#endif
        // we need something like   "\"file name.cpp\":100"  to
        // survive the gdb command line parser with file names intact
        where = "\"\\\"" + where + "\\\":" + data->lineNumber + "\"";
    } else {
        where = data->funcName;
    }

    // set up fallback in case of pending breakpoints which aren't handled
    // by the MI interface
#ifdef Q_OS_LINUX
    QString cmd = "-break-insert ";
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + " ";
    cmd += where;
#endif
#ifdef Q_OS_MAC
    QString cmd = "-break-insert -l -1 ";
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + " ";
    cmd += where;
#endif
#ifdef Q_OS_WIN
    QString cmd = "-break-insert ";
    //if (!data->condition.isEmpty())
    //    cmd += "-c " + data->condition + " ";
    cmd += where;
#endif
    sendCommand(cmd, BreakInsert, index, true);
    //processQueueAndContinue();
}

void GdbEngine::handleBreakList(const GdbResultRecord &record)
{
    // 45^done,BreakpointTable={nr_rows="2",nr_cols="6",hdr=[
    // {width="3",alignment="-1",col_name="number",colhdr="Num"}, ...
    // body=[bkpt={number="1",type="breakpoint",disp="keep",enabled="y",
    //  addr="0x000000000040109e",func="main",file="app.cpp",
    //  fullname="/home/apoenitz/dev/work/plugintest/app/app.cpp",
    //  line="11",times="1"},
    //  bkpt={number="2",type="breakpoint",disp="keep",enabled="y",
    //  addr="<PENDING>",pending="plugin.cpp:7",times="0"}] ... }

    if (record.resultClass == GdbResultDone) {
        GdbMi table = record.data.findChild("BreakpointTable");
        if (table.isValid())
            handleBreakList(table);
    }
}

void GdbEngine::handleBreakList(const GdbMi &table)
{
    //qDebug() << "GdbEngine::handleOutput: table: "
    //  << table.toString();
    GdbMi body = table.findChild("body");
    //qDebug() << "GdbEngine::handleOutput: body: "
    //  << body.toString();
    QList<GdbMi> bkpts;
    if (body.isValid()) {
        // Non-Mac
        bkpts = body.children();
    } else {
        // Mac
        bkpts = table.children();
        // remove the 'hdr' and artificial items
        //qDebug() << "FOUND " << bkpts.size() << " BREAKPOINTS";
        for (int i = bkpts.size(); --i >= 0; ) {
            int num = bkpts.at(i).findChild("number").data().toInt();
            if (num <= 0) {
                //qDebug() << "REMOVING " << i << bkpts.at(i).toString();
                bkpts.removeAt(i);
            }
        }
        //qDebug() << "LEFT " << bkpts.size() << " BREAKPOINTS";
    }

    BreakHandler *handler = qq->breakHandler();
    for (int index = 0; index != bkpts.size(); ++index) {
        BreakpointData temp(handler);
        breakpointDataFromOutput(&temp, bkpts.at(index));
        int found = handler->findBreakpoint(temp);
        if (found != -1)
            breakpointDataFromOutput(handler->at(found), bkpts.at(index));
        //else
            //qDebug() << "CANNOT HANDLE RESPONSE " << bkpts.at(index).toString();
    }

    attemptBreakpointSynchronization();
    handler->updateMarkers();
}


void GdbEngine::handleBreakIgnore(const GdbResultRecord &record, int index)
{
    // gdb 6.8:
    // ignore 2 0:
    // ~"Will stop next time breakpoint 2 is reached.\n"
    // 28^done
    // ignore 2 12:
    // &"ignore 2 12\n"
    // ~"Will ignore next 12 crossings of breakpoint 2.\n"
    // 29^done
    //
    // gdb 6.3 does not produce any console output
    BreakHandler *handler = qq->breakHandler();
    if (record.resultClass == GdbResultDone && index < handler->size()) {
        QString msg = record.data.findChild("consolestreamoutput").data();
        BreakpointData *data = handler->at(index);
        //if (msg.contains("Will stop next time breakpoint")) {
        //    data->bpIgnoreCount = "0";
        //} else if (msg.contains("Will ignore next")) {
        //    data->bpIgnoreCount = data->ignoreCount;
        //}
        // FIXME: this assumes it is doing the right thing...
        data->bpIgnoreCount = data->ignoreCount;
        attemptBreakpointSynchronization();
        handler->updateMarkers();
    }
}

void GdbEngine::handleBreakCondition(const GdbResultRecord &record, int index)
{
    BreakHandler *handler = qq->breakHandler();
    if (record.resultClass == GdbResultDone) {
        // we just assume it was successful. otherwise we had to parse
        // the output stream data
        BreakpointData *data = handler->at(index);
        //qDebug() << "HANDLE BREAK CONDITION " << index << data->condition;
        data->bpCondition = data->condition;
        attemptBreakpointSynchronization();
        handler->updateMarkers();
    } else if (record.resultClass == GdbResultError) {
        QString msg = record.data.findChild("msg").data();
        // happens on Mac
        if (1 || msg.startsWith("Error parsing breakpoint condition. "
                " Will try again when we hit the breakpoint.")) {
            BreakpointData *data = handler->at(index);
            //qDebug() << "ERROR BREAK CONDITION " << index << data->condition;
            data->bpCondition = data->condition;
            attemptBreakpointSynchronization();
            handler->updateMarkers();
        }
    }
}

void GdbEngine::handleBreakInsert(const GdbResultRecord &record, int index)
{
    BreakHandler *handler = qq->breakHandler();
    if (record.resultClass == GdbResultDone) {
        //qDebug() << "HANDLE BREAK INSERT " << index;
//#ifdef Q_OS_MAC
        // interesting only on Mac?
        BreakpointData *data = handler->at(index);
        GdbMi bkpt = record.data.findChild("bkpt");
        //qDebug() << "BKPT: " << bkpt.toString() << " DATA" << data->toToolTip();
        breakpointDataFromOutput(data, bkpt);
//#endif
        attemptBreakpointSynchronization();
        handler->updateMarkers();
    } else if (record.resultClass == GdbResultError) {
        const BreakpointData *data = handler->at(index);
#ifdef Q_OS_LINUX
        //QString where = "\"\\\"" + data->fileName + "\\\":"
        //    + data->lineNumber + "\"";
        QString where = "\"" + data->fileName + "\":"
            + data->lineNumber;
        sendCommand("break " + where, BreakInsert1, index);
#endif
#ifdef Q_OS_MAC
        QFileInfo fi(data->fileName);
        QString where = "\"" + fi.fileName() + "\":"
            + data->lineNumber;
        sendCommand("break " + where, BreakInsert1, index);
#endif
#ifdef Q_OS_WIN
        QFileInfo fi(data->fileName);
        QString where = "\"" + fi.fileName() + "\":"
            + data->lineNumber;
        //QString where = m_data->fileName + QLatin1Char(':') + data->lineNumber;
        sendCommand("break " + where, BreakInsert1, index);
#endif
    }
}

void GdbEngine::extractDataFromInfoBreak(const QString &output, BreakpointData *data)
{
    data->bpFileName = "<MULTIPLE>";

    //qDebug() << output;
    if (output.isEmpty())
        return;
    // "Num     Type           Disp Enb Address            What
    // 4       breakpoint     keep y   <MULTIPLE>         0x00000000004066ad
    // 4.1                         y     0x00000000004066ad in CTorTester
    //  at /data5/dev/ide/main/tests/manual/gdbdebugger/simple/app.cpp:124
    // - or - 
    // everything on a single line on Windows for constructors of classes
    // within namespaces.
    // Sometimes the path is relative too.
  
    QRegExp re("MULTIPLE.*(0x[0-9a-f]+) in (.*)\\s+at (.*):([\\d]+)([^\\d]|$)");
    re.setMinimal(true);

    if (re.indexIn(output) != -1) {
        data->bpAddress = re.cap(1);
        data->bpFuncName = re.cap(2).trimmed();
        data->bpLineNumber = re.cap(4);
        QString full = fullName(re.cap(3));
        data->markerLineNumber = data->bpLineNumber.toInt();
        data->markerFileName = full;
        data->bpFileName = full;
        //qDebug() << "FOUND BREAKPOINT\n" << output
        //    << re.cap(1) << "\n" << re.cap(2) << "\n"
        //    << re.cap(3) << "\n" << re.cap(4) << "\n";
    } else {
        qDebug() << "COULD NOT MATCH " << re.pattern() << " AND " << output;
        data->bpNumber = "<unavailable>";
    }
}

void GdbEngine::handleBreakInfo(const GdbResultRecord &record, int bpNumber)
{
    BreakHandler *handler = qq->breakHandler();
    if (record.resultClass == GdbResultDone) {
        // Old-style output for multiple breakpoints, presumably in a
        // constructor
        int found = handler->findBreakpoint(bpNumber);
        if (found != -1) {
            QString str = record.data.findChild("consolestreamoutput").data();
            extractDataFromInfoBreak(str, handler->at(found));
            handler->updateMarkers();
            attemptBreakpointSynchronization(); // trigger "ready"
        }
    }
}

void GdbEngine::handleBreakInsert1(const GdbResultRecord &record, int index)
{
    BreakHandler *handler = qq->breakHandler();
    if (record.resultClass == GdbResultDone) {
        // Pending breakpoints in dylibs on Mac only?
        BreakpointData *data = handler->at(index);
        GdbMi bkpt = record.data.findChild("bkpt");
        breakpointDataFromOutput(data, bkpt);
        attemptBreakpointSynchronization(); // trigger "ready"
        handler->updateMarkers();
    } else if (record.resultClass == GdbResultError) {
        qDebug() << "INSERTING BREAKPOINT WITH BASE NAME FAILED. GIVING UP";
        BreakpointData *data = handler->at(index);
        data->bpNumber = "<unavailable>";
        attemptBreakpointSynchronization(); // trigger "ready"
        handler->updateMarkers();
    }
}

void GdbEngine::attemptBreakpointSynchronization()
{
    BreakHandler *handler = qq->breakHandler();
    //qDebug() << "BREAKPOINT SYNCHRONIZATION ";

    foreach (BreakpointData *data, handler->takeRemovedBreakpoints()) {
        //qDebug() << " SYNCHRONIZATION REMOVING" << data;
        QString bpNumber = data->bpNumber;
        if (!bpNumber.trimmed().isEmpty())
            sendCommand("-break-delete " + bpNumber, BreakDelete, 0, true);
        //else
        //    qDebug() << "BP HAS NO NUMBER: " << data->markerFileName;
        delete data;
    }

    bool updateNeeded = false;

    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        // multiple breakpoints?
        if (data->bpMultiple && data->bpFileName.isEmpty()) {
            sendCommand(QString("info break %1").arg(data->bpNumber),
                BreakInfo, data->bpNumber.toInt());
            updateNeeded = true;
            break;
        }
    }

    for (int index = 0; index != handler->size(); ++index) {
        BreakpointData *data = handler->at(index);
        // unset breakpoints?
        if (data->bpNumber.isEmpty()) {
            data->bpNumber = " ";
            sendInsertBreakpoint(index);
            //qDebug() << "UPDATE NEEDED BECAUSE OF UNKNOWN BREAKPOINT";
            updateNeeded = true;
            break;
        }
    }

    if (!updateNeeded) {
        for (int index = 0; index != handler->size(); ++index) {
            BreakpointData *data = handler->at(index);
            // update conditions if needed
            if (data->bpNumber.toInt() && data->condition != data->bpCondition
                   && !data->conditionsMatch()) {
                sendCommand(QString("condition %1 %2").arg(data->bpNumber)
                    .arg(data->condition), BreakCondition, index);
                //qDebug() << "UPDATE NEEDED BECAUSE OF CONDITION"
                //    << data->condition << data->bpCondition;
                updateNeeded = true;
                break;
            }
            // update ignorecount if needed
            if (data->bpNumber.toInt() && data->ignoreCount != data->bpIgnoreCount) {
                sendCommand(QString("ignore %1 %2").arg(data->bpNumber)
                    .arg(data->ignoreCount), BreakIgnore, index);
                updateNeeded = true;
                break;
            }
        }
    }

    for (int index = 0; index != handler->size(); ++index) {
        // happens sometimes on Mac. Brush over symptoms
        BreakpointData *data = handler->at(index);
        if (data->markerFileName.startsWith("../")) {
            data->markerFileName = fullName(data->markerFileName);
            handler->updateMarkers();
        }
    }

    if (updateNeeded) {
        //interruptAndContinue();
        //sendListBreakpoints();
    }

    if (!updateNeeded && q->status() == DebuggerProcessStartingUp)
        qq->notifyStartupFinished();
}


//////////////////////////////////////////////////////////////////////
//
// Disassembler specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadDisassembler()
{
    emit sendCommand("disassemble", DisassemblerList, m_address);
}

void GdbEngine::handleDisassemblerList(const GdbResultRecord &record,
    const QString &cookie)
{
    QList<DisassemblerLine> lines;
    static const QString pad = QLatin1String("    ");
    int currentLine = -1;
    if (record.resultClass == GdbResultDone) {
        QString res = record.data.findChild("consolestreamoutput").data();
        QTextStream ts(&res, QIODevice::ReadOnly);
        while (!ts.atEnd()) {
            //0x0000000000405fd8 <_ZN11QTextStreamD1Ev@plt+0>:
            //    jmpq   *2151890(%rip)    # 0x6135b0 <_GLOBAL_OFFSET_TABLE_+640>
            //0x0000000000405fde <_ZN11QTextStreamD1Ev@plt+6>:
            //    pushq  $0x4d
            //0x0000000000405fe3 <_ZN11QTextStreamD1Ev@plt+11>:
            //    jmpq   0x405af8 <_init+24>
            //0x0000000000405fe8 <_ZN9QHashData6rehashEi@plt+0>:
            //    jmpq   *2151882(%rip)    # 0x6135b8 <_GLOBAL_OFFSET_TABLE_+648>
            QString str = ts.readLine();
            if (!str.startsWith(QLatin1String("0x"))) {
                //qDebug() << "IGNORING DISASSEMBLER" << str;
                continue;
            }
            DisassemblerLine line;
            QTextStream ts(&str, QIODevice::ReadOnly);
            ts >> line.address >> line.symbol;
            line.mnemonic = ts.readLine().trimmed();
            if (line.symbol.endsWith(QLatin1Char(':')))
                line.symbol.chop(1);
            line.addressDisplay = line.address + pad;
            if (line.addressDisplay.startsWith(QLatin1String("0x00000000")))
                line.addressDisplay.replace(2, 8, QString());
            line.symbolDisplay = line.symbol + pad;

            if (line.address == cookie)
                currentLine = lines.size();

            lines.append(line);
        }
    } else {
        DisassemblerLine line;
        line.addressDisplay = tr("<could not retreive module information>");
        lines.append(line);
    }

    qq->disassemblerHandler()->setLines(lines);
    if (currentLine != -1)
        qq->disassemblerHandler()->setCurrentLine(currentLine);
}


//////////////////////////////////////////////////////////////////////
//
// Modules specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::loadSymbols(const QString &moduleName)
{
    // FIXME: gdb does not understand quoted names here (tested with 6.8)
    sendCommand("sharedlibrary " + dotEscape(moduleName));
    reloadModules();
}

void GdbEngine::loadAllSymbols()
{
    sendCommand("sharedlibrary .*");
    reloadModules();
}

void GdbEngine::reloadModules()
{
    sendCommand("info shared", ModulesList, QVariant());
}

void GdbEngine::handleModulesList(const GdbResultRecord &record)
{
    QList<Module> modules;
    if (record.resultClass == GdbResultDone) {
        QString data = record.data.findChild("consolestreamoutput").data();
        QTextStream ts(&data, QIODevice::ReadOnly);
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            if (!line.startsWith("0x"))
                continue;
            Module module;
            QString symbolsRead;
            QTextStream ts(&line, QIODevice::ReadOnly);
            ts >> module.startAddress >> module.endAddress >> symbolsRead;
            module.moduleName = ts.readLine().trimmed();
            module.symbolsRead = (symbolsRead == "Yes");
            modules.append(module);
        }
    }
    qq->modulesHandler()->setModules(modules);
}


//////////////////////////////////////////////////////////////////////
//
// Stack specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::handleStackSelectThread(const GdbResultRecord &record, int)
{
    Q_UNUSED(record);
    qDebug("FIXME: StackHandler::handleOutput: SelectThread");
    q->showStatusMessage(tr("Retrieving data for stack view..."), -1);
    sendCommand("-stack-list-frames", StackListFrames);
}


void GdbEngine::handleStackListFrames(const GdbResultRecord &record)
{
    QList<StackFrame> stackFrames;

    const GdbMi stack = record.data.findChild("stack");
    QString dummy = stack.toString();
    if (!stack.isValid()) {
        qDebug() << "FIXME: stack: " << stack.toString();
        return;
    }

    int topFrame = -1;

    for (int i = 0; i != stack.childCount(); ++i) {
        //qDebug() << "HANDLING FRAME: " << stack.childAt(i).toString();
        const GdbMi frameMi = stack.childAt(i);
        StackFrame frame;
        frame.level = i;
        QStringList files;
        files.append(frameMi.findChild("fullname").data());
        files.append(frameMi.findChild("file").data());
        frame.file = fullName(files);
        frame.function = frameMi.findChild("func").data();
        frame.from = frameMi.findChild("from").data();
        frame.line = frameMi.findChild("line").data().toInt();
        frame.address = frameMi.findChild("addr").data();

        stackFrames.append(frame);

#ifdef Q_OS_WIN
        const bool isBogus =
            // Assume this is wrong and points to some strange stl_algobase
            // implementation. Happens on Karsten's XP system with Gdb 5.50
            (frame.file.endsWith("/bits/stl_algobase.h") && frame.line == 150)
            // Also wrong. Happens on Vista with Gdb 5.50
               || (frame.function == "operator new" && frame.line == 151);

        // immediately leave bogus frames
        if (topFrame == -1 && isBogus) {
            sendCommand("-exec-finish");
            return;
        }

#endif

        // Initialize top frame to the first valid frame
        const bool isValid = !frame.file.isEmpty() && !frame.function.isEmpty();
        if (isValid && topFrame == -1)
            topFrame = i;
    }

    qq->stackHandler()->setFrames(stackFrames);

#if 0
    if (0 && topFrame != -1) {
        // updates of locals already triggered early
        const StackFrame &frame = qq->stackHandler()->currentFrame();
        bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
        if (usable)
            q->gotoLocation(frame.file, frame.line, true);
        else
            qDebug() << "FULL NAME NOT USABLE 0: " << frame.file;
    } else {
        activateFrame(topFrame);
    }
#else
    if (topFrame != -1) {
        // updates of locals already triggered early
        const StackFrame &frame = qq->stackHandler()->currentFrame();
        bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
        if (usable)
            q->gotoLocation(frame.file, frame.line, true);
        else
            qDebug() << "FULL NAME NOT USABLE 0: " << frame.file;
    }
#endif
}

void GdbEngine::selectThread(int index)
{
    //reset location arrow
    q->resetLocation();

    ThreadsHandler *threadsHandler = qq->threadsHandler();
    threadsHandler->setCurrentThread(index);

    QList<ThreadData> threads = threadsHandler->threads();
    QWB_ASSERT(index < threads.size(), return);
    int id = threads.at(index).id;
    q->showStatusMessage(tr("Retrieving data for stack view..."), -1);
    sendCommand(QLatin1String("-thread-select ") + QString::number(id),
        StackSelectThread);
}

void GdbEngine::activateFrame(int frameIndex)
{
    if (q->status() != DebuggerInferiorStopped)
        return;

    StackHandler *stackHandler = qq->stackHandler();
    int oldIndex = stackHandler->currentIndex();
    //qDebug() << "ACTIVATE FRAME: " << frameIndex << oldIndex
    //    << stackHandler->currentIndex();

    QWB_ASSERT(frameIndex < stackHandler->stackSize(), return);

    if (oldIndex != frameIndex) {
        // Assuming this always succeeds saves a roundtrip.
        // Otherwise the lines below would need to get triggered
        // after a response to this -stack-select-frame here.
        sendCommand("-stack-select-frame " + QString::number(frameIndex));

        stackHandler->setCurrentIndex(frameIndex);
        updateLocals();
    }

    const StackFrame &frame = stackHandler->currentFrame();

    bool usable = !frame.file.isEmpty() && QFileInfo(frame.file).isReadable();
    if (usable)
        q->gotoLocation(frame.file, frame.line, true);
    else
        qDebug() << "FULL NAME NOT USABLE: " << frame.file;
}

void GdbEngine::handleStackListThreads(const GdbResultRecord &record, int id)
{
    // "72^done,{thread-ids={thread-id="2",thread-id="1"},number-of-threads="2"}
    const QList<GdbMi> items = record.data.findChild("thread-ids").children();
    QList<ThreadData> threads;
    int currentIndex = -1;
    for (int index = 0, n = items.size(); index != n; ++index) {
        ThreadData thread;
        thread.id = items.at(index).data().toInt();
        threads.append(thread);
        if (thread.id == id) {
            //qDebug() << "SETTING INDEX TO: " << index << " ID: "<< id << "RECOD: "<< record.toString();
            currentIndex = index;
        }
    }
    ThreadsHandler *threadsHandler = qq->threadsHandler();
    threadsHandler->setThreads(threads);
    threadsHandler->setCurrentThread(currentIndex);
}


//////////////////////////////////////////////////////////////////////
//
// Register specific stuff
//
//////////////////////////////////////////////////////////////////////

void GdbEngine::reloadRegisters()
{
    QString format = qq->registerHandler()->model()->property(PROPERTY_REGISTER_FORMAT).toString();
    sendCommand("-data-list-register-values " + format, RegisterListValues);
}

void GdbEngine::handleRegisterListNames(const GdbResultRecord &record)
{
    if (record.resultClass != GdbResultDone)
        return;

    QList<Register> registers;
    foreach (const GdbMi &item, record.data.findChild("register-names").children())
        registers.append(Register(item.data()));

    qq->registerHandler()->setRegisters(registers);
}

void GdbEngine::handleRegisterListValues(const GdbResultRecord &record)
{
    if (record.resultClass != GdbResultDone)
        return;

    QList<Register> registers = qq->registerHandler()->registers();

    // 24^done,register-values=[{number="0",value="0xf423f"},...]
    foreach (const GdbMi &item, record.data.findChild("register-values").children()) {
        int index = item.findChild("number").data().toInt();
        if (index < registers.size()) {
            Register &reg = registers[index];
            QString value = item.findChild("value").data();
            reg.changed = (value != reg.value);
            if (reg.changed)
                reg.value = value;
        }
    }
    qq->registerHandler()->setRegisters(registers);
}


//////////////////////////////////////////////////////////////////////
//
// Thread specific stuff
//
//////////////////////////////////////////////////////////////////////

bool GdbEngine::supportsThreads() const
{
    // 6.3 crashes happily on -thread-list-ids. So don't use it.
    // The test below is a semi-random pick, 6.8 works fine
    return m_gdbVersion > 650;
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

static WatchData m_toolTip;
static QString m_toolTipExpression;
static QPoint m_toolTipPos;
static QHash<QString, WatchData> m_toolTipCache;

static bool hasLetterOrNumber(const QString &exp)
{
    for (int i = exp.size(); --i >= 0; )
        if (exp[i].isLetterOrNumber() || exp[i] == '_')
            return true;
    return false;
}

static bool hasSideEffects(const QString &exp)
{
    // FIXME: complete?
    return exp.contains("-=")
        || exp.contains("+=")
        || exp.contains("/=")
        || exp.contains("*=")
        || exp.contains("&=")
        || exp.contains("|=")
        || exp.contains("^=")
        || exp.contains("--")
        || exp.contains("++");
}

static bool isKeyWord(const QString &exp)
{
    // FIXME: incomplete
    return exp == QLatin1String("class")
        || exp == QLatin1String("const")
        || exp == QLatin1String("do")
        || exp == QLatin1String("if")
        || exp == QLatin1String("return")
        || exp == QLatin1String("struct")
        || exp == QLatin1String("template")
        || exp == QLatin1String("void")
        || exp == QLatin1String("volatile")
        || exp == QLatin1String("while");
}

void GdbEngine::setToolTipExpression(const QPoint &pos, const QString &exp0)
{
    //qDebug() << "SET TOOLTIP EXP" << pos << exp0;
    if (q->status() != DebuggerInferiorStopped) {
        //qDebug() << "SUPPRESSING DEBUGGER TOOLTIP, INFERIOR NOT STOPPED";
        return;
    }
    
    if (qq->debugDumpersAction()->isChecked()) {
        // minimize interference
        return;
    }

    m_toolTipPos = pos;
    m_toolTipExpression = exp0;
    QString exp = exp0;
/*
    if (m_toolTip.isTypePending()) {
        qDebug() << "suppressing duplicated tooltip creation";
        return;
    }
*/
    if (m_toolTipCache.contains(exp)) {
        const WatchData & data = m_toolTipCache[exp];
        // FIXME: qq->watchHandler()->collapseChildren(data.iname);
        insertData(data);
        return;
    }

    QToolTip::hideText();
    if (exp.isEmpty() || exp.startsWith("#"))  {
        QToolTip::hideText();
        return;
    }

    if (!hasLetterOrNumber(exp)) {
        QToolTip::showText(m_toolTipPos,
            "'" + exp + "' contains no identifier");
        return;
    }

    if (isKeyWord(exp))
        return;

    if (exp.startsWith('"') && exp.endsWith('"'))  {
        QToolTip::showText(m_toolTipPos, "String literal " + exp);
        return;
    }

    if (exp.startsWith("++") || exp.startsWith("--"))
        exp = exp.mid(2);

    if (exp.endsWith("++") || exp.endsWith("--"))
        exp = exp.mid(2);

    if (exp.startsWith("<") || exp.startsWith("["))
        return;

    if (hasSideEffects(exp)) {
        QToolTip::showText(m_toolTipPos,
            "Cowardly refusing to evaluate expression '" + exp
                + "' with potential side effects");
        return;
    }

    // Gdb crashes when creating a variable object with the name
    // of the type of 'this'
/*
    for (int i = 0; i != m_currentLocals.childCount(); ++i) {
        if (m_currentLocals.childAt(i).exp == "this") {
            qDebug() << "THIS IN ROW " << i;
            if (m_currentLocals.childAt(i).type.startsWith(exp)) {
                QToolTip::showText(m_toolTipPos,
                    exp + ": type of current 'this'");
                qDebug() << " TOOLTIP CRASH SUPPRESSED";
                return;
            }
            break;
        }
    }
*/

    //if (m_manager->status() != DebuggerInferiorStopped)
    //    return;

    // FIXME: 'exp' can contain illegal characters
    m_toolTip = WatchData();
    //m_toolTip.level = 0;
   // m_toolTip.row = 0;
   // m_toolTip.parentIndex = 2;
    m_toolTip.exp = exp;
    m_toolTip.name = exp;
    m_toolTip.iname = tooltipIName;
    insertData(m_toolTip);
    updateWatchModel2();
}


//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

static const QString strNotInScope = QLatin1String("<not in scope>");

static bool isPointerType(const QString &type)
{
    return type.endsWith("*") || type.endsWith("* const");
}

static bool isAccessSpecifier(const QString &str)
{
    static const QStringList items =
        QStringList() << "private" << "protected" << "public";
    return items.contains(str);
}

static bool startsWithDigit(const QString &str)
{
    return !str.isEmpty() && str[0] >= '0' && str[0] <= '9';
}

QString stripPointerType(QString type)
{
    if (type.endsWith("*"))
        type.chop(1);
    if (type.endsWith("* const"))
        type.chop(7);
    if (type.endsWith(' '))
        type.chop(1);
    return type;
}

static QString gdbQuoteTypes(const QString &type)
{
    // gdb does not understand sizeof(Core::IFile*).
    // "sizeof('Core::IFile*')" is also not acceptable,
    // it needs to be "sizeof('Core::IFile'*)"
    //
    // We never will have a perfect solution here (even if we had a full blown
    // C++ parser as we do not have information on what is a type and what is
    // a vriable name. So "a<b>::c" could either be two comparisons of values
    // 'a', 'b' and '::c', or a nested type 'c' in a template 'a<b>'. We
    // assume here it is the latter.
    //return type;

    // (*('myns::QPointer<myns::QObject>*'*)0x684060)" is not acceptable
    // (*('myns::QPointer<myns::QObject>'**)0x684060)" is acceptable
    if (isPointerType(type))
        return gdbQuoteTypes(stripPointerType(type)) + "*";

    QString accu;
    QString result;
    int templateLevel = 0;
    for (int i = 0; i != type.size(); ++i) {
        QChar c = type.at(i);
        if (c.isLetterOrNumber() || c == '_' || c == ':' || c == ' ') {
            accu += c;
        } else if (c == '<') {
            ++templateLevel;
            accu += c;
        } else if (c == '<') {
            --templateLevel;
            accu += c;
        } else if (templateLevel > 0) {
            accu += c;
        } else {
            if (accu.contains(':') || accu.contains('<'))
                result += '\'' + accu + '\'';
            else
                result += accu;
            accu.clear();
            result += c;
        }
    }
    if (accu.contains(':') || accu.contains('<'))
        result += '\'' + accu + '\'';
    else
        result += accu;
    //qDebug() << "GDB_QUOTING" << type << " TO " << result;

    return result;
}

static void setWatchDataValue(WatchData &data, const GdbMi &mi,
    int encoding = 0)
{
    if (mi.isValid()) {
        QByteArray ba;
        switch (encoding) {
            case 0: // unencoded 8 bit data
                ba = mi.data();
                break;
            case 1: //  base64 encoded 8 bit data
                ba = QByteArray::fromBase64(mi.data());
                break;
            case 2: //  base64 encoded 16 bit data
                ba = QByteArray::fromBase64(mi.data());
                ba = QString::fromUtf16((ushort *)ba.data(), ba.size() / 2).toUtf8();
                break;
            case 3: //  base64 encoded 32 bit data
                ba = QByteArray::fromBase64(mi.data());
                ba = QString::fromUcs4((uint *)ba.data(), ba.size() / 4).toUtf8();
                break;
        }
       data.setValue(ba);
    } else {
       data.setValueNeeded();
    }
}

static void setWatchDataEditValue(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.editvalue = mi.data();
}

static void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.setValueToolTip(mi.data());
}

static void setWatchDataChildCount(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        data.childCount = mi.data().toInt();
        data.setChildCountUnneeded();
        if (data.childCount == 0)
            data.setChildrenUnneeded();
    } else {
        data.childCount = -1;
    }
}

static void setWatchDataValueDisabled(WatchData &data, const GdbMi &mi)
{
    if (mi.data() == "true")
        data.valuedisabled = true;
    else if (mi.data() == "false")
        data.valuedisabled = false;
}

static void setWatchDataExpression(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid())
        data.exp = "(" + mi.data() + ")";
}

static void setWatchDataAddress(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        data.addr = mi.data();
        if (data.exp.isEmpty())
            data.exp = "(*(" + gdbQuoteTypes(data.type) + "*)" + data.addr + ")";
    }
}

static bool extractTemplate(const QString &type, QString *tmplate, QString *inner)
{
    // Input "Template<Inner1,Inner2,...>::Foo" will return "Template::Foo" in
    // 'tmplate' and "Inner1@Inner2@..." etc in 'inner'. Result indicates
    // whether parsing was successful
    int level = 0;
    for (int i = 0; i != type.size(); ++i) {
        QChar c = type[i];
        if (c == '<') {
            *(level == 0 ? tmplate : inner) += c;
            ++level;
        } else if (c == '>') {
            --level;
            *(level == 0 ? tmplate : inner) += c;
        } else if (c == ',') {
            *inner += (level == 1) ? '@' : ',';
        } else {
            *(level == 0 ? tmplate : inner) += c;
        }
    }
    *tmplate = tmplate->trimmed();
    *tmplate = tmplate->remove("<>");
    *inner = inner->trimmed();
    //qDebug() << "EXTRACT TEMPLATE: " << *tmplate << *inner;
    return !inner->isEmpty();
}

static QString extractTypeFromPTypeOutput(const QString &str)
{
    int pos0 = str.indexOf('=');
    int pos1 = str.indexOf('{');
    int pos2 = str.lastIndexOf('}');
    QString res = str;
    if (pos0 != -1 && pos1 != -1 && pos2 != -1)
        res = str.mid(pos0 + 2, pos1 - 1 - pos0)
            + " ... " + str.right(str.size() - pos2);
    return res.simplified();
}

static bool isIntOrFloatType(const QString &type)
{
    static const QStringList types = QStringList()
        << "char" << "int" << "short" << "float" << "double" << "long"
        << "bool" << "signed char" << "unsigned" << "unsigned char"
        << "unsigned int" << "unsigned long" << "long long"
        << "unsigned long long";
    return types.contains(type);
}

static QString sizeofTypeExpression(const QString &type)
{
    if (type.endsWith('*'))
        return "sizeof(void*)";
    if (type.endsWith('>'))
        return "sizeof(" + type + ")";
    return "sizeof(" + gdbQuoteTypes(type) + ")";
}

void GdbEngine::setCustomDumpersWanted(bool on)
{
    Q_UNUSED(on);
    // FIXME: a bit too harsh, but otherwise the treeview
    // sometimes look funny
    //m_expandedINames.clear();
    updateLocals();
}

bool GdbEngine::isCustomValueDumperAvailable(const QString &type) const
{
    if (!qq->useCustomDumpers())
        return false;
    if (qq->debugDumpersAction()->isChecked()
            && qq->stackHandler()->isDebuggingDumpers())
        return false;
    if (m_dataDumperState != DataDumperAvailable)
        return false;
    if (m_availableSimpleDumpers.contains(type))
        return true;

    QString tmplate;
    QString inner;
    if (extractTemplate(type, &tmplate, &inner)) {
        if (type.startsWith(m_namespace)) {
            tmplate = tmplate.mid(m_namespace.size());
            if (tmplate == "QList")
                return true;
            if (tmplate == "QVector")
                return true;
            if (tmplate == "QHash")
                return true;
            if (tmplate == "QHashNode")
                return true;
            if (tmplate == "QMap")
                return true;
            if (tmplate == "QMapNode")
                return true;
            if (tmplate == "QSet")
                return true;
        }
        if (tmplate == "std::vector" && inner != "bool")
            return true;
        if (tmplate == "std::basic_string") {
            if (inner.startsWith("char@") || inner.startsWith("wchar_t@"))
                return true;
        }
    }

    return false;
}

void GdbEngine::runCustomDumper(const WatchData & data0, bool dumpChildren)
{
    WatchData data = data0;
    QWB_ASSERT(!data.exp.isEmpty(), return);
    QString tmplate;
    QString inner;
    bool isTemplate = extractTemplate(data.type, &tmplate, &inner);
    QStringList inners = inner.split('@');
    if (inners.at(0).isEmpty())
        inners.clear();

    QString outertype = isTemplate ? tmplate : data.type;

    if (outertype == "QWidget")
        outertype = "QObject";

    QString extraArgs[4];
    extraArgs[0] = "0";
    extraArgs[1] = "0";
    extraArgs[2] = "0";
    extraArgs[3] = "0";
    int extraArgCount = 0;

    // "generic" template dumpers: passing sizeof(argument)
    // gives already most information the dumpers need
    foreach (const QString &arg, inners)
        extraArgs[extraArgCount++] = sizeofTypeExpression(arg);

    // in rare cases we need more or less:
    if (outertype == m_namespace + "QObject") {
        extraArgs[extraArgCount++] = "(char*)&((('"
            + m_namespace + "QObjectPrivate'*)&"
            + data.exp + ")->children)-(char*)&" + data.exp;
    } else if (outertype == m_namespace + "QVector") {
        extraArgs[extraArgCount++] = "(char*)&(("
            + data.exp + ").d->array)-(char*)" + data.exp + ".d";
    } else if (outertype == m_namespace + "QObjectSlot"
            || outertype == m_namespace + "QObjectSignal") {
        // we need the number out of something like
        // iname="local.ob.slots.[2]deleteLater()"
        int lastOpened = data.iname.lastIndexOf('[');
        int lastClosed = data.iname.lastIndexOf(']');
        QString slotNumber = "-1";
        if (lastOpened != -1 && lastClosed != -1)
            slotNumber = data.iname.mid(lastOpened + 1, lastClosed - lastOpened - 1);
        extraArgs[extraArgCount++] = slotNumber;
    } else if (outertype == m_namespace + "QMap") {
        QString nodetype = m_namespace + "QMapNode";
        nodetype += data.type.mid(m_namespace.size() + 4);
        //qDebug() << "OUTERTYPE: " << outertype << " NODETYPE: " << nodetype;
        extraArgs[extraArgCount++] = sizeofTypeExpression(nodetype);
        extraArgs[extraArgCount++] = "(size_t)&(('" + nodetype + "'*)0)->value";
    } else if (outertype == m_namespace + "QMapNode") {
        extraArgs[extraArgCount++] = sizeofTypeExpression(data.type);
        extraArgs[extraArgCount++] = "(size_t)&(('" + data.type + "'*)0)->value";
    } else if (outertype == "std::vector") {
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "bool") {
            outertype = "std::vector::bool";
        } else {
            //extraArgs[extraArgCount++] = sizeofTypeExpression(data.type);
            //extraArgs[extraArgCount++] = "(size_t)&(('" + data.type + "'*)0)->value";
        }
    } else if (outertype == "std::basic_string") {
        //qDebug() << "EXTRACT TEMPLATE: " << outertype << inners;
        if (inners.at(0) == "char") {
            outertype = "std::string";
        } else if (inners.at(0) == "wchar_t") {
            outertype = "std::wstring";
        }
        extraArgs[0] = "0";
        extraArgs[1] = "0";
        extraArgs[2] = "0";
        extraArgs[3] = "0";
    }

    //int protocol = (data.iname.startsWith("watch") && data.type == "QImage") ? 3 : 2;
    //int protocol = data.iname.startsWith("watch") ? 3 : 2;
    int protocol = 2;
    //int protocol = isDisplayedIName(data.iname) ? 3 : 2;

    QString addr;
    if (data.addr.startsWith("0x"))
        addr = "(void*)" + data.addr;
    else
        addr = "&(" + data.exp + ")";

    QByteArray params;
    params.append(outertype);
    params.append('\0');
    params.append(data.iname);
    params.append('\0');
    params.append(data.exp);
    params.append('\0');
    params.append(inner);
    params.append('\0');
    params.append(data.iname);
    params.append('\0');

    sendWatchParameters(params);

    QString cmd ="call "
            + QString("qDumpObjectData440(")
            + QString::number(protocol)
            + ',' + "%1+1"                // placeholder for token
            + ',' + addr
            + ',' + (dumpChildren ? "1" : "0")
            + ',' + extraArgs[0]
            + ',' + extraArgs[1]
            + ',' + extraArgs[2]
            + ',' + extraArgs[3] + ')';

    sendSynchronizedCommand(cmd, WatchDumpCustomValue1, QVariant::fromValue(data));

    q->showStatusMessage(
        tr("Retrieving data for watch view (%1 requests pending)...")
            .arg(m_pendingRequests + 1), -1);
    // create response slot for socket data
    QVariant var;
    var.setValue(data);
    sendSynchronizedCommand(QString(), WatchDumpCustomValue2, var);

    // this increases the probability that gdb spits out output it
    // has collected so far
    //sendCommand("p qDumpInBuffer");
}

void GdbEngine::createGdbVariable(const WatchData &data)
{
    sendSynchronizedCommand("-var-delete \"" + data.iname + '"');
    QString exp = data.exp;
    if (exp.isEmpty() && data.addr.startsWith("0x"))
        exp = "*(" + gdbQuoteTypes(data.type) + "*)" + data.addr;
    QVariant val = QVariant::fromValue<WatchData>(data);
    sendSynchronizedCommand("-var-create \"" + data.iname + '"' + " * "
        + '"' + exp + '"', WatchVarCreate, val);
}

void GdbEngine::updateSubItem(const WatchData &data0)
{
    WatchData data = data0;
    #if DEBUG_SUBITEM
    qDebug() << "UPDATE SUBITEM: " << data.toString();
    #endif
    QWB_ASSERT(data.isValid(), return);

    // in any case we need the type first
    if (data.isTypeNeeded()) {
        // This should only happen if we don't have a variable yet.
        // Let's play safe, though.
        if (!data.variable.isEmpty()) {
            // Update: It does so for out-of-scope watchers.
            #if 1
            qDebug() << "FIXME: GdbEngine::updateSubItem: "
                 << data.toString() << "should not happen";
            #else
            data.setType("<out of scope>");
            data.setValue("<out of scope>");
            data.setChildCount(0);
            insertData(data);
            return;
            #endif
        }
        // The WatchVarCreate handler will receive type information
        // and re-insert a WatchData item with correct type, so
        // we will not re-enter this bit.
        // FIXME: Concurrency issues?
        createGdbVariable(data);
        return;
    }

    // we should have a type now. this is relied upon further below
    QWB_ASSERT(!data.type.isEmpty(), return);

    // a common case that can be easily solved
    if (data.isChildrenNeeded() && isPointerType(data.type)
        && !isCustomValueDumperAvailable(data.type)) {
        // We sometimes know what kind of children pointers have
        #if DEBUG_SUBITEM
        qDebug() << "IT'S A POINTER";
        #endif
#if 1
        WatchData data1;
        data1.iname = data.iname + ".*";
        data1.name = "*" + data.name;
        data1.exp = "(*(" + data.exp + "))";
        data1.type = stripPointerType(data.type);
        data1.setValueNeeded();
        insertData(data1);
        data.setChildrenUnneeded();
        insertData(data);
#else
        // Try automatic dereferentiation
        data.exp = "*(" + data.exp + ")";
        data.type = data.type + "."; // FIXME: fragile HACK to avoid recursion
        insertData(data);
#endif
        return;
    }

    if (data.isValueNeeded() && isCustomValueDumperAvailable(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE";
        #endif
        runCustomDumper(data, qq->watchHandler()->isExpandedIName(data.iname));
        return;
    }

/*
    if (data.isValueNeeded() && data.exp.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: NO EXPRESSION?";
        #endif
        data.setError("<no expression given>");
        insertData(data);
        return;
    }
*/

    if (data.isValueNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR VALUE";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with valueNeeded() set.
        return;
    }

    if (data.isValueNeeded()) {
        QWB_ASSERT(!data.variable.isEmpty(), return); // tested above
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VALUE";
        #endif
        QString cmd = "-var-evaluate-expression \"" + data.iname + "\"";
        sendSynchronizedCommand(cmd, WatchEvaluateExpression,
            QVariant::fromValue(data));
        return;
    }

    if (data.isChildrenNeeded() && isCustomValueDumperAvailable(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runCustomDumper(data, true);
        return;
    }

    if (data.isChildrenNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDREN";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }

    if (data.isChildrenNeeded()) {
        QWB_ASSERT(!data.variable.isEmpty(), return); // tested above
        QString cmd = "-var-list-children --all-values \"" + data.variable + "\"";
        sendSynchronizedCommand(cmd, WatchVarListChildren, QVariant::fromValue(data));
        return;
    }

    if (data.isChildCountNeeded() && isCustomValueDumperAvailable(data.type)) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: CUSTOMVALUE WITH CHILDREN";
        #endif
        runCustomDumper(data, qq->watchHandler()->isExpandedIName(data.iname));
        return;
    }

    if (data.isChildCountNeeded() && data.variable.isEmpty()) {
        #if DEBUG_SUBITEM
        qDebug() << "UPDATE SUBITEM: VARIABLE NEEDED FOR CHILDCOUNT";
        #endif
        createGdbVariable(data);
        // the WatchVarCreate handler will re-insert a WatchData
        // item, with childrenNeeded() set.
        return;
    }

    if (data.isChildCountNeeded()) {
        QWB_ASSERT(!data.variable.isEmpty(), return); // tested above
        QString cmd = "-var-list-children --all-values \"" + data.variable + "\"";
        sendCommand(cmd, WatchVarListChildren, QVariant::fromValue(data));
        return;
    }

    qDebug() << "FIXME: UPDATE SUBITEM: " << data.toString();
    QWB_ASSERT(false, return);
}

void GdbEngine::updateWatchModel()
{
    m_pendingRequests = 0;
    PENDING_DEBUG("EXTERNAL TRIGGERING UPDATE WATCH MODEL");
    updateWatchModel2();
}

void GdbEngine::updateWatchModel2()
{
    PENDING_DEBUG("UPDATE WATCH MODEL");
    QList<WatchData> incomplete = qq->watchHandler()->takeCurrentIncompletes();
    //QWB_ASSERT(incomplete.isEmpty(), /**/);
    if (!incomplete.isEmpty()) {
        #if DEBUG_PENDING
        qDebug() << "##############################################";
        qDebug() << "UPDATE MODEL, FOUND INCOMPLETES:";
        foreach (const WatchData &data, incomplete)
            qDebug() << data.toString();
        #endif

        // Bump requests to avoid model rebuilding during the nested
        // updateWatchModel runs.
        ++m_pendingRequests;
        foreach (const WatchData &data, incomplete)
            updateSubItem(data);
        PENDING_DEBUG("INTERNAL TRIGGERING UPDATE WATCH MODEL");
        updateWatchModel2();
        --m_pendingRequests;

        return;
    }

    if (m_pendingRequests > 0) {
        PENDING_DEBUG("UPDATE MODEL, PENDING: " << m_pendingRequests);
        return;
    }

    PENDING_DEBUG("REBUILDING MODEL")
    emit gdbInputAvailable(QString(),
        "[" + currentTime() + "]    <Rebuild Watchmodel>");
    q->showStatusMessage(tr("Finished retrieving data."), -1);
    qq->watchHandler()->rebuildModel();

    if (!m_toolTipExpression.isEmpty()) {
        WatchData *data = qq->watchHandler()->findData(tooltipIName);
        if (data) {
            //m_toolTipCache[data->exp] = *data;
            QToolTip::showText(m_toolTipPos,
                    "(" + data->type + ") " + data->exp + " = " + data->value);
        } else {
            QToolTip::showText(m_toolTipPos,
                "Cannot evaluate expression: " + m_toolTipExpression);
        }
    }

    //qDebug() << "INSERT DATA" << data0.toString();
    //q->showStatusMessage(tr("Stopped."), 5000);
}

void GdbEngine::handleQueryDataDumper1(const GdbResultRecord &record)
{
    Q_UNUSED(record);
}

void GdbEngine::handleQueryDataDumper2(const GdbResultRecord &record)
{
    // is this the official gdb response. However, it won't contain
    // interesting data other than the information that 'real' data
    // either already arrived or is still in the pipe. So we do
    // _not_ register this result for counting purposes, this will
    // be done by the 'real' result (with resultClass == GdbResultCustomDone)
    //qDebug() << "DATA DUMPER TRIAL:" << record.toString();
    GdbMi output = record.data.findChild("customvaluecontents");
    GdbMi contents(output.data());
    GdbMi simple = contents.findChild("simpledumpers");
    m_namespace = contents.findChild("namespace").data();
    //qDebug() << "OUTPUT: " << output.toString();
    //qDebug() << "CONTENTS: " << contents.toString();
    //qDebug() << "SIMPLE DUMPERS: " << simple.toString();
    m_availableSimpleDumpers.clear();
    foreach (const GdbMi &item, simple.children())
        m_availableSimpleDumpers.append(item.data());
    if (m_availableSimpleDumpers.isEmpty()) {
        m_dataDumperState = DataDumperUnavailable;
        QMessageBox::warning(q->mainWindow(),
            tr("Cannot find special data dumpers"),
            tr("The debugged binary does not contain information needed for "
                    "nice display of Qt data types.\n\n"
                    "Try might want to try include the file\n\n"
                    ".../ide/main/bin/gdbmacros/gdbmacros.cpp'\n\n"
                    "into your project directly.")
                );
    } else {
        m_dataDumperState = DataDumperAvailable;
    }
    //qDebug() << "DATA DUMPERS AVAILABLE" << m_availableSimpleDumpers;
}

void GdbEngine::sendWatchParameters(const QByteArray &params0)
{
    QByteArray params = params0;
    params.append('\0');
    char buf[50];
    sprintf(buf, "set {char[%d]} qDumpInBuffer = {", params.size());
    QByteArray encoded;
    encoded.append(buf);
    for (int i = 0; i != params.size(); ++i) {
        sprintf(buf, "%d,", int(params[i]));
        encoded.append(buf);
    }
    encoded[encoded.size() - 1] = '}';

    sendCommand(encoded);
}

void GdbEngine::handleVarAssign()
{
    // everything might have changed, force re-evaluation
    // FIXME: Speed this up by re-using variables and only
    // marking values as 'unknown'
    updateLocals();
}

void GdbEngine::setWatchDataType(WatchData &data, const GdbMi &mi)
{
    if (mi.isValid()) {
        if (!data.framekey.isEmpty())
            m_varToType[data.framekey] = mi.data();
        data.setType(mi.data());
    } else if (data.type.isEmpty()) {
        data.setTypeNeeded();
    }
}

void GdbEngine::handleVarCreate(const GdbResultRecord &record,
    const WatchData &data0)
{
    WatchData data = data0;
    // happens e.g. when we already issued a var-evaluate command
    if (!data.isValid())
        return;
    //qDebug() << "HANDLE VARIABLE CREATION: " << data.toString();
    if (record.resultClass == GdbResultDone) {
        data.variable = data.iname;
        setWatchDataType(data, record.data.findChild("type"));
        if (isCustomValueDumperAvailable(data.type)) {
            // we do not trust gdb if we have a custom dumper
            if (record.data.findChild("children").isValid())
                data.setChildrenUnneeded();
            else if (qq->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenNeeded();
            insertData(data);
        } else {
            if (record.data.findChild("children").isValid())
                data.setChildrenUnneeded();
            else if (qq->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenNeeded();
            setWatchDataChildCount(data, record.data.findChild("numchild"));
            //if (data.isValueNeeded() && data.childCount > 0)
            //    data.setValue(QByteArray());
            insertData(data);
        }
    } else if (record.resultClass == GdbResultError) {
        data.setError(record.data.findChild("msg").data());
        if (data.isWatcher()) {
            data.value = strNotInScope;
            data.type = " ";
            data.setAllUnneeded();
            data.setChildCount(0);
            data.valuedisabled = true;
            insertData(data);
        }
    }
}

void GdbEngine::handleEvaluateExpression(const GdbResultRecord &record,
    const WatchData &data0)
{
    WatchData data = data0;
    QWB_ASSERT(data.isValid(), qDebug() << "HUH?");
    if (record.resultClass == GdbResultDone) {
        //if (col == 0)
        //    data.name = record.data.findChild("value").data();
        //else
            setWatchDataValue(data, record.data.findChild("value"));
    } else if (record.resultClass == GdbResultError) {
        data.setError(record.data.findChild("msg").data());
    }
    //qDebug() << "HANDLE EVALUATE EXPRESSION: " << data.toString();
    insertData(data);
    //updateWatchModel2();
}

void GdbEngine::handleDumpCustomSetup(const GdbResultRecord &record)
{
    qDebug() << "CUSTOM SETUP RESULT: " << record.toString();
    if (record.resultClass == GdbResultDone) {
    } else if (record.resultClass == GdbResultError) {
        QString msg = record.data.findChild("msg").data();
        qDebug() << "CUSTOM DUMPER SETUP ERROR MESSAGE: " << msg;
    }
}

void GdbEngine::handleDumpCustomValue1(const GdbResultRecord &record,
    const WatchData &data0)
{
    WatchData data = data0;
    QWB_ASSERT(data.isValid(), return);
    if (record.resultClass == GdbResultDone) {
        // ignore this case, data will follow
    } else if (record.resultClass == GdbResultError) {
        // Record an extra result, as the socket result will be lost
        // in transmission
        --m_pendingRequests;
        QString msg = record.data.findChild("msg").data();
        //qDebug() << "CUSTOM DUMPER ERROR MESSAGE: " << msg;
#ifdef QT_DEBUG
        // Make debugging of dumers easier
        if (qq->debugDumpersAction()->isChecked()
                && msg.startsWith("The program being debugged stopped while")
                && msg.contains("qDumpObjectData440")) {
            // Fake full stop
            sendCommand("-file-list-exec-source-files", GdbQuerySources);
            sendCommand("-break-list", BreakList);
            sendCommand("p 0", GdbAsyncOutput2);  // dummy
            return;
        }
#endif
        if (msg.startsWith("The program being debugged was sig"))
            msg = strNotInScope;
        if (msg.startsWith("The program being debugged stopped while"))
            msg = strNotInScope;
        data.setError(msg);
        insertData(data);
    }
}

void GdbEngine::handleDumpCustomValue2(const GdbResultRecord &record,
    const WatchData &data0)
{
    WatchData data = data0;
    QWB_ASSERT(data.isValid(), return);
    //qDebug() << "CUSTOM VALUE RESULT: " << record.toString();
    //qDebug() << "FOR DATA: " << data.toString() << record.resultClass;
    if (record.resultClass == GdbResultDone) {
        GdbMi output = record.data.findChild("customvaluecontents");
        //qDebug() << "HANDLE VALUE CONTENTS: " << output.toString(true);
        if (!output.isValid()) {
             //qDebug() << "INVALID";
             // custom dumper produced no output
             if (data.isValueNeeded())
                 data.setValue("<unknown>");
             if (data.isTypeNeeded())
                 data.setType("<unknown>");
             if (data.isChildrenNeeded())
                 data.setChildCount(0);
             if (data.isChildCountNeeded())
                 data.setChildCount(0);
             data.setValueToolTip("<custom dumper produced no output>");
             insertData(data);
        } else {
            GdbMi contents;
            //qDebug() << "OUTPUT" << output.toString(true);
            contents.fromString(output.data());
            //qDebug() << "CONTENTS" << contents.toString(true);
            setWatchDataType(data, contents.findChild("type"));
            setWatchDataValue(data, contents.findChild("value"),
                contents.findChild("valueencoded").data().toInt());
            setWatchDataAddress(data, contents.findChild("addr"));
            setWatchDataChildCount(data, contents.findChild("numchild"));
            setWatchDataValueToolTip(data, contents.findChild("valuetooltip"));
            setWatchDataValueDisabled(data, contents.findChild("valuedisabled"));
            setWatchDataEditValue(data, contents.findChild("editvalue"));
            if (qq->watchHandler()->isDisplayedIName(data.iname)) {
                GdbMi editvalue = contents.findChild("editvalue");
                if (editvalue.isValid()) {
                    setWatchDataEditValue(data, editvalue);
                    qq->watchHandler()->showEditValue(data);
                }
            }
            if (!qq->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenUnneeded();
            GdbMi children = contents.findChild("children");
            if (children.isValid() || !qq->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenUnneeded();
            data.setValueUnneeded();

            // try not to repeat data too often
            WatchData childtemplate;
            setWatchDataType(childtemplate, contents.findChild("childtype"));
            setWatchDataChildCount(childtemplate, contents.findChild("childnumchild"));
            //qDebug() << "DATA: " << data.toString();
            insertData(data);
            foreach (GdbMi item, children.children()) {
                WatchData data1 = childtemplate;
                data1.name = item.findChild("name").data();
                data1.iname = data.iname + "." + data1.name;
                //qDebug() << "NAMEENCODED: " << item.findChild("nameencoded").data()
                //    << item.findChild("nameencoded").data()[1];
                if (item.findChild("nameencoded").data()[0] == '1')
                    data1.name = QByteArray::fromBase64(data1.name.toUtf8());
                if (item.findChild("nameisindex").data()[0] == '1')
                    data1.name = '[' + data1.name + ']';
                setWatchDataType(data1, item.findChild("type"));
                setWatchDataExpression(data1, item.findChild("exp"));
                setWatchDataChildCount(data1, item.findChild("numchild"));
                setWatchDataValue(data1, item.findChild("value"),
                    item.findChild("valueencoded").data().toInt());
                setWatchDataAddress(data1, item.findChild("addr"));
                setWatchDataValueToolTip(data1, item.findChild("valuetooltip"));
                setWatchDataValueDisabled(data1, item.findChild("valuedisabled"));
                if (!qq->watchHandler()->isExpandedIName(data1.iname))
                    data1.setChildrenUnneeded();
                //qDebug() << "HANDLE CUSTOM SUBCONTENTS:" << data1.toString();
                insertData(data1);
            }
        }
        //qDebug() << "HANDLE CUSTOM VALUE CONTENTS: " << data.toString();
    } else if (record.resultClass == GdbResultError) {
        // FIXME: Should not happen here, i.e. could be removed
        QString msg = record.data.findChild("msg").data();
        //qDebug() << "CUSTOM DUMPER ERROR MESSAGE: " << msg;
        if (msg.startsWith("The program being debugged was sig"))
            msg = strNotInScope;
        if (msg.startsWith("The program being debugged stopped while"))
            msg = strNotInScope;
        data.setError(msg);
        insertData(data);
    } else {
        qDebug() << "STRANGE CUSTOM DUMPER RESULT DATA: " << data.toString();
    }
}

void GdbEngine::updateLocals()
{
    setTokenBarrier();

    m_pendingRequests = 0;
    PENDING_DEBUG("\nRESET PENDING");
    m_toolTipCache.clear();
    m_toolTipExpression.clear();
    qq->watchHandler()->reinitializeWatchers();

    int level = currentFrame();
    // '2' is 'list with type and value'
    QString cmd = QString("-stack-list-arguments 2 %1 %2").arg(level).arg(level);
    sendSynchronizedCommand(cmd, StackListArguments);                 // stage 1/2
    // '2' is 'list with type and value'
    sendSynchronizedCommand("-stack-list-locals 2", StackListLocals); // stage 2/2
}

void GdbEngine::handleStackListArguments(const GdbResultRecord &record)
{
    // stage 1/2

    // Linux:
    // 12^done,stack-args=
    //   [frame={level="0",args=[
    //     {name="argc",type="int",value="1"},
    //     {name="argv",type="char **",value="(char **) 0x7..."}]}]
    // Mac:
    // 78^done,stack-args=
    //    {frame={level="0",args={
    //      varobj=
    //        {exp="this",value="0x38a2fab0",name="var21",numchild="3",
    //             type="CurrentDocumentFind *  const",typecode="PTR",
    //             dynamic_type="",in_scope="true",block_start_addr="0x3938e946",
    //             block_end_addr="0x3938eb2d"},
    //      varobj=
    //         {exp="before",value="@0xbfffb9f8: {d = 0x3a7f2a70}",
    //              name="var22",numchild="1",type="const QString  ...} }}}
    //
    // In both cases, iterating over the children of stack-args/frame/args
    // is ok.
    m_currentFunctionArgs.clear();
    if (record.resultClass == GdbResultDone) {
        const GdbMi list = record.data.findChild("stack-args");
        const GdbMi frame = list.findChild("frame");
        const GdbMi args = frame.findChild("args");
        m_currentFunctionArgs = args.children();
    } else if (record.resultClass == GdbResultError) {
        qDebug() << "FIXME: GdbEngine::handleStackListArguments: should not happen";
    }
}

void GdbEngine::handleStackListLocals(const GdbResultRecord &record)
{
    // stage 2/2

    // There could be shadowed variables
    QHash<QString, int> seen;
    QList<GdbMi> locals = record.data.findChild("locals").children();
    locals += m_currentFunctionArgs;

    //qDebug() << m_varToType;

    foreach (const GdbMi &item, locals) {
        #ifdef Q_OS_MAC
            QString name = item.findChild("exp").data();
        #else
            QString name = item.findChild("name").data();
        #endif
        int n = seen.value(name);
        if (n) {
            seen[name] = n + 1;
            WatchData data;
            data.iname = "local." + name + QString::number(n + 1);
            data.name = name + QString(" <shadowed %1>").arg(n);
            //data.setValue("<shadowed>");
            setWatchDataValue(data, item.findChild("value"));
            data.setType("<shadowed>");
            data.setChildCount(0);
            insertData(data);
        } else {
            seen[name] = 1;
            WatchData data;
            data.iname = "local." + name;
            data.name = name;
            data.exp = name;
            data.framekey = m_currentFrame + data.name;
            setWatchDataType(data, item.findChild("type"));
            // set value only directly if it is simple enough, otherwise
            // pass through the insertData() machinery
            if (isIntOrFloatType(data.type) || isPointerType(data.type))
                setWatchDataValue(data, item.findChild("value"));
            if (!qq->watchHandler()->isExpandedIName(data.iname))
                data.setChildrenUnneeded();
            if (isPointerType(data.type) || data.name == "this")
                data.setChildCount(1);
            if (0 && m_varToType.contains(data.framekey)) {
                qDebug() << "RE-USING " << m_varToType.value(data.framekey);
                data.setType(m_varToType.value(data.framekey));
            }
            insertData(data);
        }
    }
}

void GdbEngine::insertData(const WatchData &data0)
{
    //qDebug() << "INSERT DATA" << data0.toString();
    WatchData data = data0;
    if (data.value.startsWith("mi_cmd_var_create:")) {
        qDebug() << "BOGUS VALUE: " << data.toString();
        return;
    }
    qq->watchHandler()->insertData(data);
}

void GdbEngine::handleTypeContents(const QString &output)
{
    // output.startsWith("type = ") == true
    // "type = int"
    // "type = class QString {"
    // "type = class QStringList : public QList<QString> {"
    QString tip;
    QString className;
    if (output.startsWith("type = class")) {
        int posBrace = output.indexOf('{');
        QString head = output.mid(13, posBrace - 13 - 1);
        int posColon = head.indexOf(": public");
        if (posColon == -1)
            posColon = head.indexOf(": protected");
        if (posColon == -1)
            posColon = head.indexOf(": private");
        if (posColon == -1) {
            className = head;
            tip = "class " + className + " { ... }";
        } else {
            className = head.left(posColon - 1);
            tip = "class " + head + " { ... }";
        }
        //qDebug() << "posColon: " << posColon;
        //qDebug() << "posBrace: " << posBrace;
        //qDebug() << "head: " << head;
    } else {
        className = output.mid(7);
        tip = className;
    }
    //qDebug() << "output: " << output.left(100) + "...";
    //qDebug() << "className: " << className;
    //qDebug() << "tip: " << tip;
    //m_toolTip.type = className;
    m_toolTip.type.clear();
    m_toolTip.value = tip;
}

void GdbEngine::handleVarListChildrenHelper(const GdbMi &item,
    const WatchData &parent)
{
    //qDebug() <<  "VAR_LIST_CHILDREN: PARENT 2" << parent.toString();
    //qDebug() <<  "VAR_LIST_CHILDREN: APPENDEE " << data.toString();
    QByteArray exp = item.findChild("exp").data();
    QByteArray name = item.findChild("name").data();
    if (isAccessSpecifier(exp)) {
        // suppress 'private'/'protected'/'public' level
        WatchData data;
        data.variable = name;
        data.iname = parent.iname;
        //data.iname = data.variable;
        data.exp = parent.exp;
        data.setTypeUnneeded();
        data.setValueUnneeded();
        data.setChildCountUnneeded();
        data.setChildrenUnneeded();
        //qDebug() << "DATA" << data.toString();
        QString cmd = "-var-list-children --all-values \"" + data.variable + "\"";
        //iname += '.' + exp;
        sendSynchronizedCommand(cmd, WatchVarListChildren, QVariant::fromValue(data));
    } else if (item.findChild("numchild").data() == "0") {
        // happens for structs without data, e.g. interfaces.
        WatchData data;
        data.iname = parent.iname + '.' + exp;
        data.name = exp;
        data.variable = name;
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item.findChild("value"));
        setWatchDataAddress(data, item.findChild("addr"));
        data.setChildCount(0);
        insertData(data);
    } else if (parent.iname.endsWith('.')) {
        // Happens with anonymous unions
        WatchData data;
        data.iname = name;
        QString cmd = "-var-list-children --all-values \"" + data.variable + "\"";
        sendSynchronizedCommand(cmd, WatchVarListChildren, QVariant::fromValue(data));
    } else if (exp == "staticMetaObject") {
        //    && item.findChild("type").data() == "const QMetaObject")
        // FIXME: Namespaces?
        // { do nothing }    FIXME: make coinfigurable?
        // special "clever" hack to avoid clutter in the GUI.
        // I am not sure this is a good idea...
    } else {
        WatchData data;
        data.iname = parent.iname + '.' + exp;
        data.variable = name;
        setWatchDataType(data, item.findChild("type"));
        setWatchDataValue(data, item.findChild("value"));
        setWatchDataAddress(data, item.findChild("addr"));
        setWatchDataChildCount(data, item.findChild("numchild"));
        if (!qq->watchHandler()->isExpandedIName(data.iname))
            data.setChildrenUnneeded();

        data.name = exp;
        if (isPointerType(parent.type) && data.type == exp) {
            data.exp = "*(" + parent.exp + ")";
            data.name = "*" + parent.name;
        } else if (data.type == exp) {
            // A type we derive from? gdb crashes when creating variables here
            data.exp = parent.exp;
        } else if (exp.startsWith("*")) {
            // A pointer
            data.exp = "*(" + parent.exp + ")";
        } else if (startsWithDigit(exp)) {
            // An array. No variables needed?
            data.name = "[" + data.name + "]";
            data.exp = parent.exp + "[" + exp + "]";
        } else if (0 && parent.name.endsWith('.')) {
            // Happens with anonymous unions
            data.exp = parent.exp + exp;
            //data.name = "<anonymous union>";
        } else if (exp.isEmpty()) {
            // Happens with anonymous unions
            data.exp = parent.exp;
            data.name = "<n/a>";
            data.iname = parent.iname + ".@";
            data.type = "<anonymous union>";
        } else {
            // A structure. Hope there's nothing else...
            data.exp = parent.exp + '.' + exp;
        }

        if (isCustomValueDumperAvailable(data.type)) {
            // we do not trust gdb if we have a custom dumper
            data.setValueNeeded();
            data.setChildCountNeeded();
        }

        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT 3" << parent.toString();
        //qDebug() <<  "VAR_LIST_CHILDREN: APPENDEE " << data.toString();
        insertData(data);
    }
}

void GdbEngine::handleVarListChildren(const GdbResultRecord &record,
    const WatchData &data0)
{
    //WatchResultCounter dummy(this, WatchVarListChildren);
    WatchData data = data0;
    if (!data.isValid())
        return;
    if (record.resultClass == GdbResultDone) {
        //qDebug() <<  "VAR_LIST_CHILDREN: PARENT " << data.toString();
        GdbMi children = record.data.findChild("children");

        foreach (const GdbMi &child, children.children())
            handleVarListChildrenHelper(child, data);

        if (!isAccessSpecifier(data.variable.split('.').takeLast())) {
            data.setChildrenUnneeded();
            insertData(data);
        }
    } else if (record.resultClass == GdbResultError) {
        data.setError(record.data.findChild("msg").data());
    } else {
        data.setError("Unknown error: " + record.toString());
    }
}

void GdbEngine::handleToolTip(const GdbResultRecord &record,
        const QString &what)
{
    //qDebug() << "HANDLE TOOLTIP: " << what << m_toolTip.toString();
    //    << "record: " << record.toString();
    if (record.resultClass == GdbResultError) {
        QString msg = record.data.findChild("msg").data();
        if (what == "create") {
            sendCommand("ptype " + m_toolTip.exp, WatchToolTip, "ptype");
            return;
        }
        if (what == "evaluate") {
            if (msg.startsWith("Cannot look up value of a typedef")) {
                m_toolTip.value = m_toolTip.exp + " is a typedef.";
                //return;
            }
        }
    } else if (record.resultClass == GdbResultDone) {
        if (what == "create") {
            setWatchDataType(m_toolTip, record.data.findChild("type"));
            setWatchDataChildCount(m_toolTip, record.data.findChild("numchild"));
            if (isCustomValueDumperAvailable(m_toolTip.type))
                runCustomDumper(m_toolTip, false);
            else
                q->showStatusMessage(tr("Retrieving data for tooltip..."), -1);
                sendCommand("-data-evaluate-expression " + m_toolTip.exp,
                    WatchToolTip, "evaluate");
                //sendToolTipCommand("-var-evaluate-expression tooltip")
            return;
        }
        if (what == "evaluate") {
            m_toolTip.value = m_toolTip.type + ' ' + m_toolTip.exp
                   + " = " + record.data.findChild("value").data();
            //return;
        }
        if (what == "ptype") {
            GdbMi mi = record.data.findChild("consolestreamoutput");
            m_toolTip.value = extractTypeFromPTypeOutput(mi.data());
            //return;
        }
    }

    m_toolTip.iname = tooltipIName;
    m_toolTip.setChildrenUnneeded();
    m_toolTip.setChildCountUnneeded();
    insertData(m_toolTip);
    qDebug() << "DATA INSERTED";
    QTimer::singleShot(0, this, SLOT(updateWatchModel2()));
    qDebug() << "HANDLE TOOLTIP END";
}

#if 0
void GdbEngine::handleChangedItem(QStandardItem *item)
{
    // HACK: Just store the item for the slot
    //  handleChangedItem(QWidget *widget) below.
    QModelIndex index = item->index().sibling(item->index().row(), 0);
    //WatchData data = m_currentSet.takeData(iname);
    //m_editedData = inameFromItem(m_model.itemFromIndex(index)).exp;
    //qDebug() << "HANDLE CHANGED EXPRESSION: " << m_editedData;
}
#endif

void GdbEngine::assignValueInDebugger(const QString &expression, const QString &value)
{
    sendCommand("-var-delete assign");
    sendCommand("-var-create assign * " + expression);
    sendCommand("-var-assign assign " + value, WatchVarAssign);
}


void GdbEngine::tryLoadCustomDumpers()
{
    if (m_dataDumperState != DataDumperUninitialized)
        return;

    PENDING_DEBUG("TRY LOAD CUSTOM DUMPERS");
    m_dataDumperState = DataDumperLoadTried;

#if defined(Q_OS_LINUX)
    QString lib = q->m_buildDir + "/qtc-gdbmacros/libgdbmacros.so";
    if (QFileInfo(lib).isExecutable()) {
        //sendCommand("p dlopen");
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 0");
        QString flag = QString::number(RTLD_NOW);
        sendCommand("call dlopen(\"" + lib + "\", " + flag + ")");
        sendCommand("sharedlibrary " + dotEscape(lib));
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 1");
    }
#endif
#if defined(Q_OS_MAC)
    QString lib = q->m_buildDir + "/qtc-gdbmacros/libgdbmacros.dylib";
    if (QFileInfo(lib).isExecutable()) {
        //sendCommand("p dlopen"); // FIXME: remove me
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 0");
        QString flag = QString::number(RTLD_NOW);
        sendCommand("call dlopen(\"" + lib + "\", " + flag + ")");
        sendCommand("sharedlibrary " + dotEscape(lib));
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 1");
    }
#endif
#if defined(Q_OS_WIN)
    QString lib = q->m_buildDir + "/qtc-gdbmacros/debug/gdbmacros.dll";
    if (QFileInfo(lib).exists()) {
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 0");
        //sendCommand("handle SIGSEGV pass stop print");
        //sendCommand("set unwindonsignal off");
        sendCommand("call LoadLibraryA(\"" + lib + "\")");
        sendCommand("sharedlibrary " + dotEscape(lib));
        if (qq->useFastStart())
            sendCommand("set stop-on-solib-events 1");
    }
#endif

    // retreive list of dumpable classes
    sendCommand("call qDumpObjectData440(1,%1+1,0,0,0,0,0,0)",
        GdbQueryDataDumper1);
    // create response slot for socket data
    sendCommand(QString(), GdbQueryDataDumper2);
}


IDebuggerEngine *createGdbEngine(DebuggerManager *parent)
{
    return new GdbEngine(parent);
}

