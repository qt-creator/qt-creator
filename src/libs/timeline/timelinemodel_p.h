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
** conditions see http://www.qt.io/licensing.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TIMELINEMODEL_P_H
#define TIMELINEMODEL_P_H

#include "timelinemodel.h"

namespace Timeline {

class TIMELINE_EXPORT TimelineModel::TimelineModelPrivate {
public:
    static const int DefaultRowHeight = 30;

    enum IdType { SelectionId, TypeId };

    enum BoxColorProperties {
        SelectionIdHueMultiplier = 25,
        FractionHueMultiplier = 96,
        FractionHueMininimum = 10,
        Saturation = 150,
        Lightness = 166
    };

    struct Range {
        Range() : start(-1), duration(-1), selectionId(-1), parent(-1) {}
        Range(qint64 start, qint64 duration, int selectionId) :
            start(start), duration(duration), selectionId(selectionId), parent(-1) {}
        qint64 start;
        qint64 duration;
        int selectionId;
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

    TimelineModelPrivate(int modelId, const QString &displayName);
    void init(TimelineModel *q);

    int firstIndexNoParents(qint64 startTime) const;

    void incrementStartIndices(int index)
    {
        for (int i = 0; i < endTimes.size(); ++i) {
            if (endTimes[i].startIndex >= index)
                endTimes[i].startIndex++;
        }
    }

    inline int insertStart(const Range &start)
    {
        for (int i = ranges.count();;) {
            if (i == 0) {
                ranges.prepend(start);
                return 0;
            }
            const Range &range = ranges[--i];
            if (range.start < start.start || (range.start == start.start &&
                                              range.duration >= start.duration)) {
                ranges.insert(++i, start);
                return i;
            }
        }
    }

    inline int insertEnd(const RangeEnd &end)
    {
        for (int i = endTimes.count();;) {
            if (i == 0) {
                endTimes.prepend(end);
                return 0;
            }
            if (endTimes[--i].end <= end.end) {
                endTimes.insert(++i, end);
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

    int prevItemById(IdType idType, int id, qint64 time, int currentSelected) const;
    int nextItemById(IdType idType, int id, qint64 time, int currentSelected) const;

    QVector<Range> ranges;
    QVector<RangeEnd> endTimes;

    QVector<int> rowOffsets;
    const int modelId;
    const QString displayName;

    bool expanded;
    bool hidden;
    int expandedRowCount;
    int collapsedRowCount;

protected:
    TimelineModel *q_ptr;

private:
    Q_DECLARE_PUBLIC(TimelineModel)
};

} // namespace Timeline

#endif // TIMELINEMODEL_P_H
