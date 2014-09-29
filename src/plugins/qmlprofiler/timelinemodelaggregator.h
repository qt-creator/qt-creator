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
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
public:
    TimelineModelAggregator(QObject *parent = 0);
    ~TimelineModelAggregator();

    int height() const;
    void setModelManager(QmlProfilerModelManager *modelManager);
    void addModel(AbstractTimelineModel *m);
    const AbstractTimelineModel *model(int modelIndex) const;
    QVariantList models() const;

    Q_INVOKABLE int count(int modelIndex) const;
    void clear();
    Q_INVOKABLE int modelCount() const;

    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;

    Q_INVOKABLE bool isEmpty() const;

    Q_INVOKABLE int rowHeight(int modelIndex, int row) const;
    Q_INVOKABLE void setRowHeight(int modelIndex, int row, int height);
    Q_INVOKABLE int rowOffset(int modelIndex, int row) const;

    Q_INVOKABLE bool expanded(int modelIndex) const;
    Q_INVOKABLE void setExpanded(int modelIndex, bool expanded);

    Q_INVOKABLE bool hidden(int modelIndex) const;
    Q_INVOKABLE void setHidden(int modelIndex, bool hidden);

    Q_INVOKABLE int rowCount(int modelIndex) const;
    Q_INVOKABLE QString displayName(int modelIndex) const;
    Q_INVOKABLE int rowMinValue(int modelIndex, int row) const;
    Q_INVOKABLE int rowMaxValue(int modelIndex, int row) const;

    Q_INVOKABLE int firstIndex(int modelIndex, qint64 startTime) const;
    Q_INVOKABLE int firstIndexNoParents(int modelIndex, qint64 startTime) const;
    Q_INVOKABLE int lastIndex(int modelIndex, qint64 endTime) const;

    Q_INVOKABLE int row(int modelIndex, int index) const;
    Q_INVOKABLE qint64 duration(int modelIndex, int index) const;
    Q_INVOKABLE qint64 startTime(int modelIndex, int index) const;
    Q_INVOKABLE qint64 endTime(int modelIndex, int index) const;
    Q_INVOKABLE int selectionId(int modelIndex, int index) const;
    Q_INVOKABLE int bindingLoopDest(int modelIndex, int index) const;
    Q_INVOKABLE QColor color(int modelIndex, int index) const;
    Q_INVOKABLE float relativeHeight(int modelIndex, int index) const;

    Q_INVOKABLE QVariantList labels(int modelIndex) const;

    Q_INVOKABLE QVariantMap details(int modelIndex, int index) const;
    Q_INVOKABLE QVariantMap location(int modelIndex, int index) const;

    Q_INVOKABLE bool isSelectionIdValid(int modelIndex, int typeIndex) const;
    Q_INVOKABLE int selectionIdForLocation(int modelIndex, const QString &filename, int line,
                                       int column) const;

    Q_INVOKABLE void swapModels(int modelIndex1, int modelIndex2);

signals:
    void dataAvailable();
    void stateChanged();
    void expandedChanged();
    void hiddenChanged();
    void rowHeightChanged();
    void modelsChanged(int modelIndex1, int modelIndex2);
    void heightChanged();

protected slots:
    void dataChanged();

private:
    class TimelineModelAggregatorPrivate;
    TimelineModelAggregatorPrivate *d;
};

}
}

#endif // TIMELINEMODELAGGREGATOR_H
