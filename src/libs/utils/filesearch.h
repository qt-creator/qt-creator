/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#ifndef FILESEARCH_H
#define FILESEARCH_H

#include "utils_global.h"

#include <QStringList>
#include <QFuture>
#include <QMap>
#include <QStack>
#include <QDir>
#include <QTextDocument>

QT_FORWARD_DECLARE_CLASS(QTextCodec)

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileIterator
{
public:
    FileIterator();
    explicit FileIterator(const QStringList &fileList,
                          const QList<QTextCodec *> encodings);
    virtual ~FileIterator();

    virtual bool hasNext() const;
    virtual QString next();
    virtual QTextCodec *encoding() const;
    virtual int maxProgress() const;
    virtual int currentProgress() const;

private:
    QStringList m_list;
    QStringListIterator *m_iterator;
    QList<QTextCodec *> m_encodings;
    int m_index;
};

class QTCREATOR_UTILS_EXPORT SubDirFileIterator : public FileIterator
{
public:
    SubDirFileIterator(const QStringList &directories, const QStringList &filters,
                       QTextCodec *encoding = 0);

    bool hasNext() const;
    QString next();
    QTextCodec *encoding() const;
    int maxProgress() const;
    int currentProgress() const;

private:
    QStringList m_filters;
    QTextCodec *m_encoding;
    mutable QStack<QDir> m_dirs;
    mutable QStack<qreal> m_progressValues;
    mutable QStack<bool> m_processedValues;
    mutable qreal m_progress;
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
