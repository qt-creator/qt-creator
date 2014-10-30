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

#include "timelinemodel.h"
#include "timelinemodel_p.h"

#include <QLinkedList>

namespace QmlProfiler {

/*!
    \class QmlProfiler::TimelineModel
    \brief The TimelineModel class provides a sorted model for timeline data.

    The TimelineModel lets you keep range data sorted by both start and end times, so that
    visible ranges can easily be computed. The only precondition for that to work is that the ranges
    must be perfectly nested. A "parent" range of a range R is defined as a range for which the
    start time is earlier than R's start time and the end time is later than R's end time. A set
    of ranges is perfectly nested if all parent ranges of any given range have a common parent
    range. Mind that you can always make that happen by defining a range that spans the whole
    available time span. That, however, will make any code that uses firstIndex() and lastIndex()
    for selecting subsets of the model always select all of it.

    \note Indices returned from the various methods are only valid until a new range is inserted
          before them. Inserting a new range before a given index moves the range pointed to by the
          index by one. Incrementing the index by one will make it point to the item again.
*/

/*!
    Compute all ranges' parents.
    \sa firstIndex()
*/
void TimelineModel::computeNesting()
{
    Q_D(TimelineModel);
    QLinkedList<int> parents;
    for (int range = 0; range != count(); ++range) {
        TimelineModelPrivate::Range &current = d->ranges[range];
        for (QLinkedList<int>::iterator parentIt = parents.begin();;) {
            if (parentIt == parents.end()) {
                parents.append(range);
                break;
            }

            TimelineModelPrivate::Range &parent = d->ranges[*parentIt];
            qint64 parentEnd = parent.start + parent.duration;
            if (parentEnd < current.start) {
                if (parent.start == current.start) {
                    if (parent.parent == -1) {
                        parent.parent = range;
                    } else {
                        TimelineModelPrivate::Range &ancestor = d->ranges[parent.parent];
                        if (ancestor.start == current.start &&
                                ancestor.duration < current.duration)
                            parent.parent = range;
                    }
                    // Just switch the old parent range for the new, larger one
                    *parentIt = range;
                    break;
                } else {
                    parentIt = parents.erase(parentIt);
                }
            } else if (parentEnd >= current.start + current.duration) {
                // no need to insert
                current.parent = *parentIt;
                break;
            } else {
                ++parentIt;
            }
        }
    }
}

int TimelineModel::collapsedRowCount() const
{
    Q_D(const TimelineModel);
    return d->collapsedRowCount;
}

void TimelineModel::setCollapsedRowCount(int rows)
{
    Q_D(TimelineModel);
    if (d->collapsedRowCount != rows) {
        d->collapsedRowCount = rows;
        if (!d->expanded)
            emit rowCountChanged();
    }
}

int TimelineModel::expandedRowCount() const
{
    Q_D(const TimelineModel);
    return d->expandedRowCount;
}

void QmlProfiler::TimelineModel::setExpandedRowCount(int rows)
{
    Q_D(TimelineModel);
    if (d->expandedRowCount != rows) {
        d->expandedRowCount = rows;
        if (d->expanded)
            emit rowCountChanged();
    }
}


TimelineModel::TimelineModelPrivate::TimelineModelPrivate(int modelId, const QString &displayName) :
    modelId(modelId), displayName(displayName), expanded(false), hidden(false),
    expandedRowCount(1), collapsedRowCount(1), q_ptr(0)
{
}

void TimelineModel::TimelineModelPrivate::init(TimelineModel *q)
{
    q_ptr = q;
    connect(q,SIGNAL(rowHeightChanged()),q,SIGNAL(heightChanged()));
    connect(q,SIGNAL(expandedChanged()),q,SIGNAL(heightChanged()));
    connect(q,SIGNAL(hiddenChanged()),q,SIGNAL(heightChanged()));
    connect(q,SIGNAL(emptyChanged()),q,SIGNAL(heightChanged()));
}


TimelineModel::TimelineModel(TimelineModelPrivate &dd, QObject *parent) :
    QObject(parent), d_ptr(&dd)
{
    d_ptr->init(this);
}

TimelineModel::TimelineModel(int modelId, const QString &displayName, QObject *parent) :
    QObject(parent), d_ptr(new TimelineModelPrivate(modelId, displayName))
{
    d_ptr->init(this);
}

TimelineModel::~TimelineModel()
{
    Q_D(TimelineModel);
    delete d;
}

bool TimelineModel::isEmpty() const
{
    return count() == 0;
}

int TimelineModel::modelId() const
{
    Q_D(const TimelineModel);
    return d->modelId;
}

int TimelineModel::rowHeight(int rowNumber) const
{
    Q_D(const TimelineModel);
    if (!expanded())
        return TimelineModelPrivate::DefaultRowHeight;

    if (d->rowOffsets.size() > rowNumber)
        return d->rowOffsets[rowNumber] - (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    return TimelineModelPrivate::DefaultRowHeight;
}

int TimelineModel::rowOffset(int rowNumber) const
{
    Q_D(const TimelineModel);
    if (rowNumber == 0)
        return 0;
    if (!expanded())
        return TimelineModelPrivate::DefaultRowHeight * rowNumber;

    if (d->rowOffsets.size() >= rowNumber)
        return d->rowOffsets[rowNumber - 1];
    if (!d->rowOffsets.empty())
        return d->rowOffsets.last() + (rowNumber - d->rowOffsets.size()) *
                TimelineModelPrivate::DefaultRowHeight;
    return rowNumber * TimelineModelPrivate::DefaultRowHeight;
}

void TimelineModel::setRowHeight(int rowNumber, int height)
{
    Q_D(TimelineModel);
    if (d->hidden || !d->expanded)
        return;
    if (height < TimelineModelPrivate::DefaultRowHeight)
        height = TimelineModelPrivate::DefaultRowHeight;

    int nextOffset = d->rowOffsets.empty() ? 0 : d->rowOffsets.last();
    while (d->rowOffsets.size() <= rowNumber)
        d->rowOffsets << (nextOffset += TimelineModelPrivate::DefaultRowHeight);
    int difference = height - d->rowOffsets[rowNumber] +
            (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    if (difference != 0) {
        for (; rowNumber < d->rowOffsets.size(); ++rowNumber) {
            d->rowOffsets[rowNumber] += difference;
        }
        emit rowHeightChanged();
    }
}

int TimelineModel::height() const
{
    Q_D(const TimelineModel);
    if (d->hidden || isEmpty())
        return 0;

    if (!d->expanded || d->rowOffsets.empty())
        return rowCount() * TimelineModelPrivate::DefaultRowHeight;

    return d->rowOffsets.last() + (rowCount() - d->rowOffsets.size()) *
            TimelineModelPrivate::DefaultRowHeight;
}

/*!
    Returns the number of ranges in the model.
*/
int TimelineModel::count() const
{
    Q_D(const TimelineModel);
    return d->ranges.count();
}

qint64 TimelineModel::duration(int index) const
{
    Q_D(const TimelineModel);
    return d->ranges[index].duration;
}

qint64 TimelineModel::startTime(int index) const
{
    Q_D(const TimelineModel);
    return d->ranges[index].start;
}

qint64 TimelineModel::endTime(int index) const
{
    Q_D(const TimelineModel);
    return d->ranges[index].start + d->ranges[index].duration;
}

/*!
    Returns the type ID of the event with event ID \a index. The type ID is a globally valid ID
    which can be used to communicate meta information about events to other parts of the program. By
    default it is -1, which means there is no global type information about the event.
 */
int TimelineModel::typeId(int index) const
{
    Q_UNUSED(index)
    return -1;
}

/*!
    Looks up the first range with an end time later than the given time and
    returns its parent's index. If no such range is found, it returns -1. If there
    is no parent, it returns the found range's index. The parent of a range is the
    range with the earliest start time that completely covers the child range.
    "Completely covers" means:
    parent.startTime <= child.startTime && parent.endTime >= child.endTime
*/
int TimelineModel::firstIndex(qint64 startTime) const
{
    Q_D(const TimelineModel);
    int index = d->firstIndexNoParents(startTime);
    if (index == -1)
        return -1;
    int parent = d->ranges[index].parent;
    return parent == -1 ? index : parent;
}

/*!
    Looks up the first range with an end time later than the specified \a startTime and
    returns its index. If no such range is found, it returns -1.
*/
int TimelineModel::TimelineModelPrivate::firstIndexNoParents(qint64 startTime) const
{
    // in the "endtime" list, find the first event that ends after startTime
    if (endTimes.isEmpty())
        return -1;
    if (endTimes.count() == 1 || endTimes.first().end > startTime)
        return endTimes.first().startIndex;
    if (endTimes.last().end <= startTime)
        return -1;

    return endTimes[lowerBound(endTimes, startTime) + 1].startIndex;
}

/*!
    Looks up the last range with a start time earlier than the specified \a endTime and
    returns its index. If no such range is found, it returns -1.
*/
int TimelineModel::lastIndex(qint64 endTime) const
{
    Q_D(const TimelineModel);
    // in the "starttime" list, find the last event that starts before endtime
    if (d->ranges.isEmpty() || d->ranges.first().start >= endTime)
        return -1;
    if (d->ranges.count() == 1)
        return 0;
    if (d->ranges.last().start < endTime)
        return d->ranges.count() - 1;

    return d->lowerBound(d->ranges, endTime);
}

QVariantMap TimelineModel::location(int index) const
{
    Q_UNUSED(index);
    QVariantMap map;
    return map;
}

/*!
    Returns \c true if this model can contain events of global type ID \a typeIndex. Otherwise
    returns \c false. The base model does not know anything about type IDs and always returns
    \c false. You should override this method if you implement \l typeId().
 */
bool TimelineModel::handlesTypeId(int typeIndex) const
{
    Q_UNUSED(typeIndex);
    return false;
}

int TimelineModel::selectionIdForLocation(const QString &filename, int line, int column) const
{
    Q_UNUSED(filename);
    Q_UNUSED(line);
    Q_UNUSED(column);
    return -1;
}

float TimelineModel::relativeHeight(int index) const
{
    Q_UNUSED(index);
    return 1.0f;
}

int TimelineModel::rowMinValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

int TimelineModel::rowMaxValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

int TimelineModel::defaultRowHeight()
{
    return TimelineModelPrivate::DefaultRowHeight;
}

QColor TimelineModel::colorBySelectionId(int index) const
{
    return colorByHue(selectionId(index) * TimelineModelPrivate::SelectionIdHueMultiplier);
}

QColor TimelineModel::colorByFraction(double fraction) const
{
    return colorByHue(fraction * TimelineModelPrivate::FractionHueMultiplier +
                      TimelineModelPrivate::FractionHueMininimum);
}

QColor TimelineModel::colorByHue(int hue) const
{
    return QColor::fromHsl(hue % 360, TimelineModelPrivate::Saturation,
                           TimelineModelPrivate::Lightness);
}

/*!
    Inserts the range defined by \a duration and \a selectionId at the specified \a startTime and
    returns its index. The \a selectionId determines the selection group the new event belongs to.
    \sa selectionId()
*/
int TimelineModel::insert(qint64 startTime, qint64 duration, int selectionId)
{
    Q_D(TimelineModel);
    /* Doing insert-sort here is preferable as most of the time the times will actually be
     * presorted in the right way. So usually this will just result in appending. */
    int index = d->insertSorted(d->ranges,
                                TimelineModelPrivate::Range(startTime, duration, selectionId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    d->insertSorted(d->endTimes, TimelineModelPrivate::RangeEnd(index, startTime + duration));
    return index;
}

/*!
    Inserts the specified \a selectionId as range start at the specified \a startTime and returns
    its index. The range end is not set. The \a selectionId determines the selection group the new
    event belongs to.
    \sa selectionId()
*/
int TimelineModel::insertStart(qint64 startTime, int selectionId)
{
    Q_D(TimelineModel);
    int index = d->insertSorted(d->ranges, TimelineModelPrivate::Range(startTime, 0, selectionId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    return index;
}

/*!
    Adds the range \a duration at the specified start \a index.
*/
void TimelineModel::insertEnd(int index, qint64 duration)
{
    Q_D(TimelineModel);
    d->ranges[index].duration = duration;
    d->insertSorted(d->endTimes, TimelineModelPrivate::RangeEnd(index,
            d->ranges[index].start + duration));
}

bool TimelineModel::expanded() const
{
    Q_D(const TimelineModel);
    return d->expanded;
}

void TimelineModel::setExpanded(bool expanded)
{
    Q_D(TimelineModel);
    if (expanded != d->expanded) {
        d->expanded = expanded;
        emit expandedChanged();
        if (d->collapsedRowCount != d->expandedRowCount)
            emit rowCountChanged();
    }
}

bool TimelineModel::hidden() const
{
    Q_D(const TimelineModel);
    return d->hidden;
}

void TimelineModel::setHidden(bool hidden)
{
    Q_D(TimelineModel);
    if (hidden != d->hidden) {
        d->hidden = hidden;
        emit hiddenChanged();
    }
}

QString TimelineModel::displayName() const
{
    Q_D(const TimelineModel);
    return d->displayName;
}

int TimelineModel::rowCount() const
{
    Q_D(const TimelineModel);
    return d->expanded ? d->expandedRowCount : d->collapsedRowCount;
}

/*!
    Returns the ID of the selection group the event with event ID \a index belongs to. Selection
    groups are local to the model and the model can arbitrarily assign events to selection groups
    when inserting them.
    If one event from a selection group is selected, all visible other events from the same
    selection group are highlighted. Rows are expected to correspond to selection IDs when the view
    is expanded.
 */
int TimelineModel::selectionId(int index) const
{
    Q_D(const TimelineModel);
    return d->ranges[index].selectionId;
}

void TimelineModel::clear()
{
    Q_D(TimelineModel);
    bool hadRows = (rowCount() != 1);
    bool wasExpanded = d->expanded;
    bool wasHidden = d->hidden;
    bool hadRowHeights = !d->rowOffsets.empty();
    bool wasEmpty = isEmpty();
    d->collapsedRowCount = d->expandedRowCount = 1;
    d->rowOffsets.clear();
    d->expanded = false;
    d->hidden = false;
    d->ranges.clear();
    d->endTimes.clear();
    if (hadRowHeights)
        emit rowHeightChanged();
    if (wasExpanded)
        emit expandedChanged();
    if (wasHidden)
        emit hiddenChanged();
    if (hadRows)
        emit rowCountChanged();
    if (!wasEmpty)
        emit emptyChanged();
}

int TimelineModel::nextItemBySelectionId(int selectionId, qint64 time, int currentItem) const
{
    Q_D(const TimelineModel);
    return d->nextItemById(TimelineModelPrivate::SelectionId, selectionId, time, currentItem);
}

int TimelineModel::nextItemByTypeId(int typeId, qint64 time, int currentItem) const
{
    Q_D(const TimelineModel);
    return d->nextItemById(TimelineModelPrivate::TypeId, typeId, time, currentItem);
}

int TimelineModel::prevItemBySelectionId(int selectionId, qint64 time, int currentItem) const
{
    Q_D(const TimelineModel);
    return d->prevItemById(TimelineModelPrivate::SelectionId, selectionId, time, currentItem);
}

int TimelineModel::prevItemByTypeId(int typeId, qint64 time, int currentItem) const
{
    Q_D(const TimelineModel);
    return d->prevItemById(TimelineModelPrivate::TypeId, typeId, time, currentItem);
}

int TimelineModel::TimelineModelPrivate::nextItemById(IdType idType, int id, qint64 time,
                                                      int currentItem) const
{
    Q_Q(const TimelineModel);
    if (ranges.empty())
        return -1;

    int ndx = -1;
    if (currentItem == -1)
        ndx = firstIndexNoParents(time);
    else
        ndx = currentItem + 1;

    if (ndx < 0 || ndx >= ranges.count())
        ndx = 0;
    int startIndex = ndx;
    do {
        if ((idType == TypeId && q->typeId(ndx) == id) ||
                (idType == SelectionId && ranges[ndx].selectionId == id))
            return ndx;
        ndx = (ndx + 1) % ranges.count();
    } while (ndx != startIndex);
    return -1;
}

int TimelineModel::TimelineModelPrivate::prevItemById(IdType idType, int id, qint64 time,
                                                      int currentItem) const
{
    Q_Q(const TimelineModel);
    if (ranges.empty())
        return -1;

    int ndx = -1;
    if (currentItem == -1)
        ndx = firstIndexNoParents(time);
    else
        ndx = currentItem - 1;
    if (ndx < 0)
        ndx = ranges.count() - 1;
    int startIndex = ndx;
    do {
        if ((idType == TypeId && q->typeId(ndx) == id) ||
                (idType == SelectionId && ranges[ndx].selectionId == id))
            return ndx;
        if (--ndx < 0)
            ndx = ranges.count()-1;
    } while (ndx != startIndex);
    return -1;
}

}

#include "moc_timelinemodel.cpp"
