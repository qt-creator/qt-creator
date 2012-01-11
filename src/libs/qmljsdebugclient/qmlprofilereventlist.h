/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLPROFILEREVENTLIST_H
#define QMLPROFILEREVENTLIST_H

#include "qmlprofilereventtypes.h"
#include "qmljsdebugclient_global.h"

#include <QtCore/QHash>
#include <QtCore/QObject>

namespace QmlJsDebugClient {

struct QmlEventSub;
struct QV8EventSub;

struct QMLJSDEBUGCLIENT_EXPORT QmlEventData
{
    QmlEventData();
    ~QmlEventData();

    QString displayname;
    QString filename;
    QString eventHashStr;
    QString details;
    int line;
    QmlJsDebugClient::QmlEventType eventType;
    QHash <QString, QmlEventSub *> parentHash;
    QHash <QString, QmlEventSub *> childrenHash;
    qint64 duration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double timePerCall;
    double percentOfTime;
    qint64 medianTime;
    int eventId;

    QmlEventData &operator=(const QmlEventData &ref);
};

struct QMLJSDEBUGCLIENT_EXPORT QmlEventSub {
    QmlEventSub(QmlEventData *from) : reference(from), duration(0), calls(0) {}
    QmlEventSub(QmlEventSub *from) : reference(from->reference), duration(from->duration), calls(from->calls) {}
    QmlEventData *reference;
    qint64 duration;
    qint64 calls;
};

struct QMLJSDEBUGCLIENT_EXPORT QV8EventData
{
    QV8EventData();
    ~QV8EventData();

    QString displayName;
    QString filename;
    QString functionName;
    int line;
    double totalTime; // given in milliseconds
    double totalPercent;
    double selfTime;
    double selfPercent;
    QHash <QString, QV8EventSub *> parentHash;
    QHash <QString, QV8EventSub *> childrenHash;
    int eventId;

    QV8EventData &operator=(const QV8EventData &ref);
};

struct QMLJSDEBUGCLIENT_EXPORT QV8EventSub {
    QV8EventSub(QV8EventData *from) : reference(from), totalTime(0) {}
    QV8EventSub(QV8EventSub *from) : reference(from->reference), totalTime(from->totalTime) {}

    QV8EventData *reference;
    qint64 totalTime;
};

typedef QHash<QString, QmlEventData *> QmlEventHash;
typedef QList<QmlEventData *> QmlEventDescriptions;
typedef QList<QV8EventData *> QV8EventDescriptions;

class QMLJSDEBUGCLIENT_EXPORT QmlProfilerEventList : public QObject
{
    Q_OBJECT
public:

    explicit QmlProfilerEventList(QObject *parent = 0);
    ~QmlProfilerEventList();

    QmlEventDescriptions getEventDescriptions() const;
    QmlEventData *eventDescription(int eventId) const;
    const QV8EventDescriptions& getV8Events() const;
    QV8EventData *v8EventDescription(int eventId) const;

    int findFirstIndex(qint64 startTime) const;
    int findFirstIndexNoParents(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;
    Q_INVOKABLE qint64 firstTimeMark() const;
    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE int count() const;

    // data access
    Q_INVOKABLE qint64 getStartTime(int index) const;
    Q_INVOKABLE qint64 getEndTime(int index) const;
    Q_INVOKABLE qint64 getDuration(int index) const;
    Q_INVOKABLE int getType(int index) const;
    Q_INVOKABLE int getNestingLevel(int index) const;
    Q_INVOKABLE int getNestingDepth(int index) const;
    Q_INVOKABLE QString getFilename(int index) const;
    Q_INVOKABLE int getLine(int index) const;
    Q_INVOKABLE QString getDetails(int index) const;
    Q_INVOKABLE int getEventId(int index) const;
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

    void showErrorDialog(const QString &st ) const;
    void compileStatistics(qint64 startTime, qint64 endTime);
signals:
    void dataReady();
    void countChanged();
    void error(const QString &error);
    void dataClear();
    void processingData();
    void postProcessing();

public slots:
    void clear();
    void addRangedEvent(int type, qint64 startTime, qint64 length,
                        const QStringList &data, const QString &fileName, int line);
    void complete();

    void addV8Event(int depth,const QString &function,const QString &filename, int lineNumber, double totalTime, double selfTime);
    void addFrameEvent(qint64 time, int framerate, int animationcount);
    bool save(const QString &filename);
    void load(const QString &filename);
    void setFilename(const QString &filename);
    void load();

    void setTraceEndTime( qint64 time );
    void setTraceStartTime( qint64 time );

private:
    void postProcess();
    void sortEndTimes();
    void findAnimationLimits();
    void sortStartTimes();
    void computeLevels();
    void computeNestingLevels();
    void computeNestingDepth();
    void prepareForDisplay();
    void linkEndsToStarts();

private:
    class QmlProfilerEventListPrivate;
    QmlProfilerEventListPrivate *d;
};


} // namespace QmlJsDebugClient

#endif // QMLPROFILEREVENTLIST_H
