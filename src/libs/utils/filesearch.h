// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "searchresultitem.h"

#include <QMap>
#include <QPromise>
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

enum FindFlag {
    FindBackward = 0x01,
    FindCaseSensitively = 0x02,
    FindWholeWords = 0x04,
    FindRegularExpression = 0x08,
    FindPreserveCase = 0x10
};
Q_DECLARE_FLAGS(FindFlags, FindFlag)

QTCREATOR_UTILS_EXPORT
QTextDocument::FindFlags textDocumentFlagsForFindFlags(FindFlags flags);

QTCREATOR_UTILS_EXPORT
void searchInContents(QPromise<SearchResultItems> &promise, const QString &searchTerm,
                      FindFlags flags, const FilePath &filePath, const QString &contents);

QTCREATOR_UTILS_EXPORT
std::function<FilePaths(const FilePaths &)> filterFilesFunction(const QStringList &filters,
                                                                const QStringList &exclusionFilters);

QTCREATOR_UTILS_EXPORT
QStringList splitFilterUiText(const QString &text);

QTCREATOR_UTILS_EXPORT
QString msgFilePatternLabel();

QTCREATOR_UTILS_EXPORT
QString msgExclusionPatternLabel();

enum class InclusionType {
    Included,
    Excluded
};

QTCREATOR_UTILS_EXPORT
QString msgFilePatternToolTip(InclusionType inclusionType = InclusionType::Included);

class FileContainer;

class QTCREATOR_UTILS_EXPORT FileContainerIterator
{
public:
    class Item
    {
    public:
        FilePath filePath {};
        QTextCodec *encoding = nullptr;
    };

    class Data;
    using Advancer = std::function<void(Data *)>;

    class Data
    {
    public:
        const FileContainer *m_container = nullptr;
        int m_progressValue = 0;
        Advancer m_advancer = {};
        int m_index = -1; // end iterator
        Item m_value = {};
    };

    using value_type = Item;
    using pointer = const value_type *;
    using reference = const value_type &;
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;

    FileContainerIterator() = default;

    reference operator*() const { return m_data.m_value; }
    pointer operator->() const { return &m_data.m_value; }
    void operator++();

    bool operator==(const FileContainerIterator &other) const {
        return m_data.m_container == other.m_data.m_container
               && m_data.m_index == other.m_data.m_index;
    }
    bool operator!=(const FileContainerIterator &other) const { return !operator==(other); }
    int progressValue() const { return m_data.m_progressValue; }
    int progressMaximum() const;

private:
    friend class FileContainer;
    FileContainerIterator(const Data &data) : m_data(data) {}
    Data m_data;
};

class QTCREATOR_UTILS_EXPORT FileContainer
{
public:
    using AdvancerProvider = std::function<FileContainerIterator::Advancer()>;

    FileContainer() = default;

    FileContainerIterator begin() const {
        if (!m_provider)
            return end();
        const FileContainerIterator::Advancer advancer = m_provider();
        if (!advancer)
            return end();
        FileContainerIterator iterator({this, 0, advancer});
        advancer(&iterator.m_data);
        return iterator;
    }
    FileContainerIterator end() const { return FileContainerIterator({this, m_progressMaximum}); }
    int progressMaximum() const { return m_progressMaximum; }

protected:
    FileContainer(const AdvancerProvider &provider, int progressMaximum)
        : m_provider(provider)
        , m_progressMaximum(progressMaximum) {}

private:
    friend class FileContainerIterator;
    AdvancerProvider m_provider;
    int m_progressMaximum = 0;
};

class QTCREATOR_UTILS_EXPORT FileListContainer : public FileContainer
{
public:
    FileListContainer(const FilePaths &fileList, const QList<QTextCodec *> &encodings);
};

class QTCREATOR_UTILS_EXPORT SubDirFileContainer : public FileContainer
{
public:
    SubDirFileContainer(const FilePaths &directories,
                        const QStringList &filters,
                        const QStringList &exclusionFilters,
                        QTextCodec *encoding = nullptr);
};

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
    explicit FileListIterator(const FilePaths &fileList = {},
                              const QList<QTextCodec *> &encodings = {});

    int maxProgress() const override;
    int currentProgress() const override;

protected:
    void update(int requestedIndex) override;
    int currentFileCount() const override;
    const Item &itemAt(int index) const override;

private:
    const QList<Item> m_items;
    int m_maxIndex = -1;
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

QTCREATOR_UTILS_EXPORT QFuture<SearchResultItems> findInFiles(const QString &searchTerm,
    const FileContainer &container, FindFlags flags, const QMap<FilePath, QString> &fileToContentsMap);

QTCREATOR_UTILS_EXPORT QFuture<SearchResultItems> findInFiles(const QString &searchTerm,
    FileIterator *files, FindFlags flags, const QMap<FilePath, QString> &fileToContentsMap);

QTCREATOR_UTILS_EXPORT QString expandRegExpReplacement(const QString &replaceText,
                                                       const QStringList &capturedTexts);
QTCREATOR_UTILS_EXPORT QString matchCaseReplacement(const QString &originalText,
                                                    const QString &replaceText);

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::FindFlags)
Q_DECLARE_METATYPE(Utils::FindFlags)
