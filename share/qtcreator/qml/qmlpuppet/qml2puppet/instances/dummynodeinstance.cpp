/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "dummynodeinstance.h"

namespace QmlDesigner {
namespace Internal {

DummyNodeInstance::DummyNodeInstance()
   : ObjectNodeInstance(new QObject)
{
}

DummyNodeInstance::Pointer DummyNodeInstance::create()
{
    return Pointer(new DummyNodeInstance);
}

void DummyNodeInstance::paint(QPainter * /*painter*/)
{
}

QRectF DummyNodeInstance::boundingRect() const
{
    return QRectF();
}

QPointF DummyNodeInstance::position() const
{
    return QPointF();
}

QSizeF DummyNodeInstance::size() const
{
    return QSizeF();
}

QTransform DummyNodeInstance::transform() const
{
    return QTransform();
}

double DummyNodeInstance::opacity() const
{
    return 0.0;
}

void DummyNodeInstance::setPropertyVariant(const QString &/*name*/, const QVariant &/*value*/)
{
}

void DummyNodeInstance::setPropertyBinding(const QString &/*name*/, const QString &/*expression*/)
{

}

void DummyNodeInstance::setId(const QString &/*id*/)
{

}

QVariant DummyNodeInstance::property(const QString &/*name*/) const
{
    return QVariant();
}

QStringList DummyNodeInstance::properties()
{
    return QStringList();
}

QStringList DummyNodeInstance::localProperties()
{
    return QStringList();
}

void DummyNodeInstance::initializePropertyWatcher(const ObjectNodeInstance::Pointer &/*objectNodeInstance*/)
{

}

} // namespace Internal
} // namespace QmlDesigner
