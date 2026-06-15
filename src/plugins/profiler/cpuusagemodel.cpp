// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpuusagemodel.h"

#include "profilertr.h"

#include <utils/qtcassert.h>

#include <QHash>

using namespace Profiler;
using namespace Qt::StringLiterals;

namespace QmlProfiler::Internal {

// Timeline rows: 0 = category header, 1 = total graph, 2.. = threads.
constexpr int TotalRow = 1;
constexpr int FirstThreadRow = 2;

CpuUsageModel::CpuUsageModel(Timeline::TimelineModelAggregator *parent)
    : Timeline::TimelineModel(parent)
{
    setDisplayName(Tr::tr("CPU Usage"));
    setCollapsedRowCount(TotalRow + 1);
    setExpandedRowCount(TotalRow + 1);
}

void CpuUsageModel::setTraceData(const SampleTraceData *data)
{
    clear();
    m_data = data;
    load();
}

void CpuUsageModel::clear()
{
    m_data = nullptr;
    m_items.clear();
    m_threads.clear();
    m_ticks.clear();
    m_maxRunning = 1;
    m_traceEndNs = 0;
    TimelineModel::clear();
    setCollapsedRowCount(TotalRow + 1);
    setExpandedRowCount(TotalRow + 1);
}

void CpuUsageModel::load()
{
    if (!m_data || m_data->samples.isEmpty())
        return;

    // Group the flat sample list into ticks: all thread samples captured while
    // the task was suspended once share a timestamp.
    struct Tick
    {
        quint64 tsUs = 0;
        int running = 0;
        QList<quint64> runningTids;
    };
    QList<Tick> ticks;
    QHash<quint64, int> threadRows;
    for (const SampleTraceData::ThreadSample &sample : m_data->samples) {
        if (ticks.isEmpty() || ticks.last().tsUs != sample.tsUs)
            ticks.append({sample.tsUs, 0, {}});
        if (!threadRows.contains(sample.tid)) {
            threadRows.insert(sample.tid, int(m_threads.size()));
            m_threads.append(sample.tid);
        }
        if (sample.running) {
            ++ticks.last().running;
            ticks.last().runningTids.append(sample.tid);
        }
        m_maxRunning = qMax(m_maxRunning, ticks.last().running);
    }

    QTC_ASSERT(!ticks.isEmpty(), return); // non-empty samples imply at least one tick

    // Retain the grid (row-indexed) for per-column density aggregation.
    m_ticks.reserve(ticks.size());
    for (const Tick &tick : std::as_const(ticks)) {
        TickRow row;
        row.startNs = qint64(tick.tsUs) * 1000;
        row.running = tick.running;
        row.runningRows.reserve(tick.runningTids.size());
        for (quint64 tid : tick.runningTids)
            row.runningRows.append(FirstThreadRow + threadRows.value(tid));
        m_ticks.append(row);
    }

    // A tick's item lasts until the next tick; the last tick reuses the
    // previous interval (or 1 µs for a single-tick trace). Graph items never
    // nest, so the base model's computeNesting() is intentionally not used.
    for (qsizetype i = 0; i < ticks.size(); ++i) {
        const Tick &tick = ticks.at(i);
        const qint64 durUs = i + 1 < ticks.size()
                                 ? qint64(ticks.at(i + 1).tsUs - tick.tsUs)
                                 : (i > 0 ? qint64(tick.tsUs - ticks.at(i - 1).tsUs) : 1);
        const qint64 startNs = qint64(tick.tsUs) * 1000;
        const qint64 durNs = qMax<qint64>(durUs, 1) * 1000;

        m_items.insert(insert(startNs, durNs, 0), Item{tick.running, -1, 0});
        for (quint64 tid : tick.runningTids) {
            const int threadRow = threadRows.value(tid);
            m_items.insert(insert(startNs, durNs, 1 + threadRow),
                           Item{tick.running, threadRow, tid});
        }
    }
    m_traceEndNs = qint64(ticks.last().tsUs) * 1000;

    setExpandedRowCount(FirstThreadRow + int(m_threads.size()));
    setCollapsedRowCount(TotalRow + 1);
    // A taller total row makes the graph readable and is what triggers the
    // track painter's value-scale axis (drawn only for rows >= 60 px).
    setExpandedRowHeight(TotalRow, 3 * defaultRowHeight());
    emit contentChanged();
}

int CpuUsageModel::expandedRow(int index) const
{
    const Item &item = m_items.at(index);
    return item.threadRow < 0 ? TotalRow : FirstThreadRow + item.threadRow;
}

int CpuUsageModel::collapsedRow(int index) const
{
    Q_UNUSED(index)
    return TotalRow;
}

QRgb CpuUsageModel::color(int index) const
{
    // A constant color per row is what makes varying bar heights read as a
    // graph; a load-dependent hue would make the bars look like event ranges.
    return colorBySelectionId(index);
}

float CpuUsageModel::relativeHeight(int index) const
{
    const Item &item = m_items.at(index);
    // Each lane is scaled so its busiest moment fills the row: the total row
    // against the peak concurrent thread count, the per-thread items (folded
    // onto the total row when collapsed) likewise. Expanded per-thread rows
    // draw a full bar while the thread is on-CPU (its peak is one core).
    if (item.threadRow < 0 || !expanded())
        return float(item.running) / float(m_maxRunning);
    return 1.0f;
}

qint64 CpuUsageModel::rowMaxValue(int rowNumber) const
{
    // Per-core percentages: each on-CPU thread is 100% (one core). The total
    // row is scaled to its peak, so its full height is maxRunning * 100%; a
    // single thread row tops out at 100%.
    return rowNumber == TotalRow ? qint64(m_maxRunning) * 100 : 100;
}

Timeline::RowLabels CpuUsageModel::labels() const
{
    Timeline::RowLabels result;
    result.append({Tr::tr("Total"), 0});
    for (qsizetype i = 0; i < m_threads.size(); ++i) {
        const quint64 tid = m_threads.at(i);
        const QString name = m_data ? m_data->threadNames.value(tid) : QString();
        const QString description = name.isEmpty()
                                        ? Tr::tr("Thread %1").arg(tid)
                                        : u"%1 (%2)"_s.arg(name).arg(tid);
        result.append({description, int(i) + 1});
    }
    return result;
}

Timeline::ItemDetails CpuUsageModel::details(int index) const
{
    Timeline::ItemDetails result;
    const Item &item = m_items.at(index);
    if (item.threadRow < 0) {
        result.insert("displayName"_L1, Tr::tr("CPU Usage"));
        // Per-core convention: each on-CPU thread contributes 100% (one core).
        result.insert(Tr::tr("CPU Usage"), QString::number(item.running * 100) + u"%"_s);
        result.insert(Tr::tr("Running Threads"), QString::number(item.running));
    } else {
        const QString name = m_data ? m_data->threadNames.value(item.tid) : QString();
        result.insert("displayName"_L1,
                      name.isEmpty() ? Tr::tr("Thread %1").arg(item.tid) : name);
        result.insert(Tr::tr("State"), Tr::tr("Running"));
    }
    return result;
}

bool CpuUsageModel::rendersAsDensity() const
{
    return true;
}

bool CpuUsageModel::fillDensityColumns(int row, qint64 startNs, qint64 endNs,
                                       QList<float> &out) const
{
    const int columns = int(out.size());
    if (m_ticks.isEmpty() || columns <= 0 || endNs <= startNs)
        return false;

    const bool isTotal = row == TotalRow;
    // Accumulate active and covered tick counts per column in one pass; the
    // total row also sums running threads so its fill is avgRunning/maxRunning.
    QList<int> covered(columns, 0);
    QList<int> active(columns, 0); // thread rows: ticks the row was on-CPU
    QList<int> runSum(columns, 0); // total row: sum of running threads

    const double span = double(endNs - startNs);
    for (const TickRow &tick : m_ticks) {
        if (tick.startNs < startNs || tick.startNs > endNs)
            continue;
        int col = int(double(tick.startNs - startNs) * columns / span);
        if (col >= columns)
            col = columns - 1; // a tick exactly at endNs lands in the last column
        ++covered[col];
        if (isTotal)
            runSum[col] += tick.running;
        else if (tick.runningRows.contains(row))
            ++active[col];
    }

    for (int c = 0; c < columns; ++c) {
        if (covered[c] == 0) {
            out[c] = -1.0f; // marker: filled by the inherit pass below
            continue;
        }
        if (isTotal)
            out[c] = float(double(runSum[c]) / covered[c] / qMax(1, m_maxRunning));
        else
            out[c] = float(double(active[c]) / covered[c]);
    }

    // Columns with no tick of their own (zoomed in finer than the sample rate)
    // inherit the value of the nearest column that has one. Walk forward, then
    // backward, propagating the last computed value.
    float last = -1.0f;
    for (int c = 0; c < columns; ++c) {
        if (out[c] >= 0.0f)
            last = out[c];
        else if (last >= 0.0f)
            out[c] = last;
    }
    last = -1.0f;
    for (int c = columns - 1; c >= 0; --c) {
        if (out[c] >= 0.0f)
            last = out[c];
        else if (last >= 0.0f)
            out[c] = last;
    }
    for (int c = 0; c < columns; ++c) {
        if (out[c] < 0.0f)
            out[c] = 0.0f; // no ticks intersect the range at all
    }
    return true;
}

} // namespace QmlProfiler::Internal
