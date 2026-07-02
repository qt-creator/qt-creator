// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QProcess>

#include <functional>

namespace Utils {

// Process state / exit-status / error / channel-mode enums.
//
// These are Utils-owned scoped enums so they are available even where QProcess is
// compiled out (QT_FEATURE_process == -1, e.g. Qt for WebAssembly). Their underlying
// values match the corresponding QProcess enums, so conversion at the QProcess
// boundary is a checked cast (see toQProcess()/fromQProcess() below), mirroring how
// Utils::DirFilterFlag converts to QDir::Filters.
enum class ProcessState {
    NotRunning = 0,
    Starting = 1,
    Running = 2,
};

enum class ProcessExitStatus {
    NormalExit = 0,
    CrashExit = 1,
};

enum class ProcessError {
    FailedToStart = 0,
    Crashed = 1,
    Timedout = 2,
    ReadError = 3,
    WriteError = 4,
    UnknownError = 5,
};

enum class ProcessChannelMode {
    SeparateChannels = 0,
    MergedChannels = 1,
    ForwardedChannels = 2,
    ForwardedOutputChannel = 3,
    ForwardedErrorChannel = 4,
};

#if QT_CONFIG(process)

// Conversions across the QProcess boundary. The enum values above are kept in sync
// with QProcess, so each conversion is a plain checked cast.
inline QProcess::ProcessState toQProcess(ProcessState v) { return QProcess::ProcessState(int(v)); }
inline QProcess::ExitStatus toQProcess(ProcessExitStatus v) { return QProcess::ExitStatus(int(v)); }
inline QProcess::ProcessError toQProcess(ProcessError v) { return QProcess::ProcessError(int(v)); }
inline QProcess::ProcessChannelMode toQProcess(ProcessChannelMode v)
{
    return QProcess::ProcessChannelMode(int(v));
}

inline ProcessState fromQProcess(QProcess::ProcessState v) { return ProcessState(int(v)); }
inline ProcessExitStatus fromQProcess(QProcess::ExitStatus v) { return ProcessExitStatus(int(v)); }
inline ProcessError fromQProcess(QProcess::ProcessError v) { return ProcessError(int(v)); }
inline ProcessChannelMode fromQProcess(QProcess::ProcessChannelMode v)
{
    return ProcessChannelMode(int(v));
}

#endif // QT_CONFIG(process)

enum class ProcessMode {
    Reader, // This opens in ReadOnly mode if no write data or in ReadWrite mode otherwise,
            // closes the write channel afterwards.
    Writer  // This opens in ReadWrite mode and doesn't close the write channel
};

enum class TerminalMode {
    Off,
    Run,      // Start with process stub enabled
    Debug,    // Start with process stub enabled and wait for debugger to attach
    Detached, // Start in a terminal, without process stub.
};

// Miscellaneous, not process core

enum class Channel {
    Output,
    Error
};

enum class DetachedChannelMode {
    Forward,
    Discard
};

enum class TextChannelMode {
                // Keep | Emit | Emit
                //  raw | text | content
                // data |  sig |
                // -----+------+--------
    Off,        //  yes |   no | -
    SingleLine, //   no |  yes | Single lines
    MultiLine   //  yes |  yes | All the available data
};

enum class ProcessResult {
    // Finished successfully. Unless an ExitCodeInterpreter is set
    // this corresponds to a return code 0.
    FinishedWithSuccess,
    // Finished unsuccessfully. Unless an ExitCodeInterpreter is set
    // this corresponds to a return code different from 0.
    FinishedWithError,
    // Process terminated abnormally (crash)
    TerminatedAbnormally,
    // Executable could not be started
    StartFailed,
    // Canceled due to a call to terminate() or kill(),
    // This includes a call to stop() or timeout has triggered for runBlocking().
    Canceled
};

using TextChannelCallback = std::function<void(const QString & /*text*/)>;

} // namespace Utils

Q_DECLARE_METATYPE(Utils::ProcessMode);
