// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"

#include <projectexplorer/task.h>

#include <QRegularExpression>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GccParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    GccParser();

    static Utils::Id id();

    static QList<OutputLineParser *> gccParserSuite();

protected:
    void createOrAmendTask(
            Task::TaskType type,
            const QString &description,
            const QString &originalLine,
            bool forceAmend = false,
            const Utils::FilePath &file = {},
            int line = -1,
            int column = 0,
            const LinkSpecs &linkSpecs = {}
            );
    void flush() override;

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    bool isContinuation(const QString &newLine) const;

    QRegularExpression m_regExp;
    QRegularExpression m_regExpScope;
    QRegularExpression m_regExpIncluded;
    QRegularExpression m_regExpInlined;
    QRegularExpression m_regExpGccNames;
    QRegularExpression m_regExpCc1plus;

    Task m_currentTask;
    LinkSpecs m_linkSpecs;
    int m_lines = 0;
    bool m_requiredFromHereFound = false;
};

} // namespace ProjectExplorer
