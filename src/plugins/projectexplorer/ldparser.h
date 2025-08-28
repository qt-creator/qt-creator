// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "ioutputparser.h"

#include <optional>

namespace ProjectExplorer::Internal {

class LdParser : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT

public:
    LdParser();
private:
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    bool isContinuation(const QString &line) const override;

    std::optional<Result> checkMainRegex(const QString &trimmedLine, const QString &originalLine);
    std::optional<Result> checkRanlib(const QString &trimmedLine, const QString &originalLine);
    Status getStatus(const QString &line);
};

#ifdef WITH_TESTS
QObject *createLdOutputParserTest();
#endif

} // namespace ProjectExplorer::Internal
