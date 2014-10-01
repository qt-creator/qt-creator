/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "basefilefilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/fileutils.h>

#include <QDir>
#include <QStringMatcher>

using namespace Core;
using namespace Core;
using namespace Utils;

BaseFileFilter::BaseFileFilter()
  : m_forceNewSearchList(false)
{
}

QList<LocatorFilterEntry> BaseFileFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &origEntry)
{
    updateFiles();
    QList<LocatorFilterEntry> betterEntries;
    QList<LocatorFilterEntry> goodEntries;
    QString needle = trimWildcards(QDir::fromNativeSeparators(origEntry));
    const QString lineNoSuffix = EditorManager::splitLineNumber(&needle);
    QStringMatcher matcher(needle, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    QRegExp regexp(asterisk + needle+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return betterEntries;
    const QChar pathSeparator(QLatin1Char('/'));
    const bool hasPathSeparator = needle.contains(pathSeparator);
    const bool hasWildcard = needle.contains(asterisk) || needle.contains(QLatin1Char('?'));
    QStringList searchListPaths;
    QStringList searchListNames;
    const bool containsPreviousEntry = !m_previousEntry.isEmpty()
            && needle.contains(m_previousEntry);
    const bool pathSeparatorAdded = !m_previousEntry.contains(pathSeparator)
            && needle.contains(pathSeparator);
    if (!m_forceNewSearchList && containsPreviousEntry && !pathSeparatorAdded) {
        searchListPaths = m_previousResultPaths;
        searchListNames = m_previousResultNames;
    } else {
        searchListPaths = m_files;
        searchListNames = m_fileNames;
    }
    m_previousResultPaths.clear();
    m_previousResultNames.clear();
    m_forceNewSearchList = false;
    m_previousEntry = needle;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(needle);
    QStringListIterator paths(searchListPaths);
    QStringListIterator names(searchListNames);
    while (paths.hasNext() && names.hasNext()) {
        if (future.isCanceled())
            break;

        QString path = paths.next();
        QString name = names.next();
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
            m_previousResultPaths.append(path);
            m_previousResultNames.append(name);
        }
    }

    betterEntries.append(goodEntries);
    return betterEntries;
}

void BaseFileFilter::accept(Core::LocatorFilterEntry selection) const
{
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::CanContainLineNumber);
}

void BaseFileFilter::generateFileNames()
{
    m_fileNames.clear();
    foreach (const QString &fileName, m_files) {
        QFileInfo fi(fileName);
        m_fileNames.append(fi.fileName());
    }
    m_forceNewSearchList = true;
}

void BaseFileFilter::updateFiles()
{
}
