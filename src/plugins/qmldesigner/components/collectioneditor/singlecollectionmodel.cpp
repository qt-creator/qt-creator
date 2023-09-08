// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "singlecollectionmodel.h"

#include "nodemetainfo.h"
#include "variantproperty.h"

#include <utils/qtcassert.h>

namespace {
inline bool isListElement(const QmlDesigner::ModelNode &node)
{
    return node.metaInfo().isQtQuickListElement();
}

inline QByteArrayList getHeaders(const QByteArray &headersValue)
{
    QByteArrayList result;
    const QByteArrayList initialHeaders = headersValue.split(',');
    for (QByteArray header : initialHeaders) {
        header = header.trimmed();
        if (header.size())
            result.append(header);
    }
    return result;
}
} // namespace

namespace QmlDesigner {
SingleCollectionModel::SingleCollectionModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

int SingleCollectionModel::rowCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_elements.count();
}

int SingleCollectionModel::columnCount([[maybe_unused]] const QModelIndex &parent) const
{
    return m_headers.count();
}

QVariant SingleCollectionModel::data(const QModelIndex &index, int) const
{
    if (!index.isValid())
        return {};

    const QByteArray &propertyName = m_headers.at(index.column());
    const ModelNode &elementNode = m_elements.at(index.row());

    if (elementNode.hasVariantProperty(propertyName))
        return elementNode.variantProperty(propertyName).value();

    return {};
}

bool SingleCollectionModel::setData(const QModelIndex &, const QVariant &, int)
{
    return false;
}

Qt::ItemFlags SingleCollectionModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    return {Qt::ItemIsSelectable | Qt::ItemIsEnabled};
}

QVariant SingleCollectionModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           [[maybe_unused]] int role) const
{
    if (orientation == Qt::Horizontal)
        return m_headers.at(section);

    return {};
}

void SingleCollectionModel::setCollection(const ModelNode &collection)
{
    beginResetModel();
    m_collectionNode = collection;
    updateCollectionName();

    QTC_ASSERT(collection.isValid() && collection.hasVariantProperty("headers"), {
        m_headers.clear();
        m_elements.clear();
        endResetModel();
        return;
    });

    m_headers = getHeaders(collection.variantProperty("headers").value().toByteArray());
    m_elements = Utils::filtered(collection.allSubModelNodes(), &isListElement);
    endResetModel();
}

void SingleCollectionModel::updateCollectionName()
{
    QString newCollectionName = m_collectionNode.isValid()
                                    ? m_collectionNode.variantProperty("objectName").value().toString()
                                    : "";
    if (m_collectionName != newCollectionName) {
        m_collectionName = newCollectionName;
        emit this->collectionNameChanged(m_collectionName);
    }
}
} // namespace QmlDesigner
