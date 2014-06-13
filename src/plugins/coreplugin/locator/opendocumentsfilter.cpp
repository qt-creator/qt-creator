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

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>

#include <QFileInfo>

using namespace Core;
using namespace Core;
using namespace Core::Internal;
using namespace Utils;

OpenDocumentsFilter::OpenDocumentsFilter()
{
    setId("Open documents");
    setDisplayName(tr("Open Documents"));
    setShortcutString(QString(QLatin1Char('o')));
    setIncludedByDefault(true);

    connect(EditorManager::instance(), SIGNAL(editorOpened(Core::IEditor*)),
            this, SLOT(refreshInternally()));
    connect(EditorManager::instance(), SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(refreshInternally()));
}

QList<LocatorFilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<Core::LocatorFilterEntry> &future, const QString &entry_)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    QString entry = entry_;
    const QString lineNoSuffix = EditorManager::splitLineNumber(&entry);
    const QChar asterisk = QLatin1Char('*');
    QString pattern = QString(asterisk);
    pattern += entry;
    pattern += asterisk;
    QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    foreach (const DocumentModel::Entry &editorEntry, m_editors) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName();
        if (fileName.isEmpty())
            continue;
        QString displayName = editorEntry.displayName();
        if (regexp.exactMatch(displayName)) {
            QFileInfo fi(fileName);
            LocatorFilterEntry fiEntry(this, displayName, QString(fileName + lineNoSuffix));
            fiEntry.extraInfo = FileUtils::shortNativePath(FileName(fi));
            fiEntry.fileName = fileName;
            QList<LocatorFilterEntry> &category = displayName.startsWith(entry, caseSensitivityForPrefix)
                    ? betterEntries : goodEntries;
            category.append(fiEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void OpenDocumentsFilter::refreshInternally()
{
    m_editors.clear();
    foreach (DocumentModel::Entry *e, DocumentModel::entries()) {
        DocumentModel::Entry entry;
        // create copy with only the information relevant to use
        // to avoid model deleting entries behind our back
        entry.m_displayName = e->displayName();
        entry.m_fileName = e->fileName();
        m_editors.append(entry);
    }
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "refreshInternally", Qt::BlockingQueuedConnection);
}

void OpenDocumentsFilter::accept(LocatorFilterEntry selection) const
{
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::CanContainLineNumber);
}
