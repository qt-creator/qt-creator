/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef FILESEARCH_H
#define FILESEARCH_H

#include "utils_global.h"

#include <QtCore/QStringList>
#include <QtCore/QFuture>
#include <QtCore/QMap>
#include <QtGui/QTextDocument>

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileSearchResult
{
public:
    FileSearchResult() {}
    FileSearchResult(QString fileName, int lineNumber, QString matchingLine, int matchStart, int matchLength)
            : fileName(fileName), lineNumber(lineNumber), matchingLine(matchingLine), matchStart(matchStart), matchLength(matchLength)
    {
    }
    QString fileName;
    int lineNumber;
    QString matchingLine;
    int matchStart;
    int matchLength;
};

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResult> findInFiles(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap = QMap<QString, QString>());

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResult> findInFilesRegExp(const QString &searchTerm, const QStringList &files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap = QMap<QString, QString>());

} // namespace Utils

#endif // FILESEARCH_H
