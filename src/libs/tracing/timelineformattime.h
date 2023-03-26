// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "tracing_global.h"
#include <QString>
#include <QObject>
#include <QtQml/qqml.h>
#include <limits>

namespace Timeline {
QString TRACING_EXPORT formatTime(qint64 timestamp,
                                   qint64 reference = std::numeric_limits<qint64>::max());

class TRACING_EXPORT TimeFormatter : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    Q_INVOKABLE QString format(qint64 timestamp, qint64 reference)
    {
        return formatTime(timestamp, reference);
    }
};

}
