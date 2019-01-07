/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "perfprofilerconstants.h"
#include "perfprofilerplugin.h"
#include "perfprofilertracefile.h"

#include <QFile>
#include <QtEndian>

namespace PerfProfiler {
namespace Internal {

Q_STATIC_ASSERT(sizeof(Constants::PerfStreamMagic) == sizeof(Constants::PerfQzfileMagic));
Q_STATIC_ASSERT(sizeof(Constants::PerfStreamMagic) == sizeof(Constants::PerfZqfileMagic));

PerfProfilerTraceFile::PerfProfilerTraceFile(QObject *parent) :
    Timeline::TimelineTraceFile(parent), m_messageSize(0),
    m_dataStreamVersion(-1), m_compressed(false), m_mangledPackets(false)
{
    // This should result in either a direct or a queued connection, depending on where the signal
    // comes from. readMessages() should always run in the GUI thread.
    connect(this, &PerfProfilerTraceFile::messagesAvailable,
            this, &PerfProfilerTraceFile::readMessages);

    connect(this, &PerfProfilerTraceFile::blockAvailable,
            this, &PerfProfilerTraceFile::readBlock);
}

void PerfProfilerTraceFile::readBlock(const QByteArray &block)
{
    QDataStream dataStream(block);
    dataStream.setVersion(m_dataStreamVersion);
    while (!dataStream.atEnd()) {
        QByteArray message;
        dataStream >> message;
        readMessages(message);
    }
}

struct PerfNrCpus {
    quint32 online = 0;
    quint32 available = 0;
};

QDataStream &operator>>(QDataStream &stream, PerfNrCpus &nrCpus)
{
    return stream >> nrCpus.online >> nrCpus.available;
}

struct PerfBuildId {
    qint32 pid = 0;
    QByteArray id;
    QByteArray fileName;
};

QDataStream &operator>>(QDataStream &stream, PerfBuildId &buildId)
{
    return stream >> buildId.pid >> buildId.id >> buildId.fileName;
}

struct PerfCpuTopology {
    QList<QByteArray> siblingCores;
    QList<QByteArray> siblingThreads;
};

QDataStream &operator>>(QDataStream &stream, PerfCpuTopology &cpuTopology)
{
    return stream >> cpuTopology.siblingCores >> cpuTopology.siblingThreads;
}

struct PerfNumaNode {
    quint32 nodeId = 0;
    quint64 memTotal = 0;
    quint64 memFree = 0;
    QByteArray topology;
};

QDataStream &operator>>(QDataStream &stream, PerfNumaNode &node)
{
    return stream >> node.nodeId >> node.memTotal >> node.memFree >> node.topology;
}

struct PerfPmu {
    quint32 type = 0;
    QByteArray name;
};

QDataStream &operator>>(QDataStream &stream, PerfPmu &pmu)
{
    return stream >> pmu.type >> pmu.name;
}

struct PerfGroupDesc {
    QByteArray name;
    quint32 leaderIndex = 0;
    quint32 numMembers = 0;
};

QDataStream &operator>>(QDataStream &stream, PerfGroupDesc &groupDesc)
{
    return stream >> groupDesc.name >> groupDesc.leaderIndex >> groupDesc.numMembers;
}

struct PerfFeatures
{
    QByteArray hostName;
    QByteArray osRelease;
    QByteArray version;
    QByteArray architecture;
    PerfNrCpus nrCpus;
    QByteArray cpuDesc;
    QByteArray cpuId;
    quint64 totalMem = 0;
    QList<QByteArray> cmdline;
    QList<PerfBuildId> buildIds;
    PerfCpuTopology cpuTopology;
    QList<PerfNumaNode> numaTopology;
    QList<PerfPmu> pmuMappings;
    QList<PerfGroupDesc> groupDescs;
};

QDataStream &operator>>(QDataStream &stream, PerfFeatures &features)
{
    return stream >> features.hostName
                  >> features.osRelease
                  >> features.version
                  >> features.architecture
                  >> features.nrCpus
                  >> features.cpuDesc
                  >> features.cpuId
                  >> features.totalMem
                  >> features.cmdline
                  >> features.buildIds
                  >> features.cpuTopology
                  >> features.numaTopology
                  >> features.pmuMappings
                  >> features.groupDescs;
}

void PerfProfilerTraceFile::readMessages(const QByteArray &buffer)
{
    PerfProfilerTraceManager *traceManager = manager();

    QDataStream dataStream(buffer);
    dataStream.setVersion(m_dataStreamVersion);
    while (!dataStream.atEnd()) {
        PerfEvent event;
        dataStream >> event;

        const qint64 timestamp = event.timestamp();
        if (timestamp > 0)
            event.setTimestamp(adjustTimestamp(timestamp));

        qint32 id = -1;
        switch (event.feature()) {
        case PerfEventType::LocationDefinition: {
            PerfEventType location(PerfEventType::LocationDefinition);
            dataStream >> id >> location;
            traceManager->setEventType(id, std::move(location));
            break;
        }
        case PerfEventType::SymbolDefinition: {
            PerfProfilerTraceManager::Symbol symbol;
            dataStream >> id >> symbol;
            traceManager->setSymbol(id, symbol);
            break;
        }
        case PerfEventType::AttributesDefinition: {
            PerfEventType attributes(PerfEventType::AttributesDefinition);
            dataStream >> id >> attributes;
            traceManager->setEventType(PerfEvent::LastSpecialTypeId - id, std::move(attributes));
            break;
        }
        case PerfEventType::StringDefinition: {
            QByteArray string;
            dataStream >> id >> string;
            traceManager->setString(id, string);
            break;
        }
        case PerfEventType::FeaturesDefinition: {
            PerfFeatures features;
            dataStream >> features;
            break;
        }
        case PerfEventType::Error: {
            qint32 errorCode;
            QString message;
            dataStream >> errorCode >> message;
            break;
        }
        case PerfEventType::Progress: {
            float progress;
            dataStream >> progress;
            break;
        }
        case PerfEventType::TracePointFormat: {
            PerfProfilerTraceManager::TracePoint tracePoint;
            dataStream >> id >> tracePoint;
            traceManager->setTracePoint(id, tracePoint);
            break;
        }
        case PerfEventType::Command: {
            PerfProfilerTraceManager::Thread thread;
            dataStream >> thread;
            traceManager->setThread(thread.tid, thread);
            break;
        }
        case PerfEventType::ThreadStart:
        case PerfEventType::ThreadEnd:
        case PerfEventType::LostDefinition:
            if (acceptsSamples())
                traceManager->appendEvent(std::move(event));
            break;
        case PerfEventType::Sample43:
        case PerfEventType::Sample:
        case PerfEventType::TracePointSample:
            if (acceptsSamples())
                traceManager->appendSampleEvent(std::move(event));
            break;
        }

        if (!m_mangledPackets) {
            if (!dataStream.atEnd())
                qWarning() << "Read only part of message";
            break;
        }
    }
}

void PerfProfilerTraceFile::readFromDevice()
{
    if (m_dataStreamVersion < 0) {
        const int magicSize = sizeof(Constants::PerfStreamMagic);
        if (m_device->bytesAvailable() < magicSize + static_cast<int>(sizeof(qint32)))
            return;

        QVarLengthArray<char> magic(magicSize);
        m_device->read(magic.data(), magicSize);
        if (strncmp(magic.data(), Constants::PerfStreamMagic, magicSize) == 0) {
            m_compressed = false;
            m_mangledPackets = false;
        } else if (strncmp(magic.data(), Constants::PerfQzfileMagic, magicSize) == 0) {
            m_compressed = true;
            m_mangledPackets = true;
        } else if (strncmp(magic.data(), Constants::PerfZqfileMagic, magicSize) == 0) {
            m_compressed = true;
            m_mangledPackets = false;
        } else {
            fail(tr("Invalid data format"));
            return;
        }

        m_device->read(reinterpret_cast<char *>(&m_dataStreamVersion), sizeof(qint32));
        m_dataStreamVersion = qFromLittleEndian(m_dataStreamVersion);

        if (m_dataStreamVersion < 0
                || m_dataStreamVersion > QDataStream::Qt_DefaultCompiledVersion) {
            fail(tr("Invalid data format"));
            return;
        }
    }

    while (m_device->bytesAvailable() >= static_cast<qint64>(sizeof(quint32))) {
        if (m_messageSize == 0) {
            m_device->read(reinterpret_cast<char *>(&m_messageSize), sizeof(quint32));
            qFromLittleEndian(m_messageSize);
        }

        if (m_device->bytesAvailable() < m_messageSize)
            return;

        QByteArray buffer(m_device->read(m_messageSize));
        m_messageSize = 0;
        if (!m_compressed)
            emit messagesAvailable(buffer);
        else if (m_mangledPackets)
            emit messagesAvailable(qUncompress(buffer));
        else
            emit blockAvailable(qUncompress(buffer));

        int progress = 0;
        if (!m_device->isSequential()) {
            progress = static_cast<int>(m_device->pos() * 1000 / m_device->size());
        } else {
            progress = future().progressValue() + 1;
            if (progress >= future().progressMaximum())
                future().setProgressRange(0, progress * 2);
        }

        if (!updateProgress(progress))
            return;
    }
}

bool PerfProfilerTraceFile::updateProgress(int progress)
{
    if (future().isCanceled())
        return false;

    future().setProgressValue(progress);
    return true;
}

void PerfProfilerTraceFile::load(QIODevice *file)
{
    setDevice(file);
    readFromDevice();

    if (!m_device->atEnd())
        fail("Device not at end after reading trace");
    else
        finish();
}

class Packet : public QDataStream {
    Q_DISABLE_COPY(Packet)
public:
    Packet(QDataStream *parent) :
        QDataStream(&m_content, QIODevice::WriteOnly), m_parent(parent)
    {
    }

    ~Packet()
    {
        (*m_parent) << m_content;
    }

    Packet(Packet &&) = delete;
    Packet &operator=(Packet &&) = delete;

private:
    QByteArray m_content;
    QDataStream *m_parent;
};

class CompressedDataStream : public QDataStream {
    Q_DISABLE_COPY(CompressedDataStream)
public:
    CompressedDataStream(QIODevice *device) :
        QDataStream(&m_content, QIODevice::WriteOnly), m_device(device)
    {
    }

    ~CompressedDataStream()
    {
        flush();
    }

    CompressedDataStream(CompressedDataStream &&) = delete;
    CompressedDataStream &operator=(CompressedDataStream &&) = delete;

    void flush()
    {
        if (!m_device.isNull() && !m_content.isEmpty()) {
            const QByteArray compressed = qCompress(m_content);
            const qint32 size = qToLittleEndian(compressed.length());
            m_device->write(reinterpret_cast<const char *>(&size), sizeof(qint32));
            m_device->write(compressed.data(), size);
            m_content.clear();
        }
        device()->reset();
    }

    void clear() { m_content.clear(); }
    int length() const { return m_content.length(); }

private:
    QByteArray m_content;
    QPointer<QIODevice> m_device;
};

void PerfProfilerTraceFile::save(QIODevice *file)
{
    m_dataStreamVersion = QDataStream::Qt_DefaultCompiledVersion;
    setDevice(file);
    writeToDevice();
}

void PerfProfilerTraceFile::writeToDevice()
{
    m_device->write(Constants::PerfZqfileMagic, sizeof(Constants::PerfZqfileMagic));
    const qint32 sendStreamVersion = qToLittleEndian(m_dataStreamVersion);
    m_device->write(reinterpret_cast<const char *>(&sendStreamVersion), sizeof(qint32));

    int progress = 0;

    const int progressPerPreprocess = future().progressMaximum() / 10;
    const PerfProfilerTraceManager *traceManager = manager();

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::StringDefinition;
        qint32 id = 0;
        for (const QByteArray &string : traceManager->strings()) {
            Packet packet(&bufferStream);
            packet << feature << id++ << string;
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::LocationDefinition;
        qint32 id = 0;
        for (const PerfEventType &location : manager()->locations()) {
            Packet packet(&bufferStream);
            packet << feature << id++ << location.location();
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::SymbolDefinition;
        const auto &symbols = traceManager->symbols();
        for (auto it = symbols.begin(), end = symbols.end(); it != end; ++it) {
            Packet packet(&bufferStream);
            packet << feature << it.key() << it.value();
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::AttributesDefinition;
        qint32 id = 0;
        for (const PerfEventType &attribute : traceManager->attributes()) {
            if (!attribute.isAttribute())
                continue; // Not incrementing id here. The IDs start at 0.
            Packet packet(&bufferStream);
            packet << feature << id++ << attribute.attribute();
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::TracePointFormat;
        const auto &tracePoints = manager()->tracePoints();
        for (auto it = tracePoints.begin(), end = tracePoints.end(); it != end; ++it) {
            Packet packet(&bufferStream);
            packet << feature << it.key() << it.value();
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        CompressedDataStream bufferStream(m_device.data());
        const quint8 feature = PerfEventType::Command;
        const auto &threads = manager()->threads();
        for (auto it = threads.begin(), end = threads.end(); it != end; ++it) {
            Packet packet(&bufferStream);
            packet << feature << it.value();
        }
    }

    if (!updateProgress(progress += progressPerPreprocess))
        return;

    {
        int remainingProgress = future().progressMaximum() - progress;
        CompressedDataStream bufferStream(m_device.data());
        int i = 0;
        traceManager->replayPerfEvents([&](const PerfEvent &event, const PerfEventType &type) {
            Q_UNUSED(type);
            Packet packet(&bufferStream);
            packet << event;

            ++i;
            if (bufferStream.length() > (1 << 25)) {
                if (updateProgress(progress + (remainingProgress * i / traceManager->numEvents())))
                    bufferStream.flush();
                else
                    bufferStream.clear();
            }
        }, nullptr, [&](){
            if (updateProgress(progress += remainingProgress))
                bufferStream.flush();
            else
                bufferStream.clear();
        }, [this](const QString &message) {
            fail(message);
        }, future());
    }
}

qint64 PerfProfilerTraceFile::adjustTimestamp(qint64 timestamp)
{
    return timestamp;
}

bool PerfProfilerTraceFile::acceptsSamples() const
{
    return true;
}

void PerfProfilerTraceFile::clear()
{
    m_messageSize = 0;
    m_dataStreamVersion = -1;
    m_compressed = false;
}

} // namespace Internal
} // namespace PerfProfiler
