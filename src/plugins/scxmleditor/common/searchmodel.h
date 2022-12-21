// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "scxmldocument.h"
#include "scxmltag.h"

#include <QAbstractTableModel>

namespace ScxmlEditor {

namespace Common {

class SearchModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum AttributeRole {
        FilterRole = Qt::UserRole + 1
    };

    SearchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    PluginInterface::ScxmlTag *tag(const QModelIndex &ind);
    void setFilter(const QString &filter);
    void setDocument(PluginInterface::ScxmlDocument *document);

private:
    void tagChange(PluginInterface::ScxmlDocument::TagChange change, PluginInterface::ScxmlTag *tag, const QVariant &value);
    void resetModel();

    PluginInterface::ScxmlDocument *m_document = nullptr;
    QVector<PluginInterface::ScxmlTag*> m_allTags;
    QString m_strFilter;
};

} // namespace Common
} // namespace ScxmlEditor
