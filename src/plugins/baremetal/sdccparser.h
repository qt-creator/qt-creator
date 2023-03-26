// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace BareMetal::Internal {

class SdccParser final : public ProjectExplorer::OutputTaskParser
{
public:
    explicit SdccParser();
    static Utils::Id id();

private:
    void newTask(const ProjectExplorer::Task &task);
    void amendDescription(const QString &desc);

    Result handleLine(const QString &line, Utils::OutputFormat type) final;
    void flush() final;

    ProjectExplorer::Task m_lastTask;
    int m_lines = 0;
};

} // BareMetal::Internal
