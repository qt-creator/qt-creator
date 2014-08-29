/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SORTEDTIMELINEMODEL_H
#define SORTEDTIMELINEMODEL_H

#include "qmlprofiler_global.h"
#include <QObject>
#include <QVector>
#include <QLinkedList>

namespace QmlProfiler {

class QMLPROFILER_EXPORT SortedTimelineModel : public QObject {
    Q_OBJECT

public:
    struct Range {
        Range() : start(-1), duration(-1), typeId(-1), parent(-1) {}
        Range(qint64 start, qint64 duration, int typeId) :
            start(start), duration(duration), typeId(typeId), parent(-1) {}
        qint64 start;
        qint64 duration;
        int typeId;
        int parent;
        inline qint64 timestamp() const {return start;}
    };

    struct RangeEnd {
        RangeEnd() : startIndex(-1), end(-1) {}
        RangeEnd(int startIndex, qint64 end) :
            startIndex(startIndex), end(end) {}
        int startIndex;
        qint64 end;
        inline qint64 timestamp() const {return end;}
    };

    SortedTimelineModel(QObject *parent = 0) : QObject(parent) {}

    void clear();

    inline int count() const { return ranges.count(); }

    qint64 duration(int index) const { return ranges[index].duration; }
    qint64 startTime(int index) const { return ranges[index].start; }
    qint64 endTime(int index) const { return ranges[index].start + ranges[index].duration; }

    inline qint64 lastEndTime() const { return endTimes.last().end; }
    inline qint64 firstStartTime() const { return ranges.first().start; }

    inline const Range &range(int index) const { return ranges[index]; }

    inline int insert(qint64 startTime, qint64 duration, int typeId)
    {
        /* Doing insert-sort here is preferable as most of the time the times will actually be
         * presorted in the right way. So usually this will just result in appending. */
        int index = insertSorted(ranges, Range(startTime, duration, typeId));
        if (index < ranges.size() - 1)
            incrementStartIndices(index);
        insertSorted(endTimes, RangeEnd(index, startTime + duration));
        return index;
    }

    inline int insertStart(qint64 startTime, int typeId)
    {
        int index = insertSorted(ranges, Range(startTime, 0, typeId));
        if (index < ranges.size() - 1)
            incrementStartIndices(index);
        return index;
    }

    inline void insertEnd(int index, qint64 duration)
    {
        ranges[index].duration = duration;
        insertSorted(endTimes, RangeEnd(index, ranges[index].start + duration));
    }

    inline int firstIndex(qint64 startTime) const
    {
        int index = firstIndexNoParents(startTime);
        if (index == -1)
            return -1;
        int parent = ranges[index].parent;
        return parent == -1 ? index : parent;
    }

    inline int firstIndexNoParents(qint64 startTime) const
    {
        // in the "endtime" list, find the first event that ends after startTime
        if (endTimes.isEmpty())
            return -1;
        if (endTimes.count() == 1 || endTimes.first().end >= startTime)
            return endTimes.first().startIndex;
        if (endTimes.last().end <= startTime)
            return -1;

        return endTimes[lowerBound(endTimes, startTime) + 1].startIndex;
    }

    inline int lastIndex(qint64 endTime) const
    {
        // in the "starttime" list, find the last event that starts before endtime
        if (ranges.isEmpty() || ranges.first().start >= endTime)
            return -1;
        if (ranges.count() == 1)
            return 0;
        if (ranges.last().start <= endTime)
            return ranges.count() - 1;

        return lowerBound(ranges, endTime);
    }

protected:
    void computeNesting();

    void incrementStartIndices(int index)
    {
        for (int i = 0; i < endTimes.size(); ++i) {
            if (endTimes[i].startIndex >= index)
                endTimes[i].startIndex++;
        }
    }

    template<typename RangeDelimiter>
    static inline int insertSorted(QVector<RangeDelimiter> &container, const RangeDelimiter &item)
    {
        for (int i = container.count();;) {
            if (i == 0) {
                container.prepend(item);
                return 0;
            }
            if (container[--i].timestamp() <= item.timestamp()) {
                container.insert(++i, item);
                return i;
            }
        }
    }

    template<typename RangeDelimiter>
    static inline int lowerBound(const QVector<RangeDelimiter> &container, qint64 time)
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

    QVector<Range> ranges;
    QVector<RangeEnd> endTimes;
};

}

#endif
