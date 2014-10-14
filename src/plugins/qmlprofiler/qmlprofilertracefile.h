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

#ifndef QMLPROFILERTRACEFILE_H
#define QMLPROFILERTRACEFILE_H

#include <QObject>
#include <QVector>
#include <QString>

#include <qmldebug/qmlprofilereventlocation.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include "qmlprofilerdatamodel.h"
#include "qv8profilerdatamodel.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)


namespace QmlProfiler {
namespace Internal {

class QmlProfilerFileReader : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerFileReader(QObject *parent = 0);

    void setV8DataModel(QV8ProfilerDataModel *dataModel);
    void setQmlDataModel(QmlProfilerDataModel *dataModel);

    bool load(QIODevice *device);

signals:
    void traceStartTime(qint64 traceStartTime);
    void traceEndTime(qint64 traceStartTime);

    void error(const QString &error);

private:
    void loadEventData(QXmlStreamReader &reader);
    void loadProfilerDataModel(QXmlStreamReader &reader);

    QV8ProfilerDataModel *m_v8Model;
    QmlProfilerDataModel *m_qmlModel;
    QVector<QmlProfilerDataModel::QmlEventTypeData> m_qmlEvents;
    QVector<QmlProfilerDataModel::QmlEventData> m_ranges;
};


class QmlProfilerFileWriter : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerFileWriter(QObject *parent = 0);

    void setTraceTime(qint64 startTime, qint64 endTime, qint64 measturedTime);
    void setV8DataModel(QV8ProfilerDataModel *dataModel);
    void setQmlEvents(const QVector<QmlProfilerDataModel::QmlEventTypeData> &types,
                      const QVector<QmlProfilerDataModel::QmlEventData> &events);

    void save(QIODevice *device);

private:
    void calculateMeasuredTime();


    qint64 m_startTime, m_endTime, m_measuredTime;
    QV8ProfilerDataModel *m_v8Model;
    QVector<QmlProfilerDataModel::QmlEventTypeData> m_qmlEvents;
    QVector<QmlProfilerDataModel::QmlEventData> m_ranges;
};


} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTRACEFILE_H
