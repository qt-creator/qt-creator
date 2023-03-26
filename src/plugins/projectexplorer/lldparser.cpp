// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "lldparser.h"

#include "projectexplorerconstants.h"
#include "task.h"

#include <utils/fileutils.h>

#include <QStringList>

namespace ProjectExplorer {
namespace Internal {

Utils::OutputLineParser::Result LldParser::handleLine(const QString &line, Utils::OutputFormat type)
{
    if (type != Utils::StdErrFormat)
        return Status::NotHandled;

    const QString trimmedLine = rightTrimmed(line);
    if (trimmedLine.contains("error:") && trimmedLine.contains("lld")) {
        scheduleTask(CompileTask(Task::Error, trimmedLine), 1);
        return Status::Done;
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
        const auto file = absoluteFilePath(Utils::FilePath::fromUserInput(
                trimmedLine.mid(filePathOffset, filePathLen).trimmed()));
        LinkSpecs linkSpecs;
        addLinkSpecForAbsoluteFilePath(linkSpecs, file, lineNo, filePathOffset, filePathLen);
        scheduleTask(CompileTask(Task::Unknown, trimmedLine.mid(4).trimmed(),
                                 file, lineNo), 1);
        return {Status::Done, linkSpecs};
    }
    return Status::NotHandled;
}

} // namespace Internal
} // namespace ProjectExplorer
