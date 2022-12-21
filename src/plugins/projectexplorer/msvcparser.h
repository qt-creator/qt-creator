// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"
#include "task.h"

#include <QRegularExpression>
#include <QString>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT MsvcParser :  public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    MsvcParser();

    static Utils::Id id();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    Result processCompileLine(const QString &line);

    QRegularExpression m_compileRegExp;
    QRegularExpression m_additionalInfoRegExp;

    Task m_lastTask;
    LinkSpecs m_linkSpecs;
    int m_lines = 0;
};

class PROJECTEXPLORER_EXPORT ClangClParser :  public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    ClangClParser();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    const QRegularExpression m_compileRegExp;
    Task m_lastTask;
    int m_linkedLines = 0;
};

} // namespace ProjectExplorer
