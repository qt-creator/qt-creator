// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "clockclass.h"
#include "datastreamclass.h"
#include "fieldclass.h"
#include "traceclass.h"

#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>
#include <QUuid>

#include <optional>

namespace CommonTraceFormat {

struct FieldClassAlias
{
    QString name;
    FieldClassPtr fieldClass;
};

struct CMNTRCEFMT_EXPORT Schema
{
    std::optional<QUuid> uuid;
    std::optional<TraceClass> traceClass;
    QList<ClockClass> clockClasses;
    QList<DataStreamClass> dataStreamClasses;

    // Named field class aliases (resolved before data stream classes).
    QHash<QString, FieldClassPtr> fieldClassAliases;
    // Declaration order of the aliases above. A later alias may reference an
    // earlier one by name (spec 5.5), so this order MUST be preserved when
    // writing the metadata stream back out (QHash iteration order is undefined).
    QStringList fieldClassAliasOrder;

    // Convenience: find a data stream class by id.
    const DataStreamClass *findDataStreamClass(quint64 id) const;
    const EventRecordClass *findEventRecordClass(
        quint64 dataStreamClassId, quint64 eventRecordClassId) const;
};

} // namespace CommonTraceFormat
