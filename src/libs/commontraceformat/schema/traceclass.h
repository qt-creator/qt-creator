// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "fieldclass.h"

#include <QHash>
#include <QString>

namespace CommonTraceFormat {

struct CMNTRCEFMT_EXPORT TraceClass
{
    QString namespaceName;
    QString name;
    QString uid;
    // Backward-compatible environment entries (spec 5.6); non-semantic. Integer
    // values are stored as their decimal string form.
    QHash<QString, QString> environment;
    // Optional packet header field class (must be a StructureFC).
    FieldClassPtr packetHeaderFieldClass;

    // Non-semantic attributes (spec 5.2), preserved for round-tripping.
    QJsonObject attributes;
};

} // namespace CommonTraceFormat
