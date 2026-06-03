// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "datastreamreader.h"

#include "../metadata/metadatareader.h"
#include "../schema/schema.h"

#include <utils/result.h>

#include <memory>
#include <vector>

class QIODevice;

namespace CommonTraceFormat {

// Top-level trace consumer. Reads and parses the metadata stream,
// then provides per-stream DataStreamReader instances.
class CMNTRCEFMT_EXPORT TraceReader
{
public:
    static Utils::Result<TraceReader> open(QIODevice *metadataDevice);

    const Schema &schema() const { return *m_schema; }

    // Open a data stream for reading.
    DataStreamReader *openStream(const DataStreamClass &dsc, QIODevice *dataDevice);

    TraceReader(TraceReader &&) = default;
    TraceReader &operator=(TraceReader &&) = default;
    TraceReader(const TraceReader &) = delete;
    TraceReader &operator=(const TraceReader &) = delete;

private:
    explicit TraceReader(Schema schema)
        : m_schema(std::make_unique<Schema>(std::move(schema)))
    {}

    // Held behind a pointer so the Schema keeps a stable address across a move
    // of this TraceReader: the DataStreamReader instances in m_streams cache raw
    // pointers into *m_schema (the schema and its trace class), so relocating it
    // by value would leave them dangling once the moved-from object is destroyed.
    std::unique_ptr<Schema> m_schema;
    std::vector<std::unique_ptr<DataStreamReader>> m_streams;
};

} // namespace CommonTraceFormat
