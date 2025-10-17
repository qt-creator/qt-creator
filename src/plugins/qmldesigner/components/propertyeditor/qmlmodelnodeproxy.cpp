// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlmodelnodeproxy.h"
#include "abstractview.h"
#include "propertyeditortracing.h"

#include <nodemetainfo.h>

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>

#include <QtQml>

namespace QmlDesigner {

using QmlDesigner::PropertyEditorTracing::category;

QmlModelNodeProxy::QmlModelNodeProxy(QObject *parent) :
    QObject(parent)
{
    NanotraceHR::Tracer tracer{"qml model node proxy constructor", category()};
}

void QmlModelNodeProxy::setup(const ModelNode &node)
{
    NanotraceHR::Tracer tracer{"qml model node proxy setup", category()};

    setup(ModelNodes{node});
}

void QmlModelNodeProxy::setup(const ModelNodes &editorNodes)
{
    NanotraceHR::Tracer tracer{"qml model node proxy setup with editor nodes", category()};

    m_qmlObjectNode = editorNodes.isEmpty() ? ModelNode{} : editorNodes.first();
    m_editorNodes = editorNodes;

    m_subselection.clear();

    emit modelNodeChanged();
}

void QmlModelNodeProxy::registerDeclarativeType()
{
    NanotraceHR::Tracer tracer{"qml model node proxy register declarative type", category()};

    qmlRegisterType<QmlModelNodeProxy>("HelperWidgets", 2, 0, "ModelNodeProxy");
}

void QmlModelNodeProxy::emitSelectionToBeChanged()
{
    NanotraceHR::Tracer tracer{"qml model node proxy emit selection to be changed", category()};

    emit selectionToBeChanged();
}

void QmlModelNodeProxy::emitSelectionChanged()
{
    emit selectionChanged();
}

void QmlModelNodeProxy::refresh()
{
    NanotraceHR::Tracer tracer{"qml model node proxy refresh", category()};

    emit refreshRequired();
}

QmlObjectNode QmlModelNodeProxy::qmlObjectNode() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy qml object node", category()};

    return m_qmlObjectNode;
}

ModelNode QmlModelNodeProxy::modelNode() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy model node", category()};

    return m_qmlObjectNode.modelNode();
}

ModelNodes QmlModelNodeProxy::editorNodes() const
{
    return m_editorNodes;
}

ModelNode QmlModelNodeProxy::singleSelectedNode() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy single selected node", category()};

    return multiSelection() ? ModelNode{} : modelNode();
}

bool QmlModelNodeProxy::multiSelection() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy multi selection", category()};

    if (!m_qmlObjectNode.isValid())
        return false;

    return editorNodes().size() > 1;
}

QString QmlModelNodeProxy::nodeId() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy node id", category()};

    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.id();
}

QString QmlModelNodeProxy::nodeObjectName() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy node object name", category()};

    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.modelNode().variantProperty("objectName").value().toString();
}

QString QmlModelNodeProxy::simplifiedTypeName() const
{
    NanotraceHR::Tracer tracer{"qml model node proxy simplified type name", category()};

    if (!m_qmlObjectNode.isValid())
        return {};

    if (multiSelection())
        return tr("multiselection");

    return m_qmlObjectNode.simplifiedTypeName();
}

static QList<int> toInternalIdList(const QList<ModelNode> &nodes)
{
    return Utils::transform(nodes, [](const ModelNode &node) { return node.internalId(); });
}

QList<int> QmlModelNodeProxy::allChildren(int internalId) const
{
    NanotraceHR::Tracer tracer{"qml model node proxy all children", category()};

    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    return allChildren(modelNode);
}

QList<int> QmlModelNodeProxy::allChildrenOfType(const QString &typeName, int internalId) const
{
    NanotraceHR::Tracer tracer{"qml model node proxy all children of type", category()};

    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    return allChildrenOfType(modelNode, typeName);
}

QString QmlModelNodeProxy::simplifiedTypeName(int internalId) const
{
    NanotraceHR::Tracer tracer{"qml model node proxy simplified type name", category()};

    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    return modelNode.view()->modelNodeForInternalId(internalId).simplifiedDocumentTypeRepresentation();
}

PropertyEditorSubSelectionWrapper *QmlModelNodeProxy::findWrapper(int internalId) const
{
    NanotraceHR::Tracer tracer{"qml model node proxy find wrapper", category()};

    for (const auto &item : qAsConst(m_subselection)) {
        if (item->modelNode().internalId() == internalId)
            return item.data();
    }

    return nullptr;
}

PropertyEditorSubSelectionWrapper *QmlModelNodeProxy::registerSubSelectionWrapper(int internalId)
{
    NanotraceHR::Tracer tracer{"qml model node proxy register sub selection wrapper", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy create model node", category()};

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

        ModelNode newNode = view->createModelNode(typeName.toUtf8());

        parentModelNode.nodeAbstractProperty(property.toUtf8()).reparentHere(newNode);
    });
}

void QmlModelNodeProxy::moveNode(int internalIdParent,
                                 const QString &propertyName,
                                 int fromIndex,
                                 int toIndex)
{
    NanotraceHR::Tracer tracer{"qml model node proxy move node", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy is instance of", category()};

    ModelNode modelNode = m_qmlObjectNode.modelNode();

    QTC_ASSERT(modelNode.isValid(), return {});

    if (internalId >= 0)
        modelNode = modelNode.view()->modelNodeForInternalId(internalId);

    NodeMetaInfo metaInfo = modelNode.model()->metaInfo(typeName.toUtf8());

    return modelNode.metaInfo().isBasedOn(metaInfo);
}

void QmlModelNodeProxy::changeType(int internalId, const QString &typeName)
{
    NanotraceHR::Tracer tracer{"qml model node proxy change type", category()};

    QTC_ASSERT(m_qmlObjectNode.isValid(), return );

    ModelNode node = m_qmlObjectNode.view()->modelNodeForInternalId(internalId);

    QTC_ASSERT(node.isValid(), return );

    QTC_ASSERT(!node.isRootNode(), return );
    node.changeType(typeName.toUtf8());
}

void QmlModelNodeProxy::handleInstancePropertyChanged(const ModelNode &modelNode,
                                                      PropertyNameView propertyName)
{
    NanotraceHR::Tracer tracer{"qml model node proxy handle instance property changed", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy handle binding property changed", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy handle variant property changed", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy handle properties removed", category()};

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
    NanotraceHR::Tracer tracer{"qml model node proxy all children", category()};

    return toInternalIdList(modelNode.directSubModelNodes());
}

QList<int> QmlModelNodeProxy::allChildrenOfType(const ModelNode &modelNode, const QString &typeName) const
{
    NanotraceHR::Tracer tracer{"qml model node proxy all children of type", category()};

    QTC_ASSERT(modelNode.isValid(), return {});

    NodeMetaInfo metaInfo = modelNode.model()->metaInfo(typeName.toUtf8());

    return toInternalIdList(modelNode.directSubModelNodesOfType(metaInfo));
}
} // namespace QmlDesigner
