// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../filepath.h"
#include "../stringutils.h"

#include <QFileInfo>
#include <QString>
#include <QtGlobal>

#include <QtCore/private/qabstractfileengine_p.h>

namespace Utils {
namespace Internal {

#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
inline std::pair<QDir::Filters, QDirIterator::IteratorFlags> convertQDirListingIteratorFlags(
    QDirListing::IteratorFlags flags)
{
    QDir::Filters filters = QDir::Files | QDir::Dirs | QDir::System | QDir::NoDotAndDotDot;
    QDirIterator::IteratorFlags iteratorFlags = QDirIterator::NoIteratorFlags;

    if (flags & QDirListing::IteratorFlag::ExcludeFiles)
        filters.setFlag(QDir::Files, false);
    if (flags & QDirListing::IteratorFlag::ExcludeDirs)
        filters.setFlag(QDir::Dirs, false);
    if (flags & QDirListing::IteratorFlag::ExcludeSpecial)
        filters.setFlag(QDir::System, false);
    if (flags & QDirListing::IteratorFlag::CaseSensitive)
        filters.setFlag(QDir::CaseSensitive, true);
    if (flags & QDirListing::IteratorFlag::IncludeHidden)
        filters.setFlag(QDir::Hidden, true);

    if (flags & QDirListing::IteratorFlag::IncludeDotAndDotDot) {
        filters.setFlag(QDir::NoDot, false);
        filters.setFlag(QDir::NoDotDot, false);
    }

    if (flags & QDirListing::IteratorFlag::Recursive)
        iteratorFlags.setFlag(QDirIterator::Subdirectories, true);
    if (flags & QDirListing::IteratorFlag::FollowDirSymlinks)
        iteratorFlags.setFlag(QDirIterator::FollowSymlinks, true);

    return {filters, iteratorFlags};
}
#endif

class DirIterator : public QAbstractFileEngineIterator
{
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    DirIterator(FilePaths paths,
                const QString &path,
                QDir::Filters filters,
                const QStringList &filterNames)
        : QAbstractFileEngineIterator(path, filters, filterNames)
#else
    DirIterator(FilePaths paths)
        : QAbstractFileEngineIterator({}, {})
#endif
        , m_filePaths(std::move(paths))
        , it(m_filePaths.begin())
    {}

    // QAbstractFileEngineIterator interface
public:
#if QT_VERSION >= QT_VERSION_CHECK(6, 8, 0)
    bool advance() override
    {
        if (!m_filePaths.empty() && m_filePaths.end() != it + 1) {
            ++it;
            return true;
        }
        return false;
    }
#else
    QString next() override
    {
        if (it == m_filePaths.end())
            return QString();
        const QString r = chopIfEndsWith(it->toFSPathString(), '/');
        ++it;
        return r;
    }

    bool hasNext() const override { return !m_filePaths.empty() && m_filePaths.end() != it + 1; }
#endif // QT_VERSION_CHECK(6, 8, 0)

    QString currentFileName() const override
    {
        const QString result = it->fileName();
        if (result.isEmpty() && !it->host().isEmpty()) {
            return it->host().toString();
        }
        return chopIfEndsWith(result, '/');
    }

    QFileInfo currentFileInfo() const override
    {
        return QFileInfo(chopIfEndsWith(it->toFSPathString(), '/'));
    }

private:
    const FilePaths m_filePaths;
    FilePaths::const_iterator it;
};

} // namespace Internal

} // namespace Utils
