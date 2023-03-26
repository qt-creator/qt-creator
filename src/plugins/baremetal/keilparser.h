// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace BareMetal::Internal {

class KeilParser final : public ProjectExplorer::OutputTaskParser
{
public:
    explicit KeilParser();
    static Utils::Id id();

private:
    void newTask(const ProjectExplorer::Task &task);

    // ARM compiler specific parsers.
    Result parseArmWarningOrErrorDetailsMessage(const QString &lne);
    bool parseArmErrorOrFatalErorrMessage(const QString &lne);

    // MCS51 compiler specific parsers.
    Result parseMcs51WarningOrErrorDetailsMessage1(const QString &lne);
    Result parseMcs51WarningOrErrorDetailsMessage2(const QString &lne);
    bool parseMcs51WarningOrFatalErrorMessage(const QString &lne);
    bool parseMcs51FatalErrorMessage2(const QString &lne);

    Result handleLine(const QString &line, Utils::OutputFormat type) final;
    void flush() final;

    ProjectExplorer::Task m_lastTask;
    int m_lines = 0;
    QStringList m_snippets;
};

} // BareMetal::Internal
