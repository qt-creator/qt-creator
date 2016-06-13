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
    ~DocumentModelPrivate();

    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    QModelIndex parent(const QModelIndex &/*index*/) const override { return QModelIndex(); }
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex index(int row, int column = 0, const QModelIndex &parent = QModelIndex()) const override;

    Qt::DropActions supportedDragActions() const override;
    QStringList mimeTypes() const override;

    void addEntry(DocumentModel::Entry *entry);
    void removeDocument(int idx);

    int indexOfFilePath(const Utils::FileName &filePath) const;
    int indexOfDocument(IDocument *document) const;

    bool disambiguateDisplayNames(DocumentModel::Entry *entry);

    static QIcon lockedIcon();
    static void addEditor(IEditor *editor, bool *isNewDocument);
    static void addSuspendedDocument(const QString &fileName, const QString &displayName, Id id);
    static DocumentModel::Entry *firstSuspendedEntry();
    static DocumentModel::Entry *removeEditor(IEditor *editor);
    static void removeEntry(DocumentModel::Entry *entry);
    static void removeAllSuspendedEntries();

    void itemChanged();

    class DynamicEntry
    {
    public:
        DocumentModel::Entry *entry;
        int pathComponents;

        DynamicEntry(DocumentModel::Entry *e);
        DocumentModel::Entry *operator->() const;
        void disambiguate();
        void setNumberedName(int number);
    };

    QList<DocumentModel::Entry *> m_entries;
    QMap<IDocument *, QList<IEditor *> > m_editors;
    QHash<QString, DocumentModel::Entry *> m_entryByFixedPath;
};

} // Internal
} // Core
