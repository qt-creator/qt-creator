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

#ifndef QmlItemNode_H
#define QmlItemNode_H

#include <corelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;

class CORESHARED_EXPORT QmlItemNode : public QmlObjectNode
{
    friend class CORESHARED_EXPORT QmlAnchors;
public:
    QmlItemNode() : QmlObjectNode() {}
    QmlItemNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode) {}
    bool isValid() const;
    bool isRootNode() const;

    QmlModelStateGroup states() const;
    QList<QmlItemNode> children() const;
    QList<QmlObjectNode> resources() const;
    QmlAnchors anchors() const;

    bool hasChildren() const;
    bool hasResources() const;
    bool instanceHasAnchors() const;
    bool hasShowContent() const;

    bool canReparent() const;
    bool instanceIsAnchoredBy() const;

    QRectF instanceBoundingRect() const;
    QTransform instanceTransform() const;
    QTransform instanceSceneTransform() const;
    QPointF instancePosition() const;
    QSizeF instanceSize() const;

    void paintInstance(QPainter *painter) const;

    void setSize(const QSizeF &size);
    void setPosition(const QPointF &position);
    void setPositionWithBorder(const QPointF &position);

    void selectNode();
    void deselectNode();
    bool isSelected() const;

    QString simplfiedTypeName() const;

    const QList<QmlItemNode> allDirectSubModelNodes() const;
    const QList<QmlItemNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;
};

CORESHARED_EXPORT uint qHash(const QmlItemNode &node);

class CORESHARED_EXPORT QmlModelStateGroup
{
    friend class QmlItemNode;
public:

    QmlModelStateGroup() : m_modelNode(ModelNode()) {}

    ModelNode modelNode() const { return m_modelNode; }
    QStringList names() const;
    QList<QmlModelState> allStates() const;
    QmlModelState state(const QString &name) const;
    QmlModelState addState(const QString &name);
    void removeState(const QString &name);

protected:
    QmlModelStateGroup(const ModelNode &modelNode) : m_modelNode(modelNode) {}

private:
    ModelNode m_modelNode;
};

CORESHARED_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
CORESHARED_EXPORT QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner


#endif // QmlItemNode_H
