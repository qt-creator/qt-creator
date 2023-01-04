// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <qmetatype.h>
#include <QString>


namespace QmlDesigner {

class IdContainer
{
    friend QDataStream &operator>>(QDataStream &in, IdContainer &container);

public:
    IdContainer();
    IdContainer(qint32 instanceId, const QString &id);

    qint32 instanceId() const;
    QString id() const;

private:
    qint32 m_instanceId;
    QString m_id;
};

QDataStream &operator<<(QDataStream &out, const IdContainer &container);
QDataStream &operator>>(QDataStream &in, IdContainer &container);

QDebug operator <<(QDebug debug, const IdContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::IdContainer)
