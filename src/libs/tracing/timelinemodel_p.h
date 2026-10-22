// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "timelinemodel.h"
#include <functional>

namespace Timeline {

struct HueChromaLookupTable {
    QRgb table[kHueRange * kChromaSteps];
    HueChromaLookupTable();

    QRgb operator()(int hue, int chroma) const
    {
        const int chromaStep = qBound(0, chroma, kChromaSteps - 1);
        return table[(((hue % kHueRange) + kHueRange) % kHueRange) * kChromaSteps + chromaStep];
    }
};

struct QualityColorLookupTable {
    static constexpr int colorsCount = 64;
    QRgb table[colorsCount];
    QualityColorLookupTable();

    QRgb operator()(double fraction) const
    {
        const int index = qBound(0, int(colorsCount * fraction), colorsCount - 1);
        return table[index];
    }
};

class TRACING_EXPORT TimelineModel::TimelineModelPrivate {
public:

    static const int DefaultRowHeight = 30;

    // Packed to 20 bytes, as a long trace holds hundreds of millions of ranges.
#pragma pack(push, 4)
    struct Range {
        Range() = default;
        Range(qint64 start, quint32 duration, int selectionId) :
            start(start), duration(duration), selectionId(selectionId) {}
        qint64 start = -1;
        quint32 duration = 0; // Encoded, see TimelineModelPrivate::duration().
        int selectionId = -1;
        int parent = -1;
        inline qint64 timestamp() const {return start;}
    };
#pragma pack(pop)
    static_assert(sizeof(Range) == 20);

    // Durations from this one on, and negative ones, are kept in longDurations. The range
    // holds this bit and their index there.
    static constexpr quint32 LongDuration = 0x80000000u;

    // endMaxima holds one entry per block of this many ranges.
    static constexpr int EndBlockShift = 6;

    TimelineModelPrivate(int modelId);

    int firstIndexNoParents(qint64 startTime) const;
    int firstEndingAfter(qint64 time) const;

    qint64 duration(const Range &range) const
    {
        return range.duration < LongDuration ? qint64(range.duration)
                                             : longDurations[range.duration & ~LongDuration];
    }
    qint64 rangeEnd(int index) const { return ranges[index].start + duration(ranges[index]); }
    quint32 encodeDuration(qint64 duration);
    void setDuration(Range &range, qint64 duration);
    void rangeInserted(int index);
    void raiseEndMaxima(int index);
    void recomputeEndMaxima(int index);

    inline int insertStart(const Range &start)
    {
        for (int i = ranges.count();;) {
            if (i == 0) {
                ranges.prepend(start);
                return 0;
            }
            const Range &range = ranges.at(--i);
            if (range.start < start.start || (range.start == start.start &&
                                              duration(range) >= duration(start))) {
                ranges.insert(++i, start);
                return i;
            }
        }
    }

    template<typename RangeDelimiter>
    static inline int lowerBound(const QList<RangeDelimiter> &container, qint64 time)
    {
        int fromIndex = 0;
        int toIndex = container.count() - 1;
        while (toIndex - fromIndex > 1) {
            int midIndex = (fromIndex + toIndex)/2;
            if (container[midIndex].timestamp() < time)
                fromIndex = midIndex;
            else
                toIndex = midIndex;
        }

        return fromIndex;
    }

    void dropSummaries()
    {
        collapsedSummary.reset();
        expandedSummary.reset();
        overviewSummary.reset();
    }

    int prevItemById(std::function<bool(int)> matchesId, qint64 time, int currentSelected) const;
    int nextItemById(std::function<bool(int)> matchesId, qint64 time, int currentSelected) const;

    QList<Range> ranges;
    QList<qint64> longDurations;
    // The latest end of the ranges up to the end of each block, so that it is sorted.
    QList<qint64> endMaxima;

    QList<int> rowOffsets;
    const int modelId;
    QString displayName;
    QString persistentId;
    QString tooltip;
    QColor categoryColor;
    bool hasMixedTypesInExpandedState;

    bool expanded;
    bool hidden;
    int expandedRowCount;
    int collapsedRowCount;

    std::optional<Summary> collapsedSummary;
    std::optional<Summary> expandedSummary;
    std::optional<Summary> overviewSummary;
};

} // namespace Timeline
