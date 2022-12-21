// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmldocument.h"
#include "scxmltypes.h"

#include <QAbstractItemModel>
#include <QIcon>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QModelIndex)
QT_FORWARD_DECLARE_CLASS(QVariant)

namespace ScxmlEditor {

namespace Common {

class Icons
{
public:
    void addIcon(PluginInterface::TagType type, const QIcon &icon)
    {
        m_iconIndices << type;
        m_icons << icon;
    }

    QIcon icon(PluginInterface::TagType type) const
    {
        int ind = m_iconIndices.indexOf(type);
        if (ind >= 0 && ind < m_icons.count())
            return m_icons[ind];

        return m_emptyIcon;
    }

private:
    QIcon m_emptyIcon;
    QVector<PluginInterface::TagType> m_iconIndices;
    QVector<QIcon> m_icons;
};

class StructureModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    StructureModel(QObject *parent = nullptr);

    enum StructureItemDataRole {
        TagTypeRole = Qt::UserRole + 1
    };

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    PluginInterface::ScxmlTag *getItem(const QModelIndex &parent, int source_row) const;
    PluginInterface::ScxmlTag *getItem(const QModelIndex &index) const;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent) override;
    void updateData();
    void setDocument(PluginInterface::ScxmlDocument *document);

signals:
    void childAdded(const QModelIndex &childIndex);
    void selectIndex(const QModelIndex &index);

private:
    void beginTagChange(PluginInterface::ScxmlDocument::TagChange change, PluginInterface::ScxmlTag *tag, const QVariant &value);
    void endTagChange(PluginInterface::ScxmlDocument::TagChange change, PluginInterface::ScxmlTag *tag, const QVariant &value);

    QPointer<PluginInterface::ScxmlDocument> m_document;
    Icons m_icons;
    mutable QPointer<PluginInterface::ScxmlTag> m_dragTag;
};

} // namespace Common
} // namespace ScxmlEditor
