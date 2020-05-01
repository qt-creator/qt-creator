/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "ninjaparser.h"
#include <utils/fileutils.h>
namespace MesonProjectManager {
namespace Internal {
NinjaParser::NinjaParser() {}

Utils::optional<int> NinjaParser::extractProgress(const QString &line)
{
    auto progress = m_progressRegex.match(line);
    if (progress.hasMatch()) {
        auto total = progress.captured(2).toInt();
        auto pos = progress.captured(1).toInt();
        return pos * 100 / total;
    }
    return Utils::nullopt;
}

void NinjaParser::setSourceDirectory(const Utils::FilePath &sourceDir)
{
    emit addSearchDir(sourceDir);
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
