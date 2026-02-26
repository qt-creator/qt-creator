// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/ioutputparser.h>

#include <QRegularExpression>

#include <optional>

namespace GNProjectManager::Internal {

class NinjaParser final : public ProjectExplorer::OutputTaskParser
{
    Q_OBJECT
    QRegularExpression m_progressRegex{R"(^\[(\d+)/(\d+)\])"};
    std::optional<int> extractProgress(const QString &line);

public:
    NinjaParser() = default;

    Result handleLine(const QString &line, Utils::OutputFormat type) override;
    void setSourceDirectory(const Utils::FilePath &sourceDir);

    bool hasDetectedRedirection() const override { return true; }
    bool hasFatalErrors() const override;
    Q_SIGNAL void reportProgress(int progress);
};

} // namespace GNProjectManager::Internal
