/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "behaviornodeinstance.h"

#include <private/qdeclarativebehavior_p.h>

#include "invalidnodeinstanceexception.h"

namespace QmlDesigner {
namespace Internal {

BehaviorNodeInstance::BehaviorNodeInstance(QObject *object)
    : ObjectNodeInstance(object),
    m_isEnabled(true)
{
}

BehaviorNodeInstance::Pointer BehaviorNodeInstance::create(QObject *object)
{
    QDeclarativeBehavior* behavior = qobject_cast<QDeclarativeBehavior*>(object);

    if (behavior == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new BehaviorNodeInstance(behavior));

    instance->populateResetValueHash();

    behavior->setEnabled(false);

    return instance;
}

void BehaviorNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "enabled")
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void BehaviorNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "enabled")
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant BehaviorNodeInstance::property(const QString &name) const
{
    if (name == "enabled")
        return QVariant::fromValue(m_isEnabled);

    return ObjectNodeInstance::property(name);
}

void BehaviorNodeInstance::resetProperty(const QString &name)
{
    if (name == "enabled")
        m_isEnabled = true;

    ObjectNodeInstance::resetProperty(name);
}


} // namespace Internal
} // namespace QmlDesigner
