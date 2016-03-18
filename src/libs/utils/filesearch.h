/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#pragma once

#include "utils_global.h"

#include <QDir>
#include <QFuture>
#include <QMap>
#include <QStack>
#include <QTextDocument>

QT_FORWARD_DECLARE_CLASS(QTextCodec)

namespace Utils {

class QTCREATOR_UTILS_EXPORT FileIterator
{
public:
    class Item
    {
    public:
        Item() : encoding(nullptr) { }
        Item(const QString &path, QTextCodec *codec)
            : filePath(path), encoding(codec)
        {}
        QString filePath;
        QTextCodec *encoding;
    };

    typedef Item value_type;

    class const_iterator
    {
    public:
        typedef std::forward_iterator_tag iterator_category;
        typedef Item value_type;
        typedef std::ptrdiff_t difference_type;
        typedef const value_type *pointer;
        typedef const value_type &reference;

        const_iterator() : m_parent(nullptr), m_index(-1) { }
        const_iterator(const FileIterator *parent, int id)
            : m_parent(parent), m_index(id)
        {}
        const_iterator(const const_iterator &) = default;
        const_iterator &operator=(const const_iterator &) = default;

        reference operator*() const { return m_parent->itemAt(m_index); }
        pointer operator->() const { return &m_parent->itemAt(m_index); }
        void operator++() { m_parent->advance(this); }
        bool operator==(const const_iterator &other) const
        {
            return m_parent == other.m_parent && m_index == other.m_index;
        }
        bool operator!=(const const_iterator &other) const { return !operator==(other); }

        const FileIterator *m_parent;
        int m_index; // -1 == end
    };

    virtual ~FileIterator() {}
    const_iterator begin() const;
    const_iterator end() const;

    virtual int maxProgress() const = 0;
    virtual int currentProgress() const = 0;

    void advance(const_iterator *it) const;
    virtual const Item &itemAt(int index) const = 0;

protected:
    virtual void update(int requestedIndex) = 0;
    virtual int currentFileCount() const = 0;
};

class QTCREATOR_UTILS_EXPORT FileListIterator : public FileIterator
{
public:
    explicit FileListIterator(const QStringList &fileList,
                              const QList<QTextCodec *> encodings);

    int maxProgress() const override;
    int currentProgress() const override;

protected:
    void update(int requestedIndex) override;
    int currentFileCount() const override;
    const Item &itemAt(int index) const override;

private:
    QVector<Item> m_items;
    int m_maxIndex;
};

class QTCREATOR_UTILS_EXPORT SubDirFileIterator : public FileIterator
{
public:
    SubDirFileIterator(const QStringList &directories, const QStringList &filters,
                       QTextCodec *encoding = 0);
    ~SubDirFileIterator();

    int maxProgress() const override;
    int currentProgress() const override;

protected:
    void update(int requestedIndex) override;
    int currentFileCount() const override;
    const Item &itemAt(int index) const override;

private:
    QStringList m_filters;
    QTextCodec *m_encoding;
    QStack<QDir> m_dirs;
    QStack<qreal> m_progressValues;
    QStack<bool> m_processedValues;
    qreal m_progress;
    // Use heap allocated objects directly because we want references to stay valid even after resize
    QList<Item *> m_items;
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
QTCREATOR_UTILS_EXPORT QString matchCaseReplacement(const QString &originalText, const QString &replaceText);

} // namespace Utils
