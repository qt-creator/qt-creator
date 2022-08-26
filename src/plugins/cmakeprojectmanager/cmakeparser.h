// Copyright (C) 2016 Axonian LLC.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "cmake_global.h"

#include <projectexplorer/ioutputparser.h>
#include <projectexplorer/task.h>

#include <QDir>
#include <QRegularExpression>

#include <optional>

namespace CMakeProjectManager {

class CMAKE_EXPORT CMakeParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    explicit CMakeParser();
    void setSourceDirectory(const QString &sourceDir);

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    enum TripleLineError { NONE, LINE_LOCATION, LINE_DESCRIPTION, LINE_DESCRIPTION2 };

    TripleLineError m_expectTripleLineErrorData = NONE;

    std::optional<QDir> m_sourceDirectory;
    ProjectExplorer::Task m_lastTask;
    QRegularExpression m_commonError;
    QRegularExpression m_nextSubError;
    QRegularExpression m_commonWarning;
    QRegularExpression m_locationLine;
    bool m_skippedFirstEmptyLine = false;
    int m_lines = 0;
};

} // namespace CMakeProjectManager
