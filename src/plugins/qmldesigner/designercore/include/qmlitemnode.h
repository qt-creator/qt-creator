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

#ifndef QmlItemNode_H
#define QmlItemNode_H

#include <qmldesignercorelib_global.h>
#include <modelnode.h>
#include "qmlobjectnode.h"
#include "qmlstate.h"

#include <QStringList>
#include <QRectF>
#include <QTransform>

namespace QmlDesigner {

class QmlModelStateGroup;
class QmlAnchors;

class QMLDESIGNERCORE_EXPORT QmlItemNode : public QmlObjectNode
{
    friend class QMLDESIGNERCORE_EXPORT QmlAnchors;
public:
    QmlItemNode() : QmlObjectNode() {}
    QmlItemNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode) {}
    bool isValid() const;
    bool isRootNode() const;

    QmlModelStateGroup states() const;
    QList<QmlItemNode> children() const;
    QList<QmlObjectNode> resources() const;
    QList<QmlObjectNode> defaultPropertyChildren() const;
    QList<QmlObjectNode> allDirectSubNodes() const;
    QmlAnchors anchors() const;

    bool hasChildren() const;
    bool hasResources() const;
    bool instanceHasAnchors() const;
    bool hasShowContent() const;

    bool canReparent() const;
    bool instanceIsAnchoredBySibling() const;
    bool instanceIsAnchoredByChildren() const;
    bool instanceIsMovable() const;
    bool instanceIsResizable() const;
    bool instanceIsInPositioner() const;
    bool instanceHasRotationTransform() const;

    QRectF instanceBoundingRect() const;
    QRectF instancePaintedBoundingRect() const;
    QTransform instanceTransform() const;
    QTransform instanceSceneTransform() const;
    QPointF instanceScenePosition() const;
    QPointF instancePosition() const;
    QSizeF instanceSize() const;
    int instancePenWidth() const;
    bool instanceIsRenderPixmapNull() const;


    void paintInstance(QPainter *painter);

    void selectNode();
    void deselectNode();
    bool isSelected() const;

    QString simplifiedTypeName() const;

    const QList<QmlItemNode> allDirectSubModelNodes() const;
    const QList<QmlItemNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;
};

QMLDESIGNERCORE_EXPORT uint qHash(const QmlItemNode &node);

class QMLDESIGNERCORE_EXPORT QmlModelStateGroup
{
    friend class QmlItemNode;
    friend class QmlModelView;

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

QMLDESIGNERCORE_EXPORT QList<ModelNode> toModelNodeList(const QList<QmlItemNode> &fxItemNodeList);
QMLDESIGNERCORE_EXPORT QList<QmlItemNode> toQmlItemNodeList(const QList<ModelNode> &modelNodeList);

} //QmlDesigner


#endif // QmlItemNode_H
