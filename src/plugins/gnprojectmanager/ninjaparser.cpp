// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ninjaparser.h"

#include <utils/fileutils.h>

namespace GNProjectManager::Internal {

std::optional<int> NinjaParser::extractProgress(const QString &line)
{
    auto progress = m_progressRegex.match(line);
    if (progress.hasMatch()) {
        auto total = progress.captured(2).toInt();
        auto pos = progress.captured(1).toInt();
        return pos * 100 / total;
    }
    return std::nullopt;
}

void NinjaParser::setSourceDirectory(const Utils::FilePath &sourceDir)
{
    emit newSearchDirFound(sourceDir);
}

Utils::OutputLineParser::Result NinjaParser::handleLine(const QString &line,
                                                        Utils::OutputFormat type)
{
    if (type == Utils::OutputFormat::StdOutFormat) {
        auto progress = extractProgress(line);
        if (progress) {
            emit reportProgress(*progress);
        }
    }
    return ProjectExplorer::OutputTaskParser::Status::NotHandled;
}

bool NinjaParser::hasFatalErrors() const
{
    return false;
}

} // namespace GNProjectManager::Internal
