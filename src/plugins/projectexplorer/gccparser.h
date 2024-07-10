// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"

#include <projectexplorer/task.h>

#include <QRegularExpression>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT GccParser : public OutputTaskParser
{
    Q_OBJECT

public:
    GccParser();

    static Utils::Id id();

    static QList<OutputLineParser *> gccParserSuite();

protected:
    void gccCreateOrAmendTask(
        Task::TaskType type,
        const QString &description,
        const QString &originalLine,
        bool forceAmend = false,
        const Utils::FilePath &file = {},
        int line = -1,
        int column = 0,
        const LinkSpecs &linkSpecs = {});

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    bool isContinuation(const QString &newLine) const override;

    QRegularExpression m_regExpIncluded;
    QRegularExpression m_regExpGccNames;
    QRegularExpression m_regExpCc1plus;
};

} // namespace ProjectExplorer
