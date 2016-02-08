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

#ifndef QMLPROFILERTRACEFILE_H
#define QMLPROFILERTRACEFILE_H

#include <QFutureInterface>
#include <QObject>
#include <QVector>
#include <QString>

#include <qmldebug/qmlprofilereventlocation.h>
#include <qmldebug/qmlprofilereventtypes.h>

#include "qmlprofilerdatamodel.h"

QT_FORWARD_DECLARE_CLASS(QIODevice)
QT_FORWARD_DECLARE_CLASS(QXmlStreamReader)


namespace QmlProfiler {
namespace Internal {

class QmlProfilerFileReader : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerFileReader(QObject *parent = 0);

    void setFuture(QFutureInterface<void> *future);

    bool load(QIODevice *device);
    quint64 loadedFeatures() const;

    qint64 traceStart() const { return m_traceStart; }
    qint64 traceEnd() const { return m_traceEnd; }

    const QVector<QmlProfilerDataModel::QmlEventTypeData> &qmlEvents() const { return m_qmlEvents; }
    const QVector<QmlProfilerDataModel::QmlEventData> &ranges() const { return m_ranges; }
    const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes() const { return m_notes; }

signals:
    void error(const QString &error);
    void success();

private:
    void loadEventData(QXmlStreamReader &reader);
    void loadProfilerDataModel(QXmlStreamReader &reader);
    void loadNoteData(QXmlStreamReader &reader);
    void progress(QIODevice *device);
    bool isCanceled() const;

    qint64 m_traceStart, m_traceEnd;
    QFutureInterface<void> *m_future;
    QVector<QmlProfilerDataModel::QmlEventTypeData> m_qmlEvents;
    QVector<QmlProfilerDataModel::QmlEventData> m_ranges;
    QVector<QmlProfilerDataModel::QmlEventNoteData> m_notes;
    quint64 m_loadedFeatures;
};


class QmlProfilerFileWriter : public QObject
{
    Q_OBJECT

public:
    explicit QmlProfilerFileWriter(QObject *parent = 0);

    void setTraceTime(qint64 startTime, qint64 endTime, qint64 measturedTime);
    void setQmlEvents(const QVector<QmlProfilerDataModel::QmlEventTypeData> &types,
                      const QVector<QmlProfilerDataModel::QmlEventData> &events);
    void setNotes(const QVector<QmlProfilerDataModel::QmlEventNoteData> &notes);
    void setFuture(QFutureInterface<void> *future);

    void save(QIODevice *device);

private:
    void calculateMeasuredTime();
    void incrementProgress();
    bool isCanceled() const;

    qint64 m_startTime, m_endTime, m_measuredTime;
    QFutureInterface<void> *m_future;
    QVector<QmlProfilerDataModel::QmlEventTypeData> m_qmlEvents;
    QVector<QmlProfilerDataModel::QmlEventData> m_ranges;
    QVector<QmlProfilerDataModel::QmlEventNoteData> m_notes;
    int m_newProgressValue;
};


} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERTRACEFILE_H
