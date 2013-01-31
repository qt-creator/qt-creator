/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
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
using namespace Locator;
using namespace Utils;

BaseFileFilter::BaseFileFilter()
  : m_forceNewSearchList(false)
{
}

QList<FilterEntry> BaseFileFilter::matchesFor(QFutureInterface<Locator::FilterEntry> &future, const QString &origEntry)
{
    updateFiles();
    QList<FilterEntry> matches;
    QList<FilterEntry> badMatches;
    QString needle = trimWildcards(origEntry);
    const QString lineNoSuffix = EditorManager::splitLineNumber(&needle);
    QStringMatcher matcher(needle, Qt::CaseInsensitive);
    const QChar asterisk = QLatin1Char('*');
    QRegExp regexp(asterisk + needle+ asterisk, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return matches;
    const bool hasWildcard = needle.contains(asterisk) || needle.contains(QLatin1Char('?'));
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
        if (future.isCanceled())
            break;

        QString path = paths.next();
        QString name = names.next();
        if ((hasWildcard && regexp.exactMatch(name))
                || (!hasWildcard && matcher.indexIn(name) != -1)) {
            QFileInfo fi(path);
            FilterEntry entry(this, fi.fileName(), QString(path + lineNoSuffix));
            entry.extraInfo = FileUtils::shortNativePath(FileName(fi));
            entry.fileName = path;
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
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::ModeSwitch | EditorManager::CanContainLineNumber);
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
