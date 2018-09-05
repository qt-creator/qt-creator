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

#include "consoleprocess_p.h"

#include "qtcprocess.h"

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>
#include <QSettings>
#include <QTimer>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

namespace Utils {

ConsoleProcessPrivate::ConsoleProcessPrivate() = default;

ConsoleProcess::ConsoleProcess(QObject *parent)  :
    QObject(parent), d(new ConsoleProcessPrivate)
{
    connect(&d->m_stubServer, &QLocalServer::newConnection,
            this, &ConsoleProcess::stubConnectionAvailable);

    d->m_process.setProcessChannelMode(QProcess::ForwardedChannels);
}

qint64 ConsoleProcess::applicationMainThreadID() const
{
    return -1;
}

void ConsoleProcess::setSettings(QSettings *settings)
{
    d->m_settings = settings;
}

bool ConsoleProcess::start(const QString &program, const QString &args)
{
    if (isRunning())
        return false;

    d->m_errorString.clear();
    d->m_error = QProcess::UnknownError;

    QtcProcess::SplitError perr;
    QtcProcess::Arguments pargs = QtcProcess::prepareArgs(args, &perr, HostOsInfo::hostOs(),
                                                          &d->m_environment, &d->m_workingDir);
    QString pcmd;
    if (perr == QtcProcess::SplitOk) {
        pcmd = program;
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
                    QStringList({"-c", (QtcProcess::quoteArg(program) + ' ' + args)}));
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
        QByteArray contents;
        foreach (const QString &var, env) {
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
    const QStringList allArgs = terminalArgs.toUnixArgs()
                                << stubPath
                                << modeOption(d->m_mode)
                                << d->m_stubServer.fullServerName()
                                << msgPromptToClose()
                                << workingDirectory()
                                << (d->m_tempFile ? d->m_tempFile->fileName() : QString())
                                << QString::number(getpid())
                                << pcmd
                                << pargs.toUnixArgs();

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
    d->m_executable = program;
    return true;
}

void ConsoleProcess::killProcess()
{
    if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
        d->m_stubSocket->write("k", 1);
        d->m_stubSocket->flush();
    }
    d->m_appPid = 0;
}

void ConsoleProcess::killStub()
{
    if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
        d->m_stubSocket->write("s", 1);
        d->m_stubSocket->flush();
    }
    stubServerShutdown();
    d->m_stubPid = 0;
}

void ConsoleProcess::detachStub()
{
    if (d->m_stubSocket && d->m_stubSocket->isWritable()) {
        d->m_stubSocket->write("d", 1);
        d->m_stubSocket->flush();
    }
    stubServerShutdown();
    d->m_stubPid = 0;
}

void ConsoleProcess::stop()
{
    killProcess();
    killStub();
    if (isRunning()) {
        d->m_process.terminate();
        if (!d->m_process.waitForFinished(1000) && d->m_process.state() == QProcess::Running) {
            d->m_process.kill();
            d->m_process.waitForFinished();
        }
    }
}

bool ConsoleProcess::isRunning() const
{
    return d->m_process.state() != QProcess::NotRunning
            || (d->m_stubSocket && d->m_stubSocket->isOpen());
}

QString ConsoleProcess::stubServerListen()
{
    // We need to put the socket in a private directory, as some systems simply do not
    // check the file permissions of sockets.
    QString stubFifoDir;
    forever {
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
}

void ConsoleProcess::stubServerShutdown()
{
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
}

void ConsoleProcess::stubConnectionAvailable()
{
    if (d->m_stubConnectTimer) {
        delete d->m_stubConnectTimer;
        d->m_stubConnectTimer = nullptr;
    }
    d->m_stubConnected = true;
    emit stubStarted();
    d->m_stubSocket = d->m_stubServer.nextPendingConnection();
    connect(d->m_stubSocket, &QIODevice::readyRead, this, &ConsoleProcess::readStubOutput);
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
        out.chop(1); // \n
        if (out.startsWith("err:chdir ")) {
            emitError(QProcess::FailedToStart, msgCannotChangeToWorkDir(workingDirectory(), errorMsg(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emitError(QProcess::FailedToStart, msgCannotExecute(d->m_executable, errorMsg(out.mid(9).toInt())));
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
    }
}

void ConsoleProcess::stubExited()
{
    // The stub exit might get noticed before we read the error status.
    if (d->m_stubSocket && d->m_stubSocket->state() == QLocalSocket::ConnectedState)
        d->m_stubSocket->waitForDisconnected();
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
    emit stubStopped();
}

Q_GLOBAL_STATIC_WITH_ARGS(const QVector<TerminalCommand>, knownTerminals, (
{
    {"x-terminal-emulator", "", "-e"},
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
        defaultTerm = []() -> TerminalCommand {
            if (HostOsInfo::isMacHost()) {
                const QString termCmd = QCoreApplication::applicationDirPath()
                                        + "/../Resources/scripts/openTerminal.py";
                if (QFileInfo::exists(termCmd))
                    return {termCmd, "", ""};
                return {"/usr/X11/bin/xterm", "", "-e"};
            }
            const Environment env = Environment::systemEnvironment();
            for (const TerminalCommand &term : *knownTerminals) {
                const QString result = env.searchInPath(term.command).toString();
                if (!result.isEmpty())
                    return {result, term.openArgs, term.executeArgs};
            }
            return {"xterm", "", "-e"};
        }();
    }
    return defaultTerm;
}

QVector<TerminalCommand> ConsoleProcess::availableTerminalEmulators()
{
    QVector<TerminalCommand> result;
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
    return result;
}

const char kTerminalVersion[] = "4.8";
const char kTerminalVersionKey[] = "General/Terminal/SettingsVersion";
const char kTerminalCommandKey[] = "General/Terminal/Command";
const char kTerminalOpenOptionsKey[] = "General/Terminal/OpenOptions";
const char kTerminalExecuteOptionsKey[] = "General/Terminal/ExecuteOptions";

TerminalCommand ConsoleProcess::terminalEmulator(const QSettings *settings)
{
    if (settings) {
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

bool ConsoleProcess::startTerminalEmulator(QSettings *settings, const QString &workingDir,
                                           const Utils::Environment &env)
{
    const TerminalCommand term = terminalEmulator(settings);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    // for 5.9 and below we cannot set the environment
    Q_UNUSED(env);
    return QProcess::startDetached(term.command, QtcProcess::splitArgs(term.openArgs),
                                   workingDir);
#else
    QProcess process;
    process.setProgram(term.command);
    process.setArguments(QtcProcess::splitArgs(term.openArgs));
    process.setProcessEnvironment(env.toProcessEnvironment());
    process.setWorkingDirectory(workingDir);

    return process.startDetached();
#endif
}

} // namespace Utils
