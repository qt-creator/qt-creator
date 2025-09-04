// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemfiltermodel.h"

#include "propertyeditortracing.h"

#include <abstractview.h>
#include <model.h>
#include <nodemetainfo.h>
#include <qmlmodelnodeproxy.h>
#include <variantproperty.h>

#include <qmldesignerutils/stringutils.h>

#include <QFileDialog>
#include <QDirIterator>
#include <QMetaEnum>

using namespace QmlDesigner;

using QmlDesigner::PropertyEditorTracing::category;

ItemFilterModel::ItemFilterModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_typeFilter("QtQuick.Item")
    , m_selectionOnly(false)
{}

void ItemFilterModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    NanotraceHR::Tracer tracer{"item filter model set model node backend", category()};

    auto modelNodeBackendObject = modelNodeBackend.value<QObject *>();

    const auto backendObjectCasted =
            qobject_cast<const QmlModelNodeProxy *>(modelNodeBackendObject);

    disconnect(m_updateConnection);
    if (backendObjectCasted) {
        m_modelNode = backendObjectCasted->qmlObjectNode().modelNode();

        m_updateConnection = connect(backendObjectCasted,
                                     &QmlModelNodeProxy::refreshRequired,
                                     this,
                                     &ItemFilterModel::setupModel);
    }

    setupModel();
    emit modelNodeBackendChanged();
}

void ItemFilterModel::setTypeFilter(const QString &filter)
{
    NanotraceHR::Tracer tracer{"item filter model set type filter", category()};

    if (m_typeFilter == filter)
        return;

    m_typeFilter = filter;
    setupModel();
    emit typeFilterChanged();
}

void ItemFilterModel::setSelectionOnly(bool value)
{
    NanotraceHR::Tracer tracer{"item filter model set selection only", category()};

    if (m_selectionOnly == value)
        return;

    m_selectionOnly = value;
    setupModel();
    emit selectionOnlyChanged();
}

void ItemFilterModel::setSelectedItems(const QStringList &selectedItems)
{
    NanotraceHR::Tracer tracer{"item filter model set selected items", category()};

    m_selectedItems = selectedItems;
    emit selectedItemsChanged();
}

void ItemFilterModel::setValidationRoles(const QStringList &validationRoles)
{
    NanotraceHR::Tracer tracer{"item filter model set validation roles", category()};

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
    NanotraceHR::Tracer tracer{"item filter model type filter", category()};

    return m_typeFilter;
}

bool ItemFilterModel::selectionOnly() const
{
    NanotraceHR::Tracer tracer{"item filter model selection only", category()};

    return m_selectionOnly;
}

QStringList ItemFilterModel::selectedItems() const
{
    NanotraceHR::Tracer tracer{"item filter model selected items", category()};

    return m_selectedItems;
}

QStringList ItemFilterModel::itemModel() const
{
    NanotraceHR::Tracer tracer{"item filter model item model", category()};

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
    NanotraceHR::Tracer tracer{"item filter model validation roles", category()};

    return m_validationRoles;
}

QStringList ItemFilterModel::validationItems() const
{
    NanotraceHR::Tracer tracer{"item filter model validation items", category()};

    return m_validationItems;
}

QVariant ItemFilterModel::modelItemData(int row) const
{
    NanotraceHR::Tracer tracer{"item filter model model item data", category()};

    QModelIndex idx = index(row, 0, {});
    if (!idx.isValid())
        return {};

    QVariantMap mapItem;

    auto insertData = [&](Role role) {
        mapItem.insert(QString::fromUtf8(roleNames().value(role)), idx.data(role));
    };

    insertData(IdRole);
    insertData(IdAndNameRole);
    insertData(NameRole);
    insertData(EnabledRole);

    return mapItem;
}

int ItemFilterModel::indexFromId(const QString &id) const
{
    NanotraceHR::Tracer tracer{"item filter model index from id", category()};

    AbstractView *view = m_modelNode.view();
    if (!view || !view->model())
        return -1;

    int idx = -1;
    for (const auto &internalId : std::as_const(m_modelInternalIds)) {
        ++idx;
        if (id == view->modelNodeForInternalId(internalId).id())
            return idx;
    }
    return -1;
}

void ItemFilterModel::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"item filter model register declarative type", category()};

    qmlRegisterType<ItemFilterModel>("HelperWidgets", 2, 0, "ItemFilterModel");
}

int ItemFilterModel::rowCount(const QModelIndex &) const
{
    NanotraceHR::Tracer tracer{"item filter model row count", category()};

    return m_modelInternalIds.size();
}

QVariant ItemFilterModel::data(const QModelIndex &index, int role) const
{
    NanotraceHR::Tracer tracer{"item filter model data", category()};

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
    NanotraceHR::Tracer tracer{"item filter model role names", category()};

    static QHash<int, QByteArray> roleNames{{IdRole, "id"},
                                            {NameRole, "name"},
                                            {IdAndNameRole, "idAndName"},
                                            {EnabledRole, "enabled"}};

    return roleNames;
}

QVariant ItemFilterModel::modelNodeBackend() const
{
    NanotraceHR::Tracer tracer{"item filter model model node backend", category()};

    return {};
}

ModelNode ItemFilterModel::modelNodeForRow(const int &row) const
{
    NanotraceHR::Tracer tracer{"item filter model model node for row", category()};

    if (row < 0 || row >= m_modelInternalIds.size())
        return {};

    AbstractView *view = m_modelNode.view();
    if (!view || !view->model())
        return {};

    return view->modelNodeForInternalId(m_modelInternalIds.at(row));
}

namespace {

NodeMetaInfo getMetaInfo(Utils::SmallStringView typeFilter, const Model *model)
{
    if (not model)
        return {};

    auto [moduleName, typeName] = StringUtils::split_last(typeFilter);

    if (not moduleName.empty()) {
        auto module = model->module(moduleName, Storage::ModuleKind::QmlLibrary);
        return model->metaInfo(module, typeName);
    }

    return model->metaInfo(typeName);
}

} // namespace

void ItemFilterModel::setupModel()
{
    NanotraceHR::Tracer tracer{"item filter model setup model", category()};

    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    beginResetModel();
    m_modelInternalIds.clear();

    const auto nodes = m_selectionOnly ? m_modelNode.view()->selectedModelNodes()
                                       : m_modelNode.view()->allModelNodes();

    auto base = getMetaInfo(m_typeFilter.toUtf8(), m_modelNode.model());

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
    NanotraceHR::Tracer tracer{"item filter model setup validation items", category()};

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
