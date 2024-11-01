// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>

#include <functional>

namespace Utils {

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

enum class EventLoopMode {
    Off,
    On // Avoid
};

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
