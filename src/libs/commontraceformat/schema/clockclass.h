// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <QJsonObject>
#include <QString>
#include <QUuid>

#include <optional>

namespace CommonTraceFormat {

struct ClockOrigin
{
    // If isUnixEpoch is true, the other fields are ignored.
    bool isUnixEpoch = false;
    QString namespaceName;
    QString name;
    QString uid;
};

struct CMNTRCEFMT_EXPORT ClockClass
{
    // Internal ID (spec 5.7): sole purpose is linkage from a data stream class
    // via its `default-clock-class-id` property. Required in CTF2.
    QString id;
    QString namespaceName;
    QString name;
    QString uid;
    // CTF2 clock classes have no `uuid` (spec 5.7); identity is namespace/name/
    // uid. This field exists only for CTF1 (TSDL) backward compatibility, where
    // clocks do carry a uuid; the CTF2 JSON reader never populates it.
    std::optional<QUuid> uuid;
    QString description;
    ClockOrigin origin;
    quint64 frequency = 1000000000ULL; // Hz
    // Precision/accuracy in cycles. std::nullopt means "unknown" (spec 5.7); a
    // present value of 0 is distinct from absent.
    std::optional<quint64> precision;
    std::optional<quint64> accuracy;
    qint64 offsetSeconds = 0;
    quint64 offsetCycles = 0;

    // Non-semantic attributes (spec 5.2), preserved for round-tripping.
    QJsonObject attributes;
};

} // namespace CommonTraceFormat
