/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "behaviornodeinstance.h"

#include <qmlprivategate.h>

namespace QmlDesigner {
namespace Internal {

BehaviorNodeInstance::BehaviorNodeInstance(QObject *object)
    : ObjectNodeInstance(object),
    m_isEnabled(true)
{
}

BehaviorNodeInstance::Pointer BehaviorNodeInstance::create(QObject *object)
{
    Pointer instance(new BehaviorNodeInstance(object));

    instance->populateResetHashes();

    QmlPrivateGate::disableBehaivour(object);

    return instance;
}

QVariant BehaviorNodeInstance::property(const PropertyName &name) const
{
    if (name == "enabled")
        return QVariant::fromValue(m_isEnabled);

    return ObjectNodeInstance::property(name);
}

void BehaviorNodeInstance::resetProperty(const PropertyName &name)
{
    if (name == "enabled")
        m_isEnabled = true;

    ObjectNodeInstance::resetProperty(name);
}

PropertyNameList BehaviorNodeInstance::ignoredProperties() const
{
    return PropertyNameList({"enabled"});
}


} // namespace Internal
} // namespace QmlDesigner
