/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qtcprocess.h"

#include "stringutils.h"
#include "executeondestruction.h"
#include "hostosinfo.h"
#include "commandline.h"
#include "qtcassert.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QLoggingCategory>
#include <QTextCodec>
#include <QThread>
#include <QTimer>

#ifdef QT_GUI_LIB
// qmlpuppet does not use that.
#include <QApplication>
#include <QMessageBox>
#endif

#include <algorithm>
#include <limits.h>
#include <memory>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#else
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#endif


using namespace Utils::Internal;

namespace Utils {
namespace Internal {

enum { debug = 0 };
enum { syncDebug = 0 };

enum { defaultMaxHangTimerCount = 10 };

static Q_LOGGING_CATEGORY(processLog, "qtc.utils.synchronousprocess", QtWarningMsg);

static DeviceProcessHooks s_deviceHooks;

// Data for one channel buffer (stderr/stdout)
class ChannelBuffer
{
public:
    void clearForRun();

    QString linesRead();
    void append(const QByteArray &text, bool emitSignals);

    QByteArray rawData;
    QString incompleteLineBuffer; // lines not yet signaled
    QTextCodec *codec = nullptr; // Not owner
    std::unique_ptr<QTextCodec::ConverterState> codecState;
    int rawDataPos = 0;
    std::function<void(const QString &lines)> outputCallback;
};

class QtcProcessPrivate : public QObject
{
public:
    explicit QtcProcessPrivate(QtcProcess *parent) : q(parent) {}

    void setupChildProcess_impl();

    CommandLine m_commandLine;
    Environment m_environment;
    QByteArray m_writeData;
    bool m_haveEnv = false;
    bool m_useCtrlCStub = false;
    bool m_lowPriority = false;
    bool m_disableUnixTerminal = false;

    QProcess::OpenMode m_openMode = QProcess::ReadWrite;

    // SynchronousProcess left overs:
    void slotTimeout();
    void slotFinished(int exitCode, QProcess::ExitStatus e);
    void slotError(QProcess::ProcessError);
    void clearForRun();

    QtcProcess::Result interpretExitCode(int exitCode);

    QtcProcess *q;
    QTextCodec *m_codec = QTextCodec::codecForLocale();
    QTimer m_timer;
    QEventLoop m_eventLoop;
    QtcProcess::Result m_result = QtcProcess::StartFailed;
    int m_exitCode = -1;
    FilePath m_binary;
    ChannelBuffer m_stdOut;
    ChannelBuffer m_stdErr;
    ExitCodeInterpreter m_exitCodeInterpreter;

    int m_hangTimerCount = 0;
    int m_maxHangTimerCount = defaultMaxHangTimerCount;
    bool m_startFailure = false;
    bool m_timeOutMessageBoxEnabled = false;
    bool m_waitingForUser = false;
    bool m_isSynchronousProcess = false;
    bool m_processUserEvents = false;
};

void QtcProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdOut.codec = m_codec;
    m_stdErr.clearForRun();
    m_stdErr.codec = m_codec;
    m_result = QtcProcess::StartFailed;
    m_exitCode = -1;
    m_startFailure = false;
    m_binary = {};
}

QtcProcess::Result QtcProcessPrivate::interpretExitCode(int exitCode)
{
    if (m_exitCodeInterpreter)
        return m_exitCodeInterpreter(exitCode);

    // default:
    return exitCode ? QtcProcess::FinishedError : QtcProcess::Finished;
}

} // Internal

/*!
    \class Utils::QtcProcess

    \brief The QtcProcess class provides functionality for with processes.

    \sa Utils::ProcessArgs
*/

QtcProcess::QtcProcess(QObject *parent)
    : QProcess(parent), d(new QtcProcessPrivate(this))
{
    static int qProcessExitStatusMeta = qRegisterMetaType<QProcess::ExitStatus>();
    static int qProcessProcessErrorMeta = qRegisterMetaType<QProcess::ProcessError>();
    Q_UNUSED(qProcessExitStatusMeta)
    Q_UNUSED(qProcessProcessErrorMeta)

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0) && defined(Q_OS_UNIX)
    setChildProcessModifier([this] { d->setupChildProcess_impl(); });
#endif
}

QtcProcess::~QtcProcess()
{
    delete d;
}

void QtcProcess::setEnvironment(const Environment &env)
{
    d->m_environment = env;
    d->m_haveEnv = true;
}

const Environment &QtcProcess::environment() const
{
    return d->m_environment;
}

void QtcProcess::setCommand(const CommandLine &cmdLine)
{
    d->m_commandLine  = cmdLine;
}

const CommandLine &QtcProcess::commandLine() const
{
    return d->m_commandLine;
}

void QtcProcess::setUseCtrlCStub(bool enabled)
{
    // Do not use the stub in debug mode. Activating the stub will shut down
    // Qt Creator otherwise, because they share the same Windows console.
    // See QTCREATORBUG-11995 for details.
#ifndef QT_DEBUG
    d->m_useCtrlCStub = enabled;
#else
    Q_UNUSED(enabled)
#endif
}

void QtcProcess::start()
{
    QTC_CHECK(d->m_writeData.isEmpty()); // FIXME: Use it.

    if (d->m_commandLine.executable().needsDevice()) {
        QTC_ASSERT(s_deviceHooks.startProcessHook, return);
        s_deviceHooks.startProcessHook(*this);
        return;
    }

    Environment env;
    const OsType osType = HostOsInfo::hostOs();
    if (d->m_haveEnv) {
        if (d->m_environment.size() == 0)
            qWarning("QtcProcess::start: Empty environment set when running '%s'.",
                     qPrintable(d->m_commandLine.executable().toString()));
        env = d->m_environment;

        QProcess::setProcessEnvironment(env.toProcessEnvironment());
    } else {
        env = Environment::systemEnvironment();
    }

    const QString &workDir = workingDirectory();
    QString command;
    ProcessArgs arguments;
    bool success = ProcessArgs::prepareCommand(d->m_commandLine.executable().toString(),
                                               d->m_commandLine.arguments(),
                                               &command, &arguments, osType, &env, &workDir);
    if (osType == OsTypeWindows) {
        QString args;
        if (d->m_useCtrlCStub) {
            if (d->m_lowPriority)
                ProcessArgs::addArg(&args, "-nice");
            ProcessArgs::addArg(&args, QDir::toNativeSeparators(command));
            command = QCoreApplication::applicationDirPath()
                    + QLatin1String("/qtcreator_ctrlc_stub.exe");
        } else if (d->m_lowPriority) {
#ifdef Q_OS_WIN
            setCreateProcessArgumentsModifier([](CreateProcessArguments *args) {
                args->flags |= BELOW_NORMAL_PRIORITY_CLASS;
            });
#endif
        }
        ProcessArgs::addArgs(&args, arguments.toWindowsArgs());
#ifdef Q_OS_WIN
        setNativeArguments(args);
#endif
        // Note: Arguments set with setNativeArgs will be appended to the ones
        // passed with start() below.
        QProcess::start(command, QStringList(), d->m_openMode);
    } else {
        if (!success) {
            setErrorString(tr("Error in command line."));
            // Should be FailedToStart, but we cannot set the process error from the outside,
            // so it would be inconsistent.
            emit errorOccurred(QProcess::UnknownError);
            return;
        }
        QProcess::start(command, arguments.toUnixArgs(), d->m_openMode);
    }
}

#ifdef Q_OS_WIN
static BOOL sendMessage(UINT message, HWND hwnd, LPARAM lParam)
{
    DWORD dwProcessID;
    GetWindowThreadProcessId(hwnd, &dwProcessID);
    if ((DWORD)lParam == dwProcessID) {
        SendNotifyMessage(hwnd, message, 0, 0);
        return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK sendShutDownMessageToAllWindowsOfProcess_enumWnd(HWND hwnd, LPARAM lParam)
{
    static UINT uiShutDownMessage = RegisterWindowMessage(L"qtcctrlcstub_shutdown");
    return sendMessage(uiShutDownMessage, hwnd, lParam);
}

BOOL CALLBACK sendInterruptMessageToAllWindowsOfProcess_enumWnd(HWND hwnd, LPARAM lParam)
{
    static UINT uiInterruptMessage = RegisterWindowMessage(L"qtcctrlcstub_interrupt");
    return sendMessage(uiInterruptMessage, hwnd, lParam);
}
#endif

void QtcProcess::terminate()
{
#ifdef Q_OS_WIN
    if (d->m_useCtrlCStub)
        EnumWindows(sendShutDownMessageToAllWindowsOfProcess_enumWnd, processId());
    else
#endif
    QProcess::terminate();
}

void QtcProcess::interrupt()
{
#ifdef Q_OS_WIN
    QTC_ASSERT(d->m_useCtrlCStub, return);
    EnumWindows(sendInterruptMessageToAllWindowsOfProcess_enumWnd, processId());
#endif
}

void QtcProcess::setLowPriority()
{
    d->m_lowPriority = true;
}

void QtcProcess::setDisableUnixTerminal()
{
    d->m_disableUnixTerminal = true;
}

void QtcProcess::setRemoteProcessHooks(const DeviceProcessHooks &hooks)
{
    s_deviceHooks = hooks;
}

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
void QtcProcess::setupChildProcess()
{
    d->setupChildProcess_impl();
}
#endif

void QtcProcessPrivate::setupChildProcess_impl()
{
#if defined Q_OS_UNIX
    // nice value range is -20 to +19 where -20 is highest, 0 default and +19 is lowest
    if (m_lowPriority) {
        errno = 0;
        if (::nice(5) == -1 && errno != 0)
            perror("Failed to set nice value");
    }

    // Disable terminal by becoming a session leader.
    if (m_disableUnixTerminal)
        setsid();
#endif
}

void QtcProcess::setOpenMode(OpenMode mode)
{
    d->m_openMode = mode;
}

bool QtcProcess::stopProcess()
{
    if (state() == QProcess::NotRunning)
        return true;
    terminate();
    if (waitForFinished(300))
        return true;
    kill();
    return waitForFinished(300);
}

static bool askToKill(const QString &command)
{
#ifdef QT_GUI_LIB
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        return true;
    const QString title = QtcProcess::tr("Process not Responding");
    QString msg = command.isEmpty() ?
                QtcProcess::tr("The process is not responding.") :
                QtcProcess::tr("The process \"%1\" is not responding.").arg(command);
    msg += ' ';
    msg += QtcProcess::tr("Would you like to terminate it?");
    // Restore the cursor that is set to wait while running.
    const bool hasOverrideCursor = QApplication::overrideCursor() != nullptr;
    if (hasOverrideCursor)
        QApplication::restoreOverrideCursor();
    QMessageBox::StandardButton answer = QMessageBox::question(nullptr, title, msg, QMessageBox::Yes|QMessageBox::No);
    if (hasOverrideCursor)
        QApplication::setOverrideCursor(Qt::WaitCursor);
    return answer == QMessageBox::Yes;
#else
    Q_UNUSED(command)
    return true;
#endif
}

// Helper for running a process synchronously in the foreground with timeout
// detection similar SynchronousProcess' handling (taking effect after no more output
// occurs on stderr/stdout as opposed to waitForFinished()). Returns false if a timeout
// occurs. Checking of the process' exit state/code still has to be done.

bool QtcProcess::readDataFromProcess(int timeoutS,
                                     QByteArray *stdOut,
                                     QByteArray *stdErr,
                                     bool showTimeOutMessageBox)
{
    enum { syncDebug = 0 };
    if (syncDebug)
        qDebug() << ">readDataFromProcess" << timeoutS;
    if (state() != QProcess::Running) {
        qWarning("readDataFromProcess: Process in non-running state passed in.");
        return false;
    }

    QTC_ASSERT(readChannel() == QProcess::StandardOutput, return false);

    // Keep the process running until it has no longer has data
    bool finished = false;
    bool hasData = false;
    do {
        finished = waitForFinished(timeoutS > 0 ? timeoutS * 1000 : -1)
                || state() == QProcess::NotRunning;
        // First check 'stdout'
        if (bytesAvailable()) { // applies to readChannel() only
            hasData = true;
            const QByteArray newStdOut = readAllStandardOutput();
            if (stdOut)
                stdOut->append(newStdOut);
        }
        // Check 'stderr' separately. This is a special handling
        // for 'git pull' and the like which prints its progress on stderr.
        const QByteArray newStdErr = readAllStandardError();
        if (!newStdErr.isEmpty()) {
            hasData = true;
            if (stdErr)
                stdErr->append(newStdErr);
        }
        // Prompt user, pretend we have data if says 'No'.
        const bool hang = !hasData && !finished;
        hasData = hang && showTimeOutMessageBox && !askToKill(program());
    } while (hasData && !finished);
    if (syncDebug)
        qDebug() << "<readDataFromProcess" << finished;
    return finished;
}

QString QtcProcess::normalizeNewlines(const QString &text)
{
    QString res = text;
    const auto newEnd = std::unique(res.begin(), res.end(), [](const QChar &c1, const QChar &c2) {
        return c1 == '\r' && c2 == '\r'; // QTCREATORBUG-24556
    });
    res.chop(std::distance(newEnd, res.end()));
    res.replace("\r\n", "\n");
    return res;
}

QtcProcess::Result QtcProcess::result() const
{
    return d->m_result;
}

void QtcProcess::setResult(Result result)
{
    d->m_result = result;
}

int QtcProcess::exitCode() const
{
    return d->m_isSynchronousProcess ? d->m_exitCode : QProcess::exitCode(); // FIXME: Unify.
}


// Path utilities

// Locate a binary in a directory, applying all kinds of
// extensions the operating system supports.
static QString checkBinary(const QDir &dir, const QString &binary)
{
    // naive UNIX approach
    const QFileInfo info(dir.filePath(binary));
    if (info.isFile() && info.isExecutable())
        return info.absoluteFilePath();

    // Does the OS have some weird extension concept or does the
    // binary have a 3 letter extension?
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        return QString();
    const int dotIndex = binary.lastIndexOf(QLatin1Char('.'));
    if (dotIndex != -1 && dotIndex == binary.size() - 4)
        return  QString();

    switch (HostOsInfo::hostOs()) {
    case OsTypeLinux:
    case OsTypeOtherUnix:
    case OsTypeOther:
        break;
    case OsTypeWindows: {
            static const char *windowsExtensions[] = {".cmd", ".bat", ".exe", ".com"};
            // Check the Windows extensions using the order
            const int windowsExtensionCount = sizeof(windowsExtensions)/sizeof(const char*);
            for (int e = 0; e < windowsExtensionCount; e ++) {
                const QFileInfo windowsBinary(dir.filePath(binary + QLatin1String(windowsExtensions[e])));
                if (windowsBinary.isFile() && windowsBinary.isExecutable())
                    return windowsBinary.absoluteFilePath();
            }
        }
        break;
    case OsTypeMac: {
            // Check for Mac app folders
            const QFileInfo appFolder(dir.filePath(binary + QLatin1String(".app")));
            if (appFolder.isDir()) {
                QString macBinaryPath = appFolder.absoluteFilePath();
                macBinaryPath += QLatin1String("/Contents/MacOS/");
                macBinaryPath += binary;
                const QFileInfo macBinary(macBinaryPath);
                if (macBinary.isFile() && macBinary.isExecutable())
                    return macBinary.absoluteFilePath();
            }
        }
        break;
    }
    return QString();
}

QString QtcProcess::locateBinary(const QString &path, const QString &binary)
{
    // Absolute file?
    const QFileInfo absInfo(binary);
    if (absInfo.isAbsolute())
        return checkBinary(absInfo.dir(), absInfo.fileName());

    // Windows finds binaries  in the current directory
    if (HostOsInfo::isWindowsHost()) {
        const QString currentDirBinary = checkBinary(QDir::current(), binary);
        if (!currentDirBinary.isEmpty())
            return currentDirBinary;
    }

    const QStringList paths = path.split(HostOsInfo::pathListSeparator());
    if (paths.empty())
        return QString();
    const QStringList::const_iterator cend = paths.constEnd();
    for (QStringList::const_iterator it = paths.constBegin(); it != cend; ++it) {
        const QDir dir(*it);
        const QString rc = checkBinary(dir, binary);
        if (!rc.isEmpty())
            return rc;
    }
    return QString();
}

Environment QtcProcess::systemEnvironmentForBinary(const FilePath &filePath)
{
    if (filePath.needsDevice()) {
        QTC_ASSERT(s_deviceHooks.systemEnvironmentForBinary, return {});
        return s_deviceHooks.systemEnvironmentForBinary(filePath);
    }

    return Environment::systemEnvironment();
}

QString QtcProcess::locateBinary(const QString &binary)
{
    const QByteArray path = qgetenv("PATH");
    return locateBinary(QString::fromLocal8Bit(path), binary);
}


/*!
    \class Utils::SynchronousProcess

    \brief The SynchronousProcess class runs a synchronous process in its own
    event loop that blocks only user input events. Thus, it allows for the GUI to
    repaint and append output to log windows.

    The callbacks set with setStdOutCallBack(), setStdErrCallback() are called
    with complete lines based on the '\\n' marker.
    They would typically be used for log windows.

    There is a timeout handling that takes effect after the last data have been
    read from stdout/stdin (as opposed to waitForFinished(), which measures time
    since it was invoked). It is thus also suitable for slow processes that
    continuously output data (like version system operations).

    The property timeOutMessageBoxEnabled influences whether a message box is
    shown asking the user if they want to kill the process on timeout (default: false).

    There are also static utility functions for dealing with fully synchronous
    processes, like reading the output with correct timeout handling.

    Caution: This class should NOT be used if there is a chance that the process
    triggers opening dialog boxes (for example, by file watchers triggering),
    as this will cause event loop problems.
*/

QString QtcProcess::exitMessage()
{
    const QString fullCmd = commandLine().toUserOutput();
    switch (result()) {
    case Finished:
        return QtcProcess::tr("The command \"%1\" finished successfully.").arg(fullCmd);
    case FinishedError:
        return QtcProcess::tr("The command \"%1\" terminated with exit code %2.")
            .arg(fullCmd).arg(exitCode());
    case TerminatedAbnormally:
        return QtcProcess::tr("The command \"%1\" terminated abnormally.").arg(fullCmd);
    case StartFailed:
        return QtcProcess::tr("The command \"%1\" could not be started.").arg(fullCmd);
    case Hang:
        return QtcProcess::tr("The command \"%1\" did not respond within the timeout limit (%2 s).")
            .arg(fullCmd).arg(d->m_hangTimerCount);
    }
    return QString();
}

QByteArray QtcProcess::allRawOutput() const
{
    if (!d->m_stdOut.rawData.isEmpty() && !d->m_stdErr.rawData.isEmpty()) {
        QByteArray result = d->m_stdOut.rawData;
        if (!result.endsWith('\n'))
            result += '\n';
        result += d->m_stdErr.rawData;
        return result;
    }
    return !d->m_stdOut.rawData.isEmpty() ? d->m_stdOut.rawData : d->m_stdErr.rawData;
}

QString QtcProcess::allOutput() const
{
    const QString out = stdOut();
    const QString err = stdErr();

    if (!out.isEmpty() && !err.isEmpty()) {
        QString result = out;
        if (!result.endsWith('\n'))
            result += '\n';
        result += err;
        return result;
    }
    return !out.isEmpty() ? out : err;
}

QString QtcProcess::stdOut() const
{
    return normalizeNewlines(d->m_codec->toUnicode(d->m_stdOut.rawData));
}

QString QtcProcess::stdErr() const
{
    return normalizeNewlines(d->m_codec->toUnicode(d->m_stdErr.rawData));
}

QByteArray QtcProcess::rawStdOut() const
{
    return d->m_stdOut.rawData;
}

QByteArray QtcProcess::rawStdErr() const
{
    return d->m_stdErr.rawData;
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r)
{
    QDebug nsp = str.nospace();
    nsp << "QtcProcess: result="
        << r.d->m_result << " ex=" << r.exitCode() << '\n'
        << r.d->m_stdOut.rawData.size() << " bytes stdout, stderr=" << r.d->m_stdErr.rawData << '\n';
    return str;
}

void ChannelBuffer::clearForRun()
{
    rawDataPos = 0;
    rawData.clear();
    codecState.reset(new QTextCodec::ConverterState);
    incompleteLineBuffer.clear();
}

/* Check for complete lines read from the device and return them, moving the
 * buffer position. */
QString ChannelBuffer::linesRead()
{
    // Convert and append the new input to the buffer of incomplete lines
    const char *start = rawData.constData() + rawDataPos;
    const int len = rawData.size() - rawDataPos;
    incompleteLineBuffer.append(codec->toUnicode(start, len, codecState.get()));
    rawDataPos = rawData.size();

    // Any completed lines in the incompleteLineBuffer?
    const int lastLineIndex = qMax(incompleteLineBuffer.lastIndexOf('\n'),
                                   incompleteLineBuffer.lastIndexOf('\r'));
    if (lastLineIndex == -1)
        return QString();

    // Get completed lines and remove them from the incompleteLinesBuffer:
    const QString lines = QtcProcess::normalizeNewlines(incompleteLineBuffer.left(lastLineIndex + 1));
    incompleteLineBuffer = incompleteLineBuffer.mid(lastLineIndex + 1);

    return lines;
}

void ChannelBuffer::append(const QByteArray &text, bool emitSignals)
{
    if (text.isEmpty())
        return;
    rawData += text;
    if (!emitSignals)
        return;

    // Buffered. Emit complete lines?
    if (outputCallback) {
        const QString lines = linesRead();
        if (!lines.isEmpty())
            outputCallback(lines);
    }
}


// ----------- SynchronousProcess
SynchronousProcess::SynchronousProcess()
{
    d->m_isSynchronousProcess = true; // Only for QTC_ASSERTs above.

    d->m_timer.setInterval(1000);
    connect(&d->m_timer, &QTimer::timeout, d, &QtcProcessPrivate::slotTimeout);
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            d, &QtcProcessPrivate::slotFinished);
    connect(this, &QProcess::errorOccurred, d, &QtcProcessPrivate::slotError);
    connect(this, &QProcess::readyReadStandardOutput, d, [this] {
        d->m_hangTimerCount = 0;
        d->m_stdOut.append(readAllStandardOutput(), true);
    });
    connect(this, &QProcess::readyReadStandardError, d, [this] {
        d->m_hangTimerCount = 0;
        d->m_stdErr.append(readAllStandardError(), true);
    });
}

SynchronousProcess::~SynchronousProcess()
{
    disconnect(&d->m_timer, nullptr, this, nullptr);
    disconnect(this, nullptr, this, nullptr);
}

void SynchronousProcess::setProcessUserEventWhileRunning()
{
    d->m_processUserEvents = true;
}

void QtcProcess::setTimeoutS(int timeoutS)
{
    QTC_CHECK(d->m_isSynchronousProcess);
    if (timeoutS > 0)
        d->m_maxHangTimerCount = qMax(2, timeoutS);
    else
        d->m_maxHangTimerCount = INT_MAX / 1000;
}

void QtcProcess::setCodec(QTextCodec *c)
{
    QTC_ASSERT(c, return);
    d->m_codec = c;
}

void QtcProcess::setTimeOutMessageBoxEnabled(bool v)
{
    QTC_CHECK(d->m_isSynchronousProcess);
    d->m_timeOutMessageBoxEnabled = v;
}

void QtcProcess::setExitCodeInterpreter(const ExitCodeInterpreter &interpreter)
{
    d->m_exitCodeInterpreter = interpreter;
}

void QtcProcess::setWriteData(const QByteArray &writeData)
{
    QTC_CHECK(d->m_isSynchronousProcess);
    d->m_writeData = writeData;
}

#ifdef QT_GUI_LIB
static bool isGuiThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}
#endif

void SynchronousProcess::runBlocking()
{
    QTC_CHECK(d->m_isSynchronousProcess);
    // FIXME: Implement properly
    if (d->m_commandLine.executable().needsDevice()) {

        // writeData ?
        QtcProcess::start();

        waitForFinished();

        d->m_result = QtcProcess::Finished;
        d->m_exitCode = exitCode();
        d->m_stdOut.rawData += readAllStandardOutput();
        d->m_stdErr.rawData += readAllStandardError();
        return;
    };

    qCDebug(processLog).noquote() << "Starting blocking:" << d->m_commandLine.toUserOutput()
        << " process user events: " << d->m_processUserEvents;
    ExecuteOnDestruction logResult([this] { qCDebug(processLog) << *this; });

    d->clearForRun();

    d->m_binary = d->m_commandLine.executable();

    if (d->m_processUserEvents) {
        if (!d->m_writeData.isEmpty()) {
            connect(this, &QProcess::started, this, [this] {
                write(d->m_writeData);
                closeWriteChannel();
            });
        }
        setOpenMode(d->m_writeData.isEmpty() ? QIODevice::ReadOnly : QIODevice::ReadWrite);
        QtcProcess::start();

        // On Windows, start failure is triggered immediately if the
        // executable cannot be found in the path. Do not start the
        // event loop in that case.
        if (!d->m_startFailure) {
            d->m_timer.start();
#ifdef QT_GUI_LIB
            if (isGuiThread())
                QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
            d->m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
            d->m_stdOut.append(readAllStandardOutput(), false);
            d->m_stdErr.append(readAllStandardError(), false);

            d->m_timer.stop();
#ifdef QT_GUI_LIB
            if (isGuiThread())
                QApplication::restoreOverrideCursor();
#endif
        }
    } else {
        setOpenMode(QIODevice::ReadOnly);
        QtcProcess::start();
        if (!waitForStarted(d->m_maxHangTimerCount * 1000)) {
            d->m_result = QtcProcess::StartFailed;
            return;
        }
        closeWriteChannel();
        if (!waitForFinished(d->m_maxHangTimerCount * 1000)) {
            d->m_result = QtcProcess::Hang;
            terminate();
            if (!waitForFinished(1000)) {
                kill();
                waitForFinished(1000);
            }
        }

        if (state() != QProcess::NotRunning)
            return;

        d->m_exitCode = exitCode();
        if (d->m_result == QtcProcess::StartFailed) {
            if (exitStatus() != QProcess::NormalExit)
                d->m_result = QtcProcess::TerminatedAbnormally;
            else
                d->m_result = d->interpretExitCode(d->m_exitCode);
        }
        d->m_stdOut.append(readAllStandardOutput(), false);
        d->m_stdErr.append(readAllStandardError(), false);
    }
}

void QtcProcess::setStdOutCallback(const std::function<void (const QString &)> &callback)
{
    QTC_CHECK(d->m_isSynchronousProcess);
    d->m_stdOut.outputCallback = callback;
}

void QtcProcess::setStdErrCallback(const std::function<void (const QString &)> &callback)
{
    QTC_CHECK(d->m_isSynchronousProcess);
    d->m_stdErr.outputCallback = callback;
}

void QtcProcessPrivate::slotTimeout()
{
    if (!m_waitingForUser && (++m_hangTimerCount > m_maxHangTimerCount)) {
        if (debug)
            qDebug() << Q_FUNC_INFO << "HANG detected, killing";
        m_waitingForUser = true;
        const bool terminate = !m_timeOutMessageBoxEnabled || askToKill(m_binary.toString());
        m_waitingForUser = false;
        if (terminate) {
            q->stopProcess();
            m_result = QtcProcess::Hang;
        } else {
            m_hangTimerCount = 0;
        }
    } else {
        if (debug)
            qDebug() << Q_FUNC_INFO << m_hangTimerCount;
    }
}

void QtcProcessPrivate::slotFinished(int exitCode, QProcess::ExitStatus e)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << exitCode << e;
    m_hangTimerCount = 0;

    switch (e) {
    case QProcess::NormalExit:
        m_result = interpretExitCode(exitCode);
        m_exitCode = exitCode;
        break;
    case QProcess::CrashExit:
        // Was hang detected before and killed?
        if (m_result != QtcProcess::Hang)
            m_result = QtcProcess::TerminatedAbnormally;
        m_exitCode = -1;
        break;
    }
    m_eventLoop.quit();
}

void QtcProcessPrivate::slotError(QProcess::ProcessError e)
{
    m_hangTimerCount = 0;
    if (debug)
        qDebug() << Q_FUNC_INFO << e;
    // Was hang detected before and killed?
    if (m_result != QtcProcess::Hang)
        m_result = QtcProcess::StartFailed;
    m_startFailure = true;
    m_eventLoop.quit();
}

} // namespace Utils
