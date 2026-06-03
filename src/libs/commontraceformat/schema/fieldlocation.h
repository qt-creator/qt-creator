// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include <QList>
#include <QString>

#include <functional>
#include <optional>

namespace CommonTraceFormat {

// Identifies where in the packet/event structure an anterior field lives.
// Used by dynamic-length arrays, variant selectors, and optional selectors.
struct CMNTRCEFMT_EXPORT FieldLocation
{
    enum class Origin {
        PacketHeader,
        PacketContext,
        EventRecordHeader,
        EventRecordCommonContext,
        EventRecordSpecificContext,
        EventRecordPayload,
    };

    // Whether the `origin` property was present. When false, resolution starts
    // from the structure field immediately containing the dependent field
    // (spec 5.3.1: "Start the field location procedure from the structure
    // field containing S").
    bool hasOrigin = false;
    Origin origin = Origin::EventRecordPayload;
    // Each element is a structure member name. std::nullopt = parent scope.
    QList<std::optional<QString>> path;

    bool operator==(const FieldLocation &o) const = default;
};

// Resolves an anterior field value by its FieldLocation. Returns std::nullopt
// when the location can't be resolved (so a legitimately-decoded value of 0 is
// not mistaken for "not found"; spec 6.4.2 mandates an error/abort in that case).
using FieldLocationResolver = std::function<std::optional<quint64>(const FieldLocation &)>;

} // namespace CommonTraceFormat
