/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QMLPROFILERDATAMODEL_H
#define QMLPROFILERDATAMODEL_H

#include <qmldebug/qmlprofilereventtypes.h>
#include <qmldebug/qmlprofilereventlocation.h>
#include "qv8profilerdatamodel.h"

#include <QHash>
#include <QObject>

namespace QmlProfiler {
namespace Internal {

// used for parents and children
struct QmlRangeEventRelative;

struct QmlRangeEventData
{
    QmlRangeEventData();
    ~QmlRangeEventData();

    int eventId;
    int bindingType;
    QString displayName;
    QString eventHashStr;
    QString details;
    QmlDebug::QmlEventLocation location;
    QmlDebug::QmlEventType eventType;

    QHash <QString, QmlRangeEventRelative *> parentHash;
    QHash <QString, QmlRangeEventRelative *> childrenHash;

    qint64 duration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double timePerCall;
    double percentOfTime;
    qint64 medianTime;

    bool isBindingLoop;

    QmlRangeEventData &operator=(const QmlRangeEventData &ref);
};

struct QmlRangeEventRelative {
    QmlRangeEventRelative(QmlRangeEventData *from) : reference(from), duration(0), calls(0), inLoopPath(false) {}
    QmlRangeEventRelative(QmlRangeEventRelative *from) : reference(from->reference), duration(from->duration), calls(from->calls), inLoopPath(from->inLoopPath) {}
    QmlRangeEventData *reference;
    qint64 duration;
    qint64 calls;
    bool inLoopPath;
};

class QmlProfilerDataModel : public QObject
{
    Q_OBJECT
public:
    enum State {
        Empty,
        AcquiringData,
        ProcessingData,
        Done
    };

    explicit QmlProfilerDataModel(QObject *parent = 0);
    ~QmlProfilerDataModel();

    QList<QmlRangeEventData *> getEventDescriptions() const;
    QmlRangeEventData *eventDescription(int eventId) const;
    QList<QV8EventData *> getV8Events() const;
    QV8EventData *v8EventDescription(int eventId) const;

    static QString getHashStringForQmlEvent(const QmlDebug::QmlEventLocation &location, int eventType);
    static QString getHashStringForV8Event(const QString &displayName, const QString &function);
    static QString rootEventName();
    static QString rootEventDescription();
    static QString qmlEventTypeAsString(QmlDebug::QmlEventType typeEnum);
    static QmlDebug::QmlEventType qmlEventTypeAsEnum(const QString &typeString);

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;
    Q_INVOKABLE qint64 firstTimeMark() const;
    Q_INVOKABLE qint64 lastTimeMark() const;

    // data access
    Q_INVOKABLE int count() const;
    Q_INVOKABLE bool isEmpty() const;
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE int getType(int index) const;
    Q_INVOKABLE int getNestingLevel(int index) const;
    Q_INVOKABLE int getNestingDepth(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE int getColumn(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
    Q_INVOKABLE int getBindingLoopDest(int index) const;
    Q_INVOKABLE int getFramerate(int index) const;
    Q_INVOKABLE int getAnimationCount(int index) const;
    Q_INVOKABLE int getMaximumAnimationCount() const;
    Q_INVOKABLE int getMinimumAnimationCount() const;


    // per-type data
    Q_INVOKABLE int uniqueEventsOfType(int type) const;
    Q_INVOKABLE int maxNestingForType(int type) const;
    Q_INVOKABLE QString eventTextForType(int type, int index) const;
    Q_INVOKABLE QString eventDisplayNameForType(int type, int index) const;
    Q_INVOKABLE int eventIdForType(int type, int index) const;
    Q_INVOKABLE int eventPosInType(int index) const;

    Q_INVOKABLE qint64 traceStartTime() const;
    Q_INVOKABLE qint64 traceEndTime() const;
    Q_INVOKABLE qint64 traceDuration() const;
    Q_INVOKABLE qint64 qmlMeasuredTime() const;
    Q_INVOKABLE qint64 v8MeasuredTime() const;

    void compileStatistics(qint64 startTime, qint64 endTime);
    State currentState() const;
    Q_INVOKABLE int getCurrentStateFromQml() const;

signals:
    void stateChanged();
    void countChanged();
    void error(const QString &error);

    void requestDetailsForLocation(int eventType, const QmlDebug::QmlEventLocation &location);
    void detailsChanged(int eventId, const QString &newString);
    void reloadDetailLabels();
    void reloadDocumentsForDetails();

public slots:
    void clear();

    void prepareForWriting();
    void addRangedEvent(int type, int bindingType, qint64 startTime, qint64 length,
                        const QStringList &data, const QmlDebug::QmlEventLocation &location);
    void addV8Event(int depth,const QString &function,const QString &filename, int lineNumber, double totalTime, double selfTime);
    void addFrameEvent(qint64 time, int framerate, int animationcount);
    void setTraceStartTime(qint64 time);
    void setTraceEndTime(qint64 time);

    void complete();

    bool save(const QString &filename);
    void load(const QString &filename);
    void setFilename(const QString &filename);
    void load();

    void rewriteDetailsString(int eventType, const QmlDebug::QmlEventLocation &location, const QString &newString);
    void finishedRewritingDetails();

private:
    void setState(State state);
    void reloadDetails();

private:
    class QmlProfilerDataModelPrivate;
    QmlProfilerDataModelPrivate *d;

    friend class QV8ProfilerDataModel;
};


} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILERDATAMODEL_H
