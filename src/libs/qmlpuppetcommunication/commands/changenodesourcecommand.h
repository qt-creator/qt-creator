// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>
#include <QDataStream>

namespace QmlDesigner {

class ChangeNodeSourceCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeNodeSourceCommand &command);
    friend QDebug operator <<(QDebug debug, const ChangeNodeSourceCommand &command);

public:
    ChangeNodeSourceCommand();
    explicit ChangeNodeSourceCommand(qint32 instanceId, const QString &nodeSource);
    qint32 instanceId() const;
    QString nodeSource() const;

private:
    qint32 m_instanceId;
    QString m_nodeSource;
};

QDataStream &operator<<(QDataStream &out, const ChangeNodeSourceCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeNodeSourceCommand &command);

QDebug operator <<(QDebug debug, const ChangeNodeSourceCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeNodeSourceCommand)
