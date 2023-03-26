// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "ninjaparser.h"

#include <utils/fileutils.h>

namespace MesonProjectManager {
namespace Internal {

NinjaParser::NinjaParser() {}

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
            //return ProjectExplorer::OutputTaskParser::Status::InProgress;
        }
    }
    return ProjectExplorer::OutputTaskParser::Status::NotHandled;
}

bool NinjaParser::hasFatalErrors() const
{
    // TODO
    return false;
}

} // namespace Internal
} // namespace MesonProjectManager
