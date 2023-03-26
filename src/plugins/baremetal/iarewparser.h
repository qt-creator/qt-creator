// Copyright (C) 2019 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

namespace BareMetal::Internal {

class IarParser final : public ProjectExplorer::OutputTaskParser
{
public:
    explicit IarParser();
    static Utils::Id id();

private:
    void newTask(const ProjectExplorer::Task &task);
    void amendDescription();
    void amendFilePath();

    bool parseErrorOrFatalErrorDetailsMessage1(const QString &lne);
    bool parseErrorOrFatalErrorDetailsMessage2(const QString &lne);
    Result parseWarningOrErrorOrFatalErrorDetailsMessage1(const QString &lne);
    bool parseErrorInCommandLineMessage(const QString &lne);
    bool parseErrorMessage1(const QString &lne);

    Result handleLine(const QString &line, Utils::OutputFormat type) final;
    void flush() final;

    ProjectExplorer::Task m_lastTask;
    int m_lines = 0;
    bool m_expectSnippet = true;
    bool m_expectFilePath = false;
    bool m_expectDescription = false;
    QStringList m_snippets;
    QStringList m_filePathParts;
    QStringList m_descriptionParts;
};

} // BareMetal::Internal
