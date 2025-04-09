// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QList>
#include <QMetaType>

#include "informationcontainer.h"

namespace QmlDesigner {

class InformationChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);
    friend bool operator ==(const InformationChangedCommand &first, const InformationChangedCommand &second);

public:
    InformationChangedCommand();
    explicit InformationChangedCommand(const QList<InformationContainer> &informationVector);

    QList<InformationContainer> informations() const;

    void sort();

private:
    QList<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command);
QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);

bool operator ==(const InformationChangedCommand &first, const InformationChangedCommand &second);
QDebug operator <<(QDebug debug, const InformationChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationChangedCommand)
