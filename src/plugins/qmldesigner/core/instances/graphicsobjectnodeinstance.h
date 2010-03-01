/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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
    GraphicsObjectNodeInstance(QGraphicsObject *graphicsObject, bool hasContent);

    void paint(QPainter *painter) const;

    bool isTopLevel() const;
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

    bool isVisible() const;
    void setVisible(bool isVisible);

    void setPropertyVariant(const QString &name, const QVariant &value);
    QVariant property(const QString &name) const;

    bool hasContent() const;

    void paintUpdate();

protected:
    QGraphicsObject *graphicsObject() const;
    void paintRecursively(QGraphicsItem *graphicsItem, QPainter *painter) const;
    static QPair<QGraphicsObject*, bool> createGraphicsObject(const NodeMetaInfo &metaInfo, QDeclarativeContext *context);
private: // variables
    bool m_isVisible;
    bool m_hasContent;
};

} // namespace Internal
} // namespace QmlDesigner

#endif // GRAPHICSOBJECTNODEINSTANCE_H
