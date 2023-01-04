// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QString>
#include <QDataStream>
#include <QVector>

namespace QmlDesigner {

class DebugOutputCommand
{
    friend QDataStream &operator>>(QDataStream &in, DebugOutputCommand &command);
    friend bool operator ==(const DebugOutputCommand &first, const DebugOutputCommand &second);

public:
    enum Type {
        DebugType,
        WarningType,
        ErrorType,
        FatalType
    };

    DebugOutputCommand();
    explicit DebugOutputCommand(const QString &text, Type type, const QVector<qint32> &instanceIds);

    qint32 type() const;
    QString text() const;
    QVector<qint32> instanceIds() const;

private:
    QVector<qint32> m_instanceIds;
    QString m_text;
    quint32 m_type;
};

QDataStream &operator<<(QDataStream &out, const DebugOutputCommand &command);
QDataStream &operator>>(QDataStream &in, DebugOutputCommand &command);

bool operator ==(const DebugOutputCommand &first, const DebugOutputCommand &second);
QDebug operator <<(QDebug debug, const DebugOutputCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::DebugOutputCommand)
