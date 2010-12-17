/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "widgetnodeinstance.h"

#include "proxywidgetnodeinstance.h"
#include <invalidnodeinstanceexception.h>

#include <propertymetainfo.h>

namespace QmlDesigner {
namespace Internal {

WidgetNodeInstance::WidgetNodeInstance(QWidget* widget)
   : ObjectNodeInstance(widget)
{
}

WidgetNodeInstance::~WidgetNodeInstance()
{
}

WidgetNodeInstance::Pointer WidgetNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QDeclarativeContext *context, QObject  *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);


    QWidget* widget = qobject_cast<QWidget*>(object);
    if (widget == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new WidgetNodeInstance(widget));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}

void WidgetNodeInstance::paint(QPainter *painter) const
{
    Q_ASSERT(widget());

    QWidget::RenderFlags flags;
    if (!widget()->children().isEmpty())
        flags = QWidget::DrawChildren;
    else
        flags = 0;

    if (isTopLevel() && modelNode().isValid())
        widget()->render(painter, QPoint(), QRegion(), QWidget::DrawWindowBackground);
    else
        widget()->render(painter, QPoint(), QRegion(), flags);
}

bool WidgetNodeInstance::isTopLevel() const
{
    Q_ASSERT(widget());
    return widget()->isTopLevel();
}

QWidget* WidgetNodeInstance::widget() const
{
    return static_cast<QWidget*>(object());
}

bool WidgetNodeInstance::isWidget() const
{
    return true;
}

QRectF WidgetNodeInstance::boundingRect() const
{
    return widget()->frameGeometry();
}

void WidgetNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "x")
        widget()->move(value.toInt(), widget()->y());
    else if (name == "y")
        widget()->move(widget()->x(), value.toInt());
    else if (name == "width")
        widget()->resize(value.toInt(), widget()->height());
    else if (name == "height")
        widget()->resize(widget()->width(), value.toInt());
    else {
        widget()->setProperty(name.toLatin1(), value);
    }

    widget()->update();
}

QVariant WidgetNodeInstance::property(const QString &name) const
{
    return widget()->property(name.toLatin1());
}

bool WidgetNodeInstance::isVisible() const
{
    return widget()->isVisible();
}

void WidgetNodeInstance::setVisible(bool isVisible)
{
    widget()->setVisible(isVisible);
}

QPointF WidgetNodeInstance::position() const
{
    return widget()->pos();
}

QSizeF WidgetNodeInstance::size() const
{
    return widget()->size();
}

}
}
