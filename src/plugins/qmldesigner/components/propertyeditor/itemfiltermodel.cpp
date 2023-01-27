// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "itemfiltermodel.h"

#include <abstractview.h>
#include <model.h>
#include <nodemetainfo.h>
#include <qmlmodelnodeproxy.h>
#include "variantproperty.h"

#include <QFileDialog>
#include <QDirIterator>
#include <QMetaEnum>

using namespace QmlDesigner;

QHash<int, QByteArray> ItemFilterModel::m_roles;

ItemFilterModel::ItemFilterModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_typeFilter("QtQuick.Item")
    , m_selectionOnly(false)
{
    if (m_roles.empty()) {
        m_roles = QAbstractListModel::roleNames();
        QMetaEnum roleEnum = QMetaEnum::fromType<Roles>();
        for (int i = 0; i < roleEnum.keyCount(); i++)
            m_roles.insert(roleEnum.value(i), roleEnum.key(i));
    }
}

void ItemFilterModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{

    auto modelNodeBackendObject = modelNodeBackend.value<QObject*>();

    const auto backendObjectCasted =
            qobject_cast<const QmlModelNodeProxy *>(modelNodeBackendObject);

    if (backendObjectCasted)
        m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();

    setupModel();
    emit modelNodeBackendChanged();
}

void ItemFilterModel::setTypeFilter(const QString &filter)
{
    if (m_typeFilter != filter) {
        m_typeFilter = filter;
        setupModel();
    }
}

void ItemFilterModel::setSelectionOnly(bool value)
{
    if (m_selectionOnly != value) {
        m_selectionOnly = value;
        setupModel();
    }
}

QString ItemFilterModel::typeFilter() const
{
    return m_typeFilter;
}

bool ItemFilterModel::selectionOnly() const
{
    return m_selectionOnly;
}

void ItemFilterModel::registerDeclarativeType()
{
    qmlRegisterType<ItemFilterModel>("HelperWidgets",2,0,"ItemFilterModel");
}

QModelIndex ItemFilterModel::index(int row, int column, const QModelIndex &parent) const
{
    return QAbstractListModel::index(row, column, parent);
}

int ItemFilterModel::rowCount(const QModelIndex &) const
{
    return m_modelInternalIds.size();
}

QVariant ItemFilterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return {};

    ModelNode node = modelNodeForRow(index.row());

    QVariant value;
    switch (role) {
    case IdRole:
        value = node.id();
        break;
    case NameRole:
        value = node.variantProperty("objectName").value();
        break;
    case IdAndNameRole:
        value = QString("%1 [%2]").arg(
                    node.variantProperty("objectName").value().toString()
                    ,node.id());
        break;
    default:
        value = node.id();
        break;
    }

    return value;
}

// TODO: Handle model data manipulation here.
bool ItemFilterModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    return QAbstractListModel::setData(index, value, role);
}

QHash<int, QByteArray> ItemFilterModel::roleNames() const
{
    return m_roles;
}

QVariant ItemFilterModel::modelNodeBackend() const
{
    return {};
}

ModelNode ItemFilterModel::modelNodeForRow(const int &row) const
{
    if (row < 0 || row >= m_modelInternalIds.size())
        return {};

    AbstractView *view = m_modelNode.view();
    if (!view || !view->model())
        return {};

    return view->modelNodeForInternalId(m_modelInternalIds.at(row));
}

void ItemFilterModel::setupModel()
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    beginResetModel();
    m_modelInternalIds.clear();

    const auto nodes = m_selectionOnly ? m_modelNode.view()->selectedModelNodes()
                                       : m_modelNode.view()->allModelNodes();

    auto base = m_modelNode.model()->metaInfo(m_typeFilter.toUtf8());
    for (const QmlDesigner::ModelNode &node : nodes) {
        if (node.hasId() && node.metaInfo().isBasedOn(base))
            m_modelInternalIds.append(node.internalId());
    }

    endResetModel();
    emit itemModelChanged();
}

QStringList ItemFilterModel::itemModel() const
{
    AbstractView *view = m_modelNode.view();
    if (!view || !view->model())
        return {};

    QStringList retval;
    for (const auto &internalId : std::as_const(m_modelInternalIds))
        retval << view->modelNodeForInternalId(internalId).id();

    return retval;
}
