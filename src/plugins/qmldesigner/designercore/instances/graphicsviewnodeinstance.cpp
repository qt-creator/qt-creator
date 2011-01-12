/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "graphicsviewnodeinstance.h"

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
