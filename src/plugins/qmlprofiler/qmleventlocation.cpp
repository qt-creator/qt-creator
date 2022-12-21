// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmleventlocation.h"
#include <QDataStream>

namespace QmlProfiler {

QDataStream &operator>>(QDataStream &stream, QmlEventLocation &location)
{
    return stream >> location.m_filename >> location.m_line >> location.m_column;
}

QDataStream &operator<<(QDataStream &stream, const QmlEventLocation &location)
{
    return stream << location.m_filename << location.m_line << location.m_column;
}

} // namespace QmlProfiler
