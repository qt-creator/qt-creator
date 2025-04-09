// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>
#include <QDataStream>

namespace QmlDesigner {

class ChangeSelectionCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChangeSelectionCommand &command);
    friend QDebug operator <<(QDebug debug, const ChangeSelectionCommand &command);
    friend bool operator ==(const ChangeSelectionCommand &first,
                            const ChangeSelectionCommand &second);

public:
    ChangeSelectionCommand();
    explicit ChangeSelectionCommand(const QList<qint32> &idVector);

    QList<qint32> instanceIds() const;

private:
    QList<qint32> m_instanceIdVector;
};

QDataStream &operator<<(QDataStream &out, const ChangeSelectionCommand &command);
QDataStream &operator>>(QDataStream &in, ChangeSelectionCommand &command);
bool operator ==(const ChangeSelectionCommand &first, const ChangeSelectionCommand &second);

QDebug operator <<(QDebug debug, const ChangeSelectionCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChangeSelectionCommand)
