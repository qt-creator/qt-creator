// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace QtSupport {
namespace Internal {

class QtTestParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT
private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override { emitCurrentTask(); }

    void emitCurrentTask();

    ProjectExplorer::Task m_currentTask;
};

} // namespace Internal
} // namespace QtSupport
