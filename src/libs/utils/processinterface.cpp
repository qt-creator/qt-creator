// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "processinterface.h"

#include "qtcassert.h"
#include "qtcprocess.h"

#include <QLoggingCategory>

Q_LOGGING_CATEGORY(wrappedProcessInterface, "qtc.wrappedprocessinterface", QtWarningMsg)

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
class WrappedProcessInterfacePrivate
{
public:
    Process m_process;
    bool m_hasReceivedFirstOutput = false;
    qint64 m_remotePID = 0;
    QString m_unexpectedStartupOutput;
    bool m_forwardStdout = false;
    bool m_forwardStderr = false;

    WrappedProcessInterface::WrapFunction m_wrapFunction;
    WrappedProcessInterface::ControlSignalFunction m_controlSignalFunction;
};

} // namespace Internal

WrappedProcessInterface::WrappedProcessInterface(
        const WrapFunction &wrapFunction, const ControlSignalFunction &controlSignalFunction)
    : d(std::make_unique<Internal::WrappedProcessInterfacePrivate>())
{
    d->m_wrapFunction = wrapFunction;
    d->m_controlSignalFunction = controlSignalFunction;

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
            emit readyRead(d->m_process.readAllRawStandardOutput(), {});
            return;
        }

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

        if (d->m_forwardStdout && rest.size() > 0) {
            fprintf(stdout, "%s", rest.constData());
            rest = QByteArrayView();
        }

        // In case we already received some error output, send it now.
        QByteArray stdErr = d->m_process.readAllRawStandardError();
        if (stdErr.size() > 0 && d->m_forwardStderr) {
            fprintf(stderr, "%s", stdErr.constData());
            stdErr.clear();
        }

        if (rest.size() > 0 || stdErr.size() > 0)
            emit readyRead(rest.toByteArray(), stdErr);
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

        ProcessResultData resultData = d->m_process.resultData();

        if (d->m_remotePID == 0 && !d->m_hasReceivedFirstOutput) {
            resultData.m_error = QProcess::FailedToStart;
            resultData.m_errorString = d->m_unexpectedStartupOutput;

            qCWarning(wrappedProcessInterface)
                << "Process failed to start:" << d->m_process.commandLine() << ":"
                << d->m_unexpectedStartupOutput;
            QByteArray stdOut = d->m_process.readAllRawStandardOutput();
            QByteArray stdErr = d->m_process.readAllRawStandardError();

            if (!stdOut.isEmpty())
                qCWarning(wrappedProcessInterface) << "stdout:" << stdOut;
            if (!stdErr.isEmpty())
                qCWarning(wrappedProcessInterface) << "stderr:" << stdErr;
        }

        emit done(resultData);
    });
}

WrappedProcessInterface::~WrappedProcessInterface()
{
    if (d->m_process.state() == QProcess::Running)
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
    d->m_process.setProcessChannelMode(QProcess::ProcessChannelMode::SeparateChannels);
    d->m_process.setExtraData(m_setup.m_extraData);
    d->m_process.setStandardInputFile(m_setup.m_standardInputFile);
    d->m_process.setAbortOnMetaChars(m_setup.m_abortOnMetaChars);
    d->m_process.setCreateConsoleOnWindows(m_setup.m_createConsoleOnWindows);
    if (m_setup.m_lowPriority)
        d->m_process.setLowPriority();

    d->m_forwardStdout = m_setup.m_processChannelMode == QProcess::ForwardedChannels
                         || m_setup.m_processChannelMode == QProcess::ForwardedOutputChannel;
    d->m_forwardStderr = m_setup.m_processChannelMode == QProcess::ForwardedChannels
                         || m_setup.m_processChannelMode == QProcess::ForwardedErrorChannel;

    const Result<CommandLine> fullCommandLine = d->m_wrapFunction(m_setup, "__qtc%1qtc__");

    if (!fullCommandLine) {
        emit done(ProcessResultData{
            -1,
            QProcess::ExitStatus::CrashExit,
            QProcess::ProcessError::FailedToStart,
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

} // Utils
