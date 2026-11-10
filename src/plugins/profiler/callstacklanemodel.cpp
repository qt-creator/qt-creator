// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "callstacklanemodel.h"

#include "profilertr.h"

#include <tracing/timelineformatdata.h>

#include <utils/qtcassert.h>

#include <algorithm>
#include <vector>

using namespace Qt::StringLiterals;

using namespace std::chrono;

namespace Profiler::Internal {

// Expanded rows: 0 = the thread's header, 1.. = stack depths, root first.
constexpr int FirstDepthRow = 1;

CallStackLaneModel::CallStackLaneModel(Timeline::TimelineModelAggregator *parent, quint64 tid)
    : Timeline::TimelineModel(parent)
    , m_tid(tid)
{
    setCollapsedRowCount(1);
    setExpandedRowCount(FirstDepthRow + 1);
    setHasMixedTypesInExpandedState(true);
}

QString CallStackLaneModel::threadName() const
{
    const QString name = m_data ? m_data->threadNames.value(m_tid) : QString();
    return name.isEmpty() ? Tr::tr("Thread %1").arg(m_tid) : u"%1 (%2)"_s.arg(name).arg(m_tid);
}

void CallStackLaneModel::setTraceData(const SampleTraceData *data)
{
    QList<qsizetype> samples;
    if (data) {
        for (qsizetype i = 0; i < data->samples.size(); ++i) {
            if (data->samples.at(i).tid == m_tid)
                samples.append(i);
        }
    }
    setTraceData(data, samples);
}

// The interval between the trace's last two ticks, as the CPU usage lane has it:
// a thread is not sampled at every tick in perf and Windows traces, where it
// only is while it is on the CPU, so its own intervals can be seconds long.
static nanoseconds lastTickInterval(const SampleTraceData &data)
{
    if (data.samples.isEmpty())
        return 1us;
    const microseconds last = data.samples.last().timestamp;
    for (qsizetype i = data.samples.size() - 1; i >= 0; --i) {
        const microseconds time = data.samples.at(i).timestamp;
        if (time < last)
            return last - time;
    }
    return 1us;
}

void CallStackLaneModel::setTraceData(const SampleTraceData *data, const QList<qsizetype> &samples)
{
    clear();
    m_data = data;
    if (!m_data)
        return;

    setDisplayName(Tr::tr("Stacks of %1").arg(threadName()));

    struct Open
    {
        int label;
        nanoseconds start;
    };
    std::vector<Open> open;
    const auto close = [&](size_t keep, nanoseconds end) {
        // Shallowest first, so that an item is inserted before the ones it contains.
        for (size_t depth = keep; depth < open.size(); ++depth) {
            const Open &frame = open[depth];
            const int index = insert(frame.start, std::max(end - frame.start, 1ns), frame.label);
            m_items.insert(index, Item{int(depth), frame.label});
            m_depthCount = std::max(m_depthCount, int(depth) + 1);
        }
        open.resize(keep);
    };

    nanoseconds lastSample{0};
    for (const qsizetype index : samples) {
        const SampleTraceData::ThreadSample &sample = m_data->samples.at(index);
        QTC_ASSERT(sample.tid == m_tid, continue);
        const nanoseconds time = sample.timestamp;
        lastSample = time;

        size_t common = 0;
        while (common < open.size() && common < size_t(sample.frames.size())
               && open[common].label == sample.frames.at(int(common))) {
            ++common;
        }
        close(common, time);
        for (size_t depth = common; depth < size_t(sample.frames.size()); ++depth)
            open.push_back({sample.frames.at(int(depth)), time});
    }
    // The last frames last one sampling interval of the whole trace.
    close(0, lastSample + lastTickInterval(*m_data));

    // Frames of different depths overlap, so the items are not sorted into
    // rows by their times: the row is the stack depth.
    computeNesting();
    setExpandedRowCount(FirstDepthRow + std::max(m_depthCount, 1));
    emit contentChanged();
}

void CallStackLaneModel::clear()
{
    m_data = nullptr;
    m_items.clear();
    m_depthCount = 0;
    TimelineModel::clear();
    setCollapsedRowCount(1);
    setExpandedRowCount(FirstDepthRow + 1);
}

int CallStackLaneModel::expandedRow(int index) const
{
    return FirstDepthRow + m_items.value(index).depth;
}

int CallStackLaneModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return Timeline::kCollapsedStartRow;
}

QRgb CallStackLaneModel::color(int index) const
{
    return colorBySelectionId(index);
}

Timeline::RowLabels CallStackLaneModel::labels() const
{
    Timeline::RowLabels result;
    for (int depth = 0; depth < m_depthCount; ++depth)
        result.append({Tr::tr("Stack Level %1").arg(depth), depth});
    return result;
}

Timeline::ItemDetails CallStackLaneModel::details(int index) const
{
    Timeline::ItemDetails result;
    if (!m_data || index < 0 || index >= m_items.size())
        return result;
    const int label = m_items.at(index).label;
    if (label >= 0 && label < m_data->labels.size())
        result.insert("displayName"_L1, m_data->labels.at(label).name);
    return result;
}

Timeline::OrderedItemDetails CallStackLaneModel::orderedDetails(int index) const
{
    Timeline::OrderedItemDetails result;
    if (!m_data || index < 0 || index >= m_items.size())
        return result;
    const Item &item = m_items.at(index);
    if (item.label < 0 || item.label >= m_data->labels.size())
        return result;
    const SampleTraceData::Label &label = m_data->labels.at(item.label);
    result.title = label.name;
    result.content.append({Tr::tr("Thread"), threadName()});
    result.content.append({Tr::tr("Stack Level"), QString::number(item.depth)});
    result.content.append({Tr::tr("Start", "start time"), Timeline::formatTime(startTime(index))});
    result.content.append({Tr::tr("Duration"), Timeline::formatTime(duration(index))});
    if (!label.module.isEmpty())
        result.content.append({Tr::tr("Module"), label.module});
    if (!label.file.isEmpty())
        result.content.append({Tr::tr("Location"), u"%1:%2"_s.arg(label.file).arg(label.line)});
    return result;
}

QString CallStackLaneModel::itemLabel(int index) const
{
    if (!m_data || index < 0 || index >= m_items.size() || !expanded())
        return {};
    const int label = m_items.at(index).label;
    return label >= 0 && label < m_data->labels.size() ? m_data->labels.at(label).name : QString();
}

void CallStackLaneModel::navigateToDetail(int itemIndex, int detailRow)
{
    Q_UNUSED(detailRow)
    if (!m_data || itemIndex < 0 || itemIndex >= m_items.size())
        return;
    const int id = m_items.at(itemIndex).label;
    if (id < 0 || id >= m_data->labels.size())
        return;
    const SampleTraceData::Label &label = m_data->labels.at(id);
    if (label.file.isEmpty() && label.module.isEmpty() && label.offset == 0)
        return;
    emit gotoSourceLocation(label.file, label.line, 0, label.module, label.offset);
}

} // namespace Profiler::Internal
