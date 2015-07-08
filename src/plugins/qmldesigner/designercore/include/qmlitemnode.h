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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
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
class ItemLibraryEntry;

class QMLDESIGNERCORE_EXPORT QmlItemNode : public QmlObjectNode
{
    friend class QmlAnchors;
public:
    QmlItemNode() : QmlObjectNode() {}
    QmlItemNode(const ModelNode &modelNode)  : QmlObjectNode(modelNode) {}
    bool isValid() const;
    static bool isValidQmlItemNode(const ModelNode &modelNode);
    bool isRootNode() const;

    static bool isItemOrWindow(const ModelNode &modelNode);

    static QmlItemNode createQmlItemNode(AbstractView *view,
                                         const ItemLibraryEntry &itemLibraryEntry,
                                         const QPointF &position,
                                         QmlItemNode parentQmlItemNode);
    static QmlItemNode createQmlItemNode(AbstractView *view,
                                         const ItemLibraryEntry &itemLibraryEntry,
                                         const QPointF &position,
                                         NodeAbstractProperty parentproperty);
    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  QmlItemNode parentQmlItemNode);
    static QmlItemNode createQmlItemNodeFromImage(AbstractView *view,
                                                  const QString &imageName,
                                                  const QPointF &position,
                                                  NodeAbstractProperty parentproperty);

    QmlModelStateGroup states() const;
    QList<QmlItemNode> children() const;
    QList<QmlObjectNode> resources() const;
    QList<QmlObjectNode> allDirectSubNodes() const;
    QmlAnchors anchors() const;

    bool hasChildren() const;
    bool hasResources() const;
    bool instanceHasAnchor(AnchorLineType sourceAnchorLineType) const;
    bool instanceHasAnchors() const;
    bool instanceHasShowContent() const;

    bool instanceCanReparent() const;
    bool instanceIsAnchoredBySibling() const;
    bool instanceIsAnchoredByChildren() const;
    bool instanceIsMovable() const;
    bool instanceIsResizable() const;
    bool instanceIsInLayoutable() const;
    bool instanceHasRotationTransform() const;

    bool modelIsMovable() const;
    bool modelIsResizable() const;

    QRectF instanceBoundingRect() const;
    QRectF instancePaintedBoundingRect() const;
    QRectF instanceContentItemBoundingRect() const;
    QTransform instanceTransform() const;
    QTransform instanceTransformWithContentTransform() const;
    QTransform instanceTransformWithContentItemTransform() const;
    QTransform instanceSceneTransform() const;
    QTransform instanceSceneContentItemTransform() const;
    QPointF instanceScenePosition() const;
    QPointF instancePosition() const;
    QSizeF instanceSize() const;
    int instancePenWidth() const;
    bool instanceIsRenderPixmapNull() const;

    QPixmap instanceRenderPixmap() const;
    QPixmap instanceBlurredRenderPixmap() const;

    TypeName simplifiedTypeName() const;

    const QList<QmlItemNode> allDirectSubModelNodes() const;
    const QList<QmlItemNode> allSubModelNodes() const;
    bool hasAnySubModelNodes() const;

    void setPosition(const QPointF &position);
    void setPostionInBaseState(const QPointF &position);

    void setSize(const QSizeF &size);
    bool isInLayout() const;
};

QMLDESIGNERCORE_EXPORT uint qHash(const QmlItemNode &node);

class QMLDESIGNERCORE_EXPORT QmlModelStateGroup
{
    friend class QmlItemNode;
    friend class StatesEditorView;

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
