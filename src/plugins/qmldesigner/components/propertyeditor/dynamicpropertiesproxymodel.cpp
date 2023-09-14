/****************************************************************************
**
** Copyright (C) 2022 The Qt Company Ltd.
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

#include "dynamicpropertiesproxymodel.h"

#include "bindingproperty.h"
#include "propertyeditorvalue.h"
#include "connectioneditorutils.h"

#include <dynamicpropertiesmodel.h>

#include <abstractproperty.h>
#include <bindingeditor.h>
#include <variantproperty.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/messagebox.h>
#include <utils/qtcassert.h>

#include <QScopeGuard>

namespace QmlDesigner {

static const int propertyNameRole = Qt::UserRole + 1;
static const int propertyTypeRole = Qt::UserRole + 2;
static const int propertyValueRole = Qt::UserRole + 3;
static const int propertyBindingRole = Qt::UserRole + 4;

DynamicPropertiesProxyModel::DynamicPropertiesProxyModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

void DynamicPropertiesProxyModel::initModel(DynamicPropertiesModel *model)
{
    m_model = model;

    connect(m_model, &QAbstractItemModel::modelAboutToBeReset,
            this, &QAbstractItemModel::modelAboutToBeReset);
    connect(m_model, &QAbstractItemModel::modelReset,
            this, &QAbstractItemModel::modelReset);

    connect(m_model, &QAbstractItemModel::rowsAboutToBeRemoved,
            this, &QAbstractItemModel::rowsAboutToBeRemoved);
    connect(m_model, &QAbstractItemModel::rowsRemoved,
            this, &QAbstractItemModel::rowsRemoved);
    connect(m_model, &QAbstractItemModel::rowsInserted,
            this, &QAbstractItemModel::rowsInserted);

    connect(m_model, &QAbstractItemModel::dataChanged,
            this, [this](const QModelIndex &topLeft, const QModelIndex &, const QList<int> &) {
                emit dataChanged(index(topLeft.row(), 0),
                                 index(topLeft.row(), 0),
                                 { propertyNameRole, propertyTypeRole,
                                   propertyValueRole, propertyBindingRole });
            });
}

int DynamicPropertiesProxyModel::rowCount(const QModelIndex &) const
{
    return m_model ? m_model->rowCount() : 0;
}

QHash<int, QByteArray> DynamicPropertiesProxyModel::roleNames() const
{
    static QHash<int, QByteArray> roleNames{{propertyNameRole,    "propertyName"},
                                            {propertyTypeRole,    "propertyType"},
                                            {propertyValueRole,   "propertyValue"},
                                            {propertyBindingRole, "propertyBinding"}};

    return roleNames;
}

QVariant DynamicPropertiesProxyModel::data(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.row() < rowCount()) {
        AbstractProperty property = m_model->propertyForRow(index.row());

        QTC_ASSERT(property.isValid(), return QVariant());

        if (role == propertyNameRole)
            return property.name();

        if (propertyTypeRole)
            return property.dynamicTypeName();

        if (role == propertyValueRole) {
            QmlObjectNode objectNode = property.parentQmlObjectNode();
            return objectNode.modelValue(property.name());
        }

        if (role == propertyBindingRole) {
            if (property.isBindingProperty())
                return property.parentQmlObjectNode().expression(property.name());

            return {};
        }

        qWarning() << Q_FUNC_INFO << "invalid role";
    } else {
        qWarning() << Q_FUNC_INFO << "invalid index";
    }

    return {};
}

void DynamicPropertiesProxyModel::registerDeclarativeType()
{
    static bool registered = false;
    if (!registered)
        qmlRegisterType<DynamicPropertiesProxyModel>("HelperWidgets", 2, 0, "DynamicPropertiesModel");
}

DynamicPropertiesModel *DynamicPropertiesProxyModel::dynamicPropertiesModel() const
{
    return m_model;
}

QString DynamicPropertiesProxyModel::newPropertyName() const
{
    DynamicPropertiesModel *propsModel = dynamicPropertiesModel();

    return QString::fromUtf8(uniquePropertyName("property", propsModel->singleSelectedNode()));
}

void DynamicPropertiesProxyModel::createProperty(const QString &name, const QString &type)
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_PROPERTY_ADDED);

    TypeName typeName = type.toUtf8();

    const auto selectedNodes = dynamicPropertiesModel()->selectedNodes();
    if (selectedNodes.size() == 1) {
        const ModelNode modelNode = selectedNodes.constFirst();
        if (modelNode.isValid()) {
            if (modelNode.hasProperty(name.toUtf8())) {
                Core::AsynchronousMessageBox::warning(tr("Property Already Exists"),
                                                      tr("Property \"%1\" already exists.")
                                                          .arg(name));
                return;
            }
            try {
                if (isDynamicVariantPropertyType(typeName)) {
                    QVariant value = defaultValueForType(typeName);
                    VariantProperty variantProp = modelNode.variantProperty(name.toUtf8());
                    variantProp.setDynamicTypeNameAndValue(typeName, value);
                } else {
                    QString expression = defaultExpressionForType(typeName);

                    BindingProperty bindingProp = modelNode.bindingProperty(name.toUtf8());
                    bindingProp.setDynamicTypeNameAndExpression(typeName, expression);
                }
            } catch (Exception &e) {
                e.showException();
            }
        }
    } else {
        qWarning() << __FUNCTION__ << ": not one node selected";
    }
}

DynamicPropertyRow::DynamicPropertyRow()
{
    m_backendValue = new PropertyEditorValue(this);

    QObject::connect(m_backendValue,
                     &PropertyEditorValue::valueChanged,
                     this,
                     [this](const QString &, const QVariant &value) { commitValue(value); });

    QObject::connect(m_backendValue,
                     &PropertyEditorValue::expressionChanged,
                     this,
                     [this](const QString &name) {
                         if (!name.isEmpty()) // If name is empty the notifer is only for QML
                             commitExpression(m_backendValue->expression());
                         else if (m_backendValue->expression().isEmpty())
                             resetValue();
                     });
}

DynamicPropertyRow::~DynamicPropertyRow()
{
    clearProxyBackendValues();
}

void DynamicPropertyRow::registerDeclarativeType()
{
    qmlRegisterType<DynamicPropertyRow>("HelperWidgets", 2, 0, "DynamicPropertyRow");
}

void DynamicPropertyRow::setRow(int r)
{
    if (m_row == r)
        return;

    m_row = r;
    setupBackendValue();
    emit rowChanged();
}

int DynamicPropertyRow::row() const
{
    return m_row;
}

void DynamicPropertyRow::setModel(DynamicPropertiesProxyModel *model)
{
    if (model == m_model)
        return;

    if (m_model) {
        disconnect(m_model, &QAbstractItemModel::dataChanged,
                   this, &DynamicPropertyRow::handleDataChanged);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &QAbstractItemModel::dataChanged,
                this, &DynamicPropertyRow::handleDataChanged);

        if (m_row != -1)
            setupBackendValue();
    }

    emit modelChanged();
}

DynamicPropertiesProxyModel *DynamicPropertyRow::model() const
{
    return m_model;
}

PropertyEditorValue *DynamicPropertyRow::backendValue() const
{
    return m_backendValue;
}

void DynamicPropertyRow::remove()
{
    m_model->dynamicPropertiesModel()->remove(m_row);
}

PropertyEditorValue *DynamicPropertyRow::createProxyBackendValue()
{
    auto *newValue = new PropertyEditorValue(this);
    m_proxyBackendValues.append(newValue);

    return newValue;
}

void DynamicPropertyRow::clearProxyBackendValues()
{
    qDeleteAll(m_proxyBackendValues);
    m_proxyBackendValues.clear();
}

void DynamicPropertyRow::setupBackendValue()
{
    if (!m_model)
        return;

    AbstractProperty property = m_model->dynamicPropertiesModel()->propertyForRow(m_row);
    if (!property.isValid())
        return;

    if (m_backendValue->name() != property.name())
        m_backendValue->setName(property.name());

    ModelNode node = property.parentModelNode();
    if (node != m_backendValue->modelNode())
        m_backendValue->setModelNode(node);

    QVariant modelValue = property.parentQmlObjectNode().modelValue(property.name());

    const bool isBound = property.parentQmlObjectNode().hasBindingProperty(property.name());

    if (modelValue != m_backendValue->value()) {
        m_backendValue->setValue({});
        m_backendValue->setValue(modelValue);
    }

    if (isBound) {
        QString expression = property.parentQmlObjectNode().expression(property.name());
        if (m_backendValue->expression() != expression)
            m_backendValue->setExpression(expression);
    }

    emit m_backendValue->isBoundChanged();
}

void DynamicPropertyRow::commitValue(const QVariant &value)
{
    if (m_lock)
        return;

    if (m_row < 0)
        return;

    if (!value.isValid())
        return;

    auto propertiesModel = m_model->dynamicPropertiesModel();
    AbstractProperty property = propertiesModel->propertyForRow(m_row);

    if (!isDynamicVariantPropertyType(property.dynamicTypeName()))
        return;

    m_lock = true;
    const QScopeGuard cleanup([this] { m_lock = false; });

    auto view = propertiesModel->view();
    RewriterTransaction transaction = view->beginRewriterTransaction(__FUNCTION__);
    try {
        if (property.isBindingProperty()) {
            convertBindingToVariantProperty(property.toBindingProperty(), value);
        } else if (property.isVariantProperty()) {
            VariantProperty variantProperty = property.toVariantProperty();
            QmlObjectNode objectNode = variantProperty.parentQmlObjectNode();
            if (view->currentState().isBaseState()
                    && !(objectNode.timelineIsActive() && objectNode.currentTimeline().isRecording())) {
                if (variantProperty.value() != value)
                    variantProperty.setDynamicTypeNameAndValue(variantProperty.dynamicTypeName(), value);
            } else {
                QTC_CHECK(objectNode.isValid());
                PropertyName name = variantProperty.name();
                if (objectNode.isValid() && objectNode.modelValue(name) != value)
                    objectNode.setVariantProperty(name, value);
            }
        }
        transaction.commit(); // committing in the try block
    } catch (Exception &e) {
        e.showException();
    }
}

void DynamicPropertyRow::commitExpression(const QString &expression)
{
    if (m_lock || m_row < 0)
        return;

    auto propertiesModel = m_model->dynamicPropertiesModel();
    AbstractProperty property = propertiesModel->propertyForRow(m_row);

    BindingProperty bindingProperty = property.parentModelNode().bindingProperty(property.name());

    const QVariant literal = BindingProperty::convertToLiteral(bindingProperty.dynamicTypeName(),
                                                               expression);

    if (literal.isValid()) { // If the string can be converted to a literal we set it as a literal/value
        commitValue(literal);
        return;
    }

    m_lock = true;
    const QScopeGuard cleanup([this] { m_lock = false; });

    auto view = propertiesModel->view();
    RewriterTransaction transaction = view->beginRewriterTransaction(__FUNCTION__);
    try {
        QString theExpression = expression;
        if (theExpression.isEmpty())
            theExpression = "null";

        if (view->currentState().isBaseState()) {
            if (bindingProperty.expression() != theExpression) {
                bindingProperty.setDynamicTypeNameAndExpression(bindingProperty.dynamicTypeName(),
                                                                theExpression);
            }
        } else {
            QmlObjectNode objectNode = bindingProperty.parentQmlObjectNode();
            QTC_CHECK(objectNode.isValid());
            PropertyName name = bindingProperty.name();
            if (objectNode.isValid() && objectNode.expression(name) != theExpression)
                objectNode.setBindingProperty(name, theExpression);
        }

        transaction.commit(); // committing in the try block
    } catch (Exception &e) {
        e.showException();
    }
}

void DynamicPropertyRow::handleDataChanged(const QModelIndex &topLeft, const QModelIndex &, const QList<int> &)
{
    if (topLeft.row() == m_row)
        setupBackendValue();
}

void DynamicPropertyRow::resetValue()
{
    if (m_lock || m_row < 0)
        return;

    auto propertiesModel = m_model->dynamicPropertiesModel();
    auto view = propertiesModel->view();

    AbstractProperty property = propertiesModel->propertyForRow(m_row);
    TypeName typeName = property.dynamicTypeName();

    if (view->currentState().isBaseState()) {
        if (isDynamicVariantPropertyType(typeName)) {
            QVariant value = defaultValueForType(typeName);
            commitValue(value);
        } else {
            QString expression = defaultExpressionForType(typeName);
            commitExpression(expression);
        }
    } else {
        m_lock = true;
        const QScopeGuard cleanup([this] { m_lock = false; });

        RewriterTransaction transaction = view->beginRewriterTransaction(__FUNCTION__);
        try {
            QmlObjectNode objectNode = property.parentQmlObjectNode();
            QTC_CHECK(objectNode.isValid());
            PropertyName name = property.name();
            if (objectNode.isValid() && objectNode.propertyAffectedByCurrentState(name))
                objectNode.removeProperty(name);

            transaction.commit(); // committing in the try block
        } catch (Exception &e) {
            e.showException();
        }
    }
}

} // namespace QmlDesigner
