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

#ifndef QMLMODELVIEW_H
#define QMLMODELVIEW_H

#include <corelib_global.h>
#include <abstractview.h>
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "nodeinstanceview.h"


namespace QmlDesigner {

class ItemLibraryEntry;

class CORESHARED_EXPORT QmlModelView : public AbstractView
{
    Q_OBJECT
    friend CORESHARED_EXPORT class QmlObjectNode;
    friend CORESHARED_EXPORT class QmlModelNodeFacade;

public:
    QmlModelView(QObject *parent) ;

    void setCurrentState(const QmlModelState &state);
    QmlModelState currentState() const;

    QmlModelState baseState() const;
    QmlModelStateGroup rootStateGroup() const;

    QmlObjectNode createQmlObjectNode(const QString &typeString,
                                      int majorVersion,
                                      int minorVersion,
                                      const PropertyListType &propertyList = PropertyListType());

    QmlItemNode createQmlItemNode(const QString &typeString,
                                    int majorVersion,
                                    int minorVersion,
                                    const PropertyListType &propertyList = PropertyListType());

    QmlItemNode createQmlItemNode(const ItemLibraryEntry &itemLibraryEntry, const QPointF &position, QmlItemNode parentNode);
    QmlItemNode createQmlItemNodeFromImage(const QString &imageName, const QPointF &position, QmlItemNode parentNode);

    QmlObjectNode rootQmlObjectNode() const;
    QmlItemNode rootQmlItemNode() const;


    void setSelectedQmlObjectNodes(const QList<QmlObjectNode> &selectedNodeList);
    void setSelectedQmlItemNodes(const QList<QmlItemNode> &selectedNodeList);
    void selectQmlObjectNode(const QmlObjectNode &node);
    void deselectQmlObjectNode(const QmlObjectNode &node);

    QList<QmlObjectNode> selectedQmlObjectNodes() const;
    QList<QmlItemNode> selectedQmlItemNodes() const;

    QmlObjectNode fxObjectNodeForId(const QString &id);

    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data);

    void modelAttached(Model *model);
    void modelAboutToBeDetached(Model *model);

    virtual void nodeInstancePropertyChanged(const ModelNode &node, const QString &propertyName);

    void instancePropertyChange(const QList<QPair<ModelNode, QString> > &propertyList);
    void instancesCompleted(const QVector<ModelNode> &completedNodeList);

    void nodeCreated(const ModelNode &createdNode);
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange);
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent, const NodeAbstractProperty &oldPropertyParent, AbstractView::PropertyChangeFlags propertyChange);
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList);
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion);
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex);
    void nodeAboutToBeRemoved(const ModelNode &removedNode);
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId);
    void propertiesRemoved(const QList<AbstractProperty>& propertyList);
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange);
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange);
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList);
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList);


protected:
    NodeInstance instanceForModelNode(const ModelNode &modelNode);
    bool hasInstanceForModelNode(const ModelNode &modelNode);
    virtual void transformChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName) ;
    virtual void parentChanged(const QmlObjectNode &qmlObjectNode);
    virtual void otherPropertyChanged(const QmlObjectNode &qmlObjectNode, const QString &propertyName);
    virtual void stateChanged(const QmlModelState &newQmlModelState, const QmlModelState &oldQmlModelState);

    void activateState(const QmlModelState &state);
    void changeToState(const ModelNode &node, const QString &stateName);

private:
    QmlModelState m_state;
};

} //QmlDesigner

#endif // QMLMODELVIEW_H
