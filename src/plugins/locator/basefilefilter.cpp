/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "basefilefilter.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDir>
#include <QtCore/QStringMatcher>

using namespace Core;
using namespace Locator;

BaseFileFilter::BaseFileFilter()
  : m_forceNewSearchList(false)
{
}

QList<FilterEntry> BaseFileFilter::matchesFor(const QString &origEntry)
{
    updateFiles();
    QList<FilterEntry> matches;
    QList<FilterEntry> badMatches;
    QString needle = trimWildcards(origEntry);
    QStringMatcher matcher(needle, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    const QRegExp regexp(asterisk + needle+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return matches;
    bool hasWildcard = (needle.contains(asterisk) || needle.contains('?'));
    QStringList searchListPaths;
    QStringList searchListNames;
    if (!m_previousEntry.isEmpty() && !m_forceNewSearchList && needle.contains(m_previousEntry)) {
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
    QStringListIterator paths(searchListPaths);
    QStringListIterator names(searchListNames);
    while (paths.hasNext() && names.hasNext()) {
        QString path = paths.next();
        QString name = names.next();
        if ((hasWildcard && regexp.exactMatch(name))
                || (!hasWildcard && matcher.indexIn(name) != -1)) {
            QFileInfo fi(path);
            FilterEntry entry(this, fi.fileName(), path);
            entry.extraInfo = QDir::toNativeSeparators(fi.path());
            entry.resolveFileIcon = true;
            if (name.startsWith(needle))
                matches.append(entry);
            else
                badMatches.append(entry);
            m_previousResultPaths.append(path);
            m_previousResultNames.append(name);
        }
    }

    matches.append(badMatches);
    return matches;
}

void BaseFileFilter::accept(Locator::FilterEntry selection) const
{
    Core::EditorManager *em = Core::EditorManager::instance();
    em->openEditor(selection.internalData.toString());
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
