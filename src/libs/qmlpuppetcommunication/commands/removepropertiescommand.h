// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "propertyabstractcontainer.h"

namespace QmlDesigner {

class RemovePropertiesCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command);
    friend QDebug operator <<(QDebug debug, const RemovePropertiesCommand &command);

public:
    RemovePropertiesCommand();
    explicit RemovePropertiesCommand(const QVector<PropertyAbstractContainer> &properties);

    const QVector<PropertyAbstractContainer> properties() const;

private:
    QVector<PropertyAbstractContainer> m_properties;
};

QDataStream &operator<<(QDataStream &out, const RemovePropertiesCommand &command);
QDataStream &operator>>(QDataStream &in, RemovePropertiesCommand &command);

QDebug operator <<(QDebug debug, const RemovePropertiesCommand &command);

}

Q_DECLARE_METATYPE(QmlDesigner::RemovePropertiesCommand)
