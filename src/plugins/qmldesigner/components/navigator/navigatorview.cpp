/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "navigatorview.h"
#include "navigatortreemodel.h"
#include "navigatorwidget.h"
#include "nameitemdelegate.h"
#include "iconcheckboxitemdelegate.h"
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <bindingproperty.h>
#include <designmodecontext.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <QHeaderView>
#include <qmlitemnode.h>

static inline void setScenePos(const QmlDesigner::ModelNode &modelNode,const QPointF &pos)
{
    if (modelNode.hasParentProperty() && QmlDesigner::QmlItemNode::isValidQmlItemNode(modelNode.parentProperty().parentModelNode())) {
        QmlDesigner::QmlItemNode parentNode = modelNode.parentProperty().parentQmlObjectNode().toQmlItemNode();

        if (!parentNode.modelNode().metaInfo().isLayoutable()) {
            QPointF localPos = parentNode.instanceSceneTransform().inverted().map(pos);
            modelNode.variantProperty("x").setValue(localPos.toPoint().x());
            modelNode.variantProperty("y").setValue(localPos.toPoint().y());
        } else { //Items in Layouts do not have a position
            modelNode.removeProperty("x");
            modelNode.removeProperty("y");
        }
    }
}

namespace QmlDesigner {

NavigatorView::NavigatorView(QObject* parent) :
        AbstractView(parent),
        m_blockSelectionChangedSignal(false),
        m_widget(new NavigatorWidget(this)),
        m_treeModel(new NavigatorTreeModel(this))
{
    Internal::NavigatorContext *navigatorContext = new Internal::NavigatorContext(m_widget.data());
    Core::ICore::addContextObject(navigatorContext);

    m_widget->setTreeModel(m_treeModel.data());

    connect(treeWidget()->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)), this, SLOT(changeSelection(QItemSelection,QItemSelection)));

    connect(m_widget.data(), SIGNAL(leftButtonClicked()), this, SLOT(leftButtonClicked()));
    connect(m_widget.data(), SIGNAL(rightButtonClicked()), this, SLOT(rightButtonClicked()));
    connect(m_widget.data(), SIGNAL(downButtonClicked()), this, SLOT(downButtonClicked()));
    connect(m_widget.data(), SIGNAL(upButtonClicked()), this, SLOT(upButtonClicked()));

    treeWidget()->setIndentation(treeWidget()->indentation() * 0.5);

    NameItemDelegate *idDelegate = new NameItemDelegate(this,
                                                        m_treeModel.data());
    IconCheckboxItemDelegate *showDelegate =
            new IconCheckboxItemDelegate(this,
                                         Utils::Icons::EYE_OPEN_TOOLBAR.pixmap(),
                                         Utils::Icons::EYE_CLOSED_TOOLBAR.pixmap(),
                                         m_treeModel.data());

    IconCheckboxItemDelegate *exportDelegate =
            new IconCheckboxItemDelegate(this,
                                         Icons::EXPORT_CHECKED.pixmap(),
                                         Icons::EXPORT_UNCHECKED.pixmap(),
                                         m_treeModel.data());

#ifdef _LOCK_ITEMS_
    IconCheckboxItemDelegate *lockDelegate = new IconCheckboxItemDelegate(this,":/qmldesigner/images/lock.png",
                                                          ":/qmldesigner/images/hole.png",m_treeModel.data());
#endif


    treeWidget()->setItemDelegateForColumn(0,idDelegate);
#ifdef _LOCK_ITEMS_
    treeWidget()->setItemDelegateForColumn(1,lockDelegate);
    treeWidget()->setItemDelegateForColumn(2,showDelegate);
#else
    treeWidget()->setItemDelegateForColumn(1,exportDelegate);
    treeWidget()->setItemDelegateForColumn(2,showDelegate);
#endif

}

NavigatorView::~NavigatorView()
{
    if (m_widget && !m_widget->parent())
        delete m_widget.data();
}

bool NavigatorView::hasWidget() const
{
    return true;
}

WidgetInfo NavigatorView::widgetInfo()
{
    return createWidgetInfo(m_widget.data(),
                            new WidgetInfo::ToolBarWidgetDefaultFactory<NavigatorWidget>(m_widget.data()),
                            QStringLiteral("Navigator"),
                            WidgetInfo::LeftPane,
                            0);
}

void NavigatorView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_treeModel->setView(this);

    QTreeView *treeView = treeWidget();
    treeView->expandAll();

    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,26);
    treeView->setRootIsDecorated(false);
    treeView->setIndentation(20);
#ifdef _LOCK_ITEMS_
    treeView->header()->resizeSection(2,20);
#endif
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    m_treeModel->removeSubTree(rootModelNode());
    m_treeModel->clearView();
    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    treeWidget()->update();
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & propertyList, PropertyChangeFlags /*propertyChange*/)
{
    foreach (const BindingProperty &bindingProperty, propertyList) {
        /* If a binding property that exports an item using an alias property has
         * changed, we have to update the affected item.
         */
        if (bindingProperty.isAliasExport())
            m_treeModel->updateItemRow(modelNodeForId(bindingProperty.expression()));
    }
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode &removedNode)
{
    m_treeModel->removeSubTree(removedNode);
}

void NavigatorView::nodeReparented(const ModelNode &node, const NodeAbstractProperty & newPropertyParent, const NodeAbstractProperty & /*oldPropertyParent*/, AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    bool blocked = blockSelectionChangedSignal(true);

    m_treeModel->removeSubTree(node);

    if (node.isInHierarchy())
        m_treeModel->addSubTree(node);

    // make sure selection is in sync again
    updateItemSelection();

    if (newPropertyParent.parentModelNode().isValid()) {
        QModelIndex index = m_treeModel->indexForNode(newPropertyParent.parentModelNode());
        treeWidget()->expand(index);
    }

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
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty(property.toNodeListProperty());
            foreach (const ModelNode &childNode, nodeAbstractProperty.directSubNodes()) {
                m_treeModel->removeSubTree(childNode);
            }
        }
    }
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    for (const AbstractProperty &property : propertyList) {
        ModelNode node = property.parentModelNode();
        /* Update export alias state if property from root note was removed and the root node was not selected. */
        /* The check for the selection is an optimization to reduce updates. */
        if (node.isRootNode() && !selectedModelNodes().isEmpty() && !selectedModelNodes().first().isRootNode()) {

            foreach (const ModelNode &modelNode, node.allSubModelNodes())
                m_treeModel->updateItemRow(modelNode);
        }
    }
}

void NavigatorView::rootNodeTypeChanged(const QString & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    if (m_treeModel->isInTree(rootModelNode()))
        m_treeModel->updateItemRow(rootModelNode());
}

void NavigatorView::nodeTypeChanged(const ModelNode &node, const TypeName &, int , int)
{
    if (m_treeModel->isInTree(node))
        m_treeModel->updateItemRow(node);
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &modelNode, const PropertyName & name, const QVariant & /*data*/)
{
    if (name == "invisible" && m_treeModel->isInTree(modelNode))
    {
        // update model
        m_treeModel->updateItemRow(modelNode);

        // repaint row (id and icon)
        foreach (const ModelNode &currentModelNode, modelNode.allSubModelNodesAndThisNode()) {
            QModelIndex index = m_treeModel->indexForNode(currentModelNode);
            treeWidget()->update(index);
            treeWidget()->update(index.sibling(index.row(),index.column()+1));
        }
    }
}

void NavigatorView::instanceErrorChanged(const QVector<ModelNode> &errorNodeList)
{
    foreach (const ModelNode &currentModelNode, errorNodeList)
        m_treeModel->updateItemRow(currentModelNode);
}

void NavigatorView::nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &node, int /*oldIndex*/)
{
    if (m_treeModel->isInTree(node)) {
        m_treeModel->removeSubTree(listProperty.parentModelNode());

        if (node.isInHierarchy())
            m_treeModel->addSubTree(listProperty.parentModelNode());

        if (listProperty.parentModelNode().isValid()) {
            QModelIndex index = m_treeModel->indexForNode(listProperty.parentModelNode());
            treeWidget()->expand(index);
        }
    }
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
    if (index.isValid() && m_treeModel->data(index, Qt::UserRole).isValid()) {
        ModelNode doubleClickNode = m_treeModel->nodeForIndex(index);
        if (doubleClickNode.metaInfo().isFileComponent())
            Core::EditorManager::openEditor(doubleClickNode.metaInfo().componentFileName(),
                                            Core::Id(), Core::EditorManager::DoNotMakeVisible);
    }
}

void NavigatorView::leftButtonClicked()
{
    if (selectedModelNodes().count() > 1)
        return; //Semantics are unclear for multi selection.

    bool blocked = blockSelectionChangedSignal(true);

    foreach (const ModelNode &node, selectedModelNodes()) {
        if (!node.isRootNode() && !node.parentProperty().parentModelNode().isRootNode()) {
            if (QmlItemNode::isValidQmlItemNode(node)) {
                QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                node.parentProperty().parentProperty().reparentHere(node);
                if (!scenePos.isNull())
                    setScenePos(node, scenePos);
            } else {
                node.parentProperty().parentProperty().reparentHere(node);
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
        if (!node.isRootNode() && node.parentProperty().isNodeListProperty() && node.parentProperty().count() > 1) {
            int index = node.parentProperty().indexOf(node);
            index--;
            if (index >= 0) { //for the first node the semantics are not clear enough. Wrapping would be irritating.
                ModelNode newParent = node.parentProperty().toNodeListProperty().at(index);

                if (QmlItemNode::isValidQmlItemNode(node)
                        && QmlItemNode::isValidQmlItemNode(newParent)
                        && !newParent.metaInfo().defaultPropertyIsComponent()) {
                    QPointF scenePos = QmlItemNode(node).instanceScenePosition();
                    newParent.nodeAbstractProperty(newParent.metaInfo().defaultPropertyName()).reparentHere(node);
                    if (!scenePos.isNull())
                        setScenePos(node, scenePos);
                } else {
                    if (newParent.metaInfo().isValid() && !newParent.metaInfo().defaultPropertyIsComponent())
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
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index--;
            if (index < 0)
                index = node.parentProperty().count() - 1; //wrap around
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
            int oldIndex = node.parentProperty().indexOf(node);
            int index = oldIndex;
            index++;
            if (index >= node.parentProperty().count())
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
