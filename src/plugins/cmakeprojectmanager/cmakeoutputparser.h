// Copyright (C) 2016 Axonian LLC.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <utils/filepath.h>

#include <QRegularExpression>

namespace CMakeProjectManager::Internal {

class CMakeOutputParser : public ProjectExplorer::OutputTaskParser
{
public:
    explicit CMakeOutputParser();
    void setSourceDirectories(const Utils::FilePaths &sourceDirs);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    enum TripleLineError { NONE, LINE_LOCATION, LINE_DESCRIPTION, LINE_DESCRIPTION2 };

    TripleLineError m_expectTripleLineErrorData = NONE;

    Utils::FilePaths m_sourceDirectories;
    ProjectExplorer::Task m_lastTask;
    QRegularExpression m_commonError;
    QRegularExpression m_nextSubError;
    QRegularExpression m_commonWarning;
    QRegularExpression m_locationLine;
    QRegularExpression m_sourceLineAndFunction;
    bool m_skippedFirstEmptyLine = false;
    int m_lines = 0;

    struct CallStackLine
    {
        Utils::FilePath file;
        int line = -1;
        QString function;
    };
    QList<CallStackLine> m_callStack;
    bool m_callStackDetected = false;
    CallStackLine m_errorOrWarningLine;
};

class CMakeTask : public ProjectExplorer::BuildSystemTask
{
public:
    CMakeTask(TaskType type, const QString &description, const Utils::FilePath &file = {},
              int line = -1)
        : ProjectExplorer::BuildSystemTask(type, description, file, line)
    {
        setOrigin("CMake");
    }
};

#ifdef WITH_TESTS
QObject *createCMakeOutputParserTest();
#endif

} // namespace CMakeProjectManager::Internal
