// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once
#include <QMetaType>
#include <QVector>
#include <QDataStream>

namespace QmlDesigner {

class ComponentCompletedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command);
    friend bool operator ==(const ComponentCompletedCommand &first, const ComponentCompletedCommand &second);

public:
    ComponentCompletedCommand();
    explicit ComponentCompletedCommand(const QVector<qint32> &container);

    QVector<qint32> instances() const;

    void sort();

private:
    QVector<qint32> m_instanceVector;
};

QDataStream &operator<<(QDataStream &out, const ComponentCompletedCommand &command);
QDataStream &operator>>(QDataStream &in, ComponentCompletedCommand &command);

bool operator ==(const ComponentCompletedCommand &first, const ComponentCompletedCommand &second);
QDebug operator <<(QDebug debug, const ComponentCompletedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ComponentCompletedCommand)
