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

#ifndef QMLPROFILEREVENTVIEW_H
#define QMLPROFILEREVENTVIEW_H

#include "qmlprofilermodelmanager.h"
#include "qmlprofilereventsmodelproxy.h"
#include "qmlprofilerviewmanager.h"

#include <qmldebug/qmlprofilereventtypes.h>
#include <analyzerbase/ianalyzertool.h>
#include <utils/itemviews.h>

#include <QStandardItemModel>

namespace QmlProfiler {
namespace Internal {

class QmlProfilerEventsMainView;
class QmlProfilerEventChildrenView;
class QmlProfilerEventRelativesView;

enum ItemRole {
    SortRole = Qt::UserRole + 1, // Sort by data, not by displayed string
    TypeIdRole,
    FilenameRole,
    LineRole,
    ColumnRole
};

enum Fields {
    Name,
    Callee,
    CalleeDescription,
    Caller,
    CallerDescription,
    CallCount,
    Details,
    Location,
    MaxTime,
    TimePerCall,
    SelfTime,
    SelfTimeInPercent,
    MinTime,
    TimeInPercent,
    TotalTime,
    Type,
    MedianTime,
    MaxFields
};

class QmlProfilerEventsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QmlProfilerEventsWidget(QWidget *parent,
                                     QmlProfilerTool *profilerTool,
                                     QmlProfilerViewManager *container,
                                     QmlProfilerModelManager *profilerModelManager );
    ~QmlProfilerEventsWidget();

    void clear();

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    QModelIndex selectedModelIndex() const;
    bool mouseOnTable(const QPoint &position) const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    bool hasGlobalStats() const;
    void setShowExtendedStatistics(bool show);
    bool showExtendedStatistics() const;

signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);
    void resized();

public slots:
    void selectByTypeId(int typeIndex) const;
    void selectBySourceLocation(const QString &filename, int line, int column);
    void onVisibleFeaturesChanged(quint64 features);

private slots:
    void profilerDataModelStateChanged();

protected:
    void contextMenuEvent(QContextMenuEvent *ev);
    virtual void resizeEvent(QResizeEvent *event);

private:
    class QmlProfilerEventsWidgetPrivate;
    QmlProfilerEventsWidgetPrivate *d;
};

class QmlProfilerEventsMainView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerEventsMainView(QWidget *parent,
                                       QmlProfilerEventsModelProxy *modelProxy);
    ~QmlProfilerEventsMainView();

    void setFieldViewable(Fields field, bool show);
    void setShowAnonymousEvents( bool showThem );

    QModelIndex selectedModelIndex() const;
    void copyTableToClipboard() const;
    void copyRowToClipboard() const;

    static QString nameForType(QmlDebug::RangeType typeNumber);

    void getStatisticsInRange(qint64 rangeStart, qint64 rangeEnd);
    int selectedTypeId() const;

    void setShowExtendedStatistics(bool);
    bool showExtendedStatistics() const;


signals:
    void gotoSourceLocation(const QString &fileName, int lineNumber, int columnNumber);
    void typeSelected(int typeIndex);

public slots:
    void clear();
    void jumpToItem(const QModelIndex &index);
    void selectType(int typeIndex);
    void selectByLocation(const QString &filename, int line, int column);
    void buildModel();
    void updateNotes(int typeIndex);

private slots:
    void profilerDataModelStateChanged();

private:
    void selectItem(const QStandardItem *item);
    void setHeaderLabels();
    void parseModelProxy();

private:
    class QmlProfilerEventsMainViewPrivate;
    QmlProfilerEventsMainViewPrivate *d;

};

class QmlProfilerEventRelativesView : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit QmlProfilerEventRelativesView(QmlProfilerEventRelativesModelProxy *modelProxy,
                                           QWidget *parent );
    ~QmlProfilerEventRelativesView();

signals:
    void typeClicked(int typeIndex);

public slots:
    void displayType(int typeIndex);
    void jumpToItem(const QModelIndex &);
    void clear();

private:
    void rebuildTree(const QmlProfilerEventParentsModelProxy::QmlEventRelativesMap &eventMap);
    void updateHeader();
    QStandardItemModel *treeModel();

    class QmlProfilerEventParentsViewPrivate;
    QmlProfilerEventParentsViewPrivate *d;
};

} // namespace Internal
} // namespace QmlProfiler

#endif // QMLPROFILEREVENTVIEW_H
