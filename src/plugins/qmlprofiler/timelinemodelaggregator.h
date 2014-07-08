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

#ifndef TIMELINEMODELAGGREGATOR_H
#define TIMELINEMODELAGGREGATOR_H

#include "abstracttimelinemodel.h"
#include "qmlprofilermodelmanager.h"

namespace QmlProfiler {
namespace Internal {

class TimelineModelAggregator : public QObject
{
    Q_OBJECT
public:
    TimelineModelAggregator(QObject *parent = 0);
    ~TimelineModelAggregator();

    void setModelManager(QmlProfilerModelManager *modelManager);
    void addModel(AbstractTimelineModel *m);

    Q_INVOKABLE QStringList categoryTitles() const;
    Q_INVOKABLE int count(int modelIndex = -1) const;
    void clear();
    Q_INVOKABLE int modelCount() const;

    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;

    bool isEmpty() const;

    bool eventAccepted(const QmlProfilerDataModel::QmlEventData &event) const;

    Q_INVOKABLE int height(int modelIndex) const;
    Q_INVOKABLE int rowHeight(int modelIndex, int row) const;
    Q_INVOKABLE void setRowHeight(int modelIndex, int row, int height);
    Q_INVOKABLE int rowOffset(int modelIndex, int row) const;
    Q_INVOKABLE bool expanded(int modelIndex) const;
    Q_INVOKABLE void setExpanded(int modelIndex, bool expanded);
    Q_INVOKABLE int rowCount(int modelIndex) const;
    Q_INVOKABLE const QString title(int modelIndex) const;
    Q_INVOKABLE int rowMinValue(int modelIndex, int row) const;
    Q_INVOKABLE int rowMaxValue(int modelIndex, int row) const;

    int findFirstIndex(int modelIndex, qint64 startTime) const;
    int findFirstIndexNoParents(int modelIndex, qint64 startTime) const;
    int findLastIndex(int modelIndex, qint64 endTime) const;

    int getEventRow(int modelIndex, int index) const;
    Q_INVOKABLE qint64 getDuration(int modelIndex, int index) const;
    Q_INVOKABLE qint64 getStartTime(int modelIndex, int index) const;
    Q_INVOKABLE qint64 getEndTime(int modelIndex, int index) const;
    Q_INVOKABLE int getEventId(int modelIndex, int index) const;
    Q_INVOKABLE int getBindingLoopDest(int modelIndex, int index) const;
    Q_INVOKABLE QColor getColor(int modelIndex, int index) const;
    Q_INVOKABLE float getHeight(int modelIndex, int index) const;

    Q_INVOKABLE const QVariantList getLabels(int modelIndex) const;

    Q_INVOKABLE const QVariantList getEventDetails(int modelIndex, int index) const;
    Q_INVOKABLE const QVariantMap getEventLocation(int modelIndex, int index) const;

    Q_INVOKABLE int getEventIdForTypeIndex(int modelIndex, int typeIndex) const;
    Q_INVOKABLE int getEventIdForLocation(int modelIndex, const QString &filename, int line,
                                          int column) const;

signals:
    void dataAvailable();
    void stateChanged();
    void expandedChanged();
    void rowHeightChanged();

protected slots:
    void dataChanged();

private:
    class TimelineModelAggregatorPrivate;
    TimelineModelAggregatorPrivate *d;
};

}
}

#endif // TIMELINEMODELAGGREGATOR_H
