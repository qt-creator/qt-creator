// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "eventrecordclass.h"
#include "fieldclass.h"

#include <QList>
#include <QString>

namespace CommonTraceFormat {

struct CMNTRCEFMT_EXPORT DataStreamClass
{
    quint64 id = 0;
    QString namespaceName;
    QString name;
    QString uid;
    // Identifier (clock class `id`) of the default clock class, if any. Named
    // for historical reasons; matched against ClockClass::id or ::name.
    QString defaultClockClassName;

    // All must be StructureFC or nullptr.
    FieldClassPtr packetContextFieldClass;
    FieldClassPtr eventRecordHeaderFieldClass;
    FieldClassPtr eventRecordCommonContextFieldClass;

    QList<EventRecordClass> eventRecordClasses;

    // Non-semantic attributes (spec 5.2), preserved for round-tripping.
    QJsonObject attributes;
};

} // namespace CommonTraceFormat
