// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracereader.h"

#include "../metadata/ctf1packets.h"
#include "../metadata/metadatareader.h"
#include "../metadata/tsdlparser.h"

#include <QIODevice>

namespace CommonTraceFormat {

Utils::Result<TraceReader> TraceReader::open(QIODevice *metadataDevice)
{
    if (isCTF1PacketizedMetadata(metadataDevice)) {
        auto tsdlResult = readCTF1TsdlText(metadataDevice);
        if (!tsdlResult)
            return Utils::ResultError(tsdlResult.error());
        auto schemaResult = TsdlParser::parse(*tsdlResult);
        if (!schemaResult)
            return Utils::ResultError(schemaResult.error());
        return TraceReader(std::move(*schemaResult));
    }

    // CTF 1.8 also permits a non-packetized, plaintext TSDL metadata file (the
    // form perf emits). It carries no magic, so route it to the TSDL parser
    // before falling through to the CTF 2 (JSON) reader.
    if (isCTF1PlaintextMetadata(metadataDevice)) {
        const QByteArray tsdl = metadataDevice->readAll();
        auto schemaResult = TsdlParser::parse(tsdl);
        if (!schemaResult)
            return Utils::ResultError(schemaResult.error());
        return TraceReader(std::move(*schemaResult));
    }

    MetadataReader mr(metadataDevice);
    auto result = mr.read();
    if (!result)
        return Utils::ResultError(result.error());
    return TraceReader(std::move(*result));
}

DataStreamReader *TraceReader::openStream(const DataStreamClass &dsc, QIODevice *dataDevice)
{
    const TraceClass *tc = m_schema->traceClass ? &*m_schema->traceClass : nullptr;
    auto reader = std::make_unique<DataStreamReader>(dataDevice, dsc, tc, m_schema.get());
    m_streams.push_back(std::move(reader));
    return m_streams.back().get();
}

} // namespace CommonTraceFormat
