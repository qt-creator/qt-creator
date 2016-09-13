/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qmlprofilerdatamodel.h"
#include "qmlprofilernotesmodel.h"
#include "qmlprofilereventtypes.h"
#include "qmleventlocation.h"
#include "qmlprofilerconstants.h"

#include <QHash>
#include <QStack>
#include <QVector>
#include <QObject>

namespace QmlProfiler {
class QmlProfilerModelManager;
class QmlProfilerStatisticsRelativesModel;

enum QmlProfilerStatisticsRelation {
    QmlProfilerStatisticsChilden,
    QmlProfilerStatisticsParents
};

class QmlProfilerStatisticsModel : public QObject
{
    Q_OBJECT
public:
    struct QmlEventStats {
        QmlEventStats() : duration(0), durationSelf(0), calls(0),
            minTime(std::numeric_limits<qint64>::max()), maxTime(0), timePerCall(0),
            percentOfTime(0), percentSelf(0), medianTime(0), isBindingLoop(false) {}
        qint64 duration;
        qint64 durationSelf;
        qint64 calls;
        qint64 minTime;
        qint64 maxTime;
        qint64 timePerCall;
        double percentOfTime;
        double percentSelf;
        qint64 medianTime;

        bool isBindingLoop;
    };

    QmlProfilerStatisticsModel(QmlProfilerModelManager *modelManager, QObject *parent = 0);
    ~QmlProfilerStatisticsModel();

    void restrictToFeatures(qint64 features);

    const QHash<int, QmlEventStats> &getData() const;
    const QVector<QmlEventType> &getTypes() const;
    const QHash<int, QString> &getNotes() const;

    int count() const;
    void clear();

    void setRelativesModel(QmlProfilerStatisticsRelativesModel *childModel,
                           QmlProfilerStatisticsRelation relation);
    QmlProfilerModelManager *modelManager() const;

signals:
    void dataAvailable();
    void notesAvailable(int typeIndex);

private:
    void loadEvent(const QmlEvent &event, const QmlEventType &type);
    void finalize();

private slots:
    void dataChanged();
    void notesChanged(int typeIndex);

private:
    class QmlProfilerStatisticsModelPrivate;
    QmlProfilerStatisticsModelPrivate *d;
};

class QmlProfilerStatisticsRelativesModel : public QObject
{
    Q_OBJECT
public:

    struct QmlStatisticsRelativesData {
        qint64 duration;
        qint64 calls;
        bool isBindingLoop;
    };
    typedef QHash <int, QmlStatisticsRelativesData> QmlStatisticsRelativesMap;

    QmlProfilerStatisticsRelativesModel(QmlProfilerModelManager *modelManager,
                                        QmlProfilerStatisticsModel *statisticsModel,
                                        QmlProfilerStatisticsRelation relation,
                                        QObject *parent = 0);

    int count() const;
    void clear();

    const QmlStatisticsRelativesMap &getData(int typeId) const;
    const QVector<QmlEventType> &getTypes() const;

    void loadEvent(RangeType type, const QmlEvent &event);
    void finalize(const QSet<int> &eventsInBindingLoop);

    QmlProfilerStatisticsRelation relation() const;

signals:
    void dataAvailable();

protected:
    QHash <int, QmlStatisticsRelativesMap> m_data;
    QmlProfilerModelManager *m_modelManager;

    struct Frame {
        qint64 startTime;
        int typeId;
    };
    QStack<Frame> m_callStack;
    QStack<Frame> m_compileStack;
    const QmlProfilerStatisticsRelation m_relation;
};

} // namespace QmlProfiler
