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
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QRegularExpression>
#include <QTimer>

using namespace Core;
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
        previousResultNames.clear();
        previousEntry.clear();
    }

    QSharedPointer<BaseFileFilter::Iterator> iterator;
    QStringList previousResultPaths;
    QStringList previousResultNames;
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
} // Core

BaseFileFilter::Iterator::~Iterator()
{}

BaseFileFilter::BaseFileFilter()
  : d(new Internal::BaseFileFilterPrivate)
{
    d->m_data.forceNewSearchList = true;
    setFileIterator(new ListIterator(QStringList()));
}

BaseFileFilter::~BaseFileFilter()
{
    delete d;
}

void BaseFileFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    d->m_current.iterator = d->m_data.iterator;
    d->m_current.previousResultPaths = d->m_data.previousResultPaths;
    d->m_current.previousResultNames = d->m_data.previousResultNames;
    d->m_current.forceNewSearchList = d->m_data.forceNewSearchList;
    d->m_current.previousEntry = d->m_data.previousEntry;
    d->m_data.forceNewSearchList = false;
}

static int matchLevelFor(const QRegularExpressionMatch &match, const QString &matchText)
{
    const int consecutivePos = match.capturedStart(1);
    if (consecutivePos == 0)
        return 0;
    if (consecutivePos > 0) {
        const QChar prevChar = matchText.at(consecutivePos - 1);
        if (prevChar == '_' || prevChar == '.')
            return 1;
    }
    if (match.capturedStart() == 0)
        return 2;
    return 3;
}

QList<LocatorFilterEntry> BaseFileFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &origEntry)
{
    QList<LocatorFilterEntry> entries[4];
    const QString entry = QDir::fromNativeSeparators(origEntry);
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);

    const QRegularExpression regexp = createRegExp(fp.filePath);
    if (!regexp.isValid()) {
        d->m_current.clear(); // free memory
        return entries[0];
    }
    const QChar pathSeparator('/');
    const bool hasPathSeparator = fp.filePath.contains(pathSeparator);
    const bool containsPreviousEntry = !d->m_current.previousEntry.isEmpty()
            && fp.filePath.contains(d->m_current.previousEntry);
    const bool pathSeparatorAdded = !d->m_current.previousEntry.contains(pathSeparator)
            && fp.filePath.contains(pathSeparator);
    const bool searchInPreviousResults = !d->m_current.forceNewSearchList && containsPreviousEntry
            && !pathSeparatorAdded;
    if (searchInPreviousResults)
        d->m_current.iterator.reset(new ListIterator(d->m_current.previousResultPaths,
                                                     d->m_current.previousResultNames));

    QTC_ASSERT(d->m_current.iterator.data(), return QList<LocatorFilterEntry>());
    d->m_current.previousResultPaths.clear();
    d->m_current.previousResultNames.clear();
    d->m_current.previousEntry = fp.filePath;
    d->m_current.iterator->toFront();
    bool canceled = false;
    while (d->m_current.iterator->hasNext()) {
        if (future.isCanceled()) {
            canceled = true;
            break;
        }

        d->m_current.iterator->next();
        QString path = d->m_current.iterator->filePath();
        QString name = d->m_current.iterator->fileName();
        QString matchText = hasPathSeparator ? path : name;
        QRegularExpressionMatch match = regexp.match(matchText);

        if (match.hasMatch()) {
            QFileInfo fi(path);
            LocatorFilterEntry filterEntry(this, fi.fileName(), QString(path + fp.postfix));
            filterEntry.fileName = path;
            filterEntry.extraInfo = FileUtils::shortNativePath(FileName(fi));

            const int matchLevel = matchLevelFor(match, matchText);
            if (hasPathSeparator) {
                match = regexp.match(filterEntry.extraInfo);
                filterEntry.highlightInfo =
                        highlightInfo(match, LocatorFilterEntry::HighlightInfo::ExtraInfo);
            } else {
                filterEntry.highlightInfo = highlightInfo(match);
            }

            entries[matchLevel].append(filterEntry);
            d->m_current.previousResultPaths.append(path);
            d->m_current.previousResultNames.append(name);
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
    return entries[0] + entries[1] + entries[2] + entries[3];
}

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
    d->m_data.previousResultNames = d->m_current.previousResultNames;
    // forceNewSearchList was already reset in prepareSearch
}

BaseFileFilter::ListIterator::ListIterator(const QStringList &filePaths)
{
    m_filePaths = filePaths;
    for (const QString &path : filePaths) {
        QFileInfo fi(path);
        m_fileNames.append(fi.fileName());
    }
    toFront();
}

BaseFileFilter::ListIterator::ListIterator(const QStringList &filePaths,
                                           const QStringList &fileNames)
{
    m_filePaths = filePaths;
    m_fileNames = fileNames;
    toFront();
}

void BaseFileFilter::ListIterator::toFront()
{
    m_pathPosition = m_filePaths.constBegin() - 1;
    m_namePosition = m_fileNames.constBegin() - 1;
}

bool BaseFileFilter::ListIterator::hasNext() const
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return false);
    return m_pathPosition + 1 != m_filePaths.constEnd();
}

QString BaseFileFilter::ListIterator::next()
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return QString());
    QTC_ASSERT(m_namePosition != m_fileNames.constEnd(), return QString());
    ++m_pathPosition;
    ++m_namePosition;
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return QString());
    QTC_ASSERT(m_namePosition != m_fileNames.constEnd(), return QString());
    return *m_pathPosition;
}

QString BaseFileFilter::ListIterator::filePath() const
{
    QTC_ASSERT(m_pathPosition != m_filePaths.constEnd(), return QString());
    return *m_pathPosition;
}

QString BaseFileFilter::ListIterator::fileName() const
{
    QTC_ASSERT(m_namePosition != m_fileNames.constEnd(), return QString());
    return *m_namePosition;
}
