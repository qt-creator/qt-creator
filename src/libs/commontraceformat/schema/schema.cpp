// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "schema.h"

namespace CommonTraceFormat {

const DataStreamClass *Schema::findDataStreamClass(quint64 id) const
{
    for (const auto &dsc : dataStreamClasses)
        if (dsc.id == id)
            return &dsc;
    return nullptr;
}

const EventRecordClass *Schema::findEventRecordClass(
    quint64 dataStreamClassId, quint64 eventRecordClassId) const
{
    const DataStreamClass *dsc = findDataStreamClass(dataStreamClassId);
    if (!dsc)
        return nullptr;
    for (const auto &erc : dsc->eventRecordClasses)
        if (erc.id == eventRecordClassId)
            return &erc;
    return nullptr;
}

} // namespace CommonTraceFormat
