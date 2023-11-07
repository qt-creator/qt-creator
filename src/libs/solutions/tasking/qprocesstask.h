// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include "tasktree.h"

#include <QProcess>

namespace Tasking {

// Deleting a running QProcess may block the caller thread up to 30 seconds and issue warnings.
// To avoid these issues we move the running QProcess into a separate thread
// managed by the internal ProcessReaper, instead of deleting it immediately.
// Inside the ProcessReaper's thread we try to finish the process in a most gentle way:
// we call QProcess::terminate() with 500 ms timeout, and if the process is still running
// after the timeout passed, we call QProcess::kill() and wait for the process to finish.
// All these waitings are done is a separate thread, so the main thread doesn't block at all
// when the QProcessTask is destructed. Finally, on application quit, QProcessDeleter::deleteAll()
// should be called to in order to synchronize all the processes being reaped in a separate thread.
// The call to QProcessDeleter::deleteAll() is blocking, but it's unavoidable - sooner or later
// all the processes needs to finish ultimately, so better: block later!
// In this way we terminate running processes in the most safe way and keep the main thread
// responsive. That's a common case when the running QProcess needs to quit quite quicky,
// and the caller thread wants to forget about it, hoping it will be terminated in the most
// sensible way.

// The implementation of the internal reaper is inspired by the Utils::ProcessReaper taken
// from the QtCreator codebase.

class TASKING_EXPORT QProcessDeleter
{
public:
    // Blocking, should be called after all QProcessAdapter instances are deleted.
    static void deleteAll();
    void operator()(QProcess *process);
};

class TASKING_EXPORT QProcessAdapter : public TaskAdapter<QProcess, QProcessDeleter>
{
private:
    void start() {
        connect(task(), &QProcess::finished, this, [this] {
            const bool success = task()->exitStatus() == QProcess::NormalExit
                                 && task()->error() == QProcess::UnknownError
                                 && task()->exitCode() == 0;
            emit done(success);
        });
        connect(task(), &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error != QProcess::FailedToStart)
                return;
            emit done(false);
        });
        task()->start();
    }
};

using QProcessTask = CustomTask<QProcessAdapter>;

} // namespace Tasking
