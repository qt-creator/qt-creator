// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include "filepath.h"
#include "searchresultitem.h"

#include <QMap>
#include <QPromise>
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

QTCREATOR_UTILS_EXPORT QFuture<SearchResultItems> findInFiles(const QString &searchTerm,
    const FileContainer &container, FindFlags flags, const QMap<FilePath, QString> &fileToContentsMap);

QTCREATOR_UTILS_EXPORT QString expandRegExpReplacement(const QString &replaceText,
                                                       const QStringList &capturedTexts);
QTCREATOR_UTILS_EXPORT QString matchCaseReplacement(const QString &originalText,
                                                    const QString &replaceText);

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::FindFlags)
Q_DECLARE_METATYPE(Utils::FindFlags)
