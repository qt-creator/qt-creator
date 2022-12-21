// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"

#include <QDir>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QTextDocument>

#include <functional>

QT_BEGIN_NAMESPACE
template <typename T>
class QFuture;
class QTextCodec;
QT_END_NAMESPACE

namespace Utils {

QTCREATOR_UTILS_EXPORT
std::function<bool(const FilePath &)> filterFileFunction(const QStringList &filterRegs,
                                                         const QStringList &exclusionRegs);

QTCREATOR_UTILS_EXPORT
std::function<FilePaths(const FilePaths &)> filterFilesFunction(const QStringList &filters,
                                                                const QStringList &exclusionFilters);

QTCREATOR_UTILS_EXPORT
QStringList splitFilterUiText(const QString &text);

QTCREATOR_UTILS_EXPORT
QString msgFilePatternLabel();

QTCREATOR_UTILS_EXPORT
QString msgExclusionPatternLabel();

QTCREATOR_UTILS_EXPORT
QString msgFilePatternToolTip();

class QTCREATOR_UTILS_EXPORT FileIterator
{
public:
    class Item
    {
    public:
        Item() = default;
        Item(const FilePath &path, QTextCodec *codec)
            : filePath(path)
            , encoding(codec)
        {}
        FilePath filePath;
        QTextCodec *encoding = nullptr;
    };

    using value_type = Item;

    class const_iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = Item;
        using difference_type = std::ptrdiff_t;
        using pointer = const value_type*;
        using reference = const value_type&;

        const_iterator() = default;
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

        const FileIterator *m_parent = nullptr;
        int m_index = -1; // -1 == end
    };

    virtual ~FileIterator() = default;
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
    explicit FileListIterator(const FilePaths &fileList, const QList<QTextCodec *> encodings);

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
    SubDirFileIterator(const FilePaths &directories,
                       const QStringList &filters,
                       const QStringList &exclusionFilters,
                       QTextCodec *encoding = nullptr);
    ~SubDirFileIterator() override;

    int maxProgress() const override;
    int currentProgress() const override;

protected:
    void update(int requestedIndex) override;
    int currentFileCount() const override;
    const Item &itemAt(int index) const override;

private:
    std::function<FilePaths(const FilePaths &)> m_filterFiles;
    QTextCodec *m_encoding;
    QStack<FilePath> m_dirs;
    QSet<FilePath> m_knownDirs;
    QStack<qreal> m_progressValues;
    QStack<bool> m_processedValues;
    qreal m_progress;
    // Use heap allocated objects directly because we want references to stay valid even after resize
    QList<Item *> m_items;
};

class QTCREATOR_UTILS_EXPORT FileSearchResult
{
public:
    FileSearchResult() = default;
    FileSearchResult(const FilePath &fileName, int lineNumber, const QString &matchingLine,
                     int matchStart, int matchLength,
                     const QStringList &regexpCapturedTexts)
            : fileName(fileName),
            lineNumber(lineNumber),
            matchingLine(matchingLine),
            matchStart(matchStart),
            matchLength(matchLength),
            regexpCapturedTexts(regexpCapturedTexts)
    {}

    bool operator==(const FileSearchResult &o) const
    {
        return fileName == o.fileName && lineNumber == o.lineNumber
               && matchingLine == o.matchingLine && matchStart == o.matchStart
               && matchLength == o.matchLength && regexpCapturedTexts == o.regexpCapturedTexts;
    }
    bool operator!=(const FileSearchResult &o) const { return !(*this == o); }

    FilePath fileName;
    int lineNumber;
    QString matchingLine;
    int matchStart;
    int matchLength;
    QStringList regexpCapturedTexts;
};

using FileSearchResultList = QList<FileSearchResult>;

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResultList> findInFiles(
    const QString &searchTerm,
    FileIterator *files,
    QTextDocument::FindFlags flags,
    const QMap<FilePath, QString> &fileToContentsMap = QMap<FilePath, QString>());

QTCREATOR_UTILS_EXPORT QFuture<FileSearchResultList> findInFilesRegExp(
    const QString &searchTerm,
    FileIterator *files,
    QTextDocument::FindFlags flags,
    const QMap<FilePath, QString> &fileToContentsMap = QMap<FilePath, QString>());

QTCREATOR_UTILS_EXPORT QString expandRegExpReplacement(const QString &replaceText, const QStringList &capturedTexts);
QTCREATOR_UTILS_EXPORT QString matchCaseReplacement(const QString &originalText, const QString &replaceText);

} // namespace Utils

Q_DECLARE_METATYPE(Utils::FileSearchResultList)
