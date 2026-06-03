// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../commontraceformat_global.h"

#include "datastreamreader.h"
#include "tracereader.h"

#include "../schema/schema.h"

#include <utils/result.h>

#include <QList>
#include <QString>

#include <memory>
#include <optional>
#include <vector>

class QIODevice;

namespace CommonTraceFormat {

// Opens a CTF trace stored as a directory and hands out ready-to-iterate data
// stream readers, hiding the storage-layout conventions that the CTF 2
// specification itself leaves unspecified:
//
//   - The `metadata` file may live in the given directory or, for an LTTng
//     recording session, in a domain (`kernel`/`ust`), per-PID, or
//     session-rotation subdirectory. Each such directory is a separate trace
//     with its own metadata; a session may contain several.
//   - A single data stream may be split across rotated tracefiles named
//     `<channel>_<cpu>_<fileindex>`; these are concatenated in index order.
//   - When LTTng index files (`index/<stream>.idx`) are present, the data
//     stream class id and stream instance id are read from them.
//
// The actual data stream class of each packet is still resolved from its packet
// header (spec 6.1), so the discovered class is only a default/hint.
class CMNTRCEFMT_EXPORT TraceDirectory
{
public:
    struct Stream
    {
        const DataStreamClass *dsc = nullptr; // default class (header may re-select)
        std::optional<quint64> streamId;      // stream instance id (LTTng index)
        DataStreamReader *reader = nullptr;   // ready to iterate
    };
    struct Trace
    {
        QString directory;
        const Schema *schema = nullptr;
        QList<Stream> streams;
    };

    // Discover and open every trace under `root`. Returns an error if no trace
    // (no `metadata` file) is found anywhere, or if a metadata fails to parse.
    static Utils::Result<TraceDirectory> open(const QString &root);

    const QList<Trace> &traces() const { return m_traces; }

    TraceDirectory(TraceDirectory &&) noexcept;
    TraceDirectory &operator=(TraceDirectory &&) noexcept;
    ~TraceDirectory();

private:
    TraceDirectory();

    // Owns the parsed schemas and the per-stream devices that the exposed
    // readers/pointers refer to. Stored via unique_ptr so addresses stay stable
    // across moves.
    std::vector<std::unique_ptr<TraceReader>> m_readers;
    std::vector<std::unique_ptr<QIODevice>> m_devices;
    QList<Trace> m_traces;
};

} // namespace CommonTraceFormat
