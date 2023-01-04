// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDataStream>
#include <QMetaType>
#include <QVariant>
#include <QString>

#include "commondefines.h"

namespace QmlDesigner {

class InformationContainer
{
    friend QDataStream &operator>>(QDataStream &in, InformationContainer &container);
    friend QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
    friend bool operator ==(const InformationContainer &first, const InformationContainer &second);
    friend bool operator <(const InformationContainer &first, const InformationContainer &second);

public:
    InformationContainer();
    InformationContainer(qint32 instanceId,
                         InformationName name,
                         const QVariant &information,
                         const QVariant &secondInformation = QVariant(),
                         const QVariant &thirdInformation = QVariant());

    qint32 instanceId() const;
    InformationName name() const;
    QVariant information() const;
    QVariant secondInformation() const;
    QVariant thirdInformation() const;

private:
    qint32 m_instanceId;
    qint32 m_name;
    QVariant m_information;
    QVariant m_secondInformation;
    QVariant m_thirdInformation;
};

QDataStream &operator<<(QDataStream &out, const InformationContainer &container);
QDataStream &operator>>(QDataStream &in, InformationContainer &container);

bool operator ==(const InformationContainer &first, const InformationContainer &second);
bool operator <(const InformationContainer &first, const InformationContainer &second);
QDebug operator <<(QDebug debug, const InformationContainer &container);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationContainer)
