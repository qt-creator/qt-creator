// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gccparser.h"

#include <QRegularExpression>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT ClangParser : public ProjectExplorer::GccParser
{
    Q_OBJECT

public:
    ClangParser();

    static QList<Utils::OutputLineParser *> clangParserSuite();

    static Utils::Id id();

private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;

    QRegularExpression m_commandRegExp;
    QRegularExpression m_inLineRegExp;
    QRegularExpression m_messageRegExp;
    QRegularExpression m_summaryRegExp;
    QRegularExpression m_codesignRegExp;
    bool m_expectSnippet;
};

} // namespace ProjectExplorer
