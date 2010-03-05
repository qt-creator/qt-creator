/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef PropertyView_h
#define PropertyView_h

#include <QTabWidget>

#include <modelnode.h>
#include <widgetqueryview.h>


class QtProperty;

namespace QmlDesigner {

class GenericPropertiesWidget: public AbstractView
{
    Q_OBJECT

public:
    GenericPropertiesWidget(QWidget *parent = 0);
    QWidget* createPropertiesPage();
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList,
                              const QList<ModelNode> &lastSelectedNodeList);

    virtual void modelAttached(Model *model);
    virtual void nodeCreated(const ModelNode &createdNode);
    virtual void nodeAboutToBeRemoved(const ModelNode &removedNode);
    virtual void propertyAdded(const NodeState& state, const NodeProperty& property);
    virtual void propertyAboutToBeRemoved(const NodeState &state, const NodeProperty &property);
    virtual void nodeReparented(const ModelNode &node, const ModelNode &oldParent, const ModelNode &newParent);
    virtual void propertyValueChanged(const NodeState& state, const NodeProperty& property,
                              const QVariant& newValue, const QVariant& oldValue);

    void modelStateAboutToBeRemoved(const ModelState &modelState);
    void modelStateAdded(const ModelState &modelState);

    void nodeStatesAboutToBeRemoved(const QList<NodeState> &nodeStateList);
    void nodeStatesAdded(const QList<NodeState> &nodeStateList);

    void anchorsChanged(const NodeState &nodeState);

public slots:
    void select(const ModelNode& node);

private slots:
    void propertyChanged(class QtProperty* property);

private:
    void buildPropertyEditorItems();
    QtProperty* addVariantProperty(const PropertyMetaInfo& propertyMetaInfo, const QHash<QString, NodeProperty>& propertiesWithValues, const NodeInstance& instance);
    QtProperty* addEnumProperty(const PropertyMetaInfo& propertyMetaInfo, const QHash<QString, NodeProperty>& propertiesWithValues, const NodeInstance& instance);
    QtProperty* addFlagProperty(const PropertyMetaInfo& propertyMetaInfo, const QHash<QString, NodeProperty>& propertiesWithValues, const NodeInstance& instance);
    QtProperty* addProperties(const NodeMetaInfo& nodeMetaInfo, const QHash<QString, NodeProperty>& propertiesWithValues, const NodeInstance& instance);

    void disconnectEditor();
    void reconnectEditor();

private:
    class QtTreePropertyBrowser *editor;
    class QtVariantPropertyManager* variantManager;
    class QtEnumPropertyManager* enumManager;

    ModelNode selectedNode;
};

}

#endif // PropertyView_h
