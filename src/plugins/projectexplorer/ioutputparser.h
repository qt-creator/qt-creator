// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "buildstep.h"

#include <utils/outputformatter.h>

#include <functional>

namespace ProjectExplorer {
class Task;

class PROJECTEXPLORER_EXPORT OutputTaskParser : public Utils::OutputLineParser
{
    Q_OBJECT
public:
    OutputTaskParser();
    ~OutputTaskParser() override;

    class TaskInfo
    {
    public:
        TaskInfo(const Task &t, int l, int s) : task(t), linkedLines(l), skippedLines(s) {}
        Task task;
        int linkedLines = 0;
        int skippedLines = 0;
    };
    const QList<TaskInfo> taskInfo() const;

protected:
    void scheduleTask(const Task &task, int outputLines, int skippedLines = 0);
    void setDetailsFormat(Task &task, const LinkSpecs &linkSpecs = {});

private:
    void runPostPrintActions(QPlainTextEdit *edit) override;

    class Private;
    Private * const d;
};

} // namespace ProjectExplorer
