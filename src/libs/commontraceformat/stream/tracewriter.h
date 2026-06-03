// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "datastreamwriter.h"

#include "../schema/schema.h"

#include <utils/result.h>

#include <memory>
#include <vector>

class QIODevice;

namespace CommonTraceFormat {

// Top-level trace producer. Writes the metadata stream on construction,
// then provides per-stream DataStreamWriter instances.
class CMNTRCEFMT_EXPORT TraceWriter
{
public:
    // Writes the metadata stream immediately, returning an error if that fails.
    // On success, call openStream()/openStreamById() for each data stream.
    static Utils::Result<TraceWriter> create(Schema schema, QIODevice *metadataDevice);

    TraceWriter(TraceWriter &&) noexcept = default;
    TraceWriter &operator=(TraceWriter &&) noexcept = default;
    ~TraceWriter();

    // Returns a DataStreamWriter for the named data stream class.
    // The caller owns the QIODevice. Returns nullptr if name not found.
    DataStreamWriter *openStream(const QString &streamClassName, QIODevice *dataDevice);

    // Open a stream by data-stream-class id. Unlike the name-based overload this
    // is unambiguous when several classes share a name (or have none). Returns
    // nullptr if no class has the given id.
    DataStreamWriter *openStreamById(quint64 dataStreamClassId, QIODevice *dataDevice);

    // Flush and close every open stream. Returns the first flush error, if any.
    Utils::Result<> close();

private:
    explicit TraceWriter(Schema schema)
        : m_schema(std::make_unique<Schema>(std::move(schema)))
    {}

    DataStreamWriter *openStreamImpl(const DataStreamClass *dsc, QIODevice *dataDevice);

    // Held behind a pointer so the Schema keeps a stable address across a move of
    // this TraceWriter: the DataStreamWriter instances in m_streams cache raw
    // references/pointers into *m_schema (a data stream class and the trace
    // class), which a by-value move would leave dangling.
    std::unique_ptr<Schema> m_schema;
    std::vector<std::unique_ptr<DataStreamWriter>> m_streams;
};

} // namespace CommonTraceFormat
