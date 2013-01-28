/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "splitter.h"

#include <QRegExp>

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
    QRegExp splitExpr(splitExpression);
    QString filename;
    // The algorithm works like this:
    // On the first match we only get the filename of the first patch part
    // On the second match (if any) we get the diff content, and the name of the next file patch

    while (-1 != (splitIndex = splitExpr.indexIn(strData,splitIndex))) {
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

        filename = splitExpr.cap(1);
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
