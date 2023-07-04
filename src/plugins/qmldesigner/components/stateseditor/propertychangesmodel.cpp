// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "propertychangesmodel.h"

#include "stateseditorview.h"
#include <qmlmodelnodeproxy.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QWidget>
#include <QtQml>

enum {
    debug = false
};

namespace QmlDesigner {

PropertyChangesModel::PropertyChangesModel(QObject *parent)
    : QAbstractListModel(parent)
{}

PropertyChangesModel::~PropertyChangesModel()
{
    if (m_view)
        m_view->deregisterPropertyChangesModel(this);
}

int PropertyChangesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    QmlModelState modelState(m_modelNode);

    if (!modelState.isValid() || modelState.isBaseState())
        return 0;

    return modelState.propertyChanges().size();
}

QVariant PropertyChangesModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0)
        return {};

    QmlModelState modelState(m_modelNode);
    if (!modelState.isValid() || modelState.isBaseState())
        return {};

    QList<QmlPropertyChanges> propertyChanges = modelState.propertyChanges();

    switch (role) {
    case Target: {
        const ModelNode target = propertyChanges.at(index.row()).target();
        if (target.isValid())
            return target.displayName();
        return {};
    }

    case Explicit: {
        return propertyChanges.at(index.row()).explicitValue();
    }

    case RestoreEntryValues: {
        return propertyChanges.at(index.row()).restoreEntryValues();
    }

    case PropertyModelNode: {
        return propertyChanges.at(index.row()).modelNode().toVariant();
    }
    }
    return {};
}

QHash<int, QByteArray> PropertyChangesModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Target, "target"},
                                            {Explicit, "explicit"},
                                            {RestoreEntryValues, "restoreEntryValues"},
                                            {PropertyModelNode, "propertyModelNode"}};
    return roleNames;
}

void PropertyChangesModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    ModelNode modelNode = modelNodeBackend.value<ModelNode>();

    if (!modelNode.isValid() || modelNode.isRootNode())
        return;

    m_modelNode = modelNode;

    QTC_ASSERT(m_modelNode.simplifiedTypeName() == "State", return );

    m_view = qobject_cast<StatesEditorView *>(m_modelNode.view());
    if (m_view)
        m_view->registerPropertyChangesModel(this);

    emit modelNodeBackendChanged();
    emit propertyChangesVisibleChanged();
}

void PropertyChangesModel::reset()
{
    QAbstractListModel::beginResetModel();
    QAbstractListModel::endResetModel();

    emit countChanged();
}

int PropertyChangesModel::count() const
{
    return rowCount();
}

namespace {
constexpr AuxiliaryDataKeyDefaultValue propertyChangesVisibleProperty{AuxiliaryDataType::Temporary,
                                                                      "propertyChangesVisible",
                                                                      false};
}
void PropertyChangesModel::setPropertyChangesVisible(bool value)
{
    m_modelNode.setAuxiliaryData(propertyChangesVisibleProperty, value);
}

bool PropertyChangesModel::propertyChangesVisible() const
{
    return m_modelNode.auxiliaryDataWithDefault(propertyChangesVisibleProperty).toBool();
}

void PropertyChangesModel::registerDeclarativeType()
{
    qmlRegisterType<PropertyChangesModel>("HelperWidgets", 2, 0, "PropertyChangesModel");
}

QVariant PropertyChangesModel::modelNodeBackend() const
{
    return QVariant();
}

} // namespace QmlDesigner
