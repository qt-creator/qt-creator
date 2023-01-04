// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QMetaType>
#include <QVector>

#include "informationcontainer.h"

namespace QmlDesigner {

class InformationChangedCommand
{
    friend QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);
    friend bool operator ==(const InformationChangedCommand &first, const InformationChangedCommand &second);

public:
    InformationChangedCommand();
    explicit InformationChangedCommand(const QVector<InformationContainer> &informationVector);

    QVector<InformationContainer> informations() const;

    void sort();

private:
    QVector<InformationContainer> m_informationVector;
};

QDataStream &operator<<(QDataStream &out, const InformationChangedCommand &command);
QDataStream &operator>>(QDataStream &in, InformationChangedCommand &command);

bool operator ==(const InformationChangedCommand &first, const InformationChangedCommand &second);
QDebug operator <<(QDebug debug, const InformationChangedCommand &command);

} // namespace QmlDesigner

Q_DECLARE_METATYPE(QmlDesigner::InformationChangedCommand)
