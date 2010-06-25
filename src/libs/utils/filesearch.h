/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtCore/QStack>
#include <QtCore/QDir>
#include <QtGui/QTextDocument>

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileIterator
{
public:
    FileIterator();
    explicit FileIterator(const QStringList &fileList);
    ~FileIterator();

    virtual bool hasNext() const;
    virtual QString next();
    virtual int maxProgress() const;
    virtual int currentProgress() const;

private:
    QStringList m_list;
    QStringListIterator *m_iterator;
    int m_index;
};

class QTCREATOR_UTILS_EXPORT SubDirFileIterator : public FileIterator
{
public:
    SubDirFileIterator(const QStringList &directories, const QStringList &filters);

    bool hasNext() const;
    QString next();
    int maxProgress() const;
    int currentProgress() const;

private:
    QStringList m_filters;
    mutable QStack<QDir> m_dirs;
    mutable QStack<int> m_progressValues;
    mutable QStack<bool> m_processedValues;
    mutable int m_progress;
    mutable QStringList m_currentFiles;
};

class QTCREATOR_UTILS_EXPORT FileSearchResult
{
public:
    FileSearchResult() {}
    FileSearchResult(QString fileName, int lineNumber, QString matchingLine,
                     int matchStart, int matchLength,
                     QStringList regexpCapturedTexts)
            : fileName(fileName),
            lineNumber(lineNumber),
            matchingLine(matchingLine),
            matchStart(matchStart),
            matchLength(matchLength),
            regexpCapturedTexts(regexpCapturedTexts)
    {
    }
    QString fileName;
    int lineNumber;
    QString matchingLine;
    int matchStart;
    int matchLength;
    QStringList regexpCapturedTexts;
};

typedef QList<FileSearchResult> FileSearchResultList;

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResultList> findInFiles(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap = QMap<QString, QString>());

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResultList> findInFilesRegExp(const QString &searchTerm, FileIterator *files,
    QTextDocument::FindFlags flags, QMap<QString, QString> fileToContentsMap = QMap<QString, QString>());

QTCREATOR_UTILS_EXPORT QString expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts);

} // namespace Utils

#endif // FILESEARCH_H
