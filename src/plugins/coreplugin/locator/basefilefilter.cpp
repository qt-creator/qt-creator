/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basefilefilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QStringMatcher>
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

QList<LocatorFilterEntry> BaseFileFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &origEntry)
{
    QList<LocatorFilterEntry> betterEntries;
    QList<LocatorFilterEntry> goodEntries;
    QString needle = trimWildcards(QDir::fromNativeSeparators(origEntry));
    const QString lineNoSuffix = EditorManager::splitLineAndColumnNumber(&needle);
    QStringMatcher matcher(needle, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    QRegExp regexp(asterisk + needle+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid()) {
        d->m_current.clear(); // free memory
        return betterEntries;
    }
    const QChar pathSeparator(QLatin1Char('/'));
    const bool hasPathSeparator = needle.contains(pathSeparator);
    const bool hasWildcard = needle.contains(asterisk) || needle.contains(QLatin1Char('?'));
    const bool containsPreviousEntry = !d->m_current.previousEntry.isEmpty()
            && needle.contains(d->m_current.previousEntry);
    const bool pathSeparatorAdded = !d->m_current.previousEntry.contains(pathSeparator)
            && needle.contains(pathSeparator);
    const bool searchInPreviousResults = !d->m_current.forceNewSearchList && containsPreviousEntry
            && !pathSeparatorAdded;
    if (searchInPreviousResults)
        d->m_current.iterator.reset(new ListIterator(d->m_current.previousResultPaths,
                                                     d->m_current.previousResultNames));

    QTC_ASSERT(d->m_current.iterator.data(), return QList<LocatorFilterEntry>());
    d->m_current.previousResultPaths.clear();
    d->m_current.previousResultNames.clear();
    d->m_current.previousEntry = needle;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(needle);
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
        if ((hasWildcard && regexp.exactMatch(matchText))
                || (!hasWildcard && matcher.indexIn(matchText) != -1)) {
            QFileInfo fi(path);
            LocatorFilterEntry entry(this, fi.fileName(), QString(path + lineNoSuffix));
            entry.extraInfo = FileUtils::shortNativePath(FileName(fi));
            entry.fileName = path;
            if (matchText.startsWith(needle, caseSensitivityForPrefix))
                betterEntries.append(entry);
            else
                goodEntries.append(entry);
            d->m_current.previousResultPaths.append(path);
            d->m_current.previousResultNames.append(name);
        }
    }

    betterEntries.append(goodEntries);
    if (canceled) {
        // we keep the old list of previous search results if this search was canceled
        // so a later search without foreNewSearchList will use that previous list instead of an
        // incomplete list of a canceled search
        d->m_current.clear(); // free memory
    } else {
        d->m_current.iterator.clear();
        QTimer::singleShot(0, this, SLOT(updatePreviousResultData()));
    }
    return betterEntries;
}

void BaseFileFilter::accept(LocatorFilterEntry selection) const
{
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
    foreach (const QString &path, m_filePaths) {
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
