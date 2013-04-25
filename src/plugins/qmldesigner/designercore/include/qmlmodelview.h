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

#ifndef QMLMODELVIEW_H
#define QMLMODELVIEW_H

#include <qmldesignercorelib_global.h>
#include <abstractview.h>
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "nodeinstanceview.h"


namespace QmlDesigner {

class ItemLibraryEntry;

class QMLDESIGNERCORE_EXPORT QmlModelView : public AbstractView
{
    Q_OBJECT
    friend class QmlObjectNode;
    friend class QmlModelNodeFacade;

public:
    QmlModelView(QObject *parent) ;

    void setCurrentState(const QmlModelState &state);
    QmlModelState currentState() const;

    QmlModelState baseState() const;
    QmlModelStateGroup rootStateGroup() const;

    QmlObjectNode createQmlObjectNode(const TypeName &typeString,
                                      int majorVersion,
                                      int minorVersion,
                                      const PropertyListType &propertyList = PropertyListType());

    QmlItemNode createQmlItemNode(const TypeName &typeString,
                                    int majorVersion,
                                    int minorVersion,
                                    const PropertyListType &propertyList = PropertyListType());

    ModelNode createQmlState(const PropertyListType &propertyList = PropertyListType());

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

    void modelAttached(Model *model) QTC_OVERRIDE;
    void modelAboutToBeDetached(Model *model) QTC_OVERRIDE;

    void instancePropertyChange(const QList<QPair<ModelNode, PropertyName> > &propertyList) QTC_OVERRIDE;
    void instancesCompleted(const QVector<ModelNode> &completedNodeList) QTC_OVERRIDE;
    void instanceInformationsChange(const QMultiHash<ModelNode, InformationName> &informationChangeHash) QTC_OVERRIDE;
    void instancesRenderImageChanged(const QVector<ModelNode> &nodeList) QTC_OVERRIDE;
    void instancesPreviewImageChanged(const QVector<ModelNode> &nodeList) QTC_OVERRIDE;
    void instancesChildrenChanged(const QVector<ModelNode> &nodeList) QTC_OVERRIDE;
    void instancesToken(const QString &tokenName, int tokenNumber, const QVector<ModelNode> &nodeVector);

    void rewriterBeginTransaction() QTC_OVERRIDE;
    void rewriterEndTransaction() QTC_OVERRIDE;

    void actualStateChanged(const ModelNode &node);

    void nodeCreated(const ModelNode &createdNode) QTC_OVERRIDE;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty, PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void nodeAboutToBeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                                 const NodeAbstractProperty &oldPropertyParent,
                                 AbstractView::PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        AbstractView::PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void propertiesAboutToBeRemoved(const QList<AbstractProperty> &propertyList) QTC_OVERRIDE;
    void rootNodeTypeChanged(const QString &type, int majorVersion, int minorVersion) QTC_OVERRIDE;
    void nodeOrderChanged(const NodeListProperty &listProperty, const ModelNode &movedNode, int oldIndex) QTC_OVERRIDE;
    void nodeAboutToBeRemoved(const ModelNode &removedNode) QTC_OVERRIDE;
    void nodeIdChanged(const ModelNode& node, const QString& newId, const QString& oldId) QTC_OVERRIDE;
    void propertiesRemoved(const QList<AbstractProperty>& propertyList) QTC_OVERRIDE;
    void variantPropertiesChanged(const QList<VariantProperty>& propertyList, PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void bindingPropertiesChanged(const QList<BindingProperty>& propertyList, PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void signalHandlerPropertiesChanged(const QVector<SignalHandlerProperty>& propertyList,PropertyChangeFlags propertyChange) QTC_OVERRIDE;
    void scriptFunctionsChanged(const ModelNode &node, const QStringList &scriptFunctionList) QTC_OVERRIDE;
    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList) QTC_OVERRIDE;

    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) QTC_OVERRIDE;
    void nodeSourceChanged(const ModelNode &modelNode, const QString &newNodeSource) QTC_OVERRIDE;

protected:
    NodeInstance instanceForModelNode(const ModelNode &modelNode);
    bool hasInstanceForModelNode(const ModelNode &modelNode);

    void activateState(const QmlModelState &state);

private:
    QmlModelState m_state;
};

} //QmlDesigner

#endif // QMLMODELVIEW_H
