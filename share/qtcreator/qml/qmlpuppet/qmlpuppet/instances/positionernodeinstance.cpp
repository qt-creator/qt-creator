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

#include "positionernodeinstance.h"
#include <private/qdeclarativepositioners_p.h>

namespace QmlDesigner {
namespace Internal {

PositionerNodeInstance::PositionerNodeInstance(QDeclarativeBasePositioner *item)
    : QmlGraphicsItemNodeInstance(item)
{
}

bool PositionerNodeInstance::isPositioner() const
{
    return true;
}

bool PositionerNodeInstance::isResizable() const
{
    return true;
}

void PositionerNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "move" || name == "add")
        return;

    QmlGraphicsItemNodeInstance::setPropertyVariant(name, value);
}

void PositionerNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "move" || name == "add")
        return;

    QmlGraphicsItemNodeInstance::setPropertyBinding(name, expression);
}

PositionerNodeInstance::Pointer PositionerNodeInstance::create(QObject *object)
{
    QDeclarativeBasePositioner *positioner = qobject_cast<QDeclarativeBasePositioner*>(object);

    Q_ASSERT(positioner);

    Pointer instance(new PositionerNodeInstance(positioner));

    instance->setHasContent(!positioner->flags().testFlag(QGraphicsItem::ItemHasNoContents));
    positioner->setFlag(QGraphicsItem::ItemHasNoContents, false);

    static_cast<QDeclarativeParserStatus*>(positioner)->classBegin();

    instance->populateResetHashes();

    return instance;
}

QDeclarativeBasePositioner *PositionerNodeInstance::positioner() const
{
    Q_ASSERT(qobject_cast<QDeclarativeBasePositioner*>(object()));
    return static_cast<QDeclarativeBasePositioner*>(object());
}

void PositionerNodeInstance::refreshPositioner()
{
    bool success = QMetaObject::invokeMethod(positioner(), "prePositioning");
    Q_ASSERT(success);
}

}
} // namespace QmlDesigner
