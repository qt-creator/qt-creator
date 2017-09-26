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

#pragma once

#include "navigatormodelinterface.h"

#include <abstractview.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QItemSelection;
class QModelIndex;
class QAbstractItemModel;
QT_END_NAMESPACE

namespace QmlDesigner {

class NavigatorWidget;
class NavigatorTreeModel;

enum NavigatorRoles {
    ItemIsVisibleRole = Qt::UserRole,
    RowIsPropertyRole = Qt::UserRole + 1,
    ModelNodeRole = Qt::UserRole + 2
};

class NavigatorView : public AbstractView
{
    Q_OBJECT

public:
    NavigatorView(QObject* parent = 0);
    ~NavigatorView();

    bool hasWidget() const override;
    WidgetInfo widgetInfo() override;

    // AbstractView
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;

    void nodeAboutToBeRemoved(const ModelNode &removedNode) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) override;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;

    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeTypeChanged(const ModelNode& node, const TypeName &type, int majorVersion, int minorVersion) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList ,
                                      const QList<ModelNode> &lastSelectedNodeList) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;
    void instanceErrorChanged(const QVector<ModelNode> &errorNodeList) override;

    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags) override;

    void handleChangedExport(const ModelNode &modelNode, bool exported);
    bool isNodeInvisible(const ModelNode &modelNode) const;

private:
    ModelNode modelNodeForIndex(const QModelIndex &modelIndex) const;
    void changeSelection(const QItemSelection &selected, const QItemSelection &deselected);
    void updateItemSelection();
    void changeToComponent(const QModelIndex &index);
    QModelIndex indexForModelNode(const ModelNode &modelNode) const;
    QAbstractItemModel *currentModel() const;

    void leftButtonClicked();
    void rightButtonClicked();
    void upButtonClicked();
    void downButtonClicked();
    void filterToggled(bool);

protected: //functions
    QTreeView *treeWidget() const;
    NavigatorTreeModel *treeModel();
    bool blockSelectionChangedSignal(bool block);
    void expandRecursively(const QModelIndex &index);

private:
    bool m_blockSelectionChangedSignal;

    QPointer<NavigatorWidget> m_widget;
    QPointer<NavigatorTreeModel> m_treeModel;

    NavigatorModelInterface *m_currentModelInterface = nullptr;

    friend class TestNavigator;
};

}
