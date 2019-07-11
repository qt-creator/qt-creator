/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "lldparser.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/fileutils.h>

#include <QStringList>

namespace ProjectExplorer {
namespace Internal {

void LldParser::stdError(const QString &line)
{
    const QString trimmedLine = rightTrimmed(line);
    if (trimmedLine.contains("error:") && trimmedLine.contains("lld")) {
        emit addTask({Task::Error, trimmedLine, Utils::FilePath(), -1,
                      Constants::TASK_CATEGORY_COMPILE});
        return;
    }
    static const QStringList prefixes{">>> referenced by ", ">>> defined at ", ">>> "};
    for (const QString &prefix : prefixes) {
        if (!trimmedLine.startsWith(prefix))
            continue;
        int lineNo = -1;
        const int locOffset = trimmedLine.lastIndexOf(':');
        if (locOffset != -1) {
            const int endLocOffset = trimmedLine.indexOf(')', locOffset);
            const int numberWidth = endLocOffset == -1 ? -1 : endLocOffset - locOffset - 1;
            bool isNumber = true;
            lineNo = trimmedLine.mid(locOffset + 1, numberWidth).toInt(&isNumber);
            if (!isNumber)
                lineNo = -1;
        }
        int filePathOffset = trimmedLine.lastIndexOf('(', locOffset);
        if (filePathOffset != -1)
            ++filePathOffset;
        else
            filePathOffset = prefix.length();
        const int filePathLen = locOffset == -1 ? -1 : locOffset - filePathOffset;
        const auto file = Utils::FilePath::fromUserInput(
                    trimmedLine.mid(filePathOffset, filePathLen).trimmed());
        emit addTask({Task::Unknown, trimmedLine.mid(4).trimmed(), file, lineNo,
                      Constants::TASK_CATEGORY_COMPILE});
        return;
    }
    IOutputParser::stdError(line);
}

} // namespace Internal
} // namespace ProjectExplorer
