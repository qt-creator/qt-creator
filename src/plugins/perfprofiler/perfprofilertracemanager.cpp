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

#include "perfprofilertracemanager.h"
#include "perfprofilertracefile.h"
#include "perfprofilerconstants.h"
#include "perfdatareader.h"

#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/progressmanager/futureprogress.h>

#include <utils/qtcassert.h>

#include <QFileInfo>
#include <QFutureInterface>

namespace PerfProfiler {
namespace Internal {

const QByteArray PerfProfilerTraceManager::s_resourceNamePrefix
        = QByteArrayLiteral("perfprofiler_");
const QByteArray PerfProfilerTraceManager::s_resourceReleasedIdName
        = QByteArrayLiteral("released_id");
const QByteArray PerfProfilerTraceManager::s_resourceRequestedBlocksName
        = QByteArrayLiteral("requested_blocks");
const QByteArray PerfProfilerTraceManager::s_resourceRequestedAmountName
        = QByteArrayLiteral("requested_amount");
const QByteArray PerfProfilerTraceManager::s_resourceObtainedIdName
        = QByteArrayLiteral("obtained_id");
const QByteArray PerfProfilerTraceManager::s_resourceMovedIdName
        = QByteArrayLiteral("moved_id");

class PerfProfilerEventTypeStorage : public Timeline::TraceEventTypeStorage
{
public:
    const Timeline::TraceEventType &get(int typeId) const override;
    void set(int typeId, Timeline::TraceEventType &&type) override;
    int append(Timeline::TraceEventType &&type) override;
    int size() const override;
    void clear() override;

    const std::vector<PerfEventType> &attributes() const { return m_attributes; }
    const std::vector<PerfEventType> &locations() const { return m_locations; }

private:
    std::vector<PerfEventType> m_attributes;
    std::vector<PerfEventType> m_locations;
};

PerfProfilerEventStorage::PerfProfilerEventStorage(
        const std::function<void(const QString &)> &errorHandler) :
    m_file("perfprofiler-data"),
    m_errorHandler(errorHandler)
{
    QTC_CHECK(m_file.open());
}

int PerfProfilerEventStorage::append(Timeline::TraceEvent &&event)
{
    QTC_ASSERT(event.is<PerfEvent>(), return m_size);
    m_file.append(std::move(event.asRvalueRef<PerfEvent>()));
    return m_size++;
}

int PerfProfilerEventStorage::size() const
{
    return m_size;
}

void PerfProfilerEventStorage::clear()
{
    m_file.clear();
    m_size = 0;
    if (!m_file.open())
        m_errorHandler(tr("Failed to reset temporary trace file."));
}

void PerfProfilerEventStorage::finalize()
{
    if (!m_file.flush())
        m_errorHandler(tr("Failed to flush temporary trace file."));
}

bool PerfProfilerEventStorage::replay(
        const std::function<bool (Timeline::TraceEvent &&)> &receiver) const
{
    switch (m_file.replay(receiver)) {
    case Timeline::TraceStashFile<PerfEvent>::ReplaySuccess:
        return true;
    case Timeline::TraceStashFile<PerfEvent>::ReplayOpenFailed:
        m_errorHandler(tr("Cannot re-open temporary trace file."));
        break;
    case Timeline::TraceStashFile<PerfEvent>::ReplayLoadFailed:
        // Happens if the loader rejects an event. Not an actual error
        break;
    case Timeline::TraceStashFile<PerfEvent>::ReplayReadPastEnd:
        m_errorHandler(tr("Read past end from temporary trace file."));
        break;
    }
    return false;
}

PerfProfilerTraceManager::PerfProfilerTraceManager(QObject *parent)
    : Timeline::TimelineTraceManager(
          std::make_unique<PerfProfilerEventStorage>(
              std::bind(&PerfProfilerTraceManager::error, this, std::placeholders::_1)),
          std::make_unique<PerfProfilerEventTypeStorage>(), parent)
{
    m_reparseTimer.setInterval(100);
    m_reparseTimer.setSingleShot(true);

    connect(this, &PerfProfilerTraceManager::aggregateAddressesChanged,
            &m_reparseTimer, QOverload<>::of(&QTimer::start));

    // Enable/Disable all triggers many threadEnabledChanged in a row
    connect(this, &PerfProfilerTraceManager::threadEnabledChanged,
            &m_reparseTimer, QOverload<>::of(&QTimer::start));

    connect(&m_reparseTimer, &QTimer::timeout,
            this, [this]() { restrictByFilter(rangeAndThreadFilter(traceStart(), traceEnd())); });

    resetAttributes();
}

PerfProfilerTraceManager::~PerfProfilerTraceManager()
{
}

void PerfProfilerTraceManager::registerFeatures(quint64 features, PerfEventLoader eventLoader,
                                                Initializer initializer, Finalizer finalizer,
                                                Clearer clearer)
{
    const TraceEventLoader traceEventLoader = eventLoader ? [eventLoader](
            const Timeline::TraceEvent &event, const Timeline::TraceEventType &type) {
        QTC_ASSERT(event.is<PerfEvent>(), return);
        QTC_ASSERT(type.is<PerfEventType>(), return);
        eventLoader(event.asConstRef<PerfEvent>(), type.asConstRef<PerfEventType>());
    } : TraceEventLoader();

    Timeline::TimelineTraceManager::registerFeatures(features, traceEventLoader, initializer,
                                                     finalizer, clearer);
}

void PerfProfilerTraceManager::clearTypeStorage()
{
    m_threads.clear();
    m_symbols.clear();
    m_tracePoints.clear();
    m_resourceObtainedIdId = -1;
    m_resourceMovedIdId = -1;
    m_resourceReleasedIdId = -1;
    m_resourceRequestedAmountId = -1;
    m_resourceRequestedBlocksId = -1;
    TimelineTraceManager::clearTypeStorage();
    resetAttributes();
}

const Timeline::TraceEventType &PerfProfilerEventTypeStorage::get(int typeId) const
{
    static const PerfEventType emptyAttribute(PerfEventType::AttributesDefinition);
    static const PerfEventType emptyLocation(PerfEventType::LocationDefinition);

    if (typeId >= 0) {
        const size_t locationId = static_cast<size_t>(typeId);
        QTC_ASSERT(locationId < m_locations.size(), return emptyLocation);
        return m_locations[locationId];
    } else {
        const size_t attributeId = static_cast<size_t>(-typeId);
        QTC_ASSERT(attributeId < m_attributes.size(), return emptyAttribute);
        return m_attributes[attributeId];
    }
}

Timeline::TimelineTraceFile *PerfProfilerTraceManager::createTraceFile()
{
    PerfProfilerTraceFile *file = new PerfProfilerTraceFile(this);
    file->setTraceManager(this);
    return file;
}

void PerfProfilerTraceManager::replayEvents(TraceEventLoader loader, Initializer initializer,
                                            Finalizer finalizer, ErrorHandler errorHandler,
                                            QFutureInterface<void> &future) const
{
    replayPerfEvents(static_cast<PerfEventLoader>(loader), initializer, finalizer, errorHandler,
                     future);
}

void PerfProfilerTraceManager::resetAttributes()
{
    // The "meta" types are useful and also have to be reported to TimelineTraceManager.
    setEventType(PerfEvent::ThreadStartTypeId, PerfEventType(PerfEventType::ThreadStart,
                                                             tr("Thread started")));
    setEventType(PerfEvent::ThreadEndTypeId, PerfEventType(PerfEventType::ThreadEnd,
                                                           tr("Thread ended")));
    setEventType(PerfEvent::LostTypeId, PerfEventType(PerfEventType::LostDefinition,
                                                      tr("Samples lost")));
}

void PerfProfilerTraceManager::finalize()
{
    TimelineTraceManager::finalize();
}

qint64 PerfProfilerTraceManager::samplingFrequency() const
{
    static const qint64 billion = 1000000000;
    return qMax(1ll, numEvents() * billion / qMax(1ll, traceDuration()));
}

PerfProfilerTraceManager::PerfEventFilter PerfProfilerTraceManager::rangeAndThreadFilter(
        qint64 rangeStart, qint64 rangeEnd) const
{
    return [rangeStart, rangeEnd, this](PerfEventLoader loader) {
        return [rangeStart, rangeEnd, this, loader](const PerfEvent &event,
                                                    const PerfEventType &type) {
            if (thread(event.tid()).enabled
                            && (rangeStart == -1 || event.timestamp() >= rangeStart)
                            && (rangeEnd == -1 || event.timestamp() <= rangeEnd)) {
                loader(event, type);
            } else if (type.feature() == PerfEventType::TracePointSample) {
                PerfEvent newEvent = event;
                newEvent.setTimestamp(-1);
                loader(newEvent, type);
            }
        };
    };
}

void PerfProfilerTraceManager::restrictByFilter(PerfProfilerTraceManager::PerfEventFilter filter)
{
    return Timeline::TimelineTraceManager::restrictByFilter([filter](TraceEventLoader loader) {
        const auto filteredPerfLoader = filter([loader](const PerfEvent &event,
                                                        const PerfEventType &type) {
            loader(event, type);
        });

        return [filteredPerfLoader](const Timeline::TraceEvent &event,
                                    const Timeline::TraceEventType &type) {
            filteredPerfLoader(static_cast<const PerfEvent &>(event),
                               static_cast<const PerfEventType &>(type));
        };
    });
}

void PerfProfilerTraceManager::replayPerfEvents(PerfEventLoader loader, Initializer initializer,
                                                Finalizer finalizer, ErrorHandler errorHandler,
                                                QFutureInterface<void> &future) const
{
    if (initializer)
        initializer();

    const auto result = eventStorage()->replay([&](Timeline::TraceEvent &&traceEvent) {
        if (future.isCanceled())
            return false;

        QTC_ASSERT(traceEvent.is<PerfEvent>(), return false);
        PerfEvent &&event = traceEvent.asRvalueRef<PerfEvent>();
        processSample(event);
        loader(event, eventType(event.typeIndex()));
        return true;
    });

    if (!result && errorHandler) {
        errorHandler(future.isCanceled() ? QString()
                                         : tr("Failed to replay Perf events from stash file."));
    } else if (result && finalizer) {
        finalizer();
    }
}

void PerfProfilerTraceManager::setThread(quint32 tid,
                                         const PerfProfilerTraceManager::Thread &thread)
{
    m_threads[tid] = thread;
}

void PerfProfilerTraceManager::setThreadEnabled(quint32 tid, bool enabled)
{
    auto it = m_threads.find(tid);
    if (it != m_threads.end() && it->enabled != enabled) {
        it->enabled = enabled;
        emit threadEnabledChanged(tid, enabled);
    }
}

const PerfProfilerTraceManager::Thread &PerfProfilerTraceManager::thread(quint32 tid) const
{
    static const Thread empty;
    auto it = m_threads.find(tid);
    return it == m_threads.end() ? empty : *it;
}

const PerfEventType &PerfProfilerTraceManager::eventType(int id) const
{
    static const PerfEventType invalid;
    const Timeline::TraceEventType &type = TimelineTraceManager::eventType(id);
    QTC_ASSERT(type.is<PerfEventType>(), return invalid);
    return type.asConstRef<PerfEventType>();
}

void PerfProfilerTraceManager::setEventType(int id, PerfEventType &&type)
{
    TimelineTraceManager::setEventType(id, std::move(type));
}

const QByteArray &PerfProfilerTraceManager::string(qint32 id) const
{
    static const QByteArray empty;
    return (id >= 0 && id < m_strings.length()) ? m_strings.at(id) : empty;
}

void PerfProfilerTraceManager::setString(qint32 id, const QByteArray &value)
{
    if (id < 0)
        return;
    if (id >= m_strings.length())
        m_strings.resize(id + 1);
    m_strings[id] = value;
    if (m_resourceObtainedIdId < 0 && value == s_resourceObtainedIdName)
        m_resourceObtainedIdId = id;
    else if (m_resourceReleasedIdId < 0 && value == s_resourceReleasedIdName)
        m_resourceReleasedIdId = id;
    else if (m_resourceRequestedAmountId < 0 && value == s_resourceRequestedAmountName)
        m_resourceRequestedAmountId = id;
    else if (m_resourceRequestedBlocksId < 0 && value == s_resourceRequestedBlocksName)
        m_resourceRequestedBlocksId = id;
    else if (m_resourceMovedIdId < 0 && value == s_resourceMovedIdName)
        m_resourceMovedIdId = id;

}

void PerfProfilerTraceManager::setSymbol(qint32 id, const PerfProfilerTraceManager::Symbol &symbol)
{
    auto i = m_symbols.find(id);
    if (i != m_symbols.end()) {
        // replace the symbol if the new one is better
        const Symbol &info = i.value();
        if ((string(info.binary).isEmpty() && !string(symbol.binary).isEmpty()) ||
                (string(info.name).isEmpty() && !string(symbol.name).isEmpty())) {
            m_symbols.erase(i);
        } else {
            return;
        }
    }

    m_symbols.insert(id, symbol);
}

void PerfProfilerEventTypeStorage::set(int id, Timeline::TraceEventType &&type)
{
    if (id >= 0) {
        const size_t locationId = static_cast<size_t>(id);
        if (m_locations.size() <= locationId)
            m_locations.resize(locationId + 1);
        QTC_ASSERT(type.is<PerfEventType>(), return);
        const PerfEventType &assigned
                = (m_locations[locationId] = std::move(type.asRvalueRef<PerfEventType>()));
        QTC_CHECK(assigned.isLocation());
    } else {
        const size_t attributeId = static_cast<size_t>(-id);
        if (m_attributes.size() <= attributeId)
            m_attributes.resize(attributeId + 1);
        QTC_ASSERT(type.is<PerfEventType>(), return);
        const PerfEventType &assigned
                = (m_attributes[attributeId] = std::move(type.asRvalueRef<PerfEventType>()));
        QTC_CHECK(assigned.isAttribute() || assigned.isMeta());
    }
}

int PerfProfilerEventTypeStorage::append(Timeline::TraceEventType &&type)
{
    QTC_ASSERT(type.is<PerfEventType>(), return -1);
    PerfEventType &&perfType = type.asRvalueRef<PerfEventType>();
    if (perfType.isLocation()) {
        const size_t index = m_locations.size();
        m_locations.push_back(perfType);
        QTC_ASSERT(index <= std::numeric_limits<int>::max(),
                   return std::numeric_limits<int>::max());
        return static_cast<int>(index);
    } else if (perfType.isAttribute()) {
        const size_t index = m_attributes.size();
        m_attributes.push_back(perfType);
        QTC_ASSERT(index <= std::numeric_limits<int>::max(),
                   return -std::numeric_limits<int>::max());
        return -static_cast<int>(index);
    }

    return -1;
}

int PerfProfilerEventTypeStorage::size() const
{
    const size_t result = m_attributes.size() + m_locations.size();
    QTC_ASSERT(result <= std::numeric_limits<int>::max(), return std::numeric_limits<int>::max());
    return static_cast<int>(result);
}

void PerfProfilerEventTypeStorage::clear()
{
    m_attributes.clear();
    m_locations.clear();
}

void PerfProfilerTraceManager::setTracePoint(qint32 id,
                                             const PerfProfilerTraceManager::TracePoint &tracePoint)
{
    m_tracePoints[id] = tracePoint;
}

const PerfProfilerTraceManager::Symbol &PerfProfilerTraceManager::symbol(qint32 id) const
{
    static const Symbol empty;
    auto it = m_symbols.find(id);
    return it != m_symbols.end() ? it.value() : empty;
}

const PerfEventType::Location &PerfProfilerTraceManager::location(qint32 id) const
{
    QTC_CHECK(id >= 0);
    return eventType(id).location();
}

const PerfProfilerTraceManager::TracePoint &PerfProfilerTraceManager::tracePoint(qint32 id) const
{
    static const TracePoint empty;
    auto it = m_tracePoints.find(id);
    return it != m_tracePoints.end() ? it.value() : empty;
}

const PerfEventType::Attribute &PerfProfilerTraceManager::attribute(qint32 id) const
{
    QTC_CHECK(id < 0);
    return eventType(id).attribute();
}

void PerfProfilerTraceManager::checkThread(const PerfEvent &event)
{
    auto it = m_threads.find(event.tid());
    if (it == m_threads.end()) {
        m_threads.insert(event.tid(),
                         Thread(event.timestamp(), event.timestamp(), event.timestamp(),
                                event.pid(), event.tid(), -1, true));
    } else {
        if (it->firstEvent < 0 || event.timestamp() < it->firstEvent)
            it->firstEvent = event.timestamp();
        if (it->lastEvent < event.timestamp())
            it->lastEvent = event.timestamp();
    }
}

void PerfProfilerTraceManager::handleTimeOrderViolations(qint64 nextEvent)
{
    if (nextEvent < traceEnd()) {
        // Ouch. a time order violation.

        std::unique_ptr<Timeline::TraceEventStorage> newStorage
                = std::make_unique<PerfProfilerEventStorage>(
                    std::bind(&PerfProfilerTraceManager::error, this, std::placeholders::_1));
        swapEventStorage(newStorage);

        clear();
        newStorage->finalize();
        initialize();

        PerfProfilerEventStorage::StashFile::Iterator iterator
                = static_cast<PerfProfilerEventStorage *>(newStorage.get())->iterator();
        while (iterator.peekNext().timestamp() <= nextEvent) {
            PerfEvent event = iterator.next();
            if (!event.origFrames().isEmpty())
                processSample(event);
            TimelineTraceManager::appendEvent(std::move(event));
            QTC_ASSERT(iterator.hasNext(), break);
        }

        ViolatedStorage violated = { std::move(newStorage), std::move(iterator) };
        m_violatedStorages.push_back(std::move(violated));
    } else if (!m_violatedStorages.empty()) {
        forever {
            auto best = m_violatedStorages.end();
            qint64 timestamp = nextEvent;
            for (auto it = m_violatedStorages.begin(), end = best; it != end; ++it) {
                const qint64 peekTimestamp = it->iterator.peekNext().timestamp();
                if (peekTimestamp <= timestamp) {
                    best = it;
                    timestamp = peekTimestamp;
                }
            }

            if (best == m_violatedStorages.end())
                break;

            PerfEvent event = best->iterator.next();
            if (!event.origFrames().isEmpty())
                processSample(event);
            TimelineTraceManager::appendEvent(std::move(event));

            if (!best->iterator.hasNext())
                m_violatedStorages.erase(best);
        }
    }
}

void PerfProfilerTraceManager::appendEvent(PerfEvent &&event)
{
    handleTimeOrderViolations(event.timestamp());
    checkThread(event);
    TimelineTraceManager::appendEvent(std::move(event));
}

void PerfProfilerTraceManager::processSample(PerfEvent &event) const
{
    QVector<qint32> frames;
    int firstGuessed = -1;
    const bool aggregating = aggregateAddresses();
    const QVector<qint32> &eventFrames = event.origFrames();
    for (int i = 0; i < eventFrames.length(); ++i) {
        if (i == eventFrames.length() - event.origNumGuessedFrames())
            firstGuessed = frames.length();
        qint32 frame = eventFrames[i];
        while (frame >= 0) {
            // It is rare, but sometimes we hit the very first byte in a symbol
            int symbolLocationId = (symbol(frame).name != -1)
                    ? frame : location(frame).parentLocationId;
            frames.append(aggregating ? symbolLocationId : frame);
            frame = (symbolLocationId >= 0) ? location(symbolLocationId).parentLocationId : -1;
        }
    }
    event.setFrames(frames);

    int numGuessed = (firstGuessed == -1) ? 0 : frames.length() - firstGuessed;
    QTC_ASSERT(numGuessed >= 0, numGuessed = 0);
    static const quint8 quint8Max = std::numeric_limits<quint8>::max();
    event.setNumGuessedFrames(numGuessed > quint8Max ? quint8Max : static_cast<quint8>(numGuessed));
}

void PerfProfilerTraceManager::appendSampleEvent(PerfEvent &&event)
{
    handleTimeOrderViolations(event.timestamp());

    // Resolving inline frames does not change the original frames
    processSample(event);
    checkThread(event);

    // dispatch / append to stash
    TimelineTraceManager::appendEvent(std::move(event));
}

void PerfProfilerTraceManager::setAggregateAddresses(bool aggregateAddresses)
{
    if (aggregateAddresses != m_aggregateAddresses) {
        m_aggregateAddresses = aggregateAddresses;
        emit aggregateAddressesChanged(aggregateAddresses);
    }
}

const std::vector<PerfEventType> &PerfProfilerTraceManager::attributes() const
{
    return static_cast<const PerfProfilerEventTypeStorage *>(typeStorage())->attributes();
}

const std::vector<PerfEventType> &PerfProfilerTraceManager::locations() const
{
    return static_cast<const PerfProfilerEventTypeStorage *>(typeStorage())->locations();
}

qint32 PerfProfilerTraceManager::symbolLocation(qint32 locationId) const
{
    return symbol(locationId).name != -1 ? locationId : location(locationId).parentLocationId;
}

void PerfProfilerTraceManager::loadFromTraceFile(const QString &filePath)
{
    Core::ProgressManager::addTask(load(filePath), tr("Loading Trace Data"),
                                   Constants::PerfProfilerTaskLoadTrace);
}

void PerfProfilerTraceManager::saveToTraceFile(const QString &filePath)
{
    Core::ProgressManager::addTask(save(filePath), tr("Saving Trace Data"),
                                   Constants::PerfProfilerTaskSaveTrace);
}

void PerfProfilerTraceManager::loadFromPerfData(const QString &filePath,
                                                const QString &executableDirPath,
                                                ProjectExplorer::Kit *kit)
{
    clearAll();
    PerfDataReader *reader = new PerfDataReader(this);
    reader->setTraceManager(this);

    connect(reader, &PerfDataReader::finished, this, [this, reader]() {
        finalize();
        reader->future().reportFinished();
        reader->deleteLater();
    });

    connect(reader, &QObject::destroyed, this, &TimelineTraceManager::loadFinished);

    const int fileMegabytes = static_cast<int>(
                qMin(QFileInfo(filePath).size() >> 20,
                     static_cast<qint64>(std::numeric_limits<int>::max())));
    Core::FutureProgress *fp = Core::ProgressManager::addTimedTask(
                reader->future(), tr("Loading Trace Data"), Constants::PerfProfilerTaskLoadPerf,
                fileMegabytes);

    connect(fp, &Core::FutureProgress::canceled, reader, [reader]() {
        reader->stopParser();
        reader->setRecording(false);
    });

    reader->future().reportStarted();
    initialize();
    reader->loadFromFile(filePath, executableDirPath, kit);
}

} // namespace Internal
} // namespace PerfProfiler
