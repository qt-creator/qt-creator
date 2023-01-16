// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "opendocumentsfilter.h"

#include "basefilefilter.h"
#include "../coreplugintr.h"

#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/linecolumn.h>

#include <QAbstractItemModel>
#include <QMutexLocker>
#include <QRegularExpression>

using namespace Utils;

namespace Core::Internal {

OpenDocumentsFilter::OpenDocumentsFilter()
{
    setId("Open documents");
    setDisplayName(Tr::tr("Open Documents"));
    setDefaultShortcutString("o");
    setPriority(High);
    setDefaultIncludedByDefault(true);

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
    QString postfix;
    Link link = Link::fromString(entry, true, &postfix);

    const QRegularExpression regexp = createRegExp(link.targetFilePath.toString());
    if (!regexp.isValid())
        return goodEntries;

    const QList<Entry> editorEntries = editors();
    for (const Entry &editorEntry : editorEntries) {
        if (future.isCanceled())
            break;
        QString fileName = editorEntry.fileName.toString();
        if (fileName.isEmpty())
            continue;
        QString displayName = editorEntry.displayName;
        const QRegularExpressionMatch match = regexp.match(displayName);
        if (match.hasMatch()) {
            LocatorFilterEntry filterEntry(this, displayName, QString(fileName + postfix));
            filterEntry.filePath = FilePath::fromString(fileName);
            filterEntry.extraInfo = filterEntry.filePath.shortNativePath();
            filterEntry.highlightInfo = highlightInfo(match);
            if (match.capturedStart() == 0)
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
    QMutexLocker lock(&m_mutex);
    m_editors.clear();
    const QList<DocumentModel::Entry *> documentEntries = DocumentModel::entries();
    for (DocumentModel::Entry *e : documentEntries) {
        Entry entry;
        // create copy with only the information relevant to use
        // to avoid model deleting entries behind our back
        entry.displayName = e->displayName();
        entry.fileName = e->filePath();
        m_editors.append(entry);
    }
}

QList<OpenDocumentsFilter::Entry> OpenDocumentsFilter::editors() const
{
    QMutexLocker lock(&m_mutex);
    return m_editors;
}

void OpenDocumentsFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
    QMetaObject::invokeMethod(this, &OpenDocumentsFilter::refreshInternally, Qt::QueuedConnection);
}

void OpenDocumentsFilter::accept(const LocatorFilterEntry &selection,
                                 QString *newText, int *selectionStart, int *selectionLength) const
{
    Q_UNUSED(newText)
    Q_UNUSED(selectionStart)
    Q_UNUSED(selectionLength)
    BaseFileFilter::openEditorAt(selection);
}

} // Core::Internal
