// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QMutex>
#include <QProcess>
#include <QSocketNotifier>
#include <QThread>
#include <QTimer>
#include <QWinEventNotifier>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <optional>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#endif

#ifdef Q_OS_LINUX
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#endif

#include <iostream>

Q_LOGGING_CATEGORY(log, "qtc.process_stub", QtWarningMsg);

// Global variables

QCommandLineParser commandLineParser;

// The inferior command and arguments
QStringList inferiorCmdAndArguments;
// Whether to Suspend the inferior process on startup (to allow a debugger to attach)
bool debugMode = false;
// Whether to run in test mode (i.e. to start manually from the command line)
bool testMode = false;
// The control socket used to communicate with Qt Creator
QLocalSocket controlSocket;
// Environment variables to set for the inferior process
std::optional<QStringList> environmentVariables;

QProcess inferiorProcess;
int inferiorId{0};

#ifdef Q_OS_DARWIN
// A memory mapped helper to retrieve the pid of the inferior process in debugMode
static int *shared_child_pid = nullptr;
#endif

#ifdef Q_OS_WIN
Q_PROCESS_INFORMATION *win_process_information = nullptr;
#endif

bool waitingForExitKeyPress = false;

QThread processThread;

// Helper to create the shared memory mapped segment
void setupSharedPid();
// Parses the command line, returns a status code in case of error
std::optional<int> tryParseCommandLine(QCoreApplication &app);
// Sets the working directory, returns a status code in case of error
std::optional<int> trySetWorkingDir();
// Reads the environment variables from the env file, returns a status code in case of error
std::optional<int> readEnvFile();

void setupControlSocket();
void setupSignalHandlers();
void startProcess(const QString &program, const QStringList &arguments, const QString &workingDir);
void onKeyPress(std::function<void()> callback);
void sendSelfPid();
void killInferior();
void resumeInferior();

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    setupSharedPid();

    auto error = tryParseCommandLine(a);
    if (error)
        return error.value();

    qCInfo(log) << "Debug helper started: ";
    qCInfo(log) << "Socket:" << commandLineParser.value("socket");
    qCInfo(log) << "Inferior:" << inferiorCmdAndArguments.join(QChar::Space);
    qCInfo(log) << "Working Directory" << commandLineParser.value("workingDir");
    qCInfo(log) << "Env file:" << commandLineParser.value("envfile");
    qCInfo(log) << "Mode:"
                << QLatin1String(testMode ? "test | " : "")
                       + QLatin1String(debugMode ? "debug" : "run");

    error = trySetWorkingDir();
    if (error)
        return error.value();

    error = readEnvFile();
    if (error)
        return error.value();

    if (testMode) {
        sendSelfPid();
        setupSignalHandlers();

        startProcess(inferiorCmdAndArguments[0],
                     inferiorCmdAndArguments.mid(1),
                     commandLineParser.value("workingDir"));

        if (debugMode) {
            qDebug() << "Press 'c' to continue or 'k' to kill, followed by 'enter'";

            onKeyPress([] {
                char ch;
                std::cin >> ch;
                if (ch == 'k')
                    killInferior();
                else
                    resumeInferior();
            });
        }

        return a.exec();
    }

    setupControlSocket();

    return a.exec();
}

void sendMsg(const QByteArray &msg)
{
    if (controlSocket.state() == QLocalSocket::ConnectedState) {
        controlSocket.write(msg);
    } else {
        qDebug() << "MSG:" << msg;
    }
}

void sendPid(int inferiorPid)
{
    sendMsg(QString("pid %1\n").arg(inferiorPid).toUtf8());
}

void sendThreadId(int inferiorThreadPid)
{
    sendMsg(QString("thread %1\n").arg(inferiorThreadPid).toUtf8());
}

void sendSelfPid()
{
    sendMsg(QString("spid %1\n").arg(QCoreApplication::applicationPid()).toUtf8());
}

void sendExit(int exitCode)
{
    sendMsg(QString("exit %1\n").arg(exitCode).toUtf8());
}

void sendCrash(int exitCode)
{
    sendMsg(QString("crash %1\n").arg(exitCode).toUtf8());
}

void sendErrChDir()
{
    sendMsg(QString("err:chdir %1\n").arg(errno).toUtf8());
}

void doExit(int exitCode)
{
    if (waitingForExitKeyPress)
        exit(exitCode);

    if (controlSocket.state() == QLocalSocket::ConnectedState && controlSocket.bytesToWrite())
        controlSocket.waitForBytesWritten(1000);

    if (!commandLineParser.value("wait").isEmpty()) {
        std::cout << commandLineParser.value("wait").toStdString() << std::endl;

        waitingForExitKeyPress = true;
        onKeyPress([] { doExit(0); });
    } else {
        exit(exitCode);
    }
}

void onInferiorFinished(int exitCode, QProcess::ExitStatus status)
{
    qCInfo(log) << "Inferior finished";

    if (status == QProcess::CrashExit) {
        sendCrash(exitCode);
        doExit(exitCode);
    } else {
        sendExit(exitCode);
        doExit(exitCode);
    }
}

void onInferiorErrorOccurered(QProcess::ProcessError error)
{
    qCWarning(log) << "Inferior error: " << error << inferiorProcess.errorString();
}

#ifdef Q_OS_LINUX
QString statusToString(int status)
{
    if (WIFEXITED(status))
        return QString("exit, status=%1").arg(WEXITSTATUS(status));
    else if (WIFSIGNALED(status))
        return QString("Killed by: %1").arg(WTERMSIG(status));
    else if (WIFSTOPPED(status)) {
        return QString("Stopped by: %1").arg(WSTOPSIG(status));
    } else if (WIFCONTINUED(status))
        return QString("Continued");

    return QString("Unknown status");
}

bool waitFor(int signalToWaitFor)
{
    int status = 0;

    waitpid(inferiorId, &status, WUNTRACED);

    if (!WIFSTOPPED(status) || WSTOPSIG(status) != signalToWaitFor) {
        qCCritical(log) << "Unexpected status during startup:" << statusToString(status)
                        << ", aborting";
        sendCrash(0xFF);
        return false;
    }

    return true;
}
#endif

void onInferiorStarted()
{
    inferiorId = inferiorProcess.processId();
    qCInfo(log) << "Inferior started ( pid:" << inferiorId << ")";
#ifdef Q_OS_WIN
    sendThreadId(win_process_information->dwThreadId);
    sendPid(inferiorId);
#elif defined(Q_OS_DARWIN)
    // In debug mode we use the poll timer to send the pid.
    if (!debugMode)
        sendPid(inferiorId);
#else

    if (debugMode) {
        qCInfo(log) << "Waiting for SIGTRAP from inferiors execve ...";
        if (!waitFor(SIGTRAP))
            return;

        qCInfo(log) << "Detaching ...";
        ptrace(PTRACE_DETACH, inferiorId, 0, SIGSTOP);

        // Wait until the process actually finished detaching
        if (!waitFor(SIGSTOP))
            return;
    }

    qCInfo(log) << "Sending pid:" << inferiorId;
    sendPid(inferiorId);
#endif
}

void setupUnixInferior()
{
#ifndef Q_OS_WIN
    if (debugMode) {
        qCInfo(log) << "Debug mode enabled";
#ifdef Q_OS_DARWIN
        // We are using raise(SIGSTOP) to stop the child process, macOS does not support ptrace(...)
        inferiorProcess.setChildProcessModifier([] {
            // Let the parent know our pid ...
            *shared_child_pid = getpid();
            // Suspend ourselves ...
            raise(SIGSTOP);
        });
#else
        // PTRACE_TRACEME will stop execution of the child process as soon as execve is called.
        inferiorProcess.setChildProcessModifier([] {
            ptrace(PTRACE_TRACEME, 0, 0, 0);
            // Disable attachment restrictions so we are not bound by yama/ptrace_scope mode 1
            prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY);
        });
#endif
    }
#endif
}

void setupWindowsInferior()
{
#ifdef Q_OS_WIN
    inferiorProcess.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments *args) {
        if (debugMode)
            args->flags |= CREATE_SUSPENDED;
        win_process_information = args->processInformation;
    });
#endif
}

void setupPidPollTimer()
{
#ifdef Q_OS_DARWIN
    if (!debugMode)
        return;

    static QTimer pollPidTimer;

    pollPidTimer.setInterval(1);
    pollPidTimer.setSingleShot(false);
    QObject::connect(&pollPidTimer, &QTimer::timeout, &pollPidTimer, [&] {
        if (*shared_child_pid) {
            qCInfo(log) << "Received pid during polling:" << *shared_child_pid;
            inferiorId = *shared_child_pid;
            sendPid(inferiorId);
            pollPidTimer.stop();
            munmap(shared_child_pid, sizeof(int));
        } else {
            qCDebug(log) << "Waiting for inferior to start...";
        }
    });
    pollPidTimer.start();
#endif
}

enum class Out { StdOut, StdErr };

void writeToOut(const QByteArray &data, Out out)
{
#ifdef Q_OS_WIN
    static const HANDLE outHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    static const HANDLE errHandle = GetStdHandle(STD_ERROR_HANDLE);
    WriteFile(out == Out::StdOut ? outHandle : errHandle,
              data.constData(),
              data.size(),
              nullptr,
              nullptr);
#else
    auto fp = out == Out::StdOut ? stdout : stderr;
    ::fwrite(data.constData(), 1, data.size(), fp);
    ::fflush(fp);
#endif
}

void startProcess(const QString &executable, const QStringList &arguments, const QString &workingDir)
{
    setupPidPollTimer();

    qCInfo(log) << "Starting Inferior";

    QObject::connect(&inferiorProcess,
                     &QProcess::finished,
                     QCoreApplication::instance(),
                     &onInferiorFinished);
    QObject::connect(&inferiorProcess,
                     &QProcess::errorOccurred,
                     QCoreApplication::instance(),
                     &onInferiorErrorOccurered);
    QObject::connect(&inferiorProcess,
                     &QProcess::started,
                     QCoreApplication::instance(),
                     &onInferiorStarted);

    inferiorProcess.setProcessChannelMode(QProcess::ForwardedChannels);

    if (!(testMode && debugMode))
        inferiorProcess.setInputChannelMode(QProcess::ForwardedInputChannel);
    inferiorProcess.setWorkingDirectory(workingDir);
    inferiorProcess.setProgram(executable);
    inferiorProcess.setArguments(arguments);

    if (environmentVariables)
        inferiorProcess.setEnvironment(*environmentVariables);

    setupWindowsInferior();
    setupUnixInferior();

    inferiorProcess.start();
}

std::optional<int> readEnvFile()
{
    if (!commandLineParser.isSet("envfile"))
        return std::nullopt;

    const QString path = commandLineParser.value("envfile");
    qCInfo(log) << "Reading env file: " << path << "...";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(log) << "Failed to open env file: " << path;
        return 1;
    }

    environmentVariables = QStringList{};

    while (!file.atEnd()) {
        QByteArray data = file.readAll();
        if (!data.isEmpty()) {
            for (const auto &line : data.split('\0')) {
                if (!line.isEmpty())
                    *environmentVariables << QString::fromUtf8(line);
            }
        }
    }

    qCDebug(log) << "Env: ";
    for (const auto &env : *environmentVariables)
        qCDebug(log) << env;

    return std::nullopt;
}

#ifndef Q_OS_WIN
void forwardSignal(int signum)
{
    qCDebug(log) << "SIGTERM received, terminating inferior...";
    kill(inferiorId, signum);
}
#else
static BOOL WINAPI ctrlHandler(DWORD dwCtrlType)
{
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        qCDebug(log) << "Terminate inferior...";
        inferiorProcess.terminate();
        return TRUE;
    }
    return FALSE;
}
#endif

void setupSignalHandlers()
{
#ifdef Q_OS_WIN
    SetConsoleCtrlHandler(ctrlHandler, TRUE);
#else
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    act.sa_handler = SIG_IGN;
    if (sigaction(SIGTTOU, &act, NULL)) {
        qCWarning(log) << "sigaction SIGTTOU: " << strerror(errno);
        doExit(3);
    }

    act.sa_handler = forwardSignal;
    if (sigaction(SIGTERM, &act, NULL)) {
        qCWarning(log) << "sigaction SIGTERM: " << strerror(errno);
        doExit(3);
    }

    if (sigaction(SIGINT, &act, NULL)) {
        qCWarning(log) << "sigaction SIGINT: " << strerror(errno);
        doExit(3);
    }

    qCDebug(log) << "Signals set up";
#endif
}

std::optional<int> tryParseCommandLine(QCoreApplication &app)
{
    commandLineParser.setApplicationDescription("Debug helper for QtCreator");
    commandLineParser.addHelpOption();
    commandLineParser.addOption(QCommandLineOption({"d", "debug"}, "Start inferior in debug mode"));
    commandLineParser.addOption(QCommandLineOption({"t", "test"}, "Don't start the control socket"));
    commandLineParser.addOption(
        QCommandLineOption({"s", "socket"}, "Path to the unix socket", "socket"));
    commandLineParser.addOption(
        QCommandLineOption({"w", "workingDir"}, "Working directory for inferior", "workingDir"));
    commandLineParser.addOption(QCommandLineOption({"v", "verbose"}, "Print debug messages"));
    commandLineParser.addOption(QCommandLineOption({"e", "envfile"}, "Path to env file", "envfile"));
    commandLineParser.addOption(
        QCommandLineOption("wait",
                           "Message to display to the user while waiting for key press",
                           "waitmessage",
                           "Press enter to continue ..."));

    commandLineParser.process(app);

    inferiorCmdAndArguments = commandLineParser.positionalArguments();
    debugMode = commandLineParser.isSet("debug");
    testMode = commandLineParser.isSet("test");

    if (!(commandLineParser.isSet("socket") || testMode) || inferiorCmdAndArguments.isEmpty()) {
        commandLineParser.showHelp(1);
        return 1;
    }

    if (commandLineParser.isSet("verbose"))
        QLoggingCategory::setFilterRules("qtc.process_stub=true");

    return std::nullopt;
}

std::optional<int> trySetWorkingDir()
{
    if (commandLineParser.isSet("workingDir")) {
        if (!QDir::setCurrent(commandLineParser.value("workingDir"))) {
            qCWarning(log) << "Failed to change working directory to: "
                           << commandLineParser.value("workingDir");
            sendErrChDir();
            return 1;
        }
    }

    return std::nullopt;
}

void setupSharedPid()
{
#ifdef Q_OS_DARWIN
    shared_child_pid = (int *) mmap(NULL,
                                    sizeof *shared_child_pid,
                                    PROT_READ | PROT_WRITE,
                                    MAP_SHARED | MAP_ANONYMOUS,
                                    -1,
                                    0);
    *shared_child_pid = 0;
#endif
}

void onControlSocketConnected()
{
    qCInfo(log) << "Connected to control socket";

    sendSelfPid();
    setupSignalHandlers();

    startProcess(inferiorCmdAndArguments[0],
                 inferiorCmdAndArguments.mid(1),
                 commandLineParser.value("workingDir"));
}

void resumeInferior()
{
    qCDebug(log) << "Continuing inferior... (" << inferiorId << ")";
#ifdef Q_OS_WIN
    ResumeThread(win_process_information->hThread);
#else
    kill(inferiorId, SIGCONT);
#endif
}

void killInferior()
{
#ifdef Q_OS_WIN
    inferiorProcess.kill();
#else
    kill(inferiorId, SIGKILL);
#endif
}

void onControlSocketReadyRead()
{
    //k = kill, i = interrupt, c = continue, s = shutdown
    QByteArray data = controlSocket.readAll();
    for (auto ch : data) {
        qCDebug(log) << "Received:" << ch;

        switch (ch) {
        case 'k': {
            qCDebug(log) << "Killing inferior...";
            killInferior();
            break;
        }
#ifndef Q_OS_WIN
        case 'i': {
            qCDebug(log) << "Interrupting inferior...";
            kill(inferiorId, SIGINT);
            break;
        }
#endif
        case 'c': {
            resumeInferior();
            break;
        }
        case 's': {
            qCDebug(log) << "Shutting down...";
            doExit(0);
            break;
        }
        }

        sendMsg(QString("ack %1\n").arg(ch).toUtf8());
    }
}

void onControlSocketErrorOccurred(QLocalSocket::LocalSocketError socketError)
{
    qCWarning(log) << "Control socket error:" << socketError;
    doExit(1);
}

void setupControlSocket()
{
    QObject::connect(&controlSocket, &QLocalSocket::connected, &onControlSocketConnected);
    QObject::connect(&controlSocket, &QLocalSocket::readyRead, &onControlSocketReadyRead);
    QObject::connect(&controlSocket, &QLocalSocket::errorOccurred, &onControlSocketErrorOccurred);

    qCInfo(log) << "Waiting for connection...";
    controlSocket.connectToServer(commandLineParser.value("socket"));
}

void onKeyPress(std::function<void()> callback)
{
#ifdef Q_OS_WIN
    // On windows, QWinEventNotifier() doesn't work for stdin, so we have to use a thread instead.
    QThread *thread = QThread::create([] { std::cin.ignore(); });
    thread->start();
    QObject::connect(thread, &QThread::finished, &controlSocket, callback);
#else
    static auto stdInNotifier = new QSocketNotifier(fileno(stdin), QSocketNotifier::Read);
    QObject::connect(stdInNotifier, &QSocketNotifier::activated, callback);
#endif
}
