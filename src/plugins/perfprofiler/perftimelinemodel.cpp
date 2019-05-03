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

#include "perfdatareader.h"
#include "perftimelinemodel.h"
#include "perftimelinemodelmanager.h"
#include "perftimelineresourcesrenderpass.h"

#include <tracing/timelineformattime.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QFileInfo>

namespace PerfProfiler {
namespace Internal {

PerfTimelineModel::PerfTimelineModel(quint32 pid, quint32 tid, qint64 startTime, qint64 endTime,
                                     PerfTimelineModelManager *parent) :
    Timeline::TimelineModel(parent), m_lastTimestamp(-1), m_threadStartTimestamp(startTime - 1),
    m_threadEndTimestamp(endTime + 1), m_resourceBlocks(parent->resourceContainer(pid)),
    m_pid(pid), m_tid(tid), m_samplingFrequency(1)
{
    setCollapsedRowCount(MaximumSpecialRow);
    setExpandedRowCount(MaximumSpecialRow);
}

struct ColorTable {
    QRgb table[360 * 16];
    ColorTable() {
        for (int hue = 0; hue < 360; ++hue) {
            for (int saturation = 0; saturation < 16; ++saturation)
                table[hue * 16 + saturation] = QColor::fromHsl(hue, 75 + saturation * 12, 166).rgb();
        }
    }

    QRgb get(int hue, int saturation) const { return table[hue * 16 + saturation]; }
};

QRgb PerfTimelineModel::color(int index) const
{
    static const ColorTable table;
    const qint64 avgSampleDuration = static_cast<qint64>(1e9) / m_samplingFrequency;
    const qint64 sampleDuration = qMin(
                qMax(duration(index) / m_data[index].numExpectedParallelSamples,
                     avgSampleDuration / 2),
                avgSampleDuration * 2);
    const qint64 saturation = 10 * avgSampleDuration / sampleDuration - 5;
    QTC_ASSERT(saturation < 16, return QRgb(0));
    QTC_ASSERT(saturation >= 0, return QRgb(0));
    return table.get(qAbs(selectionId(index) * 25) % 360, static_cast<int>(saturation));
}

QVariantList PerfTimelineModel::labels() const
{
    QVariantList result;

    QVariantMap sample;
    sample.insert(QLatin1String("description"), tr("sample collected"));
    sample.insert(QLatin1String("id"), PerfEvent::LastSpecialTypeId);
    result << sample;

    const PerfProfilerTraceManager *manager = traceManager();
    const bool aggregated = manager->aggregateAddresses();
    for (int i = 0; i < m_locationOrder.length(); ++i) {
        int locationId = m_locationOrder[i];
        const PerfProfilerTraceManager::Symbol &symbol
                = manager->symbol(aggregated ? locationId : manager->symbolLocation(locationId));
        const PerfEventType::Location &location = manager->location(locationId);
        QVariantMap element;
        const QByteArray file = manager->string(location.file);
        if (!file.isEmpty()) {
            element.insert(QLatin1String("displayName"), QString::fromLatin1("%1:%2")
                           .arg(QFileInfo(QLatin1String(file)).fileName()).arg(location.line));
        } else {
            element.insert(QLatin1String("displayName"), manager->string(symbol.binary));
        }
        element.insert(QLatin1String("description"), manager->string(symbol.name));
        element.insert(QLatin1String("id"), locationId);
        result << element;
    }

    return result;
}

QString prettyPrintTraceData(const QVariant &data)
{
    switch (data.type()) {
    case QVariant::ULongLong:
        return QString::fromLatin1("0x%1").arg(data.toULongLong(), 16, 16, QLatin1Char('0'));
    case QVariant::UInt:
        return QString::fromLatin1("0x%1").arg(data.toUInt(), 8, 16, QLatin1Char('0'));
    case QVariant::List: {
        QStringList ret;
        for (const QVariant &item : data.toList())
            ret.append(prettyPrintTraceData(item));
        return QString::fromLatin1("[%1]").arg(ret.join(", "));
    }
    default:
        return data.toString();
    }
}

QString prettyPrintMemory(qint64 amount)
{
    const qint64 absAmount = qAbs(amount);

    if (absAmount < (1 << 10))
        return QString::number(amount);

    if (absAmount < (1 << 20)) {
        return QString::fromLatin1("%1k")
                .arg(QString::number(amount / static_cast<float>(1 << 10), 'f', 3));
    }

    if (absAmount < (1 << 30)) {
        return QString::fromLatin1("%1M")
                .arg(QString::number(amount / static_cast<float>(1 << 20), 'f', 3));
    }

    return QString::fromLatin1("%1G")
            .arg(QString::number(amount / static_cast<float>(1 << 30), 'f', 3));
}

static const QByteArray &orUnknown(const QByteArray &string)
{
    static const QByteArray unknown = PerfTimelineModel::tr("[unknown]").toUtf8();
    return string.isEmpty() ? unknown : string;
}

QVariantMap PerfTimelineModel::details(int index) const
{
    QVariantMap result;
    result.insert(QLatin1String("displayName"), displayName());

    const StackFrame &frame = m_data[index];

    const PerfProfilerTraceManager *manager = traceManager();
    int typeId = selectionId(index);
    if (isSample(index)) {
        const PerfEventType::Attribute &attribute = manager->attribute(typeId);
        result.insert(tr("Details"), orUnknown(manager->string(attribute.name)));
        result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                            manager->traceDuration()));
        const int guessedFrames = -frame.numSamples;
        if (guessedFrames > 0)
            result.insert(tr("Guessed"), tr("%n frames", nullptr, guessedFrames));
        for (int i = 0, end = numAttributes(index); i < end; ++i) {
            const auto &name = orUnknown(manager->string(
                manager->attribute(attributeId(index, i)).name));
            result.insert(QString::fromUtf8(name), attributeValue(index, i));
        }
        if (attribute.type == PerfEventType::TypeTracepoint) {
            const PerfProfilerTraceManager::TracePoint &tracePoint
                    = manager->tracePoint(static_cast<int>(attribute.config));
            result.insert(tr("System"), orUnknown(manager->string(tracePoint.system)));
            result.insert(tr("Name"), orUnknown(manager->string(tracePoint.name)));
            const QHash<qint32, QVariant> &extraData = m_extraData[index];
            for (auto it = extraData.constBegin(), end = extraData.constEnd(); it != end; ++it) {
                result.insertMulti(QString::fromUtf8(manager->string(it.key())),
                                   prettyPrintTraceData(it.value()));
            }
        }
        if (!m_resourceBlocks.isEmpty()) {
            result.insert(tr("Resource Usage"), prettyPrintMemory(frame.resourcePeak));
            result.insert(tr("Resource Change"), prettyPrintMemory(frame.resourceDelta));
        }
    } else if (typeId == PerfEvent::ThreadStartTypeId) {
        result.insert(tr("Details"), tr("thread started"));
        result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                            manager->traceDuration()));
    } else if (typeId == PerfEvent::ThreadEndTypeId) {
        result.insert(tr("Details"), tr("thread ended"));
        result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                            manager->traceDuration()));
    } else if (typeId == PerfEvent::LostTypeId) {
        result.insert(tr("Details"), tr("lost sample"));
        result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                            manager->traceDuration()));
    } else if (typeId == PerfEvent::ContextSwitchTypeId) {
        result.insert(tr("Details"), tr("context switch"));
        result.insert(tr("Timestamp"), Timeline::formatTime(startTime(index),
                                                            manager->traceDuration()));
    } else {
        const PerfProfilerTraceManager::Symbol &symbol
                = manager->symbol(manager->aggregateAddresses()
                                  ? typeId : manager->symbolLocation(typeId));
        result.insert(tr("Duration"), Timeline::formatTime(duration(index)));
        result.insert(tr("Samples"), qAbs(frame.numSamples));
        result.insert(tr("Details"), orUnknown(manager->string(symbol.name)));
        result.insert(tr("Binary"), orUnknown(manager->string(symbol.binary)));

        const PerfEventType::Location &location = manager->location(typeId);
        QString address = QString::fromLatin1("0x%1").arg(location.address, 1, 16);
        if (frame.numSamples < 0)
            address += tr(" (guessed from context)");
        result.insert(tr("Address"), address);

        const QByteArray &file = manager->string(location.file);
        if (!file.isEmpty()) {
            result.insert(tr("Source"), QString::fromLatin1("%1:%2")
                          .arg(QFileInfo(QLatin1String(file)).fileName()).arg(location.line));
        } else {
            result.insert(tr("Source"), tr("[unknown]"));
        }
        const LocationStats &stats = locationStats(typeId);
        result.insert(tr("Total Samples"), stats.numSamples);
        result.insert(tr("Total Unique Samples"), stats.numUniqueSamples);
        if (!m_resourceBlocks.isEmpty()) {
            result.insert(tr("Resource Peak"), prettyPrintMemory(frame.resourcePeak));
            result.insert(tr("Resource Change"), prettyPrintMemory(frame.resourceDelta));
        }
    }

    if (frame.resourceGuesses > 0)
        result.insert(tr("Resource Guesses"), prettyPrintMemory(frame.resourceGuesses));

    return result;
}

QVariantMap PerfTimelineModel::location(int index) const
{
    const PerfProfilerTraceManager *manager = traceManager();
    const int typeId = selectionId(index);
    if (typeId < 0) // not a location
        return QVariantMap();

    const PerfEventType::Location &location = manager->location(typeId);
    const QByteArray &file = manager->string(location.file);
    if (file.isEmpty())
        return QVariantMap();

    QVariantMap m;
    m[QStringLiteral("file")] = file;
    m[QStringLiteral("line")] = location.line;
    m[QStringLiteral("column")] = location.column;
    return m;
}

int PerfTimelineModel::typeId(int index) const
{
    QTC_ASSERT(index >= 0 && index < count(), return -1);
    return selectionId(index);
}

int PerfTimelineModel::expandedRow(int index) const
{
    return m_data[index].displayRowExpanded;
}

int PerfTimelineModel::collapsedRow(int index) const
{
    return m_data[index].displayRowCollapsed;
}

float PerfTimelineModel::relativeHeight(int index) const
{
    const StackFrame &frame = m_data[index];
    if (frame.displayRowCollapsed < MaximumSpecialRow)
        return 1.0f;
    return frame.numSamples < 0 ? 0.1f : 1.0f;
}

void PerfTimelineModel::updateTraceData(const PerfEvent &event)
{
    const PerfProfilerTraceManager *manager = traceManager();
    for (int i = 0; i < event.numAttributes(); ++i) {
        const PerfEventType::Attribute &attribute = manager->attribute(event.attributeId(i));
        if (attribute.type != PerfEventType::TypeTracepoint)
            continue;

        const PerfProfilerTraceManager::TracePoint &tracePoint
                = manager->tracePoint(static_cast<int>(attribute.config));

        const QByteArray &name = manager->string(tracePoint.name);
        if (name.startsWith(PerfProfilerTraceManager::s_resourceNamePrefix)) {
            const QHash<qint32, QVariant> &traceData = event.traceData();
            const auto end = traceData.end();

            const auto released = traceData.find(manager->resourceReleasedIdId());
            const auto amount = traceData.find(manager->resourceRequestedAmountId());
            const auto obtained = traceData.find(manager->resourceObtainedIdId());
            const auto moved = traceData.find(manager->resourceMovedIdId());

            if (amount != end) {
                const auto blocks = traceData.find(manager->resourceRequestedBlocksId());
                const qint64 amountValue = amount.value().toLongLong()
                        * (blocks == end ? 1 : blocks.value().toLongLong());

                if (released != end)
                    m_resourceBlocks.request(amountValue, released.value().toULongLong());
                else
                    m_resourceBlocks.request(amountValue);
            } else if (released != end) {
                m_resourceBlocks.release(released.value().toULongLong());
            }

            if (obtained != end)
                m_resourceBlocks.obtain(obtained.value().toULongLong());

            if (moved != end)
                m_resourceBlocks.move(moved.value().toULongLong());
        }
    }
}

void PerfTimelineModel::updateFrames(const PerfEvent &event, int numConcurrentThreads,
                                     qint64 resourceDelta, int resourceGuesses)
{
    static const int intMax = std::numeric_limits<int>::max();
    const QVector<qint32> &frames = event.frames();

    const int numFrames = frames.length();
    const qint64 sampleStart = m_lastTimestamp < 0
            ? (m_threadStartTimestamp < 0 ? event.timestamp() - 1 : m_threadStartTimestamp)
            : (m_lastTimestamp + event.timestamp()) / 2 - 1;
    const qint64 resources = m_resourceBlocks.currentTotal();

    int level = 0;
    bool parentReplaced = false;
    for (int framePosition = numFrames - 1; framePosition >= 0; --framePosition) {
        const int locationId = frames[framePosition];
        const bool guessed = (framePosition >= numFrames - event.numGuessedFrames());
        if (m_currentStack.length() > level) {
            StackFrame &frame = m_data[m_currentStack[level]];
            if (parentReplaced || selectionId(m_currentStack[level]) != locationId
                    || guessed != (frame.numSamples < 0)) {
                TimelineModel::insertEnd(m_currentStack[level],
                                         sampleStart - startTime(m_currentStack[level]));
                const int id = TimelineModel::insertStart(sampleStart, locationId);
                m_currentStack[level] = id;
                m_data.insert(id, StackFrame::contentFrame(guessed, numConcurrentThreads, level,
                                                           resources, resourceDelta,
                                                           resourceGuesses));
                parentReplaced = true;
            } else {
                frame.numSamples += (guessed ? -1 : 1);
                if (frame.numExpectedParallelSamples > intMax - numConcurrentThreads)
                    frame.numExpectedParallelSamples = intMax;
                else
                    frame.numExpectedParallelSamples += numConcurrentThreads;
                if (frame.resourcePeak < resources)
                    frame.resourcePeak = resources;
                frame.resourceDelta += resourceDelta;
                frame.resourceGuesses += resourceGuesses;
            }
        } else {
            int id = TimelineModel::insertStart(sampleStart, locationId);
            m_data.insert(id, StackFrame::contentFrame(guessed, numConcurrentThreads, level,
                                                       resources, resourceDelta, resourceGuesses));
            m_currentStack.append(id);
        }
        auto statsIter = m_locationStats.find(locationId);
        if (statsIter == m_locationStats.end())
            statsIter = m_locationStats.insert(locationId, LocationStats());

        LocationStats &stats = statsIter.value();
        ++stats.numSamples;
        bool recursion = false;
        for (int i = level - 1; i >= 0; --i) {
            if (selectionId(m_currentStack[i]) == locationId) {
                recursion = true;
                break;
            }
        }
        if (!recursion)
            ++stats.numUniqueSamples;

        QTC_CHECK(level >= 0);
        const int newStackPosition = stats.stackPosition + level;
        stats.stackPosition = newStackPosition >= stats.stackPosition
                ? newStackPosition : std::numeric_limits<int>::max();

        QTC_CHECK(level <= numFrames);
        ++level;
    }

    if (level + MaximumSpecialRow > collapsedRowCount())
        setCollapsedRowCount(level + MaximumSpecialRow);
    while (m_currentStack.length() > level) {
        TimelineModel::insertEnd(m_currentStack.last(),
                                 sampleStart - startTime(m_currentStack.last()));
        m_currentStack.pop_back();
    }
}

void PerfTimelineModel::addSample(const PerfEvent &event, qint64 resourceDelta, int resourceGuesses)
{
    const int id = TimelineModel::insert(event.timestamp(), 1, event.attributeId(0));
    StackFrame sample = StackFrame::sampleFrame();
    sample.attributeValue = event.attributeValue(0);
    sample.numSamples = event.numGuessedFrames() > 0 ? -event.numGuessedFrames() : 1;
    sample.resourcePeak = m_resourceBlocks.currentTotal();
    sample.resourceDelta = resourceDelta;
    sample.resourceGuesses = resourceGuesses;
    sample.numAttributes = event.numAttributes();
    m_data.insert(id, std::move(sample));
    const QHash<qint32, QVariant> &traceData = event.traceData();
    if (!traceData.isEmpty())
        m_extraData.insert(id, traceData);

    for (int i = 1, end = sample.numAttributes; i < end; ++i) {
        m_attributeValues[id].append(QPair<qint32, quint64>(event.attributeId(i),
                                                            event.attributeValue(i)));
    }

    m_lastTimestamp = event.timestamp();
}

void PerfTimelineModel::loadEvent(const PerfEvent &event, int numConcurrentThreads)
{
    switch (event.attributeId(0)) {
    case PerfEvent::LostTypeId: {
        QVector<int> frames;
        for (int pos = m_currentStack.length() - 1; pos >= 0; --pos)
            frames.append(selectionId(m_currentStack[pos]));

        PerfEvent guessed = event;
        guessed.setFrames(frames);
        static const int quint8Max = static_cast<int>(std::numeric_limits<quint8>::max());
        guessed.setNumGuessedFrames(static_cast<quint8>(qMin(frames.length(), quint8Max)));

        // No updateTraceData() here
        updateFrames(guessed, numConcurrentThreads, 0, 0);
        addSample(guessed, 0, 0);
        break;
    }
    case PerfEvent::ContextSwitchTypeId: {
        const int id = TimelineModel::insert(event.timestamp(), 1, PerfEvent::ContextSwitchTypeId);
        m_data.insert(id, StackFrame::sampleFrame());
        break;
    }
    case PerfEvent::ThreadStartTypeId: {
        if (m_threadStartTimestamp < 0 || event.timestamp() <= m_threadStartTimestamp)
            m_threadStartTimestamp = event.timestamp() - 1;
        const int id = TimelineModel::insert(event.timestamp(), 1, PerfEvent::ThreadStartTypeId);
        m_data.insert(id, StackFrame::sampleFrame());
        break;
    }
    case PerfEvent::ThreadEndTypeId: {
        if (m_threadEndTimestamp < 0 || event.timestamp() >= m_threadEndTimestamp)
            m_threadEndTimestamp = event.timestamp() + 1;
        while (!m_currentStack.empty()) {
            TimelineModel::insertEnd(m_currentStack.last(),
                                     m_threadEndTimestamp - startTime(m_currentStack.last()));
            m_currentStack.pop_back();
        }

        const int id = TimelineModel::insert(event.timestamp(), 1, PerfEvent::ThreadEndTypeId);
        m_data.insert(id, StackFrame::sampleFrame());
        break;
    }
    default: {
        QTC_ASSERT(event.attributeId(0) <= PerfEvent::LastSpecialTypeId, break);

        if (event.timestamp() < 0) {
            updateTraceData(event);
            break;
        }

        QTC_ASSERT(event.timestamp() >= 0, break);

        if (event.timestamp() <= m_threadStartTimestamp)
            m_threadStartTimestamp = event.timestamp() - 1;

        const qint64 total = m_resourceBlocks.currentTotal();
        const qint64 guesses = m_resourceBlocks.currentNumGuesses();
        updateTraceData(event);
        const qint64 delta = m_resourceBlocks.currentTotal() - total;
        const qint64 newGuesses = m_resourceBlocks.currentNumGuesses() - guesses;
        QTC_CHECK(newGuesses >= 0);
        QTC_CHECK(newGuesses < std::numeric_limits<int>::max());

        updateFrames(event, numConcurrentThreads, delta, static_cast<int>(newGuesses));
        addSample(event, delta, static_cast<int>(newGuesses));
        break;
    }
    }
}

void PerfTimelineModel::finalize()
{
    if (m_threadEndTimestamp <= m_lastTimestamp)
        m_threadEndTimestamp = m_lastTimestamp + 1;
    while (!m_currentStack.empty()) {
        TimelineModel::insertEnd(m_currentStack.last(),
                                 m_threadEndTimestamp - startTime(m_currentStack.last()));
        m_currentStack.pop_back();
    }

    if (isEmpty()) {
        // Empty models are hidden in the UI. We don't want that here, as it would be confusing.
        const int id = TimelineModel::insert(-1, 0, -1);
        m_data.insert(id, StackFrame::sampleFrame());
    }

    m_locationOrder.resize(m_locationStats.size());

    QMultiHash<int, LocationStats>::ConstIterator iter = m_locationStats.constBegin();
    int i = 0;
    for (; iter != m_locationStats.constEnd(); ++iter)
        m_locationOrder[i++] = iter.key();

    std::sort(m_locationOrder.begin(), m_locationOrder.end(), [this](int a, int b) -> bool {
        const LocationStats &symA = locationStats(a);
        const LocationStats &symB = locationStats(b);
        return symA.numUniqueSamples > symB.numUniqueSamples || (
                    symA.numUniqueSamples == symB.numUniqueSamples && (
                        symA.numSamples > symB.numSamples || (
                            symA.numSamples == symB.numSamples &&
                            symA.stackPosition / symA.numSamples <
                            symB.stackPosition / symB.numSamples)));
    });

    // compute range nesting
    computeNesting();

    // compute nestingLevel - expanded
    computeExpandedLevels();
}

void PerfTimelineModel::computeExpandedLevels()
{
    int expandedRows = MaximumSpecialRow;
    QHash<int, int> procLevels;
    for (int i = 0; i < m_locationOrder.length(); ++i)
        procLevels[m_locationOrder[i]] = expandedRows++;

    int eventCount = count();
    for (int i = 0; i < eventCount; i++) {
        int &expandedRow = m_data[i].displayRowExpanded;
        if (expandedRow < MaximumSpecialRow)
            continue;
        int locationId = selectionId(i);
        QTC_ASSERT(locationId >= -1, continue);
        expandedRow = procLevels[locationId];
    }
    setExpandedRowCount(expandedRows);
}

const PerfProfilerTraceManager *PerfTimelineModel::traceManager() const
{
    return static_cast<PerfTimelineModelManager *>(parent())->traceManager();
}

const PerfTimelineModel::LocationStats &PerfTimelineModel::locationStats(int locationId) const
{
    static const LocationStats empty;
    auto it = m_locationStats.constFind(locationId);
    return it == m_locationStats.constEnd() ? empty : it.value();
}

void PerfTimelineModel::clear()
{
    m_currentStack.clear();
    m_samplingFrequency = 1;
    m_pid = 0;
    m_tid = 0;
    m_lastTimestamp = -1;
    m_threadStartTimestamp = -1;
    m_threadEndTimestamp = -1;
    m_resourceBlocks.clear();
    m_locationStats.clear();
    m_locationOrder.clear();
    m_data.clear();
    m_extraData.clear();
    TimelineModel::clear();
}

bool PerfTimelineModel::isResourceTracePoint(int index) const
{
    if (!isSample(index))
        return false;

    const PerfProfilerTraceManager *manager = traceManager();

    const PerfEventType::Attribute &attribute = manager->attribute(typeId(index));
    if (attribute.type != PerfEventType::TypeTracepoint)
        return false;

    const PerfProfilerTraceManager::TracePoint &tracePoint
            = manager->tracePoint(static_cast<int>(attribute.config));

    return manager->string(tracePoint.name)
                .startsWith(PerfProfilerTraceManager::s_resourceNamePrefix);
}

float PerfTimelineModel::resourceUsage(int index) const
{
    return m_resourceBlocks.maxTotal() > m_resourceBlocks.minTotal()
            ? static_cast<float>(m_data[index].resourcePeak
                                 - m_resourceBlocks.minTotal())
              / static_cast<float>(m_resourceBlocks.maxTotal()
                                   - m_resourceBlocks.minTotal())
            : 0.0f;
}

qint32 PerfTimelineModel::attributeId(int index, int i) const
{
    return (i == 0) ? selectionId(index) : m_attributeValues[index][i].first;
}

int PerfTimelineModel::numAttributes(int index) const
{
    return m_data[index].numAttributes;
}

quint64 PerfTimelineModel::attributeValue(int index, int i) const
{
    return (i == 0) ? m_data[index].attributeValue : m_attributeValues[index][i].second;
}

bool PerfTimelineModel::handlesTypeId(int typeId) const
{
    return m_locationStats.contains(typeId);
}

qint64 PerfTimelineModel::rowMinValue(int rowNumber) const
{
    return rowNumber == SamplesRow ? m_resourceBlocks.minTotal() : 0;
}

qint64 PerfTimelineModel::rowMaxValue(int rowNumber) const
{
    return rowNumber == SamplesRow ? m_resourceBlocks.maxTotal() : 0;
}

QList<const Timeline::TimelineRenderPass *> PerfTimelineModel::supportedRenderPasses() const
{
    QList<const Timeline::TimelineRenderPass *> passes = TimelineModel::supportedRenderPasses();
    passes.append(PerfTimelineResourcesRenderPass::instance());
    return passes;
}

} // namespace Internal
} // namespace PerfProfiler
