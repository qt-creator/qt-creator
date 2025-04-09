// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>
#include <QDataStream>

namespace QmlDesigner {

class CompleteComponentCommand
{
    friend QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);
    friend QDebug operator <<(QDebug debug, const CompleteComponentCommand &command);

public:
    CompleteComponentCommand();
    explicit CompleteComponentCommand(const QList<qint32> &container);

    const QList<qint32> instances() const;

private:
    QList<qint32> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const CompleteComponentCommand &command);
QDataStream &operator>>(QDataStream &in, CompleteComponentCommand &command);

QDebug operator <<(QDebug debug, const CompleteComponentCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::CompleteComponentCommand)
