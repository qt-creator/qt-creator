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

#ifndef GRAPHICSSCENENODEINSTANCE_H
#define GRAPHICSSCENENODEINSTANCE_H

#include "objectnodeinstance.h"
#include <QGraphicsScene>
#include <QWeakPointer>

namespace QmlDesigner {
namespace Internal {

class GraphicsSceneNodeInstance : public ObjectNodeInstance
{
public:
    typedef QSharedPointer<GraphicsSceneNodeInstance> Pointer;
    typedef QWeakPointer<GraphicsSceneNodeInstance> WeakPointer;

    ~GraphicsSceneNodeInstance();

    static Pointer create(const NodeMetaInfo &metaInfo, QmlContext *context, QObject  *objectToBeWrapped);

    void paint(QPainter *painter) const;

    bool isTopLevel() const;

    void addItem(QGraphicsItem *item);

    bool isGraphicsScene() const;

    QRectF boundingRect() const;
    QPointF position() const;
    QSizeF size() const;


    bool isVisible() const;
    void setVisible(bool isVisible);

    QList<NodeInstance> instancesInRegions(const QList<QRectF> &region);

protected:
    GraphicsSceneNodeInstance(QGraphicsScene *scene);
    QGraphicsScene *graphicsScene() const;
};

}
}
#endif // GRAPHICSSCENENODEINSTANCE_H
