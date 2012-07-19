/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
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
