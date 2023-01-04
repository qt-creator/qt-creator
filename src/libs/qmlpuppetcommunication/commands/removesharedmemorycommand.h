// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>
#include <QVector>

namespace QmlDesigner {

class RemoveSharedMemoryCommand
{
    friend QDataStream &operator>>(QDataStream &in, RemoveSharedMemoryCommand &command);
    friend QDebug operator <<(QDebug debug, const RemoveSharedMemoryCommand &command);

public:
    RemoveSharedMemoryCommand();
    explicit RemoveSharedMemoryCommand(const QString &typeName, const QVector<qint32> &keyNumberVector);

    QString typeName() const;
    QVector<qint32> keyNumbers() const;

private:
    QString m_typeName;
    QVector<qint32> m_keyNumberVector;
};

QDataStream &operator<<(QDataStream &out, const RemoveSharedMemoryCommand &command);
QDataStream &operator>>(QDataStream &in, RemoveSharedMemoryCommand &command);

QDebug operator <<(QDebug debug, const RemoveSharedMemoryCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::RemoveSharedMemoryCommand)
