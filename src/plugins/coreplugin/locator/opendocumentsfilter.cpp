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

#include "opendocumentsfilter.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <utils/fileutils.h>

#include <QAbstractItemModel>
#include <QFileInfo>
#include <QMutexLocker>

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

QList<LocatorFilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry_)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    QString entry = entry_;
    const QString lineNoSuffix = EditorManager::splitLineAndColumnNumber(&entry);
    const QChar asterisk = QLatin1Char('*');
    QString pattern = QString(asterisk);
    pattern += entry;
    pattern += asterisk;
    QRegExp regexp(pattern, Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!regexp.isValid())
        return goodEntries;
    const Qt::CaseSensitivity caseSensitivityForPrefix = caseSensitivity(entry);
    foreach (const Entry &editorEntry, editors()) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName.toString();
        if (fileName.isEmpty())
            continue;
        QString displayName = editorEntry.displayName;
        if (regexp.exactMatch(displayName)) {
            LocatorFilterEntry fiEntry(this, displayName, QString(fileName + lineNoSuffix));
            fiEntry.extraInfo = FileUtils::shortNativePath(FileName::fromString(fileName));
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
