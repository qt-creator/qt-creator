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

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QMutexLocker>
#include <QRegExp>

using namespace Core;
using namespace Core;
using namespace Core::Internal;
using namespace Utils;

OpenDocumentsFilter::OpenDocumentsFilter()
{
    setId("Open documents");
    setDisplayName(tr("Open Documents"));
    setShortcutString(QString(QLatin1Char('o')));
    setPriority(High);
    setIncludedByDefault(true);

    connect(DocumentModel::model(), &QAbstractItemModel::dataChanged,
            this, &OpenDocumentsFilter::refreshInternally);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsInserted,
            this, &OpenDocumentsFilter::refreshInternally);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsRemoved,
            this, &OpenDocumentsFilter::refreshInternally);
}

QList<LocatorFilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                          const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);
    QRegExp regexp(fp.filePath, caseSensitivity(fp.filePath), QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;

    foreach (const Entry &editorEntry, editors()) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName.toString();
        if (fileName.isEmpty())
            continue;
        QString displayName = editorEntry.displayName;
        const int index = regexp.indexIn(displayName);
        if (index >= 0) {
            LocatorFilterEntry filterEntry(this, displayName, QString(fileName + fp.postfix));
            filterEntry.extraInfo = FileUtils::shortNativePath(FileName::fromString(fileName));
            filterEntry.fileName = fileName;
            filterEntry.highlightInfo = {index, regexp.matchedLength()};
            if (index == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

void OpenDocumentsFilter::refreshInternally()
{
    QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
    m_editors.clear();
    foreach (DocumentModel::Entry *e, DocumentModel::entries()) {
        Entry entry;
        // create copy with only the information relevant to use
        // to avoid model deleting entries behind our back
        entry.displayName = e->displayName();
        entry.fileName = e->fileName();
        m_editors.append(entry);
    }
}

QList<OpenDocumentsFilter::Entry> OpenDocumentsFilter::editors() const
{
    QMutexLocker lock(&m_mutex); Q_UNUSED(lock)
    return m_editors;
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, "refreshInternally", Qt::BlockingQueuedConnection);
}

void OpenDocumentsFilter::accept(LocatorFilterEntry selection) const
{
    EditorManager::openEditor(selection.internalData.toString(), Id(),
                              EditorManager::CanContainLineAndColumnNumber);
}
