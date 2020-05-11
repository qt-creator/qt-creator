/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

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
    void runPostPrintActions() override;

    class Private;
    Private * const d;
};

} // namespace ProjectExplorer
