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

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"
#include "modelnodecontextmenu.h"

#include <coreplugin/editormanager/editormanager.h>

#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <QHeaderView>

static inline void setScenePos(const QmlDesigner::ModelNode &modelNode,const QPointF &pos)
{
    QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentQmlObjectNode().toQmlItemNode();
    if (parentNode.isValid()) {
        QPointF localPos = parentNode.instanceSceneTransform().inverted().map(pos);
        modelNode.variantProperty(QLatin1String("x")) = localPos.toPoint().x();
        modelNode.variantProperty(QLatin1String("y")) = localPos.toPoint().y();
    }
}

namespace QmlDesigner {

NavigatorView::NavigatorView(QObject* parent) :
        QmlModelView(parent),
        m_blockSelectionChangedSignal(false),
        m_widget(new NavigatorWidget(this)),
        m_treeModel(new NavigatorTreeModel(this))
{
    m_widget->setTreeModel(m_treeModel.data());

    connect(treeWidget()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(changeSelection(QItemSelection,QItemSelection)));

    connect(m_widget.data(), SIGNAL(leftButtonClicked()), this, SLOT(leftButtonClicked()));
    connect(m_widget.data(), SIGNAL(rightButtonClicked()), this, SLOT(rightButtonClicked()));
    connect(m_widget.data(), SIGNAL(downButtonClicked()), this, SLOT(downButtonClicked()));
    connect(m_widget.data(), SIGNAL(upButtonClicked()), this, SLOT(upButtonClicked()));

    treeWidget()->setIndentation(treeWidget()->indentation() * 0.5);

    NameItemDelegate *idDelegate = new NameItemDelegate(this,m_treeModel.data());
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

    QTreeView *treeView = treeWidget();
    treeView->expandAll();

    treeView->header()->setResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,26);
    treeView->setRootIsDecorated(false);
    treeView->setIndentation(20);
#ifdef _LOCK_ITEMS_
    treeView->header()->resizeSection(2,20);
#endif
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    m_treeModel->clearView();
    QmlModelView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    treeWidget()->update();
}

void NavigatorView::nodeCreated(const ModelNode & /*createdNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode & /*removedNode*/, const NodeAbstractProperty & /*parentProperty*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> & /*propertyList*/)
{
}

void NavigatorView::variantPropertiesChanged(const QList<VariantProperty> & /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & /*propertyList*/, PropertyChangeFlags /*propertyChange*/)
{
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    if (m_treeModel->isInTree(removedNode))
        m_treeModel->removeSubTree(removedNode);
}

void NavigatorView::nodeAboutToBeReparented(const ModelNode &/*node*/, const NodeAbstractProperty &/*newPropertyParent*/, const NodeAbstractProperty &/*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
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

void NavigatorView::nodeIdChanged(const ModelNode& node, const QString & /*newId*/, const QString & /*oldId*/)
{
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRow(node);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList)
{
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

void NavigatorView::rootNodeTypeChanged(const QString & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    if (m_treeModel->isInTree(rootModelNode()))
        m_treeModel->updateItemRow(rootModelNode());
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &node, const QString & /*name*/, const QVariant & /*data*/)
{
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

void NavigatorView::scriptFunctionsChanged(const ModelNode &/*node*/, const QStringList &/*scriptFunctionList*/)
{
}

void NavigatorView::instancePropertyChange(const QList<QPair<ModelNode, QString> > &/*propertyList*/)
{
}

void NavigatorView::instancesCompleted(const QVector<ModelNode> &/*completedNodeList*/)
{
}

void NavigatorView::instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &/*informationChangeHash*/)
{
}

void NavigatorView::instancesRenderImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void NavigatorView::instancesPreviewImageChanged(const QVector<ModelNode> &/*nodeList*/)
{
}

void NavigatorView::instancesChildrenChanged(const QVector<ModelNode> &/*nodeList*/)
{

}

void NavigatorView::nodeSourceChanged(const ModelNode &, const QString & /*newNodeSource*/)
{

}

void NavigatorView::rewriterBeginTransaction()
{
}

void NavigatorView::rewriterEndTransaction()
{
}

void NavigatorView::actualStateChanged(const ModelNode &/*node*/)
{
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &node, int oldIndex)
{
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRowOrder(listProperty, node, oldIndex);
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
    if (index.isValid() && m_treeModel->data(index, Qt::UserRole).isValid()) {
        ModelNode doubleClickNode = m_treeModel->nodeForIndex(index);
        if (doubleClickNode.metaInfo().isFileComponent())
            Core::EditorManager::openEditor(doubleClickNode.metaInfo().componentFileName());
    }
}

void NavigatorView::leftButtonClicked()
{
    if (selectedModelNodes().count() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);

    foreach (const ModelNode &node, selectedModelNodes()) {
        if (!node.isRootNode() && !node.parentProperty().parentModelNode().isRootNode()) {
            if (QmlItemNode(node).isValid()) {
                QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                node.parentProperty().parentModelNode().parentProperty().reparentHere(node);
                if (!scenePos.isNull())
                    setScenePos(node, scenePos);
            } else {
                node.parentProperty().parentModelNode().parentProperty().reparentHere(node);
            }
        }
    }
    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::rightButtonClicked()
{
    if (selectedModelNodes().count() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);
    foreach (const ModelNode &node, selectedModelNodes()) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty() && node.parentProperty().toNodeListProperty().count() > 1) {
            int index = node.parentProperty().toNodeListProperty().indexOf(node);
            index--;
            if (index >= 0) { //for the first node the semantics are not clear enough. Wrapping would be irritating.
                ModelNode newParent = node.parentProperty().toNodeListProperty().at(index);

                if (QmlItemNode(node).isValid()) {
                    QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                    newParent.nodeAbstractProperty(newParent.metaInfo().defaultPropertyName()).reparentHere(node);
                    if (!scenePos.isNull())
                        setScenePos(node, scenePos);
                } else {
                    newParent.nodeAbstractProperty(newParent.metaInfo().defaultPropertyName()).reparentHere(node);
                }
            }
        }
    }
    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::upButtonClicked()
{
    bool blocked = blockSelectionChangedSignal(true);
    foreach (const ModelNode &node, selectedModelNodes()) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().toNodeListProperty().indexOf(node);
            int index = oldIndex;
            index--;
            if (index < 0)
                index = node.parentProperty().toNodeListProperty().count() - 1; //wrap around
            node.parentProperty().toNodeListProperty().slide(oldIndex, index);
        }
    }
    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::downButtonClicked()
{
    bool blocked = blockSelectionChangedSignal(true);
    foreach (const ModelNode &node, selectedModelNodes()) {
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty()) {
            int oldIndex = node.parentProperty().toNodeListProperty().indexOf(node);
            int index = oldIndex;
            index++;
            if (index >= node.parentProperty().toNodeListProperty().count())
                index = 0; //wrap around
            node.parentProperty().toNodeListProperty().slide(oldIndex, index);
        }
    }
    updateItemSelection();
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    if (m_blockSelectionChangedSignal)
        return;
    QSet<ModelNode> nodeSet;
    foreach (const QModelIndex &index, treeWidget()->selectionModel()->selectedIndexes()) {
        if (m_treeModel->data(index, Qt::UserRole).isValid())
            nodeSet.insert(m_treeModel->nodeForIndex(index));
    }

    bool blocked = blockSelectionChangedSignal(true);
    setSelectedModelNodes(nodeSet.toList());
    blockSelectionChangedSignal(blocked);
}

void NavigatorView::selectedNodesChanged(const QList<ModelNode> &/*selectedNodeList*/, const QList<ModelNode> &/*lastSelectedNodeList*/)
{
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

    if (!selectedModelNodes().isEmpty())
        treeWidget()->scrollTo(m_treeModel->indexForNode(selectedModelNodes().first()));

    // make sure selected nodes a visible
    foreach (const QModelIndex &selectedIndex, itemSelection.indexes()) {
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
