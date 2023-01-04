// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmetatype.h>
#include <QString>
#include <QDataStream>

#include "nodeinstanceglobal.h"

namespace QmlDesigner {

class ReparentContainer
{
    friend QDataStream &operator>>(QDataStream &in, ReparentContainer &container);
public:
    ReparentContainer();
    ReparentContainer(qint32 instanceId,
                      qint32 oldParentInstanceId,
                      const PropertyName &oldParentProperty,
                      qint32 newParentInstanceId,
                      const PropertyName &newParentProperty);

    qint32 instanceId() const;
    qint32 oldParentInstanceId() const;
    PropertyName oldParentProperty() const;
    qint32 newParentInstanceId() const;
    PropertyName newParentProperty() const;

private:
    qint32 m_instanceId;
    qint32 m_oldParentInstanceId;
    PropertyName m_oldParentProperty;
    qint32 m_newParentInstanceId;
    PropertyName m_newParentProperty;
};

QDataStream &operator<<(QDataStream &out, const ReparentContainer &container);
QDataStream &operator>>(QDataStream &in, ReparentContainer &container);

QDebug operator <<(QDebug debug, const ReparentContainer &container);

} // namespace QmlDesigner
