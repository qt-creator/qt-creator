// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "documentmodel.h"
#include "ieditor.h"

#include <QAbstractItemModel>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QMap>

namespace Core {
namespace Internal {

class DocumentModelPrivate : public QAbstractItemModel
{
    Q_OBJECT

public:
    ~DocumentModelPrivate() override;

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QModelIndex parent(const QModelIndex &/*index*/) const override { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;

    DocumentModel::Entry *addEntry(DocumentModel::Entry *entry);
    void removeDocument(int idx);

    std::optional<int> indexOfFilePath(const Utils::FilePath &filePath) const;
    std::optional<int> indexOfDocument(IDocument *document) const;

    bool disambiguateDisplayNames(DocumentModel::Entry *entry);

    static void setPinned(DocumentModel::Entry *entry, bool pinned);

    static QIcon lockedIcon();
    static QIcon pinnedIcon();
    static void addEditor(IEditor *editor, bool *isNewDocument);
    static DocumentModel::Entry *addSuspendedDocument(const Utils::FilePath &filePath,
                                                      const QString &displayName,
                                                      Utils::Id id);
    static DocumentModel::Entry *firstSuspendedEntry();
    static DocumentModel::Entry *removeEditor(IEditor *editor);
    static void removeEntry(DocumentModel::Entry *entry);
    enum PinnedFileRemovalPolicy {
        DoNotRemovePinnedFiles,
        RemovePinnedFiles
    };
    static void removeAllSuspendedEntries(PinnedFileRemovalPolicy pinnedFileRemovalPolicy
                                          = RemovePinnedFiles);

    void itemChanged(IDocument *document);

    QList<DocumentModel::Entry *> m_entries;
    QMap<IDocument *, QList<IEditor *> > m_editors;
    QHash<Utils::FilePath, DocumentModel::Entry *> m_entryByFixedPath;
};

} // Internal
} // Core
