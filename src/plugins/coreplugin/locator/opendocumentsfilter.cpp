// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "opendocumentsfilter.h"

#include "../coreplugintr.h"

#include <utils/filepath.h>
#include <utils/link.h>
#include <utils/tasktree.h>

#include <QAbstractItemModel>
#include <QMutexLocker>
#include <QRegularExpression>

using namespace Utils;

namespace Core::Internal {

OpenDocumentsFilter::OpenDocumentsFilter()
{
    setId("Open documents");
    setDisplayName(Tr::tr("Open Documents"));
    setDescription(Tr::tr("Switches to an open document."));
    setDefaultShortcutString("o");
    setPriority(High);
    setDefaultIncludedByDefault(true);
    setRefreshRecipe(Tasking::Sync([this] { refreshInternally(); return true; }));

    connect(DocumentModel::model(), &QAbstractItemModel::dataChanged,
            this, &OpenDocumentsFilter::slotDataChanged);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsInserted,
            this, &OpenDocumentsFilter::slotRowsInserted);
    connect(DocumentModel::model(), &QAbstractItemModel::rowsRemoved,
            this, &OpenDocumentsFilter::slotRowsRemoved);
}

void OpenDocumentsFilter::slotDataChanged(const QModelIndex &topLeft, const QModelIndex &bottomRight,
                                          const QVector<int> &roles)
{
    Q_UNUSED(roles)

    const int topIndex = std::max(0, topLeft.row() - 1 /*<no document>*/);
    const int bottomIndex = bottomRight.row() - 1 /*<no document>*/;

    QMutexLocker lock(&m_mutex);

    const QList<DocumentModel::Entry *> documentEntries = DocumentModel::entries();
    for (int i = topIndex; i <= bottomIndex; ++i) {
        QTC_ASSERT(i < m_editors.size(), break);
        DocumentModel::Entry *e = documentEntries.at(i);
        m_editors[i] = {e->filePath(), e->displayName()};
    }
}

void OpenDocumentsFilter::slotRowsInserted(const QModelIndex &, int first, int last)
{
    const int firstIndex = std::max(0, first - 1 /*<no document>*/);

    QMutexLocker lock(&m_mutex);

    const QList<DocumentModel::Entry *> documentEntries = DocumentModel::entries();
    for (int i = firstIndex; i < last; ++i) {
        DocumentModel::Entry *e = documentEntries.at(i);
        m_editors.insert(i, {e->filePath(), e->displayName()});
    }
}

void OpenDocumentsFilter::slotRowsRemoved(const QModelIndex &, int first, int last)
{
    QMutexLocker lock(&m_mutex);

    const int firstIndex = std::max(0, first - 1 /*<no document>*/);
    for (int i = firstIndex; i < last; ++i)
        m_editors.removeAt(i);
}

QList<LocatorFilterEntry> OpenDocumentsFilter::matchesFor(QFutureInterface<LocatorFilterEntry> &future,
                                                          const QString &entry)
{
    QList<LocatorFilterEntry> goodEntries;
    QList<LocatorFilterEntry> betterEntries;
    const Link link = Link::fromString(entry, true);

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
            LocatorFilterEntry filterEntry;
            filterEntry.displayName = displayName;
            filterEntry.filePath = FilePath::fromString(fileName);
            filterEntry.extraInfo = filterEntry.filePath.shortNativePath();
            filterEntry.highlightInfo = highlightInfo(match);
            filterEntry.linkForEditor = Link(filterEntry.filePath, link.targetLine,
                                             link.targetColumn);
            if (match.capturedStart() == 0)
                betterEntries.append(filterEntry);
            else
                goodEntries.append(filterEntry);
        }
    }
    betterEntries.append(goodEntries);
    return betterEntries;
}

QList<OpenDocumentsFilter::Entry> OpenDocumentsFilter::editors() const
{
    QMutexLocker lock(&m_mutex);
    return m_editors;
}

void OpenDocumentsFilter::refreshInternally()
{
    QMutexLocker lock(&m_mutex);
    m_editors.clear();
    const QList<DocumentModel::Entry *> documentEntries = DocumentModel::entries();
    // create copy with only the information relevant to use
    // to avoid model deleting entries behind our back
    for (DocumentModel::Entry *e : documentEntries)
        m_editors.append({e->filePath(), e->displayName()});
}

} // Core::Internal
