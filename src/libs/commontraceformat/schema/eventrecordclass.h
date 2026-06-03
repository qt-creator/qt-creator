// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "fieldclass.h"

#include <QString>

namespace CommonTraceFormat {

struct CMNTRCEFMT_EXPORT EventRecordClass
{
    quint64 id = 0;
    QString namespaceName;
    QString name;
    QString uid;
    // All must be StructureFC or nullptr.
    FieldClassPtr specificContextFieldClass;
    FieldClassPtr payloadFieldClass;

    // Non-semantic attributes (spec 5.2), preserved for round-tripping.
    QJsonObject attributes;
};

} // namespace CommonTraceFormat
