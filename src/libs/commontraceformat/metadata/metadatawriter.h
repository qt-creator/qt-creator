// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "../schema/fieldlocation.h"
#include "../schema/schema.h"

#include <utils/result.h>

#include <QJsonObject>

#include <optional>

class QIODevice;

namespace CommonTraceFormat {

// Serializes a Schema to a CTF2 metadata stream (RFC 7464 JSON text sequences).
// Each fragment is: RS (0x1E) + JSON object + LF (0x0A).
// Fragment order: preamble, field-class-aliases, trace-class, clock-classes,
//                data-stream-classes (each immediately followed by its event-record-classes).
class CMNTRCEFMT_EXPORT MetadataWriter
{
public:
    explicit MetadataWriter(QIODevice *dev);

    Utils::Result<> write(const Schema &schema);

private:
    Utils::Result<> writeFragment(const QByteArray &json);

    Utils::Result<> writePreamble(const Schema &schema);
    Utils::Result<> writeFieldClassAlias(const QString &name, const FieldClass &fc);
    Utils::Result<> writeTraceClass(const TraceClass &tc);
    Utils::Result<> writeClockClass(const ClockClass &cc);
    Utils::Result<> writeDataStreamClass(const DataStreamClass &dsc);
    Utils::Result<> writeEventRecordClass(const EventRecordClass &erc, quint64 dataStreamClassId);

    QJsonObject fieldClassToJson(const FieldClass &fc, int depth = 0) const;
    QJsonObject fieldLocationToJson(const FieldLocation &loc) const;

    QIODevice *m_dev;
    // Set when fieldClassToJson() bails out on an over-deep (possibly cyclic)
    // field-class graph; surfaced by writeFragment() so write() fails cleanly
    // instead of overflowing the stack.
    mutable std::optional<QString> m_fieldClassError;
};

} // namespace CommonTraceFormat
