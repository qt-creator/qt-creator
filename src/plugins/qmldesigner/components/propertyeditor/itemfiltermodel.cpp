// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemfiltermodel.h"

#include <abstractview.h>
#include <model.h>
#include <nodemetainfo.h>
#include <qmlmodelnodeproxy.h>
#include <variantproperty.h>

#include <QFileDialog>
#include <QDirIterator>
#include <QMetaEnum>

using namespace QmlDesigner;

ItemFilterModel::ItemFilterModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_typeFilter("QtQuick.Item")
    , m_selectionOnly(false)
{}

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
    if (m_typeFilter == filter)
        return;

    m_typeFilter = filter;
    setupModel();
    emit typeFilterChanged();
}

void ItemFilterModel::setSelectionOnly(bool value)
{
    if (m_selectionOnly == value)
        return;

    m_selectionOnly = value;
    setupModel();
    emit selectionOnlyChanged();
}

void ItemFilterModel::setSelectedItems(const QStringList &selectedItems)
{
    m_selectedItems = selectedItems;
    emit selectedItemsChanged();
}

void ItemFilterModel::setValidationRoles(const QStringList &validationRoles)
{
    auto tmp = validationRoles;
    tmp.removeDuplicates();

    if (m_validationRoles == tmp)
        return;

    m_validationRoles = tmp;
    setupValidationItems();
    emit validationRolesChanged();
}

QString ItemFilterModel::typeFilter() const
{
    return m_typeFilter;
}

bool ItemFilterModel::selectionOnly() const
{
    return m_selectionOnly;
}

QStringList ItemFilterModel::selectedItems() const
{
    return m_selectedItems;
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

QStringList ItemFilterModel::validationRoles() const
{
    return m_validationRoles;
}

QStringList ItemFilterModel::validationItems() const
{
    return m_validationItems;
}

void ItemFilterModel::registerDeclarativeType()
{
    qmlRegisterType<ItemFilterModel>("HelperWidgets", 2, 0, "ItemFilterModel");
}

int ItemFilterModel::rowCount(const QModelIndex &) const
{
    return m_modelInternalIds.size();
}

QVariant ItemFilterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount())
        return {};

    const ModelNode node = modelNodeForRow(index.row());

    switch (role) {
    case IdRole:
        return node.id();
    case NameRole:
        return node.variantProperty("objectName").value();
    case IdAndNameRole:
        return QString("%1 [%2]").arg(node.variantProperty("objectName").value().toString(),
                                      node.id());
    case EnabledRole:
        return !m_selectedItems.contains(node.id());
    default:
        return {};
    }
}

QHash<int, QByteArray> ItemFilterModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{IdRole, "id"},
                                            {NameRole, "name"},
                                            {IdAndNameRole, "idAndName"},
                                            {EnabledRole, "enabled"}};

    return roleNames;
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

    setupValidationItems();
}

void ItemFilterModel::setupValidationItems()
{
    QStringList validationItems;

    for (const QString &role : m_validationRoles) {
        int r = roleNames().key(role.toUtf8(), -1);

        if (r == -1)
            continue;

        for (int i = 0; i < rowCount(); ++i) {
            QVariant item = data(index(i), r);
            if (item.canConvert<QString>())
                validationItems.append(item.toString());
        }
    }

    validationItems.removeDuplicates();

    if (m_validationItems == validationItems)
        return;

    m_validationItems = validationItems;

    emit validationItemsChanged();
}
