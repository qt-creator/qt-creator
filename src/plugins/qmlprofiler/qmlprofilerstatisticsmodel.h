/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
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
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLPROFILERSTATISTICSMODEL_H
#define QMLPROFILERSTATISTICSMODEL_H

#include "qmlprofilerdatamodel.h"
#include "qmlprofilernotesmodel.h"
#include <QObject>
#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
#include <QHash>
#include <QVector>

namespace QmlProfiler {
class QmlProfilerModelManager;

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

    void setEventTypeAccepted(QmlDebug::RangeType type, bool accepted);
    bool eventTypeAccepted(QmlDebug::RangeType) const;

    const QHash<int, QmlEventStats> &getData() const;
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &getTypes() const;
    const QHash<int, QString> &getNotes() const;

    int count() const;
    void clear();

    void limitToRange(qint64 rangeStart, qint64 rangeEnd);

signals:
    void dataAvailable();
    void notesAvailable(int typeIndex);

private:
    void loadData(qint64 rangeStart = -1, qint64 rangeEnd = -1);
    const QSet<int> &eventsInBindingLoop() const;

private slots:
    void dataChanged();
    void notesChanged(int typeIndex);

private:
    class QmlProfilerStatisticsModelPrivate;
    QmlProfilerStatisticsModelPrivate *d;

    friend class QmlProfilerStatisticsParentsModel;
    friend class QmlProfilerStatisticsChildrenModel;
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
                                        QObject *parent = 0);

    int count() const;
    void clear();

    const QmlStatisticsRelativesMap &getData(int typeId) const;
    QVariantList getNotes(int typeId) const;
    const QVector<QmlProfilerDataModel::QmlEventTypeData> &getTypes() const;

protected:
    virtual void loadData() = 0;

signals:
    void dataAvailable();

protected slots:
    void dataChanged();

protected:
    QHash <int, QmlStatisticsRelativesMap> m_data;
    QmlProfilerModelManager *m_modelManager;
    QmlProfilerStatisticsModel *m_statisticsModel;
};

class QmlProfilerStatisticsParentsModel : public QmlProfilerStatisticsRelativesModel
{
    Q_OBJECT
public:
    QmlProfilerStatisticsParentsModel(QmlProfilerModelManager *modelManager,
                                      QmlProfilerStatisticsModel *statisticsModel,
                                      QObject *parent = 0);

protected:
    virtual void loadData();
};

class QmlProfilerStatisticsChildrenModel : public QmlProfilerStatisticsRelativesModel
{
    Q_OBJECT
public:
    QmlProfilerStatisticsChildrenModel(QmlProfilerModelManager *modelManager,
                                       QmlProfilerStatisticsModel *statisticsModel,
                                       QObject *parent = 0);

protected:
    virtual void loadData();
};

}

#endif // QMLPROFILERSTATISTICSMODEL_H
