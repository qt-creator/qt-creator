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

#ifndef GRAPHICSOBJECTNODEINSTANCE_H
#define GRAPHICSOBJECTNODEINSTANCE_H

#include "objectnodeinstance.h"

QT_BEGIN_NAMESPACE
class QGraphicsObject;
QT_END_NAMESPACE

namespace QmlDesigner {
namespace Internal {

class GraphicsObjectNodeInstance : public ObjectNodeInstance
{
public:
    GraphicsObjectNodeInstance(QGraphicsObject *graphicsObject);
    QImage renderImage() const;

    bool isGraphicsObject() const;

    QRectF boundingRect() const;
    QPointF position() const;
    QSizeF size() const;
    QTransform transform() const;
    QTransform customTransform() const;
    QTransform sceneTransform() const;
    double opacity() const;

    QObject *parent() const;

    double rotation() const;
    double scale() const;
    QList<QGraphicsTransform *> transformations() const;
    QPointF transformOriginPoint() const;
    double zValue() const;

    bool equalGraphicsItem(QGraphicsItem *item) const;

    void setPropertyVariant(const QString &name, const QVariant &value);
    void setPropertyBinding(const QString &name, const QString &expression);
    QVariant property(const QString &name) const;

    bool hasContent() const;

    QList<ServerNodeInstance> childItems() const;
    QList<ServerNodeInstance> childItemsForChild(QGraphicsItem *childItem) const;

    void paintUpdate();

    bool isMovable() const;
    void setMovable(bool movable);


protected:
    void setHasContent(bool hasContent);
    QGraphicsObject *graphicsObject() const;
    void paintRecursively(QGraphicsItem *graphicsItem, QPainter *painter) const;
    QRectF boundingRectWithStepChilds(QGraphicsItem *parentItem) const;
    bool childrenHasContent(QGraphicsItem *graphicsItem) const;

private: // variables
    bool m_hasContent;
    bool m_isMovable;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // GRAPHICSOBJECTNODEINSTANCE_H
