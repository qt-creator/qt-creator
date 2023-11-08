// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "terminalinterface.h"

#include "utilstr.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QLoggingCategory>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>
#include <QThread>
#include <QTimer>

Q_LOGGING_CATEGORY(terminalInterfaceLog, "qtc.terminalinterface", QtWarningMsg)

namespace Utils {

static QString msgCommChannelFailed(const QString &error)
{
    return Tr::tr("Cannot set up communication channel: %1").arg(error);
}

static QString msgCannotCreateTempFile(const QString &why)
{
    return Tr::tr("Cannot create temporary file: %1").arg(why);
}

static QString msgCannotWriteTempFile()
{
    return Tr::tr("Cannot write temporary file. Disk full?");
}

static QString msgCannotCreateTempDir(const QString &dir, const QString &why)
{
    return Tr::tr("Cannot create temporary directory \"%1\": %2").arg(dir, why);
}

static QString msgUnexpectedOutput(const QByteArray &what)
{
    return Tr::tr("Unexpected output from helper program (%1).").arg(QString::fromLatin1(what));
}

static QString msgCannotChangeToWorkDir(const FilePath &dir, const QString &why)
{
    return Tr::tr("Cannot change to working directory \"%1\": %2").arg(dir.toUserOutput(), why);
}

static QString msgCannotExecute(const FilePath &p, const QString &why)
{
    return Tr::tr("Cannot execute \"%1\": %2").arg(p.toUserOutput(), why);
}

static QString msgPromptToClose()
{
    // Shown in a terminal which might have a different character set on Windows.
    return Tr::tr("Press <RETURN> to close this window...");
}

class TerminalInterfacePrivate : public QObject
{
    Q_OBJECT
public:
    TerminalInterfacePrivate(TerminalInterface *p, bool waitOnExit)
        : q(p)
        , waitOnExit(waitOnExit)
    {
        connect(&stubServer,
                &QLocalServer::newConnection,
                q,
                &TerminalInterface::onNewStubConnection);
    }

public:
    QLocalServer stubServer;
    QLocalSocket *stubSocket = nullptr;

    int stubProcessId = 0;
    int inferiorProcessId = 0;
    int inferiorThreadId = 0;

    std::unique_ptr<QTemporaryFile> envListFile;
    QTemporaryDir tempDir;

    std::unique_ptr<QTimer> stubConnectTimeoutTimer;

    ProcessResultData processResultData;
    TerminalInterface *q;

    StubCreator *stubCreator{nullptr};

    const bool waitOnExit;
    bool didInferiorRun{false};
};

TerminalInterface::TerminalInterface(bool waitOnExit)
    : d(new TerminalInterfacePrivate(this, waitOnExit))
{}

TerminalInterface::~TerminalInterface()
{
    if (d->stubSocket && d->stubSocket->state() == QLocalSocket::ConnectedState) {
        if (d->inferiorProcessId)
            killInferiorProcess();
        killStubProcess();
    }
    if (d->stubCreator)
        d->stubCreator->deleteLater();
    delete d;
}

void TerminalInterface::setStubCreator(StubCreator *creator)
{
    d->stubCreator = creator;
}

int TerminalInterface::inferiorProcessId() const
{
    return d->inferiorProcessId;
}

int TerminalInterface::inferiorThreadId() const
{
    return d->inferiorThreadId;
}

static QString errnoToString(int code)
{
    return QString::fromLocal8Bit(strerror(code));
}

void TerminalInterface::onNewStubConnection()
{
    d->stubConnectTimeoutTimer.reset();

    d->stubSocket = d->stubServer.nextPendingConnection();
    if (!d->stubSocket)
        return;

    connect(d->stubSocket, &QIODevice::readyRead, this, &TerminalInterface::onStubReadyRead);

    if (HostOsInfo::isAnyUnixHost())
        connect(d->stubSocket, &QLocalSocket::disconnected, this, &TerminalInterface::onStubExited);
}

void TerminalInterface::onStubExited()
{
    // The stub exit might get noticed before we read the pid for the kill on Windows
    // or the error status elsewhere.
    if (d->stubSocket && d->stubSocket->state() == QLocalSocket::ConnectedState)
        d->stubSocket->waitForDisconnected();

    shutdownStubServer();
    d->envListFile.reset();

    if (d->inferiorProcessId)
        emitFinished(-1, QProcess::CrashExit);
    else if (!d->didInferiorRun) {
        emitError(QProcess::FailedToStart,
                  Tr::tr("Failed to start terminal process. The stub exited before the inferior "
                         "was started."));
    }
}

void TerminalInterface::onStubReadyRead()
{
    while (d->stubSocket && d->stubSocket->canReadLine()) {
        QByteArray out = d->stubSocket->readLine();
        out.chop(1); // remove newline
        if (out.startsWith("err:chdir ")) {
            emitError(QProcess::FailedToStart,
                      msgCannotChangeToWorkDir(m_setup.m_workingDirectory,
                                               errnoToString(out.mid(10).toInt())));
        } else if (out.startsWith("err:exec ")) {
            emitError(QProcess::FailedToStart,
                      msgCannotExecute(m_setup.m_commandLine.executable(),
                                       errnoToString(out.mid(9).toInt())));
        } else if (out.startsWith("spid ")) {
            d->envListFile.reset();
            d->envListFile = nullptr;
        } else if (out.startsWith("pid ")) {
            d->inferiorProcessId = out.mid(4).toInt();
            d->didInferiorRun = true;
            emit started(d->inferiorProcessId, d->inferiorThreadId);
        } else if (out.startsWith("thread ")) { // Windows only
            d->inferiorThreadId = out.mid(7).toLongLong();
        } else if (out.startsWith("exit ")) {
            emitFinished(out.mid(5).toInt(), QProcess::NormalExit);
        } else if (out.startsWith("crash ")) {
            emitFinished(out.mid(6).toInt(), QProcess::CrashExit);
        } else if (out.startsWith("ack ")) {
            qCDebug(terminalInterfaceLog) << "Received ack from stub: " << out;
        } else {
            emitError(QProcess::UnknownError, msgUnexpectedOutput(out));
            break;
        }
    }
}

expected_str<void> TerminalInterface::startStubServer()
{
    if (HostOsInfo::isWindowsHost()) {
        if (d->stubServer.listen(QString::fromLatin1("creator-%1-%2")
                                     .arg(QCoreApplication::applicationPid())
                                     .arg(rand())))
            return {};
        return make_unexpected(d->stubServer.errorString());
    }

    // We need to put the socket in a private directory, as some systems simply do not
    // check the file permissions of sockets.
    if (!QDir(d->tempDir.path())
             .mkdir("socket")) { //  QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner
        return make_unexpected(msgCannotCreateTempDir(d->tempDir.filePath("socket"),
                                                      QString::fromLocal8Bit(strerror(errno))));
    }

    if (!QFile::setPermissions(d->tempDir.filePath("socket"),
                               QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner)) {
        return make_unexpected(Tr::tr("Cannot set permissions on temporary directory \"%1\": %2")
                                   .arg(d->tempDir.filePath("socket"))
                                   .arg(QString::fromLocal8Bit(strerror(errno))));
    }

    const QString socketPath = d->tempDir.filePath("socket/stub-socket");
    if (!d->stubServer.listen(socketPath)) {
        return make_unexpected(
            Tr::tr("Cannot create socket \"%1\": %2").arg(socketPath, d->stubServer.errorString()));
    }
    return {};
}

void TerminalInterface::shutdownStubServer()
{
    if (d->stubSocket) {
        // Read potentially remaining data
        onStubReadyRead();
        // avoid getting queued readyRead signals
        d->stubSocket->disconnect();
        // we might be called from the disconnected signal of stubSocket
        d->stubSocket->deleteLater();
    }
    d->stubSocket = nullptr;
    if (d->stubServer.isListening())
        d->stubServer.close();
}

void TerminalInterface::emitError(QProcess::ProcessError error, const QString &errorString)
{
    d->processResultData.m_error = error;
    d->processResultData.m_errorString = errorString;
    if (error == QProcess::FailedToStart)
        emit done(d->processResultData);
}

void TerminalInterface::emitFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    d->inferiorProcessId = 0;
    d->inferiorThreadId = 0;
    d->processResultData.m_exitCode = exitCode;
    d->processResultData.m_exitStatus = exitStatus;
    emit done(d->processResultData);
}

bool TerminalInterface::isRunning() const
{
    return d->stubSocket && d->stubSocket->isOpen();
}

void TerminalInterface::cleanupAfterStartFailure(const QString &errorMessage)
{
    shutdownStubServer();
    emitError(QProcess::FailedToStart, errorMessage);
    d->envListFile.reset();
}

void TerminalInterface::sendCommand(char c)
{
    if (d->stubSocket && d->stubSocket->isWritable()) {
        d->stubSocket->write(&c, 1);
        d->stubSocket->flush();
    }
}

void TerminalInterface::killInferiorProcess()
{
    sendCommand('k');
    if (d->stubSocket) {
        d->stubSocket->waitForReadyRead();
        emitFinished(-1, QProcess::CrashExit);
    }
}

void TerminalInterface::killStubProcess()
{
    if (!isRunning())
        return;

    sendCommand('s');
    if (d->stubSocket)
        d->stubSocket->waitForReadyRead();
    shutdownStubServer();
}

void TerminalInterface::start()
{
    if (isRunning())
        return;

    if (m_setup.m_terminalMode == TerminalMode::Detached) {
        expected_str<qint64> result;
        QMetaObject::invokeMethod(
            d->stubCreator,
            [this, &result] { result = d->stubCreator->startStubProcess(m_setup); },
            d->stubCreator->thread() == QThread::currentThread() ? Qt::DirectConnection
                                                                 : Qt::BlockingQueuedConnection);

        if (result) {
            emit started(*result, 0);
            emitFinished(0, QProcess::NormalExit);
        } else {
            emitError(QProcess::FailedToStart, result.error());
        }
        return;
    }

    const expected_str<void> result = startStubServer();
    if (!result) {
        emitError(QProcess::FailedToStart, msgCommChannelFailed(result.error()));
        return;
    }

    Environment finalEnv = m_setup.m_environment;

    if (HostOsInfo::isWindowsHost()) {
        if (!finalEnv.hasKey("PATH")) {
            const QString path = qtcEnvironmentVariable("PATH");
            if (!path.isEmpty())
                finalEnv.set("PATH", path);
        }
        if (!finalEnv.hasKey("SystemRoot")) {
            const QString systemRoot = qtcEnvironmentVariable("SystemRoot");
            if (!systemRoot.isEmpty())
                finalEnv.set("SystemRoot", systemRoot);
        }
    } else if (HostOsInfo::isMacHost()) {
        finalEnv.set("TERM", "xterm-256color");
    }

    if (finalEnv.hasChanges()) {
        d->envListFile = std::make_unique<QTemporaryFile>(this);
        if (!d->envListFile->open()) {
            cleanupAfterStartFailure(msgCannotCreateTempFile(d->envListFile->errorString()));
            return;
        }
        QTextStream stream(d->envListFile.get());
        finalEnv.forEachEntry([&stream](const QString &key, const QString &value, bool enabled) {
            if (enabled)
                stream << key << '=' << value << '\0';
        });

        if (d->envListFile->error() != QFile::NoError) {
            cleanupAfterStartFailure(msgCannotWriteTempFile());
            return;
        }
    }

    const FilePath stubPath = FilePath::fromUserInput(QCoreApplication::applicationDirPath())
                                  .pathAppended(QLatin1String(RELATIVE_LIBEXEC_PATH))
                                  .pathAppended((HostOsInfo::isWindowsHost()
                                                     ? QLatin1String("qtcreator_process_stub.exe")
                                                     : QLatin1String("qtcreator_process_stub")));

    CommandLine cmd{stubPath, {"-s", d->stubServer.fullServerName()}};

    if (!m_setup.m_workingDirectory.isEmpty())
        cmd.addArgs({"-w", m_setup.m_workingDirectory.nativePath()});

    if (m_setup.m_terminalMode == TerminalMode::Debug)
        cmd.addArg("-d");

    if (terminalInterfaceLog().isDebugEnabled())
        cmd.addArg("-v");

    if (d->envListFile)
        cmd.addArgs({"-e", d->envListFile->fileName()});

    cmd.addArgs({"--wait", d->waitOnExit ? msgPromptToClose() : ""});

    cmd.addArgs({"--", m_setup.m_commandLine.executable().nativePath()});
    cmd.addArgs(m_setup.m_commandLine.arguments(), CommandLine::Raw);

    QTC_ASSERT(d->stubCreator, return);

    ProcessSetupData stubSetupData;
    stubSetupData.m_commandLine = cmd;

    stubSetupData.m_extraData[TERMINAL_SHELL_NAME]
        = m_setup.m_extraData.value(TERMINAL_SHELL_NAME,
                                    m_setup.m_commandLine.executable().fileName());

    if (m_setup.m_runAsRoot && !HostOsInfo::isWindowsHost()) {
        CommandLine rootCommand("sudo", {});
        rootCommand.addCommandLineAsArgs(cmd);
        stubSetupData.m_commandLine = rootCommand;
    } else {
        stubSetupData.m_commandLine = cmd;
    }

    QMetaObject::invokeMethod(
        d->stubCreator,
        [stubSetupData, this] { d->stubCreator->startStubProcess(stubSetupData); },
        d->stubCreator->thread() == QThread::currentThread() ? Qt::DirectConnection
                                                             : Qt::BlockingQueuedConnection);

    d->stubConnectTimeoutTimer = std::make_unique<QTimer>();

    connect(d->stubConnectTimeoutTimer.get(), &QTimer::timeout, this, [this] {
        killInferiorProcess();
        killStubProcess();
    });
    d->stubConnectTimeoutTimer->setSingleShot(true);
    d->stubConnectTimeoutTimer->start(10000);
}

qint64 TerminalInterface::write(const QByteArray &data)
{
    Q_UNUSED(data);
    QTC_CHECK(false);
    return -1;
}
void TerminalInterface::sendControlSignal(ControlSignal controlSignal)
{
    QTC_ASSERT(m_setup.m_terminalMode != TerminalMode::Detached, return);

    switch (controlSignal) {
    case ControlSignal::Terminate:
    case ControlSignal::Kill:
        killInferiorProcess();
        break;
    case ControlSignal::Interrupt:
        sendCommand('i');
        break;
    case ControlSignal::KickOff:
        sendCommand('c');
        break;
    case ControlSignal::CloseWriteChannel:
        QTC_CHECK(false);
        break;
    }
}

} // namespace Utils

#include "terminalinterface.moc"
