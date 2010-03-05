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
#include <forwardview.h>
#include <abstractview.h>
#include "qmlitemnode.h"
#include "qmlstate.h"
#include "nodeinstanceview.h"


namespace QmlDesigner {

class ItemLibraryInfo;

class CORESHARED_EXPORT QmlModelView : public ForwardView<NodeInstanceView>
{
    Q_OBJECT
    friend CORESHARED_EXPORT class QmlObjectNode;
    friend CORESHARED_EXPORT class QmlModelNodeFacade;

public:
    QmlModelView(QObject *parent) ;

    void setCurrentState(const QmlModelState &state);
    QmlModelState currentState() const;

    QmlModelState baseState() const;

    QmlObjectNode createQmlObjectNode(const QString &typeString,
                                      int majorVersion,
                                      int minorVersion,
                                      const PropertyListType &propertyList = PropertyListType());

    QmlItemNode createQmlItemNode(const QString &typeString,
                                    int majorVersion,
                                    int minorVersion,
                                    const PropertyListType &propertyList = PropertyListType());

    QmlItemNode createQmlItemNode(const ItemLibraryInfo &ItemLibraryRepresentation, const QPointF &position, QmlItemNode parentNode);
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

protected:
    NodeInstance instanceForModelNode(const ModelNode &modelNode);
    bool hasInstanceForModelNode(const ModelNode &modelNode);
    NodeInstanceView *nodeInstanceView() const;
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
