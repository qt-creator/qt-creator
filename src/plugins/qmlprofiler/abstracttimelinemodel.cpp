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

#include "abstracttimelinemodel.h"
#include "abstracttimelinemodel_p.h"

#include <QLinkedList>

namespace QmlProfiler {

/*!
    \class QmlProfiler::AbstractTimelineModel
    \brief The AbstractTimelineModel class provides a sorted model for timeline data.

    The AbstractTimelineModel lets you keep range data sorted by both start and end times, so that
    visible ranges can easily be computed. The only precondition for that to work is that the ranges
    must be perfectly nested. A "parent" range of a range R is defined as a range for which the
    start time is smaller than R's start time and the end time is greater than R's end time. A set
    of ranges is perfectly nested if all parent ranges of any given range have a common parent
    range. Mind that you can always make that happen by defining a range that spans the whole
    available time span. That, however, will make any code that uses firstStartTime() and
    lastEndTime() for selecting subsets of the model always select all of it.

    \note Indices returned from the various methods are only valid until a new range is inserted
          before them. Inserting a new range before a given index moves the range pointed to by the
          index by one. Incrementing the index by one will make it point to the item again.
*/

/*!
    \fn qint64 AbstractTimelineModelPrivate::firstStartTime() const
    Returns the begin of the first range in the model.
*/

/*!
    \fn qint64 AbstractTimelineModelPrivate::lastEndTime() const
    Returns the end of the last range in the model.
*/

/*!
    \fn const AbstractTimelineModelPrivate::Range &AbstractTimelineModelPrivate::range(int index) const
    Returns the range data at the specified index.
*/


/*!
    \fn void AbstractTimelineModel::computeNesting()
    Compute all ranges' parents.
    \sa findFirstIndex
*/
void AbstractTimelineModel::computeNesting()
{
    Q_D(AbstractTimelineModel);
    QLinkedList<int> parents;
    for (int range = 0; range != count(); ++range) {
        AbstractTimelineModelPrivate::Range &current = d->ranges[range];
        for (QLinkedList<int>::iterator parentIt = parents.begin();;) {
            if (parentIt == parents.end()) {
                parents.append(range);
                break;
            }

            AbstractTimelineModelPrivate::Range &parent = d->ranges[*parentIt];
            qint64 parentEnd = parent.start + parent.duration;
            if (parentEnd < current.start) {
                if (parent.start == current.start) {
                    if (parent.parent == -1) {
                        parent.parent = range;
                    } else {
                        AbstractTimelineModelPrivate::Range &ancestor = d->ranges[parent.parent];
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

int AbstractTimelineModel::collapsedRowCount() const
{
    Q_D(const AbstractTimelineModel);
    return d->collapsedRowCount;
}

void AbstractTimelineModel::setCollapsedRowCount(int rows)
{
    Q_D(AbstractTimelineModel);
    d->collapsedRowCount = rows;
}

int AbstractTimelineModel::expandedRowCount() const
{
    Q_D(const AbstractTimelineModel);
    return d->expandedRowCount;
}

void QmlProfiler::AbstractTimelineModel::setExpandedRowCount(int rows)
{
    Q_D(AbstractTimelineModel);
    d->expandedRowCount = rows;
}

void AbstractTimelineModel::AbstractTimelineModelPrivate::init(AbstractTimelineModel *q,
                                                               const QString &newDisplayName,
                                                               QmlDebug::Message newMessage,
                                                               QmlDebug::RangeType newRangeType)
{
    q_ptr = q;
    modelId = 0;
    modelManager = 0;
    expanded = false;
    hidden = false;
    displayName = newDisplayName;
    message = newMessage;
    rangeType = newRangeType;
    expandedRowCount = 1;
    collapsedRowCount = 1;
    connect(q,SIGNAL(rowHeightChanged()),q,SIGNAL(heightChanged()));
    connect(q,SIGNAL(expandedChanged()),q,SIGNAL(heightChanged()));
    connect(q,SIGNAL(hiddenChanged()),q,SIGNAL(heightChanged()));
}


AbstractTimelineModel::AbstractTimelineModel(AbstractTimelineModelPrivate *dd,
        const QString &displayName, QmlDebug::Message message, QmlDebug::RangeType rangeType,
        QObject *parent) :
    QObject(parent), d_ptr(dd)
{
    dd->init(this, displayName, message, rangeType);
}

AbstractTimelineModel::AbstractTimelineModel(const QString &displayName, QmlDebug::Message message,
                                             QmlDebug::RangeType rangeType, QObject *parent) :
    QObject(parent), d_ptr(new AbstractTimelineModelPrivate)
{
    d_ptr->init(this, displayName, message, rangeType);
}

AbstractTimelineModel::~AbstractTimelineModel()
{
    Q_D(AbstractTimelineModel);
    delete d;
}

void AbstractTimelineModel::setModelManager(QmlProfilerModelManager *modelManager)
{
    Q_D(AbstractTimelineModel);
    if (modelManager != d->modelManager) {
        if (d->modelManager != 0) {
            disconnect(d->modelManager->qmlModel(), SIGNAL(changed()),
                       this, SLOT(_q_dataChanged()));
            // completely unregistering is not supported
            d->modelManager->setProxyCountWeight(d->modelId, 0);
        }
        d->modelManager = modelManager;
        connect(d->modelManager->qmlModel(), SIGNAL(changed()),
                this, SLOT(_q_dataChanged()));
        d->modelId = d->modelManager->registerModelProxy();
        d->modelManager->announceFeatures(d->modelId, features());
        emit modelManagerChanged();
    }
}

QmlProfilerModelManager *AbstractTimelineModel::modelManager() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelManager;
}

bool AbstractTimelineModel::isEmpty() const
{
    return count() == 0;
}

int AbstractTimelineModel::modelId() const
{
    Q_D(const AbstractTimelineModel);
    return d->modelId;
}

int AbstractTimelineModel::rowHeight(int rowNumber) const
{
    Q_D(const AbstractTimelineModel);
    if (!expanded())
        return AbstractTimelineModelPrivate::DefaultRowHeight;

    if (d->rowOffsets.size() > rowNumber)
        return d->rowOffsets[rowNumber] - (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    return AbstractTimelineModelPrivate::DefaultRowHeight;
}

int AbstractTimelineModel::rowOffset(int rowNumber) const
{
    Q_D(const AbstractTimelineModel);
    if (rowNumber == 0)
        return 0;
    if (!expanded())
        return AbstractTimelineModelPrivate::DefaultRowHeight * rowNumber;

    if (d->rowOffsets.size() >= rowNumber)
        return d->rowOffsets[rowNumber - 1];
    if (!d->rowOffsets.empty())
        return d->rowOffsets.last() + (rowNumber - d->rowOffsets.size()) *
                AbstractTimelineModelPrivate::DefaultRowHeight;
    return rowNumber * AbstractTimelineModelPrivate::DefaultRowHeight;
}

void AbstractTimelineModel::setRowHeight(int rowNumber, int height)
{
    Q_D(AbstractTimelineModel);
    if (d->hidden || !d->expanded)
        return;
    if (height < AbstractTimelineModelPrivate::DefaultRowHeight)
        height = AbstractTimelineModelPrivate::DefaultRowHeight;

    int nextOffset = d->rowOffsets.empty() ? 0 : d->rowOffsets.last();
    while (d->rowOffsets.size() <= rowNumber)
        d->rowOffsets << (nextOffset += AbstractTimelineModelPrivate::DefaultRowHeight);
    int difference = height - d->rowOffsets[rowNumber] +
            (rowNumber > 0 ? d->rowOffsets[rowNumber - 1] : 0);
    if (difference != 0) {
        for (; rowNumber < d->rowOffsets.size(); ++rowNumber) {
            d->rowOffsets[rowNumber] += difference;
        }
        emit rowHeightChanged();
    }
}

int AbstractTimelineModel::height() const
{
    Q_D(const AbstractTimelineModel);
    int depth = rowCount();
    if (d->hidden || !d->expanded || d->rowOffsets.empty())
        return depth * AbstractTimelineModelPrivate::DefaultRowHeight;

    return d->rowOffsets.last() + (depth - d->rowOffsets.size()) *
            AbstractTimelineModelPrivate::DefaultRowHeight;
}

/*!
    \fn int AbstractTimelineModel::count() const
    Returns the number of ranges in the model.
*/
int AbstractTimelineModel::count() const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges.count();
}

qint64 AbstractTimelineModel::duration(int index) const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges[index].duration;
}

qint64 AbstractTimelineModel::startTime(int index) const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges[index].start;
}

qint64 AbstractTimelineModel::endTime(int index) const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges[index].start + d->ranges[index].duration;
}

int AbstractTimelineModel::typeId(int index) const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges[index].typeId;
}

/*!
    \fn int AbstractTimelineModel::firstIndex(qint64 startTime) const
    Looks up the first range with an end time greater than the given time and
    returns its parent's index. If no such range is found, it returns -1. If there
    is no parent, it returns the found range's index. The parent of a range is the
    range with the lowest start time that completely covers the child range.
    "Completely covers" means:
    parent.startTime <= child.startTime && parent.endTime >= child.endTime
*/
int AbstractTimelineModel::firstIndex(qint64 startTime) const
{
    Q_D(const AbstractTimelineModel);
    int index = firstIndexNoParents(startTime);
    if (index == -1)
        return -1;
    int parent = d->ranges[index].parent;
    return parent == -1 ? index : parent;
}

/*!
    \fn int AbstractTimelineModel::firstIndexNoParents(qint64 startTime) const
    Looks up the first range with an end time greater than the given time and
    returns its index. If no such range is found, it returns -1.
*/
int AbstractTimelineModel::firstIndexNoParents(qint64 startTime) const
{
    Q_D(const AbstractTimelineModel);
    // in the "endtime" list, find the first event that ends after startTime
    if (d->endTimes.isEmpty())
        return -1;
    if (d->endTimes.count() == 1 || d->endTimes.first().end > startTime)
        return d->endTimes.first().startIndex;
    if (d->endTimes.last().end <= startTime)
        return -1;

    return d->endTimes[d->lowerBound(d->endTimes, startTime) + 1].startIndex;
}

/*!
    \fn int AbstractTimelineModel::lastIndex(qint64 endTime) const
    Looks up the last range with a start time smaller than the given time and
    returns its index. If no such range is found, it returns -1.
*/
int AbstractTimelineModel::lastIndex(qint64 endTime) const
{
    Q_D(const AbstractTimelineModel);
    // in the "starttime" list, find the last event that starts before endtime
    if (d->ranges.isEmpty() || d->ranges.first().start >= endTime)
        return -1;
    if (d->ranges.count() == 1)
        return 0;
    if (d->ranges.last().start < endTime)
        return d->ranges.count() - 1;

    return d->lowerBound(d->ranges, endTime);
}

QVariantMap AbstractTimelineModel::location(int index) const
{
    Q_UNUSED(index);
    QVariantMap map;
    return map;
}

bool AbstractTimelineModel::handlesTypeId(int typeIndex) const
{
    Q_UNUSED(typeIndex);
    return false;
}

int AbstractTimelineModel::selectionIdForLocation(const QString &filename, int line, int column) const
{
    Q_UNUSED(filename);
    Q_UNUSED(line);
    Q_UNUSED(column);
    return -1;
}

int AbstractTimelineModel::bindingLoopDest(int index) const
{
    Q_UNUSED(index);
    return -1;
}

float AbstractTimelineModel::relativeHeight(int index) const
{
    Q_UNUSED(index);
    return 1.0f;
}

int AbstractTimelineModel::rowMinValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

int AbstractTimelineModel::rowMaxValue(int rowNumber) const
{
    Q_UNUSED(rowNumber);
    return 0;
}

int AbstractTimelineModel::defaultRowHeight()
{
    return AbstractTimelineModelPrivate::DefaultRowHeight;
}

QmlDebug::RangeType AbstractTimelineModel::rangeType() const
{
    Q_D(const AbstractTimelineModel);
    return d->rangeType;
}

QmlDebug::Message AbstractTimelineModel::message() const
{
    Q_D(const AbstractTimelineModel);
    return d->message;
}

void AbstractTimelineModel::updateProgress(qint64 count, qint64 max) const
{
    Q_D(const AbstractTimelineModel);
    d->modelManager->modelProxyCountUpdated(d->modelId, count, max);
}

QColor AbstractTimelineModel::colorBySelectionId(int index) const
{
    return colorByHue(selectionId(index) * AbstractTimelineModelPrivate::SelectionIdHueMultiplier);
}

QColor AbstractTimelineModel::colorByFraction(double fraction) const
{
    return colorByHue(fraction * AbstractTimelineModelPrivate::FractionHueMultiplier +
                      AbstractTimelineModelPrivate::FractionHueMininimum);
}

QColor AbstractTimelineModel::colorByHue(int hue) const
{
    return QColor::fromHsl(hue % 360, AbstractTimelineModelPrivate::Saturation,
                           AbstractTimelineModelPrivate::Lightness);
}

/*!
    \fn int AbstractTimelineModel::insert(qint64 startTime, qint64 duration)
    Inserts a range at the given time position and returns its index.
*/
int AbstractTimelineModel::insert(qint64 startTime, qint64 duration, int typeId)
{
    Q_D(AbstractTimelineModel);
    /* Doing insert-sort here is preferable as most of the time the times will actually be
     * presorted in the right way. So usually this will just result in appending. */
    int index = d->insertSorted(d->ranges,
                                AbstractTimelineModelPrivate::Range(startTime, duration, typeId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    d->insertSorted(d->endTimes,
                    AbstractTimelineModelPrivate::RangeEnd(index, startTime + duration));
    return index;
}

/*!
    \fn int AbstractTimelineModel::insertStart(qint64 startTime, int typeId)
    Inserts the given data as range start at the given time position and
    returns its index. The range end is not set.
*/
int AbstractTimelineModel::insertStart(qint64 startTime, int typeId)
{
    Q_D(AbstractTimelineModel);
    int index = d->insertSorted(d->ranges,
                                AbstractTimelineModelPrivate::Range(startTime, 0, typeId));
    if (index < d->ranges.size() - 1)
        d->incrementStartIndices(index);
    return index;
}

/*!
    \fn int AbstractTimelineModel::insertEnd(int index, qint64 duration)
    Adds a range end for the given start index.
*/
void AbstractTimelineModel::insertEnd(int index, qint64 duration)
{
    Q_D(AbstractTimelineModel);
    d->ranges[index].duration = duration;
    d->insertSorted(d->endTimes, AbstractTimelineModelPrivate::RangeEnd(index,
            d->ranges[index].start + duration));
}

void AbstractTimelineModel::AbstractTimelineModelPrivate::_q_dataChanged()
{
    Q_Q(AbstractTimelineModel);
    bool wasEmpty = q->isEmpty();
    switch (modelManager->state()) {
    case QmlProfilerDataState::ProcessingData:
        q->loadData();
        break;
    case QmlProfilerDataState::ClearingData:
        q->clear();
        break;
    default:
        break;
    }
    if (wasEmpty != q->isEmpty())
        emit q->emptyChanged();
}

bool AbstractTimelineModel::accepted(const QmlProfilerDataModel::QmlEventTypeData &event) const
{
    Q_D(const AbstractTimelineModel);
    return (event.rangeType == d->rangeType && event.message == d->message);
}

bool AbstractTimelineModel::expanded() const
{
    Q_D(const AbstractTimelineModel);
    return d->expanded;
}

void AbstractTimelineModel::setExpanded(bool expanded)
{
    Q_D(AbstractTimelineModel);
    if (expanded != d->expanded) {
        d->expanded = expanded;
        emit expandedChanged();
    }
}

bool AbstractTimelineModel::hidden() const
{
    Q_D(const AbstractTimelineModel);
    return d->hidden;
}

void AbstractTimelineModel::setHidden(bool hidden)
{
    Q_D(AbstractTimelineModel);
    if (hidden != d->hidden) {
        d->hidden = hidden;
        emit hiddenChanged();
    }
}

QString AbstractTimelineModel::displayName() const
{
    Q_D(const AbstractTimelineModel);
    return d->displayName;
}

int AbstractTimelineModel::rowCount() const
{
    Q_D(const AbstractTimelineModel);
    if (d->hidden)
        return 0;
    if (isEmpty())
        return d->modelManager->isEmpty() ? 1 : 0;
    return d->expanded ? d->expandedRowCount : d->collapsedRowCount;
}

int AbstractTimelineModel::selectionId(int index) const
{
    Q_D(const AbstractTimelineModel);
    return d->ranges[index].typeId;
}

void AbstractTimelineModel::clear()
{
    Q_D(AbstractTimelineModel);
    d->collapsedRowCount = d->expandedRowCount = 1;
    bool wasExpanded = d->expanded;
    bool wasHidden = d->hidden;
    bool hadRowHeights = !d->rowOffsets.empty();
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
    updateProgress(0, 1);
}

}

#include "moc_abstracttimelinemodel.cpp"
