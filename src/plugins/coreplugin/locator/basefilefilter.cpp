// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "basefilefilter.h"

#include <utils/algorithm.h>
#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>

using namespace Utils;

namespace Core {
namespace Internal {

class Data
{
public:
    void clear()
    {
        iterator.clear();
        previousResultPaths.clear();
        previousEntry.clear();
    }

    QSharedPointer<BaseFileFilter::Iterator> iterator;
    FilePaths previousResultPaths;
    bool forceNewSearchList;
    QString previousEntry;
};

class BaseFileFilterPrivate
{
public:
    Data m_data;
    Data m_current;
};

} // Internal

/*!
    \class Core::BaseFileFilter
    \inheaderfile coreplugin/locator/basefilefilter.h
    \inmodule QtCreator

    \brief The BaseFileFilter class is a base class for locator filter classes.
*/

/*!
    \class Core::BaseFileFilter::Iterator
    \inmodule QtCreator
    \internal
*/

/*!
    \class Core::BaseFileFilter::ListIterator
    \inmodule QtCreator
    \internal
*/

BaseFileFilter::Iterator::~Iterator() = default;

/*!
    \internal
*/
BaseFileFilter::BaseFileFilter()
  : d(new Internal::BaseFileFilterPrivate)
{
    d->m_data.forceNewSearchList = true;
    setFileIterator(new ListIterator({}));
}

/*!
    \internal
*/
BaseFileFilter::~BaseFileFilter()
{
    delete d;
}

/*!
    \reimp
*/
void BaseFileFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    d->m_current = d->m_data;
    d->m_data.forceNewSearchList = false;
}

ILocatorFilter::MatchLevel BaseFileFilter::matchLevelFor(const QRegularExpressionMatch &match,
                                                         const QString &matchText)
{
    const int consecutivePos = match.capturedStart(1);
    if (consecutivePos == 0)
        return MatchLevel::Best;
    if (consecutivePos > 0) {
        const QChar prevChar = matchText.at(consecutivePos - 1);
        if (prevChar == '_' || prevChar == '.')
            return MatchLevel::Better;
    }
    if (match.capturedStart() == 0)
        return MatchLevel::Good;
    return MatchLevel::Normal;
}

/*!
    \reimp
*/
QList<LocatorFilterEntry> BaseFileFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &origEntry)
{
    QList<LocatorFilterEntry> entries[int(MatchLevel::Count)];
    // If search string contains spaces, treat them as wildcard '*' and search in full path
    const QString entry = QDir::fromNativeSeparators(origEntry).replace(' ', '*');
    const Link link = Link::fromString(entry, true);

    const QRegularExpression regexp = createRegExp(link.targetFilePath.toString());
    if (!regexp.isValid()) {
        d->m_current.clear(); // free memory
        return {};
    }
    auto containsPathSeparator = [](const QString &candidate) {
        return candidate.contains('/') || candidate.contains('*');
    };

    const bool hasPathSeparator = containsPathSeparator(link.targetFilePath.toString());
    const bool containsPreviousEntry = !d->m_current.previousEntry.isEmpty()
            && link.targetFilePath.toString().contains(d->m_current.previousEntry);
    const bool pathSeparatorAdded = !containsPathSeparator(d->m_current.previousEntry)
            && hasPathSeparator;
    const bool searchInPreviousResults = !d->m_current.forceNewSearchList && containsPreviousEntry
            && !pathSeparatorAdded;
    if (searchInPreviousResults)
        d->m_current.iterator.reset(new ListIterator(d->m_current.previousResultPaths));

    QTC_ASSERT(d->m_current.iterator.data(), return QList<LocatorFilterEntry>());
    d->m_current.previousResultPaths.clear();
    d->m_current.previousEntry = link.targetFilePath.toString();
    d->m_current.iterator->toFront();
    bool canceled = false;
    while (d->m_current.iterator->hasNext()) {
        if (future.isCanceled()) {
            canceled = true;
            break;
        }

        d->m_current.iterator->next();
        FilePath path = d->m_current.iterator->filePath();
        QString matchText = hasPathSeparator ? path.toString() : path.fileName();
        QRegularExpressionMatch match = regexp.match(matchText);

        if (match.hasMatch()) {
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = path.fileName();
            filterEntry.filePath = path;
            filterEntry.extraInfo = path.shortNativePath();
            filterEntry.linkForEditor = Link(path, link.targetLine, link.targetColumn);
            const MatchLevel matchLevel = matchLevelFor(match, matchText);
            if (hasPathSeparator) {
                match = regexp.match(filterEntry.extraInfo);
                filterEntry.highlightInfo =
                        highlightInfo(match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
            } else {
                filterEntry.highlightInfo = highlightInfo(match);
            }

            entries[int(matchLevel)].append(filterEntry);
            d->m_current.previousResultPaths.append(path);
        }
    }

    if (canceled) {
        // we keep the old list of previous search results if this search was canceled
        // so a later search without forceNewSearchList will use that previous list instead of an
        // incomplete list of a canceled search
        d->m_current.clear(); // free memory
    } else {
        d->m_current.iterator.clear();
        QMetaObject::invokeMethod(this, &BaseFileFilter::updatePreviousResultData,
                                  Qt::QueuedConnection);
    }

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<LocatorFilterEntry>());
}

/*!
   Takes ownership of the \a iterator. The previously set iterator might not be deleted until
   a currently running search is finished.
*/

void BaseFileFilter::setFileIterator(BaseFileFilter::Iterator *iterator)
{
    d->m_data.clear();
    d->m_data.forceNewSearchList = true;
    d->m_data.iterator.reset(iterator);
}

/*!
    Returns the file iterator.
*/
QSharedPointer<BaseFileFilter::Iterator> BaseFileFilter::fileIterator()
{
    return d->m_data.iterator;
}

void BaseFileFilter::updatePreviousResultData()
{
    if (d->m_data.forceNewSearchList) // in the meantime the iterator was reset / cache invalidated
        return; // do not update with the new result list etc
    d->m_data.previousEntry = d->m_current.previousEntry;
    d->m_data.previousResultPaths = d->m_current.previousResultPaths;
    // forceNewSearchList was already reset in prepareSearch
}

BaseFileFilter::ListIterator::ListIterator(const FilePaths  &filePaths)
{
    m_filePaths = filePaths;
    toFront();
}

void BaseFileFilter::ListIterator::toFront()
{
    m_pathPosition = m_filePaths.constBegin() - 1;
}

bool BaseFileFilter::ListIterator::hasNext() const
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return false);
    return m_pathPosition + 1 != m_filePaths.constEnd();
}

FilePath BaseFileFilter::ListIterator::next()
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return {});
    ++m_pathPosition;
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return {});
    return *m_pathPosition;
}

FilePath BaseFileFilter::ListIterator::filePath() const
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return {});
    return *m_pathPosition;
}

} // Core
