// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <abstractview.h>

#include <nodeabstractproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <bindingproperty.h>
#include <QDebug>

namespace QmlDesigner {
class NodeInstanceView;

template <class ViewType>
class ForwardView : public AbstractView
{
public:
    using Pointer = QPointer<ForwardView<ViewType> >;
    using ViewTypePointer = typename ViewType::Pointer;

    ForwardView(QObject *parent);

    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void nodeCreated(const ModelNode &createdNode) override;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) override;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList) override;

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl) override;

    void nodeOrderChanged(const NodeListProperty &listProperty) override;
    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;

    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;

    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) override;

protected:
    void appendView(ViewType *view);
    void removeView(ViewType *view);
    QList<ViewType*> viewList() const;
    ViewType *firstView() const;

private:
    QList<ViewTypePointer> m_targetViewList;
};

template <class ViewType>
ForwardView<ViewType>::ForwardView(QObject *parent)
    : AbstractView(parent)
{
}

template <class ViewType>
void ForwardView<ViewType>::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->modelAttached(model);
}

template <class ViewType>
void ForwardView<ViewType>::modelAboutToBeDetached(Model *model)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->modelAboutToBeDetached(model);

    AbstractView::modelAboutToBeDetached(model);
}

template <class ViewType>
void ForwardView<ViewType>::nodeCreated(const ModelNode &createdNode)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeCreated(ModelNode(createdNode, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeAboutToBeRemoved(ModelNode(removedNode, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeRemoved(ModelNode(removedNode, view.data()), NodeAbstractProperty(parentProperty, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeReparented(ModelNode(node, view.data()), NodeAbstractProperty(newPropertyParent, view.data()), NodeAbstractProperty(oldPropertyParent, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeIdChanged(ModelNode(node, view.data()), newId, oldId);
}

template <class T>
static QList<T> adjustedList(const QList<T>& oldList, AbstractView *view)
{
    QList<T> newList;

    for (const T &item : oldList)
    {
       newList.append(T(item, view));
    }

    return newList;
}

template <class ViewType>
void ForwardView<ViewType>::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->propertiesAboutToBeRemoved(adjustedList(propertyList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->propertiesRemoved(adjustedList(propertyList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->variantPropertiesChanged(adjustedList(propertyList, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->bindingPropertiesChanged(adjustedList(propertyList, view.data()), propertyChange);
}

void ForwardView::signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty> &propertyList, AbstractView::PropertyChangeFlags propertyChange)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->signalHandlerPropertiesChanged(adjustedList(propertyList, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->rootNodeTypeChanged(type, majorVersion, minorVersion);
}

template <class ViewType>
void ForwardView<ViewType>::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                          const QList<ModelNode> &lastSelectedNodeList)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->selectedNodesChanged(adjustedList(selectedNodeList, view.data()), adjustedList(lastSelectedNodeList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl)
{
    AbstractView::fileUrlChanged(oldUrl, newUrl);

    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->fileUrlChanged(oldUrl, newUrl);
}

template<class ViewType>
void ForwardView<ViewType>::nodeOrderChanged(const NodeListProperty &listProperty)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->nodeOrderChanged(NodeListProperty(listProperty, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::importChanged(const Imports &addedImports, const Imports &removedImports)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->importChanged(addedImport, removedImport);
}

template <class ViewType>
void ForwardView<ViewType>::importRemoved(const Import &import)
{
    AbstractView::importRemoved(import);

    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->importRemoved(import);
}

template <class ViewType>
void ForwardView<ViewType>::auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data)
{
    AbstractView::auxiliaryDataChanged(node, name, data);

    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->auxiliaryDataChanged(ModelNode(node, view.data()), name, data);
}

template <class ViewType>
        void ForwardView<ViewType>::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        view->scriptFunctionsChanged(node, scriptFunctionList);
}

template <class ViewType>
void ForwardView<ViewType>::appendView(ViewType *view)
{
    m_targetViewList.append(view);
}

template <class ViewType>
void ForwardView<ViewType>::removeView(ViewType *view)
{
    m_targetViewList.append(view);
}

template <class ViewType>
QList<ViewType*> ForwardView<ViewType>::viewList() const
{
    QList<ViewType*> newList;

    for (const ViewTypePointer &view : std::as_const(m_targetViewList))
        newList.append(view.data());

    return newList;
}

template <class ViewType>
ViewType *ForwardView<ViewType>::firstView() const
{
    if (m_targetViewList.isEmpty())
        return 0;

    return m_targetViewList.constFirst().data();
}


} // namespace QmlDesigner
