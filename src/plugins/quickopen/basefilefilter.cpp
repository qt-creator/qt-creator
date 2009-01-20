/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "basefilefilter.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QDir>

using namespace Core;
using namespace QuickOpen;

BaseFileFilter::BaseFileFilter()
  : m_forceNewSearchList(false)
{
}

QList<FilterEntry> BaseFileFilter::matchesFor(const QString &origEntry)
{
    QList<FilterEntry> value;
    QString entry = trimWildcards(origEntry);
    QStringMatcher matcher(entry, Qt::CaseInsensitive);
    const QRegExp regexp("*"+entry+"*", Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return value;
    bool hasWildcard = (entry.contains('*') || entry.contains('?'));
    QStringList searchListPaths;
    QStringList searchListNames;
    if (!m_previousEntry.isEmpty() && !m_forceNewSearchList && entry.contains(m_previousEntry)) {
        searchListPaths = m_previousResultPaths;
        searchListNames = m_previousResultNames;
    } else {
        searchListPaths = m_files;
        searchListNames = m_fileNames;
    }
    m_previousResultPaths.clear();
    m_previousResultNames.clear();
    m_forceNewSearchList = false;
    m_previousEntry = entry;
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
            value.append(entry);
            m_previousResultPaths.append(path);
            m_previousResultNames.append(name);
        }
    }
    return value;
}

void BaseFileFilter::accept(QuickOpen::FilterEntry selection) const
{
    Core::EditorManager *em = Core::ICore::instance()->editorManager();
    em->openEditor(selection.internalData.toString());
    em->ensureEditorManagerVisible();
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
