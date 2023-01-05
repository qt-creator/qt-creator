// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qtcprocess.h"

#include "algorithm.h"
#include "environment.h"
#include "guard.h"
#include "hostosinfo.h"
#include "launcherinterface.h"
#include "launchersocket.h"
#include "processreaper.h"
#include "processutils.h"
#include "terminalprocess_p.h"
#include "threadutils.h"

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
        , m_measureProcess(qtcEnvironmentVariableIsSet("QTC_MEASURE_PROCESS"))
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
            const bool mainThread = isMainThread();
            const int hitThisAll = m_hitThisAll.fetch_add(1) + 1;
            const int hitAllAll = m_hitAllAll.fetch_add(1) + 1;
            const int hitThisMain = mainThread
                    ? m_hitThisMain.fetch_add(1) + 1
                    : m_hitThisMain.load();
            const int hitAllMain = mainThread
                    ? m_hitAllMain.fetch_add(1) + 1
                    : m_hitAllMain.load();
            const qint64 totalThisAll = toMs(m_totalThisAll.fetch_add(currentNsecs) + currentNsecs);
            const qint64 totalAllAll = toMs(m_totalAllAll.fetch_add(currentNsecs) + currentNsecs);
            const qint64 totalThisMain = toMs(mainThread
                    ? m_totalThisMain.fetch_add(currentNsecs) + currentNsecs
                    : m_totalThisMain.load());
            const qint64 totalAllMain = toMs(mainThread
                    ? m_totalAllMain.fetch_add(currentNsecs) + currentNsecs
                    : m_totalAllMain.load());
            printMeasurement(QLatin1String(m_functionName), hitThisAll, toMs(currentNsecs),
                             totalThisAll, hitAllAll, totalAllAll, mainThread,
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
    static QString formatField(int number, int fieldWidth, const QString &suffix = {})
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

    QByteArray readAllData() { return std::exchange(rawData, {}); }

    QByteArray rawData;
    QString incompleteLineBuffer; // lines not yet signaled
    QTextCodec *codec = nullptr; // Not owner
    std::unique_ptr<QTextCodec::ConverterState> codecState;
    std::function<void(const QString &lines)> outputCallback;
    TextChannelMode m_textChannelMode = TextChannelMode::Off;

    bool emitSingleLines = true;
    bool keepRawData = true;
};

class DefaultImpl : public ProcessInterface
{
private:
    virtual void start() final;
    virtual void doDefaultStart(const QString &program, const QStringList &arguments) = 0;
    bool dissolveCommand(QString *program, QStringList *arguments);
    bool ensureProgramExists(const QString &program);
};

void DefaultImpl::start()
{
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
    const CommandLine &commandLine = m_setup.m_commandLine;
    QString commandString;
    ProcessArgs processArgs;
    const bool success = ProcessArgs::prepareCommand(commandLine, &commandString, &processArgs,
                                                     &m_setup.m_environment,
                                                     &m_setup.m_workingDirectory);

    if (commandLine.executable().osType() == OsTypeWindows) {
        QString args;
        if (m_setup.m_useCtrlCStub) {
            if (m_setup.m_lowPriority)
                ProcessArgs::addArg(&args, "-nice");
            ProcessArgs::addArg(&args, QDir::toNativeSeparators(commandString));
            commandString = QCoreApplication::applicationDirPath()
                    + QLatin1String("/qtcreator_ctrlc_stub.exe");
        } else if (m_setup.m_lowPriority) {
            m_setup.m_belowNormalPriority = true;
        }
        ProcessArgs::addArgs(&args, processArgs.toWindowsArgs());
        m_setup.m_nativeArguments = args;
        // Note: Arguments set with setNativeArgs will be appended to the ones
        // passed with start() below.
        *arguments = {};
    } else {
        if (!success) {
            const ProcessResultData result = {0,
                                              QProcess::NormalExit,
                                              QProcess::FailedToStart,
                                              QtcProcess::tr("Error in command line.")};
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
    const FilePath programFilePath = resolve(m_setup.m_workingDirectory,
                                             FilePath::fromString(program));
    if (programFilePath.exists() && programFilePath.isExecutableFile())
        return true;

    const QString errorString
        = QtcProcess::tr("The program \"%1\" does not exist or is not executable.").arg(program);
    const ProcessResultData result = { 0, QProcess::NormalExit, QProcess::FailedToStart,
                                       errorString };
    emit done(result);
    return false;
}

class QProcessBlockingImpl : public ProcessBlockingInterface
{
public:
    QProcessBlockingImpl(QProcess *process) : m_process(process) {}

private:
    bool waitForSignal(ProcessSignalType signalType, int msecs) final
    {
        switch (signalType) {
        case ProcessSignalType::Started:
            return m_process->waitForStarted(msecs);
        case ProcessSignalType::ReadyRead:
            return m_process->waitForReadyRead(msecs);
        case ProcessSignalType::Done:
            return m_process->waitForFinished(msecs);
        }
        return false;
    }

    QProcess *m_process = nullptr;
};

class QProcessImpl final : public DefaultImpl
{
public:
    QProcessImpl()
        : m_process(new ProcessHelper(this))
        , m_blockingImpl(new QProcessBlockingImpl(m_process))
    {
        connect(m_process, &QProcess::started, this, &QProcessImpl::handleStarted);
        connect(m_process, &QProcess::finished, this, &QProcessImpl::handleFinished);
        connect(m_process, &QProcess::errorOccurred, this, &QProcessImpl::handleError);
        connect(m_process, &QProcess::readyReadStandardOutput, this, [this] {
            emit readyRead(m_process->readAllStandardOutput(), {});
        });
        connect(m_process, &QProcess::readyReadStandardError, this, [this] {
            emit readyRead({}, m_process->readAllStandardError());
        });
    }
    ~QProcessImpl() final { ProcessReaper::reap(m_process, m_setup.m_reaperTimeout); }

private:
    qint64 write(const QByteArray &data) final { return m_process->write(data); }
    void sendControlSignal(ControlSignal controlSignal) final {
        switch (controlSignal) {
        case ControlSignal::Terminate:
            ProcessHelper::terminateProcess(m_process);
            break;
        case ControlSignal::Kill:
            m_process->kill();
            break;
        case ControlSignal::Interrupt:
            ProcessHelper::interruptProcess(m_process);
            break;
        case ControlSignal::KickOff:
            QTC_CHECK(false);
            break;
        }
    }

    virtual ProcessBlockingInterface *processBlockingInterface() const { return m_blockingImpl; }

    void doDefaultStart(const QString &program, const QStringList &arguments) final
    {
        QTC_ASSERT(QThread::currentThread()->eventDispatcher(),
                   qWarning("QtcProcess::start(): Starting a process in a non QThread thread "
                            "may cause infinite hang when destroying the running process."));
        ProcessStartHandler *handler = m_process->processStartHandler();
        handler->setProcessMode(m_setup.m_processMode);
        handler->setWriteData(m_setup.m_writeData);
        if (m_setup.m_belowNormalPriority)
            handler->setBelowNormalPriority();
        handler->setNativeArguments(m_setup.m_nativeArguments);
        m_process->setProcessEnvironment(m_setup.m_environment.toProcessEnvironment());
        m_process->setWorkingDirectory(m_setup.m_workingDirectory.path());
        m_process->setStandardInputFile(m_setup.m_standardInputFile);
        m_process->setProcessChannelMode(m_setup.m_processChannelMode);
        if (m_setup.m_lowPriority)
            m_process->setLowPriority();
        if (m_setup.m_unixTerminalDisabled)
            m_process->setUnixTerminalDisabled();
        m_process->setUseCtrlCStub(m_setup.m_useCtrlCStub);
        m_process->start(program, arguments, handler->openMode());
        handler->handleProcessStart();
    }

    void handleStarted()
    {
        m_process->processStartHandler()->handleProcessStarted();
        emit started(m_process->processId());
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

    ProcessHelper *m_process = nullptr;
    QProcessBlockingImpl *m_blockingImpl = nullptr;
};

static uint uniqueToken()
{
    static std::atomic_uint globalUniqueToken = 0;
    return ++globalUniqueToken;
}

class ProcessLauncherBlockingImpl : public ProcessBlockingInterface
{
public:
    ProcessLauncherBlockingImpl(CallerHandle *caller) : m_caller(caller) {}

private:
    bool waitForSignal(ProcessSignalType signalType, int msecs) final
    {
        // TODO: Remove CallerHandle::SignalType
        const CallerHandle::SignalType type = [signalType] {
            switch (signalType) {
            case ProcessSignalType::Started:
                return CallerHandle::SignalType::Started;
            case ProcessSignalType::ReadyRead:
                return CallerHandle::SignalType::ReadyRead;
            case ProcessSignalType::Done:
                return CallerHandle::SignalType::Done;
            }
            QTC_CHECK(false);
            return CallerHandle::SignalType::NoSignal;
        }();
        return m_caller->waitForSignal(type, msecs);
    }

    CallerHandle *m_caller = nullptr;
};

class ProcessLauncherImpl final : public DefaultImpl
{
    Q_OBJECT
public:
    ProcessLauncherImpl() : m_token(uniqueToken())
    {
        m_handle = LauncherInterface::registerHandle(this, token());
        m_handle->setProcessSetupData(&m_setup);
        connect(m_handle, &CallerHandle::started,
                this, &ProcessInterface::started);
        connect(m_handle, &CallerHandle::readyRead,
                this, &ProcessInterface::readyRead);
        connect(m_handle, &CallerHandle::done,
                this, &ProcessInterface::done);
        m_blockingImpl = new ProcessLauncherBlockingImpl(m_handle);
    }
    ~ProcessLauncherImpl() final
    {
        m_handle->close();
        LauncherInterface::unregisterHandle(token());
        m_handle = nullptr;
    }

private:
    qint64 write(const QByteArray &data) final { return m_handle->write(data); }
    void sendControlSignal(ControlSignal controlSignal) final {
        switch (controlSignal) {
        case ControlSignal::Terminate:
            m_handle->terminate();
            break;
        case ControlSignal::Kill:
            m_handle->kill();
            break;
        case ControlSignal::Interrupt:
            ProcessHelper::interruptPid(m_handle->processId());
            break;
        case ControlSignal::KickOff:
            QTC_CHECK(false);
            break;
        }
    }

    virtual ProcessBlockingInterface *processBlockingInterface() const { return m_blockingImpl; }

    void doDefaultStart(const QString &program, const QStringList &arguments) final
    {
        m_handle->start(program, arguments);
    }

    quintptr token() const { return m_token; }

    const uint m_token = 0;
    // Lives in caller's thread.
    CallerHandle *m_handle = nullptr;
    ProcessLauncherBlockingImpl *m_blockingImpl = nullptr;
};

static ProcessImpl defaultProcessImpl()
{
    if (qtcEnvironmentVariableIsSet("QTC_USE_QPROCESS"))
        return ProcessImpl::QProcess;
    return ProcessImpl::ProcessLauncher;
}

class ProcessInterfaceSignal
{
public:
    ProcessSignalType signalType() const { return m_signalType; }
    virtual ~ProcessInterfaceSignal() = default;
protected:
    ProcessInterfaceSignal(ProcessSignalType signalType) : m_signalType(signalType) {}
private:
    const ProcessSignalType m_signalType;
};

class StartedSignal : public ProcessInterfaceSignal
{
public:
    StartedSignal(qint64 processId, qint64 applicationMainThreadId)
        : ProcessInterfaceSignal(ProcessSignalType::Started)
        , m_processId(processId)
        , m_applicationMainThreadId(applicationMainThreadId) {}
    qint64 processId() const { return m_processId; }
    qint64 applicationMainThreadId() const { return m_applicationMainThreadId; }
private:
    const qint64 m_processId;
    const qint64 m_applicationMainThreadId;
};

class ReadyReadSignal : public ProcessInterfaceSignal
{
public:
    ReadyReadSignal(const QByteArray &stdOut, const QByteArray &stdErr)
        : ProcessInterfaceSignal(ProcessSignalType::ReadyRead)
        , m_stdOut(stdOut)
        , m_stdErr(stdErr) {}
    QByteArray stdOut() const { return m_stdOut; }
    QByteArray stdErr() const { return m_stdErr; }
private:
    const QByteArray m_stdOut;
    const QByteArray m_stdErr;
};

class DoneSignal : public ProcessInterfaceSignal
{
public:
    DoneSignal(const ProcessResultData &resultData)
        : ProcessInterfaceSignal(ProcessSignalType::Done)
        , m_resultData(resultData) {}
    ProcessResultData resultData() const { return m_resultData; }
private:
    const ProcessResultData m_resultData;
};

class GeneralProcessBlockingImpl;

class ProcessInterfaceHandler : public QObject
{
public:
    ProcessInterfaceHandler(GeneralProcessBlockingImpl *caller, ProcessInterface *process);

    // Called from caller's thread exclusively.
    bool waitForSignal(ProcessSignalType newSignal, int msecs);
    void moveToCallerThread();

private:
    // Called from caller's thread exclusively.
    bool doWaitForSignal(QDeadlineTimer deadline);

    // Called from caller's thread when not waiting for signal,
    // otherwise called from temporary thread.
    void handleStarted(qint64 processId, qint64 applicationMainThreadId);
    void handleReadyRead(const QByteArray &outputData, const QByteArray &errorData);
    void handleDone(const ProcessResultData &data);
    void appendSignal(ProcessInterfaceSignal *newSignal);

    GeneralProcessBlockingImpl *m_caller = nullptr;
    QMutex m_mutex;
    QWaitCondition m_waitCondition;
};

class GeneralProcessBlockingImpl : public ProcessBlockingInterface
{
public:
    GeneralProcessBlockingImpl(QtcProcessPrivate *parent);

    void flush() { flushSignals(takeAllSignals()); }
    bool flushFor(ProcessSignalType signalType) {
        return flushSignals(takeSignalsFor(signalType), &signalType);
    }

    bool shouldFlush() const { QMutexLocker locker(&m_mutex); return !m_signals.isEmpty(); }
    // Called from ProcessInterfaceHandler thread exclusively.
    void appendSignal(ProcessInterfaceSignal *launcherSignal);

private:
    // Called from caller's thread exclusively
    bool waitForSignal(ProcessSignalType newSignal, int msecs) final;

    QList<ProcessInterfaceSignal *> takeAllSignals();
    QList<ProcessInterfaceSignal *> takeSignalsFor(ProcessSignalType signalType);
    bool flushSignals(const QList<ProcessInterfaceSignal *> &signalList,
                      ProcessSignalType *signalType = nullptr);

    void handleStartedSignal(const StartedSignal *launcherSignal);
    void handleReadyReadSignal(const ReadyReadSignal *launcherSignal);
    void handleDoneSignal(const DoneSignal *launcherSignal);

    QtcProcessPrivate *m_caller = nullptr;
    std::unique_ptr<ProcessInterfaceHandler> m_processHandler;
    mutable QMutex m_mutex;
    QList<ProcessInterfaceSignal *> m_signals;
};

class QtcProcessPrivate : public QObject
{
public:
    explicit QtcProcessPrivate(QtcProcess *parent)
        : QObject(parent)
        , q(parent)
        , m_killTimer(this)
    {
        m_setup.m_controlEnvironment = Environment::systemEnvironment();
        m_killTimer.setSingleShot(true);
        connect(&m_killTimer, &QTimer::timeout, this, [this] {
            m_killTimer.stop();
            sendControlSignal(ControlSignal::Kill);
        });
        setupDebugLog();
    }

    void setupDebugLog();
    void storeEventLoopDebugInfo(const QVariant &value);

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
        if (m_process)
            m_process->disconnect();
        m_process.reset(process);
        m_process->setParent(this);
        connect(m_process.get(), &ProcessInterface::started,
                this, &QtcProcessPrivate::handleStarted);
        connect(m_process.get(), &ProcessInterface::readyRead,
                this, &QtcProcessPrivate::handleReadyRead);
        connect(m_process.get(), &ProcessInterface::done,
                this, &QtcProcessPrivate::handleDone);

        m_blockingInterface.reset(process->processBlockingInterface());
        if (!m_blockingInterface)
            m_blockingInterface.reset(new GeneralProcessBlockingImpl(this));
        m_blockingInterface->setParent(this);
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
        Environment env = m_setup.m_environment;
        if (!env.isValid()) {
// FIXME: Either switch to using EnvironmentChange instead of full Environments, or
// feed the full environment into the QtcProcess instead of fixing it up here.
//            qWarning("QtcProcess::start: Empty environment set when running '%s'.",
//                 qPrintable(m_setup.m_commandLine.executable().toString()));
            env = m_setup.m_commandLine.executable().deviceEnvironment();
        }
        // TODO: needs SshSettings
        //        if (m_runAsRoot)
        //            RunControl::provideAskPassEntry(env);
        return env;
    }

    QtcProcess *q;
    std::unique_ptr<ProcessBlockingInterface> m_blockingInterface;
    std::unique_ptr<ProcessInterface> m_process;
    ProcessSetupData m_setup;

    void slotTimeout();
    void handleStarted(qint64 processId, qint64 applicationMainThreadId);
    void handleReadyRead(const QByteArray &outputData, const QByteArray &errorData);
    void handleDone(const ProcessResultData &data);
    void clearForRun();

    void emitGuardedSignal(void (QtcProcess::* signalName)()) {
        GuardLocker locker(m_guard);
        emit (q->*signalName)();
    }

    ProcessResult interpretExitCode(int exitCode);

    bool waitForSignal(ProcessSignalType signalType, int msecs);
    Qt::ConnectionType connectionType() const;
    void sendControlSignal(ControlSignal controlSignal);

    QTimer m_killTimer;
    QProcess::ProcessState m_state = QProcess::NotRunning;
    qint64 m_processId = 0;
    qint64 m_applicationMainThreadId = 0;
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

    Guard m_guard;
};

ProcessInterfaceHandler::ProcessInterfaceHandler(GeneralProcessBlockingImpl *caller,
                                                 ProcessInterface *process)
    : m_caller(caller)
{
    process->disconnect();
    connect(process, &ProcessInterface::started,
            this, &ProcessInterfaceHandler::handleStarted);
    connect(process, &ProcessInterface::readyRead,
            this, &ProcessInterfaceHandler::handleReadyRead);
    connect(process, &ProcessInterface::done,
            this, &ProcessInterfaceHandler::handleDone);
}

// Called from caller's thread exclusively.
bool ProcessInterfaceHandler::waitForSignal(ProcessSignalType newSignal, int msecs)
{
    QDeadlineTimer deadline(msecs);
    while (true) {
        if (deadline.hasExpired())
            break;
        if (!doWaitForSignal(deadline))
            break;
        // Matching (or Done) signal was flushed
        if (m_caller->flushFor(newSignal))
            return true;
        // Otherwise continue awaiting (e.g. when ReadyRead came while waitForFinished())
    }
    return false;
}

// Called from caller's thread exclusively.
void ProcessInterfaceHandler::moveToCallerThread()
{
    QMetaObject::invokeMethod(this, [this] {
        moveToThread(m_caller->thread());
    }, Qt::BlockingQueuedConnection);
}

// Called from caller's thread exclusively.
bool ProcessInterfaceHandler::doWaitForSignal(QDeadlineTimer deadline)
{
    QMutexLocker locker(&m_mutex);

    // Flush, if we have any stored signals.
    // This must be called when holding laucher's mutex locked prior to the call to wait,
    // so that it's done atomically.
    if (m_caller->shouldFlush())
        return true;

    return m_waitCondition.wait(&m_mutex, deadline);
}

// Called from ProcessInterfaceHandler thread exclusively
void ProcessInterfaceHandler::handleStarted(qint64 processId, qint64 applicationMainThreadId)
{
    appendSignal(new StartedSignal(processId, applicationMainThreadId));
}

// Called from ProcessInterfaceHandler thread exclusively
void ProcessInterfaceHandler::handleReadyRead(const QByteArray &outputData, const QByteArray &errorData)
{
    appendSignal(new ReadyReadSignal(outputData, errorData));
}

// Called from ProcessInterfaceHandler thread exclusively
void ProcessInterfaceHandler::handleDone(const ProcessResultData &data)
{
    appendSignal(new DoneSignal(data));
}

void ProcessInterfaceHandler::appendSignal(ProcessInterfaceSignal *newSignal)
{
    {
        QMutexLocker locker(&m_mutex);
        m_caller->appendSignal(newSignal);
    }
    m_waitCondition.wakeOne();
    // call in callers thread
    QMetaObject::invokeMethod(m_caller, &GeneralProcessBlockingImpl::flush);
}

GeneralProcessBlockingImpl::GeneralProcessBlockingImpl(QtcProcessPrivate *parent)
    : m_caller(parent)
    , m_processHandler(new ProcessInterfaceHandler(this, parent->m_process.get()))
{
    // In order to move the process interface into another thread together with handle
    parent->m_process.get()->setParent(m_processHandler.get());
    m_processHandler->setParent(this);
    // So the hierarchy looks like:
    // QtcProcessPrivate
    //  |
    //  +- GeneralProcessBlockingImpl
    //      |
    //      +- ProcessInterfaceHandler
    //          |
    //          +- ProcessInterface
}

bool GeneralProcessBlockingImpl::waitForSignal(ProcessSignalType newSignal, int msecs)
{
    m_processHandler->setParent(nullptr);

    QThread thread;
    thread.start();
    // Note: the thread may have started before and it's appending new signals before
    // waitForSignal() is called. However, in this case they won't be flushed since
    // the caller here is blocked, so all signals should be buffered and we are going
    // to flush them from inside waitForSignal().
    m_processHandler->moveToThread(&thread);
    const bool result = m_processHandler->waitForSignal(newSignal, msecs);
    m_processHandler->moveToCallerThread();
    m_processHandler->setParent(this);
    thread.quit();
    thread.wait();
    return result;
}

// Called from caller's thread exclusively
QList<ProcessInterfaceSignal *> GeneralProcessBlockingImpl::takeAllSignals()
{
    QMutexLocker locker(&m_mutex);
    return std::exchange(m_signals, {});
}

// Called from caller's thread exclusively
QList<ProcessInterfaceSignal *> GeneralProcessBlockingImpl::takeSignalsFor(ProcessSignalType signalType)
{
    // If we are flushing for ReadyRead or Done - flush all.
    if (signalType != ProcessSignalType::Started)
        return takeAllSignals();

    QMutexLocker locker(&m_mutex);
    const QList<ProcessSignalType> storedSignals = transform(std::as_const(m_signals),
                            [](const ProcessInterfaceSignal *aSignal) {
        return aSignal->signalType();
    });

    // If we are flushing for Started:
    // - if Started was buffered - flush Started only (even when Done was buffered)
    // - otherwise if Done signal was buffered - flush all.
    if (!storedSignals.contains(ProcessSignalType::Started)
            && storedSignals.contains(ProcessSignalType::Done)) {
        return std::exchange(m_signals, {}); // avoid takeAllSignals() because of mutex locked
    }

    QList<ProcessInterfaceSignal *> oldSignals;
    const auto matchingIndex = storedSignals.lastIndexOf(signalType);
    if (matchingIndex >= 0) {
        oldSignals = m_signals.mid(0, matchingIndex + 1);
        m_signals = m_signals.mid(matchingIndex + 1);
    }
    return oldSignals;
}

// Called from caller's thread exclusively
bool GeneralProcessBlockingImpl::flushSignals(const QList<ProcessInterfaceSignal *> &signalList,
                                     ProcessSignalType *signalType)
{
    bool signalMatched = false;
    for (const ProcessInterfaceSignal *storedSignal : std::as_const(signalList)) {
        const ProcessSignalType storedSignalType = storedSignal->signalType();
        if (signalType && storedSignalType == *signalType)
            signalMatched = true;
        switch (storedSignalType) {
        case ProcessSignalType::Started:
            handleStartedSignal(static_cast<const StartedSignal *>(storedSignal));
            break;
        case ProcessSignalType::ReadyRead:
            handleReadyReadSignal(static_cast<const ReadyReadSignal *>(storedSignal));
            break;
        case ProcessSignalType::Done:
            if (signalType)
                signalMatched = true;
            handleDoneSignal(static_cast<const DoneSignal *>(storedSignal));
            break;
        }
        delete storedSignal;
    }
    return signalMatched;
}

void GeneralProcessBlockingImpl::handleStartedSignal(const StartedSignal *aSignal)
{
    m_caller->handleStarted(aSignal->processId(), aSignal->applicationMainThreadId());
}

void GeneralProcessBlockingImpl::handleReadyReadSignal(const ReadyReadSignal *aSignal)
{
    m_caller->handleReadyRead(aSignal->stdOut(), aSignal->stdErr());
}

void GeneralProcessBlockingImpl::handleDoneSignal(const DoneSignal *aSignal)
{
    m_caller->handleDone(aSignal->resultData());
}

// Called from ProcessInterfaceHandler thread exclusively.
void GeneralProcessBlockingImpl::appendSignal(ProcessInterfaceSignal *newSignal)
{
    QMutexLocker locker(&m_mutex);
    m_signals.append(newSignal);
}

bool QtcProcessPrivate::waitForSignal(ProcessSignalType newSignal, int msecs)
{
    const QDeadlineTimer timeout(msecs);
    const QDeadlineTimer currentKillTimeout(m_killTimer.remainingTime());
    const bool needsSplit = m_killTimer.isActive() ? timeout > currentKillTimeout : false;
    const QDeadlineTimer mainTimeout = needsSplit ? currentKillTimeout : timeout;

    bool result = m_blockingInterface->waitForSignal(newSignal, mainTimeout.remainingTime());
    if (!result && needsSplit) {
        m_killTimer.stop();
        sendControlSignal(ControlSignal::Kill);
        result = m_blockingInterface->waitForSignal(newSignal, timeout.remainingTime());
    }
    return result;
}

Qt::ConnectionType QtcProcessPrivate::connectionType() const
{
    return (m_process->thread() == thread()) ? Qt::DirectConnection
                                             : Qt::BlockingQueuedConnection;
}

void QtcProcessPrivate::sendControlSignal(ControlSignal controlSignal)
{
    QTC_ASSERT(QThread::currentThread() == thread(), return);
    if (!m_process || (m_state == QProcess::NotRunning))
        return;

    if (controlSignal == ControlSignal::Terminate || controlSignal == ControlSignal::Kill)
        m_resultData.m_canceledByUser = true;

    QMetaObject::invokeMethod(m_process.get(), [this, controlSignal] {
        m_process->sendControlSignal(controlSignal);
    }, connectionType());
}

void QtcProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdOut.codec = m_codec;
    m_stdErr.clearForRun();
    m_stdErr.codec = m_codec;
    m_result = ProcessResult::StartFailed;

    m_killTimer.stop();
    m_state = QProcess::NotRunning;
    m_processId = 0;
    m_applicationMainThreadId = 0;
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

/*!
    \class Utils::QtcProcess

    \brief The QtcProcess class provides functionality for with processes.

    \sa Utils::ProcessArgs
*/

QtcProcess::QtcProcess(QObject *parent)
    : QObject(parent),
    d(new QtcProcessPrivate(this))
{
    qRegisterMetaType<ProcessResultData>("ProcessResultData");
    static int qProcessExitStatusMeta = qRegisterMetaType<QProcess::ExitStatus>();
    static int qProcessProcessErrorMeta = qRegisterMetaType<QProcess::ProcessError>();
    Q_UNUSED(qProcessExitStatusMeta)
    Q_UNUSED(qProcessProcessErrorMeta)
}

QtcProcess::~QtcProcess()
{
    QTC_ASSERT(!d->m_guard.isLocked(), qWarning("Deleting QtcProcess instance directly from "
               "one of its signal handlers will lead to crash!"));
    if (d->m_process)
        d->m_process->disconnect();
    delete d;
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
}

const Environment &QtcProcess::environment() const
{
    return d->m_setup.m_environment;
}

void QtcProcess::setControlEnvironment(const Environment &environment)
{
    d->m_setup.m_controlEnvironment = environment;
}

const Environment &QtcProcess::controlEnvironment() const
{
    return d->m_setup.m_controlEnvironment;
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
    d->m_setup.m_useCtrlCStub = enabled;
}

void QtcProcess::start()
{
    QTC_ASSERT(state() == QProcess::NotRunning, return);
    QTC_ASSERT(!(d->m_process && d->m_guard.isLocked()),
               qWarning("Restarting the QtcProcess directly from one of its signal handlers will "
                        "lead to crash! Consider calling close() prior to direct restart."));
    d->clearForRun();
    ProcessInterface *processImpl = nullptr;
    if (d->m_setup.m_commandLine.executable().needsDevice()) {
        QTC_ASSERT(s_deviceHooks.processImplHook, return);
        processImpl = s_deviceHooks.processImplHook(commandLine().executable());
    } else {
        processImpl = d->createProcessInterface();
    }
    QTC_ASSERT(processImpl, return);
    d->setProcessInterface(processImpl);
    d->m_state = QProcess::Starting;
    d->m_process->m_setup = d->m_setup;
    d->m_process->m_setup.m_commandLine = d->fullCommandLine();
    d->m_process->m_setup.m_environment = d->fullEnvironment();
    d->emitGuardedSignal(&QtcProcess::starting);
    d->m_process->start();
}

void QtcProcess::terminate()
{
    d->sendControlSignal(ControlSignal::Terminate);
}

void QtcProcess::kill()
{
    d->sendControlSignal(ControlSignal::Kill);
}

void QtcProcess::interrupt()
{
    d->sendControlSignal(ControlSignal::Interrupt);
}

void QtcProcess::kickoffProcess()
{
    d->sendControlSignal(ControlSignal::KickOff);
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
    if (d->m_setup.m_environment.isValid()) {
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

void QtcProcess::setReaperTimeout(int msecs)
{
    d->m_setup.m_reaperTimeout = msecs;
}

int QtcProcess::reaperTimeout() const
{
    return d->m_setup.m_reaperTimeout;
}

void QtcProcess::setRemoteProcessHooks(const DeviceProcessHooks &hooks)
{
    s_deviceHooks = hooks;
}

static bool askToKill(const QString &command)
{
#ifdef QT_GUI_LIB
    if (!isMainThread())
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

bool QtcProcess::readDataFromProcess(QByteArray *stdOut, QByteArray *stdErr, int timeoutS)
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
        const QByteArray newStdOut = readAllRawStandardOutput();
        if (!newStdOut.isEmpty()) {
            hasData = true;
            if (stdOut)
                stdOut->append(newStdOut);
        }
        // Check 'stderr' separately. This is a special handling
        // for 'git pull' and the like which prints its progress on stderr.
        const QByteArray newStdErr = readAllRawStandardError();
        if (!newStdErr.isEmpty()) {
            hasData = true;
            if (stdErr)
                stdErr->append(newStdErr);
        }
        // Prompt user, pretend we have data if says 'No'.
        const bool hang = !hasData && !finished;
        hasData = hang && !askToKill(d->m_setup.m_commandLine.executable().path());
    } while (hasData && !finished);
    if (syncDebug)
        qDebug() << "<readDataFromProcess" << finished;
    return finished;
}

QString QtcProcess::normalizeNewlines(const QString &text)
{
    QString res = text;
    const auto newEnd = std::unique(res.begin(), res.end(), [](const QChar c1, const QChar c2) {
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
        return {};
    const int dotIndex = binary.lastIndexOf(QLatin1Char('.'));
    if (dotIndex != -1 && dotIndex == binary.size() - 4)
        return {};

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
    return {};
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
        return {};
    const QStringList::const_iterator cend = paths.constEnd();
    for (QStringList::const_iterator it = paths.constBegin(); it != cend; ++it) {
        const QDir dir(*it);
        const QString rc = checkBinary(dir, binary);
        if (!rc.isEmpty())
            return rc;
    }
    return {};
}

Environment QtcProcess::systemEnvironmentForBinary(const FilePath &filePath)
{
    if (filePath.needsDevice()) {
        QTC_ASSERT(s_deviceHooks.systemEnvironmentForBinary, return {});
        return s_deviceHooks.systemEnvironmentForBinary(filePath);
    }

    return Environment::systemEnvironment();
}

qint64 QtcProcess::applicationMainThreadId() const
{
    return d->m_applicationMainThreadId;
}

QProcess::ProcessChannelMode QtcProcess::processChannelMode() const
{
    return d->m_setup.m_processChannelMode;
}

void QtcProcess::setProcessChannelMode(QProcess::ProcessChannelMode mode)
{
    d->m_setup.m_processChannelMode = mode;
}

QProcess::ProcessState QtcProcess::state() const
{
    return d->m_state;
}

bool QtcProcess::isRunning() const
{
    return state() == QProcess::Running;
}

qint64 QtcProcess::processId() const
{
    return d->m_processId;
}

bool QtcProcess::waitForStarted(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    if (d->m_state == QProcess::Running)
        return true;
    if (d->m_state == QProcess::NotRunning)
        return false;
    return s_waitForStarted.measureAndRun(&QtcProcessPrivate::waitForSignal, d,
                                          ProcessSignalType::Started, msecs);
}

bool QtcProcess::waitForReadyRead(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    if (d->m_state == QProcess::NotRunning)
        return false;
    return d->waitForSignal(ProcessSignalType::ReadyRead, msecs);
}

bool QtcProcess::waitForFinished(int msecs)
{
    QTC_ASSERT(d->m_process, return false);
    if (d->m_state == QProcess::NotRunning)
        return false;
    return d->waitForSignal(ProcessSignalType::Done, msecs);
}

QByteArray QtcProcess::readAllRawStandardOutput()
{
    return d->m_stdOut.readAllData();
}

QByteArray QtcProcess::readAllRawStandardError()
{
    return d->m_stdErr.readAllData();
}

qint64 QtcProcess::write(const QString &input)
{
    // Non-windows is assumed to be UTF-8
    if (commandLine().executable().osType() != OsTypeWindows)
        return writeRaw(input.toUtf8());

    if (HostOsInfo::hostOs() == OsTypeWindows)
        return writeRaw(input.toLocal8Bit());

    // "remote" Windows target on non-Windows host is unlikely,
    // but the true encoding is not accessible. Use UTF8 as best guess.
    QTC_CHECK(false);
    return writeRaw(input.toUtf8());
}

qint64 QtcProcess::writeRaw(const QByteArray &input)
{
    QTC_ASSERT(processMode() == ProcessMode::Writer, return -1);
    QTC_ASSERT(d->m_process, return -1);
    QTC_ASSERT(state() == QProcess::Running, return -1);
    QTC_ASSERT(QThread::currentThread() == thread(), return -1);
    qint64 result = -1;
    QMetaObject::invokeMethod(d->m_process.get(), [this, input] {
        d->m_process->write(input);
    }, d->connectionType(), &result);
    return result;
}

void QtcProcess::close()
{
    QTC_ASSERT(QThread::currentThread() == thread(), return);
    if (d->m_process) {
        // Note: the m_process may be inside ProcessInterfaceHandler's thread.
        QTC_ASSERT(d->m_process->thread() == thread(), return);
        d->m_process->disconnect();
        d->m_process.release()->deleteLater();
    }
    if (d->m_blockingInterface) {
        d->m_blockingInterface->disconnect();
        d->m_blockingInterface.release()->deleteLater();
    }
    d->clearForRun();
}

/*
   Calls terminate() directly and after a delay of reaperTimeout() it calls kill()
   if the process is still running.
*/
void QtcProcess::stop()
{
    if (state() == QProcess::NotRunning)
        return;

    d->sendControlSignal(ControlSignal::Terminate);
    d->m_killTimer.start(d->m_process->m_setup.m_reaperTimeout);
}

QString QtcProcess::readAllStandardOutput()
{
    return QString::fromUtf8(readAllRawStandardOutput());
}

QString QtcProcess::readAllStandardError()
{
    return QString::fromUtf8(readAllRawStandardError());
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
    return {};
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
    const QString out = cleanedStdOut();
    const QString err = cleanedStdErr();

    if (!out.isEmpty() && !err.isEmpty()) {
        QString result = out;
        if (!result.endsWith('\n'))
            result += '\n';
        result += err;
        return result;
    }
    return !out.isEmpty() ? out : err;
}

QByteArray QtcProcess::rawStdOut() const
{
    QTC_CHECK(d->m_stdOut.keepRawData);
    return d->m_stdOut.rawData;
}

QString QtcProcess::stdOut() const
{
    QTC_CHECK(d->m_stdOut.keepRawData);
    return d->m_codec->toUnicode(d->m_stdOut.rawData);
}

QString QtcProcess::stdErr() const
{
    QTC_CHECK(d->m_stdErr.keepRawData);
    return d->m_codec->toUnicode(d->m_stdErr.rawData);
}

QString QtcProcess::cleanedStdOut() const
{
    return normalizeNewlines(stdOut());
}

QString QtcProcess::cleanedStdErr() const
{
    return normalizeNewlines(stdErr());
}

static QStringList splitLines(const QString &text)
{
    QStringList result = text.split('\n');
    for (QString &line : result) {
        if (line.endsWith('\r'))
            line.chop(1);
    }
    return result;
}

const QStringList QtcProcess::stdOutLines() const
{
    return splitLines(cleanedStdOut());
}

const QStringList QtcProcess::stdErrLines() const
{
    return splitLines(cleanedStdErr());
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

int QtcProcess::timeoutS() const
{
    return d->m_maxHangTimerCount;
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

void QtcProcess::runBlocking(EventLoopMode eventLoopMode)
{
    // Attach a dynamic property with info about blocking type
    d->storeEventLoopDebugInfo(int(eventLoopMode));

    QtcProcess::start();

    // Remove the dynamic property so that it's not reused in subseqent start()
    d->storeEventLoopDebugInfo({});

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
            if (isMainThread())
                QApplication::setOverrideCursor(Qt::WaitCursor);
#endif
            QEventLoop eventLoop(this);
            QTC_ASSERT(!d->m_eventLoop, return);
            d->m_eventLoop = &eventLoop;
            eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
            d->m_eventLoop = nullptr;
            timer.stop();
#ifdef QT_GUI_LIB
            if (isMainThread())
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

void QtcProcess::setStdOutCallback(const TextChannelCallback &callback)
{
    d->m_stdOut.outputCallback = callback;
    d->m_stdOut.emitSingleLines = false;
}

void QtcProcess::setStdOutLineCallback(const TextChannelCallback &callback)
{
    d->m_stdOut.outputCallback = callback;
    d->m_stdOut.emitSingleLines = true;
    d->m_stdOut.keepRawData = false;
}

void QtcProcess::setStdErrCallback(const TextChannelCallback &callback)
{
    d->m_stdErr.outputCallback = callback;
    d->m_stdErr.emitSingleLines = false;
}

void QtcProcess::setStdErrLineCallback(const TextChannelCallback &callback)
{
    d->m_stdErr.outputCallback = callback;
    d->m_stdErr.emitSingleLines = true;
    d->m_stdErr.keepRawData = false;
}

void QtcProcess::setTextChannelMode(Channel channel, TextChannelMode mode)
{
    const TextChannelCallback outputCb = [this](const QString &text) {
        GuardLocker locker(d->m_guard);
        emit textOnStandardOutput(text);
    };
    const TextChannelCallback errorCb = [this](const QString &text) {
        GuardLocker locker(d->m_guard);
        emit textOnStandardError(text);
    };
    const TextChannelCallback callback = (channel == Channel::Output) ? outputCb : errorCb;
    ChannelBuffer *buffer = channel == Channel::Output ? &d->m_stdOut : &d->m_stdErr;
    QTC_ASSERT(buffer->m_textChannelMode == TextChannelMode::Off, qWarning()
               << "QtcProcess::setTextChannelMode(): Changing text channel mode for"
               << (channel == Channel::Output ? "Output": "Error")
               << "channel while it was previously set for this channel.");
    buffer->m_textChannelMode = mode;
    switch (mode) {
    case TextChannelMode::Off:
        buffer->outputCallback = {};
        buffer->emitSingleLines = true;
        buffer->keepRawData = true;
        break;
    case TextChannelMode::SingleLine:
        buffer->outputCallback = callback;
        buffer->emitSingleLines = true;
        buffer->keepRawData = false;
        break;
    case TextChannelMode::MultiLine:
        buffer->outputCallback = callback;
        buffer->emitSingleLines = false;
        buffer->keepRawData = true;
        break;
    }
}

TextChannelMode QtcProcess::textChannelMode(Channel channel) const
{
    ChannelBuffer *buffer = channel == Channel::Output ? &d->m_stdOut : &d->m_stdErr;
    return buffer->m_textChannelMode;
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
            q->stop();
            q->waitForFinished();
            m_result = ProcessResult::Hang;
        } else {
            m_hangTimerCount = 0;
        }
    } else {
        if (debug)
            qDebug() << Q_FUNC_INFO << m_hangTimerCount;
    }
}

void QtcProcessPrivate::handleStarted(qint64 processId, qint64 applicationMainThreadId)
{
    QTC_CHECK(m_state == QProcess::Starting);
    m_state = QProcess::Running;

    m_processId = processId;
    m_applicationMainThreadId = applicationMainThreadId;
    emitGuardedSignal(&QtcProcess::started);
}

void QtcProcessPrivate::handleReadyRead(const QByteArray &outputData, const QByteArray &errorData)
{
    QTC_CHECK(m_state == QProcess::Running);

    // TODO: check why we need this timer?
    m_hangTimerCount = 0;
    // TODO: store a copy of m_processChannelMode on start()? Currently we assert that state
    // is NotRunning when setting the process channel mode.

    if (!outputData.isEmpty()) {
        if (m_process->m_setup.m_processChannelMode == QProcess::ForwardedOutputChannel
                || m_process->m_setup.m_processChannelMode == QProcess::ForwardedChannels) {
            std::cout << outputData.constData() << std::flush;
        } else {
            m_stdOut.append(outputData);
            emitGuardedSignal(&QtcProcess::readyReadStandardOutput);
        }
    }
    if (!errorData.isEmpty()) {
        if (m_process->m_setup.m_processChannelMode == QProcess::ForwardedErrorChannel
                || m_process->m_setup.m_processChannelMode == QProcess::ForwardedChannels) {
            std::cerr << errorData.constData() << std::flush;
        } else {
            m_stdErr.append(errorData);
            emitGuardedSignal(&QtcProcess::readyReadStandardError);
        }
    }
}

void QtcProcessPrivate::handleDone(const ProcessResultData &data)
{
    m_killTimer.stop();
    const bool wasCanceled = m_resultData.m_canceledByUser;
    m_resultData = data;
    m_resultData.m_canceledByUser = wasCanceled;

    switch (m_state) {
    case QProcess::NotRunning:
        QTC_CHECK(false); // Can't happen
        break;
    case QProcess::Starting:
        QTC_CHECK(m_resultData.m_error == QProcess::FailedToStart);
        break;
    case QProcess::Running:
        QTC_CHECK(m_resultData.m_error != QProcess::FailedToStart);
        break;
    }
    m_state = QProcess::NotRunning;

    // This code (255) is being returned by QProcess when FailedToStart error occurred
    if (m_resultData.m_error == QProcess::FailedToStart)
        m_resultData.m_exitCode = 0xFF;

    // HACK: See QIODevice::errorString() implementation.
    if (m_resultData.m_error == QProcess::UnknownError)
        m_resultData.m_errorString.clear();
    else if (m_result != ProcessResult::Hang)
        m_result = ProcessResult::StartFailed;

    if (debug)
        qDebug() << Q_FUNC_INFO << m_resultData.m_exitCode << m_resultData.m_exitStatus;
    m_hangTimerCount = 0;

    if (m_resultData.m_error != QProcess::FailedToStart) {
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
    }
    if (m_eventLoop)
        m_eventLoop->quit();

    m_stdOut.handleRest();
    m_stdErr.handleRest();

    emitGuardedSignal(&QtcProcess::done);
    m_processId = 0;
    m_applicationMainThreadId = 0;
}

static QString blockingMessage(const QVariant &variant)
{
    if (!variant.isValid())
        return "non blocking";
    if (variant.toInt() == int(EventLoopMode::On))
        return "blocking with event loop";
    return "blocking without event loop";
}

void QtcProcessPrivate::setupDebugLog()
{
    if (!processLog().isDebugEnabled())
        return;

    auto now = [] {
        using namespace std::chrono;
        return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    };

    connect(q, &QtcProcess::starting, this, [=] {
        const quint64 msNow = now();
        setProperty(QTC_PROCESS_STARTTIME, msNow);

        static std::atomic_int startCounter = 0;
        const int currentNumber = startCounter.fetch_add(1);
        qCDebug(processLog).nospace().noquote()
                << "Process " << currentNumber << " starting ("
                << qPrintable(blockingMessage(property(QTC_PROCESS_BLOCKING_TYPE)))
                << "): " << m_setup.m_commandLine.toUserOutput();
        setProperty(QTC_PROCESS_NUMBER, currentNumber);
    });

    connect(q, &QtcProcess::done, this, [=] {
        if (!m_process.get())
            return;
        const QVariant n = property(QTC_PROCESS_NUMBER);
        if (!n.isValid())
            return;
        const quint64 msNow = now();
        const quint64 msStarted = property(QTC_PROCESS_STARTTIME).toULongLong();
        const quint64 msElapsed = msNow - msStarted;

        const int number = n.toInt();
        const QString stdOut = q->cleanedStdOut();
        const QString stdErr = q->cleanedStdErr();
        qCDebug(processLog).nospace()
                << "Process " << number << " finished: result=" << int(m_result)
                << ", ex=" << m_resultData.m_exitCode
                << ", " << stdOut.size() << " bytes stdout: " << stdOut.left(20)
                << ", " << stdErr.size() << " bytes stderr: " << stdErr.left(1000)
                << ", " << msElapsed << " ms elapsed";
        if (processStdoutLog().isDebugEnabled() && !stdOut.isEmpty())
            qCDebug(processStdoutLog).nospace() << "Process " << number << " sdout: " << stdOut;
        if (processStderrLog().isDebugEnabled() && !stdErr.isEmpty())
            qCDebug(processStderrLog).nospace() << "Process " << number << " stderr: " << stdErr;
    });
}

void QtcProcessPrivate::storeEventLoopDebugInfo(const QVariant &value)
{
    if (processLog().isDebugEnabled())
        setProperty(QTC_PROCESS_BLOCKING_TYPE, value);
}

QtcProcessAdapter::QtcProcessAdapter()
{
    connect(task(), &QtcProcess::done, this, [this] {
        emit done(task()->result() == ProcessResult::FinishedWithSuccess);
    });
}

void QtcProcessAdapter::start()
{
    task()->start();
}

} // namespace Utils

#include "qtcprocess.moc"
