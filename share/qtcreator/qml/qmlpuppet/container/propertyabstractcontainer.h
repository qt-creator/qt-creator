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

#pragma once

#include <QDataStream>
#include <qmetatype.h>
#include <QString>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class PropertyAbstractContainer;

QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);

class PropertyAbstractContainer
{

    friend QDataStream &operator<<(QDataStream &out, const PropertyAbstractContainer &container);
    friend QDataStream &operator>>(QDataStream &in, PropertyAbstractContainer &container);
    friend QDebug operator <<(QDebug debug, const PropertyAbstractContainer &container);
public:
    PropertyAbstractContainer();
    PropertyAbstractContainer(qint32 instanceId, const PropertyName &name, const TypeName &dynamicTypeName);

    qint32 instanceId() const;
    PropertyName name() const;
    bool isDynamic() const;
    TypeName dynamicTypeName() const;

private:
    qint32 m_instanceId;
    PropertyName m_name;
    TypeName m_dynamicTypeName;
};

QDebug operator <<(QDebug debug, const PropertyAbstractContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::PropertyAbstractContainer)
