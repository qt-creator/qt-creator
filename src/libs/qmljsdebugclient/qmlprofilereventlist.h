/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef QMLPROFILEREVENTLIST_H
#define QMLPROFILEREVENTLIST_H

#include "qmlprofilereventtypes.h"
#include "qmljsdebugclient_global.h"

#include <QtCore/QHash>
#include <QtCore/QObject>

namespace QmlJsDebugClient {

struct QMLJSDEBUGCLIENT_EXPORT QmlEventData
{
    QString displayname;
    QString filename;
    QString location;
    QString details;
    int line;
    QmlJsDebugClient::QmlEventType eventType;
    QList< QmlEventData *> parentList;
    QList< QmlEventData *> childrenList;
    qint64 cumulatedDuration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double timePerCall;
    double percentOfTime;
    qint64 medianTime;
};

typedef QHash<QString, QmlEventData *> QmlEventHash;
typedef QList<QmlEventData *> QmlEventDescriptions;

enum ParsingStatus {
    GettingDataStatus = 0,
    SortingListsStatus = 1,
    SortingEndsStatus = 2,
    ComputingLevelsStatus = 3,
    CompilingStatisticsStatus = 4,
    DoneStatus = 5
};

class QMLJSDEBUGCLIENT_EXPORT QmlProfilerEventList : public QObject
{
    Q_OBJECT
public:

    explicit QmlProfilerEventList(QObject *parent = 0);
    ~QmlProfilerEventList();

    QmlEventDescriptions getEventDescriptions() const;

    int findFirstIndex(qint64 startTime) const;
    int findLastIndex(qint64 endTime) const;
    Q_INVOKABLE qint64 firstTimeMark() const;
    Q_INVOKABLE qint64 lastTimeMark() const;

    Q_INVOKABLE int count() const;
    void setParsingStatus(ParsingStatus ps);
    Q_INVOKABLE ParsingStatus getParsingStatus() const;

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

    void showErrorDialog(const QString &st ) const;
signals:
    void dataReady();
    void countChanged();
    void parsingStatusChanged();
    void error(const QString &error);

public slots:
    void clear();
    void addRangedEvent(int type, qint64 startTime, qint64 length,
                        const QStringList &data, const QString &fileName, int line);
    void complete();
    void save(const QString &filename);
    void load(const QString &filename);
    void setFilename(const QString &filename);
    void load();

private slots:
    void postProcess();
    void sortEndTimes();
    void sortStartTimes();
    void computeLevels();
    void computeNestingLevels();
    void computeNestingDepth();
    void compileStatistics();
    void linkEndsToStarts();

private:
    class QmlProfilerEventListPrivate;
    QmlProfilerEventListPrivate *d;
};


} // namespace QmlJsDebugClient

#endif // QMLPROFILEREVENTLIST_H
