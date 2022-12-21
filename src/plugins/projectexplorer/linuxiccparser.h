// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"
#include "task.h"

#include <QRegularExpression>

namespace ProjectExplorer {

class LinuxIccParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    LinuxIccParser();

    static Utils::Id id();

    static QList<Utils::OutputLineParser *> iccParserSuite();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void flush() override;

    QRegularExpression m_firstLine;
    QRegularExpression m_continuationLines;
    QRegularExpression m_caretLine;
    QRegularExpression m_pchInfoLine;

    bool m_expectFirstLine = true;
    Task m_temporary;
    int m_lines = 0;
};

} // namespace ProjectExplorer
