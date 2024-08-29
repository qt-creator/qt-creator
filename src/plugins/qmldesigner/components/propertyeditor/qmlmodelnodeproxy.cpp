// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractview.h"
#include "qmlmodelnodeproxy.h"

#include <nodemetainfo.h>

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QtQml>

namespace QmlDesigner {

QmlModelNodeProxy::QmlModelNodeProxy(QObject *parent) :
    QObject(parent)
{
}

void QmlModelNodeProxy::setup(const QmlObjectNode &objectNode)
{
    m_qmlObjectNode = objectNode;

    m_subselection.clear();

    emit modelNodeChanged();
}

void QmlModelNodeProxy::registerDeclarativeType()
{
    qmlRegisterType<QmlModelNodeProxy>("HelperWidgets",2,0,"ModelNodeProxy");
}

void QmlModelNodeProxy::emitSelectionToBeChanged()
{
    emit selectionToBeChanged();
}

void QmlModelNodeProxy::emitSelectionChanged()
{
    emit selectionChanged();
}

void QmlModelNodeProxy::refresh()
{
    emit refreshRequired();
}

QmlObjectNode QmlModelNodeProxy::qmlObjectNode() const
{
    return m_qmlObjectNode;
}

ModelNode QmlModelNodeProxy::modelNode() const
{
    return m_qmlObjectNode.modelNode();
}

bool QmlModelNodeProxy::multiSelection() const
{
    if (!m_qmlObjectNode.isValid())
        return false;

    return m_qmlObjectNode.view()->selectedModelNodes().size() > 1;
}

QString QmlModelNodeProxy::nodeId() const
{
    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.id();
}

QString QmlModelNodeProxy::simplifiedTypeName() const
{
    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.simplifiedTypeName();
}

static QList<int> toInternalIdList(const QList<ModelNode> &nodes)
{
    return Utils::transform(nodes, &ModelNode::internalId);
}

QList<int> QmlModelNodeProxy::allChildren(int internalId) const
{
    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    return allChildren(modelNode);
}

QList<int> QmlModelNodeProxy::allChildrenOfType(const QString &typeName, int internalId) const
{
    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    return allChildrenOfType(modelNode, typeName);
}

QString QmlModelNodeProxy::simplifiedTypeName(int internalId) const
{
    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    return modelNode.view()->modelNodeForInternalId(internalId).simplifiedTypeName();
}

PropertyEditorSubSelectionWrapper *QmlModelNodeProxy::findWrapper(int internalId) const
{
    for (const auto &item : qAsConst(m_subselection)) {
        if (item->modelNode().internalId() == internalId)
            return item.data();
    }

    return nullptr;
}

PropertyEditorSubSelectionWrapper *QmlModelNodeProxy::registerSubSelectionWrapper(int internalId)
{
    auto result = findWrapper(internalId);

    if (result)
        return result;

    QTC_ASSERT(m_qmlObjectNode.isValid(), return nullptr);

    ModelNode node = m_qmlObjectNode.view()->modelNodeForInternalId(internalId);

    QTC_ASSERT(node.isValid(), return nullptr);

    QSharedPointer<PropertyEditorSubSelectionWrapper> wrapper(
        new PropertyEditorSubSelectionWrapper(node));
    m_subselection.append(wrapper);

    QJSEngine::setObjectOwnership(wrapper.data(), QJSEngine::CppOwnership);

    return wrapper.data();
}

void QmlModelNodeProxy::createModelNode(int internalIdParent,
                                        const QString &property,
                                        const QString &typeName,
                                        const QString &requiredImport)
{
    auto parentModelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(parentModelNode.isValid(), return );

    AbstractView *view = parentModelNode.view();

    if (internalIdParent >= 0)
        parentModelNode = view->modelNodeForInternalId(internalIdParent);

    QTC_ASSERT(parentModelNode.isValid(), return );

    Import import;
    Q_ASSERT(import.isEmpty());

    if (!requiredImport.isEmpty() && !view->model()->hasImport(requiredImport))
        import = Import::createLibraryImport(requiredImport);

    view->executeInTransaction("QmlModelNodeProxy::createModelNode", [&] {
        if (!import.isEmpty())
            view->model()->changeImports({import}, {});

#ifdef QDS_USE_PROJECTSTORAGE
        ModelNode newNode = view->createModelNode(typeName.toUtf8());
#else
        NodeMetaInfo metaInfo = parentModelNode.model()->metaInfo(typeName.toUtf8());
        ModelNode newNode = view->createModelNode(metaInfo.typeName(),
                                                  metaInfo.majorVersion(),
                                                  metaInfo.minorVersion());
#endif
        parentModelNode.nodeAbstractProperty(property.toUtf8()).reparentHere(newNode);
    });
}

void QmlModelNodeProxy::moveNode(int internalIdParent,
                                 const QString &propertyName,
                                 int fromIndex,
                                 int toIndex)
{
    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return );

    if (internalIdParent >= 0)
        modelNode = m_qmlObjectNode.view()->modelNodeForInternalId(internalIdParent);

    QTC_ASSERT(modelNode.isValid(), return );
    AbstractView *view = m_qmlObjectNode.view();
    view->executeInTransaction("QmlModelNodeProxy::moveNode", [&] {
        modelNode.nodeListProperty(propertyName.toUtf8()).slide(fromIndex, toIndex);
    });
}

bool QmlModelNodeProxy::isInstanceOf(const QString &typeName, int internalId) const
{
    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    NodeMetaInfo metaInfo = modelNode.model()->metaInfo(typeName.toUtf8());

    return modelNode.metaInfo().isBasedOn(metaInfo);
}

void QmlModelNodeProxy::changeType(int internalId, const QString &typeName)
{
    QTC_ASSERT(m_qmlObjectNode.isValid(), return );

    ModelNode node = m_qmlObjectNode.view()->modelNodeForInternalId(internalId);

    QTC_ASSERT(node.isValid(), return );

    QTC_ASSERT(!node.isRootNode(), return );
#ifdef QDS_USE_PROJECTSTORAGE
    node.changeType(typeName.toUtf8(), -1, -1);
#else
    NodeMetaInfo metaInfo = node.model()->metaInfo(typeName.toUtf8());
    node.changeType(metaInfo.typeName(), metaInfo.majorVersion(), metaInfo.minorVersion());
#endif
}

void QmlModelNodeProxy::handleInstancePropertyChanged(const ModelNode &modelNode,
                                                      PropertyNameView propertyName)
{
    const QmlObjectNode qmlObjectNode(modelNode);

    for (const auto &item : qAsConst(m_subselection)) {
        if (item && item->isRelevantModelNode(modelNode)) {
            if (!modelNode.hasProperty(propertyName)
                || modelNode.property(propertyName).isBindingProperty()) {
                item->setValueFromModel(propertyName, qmlObjectNode.instanceValue(propertyName));
            } else {
                item->setValueFromModel(propertyName, qmlObjectNode.modelValue(propertyName));
            }
        }
    }
}

void QmlModelNodeProxy::handleBindingPropertyChanged(const BindingProperty &property)
{
    for (const auto &item : qAsConst(m_subselection)) {
        if (item && item->isRelevantModelNode(property.parentModelNode())) {
            QmlObjectNode objectNode(item->modelNode());
            if (objectNode.modelNode().property(property.name()).isBindingProperty())
                item->setValueFromModel(property.name(), objectNode.instanceValue(property.name()));
            else
                item->setValueFromModel(property.name(), objectNode.modelValue(property.name()));
        }
    }
}

void QmlModelNodeProxy::handleVariantPropertyChanged(const VariantProperty &property)
{
    for (const auto &item : qAsConst(m_subselection)) {
        if (item && item->isRelevantModelNode(property.parentModelNode())) {
            QmlObjectNode objectNode(item->modelNode());
            if (objectNode.modelNode().property(property.name()).isBindingProperty())
                item->setValueFromModel(property.name(), objectNode.instanceValue(property.name()));
            else
                item->setValueFromModel(property.name(), objectNode.modelValue(property.name()));
        }
    }
}

void QmlModelNodeProxy::handlePropertiesRemoved(const AbstractProperty &property)
{
    for (const auto &item : qAsConst(m_subselection)) {
        if (item && item->isRelevantModelNode(property.parentModelNode())) {
            QmlObjectNode objectNode(item->modelNode());
            item->resetValue(property.name());
            item->setValueFromModel(property.name(), objectNode.instanceValue(property.name()));
        }
    }
}

QList<int> QmlModelNodeProxy::allChildren(const ModelNode &modelNode) const
{
    return toInternalIdList(modelNode.directSubModelNodes());
}

QList<int> QmlModelNodeProxy::allChildrenOfType(const ModelNode &modelNode, const QString &typeName) const
{
    QTC_ASSERT(modelNode.isValid(), return {});

    NodeMetaInfo metaInfo = modelNode.model()->metaInfo(typeName.toUtf8());

    return toInternalIdList(modelNode.directSubModelNodesOfType(metaInfo));
}
} // namespace QmlDesigner
