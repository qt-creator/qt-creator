/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"

#include <nodeproperty.h>
#include <QHeaderView>


namespace QmlDesigner {

NavigatorView::NavigatorView(QObject* parent) :
        QmlModelView(parent),
        m_blockSelectionChangedSignal(false),
        m_widget(new NavigatorWidget),
        m_treeModel(new NavigatorTreeModel(this))
{
    m_widget->setTreeModel(m_treeModel.data());

    connect(treeWidget()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(changeSelection(QItemSelection,QItemSelection)));
    treeWidget()->setIndentation(treeWidget()->indentation() * 0.5);

    IdItemDelegate *idDelegate = new IdItemDelegate(this,m_treeModel.data());
    IconCheckboxItemDelegate *showDelegate = new IconCheckboxItemDelegate(this,":/qmldesigner/images/eye_open.png",
                                                          ":/qmldesigner/images/placeholder.png",m_treeModel.data());

#ifdef _LOCK_ITEMS_
    IconCheckboxItemDelegate *lockDelegate = new IconCheckboxItemDelegate(this,":/qmldesigner/images/lock.png",
                                                          ":/qmldesigner/images/hole.png",m_treeModel.data());
#endif


    treeWidget()->setItemDelegateForColumn(0,idDelegate);
#ifdef _LOCK_ITEMS_
    treeWidget()->setItemDelegateForColumn(1,lockDelegate);
    treeWidget()->setItemDelegateForColumn(2,showDelegate);
#else
    treeWidget()->setItemDelegateForColumn(1,showDelegate);
#endif

}

NavigatorView::~NavigatorView()
{
    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

QWidget *NavigatorView::widget()
{
    return m_widget.data();
}

void NavigatorView::modelAttached(Model *model)
{
    QmlModelView::modelAttached(model);

    m_treeModel->setView(this);

    treeWidget()->expandAll();

    treeWidget()->header()->setResizeMode(0, QHeaderView::Stretch);
    treeWidget()->header()->resizeSection(1,26);
    treeWidget()->setRootIsDecorated(false);
    treeWidget()->setIndentation(20);
#ifdef _LOCK_ITEMS_
    treeWidget()->header()->resizeSection(2,20);
#endif
    treeWidget()->setHeaderHidden(true);
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    m_treeModel->clearView();
    QmlModelView::modelAboutToBeDetached(model);
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (m_treeModel->isInTree(removedNode))
        m_treeModel->removeSubTree(removedNode);
    QmlModelView::nodeAboutToBeRemoved(removedNode);
}

void NavigatorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty & /*newPropertyParent*/, const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    bool blocked = blockSelectionChangedSignal(true);

    if (m_treeModel->isInTree(node))
        m_treeModel->removeSubTree(node);
    if (node.isInHierarchy())
        m_treeModel->addSubTree(node);

    // make sure selection is in sync again
    updateItemSelection();

    blockSelectionChangedSignal(blocked);
}

void NavigatorView::nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId)
{
    QmlModelView::nodeIdChanged(node, newId, oldId);
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRow(node);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
    QmlModelView::propertiesAboutToBeRemoved(propertyList);
    foreach (const AbstractProperty &property, propertyList) {
        if (property.isNodeProperty()) {
            NodeProperty nodeProperty(property.toNodeProperty());
            m_treeModel->removeSubTree(nodeProperty.modelNode());
        } else if (property.isNodeListProperty()) {
            NodeListProperty nodeListProperty(property.toNodeListProperty());
            foreach (const ModelNode &node, nodeListProperty.toModelNodeList()) {
                m_treeModel->removeSubTree(node);
            }
        }
    }
}

void NavigatorView::rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion)
{
    QmlModelView::rootNodeTypeChanged(type, majorVersion, minorVersion);
    if (m_treeModel->isInTree(rootModelNode()))
        m_treeModel->updateItemRow(rootModelNode());
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data)
{
    QmlModelView::auxiliaryDataChanged(node, name, data);
    if (m_treeModel->isInTree(node))
    {
        // update model
        m_treeModel->updateItemRow(node);

        // repaint row (id and icon)
        QModelIndex index = m_treeModel->indexForNode(node);
        treeWidget()->update( index );
        treeWidget()->update( index.sibling(index.row(),index.column()+1) );
    }
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &node, int oldIndex)
{
    QmlModelView::nodeOrderChanged(listProperty, node, oldIndex);

    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRowOrder(node);
}

void NavigatorView::stateChanged(const QmlModelState &/*newQmlModelState*/, const QmlModelState &/*oldQmlModelState*/)
{
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    if (m_blockSelectionChangedSignal)
        return;
    QSet<ModelNode> nodeSet;
    foreach (const QModelIndex &index, treeWidget()->selectionModel()->selectedIndexes()) {
        nodeSet.insert(m_treeModel->nodeForIndex(index));
    }

    bool blocked = blockSelectionChangedSignal(true);
    setSelectedModelNodes(nodeSet.toList());
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList)
{
    QmlModelView::selectedNodesChanged(selectedNodeList, lastSelectedNodeList);

    updateItemSelection();
}

void NavigatorView::updateItemSelection()
{
    QItemSelection itemSelection;
    foreach (const ModelNode &node, selectedModelNodes()) {
        const QModelIndex index = m_treeModel->indexForNode(node);
        if (index.isValid()) {
            const QModelIndex beginIndex(m_treeModel->index(index.row(), 0, index.parent()));
            const QModelIndex endIndex(m_treeModel->index(index.row(), m_treeModel->columnCount(index.parent()) - 1, index.parent()));
            if (beginIndex.isValid() && endIndex.isValid())
                itemSelection.select(beginIndex, endIndex);
        }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect);
    blockSelectionChangedSignal(blocked);

    // make sure selected nodes a visible
    foreach(const QModelIndex &selectedIndex, itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandRecursively(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget()
{
    if (m_widget)
        return m_widget->treeView();
    return 0;
}

NavigatorTreeModel *NavigatorView::treeModel()
{
    return m_treeModel.data();
}

// along the lines of QObject::blockSignals
bool NavigatorView::blockSelectionChangedSignal(bool block)
{
    bool oldValue = m_blockSelectionChangedSignal;
    m_blockSelectionChangedSignal = block;
    return oldValue;
}

void NavigatorView::expandRecursively(const QModelIndex &index)
{
    QModelIndex currentIndex = index;
    while (currentIndex.isValid()) {
        if (!treeWidget()->isExpanded(currentIndex))
            treeWidget()->expand(currentIndex);
        currentIndex = currentIndex.parent();
    }
}

} // namespace QmlDesigner
