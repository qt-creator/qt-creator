/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "deviceshell.h"

#include "processinterface.h"
#include "qtcassert.h"
#include "qtcprocess.h"

#include <QLoggingCategory>
#include <QScopeGuard>

Q_LOGGING_CATEGORY(deviceShellLog, "qtc.utils.deviceshell", QtWarningMsg)

namespace Utils {

namespace {

/*!
 * The multiplex script waits for input via stdin.
 *
 * To start a command, a message is send with the format "<cmd-id> "<base64-encoded-stdin-data>" <commandline>\n"
 * To stop the script, simply send "exit\n" via stdin
 *
 * Once a message is received, two new streams are created that the new process redirects its output to ( $stdoutraw and $stderrraw ).
 *
 * These streams are piped through base64 into the two streams stdoutenc and stderrenc.
 *
 * Two subshells read from these base64 encoded streams, and prepend the command-id, as well as either "O:" or "E:" depending on whether its the stdout or stderr stream.
 *
 * Once the process exits its exit code is send to stdout with the command-id and the type "R".
 *
 */
const QLatin1String r_execScript = QLatin1String(R"SCRIPT(
#!/bin/sh
FINAL_OUT=$(mktemp -u)
mkfifo "$FINAL_OUT"

finalOutput() {
    local fileInputBuffer
    while read fileInputBuffer
    do
        if test -f "$fileInputBuffer.err"; then
            cat $fileInputBuffer.err
        fi
        cat $fileInputBuffer
        rm -f $fileInputBuffer.err $fileInputBuffer
    done
}

finalOutput < $FINAL_OUT &

readAndMark() {
    local buffer
    while read buffer
    do
        printf '%s:%s:%s\n' "$1" "$2" "$buffer"
    done
}

base64decode()
{
    base64 -d 2>/dev/null
}

base64encode()
{
    base64 2>/dev/null
}

executeAndMark()
{
    PID="$1"
    INDATA="$2"
    shift
    shift
    CMD="$@"

    # LogFile
    TMPFILE=$(mktemp)

    # Output Streams
    stdoutenc=$(mktemp -u)
    stderrenc=$(mktemp -u)
    mkfifo "$stdoutenc" "$stderrenc"

    # app output streams
    stdoutraw=$(mktemp -u)
    stderrraw=$(mktemp -u)
    mkfifo "$stdoutraw" "$stderrraw"

    # Cleanup
    trap 'rm -f "$stdoutenc" "$stderrenc" "$stdoutraw" "$stderrraw"' EXIT

    # Pipe all app output through base64, and then into the output streams
    cat $stdoutraw | base64encode > "$stdoutenc" &
    cat $stderrraw | base64encode > "$stderrenc" &

    # Mark the app's output streams
    readAndMark $PID 'O' < "$stdoutenc" >> $TMPFILE &
    readAndMark $PID 'E' < "$stderrenc" >> $TMPFILE.err &

    # Start the app ...
    if [ -z "$INDATA" ]
    then
        eval $CMD 1> "$stdoutraw" 2> "$stderrraw"
    else
        echo $INDATA | base64decode | eval "$CMD" 1> "$stdoutraw" 2> "$stderrraw"
    fi

    exitcode=$(echo $? | base64encode)
    wait
    echo "$PID:R:$exitcode" >> $TMPFILE
    echo $TMPFILE
}

execute()
{
    PID="$1"

    if [ "$#" -lt "3" ]; then
        TMPFILE=$(mktemp)
        echo "$PID:R:MjU1Cg==" > $TMPFILE
        echo $TMPFILE
    else
        INDATA=$(eval echo "$2")
        shift
        shift
        CMD=$@
        executeAndMark $PID "$INDATA" "$CMD"
    fi
}

cleanup()
{
    kill -- -$$
    exit 1
}

if [ -z "$(which base64)" ]
then
    echo "base64 command could not be found" >&2
    exit 1
fi

trap cleanup 1 2 3 6

echo SCRIPT_INSTALLED >&2

(while read -r id inData cmd; do
    if [ "$id" = "exit" ]; then
        exit
    fi
    execute $id $inData $cmd || echo "$id:R:255" &
done) > $FINAL_OUT
)SCRIPT");

} // namespace

DeviceShell::DeviceShell()
{
    m_thread.setObjectName("Shell Thread");
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
 * Runs the cmd inside the internal shell process and return whether it exited with EXIT_SUCCESS
 *
 * Will automatically defer to the internal thread
 */
bool DeviceShell::runInShell(const CommandLine &cmd, const QByteArray &stdInData)
{
    QTC_ASSERT(m_shellProcess, return false);
    Q_ASSERT(QThread::currentThread() != &m_thread);

    const RunResult result = run(cmd, stdInData);
    return result.exitCode == 0;
}

/*!
 * \brief DeviceShell::outputForRunInShell
 * \param cmd The command to run
 * \param stdInData Data to send to the stdin of the command
 * \return The stdout of the command
 *
 * Runs a command inside the running shell and returns the stdout that was generated by it.
 *
 * Will automatically defer to the internal thread
 */
DeviceShell::RunResult DeviceShell::outputForRunInShell(const CommandLine &cmd,
                                                        const QByteArray &stdInData)
{
    QTC_ASSERT(m_shellProcess, return {});
    Q_ASSERT(QThread::currentThread() != &m_thread);

    return run(cmd, stdInData);
}

DeviceShell::State DeviceShell::state() const { return m_shellScriptState; }

DeviceShell::RunResult DeviceShell::run(const CommandLine &cmd, const QByteArray &stdInData)
{
    const RunResult errorResult{-1, {}, {}};
    QTC_ASSERT(m_shellProcess, return errorResult);
    QTC_ASSERT(m_shellScriptState == State::Succeeded, return errorResult);

    QMutexLocker lk(&m_commandMutex);

    QWaitCondition waiter;
    const int id = ++m_currentId;
    const auto it = m_commandOutput.insert(id, CommandRun{{-1, {}, {}}, &waiter});

    QMetaObject::invokeMethod(m_shellProcess.get(), [this, id, cmd, stdInData]() {
        const QString command = QString("%1 \"%2\" %3\n")
                                    .arg(id)
                                    .arg(QString::fromLatin1(stdInData.toBase64()))
                                    .arg(cmd.toUserOutput());
        qCDebug(deviceShellLog) << "Running:" << command;
        m_shellProcess->writeRaw(command.toUtf8());
    });

    waiter.wait(&m_commandMutex);

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
void DeviceShell::setupShellProcess(QtcProcess *shellProcess)
{
    shellProcess->setCommand(CommandLine{"bash"});
}

/*!
 * \brief DeviceShell::startupFailed
 *
 * Override to display custom error messages
 */
void DeviceShell::startupFailed(const CommandLine &cmdLine)
{
    qCWarning(deviceShellLog) << "Failed to start shell via:" << cmdLine.toUserOutput();
}

/*!
 * \brief DeviceShell::start
 * \return Returns true if starting the Shell process succeeded
 *
 * \note You have to call this function when deriving from DeviceShell. Current implementations call the function from their constructor.
 */
bool DeviceShell::start()
{
    m_shellProcess = std::make_unique<QtcProcess>();
    connect(m_shellProcess.get(), &QtcProcess::done, m_shellProcess.get(),
            [this] { emit done(m_shellProcess->resultData()); });
    connect(&m_thread, &QThread::finished, m_shellProcess.get(), [this] { closeShellProcess(); }, Qt::DirectConnection);

    setupShellProcess(m_shellProcess.get());

    m_shellProcess->setProcessMode(ProcessMode::Writer);

    // Moving the process into its own thread ...
    m_shellProcess->moveToThread(&m_thread);

    bool result = false;
    QMetaObject::invokeMethod(
        m_shellProcess.get(),
        [this] {
            m_shellProcess->start();

            if (!m_shellProcess->waitForStarted()) {
                closeShellProcess();
                return false;
            }

            connect(m_shellProcess.get(), &QtcProcess::readyReadStandardOutput, m_shellProcess.get(), [this] {
                onReadyRead();
            });
            connect(m_shellProcess.get(), &QtcProcess::readyReadStandardError, m_shellProcess.get(), [this] {
                const QByteArray stdErr = m_shellProcess->readAllStandardError();

                if (m_shellScriptState == State::Unknown) {
                    if (stdErr.contains("SCRIPT_INSTALLED")) {
                        m_shellScriptState = State::Succeeded;
                        return;
                    }
                    if (stdErr.contains("ERROR_INSTALL_SCRIPT")) {
                        m_shellScriptState = State::FailedToStart;
                        qCWarning(deviceShellLog) << "Failed installing device shell script";
                        return;
                    }
                }

                qCWarning(deviceShellLog) << "Received unexpected output on stderr:" << stdErr;
            });

            connect(m_shellProcess.get(), &QtcProcess::done, m_shellProcess.get(), [this] {
                if (m_shellProcess->resultData().m_exitCode != EXIT_SUCCESS
                    || m_shellProcess->resultData().m_exitStatus != QProcess::NormalExit) {
                    qCWarning(deviceShellLog) << "Shell exited with error code:"
                                              << m_shellProcess->resultData().m_exitCode << "("
                                              << m_shellProcess->exitMessage() << ")";
                }
            });

            if (!installShellScript()) {
                closeShellProcess();
                return false;
            }

            return true;
        },
        Qt::BlockingQueuedConnection,
        &result);

    if (!result) {
        startupFailed(m_shellProcess->commandLine());
    }

    return result;
}

bool DeviceShell::installShellScript()
{
    const QByteArray runScriptCmd = "scriptData=$(echo "
            + QByteArray(r_execScript.begin(), r_execScript.size()).toBase64()
            + " | base64 -d 2>/dev/null ) && /bin/sh -c \"$scriptData\" || echo ERROR_INSTALL_SCRIPT >&2\n";

    qCDebug(deviceShellLog) << "Install shell script command:" << runScriptCmd;
    m_shellProcess->writeRaw(runScriptCmd);

    while (m_shellScriptState == State::Unknown) {
        if (!m_shellProcess->waitForReadyRead()) {
            qCWarning(deviceShellLog) << "Timeout while waiting for device shell script to install";
            m_shellScriptState = State::FailedToStart;
            return false;
        }
    }
    return m_shellScriptState == State::Succeeded;
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
    m_commandBuffer += m_shellProcess->readAllStandardOutput();
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
        case Utils::DeviceShell::ParseType::StdOut:
            itCmd->stdOut.append(data);
            break;
        case Utils::DeviceShell::ParseType::StdErr:
            itCmd->stdErr.append(data);
            break;
        case Utils::DeviceShell::ParseType::ExitCode: {
            bool ok = false;
            int exitCode;
            exitCode = QString::fromUtf8(data.begin(), data.size()).toInt(&ok);
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
