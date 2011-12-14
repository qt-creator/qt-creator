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

#ifndef QMLPROFILEREVENTVIEW_H
#define QMLPROFILEREVENTVIEW_H

#include <QtGui/QTreeView>
#include <qmljsdebugclient/qmlprofilereventtypes.h>
#include <qmljsdebugclient/qmlprofilereventlist.h>
#include <QtGui/QStandardItemModel>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerEventsMainView;
class QmlProfilerEventsParentsAndChildrenView;

typedef QHash<QString, QmlJsDebugClient::QmlEventData *> QmlEventHash;
typedef QList<QmlJsDebugClient::QmlEventData *> QmlEventList;

enum ItemRole {
    EventHashStrRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3,
    EventIdRole = Qt::UserRole+4
};

class QmlProfilerEventsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProfilerEventsWidget(QmlJsDebugClient::QmlProfilerEventList *model, QWidget *parent);
    ~QmlProfilerEventsWidget();

    void switchToV8View();
    void clear();

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    QModelIndex selectedItem() const;
    bool mouseOnTable(const QPoint &position) const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    bool hasGlobalStats() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber);
    void contextMenuRequested(const QPoint &position);
    void showEventInTimeline(int eventId);

public slots:
    void updateSelectedEvent(int eventId) const;
    void selectBySourceLocation(const QString &filename, int line);

protected:
    void contextMenuEvent(QContextMenuEvent *ev);

private:
    QmlProfilerEventsMainView *m_eventTree;
    QmlProfilerEventsParentsAndChildrenView *m_eventChildren;
    QmlProfilerEventsParentsAndChildrenView *m_eventParents;

    bool m_globalStatsEnabled;
};

class QmlProfilerEventsMainView : public QTreeView
{
    Q_OBJECT
public:
    enum Fields {
        Name,
        Type,
        Percent,
        TotalDuration,
        SelfPercent,
        SelfDuration,
        CallCount,
        TimePerCall,
        MaxTime,
        MinTime,
        MedianTime,
        Details,

        MaxFields
    };

    enum ViewTypes {
        EventsView,
        CallersView,
        CalleesView,
        V8ProfileView,

        MaxViewTypes
    };

    explicit QmlProfilerEventsMainView(QmlJsDebugClient::QmlProfilerEventList *model, QWidget *parent);
    ~QmlProfilerEventsMainView();

    void setEventStatisticsModel(QmlJsDebugClient::QmlProfilerEventList *model);
    void setFieldViewable(Fields field, bool show);
    void setViewType(ViewTypes type);
    void setShowAnonymousEvents( bool showThem );

    QModelIndex selectedItem() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    static QString displayTime(double time);
    static QString nameForType(int typeNumber);

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    bool isRangeGlobal(qint64 rangeStart, qint64 rangeEnd) const;
    int selectedEventId() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber);
    void eventSelected(int eventId);
    void showEventInTimeline(int eventId);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectEvent(int eventId);
    void selectEventByLocation(const QString &filename, int line);
    void buildModel();

private:
    void setHeaderLabels();

private:
    class QmlProfilerEventsMainViewPrivate;
    QmlProfilerEventsMainViewPrivate *d;

};

class QmlProfilerEventsParentsAndChildrenView : public QTreeView
{
    Q_OBJECT
public:
    enum SubViewType {
        ParentsView,
        ChildrenView,
        V8ParentsView,
        V8ChildrenView,
        MaxSubtableTypes
    };

    explicit QmlProfilerEventsParentsAndChildrenView(QmlJsDebugClient::QmlProfilerEventList *model, SubViewType subtableType, QWidget *parent);
    ~QmlProfilerEventsParentsAndChildrenView();

    void setViewType(SubViewType type);

signals:
    void eventClicked(int eventId);

public slots:
    void displayEvent(int eventId);
    void jumpToItem(const QModelIndex &);
    void clear();

private:
    void rebuildTree(void *eventList);
    void updateHeader();
    QStandardItemModel *treeModel();
    QmlJsDebugClient::QmlProfilerEventList *m_eventList;

    SubViewType m_subtableType;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILEREVENTVIEW_H
