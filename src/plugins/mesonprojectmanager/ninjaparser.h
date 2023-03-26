// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>

#include <QRegularExpression>

#include <optional>

namespace MesonProjectManager {
namespace Internal {

class NinjaParser final : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT
    QRegularExpression m_progressRegex{R"(^\[(\d+)/(\d+)\])"};
    std::optional<int> extractProgress(const QString &line);

public:
    NinjaParser();
    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void setSourceDirectory(const Utils::FilePath &sourceDir);

    bool hasDetectedRedirection() const override { return true; }
    bool hasFatalErrors() const override;
    Q_SIGNAL void reportProgress(int progress);
};

} // namespace Internal
} // namespace MesonProjectManager
