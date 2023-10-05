// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "bindingmodel.h"
#include "bindingmodelitem.h"
#include "connectioneditorutils.h"
#include "connectionview.h"
#include "modelfwd.h"

#include <bindingproperty.h>
#include <nodemetainfo.h>
#include <nodeproperty.h>
#include <rewritertransaction.h>
#include <rewritingexception.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>

#include <QSignalBlocker>

namespace QmlDesigner {

BindingModel::BindingModel(ConnectionView *parent)
    : QStandardItemModel(parent)
    , m_connectionView(parent)
    , m_delegate(new BindingModelBackendDelegate(this))
{
    setHorizontalHeaderLabels(BindingModelItem::headerLabels());
}

ConnectionView *BindingModel::connectionView() const
{
    return m_connectionView;
}

BindingModelBackendDelegate *BindingModel::delegate() const
{
    return m_delegate;
}

int BindingModel::currentIndex() const
{
    return m_currentIndex;
}

BindingProperty BindingModel::currentProperty() const
{
    return propertyForRow(m_currentIndex);
}

BindingProperty BindingModel::propertyForRow(int row) const
{
    if (!m_connectionView)
        return {};

    if (!m_connectionView->isAttached())
        return {};

    if (auto *item = itemForRow(row)) {
        int internalId = item->internalId();
        if (ModelNode node = m_connectionView->modelNodeForInternalId(internalId); node.isValid())
            return node.bindingProperty(item->targetPropertyName());
    }

    return {};
}

static PropertyName unusedProperty(const ModelNode &modelNode)
{
    if (modelNode.metaInfo().isValid()) {
        for (const auto &property : modelNode.metaInfo().properties()) {
            if (property.isWritable() && !modelNode.hasProperty(property.name()))
                return property.name();
        }
    }
    return "none";
}

void BindingModel::add()
{
    if (const QList<ModelNode> nodes = connectionView()->selectedModelNodes(); nodes.size() == 1) {
        const ModelNode modelNode = nodes.constFirst();
        if (modelNode.isValid()) {
            try {
                PropertyName name = unusedProperty(modelNode);
                modelNode.bindingProperty(name).setExpression(QLatin1String("none.none"));
            } catch (RewritingException &e) {
                showErrorMessage(e.description());
                reset();
            }
        }
    } else {
        qWarning() << __FUNCTION__ << " Requires exactly one selected node";
    }
}

void BindingModel::remove(int row)
{
    if (BindingProperty property = propertyForRow(row); property.isValid()) {
        ModelNode node = property.parentModelNode();
        node.removeProperty(property.name());
    }

    reset();
}

void BindingModel::reset(const QList<ModelNode> &nodes)
{
    if (!connectionView())
        return;

    if (!connectionView()->isAttached())
        return;

    AbstractProperty current = currentProperty();

    clear();

    if (!nodes.isEmpty()) {
        for (const ModelNode &modelNode : nodes)
            addModelNode(modelNode);
    } else {
        for (const ModelNode &modelNode : connectionView()->selectedModelNodes())
            addModelNode(modelNode);
    }

    setCurrentProperty(current);
}

void BindingModel::setCurrentIndex(int i)
{
    if (m_currentIndex != i) {
        m_currentIndex = i;
        emit currentIndexChanged();
    }
    m_delegate->update(currentProperty(), m_connectionView);
}

void BindingModel::setCurrentProperty(const AbstractProperty &property)
{
    if (auto index = rowForProperty(property))
        setCurrentIndex(*index);
}

void BindingModel::updateItem(const BindingProperty &property)
{
    if (auto *item = itemForProperty(property)) {
        item->updateProperty(property);
    } else {
        ModelNode node = property.parentModelNode();
        if (connectionView()->isSelectedModelNode(node)) {
            appendRow(new BindingModelItem(property));
            setCurrentProperty(property);
        }
    }
    m_delegate->update(currentProperty(), m_connectionView);
}

void BindingModel::removeItem(const AbstractProperty &property)
{
    AbstractProperty current = currentProperty();
    if (auto index = rowForProperty(property))
        static_cast<void>(removeRow(*index));

    setCurrentProperty(current);
    emit currentIndexChanged();
}

void BindingModel::commitExpression(int row, const QString &expression)
{
    QTC_ASSERT(connectionView(), return);

    BindingProperty bindingProperty = propertyForRow(row);
    if (!bindingProperty.isValid())
        return;

    connectionView()->executeInTransaction(__FUNCTION__, [&bindingProperty, expression]() {
        if (bindingProperty.isDynamic()) {
            TypeName type = bindingProperty.dynamicTypeName();
            bindingProperty.setDynamicTypeNameAndExpression(type, expression);
        } else {
            bindingProperty.setExpression(expression.trimmed());
        }
    });
}

void BindingModel::commitPropertyName(int row, const PropertyName &name)
{
    QTC_ASSERT(connectionView(), return);

    BindingProperty bindingProperty = propertyForRow(row);
    if (!bindingProperty.isValid())
        return;

    connectionView()->executeInTransaction(__FUNCTION__, [&]() {
        const TypeName type = bindingProperty.dynamicTypeName();
        const QString expression = bindingProperty.expression();

        ModelNode node = bindingProperty.parentModelNode();
        node.removeProperty(bindingProperty.name());
        if (bindingProperty.isDynamic())
            node.bindingProperty(name).setDynamicTypeNameAndExpression(type, expression);
        else
            node.bindingProperty(name).setExpression(expression);
    });
}

QHash<int, QByteArray> BindingModel::roleNames() const
{
    return BindingModelItem::roleNames();
}

std::optional<int> BindingModel::rowForProperty(const AbstractProperty &property) const
{
    PropertyName name = property.name();
    int internalId = property.parentModelNode().internalId();

    for (int i = 0; i < rowCount(); ++i) {
        if (auto *item = itemForRow(i)) {
            if (item->targetPropertyName() == name && item->internalId() == internalId)
                return i;
        }
    }
    return std::nullopt;
}

BindingModelItem *BindingModel::itemForRow(int row) const
{
    if (QModelIndex idx = index(row, 0); idx.isValid())
        return dynamic_cast<BindingModelItem *>(itemFromIndex(idx));
    return nullptr;
}

BindingModelItem *BindingModel::itemForProperty(const AbstractProperty &property) const
{
    if (auto row = rowForProperty(property))
        return itemForRow(*row);
    return nullptr;
}

void BindingModel::addModelNode(const ModelNode &node)
{
    if (!node.isValid())
        return;

    const QList<BindingProperty> bindingProperties = node.bindingProperties();
    for (const BindingProperty &property : bindingProperties)
        appendRow(new BindingModelItem(property));
}

BindingModelBackendDelegate::BindingModelBackendDelegate(BindingModel *parent)
    : QObject(parent)
    , m_targetNode()
    , m_property()
    , m_sourceNode()
    , m_sourceNodeProperty()
{
    connect(&m_sourceNode, &StudioQmlComboBoxBackend::activated, this, [this]() {
        sourceNodeChanged();
    });

    connect(&m_sourceNodeProperty, &StudioQmlComboBoxBackend::activated, this, [this]() {
        sourcePropertyNameChanged();
    });

    connect(&m_property, &StudioQmlComboBoxBackend::activated, this, [this]() {
        targetPropertyNameChanged();
    });
}

void BindingModelBackendDelegate::update(const BindingProperty &property, AbstractView *view)
{
    if (!property.isValid())
        return;

    auto addName = [](QStringList &&list, const QString &name) {
        if (!list.contains(name))
            list.prepend(name);
        return std::move(list);
    };

    auto [sourceNodeName, sourcePropertyName] = splitExpression(property.expression());

    QStringList sourceNodes = {};
    if (!sourceNodeName.isEmpty())
        sourceNodes = addName(availableSources(view), sourceNodeName);

    m_sourceNode.setModel(sourceNodes);
    m_sourceNode.setCurrentText(sourceNodeName);

    auto availableProperties = availableSourceProperties(sourceNodeName, property, view);
    auto sourceproperties = addName(std::move(availableProperties), sourcePropertyName);
    m_sourceNodeProperty.setModel(sourceproperties);
    m_sourceNodeProperty.setCurrentText(sourcePropertyName);

    QString targetName = QString::fromUtf8(property.name());
    m_targetNode = idOrTypeName(property.parentModelNode());

    auto targetProperties = addName(availableTargetProperties(property), targetName);
    m_property.setModel(targetProperties);
    m_property.setCurrentText(targetName);

    emit targetNodeChanged();
}

QString BindingModelBackendDelegate::targetNode() const
{
    return m_targetNode;
}

StudioQmlComboBoxBackend *BindingModelBackendDelegate::property()
{
    return &m_property;
}

StudioQmlComboBoxBackend *BindingModelBackendDelegate::sourceNode()
{
    return &m_sourceNode;
}

StudioQmlComboBoxBackend *BindingModelBackendDelegate::sourceProperty()
{
    return &m_sourceNodeProperty;
}

void BindingModelBackendDelegate::sourceNodeChanged()
{
    BindingModel *model = qobject_cast<BindingModel *>(parent());
    QTC_ASSERT(model, return);

    ConnectionView *view = model->connectionView();
    QTC_ASSERT(view, return);
    QTC_ASSERT(view->isAttached(), return );

    const QString sourceNode = m_sourceNode.currentText();
    const QString sourceProperty = m_sourceNodeProperty.currentText();

    BindingProperty targetProperty = model->currentProperty();
    QStringList properties = availableSourceProperties(sourceNode, targetProperty, view);

    if (!properties.contains(sourceProperty)) {
        QSignalBlocker blocker(this);
        properties.prepend("---");
        m_sourceNodeProperty.setModel(properties);
        m_sourceNodeProperty.setCurrentText({"---"});
    }
    sourcePropertyNameChanged();
}

void BindingModelBackendDelegate::sourcePropertyNameChanged() const
{
    const QString sourceProperty = m_sourceNodeProperty.currentText();
    if (sourceProperty.isEmpty() || sourceProperty == "---")
        return;

    auto commit = [this, sourceProperty]() {
        BindingModel *model = qobject_cast<BindingModel *>(parent());
        QTC_ASSERT(model, return);

        const QString sourceNode = m_sourceNode.currentText();
        QString expression;
        if (sourceProperty.isEmpty())
            expression = sourceNode;
        else
            expression = sourceNode + QLatin1String(".") + sourceProperty;

        int row = model->currentIndex();
        model->commitExpression(row, expression);
    };

    callLater(commit);
}

void BindingModelBackendDelegate::targetPropertyNameChanged() const
{
    auto commit = [this]() {
        BindingModel *model = qobject_cast<BindingModel *>(parent());
        QTC_ASSERT(model, return);
        const PropertyName propertyName = m_property.currentText().toUtf8();
        int row = model->currentIndex();
        model->commitPropertyName(row, propertyName);
    };

    callLater(commit);
}

} // namespace QmlDesigner
