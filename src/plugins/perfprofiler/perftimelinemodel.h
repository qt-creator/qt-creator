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

#pragma once

#include "perfprofilertracemanager.h"
#include "perftimelinemodelmanager.h"
#include "perfresourcecounter.h"

#include <tracing/timelinemodel.h>

#include <QStack>

namespace PerfProfiler {
namespace Internal {

class PerfTimelineModel : public Timeline::TimelineModel
{
    Q_OBJECT
public:

    struct LocationStats {
        LocationStats() : numSamples(0), numUniqueSamples(0), stackPosition(0) {}
        int numSamples;
        int numUniqueSamples;
        int stackPosition;
    };

    enum SpecialRows {
        SpaceRow = 0,
        SamplesRow = 1,
        MaximumSpecialRow = 2
    };

    PerfTimelineModel(quint32 pid, quint32 tid, qint64 startTime, qint64 endTime,
                      PerfTimelineModelManager *parent);

    QRgb color(int index) const override;
    QVariantList labels() const override;
    QVariantMap details(int index) const override;
    QVariantMap location(int index) const override;
    int typeId(int index) const override;
    bool handlesTypeId(int typeId) const override;

    int expandedRow(int index) const override;
    int collapsedRow(int index) const override;
    float relativeHeight(int index) const override;

    void loadEvent(const PerfEvent &event, int numConcurrentThreads);
    void finalize();

    void clear() override;

    quint32 pid() const { return m_pid; }
    quint32 tid() const { return m_tid; }

    qint64 threadStartTimestamp() const { return m_threadStartTimestamp + 1; }
    qint64 threadEndTimestamp() const { return m_threadEndTimestamp - 1; }

    void setSamplingFrequency(qint64 samplingFrequency) { m_samplingFrequency = samplingFrequency; }

    bool isResourceTracePoint(int index) const;
    float resourceUsage(int index) const;
    bool isSample(int index) const
    {
        return selectionId(index) <= PerfEvent::LastSpecialTypeId;
    }

    int numAttributes(int index) const;
    qint32 attributeId(int index, int i) const;
    quint64 attributeValue(int index, int i) const;

    QHash<qint32, QVariant> extraData(int index) const { return m_extraData.value(index); }

    qint64 rowMinValue(int rowNumber) const override;
    qint64 rowMaxValue(int rowNumber) const override;
    QList<const Timeline::TimelineRenderPass *> supportedRenderPasses() const override;

    void addLostEvent(qint64 timestamp, int numConcurrentThreads);

private:
    struct StackFrame {
        int numSamples = 1;
        int numExpectedParallelSamples = 1;
        int displayRowCollapsed = MaximumSpecialRow;
        int displayRowExpanded = MaximumSpecialRow;

        quint64 attributeValue = 0;
        qint64 resourcePeak = 0;
        qint64 resourceDelta = 0;
        int resourceGuesses = 0;
        int numAttributes = 0;

        static StackFrame sampleFrame()
        {
            StackFrame sample = StackFrame();
            sample.displayRowCollapsed = sample.displayRowExpanded = SamplesRow;
            return sample;
        }

        static StackFrame contentFrame(bool guessed, int numConcurrentThreads, int level,
                                       qint64 currentTotal, qint64 delta, int guesses)
        {
            StackFrame stackFrame = StackFrame();
            stackFrame.numSamples = guessed ? -1 : 1;
            stackFrame.numExpectedParallelSamples = numConcurrentThreads;
            stackFrame.displayRowCollapsed = level + MaximumSpecialRow;
            stackFrame.resourcePeak = currentTotal;
            stackFrame.resourceDelta = delta;
            stackFrame.resourceGuesses = guesses;
            return stackFrame;
        }
    };

    QVector<int> m_currentStack;

    qint64 m_lastTimestamp;
    qint64 m_threadStartTimestamp;
    qint64 m_threadEndTimestamp;

    PerfResourceCounter<> m_resourceBlocks;

    QVector<int> m_locationOrder;
    QHash<int, LocationStats> m_locationStats;

    quint32 m_pid;
    quint32 m_tid;
    qint64 m_samplingFrequency;

    QVector<StackFrame> m_data;
    QHash<int, QHash<qint32, QVariant>> m_extraData;
    QHash<int, QVector<QPair<qint32, quint64>>> m_attributeValues;

    void computeExpandedLevels();
    const PerfProfilerTraceManager *traceManager() const;

    const LocationStats &locationStats(int selectionId) const;

    void updateTraceData(const PerfEvent &event);
    void updateFrames(const PerfEvent &event, int numConcurrentThreads, qint64 resourceDelta,
                      int guesses);
    void addSample(const PerfEvent &event, qint64 resourceDelta,
                   int guesses);
};

} // namespace Internal
} // namespace PerfProfiler
