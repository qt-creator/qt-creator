// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>
#include "informationcontainer.h"

namespace QmlDesigner {

class ChildrenChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);
    friend bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);

public:
    ChildrenChangedCommand();
    explicit ChildrenChangedCommand(qint32 parentInstanceId, const QVector<qint32> &childrenInstancesconst, const QVector<InformationContainer> &informationVector);

    QVector<qint32> childrenInstances() const;
    qint32 parentInstanceId() const;
    QVector<InformationContainer> informations() const;

    void sort();

private:
    qint32 m_parentInstanceId;
    QVector<qint32> m_childrenVector;
    QVector<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const ChildrenChangedCommand &command);
QDataStream &operator>>(QDataStream &in, ChildrenChangedCommand &command);

bool operator ==(const ChildrenChangedCommand &first, const ChildrenChangedCommand &second);
QDebug operator <<(QDebug debug, const ChildrenChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::ChildrenChangedCommand)
