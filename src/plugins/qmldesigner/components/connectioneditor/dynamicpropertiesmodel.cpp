// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "dynamicpropertiesmodel.h"
#include "dynamicpropertiesitem.h"
#include "connectioneditorutils.h"

#include <abstractproperty.h>
#include <bindingproperty.h>
#include <modelfwd.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <variantproperty.h>
#include <qmlchangeset.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlobjectnode.h>
#include <qmltimeline.h>

#include <optional>

namespace QmlDesigner {

DynamicPropertiesModel::DynamicPropertiesModel(bool exSelection, AbstractView *parent)
    : QStandardItemModel(parent)
    , m_view(parent)
    , m_delegate(new DynamicPropertiesModelBackendDelegate(this))
    , m_explicitSelection(exSelection)
{
    setHorizontalHeaderLabels(DynamicPropertiesItem::headerLabels());
}

AbstractView *DynamicPropertiesModel::view() const
{
    return m_view;
}

DynamicPropertiesModelBackendDelegate *DynamicPropertiesModel::delegate() const
{
    return m_delegate;
}

int DynamicPropertiesModel::currentIndex() const
{
    return m_currentIndex;
}

AbstractProperty DynamicPropertiesModel::currentProperty() const
{
    return propertyForRow(m_currentIndex);
}

void DynamicPropertiesModel::add()
{
    QmlDesignerPlugin::emitUsageStatistics(Constants::EVENT_PROPERTY_ADDED);

    if (const QList<ModelNode> nodes = selectedNodes(); nodes.size() == 1) {
        const ModelNode modelNode = nodes.constFirst();
        if (!modelNode.isValid())
            return;

        try {
            PropertyName newName = uniquePropertyName("property", modelNode);
            VariantProperty newProperty = modelNode.variantProperty(newName);
            newProperty.setDynamicTypeNameAndValue("string", "This is a string");
        } catch (RewritingException &e) {
            showErrorMessage(e.description());
        }
    } else {
        qWarning() << "DynamicPropertiesModel::add not one node selected";
    }
}

void DynamicPropertiesModel::remove(int row)
{
    m_view->executeInTransaction(__FUNCTION__, [this, row]() {
        if (DynamicPropertiesItem *item = itemForRow(row)) {
            PropertyName name = item->propertyName();
            if (ModelNode node = modelNodeForItem(item); node.isValid()) {
                node.removeProperty(name);

                QmlObjectNode objectNode = QmlObjectNode(node);
                const auto stateOperations = objectNode.allAffectingStatesOperations();
                for (const QmlModelStateOperation &stateOperation : stateOperations) {
                    if (stateOperation.modelNode().hasProperty(name))
                        stateOperation.modelNode().removeProperty(name);
                }
                for (auto &timelineNode : objectNode.allTimelines()) {
                    QmlTimeline timeline(timelineNode);
                    timeline.removeKeyframesForTargetAndProperty(node, name);
                }
            }
        }
    });
    reset();
}

void DynamicPropertiesModel::reset(const QList<ModelNode> &modelNodes)
{
    AbstractProperty current = currentProperty();

    clear();

    if (!modelNodes.isEmpty()) {
        for (const ModelNode &modelNode : modelNodes)
            addModelNode(modelNode);
        return;
    }

    if (m_view->isAttached()) {
        const QList<ModelNode> selected = selectedNodes();
        for (const ModelNode &modelNode : selected)
            addModelNode(modelNode);
    }

    setCurrentProperty(current);
}

void DynamicPropertiesModel::setCurrentIndex(int i)
{
    if (m_currentIndex != i) {
        m_currentIndex = i;
        emit currentIndexChanged();
    }
    // Property properties may have changed.
    m_delegate->update(currentProperty());
}

void DynamicPropertiesModel::setCurrentProperty(const AbstractProperty &property)
{
    if (!property.isValid())
        return;

    if (auto index = findRow(property.parentModelNode().internalId(), property.name()))
        setCurrentIndex(*index);
}

void DynamicPropertiesModel::setCurrent(int internalId, const PropertyName &name)
{
    if (internalId < 0)
        return;

    if (auto index = findRow(internalId, name))
        setCurrentIndex(*index);
}

void DynamicPropertiesModel::updateItem(const AbstractProperty &property)
{
    if (!property.isDynamic())
        return;

    if (auto *item = itemForProperty(property)) {
        item->updateProperty(property);
    } else {
        ModelNode node = property.parentModelNode();
        if (selectedNodes().contains(node)) {
            addProperty(property);
            setCurrentProperty(property);
        }
    }
}

void DynamicPropertiesModel::removeItem(const AbstractProperty &property)
{
    if (!property.isValid())
        return;

    AbstractProperty current = currentProperty();

    if (auto index = findRow(property.parentModelNode().internalId(), property.name()))
        static_cast<void>(removeRow(*index));

    setCurrentProperty(current);
}

QHash<int, QByteArray> DynamicPropertiesModel::roleNames() const
{
    return DynamicPropertiesItem::roleNames();
}

AbstractProperty DynamicPropertiesModel::propertyForRow(int row) const
{
    if (!m_view)
        return {};

    if (!m_view->isAttached())
        return {};

    if (auto *item = itemForRow(row)) {
        int internalId = item->internalId();
        if (ModelNode node = m_view->modelNodeForInternalId(internalId); node.isValid())
            return node.property(item->propertyName());
    }
    return {};
}

std::optional<int> DynamicPropertiesModel::findRow(int nodeId, const PropertyName &name) const
{
    for (int i = 0; i < rowCount(); ++i) {
        if (auto *item = itemForRow(i)) {
            if (item->propertyName() == name && item->internalId() == nodeId)
                return i;
        }
    }
    return std::nullopt;
}

DynamicPropertiesItem *DynamicPropertiesModel::itemForRow(int row) const
{
    if (QModelIndex idx = index(row, 0); idx.isValid())
        return dynamic_cast<DynamicPropertiesItem *>(itemFromIndex(idx));
    return nullptr;
}

DynamicPropertiesItem *DynamicPropertiesModel::itemForProperty(const AbstractProperty &property) const
{
    if (!property.isValid())
        return nullptr;

    if (auto row = findRow(property.parentModelNode().internalId(), property.name()))
        return itemForRow(*row);

    return nullptr;
}

ModelNode DynamicPropertiesModel::modelNodeForItem(DynamicPropertiesItem *item)
{
    if (!m_view->isAttached())
        return {};

    return m_view->modelNodeForInternalId(item->internalId());
}

void DynamicPropertiesModel::addModelNode(const ModelNode &node)
{
    if (!node.isValid())
        return;

    for (const AbstractProperty &property : dynamicPropertiesFromNode(node))
        addProperty(property);
}

void DynamicPropertiesModel::addProperty(const AbstractProperty &property)
{
    const PropertyName name = property.name();
    for (int i = 0; i < rowCount(); ++i) {
        if (auto *item = itemForRow(i)) {
            if (item->propertyName() > name) {
                insertRow(i, new DynamicPropertiesItem(property));
                return;
            }
        }
    }
    appendRow(new DynamicPropertiesItem(property));
}

void DynamicPropertiesModel::commitPropertyType(int row, const TypeName &type)
{
    AbstractProperty property = propertyForRow(row);
    if (!property.isValid())
        return;

    ModelNode node = property.parentModelNode();
    RewriterTransaction transaction = m_view->beginRewriterTransaction(__FUNCTION__);
    try {
        if (property.isBindingProperty()) {
            BindingProperty binding = property.toBindingProperty();
            const QString expression = binding.expression();
            binding.parentModelNode().removeProperty(binding.name());
            binding.setDynamicTypeNameAndExpression(type, expression);
        } else if (property.isVariantProperty()) {
            VariantProperty variant = property.toVariantProperty();
            QVariant val = typeConvertVariant(variant.value(), type);
            variant.parentModelNode().removeProperty(variant.name());
            variant.setDynamicTypeNameAndValue(type, val);
        }
        transaction.commit();

    } catch (Exception &e) {
        showErrorMessage(e.description());
    }
}

void DynamicPropertiesModel::commitPropertyName(int row, const PropertyName &name)
{
    AbstractProperty property = propertyForRow(row);
    if (!property.isValid())
        return;

    ModelNode node = property.parentModelNode();
    if (property.isBindingProperty()) {
        BindingProperty binding = property.toBindingProperty();
        m_view->executeInTransaction(__FUNCTION__, [binding, name, &node]() {
            const QString expression = binding.expression();
            const TypeName type = binding.dynamicTypeName();
            node.removeProperty(binding.name());
            node.bindingProperty(name).setDynamicTypeNameAndExpression(type, expression);
        });

    } else if (property.isVariantProperty()) {
        VariantProperty variant = property.toVariantProperty();
        m_view->executeInTransaction(__FUNCTION__, [variant, name, &node]() {
            const QVariant value = variant.value();
            const TypeName type = variant.dynamicTypeName();
            node.removeProperty(variant.name());
            node.variantProperty(name).setDynamicTypeNameAndValue(type, value);
        });
    }
}

void DynamicPropertiesModel::commitPropertyValue(int row, const QVariant &value)
{
    AbstractProperty property = propertyForRow(row);
    if (!property.isValid())
        return;

    RewriterTransaction transaction = m_view->beginRewriterTransaction(__FUNCTION__);
    try {
        bool isBindingValue = isBindingExpression(value);
        if (property.isBindingProperty()) {
            BindingProperty binding = property.toBindingProperty();
            if (!isBindingValue) {
                convertBindingToVariantProperty(binding, value);
            } else {
                const QString expression = value.toString();
                const TypeName typeName = binding.dynamicTypeName();
                binding.setDynamicTypeNameAndExpression(typeName, expression);
            }
        } else if (property.isVariantProperty()) {
            VariantProperty variant = property.toVariantProperty();
            if (isBindingValue)
                convertVariantToBindingProperty(variant, value);
            else
                variant.setDynamicTypeNameAndValue(variant.dynamicTypeName(), value);
        }
        transaction.commit();
    } catch (Exception &e) {
        showErrorMessage(e.description());
    }
}

void DynamicPropertiesModel::dispatchPropertyChanges(const AbstractProperty &abstractProperty)
{
    if (abstractProperty.parentModelNode().simplifiedTypeName() == "PropertyChanges") {
        QmlPropertyChanges changes(abstractProperty.parentModelNode());
        if (changes.target().isValid()) {
            const ModelNode target = changes.target();
            const PropertyName propertyName = abstractProperty.name();
            const AbstractProperty targetProperty = target.variantProperty(propertyName);
            if (target.hasProperty(propertyName) && targetProperty.isDynamic())
                updateItem(targetProperty);
        }
    }
}

const QList<ModelNode> DynamicPropertiesModel::selectedNodes() const
{
    if (m_explicitSelection)
        return m_selectedNodes;

    return m_view->selectedModelNodes();
}

const ModelNode DynamicPropertiesModel::singleSelectedNode() const
{
    if (m_explicitSelection)
        return m_selectedNodes.first();

    return m_view->singleSelectedModelNode();
}

void DynamicPropertiesModel::setSelectedNode(const ModelNode &node)
{
    QTC_ASSERT(m_explicitSelection, return);

    if (!node.isValid())
        return;

    m_selectedNodes.clear();
    m_selectedNodes.append(node);
    reset();
}

DynamicPropertiesModelBackendDelegate::DynamicPropertiesModelBackendDelegate(DynamicPropertiesModel *parent)
    : QObject(parent)
    , m_internalNodeId(std::nullopt)
{
    m_type.setModel({"int", "bool", "var", "real", "string", "url", "color"});
    connect(&m_type, &StudioQmlComboBoxBackend::activated, this, [this]() { handleTypeChanged(); });
    connect(&m_name, &StudioQmlTextBackend::activated, this, [this]() { handleNameChanged(); });
    connect(&m_value, &StudioQmlTextBackend::activated, this, [this]() { handleValueChanged(); });
}

void DynamicPropertiesModelBackendDelegate::update(const AbstractProperty &property)
{
    if (!property.isValid())
        return;

    m_internalNodeId = property.parentModelNode().internalId();
    m_type.setCurrentText(QString::fromUtf8(property.dynamicTypeName()));
    m_name.setText(QString::fromUtf8(property.name()));

    if (property.isVariantProperty())
        m_value.setText(property.toVariantProperty().value().toString());
    else if (property.isBindingProperty())
        m_value.setText(property.toBindingProperty().expression());

    m_targetNode = property.parentModelNode().id();
    emit targetNodeChanged();
}

void DynamicPropertiesModelBackendDelegate::handleTypeChanged()
{
    DynamicPropertiesModel *model = qobject_cast<DynamicPropertiesModel *>(parent());
    QTC_ASSERT(model, return);

    const PropertyName name = m_name.text().toUtf8();

    int current = model->currentIndex();
    const TypeName type = m_type.currentText().toUtf8();
    model->commitPropertyType(current, type);

    // The order might have changed!
    model->setCurrent(m_internalNodeId.value_or(-1), name);
}

void DynamicPropertiesModelBackendDelegate::handleNameChanged()
{
    DynamicPropertiesModel *model = qobject_cast<DynamicPropertiesModel *>(parent());
    QTC_ASSERT(model, return);

    const PropertyName name = m_name.text().toUtf8();
    QTC_ASSERT(!name.isEmpty(), return);

    int current = model->currentIndex();
    model->commitPropertyName(current, name);

    // The order might have changed!
    model->setCurrent(m_internalNodeId.value_or(-1), name);
}

// TODO: Maybe replace with utils typeConvertVariant?
QVariant valueFromText(const QString &value, const QString &type)
{
    if (isBindingExpression(value))
        return value;

    if (type == "real" || type == "int")
        return value.toFloat();

    if (type == "bool")
        return value == "true";

    return value;
}

void DynamicPropertiesModelBackendDelegate::handleValueChanged()
{
    DynamicPropertiesModel *model = qobject_cast<DynamicPropertiesModel *>(parent());
    QTC_ASSERT(model, return);

    int current = model->currentIndex();
    QVariant value = valueFromText(m_value.text(), m_type.currentText());
    model->commitPropertyValue(current, value);
}

QString DynamicPropertiesModelBackendDelegate::targetNode() const
{
    return m_targetNode;
}

StudioQmlComboBoxBackend *DynamicPropertiesModelBackendDelegate::type()
{
    return &m_type;
}

StudioQmlTextBackend *DynamicPropertiesModelBackendDelegate::name()
{
    return &m_name;
}

StudioQmlTextBackend *DynamicPropertiesModelBackendDelegate::value()
{
    return &m_value;
}

} // namespace QmlDesigner
