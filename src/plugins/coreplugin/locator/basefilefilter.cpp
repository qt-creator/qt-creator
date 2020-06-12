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

#include "basefilefilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QTimer>

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
    d->m_current.iterator = d->m_data.iterator;
    d->m_current.previousResultPaths = d->m_data.previousResultPaths;
    d->m_current.forceNewSearchList = d->m_data.forceNewSearchList;
    d->m_current.previousEntry = d->m_data.previousEntry;
    d->m_data.forceNewSearchList = false;
}

ILocatorFilter::MatchLevel BaseFileFilter::matchLevelFor(const QRegularExpressionMatch &match,
                                                         const QString &matchText) const
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
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);

    const QRegularExpression regexp = createRegExp(fp.filePath);
    if (!regexp.isValid()) {
        d->m_current.clear(); // free memory
        return {};
    }
    auto containsPathSeparator = [](const QString &candidate) {
        return candidate.contains('/') || candidate.contains('*');
    };

    const bool hasPathSeparator = containsPathSeparator(fp.filePath);
    const bool containsPreviousEntry = !d->m_current.previousEntry.isEmpty()
            && fp.filePath.contains(d->m_current.previousEntry);
    const bool pathSeparatorAdded = !containsPathSeparator(d->m_current.previousEntry)
            && hasPathSeparator;
    const bool searchInPreviousResults = !d->m_current.forceNewSearchList && containsPreviousEntry
            && !pathSeparatorAdded;
    if (searchInPreviousResults)
        d->m_current.iterator.reset(new ListIterator(d->m_current.previousResultPaths));

    QTC_ASSERT(d->m_current.iterator.data(), return QList<LocatorFilterEntry>());
    d->m_current.previousResultPaths.clear();
    d->m_current.previousEntry = fp.filePath;
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
            QFileInfo fi(path.toString());
            LocatorFilterEntry filterEntry(this, fi.fileName(), QString(path.toString() + fp.postfix));
            filterEntry.fileName = path.toString();
            filterEntry.extraInfo = FilePath::fromFileInfo(fi).shortNativePath();

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
        QTimer::singleShot(0, this, &BaseFileFilter::updatePreviousResultData);
    }

    for (auto &entry : entries) {
        if (entry.size() < 1000)
            Utils::sort(entry, Core::LocatorFilterEntry::compareLexigraphically);
    }

    return std::accumulate(std::begin(entries), std::end(entries), QList<LocatorFilterEntry>());
}

/*!
    \reimp
*/
void BaseFileFilter::accept(LocatorFilterEntry selection,
                            QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::CanContainLineAndColumnNumber);
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
