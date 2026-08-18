// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinterface.h"

#include "qtcassert.h"
#include "qtcprocess.h"

#include <QLoggingCategory>
#include <QRandomGenerator>

static Q_LOGGING_CATEGORY(wrappedProcessInterface, "qtc.wrappedprocessinterface", QtWarningMsg)

namespace Utils {

namespace Pty {

void Data::resize(const QSize &size)
{
    m_size = size;
    if (m_data->m_handler)
        m_data->m_handler(size);
}

} // namespace Pty

/*!
 * \brief controlSignalToInt
 * \param controlSignal
 * \return Converts the ControlSignal enum to the corresponding unix signal
 */
int ProcessInterface::controlSignalToInt(ControlSignal controlSignal)
{
    switch (controlSignal) {
    case ControlSignal::Terminate: return 15;
    case ControlSignal::Kill:      return 9;
    case ControlSignal::Interrupt: return 2;
    case ControlSignal::KickOff:   return 19;
    case ControlSignal::CloseWriteChannel:
        QTC_CHECK(false);
        return 0;
    }
    return 0;
}

namespace Internal {
class WrappedProcessInterfacePrivate : public QObject
{
public:
    using QObject::QObject;

    Process m_process{this};
    bool m_hasReceivedFirstOutput = false;
    qint64 m_remotePID = 0;
    QString m_unexpectedStartupOutput;
    bool m_forwardStdout = false;
    bool m_forwardStderr = false;
    // The token the command echoes its status with, when it was asked to. It
    // carries a nonce and closes at both ends, in the same spirit as the PID
    // line, so that output which happens to look like it is still output.
    QByteArray m_exitCodePrefix;
    QByteArray m_exitCodeSuffix;
    QByteArray m_heldBack; // tail that may still hold a token split by a read

    // The tail that may yet turn out to be a token: a whole prefix, which the
    // digits and the closing part can still follow, or short of that the
    // longest end which is the beginning of one.
    qsizetype tokenTailSize() const
    {
        const qsizetype at = m_heldBack.lastIndexOf(m_exitCodePrefix);
        if (at >= 0)
            return m_heldBack.size() - at;
        for (qsizetype n = qMin(m_heldBack.size(), m_exitCodePrefix.size() - 1); n > 0; --n) {
            if (m_heldBack.endsWith(m_exitCodePrefix.left(n)))
                return n;
        }
        return 0;
    }

    // Only that tail is held back, so a token split between two reads is still
    // found whole while everything before it goes out at once.
    QByteArray filterOutput(const QByteArray &chunk)
    {
        if (m_exitCodePrefix.isEmpty())
            return chunk;
        m_heldBack += chunk;
        const qsizetype keep = tokenTailSize();
        const QByteArray ready = m_heldBack.left(m_heldBack.size() - keep);
        m_heldBack = m_heldBack.right(keep);
        return ready;
    }

    struct HeldBack
    {
        QByteArray output;
        std::optional<int> exitCode;
    };

    // Called when there is no more output. A token counts only whole: the
    // prefix, digits, and the closing suffix. Anything else is output.
    HeldBack takeHeldBack()
    {
        HeldBack result;
        if (m_exitCodePrefix.isEmpty()) {
            result.output = std::exchange(m_heldBack, {});
            return result;
        }
        const qsizetype at = m_heldBack.lastIndexOf(m_exitCodePrefix);
        const qsizetype digitsAt = at + m_exitCodePrefix.size();
        const qsizetype end = at < 0 ? -1 : m_heldBack.indexOf(m_exitCodeSuffix, digitsAt);
        if (end > 0) {
            const QByteArray digits = m_heldBack.mid(digitsAt, end - digitsAt);
            const bool isNumber = !digits.isEmpty()
                                  && std::all_of(digits.begin(), digits.end(),
                                                 [](char c) { return c >= '0' && c <= '9'; });
            if (isNumber) {
                result.exitCode = digits.toInt();
                const QByteArray all = std::exchange(m_heldBack, {});
                result.output = all.left(at) + all.mid(end + m_exitCodeSuffix.size());
                return result;
            }
        }
        result.output = std::exchange(m_heldBack, {});
        return result;
    }

    WrappedProcessInterface::WrapFunction m_wrapFunction;
    WrappedProcessInterface::ControlSignalFunction m_controlSignalFunction;
};

} // namespace Internal

WrappedProcessInterface::WrappedProcessInterface(
    const WrapFunction &wrapFunction, const ControlSignalFunction &controlSignalFunction,
    bool commandReportsExitCode)
    : d(new Internal::WrappedProcessInterfacePrivate(this))
{
    d->m_wrapFunction = wrapFunction;
    d->m_controlSignalFunction = controlSignalFunction;
    if (commandReportsExitCode) {
        // A nonce per launch: nothing the command writes can be guessed to
        // match it, which is what makes stripping the token safe.
        const QString nonce = QString::number(QRandomGenerator::global()->generate(), 16);
        d->m_exitCodePrefix = QString("__qtcstatus%1_").arg(nonce).toUtf8();
        d->m_exitCodeSuffix = "_statusqtc__";
    }

    d->m_process.setParent(this);

    connect(&d->m_process, &Process::started, this, [this] {
        qCDebug(wrappedProcessInterface) << "Process started:" << d->m_process.commandLine();

        if (m_setup.m_ptyData.has_value()) {
            d->m_hasReceivedFirstOutput = true;
            emit started(d->m_process.processId(), d->m_process.applicationMainThreadId());
        }
    });

    connect(&d->m_process, &Process::readyReadStandardOutput, this, [this] {
        if (d->m_hasReceivedFirstOutput) {
            const QByteArray output = d->filterOutput(d->m_process.readAllRawStandardOutput());
            qCDebug(wrappedProcessInterface) << "Received output:" << output;
            // With a marker a chunk can be held back entirely; without one this
            // is what it always was.
            if (d->m_exitCodePrefix.isEmpty() || !output.isEmpty())
                emit readyRead(output, {});
            return;
        }

        // The first line carries the PID and comes before anything the command
        // itself writes, so it is never held back.
        QByteArray output = d->m_process.readAllRawStandardOutput();
        QByteArrayView outputView(output);
        qsizetype idx = outputView.indexOf('\n');
        QByteArrayView firstLine = outputView.left(idx).trimmed();
        QByteArrayView rest = outputView.mid(idx + 1);

        qCDebug(wrappedProcessInterface)
            << "Process first line received:" << d->m_process.commandLine() << firstLine;

        if (!firstLine.startsWith("__qtc")) {
            d->m_unexpectedStartupOutput = QString::fromUtf8(firstLine);
            d->m_process.kill();
            return;
        }

        bool ok = false;
        d->m_remotePID = firstLine.mid(5, firstLine.size() - 5 - 5).toLongLong(&ok);

        if (ok)
            emit started(d->m_remotePID);
        else {
            d->m_unexpectedStartupOutput = QString::fromUtf8(firstLine);
            d->m_process.kill();
            return;
        }

        d->m_hasReceivedFirstOutput = true;

        QByteArray restOutput = d->filterOutput(rest.toByteArray());
        if (d->m_forwardStdout && restOutput.size() > 0) {
            fprintf(stdout, "%s", restOutput.constData());
            restOutput.clear();
        }

        // In case we already received some error output, send it now.
        QByteArray stdErr = d->m_process.readAllRawStandardError();
        if (stdErr.size() > 0 && d->m_forwardStderr) {
            fprintf(stderr, "%s", stdErr.constData());
            stdErr.clear();
        }

        if (restOutput.size() > 0 || stdErr.size() > 0)
            emit readyRead(restOutput, stdErr);
    });

    connect(&d->m_process, &Process::readyReadStandardError, this, [this] {
        if (!d->m_remotePID)
            return;

        if (d->m_forwardStderr) {
            fprintf(stderr, "%s", d->m_process.readAllRawStandardError().constData());
            return;
        }

        emit readyRead({}, d->m_process.readAllRawStandardError());
    });

    connect(&d->m_process, &Process::done, this, [this] {
        qCDebug(wrappedProcessInterface) << "Process exited:" << d->m_process.commandLine()
                                         << "with code:" << d->m_process.resultData().m_exitCode;

        // Whatever was held back is the token, the last of the output, or both.
        const Internal::WrappedProcessInterfacePrivate::HeldBack heldBack = d->takeHeldBack();
        if (!heldBack.output.isEmpty())
            emit readyRead(heldBack.output, {});

        ProcessResultData resultData = d->m_process.resultData();
        if (heldBack.exitCode)
            resultData.m_exitCode = *heldBack.exitCode;

        if (d->m_remotePID == 0 && !d->m_hasReceivedFirstOutput) {
            resultData.m_error = ProcessError::FailedToStart;

            const QByteArray stdOut = d->m_process.readAllRawStandardOutput();
            const QByteArray stdErr = d->m_process.readAllRawStandardError();

            QStringList details;
            if (!d->m_unexpectedStartupOutput.isEmpty())
                details.append(d->m_unexpectedStartupOutput);
            if (!stdOut.isEmpty())
                details.append(QString::fromUtf8(stdOut).trimmed());
            if (!stdErr.isEmpty())
                details.append(QString::fromUtf8(stdErr).trimmed());
            resultData.m_errorString = details.join('\n');

            qCWarning(wrappedProcessInterface)
                << "Process failed to start:" << d->m_process.commandLine() << ":"
                << resultData.m_errorString;
        }

        emit done(resultData);
    });
}

WrappedProcessInterface::~WrappedProcessInterface()
{
    if (d->m_process.state() == ProcessState::Running)
        sendControlSignal(ControlSignal::Kill);
}

void WrappedProcessInterface::start()
{
    d->m_process.setProcessMode(m_setup.m_processMode);
    d->m_process.setTerminalMode(m_setup.m_terminalMode);
    d->m_process.setPtyData(m_setup.m_ptyData);
    d->m_process.setReaperTimeout(m_setup.m_reaperTimeout);
    d->m_process.setWriteData(m_setup.m_writeData);
    // We need separate channels so we can intercept our Process ID markers.
    d->m_process.setProcessChannelMode(ProcessChannelMode::SeparateChannels);
    d->m_process.setExtraData(m_setup.m_extraData);
    d->m_process.setStandardInputFile(m_setup.m_standardInputFile);
    d->m_process.setAbortOnMetaChars(m_setup.m_abortOnMetaChars);
    d->m_process.setCreateConsoleOnWindows(m_setup.m_createConsoleOnWindows);
    if (m_setup.m_lowPriority)
        d->m_process.setLowPriority();

    d->m_forwardStdout = m_setup.m_processChannelMode == ProcessChannelMode::ForwardedChannels
                         || m_setup.m_processChannelMode == ProcessChannelMode::ForwardedOutputChannel;
    d->m_forwardStderr = m_setup.m_processChannelMode == ProcessChannelMode::ForwardedChannels
                         || m_setup.m_processChannelMode == ProcessChannelMode::ForwardedErrorChannel;

    const QString exitCodeTemplate = d->m_exitCodePrefix.isEmpty()
                                         ? QString()
                                         : QString::fromUtf8(d->m_exitCodePrefix) + "%1"
                                               + QString::fromUtf8(d->m_exitCodeSuffix);
    const Result<CommandLine> fullCommandLine
        = d->m_wrapFunction(m_setup, "__qtc%1qtc__", exitCodeTemplate);

    if (!fullCommandLine) {
        emit done(ProcessResultData{
            -1,
            ProcessExitStatus::CrashExit,
            ProcessError::FailedToStart,
            fullCommandLine.error(),
        });
        return;
    }

    d->m_process.setCommand(*fullCommandLine);
    d->m_process.start();
}

qint64 WrappedProcessInterface::write(const QByteArray &data)
{
    return d->m_process.writeRaw(data);
}

void WrappedProcessInterface::sendControlSignal(ControlSignal controlSignal)
{
    if (!m_setup.m_ptyData.has_value()) {
        QTC_ASSERT(d->m_remotePID, return);
        if (controlSignal == ControlSignal::CloseWriteChannel) {
            d->m_process.closeWriteChannel();
            return;
        }
        d->m_controlSignalFunction(controlSignal, d->m_remotePID);
    } else {
        // clang-format off
        switch (controlSignal) {
        case ControlSignal::Terminate: d->m_process.terminate();      break;
        case ControlSignal::Kill:      d->m_process.kill();           break;
        case ControlSignal::Interrupt: d->m_process.interrupt();      break;
        case ControlSignal::KickOff:   d->m_process.kickoffProcess(); break;
        case ControlSignal::CloseWriteChannel: break;
        }
        // clang-format on
    }
}

FilePath ProcessSetupData::fixedWorkingDirectory() const
{
    FilePath workingDir = m_workingDirectory;
    if (!workingDir.isDir())
        workingDir = workingDir.parentDir();
    if (!workingDir.isEmpty() && !QTC_GUARD(workingDir.exists()))
        workingDir = workingDir.withNewPath({});
    return workingDir;
}

} // Utils
