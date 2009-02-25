/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "splitter.h"

#include <QRegExp>

FileDataList splitDiffToFiles(const QByteArray &data)
{
    FileDataList ret;
    QString strData = data;
    QString splitExpression;

    if (data.contains("==== ") && data.contains(" ====\n")) {
        // Perforce diff
        splitExpression = "==== ([^\\n\\r]+) - ([^\\n\\r]+) ====";

    } else if (data.contains("--- ") && data.contains("\n+++ ")) {
        // Unified contextual diff
        splitExpression = "\\-\\-\\- ([^\\n\\r]*)"
                          "\\n\\+\\+\\+ ([^\\n\\r]*)";

    } else if (data.contains("*** ") && data.contains("\n--- ")) {
        // Copied contextual diff
        splitExpression = "\\*\\*\\* ([^\\n\\r]*) [0-9\\-]* [0-9:\\.]*[^\\n\\r]*"
                          "\\n\\-\\-\\- ([^\\n\\r]*) [0-9\\-]* [0-9:\\.]*[^\\n\\r]*";

    } else {
        ret.append(FileData("<not a diff>", data));
        return ret;
    }

    int splitIndex = 0, previousSplit = -1;
    QRegExp splitExpr(splitExpression);
    QString filename, content;
    // The algorithm works like this:
    // On the first match we only get the filename of the first patch part
    // On the second match (if any) we get the diff content, and the name of the next file patch
    
    while (-1 != (splitIndex = splitExpr.indexIn(strData,splitIndex))) {
        if (!filename.isEmpty()) {
            QString content = strData.mid(previousSplit, splitIndex - previousSplit);
            ret.append(FileData(filename, content.toLatin1()));
        }

        // If the first index in not at the beginning of the file, then we know there's content
        // we're about to skip, which is common in commit diffs, so we get that content and give it
        // a 'fake' filename.
        if (previousSplit == -1 && splitIndex > 0 && filename.isEmpty()) {
            QString content = strData.left(splitIndex);
            ret.append(FileData("<Header information>", content.toLatin1()));
        }

        filename = splitExpr.cap(1);
        previousSplit = splitIndex;
        ++splitIndex;
    }
    // Append the last patch content
    if (!filename.isEmpty()) {
        QString content = strData.mid(previousSplit);
        ret.append(FileData(filename, content.toLatin1()));
    }

    return ret;
}
