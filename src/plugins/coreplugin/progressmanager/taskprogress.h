// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

#include "futureprogress.h"

#include <QObject>

namespace Utils { class TaskTree; }

namespace Core {

class TaskProgressPrivate;

class CORE_EXPORT TaskProgress : public QObject
{
public:
    TaskProgress(Utils::TaskTree *taskTree); // Makes TaskProgress a child of task tree
    ~TaskProgress() override;

    void setHalfLifeTimePerTask(int msecs); // Default is 1000 ms
    void setDisplayName(const QString &name);
    void setKeepOnFinish(FutureProgress::KeepOnFinishType keepType);
    void setSubtitleVisibleInStatusBar(bool visible);
    void setSubtitle(const QString &subtitle);

private:
    std::unique_ptr<TaskProgressPrivate> d;
};

} // namespace Core
