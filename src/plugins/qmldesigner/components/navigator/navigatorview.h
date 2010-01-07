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

#ifndef NAVIGATORVIEW_H
#define NAVIGATORVIEW_H

#include <qmlmodelview.h>

#include <QWeakPointer>

class QTreeView;
class QStandardItem;
class QItemSelection;
class QModelIndex;

namespace QmlDesigner {

class NavigatorWidget;
class NavigatorTreeModel;

class NavigatorView : public QmlModelView
{
    Q_OBJECT

public:
    NavigatorView(QObject* parent = 0);
    ~NavigatorView();

    QWidget *widget();

    // AbstractView
    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeReparented(const ModelNode &node, const ModelNode &oldParent, const ModelNode &newParent);

    void nodeSlidedToIndex(const ModelNode &node, int newIndex, int oldIndex);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeTypeChanged(const ModelNode &node,const QString &type, int majorVersion, int minorVersion);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList);

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                                      const QList<ModelNode> &lastSelectedNodeList);
    void auxiliaryDataChanged(const ModelNode &node, const QString &name, const QVariant &data);
    void nodeSlidedToIndex(const NodeListProperty &listProperty, int newIndex, int oldIndex);

    void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);

private slots:
//    void handleChangedItem(QStandardItem * item);
    void changeSelection(const QItemSelection &selected, const QItemSelection &deselected);
    void updateItemSelection();

protected: //functions
    QTreeView *treeWidget();
    NavigatorTreeModel *treeModel();
    bool blockSelectionChangedSignal(bool block);
    void expandRecursively(const QModelIndex &index);

private:
    bool m_blockSelectionChangedSignal;

    QWeakPointer<NavigatorWidget> m_widget;
    QWeakPointer<NavigatorTreeModel> m_treeModel;

    friend class TestNavigator;
};

}
#endif // NAVIGATORVIEW_H
