// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>

#include "informationcontainer.h"

namespace QmlDesigner {

class ChildrenChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);
    friend bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);

public:
    ChildrenChangedCommand();
    explicit ChildrenChangedCommand(qint32 parentInstanceId, const QList<qint32> &childrenInstancesconst, const QList<InformationContainer> &informationVector);

    QList<qint32> childrenInstances() const;
    qint32 parentInstanceId() const;
    QList<InformationContainer> informations() const;

    void sort();

private:
    qint32 m_parentInstanceId;
    QList<qint32> m_childrenVector;
    QList<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);

bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);
QDebug operator <<(QDebug debug, const ChildrenChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChildrenChangedCommand)
