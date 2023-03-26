// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "splitter.h"

#include <QRegularExpression>

FileDataList splitDiffToFiles(const QString &strData)
{
    FileDataList ret;
    QString splitExpression;

    if (strData.contains(QLatin1String("==== ")) && strData.contains(QLatin1String(" ====\n"))) {
        // Perforce diff
        splitExpression = QLatin1String("==== ([^\\n\\r]+) - ([^\\n\\r]+) ====");

    } else if (strData.contains(QLatin1String("--- ")) && strData.contains(QLatin1String("\n+++ "))) {
        // Unified contextual diff
        splitExpression = QLatin1String("\\-\\-\\- ([^\\n\\r]*)"
                          "\\n\\+\\+\\+ ([^\\n\\r]*)");

    } else if (strData.contains(QLatin1String("*** ")) && strData.contains(QLatin1String("\n--- "))) {
        // Copied contextual diff
        splitExpression = QLatin1String("\\*\\*\\* ([^\\n\\r]*) [0-9\\-]* [0-9:\\.]*[^\\n\\r]*"
                          "\\n\\-\\-\\- ([^\\n\\r]*) [0-9\\-]* [0-9:\\.]*[^\\n\\r]*");

    } else {
        return FileDataList();
    }

    int splitIndex = 0, previousSplit = -1;
    const QRegularExpression splitExpr(splitExpression);
    QString filename;
    // The algorithm works like this:
    // On the first match we only get the filename of the first patch part
    // On the second match (if any) we get the diff content, and the name of the next file patch

    QRegularExpressionMatch match;
    while ((match = splitExpr.match(strData, splitIndex)).hasMatch()) {
        splitIndex = match.capturedStart();
        if (!filename.isEmpty()) {
            QString content = strData.mid(previousSplit, splitIndex - previousSplit);
            ret.append(FileData(filename, content));
        }

        // If the first index in not at the beginning of the file, then we know there's content
        // we're about to skip, which is common in commit diffs, so we get that content and give it
        // a 'fake' filename.
        if (previousSplit == -1 && splitIndex > 0 && filename.isEmpty()) {
            QString content = strData.left(splitIndex);
            ret.append(FileData(QLatin1String("<Header information>"), content));
        }

        filename = match.captured(1);
        previousSplit = splitIndex;
        ++splitIndex;
    }
    // Append the last patch content
    if (!filename.isEmpty()) {
        const QString content = strData.mid(previousSplit);
        ret.append(FileData(filename, content));
    }

    return ret;
}
