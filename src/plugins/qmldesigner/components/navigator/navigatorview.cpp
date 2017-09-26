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
#include "qmldesignerconstants.h"
#include "qmldesignericons.h"

#include "nameitemdelegate.h"
#include "iconcheckboxitemdelegate.h"

#include <bindingproperty.h>
#include <designmodecontext.h>
#include <designersettings.h>
#include <nodeproperty.h>
#include <nodelistproperty.h>
#include <variantproperty.h>
#include <qmlitemnode.h>
#include <rewritingexception.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>

#include <utils/icon.h>
#include <utils/utilsicons.h>

#include <QHeaderView>


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
#ifndef QMLDESIGNER_TEST
    Internal::NavigatorContext *navigatorContext = new Internal::NavigatorContext(m_widget.data());
    Core::ICore::addContextObject(navigatorContext);
#endif

    m_treeModel->setView(this);
    m_widget->setTreeModel(m_treeModel.data());
    m_currentModelInterface = m_treeModel;

    connect(treeWidget()->selectionModel(), &QItemSelectionModel::selectionChanged, this, &NavigatorView::changeSelection);

    connect(m_widget.data(), &NavigatorWidget::leftButtonClicked, this, &NavigatorView::leftButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::rightButtonClicked, this, &NavigatorView::rightButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::downButtonClicked, this, &NavigatorView::downButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::upButtonClicked, this, &NavigatorView::upButtonClicked);
    connect(m_widget.data(), &NavigatorWidget::filterToggled, this, &NavigatorView::filterToggled);

#ifndef QMLDESIGNER_TEST
    NameItemDelegate *idDelegate = new NameItemDelegate(this);
    IconCheckboxItemDelegate *showDelegate =
            new IconCheckboxItemDelegate(this,
                                         Utils::Icons::EYE_OPEN_TOOLBAR.icon(),
                                         Utils::Icons::EYE_CLOSED_TOOLBAR.icon());

    IconCheckboxItemDelegate *exportDelegate =
            new IconCheckboxItemDelegate(this,
                                         Icons::EXPORT_CHECKED.icon(),
                                         Icons::EXPORT_UNCHECKED.icon());

#ifdef _LOCK_ITEMS_
    IconCheckboxItemDelegate *lockDelegate =
            new IconCheckboxItemDelegate(this,
                                         Utils::Icons::LOCKED_TOOLBAR.icon(),
                                         Utils::Icons::UNLOCKED_TOOLBAR.icon());
#endif


    treeWidget()->setItemDelegateForColumn(0, idDelegate);
#ifdef _LOCK_ITEMS_
    treeWidget()->setItemDelegateForColumn(1,lockDelegate);
    treeWidget()->setItemDelegateForColumn(2,showDelegate);
#else
    treeWidget()->setItemDelegateForColumn(1, exportDelegate);
    treeWidget()->setItemDelegateForColumn(2, showDelegate);
#endif

#endif //QMLDESIGNER_TEST
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

    m_currentModelInterface->setFilter(
                DesignerSettings::getValue(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS).toBool());

    QTreeView *treeView = treeWidget();
    treeView->expandAll();

    treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    treeView->header()->resizeSection(1,26);
    treeView->setIndentation(20);
#ifdef _LOCK_ITEMS_
    treeView->header()->resizeSection(2,20);
#endif
}

void NavigatorView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
}

void NavigatorView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    treeWidget()->update();
}

void NavigatorView::bindingPropertiesChanged(const QList<BindingProperty> & propertyList, PropertyChangeFlags /*propertyChange*/)
{
    for (const BindingProperty &bindingProperty : propertyList) {
        /* If a binding property that exports an item using an alias property has
         * changed, we have to update the affected item.
         */

        if (bindingProperty.isAliasExport())
            m_currentModelInterface->notifyDataChanged(modelNodeForId(bindingProperty.expression()));
    }
}

void NavigatorView::handleChangedExport(const ModelNode &modelNode, bool exported)
{
    const ModelNode rootNode = rootModelNode();
    Q_ASSERT(rootNode.isValid());
    const PropertyName modelNodeId = modelNode.id().toUtf8();
    if (rootNode.hasProperty(modelNodeId))
        rootNode.removeProperty(modelNodeId);
    if (exported) {
        try {
            RewriterTransaction transaction =
                    beginRewriterTransaction(QByteArrayLiteral("NavigatorTreeModel:exportItem"));

            QmlObjectNode qmlObjectNode(modelNode);
            qmlObjectNode.ensureAliasExport();
            transaction.commit();
        }  catch (RewritingException &exception) { //better safe than sorry! There always might be cases where we fail
            exception.showException();
        }
    }
}

bool NavigatorView::isNodeInvisible(const ModelNode &modelNode) const
{
    return modelNode.auxiliaryData("invisible").toBool();
}

ModelNode NavigatorView::modelNodeForIndex(const QModelIndex &modelIndex) const
{
    return modelIndex.model()->data(modelIndex, ModelNodeRole).value<ModelNode>();
}

void NavigatorView::nodeAboutToBeRemoved(const ModelNode & /*removedNode*/)
{
}

void NavigatorView::nodeRemoved(const ModelNode &removedNode,
                                const NodeAbstractProperty & /*parentProperty*/,
                                AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    m_currentModelInterface->notifyModelNodesRemoved({removedNode});
}

void NavigatorView::nodeReparented(const ModelNode &modelNode,
                                   const NodeAbstractProperty & /*newPropertyParent*/,
                                   const NodeAbstractProperty & oldPropertyParent,
                                   AbstractView::PropertyChangeFlags /*propertyChange*/)
{
    if (!oldPropertyParent.isValid())
        m_currentModelInterface->notifyModelNodesInserted({modelNode});
    else
        m_currentModelInterface->notifyModelNodesMoved({modelNode});
    treeWidget()->expand(indexForModelNode(modelNode));
}

void NavigatorView::nodeIdChanged(const ModelNode& modelNode, const QString & /*newId*/, const QString & /*oldId*/)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::propertiesAboutToBeRemoved(const QList<AbstractProperty>& /*propertyList*/)
{
}

void NavigatorView::propertiesRemoved(const QList<AbstractProperty> &propertyList)
{
    QList<ModelNode> modelNodes;
    for (const AbstractProperty &property : propertyList) {
        if (property.isNodeAbstractProperty()) {
            NodeAbstractProperty nodeAbstractProperty(property.toNodeListProperty());
            modelNodes.append(nodeAbstractProperty.directSubNodes());
        }
    }

    m_currentModelInterface->notifyModelNodesRemoved(modelNodes);
}

void NavigatorView::rootNodeTypeChanged(const QString & /*type*/, int /*majorVersion*/, int /*minorVersion*/)
{
    m_currentModelInterface->notifyDataChanged(rootModelNode());
}

void NavigatorView::nodeTypeChanged(const ModelNode &modelNode, const TypeName &, int , int)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::auxiliaryDataChanged(const ModelNode &modelNode,
                                         const PropertyName & /*name*/,
                                         const QVariant & /*data*/)
{
    m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::instanceErrorChanged(const QVector<ModelNode> &errorNodeList)
{
    foreach (const ModelNode &modelNode, errorNodeList)
        m_currentModelInterface->notifyDataChanged(modelNode);
}

void NavigatorView::nodeOrderChanged(const NodeListProperty & listProperty,
                                     const ModelNode & /*node*/,
                                     int /*oldIndex*/)
{
    bool blocked = blockSelectionChangedSignal(true);

    m_currentModelInterface->notifyModelNodesMoved(listProperty.directSubNodes());

    // make sure selection is in sync again
    updateItemSelection();

    blockSelectionChangedSignal(blocked);
}

void NavigatorView::changeToComponent(const QModelIndex &index)
{
    if (index.isValid() && currentModel()->data(index, Qt::UserRole).isValid()) {
        const ModelNode doubleClickNode = modelNodeForIndex(index);
        if (doubleClickNode.metaInfo().isFileComponent())
            Core::EditorManager::openEditor(doubleClickNode.metaInfo().componentFileName(),
                                            Core::Id(), Core::EditorManager::DoNotMakeVisible);
    }
}

QModelIndex NavigatorView::indexForModelNode(const ModelNode &modelNode) const
{
    return m_currentModelInterface->indexForModelNode(modelNode);
}

QAbstractItemModel *NavigatorView::currentModel() const
{
    return treeWidget()->model();
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

void NavigatorView::filterToggled(bool flag)
{
    m_currentModelInterface->setFilter(flag);
    treeWidget()->expandAll();
    DesignerSettings::setValue(DesignerSettingsKey::NAVIGATOR_SHOW_ONLY_VISIBLE_ITEMS, flag);
}

void NavigatorView::changeSelection(const QItemSelection & /*newSelection*/, const QItemSelection &/*deselected*/)
{
    if (m_blockSelectionChangedSignal)
        return;

    QSet<ModelNode> nodeSet;

    for (const QModelIndex &index : treeWidget()->selectionModel()->selectedIndexes()) {

        const ModelNode modelNode = modelNodeForIndex(index);
        if (modelNode.isValid())
            nodeSet.insert(modelNode);
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
        const QModelIndex index = indexForModelNode(node);
        if (index.isValid()) {
            const QModelIndex beginIndex(currentModel()->index(index.row(), 0, index.parent()));
            const QModelIndex endIndex(currentModel()->index(index.row(), currentModel()->columnCount(index.parent()) - 1, index.parent()));
            if (beginIndex.isValid() && endIndex.isValid())
                itemSelection.select(beginIndex, endIndex);
        }
    }

    bool blocked = blockSelectionChangedSignal(true);
    treeWidget()->selectionModel()->select(itemSelection, QItemSelectionModel::ClearAndSelect);
    blockSelectionChangedSignal(blocked);

    if (!selectedModelNodes().isEmpty())
        treeWidget()->scrollTo(indexForModelNode(selectedModelNodes().first()));

    // make sure selected nodes a visible
    foreach (const QModelIndex &selectedIndex, itemSelection.indexes()) {
        if (selectedIndex.column() == 0)
            expandRecursively(selectedIndex);
    }
}

QTreeView *NavigatorView::treeWidget() const
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
