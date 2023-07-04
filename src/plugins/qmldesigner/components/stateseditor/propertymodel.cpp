// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "propertymodel.h"

#include "abstractproperty.h"
#include "abstractview.h"
#include "bindingproperty.h"
#include "nodemetainfo.h"
#include "variantproperty.h"

#include <qmlmodelnodeproxy.h>

#include <utils/qtcassert.h>

#include <QDebug>
#include <QWidget>
#include <QtQml>

enum {
    debug = false
};

namespace QmlDesigner {

PropertyModel::PropertyModel(QObject *parent)
    : QAbstractListModel(parent)
{}

int PropertyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_properties.size();
}

QVariant PropertyModel::data(const QModelIndex &index, int role) const
{
    if (index.parent().isValid() || index.column() != 0)
        return QVariant();

    switch (role) {
    case Name: {
        return m_properties.at(index.row()).name();
    }

    case Value: {
        AbstractProperty property = m_properties.at(index.row());

        if (property.isBindingProperty())
            return property.toBindingProperty().expression();

        if (property.isVariantProperty())
            return property.toVariantProperty().value();

        return {};
    }

    case Type: {
        QmlPropertyChanges propertyChanges(m_modelNode);
        if (!propertyChanges.isValid())
            return {};

        if (!propertyChanges.target().isValid())
            return {};

        return propertyChanges.target()
            .metaInfo()
            .property(m_properties.at(index.row()).name())
            .propertyType()
            .typeName();
    }
    }
    return {};
}

QHash<int, QByteArray> PropertyModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{Name, "name"}, {Value, "value"}, {Type, "type"}};
    return roleNames;
}

void PropertyModel::setModelNodeBackend(const QVariant &modelNodeBackend)
{
    ModelNode modelNode = modelNodeBackend.value<ModelNode>();

    if (!modelNode.isValid())
        return;

    m_modelNode = modelNode;

    QTC_ASSERT(m_modelNode.simplifiedTypeName() == "PropertyChanges", return );

    setupModel();
    emit modelNodeBackendChanged();
    emit expandedChanged();
}

void PropertyModel::setExplicit(bool value)
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    QmlPropertyChanges propertyChanges(m_modelNode);

    if (propertyChanges.isValid())
        propertyChanges.setExplicitValue(value);
}

void PropertyModel::setRestoreEntryValues(bool value)
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    QmlPropertyChanges propertyChanges(m_modelNode);

    if (propertyChanges.isValid())
        propertyChanges.setRestoreEntryValues(value);
}

void PropertyModel::removeProperty(const QString &name)
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    m_modelNode.removeProperty(name.toUtf8());
}

namespace {
constexpr AuxiliaryDataKeyDefaultValue expandedProperty{AuxiliaryDataType::Temporary,
                                                        "propertyModelExpanded",
                                                        false};
}

void PropertyModel::setExpanded(bool value)
{
    m_modelNode.setAuxiliaryData(expandedProperty, value);
}

bool PropertyModel::expanded() const
{
    return m_modelNode.auxiliaryDataWithDefault(expandedProperty).toBool();
}

void PropertyModel::registerDeclarativeType()
{
    qmlRegisterType<PropertyModel>("HelperWidgets", 2, 0, "PropertyModel");
}

QVariant PropertyModel::modelNodeBackend() const
{
    return QVariant();
}

void PropertyModel::setupModel()
{
    if (!m_modelNode.isValid() || !m_modelNode.view()->isAttached())
        return;

    QmlPropertyChanges propertyChanges(m_modelNode);
    if (!propertyChanges.isValid())
        return;

    m_properties = propertyChanges.targetProperties();
}

} // namespace QmlDesigner
