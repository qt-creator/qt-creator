/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
****************************************************************************/

#ifndef NAVIGATORVIEW_H
#define NAVIGATORVIEW_H

#include <abstractview.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QTreeView;
class QItemSelection;
class QModelIndex;
QT_END_NAMESPACE

namespace QmlDesigner {

class NavigatorWidget;
class NavigatorTreeModel;

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
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) override;

    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange) override;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) override;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) override;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty>& propertyList) override;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList ,
                                      const QList<ModelNode> &lastSelectedNodeList) override;
    void auxiliaryDataChanged(const ModelNode &node, const PropertyName &name, const QVariant &data) override;
    void instanceErrorChange(const QVector<ModelNode> &errorNodeList) override;

    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList, PropertyChangeFlags) override;

private slots:
    void changeSelection(const QItemSelection &selected, const QItemSelection &deselected);
    void updateItemSelection();
    void changeToComponent(const QModelIndex &index);

    void leftButtonClicked();
    void rightButtonClicked();
    void upButtonClicked();
    void downButtonClicked();

protected: //functions
    QTreeView *treeWidget();
    NavigatorTreeModel *treeModel();
    bool blockSelectionChangedSignal(bool block);
    void expandRecursively(const QModelIndex &index);

private:
    bool m_blockSelectionChangedSignal;

    QPointer<NavigatorWidget> m_widget;
    QPointer<NavigatorTreeModel> m_treeModel;

    friend class TestNavigator;
};

}
#endif // NAVIGATORVIEW_H
