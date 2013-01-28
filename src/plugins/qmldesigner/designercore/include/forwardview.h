/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORWARDVIEW_H
#define FORWARDVIEW_H

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
    typedef QWeakPointer<ForwardView> Pointer;
    typedef typename ViewType::Pointer ViewTypePointer;

    ForwardView(QObject *parent);

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    void nodeCreated(const ModelNode &createdNode);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);

    void fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl);

    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports);

    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);

    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);

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
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->modelAttached(model);
}

template <class ViewType>
void ForwardView<ViewType>::modelAboutToBeDetached(Model *model)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->modelAboutToBeDetached(model);

    AbstractView::modelAboutToBeDetached(model);
}

template <class ViewType>
void ForwardView<ViewType>::nodeCreated(const ModelNode &createdNode)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeCreated(ModelNode(createdNode, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeAboutToBeRemoved(ModelNode(removedNode, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeRemoved(ModelNode(removedNode, view.data()), NodeAbstractProperty(parentProperty, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, PropertyChangeFlags propertyChange)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeReparented(ModelNode(node, view.data()), NodeAbstractProperty(newPropertyParent, view.data()), NodeAbstractProperty(oldPropertyParent, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeIdChanged(ModelNode(node, view.data()), newId, oldId);
}

template <class T>
static QList<T> adjustedList(const QList<T>& oldList, AbstractView *view)
{
    QList<T> newList;

    foreach (const T &item, oldList)
    {
       newList.append(T(item, view));
    }

    return newList;
}

template <class ViewType>
void ForwardView<ViewType>::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->propertiesAboutToBeRemoved(adjustedList(propertyList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::propertiesRemoved(const QList<AbstractProperty>& propertyList)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->propertiesRemoved(adjustedList(propertyList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->variantPropertiesChanged(adjustedList(propertyList, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->bindingPropertiesChanged(adjustedList(propertyList, view.data()), propertyChange);
}

template <class ViewType>
void ForwardView<ViewType>::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->rootNodeTypeChanged(type, majorVersion, minorVersion);
}

template <class ViewType>
void ForwardView<ViewType>::selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                          const QList<ModelNode> &lastSelectedNodeList)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->selectedNodesChanged(adjustedList(selectedNodeList, view.data()), adjustedList(lastSelectedNodeList, view.data()));
}

template <class ViewType>
void ForwardView<ViewType>::fileUrlChanged(const QUrl &oldUrl, const QUrl &newUrl)
{
    AbstractView::fileUrlChanged(oldUrl, newUrl);

    foreach (const ViewTypePointer &view, m_targetViewList)
        view->fileUrlChanged(oldUrl, newUrl);
}

template <class ViewType>
void ForwardView<ViewType>::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->nodeOrderChanged(NodeListProperty(listProperty, view.data()),
                                ModelNode(movedNode, view.data()), oldIndex);
}

template <class ViewType>
void ForwardView<ViewType>::importChanged(const QList<Import> &addedImports, const QList<Import> &removedImports)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
        view->importChanged(addedImport, removedImport);
}

template <class ViewType>
void ForwardView<ViewType>::importRemoved(const Import &import)
{
    AbstractView::importRemoved(import);

    foreach (const ViewTypePointer &view, m_targetViewList)
        view->importRemoved(import);
}

template <class ViewType>
void ForwardView<ViewType>::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    AbstractView::auxiliaryDataChanged(node, name, data);

    foreach (const ViewTypePointer &view, m_targetViewList)
        view->auxiliaryDataChanged(ModelNode(node, view.data()), name, data);
}

template <class ViewType>
        void ForwardView<ViewType>::scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList)
{
    foreach (const ViewTypePointer &view, m_targetViewList)
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

    foreach (const ViewTypePointer &view, m_targetViewList)
        newList.append(view.data());

    return newList;
}

template <class ViewType>
ViewType *ForwardView<ViewType>::firstView() const
{
    if (m_targetViewList.isEmpty())
        return 0;

    return m_targetViewList.first().data();
}


} // namespace QmlDesigner

#endif // FORWARDVIEW_H
