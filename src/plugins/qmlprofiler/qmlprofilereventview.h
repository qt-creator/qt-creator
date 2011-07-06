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

#ifndef QMLPROFILEREVENTVIEW_H
#define QMLPROFILEREVENTVIEW_H

#include <QTreeView>
#include "qmlprofilereventtypes.h"

namespace QmlProfiler {
namespace Internal {

struct QmlEventData
{
    QmlEventData() : displayname(0) , filename(0) , location(0) , details(0),
        line(0), eventType(MaximumQmlEventType), level(-1), parentList(0), childrenList(0) {}
    ~QmlEventData() {
        delete displayname;
        delete filename;
        delete location;
        delete parentList;
        delete childrenList;
    }
    QString *displayname;
    QString *filename;
    QString *location;
    QString *details;
    int line;
    QmlEventType eventType;
    qint64 level;
    QList< QmlEventData *> *parentList;
    QList< QmlEventData *> *childrenList;
    qint64 duration;
    qint64 calls;
    qint64 minTime;
    qint64 maxTime;
    double timePerCall;
    double percentOfTime;
};


typedef QHash<QString, QmlEventData *> QmlEventHash;
typedef QList<QmlEventData *> QmlEventList;

enum ItemRole {
    LocationRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3
};

class QmlProfilerEventStatistics : public QObject
{
    Q_OBJECT
public:

    explicit QmlProfilerEventStatistics(QObject *parent = 0);
    ~QmlProfilerEventStatistics();

    QmlEventList getEventList();

signals:
    void dataReady();

public slots:
    void clear();
    void addRangedEvent(int type, int nestingLevel, int nestingInType, qint64 startTime, qint64 length,
                        const QStringList &data, const QString &fileName, int line);
    void complete();

private:
    class QmlProfilerEventStatisticsPrivate;
    QmlProfilerEventStatisticsPrivate *d;
};

class QmlProfilerEventsView : public QTreeView
{
    Q_OBJECT
public:
    enum Fields {
        Name,
        Type,
        Percent,
        TotalDuration,
        CallCount,
        TimePerCall,
        MaxTime,
        MinTime,
        Details,
        Parents,
        Children,

        MaxFields
    };

    enum ViewTypes {
        EventsView,
        CallersView,
        CalleesView,

        MaxViewTypes
    };

    explicit QmlProfilerEventsView(QWidget *parent, QmlProfilerEventStatistics *model);
    ~QmlProfilerEventsView();

    void setEventStatisticsModel( QmlProfilerEventStatistics *model );
    void setFieldViewable(Fields field, bool show);
    void setViewType(ViewTypes type);
    void setShowAnonymousEvents( bool showThem );

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void buildModel();

private:
    void setHeaderLabels();

private:
    class QmlProfilerEventsViewPrivate;
    QmlProfilerEventsViewPrivate *d;

};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILEREVENTVIEW_H
