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

#ifndef ABSTRACTTIMELINEMODEL_P_H
#define ABSTRACTTIMELINEMODEL_P_H

#include "abstracttimelinemodel.h"

namespace QmlProfiler {

class QMLPROFILER_EXPORT AbstractTimelineModel::AbstractTimelineModelPrivate {
public:
    static const int DefaultRowHeight = 30;

    enum BoxColorProperties {
        SelectionIdHueMultiplier = 25,
        FractionHueMultiplier = 96,
        FractionHueMininimum = 10,
        Saturation = 150,
        Lightness = 166
    };

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

    void init(AbstractTimelineModel *q, const QString &displayName, QmlDebug::Message message,
              QmlDebug::RangeType rangeType);

    inline qint64 lastEndTime() const { return endTimes.last().end; }
    inline qint64 firstStartTime() const { return ranges.first().start; }

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

    void _q_dataChanged();

    QVector<Range> ranges;
    QVector<RangeEnd> endTimes;

    QVector<int> rowOffsets;
    QmlProfilerModelManager *modelManager;
    int modelId;
    bool expanded;
    bool hidden;
    int expandedRowCount;
    int collapsedRowCount;
    QString displayName;
    QmlDebug::Message message;
    QmlDebug::RangeType rangeType;

protected:
    AbstractTimelineModel *q_ptr;

private:
    Q_DECLARE_PUBLIC(AbstractTimelineModel)
};

}
#endif // ABSTRACTTIMELINEMODEL_P_H
