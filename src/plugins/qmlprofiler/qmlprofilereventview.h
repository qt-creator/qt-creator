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

#ifndef QMLPROFILEREVENTVIEW_H
#define QMLPROFILEREVENTVIEW_H

#include <QTreeView>
#include <QStandardItemModel>
#include <qmldebug/qmlprofilereventtypes.h>
#include "qmlprofilerdatamodel.h"

#include <analyzerbase/ianalyzertool.h>

#include "qmlprofilerviewmanager.h"

namespace QmlProfiler {
namespace Internal {

class QmlProfilerEventsMainView;
class QmlProfilerEventsParentsAndChildrenView;

enum ItemRole {
    EventHashStrRole = Qt::UserRole+1,
    FilenameRole = Qt::UserRole+2,
    LineRole = Qt::UserRole+3,
    ColumnRole = Qt::UserRole+4,
    EventIdRole = Qt::UserRole+5
};

class QmlProfilerEventsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProfilerEventsWidget(QWidget *parent,
                                     Analyzer::IAnalyzerTool *profilerTool,
                                     QmlProfilerViewManager *container,
                                     QmlProfilerDataModel *profilerDataModel );
    ~QmlProfilerEventsWidget();

    void switchToV8View();
    void clear();

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    QModelIndex selectedItem() const;
    bool mouseOnTable(const QPoint &position) const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    bool hasGlobalStats() const;
    void setShowExtendedStatistics(bool show);
    bool showExtendedStatistics() const;

    bool isQml() const;
    bool isV8() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void showEventInTimeline(int eventId);
    void resized();

public slots:
    void updateSelectedEvent(int eventId) const;
    void selectBySourceLocation(const QString &filename, int line, int column);

private slots:
    void profilerDataModelStateChanged();

protected:
    void contextMenuEvent(QContextMenuEvent *ev);
    virtual void resizeEvent(QResizeEvent *event);

private:
    class QmlProfilerEventsWidgetPrivate;
    QmlProfilerEventsWidgetPrivate *d;
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

    explicit QmlProfilerEventsMainView(ViewTypes viewType,
                                       QWidget *parent,
                                       QmlProfilerDataModel *dataModel);
    ~QmlProfilerEventsMainView();

    void setFieldViewable(Fields field, bool show);
    void setViewType(ViewTypes type);
    ViewTypes viewType() const;
    void setShowAnonymousEvents( bool showThem );

    QModelIndex selectedItem() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    static QString displayTime(double time);
    static QString nameForType(int typeNumber);

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    bool isRangeGlobal(qint64 rangeStart, qint64 rangeEnd) const;
    int selectedEventId() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void eventSelected(int eventId);
    void showEventInTimeline(int eventId);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectEvent(int eventId);
    void selectEventByLocation(const QString &filename, int line);
    void buildModel();
    void changeDetailsForEvent(int eventId, const QString &newString);

private slots:
    void profilerDataModelStateChanged();

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

    explicit QmlProfilerEventsParentsAndChildrenView(SubViewType subtableType,
                                                     QWidget *parent,
                                                     QmlProfilerDataModel *model);
    ~QmlProfilerEventsParentsAndChildrenView();

    void setViewType(SubViewType type);

signals:
    void eventClicked(int eventId);

public slots:
    void displayEvent(int eventId);
    void jumpToItem(const QModelIndex &);
    void clear();

private:
    void rebuildTree(void *profilerDataModel);
    void updateHeader();
    QStandardItemModel *treeModel();
    QmlProfilerDataModel *m_profilerDataModel;

    SubViewType m_subtableType;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILEREVENTVIEW_H
