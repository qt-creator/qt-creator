// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracewriter.h"

#include "../metadata/metadatawriter.h"

#include <QIODevice>

namespace CommonTraceFormat {

Utils::Result<TraceWriter> TraceWriter::create(Schema schema, QIODevice *metadataDevice)
{
    MetadataWriter mw(metadataDevice);
    if (auto r = mw.write(schema); !r)
        return Utils::ResultError(r.error());
    return TraceWriter(std::move(schema));
}

TraceWriter::~TraceWriter()
{
    close();
}

DataStreamWriter *TraceWriter::openStream(const QString &streamClassName, QIODevice *dataDevice)
{
    const DataStreamClass *dsc = nullptr;
    for (const auto &d : m_schema->dataStreamClasses) {
        if (d.name == streamClassName) {
            dsc = &d;
            break;
        }
    }
    return openStreamImpl(dsc, dataDevice);
}

DataStreamWriter *TraceWriter::openStreamById(quint64 dataStreamClassId, QIODevice *dataDevice)
{
    return openStreamImpl(m_schema->findDataStreamClass(dataStreamClassId), dataDevice);
}

DataStreamWriter *TraceWriter::openStreamImpl(const DataStreamClass *dsc, QIODevice *dataDevice)
{
    if (!dsc)
        return nullptr;

    const TraceClass *tc = m_schema->traceClass ? &*m_schema->traceClass : nullptr;
    auto writer = std::make_unique<DataStreamWriter>(dataDevice, *dsc, tc, 65536, m_schema->uuid);
    m_streams.push_back(std::move(writer));
    return m_streams.back().get();
}

Utils::Result<> TraceWriter::close()
{
    Utils::Result<> result = Utils::ResultOk;
    for (auto &s : m_streams) {
        if (auto r = s->flush(); !r && result)
            result = r; // keep the first failure, but flush every stream
    }
    m_streams.clear();
    return result;
}

} // namespace CommonTraceFormat
