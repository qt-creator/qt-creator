// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tracedirectory.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QRegularExpression>
#include <QtEndian>

#include <algorithm>

namespace CommonTraceFormat {
using namespace Qt::StringLiterals;

namespace {

// A read-only, random-access QIODevice that presents several files as one
// contiguous stream, reading them from disk on demand (so rotated tracefiles
// don't all have to be loaded into memory). Random access is required because
// the packet reader seeks backwards to trim over-reads.
class ConcatenatedFileDevice : public QIODevice
{
public:
    explicit ConcatenatedFileDevice(const QStringList &paths)
    {
        qint64 offset = 0;
        for (const QString &path : paths) {
            auto file = std::make_unique<QFile>(path);
            if (!file->open(QIODevice::ReadOnly))
                continue;
            const qint64 sz = file->size();
            m_segments.push_back({std::move(file), offset, sz});
            offset += sz;
        }
        m_size = offset;
    }

    bool isSequential() const override { return false; }
    qint64 size() const override { return m_size; }

protected:
    qint64 readData(char *data, qint64 maxSize) override
    {
        qint64 total = 0;
        qint64 p = pos();
        while (maxSize > 0 && p < m_size) {
            const Segment *seg = segmentAt(p);
            if (!seg)
                break;
            const qint64 within = p - seg->offset;
            if (!seg->file->seek(within))
                break;
            const qint64 got = seg->file->read(data + total, std::min(maxSize, seg->size - within));
            if (got <= 0)
                break;
            total += got;
            p += got;
            maxSize -= got;
        }
        return total; // 0 at end of stream
    }
    qint64 writeData(const char *, qint64) override { return -1; }

private:
    struct Segment
    {
        std::unique_ptr<QFile> file;
        qint64 offset = 0; // start offset within the concatenated stream
        qint64 size = 0;
    };
    const Segment *segmentAt(qint64 p) const
    {
        // Segments are stored in ascending offset order, so binary-search for
        // the last one starting at or before p rather than scanning linearly.
        auto it = std::upper_bound(
            m_segments.begin(), m_segments.end(), p, [](qint64 pos, const Segment &s) {
                return pos < s.offset;
            });
        if (it == m_segments.begin())
            return nullptr;
        --it;
        if (p < it->offset + it->size)
            return &*it;
        return nullptr;
    }

    std::vector<Segment> m_segments;
    qint64 m_size = 0;
};

// All directories under `root` that contain a `metadata` file (`root` itself
// included). A recording session nests one such directory per domain/PID/
// rotation, so several may be returned.
QStringList findTraceDirs(const QString &root)
{
    QStringList dirs;
    if (QFile::exists(QDir(root).filePath(u"metadata"_s)))
        dirs << root;
    QDirIterator it(root, QDir::Dirs | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString d = it.next();
        if (QFile::exists(QDir(d).filePath(u"metadata"_s)))
            dirs << d;
    }
    return dirs;
}

// Group the stream files of one trace directory into data streams. A file named
// `<channel>_<cpu>_<fileindex>` is one rotated chunk of the (channel, cpu)
// stream; such files are grouped and ordered by fileindex. Any other file is a
// stream of its own. The metadata file and index files are skipped.
QList<QStringList> groupStreamFiles(const QDir &dir)
{
    static const QRegularExpression rot(uR"(^(.+_\d+)_(\d+)$)"_s); // (channel_cpu)(fileindex)

    QList<QStringList> streams;
    QHash<QString, int> rotatedAt; // group key -> index in `streams`

    const QFileInfoList entries = dir.entryInfoList(QDir::Files, QDir::Name);
    for (const QFileInfo &fi : entries) {
        const QString name = fi.fileName();
        if (name == u"metadata"_s || name.endsWith(u".idx"_s) || name.endsWith(u".index"_s))
            continue;

        const QRegularExpressionMatch m = rot.match(name);
        if (!m.hasMatch()) {
            streams.append({fi.absoluteFilePath()});
            continue;
        }
        const QString key = m.captured(1);
        if (!rotatedAt.contains(key)) {
            rotatedAt.insert(key, int(streams.size()));
            streams.append(QStringList{});
        }
        streams[rotatedAt.value(key)].append(fi.absoluteFilePath());
    }

    // Order each rotated group by its fileindex (numeric, not lexicographic).
    for (QStringList &group : streams) {
        if (group.size() < 2)
            continue;
        std::sort(group.begin(), group.end(), [](const QString &a, const QString &b) {
            auto idx = [](const QString &p) {
                const QString n = QFileInfo(p).fileName();
                return n.mid(n.lastIndexOf(u'_') + 1).toLongLong();
            };
            return idx(a) < idx(b);
        });
    }
    return streams;
}

// Read the data-stream-class id and stream instance id from the first entry of
// an LTTng index file (`index/<streamFileName>.idx`). Returns false when the
// file is absent or malformed.
bool readStreamIdsFromIndex(
    const QDir &dir, const QString &streamFileName, quint64 &outDscId, quint64 &outStreamId)
{
    QFile f(dir.filePath(u"index/%1.idx"_s.arg(streamFileName)));
    if (!f.open(QIODevice::ReadOnly))
        return false;

    // Header: magic(4) + major(4) + minor(4) + entry_size(4) = 16 bytes.
    const QByteArray hdr = f.read(16);
    if (hdr.size() < 16)
        return false;
    if (qFromBigEndian<quint32>(hdr.constData()) != 0xC1F1DCC1u)
        return false;

    const quint32 entrySize = qFromBigEndian<quint32>(hdr.constData() + 12);
    // Entry (uint64 big-endian): offset, packet_size, content_size, ts_begin,
    // ts_end, events_discarded, stream_class_id@48, stream_id@56, [seq_num@64].
    // Only the first 64 bytes are read; cap the declared size so a malformed
    // header can't request a multi-gigabyte allocation below.
    static constexpr quint32 kMaxIndexEntrySize = 4096;
    if (entrySize < 64 || entrySize > kMaxIndexEntrySize)
        return false;
    const QByteArray entry = f.read(entrySize);
    if (static_cast<quint32>(entry.size()) < entrySize)
        return false;

    outDscId = qFromBigEndian<quint64>(entry.constData() + 48);
    outStreamId = qFromBigEndian<quint64>(entry.constData() + 56);
    return true;
}

} // namespace

TraceDirectory::TraceDirectory() = default;
TraceDirectory::TraceDirectory(TraceDirectory &&) noexcept = default;
TraceDirectory &TraceDirectory::operator=(TraceDirectory &&) noexcept = default;
TraceDirectory::~TraceDirectory() = default;

Utils::Result<TraceDirectory> TraceDirectory::open(const QString &root)
{
    const QStringList traceDirs = findTraceDirs(root);
    if (traceDirs.isEmpty())
        return Utils::ResultError(u"No CTF trace (no metadata file) found under %1"_s.arg(root));

    TraceDirectory td;
    for (const QString &traceDir : traceDirs) {
        const QDir dir(traceDir);

        QFile metaFile(dir.filePath(u"metadata"_s));
        if (!metaFile.open(QIODevice::ReadOnly))
            return Utils::ResultError(u"Cannot open metadata in %1"_s.arg(traceDir));

        auto readerResult = TraceReader::open(&metaFile);
        if (!readerResult)
            return Utils::ResultError(readerResult.error());

        td.m_readers.push_back(std::make_unique<TraceReader>(std::move(*readerResult)));
        TraceReader &reader = *td.m_readers.back();
        const Schema &schema = reader.schema();

        Trace trace;
        trace.directory = traceDir;
        trace.schema = &schema;

        const bool singleDsc = schema.dataStreamClasses.size() == 1;

        for (const QStringList &files : groupStreamFiles(dir)) {
            if (files.isEmpty() || schema.dataStreamClasses.isEmpty())
                continue;

            // Default class: from the LTTng index when meaningful, else the first
            // class. The packet header re-selects the real class per packet.
            const QString firstName = QFileInfo(files.first()).fileName();
            quint64 idxDscId = 0, idxStreamId = 0;
            const bool hasIndex = readStreamIdsFromIndex(dir, firstName, idxDscId, idxStreamId);

            const DataStreamClass *dsc = nullptr;
            if (hasIndex && !singleDsc)
                dsc = schema.findDataStreamClass(idxDscId);
            if (!dsc)
                dsc = &schema.dataStreamClasses.first();

            // Present the (possibly rotated) tracefiles as one stream device,
            // read from disk on demand.
            auto device = std::make_unique<ConcatenatedFileDevice>(files);
            device->open(QIODevice::ReadOnly);

            Stream stream;
            stream.dsc = dsc;
            if (hasIndex)
                stream.streamId = idxStreamId;
            stream.reader = reader.openStream(*dsc, device.get());

            td.m_devices.push_back(std::move(device));
            trace.streams.append(stream);
        }

        td.m_traces.append(std::move(trace));
    }

    return td;
}

} // namespace CommonTraceFormat
