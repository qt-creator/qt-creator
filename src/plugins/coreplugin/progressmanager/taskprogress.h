// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include "futureprogress.h"

#include <QObject>

namespace Tasking { class TaskTree; }

namespace Core {

class TaskProgressPrivate;

class CORE_EXPORT TaskProgress : public QObject
{
    Q_OBJECT

public:
    TaskProgress(Tasking::TaskTree *taskTree); // Makes TaskProgress a child of task tree
    ~TaskProgress() override;

    void setId(Utils::Id id);
    void setAutoStopOnCancel(bool enable); // Default is true
    void setHalfLifeTimePerTask(int msecs); // Default is 1000 ms
    void setDisplayName(const QString &name);
    void setKeepOnFinish(FutureProgress::KeepOnFinishType keepType);
    void setSubtitleVisibleInStatusBar(bool visible);
    void setSubtitle(const QString &subtitle);

signals:
    void canceled();

private:
    std::unique_ptr<TaskProgressPrivate> d;
};

} // namespace Core
