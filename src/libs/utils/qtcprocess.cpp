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

#include "commandline.h"
#include "executeondestruction.h"
#include "hostosinfo.h"
#include "launcherinterface.h"
#include "launcherpackets.h"
#include "launchersocket.h"
#include "processreaper.h"
#include "processutils.h"
#include "stringutils.h"
#include "terminalprocess_p.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QLoggingCategory>
#include <QScopeGuard>
#include <QTextCodec>
#include <QThread>
#include <QTimer>

#ifdef QT_GUI_LIB
// qmlpuppet does not use that.
#include <QApplication>
#include <QMessageBox>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>

using namespace Utils::Internal;

namespace Utils {
namespace Internal {

const char QTC_PROCESS_BLOCKING_TYPE[] = "__BLOCKING_TYPE__";
const char QTC_PROCESS_NUMBER[] = "__NUMBER__";
const char QTC_PROCESS_STARTTIME[] = "__STARTTIME__";

class MeasureAndRun
{
public:
    MeasureAndRun(const char *functionName)
        : m_functionName(functionName)
        , m_measureProcess(qEnvironmentVariableIsSet("QTC_MEASURE_PROCESS"))
    {}
    template <typename Function, typename... Args>
    std::invoke_result_t<Function, Args...> measureAndRun(Function &&function, Args&&... args)
    {
        if (!m_measureProcess)
            return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
        QElapsedTimer timer;
        timer.start();
        auto cleanup = qScopeGuard([this, &timer] {
            const qint64 currentNsecs = timer.nsecsElapsed();
            const bool isMainThread = QThread::currentThread() == qApp->thread();
            const int hitThisAll = m_hitThisAll.fetch_add(1) + 1;
            const int hitAllAll = m_hitAllAll.fetch_add(1) + 1;
            const int hitThisMain = isMainThread
                    ? m_hitThisMain.fetch_add(1) + 1
                    : m_hitThisMain.load();
            const int hitAllMain = isMainThread
                    ? m_hitAllMain.fetch_add(1) + 1
                    : m_hitAllMain.load();
            const qint64 totalThisAll = toMs(m_totalThisAll.fetch_add(currentNsecs) + currentNsecs);
            const qint64 totalAllAll = toMs(m_totalAllAll.fetch_add(currentNsecs) + currentNsecs);
            const qint64 totalThisMain = toMs(isMainThread
                    ? m_totalThisMain.fetch_add(currentNsecs) + currentNsecs
                    : m_totalThisMain.load());
            const qint64 totalAllMain = toMs(isMainThread
                    ? m_totalAllMain.fetch_add(currentNsecs) + currentNsecs
                    : m_totalAllMain.load());
            printMeasurement(QLatin1String(m_functionName), hitThisAll, toMs(currentNsecs),
                             totalThisAll, hitAllAll, totalAllAll, isMainThread,
                             hitThisMain, totalThisMain, hitAllMain, totalAllMain);
        });
        return std::invoke(std::forward<Function>(function), std::forward<Args>(args)...);
    }
private:
    static void printHeader()
    {
        // [function/thread]: function:(T)his|(A)ll, thread:(M)ain|(A)ll
        qDebug() << "+----------------+-------+---------+----------+-------+----------+---------+-------+----------+-------+----------+";
        qDebug() << "| [Function/Thread] = [(T|A)/(M|A)], where: (T)his function, (A)ll functions / threads, (M)ain thread            |";
        qDebug() << "+----------------+-------+---------+----------+-------+----------+---------+-------+----------+-------+----------+";
        qDebug() << "|              1 |     2 |       3 |        4 |     5 |        6 |       7 |     8 |        9 |    10 |       11 |";
        qDebug() << "|                | [T/A] |   [T/A] |    [T/A] | [A/A] |    [A/A] |         | [T/M] |    [T/M] | [A/M] |    [A/M] |";
        qDebug() << "|       Function |   Hit | Current |    Total |   Hit |    Total | Current |   Hit |    Total |   Hit |    Total |";
        qDebug() << "|           Name | Count |  Measu- |   Measu- | Count |   Measu- | is Main | Count |   Measu- | Count |   Measu- |";
        qDebug() << "|                |       |  rement |   rement |       |   rement |  Thread |       |   rement |       |   rement |";
        qDebug() << "+----------------+-------+---------+----------+-------+----------+---------+-------+----------+-------+----------+";
    }
    static void printMeasurement(const QString &functionName, int hitThisAll, int currentNsecs,
                                 int totalThisAll, int hitAllAll, int totalAllAll, bool isMainThread,
                                 int hitThisMain, int totalThisMain, int hitAllMain, int totalAllMain)
    {
        static const int repeatHeaderLineCount = 25;
        if (s_lineCounter.fetch_add(1) % repeatHeaderLineCount == 0)
            printHeader();

        const QString &functionNameField = QString("%1").arg(functionName, 14);
        const QString &hitThisAllField = formatField(hitThisAll, 5);
        const QString &currentNsecsField = formatField(currentNsecs, 7, " ms");
        const QString &totalThisAllField = formatField(totalThisAll, 8, " ms");
        const QString &hitAllAllField = formatField(hitAllAll, 5);
        const QString &totalAllAllField = formatField(totalAllAll, 8, " ms");
        const QString &mainThreadField = isMainThread ? QString("%1").arg("yes", 7)
                                                      : QString("%1").arg("no", 7);
        const QString &hitThisMainField = formatField(hitThisMain, 5);
        const QString &totalThisMainField = formatField(totalThisMain, 8, " ms");
        const QString &hitAllMainField = formatField(hitAllMain, 5);
        const QString &totalAllMainField = formatField(totalAllMain, 8, " ms");

        const QString &totalString = QString("| %1 | %2 | %3 | %4 | %5 | %6 | %7 | %8 | %9 | %10 | %11 |")
                .arg(functionNameField, hitThisAllField, currentNsecsField,
                     totalThisAllField, hitAllAllField, totalAllAllField, mainThreadField,
                     hitThisMainField, totalThisMainField, hitAllMainField, totalAllMainField);
        qDebug("%s", qPrintable(totalString));
    }
    static QString formatField(int number, int fieldWidth, const QString &suffix = QString())
    {
        return QString("%1%2").arg(number, fieldWidth - suffix.count()).arg(suffix);
    }

    static int toMs(quint64 nsesc) // nanoseconds to miliseconds
    {
        static const int halfMillion = 500000;
        static const int million = 2 * halfMillion;
        return int((nsesc + halfMillion) / million);
    }

    const char * const m_functionName;
    const bool m_measureProcess;
    std::atomic_int m_hitThisAll = 0;
    std::atomic_int m_hitThisMain = 0;
    std::atomic_int64_t m_totalThisAll = 0;
    std::atomic_int64_t m_totalThisMain = 0;
    static std::atomic_int m_hitAllAll;
    static std::atomic_int m_hitAllMain;
    static std::atomic_int64_t m_totalAllAll;
    static std::atomic_int64_t m_totalAllMain;
    static std::atomic_int s_lineCounter;
};

std::atomic_int MeasureAndRun::m_hitAllAll = 0;
std::atomic_int MeasureAndRun::m_hitAllMain = 0;
std::atomic_int64_t MeasureAndRun::m_totalAllAll = 0;
std::atomic_int64_t MeasureAndRun::m_totalAllMain = 0;
std::atomic_int MeasureAndRun::s_lineCounter = 0;

static MeasureAndRun s_start = MeasureAndRun("start");
static MeasureAndRun s_waitForStarted = MeasureAndRun("waitForStarted");

enum { debug = 0 };
enum { syncDebug = 0 };

enum { defaultMaxHangTimerCount = 10 };

static Q_LOGGING_CATEGORY(processLog, "qtc.utils.qtcprocess", QtWarningMsg)
static Q_LOGGING_CATEGORY(processStdoutLog, "qtc.utils.qtcprocess.stdout", QtWarningMsg)
static Q_LOGGING_CATEGORY(processStderrLog, "qtc.utils.qtcprocess.stderr", QtWarningMsg)

static DeviceProcessHooks s_deviceHooks;

// Data for one channel buffer (stderr/stdout)
class ChannelBuffer
{
public:
    void clearForRun();

    void handleRest();
    void append(const QByteArray &text);

    QByteArray rawData;
    QString incompleteLineBuffer; // lines not yet signaled
    QTextCodec *codec = nullptr; // Not owner
    std::unique_ptr<QTextCodec::ConverterState> codecState;
    std::function<void(const QString &lines)> outputCallback;

    bool emitSingleLines = true;
    bool keepRawData = true;
};

class DefaultImpl : public ProcessInterface
{
public:
    virtual void start() final { defaultStart(); }

protected:
    void defaultStart();

private:
    virtual void doDefaultStart(const QString &program, const QStringList &arguments) = 0;
    bool dissolveCommand(QString *program, QStringList *arguments);
    bool ensureProgramExists(const QString &program);
};

static QString blockingMessage(const QVariant &variant)
{
    if (!variant.isValid())
        return "non blocking";
    if (variant.toInt() == int(EventLoopMode::On))
        return "blocking with event loop";
    return "blocking without event loop";
}

void DefaultImpl::defaultStart()
{
    if (processLog().isDebugEnabled()) {
        using namespace std::chrono;
        const quint64 msSinceEpoc =
                duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        setProperty(QTC_PROCESS_STARTTIME, msSinceEpoc);

        static std::atomic_int startCounter = 0;
        const int currentNumber = startCounter.fetch_add(1);
        qCDebug(processLog).nospace().noquote()
                << "Process " << currentNumber << " starting ("
                << qPrintable(blockingMessage(property(QTC_PROCESS_BLOCKING_TYPE)))
                << "): "
                << m_setup->m_commandLine.toUserOutput();
        setProperty(QTC_PROCESS_NUMBER, currentNumber);
    }

    QString program;
    QStringList arguments;
    if (!dissolveCommand(&program, &arguments))
        return;
    if (!ensureProgramExists(program))
        return;
    s_start.measureAndRun(&DefaultImpl::doDefaultStart, this, program, arguments);
}

bool DefaultImpl::dissolveCommand(QString *program, QStringList *arguments)
{
    const CommandLine &commandLine = m_setup->m_commandLine;
    QString commandString;
    ProcessArgs processArgs;
    const bool success = ProcessArgs::prepareCommand(commandLine, &commandString, &processArgs,
                                                     &m_setup->m_environment,
                                                     &m_setup->m_workingDirectory);

    if (commandLine.executable().osType() == OsTypeWindows) {
        QString args;
        if (m_setup->m_useCtrlCStub) {
            if (m_setup->m_lowPriority)
                ProcessArgs::addArg(&args, "-nice");
            ProcessArgs::addArg(&args, QDir::toNativeSeparators(commandString));
            commandString = QCoreApplication::applicationDirPath()
                    + QLatin1String("/qtcreator_ctrlc_stub.exe");
        } else if (m_setup->m_lowPriority) {
            m_setup->m_belowNormalPriority = true;
        }
        ProcessArgs::addArgs(&args, processArgs.toWindowsArgs());
        m_setup->m_nativeArguments = args;
        // Note: Arguments set with setNativeArgs will be appended to the ones
        // passed with start() below.
        *arguments = QStringList();
    } else {
        if (!success) {
            const ProcessResultData result = { 0, QProcess::NormalExit, QProcess::FailedToStart,
                                               tr("Error in command line.") };
            emit done(result);
            return false;
        }
        *arguments = processArgs.toUnixArgs();
    }
    *program = commandString;
    return true;
}

static FilePath resolve(const FilePath &workingDir, const FilePath &filePath)
{
    if (filePath.isAbsolutePath())
        return filePath;

    const FilePath fromWorkingDir = workingDir.resolvePath(filePath);
    if (fromWorkingDir.exists() && fromWorkingDir.isExecutableFile())
        return fromWorkingDir;
    return filePath.searchInPath();
}

bool DefaultImpl::ensureProgramExists(const QString &program)
{
    const FilePath programFilePath = resolve(m_setup->m_workingDirectory,
                                             FilePath::fromString(program));
    if (programFilePath.exists() && programFilePath.isExecutableFile())
        return true;

    const QString errorString = tr("The program \"%1\" does not exist or is not executable.")
                                .arg(program);
    const ProcessResultData result = { 0, QProcess::NormalExit, QProcess::FailedToStart,
                                       errorString };
    emit done(result);
    return false;
}

class QProcessImpl final : public DefaultImpl
{
public:
    QProcessImpl() : m_process(new ProcessHelper(this))
    {
        connect(m_process, &QProcess::started,
                this, &QProcessImpl::handleStarted);
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &QProcessImpl::handleFinished);
        connect(m_process, &QProcess::errorOccurred,
                this, &QProcessImpl::handleError);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
            emit readyRead(m_process->readAllStandardOutput(), {});
        });
        connect(m_process, &QProcess::readyReadStandardError, this, [this] {
            emit readyRead({}, m_process->readAllStandardError());
        });
    }
    ~QProcessImpl() final { ProcessReaper::reap(m_process); }

    void interrupt() final { ProcessHelper::interruptProcess(m_process); }
    void terminate() final { ProcessHelper::terminateProcess(m_process); }
    void kill() final { m_process->kill(); }
    void close() final { m_process->close(); }
    qint64 write(const QByteArray &data) final { return m_process->write(data); }

    QProcess::ProcessState state() const final { return m_process->state(); }
    qint64 processId() const final { return m_process->processId(); }

    bool waitForStarted(int msecs) final { return m_process->waitForStarted(msecs); }
    bool waitForReadyRead(int msecs) final { return m_process->waitForReadyRead(msecs); }
    bool waitForFinished(int msecs) final { return m_process->waitForFinished(msecs); }

private:
    void doDefaultStart(const QString &program, const QStringList &arguments) final
    {
        ProcessStartHandler *handler = m_process->processStartHandler();
        handler->setProcessMode(m_setup->m_processMode);
        handler->setWriteData(m_setup->m_writeData);
        if (m_setup->m_belowNormalPriority)
            handler->setBelowNormalPriority();
        handler->setNativeArguments(m_setup->m_nativeArguments);
        m_process->setProcessEnvironment(m_setup->m_environment.toProcessEnvironment());
        m_process->setWorkingDirectory(m_setup->m_workingDirectory.path());
        m_process->setStandardInputFile(m_setup->m_standardInputFile);
        if (m_setup->m_lowPriority)
            m_process->setLowPriority();
        if (m_setup->m_unixTerminalDisabled)
            m_process->setUnixTerminalDisabled();
        m_process->setUseCtrlCStub(m_setup->m_useCtrlCStub);
        m_process->start(program, arguments, handler->openMode());
        handler->handleProcessStart();
    }

    void handleStarted()
    {
        m_process->processStartHandler()->handleProcessStarted();
        emit started();
    }

    void handleError(QProcess::ProcessError error)
    {
        if (error != QProcess::FailedToStart)
            return;
        const ProcessResultData result = { m_process->exitCode(), m_process->exitStatus(),
                                           error, m_process->errorString() };
        emit done(result);
    }

    void handleFinished(int exitCode, QProcess::ExitStatus exitStatus)
    {
        const ProcessResultData result = { exitCode, exitStatus,
                                           m_process->error(), m_process->errorString() };
        emit done(result);
    }

    ProcessHelper *m_process;
};

static uint uniqueToken()
{
    static std::atomic_uint globalUniqueToken = 0;
    return ++globalUniqueToken;
}

class ProcessLauncherImpl final : public DefaultImpl
{
    Q_OBJECT
public:
    ProcessLauncherImpl() : m_token(uniqueToken())
    {
        m_handle = LauncherInterface::registerHandle(this, token());
        m_handle->setProcessSetupData(m_setup);
        connect(m_handle, &CallerHandle::started,
                this, &ProcessInterface::started);
        connect(m_handle, &CallerHandle::readyRead,
                this, &ProcessInterface::readyRead);
        connect(m_handle, &CallerHandle::done,
                this, &ProcessInterface::done);
    }
    ~ProcessLauncherImpl() final
    {
        m_handle->kill();
        LauncherInterface::unregisterHandle(token());
        m_handle = nullptr;
    }

    void interrupt() final
    {
        if (m_setup->m_useCtrlCStub) // bypass launcher and interrupt directly
            ProcessHelper::interruptPid(processId());
    }
    void terminate() final { m_handle->terminate(); }
    void kill() final { m_handle->kill(); }
    void close() final { m_handle->kill(); } // TODO: is it more like terminate or kill?
    qint64 write(const QByteArray &data) final { return m_handle->write(data); }

    QProcess::ProcessState state() const final { return m_handle->state(); }
    qint64 processId() const final { return m_handle->processId(); }

    bool waitForStarted(int msecs) final { return m_handle->waitForStarted(msecs); }
    bool waitForReadyRead(int msecs) final { return m_handle->waitForReadyRead(msecs); }
    bool waitForFinished(int msecs) final { return m_handle->waitForFinished(msecs); }

private:
    void doDefaultStart(const QString &program, const QStringList &arguments) final
    {
        m_handle->start(program, arguments);
    }

    quintptr token() const { return m_token; }

    const uint m_token = 0;
    // Lives in caller's thread.
    CallerHandle *m_handle = nullptr;
};

static ProcessImpl defaultProcessImpl()
{
    if (qEnvironmentVariableIsSet("QTC_USE_QPROCESS"))
        return ProcessImpl::QProcess;
    return ProcessImpl::ProcessLauncher;
}

class QtcProcessPrivate : public QObject
{
public:
    explicit QtcProcessPrivate(QtcProcess *parent)
        : QObject(parent)
        , q(parent)
    {}

    ProcessInterface *createProcessInterface()
    {
        if (m_setup.m_terminalMode != TerminalMode::Off)
            return new TerminalImpl();

        const ProcessImpl impl = m_setup.m_processImpl == ProcessImpl::Default
                               ? defaultProcessImpl() : m_setup.m_processImpl;
        if (impl == ProcessImpl::QProcess)
            return new QProcessImpl();
        return new ProcessLauncherImpl();
    }

    void setProcessInterface(ProcessInterface *process)
    {
        m_process.reset(process);
        m_process->setParent(this);

        connect(m_process.get(), &ProcessInterface::started,
                this, &QtcProcessPrivate::emitStarted);
        connect(m_process.get(), &ProcessInterface::readyRead,
                this, &QtcProcessPrivate::handleReadyRead);
        connect(m_process.get(), &ProcessInterface::done,
                this, &QtcProcessPrivate::handleDone);
    }

    CommandLine fullCommandLine() const
    {
        if (!m_setup.m_runAsRoot || HostOsInfo::isWindowsHost())
            return m_setup.m_commandLine;
        CommandLine rootCommand("sudo", {"-A"});
        rootCommand.addCommandLineAsArgs(m_setup.m_commandLine);
        return rootCommand;
    }

    Environment fullEnvironment() const
    {
        Environment env;
        if (m_setup.m_haveEnv) {
            if (m_setup.m_environment.size() == 0)
                qWarning("QtcProcess::start: Empty environment set when running '%s'.",
                         qPrintable(m_setup.m_commandLine.executable().toString()));
            env = m_setup.m_environment;
        } else {
            env = Environment::systemEnvironment();
        }

// TODO: needs SshSettings
//        if (m_runAsRoot)
//            RunControl::provideAskPassEntry(env);
        return env;
    }

    QtcProcess *q;
    std::unique_ptr<ProcessInterface> m_process;
    ProcessSetupData m_setup;

    void slotTimeout();
    void handleReadyRead(const QByteArray &outputData, const QByteArray &errorData);
    void handleDone(const ProcessResultData &data);
    void handleError();
    void clearForRun();

    void emitStarted();
    void emitFinished();
    void emitErrorOccurred(QProcess::ProcessError error);
    void emitReadyReadStandardOutput();
    void emitReadyReadStandardError();

    ProcessResult interpretExitCode(int exitCode);

    QProcess::ProcessChannelMode m_processChannelMode = QProcess::SeparateChannels;
    ProcessResultData m_resultData;

    QTextCodec *m_codec = QTextCodec::codecForLocale();
    QEventLoop *m_eventLoop = nullptr;
    ProcessResult m_result = ProcessResult::StartFailed;
    ChannelBuffer m_stdOut;
    ChannelBuffer m_stdErr;
    ExitCodeInterpreter m_exitCodeInterpreter;

    int m_hangTimerCount = 0;
    int m_maxHangTimerCount = defaultMaxHangTimerCount;
    bool m_timeOutMessageBoxEnabled = false;
    bool m_waitingForUser = false;

    class Guard {
    public:
        Guard(int &guard) : m_guard(guard) { ++guard; }
        ~Guard() { --m_guard; }
    private:
        int &m_guard;
    };
    int m_callStackGuard = 0;
};

#define CALL_STACK_GUARD() Guard guard(m_callStackGuard)

void QtcProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdOut.codec = m_codec;
    m_stdErr.clearForRun();
    m_stdErr.codec = m_codec;
    m_result = ProcessResult::StartFailed;
    m_resultData = {};
}

ProcessResult QtcProcessPrivate::interpretExitCode(int exitCode)
{
    if (m_exitCodeInterpreter)
        return m_exitCodeInterpreter(exitCode);

    // default:
    return exitCode ? ProcessResult::FinishedWithError : ProcessResult::FinishedWithSuccess;
}

} // Internal

void ProcessInterface::kickoffProcess()
{
    QTC_CHECK(false);
}

qint64 ProcessInterface::applicationMainThreadID() const
{
    QTC_CHECK(false); return -1;
}

/*!
    \class Utils::QtcProcess

    \brief The QtcProcess class provides functionality for with processes.

    \sa Utils::ProcessArgs
*/

QtcProcess::QtcProcess(QObject *parent)
    : QObject(parent),
    d(new QtcProcessPrivate(this))
{
    static int qProcessExitStatusMeta = qRegisterMetaType<QProcess::ExitStatus>();
    static int qProcessProcessErrorMeta = qRegisterMetaType<QProcess::ProcessError>();
    Q_UNUSED(qProcessExitStatusMeta)
    Q_UNUSED(qProcessProcessErrorMeta)

    if (processLog().isDebugEnabled()) {
        connect(this, &QtcProcess::finished, [this] {
            if (!d->m_process.get())
                return;
            const QVariant n = d->m_process.get()->property(QTC_PROCESS_NUMBER);
            if (!n.isValid())
                return;
            using namespace std::chrono;
            const quint64 msSinceEpoc =
                    duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
            const quint64 msStarted =
                    d->m_process.get()->property(QTC_PROCESS_STARTTIME).toULongLong();
            const quint64 msElapsed = msSinceEpoc - msStarted;

            const int number = n.toInt();
            qCDebug(processLog).nospace() << "Process " << number << " finished: "
                                          << "result=" << int(result())
                                          << ", ex=" << exitCode()
                                          << ", " << stdOut().size() << " bytes stdout: "
                                          << stdOut().left(20)
                                          << ", " << stdErr().size() << " bytes stderr: "
                                          << stdErr().left(1000)
                                          << ", " << msElapsed << " ms elapsed";
            if (processStdoutLog().isDebugEnabled() && !stdOut().isEmpty())
                qCDebug(processStdoutLog).nospace()
                        << "Process " << number << " sdout: " << stdOut();
            if (processStderrLog().isDebugEnabled() && !stdErr().isEmpty())
                qCDebug(processStderrLog).nospace()
                        << "Process " << number << " stderr: " << stdErr();
        });
    }
}

QtcProcess::~QtcProcess()
{
    QTC_CHECK(d->m_callStackGuard == 0);
    delete d;
}

void QtcProcess::emitStarted()
{
    emit started();
}

void QtcProcess::emitFinished()
{
    emit finished();
}

void QtcProcess::setProcessInterface(ProcessInterface *interface)
{
    d->setProcessInterface(interface);
    // Make a copy, don't share, until we get rid of fullCommandLine() and fullEnvironment()
    *d->m_process->m_setup = d->m_setup;
}

void QtcProcess::setProcessImpl(ProcessImpl processImpl)
{
    d->m_setup.m_processImpl = processImpl;
}

ProcessMode QtcProcess::processMode() const
{
    return d->m_setup.m_processMode;
}

void QtcProcess::setTerminalMode(TerminalMode mode)
{
    d->m_setup.m_terminalMode = mode;
}

TerminalMode QtcProcess::terminalMode() const
{
    return d->m_setup.m_terminalMode;
}

void QtcProcess::setProcessMode(ProcessMode processMode)
{
    d->m_setup.m_processMode = processMode;
}

void QtcProcess::setEnvironment(const Environment &env)
{
    d->m_setup.m_environment = env;
    d->m_setup.m_haveEnv = true;
}

void QtcProcess::unsetEnvironment()
{
    d->m_setup.m_environment = Environment();
    d->m_setup.m_haveEnv = false;
}

const Environment &QtcProcess::environment() const
{
    return d->m_setup.m_environment;
}

bool QtcProcess::hasEnvironment() const
{
    return d->m_setup.m_haveEnv;
}

void QtcProcess::setRemoteEnvironment(const Environment &environment)
{
    d->m_setup.m_remoteEnvironment = environment;
}

Environment QtcProcess::remoteEnvironment() const
{
    return d->m_setup.m_remoteEnvironment;
}

void QtcProcess::setCommand(const CommandLine &cmdLine)
{
    if (d->m_setup.m_workingDirectory.needsDevice() && cmdLine.executable().needsDevice()) {
        QTC_CHECK(d->m_setup.m_workingDirectory.host() == cmdLine.executable().host());
    }
    d->m_setup.m_commandLine = cmdLine;
}

const CommandLine &QtcProcess::commandLine() const
{
    return d->m_setup.m_commandLine;
}

FilePath QtcProcess::workingDirectory() const
{
    return d->m_setup.m_workingDirectory;
}

void QtcProcess::setWorkingDirectory(const FilePath &dir)
{
    if (dir.needsDevice() && d->m_setup.m_commandLine.executable().needsDevice()) {
        QTC_CHECK(dir.host() == d->m_setup.m_commandLine.executable().host());
    }
    d->m_setup.m_workingDirectory = dir;
}

void QtcProcess::setUseCtrlCStub(bool enabled)
{
    // Do not use the stub in debug mode. Activating the stub will shut down
    // Qt Creator otherwise, because they share the same Windows console.
    // See QTCREATORBUG-11995 for details.
#ifdef QT_DEBUG
    Q_UNUSED(enabled)
#else
    d->m_setup.m_useCtrlCStub = enabled;
#endif
}

void QtcProcess::start()
{
// TODO: Uncomment when we de-virtualize start()
//    QTC_ASSERT(state() == QProcess::NotRunning, return);

    ProcessInterface *processImpl = nullptr;
    if (d->m_setup.m_commandLine.executable().needsDevice()) {
        if (s_deviceHooks.processImplHook) { // TODO: replace "if" with an assert for the hook
            processImpl = s_deviceHooks.processImplHook(commandLine().executable());
        }
        if (!processImpl) { // TODO: remove this branch when docker is adapted accordingly
            QTC_ASSERT(s_deviceHooks.startProcessHook, return);
            s_deviceHooks.startProcessHook(*this);
            return;
        }
    } else {
        processImpl = d->createProcessInterface();
    }
    QTC_ASSERT(processImpl, return);
    setProcessInterface(processImpl);
    d->clearForRun();
    d->m_process->m_setup->m_commandLine = d->fullCommandLine();
    d->m_process->m_setup->m_environment = d->fullEnvironment();
    if (processLog().isDebugEnabled()) {
        // Pass a dynamic property with info about blocking type
        d->m_process->setProperty(QTC_PROCESS_BLOCKING_TYPE, property(QTC_PROCESS_BLOCKING_TYPE));
    }
    d->m_process->start();
}

void QtcProcess::terminate()
{
    if (d->m_process)
        d->m_process->terminate();
}

void QtcProcess::interrupt()
{
    if (d->m_process)
        d->m_process->interrupt();
}

bool QtcProcess::startDetached(const CommandLine &cmd, const FilePath &workingDirectory, qint64 *pid)
{
    return QProcess::startDetached(cmd.executable().toUserOutput(),
                                   cmd.splitArguments(),
                                   workingDirectory.toUserOutput(),
                                   pid);
}

void QtcProcess::setLowPriority()
{
    d->m_setup.m_lowPriority = true;
}

void QtcProcess::setDisableUnixTerminal()
{
    d->m_setup.m_unixTerminalDisabled = true;
}

void QtcProcess::setAbortOnMetaChars(bool abort)
{
    d->m_setup.m_abortOnMetaChars = abort;
}

void QtcProcess::setRunAsRoot(bool on)
{
    d->m_setup.m_runAsRoot = on;
}

bool QtcProcess::isRunAsRoot() const
{
    return d->m_setup.m_runAsRoot;
}

void QtcProcess::setStandardInputFile(const QString &inputFile)
{
    d->m_setup.m_standardInputFile = inputFile;
}

QString QtcProcess::toStandaloneCommandLine() const
{
    QStringList parts;
    parts.append("/usr/bin/env");
    if (!d->m_setup.m_workingDirectory.isEmpty()) {
        parts.append("-C");
        d->m_setup.m_workingDirectory.path();
    }
    parts.append("-i");
    if (d->m_setup.m_environment.size() > 0) {
        const QStringList envVars = d->m_setup.m_environment.toStringList();
        std::transform(envVars.cbegin(), envVars.cend(),
                       std::back_inserter(parts), ProcessArgs::quoteArgUnix);
    }
    parts.append(d->m_setup.m_commandLine.executable().path());
    parts.append(d->m_setup.m_commandLine.splitArguments());
    return parts.join(" ");
}

void QtcProcess::setExtraData(const QString &key, const QVariant &value)
{
    d->m_setup.m_extraData.insert(key, value);
}

QVariant QtcProcess::extraData(const QString &key) const
{
    return d->m_setup.m_extraData.value(key);
}

void QtcProcess::setExtraData(const QVariantHash &extraData)
{
    d->m_setup.m_extraData = extraData;
}

QVariantHash QtcProcess::extraData() const
{
    return d->m_setup.m_extraData;
}

void QtcProcess::setRemoteProcessHooks(const DeviceProcessHooks &hooks)
{
    s_deviceHooks = hooks;
}

void QtcProcess::stopProcess()
{
    if (state() == QProcess::NotRunning)
        return;
    terminate();
    if (waitForFinished(300))
        return;
    kill();
    waitForFinished(300);
}

static bool askToKill(const QString &command)
{
#ifdef QT_GUI_LIB
    if (QThread::currentThread() != QCoreApplication::instance()->thread())
        return true;
    const QString title = QtcProcess::tr("Process Not Responding");
    QString msg = command.isEmpty() ?
                QtcProcess::tr("The process is not responding.") :
                QtcProcess::tr("The process \"%1\" is not responding.").arg(command);
    msg += ' ';
    msg += QtcProcess::tr("Terminate the process?");
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
// detection (taking effect after no more output
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

    // Keep the process running until it has no longer has data
    bool finished = false;
    bool hasData = false;
    do {
        finished = waitForFinished(timeoutS > 0 ? timeoutS * 1000 : -1)
                || state() == QProcess::NotRunning;
        // First check 'stdout'
        const QByteArray newStdOut = readAllStandardOutput();
        if (!newStdOut.isEmpty()) {
            hasData = true;
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
        hasData = hang && showTimeOutMessageBox && !askToKill(d->m_setup.m_commandLine.executable().path());
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

ProcessResult QtcProcess::result() const
{
    return d->m_result;
}

void QtcProcess::setResult(const ProcessResult &result)
{
    d->m_result = result;
}

ProcessResultData QtcProcess::resultData() const
{
    return d->m_resultData;
}

int QtcProcess::exitCode() const
{
    return resultData().m_exitCode;
}

QProcess::ExitStatus QtcProcess::exitStatus() const
{
    return resultData().m_exitStatus;
}

QProcess::ProcessError QtcProcess::error() const
{
    return resultData().m_error;
}

QString QtcProcess::errorString() const
{
    return resultData().m_errorString;
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

void QtcProcess::kickoffProcess()
{
    if (d->m_process)
        d->m_process->kickoffProcess();
}

qint64 QtcProcess::applicationMainThreadID() const
{
    if (d->m_process)
        return d->m_process->applicationMainThreadID();
    return -1;
}

void QtcProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    QTC_CHECK(state() == QProcess::NotRunning);
    d->m_processChannelMode = mode;
}

QProcess::ProcessState QtcProcess::state() const
{
    if (d->m_process)
        return d->m_process->state();
    return QProcess::NotRunning;
}

bool QtcProcess::isRunning() const
{
    return state() == QProcess::Running;
}

qint64 QtcProcess::processId() const
{
    if (d->m_process)
        return d->m_process->processId();
    return 0;
}

bool QtcProcess::waitForStarted(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    return s_waitForStarted.measureAndRun(&ProcessInterface::waitForStarted, d->m_process, msecs);
}

bool QtcProcess::waitForReadyRead(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    return d->m_process->waitForReadyRead(msecs);
}

bool QtcProcess::waitForFinished(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    return d->m_process->waitForFinished(msecs);
}

QByteArray QtcProcess::readAllStandardOutput()
{
    QByteArray buf = d->m_stdOut.rawData;
    d->m_stdOut.rawData.clear();
    return buf;
}

QByteArray QtcProcess::readAllStandardError()
{
    QByteArray buf = d->m_stdErr.rawData;
    d->m_stdErr.rawData.clear();
    return buf;
}

void QtcProcess::kill()
{
    if (d->m_process)
        d->m_process->kill();
}

qint64 QtcProcess::write(const QByteArray &input)
{
    QTC_ASSERT(processMode() == ProcessMode::Writer, return -1);
    QTC_ASSERT(d->m_process, return -1);
    return d->m_process->write(input);
}

void QtcProcess::close()
{
    if (d->m_process)
        d->m_process->close();
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

    The callbacks set with setStdOutCallback(), setStdErrCallback() are called
    with complete lines based on the '\\n' marker.
    They would typically be used for log windows.

    Alternatively you can used setStdOutLineCallback() and setStdErrLineCallback()
    to process the output line by line.

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

QString QtcProcess::exitMessage() const
{
    const QString fullCmd = commandLine().toUserOutput();
    switch (result()) {
    case ProcessResult::FinishedWithSuccess:
        return QtcProcess::tr("The command \"%1\" finished successfully.").arg(fullCmd);
    case ProcessResult::FinishedWithError:
        return QtcProcess::tr("The command \"%1\" terminated with exit code %2.")
            .arg(fullCmd).arg(exitCode());
    case ProcessResult::TerminatedAbnormally:
        return QtcProcess::tr("The command \"%1\" terminated abnormally.").arg(fullCmd);
    case ProcessResult::StartFailed:
        return QtcProcess::tr("The command \"%1\" could not be started.").arg(fullCmd);
    case ProcessResult::Hang:
        return QtcProcess::tr("The command \"%1\" did not respond within the timeout limit (%2 s).")
            .arg(fullCmd).arg(d->m_maxHangTimerCount);
    }
    return QString();
}

QByteArray QtcProcess::allRawOutput() const
{
    QTC_CHECK(d->m_stdOut.keepRawData);
    QTC_CHECK(d->m_stdErr.keepRawData);
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
    QTC_CHECK(d->m_stdOut.keepRawData);
    QTC_CHECK(d->m_stdErr.keepRawData);
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
    QTC_CHECK(d->m_stdOut.keepRawData);
    return normalizeNewlines(d->m_codec->toUnicode(d->m_stdOut.rawData));
}

QString QtcProcess::stdErr() const
{
    // FIXME: The tighter check below is actually good theoretically, but currently
    // ShellCommand::runFullySynchronous triggers it and disentangling there
    // is not trivial. So weaken it a bit for now.
    //QTC_CHECK(d->m_stdErr.keepRawData);
    QTC_CHECK(d->m_stdErr.keepRawData || d->m_stdErr.rawData.isEmpty());
    return normalizeNewlines(d->m_codec->toUnicode(d->m_stdErr.rawData));
}

QByteArray QtcProcess::rawStdOut() const
{
    QTC_CHECK(d->m_stdOut.keepRawData);
    return d->m_stdOut.rawData;
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const QtcProcess &r)
{
    QDebug nsp = str.nospace();
    nsp << "QtcProcess: result="
        << int(r.d->m_result) << " ex=" << r.exitCode() << '\n'
        << r.d->m_stdOut.rawData.size() << " bytes stdout, stderr=" << r.d->m_stdErr.rawData << '\n';
    return str;
}

void ChannelBuffer::clearForRun()
{
    rawData.clear();
    codecState.reset(new QTextCodec::ConverterState);
    incompleteLineBuffer.clear();
}

/* Check for complete lines read from the device and return them, moving the
 * buffer position. */
void ChannelBuffer::append(const QByteArray &text)
{
    if (text.isEmpty())
        return;

    if (keepRawData)
        rawData += text;

    // Line-wise operation below:
    if (!outputCallback)
        return;

    // Convert and append the new input to the buffer of incomplete lines
    incompleteLineBuffer.append(codec->toUnicode(text.constData(), text.size(), codecState.get()));

    do {
        // Any completed lines in the incompleteLineBuffer?
        int pos = -1;
        if (emitSingleLines) {
            const int posn = incompleteLineBuffer.indexOf('\n');
            const int posr = incompleteLineBuffer.indexOf('\r');
            if (posn != -1) {
                if (posr != -1) {
                    if (posn == posr + 1)
                        pos = posn; // \r followed by \n -> line end, use the \n.
                    else
                        pos = qMin(posr, posn); // free floating \r and \n: Use the first one.
                } else {
                    pos = posn;
                }
            } else {
                pos = posr; // Make sure internal '\r' triggers a line output
            }
        } else {
            pos = qMax(incompleteLineBuffer.lastIndexOf('\n'),
                       incompleteLineBuffer.lastIndexOf('\r'));
        }

        if (pos == -1)
            break;

        // Get completed lines and remove them from the incompleteLinesBuffer:
        const QString line = QtcProcess::normalizeNewlines(incompleteLineBuffer.left(pos + 1));
        incompleteLineBuffer = incompleteLineBuffer.mid(pos + 1);

        QTC_ASSERT(outputCallback, return);
        outputCallback(line);

        if (!emitSingleLines)
            break;
    } while (true);
}

void ChannelBuffer::handleRest()
{
    if (outputCallback && !incompleteLineBuffer.isEmpty()) {
        outputCallback(incompleteLineBuffer);
        incompleteLineBuffer.clear();
    }
}

void QtcProcess::setTimeoutS(int timeoutS)
{
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
    d->m_timeOutMessageBoxEnabled = v;
}

void QtcProcess::setExitCodeInterpreter(const ExitCodeInterpreter &interpreter)
{
    d->m_exitCodeInterpreter = interpreter;
}

void QtcProcess::setWriteData(const QByteArray &writeData)
{
    d->m_setup.m_writeData = writeData;
}

#ifdef QT_GUI_LIB
static bool isGuiThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
}
#endif

void QtcProcess::runBlocking(EventLoopMode eventLoopMode)
{
    // FIXME: Implement properly

    if (d->m_setup.m_commandLine.executable().needsDevice()) {
        QtcProcess::start();
        waitForFinished();
        return;
    };

    if (processLog().isDebugEnabled()) {
        // Attach a dynamic property with info about blocking type
        setProperty(QTC_PROCESS_BLOCKING_TYPE, int(eventLoopMode));
    }

    QtcProcess::start();
    if (processLog().isDebugEnabled()) {
        // Remove the dynamic property so that it's not reused in subseqent start()
        setProperty(QTC_PROCESS_BLOCKING_TYPE, QVariant());
    }
    if (eventLoopMode == EventLoopMode::On) {
        // Start failure is triggered immediately if the executable cannot be found in the path.
        // In this case the process is left in NotRunning state.
        // Do not start the event loop in that case.
        if (state() == QProcess::Starting) {
            QTimer timer(this);
            connect(&timer, &QTimer::timeout, d, &QtcProcessPrivate::slotTimeout);
            timer.setInterval(1000);
            timer.start();
#ifdef QT_GUI_LIB
            if (isGuiThread())
                QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
            QEventLoop eventLoop(this);
            QTC_ASSERT(!d->m_eventLoop, return);
            d->m_eventLoop = &eventLoop;
            eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
            d->m_eventLoop = nullptr;
            timer.stop();
#ifdef QT_GUI_LIB
            if (isGuiThread())
                QApplication::restoreOverrideCursor();
#endif
        }
    } else {
        if (!waitForStarted(d->m_maxHangTimerCount * 1000)) {
            d->m_result = ProcessResult::StartFailed;
            return;
        }
        if (!waitForFinished(d->m_maxHangTimerCount * 1000)) {
            d->m_result = ProcessResult::Hang;
            terminate();
            if (!waitForFinished(1000)) {
                kill();
                waitForFinished(1000);
            }
        }
    }
}

void QtcProcess::setStdOutCallback(const std::function<void (const QString &)> &callback)
{
    d->m_stdOut.outputCallback = callback;
    d->m_stdOut.emitSingleLines = false;
}

void QtcProcess::setStdOutLineCallback(const std::function<void (const QString &)> &callback)
{
    d->m_stdOut.outputCallback = callback;
    d->m_stdOut.emitSingleLines = true;
    d->m_stdOut.keepRawData = false;
}

void QtcProcess::setStdErrCallback(const std::function<void (const QString &)> &callback)
{
    d->m_stdErr.outputCallback = callback;
    d->m_stdErr.emitSingleLines = false;
}

void QtcProcess::setStdErrLineCallback(const std::function<void (const QString &)> &callback)
{
    d->m_stdErr.outputCallback = callback;
    d->m_stdErr.emitSingleLines = true;
    d->m_stdErr.keepRawData = false;
}

void QtcProcessPrivate::slotTimeout()
{
    if (!m_waitingForUser && (++m_hangTimerCount > m_maxHangTimerCount)) {
        if (debug)
            qDebug() << Q_FUNC_INFO << "HANG detected, killing";
        m_waitingForUser = true;
        const bool terminate = !m_timeOutMessageBoxEnabled
            || askToKill(m_setup.m_commandLine.executable().toString());
        m_waitingForUser = false;
        if (terminate) {
            q->stopProcess();
            m_result = ProcessResult::Hang;
        } else {
            m_hangTimerCount = 0;
        }
    } else {
        if (debug)
            qDebug() << Q_FUNC_INFO << m_hangTimerCount;
    }
}

void QtcProcessPrivate::handleReadyRead(const QByteArray &outputData, const QByteArray &errorData)
{
    // TODO: check why we need this timer?
    m_hangTimerCount = 0;
    // TODO: store a copy of m_processChannelMode on start()? Currently we assert that state
    // is NotRunning when setting the process channel mode.
    if (m_processChannelMode == QProcess::MergedChannels) {
        m_stdOut.append(outputData);
        m_stdOut.append(errorData);
        if (!outputData.isEmpty() || !errorData.isEmpty())
            emitReadyReadStandardOutput();
    } else {
        if (m_processChannelMode == QProcess::ForwardedOutputChannel
                || m_processChannelMode == QProcess::ForwardedChannels) {
            std::cout << outputData.constData() << std::flush;
        } else {
            m_stdOut.append(outputData);
            if (!outputData.isEmpty())
                emitReadyReadStandardOutput();
        }
        if (m_processChannelMode == QProcess::ForwardedErrorChannel
                || m_processChannelMode == QProcess::ForwardedChannels) {
            std::cerr << errorData.constData() << std::flush;
        } else {
            m_stdErr.append(errorData);
            if (!errorData.isEmpty())
                emitReadyReadStandardError();
        }
    }
}

void QtcProcessPrivate::handleDone(const ProcessResultData &data)
{
    m_resultData = data;

    // This code (255) is being returned by QProcess when FailedToStart error occurred
    if (m_resultData.m_error == QProcess::FailedToStart)
        m_resultData.m_exitCode = 0xFF;

    // HACK: See QIODevice::errorString() implementation.
    if (m_resultData.m_error == QProcess::UnknownError)
        m_resultData.m_errorString.clear();

    if (m_resultData.m_error != QProcess::UnknownError)
        handleError();

    if (debug)
        qDebug() << Q_FUNC_INFO << m_resultData.m_exitCode << m_resultData.m_exitStatus;
    m_hangTimerCount = 0;

    switch (m_resultData.m_exitStatus) {
    case QProcess::NormalExit:
        m_result = interpretExitCode(m_resultData.m_exitCode);
        break;
    case QProcess::CrashExit:
        // Was hang detected before and killed?
        if (m_result != ProcessResult::Hang)
            m_result = ProcessResult::TerminatedAbnormally;
        break;
    }
    if (m_eventLoop)
        m_eventLoop->quit();

    m_stdOut.handleRest();
    m_stdErr.handleRest();

    if (m_resultData.m_error != QProcess::FailedToStart)
        emitFinished();

    emit q->done();
}

void QtcProcessPrivate::handleError()
{
    m_hangTimerCount = 0;
    if (debug)
        qDebug() << Q_FUNC_INFO << m_resultData.m_error;
    // Was hang detected before and killed?
    if (m_result != ProcessResult::Hang)
        m_result = ProcessResult::StartFailed;
    emitErrorOccurred(m_resultData.m_error);
}

void QtcProcessPrivate::emitStarted()
{
    CALL_STACK_GUARD();
    q->emitStarted();
}

void QtcProcessPrivate::emitFinished()
{
    CALL_STACK_GUARD();
    q->emitFinished();
}

void QtcProcessPrivate::emitErrorOccurred(QProcess::ProcessError error)
{
    CALL_STACK_GUARD();
    emit q->errorOccurred(error);
}

void QtcProcessPrivate::emitReadyReadStandardOutput()
{
    CALL_STACK_GUARD();
    emit q->readyReadStandardOutput();
}

void QtcProcessPrivate::emitReadyReadStandardError()
{
    CALL_STACK_GUARD();
    emit q->readyReadStandardError();
}

} // namespace Utils

#include "qtcprocess.moc"
