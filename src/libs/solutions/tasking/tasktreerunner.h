// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "tasking_global.h"

#include <QObject>

namespace Tasking {

enum class DoneWith;
class Group;
class TaskTree;

class TASKING_EXPORT TaskTreeRunner : public QObject
{
    Q_OBJECT

public:
    ~TaskTreeRunner();

    bool isRunning() const { return bool(m_taskTree); }

    // When task tree is running it resets the old task tree.
    void start(const Group &recipe);

    // TODO: rename to cancel(), also in TaskTree API, adapt docs.
    // When task tree is running it emits done(DoneWith::Cancel) synchronously.
    void stop();

    // No done() signal is emitted.
    void reset();

signals:
    void done(DoneWith result);

private:
    std::unique_ptr<TaskTree> m_taskTree;
};

} // namespace Tasking
