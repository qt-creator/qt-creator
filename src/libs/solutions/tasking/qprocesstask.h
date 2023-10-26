// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include "tasktree.h"

#include <QProcess>

namespace Tasking {

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
