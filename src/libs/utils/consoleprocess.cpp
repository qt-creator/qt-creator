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

#include "consoleprocess.h"

#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/winutils.h>

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QRegularExpression>
#include <QSettings>
#include <QTemporaryFile>
#include <QTimer>
#include <QWinEventNotifier>

#ifdef Q_OS_WIN

#  include <windows.h>
#  include <stdlib.h>
#  include <cstring>

#else

#  include <sys/stat.h>
#  include <sys/types.h>
#  include <errno.h>
#  include <string.h>
#  include <unistd.h>

#endif

namespace Utils {

// TerminalCommand

TerminalCommand::TerminalCommand(const QString &command, const QString &openArgs, const QString &executeArgs, bool needsQuotes)
    : command(command)
    , openArgs(openArgs)
    , executeArgs(executeArgs)
    , needsQuotes(needsQuotes)
{
}

// ConsoleProcessPrivate

class ConsoleProcessPrivate
{
public:
    ConsoleProcessPrivate() = default;

    static QString m_defaultConsoleProcess;
    ConsoleProcess::Mode m_mode = ConsoleProcess::Run;
    QString m_workingDir;
    Environment m_environment;
    qint64 m_appPid = 0;
    int m_appCode;
    CommandLine m_commandLine;
    QProcess::ExitStatus m_appStatus;
    QLocalServer m_stubServer;
    QLocalSocket *m_stubSocket = nullptr;
    QTemporaryFile *m_tempFile = nullptr;
    QProcess::ProcessError m_error = QProcess::UnknownError;
    QString m_errorString;
    bool m_abortOnMetaChars = true;
    QSettings *m_settings = nullptr;

    // Used on Unix only
    QProcess m_process;
    bool m_stubConnected = false;
    QTimer *m_stubConnectTimer = nullptr;
    QByteArray m_stubServerDir;
    qint64 m_stubPid = 0;

    // Used on Windows only
    qint64 m_appMainThreadId = 0;

#ifdef Q_OS_WIN
    PROCESS_INFORMATION *m_pid = nullptr;
    HANDLE m_hInferior = NULL;
    QWinEventNotifier *inferiorFinishedNotifier = nullptr;
    QWinEventNotifier *processFinishedNotifier = nullptr;
#endif
};


// ConsoleProcess

ConsoleProcess::ConsoleProcess(QObject *parent) :
    QObject(parent), d(new ConsoleProcessPrivate)
{
    connect(&d->m_stubServer, &QLocalServer::newConnection,
            this, &ConsoleProcess::stubConnectionAvailable);

    d->m_process.setProcessChannelMode(QProcess::ForwardedChannels);
}

ConsoleProcess::~ConsoleProcess()
{
    stop();
    delete d;
}

void ConsoleProcess::setCommand(const CommandLine &command)
{
    d->m_commandLine = command;
}

CommandLine ConsoleProcess::command() const
{
    return d->m_commandLine;
}

void ConsoleProcess::setSettings(QSettings *settings)
{
    d->m_settings = settings;
}

Q_GLOBAL_STATIC_WITH_ARGS(const QVector<TerminalCommand>, knownTerminals, (
{
    {"x-terminal-emulator", "", "-e"},
    {"xdg-terminal", "", "", true},
    {"xterm", "", "-e"},
    {"aterm", "", "-e"},
    {"Eterm", "", "-e"},
    {"rxvt", "", "-e"},
    {"urxvt", "", "-e"},
    {"xfce4-terminal", "", "-x"},
    {"konsole", "--separate", "-e"},
    {"gnome-terminal", "", "--"}
}));

TerminalCommand ConsoleProcess::defaultTerminalEmulator()
{
    static TerminalCommand defaultTerm;

    if (defaultTerm.command.isEmpty()) {

        if (HostOsInfo::isMacHost()) {
            const QString termCmd = QCoreApplication::applicationDirPath()
                            + "/../Resources/scripts/openTerminal.py";
            if (QFileInfo::exists(termCmd))
                defaultTerm = {termCmd, "", ""};
            else
                defaultTerm = {"/usr/X11/bin/xterm", "", "-e"};

        } else if (HostOsInfo::isAnyUnixHost()) {
            defaultTerm = {"xterm", "", "-e"};
            const Environment env = Environment::systemEnvironment();
            for (const TerminalCommand &term : *knownTerminals) {
                const QString result = env.searchInPath(term.command).toString();
                if (!result.isEmpty()) {
                    defaultTerm = {result, term.openArgs, term.executeArgs, term.needsQuotes};
                    break;
                }
            }
        }
    }

    return defaultTerm;
}

QVector<TerminalCommand> ConsoleProcess::availableTerminalEmulators()
{
    QVector<TerminalCommand> result;

    if (HostOsInfo::isAnyUnixHost()) {
        const Environment env = Environment::systemEnvironment();
        for (const TerminalCommand &term : *knownTerminals) {
            const QString command = env.searchInPath(term.command).toString();
            if (!command.isEmpty())
                result.push_back({command, term.openArgs, term.executeArgs});
        }
        // sort and put default terminal on top
        const TerminalCommand defaultTerm = defaultTerminalEmulator();
        result.removeAll(defaultTerm);
        sort(result);
        result.prepend(defaultTerm);
    }

    return result;
}

const char kTerminalVersion[] = "4.8";
const char kTerminalVersionKey[] = "General/Terminal/SettingsVersion";
const char kTerminalCommandKey[] = "General/Terminal/Command";
const char kTerminalOpenOptionsKey[] = "General/Terminal/OpenOptions";
const char kTerminalExecuteOptionsKey[] = "General/Terminal/ExecuteOptions";

TerminalCommand ConsoleProcess::terminalEmulator(const QSettings *settings)
{
    if (settings && HostOsInfo::isAnyUnixHost()) {
        if (settings->value(kTerminalVersionKey).toString() == kTerminalVersion) {
            if (settings->contains(kTerminalCommandKey))
                return {settings->value(kTerminalCommandKey).toString(),
                                    settings->value(kTerminalOpenOptionsKey).toString(),
                                    settings->value(kTerminalExecuteOptionsKey).toString()};
        } else {
            // TODO remove reading of old settings some time after 4.8
            const QString value = settings->value("General/TerminalEmulator").toString().trimmed();
            if (!value.isEmpty()) {
                // split off command and options
                const QStringList splitCommand = QtcProcess::splitArgs(value);
                if (QTC_GUARD(!splitCommand.isEmpty())) {
                    const QString command = splitCommand.first();
                    const QStringList quotedArgs = Utils::transform(splitCommand.mid(1),
                                                                    &QtcProcess::quoteArgUnix);
                    const QString options = quotedArgs.join(' ');
                    return {command, "", options};
                }
            }
        }
    }

    return defaultTerminalEmulator();
}

void ConsoleProcess::setTerminalEmulator(QSettings *settings, const TerminalCommand &term)
{
    if (HostOsInfo::isAnyUnixHost()) {
        settings->setValue(kTerminalVersionKey, kTerminalVersion);
        if (term == defaultTerminalEmulator()) {
            settings->remove(kTerminalCommandKey);
            settings->remove(kTerminalOpenOptionsKey);
            settings->remove(kTerminalExecuteOptionsKey);
        } else {
            settings->setValue(kTerminalCommandKey, term.command);
            settings->setValue(kTerminalOpenOptionsKey, term.openArgs);
            settings->setValue(kTerminalExecuteOptionsKey, term.executeArgs);
        }
    }
}

static QString quoteWinCommand(const QString &program)
{
    const QChar doubleQuote = QLatin1Char('"');

    // add the program as the first arg ... it works better
    QString programName = program;
    programName.replace(QLatin1Char('/'), QLatin1Char('\\'));
    if (!programName.startsWith(doubleQuote) && !programName.endsWith(doubleQuote)
            && programName.contains(QLatin1Char(' '))) {
        programName.prepend(doubleQuote);
        programName.append(doubleQuote);
    }
    return programName;
}

static QString quoteWinArgument(const QString &arg)
{
    if (arg.isEmpty())
        return QString::fromLatin1("\"\"");

    QString ret(arg);
    // Quotes are escaped and their preceding backslashes are doubled.
    ret.replace(QRegularExpression("(\\\\*)\""), "\\1\\1\\\"");
    if (ret.contains(QRegularExpression("\\s"))) {
        // The argument must not end with a \ since this would be interpreted
        // as escaping the quote -- rather put the \ behind the quote: e.g.
        // rather use "foo"\ than "foo\"
        int i = ret.length();
        while (i > 0 && ret.at(i - 1) == QLatin1Char('\\'))
            --i;
        ret.insert(i, QLatin1Char('"'));
        ret.prepend(QLatin1Char('"'));
    }
    return ret;
}

// Quote a Windows command line correctly for the "CreateProcess" API
QString createWinCommandline(const QString &program, const QStringList &args)
{
    QString programName = quoteWinCommand(program);
    for (const QString &arg : args) {
        programName += QLatin1Char(' ');
        programName += quoteWinArgument(arg);
    }
    return programName;
}

QString createWinCommandline(const QString &program, const QString &args)
{
    QString programName = quoteWinCommand(program);
    if (!args.isEmpty()) {
        programName += QLatin1Char(' ');
        programName += args;
    }
    return programName;
}


bool ConsoleProcess::startTerminalEmulator(QSettings *settings, const QString &workingDir,
                                           const Environment &env)
{
#ifdef Q_OS_WIN
    Q_UNUSED(settings)

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pinfo;
    ZeroMemory(&pinfo, sizeof(pinfo));

    QString cmdLine = createWinCommandline(
                QString::fromLocal8Bit(qgetenv("COMSPEC")), QString());
    // cmdLine is assumed to be detached -
    // https://blogs.msdn.microsoft.com/oldnewthing/20090601-00/?p=18083

    QString totalEnvironment = env.toStringList().join(QChar(QChar::Null)) + QChar(QChar::Null);
    LPVOID envPtr = (env != Environment::systemEnvironment())
            ? (WCHAR *)(totalEnvironment.utf16()) : nullptr;

    bool success = CreateProcessW(0, (WCHAR *)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
                                  envPtr, workingDir.isEmpty() ? 0 : (WCHAR *)workingDir.utf16(),
                                  &si, &pinfo);

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
    }

    return success;
#else
    const TerminalCommand term = terminalEmulator(settings);
    QProcess process;
    process.setProgram(term.command);
    process.setArguments(QtcProcess::splitArgs(term.openArgs));
    process.setProcessEnvironment(env.toProcessEnvironment());
    process.setWorkingDirectory(workingDir);

    return process.startDetached();
#endif
}

void ConsoleProcess::setAbortOnMetaChars(bool abort)
{
    d->m_abortOnMetaChars = abort;
}

qint64 ConsoleProcess::applicationMainThreadID() const
{
    if (HostOsInfo::isWindowsHost())
        return d->m_appMainThreadId;
    return -1;
}

bool ConsoleProcess::start()
{
    if (isRunning())
        return false;

    d->m_errorString.clear();
    d->m_error = QProcess::UnknownError;

#ifdef Q_OS_WIN

    QString pcmd;
    QString pargs;
    if (d->m_mode != Run) { // The debugger engines already pre-process the arguments.
        pcmd = d->m_commandLine.executable().toString();
        pargs = d->m_commandLine.arguments();
    } else {
        QtcProcess::Arguments outArgs;
        QtcProcess::prepareCommand(d->m_commandLine.executable().toString(),
                                   d->m_commandLine.arguments(),
                                   &pcmd, &outArgs, OsTypeWindows,
                                   &d->m_environment, &d->m_workingDir);
        pargs = outArgs.toWindowsArgs();
    }

    const QString err = stubServerListen();
    if (!err.isEmpty()) {
        emitError(QProcess::FailedToStart, msgCommChannelFailed(err));
        return false;
    }

    QStringList env = d->m_environment.toStringList();
    if (!env.isEmpty()) {
        d->m_tempFile = new QTemporaryFile();
        if (!d->m_tempFile->open()) {
            stubServerShutdown();
            emitError(QProcess::FailedToStart, msgCannotCreateTempFile(d->m_tempFile->errorString()));
            delete d->m_tempFile;
            d->m_tempFile = nullptr;
            return false;
        }
        QTextStream out(d->m_tempFile);
        out.setCodec("UTF-16LE");
        out.setGenerateByteOrderMark(false);

        // Add PATH and SystemRoot environment variables in case they are missing
        const QStringList fixedEnvironment = [env] {
            QStringList envStrings = env;
            // add PATH if necessary (for DLL loading)
            if (envStrings.filter(QRegularExpression("^PATH=.*", QRegularExpression::CaseInsensitiveOption)).isEmpty()) {
                QByteArray path = qgetenv("PATH");
                if (!path.isEmpty())
                    envStrings.prepend(QString::fromLatin1("PATH=%1").arg(QString::fromLocal8Bit(path)));
            }
            // add systemroot if needed
            if (envStrings.filter(QRegularExpression("^SystemRoot=.*", QRegularExpression::CaseInsensitiveOption)).isEmpty()) {
                QByteArray systemRoot = qgetenv("SystemRoot");
                if (!systemRoot.isEmpty())
                    envStrings.prepend(QString::fromLatin1("SystemRoot=%1").arg(QString::fromLocal8Bit(systemRoot)));
            }
            return envStrings;
        }();

        for (const QString &var : fixedEnvironment)
            out << var << QChar(0);
        out << QChar(0);
        out.flush();
        if (out.status() != QTextStream::Ok) {
            stubServerShutdown();
            emitError(QProcess::FailedToStart, msgCannotWriteTempFile());
            delete d->m_tempFile;
            d->m_tempFile = nullptr;
            return false;
        }
    }

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    d->m_pid = new PROCESS_INFORMATION;
    ZeroMemory(d->m_pid, sizeof(PROCESS_INFORMATION));

    QString workDir = QDir::toNativeSeparators(workingDirectory());
    if (!workDir.isEmpty() && !workDir.endsWith(QLatin1Char('\\')))
        workDir.append(QLatin1Char('\\'));

    QStringList stubArgs;
    stubArgs << modeOption(d->m_mode)
             << d->m_stubServer.fullServerName()
             << workDir
             << (d->m_tempFile ? d->m_tempFile->fileName() : QString())
             << createWinCommandline(pcmd, pargs)
             << msgPromptToClose();

    const QString cmdLine = createWinCommandline(
            QCoreApplication::applicationDirPath() + QLatin1String("/qtcreator_process_stub.exe"), stubArgs);

    bool success = CreateProcessW(0, (WCHAR*)cmdLine.utf16(),
                                  0, 0, FALSE, CREATE_NEW_CONSOLE,
                                  0, 0,
                                  &si, d->m_pid);

    if (!success) {
        delete d->m_pid;
        d->m_pid = nullptr;
        delete d->m_tempFile;
        d->m_tempFile = nullptr;
        stubServerShutdown();
        emitError(QProcess::FailedToStart, tr("The process \"%1\" could not be started: %2").arg(cmdLine, winErrorMessage(GetLastError())));
        return false;
    }

    d->processFinishedNotifier = new QWinEventNotifier(d->m_pid->hProcess, this);
    connect(d->processFinishedNotifier, &QWinEventNotifier::activated,
            this, &ConsoleProcess::stubExited);

#else

    QtcProcess::SplitError perr;
    QtcProcess::Arguments pargs = QtcProcess::prepareArgs(d->m_commandLine.arguments(),
                                                          &perr,
                                                          HostOsInfo::hostOs(),
                                                          &d->m_environment,
                                                          &d->m_workingDir,
                                                          d->m_abortOnMetaChars);

    QString pcmd;
    if (perr == QtcProcess::SplitOk) {
        pcmd = d->m_commandLine.executable().toString();
    } else {
        if (perr != QtcProcess::FoundMeta) {
            emitError(QProcess::FailedToStart, tr("Quoting error in command."));
            return false;
        }
        if (d->m_mode == Debug) {
            // FIXME: QTCREATORBUG-2809
            emitError(QProcess::FailedToStart, tr("Debugging complex shell commands in a terminal"
                                 " is currently not supported."));
            return false;
        }
        pcmd = QLatin1String("/bin/sh");
        pargs = QtcProcess::Arguments::createUnixArgs(
                        {"-c", (QtcProcess::quoteArg(d->m_commandLine.executable().toString())
                         + ' ' + d->m_commandLine.arguments())});
    }

    QtcProcess::SplitError qerr;
    const TerminalCommand terminal = terminalEmulator(d->m_settings);
    const QtcProcess::Arguments terminalArgs = QtcProcess::prepareArgs(terminal.executeArgs,
                                                                       &qerr,
                                                                       HostOsInfo::hostOs(),
                                                                       &d->m_environment,
                                                                       &d->m_workingDir);
    if (qerr != QtcProcess::SplitOk) {
        emitError(QProcess::FailedToStart, qerr == QtcProcess::BadQuoting
                          ? tr("Quoting error in terminal command.")
                          : tr("Terminal command may not be a shell command."));
        return false;
    }

    const QString err = stubServerListen();
    if (!err.isEmpty()) {
        emitError(QProcess::FailedToStart, msgCommChannelFailed(err));
        return false;
    }

    d->m_environment.unset(QLatin1String("TERM"));
    const QStringList env = d->m_environment.toStringList();
    if (!env.isEmpty()) {
        d->m_tempFile = new QTemporaryFile();
        if (!d->m_tempFile->open()) {
            stubServerShutdown();
            emitError(QProcess::FailedToStart, msgCannotCreateTempFile(d->m_tempFile->errorString()));
            delete d->m_tempFile;
            d->m_tempFile = nullptr;
            return false;
        }
        QByteArray contents;
        for (const QString &var : env) {
            QByteArray l8b = var.toLocal8Bit();
            contents.append(l8b.constData(), l8b.size() + 1);
        }
        if (d->m_tempFile->write(contents) != contents.size() || !d->m_tempFile->flush()) {
            stubServerShutdown();
            emitError(QProcess::FailedToStart, msgCannotWriteTempFile());
            delete d->m_tempFile;
            d->m_tempFile = nullptr;
            return false;
        }
    }

    const QString stubPath = QCoreApplication::applicationDirPath()
            + QLatin1String("/" QTC_REL_TOOLS_PATH "/qtcreator_process_stub");
    QStringList allArgs = terminalArgs.toUnixArgs()
                          << stubPath
                          << modeOption(d->m_mode)
                          << d->m_stubServer.fullServerName()
                          << msgPromptToClose()
                          << workingDirectory()
                          << (d->m_tempFile ? d->m_tempFile->fileName() : QString())
                          << QString::number(getpid())
                          << pcmd
                          << pargs.toUnixArgs();

    if (terminal.needsQuotes)
        allArgs = QStringList { QtcProcess::joinArgs(allArgs) };

    d->m_process.start(terminal.command, allArgs);
    if (!d->m_process.waitForStarted()) {
        stubServerShutdown();
        emitError(QProcess::UnknownError, tr("Cannot start the terminal emulator \"%1\", change the setting in the "
                             "Environment options.").arg(terminal.command));
        delete d->m_tempFile;
        d->m_tempFile = nullptr;
        return false;
    }
    d->m_stubConnectTimer = new QTimer(this);
    connect(d->m_stubConnectTimer, &QTimer::timeout, this, &ConsoleProcess::stop);
    d->m_stubConnectTimer->setSingleShot(true);
    d->m_stubConnectTimer->start(10000);

#endif

    return true;
}

void ConsoleProcess::killProcess()
{
#ifdef Q_OS_WIN
    if (d->m_hInferior != NULL) {
        TerminateProcess(d->m_hInferior, (unsigned)-1);
        cleanupInferior();
    }
#else
    if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
        d->m_stubSocket->write("k", 1);
        d->m_stubSocket->flush();
    }
    d->m_appPid = 0;
#endif
}

void ConsoleProcess::killStub()
{
#ifdef Q_OS_WIN
    if (d->m_pid) {
        TerminateProcess(d->m_pid->hProcess, (unsigned)-1);
        WaitForSingleObject(d->m_pid->hProcess, INFINITE);
        cleanupStub();
    }
#else
    if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
        d->m_stubSocket->write("s", 1);
        d->m_stubSocket->flush();
    }
    stubServerShutdown();
    d->m_stubPid = 0;
#endif
}

void ConsoleProcess::stop()
{
    killProcess();
    killStub();
    if (isRunning() && HostOsInfo::isAnyUnixHost()) {
        d->m_process.terminate();
        if (!d->m_process.waitForFinished(1000) && d->m_process.state() == QProcess::Running) {
            d->m_process.kill();
            d->m_process.waitForFinished();
        }
    }
}

bool ConsoleProcess::isRunning() const
{
#ifdef Q_OS_WIN
    return d->m_pid != nullptr;
#else
    return d->m_process.state() != QProcess::NotRunning
            || (d->m_stubSocket && d->m_stubSocket->isOpen());
#endif
}

QString ConsoleProcess::stubServerListen()
{
#ifdef Q_OS_WIN
    if (d->m_stubServer.listen(QString::fromLatin1("creator-%1-%2")
                            .arg(QCoreApplication::applicationPid())
                            .arg(rand())))
        return QString();
    return d->m_stubServer.errorString();
#else
    // We need to put the socket in a private directory, as some systems simply do not
    // check the file permissions of sockets.
    QString stubFifoDir;
    while (true) {
        {
            QTemporaryFile tf;
            if (!tf.open())
                return msgCannotCreateTempFile(tf.errorString());
            stubFifoDir = tf.fileName();
        }
        // By now the temp file was deleted again
        d->m_stubServerDir = QFile::encodeName(stubFifoDir);
        if (!::mkdir(d->m_stubServerDir.constData(), 0700))
            break;
        if (errno != EEXIST)
            return msgCannotCreateTempDir(stubFifoDir, QString::fromLocal8Bit(strerror(errno)));
    }
    const QString stubServer  = stubFifoDir + QLatin1String("/stub-socket");
    if (!d->m_stubServer.listen(stubServer)) {
        ::rmdir(d->m_stubServerDir.constData());
        return tr("Cannot create socket \"%1\": %2").arg(stubServer, d->m_stubServer.errorString());
    }
    return QString();
#endif
}

void ConsoleProcess::stubServerShutdown()
{
#ifdef Q_OS_WIN
    delete d->m_stubSocket;
    d->m_stubSocket = nullptr;
    if (d->m_stubServer.isListening())
        d->m_stubServer.close();
#else
    if (d->m_stubSocket) {
        readStubOutput(); // we could get the shutdown signal before emptying the buffer
        d->m_stubSocket->disconnect(); // avoid getting queued readyRead signals
        d->m_stubSocket->deleteLater(); // we might be called from the disconnected signal of m_stubSocket
    }
    d->m_stubSocket = nullptr;
    if (d->m_stubServer.isListening()) {
        d->m_stubServer.close();
        ::rmdir(d->m_stubServerDir.constData());
    }
#endif
}

void ConsoleProcess::stubConnectionAvailable()
{
    d->m_stubConnected = true;
    emit stubStarted();

    if (d->m_stubConnectTimer) {
        delete d->m_stubConnectTimer;
        d->m_stubConnectTimer = nullptr;
    }

    d->m_stubSocket = d->m_stubServer.nextPendingConnection();
    connect(d->m_stubSocket, &QIODevice::readyRead, this, &ConsoleProcess::readStubOutput);

    if (HostOsInfo::isAnyUnixHost())
        connect(d->m_stubSocket, &QLocalSocket::disconnected, this, &ConsoleProcess::stubExited);
}

static QString errorMsg(int code)
{
    return QString::fromLocal8Bit(strerror(code));
}

void ConsoleProcess::readStubOutput()
{
    while (d->m_stubSocket->canReadLine()) {
        QByteArray out = d->m_stubSocket->readLine();
#ifdef Q_OS_WIN
        out.chop(2); // \r\n
        if (out.startsWith("err:chdir ")) {
            emitError(QProcess::FailedToStart, msgCannotChangeToWorkDir(workingDirectory(), winErrorMessage(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emitError(QProcess::FailedToStart, msgCannotExecute(d->m_commandLine.executable().toUserOutput(), winErrorMessage(out.mid(9).toInt())));
        } else if (out.startsWith("thread ")) { // Windows only
            d->m_appMainThreadId = out.mid(7).toLongLong();
        } else if (out.startsWith("pid ")) {
            // Will not need it any more
            delete d->m_tempFile;
            d->m_tempFile = nullptr;
            d->m_appPid = out.mid(4).toLongLong();

            d->m_hInferior = OpenProcess(
                    SYNCHRONIZE | PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,
                    FALSE, d->m_appPid);
            if (d->m_hInferior == NULL) {
                emitError(QProcess::FailedToStart, tr("Cannot obtain a handle to the inferior: %1")
                                  .arg(winErrorMessage(GetLastError())));
                // Uhm, and now what?
                continue;
            }
            d->inferiorFinishedNotifier = new QWinEventNotifier(d->m_hInferior, this);
            connect(d->inferiorFinishedNotifier, &QWinEventNotifier::activated, this, [this] {
                DWORD chldStatus;

                if (!GetExitCodeProcess(d->m_hInferior, &chldStatus))
                emitError(QProcess::UnknownError, tr("Cannot obtain exit status from inferior: %1")
                          .arg(winErrorMessage(GetLastError())));
                cleanupInferior();
                d->m_appStatus = QProcess::NormalExit;
                d->m_appCode = chldStatus;
                emit processStopped(d->m_appCode, d->m_appStatus);
            });

            emit processStarted();
        } else {
            emitError(QProcess::UnknownError, msgUnexpectedOutput(out));
            TerminateProcess(d->m_pid->hProcess, (unsigned)-1);
            break;
        }
#else
        out.chop(1); // \n
        if (out.startsWith("err:chdir ")) {
            emitError(QProcess::FailedToStart, msgCannotChangeToWorkDir(workingDirectory(), errorMsg(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emitError(QProcess::FailedToStart, msgCannotExecute(d->m_commandLine.executable().toString(), errorMsg(out.mid(9).toInt())));
        } else if (out.startsWith("spid ")) {
            delete d->m_tempFile;
            d->m_tempFile = nullptr;

            d->m_stubPid = out.mid(4).toInt();
        } else if (out.startsWith("pid ")) {
            d->m_appPid = out.mid(4).toInt();
            emit processStarted();
        } else if (out.startsWith("exit ")) {
            d->m_appStatus = QProcess::NormalExit;
            d->m_appCode = out.mid(5).toInt();
            d->m_appPid = 0;
            emit processStopped(d->m_appCode, d->m_appStatus);
        } else if (out.startsWith("crash ")) {
            d->m_appStatus = QProcess::CrashExit;
            d->m_appCode = out.mid(6).toInt();
            d->m_appPid = 0;
            emit processStopped(d->m_appCode, d->m_appStatus);
        } else {
            emitError(QProcess::UnknownError, msgUnexpectedOutput(out));
            d->m_stubPid = 0;
            d->m_process.terminate();
            break;
        }
#endif
    } // while
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the pid for the kill on Windows
    // or the error status elsewhere.
    if (d->m_stubSocket && d->m_stubSocket->state() == QLocalSocket::ConnectedState)
        d->m_stubSocket->waitForDisconnected();

#ifdef Q_OS_WIN
    cleanupStub();
    if (d->m_hInferior != NULL) {
        TerminateProcess(d->m_hInferior, (unsigned)-1);
        cleanupInferior();
        d->m_appStatus = QProcess::CrashExit;
        d->m_appCode = -1;
        emit processStopped(d->m_appCode, d->m_appStatus);
    }
#else
    stubServerShutdown();
    d->m_stubPid = 0;
    delete d->m_tempFile;
    d->m_tempFile = nullptr;
    if (d->m_appPid) {
        d->m_appStatus = QProcess::CrashExit;
        d->m_appCode = -1;
        d->m_appPid = 0;
        emit processStopped(d->m_appCode, d->m_appStatus); // Maybe it actually did not, but keep state consistent
    }
#endif
    emit stubStopped();
}

void ConsoleProcess::detachStub()
{
    if (HostOsInfo::isAnyUnixHost()) {
        if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
            d->m_stubSocket->write("d", 1);
            d->m_stubSocket->flush();
        }
        stubServerShutdown();
        d->m_stubPid = 0;
    }
}

void ConsoleProcess::cleanupInferior()
{
#ifdef Q_OS_WIN
    delete d->inferiorFinishedNotifier;
    d->inferiorFinishedNotifier = nullptr;
    CloseHandle(d->m_hInferior);
    d->m_hInferior = NULL;
    d->m_appPid = 0;
#endif
}

void ConsoleProcess::cleanupStub()
{
#ifdef Q_OS_WIN
    stubServerShutdown();
    delete d->processFinishedNotifier;
    d->processFinishedNotifier = nullptr;
    CloseHandle(d->m_pid->hThread);
    CloseHandle(d->m_pid->hProcess);
    delete d->m_pid;
    d->m_pid = nullptr;
    delete d->m_tempFile;
    d->m_tempFile = nullptr;
#endif
}

void ConsoleProcess::setMode(Mode m)
{
    d->m_mode = m;
}

ConsoleProcess::Mode ConsoleProcess::mode() const
{
    return d->m_mode;
}

QString ConsoleProcess::modeOption(Mode m)
{
    switch (m) {
        case Debug:
        return QLatin1String("debug");
        case Suspend:
        return QLatin1String("suspend");
        case Run:
        break;
    }
    return QLatin1String("run");
}

qint64 ConsoleProcess::applicationPID() const
{
    return d->m_appPid;
}

int ConsoleProcess::exitCode() const
{
    return d->m_appCode;
} // This will be the signal number if exitStatus == CrashExit

QProcess::ExitStatus ConsoleProcess::exitStatus() const
{
    return d->m_appStatus;
}

void ConsoleProcess::setWorkingDirectory(const QString &dir)
{
    d->m_workingDir = dir;
}

QString ConsoleProcess::workingDirectory() const
{
    return d->m_workingDir;
}

void ConsoleProcess::setEnvironment(const Environment &env)
{
    d->m_environment = env;
}

Environment ConsoleProcess::environment() const
{
    return d->m_environment;
}

QProcess::ProcessError ConsoleProcess::error() const
{
    return d->m_error;
}

QString ConsoleProcess::errorString() const
{
    return d->m_errorString;
}

QString ConsoleProcess::msgCommChannelFailed(const QString &error)
{
    return tr("Cannot set up communication channel: %1").arg(error);
}

QString ConsoleProcess::msgPromptToClose()
{
    //! Showed in a terminal which might have
    //! a different character set on Windows.
    return tr("Press <RETURN> to close this window...");
}

QString ConsoleProcess::msgCannotCreateTempFile(const QString &why)
{
    return tr("Cannot create temporary file: %1").arg(why);
}

QString ConsoleProcess::msgCannotWriteTempFile()
{
    return tr("Cannot write temporary file. Disk full?");
}

QString ConsoleProcess::msgCannotCreateTempDir(const QString & dir, const QString &why)
{
    return tr("Cannot create temporary directory \"%1\": %2").arg(dir, why);
}

QString ConsoleProcess::msgUnexpectedOutput(const QByteArray &what)
{
    return tr("Unexpected output from helper program (%1).").arg(QString::fromLatin1(what));
}

QString ConsoleProcess::msgCannotChangeToWorkDir(const QString & dir, const QString &why)
{
    return tr("Cannot change to working directory \"%1\": %2").arg(dir, why);
}

QString ConsoleProcess::msgCannotExecute(const QString & p, const QString &why)
{
    return tr("Cannot execute \"%1\": %2").arg(p, why);
}

void ConsoleProcess::emitError(QProcess::ProcessError err, const QString &errorString)
{
    d->m_error = err;
    d->m_errorString = errorString;
    emit error(err);
    emit processError(errorString);
}

bool TerminalCommand::operator==(const TerminalCommand &other) const
{
    return other.command == command && other.openArgs == openArgs
           && other.executeArgs == executeArgs;
}

bool TerminalCommand::operator<(const TerminalCommand &other) const
{
    if (command == other.command) {
        if (openArgs == other.openArgs)
            return executeArgs < other.executeArgs;
        return openArgs < other.openArgs;
    }
    return command < other.command;
}

} // Utils
