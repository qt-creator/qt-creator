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

#include "graphicsviewnodeinstance.h"

#include <private/qdeclarativemetatype_p.h>
#include <QDeclarativeEngine>
#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

GraphicsViewNodeInstance::GraphicsViewNodeInstance(QGraphicsView *view)
  : WidgetNodeInstance(view)
{
}


GraphicsViewNodeInstance::Pointer GraphicsViewNodeInstance::create(const NodeMetaInfo &nodeMetaInfo,
                                                                   QDeclarativeContext *context,
                                                                   QObject *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);

    QGraphicsView* view = qobject_cast<QGraphicsView*>(object);
    if (view == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new GraphicsViewNodeInstance(view));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

QGraphicsView* GraphicsViewNodeInstance::graphicsView() const
{
    QGraphicsView* view = qobject_cast<QGraphicsView*>(widget());
    Q_ASSERT(view);
    return view;
}

void GraphicsViewNodeInstance::setScene(QGraphicsScene *scene)
{
    graphicsView()->setScene(scene);
}

bool GraphicsViewNodeInstance::isGraphicsView() const
{
    return true;
}

QSizeF GraphicsViewNodeInstance::size() const
{
    return graphicsView()->scene()->itemsBoundingRect().size();
}

QTransform GraphicsViewNodeInstance::transform() const
{
    return graphicsView()->transform();
}

void GraphicsViewNodeInstance::paint(QPainter *painter) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setRenderHint(QPainter::HighQualityAntialiasing, true);
    painter->setRenderHint(QPainter::NonCosmeticDefaultPen, true);
    graphicsView()->render(painter, QRectF(), graphicsView()->scene()->itemsBoundingRect().toRect());
    painter->restore();
}

}
}
