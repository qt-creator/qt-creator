// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "deviceshell.h"

#include "process.h"
#include "processinterface.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(deviceShellLog, "qtc.utils.deviceshell", QtWarningMsg)

namespace Utils {

/*
 * The multiplex script waits for input via stdin.
 *
 * To start a command, a message is sent with
 * the format "<cmd-id> "<base64-encoded-stdin-data>" <commandline>\n"
 * To stop the script, simply send "exit\n" via stdin
 *
 * Once a message is received, two new streams are created that the new process redirects
 * its output to ( $stdoutraw and $stderrraw ).
 *
 * These streams are piped through base64 into the two streams stdoutenc and stderrenc.
 *
 * Two subshells read from these base64 encoded streams, and prepend the command-id,
 * as well as either "O:" or "E:" depending on whether its the stdout or stderr stream.
 *
 * Once the process exits its exit code is send to stdout with the command-id and the type "R".
 *
 */

DeviceShell::DeviceShell(bool forceFailScriptInstallation)
: m_forceFailScriptInstallation(forceFailScriptInstallation)
{
    m_thread.setObjectName("DeviceShell");
    m_thread.start();
}

DeviceShell::~DeviceShell()
{
    if (m_thread.isRunning()) {
        m_thread.quit();
        m_thread.wait();
    }

    QTC_CHECK(!m_shellProcess);
}

/*!
 * \brief DeviceShell::runInShell
 * \param cmd The command to run
 * \param stdInData Data to send to the stdin of the command
 * \return true if the command finished with EXIT_SUCCESS(0)
 *
 * Runs the cmd inside the internal shell process and return stdout, stderr and exit code
 *
 * Will automatically defer to the internal thread
 */
RunResult DeviceShell::runInShell(const CommandLine &cmd, const QByteArray &stdInData)
{
    Q_ASSERT(QThread::currentThread() != &m_thread);

    return run(cmd, stdInData);
}

DeviceShell::State DeviceShell::state() const { return m_shellScriptState; }

QStringList DeviceShell::missingFeatures() const { return m_missingFeatures; }

RunResult DeviceShell::run(const CommandLine &cmd, const QByteArray &stdInData)
{
    // If the script failed to install, use Process directly instead.
    bool useProcess = m_shellScriptState == State::Failed;

    // Transferring large amounts of stdInData is slow via the shell script.
    // Use Process directly if the size exceeds 100kb.
    useProcess |= stdInData.size() > (1024 * 100);

    if (useProcess) {
        Process proc;
        const CommandLine fallbackCmd = createFallbackCommand(cmd);
        qCDebug(deviceShellLog) << "Running fallback:" << fallbackCmd;
        proc.setCommand(fallbackCmd);
        proc.setWriteData(stdInData);
        proc.runBlocking();

        return RunResult{
            proc.exitCode(),
            proc.readAllRawStandardOutput(),
            proc.readAllRawStandardError()
        };
    }

    const RunResult errorResult{-1, {}, {}};
    QTC_ASSERT(m_shellProcess, return errorResult);
    QTC_ASSERT(m_shellProcess->isRunning(), return errorResult);
    QTC_ASSERT(m_shellScriptState == State::Succeeded, return errorResult);

    QMutexLocker lk(&m_commandMutex);

    QWaitCondition waiter;
    const int id = ++m_currentId;
    m_commandOutput.insert(id, CommandRun{{-1, {}, {}}, &waiter});

    QMetaObject::invokeMethod(m_shellProcess.get(), [this, id, cmd, stdInData] {
        const QString command = QString("%1 \"%2\" %3\n").arg(id)
                                .arg(QString::fromLatin1(stdInData.toBase64()), cmd.toUserOutput());
        qCDebug(deviceShellLog) << "Running via shell:" << command;
        m_shellProcess->writeRaw(command.toUtf8());
    });

    waiter.wait(&m_commandMutex);

    const auto it = m_commandOutput.constFind(id);
    const RunResult result = *it;
    m_commandOutput.erase(it);

    return result;
}

void DeviceShell::close()
{
    QTC_ASSERT(QThread::currentThread() == thread(), return );
    QTC_ASSERT(m_thread.isRunning(), return );

    m_thread.quit();
    m_thread.wait();
}

/*!
 * \brief DeviceShell::setupShellProcess
 *
 * Override this function to setup the shell process.
 * The default implementation just sets the command line to "bash"
 */
void DeviceShell::setupShellProcess(Process *shellProcess)
{
    shellProcess->setCommand(CommandLine{"bash"});
}

/*!
* \brief DeviceShell::createFallbackCommand
* \param cmd The command to run
* \return The command to run in case the shell script is not available
*
* Creates a command to run in case the shell script is not available
*/
CommandLine DeviceShell::createFallbackCommand(const CommandLine &cmd)
{
    return cmd;
}

/*!
 * \brief DeviceShell::start
 * \return Returns true if starting the Shell process succeeded
 *
 * \note You have to call this function when deriving from DeviceShell. Current implementations call the function from their constructor.
 */
expected_str<void> DeviceShell::start()
{
    m_shellProcess = std::make_unique<Process>();
    connect(m_shellProcess.get(), &Process::done, m_shellProcess.get(),
            [this] { emit done(m_shellProcess->resultData()); });
    connect(&m_thread, &QThread::finished, m_shellProcess.get(), [this] { closeShellProcess(); }, Qt::DirectConnection);

    setupShellProcess(m_shellProcess.get());

    CommandLine cmdLine = m_shellProcess->commandLine();

    m_shellProcess->setProcessMode(ProcessMode::Writer);

    // Moving the process into its own thread ...
    m_shellProcess->moveToThread(&m_thread);

    expected_str<void> result;
    QMetaObject::invokeMethod(
        m_shellProcess.get(),
        [this]() -> expected_str<void> {
            qCDebug(deviceShellLog)
                << "Starting shell process:" << m_shellProcess->commandLine().toUserOutput();
            m_shellProcess->start();

            if (!m_shellProcess->waitForStarted()) {
                closeShellProcess();
                return make_unexpected(Tr::tr("The process failed to start."));
            }

            auto installResult = installShellScript();
            if (installResult) {
                connect(m_shellProcess.get(),
                        &Process::readyReadStandardOutput,
                        m_shellProcess.get(),
                        [this] { onReadyRead(); });
                connect(m_shellProcess.get(),
                        &Process::readyReadStandardError,
                        m_shellProcess.get(),
                        [this] {
                            const QByteArray stdErr = m_shellProcess->readAllRawStandardError();
                            qCWarning(deviceShellLog)
                                << "Received unexpected output on stderr:" << stdErr;
                        });

                connect(m_shellProcess.get(), &Process::done, m_shellProcess.get(), [this] {
                    if (m_shellProcess->resultData().m_exitCode != EXIT_SUCCESS
                        || m_shellProcess->resultData().m_exitStatus != QProcess::NormalExit) {
                        qCWarning(deviceShellLog) << "Shell exited with error code:"
                                                  << m_shellProcess->resultData().m_exitCode << "("
                                                  << m_shellProcess->exitMessage() << ")";
                    }
                });

                return {};
            } else if (m_shellProcess->isRunning()) {
                m_shellProcess->kill();
            }
            const QString stdErr = m_shellProcess->readAllStandardError();
            m_shellProcess.reset();

            return make_unexpected(Tr::tr("Failed to install shell script: %1\n%2")
                                       .arg(installResult.error())
                                       .arg(stdErr));
        },
        Qt::BlockingQueuedConnection,
        &result);

    return result;
}

expected_str<QByteArray> DeviceShell::checkCommand(const QByteArray &command)
{
    const QByteArray checkCmd = "(type " + command + " || echo '<missing>')\n";

    m_shellProcess->writeRaw(checkCmd);
    if (!m_shellProcess->waitForReadyRead()) {
        return make_unexpected(
            Tr::tr("Timeout while trying to check for %1.").arg(QString::fromUtf8(command)));
    }
    QByteArray out = m_shellProcess->readAllRawStandardOutput();
    if (out.contains("<missing>")) {
        m_shellScriptState = State::Failed;
        m_missingFeatures.append(QString::fromUtf8(command));
        return make_unexpected(
            Tr::tr("Command \"%1\" was not found.").arg(QString::fromUtf8(command)));
    }

    return out;
}

expected_str<void> DeviceShell::installShellScript()
{
    if (m_forceFailScriptInstallation) {
        m_shellScriptState = State::Failed;
        return make_unexpected(Tr::tr("Script installation was forced to fail."));
    }

    static const QList<QByteArray> requiredCommands
        = {"base64", "cat", "echo", "kill", "mkfifo", "mktemp", "rm"};

    for (const QByteArray &command : requiredCommands) {
        auto checkResult = checkCommand(command);
        if (!checkResult)
            return make_unexpected(checkResult.error());
    }

    const static QByteArray shellScriptBase64 = FilePath(":/utils/scripts/deviceshell.sh")
                                                    .fileContents()
                                                    .value()
                                                    .replace("\r\n", "\n")
                                                    .toBase64();
    const QByteArray scriptCmd = "(scriptData=$(echo " + shellScriptBase64
                                 + " | base64 -d 2>/dev/null ) && /bin/sh -c \"$scriptData\") || "
                                   "echo ERROR_INSTALL_SCRIPT >&2\n";

    qCDebug(deviceShellLog) << "Installing shell script:" << scriptCmd;
    m_shellProcess->writeRaw(scriptCmd);

    while (m_shellScriptState == State::Unknown) {
        if (!m_shellProcess->waitForReadyRead(5000)) {
            return make_unexpected(Tr::tr("Timeout while waiting for shell script installation."));
        }

        QByteArray out = m_shellProcess->readAllRawStandardError();
        if (out.contains("SCRIPT_INSTALLED") && !out.contains("ERROR_INSTALL_SCRIPT")) {
            m_shellScriptState = State::Succeeded;
            return {};
        }
        if (out.contains("ERROR_INSTALL_SCRIPT")) {
            m_shellScriptState = State::Failed;
            return make_unexpected(
                Tr::tr("Failed to install shell script: %1").arg(QString::fromUtf8(out)));
        }
        if (!out.isEmpty()) {
            qCWarning(deviceShellLog)
                << "Unexpected output while installing device shell script:" << out;
        }
    }

    return {};
}

void DeviceShell::closeShellProcess()
{
    if (m_shellProcess) {
        if (m_shellProcess->isRunning()) {
            m_shellProcess->write("exit\nexit\n");
            if (!m_shellProcess->waitForFinished(2000))
                m_shellProcess->terminate();
        }
        m_shellProcess.reset();
    }
}

QByteArray::const_iterator next(const QByteArray::const_iterator &bufferEnd,
                                const QByteArray::const_iterator &itCurrent)
{
    for (QByteArray::const_iterator it = itCurrent; it != bufferEnd; ++it) {
        if (*it == '\n')
            return it;
    }
    return bufferEnd;
}

QByteArray byteArrayFromRange(QByteArray::const_iterator itStart, QByteArray::const_iterator itEnd)
{
    return QByteArray(itStart, std::distance(itStart, itEnd));
}

QList<std::tuple<int, DeviceShell::ParseType, QByteArray>> parseShellOutput(const QByteArray &data)
{
    auto itStart = data.cbegin();
    const auto itEnd = data.cend();

    QList<std::tuple<int, DeviceShell::ParseType, QByteArray>> result;

    for (auto it = next(itEnd, itStart); it != itEnd; ++it, itStart = it, it = next(itEnd, it)) {
        const QByteArray lineView = byteArrayFromRange(itStart, it);
        QTC_ASSERT(lineView.size() > 0, continue);

        const auto pidEnd = lineView.indexOf(':');
        const auto typeEnd = lineView.indexOf(':', pidEnd + 1);

        QTC_ASSERT(pidEnd != -1 && typeEnd != -1, continue);

        bool ok = false;
        const QLatin1String sId(lineView.begin(), pidEnd);
        const int id = QString(sId).toInt(&ok);
        QTC_ASSERT(ok, continue);

        const QByteArray data = byteArrayFromRange(lineView.begin() + typeEnd + 1, lineView.end());
        const QByteArray decoded = QByteArray::fromBase64(data);

        DeviceShell::ParseType t;
        char type = lineView.at(typeEnd - 1);
        switch (type) {
        case 'O':
            t = DeviceShell::ParseType::StdOut;
            break;
        case 'E':
            t = DeviceShell::ParseType::StdErr;
            break;
        case 'R':
            t = DeviceShell::ParseType::ExitCode;
            break;
        default:
            QTC_CHECK(false);
            continue;
        }

        result.append(std::make_tuple(id, t, decoded));
    }

    return result;
}

/*!
 * \brief DeviceShell::onReadyRead
 *
 * Reads lines coming from the multiplex script.
 *
 * The format is: "<command-id>:<type>:base64-encoded-text-or-returnvalue"
 * The possible <type>'s are:
 * O for stdout
 * E for stderr
 * R for exit code
 *
 * Multiple O/E messages may be received for a process. Once
 * a single "R" is received, the exit code is reported back
 * and no further messages from that process are expected.
 */
void DeviceShell::onReadyRead()
{
    m_commandBuffer += m_shellProcess->readAllRawStandardOutput();
    const qsizetype lastLineEndIndex = m_commandBuffer.lastIndexOf('\n') + 1;

    if (lastLineEndIndex == 0)
        return;

    const QByteArray input(m_commandBuffer.cbegin(), lastLineEndIndex);

    const auto result = parseShellOutput(input);

    QMutexLocker lk(&m_commandMutex);
    for (const auto &line : result) {
        const auto &[cmdId, type, data] = line;

        const auto itCmd = m_commandOutput.find(cmdId);
        QTC_ASSERT(itCmd != m_commandOutput.end(), continue);

        switch (type) {
        case ParseType::StdOut:
            itCmd->stdOut.append(data);
            break;
        case ParseType::StdErr:
            itCmd->stdErr.append(data);
            break;
        case ParseType::ExitCode: {
            bool ok = false;
            int exitCode;
            exitCode = data.toInt(&ok);
            QTC_ASSERT(ok, exitCode = -1);
            itCmd->exitCode = exitCode;
            itCmd->waiter->wakeOne();
            break;
        }
        }
    };

    if (lastLineEndIndex == m_commandBuffer.size())
        m_commandBuffer.clear();
    else
        m_commandBuffer = m_commandBuffer.mid(lastLineEndIndex);
}

} // namespace Utils
